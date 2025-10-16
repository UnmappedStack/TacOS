use alloc::{vec::Vec, string::String, boxed::Box};
use core::fmt::Write;
use crate::{println, kernel::*, err::*, utils};
use crate::fs::{tempfs};
use hashbrown::HashMap;

#[allow(non_camel_case_types)]
#[repr(u32)]
pub enum AccessFlags {
    O_CREAT     = 0b001,
    O_EXCL      = 0b010,
}

#[derive(Eq, Hash, PartialEq, Clone)]
pub struct Path {
    segments: Vec<String>,
}

pub struct InodeInfo {
    pub fname: String,
}

pub trait FsInode {
    /* &self is the parent dir to look for it in (fs-specific inode)
     * and it returns an Ok(fs-specific inode). */
    fn open(&mut self, fname: &str, flags: u32) -> Result<Box<dyn FsInode>, i32>;
    fn close(&mut self) -> i32;
    /* `&mut self` is the directory to open it in. The created file
     * will not be opened. */
    fn mkfile(&mut self, fname: &str) -> i32;
    fn getfinfo(&self) -> InodeInfo;
    /* getdents & read are just slightly different to POSIX on the VFS level.
     * Instead of writing to an existing buffer, they *push* to a vector. The syscall layer
     * abstracts this away. */
    fn getdents(&self, buf: &mut Vec<Box<dyn FsInode>>, max: usize) -> isize;
    fn read(&self, buf: &mut Vec<i8>, offset: usize, bytes: usize) -> isize;
    fn write(&mut self, buf: Vec<i8>, offset: usize, bytes: usize) -> isize;
}

pub struct Inode {
    pub inner: Box<dyn FsInode>,
}

pub trait FsDrive {
    fn open_root(&mut self) -> Inode;
}

pub struct Drive {
    fs: Box<dyn FsDrive>,
}

pub type MountpointList = HashMap<Path, Drive>;

fn parse_path(mut path: &str) -> Path {
    if path == "/" { return Path {segments: Vec::new()}; }
    if path.chars().next().unwrap() == '/' { path = &path[1..]; }
    if path.chars().last().unwrap() == '/' { path = &path[..path.len()-1]}
    let segments = path
        .split('/')
        .map(|s| String::from(s))
        .collect();
    return Path {segments}
}

fn path_exists(path: &str) -> bool {
    if parse_path(path).segments.len() != 0 {
        todo!("proper path_exists, only root supported");
    }
    true
}

pub fn mount(kernel: &mut Kernel, drive: Drive, path: &str) -> i32 {
    if !path_exists(path) { return -(Errno::ENOENT as i32); }
    let mountpoint_list: &mut MountpointList = kernel.mountpoint_list.as_mut().unwrap();
    mountpoint_list.insert(
        parse_path(path),
        drive,
    );
    0
}

fn get_mount<'a>(kernel: &'a mut Kernel, path: &str) -> Result<(Path, &'a mut Drive), i32> {
    let mut parsed = parse_path(path);
    let mountpoints = kernel.mountpoint_list.as_mut().unwrap();
    loop {
        if mountpoints.contains_key(&parsed) {
            return Ok((parsed.clone(), mountpoints.get_mut(&parsed).unwrap()));
        }
        if parsed.segments.len() == 0 { break }
        parsed.segments = parsed.segments[..parsed.segments.len()-1].to_vec();
    }
    Err(-(Errno::ENOENT as i32))
}

fn open(kernel: &mut Kernel, path: &str, flags: u32)
                                -> Result<Box<dyn FsInode>, i32> {
    let (mount_path, drive) = match get_mount(kernel, path) {
        Ok(v)  => (v.0, v.1),
        Err(e) => return Err(e),
    };
    let mut parsed = &parse_path(path).segments[mount_path.segments.len()..];
    let mut currently_open = drive.open_root().inner;
    loop {
        let segment = &parsed[0];
        currently_open = match currently_open.open(segment, flags) {
            Ok(v)  => v,
            Err(e) => return Err(e),
        };
        parsed = &parsed[1..];
        if parsed.len() == 0 { break }
    }
    Ok(currently_open)
}

pub fn init() {
    let mut kernel = KERNEL.lock();
    kernel.mountpoint_list = Some(HashMap::new());
    println!("VFS initialised.");
    test(&mut *kernel);
}

impl Drive {
    pub fn new(fs: Box<dyn FsDrive>) -> Self {
        Self {fs}
    }
    pub fn open_root(&mut self) -> Inode {
        self.fs.open_root()
    }
}

fn test(kernel: &mut Kernel) -> Result<(), i32> {
    let fs = Drive::new(Box::new(tempfs::new()));
    mount(kernel, fs, "/");
    open(kernel, "/test", AccessFlags::O_CREAT as u32);
    let mut f = open(kernel, "/file", AccessFlags::O_CREAT as u32)?;
    let mountpoints = kernel.mountpoint_list.as_mut().unwrap();
    let mounted_fs = mountpoints.get_mut(&parse_path("/")).unwrap();
    let mut ents = Vec::new();
    mounted_fs.open_root().inner.getdents(&mut ents, 10);
    println!("Enumerating `/`...");
    for ent in ents {
        let finfo = ent.getfinfo();
        println!(" -> Entry found in directory: {}", finfo.fname);
    }
    println!("Writing to /test...");
    f.write(utils::str_as_cstr("Hello, world!"), 0, 14);
    println!("Reading back...");
    let mut buf = Vec::new();
    f.read(&mut buf, 0, 14);
    println!("Read back \"{}\" successfully!", utils::cstr_as_str(buf));
    Ok(())
}
