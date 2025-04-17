#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <stdio.h>

int main(int argc, char **argv, char **envp) {
    stdin  = fopen("/dev/kb0", "r");
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
    setenv("PATH", "/usr/bin", false);
    printf("[INIT] Iniatiated $PATH\n");
    chdir("/");
    printf("[INIT] Set initial current working directory\n");
    printf("[INIT] Listing %d argument(s) passed from kernel\n", argc);
    for (size_t i = 0; i < argc; i++) {
        printf("        -> %s\n", argv[i]);
    }
    printf("[INIT] Listing environmental variables.\n");
    for (size_t i = 0; envp[i]; i++) {
        printf("        -> %s\n", envp[i]);
    }
    printf("[INIT] Spawning child.\n\n");
    pid_t pid = fork();
    if (pid) {
        int status;
        waitpid(2, &status, 0);
        printf("\n[INIT] Child has finished executing.\n");
    } else {
        execvp("/usr/bin/helloworld", (char*[]) {"helloworld", "i_am_taco", NULL});
        printf("[INIT] ERROR: init failed to execute child.\n");
    }
    for (;;);
    return 0;
}
