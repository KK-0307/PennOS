#include <signal.h>   // sigaction, sigemptyset, sigfillset, signal
#include <stdlib.h>   // malloc, free
#include <sys/time.h> // setitimer
#include <ucontext.h> // getcontext, makecontext, setcontext, swapcontext

// FOR WRITING TO LOG
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#include "linked_list.h"
#include "pcb_linked_list.h"
#include "job_linked_list.h"
#include "scheduler.h"
#include "user_functions.h"
#include "user_programs.h"
#include "error.h"
#include "pennfat.h"
#include "kernel.h"

#include "constants.h"

// FOR TESTING, REMOVE LATER
#include <stdio.h>

static int pid_counter = 1;
static int job_counter = 1;
FILE *file;
char *file_name;

int ERRNO = -1;

struct Job *jobs_list;
struct Job *ps_list;

// Uncomment this for actual main

void k_log_event(int event_id, struct PCB *curr_pcb, int optional)
{
    /*
    0 : SCHEDULE
    1 : CREATE
    2 : SIGNALED
    3 : EXITED
    4 : ZOMBIE
    5 : ORPHAN
    6 : WAITED
    7 : NICE
    8 : BLOCKED
    9 : UNBLOCKED
    10 : STOPPED
    11 : CONTINUED
    */
    char *events[] = {
        "SCHEDULE",
        "CREATE",
        "SIGNALED",
        "EXITED",
        "ZOMBIE",
        "ORPHAN",
        "WAITED",
        "NICE",
        "BLOCKED",
        "UNBLOCKED",
        "STOPPED",
        "CONTINUED"};

    file = fopen(file_name, "a");

    int ticks = getTicks();

    if (event_id == 0)
    {
        // SCHEDULE PID QUEUE PROCESS_NAME
        fprintf(file, "[%5d]  %-10s %4d %2d %s\n", ticks, events[event_id], curr_pcb->thread_pid, curr_pcb->priority_level, curr_pcb->prog);
    }
    else if (1 <= event_id && event_id <= 6)
    {
        // EVENT PID NICE_VALUE PROCESS_NAME
        fprintf(file, "[%5d]  %-10s %4d %2d %s\n", ticks, events[event_id], curr_pcb->thread_pid, curr_pcb->priority_level, curr_pcb->prog);
    }
    else if (event_id == 7)
    {
        // NICE PID OLD_NICE_VALUE NEW_NICE_VALUE PROCESS_NAME
        // optional will be the old nice value
        fprintf(file, "[%5d]  %-10s %4d %2d %d %s\n", ticks, events[event_id], curr_pcb->thread_pid, optional, curr_pcb->priority_level, curr_pcb->prog);
    }
    else if (8 <= event_id && event_id <= 9)
    {
        // BLOCKED PID NICE_VALUE PROCESS_NAME
        fprintf(file, "[%5d]  %-10s %4d %2d %s\n", ticks, events[event_id], curr_pcb->thread_pid, curr_pcb->priority_level, curr_pcb->prog);
    }
    else if (10 <= event_id && event_id <= 11)
    {
        // STOPPED PID NICE_VALUE PROCESS_NAME
        fprintf(file, "[%5d]  %-10s %4d %2d %s\n", ticks, events[event_id], curr_pcb->thread_pid, curr_pcb->priority_level, curr_pcb->prog);
    }
    else
    {
        // INVALID EVENT ID
    }

    fclose(file);

    return;
}

struct PCB *k_process_create(struct PCB *parent)
{
    struct PCB *new_pcb;

