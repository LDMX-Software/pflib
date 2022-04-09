#include "Menu.h"

namespace {

void print_cmd(const std::string& cmd, int* p) {
  std::cout << std::hex << p << std::endl;
  std::cout << " Ran command " << cmd << " with " << *p << std::endl;
}

void increment(int* p) {
  std::cout << std::hex << p << std::endl;
  std::cout << " " << *p << " -> ";
  (*p)++;
  std::cout << *p << std::endl;
}

Menu<int> sb;

auto s0 = sb.declare("THREE", "third command", print_cmd);
auto s1 = sb.declare("INC", "increment the target", increment);

auto v0 = Menu<int>::root().declare("INC", "increment the target", increment);
auto v1 = Menu<int>::root().declare("ONE", "one command", print_cmd);
auto v2 = Menu<int>::root().declare("SB", "submenu", sb);

}

/**
 * test-menu executable
 *
 * A light executable that is useful for developing
 * the Menu apparatus. You can compile this executable
 * without the rest of pflib by executing the compiler
 * directly. In the tool directory,
 * ```
 * g++ -DPFLIB_TEST_MENU=1 -o test-menu test_menu.cxx Menu.cc -lreadline
 * ```
 */
int main(int argc, char* argv[]) {
  try {
    int i = 3;
    Menu<int>::run(&i);
  } catch (std::exception& e) {
    fprintf(stderr, "Exception!  %s\n",e.what());
    return 1;
  }
  return 0;
}
