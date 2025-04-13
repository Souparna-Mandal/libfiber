#ifndef SIMPLE_HASHMAP_H
#define SIMPLE_HASHMAP_H

#include <stdlib.h>
#include <stdint.h>

#define HASHMAP_SIZE 64  // Adjust as needed

typedef struct hashmap_entry {
    void* key;
    int value;
    struct hashmap_entry* next;
} hashmap_entry_t;

typedef struct hashmap {
    hashmap_entry_t* buckets[HASHMAP_SIZE];
} hashmap_t;

hashmap_t* hashmap_create();
int hashmap_get(hashmap_t* map, void* key, int* out_value);
int hashmap_put(hashmap_t* map, void* key, int value);
void hashmap_destroy(hashmap_t* map);

#endif
