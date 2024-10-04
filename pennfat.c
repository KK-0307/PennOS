#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <stdbool.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/mman.h>  // for mmap and munmap
#include <sys/types.h>
#include <sys/stat.h>
#include <stdint.h>
#include "entry.h"
#include <time.h>
#include "pennfat.h"

//const int MAX_LINE_LENGTH = 4096;
int BLOCKS_IN_FAT;
int BLOCKS_SIZE_CONFIG_NUM;
int BLOCKS_SIZE_CONFIG;
int FAT_SIZE;
int FAT_FD;
uint16_t* FAT;
uint8_t* DATA;
int NUM_FAT_ENTRIES;
int NUM_ENTRIES_IN_BLOCK;
int FIRST_DATA_INDEX; //index 1, first index in the data region
int DATA_SIZE; //number of blocks in data region

//helper methods

//find file in entries
entry* find_file(char* file_name) {
    //for every block that is part of the root directory
    //first block of root directory
    int fat_index = 1;
    int data_index = 0;

    do {
        entry* e = (entry*) &DATA[data_index];

        for (int i = 0; i < NUM_ENTRIES_IN_BLOCK; i++) {
            char* name = e->name;
            if (e == NULL) {
                return NULL;
            } else {
                if (strcmp(name, file_name) == 0) {
                    return e;
                }
            }
            data_index = data_index + 64;
            e = (entry*) &DATA[data_index];

        }
        fat_index = FAT[fat_index];
        data_index = (fat_index - 1) * BLOCKS_SIZE_CONFIG;
    } while(fat_index != 0xFFFF);

    return NULL;
}

//finds next empty block in fat that's aligned with data region
int find_next_empty_block() { //for fat
    int index = 1;
    while(FAT[index] != 0x0000) {
        index++;
    }
    return index;
}

//finds next available entry to populate root directory
void next_empty_entry(entry** result_entry, int* result_fat, int* result_data) {
    int fat_index = 1;
    int prev_fat_index = fat_index;
    int data_index = 0;

    do {
        entry* e= (entry*) &DATA[data_index];
        for (int i = 0; i < NUM_ENTRIES_IN_BLOCK; i++) {
            if (((uint8_t*)e)[0] == 0) { // rough check, should be more rigorous
                //*result_entry = e;
                *result_fat = fat_index;
                *result_data = data_index;
                *result_entry = (entry*) &DATA[data_index];
                return;
            } 
            data_index = data_index + 64;
            e = (entry*) &DATA[data_index];
        }
        prev_fat_index = fat_index;
        fat_index = FAT[fat_index];
        data_index = fat_index - 1;
    } while(fat_index != 0xFFFF);

    
    //need to allocate a new block to root directory
    //int prev_fat_index = fat_index;
    fat_index = find_next_empty_block();
    
    //data_index = fat_index - 1;
    data_index = (fat_index - 1) * BLOCKS_SIZE_CONFIG;
    entry* e = (entry*) &DATA[data_index]; //here
    *result_entry = e;
    *result_fat = fat_index;
    *result_data = data_index;
    //update fat table for root directory
    FAT[prev_fat_index] = fat_index;
    FAT[fat_index] = 0xFFFF; 
    return;
}

int bytes_available(int data_index, int fat_index) {
    int count = 0;
    //while(data_index < data_index + BLOCKS_SIZE_CONFIG) { //gets data index, sees if there's data
    while(data_index < (fat_index) * BLOCKS_SIZE_CONFIG) {
        if (DATA[data_index] == 0) {
            return BLOCKS_SIZE_CONFIG - count;
        }
        else {
            data_index++;
            count++;
        }
    }
    return 0;
}

int traverse_fat(uint16_t starting_block) {
    uint16_t index = starting_block;
    while(FAT[index] != 0xFFFF) {
        index = FAT[index];
    }
    return (int) index;
}

