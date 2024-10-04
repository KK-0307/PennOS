#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "file_system.h"

MyFile open_files[MAX_FILES];

int find_empty_fd() {
    for (int i = 0; i< MAX_FILES; i++) {
        if (open_files[i].file == NULL) {
            return i;
        }
    }

    return -1; //no avaiblae file descriptors
}

int f_open(const char *fname, int mode) {
    int fd = find_empty_fd();

    if (fd == -1) {
        return -1; //no available file descriptors
    }

    if (mode == F_WRITE) {
        //check if file is already open in write mode 
        for (int i = 0; i < MAX_FILES; i++) {
            if (open_files[i].mode == F_WRITE && strcmp(open_files[i].filename, fname) == 0) {
                return -1; // file already open in write mode
            }
        }
    }

    FILE *file = NULL; // decalres pointer and initialized to NULL
    if (mode == F_WRITE) {
        file = fopen(fname, "w+"); // opens file for writing, it trunctates its content and creates a new file that allows both reading and writing
    } else if (mode == F_READ) {
        file = fopen(fname, "r"); //opens file for reading, file must exist or opening will fail
    } else if (mode == F_APPEND) {
        file = fopen(fname, "a+"); // opens file for appending, if exists then file position to the end of the file so that all write operations are appended, if doesn't exist it creates a new empty file
    }

    if (!file) {
        return -1; // Error opening file
    }

    strcpy(open_files[fd].filename, fname);
    open_files[fd].mode = mode;
    open_files[fd].file_pointer = 0;
    open_files[fd].file = file;

    return fd;
}

int f_read(int fd, int n, char *buf) {
    if (fd < 0 || fd >= MAX_FILES || open_files[fd].file == NULL) {
        return -1; // Invalid file descriptor
    }

    fseek(open_files[fd].file, open_files[fd].file_pointer, SEEK_SET);
    int bytesRead = fread(buf, 1, n, open_files[fd].file);
    open_files[fd].file_pointer = ftell(open_files[fd].file);

    if (bytesRead == 0 && !feof(open_files[fd].file)) {
        return -1; // Error reading from file
    }

    return bytesRead;
}

int f_write(int fd, const char *str, int n) {
    if (fd < 0 || fd >= MAX_FILES || open_files[fd].file == NULL) {
        return -1; // Invalid file descriptor
    }

    // Check if file is opened in write mode
    if (open_files[fd].mode != F_WRITE) {
        return -1; // File not opened in write mode
    }

    fwrite(str, 1, n, open_files[fd].file);
    open_files[fd].file_pointer = ftell(open_files[fd].file);

    return n;
}

int f_close(int fd) {
    if (fd < 0 || fd >= MAX_FILES || open_files[fd].file == NULL) {
        return -1; // Invalid file descriptor
    }

    fclose(open_files[fd].file);
    memset(&open_files[fd], 0, sizeof(MyFile));

    return 0;
}

int f_unlink(const char *fname) {
    for (int i = 0; i < MAX_FILES; ++i) {
        if (open_files[i].file != NULL && strcmp(open_files[i].filename, fname) == 0) {
            return -1; // File is open, cannot unlink
        }
    }

    if (remove(fname) != 0) {
        return -1; // Error unlinking file
    }

    return 0;
}

int f_lseek(int fd, int offset, int whence) {
    if (fd < 0 || fd >= MAX_FILES || open_files[fd].file == NULL) {
        return -1; // Invalid file descriptor
    }

    fseek(open_files[fd].file, open_files[fd].file_pointer, SEEK_SET);

    switch (whence) {
        case F_SEEK_SET:
            fseek(open_files[fd].file, offset, SEEK_SET);
            break;
        case F_SEEK_CUR:
            fseek(open_files[fd].file, offset, SEEK_CUR);
            break;
        case F_SEEK_END:
            fseek(open_files[fd].file, offset, SEEK_END);
            break;
        default:
            return -1; // Invalid whence
    }

    open_files[fd].file_pointer = ftell(open_files[fd].file);

    return 0;
}

void f_ls(const char *filename) {
    // (Listing files in the current directory or specific directory)
}

/*
int main() {
    // Example usage:
    int fd = f_open("example.txt", F_WRITE);
    if (fd < 0) {
        printf("Error opening file!\n");
        return 1;
    }

    const char *text = "Hello, File System!";
    int bytes_written = f_write(fd, text, strlen(text));
    if (bytes_written < 0) {
        printf("Error writing to file!\n");
        return 1;
    }

    char buffer[100];
    int bytes_read = f_read(fd, sizeof(buffer), buffer);
    if (bytes_read < 0) {
        printf("Error reading from file!\n");
        return 1;
    }

    printf("Read from file: %s\n", buffer);

    f_close(fd);

    return 0;
} */