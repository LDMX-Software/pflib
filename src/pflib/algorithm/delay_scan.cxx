#include <vector>

#include "delay_scan.h"

#include "inl_scan.h"
#include "pflib/utility/efficiency.h"
#include "pflib/utility/string_format.h"



namespace pflib::algorithm { 

    template <class EventPacket>
    void delay_scan(Target* tgt) {

        static auto the_log_{::pflib::logging::get("delay_scan")};

        int n_events = pftool::readline_int("How many events per time point? ", 1);
        int channel = pftool::readline_int("Channel to pulse into? ", 61);
        int link = (channel / 36);
        //int i_ch = channel % 36;  // 0–35
        auto channel_page = pflib::utility::string_format("CH_%d", channel);
        auto refvol_page = pflib::utility::string_format("REFERENCEVOLTAGE_%d", link);
        pflib::ROC roc{tgt->roc(pftool::state.iroc)};
        DecodeAndBuffer<EventPacket> buffer{n_events, 1}; // nevents, nr of links active

        std::vector delay_list;
        std::vector inl_list;
        std::vector<int,3> delays;

        for (int i = 1; i < 8; i++){ // I hate this, but I have no better ideas atm
            for (int j = 1; j < 8; j++){
                for (int k = 1; k < 8; k ++){ 
                    auto globalanalog_page = pftool::utility::string_format("GLOBALANALOG_%d", link);
                    delays = [i,j,k];
                    delay_list.push_back(delays);
                    inl_list.push_back(inl_scan(tgt, roc, n_events, channel_page, refvol_page, globalanalog_page, buffer, delays));
                }
            }
        }

        // minimize the inl
        auto min_inl = std::min_element(inl_list.begin(), inl_list.end());
        auto min_inl_index = std::find(inl_list.begin(), inl_list.end(), min_inl);
        auto optimal_delay = delay_list[min_inl_index];
        pflib_log(info) << "Optimal delay settings found: " << optimal_delay;
    }
}