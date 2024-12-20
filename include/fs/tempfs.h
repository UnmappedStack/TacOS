#pragma once
#include <mem/paging.h>

#define MAX_FILENAME_LEN    20
#define FILE_DATA_BLOCK_LEN (PAGE_SIZE - sizeof(DirEntry*))

typedef struct FileNode FileNode;
typedef struct DirEntry DirEntry;
typedef struct Inode Inode;

typedef enum {
    RegularFile,
    Directory,
} InodeType;

struct FileNode {
    FileNode *next;
    char data[FILE_DATA_BLOCK_LEN];
};

struct DirEntry {
    DirEntry *next;
    Inode *inode;
};

struct Inode {
    char name[MAX_FILENAME_LEN]; // TODO: Allocate dynamically
    Inode *parent;
    InodeType type;
    union {
        FileNode *first_file_node;
        DirEntry *first_dir_entry;
    };
};

typedef struct {
    Inode    *inode;
    DirEntry *current_entry;
} DirIter;

Inode *tempfs_new();
Inode *tempfs_find_root(Inode *fs);
Inode *tempfs_create_entry(Inode *dir);
Inode *tempfs_new_file(Inode *dir, char *name);
Inode *tempfs_mkdir(Inode *parentdir, char *name);
Inode *tempfs_open(Inode *file);
void tempfs_close(Inode *file);
void tempfs_opendir(DirIter *buf, Inode *dir);
int tempfs_write(Inode *file, char *buf, size_t len);
int tempfs_read(Inode *file, char *buf, size_t len);
Inode *tempfs_diriter(DirIter *iter);
void tempfs_closedir(Inode *dir);
Inode *tempfs_rmdir(Inode *dir);
Inode *tempfs_rmfile(Inode *file);
