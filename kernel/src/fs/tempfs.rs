use core::fmt::Write;
use crate::println;
use alloc::{boxed::Box, string::String, vec::Vec};

enum InodeContents {
    File(Box<File>),
    Dir(Box<Dir>),
}

struct Inode {
    fname: String,
    contents: InodeContents,
}

pub struct File {
    contents: Vec<i8>,
}

pub struct Dir {
    entries: Vec<Inode>,
}

pub struct TempFS {
    rootdir: Dir, 
}

pub fn new() -> TempFS {
    let rootdir = Dir {
        entries: Vec::<Inode>::new(),
    };
    let ret = TempFS { rootdir };
    println!("Initiated new empty TempFS");
    ret
}

pub fn openroot(superblock: &mut TempFS) -> &mut Dir {
    &mut superblock.rootdir
}

pub fn mkfile(dir: &mut Dir, fname: &str) {
    dir.entries.push(Inode {
        fname: String::from(fname),
        contents: InodeContents::File(
            Box::new(File {
                contents: Vec::<i8>::new(),
            })
        ),
    });
    println!("Created TempFS file {}", fname);
}

pub fn openfile<'a>(dir: &'a mut Dir, fname: &str) -> &'a mut Box<File> {
    for entry in dir.entries.iter_mut() {
        if entry.fname != fname { continue }
        match &mut entry.contents {
            InodeContents::File(v) => return v,
            InodeContents::Dir(_)  => {
                todo!("tmpfs error handling (err: tried to open dir as file)");
            }
        }
    }
    todo!("tmpfs error handling (err: file not found)");
}

pub fn writefile(file: &mut Box<File>, buf: Vec<i8>, bytes: usize) {
    for i in 0..bytes {
        file.contents.push(buf[i]);
    }
}

pub fn readfile(file: &Box<File>, buf: &mut Vec<i8>, bytes: usize) {
    for i in 0..bytes {
        buf[i] = file.contents[i];
    }
}
