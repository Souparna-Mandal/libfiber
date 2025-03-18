#ifndef _SCHEDULER_LOCK_H_
#define _SCHEDULER_LOCK_H_

#include <stdlib.h>
#include <stdatomic.h>
#include "fiber.h"

// each bit represnts if lock with id bit_no is being used 
extern atomic_int lock_using; 

// array of fibers marking the lock holders 
extern fiber_t* lock_holder_f[64] ; 

extern void add_lock();
extern void init_scheduler_lock_data();
extern int is_lock_using();
extern void set_lock_status(int n, fiber_t* lock_holder);

#endif