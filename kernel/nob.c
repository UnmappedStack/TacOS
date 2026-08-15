/* Build system for the kernel, written in C using the very nifty nob.h library. 
 * I already really hate this build system. Like, a lot.
 *
 * TODO: rewrite it completely, this shit is unsalvagable. */

#define ARENA_IMPLEMENTATION 
#define NOB_IMPLEMENTATION
#define NOB_NO_ECHO
#include "nob.h"
#include "arena.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

static Arena arena = {0}; // this maybe shouldn't be global but whatever

#define ARRLEN(arr) (sizeof(arr)/sizeof(arr[0]))

#define OBJDIR "obj"
#define BINDIR "bin"

#define OUTPUT_PATH "bin/tacos"

#define LD "ld"
#define CC "cc"
const char *cflags[] = {
	"-fno-builtin",
    "-Wall",
    "-Wextra",
	"-Werror",
	"-ffreestanding",
    "-fno-stack-protector",
    "-fno-PIE",
    "-m64",
    "-MMD",
    "-MP",
    "-mno-80387",
    "-mno-mmx",
	"-g",
    "-mno-sse",
    "-mno-sse2",
	"-fno-omit-frame-pointer",
    "-mno-red-zone",
	"-O0",
	"-mcmodel=kernel",
	"-std=c23",
    "-pipe",
    "-Iinclude",
    "-c", 
};

const char *nasmflags[] = {
    "-Wall",
    "-felf64",
    "-w-reloc-abs-dword",
    "-w-reloc-rel-dword",
};

const char *ldflags[] = {
    "-z", "max-page-size=0x1000",
    "-T", "linker.ld",
};

typedef enum {
    X86_64, RISCV64, NUM_ARCHES
} Arch;

const struct {
#define MAX_FLAGS 25
    char *stringified;
    char *march_flag;
    char *link_target;
    char *cflags[MAX_FLAGS];
    char *ldflags[MAX_FLAGS];
    char *cc;
    char *ld;
} arches[] = {
    [X86_64] = {
        .stringified = "x86_64",
        .march_flag = "-march=x86-64",
        .link_target = "elf_x86_64",
        .cflags = {},
        .ldflags = {},
        .cc = "cc",
        .ld = "ld",
    },
    [RISCV64] = {
        .stringified = "riscv64",
        .march_flag = "-march=riscv64",
        .link_target = "elf_riscv64",
        .cflags = {},
        .ldflags = {},
        .cc = "cc",
        .ld = "ld",
    },
};

Arch str_to_arch(const char *s) {
    for (Arch arch = 0; arch < NUM_ARCHES; arch++) {
        if (!strcmp(arches[arch].stringified, s)) return arch;
    }
    return -1;
}

void replace_char_with_char(char *s, char from, char to) {
    for (; *s; s++) {
        if (*s != from) continue;
        *s = to;
    }
}

// invokee -> cc? nasm? what should be invoked
// flags -> compiler/assembler flags
// path -> file to build
int build_source(const char *path, const char **flags, int num_flags, const char *invokee, Arch arch) {
    const char *path_no_src = &path[4]; // skip `src/`
    Cmd cmd = {0};
    char *output = temp_sprintf(OBJDIR "/%s.o", path_no_src);
    replace_char_with_char(output, '/', '_');
    output[3] = '/'; // we want the obj/ to still be / not _
    
    cmd_append(&cmd, invokee, "-o", output, path);
    for (int i = 0; i < num_flags; i++)
        cmd_append(&cmd, flags[i]);

    // if its less than 0 it's nasm and we don't need it
    if ((int)arch >= 0)
        cmd_append(&cmd, arches[arch].march_flag);

    if (!cmd_run(&cmd)) {
        printf("Failed to build %s\n", path);
        return -1;
    }
    return 0;
}

int compile_c_source(const char *path, Arch archflag) {
    return build_source(path, cflags, ARRLEN(cflags), CC, archflag);
}

