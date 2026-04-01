#ifndef PFLIB_TARGET_H_INCLUDED
#define PFLIB_TARGET_H_INCLUDED

#include "pflib/DAQ.h"
#include "pflib/ECON.h"
#include "pflib/Elinks.h"
#include "pflib/FastControl.h"
#include "pflib/I2C.h"
#include "pflib/ROC.h"
#include "pflib/lpGBT.h"

namespace pflib {

class OptoLink;

/**
 * @class Target class for encapulating a given setup's access rules
 *
 * Since both the Hcal and Ecal have ECONs and HGCROCs (that is the whole
 * reason we share pflib and firmware), this pflib::Target is the unifying
 * point for them. It represents one "DMA access" point (i.e. an HcalBackplane
 * or a "group" of Ecal modules).
 */
class Target {
 public:
  virtual ~Target() = default;

  /** number of boards */
  virtual int nrocs() { return -1; }

  /// number of econds
  virtual int necons() { return -1; }

  /** do we have a roc with this id? */
  virtual bool have_roc(int iroc) const { return false; }

  /** do we have an econ with this id? */
  virtual bool have_econ(int iecon) const { return false; }

  /** get a list of the IDs we have set up */
  virtual std::vector<int> roc_ids() const { return {}; }

  /** get a list of the econ IDs we have set up */
  virtual std::vector<int> econ_ids() const { return {}; }

  /** Get a ROC interface for the given HGCROC board */
  virtual ROC& roc(int which) {
    PFEXCEPTION_RAISE(
        "NoImp", "ROC Access has not been implemented for the current target.");
  }

  /** get a ECON interface for the given econ board */
  virtual ECON& econ(int which) {
    PFEXCEPTION_RAISE(
        "NoImp",
        "ECON Access has not been implemented for the current target.");
  }

  /** Generate a hard reset to all the HGCROC boards */
  virtual void hardResetROCs() {}

  /** generate a hard reset to all the ECON boards */
  virtual void hardResetECONs() {}

  /** Get the firmware version */
  virtual uint32_t getFirmwareVersion() { return -1; }

  /** Generate a soft reset to a specific HGCROC board, -1 for all */
  virtual void softResetROC(int which = -1) {}

  /** Generate a soft reset to a specific ECON board, -1 for all */
  virtual void softResetECON(int which = -1) {}

  /** get the Elinks object */
  virtual Elinks& elinks() = 0;

  /** get the FastControl object */
  virtual FastControl& fc() = 0;

  /** get the DAQ object */
  virtual DAQ& daq() = 0;

  /// names of different I2C busses we could talk to
  std::vector<std::string> i2c_bus_names();

  /// get an I2C bus by name
  I2C& get_i2c_bus(const std::string& name);

  /// names of different Optical Links we could talk to
  std::vector<std::string> opto_link_names() const;

  /// get an OptoLink by name
  OptoLink& get_opto_link(const std::string& name) const;

  virtual const std::vector<std::pair<int, int>>& getRocErxMapping() = 0;

  /**
   * Convert the input ECON (link, channel) index to the ROC (index, channel)
   * using the RocErxMapping defined for the Target and the number of active ROCs.
   *
   * @note This logic only works for single-ECON-D setups where all the ROCs are
   * going into a single ECON-D.
   *
   * @param[in] i_link link index as output after decoding
   * @param[in] channel channel index within that link (0-36)
   * @return [i_roc, channel] where i_roc is the ROC index (retrieve with roc(i_roc))
   * and channel is the channel within that ROC (0-72)
   */
  std::pair<int, int> toROCChannel(int i_link, int channel);
  std::pair<int, int> toECONChannel(int i_roc, int channel);

  /**
   * types of daq formats that we can do
   */
  enum class DaqFormat {
    /**
     * simple format for direct HGCROC connection
     *
     * This format is only supported for fiberless connections
     * directly between a ZCU and an Hcal HGCROC board. After
     * initial testing of DAQ is complete in this setup, you
     * are encouraged to use ECOND_HEADERS so that code you
     * develop is more transparently shared to other (fiberfull)
     * test stands.
     */
    SIMPLEROC = 1,
    /**
     * ECON-D format with headers inserted by software (on the ZCU)
     * or firmware (on the Bittware) to mark the beginning of samples
     * and the end of a multi-sample sequence.
     */
    ECOND_SW_HEADERS = 2
  };

  virtual void setup_run(int irun, DaqFormat format, int contrib_id = -1) {}
  virtual std::vector<uint32_t> read_event() = 0;
  virtual bool has_event() { return daq().getEventOccupancy() > 0; }

 protected:
  std::map<std::string, std::shared_ptr<I2C>> i2c_;
  std::map<std::string, std::shared_ptr<OptoLink>> opto_;
  mutable logging::logger the_log_{logging::get("Target")};
};

Target* makeTargetFiberless();
Target* makeTargetHcalBackplaneZCU(int ilink, uint8_t board_mask);
Target* makeTargetHcalBackplaneBittware(int ilink, uint8_t board_mask,
                                        const char* dev);
Target* makeTargetEcalSMMZCU(int ilink, uint8_t roc_mask);
Target* makeTargetEcalSMMBittware(int ilink, uint8_t rocmask, const char* dev);

}  // namespace pflib

#endif  // PFLIB_TARGET_H_INCLUDED
