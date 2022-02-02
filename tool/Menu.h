#ifndef Menu_h_included_
#define Menu_h_included_

#include <string>
#include <vector>
#include <list>
#include <stdio.h>
#include <string.h>

  
static bool quiet_batch_mode=false;

class BaseMenu {
 public:
  static std::string readline(const std::string& prompt, const std::string& defval, bool preserve_last_blank=false);
  static std::string readline(const std::string& prompt) ; 
  static std::string readline_nosplit(const std::string& prompt, const std::string& defval);
  static int readline_int(const std::string& prompt) ; 
  static int readline_int(const std::string& prompt, int aval) ; 
  static double readline_float(const std::string& prompt);
  static bool readline_bool(const std::string& prompt,bool aval);
 protected:
  void add_to_history(const std::string& cmd);
  static std::list<std::string> cmdTextQueue_;
};

template<class target_T>
class Menu : public BaseMenu {
public:  
  class Line {
  public:
    Line(const char* n, const char* d, void (*f)( target_T* p_target )) : name(n),desc(d),subMenu(0),func(f),func2(0), isNull(false) { }
    Line(const char* n, const char* d, void (*f2)( const std::string& cmd, target_T* p_target )) : name(n),desc(d),subMenu(0),func(0),func2(f2), isNull(false) { }
    Line(const char* n, const char* d, Menu* sb ) : name(n),desc(d),subMenu(sb), func(0),func2(0), isNull(false) { }
    Line(const char* n, const char* d) : name(n),desc(d),subMenu(0), func(0),func2(0), isNull(false) { }
    Line() : isNull(true) { }
    bool null() const { return isNull; }
    const char* name;
    const char* desc;
    Menu* subMenu;
    void (*func)( target_T* p_target ) ;
    void (*func2)( const std::string& cmd, target_T* p_target ) ;
  private:
    bool isNull;
  };
  
  Menu(void (*renderf)( target_T* p_target ), const Line* tlines) : renderFunc(renderf) { 
    for (size_t i=0; !tlines[i].null(); i++) lines.push_back(tlines[i]);
  }
  
  Menu(const Line* tlines) : renderFunc(0) {
    for (size_t i=0; !tlines[i].null(); i++) lines.push_back(tlines[i]);
  }
  
  void addLine(const Line& line) { lines.push_back(line); }
  
  void (*renderFunc)( target_T* p_target );
  
  std::vector<Line> lines;
  
  void steer( target_T* p_target  ) ;
};

template<class target_T>
void Menu<target_T>::steer( target_T* p_target ) {
  
  const Line* theMatch=0;
  do {
    printf("\n");
    if (renderFunc!=0) {
      this->renderFunc( p_target ) ;
    }
    if (!quiet_batch_mode) 
      for (size_t i=0; i<lines.size(); i++) {
	printf("   %-12s %s\n",lines[i].name,lines[i].desc);	       
      }
    std::string request=readline(" > ");
    theMatch=0;
    // check for a unique match...
    int nmatch=0;
    for (size_t i=0; i<lines.size(); i++) 
      if (strncasecmp(request.c_str(),lines[i].name,request.length())==0) {
	theMatch=&(lines[i]);
	nmatch++;
      }
    if (nmatch>1) theMatch=0;
    // ok
    if (theMatch==0) printf("  Command '%s' not understood.\n\n",request.c_str());
    else add_to_history(theMatch->name);
    // actions
    if (theMatch!=0 && theMatch->subMenu!=0)          theMatch->subMenu->steer( p_target );
    else if (theMatch!=0 && theMatch->func!=0 )       {
      try {
	theMatch->func( p_target ) ;
      } catch (std::exception& e) {
	printf("  Exception: %s\n",e.what());
      }
    } else if (theMatch!=0 && theMatch->func2!=0 )       {
      try  {
	theMatch->func2( theMatch->name, p_target ) ;
      } catch (std::exception& e) {
       	printf("  Exception: %s\n",e.what());
      }
    }
  } while (theMatch==0 || theMatch->subMenu!=0 || (theMatch->func!=0 || theMatch->func2 !=0) );
}


#endif // Menu_h_included_
