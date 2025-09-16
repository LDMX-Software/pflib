#include "pflib/WriteToBinaryFile.h"

namespace pflib {

WriteToBinaryFile::WriteToBinaryFile(const std::string& file_name)
    : file_name_{file_name}, fp_{fopen(file_name.c_str(), "a")} {
  if (not fp_) {
    PFEXCEPTION_RAISE("FileOpen", "Unable to open " + file_name_);
  }
}
WriteToBinaryFile::~WriteToBinaryFile() {
  if (fp_) fclose(fp_);
  fp_ = 0;
}

void WriteToBinaryFile::consume(std::vector<uint32_t>& event) {
  fwrite(&(event[0]), sizeof(uint32_t), event.size(), fp_);
}

}  // namespace pflib
