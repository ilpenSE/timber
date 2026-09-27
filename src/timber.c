#include <timber.h>
#include <assert.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdalign.h>
#include <time.h>

#define TIMBER_TODO(fmt, ...)             \
  do {                                    \
    fprintf(stderr, "%s:%d: TODO: " fmt " \n", \
      __FILE__, __LINE__, ##__VA_ARGS__); \
    exit(134);                            \
  } while (0)

#ifdef _MSC_VER
  #include <intrin.h>
  #if defined(_M_X64) || defined(_M_IX86)
    #define _TIMBER_PAUSE _mm_pause() // MSVC X86/X64 intrinsic
  #elif defined(_M_ARM64)
    #define _TIMBER_PAUSE __yield()  // MSVC ARM64 intrinsic
  #else
    #error "Unsupported MSVC platform for pause instruction"
  #endif
#else
  #if defined(__x86_64__) || defined(__i386__) // amd64/x86_64 or i386/x86/x86_32
    #define _TIMBER_PAUSE __asm__ volatile("pause" ::: "memory")
  #elif defined(__aarch64__) // ARM64
    #define _TIMBER_PAUSE __asm__ volatile("yield" ::: "memory")
  #else
    #error "No such supported platform for pause instruction"
  #endif
#endif

#ifdef __cplusplus
#include <atomic>
#define TIMBER_ATOMIC(T) ::std::atomic<T>
#define timber_atomic_init ::std::atomic_init
#define timber_morder_relaxed ::std::memory_order_relaxed
#define timber_morder_acquire ::std::memory_order_acquire
#define timber_morder_release ::std::memory_order_release
#define timber_atomic_store ::std::atomic_store_explicit
#define timber_atomic_load ::std::atomic_load_explicit
#define timber_atomic_fetch_add ::std::atomic_fetch_add_explicit
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
#define timber_atomic_fetch_add atomic_fetch_add_explicit
#define timber_atomic_cas_weak atomic_compare_exchange_weak_explicit
#endif

// POSIX/Windows abstraction
typedef void *(*timber_thread_fn_t)(void*);
#ifdef _WIN32
#include <windows.h>
#include <process.h>

typedef struct TimberThreadCtx {
  timber_thread_fn_t start_routine;
  void *arg;
  void *retval;
  HANDLE handle;
} timber_pthread_t;

typedef struct {
  void  *iov_base;
  size_t iov_len;
} timber_iovec;

typedef HANDLE timber_sem_t;
typedef SECURITY_ATTRIBUTES timber_pthread_attr_t;
typedef CRITICAL_SECTION timber_mutex_t;
typedef CONDITION_VARIABLE timber_cond_t;
typedef void timber_mutexattr_t;
typedef void timber_condattr_t;
typedef HANDLE timber_fd_t;

// Mutexes
static inline bool timber_mutex_init(timber_mutex_t *mutex, const timber_mutexattr_t *attr)
{ InitializeCriticalSection(mutex); return true; }

static inline bool timber_mutex_lock(timber_mutex_t *mutex)
{ EnterCriticalSection(mutex); return true; }

static inline bool timber_mutex_unlock(timber_mutex_t *mutex)
{ LeaveCriticalSection(mutex); return true; }

static inline bool timber_mutex_destroy(timber_mutex_t *mutex)
{ DeleteCriticalSection(mutex); return true; }

// Condition variables
static inline bool timber_cond_init(timber_cond_t *cond, timber_condattr_t *cond_attr)
{ InitializeConditionVariable(cond); return true; }

static inline bool timber_cond_broadcast(timber_cond_t *cond)
{ WakeAllConditionVariable(cond); return true; }

static inline bool timber_cond_wait(timber_cond_t *cond, timber_mutex_t *mutex)
{ return SleepConditionVariableCS(cond, mutex, INFINITE) != 0; }

static inline bool timber_cond_destroy(timber_cond_t *cond)
{ (void)cond; return true; }

static inline bool timber_cond_signal(timber_cond_t *cond)
{ WakeConditionVariable(cond); return true; }

// Semaphores
static inline bool timber_sem_init(timber_sem_t *sem, int pshared, unsigned int value) {
  (void)pshared;
  *sem = CreateSemaphore(NULL, value, LONG_MAX, NULL);
  return *sem != NULL;
}

static inline bool timber_sem_wait(timber_sem_t *sem)
{ return WaitForSingleObject(*sem, INFINITE) == WAIT_OBJECT_0; }

static inline bool timber_sem_trywait(timber_sem_t *sem)
{ return WaitForSingleObject(*sem, INFINITE) == WAIT_OBJECT_0; }

static inline bool timber_sem_post(timber_sem_t *sem)
{ return ReleaseSemaphore(*sem, 1, NULL) != 0; }

static inline bool timber_sem_destroy(timber_sem_t *sem)
{ return CloseHandle(*sem) != 0; }

// Pthreads
static inline unsigned __stdcall _timber_consumer_trampoline(void *cp) {
  timber_pthread_t *ctx = (timber_pthread_t *)cp;
  ctx->retval = ctx->start_routine(ctx->arg);
  return 0;
}

static inline bool timber_pthread_create(timber_pthread_t *pthread,
                                         const timber_pthread_attr_t *attr,
                                         timber_thread_fn_t start_routine,
                                         void *arg)
{
  pthread->start_routine = start_routine;
  pthread->arg = arg;
  uintptr_t h = _beginthreadex(
    (timber_pthread_attr_t*)attr, 0, _timber_consumer_trampoline, (void*)pthread, 0, NULL
  );
  if (h == 0) return false;
  pthread->handle = (HANDLE)h;
  return true;
}

static inline bool timber_pthread_join(timber_pthread_t *pthread, void **retval) {
  if (WaitForSingleObject(pthread->handle, INFINITE) != WAIT_OBJECT_0) return false;
  if (!CloseHandle(pthread->handle)) return false;
  if (retval) *retval = pthread->retval;
  return true;
}

static WCHAR *_timber_win32_utf8_to_wide(const char *str) {
  int len = MultiByteToWideChar(CP_UTF8, 0, str, -1, NULL, 0);
  WCHAR *buf = (WCHAR *)malloc((len + 1) * sizeof(WCHAR));
  if (!buf) { errno = ENOMEM; return NULL; }
  MultiByteToWideChar(CP_UTF8, 0, str, -1, buf, len);
  return buf;
}

static int _timber_win32_error_to_cerrno() {
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
#include <sys/uio.h>

typedef pthread_t timber_pthread_t;
typedef pthread_attr_t timber_pthread_attr_t;
typedef pthread_mutex_t timber_mutex_t;
typedef pthread_cond_t timber_cond_t;
typedef pthread_mutexattr_t timber_mutexattr_t;
typedef pthread_condattr_t timber_condattr_t;
typedef sem_t timber_sem_t;
typedef int timber_fd_t;
typedef struct iovec timber_iovec;

// Mutexes
static inline bool timber_mutex_init(timber_mutex_t *mutex, const timber_mutexattr_t *attr)
{ return pthread_mutex_init(mutex, attr) == 0; }

static inline bool timber_mutex_lock(timber_mutex_t *mutex)
{ return pthread_mutex_lock(mutex) == 0; }

static inline bool timber_mutex_unlock(timber_mutex_t *mutex)
{ return pthread_mutex_unlock(mutex) == 0; }

static inline bool timber_mutex_destroy(timber_mutex_t *mutex)
{ return pthread_mutex_destroy(mutex) == 0; }

// Condition variables
static inline bool timber_cond_init(timber_cond_t *cond, timber_condattr_t *cond_attr)
{ return pthread_cond_init(cond, cond_attr) == 0; }

static inline bool timber_cond_broadcast(timber_cond_t *cond)
{ return pthread_cond_broadcast(cond) == 0; }

static inline bool timber_cond_wait(timber_cond_t *cond, timber_mutex_t *mutex)
{ return pthread_cond_wait(cond, mutex) == 0; }

static inline bool timber_cond_destroy(timber_cond_t *cond)
{ return pthread_cond_destroy(cond) == 0; }

static inline bool timber_cond_signal(timber_cond_t *cond)
{ return pthread_cond_signal(cond) == 0; }

// Semaphores
static inline bool timber_sem_init(timber_sem_t *s, int pshared, unsigned int v)
{ return sem_init(s, pshared, v) == 0; }

static inline bool timber_sem_wait(timber_sem_t *s)
{
  int r;
  do { r = sem_wait(s); } while (r != 0 && errno == EINTR);
  return r == 0;
}

static inline bool timber_sem_post(timber_sem_t *s)
{ return sem_post(s) == 0; }

static inline bool timber_sem_destroy(timber_sem_t *s)
{ return sem_destroy(s) == 0; }

static inline bool timber_sem_trywait(timber_sem_t *s)
{ return sem_trywait(s) == 0; }

// Pthreads
static inline bool timber_pthread_create(timber_pthread_t *thread,
                                         const timber_pthread_attr_t *attr,
                                         timber_thread_fn_t start_routine,
                                         void *arg)
{ return pthread_create(thread, attr, start_routine, arg) == 0; }

static inline bool timber_pthread_join(timber_pthread_t *thread, void **retval)
{ return pthread_join(*thread, retval) == 0; }
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
  alignas(64) TIMBER_ATOMIC(size_t) head;
  alignas(64) TIMBER_ATOMIC(size_t) tail;
};

typedef enum {
  _TIMBER_TK_LITERAL = 0,
  _TIMBER_TK_TIME,
  _TIMBER_TK_LEVEL,
  _TIMBER_TK_MESSAGE,
  _TimberTokenType_count,
} TimberTokenType;

struct TimberToken {
  TimberTokenType type;
  const char *lit;
  size_t lit_count;
};

struct TimberFormat {
  struct TimberToken tokens[TIMBER_MAX_TOKENS];
  size_t token_count;
};

struct TimberSink {
  const char *file_path;
  timber_fd_t fd;
};

struct Timber {
  alignas(64) TIMBER_ATOMIC(bool) is_alive;
  struct TimberQueue queue;
  struct TimberFormat format;
  timber_pthread_t thread;
  struct TimberSink sinks[TIMBER_MAX_SINKS]; // array of fds/HANDLEs
  size_t sink_count;
  // the signal which is used for producers to signal consumer
  timber_sem_t sem_full_slots;
  // the signal which is used for consumer signals to producers for an empty slot (only for TIMBER_BLOCK_POLICY)
  timber_sem_t sem_empty_slots;
  // mutex and condvar for flush() barrier
  timber_mutex_t mtx_flush;
  timber_cond_t  cond_flush;
  TimberPolicy log_policy;
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

// Helper for adding token to TimberFormat
static bool _timber_add_fmt_token(struct TimberFormat *format,
                                  TimberTokenType type,
                                  const char *lit, size_t lit_count)
{
  if (format->token_count >= TIMBER_MAX_TOKENS) return false;
  format->tokens[format->token_count++] = (struct TimberToken){type, lit, lit_count};
  return true;
}

static void _timber_append_buf(char *out, size_t *outcnt, size_t outsz, const char *in, size_t incnt)
{
  if (*outcnt + incnt > outsz) incnt = outsz - *outcnt;
  memcpy(out + (*outcnt), in, incnt);
  *outcnt += incnt;
}

// Manual writes for _timber_get_time
static inline void _timber_time_write2(char* p, int v) {
  p[0] = (char)('0' + v / 10); p[1] = (char)('0' + v % 10);
}
static inline void _timber_time_write4(char* p, int v) {
  _timber_time_write2(p, v / 100); _timber_time_write2(p + 2, v % 100);
}
static inline void _timber_time_write3(char* p, int v) {
  p[0] = (char)('0' + v / 100);
  p[1] = (char)('0' + (v / 10) % 10);
  p[2] = (char)('0' + v % 10);
}

static size_t _timber_get_time(char *buf, size_t bufsz)
{
  static _Thread_local struct tm cached_tm;
  static _Thread_local time_t cached_now;

  time_t now = time(0);
  if (now != cached_now) {
#ifdef _WIN32
    if (localtime_s(&cached_tm, &now) != 0) return 0;
#else
    if (!localtime_r(&now, &cached_tm)) return 0;
#endif
    cached_now = now;
  }

  // 14-08-2026 20:45:00
  size_t n = 0;
  _timber_time_write2(buf + n, cached_tm.tm_mday); n += 2;
  buf[n++] = '-';
  _timber_time_write2(buf + n, cached_tm.tm_mon + 1); n += 2;
  buf[n++] = '-';
  _timber_time_write4(buf + n, cached_tm.tm_year + 1900); n += 4;
  buf[n++] = ' ';
  _timber_time_write2(buf + n, cached_tm.tm_hour); n += 2;
  buf[n++] = ':';
  _timber_time_write2(buf + n, cached_tm.tm_min); n += 2;
  buf[n++] = ':';
  _timber_time_write2(buf + n, cached_tm.tm_sec); n += 2;
  return n;
}

static size_t _timber_format_msg(struct TimberFormat *format,
                                 struct TimberPayload *payload,
                                 char *out, size_t outsz)
{
  size_t outcnt = 0;
  const char *level_str = timber_level_to_cstr(payload->level);
  size_t level_cnt = strlen(level_str);

  for (size_t i = 0; i < format->token_count; i++) {
    struct TimberToken tok = format->tokens[i];
    switch (tok.type) {
    case _TIMBER_TK_TIME: {
      char buffer[24];
      size_t n = _timber_get_time(buffer, sizeof(buffer));
      _timber_append_buf(out, &outcnt, outsz, buffer, n);
    } break;
    case _TIMBER_TK_LEVEL: {
      _timber_append_buf(out, &outcnt, outsz, level_str, level_cnt);
    } break;
    case _TIMBER_TK_MESSAGE: {
      _timber_append_buf(out, &outcnt, outsz, payload->msg, payload->msg_count);
    } break;
    case _TIMBER_TK_LITERAL: {
      _timber_append_buf(out, &outcnt, outsz, tok.lit, tok.lit_count);
    } break;
    default: return -1;
    }
  }

  out[outcnt++] = '\n';
  return outcnt;
}

static void *_timber_consumer(void *ctxptr)
{
  _timber_debug("thread initialized");
  struct Timber *ctx = (struct Timber *)ctxptr;
  struct TimberQueue *q = &ctx->queue;
#ifdef TIMBER_DEBUG
  size_t processed = 0;
#endif
  char buffers[TIMBER_MAX_BATCH][TIMBER_MAX_MSG_SIZE + TIMBER_FORMAT_EXTRA + 1];
  timber_iovec vecs[TIMBER_MAX_BATCH];
  int vec_count = 0;

  // Event loop
  while (1) {
    // wait for ready messages
    bool ret = timber_sem_wait(&ctx->sem_full_slots);
    assert(ret && "sem_wait");
    size_t head = timber_atomic_load(&q->head, timber_morder_relaxed);
    size_t tail = timber_atomic_load(&q->tail, timber_morder_relaxed);
    if (head == tail) {
      if (!timber_atomic_load(&ctx->is_alive, timber_morder_relaxed)) break;
      continue;
    }

    // Process payload
    size_t start_pos = timber_atomic_load(&q->tail, timber_morder_relaxed);
    size_t batch_count = 0;
    while (batch_count < TIMBER_MAX_BATCH) {
      size_t idx = start_pos + batch_count;
      struct TimberSlot *slot = &q->items[idx%TIMBER_QUEUE_SIZE];
      if (timber_atomic_load(&slot->seq, timber_morder_acquire) != idx + 1) {
        break; // slot is not ready yet
      }
      batch_count++;
    }

    vec_count = 0;
    for (size_t i = 0; i < batch_count; i++) {
      struct TimberSlot *s = &q->items[(start_pos + i)%TIMBER_QUEUE_SIZE];
      struct TimberPayload payload = s->payload;
      timber_atomic_store(&s->seq, start_pos + i + TIMBER_QUEUE_SIZE, timber_morder_release);
      if (ctx->log_policy == TIMBER_BLOCK_POLICY) timber_sem_post(&ctx->sem_empty_slots);

      size_t count = _timber_format_msg(&ctx->format, &payload, buffers[i], sizeof(buffers[i]));
      if (count == -1) {
        _timber_debug_err("[Consumer] Couldn't format a message, dropping it");
        continue;
      }
      vecs[vec_count++] = (timber_iovec){buffers[i], count};
    }

    for (size_t i = 0; i < ctx->sink_count; i++) {
      struct TimberSink *sink = &ctx->sinks[i];
#ifdef _WIN32
      // Windows doesn't deserve gather/scatter IO because none of the games on earth uses it
      // And also, no one cares about scatter IO amongst Windows users because they're all gamers
      for (size_t i = 0; i < vec_count; i++) {
        timber_iovec vec = vecs[i];
        BOOL ok = WriteFile(sink->fd, vec.iov_base, vec.iov_len, NULL, NULL);
        #ifdef TIMBER_DEBUG
        if (!ok) {
          _timber_debug_err("WriteFile");
          continue;
        }
        #endif
      }

#else // POSIX
      ssize_t n = writev(sink->fd, vecs, vec_count);
      #ifdef TIMBER_DEBUG
      if (n < 0) {
        _timber_debug_err("writev");
        continue;
      }
      #endif
#endif
    }

    timber_atomic_fetch_add(&q->tail, batch_count, timber_morder_release);
    timber_mutex_lock(&ctx->mtx_flush);
    timber_cond_broadcast(&ctx->cond_flush);
    timber_mutex_unlock(&ctx->mtx_flush);

#ifdef TIMBER_DEBUG
    processed += batch_count;
#endif
  }

#ifdef TIMBER_DEBUG
  _timber_debug("Exiting consumer thread, stats:");
  size_t head = timber_atomic_load(&q->head, timber_morder_relaxed);
  size_t tail = timber_atomic_load(&q->tail, timber_morder_relaxed);
  size_t remaining = head - tail;
  _timber_debug("total unprocessed %zu messages", remaining);
  _timber_debug("total processed %zu messages", processed);
#endif
  return (void *)1;
}

bool timber_flush(Timber *lg)
{
  if (!lg) return false;
  if (!timber_atomic_load(&lg->is_alive, timber_morder_relaxed)) return false;
  size_t head = timber_atomic_load(&lg->queue.head, timber_morder_acquire);
  timber_mutex_lock(&lg->mtx_flush);
  while (timber_atomic_load(&lg->queue.tail, timber_morder_acquire) < head) {
    timber_cond_wait(&lg->cond_flush, &lg->mtx_flush);
  }
  timber_mutex_unlock(&lg->mtx_flush);
  return true;
}

bool timber_vlogf(Timber *lg, TimberLevel level, const char *fmt, va_list args)
{
  char buf[4096];
  int n = vsnprintf(buf, sizeof(buf), fmt, args);
  if (n < 0) { errno = EINVAL; return false; }

  size_t len = (size_t)n < sizeof(buf) ? (size_t)n : sizeof(buf) - 1;
  return timber_logn(lg, level, buf, len);
}

bool timber_logf(Timber *lg, TimberLevel level, const char *fmt, ...)
{
  va_list args; va_start(args, fmt);
  bool ok = timber_vlogf(lg, level, fmt, args);
  va_end(args);
  return ok;
}

bool timber_log(Timber *lg, TimberLevel level, const char *msg)
{
  return timber_logn(lg, level, msg, strlen(msg));
}

bool timber_logn(Timber *lg, TimberLevel level, const char *msg, size_t msgsz)
{
  if (!timber_atomic_load(&lg->is_alive, timber_morder_relaxed)) {
    errno = EPIPE; return false;
  }

  // Truncate if it's too big
  if (msgsz > TIMBER_MAX_MSG_SIZE) {
    msgsz = TIMBER_MAX_MSG_SIZE;
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
        bool ret = timber_sem_wait(&lg->sem_empty_slots);
        assert(ret && "sem_wait on producer CAS loop");
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
  bool ret = timber_sem_post(&lg->sem_full_slots);
  assert(ret && "sem_post(sem_full_slots)");
  return true;
}

bool timber_init(Timber *lg)
{
  if (!lg) { errno = EINVAL; return false; }
  bool ret;
  lg->is_alive = true;

  // Create semaphores (error when resources aren't available)
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

  // Create thread (error when resources aren't available)
  if (!timber_pthread_create(&lg->thread, NULL, _timber_consumer, lg)) {
    _timber_report_error("pthread_create");
    goto fail_sem_empty;
  }

  // Initialize atomics (no errors)
  timber_atomic_init(&lg->queue.head, 0);
  for (size_t i = 0; i < TIMBER_QUEUE_SIZE; ++i)
    timber_atomic_init(&lg->queue.items[i].seq, i);

  // Initialize mutex and condvar (shouldn't error out)
  ret = timber_mutex_init(&lg->mtx_flush, NULL);
  assert(ret && "mutex_init");
  ret = timber_cond_init(&lg->cond_flush, NULL);
  assert(ret && "cond_init");

  // Default format style: $T [$L] $M
  if (lg->format.token_count == 0) {
    _timber_add_fmt_token(&lg->format, _TIMBER_TK_TIME, NULL, 0);
    _timber_add_fmt_token(&lg->format, _TIMBER_TK_LITERAL, " [", 2);
    _timber_add_fmt_token(&lg->format, _TIMBER_TK_LEVEL, NULL, 0);
    _timber_add_fmt_token(&lg->format, _TIMBER_TK_LITERAL, "] ", 2);
    _timber_add_fmt_token(&lg->format, _TIMBER_TK_MESSAGE, NULL, 0);
  }

  return true;
fail_sem_empty:
  if (lg->log_policy == TIMBER_BLOCK_POLICY) {
    timber_sem_destroy(&lg->sem_empty_slots);
  }
fail_sem_full:
  timber_sem_destroy(&lg->sem_full_slots);
fail:
  lg->is_alive = false;
  return false;
}

bool timber_destroy(Timber *lg)
{
  if (!lg) return false;
  bool all_ok = true;

  timber_atomic_store(&lg->is_alive, false, timber_morder_relaxed);
  all_ok &= timber_sem_post(&lg->sem_full_slots);

  void *thread_retval;
  all_ok &= timber_pthread_join(&lg->thread, &thread_retval);

#ifdef TIMBER_DEBUG
  if (thread_retval == NULL) _timber_report_error("consumer thread");
#endif

  all_ok &= timber_sem_destroy(&lg->sem_full_slots);

  if (lg->log_policy == TIMBER_BLOCK_POLICY) {
    all_ok &= timber_sem_destroy(&lg->sem_empty_slots);
  }

  all_ok &= timber_mutex_destroy(&lg->mtx_flush);
  all_ok &= timber_cond_destroy(&lg->cond_flush);

  for (size_t i = 0; i < lg->sink_count; i++) {
    struct TimberSink *sink = &lg->sinks[i];
    #ifdef _WIN32
    if (sink->fd == GetStdHandle(STD_OUTPUT_HANDLE) || sink->fd == GetStdHandle(STD_ERROR_HANDLE)) continue;
    all_ok &= CloseHandle(sink->fd);
    #else
    if (sink->fd == STDOUT_FILENO || sink->fd == STDERR_FILENO) continue;
    all_ok &= (close(sink->fd) == 0);
    #endif
  }
  return all_ok;
}

Timber *timber_alloc(void)
{
  void *ptr = calloc(1, sizeof(struct Timber));
  if (!ptr) { errno = ENOMEM; return NULL; }
  return (Timber *)ptr;
}

void timber_free(Timber *lg)
{
  if (!lg) return;
  free(lg);
}

Timber *timber_add_file_sink(Timber *lg, const char *file_path)
{
  if (lg->sink_count >= TIMBER_MAX_SINKS) { errno = ERANGE; return NULL; }

  // If the same file path exists, dont create it, return earlier
  for (size_t i = 0; i < lg->sink_count; i++) {
    if (strcmp(lg->sinks[i].file_path, file_path) == 0) return lg;
  }

  timber_fd_t fd;
#ifdef _WIN32
  WCHAR *wfile_path = _timber_win32_utf8_to_wide(file_path);
  if (!wfile_path) { return NULL; }
  fd = CreateFileW(wfile_path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
  free(wfile_path);
  if (fd == INVALID_HANDLE_VALUE) {
    _timber_report_error("CreateFileW");
    errno = _timber_win32_error_to_cerrno(); return NULL;
  }
#else
  fd = open(file_path, O_WRONLY | O_CREAT, 0640);
  if (fd < 0) { _timber_report_error("open"); return NULL; }
#endif

  struct TimberSink *s = &lg->sinks[lg->sink_count++];
  s->file_path = file_path;
  s->fd = fd;
  return lg;
}

Timber *timber_add_stdout_sink(Timber *lg)
{
  if (lg->sink_count >= TIMBER_MAX_SINKS) { errno = ERANGE; return NULL; }

#ifdef _WIN32
  timber_fd_t stdout_fd = GetStdHandle(STD_OUTPUT_HANDLE);
  if (stdout_fd == INVALID_HANDLE_VALUE) {
    errno = _timber_win32_error_to_cerrno(); return NULL;
  } else if (stdout_fd == NULL) {
    // returns null on GUI programs
    // pretend to be added
    return lg;
  }
#else
  timber_fd_t stdout_fd = STDOUT_FILENO;
#endif

  for (size_t i = 0; i < lg->sink_count; i++) {
    if (lg->sinks[i].fd == stdout_fd) return lg;
  }

  struct TimberSink *s = &lg->sinks[lg->sink_count++];
  s->fd = stdout_fd;
  s->file_path = NULL;
  return lg;
}

Timber *timber_add_stderr_sink(Timber *lg)
{
  if (lg->sink_count >= TIMBER_MAX_SINKS) { errno = ERANGE; return NULL; }

#ifdef _WIN32
  timber_fd_t stderr_fd = GetStdHandle(STD_ERROR_HANDLE);
  if (stderr_fd == INVALID_HANDLE_VALUE) {
    errno = _timber_win32_error_to_cerrno(); return NULL;
  } else if (stderr_fd == NULL) {
    // returns null on GUI programs
    // pretend to be added
    return lg;
  }
#else
  timber_fd_t stderr_fd = STDERR_FILENO;
#endif

  for (size_t i = 0; i < lg->sink_count; i++) {
    if (lg->sinks[i].fd == stderr_fd) return lg;
  }

  struct TimberSink *s = &lg->sinks[lg->sink_count++];
  s->fd = stderr_fd;
  s->file_path = NULL;
  return lg;
}

Timber *timber_set_policy(Timber *lg, TimberPolicy policy)
{
  if (!lg) return NULL;
  lg->log_policy = policy;
  return lg;
}

Timber *timber_set_format(Timber *lg, const char *fmt)
{
  if (!lg) return NULL;
  const char *pfmt = fmt;
  size_t sfmt = strlen(fmt);

  while (sfmt > 0) {
    const char *found = (const char *)memchr(pfmt, '$', sfmt);
    if (found) {
      size_t amount = (size_t)(found - pfmt);
      if (amount == sfmt - 1) {
        _timber_debug_err("Unexpected end of string in format");
        return NULL;
      }
      const char var_ch = *(found + 1);

      if (var_ch == '$') {
        if (!_timber_add_fmt_token(&lg->format, _TIMBER_TK_LITERAL, pfmt, amount + 1)) return NULL;
      } else {
        if (amount > 0) {
          if (!_timber_add_fmt_token(&lg->format, _TIMBER_TK_LITERAL, pfmt, amount)) return NULL;
        }

        TimberTokenType tk_type;
        switch (var_ch) {
        case 'T': tk_type = _TIMBER_TK_TIME; break;
        case 'L': tk_type = _TIMBER_TK_LEVEL; break;
        case 'M': tk_type = _TIMBER_TK_MESSAGE; break;
        default:
          _timber_debug_err("Invalid variable in format: '%c'\n", var_ch);
          return NULL;
        }
        if (!_timber_add_fmt_token(&lg->format, tk_type, NULL, 0)) return NULL;
      }

      pfmt += amount + 2;
      sfmt -= amount + 2;
    } else {
      if (!_timber_add_fmt_token(&lg->format, _TIMBER_TK_LITERAL, pfmt, sfmt)) {
        return NULL;
      }
      sfmt = 0;
    }
  }
  return lg;
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

const char *timber_level_to_cstr(TimberLevel level)
{
  switch(level) {
#define X(_, name) case TIMBER_##name: return #name;
TIMBER_LEVELS
#undef X
  default: return "";
  }
  _TIMBER_UNREACHABLE("timber_level_to_cstr");
}
