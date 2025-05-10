# TacOS
My from-scratch OS with its own kernel written in Rust and Assembly

> [!IMPORTANT]
> This branch is the Rust rewrite of TacOS. It can not yet do things that the main C branch can do, such as running Doom. If you want a system
> which actually does stuff, switch to main and follow the build instructions there.

TacOS is a UNIX-like kernel which is able to run DOOM, among various other smaller userspace programs. It has things like a VFS, scheduler, TempFS, devices, context switching, virtual memory management, physical page frame allocation, and a port of Doom. It runs both on real hardware (tested on my laptop) and in the Qemu emulator.

![A screenshot of TacOS's shell](/screenshots/screenshot1.webp)
![A screenshot of TacOS running DOOM](/screenshots/screenshot2.webp)

Please note that TacOS is a hobby toy OS and is not complete enough for real usage. It has multiple known bugs.

I have a Discord server for TacOS where I will share most updates, and you can also get help with your own OSDev project or just have a chat. You can join [here](https://discord.gg/hPg9S2F2nD).

## Quickstart
To build and run TacOS, simply run in your shell:
```
git clone https://github.com/UnmappedStack/TacOS
cd TacOS
make run
```
You'll need to have Qemu, GCC, GNU Make, and rustup installed. It will automatically run in the Qemu emulator.

## License
TacOS is under the Mozilla Public License 2.0. See `LICENSE` for more information.
