
#include "pflib/rogue/DMAReadoutReceiver.h"

#include <rogue/interfaces/stream/Frame.h>
#include <rogue/interfaces/stream/FrameIterator.h>
#include <rogue/interfaces/stream/FrameLock.h>

namespace pflib {
namespace rogue {

DMAReadoutReceiver::DMAReadoutReceiver() : ::rogue::interfaces::stream::Slave() {}
DMAReadoutReceiver::~DMAReadoutReceiver() {}

void DMAReadoutReceiver::acceptFrame(std::shared_ptr<::rogue::interfaces::stream::Frame> frame) {
  std::vector<uint32_t> data;
  { // lock frame while we copy it to our buffer
    auto lock = frame->lock();
    std::copy(frame->begin(), frame->end(), data.begin());
  }

  // process the decoded event data
  this->process(pflib::decoding::SuperPacket(&(data[0]),data.size()));
}

} // namespace rogue
} // namespace pflib

