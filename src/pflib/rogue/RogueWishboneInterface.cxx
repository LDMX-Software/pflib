#include "pflib/rogue/RogueWishboneInterface.h"
#include <rogue/interfaces/memory/Constants.h>
#include <rogue/interfaces/memory/Master.h>
#include <rogue/interfaces/memory/TcpClient.h>

namespace pflib {
namespace rogue {


RogueWishboneInterface::RogueWishboneInterface(const std::string& host, int port) {
  intf_=::rogue::interfaces::memory::Master::create();
  client_=::rogue::interfaces::memory::TcpClient::create(host,port);
  intf_->setSlave(client_);

  // initial pump..  why this is needed, I don't understand...
  try {
    wb_read(0,0);
  } catch(...) {
  }
  
}

RogueWishboneInterface::~RogueWishboneInterface() {
  client_->stop();
  intf_->stop();
}

void RogueWishboneInterface::wb_write(int target, uint32_t addr, uint32_t data) {
  uint64_t fullAddr=(((uint64_t)(target))<<32) | (addr);

  intf_->clearError();
  uint32_t id=intf_->reqTransaction(fullAddr,4,&data,::rogue::interfaces::memory::Write);
  intf_->waitTransaction(id);
  if (intf_->getError()!="") {
    PFEXCEPTION_RAISE("RogueWishboneInterface",intf_->getError().c_str());
  }
}

uint32_t RogueWishboneInterface::wb_read(int target, uint32_t addr) {
  uint32_t result;
  uint64_t fullAddr=(((uint64_t)(target))<<32) | (addr);

  intf_->clearError();
  uint32_t id=intf_->reqTransaction(fullAddr,4,&result,::rogue::interfaces::memory::Read);
  intf_->waitTransaction(id);
  if (intf_->getError()!="") {
    PFEXCEPTION_RAISE("RogueWishboneInterface",intf_->getError().c_str());
  }
  return result;
}
void RogueWishboneInterface::wb_reset() {
}

void RogueWishboneInterface::wb_errors(uint32_t& crcup_errors, uint32_t& crcdn_errors, uint32_t& wb_errors) {
}

