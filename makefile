#makefile for zenglOS

GCC_PATH = /mnt/zenglOX/opt/cross/bin
AS = $(GCC_PATH)/i586-elf-as
CC = $(GCC_PATH)/i586-elf-gcc

#CFLAGS = -std=gnu99 -ffreestanding -O2 -Wall -Wextra
#CLINK_FLAGS = -ffreestanding -O2 -nostdlib
CFLAGS = -std=gnu99 -ffreestanding -gdwarf-2 -g3 -Wall -Wextra
CLINK_FLAGS = -ffreestanding -gdwarf-2 -g3 -nostdlib
ASFLAGS = -gdwarf-2 -g3

DEPS = zlox_common.h zlox_monitor.h
OBJS = zlox_boot.o zlox_kernel.o zlox_common.o zlox_monitor.o

zenglOX.bin: $(OBJS) linker.ld
	$(CC) -T linker.ld -o zenglOX.bin $(CLINK_FLAGS) $(OBJS)

zlox_boot.o: zlox_boot.s
	$(AS) zlox_boot.s -o zlox_boot.o $(ASFLAGS)

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

clean:
	rm -vf *.o
	rm -vf zenglOX.bin
	rm -vf zenglOX.iso
	rm -vf isodir/boot/zenglOX.bin
	rm -vf isodir/boot/grub/grub.cfg

all: zenglOX.bin

iso: zenglOX.bin
	cp zenglOX.bin isodir/boot/zenglOX.bin
	cp grub.cfg isodir/boot/grub/grub.cfg
	grub-mkrescue -o zenglOX.iso isodir
