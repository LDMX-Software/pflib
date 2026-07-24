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
  DAQ(int links);

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
  virtual void setup(int econid, int samples_per_ror, int soi = -1);
  /// get the econid
  int econid() const;
  /// get the samples
  int samples_per_ror() const;
  /// get the soi
  int soi() const;

  /// enable/disable the readout
  virtual void enable(bool enable = true);
  /// is the readout enabled?
  virtual bool enabled();
  /// number of elinks
  int nlinks() const;
  /// is AXIS enabled?
  virtual bool AXIS_enabled();
  /// enable/disable AXIS
  virtual void AXIS_enable(bool enable);

  /// read out link data
  virtual std::vector<uint32_t> getLinkData(int ilink) = 0;
  /// Advance link read pointer
  virtual void advanceLinkReadPtr() = 0;

  /// get any useful debugging data
  virtual std::map<std::string, uint32_t> get_debug(uint32_t ask);

  /**
   * readout an event including emulation of the headers the firmware inserts
   *
   * The Bittware firmware includes headers when copying the data into the axi
   * stream and we include those here so that the non-axis readout can have
   * data that is in the same format as axis data.
   */
  std::vector<uint32_t> read_event_sw_headers();

 private:
  /// number of links
  int n_links;
  /// enabled
  bool enabled_;
  /// id for the econ we are reading
  int econid_;
  /// number of samples per readout request
  int samples_;
  /// index for sample of interest in set of samples
  int soi_;
};

}  // namespace pflib

#endif  // PFLIB_DAQ_H_INCLUDED
