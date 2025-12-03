#ifndef PFLIB_TOOL_MENU_H
#define PFLIB_TOOL_MENU_H

#include <stdio.h>
#include <string.h>

#include <functional>
#include <iomanip>
#include <iostream>
#include <list>
#include <string>
#include <vector>

#include "pflib/Exception.h"
#include "pflib/logging/Logging.h"

namespace pflib::menu {

/**
 * Type-less menu base for common tasks
 *
 * This base menu class handles most of the interaction with
 * the GNU readline library. This includes some helper commands
 * for obtaining parameters via readline and keeping command
 * history.
 */
class BaseMenu {
  static std::string history_filepath_;

 public:
  /**
   * Decide where the filepath for reading/writing the history should be
   */
  static void set_history_filepath(std::string fp);

  /**
   * open history and read from file (if it exists)
   *
   * The history is stored at a file defined by set_history_filepath
   * which is updated when the menu exits (either normally or with Ctrl+C).
   */
  static void open_history();

  /**
   * close up history
   */
  static void close_history();

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
   * list of options for tab-completion
   *
   * @param[in] prompt Prompt string to the user
   * @param[in] opts list of options for tab completion of user entry
   * @param[in] def default value if user provides nothing
   * @return value chosen by user
   */
  static std::string readline(const std::string& prompt,
                              const std::vector<std::string>& opts,
                              const std::string& def = "");

  /**
   * Create a default path from the output directory and timestamp format
   * parameters
   *
   * @param[in] name name to be included in output filename before timestamp
   * @param[in] extension extension of file including `.`
   * @return filepath string that looks like
   * [BaseMenu::output_directory/]{name}{timestamp}[extension] where {timestamp}
   * is created from the current time and BaseMenu::timestamp_format
   */
  static std::string default_path(const std::string& name,
                                  const std::string& extension = "");

  /// output directory to include in default path
  static std::string output_directory;

  /**
   * Read a path from the user using default_path to generate a default value.
   *
   * The prompt is "Filename: " if an extension is given or "Filename (no
   * extension):" if not. The arguments are then passed on to
   * BaseMenu::default_path to create a default path for the user to confirm or
   * rewrite if they desire.
   */
  static std::string readline_path(const std::string& name,
                                   const std::string& extension = "");

  /**
   * format of timestamp to append to default path
   *
   * The maximum size of the resulting timestamp is 64 characters,
   * which should be more than enough.
   * A complete specification of time down to the second is 14 characters,
   * so this gives you the ability to add 50 characters of separators and
   * other suffix-related things.
   */
  static std::string timestamp_format;

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
  static int readline_int(const std::string& prompt, int aval,
                          bool ashex = false);

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

  /// the current command options (for interfacing with readline's tab
  /// completion)
  static std::vector<std::string> cmd_options_;

  /// a pointer to the list of options when attempting readline completion
  static const std::vector<std::string>* rl_comp_opts_;

  static ::pflib::logging::logger the_log_;

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
 * Generally, it is a good idea to have a header defining
 * the type of menu you wish to operate for your program.
 * In it, you would have a line like
 * ```cpp
 * using pftool = Menu<PolarfireTarget>;
 * ```
 * so that in the files where you are implementing menu
 * commands the registration is less likely to be messed up.
 * ```cpp
 * // example registration using above alias
 * namespace {
 * auto sb = pftool::menu("SB","example empty submenu");
 * }
 * ```
 * and running the menu takes on a more appealing form.
 * ```cpp
 * // tgt is a handle to the target of the menu
 * // i.e. it is of type pftool::TargetHandle
 * pftool::run(tgt);
 * ```
 *
 * @tparam T class to be passed to execution commands
 */
template <typename T>
class Menu : public BaseMenu {
 public:
  /// the type of target this menu will hold and pass around
  using TargetHandle = T;
  /// type of function which does something with the target
  using SingleTargetCommand = std::function<void(TargetHandle)>;
  /// type of function which does one of several commands with target
  using MultipleTargetCommands =
      std::function<void(const std::string&, TargetHandle)>;

  /**
   * The type of function used to "render" a menu
   *
   * "rendering" allows the user to have a handle for when the
   * menu will be printed to the terminal prompt. Moreover,
   * this can include necessary initiliazation procedures
   * if need be.
   */
  using RenderFuncType = std::function<void(TargetHandle)>;

  /**
   * declare a single target command with thei nput name and desc
   *
   * appends this menu line to us and then returns a pointer to us
   *
   * @param[in] name Name of this line to use when running
   * @param[in] desc short description to print with menu
   * @param[in] ex function to execute when name is called
   * @param[in] category integer flag categorizing this menu line
   * @return pointer to us
   */
  Menu* line(const char* name, const char* desc, SingleTargetCommand ex, unsigned int category = 0) {
    lines_.emplace_back(name, desc, ex, category);
    return this;
  }

