
#include "pftool_headers.h"


void HeaderStatus::update(const pflib::decoding::RocPacket packet)
{
    if (packet.length() > 2) {
        if (packet.good_bxheader()) {
            n_good_bxheaders++;
        } else  {
            n_bad_bxheaders++;
        }
        if (packet.good_idle()) {
            n_good_idles++;
        } else {
            n_bad_idles++;
        }
    }
}
