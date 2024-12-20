#include <fs/tempfs.h>
#include <printf.h>
#include <mem/slab.h>
#include <mem/pmm.h>
#include <kernel.h>
#include <string.h>
#include <stdbool.h>

Inode *tempfs_new() {
    if (!kernel.tempfs_inode_cache)
        kernel.tempfs_inode_cache = init_slab_cache(sizeof(Inode), "TempFS Inodes");
    if (!kernel.tempfs_direntry_cache)
        kernel.tempfs_direntry_cache = init_slab_cache(sizeof(DirEntry), "TempFS DirEntries");
    Inode *newfs = slab_alloc(kernel.tempfs_inode_cache);
    memcpy(newfs->name, "FSROOT", 7);
    newfs->type = Directory;
    newfs->parent = newfs;
    printf("Created tempfs\n");
    return newfs;
}

// Highly complexicated indeed
Inode *tempfs_find_root(Inode *fs) {
    return fs;
}

Inode *tempfs_create_entry(Inode *dir) {
    if (dir->type != Directory) {
        printf("Failed to create TempFS entry: Inode is not a directory.\n");
        return NULL;
    }
    DirEntry *new_entry;
    if (!dir->first_dir_entry) {
        dir->first_dir_entry = slab_alloc(kernel.tempfs_direntry_cache);
        new_entry = dir->first_dir_entry;
    } else {
        // TODO: Make this wayyy faster cos this sucks a lot. Possibly use `struct list`?
        DirEntry *last_entry = dir->first_dir_entry;
        while (last_entry->next) last_entry = last_entry->next;
        last_entry->next = slab_alloc(kernel.tempfs_direntry_cache);
        new_entry = last_entry->next;
    }
    new_entry->next = NULL;
    new_entry->inode = slab_alloc(kernel.tempfs_inode_cache);
    new_entry->inode->parent = dir;
    return new_entry->inode;
}

Inode *tempfs_new_file(Inode *dir, char *name) {
    Inode *new_inode;
    if ((new_inode = tempfs_create_entry(dir)) == NULL) return new_inode;
    if (strlen(name) + 1 > MAX_FILENAME_LEN) {
        printf("Filename is longer than %i characters, which is the max length for file system of type TempFS.\n", MAX_FILENAME_LEN);
        return NULL;
    }
    strcpy(new_inode->name, name);
    new_inode->type = RegularFile;
    return new_inode;
}

Inode *tempfs_mkdir(Inode *parentdir, char *name) {
    Inode *new_inode;
    if ((new_inode = tempfs_create_entry(parentdir)) == NULL) return new_inode;
    if (strlen(name) + 1 > MAX_FILENAME_LEN) {
        printf("Directory name is longer than %i characters, which is the max length for file system of type TempFS.\n", MAX_FILENAME_LEN);
        return NULL;
    }
    strcpy(new_inode->name, name);
    new_inode->type = Directory;
    return new_inode;
   
}

Inode *tempfs_diriter(DirIter *iter) {
    if (!iter->current_entry) return NULL;
    Inode *to_return = iter->current_entry->inode;
    iter->current_entry = iter->current_entry->next;
    return to_return;
}

int tempfs_access(Inode *file, char *buf, size_t len, bool write) {
    if (file->type != RegularFile) return -1;
    if (write) {
        if (!file->first_file_node) file->first_file_node = (FileNode*) (kmalloc(1) + kernel.hhdm);
    } else {
        if (!file->first_file_node) return -1;
    }
    FileNode *this_fnode = file->first_file_node;
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

int tempfs_write(Inode *file, char *buf, size_t len) {
    return tempfs_access(file, buf, len, true);
}

int tempfs_read(Inode *file, char *buf, size_t len) {
    return tempfs_access(file, buf, len, false);
}

Inode *tempfs_rmdir(Inode *dir) {
    (void) dir;
    printf("TODO: Implement rmdir in tempfs.\n");
    return NULL;
}

Inode *tempfs_rmfile(Inode *file) {
    (void) file;
    printf("TODO: Implement rmfile in tempfs.\n");
    return NULL;
}

// These functions are *super* complex, *clearly*
Inode *tempfs_open(Inode *file) {
    return file;
}
void tempfs_close(Inode *file) {
    (void) file;
}
void tempfs_opendir(DirIter *buf, Inode *dir) {
    buf->inode = dir;
    buf->current_entry = dir->first_dir_entry;
}
void tempfs_closedir(Inode *dir) {
    (void) dir;
}
