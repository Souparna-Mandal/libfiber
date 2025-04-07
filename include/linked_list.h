#ifndef LINKEDLIST_H
#define LINKEDLIST_H

#include <stdlib.h>

/* Node structure:
 * - value: holds a pointer (can be NULL initially)
 * - next: points to the next node in the list.
 */
typedef struct Node {
    void *value;
    struct Node *next;
} Node ;

/* LinkedList structure:
 * - head: pointer to the first node in the list.
 */
typedef struct LinkedList {
    Node *head;
} LinkedList;

/* Function Declarations */

/* 
 * Creates a new, empty linked list.
 * Returns a pointer to the new LinkedList structure.
 */
LinkedList *llist_create(void);

/*
 * Inserts a new node at the beginning of the linked list.
 * The new node's value is set to the provided pointer.
 * Note: The value pointer can be NULL, and it is up to the user to cast it later.
 */
void llist_insert(LinkedList *list, void *value);

/*
 * Deletes the first node in the linked list whose value pointer matches the provided value.
 * Returns 1 if a node was successfully deleted, or 0 if no matching node was found.
 */
int llist_delete(LinkedList *list, void *value);

/*
 * Frees all nodes in the linked list.
 * Does not free the memory pointed to by the node values.
 */
void llist_free(LinkedList *list);

#endif /* LINKEDLIST_H */
