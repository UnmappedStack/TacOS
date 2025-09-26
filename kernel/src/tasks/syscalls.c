#include <cpu.h>
#include <mem/pmm.h>
#include <cpu/msr.h>
#include <exec.h>
#include <fork.h>
#include <kernel.h>
#include <printf.h>
#include <scheduler.h>
#include <stdint.h>

#define LSTAR_MSR 0xC0000082
#define  EFER_MSR 0xC0000080

extern void syscall_handler(void);

// This is just for the `syscall`/`sysret` method of interrupts which isn't actually
// used, I was just playing around with it. Interrupts for syscalls rein supreme!!
void init_syscalls(void) {
    // enable syscall/sysret
    uint64_t EFER;
    read_msr(EFER_MSR, &EFER);
    write_msr(EFER_MSR, EFER | 0b1);
    // set handler
    write_msr(LSTAR_MSR, (uint64_t) &syscall_handler);
}

int remove_child(Task *parent, pid_t child, bool in_wait, int status) {
    for (size_t i = 0; i < MAX_CHILDREN; i++) {
        if (parent->children[i].pid != child) continue;
        if (in_wait) // called from wait syscall
            parent->children[i].pid = 0;
        else
            parent->children[i].status = status;
        return 0;
    }
    return -1;
}

int sys_open(char *filename, int flags, int mode) {
    (void)mode;
    uint64_t file_descriptor = 0;
    Task *current_task = CURRENT_TASK;
    for (; file_descriptor < MAX_RESOURCES; file_descriptor++) {
        if (current_task->resources[file_descriptor].f)
            continue;
        current_task->resources[file_descriptor].f =
            open((char *)filename, flags);
        if (!current_task->resources[file_descriptor].f)
            goto err;
        current_task->resources[file_descriptor].offset = 0;
        printf("Opened %s to file descriptor %i\n", filename, file_descriptor);
        return file_descriptor;
    }
err:
    printf("Couldn't open file %s (fd = %i)\n", filename, file_descriptor);
    return -1;
}

int sys_close(int fd) {
    printf("close fd = %b\n", fd);
    return close(CURRENT_TASK->resources[fd].f);
}

size_t sys_read(int fd, char *buf, size_t count) {
    size_t resize = vfs_read(CURRENT_TASK->resources[fd].f, buf, count,
                             CURRENT_TASK->resources[fd].offset);
    CURRENT_TASK->resources[fd].offset += resize;
    return resize;
}
size_t sys_write(int fd, char *buf, size_t count) {
    return vfs_write(CURRENT_TASK->resources[fd].f, buf, count,
                     CURRENT_TASK->resources[fd].offset);
}

void sys_exit(int status) {
    // TODO: Clean up and report to parent
    if (CURRENT_TASK->pid <= 1) {
        DISABLE_INTERRUPTS();
        printf(
            "Init task exited! Halting device. (Exited with status %i, 0x%x)\n",
            status, status);
        HALT_DEVICE();
    }
    DISABLE_INTERRUPTS();
    printf("Exited task with status %i\n", status);
    remove_child(CURRENT_TASK->parent, CURRENT_TASK->pid, false, status);
    if (CURRENT_TASK->parent->waiting_for == CURRENT_TASK->pid)
        CURRENT_TASK->parent->waiting_for = 0;
    CURRENT_TASK->flags |= TASK_DEAD;
    CURRENT_TASK->flags &= ~TASK_PRESENT;
    ENABLE_INTERRUPTS();
    for (;;);
}

int sys_getpid(void) { return CURRENT_TASK->pid; }

int sys_execve(char *path, char **argv, char **envp) {
    DISABLE_INTERRUPTS();
    int e;
    if ((e = execve(CURRENT_TASK, path, argv, envp)) < 0)
        return e;
    ENABLE_INTERRUPTS();
    for (;;);
}

// TODO: Actually send a signal to the task and do clean up, report to it's
// parent
int sys_kill(int pid, int sig) {
    if (pid < 0) {
        printf("Tried to kill process group, but not supported yet.\n");
        return -1;
    } else if (pid <= 1) {
        printf("Killed init task! Halting device.\n");
        HALT_DEVICE();
        return -1;
    } else {
        Task *to_kill = task_from_pid(pid);
        to_kill->flags &= ~TASK_PRESENT;
        to_kill->flags |= TASK_DEAD;
        remove_child(to_kill->parent, to_kill->pid, false, sig);
        printf("Killed task %i with signal %i\n", pid, sig);
        return 0;
    }
}

int sys_wait(int *status) {
    (void) status;
    printf("TODO: sys_wait is not implemented yet\n");
    sys_exit(-1);
    return -1;
}

