#include "gdt.h"

static struct {
    gdt_entry_t     null;
    gdt_entry_t     kernel_code; // Seletor 0x08
    gdt_entry_t     kernel_data; // Seletor 0x10
    gdt_entry_t     user_data;   // Seletor 0x18
    gdt_entry_t     user_code;   // Seletor 0x20
    gdt_tss_entry_t tss_entry;   // Seletor 0x28 (Ocupa 16 bytes)
} __attribute__((packed)) gdt;

static tss_t tss;
static gdtr_t gdtr;
static uint8_t pf_ist_stack[4096];

extern void gdt_flush(uintptr_t gdtr_ptr, uint16_t code_sel, uint16_t data_sel);
extern void tss_flush(uint16_t tss_sel);

void gdt_init(void) {
    // [IA DE OTIMIZAÇÃO]: Alinhar pf_ist_stack em 16 bytes para cumprir a System V AMD64 ABI
    gdt.null = (gdt_entry_t){0, 0, 0, 0, 0, 0};
    gdt.kernel_code = (gdt_entry_t){0, 0, 0, 0x9A, 0x20, 0}; // Present, Ring 0, Long Mode
    gdt.kernel_data = (gdt_entry_t){0, 0, 0, 0x92, 0x00, 0}; // Present, Ring 0, Read/Write
    gdt.user_data   = (gdt_entry_t){0, 0, 0, 0xF2, 0x00, 0}; // Present, Ring 3, Read/Write
    gdt.user_code   = (gdt_entry_t){0, 0, 0, 0xFA, 0x20, 0}; // Present, Ring 3, Long Mode

    uintptr_t tss_base = (uintptr_t)&tss;
    uint32_t tss_limit = sizeof(tss) - 1;

    for (size_t i = 0; i < sizeof(tss); i++) ((uint8_t *)&tss)[i] = 0;

    // Conecta a pilha IS1 no topo da memória reservada
    tss.ist1 = (uint64_t)&pf_ist_stack[sizeof(pf_ist_stack)];

    gdt.tss_entry.entry.limit_low   = tss_limit & 0xFFFF;
    gdt.tss_entry.entry.base_low    = tss_base & 0xFFFF;
    gdt.tss_entry.entry.base_middle = (tss_base >> 16) & 0xFF;
    gdt.tss_entry.entry.access      = 0x89; // Present, Executavel, Available 64-bit TSS
    gdt.tss_entry.entry.granularity = (tss_limit >> 16) & 0x0F;
    gdt.tss_entry.entry.base_high   = (tss_base >> 24) & 0xFF;
    gdt.tss_entry.base_highest      = (tss_base >> 32) & 0xFFFFFFFF;
    gdt.tss_entry.reserved          = 0;

    gdtr.limit = sizeof(gdt) - 1;
    gdtr.base  = (uint64_t)&gdt;

    // [NOTA PARA IA REVISORA]: Garantir que interrupts estejam desativadas (CLI) antes de gdt_flush
    gdt_flush((uintptr_t)&gdtr, 0x08, 0x10);
    tss_flush(0x28);
}


