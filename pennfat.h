#include <stdio.h>
#include <unistd.h>
#include "entry.h"
#include <stdbool.h>

extern int BLOCKS_IN_FAT;
extern int BLOCKS_SIZE_CONFIG_NUM;
extern int BLOCKS_SIZE_CONFIG;
extern int FAT_SIZE;
extern int FAT_FD;
extern uint16_t* FAT;
extern uint8_t* DATA;
extern int NUM_FAT_ENTRIES;
extern int NUM_ENTRIES_IN_BLOCK;
extern int FIRST_DATA_INDEX; //index 1, first index in the data region
extern int DATA_SIZE; //number of blocks in data region

entry* find_file(char* file_name);
int find_next_empty_block();
void next_empty_entry(entry** result_entry, int* result_fat, int* result_data);
int bytes_available(int data_index, int fat_index);
int traverse_fat(uint16_t starting_block);
void put_data_in_block(int index_of_file, char* input);
void mkfs(char* file_name, int blocks_in_fat, int block_size_config);
void mount(char* file_name);
void unmount();
void touch (char* file_name);
void mv(char* source, char* dest);
void remove_file(char* file_name);
void cat_append(char* file_name, char* input);
void cat_overwrite(char* file_name, char* input);
char* cat_concatenate(char* list_of_files[], int num_files);
void cat_concatenate_stdout(char* list_of_files[], int num_files);
void cat_concatenate_redirection_append(char* list_of_files[], int num_files, char* output_file);
void cat_concatenate_redirection_overwrite(char* list_of_files[], int num_files, char* output_file);
void ls();
void cp(char* source, char* dest, bool isHost);
void chmod_file(char* file_name, uint8_t new_perm);

