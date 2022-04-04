#include "Menu.h"

#include <readline/readline.h>
#include <readline/history.h>

#include <iostream>

#ifdef PFLIB_TEST_MENU
#include <string> // for stol
#else
#include "pflib/Compile.h" // for str_to_int
#endif

std::list<std::string> BaseMenu::cmdTextQueue_;
std::vector<std::string> BaseMenu::cmd_options_;

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
    if (retval.empty()) retval=defval;
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

int BaseMenu::readline_int(const std::string& prompt) {
#ifdef PFLIB_TEST_MENU
  return std::stol(BaseMenu::readline(prompt),0,0);
#else
  return pflib::str_to_int(BaseMenu::readline(prompt));
#endif
}

double BaseMenu::readline_float(const std::string& prompt) {
  return atof(BaseMenu::readline(prompt).c_str());
}

int BaseMenu::readline_int(const std::string& prompt, int aval) {
  char buffer[50];
  sprintf(buffer, "%d", aval);
#ifdef PFLIB_TEST_MENU
  return std::stol(BaseMenu::readline(prompt,buffer),0,0);
#else
  return pflib::str_to_int(BaseMenu::readline(prompt,buffer));
#endif
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

std::string BaseMenu::readline_cmd() {
  rl_completion_entry_function = &BaseMenu::command_matcher;
  std::string cmd = readline(" > ");
  rl_completion_entry_function = NULL;
  return cmd;
}

char *BaseMenu::command_matcher(const char* prefix, int state) {
  static int list_index, len;
  char *name; 

  if (state == 0) {
    // first call, reset static variables
    list_index = 0;
    len = strlen(prefix);
  }

  while (list_index < cmd_options_.size()) {
    const char* curr_opt{cmd_options_.at(list_index++).c_str()};
    if (strncasecmp(curr_opt, prefix, len) == 0) {
      return strdup(curr_opt);
    }
  }
  return NULL;
}
