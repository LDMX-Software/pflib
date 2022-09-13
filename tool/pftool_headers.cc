
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
void HeaderCheckResults::add_event(const pflib::decoding::SuperPacket event,
                                   const int nsamples) {
  for (int sample{0}; sample < nsamples; ++sample) {
    for (auto &status : res) {
      const auto packet{event.sample(sample).link(status.link)};
      status.update(packet);
    }
  }
}

bool HeaderStatus::is_acceptable(const float threshold) const
{
    if (percent_bad_headers() > threshold) {
        std::cout << "Bad headers!, "
                  << percent_bad_headers() * 100.
                  << " %" << std::endl;
        return false;
    }
    if (percent_bad_idles() > threshold) {
        std::cout << "Bad idles!, "
                  << percent_bad_idles() * 100.
                  << " %" << std::endl;
        return false;
    }
    return true;
}
bool HeaderCheckResults::is_acceptable(const std::vector<float> thresholds) const
{
    for (auto link_index{0}; link_index < thresholds.size(); ++link_index) {
        auto& status {res[link_index]};
        std::cout << "Testing link " << status.link << "... ";
        auto threshold = thresholds[link_index];
        if (!status.is_acceptable(threshold)) {
            return false;
        }
        std::cout << " ok!" << std::endl;

    }
    return true;
}
bool HeaderCheckResults::is_acceptable(const float threshold) const {
  for (auto status : res) {
    std::cout << "Testing link " << status.link << "... ";
    if (!status.is_acceptable(threshold)) {
      return false;
    }
    std::cout << " ok!" << std::endl;
  }
  return true;
};
void HeaderStatus::update(const pflib::decoding::LinkPacket packet) {
  if (packet.length() > 2) {
    if (packet.good_bxheader()) {
      n_good_bxheaders++;
    } else {
      n_bad_bxheaders++;
    }
    if (packet.good_idle()) {
      n_good_idles++;
    } else {
      n_bad_idles++;
    }
  }
}
