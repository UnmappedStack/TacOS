#include <fs/tempfs.h>
#include <printf.h>
#include <mem/slab.h>
#include <mem/pmm.h>
#include <kernel.h>
#include <string.h>
#include <stdbool.h>

TempfsInode *tempfs_new() {
    if (!kernel.tempfs_inode_cache)
        kernel.tempfs_inode_cache = init_slab_cache(sizeof(TempfsInode), "TempFS TempfsInodes");
    if (!kernel.tempfs_direntry_cache)
        kernel.tempfs_direntry_cache = init_slab_cache(sizeof(TempfsDirEntry), "TempFS DirEntries");
    TempfsInode *newfs = slab_alloc(kernel.tempfs_inode_cache);
    memcpy(newfs->name, "FSROOT", 7);
    newfs->type = Directory;
    newfs->parent = newfs;
    return newfs;
}

// Highly complexicated indeed
TempfsInode *tempfs_find_root(TempfsInode *fs) {
    return fs;
}

TempfsInode *tempfs_create_entry(TempfsInode *dir) {
    if (dir->type != Directory) {
        printf("Failed to create TempFS entry: TempfsInode is not a directory.\n");
        return NULL;
    }
    TempfsDirEntry *new_entry;
    if (!dir->first_dir_entry) {
        dir->first_dir_entry = slab_alloc(kernel.tempfs_direntry_cache);
        new_entry = dir->first_dir_entry;
    } else {
        // TODO: Make this wayyy faster cos this sucks a lot. Possibly use `struct list`?
        TempfsDirEntry *last_entry = dir->first_dir_entry;
        while (last_entry->next) last_entry = last_entry->next;
        last_entry->next = slab_alloc(kernel.tempfs_direntry_cache);
        new_entry = last_entry->next;
    }
    new_entry->next = NULL;
    new_entry->inode = slab_alloc(kernel.tempfs_inode_cache);
    new_entry->inode->parent = dir;
    return new_entry->inode;
}

TempfsInode *tempfs_new_file(TempfsInode *dir, char *name) {
    TempfsInode *new_inode;
    if ((new_inode = tempfs_create_entry(dir)) == NULL) return new_inode;
    if (strlen(name) + 1 > MAX_FILENAME_LEN) {
        printf("Filename is longer than %i characters, which is the max length for file system of type TempFS.\n", MAX_FILENAME_LEN);
        return NULL;
    }
    strcpy(new_inode->name, name);
    new_inode->type = RegularFile;
    return new_inode;
}

TempfsInode *tempfs_mkdir(TempfsInode *parentdir, char *name) {
    TempfsInode *new_inode;
    if ((new_inode = tempfs_create_entry(parentdir)) == NULL) return new_inode;
    if (strlen(name) + 1 > MAX_FILENAME_LEN) {
        printf("Directory name is longer than %i characters, which is the max length for file system of type TempFS.\n", MAX_FILENAME_LEN);
        return NULL;
    }
    strcpy(new_inode->name, name);
    new_inode->type = Directory;
    return new_inode;
   
}

TempfsInode *tempfs_diriter(TempfsDirIter *iter) {
    if (!iter->current_entry) return NULL;
    TempfsInode *to_return = iter->current_entry->inode;
    iter->current_entry = iter->current_entry->next;
    return to_return;
}

int tempfs_identify(TempfsInode *inode, char *namebuf, bool *is_dir_buf) {
    if (!inode) return -1;
    strcpy(namebuf, inode->name);
    *is_dir_buf = inode->type == Directory;
    return 0;
}

int tempfs_access(TempfsInode *file, char *buf, size_t len, size_t offset, bool write) {
    if (file->type != RegularFile) return -1;
    if (write) {
        if (!file->first_file_node) file->first_file_node = (TempfsFileNode*) (kmalloc(1) + kernel.hhdm);
    } else {
        if (!file->first_file_node) return -1;
    }
    TempfsFileNode *this_fnode = file->first_file_node;
    size_t len_left = len;
    size_t off = 0;
    while (len_left > 0 && this_fnode) {
        size_t bytes_to_copy = (len_left > FILE_DATA_BLOCK_LEN) ? FILE_DATA_BLOCK_LEN : len_left;
        if (offset < FILE_DATA_BLOCK_LEN) {
            if (write) // accessing as write...
                memcpy(this_fnode->data, buf + off + offset, bytes_to_copy);
            else // or acccessing as read?
                memcpy(buf + off, this_fnode->data + offset, bytes_to_copy);
            off += bytes_to_copy;
            len_left -= bytes_to_copy;
            this_fnode = this_fnode->next;
            offset = 0;
        } else
            offset -= FILE_DATA_BLOCK_LEN;
    }
    return 0;
}

int tempfs_write(TempfsInode *file, char *buf, size_t len, size_t offset) {
    return tempfs_access(file, buf, len, offset, true);
}

int tempfs_read(TempfsInode *file, char *buf, size_t len, size_t offset) {
    return tempfs_access(file, buf, len, offset, false);
}

int tempfs_rmdir(TempfsDirIter *dir) {
    (void) dir;
    printf("TODO: Implement rmdir in tempfs.\n");
    return -1;
}

int tempfs_rmfile(TempfsInode *file) {
    (void) file;
    printf("TODO: Implement rmfile in tempfs.\n");
    return -1;
}

// These functions are *super* complex, *clearly*
TempfsInode *tempfs_open(TempfsInode *file) {
    return file;
}

int tempfs_close(TempfsInode *file) {
    (void) file;
    return 0;
}

TempfsDirIter *tempfs_opendir(TempfsInode *dir) {
    TempfsDirIter *buf = slab_alloc(kernel.tempfs_direntry_cache);
    buf->inode = dir;
    buf->current_entry = dir->first_dir_entry;
    return buf;
}

int tempfs_closedir(TempfsDirIter *dir) {
    (void) dir;
    return 0;
}

void *tempfs_file_from_diriter(TempfsDirIter *iter) {
    return iter->inode;
}

FileSystem tempfs = (FileSystem) {
    .fs_id             = fs_tempfs,
    .find_root_fn      = (void *(*)(void*))tempfs_find_root,
    .open_fn           = (void *(*)(void *))tempfs_open,
    .close_fn          = (int (*)(void *))tempfs_close,
    .mkfile_fn         = (void *(*)(void *, char *))tempfs_new_file,
    .mkdir_fn          = (void *(*)(void *, char *))tempfs_mkdir,
    .opendir_fn        = (void *(*)(void *))tempfs_opendir,
    .closedir_fn       = (int (*)(void *))tempfs_closedir,
    .rmfile_fn         = (int (*)(void *))tempfs_rmfile,
    .rmdir_fn          = (int (*)(void *))tempfs_rmdir,
    .diriter_fn        = (void *(*)(void *))tempfs_diriter,
    .write_fn          = (int (*)(void *, char *, size_t, size_t))tempfs_write,
    .read_fn           = (int (*)(void *, char *, size_t, size_t))tempfs_read,
    .identify_fn       = (int (*)(void *, char *, bool *))tempfs_identify,
    .file_from_diriter = (void *(*)(void *))tempfs_file_from_diriter,
};
