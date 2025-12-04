#include <vector>

#include "pflib/DecodeAndBuffer.h"
#include "pflib/utility/efficiency.h"
#include "pflib/utility/string_format.h"

namespace pflib::algorithm {
    
    double eff_scan(Target* tgt, ROC roc, int channel, int vref_value, int nevents, auto refvol_page, auto buffer) { // will the script understand auto refvol_page and auto buffer?
        auto vref_test_param = roc.testParameters()
                                 .add(refvol_page, "TOT_VREF", vref_value)
                                 .apply();  // applying relevant parameters
        usleep(10);

        // daq run
        tgt->daq_run("CHARGE", buffer, nevents, 100);

        auto data = buffer.get_buffer();
        std::vector<double> tot_list;
        for (std::size_t i{0}; i < data.size(); i++) {
            auto tot = data[i].channel(channel).tot();
            if (tot > 0) {
            tot_list.push_back(tot);
            }
        }

        double tot_eff = static_cast<double>(tot_list.size()) /
                    nevents;  // calculating tot efficiency
        return tot_eff;
    }

    int global_vref_scan(Target* tgt, ROC roc, int channel, int nevents, auto refvol_page, auto buffer) {
        static auto the_log_{::pflib::logging::get("tp50_scan")};
        std::vector<double> tot_eff_list;
        std::vector<int> vref_list = {0, 600};
        int vref_value{100000};
        double tol{0.1};
        int max_its = 40;

        while (true) {
            // Bisectional search
            if (!tot_eff_list.empty()) {
                if (tot_eff_list.back() > 0.5) {
                vref_list.front() = vref_value;
                } else {
                vref_list.back() = vref_value;
                }
                vref_value = (vref_list.back() + vref_list.front()) / 2;
            } else {
                vref_value = (vref_list.back() + vref_list.front()) / 2;
            }

            double efficiency = eff_scan(tgt, roc, channel, vref_value, nevents, refvol_page, buffer);
            
            if (std::abs(efficiency - 0.5) < tol) {
                pflib_log(info) << "Efficiency within tolerance!";
                break;
            }

            if (vref_value == 0 || vref_value == 600) {
                pflib_log(info) << "No tp_50 was found! Channel " << channel;
                vref_value = -1;
                break;
            }

            tot_eff_list.push_back(
                efficiency);  // vref with tot_eff was found, but tot_eff - 0.5 !< tol

            // if the tot_eff is consistently above tolerance, we limit the iterations
            if (tot_eff_list.size() > max_its) {
                pflib_log(info) << "Ended after " << max_its << " iterations!" << '\n'
                                << "Final vref is : " << vref_value
                                << " with tot_eff = " << efficiency
                                << " for channel : " << channel;
                vref_value = -1;
                break;
            }
        }
        return vref_value;
    }

    int local_vref_scan(Target* tgt, ROC roc, int channel, int vref_value, int nevents, auto refvol_page, auto buffer) {

        std::vector<int> vref_buffer;
        std::vector<double> diff_buffer;

        // finding tot_eff for 5 points behind and in front of the found vref
        for (int vref = vref_value; (vref > vref_value - 5) && (vref > 0);
             vref--) {
        
            double efficiency = eff_scan(tgt, roc, channel, vref, nevents, refvol_page, buffer);
            auto diff = std::abs(efficiency - 0.5);

            vref_buffer.push_back(vref);
            diff_buffer.push_back(diff);
        }

        for (int vref = vref_value; (vref < vref_value + 5) && (vref < 600);
             vref++) {
    
            double efficiency = eff_scan(tgt, roc, channel, vref, nevents, refvol_page, buffer);
            auto diff = std::abs(efficiency - 0.5);

            vref_buffer.push_back(vref);
            diff_buffer.push_back(diff);
        }

        int mindiff = *std::min_element(diff_buffer.begin(), diff_buffer.end()); // finding the efficiency closest to 0.5
        auto it = std::find(diff_buffer.begin(), diff_buffer.end(), mindiff);
        int index = -1;

        if (it !=
            diff_buffer
                .end()) {  // if it is not found, it assumes arr.end() (which
                           // is, confusingly, not the last element)
            index = std::distance(diff_buffer.begin(), it);
        }

        return vref_buffer[index];
    }

    std::array<int,2> tp50_scan(Target* tgt, ROC roc, std::array<int,72> calib) {
        
        static auto the_log_{::pflib::logging::get("tp50_scan")};
        int nevents = 100;

        std::array<int, 2> link_vref_list = {
            -1, -1};  // results array, which will hold the best vref for each link

        for (int channel = 0; channel < 72; channel++) {
            pflib_log(info) << "Scanning channel " << channel << " for tp50, "
                            << "using CALIB " << calib[channel];

            int link = channel/36;
            auto channel_page = pflib::utility::string_format("CH_%d", channel);
            auto refvol_page =
            pflib::utility::string_format("REFERENCEVOLTAGE_%d", link);
            auto test_param_handle = roc.testParameters()
                                        .add(refvol_page, "INTCTEST", 1)
                                        .add(refvol_page, "CHOICE_CINJ", 1)
                                        .add(refvol_page, "CALIB", calib[channel])
                                        .add(channel_page, "HIGHRANGE", 1)
                                        .apply();

            tgt->setup_run(1, Target::DaqFormat::SIMPLEROC, 1);
            DecodeAndBuffer buffer{nevents};

            int vref{0};

            if (link_vref_list[link] == -1) {
		pflib_log(info) << "Scanning first channel in link: " << link;
                vref = global_vref_scan(tgt, roc, channel, nevents, refvol_page, buffer);
                link_vref_list[link] = vref;
                continue;
            }

            double channel_eff = eff_scan(tgt, roc, channel, vref, nevents, refvol_page, buffer);

            if (channel_eff < 0.5) {
		pflib_log(info) << "Channel accounted for by previous vref!";
                continue;
            }

            else if (channel_eff < 0.6) {
		pflib_log(info) << "Scanning vref's local neighbourhood for a more suitable vref...";
                int channel_vref = local_vref_scan(tgt, roc, channel, vref, nevents, refvol_page, buffer);
                double new_channel_eff = eff_scan(tgt, roc, channel, channel_vref, nevents, refvol_page, buffer);
                if (new_channel_eff < 0.5) {
		    pflib_log(info) << "More suitable vref found!";
                    link_vref_list[link] = channel_vref;
                    continue;
                }
                else {
                    int channel_vref = global_vref_scan(tgt, roc, channel, nevents, refvol_page, buffer);
                    if (channel_vref != -1) {
			pflib_log(info) << "More suitable vref found after global scan!";
                        link_vref_list[link] = channel_vref;
                        continue;
                    }
                    else {
                        pflib_log(info) << "No suitable vref found for channel: " << channel;
                        continue;
                    }
                }
            }

            else {
		pflib_log(info) << "Channel not accounted for, initializing global vref scan...";
                int channel_vref = global_vref_scan(tgt, roc, channel, nevents, refvol_page, buffer);
                if (channel_vref != -1) {
		    pflib_log(info) << "Suitable vref found!";
                    link_vref_list[link] = channel_vref;
                    continue;
                }
                else {
                    pflib_log(info) << "No suitable vref found for channel: " << channel;
                    continue;
                }
            }
        }
        return link_vref_list;
    }
}
