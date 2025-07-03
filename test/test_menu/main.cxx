#include "test_menu.h"

test_menu::State test_menu::state{};

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

  /*
auto sb = test_menu::menu("SB", "example submenu", [](test_menu::TargetHandle p) {
                std::cout << test_menu::state.p << std::endl;
                })
              ->line("PRINT", "print target", print_cmd)
              ->line("INCSB", "increment the target", increment)
              ->line("ADD", "add something to the target",
                  [](test_menu::TargetHandle p) {
                    int n = test_menu::readline_int("Number to add: ");
                    std::cout << " " << *p << " -> ";
                    (*p) += n;
                    std::cout << *p << std::endl;
                  }
              );
                
              */

auto r = test_menu::root()
             ->line("INC", "increment the target", increment)
             ->line("ONE", "one command", print_cmd);

}  // namespace

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
