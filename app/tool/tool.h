/**
 * @file tool.h
 * Shared declarations for all pftool menu commands
 */

#pragma once

#include "pflib/menu/Menu.h"
#include "pflib/Target.h"

/**
 * @macro ENABLE_LOGGING
 * Enable logging using the file name as the channel name
 */
#define ENABLE_LOGGING() \
  static auto the_log_{::pflib::logging::get("pftool." __FILE__)};

/**
 * pull the target of our menu into this source file to reduce code
 */
using pflib::Target;

/// format mode integer for simple single-HGCROC
static const int DAQ_FORMAT_SIMPLEROC = 1;
/// format mode integer for ECON without Zero Suppression
static const int DAQ_FORMAT_ECON = 2;

/**
 * The type of menu we are constructing
 *
 * We inherit from the base menu class so we can hold a static
 * State of the menu.
 */
class pftool : public pflib::menu::Menu<Target*> {
 public:
  /// static variables to share across menu
  struct State {
    /// index of HGCROC currently being interacted with
    int iroc{0};
    /// type_version of HGCROC currently being interacted with
    std::string type_version{"sipm_rocv3b"};
    /// index of link currently being interacted with
    int ilink{0};
    /// current format mode to use
    int daq_format_mode{1};
    /// contributor ID of daq
    int daq_contrib_id{20};
    /// daq collection rate in Hz
    int daq_rate{100};
    /// path to where the run number is stored
    std::string last_run_file{".last_run_file"};
  };
  /// actual instance of the state
  static State state;
};
