#include "pftool_roc.h"
int iroc = 0;
void roc_render( PolarfireTarget* pft ) {
  printf(" Active ROC: %d\n",iroc);
}

void load_parameters(PolarfireTarget* pft, const int iroc) {
  std::cout << "\n"
               " --- This command expects a YAML file with page names, "
               "parameter names and their values.\n"
            << std::flush;
  const std::string default_yaml{"baseConfig.yaml"};
  std::string fname = BaseMenu::readline("Filename: ", default_yaml);
  bool prepend_defaults = BaseMenu::readline_bool(
      "Update all parameter values on the chip using the defaults in the "
      "manual for any values not provided? ",
      false);
  if (iroc == -1) {
    const int default_num_rocs{3};
    const int num_rocs{BaseMenu::readline_int(
        "How many ROCs are connected to this DPM?", default_num_rocs)};
    for (int roc_number{0}; roc_number < num_rocs; ++roc_number) {
      pft->loadROCParameters(roc_number, fname, prepend_defaults);
    }
  } else {
    pft->loadROCParameters(iroc, fname, prepend_defaults);
  }
}
void poke_all_channels(PolarfireTarget* pft, const std::string& parameter,
                       const int value) {
  const std::string page_template {"CHANNEL_"};
  const int default_num_rocs {3};
  const int default_num_channels{72};
  const int num_channels {BaseMenu::readline_int("How many channels per ROC?",
                                                 default_num_channels)};
  const int num_rocs {BaseMenu::readline_int("How many ROCs are connected to this DPM?",
                                             default_num_rocs)};

  for (int roc_number{0}; roc_number < num_rocs; ++roc_number) {
    pflib::ROC roc {pft->hcal.roc(roc_number)};
    for (int channel{0}; channel < num_channels; ++channel) {
      roc.applyParameter(page_template + std::to_string(channel), parameter, value);
    }
  }
}

void poke_all_rochalves(PolarfireTarget* pft,
                        const std::string& page_template,
                        const std::string& parameter,
                        const int value) {

  const int default_num_rocs {3};
  const int num_rocs {BaseMenu::readline_int("How many ROCs are connected to this DPM?",
                                             default_num_rocs)};
  // Avoid shadowing iroc
  for (int roc_number {0}; roc_number < num_rocs; ++roc_number) {
    pflib::ROC roc {pft->hcal.roc(roc_number)};
    roc.applyParameter(page_template + std::to_string(0), parameter, value);
    roc.applyParameter(page_template + std::to_string(1), parameter, value);
  }
}

void roc( const std::string& cmd, PolarfireTarget* pft )
{
  static std::string parameter;
  if (cmd=="HARDRESET") {
    pft->hcal.hardResetROCs();
  }
  if (cmd=="SOFTRESET") {
    iroc=BaseMenu::readline_int("Which ROC to reset [-1 for all]: ",iroc);
    pft->hcal.softResetROC(iroc);
  }
  if (cmd=="RESYNCLOAD") {
    iroc=BaseMenu::readline_int("Which ROC to reset [-1 for all]: ",iroc);
    pft->hcal.resyncLoadROC(iroc);
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
    parameter = BaseMenu::readline("Parameter: ", parameter);
    int val = BaseMenu::readline_int("New value: ");
    roc.applyParameter(page, parameter, val);
    return;
  }
  if (cmd == "POKE_ALL_CHANNELS") {
    parameter = BaseMenu::readline("Parameter: ", parameter);
    const int value {BaseMenu::readline_int("New value: ")};
    poke_all_channels(pft, parameter, value);
    return;
  }
  if (cmd == "POKE_ALL_ROCHALVES") {
    const std::string page_template = BaseMenu::readline(
      "Page template: (No 0/1 after last \"_\")", "Global_Analog_");
    parameter = BaseMenu::readline("Parameter: ", parameter);
    const int value {BaseMenu::readline_int("New value: ")};
    poke_all_rochalves(pft, page_template, parameter, value);
    return;
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
    load_parameters(pft, iroc);
  }
  if (cmd=="DEFAULT_PARAMS") {
    pft->loadDefaultROCParameters();
  }
  if (cmd=="DUMP") {
    std::string fname_def_format = "hgcroc_"+std::to_string(iroc)+"_settings_%Y%m%d_%H%M%S.yaml";

    time_t t = time(NULL)
;    struct tm *tm = localtime(&t);

    char fname_def[64];
    strftime(fname_def, sizeof(fname_def), fname_def_format.c_str(), tm);

    std::string fname = BaseMenu::readline("Filename: ", fname_def);
	  bool decompile = BaseMenu::readline_bool("Decompile register values? ",true);
    pft->dumpSettings(iroc,fname,decompile);
  }
}