  /**
   * Clear the monitoring counters
   */
void RogueWishboneInterface::wb_clear_counters() {
}

static const int TARGET_FC_BACKEND = 257;
static const int TARGET_DAQ_BACKEND = 258;

static const int REG_FC_SINGLES                        = 1;
static const int MASK_FC_SINGLE_L1A                    = 0x1;
static const int MASK_FC_SINGLE_LINKRESET              = 0x2;
static const int MASK_FC_SINGLE_BUFFERRESET            = 0x4;
static const int MASK_FC_SINGLE_CALIBPULSE             = 0x8;
static const int REG_FC_SETUP                          = 2;
static const int SHIFT_FC_SETUP_CALIB_PULSELEN         = 24;
static const int MASK_FC_SETUP_CALIB_PULSELEN          = 0xF;
static const int SHIFT_FC_SETUP_CALIB_L1AOFFSET        = 16;
static const int MASK_FC_SETUP_CALIB_L1AOFFSET         = 0xFF;
  
void RogueWishboneInterface::fc_sendL1A() {
  wb_write(TARGET_FC_BACKEND,REG_FC_SINGLES,MASK_FC_SINGLE_L1A);
}
void RogueWishboneInterface::fc_linkreset() {
  wb_write(TARGET_FC_BACKEND,REG_FC_SINGLES,MASK_FC_SINGLE_LINKRESET);
}
void RogueWishboneInterface::fc_bufferclear() {
  wb_write(TARGET_FC_BACKEND,REG_FC_SINGLES,MASK_FC_SINGLE_BUFFERRESET);
}
void RogueWishboneInterface::fc_calibpulse() {
  wb_write(TARGET_FC_BACKEND,REG_FC_SINGLES,MASK_FC_SINGLE_CALIBPULSE);
}
void RogueWishboneInterface::fc_setup_calib(int pulse_len, int l1a_offset) {
  uint32_t reg=wb_read(TARGET_FC_BACKEND,REG_FC_SETUP);
  // remove old values
  reg=reg&(0xFFFFFFFFu^(MASK_FC_SETUP_CALIB_PULSELEN<<SHIFT_FC_SETUP_CALIB_PULSELEN));
  reg=reg&(0xFFFFFFFFu^(MASK_FC_SETUP_CALIB_L1AOFFSET<<SHIFT_FC_SETUP_CALIB_L1AOFFSET));
  // add new values
  reg=reg|((pulse_len&MASK_FC_SETUP_CALIB_PULSELEN)<<SHIFT_FC_SETUP_CALIB_PULSELEN);
  reg=reg|((l1a_offset&MASK_FC_SETUP_CALIB_L1AOFFSET)<<SHIFT_FC_SETUP_CALIB_L1AOFFSET);
  wb_write(TARGET_FC_BACKEND,REG_FC_SETUP,reg);
}
void RogueWishboneInterface::fc_get_setup_calib(int& pulse_len, int& l1a_offset) {
  uint32_t reg=wb_read(TARGET_FC_BACKEND,REG_FC_SETUP);
  pulse_len=(reg>>SHIFT_FC_SETUP_CALIB_PULSELEN)&MASK_FC_SETUP_CALIB_PULSELEN;
  l1a_offset=(reg>>SHIFT_FC_SETUP_CALIB_L1AOFFSET)&MASK_FC_SETUP_CALIB_L1AOFFSET;
}

  
static const int REG_DAQ_SETUP                           = 0;
static const int MASK_DAQ_DMA_ENABLE                     = 0x10;
static const int MASK_FPGA_ID                            = 0xFF00;
static const int SHIFT_FPGA_ID                           = 8;
static const int MASK_SAMPLES_PER_EVENT                  = 0xF0000;
static const int SHIFT_SAMPLES_PER_EVENT                  = 16;

static const int REG_DAQ_CTL                             = 1;
static const int MASK_DAQ_RESET                          = 0x1;
static const int MASK_DAQ_ADVANCEPTR                     = 0x2;
static const int REG_DAQ_STATUS                          = 65;
static const int REG_DAQ_DMA_STATUS                      = 67;
static const uint32_t MASK_DAQ_STATUS_FULL               = 0x2;
static const uint32_t MASK_DAQ_STATUS_EMPTY              = 0x1;
static const uint32_t MASK_DAQ_EVENTS                    = 0x1FF0;
static const uint32_t SHIFT_DAQ_EVENTS                   = 4;
static const uint32_t MASK_DAQ_NEXT_SIZE                 = 0x7FF0000;
static const uint32_t SHIFT_DAQ_NEXT_SIZE                = 16;
static const uint32_t BASE_DAQ_BUFFER                    = 0x800;

void RogueWishboneInterface::daq_reset() {
  wb_write(TARGET_DAQ_BACKEND,REG_DAQ_CTL,MASK_DAQ_RESET);
}
void RogueWishboneInterface::daq_advance_ptr() {
  wb_write(TARGET_DAQ_BACKEND,REG_DAQ_CTL,MASK_DAQ_ADVANCEPTR);
}
void RogueWishboneInterface::daq_status(bool& full, bool& empty, int& nevents, int& next_event_size) {
  uint32_t status=wb_read(TARGET_DAQ_BACKEND,REG_DAQ_STATUS);
  full=(status&MASK_DAQ_STATUS_FULL)!=0;
  empty=(status&MASK_DAQ_STATUS_EMPTY)!=0;
  nevents=(status&MASK_DAQ_EVENTS)>>SHIFT_DAQ_EVENTS;
  next_event_size=(status&MASK_DAQ_NEXT_SIZE)>>SHIFT_DAQ_NEXT_SIZE;
}
std::vector<uint32_t> RogueWishboneInterface::daq_read_event() {
  uint32_t status=wb_read(TARGET_DAQ_BACKEND,REG_DAQ_STATUS);
  int next_event_size=(status&MASK_DAQ_NEXT_SIZE)>>SHIFT_DAQ_NEXT_SIZE;
  std::vector<uint32_t> x(next_event_size,0);

  uint64_t fullAddr=(((uint64_t)(TARGET_DAQ_BACKEND))<<32) | (BASE_DAQ_BUFFER);

  intf_->clearError();
  uint32_t id=intf_->reqTransaction(fullAddr,next_event_size*4,&(x[0]),::rogue::interfaces::memory::Read);
  intf_->waitTransaction(id);
  if (intf_->getError()!="") {
    PFEXCEPTION_RAISE("RogueWishboneInterface",intf_->getError().c_str());
  }

  return x;
}

void RogueWishboneInterface::daq_dma_enable(bool enable) {
  uint32_t ctl=wb_read(TARGET_DAQ_BACKEND,REG_DAQ_SETUP);
  ctl|= MASK_DAQ_DMA_ENABLE;
  if (!enable) ctl^=MASK_DAQ_DMA_ENABLE;
  wb_write(TARGET_DAQ_BACKEND,REG_DAQ_SETUP,ctl);
}
void RogueWishboneInterface::daq_dma_setup(uint8_t fpga_id, uint8_t samples_per_event) {
  uint32_t ctl=wb_read(TARGET_DAQ_BACKEND,REG_DAQ_SETUP);
  // zero the bits
  ctl|=(MASK_FPGA_ID|MASK_SAMPLES_PER_EVENT);
  ctl^=(MASK_FPGA_ID|MASK_SAMPLES_PER_EVENT);
  //load the values
  ctl|=(fpga_id<<SHIFT_FPGA_ID)&MASK_FPGA_ID;
  ctl|=(samples_per_event<<SHIFT_SAMPLES_PER_EVENT)&MASK_SAMPLES_PER_EVENT;
  wb_write(TARGET_DAQ_BACKEND,REG_DAQ_SETUP,ctl);
}
void RogueWishboneInterface::daq_get_dma_setup(uint8_t& fpga_id, uint8_t& samples_per_event, bool& enabled) {
  uint32_t ctl=wb_read(TARGET_DAQ_BACKEND,REG_DAQ_SETUP);
  enabled=(ctl&MASK_DAQ_DMA_ENABLE)!=0;
  samples_per_event=(ctl&MASK_SAMPLES_PER_EVENT)>>SHIFT_SAMPLES_PER_EVENT;
  fpga_id=(ctl&MASK_FPGA_ID)>>SHIFT_FPGA_ID;
}
uint32_t RogueWishboneInterface::daq_dma_status() {
   return wb_read(TARGET_DAQ_BACKEND,REG_DAQ_DMA_STATUS);
}


}
}