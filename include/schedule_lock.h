#ifndef _SCHEDULER_LOCK_H_
#define _SCHEDULER_LOCK_H_

#include <stdlib.h>

extern int lock_using;

extern void add_lock();
extern void init_scheduler_lock_data();
extern int is_lock_using();
extern void set_lock_status();

#endif