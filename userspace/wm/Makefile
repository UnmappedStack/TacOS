OUTDIR = ../../initrd/usr/bin
OUT = ../../initrd/usr/bin/wm
LIBC = ../../libc
HEADERS = $(LIBC)/include
OBJDIR = obj
LIBMEDIA = ../../libs/media
LIBMEDIAINC = $(LIBMEDIA)/include

PHONY: all
all:
	mkdir -p $(OUTDIR)
	mkdir -p $(OBJDIR)
	gcc -c main.c -o $(OBJDIR)/main.o -ffreestanding -I $(HEADERS) -I $(LIBMEDIAINC) -static -nostdlib -fno-stack-protector -fno-pie -g -mno-tls-direct-seg-refs
	gcc $(OBJDIR)/main.o $(LIBMEDIA)/libmedia.a $(LIBC)/bin/* -o $(OUT) -static -nostdlib -g -fno-pie 
