// C++ bindings (syntax sugar and formatting) for timber.h - v2.0.0
#ifndef TIMBER_HPP
#define TIMBER_HPP

#include <string>
#include <sstream>

#if __cplusplus >= 202002L
#include <format>
#endif

#ifdef QT_CORE_LIB
#include <QString>
#endif

#include "timber.h"

namespace timber {
class Stream {
public:
  explicit Stream(::Timber* ctx, ::TimberLevel level) : m_ctx(ctx), m_level(level) {}
  ~Stream() {
    const std::string s = m_buffer.str();
    if (!s.empty()) {
      timber_logn(m_ctx, m_level, s.c_str(), s.size());
    }
  }
  Stream(Stream&&) = default;
  Stream& operator=(Stream&&) = default;
  Stream(const Stream&) = delete;
  Stream& operator=(const Stream&) = delete;

#ifdef QT_CORE_LIB
  template <typename T>
  Stream& operator <<(const T& value) {
    m_buffer << value.toUtf8().constData();
    return *this;
  }
#endif

  template <typename T>
  Stream& operator <<(const T& value) {
    m_buffer << value;
    return *this;
  }

private:
  ::Timber *m_ctx;
  ::TimberLevel m_level;
  std::ostringstream m_buffer;
};

class Instance {
public:
  Instance(TimberPolicy policy = TIMBER_DROP_POLICY, const char *format = nullptr) {
    m_inst = timber_alloc();
    timber_set_policy(m_inst, policy);
    if (format) timber_set_format(m_inst, format);
  }
  ~Instance() { if (m_is_initialized) timber_destroy(m_inst); timber_free(m_inst); }
  Instance(const Instance&) = delete;
  Instance& operator =(const Instance&) = delete;
  Instance(Instance&&) = delete;
  Instance& operator =(Instance&&) = delete;

  bool init() {
    m_is_initialized = timber_init(m_inst);
    return m_is_initialized;
  }

  bool log(::TimberLevel level, const char *cstr, size_t size = 0) {
    return timber_logn(m_inst, level, cstr, size == 0 ? strlen(cstr) : size);
  }
#define X(lower, upper) \
  bool lower(const char *cstr, size_t size = 0) { \
    return log(TIMBER_##upper, cstr, size); \
  } \
  Stream lower() { return Stream(m_inst, TIMBER_##upper); }
TIMBER_LEVELS
#undef X

#if __cplusplus >= 202002L
  template <typename... Args>
  bool log(::TimberLevel level, std::format_string<Args...> fmt, Args&&... args) {
    std::string formatted = std::format(fmt, std::forward<Args>(args)...);
    return log(level, formatted.c_str(), formatted.size());
  }

#define X(lower, upper) \
  template <typename... Args> \
  bool lower(std::format_string<Args...> fmt, Args&&... args) { \
    std::string formatted = std::format(fmt, std::forward<Args>(args)...); \
    return log(TIMBER_##upper, formatted.c_str(), formatted.size()); \
  }
TIMBER_LEVELS
#undef X
#endif // __cplusplus

  Instance *add_sink(const char *file_path) {
    if (!timber_add_file_sink(m_inst, file_path)) return nullptr;
    return this;
  }

  Instance *set_mode(TimberMode mode) {
    if (!timber_set_mode(m_inst, mode)) return nullptr;
    return this;
  }

  Instance *set_policy(TimberPolicy policy) {
    if (!timber_set_policy(m_inst, policy)) return nullptr;
    return this;
  }

  Instance *set_format(const char *format) {
    if (!timber_set_format(m_inst, format)) return nullptr;
    return this;
  }

  Instance *add_stderr() {
    if (!timber_add_stderr_sink(m_inst)) return nullptr;
    return this;
  }

  Instance *add_stdout() {
    if (!timber_add_stdout_sink(m_inst)) return nullptr;
    return this;
  }

private:
  ::Timber *m_inst = nullptr;
  bool m_is_initialized = false;
}; // class Timber
} // namespace timber

#endif // TIMBER_HPP
