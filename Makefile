
PROG=penn-os

SCHED_PROMPT = '"$ "'

INT_PROG = integrate
INT_PROMPT='"$ "'

FAT_PROG=pennfat
FAT_PROMPT='"$(FAT_PROG)> "'

CC = clang
CFLAGS = -Wall -Werror -g -O1

SRCS = $(wildcard *.c)
OBJS = $(SRCS:.c=.o)
HEADERS = $(wildcard *.h)

FAT_FILES = pennfat.c pennfatshell.c
SCHED_FILES = error.c kernel.c linked_list.c pcb_linked_list.c scheduler.c syscalls.c user_functions.c user_programs.c job_linked_list.c
INTEGRATE_FILES = error.c file_system2.c kernel.c linked_list.c pcb_linked_list.c pennfat.c scheduler.c syscalls.c user_functions.c user_programs.c testmain.c job_linked_list.c stress.c

override CPPFLAGS += -DNDEBUG -DPROMPT=$(PROMPT)

.PHONY : clean

$(PROG) : $(OBJS) $(HEADERS)
	$(CC) -o $@ $(OBJS) parser.o

%.o: %.c $(HEADERS)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $<

pennfat: 
	$(CC) $(FAT_FILES) -DNDEBUG -DPROMPT=$(FAT_PROMPT) $(CFLAGS) -o pennfat parser.o

scheduler:
	$(CC) $(SCHED_FILES) -DNDEBUG -DPROMPT=$(SCHED_PROMPT) $(CFLAGS) -o penn-os parser.o

integrate:
	$(CC) $(INTEGRATE_FILES) -DNDEBUG -DPROMPT=$(INT_PROMPT) $(CFLAGS) -o integrate parser.o


clean :
	$(RM) $(OBJS) $(PROG) fs integrate