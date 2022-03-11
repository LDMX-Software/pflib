#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>
#include <map>
#include <iomanip>
#include <stdlib.h>
#include <string>
#include <exception>
#include "pflib/PolarfireTarget.h"
#include "pflib/Compile.h" // for parameter listing
#ifdef PFTOOL_ROGUE
#include "pflib/rogue/RogueWishboneInterface.h"
#endif
#ifdef PFTOOL_UHAL
#include "pflib/uhal/uhalWishboneInterface.h"
#endif
#include "Menu.h"
#include "Rcfile.h"

using pflib::PolarfireTarget;

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
static void ldmx_daq_setup( const std::string& cmd, PolarfireTarget* pft );

Rcfile options;
bool file_exists(const std::string& fname) {
  FILE* f=fopen(fname.c_str(),"r");
  if (f==0) return false;
  fclose(f);
  return true;
}

void prepareOpts(Rcfile& rcfile) {
  rcfile.declareVBool("roclinks",
      "Vector Bool[8] indicating which roc links are active");
  rcfile.declareString("ipbus_map_path", 
      "Full path to directory containgin IP-bus mapping. Only required for uHal comm.");
  rcfile.declareString("default_hostname",
      "Hostname of polarfire to connect to if none are given on the command line");
}

int main(int argc, char* argv[]) {
  prepareOpts(options);
  
  // print help
  if (argc == 2 and (!strcmp(argv[1],"-h") or !strcmp(argv[1],"--help"))) {
#ifdef PFTOOL_ROGUE
#ifdef PFTOOL_UHAL
    printf("Usage: pftool [hostname] [-u] [-r] [-s script]\n");
    printf("  Supporting both UHAL and ROGUE\n");
    printf("   -u uHAL connection\n");
    printf("   -r rogue connection\n");
#else
    printf("Usage: pftool [hostname] [-s script]\n");
    printf("  Supporting only ROGUE\n");
#endif
#else
#ifdef PFTOOL_UHAL
    printf("Usage: pftool [hostname] [-s script]\n");
    printf("  Supporting only UHAL\n");
#else
    printf("Usage: pftool [hostname] [-s script]\n");
    printf("  Supporting NO TRANSPORTS\n");
#endif
#endif
    printf("Reading RC files from ${PFTOOLRC}, ${CWD}/pftoolrc, ${HOME}/.pftoolrc with priority in this order\n");
    options.help();
    
    printf("\n");
    return 1;
  }

  /*****************************************************************************
   * Load RC File
   ****************************************************************************/
  std::string home=getenv("HOME");
  if (getenv("PFTOOLRC")) {
    if (file_exists(getenv("PFTOOLRC"))) {
      options.load(getenv("PFTOOLRC"));
    } else {
      std::cerr << "WARNING: PFTOOLRC=" << getenv("PFTOOLRC") 
        << " is not loaded because it doesn't exist." << std::endl;
    }
  }
  if (file_exists("pftoolrc"))
    options.load("pftoolrc");
  if (file_exists(home+"/.pftoolrc"))
    options.load(home+"/.pftoolrc");

  /*****************************************************************************
   * Parse Command Line Parameters
   ****************************************************************************/
  bool isuhal=false;
  bool isrogue=false;
  // rogue is default if it is available
#ifdef PFTOOL_ROGUE
  isrogue=true; // default
#else
  isuhal=true; // default
#endif
  std::vector<std::string> hostnames;
  for (int i = 1 ; i < argc ; i++) {
    std::string arg(argv[i]);
#ifdef PFTOOL_UHAL
#ifdef PFTOOL_ROGUE
    if (arg=="-u") {
      isuhal  = true;
      isrogue = false;
    } 
    else if (arg=="-r") {
      isrogue = true;
      isuhal  = false;
    } 
#endif
#endif
    else if (arg=="-s") {
      if (i+1 == argc or argv[i+1][0] == '-') {
        std::cerr << "Argument " << arg << " requires a file after it." << std::endl;
        return 2;
      }
      i++;
      std::fstream sFile(argv[i]);
      if (!sFile.is_open()) {
        std::cerr << "Unable to open script file " << argv[i] << std::endl;
        return 2;
      }
      
      std::string line;
      while (getline(sFile, line)) {
        // erase whitespace at beginning of line
        while (!line.empty() && isspace(line[0])) line.erase(line.begin());
        // skip empty lines or ones whose first character is #
        if ( !line.empty() && line[0] == '#' ) continue ;
        // add to command queue
        BaseMenu::add_to_command_queue(line);
      }
      sFile.close() ;
    } else {
      // positional argument -> hostname
      hostnames.push_back( arg ) ;
    }
  }

  if (hostnames.size() == 0) {
    std::string hn = options.contents().getString("default_hostname");
    if (hn.empty()) {
      std::cerr << "No hostnames to connect to provided on the command line or in RC file" << std::endl;
      return 3;
    } else {
      hostnames.push_back(hn);
    }
  }

  /*****************************************************************************
   * Run tool
   ****************************************************************************/
  try {
    int i_host{-1};
    bool continue_running = true; // used if multiple hosts
    do {
      if (hostnames.size() > 1) {
        while (true) {
          std::cout << "ID - Hostname" << std::endl;
          for (std::size_t k{0}; k < hostnames.size(); k++) {
            std::cout << std::setw(2) << k << " - " << hostnames.at(k) << std::endl;
          }
          i_host = BaseMenu::readline_int(" ID of Polarfire Hostname (-1 to exit) : ", i_host);
          if (i_host == -1 or (i_host >= 0 and i_host < hostnames.size())) {
            // valid choice, let's leave
            break;
          } else {
            std::cerr << "\n " << i_host << " is not a valid choice." << std::endl;
          }
        }
        // if user chooses to leave menu
        if (i_host == -1) break;
      } else {
        i_host = 0;
      }
      
      // initialize connect with Polarfire
      std::unique_ptr<PolarfireTarget> p_pft;
      try {
#ifdef PFTOOL_ROGUE
        if (isrogue) {
          // the PolarfireTarget wraps the passed pointers in STL smart pointers so the memory will be handled
          p_pft=std::make_unique<PolarfireTarget>(new pflib::rogue::RogueWishboneInterface(hostnames.at(i_host),5970));
        }
#endif
#ifdef PFTOOL_UHAL
        if (isuhal) {
          // the PolarfireTarget wraps the passed pointers in STL smart pointers so the memory will be handled
          p_pft=std::make_unique<PolarfireTarget>(new pflib::uhal::uhalWishboneInterface(hostnames.at(i_host),
                options.contents().getString("ipbus_map_path")));
        }
#endif
      } catch (const pflib::Exception& e) {
        std::cerr << "pflib Init Error [" << e.name() << "] : " << e.message() << std::endl;
        return 3;
      }

      if (p_pft) {
      	// prepare the links
      	if (options.contents().is_vector("roclinks")) {	  
      	  std::vector<bool> actives=options.contents().getVBool("roclinks");
      	  for (int ilink=0; 
              ilink<p_pft->hcal.elinks().nlinks() and ilink<int(actives.size()); 
              ilink++) p_pft->hcal.elinks().markActive(ilink,actives[ilink]);
      	}
        ldmx_status(p_pft.get());
        RunMenu(p_pft.get());
      } else {
        std::cerr << "No Polarfire Target available to connect with. Not sure how we got here." << std::endl;
        return 126;
      }

      if (hostnames.size() > 1)  {      
        // menu for that target has been exited, check if user wants to choose another host
        continue_running = BaseMenu::readline_bool(" Choose a new card/host to connect to ? ", true);
      } else {
        // no other hosts, leave
        continue_running = false;
      }
    } while(continue_running);
  } catch (const std::exception& e) {
    std::cerr << " Unrecognized Exception : " << e.what() << std::endl;
    return 127;
  }
  return 0;
}

