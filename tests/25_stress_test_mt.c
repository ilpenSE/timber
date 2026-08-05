#include <timber.h>
#include <time.h>
#include <inttypes.h>

#define THREAD_COUNT 10
#define MESSAGES_PER_THREAD 100000
Timber timber = {0};

struct ThreadCtx {
  pthread_t id;
  int64_t elapsed_ns;
  size_t dropped;
};

void *thread_func(void *cp) {
  struct ThreadCtx *ctx = (struct ThreadCtx *)cp;
  struct timespec start, end;
  volatile size_t dropped = 0;

  clock_gettime(CLOCK_MONOTONIC, &start);
  for (volatile long i = 0; i < MESSAGES_PER_THREAD; i++) {
    if (!timber_infof(&timber, "[Thread %lu, %ld] Hello, World!", ctx->id, i)) {
      dropped++;
    }
  }
  clock_gettime(CLOCK_MONOTONIC, &end);

  int64_t elapsed_ns = (int64_t)(end.tv_sec - start.tv_sec) * 1000000000LL
                       + (int64_t)(end.tv_nsec - start.tv_nsec);
  printf("[Thread %lu] Total elapsed time = %" PRId64 " ns, %lf ns/call\n",
          ctx->id, elapsed_ns, (double)elapsed_ns/MESSAGES_PER_THREAD);
  printf("[Thread %lu] Dropped messages: %zu\n", ctx->id, dropped);
  ctx->elapsed_ns = elapsed_ns;
  ctx->dropped = dropped;
  return NULL;
}

int main(void) {
  timber.log_policy = TIMBER_DROP_POLICY;
  if (!timber_init(&timber)) return 1;
  struct ThreadCtx threads[THREAD_COUNT] = {0};

  for (size_t i = 0; i < sizeof(threads)/sizeof(*threads); i++) {
    pthread_create(&threads[i].id, NULL, thread_func, &threads[i]);
  }

  for (size_t i = 0; i < sizeof(threads)/sizeof(*threads); i++) {
    pthread_join(threads[i].id, NULL);
  }

  size_t total_dropped = 0;
  int64_t total_elapsed_ns = 0;
  for (size_t i = 0; i < sizeof(threads)/sizeof(*threads); i++) {
    total_dropped += threads[i].dropped;
    total_elapsed_ns += threads[i].elapsed_ns;
  }

  printf("Total elapsed: %" PRId64" ns\n", total_elapsed_ns);
  printf("Total elapsed time per call: %lf ns\n", (double)total_elapsed_ns/(MESSAGES_PER_THREAD*THREAD_COUNT));
  printf("Total dropped messages: %zu\n", total_dropped);

  if (!timber_destroy(&timber)) return 2;
  return 0;
}
