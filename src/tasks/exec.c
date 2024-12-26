#include <mem/memregion.h>
#include <exec.h>
#include <scheduler.h>
#include <kernel.h>
#include <mem/pmm.h>
#include <mem/paging.h>
#include <string.h>
#include <fs/vfs.h>
#include <printf.h>
#include <stddef.h>

#define CURRENT_TASK kernel.scheduler.current_task

int verify_elf(elf_file_header *file_header) {
    if (
        memcmp(file_header->id, "\x7f" "ELF", 4) || // It's not an elf file
        file_header->id[5] != 1 || // It's big endian but needs to be little endian
        file_header->id[4] != 2 || // It's a 32 bit ELF but needs to be 64 bit
        file_header->machine_type != 0x3E // It's not for x86_64
    ) {
        return -1;
    }
    return 0;
}

// TODO: Add argv and envp
int execve(char *filename) {
    VfsFile *f = open(filename, 0);
    if (!f) {
        printf("Failed to open file \"%s\".\n", filename);
        return -1;
    }
    elf_file_header file_header;
    if (vfs_read(f, (char*) &file_header, sizeof(elf_file_header), 0) < 0) {
        elf_read_err:
        printf("Failed to read from file \"%s\".\n", filename);
        elf_generic_err:
        close(f);
        return -1;
    }
    if (verify_elf(&file_header) < 0) {
        printf("Invalid ELF file.\n");
        goto elf_generic_err;
    }
    size_t offset = file_header.program_header_offset;
    elf_program_header program_header;
    for (size_t i = 0; i < file_header.program_header_entry_count; i++) {
        if (vfs_read(f, (char*) &program_header, sizeof(elf_file_header), offset) < 0)
            goto elf_read_err;
        if (program_header.type == 1) {
            uint64_t header_data_phys = kmalloc(bytes_to_pages(PAGE_ALIGN_UP(program_header.size_in_memory)));
            if (vfs_read(f, (void*) (header_data_phys + kernel.hhdm), program_header.size_in_file, program_header.offset) < 0)
                goto elf_read_err;
            uint64_t flags = KERNEL_PFLAG_USER | KERNEL_PFLAG_PRESENT;
            if (!(program_header.flags & ELF_FLAG_WRITABLE))
                flags |= KERNEL_PFLAG_WRITE;
            map_pages((uint64_t*) (CURRENT_TASK->pml4 + kernel.hhdm), program_header.virtual_address, header_data_phys, bytes_to_pages(program_header.size_in_memory), flags);
            add_memregion(&CURRENT_TASK->memregion_list, program_header.virtual_address, program_header.size_in_memory, true, flags);
        }
        printf("Header with type = %i, num %i, off = %i, size_vmem = %i\n", program_header.type, i, program_header.offset, program_header.size_in_memory);
        offset += file_header.program_header_entry_size;
    }
    alloc_pages((uint64_t*) (CURRENT_TASK->pml4 + kernel.hhdm), USER_STACK_ADDR, USER_STACK_PAGES, KERNEL_PFLAG_WRITE | KERNEL_PFLAG_USER | KERNEL_PFLAG_PRESENT);
    add_memregion(&CURRENT_TASK->memregion_list, USER_STACK_ADDR, false, USER_STACK_PAGES, KERNEL_PFLAG_WRITE | KERNEL_PFLAG_USER | KERNEL_PFLAG_PRESENT);
    CURRENT_TASK->entry = (void*) file_header.entry;
    CURRENT_TASK->flags = TASK_FIRST_EXEC | TASK_PRESENT;
    close(f);
    return 0;
}
