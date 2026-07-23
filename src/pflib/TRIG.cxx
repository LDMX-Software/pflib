#include "pflib/TRIG.h"

#include "pflib/Exception.h"

namespace pflib {

void TRIG::set_l1a_per_ror(int l1a_per_ror) {
  l1a_per_ror_ = l1a_per_ror;
}

int TRIG::get_l1a_per_ror() const {
  return l1a_per_ror_;
}

std::vector<uint32_t> TRIG::read_event() {
  if (get_l1a_per_ror() == 0) {
    PFEXCEPTION_RAISE("BadCode",
      "The number of L1A per RoR has not been copied into TRIG from DAQ.");
  }
  std::vector<uint32_t> event;
  for (int i{0}; i < get_l1a_per_ror(); i++) {
    auto sample = read_sample();
    event.insert(event.end(), sample.begin(), sample.end());
  }
  return event;
}

std::vector<uint32_t> TRIG::read_algo_output() {
  if (get_l1a_per_ror() == 0) {
    PFEXCEPTION_RAISE("BadCode",
      "The number of L1A per RoR has not been copied into TRIG from DAQ.");
  }
  std::vector<uint32_t> event;
  for (int i{0}; i < get_l1a_per_ror(); i++) {
    auto sample = read_algo_output_sample();
    event.insert(event.end(), sample.begin(), sample.end());
  }
  return event;
}

}
