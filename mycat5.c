#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <stddef.h> // For size_t, NULL
#include <stdint.h> // For uintptr_t
#include <errno.h> // For errno
#include <sys/sysinfo.h> // For sysconf
#include <sys/stat.h> // For struct stat

// Function to allocate memory aligned to page size
char* align_alloc(size_t size) {
    long page_size = sysconf(_SC_PAGESIZE);
    if (page_size == -1) {
        perror("Error getting page size for align_alloc");
        page_size = 4096;
    }

    void *original_ptr = malloc(size + page_size);
    if (original_ptr == NULL) {
        return NULL;
    }

    uintptr_t aligned_addr = (uintptr_t)original_ptr + page_size;
    aligned_addr &= ~(page_size - 1);

    if ((void*)aligned_addr < original_ptr) {
        aligned_addr += page_size;
    }

    void **ptr_store = (void **)((char *)aligned_addr - sizeof(void *));
    *ptr_store = original_ptr;

    return (char *)aligned_addr;
}

// Function to free memory allocated by align_alloc
void align_free(void* ptr) {
    if (ptr == NULL) {
        return;
    }

    void **ptr_store = (void **)((char *)ptr - sizeof(void *));
    void *original_ptr = *ptr_store;

    free(original_ptr);
}

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

    // Get system page size (still useful for align_alloc)
    long page_size = sysconf(_SC_PAGESIZE);
    if (page_size == -1) {
        perror("Error getting page size");
        close(fd);
        exit(EXIT_FAILURE);
    }

    // Get file system block size (still useful for context/analysis)
    struct stat st;
    if (fstat(fd, &st) == -1) {
        perror("Error getting file status");
        close(fd);
        exit(EXIT_FAILURE);
    }
    long fs_block_size = st.st_blksize;
    
    // Based on experimentation in Task 5, set an optimal buffer size.
    // REPLACE THIS WITH YOUR EXPERIMENTALLY DETERMINED OPTIMAL SIZE
    size_t optimal_buffer_size = 32768;

    // Dynamically allocate aligned buffer
    char *buffer = align_alloc(optimal_buffer_size);
    if (buffer == NULL) {
        perror("Error allocating aligned buffer");
        close(fd);
        exit(EXIT_FAILURE);
    }

    ssize_t bytes_read;
    while ((bytes_read = read(fd, buffer, optimal_buffer_size)) > 0) {
        if (write(STDOUT_FILENO, buffer, bytes_read) != bytes_read) {
            perror("Error writing to stdout");
            align_free(buffer);
            close(fd);
            exit(EXIT_FAILURE);
        }
    }

    if (bytes_read == -1) {
        perror("Error reading file");
        align_free(buffer);
        close(fd);
        exit(EXIT_FAILURE);
    }

    align_free(buffer);
    close(fd);
    return 0;
} 