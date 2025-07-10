#pragma once

#include <fstream>
#include <memory>
#include <functional>
#include <vector>

#include "pflib/packing/SingleROCEventPacket.h"
#include "pflib/Logging.h"
#include "pflib/Target.h"

namespace pflib {

/**
 * Consume an event packet, decode it, and then do something else
 *
 * This class holds the event packet for the user so that
 * other code just needs to write functions that define how the
 * decoded data should be written out.
 */
class DecodeAndWrite : public Target::DAQRunConsumer {
 public:
  virtual ~DecodeAndWrite() = default;
  /**
   * Decode the input event packet into our pflib::packing::SingleROCEventPacket
   * and then call write_event on it.
   */
  virtual void consume(std::vector<uint32_t>& event) final;

  /// pure virtual function for writing out decoded event
  virtual void write_event(const pflib::packing::SingleROCEventPacket& ep) = 0;

 private:
  /// event packet for decoding
  pflib::packing::SingleROCEventPacket ep_;
  /// logging for warning messages on empty events
  mutable ::pflib::logging::logger the_log_;
};

/**
 * specializatin of DecodeAndWrite that holds a std::ofstream
 * for the user with functions for writing the header and events
 */
class DecodeAndWriteToCSV : public DecodeAndWrite {
  /// output file writing to
  std::ofstream file_;
  /// function that writes row(s) to csv given an event
  std::function<void(std::ofstream&, const pflib::packing::SingleROCEventPacket&)> write_event_;
 public:
  DecodeAndWriteToCSV(
    const std::string& file_name,
    std::function<void(std::ofstream&)> write_header,
    std::function<void(std::ofstream&, const pflib::packing::SingleROCEventPacket&)> write_event
  );
  virtual ~DecodeAndWriteToCSV() = default;
  /// call write_event with our file handle
  virtual void write_event(const pflib::packing::SingleROCEventPacket& ep) final;
};

DecodeAndWriteToCSV all_channels_to_csv(const std::string& file_name);

class DecodeAndWriteToBuffer : public DecodeAndWrite {
  public:
    DecodeAndWriteToBuffer();
    virtual ~DecodeAndWriteToBuffer() = default;
    /// Read out buffer
    virtual std::vector<pflib::packing::SingleROCEventPacket> read_data();
    /// Save to buffer
    virtual void write_event(const pflib::packing::SingleROCEventPacket& ep) final;
    // Safecheck if the buffer is getting too big (it's not being read out)
    virtual void end_run() override;
  private:
    /// Buffer to save event packets to
    std::vector<pflib::packing::SingleROCEventPacket> ep_buffer_;
};

}
