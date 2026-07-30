#include "shell.h"
#include "filesystem.h"
#include "terminal.h"
#include "gui.h"
#include "nic.h"

#define SHELL_LINE_MAX 160

static char line[SHELL_LINE_MAX];
static uint16_t line_length;
static fs_node_t* working_directory;
static uint8_t shell_started;

static int strings_equal(const char* left, const char* right) {
    while (*left != '\0' && *right != '\0') {
        if (*left++ != *right++)
            return 0;
    }
    return *left == *right;
}

static char* next_word(char** text) {
    char* start;
    while (**text == ' ')
        (*text)++;
    if (**text == '\0')
        return 0;
    start = *text;
    while (**text != '\0' && **text != ' ')
        (*text)++;
    if (**text == ' ') {
        **text = '\0';
        (*text)++;
    }
    return start;
}

static char* rest_of_line(char** text) {
    while (**text == ' ')
        (*text)++;
    return **text == '\0' ? 0 : *text;
}

static void print_prompt(void) {
    terminal_write("duo:");
    fs_node_t* trail[FS_NAME_MAX];
    uint16_t count = 0;
    fs_node_t* node = working_directory;
    while (node != fs_root() && count < FS_NAME_MAX) {
        trail[count++] = node;
        node = node->parent;
    }
    terminal_putchar('/');
    while (count > 0) {
        terminal_write(trail[--count]->name);
        if (count > 0)
            terminal_putchar('/');
    }
    terminal_write("$ ");
}

static fs_node_t* resolve(char* path) {
    fs_node_t* node = path[0] == '/' ? fs_root() : working_directory;
    char* part = path;
    if (*part == '/')
        part++;

    if (*part == '\0')
        return node;
    while (*part != '\0') {
        char* next = part;
        while (*next != '\0' && *next != '/')
            next++;
        if (*next == '/')
            *next++ = '\0';
        if (strings_equal(part, ".")) {
            /* no change */
        } else if (strings_equal(part, "..")) {
            node = node->parent;
        } else {
            node = fs_find_child(node, part);
            if (node == 0)
                return 0;
        }
        part = next;
    }
    return node;
}

static void print_working_directory(void) {
    fs_node_t* trail[FS_NAME_MAX];
    uint16_t count = 0;
    fs_node_t* node = working_directory;
    while (node != fs_root() && count < FS_NAME_MAX) {
        trail[count++] = node;
        node = node->parent;
    }
    terminal_putchar('/');
    while (count > 0) {
        terminal_write(trail[--count]->name);
        if (count > 0)
            terminal_putchar('/');
    }
    terminal_putchar('\n');
}

static void command_help(void) {
    terminal_write("Commands: help clear pwd ls cd mkdir touch cat write rm\n");
    terminal_write("          desktop meminfo netinfo\n");
    terminal_write("write <file> <text> creates or replaces a text file.\n");
}

static void list_visitor(fs_node_t* node, void* context) {
    uint16_t* count = (uint16_t*)context;
    terminal_write(node->type == FS_DIRECTORY ? "DIR   " : "FILE  ");
    terminal_write(node->name);
    if (node->type == FS_FILE) {
        terminal_write("  ");
        terminal_write_dec(node->size);
        terminal_write(" bytes");
    }
    terminal_putchar('\n');
    (*count)++;
}

static void command_ls(void) {
    uint16_t found = 0;
    fs_list_directory(working_directory, list_visitor, &found);
    if (found == 0)
        terminal_write("(empty)\n");
}

static void execute(char* input) {
    char* cursor = input;
    char* command = next_word(&cursor);
    char* argument;
    fs_node_t* node;

    if (command == 0)
        return;
    if (strings_equal(command, "help")) command_help();
    else if (strings_equal(command, "clear")) terminal_clear();
    else if (strings_equal(command, "pwd")) {
        print_working_directory();
    } else if (strings_equal(command, "ls")) command_ls();
    else if (strings_equal(command, "cd")) {
        argument = next_word(&cursor);
        node = argument ? resolve(argument) : fs_root();
        if (node != 0 && node->type == FS_DIRECTORY) working_directory = node;
        else terminal_write("cd: directory not found\n");
    } else if (strings_equal(command, "mkdir") || strings_equal(command, "touch")) {
        argument = next_word(&cursor);
        if (argument == 0 || fs_create(working_directory, argument,
            strings_equal(command, "mkdir") ? FS_DIRECTORY : FS_FILE) == 0)
            terminal_write("create: invalid name or no free entries\n");
    } else if (strings_equal(command, "cat")) {
        argument = next_word(&cursor); node = argument ? resolve(argument) : 0;
        if (node != 0 && node->type == FS_FILE) { terminal_write(node->content); terminal_putchar('\n'); }
        else terminal_write("cat: file not found\n");
    } else if (strings_equal(command, "write")) {
        uint8_t has_path_separator = 0;
        argument = next_word(&cursor);
        if (argument != 0) {
            for (uint16_t i = 0; argument[i] != '\0'; i++)
                if (argument[i] == '/') has_path_separator = 1;
        }
        node = argument ? resolve(argument) : 0;
        char* content = rest_of_line(&cursor);
        if (node == 0 && argument != 0 && !has_path_separator)
            node = fs_create(working_directory, argument, FS_FILE);
        if (node != 0 && content != 0 && fs_write(node, content)) terminal_write("saved\n");
        else terminal_write("write: a file name and text are required\n");
    } else if (strings_equal(command, "rm")) {
        argument = next_word(&cursor); node = argument ? resolve(argument) : 0;
        if (!fs_remove(node)) terminal_write("rm: file not found or directory is not empty\n");
    } else if (strings_equal(command, "desktop")) {
        gui_show_desktop();
    } else if (strings_equal(command, "meminfo")) {
        terminal_write("RAM filesystem: 32 entries, up to 255 bytes per file\n");
    } else if (strings_equal(command, "netinfo")) {
        terminal_write("Network adapter: ");
        terminal_write(nic_name());
        terminal_putchar('\n');
    } else terminal_write("Unknown command. Type help.\n");
}

void shell_init(void) {
    if (!shell_started) {
        fs_init();
        working_directory = fs_root();
        shell_started = 1;
    }
    line_length = 0;
    terminal_write("RAM filesystem ready.\n");
    print_prompt();
}

void shell_handle_key(char c) {
    if (c == '\n') {
        terminal_putchar('\n');
        line[line_length] = '\0';
        execute(line);
        line_length = 0;
        if (!gui_desktop_active())
            print_prompt();
    } else if (c == '\b') {
        if (line_length > 0) {
            line_length--;
            terminal_putchar('\b');
        }
    } else if (c >= ' ' && c <= '~' && line_length < SHELL_LINE_MAX - 1) {
        line[line_length++] = c;
        terminal_putchar(c);
    }
}
