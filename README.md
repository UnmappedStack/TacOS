# TacOS
My from-scratch OS with it's own kernel written in C and assembly

TacOS is a UNIX-like kernel which is able to run DOOM, among various other smaller userspace programs. It has things like a VFS, scheduler, TempFS, devices, context switching, virtual memory management, physical page frame allocation, and a port of Doom. It runs both on real hardware (tested on my laptop) and in the Qemu emulator.

![A screenshot of TacOS's shell](/screenshots/screenshot1.webp)
![A screenshot of TacOS running DOOM](/screenshots/screenshot2.webp)

Please note that TacOS is a hobby toy OS and is not complete enough for real usage. It has multiple known bugs.

## Quickstart
To build and run TacOS, simply run in your shell:
```
git clone https://github.com/UnmappedStack/TacOS
cd TacOS
make
```
You'll need to have Qemu, NASM, and Clang installed. It will automatically run in the Qemu emulator.

## License
TacOS is under the Mozilla Public License 2.0. See `LICENSE` for more information.
