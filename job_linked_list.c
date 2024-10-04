#include <signal.h>   // sigaction, sigemptyset, sigfillset, signal
#include <stdlib.h>   // malloc, free
#include <sys/time.h> // setitimer
#include <ucontext.h> // getcontext, makecontext, setcontext, swapcontext
#include <valgrind/valgrind.h>
#include "entry.h"
#include "syscalls.h"

// FOR TESTING, REMOVE LATER
#include <stdio.h>

#include "linked_list.h"
#include "pcb_linked_list.h"

struct Job
{
    // Current job id
    int job_id;

    // Job's associated pcb
    struct PCB *pcb;

    // Next job
    struct Job *next;

    // Most recent stop/background time
    int stop_background_time;
};

struct Job *create_new_job(struct PCB *pcb, int job_id)
{
    struct Job *new_job = (struct Job *)malloc(sizeof(struct Job));
    if (new_job == NULL)
    {
        // EXIT
    }

    new_job->job_id = job_id;

    new_job->pcb = pcb;

    new_job->stop_background_time = 0;

    return new_job;
}

void append_job(struct Job **head, struct Job *new_job)
{

    new_job->next = NULL;

    if (*head == NULL)
    {
        *head = new_job;
        return;
    }

    struct Job *cur_job = *head;

    while (cur_job->next != NULL)
    {
        cur_job = cur_job->next;
    }

    cur_job->next = new_job;
}

struct Job* find_job(struct Job *head, struct PCB* process, int* job_id, int* is_head) {
    struct Job *cur_job = head;
    struct Job *found_job = NULL;
    int recent_time = 0;

    while(cur_job != NULL) {
        if(cur_job->stop_background_time > recent_time) {
            recent_time = cur_job->stop_background_time;
        }
        if(cur_job->pcb == process) {
            *job_id = cur_job->job_id;
            found_job = cur_job;
        }

        cur_job = cur_job->next;
    }

    if(found_job == NULL) {
        return NULL;
    }

    if(recent_time == found_job->stop_background_time) {
        *is_head = 1;
    }
    return found_job;
}

int delete_job(struct Job **head, int thread_pid)
{
    struct Job *temp = *head;
    struct Job *prev = NULL;

    if (temp != NULL && temp->job_id == thread_pid)
    {
        *head = temp->next;
        free(temp);
        return thread_pid;
    }

    while (temp != NULL && temp->job_id != thread_pid)
    {
        prev = temp;
        temp = temp->next;
    }

    if (temp == NULL)
    {
        return -1;
    }

    prev->next = temp->next;
    free(temp);
    return thread_pid;
}

void print_ps(struct Job *head)
{
    // printAllThreads();
    
    struct Job *cur_job = head;

    f_write(STDOUT_FILENO, "PID\tPPID\tPRI\tSTAT\tCMD\n", 23);

    if (cur_job == NULL)
    {
        return;
    }

    int buf_size = 100;

    char ps_string[buf_size];
    sprintf(ps_string, "%d", cur_job->pcb->thread_pid);
    f_write(STDOUT_FILENO, ps_string, strlen(ps_string));
    f_write(STDOUT_FILENO, "\t", 2);

    sprintf(ps_string, "%d", cur_job->pcb->parent_pid);
    f_write(STDOUT_FILENO, ps_string, strlen(ps_string));
    f_write(STDOUT_FILENO, "\t", 2);

    sprintf(ps_string, "%d", cur_job->pcb->priority_level);
    f_write(STDOUT_FILENO, ps_string, strlen(ps_string));
    f_write(STDOUT_FILENO, "\t", 2);

    if(cur_job->pcb->status == 0) {
        f_write(STDOUT_FILENO, "R", 2);
        f_write(STDOUT_FILENO, "\t", 2);
    } else if(cur_job->pcb->status == 1) {
        f_write(STDOUT_FILENO, "S", 2);
        f_write(STDOUT_FILENO, "\t", 2);
    } else if(cur_job->pcb->status == 2) {
        f_write(STDOUT_FILENO, "B", 2);
        f_write(STDOUT_FILENO, "\t", 2);
    } else if(cur_job->pcb->status == 3) {
        f_write(STDOUT_FILENO, "Z", 2);
        f_write(STDOUT_FILENO, "\t", 2);
    } else {
        f_write(STDOUT_FILENO, "?", 2);
        f_write(STDOUT_FILENO, "\t", 2);
    }

    for(int j = 0; j < buf_size; j++) {
        ps_string[j] = '\0';
    }
    int i = 0;
    f_write(STDOUT_FILENO, cur_job->pcb->prog, strlen(cur_job->pcb->prog));
    while(cur_job->pcb->command != NULL) {
        f_write(STDOUT_FILENO, cur_job->pcb->command[i], strlen(cur_job->pcb->command[i]));
        i++;
    }
    f_write(STDOUT_FILENO, "\n", 2);


    while (cur_job->next != NULL)
    {
        cur_job = cur_job->next;

        sprintf(ps_string, "%d", cur_job->pcb->thread_pid);
        f_write(STDOUT_FILENO, ps_string, strlen(ps_string));
        f_write(STDOUT_FILENO, "\t", 2);

        sprintf(ps_string, "%d", cur_job->pcb->parent_pid);
        f_write(STDOUT_FILENO, ps_string, strlen(ps_string));
        f_write(STDOUT_FILENO, "\t", 2);

        sprintf(ps_string, "%d", cur_job->pcb->priority_level);
        f_write(STDOUT_FILENO, ps_string, strlen(ps_string));
        f_write(STDOUT_FILENO, "\t", 2);

        if(cur_job->pcb->status == 0) {
            f_write(STDOUT_FILENO, "R", 2);
            f_write(STDOUT_FILENO, "\t", 2);
        } else if(cur_job->pcb->status == 1) {
            f_write(STDOUT_FILENO, "S", 2);
            f_write(STDOUT_FILENO, "\t", 2);
        } else if(cur_job->pcb->status == 2) {
            f_write(STDOUT_FILENO, "B", 2);
            f_write(STDOUT_FILENO, "\t", 2);
        } else if(cur_job->pcb->status == 3) {
            f_write(STDOUT_FILENO, "Z", 2);
            f_write(STDOUT_FILENO, "\t", 2);
        } else {
            f_write(STDOUT_FILENO, "?", 2);
            f_write(STDOUT_FILENO, "\t", 2);
        }

        i = 1;
        f_write(STDOUT_FILENO, cur_job->pcb->prog, strlen(cur_job->pcb->prog));

        if(cur_job->pcb != NULL && cur_job->pcb->command != NULL) {
            while(cur_job->pcb->command[i] != NULL) {
                f_write(STDOUT_FILENO, " ", 2);
                f_write(STDOUT_FILENO, cur_job->pcb->command[i], strlen(cur_job->pcb->command[i]));
                i++;
            }
        }
        f_write(STDOUT_FILENO, "\n", 2);
        


        for(int j = 0; j < buf_size; j++) {
            ps_string[j] = '\0';
        }
    }
}

