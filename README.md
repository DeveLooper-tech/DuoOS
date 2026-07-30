# DuoOS – saját 64 bites x86_64 operációs rendszer, alapoktól

Ez egy minimális, de **ténylegesen bootolható** 64 bites kernel-váz. GRUB2 tölti be
multiboot2 protokollal, ami 32 bites protected módban indít – a `boot.asm` állítja
be a lapozást (paging) és vált át 64 bites long módba, majd meghívja a C-ben írt
kernelt, ami kiír egy üzenetet a képernyőre.

Ez a "0. lépés" egy hobby OS-hez. Innen tudsz építkezni: megszakításkezelés,
memóriakezelő, billentyűzet driver, fájlrendszer, felhasználói módú folyamatok stb.

## Fájlok

- `boot.asm` – multiboot2 header, lapozás beállítása, átállás long módba,
  a multiboot2 info pointer elmentése/átadása a kernelnek
- `kernel.c` – a C kernel belépési pontja (`kernel_main`), inicializálási sorrend
- `terminal.c/.h` – VGA szöveges kiírás (write, hex, decimális)
- `idt.c/.h` – Interrupt Descriptor Table felépítése és betöltése
- `isr.asm` – megszakítás-belépési pontok (kivételek + IRQ-k), regisztermentés
- `isr_handlers.c` – C oldali kivétel-dump és IRQ diszpécser
- `pic.c/.h` – 8259 PIC átcímzése (remap) IRQ0-15 → INT 32-47, EOI kezelés
- `keyboard.c/.h` – PS/2 billentyűzet driver (IRQ1, scancode → ASCII)
- `shell.c/.h` – sor-alapú parancsértelmező és billentyűzet-bemenet kezelése
- `filesystem.c/.h` – egyszerű, fa-struktúrájú RAM fájlrendszer
- `pmm.c/.h` + `multiboot2.h` – bitmap alapú fizikai memóriakezelő,
  a multiboot2 memória-térkép (mmap tag) feldolgozásával
- `io.h` – `inb`/`outb` port I/O segédfüggvények
- `linker.ld` – linker script, `kernel_end` szimbólum a PMM számára
- `grub.cfg` – GRUB menü bejegyzés a DuoOS-hez
- `Makefile` – build automatizálás (minden `.c`/`.asm` fájlt lefordít)

### Indítási sorrend (`kernel_main`)

1. `terminal_clear()` – képernyő törlése
2. `idt_init()` – IDT feltöltése, `lidt` végrehajtása
3. `pic_remap()` – PIC-ek átcímzése, hogy az IRQ-k ne ütközzenek a CPU
   kivételekkel (0-31)
4. `pmm_init()` – multiboot2 memória-térkép feldolgozása, szabad
   keretek (frame) nyilvántartásba vétele
5. `sti` – megszakítások engedélyezése
6. Ezután a billentyűzet (IRQ1) élőben ír a képernyőre – próbáld ki!

## 1. Szükséges eszközök (Ubuntu/Debian alapon)

Natív `gcc` **nem** használható közvetlenül, mert az Linux binárisokat (ELF, glibc
függőségekkel) generál – nekünk "szabad álló" (freestanding), operációs rendszer
nélküli kódra van szükségünk. Ezért kell egy **cross-compiler** target: `x86_64-elf`.

```bash
sudo apt update
sudo apt install build-essential nasm grub-pc-bin grub-common xorriso \
                  qemu-system-x86 mtools bison flex libgmp-dev libmpc-dev \
                  libmpfr-dev texinfo
```

### Cross-compiler telepítése (x86_64-elf-gcc)

A legegyszerűbb, ha egy előre buildelt toolchaint töltesz le (pl. a
`x86_64-elf-tools` csomagokból különböző disztribúciókhoz), vagy magad fordítod
le az OSDev wiki "GCC Cross-Compiler" útmutatója alapján (binutils + gcc, target
`x86_64-elf`, `--without-headers`). Ez kb. 20-30 percet vesz igénybe, de csak
egyszer kell megcsinálni. Röviden:

