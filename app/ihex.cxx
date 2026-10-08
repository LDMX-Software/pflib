#include "pflib/utility/intel_hex.h"

int main() {
  {
    pflib::utility::intel_hex::Writer w{"file.hex"};
    pflib::utility::intel_hex::DataRecord d;
    d.from<uint32_t>(0x0102, 0x12345678);
    w.add(d);
  }

  {
    pflib::utility::intel_hex::Reader r{"file.hex"};
    while (r.next()) {
      auto d = r.get();
      printf("%04x -> %08x\n", d.addr(), d.get<uint32_t>());
    }
  }
}
