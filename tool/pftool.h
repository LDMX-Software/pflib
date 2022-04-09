#ifndef PFTOOL_H
#define PFTOOL_H

#include "pftool_i2c.h"
#include "pftool_tasks.h"
#include "pftool_roc.h"
#include "pftool_daq.h"
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
#include "pflib/decoding/SuperPacket.h"

/**
 * pull the target of our menu into this source file to reduce code
 */
using pflib::PolarfireTarget;

#endif /* PFTOOL_H */
