BINDIR = bin
OBJDIR = obj
SRCDIR = src

SRCS = $(wildcard $(SRCDIR)/*.c)
OBJS = $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(SRCS))

.PHONY: all
all: crt0 $(OBJS) lib

.PHONY: crt0
crt0:
	mkdir -p $(BINDIR)
	nasm -felf64 crt0.asm -o $(BINDIR)/crt0.o -g

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	@mkdir -p $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@ -static -ffreestanding -nostdlib

lib:
	ar rcs $(BINDIR)/tacoc.a $(OBJDIR)/*
