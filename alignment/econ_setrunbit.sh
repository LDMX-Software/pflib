#!/bin/bash

#Get module:
SIDE=$1
MOD=$2

# Define base command for reuse
BASE_CMD="/opt/swamp/bin/econ_control -F hwcfg_uva_partials.yaml"

for ECON in d t; do
    # Write 1 to run bit
    echo "/--------Writing run1 to run bit: MODULE ${MOD} ECON ${ECON}--------/"
    $BASE_CMD -l trg${SIDE} -e econ${ECON}_${SIDE}${MOD} -p ClocksAndResets.Global.pusm_run -w 1 -v 1
    sleep 1
    echo "/------Read Power Up State Machine: MODULE ${MOD} ECON ${ECON}------/"
    # Read Power Up State Machine
    $BASE_CMD -l trg${SIDE} -e econ${ECON}_${SIDE}${MOD} -p ClocksAndResets.Global.pusm_state -w 0
done
