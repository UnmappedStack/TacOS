use core::fmt::Write;
use crate::println;
use alloc::{string::String, vec::Vec};

#[derive(Clone, Default)]
pub enum InodeContents {
    File(Vec<i8>),
    Dir(Vec<Inode>),
    #[default]
    Default,
}

#[derive(Clone, Default)]
pub struct Inode {
    pub fname: String,
    pub contents: InodeContents,
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
            _  => {
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

pub fn closedir(f: &Inode) -> i32 {
    match f.contents {
        InodeContents::Dir(_) => 0,
        _ => todo!("tmpfs error handling (err: tried to close file as dir)"),
    }
}

pub fn writefile(file: &mut Inode, buf: Vec<i8>,
                                        offset: usize, bytes: usize) -> isize {
    let contents = match &mut file.contents {
        InodeContents::File(v) => v,
        _ => todo!("tmpfs error handling (err: tried to write to file as dir)"),
    };
    for i in 0..bytes {
        if offset+i >= contents.len() {
            contents.push(buf[i]);
        } else {
            contents[offset+i] = buf[i];
        }
    }
    bytes as isize
}

pub fn readfile(file: &Inode, buf: &mut Vec<i8>,
                                        offset: usize, bytes: usize) -> isize {
    let contents = match &file.contents {
        InodeContents::File(v) => v,
        _ => todo!("tmpfs error handling (err: tried to write to file as dir)"),
    };
    let len = contents.len();
    for i in 0..bytes {
        if offset + i >= len { return i as isize }
        buf[i] = contents[offset + i];
    }
    bytes as isize
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

pub fn opendir<'a>(parent: &'a mut Inode, dirname: &str) -> &'a mut Inode {
    let entries = match &mut parent.contents {
        InodeContents::Dir(v) => v,
        _ => todo!("tmpfs error handling (err: expected dir got file in open)"),
    };
    for entry in entries.iter_mut() {
        if entry.fname != dirname { continue }
        match entry.contents {
            InodeContents::Dir(_) => return entry,
            _ => todo!("tmpfs error handling (err: tried to open file as dir)"),
        }
    }
    todo!("tmpfs error handling (err: directory does not exist)");
}

pub fn getdents(dir: &Inode, buf: &mut Vec<Inode>, max: usize) {
    let entries = match &dir.contents {
        InodeContents::Dir(v) => v,
        _ => todo!("tmpfs error handling (err: tried to get dents of file)"),
    };
    let nentries = entries.len();
    for i in 0..max {
        if i >= nentries { break }
        buf[i] = entries[i].clone();
    }
}
