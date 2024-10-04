#include <signal.h> // sigaction, sigemptyset, sigfillset, signal
#include <stdlib.h> // malloc, free
#include <stdbool.h>
#include <ctype.h>

#include "parser.h"
#include "syscalls.h"
#include "user_programs.h"
#include "user_functions.h"
#include "error.h"
#include "constants.h"
#include "stress.h"

#include <string.h>
#include <unistd.h>
#include <stdbool.h>

#define MAX_LINE_LENGTH 4096

// FOR TESTING, REMOVE LATER
#include <stdio.h>
struct parsed_command *cmd;

int is_number(char *str)
{
    char *end;
    strtol(str, &end, 10);

    return (*end == '\0');
}

// Check if is built-in subroutine
int cmd_handler_subroutine(char *command[], int fd0, int fd1)
{
    int res = -1;
    if (strcmp(command[0], "nice_pid") == 0)
    {
        if (command[1] != NULL &&
            command[2] != NULL &&
            isdigit(*command[2]) &&
            ((strcmp(command[1], "-1") == 0) || (strcmp(command[1], "0") == 0) || (strcmp(command[1], "1") == 0)))
        {
            // Valid command
            p_nice(atoi(command[2]), atoi(command[1]));
        }
        else
        {
            ERRNO = INVPRIO;
            p_error("error");
        }
        res = 0;
    }
    else if (strcmp(command[0], "man") == 0)
    {
        char *manual = "\
        cat : reads from STDIN and prints to STDOUT\n\
        cat FILE...: prints content of listed files\n\
        cat > FILE: reads from STDIN and overwrites FILE\n\
        cat >> FILE: reads from STDIN and appends to FILE\n\
        cat FILE1 FILE2...> FILE3: concatenates contents of listed files and overwrites FILE3\n\
        cat FILE1 FILE2...>> FILE3: concatenates contents of listed files and appends to FILE3\n\
        cat < FILE1: prints content of FILE1\n\n\
        sleep: Sleep for specified seconds\n\
        args: Number of seconds to sleep for n\n\n\
        busy: Busy wait indefinitely\n\
        args: None\n\n\
        echo: Echo the string to standard output\n\
        args: String to be echoed\n\n\
        ls: List all files in working directory\n\
        args: None\n\n\
        touch: Create an empty file if it does not exist, otherwise update timestamp\n\
        args: Filename\n\n\
        mv: Rename src to dst\n\
        args: src, dst\n\n\
        cp: Copy src to dest\n\
        args: src, dest\n\n\
        rm: Remove specified files\n\
        args: FILES...\n\n\
        chmod: Change file mode bits\n\
        args: (filename: name of file you want to change permissions), (new_perm: new permissions) \n\n\
        ps: List all processes on PennOS\n\
        args: None\n\n\
        kill: Send specified signal to process with process id pid\n\
        args: (-term: terminate process | -stop: stop process | -cont: continue process) pids...\n\n\
        nice: Set priority of command to priority and execute the command\n\
        args: priority {0, 1, -1}, command, args for command... \n\n\
        nice_pid: change priority of process with process id pid to priority priority\n\
        args: priority, pid\n\n\
        man: List all available commands\n\
        args: None\n\n\
        bg: Continue last stopped job or job specified by job id\n\
        args: job id\n\n\
        fg: Bring last stopped or backgrounded job to foreground, or job specified by job id\n\
        args: job id\n\n\
        jobs: List all jobs\n\
        args: None\n\n\
        logout: Exit the shell and shut down PennOS\n\
        args: None\n\n\
        hang: Spawn 10 children and block-wait for any of them in a loop until all are reaped (non-deterministic)\n\
        args: None\n\n\
        nohang: Spawn 10 children and nonblocking wait for any of them in a loop until all are reaped (non-deterministic)\n\
        args: None\n\n\
        recur: Recursively spawn self 26 times and names spawned processes Gen_A through Gen_Z. Each process is block-waited and reaped by parent\n\
        args: None\n";
        f_write(STDOUT_FILENO, manual, strlen(manual));
        res = 0;
    }
    else if (strcmp(command[0], "bg") == 0)
    {
        if (p_bg(command[1]) == -1)
        {
            p_error("error");
        }
        res = 0;
    }
    else if (strcmp(command[0], "fg") == 0)
    {
        int foreground_pid = p_fg(command[1]);
        int wstatus;

        printf("FOREGROUND PID IS %d\n", foreground_pid);

        if (foreground_pid == -1)
        {
            p_error("error");
        }
        else
        {
            p_waitpid(foreground_pid, &wstatus, false);
        }
        res = 0;
    }
    else if (strcmp(command[0], "jobs") == 0)
    {
        p_print_jobs();
        res = 0;
    }
    else if (strcmp(command[0], "logout") == 0)
    {
        sigset_t mask;
        sigfillset(&mask);
        sigprocmask(SIG_SETMASK, &mask, NULL);

        p_logout();
        res = 0;
    }
    else if (strcmp(command[0], "hang") == 0)
    {
        hang();
        res = 0;
    }
    else if (strcmp(command[0], "nohang") == 0)
    {
        nohang();
        printf("RETURNED\n");
        res = 0;
    }
    else if (strcmp(command[0], "recur") == 0)
    {
        recur();
        res = 0;
    }
    else if (strcmp(command[0], "script") == 0)
    {
        p_script(command);
        res = 0;
    }

    return res;
}
// Shell built-ins: cat, sleep, busy, echo...
int cmd_handler(char *command[], int fd0, int fd1)
{
    int res = -1;
    if (strcmp(command[0], "nice") == 0)
    {
        if (command[1] != NULL && is_number(command[1]) && command[2] != NULL)
        {
            res = cmd_handler(&command[2], fd0, fd1);
            if (p_nice(res, atoi(command[1])) == -1)
            {
                p_error("error");
            }
        }
        else
        {
            ERRNO = INVCOM;
        }
    }
    else if (strcmp(command[0], "cat") == 0)
    {
        res = p_spawn(p_cat, command, fd0, fd1);
    }
    else if (strcmp(command[0], "sleep") == 0)
    {
        if (command[1] == NULL || !is_number(command[1]))
        {
            // Invalid sleep
            ERRNO = SLPSEC;
        }
        else
        {
            res = p_spawn(p1_sleep, command, fd0, fd1);
        }
    }
    else if (strcmp(command[0], "busy") == 0)
    {
        res = p_spawn(p_busy, command, fd0, fd1);
    }
    else if (strcmp(command[0], "echo") == 0)
    {
        res = p_spawn(p_echo, command, fd0, fd1);
    }
    else if (strcmp(command[0], "ls") == 0)
    {
        res = p_spawn(p_ls, command, fd0, fd1);
    }
    else if (strcmp(command[0], "touch") == 0)
    {
        res = p_spawn(p_touch, command, fd0, fd1);
    }
    else if (strcmp(command[0], "mv") == 0)
    {
        res = p_spawn(p_mv, command, fd0, fd1);
    }
    else if (strcmp(command[0], "cp") == 0)
    {
        res = p_spawn(p_cp, command, fd0, fd1);
    }
    else if (strcmp(command[0], "rm") == 0)
    {
        res = p_spawn(p_rm, command, fd0, fd1);
    }
    else if (strcmp(command[0], "chmod") == 0)
    {
        res = p_spawn(p_chmod, command, fd0, fd1);
    }
    else if (strcmp(command[0], "ps") == 0)
    {
        res = p_spawn(p_ps, command, fd0, fd1);
    }
    else if (strcmp(command[0], "zombify") == 0)
    {
        res = p_spawn(p_zombify, command, fd0, fd1);
    }
    else if (strcmp(command[0], "orphanify") == 0)
    {
        res = p_spawn(p_orphanify, command, fd0, fd1);
    }
    else if (strcmp(command[0], "kill") == 0)
    {
        if (command[1] == NULL || command[2] == NULL || !is_number(command[2]))
        {
            ERRNO = INVCOM;
        }
        else
        {
            res = p_spawn(p1_kill, command, fd0, fd1);
        }
    }
    else
    {
        ERRNO = INVCOM;
    }

    return res;
}

