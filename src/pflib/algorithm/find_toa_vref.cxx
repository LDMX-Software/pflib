#include "pflib/algorithm/find_toa_vref.h"

#include "pflib/utility/efficiency.h"

#include "pflib/DecodeAndBuffer.h"

/**
 * get the efficiency of the channel TOA values
 */

void toa_vref_scan(Target* tgt) {
  int nevents = pftool::readline_int("Number of events per toa_vref point? More is better for detecting outliers. ", 100);
  int toa_vref = pftool::readline_int("Lower limit for TOA_VREF (0-1023), but should be close to 0 ", 0);
  int vref_upper = pftool::readline_int("Upper limit for TOA_VREF (0-1023), but should be closer to 250 ", 250);
  int step_size = pftool::readline_int("Size of steps between TOA_VREF values ", 1);
  int offset = pftool::readline_int("Offset to increase predicted TOA_VREF to ensure no pedestals trigger TOA ", 10);
  std::string output_filepath = pftool::readline_path("toa_vref_scan", ".csv");

  auto roc = tgt->hcal().roc(pftool::state.iroc, pftool::state.type_version());


  // old code for when we wrote to CSV. but now we want to write to buffer
  pflib::DecodeAndWriteToCSV writer{
    output_filepath,
    [&](std::ofstream& f) {
      boost::json::object header;
      header["scan_type"] = "TOA_VREF scan";
      header["trigger"] = "PEDESTAL";
      header["nevents_per_point"] = nevents;
      header["offset"] = offset;
      f << "# " << boost::json::serialize(header) << "\n"
        << "TOA_VREF";
        // need a header for each channel toa, since we look at all of them
      for (int ch{0}; ch < 72; ch++) {
        f << "," << ch;
      }
      f << "\n";
    },
    [&](std::ofstream& f, const pflib::packing::SingleROCEventPacket &ep) {
      f << toa_vref;
      for (int ch{0}; ch < 72; ch++) {
        f << "," << ep.channel(ch).toa();
      }
      f << "\n";
    }
  };

  tgt->setup_run(1 /* dummy */, Target::DaqFormat::SIMPLEROC, 1 /* dummy */);

  std::array<int, 2> target; // store final target values for toa_vref
  double efficiency_data[vref_upper - toa_vref + 1][72]; //store TOA values here


  // I want to have a function to get the efficiencies and then use that function later on
  static std::array<double, 72> get_toa_efficiencies(const std::vector<pflib::packing::SingleROCEventPacket> &data) {
    std::array<double, 72> efficiencies;
    /// reserve a vector of the appropriate size to avoid repeating allocation time for all 72 channels
    std::vector<int> toas(data.size());
    for (int ch{0}; ch < 72; ch++) {
      for (std::size_t i{0}; i < toas.size(); i++) {
        toas[i] = data[i].channel(ch).toa();
      }
      // might need something here if it freaks out when toas.size() == 0
      if (toas.empty()) {
        pflib_log(warn) << "No TOA values found for channel " << ch
                        << ", setting efficiency to 0";
        efficiencies[ch] = 0.0;
        continue;
      }
      // count the number of non-zero TOA values and divide by total number of events
      efficiencies[ch] = std::count_if(toas.begin(), toas.end(), [](int toa) { return toa > 0; }) / static_cast<double>(toas.size());
    }
    return efficiencies;
  }

  pflib::DecodeAndBuffer buffer{nevents};
  int vref_lower = toa_vref;

  // Take a pedestal run on each parameter point
  // Toa_vref has an 10 b range, but we're likely not going to need all of that
  // since the pedestal variations are usually lower than ~300 adc.
  for (toa_vref = vref_lower; toa_vref <= vref_upper; toa_vref += step_size) {
    // set the parameters
    auto toa_vref_test = roc.testParameters()
      .add("REFERENCEVOLTAGE_0", "TOA_VREF", toa_vref)
      .add("REFERENCEVOLTAGE_1", "TOA_VREF", toa_vref)
      .apply();
    usleep(10); // make sure parameters are applied
    pflib_log(info) << "Running TOA_VREF = " << toa_vref;

    // daq run
    // tgt->daq_run("PEDESTAL", writer, nevents, pftool::state.daq_rate);
    tgt->daq_run("PEDESTAL", buffer, nevents, 100); // maybe this is correct...
    auto toa_efficiencies = get_toa_efficiencies(buffer.get_buffer());
    // need to save to another array?
    for (int ch{0}; ch < 72; ch++) {
      efficiency_data[toa_vref][ch] = toa_efficiencies[ch]; // line is toa_vref, column is efficiency
    }
  }
  // Now, we just have to find the target values
  // just get the highest toa_vref with a non-zero efficiency.
  for (int ch{0}; ch < 72; ch++) {
    target[ch] = 0; // initialize to 0
    for (int i{vref_upper - vref_lower}; i >= 0; i--) {
      if (efficiency_data[i][ch] > 0) {
        target[ch] = i + vref_lower; // add the offset
        pflib_log(info) << "Channel " << ch << " target TOA_VREF: " << target[ch];
        // break;
      }
    }
    if (target[ch] == 0) {
      pflib_log(warn) << "Channel " << ch << " has no non-zero efficiency, setting target to 0";
    }
  }
  // eventually want to get this to work for each link... since I want vref per link, not per channel.
}