void RunMenu( PolarfireTarget* pft_ ) {
  
  using pfMenu = Menu<PolarfireTarget>;

  pfMenu menu_wishbone({
    pfMenu::Line("RESET", "Enable/disable (toggle)",  &wb_action ),
    pfMenu::Line("READ", "Read from an address",  &wb_action ),
    pfMenu::Line("BLOCKREAD", "Read several words starting at an address",  &wb_action ),
    pfMenu::Line("WRITE", "Write to an address",  &wb_action ),
    pfMenu::Line("STATUS", "Wishbone errors counters",  &wb_action ),
    pfMenu::Line("QUIT","Back to top menu")
    });
  
  pfMenu menu_ldmx_i2c({
    pfMenu::Line("BUS","Pick the I2C bus to use", &ldmx_i2c ),
    pfMenu::Line("READ", "Read from an address",  &ldmx_i2c ),
    pfMenu::Line("WRITE", "Write to an address",  &ldmx_i2c ),
    pfMenu::Line("MULTIREAD", "Read from an address",  &ldmx_i2c ),
    pfMenu::Line("MULTIWRITE", "Write to an address",  &ldmx_i2c ),
    pfMenu::Line("QUIT","Back to top menu")
    });
  
  pfMenu menu_ldmx_link({
    pfMenu::Line("STATUS","Dump link status", &ldmx_link ),
    pfMenu::Line("CONFIG","Setup link", &ldmx_link ),
    pfMenu::Line("SPY", "Spy on the uplink",  &ldmx_link ),
    pfMenu::Line("QUIT","Back to top menu")
    });
  
  pfMenu menu_ldmx_elinks({
    pfMenu::Line("RELINK","Follow standard procedure to establish links", &ldmx_elinks),
    pfMenu::Line("HARD_RESET","Hard reset of the PLL", &ldmx_elinks),    
    pfMenu::Line("STATUS", "Elink status summary",  &ldmx_elinks ),
    pfMenu::Line("SPY", "Spy on an elink",  &ldmx_elinks ),
    pfMenu::Line("BITSLIP", "Set the bitslip for a link or turn on auto", &ldmx_elinks),
    pfMenu::Line("SCAN", "Scan on an elink",  &ldmx_elinks ),
    pfMenu::Line("DELAY", "Set the delay on an elink", &ldmx_elinks),
    pfMenu::Line("BIGSPY", "Take a spy of a specific channel at 32-bits", &ldmx_elinks),
    pfMenu::Line("QUIT","Back to top menu")
  });
  
  pfMenu menu_ldmx_roc({
     pfMenu::Line("HARDRESET","Hard reset to all rocs", &ldmx_roc),
     pfMenu::Line("SOFTRESET","Soft reset to all rocs", &ldmx_roc),
     pfMenu::Line("RESYNCLOAD","ResyncLoad to all rocs to help maintain link stability", &ldmx_roc),
     pfMenu::Line("IROC","Change the active ROC number", &ldmx_roc ),
     pfMenu::Line("CHAN","Dump link status", &ldmx_roc ),
     pfMenu::Line("PAGE","Dump a page", &ldmx_roc ),
     pfMenu::Line("PARAM_NAMES", "Print a list of parameters on a page", &ldmx_roc),
     pfMenu::Line("POKE_REG","Change a single register value", &ldmx_roc ),
     pfMenu::Line("POKE_PARAM","Change a single parameter value", &ldmx_roc ),
     pfMenu::Line("POKE","Alias for POKE_PARAM", &ldmx_roc ),
     pfMenu::Line("LOAD_REG","Load register values onto the chip from a CSV file", &ldmx_roc ),
     pfMenu::Line("LOAD_PARAM","Load parameter values onto the chip from a YAML file", &ldmx_roc ),
     pfMenu::Line("LOAD","Alias for LOAD_PARAM", &ldmx_roc ),
     pfMenu::Line("DUMP","Dump hgcroc settings to a file", &ldmx_roc ),
     pfMenu::Line("QUIT","Back to top menu")
    }, ldmx_roc_render);
  
  pfMenu menu_ldmx_bias({
  //  pfMenu::Line("STATUS","Read the bias line settings", &ldmx_bias ),
    pfMenu::Line("INIT","Initialize a board", &ldmx_bias ),
    pfMenu::Line("SET","Set a specific bias line setting", &ldmx_bias ),
    pfMenu::Line("LOAD","Load bias values from file", &ldmx_bias ),
    pfMenu::Line("QUIT","Back to top menu"),
  });
  
  pfMenu menu_ldmx_fc({
    pfMenu::Line("STATUS","Check status and counters", &ldmx_fc ),
    pfMenu::Line("SW_L1A","Send a software L1A", &ldmx_fc ),
    pfMenu::Line("LINK_RESET","Send a link reset", &ldmx_fc ),
    pfMenu::Line("BUFFER_CLEAR","Send a buffer clear", &ldmx_fc ),
    pfMenu::Line("COUNTER_RESET","Reset counters", &ldmx_fc ),
    pfMenu::Line("FC_RESET","Reset the fast control", &ldmx_fc ),
    pfMenu::Line("MULTISAMPLE","Setup multisample readout", &ldmx_fc ),
    pfMenu::Line("CALIB","Setup calibration pulse", &ldmx_fc ),
    pfMenu::Line("QUIT","Back to top menu")
  });
  
  pfMenu menu_ldmx_daq_debug({
    pfMenu::Line("STATUS","Provide the status", &ldmx_daq_debug ),
    pfMenu::Line("FULL_DEBUG", "Toggle debug mode for full-event buffer",  &ldmx_daq_debug ),
    pfMenu::Line("DISABLE_ROCLINKS", "Disable ROC links to drive only from SW",  &ldmx_daq_debug ),
    pfMenu::Line("READ", "Read an event", &ldmx_daq),
    pfMenu::Line("ROC_LOAD", "Load a practice ROC events from a file",  &ldmx_daq_debug ),
    pfMenu::Line("ROC_SEND", "Generate a SW L1A to send the ROC buffers to the builder",  &ldmx_daq_debug ),
    pfMenu::Line("FULL_LOAD", "Load a practice full event from a file",  &ldmx_daq_debug ),
    pfMenu::Line("FULL_SEND", "Send the buffer to the off-detector electronics",  &ldmx_daq_debug ),
    pfMenu::Line("SPY", "Spy on the front-end buffer",  &ldmx_daq_debug ),
    pfMenu::Line("IBSPY","Spy on an input buffer",  &ldmx_daq_debug ),
    pfMenu::Line("EFSPY","Spy on an event formatter buffer",  &ldmx_daq_debug ),
    pfMenu::Line("QUIT","Back to top menu")
  });
  
  pfMenu menu_ldmx_daq_setup({
    pfMenu::Line("STATUS", "Status of the DAQ", &ldmx_daq_setup),
    pfMenu::Line("ENABLE", "Toggle enable status", &ldmx_daq_setup),
    pfMenu::Line("ZS", "Toggle ZS status", &ldmx_daq_setup),
    pfMenu::Line("L1APARAMS", "Setup parameters for L1A capture", &ldmx_daq_setup),
    pfMenu::Line("FPGA", "Set FPGA id", &ldmx_daq_setup),
    pfMenu::Line("STANDARD","Do the standard setup for HCAL", &ldmx_daq_setup),
    pfMenu::Line("MULTISAMPLE","Setup multisample readout", &ldmx_fc ),
#ifdef PFTOOL_ROGUE
    pfMenu::Line("DMA", "Enable/disable DMA readout (only available with rogue)", &ldmx_daq_setup),
#endif
    pfMenu::Line("QUIT","Back to DAQ menu")
  });
  
  pfMenu menu_ldmx_daq({
    pfMenu::Line("DEBUG", "Debugging menu",  &menu_ldmx_daq_debug ),
    pfMenu::Line("STATUS", "Status of the DAQ", &ldmx_daq),
    pfMenu::Line("SETUP", "Setup the DAQ", &menu_ldmx_daq_setup),
    pfMenu::Line("RESET", "Reset the DAQ", &ldmx_daq),
    pfMenu::Line("HARD_RESET", "Reset the DAQ, including all parameters", &ldmx_daq),
    pfMenu::Line("PEDESTAL","Take a simple random pedestal run", &ldmx_daq),
    pfMenu::Line("CHARGE","Take a charge-injection run", &ldmx_daq),
    pfMenu::Line("QUIT","Back to top menu")
  });
  
  pfMenu menu_expert({ 
    pfMenu::Line("OLINK","Optical link functions", &menu_ldmx_link),
    pfMenu::Line("WB","Raw wishbone interactions", &menu_wishbone ),
    pfMenu::Line("I2C","Access the I2C Core", &menu_ldmx_i2c ),
    pfMenu::Line("QUIT","Back to top menu")
  });
  
  pfMenu menu_utop({ 
    pfMenu::Line("STATUS","Status summary", &ldmx_status),
    pfMenu::Line("FAST_CONTROL","Fast Control", &menu_ldmx_fc ),
    pfMenu::Line("ROC","ROC Configuration", &menu_ldmx_roc ),
    pfMenu::Line("BIAS","BIAS voltage setting", &menu_ldmx_bias ),
    pfMenu::Line("ELINKS","Manage the elinks", &menu_ldmx_elinks ),
    pfMenu::Line("DAQ","DAQ", &menu_ldmx_daq ),
    pfMenu::Line("EXPERT","Expert functions", &menu_expert ),
    pfMenu::Line("EXIT","Exit this tool")
  });

  menu_utop.steer( pft_ ) ;
}

