#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>
#include <readline/readline.h>
#include <readline/history.h>
#include <malloc.h>
#include <memory>
#include <map>
#include <iomanip>
#include <stdlib.h>
#include <string>
#include <list>
#include <exception>
#include "pflib/Hcal.h"
#include "pflib/ROC.h"
#include "pflib/Elinks.h"
#include "pflib/Bias.h"
#include "pflib/FastControl.h"
#include "pflib/Backend.h"
#include "pflib/rogue/RogueWishboneInterface.h"

/**
 * Modern RAII-styled CSV extractor taken from
 * https://stackoverflow.com/a/1120224/17617632
 * this allows us to **discard white space within the cells**
 * making the CSV more human readable.
 */
static std::vector<int> getNextLineAndExtractValues(std::istream& ss) {
  std::vector<int> result;

  std::string line, cell;
  std::getline(ss, line);

  if (line.empty() or line[0] == '#') {
    // empty line or comment, return empty container
    return result;
  }

  std::stringstream line_stream(line);
  while (std::getline(line_stream, cell, ',')) {
    /**
     * std stoi has a auto-detect base feature
     * https://en.cppreference.com/w/cpp/string/basic_string/stol
     * which we can enable by setting the pre-defined base to 0
     * (the third parameter).
     * The second parameter is and address to put the number of characters processed,
     * which I disregard at this time.
     *
     * Do we allow empty cells?
     */
    result.push_back(cell.empty() ? 0 : std::stoi(cell,nullptr,0));
  }
  // checks for a trailing comma with no data after it
  if (!line_stream and cell.empty()) {
    // trailing comma, put in one more 0
    result.push_back(0);
  }

  return result;
}

struct PolarfireTarget {
  pflib::WishboneInterface* wb;
  pflib::Hcal* hcal;
  pflib::Backend* backend;
};

static std::string tool_readline(const std::string& prompt) ; 
static std::string tool_readline(const std::string& prompt, const std::string& defval);
static std::string tool_readline_nosplit(const std::string& prompt, const std::string& defval);
static int tool_readline_int(const std::string& prompt) ; 
static int tool_readline_int(const std::string& prompt, int aval) ; 

static bool tool_readline_bool(const std::string& prompt,bool aval) {
  char buffer[50];
  if (aval) sprintf(buffer,"Y");
  else sprintf(buffer,"N");
  std::string rv=tool_readline(prompt+" (Y/N) ",buffer);
  return (rv.find_first_of("yY1tT")!=std::string::npos);
}

static bool quiet_batch_mode=false;

template<class ctr_T>
class Menu {
public:
	
  class Line {
  public:
    Line(const char* n, const char* d, void (*f)( ctr_T* mCTR_ )) : name(n),desc(d),subMenu(0),func(f),func2(0), isNull(false) { }
    Line(const char* n, const char* d, void (*f2)( const std::string& cmd, ctr_T* mCTR_ )) : name(n),desc(d),subMenu(0),func(0),func2(f2), isNull(false) { }
    Line(const char* n, const char* d, Menu* sb ) : name(n),desc(d),subMenu(sb), func(0),func2(0), isNull(false) { }
    Line(const char* n, const char* d) : name(n),desc(d),subMenu(0), func(0),func2(0), isNull(false) { }
    Line() : isNull(true) { }
    bool null() const { return isNull; }
    const char* name;
    const char* desc;
    Menu* subMenu;
    void (*func)( ctr_T* mCTR_ ) ;
    void (*func2)( const std::string& cmd, ctr_T* mCTR_ ) ;
  private:
    bool isNull;
  };
  
  Menu(void (*renderf)( ctr_T* mCTR_ ), const Line* tlines) : renderFunc(renderf) { 
    for (size_t i=0; !tlines[i].null(); i++) lines.push_back(tlines[i]);
  }
  
  Menu(const Line* tlines) : renderFunc(0) {
    for (size_t i=0; !tlines[i].null(); i++) lines.push_back(tlines[i]);
  }
  
  void addLine(const Line& line) { lines.push_back(line); }
  
  void (*renderFunc)( ctr_T* mCTR_ );
  
  std::vector<Line> lines;
  
  void steer( ctr_T* mCTR_  ) ;
};

template<class ctr_T>
void Menu<ctr_T>::steer( ctr_T* mCTR_ ) {
  
  const Line* theMatch=0;
  do {
    printf("\n");
    if (renderFunc!=0) {
      this->renderFunc( mCTR_ ) ;
    }
    if (!quiet_batch_mode) 
      for (size_t i=0; i<lines.size(); i++) {
	printf("   %-12s %s\n",lines[i].name,lines[i].desc);	       
      }
    std::string request=tool_readline(" > ");
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
    else add_history(theMatch->name);
    // actions
    if (theMatch!=0 && theMatch->subMenu!=0)          theMatch->subMenu->steer( mCTR_ );
    else if (theMatch!=0 && theMatch->func!=0 )       {
      try {
	theMatch->func( mCTR_ ) ;
      } catch (std::exception& e) {
	printf("  Exception: %s\n",e.what());
      }
    } else if (theMatch!=0 && theMatch->func2!=0 )       {
      try  {
	theMatch->func2( theMatch->name, mCTR_ ) ;
      } catch (std::exception& e) {
       	printf("  Exception: %s\n",e.what());
      }
    }
  } while (theMatch==0 || theMatch->subMenu!=0 || (theMatch->func!=0 || theMatch->func2 !=0) );
}

typedef Menu<PolarfireTarget> uMenu ;

static void RunMenu( PolarfireTarget* pft_  )  ;

static void ldmx_status( PolarfireTarget* pft );
static void wb_action( const std::string& cmd, PolarfireTarget* pft );
static void ldmx_i2c( const std::string& cmd, PolarfireTarget* pft );
static void ldmx_link( const std::string& cmd, PolarfireTarget* pft );
static void ldmx_fc( const std::string& cmd, PolarfireTarget* pft );
static void ldmx_bias( const std::string& cmd, PolarfireTarget* pft );
static void ldmx_daq( const std::string& cmd, PolarfireTarget* pft );
static void ldmx_elinks( const std::string& cmd, PolarfireTarget* pft );
static void ldmx_roc( const std::string& cmd, PolarfireTarget* pft );
static void ldmx_roc_render( PolarfireTarget* pft );
static void ldmx_daq_debug( const std::string& cmd, PolarfireTarget* pft );

static void loadOptions(const std::string& optfile);
static std::string getOpt(const std::string& opt, const std::string& def);

