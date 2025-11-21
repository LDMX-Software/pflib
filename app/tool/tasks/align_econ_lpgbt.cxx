#include "align_econ_lpgbt.h"


void align_econ_lpgbt(Target* tgt) {
  auto econ = tgt->econ(pftool::state.iecon);


  if (pftool::state.iecon!=0) {
    printf(" I only know how to align ECON-D to link 0\n");
    return;
  }
  uint32_t idle=pftool::readline_int("Idle pattern",0x1277CC,true);

  std::vector<uint32_t> got;
  for (int phase = 0; phase<32; phase++) {
    econ.applyParameter("FormatterBuffer","Global_align_serializer_0",phase);
    usleep(1000);
    std::vector<uint32_t> spy=tgt->elinks().spy(0);
    got.push_back(spy[0]);
    uint32_t obs=spy[0]>>8;
    if (obs==idle) {
      printf("Found alignment at %d\n",phase);
      return;
    }
  }
  for (int phase=0; phase<32; phase++) {
    printf(" %2d 0x%08x\n",phase,got[phase]);
  }
  printf("Did not find alignment\n");
}
