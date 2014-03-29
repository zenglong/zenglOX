#makefile for ram disk

CC = @gcc

CFLAGS = -std=gnu99 -ffreestanding -gdwarf-2 -g3 -Wall -Wextra
INITRD_FILES = test.txt test2.txt

initrd.img:make_initrd.c $(INITRD_FILES)
	@echo 'building initrd.img'
	$(CC) -o make_initrd make_initrd.c $(CFLAGS)
	./make_initrd $(INITRD_FILES)

