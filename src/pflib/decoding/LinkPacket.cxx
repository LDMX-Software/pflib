#include "pflib/decoding/LinkPacket.h"
#include <stdio.h>

namespace pflib {
namespace decoding {

LinkPacket::LinkPacket(const uint32_t* header_ptr, int len) 
  : data_{header_ptr}, length_{len} {}

int LinkPacket::linkid() const {
  if (length_ == 0) return -1;
  return (data_[0]>>16)&0xFFFF;
}

int LinkPacket::crc() const {
  if (length_==0) return -1; 
  return (data_[0]>>15)&0x1;
}

int LinkPacket::bxid() const { 
  if (length_<3) return -1; 
  return (data_[2]>>11)&0x7FF;
}

int LinkPacket::wadd() const { 
  if (length_<3) return -1; 
  return (data_[2]>>3)&0xFF;
}

int LinkPacket::length() const {
  return length_; 
}

bool LinkPacket::good_bxheader() const { 
  if (length_<3) return false; 
  return (data_[2]&0xff000000)==0xaa000000; 
}

bool LinkPacket::good_idle() const { 
  if (length_<42) return false; 
  return data_[41]==0xaccccccc; 
}

bool LinkPacket::has_chan(int ichan) const {
  return offset_to_chan(ichan)>=0;
}

int LinkPacket::get_tot(int ichan) const { 
  int offset = offset_to_chan(ichan); 
  if (offset == -1) return -1; 
  return (data_[offset]>>20)&0xFFF;
} 

int LinkPacket::get_toa(int ichan) const {
  int offset = offset_to_chan(ichan); 
  if (offset == -1) return -1; 
  return (data_[offset]>>10)&0x3FF;
}

int LinkPacket::get_adc(int ichan) const { 
  int offset = offset_to_chan(ichan); 
  if (offset == -1) return -1; 
  return data_[offset]&0x3FF;
} 

void LinkPacket::dump() const {
  for (int i=0; i<length_; i++) {
    printf("%2d %08x ",i,data_[i]);
    if (i<36 && has_chan(i)) printf(" %d",get_adc(i));
    printf("\n");
  }
}

int LinkPacket::offset_to_chan(int ichan) const {
  if (length_<2 || ichan<0 || ichan>=36) return -1;
  int offset=0;

  uint64_t readout_map=(uint64_t(data_[0])<<32)|data_[1];

  int inominal=ichan+1; // first word in ROC V2 is header
  if (ichan>=18) inominal+=2; // common mode and calib in the middle

  for (int i=0; i<inominal; i++) {
    if (readout_map&0x1) offset++;
    readout_map=readout_map>>1;
  }
  if (readout_map&0x1) return offset+2; // +2 for the two Polarfire header words
  else return -1;

}

}
}