static std::map<std::string,std::string> Options;
static const double clockFreq=1.6e9; // Hz
static int mId = 0 ;
static std::list<std::string> cmdTextQueue;

//**tool_readline functions
//helper functions for Menu to handle user input
//works for _float and _int. Parameters (prompt, defval={"",0} , bool preserve_last_blank = false)
//overloaded so you can leave out the last param or last 2 params

/*
static void write_buffer_to_file(std::vector<uint32_t>& buffer) {
  std::string of=tool_readline("Save to filename (blank for no save) : ");
  if (!of.empty()) {
    FILE* f=fopen(of.c_str(),"wt");
    if (f==0) printf("Unable to open '%s' for writing\n\n",of.c_str());
    else {
      for (size_t i=0; i<buffer.size(); i++) {
	fprintf(f,"%5d %08x\n",int(i),buffer[i]);
      }   
      fclose(f);
    }
  }
}
*/

static std::string tool_readline(const std::string& prompt, const std::string& defval, bool preserve_last_blank) {
  std::string retval;
  std::string trueprompt(prompt);
  if (!defval.empty()) trueprompt+=" ["+defval+"] ";


  if (!cmdTextQueue.empty()) {
    retval=cmdTextQueue.front();
    printf("%s %s\n",trueprompt.c_str(),retval.c_str());

    if (!retval.empty() && retval[0]=='~') {
      retval.erase(0,1);
      retval.insert(0,getenv("HOME"));
    }
    cmdTextQueue.pop_front();
  } else {
    char* res=readline(trueprompt.c_str());
    retval=std::string(res);
    free(res);
    if (retval.empty()) retval=defval;
    else if (!preserve_last_blank && isspace(retval[retval.size()-1])) retval.erase(retval.size()-1);

    if (!retval.empty() && retval[0]=='~') {
      retval.erase(0,1);
      retval.insert(0,getenv("HOME"));
    }
    
    std::string rvwork;
    bool lastWasWhite=true;
    for (size_t i=0; i<retval.size(); i++) {
      if (isspace(retval[i])) {
	if (!lastWasWhite) {
	  cmdTextQueue.push_back(rvwork);
	  rvwork="";
	}	  
	lastWasWhite=true;
      } else {
	rvwork+=retval[i];
	lastWasWhite=false;
      }
    }
    if (!rvwork.empty()) cmdTextQueue.push_back(rvwork);
    if (!cmdTextQueue.empty()) {
      retval=cmdTextQueue.front();
      cmdTextQueue.pop_front();    
    }
  }
  return retval;
}


static std::string tool_readline_nosplit(const std::string& prompt, const std::string& defval) {
  std::string retval;
  std::string trueprompt(prompt);
  if (!defval.empty()) trueprompt+=" ["+defval+"] ";


  char* res=readline(trueprompt.c_str());
  retval=std::string(res);
  free(res);
  
  if (retval.empty()) retval=defval;
  else if (isspace(retval[retval.size()-1])) retval.erase(retval.size()-1);

  return retval;
}


static std::string tool_readline(const std::string& prompt, const std::string& defval) { return tool_readline(prompt,defval,false); }
static std::string tool_readline(const std::string& prompt) { return tool_readline(prompt,"",false); }

static int tool_readline_int(const std::string& prompt) {
  return strtol(tool_readline(prompt).c_str(),0,0);
}

double tool_readline_float(const std::string& prompt) {
  return atof(tool_readline(prompt).c_str());
}

int tool_readline_int(const std::string& prompt,int aval) {
  char buffer[50];
  sprintf(buffer,"%d",aval);
  return strtol(tool_readline(prompt,buffer).c_str(),0,0);
}

void loadOptions(const std::string& optfile) {
  FILE* f=fopen(optfile.c_str(),"r");
  if (f==0) return;
  char buffer[2048];
  
  while (!feof(f)) {
    buffer[0]=0;
    fgets(buffer,2047,f);
    if (strchr(buffer,'#')!=0) (*strchr(buffer,'#'))=0; // trim out comments
    if (!strchr(buffer,'=')) continue; // no equals sign.
    std::string key;
    int i;
    for (i=0; buffer[i]!='='; i++)
      if (!isspace(buffer[i])) key+=toupper(buffer[i]);
    std::string value;
    bool quoted=false;
    for (i++; buffer[i]!=0; i++) 
      if (buffer[i]=='"') quoted=!quoted;
      else if (!isspace(buffer[i]) || quoted) value+=buffer[i];
    Options[key]=value;
  }
  fclose(f);
}


std::string getOpt(const std::string& opt, const std::string& def) {
  std::map<std::string,std::string>::const_iterator i=Options.find(opt);
  if (i==Options.end()) return def;
  else return i->second;
}

int main(int argc, char* argv[]) {
   //if not enough arguments output usage
  if (argc<2 || !strcmp(argv[1],"-h")) {
    printf("Usage: pftool [hostname/ip for bridge] [-u] [-s script] [-o host_uri]\n");
    printf("   -ho : help on options settable in the rc file\n");
    printf("   Loads options file from ${HOME}/.uhtrtoolrc and ${PWD}/.uhtrtoolrc\n");
    printf("\n");
    return 1;
  }
  
  std::string home=getenv("HOME");
  std::vector<std::string> ipV ;

  ipV.clear() ;
  std::string line;

 try {

    static int sz = argc ;
    bool skip[sz] ; 
    for ( int i =0 ; i < argc; i++)  {  skip[i] = false ; }
    
    for ( int i = 1 ; i < argc ; i++ ) {
        if ( skip[i] ) continue ;

        ipV.push_back( argv[i] ) ;
    }
    

    bool exitMenu = false ;
    do {
      
      do {
	
	if ( ipV.size()==0) {
	  std::cout << "No IP's loaded" << std::endl;
	  mId = -1;
	  break;
	}


	if ( ipV.size()==1) {
	  mId = 0;
	} else {
	
	  for ( size_t k=0; k< ipV.size(); k++) {  
	    //std::cout<<" ("<<k<<")  IP["<< ipV[k] <<"]  Type:"<< typeV[k] << std::endl ; 
	    printf(" ID[%d] IP[%s]  Type: %s \n", (int)k, ipV[k].c_str(),"PolarfireTarget" ) ;
	  }
	
	  mId = tool_readline_int(" ID of pft (-1 for exiting the tool) :: ", mId );
	}
	
	if( (mId < int(ipV.size()) && mId >= 0) || mId == -1)
	  break;
	
	//mId = tool_readline_int(" ID of mCTR/pft ( -1 for exiting the tool ) : ", mId );
	
	std::cout << "Not a Valid ID\n";
      } while (1);
      
      if ( mId == -1 ) break ;
      
      PolarfireTarget pft_;
      pflib::rogue::RogueWishboneInterface wbi(ipV[mId],5970);
      pft_.wb=&wbi;
      pft_.hcal=new pflib::Hcal(pft_.wb);
      pft_.backend=&wbi;
      ldmx_status(&pft_);
      RunMenu( &pft_  ) ;

      if (ipV.size()>1)  {      
	static std::string RunOrExit = "Exit" ;
	RunOrExit = tool_readline(" Choose a new card(new) or Exit(exit) ? ", RunOrExit );
	if (strncasecmp( RunOrExit.c_str(), "NEW",  1)==0) exitMenu = false ;
	if (strncasecmp( RunOrExit.c_str(), "EXIT", 1)==0) exitMenu = true ;
      } else exitMenu=true;
    } while( !exitMenu ) ;
    
  } catch (std::exception& e) {
    fprintf(stderr, "Exception!  %s\n",e.what());
  }
  return 0;
}

