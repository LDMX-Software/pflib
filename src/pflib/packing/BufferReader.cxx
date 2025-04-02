#include "pflib/packing/BufferReader.h"

namespace pflib::packing {

BufferReader::BufferReader(const std::vector<uint8_t>& b)
    : Reader(), buffer_{b}, i_word_{0} {}

void BufferReader::seek(int off) { i_word_ = off; }

int BufferReader::tell() { return i_word_; }

Reader& BufferReader::read(char* w, std::size_t count) {
  for (std::size_t i_byte{0}; i_byte < count; i_byte++) {
    w[i_byte] = buffer_[i_word_];
    i_word_++;
  }
  return *this;
}

bool BufferReader::good() const { return (i_word_ < buffer_.size()); }

}  // namespace pflib::packing
