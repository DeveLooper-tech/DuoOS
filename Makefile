ASM = nasm
CC  = x86_64-elf-gcc
LD  = x86_64-elf-ld

CFLAGS  = -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone \
          -mcmodel=kernel -Wall -Wextra -c -std=gnu11
ASFLAGS = -f elf64

C_SOURCES = kernel.c terminal.c idt.c isr_handlers.c pic.c keyboard.c mouse.c gui.c pmm.c pci.c e1000.c rtl8139.c nic.c net.c filesystem.c shell.c
ASM_SOURCES = boot.asm isr.asm

C_OBJECTS = $(C_SOURCES:.c=.o)
ASM_OBJECTS = $(ASM_SOURCES:.asm=.o)
OBJECTS = $(ASM_OBJECTS) $(C_OBJECTS)

all: duoos.iso

%.o: %.asm
	$(ASM) $(ASFLAGS) $< -o $@

%.o: %.c
	$(CC) $(CFLAGS) $< -o $@

duoos.bin: $(OBJECTS) linker.ld
	$(LD) -n -T linker.ld -o duoos.bin $(OBJECTS)

duoos.iso: duoos.bin grub.cfg
	mkdir -p isodir/boot/grub
	cp duoos.bin isodir/boot/duoos.bin
	cp grub.cfg isodir/boot/grub/grub.cfg
	grub-mkrescue -o duoos.iso isodir

run: duoos.iso
	qemu-system-x86_64 -cdrom duoos.iso -netdev user,id=n0 -device e1000,netdev=n0

clean:
	rm -rf *.o *.bin *.iso isodir

.PHONY: all run clean
