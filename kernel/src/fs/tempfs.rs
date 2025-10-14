use core::{fmt::Write, cell::RefCell};
use crate::{println, err::Errno, fs::vfs};
use alloc::{string::String, vec::Vec, boxed::Box, rc::Rc};

#[derive(Clone, Default, Debug)]
pub enum InodeContents {
    File(Vec<i8>),
    Dir(Vec<Inode>),
    #[default]
    Default,
}

#[derive(Clone, Default, Debug)]
pub struct Inode {
    pub fname: String,
    pub contents: Rc<RefCell<InodeContents>>,
}

#[derive(Debug)]
pub struct TempFS {
    rootdir: Inode, 
}

pub fn new() -> TempFS {
    let rootdir = Inode {
        fname: String::from("TMPFSROOT"),
        contents: Rc::new(RefCell::new(InodeContents::Dir(Vec::new()))),
    };
    let ret = TempFS { rootdir };
    println!("Initiated new empty TempFS.");
    ret
}

pub fn mkfile(dir: &mut Inode, fname: &str) -> i32 {
    let mut binding = dir.contents.borrow_mut();
    let entries = match &mut *binding {
        InodeContents::Dir(v) => v,
        _ => return -(Errno::ENOTDIR as i32),
    };
    entries.push(Inode {
        fname: String::from(fname),
        contents: Rc::new(RefCell::new(InodeContents::File(
            Vec::<i8>::new(),
        ))),
    });
    0
}

pub fn open<'a>(parent: &'a mut Inode, fname: &str)
                                -> Result<Box<dyn vfs::FsInode>, i32> {
    let binding = parent.contents.borrow();
    let entries = match &*binding{
        InodeContents::Dir(v) => v,
        _ => return Err(-(Errno::ENOTDIR as i32)),
    };
    for entry in entries.iter() {
        if entry.fname != fname { continue }
        return Ok(Box::new(entry.clone()));
    }
    Err(-(Errno::ENOENT as i32))
}

pub fn close(_f: &Inode) -> i32 {
    0
}

pub fn writefile(file: &mut Inode, buf: Vec<i8>,
                                        offset: usize, bytes: usize) -> isize {
    let mut binding = file.contents.borrow_mut();
    let contents = match &mut *binding {
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
    let binding = file.contents.borrow();
    let contents = match &*binding{
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
    let mut binding = parent.contents.borrow_mut();
    let entries = match &mut *binding {
        InodeContents::Dir(v) => v,
        _ => return -(Errno::ENOTDIR as i32),
    };
    entries.push(Inode {
        fname: String::from(dirname),
        contents: Rc::new(RefCell::new(InodeContents::Dir(Vec::new()))),
    });
    0
}

pub fn getdents(dir: &Inode, buf: &mut Vec<Box<dyn vfs::FsInode>>, max: usize)
                                                    -> isize {
    let binding = dir.contents.borrow();
    let entries = match &*binding {
        InodeContents::Dir(v) => v,
        _ => return -(Errno::ENOTDIR as isize),
    };
    let nentries = entries.len();
    for i in 0..max {
        if i >= nentries { return (i * size_of::<Inode>()) as isize }
        buf.push(Box::new(entries[i].clone()));
    }
    return (max * size_of::<Inode>()) as isize
}

impl vfs::FsInode for Inode {
    fn open(&mut self, fname: &str, flags: u32) 
                    -> Result<Box<dyn vfs::FsInode>, i32> {
        let mut ret = open(self, fname);
        if flags & (vfs::AccessFlags::O_CREAT as u32) != 0 {
            if matches!(ret, Err(e) if e == -(Errno::ENOENT as i32)) {
                // end scope to create file
            } else if flags & (vfs::AccessFlags::O_EXCL as u32) != 0 {
                return Err(-(Errno::EEXIST as i32));
            } else {
                return ret
            }
        } else {
            return ret
        }
        drop(ret);
        mkfile(self, fname);
        return open(self, fname);
    }
    fn close(&mut self) -> i32 {
        close(self)
    }
    fn mkfile(&mut self, fname: &str) -> i32 {
        mkfile(self, fname)
    }
    fn getdents(&self, buf: &mut Vec<Box<dyn vfs::FsInode>>, max: usize) -> isize {
        getdents(self, buf, max)
    }
    fn getfinfo(&self) -> vfs::InodeInfo {
        vfs::InodeInfo {
            fname: self.fname.clone(),
        }
    }
}

impl vfs::FsDrive for TempFS {
    fn open_root(&mut self) -> vfs::Inode {
        vfs::Inode {
            inner: Box::new(self.rootdir.clone())
        }
    }
}
