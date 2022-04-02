#ifndef pflib_decoding_SuperPacket_h
#define pflib_decoding_SuperPacket_h 1

#include "pflib/decoding/PolarfirePacket.h"

namespace pflib {
namespace decoding {

  /** \class This class decodes the outer level of a multisample packet
   */
class SuperPacket {
 public:
  SuperPacket(const uint32_t* header_ptr, int len);
  
  int length64() const;
  int length32() const;
  int fpgaid() const {if (length_ == 0) return -1; return (data_[0]>>20)&0xFF;}
  int nsamples() const {if (length_ == 0) return -1; return (data_[0]>>16)&0xF;}
  int formatversion() const {if (length_ == 0) return -1; return (data_[0]>>28)&0xF;}
  int length32_for_sample(int isample) const;
    
  PolarfirePacket sample(int isample) const;
private:
  const uint32_t* data_;
  int length_;
  int version_;
};
  
}
}

#endif// pflib_decoding_PolarfirePacket_h
  
