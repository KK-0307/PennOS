#include <signal.h>   // sigaction, sigemptyset, sigfillset, signal
#include <stdlib.h>   // malloc, free
#include <sys/time.h> // setitimer
#include <ucontext.h> // getcontext, makecontext, setcontext, swapcontext
#include <valgrind/valgrind.h>

#include "linked_list.h"
#include "pcb_linked_list.h"
#include "job_linked_list.h"
#include "kernel.h"
#include "error.h"
#include "syscall.h"

// FOR TESTING, REMOVE LATER
#include <stdio.h>
#include <unistd.h>
#include <string.h>

ucontext_t schedulerContext;
ucontext_t cleanupContext;
ucontext_t idleContext;
ucontext_t finishContext;

struct PCB *threads_neg = NULL;
struct PCB *threads_zero = NULL;
struct PCB *threads_pos = NULL;

sigset_t blocked_signals;

struct PCB *threads_stopped = NULL;
struct PCB *threads_blocked = NULL;
struct PCB *threads_zombied = NULL;

static ucontext_t *activeContext;
struct PCB *active_PCB = NULL;

static int fg_process = 1;

struct Node *waits = NULL;
struct Node *sleeps = NULL;
struct Node *sleeps_stopped = NULL;

static const int centisecond = 10000; // 10 milliseconds

static const int stack_size = 819200;

// Order of scheduling processes (has correct ratios)
static int order[19] = {1, -1, 0, -1, 0, -1, 1, -1, 0, -1, 0, -1, 1, -1, 0, -1, 0, -1, 1};

static int thread = 0;

static int ticks;

void printAllThreads()
{
    printf("ACTIVE\n");
    print_threads(active_PCB);
    printf("NEG\n");
    print_threads(threads_neg);
    printf("ZERO\n");
    print_threads(threads_zero);
    printf("POS\n");
    print_threads(threads_pos);
    printf("STOPPED\n");
    print_threads(threads_stopped);
    printf("BLOCKED\n");
    print_threads(threads_blocked);
    printf("ZOMBIED\n");
    print_threads(threads_zombied);
    printf("WAITS\n");
    print_list(waits);
    printf("SLEEPS\n");
    print_list(sleeps);
}

void scheduleIdle()
{
    activeContext = &idleContext;
    active_PCB = NULL;
    setcontext(activeContext);
}

void setTerminalControl(pid_t pid)
{
    fg_process = pid;
}

void hasTerminalControl()
{
    while (active_PCB->thread_pid != fg_process)
    {
        sigprocmask(SIG_BLOCK, &blocked_signals, &active_PCB->context.uc_sigmask);
        struct PCB *temp = active_PCB;
        active_PCB = NULL;
        activeContext = &idleContext;
        append(&threads_stopped, temp);
        temp->status = 1;
        k_log_event(10, temp, -1);
        swapcontext(&temp->context, activeContext);
        sigprocmask(SIG_UNBLOCK, &blocked_signals, &active_PCB->context.uc_sigmask);
    }
}

struct PCB *find_threads(int thread_pid, int *list_number)
{
    /*
    -1 : Not Found
    0 : Neg List
    1 : Zero List
    2 : Pos List
    3 : Stopped List
    4 : Blocked List
    5 : Zombie List
    6 : ACTIVE
    */
    struct PCB *found_thread = find_thread(threads_neg, thread_pid);
    if (found_thread != NULL)
    {
        *list_number = 0;
        return found_thread;
    }
    else
    {
        found_thread = find_thread(threads_zero, thread_pid);
    }
    if (found_thread != NULL)
    {
        *list_number = 1;
        return found_thread;
    }
    else
    {
        found_thread = find_thread(threads_pos, thread_pid);
    }
    if (found_thread != NULL)
    {
        *list_number = 2;
        return found_thread;
    }
    else
    {
        found_thread = find_thread(threads_stopped, thread_pid);
    }
    if (found_thread != NULL)
    {
        *list_number = 3;
        return found_thread;
    }
    else
    {
        found_thread = find_thread(threads_blocked, thread_pid);
    }
    if (found_thread != NULL)
    {
        *list_number = 4;
        return found_thread;
    }
    else
    {
        found_thread = find_thread(threads_zombied, thread_pid);
    }
    if (found_thread != NULL)
    {
        *list_number = 5;
        return found_thread;
    }
    else
    {
        if (active_PCB->thread_pid == thread_pid)
        {
            *list_number = 6;
            return active_PCB;
        }
        return NULL;
    }
}

