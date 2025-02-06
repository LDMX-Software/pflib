#include "pflib/Elinks.h"
#include "pflib/DAQ.h"
#include "pflib/zcu/UIO.h"
#include <memory>

namespace pflib {

class Capture_zcu : public Elinks, public DAQ {
  public:
  Capture_zcu() : Elinks(6),DAQ(6), uio_("/dev/uio5",16*4096) { // currently covers all elinks
    }

    virtual int getBitslip(int ilink);
    virtual uint32_t getStatusRaw(int ilink) { return 0; }
    virtual void setBitslip(int ilink, int bitslip);
    virtual std::vector<uint32_t> spy(int ilink);
    virtual void clearErrorCounters(int ilink) { }
    virtual void resetHard() { }
    virtual void setAlignPhase(int ilink, int phase);
    virtual int getAlignPhase(int ilink);

  // DAQ-related
  virtual void reset();
  virtual int getEventOccupancy();
  virtual void setupLink(int ilink, int l1a_delay, int l1a_capture_width);
  virtual void getLinkSetup(int ilink, int& l1a_delay, int& l1a_capture_width);
  virtual void bufferStatus(int ilink, bool& empty, bool& full);
  virtual std::vector<uint32_t> getLinkData(int ilink);
  virtual void advanceLinkReadPtr();
  
  private:
    int ctl_for(int ilink) {
      if (ilink<2) return 4+ilink;
      else return 4+4+(ilink-2);
    }
    UIO uio_;
  std::vector<int> l1a_capture_width_;
  };

  
static const uint32_t MASK_CAPTURE_WIDTH  = 0x3F000000;
static const uint32_t MASK_CAPTURE_DELAY  = 0x00FF0000;
static const uint32_t MASK_BITSLIP        = 0x00003E00;
static const uint32_t MASK_PHASE          = 0x000001FF;

static const uint32_t MASK_RESET_BUFFER   = 0x00010000;
static const uint32_t MASK_ADVANCE_FIFO   = 0x00020000;
static const uint32_t MASK_SOFTWARE_L1A   = 0x00040000;

static const uint32_t MASK_OCCUPANCY      = 0x000000FF;

static const size_t ADDR_TOP_CTL          =  0x0;
static const size_t ADDR_LINK_STATUS_BASE = 0x26;
static const size_t ADDR_OFFSET_BUFSTATUS =    1;

int Capture_zcu::getBitslip(int ilink) {
  int ictl=ctl_for(ilink);
  return uio_.readMasked(ictl,MASK_BITSLIP);
}

void Capture_zcu::setBitslip(int ilink, int bitslip) {
  int ictl=ctl_for(ilink);
  uio_.writeMasked(ictl,MASK_BITSLIP,bitslip);    
}

void Capture_zcu::setAlignPhase(int ilink, int phase) {
  int ictl=ctl_for(ilink);
  uio_.writeMasked(ictl,MASK_PHASE,phase);      
}

int Capture_zcu::getAlignPhase(int ilink) {
  int ictl=ctl_for(ilink);
  return uio_.readMasked(ictl,MASK_PHASE);      
}

std::vector<uint32_t> Capture_zcu::spy(int ilink) {
  std::vector<uint32_t> rv;
  int spy_addr=ADDR_LINK_STATUS_BASE+2*ilink;
  rv.push_back(uio_.read(spy_addr));
  return rv;;
}

void Capture_zcu::reset() {
  uio_.rmw(ADDR_TOP_CTL,MASK_RESET_BUFFER,MASK_RESET_BUFFER);
}
int Capture_zcu::getEventOccupancy() {
  // use link 0 for occupancy
  return uio_.readMasked(ADDR_LINK_STATUS_BASE+ADDR_OFFSET_BUFSTATUS,MASK_OCCUPANCY);
}
void Capture_zcu::setupLink(int ilink, int l1a_delay, int l1a_capture_width) {
  int ictl=ctl_for(ilink);
  // gather lengths if we don't have them
  if (l1a_capture_width_.empty()) {
    int a,b;
    for (int i=0; i<DAQ::nlinks(); i++) {
      getLinkSetup(i,a,b);
      l1a_capture_width_.push_back(b);
    }
  }
  l1a_capture_width_[ilink]=l1a_capture_width;

  uio_.writeMasked(ictl,MASK_CAPTURE_DELAY,l1a_delay);
  uio_.writeMasked(ictl,MASK_CAPTURE_WIDTH,l1a_capture_width);  
}
void Capture_zcu::getLinkSetup(int ilink, int& l1a_delay, int& l1a_capture_width) {
  int ictl=ctl_for(ilink);
  l1a_delay=uio_.readMasked(ictl,MASK_CAPTURE_DELAY);
  l1a_capture_width=uio_.readMasked(ictl,MASK_CAPTURE_WIDTH);
}
void Capture_zcu::bufferStatus(int ilink, bool& empty, bool& full) {
}

std::vector<uint32_t> Capture_zcu::getLinkData(int ilink) {
  std::vector<uint32_t> rv;
  if (ilink<0 || ilink>=DAQ::nlinks()) return rv;
  // gather lengths if we don't have them
  if (l1a_capture_width_.empty()) {
    int a,b;
    for (int i=0; i<DAQ::nlinks(); i++) {
      getLinkSetup(i,a,b);
      l1a_capture_width_.push_back(b);
    }
  }
  
  size_t addr=0x200|(ilink<<6);
  printf("%d %d %x\n",ilink,addr,addr);
  for (size_t i=0; i<l1a_capture_width_[ilink]; i++)
    rv.push_back(uio_.read(addr+i));
  return rv;
}

void Capture_zcu::advanceLinkReadPtr() {
  if (getEventOccupancy()>0) 
    uio_.rmw(ADDR_TOP_CTL,MASK_ADVANCE_FIFO,MASK_ADVANCE_FIFO);
}

static std::unique_ptr<Capture_zcu> the_capture_;
  
  Elinks* get_Elinks_zcu() {
    if (!(the_capture_)) the_capture_=std::make_unique<Capture_zcu>(); 
    return the_capture_.get();
  }
  DAQ* get_DAQ_zcu() {
    if (!(the_capture_)) the_capture_=std::make_unique<Capture_zcu>(); 
    return the_capture_.get();
  }

}
