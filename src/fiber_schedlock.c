#include "fiber_schedlock.h"

void sched_lock_init(struct sched_lock *lock)
{
    lock->start_ticks = (struct timeval){0, 0};
    lock->end_ticks = (struct timeval){0, 0};
    lock->slice_end_time = (struct timeval){0, 0};
    lock->slice_acquired = 0; // can be used to track is lock is held 
    lock->lock_stat = malloc(sizeof(lock_stats_t));
    lock->lock_stat->banned_until = (struct timeval){0, 0};
    lock->lock_stat->slice_size = (struct timeval){0, 0};
    lock->lock_held = 0;
    lock->holder = NULL;
    lock->num_holders = 0;
    // fiber_spinlock_init(&lock->spinlock);
}

void sched_lock_acquire(struct sched_lock *lock)
{
    // Assuming that only one fiber can request acquire at a time 


    // Colour if UnColoured and Record Lock.
    record_lock_for_fiber((void*)lock, SLICE_SIZE_US, NULL);
    if (lock->slice_acquired == 0){
        // Record the start time.
        gettimeofday(&lock->start_ticks, NULL);
        
        // Retrieve the fiber's lock statistics.
        if (get_lock_fiber_data((void*)lock, lock->lock_stat, NULL) == 0){
            printf("Error: No Stats, something is wrong.\n");
            abort();
        }
        // Compute the slice end time.
        timeval_add(&lock->slice_end_time, &lock->start_ticks, &lock->lock_stat->slice_size);
        lock->slice_acquired = 1;
        lock->holder = fiber_manager_get()->current_fiber;
    }
    // else{
    //     gettimeofday(&lock->end_ticks, NULL); // we use the last possible end-ticks 
    //     if (timercmp(&lock->end_ticks, &lock->slice_end_time,>)){ // enter if slice has expired
    //         // lock->holder = NULL;
    //         // ban_fibers(lock, NULL);
    //         // lock->lock_held = 0;
    //         // lock->slice_acquired = 0;
    //         fiber_yield(0); // Yield to Allow Others to get resources
    //     }
    // }
    lock->lock_held = 1;
}

void sched_lock_release(struct sched_lock *lock)
{

    gettimeofday(&lock->end_ticks, NULL); // we use the last possible end-ticks 
    if (timercmp(&lock->end_ticks, &lock->slice_end_time,>)){ // enter if slice has expired
        // lock->slice_acquired = 0;
        // lock->holder = NULL;
        // ban_fibers(lock, NULL);
        // lock->lock_held = 0;
        fiber_yield(1); // Yield to Allow Others to get resources
        return;
    }
    lock->lock_held = 0;
    return;
}

void ban_fibers(struct sched_lock *lock , fiber_t* fiber){
    struct timeval time_adder;
    struct timeval banned_until;
    unsigned long long cs_length;
    int nthreads = atomic_load(&lock->num_holders);
    if (lock->holder != fiber){
      return;
    } else if ((nthreads > 1) && (lock->start_ticks.tv_sec > 0) &&
               (lock->end_ticks.tv_sec > 0)) {  // don't ban in the start
      /* Expand ban tvime by (cs_length * num_threads). */
      cs_length = time_difference(&lock->start_ticks, &lock->end_ticks);
      time_adder.tv_sec = (cs_length * (nthreads - 1)) / 1000000ULL;
      time_adder.tv_usec = (cs_length * (nthreads - 1)) % 1000000ULL;
    //   printf("Fiber %p is banned for %lld u-secs \n start_ticks %ld s and %ld usecs \n" 
    //     "end_ticks %ld s and %ld u-secs \n",
    //     fiber, cs_length, lock->start_ticks.tv_sec, lock->start_ticks.tv_usec, lock->end_ticks.tv_sec, lock->end_ticks.tv_usec);
 

      timeval_add(&banned_until, &lock->end_ticks, &time_adder);
      set_lock_fiber_data((void *)lock, banned_until,
                          (struct timeval){0, SLICE_SIZE_US}, fiber);
    } else {
      /* If only one fiber, no ban needed. */
      set_lock_fiber_data((void *)lock, lock->end_ticks,
                          (struct timeval){0, SLICE_SIZE_US}, fiber);
    }
}