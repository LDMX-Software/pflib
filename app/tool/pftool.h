/**
 * @file pftool.h
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
  static auto the_log_{::pflib::logging::get_by_file("pftool.", __FILE__)};

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
  class State {
    /// type_version of HGCROC currently being interacted with
    std::string type_version_;
    /// list of page names for tab completion
    std::vector<std::string> page_names_;
    /// list of parameter names for tab-completion
    std::map<std::string, std::vector<std::string>> param_names_;
   public:
    /// default constructor which sets default type_version
    State();
    /// update roc type version, regenerating tab completion lists if needed
    void update_type_version(const std::string& type_version);
    /// get the type_version of the HGCROC currently being interacted with
    const std::string& type_version() const;
    /// get page names for tab completion
    const std::vector<std::string>& page_names() const;
    /// get the parameter names for tab completion
    const std::vector<std::string>& param_names(const std::string& page) const;
    /// index of HGCROC currently being interacted with
    int iroc{0};
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