void put_data_in_block(int index_of_file, char* input) {
    size_t len = strlen(input);
    size_t bytes_written = 0;
    int prev_fat_index = index_of_file;

    //check if input is completely allocated
     while(bytes_written < len) {
            //find bytes available
            int bytes = bytes_available((index_of_file - 1) * BLOCKS_SIZE_CONFIG, index_of_file); 
            //input fits in block
            int index_of_new_data = (index_of_file - 1) * BLOCKS_SIZE_CONFIG + (BLOCKS_SIZE_CONFIG - bytes);
            //printf("index_of_new_data: %d\n", index_of_new_data); 
            char* pointer_to_data = (char*) &DATA[(index_of_new_data)];
            if (len - bytes_written <= BLOCKS_SIZE_CONFIG) { //data fits
                strcpy(pointer_to_data, input);
                FAT[prev_fat_index] = index_of_file;
                FAT[index_of_file] = 0xFFFF;
                return;
            } else { //data does not fit
                //allocate remaining
                strncpy(pointer_to_data, input, bytes); 

                bytes_written += bytes;
                //move pointer
                input = input + bytes;
                //allocate new block to file
                FAT[prev_fat_index] = index_of_file;
                FAT[index_of_file] = 0xFFFF;
                prev_fat_index = index_of_file;
                index_of_file = find_next_empty_block();
            }
        }
}

void mkfs(char* file_name, int blocks_in_fat, int block_size_config) {
    FAT_FD = open(file_name, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR); //| O_CREAT
    if (FAT_FD < 0) {
        perror("cannot open file");
    }
    BLOCKS_IN_FAT = blocks_in_fat;
    BLOCKS_SIZE_CONFIG_NUM = block_size_config;
    if (block_size_config == 0) {
        BLOCKS_SIZE_CONFIG = 256;
    } else if (block_size_config == 1) {
        BLOCKS_SIZE_CONFIG = 512;
    } else if (block_size_config == 2) {
        BLOCKS_SIZE_CONFIG = 1024;
    }
    else if (block_size_config == 3) {
        BLOCKS_SIZE_CONFIG = 2048;
    }
    else if (block_size_config == 4) {
        BLOCKS_SIZE_CONFIG = 4096;
    }
    FAT_SIZE = BLOCKS_IN_FAT * BLOCKS_SIZE_CONFIG;
    NUM_FAT_ENTRIES = BLOCKS_SIZE_CONFIG * BLOCKS_IN_FAT / 2;
    NUM_ENTRIES_IN_BLOCK = BLOCKS_SIZE_CONFIG/64;
    FIRST_DATA_INDEX = FAT_SIZE;
    DATA_SIZE = BLOCKS_SIZE_CONFIG * (NUM_FAT_ENTRIES - 1); 

    int size = FAT_SIZE + DATA_SIZE;
    ftruncate(FAT_FD, size);
    char* array = (char*) malloc(size);
    memset(array, 0, size);
    array[0] = (char) BLOCKS_SIZE_CONFIG_NUM;
    array[1] = (char) BLOCKS_IN_FAT;
    array[2] = 0xFF;
    array[3] = 0xFF;
    write (FAT_FD, array, size);


    // FAT[0] =  (BLOCKS_IN_FAT) | (BLOCKS_SIZE_CONFIG_NUM < 8); 
    // FAT[1] = 0xFFFF; //root directory
    // DATA = (uint8_t*) &FAT[NUM_FAT_ENTRIES];

}

void mount(char* file_name) {
    //need to be able to mount without mkfs...
    FAT_FD = open(file_name, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR); 
    unsigned char buffer[2];
    ssize_t bytesRead;
    bytesRead = read(FAT_FD, buffer, sizeof(buffer));
    if (bytesRead < 0) {
        perror("mount read error");
    }
    BLOCKS_IN_FAT = buffer[1]; //can switch later
    BLOCKS_SIZE_CONFIG_NUM = buffer[0];
    if (BLOCKS_SIZE_CONFIG_NUM == 0) {
        BLOCKS_SIZE_CONFIG = 256;
    } else if (BLOCKS_SIZE_CONFIG_NUM == 1) {
        BLOCKS_SIZE_CONFIG = 512;
    } else if (BLOCKS_SIZE_CONFIG_NUM == 2) {
        BLOCKS_SIZE_CONFIG = 1024;
    }
    else if (BLOCKS_SIZE_CONFIG_NUM == 3) {
        BLOCKS_SIZE_CONFIG = 2048;
    }
    else if (BLOCKS_SIZE_CONFIG_NUM == 4) {
        BLOCKS_SIZE_CONFIG = 4096;
    }

    FAT_SIZE = BLOCKS_IN_FAT * BLOCKS_SIZE_CONFIG;
    NUM_FAT_ENTRIES = BLOCKS_SIZE_CONFIG * BLOCKS_IN_FAT / 2;
    NUM_ENTRIES_IN_BLOCK = BLOCKS_SIZE_CONFIG/64;
    FIRST_DATA_INDEX = FAT_SIZE;
    DATA_SIZE = BLOCKS_SIZE_CONFIG * (NUM_FAT_ENTRIES - 1); 

    FAT = mmap(NULL, FAT_SIZE +  DATA_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, FAT_FD, 0);
    if (FAT == MAP_FAILED){
        perror("mmap failed");
    }
    DATA = (uint8_t*) &FAT[NUM_FAT_ENTRIES];
    
}

