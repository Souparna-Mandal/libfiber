#ifndef _TIMING_H_
#define _TIMING_H_

#include <time.h>
#include <sys/time.h>

extern unsigned long long time_difference(struct timeval *t0, struct timeval *t1);
extern void timeval_add(struct timeval *result, const struct timeval *t1,
                 const struct timeval *t2);


#endif