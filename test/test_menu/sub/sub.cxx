#include "../test_menu.h"
#include "another.h"
#include "three.h"

static auto sub_menu =
    test_menu::menu("SB", "sub menu example")
        ->line("PRINT_CMD", "calling a function defined in parent", print_cmd)
        ->line("THREE", "third function defined here", three)
        ->line("ANOTHER", "another function to multiply target", another, SHOULD_KEEP)
        ->line("ALSO_DNE", "should also not show up in menu", another, SHOULD_DISABLE);
