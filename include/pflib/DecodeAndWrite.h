#pragma once

#include <fstream>
#include <functional>
#include <memory>
#include <vector>

#include "pflib/Logging.h"
#include "pflib/Target.h"
#include "pflib/packing/SingleROCEventPacket.h"

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

 protected:
  /// logging for warning messages on empty events
  mutable ::pflib::logging::logger the_log_;

 private:
  /// event packet for decoding
  pflib::packing::SingleROCEventPacket ep_;
};

/**
 * specializatin of DecodeAndWrite that holds a std::ofstream
 * for the user with functions for writing the header and events
 */
class DecodeAndWriteToCSV : public DecodeAndWrite {
  /// output file writing to
  std::ofstream file_;
  /// function that writes row(s) to csv given an event
  std::function<void(std::ofstream&,
                     const pflib::packing::SingleROCEventPacket&)>
      write_event_;

 public:
  DecodeAndWriteToCSV(
      const std::string& file_name,
      std::function<void(std::ofstream&)> write_header,
      std::function<void(std::ofstream&,
                         const pflib::packing::SingleROCEventPacket&)>
          write_event);
  virtual ~DecodeAndWriteToCSV() = default;
  /// call write_event with our file handle
  virtual void write_event(
      const pflib::packing::SingleROCEventPacket& ep) final;
};

DecodeAndWriteToCSV all_channels_to_csv(const std::string& file_name);

}  // namespace pflib
