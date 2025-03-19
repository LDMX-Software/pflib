#pragma once

#define BOOST_LOG_DYN_LINK

#include <boost/log/core.hpp>
#include <boost/log/expressions.hpp>

namespace pflib::logging {
enum level {
  trace = -1,
  debug = 0,
  info = 1,
  warn = 2,
  error = 3,
  fatal = 4
};

using logger = boost::log::sources::severity_channel_logger_mt<level, std::string>;

logger get(const std::string& name);

}

#define enable_logging(name) \
  mutable ::pflib::logging::logger the_log_{::pflib::logging::get(name)};

#define pflib_log(lvl) \
  BOOST_LOG_SEV(the_log_, ::pflib::logging::level::lvl)
