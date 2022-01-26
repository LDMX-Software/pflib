#include "pflib/compile/Compiler.h"
#include "pflib/Exception.h"

#include <map>
#include <regex>

#include <yaml-cpp/yaml.h>

namespace pflib::compile {

/**
 * Structure holding a location in the registers
 */
struct RegisterLocation {
  /// the register the parameter is in (0-15)
  const short reg;
  /// the min bit the location is in the register (0-7)
  const short min_bit;
  /// the number of bits the location has in the register (1-8)
  const short n_bits;
  /// the mask for this number of bits
  const short mask;
  /**
   * Usable constructor
   */
  RegisterLocation(short r, short m, short n)
    : reg{r}, min_bit{m}, n_bits{n},
      mask{((static_cast<short>(1) << n_bits) - static_cast<short>(1))} {}
};

struct Parameter {
  /// the default parameter value
  const int def;
  /// the locations that the parameter is split over
  const std::vector<RegisterLocation> registers;
  /// pass locations and default value of parameter
  Parameter(std::initializer_list<RegisterLocation> r,int def)
    : def{def}, registers{r} {}
  /// short constructor for single-location parameters
  Parameter(short r, short m, short n, int def)
    : Parameter({RegisterLocation(r,m,n)},def) {}
};

const std::map<std::string,Parameter>
GLOBAL_ANALOG_LUT = {
  {"ON_dac_trim", Parameter(0,0,1,1)},
  {"ON_input_dac", Parameter(0,1,1,1)},
  {"ON_conv", Parameter(0,2,1,1)},
  {"ON_pa", Parameter(0,3,1,1)},
  {"Gain_conv", Parameter(0,4,4,0b0100)},
  {"ON_rtr", Parameter(1,0,1,1)},
  {"Sw_super_conv", Parameter(1,1,1,1)},
  {"Dacb_vb_conv", Parameter(1,2,6,0b110011)},
  {"ON_toa", Parameter(2,0,1,1)},
  {"ON_tot", Parameter(2,1,1,1)},
  {"Dacb_vbi_pa", Parameter(2,2,6,0b011111)},
  {"Ibi_sk", Parameter(3,0,2,0)},
  {"Ibo_sk", Parameter(3,2,6,0b001010)},
  {"Ibi_inv", Parameter(4,0,2,0)},
  {"Ibo_inv", Parameter(4,2,6,0b001010)},
  {"Ibi_noinv", Parameter(5,0,2,0)},
  {"Ibo_noinv", Parameter(5,2,6,0b001010)},
  {"Ibi_inv_buf", Parameter(6,0,2,0b11)},
  {"Ibo_inv_buf", Parameter(6,2,6,0b100110)},
  {"Ibi_noinv_buf", Parameter(7,0,2,0b11)},
  {"Ibo_noinv_buf", Parameter(7,2,6,0b100110)},
  {"Sw_cd", Parameter(8,0,3,0b111)},
  {"En_hyst_tot", Parameter(8,3,1,0)},
  {"Sw_Cf_comp", Parameter(8,4,4,0b1010)},
  {"Sw_Cf", Parameter(9,0,4,0b1010)},
  {"Sw_Rf", Parameter(9,4,4,0b1000)},
  {"Clr_ShaperTail", Parameter(10,1,1,0)},
  {"SelRisingEdge", Parameter(10,2,1,1)},
  {"SelExtADC", Parameter(10,3,1,0)},
  {"Clr_ADC", Parameter(10,4,1,0)},
  {"S_sk", Parameter(10,5,3,0b010)},
  {"S_inv", Parameter(11,2,3,0b010)},
  {"S_noinv", Parameter(11,5,3,0b010)},
  {"S_inv_buf", Parameter(12,2,3,0b110)},
  {"S_noinv_buf", Parameter(12,5,3,0b110)},
  {"Ref_adc", Parameter(13,0,2,0)},
  {"Delay40", Parameter(13,2,3,0)},
  {"Delay65", Parameter(13,5,3,0)},
  {"ON_ref_adc", Parameter(14,0,1,1)},
  {"Pol_adc", Parameter(14,1,1,1)},
  {"Delay87", Parameter(14,2,3,0)},
  {"Delay9", Parameter(14,5,3,0)}
};

const std::map<std::string,Parameter>
REFERENCE_VOLTAGE_LUT = {
  {"Probe_vref_pa", Parameter(0,0,1,0)},
  {"Probe_vref_time", Parameter(0,1,1,0)},
  {"Refi", Parameter(0,2,2,0b11)},
  {"Vbg_1v", Parameter(0,4,3,0b111)},
  {"ON_dac", Parameter(0,7,1,1)},
  {"Noinv_vref", Parameter({RegisterLocation(1,0,2),RegisterLocation(5,0,8)},0b0100111100)},
  {"Inv_vref", Parameter({RegisterLocation(1,2,2),RegisterLocation(4,0,8)},0b0110000000)},
  {"Toa_vref", Parameter({RegisterLocation(1,4,2),RegisterLocation(3,0,8)},0b0001110000)},
  {"Tot_vref", Parameter({RegisterLocation(1,6,2),RegisterLocation(2,0,8)},0b0110110000)},
  {"Calib_dac", Parameter({RegisterLocation(6,0,8),RegisterLocation(7,0,4)},0)},
  {"IntCtest", Parameter(7,6,1,0)},
  {"ExtCtest", Parameter(7,7,1,0)},
  {"Probe_dc", Parameter(8,0,8,0)}
};

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

const std::map<std::string,Parameter> CHANNEL_WISE_LUT = {
  {"Inputdac", Parameter(0,0,6,0b011111)},
  {"Dacb", Parameter({RegisterLocation(2,6,2),RegisterLocation(1,6,2),RegisterLocation(0,6,2)},0b111111)},
  {"Sign_dac", Parameter(1,0,1,0)},
  {"Ref_dac_toa", Parameter(1,1,5,0)},
  {"Probe_noinv", Parameter(2,0,1,0)},
  {"Ref_dac_tot", Parameter(2,1,5,0)},
  {"Mask_toa", Parameter(3,0,1,0)},
  {"Ref_dac_inv", Parameter(3,1,5,0)},
  {"Sel_trigger_toa", Parameter(3,6,1,0)},
  {"Probe_inv", Parameter(3,7,1,0)},
  {"Probe_pa", Parameter(4,0,1,0)},
  {"LowRange", Parameter(4,1,1,0)},
  {"HighRange", Parameter(4,2,1,0)},
  {"Channel_off", Parameter(4,3,1,0)},
  {"Sel_trigger_tot", Parameter(4,4,1,0)},
  {"Mask_tot", Parameter(4,5,1,0)},
  {"Probe_tot", Parameter(4,6,1,0)},
  {"Probe_toa", Parameter(4,7,1,0)},
  {"DAC_CAL_FTDC_TOA", Parameter(5,0,6,0)},
  {"Mask_adc", Parameter(5,7,1,0)},
  {"DAC_CAL_CTDC_TOA", Parameter(6,0,6,0)},
  {"DAC_CAL_FTDC_TOT", Parameter(7,0,6,0)},
  {"DAC_CAL_CTDC_TOT", Parameter(8,0,6,0)},
  {"IN_FTDC_ENCODER_TOA", Parameter(9,0,6,0)},
  {"IN_FTDC_ENCODER_TOT", Parameter(10,0,6,0)},
  {"DIS_TDC", Parameter(10,7,1,0)},
  {"ExtData", Parameter({RegisterLocation(13,0,8),RegisterLocation(11,0,2)},0)},
  {"Mask_AlignBuffer", Parameter(11,7,1,0)},
  {"Adc_pedestal", Parameter(12,0,8,0)}
};

const std::map<std::string,Parameter> DIGITAL_HALF_LUT = {
  {"SelRawData", Parameter(0,0,1,1)},
  {"SelTC4"    , Parameter(0,1,1,1)},
  {"CmdSelEdge", Parameter(0,2,1,1)},
  {"Adc_TH"    , Parameter(0,4,4,0)},
  {"MultFactor", Parameter(1,0,5,0b11001)},
  {"L1Offset"  , Parameter({RegisterLocation(2,0,8),RegisterLocation(1,7,1)},0b000001000)},
  {"IdleFrame" , Parameter({RegisterLocation(3,0,8),RegisterLocation(4,0,8),RegisterLocation(5,0,8),RegisterLocation(6,0,4)},0b1100110011001100110011001100)},
  {"ByPassCh0" , Parameter(6,4,1,0)},
  {"ByPassCh17", Parameter(6,5,1,0)},
  {"ByPassCh35", Parameter(6,6,1,0)},
  {"Tot_TH0"   , Parameter(7,0,8,0)},
  {"Tot_TH1"   , Parameter(8,0,8,0)},
  {"Tot_TH2"   , Parameter(9,0,8,0)},
  {"Tot_TH3"   , Parameter(10,0,8,0)},
  {"Tot_P0"    , Parameter(11,0,7,0)},
  {"Tot_P1"    , Parameter(12,0,7,0)},
  {"Tot_P2"    , Parameter(13,0,7,0)},
  {"Tot_P3"    , Parameter(14,0,7,0)}
};

const std::map<std::string,Parameter> TOP_LUT = {
  {"EN_LOCK_CONTROL", Parameter(0,0,1,1)},
  {"ERROR_LIMIT_SC" , Parameter(0,1,3,0b010)},
  {"Sel_PLL_Locked" , Parameter(0,4,1,1)},
  {"PllLockedSc"    , Parameter(0,5,1,1)},
  {"OrbitSync_sc"   , Parameter(0,7,1,0)},
  {"EN_PLL"         , Parameter(1,0,1,1)},
  {"DIV_PLL_A"      , Parameter(1,1,1,0)},
  {"DIV_PLL_B"      , Parameter(1,2,1,0)},
  {"EN_HIGH_CAPA"   , Parameter(1,3,1,0)},
  {"EN_REF_BG"      , Parameter(1,4,1,1)},
  {"VOUT_INIT_EN"   , Parameter(1,5,1,0)},
  {"VOUT_INIT_EXT_EN", Parameter(2,0,1,0)},
  {"VOUT_INIT_EXT_D" , Parameter(2,1,5,0)},
  {"FOLLOWER_PLL_EN" , Parameter(3,0,1,1)},
  {"BIAS_I_PLL_D"    , Parameter(3,1,6,0b011000)},
  {"Sel_40M_ext"     , Parameter(3,7,1,0)},
  {"EN0_probe_pll"   , Parameter(4,0,1,1)},
  {"EN1_probe_pll"   , Parameter(4,1,1,1)},
  {"EN2_probe_pll"   , Parameter(4,2,1,0)},
  {"EN-pE0_probe_pll", Parameter(4,3,1,0)},
  {"EN-pE1_probe_pll", Parameter(4,4,1,0)},
  {"EN-pE2_probe_pll", Parameter(4,5,1,0)},
  {"S0_probe_pll"    , Parameter(4,6,1,0)},
  {"S1_probe_pll"    , Parameter(4,7,1,0)},
  {"EN1"             , Parameter(5,0,1,1)},
  {"EN2"             , Parameter(5,1,1,1)},
  {"EN3"             , Parameter(5,2,1,0)},
  {"EN-pE0"          , Parameter(5,3,1,0)},
  {"EN-pE1"          , Parameter(5,4,1,0)},
  {"EN-pE2"          , Parameter(5,5,1,0)},
  {"S0"              , Parameter(5,6,1,0)},
  {"S1"              , Parameter(5,7,1,0)},
  {"Sel_ReSync_fcmd" , Parameter(6,0,1,1)},
  {"Sel_L1_fcmd"     , Parameter(6,1,1,1)},
  {"Sel_STROBE_fcmd" , Parameter(6,2,1,1)},
  {"Sel_OrbitSync_fcmd", Parameter(6,3,1,1)},
  {"En_PhaseShift" , Parameter(7,0,1,1)},
  {"Phase"         , Parameter(7,1,4,0)},
  {"En_pll_ext"    , Parameter(7,7,1,0)}
};

const std::map<std::string,std::pair<int,std::map<std::string,Parameter>>>
PARAMETER_LUT = {
  {"Global_Analog_0", { 297, GLOBAL_ANALOG_LUT }},
  {"Reference_Voltage_0", {296, REFERENCE_VOLTAGE_LUT}},
  {"Global_Analog_0", {297, GLOBAL_ANALOG_LUT}},
  {"Master_TDC_0", {298, MASTER_TDC_LUT}},
  //{"Digital_Half_0", {299, DIGITAL_HALF_LUT}},
  {"Reference_Voltage_1", {40, REFERENCE_VOLTAGE_LUT}},
  {"Global_Analog_1", {41, GLOBAL_ANALOG_LUT}},
  {"Master_TDC_1", {42, MASTER_TDC_LUT}},
  //{"Digital_Half_1", {43, DIGITAL_HALF_LUT}},
  //{"Top", {44,TOP_LUT}},
  //{"CM0", {275, CHANNEL_WISE_LUT}},
  //{"CM1", {276, CHANNEL_WISE_LUT}},
  //{"CALIB0", {274, CHANNEL_WISE_LUt}},
  //{"CM2", {19, CHANNEL_WISE_LUT}},
  //{"CM3", {20, CHANNEL_WISE_LUT}},
  //{"CALIB1", {18, CHANNEL_WISE_LUT}},
  {"Channel_0", {261, CHANNEL_WISE_LUT}},
  {"Channel_1", {260, CHANNEL_WISE_LUT}},
  {"Channel_2", {259, CHANNEL_WISE_LUT}},
  {"Channel_3", {258, CHANNEL_WISE_LUT}},
  {"Channel_4", {265, CHANNEL_WISE_LUT}},
  {"Channel_5", {264, CHANNEL_WISE_LUT}},
  {"Channel_6", {263, CHANNEL_WISE_LUT}},
  {"Channel_7", {262, CHANNEL_WISE_LUT}},
  {"Channel_8", {269, CHANNEL_WISE_LUT}},
  {"Channel_9", {268, CHANNEL_WISE_LUT}},
  {"Channel_10", {267, CHANNEL_WISE_LUT}},
  {"Channel_11", {266, CHANNEL_WISE_LUT}},
  {"Channel_12", {273, CHANNEL_WISE_LUT}},
  {"Channel_13", {272, CHANNEL_WISE_LUT}},
  {"Channel_14", {271, CHANNEL_WISE_LUT}},
  {"Channel_15", {270, CHANNEL_WISE_LUT}},
  {"Channel_16", {294, CHANNEL_WISE_LUT}},
  {"Channel_17", {256, CHANNEL_WISE_LUT}},
  {"Channel_18", {277, CHANNEL_WISE_LUT}},
  {"Channel_19", {295, CHANNEL_WISE_LUT}},
  {"Channel_20", {278, CHANNEL_WISE_LUT}},
  {"Channel_21", {279, CHANNEL_WISE_LUT}},
  {"Channel_22", {280, CHANNEL_WISE_LUT}},
  {"Channel_23", {281, CHANNEL_WISE_LUT}},
  {"Channel_24", {282, CHANNEL_WISE_LUT}},
  {"Channel_25", {283, CHANNEL_WISE_LUT}},
  {"Channel_26", {284, CHANNEL_WISE_LUT}},
  {"Channel_27", {285, CHANNEL_WISE_LUT}},
  {"Channel_28", {286, CHANNEL_WISE_LUT}},
  {"Channel_29", {287, CHANNEL_WISE_LUT}},
  {"Channel_30", {288, CHANNEL_WISE_LUT}},
  {"Channel_31", {289, CHANNEL_WISE_LUT}},
  {"Channel_32", {290, CHANNEL_WISE_LUT}},
  {"Channel_33", {291, CHANNEL_WISE_LUT}},
  {"Channel_34", {292, CHANNEL_WISE_LUT}},
  {"Channel_35", {293, CHANNEL_WISE_LUT}},
  {"Channel_36", {5, CHANNEL_WISE_LUT}},
  {"Channel_37", {4, CHANNEL_WISE_LUT}},
  {"Channel_38", {3, CHANNEL_WISE_LUT}},
  {"Channel_39", {2, CHANNEL_WISE_LUT}},
  {"Channel_40", {9, CHANNEL_WISE_LUT}},
  {"Channel_41", {8, CHANNEL_WISE_LUT}},
  {"Channel_42", {7, CHANNEL_WISE_LUT}},
  {"Channel_43", {6, CHANNEL_WISE_LUT}},
  {"Channel_44", {13, CHANNEL_WISE_LUT}},
  {"Channel_45", {12, CHANNEL_WISE_LUT}},
  {"Channel_46", {11, CHANNEL_WISE_LUT}},
  {"Channel_47", {10, CHANNEL_WISE_LUT}},
  {"Channel_48", {17, CHANNEL_WISE_LUT}},
  {"Channel_49", {16, CHANNEL_WISE_LUT}},
  {"Channel_50", {15, CHANNEL_WISE_LUT}},
  {"Channel_51", {14, CHANNEL_WISE_LUT}},
  {"Channel_52", {38, CHANNEL_WISE_LUT}},
  {"Channel_53", {0, CHANNEL_WISE_LUT}},
  {"Channel_54", {21, CHANNEL_WISE_LUT}},
  {"Channel_55", {39, CHANNEL_WISE_LUT}},
  {"Channel_56", {22, CHANNEL_WISE_LUT}},
  {"Channel_57", {23, CHANNEL_WISE_LUT}},
  {"Channel_58", {24, CHANNEL_WISE_LUT}},
  {"Channel_59", {25, CHANNEL_WISE_LUT}},
  {"Channel_60", {26, CHANNEL_WISE_LUT}},
  {"Channel_61", {27, CHANNEL_WISE_LUT}},
  {"Channel_62", {28, CHANNEL_WISE_LUT}},
  {"Channel_63", {29, CHANNEL_WISE_LUT}},
  {"Channel_64", {30, CHANNEL_WISE_LUT}},
  {"Channel_65", {31, CHANNEL_WISE_LUT}},
  {"Channel_66", {32, CHANNEL_WISE_LUT}},
  {"Channel_67", {33, CHANNEL_WISE_LUT}},
  {"Channel_68", {34, CHANNEL_WISE_LUT}},
  {"Channel_69", {35, CHANNEL_WISE_LUT}},
  {"Channel_70", {36, CHANNEL_WISE_LUT}},
  {"Channel_71", {37, CHANNEL_WISE_LUT}}
};

void Compiler::apply(YAML::Node params) {
  // deduce list of page names for regex search
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

    // apply these settings only to pages that match the regex
    std::regex page_regex(page_name);
    std::vector<std::string> matching_pages;
    std::copy_if(page_names.begin(), page_names.end(), std::back_inserter(matching_pages),
        [&](const std::string& page) { return std::regex_match(page,page_regex); });

    if (matching_pages.empty()) {
      PFEXCEPTION_RAISE("NotFound", "The page "+page_name+" does not match any pages in the look up table.");
    }

    for (const auto& page : matching_pages) {
      for (const auto& param : page_settings)
        settings_[page][param.first.as<std::string>()] = param.second.as<int>();
    }
  }
}

