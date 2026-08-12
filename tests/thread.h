#ifndef THREAD_H
#define THREAD_H

typedef void *(*thread_routine_t)(void*);

#ifdef _WIN32
#include <windows.h>
#include <process.h>
typedef SECURITY_ATTRIBUTES thread_attr_t;

typedef struct {
  thread_routine_t start_routine;
  void *arg;
  void *retval;
  HANDLE handle;
} thread_t;

static inline unsigned __stdcall _thread_trampoline(void *cp) {
  thread_t *ctx = (thread_t *)cp;
  ctx->retval = ctx->start_routine(ctx->arg);
  return 0;
}

static inline bool thread_create(thread_t *th,
                                         const thread_attr_t *attr,
                                         thread_routine_t start_routine,
                                         void *arg)
{
  th->start_routine = start_routine;
  th->arg = arg;
  uintptr_t h = _beginthreadex(
    (thread_attr_t*)attr, 0, _thread_trampoline, (void*)th, 0, NULL
  );
  if (h == 0) return false;
  th->handle = (HANDLE)h;
  return true;
}

static inline bool thread_join(thread_t *th, void **retval) {
  if (WaitForSingleObject(th->handle, INFINITE) != WAIT_OBJECT_0) return false;
  if (!CloseHandle(th->handle)) return false;
  *retval = th->retval;
  return true;
}

#else // POSIX
#include <pthread.h>
typedef pthread_t thread_t;
typedef pthread_attr_t thread_attr_t;

static inline int thread_create(thread_t *th,
                                const thread_attr_t *attr,
                                thread_routine_t start_routine,
                                void *arg)
{ return pthread_create(th, attr, start_routine, arg); }

static inline int thread_join(thread_t *th, void **retval)
{ return pthread_join(*th, retval); }

#endif

#endif // THREAD_H
