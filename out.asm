global _start
section .text
_start:
    push rbp
    mov rbp, rsp
    mov rbx, 4
    mov rdi, rbx
    mov rax, 60
    syscall
