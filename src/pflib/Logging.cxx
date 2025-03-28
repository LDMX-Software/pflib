#include "pflib/Logging.h"

#include <boost/core/null_deleter.hpp>  //to avoid deleting std::cout
#include <boost/log/expressions.hpp>          //for attributes and expressions
#include <boost/log/sinks/sync_frontend.hpp>  //syncronous sink frontend
#include <boost/log/sinks/text_ostream_backend.hpp>  //output stream sink backend
#include <boost/log/sources/global_logger_storage.hpp>  //for global logger default
#include <boost/log/utility/setup/common_attributes.hpp>

#include <boost/log/utility/setup/file.hpp>

namespace pflib::logging {

level convert(int i_lvl) {
  if (i_lvl < -1)
    i_lvl = -1;
  else if (i_lvl > 4)
    i_lvl = 4;
  return level(i_lvl);
}

logger get(const std::string& name) {
  logger lg(boost::log::keywords::channel = name);
  return boost::move(lg);
}

/**
 * Safely extract a value from the Boost.Log attribute
 *
 * If the attribute has not been set, then we return a default
 * constructed version of the value.
 */
template <typename T>
const T& safe_extract(boost::log::attribute_value attr) {
  static const T empty_value = {};
  auto attr_val = boost::log::extract<T>(attr);
  if (attr_val) {
    return *attr_val;
  }
  return empty_value;
}

/// the default level to apply regardless of settings
static level default_level = level::info;

/// custom levels to apply to specific channels
static std::unordered_map<std::string, level> custom_levels;

void open() {
  using OurSinkBackend = boost::log::sinks::text_ostream_backend;
  using OurSinkFrontend = boost::log::sinks::synchronous_sink<OurSinkBackend>;

  boost::log::add_common_attributes();
  auto core = boost::log::core::get();
  
  auto term_back = boost::make_shared<OurSinkBackend>();
  term_back->add_stream(
      boost::shared_ptr<std::ostream>(
        &std::cout,
        boost::null_deleter()
      ));
  // flush to screen **after each message**
  term_back->auto_flush(true);

  auto term_sink = boost::make_shared<OurSinkFrontend>();
  term_sink->set_filter([&](boost::log::attribute_value_set const& attrs) {
        auto it = custom_levels.find(safe_extract<std::string>(attrs["Channel"]));
        if (it != custom_levels.end()) {
          return safe_extract<level>(attrs["Severity"]) >= it->second;
        }
        return safe_extract<level>(attrs["Severity"]) >= default_level;
      });
  term_sink->set_formatter(
      [](const boost::log::record_view& view, boost::log::formatting_ostream& os) {
        std::cout 
          << "i can be seen so i know we are passing the filter "
          << view[boost::log::expressions::smessage]
          << std::endl;
        os << "i cannot be seen so something is wrong";
        os << "[ "
           << boost::log::extract<std::string>("Channel", view)
           << "] ";
        const level msg_level{safe_extract<level>(view["Severity"])};
        switch (msg_level) {
          case level::trace:
            os << "trace";
            break;
          case level::debug:
            os << "debug";
            break;
          case level::info:
            os << " info";
            break;  
          case level::warn:
            os << " warn";
            break;
          case level::error:
            os << "error";
            break;
          case level::fatal:
            os << "fatal";
            break;
          default:
            os << "?????";
            break;
        }
        os << "(";
        if (auto timestamp = boost::log::extract< boost::posix_time::ptime >("TimeStamp", view)) {
          std::tm ts = boost::posix_time::to_tm(*timestamp);
          char buf[128];
          if (std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &ts) > 0) {
            os << buf;
          } else {
            os << "?TIME?";
          }
        } else {
            os << "?TIME?";
        }
        os << ") " << view[boost::log::expressions::smessage];
      }
  );

  core->add_sink(term_sink);
}

void set(level lvl, const std::string& only) {
  if (only.empty()) {
    // apply globally
    default_level = lvl;
  } else {
    custom_levels[only] = lvl;
  }
}

void close() {
  boost::log::core::get()->remove_all_sinks();
}


}
