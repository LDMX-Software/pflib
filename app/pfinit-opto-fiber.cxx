#include "pflib/menu/Menu.h"
#include "pflib/Parameters.h"
#include "pflib/logging/Logging.h"
#include "pflib/zcu/zcu_optolink.h"
#ifdef USE_ROGUE
#include "pflib/bittware/bittware_optolink.h"
#endif
#include "pflib/lpgbt/lpGBT_standard_configs.h"
#include "pflib/lpGBT.h"

// TODO: move to OptoLink to share between app/lpgbt and here
void print_opto_status(pflib::OptoLink& olink) {
  printf("Polarity -- TX: %d  RX: %d\n", olink.get_tx_polarity(),
         olink.get_rx_polarity());
  std::map<std::string, uint32_t> info;
  info = olink.opto_status();
  printf("Optical status:\n");
  for (auto i : info) {
    printf("  %-20s : 0x%04x\n", i.first.c_str(), i.second);
  }
  info = olink.opto_rates();
  printf("Optical rates:\n");
  for (auto i : info) {
    printf("  %-20s : %.3f MHz (0x%04x)\n", i.first.c_str(), i.second / 1e3,
           i.second);
  }
}

// TODO: move to lpGBT to share between app/lpgbt and here
void i2c_scan(int ibus, pflib::lpGBT& lpgbt) {
  lpgbt.setup_i2c(ibus, 100);
  for (int addr = 0; addr < 0x80; addr++) {
    bool success = true;
    char failchar;
    lpgbt.start_i2c_read(ibus, addr);  // try reading a byte
    try {
      lpgbt.i2c_transaction_check(ibus, true);
    } catch (pflib::Exception& e) {
      if (e.name() == "I2CErrorNoCLK")
        failchar = 'C';
      else if (e.name() == "I2CErrorNoACK")
        failchar = '-';
      else if (e.name() == "I2CErrorSDALow")
        failchar = '*';
      else
        failchar = '?';
      //	printf(e.what());
      success = false;
    }
    if ((addr % 0x10) == 0) printf("\n%02x ", addr);
    if (success)
      printf("%02x ", addr);
    else
      printf("%c%c ", failchar, failchar);
  }
  printf("\n");
}

