# DuoOS

DuoOS is a bootable 64-bit x86_64 hobby operating-system kernel. GRUB starts
the kernel through Multiboot2; `boot.asm` enables paging and long mode before
handing control to the C kernel.

## Current features

- VGA text-mode desktop with a clickable Terminal icon
- PS/2 keyboard and mouse support
- Interactive shell and a small RAM filesystem
- x86 exceptions, PIC IRQ routing, and physical-memory bitmap allocator
- PCI network adapter detection
- Ethernet support for Intel E1000/8254x variants and Realtek RTL8139
- ARP replies and ICMP echo replies through the selected adapter

The filesystem is memory-backed: its contents are intentionally lost on reboot.

## Desktop and terminal

The desktop starts automatically. Click the **Terminal** icon, or press Enter,
to open the command line. Run `desktop` inside the terminal to return.

The shell provides:

```
help clear pwd ls cd mkdir touch cat write rm desktop meminfo netinfo
```

For example, `write note Hello` creates a file and stores its text. `netinfo`
prints the currently selected Ethernet driver.

## Supported Ethernet adapters

At boot, DuoOS first tries a supported Intel E1000-family PCI device and then
tries a Realtek RTL8139. The network stack uses the selected driver through a
single NIC interface, so additional drivers can be added without changing ARP
or ICMP handling.

VirtIO, modern Realtek PCIe adapters, and USB Ethernet need separate drivers;
they are not presented as supported until those implementations exist.

## Build and run

Install `nasm`, an `x86_64-elf-gcc` cross-compiler, GRUB tools, `xorriso`, and
QEMU. Then run:

```bash
make
make run
```

To explicitly choose a QEMU network device:

```bash
qemu-system-x86_64 -cdrom duoos.iso -netdev user,id=n0 -device e1000,netdev=n0
qemu-system-x86_64 -cdrom duoos.iso -netdev user,id=n0 -device rtl8139,netdev=n0
```

`make` creates the bootable `duoos.iso` image.
