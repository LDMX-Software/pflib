#!/bin/bash

#Get side and module:
SIDE=$1 # e or w
MOD=$2

# Define base command for reuse
BASE_CMD="/opt/swamp/bin/lpgbt_gpio_control -F hwcfg_uva_partials.yaml"

# Check east 0 with 1-second delay
$BASE_CMD -l trg${SIDE} -g pwr_en_${SIDE}${MOD} -w 1 -u 0
sleep 1

$BASE_CMD -l trg${SIDE} -g hgcroc_re_hb_${SIDE}${MOD} -w 1 -u 0
sleep 1

$BASE_CMD -l trg${SIDE} -g hgcroc_re_sb_${SIDE}${MOD} -w 1 -u 0
sleep 1

$BASE_CMD -l trg${SIDE} -g econ_re_hb_${SIDE}${MOD} -w 1 -u 0
sleep 1

$BASE_CMD -l trg${SIDE} -g econ_re_sb_${SIDE}${MOD} -w 1 -u 0

