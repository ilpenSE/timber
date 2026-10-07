#include <timber.h>
#include <inttypes.h>
#include "benchmark.h"

#define MESSAGES 1000000

int main(void) {
  Timber *timber = timber_alloc();
#if 0
  timber_set_policy(timber, TIMBER_BLOCK_POLICY);
#endif
  if (!timber_init(timber)) return 1;
  struct timespec start, end;
  volatile size_t dropped = 0;

  get_monotonic_time(&start);
  for (long i = 0; i < MESSAGES; i++) {
    if (!timber_info(timber, "Hello, World!")) {
      dropped++;
    }
  }
  get_monotonic_time(&end);

  int64_t elapsed_ns = (int64_t)(end.tv_sec - start.tv_sec) * 1000000000LL
                       + (int64_t)(end.tv_nsec - start.tv_nsec);
  printf("Total elapsed time = %s ns\n", fmt_thousands(elapsed_ns));
  printf("%s ns/call\n", fmt_thousandsf((double)elapsed_ns/MESSAGES, 2));
  printf("Dropped messages: %s\n", fmt_thousands(dropped));
  double throughput = (double)(MESSAGES - dropped) / (elapsed_ns * 1e-9);
  printf("Throughput: %s logs/sec\n", fmt_thousandsf(throughput, 2));

  if (!timber_destroy(timber)) return 2;
  timber_free(timber);
  return 0;
}
