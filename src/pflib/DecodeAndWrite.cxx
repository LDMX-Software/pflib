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

DecodeAndBuffer::DecodeAndBuffer() : DecodeAndWrite() {};

void DecodeAndBuffer::write_event(const pflib::packing::SingleROCEventPacket& ep) {
  ep_buffer_.push_back(ep);
}

std::vector<pflib::packing::SingleROCEventPacket> DecodeAndBuffer::read_and_flush() {
  std::vector<pflib::packing::SingleROCEventPacket> data = ep_buffer_;
  ep_buffer_.clear();
  return data;
}

void DecodeAndBuffer::start_run() {
  if (ep_buffer_.size() > 0) {
    //pflib_log(warn) << "buffer was not read and flushed since last run"
  }
}

void DecodeAndBuffer::end_run() {
  if (ep_buffer_.size() > 10000) ep_buffer_.clear();
  //pflib_log(warn) << "too many events saved in buffer. buffer cleared";
}

}
