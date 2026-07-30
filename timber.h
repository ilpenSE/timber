/* SPDX-License-Identifier: MIT */
#ifndef TIMBER_H
#define TIMBER_H

#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdarg.h>

/* C/C++ version checking */
#ifdef __cplusplus
#if __cplusplus < 201103L
  #error "timber.h requires minimum C++11"
#endif
#define TIMBER_INLINE inline
#else /* C */
/* Inline keyword support in <=C89 */
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
  #define TIMBER_INLINE inline /* Use keyword directly in >=C99 */
#elif defined(_MSC_VER)
  #define TIMBER_INLINE __inline /* MSVC C89 extension */
#elif defined(__GNUC__) || defined(__clang__)
  #define TIMBER_INLINE __inline__ /* GNU C89 extension */
#else
  #define TIMBER_INLINE
#endif
#endif

/* Define TIMBER_RELEASE or TIMBER_NDEBUG for release builds */
#if !defined(TIMBER_RELEASE) && !defined(TIMBER_NDEBUG)
  #define TIMBER_DEBUG 1
#endif

/*
  Version of Timber
  Format: MAJOR_MINOR_PATCH_L (concat these)
  Minor and patch should be padded with 0 if they're 1 digit long.
*/
#define TIMBER_VER 10000L
#define TIMBER_MAJOR 1
#define TIMBER_MINOR 0
#define TIMBER_PATCH 0

/* Maximum amount of sinks in an instance */
#define TIMBER_MAX_SINKS 8
#define TIMBER_MAX_MSG_SIZE 256
#define TIMBER_QUEUE_SIZE 1024
typedef struct Timber Timber;

/* Log Levels you can add another one like we did if you wish */
#define TIMBER_LEVELS \
  X(info, INFO) \
  X(error, ERROR) \
  X(warning, WARNING)

typedef enum {
  #define X(_, name) TIMBER_##name,
  TIMBER_LEVELS
  #undef X
  _TimberLevel_count,
} TimberLevel;

typedef enum {
  TIMBER_DROP_POLICY,
  TIMBER_BLOCK_POLICY,
  _TimberPolicy_count,
} TimberPolicy;

#ifdef __cplusplus
  #define TIMBER_EXTERN extern "C"
#else
  #define TIMBER_EXTERN extern
#endif

#ifdef _WIN32
  #define TIMBER_DLLEXPORT __declspec(dllexport)
  #define TIMBER_DLLIMPORT __declspec(dllimport)
#else
  #define TIMBER_DLLEXPORT __attribute__((visibility("default")))
  #define TIMBER_DLLIMPORT
#endif

#ifdef TIMBER_SHARED
  #ifdef TIMBER_BUILD
    #define TIMBER_API TIMBER_DLLEXPORT TIMBER_EXTERN
  #else
    #define TIMBER_API TIMBER_DLLIMPORT TIMBER_EXTERN
  #endif
#else
  #define TIMBER_API TIMBER_EXTERN
#endif

/* portable printf-format style checker (only available on gcc and clang) */
#if defined(__clang__) || defined(__GNUC__)
  #define TIMBER_PRINTF_LIKE(fmt, args) __attribute__((format(printf, fmt, args)))
#else
  #define TIMBER_PRINTF_LIKE(fmt, args)
#endif

TIMBER_API Timber *timber_alloc(void);
TIMBER_API bool timber_init(Timber *lg);
TIMBER_API bool timber_logn(Timber *lg, TimberLevel level, const char *msg, size_t msgsz);
TIMBER_API bool timber_log(Timber *lg, TimberLevel level, const char *msg);
TIMBER_API bool timber_logf(Timber *lg, TimberLevel level, const char *fmt, ...) TIMBER_PRINTF_LIKE(3, 4);
TIMBER_API bool timber_vlogf(Timber *lg, TimberLevel level, const char *fmt, va_list args) TIMBER_PRINTF_LIKE(3, 0);
TIMBER_API bool timber_destroy(Timber *lg);
TIMBER_API bool timber_free(Timber *lg);
TIMBER_API const char *timber_level_to_cstr(TimberLevel level);
TIMBER_API bool timber_add_file_sink(Timber *lg, const char *file_path);
TIMBER_API bool timber_add_stdout_sink(Timber *lg);
TIMBER_API bool timber_add_stderr_sink(Timber *lg);
TIMBER_API void timber_set_policy(Timber *lg, TimberPolicy policy);
TIMBER_API void timber_set_format(Timber *lg, const char *format);

