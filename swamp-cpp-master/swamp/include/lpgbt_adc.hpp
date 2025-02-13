#pragma once
#include "adc.hpp"

const std::unordered_map<std::string,int> ADCPINMAP = {
  {"EXT0", 0},
  {"EXT1", 1},
  {"EXT2", 2},
  {"EXT3", 3},
  {"EXT4", 4},
  {"EXT5", 5},
  {"EXT6", 6},
  {"EXT7", 7},
  {"VDAC" , 8 },
  {"VSSA" , 9 },
  {"VDDTX", 10},
  {"VDDRX", 11},
  {"VDD"  , 12},
  {"VDDA" , 13},
  {"TEMP" , 14},
  {"VREF2", 15}
};

constexpr static uint16_t MAXADCGAINSELECT = 3;


struct LPGBT_ADC_MEASUREMENT_CONFIG
{
  LPGBT_ADC_MEASUREMENT_CONFIG(){;}
  LPGBT_ADC_MEASUREMENT_CONFIG(  std::string aname, std::string achannel_p, std::string achannel_n, int acurrent_dac_ch_enable, int again) : name(aname),
																	     channel_p(achannel_p),
																	     channel_n(achannel_n),
																	     current_dac_ch_enable(acurrent_dac_ch_enable),
																	     gain(again)
  {
    if( ADCPINMAP.find(channel_p)==ADCPINMAP.end() ){
      auto errorMsg = "ADC pin " + channel_p + " does not exits. Wrong configuration of ADC " + name ;
      spdlog::apply_all([&](LoggerPtr l) { l->critical(errorMsg); });
      throw swamp::LPGBTADCYAMLConfigErrorException(errorMsg.c_str());
    }
    if( ADCPINMAP.find(channel_n)==ADCPINMAP.end() ){
      auto errorMsg = "ADC pin " + channel_n + " does not exits. Wrong configuration of ADC " + name ;
      spdlog::apply_all([&](LoggerPtr l) { l->critical(errorMsg); });
      throw swamp::LPGBTADCYAMLConfigErrorException(errorMsg.c_str());
    }
    if( gain>MAXADCGAINSELECT ){
      auto errorMsg = "Chosen ADC gain is greater than the maximum allowed value (max=3) for " + name ;
      spdlog::apply_all([&](LoggerPtr l) { l->critical(errorMsg); });
      throw swamp::LPGBTADCYAMLConfigErrorException(errorMsg.c_str());
    }
  }
  std::string name;
  std::string channel_p;
  std::string channel_n;
  int current_dac_ch_enable;
  int gain;

  friend struct YAML::convert<LPGBT_ADC_MEASUREMENT_CONFIG>;
  friend std::ostream& operator<<(std::ostream& out,const LPGBT_ADC_MEASUREMENT_CONFIG& lamc)
  {
    out << "LPGBT_ADC_MEASUREMENT_CONFIG (name, channel_p, channel_n, current_dac_ch_enable, gain): "
	<< lamc.name << " "
	<< lamc.channel_p << " "
	<< lamc.channel_n << " "
	<< lamc.current_dac_ch_enable << " "
	<< lamc.gain ;
    return out;
  }
};

namespace YAML {
  template<>
  struct convert<LPGBT_ADC_MEASUREMENT_CONFIG> {
    static Node encode(const LPGBT_ADC_MEASUREMENT_CONFIG& rhs) {
      Node node;
      node["name"]      = rhs.name;
      node["channel_p"] = rhs.channel_p;
      node["channel_n"] = rhs.channel_n;
      node["current_dac_ch_enable"] = rhs.current_dac_ch_enable;
      node["gain"] = rhs.gain;
      return node;
    }
    
    static bool decode(const Node& node, LPGBT_ADC_MEASUREMENT_CONFIG& rhs) {
      if(!node.IsMap() || node.size() != 5) {
	return false;
      }
      auto name = node["name"].as<std::string>();
      auto channel_p = node["channel_p"].as<std::string>();
      auto channel_n = node["channel_n"].as<std::string>();
      std::transform(channel_p.begin(), channel_p.end(), channel_p.begin(),
		     [](const char& c){ return std::toupper(c);} );
      std::transform(channel_n.begin(), channel_n.end(), channel_n.begin(),
		     [](const char& c){ return std::toupper(c);} );
      auto current_dac_ch_enable = node["current_dac_ch_enable"].as<int>();
      auto gain = node["gain"].as<int>();
      rhs = LPGBT_ADC_MEASUREMENT_CONFIG(name, channel_p, channel_n, current_dac_ch_enable, gain);
      return true;
    }
  };
}

struct VREFCNTR{
  constexpr static uint16_t ADDRESS = 0x001C;

  constexpr static uint16_t VREFENABLE_BIT_MASK = 128;
  constexpr static uint16_t VREFENABLE_LENGTH = 1;
  constexpr static uint16_t VREFENABLE_OFFSET = 7;
};

