# TacOS
My from-scratch OS with its own kernel written in C and assembly

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
git clone https://github.com/limine-bootloader/limine
cd limine
git checkout v9.x-binary
cd ..
make run
```
You'll need to have Qemu, NASM, and Clang installed. It will automatically run in the Qemu emulator.

The build system has a few primary commands which you can use:
 - `make run`: Builds TacOS and runs it in Qemu
 - `make qemu`: Runs TacOS in Qemu, assuming it's already built (see below command)
 - `make disk`: Builds the full disk image of TacOS as `tacos.iso`
 - `make kernel`: Builds the TacOS kernel, the core of the system
 - `make libc`: Build the standard library
 - `make userspace`: Builds the userspace applications
 - `make initrd`: Creates the initial ramdisk that the system will boot from
 - `make lint`: Runs linter rules on the kernel
 - `make qemu-gdb`: Runs TacOS in Qemu with GDB attached (see below)

## Debugging TacOS
To attach GDB when testing TacOS, run `make qemu-gdb`, then in another terminal, run the following:
```
$ gdb -q
(gdb) target remote :1234
Remote debugging using :1234
warning: No executable has been specified and target does not support
determining executable automatically.  Try using the "file" command.
0x000000000000fff0 in ?? ()
(gdb) file kernel/bin/tacos # if you're debugging the kernel
(gdb) file initrd/usr/bin/<program> # if you're debugging a userspace program
(gdb) continue
```

## License & Contributions
TacOS is under the Mozilla Public License 2.0. See `LICENSE` for more information.

I'm open to contributions, however I have some simple ground rules and conventions:
 - Please open an [issue](https://github.com/UnmappedStack/TacOS/issues) before opening a pull request, and wait for me to assign you the change.
 - I ***will not*** merge any pull requests with a simple typo or grammar mistake! Open an issue if it is a problem.
 - Commit messages should be in the following format: `[component] change`, where `component` is the general area of the project you have changed.
 - Break down your commits and pull requests. I will not even bother reviewing a huge pull request with one single massive commit with thousands of lines changed in completely unrelated components.
 - Before staging your commits, run `make lint` to run the linter+format checker on your code, and modify accordingly. Do not make any modifications to the linter rules.
