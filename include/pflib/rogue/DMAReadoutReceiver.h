#ifndef PFLIB_ROGUE_DMAREADOUTRECEIVER_H
#define PFLIB_ROGUE_DMAREADOUTRECEIVER_H

#include <rogue/interfaces/stream/Slave.h>

#include "pflib/decoding/SuperPacket.h"

namespace pflib {
namespace rogue {

/**
 * DMA Readout mode has a pipeline that pushes the data down rogue
 * streams, in order for pftool to be able to access this data
 * on an event-by-event basis, we need to add a section of piping
 * that we can control.
 */
class DMAReadoutReceiver : public ::rogue::interfaces::stream::Slave {
 public:
  /// default constructor
  DMAReadoutReceiver();
  /// default destructor, virtual for derived classes
  virtual ~DMAReadoutReceiver();
  /**
   * accept the frame, lock it, copy it into a format that SuperPacket
   * can decode, then provide the data to process
   */
  void acceptFrame(std::shared_ptr<::rogue::interfaces::stream::Frame> frame);
  /**
   * do what you want to do with this event data
   *
   * This processing should be kept as short as possible since
   * it needs to complete before the next event can be acquired.
   */
  virtual void process(pflib::decoding::SuperPacket data) = 0;
};  // DMAReadoutReceiver

} // namespace rogue
} // namespace pflib

#endif
