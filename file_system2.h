#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <stdbool.h>
#include "entry.h"
#include <stdint.h>

#define FAILURE -1
#define SUCCESS 0
#define F_SEEK_SET 1
#define F_SEEK_CUR 2
#define F_SEEK_END 3

//Store fds associated with a file
typedef struct FileNode {
    entry* e;
    int num_fds;
    int file_offset;
    struct FileNode* next;
} node;

typedef struct FileDescriptorAssignments {
    entry* e;
    int fd;
    struct FileDescriptorAssignments* next;
} fd_node;

int f_open_f(const char* fname, int mode);
int f_read_f(int fd, int n, char* buf);
int f_write_f(int fd, const char* str, int n);
int f_close_f(int fd);
int f_unlink_f(const char *fname);
int f_lseek_f(int fd, int offset, int whence);
int f_ls_f(const char* filename);
int f_cp_f(char* src, char* dest);
int f_chmod_f(char* fname, uint8_t new_perm);
int f_mv_f(char* src, char* dest);
int f_mkfs_f(char* file_name, int blocks_in_fat, int block_size_config);
int f_mount_f(char* file_name);
int f_umount_f();
int f_get_size_f(int fs_fd);

