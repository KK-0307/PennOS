#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <signal.h>   // sigaction, sigemptyset, sigfillset, signal
#include <stdlib.h>   // malloc, free
#include <sys/time.h> // setitimer
#include <ucontext.h> // getcontext, makecontext, setcontext, swapcontext
#include <valgrind/valgrind.h>

// Scheduler
void scheduler();

void makeSchedulerContext();

void setStack(stack_t *stack);

void makeContext(ucontext_t *ucp, void (*func)(), char *argv[]);

void scheduleSleep(int pid, int numTicks);

void deleteSleep(int pid);

// Clock
void setTimer(void);

void startClock();

int getTicks();

void tick();

// Contexts
void idle();

void wakeUp();

void alarmHandler(int signum);

void blockSignals();

void logoutDone();

void logout_context();

// Thread management
void addToQueue(struct PCB *new_thread);

int moveThread(int pid, int source, int dest);

struct PCB *getActive();

struct PCB *find_threads(int thread_pid, int *list_number);

// Terminal control
void hasTerminalControl();

void setTerminalControl(pid_t pid);

int getFgProcessId();

struct PCB *getFgProcess();

// Wait handling
int waitHandler(int pid, int *status);

void cleanFinishedProcess(struct PCB *process);

int reapZombies(int pid, int *status);

void cleanAllProcesses();

// Other

void printAllThreads();

#endif // SCHEDULER_H