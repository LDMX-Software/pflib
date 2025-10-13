#include "pflib/Hcal.h"

#include "pflib/utility/string_format.h"

namespace pflib {

Hcal::Hcal() {
  nhgcroc_ = 0;
  necon_ = 0;
}

bool Hcal::have_roc(int iroc) const {
  return roc_connections_.find(iroc) != roc_connections_.end();
}

bool Hcal::have_econ(int iecon) const {
  return econ_connections_.find(iecon) != econ_connections_.end();
}

std::vector<int> Hcal::roc_ids() const {
  std::vector<int> ids;
  ids.reserve(roc_connections_.size());
  for (const auto& [id, _conn] : roc_connections_) {
    ids.push_back(id);
  }
  return ids;
}

std::vector<int> Hcal::econ_ids() const {
  std::vector<int> ids;
  ids.reserve(econ_connections_.size());
  for (const auto& [id, _conn] : econ_connections_) {
    ids.push_back(id);
  }
  return ids;
}

void Hcal::add_roc(int iroc, uint8_t roc_baseaddr, const std::string& roc_type_version,
                   std::shared_ptr<I2C> roc_i2c,
                   std::shared_ptr<I2C> bias_i2c,
                   std::shared_ptr<I2C> board_i2c) {
  if (have_roc(iroc)) {
    PFEXCEPTION_RAISE("DuplicateROC",
                      pflib::utility::string_format(
                          "Already have registered ROC with id %d", iroc));
  }
  nhgcroc_++;
  pflib::ROC roc{*roc_i2c, roc_baseaddr, roc_type_version};
  pflib::Bias bias{bias_i2c};
  roc_connections_.emplace(
    iroc,
    ROCConnection {
      .roc_ = roc,
      .roc_i2c_ = roc_i2c,
      .bias_ = bias,
      .bias_i2c_ = bias_i2c,
      .board_i2c_ = board_i2c
    }
  );
}

void Hcal::add_econ(int iecon, uint8_t econ_baseaddr, const std::string& type_version,
                    std::shared_ptr<I2C> i2c) {
  if (have_econ(iecon)) {
    PFEXCEPTION_RAISE("DuplicateECON",
                      pflib::utility::string_format(
                          "Already have registered ECON with id %d", iecon));
  }
  necon_++;
  pflib::ECON econ{*i2c, econ_baseaddr, type_version};
  econ_connections_.emplace(
      iecon,
      ECONConnection {
        .econ_ = econ,
        .i2c_ = i2c
      }
  );
}

ROC Hcal::roc(int which) {
  auto roc_it{roc_connections_.find(which)};
  if (roc_it == roc_connections_.end()) {
    PFEXCEPTION_RAISE("InvalidROCid", pflib::utility::string_format(
                                          "Unknown ROC id %d", which));
  }
  return roc_it->second.roc_;
}

ECON Hcal::econ(int which) {
  auto econ_it{econ_connections_.find(which)};
  if (econ_it == econ_connections_.end()) {
    PFEXCEPTION_RAISE("InvalidECONid", pflib::utility::string_format(
                                          "Unknown ECON id %d", which));
  }
  return econ_it->second.econ_;
}

Bias Hcal::bias(int which) {
  auto roc_it{roc_connections_.find(which)};
  if (roc_it == roc_connections_.end()) {
    PFEXCEPTION_RAISE("InvalidBoardId", pflib::utility::string_format(
                                            "Unknown board id %d", which));
  }

  return roc_it->second.bias_;
}

void Hcal::hardResetROCs() {}

void Hcal::hardResetECONs() {}

void Hcal::softResetROC(int which) {}

void Hcal::softResetECON(int which) {}

uint32_t Hcal::getFirmwareVersion() { return 0; }

}  // namespace pflib
