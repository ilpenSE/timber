/* SPDX-License-Identifier: MIT */
// Timber v1.0.1 - Asynchronous Logging Library
#ifndef TIMBER_H
#define TIMBER_H

#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdarg.h>

// C/C++ version checking
#if __STDC_VERSION__ >= 199901L
  #define _TIMBER_USING_VALID_C 1
#else
  #if __cplusplus >= 201103L
    #define _TIMBER_USING_VALID_CXX 1
  #endif
#endif

#ifndef _TIMBER_USING_VALID_C
#ifndef _TIMBER_USING_VALID_CXX
  #error "Required C or C++ version not found: >=C99 or >=C++11"
#endif
#endif

// Define TIMBER_RELEASE or TIMBER_NDEBUG for release builds
#if !defined(TIMBER_RELEASE) && !defined(TIMBER_NDEBUG)
  #define TIMBER_DEBUG 1
#endif

/*
  Version of Timber
  Format: MAJOR_MINOR_PATCH_L (concat these)
  Minor and patch should be padded with 0 if they're 1 digit long.
*/
#define TIMBER_VER 10001L
#define TIMBER_MAJOR 1
#define TIMBER_MINOR 0
#define TIMBER_PATCH 1

// Maximum amount of sinks in an instance
#define TIMBER_MAX_SINKS 8
#define TIMBER_MAX_MSG_SIZE 256
#define TIMBER_QUEUE_SIZE 1024
typedef struct Timber Timber;

// Log Levels you can add another one like we did if you wish
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

// portable printf-format style checker (only available on gcc and clang)
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

#define X(lower, upper)                                                                          \
static inline bool timber_##lower##f(Timber *lg, const char *fmt, ...) TIMBER_PRINTF_LIKE(2, 3); \
static inline bool timber_##lower(Timber *lg, const char *msg);                                  \
static inline bool timber_##lower##n(Timber *lg, const char *msg, size_t msgsz);
TIMBER_LEVELS
#undef X

#endif // TIMBER_H
