#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char **argv, char **envp) {
  if (argc > 1 && argv[1][0] == '-') {
    printf("GuacUtils pwd: A simple shell utility for checking the current "
           "directory.\n"
           "Originally written by Luka Talevski. Copyright (C) 2025 Jake "
           "Steinburger (UnmappedStack)\n"
           "under the Mozilla Public License 2.0. \n"
           "See LICENSE in the source repo for more information.\n");
    exit(-1);
  }
  char working_dir[4096];

  if (getcwd(working_dir, sizeof(working_dir) / sizeof(working_dir[0])) !=
      NULL) {
    printf("%s\n", working_dir);
  } else {
    printf("ERROR: getcwd() returned NULL.\n");
    return 1;
  }
  return 0;
}
