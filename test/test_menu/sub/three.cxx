#include "sub.h"

void three(test_menu::TargetHandle p) {
  int n = test_menu::readline_int("How much to add to p?", 0);
  std::cout << *p << " -> ";
  *p += n;
  std::cout << *p << std::endl;
}

static auto v = sub_menu->line("THREE", "add arbitrary number", three);
