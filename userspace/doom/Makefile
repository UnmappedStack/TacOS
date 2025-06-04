OUTDIR = ../../initrd/usr/bin
OUT = ../../initrd/usr/bin/doom
LIBC = ../../libc
HEADERS = $(LIBC)/include
OBJDIR = obj

PHONY: all
all:
	if [ ! -d "tacos-doomgeneric" ]; then \
		git clone https://github.com/UnmappedStack/tacos-doomgeneric --depth=1; \
	else \
		echo "tacos-doomgeneric already exists, skipping clone."; \
	fi
	cd tacos-doomgeneric/doomgeneric && pwd && make -f Makefile.tacos
	mv tacos-doomgeneric/doomgeneric/doomgeneric $(OUT)

clean:
	rm -rf tacos-doomgeneric
