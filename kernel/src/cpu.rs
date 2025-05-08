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

pub fn enable_interrupts() {
    unsafe {
        asm!("sti");
    }
}

pub fn halt_device() -> ! {
    disable_interrupts();
    loop {
        wait_for_interrupt();
    }
}

pub fn outb(port: u16, val: u8) {
    unsafe {
        asm!("out dx, al",
            in("dx") port,
            in("al") val,
        );
    }
}

pub fn inb(port: u16) -> u8 {
    let mut ret: u8;
    unsafe {
        asm!("in al, dx",
            in("dx") port,
            out("al") ret,
        );
    }
    ret
}
