#include <signal.h> // sigaction, sigemptyset, sigfillset, signal
#include <stdlib.h> // malloc, free
#include <stdbool.h>

#include "kernel.h"
#include "scheduler.h"
#include "user_programs.h"
#include "error.h"
#include "stress.h"

// FOR TESTING, REMOVE LATER
#include <stdio.h>
#include <unistd.h>

static struct PCB *shell_pcb;

// Set priority of thread pid to priority
int p_nice(int pid, int priority)
{
    int list_num = -1;
    struct PCB *change_pcb = find_threads(pid, &list_num);

    if (priority != -1 && priority != 0 && priority != 1)
    {
        ERRNO = INVPRIO;
        return -1;
    }

    if (change_pcb == NULL)
    {
        change_pcb = getActive();
    }

    if (change_pcb == NULL || change_pcb->thread_pid != pid)
    {
        ERRNO = INVPID;
        return -1;
    }
    else
    {
        int old_priority = change_pcb->priority_level;
        change_pcb->priority_level = priority;
        k_log_event(7, change_pcb, old_priority);

        return change_pcb->thread_pid;
    }
}

// Set calling process to blocked until ticks of system clock elapse,
// then set thread to running.
void p_sleep(unsigned int ticks)
{
    scheduleSleep(getActive()->thread_pid, ticks);

    return;
}

// return pid of child thread on `ccess, -1 on error
int p_spawn(void (*func)(), char *argv[], int fd0, int fd1)
{
    // Call k_process_create
    int ret_pid = -1;
    struct PCB *new_pcb;

    if (func == p_shell)
    {
        shell_pcb = k_process_create(NULL);
        shell_pcb->command = argv;

        shell_pcb->prog = "shell";
        shell_pcb->status = 0;

        ret_pid = shell_pcb->thread_pid;

        k_log_event(1, shell_pcb, -1);

        addToQueue(shell_pcb);
    }
    else
    {
        new_pcb = k_process_create(getActive());
        new_pcb->command = argv;

        if (func == p_cat)
        {
            new_pcb->prog = "cat";
        }
        else if (func == p1_sleep)
        {
            new_pcb->prog = "sleep";
        }
        else if (func == p_busy)
        {
            new_pcb->prog = "busy";
        }
        else if (func == p_echo)
        {
            new_pcb->prog = "echo";
        }
        else if (func == p_ls)
        {
            new_pcb->prog = "ls";
        }
        else if (func == p_touch)
        {
            new_pcb->prog = "touch";
        }
        else if (func == p_mv)
        {
            new_pcb->prog = "mv";
        }
        else if (func == p_cp)
        {
            new_pcb->prog = "cp";
        }
        else if (func == p_rm)
        {
            new_pcb->prog = "rm";
        }
        else if (func == p_chmod)
        {
            new_pcb->prog = "chmod";
        }
        else if (func == p_ps)
        {
            new_pcb->prog = "ps";
        }
        else if (func == p1_kill)
        {
            new_pcb->prog = "kill";
        }
        else if (func == p_zombify)
        {
            new_pcb->prog = "zombify";
        }
        else if (func == zombie_child)
        {
            new_pcb->prog = "zombie_child";
        }
        else if (func == p_orphanify)
        {
            new_pcb->prog = "orphanify";
        }
        else if (func == p_orphanify)
        {
            new_pcb->prog = "orphanify";
        }
        else if (func == orphan_child)
        {
            new_pcb->prog = "orphan_child";
        }
        else
        {
            new_pcb->prog = "other";
        }

        makeContext(&new_pcb->context, func, argv);
        new_pcb->status = 0;

        ret_pid = new_pcb->thread_pid;
        k_log_event(1, new_pcb, -1);

        addToQueue(new_pcb);
    }

    // HAVE NOT SET FILE DESCRIPTORS, GONNA WAIT TILL UNDERSTAND HOW WE'RE DOING PENNFAT

    return ret_pid;
}

void p_spawn_shell()
{
    shell_pcb = k_process_create(NULL);
}

/*
Use if you want to print child statuses
#include "linked_list.h"
*/

// return pid of child that has changed state on success, or -1 on error
int p_waitpid(int pid, int *wstatus, bool nohang)
{
    /*
    print_list(getActive()->child_statuses);
    */

    int statusInt = -1;
    int *status;

    if (wstatus != NULL)
    {
        *wstatus = -1;
        status = wstatus;
    }
    else
    {
        status = &statusInt;
    }

    int ret_val = reapZombies(pid, status);

    if (ret_val == 0 && !nohang)
    {
        int reaped_child = waitHandler(pid, status);
        return reaped_child;
    }
    else
    {
        if (ret_val == -1)
        {
            ERRNO = WAITERR;
        }
        return ret_val;
    }
}

// return 0 on success, -1 on error
int p_kill(int pid, int sig)
{
    // Send signal sig to thread referenced by pid
    struct PCB *kill_pcb;
    int list_num = -1;
    kill_pcb = find_threads(pid, &list_num);

    if (kill_pcb == NULL)
    {
        ERRNO = INVPID;
        return -1;
    }
    else
    {
        if (k_process_kill(kill_pcb, sig) == -1)
        {
            p_error("error");
            return -1;
        }

        return 0;
    }
}

int p_fg(char *job_id)
{
    return k_fg(job_id);
}

int p_bg(char *job_id)
{
    return k_bg(job_id);
}

void p_add_job(int pid) {
    int buf_size = 100;
    int job_id = k_add_job(pid);

    if(job_id != -1) {
        f_write(STDOUT_FILENO, "[", 2);
        char ps_string[buf_size];
        sprintf(ps_string, "%d", job_id);
        f_write(STDOUT_FILENO, ps_string, strlen(ps_string));
        f_write(STDOUT_FILENO, "] ", 2);

        sprintf(ps_string, "%d", pid);
        f_write(STDOUT_FILENO, ps_string, strlen(ps_string));
        f_write(STDOUT_FILENO, "\n", 2);
    }
    
}

void p_print_ps()
{
    k_print_ps();
}

void p_print_jobs()
{
    k_print_jobs();
}

void p_exit()
{
    struct PCB *active = getActive();
    if (active->is_shell == 0)
    {
        addStatus(active->parent_pid, active->thread_pid, 1);
        k_log_event(3, active, -1);
        k_process_cleanup(getActive());
    }
}

void p_logout()
{
    k_logout();
}

void p_set_terminal_control(int pid)
{
    setTerminalControl(pid);
}
