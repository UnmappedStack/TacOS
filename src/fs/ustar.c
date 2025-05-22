#include <cpu.h>
#include <fs/tempfs.h> // for creating a new file system
#include <fs/ustar.h>
#include <fs/vfs.h>
#include <kernel.h>
#include <printf.h>
#include <string.h>

void unpack_initrd(void) {
    VfsDrive ramdisk = {
        .in_memory = true,
        .fs = tempfs,
        .private = tempfs_new(),
    };
    if (vfs_mount("/", ramdisk)) {
        printf("Failed to mount initrd on root.\n");
        HALT_DEVICE();
    }
    char *at = kernel.initrd_addr;
    while (!memcmp(at + INDICATOR_OFF, "ustar", 5)) {
        size_t size = oct2bin(at + FILESIZE_OFF, 11);
        char *filename = at + FILENAME_OFF;
        bool is_dir = (*(at + TYPE_OFF) & 0b111) == 5;
        char *label = (is_dir) ? "Directory" : "File";
        printf("%s found: \"%s\", %i bytes.\n", label, filename, size);
        size_t filename_len = strlen(filename);
        if (filename_len >= 99) {
            printf("%s name is too long (>98 characters), cannot unpack "
                   "initrd. (%s name: \"%s\")\n",
                   label, label, filename);
            HALT_DEVICE();
        }
        memmove(filename + 1, filename, filename_len + 1);
        *filename = '/';
        if (is_dir) {
            filename[filename_len] = 0;
            if (mkdir(filename) < 0) {
                printf("Failed to unpack initrd: Cannot create directory: "
                       "\"%s\"\n",
                       filename);
                HALT_DEVICE();
            }
        } else {
            VfsFile *f;
            if (!(f = open(filename, O_CREAT))) {
                printf("Failed to unpack initrd: Cannot create + open file: "
                       "\"%s\"\n",
                       filename);
                HALT_DEVICE();
            }
            if (vfs_write(f, at + DATA_OFF, size, 0) < 0) {
                printf(
                    "Failed to unpack initrd: Cannot write to file: \"%s\"\n",
                    filename);
                HALT_DEVICE();
            }
            close(f);
        }
        at += (((size + 511) / 512) + 1) * 512;
    }
    printf("Unpacked initial ramdisk.\n");
}
