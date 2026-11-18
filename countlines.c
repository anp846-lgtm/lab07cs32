#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>

#define BUFFER_SIZE 4096

static pid_t start_command(char *argv[], int *read_fd) {
    int fds[2];
    if (pipe(fds) < 0) {
        perror("pipe");
        return -1;
    }
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        close(fds[0]);
        close(fds[1]);
        return -1;
    }
    if (pid == 0) {
        close(fds[0]);
        if (dup2(fds[1], STDOUT_FILENO) < 0) {
            perror("dup2");
            close(fds[1]);
            _exit(1);
        }
        if (fds[1] != STDOUT_FILENO) close(fds[1]);
        execvp(argv[0], argv);
        perror(argv[0]);
        _exit(127);
    }
    close(fds[1]);
    *read_fd = fds[0];
    return pid;
}

static int count_lines(int fd) {
    char buffer[BUFFER_SIZE];
    int lines = 0;
    for (;;) {
        ssize_t n = read(fd, buffer, sizeof(buffer));
        if (n == 0) return lines;
        if (n < 0) {
            if (errno == EINTR) continue;
            perror("read");
            return -1;
        }
        for (ssize_t i = 0; i < n; i++) {
            if (buffer[i] == '\n') lines++;
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: countlines COMMAND [ARG]...\n");
        return 1;
    }
    int fd;
    pid_t pid = start_command(argv + 1, &fd);
    if (pid < 0) return 1;
    int lines = count_lines(fd);
    int close_result = close(fd);
    if (close_result < 0) perror("close");
    int status;
    while (waitpid(pid, &status, 0) < 0) {
        if (errno == EINTR) continue;
        perror("waitpid");
        return 1;
    }
    if (lines < 0 || close_result < 0) return 1;
    int exit_status;
    if (WIFEXITED(status)) exit_status = WEXITSTATUS(status);
    else if (WIFSIGNALED(status)) exit_status = 128 + WTERMSIG(status);
    else return 1;
    printf("lines: %d\nexit status: %d\n", lines, exit_status);
    return 0;
}
