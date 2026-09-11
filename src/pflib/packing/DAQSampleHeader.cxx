#include "pflib/packing/DAQSampleHeader.h"

#include "pflib/packing/Mask.h"

namespace pflib::packing {

void DAQSampleHeader::from(uint32_t word) {
  version = ((word >> 28) & mask<4>);
  econd_id = ((word >> 18) & mask<10>);
  i_l1a = ((word >> 13) & mask<5>);
  is_soi = (((word >> 12) & mask<1>) == 1);
  econd_len = (word & mask<12>);
}

uint32_t DAQSampleHeader::to() const {
  return (((version & mask<4>) << 28) | ((econd_id & mask<10>) << 18) |
          ((i_l1a & mask<5>) << 13) | ((is_soi ? 1 : 0) << 12) |
          (econd_len & mask<12>));
}

std::ostream& operator<<(std::ostream& o, const DAQSampleHeader& h) {
  return (o << "DAQSampleHeader { "
            << "version: " << h.version << ", "
            << "econd_id: " << h.econd_id << ", "
            << "i_l1a: " << h.i_l1a << ", "
            << "is_soi: " << h.is_soi << ", "
            << "econd_len: " << h.econd_len << " }");
}

uint32_t DAQSampleHeader::ending_trailer() {
  return DAQSampleHeader{.version = 1,
                         .econd_id = 0x3ff,
                         .i_l1a = 0x1f,
                         .is_soi = false,
                         .econd_len = 0}
      .to();
}

bool DAQSampleHeader::is_ending_trailer() const {
  return ((i_l1a == 0x1f) and (econd_id == 0x3ff));
}

}  // namespace pflib::packing
