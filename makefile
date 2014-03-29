#makefile for zenglOS

GCC_PATH = /mnt/zenglOX/opt/cross/bin
AS = @$(GCC_PATH)/i586-elf-as
CC = @$(GCC_PATH)/i586-elf-gcc

#CFLAGS = -std=gnu99 -ffreestanding -O2 -Wall -Wextra
#CLINK_FLAGS = -ffreestanding -O2 -nostdlib
CFLAGS = -std=gnu99 -ffreestanding -gdwarf-2 -g3 -Wall -Wextra
CLINK_FLAGS = -ffreestanding -gdwarf-2 -g3 -nostdlib
ASFLAGS = -gdwarf-2 -g3
RM = rm
RMFLAGS = -vf

DEPS = zlox_common.h zlox_monitor.h zlox_descriptor_tables.h zlox_isr.h zlox_time.h zlox_kheap.h \
		zlox_paging.h zlox_ordered_array.h zlox_initrd.h zlox_fs.h zlox_multiboot.h 
OBJS = zlox_boot.o zlox_kernel.o zlox_common.o zlox_monitor.o zlox_descriptor_tables.o \
		zlox_gdt.o zlox_interrupt.o zlox_isr.o zlox_time.o zlox_kheap.o zlox_paging.o \
		zlox_ordered_array.o zlox_initrd.o zlox_fs.o 

INITRD_IMG = build_initrd_img/initrd.img

zenglOX.bin: $(OBJS) linker.ld $(INITRD_IMG)
	@echo 'building zenglOX.bin'
	$(CC) -T linker.ld -o zenglOX.bin $(CLINK_FLAGS) $(OBJS)

%.o: %.s
	@echo "building $@"
	$(AS) -o $@ $< $(ASFLAGS)

%.o: %.c $(DEPS)
	@echo "building $@"
	$(CC) -c -o $@ $< $(CFLAGS)

build_initrd_img/initrd.img:
	@(cd build_initrd_img; make)

clean:
	$(RM) $(RMFLAGS) *.o
	$(RM) $(RMFLAGS) zenglOX.bin
	$(RM) $(RMFLAGS) zenglOX.iso
	$(RM) $(RMFLAGS) build_initrd_img/*.o
	$(RM) $(RMFLAGS) build_initrd_img/make_initrd
	$(RM) $(RMFLAGS) build_initrd_img/initrd.img
	$(RM) $(RMFLAGS) isodir/boot/zenglOX.bin
	$(RM) $(RMFLAGS) isodir/boot/initrd.img
	$(RM) $(RMFLAGS) isodir/boot/grub/grub.cfg

all: zenglOX.bin

iso: zenglOX.bin
	cp zenglOX.bin isodir/boot/zenglOX.bin
	cp build_initrd_img/initrd.img isodir/boot/initrd.img
	cp grub.cfg isodir/boot/grub/grub.cfg
	grub-mkrescue -o zenglOX.iso isodir

