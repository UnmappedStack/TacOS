#include <stdlib.h>
#include <stdio.h>

int main(int argc, char **argv, char **envp) {
  if (argc > 1 && argv[1][0] == '-') {
    printf("GuacUtils yes: A simple shell utility for printing a string repeatedly to standard output.\n"
           "Originally written by Luka Talevski. Copyright (C) 2025 Jake "
           "Steinburger (UnmappedStack)\n"
           "under the Mozilla Public License 2.0. \n"
           "See LICENSE in the source repo for more information.\n");
    exit(-1);
  }

  if (argc == 2) {
    for(;;) {
      printf("%s\n", argv[1]);
    }
  } else {
    for(;;) {
      printf("y\n");
    }
  }

  printf("You aren't supposed to be here...\n");
  return 1;
}
