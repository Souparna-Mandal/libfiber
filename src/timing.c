#include "timing.h"

// Returns the difference between t1 and t0 in microseconds
unsigned long long time_diff(struct timeval t0, struct timeval t1) {
    return (unsigned long long)((t1.tv_sec - t0.tv_sec) * 1000000 + (t1.tv_usec - t0.tv_usec));
}

// Adds two timeval structures and stores the result in 'result'
void timeval_add(struct timeval *result, const struct timeval *t1, const struct timeval *t2) {
    result->tv_sec = t1->tv_sec + t2->tv_sec;
    result->tv_usec = t1->tv_usec + t2->tv_usec;

    if (result->tv_usec >= 1000000) { // if microseconds exceed 10^6 then convert to seconds
        result->tv_sec += result->tv_usec / 1000000;
        result->tv_usec %= 1000000;
    }
}

// Converts a struct timeval to an unsigned long long (microseconds)
unsigned long long timeval_to_ull(const struct timeval t) {
    return ((unsigned long long)t.tv_sec * 1000000ULL) + t.tv_usec;
}

// Converts an unsigned long long (microseconds) to a struct timeval
struct timeval ull_to_timeval(unsigned long long usec) {
    struct timeval t;
    t.tv_sec = usec / 1000000;
    t.tv_usec = usec % 1000000;
    return t;
}