int compile_nasm_source(const char *path) {
    return build_source(path, nasmflags, ARRLEN(nasmflags), "nasm", -1);
}

int build_source_file(const char *path, Arch arch) {
    const char *extension = &strrchr(path, '.')[1];
    if (!strcmp(extension, "c")) {
        return compile_c_source(path, arch);
    } else if (!strcmp(extension, "asm")) {
        return compile_nasm_source(path);
    }
    fprintf(stderr, "Unexpected file type of %s (%s), cannot build.\n");
    return -1;
}

int search_and_build_dir(const char *path, Arch target_arch) {
    Dir_Entry dir;
    if (!dir_entry_open(path, &dir)) return -1;

    for (;;) {
        if (!dir_entry_next(&dir)) {
            if (dir.error) return -1;
            else break;
        }
        if (dir.name[0] == '.') continue;

        char *child_path = temp_sprintf("%s/%s", path, dir.name);

        if (get_file_type(child_path) == NOB_FILE_DIRECTORY) {
            if (!memcmp(&path[strlen(path)-3], "isa", 4) &&
                    strcmp(dir.name, arches[target_arch].stringified)) {
                /* dir for architecture-specific stuff
                 * which is not the target arch we want */
                continue;
            }
            if (!search_and_build_dir(child_path, target_arch) < 0) return -1;
            continue;
        }
        printf(" > %s\n", child_path);
        if (build_source_file(child_path, target_arch) < 0) {
            printf("[ERROR] Failed to build %s\n", child_path);
            return -1;
        }
    }

    return 0;
}

// quite simple compared to building, and no recursive stuff is required.
int link_to_executable(Arch arch) {
    Dir_Entry dir;
    Cmd cmd = {0};
    cmd_append(&cmd, LD);

    if (!dir_entry_open(OBJDIR, &dir)) return -1;
    for (;;) {
        if (!dir_entry_next(&dir)) {
            if (dir.error) return -1;
            else break;
        }
        if (strcmp(strrchr(dir.name, '.'), ".o")) continue; // not object file
        char *object = temp_sprintf(OBJDIR "/%s", dir.name);
        cmd_append(&cmd, object);
    }

    for (int i = 0; i < ARRLEN(ldflags); i++)
        cmd_append(&cmd, ldflags[i]);

    cmd_append(&cmd, "-o", OUTPUT_PATH);

    cmd_append(&cmd, "-m", arches[arch].link_target);

    if (!cmd_run(&cmd)) return -1;
    return 0;
}

int main(int argc, char **argv) {
    GO_REBUILD_URSELF(argc, argv);
    int e;

    // if we're running from repo root we need to go into the kernel dir
    const char *cwd = strrchr(get_current_dir_temp(), '/')+1;
    if (strcmp(cwd, "kernel") != 0) {
        if (!set_current_dir("kernel")) {
            printf(" !! Kernel buildsystem must be run from either repo root or /kernel/.\n");
            return -1;
        }
    }

    // TODO: nob gives us an easier way to do cmdline parsing
    Arch target_arch = -1;
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--arch")) {
            target_arch = str_to_arch(argv[++i]);
            printf(" # Select target architecture %s\n", arches[target_arch].stringified);
        } else {
            fprintf(stderr, " !! Unexpected flag/argument %s\n", argv[i]);
            return -1;
        }
    }

    if ((int)target_arch < 0) {
        fprintf(stderr, "Must specify target architecture (--arch [arch])\n"); // TODO: list ISAs
        return -1;
    }

    if (!mkdir_if_not_exists(OBJDIR)) return -1;
    if (!mkdir_if_not_exists(BINDIR)) return -1;

    printf("[Collecting + building source files]\n");
    if ((e=search_and_build_dir("src", target_arch)) < 0) return e;

    printf("[Linking kernel binary]\n");
    if ((e=link_to_executable(target_arch)) < 0) return e;

    printf("[Kernel image built to %s successfully]\n", OUTPUT_PATH);

    arena_free(&arena);

}