int sys_waitpid(int pid, int *status, int options) {
    // TODO: Write status info to *status and take options into consideration
    (void)status, (void)options;
    if (pid <= 0) {
        printf("TODO: waitpid does not yet support <=0 for the pid\n");
        return -1;
    }
    if (!(task_from_pid(pid)->flags & TASK_DEAD))
        CURRENT_TASK->waiting_for = pid;
    WAIT_FOR_INTERRUPT();
    return -1; // pid not found in children
}

uintptr_t __sbrk(intptr_t increment, bool map_to_phys) {
    if (!increment) {
        return CURRENT_TASK->program_break;
    }
    uintptr_t previous_break = CURRENT_TASK->program_break;
    CURRENT_TASK->program_break += PAGE_ALIGN_UP(increment);
    if (CURRENT_TASK->program_break > previous_break) {
        size_t num_new_pages = PAGE_ALIGN_UP(increment) / 4096;
        if (map_to_phys) {
            uintptr_t phys_pages = kmalloc(num_new_pages);
            map_pages((uint64_t *)(CURRENT_TASK->pml4 + kernel.hhdm),
                      previous_break, phys_pages, num_new_pages,
                      KERNEL_PFLAG_WRITE | KERNEL_PFLAG_PRESENT |
                          KERNEL_PFLAG_USER);
        }
        add_memregion(
            &CURRENT_TASK->memregion_list, previous_break, num_new_pages, true,
            KERNEL_PFLAG_WRITE | KERNEL_PFLAG_PRESENT | KERNEL_PFLAG_USER);
    }
    return previous_break;
}

uintptr_t sys_sbrk(intptr_t increment) {
    return __sbrk(increment, true);
}

/* TODO: Right now, this just deletes a file.
 * See https://man7.org/linux/man-pages/man2/unlink.2.html for how this should
 * actually work. This currently isn't posix compliant. */
int sys_unlink(char *path) {
    VfsFile *f = open(path, 0);
    return rm_file(f);
}

int sys_remove(char *filename) {
    VfsFile *f = vfs_access(filename, 0, 0);
    if (!f)
        return -1;
    VFSFileType type;
    if (vfs_identify(f, NULL, &type, NULL))
        return -1;
    if (type == FT_DIRECTORY) {
        VfsDirIter dir = vfs_file_to_diriter(f);
        return rm_dir(&dir);
    } else {
        return rm_file(f);
    }
}

int sys_mkdir(char *filename, size_t mode) {
    (void)mode;
    return mkdir(filename);
}

typedef enum {
    SEEK_SET,
    SEEK_CUR,
    SEEK_END,
} FileLocs;
size_t sys_lseek(int fd, size_t offset, int whence) {
    size_t fsize;
    switch (whence) {
    case SEEK_SET:
        CURRENT_TASK->resources[fd].offset = offset;
        return offset;
    case SEEK_CUR:
        CURRENT_TASK->resources[fd].offset += offset;
        return CURRENT_TASK->resources[fd].offset;
    case SEEK_END:
        vfs_identify(CURRENT_TASK->resources[fd].f, NULL, NULL, &fsize);
        CURRENT_TASK->resources[fd].offset = fsize + offset;
        return CURRENT_TASK->resources[fd].offset;
    default:
        printf("Invalid whence value for lseek\n");
        sys_exit(1);
        return 1;
    }
}

int sys_link(char *old, char *new) {
    (void) old, (void) new;
    printf("TODO: sys_link not implemented yet\n");
    sys_exit(-1);
    return -1;
}

void sys_invalid(int sys) {
    printf("Invalid syscall: %i\n", sys);
    sys_exit(-1);
}

/* called by the task switch. Don't ask why this is in the syscalls.c file lol
 */
void increment_global_clock(void) {
    // since this is called about every ms, add one ms to it
    const size_t frequency =
        1000000; // it's called every `frequency` nanoseconds
    const size_t ns_in_s = 1000000000;
    kernel.global_clock.tv_nsec += frequency;
    if (kernel.global_clock.tv_nsec >= ns_in_s) {
        kernel.global_clock.tv_sec++;
        kernel.global_clock.tv_nsec -= ns_in_s;
    }
}

/* TODO: Stop ignoring the clockid and actually have separate clocks */
int sys_clock_gettime(size_t clockid, struct timespec *tp) {
    (void)clockid;
    tp->tv_sec = kernel.global_clock.tv_sec;
    tp->tv_nsec = kernel.global_clock.tv_nsec;
    return 0;
}

void sys_times(void *buf) {
    (void) buf;
    printf("TODO: sys_times() is not implemented yet\n");
    sys_exit(-1);
}

