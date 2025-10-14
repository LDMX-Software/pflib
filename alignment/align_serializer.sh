#!/bin/bash

HWCFG="hwcfg_uva_partials.yaml"
ECON=$1 #d or t
SIDE=$2 #e or w
MOD=$3 #0 1 or 2

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

echo "ECON address " $ECON_ADDR

# Read FCMD status
python swamp/econ_control.py -s ${SIDE}$MOD -a $ECON_ADDR -pname FCtrl.Global.command_rx_inverted -rw read

# Write ECON configuration
echo "Writing ${SIDE}${MOD}_econ${ECON}_align_etx_cpp.yaml"
#/opt/swamp/bin/econ_control -F hwcfg_uva_partials.yaml  -l trg${SIDE} -e econ${ECON}_${SIDE}${MOD} -f configs/${SIDE}${MOD}_econ${ECON}_align_etx_cpp.yaml -w 1
# Read back
#/opt/swamp/bin/econ_control -F hwcfg_uva_partials.yaml  -l trg${SIDE} -e econ${ECON}_${SIDE}${MOD} -f configs/${SIDE}${MOD}_econ${ECON}_align_etx_cpp.yaml -w 0

python swamp/econ_control.py -s ${SIDE}$MOD -a $ECON_ADDR -f configs/${SIDE}${MOD}_econ${ECON}_align_etx_cpp.yaml -rw write
python swamp/econ_control.py -s ${SIDE}$MOD -a $ECON_ADDR -f configs/${SIDE}${MOD}_econ${ECON}_align_etx_cpp.yaml -rw read

# Configure link capture
if [[ "$ECON" == "d" ]]; then
    echo "Sending linkreset ECONd"
    ./configLCs.py --lc DAQ --linkreset
elif [[ "$ECON" == "t" ]]; then
    echo "Sending linkreset ECONt"
    ./configLCs.py --lc trig --linkreset
else
    echo "Unknown ECON value: $ECON"
fi

# Trigger capture
./uhal_backend_v3.py --send link_reset_econ${ECON}

# Dump FIFO
if [[ "$ECON" == "d" ]]; then
    echo "Reading DAQ FIFO:"
    #./uhal_backend_v3.py --fifo | grep -E "^[[:space:]]*[0-9]+|link [0-9]"
    #./uhal_backend_v3.py --fifo | tail -n 3
    ./uhal_backend_v3.py --fifo
elif [[ "$ECON" == "t" ]]; then
    # need to add --trig to commands
    echo "Reading TRG FIFO:"
    # ./uhal_backend_v3.py --status --trig
    #./uhal_backend_v3.py --fifo --trig
    ./uhal_backend_v3.py --fifo --trig | tail -n +30 | head -n 100
else
    echo "Unknown ECON value: $ECON"
fi
