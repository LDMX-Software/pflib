#include "pflib/packing/FileReader.h"

namespace pflib::packing {

FileReader::FileReader() : Reader() { file_.unsetf(std::ios::skipws); }

void FileReader::open(const std::string& file_name) {
  file_.open(file_name, std::ios::in | std::ios::binary);
  file_.seekg(0, std::ios::end);
  file_size_ = file_.tellg();
  file_.seekg(0);
}

FileReader::FileReader(const std::string& file_name) : FileReader() {
  this->open(file_name);
}

void FileReader::seek(int off) { file_.seekg(off, std::ios::beg); }

int FileReader::tell() { return file_.tellg(); }

Reader& FileReader::read(char* w, std::size_t count) {
  file_.read(w, count);
  return *this;
}

bool FileReader::good() const { return !file_.fail(); }

bool FileReader::eof() { return file_.eof() or file_.tellg() == file_size_; }

}  // namespace pflib::packing
