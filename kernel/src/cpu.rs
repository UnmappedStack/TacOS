use core::arch::asm;

pub fn wait_for_interrupt() {
    unsafe {
        asm!("hlt");
    }
}

pub fn disable_interrupts() {
    unsafe {
        asm!("cli");
    }
}

pub fn halt_device() -> ! {
    disable_interrupts();
    loop {
        wait_for_interrupt();
    }
}

pub unsafe fn outb(port: u16, val: u8) {
    unsafe {
        asm!("out dx, al",
            in("dx") port,
            in("al") val,
        );
    }
}

pub unsafe fn inb(port: u16) -> u8 {
    let mut ret: u8;
    unsafe {
        asm!("in al, dx",
            in("dx") port,
            out("al") ret,
        );
    }
    ret
}

pub unsafe fn inw(port: u16) -> u16 {
    let mut ret: u16;
    unsafe {
        asm!("in ax, dx",
            in("dx") port,
            out("ax") ret,
        );
    }
    ret
}

pub unsafe fn outw(port: u16, val: u16) {
    unsafe {
        asm!("out dx, ax",
            in("dx") port,
            in("ax") val,
        );
    }
}

pub unsafe fn ind(port: u16) -> u32 {
    let mut ret: u32;
    unsafe {
        asm!("in eax, dx",
            in("dx") port,
            out("eax") ret,
        );
    }
    ret
}

pub unsafe fn outd(port: u16, val: u32) {
    unsafe {
        asm!("out dx, eax",
            in("dx") port,
            in("eax") val,
        );
    }
}
