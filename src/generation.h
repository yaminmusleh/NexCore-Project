#pragma once


class Generator {
public:
    inline explicit Generator(NodeExit root)
        : m_root(std::move(root)) {
    }

    [[nodiscard]] std::string generate() const {
        std::string buffer =
                "global _start\n"
                "section .text\n"
                "_start:\n";

        buffer += "    mov rax, 60\n";
        buffer += "    mov rdi, ";
        buffer += m_root.expr.int_lit.value.value();
        // we make the m_root reach the expr from parser then grab its int_lit and take its value.
        buffer += "\n";
        buffer += "    syscall\n";

        return buffer;
    }

private:
    const NodeExit& m_root;
};
