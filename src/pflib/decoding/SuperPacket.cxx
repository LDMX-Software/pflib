#include "pflib/decoding/SuperPacket.h"

namespace pflib {
namespace decoding {

SuperPacket::SuperPacket(const uint32_t* header_ptr, int len);
  
int SuperPacket::length64() const;
int SuperPacket::length32() const;
int SuperPacket::fpgaid() const;
int SuperPacket::nsamples() const;
int SuperPacket::formatversion() const;
int SuperPacket::length_for_sample(int isample);

PolarfirePacket SuperPacket::sample(int isample) const;

}
}
