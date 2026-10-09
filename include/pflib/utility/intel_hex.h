#pragma once
#ifndef PFLIB_UTILITY_INTEL_HEX_H
#define PFLIB_UTILITY_INTEL_HEX_H

#include <cstdint>
#include <cstdio>
#include <vector>
#include <string>
#include <type_traits>
#include <fstream>

namespace pflib::utility {
 
/**
 * A reader/writer implemeting a subset of the Intel HEX format
 *
 * After a brief survey of the available options online, it appears
 * like the format is simple enough that there is no maintained C++
 * library for interacting with it. This implementation follows
 * the description given on [Wikipedia](https://en.wikipedia.org/wiki/Intel_HEX)
 * and only supports record types 0x00 (Data) and 0x01 (EoF).
 * This is okay for our purposes since both the ECONs and the ROCs
 * use 16 bit addressing for their registers.
 *
 * A few other specializations
 * - the lines are always separated by a newline
 * - empty lines are ignored
 * - lines beginning with '//' are comments and ignored
 * - lines beginning with ':' are records and parsed,
 * - all other lines are errors
 */
namespace intel_hex {

/**
 * a record of data from/to the IntelHex format
 *
 * The from/get template functions are where we decide on endian-ness
 * of the data values (we choose big-endian as defined by format).
 */
class DataRecord {
  /// actual data in this record
  std::vector<uint8_t> data_;
  /// address for the data
  uint16_t addr_;
 public:
  /// set contents of data record
  void from_bytes(uint16_t addr, std::vector<uint8_t> data);
  /// set contents of data record using width of integer type
  template<typename WordType,
           std::enable_if_t<std::is_integral<WordType>::value, bool> = true>
  void from(uint16_t addr, WordType value) {
    addr_ = addr;
    data_.resize(sizeof(WordType));
    for (int i_byte{0}; i_byte < sizeof(WordType); i_byte++) {
      data_[i_byte] = static_cast<uint8_t>((value >> 8*(sizeof(WordType)-i_byte-1)) & 0xff);
    }
  }
  /// get raw bytes of data
  const std::vector<uint8_t>& get_bytes() const;
  /// get data in terms of integer type
  template<typename WordType,
           std::enable_if_t<std::is_integral<WordType>::value, bool> = true>
  WordType get() {
    WordType word = 0;
    for (int i_byte{0}; i_byte < get_bytes().size(); i_byte++) {
      word |= (static_cast<WordType>(get_bytes().at(i_byte)) << 8*(sizeof(WordType)-i_byte-1));
    }
    return word;
  }
  /// get address
  const uint16_t& addr() const;
};

/**
 * reader for the IntelHex format
 *
 * we go record-by-record giving the user a choice
 * on if the data is being kept in a large array in
 * memory or if it is being sent incrementally to
 * be applied to the chip for configuration
 *
 * ```cpp
 * intel_hex::Reader ih_reader("file.hex");
 * while (ih_reader.next()) {
 *   auto data = ih_reader.get();
 *   // do something with data
 * }
 * ```
 */
class Reader {
  std::ifstream input_file_;
  DataRecord current_record_;
  std::size_t error_count_;
 public:
  Reader(const Reader&) = delete;
  Reader& operator=(const Reader&) = delete;
  ~Reader() = default;
  Reader(const std::string& input_filepath);
  std::size_t errors() const;
  bool next();
  const DataRecord& get();
};

/**
 * writer for the IntelHexFormat
 *
 * The user is expected to Writer::add records
 * one at a time.
 * ```cpp
 * intel_hex::Writer ih_writer("file.hex");
 * DataRecord record;
 * for (const auto& [addr, value] : registers) {
 *   // maybe value is a 4 bytes (32 bits)
 *   record.from(addr, value);
 * }
 * // file closed when ih_writer is destructed
 * // or you can call close() manually
 * ```
 */
class Writer {
  FILE* output_file_;
 public:
  Writer(const Writer&) = delete;
  Writer& operator=(const Writer&) = delete;
  Writer(const std::string& output_filepath);
  ~Writer();
  void add(const DataRecord& data);
  void close();
};

}

}

#endif