void wb_action( const std::string& cmd, PolarfireTarget* pft ) {
  static uint32_t target=0;
  static uint32_t addr=0;
  if (cmd=="RESET") {
    pft->wb->wb_reset();
  }
  if (cmd=="WRITE") {
    target=BaseMenu::readline_int("Target",target);
    addr=BaseMenu::readline_int("Address",addr);
    uint32_t val=BaseMenu::readline_int("Value",0);
    pft->wb->wb_write(target,addr,val);
  }
  if (cmd=="READ") {
    target=BaseMenu::readline_int("Target",target);
    addr=BaseMenu::readline_int("Address",addr);
    uint32_t val=pft->wb->wb_read(target,addr);
    printf(" Read: 0x%08x\n",val);
  }
  if (cmd=="BLOCKREAD") {
    target=BaseMenu::readline_int("Target",target);
    addr=BaseMenu::readline_int("Starting address",addr);
    int len=BaseMenu::readline_int("Number of words",8);
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
  std::pair<int,int> version = pft->getFirmwareVersion();
  printf(" Polarfire firmware : %4x.%02x\n",version.first,version.second);
  printf("  Active DAQ links: ");
  for (int ilink=0; ilink<pft->hcal.elinks().nlinks(); ilink++)
    if (pft->hcal.elinks().isActive(ilink)) printf("%d ",ilink);
  printf("\n");
}

void ldmx_i2c( const std::string& cmd, PolarfireTarget* pft ) {
  static uint32_t addr=0;
  static int waddrlen=1;
  static int i2caddr=0;
  pflib::I2C i2c(pft->wb);

  if (cmd=="BUS") {
    int ibus=i2c.get_active_bus();
    ibus=BaseMenu::readline_int("Bus to make active",ibus);
    i2c.set_active_bus(ibus);
  }
  if (cmd=="WRITE") {
    i2caddr=BaseMenu::readline_int("I2C Target ",i2caddr);
    uint32_t val=BaseMenu::readline_int("Value ",0);
    i2c.set_bus_speed(1000);
    i2c.write_byte(i2caddr,val);
  }
  if (cmd=="READ") {
    i2caddr=BaseMenu::readline_int("I2C Target",i2caddr);
    i2c.set_bus_speed(100);
//    ldmx->i2c().set_bus_speed(1000);
    uint8_t val=i2c.read_byte(i2caddr);
    printf("%02x : %02x\n",i2caddr,val);
  }
  if (cmd=="MULTIREAD") {
    i2caddr=BaseMenu::readline_int("I2C Target",i2caddr);
    waddrlen=BaseMenu::readline_int("Read address length",waddrlen);
    std::vector<uint8_t> waddr;
    if (waddrlen>0) {
      addr=BaseMenu::readline_int("Read address",addr);
      for (int i=0; i<waddrlen; i++) {
        waddr.push_back(uint8_t(addr&0xFF));
        addr=(addr>>8);
      }
    }
    int len=BaseMenu::readline_int("Read length",1);
    std::vector<uint8_t> data=i2c.general_write_read(i2caddr,waddr);
    for (size_t i=0; i<data.size(); i++)
      printf("%02x : %02x\n",int(i),data[i]);
  }
  if (cmd=="MULTIWRITE") {
    i2caddr=BaseMenu::readline_int("I2C Target",i2caddr);
    waddrlen=BaseMenu::readline_int("Write address length",waddrlen);
    std::vector<uint8_t> wdata;
    if (waddrlen>0) {
      addr=BaseMenu::readline_int("Write address",addr);
      for (int i=0; i<waddrlen; i++) {
        wdata.push_back(uint8_t(addr&0xFF));
        addr=(addr>>8);
      }
    }
    int len=BaseMenu::readline_int("Write data length",1);
    for (int j=0; j<len; j++) {
      char prompt[64];
      sprintf(prompt,"Byte %d: ",j);
      int id=BaseMenu::readline_int(prompt,0);
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
    bool reset_txpll=BaseMenu::readline_bool("TX-PLL Reset?", false);
    bool reset_gtxtx=BaseMenu::readline_bool("GTX-TX Reset?", false);
    bool reset_tx=BaseMenu::readline_bool("TX Reset?", false);
    bool reset_rx=BaseMenu::readline_bool("RX Reset?", false);
    static bool polarity=false;
    polarity=BaseMenu::readline_bool("Choose negative TX polarity?",polarity);
    static bool polarity2=false;
    polarity2=BaseMenu::readline_bool("Choose negative RX polarity?",polarity2);
    static bool bypass=false;
    bypass=BaseMenu::readline_bool("Bypass CRC on receiver?",false);
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
  
  pflib::Elinks& elinks=pft->hcal.elinks();
  static int ilink=0;
  if (cmd=="RELINK")
    pft->elink_relink(2);
  if (cmd=="SPY") {
    ilink=BaseMenu::readline_int("Which elink? ",ilink);
    std::vector<uint8_t> spy=elinks.spy(ilink);
    for (size_t i=0; i<spy.size(); i++)
      printf("%02d %05x\n",int(i),spy[i]);
  }
  if (cmd=="BITSLIP") {
    ilink=BaseMenu::readline_int("Which elink? ",ilink);
    
    int bitslip=BaseMenu::readline_int("Bitslip value (-1 for auto): ",elinks.getBitslip(ilink));
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
    int mode=BaseMenu::readline_int("Mode? (0=immediate, 1=L1A) ",0);
    ilink=BaseMenu::readline_int("Which elink? ",ilink);
    int presamples=BaseMenu::readline_int("Presamples? ",20);
    std::vector<uint32_t> words = pft->elinksBigSpy(ilink,presamples,mode==1);
    for (int i=0; i<presamples+100; i++) {
      printf("%03d %08x\n",i,words[i]);
    }
  }
  if (cmd=="DELAY") {
    ilink=BaseMenu::readline_int("Which elink? ",ilink);
    int idelay=BaseMenu::readline_int("Delay value: ",128);
    elinks.setDelay(ilink,idelay);
  }
  if (cmd=="HARD_RESET") {
    elinks.resetHard();
  }
  if (cmd=="SCAN") {
    ilink=BaseMenu::readline_int("Which elink? ",ilink);
    elinks.scanAlign(ilink);
  }
  if (cmd=="STATUS") {
    pft->elinkStatus(std::cout);
  } 
}

static int iroc=0;
void ldmx_roc_render(PolarfireTarget*) {
  printf(" Active ROC: %d\n",iroc);
}

void ldmx_roc( const std::string& cmd, PolarfireTarget* pft ) {
  if (cmd=="HARDRESET") {
    pft->hcal.hardResetROCs();
  }
  if (cmd=="SOFTRESET") {
    pft->hcal.softResetROC();
  }
  if (cmd=="RESYNCLOAD") {
    pft->hcal.resyncLoadROC();
  }
  if (cmd=="IROC") {
    iroc=BaseMenu::readline_int("Which ROC to manage: ",iroc);
  }
  pflib::ROC roc=pft->hcal.roc(iroc);
  if (cmd=="CHAN") {
    int chan=BaseMenu::readline_int("Which channel? ",0);
    std::vector<uint8_t> v=roc.getChannelParameters(chan);
    for (int i=0; i<int(v.size()); i++)
      printf("%02d : %02x\n",i,v[i]);
  }
  if (cmd=="PAGE") {
    int page=BaseMenu::readline_int("Which page? ",0);
    int len=BaseMenu::readline_int("Length?", 8);
    std::vector<uint8_t> v=roc.readPage(page,len);
    for (int i=0; i<int(v.size()); i++)
      printf("%02d : %02x\n",i,v[i]);
  }
  if (cmd=="PARAM_NAMES") {
    std::cout <<
      "Select a page type from the following list:\n"
      " - DigitalHalf\n"
      " - ChannelWise (used for Channel_, CALIB, and CM pages)\n"
      " - Top\n"
      " - MasterTDC\n"
      " - ReferenceVoltage\n"
      " - GlobalAnalog\n"
      << std::endl;
    std::string p = BaseMenu::readline("Page type? ", "Top");
    std::vector<std::string> param_names = pflib::parameters(p);
    for (const std::string& pn : param_names) {
      std::cout << pn << "\n";
    }
    std::cout << std::endl;
  }
  if (cmd=="POKE_REG") {
    int page=BaseMenu::readline_int("Which page? ",0);
    int entry=BaseMenu::readline_int("Offset: ",0);
    int value=BaseMenu::readline_int("New value: ");
  
    roc.setValue(page,entry,value);
  }
  if (cmd=="POKE"||cmd=="POKE_PARAM") {
    std::string page = BaseMenu::readline("Page: ", "Global_Analog_0");
    std::string param = BaseMenu::readline("Parameter: ");
    int val = BaseMenu::readline_int("New value: ");

    roc.applyParameter(page, param, val);
  }
  if (cmd=="LOAD_REG") {
    std::cout <<
      "\n"
      " --- This command expects a CSV file with columns [page,offset,value].\n"
      << std::flush;
    std::string fname = BaseMenu::readline("Filename: ");
    pft->loadROCRegisters(iroc,fname);
  }
  if (cmd=="LOAD"||cmd=="LOAD_PARAM") {
    std::cout <<
      "\n"
      " --- This command expects a YAML file with page names, parameter names and their values.\n"
      << std::flush;
    std::string fname = BaseMenu::readline("Filename: ");
    bool prepend_defaults = BaseMenu::readline_bool("Update all parameter values on the chip using the defaults in the manual for any values not provided? ", false);
    pft->loadROCParameters(iroc,fname,prepend_defaults);
  }
  if (cmd=="DUMP") {
    std::string fname_def_format = "hgcroc_"+std::to_string(iroc)+"_settings_%Y%m%d_%H%M%S.yaml";

    time_t t = time(NULL);
    struct tm *tm = localtime(&t);

    char fname_def[64];
    strftime(fname_def, sizeof(fname_def), fname_def_format.c_str(), tm); 
    
    std::string fname = BaseMenu::readline("Filename: ", fname_def);
	  bool decompile = BaseMenu::readline_bool("Decompile register values? ",true);
    pft->dumpSettings(iroc,fname,decompile);
  }
}

void ldmx_fc( const std::string& cmd, PolarfireTarget* pft ) {
  bool do_status=false;

  pflib::FastControl& fc=pft->hcal.fc();
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
    pft->hcal.fc().resetCounters();
    do_status=true;
  }
  if (cmd=="FC_RESET") {
    pft->hcal.fc().resetTransmitter();
  }
  if (cmd=="CALIB") {
    int len, offset;
    pft->backend->fc_get_setup_calib(len,offset);
    std::cout <<
      "NOTE: A known bug in uMNio firmware which has been patched in later versions\n"
      "      leads to the inability of the firmware to read some parameters.\n"
      "      If you are seeing 0 as the default even after setting these parameters,\n"
      "      you have this (slightly) buggy firmware."
      << std::endl << std::endl;
    len=BaseMenu::readline_int("Calibration pulse length?",len);
    offset=BaseMenu::readline_int("Calibration L1A offset?",offset);
    pft->backend->fc_setup_calib(len,offset);
  }
  if (cmd=="MULTISAMPLE") {
    bool multi;
    int nextra;
    pft->hcal.fc().getMultisampleSetup(multi,nextra);
    multi=BaseMenu::readline_bool("Enable multisample readout? ",multi);
    if (multi)
      nextra=BaseMenu::readline_int("Extra samples (total is +1 from this number) : ",nextra);
    pft->hcal.fc().setupMultisample(multi,nextra);
    // also, DMA needs to know
#ifdef PFTOOL_ROGUE
    auto rwbi=dynamic_cast<pflib::rogue::RogueWishboneInterface*>(pft->wb);
    if (rwbi) {
      bool enabled;
      uint8_t samples_per_event, fpgaid_i;
      rwbi->daq_get_dma_setup(fpgaid_i,samples_per_event, enabled);
      samples_per_event=nextra+1;
      rwbi->daq_dma_setup(fpgaid_i,samples_per_event);
    }
#endif
  }
  if (cmd=="STATUS" || do_status) {
    static const std::vector<std::string> bit_comments = {
      "all requests",
      "read requests",
      "",
      "",
      "",
      "calib pulse requests",
      "",
      ""
    };
    bool multi;
    int nextra;
    pft->hcal.fc().getMultisampleSetup(multi,nextra);
    if (multi) printf(" Multisample readout enabled : %d extra L1a (%d total samples)\n",nextra,nextra+1);
    else printf(" Multisaple readout disabled\n");
    printf(" Snapshot: %03x\n",pft->wb->wb_read(1,3));
    uint32_t sbe,dbe;
    pft->hcal.fc().getErrorCounters(sbe,dbe);
    printf("  Single bit errors: %d     Double bit errors: %d\n",sbe,dbe);
    std::vector<uint32_t> cnt=pft->hcal.fc().getCmdCounters();
    for (int i=0; i<8; i++) 
      printf("  Bit %d count: %u %s\n",i,cnt[i],bit_comments.at(i)); 
  }
}

void ldmx_daq_setup( const std::string& cmd, PolarfireTarget* pft ) {
  pflib::DAQ& daq=pft->hcal.daq();

  if (cmd=="STATUS") {
    pft->daqStatus(std::cout);
#ifdef PFTOOL_ROGUE
    auto rwbi=dynamic_cast<pflib::rogue::RogueWishboneInterface*>(pft->wb);
    if (rwbi) {
      printf("DMA : %08x\n",rwbi->daq_dma_status());
    } 
#endif
  }
  if (cmd=="ENABLE") {
    daq.enable(!daq.enabled());
  }
  if (cmd=="ZS") {
    int jlink=BaseMenu::readline_int("Which link (-1 for all)? ",-1);    
	  bool fullSuppress=BaseMenu::readline_bool("Suppress all channels? ",false);
    pft->enableZeroSuppression(jlink,fullSuppress);
  }
  if (cmd=="L1APARAMS") {
    int ilink=BaseMenu::readline_int("Which link? ",-1);    
    if (ilink>=0) {
      uint32_t reg1=pft->wb->wb_read(pflib::tgt_DAQ_Inbuffer,(ilink<<7)|1);
      int delay=BaseMenu::readline_int("L1A delay? ",(reg1>>8)&0xFF);
      int capture=BaseMenu::readline_int("L1A capture length? ",(reg1>>16)&0xFF);
      reg1=(reg1&0xFF)|((delay&0xFF)<<8)|((capture&0xFF)<<16);
      pft->wb->wb_write(pflib::tgt_DAQ_Inbuffer,(ilink<<7)|1,reg1);
    }
  }
  if (cmd=="DMA") {
#ifdef PFTOOL_ROGUE
    auto rwbi=dynamic_cast<pflib::rogue::RogueWishboneInterface*>(pft->wb);
    if (rwbi) {
      bool enabled;
      uint8_t samples_per_event, fpgaid_i;
      rwbi->daq_get_dma_setup(fpgaid_i,samples_per_event, enabled);
      enabled=BaseMenu::readline_bool("Enable DMA? ",enabled);
      rwbi->daq_dma_enable(enabled);
    } else {
      std::cout << "\nNot connected to chip with RogueWishboneInterface, cannot activate DMA.\n" << std::endl;
    }
#endif
  }
  if (cmd=="STANDARD") {
    int fpgaid=BaseMenu::readline_int("FPGA id: ",daq.getFPGAid());
    daq.setIds(fpgaid);
#ifdef PFTOOL_ROGUE
    auto rwbi=dynamic_cast<pflib::rogue::RogueWishboneInterface*>(pft->wb);
    if (rwbi) {
      bool enabled;
      uint8_t samples_per_event, fpgaid_i;
      rwbi->daq_get_dma_setup(fpgaid_i,samples_per_event, enabled);
      fpgaid_i=(uint8_t(fpgaid));
      rwbi->daq_dma_setup(fpgaid_i,samples_per_event);
    }
#endif
    pflib::Elinks& elinks=pft->hcal.elinks();
    
    for (int i=0; i<daq.nlinks(); i++) {
      if (elinks.isActive(i)) daq.setupLink(i,false,false,15,40);
      else daq.setupLink(i,true,true,15,40);
    }
  }
  if (cmd=="FPGA") {
    int fpgaid=BaseMenu::readline_int("FPGA id: ",daq.getFPGAid());
    daq.setIds(fpgaid);
#ifdef PFTOOL_ROGUE
    auto rwbi=dynamic_cast<pflib::rogue::RogueWishboneInterface*>(pft->wb);
    if (rwbi) {
      bool enabled;
      uint8_t samples_per_event, fpgaid_i;
      rwbi->daq_get_dma_setup(fpgaid_i,samples_per_event, enabled);
      fpgaid_i=(uint8_t(fpgaid));
      rwbi->daq_dma_setup(fpgaid_i,samples_per_event);
    }
#endif
  }
}

void ldmx_daq( const std::string& cmd, PolarfireTarget* pft ) {
  pflib::DAQ& daq=pft->hcal.daq();
  if (cmd=="STATUS") {
    pft->daqStatus(std::cout);
#ifdef PFTOOL_ROGUE
    auto rwbi=dynamic_cast<pflib::rogue::RogueWishboneInterface*>(pft->wb);
    if (rwbi) {
      printf("DMA : %08x\n",rwbi->daq_dma_status());
    }
#endif
  }
  if (cmd=="RESET") {
    pft->daqSoftReset();
  }
  if (cmd=="HARD_RESET") {
    pft->daqHardReset();
  }
  if (cmd=="READ") {
    std::vector<uint32_t> buffer = pft->daqReadDirect();
    for (size_t i=0; i<buffer.size() && i<32; i++) {
      printf("%04d %08x\n",int(i),buffer[i]);
    }
    bool save_to_disk=BaseMenu::readline_bool("Save to disk?  ",false);
    if (save_to_disk) {
      std::string fname=BaseMenu::readline("Filename :  ");
      FILE* f=fopen(fname.c_str(),"w");
      fwrite(&(buffer[0]),sizeof(uint32_t),buffer.size(),f);
      fclose(f);
    }
  }
  if (cmd=="PEDESTAL" || cmd=="CHARGE") {
    std::string fname_def_format = "pedestal_%Y%m%d_%H%M%S.raw";
    if (cmd=="CHARGE") fname_def_format = "charge_%Y%m%d_%H%M%S.raw";

    time_t t = time(NULL);
    struct tm *tm = localtime(&t);

    char fname_def[64];
    strftime(fname_def, sizeof(fname_def), fname_def_format.c_str(), tm); 
    
    int nevents=BaseMenu::readline_int("How many events? ", 100);
    std::string fname=BaseMenu::readline("Filename :  ", fname_def);
    FILE* f=fopen(fname.c_str(),"w");

    pft->prepareNewRun();
    
    for (int ievt=0; ievt<nevents; ievt++) {
      // normally, some other controller would send the L1A
      //  we are sending it so we get data during no signal
      if (cmd=="PEDESTAL")
        pft->backend->fc_sendL1A();
      if (cmd=="CHARGE")
        pft->backend->fc_calibpulse();
      
      std::vector<uint32_t> event = pft->daqReadEvent();
      fwrite(&(event[0]),sizeof(uint32_t),event.size(),f);      
    }
    fclose(f);
  }
  if (cmd=="SCAN"){
    std::string pagename=BaseMenu::readline("Sub-block (aka Page) name :  ");
    std::string valuename=BaseMenu::readline("Value (aka Parameter) name :  ");
    int iroc=BaseMenu::readline_int("Which ROC :  ");
    int minvalue=BaseMenu::readline_int("Minimum value :  ");
    int maxvalue=BaseMenu::readline_int("Maximum value :  ");
    int step=BaseMenu::readline_int("Step :  ");
    int nevents=BaseMenu::readline_int("Events per step :  ", 10);
    std::string fname=BaseMenu::readline("Filename :  ");

    // smart pointer lets us close file even if exception is thrown
    std::unique_ptr<FILE, int (*)(FILE*)> fp{fopen(fname.c_str(),"w"),&fclose};
    pft->prepareNewRun();

    for(int value = minvalue; value <= maxvalue; value += step){
      pft->hcal.roc(iroc).applyParameter(pagename, valuename, value);
      for (int ievt=0; ievt<nevents; ievt++) {
        pft->backend->fc_sendL1A();
        std::vector<uint32_t> event = pft->daqReadEvent();
        fwrite(&(event[0]),sizeof(uint32_t),event.size(),fp.get());
      }
    }
  }
}

std::vector<uint32_t> read_words_from_file() {
  std::vector<uint32_t> data;
  /// load from file
  std::string fname=BaseMenu::readline("Read from text file (line by line hex 32-bit words)");
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
    reg1=pft->wb->wb_read(pflib::tgt_DAQ_Control,1);
    printf(" Disable ROC links: %s\n",(reg1&0x80000000u)?("TRUE"):("FALSE"));

    printf(" Link  F E RP WP \n");
    for (int ilink=0; ilink<PolarfireTarget::NLINKS; ilink++) {
      uint32_t reg=pft->wb->wb_read(pflib::tgt_DAQ_Inbuffer,(ilink<<7)|3);
      printf("   %2d  %d %d %2d %2d       %08x\n",ilink,(reg>>26)&1,(reg>>27)&1,(reg>>16)&0xf,(reg>>12)&0xf,reg);
    }
    printf("-----Event builder    -----\n");
    reg1=pft->wb->wb_read(pflib::tgt_DAQ_Control,6);
    printf(" EVB Debug word: %08x\n",reg1);
    reg1=pft->wb->wb_read(pflib::tgt_DAQ_Outbuffer,1);
    reg2=pft->wb->wb_read(pflib::tgt_DAQ_Outbuffer,4);
    printf(" Event buffer Debug word: %08x %08x\n",reg1,reg2);

    printf("-----Full event buffer-----\n");
    reg1=pft->wb->wb_read(pflib::tgt_DAQ_Outbuffer,1);
    reg2=pft->wb->wb_read(pflib::tgt_DAQ_Outbuffer,2);    printf(" Read Page: %d  Write Page : %d   Full: %d  Empty: %d   Evt Length on current page: %d\n",(reg1>>13)&0x1,(reg1>>12)&0x1,(reg1>>15)&0x1,(reg1>>14)&0x1,(reg1>>0)&0xFFF);
    printf(" Spy page : %d  Spy-as-source : %d  Length-of-spy-injected-event : %d\n",reg2&0x1,(reg2>>1)&0x1,(reg2>>16)&0xFFF);
  } 
  if (cmd=="FULL_DEBUG") {
    uint32_t reg2=pft->wb->wb_read(pflib::tgt_DAQ_Outbuffer,2);
    reg2=reg2^0x2;// invert
    pft->wb->wb_write(pflib::tgt_DAQ_Outbuffer,2,reg2);
  }
  if (cmd=="DISABLE_ROCLINKS") {
    uint32_t reg=pft->wb->wb_read(pflib::tgt_DAQ_Control,1);
    reg=reg^0x80000000u;// invert
    pft->wb->wb_write(pflib::tgt_DAQ_Control,1,reg);
  }
  if (cmd=="FULL_SEND") {
    uint32_t reg2=pft->wb->wb_read(pflib::tgt_DAQ_Outbuffer,2);
    reg2=reg2|0x1000;// set the send bit
    pft->wb->wb_write(pflib::tgt_DAQ_Outbuffer,2,reg2);
  }
  if (cmd=="ROC_SEND") {
    uint32_t reg2=pft->wb->wb_read(pflib::tgt_DAQ_Control,1);
    reg2=reg2|0x40000000;// set the send bit
    pft->wb->wb_write(pflib::tgt_DAQ_Control,1,reg2);
  }
  if (cmd=="FULL_LOAD") {
    std::vector<uint32_t> data=read_words_from_file();

    // set the spy page to match the read page
    uint32_t reg1=pft->wb->wb_read(pflib::tgt_DAQ_Outbuffer,1);
    uint32_t reg2=pft->wb->wb_read(pflib::tgt_DAQ_Outbuffer,2);
    int rpage=(reg1>>12)&0x1;
    if (rpage) reg2=reg2|0x1;
    else reg2=reg2&0xFFFFFFFEu;
    pft->wb->wb_write(pflib::tgt_DAQ_Outbuffer,2,reg2);

    for (size_t i=0; i<data.size(); i++) 
      pft->wb->wb_write(pflib::tgt_DAQ_Outbuffer,0x1000+i,data[i]);
          
    /// set the length
    reg2=pft->wb->wb_read(pflib::tgt_DAQ_Outbuffer,2);
    reg2=reg2&0xFFFF;// remove the upper bits
    reg2=reg2|(data.size()<<16);
    pft->wb->wb_write(pflib::tgt_DAQ_Outbuffer,2,reg2);
  }
  if (cmd=="IBSPY") {
    static int input=0;
    input=BaseMenu::readline_int("Which input?",input);
    uint32_t reg=pft->wb->wb_read(pflib::tgt_DAQ_Inbuffer,(input<<7)|3);
    int rp=BaseMenu::readline_int("Read page?",(reg>>16)&0xF);
    reg=reg&0xFFFFFFF0u;
    reg=reg|(rp&0xF);
    pft->wb->wb_write(pflib::tgt_DAQ_Inbuffer,((input<<7)|3),reg);
    for (int i=0; i<40; i++) {
      uint32_t val=pft->wb->wb_read(pflib::tgt_DAQ_Inbuffer,(input<<7)|0x40|i);
      printf("%2d %08x\n",i,val);
    }      
  }
  if (cmd=="EFSPY") {
    static int input=0;
    input=BaseMenu::readline_int("Which input?",input);
    pft->wb->wb_write(pflib::tgt_DAQ_LinkFmt,(input<<7)|3,0);
    uint32_t reg=pft->wb->wb_read(pflib::tgt_DAQ_LinkFmt,(input<<7)|4);
    printf("PTRs now: 0x%08x\n",reg);
    int rp=BaseMenu::readline_int("Read page?",reg&0xF);
    for (int i=0; i<40; i++) {
      pft->wb->wb_write(pflib::tgt_DAQ_LinkFmt,(input<<7)|3,0x400|(rp<<6)|i);
      uint32_t val=pft->wb->wb_read(pflib::tgt_DAQ_LinkFmt,(input<<7)|4);
      printf("%2d %08x\n",i,val);
    }
    pft->wb->wb_write(pflib::tgt_DAQ_LinkFmt,(input<<7)|3,0);    
  }
  if (cmd=="SPY") {
    // set the spy page to match the read page
    uint32_t reg1=pft->wb->wb_read(pflib::tgt_DAQ_Outbuffer,1);
    uint32_t reg2=pft->wb->wb_read(pflib::tgt_DAQ_Outbuffer,2);
    int rpage=(reg1>>12)&0x1;
    if (rpage) reg2=reg2|0x1;
    else reg2=reg2&0xFFFFFFFEu;
    pft->wb->wb_write(pflib::tgt_DAQ_Outbuffer,2,reg2);

    for (size_t i=0; i<32; i++) {
      uint32_t val=pft->wb->wb_read(pflib::tgt_DAQ_Outbuffer,0x1000|i);
      printf("%04d %08x\n",int(i),val);
    }
  }
    
  if (cmd=="ROC_LOAD") {
    std::vector<uint32_t> data=read_words_from_file();
    if (int(data.size())!=PolarfireTarget::NLINKS*40) {
      printf("Expected %d words, got only %d\n",PolarfireTarget::NLINKS*40,int(data.size()));
      return;
    }
    for (int ilink=0; ilink<PolarfireTarget::NLINKS; ilink++) {
      uint32_t reg;
      // set the wishbone page to match the read page, and set the length
      reg=pft->wb->wb_read(pflib::tgt_DAQ_Inbuffer,(ilink<<7)+3);
      int rpage=(reg>>16)&0xF;
      reg=(reg&0xFFFFF000u)|rpage|(40<<4);
      pft->wb->wb_write(pflib::tgt_DAQ_Inbuffer,(ilink<<7)+3,reg);
      // load the bytes
      for (int i=0; i<40; i++)
        pft->wb->wb_write(pflib::tgt_DAQ_Inbuffer,(ilink<<7)|0x40|i,data[40*ilink+i]);      
    }
  }
}

void ldmx_bias( const std::string& cmd, PolarfireTarget* pft ) {

  static int iboard=0;
  if (cmd=="STATUS") {
    iboard=BaseMenu::readline_int("Which board? ",iboard);
    pflib::Bias bias=pft->hcal.bias(iboard);
  }
  
  if (cmd=="INIT") {
    iboard=BaseMenu::readline_int("Which board? ",iboard);
    pflib::Bias bias=pft->hcal.bias(iboard);
    bias.initialize();
  }
  if (cmd=="SET") {
    iboard=BaseMenu::readline_int("Which board? ",iboard);    
    static int led_sipm=0;
    led_sipm=BaseMenu::readline_int(" SiPM(0) or LED(1)? ",led_sipm);
    int ichan=BaseMenu::readline_int(" Which HDMI connector? ",-1);
    int dac=BaseMenu::readline_int(" What DAC value? ",0);
    if (ichan>=0) {
      pft->setBiasSetting(iboard,led_sipm==1,ichan,dac);
    }
  }
  if (cmd=="LOAD") {
    printf("\n --- This command expects a CSV file with four columns [0=SiPM/1=LED,board,hdmi#,value].\n");
    printf(" --- Line starting with # are ignored.\n");
    std::string fname=BaseMenu::readline("Filename: ");
    if (not pft->loadBiasSettings(fname)) {
      std::cerr << "\n\n  ERROR: Unable to access " << fname << std::endl;
    }
  }
}