struct PCB *remove_threads(int thread_pid, int *list_number)
{
    /*
    -1 : Not Found
    0 : Neg List
    1 : Zero List
    2 : Pos List
    3 : Stopped List
    4 : Blocked List
    5 : Zombie List
    6 : ACTIVE
    */
    struct PCB *removed_thread = remove_thread(&threads_neg, thread_pid);
    if (removed_thread != NULL)
    {
        *list_number = 0;
        return removed_thread;
    }
    else
    {
        removed_thread = remove_thread(&threads_zero, thread_pid);
    }
    if (removed_thread != NULL)
    {
        *list_number = 1;
        return removed_thread;
    }
    else
    {
        removed_thread = remove_thread(&threads_pos, thread_pid);
    }
    if (removed_thread != NULL)
    {
        *list_number = 2;
        return removed_thread;
    }
    else
    {
        removed_thread = remove_thread(&threads_stopped, thread_pid);
    }
    if (removed_thread != NULL)
    {
        *list_number = 3;
        return removed_thread;
    }
    else
    {
        removed_thread = remove_thread(&threads_blocked, thread_pid);
    }
    if (removed_thread != NULL)
    {
        *list_number = 4;
        return removed_thread;
    }
    else
    {
        removed_thread = remove_thread(&threads_zombied, thread_pid);
    }
    if (removed_thread != NULL)
    {
        *list_number = 5;
        return removed_thread;
    }
    else
    {
        if (active_PCB->thread_pid == thread_pid)
        {
            *list_number = 6;
            return active_PCB;
        }
        return NULL;
    }
}

int getFgProcessId()
{
    return fg_process;
}

struct PCB *getFgProcess()
{
    struct PCB *retval = find_thread(threads_pos, fg_process);
    if (retval != NULL)
    {
        return retval;
    }
    retval = find_thread(threads_zero, fg_process);
    if (retval != NULL)
    {
        return retval;
    }
    retval = find_thread(threads_neg, fg_process);
    if (retval != NULL)
    {
        return retval;
    }
    retval = find_thread(threads_stopped, fg_process);
    if (retval != NULL)
    {
        return retval;
    }
    retval = find_thread(threads_blocked, fg_process);
    if (retval != NULL)
    {
        return retval;
    }
    retval = find_thread(active_PCB, fg_process);
    return retval;
}

void addToQueue(struct PCB *new_thread)
{
    if (new_thread->priority_level == -1)
    {
        append(&threads_neg, new_thread);
    }
    else if (new_thread->priority_level == 0)
    {
        append(&threads_zero, new_thread);
    }
    else
    {
        append(&threads_pos, new_thread);
    }
}

