#ifndef PFLIB_DAQ_H_INCLUDED
#define PFLIB_DAQ_H_INCLUDED

#include <stdint.h>

#include <map>
#include <string>
#include <vector>

namespace pflib {

/**
 * Interface with DAQ via a WishboneInterface
 *
 * This is a very light class focused on just providing helpful
 * functionality. Constructing/deconstructing these objects is very\
 * light so it can be done often.
 */
class DAQ {
 protected:
  DAQ(int links) : n_links{links}, samples_{1}, soi_{0} { for (int i=0; i<n_links; i++) econid_.push_back(0x300|i); }

 public:
  virtual void reset() = 0;
  ///
  virtual int getEventOccupancy() = 0;
  /// Setup a link.
  virtual void setupLink(int ilink, int l1a_delay, int l1a_capture_width) = 0;
  /// read link parameters into the passed variables
  virtual void getLinkSetup(int ilink, int& l1a_delay,
                            int& l1a_capture_width) = 0;
  /// get empty/full status for the given link and stage
  virtual void bufferStatus(int ilink, bool& empty, bool& full) = 0;

  /// setup overall event information for daq channels
  virtual void setup(int samples_per_ror, int soi = -1) {
    samples_ = samples_per_ror;
    soi_ = (soi < 0 || soi > samples_ - 1) ? (0) : soi;
  }
  virtual void setEconId(int ilink, int econid) {
    econid_[ilink] = econid;
  }
  /// get the econid
  int econid(int ilink) const { return econid_[ilink]; }
  /// get the samples
  int samples_per_ror() const { return samples_; }
  /// get the soi
  int soi() const { return soi_; }

  /// enable/disable the readout (overall control)
  virtual void enable(bool enable = true) { enabled_ = enable; }
  /// is the readout enabled? (overall control)
  virtual bool enabled() { return enabled_; }
  /// enable/disable the readout of given econ
  virtual void enableECON(int iecon, bool enable = true) {  }
  /// is the readout enabled for this ECON
  virtual bool enabledECON(int iecon) { return true; }
  /// number of elinks
  int nlinks() const { return n_links; }
  /// is AXIS enabled?
  virtual bool AXIS_enabled() { return false; }
  /// enable/disable AXIS
  virtual void AXIS_enable(bool enable) {}

  /// read out link data
  virtual std::vector<uint32_t> getLinkData(int ilink) = 0;
  /// Advance link read pointer
  virtual void advanceLinkReadPtr(int ilink) {}

  // get any useful debugging data
  virtual std::map<std::string, uint32_t> get_debug(uint32_t ask) {
    return std::map<std::string, uint32_t>();
  }

 private:
  /// number of links
  int n_links;
  /// enabled
  bool enabled_;
  std::vector<int> econid_;
  int samples_, soi_;
};

DAQ* get_DAQ_zcu();

}  // namespace pflib

#endif  // PFLIB_DAQ_H_INCLUDED
