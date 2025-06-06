#include <cpu.h>
#include <fs/tempfs.h>
#include <kernel.h>
#include <mem/pmm.h>
#include <mem/slab.h>
#include <printf.h>
#include <stdbool.h>
#include <string.h>

TempfsInode *tempfs_new() {
    if (!kernel.tempfs_inode_cache)
        kernel.tempfs_inode_cache =
            init_slab_cache(sizeof(TempfsInode), "TempFS TempfsInodes");
    if (!kernel.tempfs_direntry_cache)
        kernel.tempfs_direntry_cache =
            init_slab_cache(sizeof(TempfsDirEntry), "TempFS DirEntries");
    if (!kernel.tempfs_data_cache)
        kernel.tempfs_data_cache = init_slab_cache(PAGE_SIZE, "TempFS Data");
    TempfsInode *newfs = slab_alloc(kernel.tempfs_inode_cache);
    memcpy(newfs->name, "FSROOT", 7);
    newfs->type = Directory;
    newfs->parent = newfs;
    // create . and .. entries
    TempfsInode *prevdirentry, *thisdirentry;
    if (!(prevdirentry = tempfs_create_entry(newfs)))
        return NULL;
    if (!(thisdirentry = tempfs_create_entry(newfs)))
        return NULL;
    strcpy(prevdirentry->name, "..");
    strcpy(thisdirentry->name, ".");
    thisdirentry->type = prevdirentry->type = Directory;
    thisdirentry->first_dir_entry = prevdirentry->first_dir_entry =
        newfs->first_dir_entry;
    return newfs;
}

// Highly complexicated indeed
TempfsInode *tempfs_find_root(TempfsInode *fs) { return fs; }

TempfsInode *tempfs_create_entry(TempfsInode *dir) {
    if (dir->type != Directory) {
        printf(
            "Failed to create TempFS entry: TempfsInode is not a directory.\n");
        return NULL;
    }
    TempfsDirEntry *new_entry;
    if (!dir->first_dir_entry) {
        dir->first_dir_entry = slab_alloc(kernel.tempfs_direntry_cache);
        new_entry = dir->first_dir_entry;
    } else {
        // TODO: Make this wayyy faster cos this sucks a lot. Possibly use
        // `struct list`?
        TempfsDirEntry *last_entry = dir->first_dir_entry;
        while (last_entry->next) last_entry = last_entry->next;
        last_entry->next = slab_alloc(kernel.tempfs_direntry_cache);
        new_entry = last_entry->next;
    }
    new_entry->next = NULL;
    new_entry->inode = (TempfsInode *)(kmalloc(1) + kernel.hhdm);
    new_entry->inode->parent = dir;
    return new_entry->inode;
}

TempfsInode *tempfs_new_file(TempfsInode *dir, char *name) {
    TempfsInode *new_inode;
    if ((new_inode = tempfs_create_entry(dir)) == NULL)
        return new_inode;
    if (strlen(name) + 1 > MAX_FILENAME_LEN) {
        printf("Filename is longer than %i characters, which is the max length "
               "for file system of type TempFS.\n",
               MAX_FILENAME_LEN);
        return NULL;
    }
    strcpy(new_inode->name, name);
    new_inode->type = RegularFile;
    new_inode->size = 0;
    return new_inode;
}

TempfsInode *tempfs_mkdir(TempfsInode *parentdir, char *name) {
    TempfsInode *new_inode;
    if ((new_inode = tempfs_create_entry(parentdir)) == NULL)
        return new_inode;
    if (strlen(name) + 1 > MAX_FILENAME_LEN) {
        printf("Directory name is longer than %i characters, which is the max "
               "length for file system of type TempFS.\n",
               MAX_FILENAME_LEN);
        return NULL;
    }
    strcpy(new_inode->name, name);
    new_inode->type = Directory;
    // create the .. and . entries
    TempfsInode *prevdirentry, *thisdirentry;
    if (!(prevdirentry = tempfs_create_entry(new_inode)))
        return NULL;
    if (!(thisdirentry = tempfs_create_entry(new_inode)))
        return NULL;
    strcpy(prevdirentry->name, "..");
    strcpy(thisdirentry->name, ".");
    thisdirentry->type = prevdirentry->type = Directory;
    thisdirentry->first_dir_entry = new_inode->first_dir_entry;
    prevdirentry->first_dir_entry = parentdir->first_dir_entry;
    return new_inode;
}

TempfsInode *tempfs_diriter(TempfsDirIter *iter) {
    if (!iter->current_entry)
        return NULL;
    TempfsInode *to_return = iter->current_entry->inode;
    if (iter->is_first) {
        iter->is_first = false;
        return to_return;
    }
    iter->current_entry = iter->current_entry->next;
    return to_return;
}

