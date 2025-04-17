#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

int main(int argc, char **argv, char **envp) {
    printf("Hello world from another program spawned by init! There are %d arguments:\n", argc);
    for (size_t i = 0; i < argc; i++) {
        printf(" -> %s\n", argv[i]);
    }
    char buf[200];
    printf("helloworld got cwd %s\n", getcwd(buf, 100));
    fgets(buf, 199, stdin);
    printf("\n\nGot input from keyboard: %s\n", buf);
    return 0;
}
