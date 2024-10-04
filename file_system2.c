#include "file_system2.h"
#include "pennfat.h"
#include <unistd.h>
#include <stdint.h>
#include <time.h>

// Global variables
node *file_list;  // List of files 
fd_node* fd_list; //List of assignments for fds
int counter = 2; //helps assign file system fds, reserve 0 and 1 for stdin/stdout

node* find_file_list(char* filename) {
    node* p = file_list;
    while (p != NULL) {
        //find entry filename that matches
        if (strcmp(p->e->name, filename) == 0) {
            return p;
        }
        p = p->next;
    }
    return NULL;
}

void add_file_to_list(char* filename) {
    node* p = file_list;
    node* new_file = malloc(sizeof(node));
    new_file->e = find_file(filename);

    //check if already in list, then add
    while(p != NULL) {
        if (new_file->e == p->e) {
            new_file->num_fds = new_file->num_fds + 1;
            return;
        }
        p = p->next;
    }

    p = file_list;

    new_file->num_fds = 0; // or 1
    new_file->file_offset = 0;
    new_file->next = NULL;

    if (p == NULL) {
        file_list = new_file;
        return;
    }
    while (p->next != NULL) {
        p = p->next;
    }
    p->next = new_file;
    p = p->next;
    p->next = NULL;
    return;
}

void add_fd_assignment(int fd, char* filename) {
    fd_node* p = fd_list;
    fd_node* new_assignment = malloc(sizeof(fd_node));
    
    new_assignment->e = find_file(filename);
    new_assignment->fd = fd;
    if (p == NULL) {
        fd_list = new_assignment;
        return;
    }
    while (p->next != NULL) {
        p = p->next;
    }
    p->next = new_assignment;
    p = p->next;
    p->next= NULL;
    return;
}

fd_node* search_fd_assignment(int fd) {
    fd_node* p = fd_list;
    while (p != NULL) {
        if (p->fd == fd) {
            return p;
        }
        p = p->next;
    }
    return NULL;
}

char* remove_from_fd_list(int fd) {
    fd_node* p = fd_list;
    fd_node* prev = NULL;
    while (p != NULL) {
        if (p->fd == fd) {
            char* filename = p->e->name;
            //if first node in list
            if (prev == NULL) {
                fd_list = p->next;
                return filename;
            } else {
                prev->next = p->next;
                return filename;
            }
        }
        prev = p;
        p = p->next;
    }
    return NULL;
}

void remove_from_file_list(char* filename) {
    node* p = file_list;
    node* prev = NULL;
    while (p != NULL) {
        if (strcmp(p->e->name, filename) == 0) {
            //if first node in list
            if (prev == NULL) {
                file_list = p->next;
                return;
            } else {
                prev->next = p->next;
                return;
            }
        }
        prev = p;
        p = p->next;
    }
    return;
}


int f_open_f(const char* fname, int mode) {
    char* filename = (char*) malloc(strlen(fname)+1);

    filename = strcpy(filename, fname);
    
    entry* result= find_file(filename);
    node* file = find_file_list(filename);
    if (mode == 1 && result  == NULL) {
        perror("f_open_f: file does not exist");
        return EXIT_FAILURE;
    }

    touch(filename);
    //if file doesn't exist for this process
    if (file == NULL) {
        add_file_to_list(filename); //increments num_fds already

    } else { //file does exist for this process
        //entry* result = file->e;
        if (mode == 0) { //FWRITE, truncate file
            //remove data
            uint16_t first_index = result->firstBlock;
            if (result->size == 0) { //check if file has no data
                file->num_fds = file->num_fds + 1;
                //printf("num_fds in %s: %d for open\n", filename, file->num_fds);
                counter++;
                add_fd_assignment(counter, filename);
                return counter;
            }
            else {
                //delete contents in file, same as replacing the whole block in data region with 0s
                uint16_t index = first_index;
                do {
                    //2nd block
                    memset(DATA + ((index-1) * BLOCKS_SIZE_CONFIG), 0, BLOCKS_SIZE_CONFIG);
                    index = FAT[index];
                }
                while(index != 0xFFFF);
            }
            file->file_offset = 0;
            result->size = 0;
        }
        else if (mode == 2) { //F_APPEND
            file->file_offset = result->size;
        }
        else if (mode == 1) {
            file->file_offset = 0; //read from beginning
        }
        file->num_fds = file->num_fds + 1;
        //printf("num_fds in %s: %d for open\n", filename, file->num_fds);
        //printf("num_fds: %d\n", file->num_fds);
    }
    counter++;
    add_fd_assignment(counter, filename);
    return counter;
}

