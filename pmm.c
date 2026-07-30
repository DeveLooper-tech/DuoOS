#include "pmm.h"
#include "multiboot2.h"
#include "terminal.h"

#define MAX_FRAMES ((uint64_t)4ULL * 1024 * 1024 * 1024 / PMM_FRAME_SIZE)

static uint8_t frame_bitmap[MAX_FRAMES / 8];
static uint64_t free_frames = 0;

extern uint8_t kernel_end;

static inline void set_used(uint64_t frame) {
    if (frame >= MAX_FRAMES) return;
    if (!(frame_bitmap[frame / 8] & (1 << (frame % 8)))) {
        frame_bitmap[frame / 8] |= (1 << (frame % 8));
        if (free_frames > 0) free_frames--;
    }
}

static inline void set_free(uint64_t frame) {
    if (frame >= MAX_FRAMES) return;
    if (frame_bitmap[frame / 8] & (1 << (frame % 8))) {
        frame_bitmap[frame / 8] &= ~(1 << (frame % 8));
        free_frames++;
    }
}

static inline int is_used(uint64_t frame) {
    if (frame >= MAX_FRAMES) return 1;
    return frame_bitmap[frame / 8] & (1 << (frame % 8));
}

void pmm_init(uint64_t multiboot_info_addr) {
    for (uint64_t i = 0; i < MAX_FRAMES; i++)
        frame_bitmap[i / 8] |= (1 << (i % 8));
    free_frames = 0;

    mb2_info_t* info = (mb2_info_t*)(uintptr_t)multiboot_info_addr;
    uint8_t* tag_ptr = (uint8_t*)info + 8;
    uint8_t* end_ptr = (uint8_t*)info + info->total_size;

    while (tag_ptr < end_ptr) {
        mb2_tag_t* tag = (mb2_tag_t*)tag_ptr;
        if (tag->type == MB2_TAG_TYPE_END)
            break;

        if (tag->type == MB2_TAG_TYPE_MMAP) {
            mb2_tag_mmap_t* mmap = (mb2_tag_mmap_t*)tag;
            uint8_t* entry_ptr = (uint8_t*)mmap + sizeof(mb2_tag_mmap_t);
            uint8_t* mmap_end  = (uint8_t*)mmap + mmap->size;

            while (entry_ptr < mmap_end) {
                mb2_mmap_entry_t* e = (mb2_mmap_entry_t*)entry_ptr;
                if (e->type == MB2_MEMORY_AVAILABLE) {
                    uint64_t start_frame = e->base_addr / PMM_FRAME_SIZE;
                    uint64_t num_frames  = e->length / PMM_FRAME_SIZE;
                    for (uint64_t f = 0; f < num_frames; f++)
                        set_free(start_frame + f);
                }
                entry_ptr += mmap->entry_size;
            }
        }

        tag_ptr += (tag->size + 7) & ~7u;
    }

    uint64_t kernel_end_frame = ((uint64_t)(uintptr_t)&kernel_end) / PMM_FRAME_SIZE + 1;
    for (uint64_t f = 0; f < kernel_end_frame; f++)
        set_used(f);
}

uint64_t pmm_alloc_frame(void) {
    for (uint64_t i = 0; i < MAX_FRAMES; i++) {
        if (!is_used(i)) {
            set_used(i);
            return i * PMM_FRAME_SIZE;
        }
    }
    return 0;
}

void pmm_free_frame(uint64_t phys_addr) {
    set_free(phys_addr / PMM_FRAME_SIZE);
}

uint64_t pmm_free_frame_count(void) {
    return free_frames;
}

uint64_t pmm_total_frame_count(void) {
    return MAX_FRAMES;
}
