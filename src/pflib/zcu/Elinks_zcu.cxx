#include "pflib/Elinks.h"
#include "pflib/zcu/UIO.h"

namespace pflib {

  class Elinks_zcu : public Elinks {
  public:
    Elinks_zcu(bool isdaq) : Elinks((isdaq)?(2):(4)) {
      isdaq_=isdaq;
    }

    virtual int getBitslip(int ilink);
    virtual uint32_t getStatusRaw(int ilink) { return 0; }
    virtual void setBitslip(int ilink, int bitslip);
    virtual std::vector<uint32_t> spy(int ilink);
    virtual void clearErrorCounters(int ilink) { }
    virtual void resetHard() { }
    
  private:
    int ctl_for(int ilink) { if (isdaq_) return 4+ilink; return 4+4+ilink; }
      
    bool isdaq_;
    static UIO uio_;
  };

  UIO Elinks_zcu::uio_("/dev/uio5",4*4096);
  
  static const uint32_t MASK_BITSLIP  = 0x00003E00;
  
  int Elinks_zcu::getBitslip(int ilink) {
    int ictl=ctl_for(ilink);
    return uio_.readMasked(ictl,MASK_BITSLIP);
  }

  void Elinks_zcu::setBitslip(int ilink, int bitslip) {
    int ictl=ctl_for(ilink);
    uio_.writeMasked(ictl,MASK_BITSLIP,bitslip);    
  }
  std::vector<uint32_t> Elinks_zcu::spy(int ilink) {
    return std::vector<uint32_t>();
  }

  
  Elinks* create_Elinks_zcu(bool daq) {
    return new Elinks_zcu(daq);
  }

}
