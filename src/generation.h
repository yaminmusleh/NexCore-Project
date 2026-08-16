#pragma once

#include <iostream>
#include <string>
#include <variant>
#include <vector>
#include <unordered_map>
#include <stdexcept>
#include <type_traits>
#include <utility>

class Generator
{
public:
    inline explicit Generator(NodeProgram root)
        : m_root(std::move(root))
    {
    }

    [[nodiscard]] std::string generate()
    {
        m_scopes.clear();
        m_scopes.emplace_back();
        m_labelCount = 0;
        m_variableCount = 0;

        std::string buffer =
            "global _start\n"
            "section .text\n"
            "_start:\n"
            "    push rbp\n"
            "    mov rbp, rsp\n";

        bool has_exit = false;

        // Generate every statement in the program.
        // Statements can contain nested scopes, so generate_statement()
        // handles recursion.
        for (auto statement : m_root.statements)
        {
            if (has_exit)
                break;

            generate_statement(statement, buffer, has_exit);
        }

        // If the program never called exit(),
        // return 0 by default.
        if (!has_exit)
        {
            buffer +=
                "    mov rdi, 0\n"
                "    mov rax, 60\n"
                "    syscall\n";
            return buffer;
        }

        return buffer;
    }

private:
    // Generates temporary values.
    //
    // Example:
    //
    // push rbx
    //
    // This temporarily stores a value while another
    // expression is being calculated.
    void push_temp(std::string &buffer, const std::string &value)
    {
        buffer += "    push " + value + "\n";
    }

    // Removes a temporary value from the stack.
    //
    // Example:
    //
    // pop rax
    //
    // This takes the top temporary value and places it
    // into the requested register.
    void pop_temp(std::string &buffer, const std::string &reg)
    {
        buffer += "    pop " + reg + "\n";
    }

    struct Var
    {
        // The position of the variable relative to rbp.
        //
        // Example:
        //
        // offset = 8
        //
        // means:
        //
        // [rbp - 8]
        //
        size_t offset;
    };

    void generate_if(NodeStmntIf *stmt,
                     std::string &buffer,
                     bool &has_exit)
    {
        std::string falseLabel = ".L" + std::to_string(m_labelCount++);
        std::string endLabel = ".L" + std::to_string(m_labelCount++);

        generate_condition(stmt->condition, buffer, falseLabel);

        bool if_exit = false;
        generate_scope(stmt->scope, buffer, if_exit);

        if (!if_exit)
            buffer += "    jmp " + endLabel + "\n";

        buffer += falseLabel + ":\n";

        bool else_exit = false;

        if (stmt->else_if)
        {
            generate_if(stmt->else_if, buffer, else_exit);
        }
        else if (stmt->else_scope)
        {
            generate_scope(stmt->else_scope, buffer, else_exit);
        }

        buffer += endLabel + ":\n";

        has_exit = if_exit && else_exit;
    }

    void generate_for(NodeStmntFor *stmt, std::string &buffer, bool &has_exit)
    {
        std::string startLabel = ".L" + std::to_string(m_labelCount++);
        std::string endLabel = ".L" + std::to_string(m_labelCount++);

        generate_statement(stmt->init, buffer, has_exit); // generate initialization once

        buffer += startLabel + ":\n"; // start of the loop

        generate_condition(stmt->condition, buffer, endLabel); // check condition, if false jump to endLabel

        bool body_exit = false;

        generate_scope(stmt->scope, buffer, body_exit); // generate the body of the loop { ... }

        if (!body_exit)
        {
            generate_statement(stmt->increment, buffer, has_exit); // generate increment statement after the body of the loop
            // If the body did not exit the program, go back and check the condition again.
            // this is like c++ when it checks the condition of the loop after executing the body of the loop.
            buffer += "    jmp " + startLabel + "\n";
        }
        buffer += endLabel + ":\n";
        has_exit = body_exit;
        /*
        The for loop is generated as follows:
        1. Generate the initialization statement once before the loop starts. (generate_statement)
        2. Generate the condition check at the start of each iteration. (generate_condition)
        3. Generate the body of the loop. (generate_scope)
        4. Generate the increment statement after the body of the loop. (generate_statement) its a statement because we declared it as a statement in the AST.
        5. If the body did not exit the program, go back and check the condition again. (buffer += "    jmp " + startLabel + "\n";) go back to start_label to check the condition again.
        6. If the condition is false, jump to the end label and exit the loop. (generate_condition will jump to endLabel if the condition is false)
        7. The end label marks the end of the loop.
        This is similar to how a for loop works in C++.
        */
    }

    void generate_whilst(NodeStmntWhile *stmt,
                         std::string &buffer,
                         bool &has_exit)
    {
        std::string startLabel =
            ".L" + std::to_string(m_labelCount++);

        std::string endLabel =
            ".L" + std::to_string(m_labelCount++);

        // Start of the loop.
        buffer += startLabel + ":\n";

        // Check condition.
        //
        // If condition is false,
        // jump to endLabel.
        generate_condition(
            stmt->condition,
            buffer,
            endLabel);

        // Generate the body.
        bool body_exit = false;

        generate_scope(
            stmt->scope,
            buffer,
            body_exit);

        // If the body did not exit the program,
        // go back and check the condition again.
        if (!body_exit)
        {
            buffer += "    jmp " + startLabel + "\n";
        }

        // Loop ends here.
        buffer += endLabel + ":\n";

        has_exit = body_exit;
    }