struct VREFTUNE{
  constexpr static uint16_t ADDRESS = 0x001D;

  constexpr static uint16_t VREFTUNE_BIT_MASK = 255;
  constexpr static uint16_t VREFTUNE_LENGTH = 8;
  constexpr static uint16_t VREFTUNE_OFFSET = 0;
};

struct DACCONFIGH{
  constexpr static uint16_t ADDRESS = 0x006A;

  constexpr static uint16_t VOLDACENABLE_BIT_MASK = 128;
  constexpr static uint16_t VOLDACENABLE_LENGTH = 1;
  constexpr static uint16_t VOLDACENABLE_OFFSET = 7;

  constexpr static uint16_t CURDACENABLE_BIT_MASK = 64;
  constexpr static uint16_t CURDACENABLE_LENGTH = 1;
  constexpr static uint16_t CURDACENABLE_OFFSET = 6;

  constexpr static uint16_t VOLDACVALUE_BIT_MASK = 15;
  constexpr static uint16_t VOLDACVALUE_LENGTH = 4;
  constexpr static uint16_t VOLDACVALUE_OFFSET = 0;  
};

struct CURDACVALUE{
  constexpr static uint16_t ADDRESS = 0x006C;

  constexpr static uint16_t CURDACVALUE_BIT_MASK = 255;
  constexpr static uint16_t CURDACVALUE_LENGTH = 8;
  constexpr static uint16_t CURDACVALUE_OFFSET = 0;
};

struct CURDACCHN{
  constexpr static uint16_t ADDRESS = 0x006D;

  constexpr static uint16_t CURDACCHNENABLE_BIT_MASK = 255;
  constexpr static uint16_t CURDACCHNENABLE_LENGTH = 8;
  constexpr static uint16_t CURDACCHNENABLE_OFFSET = 0;
};

struct ADCCONFIG{
  constexpr static uint16_t ADDRESS = 0x0123;
  
  constexpr static uint16_t ADCCONVERT_BIT_MASK = 128;
  constexpr static uint16_t ADCCONVERT_LENGTH = 1;
  constexpr static uint16_t ADCCONVERT_OFFSET = 7;

  constexpr static uint16_t ADCENABLE_BIT_MASK = 4;
  constexpr static uint16_t ADCENABLE_LENGTH = 1;
  constexpr static uint16_t ADCENABLE_OFFSET = 2;

  constexpr static uint16_t ADCGAINSELECT_BIT_MASK = 3;
  constexpr static uint16_t ADCGAINSELECT_LENGTH = 2;
  constexpr static uint16_t ADCGAINSELECT_OFFSET = 0;
};

struct ADCSELECT{
  constexpr static uint16_t ADDRESS = 0x0121;

  constexpr static uint16_t ADCINPSELECT_BIT_MASK = 0xF0;
  constexpr static uint16_t ADCINPSELECT_LENGTH = 4;
  constexpr static uint16_t ADCINPSELECT_OFFSET = 4;

  constexpr static uint16_t ADCINNSELECT_BIT_MASK = 0x0F;
  constexpr static uint16_t ADCINNSELECT_LENGTH = 4;
  constexpr static uint16_t ADCINNSELECT_OFFSET = 0;
};

struct ADCSTATUSH{
  constexpr static uint16_t ADDRESS = 0x01CA;

  constexpr static uint16_t ADCBUSY_BIT_MASK = 128;
  constexpr static uint16_t ADCBUSY_LENGTH = 1;
  constexpr static uint16_t ADCBUSY_OFFSET = 7;

  constexpr static uint16_t ADCDONE_BIT_MASK = 64;
  constexpr static uint16_t ADCDONE_LENGTH = 1;
  constexpr static uint16_t ADCDONE_OFFSET = 6;

  constexpr static uint16_t ADCVALUE_BIT_MASK = 3;
  constexpr static uint16_t ADCVALUE_LENGTH = 2;
  constexpr static uint16_t ADCVALUE_OFFSET = 0;
};

struct ADCSTATUSL{
  constexpr static uint16_t ADDRESS = 0x01CB;

  constexpr static uint16_t ADCVALUE_BIT_MASK = 255;
  constexpr static uint16_t ADCVALUE_LENGTH = 8;
  constexpr static uint16_t ADCVALUE_OFFSET = 0;
};

class LPGBT_ADC : public ADC
{
public:

  LPGBT_ADC( const std::string& name, const YAML::Node& config);

  void ConfigureImpl(const YAML::Node& node) override;

  void AcquireImpl(unsigned int nSamples=1) override;

protected:
  unsigned int m_current_dac_enable;
  unsigned int m_current_dac_value;
  unsigned int m_voltage_dac_enable;
  unsigned int m_voltage_dac_value;
  unsigned int m_vref_tune;
  std::vector<LPGBT_ADC_MEASUREMENT_CONFIG> m_external_measurements;
};
