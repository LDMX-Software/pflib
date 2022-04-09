#ifndef PFLIB_TOOL_MENU_H
#define PFLIB_TOOL_MENU_H

#include <stdio.h>
#include <string.h>

#include <list>
#include <string>
#include <vector>
#include <functional>
#include <iostream>
#include <iomanip>

#ifndef PFLIB_TEST_MENU
#include "pflib/Exception.h"
#endif

/**
 * Type-less menu base for common tasks
 *
 * This base menu class handles most of the interaction with
 * the GNU readline library. This includes some helper commands
 * for obtaining parameters via readline and keeping command
 * history.
 */
class BaseMenu {
 public:
  /**
   * Read in a parameter using the default value if nothing provided
   *
   * Expands '~' into $HOME within strings.
   *
   * If the cmdTextQueue is not empty, then it uses the next command
   * in that list rather than keyboard input. This leads to the
   * effect that executables using this menu can provide an "initialization
   * script" of commands by registering them in order with the
   * BaseMenu::add_to_command_queue before launching the menu.
   *
   * @param[in] prompt The informing the user what the parameter is
   * @param[in] defval default value if user does not provide anything
   * @param[in] preserve_last_blank keep whitespace at end of input
   * @return value input by user or default value
   */
  static std::string readline(const std::string& prompt,
                              const std::string& defval,
                              bool preserve_last_blank = false);

  /**
   * Get a raw input value without the additional splitting and
   * modifications done in base readline.
   *
   * @param[in] prompt The informing the user what the parameter is
   * @param[in] defval default value if user does not provide anything
   * @return value input by user or default value
   */
  static std::string readline_nosplit(const std::string& prompt,
                                      const std::string& defval);
  /**
   * Read a string parameter without a default
   *
   * @see readline with an empty default value
   *
   * @param[in] prompt The informing the user what the parameter is
   * @return value input by user
   */
  static std::string readline(const std::string& prompt);

  /**
   * Read a string parameter without a default but with the input
   * list of options for tab-completion (and validation)
   *
   * @param[in] prompt The informing the user what the parameter is
   * @return value input by user
   */
  static std::string readline(const std::string& prompt, const std::vector<std::string>& opts);

  /**
   * Read an integer parameter without a default
   *
   * Uses string readline and then converts the output string
   * into a integer using stol.
   *
   * @param[in] prompt The informing the user what the parameter is
   * @return value input by user
   */
  static int readline_int(const std::string& prompt);

  /**
   * Read an integer parameter with a default
   *
   * Uses string readline and then converts the output string
   * into a integer using stol.
   *
   * @param[in] prompt The informing the user what the parameter is
   * @param[in] defval default value if user does not provide anything
   * @return value input by user
   */
  static int readline_int(const std::string& prompt, int aval);

  /**
   * Read a float parameter with a default
   *
   * Uses string readline and then converts the output string
   * into a float using atof.
   *
   * @param[in] prompt The informing the user what the parameter is
   * @param[in] defval default value if user does not provide anything
   * @return value input by user
   */
  static double readline_float(const std::string& prompt);

  /**
   * Read a bool parameter with a default
   *
   * The characters y, Y, 1, t, and T are accepted as equivalent
   * to "true" while all others are equivalent to "false". The
   * prompt should be in the form of a yes/no question since the
   * string ' [Y/N]' is appended to it before passing along.
   *
   * @param[in] prompt The informing the user what the parameter is
   * @param[in] defval default value if user does not provide anything
   * @return value input by user
   */
  static bool readline_bool(const std::string& prompt, bool aval);

  /**
   * Read a command from the menu
   *
   * We use the prompt ' > ' before the command and we wrap the 
   * underlying readline call with the setup/teardown of the
   * TAB-completion function that readline calls if the user presses
   * TAB.
   *
   * @return command entered by user
   */
  static std::string readline_cmd();

  /** 
   * Add to the queue of commands to execute automatically 
   *
   * @param[in] str command to add into the queue
   */
  static void add_to_command_queue(const std::string& str);
  
 protected:
  /**
   * Add a command to the history of commands that have been executed.
   *
   * Uses readline's add_to_history.
   *
   * @param[in] cmd Command string that was executed
   */
  void add_to_history(const std::string& cmd) const;

  /// the ordered list of commands to be executed from a script file
  static std::list<std::string> cmdTextQueue_;

