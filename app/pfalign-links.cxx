#include <vector>
#include <bitset>

#include "pflib/Parameters.h"
#include "pflib/logging/Logging.h"
#include "pflib/Target.h"
#include "pflib/ECON.h"
#include "pflib/ROC.h"
#include "pflib/align_econ_rocs.h"
#include "pflib/align_econ_lpgbt.h"

static auto the_log_{pflib::logging::get("pfalign-links")};

int main(int argc, char* argv[]) try {
  pflib::logging::fixture f;

  if (argc < 2 or (!strcmp(argv[1], "-h") or !strcmp(argv[1], "--help"))) {
    printf("\nUSAGE:\n");
    printf("   %s [-h | --help] config.yaml...\n\n", argv[0]);
    printf("OPTIONS:\n");
    printf("  -h|--help : print this help and exit\n");
    printf("\n");
    return 1;
  }

  std::vector<std::string> configs;
  for (int i = 1; i < argc; i++) {
    std::string arg{argv[i]};
    configs.push_back(arg);
  }

  for (const auto& config : configs) {
    pflib::Parameters configuration;
    configuration.from_yaml(config, true);

    // any pftool settings?

    if (not configuration.exists("target")) {
      PFEXCEPTION_RAISE("BadConf", "Need to specify a 'target'.");
    }

    auto target{configuration.get<pflib::Parameters>("target")};

    if (not target.exists("type")) {
      PFEXCEPTION_RAISE("BadConf", "Need to specify 'type' of target.");
    }

    // construct target
    auto target_type{target.get<std::string>("type")};
    bool is_hcal = false;
    bool on_zcu = false;
    std::unique_ptr<pflib::Target> tgt;
    if (target_type == "Fiberless" or target_type == "HcalFMC") {
      PFEXCEPTION_RAISE("BadConf",
          "There are no ECONs on fiberless targets to align.");
    } else if (target_type == "HcalBackplaneZCU") {
      is_hcal = true;
      on_zcu = true;
      auto ilink = target.get<int>("ilink");
      auto boardmask = target.get<int>("boardmask", 0xf);
      tgt.reset(pflib::makeTargetHcalBackplaneZCU(ilink, boardmask));
    } else if (target_type == "EcalSMMZCU") {
      on_zcu = true;
      auto ilink = target.get<int>("ilink");
      auto rocmask = target.get<int>("rocmask", 0x3f);
      tgt.reset(pflib::makeTargetEcalSMMZCU(ilink, rocmask));
    } else if (target_type == "HcalBackplaneBittware") {
#ifdef USE_ROGUE
      // need ilink to be in configuration
      is_hcal = true;
      auto ilink = target.get<int>("ilink");
      auto boardmask = target.get<int>("boardmask", 0xf);
      auto dev = target.get<std::string>("dev", "/dev/datadev_0");
      tgt.reset(pflib::makeTargetHcalBackplaneBittware(ilink, boardmask,
                                                       dev.c_str()));
#else
      pflib_log(fatal) << "Target type '" << target_type << "' requires Rogue.";
      return 1;
#endif
    } else if (target_type == "EcalSMMBittware") {
#ifdef USE_ROGUE
      // need ilink to be in configuration
      auto ilink = target.get<int>("ilink");
      auto dev = target.get<std::string>("dev", "/dev/datadev_0");
      auto rocmask = target.get<int>("rocmask", 0x3f);
      tgt.reset(pflib::makeTargetEcalSMMBittware(ilink, rocmask, dev.c_str()));
#else
      pflib_log(fatal) << "Target type '" << target_type << "' requires Rogue.";
      return 1;
#endif
    } else {
      PFEXCEPTION_RAISE("BadConf",
          "Unrecognized target type '"+target_type+"'.");
    }

    // basic configuration of ECONs

    // up to 12 eRx on up to 3 ECONs
    std::vector<std::bitset<12>> erx_enable(3);
    for (int roc : tgt->roc_ids()) {
      // get the routing of the DATA path
      // assuming that the ECON-D is index 0
      auto [lo_erx, hi_erx] = tgt->getHardwareRocErxMappingDAQ().at(roc);
      erx_enable[0].set(lo_erx);
      erx_enable[0].set(hi_erx);

      // get the routing for the TRIG path
      auto [i_econ, erx_ls] = tgt->getHardwareRocErxMappingTRG().at(roc);
      for (int erx : erx_ls) {
        erx_enable[i_econ].set(erx);
      }
    }

    // now that we know which eRx should be enable for each ECON
    // let's configure the ECONs
    for (int iecon : tgt->econ_ids()) {
      pflib_log(info) << "ECON " << iecon << " eRx enable: " << erx_enable[iecon];

      // TODO: need a hard/soft reset?

      std::map<std::string, std::map<std::string, uint64_t>> parameters;
      for (int ierx{0}; ierx < 12; ierx++) {
        parameters["ERX"][std::to_string(ierx)+"_ENABLE"] = erx_enable[iecon].test(ierx);
      }
      parameters["ROCDAQCTRL"]["GLOBAL_ACTIVE_ERXS"] = erx_enable[iecon].to_ulong();
      parameters["FORMATTERBUFFER"]["GLOBAL_LINK_RESET_PATTERN"] = 0x526;
      uint64_t active_etx_bitmask = 0b1;
      if (iecon == 0) {
        // assuming that the ECON-D is index 0
        parameters["FORMATTERBUFFER"]["GLOBAL_IDLE_PATTERN"] = 0x1277cc;
      } else if (is_hcal) {
        active_etx_bitmask = 0b111;
      } else {
        active_etx_bitmask = 0b111111;
      }
      parameters["FORMATTERBUFFER"]["GLOBAL_ACTIVE_ETXS"] = active_etx_bitmask;

      // Hcal needs to invert the eTx output from the ECONs
      // ETX.nn_INVERT_DATA: 1
      // leave it as its default 0 for the Ecal
      uint64_t invert_etx_data = (is_hcal ? 1 : 0);
      for (int etx{0}; etx < 12; etx++) {
        parameters["ETX"][std::to_string(etx)+"_INVERT_DATA"] = invert_etx_data;
      }

      pflib::ECON& econ{tgt->econ(iecon)};

      // LOAD the necessary link configuration onto the chip
      econ.applyParameters(parameters);

      // put the ECON into RUNMODE
      int edgesel = (is_hcal ? 0 : 1);
      int invertfcmd = (is_hcal ? 1 : 0);
      econ.setRunMode(true, edgesel, invertfcmd);

      // put the ROCs into RUNMODE
      for (int iroc : tgt->roc_ids()) {
        tgt->roc(iroc).setRunMode(true);
      }

      // sleep a bit
      sleep(1);

      // TODO share econ_status

      // align bits and words between ECON and ROCs
      align_econ_rocs(tgt.get(), iecon, on_zcu);

      // align bits between ECON and lpGBT
      align_econ_lpgbt_bit(tgt.get(), iecon, false, is_hcal);

      // word alignment between ECON and lpGBT requires
      // ability to capture words from the lpGBT, only
      // possible for DATA path or TRIG path on ZCU
      if (iecon == 0 or on_zcu) {
        align_econ_lpgbt_word(tgt.get(), 0, false, (iecon == 0) ? 0x1277cc : 0x526, is_hcal);
      } else {
        pflib_log(warn) << "unable to word-align ECON " << iecon << " with its TRIG lpGBT";
      }
    }
  }

  return 0;
} catch (const pflib::Exception& e) {
  pflib_log(fatal) << "[" << e.name() << "]: " << e.message();
  return 126;
} catch (const std::runtime_error& e) {
  pflib_log(fatal) << e.what();
  return 127;
}
