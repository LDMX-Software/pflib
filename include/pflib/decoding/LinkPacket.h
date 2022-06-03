#ifndef pflib_decoding_LinkPacket_h
#define pflib_decoding_LinkPacket_h 1

#include <stdint.h>

namespace pflib {
namespace decoding {

class LinkPacket {
 public:
  LinkPacket(const uint32_t* header_ptr, int len);

  int linkid() const { if (length_==0) return -1; return (data_[0]>>16)&0xFFFF;}

  int crc() const {if (length_==0) return -1; return (data_[0]>>15)&0x1;}

  int bxid() const { if (length_<3) return -1; return (data_[2]>>11)&0x7FF;}

  int wadd() const { if (length_<3) return -1; return (data_[2]>>3)&0xFF;}

  int length() const { return length_; }

  bool good_bxheader() const { if (length_<3) return false; return (data_[2]&0xff000000)==0xaa000000; }

  bool good_idle() const { if (length_<42) return false; return data_[41]==0xaccccccc; }

  bool has_chan(int ichan) const { return offset_to_chan(ichan)>=0; }
 
  int get_tot(int ichan) const { int offset = offset_to_chan(ichan); if (offset == -1) return -1; return (data_[offset]>>20)&0xFFF;} 

  int get_toa(int ichan) const { int offset = offset_to_chan(ichan); if (offset == -1) return -1; return (data_[offset]>>10)&0x3FF;} 
  
  int get_adc(int ichan) const { int offset = offset_to_chan(ichan); if (offset == -1) return -1; return data_[offset]&0x3FF;} 

  void dump() const;
  
 private:
  int offset_to_chan(int ichan) const;
  
  const uint32_t* data_;
  int length_;
  
};

}
}

#endif// pflib_decoding_LinkPacket_h
  
