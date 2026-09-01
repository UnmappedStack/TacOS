# TODO: This should probably be a makefile or nob or something idk
# We assume that arg 1 is the architecture.

set -e

echo "[BOOTLOADER] Checking if bootloader exists"
if [ ! -d "limine-binary" ]; then
    echo "[BOOTLOADER] Limine not found, cloning"
    curl -L https://github.com/Limine-Bootloader/Limine/releases/latest/download/limine-binary.tar.gz | gunzip | tar -xf -
fi

echo "[FIRMWARE] Checking if firmware exists"
if [ ! -d "edk2-ovmf-bins" ]; then
    echo "[FIRMWARE] OVMF not found, cloning"
    curl -L https://github.com/osdev0/edk2-ovmf-stable-bins/releases/latest/download/edk2-ovmf-bins.tar.gz | gunzip | tar -xf -
fi

echo "[BOOTLOADER] Building bootloader"
make -C limine-binary >/dev/null

echo "[KERNEL] Building kernel"
cd kernel
cc nob.c -o nob
./nob --arch $1
cd ..

echo "[IMAGE] Setting up sysroot"
mkdir -p iso_root
mkdir -p iso_root/boot
cp kernel/bin/tacos iso_root/boot/
mkdir -p iso_root/boot/limine
cp limine.conf limine-binary/limine-bios.sys limine-binary/limine-bios-cd.bin \
    limine-binary/limine-uefi-cd.bin iso_root/boot/limine/

mkdir -p iso_root/EFI/BOOT
if [[ $1 == "x86_64" ]]; then
    echo "[IMAGE] Installing bootloader onto image"
    cp limine-binary/BOOTX64.EFI iso_root/EFI/BOOT/
    cp limine-binary/BOOTIA32.EFI iso_root/EFI/BOOT/

    echo "[IMAGE] Building image from sysroot"
    xorriso -as mkisofs -R -r -J -b boot/limine/limine-bios-cd.bin \
            -no-emul-boot -boot-load-size 4 -boot-info-table -hfsplus \
            -apm-block-size 2048 --efi-boot boot/limine/limine-uefi-cd.bin \
            -efi-boot-part --efi-boot-image --protective-msdos-label \
            iso_root -o image.iso &>/dev/null

    echo "[IMAGE] Installing legacy limine BIOS onto image"
    ./limine-binary/limine bios-install image.iso &>/dev/null
elif [[ $1 == "riscv64" ]]; then
    echo "[IMAGE] Installing bootloader onto image"
    cp -v limine-binary/limine-uefi-cd.bin iso_root/boot/limine/
	cp -v limine-binary/BOOTRISCV64.EFI iso_root/EFI/BOOT/

    echo "[IMAGE] Building image from sysroot"
    xorriso -as mkisofs -R -r -J \
    		-hfsplus -apm-block-size 2048 \
    		--efi-boot boot/limine/limine-uefi-cd.bin \
    		-efi-boot-part --efi-boot-image --protective-msdos-label \
    		iso_root -o image.iso
fi

echo "[QEMU] Running image in qemu"
if [[ $1 == "x86_64" ]]; then
    qemu-system-x86_64 image.iso -serial stdio --no-reboot --no-shutdown \
        -monitor telnet:127.0.0.1:8000,server,nowait -smp 5 --accel kvm -m 4G
elif [[ $1 == "riscv64" ]]; then
    qemu-system-riscv64 -cdrom image.iso -device ramfb -boot menu=on,splash-time=0 \
        -drive if=pflash,unit=0,format=raw,file=edk2-ovmf-bins/ovmf-code-riscv64.fd,readonly=on \
        -cpu rv64 -M virt,acpi=off -serial stdio \
        -device qemu-xhci -device usb-kbd -device usb-tablet -smp 5 \
        -monitor telnet:127.0.0.1:8000,server,nowait
fi
