ASM = nasm
CC  = x86_64-elf-gcc
LD  = x86_64-elf-ld

CFLAGS  = -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone \
          -mcmodel=kernel -Wall -Wextra -c -std=gnu11 -Iinclude
ASFLAGS = -f elf64

C_SOURCES = kernel/kernel.c kernel/terminal.c kernel/shell.c kernel/filesystem.c \
            kernel/gui.c kernel/pmm.c arch/x86/idt.c arch/x86/isr_handlers.c \
            drivers/pic.c drivers/keyboard.c drivers/mouse.c drivers/pci.c \
            drivers/e1000.c drivers/rtl8139.c network/nic.c network/net.c
ASM_SOURCES = boot/boot.asm arch/x86/isr.asm

C_OBJECTS = $(C_SOURCES:.c=.o)
ASM_OBJECTS = $(ASM_SOURCES:.asm=.o)
OBJECTS = $(ASM_OBJECTS) $(C_OBJECTS)

all: duoos.iso

%.o: %.asm
	$(ASM) $(ASFLAGS) $< -o $@

%.o: %.c
	$(CC) $(CFLAGS) $< -o $@

duoos.bin: $(OBJECTS) boot/linker.ld
	$(LD) -n -T boot/linker.ld -o duoos.bin $(OBJECTS)

duoos.iso: duoos.bin boot/grub.cfg
	mkdir -p isodir/boot/grub
	cp duoos.bin isodir/boot/duoos.bin
	cp boot/grub.cfg isodir/boot/grub/grub.cfg
	grub-mkrescue -o duoos.iso isodir

run: duoos.iso
	qemu-system-x86_64 -cdrom duoos.iso -netdev user,id=n0 -device e1000,netdev=n0

clean:
	rm -rf $(OBJECTS) *.o duoos.bin duoos.iso isodir

.PHONY: all run clean
