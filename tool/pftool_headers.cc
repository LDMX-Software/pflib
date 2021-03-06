
#include "pftool_headers.h"


double HeaderStatus::percent_bad_headers() const
{
    const auto num_headers {n_good_bxheaders + n_bad_bxheaders};
    if (num_headers > 0) {
        return {static_cast<double>(n_bad_bxheaders) / num_headers };
    }
    return -1;
}
double HeaderStatus::percent_bad_idles() const
{
    const auto num_idles {n_good_idles + n_bad_idles};
    if (num_idles > 0) {
        return {static_cast<double>(n_bad_idles)/ num_idles};
    }
    return -1;
}
void HeaderCheckResults::add_event(const pflib::decoding::SuperPacket event, const int nsamples)
{
    for (int sample{0}; sample < nsamples; ++sample) {
        for (int link{0}; link < num_active_links; ++link) {
            const auto packet {event.sample(sample).link(link)};
            auto& status {res[link]};
            status.update(packet);
        }
    }
}

void HeaderStatus::update(const pflib::decoding::LinkPacket packet)
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
