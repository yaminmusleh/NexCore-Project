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

        for (const auto& statement : m_root.statements) {

            std::visit([&](auto&& stmt) {

                using T = std::decay_t<decltype(stmt)>;


                // Handles:
                // exit(expression);
                if constexpr (std::is_same_v<T, NodeStmntExit>) {


                    // The value used by exit is first moved into rbx.
                    buffer += "    mov rbx, ";




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


                            buffer += m_variables[name];

                        }


                    }, stmt.expr.expr);



                    buffer += "\n";


                    // Push the value stored in rbx onto the stack.
                    buffer += "    push rbx\n";


                    // Pop the value into rdi because exit syscall expects it there.
                    pop(buffer, "rdi");

                }


                // Handles:
                // set x = expression;
                else if constexpr (std::is_same_v<T, NodeStmntLet>) {


                    std::string name =
                        stmt.identifier.value.value();


                    std::visit([&](auto&& expr) {

                        using ExprType =
                            std::decay_t<decltype(expr)>;


                        // Handles:
                        // set x = 7;

                        if constexpr (std::is_same_v<ExprType, NodeExprIntLit>) {

                            m_variables[name] =
                                expr.int_lit.value.value();

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


                            m_variables[name] =
                                m_variables[other];

                        }


                    }, stmt.expr.expr);

                }


            }, statement.stmnt);

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

    void pop(std::string& buffer, const std::string& reg) {

        buffer += "    pop " + reg + "\n";

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


    // Stores values that are temporarily pushed.
    //
    // Example:
    //
    // mov rbx, 7
    // push rbx
    //
    // The value 7 is now stored on the stack.

    std::vector<std::string> m_stack;

};