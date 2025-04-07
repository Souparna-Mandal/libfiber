// SPDX-FileCopyrightText: 2012-2023 Brian Watling <brian@oxbo.dev>
// SPDX-License-Identifier: MIT

#ifndef _FIBER_FIBER_H_
#define _FIBER_FIBER_H_

#include <stdint.h>

#include "fiber_context.h"
#include "mpsc_fifo.h"
#include "hashmap.h"
#include "hashmap_1_d.h"
#include "linked_list.h"

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
#define MAX_LOCKS (64)
#define MAX_FIBS (1000)

typedef long long colours_t;

typedef struct fiber {
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
  LinkedList* locks;
  int num_locks;
  colours_t bitcolour;
  int kill_colour;
} fiber_t;

/* Stuff to make Libcolour and SCL work together*/ 

// Set the hashmap
extern hashmap2d* lock_fiber_d;

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

extern void set_fib_colour(fiber_t* f, int index, void* lock);

extern colours_t get_colour(fiber_t* f);

extern LinkedList* get_locks(fiber_t* f);

extern void add_locks(fiber_t* f, void* lock);

extern void remove_locks(fiber_t* f, void* lock);

extern void remove_fiber_from_locks(fiber_t* f);

extern int get_fiber_count();

extern int get_num_locks(fiber_t* f);

#ifdef __cplusplus
}
#endif

#endif
