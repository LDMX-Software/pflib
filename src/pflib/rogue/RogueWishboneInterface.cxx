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

}
}
