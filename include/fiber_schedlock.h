#ifndef _FIBER_SCHED_LOCK_H_
#define _FIBER_SCHED_LOCK_H_

#include <stdio.h>                  
#include <stdbool.h>        
#include <stdatomic.h>      
             
#include "timing.h"         
#include "fiber_manager.h"  
// #include "fiber_mutex.h"  
#include "fiber_spinlock.h"

#define SLICE_SIZE_US 200

// static struct timeval inactive_threshold = {1, 0}; 

typedef struct sched_lock {
    struct timeval start_ticks;
    struct timeval end_ticks;
    struct timeval slice_end_time;
    fiber_t* holder;
    // fiber_mutex_t mutex;
    // fiber_spinlock_t spinlock;
    lock_stats_t* lock_stat;
    int slice_set;

} sched_lock_t;


void sched_lock_init(struct sched_lock *lock);

void sched_lock_acquire(struct sched_lock *lock);

void sched_lock_release(struct sched_lock *lock);

void ban_fibers(struct sched_lock *lock, fiber_t* holder);

#endif