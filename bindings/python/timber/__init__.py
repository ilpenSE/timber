"""
Timber Python Bindings
"""

from cffi import FFI
from enum import IntEnum

ffi = FFI()
ffi.cdef("""
typedef struct Timber Timber;

typedef enum {
  TIMBER_INFO, TIMBER_ERROR, TIMBER_WARNING,
  _TimberLevel_count,
} TimberLevel;

typedef enum {
  TIMBER_DROP_POLICY = 0,
  TIMBER_BLOCK_POLICY,
  _TimberPolicy_count,
} TimberPolicy;

bool timber_init(Timber *lg);
bool timber_destroy(Timber *lg);

bool timber_logn(Timber *lg, TimberLevel level, const char *msg, size_t msgsz);
bool timber_log(Timber *lg, TimberLevel level, const char *msg);

bool timber_flush(Timber *lg);
Timber *timber_add_file_sink(Timber *lg, const char *file_path);
Timber *timber_add_stdout_sink(Timber *lg);
Timber *timber_add_stderr_sink(Timber *lg);
Timber *timber_set_policy(Timber *lg, TimberPolicy policy);
Timber *timber_set_format(Timber *lg, const char *format);

Timber *timber_alloc();
void timber_free(Timber *lg);
""")

import os
import sys

if sys.platform == "win32":
  lib_name = "timber.dll"
elif sys.platform == "darwin":
  lib_name = "libtimber.dylib"
else:
  lib_name = "libtimber.so"

_real_path = os.path.join(os.path.dirname(__file__), lib_name)
_timber = ffi.dlopen(_real_path)

# Add better and clearer API
