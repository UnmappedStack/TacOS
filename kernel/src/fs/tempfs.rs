use core::fmt::Write;
use crate::{println, err::Errno};
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

pub fn mkfile(dir: &mut Inode, fname: &str) -> i32 {
    let entries = match &mut dir.contents {
        InodeContents::Dir(v) => v,
        _ => return -(Errno::ENOTDIR as i32),
    };
    entries.push(Inode {
        fname: String::from(fname),
        contents: InodeContents::File(
            Vec::<i8>::new(),
        ),
    });
    println!("Created TempFS file {}", fname);
    0
}

pub fn openfile<'a>(dir: &'a mut Inode, fname: &str,
                                            buf: &'a mut &'a mut Inode) -> i32 {
    let entries = match &mut dir.contents {
        InodeContents::Dir(v) => v,
        _ => return -(Errno::EISDIR as i32),
    };
    for entry in entries.iter_mut() {
        if entry.fname != fname { continue }
        match &mut entry.contents {
            InodeContents::File(_) => {
                *buf = entry;
                return 0
            },
            _  => return -(Errno::EISDIR as i32),
        }
    }
    -(Errno::ENOENT as i32)
}

pub fn closefile(f: &Inode) -> i32 {
    match f.contents {
        InodeContents::File(_) => 0,
        _ => -(Errno::EISDIR as i32),
    }
}

pub fn closedir(f: &Inode) -> i32 {
    match f.contents {
        InodeContents::Dir(_) => 0,
        _ => -(Errno::ENOTDIR as i32),
    }
}

pub fn writefile(file: &mut Inode, buf: Vec<i8>,
                                        offset: usize, bytes: usize) -> isize {
    let contents = match &mut file.contents {
        InodeContents::File(v) => v,
        _ => return -(Errno::EISDIR as isize),
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
        _ => return -(Errno::EISDIR as isize),
    };
    let len = contents.len();
    for i in 0..bytes {
        if offset + i >= len { return i as isize }
        buf[i] = contents[offset + i];
    }
    bytes as isize
}

pub fn mkdir(parent: &mut Inode, dirname: &str) -> i32 {
    let entries = match &mut parent.contents {
        InodeContents::Dir(v) => v,
        _ => return -(Errno::ENOTDIR as i32),
    };
    entries.push(Inode {
        fname: String::from(dirname),
        contents: InodeContents::Dir(Vec::new()),
    });
    0
}

pub fn opendir<'a>(buf: &mut &'a mut Inode, parent: &'a mut Inode,
                                                        dirname: &str) -> i32 {
    let entries = match &mut parent.contents {
        InodeContents::Dir(v) => v,
        _ => return -(Errno::ENOTDIR as i32),
    };
    for entry in entries.iter_mut() {
        if entry.fname != dirname { continue }
        match entry.contents {
            InodeContents::Dir(_) => {
                *buf = entry;
                return 0
            },
            _ => return -(Errno::ENOTDIR as i32),
        }
    }
    -(Errno::ENOENT as i32)
}

pub fn getdents(dir: &Inode, buf: &mut Vec<Inode>, max: usize) -> isize {
    let entries = match &dir.contents {
        InodeContents::Dir(v) => v,
        _ => return -(Errno::ENOTDIR as isize),
    };
    let nentries = entries.len();
    for i in 0..max {
        if i >= nentries { return (i * size_of::<Inode>()) as isize }
        buf[i] = entries[i].clone();
    }
    return (max * size_of::<Inode>()) as isize
}
