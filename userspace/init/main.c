#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <stdio.h>

extern int is_init;

char *ascii_art = " ________                     ______    ______  \n"
"|        \\                   /      \\  /      \\\n" 
" \\$$$$$$$$______    _______ |  $$$$$$\\|  $$$$$$\\\n"
"   | $$  |      \\  /       \\| $$  | $$| $$___\\$$\n"
"   | $$   \\$$$$$$\\|  $$$$$$$| $$  | $$ \\$$    \\ \n"
"   | $$  /      $$| $$      | $$  | $$ _\\$$$$$$\\\n"
"   | $$ |  $$$$$$$| $$_____ | $$__/ $$|  \\__| $$\n"
"   | $$  \\$$    $$ \\$$     \\ \\$$    $$ \\$$    $$\n"
"    \\$$   \\$$$$$$$  \\$$$$$$$  \\$$$$$$   \\$$$$$$ \n";

int main(int argc, char **argv, char **envp) {
    if (argc <= 1 || (strcmp(argv[1], "YOU_ARE_INIT") && argv[1][0] != '-')) {
        // Seems like init was called by another user program rather than the kernel.
        // It should be safe to assume that the streams are aready open.
        printf("sysinit cannot be run manually, must be called by kernel. Try run `init --help`.\n");
        return -1;
    } else if (argv[1][0] == '-') {
        // Again, likely not actually init
        printf("sysinit: The init daemon for TacOS which sets up userspace for other programs and enters the shell.\n"
               "Copyright (C) 2025 Jake Steinburger (UnmappedStack) under the Mozilla Public License 2.0. "
               "See LICENSE in the source repo for more information.\n");
        return -1;
    }
    is_init = 1;
    stdin  = fopen("/dev/stdin0", "r");
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
    setenv("PATH", "/usr/bin", false);
    chdir("/home");
    pid_t pid = fork();
    if (pid) {
        int status;
        waitpid(pid, &status, 0);
        printf("\n[INIT] Child shell has finished executing.\n");
    } else {
        printf("%s\nWelcome to TacOS! This message is from /usr/bin/init. Spawning shell (/usr/bin/shell) now.\n"
               "See /home/README.txt for more information.\n", ascii_art);
        execvp("/usr/bin/shell", (char*[]) {"/usr/bin/shell", NULL});
        printf("[INIT] ERROR: init failed to execute child.\n");
    }
    for (;;);
    return 0;
}