int tempfs_identify(TempfsInode *inode, char *namebuf, bool *is_dir_buf,
                    size_t *fsize) {
    if (!inode)
        return -1;
    if (namebuf)
        strcpy(namebuf, inode->name);
    if (is_dir_buf)
        *is_dir_buf = inode->type == Directory;
    if (fsize)
        *fsize = inode->size;
    return 0;
}

int tempfs_access(TempfsInode *file, char *buf, size_t len, size_t offset,
                  bool write) {
    if (file->type != RegularFile)
        return 0;
    if (write) {
        if (!file->first_file_node)
            file->first_file_node =
                (TempfsFileNode *)(kmalloc(1) + kernel.hhdm);
    } else {
        if (!file->first_file_node)
            return 0;
    }
    TempfsFileNode *this_fnode = file->first_file_node;
    size_t len_left = len;
    size_t off = 0;
    while (len_left > 0) {
        if (!this_fnode && !write)
            return 0; // EOF
        if (offset < FILE_DATA_BLOCK_LEN) {
            size_t bytes_to_copy = (len_left > (FILE_DATA_BLOCK_LEN - offset))
                                       ? (FILE_DATA_BLOCK_LEN - offset)
                                       : len_left;
            if (write) // accessing as write?
                memcpy(this_fnode->data + offset, buf + off, bytes_to_copy);
            else // ...or accessing as read?
                memcpy(buf + off, this_fnode->data + offset, bytes_to_copy);
            len_left -= bytes_to_copy;
            offset = 0;
            off += bytes_to_copy;
        } else {
            offset -= FILE_DATA_BLOCK_LEN;
        }
        if (write && !this_fnode->next && len_left > 0) {
            this_fnode->next = (TempfsFileNode *)(kmalloc(1) + kernel.hhdm);
            this_fnode->next->next = 0;
        }
        this_fnode = this_fnode->next;
    }
    if (write)
        file->size += len;
    return len - len_left;
}

int tempfs_write(TempfsInode *file, char *buf, size_t len, size_t offset) {
    if (file->type == Device) {
        return file->devops.write(file, buf, len, offset);
    }
    return tempfs_access(file, buf, len, offset, true);
}

int tempfs_read(TempfsInode *file, char *buf, size_t len, size_t offset) {
    if (file->type == Device)
        return file->devops.read(file, buf, len, offset);
    return tempfs_access(file, buf, len, offset, false);
}

int tempfs_rmdir(TempfsDirIter *dir) {
    (void)dir;
    printf("TODO: Implement rmdir in tempfs.\n");
    return -1;
}

int tempfs_rmfile(TempfsInode *file) {
    (void)file;
    printf("TODO: Implement rmfile in tempfs.\n");
    return -1;
}

TempfsInode *tempfs_open(TempfsInode *file) {
    if (file->type == Device) {
        if (file->devops.open(file))
            return NULL;
    }
    return file;
}

int tempfs_close(TempfsInode *file) {
    if (file->type == Device)
        return file->devops.close(file);
    return 0;
}

TempfsDirIter *tempfs_opendir(TempfsInode *dir) {
    if (dir->type != Directory) {
        printf("Cannot open directory because it's not a directory. dir = %p\n",
               dir);
        HALT_DEVICE();
        return NULL;
    }
    TempfsDirIter *buf = slab_alloc(kernel.tempfs_direntry_cache);
    buf->inode = dir;
    buf->current_entry = dir->first_dir_entry;
    buf->is_first = true;
    return buf;
}

int tempfs_closedir(TempfsDirIter *dir) {
    (void)dir;
    return 0;
}

void *tempfs_file_from_diriter(TempfsDirIter *iter) { return iter->inode; }

FileSystem tempfs = (FileSystem){
    .fs_id = fs_tempfs,
    .find_root_fn = (void *(*)(void *))tempfs_find_root,
    .open_fn = (void *(*)(void *))tempfs_open,
    .close_fn = (int (*)(void *))tempfs_close,
    .mkfile_fn = (void *(*)(void *, char *))tempfs_new_file,
    .mkdir_fn = (void *(*)(void *, char *))tempfs_mkdir,
    .opendir_fn = (void *(*)(void *))tempfs_opendir,
    .closedir_fn = (int (*)(void *))tempfs_closedir,
    .rmfile_fn = (int (*)(void *))tempfs_rmfile,
    .rmdir_fn = (int (*)(void *))tempfs_rmdir,
    .diriter_fn = (void *(*)(void *))tempfs_diriter,
    .write_fn = (int (*)(void *, char *, size_t, size_t))tempfs_write,
    .read_fn = (int (*)(void *, char *, size_t, size_t))tempfs_read,
    .identify_fn = (int (*)(void *, char *, bool *, size_t *))tempfs_identify,
    .file_from_diriter = (void *(*)(void *))tempfs_file_from_diriter,
};
