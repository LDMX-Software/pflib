
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
        void update(const pflib::decoding::LinkPacket packet);
};
struct HeaderCheckResults {
    int num_links;
    int num_active_links;
    std::vector<HeaderStatus> res;
    HeaderCheckResults(int inum_links, int inum_active_links) :
        num_links{inum_links},
        num_active_links{inum_active_links}
    {
        for (int link{0}; link < num_active_links; ++link) {
            res.push_back(link);
        }
    }
    void report() const {
        for (auto status : res) {
            status.report();
        }
    }
    void add_event(const pflib::decoding::SuperPacket event, const int nsamples);
    bool is_acceptable (const double threshold) {
        for (auto status : res) {
            std::cout << "Testing link " << status.link << "... ";
            if (status.percent_bad_headers() > threshold )
                 {
                     std::cout << "bad headers!,  "
                               << status.percent_bad_headers() * 100
                               << " %" << std::endl;
                     return false;
                 } else if (status.percent_bad_idles() > threshold) {
                     std::cout << "bad idles!,  "
                               << status.percent_bad_idles() * 100
                               << " %" << std::endl;
                     return false;
            }
            std::cout << " ok!" << std::endl;
        }
        return true;
    };
};



#endif /* PFTOOL_HEADERS_H */
