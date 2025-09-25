OUTDIR = ../../initrd/usr/bin
OUT = ../../initrd/usr/bin/testwin
LIBC = ../../libc
HEADERS = $(LIBC)/include
OBJDIR = obj
LIBWM = ../../libs/wm
LIBWMINC = $(LIBWM)/include

PHONY: all
all:
	mkdir -p $(OUTDIR)
	mkdir -p $(OBJDIR)
	gcc -c main.c -o $(OBJDIR)/main.o -ffreestanding -I $(HEADERS) -I $(LIBWMINC) -static -nostdlib -fno-stack-protector -fno-pie -g
	gcc $(OBJDIR)/main.o $(LIBWM)/libwm.o $(LIBC)/bin/* -o $(OUT) -static -nostdlib -g -fno-pie