/*
  Level-specific log functions that're generated at compilation
  You cannot use them in FFI because they marked as "static inline"
  and they aren't in dynamic symbol table or has external linkage
  Use them in C or C++ code which uses this library directly.
  What you can do in FFI is write your own bindings for level-specific
  function if you wanna omit levels (you already wanna write/generate bindings)
  Generated function names are like this: (for example: info)
    timber_infof(Timber *lg, const char *fmt, ...);
    timber_infon(Timber *lg, const char *msg, size_t msgsz);
    timber_info(Timber *lg, const char *msg);
  - timber_levelf -> timber_vlogf
  - timber_level  -> timber_logn with calling strlen
  - timber_leveln -> timber_logn
*/
#define X(lower, upper)                                                                                 \
static TIMBER_INLINE bool timber_##lower##f(Timber *lg, const char *fmt, ...) TIMBER_PRINTF_LIKE(2, 3); \
static TIMBER_INLINE bool timber_##lower(Timber *lg, const char *msg);                                  \
static TIMBER_INLINE bool timber_##lower##n(Timber *lg, const char *msg, size_t msgsz);
TIMBER_LEVELS
#undef X

#ifdef TIMBER_IMPLEMENTATION
#include <assert.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

#define TIMBER_TODO(fmt, ...) \
  do { \
    fprintf(stderr, "%s:%d: TODO: " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__); \
    abort(); \
  } while (0)

#define TIMBER_PRIVATE static

#ifdef __cplusplus
#include <atomic>
#define TIMBER_ATOMIC(T) ::std::atomic<T>
#define timber_atomic_init ::std::atomic_init
#define timber_morder_relaxed ::std::memory_order_relaxed
#define timber_morder_acquire ::std::memory_order_acquire
#define timber_morder_release ::std::memory_order_release
#define timber_atomic_store ::std::atomic_store_explicit
#define timber_atomic_load ::std::atomic_load_explicit
#define timber_atomic_cas_weak ::std::atomic_compare_exchange_weak_explicit
#else
#include <stdatomic.h>
#define TIMBER_ATOMIC(T) _Atomic(T)
#define timber_atomic_init atomic_init
#define timber_morder_relaxed memory_order_relaxed
#define timber_morder_acquire memory_order_acquire
#define timber_morder_release memory_order_release
#define timber_atomic_store atomic_store_explicit
#define timber_atomic_load atomic_load_explicit
#define timber_atomic_cas_weak atomic_compare_exchange_weak_explicit
#endif

// POSIX/Windows semaphore and pthread abstraction
#ifdef _WIN32
#include <windows.h>
#include <process.h>
typedef struct TimberThreadCtx {
  void *(*start_routine)(void*);
  void *arg;
  void *retval;
  HANDLE handle;
} timber_pthread_t;

typedef HANDLE timber_sem_t;
typedef SECURITY_ATTRIBUTES timber_pthread_attr_t;
typedef HANDLE timber_fd_t;
#define timber_sched_yield SwitchToThread

TIMBER_PRIVATE TIMBER_INLINE bool timber_sem_init(timber_sem_t *sem, int pshared, unsigned int value) {
  (void)pshared;
  *sem = CreateSemaphore(NULL, value, LONG_MAX, NULL);
  return *sem != NULL;
}
TIMBER_PRIVATE TIMBER_INLINE bool timber_sem_wait(timber_sem_t *sem) {
  return WaitForSingleObject(*sem, INFINITE) == WAIT_OBJECT_0;
}
TIMBER_PRIVATE TIMBER_INLINE bool timber_sem_trywait(timber_sem_t *sem) {
  return WaitForSingleObject(*sem, INFINITE) == WAIT_OBJECT_0;
}
TIMBER_PRIVATE TIMBER_INLINE bool timber_sem_post(timber_sem_t *sem) {
  return ReleaseSemaphore(*sem, 1, NULL) != 0;
}
TIMBER_PRIVATE TIMBER_INLINE bool timber_sem_destroy(timber_sem_t *sem) {
  return CloseHandle(*sem) != 0;
}

TIMBER_PRIVATE unsigned __stdcall _timber_consumer_trampoline(void *cp) {
  timber_pthread_t *ctx = (timber_pthread_t *)cp;
  ctx->retval = ctx->start_routine(ctx->arg);
  return 0;
}

