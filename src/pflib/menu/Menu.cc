#include "pflib/menu/Menu.h"

#include <signal.h>
#include <readline/history.h>
#include <readline/readline.h>

#include <iostream>

#include "pflib/Compile.h"  // for str_to_int
#include "pflib/Logging.h"

namespace pflib::menu {

std::list<std::string> BaseMenu::cmdTextQueue_;
std::vector<std::string> BaseMenu::cmd_options_;
const std::vector<std::string>* BaseMenu::rl_comp_opts_ =
    &BaseMenu::cmd_options_;

::pflib::logging::logger BaseMenu::the_log_ = ::pflib::logging::get("menu");

static void close_history_on_ctrl_C(int s) {
  BaseMenu::close_history();
  exit(130);
}

const char * MENU_HISTORY_FILEPATH = ".pftool-history";

void BaseMenu::open_history() {
  ::using_history();
  if (int rc = ::read_history(MENU_HISTORY_FILEPATH); rc != 0) {
    // error no 2 is non-existent file which makes sense if it doesn't exist yet
    // so we ignore that error number
    if (rc != 2) {
      std::cerr << "warn: failure to read history file " << rc << std::endl;
    }
  }

  struct sigaction sig_int_handler;
  sig_int_handler.sa_handler = close_history_on_ctrl_C;
  sigemptyset(&sig_int_handler.sa_mask);
  sig_int_handler.sa_flags = 0;
  sigaction(SIGINT, &sig_int_handler, NULL);
}

void BaseMenu::close_history() {
  if (int rc = ::write_history(MENU_HISTORY_FILEPATH); rc != 0) {
    std::cerr << "warn: failure to write history file " << rc << std::endl;
  }
}

void BaseMenu::add_to_history(const std::string& cmd) const {
  ::add_history(cmd.c_str());
}

void BaseMenu::add_to_command_queue(const std::string& str) {
  cmdTextQueue_.push_back(str);
}

std::string BaseMenu::readline(const std::string& prompt,
                               const std::string& defval,
                               bool preserve_last_blank) {
  std::string retval;
  std::string trueprompt(prompt);
  if (!defval.empty()) trueprompt += " [" + defval + "] ";

  if (!cmdTextQueue_.empty()) {
    retval = cmdTextQueue_.front();
    if (retval.empty()) retval = defval;
    printf("%s %s\n", trueprompt.c_str(), retval.c_str());

    if (!retval.empty() && retval[0] == '~') {
      retval.erase(0, 1);
      retval.insert(0, getenv("HOME"));
    }
    cmdTextQueue_.pop_front();
  } else {
    char* res = ::readline(trueprompt.c_str());
    retval = std::string(res);
    free(res);
    if (retval.empty())
      retval = defval;
    else if (!preserve_last_blank && isspace(retval[retval.size() - 1]))
      retval.erase(retval.size() - 1);

    if (!retval.empty() && retval[0] == '~') {
      retval.erase(0, 1);
      retval.insert(0, getenv("HOME"));
    }

    std::string rvwork;
    bool lastWasWhite = true;
    for (size_t i = 0; i < retval.size(); i++) {
      if (isspace(retval[i])) {
        if (!lastWasWhite) {
          cmdTextQueue_.push_back(rvwork);
          rvwork = "";
        }
        lastWasWhite = true;
      } else {
        rvwork += retval[i];
        lastWasWhite = false;
      }
    }
    if (!rvwork.empty()) cmdTextQueue_.push_back(rvwork);
    if (!cmdTextQueue_.empty()) {
      retval = cmdTextQueue_.front();
      cmdTextQueue_.pop_front();
    }
  }
  return retval;
}

std::string BaseMenu::readline_nosplit(const std::string& prompt,
                                       const std::string& defval) {
  std::string retval;
  std::string trueprompt(prompt);
  if (!defval.empty()) trueprompt += " [" + defval + "] ";

  char* res = ::readline(trueprompt.c_str());
  retval = std::string(res);
  free(res);

  if (retval.empty())
    retval = defval;
  else if (isspace(retval[retval.size() - 1]))
    retval.erase(retval.size() - 1);

  return retval;
}

std::string BaseMenu::readline(const std::string& prompt) {
  return BaseMenu::readline(prompt, "", false);
}

std::string BaseMenu::readline(const std::string& prompt,
                               const std::vector<std::string>& opts,
                               const std::string& def) {
  auto old_opts = BaseMenu::rl_comp_opts_;
  BaseMenu::rl_comp_opts_ = &opts;
  rl_completion_entry_function = &BaseMenu::matcher;
  auto ret = readline(prompt, def);
  rl_completion_entry_function = NULL;
  BaseMenu::rl_comp_opts_ = old_opts;
  return ret;
}

std::string BaseMenu::output_directory = "";
std::string BaseMenu::timestamp_format = "_%Y%m%d_%H%M%S";

std::string BaseMenu::default_path(const std::string& name, const std::string& extension) {
  time_t t = time(NULL);
  struct tm* tm = localtime(&t);
  char timestamp[64];
  strftime(timestamp, sizeof(timestamp), BaseMenu::timestamp_format.c_str(), tm);

  std::string path_prefix{}; 
  if (not output_directory.empty()) {
    if (output_directory[output_directory.size()-1] != '/') {
      path_prefix = (output_directory + '/');
    } else {
      path_prefix = output_directory;
    }
  }

  return path_prefix + name + timestamp + extension;
}

std::string BaseMenu::readline_path(const std::string& name, const std::string& extension) {
  std::string prompt{"Filename"};
  if (extension.empty()) {
    prompt += " (no extension)";
  }
  prompt += ": ";
  return BaseMenu::readline(prompt, BaseMenu::default_path(name, extension));
}

int BaseMenu::readline_int(const std::string& prompt) {
  return std::stol(BaseMenu::readline(prompt), 0, 0);
}

double BaseMenu::readline_float(const std::string& prompt) {
  return atof(BaseMenu::readline(prompt).c_str());
}

int BaseMenu::readline_int(const std::string& prompt, int aval, bool ashex) {
  char buffer[50];
  if (ashex)
    sprintf(buffer, "0x%x", aval);
  else
    sprintf(buffer, "%d", aval);
  return pflib::str_to_int(BaseMenu::readline(prompt, buffer));
}

bool BaseMenu::readline_bool(const std::string& prompt, bool aval) {
  char buffer[50];
  if (aval)
    sprintf(buffer, "Y");
  else
    sprintf(buffer, "N");
  std::string rv = readline(prompt + " (Y/N) ", buffer);
  return (rv.find_first_of("yY1tT") != std::string::npos);
}

std::string BaseMenu::readline_cmd() { return readline(" > ", cmd_options_); }

char* BaseMenu::matcher(const char* prefix, int state) {
  static int list_index, len;
  char* name;

  if (state == 0) {
    // first call, reset static variables
    list_index = 0;
    len = strlen(prefix);
  }

  while (list_index < rl_comp_opts_->size()) {
    const char* curr_opt{rl_comp_opts_->at(list_index++).c_str()};
    if (strncasecmp(curr_opt, prefix, len) == 0) {
      return strdup(curr_opt);
    }
  }
  return NULL;
}

}
