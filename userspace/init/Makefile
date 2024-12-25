OUTDIR = ../../initrd/lib/bin
OUT = ../../initrd/lib/bin/init
OBJDIR = obj

all:
	$(shell mkdir -p $(OUTDIR))
	$(shell mkdir $(OBJDIR))
	nasm -f elf64 main.asm -o $(OBJDIR)/main.o
	ld $(OBJDIR)/main.o -o  $(OUT)