void RunMenu( PolarfireTarget* pft_ ) {

uMenu::Line menu_wb_lines[] = { 
  uMenu::Line("RESET", "Enable/disable (toggle)",  &wb_action ),
  uMenu::Line("READ", "Read from an address",  &wb_action ),
  uMenu::Line("BLOCKREAD", "Read several words starting at an address",  &wb_action ),
  uMenu::Line("WRITE", "Write to an address",  &wb_action ),
  uMenu::Line("STATUS", "Wishbone errors counters",  &wb_action ),
  uMenu::Line("QUIT","Back to top menu"),
  uMenu::Line()
};

uMenu menu_wishbone(menu_wb_lines);

uMenu::Line menu_ldmx_i2c_lines[] = {
  uMenu::Line("BUS","Pick the I2C bus to use", &ldmx_i2c ),
  uMenu::Line("READ", "Read from an address",  &ldmx_i2c ),
  uMenu::Line("WRITE", "Write to an address",  &ldmx_i2c ),
  uMenu::Line("MULTIREAD", "Read from an address",  &ldmx_i2c ),
  uMenu::Line("MULTIWRITE", "Write to an address",  &ldmx_i2c ),
  uMenu::Line("QUIT","Back to top menu"),
  uMenu::Line()
};

uMenu menu_ldmx_i2c(menu_ldmx_i2c_lines);

uMenu::Line menu_ldmx_link_lines[] = {
  uMenu::Line("STATUS","Dump link status", &ldmx_link ),
  uMenu::Line("CONFIG","Setup link", &ldmx_link ),
  uMenu::Line("SPY", "Spy on the uplink",  &ldmx_link ),
  uMenu::Line("QUIT","Back to top menu"),
  uMenu::Line()
};

uMenu menu_ldmx_link(menu_ldmx_link_lines);

uMenu::Line menu_ldmx_elinks_lines[] = {
  uMenu::Line("HARD_RESET","Hard reset of the PLL", &ldmx_elinks),
  uMenu::Line("STATUS", "Elink status summary",  &ldmx_elinks ),
  uMenu::Line("SPY", "Spy on an elink",  &ldmx_elinks ),
  uMenu::Line("BITSLIP", "Set the bitslip for a link or turn on auto", &ldmx_elinks),
  uMenu::Line("SCAN", "Scan on an elink",  &ldmx_elinks ),
  uMenu::Line("DELAY", "Set the delay on an elink", &ldmx_elinks),
  uMenu::Line("BIGSPY", "Take a spy of a specific channel at 32-bits", &ldmx_elinks),
  uMenu::Line("QUIT","Back to top menu"),
  uMenu::Line()
};

uMenu menu_ldmx_elinks(menu_ldmx_elinks_lines);

uMenu::Line menu_ldmx_roc_lines[] = {
  uMenu::Line("IROC","Change the active ROC number", &ldmx_roc ),
  uMenu::Line("CHAN","Dump link status", &ldmx_roc ),
  uMenu::Line("PAGE","Dump a page", &ldmx_roc ),
  uMenu::Line("POKE","Change a single value", &ldmx_roc ),
  uMenu::Line("LOAD","Load values from file", &ldmx_roc ),
  uMenu::Line("QUIT","Back to top menu"),
  uMenu::Line()
};

uMenu menu_ldmx_roc(ldmx_roc_render, menu_ldmx_roc_lines);

uMenu::Line menu_ldmx_bias_lines[] = {
				      //  uMenu::Line("STATUS","Read the bias line settings", &ldmx_bias ),
  uMenu::Line("INIT","Initialize a board", &ldmx_bias ),
  uMenu::Line("SET","Set a specific bias line setting", &ldmx_bias ),
  uMenu::Line("LOAD","Load bias values from file", &ldmx_bias ),
  uMenu::Line("QUIT","Back to top menu"),
  uMenu::Line()
};

uMenu menu_ldmx_bias(menu_ldmx_bias_lines);


uMenu::Line menu_ldmx_fc_lines[] = {
  uMenu::Line("STATUS","Check status and counters", &ldmx_fc ),
  uMenu::Line("SW_L1A","Send a software L1A", &ldmx_fc ),
  uMenu::Line("LINK_RESET","Send a link reset", &ldmx_fc ),
  uMenu::Line("BUFFER_CLEAR","Send a buffer clear", &ldmx_fc ),
  uMenu::Line("COUNTER_RESET","Reset counters", &ldmx_fc ),
  uMenu::Line("FC_RESET","Reset the fast control", &ldmx_fc ),
  uMenu::Line("MULTISAMPLE","Setup multisample readout", &ldmx_fc ),
  uMenu::Line("QUIT","Back to top menu"),
  uMenu::Line()
};

uMenu menu_ldmx_fc(menu_ldmx_fc_lines);

uMenu::Line menu_ldmx_daq_debug_lines[] = {
  uMenu::Line("STATUS","Provide the status", &ldmx_daq_debug ),
  uMenu::Line("FULL_DEBUG", "Toggle debug mode for full-event buffer",  &ldmx_daq_debug ),
  uMenu::Line("DISABLE_ROCLINKS", "Disable ROC links to drive only from SW",  &ldmx_daq_debug ),
  uMenu::Line("ROC_LOAD", "Load a practice ROC events from a file",  &ldmx_daq_debug ),
  uMenu::Line("ROC_SEND", "Generate a SW L1A to send the ROC buffers to the builder",  &ldmx_daq_debug ),
  uMenu::Line("FULL_LOAD", "Load a practice full event from a file",  &ldmx_daq_debug ),
  uMenu::Line("FULL_SEND", "Send the buffer to the off-detector electronics",  &ldmx_daq_debug ),
  uMenu::Line("SPY", "Spy on the front-end buffer",  &ldmx_daq_debug ),
  uMenu::Line("QUIT","Back to top menu"),
  uMenu::Line()
};

uMenu menu_ldmx_daq_debug(menu_ldmx_daq_debug_lines);

uMenu::Line menu_ldmx_daq_lines[] = {
  uMenu::Line("DEBUG", "Debugging menu",  &menu_ldmx_daq_debug ),
  uMenu::Line("STATUS", "Status of the DAQ", &ldmx_daq),
  uMenu::Line("ENABLE", "Toggle enable status", &ldmx_daq),
  uMenu::Line("ZS", "Toggle ZS status", &ldmx_daq),
  uMenu::Line("L1APARAMS", "Setup parameters for L1A capture", &ldmx_daq),
  uMenu::Line("READ", "Read an event", &ldmx_daq),
  uMenu::Line("RESET", "Reset the DAQ", &ldmx_daq),
  uMenu::Line("HARD_RESET", "Reset the DAQ, including all parameters", &ldmx_daq),
  uMenu::Line("STANDARD","Do the standard setup for HCAL", &ldmx_daq),
  uMenu::Line("PEDESTAL","Take a simple random pedestal run", &ldmx_daq),
  uMenu::Line("QUIT","Back to top menu"),
  uMenu::Line()
};

uMenu menu_ldmx_daq(menu_ldmx_daq_lines);

     uMenu::Line menu_expert_lines[] = { 
					uMenu::Line("OLINK","Optical link functions", &menu_ldmx_link),
					uMenu::Line("WB","Raw wishbone interactions", &menu_wishbone ),
					uMenu::Line("I2C","Access the I2C Core", &menu_ldmx_i2c ),
					uMenu::Line("QUIT","Back to top menu"),
					uMenu::Line()
     };

     uMenu menu_expert(menu_expert_lines);

     uMenu::Line menu_utop_lines[] = { 
       uMenu::Line("STATUS","Status summary", &ldmx_status),
       uMenu::Line("FAST_CONTROL","Fast Control", &menu_ldmx_fc ),
       uMenu::Line("ROC","ROC Configuration", &menu_ldmx_roc ),
       uMenu::Line("BIAS","BIAS voltage setting", &menu_ldmx_bias ),
       uMenu::Line("ELINKS","Manage the elinks", &menu_ldmx_elinks ),
       uMenu::Line("DAQ","DAQ", &menu_ldmx_daq ),
       uMenu::Line("EXPERT","Expert functions", &menu_expert ),
       uMenu::Line() 
     };
     uMenu menu_utop(menu_utop_lines);

     menu_utop.addLine(uMenu::Line("EXIT", "Exit this tool" ));

     menu_utop.steer( pft_ ) ;

}



