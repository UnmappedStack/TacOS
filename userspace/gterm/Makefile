OUTDIR = ../../initrd/usr/bin
OUT = ../../initrd/usr/bin/gterm
LIBC = ../../libc
HEADERS = $(LIBC)/include
OBJDIR = obj
LIBWM = ../../libs/wm
LIBWMINC = $(LIBWM)/include
LIBMEDIA = ../../libs/media
LIBMEDIAINC = $(LIBMEDIA)/include

PHONY: all
all:
	mkdir -p $(OUTDIR)
	mkdir -p $(OBJDIR)
	gcc -c main.c -o $(OBJDIR)/main.o -ffreestanding -I $(HEADERS) -I $(LIBWMINC) -I $(LIBMEDIAINC) -static -nostdlib -fno-stack-protector -fno-pie -g
	gcc $(OBJDIR)/main.o $(LIBWM)/libwm.o $(LIBMEDIA)/libmedia.a $(LIBC)/bin/* -o $(OUT) -static -nostdlib -g -fno-pie