int f_read_f(int fd, int n, char* buf) {

    fd_node* fd_file= search_fd_assignment(fd);
    char* filename = fd_file->e->name;
    node* file = find_file_list(filename);
    uint32_t offset = file->file_offset;

    //calculate index of offset
    int block_bytes = (offset / BLOCKS_SIZE_CONFIG);
    int remainder_bytes = offset % BLOCKS_SIZE_CONFIG;

    int count = 0;
    int block = file->e->firstBlock;
    while(count < block_bytes) {
        block = FAT[block];
        count++;
    }
    //block is now index
    do {
        
        int index_to_read = (block-1) * BLOCKS_SIZE_CONFIG + remainder_bytes; 
        char* pointer_to_data = (char*) &DATA[index_to_read];
        if (n <= BLOCKS_SIZE_CONFIG) {
            memcpy(buf, pointer_to_data, n);
            //strncat(buf, pointer_to_data, n);
            file->file_offset = file->file_offset + n;
            n = 0;
            return SUCCESS;
        } else {
            memcpy(buf, pointer_to_data, BLOCKS_SIZE_CONFIG - remainder_bytes);
            //pointer_to_data += (BLOCKS_SIZE_CONFIG - remainder_bytes);
            buf += BLOCKS_SIZE_CONFIG - remainder_bytes;
            //strncat(buf, pointer_to_data, BLOCKS_SIZE_CONFIG - remainder_bytes);
            block = FAT[block];
            n = n - (BLOCKS_SIZE_CONFIG - remainder_bytes);
            remainder_bytes = 0;
        }
    } while(n > 0); //still have bytes to read
    file->file_offset = file->file_offset + n; //not necessary?
    return SUCCESS;
}

//fd is file system file descriptor
int f_write_f(int fd, const char* str, int n) {

    int original_n = n;
    fd_node* fd_file= search_fd_assignment(fd);
    char* file_name = fd_file->e->name;
    node* file = find_file_list(file_name);
    uint32_t offset = file->file_offset;
    //printf("offset %d\n", offset);
    char* str_to_write = malloc(n);
    strncpy(str_to_write, str, n); 

    //calculate index of offset
    int block_bytes = (offset / BLOCKS_SIZE_CONFIG); //if 0 then 0
    int remainder_bytes = offset % BLOCKS_SIZE_CONFIG; //should be 0

    int count = 0;
    int block = file->e->firstBlock;
    
    if (block == 0) {
        block = find_next_empty_block();
        file->e->firstBlock = block;
    } else {
        while(count < block_bytes) {
        block = FAT[block];
        count++;
        }
    }
    int prev_fat_index = block;
    do {
        int index_to_write = (block - 1) * BLOCKS_SIZE_CONFIG + remainder_bytes; //should be block-1?
        char* pointer_to_data = (char*) &DATA[index_to_write];
       //printf("<=: %d\n", BLOCKS_SIZE_CONFIG * (block-1));
        if (n <= BLOCKS_SIZE_CONFIG) {
            //write to file
            strcpy(pointer_to_data, str_to_write);
            FAT[prev_fat_index] = block;
            FAT[block] = 0xFFFF;
            file->file_offset = file->file_offset + original_n;
            file->e->size = file->e->size + original_n;
            return SUCCESS;
        } else {
            strncpy(pointer_to_data, str_to_write, BLOCKS_SIZE_CONFIG - remainder_bytes);
            str_to_write = str_to_write + (BLOCKS_SIZE_CONFIG - remainder_bytes);

            FAT[prev_fat_index] = block;
            block = find_next_empty_block();
            FAT[block] = 0xFFFF;
            n = n - (BLOCKS_SIZE_CONFIG - remainder_bytes);
            remainder_bytes = 0;

        }
    } while(n > 0); //still have bytes to write
    //file->file_offset = file->file_offset + original_n;
    return SUCCESS;
}