    if (parent == NULL)
    {
        // If parent is null, we are initializing the shell
        ucontext_t shell_context;
        makeContext(&shell_context, p_shell, NULL);
        new_pcb = create_new_node(shell_context);

        // open_fds : process_fd, fs_fd, mode
        int rows = 16;
        int columns = 2;

        for (int i = 0; i < rows; i++)
        {
            for (int j = 0; j < columns; j++)
            {
                new_pcb->open_fds[i][j] = -1;
            }
        }

        // Initialize stdin and stdout
        // open_fds : process_fd, fs_fd, mode
        new_pcb->open_fds[0][0] = 0;
        new_pcb->open_fds[1][0] = 1;

        new_pcb->thread_pid = pid_counter;
        new_pcb->parent_pid = 0;
        new_pcb->children_pids = NULL;
        new_pcb->priority_level = 0;
        new_pcb->is_shell = 1;
        new_pcb->num_children = 0;

        pid_counter++;
    }
    else
    {
        // Otherwise initializing new process
        new_pcb = create_new_node(parent->context);
        parent->num_children++;

        int rows = 16;
        int columns = 3;

        for (int i = 0; i < rows; i++)
        {
            for (int j = 0; j < columns; j++)
            {
                new_pcb->open_fds[i][j] = -1;
            }
        }

        // Initialize stdin and stdout
        // open_fds : process_fd, fs_fd, mode
        new_pcb->open_fds[0][0] = 0;
        new_pcb->open_fds[1][0] = 1;

        new_pcb->thread_pid = pid_counter;
        new_pcb->parent_pid = parent->thread_pid;
        new_pcb->children_pids = NULL;
        new_pcb->priority_level = 0;
        new_pcb->is_shell = 0;
        new_pcb->num_children = 0;

        pid_counter++;
    }

    struct Job *new_job = create_new_job(new_pcb, new_pcb->thread_pid);
    new_job->stop_background_time = getTicks();

    append_job(&ps_list, new_job);

    return new_pcb;
}

void k_process_cleanup(struct PCB *process)
{
    int fin_job_id = -1;
    int is_head = 0;
    struct Job* fin_job = find_job(jobs_list, process, &fin_job_id, &is_head);
    if(fin_job != NULL) {
        print_finished_job(fin_job, is_head);
    }

    delete_job(&jobs_list, fin_job_id);
    delete_job(&ps_list, process->thread_pid);

    cleanFinishedProcess(process);

    return;
}

int addStatus(int parent_pid, int child_pid, int update_status)
{
    int list_num = -1;
    struct PCB *parent_pcb = find_threads(parent_pid, &list_num);

    if (list_num == -1)
    {
        return -1;
    }
    /*
    First field is child pid, second is the update status code
    Changed Status Code:
    1 : Exited Normally
    2 : Stopped
    3 : Signaled
    */

    l_append_2(&parent_pcb->child_statuses, child_pid, update_status);

    return 0;
}

int k_process_kill(struct PCB *process, int signal)
{
    int list_num = -1;
    find_threads(process->thread_pid, &list_num);

    if (list_num == -1)
    {
        ERRNO = INVPID;
        return -1;
    }

    if (signal == S_SIGSTOP)
    {
        if (list_num == 4 || list_num == 6 || (list_num >= 0 && list_num <= 2))
        {
            // Want to update stop time
            struct Job *temp = jobs_list;
            int found_job = 0;
            while (temp != NULL)
            {
                if (temp->pcb == process)
                {
                    temp->stop_background_time = getTicks();
                    found_job = 1;
                }
                temp = temp->next;
            }

            if(found_job == 0) {
                // Not found, need to add job
                struct Job* new_job = create_new_job(process, job_counter);
                new_job->stop_background_time = getTicks();

                append_job(&jobs_list, new_job);
                job_counter++;
            }

            // In running state, move to stopped list (LIST NUMBERS IN scheduler.c)
            int res = moveThread(process->thread_pid, list_num, 3);
            if (res != 0)
            {
                ERRNO = INVPID;
                return -1;
            }

            addStatus(process->parent_pid, process->thread_pid, 2);

            return process->thread_pid;
        }
        else
        {
            // Not running
            return -1;
        }
    }
    else if (signal == S_SIGCONT)
    {
        if (list_num == 3)
        {
            int res = -1;
            if (strcmp(process->prog, "sleep") == 0)
            {
                res = moveThread(process->thread_pid, list_num, 4);
            }
            else
            {
                if (process->priority_level == -1)
                {
                    res = moveThread(process->thread_pid, list_num, 0);
                }
                else if (process->priority_level == 0)
                {
                    res = moveThread(process->thread_pid, list_num, 1);
                }
                else if (process->priority_level == 1)
                {
                    res = moveThread(process->thread_pid, list_num, 2);
                }
                else
                {
                    ERRNO = INVPRIO;
                    return -1;
                }
                
                process->status = 0;
            }

            return res;
        }
        else
        {
            ERRNO = INVPID;
            return -1;
        }
    }
    else if (signal == S_SIGTERM)
    {
        k_log_event(2, process, -1);

        if (strcmp(process->prog, "sleep") == 0)
        {
            int pid = process->thread_pid;
            deleteSleep(pid);
            k_process_cleanup(process);
            return pid;
        }
        else
        {
            int pid = process->thread_pid;
            k_process_cleanup(process);
            return pid;
        }
    }
    else
    {
        ERRNO = INVSIG;
        return -1;
    }

    return 0;
}

