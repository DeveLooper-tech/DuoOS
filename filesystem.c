#include "filesystem.h"

#define FS_MAX_NODES 32

static fs_node_t nodes[FS_MAX_NODES];

static int strings_equal(const char* left, const char* right) {
    while (*left != '\0' && *right != '\0') {
        if (*left++ != *right++)
            return 0;
    }
    return *left == *right;
}

static int valid_name(const char* name) {
    uint16_t length = 0;
    if (*name == '\0' || (*name == '.' && name[1] == '\0'))
        return 0;
    while (name[length] != '\0') {
        if (name[length] == '/' || name[length] == ' ' || ++length >= FS_NAME_MAX)
            return 0;
    }
    return 1;
}

void fs_init(void) {
    for (uint16_t i = 0; i < FS_MAX_NODES; i++)
        nodes[i].used = 0;

    nodes[0].used = 1;
    nodes[0].type = FS_DIRECTORY;
    nodes[0].parent = &nodes[0];
    nodes[0].name[0] = '\0';
    nodes[0].size = 0;
}

fs_node_t* fs_root(void) {
    return &nodes[0];
}

fs_node_t* fs_find_child(fs_node_t* directory, const char* name) {
    if (directory == 0 || directory->type != FS_DIRECTORY)
        return 0;

    for (uint16_t i = 1; i < FS_MAX_NODES; i++) {
        if (nodes[i].used && nodes[i].parent == directory && strings_equal(nodes[i].name, name))
            return &nodes[i];
    }
    return 0;
}

void fs_list_directory(fs_node_t* directory, fs_directory_visitor_t visitor, void* context) {
    if (directory == 0 || directory->type != FS_DIRECTORY || visitor == 0)
        return;
    for (uint16_t i = 1; i < FS_MAX_NODES; i++)
        if (nodes[i].used && nodes[i].parent == directory)
            visitor(&nodes[i], context);
}

fs_node_t* fs_create(fs_node_t* directory, const char* name, fs_node_type_t type) {
    if (directory == 0 || directory->type != FS_DIRECTORY || !valid_name(name)
        || fs_find_child(directory, name) != 0)
        return 0;

    for (uint16_t i = 1; i < FS_MAX_NODES; i++) {
        if (!nodes[i].used) {
            fs_node_t* node = &nodes[i];
            uint16_t j = 0;
            while (name[j] != '\0') {
                node->name[j] = name[j];
                j++;
            }
            node->name[j] = '\0';
            node->type = type;
            node->parent = directory;
            node->content[0] = '\0';
            node->size = 0;
            node->used = 1;
            return node;
        }
    }
    return 0;
}

int fs_remove(fs_node_t* node) {
    if (node == 0 || node == fs_root())
        return 0;
    if (node->type == FS_DIRECTORY) {
        for (uint16_t i = 1; i < FS_MAX_NODES; i++)
            if (nodes[i].used && nodes[i].parent == node)
                return 0;
    }
    node->used = 0;
    return 1;
}

int fs_write(fs_node_t* node, const char* text) {
    if (node == 0 || node->type != FS_FILE)
        return 0;

    uint16_t i = 0;
    while (text[i] != '\0' && i < FS_CONTENT_MAX - 1) {
        node->content[i] = text[i];
        i++;
    }
    node->content[i] = '\0';
    node->size = i;
    return text[i] == '\0';
}

const char* fs_type_name(fs_node_type_t type) {
    return type == FS_DIRECTORY ? "dir" : "file";
}
