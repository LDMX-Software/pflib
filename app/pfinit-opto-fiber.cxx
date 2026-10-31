#include "pflib/Parameters.h"
#include "pflib/logging/Logging.h"
#include "pflib/menu/Menu.h"
#include "pflib/zcu/zcu_optolink.h"
#ifdef USE_ROGUE
#include "pflib/bittware/bittware_optolink.h"
#endif
#include "pflib/lpGBT.h"
#include "pflib/lpgbt/lpGBT_standard_configs.h"

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

// one-line countdown while the front end powers up
void wait_for_power_up(int seconds) {
  const std::string label = "Waiting for the front end to power up";
  for (int left = seconds; left > 0; left--) {
    int done = seconds - left;
    printf("\r  %s [%s%s] %2d s ", label.c_str(),
           std::string(2 * done, '#').c_str(),
           std::string(2 * left, '.').c_str(), left);
    fflush(stdout);
    sleep(1);
  }
  printf("\r  %s [%s] done\n", label.c_str(),
         std::string(2 * seconds, '#').c_str());
}

struct LpGBTPair {
  mutable pflib::logging::logger the_log_{pflib::logging::get("LpGBTPair")};
  std::string name;
  pflib::Parameters configuration;
  bool bittware;
  bool hcal;
  std::unique_ptr<pflib::OptoLink> olink_data, olink_trig;
  std::unique_ptr<pflib::lpGBT> lpgbt_ic, lpgbt_ec;
  void from_yaml(const std::string& filepath) {
    name = filepath;
    configuration.from_yaml(filepath);
    auto target{configuration.get<pflib::Parameters>("target")};
    if (not target.exists("type")) {
      PFEXCEPTION_RAISE("BadConf",
                        "Target configuration does not define a 'type'.");
    }
    std::string target_type{target.get<std::string>("type")};
    std::string device;
    int i_link;
    if (target_type == "Fiberless" or target_type == "HcalFMC") {
      PFEXCEPTION_RAISE(
          "BadConf",
          "No reason to initialize optical fibers on fiberless targets.");
    } else if (target_type == "HcalBackplaneZCU" or
               target_type == "EcalSMMZCU") {
      bittware = false;
      i_link = target.get<int>("ilink");
      if (i_link < 0 or i_link > 1) {
        pflib_log(fatal) << "ZCU i_link can only be 0 or 1";
      }
      device = "standardLpGBTpair-" + std::to_string(i_link);
    } else if (target_type == "HcalBackplaneBittware" or
               target_type == "EcalSMMBittware") {
#ifdef USE_ROGUE
      bittware = true;
      i_link = target.get<int>("ilink");
      device = target.get<std::string>("dev", "/dev/datadev_0");
#else
      PFEXCEPTION_RAISE("BadComp", "Bittware type targets require Rogue");
#endif
    } else {
      PFEXCEPTION_RAISE("BadConf", target_type + " is not recognized.");
    }

    if (bittware) {
#ifdef USE_ROGUE
      auto bw_olink_data =
          new pflib::bittware::BWOptoLink(i_link, device.c_str());
      olink_data.reset(bw_olink_data);
      olink_trig = std::make_unique<pflib::bittware::BWOptoLink>(
          i_link + 1, *bw_olink_data);
#endif
    } else {
      olink_data = std::make_unique<pflib::zcu::ZCUOptoLink>(
          device, 2 * i_link + 0, true);
      olink_trig = std::make_unique<pflib::zcu::ZCUOptoLink>(
          device, 2 * i_link + 1, false);
    }

    lpgbt_ic = std::make_unique<pflib::lpGBT>(olink_data->lpgbt_transport());
    lpgbt_ec = std::make_unique<pflib::lpGBT>(olink_trig->lpgbt_transport());

    hcal = (target_type == "HcalBackplaneBittware" or
            target_type == "HcalBackplaneZCU");
  }
};

