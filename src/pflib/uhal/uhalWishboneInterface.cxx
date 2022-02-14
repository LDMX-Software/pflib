#include "uhal/HwInterface.hpp"
#include "uhal/Node.hpp"
#include "uhal/ConnectionManager.hpp"
#include "pflib/uhal/uhalWishboneInterface.h"
     
namespace pflib {
namespace uhal {

  /** Construct a TCP bridge*/
uhalWishboneInterface::uhalWishboneInterface(const std::string& target, const std::string& ipbus_map_path) {
  std::string addressTable_uMNio("file://");
  addressTable_uMNio+=ipbus_map_path;
  if (not ipbus_map_path.empty() and ipbus_map_path.back() != '/') addressTable_uMNio += '/';
  addressTable_uMNio+="uMNio.xml";

  std::string location="chtcp-2.0://localhost:10203?target=";
  location+=target;
  location+=":50001";
  
  hw_=std::shared_ptr<::uhal::HwInterface>(new ::uhal::HwInterface(::uhal::ConnectionManager::getDevice(target, location, addressTable_uMNio)));
}

uhalWishboneInterface::~uhalWishboneInterface() {
}
  void uhalWishboneInterface::dispatch() { hw_->dispatch(); }
  /**
   * write a 32-bit word to the given target and address
   * @throws pflib::Exception in the case of a timeout or wishbone error
   */
void uhalWishboneInterface::wb_write(int target, uint32_t addr, uint32_t data) {
  const ::uhal::Node& m_base(hw_->getNode("LDMX"));
  
  m_base.getNode("WISHBONE.WB_WRITE_DATA").write(data);
  m_base.getNode("WISHBONE.WB_ADDR").write(addr);
  m_base.getNode("WISHBONE.WB_TARGET").write(target);
  m_base.getNode("WISHBONE.WB_WE").write(1);
  m_base.getNode("WISHBONE.START").write(0x1);
  dispatch();
  ::uhal::ValWord<uint32_t> rv;
  int cycles=0;
  do {
    rv=m_base.getNode("WISHBONE.DONE").read();
    dispatch();
    cycles++;
    if (cycles>100 && rv.value()!=1) {
      PFEXCEPTION_RAISE("WishboneTimeout","Timeout on Wishbone bus");
    }
  } while (rv.value()!=1);
}
  
  /**
   * read a 32-bit word from the given target and address
   * @throws pflib::Exception in the case of a timeout or wishbone error
   */
uint32_t uhalWishboneInterface::wb_read(int target, uint32_t addr) {
  const ::uhal::Node& m_base(hw_->getNode("LDMX"));
  m_base.getNode("WISHBONE.WB_ADDR").write(addr);
  m_base.getNode("WISHBONE.WB_TARGET").write(target);
  m_base.getNode("WISHBONE.WB_WE").write(0);
  m_base.getNode("WISHBONE.START").write(0x1);
  dispatch();
  ::uhal::ValWord<uint32_t> rv;
  int cycles=0;
  do {
    rv=m_base.getNode("WISHBONE.DONE").read();
    dispatch();
    cycles++;
    if (cycles>100 && rv.value()!=1) {
      PFEXCEPTION_RAISE("WishboneTimeout","Timeout on Wishbone bus");
    }
  } while (rv.value()!=1);
  rv=m_base.getNode("WISHBONE.WB_READ_DATA").read();
  dispatch();
  return rv.value();
}
  
  /**
   * reset the wishbone bus (on/off cycle)
   */
void uhalWishboneInterface::wb_reset() {
}

  /** 
   * Read the monitoring counters
   *  crcup_errors -- CRC errors observed on the uplink (wraps)
   *  crcdn_errors -- CRC errors from the downlink reported on the uplink (wraps)
   *  wb_errors -- Wishbone errors indicated by the Polarfire
   */
void uhalWishboneInterface::wb_errors(uint32_t& crcup_errors, uint32_t& crcdn_errors, uint32_t& wb_errors) {
}

  /**
   * Clear the monitoring counters
   */
void uhalWishboneInterface::wb_clear_counters() {
}


  /// Backend implementation
void uhalWishboneInterface::fc_sendL1A() {
  const ::uhal::Node& m_base(hw_->getNode("LDMX"));
  m_base.getNode("FAST_CONTROL.SEND_L1A").write(1);
  dispatch();
}
void uhalWishboneInterface::fc_linkreset() {
  const ::uhal::Node& m_base(hw_->getNode("LDMX"));
  m_base.getNode("FAST_CONTROL.SEND_LINK_RESET").write(1);
  dispatch();  
}
void uhalWishboneInterface::fc_bufferclear() {
  const ::uhal::Node& m_base(hw_->getNode("LDMX"));
  m_base.getNode("FAST_CONTROL.SEND_BUFFER_CLEAR").write(1);
  dispatch();
}
void uhalWishboneInterface::fc_calibpulse() {
  const ::uhal::Node& m_base(hw_->getNode("LDMX"));
  m_base.getNode("FAST_CONTROL.SEND_CALIB_REQ").write(1);
  dispatch();
}
void uhalWishboneInterface::fc_setup_calib(int pulse_len, int l1a_offset) {
  const ::uhal::Node& m_base(hw_->getNode("LDMX"));
  m_base.getNode("FAST_CONTROL.CALIB_PULSE_LEN").write(pulse_len&0xF);
  m_base.getNode("FAST_CONTROL.CALIB_L1A_OFFSET").write(pulse_len&0xF);
  dispatch();  
}
void uhalWishboneInterface::fc_get_setup_calib(int& pulse_len, int& l1a_offset) {
  const ::uhal::Node& m_base(hw_->getNode("LDMX"));
  ::uhal::ValWord<uint32_t> pl=m_base.getNode("FAST_CONTROL.CALIB_PULSE_LEN").read();
  ::uhal::ValWord<uint32_t> lo=m_base.getNode("FAST_CONTROL.CALIB_L1A_OFFSET").read();
  dispatch();
  pulse_len=pl;
  l1a_offset=lo;
}

void uhalWishboneInterface::daq_reset() {
  const ::uhal::Node& m_base(hw_->getNode("LDMX"));
  m_base.getNode("DAQ.DAQ_CLEAR").write(1);
  dispatch();
}
void uhalWishboneInterface::daq_advance_ptr() {
  const ::uhal::Node& m_base(hw_->getNode("LDMX"));
  m_base.getNode("DAQ.ADVANCE_READ_PTR").write(1);
  dispatch();
}


void uhalWishboneInterface::daq_status(bool& full, bool& empty, int& nevents, int& next_event_size) {
  const ::uhal::Node& m_base(hw_->getNode("LDMX"));
  ::uhal::ValWord<uint32_t> bf,be,n,next_size;
  bf=m_base.getNode("DAQ.FULL").read();
  be=m_base.getNode("DAQ.EMPTY").read();
  n=m_base.getNode("DAQ.NEVENTS").read();
  next_size=m_base.getNode("DAQ.EVENT_LENGTH").read();
  dispatch();
  full=(bf!=0);
  empty=(be!=0);
  nevents=n;
  next_event_size=next_size;
}
std::vector<uint32_t> uhalWishboneInterface::daq_read_event() {
  const ::uhal::Node& m_base(hw_->getNode("LDMX"));
  ::uhal::ValWord<uint32_t> next_size;
  next_size=m_base.getNode("DAQ.EVENT_LENGTH").read();
  dispatch();
  ::uhal::ValVector<uint32_t> buffer;
  buffer=m_base.getNode("DAQ.BUFFER").readBlock(int(next_size));
  dispatch();
  return buffer.value();
}

}
}
