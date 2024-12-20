#pragma once
#include <mem/paging.h>

#define MAX_FILENAME_LEN    20
#define FILE_DATA_BLOCK_LEN (PAGE_SIZE - sizeof(TempfsDirEntry*))

typedef struct TempfsFileNode TempfsFileNode;
typedef struct TempfsDirEntry TempfsDirEntry;
typedef struct TempfsInode TempfsInode;

typedef enum {
    RegularFile,
    Directory,
} TempfsInodeType;

struct TempfsFileNode {
    TempfsFileNode *next;
    char data[FILE_DATA_BLOCK_LEN];
};

struct TempfsDirEntry {
    TempfsDirEntry *next;
    TempfsInode *inode;
};

struct TempfsInode {
    char name[MAX_FILENAME_LEN]; // TODO: Allocate dynamically
    TempfsInode *parent;
    TempfsInodeType type;
    union {
        TempfsFileNode *first_file_node;
        TempfsDirEntry *first_dir_entry;
    };
};

typedef struct {
    TempfsInode    *inode;
    TempfsDirEntry *current_entry;
} TempfsDirIter;

TempfsInode *tempfs_new();
TempfsInode *tempfs_find_root(TempfsInode *fs);
TempfsInode *tempfs_create_entry(TempfsInode *dir);
TempfsInode *tempfs_new_file(TempfsInode *dir, char *name);
TempfsInode *tempfs_mkdir(TempfsInode *parentdir, char *name);
TempfsInode *tempfs_open(TempfsInode *file);
void tempfs_close(TempfsInode *file);
void tempfs_opendir(TempfsDirIter *buf, TempfsInode *dir);
int tempfs_write(TempfsInode *file, char *buf, size_t len);
int tempfs_read(TempfsInode *file, char *buf, size_t len);
TempfsInode *tempfs_diriter(TempfsDirIter *iter);
void tempfs_closedir(TempfsInode *dir);
TempfsInode *tempfs_rmdir(TempfsInode *dir);
TempfsInode *tempfs_rmfile(TempfsInode *file);