int f_close_f(int fd) {
    //remove from fd_list (assignments list)
    char* filename = remove_from_fd_list(fd);
    //find which file it belongs to, update file_list num_fds
    node* file = find_file_list(filename);
    if (file->num_fds > 0) { //> 1
        file->num_fds = file->num_fds - 1;
        //printf("num_fds in %s: %d for close\n", filename, file->num_fds);
        //remove_from_fd_list(fd);
        return SUCCESS;
    } else if (file->num_fds == 1 && filename[0] == '2'){ //unlink has been called
        remove_file(filename); //remove from FAT
        //remove_from_fd_list(fd); //remove from fd assignment list
        remove_from_file_list(filename); //remove from file list
    } 
    else { //unlink has not been called
        //remove_from_fd_list(fd); //remove from fd assignment list
        remove_from_file_list(filename); //remove from file list
    }
    return SUCCESS;
}

//remove file. should not be able to remove file if it is in use by a process
//given fs_fd in f_unlink, find mapping
int f_unlink_f(const char *fname) {
    char* filename = (char*) fname;
    node* file = find_file_list(filename);
    if (file == NULL) {
        entry* e = find_file(filename);
        if (e != NULL) { //if just unlink then can remove the file if it exists in file system
            remove_file(filename);
            return SUCCESS;
        }
        perror("file not found, unlink may have been called previously\n");
        return EXIT_FAILURE;
    }
    else {
        if (file->num_fds == 0) { //no processes using the file
            remove_file(filename);
            return SUCCESS;
        }
    }
    //printf("num_fds for %s: %d for unlink\n", filename, file->num_fds);
    file->e->name[0] = '2';
    return SUCCESS;
}


int f_lseek_f(int fd, int offset, int whence) {
   //find file and current offset
    fd_node* f = search_fd_assignment(fd);
    char* filename = f->e->name;
    node* file = find_file_list(filename);
    //get entry
    entry* e = f->e;
    uint32_t file_size = e->size;
    
    // The file offset is set to offset bytes.
    if (whence == F_SEEK_SET) {
        file->file_offset = offset;
        return file->file_offset;
    } 
    //The file offset is set to its current location plus offset bytes.
    else if (whence == F_SEEK_CUR) {
        file->file_offset = file->file_offset + offset;
        return file->file_offset;
    }
    //The file offset is set to the size of the file plus offset bytes.
    else if (whence == F_SEEK_END) {
        file->file_offset = file_size + offset;
        return file->file_offset;
    }
    return -1;
}

int f_ls_f(const char* filename) {
    char* file_name = (char*) filename;
    if (file_name == NULL) {
        ls();
    }
    else {
        //get entry for file
        entry* e = find_file(file_name);
        printf("%d\t%s\t%d\t%d\t%ld\t%s\n",
                e->type,
                e->name,
                e->size,
                e->perm,
                e->mtime,
                ctime(&e->mtime));
    }
    return SUCCESS;
}

int f_mv_f(char* src, char* dest) {
    mv(src, dest);
    return SUCCESS;
}

//Assuming source file is from host OS
int f_cp_f(char* src, char* dest) {
    cp(src, dest, true);
    return SUCCESS;
}

int f_chmod_f(char* fname, uint8_t new_perm) {
    chmod_file(fname, new_perm);
    return SUCCESS;
}

int f_mkfs_f(char* file_name, int blocks_in_fat, int block_size_config) {
    mkfs(file_name, blocks_in_fat, block_size_config);
    return SUCCESS;
}
int f_mount_f(char* file_name) {
    mount(file_name);
    return SUCCESS;
}
int f_umount_f() {
    unmount();
    return SUCCESS;
}

int f_get_size_f(int fs_fd) {
    fd_node* file = search_fd_assignment(fs_fd);
    return file->e->size;

}

