#include "timing.h"

unsigned long long  time_difference(struct timeval *t0, struct timeval *t1) {
    return (unsigned long long  )((t1->tv_sec - t0->tv_sec) * 1000000 + (t1->tv_usec - t0->tv_usec));
}


void timeval_add(struct timeval *result, const struct timeval *t1, const struct timeval *t2) {
    result->tv_sec = t1->tv_sec + t2->tv_sec;
    result->tv_usec = t1->tv_usec + t2->tv_usec;

    if (result->tv_usec >= 1000000) { // if microseconds exceed 10^6 then covert to seconds
        result->tv_sec += result->tv_usec / 1000000;
        result->tv_usec %= 1000000;
    }
}
