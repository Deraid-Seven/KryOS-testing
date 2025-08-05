ASM = nasm
CC = gcc
LD = ld
QEMU = qemu-system-i386

ASMFLAGS = -f elf32
CFLAGS = -m32 -c -ffreestanding -fno-builtin -fno-stack-protector
LDFLAGS = -m elf_i386 -T link.ld

KERNEL = kernel
OBJECTS = kasm.o kc.o

.PHONY: all clean run

all: $(KERNEL)

$(KERNEL): $(OBJECTS)
	$(LD) $(LDFLAGS) -o $@ $^

kasm.o: kernel.asm
	$(ASM) $(ASMFLAGS) $< -o $@

kc.o: kernel.c
	$(CC) $(CFLAGS) $< -o $@

run: $(KERNEL)
	$(QEMU) -kernel $(KERNEL)

clean:
	rm -f $(KERNEL) $(OBJECTS)