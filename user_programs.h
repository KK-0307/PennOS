#ifndef USER_PROGRAMS_H
#define USER_PROGRAMS_H

#include <signal.h> 	// sigaction, sigemptyset, sigfillset, signal
#include <stdlib.h> 	// malloc, free
#include <stdbool.h>

void p_shell();

void p_idle();

void p_cat();

void p1_sleep();

void p_busy();

void p_echo();

void p_script();

void p_ls();

void p_touch();

void p_mv();

void p_cp();

void p_rm();

void p_chmod();

void p_ps();

void p1_kill();

void zombie_child();

void p_zombify();

void orphan_child();

void p_orphanify();

int num_commands_script(int fd);

bool contains_redirect(char* line);

#endif // USER_PROGRAMS_H