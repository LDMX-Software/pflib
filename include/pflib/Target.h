#ifndef PFLIB_TARGET_H_INCLUDED
#define PFLIB_TARGET_H_INCLUDED

#include "pflib/DAQ.h"
#include "pflib/ECON.h"
#include "pflib/Elinks.h"
#include "pflib/FastControl.h"
#include "pflib/I2C.h"
#include "pflib/ROC.h"
#include "pflib/lpGBT.h"
#include "pflib/packing/SingleECONDRocErxMapping.h"

namespace pflib {

class OptoLink;
class TRIG;

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
  virtual int nrocs() = 0;

  /// number of econds
  virtual int necons() = 0;

  /** do we have a roc with this id? */
  virtual bool have_roc(int iroc) const = 0;

  /** do we have an econ with this id? */
  virtual bool have_econ(int iecon) const = 0;

  /** get a list of the IDs we have set up */
  virtual std::vector<int> roc_ids() const = 0;

  /** get a list of the econ IDs we have set up */
  virtual std::vector<int> econ_ids() const = 0;

  /** Get a ROC interface for the given HGCROC board */
  virtual ROC& roc(int which) = 0;

  /** get a ECON interface for the given econ board */
  virtual ECON& econ(int which) = 0;

  /** Generate a hard reset to all the HGCROC boards */
  virtual void hardResetROCs() = 0;

  /** generate a hard reset to all the ECON boards */
  virtual void hardResetECONs() = 0;

  /** Get the firmware version */
  virtual uint32_t getFirmwareVersion() { return -1; }

  /** Generate a soft reset to a specific HGCROC board, -1 for all */
  virtual void softResetROC(int which = -1) = 0;

  /** Generate a soft reset to a specific ECON board, -1 for all */
  virtual void softResetECON(int which = -1) = 0;

  /** get the Elinks object */
  virtual Elinks& elinks() = 0;

  /** get the FastControl object */
  virtual FastControl& fc() = 0;

  /** get the DAQ object */
  virtual DAQ& daq() = 0;

  /** get the TRIG object, if it exists (may return NULL) */
  virtual TRIG* trig() { return 0; }

  /// names of different I2C busses we could talk to
  std::vector<std::string> i2c_bus_names();

  /// get an I2C bus by name
  I2C& get_i2c_bus(const std::string& name);

  /// names of different Optical Links we could talk to
  std::vector<std::string> opto_link_names() const;

  /// get an OptoLink by name
  OptoLink& get_opto_link(const std::string& name) const;

  /**
   * Define the ROC-half -> eRx input into the ECON-D for the hardware
   *
   * This does *not* include which ROCs are active (i.e. assume all the ROCs are
   * active). This is used when constructing the SingleECONDRocErxMapping object
   * that is used to to the index conversions for us.
   *
   * @return vector whose index is i_roc and value is the pair of eRx for that
   * ROC in ROC-half order (so the lower ROC half is the "first" eRx in the
   * pair).
   */
  virtual const std::vector<std::pair<int, int>>&
  getHardwareRocErxMappingDAQ() = 0;

  /**
   * Define the ROC-TRG -> eRx input into the ECON-T for the hardware
   *
   * This does *not* include which ROCs are active.
   * This is used in link alignment which is responsible for checking
   * which ROCs are active.
   *
   * @return vetor whose index is i_roc and value is the pair of ECON index
   * and a vector of eRx in ROC-TRG order (so ROC-TRG0 is the zero'th entry
   * in the pair's vector).
   */
  virtual const std::vector<std::pair<int, std::vector<int>>>&
  getHardwareRocErxMappingTRG() = 0;

  /**
   * get the mapping that can be used to convert between (i_erx, link_chan)
   * and (i_roc, chan) indices
   */
  const packing::SingleECONDRocErxMapping& getRocErxMapping();

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

  virtual void setup_run(int irun, DaqFormat format, int contrib_id = -1) = 0;
  virtual std::vector<uint32_t> read_event() = 0;
  virtual bool has_event() { return daq().getEventOccupancy() > 0; }

  /**
   * temporarily apply parameters to all of the ROCs connected to a target
   * and then unset them
   *
   * This is specifically for ROCs and not templated to support ECONs because
   * I could not think of a situation where this would be helpful for the ECONs.
   */
  class TempParametersAllROCs {
   public:
    /// applies the same parameters to all the ROCs and holds the previous
    /// registers
    TempParametersAllROCs(
        Target* tgt,
        const std::map<std::string, std::map<std::string, uint64_t>>&
            parameters);
    /// different parameter sets depending on ROC index
    TempParametersAllROCs(
        Target* tgt,
        const std::map<int,
                       std::map<std::string, std::map<std::string, uint64_t>>>&
            parameters);
    /// cannot copy or assign this lock
    TempParametersAllROCs(const TempParametersAllROCs&) = delete;
    TempParametersAllROCs& operator=(const TempParametersAllROCs&) = delete;
    ~TempParametersAllROCs();

   private:
    /// handle to target holding ROCs
    Target* tgt_;
    /// set of prior registers separated by ROC
    std::map<int, std::map<int, std::map<int, uint8_t>>> prior_registers_;
  };

  /// *temporarily* apply the same parameters to all the ROCs
  /// these parameters are unset when the returned object goes out of scope
  TempParametersAllROCs tempApplyAllROCs(
      const std::map<std::string, std::map<std::string, uint64_t>>& parameters);

  /// *temporarily* apply some parameters (varying depending on ROC)
  /// these parameters are unset when the returned object goes out of scope
  TempParametersAllROCs tempApplyAllROCs(
      const std::map<int,
                     std::map<std::string, std::map<std::string, uint64_t>>>&
          parameters);

 protected:
  std::map<std::string, std::shared_ptr<I2C>> i2c_;
  std::map<std::string, std::shared_ptr<OptoLink>> opto_;
  mutable logging::logger the_log_{logging::get("Target")};

 private:
  std::unique_ptr<packing::SingleECONDRocErxMapping> mapping_;
};

Target* makeTargetFiberless();
Target* makeTargetHcalBackplaneZCU(int ilink, uint8_t board_mask);
Target* makeTargetHcalBackplaneBittware(int ilink, uint8_t board_mask,
                                        const char* dev);
Target* makeTargetEcalSMMZCU(int ilink, uint8_t roc_mask);
Target* makeTargetEcalSMMBittware(int ilink, uint8_t rocmask, const char* dev);

}  // namespace pflib

#endif  // PFLIB_TARGET_H_INCLUDED
