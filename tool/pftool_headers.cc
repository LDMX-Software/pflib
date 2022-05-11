
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
        for (auto& status: res) {
            const auto packet {event.sample(sample).roc(status.link)};
            if (verbose && status.link == 0 && sample == 0) {
                std::cout << std::hex;
                std::cout << "Debugging link 0, sample 0: " << std::endl;
                std::cout << "Packet length (expected >=3, ==42)" << packet.length() << std::endl;
                std::cout << "Packet BX header (expected 0xaa000000) " << packet.bxheader() << std::endl;
                std::cout << "Packet idle at position 42 (expected 0xaccccccc)" << packet.idle() << std::endl;
            }
            status.update(packet);
        }
    }
}
bool HeaderCheckResults::is_acceptable (const double threshold) const
{
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
