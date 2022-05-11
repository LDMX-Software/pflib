
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
            printf("%4d %10d %10d %.2f | %10d %10d %.2f\n", link,
                   n_good_bxheaders, n_bad_bxheaders, percent_bad_headers(),
                   n_good_idles, n_bad_idles, percent_bad_idles());
        }
        void update(const pflib::decoding::RocPacket packet);
        HeaderStatus(const HeaderStatus&) = default;
        HeaderStatus& operator=(const HeaderStatus&) = default;
        HeaderStatus() : link{-1} {}
};
struct HeaderCheckResults {
    std::vector<HeaderStatus> res;
    HeaderCheckResults(const std::vector<int>& active_links )
    {
        for (auto link : active_links) {
            res.push_back(link);
            std::cout << "Adding active link: " << link << std::endl;
        }
    }
    void report() const {
        printf("     %26s | %26s\n","BX Headers","Idles");
        printf("Link %10s %10s %4s | %10s %10s %4s\n","Good","Bad","B/T","Good","Bad","B/T");
        for (auto status : res) {
            status.report();
        }
    }
    void add_event(const pflib::decoding::SuperPacket event, const int nsamples);
    bool is_acceptable (const double threshold) const;
};



#endif /* PFTOOL_HEADERS_H */
