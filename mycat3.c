#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <stddef.h> // For size_t, NULL
#include <stdint.h> // For uintptr_t
#include <errno.h> // For errno
#include <sys/sysinfo.h> // For sysconf

// Function to allocate memory aligned to page size
char* align_alloc(size_t size) {
    long page_size = sysconf(_SC_PAGESIZE);
    if (page_size == -1) {
        // Handle error or use a default size if sysconf fails
        perror("Error getting page size for align_alloc");
        // Using a common default size as fallback, though not ideal
        page_size = 4096;
    }

    // We allocate size + page_size to store the original pointer
    // and ensure we can find an aligned address within the allocated block.
    // We store the original pointer just before the aligned address.
    void *original_ptr = malloc(size + page_size);
    if (original_ptr == NULL) {
        return NULL; // malloc failed
    }

    // Calculate the aligned address
    uintptr_t aligned_addr = (uintptr_t)original_ptr + page_size;
    aligned_addr &= ~(page_size - 1); // Align down to the nearest page boundary

    // If the calculated aligned address is before the original pointer, adjust it.
    // This should ideally not happen if we allocate size + page_size, but as a safeguard.
    if ((void*)aligned_addr < original_ptr) {
        aligned_addr += page_size;
    }

    // Store the original pointer just before the aligned address
    // Ensure there's enough space to store the pointer (size_t or void*)
    // This assumes size_t is large enough to hold a pointer.
    void **ptr_store = (void **)((char *)aligned_addr - sizeof(void *));
    *ptr_store = original_ptr;

    return (char *)aligned_addr;
}

// Function to free memory allocated by align_alloc
void align_free(void* ptr) {
    if (ptr == NULL) {
        return;
    }

    // Retrieve the original pointer stored just before the aligned address
    void **ptr_store = (void **)((char *)ptr - sizeof(void *));
    void *original_ptr = *ptr_store;

    // Free the original allocated memory
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

    // Get system page size
    long page_size = sysconf(_SC_PAGESIZE);
    if (page_size == -1) {
        perror("Error getting page size");
        close(fd);
        exit(EXIT_FAILURE);
    }

    // Dynamically allocate aligned buffer
    char *buffer = align_alloc(page_size);
    if (buffer == NULL) {
        perror("Error allocating aligned buffer");
        close(fd);
        exit(EXIT_FAILURE);
    }

    ssize_t bytes_read;
    while ((bytes_read = read(fd, buffer, page_size)) > 0) {
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