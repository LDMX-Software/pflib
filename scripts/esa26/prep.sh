#!/bin/bash
# ESA26 run prep, after Prestart and before OpenFile: L1OFFSET (HCal 63, ECal 65), 5 samples SoI 2,
# central L1As to AXIS on L2, L3, HCal (L2 first: the others start with DAQ RESET), then clear both
# formatters of any RoR left from Prestart. FW = ldmx-firmware checkout with formatter_reset.py.
cd "$(dirname "$0")/../.."
source /u1/ldmx/miniforge3/etc/profile.d/conda.sh && conda activate ldmx-env-base
# normal drive + pull-ups until 24 is the pflib default
export PFLIB_LPGBT_I2C_CFG=${PFLIB_LPGBT_I2C_CFG:-24}
FW=${FW:-/u1/tamasvami/hcaltrig/ldmx-firmware}
L=logs-$(date +%Y%m%d)/prep-$(date +%H%M%S); mkdir -p $L
for x in "l2 ecal-smm-esa-l2-100v" "l3 l3-roc023-ecal-smm-esa-100v" "hcal hcal-backplane-esa"; do
  set -- $x
  ./build/pftool -s scripts/esa26/prep-$1.pfl config/pftool/$2.yaml < /dev/null > $L/$1.log 2>&1
  echo "$1: rc $? NoACK $(grep -c I2CErrorNoACK $L/$1.log) $(grep -E "^ +(Enabled|AXIS Enabled|Samples per RoR|SoI):" $L/$1.log | tr -s " " | tr "\n" ";")"
done
source $FW/software/setup_env_esa.sh > /dev/null 2>&1
for d in 1 2; do python $FW/software/scripts/formatter_reset.py --dev /dev/datadev_$d 2>&1 | grep -v Rogue | tail -1; done