int main(int argc, char* argv[]) {
  pflib::logging::fixture f;
  auto the_log_{pflib::logging::get("pfinit-opto-fiber")};

  if (argc < 2 or (!strcmp(argv[1], "-h") or !strcmp(argv[1], "--help"))) {
    printf("\nUSAGE:\n");
    printf("   %s [-h | --help] config.yaml\n\n",
           argv[0]);
    printf("OPTIONS:\n");
    printf("  -h|--help : print this help and exit\n");
    printf("\n");
    return 1;
  }

  std::vector<std::string> configs;
  for (int i = 1; i < argc; i++) {
    configs.push_back(argv[i]);
  }

  if (configs.size() > 1) {
    pflib_log(error) << "Only one config.yaml at a time rightnow.";
    return 2;
  }

  pflib::Parameters configuration;
  configuration.from_yaml(configs[0], true);

  auto pftool_params{configuration.get<pflib::Parameters>("pftool", {})};
  if (pftool_params.exists("log_level")) {
    pflib::logging::set(
        pflib::logging::convert(pftool_params.get<int>("log_level")));
  }

  if (not configuration.exists("target")) {
    pflib_log(error) << "Need to define a 'target' in the configuration.";
    return 3;
  }

  auto target{configuration.get<pflib::Parameters>("target")};

  if (not target.exists("type")) {
    pflib_log(error) << "Need to define target's 'type'.";
    return 4;
  }

  bool bittware = false;
  int i_link = 0;
  std::string target_type{target.get<std::string>("type")};
  std::string target_name;
  if (target_type == "Fiberless" or target_type == "HcalFMC") {
    pflib_log(fatal) << "No reason to initialize optical fibers on fiberless targets.";
    return 5;
  } else if (target_type == "HcalBackplaneZCU" or target_type == "EcalSMMZCU") {
    bittware = false;
    i_link = target.get<int>("ilink");
    if (i_link < 0 or i_link > 1) {
      pflib_log(fatal) << "ZCU i_link can only be 0 or 1";
    }
    target_name = "standardLpGBTpair-"+std::to_string(i_link);
  } else if (target_type == "HcalBackplaneBittware" or target_type == "EcalSMMBittware") {
#ifdef USE_ROGUE
    bittware = true;
    i_link = target.get<int>("ilink");
    target_name = target.get<std::string>("dev", "/dev/datadev_0");
#else
    pflib_log(fatal) << "Bittware type targets require Rogue";
    return 1;
#endif
  } else {
    pflib_log(fatal) << "Target type '" << target_type << "' not recognized.";
    return 1;
  }

  std::unique_ptr<pflib::OptoLink> olink_data, olink_trig;
  if (bittware) {
#ifdef USE_ROGUE
    auto bw_olink_data = new pflib::bittware::BWOptoLink(i_link, target_name.c_str());
    olink_data.reset(bw_olink_data);
    olink_trig = std::make_unique<pflib::bittware::BWOptoLink>(i_link + 1, *bw_olink_data);
#endif
  } else {
    olink_data = std::make_unique<pflib::zcu::ZCUOptoLink>(target_name, 2 * i_link + 0, true);
    olink_trig = std::make_unique<pflib::zcu::ZCUOptoLink>(target_name, 2 * i_link + 1, false);
  }

  int chipaddr = 0x78 | 0x4;

  pflib::lpGBT lpgbt_ic(olink_data->lpgbt_transport());
  pflib::lpGBT lpgbt_ec(olink_trig->lpgbt_transport());

  printf("DATA Optical Fiber\n");
  print_opto_status(*olink_data);

  pflib_log(info) << "reseting DATA fiber where slow control is";
  olink_data->reset_link();
  auto info = olink_data->opto_status();
  if (info["BUFFBYPASS_DONE"] != 1 or info["BUFFBYPASS_ERROR"] != 0) {
    pflib_log(error) << "unable to do buffer bypass, board was not off during olink reset";
    return 5;
  }
  usleep(10000);
  printf("DATA Optical Fiber\n");
  print_opto_status(*olink_data);

  // wait for power
  if (not pflib::menu::BaseMenu::readline_bool(
        "Waiting to re-power/re-connect the front-end ('n' to exit)...", true)) {
    pflib_log(debug) << "leaving";
    return 0;
  }

  usleep(1000); // let links get aligned
  printf("DATA Optical Fiber\n");
  print_opto_status(*olink_data);
  info = olink_data->opto_status();
  if (info["READY"] != 1) {
    pflib_log(error) << "board is not reporting READY";
    return 5;
  }

  info = olink_data->opto_rates();
  if (info["BX_CLK"] != info["LINK_WORD"] or info["LINK_ERROR"] > 0) {
    pflib_log(error) << "board not achieving optical link lock";
    return 5;
  }
  pflib_log(info) << "DATA optical fiber locked with board";

  pflib_log(info) << "applying lpGBT configuration";
  if (target_type == "HcalBackplaneZCU" or target_type == "HcalBackplaneBittware") {
    pflib::lpgbt::standard_config::setup_hcal_daq(lpgbt_ic);
    pflib_log(info) << "applied standard HCAL DAQ configuration, pausing to be loaded...";
    sleep(2);
    pflib::lpgbt::standard_config::setup_hcal_trig(lpgbt_ec);
    pflib_log(info) << "applied standard HCAL TRIG configuration";
  } else if (target_type == "EcalSMMZCU" or target_type == "EcalSMMBittware") {
    pflib::lpgbt::standard_config::setup_ecal(
        lpgbt_ic, pflib::lpgbt::standard_config::ECAL_lpGBT_Config::
                               DAQ_SingleModuleMotherboard);
    pflib_log(info) << "applied standard ECAL DAQ configuration, pausing to be loaded...";
    sleep(2);
    pflib::lpgbt::standard_config::setup_ecal(
        lpgbt_ec, pflib::lpgbt::standard_config::ECAL_lpGBT_Config::
                      TRIG_SingleModuleMotherboard);
    pflib_log(info) << "applied standard ECAL TRIG configuration";
  } else {
    pflib_log(error) << "unrecognized target type '" << target_type << "' when trying to apply lpGBT configuration";
    return 5;
  }

  // do i2c scan, Hcal uses Bus 0 and Bus 1 while Ecal only uses Bus 1
  if (target_type == "HcalBackplaneZCU" or target_type == "HcalBackplaneBittware") {
    printf("I2C Scan on Bus 0\n");
    i2c_scan(0, lpgbt_ic);
  }
  printf("I2C Scan on Bus 1\n");
  i2c_scan(1, lpgbt_ic);

  // try link trick on TRIG fiber
  pflib_log(info) << "do link trick on TRIG fiber";
  olink_trig->run_linktrick();
  usleep(1000);
  printf("TRIG Optical Fiber\n");
  print_opto_status(*olink_trig);

  return 0;
}
