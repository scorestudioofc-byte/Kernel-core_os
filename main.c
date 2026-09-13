#include "types.h"
#include "gdt.h"
#include "idt.h"
#include "pmm.h"
#include "vmm.h"

extern uint8_t __kernel_end[];

void kmain(unsigned long magic, unsigned long addr) {
    if (magic != 0x36d37158) {
        while (1) { __asm__ __volatile__("cli; hlt"); }
    }

    // 1. GDT e TSS
    gdt_init();

    // 2. IDT
    idt_init();

    // 3. PMM
    pmm_init((void *)addr, (uintptr_t)__kernel_end);

    // 4. VMM
    vmm_init((uintptr_t)__kernel_end);

    // Loop de repouso
    while (1) {
        __asm__ __volatile__("hlt");
    }
}

