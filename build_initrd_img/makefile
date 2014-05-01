#makefile for ram disk

CC = @gcc

CFLAGS = -std=gnu99 -ffreestanding -gdwarf-2 -g3 -Wall -Wextra
INITRD_FILES = test.txt test2.txt cpuid

initrd.img:make_initrd.c $(INITRD_FILES)
	@echo 'building initrd.img'
	$(CC) -o make_initrd make_initrd.c $(CFLAGS)
	./make_initrd $(INITRD_FILES)

cpuid:cpuid.o
	@echo "building $@"
	$(CROSS_LD) -o $@ $<

cpuid.o: cpuid.s
	@echo "building $@"
	$(CROSS_AS) -o $@ $< $(CROSS_AS_FLAGS)

clean:
	$(RM) $(RMFLAGS) *.o
	$(RM) $(RMFLAGS) cpuid
	$(RM) $(RMFLAGS) make_initrd
	$(RM) $(RMFLAGS) initrd.img

all: initrd.img