// TODO: actually yield properly
void sys_sched_yield(void) {
    for (size_t i = 0; i < 3; i++) __asm__ volatile("hlt\n");
}

typedef struct {
    struct list list;
    uintptr_t paddr_start;
    VfsFile *f;
    size_t len;
    bool shared;
} FileVMemMapping;
struct list file_mappings = {0};
#define MS_SYNC  0b01
#define MS_ASYNC 0b10
int sys_msync(void *addr, size_t length, int flags) {
    (void) length;
    if ((flags & MS_ASYNC) || !(flags & MS_SYNC)) {
        printf("TODO: MS_ASYNC not supported yet\n");
        return -1;
    }
    uintptr_t paddr = virt_to_phys((uint64_t*) (CURRENT_TASK->pml4 + kernel.hhdm), (uint64_t) addr);
    for (struct list *entry = file_mappings.next; entry != &file_mappings; entry = entry->next) {
        FileVMemMapping *mapping = (FileVMemMapping*) entry;
        if (mapping->paddr_start != paddr) continue;
        vfs_write(mapping->f, addr, mapping->len, 0);
        return 0;
    }
    return -1; // not found
}

#define MAP_ANONYMOUS 0b01
#define MAP_SHARED    0b10
void *sys_mmap(void *addr, size_t length, int prot, int flags, int fd,
               size_t offset) {
    length += PAGE_SIZE;
    printf("mmap %i pages\n", PAGE_ALIGN_UP(length) / 4096);
    static Cache *fmcache;
    if (file_mappings.next == NULL) {
        list_init(&file_mappings);
        fmcache = init_slab_cache(sizeof(FileVMemMapping), "mmap cache");
    }

    (void) prot, (void) offset;
    if (flags & MAP_ANONYMOUS) return addr;
    if (!(flags & MAP_SHARED)) {
add_to_list:
        // dw sbrk will return a vaddr who's mem is physically contiguous
        addr = (void*) ((addr) ? PAGE_ALIGN_UP((uintptr_t)addr) : sys_sbrk(length));
        FileVMemMapping *mapping = slab_alloc(fmcache);
        mapping->paddr_start = virt_to_phys((uint64_t*) (CURRENT_TASK->pml4 + kernel.hhdm), (uint64_t) addr);
        mapping->f = CURRENT_TASK->resources[fd].f;
        mapping->shared = (flags & MAP_SHARED) ? true : false; // seems dumb but it's cos of the goto thing
        mapping->len = length;
        sys_read(fd, addr, length);
        list_insert(&file_mappings, &mapping->list);
        return addr;
    }
    for (struct list *entry = file_mappings.next; entry != &file_mappings; entry = entry->next) {
        FileVMemMapping *mapping = (FileVMemMapping*) entry;
        if (!mapping->shared || CURRENT_TASK->resources[fd].f->private != mapping->f->private) continue;
        addr = (void*) __sbrk(length, false);
        map_pages((void*) (CURRENT_TASK->pml4 + kernel.hhdm), (uintptr_t) addr, mapping->paddr_start,
                PAGE_ALIGN_UP(length) / PAGE_SIZE, KERNEL_PFLAG_PRESENT | KERNEL_PFLAG_WRITE | KERNEL_PFLAG_USER);
        return addr;
    }
    goto add_to_list; // not found, create new shared mem thingy
}

// kinda assumes the path is perfectly formatted etc. This is why you should use
// vfs_dir_exists before calling this. Also the strlens could def be reduced to
// make this faster.
void expand_path_dots(char *path) {
    for (int i = 0; path[i]; i++) {
        if (!memcmp(&path[i], "/../", 4) || !memcmp(&path[i], "/..\0", 4)) {
            size_t off = 0;
            if (i > 1) {
                off++;
                while (path[i - off] != '/') off++;
            }
            memmove(&path[i - off], &path[i + 3], strlen(&path[3] + 1));
            i -= 4 + off;
        } else if (!memcmp(&path[i], "/./", 3) ||
                   !memcmp(&path[i], "/.\0", 3)) {
            memmove(&path[i], &path[i + 2], strlen(&path[2] + 1));
            i -= 3;
        }
    }
}

int sys_chdir(char *path) {
    char buf[MAX_PATH_LEN];
    if (path[0] != '/') {
        strcpy(buf, CURRENT_TASK->cwd);
        strcpy(buf + strlen(CURRENT_TASK->cwd), path);
        path = buf;
    }
    // check dir exists
    if (!vfs_dir_exists(path))
        return -1;
    expand_path_dots(path);
    // set dir
    size_t len = strlen(path);
    memcpy(CURRENT_TASK->cwd, path, len + 1);
    if (CURRENT_TASK->cwd[len - 1] != '/') {
        CURRENT_TASK->cwd[len] = '/';
        CURRENT_TASK->cwd[len + 1] = '\0';
    }
    return 0;
}

