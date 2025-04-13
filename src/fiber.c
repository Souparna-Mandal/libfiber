// SPDX-FileCopyrightText: 2012-2023 Brian Watling <brian@oxbo.dev>
// SPDX-License-Identifier: MIT

#include "fiber.h"

#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include "fiber_scheduler.h"
#include "fiber_schedlock.h"
#include "timing.h"

static uint fiber_global_id = 0;
hashmap_t* locks_to_indices;
hashmap2d* lock_fiber_d;
int current_lock_index;

#ifdef DEBUG
ull fiber_run_time = 0;
#endif

#include "fiber_manager.h"
#include "mpmc_lifo.h"

void fiber_mark_completed(fiber_t* the_fiber, void* result) {
  atomic_store_explicit(&the_fiber->result, result, memory_order_release);
  reset_colour_scheduling_fiber_lock(the_fiber->bitcolour); // reset colour 

  if (the_fiber->detach_state != FIBER_DETACH_DETACHED) {
    const int old_state =
        atomic_exchange(&the_fiber->detach_state, FIBER_DETACH_WAIT_FOR_JOINER);
    if (old_state == FIBER_DETACH_NONE) {
      // need to wait until another fiber joins this one
      fiber_manager_set_and_wait(fiber_manager_get(),
                                 (void**)&the_fiber->join_info, the_fiber);
    } else if (old_state == FIBER_DETACH_WAIT_TO_JOIN) {
      // the joining fiber is waiting for us to finish
      fiber_t* const to_schedule = fiber_manager_clear_or_wait(
          fiber_manager_get(), (_Atomic(void*)*)&the_fiber->join_info);
      to_schedule->result = the_fiber->result;
      to_schedule->state = FIBER_STATE_READY;
      fiber_manager_schedule(fiber_manager_get(), to_schedule);
    }
  }

  the_fiber->state = FIBER_STATE_DONE;
}

static void fiber_join_routine(fiber_t* the_fiber, void* result) {
  fiber_mark_completed(the_fiber, result);
  fiber_manager_get()->done_fiber = the_fiber;
  fiber_manager_yield(fiber_manager_get());
  assert(0 && "should never get here");
}

#ifdef FIBER_STACK_SPLIT
__attribute__((__no_split_stack__))
#endif
static void*
fiber_go_function(void* param) {
  fiber_t* the_fiber = (fiber_t*)param;

  /* do maintenance - this is usually done after fiber_context_swap, but we do
   * it here too since we are coming from a new place */
  fiber_manager_do_maintenance();

  void* const result = the_fiber->run_function(the_fiber->param);

  fiber_join_routine(the_fiber, result);

  return NULL;
}

fiber_t* fiber_create_no_sched(size_t stack_size,
                               fiber_run_function_t run_function, void* param) {
  fiber_t* ret = calloc(1, sizeof(*ret));
  if (!ret) {
    errno = ENOMEM;
    return NULL;
  }
  ret->mpsc_fifo_node = calloc(1, sizeof(*ret->mpsc_fifo_node));
  if (!ret->mpsc_fifo_node) {
    free(ret);
    errno = ENOMEM;
    return NULL;
  }

  ret->run_function = run_function;
  ret->param = param;
  ret->state = FIBER_STATE_READY;
  ret->detach_state = FIBER_DETACH_NONE;
  ret->join_info = NULL;
  ret->result = NULL;
  ret->id += 1;
  ret->bitcolour = 0;
  ret->locks = llist_create();
  if (FIBER_SUCCESS !=
      fiber_context_init(&ret->context, stack_size, &fiber_go_function, ret)) {
    free(ret);
    return NULL;
  }

  return ret;
}

fiber_t* fiber_create(size_t stack_size, fiber_run_function_t run_function,
                      void* param) {
  fiber_t* const ret = fiber_create_no_sched(stack_size, run_function, param);
  if (ret) {
    ret->fiber_id = ++fiber_global_id;
    ret->count = 0;
    fiber_manager_schedule(fiber_manager_get(), ret);
  }
  return ret;
}

fiber_t* fiber_create_from_thread() {
  fiber_t* const ret = calloc(1, sizeof(*ret));
  if (!ret) {
    errno = ENOMEM;
    return NULL;
  }
  ret->mpsc_fifo_node = calloc(1, sizeof(*ret->mpsc_fifo_node));
  if (!ret->mpsc_fifo_node) {
    free(ret);
    errno = ENOMEM;
    return NULL;
  }

  ret->state = FIBER_STATE_RUNNING;
  ret->detach_state = FIBER_DETACH_NONE;
  ret->join_info = NULL;
  ret->result = NULL;
  ret->id = 1;
  ret->bitcolour = 0;
  ret->locks = llist_create();
  if (FIBER_SUCCESS != fiber_context_init_from_thread(&ret->context)) {
    free(ret);
    return NULL;
  }
  return ret;
}

