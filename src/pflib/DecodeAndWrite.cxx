#include "pflib/DecodeAndWrite.h"

#include "pflib/packing/BufferReader.h"
#include "pflib/Exception.h"

namespace pflib {

void DecodeAndWrite::consume(std::vector<uint32_t>& event) {
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

DecodeAndWriteToCSV::DecodeAndWriteToCSV(
    const std::string& file_name,
    std::function<void(std::ofstream&)> write_header,
    std::function<void(std::ofstream& f, const pflib::packing::SingleROCEventPacket&)> write_event
) : DecodeAndWrite(), file_{file_name}, write_event_{write_event} {
  if (not file_) {
    PFEXCEPTION_RAISE("FileOpen", "unable to open "+file_name+" for writing");
  }
  write_header(file_);
}

void DecodeAndWriteToCSV::write_event(const pflib::packing::SingleROCEventPacket& ep) {
  write_event_(file_, ep);
}

DecodeAndWriteToCSV all_channels_to_csv(const std::string& file_name) {
  return DecodeAndWriteToCSV(
      file_name,
      [](std::ofstream& f) {
        f << std::boolalpha;
        f << packing::SingleROCEventPacket::to_csv_header << '\n';
      },
      [](std::ofstream& f, const pflib::packing::SingleROCEventPacket& ep) {
        ep.to_csv(f);
      }
  );
}

DecodeAndWriteRunSummary::DecodeAndWriteRunSummary(
    const std::string& file_name,
    int n_events,
    std::function<void(std::ofstream&)> write_header,
    std::function<void(std::ofstream&, const std::array<std::vector<pflib::packing::Sample>, 72>&)> write_row
) : DecodeAndWrite(), file_{file_name}, write_row_{write_row}, data_{}, i_event_{0} {
  if (not file_) {
    PFEXCEPTION_RAISE("FileOpen", "unable to open "+file_name+" for writing");
  }
  write_header(file_);

  // allocate table of samples
  for (auto& ch: data_) {
    ch.resize(n_events);
  }
}

void DecodeAndWriteRunSummary::start_run() {
  i_event_ = 0;
  for (auto& ch: data_) {
    for (auto& sample: ch) {
      sample.word = 0;
    }
  }
}

void DecodeAndWriteRunSummary::write_event(const pflib::packing::SingleROCEventPacket& ep) {
  for (int i_ch{0}; i_ch < data_.size(); i_ch++) {
    data_[i_ch][i_event_] = ep.channel(i_ch);
  }
  i_event_++;
}

void end_run() {
  write_row_(file_, data_);
}

}
