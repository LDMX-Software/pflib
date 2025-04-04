#ifndef PFLIB_EXCEPTION_H_
#define PFLIB_EXCEPTION_H_

#include <exception>
#include <string>

namespace pflib {

/**
 * @class Exception class for PFlib
 */
class Exception : public std::exception {
 public:
  /**
   * Empty constructor.
   *
   * Don't build stack trace for empty exceptions.
   */
  Exception() throw() {}

  /**
   * Class constructor.
   * @param name Name of the exception.
   * @param message Extended message describing the exception.
   * @param module Filename in the source code where the exception occurred.
   * @param line Line in the source code where the exception occurred.
   * @param function Function in which the exception occurred.
   */
  Exception(const std::string &name, const std::string &message,
            const std::string &module, int line, const std::string &function)
      : name_{name},
        message_{message},
        module_{module},
        function_{function},
        line_{line} {}

  /**
   * Class destructor.
   */
  virtual ~Exception() throw() {}

  /**
   * Get the name of the exception.
   * @return The name of the exception.
   */
  const std::string &name() const throw() { return name_; }

  /**
   * Get the message of the exception.
   * @return The message of the exception.
   */
  const std::string &message() const throw() { return message_; }

  /**
   * Get the source filename where the exception occurred.
   * @return The source filename where the exception occurred.
   */
  const std::string &module() const throw() { return module_; }

  /**
   * Get the function name where the exception occurred.
   * @return The function name where the exception occurred.
   */
  const std::string &function() const throw() { return function_; }

  /**
   * Get the source line number where the exception occurred.
   * @return The source line number where the exception occurred.
   */
  int line() const throw() { return line_; }

  /**
   * The error message.
   * @return The error message.
   */
  virtual const char *what() const throw() { return message_.c_str(); }

 private:
  /** Exception name. */
  std::string name_;

  /** Error message. */
  std::string message_;

  /** Source filename where the exception occurred. */
  std::string module_;

  /** Function name where the exception occurred. */
  std::string function_;

  /** Source line number where the exception occurred. */
  int line_{0};
};

}  // namespace pflib

/**
 * @def PFEXCEPTION_RAISE(EXCEPTION, MSG)
 * @param EXCEPTION Exception name
 * @param MSG Error message
 *
 * @brief Utility macro for throwing exceptions, automatically including the
 * necessary file, line, and function information.  The user need only
 * supply the exception name and error message
 */
#define PFEXCEPTION_RAISE(EXCEPTION, MSG) \
  throw pflib::Exception(EXCEPTION, MSG, __FILE__, __LINE__, __FUNCTION__)

#endif  // PFLIB_EXCEPTION_H_
