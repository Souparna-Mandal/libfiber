#include "linked_list.h"

LinkedList *llist_create(void) {
    LinkedList *list = (LinkedList *)malloc(sizeof(LinkedList));
    if (list) {
        list->head = NULL;
    }
    return list;
}

void llist_insert(LinkedList *list, void *value) {
    if (list) {
        Node *new_node = (Node *)malloc(sizeof(Node));
        if (new_node) {
            new_node->value = value;
            new_node->next = list->head;  // Insert at the start
            list->head = new_node;
        }
    }
}

int llist_delete(LinkedList *list, void *value) {
    if (!list || !list->head) {
        return 0;
    }

    Node *current = list->head;
    Node *previous = NULL;

    // Check if the head node is the one to be deleted.
    if (current->value == value) {
        list->head = current->next;
        free(current);
        return 1;
    }

    // Search through the rest of the list.
    while (current != NULL) {
        if (current->value == value) {
            previous->next = current->next;
            free(current);
            return 1;
        }
        previous = current;
        current = current->next;
    }
    return 0;
}

void llist_free(LinkedList *list) {
    if (list) {
        Node *current = list->head;
        while (current) {
            Node *temp = current;
            current = current->next;
            free(temp);
        }
        free(list);
    }
}
