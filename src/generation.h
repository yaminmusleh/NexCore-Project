#pragma once

#include <iostream>
#include <string>
#include <variant>
#include <vector>
#include <unordered_map>
#include <stdexcept>
#include <type_traits>
#include <utility>


class Generator {
public:

    inline explicit Generator(NodeProgram root)
        : m_root(std::move(root)) {
    }


    [[nodiscard]] std::string generate() {

        m_scopes.clear();
        m_scopes.emplace_back();

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
        for (auto statement : m_root.statements) {

            if (has_exit)
                break;

            generate_statement(statement, buffer, has_exit);
        }


        // If the program never called exit(),
        // return 0 by default.
        if (!has_exit) {
            buffer += "    mov rdi, 0\n";
        }


        buffer +=
                "    mov rax, 60\n"
                "    syscall\n";


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
    void push_temp(std::string &buffer, const std::string &value) {

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
    void pop_temp(std::string &buffer, const std::string &reg) {

        buffer += "    pop " + reg + "\n";
    }



    struct Var {

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
                            bool &has_exit) {


        std::visit([&](auto &&stmt) {

            using T = std::decay_t<decltype(stmt)>;



            // exit(expression);
            if constexpr (std::is_same_v<T, NodeStmntExit>) {


                has_exit = true;


                generate_expr(stmt.expr, buffer);


                // Linux exit syscall expects the return value in rdi.
                buffer += "    mov rdi, rbx\n";
            }



            // set variable = expression;
            else if constexpr (std::is_same_v<T, NodeStmntLet>) {


                std::string name =
                        stmt.identifier.value.value();



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
            }



            // A scope creates a new variable area.
            //
            // Example:
            //
            // {
            //      set x = 10;
            // }
            //
            // Variables inside the scope should not
            // be visible outside.
            else if constexpr (std::is_same_v<T, NodeScope*>) {



                m_scopes.emplace_back();



                for (auto child : stmt->statements) {


                    if (has_exit)
                        break;


                    generate_statement(child, buffer, has_exit);
                }



                m_scopes.pop_back();
            }



        }, statement->stmnt);
    }





    // Generates expressions.
    //
    // Supported:
    //
    // 10
    // x
    // a + b
    // a * b
    // a / b
    //
    void generate_expr(NodeExpr *expr,
                       std::string &buffer) {



        std::visit([&](auto &&node) {


            using T = std::decay_t<decltype(node)>;



            // Integer literal.
            //
            // Example:
            //
            // 123
            //
            // becomes:
            //
            // mov rbx, 123
            if constexpr (std::is_same_v<T, NodeExprIntLit>) {


                buffer += "    mov rbx, ";
                buffer += node.int_lit.value.value();
                buffer += "\n";
            }



            // Identifier.
            //
            // Example:
            //
            // x
            //
            // loads the stored value from memory.
            else if constexpr (std::is_same_v<T, NodeExprIdentifier>) {


                std::string name =
                        node.identifier.value.value();



                Var var = lookup(name);



                buffer += "    mov rbx, [rbp - ";
                buffer += std::to_string(var.offset);
                buffer += "]\n";
            }



            // Binary expression.
            //
            // Example:
            //
            // a + b
            //
            // Calculates:
            //
            // left operator right
            else if constexpr (std::is_same_v<T, BinaryExpr*>) {



                generate_expr(node->left, buffer);


                // Save left side temporarily.
                push_temp(buffer, "rbx");



                generate_expr(node->right, buffer);



                // Restore left side into rax.
                pop_temp(buffer, "rax");



                if (node->op == "+") {

                    buffer += "    add rax, rbx\n";
                }

                else if (node->op == "-") {

                    buffer += "    sub rax, rbx\n";
                }

                else if (node->op == "*") {

                    buffer += "    imul rax, rbx\n";
                }

                else if (node->op == "/") {


                    buffer += "    cqo\n";
                    buffer += "    idiv rbx\n";
                }

                else {

                    throw std::runtime_error(
                            "Unknown binary operator"
                    );
                }



                // The final result is always kept in rbx.
                buffer += "    mov rbx, rax\n";
            }



        }, expr->expr);
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
    Var lookup(const std::string &name) {


        for (auto it = m_scopes.rbegin();
             it != m_scopes.rend();
             ++it) {



            auto found = it->find(name);



            if (found != it->end())
                return found->second;
        }



        throw std::runtime_error(
                "Unknown variable: " + name
        );
    }



    NodeProgram m_root;



    // Stores variables for each scope.
    std::vector<std::unordered_map<std::string, Var>> m_scopes;



    // Counts how many variables exist.
    size_t m_variableCount = 0;
};