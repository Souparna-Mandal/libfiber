#include "schedule_lock.h"
#include "fiber_rwlock.h"

fiber_rwlock_t lock;
int lock_using ;

void add_lock(){
    /* Logic to allocate hashtable entry for new lock*/
    int val;
    fiber_rwlock_rdlock(&lock);
    val = lock_using;
    fiber_rwlock_rdunlock(&lock);

    if (val <0 ){ // add lock only if it hasnt been added before
        fiber_rwlock_wrlock(&lock);
        lock_using = 0; // sets this globally
        fiber_rwlock_wrlock(&lock);
    }
}

void set_lock_status() {
  fiber_rwlock_wrlock(&lock);
  lock_using = 1;
  fiber_rwlock_wrunlock(&lock);
}

void init_scheduler_lock_data() { 
    lock_using = -1; 
   }

int is_lock_using() {
  int val;
  fiber_rwlock_rdlock(&lock);
  val = lock_using;
  fiber_rwlock_rdunlock(&lock);
  return val;
}  // we can pass in a particular lock we want toi look up
