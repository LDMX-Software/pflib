#pragma once

#include <memory>
#include <string>

#include "pflib/Exception.h"

namespace pflib::utility {

/**
 * Backport of C++20 std::format-like function
 *
 * I say "std::format-like" because instead of using curly-braces `{}`
 * for inserting the arguments like std::format does, this function uses
 * the C-style (printf-style) percent-characters (e.g. `%s` to insert a string).
 *
 * (Supported in GCC-13 and we have GCC-11)
 *
 * We basically write to a char* and then conver that to std::string.
 *
 * @tparam Args types of arguments passed into snprintf
 * @param[in] format snprintf format string to use
 * @param[in] args arguments for snprintf
 * @return formatted std::string
 *
 * Shamelessly taken from https://stackoverflow.com/a/26221725
 * which has a line-by-line explanation for those who wish to learn more C++!
 * I've modified it slightly to use our exceptions and longer variable names.
 */
template<typename ... Args>
std::string string_format(const std::string& format, Args ... args) {
  // length of string without the closing null byte \0
  int size_s = std::snprintf(nullptr, 0, format.c_str(), args...);
  if (size_s < 0) {
    PFEXCEPTION_RAISE("string_format", "error during formating of string "+format);
  }
  // need one larger for the closing null byte
  auto size = static_cast<std::size_t>(size_s + 1);
  std::unique_ptr<char[]> buffer(new char[size]);
  // do format again, but this time we know the size and that it works!
  std::snprintf(buffer.get(), size, format.c_str(), args...);
  // remove trailing null byte when copying into std::string
  return std::string(buffer.get(), buffer.get() + size - 1);
}

}
