#pragma once

#include <string>
#include <variant>
#include <unordered_map>
#include <stdexcept>

class Generator {
public:
    inline explicit Generator(NodeProgram root)
        : m_root(std::move(root)) {
    }

    [[nodiscard]] std::string generate() {

        std::string buffer =
                "global _start\n"
                "section .text\n"
                "_start:\n";


        buffer += "    mov rax, 60\n";


        // Go through every statement in the program.
        // The program contains a list of statements:
        //
        // set x = 7;
        // exit(x);
        //
        // Each statement is stored inside NodeStmnt's variant,
        // so we use std::visit to find out what type it is.

        for (const auto& statement : m_root.statements) {

            std::visit([&](auto&& stmt) {

                using T = std::decay_t<decltype(stmt)>;


                // Handles:
                // exit(expression);
                if constexpr (std::is_same_v<T, NodeStmntExit>) {


                    buffer += "    mov rdi, ";


                    std::visit([&](auto&& expr) {

                        using ExprType =
                            std::decay_t<decltype(expr)>;
                        /*
                        is used in modern C++ to extract the "plain",
                        clean underlying value type of expression. It strips away all reference wrappers,
                        constant modifiers (const), and volatile modifiers (volatile),
                        while also converting arrays and functions into pointers
                         */


                        // Handles:
                        // exit(7); the value passed into exit() is dealt with here:

                        if constexpr (std::is_same_v<ExprType, NodeExprIntLit>) {

                            buffer += expr.int_lit.value.value();

                        }


                        // Handles:
                        // exit(x);

                        else if constexpr (std::is_same_v<ExprType, NodeExprIdentifier>) {


                            std::string name =
                                expr.identifier.value.value();


                            if (m_variables.find(name) == m_variables.end()) {
                                throw std::runtime_error(
                                    "Unknown variable: " + name
                                );
                            }


                            buffer += m_variables[name];
                        }


                    }, stmt.expr.expr);


                    buffer += "\n";
                }


                // Handles:
                // set x = expression;
                else if constexpr (std::is_same_v<T, NodeStmntLet>) {


                    std::string name =
                        stmt.identifier.value.value();


                    std::visit([&](auto&& expr) {

                        using ExprType =
                            std::decay_t<decltype(expr)>;


                        if constexpr (std::is_same_v<ExprType, NodeExprIntLit>) {

                            m_variables[name] =
                                expr.int_lit.value.value();

                        }


                        else if constexpr (std::is_same_v<ExprType, NodeExprIdentifier>) {

                            std::string other =
                                expr.identifier.value.value();


                            if (m_variables.find(other) == m_variables.end()) {
                                throw std::runtime_error(
                                    "Unknown variable: " + other
                                );
                            }


                            m_variables[name] =
                                m_variables[other];
                        }


                    }, stmt.expr.expr);
                }


            }, statement.stmnt);
        }


        buffer += "    syscall\n";


        return buffer;
    }


private:

    NodeProgram m_root;


    // Stores variables:
    //
    // x -> 7
    //
    // so when we see:
    //
    // exit(x);
    //
    // we can replace x with 7.

    std::unordered_map<std::string, std::string> m_variables;
};