TIMBER_PRIVATE TIMBER_INLINE bool timber_pthread_create(
                                         timber_pthread_t *pthread,
                                         const timber_pthread_attr_t *attr,
                                         void *(*start_routine)(void *),
                                         void *arg)
{
  pthread->start_routine = start_routine;
  pthread->arg = arg;
  uintptr_t h = _beginthreadex(attr, 0, _timber_consumer_trampoline, (void*)pthread, 0, NULL);
  if (h == 0) return false;
  pthread->handle = (HANDLE)h;
  return true;
}

TIMBER_PRIVATE TIMBER_INLINE bool timber_pthread_join(timber_pthread_t *pthread, void **retval) {
  if (WaitForSingleObject(pthread->handle, INFINITE) != WAIT_OBJECT_0) return false;
  if (!CloseHandle(pthread->handle)) return false;
  *retval = pthread->retval;
  return true;
}

TIMBER_PRIVATE WCHAR *_timber_win32_utf8_to_wide(const char *str) {
  int len = MultiByteToWideChar(CP_UTF8, 0, str, -1, NULL, 0);
  WCHAR *buf = (WCHAR *)malloc((len + 1) * sizeof(WCHAR));
  if (!buf) { errno = ENOMEM; return NULL; }
  MultiByteToWideChar(CP_UTF8, 0, str, -1, buf, len);
  return buf;
}

TIMBER_PRIVATE int _timber_win32_error_to_cerrno() {
  int err;
  switch (GetLastError()) {
  case ERROR_FILE_NOT_FOUND:
  case ERROR_PATH_NOT_FOUND:
    err = ENOENT; break;
  case ERROR_ACCESS_DENIED:
    err = EACCES; break;
  case ERROR_ALREADY_EXISTS:
  case ERROR_FILE_EXISTS:
    err = EEXIST; break;
  case ERROR_INVALID_NAME:
  case ERROR_BAD_PATHNAME:
    err = EINVAL; break;
  case ERROR_TOO_MANY_OPEN_FILES:
    err = EMFILE; break;
  case ERROR_DISK_FULL:
    err = ENOSPC; break;
  case ERROR_NOT_READY:
    err = ENODEV; break;
  case ERROR_DIRECTORY:
    err = ENOTDIR; break;
  case ERROR_CANT_RESOLVE_FILENAME: // symlink loop
    err = ELOOP; break;
  default:
    err = EIO;
  }
  return err;
}

#else // POSIX
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <semaphore.h>
#include <sched.h>

typedef pthread_t timber_pthread_t;
typedef sem_t timber_sem_t;
typedef int timber_fd_t;

#define timber_sched_yield sched_yield

#define timber_sem_init(s, pshared, v) (sem_init((s), (pshared), (v)) == 0)
#define timber_sem_wait(s) (sem_wait((s)) == 0)
#define timber_sem_post(s) (sem_post((s)) == 0)
#define timber_sem_destroy(s) (sem_destroy((s)) == 0)
#define timber_sem_trywait(s) (sem_trywait((s)) == 0)

#define timber_pthread_create(thread, attr, start_routine, arg) \
  (pthread_create((thread), (attr), (start_routine), (arg)) == 0)
#define timber_pthread_join(thread, retval) \
  (pthread_join(*(thread), (retval)) == 0)
#endif // _WIN32

struct TimberPayload {
  char msg[TIMBER_MAX_MSG_SIZE];
  size_t msg_count;
  TimberLevel level;
};

struct TimberSlot {
  struct TimberPayload payload;
  TIMBER_ATOMIC(size_t) seq;
};

struct TimberQueue {
  struct TimberSlot items[TIMBER_QUEUE_SIZE];
  TIMBER_ATOMIC(size_t) head;
  size_t tail;
};

struct Timber {
  TIMBER_ATOMIC(bool) is_alive;
  timber_pthread_t thread;
  // the signal which is used for producers to signal consumer
  timber_sem_t sem_full_slots;
  // the signal which is used for consumer signals to producers for an empty slot (only for TIMBER_BLOCK_POLICY)
  timber_sem_t sem_empty_slots;
  timber_fd_t sinks[TIMBER_MAX_SINKS]; // array of fds/HANDLEs
  size_t sink_count;
  struct TimberQueue queue;
  TimberPolicy log_policy;
  const char *format;
  // TODO: Add format string and parsing
};

#ifdef TIMBER_DEBUG
#define _timber_debug_err(fmt, ...) \
  do { \
    fprintf(stderr, "%s:%d: [TIMBER] DEBUG/ERROR: " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__); \
  } while (0)
