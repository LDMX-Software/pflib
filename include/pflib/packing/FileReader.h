#pragma once

#include <fstream>
#include <iostream>  //debuggin
#include <string>
#include <type_traits>

#include "pflib/packing/Reader.h"

namespace pflib::packing {

/**
 * @class FileReader
 * Reading a raw data file.
 *
 * We wrap a basic std::ifstream in order to make the retrieving
 * of specific-width words easier for ourselves.
 * The abstract base Reader implements the templated complexities.
 */
class FileReader : public Reader {
 public:
  /**
   * default constructor
   *
   * We make sure that our input file stream will not skip white space.
   */
  FileReader();

  /**
   * Open a file with this reader
   *
   * We open the file as an input, binary file.
   *
   * @param[in] file_name full path to the file we are going to open
   */
  void open(const std::string& file_name);

  /**
   * Constructor that also opens the input file
   * @see open
   * @param[in] file_name full path to the file we are going to open
   */
  FileReader(const std::string& file_name);

  /// destructor, close the input file stream
  ~FileReader() = default;

  /**
   * Go ("seek") a specific position in the file.
   *
   * This non-template version of seek uses the default
   * meaning of the "off" parameter in which it counts bytes.
   *
   * @param[in] off number of bytes to move relative to dir
   * @param[in] dir location flag for the file, default is beginning
   */
  void seek(int off) override;

  /**
   * Tell us where the reader is
   *
   * @return int number of bytes relative to beginning of file
   */
  int tell() override;

  /**
   * Read the next `count` bytes into pointer w
   * 
   * @param[in] w pointer to array to write data into
   * @param[in] count number of bytes to read
   * @return (*this)
   */
  Reader& read(char* w, std::size_t count) override;

  /**
   * Check if reader is in a fail state
   *
   * Following the C++ reference, we pass-along the check
   * on if our ifstream is in a fail state.
   *
   * @return bool true if ifstream is in fail state
   */
  bool good() const override;

  /**
   * check if file is done
   *
   * Just calls the underlying ifstream eof.
   *
   * @return true if we have reached the end of file.
   */
  bool eof() override;

 private:
  /// file stream we are reading from
  std::ifstream file_;
  /// file size in bytes
  std::size_t file_size_;
};  // Reader

}  // namespace pflib::packing
