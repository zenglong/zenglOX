#makefile for ram disk

CC = @gcc

CFLAGS = -std=gnu99 -ffreestanding -gdwarf-2 -g3 -Wall -Wextra
DEPS = common.h syscall.h elf.h task.h kheap.h fs.h ata.h
AS_TARGET = cpuid uname reboot shutdown
C_TARGET = shell ls ps cat ata mount unmount testoverflow
C_DEP_LIB = ld.so libcommon.so
INITRD_FILES = licence.txt $(AS_TARGET) $(C_TARGET) $(C_DEP_LIB)

initrd.img:make_initrd.c $(INITRD_FILES)
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

$(foreach l, $(C_TARGET), \
  $(eval $(call C_make_template,$(l))) \
)

libcommon.so:common.o syscall.c
	@echo "building $@"
	$(CROSS_CC) -Wl,-shared -o $@ common.o syscall.c $(CROSS_CLINK_FLAGS)

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
	$(RM) $(RMFLAGS) $(AS_TARGET) $(C_TARGET) $(C_DEP_LIB)
	$(RM) $(RMFLAGS) make_initrd
	$(RM) $(RMFLAGS) initrd.img

all: initrd.img

