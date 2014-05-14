#makefile for ram disk

CC = @gcc

CFLAGS = -std=gnu99 -ffreestanding -gdwarf-2 -g3 -Wall -Wextra
OBJS = cpuid.o uname.o shell.o syscall.o entry.o common.o ls.o ps.o cat.o reboot.o shutdown.o
INITRD_FILES = licence.txt cpuid uname shell ls ps cat reboot shutdown

initrd.img:make_initrd.c $(INITRD_FILES) $(OBJS)
	@echo 'building initrd.img'
	$(CC) -o make_initrd make_initrd.c $(CFLAGS)
	./make_initrd $(INITRD_FILES)

cpuid:cpuid.o
	@echo "building $@"
	$(CROSS_LD) -o $@ $<

uname:uname.o
	@echo "building $@"
	$(CROSS_LD) -o $@ $<

reboot:reboot.o
	@echo "building $@"
	$(CROSS_LD) -o $@ $<

shutdown:shutdown.o
	@echo "building $@"
	$(CROSS_LD) -o $@ $<

shell: shell.o syscall.o entry.o common.o
	@echo 'building $@'
	$(CROSS_CC) -Wl,-eentry -o $@ $(CROSS_CLINK_FLAGS) shell.o syscall.o entry.o common.o

ls: ls.o syscall.o entry.o common.o
	@echo 'building $@'
	$(CROSS_CC) -Wl,-eentry -o $@ $(CROSS_CLINK_FLAGS) ls.o syscall.o entry.o common.o

ps: ps.o syscall.o entry.o common.o
	@echo 'building $@'
	$(CROSS_CC) -Wl,-eentry -o $@ $(CROSS_CLINK_FLAGS) ps.o syscall.o entry.o common.o

cat: cat.o syscall.o entry.o common.o
	@echo 'building $@'
	$(CROSS_CC) -Wl,-eentry -o $@ $(CROSS_CLINK_FLAGS) cat.o syscall.o entry.o common.o

cpuid.o: cpuid.s
	@echo "building $@"
	$(CROSS_AS) -o $@ $< $(CROSS_AS_FLAGS)

uname.o: uname.s
	@echo "building $@"
	$(CROSS_AS) -o $@ $< $(CROSS_AS_FLAGS)

reboot.o: reboot.s
	@echo "building $@"
	$(CROSS_AS) -o $@ $< $(CROSS_AS_FLAGS)

shutdown.o: shutdown.s
	@echo "building $@"
	$(CROSS_AS) -o $@ $< $(CROSS_AS_FLAGS)

common.o: common.c common.h
	@echo "building $@"
	$(CROSS_CC) -c -o $@ $< $(CROSS_CFLAGS)

entry.o: entry.c common.h syscall.h task.h
	@echo "building $@"
	$(CROSS_CC) -c -o $@ $< $(CROSS_CFLAGS)

syscall.o: syscall.c common.h syscall.h
	@echo "building $@"
	$(CROSS_CC) -c -o $@ $< $(CROSS_CFLAGS)

shell.o: shell.c common.h syscall.h task.h
	@echo "building $@"
	$(CROSS_CC) -c -o $@ $< $(CROSS_CFLAGS)

ls.o: ls.c common.h syscall.h fs.h
	@echo "building $@"
	$(CROSS_CC) -c -o $@ $< $(CROSS_CFLAGS)

ps.o: ps.c common.h syscall.h task.h kheap.h
	@echo "building $@"
	$(CROSS_CC) -c -o $@ $< $(CROSS_CFLAGS)

cat.o: cat.c common.h syscall.h fs.h
	@echo "building $@"
	$(CROSS_CC) -c -o $@ $< $(CROSS_CFLAGS)

clean:
	$(RM) $(RMFLAGS) *.o
	$(RM) $(RMFLAGS) cpuid
	$(RM) $(RMFLAGS) uname
	$(RM) $(RMFLAGS) shell
	$(RM) $(RMFLAGS) ls
	$(RM) $(RMFLAGS) ps
	$(RM) $(RMFLAGS) cat
	$(RM) $(RMFLAGS) reboot
	$(RM) $(RMFLAGS) shutdown
	$(RM) $(RMFLAGS) make_initrd
	$(RM) $(RMFLAGS) initrd.img

all: initrd.img

