// hashmap.c
#include "hashmap.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h> // if needed

// 1) Implement hash2d
unsigned int hash2d(void *fiber, void *lock, int table_size) {
    uint64_t hash = 14695981039346656037ULL;
    uint64_t fnv_prime = 1099511628211ULL;

    // Process fiber pointer
    uint64_t fiber_val = (uint64_t) fiber;
    for (int i = 0; i < 8; i++) {
        uint8_t byte = (fiber_val >> (i * 8)) & 0xFF;
        hash ^= byte;
        hash *= fnv_prime;
    }

    // Process lock pointer
    uint64_t lock_val = (uint64_t) lock;
    for (int i = 0; i < 8; i++) {
        uint8_t byte = (lock_val >> (i * 8)) & 0xFF;
        hash ^= byte;
        hash *= fnv_prime;
    }

    return (unsigned int)(hash % table_size);
}

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
void insert(hashmap2d *hm, void *fiber, void *lock, unsigned long long value) {
    unsigned int index = hash2d(fiber, lock, hm->table_size);
    entry *current = hm->entries[index];

    // Check if an entry with the same fiber & lock already exists
    while (current) {
        if (current->fiber == fiber && current->lock == lock) {
            current->value = value;
            return;
        }
        current = current->next; 
    } // why a while loop here  TODO

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
int get(hashmap2d *hm, void *fiber, void *lock, unsigned long long *value_out) {
    unsigned int index = hash2d(fiber, lock, hm->table_size);
    entry *current = hm->entries[index];
    while (current) {
        if (current->fiber == fiber && current->lock == lock) {
            value_out = &current->value; // address of the ull 
            return 1;
        }
        current = current->next;
    }
    return 0; // not found
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
