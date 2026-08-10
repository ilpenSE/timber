#ifndef BENCHMARK_H
#define BENCHMARK_H

#ifdef _WIN32
#include <windows.h>

#ifndef _TIMESPEC_DEFINED
#define _TIMESPEC_DEFINED
struct timespec {
  time_t tv_sec;
  long   tv_nsec;
};
#endif

static inline void get_monotonic_time(struct timespec *ts) {
  static LARGE_INTEGER freq;
  LARGE_INTEGER counter;
  if (freq.QuadPart == 0) QueryPerformanceFrequency(&freq);
  QueryPerformanceCounter(&counter);
  ts->tv_sec =  counter.QuadPart / freq.QuadPart;
  ts->tv_nsec = (long)((counter.QuadPart % freq.QuadPart) * 1000000000LL / freq.QuadPart);
}

#else
#include <time.h>
static inline void get_monotonic_time(struct timespec *ts) {
  clock_gettime(CLOCK_MONOTONIC, ts);
}
#endif

#endif // BENCHMARK_H
