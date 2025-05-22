/* NOTE: This isn't the complete API which user programs will use! The
 * syscall layer will abstract it to follow POSIX properly.
 * I'm not a huge fan of this VFS overall, both the design is clunky and the
 * implementation is messy, but for now it works well enough, especially because
 * of the syscall layer's extra abstraction. Then again, there's already quite a
 * few levels of unnecessary abstraction so maybe that's not a good idea... */

#include <cpu.h>
#include <fs/vfs.h>
#include <kernel.h>
#include <mem/slab.h>
#include <printf.h>
#include <string.h>

void init_vfs(void) {
    kernel.vfs_mount_table_cache =
        init_slab_cache(sizeof(VfsMountTableEntry), "VFS mount table");
    kernel.vfs_file_cache = init_slab_cache(sizeof(VfsFile), "VFS file cache");
    list_init(&kernel.vfs_mount_table_list);
    printf("Inititated VFS.\n");
}

int vfs_dir_exists(char *path) {
    VfsFile *tmp;
    VfsDirIter tmpbuf;
    if (opendir(&tmpbuf, &tmp, path, 0) < 0)
        return 0;
    closedir(&tmpbuf);
    return 1;
}

int vfs_mount(char *path, VfsDrive drive) {
    if (strlen(path) >= MAX_PATH_LEN) {
        printf("Cannot mount drive on this path, path is too long (max: %i "
               "characters)\n",
               MAX_PATH_LEN);
        return -1;
    }
    if (!vfs_dir_exists(path) && strcmp(path, "/")) {
        printf("Cannot mount: directory doesn't exist.\n");
        return -1;
    }
    VfsMountTableEntry *new_entry = slab_alloc(kernel.vfs_mount_table_cache);
    strcpy(new_entry->path, path);
    new_entry->drive = drive;
    printf("new entry find root is %p\n", new_entry,
           new_entry->drive.fs.find_root_fn);
    list_insert(&kernel.vfs_mount_table_list, &new_entry->list);
    return 0;
}

VfsDrive *vfs_find_mounted_drive(char *path) {
    VfsMountTableEntry *this_entry;
    for (struct list *iter = kernel.vfs_mount_table_list.next;
         iter != &kernel.vfs_mount_table_list; iter = iter->next) {
        this_entry = (VfsMountTableEntry *)iter;
        if (strcmp(this_entry->path, path))
            continue;
        return &this_entry->drive;
    }
    return NULL;
}

VfsDrive *vfs_path_to_drive(char *path, size_t *drive_root_idx_buf) {
    char path_cpy[MAX_PATH_LEN];
    strcpy(path_cpy, path);
    size_t path_len = strlen(path);
    char *at = &path_cpy[path_len];
    char *at_end = at;
    const char *path_end = at;
    VfsDrive *drive;
    size_t i = path_len;
    size_t i_lag = i;
    while (*at || at == path_end || at == path_cpy) {
        if (*at == '/' || at == path_cpy) {
            *at_end = 0;
            if ((drive = vfs_find_mounted_drive(path_cpy))) {
                *drive_root_idx_buf = i_lag;
                return drive;
            }
            *at_end = '/';
            at_end = at;
            i_lag = i;
        }
        i--;
        at--;
    }
    *drive_root_idx_buf = 0;
    return vfs_find_mounted_drive("/");
}

// If any field is NULL, it won't copy it to the buffer.
int vfs_identify(VfsFile *file, char *name, bool *is_dir, size_t *fsize) {
    return file->drive.fs.identify_fn(file->private, name, is_dir, fsize);
}

// Returns the *private*, not an actual VfsFile
void *find_direntry(VfsDrive *drive, VfsDirIter *dir, char *name) {
    char entry_name[MAX_FILENAME_LEN];
    void *this_entry;
    for (;;) {
        if (!dir)
            return NULL;
        if (!(this_entry = drive->fs.diriter_fn(dir->private)))
            return NULL;
        if (drive->fs.identify_fn(this_entry, entry_name, NULL, NULL) < 0)
            return NULL;
        if (!strcmp(name, entry_name))
            return this_entry;
    }
}

