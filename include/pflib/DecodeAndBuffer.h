#pragma once

#include <memory>
#include <functional>
#include <vector>

#include "pflib/packing/SingleROCEventPacket.h"
#include "pflib/Logging.h"
#include "pflib/Target.h"

namespace pflib {

/**
 * Consume an event packet, decode it, and save to buffer.
 * The constructor takes an argument of the size of the buffer.
 *
 * The buffer can be read out with read_buffer(). The buffer is cleared upon the start
 * of every event.
 *
 */
class DecodeAndBuffer : public Target::DAQRunConsumer {
  public:
    // Constructor for allocating the size of the buffer.
    DecodeAndBuffer(int nevents);
    virtual ~DecodeAndBuffer() = default;
    /// Read out buffer
    virtual std::vector<pflib::packing::SingleROCEventPacket> read_and_flush();
    /// Save to buffer
    virtual void write_event(const pflib::packing::SingleROCEventPacket& ep) = 0;
    /// Consumer for events
    virtual void consume(std::vector<uint32_t>& event) final;
    /// Check that the buffer was read and flushed since last run
    virtual void start_run() override;
    /// Failcheck if the buffer is getting too big
    virtual void end_run() override;
  private:
    /// event packet for decoding
    pflib::packing::SingleROCEventPacket ep_;
    /// Buffer for event packets
    std::vector<pflib::packing::SingleROCEventPacket> ep_buffer_;
    /// Capacity of the buffer
    int buffer_size_;
    /// logging for warning messages
    mutable ::pflib::logging::logger the_log_;
};

}
