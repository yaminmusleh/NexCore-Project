global _start
section .text
_start:
    push rbp
    mov rbp, rsp
    mov rbx, 3
    mov [rbp - 8], rbx
    mov rdi, 0
    mov rax, 60
    syscall

section .rodata
