global _start
section .bss
    .L_print_buffer resb 32

section .text
_start:
    push rbp
    mov rbp, rsp
    lea rbx, [rel .L_string_0]
    mov [rbp - 8], rbx
    mov rbx, [rbp - 8]
    mov rsi, rbx
    xor rdx, rdx
.L_print_string_loop_0:
    cmp byte [rsi + rdx], 0
    je .L_print_string_done_1
    inc rdx
    jmp .L_print_string_loop_0
.L_print_string_done_1:
    mov rax, 1
    mov rdi, 1
    syscall
    mov rbx, 0
    mov rdi, rbx
    mov rax, 60
    syscall

section .rodata
.L_string_0 db 89, 97, 109, 101, 110, 10, 0
