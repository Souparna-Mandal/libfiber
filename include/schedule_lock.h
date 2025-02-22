#ifndef _SCHEDULER_LOCK_H_
#define _SCHEDULER_LOCK_H_

#include <stdlib.h>
#include <stdatomic.h>
#include "fiber.h"

extern atomic_int lock_using;
extern fiber_t* lock_holder_f;

extern void add_lock();
extern void init_scheduler_lock_data();
extern int is_lock_using();
extern void set_lock_status(int n, fiber_t* lock_holder);

#endif