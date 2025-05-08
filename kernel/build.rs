fn main() {
    let arch = std::env::var("CARGO_CFG_TARGET_ARCH").unwrap();
    // Tell cargo to pass the linker script to the linker..
    println!("cargo:rustc-link-arg=-Tlinker-{arch}.ld");
    // ..and to re-run if it changes.
    println!("cargo:rerun-if-changed=linker-{arch}.ld");
    // link needed objects from assembly files
    println!("cargo:rustc-link-arg=./obj/gdt.o");
    println!("cargo:rustc-link-arg=./obj/exceptions.o");
}
