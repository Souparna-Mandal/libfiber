#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fiber_lock_stats.h>
#include <fiber.h>

// Each entry now stores two keys: fiber and lock, plus the value.
typedef struct entry {
    fiber_t *fiber;
    void *lock;
    lock_stats_t *value; // returns a pointer to the lock_stats_t type with the ban_time and slice_time 
    struct entry *next;
} entry;

// The 2D hashmap structure.
typedef struct {
    int table_size;   // User-defined table size.
    entry **entries;
} hashmap2d;

// 2D hash function that processes both the fiber and lock pointers.
// This uses the 64-bit FNV-1a algorithm for each pointer.
unsigned int hash2d(void *fiber, void *lock, int table_size) {
    uint64_t hash = 14695981039346656037ULL;
    uint64_t fnv_prime = 1099511628211ULL;
    
    // Process the fiber pointer bytes.
    uint64_t fiber_val = (uint64_t) fiber;
    for (int i = 0; i < 8; i++) {
        uint8_t byte = (fiber_val >> (i * 8)) & 0xFF;
        hash ^= byte;
        hash *= fnv_prime;
    }
    
    // Process the lock pointer bytes.
    uint64_t lock_val = (uint64_t) lock;
    for (int i = 0; i < 8; i++) {
        uint8_t byte = (lock_val >> (i * 8)) & 0xFF;
        hash ^= byte;
        hash *= fnv_prime;
    }
    
    return (unsigned int)(hash % table_size);
}

// Create and initialize a new 2D hashmap.
hashmap2d *create_hashmap2d(int table_size) {
    hashmap2d *hm = malloc(sizeof(hashmap2d));
    if (hm == NULL) {
        fprintf(stderr, "Error allocating memory for hashmap2d.\n");
        exit(EXIT_FAILURE);
    }
    hm->table_size = table_size;
    hm->entries = malloc(sizeof(entry2 *) * table_size);
    if (hm->entries == NULL) {
        fprintf(stderr, "Error allocating memory for entries.\n");
        exit(EXIT_FAILURE);
    }
    for (int i = 0; i < table_size; i++) {
        hm->entries[i] = NULL;
    }
    return hm;
}

// Insert or update a key-value pair in the 2D hashmap using both fiber and lock as keys.
void insert(hashmap2d *hm, fiber_t *fiber, void *lock, lock_stats_t *value) {
    unsigned int index = hash2d(fiber, lock, hm->table_size);
    entry *current = hm->entries[index];

    // Check if an entry with the same fiber and lock already exists.
    while (current != NULL) {
        if (current->fiber == fiber && current->lock == lock) {
            current->value = value;
            return;
        }
        current = current->next;
    }

    // Entry not found: create a new entry and insert it at the head.
    entry *new_entry = malloc(sizeof(entry2));
    if (new_entry == NULL) {
        fprintf(stderr, "Error allocating memory for new entry.\n");
        exit(EXIT_FAILURE);
    }
    new_entry->fiber = fiber;
    new_entry->lock = lock;
    new_entry->value = value;
    new_entry->next = hm->entries[index];
    hm->entries[index] = new_entry;
}

// Retrieve a value using two keys: fiber and lock.
// Returns 1 (and sets *value_out) if the keys are found; returns 0 otherwise.
int get(hashmap2d *hm, fiber_t *fiber, void *lock, lock_stats_t **value_out) {
    unsigned int index = hash2d(fiber, lock, hm->table_size);
    entry2 *current = hm->entries[index];
    while (current != NULL) {
        if (current->fiber == fiber && current->lock == lock) {
            *value_out = current->value;
            return 1;
        }
        current = current->next;
    }
    return 0; // Keys not found.
}

// Free the memory allocated for the 2D hashmap.
// Note: This does not free memory for the keys or the values.
void free_hashmap(hashmap2d *hm) {
    for (int i = 0; i < hm->table_size; i++) {
        entry2 *current = hm->entries[i];
        while (current != NULL) {
            entry2 *temp = current;
            current = current->next;
            free(temp);
        }
    }
    free(hm->entries);
    free(hm);
}

/* 
// Sample usage:
int main() {
    // User-specified table size (for example, 100).
    int table_size = 100;
    hashmap2d *hm = create_hashmap2d(table_size);

    int fiber1 = 1, fiber2 = 2;
    int lock1 = 100, lock2 = 200;
    int valueA = 123, valueB = 456;

    // Insert key-value pairs where keys are the addresses of fiber and lock.
    insert2d(hm, &fiber1, &lock1, &valueA);
    insert2d(hm, &fiber2, &lock2, &valueB);

    // Retrieve and print a value using both keys.
    void *value;
    if (get2d(hm, &fiber1, &lock1, &value)) {
        printf("For fiber key %p and lock key %p, value is %p (points to %d)\n", 
               (void*)&fiber1, (void*)&lock1, value, *(int *)value);
    } else {
        printf("Key not found.\n");
    }

    // Clean up.
    free_hashmap2d(hm);
    return 0;
}
*/

