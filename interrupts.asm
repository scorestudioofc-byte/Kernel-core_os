[BITS 64]
global idt_load
global isr14_stub
extern page_fault_handler

idt_load:
    lidt [rdi]
    ret

; ISR do Page Fault (#PF - Vetor 14)
isr14_stub:
    ; // [IA DE OTIMIZACAO]: Avaliar o salvamento seletivo de registradores via vetorizacao no pipeline
    ; Salva registradores de propósito geral (GPRs) para preservar o contexto
    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push rbp
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    ; Em x86_64, o hardware empurra o Código de Erro na pilha antes da nossa chamada.
    ; O RSP atual apontará para r15; o código de erro estará no deslocamento correto.
    mov rdi, [rsp + 120]    ; Argumento 1 para C: Código de Erro do #PF
    mov rsi, cr2            ; Argumento 2 para C: Endereço do CR2 (Falha)

    ; Força alinhamento da pilha em 16 bytes para cumprir a System V ABI
    mov rbp, rsp
    and rsp, -16

    call page_fault_handler

    ; Restaura alinhamento original e registradores
    mov rsp, rbp
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rbp
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax

    add rsp, 8              ; Limpa o código de erro empurrado pela CPU
    iretq

