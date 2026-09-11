#include "inv_vref_scan.h"

#include <nlohmann/json.hpp>

#include "../daq_run.h"

ENABLE_LOGGING();

void inv_vref_scan(Target* tgt) {
  int nevents = pftool::readline_int("Number of events per point: ", 1);
  int inv_vref = 0;

  std::string output_filepath = pftool::readline_path("inv_vref_scan", ".csv");
  int ch_link0 = pftool::readline_int("Channel to scan on link 0", 17);
  int ch_link1 = pftool::readline_int("Channel to scan on link 1", 51);
  std::array<int, 2> channels = {ch_link0, ch_link1};

  auto& roc = tgt->roc(pftool::state.iroc);
  // TODO 348
  int link = 0;
  int i_ch = 0;
  int n_links = 2 * tgt->nrocs();
  DecodeAndWriteToCSV writer{
      output_filepath,
      [&](std::ofstream& f) {
        nlohmann::json header;
        header["scan_type"] = "CH_#.INV_VREF sweep";
        header["trigger"] = "PEDESTAL";
        header["nevents_per_point"] = nevents;
        f << "# " << header << "\n"
          << "INV_VREF";
        for (int ch : channels) {
          f << ',' << ch;
        }
        f << '\n';
      },
      [&](std::ofstream& f,
          const pflib::packing::MultiSampleECONDEventPacket& ep) {
        f << inv_vref;
        for (int ch : channels) {
          link = (ch / 36);
          i_ch = ch % 36;
          f << ',' << ep.samples[ep.i_soi].channel(link, i_ch).adc();
        }
        f << '\n';
      },
      n_links};

  tgt->setup_run(1, Target::DaqFormat::ECOND_SW_HEADERS, 1);

  // increment inv_vref in increments of 20. 10 bit value but only scanning to
  // 600
  for (inv_vref = 0; inv_vref <= 600; inv_vref += 20) {
    pflib_log(info) << "Running INV_VREF = " << inv_vref;
    // set inv_vref simultaneously for both links
    auto test_param = roc.testParameters()
                          .add("REFERENCEVOLTAGE_0", "INV_VREF", inv_vref)
                          .add("REFERENCEVOLTAGE_1", "INV_VREF", inv_vref)
                          .apply();
    // store current scan state in header for writer access
    daq_run(tgt, "PEDESTAL", writer, nevents, pftool::state.daq_rate);
  }
}
