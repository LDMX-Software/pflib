#!/bin/bash
# ZS before the run: ECal calib ped + TE, HCal pass-through ped + TH
# TE=5 TH=10 OUT=dir [ONLY=ecal|hcal] scripts/esa26/zs-on.sh
cd "$(dirname "$0")/../.."
source /u1/ldmx/miniforge3/etc/profile.d/conda.sh && conda activate ldmx-env-base
export PFLIB_LPGBT_I2C_CFG=${PFLIB_LPGBT_I2C_CFG:-24}
TE=${TE:-5}; TH=${TH:-10}; R=$(realpath -m ${OUT:-zs-$(date +%Y%m%d)}); mkdir -p $R
C=config/pftool
# pedestal run into $R/$1.csv
ped() { printf "DAQ\nRESET\nSETUP\nFORMAT\nCONFIG\n\n\n\n\nDISABLE\nENABLE\nN\nN\nEXIT\nPEDESTAL\n$2\n\n$R/$1\nY\nSTATUS\nSETUP\nDISABLE\nEXIT\nEXIT\n"; }
kept() { python3 -c "
import csv; r=[x for x in csv.DictReader(open('$R/$1.csv')) if x['channel'].isdigit()]; print(round(sum(int(x['adc'])>0 for x in r)/$2,3))"; }
one() {  # name config zs-answers nevents
  { printf "TASKS\nSET_ZS_THRESHOLDS\n$3\nEXIT\n"; ped $1 $4; } > $R/$1.pfl
  ./build/pftool -s $R/$1.pfl $2 < /dev/null > $R/$1.log 2>&1; local rc=$?
  echo "$1: rc $rc NoACK $(grep -c I2CErrorNoACK $R/$1.log) err $(grep -c "\] error" $R/$1.log) | $(grep -o "ROC [0-9]: c from [0-9]* to [0-9]*" $R/$1.log | tr "\n" " ")| $(grep -o "pass-through [01]\|applied" $R/$1.log) | kept per event $(kept $1 $4)"
}
# answers: source, calib, T, lambda, kappa, apply, output
[ "$ONLY" = hcal ] || one zs-l2 $C/ecal-smm-esa-l2-100v.yaml "CALIB\n\n$TE\n0\n0\nY\n$R/zs-l2.yaml" 1000
[ "$ONLY" = hcal ] || one zs-l3 $C/l3-roc023-ecal-smm-esa-100v.yaml "CALIB\n\n$TE\n0\n0\nY\n$R/zs-l3.yaml" 1000
[ "$ONLY" = ecal ] || one hcal-pt $C/hcal-backplane-esa.yaml "YAML\nconfig/esa26/hcal-zs-defaults.yaml" 1000
[ "$ONLY" = ecal ] || python3 scripts/esa26/hcal_zs_yaml.py $R/hcal-pt.csv $TH $R/zs-hcal.yaml
[ "$ONLY" = ecal ] || one zs-hcal $C/hcal-backplane-esa.yaml "YAML\n$R/zs-hcal.yaml" 1000
