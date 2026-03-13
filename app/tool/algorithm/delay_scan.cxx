#include <vector>
#include <array>

#include "delay_scan.h"

#include "non-linearity_scan.h"
#include "../daq_run.h"
#include "../tasks/set_delays.h"
#include "pflib/utility/efficiency.h"
#include "pflib/utility/string_format.h"

namespace pflib::algorithm {

  template <class EventPacket>
  std::map<std::string, std::map<std::string, uint64_t>> delay_scan(Target* tgt, ROC& roc)
  {

    static auto the_log_{::pflib::logging::get("delay_scan")};

    size_t n_events = pftool::readline_int("How many events per time point? ", 1);
    int channel = pftool::readline_int("Channel to pulse into? ", 61);
    int start_calib = pftool::readline_int("Starting CALIB : ", 3000);
    int end_calib = pftool::readline_int("Final CALIB : ", 3100);
    int steps = pftool::readline_int("CALIB step magnitude : ", 10);
    int start_delay_setting = pftool::readline_int("Starting delay settings (1-8): ", 1);
    int end_delay_setting = pftool::readline_int("Final delay settings (1-8)", 3); 
    int i_link = (channel / 36);
    std::vector<std::array<int, 3>> delay_list;
    std::vector<double> inl_list;
    std::vector<double> CALIBs;

    if (start_delay_setting > end_delay_setting) {
      pflib_log(warn) << "Your final delay setting is lower than your starting delay setting!";
      start_delay_setting = pftool::readline_int("Starting delay settings (1-8): ", 1);
      end_delay_setting = pftool::readline_int("Final delay settings (1-8)", 3); 
    }
    if ((not (0 < start_delay_setting < 9)) || (not (0 < end_delay_setting < 9))) {
      pflib_log(warn) << "At least one of your delay settings is out of the available range (1-8)!";
      start_delay_setting = pftool::readline_int("Starting delay settings (1-8): ", 1);
      end_delay_setting = pftool::readline_int("Final delay settings (1-8)", 3); 
    }

    for (double c = start_calib; c <= end_calib; c += steps)
    {
      CALIBs.push_back(c);
    }

    // Finding the bx corresponding to the ADC peak (preCC) ------

    auto channel_page = pflib::utility::string_format("CH_%d", channel);
    auto refvol_page = pflib::utility::string_format("REFERENCEVOLTAGE_%d", i_link);
    auto globalanalog_page = pflib::utility::string_format("GLOBALANALOG_%d", i_link);

    DecodeAndBuffer<EventPacket> buffer{1, 1};

    double central_charge_to_l1a{16};
    double charge_to_l1a{0};
    double start_bx{0};
    double n_bx{7}; // both this and start_bx should be based on input (or I should just hardcode the place of the peak [actually probably I should do that])
    int phase_strobe{0};
    int n_phase_strobe{16};
    std::vector<double> adc;
    std::vector<double> avg_adc;
    std::vector<double> peaks;
    std::vector<std::array<double, 2>> adc_to_bx;

    tgt->setup_run(1 /* dummy - not stored */, pftool::state.daq_format_mode,
                  1 /* dummy */);

    auto vref_test_param = roc.testParameters()
                              .add(refvol_page, "INTCTEST", 1)
                              .add(refvol_page, "CHOICE_CINJ", 0)
                              .add(channel_page, "HIGHRANGE", 1)
                              .add(channel_page, "LOWRANGE", 0) // preCC
                              .apply();                         // applying static parameters
    usleep(10);

    // calibration run (finding the bx corresponding to the peak)

    for (charge_to_l1a = central_charge_to_l1a + start_bx; charge_to_l1a < central_charge_to_l1a + start_bx + n_bx; charge_to_l1a++)
    { // moving through different bxs

      tgt->fc().fc_setup_calib(charge_to_l1a);
      pflib_log(info) << "charge_to_l1a = " << tgt->fc().fc_get_setup_calib();
      usleep(10);

      int calib = CALIBs[0];

      for (phase_strobe = 0; phase_strobe < n_phase_strobe; phase_strobe++)
      {
        auto phase_strobe_test_handle =
          roc.testParameters().add("TOP", "PHASE_STROBE", phase_strobe).add(refvol_page, "CALIB_2V5", calib).apply();
        pflib_log(info) << "TOP.PHASE_STROBE = " << phase_strobe;
        usleep(10); // make sure parameters are applied

        daq_run(tgt, "CHARGE", buffer, 1, 100);
        auto data = buffer.get_buffer();
        double data_adc = 0.;

        if constexpr (std::is_same_v<EventPacket,
                      pflib::packing::MultiSampleECONDEventPacket>) {
          data_adc = data[0].samples[data[0].i_soi].channel(channel, i_link).adc();
        } else if constexpr (std::is_same_v<EventPacket,
                            pflib::packing::SingleROCEventPacket>) {
          data_adc = data[0].channel(channel).adc();
        }
        
        std::array<double, 2> bx{data_adc, charge_to_l1a};
        adc_to_bx.push_back(bx);
      }
    }

    auto it = std::max_element(adc_to_bx.begin(), adc_to_bx.end(), [](const auto &a, const auto &b)
                              { return a[0] < b[0]; }); // finds the peak and corresponding bx

    charge_to_l1a = it->at(1); // bx corresponding to the peak

    // ------

    double optimal_bx = charge_to_l1a;
    
    for (int i = start_delay_setting; i <= end_delay_setting; i++)
    {
      for (int j = start_delay_setting; j <= end_delay_setting; j++)
      {
        for (int k = start_delay_setting; k <= end_delay_setting; k++)
        {
          pflib_log(info) << "Trying delays : " << i << ", " << j << ", " << k;
          std::array<int, 3> delays{i, j, k};
          delay_list.push_back({i, j, k});
          std::vector<double> nl_vector{0.,0.,0.};
          if (pftool::state.daq_format_mode == Target::DaqFormat::SIMPLEROC) {
            nl_vector = nl_scan<pflib::packing::SingleROCEventPacket>(tgt, roc, n_events, channel, i_link, delays, CALIBs, optimal_bx);
          } else if (pftool::state.daq_format_mode == Target::DaqFormat::ECOND_SW_HEADERS) {
            nl_vector = nl_scan<pflib::packing::MultiSampleECONDEventPacket>(tgt, roc, n_events, channel, i_link, delays, CALIBs, optimal_bx);
          }
          inl_list.push_back(nl_array[0]);
        }
      }
    }
    for (double value : inl_list)
    {
      pflib_log(info) << "All max INLs :" << value;
    }
    // minimize the inl
    auto min_value = std::min_element(inl_list.begin(), inl_list.end());
    double min_inl = *min_value;
    auto min_inl_it = std::find(inl_list.begin(), inl_list.end(), min_inl);
    size_t min_inl_index;
    if (min_inl_it != inl_list.end())
    {
      min_inl_index = std::distance(inl_list.begin(), min_inl_it);
    }
    std::array<int, 3> optimal_delay = delay_list[min_inl_index];
    pflib_log(info) << "Optimal delay settings found: " << optimal_delay[0] << ", " << optimal_delay[1] << ", " << optimal_delay[2] << ";\n"
                    << "Minimum INL: " << min_inl;
    

    std::map<std::string, std::map<std::string, uint64_t>> delay_settings;
    for (int i_link{0}; i_link < 2; i_link++) {
      globalanalog_page = pflib::utility::string_format("GLOBALANALOG_%d", i_link);
      delay_settings[globalanalog_page]["DELAY65"] = optimal_delay[0];
      delay_settings[globalanalog_page]["DELAY87"] = optimal_delay[1];
      delay_settings[globalanalog_page]["DELAY9"] = optimal_delay[2];
    }
    
    return delay_settings;
  }

  template std::map<std::string, std::map<std::string, uint64_t>> delay_scan<pflib::packing::SingleROCEventPacket>(Target* tgt, ROC& roc);
  template std::map<std::string, std::map<std::string, uint64_t>> delay_scan<pflib::packing::MultiSampleECONDEventPacket>(Target* tgt, ROC& roc);
}
