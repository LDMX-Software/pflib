#include "daq_run.h"

#include <sys/time.h>

#include "pflib/Exception.h"
#include "pflib/packing/BufferReader.h"
#include "pftool.h"

ENABLE_LOGGING();

void daq_run(Target* tgt, const std::string& cmd, DAQRunConsumer& consumer,
             int nevents, int rate) {
  static const std::unordered_map<std::string,
                                  std::function<void(pflib::FastControl&)>>
      cmds = {{"PEDESTAL", [](pflib::FastControl& fc) { fc.sendL1A(); }},
              {"CHARGE", [](pflib::FastControl& fc) { fc.chargepulse(); }},
              {"LED", [](pflib::FastControl& fc) { fc.ledpulse(); }}};
  if (cmds.find(cmd) == cmds.end()) {
    PFEXCEPTION_RAISE("UnknownCMD",
                      "Command " + cmd + " is not one of the daq_run options.");
  }
  auto trigger{cmds.at(cmd)};

  consumer.start_run();

  timeval tv0, tvi;
  gettimeofday(&tv0, 0);
  for (int ievt = 0; ievt < nevents; ievt++) {
    pflib_log(trace) << "daq event occupancy pre-L1A    : "
                     << tgt->daq().getEventOccupancy();
    trigger(tgt->fc());

    pflib_log(trace) << "daq event occupancy post-L1A   : "
                     << tgt->daq().getEventOccupancy();
    gettimeofday(&tvi, 0);
    double runsec =
        (tvi.tv_sec - tv0.tv_sec) + (tvi.tv_usec - tv0.tv_usec) / 1e6;
    double targettime = (ievt + 1.0) / rate;
    int usec_ahead = int((targettime - runsec) * 1e6);
    pflib_log(trace) << " at " << runsec << "s instead of " << targettime
                     << "s aheady by " << usec_ahead << "us";
    if (usec_ahead > 100) {
      usleep(usec_ahead);
    }

    pflib_log(trace) << "daq event occupancy after pause: "
                     << tgt->daq().getEventOccupancy();

    std::vector<uint32_t> event = tgt->read_event();
    pflib_log(trace) << "daq event occupancy after read : "
                     << tgt->daq().getEventOccupancy();
    pflib_log(debug) << "event " << ievt << " has " << event.size()
                     << " 32-bit words";
    if (event.size() == 0) {
      pflib_log(warn) << "event " << ievt
                      << " did not have any words, skipping.";
    } else {
      consumer.consume(event);
    }
  }

  consumer.end_run();
}

// Adding constructor definition due to non default constructor containing
// n_links which default=0,
template <class EventPacket>
DecodeAndWrite<EventPacket>::DecodeAndWrite(int n_links) {
  if constexpr (std::is_same_v<EventPacket,
                               pflib::packing::MultiSampleECONDEventPacket>) {
    ep_ = std::make_unique<EventPacket>(n_links);
  } else {
    ep_ = std::make_unique<EventPacket>();
  }
}

WriteToBinaryFile::WriteToBinaryFile(const std::string& file_name)
    : file_name_{file_name}, fp_{fopen(file_name.c_str(), "a")} {
  if (not fp_) {
    PFEXCEPTION_RAISE("FileOpen", "Unable to open " + file_name_);
  }
}
WriteToBinaryFile::~WriteToBinaryFile() {
  if (fp_) fclose(fp_);
  fp_ = 0;
}

void WriteToBinaryFile::consume(std::vector<uint32_t>& event) {
  fwrite(&(event[0]), sizeof(uint32_t), event.size(), fp_);
}

template <class EventPacket>
void DecodeAndWrite<EventPacket>::consume(std::vector<uint32_t>& event) {
  // we have to manually check the size so that we can do the reinterpret_cast
  if (event.size() == 0) {
    pflib_log(warn) << "event with zero words passed in, skipping";
    return;
  }
  // reinterpret the 32-bit words into a vector of bytes which is
  // what is consummed by the BufferReader
  const auto& buffer{*reinterpret_cast<const std::vector<uint8_t>*>(&event)};
  pflib::packing::BufferReader r{buffer};
  r >> ep_;
  write_event(ep_);
}

template <class EventPacket>
DecodeAndWriteToCSV<EventPacket>::DecodeAndWriteToCSV(
    const std::string& file_name,
    std::function<void(std::ofstream&)> write_header,
    std::function<void(std::ofstream& f, const EventPacket&)> write_event)
    : DecodeAndWrite<EventPacket>(),
      file_{file_name},
      write_event_{write_event} {
  if (not file_) {
    PFEXCEPTION_RAISE("FileOpen",
                      "unable to open " + file_name + " for writing");
  }
  write_header(file_);
}

template <class EventPacket>
void DecodeAndWriteToCSV<EventPacket>::write_event(const EventPacket& ep) {
  write_event_(file_, ep);
}

template <class EventPacket>
DecodeAndWriteToCSV<EventPacket> all_channels_to_csv(
    const std::string& file_name) {
  return DecodeAndWriteToCSV<EventPacket>(
      file_name,
      [](std::ofstream& f) {
        f << std::boolalpha;
        f << EventPacket::to_csv_header << '\n';
      },
      [](std::ofstream& f, const EventPacket& ep) { ep.to_csv(f); });
}

template <class EventPacket>
DecodeAndBuffer<EventPacket>::DecodeAndBuffer(int nevents)
    : DecodeAndWrite<EventPacket>() {
  set_buffer_size(nevents);
}

template <class EventPacket>
void DecodeAndBuffer<EventPacket>::write_event(const EventPacket& ep) {
  if (ep_buffer_.size() > ep_buffer_.capacity()) {
    pflib_log(warn) << "Trying to push more elements to buffer than allocated "
                       "capacity. Skipping!";
    return;
  }
  ep_buffer_.push_back(ep);
}

template <class EventPacket>
void DecodeAndBuffer<EventPacket>::start_run() {
  ep_buffer_.clear();
}

template <class EventPacket>
const std::vector<EventPacket>& DecodeAndBuffer<EventPacket>::get_buffer()
    const {
  return ep_buffer_;
}

template <class EventPacket>
void DecodeAndBuffer<EventPacket>::set_buffer_size(int nevents) {
  ep_buffer_.reserve(nevents);
}

// -----------------------------------------------------------------------------
// Explicit template instantiations
// -----------------------------------------------------------------------------

// DecodeAndWrite
template class DecodeAndWrite<pflib::packing::SingleROCEventPacket>;
template class DecodeAndWrite<pflib::packing::MultiSampleECONDEventPacket>;

// DecodeAndWriteToCSV
template class DecodeAndWriteToCSV<pflib::packing::SingleROCEventPacket>;
template class DecodeAndWriteToCSV<pflib::packing::MultiSampleECONDEventPacket>;

// DecodeAndBuffer
template class DecodeAndBuffer<pflib::packing::SingleROCEventPacket>;
template class DecodeAndBuffer<pflib::packing::MultiSampleECONDEventPacket>;

// all_channels_to_csv free-function template
template DecodeAndWriteToCSV<pflib::packing::SingleROCEventPacket>
all_channels_to_csv<pflib::packing::SingleROCEventPacket>(const std::string&);

template DecodeAndWriteToCSV<pflib::packing::MultiSampleECONDEventPacket>
all_channels_to_csv<pflib::packing::MultiSampleECONDEventPacket>(
    const std::string&);