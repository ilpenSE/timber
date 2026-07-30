// C++ bindings (syntax sugar and formatting) for timber.h
#ifndef TIMBER_HPP
#define TIMBER_HPP

#include <string>
#include <sstream>

#ifdef QT_CORE_LIB
#include <QString>
#endif

#include "timber.h"

// TODO: Add C++20 std::format support

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
  ::Timber* m_ctx;
  ::TimberLevel m_level;
  std::ostringstream m_buffer;
};

class Timber {
public:
  Timber(TimberPolicy policy = TIMBER_DROP_POLICY, const char *format = "") {
    timber_set_policy(&m_inst, policy);
    timber_set_format(&m_inst, format);
  }
  ~Timber() { if (m_is_initialized) timber_destroy(&m_inst); }
  Timber(const Timber&) = delete;
  Timber& operator =(const Timber&) = delete;
  Timber(Timber&&) = delete;
  Timber& operator =(Timber&&) = delete;

  bool init() {
    m_is_initialized = timber_init(&m_inst);
    return m_is_initialized;
  }

  bool log(::TimberLevel level, const char *cstr, size_t size = 0) {
    return timber_logn(&m_inst, level, cstr, size == 0 ? strlen(cstr) : size);
  }
#define X(lower, upper) \
  bool lower(const char *cstr, size_t size = 0) { \
    return timber_logn(&m_inst, TIMBER_##upper, cstr, size == 0 ? strlen(cstr) : size); \
  } \
  Stream lower() { return Stream(&m_inst, TIMBER_##upper); }
TIMBER_LEVELS
#undef X

  bool add_sink(const char *file_path) {
    return timber_add_file_sink(&m_inst, file_path);
  }

  bool add_sink(FILE *file) {
    if (file == stdout) timber_add_stdout_sink(&m_inst);
    else if (file == stderr) timber_add_stderr_sink(&m_inst);
    else return false;
    return true;
  }

private:
  ::Timber m_inst{};
  bool m_is_initialized;
}; // class Timber
} // namespace timber

#endif // TIMBER_HPP
