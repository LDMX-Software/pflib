#!/bin/bash
# run prep after Prestart: L1OFFSET, 5 samples, central L1As
cd "$(dirname "$0")/../.."
source /u1/ldmx/miniforge3/etc/profile.d/conda.sh && conda activate ldmx-env-base
# until 24 is the pflib default
export PFLIB_LPGBT_I2C_CFG=${PFLIB_LPGBT_I2C_CFG:-24}
L=logs-$(date +%Y%m%d)/prep-$(date +%H%M%S); mkdir -p $L
for x in "l2 ecal-smm-esa-l2-100v" "l3 l3-roc023-ecal-smm-esa-100v" "hcal hcal-backplane-esa"; do
  set -- $x
  ./build/pftool -s scripts/esa26/prep-$1.pfl config/pftool/$2.yaml < /dev/null > $L/$1.log 2>&1
  echo "$1: rc $? NoACK $(grep -c I2CErrorNoACK $L/$1.log) $(grep -E "^ +(Enabled|AXIS Enabled|Samples per RoR|SoI):" $L/$1.log | tr -s " " | tr "\n" ";")"
done
