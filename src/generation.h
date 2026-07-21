#pragma once

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
        m_variables.clear();
        m_stackSize = 0;

        std::string buffer =
                "global _start\n"
                "section .text\n"
                "_start:\n";


        bool has_exit = false;


        // Linux exit syscall
        buffer += "    mov rax, 60\n";


        for (const auto *statement: m_root.statements) {
            if (has_exit)
                break;


            std::visit([&](auto &&stmt) {
                using T = std::decay_t<decltype(stmt)>;


                // exit(expression);
                if constexpr (std::is_same_v<T, NodeStmntExit>) {
                    has_exit = true;


                    generate_expr(stmt.expr, buffer);


                    // exit syscall expects value in rdi
                    buffer += "    mov rdi, rbx\n";
                }


                // set x = expression;
                else if constexpr (std::is_same_v<T, NodeStmntLet>) {
                    std::string name =
                            stmt.identifier.value.value();


                    if (m_variables.find(name) != m_variables.end()) {
                        throw std::runtime_error(
                            "Variable already declared: " + name
                        );
                    }


                    // Evaluate the expression
                    // result comes back in rbx
                    generate_expr(stmt.expr, buffer);


                    // Store variable value
                    push(buffer, "rbx");


                    m_variables[name] = Var{
                        m_stackSize - 1
                    };
                }
            }, statement->stmnt);
        }


        if (!has_exit) {
            buffer += "    mov rdi, 0\n";
        }


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

    void generate_expr(NodeExpr *expr, std::string &buffer) {
        std::visit([&](auto &&node) {
            using T = std::decay_t<decltype(node)>;

            // Integer literal
            if constexpr (std::is_same_v<T, NodeExprIntLit>) {
                buffer += "    mov rbx, ";
                buffer += node.int_lit.value.value();
                buffer += "\n";
            }

            // Identifier
            else if constexpr (std::is_same_v<T, NodeExprIdentifier>) {
                std::string name = node.identifier.value.value();

                if (m_variables.find(name) == m_variables.end())
                    throw std::runtime_error("Unknown variable: " + name);

                Var var = m_variables[name];
                size_t offset = m_stackSize - var.stackSlot - 1;

                if (offset == 0)
                    buffer += "    mov rbx, [rsp]\n";
                else {
                    buffer += "    mov rbx, [rsp + ";
                    buffer += std::to_string(offset * 8);
                    buffer += "]\n";
                }
            }

            // Binary expression
            else if constexpr (std::is_same_v<T, BinaryExpr *>) {
                generate_expr(node->left, buffer);
                push(buffer, "rbx");

                generate_expr(node->right, buffer);
                pop(buffer, "rax");

                if (node->op == "+")
                    buffer += "    add rax, rbx\n";
                else if (node->op == "-")
                    buffer += "    sub rax, rbx\n";
                else if (node->op == "*")
                    buffer += "    imul rax, rbx\n";
                else if (node->op == "/") {
                    buffer += "    cqo\n";
                    buffer += "    idiv rbx\n";
                } else
                    throw std::runtime_error("Unknown binary operator");

                buffer += "    mov rbx, rax\n";
            }
        }, expr->expr);
    }

    NodeProgram m_root;
    std::unordered_map<std::string, Var> m_variables;
    size_t m_stackSize = 0;
};