void wb_action( const std::string& cmd, PolarfireTarget* pft ) {
  static uint32_t target=0;
  static uint32_t addr=0;
  if (cmd=="RESET") {
    pft->wb->wb_reset();
  }
  if (cmd=="WRITE") {
    target=tool_readline_int("Target",target);
    addr=tool_readline_int("Address",addr);
    uint32_t val=tool_readline_int("Value",0);
    pft->wb->wb_write(target,addr,val);
  }
  if (cmd=="READ") {
    target=tool_readline_int("Target",target);
    addr=tool_readline_int("Address",addr);
    uint32_t val=pft->wb->wb_read(target,addr);
    printf(" Read: 0x%08x\n",val);
  }
  if (cmd=="BLOCKREAD") {
    target=tool_readline_int("Target",target);
    addr=tool_readline_int("Starting address",addr);
    int len=tool_readline_int("Number of words",8);
    for (int i=0; i<len; i++) {
      uint32_t val=pft->wb->wb_read(target,addr+i);
      printf(" %2d/0x%04x : 0x%08x\n",target,addr+i,val);
    }
  }
  if (cmd=="STATUS") {
    uint32_t crcd, crcu, wbe;
    pft->wb->wb_errors(crcu,crcd,wbe);
    printf("CRC errors: Up -- %d   Down --%d \n",crcu,crcd);
    printf("Wishbone errors: %d\n",wbe);
  }
}

void ldmx_status( PolarfireTarget* pft) {
  uint32_t pf_firmware=pft->hcal->getFirmwareVersion();
  printf(" Polarfire firmware : %4x.%02x\n",pf_firmware>>8,pf_firmware&0xFF);
}

void ldmx_i2c( const std::string& cmd, PolarfireTarget* pft ) {
  static uint32_t addr=0;
  static int waddrlen=1;
  static int i2caddr=0;
  pflib::I2C i2c(pft->wb);

  if (cmd=="BUS") {
    int ibus=i2c.get_active_bus();
    ibus=tool_readline_int("Bus to make active",ibus);
    i2c.set_active_bus(ibus);
  }
  if (cmd=="WRITE") {
    i2caddr=tool_readline_int("I2C Target ",i2caddr);
    uint32_t val=tool_readline_int("Value ",0);
    i2c.set_bus_speed(1000);
    i2c.write_byte(i2caddr,val);
  }
  if (cmd=="READ") {
    i2caddr=tool_readline_int("I2C Target",i2caddr);
    i2c.set_bus_speed(100);
//    ldmx->i2c().set_bus_speed(1000);
    uint8_t val=i2c.read_byte(i2caddr);
    printf("%02x : %02x\n",i2caddr,val);
  }
  if (cmd=="MULTIREAD") {
    i2caddr=tool_readline_int("I2C Target",i2caddr);
    waddrlen=tool_readline_int("Read address length",waddrlen);
    std::vector<uint8_t> waddr;
    if (waddrlen>0) {
      addr=tool_readline_int("Read address",addr);
      for (int i=0; i<waddrlen; i++) {
        waddr.push_back(uint8_t(addr&0xFF));
        addr=(addr>>8);
      }
    }
    int len=tool_readline_int("Read length",1);
    std::vector<uint8_t> data=i2c.general_write_read(i2caddr,waddr);
    for (size_t i=0; i<data.size(); i++)
      printf("%02x : %02x\n",int(i),data[i]);
  }
  if (cmd=="MULTIWRITE") {
    i2caddr=tool_readline_int("I2C Target",i2caddr);
    waddrlen=tool_readline_int("Write address length",waddrlen);
    std::vector<uint8_t> wdata;
    if (waddrlen>0) {
      addr=tool_readline_int("Write address",addr);
      for (int i=0; i<waddrlen; i++) {
        wdata.push_back(uint8_t(addr&0xFF));
        addr=(addr>>8);
      }
    }
    int len=tool_readline_int("Write data length",1);
    for (int j=0; j<len; j++) {
      char prompt[64];
      sprintf(prompt,"Byte %d: ",j);
      int id=tool_readline_int(prompt,0);
      wdata.push_back(uint8_t(id));
    }
    i2c.general_write_read(i2caddr,wdata);
  }
}

