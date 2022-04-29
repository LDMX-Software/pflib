#ifndef PFTOOL_H
#define PFTOOL_H
#include "pftool_fastcontrol.h"
#include "pftool_expert.h"
#include "pftool_tasks.h"
#include "pftool_roc.h"
#include "pftool_daq.h"
#include "pftool_elinks.h"
#include "pftool_bias.h"
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
 * Check if a file exists by attempting to open it for reading
 * @param[in] fname name of file to check
 * @return true if file can be opened, false otherwise
 */
bool file_exists(const std::string& fname);

#endif /* PFTOOL_H */
