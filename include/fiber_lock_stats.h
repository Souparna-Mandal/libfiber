#ifndef _FIBER_LOCK_STATS_H_
#define _FIBER_LOCK_STATS_H_

#include <time.h>
#include <sys/time.h>

/*
It is just supporting one lock for now, we will change it to support Many locks using a hash table
*/
typedef struct lock_stats{
    struct timeval banned_until;
    struct timeval slice_size;
} lock_stats_t;

#endif