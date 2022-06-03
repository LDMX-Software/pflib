#include "pflib/decoding/PolarfirePacket.h"

namespace pflib {
namespace decoding {

PolarfirePacket::PolarfirePacket(const uint32_t* header_ptr, int len)
  : data_{header_ptr}, length_{len} {}

int PolarfirePacket::length() const { 
  if (length_<1) return -1; 
  return data_[0]&0xFFF; 
}

int PolarfirePacket::nlinks() const { 
  if (length_<1) return -1; 
  return (data_[0]>>14)&0x3F; 
}

int PolarfirePacket::fpgaid() const {
  if (length_<1) return -1;
  return (data_[0]>>20)&0xFF;
}

int PolarfirePacket::formatversion() const {
  if (length_<1) return -1;
  return (data_[0]>>28)&0xF;
}

int PolarfirePacket::length_for_elink(int ilink) const {
  if (ilink>=nlinks()) return 0;
  return (data_[2+(ilink/4)]>>(8*(ilink%4)))&0x3F;
}

int PolarfirePacket::bxid() const {
  if (length_<2) return -1;
  return (data_[1]>>20)&0xFFF;
}

int PolarfirePacket::rreq() const {
  if (length_<2) return -1;
  return (data_[1]>>10)&0x3FF;
}

int PolarfirePacket::orbit() const {
  if (length_<2) return -1;
  return data_[1]&0x3FF;
}

LinkPacket PolarfirePacket::link(int ilink) const {
  int offset=offset_to_elink(ilink);
  if (offset<0) return LinkPacket(0,0);
  else return LinkPacket(data_+offset,length_for_elink(ilink));
}

int PolarfirePacket::offset_to_elink(int ilink) const {
  if (ilink<0 || ilink>=nlinks()) return -1;
  int offset=2+((nlinks()+3)/4); // header words
  for (int i=0; i<ilink; i++)
    offset+=length_for_elink(i);
  return offset;
}

}
}
