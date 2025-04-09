// hashmap.c
#include "hashmap-lock-fiber.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h> // if needed

// 1) Implement a simpler hash2d
// inline unsigned int hash2d(void *fiber, void *lock, int table_size) {
//     uintptr_t f = (uintptr_t)fiber;
//     uintptr_t l = (uintptr_t)lock;
//     uintptr_t combined = f ^ (l << 1);
//     return (unsigned int)(combined & (table_size - 1));
// }


// 2) Implement create_hashmap2d
hashmap2d *create_hashmap2d(int table_size) {
    hashmap2d *hm = malloc(sizeof(hashmap2d));
    if (!hm) {
        fprintf(stderr, "Error allocating memory for hashmap2d.\n");
        exit(EXIT_FAILURE);
    }
    hm->table_size = table_size;
    hm->entries = malloc(sizeof(entry *) * table_size);
    if (!hm->entries) {
        fprintf(stderr, "Error allocating memory for entries.\n");
        exit(EXIT_FAILURE);
    }
    for (int i = 0; i < table_size; i++) {
        hm->entries[i] = NULL;
    }
    return hm;
}

// 3) Implement insert
void insert(hashmap2d *hm, void *fiber, void *lock, lock_stats_t value) {
    unsigned int index = hash2d(fiber, lock, hm->table_size);
    entry *current = hm->entries[index];

    // Check if an entry with the same fiber & lock already exists
    while (current) {
        if (current->fiber == fiber && current->lock == lock) {
            current->value = value;  // update existing
            return;
        }
        current = current->next;
    }

    // Not found: create a new entry
    entry *new_entry = malloc(sizeof(entry));
    if (!new_entry) {
        fprintf(stderr, "Error allocating memory for new entry.\n");
        exit(EXIT_FAILURE);
    }
    new_entry->fiber = fiber;
    new_entry->lock = lock;
    new_entry->value = value;
    new_entry->next = hm->entries[index];
    hm->entries[index] = new_entry;
}

// 4) Implement get
int get(hashmap2d *hm, void *fiber, void *lock, lock_stats_t *value_out) {
    unsigned int index = hash2d(fiber, lock, hm->table_size);
    entry *current = hm->entries[index];

    while (current) {
        if (current->fiber == fiber && current->lock == lock) {
            *value_out = current->value;
            return 1;  // found
        }
        current = current->next;
    }
    return 0;  // not found
}

// 5) Implement free_hashmap
void free_hashmap(hashmap2d *hm) {
    for (int i = 0; i < hm->table_size; i++) {
        entry *current = hm->entries[i];
        while (current) {
            entry *temp = current;
            current = current->next;
            free(temp);
        }
    }
    free(hm->entries);
    free(hm);
}