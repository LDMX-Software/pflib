#ifndef PFLIB_DAQ_H_INCLUDED
#define PFLIB_DAQ_H_INCLUDED

#include <vector>
#include "pflib/WishboneInterface.h"

namespace pflib {

  /**
   * @class DAQ setup
   *
   */
class DAQ {
 public:
  DAQ(WishboneInterface* wb);

  // 
  void reset();
  //
  void getHeaderOccupancy(int& current, int& maximum);
  // Set the FPGA id and the link ids based on the FPGA id
  void setIds(int fpga_id);
  // Get the FPGA id
  int getFPGAid();
  // Setup a link
  void setupLink(int ilink, bool zs, bool zs_all, int l1a_delay, int l1a_capture_width);
  // 
  void getLinkSetup(int ilink, bool& zs, bool& zs_all, int& l1a_delay, int& l1a_capture_width);
  // get empty/full status for the given link and stage
  void bufferStatus(int ilink, bool postfmt, bool& empty, bool& full);
  // enable/disable the readout
  void enable(bool enable=true);
  // is the readout enabled?
  bool enabled();

  // number of elinks
  int nlinks() const { return n_links; }
 private:
  WishboneInterface* wb_;
  // number of links
  int n_links;
};      
  
}

#endif // PFLIB_DAQ_H_INCLUDED
