ENTRY(_start)

SECTIONS
{
    . = 1M;

    .boot :
    {
        *(.multiboot2)
    }

    .text :
    {
        *(.text)
    }

    .rodata :
    {
        *(.rodata)
    }

    .data :
    {
        *(.data)
    }

    .bss :
    {
        *(COMMON)
        *(.bss)
    }

    . = ALIGN(4096);
    kernel_end = .;
}
