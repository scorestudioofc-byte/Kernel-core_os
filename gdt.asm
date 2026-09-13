[BITS 64]
global gdt_flush
global tss_flush

gdt_flush:
    lgdt [rdi]
    mov ax, dx
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    push rsi                ; Empurra o seletor 0x08 (Code)
    lea rax, [rel .reload_cs]
    push rax                ; Empurra o ponteiro do rótulo
    retfq                   ; Far return força a recarga do CS em 64-bit

.reload_cs:
    ret

tss_flush:
    ltr di                  ; Carrega o seletor de TSS 0x28
    ret


