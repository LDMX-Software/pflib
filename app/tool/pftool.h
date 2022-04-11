/**
 * @file pftool.h
 * shared includes and declarations across all of pftool
 */
#ifndef PFTOOL_H
#define PFTOOL_H

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>
#include <map>
#include <iomanip>
#include <stdlib.h>
#include <unistd.h>
#include <string>
#include <exception>
#include "pflib/PolarfireTarget.h"
#include "pflib/Compile.h" // for parameter listing
#ifdef PFTOOL_ROGUE
#include "pflib/rogue/RogueWishboneInterface.h"
#endif
#ifdef PFTOOL_UHAL
#include "pflib/uhal/uhalWishboneInterface.h"
#endif
#include "Menu.h"
#include "Rcfile.h"

/**
 * our menu program uses the PolarfireTarget
 */
using pftool = Menu<pflib::PolarfireTarget>;

/**
 * storage of file path to last run file
 *
 * used in DAQ.EXTERNAL
 */
static std::string last_run_file;

#endif  // PFTOOL_H
