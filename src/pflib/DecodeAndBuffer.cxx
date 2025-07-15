#include "pflib/DecodeAndBuffer.h"

#include "pflib/packing/BufferReader.h"
#include "pflib/Exception.h"

namespace pflib {

DecodeAndBuffer::DecodeAndBuffer(int nevents) : DecodeAndWrite() {
  set_buffer_size(nevents);
}

void DecodeAndBuffer::write_event(const pflib::packing::SingleROCEventPacket& ep) {
  if (ep_buffer_.size() > ep_buffer_.capacity()) {
    pflib_log(warn) << "Trying to push more elements to buffer than allocated capacity. Skipping!";
    return;
  }
  ep_buffer_.push_back(ep);
}

void DecodeAndBuffer::start_run() {
  ep_buffer_.clear();
}

const std::vector<pflib::packing::SingleROCEventPacket>& DecodeAndBuffer::get_buffer() const {
  return ep_buffer_;
}

void DecodeAndBuffer::set_buffer_size(int nevents) {
  ep_buffer_.reserve(nevents);
}

}