int moveThread(int pid, int source, int dest)
{
    struct PCB *mov_pcb;
    int unblock_parent = 0;

    if (source == 0)
    {
        mov_pcb = remove_thread(&threads_neg, pid);
    }
    else if (source == 1)
    {
        mov_pcb = remove_thread(&threads_zero, pid);
    }
    else if (source == 2)
    {
        mov_pcb = remove_thread(&threads_pos, pid);
    }
    else if (source == 3)
    {
        mov_pcb = remove_thread(&threads_stopped, pid);
        mov_pcb->status = 0;
        k_log_event(11, mov_pcb, -1);
    }
    else if (source == 4)
    {
        mov_pcb = remove_thread(&threads_blocked, pid);
    }
    else if (source == 5)
    {
        mov_pcb = remove_thread(&threads_zombied, pid);
    }
    else if (source == 6)
    {
        mov_pcb = active_PCB;
    }
    else
    {
        return 1;
    }

    if (mov_pcb == NULL)
    {
        printf("MOV IS NULL\n");
        return 1;
    }

    if (dest == 0)
    {
        append(&threads_neg, mov_pcb);
    }
    else if (dest == 1)
    {
        append(&threads_zero, mov_pcb);
    }
    else if (dest == 2)
    {
        append(&threads_pos, mov_pcb);
    }
    else if (dest == 3)
    {
        // Remove sleep and add to stopped sleeps. Data2 is remaining sleep time
        if(strcmp(mov_pcb->prog, "sleep") == 0) {
            struct Node *removed_sleep = remove_data(&sleeps, mov_pcb->thread_pid);
            l_append_2(&sleeps_stopped, mov_pcb->thread_pid, removed_sleep->data2 - ticks);
        }
        
        unblock_parent = 1;
        append(&threads_stopped, mov_pcb);
        mov_pcb->status = 1;
        k_log_event(10, mov_pcb, -1);
    }
    else if (dest == 4)
    {
        append(&threads_blocked, mov_pcb);
        // If continued thread is sleep, add to sleeps
        if (strcmp(mov_pcb->prog, "sleep") == 0)
        {
            struct Node *continued_sleep = remove_data(&sleeps_stopped, mov_pcb->thread_pid);
            l_append_2(&sleeps, continued_sleep->data, ticks + continued_sleep->data2);
        }
        mov_pcb->status = 2;
    }
    else if (dest == 5)
    {
        append(&threads_zombied, mov_pcb);
    }
    else
    {
        printf("INVALID DEST\n");
        return -1;
    }

    if (unblock_parent == 1)
    {
        sigprocmask(SIG_BLOCK, &blocked_signals, &active_PCB->context.uc_sigmask);
        int is_active = 0;
        if (mov_pcb == active_PCB)
        {
            is_active = 1;
            active_PCB = NULL;
        }
        int i = search(&waits, mov_pcb->parent_pid, mov_pcb->thread_pid);

        if (i != -1)
        {
            if (fg_process == mov_pcb->thread_pid)
            {
                fg_process = mov_pcb->parent_pid;
            }
            struct PCB *temp = remove_thread(&threads_blocked, mov_pcb->parent_pid);
            temp->status = 0;
            temp->reaped_child = mov_pcb->thread_pid;
            k_log_event(9, temp, -1);
            k_log_event(6, mov_pcb, -1);
            addToQueue(temp);
        }
        sigprocmask(SIG_UNBLOCK, &blocked_signals, &active_PCB->context.uc_sigmask);
        if (is_active == 1)
        {
            setcontext(&idleContext);
        }
    }

    return 0;
}

void idle()
{
    sigset_t mask;
    sigemptyset(&mask);
    sigsuspend(&mask);
}

void wakeUp()
{
    struct Node *cur_node = sleeps;

    while (cur_node != NULL)
    {
        if (cur_node->data2 == ticks)
        {
            int pid = cur_node->data;
            struct PCB *wake_pcb = remove_thread(&threads_blocked, cur_node->data);

            cur_node = cur_node->next;
            l_delete_data(&sleeps, pid);

            addToQueue(wake_pcb);

            k_log_event(9, wake_pcb, -1);
            wake_pcb->status = 0;
        }
        else
        {
            cur_node = cur_node->next;
        }
    }
}

void scheduler()
{
    // Want to avoid race conditions, wait until SIGALRM is received to schedule next process

    wakeUp();

    thread++;

    if (thread == 19)
    {
        thread = 0;
    }

    struct PCB *temp = active_PCB;

    int found_thread = 0;

    for (int i = 0; i < 6; i++)
    {
        if ((order[thread] == 1) && (threads_pos != NULL))
        {
            active_PCB = get_first(&threads_pos);
            found_thread = 1;
            break;
        }
        else if (order[thread] == 0 && (threads_zero != NULL))
        {
            active_PCB = get_first(&threads_zero);
            found_thread = 1;
            break;
        }
        else if (order[thread] == -1 && (threads_neg != NULL))
        {
            active_PCB = get_first(&threads_neg);
            found_thread = 1;
            break;
        }
        else
        {
            thread++;
            if (thread == 19)
            {
                thread = 0;
            }
        }
    }

    if (found_thread == 0)
    {
        scheduleIdle();
    }

    activeContext = &active_PCB->context;

    if (temp != active_PCB)
    {
        k_log_event(0, active_PCB, -1);
    }

    setcontext(activeContext);
    exit(EXIT_FAILURE);
}

