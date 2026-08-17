global _start
section .text
_start:
    push rbp
    mov rbp, rsp
    sub rsp, 8
    mov [rbp - 8], rbx
    mov rbx, [rbp - 8]
    mov rdi, rbx
    mov rax, 60
    syscall
