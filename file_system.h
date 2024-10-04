#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#define MAX_FILES 100
#define MAX_FILENAME_LENGTH 256
#define F_WRITE 0
#define F_READ 1
#define F_APPEND 2
#define F_SEEK_SET 0
#define F_SEEK_CUR 1
#define F_SEEK_END 2

typedef struct {
    char filename[MAX_FILENAME_LENGTH];
    int mode;
    int file_pointer;
    FILE *file;
} MyFile;

int find_empty_fd();
int f_open(const char *fname, int mode);
int f_read(int fd, int n, char *buf);
int f_write(int fd, const char *str, int n);
int f_close(int fd);
int f_unlink(const char *fname);
int f_lseek(int fd, int offset, int whence);
void f_ls(const char *filename);
