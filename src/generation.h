#pragma once

#include <iostream>
#include <string>
#include <variant>
#include <vector>
#include <unordered_map>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <cstdint>
#include <cstring>
#include <cstdio>

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
        m_strings.clear();
        m_stringCount = 0;

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
        }
        buffer +=
            "\nsection .rodata\n";

        for (const auto &str : m_strings)
        {
            buffer += str.label + " db ";

            for (unsigned char c : str.value)
            {
                buffer += std::to_string(c);
                buffer += ", ";
            }

            // Newline character
            buffer += "10, ";

            // Null terminator
            buffer += "0\n";
        }
        return buffer;
    }

private:
    void generate_print_integer(std::string &buffer)
    {
        std::string loop =
            ".L_print_loop_" + std::to_string(m_labelCount++);

        std::string done =
            ".L_print_done_" + std::to_string(m_labelCount++);

        std::string negative =
            ".L_print_negative_" + std::to_string(m_labelCount++);

        std::string positive =
            ".L_print_positive_" + std::to_string(m_labelCount++);

        /*
            rbx = integer to print

            Reserve 32 bytes on the stack for the
            temporary string buffer.
        */

        buffer += "    sub rsp, 32\n";

        // r12 points to the end of our buffer.
        buffer += "    lea r12, [rsp + 32]\n";

        // rax = integer
        buffer += "    mov rax, rbx\n";

        // Check whether the number is negative.
        buffer += "    cmp rax, 0\n";
        buffer += "    jl " + negative + "\n";

        // Positive / zero.
        buffer += "    jmp " + positive + "\n";

        // --------------------------------------------------
        // Negative
        // --------------------------------------------------
        buffer += negative + ":\n";

        // Remember that the number was negative.
        //
        // r13 = 1 means negative.
        buffer += "    mov r13, 1\n";

        // Convert the number to its absolute value.
        buffer += "    neg rax\n";

        buffer += "    jmp " + loop + "\n";

        // --------------------------------------------------
        // Positive
        // --------------------------------------------------
        buffer += positive + ":\n";

        // r13 = 0 means positive.
        buffer += "    xor r13, r13\n";

        // --------------------------------------------------
        // Convert digits
        // --------------------------------------------------
        buffer += loop + ":\n";

        buffer += "    xor rdx, rdx\n";
        buffer += "    mov rcx, 10\n";
        buffer += "    div rcx\n";

        buffer += "    add dl, '0'\n";

        // Since we're building backwards,
        // digits are inserted from right to left.
        buffer += "    dec r12\n";
        buffer += "    mov [r12], dl\n";

        buffer += "    test rax, rax\n";
        buffer += "    jnz " + loop + "\n";

        // --------------------------------------------------
        // Add '-' AFTER the digits
        // --------------------------------------------------
        buffer += "    cmp r13, 0\n";
        buffer += "    je " + done + "\n";

        buffer += "    dec r12\n";
        buffer += "    mov byte [r12], '-'\n";

        buffer += done + ":\n";

        // --------------------------------------------------
        // Write number
        // --------------------------------------------------
        buffer += "    mov rax, 1\n";
        buffer += "    mov rdi, 1\n";
        buffer += "    mov rsi, r12\n";

        buffer += "    lea rdx, [rsp + 32]\n";
        buffer += "    sub rdx, r12\n";

        buffer += "    syscall\n";

        // --------------------------------------------------
        // Newline
        // --------------------------------------------------
        buffer += "    mov byte [rsp], 10\n";

        buffer += "    mov rax, 1\n";
        buffer += "    mov rdi, 1\n";
        buffer += "    mov rsi, rsp\n";
        buffer += "    mov rdx, 1\n";

        buffer += "    syscall\n";

        // Free temporary buffer.
        buffer += "    add rsp, 32\n";
    }
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

    void pop_temp(std::string &buffer, const std::string &reg)
    {
        buffer += "    pop " + reg + "\n";
    }

    enum class ValueType
    {
        Int,
        Float,
        Double
    };

    struct Var
    {

        size_t offset;
        ValueType type;
    };

    uint32_t float_bits(const std::string &value)
    {
        float f = std::stof(value);

        uint32_t bits;
        std::memcpy(&bits, &f, sizeof(bits));

        return bits;
    }

    uint64_t double_bits(const std::string &value)
    {
        double d = std::stod(value);

        uint64_t bits;
        std::memcpy(&bits, &d, sizeof(bits));

        return bits;
    }

    void convert_value(
        ValueType from,
        ValueType to,
        std::string &buffer)
    {
        if (from == to)
            return;

        // int -> float
        if (from == ValueType::Int &&
            to == ValueType::Float)
        {
            buffer += "    cvtsi2ss xmm0, rbx\n";
            return;
        }

        // int -> double
        if (from == ValueType::Int &&
            to == ValueType::Double)
        {
            buffer += "    cvtsi2sd xmm0, rbx\n";
            return;
        }

        // float -> double
        if (from == ValueType::Float &&
            to == ValueType::Double)
        {
            buffer += "    cvtss2sd xmm0, xmm0\n";
            return;
        }

        // double -> float
        if (from == ValueType::Double &&
            to == ValueType::Float)
        {
            buffer += "    cvtsd2ss xmm0, xmm0\n";
            return;
        }

        throw std::runtime_error(
            "Unsupported numeric conversion");
    }

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
    void generate_for(NodeStmntFor *stmt,
                      std::string &buffer,
                      bool &has_exit)

    {
        std::string startLabel =
            ".L" + std::to_string(m_labelCount++);

        std::string endLabel =
            ".L" + std::to_string(m_labelCount++);

        // 1. Generate initialization once.
        bool init_exit = false;

        generate_statement(
            stmt->init,
            buffer,
            init_exit);

        if (init_exit)
        {
            has_exit = true;
            return;
        }

        // 2. Start of loop.
        buffer += startLabel + ":\n";

        // 3. Check condition.
        // If false, jump directly to endLabel.
        generate_condition(
            stmt->condition,
            buffer,
            endLabel);

        // 4. Generate body.
        bool body_exit = false;

        generate_scope(
            stmt->scope,
            buffer,
            body_exit);

        // If the body called exit(), the program is already terminated.
        if (body_exit)
        {
            has_exit = true;
            return;
        }

        // 5. Generate increment.
        bool increment_exit = false;

        generate_statement(
            stmt->increment,
            buffer,
            increment_exit);

        if (increment_exit)
        {
            has_exit = true;
            return;
        }

        // 6. Go back to the condition.
        buffer += "    jmp " + startLabel + "\n";

        // 7. Loop exit.
        buffer += endLabel + ":\n";

        // Reaching the end of a for loop does NOT mean
        // the whole program exited.
        has_exit = false;
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

    ValueType get_expr_type(NodeExpr *expr)
    {
        return std::visit([&](auto &&node) -> ValueType
                          {
        using T = std::decay_t<decltype(node)>;

        if constexpr (std::is_same_v<T, NodeExprIntLit>)
        {
            return ValueType::Int;
        }
        else if constexpr (std::is_same_v<T, NodeExprFloatLit>)
        {
            return ValueType::Float;
        }
        else if constexpr (std::is_same_v<T, NodeExprDoubleLit>)
        {
            return ValueType::Double;
        }
        else if constexpr (std::is_same_v<T, NodeExprIdentifier>)
        {
            return lookup(node.value).type;
        }
        else if constexpr (std::is_same_v<T, BinaryExpr *>)
        {
            ValueType left = get_expr_type(node->left);
            ValueType right = get_expr_type(node->right);

            if (left == ValueType::Double ||
                right == ValueType::Double)
            {
                return ValueType::Double;
            }

            if (left == ValueType::Float ||
                right == ValueType::Float)
            {
                return ValueType::Float;
            }

            return ValueType::Int;
        }
        else if constexpr (std::is_same_v<T, UnaryExpr *>)
        {
            return get_expr_type(node->expr);
        }
        else
        {
            throw std::runtime_error(
                "Expression does not have a numeric type");
        } },
                          expr->expr);
    }

    void generate_statement(NodeStmnt *statement,
                            std::string &buffer,
                            bool &has_exit)
    {
        std::visit([&](auto &&stmt)
                   {
                       using T = std::decay_t<decltype(stmt)>; // stmt accepts all types of data.

                       // exit(expression);
                       if constexpr (std::is_same_v<T, NodeStmntExit>)
                       {
                           has_exit = true;

                           generate_expr(stmt.expr, buffer);

                           buffer +=
                               "    mov rdi, rbx\n"
                               "    mov rax, 60\n"
                               "    syscall\n";
                       }

                       // set variable = expression;
                       else if constexpr (std::is_same_v<T, NodeStmntLet>)
                       {
                           std::string name = stmt.identifier;

                           ValueType type = get_expr_type(stmt.expr);
                           generate_expr(stmt.expr, buffer);

                           size_t offset = (m_variableCount + 1) * 8;

                           if (type == ValueType::Int)
                           {
                               buffer += "    mov [rbp - ";
                               buffer += std::to_string(offset);
                               buffer += "], rbx\n";
                           }
                           else if(type == ValueType::Double)
                           {
                               buffer += "    movsd [rbp - ";
                               buffer += std::to_string(offset);
                               buffer += "], xmm0\n";
                           }

                           m_variableCount++;

                           // Remember where this variable lives.
                           m_scopes.back()[name] =
                               Var{offset, type};
                       }


                       else if constexpr (std::is_same_v<T, NodeStmntIf *>)
                       {
                           generate_if(stmt, buffer, has_exit);
                       }
                       else if constexpr (std::is_same_v<T, NodeStmntWhile *>)
                       {
                           generate_whilst(stmt, buffer, has_exit);
                       }
                       else if constexpr (std::is_same_v<T, NodeStmntFor *>)
                       {
                           generate_for(stmt, buffer, has_exit);
                       }
                       else if constexpr (std::is_same_v<T, NodeStmntAssign>)
                       {
                           std::string name = stmt.identifier;
                       
                           Var var = lookup(name);
                       
                           ValueType type = get_expr_type(stmt.expr);
                       
                           if (type != var.type)
                           {
                               throw std::runtime_error(
                                   "Type mismatch in assignment to variable: " + name
                               );
                           }
                       
                           generate_expr(stmt.expr, buffer);
                       
                           if(var.type == ValueType::Int)
                           {
                            buffer += "    mov [rbp - ";
                            buffer += std::to_string(var.offset);
                            buffer += "], rbx\n";
                           }
                           else if (var.type == ValueType::Float)
                           {
                            buffer += "    movss [rbp - ";
                            buffer += std::to_string(var.offset);
                            buffer += "], xmm0\n";
                           }
                           else if (var.type == ValueType::Double)
                           {
                            buffer += "    movss [rbp - ";
                            buffer += std::to_string(var.offset);
                            buffer += "], xmm0\n";
                           }
                       }
                      else if constexpr (std::is_same_v<T, NodeStmntPrint>)                      
                      {
                          generate_expr(stmt.expr, buffer);
                      
                          // --------------------------------------------------                      
    // String
    // --------------------------------------------------
                          if (std::holds_alternative<NodeExprStringLit>(
                                  stmt.expr->expr))                      
                             {
                                 auto &string =
                                     std::get<NodeExprStringLit>(stmt.expr->expr);
                         
                                 buffer += "    mov rax, 1\n"; // sys_write
                                 buffer += "    mov rdi, 1\n"; // stdout
                                 buffer += "    mov rsi, rbx\n"; // string address
                                 buffer += "    mov rdx, ";
                                 buffer += std::to_string(string.value.size() + 1);
                                 buffer += "\n";
                                 buffer += "    syscall\n";
                             }
                      
                           // --------------------------------------------------
                          // Integer / expression
                          // --------------------------------------------------
                          else
                          {
                             generate_print_integer(buffer);
                          }
                    }
                       else if constexpr (std::is_same_v<T, NodeScope *>)
                       {
                           generate_scope(stmt, buffer, has_exit);
                       } },
                   statement->stmnt);
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

    //----------------------------------------------------//
    void generate_expr(NodeExpr *expr,
                       std::string &buffer)
    {
        std::visit([&](auto &&node)
                   {
                       using T = std::decay_t<decltype(node)>;

                       // Integer literal
                       if constexpr (std::is_same_v<T, NodeExprIntLit>)
                       {
                           buffer += "    mov rbx, ";
                           buffer += node.value;
                           buffer += "\n";
                       }

                       // Float literal
                       else if constexpr (std::is_same_v<T, NodeExprFloatLit>)
                       {
                           uint32_t bits = float_bits(node.value);

                           buffer += "    mov eax, 0x";

                           char hex[16];
                           std::snprintf(
                               hex,
                               sizeof(hex),
                               "%08x",
                               bits);
                           buffer += hex;
                           buffer += "\n";
                       }

                       // Double literal
                       else if constexpr (std::is_same_v<T, NodeExprDoubleLit>)
                       {
                           uint64_t bits = double_bits(node.value);
                           // is used to safely inspect the raw, underlying binary bits of a 64-bit floating-point number (double) without changing its numerical value.

                           buffer += "    mov rax, 0x";

                           char hex[32];
                           std::snprintf(
                               hex,
                               sizeof(hex),
                               "016llx",
                               static_cast<unsigned long long>(bits));

                           buffer += hex;
                           buffer += "\n";

                           buffer += "    movq xmm0, rax\n";
                       }

                       // Identifier
                       else if constexpr (std::is_same_v<T, NodeExprIdentifier>)
                       {
                           std::string name = node.value;

                           Var var = lookup(node.value);

                           // int
                           if (var.type == ValueType::Int)
                           {
                               buffer += "    mov rbx, [rbp - ";
                               buffer += std::to_string(var.offset);
                               buffer += "]\n";
                           }

                           // float
                           if (var.type == ValueType::Float)
                           {
                               buffer += "    movss xmm0, [rbp - ";
                               buffer += std::to_string(var.offset);
                               buffer += "]\n";
                           }

                           // double
                           if (var.type == ValueType::Int)
                           {
                               buffer += "    movsd xmm0, [rbp - ";
                               buffer += std::to_string(var.offset);
                               buffer += "]\n";
                           }
                       }

                       // String literal
                       else if constexpr (std::is_same_v<T, NodeExprStringLit>)
                       {

                           std::string label =
                               ".L_string_" + std::to_string(m_stringCount++);

                           m_strings.push_back({label, node.value});

                           buffer += "    lea rbx, [rel ";
                           buffer += label;
                           buffer += "]\n";
                       }

                       // Binary expression
                       else if constexpr (std::is_same_v<T, BinaryExpr *>)
                       {
                           if (node->op == "<" ||
                               node->op == ">" ||
                               node->op == "<=" ||
                               node->op == ">=" ||
                               node->op == "==" ||
                               node->op == "!=" ||
                               node->op == "&&" ||
                               node->op == "||")
                           {
                               throw std::runtime_error(
                                   "Condition used as expression");
                           }

                           ValueType leftType = get_expr_type(node->left);
                           ValueType rightType = get_expr_type(node->right);
                           ValueType resultType;

                           if (leftType == ValueType::Double ||
                               rightType == ValueType::Double)
                           {
                               resultType = ValueType::Double;
                           }
                           else if (leftType == ValueType::Float ||
                                    rightType == ValueType::Float)
                           {
                               resultType = ValueType::Float;
                           }
                           else
                           {
                               resultType = ValueType::Int;
                           }

                           // ==============================
                           // Integer Arithmetic
                           // ==============================

                           if (resultType == ValueType::Int)
                           {
                               generate_expr(node->left, buffer);
                               buffer += "    push rbx\n";

                               generate_expr(node->right, buffer);

                               buffer += "    pop rax\n";

                               if (node->op == "+")
                               {
                                   buffer += "    add rax, rbx\n";
                               }
                               else if (node->op == "-")
                               {
                                   buffer += "    sub rax, rbx\n";
                               }
                               else if (node->op == "*")
                               {
                                   buffer += "    imul rax, rbx\n";
                               }
                               else if (node->op == "/")
                               {
                                   buffer += "    cqo\n";
                                   buffer += "    idiv rax, rbx\n";
                               }
                               else if (node->op == "%")
                               {
                                   buffer += "    cqo\n";
                                   buffer += "    idiv rax, rbx\n";
                                   buffer += "    mov rax,rdx\n";
                               }
                               else
                               {
                                   throw std::runtime_error(
                                       "Uknown integer operator: " +
                                       node->op);
                               }

                               buffer += "    mov rbx,rax\n";
                               return;
                           }

                           // ===========================
                           // Floating-point arithmetic
                           // ===========================

                           // we will generate left operator
                           generate_expr(node->left, buffer);

                           // then convert left to the resultType:
                           convert_value(
                               leftType, resultType, buffer);

                           // we will save the left in xmm1 through the stack
                           buffer += "    sub rsp, 8\n";

                           if (resultType == ValueType::Float)
                           {
                               buffer +=
                                   "    movss [rsp], xmm0\n";
                           }
                           else
                           {
                               buffer +=
                                   "    movsd [rsp], xmm0\n";
                           }

                           // we will generate right operator
                           generate_expr(node->right, buffer);

                           // then convert right to the resultType:
                           convert_value(
                               rightType, resultType, buffer);

                           // then restore left into xmm1
                           if (resultType == ValueType::Float)
                           {
                               buffer +=
                                   "    movss xmm1, [rsp]\n";
                           }
                           else
                           {
                               buffer +=
                                   "    movsd xmm1, [rsp]\n";
                           }

                           buffer += "    add rsp, 8\n";

                           // ===================
                           // Float
                           // ===================

                           if (resultType == ValueType::Float)
                           {
                               if (node->op == "+")
                               {
                                   buffer +=
                                       "    addss xmm1, xmm0\n";
                               }

                               else if (node->op == "-")
                               {
                                   buffer +=
                                       "    subss xmm1, xmm0\n";
                               }
                               else if (node->op == "*")
                               {
                                   buffer +=
                                       "    mulss xmm1, xmm0\n";
                               }
                               else if (node->op == "/")
                               {
                                   buffer +=
                                       "    divss xmm1, xmm0\n";
                               }
                               else if (node->op == "%")
                               {
                                   throw std::runtime_error(
                                       "Modulo is not supported for floating point");
                               }
                               else
                               {
                                   throw std::runtime_error(
                                       "Unknown floating-point operator: " +
                                       node->op);
                               }

                               buffer += "    movaps xmm0, xmm1\n";
                           }

                           // =====================================================
                           // Double
                           // =====================================================

                           else if (resultType == ValueType::Double)
                           {
                               if (node->op == "+")
                               {
                                   buffer +=
                                       "    addsd xmm1, xmm0\n";
                               }
                               else if (node->op == "-")
                               {
                                   buffer +=
                                       "    subsd xmm1, xmm0\n";
                               }
                               else if (node->op == "*")
                               {
                                   buffer +=
                                       "    mulsd xmm1, xmm0\n";
                               }
                               else if (node->op == "/")
                               {
                                   buffer +=
                                       "    divsd xmm1, xmm0\n";
                               }
                               else if (node->op == "%")
                               {
                                   throw std::runtime_error(
                                       "Modulo is not supported for floating point");
                               }
                               else
                               {
                                   throw std::runtime_error(
                                       "Unknown floating-point operator: " +
                                       node->op);
                               }

                               buffer += "    movapd xmm0, xmm1\n";
                           }
                       }

                       // Unary expression
                       else if constexpr (std::is_same_v<T, UnaryExpr *>)
                       {
                           ValueType type =
                               get_expr_type(node->expr);

                           generate_expr(node->expr, buffer);

                           if (node->op == "-")
                           {
                               if (type == ValueType::Int)
                               {
                                   buffer +=
                                       "    neg rbx\n";
                               }
                               else if (type == ValueType::Float)
                               {
                                   buffer +=
                                       "    xorps xmm1, xmm1\n";
                                   buffer +=
                                       "    subss xmm1, xmm0\n";
                                   buffer +=
                                       "    movaps xmm0, xmm1\n";
                               }
                               else if (type == ValueType::Double)
                               {
                                   buffer +=
                                       "    xorpd xmm1, xmm1\n";
                                   buffer +=
                                       "    subsd xmm1, xmm0\n";
                                   buffer +=
                                       "    movapd xmm0, xmm1\n";
                               }
                           }
                           else if (node->op == "!")
                           {
                               if (type == ValueType::Int)
                               {
                                   buffer +=
                                       "    cmp rbx, 0\n";
                                   buffer +=
                                       "    sete al\n";
                                   buffer +=
                                       "    movzx rbx, al\n";
                               }
                               else if (type == ValueType::Float)
                               {
                                   buffer +=
                                       "    xorps xmm1, xmm1\n";
                                   buffer +=
                                       "    ucomiss xmm0, xmm1\n";
                                   buffer +=
                                       "    sete al\n";
                                   buffer +=
                                       "    movzx rbx, al\n";
                               }
                               else if (type == ValueType::Double)
                               {
                                   buffer +=
                                       "    xorpd xmm1, xmm1\n";
                                   buffer +=
                                       "    ucomisd xmm0, xmm1\n";
                                   buffer +=
                                       "    sete al\n";
                                   buffer +=
                                       "    movzx rbx, al\n";
                               }
                           }
                           else
                           {
                               throw std::runtime_error(
                                   "Unknown unary operator: " +
                                   node->op);
                           }
                       } },
                   expr->expr);
    }

    //----------------------------------------------------//
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

    struct StringLiteral
    {
        std::string label;
        std::string value;
    };

    std::vector<StringLiteral> m_strings;
    size_t m_stringCount = 0;
};
