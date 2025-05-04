#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>
#include <unistd.h>
#include <fcntl.h>

int main(int argc, char **argv, char **envp) {
    printf("Hello world! There are %d arguments:\n", argc);
    for (size_t i = 0; i < argc; i++) {
        printf(" -> %s\n", argv[i]);
    }
    return 0; 
}