  /// the current command options (for interfacing with readline's tab completion)
  static std::vector<std::string> cmd_options_;

  /// a pointer to the list of options when attempting readline completion
  static const std::vector<std::string>* rl_comp_opts_;
 
 private:
  /**
   * matcher function following readline's function signature
   *
   * We get the command options from the BaseMenu::rl_comp_opts_
   * which is determined by the Menu::command_options function
   * at the beginning of Menu::steer or after leaving a sub-menu.
   *
   * We check for matching with strncasecmp so that the tab completion
   * is also case-insensitive (same as the menu selection itself).
   *
   * @param[in] text the text to potentionally match
   * @param[in] state 0 if first call, incrementing otherwise
   * @return matching string until all out, NULL at end
   */
  static char* matcher(const char* text, int state);
};  // BaseMenu

/**
 * A menu to execute commands with a specific target
 *
 * @tparam TargetType class to be passed to execution commands
 */
template <class TargetType>
class Menu : public BaseMenu {
 public:
  /// the type of handle we use to access the target
  using TargetHandle = TargetType*;
  /// type of function which does something with the target
  using SingleTargetCommand = std::function<void(TargetHandle)>;
  /// type of function which does one of several commands with target
  using MultipleTargetCommands = std::function<void(const std::string&, TargetHandle)>;

  /**
   * A command in the menu
   *
   * A menu command can do one of two tasks:
   * 1. Enter a sub-menu
   * 2. Execute a specific command on the target
   *
   * There are two ways to execute a command on the target,
   * the user can provide a function that only does one command
   * on the target (the function signature is SingleTargetCommand)
   * or the user can provide a function that can do multiple
   * commands on a target (the function signature is MultipleTargetCommands)
   * where the first argument to that function will be the name of that
   * Line.
   */
  class Line {
   public:
    /// define a menu line that uses a single target command
    Line(const char* n, const char* d, SingleTargetCommand f)
        : name_(n), desc_(d), sub_menu_(0), cmd_(f), mult_cmds_{0} {}
    /// define a menu line that uses a multiple command function
    Line(const char* n, const char* d, MultipleTargetCommands f)
        : name_(n), desc_(d), sub_menu_(0), mult_cmds_{f} {}
    /// define a menu line that enters a sub menu
    Line(const char* n, const char* d, Menu& m)
        : name_(n), desc_(d), sub_menu_(&m), cmd_(0), mult_cmds_{0} {}
    /**
     * define an empty menu line with only a name and description
     *
     * empty lines are used to exit menus because they will do nothing
     * when execute is called and will leave the do-while loop in Menu::steer
     */
    Line(const char* n, const char* d)
        : name_(n), desc_(d), sub_menu_(0), cmd_(0), mult_cmds_{0} {}
    
    /**
     * Execute this line
     *
     * If this line is a sub-menu, we give the target to that menu's
     * steer function. Otherwise, we execute the function that is
     * available for this line, perferring the single target command.
     *
     * @note Empty menu lines will just do nothing when executed.
     *
     * @param[in] p pointer to target
     * @return true if we were a sub-menu and the command options
     *  need to be reset to the parent menu options
     */
    bool execute(TargetHandle p) const {
      if (sub_menu_) {
        sub_menu_->steer(p);
        return true;
      } else if (cmd_ or mult_cmds_) {
        try {
          if (cmd_) cmd_(p);
          else mult_cmds_(name_,p);
#ifndef PFLIB_TEST_MENU
        } catch(const pflib::Exception& e) {
          std::cerr << " pflib ERR [" << e.name()
            << "] : " << e.message() << std::endl;
#endif
        } catch(const std::exception& e) {
          std::cerr << " Unknown Exception " << e.what() << std::endl;
        }
      }
      // empty and command lines don't need the parent menu
      // to reset the command options
      return false;
    }

    /**
     * Check if this line is an empty one
     */
    bool empty() const {
      return sub_menu_ == 0 and cmd_ == 0 and mult_cmds_ == 0;
    }

    /**
     * add an exit line if this is a menu, otherwise do nothing
     */
    void render() {
      if (sub_menu_) sub_menu_->render();
    }

