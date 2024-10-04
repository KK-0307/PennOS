#ifndef KERNEL_H
#define KERNEL_H

#include <signal.h> 	// sigaction, sigemptyset, sigfillset, signal
#include <stdlib.h> 	// malloc, free
#include <sys/time.h> 	// setitimer
#include <ucontext.h> 	// getcontext, makecontext, setcontext, swapcontext

#include "pcb_linked_list.h"

struct PCB* k_process_create(struct PCB* parent);

int k_process_kill(struct PCB* process, int signal);

void k_process_cleanup(struct PCB* process);

void k_log_event(int event_id, struct PCB* curr_pcb, int optional);

void k_print_ps();

void k_print_jobs();

int k_fg(char* job_id);

int k_bg(char* job_id);

int k_add_job(int pid);

void k_logout();

int addStatus(int parent_pid, int child_pid, int update_status);

#endif // KERNEL_H