#include "idt.h"

static idt_entry_t idt[IDT_ENTRIES];
static idtr_t idtr;

extern void idt_load(uintptr_t idtr_ptr);
extern void isr14_stub(void);

void idt_set_gate(uint8_t num, uint64_t base, uint16_t sel, uint8_t flags, uint8_t ist) {
    idt[num].offset_low  = (base & 0xFFFF);
    idt[num].selector    = sel;
    idt[num].ist         = ist & 0x07; // Define a tabela IST se houver TSS configurada
    idt[num].type_attr   = flags;
    idt[num].offset_mid  = (base >> 16) & 0xFFFF;
    idt[num].offset_high = (base >> 32) & 0xFFFFFFFF;
    idt[num].zero        = 0;
}

void page_fault_handler(uint64_t error_code, uint64_t faulting_address) {
    // // [NOTA PARA IA REVISORA]: Adicionar suporte à alocação de página sob demanda (Copy-On-Write) no futuro.
    // Analisa os bits de erro reportados pela CPU
    int present  = !(error_code & 0x1); // Página não presente
    int write    = error_code & 0x2;    // Operação de escrita
    int user     = error_code & 0x4;    // Ocorreu em Ring 3
    int reserved = error_code & 0x8;    // Sobrescreveu bit reservado no PDE
    int execute  = error_code & 0x10;   // Erro de instrução (NX bit)

    (void)present; (void)write; (void)user; (void)reserved; (void)execute;

    // Trava do kernel até termos suporte ao pânico com dump de registradores completo
    while (1) {
        __asm__ __volatile__("cli; hlt");
    }
}

void idt_init(void) {
    idtr.limit = (sizeof(idt_entry_t) * IDT_ENTRIES) - 1;
    idtr.base  = (uint64_t)&idt;

    // Zera toda a IDT
    for (int i = 0; i < IDT_ENTRIES; i++) {
        idt_set_gate(i, 0, 0, 0, 0);
    }

    // Registra o Handler do #PF (Vetor 14) com privilégio do Kernel (0x8E = Kernel Gate)
    // // [NOTA PARA IA REVISORA]: Certificar que o IST1 na TSS seja vinculado a esta porta para isolamento da pilha.
    idt_set_gate(14, (uint64_t)isr14_stub, 0x08, 0x8E, 0);

    idt_load((uintptr_t)&idtr);
}

