// hashmap.h
#ifndef _HASHMAP_H_
#define _HASHMAP_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

typedef struct entry {
    void *fiber;
    void *lock;
    unsigned long long value;
    struct entry *next;
} entry;

typedef struct {
    int table_size;
    entry **entries;
} hashmap2d;

// Prototypes only:
unsigned int hash2d(void *fiber, void *lock, int table_size);
hashmap2d *create_hashmap2d(int table_size);
void insert(hashmap2d *hm, void *fiber, void *lock, unsigned long long value);
int get(hashmap2d *hm, void *fiber, void *lock, unsigned long long *value_out);
void free_hashmap(hashmap2d *hm);

#endif
