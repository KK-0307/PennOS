#ifndef JOB_LINKED_LIST_H
#define JOB_LINKED_LIST_H

#include <signal.h>   // sigaction, sigemptyset, sigfillset, signal
#include <stdlib.h>   // malloc, free
#include <sys/time.h> // setitimer
#include <ucontext.h> // getcontext, makecontext, setcontext, swapcontext

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

struct Job *create_new_job(struct PCB *pcb, int job_id);

void append_job(struct Job **head, struct Job *new_job);

struct Job* find_job(struct Job *head, struct PCB* process, int* job_id, int* is_head);

int delete_job(struct Job **head, int thread_pid);

void print_ps(struct Job *head);

void print_jobs(struct Job* head, int fg);

void print_finished_job(struct Job* job, int is_head);

void j_free_list(struct Job *head);

#endif