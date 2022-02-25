#include "Menu.h"

/**
 * Just print the command that is provided
 */
static void print_cmd(const std::string& cmd, int* p) {
  std::cout << " Ran command " << cmd << std::endl;
}

/**
 * Define the menu options
 */
static void RunMenu(int* p) {
  using Menu = Menu<int>;
  Menu menu({
      Menu::Line("ONE", "One command", &print_cmd),
      Menu::Line("TWO", "Second command", &print_cmd),
      Menu::Line("EXIT", "Leave")
      });
  menu.steer(p);
}

/**
 * test-menu executable
 *
 * A light executable that is useful for developing
 * the Menu apparatus. You can compile this executable
 * without the rest of pflib by executing the compiler
 * directly. In the tool directory,
 * ```
 * g++ -DTEST_MENU=1 -o test-menu test_menu.cxx Menu.cc -lreadline
 * ```
 */
int main(int argc, char* argv[]) {
  try {
    int i = 3;
    RunMenu(&i);
  } catch (std::exception& e) {
    fprintf(stderr, "Exception!  %s\n",e.what());
    return 1;
  }
  return 0;
}
