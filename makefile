#makefile for zenglOS

GCC_PATH = /mnt/zenglOX/opt/cross/bin
AS = $(GCC_PATH)/i586-elf-as
CC = $(GCC_PATH)/i586-elf-gcc

#CFLAGS = -std=gnu99 -ffreestanding -O2 -Wall -Wextra
#CLINK_FLAGS = -ffreestanding -O2 -nostdlib
CFLAGS = -std=gnu99 -ffreestanding -gdwarf-2 -g3 -Wall -Wextra
CLINK_FLAGS = -ffreestanding -gdwarf-2 -g3 -nostdlib
ASFLAGS = -gdwarf-2 -g3

OBJS = zlox_boot.o zlox_kernel.o

zenglOX.bin: $(OBJS) linker.ld
	$(CC) -T linker.ld -o zenglOX.bin $(CLINK_FLAGS) $(OBJS)

zlox_boot.o: zlox_boot.s
	$(AS) zlox_boot.s -o zlox_boot.o $(ASFLAGS)

zlox_kernel.o: zlox_kernel.c
	$(CC) -c zlox_kernel.c -o zlox_kernel.o $(CFLAGS)

clean:
	rm -v zlox_boot.o
	rm -v zlox_kernel.o
	rm -v zenglOX.bin

all: zenglOX.bin

iso: zenglOX.bin
	cp zenglOX.bin isodir/boot/zenglOX.bin
	cp grub.cfg isodir/boot/grub/grub.cfg
	grub-mkrescue -o zenglOX.iso isodir