#include <stdio.h>

int fiber_join(fiber_t* f, void** result) {
  assert(f);
  if (result) {
    *result = NULL;
  }
  if (f->detach_state == FIBER_DETACH_DETACHED) {
    return FIBER_ERROR;
  }

  const int old_state =
      atomic_exchange(&f->detach_state, FIBER_DETACH_WAIT_TO_JOIN);
  if (old_state == FIBER_DETACH_NONE) {
    // need to wait till the fiber finishes
    fiber_manager_t* const manager = fiber_manager_get();
    fiber_t* const current_fiber = manager->current_fiber;
    fiber_manager_set_and_wait(manager, (void**)&f->join_info, current_fiber);
    if (result) {
      *result = current_fiber->result;
    }
    current_fiber->result = NULL;
  } else if (old_state == FIBER_DETACH_WAIT_FOR_JOINER) {
    // the other fiber is waiting for us to join
    if (result) {
      *result = f->result;
    }
    fiber_t* const to_schedule = fiber_manager_clear_or_wait(
        fiber_manager_get(), (_Atomic(void*)*)&f->join_info);
    to_schedule->state = FIBER_STATE_READY;
    fiber_manager_schedule(fiber_manager_get(), to_schedule);
  } else {
    // it's either WAIT_TO_JOIN or DETACHED - that's an error!
    return FIBER_ERROR;
  }

  return FIBER_SUCCESS;
}

int fiber_tryjoin(fiber_t* f, void** result) {
  assert(f);
  if (result) {
    *result = NULL;
  }
  if (f->detach_state == FIBER_DETACH_DETACHED) {
    return FIBER_ERROR;
  }

  if (f->detach_state == FIBER_DETACH_WAIT_FOR_JOINER) {
    // here we've read that the fiber is waiting to be joined.
    // if the fiber is still waiting to be joined after we atmically change its
    // state, then we can go ahead and wake it up. if the fiber's state has
    // changed, we can assume the fiber has been detached or has be joined by
    // some other fiber
    const int old_state =
        atomic_exchange(&f->detach_state, FIBER_DETACH_WAIT_TO_JOIN);
    if (old_state == FIBER_DETACH_WAIT_FOR_JOINER) {
      // the other fiber is waiting for us to join
      if (result) {
        *result = f->result;
      }
      fiber_t* const to_schedule = fiber_manager_clear_or_wait(
          fiber_manager_get(), (_Atomic(void*)*)&f->join_info);
      to_schedule->state = FIBER_STATE_READY;
      fiber_manager_schedule(fiber_manager_get(), to_schedule);
      return FIBER_SUCCESS;
    }
  }

  return FIBER_ERROR;
}

int fiber_yield(int lock_free_yield) {
  fiber_manager_t* m = fiber_manager_get();
#ifdef DEBUG
  gettimeofday(&m->end_time_d, NULL);
  if (m->end_time_set== 1){
    fiber_run_time +=
        time_difference(&m->start_time_d, &m->end_time_d);
  }

#endif
  if (lock_free_yield){
    fiber_yield_lock_processing(m->current_fiber);
    if (m->current_fiber) {
      reset_colour_scheduling_fiber_lock(m->current_fiber->bitcolour); //reset colour
    }
  }
  fiber_manager_yield(m);
#ifdef DEBUG
  gettimeofday(&m->start_time_d, NULL);
  if (m->end_time_set== 0) { m->end_time_set = 1; }

#endif
  return 1;
}

int fiber_detach(fiber_t* f) {
  if (!f) {
    return FIBER_ERROR;
  }
  const int old_state =
      atomic_exchange(&f->detach_state, FIBER_DETACH_DETACHED);
  if (old_state == FIBER_DETACH_WAIT_FOR_JOINER ||
      old_state == FIBER_DETACH_WAIT_TO_JOIN) {
    // wake up the fiber or the fiber trying to join it (this second case is a
    // convenience, pthreads specifies undefined behaviour in that case)
    fiber_t* const to_schedule = fiber_manager_clear_or_wait(
        fiber_manager_get(), (_Atomic(void*)*)&f->join_info);
    to_schedule->state = FIBER_STATE_READY;
    fiber_manager_schedule(fiber_manager_get(), to_schedule);
  } else if (old_state == FIBER_DETACH_DETACHED) {
    return FIBER_ERROR;
  }
  return FIBER_SUCCESS;
}

