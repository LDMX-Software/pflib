#include "scl_delay_scan.h"

#include "pflib/EcalSingleModuleMotherboard.h"

ENABLE_LOGGING();

void scl_delay_scan(Target* tgt) {
  auto smm = dynamic_cast<pflib::EcalSingleModuleMotherboard*>(tgt);
  if (not smm) {
    PFEXCEPTION_RAISE("BadTarget",
                      "scl_delay_scan only available for the ECal SMM");
  }

  int n_tries = pftool::readline_int("I2C tries per ROC and delay: ", 100);

  int before = smm->getSclDelay();
  int errors[4];
  for (int delay{0}; delay < 4; delay++) {
    smm->setSclDelay(delay);
    errors[delay] = 0;
    for (int i_roc : tgt->roc_ids()) {
      errors[delay] += tgt->roc(i_roc).countI2CErrors(n_tries);
    }
    pflib_log(info) << "SCL delay " << delay << ": " << errors[delay]
                    << " errors in " << n_tries * tgt->nrocs() << " tries";
  }

  // keep the delay in use unless another one is better
  int best{before};
  for (int delay{0}; delay < 4; delay++) {
    if (errors[delay] < errors[best]) best = delay;
  }

  smm->setSclDelay(best);
  if (errors[best] > 0) {
    pflib_log(warn) << "no SCL delay without I2C errors";
  }
  pflib_log(info) << "SCL delay set to " << best << " (was " << before << ")";
}
