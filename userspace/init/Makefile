OUTDIR = ../../initrd/usr/bin
OUT = ../../initrd/usr/bin/init
LIBC = ../../libc
HEADERS = $(LIBC)/include
OBJDIR = obj

PHONY: all
all:
	mkdir -p $(OUTDIR)
	mkdir -p $(OBJDIR)
	gcc -c main.c -o $(OBJDIR)/main.o -ffreestanding -I $(HEADERS) -static -nostdlib -g
	gcc $(OBJDIR)/main.o $(LIBC)/bin/* -o $(OUT) -static -nostdlib -g