void unmount() {
    munmap(FAT, FAT_SIZE + DATA_SIZE);
    //FILE *filePointer = fdopen(FAT_FD, "r");
    close(FAT_FD);
}



void touch (char* file_name) {
    entry* result = find_file(file_name);
    if (result != NULL) { //updates time
        result->mtime = time(NULL);
        return;
    }

    entry* new_entry_pointer;
    int new_entry_fat_index;
    int new_entry_data_index;

    next_empty_entry(&new_entry_pointer, &new_entry_fat_index, &new_entry_data_index);

    strncpy(new_entry_pointer->name, file_name, sizeof(new_entry_pointer->name));
    new_entry_pointer->size = 0; 
    new_entry_pointer->firstBlock = 0; 
    new_entry_pointer->type = 1; 
    new_entry_pointer->perm =  6; 
    new_entry_pointer->mtime = time(NULL);
    return;

}

//rename source to dest
 void mv(char* source, char* dest) {
    //find the source file in the file system
    entry* source_entry = find_file(source);
    if (source_entry == NULL) {
        //printf("Source file %s not found.\n", source);
        printf("File Not Found\n");
        return;
    }

    //check if the destination file already exists
    entry* dest_entry = find_file(dest);
    //if destination file exists, replace dest with source
    if (dest_entry != NULL) {
        //TODO
        remove_file(dest);
        strncpy(source_entry->name, dest, sizeof(source_entry->name));
        return;
    }

    strncpy(source_entry->name, dest, sizeof(source_entry->name));

    //printf("File %s moved to %s.\n", source, dest);
 }

