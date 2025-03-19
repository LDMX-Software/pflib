#include "pflib/Logging.h"

namespace pflib::logging {

logger get(const std::string& name) {
  logger lg(log::keywords::channel = name);
  return boost::move(lg);
}

}
