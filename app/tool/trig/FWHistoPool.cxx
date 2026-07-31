#include "FWHistoPool.h"

#include "pflib/version/Version.h"
#include "pflib/utility/string_format.h"
using pflib::utility::string_format;

#include "pflib/Exception.h"

static constexpr uint32_t ADDR_HISTO_CLEAR = 0x100 / 4;
static constexpr uint32_t MASK_HISTO_CLEAR = 0x4;
static constexpr uint32_t ADDR_TRIG_HISTO_03 = 0xC40 / 4;
static constexpr uint32_t ADDR_TRIG_HISTO_47 = 0xC44 / 4;
static constexpr uint32_t ADDR_TRIG_HISTO_TEST = 0xC7C / 4;

FWHistoPool::FWHistoPool(int i_trigpath)
  : uio_{string_format("trigpath-%d", i_trigpath)}
  {}

void FWHistoPool::clear() {
  uio_.write(ADDR_HISTO_CLEAR, MASK_HISTO_CLEAR);
}

void FWHistoPool::debug(int fill_val) {
  uio_.write(ADDR_HISTO_CLEAR, (fill_val & 0xFF) << 8);
  std::array<std::array<uint32_t, 256>, 4> hists;
  for (int i{0}; i < 4; i++) {
    hists[i] = read(8 + i);
  }
  printf("bin: %10u %10u %10u %10u\n", 0, 1, 2, 3);
  for (int i{0}; i < 256; i++) {
    printf("%3d:", i);
    for (int j{0}; j < 4; j++) {
      printf(" %10u", hists[j][i]);
    }
    printf("\n");
  }
}

std::array<uint32_t, 256> FWHistoPool::read(int ihist) {
  if (ihist < 0 or ihist > 11) {
    PFEXCEPTION_RAISE("OutOfRange",
        "Provided histogram index " + std::to_string(ihist) +
        " but the FWHistoPool only supports indices [0,11]");
  }
  int block = ihist/4;
  uint32_t data_addr = 0;
  if (block == 0) {
    data_addr = ADDR_TRIG_HISTO_03;
  } else if (block == 1) {
    data_addr = ADDR_TRIG_HISTO_47;
  } else if (block == 2) {
    data_addr = ADDR_TRIG_HISTO_TEST;
  }
  std::array<uint32_t, 256> data;
  for (int i{0}; i < data.size(); i++) {
    uint32_t val = i | (ihist << 8);
    uio_.writeMasked(ADDR_REG, ADDR_MASK, val);
    data[i] = uio_.read(data_addr);
  }
  return data;
}

nlohmann::json FWHistoPool::to_json(const std::array<uint32_t, 256>& data, int ihist) {
  nlohmann::json hist;
  hist["uhi_schema"] = 1;
  hist["writer_info"]["pftool.FWHistoPool"]["version"] = pflib::version::debug();
  hist["metadata"]["_variance_known"] = true;

  nlohmann::json axis;
  axis["type"] = "regular";
  axis["lower"] = 0.0;
  axis["upper"] = 256.0;
  axis["bins"] = 256;
  axis["underflow"] = false;
  axis["overflow"] = false;
  axis["circular"] = false;
  axis["metadata"]["name"] = "";
  std::string label;
  if (ihist < 8) {
    label = string_format("STC%d", ihist);
  } else {
    label = string_format("TEST%d", ihist-8);
  }
  axis["metadata"]["label"] = label;

  hist["axes"] = { axis };
  hist["storage"]["type"] = "double";
  hist["storage"]["values"] = data;
  return hist;
}
