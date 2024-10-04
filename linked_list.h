#ifndef LINKED_LIST_H
#define LINKED_LIST_H

#include <signal.h>   // sigaction, sigemptyset, sigfillset, signal
#include <stdlib.h>   // malloc, free
#include <sys/time.h> // setitimer
#include <ucontext.h> // getcontext, makecontext, setcontext, swapcontext

struct Node
{
    int data;
    int data2;
    struct Node *next;
};

struct Node *l_create_new_node(int data);

struct Node *l_create_new_node_2(int data, int data2);

void l_insert(struct Node **head, int data);

int search(struct Node **head, int data, int data2);

int search_status(struct Node **head, int data, int *status);

void l_append(struct Node **head, int data);

void l_append_2(struct Node **head, int data, int data2);

void l_delete_data(struct Node **head, int data);

void l_delete_data_all(struct Node **head, int data);

void l_free_list(struct Node *head);

void print_list(struct Node *head);

struct Node *remove_data(struct Node **head, int thread_pid);

#endif // LINKED_LIST_H
