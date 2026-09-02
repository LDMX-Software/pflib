#include "FWHistoPool.h"

#include "pflib/utility/string_format.h"
#include "pflib/version/Version.h"
using pflib::utility::string_format;

#include "pflib/Exception.h"

static constexpr uint32_t ADDR_HISTO_CLEAR = 0x100 / 4;
static constexpr uint32_t MASK_HISTO_CLEAR = 0x4;

uint32_t FWHistoPool::FW_VERSION = 0;

std::optional<double> FWHistoPool::get_collection_time(
    FWHistoPool::a_time_point now) const {
  using namespace std::literals;
  if (time_of_last_clear_) {
    return (now - time_of_last_clear_.value()) / 1.0s;
  } else {
    pflib_log(warn) << "There hasn't be a CLEAR recently,"
                       " so we don't know the collection time and"
                       " the histograms are probably saturated!";
    return {};
  }
}

std::array<uint32_t, 256> FWHistoPool::read_fw(FWHistoPool::FillValue fill_type,
                                               int index) {
  uint32_t quad_addr, quad_index{static_cast<uint32_t>(index % 4)};
  if (fill_type == FWHistoPool::FillValue::DecodedSum) {
    quad_addr = (0xC00 + 4 * 0x10 + 4 * (index / 4)) / 4;
  } else if (fill_type == FWHistoPool::FillValue::EncodedSum) {
    quad_addr = (0xC00 + 4 * 0x12 + 4 * (index / 4)) / 4;
  } else if (fill_type == FWHistoPool::FillValue::HighPeak) {
    quad_addr = (0xC00 + 4 * 0x14 + 4 * (index / 4)) / 4;
  } else if (fill_type == FWHistoPool::FillValue::Test) {
    // only one quad for test histogram
    quad_addr = 0xC7C / 4;
  }
  std::array<uint32_t, 256> data;
  for (int i{0}; i < data.size(); i++) {
    uint32_t val = i | (quad_index << 8);
    uio_.writeMasked(ADDR_REG, ADDR_MASK, val);
    data[i] = uio_.read(quad_addr);
  }
  return data;
}

FWHistoPool::FWHistoPool(int i_trigpath)
    : uio_{string_format("trigpath-%d", i_trigpath)},
      time_of_last_clear_{},
      the_log_{pflib::logging::get("pftool.trig.histo")} {
  FW_VERSION = (uio_.read(0) & 0xffff);
}

void FWHistoPool::clear() {
  uio_.write(ADDR_HISTO_CLEAR, MASK_HISTO_CLEAR);
  time_of_last_clear_ = the_clock::now();
}

