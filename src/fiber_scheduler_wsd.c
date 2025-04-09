// SPDX-FileCopyrightText: 2012-2023 Brian Watling <brian@oxbo.dev>
// SPDX-License-Identifier: MIT

#include <assert.h>
#include <stddef.h>

#include "fiber_scheduler.h"
#include "work_stealing_deque.h"
#include "fiber_schedlock.h"

uint64_t colours;
pthread_spinlock_t scheduler_spinlock; // Need to decide if this is a good idea
fiber_spinlock_t lock_index;

typedef struct fiber_scheduler_wsd {
  wsd_work_stealing_deque_t* queue_one;
  wsd_work_stealing_deque_t* queue_two;
  wsd_work_stealing_deque_t* volatile schedule_from;
  wsd_work_stealing_deque_t* volatile store_to;
  size_t id;
  uint64_t steal_count;
  uint64_t failed_steal_count;
} fiber_scheduler_wsd_t;

static size_t fiber_scheduler_num_threads = 0;
static fiber_scheduler_wsd_t* fiber_schedulers = NULL;
static wsd_work_stealing_deque_t** fiber_scheduler_thread_queues = NULL;

int fiber_scheduler_wsd_init(fiber_scheduler_wsd_t* scheduler, size_t id) {
  assert(scheduler);
  scheduler->queue_one = wsd_work_stealing_deque_create();
  scheduler->queue_two = wsd_work_stealing_deque_create();
  scheduler->schedule_from = scheduler->queue_one;
  scheduler->store_to = scheduler->queue_two;
  scheduler->id = id;
  scheduler->steal_count = 0;
  scheduler->failed_steal_count = 0;

  if (!scheduler->queue_one || !scheduler->queue_two) {
    wsd_work_stealing_deque_destroy(scheduler->queue_one);
    wsd_work_stealing_deque_destroy(scheduler->queue_two);
    return 0;
  }
  return 1;
}

void fiber_scheduler_wsd_destroy(fiber_scheduler_wsd_t* scheduler) {
  wsd_work_stealing_deque_destroy(scheduler->queue_one);
  wsd_work_stealing_deque_destroy(scheduler->queue_two);
}

int fiber_scheduler_init(size_t num_threads) {
  assert(num_threads > 0);
  fiber_scheduler_num_threads = num_threads;

  assert(!fiber_schedulers);
  fiber_schedulers = calloc(num_threads, sizeof(*fiber_schedulers));
  assert(fiber_schedulers);
  assert(!fiber_scheduler_thread_queues);
  fiber_scheduler_thread_queues =
      calloc(2 * num_threads, sizeof(*fiber_scheduler_thread_queues));
  assert(fiber_scheduler_thread_queues);

  size_t i;
  for (i = 0; i < num_threads; ++i) {
    const int ret = fiber_scheduler_wsd_init(&fiber_schedulers[i], i);
    (void)ret;
    assert(ret);
    fiber_scheduler_thread_queues[i * 2] = fiber_schedulers[i].queue_one;
    fiber_scheduler_thread_queues[i * 2 + 1] = fiber_schedulers[i].queue_two;
  }
  return 1;
}

void fiber_scheduler_shutdown() {
  size_t i;
  for (i = 0; i < fiber_scheduler_num_threads; ++i) {
    fiber_scheduler_wsd_destroy(&fiber_schedulers[i]);
  }
  free(fiber_schedulers);
  fiber_schedulers = NULL;
  free(fiber_scheduler_thread_queues);
  fiber_scheduler_thread_queues = NULL;
}

fiber_scheduler_t* fiber_scheduler_for_thread(size_t thread_id) {
  assert(fiber_schedulers);
  assert(thread_id < fiber_scheduler_num_threads);
  return (fiber_scheduler_t*)&fiber_schedulers[thread_id];
}

void fiber_scheduler_schedule(fiber_scheduler_t* scheduler,
                              fiber_t* the_fiber) {
  assert(scheduler);
  assert(the_fiber);
  wsd_work_stealing_deque_push_bottom(
      ((fiber_scheduler_wsd_t*)scheduler)->schedule_from, the_fiber);
}

int try_update_colour_lock(uint64_t fiber_colour) {
  int result = 0;
  pthread_spin_lock(&scheduler_spinlock);
  if ((colours & fiber_colour) == 0) {
      colours |= fiber_colour;
      result = 1;
  }
  pthread_spin_unlock(&scheduler_spinlock);
  return result;
}

int try_update_colour(uint64_t fiber_colour) {
  int result = 0;
  // pthread_spin_lock(&scheduler_spinlock);
  if ((colours & fiber_colour) == 0) {
      colours |= fiber_colour;
      result = 1;
  }
  // pthread_spin_unlock(&scheduler_spinlock);
  return result;
}

void reset_colour_scheduling_fiber(uint64_t fiber_colour){
	//printf("Before color reset: fiber color %ld, Color_array %ld\n", fiber_color, color_array);
    // pthread_spin_lock(&scheduler_spinlock);    
    colours = colours & ~(fiber_colour);
    // pthread_spin_unlock(&scheduler_spinlock);
	//printf("After color reset: fiber color %ld, Color_array %ld\n", fiber_color, color_array);
}

void reset_colour_scheduling_fiber_lock(uint64_t fiber_colour){
	//printf("Before color reset: fiber color %ld, Color_array %ld\n", fiber_color, color_array);
    pthread_spin_lock(&scheduler_spinlock);    
    colours = colours & ~(fiber_colour);
    pthread_spin_unlock(&scheduler_spinlock);
	//printf("After color reset: fiber color %ld, Color_array %ld\n", fiber_color, color_array);
}

