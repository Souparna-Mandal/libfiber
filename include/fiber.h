// SPDX-FileCopyrightText: 2012-2023 Brian Watling <brian@oxbo.dev>
// SPDX-License-Identifier: MIT

#ifndef _FIBER_FIBER_H_
#define _FIBER_FIBER_H_

#include <stdint.h>

#include "fiber_context.h"
#include "mpsc_fifo.h"
#include "linked_list.h"
#include "lock_stats.h"

typedef int fiber_state_t;

struct fiber_manager;

#define FIBER_STATE_RUNNING (1)
#define FIBER_STATE_READY (2)
#define FIBER_STATE_WAITING (3)
#define FIBER_STATE_DONE (4)
#define FIBER_STATE_SAVING_STATE_TO_WAIT (5)

#define FIBER_DETACH_NONE (0)
#define FIBER_DETACH_WAIT_FOR_JOINER (1)
#define FIBER_DETACH_WAIT_TO_JOIN (2)
#define FIBER_DETACH_DETACHED (3)
#define MAX_FIBS (1024)
#define MAX_LOCKS (64)

typedef struct fiber {
  uint64_t bitcolour;
  LinkedList* locks;
  uint64_t fiber_id; 
  long long int count;

  volatile fiber_state_t state;
  fiber_run_function_t run_function;
  void* param;
  uint64_t volatile id; /* not unique globally, only within this fiber instance.
                           used for joining */
  fiber_context_t context;
  _Atomic(void*) result;
  mpsc_fifo_node_t* volatile mpsc_fifo_node;
  _Atomic int detach_state;
  _Atomic(struct fiber*) join_info;
  void* volatile scratch;  // to be used by internal fiber mechanisms. be sure
                           // mechanisms do not conflict! (ie. only use scratch
                           // while a fiber is sleeping/waiting)
} fiber_t;

#ifdef DEBUG
typedef unsigned long long ull;
extern ull fiber_run_time;
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define FIBER_DEFAULT_STACK_SIZE (102400)
#define FIBER_MIN_STACK_SIZE (1024)

extern fiber_t* fiber_create(size_t stack_size, fiber_run_function_t run,
                             void* param);

extern fiber_t* fiber_create_no_sched(size_t stack_size,
                                      fiber_run_function_t run, void* param);

extern fiber_t* fiber_create_from_thread();

extern int fiber_join(fiber_t* f, void** result);

extern int fiber_tryjoin(fiber_t* f, void** result);

extern int fiber_yield();

extern int fiber_detach(fiber_t* f);

void set_fib_colour(fiber_t* f, int index, void* lock);
void remove_fiber_from_locks(fiber_t* f);
int get_lock_index(void* lock);
void set_lock_fiber_data(void* lock, struct timeval ban_time,
   struct timeval time_slice, 
   struct timeval start_ticks, 
   struct timeval end_ticks, 
   fiber_t* fiber);
   
lock_stats_t* get_lock_fiber_data(void* lock, lock_stats_t* lock_stat,
                                  fiber_t* f);
void record_lock_for_fiber(void* lock, int slice_size_us, fiber_t* f);
void add_locks(fiber_t* f, void* lock);
void remove_locks(fiber_t* f, void* lock);
LinkedList* get_locks(fiber_t* f);
void fiber_yield_lock_processing(fiber_t* fiber);

#ifdef __cplusplus
}
#endif

#endif
