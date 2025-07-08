#ifndef PFLIB_DAQ_H_INCLUDED
#define PFLIB_DAQ_H_INCLUDED

#include <stdint.h>

#include <vector>
#include <map>

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
  DAQ(int links) : n_links{links}, econid_{0xFFF}, samples_{1}, soi_{0} {}

 public:
  virtual void reset() = 0;
  ///
  virtual int getEventOccupancy() = 0;
  /// Set the FPGA id and the link ids based on the FPGA id
  // void setIds(int fpga_id);
  /// Get the FPGA id
  // int getFPGAid();
  /// Setup a link.
  virtual void setupLink(int ilink, int l1a_delay, int l1a_capture_width) = 0;
  /// read link parameters into the passed variables
  virtual void getLinkSetup(int ilink, int& l1a_delay,
                            int& l1a_capture_width) = 0;
  /// get empty/full status for the given link and stage
  virtual void bufferStatus(int ilink, bool& empty, bool& full) = 0;

  /// setup overall event information for daq channels
  virtual void setup(int econid, int samples_per_ror, int soi = -1) {
    econid_ = econid;
    samples_ = samples_per_ror;
    soi_ = (soi < 0 || soi > samples_ - 1) ? (0) : soi;
  }
  /// get the econid
  int econid() const { return econid_; }
  /// get the samples
  int samples_per_ror() const { return samples_; }
  /// get the soi
  int soi() const { return soi_; }

  /// enable/disable the readout
  virtual void enable(bool enable = true) { enabled_ = enable; }
  /// is the readout enabled?
  virtual bool enabled() { return enabled_; }
  /// number of elinks
  int nlinks() const { return n_links; }
  /// read out link data
  virtual std::vector<uint32_t> getLinkData(int ilink) = 0;
  /// Advance link read pointer
  virtual void advanceLinkReadPtr() {}

  // get any useful debugging data
  virtual std::map<std::string, uint32_t> get_debug() { return std::map<std::string, uint32_t>(); }
  
 private:
  /// number of links
  int n_links;
  /// enabled
  bool enabled_;
  int econid_;
  int samples_, soi_;
};

DAQ* get_DAQ_zcu();

}  // namespace pflib

#endif  // PFLIB_DAQ_H_INCLUDED
