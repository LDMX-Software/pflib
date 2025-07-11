#include "pflib/DecodeAndBuffer.h"

#include "pflib/packing/BufferReader.h"
#include "pflib/Exception.h"

namespace pflib {

DecodeAndBuffer::DecodeAndBuffer(int nevents) {
  buffer_size_ = nevents;
  ep_buffer_.reserve(buffer_size_);
}

void DecodeAndBuffer::consume(std::vector<uint32_t>& event) {
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

void DecodeAndBuffer::write_event(const pflib::packing::SingleROCEventPacket& ep) {
  if (ep_buffer_.size() > buffer_size_) {
    pflib_log(warn) << "Trying to push more elements to buffer than allocated capacity. Skipping!";
    return;
  }
  ep_buffer_.push_back(ep);
}

std::vector<pflib::packing::SingleROCEventPacket> DecodeAndBuffer::read_buffer() {
  return ep_buffer_;
}

void DecodeAndBuffer::start_run() {
  ep_buffer_.clear();
}

}
