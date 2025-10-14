#!/bin/bash

#Get side and module
SIDE=$1 #e or w
MOD=$2

#Get ECON(d,t)
ECON=$3

#Get config
CONFIG=$4

# Define base command for reuse
BASE_CMD="/opt/swamp/bin/econ_control -F hwcfg_uva_partials.yaml"

#echo "$BASE_CMD -l trg${SIDE} -e econ${ECON}_${SIDE}${MOD} -f configs/$CONFIG -w 0"
#$BASE_CMD -l trg${SIDE} -e econ${ECON}_${SIDE}${MOD} -f configs/$CONFIG -w 0

echo "$BASE_CMD -l trg${SIDE} -e econ${ECON}_${SIDE}${MOD} -f configs/$CONFIG -w 1"
$BASE_CMD -l trg${SIDE} -e econ${ECON}_${SIDE}${MOD} -f configs/$CONFIG -w 1


echo "$BASE_CMD -l trg${SIDE} -e econ${ECON}_${SIDE}${MOD} -f configs/$CONFIG -w 0 -c 1"
$BASE_CMD -l trg${SIDE} -e econ${ECON}_${SIDE}${MOD} -f configs/$CONFIG -w 0 -c 1
