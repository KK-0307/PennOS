#include <stdint.h>
#include "pcb_linked_list.h"

#define STDIN_FILENO 0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2

#define F_WRITE 0
#define F_READ 1
#define F_APPEND 2

int f_open(const char *fname, int mode);

int f_read(int fd, int n, char *buf);

int f_write(int fd, const char *str, int n);

int f_close(int fd);

int f_unlink(const char *fname);

int f_ls(const char* filename);

int f_lseek(int fd, int offset, int whence);

int f_mv(char* src, char* dest);

int f_cp(char* src, char* dest);

int f_chmod(char* fname, uint8_t new_perm);

int f_mv(char* src, char* dest);

int f_mkfs(char* file_name, int blocks_int_fat, int blocks_size_config);

int f_mount(char* file_name);

int f_umount();

int f_get_size(int fd);
