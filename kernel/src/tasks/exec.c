#include <exec.h>
#include <spinlock.h>
#include <fs/vfs.h>
#include <kernel.h>
#include <mem/memregion.h>
#include <mem/paging.h>
#include <mem/pmm.h>
#include <printf.h>
#include <stddef.h>
#include <string.h>

elf_program_header program_header;
elf_file_header file_header;

int verify_elf(elf_file_header *file_header) {
    if (memcmp(file_header->id,
               "\x7f"
               "ELF",
               4) || // It's not an elf file
        file_header->id[5] !=
            1 || // It's big endian but needs to be little endian
        file_header->id[4] != 2 || // It's a 32 bit ELF but needs to be 64 bit
        file_header->machine_type != 0x3E // It's not for x86_64
    ) {
        return -1;
    }
    return 0;
}

Task *task_from_pid(pid_t pid) {
    bool is_first = true;
    for (struct list *iter = kernel.scheduler.list;
         iter != kernel.scheduler.list || is_first; iter = iter->next) {
        is_first = false;
        if (((Task *)iter)->pid == pid)
            return (Task *)iter;
    }
    return NULL; // none found
}

void setup_program_args(Task *task, char **argv, uintptr_t usr_stack_paddr,
                        size_t from_end) {
    size_t argc = 0;
    size_t before_vals_off = from_end;
    for (; argv[argc]; argc++) {
        size_t len = strlen(argv[argc]) + 1;
        before_vals_off += len;
        memcpy((void *)(usr_stack_paddr + PAGE_SIZE * USER_STACK_PAGES -
                        before_vals_off + kernel.hhdm),
               argv[argc], len);
        argv[argc] = (char *)(USER_STACK_PTR - before_vals_off);
    }
    // set up the actual array pointing to each string
    size_t i = 1;
    argv[argc] = NULL;
    for (; i <= argc + 1; i++) {
        *((char **)(usr_stack_paddr + PAGE_SIZE * USER_STACK_PAGES -
                    before_vals_off - i * sizeof(char *) + kernel.hhdm)) =
            argv[argc + 1 - i];
    }
    i--;
    if (from_end)
        task->envp =
            (char **)(USER_STACK_PTR - before_vals_off - i * sizeof(char *));
    else
        task->argc = argc,
        task->argv =
            (char **)(USER_STACK_PTR - before_vals_off - i * sizeof(char *));
}

extern Spinlock scheduler_lock;
int execve(Task *task, char *filename, char **argv, char **envp) {
    printf("Start execve on %s\n", filename);
    // get argc + copy argument strings to end of new stack
    uintptr_t usr_stack_paddr = kmalloc(USER_STACK_PAGES);
    setup_program_args(task, argv, usr_stack_paddr, /* stack end offset */ 0);
    setup_program_args(task, envp, usr_stack_paddr,
                       /* stack end offset */ USER_STACK_PTR -
                           (uintptr_t)task->argv);
    task->first_rsp = task->rsp = (uintptr_t)task->envp - 16;
    // parse and load the elf
    task->flags = TASK_RUNNING;
    VfsFile *f = open(filename, 0);
    if (!f) {
        printf("Failed to open file \"%s\", execve failed.\n", filename);
        return -1;
    }
    if (vfs_read(f, (char *)&file_header, sizeof(elf_file_header), 0) < 0) {
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
    size_t end_of_executable = 0;
    for (size_t i = 0; i < file_header.program_header_entry_count; i++) {
        if (vfs_read(f, (char *)&program_header, sizeof(elf_program_header),
                     offset) < 0)
            goto elf_read_err;
        if (program_header.type == 1 && program_header.virtual_address) {
            uint64_t header_data_phys = kmalloc(
                bytes_to_pages(PAGE_ALIGN_UP(program_header.size_in_memory)));
            if (vfs_read(f, (void *)(header_data_phys + kernel.hhdm),
                         program_header.size_in_file,
                         program_header.offset) < 0)
                goto elf_read_err;
            uint64_t flags = KERNEL_PFLAG_USER | KERNEL_PFLAG_PRESENT;
            uint64_t new_eoe =
                program_header.virtual_address + program_header.size_in_memory;
            if (new_eoe > end_of_executable)
                end_of_executable = new_eoe;
            if (!(program_header.flags & ELF_FLAG_WRITABLE))
                flags |= KERNEL_PFLAG_WRITE;
            map_pages((uint64_t *)(task->pml4 + kernel.hhdm),
                      program_header.virtual_address, header_data_phys,
                      bytes_to_pages(program_header.size_in_memory), flags);
            add_memregion(&task->memregion_list, program_header.virtual_address,
                          PAGE_ALIGN_UP(program_header.size_in_memory) / 4096, true, flags);
            printf("Header with type = %i, num %i, vaddr = %p, off = %i, size_vmem "
                   "= %i, is writable = %i\n",
                   program_header.type, i, program_header.virtual_address,
                   program_header.offset, program_header.size_in_memory,
                   (uint64_t)(program_header.flags & ELF_FLAG_WRITABLE));
        }
        offset += file_header.program_header_entry_size;
    }
    map_pages((uint64_t *)(task->pml4 + kernel.hhdm), USER_STACK_ADDR,
              usr_stack_paddr, USER_STACK_PAGES,
              KERNEL_PFLAG_WRITE | KERNEL_PFLAG_USER | KERNEL_PFLAG_PRESENT);
    add_memregion(
        &task->memregion_list, USER_STACK_ADDR, USER_STACK_PAGES, true,
        KERNEL_PFLAG_WRITE | KERNEL_PFLAG_USER | KERNEL_PFLAG_PRESENT);
    task->entry = (void *)file_header.entry;
    task->program_break = PAGE_ALIGN_UP(end_of_executable);
    alloc_pages((uint64_t *)(task->pml4 + kernel.hhdm), task->program_break, 1,
                KERNEL_PFLAG_WRITE | KERNEL_PFLAG_PRESENT | KERNEL_PFLAG_USER);
    add_memregion(&task->memregion_list, task->program_break, 1, true,
                  KERNEL_PFLAG_WRITE | KERNEL_PFLAG_PRESENT |
                      KERNEL_PFLAG_USER);
    close(f);
    task->flags = TASK_FIRST_EXEC | TASK_PRESENT;
    printf("\n -> execve(): Task %i has flags 0b%b, task ptr = 0x%p\n\n",
           task->pid, task->flags, &task->flags);
    return 0;
}
