/* Note that device is one of the only parts of the kernel besides the VFS and
 * the TempFS itselfs that interacts with both of them. */

#include <fs/device.h>
#include <fs/tempfs.h>
#include <fs/vfs.h>
#include <printf.h>

int test_open(void *f) {
    printf("test open with f = %p\n", f);
    return 0;
}

int test_close(void *f) {
    printf("test close with f = %p\n", f);
    return 0;
}

int test_write(void *f, char *buf, size_t len, size_t off) {
    printf("test write with f = %p, txt = %s, len = %i, off = %i\n", f, buf,
           len, off);
    return 0;
}

int test_read(void *f, char *buf, size_t len, size_t off) {
    printf("test read with f = %p, txt = %s, len = %i, off = %i\n", f, buf, len,
           off);
    return 0;
}

void init_devices(void) {
    VfsDrive drive = (VfsDrive){
        .in_memory = true,
        .fs = tempfs,
        .private = tempfs_new(),
    };
    mkdir("/dev");
    vfs_mount("/dev", drive);
    printf("Initiated devices.\n");
}

VfsFile *mkdevice(char *path, DeviceOps ops) {
    VfsFile *new_file = vfs_access(path, O_CREAT, VAT_mkfile);
    if (!new_file)
        return NULL;
    TempfsInode *private = new_file->private;
    private->devops = ops;
    private->type = Device;
    printf("Created new device at %s, addr = %p\n", path, new_file);
    return new_file;
}
