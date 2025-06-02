#include "pflib/version/Version.h"

#include <cstdio>

#include "cmake_version.h"

namespace pflib::version {

std::string tag() {
  return PFLIB_VERSION;
}

std::string git_describe() {
  return GIT_DESCRIBE;
}

std::string debug() {
  return tag() + " (" + git_describe() + ")";
}

}
