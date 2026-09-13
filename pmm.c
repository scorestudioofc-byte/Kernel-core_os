#include "pmm.h"

// Estruturas internas para navegação no mapa de memória do Multiboot2
struct multiboot_tag {
    uint32_t type;
    uint32_t size;
};

struct multiboot_mmap_entry {
    uint64_t addr;
    uint64_t len;
    uint32_t type;
    uint32_t reserved;
} __attribute__((packed));

struct multiboot_tag_mmap {
    uint32_t type;
    uint32_t size;
    uint32_t entry_size;
    uint32_t entry_version;
    struct multiboot_mmap_entry entries[];
};

#define MULTIBOOT_TAG_TYPE_END  0
#define MULTIBOOT_TAG_TYPE_MMAP 6
#define MULTIBOOT_MEMORY_AVAIL  1

static uint64_t *bitmap = NULL;
static size_t total_frames = 0;
static size_t used_frames = 0;
static size_t bitmap_uint64_count = 0;

static inline void bit_set(size_t bit) {
    bitmap[bit / 64u] |= (1ULL << (bit % 64u));
}

static inline void bit_clear(size_t bit) {
    bitmap[bit / 64u] &= ~(1ULL << (bit % 64u));
}

static inline bool bit_test(size_t bit) {
    return (bitmap[bit / 64u] & (1ULL << (bit % 64u))) != 0u;
}

void pmm_init(const void *multiboot_info, uintptr_t kernel_end) {
    // Garantir alinhamento de página para o início do bitmap
    uintptr_t bitmap_start = PAGE_ALIGN_UP(kernel_end);

    const struct multiboot_tag *tag;
    const struct multiboot_mmap_entry *mmap_entries = NULL;
    size_t mmap_entries_count = 0;

    // Varredura de tags Multiboot2
    for (tag = (const struct multiboot_tag *)((const uint8_t *)multiboot_info + 8);
         tag->type != MULTIBOOT_TAG_TYPE_END;
         tag = (const struct multiboot_tag *)((const uint8_t *)tag + ((tag->size + 7u) & ~7u))) {
        
        if (tag->type == MULTIBOOT_TAG_TYPE_MMAP) {
            const struct multiboot_tag_mmap *mmap_tag = (const struct multiboot_tag_mmap *)tag;
            mmap_entries = mmap_tag->entries;
            mmap_entries_count = (mmap_tag->size - sizeof(struct multiboot_tag_mmap)) / mmap_tag->entry_size;
            break;
        }
    }

    uintptr_t highest_addr = 0;
    for (size_t i = 0; i < mmap_entries_count; i++) {
        if (mmap_entries[i].type == MULTIBOOT_MEMORY_AVAIL) {
            uintptr_t top = (uintptr_t)(mmap_entries[i].addr + mmap_entries[i].len);
            if (top > highest_addr) highest_addr = top;
        }
    }

    total_frames = highest_addr / PAGE_SIZE;
    bitmap_uint64_count = (total_frames + 63u) / 64u;
    bitmap = (uint64_t *)bitmap_start;

    // Bloqueia toda a memória por padrão
    for (size_t i = 0; i < bitmap_uint64_count; i++) {
        bitmap[i] = ~0ULL;
    }
    used_frames = total_frames;

    // Libera regiões disponíveis, respeitando o kernel e o bitmap
    uintptr_t bitmap_end = bitmap_start + (bitmap_uint64_count * sizeof(uint64_t));
    bitmap_end = PAGE_ALIGN_UP(bitmap_end);

    for (size_t i = 0; i < mmap_entries_count; i++) {
        if (mmap_entries[i].type == MULTIBOOT_MEMORY_AVAIL) {
            uintptr_t base = PAGE_ALIGN_UP(mmap_entries[i].addr);
            uintptr_t limit = PAGE_ALIGN_DOWN(mmap_entries[i].addr + mmap_entries[i].len);

            for (uintptr_t addr = base; addr < limit; addr += PAGE_SIZE) {
                // Preserva os primeiros 1MB (IVT, BIOS) e a área do kernel/bitmap
                if (addr < 0x100000u || (addr >= 0x100000u && addr < bitmap_end)) {
                    continue;
                }
                pmm_free_block(addr);
            }
        }
    }
}

