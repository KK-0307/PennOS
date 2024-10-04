#ifndef ERROR_H
#define ERROR_H

#include "syscalls.h"

extern int ERRNO; 

// Invalid Command
#define INVCOM 30
#define SLPSEC 31
#define WAITERR 32
#define INVPID 33
#define INVSIG 34
#define INVPRIO 35

void p_error(char* message);

#endif // ERROR_H