void clearWaits(int parent_pid)
{
    l_delete_data(&waits, parent_pid);
}

int waitHandler(int pid, int *status)
{
    sigprocmask(SIG_BLOCK, &blocked_signals, &active_PCB->context.uc_sigmask);
    struct PCB *temp = active_PCB;
    l_append_2(&waits, temp->thread_pid, pid);
    append(&threads_blocked, temp);
    temp->status = 2;
    k_log_event(8, temp, -1);
    active_PCB = NULL;
    activeContext = &idleContext;
    swapcontext(&temp->context, activeContext);
    temp->num_children--;
    sigprocmask(SIG_UNBLOCK, &blocked_signals, &active_PCB->context.uc_sigmask);
    return search_status(&temp->child_statuses, pid, status);
}

void setStack(stack_t *stack)
{
    void *sp = malloc(stack_size);
    VALGRIND_STACK_REGISTER(sp, sp + stack_size);
    *stack = (stack_t){.ss_sp = sp, .ss_size = stack_size};
}

void makeContext(ucontext_t *ucp, void (*func)(), char *argv[])
{
    getcontext(ucp);
    sigemptyset(&ucp->uc_sigmask);
    setStack(&ucp->uc_stack);

    // When each process is done, wait for SIGALRM
    // Probably will change later
    ucp->uc_link = &idleContext;

    // int argc = 0;

    if (argv == NULL)
    {
        makecontext(ucp, func, 0);
    }
    else
    {
        makecontext(ucp, func, 1, argv);
    }
}

void killOrphans(int parent_pid)
{
    struct PCB *orphans = NULL;
    orphans = remove_threads_parent(&threads_zombied, parent_pid);
    while (orphans != NULL)
    {
        delete_first(&orphans);
    }
    orphans = remove_threads_parent(&threads_neg, parent_pid);
    while (orphans != NULL)
    {
        orphans->status = 4;
        k_process_cleanup(orphans);
        delete_first(&orphans);
    }
    orphans = remove_threads_parent(&threads_zero, parent_pid);
    while (orphans != NULL)
    {
        orphans->status = 4;
        k_process_cleanup(orphans);
        delete_first(&orphans);
    }
    orphans = remove_threads_parent(&threads_pos, parent_pid);
    while (orphans != NULL)
    {
        orphans->status = 4;
        k_process_cleanup(orphans);
        delete_first(&orphans);
    }
    orphans = remove_threads_parent(&threads_blocked, parent_pid);
    while (orphans != NULL)
    {
        orphans->status = 4;
        k_process_cleanup(orphans);
        delete_first(&orphans);
    }
    orphans = remove_threads_parent(&threads_stopped, parent_pid);
    while (orphans != NULL)
    {
        orphans->status = 4;
        k_process_cleanup(orphans);
        delete_first(&orphans);
    }
}

void cleanFinishedProcess(struct PCB *process)
{
    /*
    Statuses:
    0 : running
    1 : stopped
    2 : blocked
    3 : zombied
    4 : orphaned
    add more later as necessary
    */

    sigprocmask(SIG_BLOCK, &blocked_signals, &active_PCB->context.uc_sigmask);
    clearWaits(process->thread_pid);
    killOrphans(process->thread_pid);
    int is_active = 0;
    int list_number = 0;
    if (process == active_PCB)
    {
        is_active = 1;
        active_PCB = NULL;
    }
    else
    {
        remove_threads(process->thread_pid, &list_number);
    }

    if (fg_process == process->thread_pid)
    {
        fg_process = process->parent_pid;
    }

    // check to see if process is currently being waited on

    if (process->status != 4)
    {
        int i = search(&waits, process->parent_pid, process->thread_pid);

        if (i == -1)
        {
            k_log_event(4, process, -1);
            append(&threads_zombied, process);
        }
        else
        {
            k_log_event(4, process, -1);
            struct PCB *temp = remove_thread(&threads_blocked, process->parent_pid);
            temp->status = 0;
            temp->reaped_child = process->thread_pid;
            k_log_event(9, temp, -1);
            k_log_event(6, process, -1);
            delete_first(&process);
            addToQueue(temp);
        }
    }
    else
    {
        k_log_event(5, process, -1);
    }

    // CLEAR STACK + OTHER MEMORY HERE
    /*
    VALGRIND_STACK_DEREGISTER(process->context.uc_stack.ss_sp);
    free(process->context.uc_stack.ss_sp);
    */
    // WANT TO IDLE UNTIL RECEIVE SIGALRM

    if (is_active == 1)
    {
        setcontext(&idleContext);
    }
}

