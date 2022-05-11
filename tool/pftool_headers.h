
#ifndef PFTOOL_HEADERS_H
#define PFTOOL_HEADERS_H

#include "pflib/decoding/SuperPacket.h"
#include <iostream>
#include <vector>
struct HeaderStatus {
    public:
        int n_good_bxheaders = 0;
        int n_bad_bxheaders = 0;
        int n_good_idles = 0;
        int n_bad_idles = 0;
        int link;
        double percent_bad_headers() const;
        double percent_bad_idles() const;
        HeaderStatus(int ilink) : link(ilink) {}
        void report() const {
            std::cout << "Link: " << link << "\n";
            std::cout << "Num bad headers: " << n_bad_bxheaders
                      << ", ratio: " << percent_bad_headers()
                      << std::endl;
            std::cout << "Num bad idles: " << n_bad_idles
                      << ", ratio: " << percent_bad_idles()
                      << std::endl;
        }
        void update(const pflib::decoding::RocPacket packet);
};
struct HeaderCheckResults {
    int num_links;
    int num_active_links;
    std::vector<HeaderStatus> res;
    HeaderCheckResults(const std::vector<int>& active_links )
    {
        for (auto link : active_links) {
            res.push_back(link);
            std::cout << "Adding active link: " << link << std::endl;
        }
    }
    void report() const {
        for (auto status : res) {
            status.report();
        }
    }
    void add_event(const pflib::decoding::SuperPacket event, const int nsamples);
    bool is_acceptable (const double threshold) const;
};



#endif /* PFTOOL_HEADERS_H */