  /**
   * declare a single target command with the input name and desc
   *
   * MultipleTargetCommands functions have a string parameter which
   * is given name when run.
   *
   * appends this menu line to us and then returns a pointer to us
   *
   * @param[in] name Name of this line to use when running
   * @param[in] desc short description to print with menu
   * @param[in] ex function to execute when name is called
   * @param[in] category integer flag categorizing this menu line
   * @return pointer to us
   */
  Menu* line(const char* name, const char* desc, MultipleTargetCommands ex, unsigned int category = 0) {
    lines_.emplace_back(name, desc, ex, category);
    return this;
  }

  /**
   * declare a sub menu of us with input name and desc
   *
   * appends this menu line to us and then returns a pointer to us
   *
   * This can be used if you already have a menu and you wish to add
   * another submenu to it.
   * ```cpp
   * namespace {
   * auto mymain = Menu<T>::menu("MAIN","will be on the root menu");
   * auto sb = mymain->submenu("SB","A line in the MAIN menu");
   * }
   * ```
   *
   * @param[in] name Name of this line to use when running
   * @param[in] desc short description to print with menu
   * @param[in] ex pointer to menu that we should append
   * @param[in] category integer flag categorizing this menu line
   * @return pointer to the newly created submenu
   */
  std::shared_ptr<Menu> submenu(const char* name, const char* desc,
                                RenderFuncType f = 0, unsigned int category = 0) {
    auto sb = std::make_shared<Menu>(f);
    lines_.emplace_back(name, desc, sb, category);
    return sb;
  }

  /**
   * Retreve a pointer to the root menu
   */
  static Menu* root() {
    static Menu root;
    return &root;
  }

  /**
   * Go through and build each of the lines in this menu
   *
   * @note No need to add EXIT/QUIT lines!
   *
   * Afterwards, we append the "EXIT" line to us.
   */
  void build() {
    // go through menu options and add exit
    for (Line& l : lines_) l.build();
    lines_.emplace_back("EXIT", "leave this menu");
    lines_.emplace_back("HELP", "print help for this menu",
                        [this](TargetHandle tgt) {
                          this->render(tgt);
                        });
  }

  /**
   * Call this function when ready to run
   *
   * Provide a TargetHandle to run over.
   *
   * After rendering the menu, we give tgt to
   * steer to start off.
   *
   * @param[in] tgt TargetHandle to run with
   */
  static void run(TargetHandle tgt) {
    BaseMenu::open_history();
    root()->build();
    root()->steer(tgt);
    BaseMenu::close_history();
  }

  /// no copying
  Menu(const Menu&) = delete;
  /// no copying
  void operator=(const Menu&) = delete;

  /**
   * Construct a menu with a rendering function
   */
  Menu(RenderFuncType f = 0, unsigned int hidden_categories = 0)
    : render_func_{f}, hidden_categories_{hidden_categories} {}

  /// set hidden categories
  void hide(unsigned int categories) {
    hidden_categories_ = categories;
    for (auto& l : lines_) {
      l.hide(categories);
    }
  }

  /**
   * Print menu without running it
   */
  void print(std::ostream& s, int indent = 0) const {
    for (const auto& l : lines_) {
      if ((l.category() & hidden_categories_) == 0) {
        l.print(s, indent);
      }
    }
  }

  /**
   * render this menu to the user
   *
   * This is its own function because it is called when
   * we enter a menu and if the user wants to print the HELP
   * command
   * 1. call the user-provided render function
   * 2. printout the list of commands in this menu
   */
  void render(TargetHandle tgt) const {
    if (render_func_) {
      render_func_(tgt);
    }
    std::cout << "\n";
    for (const auto& l : lines_) {
      if ((l.category() & hidden_categories_) == 0) {
        std::cout << l << std::endl;
      }
    }
  }

  /**
   * entered this menu
   *
   * There are a few tasks we do when entering this menu:
   * 1. copy our command_options into the static command options
   *    for tab completion
   * 2. call the user-provided render function
   * 3. printout the list of commands in this menu
   *
   * @see Menu::render for how the last two are done.
   *
   * These tasks are only done if the command queue is empty
   * signalling that we are not in a script.
   */
  void enter(TargetHandle tgt) const {
    if (cmdTextQueue_.empty()) {
      // copy over command options for tab complete
      this->cmd_options_ = this->command_options();
      this->render(tgt);
    }
  }

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
  void steer(TargetHandle p_target) const {
    enter(p_target);
    const Line* theMatch = 0;
    do {
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
          enter(p_target);
        }
      }
    } while (theMatch == 0 or not theMatch->empty());
  }

