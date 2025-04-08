#include "hashmap-lock-index.h"
#include <string.h>

static uint64_t pointer_hash(void* key) {
    uintptr_t val = (uintptr_t)key;
    val = (val >> 3) ^ (val >> 13) ^ (val >> 23);
    return val % HASHMAP_SIZE;
}

hashmap_t* hashmap_create() {
    hashmap_t* map = (hashmap_t*)calloc(1, sizeof(hashmap_t));
    return map;
}

int hashmap_get(hashmap_t* map, void* key, int* out_value) {
    uint64_t index = pointer_hash(key);
    hashmap_entry_t* entry = map->buckets[index];
    while (entry) {
        if (entry->key == key) {
            *out_value = entry->value;
            return 1;  // found
        }
        entry = entry->next;
    }
    return 0;  // not found
}

int hashmap_put(hashmap_t* map, void* key, int value) {
    uint64_t index = pointer_hash(key);
    hashmap_entry_t* entry = map->buckets[index];

    while (entry) {
        if (entry->key == key) {
            entry->value = value;  // update existing entry
            return 1;
        }
        entry = entry->next;
    }

    // Not found; create new entry
    hashmap_entry_t* new_entry = (hashmap_entry_t*)malloc(sizeof(hashmap_entry_t));
    if (!new_entry) return 0;

    new_entry->key = key;
    new_entry->value = value;
    new_entry->next = map->buckets[index];
    map->buckets[index] = new_entry;

    return 1;
}

void hashmap_destroy(hashmap_t* map) {
    for (int i = 0; i < HASHMAP_SIZE; ++i) {
        hashmap_entry_t* entry = map->buckets[i];
        while (entry) {
            hashmap_entry_t* tmp = entry;
            entry = entry->next;
            free(tmp);
        }
    }
    free(map);
}