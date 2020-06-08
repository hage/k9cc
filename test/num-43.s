.intel_syntax noprefix
.global main
main:
        push rbp
        mov rbp, rsp
        sub rsp, 0
        push 86
        push 2
        pop rsi
        pop rdi
        mov rax, rsp
        and rax, 31
        jnz .Lnoalign0000
        mov rax, rsp
        call foo
        jmp .Lend0000
.Lnoalign0000:
        sub rsp, 8
        mov rax, 0
        call foo
        add rsp, 8
.Lend0000:
        push rax
        pop rax
        mov rsp, rbp
        pop rbp
        ret
        mov rsp, rbp
        pop rbp
        ret
.global foo
foo:
        push rbp
        mov rbp, rsp
        sub rsp, 24
        mov rax, rbp
        sub rax, 16
        mov [rax], rsi
        mov rax, rbp
        sub rax, 8
        mov [rax], rdi
        mov rax, rbp
        sub rax, 24
        push rax
        mov rax, rbp
        sub rax, 8
        push rax
        pop rax
        mov rax, [rax]
        push rax
        pop rdi
        pop rax
        mov [rax], rdi
        push rdi
        mov rax, rbp
        sub rax, 24
        push rax
        pop rax
        mov rax, [rax]
        push rax
        mov rax, rbp
        sub rax, 16
        push rax
        pop rax
        mov rax, [rax]
        push rax
        pop rdi
        pop rax
        cqo
        idiv rdi
        push rax
        pop rax
        mov rsp, rbp
        pop rbp
        ret
        mov rsp, rbp
        pop rbp
        ret
