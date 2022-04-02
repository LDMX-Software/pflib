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
  RocPacket(const uint32_t* header_ptr, int len);

  int rocid() const { if (length_==0) return -1; return (data_[0]>>16)&0xFFFF;}

  bool has_chan(int ichan) const;
   
 private:
  int offset_to_chan(int ichan) const;

  
  const uint32_t* data_;
  int length_;
  
};

}
}

#endif// pflib_decoding_RocPacket_h
  