#define _timber_debug(fmt, ...) \
  do { \
    printf("%s:%d: [TIMBER] DEBUG/INFO: " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__); \
  } while (0)
#define _timber_report_error(function) \
  do { \
    _timber_debug_err(function " failed: %s", strerror(errno)); \
  } while (0)
#else
#define _timber_debug_err(fmt, ...) (void)0
#define _timber_debug(fmt, ...) (void)0
#define _timber_report_error(function) (void)(function)
#endif

TIMBER_PRIVATE void *_timber_consumer(void *ctxptr) {
  _timber_debug("thread initialized, sleeping");
  struct Timber *ctx = (struct Timber *)ctxptr;
  struct TimberQueue *q = &ctx->queue;
#ifdef TIMBER_DEBUG
  size_t processed = 0;
#endif

  // Event loop
  while (1) {
    // wait for ready messages
    if (!timber_sem_wait(&ctx->sem_full_slots)) {
      _timber_report_error("sem_wait in consumer");
      return NULL;
    }
    size_t head = timber_atomic_load(&q->head, timber_morder_relaxed);
    if (head == q->tail) {
      if (!timber_atomic_load(&ctx->is_alive, timber_morder_relaxed)) break;
      continue;
    }

    // Process payload
    // TODO: Add batch writing and formating
    size_t pos = q->tail;
    struct TimberSlot *slot = &q->items[pos % TIMBER_QUEUE_SIZE];
    size_t spins = 0;
    while (timber_atomic_load(&slot->seq, timber_morder_acquire) != pos + 1) {
      // spin-wait for some time then yield this thread to another runnables
      // until the slot is ready (one of producers has wrote a valid payload)
      // timber_sched_yield expanded to sched_yield on POSIX, SwitchToThread() on windows
      if (spins++ > 64) timber_sched_yield();
    }

    struct TimberPayload payload = slot->payload;
    timber_atomic_store(&slot->seq, pos + TIMBER_QUEUE_SIZE, timber_morder_release);
    q->tail++;
    if (ctx->log_policy == TIMBER_BLOCK_POLICY) timber_sem_post(&ctx->sem_empty_slots);

    for (size_t i = 0; i < ctx->sink_count; i++) {
      timber_fd_t fd = ctx->sinks[i];
      const char *level_str = timber_level_to_cstr(payload.level);

      char buffer[1024];
      int n = snprintf(buffer, sizeof(buffer), "%s: %.*s\n", level_str, (int)payload.msg_count, payload.msg);
      if (n < 0) {
        errno = EINVAL;
        _timber_report_error("snprintf failed in consumer");
        return NULL;
      } else if ((size_t)n >= sizeof(buffer)) {
        n = sizeof(buffer) - 1;
        buffer[sizeof(buffer) - 2] = '\n';
      }

#ifdef _WIN32
      DWORD written;
      WriteFile(fd, buffer, (DWORD)n, &written, NULL);
#else
      write(fd, buffer, n);
#endif
    }

#ifdef TIMBER_DEBUG
    processed++;
#endif
  }

#ifdef TIMBER_DEBUG
  _timber_debug("Exiting consumer thread, stats:");
  size_t remaining = q->tail - timber_atomic_load(&q->head, timber_morder_relaxed);
  _timber_debug("total unprocessed %zu messages", remaining);
  _timber_debug("total processed %zu messages", processed);
#endif
  return (void *)1;
}

bool timber_vlogf(Timber *lg, TimberLevel level, const char *fmt, va_list args) {
  char buf[4096];
  int n = vsnprintf(buf, sizeof(buf), fmt, args);
  if (n < 0) {
    errno = EINVAL;
    return false;
  }

  size_t len = (size_t)n < sizeof(buf) ? (size_t)n : sizeof(buf) - 1;
  return timber_logn(lg, level, buf, len);
}

bool timber_logf(Timber *lg, TimberLevel level, const char *fmt, ...) {
  va_list args; va_start(args, fmt);
  bool ok = timber_vlogf(lg, level, fmt, args);
  va_end(args);
  return ok;
}

bool timber_log(Timber *lg, TimberLevel level, const char *msg) {
  return timber_logn(lg, level, msg, strlen(msg));
}