    // Generates a single statement.
    //
    // Statements can be:
    //
    // set x = expression;
    // exit(expression);
    // { scope }
    //
    // A scope can contain more statements,
    // so this function calls itself recursively.
    void generate_statement(NodeStmnt *statement,
                            std::string &buffer,
                            bool &has_exit)
    {
        std::visit([&](auto &&stmt)
                   {
            using T = std::decay_t<decltype(stmt)>; // stmt accepts all types of data.


            // exit(expression);
            if constexpr (std::is_same_v<T, NodeStmntExit>) {
                has_exit = true;

                generate_expr(stmt.expr, buffer);

                buffer +=
                        "    mov rdi, rbx\n"
                        "    mov rax, 60\n"
                        "    syscall\n";
            }


            // set variable = expression;
            else if constexpr (std::is_same_v<T, NodeStmntLet>) {
                std::string name =
                        stmt.identifier;


                // Generate the expression first.
                // The result will be placed inside rbx.
                generate_expr(stmt.expr, buffer);


                // Reserve 8 bytes on the stack
                // for this variable.
                buffer += "    sub rsp, 8\n";


                // Store the variable at a fixed location.
                //
                // Example:
                //
                // [rbp - 8] = a
                //
                // [rbp - 16] = b
                //
                buffer += "    mov [rbp - ";
                buffer += std::to_string((m_variableCount + 1) * 8);
                buffer += "], rbx\n";


                m_variableCount++;


                // Remember where this variable lives.
                m_scopes.back()[name] =
                        Var{m_variableCount * 8};
            } else if constexpr (std::is_same_v<T, NodeStmntIf *>) {
                generate_if(stmt, buffer, has_exit);
            } else if constexpr (std::is_same_v<T, NodeStmntWhile *>) {
                generate_whilst(stmt, buffer, has_exit);
            }
            else if constexpr (std::is_same_v<T, NodeStmntFor *>) {
                generate_for(stmt, buffer, has_exit);
            }
             else if constexpr (std::is_same_v<T, NodeStmntAssign>) {
                std::string name = stmt.identifier;

                generate_expr(stmt.expr, buffer);
                Var var = lookup(name);

                buffer += "    mov [rbp - ";
                buffer += std::to_string(var.offset);
                buffer += "], rbx\n";
            } else if constexpr (std::is_same_v<T, NodeScope *>) {
                generate_scope(stmt, buffer, has_exit);
            } }, statement->stmnt);
    }

    void generate_scope(NodeScope *scope,
                        std::string &buffer,
                        bool &has_exit)
    {
        size_t oldVariableCount = m_variableCount;

        m_scopes.emplace_back();

        for (auto child : scope->statements)
        {
            generate_statement(child, buffer, has_exit);

            if (has_exit)
                break;
        }

        if (!has_exit)
        {
            size_t variablesCreated =
                m_variableCount - oldVariableCount;

            if (variablesCreated > 0)
            {
                buffer += "    add rsp, ";
                buffer += std::to_string(variablesCreated * 8);
                buffer += "\n";

                m_variableCount = oldVariableCount;
            }
        }

        m_scopes.pop_back();
    }

    void generate_condition_branch(NodeExpr *condition,
                                   std::string &buffer,
                                   const std::string &trueLabel,
                                   const std::string &falseLabel)
    {
        if (std::holds_alternative<UnaryExpr *>(condition->expr))
        {
            auto unary = std::get<UnaryExpr *>(condition->expr);

            if (unary->op == "!")
            {
                generate_condition_branch(
                    unary->expr,
                    buffer,
                    falseLabel,
                    trueLabel
                    // as i see i swapped true label and false label because this is how Logical NOT works.
                );

                return;
            }
        }
        if (!std::holds_alternative<BinaryExpr *>(condition->expr))
        {
            generate_expr(condition, buffer);

            buffer += "    cmp rbx, 0\n";
            buffer += "    je " + falseLabel + "\n";
            buffer += "    jmp " + trueLabel + "\n";
            return;
        }

        auto binary = std::get<BinaryExpr *>(condition->expr);

        // true && true
        if (binary->op == "&&")
        {
            std::string next =
                ".L" + std::to_string(m_labelCount++);

            generate_condition_branch(binary->left,
                                      buffer,
                                      next,
                                      falseLabel);

            buffer += next + ":\n";

            generate_condition_branch(binary->right,
                                      buffer,
                                      trueLabel,
                                      falseLabel);

            return;
        }

        // true || true
        if (binary->op == "||")
        {
            std::string next =
                ".L" + std::to_string(m_labelCount++);

            generate_condition_branch(binary->left,
                                      buffer,
                                      trueLabel,
                                      next);

            buffer += next + ":\n";

            generate_condition_branch(binary->right,
                                      buffer,
                                      trueLabel,
                                      falseLabel);

            return;
        }

        // comparison
        generate_expr(binary->left, buffer);

        push_temp(buffer, "rbx");

        generate_expr(binary->right, buffer);

        pop_temp(buffer, "rax");

        buffer += "    cmp rax, rbx\n";

        if (binary->op == "==")
        {
            buffer += "    je " + trueLabel + "\n";
            buffer += "    jmp " + falseLabel + "\n";
        }
        else if (binary->op == "!=")
        {
            buffer += "    jne " + trueLabel + "\n";
            buffer += "    jmp " + falseLabel + "\n";
        }
        else if (binary->op == "<")
        {
            buffer += "    jl " + trueLabel + "\n";
            buffer += "    jmp " + falseLabel + "\n";
        }
        else if (binary->op == "<=")
        {
            buffer += "    jle " + trueLabel + "\n";
            buffer += "    jmp " + falseLabel + "\n";
        }
        else if (binary->op == ">")
        {
            buffer += "    jg " + trueLabel + "\n";
            buffer += "    jmp " + falseLabel + "\n";
        }
        else if (binary->op == ">=")
        {
            buffer += "    jge " + trueLabel + "\n";
            buffer += "    jmp " + falseLabel + "\n";
        }
        else
        {
            throw std::runtime_error(
                "Unknown condition operator: " + binary->op);
        }
    }

