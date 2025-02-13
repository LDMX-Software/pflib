#pragma once

#include <hw_interface.hpp>
#include "emp/Controller.hpp"
#include "emp/DatapathNode.hpp"
#include "emp/SCCNode.hpp"
#include "emp/SCCICNode.hpp"
#include "emp/exception.hpp"

typedef std::shared_ptr<emp::Controller> EMPControllPTR;
constexpr unsigned MAX_NUMBER_OF_ICEC_TRIES = 10;

class EMPInterface : public HWInterface
{
public:

  EMPInterface( )
  {;}

  EMPInterface( const std::string connection_file,
		const std::string device,
		const unsigned int emp_channel,
		const unsigned int timeout,
		const std::string mutexName) : HWInterface(connection_file,device,mutexName)
  {
    m_controller = std::make_shared<emp::Controller>(*mUhal);
    m_controller->hw().setTimeoutPeriod(timeout);
    m_emp_channel = emp_channel;
    m_n_retry=0;
  }

  void set_EC()
  {
    activateThisEMPChannel();
    mUhal->getNode("datapath.region.fe_mgt.data_framer.scc.ctrl.ic_ec_mux_sel").write(1);
    mUhal->dispatch();
  }

  void set_IC()
  {
    activateThisEMPChannel();
    mUhal->getNode("datapath.region.fe_mgt.data_framer.scc.ctrl.ic_ec_mux_sel").write(0);
    mUhal->dispatch();
  }

  void write_IC(unsigned int reg, const std::vector<unsigned int>& vals, unsigned int lpgbt_addr=0x70)
  {
    m_controller->getSCC().reset();
    spdlog::apply_all([&](LoggerPtr l) { l->trace("write_IC :  lpgbt_addr = 0x{:02x}, reg = 0x{:04x}, first val = 0x{:04x}, nregs = {}",
  						 lpgbt_addr,
  						 reg,
  						 vals[0],
  						 vals.size()
  						 ); } );
    m_n_retry=0;
    try_to_write(reg, vals, lpgbt_addr);
  }
  
  std::vector<unsigned int> read_IC(unsigned int reg, unsigned read_len=1, unsigned int lpgbt_addr=0x70)
  {
    spdlog::apply_all([&](LoggerPtr l) { l->trace("read_IC :  lpgbt_addr = 0x{:02x}, reg = 0x{:04x}, read_len = {}",
  						 lpgbt_addr,
  						 reg,
  						 read_len
  						 ); } );
    m_controller->getSCC().reset();
    m_n_retry=0;
    return try_to_read(reg, read_len, lpgbt_addr);
  }

  void blockWrite_IC(const std::vector<unsigned int>& regs, const std::vector<std::vector<unsigned int>>& vals, unsigned int lpgbt_addr=0x70)
  {
    spdlog::apply_all([&](LoggerPtr l) { l->trace("blockWrite_IC :  lpgbt_addr = 0x{:02x}, first reg address = 0x{:04x}",
						  lpgbt_addr,
						  regs[0]
						  ); } );
    m_controller->getSCC().reset();
    m_n_retry=0;
    try_to_blockWrite(regs, vals, lpgbt_addr);
  }
  
  std::vector<unsigned int> blockRead_IC(const std::vector<unsigned int>& regs, const std::vector<unsigned int>& read_len_vec, unsigned int lpgbt_addr=0x70)
  {
    spdlog::apply_all([&](LoggerPtr l) { l->trace("blockRead_IC :  lpgbt_addr = 0x{:02x}, first reg address = 0x{:04x}",
						  lpgbt_addr,
						  regs[0]
						  ); } );
    m_controller->getSCC().reset();
    m_n_retry=0;
    return try_to_blockRead(regs, read_len_vec, lpgbt_addr);
  }
  
  
protected:
  void activateThisEMPChannel()
  {
    (*m_controller).getDatapath().selectRegion(m_emp_channel/4);
    (*m_controller).getDatapath().selectLink(m_emp_channel);
  }

  void try_to_write(unsigned int reg, const std::vector<unsigned int>& vals, unsigned int lpgbt_addr)
  {
    try{
      m_controller->getSCCIC().icWriteBlock(reg, vals, lpgbt_addr);
    }
    catch(emp::exception& e){
      spdlog::apply_all([&](LoggerPtr l) { l->warn("EMP write timeout -> will try again"); } );
      if(m_n_retry<MAX_NUMBER_OF_ICEC_TRIES) {
	m_n_retry++;
	try_to_write(reg, vals, lpgbt_addr);
      }
      else{
	unlock();
	throw swamp::ICECWriteTimeoutException("IC/EC write timeout");
      }
    }
  }

  std::vector<unsigned int> try_to_read(unsigned int reg, unsigned read_len, unsigned int lpgbt_addr)
  {
    try{
      auto data = m_controller->getSCCIC().icReadBlock(reg, read_len, lpgbt_addr);
      return data;
    }
    catch(emp::exception& e){
      spdlog::apply_all([&](LoggerPtr l) { l->warn("EMP read timeout -> will try again"); } );
      if(m_n_retry<MAX_NUMBER_OF_ICEC_TRIES) {
	m_n_retry++;
	return try_to_read(reg, read_len, lpgbt_addr);
      }
      else{
	unlock();
	throw swamp::ICECReadTimeoutException("IC/EC read timeout");
      }
    }
  }

  void try_to_blockWrite(const std::vector<unsigned int>& regs, const std::vector<std::vector<unsigned int>>& vals, unsigned int lpgbt_addr)
  {
    spdlog::apply_all([&](LoggerPtr l) { l->debug("EMP trying blockWrite"); } );
    try{
      m_controller->getSCCIC().icBlockWrite(regs, vals, lpgbt_addr);
    }
    catch(emp::exception& e){
      spdlog::apply_all([&](LoggerPtr l) { l->warn("EMP write timeout -> will try again"); } );
      if(m_n_retry<MAX_NUMBER_OF_ICEC_TRIES) {
	m_n_retry++;
	try_to_blockWrite(regs, vals, lpgbt_addr);
      }
      else{
	unlock();
	throw swamp::ICECWriteTimeoutException("IC/EC write timeout");
      }
    }
  }

  std::vector<unsigned int> try_to_blockRead(const std::vector<unsigned int>& regs, const std::vector<unsigned int>& read_len_vec, unsigned int lpgbt_addr)
  {
    spdlog::apply_all([&](LoggerPtr l) { l->debug("EMP trying blockRead"); } );
    try{
      auto data = m_controller->getSCCIC().icBlockRead(regs, read_len_vec, lpgbt_addr);
      return data;
    }
    catch(emp::exception& e){
      spdlog::apply_all([&](LoggerPtr l) { l->warn("EMP read timeout -> will try again"); } );
      if(m_n_retry<MAX_NUMBER_OF_ICEC_TRIES) {
	m_n_retry++;
	return try_to_blockRead(regs, read_len_vec, lpgbt_addr);
      }
      else{
	unlock();
	throw swamp::ICECReadTimeoutException("IC/EC read timeout");
      }
    }
  }
  
protected:
  EMPControllPTR m_controller;
  unsigned int m_emp_channel;
  unsigned int m_n_retry;
};

