#pragma once

#include <functional>
#include <memory>
#include <vector>

#include "pflib/DecodeAndWrite.h"
#include "pflib/Logging.h"
#include "pflib/Target.h"
#include "pflib/packing/SingleROCEventPacket.h"

namespace pflib {

/**
 * Consume an event packet, decode it, and save to buffer.
 * The constructor takes an argument of the size of the buffer which should
 * be the number of events being collected in the run(s) where this buffer is used.
 *
 * The buffer can be read out with get_buffer().
 * The buffer is cleared upon the start of every run.
 *
 * Avoid copying this potentially large buffer around by using constant references.
 * ```cpp
 * // after daq_run has filled the DecodeAndBuffer object 'buffer'
 * const auto& events{buffer.get_buffer()};
 * ```
 */
class DecodeAndBuffer : public DecodeAndWrite {
 public:
  DecodeAndBuffer(int nevents);
  virtual ~DecodeAndBuffer() = default;
  /// get buffer
  const std::vector<pflib::packing::SingleROCEventPacket>& get_buffer() const;
  /// Set the buffer size
  void set_buffer_size(int nevents);
  /// Save to buffer
  virtual void write_event(
      const pflib::packing::SingleROCEventPacket& ep) override;
  /// Check that the buffer was read and flushed since last run
  virtual void start_run() override;

 private:
  /// Buffer for event packets
  std::vector<pflib::packing::SingleROCEventPacket> ep_buffer_;
};

}  // namespace pflib
