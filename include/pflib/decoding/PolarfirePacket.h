#ifndef pflib_decoding_PolarfirePacket_h
#define pflib_decoding_PolarfirePacket_h 1

#include "pflib/decoding/LinkPacket.h"

namespace pflib {
namespace decoding {

/** 
 * decodes the innermost part of an HGCROC packet given 
 * a pointer to a series of unsigned 32-bit integers and a length.
 */
class PolarfirePacket {
 public:
  PolarfirePacket(const uint32_t* header_ptr, int len)  : data_{header_ptr}, length_{len} { }

  int length() const { if (length_<1) return -1; return data_[0]&0xFFF; }
  int nlinks() const { if (length_<1) return -1; return (data_[0]>>14)&0x3F; }
  int fpgaid() const { if (length_<1) return -1; return (data_[0]>>20)&0xFF; }
  int formatversion() const { if (length_<1) return -1; return (data_[0]>>28)&0xF; }

  int length_for_elink(int ilink) const { if (ilink>=nlinks()) return 0; return (data_[2+(ilink/4)]>>(8*(ilink%4)))&0x3F; }
    
  int bxid() const { if (length_<2) return -1; return (data_[1]>>20)&0xFFF;}

  int rreq() const { if (length_<2) return -1; return (data_[1]>>10)&0x3FF;}

  int orbit() const { if (length_<2) return -1; return data_[1]&0x3FF;}

  int linklen(int link) const { if (length_<3) return -1; return (data_[2]>>(link*8))&0x3F;}
  
  int linkcrc(int link) const { if (length_<3) return -1; return (data_[2]>>(link*8+6))&0x1;}
  
  int linkrid(int link) const { if (length_<3) return -1; return (data_[2]>>(link*8+7))&0x1;}

  LinkPacket link(int ilink) const;
 private:
  int offset_to_elink(int ilink) const;
  
  const uint32_t* data_;
  int length_;
};
  
}
}

#endif// pflib_decoding_PolarfirePacket_h
  
