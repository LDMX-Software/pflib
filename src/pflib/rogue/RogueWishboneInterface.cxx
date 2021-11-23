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


void RogueWishboneInterface::fc_sendL1A() {
  wb_write(TARGET_FC_BACKEND,REG_FC_SINGLES,MASK_FC_SINGLE_L1A);
}
void RogueWishboneInterface::fc_linkreset() {
  wb_write(TARGET_FC_BACKEND,REG_FC_SINGLES,MASK_FC_SINGLE_LINKRESET);
}
void RogueWishboneInterface::fc_bufferclear() {
  wb_write(TARGET_FC_BACKEND,REG_FC_SINGLES,MASK_FC_SINGLE_BUFFERRESET);
}

static const int REG_DAQ_CTL                             = 1;
static const int MASK_DAQ_RESET                          = 0x1;
static const int MASK_DAQ_ADVANCEPTR                     = 0x2;
static const int REG_DAQ_STATUS                          = 65;
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
}
}
