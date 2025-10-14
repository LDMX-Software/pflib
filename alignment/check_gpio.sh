#!/bin/bash

# Define base command for reuse
BASE_CMD="/opt/swamp/bin/lpgbt_gpio_control -F hwcfg_uva_partials.yaml"

# Check east 0 with 1-second delay
echo "===================================="
echo "          Checking East 0"
echo "===================================="
$BASE_CMD -l trge -g pwr_en_e0 -w 0
sleep 1

$BASE_CMD -l trge -g hgcroc_re_hb_e0 -w 0
sleep 1

$BASE_CMD -l trge -g hgcroc_re_sb_e0 -w 0
sleep 1

$BASE_CMD -l trge -g econ_re_hb_e0 -w 0
sleep 1

$BASE_CMD -l trge -g econ_re_sb_e0 -w 0

# Check east1 with 1-second delay
echo "===================================="
echo "          Checking East 1"
echo "===================================="
$BASE_CMD -l trge -g pwr_en_e1 -w 0
sleep 1

$BASE_CMD -l trge -g hgcroc_re_hb_e1 -w 0
sleep 1

$BASE_CMD -l trge -g hgcroc_re_sb_e1 -w 0
sleep 1

$BASE_CMD -l trge -g econ_re_hb_e1 -w 0
sleep 1

$BASE_CMD -l trge -g econ_re_sb_e1 -w 0

# Check east2 with 1-second delay
echo "===================================="
echo "          Checking East 2"
echo "===================================="
$BASE_CMD -l trge -g pwr_en_e2 -w 0
sleep 1

$BASE_CMD -l trge -g hgcroc_re_hb_e2 -w 0
sleep 1

$BASE_CMD -l trge -g hgcroc_re_sb_e2 -w 0
sleep 1

$BASE_CMD -l trge -g econ_re_hb_e2 -w 0
sleep 1

$BASE_CMD -l trge -g econ_re_sb_e2 -w 0


for MOD in 0 1 2; do
    echo "===================================="
    echo "          Checking West ${MOD}"
    echo "===================================="

    $BASE_CMD -l trgw -g pwr_en_w${MOD} -w 0
    sleep 1

    $BASE_CMD -l trgw -g hgcroc_re_hb_w${MOD} -w 0
    sleep 1

    $BASE_CMD -l trgw -g hgcroc_re_sb_w${MOD} -w 0
    sleep 1

    $BASE_CMD -l trgw -g econ_re_hb_w${MOD} -w 0
    sleep 1

    $BASE_CMD -l trgw -g econ_re_sb_w${MOD} -w 0
    sleep 1
done
