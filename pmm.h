 

ifndef PMM_H
#define PMM_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PAGE_SIZE           ((uintptr_t)4096u)
#define PAGE_MASK           (PAGE_SIZE - 1u)
#define PAGE_SHIFT          12u
#define PMM_INVALID_FRAME   ((uintptr_t)UINTPTR_MAX)

#define PAGE_ALIGN_DOWN(addr)       ((uintptr_t)(addr) & ~PAGE_MASK)
#define PAGE_ALIGN_UP(addr)         ((((uintptr_t)(addr) + PAGE_MASK) & ~PAGE_MASK))
#define PAGE_IS_ALIGNED(addr)       (((uintptr_t)(addr) & PAGE_MASK) == 0u)
#define PAGE_OFFSET(addr)           ((uintptr_t)(addr) & PAGE_MASK)

#define PAGES_FOR_SIZE(size)        \
    (((size_t)(size) == 0u) ? 0u : (((size_t)(size) - 1u) / PAGE_SIZE + 1u))

#define BYTES_FOR_PAGES(pages)      ((size_t)(pages) * PAGE_SIZE)

#define PMM_IS_INVALID_FRAME(frame) ((pmm_frame_t)(frame) == PMM_INVALID_FRAME)
#define PMM_IS_VALID_FRAME(frame)   ((pmm_frame_t)(frame) != PMM_INVALID_FRAME)

typedef uintptr_t pmm_frame_t;

_Static_assert(
    PAGE_SIZE != 0u && (PAGE_SIZE & (PAGE_SIZE - 1u)) == 0u,
    "PAGE_SIZE deve ser potência de dois"
);

_Static_assert(
    sizeof(pmm_frame_t) >= sizeof(void *),
    "pmm_frame_t insuficiente para endereços"
);

typedef struct {
    size_t total;
    size_t used;
    size_t free;
} pmm_stats_t;

void pmm_init(const void *multiboot_info, uintptr_t kernel_end);

pmm_frame_t pmm_alloc_block(void);
pmm_frame_t pmm_alloc_blocks(size_t count);
pmm_frame_t pmm_alloc_aligned(size_t count, size_t alignment);

void pmm_free_block(pmm_frame_t frame_addr);
void pmm_free_blocks(pmm_frame_t frame_addr, size_t count);

size_t pmm_get_total_blocks(void);
size_t pmm_get_used_blocks(void);
size_t pmm_get_free_blocks(void);

bool pmm_is_valid_block(pmm_frame_t frame);
bool pmm_is_allocated(pmm_frame_t frame);

void pmm_get_stats(pmm_stats_t *stats);

#ifdef __cplusplus
}
#endif

#endif