void print_job(struct Job* job) {
    int buf_size = 100;

    f_write(STDOUT_FILENO, "[", 2);
    char ps_string[buf_size];
    sprintf(ps_string, "%d", job->job_id);
    f_write(STDOUT_FILENO, ps_string, strlen(ps_string));
    f_write(STDOUT_FILENO, "] ", 2);


    int i = 1;
    f_write(STDOUT_FILENO, job->pcb->prog, strlen(job->pcb->prog));

    while(job->pcb->command[i] != NULL) {
        f_write(STDOUT_FILENO, " ", 2);
        f_write(STDOUT_FILENO, job->pcb->command[i], strlen(job->pcb->command[i]));
        f_write(STDOUT_FILENO, " ", 2);
        i++;
    }

    if(job->pcb->status == 0) {
        f_write(STDOUT_FILENO, " (running)\n", 11);
    } else if(job->pcb->status == 1) {
        f_write(STDOUT_FILENO, " (stopped)\n", 11);
    } else if(job->pcb->status == 2) {
        f_write(STDOUT_FILENO, " (blocked)\n", 11);
    } 
}

void print_finished_job(struct Job* job, int is_head) {
    int buf_size = 100;

    f_write(STDOUT_FILENO, "\n", 2);
    f_write(STDOUT_FILENO, "[", 2);
    char ps_string[buf_size];
    sprintf(ps_string, "%d", job->job_id);
    f_write(STDOUT_FILENO, ps_string, strlen(ps_string));
    f_write(STDOUT_FILENO, "]", 2);
    
    if(is_head == 1) {
        f_write(STDOUT_FILENO, "+", 2);
    } else {
        f_write(STDOUT_FILENO, " ", 2);
    }
    f_write(STDOUT_FILENO, " ", 2);

    f_write(STDOUT_FILENO, "DONE ", 6);

    f_write(STDOUT_FILENO, job->pcb->prog, strlen(job->pcb->prog));

    f_write(STDOUT_FILENO, " ", 2);

    int i = 1;
    while(job->pcb->command[i] != NULL) {
        f_write(STDOUT_FILENO, job->pcb->command[i], strlen(job->pcb->command[i]));
        f_write(STDOUT_FILENO, " ", 2);
        i++;
    }
    f_write(STDOUT_FILENO, "\n", 2);
    
}

void j_free_list(struct Job *head) {
    struct Job *cur_node = head;
    while (cur_node != NULL)
    {
        struct Job *temp = cur_node;
        cur_node = cur_node->next;
        free(temp);
    }
}

void print_jobs(struct Job* head, int fg) {
    struct Job *cur_job = head;
    while(cur_job != NULL) {
        if(cur_job->pcb->thread_pid != fg) {
            print_job(cur_job);
        }
        cur_job = cur_job->next;
    }
}

