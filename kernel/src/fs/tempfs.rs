use core::fmt::Write;
use crate::println;
use alloc::{string::String, vec::Vec};

enum InodeContents {
    File(Vec<i8>),
    Dir(Vec<Inode>),
}

pub struct Inode {
    fname: String,
    contents: InodeContents,
}

pub struct TempFS {
    rootdir: Inode, 
}

pub fn new() -> TempFS {
    let rootdir = Inode {
        fname: String::from("TMPFSROOT"),
        contents: InodeContents::Dir(Vec::new()),
    };
    let ret = TempFS { rootdir };
    println!("Initiated new empty TempFS");
    ret
}

pub fn openroot(superblock: &mut TempFS) -> &mut Inode {
    &mut superblock.rootdir
}

pub fn mkfile(dir: &mut Inode, fname: &str) {
    let entries = match &mut dir.contents {
        InodeContents::Dir(v) => v,
        _ => todo!("tmpfs error handling (err: mkfile dir is a file)"),
    };
    entries.push(Inode {
        fname: String::from(fname),
        contents: InodeContents::File(
            Vec::<i8>::new(),
        ),
    });
    println!("Created TempFS file {}", fname);
}

pub fn openfile<'a>(dir: &'a mut Inode, fname: &str) -> &'a mut Inode {
    let entries = match &mut dir.contents {
        InodeContents::Dir(v) => v,
        _ => todo!("tmpfs error handling (err: expected dir got file in open)"),
    };
    for entry in entries.iter_mut() {
        if entry.fname != fname { continue }
        match &mut entry.contents {
            InodeContents::File(_) => return entry,
            InodeContents::Dir(_)  => {
                todo!("tmpfs error handling (err: tried to open dir as file)");
            }
        }
    }
    todo!("tmpfs error handling (err: file not found)");
}

pub fn closefile(f: &Inode) -> i32 {
    match f.contents {
        InodeContents::File(_) => 0,
        _ => todo!("tmpfs error handling (err: tried to close dir as file)"),
    }
}

pub fn writefile(file: &mut Inode, buf: Vec<i8>, bytes: usize) {
    let contents = match &mut file.contents {
        InodeContents::File(v) => v,
        _ => todo!("tmpfs error handling (err: tried to write to file as dir)"),
    };
    for i in 0..bytes {
        contents.push(buf[i]);
    }
}

pub fn readfile(file: &Inode, buf: &mut Vec<i8>, bytes: usize) {
    let contents = match &file.contents {
        InodeContents::File(v) => v,
        _ => todo!("tmpfs error handling (err: tried to write to file as dir)"),
    };
    for i in 0..bytes {
        buf[i] = contents[i];
    }
}

pub fn mkdir(parent: &mut Inode, dirname: &str) {
    let entries = match &mut parent.contents {
        InodeContents::Dir(v) => v,
        _ => todo!("tmpfs error handling (err: expected dir got file in open)"),
    };
    entries.push(Inode {
        fname: String::from(dirname),
        contents: InodeContents::Dir(Vec::new()),
    });
}

