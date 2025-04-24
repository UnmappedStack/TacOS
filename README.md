# TacOS
My from-scratch OS with it's own kernel written in C and assembly

TacOS is a UNIX-like kernel which is able to run DOOM, among various other smaller userspace programs. It has things like a VFS, scheduler, TempFS, devices, context switching, virtual memory management, physical page frame allocation, and a port of Doom. It runs both on real hardware (tested on my laptop) and in the Qemu emulator.

![A screenshot of TacOS's shell](/screenshots/screenshot1.webp)
![A screenshot of TacOS running DOOM](/screenshots/screenshot2.webp)

Please note that TacOS is a hobby toy OS and is not complete enough for real usage. It has multiple known bugs.

I have a Discord server for PotatOS where I will share most updates, and you can also get help with your own OSDev project or just have a chat. You can join [here](https://discord.gg/hPg9S2F2nD).

## Quickstart
To build and run TacOS, simply run in your shell:
```
git clone https://github.com/UnmappedStack/TacOS
cd TacOS
git clone https://github.com/limine-bootloader/limine
cd limine
git checkout v9.x-binary
cd ..
make
```
You'll need to have Qemu, NASM, and Clang installed. It will automatically run in the Qemu emulator.

## License
TacOS is under the Mozilla Public License 2.0. See `LICENSE` for more information.
