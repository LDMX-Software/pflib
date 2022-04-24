
#include "pftool_headers.h"


void HeaderCheckResults::add_event(const pflib::decoding::SuperPacket event, const int nsamples)
{
    for (int sample{0}; sample < nsamples; ++sample) {
        for (int link{0}; link < num_active_links; ++link) {
            const auto packet {event.sample(sample).roc(link)};
            auto& status {res[link]};
            status.update(packet);
        }
    }
}

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
