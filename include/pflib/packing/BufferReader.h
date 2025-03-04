#pragma once

#include <stdexcept>
#include <vector>

#include "pflib/packing/Reader.h"

namespace pflib::packing {

/**
 * @class BufferReader
 * This class is a helper class for reading the buffer
 * stored in the raw data format.
 *
 * The raw data format specifies that the buffer for all
 * data is a specific type (vector of bytes),
 * but the users in different subsystems/translators may
 * want the buffer to be read in different size words
 * than single bytes. This class helps in that translation.
 *
 * @tparam[in] WordType type of word user wants to read out from buffer
 */
class BufferReader : public Reader {
 public:
  /**
   * Initialize a reader by wrapping a buffer to read.
   */
  BufferReader(const std::vector<uint8_t>& b);

  ~BufferReader() = default;

  void seek(int off) override;
  int tell() override;
  Reader& read(char* w, std::size_t count) override;
  /**
   * Return state of buffer.
   * false if buffer is done being read,
   * true otherwise.
   */
  bool good() const override;

 private:
  // current buffer we are reading
  const std::vector<uint8_t>& buffer_;
  // current index in buffer we are reading
  std::size_t i_word_;
};  // BufferReader

}  // namespace pflib::packing
