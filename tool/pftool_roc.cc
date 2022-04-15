#include "pftool_roc.h"
int iroc = 0;
void roc_render( PolarfireTarget* pft ) {
  printf(" Active ROC: %d\n",iroc);
}

int get_number_of_samples_per_event(PolarfireTarget* pft)
{
  int nsamples=1;
  {
    bool multi;
    int nextra;
    pft->hcal.fc().getMultisampleSetup(multi,nextra);
    if (multi) nsamples=nextra+1;
  }
  return nsamples;
}
int get_num_channels_per_elink()
{
  const int default_num_channels{36};
  static int num_channels {BaseMenu::readline_int("How many channels per ELINK? (Half of number of channels per ROC)",
                                                 default_num_channels)};
  return num_channels;

}
int get_num_channels_per_roc()
{
  return get_num_channels_per_elink() * 2;
}

int get_num_rocs()
{
    static int num_rocs = BaseMenu::readline_int("How many ROCs are connected to this DPM?", 3);
    return num_rocs;
}

int get_dpm_number() {
  static int dpm {BaseMenu::readline_int("Which DPM are you on?: ")};
  return dpm;
}
std::string make_roc_config_filename(const int config_version, const int roc)
{
  const int dpm {get_dpm_number()};
  const std::string config_version_string = std::to_string(config_version);

  return "config/v" + config_version_string
    + "/Config_v" +  config_version_string
    +"_dpm" + std::to_string(dpm)
    + "_board" + std::to_string(roc) + ".yaml";
}

void load_parameters(PolarfireTarget* pft, const int config_version,
                     const std::vector<std::string> filenames,
                     const bool prepend_defaults) {
  const int dpm {get_dpm_number()};
  const int num_rocs{get_num_rocs()};
    for (int roc_number{0}; roc_number < num_rocs; ++roc_number) {
      load_parameters(pft, roc_number, filenames[roc_number], prepend_defaults);
    }
}
void load_parameters(PolarfireTarget* pft, const int iroc, const std::string& fname,
                     const bool prepend_defaults) {
    pft->loadROCParameters(iroc, fname, prepend_defaults);
}

void load_parameters(PolarfireTarget* pft, const int iroc) {
  std::cout << "\n"
               " --- This command expects a YAML file with page names, "
               "parameter names and their values.\n"
            << std::flush;

  int config_version {BaseMenu::readline_int("Config version: ", 0)};
  const int dpm {get_dpm_number()};
  bool prepend_defaults = BaseMenu::readline_bool(
      "Update all parameter values on the chip using the defaults in the "
      "manual for any values not provided? ",
      false);
    if (iroc == -1) {
      const int num_rocs{get_num_rocs()};
      std::vector<std::string> roc_filenames{};
      for (int roc{0}; roc < num_rocs; ++roc) {
      std::string filename = BaseMenu::readline("Configuration filename for roc: " + std::to_string(roc),
                                           make_roc_config_filename(config_version, roc));
      roc_filenames.push_back(filename);

      }
      load_parameters(pft, config_version,  roc_filenames, prepend_defaults);
    } else {
    auto fname = BaseMenu::readline("Filename: ", make_roc_config_filename(config_version, iroc));
    load_parameters(pft, iroc, fname, prepend_defaults);
    }
}
void poke_all_channels(PolarfireTarget *pft, const std::string &parameter,
                       const int value)
{
  const std::string page_template {"CHANNEL_"};
  const int num_channels = get_num_channels_per_roc();
  const int num_rocs{get_num_rocs()};
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
                        const int value)
{
  const int num_rocs {get_num_rocs()};
  // Avoid shadowing iroc
  for (int roc_number {0}; roc_number < num_rocs; ++roc_number) {
    pflib::ROC roc {pft->hcal.roc(roc_number)};
    roc.applyParameter(page_template + std::to_string(0), parameter, value);
    roc.applyParameter(page_template + std::to_string(1), parameter, value);
  }
}

void roc( const std::string& cmd, PolarfireTarget* pft ) {
  // generate lists of page names and param names for those pages
  // for tab completion
  static std::vector<std::string> page_names;
  static std::map<std::string,std::vector<std::string>> param_names;
  if (page_names.empty()) {
    // only need to do this once 
    auto defs = pflib::defaults();
    for (const auto& page : defs) {
      page_names.push_back(page.first);
      for (const auto& param : page.second) {
        param_names[page.first].push_back(param.first);
      }
    }
  }

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
    std::string page = BaseMenu::readline("Page: ", page_names);
    if (param_names.find(page) == param_names.end()) {
      PFEXCEPTION_RAISE("BadPage","Page name "+page+" not recognized.");
    }
    const std::string parameter = BaseMenu::readline("Parameter: ", param_names.at(page));
    int val = BaseMenu::readline_int("New value: ");
    roc.applyParameter(page, parameter, val);
    return;
  }
  if (cmd == "POKE_ALL_CHANNELS") {
    const std::string parameter = BaseMenu::readline("Parameter: ", parameter);
    const int value {BaseMenu::readline_int("New value: ")};
    poke_all_channels(pft, parameter, value);
    return;
  }
  if (cmd == "POKE_ALL_ROCHALVES") {
    const std::string page_template = BaseMenu::readline(
      "Page template: (No 0/1 after last \"_\")", "Global_Analog_");
    const std::string parameter = BaseMenu::readline("Parameter: ", parameter);
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
    dump_rocconfig(pft, iroc);
  }
}

void dump_rocconfig(PolarfireTarget* pft, const int iroc) {
  std::string fname_def_format =
      "hgcroc_" + std::to_string(iroc) + "_settings_%Y%m%d_%H%M%S.yaml";

  time_t t = time(NULL);
  struct tm *tm = localtime(&t);

  char fname_def[64];
  strftime(fname_def, sizeof(fname_def), fname_def_format.c_str(), tm);

  std::string fname = BaseMenu::readline("Filename: ", fname_def);
  bool decompile = BaseMenu::readline_bool("Decompile register values? ", true);
  pft->dumpSettings(iroc, fname, decompile);
}
