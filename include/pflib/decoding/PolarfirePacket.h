#ifndef pflib_decoding_PolarfirePacket_h
#define pflib_decoding_PolarfirePacket_h 1

#include "pflib/decoding/RocPacket.h"

namespace pflib {
namespace decoding {

  /** \class This class decodes the innermost part of an HGCROC packet given
    a pointer to a series of unsigned 32-bit integers and a length.
*/
class PolarfirePacket {
 public:
  PolarfirePacket(const uint32_t* header_ptr, int len)  : data_{header_ptr}, length_{len} { }

  int length() const { if (length_<1) return -1; return data_[0]&0xFFF; }
  int nlinks() const { if (length_<1) return -1; return (data_[0]>>14)&0x3F; }
  int fpgaid() const { if (length_<1) return -1; return (data_[0]>>20)&0xFF; }
  int formatversion() const { if (length_<1) return -1; return (data_[0]>>28)&0xF; }

  int length_for_elink(int ilink) const { if (ilink>=nlinks) return 0; return (data_[2+(ilink/4)]>>(8*(ilink%4)))&0x3F; }
    
  RocPacket roc(int iroc) const;
private:
  int offset_to_elink(int iroc) const;
  
  const uint32_t* data_;
  int length_;
};
  
}
}

#endif// pflib_decoding_PolarfirePacket_h
  