void FWHistoPool::debug(int fill_val) {
  uio_.write(ADDR_HISTO_CLEAR, (fill_val & 0xFF) << 8);
  std::array<std::array<uint32_t, 256>, 4> hists;
  for (int i{0}; i < 4; i++) {
    hists[i] = read_fw(FWHistoPool::FillValue::Test, i);
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

FWHistoPool::SingleChannelHistogram::SingleChannelHistogram(
    FillValue fill_type, int index, std::array<uint32_t, 256> values,
    double collection_time)
    : fill_type_{fill_type},
      index_{index},
      values_{values},
      collection_time_{collection_time} {}

FWHistoPool::FillValue FWHistoPool::SingleChannelHistogram::fill_type() const {
  return fill_type_;
}

int FWHistoPool::SingleChannelHistogram::index() const { return index_; }

const std::array<uint32_t, 256> FWHistoPool::SingleChannelHistogram::values()
    const {
  return values_;
}

double FWHistoPool::SingleChannelHistogram::collection_time() const {
  return collection_time_;
}

static const std::unordered_map<FWHistoPool::FillValue, std::string> LABELS = {
    {FWHistoPool::FillValue::EncodedSum, "Encoded Sum"},
    {FWHistoPool::FillValue::DecodedSum, "Decoded Sum"},
    {FWHistoPool::FillValue::HighPeak, "High Peak"},
    {FWHistoPool::FillValue::Test, "Test Fill Code"}};

static const std::unordered_map<FWHistoPool::FillValue, std::string> NAMES = {
    {FWHistoPool::FillValue::EncodedSum, "encoded_sum"},
    {FWHistoPool::FillValue::DecodedSum, "decoded_sum"},
    {FWHistoPool::FillValue::HighPeak, "high_peak"},
    {FWHistoPool::FillValue::Test, "test_fill_code"}};

nlohmann::json FWHistoPool::SingleChannelHistogram::to_json() const {
  nlohmann::json hist;
  hist["uhi_schema"] = 1;
  hist["writer_info"]["pftool.FWHistoPool"]["version"] =
      pflib::version::debug();
  hist["writer_info"]["trigpath-firmware"]["version"] = FW_VERSION;
  hist["metadata"]["_variance_known"] = true;
  hist["metadata"]["collection_time"] = collection_time_;

  nlohmann::json axis;
  axis["type"] = "regular";
  axis["lower"] = 0.0;
  axis["upper"] = values_.size();
  axis["bins"] = values_.size();
  axis["underflow"] = false;
  axis["overflow"] = false;
  axis["circular"] = false;
  std::string name;
  if (fill_type_ == FWHistoPool::FillValue::Test) {
    name = string_format("TEST%d", index_);
  } else {
    name = string_format("STC%d", index_);
  }
  axis["metadata"]["name"] = name;
  axis["metadata"]["label"] = name + LABELS.at(fill_type_);

  hist["axes"] = {axis};
  hist["storage"]["type"] = "double";
  hist["storage"]["values"] = values_;
  return hist;
}

FWHistoPool::SingleChannelHistogram FWHistoPool::read(
    FWHistoPool::FillValue fill_type, int index) {
  if (index < 0 or index > 7) {
    PFEXCEPTION_RAISE("OutOfRange",
                      "Provided histogram index " + std::to_string(index) +
                          " but the FWHistoPool only supports indices [0,7]");
  }

  return SingleChannelHistogram(
      fill_type, index, read_fw(fill_type, index),
      get_collection_time(the_clock::now()).value_or(0));
}

FWHistoPool::BlockHistogram::BlockHistogram(
    FillValue fill_type, std::array<std::array<uint32_t, 256>, 8> values,
    double collection_time)
    : fill_type_{fill_type},
      values_{values},
      collection_time_{collection_time} {}

FWHistoPool::FillValue FWHistoPool::BlockHistogram::fill_type() const {
  return fill_type_;
}

const std::array<std::array<uint32_t, 256>, 8>
FWHistoPool::BlockHistogram::values() const {
  return values_;
}

double FWHistoPool::BlockHistogram::collection_time() const {
  return collection_time_;
}

nlohmann::json FWHistoPool::BlockHistogram::to_json() const {
  nlohmann::json hist;
  hist["uhi_schema"] = 1;
  hist["writer_info"]["pftool.FWHistoPool"]["version"] =
      pflib::version::debug();
  hist["writer_info"]["trigpath-firmware"]["version"] = FW_VERSION;
  hist["metadata"]["_variance_known"] = true;
  hist["metadata"]["collection_time"] = collection_time_;

  nlohmann::json cat;
  cat["type"] = "category_str";
  cat["categories"] = {"STC0", "STC1", "STC2", "STC3",
                       "STC4", "STC5", "STC6", "STC7"};
  cat["flow"] = false;
  cat["metadata"]["name"] = "stc";
  cat["metadata"]["label"] = "STC";

  nlohmann::json reg;
  reg["type"] = "regular";
  reg["lower"] = 0;
  reg["upper"] = 256;
  reg["bins"] = 256;
  reg["underflow"] = false;
  reg["overflow"] = false;
  reg["circular"] = false;
  reg["metadata"]["name"] = NAMES.at(fill_type_);
  reg["metadata"]["label"] = LABELS.at(fill_type_);

  hist["axes"] = {cat, reg};
  hist["storage"]["type"] = "double";
  hist["storage"]["values"] = values_;
  return hist;
}

FWHistoPool::BlockHistogram FWHistoPool::read(FillValue fill_type) {
  auto colltime = get_collection_time(the_clock::now());
  std::array<std::array<uint32_t, 256>, 8> content;
  for (int ihist{0}; ihist < 8; ihist++) {
    content[ihist] = read_fw(fill_type, ihist);
  }
  return BlockHistogram(fill_type, content, colltime.value_or(0));
}
