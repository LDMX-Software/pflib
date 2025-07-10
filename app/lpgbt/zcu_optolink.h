#ifndef ZCU_OPTOLINK_INCLUDED
#define ZCU_OPTOLINK_INCLUDED 1

#include "pflib/zcu/UIO.h"

namespace pflib {
namespace zcu {

class OptoLink {
 public:
  OptoLink();
  
 private:
  ::pflib::UIO transright_;
  
};

}
}

#endif ZCU_OPTOLINK_INCLUDED