char *sys_getcwd(char *buf, size_t n) {
    memcpy(buf, CURRENT_TASK->cwd, (n > MAX_PATH_LEN) ? MAX_PATH_LEN : n);
    return buf;
}

// returns fd of directory, creates a new diriter and stored it in buf.
// opens dir from path given in `path`
int sys_opendir(VfsDirIter *buf, char *filename) {
    VfsFile *newfile;
    if (opendir(buf, &newfile, filename, 0) < 0)
        goto err;
    // find fd
    uint64_t file_descriptor = 0;
    Task *current_task = CURRENT_TASK;
    for (; file_descriptor < MAX_RESOURCES; file_descriptor++) {
        if (current_task->resources[file_descriptor].f)
            continue;
        current_task->resources[file_descriptor].f = newfile;
        if (!current_task->resources[file_descriptor].f)
            goto err;
        current_task->resources[file_descriptor].offset = 0;
        printf("Opened directory: %s\n", buf);
        return file_descriptor;
    }
err:
    printf("Couldn't open directory %s\n", filename);
    return -1;
}

// structure source: https://stackoverflow.com/a/12991451
struct dirent {
    int            d_ino;       /* inode number */
    size_t         d_off;       /* offset to the next dirent */
    unsigned short d_reclen;    /* length of this record */
    unsigned char  d_type;      /* type of file; not supported
                                   by all file system types */
    char           d_name[256]; /* filename */
};

enum {
    DT_UNKNOWN = 0,
    DT_FIFO    = 1,
    DT_CHR     = 2,
    DT_DIR     = 4,
    DT_BLK     = 6,
    DT_REG     = 8,
    DT_LNK     = 10,
    DT_SOCK    = 12,
    DT_WHT     = 14
};

unsigned char vfs_type_as_dirent_type(VFSFileType type) {
    switch (type) {
    case FT_REGFILE:   return DT_REG;
    case FT_DIRECTORY: return DT_DIR;
    case FT_CHARDEV:   return DT_CHR;
    default:           return DT_UNKNOWN;
    };
}

// reads the current entry in a DirIter to return then iterates
int sys_readdir(VfsDirIter *iter, struct dirent *dp) {
    VFSFileType type;
    VfsFile *entry = vfs_diriter(iter, NULL);
    if (!entry)
        return 1;
    vfs_identify(entry, dp->d_name, &type, NULL);
    dp->d_ino = (int)(uintptr_t)entry->private; // this is kinda dumb but my temporary solution is
                                                // to just make the d_ino be the lower 32 bits of the pointer
                                                // to private of the file
    dp->d_off = 0;
    dp->d_reclen = sizeof(struct dirent);
    dp->d_type = vfs_type_as_dirent_type(type);
    return 0;
}

int sys_stat(char *file, void *statbuf) {
    (void) file, (void) statbuf;
    printf("TODO: sys_stat is not implemented yet\n");
    sys_exit(-1);
    return -1;
}

int sys_ftruncate(int fd, size_t sz) {
    VfsFile *f = CURRENT_TASK->resources[fd].f;
    return vfs_truncate(f, sz);
}

int sys_dup2(int oldfd, int newfd) {
    CURRENT_TASK->resources[newfd] = CURRENT_TASK->resources[oldfd];
    return 0;
}

pid_t sys_fork();
int sys_socket(int domain, int type, int protocol);
int sys_bind(int sockfd, void *addr, int addrlen);
int sys_listen(int sockfd, int backlog);
int sys_connect(int sockfd, void *addr,
                   int addrlen);
int sys_accept(int sockfd, void *addr,
                  int *addrlen, bool block);
int sys_openpty(int *amaster, int *aslave, char *name,
            void *termp, void *winp);

void *syscalls[] = {
    sys_read,
    sys_write,
    sys_open,
    sys_close,
    sys_exit,
    sys_getpid,
    sys_fork,
    sys_execve,
    sys_kill,
    sys_times,
    sys_wait,
    sys_sbrk,
    sys_unlink,
    sys_remove,
    sys_mkdir,
    sys_lseek,
    sys_clock_gettime,
    sys_sched_yield,
    sys_mmap,
    sys_waitpid,
    sys_getcwd,
    sys_chdir,
    sys_opendir,
    sys_readdir,
    sys_stat,
    sys_socket,
    sys_bind,
    sys_listen,
    sys_connect,
    sys_accept,
    sys_msync,
    sys_ftruncate,
    sys_openpty,
    sys_dup2,
};

uint64_t num_syscalls = sizeof(syscalls) / sizeof(syscalls[0]);