void p_shell()
{
    // struct parsed_command *cmd;
    char buf[MAX_LINE_LENGTH];
    int wstatus;
    while (1)
    {
        f_write(STDOUT_FILENO, "$ ", 3);

        f_read(STDIN_FILENO, MAX_LINE_LENGTH, buf);

        int buf_len = strlen(buf);

        if (buf_len == 0)
        {
            free(cmd);
            char *logout_string = "logout";
            cmd_handler_subroutine(&logout_string, 0, 1);
        }

        int j = -1;

        while (j > 0)
        {
            j = p_waitpid(-1, &wstatus, true);
        }

        int i = -1;

        i = parse_command(buf, &cmd);

        if (i != 0)
        {
            ERRNO = i;
            p_error("error");
        }
        else if (cmd->commands[0] == NULL)
        {
            // No command, do nothing
        }
        else
        {
            // call p_spawn
            int fd0 = STDIN_FILENO;
            if (cmd->stdin_file != NULL)
            {
                fd0 = f_open(cmd->stdin_file, F_READ);
            }

            int fd1 = STDOUT_FILENO;
            if (cmd->stdout_file != NULL)
            {
                if (cmd->is_file_append)
                {
                    fd1 = f_open(cmd->stdout_file, F_APPEND);
                }
                else
                {
                    fd1 = f_open(cmd->stdout_file, F_WRITE);
                }
            }

            if (strcmp(*cmd->commands[0], "logout") == 0)
            {
                free(cmd);
                char *logout_string = "logout";
                cmd_handler_subroutine(&logout_string, fd0, fd1);
            }
            else if (cmd_handler_subroutine(cmd->commands[0], fd0, fd1) == -1)
            {
                int pid = cmd_handler(cmd->commands[0], fd0, fd1);
                if(cmd->is_background) {
                    p_add_job(pid);
                }
                if (pid == -1)
                {
                    p_error("error");
                }
                else if (!cmd->is_background)
                {
                    p_set_terminal_control(pid);
                    // wait on the child
                    if (p_waitpid(pid, &wstatus, false) == -1)
                    {
                        printf("WAIT ERROR\n");
                        p_error("error");
                    }
                }
            }

            // free(cmd);
        }

        for (int i = 0; buf[i] != '\0'; i++)
        {
            buf[i] = '\0';
        }
    }

    p_exit();
    return;
}

