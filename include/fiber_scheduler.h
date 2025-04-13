// SPDX-FileCopyrightText: 2012-2023 Brian Watling <brian@oxbo.dev>
// SPDX-License-Identifier: MIT

#ifndef _FIBER_SCHEDULER_H_
#define _FIBER_SCHEDULER_H_

#include <pthread.h>

#include "fiber.h"
#include "fiber_spinlock.h"
#include "hashmap-lock-fiber.h"
#include "hashmap-lock-index.h"


#ifdef __cplusplus
extern "C" {
#endif

typedef void* fiber_scheduler_t;

/* Start of Data Structures Required for Fair Lock Scheduler*/
extern pthread_spinlock_t scheduler_spinlock; // Need to decide if this is a good idea 
extern hashmap_t* locks_to_indices;
extern hashmap2d* lock_fiber_d;
extern uint64_t colours;
extern fiber_spinlock_t lock_index;
extern fiber_spinlock_t free_slice;
extern int current_lock_index;
/* End of Data Structures Required for Fair Lock Scheduler*/

int fiber_scheduler_init(size_t num_threads);

int try_update_colour(uint64_t fiber_colour);

int try_update_colour_lock(uint64_t fiber_colour);

void reset_colour_scheduling_fiber_lock(uint64_t fiber_colour);

int fiber_colour_scheduling_check(uint64_t fiber_colour);

int is_fiber_runable(fiber_t* fiber);

void try_free_expired_slices(LinkedList* locks);

void reset_colour_scheduling_fiber(uint64_t fiber_color);

void update_color_scheduling(uint64_t fiber_color);

void fiber_scheduler_shutdown();

fiber_scheduler_t* fiber_scheduler_for_thread(size_t thread_id);

void fiber_scheduler_schedule(fiber_scheduler_t* scheduler, fiber_t* the_fiber);

fiber_t* fiber_scheduler_next(fiber_scheduler_t* scheduler, fiber_t * current_fiber);

void fiber_scheduler_load_balance(fiber_scheduler_t* scheduler);

void fiber_scheduler_stats(fiber_scheduler_t* scheduler, uint64_t* steal_count,
                           uint64_t* failed_steal_count);

#ifdef __cplusplus
}
#endif

#endif
