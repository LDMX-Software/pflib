#include "pflib/decoding/PolarfirePacket.h"

namespace pflib {
namespace decoding {

RocPacket PolarfirePacket::roc(int ilink) const {
  int offset=offset_to_elink(ilink);
  if (offset<0) return RocPacket(0,0);
  else return RocPacket(data_+offset,length_for_elink(ilink));
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
