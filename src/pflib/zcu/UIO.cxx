#include "pflib/zcu/UIO.h"

#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include <map>

#include "pflib/Exception.h"

namespace pflib {

struct Mapping {
  uint32_t* ptr{0};
  int users{0};
  int handle{0};
};

/// SHOULD REALLY BE PROTECTED BY MUTEX
std::map<std::string, Mapping> gl_mappings;

UIO::UIO(const std::string& name, size_t length)
    : name_{name}, size_{length}, ptr_{0}, handle_{0} {
  auto gptr = gl_mappings.find(name);
  if (gptr != gl_mappings.end()) {
    gptr->second.users++;
    ptr_ = gptr->second.ptr;
    handle_ = gptr->second.handle;
    return;
  }

  /** first, look for the DTSI map */
  FILE* fdtsi =
      fopen("/opt/ldmx-firmware/active/device-tree/pl-full.dtsi", "r");
  uint32_t baseaddr = 0;
  if (fdtsi != 0) {
    char buf[1024], *where;
    uint32_t abaseaddr = 0;
    while (!feof(fdtsi)) {
      buf[0] = 0;
      fgets(buf, 1024, fdtsi);

      if ((where = strstr(buf, "@")) != 0) {
        abaseaddr = strtoul(where + 1, 0, 16);
      } else if ((where = strstr(buf, "instance_id = ")) != 0) {
        std::string iname = strstr(where, "\"") + 1;
        iname.erase(iname.find('"'));
        if (iname == name) {
          baseaddr = abaseaddr;
          break;
        }
        //	printf("%08x %s\n",baseaddr,iname.c_str());
      }
    }
    fclose(fdtsi);
  }
  for (int i = 0; i < 100; i++) {
    char namefile[200], buffer[100];
    if (baseaddr != 0) {
      snprintf(namefile, 200, "/sys/class/uio/uio%d/maps/map0/addr", i);
    } else {
      snprintf(namefile, 200, "/sys/class/uio/uio%d/maps/map0/name", i);
    }
    FILE* f = fopen(namefile, "r");
    if (!f) continue;
    fgets(buffer, 100, f);
    fclose(f);
    if (baseaddr != 0) {
      if (strtoul(buffer, 0, 0) == baseaddr) {
        snprintf(namefile, 200, "/dev/uio%d", i);
        iopen(namefile, length);
        break;
      }
    } else {
      // does it start with the same string?
      if (strstr(buffer, name.c_str()) == buffer) {
        snprintf(namefile, 200, "/dev/uio%d", i);
        iopen(namefile, length);
        break;
      }
    }
  }
  if (ptr_ == 0) {
    char msg[200];
    snprintf(msg, 200, "Found no UIO area with name %s", name.c_str());
    handle_ = 0;
    PFEXCEPTION_RAISE("DeviceFileNotFoundError", msg);
  }
  Mapping m;
  m.ptr = ptr_;
  m.users = 1;
  m.handle = handle_;
  gl_mappings[name] = m;
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
  auto gptr = gl_mappings.find(name_);
  if (gptr != gl_mappings.end()) {
    gptr->second.users--;
    if (gptr->second.users > 0) return;
  }

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
