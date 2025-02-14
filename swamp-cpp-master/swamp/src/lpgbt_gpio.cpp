#include "lpgbt_gpio.hpp"

LPGBT_GPIO::LPGBT_GPIO( const std::string& name, const YAML::Node& config) : GPIO(name,config)
{
  if( m_pin>=16 ){
    auto errorMsg = "lpGBT GPIO {} : wrong 'pin' value which must be lower than 16 -> will exit the program";
    spdlog::apply_all([&](LoggerPtr l) { l->critical(errorMsg,m_name); });
    throw swamp::LPGBTGPIOConstructionException(errorMsg);
  }
  m_reg_offset = m_pin >= 8 ? 0 : 1;

  m_bit_mask = 1<<(m_pin%8);
}

void LPGBT_GPIO::UpImpl()
{
  m_carrier->write_reg( m_reg_offset+PIOOUTH_ADDRESS,
			1,
			m_bit_mask
			);
}

void LPGBT_GPIO::DownImpl()
{
  m_carrier->write_reg( m_reg_offset+PIOOUTH_ADDRESS,
			0,
			m_bit_mask
			);
}


bool LPGBT_GPIO::IsUpImpl()
{
  auto val = m_carrier->read_reg( m_reg_offset+PIOINH_ADDRESS );
  return (val & m_bit_mask) > 0 ? true : false;
}  

bool LPGBT_GPIO::IsDownImpl()
{
  auto val = m_carrier->read_reg( m_reg_offset+PIOINH_ADDRESS );
  return (val & m_bit_mask) > 0 ? false : true;
}

void LPGBT_GPIO::SetDirectionImpl(GPIO_Direction dir)
{
  m_carrier->write_reg( m_reg_offset+PIODIRH_ADDRESS,
			dir,
			m_bit_mask
			);  
}

GPIO_Direction LPGBT_GPIO::GetDirectionImpl()
{
  auto val = m_carrier->read_reg( m_reg_offset+PIODIRH_ADDRESS );
  return (val & m_bit_mask) > 0 ? GPIO_Direction::OUTPUT : GPIO_Direction::INPUT;
}

void LPGBT_GPIO::ConfigureImpl(const YAML::Node& node)
{
}

// void LPGBT_GPIO::configure(const YAML::Node& node)
// {
//   if( config["pin"].IsDefined() ){
//     auto apin = config["pin"].as<unsigned int>();
//     if( apin>=16 )
//       spdlog::apply_all([&](LoggerPtr l) { l->warning("lpGBT GPIO {} : wrong 'pin' value which must be lower than 16 -> we will keep the same pin value ({})",m_name,m_pin); });
//     else
//       m_pin = pin;
//   }
  
//   if( config["dir"].IsDefined() ){
//     std::string direction = config["direction"].as<std::string>();
//     std::transform(direction.begin(),
// 		   direction.end(),
// 		   direction.begin(),
// 		   [](const char& c){ return std::tolower(c);} );
//     auto it = GPIODirectionMap.find(direction);
//     if (it != GPIODirectionMap.end())
//       m_direction = it->second;
//     else
//       spdlog::apply_all([&](LoggerPtr l) { l->warning("Direction of GPIO {} should be either \"input\" or \"output\", we will keep the same direction ({})",m_name,m_direction); });
//   }
// }