void p_shell_script(char* buf) { //command to execute
    
    int wstatus;

     int j = -1;

    while (j != 0)
    {
        j = p_waitpid(-1, &wstatus, true);
    }

    int i = -1;
    i = parse_command(buf, &cmd);

    if (i != 0)
    {
        ERRNO = i;
        p_error("error");
    }
    else if (cmd->commands[0] == NULL)
    {
        // No command, do nothing
    }
    else
    {
        // call p_spawn
        
        int fd0 = STDIN_FILENO;
        if (cmd->stdin_file != NULL)
        {
            fd0 = f_open(cmd->stdin_file, F_READ);
        }

        int fd1 = STDOUT_FILENO;
        if (cmd->stdout_file != NULL)
        {
            if(cmd->is_file_append) {
                fd1 = f_open(cmd->stdout_file, F_APPEND);
            } else {
                fd1 = f_open(cmd->stdout_file, F_WRITE);
            }
        }
        

        if (strcmp(*cmd->commands[0], "logout") == 0)
        {
            free(cmd);
            char *logout_string = "logout";
            cmd_handler_subroutine(&logout_string, fd0, fd1);
        }
        else if (cmd_handler_subroutine(cmd->commands[0], fd0, fd1) == -1)
        {
                
            int pid = cmd_handler(cmd->commands[0], fd0, fd1);
            if (pid == -1)
            {
                p_error("error");
            }
            else if (!cmd->is_background)
            {
                // p_set_terminal_control(pid);
                //  wait on the child
                if (p_waitpid(pid, &wstatus, false) == -1)
                {
                    printf("WAIT ERROR\n");
                    p_error("error");
                }
            }
        }

            // free(cmd);
    }

    for (int i = 0; buf[i] != '\0'; i++)
    {   
        buf[i] = '\0';
    }
        
    return;
}

