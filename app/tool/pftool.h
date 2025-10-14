/**
 * @file pftool.h
 * Shared declarations for all pftool menu commands
 */

#pragma once

#include "pflib/ECON.h"
#include "pflib/Target.h"
#include "pflib/menu/Menu.h"

/**
 * get a logger using the file name as the channel name
 *
 * the filename's extension is removed and the file path
 * that is shared with the directory of this file is replaced
 * by 'pftool'.
 */
::pflib::logging::logger get_by_file(const std::string& filepath);

/**
 * @macro ENABLE_LOGGING
 * Enable logging using the file name as the channel name
 */
#define ENABLE_LOGGING() static auto the_log_{get_by_file(__FILE__)};

/**
 * pull the target of our menu into this source file to reduce code
 */
using pflib::Target;

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
    /// list of page names for tab completion per ROC ID
    std::map<int, std::vector<std::string>> roc_page_names_;
    /// list of parameter names for tab-completion per ROC ID
    std::map<int, std::map<std::string, std::vector<std::string>>>
        roc_param_names_;
    /// list of page names for tab completion per ECON type
    std::map<std::string, std::vector<std::string>> econ_page_names_;
    /// list of parameter names for tab-completion per ECON type
    std::map<std::string, std::map<std::string, std::vector<std::string>>>
        econ_param_names_;

   public:
    /// initialize the state with a Target
    void init(Target* tgt);
    /// get page names for tab completion
    const std::vector<std::string>& roc_page_names() const;
    /// get the parameter names for tab completion
    const std::vector<std::string>& roc_param_names(
        const std::string& page) const;
    /// get page names for tab completion
    const std::vector<std::string>& econ_page_names(pflib::ECON econ) const;
    /// get the parameter names for tab completion
    const std::vector<std::string>& econ_param_names(
        pflib::ECON econ, const std::string& page) const;
    /// index of HGCROC currently being interacted with
    int iroc{0};
    /// index of ECON currently being interacted with
    int iecon{0};
    /// index of link currently being interacted with
    int ilink{0};
    /// current format mode to use
    Target::DaqFormat daq_format_mode{Target::DaqFormat::SIMPLEROC};
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
