#pragma once

#define BOOST_LOG_DYN_LINK

#include <boost/log/core.hpp>  //core logging service
#include <boost/log/sources/record_ostream.hpp>
#include <boost/log/sources/severity_channel_logger.hpp>  //for the severity logger
#include <boost/log/sources/severity_feature.hpp>  //for the severity feature in a logger

/**
 * hold logging infrastructure in namespace
 */
namespace pflib::logging {

/**
 * logging severity labels and their corresponding integers
 */
enum level { trace = -1, debug = 0, info = 1, warn = 2, error = 3, fatal = 4 };

/**
 * convert an integer to the severity level enum
 *
 * Any integer below -1 is set to trace and
 * any integer above four is set to fatal.
 *
 * @param[in] i_lvl integer level to be converted
 * @return converted enum level
 */
level convert(int i_lvl);

/**
 * our logger type
 *
 * holds a severity level and a channel name in addition to
 * the other default attributes.
 */
using logger =
    boost::log::sources::severity_channel_logger_mt<level, std::string>;

/**
 * Gets a logger with the input name for its channel.
 *
 * We maintain a cache of channel-names to loggers
 * so that multiple locations can share the same channel;
 * however, the calls to this function should be minimized
 * in order to avoid wasting time looking up the logger
 * in the cache, making copies of a logger, or creating a new one.
 *
 * @param name name of this logging channel
 * @return logger with the input channel name
 */
logger get(const std::string& name);

/**
 * Get a logger for the input prefix and file
 *
 * This is useful in menus where we want the logging
 * message associated with the __FILE__ from which the command
 * originated but we don't want to have to include the whole filepath
 * in the logging channel.
 *
 * @param[in] prefix string to prefix filestem with (e.g. menu name)
 * @param[in] filepath filepath string e.g. from the __FILE__ macro
 * @return logger
 */
logger get_by_file(const std::string& prefix, const std::string& filepath);

/**
 * Initialize the logging backend
 *
 * This function sets up the sinks for the logs (e.g. terminal output)
 * and sets the format and filtering.
 *
 * The formatting is set once and can support coloring the messages
 * using ANSI escape sequences.
 */
void open(bool color);

/**
 * Change the level for the logs
 *
 * @param[in] lvl level (and above) to include in messages
 * @param[in] only channel name to apply the level to
 */
void set(level lvl, const std::string& only = "");

/**
 * Close up the logging
 */
void close();

/**
 * a "fixture" that opens the logging when it is created
 * and closes the logging when it is destroyed.
 *
 * This is helpful for writing new programs where we want to
 * open the logging at the beginning and then have the logging
 * closed automatically when execution is completed.
 *
 * ```cpp
 * int main(int argc, char** argv) {
 *   pflib::logging::fixture f;
 *
 *   // logging enabled and will be closed when f
 *   // goes out of scope and is destructed
 * }
 * ```
 */
struct fixture {
  /**
   * constructor to open logging and provide an initial configuration
   *
   * The default color is true if we are connected to a terminal (isatty).
   * The default logging level is info.
   */
  fixture();

  /// destructor to close logging
  ~fixture();
};

}  // namespace pflib::logging

/**
 * @macro pflib_log
 *
 * This macro can be used in place of std::cout with the following notes.
 * - A newline is appended to the message so you do not need to end your message
 * with std::endl.
 * - This macro assumes a logger variable named the_log_ is in scope.
 *
 * Use ::pflib::logging::get to create a logger and hold it within your class or
 * namespace. For example, in a class declaration use
 * ```cpp
 * mutable ::pflib::logging::logger
 * the_log_{::pflib::logging::logger::get("my_channel")};
 * ```
 *
 * @param lvl input logging level (without namespace or enum)
 */
#define pflib_log(lvl) BOOST_LOG_SEV(the_log_, ::pflib::logging::level::lvl)
