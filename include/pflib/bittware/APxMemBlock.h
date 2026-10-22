#pragma once
#ifndef PFLIB_BITTWARE_APXMEMBLOCK_H
#define PFLIB_BITTWARE_APXMEMBLOCK_H

#include <rogue/protocols/udp/Client.h>
#include <rogue/protocols/rssi/Client.h>
#include <rogue/protocols/packetizer/CoreV2.h>
#include <rogue/protocols/srp/SrpV3.h>
#include <rogue/interfaces/memory/Block.h>

namespace pflib::bittware {

/**
 * block of memory representing firmware configuration registers on the APx
 */
class APxMemBlock {
  /// UDP client defining host/port
  rogue::protocols::udp::ClientPtr udp_;
  /// RSSI client defining network frame width
  rogue::protocols::rssi::ClientPtr rssi_;
  /// packetizer defining how to construct network packet
  rogue::protocols::packetizer::CoreV2Ptr pack_;
  /// SRPv3 stream frames for SRP hardware endpoints
  rogue::protocols::srp::SrpV3Ptr srp_;
  /// memory block interface to represent memory on APx here in software
  rogue::interfaces::memory::BlockPtr mem_;
 public:
  APxMemBlock(
      const std::string& hostname,
      int port
  );
};

}

#endif
