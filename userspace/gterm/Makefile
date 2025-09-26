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
	@if [ ! -d "Flanterm" ]; then \
		git clone https://codeberg.org/Mintsuki/Flanterm; \
	fi
	mkdir -p $(OUTDIR)
	mkdir -p $(OBJDIR)
	gcc -c main.c -I Flanterm/src -o $(OBJDIR)/main.o -ffreestanding -I $(HEADERS) -I $(LIBWMINC) -I $(LIBMEDIAINC) -static -nostdlib -fno-stack-protector -fno-pie -g
	gcc -c Flanterm/src/flanterm.c -o $(OBJDIR)/flanterm.o -ffreestanding -I $(HEADERS) -I $(LIBWMINC) -I $(LIBMEDIAINC) -static -nostdlib -fno-stack-protector -fno-pie -g
	gcc -c Flanterm/src/flanterm_backends/fb.c -o $(OBJDIR)/flantermfb.o -ffreestanding -I $(HEADERS) -I $(LIBWMINC) -I $(LIBMEDIAINC) -static -nostdlib -fno-stack-protector -fno-pie -g
	gcc $(OBJDIR)/main.o $(OBJDIR)/flantermfb.o $(OBJDIR)/flanterm.o $(LIBWM)/libwm.o $(LIBMEDIA)/libmedia.a $(LIBC)/bin/* -o $(OUT) -static -nostdlib -g -fno-pie