void ldmx_link( const std::string& cmd, PolarfireTarget* pft ) {
  if (cmd=="STATUS") {
    uint32_t status;
    uint32_t ratetx=0, raterx=0, ratek=0;
    //    uint32_t status=ldmx->link_status(&ratetx,&raterx,&ratek);
    printf("Link Status: %08x   TX Rate: %d   RX Rate: %d   K Rate : %d\n",status,ratetx,raterx,ratek);
  }
  if (cmd=="CONFIG") {
    bool reset_txpll=tool_readline_bool("TX-PLL Reset?", false);
    bool reset_gtxtx=tool_readline_bool("GTX-TX Reset?", false);
    bool reset_tx=tool_readline_bool("TX Reset?", false);
    bool reset_rx=tool_readline_bool("RX Reset?", false);
    static bool polarity=false;
    polarity=tool_readline_bool("Choose negative TX polarity?",polarity);
    static bool polarity2=false;
    polarity2=tool_readline_bool("Choose negative RX polarity?",polarity2);
    static bool bypass=false;
    bypass=tool_readline_bool("Bypass CRC on receiver?",false);
    //    ldmx->link_setup(polarity,polarity2,bypass);
    // ldmx->link_resets(reset_txpll,reset_gtxtx,reset_tx,reset_rx);
  }
  if (cmd=="SPY") {
    /*
    std::vector<uint32_t> spy=ldmx->link_spy();
    for (size_t i=0; i<spy.size(); i++)
      printf("%02d %05x\n",int(i),spy[i]);
    */
  }
}


void ldmx_elinks( const std::string& cmd, PolarfireTarget* pft ) {
  
  pflib::Elinks& elinks=pft->hcal->elinks();
  static int ilink=0;
  if (cmd=="SPY") {
    ilink=tool_readline_int("Which elink? ",ilink);
    std::vector<uint8_t> spy=elinks.spy(ilink);
    for (size_t i=0; i<spy.size(); i++)
      printf("%02d %05x\n",int(i),spy[i]);
  }
  if (cmd=="BITSLIP") {
    ilink=tool_readline_int("Which elink? ",ilink);
    
    int bitslip=tool_readline_int("Bitslip value (-1 for auto): ",elinks.getBitslip(ilink));
    for (int jlink=0; jlink<8; jlink++) {
      if (ilink>=0 && jlink!=ilink) continue;
      if (bitslip<0) elinks.setBitslipAuto(jlink,true);
      else {
	elinks.setBitslipAuto(jlink,false);
	elinks.setBitslip(jlink,bitslip);
      }
    }
  }
  if (cmd=="BIGSPY") {
    int mode=tool_readline_int("Mode? (0=immediate, 1=L1A) ",0);
    ilink=tool_readline_int("Which elink? ",ilink);
    int presamples=tool_readline_int("Presamples? ",20);
    elinks.setupBigspy(mode,ilink,presamples);
    if (mode==1) {
      ldmx_fc("SW_L1A",pft);
    }
    std::vector<uint32_t> words=elinks.bigspy();
    for (int i=0; i<presamples+100; i++) {
      printf("%03d %08x\n",i,words[i]);
    }
  }
  if (cmd=="DELAY") {
    ilink=tool_readline_int("Which elink? ",ilink);
    int idelay=tool_readline_int("Delay value: ",128);
    elinks.setDelay(ilink,idelay);
  }
  if (cmd=="HARD_RESET") {
    elinks.resetHard();
  }
  if (cmd=="SCAN") {
    ilink=tool_readline_int("Which elink? ",ilink);
    elinks.scanAlign(ilink);
  }
  if (cmd=="STATUS") {
    // need to get link count someday
    int nlinks=8;
    std::vector<uint32_t> status;
    for (int i=0; i<nlinks; i++) 
      status.push_back(elinks.getStatusRaw(i));

    printf("%20s","");
    for (int i=0; i<nlinks; i++)
      printf("%4d ",i);
    printf("\n");
    printf("%20s","DLY_RANGE");
    for (int i=0; i<nlinks; i++)
      printf(" %3d ",status[i]&0x1);
    printf("\n");
    printf("%20s","EYE_EARLY");
    for (int i=0; i<nlinks; i++)
      printf(" %3d ",(status[i]>>1)&0x1);
    printf("\n");
    printf("%20s","EYE_LATE");
    for (int i=0; i<nlinks; i++)
      printf(" %3d ",(status[i]>>2)&0x1);
    printf("\n");
    printf("%20s","IS_IDLE");
    for (int i=0; i<nlinks; i++)
      printf(" %3d ",(status[i]>>3)&0x1);
    printf("\n");
    printf("%20s","IS_ALIGNED");
    for (int i=0; i<nlinks; i++)
      printf(" %3d ",(status[i]>>4)&0x1);
    printf("\n");
    printf("%20s","AUTO_LOCKED");
    for (int i=0; i<nlinks; i++) {
      if (!elinks.isBitslipAuto(i)) printf("  X  ");
      else printf(" %3d ",(status[i]>>5)&0x1);
    }
    printf("\n");
    printf("%20s","AUTO_PHASE");
    for (int i=0; i<nlinks; i++)
      printf(" %3d ",(status[i]>>8)&0x7);
    printf("\n");
    printf("%20s","BAD_COUNT");
    for (int i=0; i<nlinks; i++)
      printf("%4d ",(status[i]>>16)&0xFFF);
    printf("\n");
    printf("%20s","AUTO_COUNT");
    for (int i=0; i<nlinks; i++)
      printf(" %3d ",(status[i]>>28)&0xF);
    printf("\n");
  }
    
}


static int iroc=0;
void ldmx_roc_render(PolarfireTarget*) {
  printf(" Active ROC: %d\n",iroc);
}

