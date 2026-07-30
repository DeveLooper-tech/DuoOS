#ifndef DUOOS_MULTIBOOT2_H
#define DUOOS_MULTIBOOT2_H

#include <stdint.h>

typedef struct {
    uint32_t total_size;
    uint32_t reserved;
} mb2_info_t;

typedef struct {
    uint32_t type;
    uint32_t size;
} mb2_tag_t;

#define MB2_TAG_TYPE_END  0
#define MB2_TAG_TYPE_MMAP 6

#define MB2_MEMORY_AVAILABLE 1

typedef struct {
    uint32_t type;
    uint32_t size;
    uint32_t entry_size;
    uint32_t entry_version;
} mb2_tag_mmap_t;

typedef struct {
    uint64_t base_addr;
    uint64_t length;
    uint32_t type;
    uint32_t reserved;
} mb2_mmap_entry_t;

#endif
