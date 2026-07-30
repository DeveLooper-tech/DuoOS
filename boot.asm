bits 32

section .multiboot2
align 8
multiboot_header:
    dd 0xE85250D6
    dd 0
    dd multiboot_header_end - multiboot_header
    dd -(0xE85250D6 + 0 + (multiboot_header_end - multiboot_header))
    dw 0
    dw 0
    dd 8
multiboot_header_end:

section .bss
align 4096
p4_table:    resb 4096
p3_table:    resb 4096
p2_table_0:  resb 4096
p2_table_1:  resb 4096
p2_table_2:  resb 4096
p2_table_3:  resb 4096
stack_bottom:
    resb 16384
stack_top:
mb_info:  resd 1
mb_magic: resd 1

section .text
global _start
extern kernel_main

_start:
    mov esp, stack_top
    mov [mb_info], ebx
    mov [mb_magic], eax

    call setup_page_tables
    call enable_paging

    lgdt [gdt64.pointer]
    jmp gdt64.code:long_mode_start

setup_page_tables:
    mov eax, p3_table
    or eax, 0b11
    mov [p4_table], eax

    mov eax, p2_table_0
    or eax, 0b11
    mov [p3_table], eax
    mov eax, p2_table_1
    or eax, 0b11
    mov [p3_table + 8], eax
    mov eax, p2_table_2
    or eax, 0b11
    mov [p3_table + 16], eax
    mov eax, p2_table_3
    or eax, 0b11
    mov [p3_table + 24], eax

    mov edi, p2_table_0
    xor ebx, ebx
    mov ecx, 2048            ; 4 x 512 = 2048 db 2MB-os lap = 4 GiB
.map_loop:
    mov eax, ebx
    or eax, 0b10000011
    mov [edi], eax
    add edi, 8
    add ebx, 0x200000
    loop .map_loop
    ret

enable_paging:
    mov eax, p4_table
    mov cr3, eax

    mov eax, cr4
    or eax, 1 << 5
    mov cr4, eax

    mov ecx, 0xC0000080
    rdmsr
    or eax, 1 << 8
    wrmsr

    mov eax, cr0
    or eax, 1 << 31
    mov cr0, eax
    ret

section .rodata
gdt64:
    dq 0
.code: equ $ - gdt64
    dq (1<<43) | (1<<44) | (1<<47) | (1<<53)
.pointer:
    dw $ - gdt64 - 1
    dq gdt64

section .text
bits 64
long_mode_start:
    mov ax, 0
    mov ss, ax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    mov edi, [mb_info]
    call kernel_main

.hang:
    hlt
    jmp .hang