void k_print_ps()
{
    print_ps(ps_list);
}

void k_print_jobs()
{
    print_jobs(jobs_list, 1);
}

int k_fg(char *job_id)
{
    int foreground_id = getFgProcessId();

    if (job_id != NULL)
    {
        struct Job *temp = jobs_list;

        while (temp != NULL)
        {
            if (temp->job_id == atoi(job_id) && temp->pcb->thread_pid != foreground_id)
            {
                if(temp->pcb->status == 1) {
                    k_process_kill(temp->pcb, S_SIGCONT);
                }
                setTerminalControl(temp->pcb->thread_pid);
                return temp->pcb->thread_pid;
            }

            temp = temp->next;
        }

        ERRNO = INVCOM;
        return -1;
    }
    else
    {
        struct Job *temp = jobs_list;
        struct Job *most_recent = NULL;

        while (temp != NULL)
        {
            if (temp->pcb->thread_pid != foreground_id)
            {
                if (most_recent == NULL)
                {
                    most_recent = temp;
                }
                else
                {
                    if (temp->stop_background_time > most_recent->stop_background_time)
                    {
                        most_recent = temp;
                    }
                }
            }

            temp = temp->next;
        }

        if (most_recent != NULL)
        {
            if(most_recent->pcb->status  == 1) {
                k_process_kill(most_recent->pcb, S_SIGCONT);
            }
            setTerminalControl(most_recent->pcb->thread_pid);
            return most_recent->pcb->thread_pid;
        }
    }

    ERRNO = INVCOM;
    return -1;
}

int k_bg(char *job_id)
{
    int foreground_id = getFgProcessId();

    if (job_id != NULL)
    {
        struct Job *temp = jobs_list;

        while (temp != NULL)
        {
            if (temp->job_id == atoi(job_id) && temp->pcb->thread_pid != foreground_id && temp->pcb->status != 0 && temp->pcb->status != 2)
            {
                k_process_kill(temp->pcb, S_SIGCONT);
                return temp->pcb->thread_pid;
            }

            temp = temp->next;
        }

        ERRNO = INVCOM;
        return -1;
    }
    else
    {
        struct Job *temp = jobs_list;
        struct Job *most_recent = NULL;

        while (temp != NULL)
        {
            if (temp->pcb->thread_pid != foreground_id && temp->pcb->status != 0 && temp->pcb->status != 2)
            {
                if (most_recent == NULL)
                {
                    most_recent = temp;
                }
                else
                {
                    if (temp->stop_background_time > most_recent->stop_background_time)
                    {
                        most_recent = temp;
                    }
                }
            }

            temp = temp->next;
        }

        if (most_recent != NULL)
        {
            // Continue the most recent stopped/bg job
            k_process_kill(most_recent->pcb, S_SIGCONT);
            return most_recent->pcb->thread_pid;
        }
    }
    return -1;
}

