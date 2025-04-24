#pragma once

#include <type_traits>
#include <vector>

namespace pflib::packing {

/**
 * @class Reader
 * Reading a raw data stream with some underlying backend.
 *
 * This abstract base class assumes a derived class defines
 * how to extract the next byte of data from some input data
 * stream (e.g. either a in-memory buffer or a binary file).
 * Then this interface has several templated methods that deduce
 * how many bytes to extract and calls `read` methods on objects
 * that are not integral types.
 */
class Reader {
 public:
  /**
   * default constructor
   *
   * We make sure that our input file stream will not skip white space.
   */
  Reader() = default;

  /**
   * virtual destructor for inheritance
   */
  virtual ~Reader() = default;

  /**
   * Go ("seek") a specific position in the stream.
   *
   * This non-template version of seek uses the default
   * meaning of the "off" parameter in which it counts *bytes*.
   *
   * @param[in] off number of bytes to move relative to dir
   * @param[in] dir location flag for the file, default is beginning
   */
  virtual void seek(int off) = 0;

  /**
   * Seek by number of words
   *
   * This template version of seek uses the input word type
   * to move around by the count of the input words rather than
   * the count of bytes.
   *
   * @tparam[in] WordType Integral-type to count by
   * @param[in] off number of words to move relative to dir
   * @param[in] dir location flag for the file, default is beginning
   */
  template <typename WordType,
            std::enable_if_t<std::is_integral<WordType>::value, bool> = true>
  void seek(int off) {
    seek(off * sizeof(WordType));
  }

  /**
   * Tell us where the reader is
   *
   * @return int number of bytes relative to beginning of file
   */
  virtual int tell() = 0;

  /**
   * Tell by number of words
   *
   * @return int number of words relative to beginning of file
   */
  template <typename WordType>
  int tell() {
    return tell() / sizeof(WordType);
  }

  /**
   * Read the next 'count' bytes into the input handle w
   *
   * @param[in] w pointer to array to write data into
   * @param[in] count number of bytes to read
   * @return (*this)
   */
  virtual Reader& read(char* w, std::size_t count) = 0;

  /**
   * Read the next 'count' words into the input handle.
   *
   * This implementation of read is only available to pointers to integral
   * types. We assume that whatever space pointed to by w already has the space
   * reserved necessary for the input words.
   *
   * @tparam[in] WordType integral-type word to read out
   * @param[in] w pointer to WordType array to write data to
   * @param[in] count number of words to read
   * @return (*this)
   */
  template <typename WordType,
            std::enable_if_t<std::is_integral<WordType>::value, bool> = true>
  Reader& read(WordType* w, std::size_t count) {
    return read(reinterpret_cast<char*>(w), sizeof(WordType) * count);
  }

  /**
   * Stream the next word into the input handle
   *
   * This implementation of the stream operator is only available to handles of
   * integral types. Helps for shorthand of only grabbing a single word from the
   * reader.
   *
   * @see read
   *
   * @tparam[in] WordType integral-type word to read out
   * @param[in] w reference to word to read into
   * @return handle to modified reader
   */
  template <typename WordType,
            std::enable_if_t<std::is_integral<WordType>::value, bool> = true>
  Reader& operator>>(WordType& w) {
    return read(&w, 1);
  }

  /**
   * Stream into a class object
   *
   * We assume that the class we are streaming to has a specific method defined.
   *
   *  Reader& read(Reader&)
   *
   * This can be satisfied by the classes that we own and operate.
   *
   * @tparam[in] ObjectType class type to read
   * @param[in] o instance of object to read into
   * @return *this
   */
  template <typename ObjectType,
            std::enable_if_t<std::is_class<ObjectType>::value, bool> = true>
  Reader& operator>>(ObjectType& o) {
    return o.read(*this);
  }

  /**
   * Read the next 'count' objects into the input vector.
   *
   * This is common enough, I wanted to specialize the read function.
   *
   * The std::vector::resize is helpful for avoiding additional
   * copies while the vector is being expanded. After allocating the space
   * for each entry in the vector, we call the stream operator from
   * *this into each entry in order, leaving early if a failure occurs.
   *
   * @tparam[in] ContentType type of object inside the vector
   * @param[in] vec object vector to read into
   * @param[in] count number of objects to read
   * @return *this
   */
  template <typename ContentType>
  Reader& read(std::vector<ContentType>& vec, std::size_t count,
               std::size_t offset = 0) {
    vec.resize(count + offset);
    for (std::size_t i{offset}; i < vec.size(); i++) {
      if (!(*this >> vec[i])) return *this;
    }
    return *this;
  }

  /**
   * Check if reader is in a good state (e.g. successfully open)
   *
   * @return bool true if reader is good and can keep reading
   */
  virtual bool good() const = 0;

  /**
   * Check if reader is in a fail or done state
   *
   * @return bool true if reader is in fail state
   */
  virtual bool operator!() const { return !good() or eof(); }

  /**
   * Check if reader is in good state and is not done reading yet
   *
   * Defining this operator allows us to do the following.
   *
   * ```cpp
   * // r is some concreate instance of a Reader
   * if (r) {
   *   std::cout << "r can read more data" << std::endl;
   * }
   * ```
   *
   * @return bool true if reader is in good state
   */
  virtual operator bool() const { return good() and not eof(); }

  /**
   * check if file is done
   *
   * @return true if we have reached the end of file.
   */
  virtual bool eof() const = 0;
};  // Reader

}  // namespace pflib::packing