void ldmx_roc( const std::string& cmd, PolarfireTarget* pft ) {

  if (cmd=="IROC") {
    iroc=tool_readline_int("Which ROC to manage: ",iroc);
  }
  pflib::ROC roc=pft->hcal->roc(iroc);
  if (cmd=="CHAN") {
    int chan=tool_readline_int("Which channel? ",0);
    std::vector<uint8_t> v=roc.getChannelParameters(chan);
    for (int i=0; i<int(v.size()); i++)
      printf("%02d : %02x\n",i,v[i]);
  }
  if (cmd=="PAGE") {
    int page=tool_readline_int("Which page? ",0);
    int len=tool_readline_int("Length?", 8);
    std::vector<uint8_t> v=roc.readPage(page,len);
    for (int i=0; i<int(v.size()); i++)
      printf("%02d : %02x\n",i,v[i]);
  }
  if (cmd=="POKE") {
    int page=tool_readline_int("Which page? ",0);
    int entry=tool_readline_int("Offset: ",0);
    int value=tool_readline_int("New value: ",0);

    roc.setValue(page,entry,value);
  }
  if (cmd=="LOAD") {
    printf("\n --- This command expects a CSV file with columns [page,offset,value].\n");
    printf(" --- Line starting with # are ignored.\n");
    std::string fname=tool_readline("Filename: ");
    if (!fname.empty()) {
      std::ifstream f{fname};
      if (!f.is_open()) {
	      printf("\n\n  ERROR: Unable to open '%s'\n\n",fname.c_str());
	      return;
      }
      while (f) {
        auto cells = getNextLineAndExtractValues(f);
        if (cells.empty()) {
          // empty line or comment
          continue;
        }
        if (cells.size() == 3) {
          roc.setValue(cells.at(0), cells.at(1), cells.at(2));
        } else {
          printf("WARNING: Ignoring line without exactly three columns.\n");
        }
      }
    }
  }
}

void ldmx_fc( const std::string& cmd, PolarfireTarget* pft ) {
  bool do_status=false;

  pflib::FastControl& fc=pft->hcal->fc();
  if (cmd=="SW_L1A") {
    pft->backend->fc_sendL1A();
    printf("Sent SW L1A\n");
  }
  if (cmd=="LINK_RESET") {
    pft->backend->fc_linkreset();
    printf("Sent LINK RESET\n");
  }
  if (cmd=="BUFFER_CLEAR") {
    pft->backend->fc_bufferclear();
    printf("Sent BUFFER CLEAR\n");
  }
  if (cmd=="COUNTER_RESET") {
    pft->hcal->fc().resetCounters();
    do_status=true;
  }
  if (cmd=="FC_RESET") {
    pft->hcal->fc().resetTransmitter();
  }
  if (cmd=="MULTISAMPLE") {
    bool multi;
    int nextra;
    pft->hcal->fc().getMultisampleSetup(multi,nextra);
    multi=tool_readline_bool("Enable multisample readout? ",multi);
    if (multi)
      nextra=tool_readline_int("Extra samples (total is +1 from this number) : ",nextra);
    pft->hcal->fc().setupMultisample(multi,nextra);
  }
  if (cmd=="STATUS" || do_status) {
    bool multi;
    int nextra;
    pft->hcal->fc().getMultisampleSetup(multi,nextra);
    if (multi) printf(" Multisample readout enabled : %d extra L1a (%d total samples)\n",nextra,nextra+1);
    else printf(" Multisaple readout disabled\n");
    printf(" Snapshot: %03x\n",pft->wb->wb_read(1,3));
    uint32_t sbe,dbe;
    pft->hcal->fc().getErrorCounters(sbe,dbe);
    printf("  Single bit errors: %d     Double bit errors: %d\n",sbe,dbe);
    std::vector<uint32_t> cnt=pft->hcal->fc().getCmdCounters();
    for (int i=0; i<8; i++) 
      printf("  Bit %d count: %u\n",i,cnt[i]); 
      
  }

}

  static const int tgt_ctl    = 8;
  static const int tgt_rocbuf = 9;
  static const int tgt_fmt    = 10;
  static const int tgt_buffer = 11;
static int nlinks=8; // need to read this eventually



