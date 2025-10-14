#!/bin/bash

# execute as ./econ_align.sh 2 d 1

USE_FIVE=false

#Check for --five flag
if [[ "$1" == "--five" || "$1" == "-f" ]]; then
    USE_FIVE=true
    shift
fi
echo $USE_FIVE

#Get module:
MOD=$1

#Get econ(d,t):
ECON=$2

SIDE=$3
#Get chosen orbsyn_val
ORBSYNCSNAPSHOT=$4

echo "OrbSyncSnapshotValue" $ORBSYNCSNAPSHOT

# Define base command for reuse
BASE_CMD="/opt/swamp/bin/econ_control -F hwcfg_uva_partials.yaml"
BASE_CMD_ROC="/opt/swamp/bin/roc_control -F hwcfg_uva_partials.yaml"

if [ "$ECON" = "d" ]; then
    if [ "$MOD" -eq 0 ]; then
	ECON_ADDR=0x60
    elif [ "$MOD" -gt 0 ]; then
	ECON_ADDR=0x61
    fi
elif [ "$ECON" = "t" ]; then
    if [ "$MOD" -eq 0 ]; then
        ECON_ADDR=0x20
    elif [ "$MOD" -gt 0 ]; then
        ECON_ADDR=0x21
    fi
fi


echo "=====ROC configuration===="
# set idleframe in ROC 0xa5555aa: 173364650
#IDLE=268374015
# set idleframe in ROC 0xFFF0FFF: 268374015
#IDLE=173364650
# set idleframe in ROC 0x5555555: 89478485
IDLE=89478485

for IROC in {0..2}; do
	# skip ROC 2 if in a partial module
    if [[ "$MOD" -gt 0 && "$IROC" -eq 2 && "$USE_FIVE" == false ]]; then
        continue
    fi
    echo "========ROC ${IROC} from Module ${MOD}======="

    $BASE_CMD_ROC -l trg${SIDE} -r roc${IROC}_${SIDE}${MOD} -w 1 -p DigitalHalf.0.IdleFrame -v $IDLE
    $BASE_CMD_ROC -l trg${SIDE} -r roc${IROC}_${SIDE}${MOD} -w 1 -p DigitalHalf.1.IdleFrame -v $IDLE

    # set the register that inverts the Fast Commands reaching the ROC (1: inverts, 0: do not invert)
    # the full LD needs to have INVERT_FCMD=1
    INVERT_FCMD=0
    if [ "$MOD" -eq 0 ]; then
	INVERT_FCMD=1
    fi
    $BASE_CMD_ROC -l trg${SIDE} -r roc${IROC}_${SIDE}${MOD} -w 1 -p Top.0.in_inv_cmd_rx -v $INVERT_FCMD
    
    # read ROC registers
    $BASE_CMD_ROC -l trg${SIDE} -r roc${IROC}_${SIDE}${MOD} -w 0 -p Top.0.in_inv_cmd_rx
    $BASE_CMD_ROC -l trg${SIDE} -r roc${IROC}_${SIDE}${MOD} -w 0 -p Top.0.RunL
    $BASE_CMD_ROC -l trg${SIDE} -r roc${IROC}_${SIDE}${MOD} -w 0 -p Top.0.RunR
    $BASE_CMD_ROC -l trg${SIDE} -r roc${IROC}_${SIDE}${MOD} -w 0 -p DigitalHalf.0.IdleFrame
    $BASE_CMD_ROC -l trg${SIDE} -r roc${IROC}_${SIDE}${MOD} -w 0 -p DigitalHalf.1.IdleFrame
    $BASE_CMD_ROC -l trg${SIDE} -r roc${IROC}_${SIDE}${MOD} -w 0 -p DigitalHalf.0.Bx_offset
    $BASE_CMD_ROC -l trg${SIDE} -r roc${IROC}_${SIDE}${MOD} -w 0 -p DigitalHalf.1.Bx_offset
    $BASE_CMD_ROC -l trg${SIDE} -r roc${IROC}_${SIDE}${MOD} -w 0 -p DigitalHalf.0.Bx_trigger
    $BASE_CMD_ROC -l trg${SIDE} -r roc${IROC}_${SIDE}${MOD} -w 0 -p DigitalHalf.1.Bx_trigger
done

# configure ECON for alignment
#$BASE_CMD -l trge -e econ${ECON}_e${MOD} -f configs/econ${ECON}_init_cpp.yaml -w 1
if [ "$USE_FIVE" = true ]; then
    CONFIG_INIT="configs/econ${ECON}_init_cpp_Five.yaml"
else
    CONFIG_INIT="configs/econ${ECON}_init_cpp.yaml"
fi
echo $CONFIG_INIT
$BASE_CMD -l trg${SIDE} -e econ${ECON}_${SIDE}${MOD} -f $CONFIG_INIT -w 1

# sets when snapshot is going to be taken
python swamp/econ_control.py -s ${SIDE}${MOD} -a $ECON_ADDR -rw write -pname Aligner.Global.orbsyn_cnt_snapshot -v $ORBSYNCSNAPSHOT
# read orbsyncnt val and snapshot
python swamp/econ_control.py -s ${SIDE}${MOD} -a $ECON_ADDR -rw read -pname Aligner.Global.orbsyn_cnt_load_val
#python swamp/econ_control.py -s e${MOD} -a $ECON_ADDR -rw read -pname Aligner.Global.orbsyn_cnt_snapshot

echo "=====ECON configuration===="
# read ECON configuration
$BASE_CMD -l trg${SIDE} -e econ${ECON}_${SIDE}${MOD} -f configs/econ${ECON}_init_cpp.yaml -w 0
#$BASE_CMD -l trge -e econ${ECON}_e${MOD} -f $CONFIG_INIT -w 0

# sends link reset ROC
echo "Send link_reset_roc${ECON} at BX:"
if [ "$ECON" = "D" ]; then
    BX_LR=3516
else
    BX_LR=3517
fi

./uhal_backend_v3.py -b Housekeeping-FastCommands-fastcontrol-axi-0 --node bx_link_reset_roc${ECON} --val ${BX_LR}
./uhal_backend_v3.py --send link_reset_roc${ECON}

# reads the snapshot (This is useful because it allows you to see the ROC-D pattern in each eRx)
python swamp/econ_control.py -s ${SIDE}${MOD} -a $ECON_ADDR -rw read -f configs/read_snapshot.yaml

# read the alignment status
$BASE_CMD -l trg${SIDE} -e econ${ECON}_${SIDE}${MOD} -f configs/check_econd_roc_alignment.yaml -w 0