/* NEW METHODS TO SUPPORT Fair Lock Scheduler*/
void set_fib_colour(fiber_t* f, int index, void* lock) {
  // This means if this is 0 then lock has been previously recorded
     f->bitcolour |= (1 << index);
 }

LinkedList* get_locks(fiber_t* f) { return f->locks; } // locks is an array of pointers void*

void remove_fiber_from_locks(fiber_t* f){
  if (f->locks) {
    Node *current = f->locks->head;
    while (current) {
        Node *temp = current;
        current = current->next;
        sched_lock_t* lock = (sched_lock_t*)temp->value;
        atomic_fetch_sub_explicit(&lock->num_holders, 1, memory_order_relaxed);
    }
}
// llist_free(f->locks);
}

// This is needed to find the index each lock corresponds to in the colours vector
int get_lock_index(void* lock) {
  int index;
  fiber_spinlock_lock(&lock_index);
  // pthread_spin_lock(&shared_ds_lock);
  if (!hashmap_get(locks_to_indices, lock, &index)) {
    index = current_lock_index++;
    hashmap_put(locks_to_indices, lock, index);
  }
  fiber_spinlock_unlock(&lock_index);
  return index;
}

// Set Data in the Shared Hashmap holding lock-fiber statistics for each fiber and lock pair 
void set_lock_fiber_data(void* lock, struct timeval ban_time, struct timeval time_slice, fiber_t* fiber) {
  fiber_manager_t* manager = fiber_manager_get();
  if (fiber == NULL){
    fiber = manager->current_fiber;
  }
  lock_stats_t temp_stats = {ban_time, time_slice};
  insert(lock_fiber_d, (void*)fiber, lock, temp_stats);
}


// Get Data in the Shared Hashmap holding lock-fiber statistics for each fiber and lock pair 
lock_stats_t* get_lock_fiber_data(void* lock, lock_stats_t* lock_stat, fiber_t* f) {
  if (f == NULL){
    f = fiber_manager_get()->current_fiber;
  }
  if (get(lock_fiber_d, (void*)f, lock, lock_stat)) {
      return lock_stat;
  }
  return NULL; 
}

// Assign colours to threads (Auto-Colouring)
// Add and associate it with exsiting fibers 
void record_lock_for_fiber(void* lock, int slice_size_us, fiber_t* f){
  if (f == NULL){
    f = fiber_manager_get()->current_fiber;
  }
  sched_lock_t* s_lock = (sched_lock_t*) lock;
  int lock_index = get_lock_index(lock);
  if (((f->bitcolour) & (1 << lock_index)) == 0) {  // This means if this is 1 then lock has been previously recorded
    // printf("Recording a lock  of index %d\n", lock_index);
    set_fib_colour(f, lock_index, lock);
    add_locks(f, lock); // add locks 
    set_lock_fiber_data(lock, /* ban time*/ (struct timeval){0,0}, /* slice time */ (struct timeval){0, slice_size_us}, NULL);
    atomic_fetch_add_explicit(&s_lock->num_holders, 1, memory_order_relaxed); // update lock data 
    fiber_yield(1);
    // we yield after setting the fiber colour for the first time for a lock
  }
}

void add_locks(fiber_t* f, void* lock) { // Add a lock to the list of locks being used by the fiber 
  
  llist_insert(f->locks, lock);
}

void remove_locks(fiber_t* f, void* lock){

  llist_delete(f->locks, lock);
}

void fiber_yield_lock_processing(fiber_t* fiber){
  Node* current = fiber->locks->head;
  sched_lock_t* lock;
  while (current) {
    if (current->value) {
      lock = (sched_lock_t*)current->value;
    } else {
      printf("ERROR no lock.... skipping");
      current = current->next;
      continue;
    }
    ban_fibers(lock, fiber);

    /*Reset LOCK*/
    lock->start_ticks = (struct timeval){0, 0};
    lock->end_ticks = (struct timeval){0, 0};
    lock->slice_end_time = (struct timeval){0, 0};
    lock->lock_stat->banned_until = (struct timeval){0, 0};
    lock->lock_stat->slice_size = (struct timeval){0, 0};
    
    atomic_store_explicit(&lock->holder, NULL, memory_order_release);
    atomic_store_explicit(&lock->slice_state, SLICE_FREE, memory_order_release);
    atomic_store_explicit(&lock->lock_held, 0, memory_order_release);

    current = current->next;
  }
}