#include "align_econ_lpgbt.h"

void align_econ_lpgbt(Target* tgt) {
    auto econ = tgt->hcal().econ(pftool::state.iecon);

    int chipaddr = 0x78;
    chipaddr |= 0x4;

    pflib::zcu::lpGBT_ICEC_Simple ic("standardLpGBTpair-0", false, chipaddr);
    pflib::lpGBT lpgbt_daq(ic);
    pflib::zcu::lpGBT_ICEC_Simple ec("standardLpGBTpair-0", true, 0x78);
    pflib::lpGBT lpgbt_trg(ec);
    
    int pusm_daq = lpgbt_daq.status();
    printf(" lpGBT-DAQ PUSM %s (%d)\n", lpgbt_daq.status_name(pusm_daq).c_str(), pusm_daq);

    int pusm_trg = lpgbt_trg.status();
    printf(" lpGBT-TRG PUSM %s (%d)\n", lpgbt_trg.status_name(pusm_trg).c_str(), pusm_trg);
    
}
