BINDIR = bin
OBJDIR = obj

.PHONY: crt0
crt0:
	mkdir -p $(BINDIR)
	nasm -felf64 crt0.asm -o $(BINDIR)/crt0.o -g
