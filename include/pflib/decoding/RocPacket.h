#ifndef pflib_decoding_RocPacket_h
#define pflib_decoding_RocPacket_h 1

#include <stdint.h>

namespace pflib {
namespace decoding {

/** \class This class decodes the innermost part of an HGCROC packet given
    a pointer to a series of unsigned 32-bit integers and a length.
*/
class RocPacket {
 public:
  RocPacket(const uint32_t* header_ptr, int len) : data_{header_ptr}, length_{len} { }

  int rocid() const { if (length_==0) return -1; return (data_[0]>>16)&0xFFFF;}

  int crc() const {if (length_==0) return -1; return (data_[0]>>15)&0x1;}

  int bxid() const { if (length_<3) return -1; return (data_[2]>>11)&0x7FF;}

  int wadd() const { if (length_<3) return -1; return (data_[2]>>3)&0xFF;}

  bool has_chan(int ichan) const { if (length_<2) return -1; if (ichan < 32) return (data_[1]>>ichan)&0x1; return (data_[0]>>(ichan-32))&0x1;}
 
  int get_tot(int ichan) const { int offset = offset_to_chan(ichan); if (offset == -1) return -1; return (data_[offset]>>34)&0x1FFFF;} 

  int get_toa(int ichan) const { int offset = offset_to_chan(ichan); if (offset == -1) return -1; return (data_[offset]>>17)&0x1FFFF;} 
  
  int get_adc(int ichan) const { int offset = offset_to_chan(ichan); if (offset == -1) return -1; return data_[offset]&0x1FFFF;} 
  
 private:
  int offset_to_chan(int ichan) const;
  
  const uint32_t* data_;
  int length_;
  
};

}
}

#endif// pflib_decoding_RocPacket_h
  
