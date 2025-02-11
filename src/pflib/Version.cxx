#include "pflib/Version.h"
#include <cstdio>

void print_version() {
  printf("  pflib %s (%s)\n", PFLIB_VERSION, GIT_DESCRIBE);
}
