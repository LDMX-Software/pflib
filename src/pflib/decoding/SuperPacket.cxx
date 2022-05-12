#include "pflib/decoding/SuperPacket.h"
#include <stdio.h>

namespace pflib {
namespace decoding {

SuperPacket::SuperPacket(const uint32_t* header_ptr, int len) : data_{header_ptr}, length_{len}, version_{0}, offset_{0} {
  bool found_header=false;
  while (length_>0 && !found_header) {
    if (*data_==0xBEEF2021u) {
      found_header=true;
      version_=1;
    }
    if (*data_==0xBEEF2022u) {
      found_header=true;
      version_=2;
    }
    length_--;
    data_++;
    offset_++;
  }
}
  
int SuperPacket::length64() const { return (length32()+1)/2; }
int SuperPacket::length32() const {
  if (version_==1) return data_[0]&0xFFFF;
  if (version_==2) return (data_[0]&0xFFFF)*2;
  return -1;
}
int SuperPacket::length32_for_sample(int isample) const {
  if (isample<0 || isample>=nsamples()) return 0;
  return (data_[1+(isample/2)]>>(16*(isample%2)))&0xFFF;
}

int SuperPacket::event_tag_length() const {
  if (version_<2) return 0;
  else return (data_[1+8]>>24)&0xFF;
}
int SuperPacket::spill() const {
  if (version_<2) return 0;
  else return (data_[1+8]>>12)&0xFFF;
}
int SuperPacket::bxid() const {
  if (version_<2) return 0;
  else return (data_[1+8])&0xFFF;
}
uint32_t SuperPacket::time_in_spill() const {
  if (version_!=2 || event_tag_length()<4) return 0;
  else return (data_[1+8+1]);  
}
uint32_t SuperPacket::eventid() const {
  if (version_!=2 || event_tag_length()<4) return 0;
  else return (data_[1+8+2]);  
}
int SuperPacket::runid() const {
  if (version_!=2 || event_tag_length()<4) return 0;
  else return (data_[1+8+3]&0xFFF);    
}
void SuperPacket::run_start(int& month, int& day, int& hour, int& minute) {
  if (version_!=2 || event_tag_length()<4) return;
  uint32_t value=data_[1+8+3]>>12;
  minute=value&0x3F;
  hour=(value>>6)&0x1F;
  day=(value>>11)&0x1F;
  month=(value>>16)&0xF;
}

PolarfirePacket SuperPacket::sample(int isample) const {
  int offset=0;
  if (version_==1) {
    offset=1+((nsamples()+1)/2);
  } else if (version_==2) {
    offset=1+8; // fixed length information
    offset+=event_tag_length();
  }
  for (int i=0; i<isample; i++)
    offset+=length32_for_sample(i);
  return PolarfirePacket(data_+offset,length32_for_sample(isample));
}

}
}
