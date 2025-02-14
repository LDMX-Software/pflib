/**
 * @file sipm_rocv2.h
 *
 * These register maps (or Look-Up Tables -- LUTs) were manually written
 * using the HGCROC v2 manual as a reference.
 * The other register maps for later versions of the HGCROC are generated
 * using a python script which parses a YAML file that is more easily
 * compared to the manual and is obtained from CMS folks.
 */

#include "register_maps/register_maps_types.h"

namespace sipm_rocv2 {

/**
 * The Look Up Table of for the Global Analog sub-blocks
 * of an HGC ROC
 */
const Page GLOBAL_ANALOG_LUT = Page::Mapping({
  {"ON_DAC_TRIM", Parameter(0,0,1,1)},
  {"ON_INPUT_DAC", Parameter(0,1,1,1)},
  {"ON_CONV", Parameter(0,2,1,1)},
  {"ON_PA", Parameter(0,3,1,1)},
  {"GAIN_CONV", Parameter(0,4,4,0b0100)},
  {"ON_RTR", Parameter(1,0,1,1)},
  {"SW_SUPER_CONV", Parameter(1,1,1,1)},
  {"DACB_VB_CONV", Parameter(1,2,6,0b110011)},
  {"ON_TOA", Parameter(2,0,1,1)},
  {"ON_TOT", Parameter(2,1,1,1)},
  {"DACB_VBI_PA", Parameter(2,2,6,0b011111)},
  {"IBI_SK", Parameter(3,0,2,0)},
  {"IBO_SK", Parameter(3,2,6,0b001010)},
  {"IBI_INV", Parameter(4,0,2,0)},
  {"IBO_INV", Parameter(4,2,6,0b001010)},
  {"IBI_NOINV", Parameter(5,0,2,0)},
  {"IBO_NOINV", Parameter(5,2,6,0b001010)},
  {"IBI_INV_BUF", Parameter(6,0,2,0b11)},
  {"IBO_INV_BUF", Parameter(6,2,6,0b100110)},
  {"IBI_NOINV_BUF", Parameter(7,0,2,0b11)},
  {"IBO_NOINV_BUF", Parameter(7,2,6,0b100110)},
  {"SW_CD", Parameter(8,0,3,0b111)},
  {"EN_HYST_TOT", Parameter(8,3,1,0)},
  {"SW_CF_COMP", Parameter(8,4,4,0b1010)},
  {"SW_CF", Parameter(9,0,4,0b1010)},
  {"SW_RF", Parameter(9,4,4,0b1000)},
  {"CLR_SHAPERTAIL", Parameter(10,1,1,0)},
  {"SELRISINGEDGE", Parameter(10,2,1,1)},
  {"SELEXTADC", Parameter(10,3,1,0)},
  {"CLR_ADC", Parameter(10,4,1,0)},
  {"S_SK", Parameter(10,5,3,0b010)},
  {"S_INV", Parameter(11,2,3,0b010)},
  {"S_NOINV", Parameter(11,5,3,0b010)},
  {"S_INV_BUF", Parameter(12,2,3,0b110)},
  {"S_NOINV_BUF", Parameter(12,5,3,0b110)},
  {"REF_ADC", Parameter(13,0,2,0)},
  {"DELAY40", Parameter(13,2,3,0)},
  {"DELAY65", Parameter(13,5,3,0)},
  {"ON_REF_ADC", Parameter(14,0,1,1)},
  {"POL_ADC", Parameter(14,1,1,1)},
  {"DELAY87", Parameter(14,2,3,0)},
  {"DELAY9", Parameter(14,5,3,0)}
});

/**
 * The Look Up Table of for the Reference Voltage sub-blocks
 * of an HGC ROC
 */
const Page REFERENCE_VOLTAGE_LUT = Page::Mapping({
  {"PROBE_VREF_PA", Parameter(0,0,1,0)},
  {"PROBE_VREF_TIME", Parameter(0,1,1,0)},
  {"REFI", Parameter(0,2,2,0b11)},
  {"VBG_1V", Parameter(0,4,3,0b111)},
  {"ON_DAC", Parameter(0,7,1,1)},
  {"NOINV_VREF", Parameter({RegisterLocation(1,0,2),RegisterLocation(5,0,8)},0b0100111100)},
  {"INV_VREF", Parameter({RegisterLocation(1,2,2),RegisterLocation(4,0,8)},0b0110000000)},
  {"TOA_VREF", Parameter({RegisterLocation(1,4,2),RegisterLocation(3,0,8)},0b0001110000)},
  {"TOT_VREF", Parameter({RegisterLocation(1,6,2),RegisterLocation(2,0,8)},0b0110110000)},
  {"CALIB_DAC", Parameter({RegisterLocation(6,0,8),RegisterLocation(7,0,4)},0)},
  {"INTCTEST", Parameter(7,6,1,0)},
  {"EXTCTEST", Parameter(7,7,1,0)},
  {"PROBE_DC", Parameter(8,0,8,0)}
});

/**
 * The Look Up Table of for the Master TDC sub-blocks`
 * of an HGC ROC
 */
const Page MASTER_TDC_LUT = Page::Mapping({
  {"GLOBAL_TA_SELECT_GAIN_TOA", Parameter(0,0,4,0b0011)},
  {"GLOBAL_TA_SELECT_GAIN_TOT", Parameter(0,4,4,0b0011)},
  {"GLOBAL_MODE_NO_TOT_SUB", Parameter(1,0,1,0)},
  {"GLOBAL_LATENCY_TIME", Parameter(1,1,4,0b1010)},
  {"GLOBAL_MODE_FTDC_TOA_S0", Parameter(1,5,1,0)},
  {"GLOBAL_MODE_FTDC_TOA_S1", Parameter(1,6,1,1)},
  {"GLOBAL_SEU_TIME_OUT", Parameter(1,7,1,1)},
  {"BIAS_FOLLOWER_CAL_P_D", Parameter(2,0,4,0)},
  {"BIAS_FOLLOWER_CAL_P_EN", Parameter(2,4,1,0)},
  {"INV_FRONT_40MHZ", Parameter(2,5,1,0)},
  {"START_COUNTER", Parameter(2,6,1,1)},
  {"CALIB_CHANNEL_DLL", Parameter(2,7,1,0)},
  {"VD_CTDC_P_D", Parameter(3,0,5,0)},
  {"VD_CTDC_P_DAC_EN", Parameter(3,5,1,0)},
  {"EN_MASTER_CTDC_VOUT_INIT", Parameter(3,6,1,0)},
  {"EN_MASTER_CTDC_DLL", Parameter(3,7,1,1)},
  {"BIAS_CAL_DAC_CTDC_P_D", Parameter({RegisterLocation(4,0,2),RegisterLocation(7,6,2)},0b0000)},
  {"CTDC_CALIB_FREQUENCY", Parameter(4,2,6,0b000010)},
  {"GLOBAL_MODE_TOA_DIRECT_OUTPUT", Parameter(5,0,1,0)},
  {"BIAS_I_CTDC_D", Parameter(5,1,6,0b011000)},
  {"FOLLOWER_CTDC_EN", Parameter(5,7,1,1)},
  {"GLOBAL_EN_BUFFER_CTDC", Parameter(6,0,1,0)},
  {"VD_CTDC_N_FORCE_MAX", Parameter(6,1,1,1)},
  {"VD_CTDC_N_D", Parameter(6,2,5,0)},
  {"VD_CTDC_N_DAC_EN", Parameter(6,7,1,0)},
  {"CTRL_IN_REF_CTDC_P_D", Parameter(7,0,5,0)},
  {"CTRL_IN_REF_CTDC_P_EN", Parameter(7,5,1,0)},
  {"CTRL_IN_SIG_CTDC_P_D", Parameter(8,0,5,0)},
  {"CTRL_IN_SIG_CTDC_P_EN", Parameter(8,5,1,0)},
  {"GLOBAL_INIT_DAC_B_CTDC", Parameter(8,6,1,0)},
  {"BIAS_CAL_DAC_CTDC_P_EN", Parameter(8,7,1,0)},
  {"VD_FTDC_P_D", Parameter(9,0,5,0)},
  {"VD_FTDC_P_DAC_EN", Parameter(9,5,1,0)},
  {"EN_MASTER_FTDC_VOUT_INIT", Parameter(9,6,1,0)},
  {"EN_MASTER_FTDC_DLL", Parameter(9,7,1,1)},
  {"BIAS_CAL_DAC_FTDC_P_D", Parameter({RegisterLocation(10,0,2),RegisterLocation(14,6,2)},0b0000)},
  {"FTDC_CALIB_FREQUENCY", Parameter(10,2,6,0b000010)},
  {"EN_REF_BG", Parameter(11,0,1,1)},
  {"BIAS_I_FTDC_D", Parameter(11,1,6,0b011000)},
  {"FOLLOWER_FTDC_EN", Parameter(11,7,1,1)},
  {"GLOBAL_EN_BUFFER_FTDC", Parameter(12,0,1,0)},
  {"VD_FTDC_N_FORCE_MAX", Parameter(12,1,1,1)},
  {"VD_FTDC_N_D", Parameter(12,2,5,0)},
  {"VD_FTDC_N_DAC_EN", Parameter(12,7,1,0)},
  {"CTRL_IN_SIG_FTDC_P_D", Parameter(13,0,5,0)},
  {"CTRL_IN_SIG_FTDC_P_EN", Parameter(13,5,1,0)},
  {"GLOBAL_INIT_DAC_B_FTDC", Parameter(13,6,1,0)},
  {"BIAS_CAL_DAC_FTDC_P_EN", Parameter(13,7,1,0)},
  {"CTRL_IN_REF_FTDC_P_D", Parameter(14,0,5,0)},
  {"CTRL_IN_REF_FTDC_P_EN", Parameter(14,5,1,0)},
  {"GLOBAL_DISABLE_TOT_LIMIT", Parameter(15,0,1,0)},
  {"GLOBAL_FORCE_EN_CLK", Parameter(15,1,1,0)},
  {"GLOBAL_FORCE_EN_OUTPUT_DATA", Parameter(15,2,1,0)},
  {"GLOBAL_FORCE_EN_TOT", Parameter(15,3,1,0)}
});

/**
 * The Look Up Table of for the individual channel sub-blocks`
 * of an HGC ROC
 */
const Page CHANNEL_WISE_LUT = Page::Mapping({
  {"INPUTDAC", Parameter(0,0,6,0b011111)},
  {"DACB", Parameter({RegisterLocation(2,6,2),RegisterLocation(1,6,2),RegisterLocation(0,6,2)},0b111111)},
  {"SIGN_DAC", Parameter(1,0,1,0)},
  {"REF_DAC_TOA", Parameter(1,1,5,0)},
  {"PROBE_NOINV", Parameter(2,0,1,0)},
  {"REF_DAC_TOT", Parameter(2,1,5,0)},
  {"MASK_TOA", Parameter(3,0,1,0)},
  {"REF_DAC_INV", Parameter(3,1,5,0)},
  {"SEL_TRIGGER_TOA", Parameter(3,6,1,0)},
  {"PROBE_INV", Parameter(3,7,1,0)},
  {"PROBE_PA", Parameter(4,0,1,0)},
  {"LOWRANGE", Parameter(4,1,1,0)},
  {"HIGHRANGE", Parameter(4,2,1,0)},
  {"CHANNEL_OFF", Parameter(4,3,1,0)},
  {"SEL_TRIGGER_TOT", Parameter(4,4,1,0)},
  {"MASK_TOT", Parameter(4,5,1,0)},
  {"PROBE_TOT", Parameter(4,6,1,0)},
  {"PROBE_TOA", Parameter(4,7,1,0)},
  {"DAC_CAL_FTDC_TOA", Parameter(5,0,6,0)},
  {"MASK_ADC", Parameter(5,7,1,0)},
  {"DAC_CAL_CTDC_TOA", Parameter(6,0,6,0)},
  {"DAC_CAL_FTDC_TOT", Parameter(7,0,6,0)},
  {"DAC_CAL_CTDC_TOT", Parameter(8,0,6,0)},
  {"IN_FTDC_ENCODER_TOA", Parameter(9,0,6,0)},
  {"IN_FTDC_ENCODER_TOT", Parameter(10,0,6,0)},
  {"DIS_TDC", Parameter(10,7,1,0)},
  {"EXTDATA", Parameter({RegisterLocation(13,0,8),RegisterLocation(11,0,2)},0)},
  {"MASK_ALIGNBUFFER", Parameter(11,7,1,0)},
  {"ADC_PEDESTAL", Parameter(12,0,8,0)}
});

/**
 * The Look Up Table of for the Digital Half sub-blocks`
 * of an HGC ROC
 */
const Page DIGITAL_HALF_LUT = Page::Mapping({
  {"SELRAWDATA", Parameter(0,0,1,1)},
  {"SELTC4"    , Parameter(0,1,1,1)},
  {"CMDSELEDGE", Parameter(0,2,1,1)},
  {"ADC_TH"    , Parameter(0,4,4,0)},
  {"MULTFACTOR", Parameter(1,0,5,0b11001)},
  {"L1OFFSET"  , Parameter({RegisterLocation(2,0,8),RegisterLocation(1,7,1)},0b000001000)},
  {"IDLEFRAME" , Parameter({RegisterLocation(3,0,8),RegisterLocation(4,0,8),RegisterLocation(5,0,8),RegisterLocation(6,0,4)},0b1100110011001100110011001100)},
  {"BYPASSCH0" , Parameter(6,4,1,0)},
  {"BYPASSCH17", Parameter(6,5,1,0)},
  {"BYPASSCH35", Parameter(6,6,1,0)},
  {"TOT_TH0"   , Parameter(7,0,8,0)},
  {"TOT_TH1"   , Parameter(8,0,8,0)},
  {"TOT_TH2"   , Parameter(9,0,8,0)},
  {"TOT_TH3"   , Parameter(10,0,8,0)},
  {"TOT_P0"    , Parameter(11,0,7,0)},
  {"TOT_P1"    , Parameter(12,0,7,0)},
  {"TOT_P2"    , Parameter(13,0,7,0)},
  {"TOT_P3"    , Parameter(14,0,7,0)}
});

/**
 * The Look Up Table of for the Top sub-block
 * of an HGC ROC
 */
const Page TOP_LUT = Page::Mapping({
  {"EN_LOCK_CONTROL", Parameter(0,0,1,1)},
  {"ERROR_LIMIT_SC" , Parameter(0,1,3,0b010)},
  {"SEL_PLL_LOCKED" , Parameter(0,4,1,1)},
  {"PLLLOCKEDSC"    , Parameter(0,5,1,1)},
  {"ORBITSYNC_SC"   , Parameter(0,7,1,0)},
  {"EN_PLL"         , Parameter(1,0,1,1)},
  {"DIV_PLL"        , Parameter(1,1,2,0)},
  {"EN_HIGH_CAPA"   , Parameter(1,3,1,0)},
  {"EN_REF_BG"      , Parameter(1,4,1,1)},
  {"VOUT_INIT_EN"   , Parameter(1,5,1,0)},
  {"VOUT_INIT_EXT_EN", Parameter(2,0,1,0)},
  {"VOUT_INIT_EXT_D" , Parameter(2,1,5,0)},
  {"FOLLOWER_PLL_EN" , Parameter(3,0,1,1)},
  {"BIAS_I_PLL_D"    , Parameter(3,1,6,0b011000)},
  {"SEL_40M_EXT"     , Parameter(3,7,1,0)},
  {"PLL_PROBE_AMPLITUDE" , Parameter(4,0,3,0b011)},
  {"PLL_PROBE_PRE_SCALE" , Parameter(4,3,3,0)},
  {"PLL_PROBE_PRE_PHASE" , Parameter(4,6,2,0)},
  {"ET_AMPLITUDE"        , Parameter(5,0,3,0b011)},
  {"ET_PRE_SCALE"        , Parameter(5,3,3,0)},
  {"ET_PRE_PHASE"        , Parameter(5,6,2,0)},
  {"SEL_RESYNC_FCMD" , Parameter(6,0,1,1)},
  {"SEL_L1_FCMD"     , Parameter(6,1,1,1)},
  {"SEL_STROBE_FCMD" , Parameter(6,2,1,1)},
  {"SEL_ORBITSYNC_FCMD", Parameter(6,3,1,1)},
  {"EN_PHASESHIFT" , Parameter(7,0,1,1)},
  {"PHASE"         , Parameter(7,1,4,0)},
  {"EN_PLL_EXT"    , Parameter(7,7,1,0)}
});

const PageLUT PAGE_LUT = PageLUT::Mapping({
  {"GLOBAL_ANALOG", GLOBAL_ANALOG_LUT},
  {"REFERENCE_VOLTAGE", REFERENCE_VOLTAGE_LUT},
  {"MASTER_TDC", MASTER_TDC_LUT},
  {"DIGITAL_HALF", DIGITAL_HALF_LUT},
  {"TOP", TOP_LUT},
  {"CHANNEL_WISE", CHANNEL_WISE_LUT}
});

/**
 * Entire parameter Look Up Table.
 */
const ParameterLUT PARAMETER_LUT = ParameterLUT::Mapping({
  {"GLOBAL_ANALOG_0", { 297, GLOBAL_ANALOG_LUT }},
  {"REFERENCE_VOLTAGE_0", {296, REFERENCE_VOLTAGE_LUT}},
  {"GLOBAL_ANALOG_0", {297, GLOBAL_ANALOG_LUT}},
  {"MASTER_TDC_0", {298, MASTER_TDC_LUT}},
  {"DIGITAL_HALF_0", {299, DIGITAL_HALF_LUT}},
  {"REFERENCE_VOLTAGE_1", {40, REFERENCE_VOLTAGE_LUT}},
  {"GLOBAL_ANALOG_1", {41, GLOBAL_ANALOG_LUT}},
  {"MASTER_TDC_1", {42, MASTER_TDC_LUT}},
  {"DIGITAL_HALF_1", {43, DIGITAL_HALF_LUT}},
  {"TOP", {44,TOP_LUT}},
  {"CM0", {275, CHANNEL_WISE_LUT}},
  {"CM1", {276, CHANNEL_WISE_LUT}},
  {"CALIB0", {274, CHANNEL_WISE_LUT}},
  {"CM2", {19, CHANNEL_WISE_LUT}},
  {"CM3", {20, CHANNEL_WISE_LUT}},
  {"CALIB1", {18, CHANNEL_WISE_LUT}},
  {"CHANNEL_0", {261, CHANNEL_WISE_LUT}},
  {"CHANNEL_1", {260, CHANNEL_WISE_LUT}},
  {"CHANNEL_2", {259, CHANNEL_WISE_LUT}},
  {"CHANNEL_3", {258, CHANNEL_WISE_LUT}},
  {"CHANNEL_4", {265, CHANNEL_WISE_LUT}},
  {"CHANNEL_5", {264, CHANNEL_WISE_LUT}},
  {"CHANNEL_6", {263, CHANNEL_WISE_LUT}},
  {"CHANNEL_7", {262, CHANNEL_WISE_LUT}},
  {"CHANNEL_8", {269, CHANNEL_WISE_LUT}},
  {"CHANNEL_9", {268, CHANNEL_WISE_LUT}},
  {"CHANNEL_10", {267, CHANNEL_WISE_LUT}},
  {"CHANNEL_11", {266, CHANNEL_WISE_LUT}},
  {"CHANNEL_12", {273, CHANNEL_WISE_LUT}},
  {"CHANNEL_13", {272, CHANNEL_WISE_LUT}},
  {"CHANNEL_14", {271, CHANNEL_WISE_LUT}},
  {"CHANNEL_15", {270, CHANNEL_WISE_LUT}},
  {"CHANNEL_16", {294, CHANNEL_WISE_LUT}},
  {"CHANNEL_17", {256, CHANNEL_WISE_LUT}},
  {"CHANNEL_18", {277, CHANNEL_WISE_LUT}},
  {"CHANNEL_19", {295, CHANNEL_WISE_LUT}},
  {"CHANNEL_20", {278, CHANNEL_WISE_LUT}},
  {"CHANNEL_21", {279, CHANNEL_WISE_LUT}},
  {"CHANNEL_22", {280, CHANNEL_WISE_LUT}},
  {"CHANNEL_23", {281, CHANNEL_WISE_LUT}},
  {"CHANNEL_24", {282, CHANNEL_WISE_LUT}},
  {"CHANNEL_25", {283, CHANNEL_WISE_LUT}},
  {"CHANNEL_26", {284, CHANNEL_WISE_LUT}},
  {"CHANNEL_27", {285, CHANNEL_WISE_LUT}},
  {"CHANNEL_28", {286, CHANNEL_WISE_LUT}},
  {"CHANNEL_29", {287, CHANNEL_WISE_LUT}},
  {"CHANNEL_30", {288, CHANNEL_WISE_LUT}},
  {"CHANNEL_31", {289, CHANNEL_WISE_LUT}},
  {"CHANNEL_32", {290, CHANNEL_WISE_LUT}},
  {"CHANNEL_33", {291, CHANNEL_WISE_LUT}},
  {"CHANNEL_34", {292, CHANNEL_WISE_LUT}},
  {"CHANNEL_35", {293, CHANNEL_WISE_LUT}},
  {"CHANNEL_36", {5, CHANNEL_WISE_LUT}},
  {"CHANNEL_37", {4, CHANNEL_WISE_LUT}},
  {"CHANNEL_38", {3, CHANNEL_WISE_LUT}},
  {"CHANNEL_39", {2, CHANNEL_WISE_LUT}},
  {"CHANNEL_40", {9, CHANNEL_WISE_LUT}},
  {"CHANNEL_41", {8, CHANNEL_WISE_LUT}},
  {"CHANNEL_42", {7, CHANNEL_WISE_LUT}},
  {"CHANNEL_43", {6, CHANNEL_WISE_LUT}},
  {"CHANNEL_44", {13, CHANNEL_WISE_LUT}},
  {"CHANNEL_45", {12, CHANNEL_WISE_LUT}},
  {"CHANNEL_46", {11, CHANNEL_WISE_LUT}},
  {"CHANNEL_47", {10, CHANNEL_WISE_LUT}},
  {"CHANNEL_48", {17, CHANNEL_WISE_LUT}},
  {"CHANNEL_49", {16, CHANNEL_WISE_LUT}},
  {"CHANNEL_50", {15, CHANNEL_WISE_LUT}},
  {"CHANNEL_51", {14, CHANNEL_WISE_LUT}},
  {"CHANNEL_52", {38, CHANNEL_WISE_LUT}},
  {"CHANNEL_53", {0, CHANNEL_WISE_LUT}},
  {"CHANNEL_54", {21, CHANNEL_WISE_LUT}},
  {"CHANNEL_55", {39, CHANNEL_WISE_LUT}},
  {"CHANNEL_56", {22, CHANNEL_WISE_LUT}},
  {"CHANNEL_57", {23, CHANNEL_WISE_LUT}},
  {"CHANNEL_58", {24, CHANNEL_WISE_LUT}},
  {"CHANNEL_59", {25, CHANNEL_WISE_LUT}},
  {"CHANNEL_60", {26, CHANNEL_WISE_LUT}},
  {"CHANNEL_61", {27, CHANNEL_WISE_LUT}},
  {"CHANNEL_62", {28, CHANNEL_WISE_LUT}},
  {"CHANNEL_63", {29, CHANNEL_WISE_LUT}},
  {"CHANNEL_64", {30, CHANNEL_WISE_LUT}},
  {"CHANNEL_65", {31, CHANNEL_WISE_LUT}},
  {"CHANNEL_66", {32, CHANNEL_WISE_LUT}},
  {"CHANNEL_67", {33, CHANNEL_WISE_LUT}},
  {"CHANNEL_68", {34, CHANNEL_WISE_LUT}},
  {"CHANNEL_69", {35, CHANNEL_WISE_LUT}},
  {"CHANNEL_70", {36, CHANNEL_WISE_LUT}},
  {"CHANNEL_71", {37, CHANNEL_WISE_LUT}}
});

}