```bash
export PREFIX="$HOME/opt/cross"
export TARGET=x86_64-elf
export PATH="$PREFIX/bin:$PATH"

mkdir -p ~/src && cd ~/src
wget https://ftp.gnu.org/gnu/binutils/binutils-2.42.tar.gz
wget https://ftp.gnu.org/gnu/gcc/gcc-13.2.0/gcc-13.2.0.tar.gz
tar xf binutils-2.42.tar.gz
tar xf gcc-13.2.0.tar.gz

mkdir build-binutils && cd build-binutils
../binutils-2.42/configure --target=$TARGET --prefix="$PREFIX" \
    --with-sysroot --disable-nls --disable-werror
make -j$(nproc)
make install
cd ..

mkdir build-gcc && cd build-gcc
../gcc-13.2.0/configure --target=$TARGET --prefix="$PREFIX" \
    --disable-nls --enable-languages=c --without-headers
make -j$(nproc) all-gcc
make -j$(nproc) all-target-libgcc
make install-gcc
make install-target-libgcc
```

Ezután `x86_64-elf-gcc` és `x86_64-elf-ld` elérhető lesz a `PATH`-on.

## 2. Fordítás

```bash
cd duoos
make
```

Ez létrehozza a `duoos.iso` fájlt (bootolható CD/USB image GRUB-bal).

## 3. Futtatás QEMU-ban

```bash
make run
```

Ha minden jól ment, egy fekete képernyőn zöld szöveggel meg kell jelennie:

```
DuoOS v0.1
64 bites kernel sikeresen elindult long modeban.

Kovetkezo lepesek: GDT/IDT, megszakitasok,
memoriakezeles, billentyuzet driver...
```

## 4. Valós gépen / USB-n tesztelés (opcionális)

```bash
sudo dd if=duoos.iso of=/dev/sdX bs=4M status=progress && sync
```

**Vigyázz**: `/dev/sdX` helyére a helyes eszközt írd (`lsblk` segít
ellenőrizni), különben adatot veszíthetsz.

## 5. Hogyan tovább? – ütemterv DuoOS-hez

✅ **Kész:** IDT/ISR/IRQ, PIC remap, PS/2 billentyűzet driver, bitmap alapú
fizikai memóriakezelő (PMM). Ezek a `v0.2`-ben már benne vannak.

Következő lépések (nagyjából sorrendben):

1. **Shift/Caps Lock kezelés + billentyűzet-puffer** – jelenleg csak
   kisbetűs US layout működik, nincs shift-tábla és nincs sor-alapú
   bemenet-kezelés (line editing, Enterre lezáró input buffer).
2. **Virtuális memóriakezelő / heap** – `kmalloc`/`kfree` a `pmm_alloc_frame()`
   keretei fölé építve, page table módosítás futásidőben.
3. ✅ **Shell + RAM fájlrendszer** – sor-alapú bevitel, könyvtárak és fájlok.
   Parancsok: `help`, `clear`, `pwd`, `ls`, `cd`, `mkdir`, `touch`, `cat`,
   `write`, `rm`, `meminfo`. A RAMFS újraindításkor törlődik.
4. **PIT időzítő (IRQ0)** – rendszeróra, alvás (`sleep`), később ütemező alap.
5. **Perzisztens fájlrendszer** – lemezvezérlő + blokkeszköz réteg után FAT32
   vagy ext2 olvasás/írás.
6. **Felhasználói módú folyamatok** – ring 3, szegmens/lapozás elkülönítés,
   rendszerhívások (syscall/sysret).
7. **Multitasking** – ütemező, kontextusváltás.

Ez egy több hetes-hónapos projekt szokott lenni hobby szinten – érdemes az
OSDev Wiki-t (osdev.org/wiki) kézikönyvként használni minden egyes lépéshez,
mert rendkívül részletes és naprakész referencia x86_64 fejlesztéshez.

## Megjegyzés

Ezt a kódot nem tudtam a jelen környezetben lefordítani/tesztelni (nincs
hálózati/cross-compiler hozzáférés a sandboxban), úgyhogy a saját gépeden
buildeld és QEMU-ban ellenőrizd. Ha hibába futsz fordításnál, másold be ide
a hibaüzenetet, és továbbsegítlek.
