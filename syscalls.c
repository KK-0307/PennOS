#include "syscalls.h"
#include <fcntl.h>
#include <unistd.h>
#include "file_system2.h"
#include "error.h"
#include "pcb_linked_list.h"
#include "scheduler.h"
#include <stdint.h>
#include "pennfat.h"

int next_available_fd(int table[16][2])
{
    for (int r = 2; r < 16; r++)
    {
        if (table[r][0] == -1)
        {
            return r;
        }
    }
    return -1;
}

int f_open(const char *fname, int mode)
{
    char *filename = (char *)fname;
    int fs_fd = f_open_f(filename, mode);

    struct PCB* pcb = getActive();
    int process_fd = next_available_fd(pcb->open_fds);
    pcb->open_fds[process_fd][0] = fs_fd;
    pcb->open_fds[process_fd][1] = mode;

    return process_fd;
}

int f_read(int fd, int n, char *buf)
{
    // get fs_fd from mapping
    struct PCB *pcb = getActive();
    int fs_fd = pcb->open_fds[fd][0];

    if (fs_fd == 0)
    { // STDIN
        hasTerminalControl();
        return read(fd, buf, n);
    }

    return f_read_f(fs_fd, n, buf);
}

int f_write(int fd, const char *str, int n)
{
    //get fs_fd from mapping
    struct PCB* pcb = getActive();
    int fs_fd = pcb->open_fds[fd][0];

    if (fs_fd == 1)
    { // STDOUT
        return write(fd, str, n);
    }

    // get mode from mapping, append or overwrite, this is temporary
    return f_write_f(fs_fd, str, n);
}

int f_close(int fd)
{
    // get fs_fd from mapping

    struct PCB *pcb = getActive();
    int fs_fd = pcb->open_fds[fd][0];

    return f_close_f(fs_fd);
}

int f_unlink(const char *fname)
{
    return f_unlink_f(fname);
}

int f_ls(const char *filename)
{
    return f_ls_f(filename);
}

int f_lseek(int fd, int offset, int whence)
{
    // get fs_fd from mapping
    struct PCB *pcb = getActive();
    int fs_fd = pcb->open_fds[fd][0];

    return f_lseek_f(fs_fd, offset, whence);
}

int f_mv(char *src, char *dest)
{
    return f_mv_f(src, dest);
}

int f_cp(char *src, char *dest)
{
    return f_cp_f(src, dest);
}

int f_chmod(char *fname, uint8_t new_perm)
{
    return f_chmod_f(fname, new_perm);
}

int f_mkfs(char *file_name, int blocks_int_fat, int blocks_size_config)
{
    return f_mkfs_f(file_name, blocks_int_fat, blocks_size_config);
}

int f_mount(char *file_name)
{
    return f_mount_f(file_name);
}

int f_umount()
{
    return f_umount_f();
}

int f_get_size(int fd)
{
    struct PCB *pcb = getActive();
    int fs_fd = pcb->open_fds[fd][0];
    return f_get_size_f(fs_fd);
}
