BINDIR = bin
OBJDIR = obj
SRCDIR = src

CSRCS = $(wildcard $(SRCDIR)/*.c)
ASRCS = $(wildcard $(SRCDIR)/*.S)
SRCS = $(CSRCS) $(ASRCS)

OBJS = $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(CSRCS)) \
       $(patsubst $(SRCDIR)/%.S,$(OBJDIR)/%.o,$(ASRCS))

.PHONY: all
all: crt0 $(OBJS) lib

.PHONY: crt0
crt0:
	mkdir -p $(BINDIR)
	nasm -felf64 crt0.asm -o $(BINDIR)/crt0.o -g

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	@mkdir -p $(OBJDIR)
	gcc $(CFLAGS) -c $< -o $@ -static -ffreestanding -nostdlib -I include -g -fno-stack-protector -pie

$(OBJDIR)/%.o: $(SRCDIR)/%.S
	@mkdir -p $(OBJDIR)
	gcc $(CFLAGS) -c $< -o $@ -static -ffreestanding -nostdlib -I include -g -fno-stack-protector -pie

.PHONY: lib
lib:
	ar rcs $(BINDIR)/tacoc.a $(OBJDIR)/*

.PHONY: clean
clean:
	rm -r $(OBJDIR) $(BINDIR)
