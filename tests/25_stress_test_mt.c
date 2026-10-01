#include <timber.h>
#include <inttypes.h>
#include "thread.h"
#include "benchmark.h"

#define THREAD_COUNT 10
#define MESSAGES_PER_THREAD 100000
Timber *timber;

struct ThreadCtx {
  thread_t id;
  int64_t elapsed_ns;
  size_t dropped;
};

void *thread_func(void *cp) {
  struct ThreadCtx *ctx = (struct ThreadCtx *)cp;
#if _WIN32
  #define PRIthid "p"
  void *id = ctx->id.handle;
#else
  #define PRIthid "lu"
  thread_t id = ctx->id;
#endif

  struct timespec start, end;
  volatile size_t dropped = 0;

  get_monotonic_time(&start);
  for (volatile long i = 0; i < MESSAGES_PER_THREAD; i++) {
    if (!timber_infof(timber, "[Thread %" PRIthid ", %ld] Hello, World!", id, i)) {
      dropped++;
    }
  }
  get_monotonic_time(&end);

  int64_t elapsed_ns = (int64_t)(end.tv_sec - start.tv_sec) * 1000000000LL
                       + (int64_t)(end.tv_nsec - start.tv_nsec);
  printf("[Thread %" PRIthid "] Total elapsed time = %" PRId64 " ns, %lf ns/call\n",
          id, elapsed_ns, (double)elapsed_ns/MESSAGES_PER_THREAD);
  printf("[Thread %" PRIthid "] Dropped messages: %zu\n", id, dropped);
  ctx->elapsed_ns = elapsed_ns;
  ctx->dropped = dropped;
  return NULL;
}

int main(void) {
  timber = timber_alloc();
  if (!timber_init(timber)) return 1;
  struct ThreadCtx threads[THREAD_COUNT] = {0};

  for (size_t i = 0; i < sizeof(threads)/sizeof(*threads); i++) {
    thread_create(&threads[i].id, NULL, thread_func, &threads[i]);
  }

  for (size_t i = 0; i < sizeof(threads)/sizeof(*threads); i++) {
    thread_join(&threads[i].id, NULL);
  }

  size_t total_dropped = 0;
  int64_t total_elapsed_ns = 0;
  for (size_t i = 0; i < sizeof(threads)/sizeof(*threads); i++) {
    total_dropped += threads[i].dropped;
    total_elapsed_ns += threads[i].elapsed_ns;
  }

  char buf[1024*1024];
  printf("Total elapsed: %s ns\n", fmt_thousands(total_elapsed_ns));
  printf("Total elapsed time per call: %s ns\n", fmt_thousandsf((double)total_elapsed_ns/(MESSAGES_PER_THREAD*THREAD_COUNT), 2));
  printf("Total dropped messages: %s\n", fmt_thousands(total_dropped));
  double throughput = (double)(MESSAGES_PER_THREAD * THREAD_COUNT) / (total_elapsed_ns * 1e-9);
  printf("Throughput: %s log/sec\n", fmt_thousandsf(throughput, 2));

  if (!timber_destroy(timber)) return 2;
  return 0;
}
