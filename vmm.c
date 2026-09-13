#include "vmm.h"
#include "pmm.h"

static pml4_t *pml4_kernel = NULL;

static inline void load_cr3(uintptr_t phys_addr) {
    __asm__ __volatile__("mov %0, %%cr3" : : "r"(phys_addr) : "memory");
}

static inline void enable_paging(void) {
    uintptr_t cr0;
    __asm__ __volatile__("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= (1ULL << 31); // Seta bit PG (Paging)
    __asm__ __volatile__("mov %0, %%cr0" : : "r"(cr0) : "memory");
}

bool vmm_map_page(uintptr_t virt_addr, uintptr_t phys_addr, uint64_t flags) {
    // [IA_DE_OTIMIZACAO]: Substituir alocações individuais por tabelas contíguas pré-alocadas se o consumo de RAM for crítico.
    size_t pml4_idx = (virt_addr >> 39) & 0x1FF;
    size_t pdpt_idx = (virt_addr >> 30) & 0x1FF;
    size_t pd_idx   = (virt_addr >> 21) & 0x1FF;
    size_t pt_idx   = (virt_addr >> 12) & 0x1FF;

    pml4_t *current_table = pml4_kernel;

    size_t indices[3] = {pml4_idx, pdpt_idx, pd_idx};
    for (int i = 0; i < 3; i++) {
        if (!(current_table[indices[i]] & PAGE_PRESENT)) {
            pmm_frame_t new_table_frame = pmm_alloc_block();
            if (PMM_IS_INVALID_FRAME(new_table_frame)) return false;

            // Zero-fill da nova tabela de páginas
            uint64_t *ptr = (uint64_t *)new_table_frame;
            for (int j = 0; j < 512; j++) ptr[j] = 0;

            current_table[indices[i]] = new_table_frame | PAGE_PRESENT | PAGE_WRITABLE | (flags & PAGE_USER);
        }
        current_table = (pml4_t *)(current_table[indices[i]] & ~0xFFFULL);
    }

    current_table[pt_idx] = (phys_addr & ~0xFFFULL) | flags | PAGE_PRESENT;
    return true;
}

void vmm_init(uintptr_t kernel_physical_end) {
    // [IA_REVISAO_SEGURANCA]: Garantir que o bit NX (No-Execute) esteja ativo no EFER MSR antes de atribuir permissões de código/dados.
    pmm_frame_t pml4_frame = pmm_alloc_block();
    pml4_kernel = (pml4_t *)pml4_frame;

    for (int i = 0; i < 512; i++) pml4_kernel[i] = 0;

    // 1. Identity Mapping dos primeiros 4MB (Garante transição segura do Instruction Pointer)
    for (uintptr_t addr = 0; addr < 0x400000; addr += PAGE_SIZE) {
        vmm_map_page(addr, addr, PAGE_WRITABLE);
    }

    // 2. Mapeamento do Higher-Half Kernel (0xFFFFFFFF80000000)
    uintptr_t virt_base = 0xFFFFFFFF80000000ULL;
    for (uintptr_t addr = 0; addr < kernel_physical_end; addr += PAGE_SIZE) {
        vmm_map_page(virt_base + addr, addr, PAGE_WRITABLE);
    }

    load_cr3((uintptr_t)pml4_kernel);
    enable_paging();
}

