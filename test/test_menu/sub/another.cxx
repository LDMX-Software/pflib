#include "sub.h"

void another(test_menu::TargetHandle p) {
  int multiplier = test_menu::readline_int("Number to multiply target by: ", 1);
  std::cout << *p << " -> ";
  (*p) *= multiplier;
  std::cout << *p << std::endl;
}

static auto a = sub_menu->line("ANOTHER", "multiply target by a number", another);
