#include <signal.h>   // sigaction, sigemptyset, sigfillset, signal
#include <stdlib.h>   // malloc, free
#include <sys/time.h> // setitimer
#include <ucontext.h> // getcontext, makecontext, setcontext, swapcontext

// FOR TESTING, REMOVE LATER
#include <stdio.h>
#include <unistd.h>
#include <string.h>

struct Node
{
    // Primary data field
    int data;

    // Secondary data field
    int data2;

    // Pointer to next Node in the linked list
    struct Node *next;
};

struct Node *l_create_new_node(int data)
{
    struct Node *new_node = (struct Node *)malloc(sizeof(struct Node));
    if (new_node == NULL)
    {
        // EXIT
    }

    new_node->data = data;
    new_node->data2 = 0;
    new_node->next = NULL;
    return new_node;
}

struct Node *l_create_new_node_2(int data, int data2)
{
    struct Node *new_node = (struct Node *)malloc(sizeof(struct Node));
    if (new_node == NULL)
    {
        // EXIT
    }

    new_node->data = data;
    new_node->data2 = data2;
    new_node->next = NULL;
    return new_node;
}

void l_insert(struct Node **head, int data)
{
    struct Node *new_node = l_create_new_node(data);

    new_node->next = *head;
    *head = new_node;
}

int search(struct Node **head, int data, int data2)
{
    struct Node *temp = *head;
    struct Node *prev = NULL;

    if (temp == NULL)
    {
        return -1;
    }

    if (temp->data == data && (temp->data2 == data2 || temp->data2 == -1))
    {
        *head = temp->next;
        free(temp);
        return data2;
    }

    while (temp != NULL && (temp->data != data || (temp->data2 != data2 && temp->data2 != -1)))
    {
        prev = temp;
        temp = temp->next;
    }

    if (temp == NULL)
    {
        return -1;
    }

    prev->next = temp->next;
    free(temp);
    return data2;
}

int search_status(struct Node **head, int data, int *status)
{
    struct Node *temp = *head;
    struct Node *prev = NULL;
    int retval = -1;

    if (temp == NULL)
    {
        return 0;
    }

    if (temp->data == data || data == -1)
    {
        *head = temp->next;
        *status = temp->data2;
        retval = temp->data;
        free(temp);
        return retval;
    }

    while (temp != NULL && (temp->data != data))
    {
        prev = temp;
        temp = temp->next;
    }

    if (temp == NULL)
    {
        return 0;
    }

    prev->next = temp->next;
    *status = temp->data2;
    retval = temp->data;
    free(temp);

    return retval;
}

void l_append(struct Node **head, int data)
{
    struct Node *new_node = l_create_new_node(data);

    new_node->next = NULL;

    if (*head == NULL)
    {
        *head = new_node;
        return;
    }

    struct Node *cur_node = *head;

    while (cur_node->next != NULL)
    {
        cur_node = cur_node->next;
    }

    cur_node->next = new_node;
}

void l_append_2(struct Node **head, int data, int data2)
{
    struct Node *new_node = l_create_new_node_2(data, data2);

    new_node->next = NULL;

    if (*head == NULL)
    {
        *head = new_node;
        return;
    }

    struct Node *cur_node = *head;

    while (cur_node->next != NULL)
    {
        cur_node = cur_node->next;
    }

    cur_node->next = new_node;
}

void l_delete_data(struct Node **head, int data)
{
    struct Node *temp = *head;
    struct Node *prev = NULL;

    if (temp == NULL)
    {
        return;
    }

    if (temp->data == data)
    {
        *head = temp->next;
        free(temp);
        return;
    }

    while (temp != NULL && temp->data != data)
    {
        prev = temp;
        temp = temp->next;
    }

    if (temp == NULL)
    {
        return;
    }

    prev->next = temp->next;
    free(temp);

    return;
}

void l_delete_data_all(struct Node **head, int data)
{
    struct Node *temp = *head;
    struct Node *prev = NULL;

    if (temp == NULL)
    {
        return;
    }

    while (temp != NULL && temp->data == data)
    {
        *head = temp->next;
        free(temp);
        temp = temp->next;
    }

    while (temp != NULL)
    {
        if (temp->data == data)
        {
            if (prev != NULL)
            {
                prev->next = temp->next;
                free(temp);
                temp = prev->next;
            }
            else
            {
                prev = temp;
                temp = temp->next;
                free(prev);
                prev = NULL;
            }
        }
        else
        {
            prev = temp;
            temp = temp->next;
        }
    }
}

void l_free_list(struct Node *head)
{
    struct Node *cur_node = head;
    while (cur_node != NULL)
    {
        struct Node *temp = cur_node;
        cur_node = cur_node->next;
        free(temp);
    }
}

void print_list(struct Node *head)
{
    struct Node *temp = head;
    printf("PRINTING LIST: \n");

    while (temp != NULL)
    {
        printf("FIRST: %d, SECOND: %d\n", temp->data, temp->data2);
        temp = temp->next;
    }
}

struct Node *remove_data(struct Node **head, int thread_pid)
{
    struct Node *temp = *head;
    struct Node *prev = NULL;

    if (temp != NULL && temp->data == thread_pid)
    {
        *head = temp->next;
        return temp;
    }

    while (temp != NULL && temp->data != thread_pid)
    {
        prev = temp;
        temp = temp->next;
    }

    if (temp == NULL)
    {
        return NULL;
    }

    prev->next = temp->next;

    return temp;
}