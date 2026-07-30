#ifndef DUOOS_FILESYSTEM_H
#define DUOOS_FILESYSTEM_H

#include <stdint.h>

#define FS_NAME_MAX 24
#define FS_CONTENT_MAX 256

typedef enum {
    FS_FILE,
    FS_DIRECTORY
} fs_node_type_t;

typedef struct fs_node {
    char name[FS_NAME_MAX];
    fs_node_type_t type;
    struct fs_node* parent;
    char content[FS_CONTENT_MAX];
    uint16_t size;
    uint8_t used;
} fs_node_t;

typedef void (*fs_directory_visitor_t)(fs_node_t* node, void* context);

void fs_init(void);
fs_node_t* fs_root(void);
fs_node_t* fs_find_child(fs_node_t* directory, const char* name);
void fs_list_directory(fs_node_t* directory, fs_directory_visitor_t visitor, void* context);
fs_node_t* fs_create(fs_node_t* directory, const char* name, fs_node_type_t type);
int fs_remove(fs_node_t* node);
int fs_write(fs_node_t* node, const char* text);
const char* fs_type_name(fs_node_type_t type);

#endif
