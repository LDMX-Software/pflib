#pragma once

#include <stdio.h>

#include <fstream>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "pflib/Target.h"
#include "pflib/logging/Logging.h"
#include "pflib/packing/MultiSampleECONDEventPacket.h"
#include "pflib/packing/SingleROCEventPacket.h"

/**
 * Abstract base class for consuming event packets
 */
class DAQRunConsumer {
 public:
  virtual void start_run() {}
  virtual void end_run() {}
  virtual void consume(std::vector<uint32_t>& event) = 0;
  virtual ~DAQRunConsumer() = default;
};

/**
 * Do a DAQ run
 *
 * @param[in] cmd PEDESTAL, CHARGE, or LED (type of trigger to send)
 * @param[in] consumer DAQRunConsumer that handles the readout event packets
 * (probably writes them to a file or something like that)
 * @param[in] nevents number of events to collect
 * @param[in] rate how fast to collect events, default 100
 */
void daq_run(pflib::Target* tgt, const std::string& cmd,
             DAQRunConsumer& consumer, int nevents, int rate = 100);

/**
 * just copy input event packets to the output file as binary
 */
class WriteToBinaryFile : public DAQRunConsumer {
  std::string file_name_;
  FILE* fp_;

 public:
  WriteToBinaryFile(const std::string& file_name);
  ~WriteToBinaryFile();
  virtual void consume(std::vector<uint32_t>& event) final;
};

/**
 * Consume an event packet, decode it, and then do something else
 *
 * This class holds the event packet for the user so that
 * other code just needs to write functions that define how the
 * decoded data should be written out.
 */
template <class EventPacket>
class DecodeAndWrite : public DAQRunConsumer {
 public:
  /**
   * @param[in] n_links number of links is necessary for the ECOND event packet
   * but for the SingleROC it is always 2 (both halves) and is therefore ignored
   */
  explicit DecodeAndWrite(int n_links);
  virtual ~DecodeAndWrite() = default;
  /**
   * Decode the input event packet into our pflib::packing::SingleROCEventPacket
   * and then call write_event on it.
   */
  virtual void consume(std::vector<uint32_t>& event) final;

  /// pure virtual function for writing out decoded event
  virtual void write_event(const EventPacket& ep) = 0;

 protected:
  /// logging for warning messages on empty events
  mutable ::pflib::logging::logger the_log_;

 private:
  /// event packet for decoding
  EventPacket ep_;
};

/**
 * specializatin of DecodeAndWrite that holds a std::ofstream
 * for the user with functions for writing the header and events
 */
template <class EventPacket>
class DecodeAndWriteToCSV : public DecodeAndWrite<EventPacket> {
  /// output file writing to
  std::ofstream file_;
  /// function that writes row(s) to csv given an event
  std::function<void(std::ofstream&, const EventPacket&)> write_event_;

 public:
  DecodeAndWriteToCSV(
      const std::string& file_name,
      std::function<void(std::ofstream&)> write_header,
      std::function<void(std::ofstream&, const EventPacket&)> write_event,
      int n_links);
  virtual ~DecodeAndWriteToCSV() = default;
  /// call write_event with our file handle
  virtual void write_event(const EventPacket& ep) final;
};

template <class EventPacket>
DecodeAndWriteToCSV<EventPacket> all_channels_to_csv(
    const std::string& file_name);

/**
 * Consume an event packet, decode it, and save to buffer.
 * The constructor takes an argument of the size of the buffer which should
 * be the number of events being collected in the run(s) where this buffer is
 * used.
 *
 * The buffer can be read out with get_buffer().
 * The buffer is cleared upon the start of every run.
 *
 * Avoid copying this potentially large buffer around by using constant
 * references.
 * ```cpp
 * // after daq_run has filled the DecodeAndBuffer object 'buffer'
 * const auto& events{buffer.get_buffer()};
 * ```
 */
template <typename EventPacket>
class DecodeAndBuffer : public DecodeAndWrite<EventPacket> {
 public:
  /// define number of events to buffer and number of links enabled
  DecodeAndBuffer(std::size_t nevents, int n_links);
  virtual ~DecodeAndBuffer() = default;
  /// get buffer
  const std::vector<EventPacket>& get_buffer() const;
  /// Set the buffer size
  void set_buffer_size(std::size_t nevents);
  /// Save to buffer
  virtual void write_event(const EventPacket& ep) override;
  /// Check that the buffer was read and flushed since last run
  virtual void start_run() override;

 private:
  /// Buffer for event packets
  std::vector<EventPacket> ep_buffer_;
};
