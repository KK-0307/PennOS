#ifndef PCB_LINKED_LIST_H
#define PCB_LINKED_LIST_H

#include <signal.h>   // sigaction, sigemptyset, sigfillset, signal
#include <stdlib.h>   // malloc, free
#include <sys/time.h> // setitimer
#include <ucontext.h> // getcontext, makecontext, setcontext, swapcontext
#include "entry.h"

struct PCB
{
    // Current PCB's associated ucontext
    ucontext_t context;

    // Current PCB's process id
    int thread_pid;

    // Current PCB's parent pid, usually shell
    int parent_pid;

    // Linked_list storing children_pids, initialized empty
    struct Node *children_pids;

    // Open file descriptor table, see kernel for description
    int open_fds[16][2];
    // Priority level for PCB, 0 by default
    int priority_level;

    // Whether or not the PCB is the shell
    int is_shell;

    // Statuses for children that have changed state
    struct Node *child_statuses;

    // PCB status
    /*
    Statuses:
    0 : running
    1 : stopped
    2 : blocked
    3 : zombied
    */
    int status;

    // Reaped child pid
    int reaped_child;

    // number of children
    int num_children;

    // String describing program, i.e. echo, shell, etc.
    char *prog;

    // Pointer to next PCB in the linked list
    struct PCB *next;

    char **command;
};

struct PCB *create_new_node(ucontext_t context);

void insert(struct PCB **head, struct PCB *new_thread);

void append(struct PCB **head, struct PCB *new_thread);

struct PCB *get_first(struct PCB **head);

int delete(struct PCB **head, int thread_pid);

int delete_first(struct PCB **head);

struct PCB *remove_thread(struct PCB **head, int thread_pid);

struct PCB *remove_threads_parent(struct PCB **head, int parent_pid);

void print_threads(struct PCB *head);

struct PCB *find_thread(struct PCB *head, int thread_pid);

void free_list(struct PCB *head);

#endif // PCB_LINKED_LIST_H