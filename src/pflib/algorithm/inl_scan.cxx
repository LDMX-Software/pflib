#include inl_scan.h

#include <vector>
#include <cmath>

#include "pflib/utility/efficiency.h"
#include "pflib/utility/string_format.h"

namespace pflib::algorithm { 

    double mean(const std::vector<double>& adc){

        double sum = 0.0;
        for (double measurement : adc) {
            sum += measurement;
        }

        return adc.empty() ? 0 : sum / adc.size();
    }

    /*double stdev(const std::vector<double>& adc, double& mean){
        
        int size = adc.size();
        double stdeviation{0.0};

        for (int i = 0; i < size; i++){
            stdeviation += pow(adc[i] - mean, 2);
        }
        
        return sqrt(stdeviation / size);
    }*/

    void linear_fit(const std::vector<double>& x, // chatGPT code for now
                    const std::vector<double>& y,
                    double& m,
                    double& b)
    {
        int n = x.size();
        double sum_x = 0, sum_y = 0, sum_xy = 0, sum_x2 = 0;

        for (int i = 0; i < n; i++) {
            sum_x  += x[i];
            sum_y  += y[i];
            sum_xy += x[i] * y[i];
            sum_x2 += x[i] * x[i];
        }

        m = (n * sum_xy - sum_x * sum_y) /
            (n * sum_x2 - sum_x * sum_x);

        b = (sum_y - m * sum_x) / n;
    }

    double inl(double& m, double b&, std::vector<double> calibs, std::vector<double> peaks){
    
        std::vector<double> y = m * calibs + b;
        std::vector<double> inl = std::abs(y - peaks);

        return double inl = std::max_element(inl.begin(), inl.end());
    }

    template <class EventPacket>
    double inl_scan(Target* tgt, ROC& roc, size_t& n_events, auto& channel_page, auto& refvol_page, auto& globalanalog_page,
                            auto& buffer, std::array<int,3> delays) {

        // The results array will contain the largest INL and the spread (sigma/RMS)

        // The delay parameters of the board should be set up in the delay scan
        // tgt, roc, buffer, globalanalog_page
        // turns out it also requires channel_page and refvol_page?
        
        static auto the_log_{::pflib::logging::get("INL_scan")};

        int central_charge_to_l1a;
        int charge_to_l1a{2};
        int phase_strobe{0};
        int n_phase_strobe{16};
        std::vector<double> adc;
        std::vector<double> avg_adc;
        //std::vector<double> stdev_array; 
        std::vector<double> peaks;

        tgt->setup_run(1 /* dummy - not stored */, pftool::state.daq_format_mode,
                 1 /* dummy */);
            
        // should set up bx = 2

        central_charge_to_l1a = tgt->fc().fc_get_setup_calib();
        tgt->fc().fc_setup_calib(charge_to_l1a);
        pflib_log(info) << "charge_to_l1a = " << tgt->fc().fc_get_setup_calib();

        auto vref_test_param = roc.testParameters()
                            .add(globalanalog_page, "DELAY65", delays[0])
                            .add(globalanalog_page, "DELAY87", delays[1])
                            .add(globalanalog_page, "DELAY9", delays[2])
                            .add(channel_page, "HIGHRANGE", 0)
                            .add(channel_page, "LOWRANGE", 1) // should be preCC actually but idk how to set that up rn
                            .apply();  // applying static parameters
        usleep(10);

        // daq run
        // CALIBs can be set in this script temporarily
        std::vector CALIBs = [3000, 3001, 3002, 3003, 3004, 3005];
        for (int calib : CALIBs) {
            pflib_log(info) << "CALIB = " << << calib;
            for (phase_strobe = 0; phase_strobe < n_phase_strobe; phase_strobe++) {
                auto phase_strobe_test_handle =
                    roc.testParameters().add("TOP", "PHASE_STROBE", phase_strobe).add(refvol_page, "CALIB_2V5", calib).apply();
                pflib_log(info) << "TOP.PHASE_STROBE = " << phase_strobe;
                usleep(10);  // make sure parameters are applied

                daq_run(tgt, "CHARGE", buffer, n_events, 100); 
                auto data = buffer.get_buffer();
                
                adc = data.adc();
                double avg = mean(adc);
                avg_adc.push_back(avg);
            }
            peaks.push_back(std::max_element(avg_adc.begin(), avg_adc.end()));
        }

        double m, b;
        linear_fit(CALIBs, peaks, m, b);
        double inl = inl(m, b, CALIBs, peaks);

        return inl;
    }
}