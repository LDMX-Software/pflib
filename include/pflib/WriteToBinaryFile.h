#pragma once

#include <stdio.h>

#include <string>

#include "pflib/Target.h"

namespace pflib {

/**
 * just copy input event packets to the output file as binary
 */
class WriteToBinaryFile : public Target::DAQRunConsumer {
  std::string file_name_;
  FILE* fp_;

 public:
  WriteToBinaryFile(const std::string& file_name);
  ~WriteToBinaryFile();
  virtual void consume(std::vector<uint32_t>& event) final;
};

}  // namespace pflib
