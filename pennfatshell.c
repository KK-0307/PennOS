#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <stdbool.h>
#include <sys/wait.h>
#include <sys/types.h>
#include "parser.h"
#include "pennfat.h"

const int MAX_LINE_LENGTH = 4096;



int num_arguments(char* command) {
    int count = 0;
    while(*command != '\0') {
        if (*command == ' ') {
            count++;
        }
        command++;
    }
    return count;
}

int main() {
    while(true) {
        //Write prompt
        ssize_t written = write(STDOUT_FILENO, PROMPT, strlen(PROMPT));
        if (written == -1) {
            perror("write");
            exit(EXIT_FAILURE);
        }

        //Read command
        char cmd[MAX_LINE_LENGTH];
        int numBytes = read(STDIN_FILENO, cmd, MAX_LINE_LENGTH);
        if (numBytes < 0) {
            perror("read");
            exit(EXIT_FAILURE);
        } else if (numBytes == 0) {
            kill(0, SIGKILL);
            kill(1, SIGKILL);
        }

        cmd[numBytes - 1] = '\0';

        //If just enter is pressed
        if (cmd[0] == '\0') {
            continue;
        }

        struct parsed_command *command;
        int parse_result = parse_command(cmd, &command);
        if (parse_result != 0) {
            fprintf(stderr, "Error: Invalid Command\n");
            continue;
        }

        char* keyword = command->commands[0][0];

        if (strcmp(keyword, "mkfs") == 0) {
            char* fs_name = command->commands[0][1];
            //printf("fs_name: %s\n", fs_name);
            char* blocks_in_fat_char = command->commands[0][2];
            //printf("blocks_in_fat_char: %s\n", blocks_in_fat_char);
            char* blocks_size_char = command->commands[0][3];
            //printf("blocks_size_char: %s\n", blocks_size_char);
            int blocks_in_fat = atoi(blocks_in_fat_char);
            int blocks_size = atoi(blocks_size_char);

            mkfs(fs_name, blocks_in_fat, blocks_size);
        } 

        else if (strcmp(keyword, "mount") == 0) {
            char *token = strtok(cmd, " ");
            token = strtok(NULL, " ");
            char* file_name = token; //temporary
            mount(file_name);
        } 
        else if (strcmp(keyword, "umount") == 0) {
            unmount();
        } 
        else if (strcmp(keyword, "touch") == 0) {
            //find a way to count number of files
            int num_files = num_arguments(cmd);
            char *token = strtok(cmd, " ");
            for (int i = 0; i < num_files; i++) {
                token = strtok(NULL, " ");
                //printf("Token: %s\n", token);
                touch(token);
            }  
        }
        else if (strcmp(keyword, "mv") == 0) {
            char *token = strtok(cmd, " ");
            token = strtok(NULL, " ");
            char* source = token;
            //printf("Source: %s\n", source);
            char* dest = strtok(NULL, " ");
            //printf("Dest: %s\n", dest);
            mv(source, dest);
        }
        else if (strcmp(keyword, "rm") == 0) {
            //find a way to count number of files
            int num_files = num_arguments(cmd);
            char *token = strtok(cmd, " ");
            for (int i = 0; i < num_files; i++) {
                token = strtok(NULL, " ");
                remove_file(token);
            }  
        }  //cat commands
        else if (strcmp(keyword, "cat") == 0) {
            char* cmd_copy = malloc(strlen(cmd) + 1);
            char* cmd_copy2 = malloc(strlen(cmd) + 1);;
            strcpy(cmd_copy, cmd);
            strcpy(cmd_copy2, cmd);
            strtok(cmd, " ");
            char* second_keyword = strtok(NULL, " ");
            //cat -w OUTPUT_FILE
            if (strcmp(second_keyword, "-w") == 0) {
                char input_array[MAX_LINE_LENGTH];
                int numBytes = read(STDIN_FILENO, input_array, MAX_LINE_LENGTH);
                if (numBytes < 0) {
                    perror("read");
                    exit(EXIT_FAILURE);
                } 
                input_array[numBytes] = '\0';
                char* input = (char*) input_array;
                char* file_name = strtok(NULL, " ");
                cat_overwrite(file_name, input);
            }
            //cat -a OUTPUT_FILE
            else if (strcmp(second_keyword, "-a") == 0) {
                char input_array[MAX_LINE_LENGTH];
                int numBytes = read(STDIN_FILENO, input_array, MAX_LINE_LENGTH);
                if (numBytes < 0) {
                    perror("read");
                    exit(EXIT_FAILURE);
                } 
                input_array[numBytes] = '\0';
                char* input = (char*) input_array;
                char* file_name = strtok(NULL, " ");
                cat_append(file_name, input);
            }
            else {
                int num_args = num_arguments(cmd_copy);
                //printf("%d\n", num_args);
                char *token = strtok(cmd_copy, " ");
                char** files = (char**) malloc(num_args - 2);
                for (int i = 0; i < num_args-2; i++) {
                    token = strtok(NULL, " ");
                    //printf("Token: %s\n", token);
                    files[i] = token;
                } 
                char* third_keyword = strtok(NULL, " ");
                //printf("third keyword: %s\n", third_keyword);
                char* output_file = strtok(NULL, " ");
                //printf("output file: %s\n", output_file);
                //cat FILE ... [ -w OUTPUT_FILE ]
                if (strcmp(third_keyword, "-w") == 0) {
                    cat_concatenate_redirection_overwrite(files, num_args - 2, output_file);
                }
                //cat FILE ... [ -a OUTPUT_FILE ]
                else if (strcmp(third_keyword, "-a") == 0) {
                    cat_concatenate_redirection_append(files, num_args - 2, output_file);
                } 
                //stdout
                else {
                    char* files_stdout[num_args];
                    char *token = strtok(cmd_copy2, " ");
                    for (int i = 0; i < num_args; i++) {
                        token = strtok(NULL, " ");
                        files_stdout[i] = token;
                    } 
                    cat_concatenate_stdout(files_stdout, num_args);
                }
            }
        } 
        else if (strcmp(keyword, "cp") == 0) {
            strtok(cmd, " ");
            char* second_keyword = strtok(NULL, " ");
            //cp [ -h ] SOURCE DEST, source is from host OS
            if (strcmp(second_keyword, "-h") == 0) {
                char* source = strtok(NULL, " ");
                char* dest = strtok(NULL, " ");
                cp(source, dest, true);
            }
            //cp SOURCE -h DEST, dest is from host OS
            else {
                char* source = second_keyword;
                strtok(NULL, " ");
                char* dest = strtok(NULL, " ");
                cp(source, dest, false);
            }
        }
        else if (strcmp(keyword, "ls") == 0) {
            ls();
        }
        else if (strcmp(keyword, "chmod") == 0) {
            strtok(cmd, " ");
            char* permission_char = strtok(NULL, " ");
            int permission_int = atoi(permission_char);
            uint8_t permission = (uint8_t) permission_int;
            char* file_name = strtok(NULL, " ");
            chmod_file(file_name, permission);
        } else {
            printf("Unknown command\n");
        }

        
        



    }
}