void ldmx_daq( const std::string& cmd, PolarfireTarget* pft ) {

  pflib::DAQ& daq=pft->hcal->daq();
  if (cmd=="STATUS") {
    bool full, empty;
    int events, asize;
    uint32_t reg;

    printf("-----Front-end FIFO-----\n");
    reg=pft->wb->wb_read(tgt_ctl,4);
    printf(" Header occupancy : %d  Maximum occupancy : %d \n",(reg>>8)&0x3F,(reg>>0)&0x3F);
    reg=pft->wb->wb_read(tgt_ctl,5);
    printf(" Next event info: 0x%08x\n",reg);

    printf("-----Per-ROCLINK processing-----\n");
    printf(" Link  ID  EN ZS FL EM\n");
    for (int ilink=0; ilink<nlinks; ilink++) {
      uint32_t reg1=pft->wb->wb_read(tgt_fmt,(ilink<<7)|1);
      uint32_t reg2=pft->wb->wb_read(tgt_fmt,(ilink<<7)|2);
      printf("  %2d  %04x %2d %2d %2d %2d      %08x\n",ilink,reg2,(reg1>>0)&1,(reg1>>1)&0x1,(reg1>>22)&1,(reg1>>23)&1,reg1);
    }

    printf("-----Off-detector FIFO-----\n");
    pft->backend->daq_status(full, empty, events,asize);
   
    printf(" %8s %9s  Events ready : %3d  Next event size : %d\n",
	   (full)?("FULL"):("NOT-FULL"),(empty)?("EMPTY"):("NOT-EMPTY"),events,asize);
  }
  if (cmd=="RESET") {
    printf("..Halting the event builder\n");
    for (int ilink=0; ilink<nlinks; ilink++) {
      uint32_t reg1=pft->wb->wb_read(tgt_fmt,(ilink<<7)|1);
      pft->wb->wb_write(tgt_fmt,(ilink<<7)|1,(reg1&0xfffffffeu)|0x2);
    }
    printf("..Sending a buffer clear\n");
    ldmx_fc("BUFFER_CLEAR",pft);
    printf("..Sending DAQ reset\n");
    pft->backend->daq_reset();
  }
  if (cmd=="HARD_RESET") {
    printf("..HARD reset\n");
    daq.reset();
    pft->backend->daq_reset();
  }
  if (cmd=="ENABLE") {
    daq.enable(!daq.enabled());
  }
  if (cmd=="ZS") {
    int jlink=tool_readline_int("Which link (-1 for all)? ",-1);    

    for (int ilink=0; ilink<nlinks; ilink++) {
      if (ilink!=jlink && jlink>=0) continue;
      uint32_t reg1=pft->wb->wb_read(tgt_fmt,(ilink<<7)|1);
      bool wasZS=(reg1&0x2)!=0;
      if (jlink>=0 && !wasZS) {
	bool fullSuppress=tool_readline_bool("Suppress all channels? ",false);
	reg1=reg1|0x4;
	if (!fullSuppress) reg1=reg1^0x4;
      }
      pft->wb->wb_write(tgt_fmt,(ilink<<7)|1,reg1^0x2);
    }
  }
  if (cmd=="L1APARAMS") {
    int ilink=tool_readline_int("Which link? ",-1);    
    if (ilink>=0) {
      uint32_t reg1=pft->wb->wb_read(tgt_rocbuf,(ilink<<7)|1);
      int delay=tool_readline_int("L1A delay? ",(reg1>>8)&0xFF);
      int capture=tool_readline_int("L1A capture length? ",(reg1>>16)&0xFF);
      reg1=(reg1&0xFF)|((delay&0xFF)<<8)|((capture&0xFF)<<16);
      pft->wb->wb_write(tgt_rocbuf,(ilink<<7)|1,reg1);
    }
  }
    
  if (cmd=="READ") {
    bool full, empty;
    int events, esize;
    pft->backend->daq_status(full, empty, events, esize);
    std::vector<uint32_t> buffer;
    buffer=pft->backend->daq_read_event();
    if (!empty) pft->backend->daq_advance_ptr();

    for (size_t i=0; i<buffer.size() && i<32; i++) {
      printf("%04d %08x\n",int(i),buffer[i]);
    }
    bool save_to_disk=tool_readline_bool("Save to disk?  ",false);
    if (save_to_disk) {
      std::string fname=tool_readline("Filename :  ");
      FILE* f=fopen(fname.c_str(),"w");
      fwrite(&(buffer[0]),sizeof(uint32_t),buffer.size(),f);
      fclose(f);
    }
  }
  if (cmd=="STANDARD") {
    int fpgaid=tool_readline_int("FPGA id: ",daq.getFPGAid());
    daq.setIds(fpgaid);
    for (int i=0; i<daq.nlinks(); i++) {
      if (i<2) daq.setupLink(i,false,false,14,40);
      else daq.setupLink(i,true,true,14,40);
    }
  }
  if (cmd=="PEDESTAL") {
    bool enable;
    int extra_samples;
    pflib::FastControl& fc=pft->hcal->fc();
    fc.getMultisampleSetup(enable,extra_samples);

    std::string fname_def_format = "pedestal_%Y%m%d_%H%M%S.raw";

    time_t t = time(NULL);
    struct tm *tm = localtime(&t);

    char fname_def[64];
    strftime(fname_def, sizeof(fname_def), fname_def_format.c_str(), tm); 
    
    int nevents=tool_readline_int("How many events? ", 100);
    std::string fname=tool_readline("Filename :  ", fname_def);
    FILE* f=fopen(fname.c_str(),"w");

    for (int ievt=0; ievt<nevents; ievt++) {
      pft->backend->fc_sendL1A();
           
      bool full, empty;
      int events, esize;
      
      do {
	pft->backend->daq_status(full, empty, events, esize);
      } while (empty);
      
      
      std::vector<uint32_t> superbuffer;
      std::vector<int> lens;
      int fpgaid=0;
      int totlen=0;
      
      for (int i=0; i<(extra_samples+1); i++) {
        std::vector<uint32_t> buffer;
        
        buffer=pft->backend->daq_read_event();
        if (!empty) pft->backend->daq_advance_ptr();

        lens.push_back(int(buffer.size()));
        // add to the superbuffer
        superbuffer.insert(superbuffer.end(),buffer.begin(),buffer.end());
        if (i==0) fpgaid=(buffer[0]>>(12+2+6))&0xFF;        
        totlen+=buffer.size();
      }

      std::vector<uint32_t> header;
      header.push_back(0x11111111u);
      header.push_back(0xBEEF2021u);
      uint32_t val=(0x1<<28); // version
      val|=(fpgaid&0xFF)<<20;
      val|=(lens.size())<<16;
      totlen+=1; //header
      totlen+=(lens.size()+1)/2;
      val|=totlen;
      header.push_back(val);
      val=0;
      // now the various subpackets
      for (int i=0; i<int(lens.size()); i++) {
        val|=(lens[i])<<(16*(i%2));
        if ((i%2)||(i+1==int(lens.size()))) {
          header.push_back(val);
          val=0;
        }
      }
      fwrite(&(header[0]),sizeof(uint32_t),header.size(),f);

      superbuffer.push_back(0xd07e2021u);
      superbuffer.push_back(0x12345678u);
      fwrite(&(superbuffer[0]),sizeof(uint32_t),superbuffer.size(),f);      
    }
    fclose(f);
  }
}

std::vector<uint32_t> read_words_from_file() {
  std::vector<uint32_t> data;
  /// load from file
  std::string fname=tool_readline("Read from text file (line by line hex 32-bit words)");
  char buffer[512];
  FILE* f=fopen(fname.c_str(),"r");
  if (f==0) {
    printf("Unable to open '%s'\n",fname.c_str());
    return data;
  }
  while (!feof(f)) {
    buffer[0]=0;
    fgets(buffer,511,f);
    if (strlen(buffer)<8 || strchr(buffer,'#')!=0) continue;
    uint32_t val;
    val=strtoul(buffer,0,16);
    data.push_back(val);
    printf("%08x\n",val);
  }
  fclose(f);
  return data;
}

