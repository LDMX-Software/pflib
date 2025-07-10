#include "sub.h"

pflib::menu::Menu<test_menu::TargetHandle>* sub_menu = test_menu::menu("SB", "sub menu");

static auto _f = sub_menu->line("SUB_PRINT", "print command", print_cmd);
