set -e

echo "[BOOTLOADER] Checking if bootloader exists"
if [ ! -d "limine-binary" ]; then
    echo "[BOOTLOADER] Limine not found, cloning"
    curl -L https://github.com/Limine-Bootloader/Limine/releases/latest/download/limine-binary.tar.gz | gunzip | tar -xf -
fi
echo "[BOOTLOADER] Building bootloader"
make -C limine-binary >/dev/null

echo "[IMAGE] Setting up sysroot"
mkdir -p iso_root
mkdir -p iso_root/boot
cp bin/tacos iso_root/boot/
mkdir -p iso_root/boot/limine
cp limine.conf limine-binary/limine-bios.sys limine-binary/limine-bios-cd.bin \
    limine-binary/limine-uefi-cd.bin iso_root/boot/limine/

echo "[IMAGE] Installing bootloader onto image"
mkdir -p iso_root/EFI/BOOT
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

echo "[QEMU] Running image in qemu"
qemu-system-x86_64 image.iso -serial stdio
