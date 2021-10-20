#include "pflib/Hcal.h"

namespace pflib {
static const int GPO_BIT_SOFTRESET0  = 0;
static const int GPO_BIT_RESYNCLOAD0 = 4;
static const int GPO_BIT_HARDRESET   = 8;
    
static const int N_ROC = 4;

Hcal::Hcal(WishboneInterface* wb) : wb_(wb), gpio_(wb), i2c_(wb) {
  // make sure reset is /disabled/
  std::vector<bool> gpo=gpio_.getGPO();

  // soft resets
  for (int i=GPO_BIT_SOFTRESET0; i<GPO_BIT_SOFTRESET0+N_ROC; i++)
    gpo[i]=true;

  gpo[GPO_BIT_HARDRESET]=true;

  gpio_.setGPO(gpo);
}

ROC Hcal::roc(int which) {
  if (which<0 || which>=N_ROC) {
    PFEXCEPTION_RAISE("InvalidROCid","Requested out-of-range ROC id");
  }
  return ROC(i2c_,which+0); 
}

  Bias Hcal::bias(int which) {
    if (which<0 || which>=N_ROC) {
      PFEXCEPTION_RAISE("InvalidBoardId","Requested out-of-range Board id");
    }
    return Bias(i2c_,which+4); 
  }
  
void Hcal::hardResetROCs() {
  gpio_.setGPO(GPO_BIT_HARDRESET,false);
  gpio_.setGPO(GPO_BIT_HARDRESET,true);
}

void Hcal::softResetROC(int which) {
  if (which>=0 && which<N_ROC) {
    gpio_.setGPO(GPO_BIT_SOFTRESET0+which,false);
    gpio_.setGPO(GPO_BIT_SOFTRESET0+which,true);
  } else {
    for (int i=0; i<N_ROC; i++ ) {
      gpio_.setGPO(GPO_BIT_SOFTRESET0+i,false);
      gpio_.setGPO(GPO_BIT_SOFTRESET0+i,true);
    }
  }
}

void Hcal::resyncLoadROC(int which) {
  if (which>=0 && which<N_ROC) {
    gpio_.setGPO(GPO_BIT_RESYNCLOAD0+which,false);
    gpio_.setGPO(GPO_BIT_RESYNCLOAD0+which,true);
    gpio_.setGPO(GPO_BIT_RESYNCLOAD0+which,false);
  } else {
    for (int i=0; i<N_ROC; i++ ) {
      gpio_.setGPO(GPO_BIT_RESYNCLOAD0+i,false);
      gpio_.setGPO(GPO_BIT_RESYNCLOAD0+i,true);
      gpio_.setGPO(GPO_BIT_RESYNCLOAD0+i,false);
    }
  }
}



}
