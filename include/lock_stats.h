#ifndef _FIBER_LOCK_STATS_
#define _FIBER_LOCK_STATS_

#include "timing.h"  

typedef struct lock_stats{
    struct timeval banned_until;
    struct timeval slice_size;
    struct timeval start_ticks;
    struct timeval end_ticks;
} lock_stats_t;

#endif