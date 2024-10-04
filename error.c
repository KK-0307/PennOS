#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "syscalls.h"

#include "error.h"
#include "parser.h"


void p_error(char* message) {
    if (message != NULL && *message != '\0') {
        f_write(STDOUT_FILENO, message, strlen(message) + 1);
        f_write(STDOUT_FILENO, ": ", 3);
        
        if(ERRNO == INVCOM) {
            f_write(STDOUT_FILENO, "invalid command\n", 17);
        } else if(ERRNO == SLPSEC) {
            f_write(STDOUT_FILENO, "invalid sleep time\n", 20);
        } else if(ERRNO == WAITERR) {
            f_write(STDOUT_FILENO, "invalid wait\n", 14);
        } else if(ERRNO == INVPID) {
            f_write(STDOUT_FILENO, "invalid pid\n", 13);
        } else if(ERRNO == INVSIG) {
            f_write(STDOUT_FILENO, "invalid signal\n", 16);
        } else if(ERRNO == INVPRIO) {
            f_write(STDOUT_FILENO, "invalid priority\n", 18);
        } else if(ERRNO == UNEXPECTED_FILE_INPUT) {
            f_write(STDOUT_FILENO, "parser encountered an unexpected file input token '<'\n", 55);
        } else if(ERRNO == UNEXPECTED_FILE_OUTPUT) {
            f_write(STDOUT_FILENO, "parser encountered an unexpected file output token '>'\n", 56);
        } else if(ERRNO == UNEXPECTED_PIPELINE) {
            f_write(STDOUT_FILENO, "parser encountered an unexpected pipeline token '|'\n", 53);
        } else if(ERRNO == UNEXPECTED_AMPERSAND) {
            f_write(STDOUT_FILENO, "parser encountered an unexpected ampersand token '&'\n", 54);
        } else if(ERRNO == EXPECT_INPUT_FILENAME) {
            f_write(STDOUT_FILENO, "parser didn't find input filename following '<'\n", 49);
        } else if(ERRNO == EXPECT_OUTPUT_FILENAME) {
            f_write(STDOUT_FILENO, "parser didn't find output filename following '>' or '>>'\n", 58);
        } else if(ERRNO == EXPECT_COMMANDS) {
            f_write(STDOUT_FILENO, "parser didn't find any commands or arguments where it expects one\n", 18);
        } else  {
            f_write(STDOUT_FILENO, "ERRNO NOT SPECIFIED\n", 21);
        }
    }
}