void ldmx_daq_debug( const std::string& cmd, PolarfireTarget* pft ) {
  if (cmd=="STATUS") {
    // get the general status
    ldmx_daq("STATUS", pft); 
    uint32_t reg1,reg2;
    printf("-----Per-ROC Controls-----\n");
    reg1=pft->wb->wb_read(tgt_ctl,1);
    printf(" Disable ROC links: %s\n",(reg1&0x80000000u)?("TRUE"):("FALSE"));

    printf(" Link  F E RP WP \n");
    for (int ilink=0; ilink<nlinks; ilink++) {
      uint32_t reg=pft->wb->wb_read(tgt_rocbuf,(ilink<<7)|3);
      printf("   %2d  %d %d %2d %2d       %08x\n",ilink,(reg>>26)&1,(reg>>27)&1,(reg>>16)&0xf,(reg>>12)&0xf,reg);
    }
    printf("-----Event builder    -----\n");
    reg1=pft->wb->wb_read(tgt_ctl,6);
    printf(" EVB Debug word: %08x\n",reg1);
    reg1=pft->wb->wb_read(tgt_buffer,1);
    reg2=pft->wb->wb_read(tgt_buffer,4);
    printf(" Event buffer Debug word: %08x %08x\n",reg1,reg2);

    printf("-----Full event buffer-----\n");
    reg1=pft->wb->wb_read(tgt_buffer,1);
    reg2=pft->wb->wb_read(tgt_buffer,2);    printf(" Read Page: %d  Write Page : %d   Full: %d  Empty: %d   Evt Length on current page: %d\n",(reg1>>13)&0x1,(reg1>>12)&0x1,(reg1>>15)&0x1,(reg1>>14)&0x1,(reg1>>0)&0xFFF);
    printf(" Spy page : %d  Spy-as-source : %d  Length-of-spy-injected-event : %d\n",reg2&0x1,(reg2>>1)&0x1,(reg2>>16)&0xFFF);
  } 
  if (cmd=="FULL_DEBUG") {
    uint32_t reg2=pft->wb->wb_read(tgt_buffer,2);
    reg2=reg2^0x2;// invert
    pft->wb->wb_write(tgt_buffer,2,reg2);
  }
  if (cmd=="DISABLE_ROCLINKS") {
    uint32_t reg=pft->wb->wb_read(tgt_ctl,1);
    reg=reg^0x80000000u;// invert
    pft->wb->wb_write(tgt_ctl,1,reg);
  }
  if (cmd=="FULL_SEND") {
    uint32_t reg2=pft->wb->wb_read(tgt_buffer,2);
    reg2=reg2|0x1000;// set the send bit
    pft->wb->wb_write(tgt_buffer,2,reg2);
  }
  if (cmd=="ROC_SEND") {
    uint32_t reg2=pft->wb->wb_read(tgt_ctl,1);
    reg2=reg2|0x40000000;// set the send bit
    pft->wb->wb_write(tgt_ctl,1,reg2);
  }
  if (cmd=="FULL_LOAD") {
    std::vector<uint32_t> data=read_words_from_file();

    // set the spy page to match the read page
    uint32_t reg1=pft->wb->wb_read(tgt_buffer,1);
    uint32_t reg2=pft->wb->wb_read(tgt_buffer,2);
    int rpage=(reg1>>12)&0x1;
    if (rpage) reg2=reg2|0x1;
    else reg2=reg2&0xFFFFFFFEu;
    pft->wb->wb_write(tgt_buffer,2,reg2);

    for (size_t i=0; i<data.size(); i++) 
      pft->wb->wb_write(tgt_buffer,0x1000+i,data[i]);
          
    /// set the length
    reg2=pft->wb->wb_read(tgt_buffer,2);
    reg2=reg2&0xFFFF;// remove the upper bits
    reg2=reg2|(data.size()<<16);
    pft->wb->wb_write(tgt_buffer,2,reg2);
  }
  if (cmd=="SPY") {
    // set the spy page to match the read page
    uint32_t reg1=pft->wb->wb_read(tgt_buffer,1);
    uint32_t reg2=pft->wb->wb_read(tgt_buffer,2);
    int rpage=(reg1>>12)&0x1;
    if (rpage) reg2=reg2|0x1;
    else reg2=reg2&0xFFFFFFFEu;
    pft->wb->wb_write(tgt_buffer,2,reg2);

    for (size_t i=0; i<32; i++) {
      uint32_t val=pft->wb->wb_read(tgt_buffer,0x1000|i);
      printf("%04d %08x\n",int(i),val);
    }
  }
    
  if (cmd=="ROC_LOAD") {
    std::vector<uint32_t> data=read_words_from_file();
    if (int(data.size())!=nlinks*40) {
      printf("Expected %d words, got only %d\n",nlinks*40,int(data.size()));
      return;
    }
    for (int ilink=0; ilink<nlinks; ilink++) {
      uint32_t reg;
      // set the wishbone page to match the read page, and set the length
      reg=pft->wb->wb_read(tgt_rocbuf,(ilink<<7)+3);
      int rpage=(reg>>16)&0xF;
      reg=(reg&0xFFFFF000u)|rpage|(40<<4);
      pft->wb->wb_write(tgt_rocbuf,(ilink<<7)+3,reg);
      // load the bytes
      for (int i=0; i<40; i++)
	pft->wb->wb_write(tgt_rocbuf,(ilink<<7)|0x40|i,data[40*ilink+i]);      
    }
  }
}

void ldmx_bias( const std::string& cmd, PolarfireTarget* pft ) {

  static int iboard=0;
  if (cmd=="STATUS") {
    iboard=tool_readline_int("Which board? ",iboard);
    pflib::Bias bias=pft->hcal->bias(iboard);
  }
  
  if (cmd=="INIT") {
    iboard=tool_readline_int("Which board? ",iboard);
    pflib::Bias bias=pft->hcal->bias(iboard);
    bias.initialize();
  }
  if (cmd=="SET") {
    iboard=tool_readline_int("Which board? ",iboard);    
    static int led_sipm=0;
    led_sipm=tool_readline_int(" SiPM(0) or LED(1)? ",led_sipm);
    int ichan=tool_readline_int(" Which HDMI connector? ",-1);
    int dac=tool_readline_int(" What DAC value? ",0);
    if (ichan>=0) {
      pflib::Bias bias=pft->hcal->bias(iboard);
      if (!led_sipm) bias.setSiPM(ichan,dac);
      else bias.setLED(ichan,dac);
    }
  }
  if (cmd=="LOAD") {
    printf("\n --- This command expects a CSV file with four columns [0=SiPM/1=LED,board,hdmi#,value].\n");
    printf(" --- Line starting with # are ignored.\n");
    std::string fname=tool_readline("Filename: ");
    if (!fname.empty()) {
      FILE* f=fopen(fname.c_str(),"r");
      if (f==0) {
	printf("\n\n  ERROR: Unable to open '%s'\n\n",fname.c_str());
	return;
      }
      char buffer[1025];
      while (!feof(f)) {
	buffer[0]=0;
	fgets(buffer,1024,f);
	if (strchr(buffer,'#')!=0) *(strchr(buffer,'#'))=0;
	int vals[4];
	int itok=0;
	for (itok=0; itok<4; itok++) {
	  char* tok=strtok((itok==0)?(buffer):(0),", \t");
	  if (tok==0) break;
	  vals[itok]=strtol(tok,0,0);
	}
	if (itok==4) {	  
	  pflib::Bias bias=pft->hcal->bias(vals[0]);
	  if (!vals[1]) bias.setSiPM(vals[2],vals[3]);
	  else bias.setLED(vals[2],vals[3]);
	}
      }
      fclose(f);
    }
  }
}

