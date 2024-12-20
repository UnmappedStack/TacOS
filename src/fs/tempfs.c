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
    printf("Created tempfs\n");
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

int tempfs_access(TempfsInode *file, char *buf, size_t len, bool write) {
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
        if (write) // accessing as write...
            memcpy(this_fnode->data, buf + off, bytes_to_copy);
        else // or acccessing as read?
            memcpy(buf + off, this_fnode->data, bytes_to_copy);
        off += bytes_to_copy;
        len_left -= bytes_to_copy;
        this_fnode = this_fnode->next;
    }
    return 0;
}

int tempfs_write(TempfsInode *file, char *buf, size_t len) {
    return tempfs_access(file, buf, len, true);
}

int tempfs_read(TempfsInode *file, char *buf, size_t len) {
    return tempfs_access(file, buf, len, false);
}

TempfsInode *tempfs_rmdir(TempfsInode *dir) {
    (void) dir;
    printf("TODO: Implement rmdir in tempfs.\n");
    return NULL;
}

TempfsInode *tempfs_rmfile(TempfsInode *file) {
    (void) file;
    printf("TODO: Implement rmfile in tempfs.\n");
    return NULL;
}

// These functions are *super* complex, *clearly*
TempfsInode *tempfs_open(TempfsInode *file) {
    return file;
}
void tempfs_close(TempfsInode *file) {
    (void) file;
}
void tempfs_opendir(TempfsDirIter *buf, TempfsInode *dir) {
    buf->inode = dir;
    buf->current_entry = dir->first_dir_entry;
}
void tempfs_closedir(TempfsInode *dir) {
    (void) dir;
}
