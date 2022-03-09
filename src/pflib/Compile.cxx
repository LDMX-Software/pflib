#include "pflib/Compile.h"
#include "pflib/Exception.h"

#include <map>
#include <iostream>
#include <algorithm>
#include <strings.h>

#include <yaml-cpp/yaml.h>
#include <algorithm>

namespace pflib {

/**
 * Structure holding a location in the registers
 */
struct RegisterLocation {
  /// the register the parameter is in (0-15)
  const int reg;
  /// the min bit the location is in the register (0-7)
  const int min_bit;
  /// the number of bits the location has in the register (1-8)
  const int n_bits;
  /// the mask for this number of bits
  const int mask;
  /**
   * Usable constructor
   *
   * This constructor allows us to calculat the mask from the
   * number of bits so that the downstream compilation functions
   * don't have to calculate it themselves.
   */
  RegisterLocation(int r, int m, int n)
    : reg{r}, min_bit{m}, n_bits{n},
      mask{((1 << n_bits) - 1)} {}
};

/**
 * A parameter for the HGC ROC includes one or more register locations
 * and a default value defined in the manual.
 */
struct Parameter {
  /// the default parameter value
  const int def;
  /// the locations that the parameter is split over
  const std::vector<RegisterLocation> registers;
  /// pass locations and default value of parameter
  Parameter(std::initializer_list<RegisterLocation> r,int def)
    : def{def}, registers{r} {}
  /// short constructor for single-location parameters
  Parameter(int r, int m, int n, int def)
    : Parameter({RegisterLocation(r,m,n)},def) {}
};

/**
 * The Look Up Table of for the Global Analog sub-blocks
 * of an HGC ROC
 */
const std::map<std::string,Parameter>
GLOBAL_ANALOG_LUT = {
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
};

/**
 * The Look Up Table of for the Reference Voltage sub-blocks
 * of an HGC ROC
 */
const std::map<std::string,Parameter>
REFERENCE_VOLTAGE_LUT = {
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
};

/**
 * The Look Up Table of for the Master TDC sub-blocks`
 * of an HGC ROC
 */
const std::map<std::string,Parameter>
MASTER_TDC_LUT = {
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
};

/**
 * The Look Up Table of for the individual channel sub-blocks`
 * of an HGC ROC
 */
const std::map<std::string,Parameter> CHANNEL_WISE_LUT = {
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
};

/**
 * The Look Up Table of for the Digital Half sub-blocks`
 * of an HGC ROC
 */
const std::map<std::string,Parameter> DIGITAL_HALF_LUT = {
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
};

/**
 * The Look Up Table of for the Top sub-block
 * of an HGC ROC
 */
const std::map<std::string,Parameter> TOP_LUT = {
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
};

/**
 * Entire parameter Look Up Table.
 */
const std::map<std::string,std::pair<int,std::map<std::string,Parameter>>>
PARAMETER_LUT = {
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
};

int str_to_int(std::string str) {
  if (str == "0") return 0;
  int base = 10;
  if (str[0] == '0' and str.length() > 2) {
    // binary or hexadecimal
    if (str[1] == 'b' or str[1] == 'B') {
      base = 2;
      str = str.substr(2);
    } else if (str[1] == 'x' or str[1] == 'X') {
      base = 16;
      str = str.substr(2);
    } 
  } else if (str[0] == '0' and str.length() > 1) {
    // octal
    base = 8;
    str = str.substr(1);
  }

  return std::stoi(str,nullptr,base);
}

std::string upper_cp(const std::string& str) {
  std::string STR{str};
  for (auto& c : STR) c = toupper(c);
  return STR;
}

void compile(const std::string& page_name, const std::string& param_name, const int& val, 
    std::map<int,std::map<int,uint8_t>>& register_values) {
  const auto& page_id {PARAMETER_LUT.at(page_name).first};
  const Parameter& spec{PARAMETER_LUT.at(page_name).second.at(param_name)};
  std::size_t value_curr_min_bit{0};
  for (const RegisterLocation& location : spec.registers) {
    // grab sub value of parameter in this register
    uint8_t sub_val = ((val >> value_curr_min_bit) & location.mask);
    value_curr_min_bit += location.n_bits;
    if (register_values[page_id].find(location.reg) == register_values[page_id].end()) {
      // initialize register value to zero if it hasn't been touched before
      register_values[page_id][location.reg] = 0;
    } else {
      // make sure to clear old value
      register_values[page_id][location.reg] &= ((location.mask << location.min_bit) ^ 0b11111111);
    }
    // put value into register at the specified location
    register_values[page_id][location.reg] += (sub_val << location.min_bit);
  } // loop over register locations
  return;
}

std::map<int,std::map<int,uint8_t>> 
compile(const std::string& page_name, const std::string& param_name, const int& val) {
  std::string PAGE_NAME(upper_cp(page_name)), PARAM_NAME(upper_cp(param_name));
  if (PARAMETER_LUT.find(PAGE_NAME) == PARAMETER_LUT.end()) {
    PFEXCEPTION_RAISE("NotFound", "The page named '"+PAGE_NAME+"' is not found in the look up table.");
  }
  const auto& page_lut{PARAMETER_LUT.at(PAGE_NAME).second};
  if (page_lut.find(PARAM_NAME) == page_lut.end()) {
    PFEXCEPTION_RAISE("NotFound", "The parameter named '"+PARAM_NAME
        +"' is not found in the look up table for page "+PAGE_NAME);
  }
  std::map<int,std::map<int,uint8_t>> rv;
  compile(PAGE_NAME, PARAM_NAME, val, rv);
  return rv;
}

std::map<int,std::map<int,uint8_t>> 
compile(const std::map<std::string,std::map<std::string,int>>& settings) {
  std::map<int,std::map<int,uint8_t>> register_values;
  for (const auto& page : settings) {
    // page.first => page name
    // page.second => parameter to value map
    std::string page_name = upper_cp(page.first);
    if (PARAMETER_LUT.find(page_name) == PARAMETER_LUT.end()) {
      // this exception shouldn't really ever happen because we check if the input
      // page matches any of the pages in the LUT in detail::apply, but
      // we leave this check in here for future development
      PFEXCEPTION_RAISE("NotFound", "The page named '"+page.first+"' is not found in the look up table.");
    }
    const auto& page_lut{PARAMETER_LUT.at(page_name).second};
    for (const auto& param : page.second) {
      // param.first => parameter name
      // param.second => value
      std::string param_name = upper_cp(param.first);
      if (page_lut.find(param_name) == page_lut.end()) {
        PFEXCEPTION_RAISE("NotFound", "The parameter named '"+param.first 
            +"' is not found in the look up table for page "+page.first);
      }
      compile(page_name, param_name, param.second, register_values);
    }   // loop over parameters in page
  }     // loop over pages

  return register_values;
}

std::map<std::string,std::map<std::string,int>>
decompile(const std::map<int,std::map<int,uint8_t>>& compiled_config, bool be_careful) {
  std::map<std::string,std::map<std::string,int>> settings;
  for (const auto& page : PARAMETER_LUT) {
    const std::string& page_name{page.first};
    const int& page_id{page.second.first};
    const auto& page_lut{page.second.second};
    if (compiled_config.find(page_id) == compiled_config.end()) {
      if (be_careful) {
        std::cerr << "WARNING: Page named " << page_name
          << " wasn't provided the necessary page " << page_id 
          << " to be deduced." << std::endl;
      }
      continue;
    }
    const auto& page_conf{compiled_config.at(page_id)};
    for (const auto& param : page_lut) {
      const Parameter& spec{page_lut.at(param.first)};
      std::size_t value_curr_min_bit{0};
      int pval{0};
      int n_missing_regs{0};
      for (const RegisterLocation& location : spec.registers) {
        uint8_t sub_val = 0; // defaults ot zero if not careful and register not found
        if (page_conf.find(location.reg) == page_conf.end()) {
          n_missing_regs++;
          if (be_careful) break;
        } else {
          // grab sub value of parameter in this register
          sub_val = ((page_conf.at(location.reg) >> location.min_bit) & location.mask);
        }
        pval += (sub_val << value_curr_min_bit);
        value_curr_min_bit += location.n_bits;
      }

      if (n_missing_regs == spec.registers.size() or (be_careful and n_missing_regs > 0)) {
        // skip this parameter
        if (be_careful) {
          std::cerr << "WARNING: Parameter " << param.first << " in page " << page_name
            << " wasn't provided the necessary registers to be deduced." << std::endl;
        }
      } else {
        settings[page_name][param.first] = pval;
      }
    }
  }

  return settings;
}

std::vector<std::string> parameters(const std::string& page) {
  static auto get_parameter_names = [&](const std::map<std::string,Parameter>& lut) -> std::vector<std::string> {
    std::vector<std::string> names;
    for (const auto& param : lut) names.push_back(param.first);
    return names;
  };

  std::string PAGE{upper_cp(page)};
  if (PAGE == "DIGITALHALF") {
    return get_parameter_names(DIGITAL_HALF_LUT);
  } else if (PAGE == "CHANNELWISE") {
    return get_parameter_names(CHANNEL_WISE_LUT);
  } else if (PAGE == "TOP" ) {
    return get_parameter_names(TOP_LUT);
  } else if (PAGE == "MASTERTDC") {
    return get_parameter_names(MASTER_TDC_LUT);
  } else if (PAGE == "REFERENCEVOLTAGE") {
    return get_parameter_names(REFERENCE_VOLTAGE_LUT);
  } else if (PAGE == "GLOBALANALOG") {
    return get_parameter_names(GLOBAL_ANALOG_LUT);
  } else if (PARAMETER_LUT.find(PAGE) != PARAMETER_LUT.end()) {
    return get_parameter_names(PARAMETER_LUT.at(PAGE).second);
  } else {
    PFEXCEPTION_RAISE("BadName", 
        "Input page name "+page+" does not match a page or type of page.");
  }
}

std::map<std::string,std::map<std::string,int>> defaults() {
  std::map<std::string,std::map<std::string,int>> settings;
  for(auto& page : PARAMETER_LUT) {
    for (auto& param : page.second.second) {
      settings[page.first][param.first] = param.second.def;
    }
  }
  return settings;
}

/**
 * Hidden functions to avoid users using them
 */
namespace detail {

/**
 * Extract a map of page_name, param_name to their values by crawling the YAML tree.
 *
 * @throw pflib::Exception if YAML file has a bad format (root node is not a map,
 * page nodes are not maps, page name doesn't match any pages, or parameter's value
 * does not exist).
 *
 * @param[in] params a YAML::Node to start extraction from
 * @param[in,out] settings map of names to values for extract parameters
 */
void extract(YAML::Node params, std::map<std::string,std::map<std::string,int>>& settings) {
  // deduce list of page names for search
  //    only do this once per program run
  static std::vector<std::string> page_names;
  if (page_names.empty()) {
    for (auto& page : PARAMETER_LUT) page_names.push_back(page.first);
  }

  if (not params.IsMap()) {
    PFEXCEPTION_RAISE("BadFormat", "The YAML node provided is not a map.");
  }

  for (const auto& page_pair : params) {
    std::string page_name = page_pair.first.as<std::string>();
    YAML::Node page_settings = page_pair.second;
    if (not page_settings.IsMap()) {
      PFEXCEPTION_RAISE("BadFormat", "The YAML node for a page "+page_name+" is not a map.");
    }
    page_name = page_name.substr(0,page_name.find('*'));

    // apply these settings only to pages the input name
    std::vector<std::string> matching_pages;
    std::copy_if(page_names.begin(), page_names.end(), std::back_inserter(matching_pages),
        [&](const std::string& page) { 
          return strncasecmp(page.c_str(), page_name.c_str(), page_name.size()) == 0; 
        });

    if (matching_pages.empty()) {
      PFEXCEPTION_RAISE("NotFound", 
          "The page "+page_name+" does not match any pages in the look up table.");
    }

    for (const auto& page : matching_pages) {
      for (const auto& param : page_settings) {
        std::string sval = param.second.as<std::string>();
        if (sval.empty()) {
          PFEXCEPTION_RAISE("BadFormat",
              "Non-existent value for parameter "+param.first.as<std::string>());
        }
        std::string param_name = upper_cp(param.first.as<std::string>());
        settings[page][param_name] = str_to_int(sval);
      }
    }
  }
}

} // namespace detail

void extract(const std::vector<std::string>& setting_files,
    std::map<std::string,std::map<std::string,int>>& settings) {
  for (auto& setting_file : setting_files) {
    YAML::Node setting_yaml;
    try {
       setting_yaml = YAML::LoadFile(setting_file);
    } catch (const YAML::BadFile& e) {
      PFEXCEPTION_RAISE("BadFile","Unable to load file " + setting_file);
    }
    if (setting_yaml.IsSequence()) {
      for (std::size_t i{0}; i < setting_yaml.size(); i++) detail::extract(setting_yaml[i],settings);
    } else {
      detail::extract(setting_yaml, settings);
    }
  }
}

std::map<int,std::map<int,uint8_t>> 
compile(const std::vector<std::string>& setting_files, bool prepend_defaults) {
  std::map<std::string,std::map<std::string,int>> settings;
  // if we prepend the defaults, put all settings and their defaults 
  // into the settings map before extraction
  if (prepend_defaults) { settings = defaults(); }
  extract(setting_files,settings);
  return compile(settings);
}

std::map<int,std::map<int,uint8_t>> 
compile(const std::string& setting_file, bool prepend_defaults) {
  return compile(std::vector<std::string>{setting_file}, prepend_defaults);
}
}
