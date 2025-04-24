#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/types.h>

void lsdir(char *dir) {
    struct dirent *entry;
    DIR *d = opendir(dir);
    if (!d) {
        printf("ls: cannot open \'%s\': No such file or directory\n", dir);
        exit(-1);
    }
    printf("%s:\n", dir);
    while ((entry = readdir(d)) != NULL) {
        printf(" %s%s: %zu bytes\n", entry->d_name, (entry->d_isdir) ? " [DIR]" : "", entry->d_fsize);
    }
}

int main(int argc, char **argv) {
    if (argc > 1 && argv[1][0] == '-') {
        printf("GuacUtils ls: A simple shell utility for listing the files in a directory.\n"
               "Copyright (C) 2025 Jake Steinburger (UnmappedStack) under the Mozilla Public License 2.0. "
               "See LICENSE in the source repo for more information.\n");
        exit(-1);
    }
    if (argc <= 1) {
        char cwd[30];
        getcwd(cwd, 30);
        lsdir(cwd);
        return 0;
    }
    for (size_t i = 1; i < argc; i++) lsdir(argv[i]);
    return 0;
}
