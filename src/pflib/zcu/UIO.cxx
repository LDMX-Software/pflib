#include "pflib/zcu/UIO.h"

#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>
#include "pflib/Exception.h"

namespace pflib {

UIO::UIO(const std::string& name, size_t length) : size_{length}, ptr_{0}, handle_{0} {
  for (int i=0; i<100; i++) {
    char namefile[200],buffer[100];
    snprintf(namefile,200,"/sys/class/uio/uio%d/maps/map0/name",i);
    FILE* f=fopen(namefile,"r");
    if (!f) continue;
    fgets(buffer,100,f);
    fclose(f);
    // does it start with the same string?
    if (strstr(buffer,name.c_str())==buffer) {
      snprintf(namefile,200,"/dev/uio%d",i);
      iopen(namefile,length);
    }
  }
  if (ptr_==0) {
    char msg[200];
    snprintf(msg, 200, "Found no UIO area with name %s", name.c_str());
    handle_ = 0;
    PFEXCEPTION_RAISE("DeviceFileNotFoundError", msg);
  }
}

void UIO::iopen(const std::string& path, size_t length) {
    
  handle_ = open(path.c_str(), O_RDWR);
  if (handle_ < 0) {
    char msg[200];
    snprintf(msg, 200, "Error opening %s : %d", path.c_str(), errno);
    handle_ = 0;
    PFEXCEPTION_RAISE("DeviceFileAccessError", msg);
  }
  ptr_ = (uint32_t*)(mmap(0, size_, PROT_READ | PROT_WRITE, MAP_SHARED, handle_,
                          0));
  if (ptr_ == MAP_FAILED) {
    PFEXCEPTION_RAISE("DeviceFileAccessError",
                      "Failed to mmap FastControl memory block");
  }
}

UIO::~UIO() {
  if (handle_ != 0) {
    munmap(ptr_, size_);
    close(handle_);
    handle_ = 0;
  }
}

void UIO::write(size_t where, uint32_t what) {
  if (where >= size_) {
    PFEXCEPTION_RAISE("WriteOutOfRange",
                      "Attemped to write outside valid UIO block");
  }
  usleep(1);  // required to avoid stacking up too many writes, which results in
              // problems...
  ptr_[where] = what;
}

void UIO::rmw(size_t where, uint32_t mask_to_mod, uint32_t orval) {
  if (where >= size_) {
    PFEXCEPTION_RAISE("WriteOutOfRange",
                      "Attemped to write outside valid UIO block");
  }
  uint32_t val = ptr_[where];
  uint32_t mask = mask_to_mod ^ 0xFFFFFFFFu;
  val = (val & mask) | orval;
  usleep(1);  // for safety...
  ptr_[where] = val;
}
}  // namespace pflib
