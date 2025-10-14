#!/bin/bash

#Get side and  module:
SIDE=$1 #e or w
MOD=$2
#Get config file:
CONFIG=$3

USE_FIVE=false

# Define base command for reuse
BASE_CMD="/opt/swamp/bin/roc_control -F hwcfg_uva_partials.yaml"

if [[ "$1" == "--five" || "$1" == "-f" ]]; then
    USE_FIVE=true
    shift
fi
echo "USE_FIVE = $USE_FIVE"

# Configure with YAML
for IROC in {0..2}; do
    if [[ "$MOD" -gt 0 && "$IROC" -eq 2 && "$USE_FIVE" == false ]]; then
        continue
    fi

    echo "/------Starting configuration: MODULE ${MOD} ROC ${IROC}------/"
    $BASE_CMD -l trg${SIDE} -r roc${IROC}_${SIDE}${MOD} -f configs/$CONFIG -w 1
    sleep 1
    #echo "/--------State after configuration: ${MOD} ROC ${IROC}--------/"
    $BASE_CMD -l trg${SIDE} -r roc${IROC}_${SIDE}${MOD} -f configs/$CONFIG -w 0 -c 1
done
