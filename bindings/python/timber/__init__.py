"""
Timber Python Bindings
"""

from cffi import FFI
from enum import IntEnum

ffi = FFI()
ffi.cdef("""
typedef struct Timber Timber;

typedef enum {
  TIMBER_INFO = 0, TIMBER_ERROR, TIMBER_WARNING,
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

import os, sys, platform, threading

m = platform.machine().lower()
if m in ("amd64", "x86_64", "x64"):
  arch_suffix = "x86_64"
elif m in ("arm64", "aarch64"):
  arch_suffix = "aarch64"
else: raise RuntimeError(f"Unsupported architecture: {m}")

if sys.platform == "win32":
  lib_name = f"timber-{arch_suffix}.dll"
elif sys.platform == "darwin":
  lib_name = f"libtimber-{arch_suffix}.dylib"
else:
  lib_name = f"libtimber-{arch_suffix}.so"

_real_path = os.path.join(os.path.dirname(__file__), lib_name)
_native = ffi.dlopen(_real_path)

class LogLevel(IntEnum):
  INFO    = _native.TIMBER_INFO
  ERROR   = _native.TIMBER_ERROR
  WARNING = _native.TIMBER_WARNING

  def __str__(self):
    return self.name

class LogPolicy(IntEnum):
  DROP  = _native.TIMBER_DROP_POLICY
  BLOCK = _native.TIMBER_BLOCK_POLICY

  def __str__(self):
    return self.name

class Timber:
  def __init__(self, fmt: str | None = None) -> None:
    self._ptr = None
    self._started = False
    self._lock = threading.Lock()
    ptr = _native.timber_alloc()
    if not ptr:
      raise MemoryError("Could not allocate timber instance!")
    self._ptr = ptr
    if fmt is not None:
      self.set_format(fmt)

  def _config_ptr(self):
    if not self._ptr:
      raise ValueError("Timber is closed")
    if self._started:
      raise RuntimeError("configure before the first log call")
    return self._ptr

  def _start(self):
    with self._lock:
      if not self._ptr:
        raise ValueError("Timber is closed")
      if not self._started:
        if not _native.timber_init(self._ptr):
          raise RuntimeError("timber_init failed")
        self._started = True
      return self._ptr

  def close(self) -> None:
    with self._lock:
      ptr, self._ptr = self._ptr, None
      started, self._started = self._started, False
    if ptr:
      if started:
        _native.timber_destroy(ptr)
      _native.timber_free(ptr)

  def __del__(self):
    self.close()

  def __enter__(self):
    return self

  def __exit__(self, *exc):
    self.close()
    return False

  def _check(self):
    if not self._ptr:
      raise ValueError("Timber is closed")
    return self._ptr

  def log(self, level: LogLevel, msg: str) -> bool:
    ptr = self._start()
    b = msg.encode("utf-8")
    return _native.timber_logn(ptr, level, b, len(b))

  def info(self, msg: str) -> bool:
    return self.log(LogLevel.INFO, msg)
  def error(self, msg: str) -> bool:
    return self.log(LogLevel.ERROR, msg)
  def warning(self, msg: str) -> bool:
    return self.log(LogLevel.WARNING, msg)

  def flush(self) -> bool:
    return _native.timber_flush(self._check())

  def set_format(self, fmt: str) -> "Timber":
    if not _native.timber_set_format(self._config_ptr(), fmt.encode("utf-8")):
      raise ValueError("invalid format")
    return self

  def set_policy(self, policy: TimberPolicy) -> "Timber":
    if not _native.timber_set_policy(self._config_ptr(), policy):
      raise ValueError("invalid policy")
    return self

  def add_file(self, path) -> "Timber":
    if not _native.timber_add_file_sink(self._config_ptr(), os.fsencode(path)):
      raise OSError("failed to add file sink")
    return self

  def add_stdout(self) -> "Timber":
    if not _native.timber_add_stdout_sink(self._config_ptr()):
      raise OSError("failed to add stdout sink")
    return self

  def add_stderr(self) -> "Timber":
    if not _native.timber_add_stderr_sink(self._config_ptr()):
      raise OSError("failed to add stderr sink")
    return self
