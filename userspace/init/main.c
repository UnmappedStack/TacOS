#include <unistd.h>
#include <stdio.h>

int main(int argc, char **argv) {
    stdin  = fopen("/home/longfile.txt", "r"); // TODO: keyboard input + stdin stream
    stdout = fopen("/dev/tty0", "w");
    stderr = fopen("/dev/tty0", "w"); // same device, but no IO buffering
    setvbuf(stdin, NULL, _IOLBF, 0);
    setvbuf(stdout, NULL, _IOLBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);
    if (stdin->fd != 0) {
        fprintf(stderr, "stdin is wrong file descriptor (init)! (fd = %d)\n", stdin->fd);
        return 1;
    }
    if (stdout->fd != 1) {
        fprintf(stderr, "stdout is wrong file descriptor (init)! (fd = %d)\n", stdout->fd);
        return 1;
    }
    if (stderr->fd != 2) {
        fprintf(stderr, "stderr is wrong file descriptor (init)! (fd = %d)\n", stderr->fd);
        return 1;
    }
    printf("[INIT] Initiated streams.\n");
    printf("[INIT] Printing %d argument(s) passed from kernel\n", argc);
    for (size_t i = 0; i < argc; i++) {
        printf("        -> %s\n", argv[i]);
    }
    printf("[INIT] Spawning child.\n\n");
    pid_t pid = fork();
    if (!pid) {
        char *args[] = {"helloworld", "no", NULL};
        printf("args is %p, args[0] = %p\n", args, args[0]);
        execve("/usr/bin/helloworld", args);
        printf("[INIT] ERROR: init failed to execute child.\n");
    }
    for (;;);
    return 0;
}
