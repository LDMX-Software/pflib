#include "pflib/bittware/APxMemBlock.h"

#include <rogue/protocols/packetizer/Application.h>
#include <rogue/protocols/packetizer/Transport.h>
#include <rogue/protocols/rssi/Transport.h>
#include <rogue/Helpers.h>

namespace pflib::bittware {

// offset applied to all registers we will use
static const uint64_t HCAL_APP_GLOBAL_OFFSET = 0x80000000;
// TODO: deduce size of Hcal section of APx firmware registers
static const uint32_t HCAL_APP_GLOBAL_SIZE   = 1;

APxMemBlock::APxMemBlock(
    const std::string& hostname,
    int port
) {
  udp_ = rogue::protocols::udp::Client::create(hostname, port, true /* jumbo frames */);
  rssi_ = rogue::protocols::rssi::Client::create(udp_->maxPayload() - 8);
  pack_ = rogue::protocols::packetizer::CoreV2::create(
      false /* in CRC check */,
      true /* out CRC gen */,
      true /* en SSI framing */
  );
  srp_ = rogue::protocols::srp::SrpV3::create();
  mem_ = rogue::interfaces::memory::Block::create(HCAL_APP_GLOBAL_OFFSET, HCAL_APP_GLOBAL_SIZE);

  // make connections
  // UDP <-> RSSI <-> Packetizer <-> SRP Format <- Mem Block
  rogueStreamConnectBiDir(udp_, rssi_->transport());
  rogueStreamConnectBiDir(rssi_->transport(), pack_->transport());
  rogueStreamConnectBiDir(pack_->application(0), srp_);
  rogueBusConnect(mem_, srp_);
}

}
