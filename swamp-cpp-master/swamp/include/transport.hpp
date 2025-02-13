#pragma once
#include "carrier.hpp"
#include "swamp_type.hpp"

class Transport
{
public:

  Transport( const std::string& name, const YAML::Node& config) : m_name(name)
  {
    std::ostringstream os( std::ostringstream::ate );
    os.str(""); os << config ;
    spdlog::apply_all([&](LoggerPtr l) { l->debug("Creating Transport {} with config \n{}",m_name,os.str().c_str()); });

    std::string transportTypeStr = "GENERIC";
    if( config["transportType"].IsDefined() )
      transportTypeStr = config["transportType"].as<std::string>();
    else{
      auto warnMsg = "transportType was not found in transport config (in constructor) -> generic type will be applied";
      spdlog::apply_all([&](LoggerPtr l) { l->warn(warnMsg); });
    }
    
    std::transform(transportTypeStr.begin(), transportTypeStr.end(), transportTypeStr.begin(), 
		   [](const char& c){ return std::toupper(c);} );

    std::unordered_map<std::string,swamp::TransportType> typeMap{ {"GENERIC"       ,swamp::TransportType::GENERIC},
								  {"IC"            ,swamp::TransportType::IC},
								  {"EC"            ,swamp::TransportType::EC},
								  {"LPGBT_I2C"     ,swamp::TransportType::LPGBT_I2C},
								  {"LPGBT_I2C_DAQ" ,swamp::TransportType::LPGBT_I2C_DAQ},
								  {"LPGBT_I2C_TRG" ,swamp::TransportType::LPGBT_I2C_TRG},
								  {"GBT_SCA_I2C"   ,swamp::TransportType::GBT_SCA_I2C}
    };
    
    if( auto it = typeMap.find( transportTypeStr ); it != typeMap.end() ){
      m_transportType = it->second;
    }
    else{
      auto errorMsg = "A transportType ({}) was not found in transport type Map (in Transport constructor)";
      spdlog::apply_all([&](LoggerPtr l) { l->critical(errorMsg,transportTypeStr); });
      throw swamp::TransportConstructorException(errorMsg);      
    }

  }

  virtual ~Transport() = default;

  inline void setCarrier( CarrierPtr carrier ){ m_carrier = carrier; }

  void setLogger( LoggerPtr logger ) 
  {
    m_logger=logger;
  }

  virtual void write_regs(const unsigned int addr, const unsigned int reg_addr_width, const unsigned int reg_addr, const std::vector<unsigned int>& reg_vals) {return;};
  
  virtual void write_reg(const unsigned int addr, const unsigned int reg_val) {return;};
  
  virtual std::vector<unsigned int> read_regs(const unsigned int addr, const unsigned int reg_addr_width, const unsigned int reg_addr, const unsigned int read_len) {return std::vector<unsigned int>();};

  virtual std::vector<unsigned int> read_regs(const unsigned int addr, const unsigned int read_len) {return std::vector<unsigned int>();};
  
  virtual unsigned int read_reg(const unsigned int addr) {return (unsigned int)(0);};

  virtual void write_regs(const unsigned int addr, const unsigned int reg_addr_width, const std::vector<unsigned int>& reg_addr_vec, const std::vector<std::vector<unsigned int>>& reg_vals_vec) {return;}

  virtual std::vector<unsigned int> read_regs(const unsigned int addr, const unsigned int reg_addr_width, const std::vector<unsigned int>& reg_addr_vec, const std::vector<unsigned int>& read_len_vec) {
    auto errorMsg = "{}: read_regs(const unsigned int addr, const unsigned int reg_addr_width, const std::vector<unsigned int>& reg_addr_vec, const std::vector<unsigned int>& read_len_vec) is not implemented, please use other read_regs method instead";
    spdlog::apply_all([&](LoggerPtr l) { l->critical(errorMsg); });
    throw swamp::MethodNotImplementedException(errorMsg);
    return std::vector<unsigned int>();
  }

  void reset()
  {
    spdlog::apply_all([&](LoggerPtr l) { l->debug("{} : reseting",m_name); });
    lock();
    ResetImpl();
    unlock();
  }
  
  void configure( const YAML::Node& cfg )
  {
    std::ostringstream os( std::ostringstream::ate );
    os.str(""); os << cfg ;
    spdlog::apply_all([&](LoggerPtr l) { l->debug("{} : configuring with config \n{}",m_name,os.str().c_str()); });
    lock();
    ConfigureImpl(cfg);
    unlock();
  }

  void lock()
  {
    spdlog::apply_all([&](LoggerPtr l) { l->trace("{} : trying to lock ressource",m_name); });
    if(m_carrier==nullptr)
      LockImpl();
    else
      m_carrier->lock();
  }

  void unlock()
  {
    spdlog::apply_all([&](LoggerPtr l) { l->trace("{} : unlocking ressource",m_name); });
    if(m_carrier==nullptr)
      UnLockImpl();
    else
      m_carrier->unlock();
  }

  const swamp::TransportType transportType()
  {
    return m_transportType;
  }

protected:

  virtual void LockImpl(){;}

  virtual void UnLockImpl(){;}

  virtual void ConfigureImpl( const YAML::Node& config ) = 0;
  
  virtual void ResetImpl() = 0;

protected:

  std::string m_name;
  
  CarrierPtr m_carrier;

  LoggerPtr m_logger;

  swamp::TransportType m_transportType;
};

using TransportPtr = std::shared_ptr<Transport>;
 
