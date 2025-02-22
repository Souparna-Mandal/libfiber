#include <stdatomic.h>
#include "schedule_lock.h"

atomic_int lock_using;
fiber_t* lock_holder_f;

void add_lock(){
    // If lock_using is -1, then atomically set it to 0.
    int expected = -1;
    atomic_compare_exchange_strong(&lock_using, &expected, 0);
}

void set_lock_status(int n, fiber_t* fiber) {
    if (n==0){
        if(fiber == lock_holder_f){
            atomic_store(&lock_using, n);
        }
    }
    else if (n==1){
        atomic_store(&lock_using, n);
        lock_holder_f = fiber;
    }
}

void init_scheduler_lock_data() { 
    atomic_store(&lock_using, -1);
}

int is_lock_using() {
    return atomic_load(&lock_using);
}