  /**
   * Construct a new menu with the root as parent and optional rendering
   *
   * After using submenu to attach ourselves to the input parent,
   * we return a pointer to the newly constructed menu.
   * This is to allow chaining during the declaration of a menu,
   * so that the declaration of a menu has a form similar to what
   * the menu looks like at run time.
   *
   * A simple example (actually using the shorter alias) is below.
   * `one_func` and `two_func` are both functions accessible by this
   * piece of code (either via a header or in this translation unit)
   * and are `void` functions with take a handle to the target type.
   * `color_func` is similarly accessible but it takes a string and
   * the target handle. The string will be the command that is selected
   * at runtime (e.g. "RED" if it is typed by the user)
   * ```cpp
   * namespace {
   * auto sb = Menu<T>::menu("SB","example submenu")
   *   ->line("ONE", "one command in this menu", one_func)
   *   ->line("TWO", "two command in this menu", two_func)
   *   ->line("RED", "can have multiple lines directed to the same func",
   * color_func)
   *   ->line("BLUE", "see?" , color_func)
   * ;
   * }
   * ```
   * The anonymous namespace is used to force the variable within it to
   * be static and therefore created at library linking time.
   * The dummy variable is (unfortunately) necessary so that the sub
   * menu can maintain existence throughout the runtime of the program.
   *
   * @see submenu for attaching to another menu directly
   * @param[in] name Name of this sub menu for selection
   * @param[in] desc description of this sub menu
   * @param[in] render_func function to use to render this sub menu
   * @return pointer to newly created menu
   */
  static std::shared_ptr<Menu> menu(const char* name, const char* desc,
                                    RenderFuncType render_func = 0) {
    return root()->submenu(name, desc, render_func);
  }

 private:
  /**
   * Provide the list of command options
   *
   * @return list of commands that could be run from this menu
   */
  virtual std::vector<std::string> command_options() const {
    std::vector<std::string> v;
    v.reserve(lines_.size());
    for (const auto& l : lines_) {
      if ((l.category() & hidden_categories_) == 0) {
        v.push_back(l.name());
      }
    }
    return v;
  }

 private:
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
    Line(const char* n, const char* d, SingleTargetCommand f, unsigned int category = 0)
        : name_(n), desc_(d), sub_menu_{nullptr}, cmd_(f), mult_cmds_{0}, category_{category} {}
    /// define a menu line that uses a multiple command function
    Line(const char* n, const char* d, MultipleTargetCommands f, unsigned int category = 0)
        : name_(n), desc_(d), sub_menu_{nullptr}, mult_cmds_{f}, category_{category} {}
    /// define a menu line that enters a sub menu
    Line(const char* n, const char* d, std::shared_ptr<Menu> m, unsigned int category = 0)
        : name_(n), desc_(d), sub_menu_(m), cmd_(0), mult_cmds_{0}, category_{category} {}
    /**
     * define an empty menu line with only a name and description
     *
     * empty lines are used to exit menus because they will do nothing
     * when execute is called and will leave the do-while loop in Menu::steer
     */
    Line(const char* n, const char* d)
        : name_(n), desc_(d), sub_menu_{nullptr}, cmd_(0), mult_cmds_{0}, category_{0} {}

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
          if (cmd_)
            cmd_(p);
          else
            mult_cmds_(name_, p);
        } catch (const pflib::Exception& e) {
          pflib_log(error) << "[" << e.name() << "] : " << e.message();
        } catch (const std::exception& e) {
          pflib_log(error) << "Unknown Exception " << e.what();
        }
      }
      // empty and command lines don't need the parent menu
      // to reset the command options
      return false;
    }

    /**
     * Check if this line is an empty one
     */
    bool empty() const { return !sub_menu_ and cmd_ == 0 and mult_cmds_ == 0; }

    /**
     * build this line
     *
     * we only need to do something if this line is a sub-menu
     * in which we case we call build on the submenu.
     */
    void build() {
      if (sub_menu_) sub_menu_->build();
    }

    /**
     * pass on configuration on which categories to hide
     */
    void hide(unsigned int categories) {
      if (sub_menu_) sub_menu_->hide(categories);
    }

    /// name of this line to select it
    const char* name() const { return name_; }
    /// short description to print with menu
    const char* desc() const { return desc_; }
    /// category int for hiding if certain categories are disabled
    unsigned int category() const { return category_; }

    /**
     * Overload output stream operator for easier printing
     */
    friend std::ostream& operator<<(std::ostream& s, const Line& l) {
      return (s << "  " << std::left << std::setw(12) << l.name() << " "
                << l.desc());
    }

    /**
     * More specialized printing function to make it easier to recursively
     * printout entire menu with descriptions.
     */
    void print(std::ostream& s, int indent = 0) const {
      for (std::size_t i{0}; i < indent; i++) s << " ";
      s << *this << "\n";
      if (sub_menu_) {
        sub_menu_->print(s, indent + 2);
      }
    }

   private:
    /// the name of this line
    const char* name_;
    /// short description for what this line is
    const char* desc_;
    /// pointer to sub menu (if it exists)
    std::shared_ptr<Menu> sub_menu_;
    /// function pointer to execute (if exists)
    SingleTargetCommand cmd_;
    /// function handling multiple commands to execute (if exists)
    MultipleTargetCommands mult_cmds_;
    /// category integer for disabling menu lines by groups
    unsigned int category_;
  };  // Line

 private:
  /// lines in this menu
  std::vector<Line> lines_;
  /// function pointer to render the menu prompt
  RenderFuncType render_func_;
  /// bit-wise OR of any category integers that should not be displayed
  unsigned int hidden_categories_;
};  // Menu

}  // namespace pflib::menu

#endif  // PFLIB_TOOL_MENU_H
