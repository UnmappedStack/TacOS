#include <stdio.h>

int main(int argc, char **argv, char **envp) {
    printf("Hello world from another program spawned by init! There are %d arguments:\n", argc);
    for (size_t i = 0; i < argc; i++) {
        printf(" -> %s\n", argv[i]);
    }
    return 0;
}
