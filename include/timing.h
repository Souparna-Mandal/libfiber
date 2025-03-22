#ifndef _TIMING_H_
#define _TIMING_H_

#include <time.h>
#include <sys/time.h>

// Returns the difference between t1 and t0 in microseconds
extern unsigned long long time_diff(struct timeval t0, struct timeval t1);

// Adds two timeval structures and stores the result in 'result'
extern void timeval_add(struct timeval *result, const struct timeval *t1,
                 const struct timeval *t2);

// Converts a struct timeval to an unsigned long long (microseconds)
extern unsigned long long timeval_to_ull(const struct timeval t);

// Converts an unsigned long long (microseconds) to a struct timeval
extern struct timeval ull_to_timeval(unsigned long long usec);

#endif
