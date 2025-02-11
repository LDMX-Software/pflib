#include "pflib/Version.h"
#include <cstdio>

void print_version() {
  printf("  pflib %s (%s)\n  built for %s\n", PFLIB_VERSION, GIT_DESCRIBE, ROC_VERSION);
}
