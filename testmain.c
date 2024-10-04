#include <signal.h>   // sigaction, sigemptyset, sigfillset, signal
#include <stdlib.h> 
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include "pennfat.h"
#include "syscalls.h"

/*
int main() {

    //Test1 with testfs
    
    mkfs("fs", 1, 0);
    mount("fs");
    int fs_fd_1 = f_open("file1", 2);
    printf("fs_fd_1: %d\n", fs_fd_1);
    int fs_fd_2 = f_open("file2", 2);
    printf("fs_fd_2: %d\n", fs_fd_2);
    int fs_fd_3 = f_open("file3", 2);
    printf("fs_fd_3: %d\n", fs_fd_3);
    int fs_fd_4 = f_open("file4", 2);
    printf("fs_fd_4: %d\n", fs_fd_4);


    f_ls(NULL);
    f_write(fs_fd_4, "In this assignment you will implement PennOS, your own UNIX-like operating system. PennOS is designed around subsystems that model those of standard UNIX. This will include programming a basic priority scheduler, FAT file system, and user shell interactions.", 259);
    //no malloc error here, once you try to write there is
    char* buffer = (char*) malloc(259); //for some reason need to account for special characters
    f_lseek(fs_fd_4, 0, 1);
    f_read(fs_fd_4, 259, buffer);
    printf("%s\n", buffer);
    
    f_write(fs_fd_1, "hello", 5);
    
    int fs_fd_5 = f_open("file5", 0);
    printf("fs_fd_5: %d\n", fs_fd_5);
    f_ls(NULL);

    int fs_fd_1_again = f_open("file1", 0);
    printf("fs_fd_1_again: %d\n", fs_fd_1_again);

    f_write(fs_fd_1_again, "hello again", 11);

    int fs_fd_1_again_2 = f_open("file1", 2);
    f_write(fs_fd_1_again_2, "again", 5); 
    f_ls("file1");

    f_open("idonotexist", 1);

    //some root directory stuff is not updating correctly
    //f_unlink("file4", fs_fd_4); //removes file with data, correctly removes, should remove ff?
    //f_unlink("file1", fs_fd_1); //used by multiple processes, name[0] changes
    //f_unlink("file1", fs_fd_1_again); 
    //f_unlink("file1", fs_fd_1_again_2); //should be link to root directory

    //f_open("file300", 0);
    //f_open("a new file", 0);
    //f_open("another new file", 0);

    f_close(fs_fd_2);

    f_lseek(fs_fd_1, 0, 1); //F_SEEK_SET works
    f_lseek(fs_fd_1, 2, 2); //F_SEEK_CUR works
    char* another_buf = (char*) malloc(14); 
    f_read(fs_fd_1, 14, another_buf);
    printf("another_buf: %s\n", another_buf);

    f_ls("file1");

    f_lseek(fs_fd_1, 1, 3); //F_SEEK_END works
    f_write(fs_fd_1, "again!", 6);

    f_open("file1", 0);
    f_ls("file1");


    
    mkfs("fs", 1, 0);
    mount("fs");
    int fs_fd_1 = f_open("file1", 0);
    printf("fs_fd_1: %d\n", fs_fd_1);
    int fs_fd_2 = f_open("file2", 0);
    printf("fs_fd_2: %d\n", fs_fd_2);
    f_write(fs_fd_1, "hello", 5);
    f_write(fs_fd_2, "hi", 2);
    char* buffer = (char*) malloc(5);
    f_read(fs_fd_1, 5, buffer);
    printf("%s\n", buffer);
    f_ls("file1");
    printf("space\n");
    f_ls(NULL);
    

    mkfs("fs", 1, 0);
    mount("fs");
    //delete just one file from one process
    int fs_fd_1 = f_open("file1", 0);
    int fs_fd_1_again = f_open("file1", 0);
    f_write(fs_fd_1, "hello", 5);
    f_unlink("file1");
    f_close(fs_fd_1);
    f_close(fs_fd_1_again);
    //what about f_close then f_unlink?
    
    
    

    //char* buffer = (char*) malloc(259);
    //f_read(fs_fd_4, 259, buffer);
   //printf("%s\n", buffer);


}
*/