# Nuke built-in rules and variables.
MAKEFLAGS += -rR
.SUFFIXES:

# This is the name that our final executable will have.
# Change as needed.
override OUTPUT := tacos

# Convenience macro to reliably declare user overridable variables.
override USER_VARIABLE = $(if $(filter $(origin $(1)),default undefined),$(eval override $(1) := $(2)))

override XORRISO_FLAGS += \
						  -as mkisofs -b boot/limine/limine-bios-cd.bin \
						  -no-emul-boot -boot-load-size 4 -boot-info-table \
						  --efi-boot boot/limine/limine-uefi-cd.bin \
						  -efi-boot-part --efi-boot-image --protective-msdos-label \
						  sysroot -o tacos.iso

# User controllable C compiler command.
$(call USER_VARIABLE,KCC,clang)

# User controllable linker command.
$(call USER_VARIABLE,KLD,ld)

# User controllable C flags.
$(call USER_VARIABLE,KCFLAGS,-g -pipe)

# User controllable C preprocessor flags. We set none by default.
$(call USER_VARIABLE,KCPPFLAGS,)

# User controllable nasm flags.
$(call USER_VARIABLE,KNASMFLAGS,-F dwarf -g)

# User controllable linker flags. We set none by default.
$(call USER_VARIABLE,KLDFLAGS,)

all: bootloader kernel libc userspace initrd disk

bootloader:
	make -C limine

USER_PROGRAMS := $(wildcard userspace/*)
.PHONY: userspace $(USER_PROGRAMS)
userspace: $(USER_PROGRAMS)
$(USER_PROGRAMS):
	$(MAKE) -C $@

.PHONY: libc
libc:
	make -C libc

.PHONY: initrd
initrd:
	mkdir -p sysroot/boot
	tar --create --file=sysroot/boot/initrd --format=ustar -C initrd home usr

.PHONY: disk
disk:
	cp -v bin/tacos sysroot/boot/
	mkdir -p sysroot/boot/limine
	cp -v limine.conf limine/limine-bios.sys limine/limine-bios-cd.bin \
    	limine/limine-uefi-cd.bin sysroot/boot/limine/
	mkdir -p sysroot/EFI/BOOT
	cp -v limine/BOOTX64.EFI sysroot/EFI/BOOT/
	cp -v limine/BOOTIA32.EFI sysroot/EFI/BOOT/
	xorriso ${XORRISO_FLAGS}
	./limine/limine bios-install tacos.iso

qemu:
	qemu-system-x86_64 tacos.iso -serial stdio -no-shutdown -no-reboot -monitor telnet:127.0.0.1:8000,server,nowait -d int,cpu_reset,in_asm -D log.txt -m 4G -accel kvm

run: all qemu

qemu-gdb:
	qemu-system-x86_64 tacos.iso -serial stdio -no-shutdown -no-reboot -monitor telnet:127.0.0.1:8000,server,nowait -d int,cpu_reset,in_asm -D log.txt -S -s 

# everything below here is the kernel

# Internal C flags that should not be changed by the user.
override KCFLAGS += \
    -Wall \
    -Wextra \
	-Werror \
	-std=c23 \
	-ffreestanding \
    -fno-stack-protector \
    -fno-stack-check \
    -fPIE \
    -m64 \
    -march=x86-64 \
    -mno-80387 \
    -mno-mmx \
	-g \
    -mno-sse \
    -mno-sse2 \
	-fno-omit-frame-pointer \
    -mno-red-zone \
	-O0 \
	-mcmodel=kernel

# Internal C preprocessor flags that should not be changed by the user.
override KCPPFLAGS := \
    -I include \
    $(KCPPFLAGS) \
    -MMD \
    -MP

# Internal nasm flags that should not be changed by the user.
override KNASMFLAGS += \
    -Wall \
    -f elf64

# Internal linker flags that should not be changed by the user.
override KLDFLAGS += \
    -m elf_x86_64 \
    -nostdlib \
    -static \
    -z max-page-size=0x1000 \
    -T linker.ld

# Use "find" to glob all *.c, *.S, and *.asm files in the tree and obtain the
# object and header dependency file names.
override CFILES := $(shell cd src && find -L * -type f -name '*.c' | LC_ALL=C sort)
override ASFILES := $(shell cd src && find -L * -type f -name '*.S' | LC_ALL=C sort)
override NASMFILES := $(shell cd src && find -L * -type f -name '*.asm' | LC_ALL=C sort)
override OBJ := $(addprefix obj/,$(CFILES:.c=.c.o) $(ASFILES:.S=.S.o) $(NASMFILES:.asm=.asm.o))
override HEADER_DEPS := $(addprefix obj/,$(CFILES:.c=.c.d) $(ASFILES:.S=.S.d))

# Default target.
.PHONY: kernel
kernel: bin/$(OUTPUT)

# Link rules for the final executable.
bin/$(OUTPUT): linker.ld $(OBJ)
	mkdir -p "$$(dirname $@)"
	$(KLD) $(OBJ) $(KLDFLAGS) -o $@

# Include header dependencies.
-include $(HEADER_DEPS)

# Compilation rules for *.c files.
obj/%.c.o: src/%.c
	mkdir -p "$$(dirname $@)"
	$(KCC) $(KCFLAGS) $(KCPPFLAGS) -c $< -o $@

# Compilation rules for *.S files.
obj/%.S.o: src/%.S 
	mkdir -p "$$(dirname $@)"
	$(KCC) $(KCFLAGS) $(KCPPFLAGS) -c $< -o $@

# Compilation rules for *.asm (nasm) files.
obj/%.asm.o: src/%.asm
	mkdir -p "$$(dirname $@)"
	nasm $(KNASMFLAGS) $< -o $@

# Remove object files and the final executable.
.PHONY: clean
clean:
	rm -rf obj bin