int k_add_job(int pid) {
    struct Job *temp = ps_list;
    while (temp != NULL) {
        if(temp->job_id == pid) {
            struct Job* new_job = create_new_job(temp->pcb, job_counter);
            new_job->stop_background_time = getTicks();

            append_job(&jobs_list, new_job);
            job_counter++;
            return job_counter - 1;
        }
        temp = temp->next;
    }
    return -1;
}

void k_logout()
{
    logoutDone();
}

void sighandler(int signum)
{
    struct PCB *fg_process = getFgProcess();

    if (fg_process == NULL)
    {
        printf("NULL FG PROCESS\n");
    }

    if (signum == SIGINT)
    {
        if (fg_process->is_shell == 1)
        {
            f_write(STDOUT_FILENO, "\n$ ", 4);
        }
        else
        {
            k_process_kill(fg_process, S_SIGTERM);
        }
    }
    if (signum == SIGTSTP)
    {
        if (fg_process->is_shell == 1)
        {
            f_write(STDOUT_FILENO, "\n$ ", 4);
        }
        else
        {
            k_process_kill(fg_process, S_SIGSTOP);
        }
    }
}

void install_handlers()
{
    struct sigaction setup_action;
    struct sigaction alarm_action;
    sigset_t block_mask;
    sigemptyset(&block_mask);
    sigaddset(&block_mask, SIGINT);
    sigaddset(&block_mask, SIGTSTP);
    sigaddset(&block_mask, SIGALRM);
    setup_action.sa_handler = sighandler;
    setup_action.sa_mask = block_mask;
    setup_action.sa_flags = 0;
    alarm_action.sa_handler = alarmHandler;
    alarm_action.sa_mask = block_mask;
    alarm_action.sa_flags = 0;
    sigaction(SIGTSTP, &setup_action, NULL);
    sigaction(SIGINT, &setup_action, NULL);
    sigaction(SIGALRM, &alarm_action, NULL);
}

int main(int argc, char *argv[])
{
    if (argc == 1)
    {
        f_mkfs("fs", 32, 4);
        f_mount("fs");
    }
    else
    {
        char *file_system_name = argv[1];
        f_mount(file_system_name);
    }

    if (argc <= 2)
    {
        file_name = "log.txt";
        file = fopen(file_name, "w"); // this isn't using methods in syscalls.c
    }
    else
    {
        file_name = argv[2];
        file = fopen(argv[2], "w");
    }
    // Start clock
    startClock();

    // Empty log file

    fclose(file);

    // Make Shell context, unfinished
    int pid_shell = p_spawn(p_shell, NULL, 0, 1);
    p_nice(pid_shell, -1);

    // Set alarm for process
    setTimer();

    struct sigaction setup_action;
    struct sigaction alarm_action;
    sigset_t block_mask;
    sigemptyset(&block_mask);
    sigaddset(&block_mask, SIGINT);
    sigaddset(&block_mask, SIGTSTP);
    sigaddset(&block_mask, SIGALRM);
    setup_action.sa_handler = sighandler;
    setup_action.sa_mask = block_mask;
    setup_action.sa_flags = SA_RESTART;
    alarm_action.sa_handler = alarmHandler;
    alarm_action.sa_mask = block_mask;
    alarm_action.sa_flags = SA_RESTART;
    sigaction(SIGTSTP, &setup_action, NULL);
    sigaction(SIGINT, &setup_action, NULL);
    sigaction(SIGALRM, &alarm_action, NULL);

    // Make scheduler context and set context to the scheduler
    makeSchedulerContext();

    j_free_list(ps_list);
    cleanAllProcesses();
}