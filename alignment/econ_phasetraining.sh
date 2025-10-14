#!/bin/bash

#Get module:
MOD=$1

#Get econ(d,t):
ECON=$2

#Get side (east, west):
SIDE=$3

# Define base command for reuse
BASE_CMD="/opt/swamp/bin/econ_control -F hwcfg_uva_partials.yaml"
BASE_CMD_ROC="/opt/swamp/bin/roc_control -F hwcfg_uva_partials.yaml"

IDLE=89478485 # 0x5555555
for IROC in {0..2}; do
    if [[ "$MOD" -gt 0 && "$IROC" -eq 2 ]]; then
        continue
    fi
    $BASE_CMD_ROC -l trg${SIDE} -r roc${IROC}_${SIDE}${MOD} -w 1 -p DigitalHalf.0.IdleFrame -v $IDLE
    $BASE_CMD_ROC -l trg${SIDE} -r roc${IROC}_${SIDE}${MOD} -w 1 -p DigitalHalf.1.IdleFrame -v $IDLE
done

# Write 0 to run bit while configuring
$BASE_CMD -l trg${SIDE} -e econ${ECON}_${SIDE}${MOD} -p ClocksAndResets.Global.pusm_run -w 1 -v 0
sleep 1

# Phase ON
$BASE_CMD -l trg${SIDE} -e econ${ECON}_${SIDE}${MOD} -f configs/train_erx_phase_ON_econ.yaml -w 1
$BASE_CMD -l trg${SIDE} -e econ${ECON}_${SIDE}${MOD} -f configs/train_erx_phase_TRAIN_econ.yaml -w 1

# Set run bit
$BASE_CMD -l trg${SIDE} -e econ${ECON}_${SIDE}${MOD} -p ClocksAndResets.Global.pusm_run -w 1 -v 1
# ADD LINK RESET HERE
#./uhal_backend_v3.py --send link_reset_roc${ECON}
#./uhal_backend_v3.py --send link_reset_roc${ECON}
#./uhal_backend_v3.py --send link_reset_roc${ECON}
#./uhal_backend_v3.py --send link_reset_roc${ECON}
#./uhal_backend_v3.py --send link_reset_roc${ECON}

# Set run bit off
$BASE_CMD -l trg${SIDE} -e econ${ECON}_${SIDE}${MOD} -p ClocksAndResets.Global.pusm_run -w 1 -v 0
# Set training off
$BASE_CMD -l trg${SIDE} -e econ${ECON}_${SIDE}${MOD} -f configs/train_erx_phase_OFF_econ.yaml -w 1
# Set run bit on
$BASE_CMD -l trg${SIDE} -e econ${ECON}_${SIDE}${MOD} -p ClocksAndResets.Global.pusm_run -w 1 -v 1

# Check eRX
$BASE_CMD -l trg${SIDE} -e econ${ECON}_${SIDE}${MOD} -f configs/check_erx_current_channel_locked_econ$ECON.yaml -w 0
# Read PUSM
$BASE_CMD -l trg${SIDE} -e econ${ECON}_${SIDE}${MOD} -p ClocksAndResets.Global.pusm_state -w 0

# Check phase_select
$BASE_CMD -l trg${SIDE} -e econ${ECON}_${SIDE}${MOD} -p ChEprxGrp.00.phase_select_channeloutput -w 0
$BASE_CMD -l trg${SIDE} -e econ${ECON}_${SIDE}${MOD} -p ChEprxGrp.01.phase_select_channeloutput -w 0
$BASE_CMD -l trg${SIDE} -e econ${ECON}_${SIDE}${MOD} -p ChEprxGrp.02.phase_select_channeloutput -w 0
$BASE_CMD -l trg${SIDE} -e econ${ECON}_${SIDE}${MOD} -p ChEprxGrp.03.phase_select_channeloutput -w 0
$BASE_CMD -l trg${SIDE} -e econ${ECON}_${SIDE}${MOD} -p ChEprxGrp.04.phase_select_channeloutput -w 0
$BASE_CMD -l trg${SIDE} -e econ${ECON}_${SIDE}${MOD} -p ChEprxGrp.05.phase_select_channeloutput -w 0
$BASE_CMD -l trg${SIDE} -e econ${ECON}_${SIDE}${MOD} -f configs/check_erx_current_channel_locked_econ$ECON.yaml -w 0

# Read trackmode
$BASE_CMD -l trg${SIDE} -e econ${ECON}_${SIDE}${MOD} -p EprxGrpTop.Global.track_mode -w 0
# Read Power Up State Machine
$BASE_CMD -l trg${SIDE} -e econ${ECON}_${SIDE}${MOD} -p ClocksAndResets.Global.pusm_state -w 0
