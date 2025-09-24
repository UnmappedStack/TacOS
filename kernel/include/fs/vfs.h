#pragma once
#include <list.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#define MAX_FILENAME_LEN 20
#define MAX_PATH_LEN 260

typedef enum {
    fs_tempfs,
} FsID;

typedef enum {
    VAT_open,
    VAT_opendir,
    VAT_mkfile,
    VAT_mkdir,
} VfsAccessType;

typedef enum {
    FT_REGFILE,
    FT_DIRECTORY,
    FT_CHARDEV,
} VFSFileType;

/* TODO: Actually take flags into account when reading/writing files */
typedef enum {
    O_CREAT = 1,
    O_RDONLY = 2,
    O_WRONLY = 4,
    O_RDWR = 8,
} VfsFlag;

typedef struct FSOps FSOps;
struct FSOps {
    int (*open_fn)(void **buf, void *file);
    int (*close_fn)(void *file);
    void *(*mkfile_fn)(void *dir, char *name, FSOps *ops);
    void *(*mkdir_fn)(void *parentdir, char *name, FSOps *ops);
    void *(*opendir_fn)(void *dir);
    int (*rmfile_fn)(void *file);
    int (*rmdir_fn)(void *dir);
    void *(*diriter_fn)(void *iter, FSOps *ops);
    int (*write_fn)(void *file, char *buf, size_t len, size_t offset);
    int (*read_fn)(void *file, char *buf, size_t len, size_t offset);
    int (*identify_fn)(void *priv, char *buf, VFSFileType *type, size_t *fsize);
    void *(*file_from_diriter)(void *iter);
    void *(*find_inode_in_dir)(void *dir, char *fname, FSOps *ops);
    int (*truncate_fn)(void *file, size_t sz);
};

typedef struct {
    uint8_t fs_id;
    void *(*find_root_fn)(void *fs, FSOps *ops);
} FileSystem;

typedef struct {
    bool in_memory;
    FileSystem fs;
    void *private;
    // TODO: Add more info about drives (location, type, file system it uses,
    // etc.)
} VfsDrive;

typedef struct {
    struct list list;
    char path[MAX_PATH_LEN];
    VfsDrive drive;
} VfsMountTableEntry;

typedef struct {
    VfsDrive drive;
    FSOps ops;
    void *private;
} VfsFile;

typedef struct {
    char path[MAX_PATH_LEN];
    void *private;
    VfsDrive drive;
    size_t mntidx;
    FSOps ops;
} VfsDirIter;

void init_vfs(void);
int vfs_mount(char *path, VfsDrive drive);
VfsDrive *vfs_find_mounted_drive(char *path);
VfsDrive *vfs_path_to_drive(char *path, size_t *drive_root_idx_buf);
VfsFile *vfs_access(char *path, int flags, VfsAccessType type);
int vfs_identify(VfsFile *file, char *name, VFSFileType *type, size_t *fsize);
VfsFile *open(char *path, int flags);
int opendir(VfsDirIter *buf, VfsFile **first_entry_buf, char *path, int flags);
int mkfile(char *path);
int mkdir(char *path);
int closedir(VfsDirIter *dir);
int close(VfsFile *file);
int rm_file(VfsFile *file);
int rm_dir(VfsDirIter *dir);
VfsFile *vfs_diriter(VfsDirIter *dir, VFSFileType *type);
VfsDirIter vfs_file_to_diriter(VfsFile *f);
int vfs_read(VfsFile *file, char *buffer, size_t len, size_t offset);
int vfs_write(VfsFile *file, char *buffer, size_t len, size_t offset);
int vfs_dir_exists(char *path);
int vfs_truncate(VfsFile *f, size_t sz);