void p_idle()
{
    char buf[MAX_LINE_LENGTH];
    while (1)
    {
        f_read(STDIN_FILENO, MAX_LINE_LENGTH, buf);
        f_write(STDOUT_FILENO, buf, strlen(buf) + 1);
    }

    p_exit();
}

void p_cat(char *argv[]) // test
{

    char *stdout_file = (char *)cmd->stdout_file;
    char *stdin_file = (char *)cmd->stdin_file;
    bool is_file_append = cmd->is_file_append;
    if (stdout_file == NULL && stdin_file == NULL)
    { // no redirection, print out contents of file
        if (argv[1] == NULL)
        { // just cat, echo
            char buf[MAX_LINE_LENGTH];
            f_read(STDIN_FILENO, MAX_LINE_LENGTH, buf);
            f_write(STDOUT_FILENO, buf, strlen(buf));
            f_write(STDOUT_FILENO, " ", 1);
        }
        else
        { // cat file1 file2
            int i = 1;
            while (argv[i] != NULL)
            {
                char *input_file_name = argv[i];
                int fd_input = f_open(input_file_name, F_READ);
                int size = f_get_size(fd_input);
                char buffer[size];
                f_read(fd_input, size, buffer);       // 10 for now
                f_write(STDOUT_FILENO, buffer, size); // 10 for now
                f_write(STDOUT_FILENO, " ", 1);
                i++;
                f_close(fd_input);
            }
        }
    }
    else if (stdout_file != NULL & argv[1] == NULL)
    { // output redirection with no files
        int fd = -1;
        if (is_file_append)
        { // cat >> filename, waits for STDIN and appends to filename
            // printf(">> expected\n");
            fd = f_open(stdout_file, F_APPEND);
        }
        else
        { // cat > filename, waits for STDIN and overwrites to filename
            // printf("> expected\n");
            fd = f_open(stdout_file, F_WRITE);
        }
        char buf[MAX_LINE_LENGTH];
        f_read(STDIN_FILENO, MAX_LINE_LENGTH, buf);
        f_write(fd, buf, strlen(buf));
        f_close(fd);
    }
    else if (stdout_file != NULL & argv[1] != NULL)
    { // output redirection with files
        int i = 1;
        // get sizes
        int sum_of_size = 0;
        while (argv[i] != NULL)
        {
            int fd_file = f_open(argv[i], F_READ);
            int size = f_get_size(fd_file);
            sum_of_size += size;
            f_close(fd_file);
            i++;
        }
        i = 1;
        char *cat_string = (char *)malloc(sum_of_size); // resulting string
        while (argv[i] != NULL)
        {
            // open file to read
            int fd_file = f_open(argv[i], F_READ);
            int size = f_get_size(fd_file);
            char buf[size];
            f_read(fd_file, size, buf);
            strcpy(cat_string, buf);
            cat_string += size;
            f_close(fd_file);
            i++;
        }
        cat_string -= sum_of_size;
        int fd = -1;
        if (is_file_append)
        { // cat file1 >> file2
            fd = f_open(stdout_file, F_APPEND);
        }
        else
        { // cat file1 > file2
            fd = f_open(stdout_file, F_WRITE);
        }
        f_write(fd, cat_string, sum_of_size);
        f_close(fd);
    }
    else if (stdin_file != NULL)
    { // basic input redirection cat < file
        int fd_input = f_open(stdin_file, F_READ);
        int size = f_get_size(fd_input);
        char buffer[size];
        f_read(fd_input, size, buffer);
        f_write(STDOUT_FILENO, buffer, size);
        f_write(STDOUT_FILENO, " ", 1);
        f_close(fd_input);
    }

    p_exit();
}

