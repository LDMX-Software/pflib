#pragma once

#include <fstream>
#include <memory>
#include <functional>

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

/**
 * Write all channels into a CSV
 */
DecodeAndWriteToCSV all_channels_to_csv(const std::string& file_name);

/**
 * Specialization of DecodeAndWrite which holds a table of all samples
 * and then allows the user decide how to write this table at the end of
 * the run.
 *
 * If you want to write out all the samples anyways, it is more efficient
 * to use DecodeAndWriteToCSV. This is helpful for if you want to condense
 * the samples with some summary statistics (e.g. median or mean).
 */
class DecodeAndWriteRunSummary : public DecodeAndWrite {
  /// output file writing to
  std::ofstream file_;
  /// function that writes the rows given the table of samples from the last run
  std::function<void(std::ofstream&, const std::array<std::vector<pflib::packing::Sample>, 72>&)> write_row_;
  /// table of samples we accrue with data
  std::array<std::vector<pflib::packing::Sample>, 72> data_;
  /// index of current event in table
  std::size_t i_event_;
 public:
  DecodeAndWriteRunSummary(
      const std::string& file_name,
      int n_events,
      std::function<void(std::ofstream&)> write_header,
      std::function<void(std::ofstream&, const std::array<std::vector<pflib::packing::Sample>, 72>&)> write_row
  );
  virtual ~DecodeAndWriteRunSummary() = default;
  /// reset table of samples
  virtual void start_run() final;
  /// add event into our table of samples
  virtual void write_event(const pflib::packing::SingleROCEventPacket& ep) final;
  /// call write_row on the table of samples
  virtual void end_run() final;
};

}
