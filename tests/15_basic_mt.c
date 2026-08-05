#include <timber.h>
#include <stdio.h>
#include <pthread.h>

Timber timber;
#define THREAD_COUNT 10
#define MESSAGES_PER_THREAD 20
pthread_t threads[THREAD_COUNT];

void *thread_func(void *ctx) {
  size_t id = (size_t)ctx;
  for (size_t i = 0; i < MESSAGES_PER_THREAD; i++) {
    timber_infof(&timber, "[Thread %zu, %zu] Hello, World!", id, i);
  }
  return NULL;
}

int main(void) {
  if (!timber_init(&timber)) return 1;
  timber_add_stdout_sink(&timber);

  printf("Creating %d threads\n", THREAD_COUNT);
  for (size_t i = 0; i < THREAD_COUNT; i++) {
    pthread_create(&threads[i], NULL, thread_func, (void*)i);
  }

  printf("Joining %d threads\n", THREAD_COUNT);
  for (int i = 0; i < THREAD_COUNT; i++) {
    pthread_join(threads[i], NULL);
  }

  if (!timber_destroy(&timber)) return 2;
}