//deletes all entries if file has no data
void remove_file(char* file_name) { //need to delete entry dummy

    //remove data
    entry* result = find_file(file_name);
    uint16_t first_index = result->firstBlock;
    if (result == NULL) { 
        printf("File Not Found\n");
        return;
    }
    else if (result->size == 0) { //check if file has no data
        //remove the entry
        memset(result, 0, 64); 
        return;
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
    //remove from fat
    uint16_t index_fat = first_index;
    do { 
        uint16_t prev_index = index_fat;
        index_fat = FAT[index_fat];
        FAT[prev_index] = 0; //not necessary remove everything
    }
    while(index_fat != 0xFFFF);  
    memset(result, 0, 64); 
}

//write to file specified
void cat_append(char* file_name, char* input) {
    //redirect here, input is either char[] or char*

    //if directory already contains file

    entry* result = find_file(file_name);
    result->size = result->size + strlen(input);

    if (result == NULL) { 
        perror("file does not exist");
        return;
    }
    else {
        //find fat index for data in root directory, check fat table
        uint16_t fat_block = result->firstBlock; //make sure this value is correct
        int index_of_file;
        if (fat_block == 0) { //not initialized, goes here because initialized to 0
            index_of_file = find_next_empty_block();
        } else { 
            index_of_file = traverse_fat(fat_block);
        }
        
        result->firstBlock = index_of_file;
        put_data_in_block((int) index_of_file, input);
        return;
    }
}

void cat_overwrite(char* file_name, char* input) {
    entry* result = find_file(file_name);
    int new_size = strlen(input);
     
    uint16_t first_index = result->firstBlock;
    int prev_index = first_index;
    if (result == NULL) { 
        perror("file does not exist");
        return;
    }
    else {
        //delete contents in file, same as replacing the whole block in data region with 0s
        uint16_t index = first_index;
        do {
            //2nd block
            memset(DATA + ((index-1) * BLOCKS_SIZE_CONFIG), 0, BLOCKS_SIZE_CONFIG);
            prev_index = index;
            index = FAT[index];
            FAT[prev_index] = 0;
        }
        while(index != 0xFFFF);

    }
    //FAT[first_index] = 0xFFFF;
    //what to do with extra blocks
    //make sure cat_append appends to the correct block

    put_data_in_block((int) first_index, input);
    //printf("input: %s\n", input);
    result->size = new_size;

}


char* cat_concatenate(char* list_of_files[], int num_files) {
    
    //sum up size of files
    uint32_t sum_of_size = 0;
    for (int i = 0; i < num_files; i++) {
        char* file_name = list_of_files[i];
        entry* e = find_file(file_name);
        if (e == NULL) {
            printf("File Not Found\n");
            return NULL;
        }
        sum_of_size = sum_of_size + e->size;
    }
    char* concatenate_result = (char*) malloc(sum_of_size);

    for (int i = 0; i < num_files; i++) {
        char* file_name = list_of_files[i];
        //for each file add data of file to concatenate_result, use strcat
        entry* result = find_file(file_name);
        uint16_t index = result->firstBlock;
        uint32_t size = result->size; 
        uint32_t original_size = size;
        char* data = (char*) malloc (size);
        bool flag= false;
    
        do { //get all data from data blocks
            int index_of_new_data = (index - 1) * BLOCKS_SIZE_CONFIG;
            char* pointer_to_data = (char*) &DATA[(index_of_new_data)];

            if (size <= BLOCKS_SIZE_CONFIG) {
                strncpy(data, pointer_to_data, result->size); 
                size = 0;
            } else {
                flag = true;
                strncpy(data, pointer_to_data, BLOCKS_SIZE_CONFIG);
                index = FAT[index];
                size = size - BLOCKS_SIZE_CONFIG;
                data += BLOCKS_SIZE_CONFIG;
            }
        }
        while(size > 0);

        while(size > 0);
        if (flag) {
            data -= original_size;
        }
        
        strcat(concatenate_result, data);
    }
    return concatenate_result;
}


void cat_concatenate_stdout(char* list_of_files[], int num_files) {
    char* result = cat_concatenate(list_of_files, num_files);
    if (result == NULL) {
        return;
    }
    write(STDOUT_FILENO, result, strlen(result));
}


void cat_concatenate_redirection_append(char* list_of_files[], int num_files, char* output_file) {
    char* result = cat_concatenate(list_of_files, num_files);
    cat_append(output_file, result);
}

void cat_concatenate_redirection_overwrite(char* list_of_files[], int num_files, char* output_file) {
    char* result = cat_concatenate(list_of_files, num_files);
    cat_overwrite(output_file, result);
}


void ls(){
    int fat_index = 1;
    int data_index = 0;

    do {
        entry* e = (entry*) &DATA[data_index];

        for (int i = 0; i < NUM_ENTRIES_IN_BLOCK; i++) {
            if (e->name[0] != '\0') { //this check doesn't work
                printf("%d\t%s\t%d\t%d\t%ld\t%s\n",
                e->type,
                e->name,
                e->size,
                e->perm,
                e->mtime,
                ctime(&e->mtime));
            }
            data_index = data_index + 64;
            e = (entry*) &DATA[data_index];
        }
        fat_index = FAT[fat_index];
        data_index = (fat_index - 1) * BLOCKS_SIZE_CONFIG;
    } while(fat_index != 0xFFFF);
}

//isHost meaning source file is from host OS
void cp(char* source, char* dest, bool isHost) {
    //cp [ -h ] SOURCE DEST
    if (isHost) {
        //read from the host OS (assuming the source fule is accessible)
        int sourceFD = open(source, O_RDONLY);
        if (sourceFD < 0) {
            perror("Error opening source file");
            return;
        }
        //copy data from the host file to the destination file in the file system 
        char buffer[32000];
        ssize_t bytesRead;
        bytesRead = read(sourceFD, buffer, sizeof(buffer));
        if (bytesRead < 0) {
            perror("cp read error");
        }

        //check if the dest file is in the file system
        entry* e = find_file(dest);
        if (e == NULL) {
            //file doesn't exist
            touch(dest);
            cat_append(dest, buffer);
            //should just work with cat_append but for some reason doesn't.
            //cat_overwrite(dest,buffer);
        } else {
            cat_overwrite(dest, buffer);
        }

        close(sourceFD);
    } 
    //cp SOURCE -h DEST
    else {
        //read from the file system (assuming the source file is in the file system)
        entry* sourceEntry = find_file(source);
        if (sourceEntry == NULL) {
            printf("File Not Found\n");
            //printf("Source file %s not found in the file system\n", source);
            return;
        }

        //open the destination file in the host OS
        int destFD = open(dest, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
        if(destFD < 0) {
            perror("Error opening destination file");
            return;
        }

        //copy data from the file system to the host file

        uint16_t index = sourceEntry->firstBlock;
        uint32_t size = sourceEntry->size; 
        uint32_t original_size = size;
        char* data = (char*) malloc (size);
        bool flag = false;
    
        do { //get all data from data blocks
            int index_of_new_data = (index - 1) * BLOCKS_SIZE_CONFIG;
            char* pointer_to_data = (char*) &DATA[(index_of_new_data)];

            if (size <= BLOCKS_SIZE_CONFIG) {
                strncpy(data, pointer_to_data, sourceEntry->size); //segfaults here
                size = 0;
            } else {
                flag = true;
                strncpy(data, pointer_to_data, BLOCKS_SIZE_CONFIG);
                index = FAT[index];
                size = size - BLOCKS_SIZE_CONFIG;
                data += BLOCKS_SIZE_CONFIG;
            }
        }
        
        while(size > 0);
        if (flag) {
            data -= original_size;
        }
        
        
        write(destFD, data, original_size);
        
        /*
        while (DATA[dataIndex] != 0) {
            write(destFD, &DATA[dataIndex], bytes_to_write);
            fatBlock = FAT[fatBlock];
            dataIndex = (fatBlock - 1) * BLOCKS_SIZE_CONFIG;
        }
        */
        //close the destination file in the host OS
        close(destFD);
    }
}

 //changes the permissions of a file in the PennFat file system 
 void chmod_file(char* file_name, uint8_t new_perm) {
    //find the file in the file system 
    entry* result = find_file(file_name);

    if (result != NULL) {
        //file found, update its permissions 
        result->perm = new_perm;
    } else {
        //file not found
        printf("File not found: %s\n", file_name);
    }
 }


// int main() {

//     //practice here call mkfs, mount, umount
//     mkfs("hello", 32, 4);
//     mount("hello");

//     printf("Before touch\n");
//     printf("FAT[0]: %d\n", FAT[0]); //should be size
//     printf("FAT[1]:%d\n", FAT[1]); //should be 0xFFFF for root directory
//     printf("FAT[2]:%d\n", FAT[2]); //should be 0
//     printf("DATA[4096]:%d\n", DATA[BLOCKS_SIZE_CONFIG]); //should be 0
//     printf("DATA[4097]:%d\n", DATA[BLOCKS_SIZE_CONFIG + 1]); //should be 0

//     touch("testfsadd");


//     printf("After touch testfsadd\n");
//     printf("FAT[0]: %d\n", FAT[0]); //should be size
//     printf("FAT[1]: %d\n", FAT[1]); //should be 0xFFFF for root directory
//     printf("FAT[2]: %d\n", FAT[2]); //should be 0 as nothing in file
//     printf("DATA[4096]: %d\n", DATA[BLOCKS_SIZE_CONFIG]); //should be 0
//     printf("DATA[4097]: %d\n", DATA[BLOCKS_SIZE_CONFIG + 1]); //should be 0

//     touch("testfsadd2");
//     printf("After touch testfsadd2\n");
//     printf("FAT[0]: %d\n", FAT[0]); //should be size
//     printf("FAT[1]: %d\n", FAT[1]); //should be 0xFFFF for root directory
//     printf("FAT[2]: %d\n", FAT[2]); //should be 0 as nothing in file
//     printf("FAT[3]: %d\n", FAT[3]); //should be 0 as nothing in file
//     printf("%d\n", DATA[BLOCKS_SIZE_CONFIG]); //should be 0
//     printf("%d\n", DATA[BLOCKS_SIZE_CONFIG + 1]); //should be 0
    
//     cat_append("testfsadd", "hello\n");

//     printf("After append testfsadd\n");
//     printf("FAT[0]: %d\n", FAT[0]); //should be size
//     printf("FAT[1]: %d\n", FAT[1]); //should be 0xFFFF for root directory
//     printf("FAT[2]: %d\n", FAT[2]); //should be 0xFFFF for file
//     printf("FAT[3]: %d\n", FAT[3]); //should be 0 as nothing in file
//     printf("%d\n", DATA[BLOCKS_SIZE_CONFIG]); //h
//     printf("%d\n", DATA[BLOCKS_SIZE_CONFIG + 1]); //e
//     printf("%d\n", DATA[BLOCKS_SIZE_CONFIG + 2]); //l
//     printf("%d\n", DATA[BLOCKS_SIZE_CONFIG + 3]); //l
//     printf("%d\n", DATA[BLOCKS_SIZE_CONFIG + 4]); //o
//     printf("%d\n", DATA[BLOCKS_SIZE_CONFIG + 5]); //\n
//     printf("%d\n", DATA[BLOCKS_SIZE_CONFIG + 6]); //should be 0

//     cat_append("testfsadd2", "llo\n");

//     printf("After append testfsadd2\n"); //correct
//     printf("FAT[0]: %d\n", FAT[0]); //should be size
//     printf("FAT[1]: %d\n", FAT[1]); //should be 0xFFFF for root directory
//     printf("FAT[2]: %d\n", FAT[2]); //should be 0
//     printf("FAT[3]: %d\n", FAT[3]); //should be 0xFFFF for file//
//     printf("%d\n", DATA[BLOCKS_SIZE_CONFIG * 2]); //l
//     printf("%d\n", DATA[BLOCKS_SIZE_CONFIG * 2 + 1]); //l
//     printf("%d\n", DATA[BLOCKS_SIZE_CONFIG* 2 + 2]); //o
//     printf("%d\n", DATA[BLOCKS_SIZE_CONFIG* 2 + 3]); //\n
//     printf("%d\n", DATA[BLOCKS_SIZE_CONFIG *2 + 4]); //should be 0
//     printf("%d\n", DATA[BLOCKS_SIZE_CONFIG*2+ 5]); //should be 0 
//     printf("%d\n", DATA[BLOCKS_SIZE_CONFIG*2 + 6]); //should be 0


//     cat_overwrite("testfsadd", "he");

//     printf("After overwrite\n");
//     printf("FAT[0]: %d\n", FAT[0]); //should be size
//     printf("FAT[1]: %d\n", FAT[1]); //should be 0xFFFF for root directory
//     printf("FAT[2]: %d\n", FAT[2]); //should be 0xFFFF for file
//     printf("FAT[3]: %d\n", FAT[3]); //should be 0xFFFF for file
//     printf("%d\n", DATA[BLOCKS_SIZE_CONFIG]); //h
//     printf("%d\n", DATA[BLOCKS_SIZE_CONFIG + 1]); //e
//     printf("%d\n", DATA[BLOCKS_SIZE_CONFIG + 2]); //should not be 0 \n
//     printf("%d\n", DATA[BLOCKS_SIZE_CONFIG + 3]); //should be 0
//     printf("%d\n", DATA[BLOCKS_SIZE_CONFIG + 4]); //should be 0
//     printf("%d\n", DATA[BLOCKS_SIZE_CONFIG + 5]); //should be 0
//     printf("%d\n", DATA[BLOCKS_SIZE_CONFIG + 6]); //should be 0

//     //ls();

//     //Test for cat concatenate
//     char* list[] = {"testfsadd", "testfsadd2"};
//     char* result = cat_concatenate(list, 2);
//     printf("Result of concatenate: %s\n", result); //should be hello\n

//     ls(); //only prints out one file

//     remove_file("testfsadd");

//     printf("After remove file\n");
//     printf("FAT[0]: %d\n", FAT[0]);
//     printf("FAT[1]: %d\n", FAT[1]);
//     printf("FAT[2]: %d\n", FAT[2]); //should not be 0
//     printf("FAT[3]: %d\n", FAT[3]); //should be 0xFFFF for file
//     printf("%d\n", DATA[BLOCKS_SIZE_CONFIG]); //should be 0
//     printf("%d\n", DATA[BLOCKS_SIZE_CONFIG + 1]); //should not be 0 e
//     printf("%d\n", DATA[BLOCKS_SIZE_CONFIG + 2]); //should not be 0 \n
//     printf("%d\n", DATA[BLOCKS_SIZE_CONFIG + 3]); //should not be 0 nothing
//     printf("%d\n", DATA[BLOCKS_SIZE_CONFIG + 4]); //should not be 0 nothing
//     printf("%d\n", DATA[BLOCKS_SIZE_CONFIG + 5]); //should not be 0 nothing
//     printf("%d\n", DATA[BLOCKS_SIZE_CONFIG + 6]); //should not be 0 nothing
    
//     ls();
//     entry* e = find_file("testfsadd");
//     if (e == NULL) {
//         printf("testfsadd entry has been deleted\n");
//     } else {
//         printf("testfsadd entry has not been deleted\n");
//     }
//     e = find_file("testfsadd2");
//     if (e == NULL) {
//         printf("testfsadd2 entry has been deleted\n");
//     } else {
//         printf("testfsadd2 entry has not been deleted\n");
//     }

    
//     unmount();


// }

