#include <timber.h>
#include <time.h>
#include <inttypes.h>

#define MESSAGES 1000000

int main(void) {
  Timber *timber = timber_alloc();
  timber_set_policy(timber, TIMBER_DROP_POLICY);
  if (!timber_init(timber)) return 1;
  struct timespec start, end;
  volatile size_t dropped = 0;

  clock_gettime(CLOCK_MONOTONIC, &start);
  for (volatile long i = 0; i < MESSAGES; i++) {
    if (!timber_info(timber, "Hello, World!")) {
      dropped++;
    }
  }
  clock_gettime(CLOCK_MONOTONIC, &end);

  int64_t elapsed_ns = (int64_t)(end.tv_sec - start.tv_sec) * 1000000000LL
                       + (int64_t)(end.tv_nsec - start.tv_nsec);
  printf("Total elapsed time = %" PRId64 " ns, ", elapsed_ns);
  printf("%lf ns/call\n", (double)elapsed_ns/MESSAGES);
  printf("Dropped messages: %zu\n", dropped);

  if (!timber_destroy(timber)) return 2;
  timber_free(timber);
  return 0;
}
