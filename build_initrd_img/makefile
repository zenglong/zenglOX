#makefile for ram disk

CC = @gcc

CFLAGS = -std=gnu99 -ffreestanding -gdwarf-2 -g3 -Wall -Wextra
DEPS = common.h syscall.h elf.h task.h kheap.h fs.h ata.h fdisk.h format.h vga.h network.h pci.h
AS_TARGET = cpuid uname reboot shutdown
C_TARGET = shell ls ps cat ata mount unmount testoverflow fdisk format file vga lspci isoget
C_NET_TARGET = arp dhcp ipconf ping
C_EXTRA_TARGET = extra/output/ee extra/output/libc.so extra/output/zengl
C_DEP_LIB = ld.so libcommon.so
C_DEP_NET_LIB = libnetwork.so
INITRD_FILES = licence.txt $(AS_TARGET) $(C_TARGET) $(C_NET_TARGET) $(C_DEP_LIB) $(C_DEP_NET_LIB)

LIBC_SRCS = standard/libc.c standard/printf.c standard/vsnprintf.c standard/fcvt.c standard/modf.c

ZENGL_SRCS = extra/zengl/linux/zengl_main.c extra/zengl/linux/zengl_parser.c extra/zengl/linux/zengl_symbol.c extra/zengl/linux/zengl_locals.c extra/zengl/linux/zengl_assemble.c extra/zengl/linux/zengl_ld.c extra/zengl/linux/zenglrun_main.c extra/zengl/linux/zenglrun_func.c extra/zengl/linux/zenglApi.c extra/zengl/linux/zenglApi_BltModFuns.c extra/zengl/linux/zenglDebug.c

initrd.img:make_initrd.c $(INITRD_FILES) $(C_EXTRA_TARGET)
	@echo 'building initrd.img'
	$(CC) -o make_initrd make_initrd.c $(CFLAGS)
	./make_initrd $(INITRD_FILES)

define AS_make_template
# translation 
$1: $1.o
	@echo "building $$@"
	$(CROSS_LD) -o $$@ $$<
endef

$(foreach l, $(AS_TARGET), \
  $(eval $(call AS_make_template,$(l))) \
)

define C_make_template
# translation 
$1: $1.o $(C_DEP_LIB)
	@echo "building $$@"
	$(CROSS_CC) -Wl,-emain -Wl,-dynamic-linker,ld.so -o $$@ $$< $(CROSS_CLINK_FLAGS) -L. -lcommon
endef

define C_NET_make_template
# translation 
$1: $1.o $(C_DEP_LIB) $(C_DEP_NET_LIB)
	@echo "building $$@"
	$(CROSS_CC) -Wl,-emain -Wl,-dynamic-linker,ld.so -o $$@ $$< $(CROSS_CLINK_FLAGS) -L. -lcommon -lnetwork
endef

$(foreach l, $(C_TARGET), \
  $(eval $(call C_make_template,$(l))) \
)

$(foreach l, $(C_NET_TARGET), \
  $(eval $(call C_NET_make_template,$(l))) \
)

extra/output/ee: extra/ee.c standard/include/stdlib.h common.h syscall.h task.h fs.h kheap.h extra/output/libc.so
	@echo "building $@"
	@mkdir -p extra/output
	$(CROSS_CC) -Wl,-emain -Wl,-dynamic-linker,ld.so -o extra/output/ee extra/ee.c $(CROSS_CLINK_FLAGS) -I standard/include -I. -L. -lcommon -L extra/output -lc

extra/output/libc.so: $(LIBC_SRCS) standard/include/stdlib.h common.h syscall.h task.h fs.h kheap.h
	@echo "building $@"
	@mkdir -p extra/output
	$(CROSS_CC) -fPIC -Wl,-shared -Wl,-soname,hd/lib/libc.so -o extra/output/libc.so $(LIBC_SRCS) $(CROSS_CLINK_FLAGS) -I standard/include -I. -L. -lcommon

extra/output/zengl: extra/zengl/zenglOX/zengl.c $(ZENGL_SRCS) libcommon.so extra/output/libc.so
	@echo "building $@"
	@mkdir -p extra/output
	@(cd extra/zengl/zenglOX; make all)
	@cp -rvf extra/zengl/zenglOX/zengl extra/output/

libcommon.so:common.o syscall.c
	@echo "building $@"
	$(CROSS_CC) -Wl,-shared -o $@ common.o syscall.c $(CROSS_CLINK_FLAGS)

libnetwork.so:network.c network.h common.h libcommon.so
	@echo "building $@"
	$(CROSS_CC) -fPIC -Wl,-shared -o $@ network.c $(CROSS_CLINK_FLAGS) -L. -lcommon

ld.so:ld.c ld.s common.h syscall.h elf.h task.h libcommon.so
	@echo "building $@"
	$(CROSS_CC) -fPIC -Wl,-shared -Wl,-edl_main -o $@ ld.c ld.s $(CROSS_CLINK_FLAGS) -L. -lcommon

%.o: %.s
	@echo "building $@"
	$(CROSS_AS) -o $@ $< $(CROSS_AS_FLAGS)

%.o: %.c $(DEPS)
	@echo "building $@"
	$(CROSS_CC) -fPIC -c -o $@ $< $(CROSS_CFLAGS)

clean:
	$(RM) $(RMFLAGS) *.o
	$(RM) $(RMFLAGS) $(AS_TARGET) $(C_TARGET) $(C_NET_TARGET) $(C_DEP_LIB) $(C_DEP_NET_LIB)
	$(RM) $(RMFLAGS) extra/output
	$(RM) $(RMFLAGS) make_initrd
	$(RM) $(RMFLAGS) initrd.img
	@(cd extra/zengl/zenglOX; make clean)

all: initrd.img

