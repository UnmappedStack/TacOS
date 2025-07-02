#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/types.h>
#include <stdbool.h>
#include <string.h>

void lsdir(char *dir, bool show_all) {
    struct dirent *entry;
    DIR *d = opendir(dir);
    if (!d) {
        printf("ls: cannot open \'%s\': No such file or directory\n", dir);
        exit(-1);
    }
    printf("%s:\n", dir);
    while ((entry = readdir(d)) != NULL) {
        if (entry->d_name[0] == '.' && !show_all) continue;
        printf("  ");
        if (entry->d_type == DT_DIR)
            printf("\x1B[36m");
        else if (entry->d_type == DT_CHR) 
            printf("\x1B[40;33m");
        printf("%s", entry->d_name);
        if (entry->d_type != DT_REG)
            printf("\x1B[49;39m");
        printf("\n");
    }
}

int help() {
    printf("GuacUtils ls: A simple shell utility for listing the files in a directory.\n"
           "Copyright (C) 2025 Jake Steinburger (UnmappedStack) under the Mozilla Public License 2.0. "
           "See LICENSE in the source repo for more information.\n");
    return 0;
}

int main(int argc, char **argv) {
    bool listed_in_args = false;
    bool show_all = false;
    for (size_t i = 1; i < argc; i++) {
        if (argv[i][0] == '-') {
            if (argv[i][1] == '-') argv[i]++;
            // check the arg
            if (!strcmp(argv[i], "-help") || !strcmp(argv[i], "-version")) {
                return help();
            } else if (!strcmp(argv[i], "-a")) {
                show_all = true;
            }
        } else {
            // list this dir
            lsdir(argv[i], show_all);
            listed_in_args = true;
        }
    }
    if (!listed_in_args) lsdir(".", show_all);
    return 0;
}