int is_fiber_runable(fiber_t* fiber){
  pthread_spin_lock(&scheduler_spinlock);
  lock_stats_t lock_stat;
  uint64_t fib_colour = fiber->bitcolour;
  LinkedList* locks = get_locks(fiber);
  if (fib_colour == 0){
    pthread_spin_unlock(&scheduler_spinlock);
    return 1;
  }
  // if (fiber_colour_scheduling_check(fib_colour) == 0) { 
  if (!try_update_colour(fib_colour)) { 
    // try_free_expired_slices(locks);
    pthread_spin_unlock(&scheduler_spinlock);
    return 0;  // Not runsable 
  }

  struct timeval now;
  gettimeofday(&now, NULL);

  Node* current = locks->head;
  while (current != NULL) {
    if (get(lock_fiber_d, (void*)fiber, current->value, &lock_stat)) {
      if (timercmp(&now, &lock_stat.banned_until, <)) { // The Fiber is Banned from Using at least one Lock
        reset_colour_scheduling_fiber(fib_colour);
        pthread_spin_unlock(&scheduler_spinlock);
        return 0;
      }
    }
    current = current->next;
  }
  pthread_spin_unlock(&scheduler_spinlock);
  return 1;
}

void try_free_expired_slices(LinkedList* locks) {
  fiber_t* holder;
  struct timeval now;
  gettimeofday(&now, NULL);

  Node* current = locks->head;
  while ((current!= NULL) && (current->value != NULL)) {
      // Assume each lock pointer is a pointer to a sched_lock_t structure.
      sched_lock_t* sched_lock = (sched_lock_t*)current->value;

      holder = sched_lock->holder; // The lock should be held if we want to clear it 
      if (holder == NULL){
        current = current->next;
        continue; // Can do optimisation where we only go through conflicting colours #TODO
      }
      if ((timercmp(&now, &sched_lock->slice_end_time, >)) && (sched_lock->lock_held == 0)){ // This lock_held check prevents us from premting the lock in the critical section
          // Mark the slice as expired.
          sched_lock->slice_acquired = 0;
          sched_lock->holder = NULL;
          ban_fibers((void*)sched_lock, holder);
          int lock_index = get_lock_index((void*)sched_lock);
          remove_locks(holder, (void*)sched_lock);
          holder->bitcolour &= ~(1 << lock_index); // reset the lock usage, assume lock is unwanted
          colours &= ~(1 << lock_index); //reset and release slice
      }
      current = current->next;
  }
}

fiber_t* fiber_scheduler_next(fiber_scheduler_t* sched, fiber_t * current_fiber) {
  fiber_scheduler_wsd_t* const scheduler = (fiber_scheduler_wsd_t*)sched;
  assert(scheduler);
  if (wsd_work_stealing_deque_size(scheduler->schedule_from) == 0) {
    wsd_work_stealing_deque_t* const temp = scheduler->schedule_from;
    scheduler->schedule_from = scheduler->store_to;
    scheduler->store_to = temp;
  }

  while (wsd_work_stealing_deque_size(scheduler->schedule_from) > 0) {
    fiber_t* const new_fiber =
        (fiber_t*)wsd_work_stealing_deque_pop_bottom(scheduler->schedule_from);
    if (new_fiber != WSD_EMPTY && new_fiber != WSD_ABORT) {
      if (new_fiber->state == FIBER_STATE_SAVING_STATE_TO_WAIT) {
        wsd_work_stealing_deque_push_bottom(scheduler->store_to, new_fiber);
      } 
      else if (!is_fiber_runable(new_fiber)){
        wsd_work_stealing_deque_push_bottom(scheduler->store_to, new_fiber);
      }
      
      // else if (!try_update_colour(new_fiber->bitcolour)){
      //   wsd_work_stealing_deque_push_bottom(scheduler->store_to, new_fiber);
      // }

      else {
        // update_color_scheduling(new_fiber->bitcolour);
        return new_fiber;
      }
    }
  }
  return NULL;
}

void fiber_scheduler_load_balance(fiber_scheduler_t* sched) {
  fiber_scheduler_wsd_t* const scheduler = (fiber_scheduler_wsd_t*)sched;
  size_t max_steal = 50;
  size_t i = 2 * (scheduler->id + 1);
  const size_t end = i + 2 * (fiber_scheduler_num_threads - 1);
  const size_t mod = 2 * fiber_scheduler_num_threads;
  size_t local_count = wsd_work_stealing_deque_size(scheduler->schedule_from);
  for (; i < end; ++i) {
    const size_t index = i % mod;
    wsd_work_stealing_deque_t* const remote_queue =
        fiber_scheduler_thread_queues[index];
    assert(remote_queue != scheduler->queue_one);
    assert(remote_queue != scheduler->queue_two);
    if (!remote_queue) {
      continue;
    }
    size_t remote_count = wsd_work_stealing_deque_size(remote_queue);
    while (remote_count > local_count && max_steal > 0) {
      fiber_t* const stolen =
          (fiber_t*)wsd_work_stealing_deque_steal(remote_queue);
      if (stolen == WSD_EMPTY || stolen == WSD_ABORT) {
        ++scheduler->failed_steal_count;
        break;
      }
      wsd_work_stealing_deque_push_bottom(scheduler->schedule_from, stolen);
      --remote_count;
      ++local_count;
      --max_steal;
      ++scheduler->steal_count;
    }
  }
}

void fiber_scheduler_stats(fiber_scheduler_t* sched, uint64_t* steal_count,
                           uint64_t* failed_steal_count) {
  fiber_scheduler_wsd_t* const scheduler = (fiber_scheduler_wsd_t*)sched;
  assert(scheduler);
  *steal_count += scheduler->steal_count;
  *failed_steal_count += scheduler->failed_steal_count;
}
