// SPDX-FileCopyrightText: 2012-2023 Brian Watling <brian@oxbo.dev>
// SPDX-License-Identifier: MIT

#ifndef _FIBER_SCHEDULER_H_
#define _FIBER_SCHEDULER_H_

#include "fiber.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void* fiber_scheduler_t;

int is_fiber_runable(fiber_t* fiber, hashmap2d* lock_fiber_d, colours_t* running); // check the colour, and if any of the 

int fiber_scheduler_init(size_t num_threads);

void fiber_scheduler_shutdown();

fiber_scheduler_t* fiber_scheduler_for_thread(size_t thread_id);

void fiber_scheduler_schedule(fiber_scheduler_t* scheduler, fiber_t* the_fiber); // add it to thr queue to schedule 

fiber_t* fiber_scheduler_next(fiber_scheduler_t* scheduler, hashmap2d* lock_fiber_d, colours_t running);

void fiber_scheduler_load_balance(fiber_scheduler_t* scheduler);

void fiber_scheduler_stats(fiber_scheduler_t* scheduler, uint64_t* steal_count,
                           uint64_t* failed_steal_count);

#ifdef __cplusplus
}
#endif

#endif