void p1_sleep(char *argv[])
{
    p_sleep(atoi(argv[1]));
    p_exit();
}

void p_busy()
{
    while (1)
    {
        // Do nothing
    }

    // Should never return
    p_exit();
}

void p_echo(char *argv[]) // test
{
    char *stdout_file = (char *)cmd->stdout_file;
    bool is_file_append = cmd->is_file_append;
    if (stdout_file != NULL)
    { // echo hello > FILE or echo hello >> FILE
        int fd = -1;
        if (is_file_append)
        { // >>
            fd = f_open(stdout_file, F_APPEND);
        }
        else
        { //>
            fd = f_open(stdout_file, F_WRITE);
        }
        int i = 1;
        while (argv[i] != NULL)
        {
            f_write(fd, argv[i], strlen(argv[i]));
            i++;
            f_write(fd, " ", 1);
        }
        f_write(fd, "\n", 1);
        f_close(fd);
    }
    else
    { // echo
        int i = 1;
        while (argv[i] != NULL)
        {
            f_write(STDOUT_FILENO, argv[i], strlen(argv[i]));
            f_write(STDOUT_FILENO, " ", 1);
            i++;
        }
        f_write(STDOUT_FILENO, "\n", 1);
    }
    p_exit();
}

void p_script(char* argv[]) {
    //if no redirection
    if (cmd->stdout_file == NULL) {
        int fd_script_1 = f_open("script", F_READ); 
        int fd_script_temp_1 = f_open("script", F_READ);
        int num_commands_1 = num_commands_script(fd_script_temp_1);
        int j = 0;
        int script_size_1 = f_get_size(fd_script_1);
        char* line_1 = malloc(script_size_1);

        int fd_script_2 = f_open("script", F_READ);
        f_read(fd_script_2, script_size_1, line_1);

        char* token_1 = strtok(line_1, "\n");
        while(j < num_commands_1) {
            if (j != 0){
                token_1 = strtok(NULL, "\n");
            }
            p_shell_script(token_1); 
            j++;
        }
        f_close(fd_script_1);
        f_close(fd_script_temp_1);
        f_close(fd_script_2);
        return;
    }

    char* stdout_file = (char*) cmd->stdout_file;
    bool is_file_append = cmd->is_file_append;
    char* to_add;
    if (is_file_append) { //>>
        char add[] = " >> ";
        to_add = malloc(4 + strlen(stdout_file));
        strcpy(to_add, add);
        strcat(to_add, stdout_file);
    }
    else { //>
        char add[] = " > ";
        to_add = malloc(3 + strlen(stdout_file));
        strcpy(to_add, add);
        strcat(to_add, stdout_file);
    }
    //printf("to_add: %s\n", to_add);
    
    //execute script
    //for every new line/new command
    int fd_script = f_open("script", F_READ); //should be script

    int num_commands = num_commands_script(fd_script);
    int i = 0;
    int fd_script_temp = f_open("script", F_READ); 
    int script_size = f_get_size(fd_script_temp);
    char* line = malloc(script_size);
    f_read(fd_script_temp, script_size, line);
    char* token = strtok(line, "\n"); //get each line in file
    bool redirect = false;

    while(i < num_commands) {
        int n = strlen(token); //size of command, temp 10
        if (redirect) { //if redirect
            printf("redirect\n");
            char* buf = (char*) malloc(n);
            f_read(fd_script, n, buf); 
            f_lseek(fd_script, n+2, 1); //F_SEEK_SET
            strcpy(buf, token);
            p_shell_script(buf); 
        } else {
            if (i != 0) {
                token = strtok(NULL, "\n");
            }
            struct parsed_command *cmd_2;
            parse_command(token, &cmd_2);  //parse the command in file
            //if token already contains redirection then don't to_add is ""
            //or if token is something other than echo or cat
            if ((strcmp(*cmd_2->commands[0], "echo") != 0 && strcmp(*cmd_2->commands[0], "cat") != 0) || cmd_2->stdout_file != NULL) {
                char* buf = (char*) malloc(n);
                f_read(fd_script, n, buf); 
                f_lseek(fd_script, n+2, 1); //F_SEEK_SET
                strcpy(buf, token);
                p_shell_script(buf); 
            } else { //if echo or cat
                if (i != 0) { //change to append
                    char add[] = " >> ";
                    to_add = malloc(4 + strlen(stdout_file));
                    strcpy(to_add, add);
                    strcat(to_add, stdout_file);
                }
                char* buf = (char*) malloc(n + strlen(to_add));
                strcpy(buf, token);
                strcat(buf, to_add);
                p_shell_script(buf); 
            }
        }
        
        i++;
    }
    f_close(fd_script);
    f_close(fd_script_temp);
    return;
}

