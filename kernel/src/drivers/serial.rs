use crate::cpu;
use core::fmt::{self, Write};

const COM1: u16 = 0x3f8;

pub fn init_serial() {
    cpu::outb(COM1 + 3, 0x80);
    cpu::outb(COM1 + 0, 0x03);
    cpu::outb(COM1 + 1, 0x00);
    cpu::outb(COM1 + 3, 0x03);
    cpu::outb(COM1 + 2, 0xC7);
    cpu::outb(COM1 + 4, 0x0B);
    cpu::outb(COM1 + 4, 0x1E);
    cpu::outb(COM1 + 0, 0xAE);
    if cpu::inb(COM1 + 0) != 0xAE { return }
    cpu::outb(COM1 + 4, 0x0F);
}

fn is_transmit_empty() -> bool {
    (cpu::inb(COM1 + 5) & 0x20) != 0
}

pub fn serial_writechar(ch: char) {
    while !is_transmit_empty() {}
    cpu::outb(COM1, ch as u8);
}

pub fn serial_writestring(s: &str) {
    for c in s.chars() {
        serial_writechar(c);
    }
}

pub struct SerialWriter {}

impl Write for SerialWriter {
    fn write_str(&mut self, s: &str) -> fmt::Result {
        serial_writestring(s);
        Ok(())
    }
}

#[macro_export] macro_rules! println {
    ($($arg:tt)*) => {
        let mut writer = crate::serial::SerialWriter {};
        writer.write_fmt(format_args!($($arg)*)).unwrap();
        crate::serial::serial_writechar('\n');
    };
}
