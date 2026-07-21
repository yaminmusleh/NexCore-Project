#pragma once

#include <string>
#include <variant>
#include <vector>
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

        bool has_exit = false;

        // Linux exit syscall number.
        buffer += "    mov rax, 60\n";


        // Go through every statement in the program.
        // The program contains a list of statements:
        //
        // set x = 7;
        // exit(x);
        //
        // Each statement is stored inside NodeStmnt's variant,
        // so we use std::visit to find out what type it is.

        for (const auto* statement: m_root.statements) {
            if (has_exit) break;
            std::visit([&](auto &&stmt) {
                using T = std::decay_t<decltype(stmt)>;


                // Handles:
                // exit(expression);
                if constexpr (std::is_same_v<T, NodeStmntExit>) {
                    has_exit = true;


                    std::visit([&](auto &&expr) {
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
                            buffer += "    mov rbx, ";
                            buffer += expr.int_lit.value.value();
                            buffer += "\n";

                            //clarifying this code:
                            //expr.int_lit means get the token
                            //expr.int_lit.value means get the optional<string> value{}; inside the Token in tokenization.h
                            //expr.int_lit.value.value() extracts the actual value from the optional<string> value{} which is integer value
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


                            Var var = m_variables[name];
                            size_t offset = m_stackSize - var.stackSlot - 1;

                            if (offset == 0) {
                                buffer += "    mov rbx, [rsp]\n";
                            } else {
                                buffer += "    mov rbx, [rsp + ";
                                buffer += std::to_string(offset * 8);
                                buffer += "]\n";
                            }
                        }
                    }, stmt.expr->expr);


                    // Push the value stored in rbx onto the stack.
                    push(buffer, "rbx");


                    // Pop the value into rdi because exit syscall expects it there.
                    pop(buffer, "rdi");
                }


                // Handles:
                // set x = expression;
                else if constexpr (std::is_same_v<T, NodeStmntLet>) {
                    std::string name =
                            stmt.identifier.value.value();


                    if (m_variables.find(name) != m_variables.end()) {
                        throw std::runtime_error(
                            "Variable already declared: " + name
                        );
                    }
                    std::visit([&](auto &&expr) {
                        using ExprType =
                                std::decay_t<decltype(expr)>;


                        // Handles:
                        // set x = 7;

                        if constexpr (std::is_same_v<ExprType, NodeExprIntLit>) {
                            push(buffer, expr.int_lit.value.value());

                            m_variables[name] = Var{m_stackSize - 1};
                        }


                        // Handles:
                        // set x = y;

                        else if constexpr (std::is_same_v<ExprType, NodeExprIdentifier>) {
                            std::string other =
                                    expr.identifier.value.value();


                            if (m_variables.find(other) == m_variables.end()) {
                                throw std::runtime_error(
                                    "Unknown variable: " + other
                                );
                            }


                            Var var = m_variables[other];
                            size_t offset = m_stackSize - var.stackSlot - 1;

                            buffer += "    mov rbx, [rsp + ";
                            buffer += std::to_string(offset * 8);
                            buffer += "]\n";

                            push(buffer, "rbx");

                            m_variables[name] = Var{m_stackSize - 1};
                        }
                    }, stmt.expr->expr);
                }
            }, statement->stmnt);
        }

        if (!has_exit) {
            buffer += "    mov rdi, 0\n";
        }

        // Execute the syscall.
        buffer += "    syscall\n";


        return buffer;
    }


    // Generates a pop instruction.
    //
    // Example:
    //
    // pop rdi
    //
    // This removes the top value from the stack
    // and places it inside the given register.


private:
    void push(std::string &buffer, const std::string &value) {
        buffer += "    push " + value + "\n";
        ++m_stackSize;
    }

    void pop(std::string &buffer, const std::string &reg) {
        if (m_stackSize == 0)
            throw std::runtime_error("Stack underflow.");

        buffer += "    pop " + reg + "\n";
        --m_stackSize;
    }

    struct Var {
        size_t stackSlot;
    };

    NodeProgram m_root;
    std::unordered_map<std::string, Var> m_variables;
    size_t m_stackSize = 0;
};
