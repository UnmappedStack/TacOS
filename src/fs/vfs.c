#include <cpu.h>
#include <fs/vfs.h>
#include <string.h>
#include <printf.h>
#include <mem/slab.h>
#include <kernel.h>

void init_vfs() {
    kernel.vfs_mount_table_cache = init_slab_cache(sizeof(VfsMountTableEntry), "VFS mount table");
    kernel.vfs_file_cache = init_slab_cache(sizeof(VfsFile), "VFS file cache");
    list_init(&kernel.vfs_mount_table_list);
    printf("Inititated VFS.\n");
}

int vfs_mount(char *path, VfsDrive drive) {
    if (strlen(path) >= MAX_PATH_LEN) {
        printf("Cannot mount drive on this path, path is too long (max: %i characters)\n", MAX_PATH_LEN);
        return -1;
    }
    VfsMountTableEntry *new_entry = slab_alloc(kernel.vfs_mount_table_cache);
    printf("Created new Drive at 0x%p\n", &new_entry->drive);
    strcpy(new_entry->path, path);
    new_entry->drive = drive;
    list_insert(&kernel.vfs_mount_table_list, &new_entry->list);
    return 0;
}

VfsDrive *vfs_find_mounted_drive(char *path) {
    VfsMountTableEntry *this_entry;
    for (struct list *iter = kernel.vfs_mount_table_list.next; iter != &kernel.vfs_mount_table_list; iter = iter->next) {
        this_entry = (VfsMountTableEntry*) iter;
        if (strcmp(this_entry->path, path)) continue;
        printf("Found matching drive, path = %s, drive addr = 0x%p\n", this_entry->path, &this_entry->drive);
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

int vfs_identify(VfsFile *file, char *name, bool *is_dir) {
    return file->drive.fs.identify_fn(file->private, name, is_dir);
}

// Returns the *private*, not an actual VfsFile
void *find_direntry(VfsDrive *drive, VfsDirIter *dir, char *name) {
    char entry_name[MAX_FILENAME_LEN];
    bool is_dir;
    void *this_entry;
    for (;;) {
        if (!dir) return NULL;
        if (!(this_entry = drive->fs.diriter_fn(dir))) return NULL;;
        if (drive->fs.identify_fn(this_entry, entry_name, &is_dir) < 0) return NULL;
        if (!strcmp(name, entry_name)) return this_entry;
    }
}


/// TODO AND WARNING YOU MUST SHORTEN THE PATH TO THE START OF THE MOUNTED THINGY!!!!
VfsFile *vfs_open(char *path, int flags) {
    if (*path != '/') {
        printf("Relative path accessing is not yet supported (TODO).\n");
        return NULL;
    }
    if (strlen(path) >= MAX_PATH_LEN) {
        printf("Path is too long (max length is currently %i bytes)\n", MAX_PATH_LEN);
    }
    size_t new_path_start_idx;
    VfsDrive *drive = vfs_path_to_drive(path, &new_path_start_idx);
    if (!drive) {
        printf("Path is not mounted anywhere (make sure root is mounted!). Cannot open.\n");
        return NULL;
    }
    char path_cpy[MAX_PATH_LEN];
    strcpy(path_cpy, &path[new_path_start_idx]);
    char *path_lag = &path_cpy[1];
    size_t pathlen = strlen(path) + 1;
    void *current_dir = drive->fs.find_root_fn(drive->private);
    VfsDirIter cd_iter;
    for (size_t i = 1; i < pathlen; i++) {
        if (path_cpy[i] == '/' || !path_cpy[i]) {
            char original_char = path_cpy[i];
            path_cpy[i] = 0;
            if (*path_lag == '/') path_lag++;
            bool is_dir = original_char == '/';
            drive->fs.opendir_fn(&cd_iter, current_dir);
            if (is_dir) {
                VfsFile *cd_temp = find_direntry(drive, &cd_iter, path_lag);
                drive->fs.close_fn(current_dir);
                if (!cd_temp) {
                    printf("Couldn't open directory: \"%s\": doesn't exist.\n", path_lag);
                    return NULL;
                }
                current_dir = cd_temp;
            } else {
                VfsFile *entry = find_direntry(drive, &cd_iter, path_lag);
                if (!entry && (flags & O_CREAT)) {
                    void *new_file = drive->fs.mkfile_fn(current_dir, path_lag);
                    drive->fs.close_fn(current_dir);
                    VfsFile *file_addr = slab_alloc(kernel.vfs_file_cache);
                    *file_addr = (VfsFile) {
                        .private = new_file,
                        .drive = *drive,
                    };
                    return file_addr;
                } else if (!entry) {
                    printf("Can't open file: doesn't exist and O_CREAT is not in use.\n");
                    drive->fs.close_fn(current_dir);
                    return NULL;
                } else {
                    drive->fs.close_fn(current_dir);
                    VfsFile *file_addr = slab_alloc(kernel.vfs_file_cache);
                    *file_addr = (VfsFile) {
                        .private = drive->fs.open_fn(entry),
                        .drive = *drive,
                    };
                    return file_addr;
                }
            }
            path_cpy[i] = original_char;
            path_lag = &path_cpy[i];
        }
    }
    return NULL;
}