pmm_frame_t pmm_alloc_block(void) {
    // [IA_DE_OTIMIZACAO]: Otimizado para pular quadros inteiros de 64 bits ocupados
    for (size_t i = 0; i < bitmap_uint64_count; i++) {
        if (bitmap[i] != ~0ULL) {
            for (size_t j = 0; j < 64u; j++) {
                if (!(bitmap[i] & (1ULL << j))) {
                    size_t frame_idx = (i * 64u) + j;
                    if (frame_idx >= total_frames) return PMM_INVALID_FRAME;
                    
                    bit_set(frame_idx);
                    used_frames++;
                    return (pmm_frame_t)(frame_idx * PAGE_SIZE);
                }
            }
        }
    }
    return PMM_INVALID_FRAME;
}

pmm_frame_t pmm_alloc_blocks(size_t count) {
    if (count == 0) return PMM_INVALID_FRAME;
    if (count == 1) return pmm_alloc_block();

    size_t consecutive = 0;
    size_t start_frame = 0;

    for (size_t i = 0; i < total_frames; i++) {
        if (!bit_test(i)) {
            if (consecutive == 0) start_frame = i;
            consecutive++;
            if (consecutive == count) {
                for (size_t j = start_frame; j < start_frame + count; j++) {
                    bit_set(j);
                }
                used_frames += count;
                return (pmm_frame_t)(start_frame * PAGE_SIZE);
            }
        } else {
            consecutive = 0;
        }
    }
    return PMM_INVALID_FRAME;
}

pmm_frame_t pmm_alloc_aligned(size_t count, size_t alignment) {
    if (alignment == 0 || (alignment & (alignment - 1)) != 0) return PMM_INVALID_FRAME;
    size_t align_frames = alignment / PAGE_SIZE;
    if (align_frames == 0) align_frames = 1;

    for (size_t i = 0; i < total_frames; i += align_frames) {
        bool free_chunk = true;
        for (size_t j = 0; j < count; j++) {
            if (i + j >= total_frames || bit_test(i + j)) {
                free_chunk = false;
                break;
            }
        }
        if (free_chunk) {
            for (size_t j = 0; j < count; j++) {
                bit_set(i + j);
            }
            used_frames += count;
            return (pmm_frame_t)(i * PAGE_SIZE);
        }
    }
    return PMM_INVALID_FRAME;
}

void pmm_free_block(pmm_frame_t frame_addr) {
    size_t frame_idx = frame_addr / PAGE_SIZE;
    if (frame_idx < total_frames && bit_test(frame_idx)) {
        bit_clear(frame_idx);
        used_frames--;
    }
}

void pmm_free_blocks(pmm_frame_t frame_addr, size_t count) {
    for (size_t i = 0; i < count; i++) {
        pmm_free_block(frame_addr + (i * PAGE_SIZE));
    }
}

size_t pmm_get_total_blocks(void) { return total_frames; }
size_t pmm_get_used_blocks(void) { return used_frames; }
size_t pmm_get_free_blocks(void) { return total_frames - used_frames; }

bool pmm_is_valid_block(pmm_frame_t frame) { return (frame / PAGE_SIZE) < total_frames; }
bool pmm_is_allocated(pmm_frame_t frame) {
    size_t idx = frame / PAGE_SIZE;
    return (idx < total_frames) ? bit_test(idx) : true;
}

void pmm_get_stats(pmm_stats_t *stats) {
    if (!stats) return;
    stats->total = total_frames;
    stats->used = used_frames;
    stats->free = total_frames - used_frames;
}
	