bool timber_logn(Timber *lg, TimberLevel level, const char *msg, size_t msgsz) {
  if (!timber_atomic_load(&lg->is_alive, timber_morder_relaxed)) {
    errno = EPIPE;
    return false;
  }
  if (msgsz > TIMBER_MAX_MSG_SIZE) {
    errno = EINVAL;
    return false;
  }

  // CAS loop for claiming slot
  size_t pos;
  struct TimberSlot *slot;
  struct TimberQueue *q = &lg->queue;
  for (;;) {
    pos = timber_atomic_load(&q->head, timber_morder_relaxed);
    slot = &q->items[pos % TIMBER_QUEUE_SIZE];
    size_t seq = timber_atomic_load(&slot->seq, timber_morder_acquire);
    if (seq == pos) {
      if (timber_atomic_cas_weak(&q->head, &pos, pos + 1, timber_morder_relaxed, timber_morder_relaxed))
        break; // this producer claimed this slot
      continue; // this producer couldn't claim this slot, try again
    } else if (seq < pos) {
      // queue full, decide what to do by policy
      if (lg->log_policy == TIMBER_BLOCK_POLICY) {
        if (!timber_sem_wait(&lg->sem_empty_slots)) {
          _timber_report_error("sem_wait(sem_empty_slots)");
          return false;
        }
      } else { errno = ENOBUFS; return false; }
    }
  }

  struct TimberPayload *pyld = &slot->payload;
  pyld->msg_count = msgsz;
  pyld->level = level;
  memcpy(pyld->msg, msg, msgsz);

  // set slot's seq == pos + 1 (ready signal for consumer)
  // and signal consumer thread
  timber_atomic_store(&slot->seq, pos + 1, timber_morder_release);
  if (!timber_sem_post(&lg->sem_full_slots)) {
    _timber_report_error("sem_post(sem_full_slots)");
    return false;
  }
  return true;
}

bool timber_init(Timber *lg) {
  if (!lg) {
    errno = EINVAL;
    return false;
  }
  int ret;
  lg->is_alive = true;

  for (size_t i = 0; i < TIMBER_QUEUE_SIZE; ++i)
    timber_atomic_init(&lg->queue.items[i].seq, i);

  if (!timber_sem_init(&lg->sem_full_slots, 0, 0)) {
    _timber_report_error("sem_init(sem_full_slots)");
    goto fail;
  }

  if (lg->log_policy == TIMBER_BLOCK_POLICY) {
    if (!timber_sem_init(&lg->sem_empty_slots, 0, TIMBER_QUEUE_SIZE)) {
      _timber_report_error("sem_init(sem_empty_slots)");
      goto fail_sem_full;
    }
  }

  if (!timber_pthread_create(&lg->thread, NULL, _timber_consumer, lg)) {
    _timber_report_error("pthread_create");
    goto fail_sem_empty;
  }

  return true;
fail_sem_empty:
  if (lg->log_policy == TIMBER_BLOCK_POLICY) {
    ret = timber_sem_destroy(&lg->sem_empty_slots);
    assert(ret && "timber_sem_destroy failed (sem_empty_slots)");
  }
fail_sem_full:
  ret = timber_sem_destroy(&lg->sem_full_slots);
  assert(ret && "timber_sem_destroy failed (sem_full_slots)");
fail:
  lg->is_alive = false;
  return false;
}

bool timber_destroy(Timber *lg) {
  timber_atomic_store(&lg->is_alive, false, timber_morder_relaxed);
  if (!timber_sem_post(&lg->sem_full_slots)) {
    _timber_report_error("sem_post");
    return false;
  }
  void *thread_retval;
  if (!timber_pthread_join(&lg->thread, &thread_retval)) {
    _timber_report_error("pthread_join");
    return false;
  }
  if (thread_retval == NULL) {
    _timber_report_error("consumer thread");
  }

  if (!timber_sem_destroy(&lg->sem_full_slots)) {
    _timber_report_error("sem_destroy(sem_full_slots)");
    return false;
  }
  if (!timber_sem_destroy(&lg->sem_empty_slots)) {
    _timber_report_error("sem_destroy(sem_empty_slots)");
    return false;
  }

  for (size_t i = 0; i < lg->sink_count; i++) {
    timber_fd_t sink = lg->sinks[i];
    #ifdef _WIN32
    if (sink == GetStdHandle(STD_OUTPUT_HANDLE) || sink == GetStdHandle(STD_ERROR_HANDLE)) continue;
    CloseHandle(sink);
    #else
    if (sink == STDOUT_FILENO || sink == STDERR_FILENO) continue;
    close(sink);
    #endif
  }
  return true;
}

