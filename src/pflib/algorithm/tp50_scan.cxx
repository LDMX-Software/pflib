#include "pflib/algorithm/tp50_scan.h"

#include <algorithm>
#include <cmath>
#include <vector>

#include "pflib/DecodeAndBuffer.h"
#include "pflib/utility/string_format.h" 

namespace pflib::algorithm {

	std::array<int, 2> tp50_scan(Target* tgt, ROC roc, std::array<int, 72> calib) {

		int nevents = 100;

		std::array<int, 2> link_vref_list; // results array, which will hold the best vref for each link

		for (channel = 0; channel < 72; channel++) {

			auto channel_page = pflib::utility::string_format("CH_%d", channel);
			int link = (channel / 36);
			auto refvol_page = pflib::utility::string_format("REFERENCEVOLTAGE_%d", link);
			auto test_param_handle =
			roc.testParameters()
			.add(refvol_page, "INTCTEST", 1)
			.add(refvol_page, "CHOICE_CINJ", 1)
			.add(refvol_page, "CALIB", calib)
			.add(channel_page, "HIGHRANGE", 1)
			.apply();

			tgt->setup_run(1, Target::DaqFormat::SIMPLEROC, 1);
			DecodeAndBuffer buffer{n_events};

			std::vector<double> tot_eff_list;
			std::vector<int> vref_list = {0,600};
			int vref_value{100000};
			double tot_eff{0};
			double tol{0.1};
			int max_its = 25;

			while (true) {
				// Bisectional search
				if (!tot_eff_list.empty()) {
					if (tot_eff_list.back() > 0.5) {
						vref_list.front() = vref_value;
					} else {
						vref_list.back() = vref_value;
					}
					vref_value = (vref_list.back() - vref_list.front())/2 + vref_list.front();
				} else {
					vref_value = (vref_list.back() - vref_list.front())/2;
				}
				
				auto vref_test_param = roc.testParameters().add(refvol_page, "TOT_VREF", vref_value).apply(); // applying relevant parameters
				usleep(10);
				
				// daq run
				tgt->daq_run("CHARGE", buffer, nevents, 100);
				auto data = buffer.get_buffer();

				std::vector<double> tot_list
				for (pflib::packing::SingleROCEventPacket ep : data) {
					auto tot = ep.channel(channel).tot();
					if (tot > 0) {
					tot_list.push_back(tot);
					}
				}

				tot_eff = static_cast<double>(tot_list.size())/nevents; // calculating tot efficiency

				if (std::abs(tot_eff - 0.5) < tol) {

					std::vector<int> vref_buffer;
					std::vector<double> diff_buffer;

					// finding tot_eff for 5 points behind and in front of the found vref
					for (vref = vref_value, (vref > vref_value - 5) && (vref > 0), vref--) {
						auto vref_test_param = roc.testParameters().add(refvol_page, "TOT_VREF", vref).apply();
						usleep(10);
						
						// daq run
						tgt->daq_run("CHARGE", buffer, nevents, 100);
						auto data = buffer.get_buffer();

						std::vector<double> tot_list
						for (pflib::packing::SingleROCEventPacket ep : data) {
							auto tot = ep.channel(channel).tot();
							if (tot > 0) {
							tot_list.push_back(tot);
							}
						}

						tot_eff = static_cast<double>(tot_list.size())/nevents;
						diff = std::abs(tot_eff - 0.5)

						vref_buffer.push_back(vref);
						diff_buffer.push_back(diff);
					}

					for (vref = vref_value; (vref < vref_value + 5) && (vref < 600); vref++) {
						auto vref_test_param = roc.testParameters().add(refvol_page, "TOT_VREF", vref).apply();
						usleep(10);
						
						// daq run
						tgt->daq_run("CHARGE", buffer, nevents, 100);
						auto data = buffer.get_buffer();

						std::vector<double> tot_list
						for (pflib::packing::SingleROCEventPacket ep : data) {
							auto tot = ep.channel(channel).tot();
							if (tot > 0) {
							tot_list.push_back(tot);
							}
						}

						tot_eff = static_cast<double>(tot_list.size())/nevents;
						diff = std::abs(tot_eff - 0.5)

						vref_buffer.push_back(vref);
						diff_buffer.push_back(diff);
					}

					int mindiff = *std::min_element(diff_buffer.begin(), diff_buffer.end());
					auto it = std::find(diff_buffer.begin(), diff_buffer.end(), mindiff);

					if (it != diff_buffer.end()) { // if it is not found, it assumes arr.end() (which is, confusingly, not the last element)
						int index = std::distance(diff_buffer.begin(), it);
					}

					if (std::isnan(link_vref_list[link])) {
						link_vref_list[link] = vref_buffer[index];
						break;
					} else {
						if (vref_buffer[index] <= link_vref_list[link]) {
							break;
						} else {
							link_vref_list[link] = vref_buffer[index];
							break;
						}
					}
				}

				// if the vref doesn't move from 0 or 600, tp50 was not found
				if (vref_value == 0 || vref_value == 600) {
					pflib_log(info) << "Not tp_50 was found! Channel " << channel;
					break;
				}

				tot_eff_list.push_back(tot_eff); // vref with tot_eff was found, but tot_eff - 0.5 !< tol

				// if the tot_eff is consistently above tolerance, we limit the iterations
				if (tot_eff_list.size() > max_its) {
					pflib_log(info) << "Ended after " << max_its << " iterations!" << '\n'
							<< "Final vref is : " << vref_value
							<< " with tot_eff = " << tot_eff
							<< " for channel : " << channel
					break;
				}
			}
		}
		return link_vref_list;
	}
}