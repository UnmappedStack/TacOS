#pragma once
#include <stddef.h>
#include <stdint.h>
#include <list.h>
#include <stdbool.h>
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

#define O_CREAT 64

typedef struct {
    uint8_t fs_id;
    void*   (*find_root_fn     )(void *fs);
    void*   (*open_fn          )(void *file);
    void    (*close_fn         )(void *file);
    void*   (*mkfile_fn        )(void *dir, char *name);
    void*   (*mkdir_fn         )(void *parentdir, char *name);
    void*   (*opendir_fn       )(void *buf, void *dir);
    void*   (*closedir_fn      )(void *dir);
    int     (*rmfile_fn        )(void *file);
    int     (*rmdir_fn         )(void *dir);
    void*   (*diriter_fn       )(void *iter);
    int     (*write_fn         )(void *file, char *buf, size_t len);
    int     (*read_fn          )(void *file, char *buf, size_t len);
    int     (*identify_fn      )(void *priv, char *buf, bool *is_dir);
    void*   (*file_from_diriter)(void *iter);
} FileSystem;

typedef struct {
    bool in_memory;
    FileSystem fs;
    void *private;
    // TODO: Add more info about drives (location, type, file system it uses, etc.)
} VfsDrive;

typedef struct {
    struct list list;
    char        path[MAX_PATH_LEN];
    VfsDrive    drive;
} VfsMountTableEntry;

typedef struct {
    VfsDrive drive;
    void *private;
} VfsFile;

typedef struct {
    void *private;
    uint8_t rsvd[20];
} VfsDirIter;

void init_vfs();
int vfs_mount(char *path, VfsDrive drive);
VfsDrive *vfs_find_mounted_drive(char *path);
VfsDrive *vfs_path_to_drive(char *path, size_t *drive_root_idx_buf);
VfsFile *vfs_access(char *path, int flags, VfsAccessType type);
int vfs_identify(VfsFile *file, char *name, bool *is_dir);
VfsFile *open(char *path, int flags);
int opendir(VfsDirIter *buf, char *path, int flags);
int mkfile(char *path);
int mkdir(char *path);
