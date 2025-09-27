#include <printf.h>
#include <pty.h>
#include <kernel.h>
#include <fs/tempfs.h>
#include <fs/device.h>
#include <fs/vfs.h>
#include <string.h>

void init_usrptys(void) {
    kernel.usrptys_cache = init_slab_cache(sizeof(PtyDev), "User PTYs Cache");
}

static int next_avaliable_resource_slot(void) {
    for (size_t i = 0; i < MAX_RESOURCES; i++) {
        if (CURRENT_TASK->resources[i].f) continue;
        return i;
    }
    return -1;
}

int pty_write(void *file, char *buf, size_t len, size_t offset) {
    (void) offset;
    PtyDev *dev = (PtyDev*) ((TempfsInode*)file)->private;
    return ringbuf_write(&dev->other->data, len, buf);
}

int pty_read(void *file, char *buf, size_t len, size_t offset) {
    (void) offset;
    PtyDev *dev = (PtyDev*) ((TempfsInode*)file)->private;
    if (dev->is_master)
        return ringbuf_read(&dev->data, len, buf);
    int ret;
    while ((ret=ringbuf_read(&dev->data, len, buf)) < 1);
    return ret;
}

int sys_openpty(int *amaster, int *aslave, char *name,
            void *termp, void *winp) {
    if (termp || winp) {
        printf("termp/winp stuff isn't implemented yet (TODO), make it NULL\n");
        return -1;
    }
    mkdir("/dev/pts"); // doesn't matter if it fails cos it exists already
    char fnamealt[3];
    static uint64_t ptynum = 0;
    if (!name) {
        uint64_to_string(ptynum++, fnamealt);
        name = fnamealt;
    }
    char path[30] = "/dev/pts/";
    strcpy(&path[9], name);

    VfsFile *master = slab_alloc(kernel.vfs_file_cache);
    VfsFile *slave  = vfs_access(path, 0, VAT_mkfile);
    TempfsInode *master_tprivate = master->private;
    TempfsInode *slave_tprivate  =  slave->private;
    master_tprivate->type = slave_tprivate->type = FT_CHARDEV;
    master->ops = tempfs_regfile_ops; // read/write will be changed later on to be separate
    master_tprivate->private = slab_alloc(kernel.usrptys_cache);
    slave_tprivate->private  = slab_alloc(kernel.usrptys_cache);
    master->ops.write_fn = slave->ops.write_fn = pty_write;
    master->ops.read_fn  = slave->ops.read_fn  = pty_read;
    ((PtyDev*) master_tprivate->private)->other = (PtyDev*) slave_tprivate->private;
    ((PtyDev*) slave_tprivate->private)->other = (PtyDev*) master_tprivate->private;
    ringbuffer_init(&((PtyDev*) master_tprivate->private)->data);
    ringbuffer_init(&((PtyDev*) slave_tprivate->private)->data);
    ((PtyDev*)master_tprivate->private)->is_master = true;
    ((PtyDev*)slave_tprivate->private)->is_master  = false;

    int master_resource = next_avaliable_resource_slot();
    if (master_resource < 0) return -1;
    CURRENT_TASK->resources[master_resource].f = master;
    int slave_resource = next_avaliable_resource_slot();
    if (slave_resource < 0) return -1;
    CURRENT_TASK->resources[slave_resource].f = slave;
    
    *aslave = slave_resource, *amaster = master_resource;

    return 0;
}
