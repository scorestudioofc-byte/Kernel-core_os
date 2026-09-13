#ifndef IDT_H
#define IDT_H

#include "types.h"

#define IDT_ENTRIES 256

typedef struct {
    uint16_t offset_low;    // Offset bits 0..15
    uint16_t selector;      // Seletor de segmento de código no GDT
    uint8_t  ist;           // Bits 0..2: IST index (TSS), restantes 0
    uint8_t  type_attr;     // Tipo e atributos de privilégio (DPL)
    uint16_t offset_mid;    // Offset bits 16..31
    uint32_t offset_high;   // Offset bits 32..63
    uint32_t zero;          // Reservado
} __attribute__((packed)) idt_entry_t;

typedef struct {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed)) idtr_t;

void idt_init(void);
void idt_set_gate(uint8_t num, uint64_t base, uint16_t sel, uint8_t flags, uint8_t ist);

#endif


