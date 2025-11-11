#include "set_toa.h"

#include "../daq_run.h"
#include "pflib/Exception.h"
#include "pflib/utility/string_format.h"

ENABLE_LOGGING();

void set_toa(Target* tgt, pflib::ROC& roc, int channel) {
  int link = (channel / 36);
  auto refvol_page = pflib::utility::string_format("REFERENCEVOLTAGE_%d", link);
  auto channel_page = pflib::utility::string_format("CH_%d", channel);
  int nevents = 30;
  double toa_eff{2};
  // in the calibration documentation, it is suggested to send a "small" charge
  // injection. Here I used 200 but there is maybe a better value.
  int calib = 200;
  auto test_param_handle = roc.testParameters()
                               .add(refvol_page, "CALIB", calib)
                               .add(refvol_page, "INTCTEST", 1)
                               .add(channel_page, "LOWRANGE", 1)
                               .apply();

  pflib_log(info) << "finding the TOA threshold!";
  // This class doesn't write to csv. When we just would like to
  // use the data for setting params.
  DecodeAndBuffer buffer(nevents);

  tgt->setup_run(1 /* dummy - not stored */, Target::DaqFormat::SIMPLEROC,
                 1 /* dummy */);
  for (int toa_vref = 100; toa_vref < 250; toa_vref++) {
    auto test_handle =
        roc.testParameters().add(refvol_page, "TOA_VREF", toa_vref).apply();
    daq_run(tgt, "CHARGE", buffer, nevents, pftool::state.daq_rate);
    std::vector<double> toa_data;
    for (const pflib::packing::SingleROCEventPacket& ep : buffer.get_buffer()) {
      auto toa = ep.channel(channel).toa();
      if (toa > 0) {
        toa_data.push_back(toa);
      }
    }
    toa_eff = static_cast<double>(toa_data.size()) / nevents;
    if (toa_eff == 1) {
      roc.applyParameter(refvol_page, "TOA_VREF", toa_vref);
      pflib_log(info) << "the TOA threshold is set to " << toa_vref;
      return;
    }
    toa_vref += 1;
  }
  PFEXCEPTION_RAISE("NOTOA", "No TOA threshold was found for channel " +
                                 std::to_string(channel) + "!");
}
