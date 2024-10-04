#ifndef USER_FUNCTIONS_H
#define USER_FUNCTIONS_H

#include <signal.h> // sigaction, sigemptyset, sigfillset, signal
#include <stdlib.h> // malloc, free
#include <stdbool.h>

int p_nice(int pid, int priority);

void p_sleep(unsigned int ticks);

int p_spawn(void (*func)(), char *argv[], int fd0, int fd1);

int p_waitpid(int pid, int *wstatus, bool nohang);

int p_kill(int pid, int sig);

int p_fg(char* job_id);

int p_bg(char* job_id);

void p_add_job(int pid);

void p_print_ps();

void p_print_jobs();

void p_exit();

void p_logout();

int p_fg();

void p_set_terminal_control(int pid);

#endif // USER_FUNCTIONS_H