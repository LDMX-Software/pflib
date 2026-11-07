#!/bin/bash
# after Prestart, just before Go: clear the RoR left by Prestart on both cards
source /u1/tamasvami/iss49/ldmx-firmware/software/setup_env_esa.sh > /dev/null 2>&1
for d in 1 2; do echo "datadev_$d: $(python /u1/tamasvami/merger-sim/formatter_reset.py --dev /dev/datadev_$d 2>&1 | grep -v Rogue | tr "\n" " ")"; done
