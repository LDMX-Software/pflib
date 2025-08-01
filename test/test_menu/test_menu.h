#pragma once
#include "pflib/menu/Menu.h"

class test_menu : public pflib::menu::Menu<int*> {
 public:
  struct State {
    int p;
  };
  static State state;
};

void print_cmd(const std::string& cmd, test_menu::TargetHandle p);