VfsFile *vfs_access(char *path, int flags, VfsAccessType type) {
    char path_from_rel[MAX_PATH_LEN];
    if (*path != '/') {
        char *cwd = kernel.scheduler.current_task->cwd;
        size_t cwd_len = strlen(cwd);
        memcpy(path_from_rel, cwd, cwd_len);
        memcpy(path_from_rel + cwd_len, path, strlen(path) + 1);
        path = path_from_rel;
    }
    if (strlen(path) >= MAX_PATH_LEN) {
        printf("Path is too long (max length is currently %i bytes)\n",
               MAX_PATH_LEN);
    }
    size_t new_path_start_idx;
    VfsDrive *drive = vfs_path_to_drive(path, &new_path_start_idx);
    if (!drive) {
        printf("Path is not mounted anywhere (make sure root is mounted!). "
               "Cannot open.\n");
        return NULL;
    }
    char path_cpy[MAX_PATH_LEN];
    strcpy(path_cpy, &path[new_path_start_idx]);
    if (!path_cpy[0]) {
        VfsFile *file_addr = slab_alloc(kernel.vfs_file_cache);
        *file_addr = (VfsFile){
            .private = drive->fs.find_root_fn(drive->private),
            .drive = *drive,
        };
        return file_addr;
    }
    char *path_lag = &path_cpy[1];
    size_t pathlen = strlen(path) + 1;
    void *current_dir = drive->fs.find_root_fn(drive->private);
    VfsDirIter cd_iter = {0};
    for (size_t i = 1; i < pathlen; i++) {
        if (path_cpy[i] == '/' || !path_cpy[i]) {
            char original_char = path_cpy[i];
            path_cpy[i] = 0;
            if (*path_lag == '/')
                path_lag++;
            bool is_dir = original_char == '/';
            cd_iter.private = drive->fs.opendir_fn(current_dir);
            if (is_dir) {
                VfsFile *cd_temp = find_direntry(drive, &cd_iter, path_lag);
                drive->fs.close_fn(current_dir);
                if (!cd_temp) {
                    printf("Couldn't open directory: \"%s\": doesn't exist.\n",
                           path_lag);
                    return NULL;
                }
                current_dir = cd_temp;
            } else {
                VfsFile *entry = find_direntry(drive, &cd_iter, path_lag);
                if (!entry && ((flags & O_CREAT) || type == VAT_mkdir ||
                               type == VAT_mkfile)) {
                    void *new_file = NULL;
                    ;
                    if (type == VAT_mkfile || type == VAT_open) {
                        new_file = drive->fs.mkfile_fn(current_dir, path_lag);
                    } else if (type == VAT_mkdir || type == VAT_opendir)
                        new_file = drive->fs.mkdir_fn(current_dir, path_lag);
                    else {
                        printf("Unsupported type in vfs_access\n");
                        return NULL;
                    }
                    drive->fs.close_fn(current_dir);
                    VfsFile *file_addr = slab_alloc(kernel.vfs_file_cache);
                    *file_addr = (VfsFile){
                        .private = new_file,
                        .drive = *drive,
                    };
                    return file_addr;
                } else if (!entry) {
                    printf("Can't open file/directory: doesn't exist and "
                           "O_CREAT is not in use.\n");
                    drive->fs.close_fn(current_dir);
                    return NULL;
                } else {
                    if (type == VAT_mkdir || type == VAT_mkfile) {
                        printf("Cannot create: file or directory already "
                               "exists.\n");
                        return NULL;
                    }
                    drive->fs.close_fn(current_dir);
                    VfsFile *file_addr = slab_alloc(kernel.vfs_file_cache);
                    *file_addr = (VfsFile){
                        .private = drive->fs.open_fn(entry),
                        .drive = *drive,
                    };
                    vfs_identify(file_addr, path_cpy, &is_dir, NULL);
                    if (is_dir && type == VAT_open) {
                        printf("Can't open file, is a directory.\n");
                        drive->fs.close_fn(entry);
                        return NULL;
                    } else if (!is_dir && type == VAT_opendir) {
                        printf("Can't open directory, is a file.\n");
                        drive->fs.close_fn(entry);
                        return NULL;
                    }
                    return file_addr;
                }
            }
            path_cpy[i] = original_char;
            path_lag = &path_cpy[i];
        }
    }
    return NULL;
}

VfsFile *open(char *path, int flags) {
    return vfs_access(path, flags, VAT_open);
}

int opendir(VfsDirIter *buf, VfsFile **first_entry_buf, char *path, int flags) {
    if (!buf)
        return -1;
    VfsFile *temp = vfs_access(path, flags, VAT_opendir);
    if (!temp)
        return -1;
    buf->private = temp->drive.fs.opendir_fn(temp->private);
    buf->drive = temp->drive;
    temp->drive.fs.close_fn(temp);
    *first_entry_buf = slab_alloc(kernel.vfs_file_cache);
    **first_entry_buf = (VfsFile){
        .private = temp->drive.fs.diriter_fn(buf->private),
        .drive = temp->drive,
    };
    strcpy(buf->path, path);
    return 0;
}

int mkfile(char *path) {
    VfsFile *temp;
    if ((temp = vfs_access(path, 0, VAT_mkfile))) {
        temp->drive.fs.close_fn(temp);
        return 0;
    } else {
        return -1;
    }
}

int mkdir(char *path) {
    VfsFile *temp;
    if ((temp = vfs_access(path, 0, VAT_mkdir))) {
        temp->drive.fs.close_fn(temp);
        return 0;
    } else {
        return -1;
    }
}

int close(VfsFile *file) { return file->drive.fs.close_fn(file->private); }

int closedir(VfsDirIter *dir) {
    return dir->drive.fs.closedir_fn(dir->private);
}

int rm_file(VfsFile *file) { return file->drive.fs.rmfile_fn(file->private); }

int rm_dir(VfsDirIter *dir) { return dir->drive.fs.rmdir_fn(dir->private); }

int vfs_read(VfsFile *file, char *buffer, size_t len, size_t offset) {
    return file->drive.fs.read_fn(file->private, buffer, len, offset);
}

int vfs_write(VfsFile *file, char *buffer, size_t len, size_t offset) {
    return file->drive.fs.write_fn(file->private, buffer, len, offset);
}

/* This is a kinda clunky API, but basically:
 *  - You can get a VfsFile from vfs_diriter
 *  - If you read that it's a dir, you should use vfs_file_to_diriter to get
 * that new directory as a VfsDirIter
 */
VfsFile *vfs_diriter(VfsDirIter *dir, bool *is_dir) {
    VfsFile *to_return = slab_alloc(kernel.vfs_file_cache);
    *to_return = (VfsFile){
        .drive = dir->drive,
        .private = dir->drive.fs.diriter_fn(dir->private),
    };
    if (!to_return->private)
        return NULL;
    vfs_identify(to_return, NULL, is_dir, NULL);
    return to_return;
}

VfsDirIter vfs_file_to_diriter(VfsFile *f) {
    return (VfsDirIter){
        .drive = f->drive,
        .private = f->drive.fs.opendir_fn(f),
    };
}
