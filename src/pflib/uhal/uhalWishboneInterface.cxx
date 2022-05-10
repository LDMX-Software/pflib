#include "uhal/HwInterface.hpp"
#include "uhal/Node.hpp"
#include "uhal/ConnectionManager.hpp"
#include "pflib/uhal/uhalWishboneInterface.h"
     
namespace pflib {
namespace uhal {

  /** Construct a TCP bridge*/
uhalWishboneInterface::uhalWishboneInterface(
    const std::string& target, 
    const std::string& ipbus_map_path) {
  std::string addressTable_uMNio("file://");
  addressTable_uMNio+=ipbus_map_path;
  if (not ipbus_map_path.empty() and ipbus_map_path.back() != '/') addressTable_uMNio += '/';
  addressTable_uMNio+="uMNio.xml";

  std::string location="chtcp-2.0://localhost:10203?target=";
  location+=target;
  location+=":50001";

  // raise logging threshold to errors
  ::uhal::setLogLevelTo(::uhal::Error());
  
  hw_=std::make_unique<::uhal::HwInterface>(
      ::uhal::ConnectionManager::getDevice(target, location, addressTable_uMNio));
}

uhalWishboneInterface::~uhalWishboneInterface() {}

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
void uhalWishboneInterface::wb_reset() {}

/** 
 * Read the monitoring counters
 *  crcup_errors -- CRC errors observed on the uplink (wraps)
 *  crcdn_errors -- CRC errors from the downlink reported on the uplink (wraps)
 *  wb_errors -- Wishbone errors indicated by the Polarfire
 */
void uhalWishboneInterface::wb_errors(uint32_t& crcup_errors, 
    uint32_t& crcdn_errors, uint32_t& wb_errors) {}

/**
 * Clear the monitoring counters
 */
void uhalWishboneInterface::wb_clear_counters() {}


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
  m_base.getNode("FAST_CONTROL.CALIB_L1A_OFFSET").write(l1a_offset&0xFF);
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

void uhalWishboneInterface::fc_read_counters(int&  spill_count, int& header_occ, int& header_occ_max, int& event_count, int& vetoed_counter) {
  const ::uhal::Node& m_base(hw_->getNode("LDMX"));
  
  ::uhal::ValWord<uint32_t> occ = m_base.getNode("FAST_CONTROL.OCC").read();
  ::uhal::ValWord<uint32_t> occ_max = m_base.getNode("FAST_CONTROL.OCC_MAX").read();
  ::uhal::ValWord<uint32_t> sc = m_base.getNode("FAST_CONTROL.SPILL_COUNT").read();
  ::uhal::ValWord<uint32_t> ec = m_base.getNode("FAST_CONTROL.EVENT_COUNT").read();
  ::uhal::ValWord<uint32_t> veto = m_base.getNode("FAST_CONTROL.VETO").read();

  dispatch();

  header_occ=occ;
  header_occ_max=occ_max;
  spill_count = sc;
  event_count = ec;
  vetoed_counter = veto;
}

void uhalWishboneInterface::fc_clear_run() {
  const ::uhal::Node& m_base(hw_->getNode("LDMX"));
  m_base.getNode("FAST_CONTROL.CLEAR_FIFO").write(0x1);
  dispatch();
}

void uhalWishboneInterface::fc_enables_read(bool& ext_l1a, bool& ext_spill, bool& timer_l1a) {
  const ::uhal::Node& m_base(hw_->getNode("LDMX"));

  ::uhal::ValWord<uint32_t> reg = m_base.getNode("FAST_CONTROL.ENABLES").read();
  dispatch();

  const int MASK_FC_CONTROL_ENABLE_EXT_L1A = 0x02;
  const int MASK_FC_CONTROL_ENABLE_EXT_SPILL = 0x01;
  const int MASK_FC_CONTROL_ENABLE_TIMER_L1A = 0x04;
  
  ext_l1a=(reg&MASK_FC_CONTROL_ENABLE_EXT_L1A);
  ext_spill=(reg&MASK_FC_CONTROL_ENABLE_EXT_SPILL);
  timer_l1a=(reg&MASK_FC_CONTROL_ENABLE_TIMER_L1A);

}

void uhalWishboneInterface::fc_enables(bool ext_l1a, bool ext_spill, bool timer_l1a) {

  const ::uhal::Node& m_base(hw_->getNode("LDMX"));

  ::uhal::ValWord<uint32_t> regval = m_base.getNode("FAST_CONTROL.ENABLES").read();
  dispatch();

  uint32_t reg = regval;

  const int MASK_FC_CONTROL_ENABLE_EXT_L1A = 0x02;
  const int MASK_FC_CONTROL_ENABLE_EXT_SPILL = 0x01;
  const int MASK_FC_CONTROL_ENABLE_TIMER_L1A = 0x04;

  reg|=MASK_FC_CONTROL_ENABLE_EXT_L1A;
  if (!ext_l1a) reg^=MASK_FC_CONTROL_ENABLE_EXT_L1A;

  reg|=MASK_FC_CONTROL_ENABLE_EXT_SPILL;
  if (!ext_spill) reg^=MASK_FC_CONTROL_ENABLE_EXT_SPILL;

  reg|=MASK_FC_CONTROL_ENABLE_TIMER_L1A;
  if (!timer_l1a) reg^=MASK_FC_CONTROL_ENABLE_TIMER_L1A;

  m_base.getNode("FAST_CONTROL.ENABLES").write(reg);
  dispatch();
}

//time setup read

//timer setup

void uhalWishboneInterface::fc_veto_setup_read(bool& veto_daq_busy, bool& veto_l1_occ, int& l1_occ_busy, int& l1_occ_ok) {
  const ::uhal::Node& m_base(hw_->getNode("LDMX"));

  ::uhal::ValWord<uint32_t> vetoregval = m_base.getNode("FAST_CONTROL.VETO_SETUP").read();
  dispatch();

  uint32_t vetoreg = vetoregval;

  const int MASK_FC_CONTROL_ENABLE_VETO_BUSY_DAQ  = 0x08;
  const int MASK_FC_CONTROL_ENABLE_VETO_HEADEROCC = 0x10;

  veto_daq_busy=(vetoreg&MASK_FC_CONTROL_ENABLE_VETO_BUSY_DAQ)!=0;
  veto_l1_occ=(vetoreg&MASK_FC_CONTROL_ENABLE_VETO_HEADEROCC)!=0;

   ::uhal::ValWord<uint32_t> occregval = m_base.getNode("FAST_CONTROL.OCCVETO").read();
  dispatch();

  uint32_t occreg = occregval; 

  const int MASK_FC_OCCBUSY = 0xFF;
  const int SHIFT_FC_OCC_OK = 12;
  const int SHIFT_FC_OCCBUSY = 0;
  const int MASK_FC_OCC_OK = 0xFF;

  l1_occ_busy=(occreg>>SHIFT_FC_OCCBUSY)&MASK_FC_OCCBUSY;
  l1_occ_ok=(occreg>>SHIFT_FC_OCC_OK)&MASK_FC_OCC_OK;

}

//veto setup
void uhalWishboneInterface::fc_veto_setup(bool veto_daq_busy, bool veto_l1_occ, int l1_occ_busy, int l1_occ_ok) {

  const ::uhal::Node& m_base(hw_->getNode("LDMX"));

  ::uhal::ValWord<uint32_t> regval = m_base.getNode("FAST_CONTROL.VETO_SETUP").read();
  dispatch();

  uint32_t reg = regval;

  const int MASK_FC_CONTROL_ENABLE_VETO_BUSY_DAQ  = 0x08;
  const int MASK_FC_CONTROL_ENABLE_VETO_HEADEROCC = 0x10;

  reg|=(MASK_FC_CONTROL_ENABLE_VETO_BUSY_DAQ|MASK_FC_CONTROL_ENABLE_VETO_HEADEROCC);
  if (!veto_daq_busy) reg^=MASK_FC_CONTROL_ENABLE_VETO_BUSY_DAQ;
  if (!veto_l1_occ) reg^=MASK_FC_CONTROL_ENABLE_VETO_HEADEROCC;
   
  m_base.getNode("FAST_CONTROL.VETO_SETUP").write(reg);
  dispatch();

  const int MASK_FC_OCCBUSY = 0xFF;
  const int SHIFT_FC_OCC_OK = 12;
  const int SHIFT_FC_OCCBUSY = 0;
  const int MASK_FC_OCC_OK = 0xFF;

  uint32_t occreg=((l1_occ_busy&MASK_FC_OCCBUSY)<<SHIFT_FC_OCCBUSY)|((l1_occ_ok&MASK_FC_OCC_OK)<<SHIFT_FC_OCC_OK);

  m_base.getNode("FAST_CONTROL.OCCVETO").write(occreg);
  dispatch(); 

}


void uhalWishboneInterface::fc_advance_l1_fifo() {
  const ::uhal::Node& m_base(hw_->getNode("LDMX"));
  m_base.getNode("FAST_CONTROL.ADVANCE_L1_FIFO").write(1);
  dispatch();
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
