#include "pftool.h"

/**
 * @file pftool.cc File defining the pftool menus and their commands.
 *
 * The commands are written into functions corresponding to the menu's name.
 */

/**
 * pull the target of our menu into this source file to reduce code
 */
using pflib::PolarfireTarget;

/**
 * The type of menu we are constructing
 */
using pftool = Menu<PolarfireTarget>;

/**
 * Main status of menu
 *
 * Prints the firmware version and which links are labeled as active.
 *
 * @param[in] pft pointer to active target
 */
static void status( PolarfireTarget* pft ) {
  std::pair<int,int> version = pft->getFirmwareVersion();
  printf(" Polarfire firmware : %4x.%02x\n",version.first,version.second);
  printf("  Active DAQ links: ");
  for (int ilink=0; ilink<pft->hcal.elinks().nlinks(); ilink++)
    if (pft->hcal.elinks().isActive(ilink)) printf("%d ",ilink);
  printf("\n");
}



/**
 * LINK menu commands
 *
 * @note All interaction with target has been commented out.
 * These commands do nothing at the moment.
 *
 * @param[in] cmd LINK command
 * @param[in] pft active target
 */
static void link( const std::string& cmd, PolarfireTarget* pft ) {
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

namespace {

auto menu_expert = pftool::menu("EXPERT", "expert functions");

auto menu_wishbone = menu_expert->submenu("WB", "raw wishbone interactions")
  ->line("RESET", "Enable/disable (toggle)",  wb )
  ->line("READ", "Read from an address",  wb )
  ->line("BLOCKREAD", "Read several words starting at an address",  wb )
  ->line("WRITE", "Write to an address",  wb )
  ->line("STATUS", "Wishbone errors counters",  wb )
;

auto menu_i2c = menu_expert->submenu("I2C", "raw I2C interactions")
  ->line("BUS","Pick the I2C bus to use", i2c )
  ->line("READ", "Read from an address",  i2c )
  ->line("WRITE", "Write to an address",  i2c )
  ->line("MULTIREAD", "Read from an address",  i2c )
  ->line("MULTIWRITE", "Write to an address",  i2c )
;

auto menu_olink = menu_expert->submenu("OLINK","optical link interactions (NOT IMPLEMENTED)")
      /*
  ->line("STATUS","Dump link status", link )
  ->line("CONFIG","Setup link", link )
  ->line("SPY", "Spy on the uplink",  link )
    */
;

auto menu_elinks = pftool::menu("ELINKS","manage the elinks")
  ->line("RELINK","Follow standard procedure to establish links", elinks)
  ->line("HARD_RESET","Hard reset of the PLL", elinks)
  ->line("STATUS", "Elink status summary",  elinks )
  ->line("SPY", "Spy on an elink",  elinks )
  ->line("AUTOALIGN", "Attempt to re-align automatically", elinks)
  ->line("HEADER_CHECK", "Do a pedestal run and tally good/bad headers, only non-DMA", elinks)
  ->line("ALIGN", "Align elink using packet headers and idle patterns, only non-DMA", elinks)
  ->line("BITSLIP", "Set the bitslip for a link or turn on auto", elinks)
  ->line("SCAN", "Scan on an elink",  elinks )
  ->line("DELAY", "Set the delay on an elink", elinks)
  ->line("BIGSPY", "Take a spy of a specific channel at 32-bits", elinks)
;

auto menu_roc = pftool::menu("ROC","Read-Out Chip Configuration", roc_render)
   ->line("HARDRESET","Hard reset to all rocs", roc)
   ->line("SOFTRESET","Soft reset to all rocs", roc)
   ->line("RESYNCLOAD","ResyncLoad to specified roc to help maintain link stability", roc)
   ->line("IROC","Change the active ROC number", roc )
   ->line("CHAN","Dump link status", roc )
   ->line("PAGE","Dump a page", roc )
   ->line("PARAM_NAMES", "Print a list of parameters on a page", roc)
   ->line("POKE_REG","Change a single register value", roc )
   ->line("POKE_PARAM","Change a single parameter value", roc )
   ->line("POKE","Alias for POKE_PARAM", roc )
   ->line("POKE_ALL_ROCHALVES", "Like POKE_PARAM, but applies parameter to both halves of all ROCs", roc)
   ->line("POKE_ALL_CHANNELS", "Like POKE_PARAM, but applies parameter to all channels of the all ROCs", roc)
   ->line("LOAD_REG","Load register values onto the chip from a CSV file", roc )
   ->line("LOAD_PARAM","Load parameter values onto the chip from a YAML file", roc )
   ->line("LOAD","Alias for LOAD_PARAM", roc )
   ->line("DUMP","Dump hgcroc settings to a file", roc )
   ->line("DEFAULT_PARAMS", "Load default YAML files", roc)
;

auto menu_bias = pftool::menu("BIAS","bias voltage settings")
  //->line("STATUS","Read the bias line settings", bias )
  ->line("INIT","Initialize a board", bias )
  ->line("SET","Set a specific bias line setting", bias )
  ->line("SET_ALL", "Set a specific bias line setting to every connector", bias)
  ->line("LOAD","Load bias values from file", bias )
;

auto menu_fc = pftool::menu("FAST_CONTROL","configuration and testing of fast control")
  ->line("STATUS","Check status and counters", fc )
  ->line("SW_L1A","Send a software L1A", fc )
  ->line("LINK_RESET","Send a link reset", fc )
  ->line("BUFFER_CLEAR","Send a buffer clear", fc )
  ->line("RUN_CLEAR","Send a run clear", fc )
  ->line("COUNTER_RESET","Reset counters", fc )
  ->line("FC_RESET","Reset the fast control", fc )
  ->line("VETO_SETUP","Setup the L1 Vetos", fc )
  ->line("MULTISAMPLE","Setup multisample readout", fc )
  ->line("CALIB","Setup calibration pulse", fc )
  ->line("ENABLES","Enable various sources of signal", fc )
;

auto menu_daq = pftool::menu("DAQ","Data AcQuisition configuration and testing")
  ->line("STATUS", "Status of the DAQ", daq)
  ->line("RESET", "Reset the DAQ", daq)
  ->line("HARD_RESET", "Reset the DAQ, including all parameters", daq)
  ->line("PEDESTAL","Take a simple random pedestal run", daq)
  ->line("CHARGE","Take a charge-injection run", daq)
  ->line("EXTERNAL","Take an externally-triggered run", daq)
//  ->line("SCAN","Take many charge or pedestal runs while changing a single parameter", daq)
;

auto menu_daq_debug = menu_daq->submenu("DEBUG","expert functions for debugging DAQ")
  ->line("STATUS","Provide the status", daq_debug )
  ->line("FULL_DEBUG", "Toggle debug mode for full-event buffer",  daq_debug )
  ->line("DISABLE_ROCLINKS", "Disable ROC links to drive only from SW",  daq_debug )
  ->line("READ", "Read an event", daq)
  ->line("ROC_LOAD", "Load a practice ROC events from a file",  daq_debug )
  ->line("ROC_SEND", "Generate a SW L1A to send the ROC buffers to the builder",  daq_debug )
  ->line("FULL_LOAD", "Load a practice full event from a file",  daq_debug )
  ->line("FULL_SEND", "Send the buffer to the off-detector electronics",  daq_debug )
  ->line("SPY", "Spy on the front-end buffer",  daq_debug )
  ->line("IBSPY","Spy on an input buffer",  daq_debug )
  ->line("EFSPY","Spy on an event formatter buffer",  daq_debug )
;

auto menu_daq_setup = menu_daq->submenu("SETUP","setup the DAQ")
  ->line("STATUS", "Status of the DAQ", daq_setup)
  ->line("ENABLE", "Toggle enable status", daq_setup)
  ->line("ZS", "Toggle ZS status", daq_setup)
  ->line("L1APARAMS", "Setup parameters for L1A capture", daq_setup)
  ->line("FPGA", "Set FPGA id", daq_setup)
  ->line("STANDARD","Do the standard setup for HCAL", daq_setup)
  ->line("MULTISAMPLE","Setup multisample readout", fc )
#ifdef PFTOOL_ROGUE
  ->line("DMA", "Enable/disable DMA readout (only available with rogue)", daq_setup)
#endif
;

auto menu_tasks = pftool::menu("TASKS","various high-level tasks like scans and tunes")
//  ->line("RESET_POWERUP", "Execute FC,ELINKS,DAQ reset after power up", tasks)
  ->line("SCANCHARGE","Charge scan over all active channels", tasks)
  ->line("DELAYSCAN","Charge injection delay scan", tasks )
  ->line("BEAMPREP", "Run settings and optional configuration for taking beamdata", tasks)
  ->line("CALIBRUN", "Produce the calibration scans", tasks)
  ->line("TUNE_TOT", "Tune TOT globally and per-channel", tasks)
  ->line("PEDESTAL_READ", "foo", tasks)
  ->line("ALIGN_PREAMP", "foo", tasks)
  ->line("DACB","foo",tasks)
;

}

bool file_exists(const std::string& fname)
{
  FILE* f=fopen(fname.c_str(),"r");
  if (f==0) return false;
  fclose(f);
  return true;
}

/**
 * List the options that could be provided within the RC file
 *
 * @param[in] rcfile RC file prepare
 */
void prepareOpts(Rcfile& rcfile) {
  rcfile.declareVBool("roclinks",
      "Vector Bool[8] indicating which roc links are active");
  rcfile.declareString("ipbus_map_path",
      "Full path to directory containgin IP-bus mapping. Only required for uHal comm.");
  rcfile.declareString("default_hostname",
      "Hostname of polarfire to connect to if none are given on the command line");
  rcfile.declareVString("roc_configs", "Vector String[4] pointing to default .yaml configs for each roc");
  rcfile.declareString("runnumber_file",
                       "Full path to a file which contains the last run number");
}

/**
 * This is the main executable for the pftool.
 *
 * The options are prepared first so their help information
 * is available for the usage information.
 *
 * If '-h' or '--help' is the only parameter provided,
 * the usage information is printed and we exit; otherwise,
 * we continue.
 *
 * The RC files  are read from ${PFTOOLRC}, pftoolrc, and ~/.pftoolrc
 * with priority in that order.
 * Then the command line parameters are parsed; if not hostname(s) is(are)
 * provided on the command line, then the 'default_hostname' RC file option
 * is necessary.
 *
 * Initialization scripts (provided with `-s`) are parsed such that white space
 * and lines which start with `#` are ignored. The keystrokes in those "sripts"
 * are run upon launching of the menu and then the user gets control of the tool
 * back after the script is done.
 */
int main(int argc, char* argv[]) {
  Rcfile options;
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
    if (arg=="-s") {
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
    }
#ifdef PFTOOL_UHAL
#ifdef PFTOOL_ROGUE
    else if (arg=="-u") {
      isuhal  = true;
      isrogue = false;
    }
    else if (arg=="-r") {
      isrogue = true;
      isuhal  = false;
    }
#endif
#endif
    else {
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

      if (options.contents().has_key("runnumber_file"))
        last_run_file=options.contents().getString("runnumber_file");

      if (p_pft) {
      	// prepare the links
      	if (options.contents().is_vector("roclinks")) {
      	  std::vector<bool> actives=options.contents().getVBool("roclinks");
      	  for (int ilink=0;
              ilink<p_pft->hcal.elinks().nlinks() and ilink<int(actives.size());
              ilink++) p_pft->hcal.elinks().markActive(ilink,actives[ilink]);
      	}

        if (options.contents().is_vector("roc_configs")) {
          p_pft->findDefaultROCParameters(options.contents().getVString("roc_configs"));
        }

        status(p_pft.get());
        pftool::run(p_pft.get());
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
