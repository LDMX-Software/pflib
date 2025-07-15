#pragma once

#include <memory>
#include <functional>
#include <vector>

#include "pflib/packing/SingleROCEventPacket.h"
#include "pflib/Logging.h"
#include "pflib/Target.h"
#include "pflib/DecodeAndWrite.h"

namespace pflib {

/**
 * Consume an event packet, decode it, and save to buffer.
 * The constructor takes an argument of the size of the buffer.
 *
 * The buffer can be read out with read_buffer(). The buffer is cleared upon the start
 * of every event.
 *
 */
class DecodeAndBuffer : public DecodeAndWrite {
  public:
    DecodeAndBuffer(int nevents);
    virtual ~DecodeAndBuffer() = default;
    /// get buffer
    std::vector<pflib::packing::SingleROCEventPacket> get_buffer();
    /// Set the buffer size
    void set_buffer_size(int nevents);
    /// Save to buffer
    virtual void write_event(const pflib::packing::SingleROCEventPacket& ep) override;
    /// Read out buffer
    std::vector<pflib::packing::SingleROCEventPacket> read_buffer();
    /// Check that the buffer was read and flushed since last run
    virtual void start_run() override;
  private:
    /// event packet for decoding
    pflib::packing::SingleROCEventPacket ep_;
    /// Buffer for event packets
    std::vector<pflib::packing::SingleROCEventPacket> ep_buffer_;
    /// logging for warning messages
    mutable ::pflib::logging::logger the_log_;
};

}
