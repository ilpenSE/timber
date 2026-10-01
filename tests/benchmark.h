#ifndef BENCHMARK_H
#define BENCHMARK_H

#include <stdint.h>
#include <stdio.h>

#define THOUSANDS_SEP '.'
#define DECIMAL_SEP   ','
static char g_temp[1024*1024];

static inline const char *fmt_thousandsf(double v, int decimals)
{
  char tmp[64];
  int len = snprintf(tmp, sizeof tmp, "%.*f", decimals, v);

  // çok büyük sayı (tmp'ye sığmadı) ya da bozuk durum: ayraçsız yaz
  if (len < 0 || len >= (int)sizeof tmp) {
    snprintf(g_temp, sizeof(g_temp), "%.*f", decimals, v);
    return g_temp;
  }

  int i = 0;
  size_t out = 0;
  if (tmp[0] == '-') i++;

  int int_end = i;
  while (tmp[int_end] >= '0' && tmp[int_end] <= '9') int_end++;
  int int_len = int_end - i;

  // nan / inf: olduğu gibi bas
  if (int_len == 0) {
    snprintf(g_temp, sizeof(g_temp), "%s", tmp);
    return g_temp;
  }

  if (i && out < sizeof(g_temp) - 1) g_temp[out++] = '-';

  for (int k = 0; k < int_len; k++) {
    if (k > 0 && (int_len - k) % 3 == 0 && out < sizeof(g_temp) - 1) g_temp[out++] = THOUSANDS_SEP;
    if (out < sizeof(g_temp) - 1) g_temp[out++] = tmp[i + k];
  }

  // ondalık kısım ("." ile başlar)
  for (int k = int_end; tmp[k] && out < sizeof(g_temp) - 1; k++)
    g_temp[out++] = (tmp[k] == '.') ? DECIMAL_SEP : tmp[k];

  g_temp[out] = '\0';
  return g_temp;
}

static inline const char *fmt_thousands(uint64_t n)
{
  char tmp[32];
  int len = snprintf(tmp, sizeof(tmp), "%llu", (unsigned long long)n);
  int out = 0;
  for (int i = 0; i < len && out < (int)(sizeof(g_temp)) - 2; i++) {
    if (i > 0 && (len - i) % 3 == 0) g_temp[out++] = '.';
    g_temp[out++] = tmp[i];
  }
  g_temp[out] = '\0';
  return g_temp;
}

#ifdef _WIN32
#include <windows.h>

#ifndef _TIMESPEC_DEFINED
#define _TIMESPEC_DEFINED
struct timespec {
  time_t tv_sec;
  long   tv_nsec;
};
#endif

static inline void get_monotonic_time(struct timespec *ts)
{
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
