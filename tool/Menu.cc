#include "pflib/Exception.h"
#include <iostream>
#include "Menu.h"
#include <malloc.h>
#include <readline/history.h>
#include <readline/readline.h>

#include "pflib/Compile.h" // for str_to_int

std::list<std::string> BaseMenu::cmdTextQueue_;

void BaseMenu::add_to_history(const std::string& cmd) {
  ::add_history(cmd.c_str());
}

std::string BaseMenu::readline(const std::string& prompt,
                               const std::string& defval,
                               bool preserve_last_blank) {
  std::string retval;
  std::string trueprompt(prompt);
  if (!defval.empty()) trueprompt += " [" + defval + "] ";

  if (!cmdTextQueue_.empty()) {
    retval = cmdTextQueue_.front();
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
  return pflib::str_to_int(BaseMenu::readline(prompt).c_str(), 0, 0);
}

double BaseMenu::readline_float(const std::string& prompt) {
  return atof(BaseMenu::readline(prompt).c_str());
}

int BaseMenu::readline_int(const std::string& prompt, int aval) {
  char buffer[50];
  sprintf(buffer, "%d", aval);
  return pflib::str_to_int(BaseMenu::readline(prompt, buffer).c_str(), 0, 0);
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
