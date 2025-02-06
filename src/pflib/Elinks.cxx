#include "pflib/Elinks.h"
#include <unistd.h>
#include <stdio.h>

namespace pflib {

  Elinks::Elinks(int links) : n_links{links} {
    m_linksActive=std::vector<bool>(n_links,true);
  }

  static const uint32_t ALIGN_PATTERN=0xACCCCCCCu;
  
  // bit slip should produce 0xACCCCCC
  int Elinks::scanBitslip(int ilink) {
    int islip;
    for (islip=0; islip<32; islip++) {
      setBitslip(ilink,islip);
      usleep(10);
      std::vector<uint32_t> sv=spy(ilink);
      if (sv[0]==ALIGN_PATTERN) return islip;
    }
    return -1; // nothing found
  }

  int Elinks::scanAlign(int ilink, bool debug) {
    std::vector<std::pair<int,int> > points;

    int zero_started=-1;
    for (int i=0; i<255; i+=5) {
      // set a phase
      setAlignPhase(ilink,i);
      //
      uint32_t v;
      int mismatches=0;
      
      for (int test=0; test<100; test++) {
        std::vector<uint32_t> x=spy(ilink);
        if (test>0 && x[0]!=v) mismatches++;
	v=x[0];
      }
      if (debug) printf(" %d %d \n",i,mismatches);
      if (mismatches==0 && zero_started<0) zero_started=i;
      else if (mismatches!=0 && zero_started>=0) {
	points.push_back(std::pair<int,int>(zero_started,i-1));
	zero_started=-1;
      }
    }
    if (zero_started>=0) points.push_back(std::pair<int,int>(zero_started,255));

    if (points.empty()) return -1;

    int deltaTMax=points[0].second-points[0].first;
    int bestpoint=(points[0].second+points[0].first)/2;
    if (debug) printf(" DeltaT : %d %d\n",deltaTMax,bestpoint);
    for (size_t ipt=1; ipt<points.size(); ipt++) {
      int adt=points[ipt].second-points[ipt].first;
      int abt=(points[ipt].second+points[ipt].first)/2;
      if (debug) printf(" DeltaT : %d %d\n",adt,abt);
      if (adt>deltaTMax) {
	deltaTMax=adt;
	bestpoint=abt;
      }
    }
    return bestpoint;    
  }
  
}
