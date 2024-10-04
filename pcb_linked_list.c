#include <signal.h>   // sigaction, sigemptyset, sigfillset, signal
#include <stdlib.h>   // malloc, free
#include <sys/time.h> // setitimer
#include <ucontext.h> // getcontext, makecontext, setcontext, swapcontext
#include <valgrind/valgrind.h>
#include "entry.h"

// FOR TESTING, REMOVE LATER
#include <stdio.h>

#include "linked_list.h"

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

    // Save command that spawned this PCB
    char **command;
};

struct PCB *create_new_node(ucontext_t context)
{
    struct PCB *new_pcb = (struct PCB *)malloc(sizeof(struct PCB));
    if (new_pcb == NULL)
    {
        // EXIT
    }

    new_pcb->context = context;

    new_pcb->next = NULL;

    return new_pcb;
}

void insert(struct PCB **head, struct PCB *new_thread)
{
    new_thread->next = *head;
    *head = new_thread;
}

void append(struct PCB **head, struct PCB *new_thread)
{

    new_thread->next = NULL;

    if (*head == NULL)
    {
        *head = new_thread;
        return;
    }

    struct PCB *cur_pcb = *head;

    while (cur_pcb->next != NULL)
    {
        cur_pcb = cur_pcb->next;
    }

    cur_pcb->next = new_thread;
}

struct PCB *get_first(struct PCB **head)
{
    if (*head == NULL)
    {
        return NULL;
    }

    struct PCB *temp = *head;
    *head = temp->next;
    temp->next = NULL;

    return temp;
}

int delete(struct PCB **head, int thread_pid)
{
    struct PCB *temp = *head;
    struct PCB *prev = NULL;

    if (temp != NULL && temp->thread_pid == thread_pid)
    {
        *head = temp->next;
        l_free_list(temp->children_pids);
        free(temp);
    }

    while (temp != NULL && temp->thread_pid != thread_pid)
    {
        prev = temp;
        temp = temp->next;
    }

    if (temp == NULL)
    {
        return -1;
    }

    prev->next = temp->next;
    l_free_list(temp->children_pids);
    free(temp);
    return thread_pid;
}

int delete_first(struct PCB **head)
{
    struct PCB *temp = *head;

    if (temp != NULL)
    {
        int ret_val = temp->thread_pid;
        *head = temp->next;
        l_free_list(temp->children_pids);
        free(temp);
        return ret_val;
    }
    else
    {
        return -1;
    }
}

struct PCB *remove_thread(struct PCB **head, int thread_pid)
{
    struct PCB *temp = *head;
    struct PCB *prev = NULL;

    if (temp != NULL && temp->thread_pid == thread_pid)
    {
        *head = temp->next;
        return temp;
    }

    while (temp != NULL && temp->thread_pid != thread_pid)
    {
        prev = temp;
        temp = temp->next;
    }

    if (temp == NULL)
    {
        return NULL;
    }

    prev->next = temp->next;

    return temp;
}

struct PCB *remove_threads_parent(struct PCB **head, int parent_pid)
{
    struct PCB *newHead = NULL;
    struct PCB *newTail = NULL;
    struct PCB *temp = *head;
    struct PCB *prev = NULL;

    if (temp != NULL)
    {
        if (temp->parent_pid == parent_pid)
        {
            *head = temp->next;
            temp->next = NULL;
            newHead = temp;
            newTail = temp;
            temp = *head;
        }
        else
        {
            prev = temp;
            temp = temp->next;
        }
    }

    while (temp != NULL)
    {
        if (temp->parent_pid == parent_pid)
        {

            if (newHead == NULL)
            {
                newHead = temp;
            }
            else
            {
                newTail->next = temp;
            }
            newTail = temp;
            temp = temp->next;
            newTail->next = NULL;
            if (prev != NULL)
            {
                prev->next = temp;
            }
        }
        else
        {
            prev = temp;
            temp = temp->next;
        }
    }
    return newHead;
}

void print_threads(struct PCB *head)
{
    struct PCB *cur_PCB = head;
    if (cur_PCB == NULL)
    {
        printf("EMPTY\n");
        return;
    }

    printf("THREAD %d | ", cur_PCB->thread_pid);

    while (cur_PCB->next != NULL)
    {
        cur_PCB = cur_PCB->next;
        printf("THREAD %d | ", cur_PCB->thread_pid);
    }
    printf("\n");
}

struct PCB *find_thread(struct PCB *head, int thread_pid)
{
    struct PCB *cur_PCB = head;

    while (cur_PCB != NULL)
    {
        if (cur_PCB->thread_pid == thread_pid)
        {
            return cur_PCB;
        }
        cur_PCB = cur_PCB->next;
    }
    return NULL;
}

void free_list(struct PCB *head)
{
    if (head == NULL)
    {
        return;
    }
    struct PCB *cur_pcb = head;
    while (cur_pcb != NULL)
    {
        struct PCB *temp = cur_pcb;
        cur_pcb = cur_pcb->next;

        // Free context stack
        if (temp->context.uc_stack.ss_sp != NULL)
        {
            VALGRIND_STACK_DEREGISTER(temp->context.uc_stack.ss_sp);
            free(temp->context.uc_stack.ss_sp);
        }

        if (temp->children_pids != NULL)
        {
            l_free_list(temp->children_pids);
        }

        free(temp);
    }
}