//counts number of commands in a script
int num_commands_script(int fd) {
    int count = 0;
    int size = f_get_size(fd);
    char* buf = malloc(size);
    f_read(fd, size, buf);
    char* token = strtok(buf, "\n");
    //printf("token in num_commands: %s\n", token);
    while(token != NULL) {
        count++;
        token = strtok(NULL, "\n");
    }
    if (count == 1) {
        return 1;
    }
    return count-1;
}

bool contains_redirect(char* line) {
    char* token = strtok(line, "\n");
    while(token != NULL) {
        if (strcmp(token, ">") == 0 || strcmp(token, ">>") == 0) {
            return true;
        }
        token = strtok(NULL, "\n");
    }
    return false;
}


void p_ls(char *argv[])
{
    f_ls(NULL);
    p_exit();
}

void p_touch(char *argv[])
{   
    int i = 1;
    while (argv[i] != NULL)
    {
        int fd = f_open(argv[i], 0);
        f_close(fd);
        i++;
    }
    p_exit();
}

void p_mv(char *argv[])
{
    char *src = argv[1];
    char *dest = argv[2];
    f_mv(src, dest);
    p_exit();
}

void p_cp(char *argv[])
{
    char* src = argv[1];
    char* dest = argv[2];
    
    int fd_src = f_open(src, F_READ);
    int size = f_get_size(fd_src);
    char* buf = malloc(size);
    f_read(fd_src, size, buf);
    int fd_dest = f_open(dest, F_WRITE);
    f_write(fd_dest, buf, strlen(buf));
    f_close(fd_src);
    f_close(fd_dest);

    //f_cp(src, dest);


    p_exit();
}

void p_rm(char *argv[])
{
    int i = 1;
    while (argv[i] != NULL)
    {
        char* filename = argv[i];
        f_unlink(filename);
        i++;
    } 
    p_exit();
}

void p_chmod(char *argv[])
{
    uint8_t new_perm = atoi(argv[1]);
    char *filename = argv[2];
    f_chmod(filename, new_perm);
    p_exit();
}

void p_ps()
{
    p_print_ps();
    p_exit();
}

void p1_kill(char *argv[])
{
    int i = 0;
    if (strcmp(argv[1], "-term") == 0)
    {
        while (argv[i] != NULL)
        {
            p_kill(atoi(argv[i]), S_SIGTERM);
            i++;
        }
    }
    else if (strcmp(argv[1], "-stop") == 0)
    {
        while (argv[i] != NULL)
        {
            p_kill(atoi(argv[i]), S_SIGSTOP);
            i++;
        }
    }
    else if (strcmp(argv[1], "-cont") == 0)
    {
        while (argv[i] != NULL)
        {
            p_kill(atoi(argv[i]), S_SIGCONT);
            i++;
        }
    }

    p_exit();
}

void zombie_child()
{
    // MMMMM Brains...!
    return;
}

void p_zombify()
{
    // MMMMM Brains...!
    p_spawn(zombie_child, NULL, 0, 1);
    while (1)
        ;
    p_exit();
}

void orphan_child()
{
    // Please sir,
    // I want some more
    while (1)
        ;
}

void p_orphanify()
{
    // Please sir,
    // I want some more
    p_spawn(orphan_child, NULL, 0, 1);
    p_exit();
}
