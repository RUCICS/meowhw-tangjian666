#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <stddef.h> // For NULL
#include <sys/sysinfo.h> // For getpagesize

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char *filename = argv[1];
    int fd = open(filename, O_RDONLY);
    if (fd == -1) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    // Get system page size
    long page_size = sysconf(_SC_PAGESIZE);
    if (page_size == -1) {
        perror("Error getting page size");
        close(fd);
        exit(EXIT_FAILURE);
    }

    // Dynamically allocate buffer
    char *buffer = (char *)malloc(page_size);
    if (buffer == NULL) {
        perror("Error allocating buffer");
        close(fd);
        exit(EXIT_FAILURE);
    }

    ssize_t bytes_read;
    while ((bytes_read = read(fd, buffer, page_size)) > 0) {
        if (write(STDOUT_FILENO, buffer, bytes_read) != bytes_read) {
            perror("Error writing to stdout");
            free(buffer);
            close(fd);
            exit(EXIT_FAILURE);
        }
    }

    if (bytes_read == -1) {
        perror("Error reading file");
        free(buffer);
        close(fd);
        exit(EXIT_FAILURE);
    }

    free(buffer);
    close(fd);
    return 0;
} 