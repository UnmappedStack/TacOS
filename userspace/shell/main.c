#include <stddef.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/wait.h>
#include "utils.h"

// forks, execs, then waits
void run_program(char *program_file, char *argv[]) {
    pid_t pid = fork();
    if (pid == 0) {
        // hey... I'm not the original! I'm a child???
        execvp(program_file, argv);
        exit(127);
    } else {
        // I'm the original, suck it! (and wait)
        int status;
        waitpid(pid, &status, 0);
    }
}

void run_cmd(char *cmd) {
    if (!*cmd) return;
    char *argv[10];
    memset(argv, 0, 10 * sizeof(char*));
    size_t argc = 0;
    char *start = cmd;
    char *end = cmd;
    for (; end == cmd || *(end - 1) != '\0'; end++) {
        if (*end != ' ' && *end != '\0') continue;
        *end = 0;
        argv[argc++] = start;
        start = ++end;
    }
    if (!strcmp(argv[0], "exit")) exit(0);
    else if (!strcmp(argv[0], "cd")) {
        if (!argv[1] || argv[2]) {
            printf("Shell: cd expects one argument being the new current working directory.\n");
            return;
        }
        chdir(argv[1]);
        return;
    }
    run_program(argv[0], argv);
}

void shell_mode() {
    char input[200];
    char cwd[200];
    for (;;) {
        getcwd(cwd, 200);
        printf("[user@TacOS %s]$ ", cwd);
        fflush(stdout);
        memset(input, 0, 200);
        fgets(input, 200, stdin);
        printf("\n");
        run_cmd(input);
    }
}

int main(int argc, char **argv) {
    setenv("SHELL", "unbash", true);
    if (argc == 1)
        shell_mode();
    else
        printf("Too many arguments for unbash.\n");
}
