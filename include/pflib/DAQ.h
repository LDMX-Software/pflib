#ifndef PFLIB_DAQ_H_INCLUDED
#define PFLIB_DAQ_H_INCLUDED

#include <stdint.h>

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
  DAQ(int links) : n_links{links} {}

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

 private:
  /// number of links
  int n_links;
  /// enabled
  bool enabled_;
};

DAQ* get_DAQ_zcu();

}  // namespace pflib

#endif  // PFLIB_DAQ_H_INCLUDED
