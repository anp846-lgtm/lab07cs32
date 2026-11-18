#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>

#define BUFFER_SIZE 4096

static int copy_fd(int in_fd, int out_fd) {
    char buffer[BUFFER_SIZE];
    ssize_t n;
    while ((n = read(in_fd, buffer, sizeof(buffer))) > 0) {
        ssize_t sent = 0;
        while (sent < n) {
            ssize_t written = write(out_fd, buffer + sent, n - sent);
            if (written < 0) return -1;
            if (written == 0) {
                errno = EIO;
                return -1;
            }
            sent += written;
        }
    }
    return n < 0 ? -1 : 0;
}

static int cat_file(const char *path) {
    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        perror(path);
        return -1;
    }
    int result = copy_fd(fd, STDOUT_FILENO);
    if (result < 0) perror(path);
    if (close(fd) < 0) {
        perror(path);
        return -1;
    }
    return result;
}

int main(int argc, char *argv[]) {
    // No file names: copy standard input (fd 0) to standard output (fd 1)
    if (argc == 1) {
        if (copy_fd(STDIN_FILENO, STDOUT_FILENO) < 0) {
            perror("mycat");
            return 1;
        }
        return 0;
    }

    // Print each file in order. If one fails, keep going with the rest,
    // but exit with status 1 at the end.
    int status = 0;
    for (int i = 1; i < argc; i++) {
        if (cat_file(argv[i]) < 0) {
            status = 1;
        }
    }
    return status;
}
