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
    if (!strcmp(program_file, "init") && !strcmp(argv[1], "YOU_ARE_INIT")) {
        // safeguard to prevent init being called as if it's actually the first process.
        printf("ERROR: Stop trying to break TacOS, the shell has a safeguard for people like you :P\n");
        return;
    }
    pid_t pid = fork();
    if (pid == 0) {
        // hey... I'm not the original! I'm a child???
        execvp(program_file, argv);
        printf("shell: command not found: %s\n", program_file);
        exit(127);
    } else {
        // I'm the original, suck it! (and wait)
        int status;
        waitpid(pid, &status, 0);
    }
}

// this is so horrible. I've just given up on making it decent.
void run_cmd(char *cmd) {
    if (!*cmd || *cmd == '#') return;
    size_t len = strlen(cmd);
    if (cmd[len - 1] != '\n') {
        char *original = cmd;
        cmd = malloc(len + 2);
        sprintf(cmd, "%s\n", original);
    }
    char *argv[30];
    memset(argv, 0, 30 * sizeof(char*));
    size_t argc = 0;
    char *start = cmd;
    char *end = cmd;
    for (; end == cmd || (*(end - 1) != '\0' && *(end - 1) != '\n'); end++) {
        if (*end != ' ' && *end != '\n' && *end != '\0') continue;
        *end = 0;
        argv[argc++] = start;
        start = ++end;
    }
    if (!strcmp(argv[0], "exit")) exit(0);
    else if (!strcmp(argv[0], "cd")) {
        if (!argv[1] || argv[2]) return;
        if (chdir(argv[1]) < 0)
            printf("cd: no such directory: %s\n", argv[1]);
        return;
    }
    run_program(argv[0], argv);
}

void shell_mode() {
    char input[200];
    char cwd[200];
    for (;;) {
        getcwd(cwd, 200);
        printf(GREEN "user@TacOS" RESET ":" BLUE "%s" RESET"$ ", cwd);
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
    else if (!strcmp(argv[1], "-c")) {
        run_cmd(argv[2]);
        return 0;
    } else if (argv[1][0] == '-') {
        printf("GuacUtils shell: A simple CLI shell.\n"
               "Copyright (C) 2025 Jake Steinburger (UnmappedStack) under the Mozilla Public License 2.0. "
               "See LICENSE in the source repo for more information.\n");
        exit(-1);
    }
    printf("Invalid argument for shell. Run `%s --help`.\n", argv[0]);
}