std::map<int,std::map<int,uint8_t>> Compiler::compile() {
  std::map<int,std::map<int,uint8_t>> register_values;
  for (const auto& page : settings_) {
    // page.first => page name
    // page.second => parameter to value map
    if (PARAMETER_LUT.find(page.first) == PARAMETER_LUT.end()) {
      // this exception shouldn't really ever happen because we check if the input
      // page matches any of the pages in the LUT in Compiler::apply, but
      // we leave this check in here for future development
      PFEXCEPTION_RAISE("NotFound", "The page named '"+page.first+"' is not found in the look up table.");
    }
    const auto& page_id{PARAMETER_LUT.at(page.first).first};
    const auto& page_lut{PARAMETER_LUT.at(page.first).second};
    for (const auto& param : page.second) {
      // param.first => parameter name
      // param.second => value
      if (page_lut.find(param.first) == page_lut.end()) {
        PFEXCEPTION_RAISE("NotFound", "The parameter named '"+param.first 
            +"' is not found in the look up table for page "+page.first);
      }

      const Parameter& spec{page_lut.at(param.first)};
      std::size_t value_curr_min_bit{0};
      for (const RegisterLocation& location : spec.registers) {
        // grab sub value of parameter in this register
        uint8_t sub_val = ((param.second >> value_curr_min_bit) & location.mask);
        value_curr_min_bit += location.n_bits;
        // initialize register value to zero if it hasn't been touched before
        if (register_values[page_id].find(location.reg) == register_values[page_id].end()) {
          register_values[page_id][location.reg] = 0;
        }

        // put value into register at the specified location
        register_values[page_id][location.reg] += (sub_val << location.min_bit);
      } // loop over register locations
    }   // loop over parameters in page
  }     // loop over pages

  return register_values;
}

Compiler::Compiler(const std::vector<std::string>& setting_files, bool prepend_defaults) {
  // if we prepend the defaults, put all settings and their defaults 
  // into the settings map
  if (prepend_defaults) {
    for(auto& page : PARAMETER_LUT) {
      for (auto& param : page.second.second) {
        settings_[page.first][param.first] = param.second.def;
      }
    }
  }

  for (auto& setting_file : setting_files) {
    YAML::Node setting_yaml;
    try {
       setting_yaml = YAML::LoadFile(setting_file);
    } catch (const YAML::BadFile& e) {
      PFEXCEPTION_RAISE("BadFile","Unable to load file " + setting_file);
    }
    if (setting_yaml.IsSequence()) {
      for (std::size_t i{0}; i < setting_yaml.size(); i++) apply(setting_yaml[i]);
    } else {
      apply(setting_yaml);
    }
  }
}

}