int main(int argc, char* argv[]) try {
  pflib::logging::fixture f;
  auto the_log_{pflib::logging::get("pfinit-opto-fiber")};

  if (argc < 2 or (!strcmp(argv[1], "-h") or !strcmp(argv[1], "--help"))) {
    printf("\nUSAGE:\n");
    printf("   %s [-h | --help] config.yaml\n\n", argv[0]);
    printf("OPTIONS:\n");
    printf("  -h|--help : print this help and exit\n");
    printf("\n");
    return 1;
  }

  std::vector<LpGBTPair> targets(argc - 1);
  for (int i = 1; i < argc; i++) {
    targets[i - 1].from_yaml(argv[i]);
  }

  for (const auto& tgt : targets) {
    printf("%s DATA Opto Fiber Status\n", tgt.name.c_str());
    print_opto_status(*tgt.olink_data);
  }

  pflib_log(info) << "reseting DATA fiber where slow control is";
  bool any_fail = false;
  for (const auto& tgt : targets) {
    tgt.olink_data->reset_link();
    auto info = tgt.olink_data->opto_status();
    if (info["BUFFBYPASS_DONE"] != 1 or info["BUFFBYPASS_ERROR"] != 0) {
      any_fail = true;
      pflib_log(error) << "unable to do buffer bypass for target '" << tgt.name
                       << "', board was probably not off during olink reset";
    }
  }
  if (any_fail) return 1;
  usleep(10000);

  for (const auto& tgt : targets) {
    printf("%s DATA Opto Fiber Status\n", tgt.name.c_str());
    print_opto_status(*tgt.olink_data);
  }

  // wait for power
  if (not pflib::menu::BaseMenu::readline_bool(
          "Waiting to re-power/re-connect the front-end ('n' to exit)...",
          true)) {
    pflib_log(debug) << "leaving";
    return 0;
  }

  wait_for_power_up(10);  // let the front end power up and links align

  any_fail = false;
  for (const auto& tgt : targets) {
    printf("%s DATA Opto Fiber Status\n", tgt.name.c_str());
    print_opto_status(*tgt.olink_data);
    auto info = tgt.olink_data->opto_status();
    if (info["READY"] != 1) {
      pflib_log(error) << "board is not reporting READY";
      any_fail = true;
    }
    info = tgt.olink_data->opto_rates();
    if (info["BX_CLK"] != info["LINK_WORD"] or info["LINK_ERROR"] > 0) {
      pflib_log(error) << "board not achieving optical link lock";
      any_fail = true;
    }
    pflib_log(info) << "DATA optical fiber locked with board";
  }
  if (any_fail) return 1;

  for (const auto& tgt : targets) {
    pflib_log(info) << "applying lpGBT configuration onto " << tgt.name;
    if (tgt.hcal) {
      pflib::lpgbt::standard_config::setup_hcal_daq(*tgt.lpgbt_ic);
      pflib_log(info)
          << "applied standard HCAL DAQ configuration, pausing to be loaded...";
      sleep(2);
      try {
        pflib::lpgbt::standard_config::setup_hcal_trig(*tgt.lpgbt_ec);
        pflib_log(info) << "applied standard HCAL TRIG configuration";
      } catch (const pflib::Exception& e) {
        // TRIG lpGBT is optional for DAQ, keep configuring the other targets
        pflib_log(warn) << "HCAL TRIG lpGBT not configured on " << tgt.name
                        << ": [" << e.name() << "] " << e.message();
      }
    } else {
      pflib::lpgbt::standard_config::setup_ecal(
          *tgt.lpgbt_ic, pflib::lpgbt::standard_config::ECAL_lpGBT_Config::
                             DAQ_SingleModuleMotherboard);
      pflib_log(info)
          << "applied standard ECAL DAQ configuration, pausing to be loaded...";
      sleep(2);
      try {
        pflib::lpgbt::standard_config::setup_ecal(
            *tgt.lpgbt_ec, pflib::lpgbt::standard_config::ECAL_lpGBT_Config::
                               TRIG_SingleModuleMotherboard);
        pflib_log(info) << "applied standard ECAL TRIG configuration";
      } catch (const pflib::Exception& e) {
        // TRIG lpGBT is optional for DAQ, keep configuring the other targets
        pflib_log(warn) << "ECAL TRIG lpGBT not configured on " << tgt.name
                        << ": [" << e.name() << "] " << e.message();
      }
    }
  }

  for (const auto& tgt : targets) {
    // do i2c scan, Hcal uses Bus 0 and Bus 1 while Ecal only uses Bus 1
    if (tgt.hcal) {
      pflib_log(info) << "I2C Scan on Bus 0 of " << tgt.name;
    }
    pflib_log(info) << "I2C Scan on Bus 1 of " << tgt.name;
    i2c_scan(1, *tgt.lpgbt_ic);
  }

  for (const auto& tgt : targets) {
    // try link trick on TRIG fiber
    if (not tgt.bittware) {
      pflib_log(info) << "do link trick on TRIG fiber for " << tgt.name;
      tgt.olink_trig->run_linktrick();
      usleep(1000);
      pflib_log(info) << "TRIG Opto Fiber Status from " << tgt.name;
      print_opto_status(*tgt.olink_trig);
    }
  }

  return 0;
} catch (const pflib::Exception& e) {
  auto the_log_{pflib::logging::get("pfinit-opto-fiber")};
  pflib_log(fatal) << "[" << e.name() << "]: " << e.message();
  return 126;
} catch (const std::runtime_error& e) {
  auto the_log_{pflib::logging::get("pfinit-opto-fiber")};
  pflib_log(fatal) << e.what();
  return 127;
}
