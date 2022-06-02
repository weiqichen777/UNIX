section .text
global _SysCall : function
global _sigret_rt : function
global _sys_rt_sigprocmask : function

global setjmp : function 
global longjmp : function 
global exit : function

exit:
    mov rax, 60
    syscall

_SysCall:
    mov rax, rdi
    mov rdi, rsi
    mov rsi, rdx
    mov rdx, rcx
    mov r10, r8
    mov r8, r9
    mov r9, [rsp + 8]
    syscall
    ret

_sigret_rt:
    mov rax, 15
    syscall

_sys_rt_sigprocmask:
    push r10
	mov	r10, rcx
    mov rax, 13
    syscall
    pop r10
    ret

setjmp:
    mov qword [rdi + 0], rbx
    mov qword [rdi + 8], rsp
    mov qword [rdi + 16], rbp
    mov qword [rdi + 24], r12
    mov qword [rdi + 32], r13
    mov qword [rdi + 40], r14
    mov qword [rdi + 48], r15
    mov rax, qword [rsp] 
    mov qword [rdi + 56], rax

    push rdi
    push rsi
    push rdx
    push rcx

    mov rdi, 0
    mov rsi, 0
    lea rdx, [rdi + 64] 
    mov rcx, 8
    call _sys_rt_sigprocmask
    
    pop rcx     
    pop rdx
    pop rsi
    pop rdi
    mov rax, 0
    ret

longjmp:
    mov rbx, qword [rdi + 0]
    mov rsp, qword [rdi + 8]
    mov rbp, qword [rdi + 16]
    mov r12, qword [rdi + 24]
    mov r13, qword [rdi + 32]
    mov r14, qword [rdi + 40]
    mov r15, qword [rdi + 48]
    mov rax, qword [rdi + 56]
    mov qword [rsp], rax

    push rdi
    push rsi
    push rdx
    push rcx

    lea rsi, [rdi + 64] 
    mov rdi, 2
    mov rdx, 0
    mov rcx, 8
    call _sys_rt_sigprocmask

    pop rcx     
    pop rdx
    pop rsi
    pop rdi
    mov rax, rsi
    ret