    void generate_condition(NodeExpr *condition,
                            std::string &buffer,
                            const std::string &falseLabel)
    {
        std::string trueLabel =
            ".L" + std::to_string(m_labelCount++);

        generate_condition_branch(condition,
                                  buffer,
                                  trueLabel,
                                  falseLabel);

        buffer += trueLabel + ":\n";
    }

    void generate_expr(NodeExpr *expr,
                       std::string &buffer)
    {
        std::visit([&](auto &&node)
                   {
            using T = std::decay_t<decltype(node)>;

            // Integer literal
            if constexpr (std::is_same_v<T, NodeExprIntLit>) {
                buffer += "    mov rbx, ";
                buffer += node.value;
                buffer += "\n";
            }

            // Identifier
            else if constexpr (std::is_same_v<T, NodeExprIdentifier>) {
                std::string name = node.value;

                Var var = lookup(name);

                buffer += "    mov rbx, [rbp - ";
                buffer += std::to_string(var.offset);
                buffer += "]\n";
            }

            // Binary expression
            else if constexpr (std::is_same_v<T, BinaryExpr *>) {
                if (node->op == "<" ||
                    node->op == ">" ||
                    node->op == "<=" ||
                    node->op == ">=" ||
                    node->op == "==" ||
                    node->op == "!=" ||
                    node->op == "&&" ||
                    node->op == "||") {
                    throw std::runtime_error(
                        "Condition used as expression"
                    );
                }

                generate_expr(node->left, buffer);

                push_temp(buffer, "rbx");

                generate_expr(node->right, buffer);

                pop_temp(buffer, "rax");

                if (node->op == "+") {
                    buffer += "    add rax, rbx\n";
                } else if (node->op == "-") {
                    buffer += "    sub rax, rbx\n";
                } else if (node->op == "*") {
                    buffer += "    imul rax, rbx\n";
                } else if (node->op == "/") {
                    buffer += "    cqo\n";
                    buffer += "    idiv rbx\n";
                } else if (node->op == "%") {
                    buffer += "    cqo\n";
                    buffer += "    idiv rbx\n";
                    buffer += "    mov rax, rdx\n";
                } else {
                    throw std::runtime_error(
                        "Unknown binary operator: " + node->op
                    );
                }

                buffer += "    mov rbx, rax\n";
            }

            // Unary expression
            else if constexpr (std::is_same_v<T, UnaryExpr *>) {
                generate_expr(node->expr, buffer);

                if (node->op == "-") {
                    buffer += "    neg rbx\n";
                } else if (node->op == "!") {
                    buffer += "    cmp rbx, 0\n";
                    buffer += "    sete al\n";
                    buffer += "    movzx rbx, al\n";
                } else {
                    throw std::runtime_error(
                        "Unknown unary operator: " + node->op
                    );
                }
            } }, expr->expr);
    }

    // Finds a variable by searching from the newest scope
    // to the oldest scope.
    //
    // This allows:
    //
    // {
    //      set a = 20;
    // }
    //
    // to temporarily hide an outer variable named a.
    Var lookup(const std::string &name)
    {
        for (auto it = m_scopes.rbegin();
             it != m_scopes.rend();
             ++it)
        {
            auto found = it->find(name);

            if (found != it->end())
                return found->second;
        }

        throw std::runtime_error(
            "Unknown variable: " + name);
    }

    NodeProgram m_root;

    // Stores variables for each scope.
    std::vector<std::unordered_map<std::string, Var>> m_scopes;

    // Counts how many variables exist.
    size_t m_variableCount = 0;
    size_t m_labelCount = 0;
};
