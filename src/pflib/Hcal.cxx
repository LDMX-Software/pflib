#include "pflib/Hcal.h"

#include "pflib/utility/string_format.h"

namespace pflib {

Hcal::Hcal() { nhgcroc_ = 0; }

bool Hcal::have_roc(int iroc) {
  return connections_.find(iroc) != connections_.end();
}

std::vector<int> Hcal::roc_ids() const {
  std::vector<int> ids;
  ids.reserve(connections_.size());
  for (const auto& [id, _conn] : connections_) {
    ids.push_back(id);
  }
  return ids;
}

void Hcal::add_roc(int iroc, uint8_t roc_baseaddr, const std::string& roc_type_version,
                   std::shared_ptr<I2C>& roc_i2c,
                   std::shared_ptr<I2C>& bias_i2c,
                   std::shared_ptr<I2C>& board_i2c) {
  if (have_roc(iroc)) {
    PFEXCEPTION_RAISE("DuplicateROC",
                      pflib::utility::string_format(
                          "Already have registered ROC with id %d", iroc));
  }
  nhgcroc_++;
  pflib::ROC roc{*roc_i2c, roc_baseaddr, roc_type_version};
  pflib::Bias bias{bias_i2c};
  connections_.emplace(
    iroc,
    Connection {
      .roc_ = roc,
      .roc_i2c_ = roc_i2c,
      .bias_ = bias,
      .bias_i2c_ = bias_i2c,
      .board_i2c_ = board_i2c
    }
  );
}

ROC Hcal::roc(int which) {
  auto roc_it{connections_.find(which)};
  if (roc_it == connections_.end()) {
    PFEXCEPTION_RAISE("InvalidROCid", pflib::utility::string_format(
                                          "Unknown ROC id %d", which));
  }
  return roc_it->second.roc_;
}

Bias Hcal::bias(int which) {
  auto roc_it{connections_.find(which)};
  if (roc_it == connections_.end()) {
    PFEXCEPTION_RAISE("InvalidBoardId", pflib::utility::string_format(
                                            "Unknown board id %d", which));
  }

  return roc_it->second.bias_;
}

void Hcal::hardResetROCs() {}

void Hcal::softResetROC(int which) {}

uint32_t Hcal::getFirmwareVersion() { return 0; }

}  // namespace pflib
