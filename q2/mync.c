#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

int main(int argc, char *argv[]) {
    if (argc != 3 || strcmp(argv[1], "-e") != 0) {
        fprintf(stderr, "Usage: %s -e <command>\n", argv[0]);
        return EXIT_FAILURE;
    }

    // Fork a new process
    // As shown in the beej Guide part 2.2 fork()

    pid_t pid = fork();
    switch (pid) {
        case -1:// error
            // Fork failed
            perror("fork");
            return EXIT_FAILURE;
        case 0:// !fork()
            // Child process
            // Execute the command using execl
            execl("/bin/sh", "sh", "-c", argv[2], (char *)NULL);
            // If execl returns, it must have failed
            perror("execl");
            return EXIT_FAILURE;
        default://  fork()
            // Parent process
            // Wait for the child process to complete
            if (waitpid(pid, NULL, 0) < 0) {
                perror("waitpid");
                return EXIT_FAILURE;
            }
    }
    return EXIT_SUCCESS;
}