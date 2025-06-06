#define MENU_HISTORY_FILEPATH ".pflib-test-menu-history"
#include "pflib/menu/Menu.h"

#include <signal.h>
#include <readline/history.h>

using test_menu = pflib::menu::Menu<int*>;

void print_cmd(const std::string& cmd, test_menu::TargetHandle p) {
  std::cout << std::hex << p << std::endl;
  std::cout << " Ran command " << cmd << " with " << *p << std::endl;
}

void increment(test_menu::TargetHandle p) {
  std::cout << std::hex << p << std::endl;
  std::cout << " " << *p << " -> ";
  (*p)++;
  std::cout << *p << std::endl;
}

namespace {

auto sb = test_menu::menu("SB", "example submenu")
              ->line("THREE", "third command", print_cmd)
              ->line("INCSB", "increment the target", increment)
              ->line("ADD", "add something to the target",
                  [](test_menu::TargetHandle p) {
                    int n = test_menu::readline_int("Number to add: ");
                    std::cout << " " << *p << " -> ";
                    (*p) += n;
                    std::cout << *p << std::endl;
                  }
              );
                

auto r = test_menu::root()
             ->line("INC", "increment the target", increment)
             ->line("ONE", "one command", print_cmd);

}  // namespace

void close_history(int s) {
  std::cout << "caught signal " << s << std::endl;
  if (int rc = ::write_history(".pflib-test-menu-history"); rc != 0) {
    std::cout << "warn: failure to write history file " << rc << std::endl;
  }
  exit(1);
}

int main(int argc, char* argv[]) {
  if (argc > 1) {
    test_menu::root()->print(std::cout);
    std::cout << std::flush;
    return 0;
  }
  test_menu::set_history_filepath("~/.pflib-test-menu-history");
  try {
    int i = 3;
    test_menu::run(&i);
  } catch (std::exception& e) {
    fprintf(stderr, "Exception!  %s\n", e.what());
    return 1;
  }
  return 0;
}