int reapZombies(int pid, int *status)
{
    if (active_PCB->num_children <= 0)
    {
        return -1;
    }
    *status = -1;
    int child = search_status(&active_PCB->child_statuses, pid, status);
    if (*status == 1 || *status == 3)
    {
        delete (&threads_zombied, child);
        active_PCB->num_children--;
    }
    return child;
}

void logoutDone()
{
    setcontext(&finishContext);
}

void cleanAllProcesses()
{
    f_umount();

    VALGRIND_STACK_DEREGISTER(schedulerContext.uc_stack.ss_sp);
    free(schedulerContext.uc_stack.ss_sp);

    VALGRIND_STACK_DEREGISTER(cleanupContext.uc_stack.ss_sp);
    free(cleanupContext.uc_stack.ss_sp);

    VALGRIND_STACK_DEREGISTER(idleContext.uc_stack.ss_sp);
    free(idleContext.uc_stack.ss_sp);

    free_list(threads_neg);
    free_list(threads_zero);
    free_list(threads_pos);
    free_list(threads_blocked);
    free_list(threads_stopped);
    free_list(threads_zombied);
    free_list(active_PCB);

    l_free_list(waits);
    l_free_list(sleeps);
    l_free_list(sleeps_stopped);

    exit(EXIT_FAILURE);
}

void makeSchedulerContext()
{
    makeContext(&cleanupContext, cleanFinishedProcess, NULL);
    makeContext(&idleContext, idle, NULL);
    makeContext(&schedulerContext, scheduler, NULL);

    sigemptyset(&blocked_signals);
    sigaddset(&blocked_signals, SIGINT);
    sigaddset(&blocked_signals, SIGTSTP);
    sigaddset(&blocked_signals, SIGALRM);
    sigprocmask(SIG_BLOCK, &blocked_signals, &schedulerContext.uc_sigmask);
    swapcontext(&finishContext, &schedulerContext);
}

void alarmHandler(int signum)
{ // SIGALARM
    ticks++;

    if (active_PCB == NULL)
    {
        setcontext(&schedulerContext);
    }
    else if (active_PCB->status == 0)
    {
        addToQueue(active_PCB);
    }
    swapcontext(&active_PCB->context, &schedulerContext);
}

void setTimer(void)
{
    struct itimerval it;
    it.it_interval = (struct timeval){.tv_usec = centisecond * 10};
    it.it_value = it.it_interval;
    setitimer(ITIMER_REAL, &it, NULL);
}

struct PCB *getActive()
{
    return active_PCB;
}

void startClock()
{
    ticks = 0;
}

int getTicks()
{
    return ticks;
}

void scheduleSleep(int pid, int numTicks)
{
    sigprocmask(SIG_BLOCK, &blocked_signals, &active_PCB->context.uc_sigmask);
    append(&threads_blocked, active_PCB);
    active_PCB->status = 2;

    struct PCB *temp = active_PCB;
    active_PCB = NULL;
    activeContext = &idleContext;

    k_log_event(8, temp, -1);
    l_append_2(&sleeps, pid, ticks + (numTicks * 10));

    sigprocmask(SIG_UNBLOCK, &blocked_signals, &active_PCB->context.uc_sigmask);

    swapcontext(&temp->context, activeContext);
}

void deleteSleep(int pid)
{
    l_delete_data(&sleeps, pid);
}
