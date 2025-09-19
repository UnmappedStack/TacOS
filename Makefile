all: bootloader kernel libc userspace initrd disk

override XORRISO_FLAGS += \
						  -as mkisofs -b boot/limine/limine-bios-cd.bin \
						  -no-emul-boot -boot-load-size 4 -boot-info-table \
						  --efi-boot boot/limine/limine-uefi-cd.bin \
						  -efi-boot-part --efi-boot-image --protective-msdos-label \
						  sysroot -o tacos.iso

.PHONY: kernel
kernel:
	make -C kernel

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
	sudo mkdir -p /home/TacOSlibc
	sudo cp libc/bin/tacoc.a /home/TacOSlibc/tacoc.a
	sudo cp libc/bin/crt0.o /home/TacOSlibc/crt0.o
	sudo cp -r libc/include /home/TacOSlibc/include

.PHONY: initrd
initrd:
	mkdir -p sysroot/boot
	tar --create --file=sysroot/boot/initrd --format=ustar -C initrd home usr media

.PHONY: disk
disk:
	cp -v kernel/bin/tacos sysroot/boot/
	mkdir -p sysroot/boot/limine
	cp -v limine.conf limine/limine-bios.sys limine/limine-bios-cd.bin \
    	limine/limine-uefi-cd.bin sysroot/boot/limine/
	mkdir -p sysroot/EFI/BOOT
	cp -v limine/BOOTX64.EFI sysroot/EFI/BOOT/
	cp -v limine/BOOTIA32.EFI sysroot/EFI/BOOT/
	xorriso ${XORRISO_FLAGS}
	./limine/limine bios-install tacos.iso

qemu:
	qemu-system-x86_64 tacos.iso -serial stdio -no-shutdown -no-reboot \
		-monitor telnet:127.0.0.1:8000,server,nowait -d int,cpu_reset,in_asm -D log.txt -m 4G -accel kvm \
		-drive file=README.md,if=none,id=nvm \
		-device nvme,drive=nvm,serial=deadbeef \
		-trace 'pci_nvme*' 

run: all qemu

qemu-gdb:
	qemu-system-x86_64 tacos.iso -serial stdio -no-shutdown -no-reboot -monitor telnet:127.0.0.1:8000,server,nowait -d int,cpu_reset,in_asm -D log.txt -S -s 

lint: lint-signatures lint-clang-format

lint-clang-format:
	@find kernel/src/ kernel/include/ -type f \( -name '*.c' -o -name '*.h' \) -exec clang-format -i {} +

lint-signatures:
	@grep -rPn '^\s*void\s+\w+\s*\(\s*\)' kernel/src/ kernel/include/ && \
	( echo "Functions with empty parameter lists should use 'void' instead of '()'"; exit 1 ) || true

.PHONY: clean
clean:
	rm -rf kernel/obj kernel/bin
