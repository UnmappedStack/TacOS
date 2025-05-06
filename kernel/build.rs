fn main() {
    let arch = std::env::var("CARGO_CFG_TARGET_ARCH").unwrap();
    // Tell cargo to pass the linker script to the linker..
    println!("cargo:rustc-link-arg=-Tlinker-{arch}.ld");
    // ..and to re-run if it changes.
    println!("cargo:rerun-if-changed=linker-{arch}.ld");
    // Also, link object files generated from assembly separate to the Rust
    // The following if how you'd do that
    //println!("cargo:rustc-link-arg=./obj/<object name>.o");
}
