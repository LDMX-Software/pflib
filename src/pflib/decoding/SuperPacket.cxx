#include "pflib/decoding/SuperPacket.h"

namespace pflib {
namespace decoding {

SuperPacket::SuperPacket(const uint32_t* header_ptr, int len) : data_{header_ptr}, length_{len}{
  bool found_header=false;
  while (len>0 && !found_header) {
    if (*header_ptr==0xBEEF2021u) found_header=true;      
    len--;
    header_ptr++;
  }          
}
  
int SuperPacket::length64() const { return -1;}
int SuperPacket::length32() const { return -1;}
int SuperPacket::length32_for_sample(int isample) const {
  if (isample<0 || isample>=nsamples()) return 0;
  return (data_[1+(isample/2)]>>(16*(isample%2)))&0xFFF;
}

PolarfirePacket SuperPacket::sample(int isample) const {
  int offset=1+((nsamples()+1)/2);
  for (int i=0; i<isample; i++)
    offset+=length32_for_sample(i);
  return PolarfirePacket(data_+offset,length32_for_sample(isample));
}

}
}
