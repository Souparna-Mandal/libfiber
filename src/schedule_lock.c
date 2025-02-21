#include <stdatomic.h>
#include "schedule_lock.h"

atomic_int lock_using;

void add_lock(){
    // If lock_using is -1, then atomically set it to 0.
    int expected = -1;
    atomic_compare_exchange_strong(&lock_using, &expected, 0);
}

void set_lock_status(int n) {
    atomic_store(&lock_using, n);
}

void init_scheduler_lock_data() { 
    atomic_store(&lock_using, -1);
}

int is_lock_using() {
    return atomic_load(&lock_using);
}