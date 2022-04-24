
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
        void update(const pflib::decoding::RocPacket packet);
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
    void add_event(const pflib::decoding::SuperPacket event, const int nsamples);
};

#endif /* PFTOOL_HEADERS_H */
