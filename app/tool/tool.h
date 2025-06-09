/**
 * @file tool.h
 * Shared declarations for all pftool menu commands
 */

#pragma once

#include "pflib/menu/Menu.h"
#include "pflib/Target.h"

/**
 * pull the target of our menu into this source file to reduce code
 */
using pflib::Target;

/**
 * The type of menu we are constructing
 */
using pftool = pflib::menu::Menu<Target*>;
