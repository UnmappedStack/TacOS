/* Note that device is one of the only parts of the kernel besides the VFS and
 * the TempFS itselfs that interacts with both of them. */

#include <fs/device.h>
#include <fs/tempfs.h>
#include <fs/vfs.h>
#include <printf.h>

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
    private->ops = (FSOps) {
        .open_fn     = (int (*)(void **, void *))ops.open,
        .close_fn    = (int (*)(void *))ops.close,
        .write_fn    = (int (*)(void *, char *, size_t, size_t))ops.write,
        .read_fn     = (int (*)(void *, char *, size_t, size_t))ops.read,
        .identify_fn = (int (*)(void *, char *, bool *, size_t *))tempfs_identify,
    };
    new_file->ops = private->ops;
    private->type = Device;
    printf("Created new device at %s, addr = %p\n", path, new_file);
    return new_file;
}