Timber *timber_alloc(void) {
  void *ptr = calloc(1, sizeof(struct Timber));
  if (!ptr) {
    errno = ENOMEM;
    return NULL;
  }
  return (Timber *)ptr;
}
bool timber_free(Timber *lg) {
  if (!lg) return false;
  free(lg);
  return true;
}

bool timber_add_file_sink(Timber *lg, const char *file_path) {
  if (lg->sink_count >= TIMBER_MAX_SINKS) {
    errno = ERANGE;
    return false;
  }
  timber_fd_t fd;

#ifdef _WIN32
  WCHAR *wfile_path = _timber_win32_utf8_to_wide(file_path);
  if (!wfile_path) { return false; }
  fd = CreateFileW(wfile_path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
  free(wfile_path);
  if (fd == INVALID_HANDLE_VALUE) {
    errno = _timber_win32_error_to_cerrno();
    _timber_report_error("OpenFile");
    return false;
  }
#else
  fd = open(file_path, O_WRONLY | O_CREAT, 0640);
  if (fd < 0) {
    _timber_report_error("open");
    return false;
  }
#endif

  lg->sinks[lg->sink_count++] = fd;
  return true;
}

bool timber_add_stdout_sink(Timber *lg) {
  if (lg->sink_count >= TIMBER_MAX_SINKS) {
    errno = ERANGE;
    return false;
  }
#ifdef _WIN32
  timber_fd_t stdout_fd = GetStdHandle(STD_OUTPUT_HANDLE);
  if (stdout_fd == INVALID_HANDLE_VALUE) {
    errno = _timber_win32_error_to_cerrno();
    return false;
  }
#else
  timber_fd_t stdout_fd = STDOUT_FILENO;
#endif
  lg->sinks[lg->sink_count++] = stdout_fd;
  return true;
}

bool timber_add_stderr_sink(Timber *lg) {
  if (lg->sink_count >= TIMBER_MAX_SINKS) {
    errno = ERANGE;
    return false;
  }

#ifdef _WIN32
  timber_fd_t stderr_fd = GetStdHandle(STD_ERROR_HANDLE);
  if (stderr_fd == INVALID_HANDLE_VALUE) {
    errno = _timber_win32_error_to_cerrno();
    return false;
  }
#else
  timber_fd_t stderr_fd = STDERR_FILENO;
#endif

  lg->sinks[lg->sink_count++] = stderr_fd;
  return true;
}

void timber_set_policy(Timber *lg, TimberPolicy policy) {
  if (!lg) return;
  lg->log_policy = policy;
}

void timber_set_format(Timber *lg, const char *format) {
  if (!lg) return;
  lg->format = format;
}

#ifdef _MSC_VER
#define _TIMBER_UNREACHABLE(fmt, ...)            \
  do {                                           \
    fprintf(stderr, "%s:%d: UNREACHABLE: " fmt " \n", \
      __FILE__, __LINE__, ##__VA_ARGS__);        \
    __assume(0);                                 \
  } while (0)
#else
#define _TIMBER_UNREACHABLE(fmt, ...)            \
  do {                                           \
    fprintf(stderr, "%s:%d: UNREACHABLE: " fmt " \n", \
      __FILE__, __LINE__, ##__VA_ARGS__);        \
    __builtin_unreachable();                     \
  } while (0)
#endif

const char *timber_level_to_cstr(TimberLevel level) {
  switch(level) {
#define X(_, name) case TIMBER_##name: return #name;
TIMBER_LEVELS
#undef X
  default: return "";
  }
  _TIMBER_UNREACHABLE("timber_level_to_cstr");
}

#define X(lower, upper)                                                                  \
static TIMBER_INLINE bool timber_##lower##f(Timber *lg, const char *fmt, ...) { \
  va_list args; va_start(args, fmt);                                                     \
  bool ok = timber_vlogf(lg, TIMBER_##upper, fmt, args);                                 \
  va_end(args);                                                                          \
  return ok;                                                                             \
}                                                                                        \
static TIMBER_INLINE bool timber_##lower(Timber *lg, const char *msg) {                  \
  return timber_logn(lg, TIMBER_##upper, msg, strlen(msg));                              \
}                                                                                        \
static TIMBER_INLINE bool timber_##lower##n(Timber *lg, const char *msg, size_t msgsz) { \
  return timber_logn(lg, TIMBER_##upper, msg, msgsz);                                    \
}
TIMBER_LEVELS
#undef X

#endif /* TIMBER_IMPLEMENTATION */
#endif /* TIMBER_H */
