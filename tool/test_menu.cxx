#include "Menu.h"

static void print_cmd(const std::string& cmd, int* p) {
  std::cout << " Ran command " << cmd << std::endl;
}

static void RunMenu(int* p) {
  using Menu = Menu<int>;
  Menu menu({
      Menu::Line("ONE", "One command", &print_cmd),
      Menu::Line("TWO", "Second command", &print_cmd),
      Menu::Line("EXIT", "Leave")
      });
  menu.steer(p);
}

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
