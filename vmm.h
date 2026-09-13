#ifndef VMM_H
#define VMM_H

#include "types.h"

#define PAGE_PRESENT  (1ULL << 0)
#define PAGE_WRITABLE (1ULL << 1)
#define PAGE_USER     (1ULL << 2)
#define PAGE_NX       (1ULL << 63)

typedef uint64_t pml4_t;

void vmm_init(uintptr_t kernel_physical_end);
bool vmm_map_page(uintptr_t virt_addr, uintptr_t phys_addr, uint64_t flags);
void vmm_switch_pml4(uintptr_t pml4_phys);

#endif

