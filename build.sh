#!/bin/bash
set -e

if [ ! -d "limine" ]; then
    git clone https://github.com/limine-bootloader/limine.git --branch=v8.x-binary --depth=1
fi

# compile the bootloader & the kernel

echo "Building bootloader..."
make -C limine

echo "Building kernel..."
make

echo "Building userspace components..."
make -C userspace/*

echo "Building initial ramdisk..."
mkdir -p sysroot/boot
tar --create --file=sysroot/boot/initrd --format=ustar -C initrd home usr

echo "Building disk image..."
cp -v bin/tacos sysroot/boot/
mkdir -p sysroot/boot/limine
cp -v limine.conf limine/limine-bios.sys limine/limine-bios-cd.bin \
      limine/limine-uefi-cd.bin sysroot/boot/limine/

mkdir -p sysroot/EFI/BOOT
cp -v limine/BOOTX64.EFI sysroot/EFI/BOOT/
cp -v limine/BOOTIA32.EFI sysroot/EFI/BOOT/

xorriso -as mkisofs -b boot/limine/limine-bios-cd.bin \
        -no-emul-boot -boot-load-size 4 -boot-info-table \
        --efi-boot boot/limine/limine-uefi-cd.bin \
        -efi-boot-part --efi-boot-image --protective-msdos-label \
        sysroot -o tacos.iso

./limine/limine bios-install tacos.iso

# run in qemu

qemu-system-x86_64 tacos.iso --serial stdio --no-reboot --no-shutdown -accel kvm -monitor telnet:127.0.0.1:8000,server,nowait -d int,cpu_reset -D log.txt

