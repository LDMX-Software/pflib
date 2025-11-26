#include "set_toa.h"

#include "../daq_run.h"
#include "pflib/Exception.h"
#include "pflib/utility/string_format.h"

ENABLE_LOGGING();

template <class EventPacket>
void set_toa_runs(Target* tgt, pflib::ROC& roc, size_t nevents,
                  auto refvol_page, int& channel) {
  double toa_eff{2};
  int link = (channel / 36);
  int i_ch = channel % 36;  // 0–35

  DecodeAndBuffer<EventPacket> buffer{nevents};

  tgt->setup_run(1 /* dummy - not stored */, pftool::state.daq_format_mode,
                 1 /* dummy */);
  for (int toa_vref = 100; toa_vref < 250; toa_vref++) {
    auto test_handle =
        roc.testParameters().add(refvol_page, "TOA_VREF", toa_vref).apply();
    daq_run(tgt, "CHARGE", buffer, nevents, pftool::state.daq_rate);
    std::vector<double> toa_data;

    if constexpr (std::is_same_v<EventPacket, pflib::packing::SingleROCEventPacket>) {
      for (const EventPacket& ep : buffer.get_buffer()) {
        auto toa = ep.channel(channel).toa();
        if (toa > 0) {
          toa_data.push_back(toa);
        }
      }
    } else if constexpr (std::is_same_v<EventPacket, pflib::packing::MultiSampleECONDEventPacket>) {
      for (const EventPacket& ep : buffer.get_buffer()) {
        auto toa = ep.samples[ep.i_soi].channel(link, i_ch).adc();
        if (toa > 0) {
          toa_data.push_back(toa);
        }
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

void set_toa(Target* tgt, pflib::ROC& roc, int channel) {
  int link = (channel / 36);
  auto refvol_page = pflib::utility::string_format("REFERENCEVOLTAGE_%d", link);
  auto channel_page = pflib::utility::string_format("CH_%d", channel);
  int nevents = 30;
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

  // call helper function to conuduct the scan
  if (pftool::state.daq_format_mode == Target::DaqFormat::SIMPLEROC) {
    set_toa_runs<pflib::packing::SingleROCEventPacket>(tgt, roc, nevents,
                                                       refvol_page, channel);
  } else if (pftool::state.daq_format_mode ==
             Target::DaqFormat::ECOND_SW_HEADERS) {
    set_toa_runs<pflib::packing::MultiSampleECONDEventPacket>(
        tgt, roc, nevents, refvol_page, channel);
  }
  // DecodeAndBuffer buffer(nevents);

  // tgt->setup_run(1 /* dummy - not stored */, Target::DaqFormat::SIMPLEROC,
  //                1 /* dummy */);
  // for (int toa_vref = 100; toa_vref < 250; toa_vref++) {
  //   auto test_handle =
  //       roc.testParameters().add(refvol_page, "TOA_VREF", toa_vref).apply();
  //   daq_run(tgt, "CHARGE", buffer, nevents, pftool::state.daq_rate);
  //   std::vector<double> toa_data;
  //   for (const pflib::packing::SingleROCEventPacket& ep :
  //   buffer.get_buffer()) {
  //     auto toa = ep.channel(channel).toa();
  //     if (toa > 0) {
  //       toa_data.push_back(toa);
  //     }
  //   }
  //   toa_eff = static_cast<double>(toa_data.size()) / nevents;
  //   if (toa_eff == 1) {
  //     roc.applyParameter(refvol_page, "TOA_VREF", toa_vref);
  //     pflib_log(info) << "the TOA threshold is set to " << toa_vref;
  //     return;
  //   }
  //   toa_vref += 1;
  // }
  // PFEXCEPTION_RAISE("NOTOA", "No TOA threshold was found for channel " +
  //                                std::to_string(channel) + "!");
}
