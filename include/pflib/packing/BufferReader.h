#pragma once

#include <stdexcept>
#include <vector>
#include <cstdint>
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
 * ```cpp
 * BufferReader r{data}; // data is some vector of bytes
 * r >> obj; // obj is some object with a Reader& read(Reader&) method
 * ```
 */
class BufferReader : public Reader {
 public:
  /**
   * Initialize a reader by wrapping a buffer to read.
   */
  BufferReader(const std::vector<uint8_t>& b);

  /// default destructor so handle to buffer is given up
  ~BufferReader() = default;

  /// go to the index off in bytes
  void seek(int off) override;
  /// return where in the word we are in bytes
  int tell() override;
  /// read the next count bytes into array w and move the index
  Reader& read(char* w, std::size_t count) override;

  /**
   * Return state of buffer.
   * false if buffer is done being read,
   * true otherwise.
   */
  bool good() const override;
  bool eof() const override;

 private:
  // current buffer we are reading
  const std::vector<uint8_t>& buffer_;
  // current index in buffer we are reading
  std::size_t i_word_;
};  // BufferReader

}  // namespace pflib::packing
