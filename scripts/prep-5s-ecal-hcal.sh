#!/bin/bash
# last step before Go (GUI already in Prestart): 5-sample prep of L2, L3, HCal,
# then clear both formatters so no RoR from Prestart is left over
cd /u1/tamasvami/pflib
L=logs-$(date +%Y%m%d)/prep5s-$(date +%H%M%S); mkdir -p $L
./build/pftool -s scripts/daq-axis-ext-l2-5s.pfl config/pftool/ecal-smm-esa-l2.yaml < /dev/null > $L/l2.log 2>&1
./build/pftool -s scripts/run-final-l3-5s.pfl config/pftool/l3-roc023-ecal-smm-esa.yaml < /dev/null > $L/l3.log 2>&1
./build/pftool -s scripts/run-final-hcal-5s.pfl config/pftool/hcal-backplane-esa.yaml < /dev/null > $L/hcal.log 2>&1
source /u1/tamasvami/iss49/ldmx-firmware/software/setup_env_esa.sh > /dev/null 2>&1
python /u1/tamasvami/merger-sim/formatter_reset.py --dev /dev/datadev_1 > $L/fmt1.log 2>&1
python /u1/tamasvami/merger-sim/formatter_reset.py --dev /dev/datadev_2 > $L/fmt2.log 2>&1
for t in l2 l3 hcal; do
  echo "$t: $(grep -E "^ +(Enabled|AXIS Enabled|Samples per RoR|SoI):" $L/$t.log | tr -s " " | tr "\n" ";") warn=$(grep -c warn $L/$t.log)"
done
echo "formatter reset: $(grep -v Rogue $L/fmt1.log | tail -1) | $(grep -v Rogue $L/fmt2.log | tail -1)"
