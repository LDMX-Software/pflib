#ifndef PFLIB_TOOL_MENU_H
#define PFLIB_TOOL_MENU_H

#include <stdio.h>
#include <string.h>

#include <list>
#include <string>
#include <vector>
#include <functional>
#include <iostream>

#include "pflib/Exception.h"

/**
 * Compile-time constant for determining verbosity
 */
static bool quiet_batch_mode = false;

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

 protected:
  /**
   * Add a command to the history of commands that have been executed.
   *
   * Uses readline's add_to_history.
   *
   * @param[in] cmd Command string that was executed
   */
  void add_to_history(const std::string& cmd) const;

  /// the ordered list of commands that have been executed
  static std::list<std::string> cmdTextQueue_;
};  // BaseMenu

/**
 * A menu to execute commands with a specific target
 *
 * @tparam TargetType class to be passed to execution commands
 */
template <class TargetType>
class Menu : public BaseMenu {
 public:
  /// type of function which does something with the target
  using SingleTargetCommand = std::function<void(TargetType*)>;
  /// type of function which does one of several commands with target
  using MultipleTargetCommands = std::function<void(const std::string&, TargetType*)>;

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
        : name_(n), desc_(d), sub_menu_(0), cmd_(f) {}
    /// define a menu line that uses a multiple command function
    Line(const char* n, const char* d, MultipleTargetCommands f)
        : name_(n), desc_(d), sub_menu_(0), 
          cmd_([&](TargetType* p) { f(this->name_, p); }) {}
    /// define a menu line that enters a sub menu
    Line(const char* n, const char* d, Menu* m)
        : name_(n), desc_(d), sub_menu_(m), cmd_(0) {}
    /**
     * define an empty menu line with only a name and description
     *
     * empty lines are used to exit menus because they will do nothing
     * when execute is called and will leave the do-while loop in Menu::steer
     */
    Line(const char* n, const char* d)
        : name_(n), desc_(d), sub_menu_(0), cmd_(0) {}
    
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
     */
    void execute(TargetType* p) const {
      if (sub_menu_) {
        sub_menu_->steer(p);
      } else {
        try {
          if (cmd_) cmd_(p);
        } catch(const pflib::Exception& e) {
          std::cerr << " pflib ERR [" << e.name()
            << "] : " << e.message() << std::endl;
        } catch(const std::exception& e) {
          std::cerr << " Unknown Exception " << e.what() << std::endl;
        }
      }
    }

    /**
     * Check if this line is an empty one
     */
    bool empty() const {
      return sub_menu_ == 0 and cmd_ == 0;
    }

    const char* name() const { return name_; }
    const char* desc() const { return desc_; }
   private:
    /// the name of this line
    const char* name_;
    /// short description for what this line is
    const char* desc;
    /// pointer to sub menu (if it exists)
    Menu* sub_menu_;
    /// function pointer to execute (if exists)
    SingleTargetCommand cmd_;
    /// is this a null line?
    bool is_null;
  };

  /**
   * The type of function used to "render" a menu
   *
   * "rendering" allows the user to have a handle for when the 
   * menu will be printed to the terminal prompt. Moreover,
   * this can include necessary initiliazation procedures
   * if need be.
   */
  using RenderFuncType = void (*)(TargetType*);

  /**
   * Construct a menu with a rendering function and the input lines
   */
  Menu(std::initializer_list<Line> lines, RenderFuncType f = 0)
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
   * 4. Loop through our lines to find all available matches.
   * 5. Execute the line if there is a unique match; otherwise,
   *    print an error message.
   *
   * @param[in] p_target pointer to the target
   */
  void steer(TargetType* p_target) const;

 private:
  /// lines in this menu
  std::vector<Line> lines_;
  /// function pointer to render the menu prompt
  RenderFuncType render_func_;
};

template <class TargetType>
void Menu<TargetType>::steer(TargetType* p_target) const {
  const Line* theMatch = 0;
  do {
    printf("\n");
    if (render_func_ != 0) {
      this->render_func_(p_target);
    }
    if (!quiet_batch_mode)
      for (size_t i = 0; i < lines_.size(); i++) {
        printf("   %-12s %s\n", lines_[i].name(), lines_[i].desc());
      }
    std::string request = readline(" > ");
    theMatch = 0;
    // check for a unique match...
    int nmatch = 0;
    for (size_t i = 0; i < lines_.size(); i++)
      if (strncasecmp(request.c_str(), lines_[i].name(), request.length()) == 0) {
        theMatch = &(lines_[i]);
        nmatch++;
      }
    if (nmatch > 1) theMatch = 0;
    // ok
    if (theMatch == 0)
      printf("  Command '%s' not understood.\n\n", request.c_str());
    else {
      add_to_history(theMatch->name());
      theMatch->execute(p_target);
    }
  } while (theMatch != 0 and not theMatch->empty());
}

#endif  // PFLIB_TOOL_MENU_H