    /// name of this line to select it
    const char* name() const { return name_; }
    /// short description to print with menu
    const char* desc() const { return desc_; }
    friend std::ostream& operator<<(std::ostream& s, const Line& l) {
      return (s << "  " << std::left << std::setw(12) << l.name() 
                << " " << l.desc());
    }
   private:
    /// the name of this line
    const char* name_;
    /// short description for what this line is
    const char* desc_;
    /// pointer to sub menu (if it exists)
    Menu* sub_menu_;
    /// function pointer to execute (if exists)
    SingleTargetCommand cmd_;
    /// function handling multiple commands to execute (if exists)
    MultipleTargetCommands mult_cmds_;
    /// is this a null line?
    bool is_null;
  };

  uint64_t declare(const char* name, const char* desc, SingleTargetCommand ex) {
    lines_.emplace_back(name,desc,ex);
    return lines_.size();
  }

  uint64_t declare(const char* name, const char* desc, MultipleTargetCommands ex) {
    lines_.emplace_back(name,desc,ex);
    return lines_.size();
  }

  uint64_t declare(const char* name, const char* desc, Menu& ex) {
    lines_.emplace_back(name,desc,ex);
    return lines_.size();
  }

  static Menu& root() {
    static Menu root;
    return root;
  }

  void render() {
    // go through menu options and add exit
    for (auto& l : lines_) l.render();
    lines_.emplace_back("EXIT","leave this menu");
  }

  static void run(TargetHandle tgt) {
    root().render();
    root().steer(tgt);
  }

  /**
   * The type of function used to "render" a menu
   *
   * "rendering" allows the user to have a handle for when the 
   * menu will be printed to the terminal prompt. Moreover,
   * this can include necessary initiliazation procedures
   * if need be.
   */
  using RenderFuncType = void (*)(TargetHandle);

  /**
   * Construct a menu with a rendering function and the input lines
   */
  Menu(std::initializer_list<Line> lines = {}, RenderFuncType f = 0)
      : lines_{lines}, render_func_{f} {}

  /// add a new line to this menu
  void addLine(const Line& line) { lines_.push_back(line); }

  /**
   * give control over the target to this menu
   *
   * We enter a do-while loop that continues until the user selects
   * an empty line to execute. The contents of the do-while loop
   * follow the procedure below.
   *
   * 1. If we have a render function set, we give the target to it.
   * 2. If we aren't in batch mode, we print all of our lines and 
   *    their descriptions.
   * 3. Use the readline function to get the requested command from
   *    the user
   *    - We use the BaseMenu::command_matcher function to provide
   *      options given the user attempting <TAB> completion.
   * 4. Loop through our lines to find all available matches.
   * 5. Execute the line if there is a unique match; otherwise,
   *    print an error message.
   *
   * @param[in] p_target pointer to the target
   */
  void steer(TargetHandle p_target) const;

 private:
  /**
   * Provide the list of command options
   *
   * @return list of commands that could be run from this menu
   */
  virtual std::vector<std::string> command_options() const {
    std::vector<std::string> v;
    v.reserve(lines_.size());
    for (const auto& l : lines_) v.push_back(l.name());
    return v;
  }

 private:
  /// lines in this menu
  std::vector<Line> lines_;
  /// function pointer to render the menu prompt
  RenderFuncType render_func_;
};

template <class TargetType>
void Menu<TargetType>::steer(Menu<TargetType>::TargetHandle p_target) const {
  this->cmd_options_ = this->command_options(); // we are the captain now
  const Line* theMatch = 0;
  do {
    if (render_func_ != 0) {
      this->render_func_(p_target);
    }
    // if cmd text queue is empty, then print menu for interactive user
    if (this->cmdTextQueue_.empty()) {
      printf("\n");
      for (const auto& l : lines_) 
        std::cout << l << std::endl;
    }
    std::string request = readline_cmd();
    theMatch = 0;
    // check for a unique match...
    int nmatch = 0;
    for (size_t i = 0; i < lines_.size(); i++)
      if (strcasecmp(request.c_str(), lines_[i].name()) == 0) {
        theMatch = &(lines_[i]);
        nmatch++;
      }
    if (nmatch > 1) theMatch = 0;
    // ok
    if (theMatch == 0)
      printf("  Command '%s' not understood.\n\n", request.c_str());
    else {
      add_to_history(theMatch->name());
      if (theMatch->execute(p_target)) {
        // resume control when the chosen line was a submenu
        this->cmd_options_ = this->command_options();
      }
    }
  } while (theMatch == 0 or not theMatch->empty());
}

#endif  // PFLIB_TOOL_MENU_H
