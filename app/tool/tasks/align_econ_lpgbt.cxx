#include "align_econ_lpgbt.h"

void align_econ_lpgbt(Target* tgt) {
    auto econ = tgt->hcal().econ(pftool::state.iecon);

    auto lpgbt_daq = tgt->lpgbt_ic;

    bool comm_is_i2c = (tgt->lpgbt == tgt->lpgbt_i2c);
    if (comm_is_i2c)
      printf(" Communication by I2C\n");
    else if (tgt->lpgbt == tgt->lpgbt_ic)
      printf(" Communication by IC\n");
    else
      printf(" Communication by EC\n");
    
    int pusm = tgt->lpgbt->status();
    printf(" PUSM %s (%d)\n", tgt->lpgbt->status_name(pusm).c_str(), pusm);

    int pusm = tgt->lpgbt_ic->status();
    printf(" PUSM %s (%d)\n", tgt->lpgbtic->status_name(pusm).c_str(), pusm);
    
}
