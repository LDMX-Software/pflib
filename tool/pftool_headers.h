
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
};

#endif /* PFTOOL_HEADERS_H */
