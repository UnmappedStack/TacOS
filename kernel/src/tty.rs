use flanterm::sys::{flanterm_fb_init, flanterm_write, flanterm_context};
use crate::kernel;
use core::ptr::null_mut;
use core::fmt::Write;
use crate::println;

pub fn write(ctx: Option<*mut flanterm_context>, msg: &str) {
    // quick and dirty &str->C string conversion without the heap
    let mut buf: [i8; 128] = [0; 128];
    for c in 0..msg.len() {
        buf[c] = msg.chars().nth(c).unwrap() as i8;
    }
    unsafe {
        flanterm_write(ctx.unwrap(), buf.as_ptr(), msg.len());
    }
}

pub fn init(kernel: &mut kernel::Kernel) {
    let fb = kernel.fb.framebuffers().nth(0).unwrap();
    unsafe {
        let ctx = flanterm_fb_init(
            None, None,
            fb.addr() as *mut u32,
            fb.width() as usize, fb.height() as usize, fb.pitch() as usize,
            fb.red_mask_size(), fb.red_mask_shift(),
            fb.green_mask_size(), fb.green_mask_shift(),
            fb.blue_mask_size(), fb.blue_mask_shift(),
            null_mut(), null_mut(), null_mut(), null_mut(), 
            null_mut(), null_mut(), null_mut(), null_mut(), 
            0, 0, 1, 0, 0, 0,
        );
        kernel.tty = Some(ctx);
    }
    println!("Framebuffer TTY initialised.");
}
