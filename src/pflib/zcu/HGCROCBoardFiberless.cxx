#include "pflib/zcu/HGCROCBoardFiberless.h"

namespace pflib {

int FiberlessCapture::getBitslip(int ilink) {
  int ictl = ctl_for(ilink);
  return uio_.readMasked(ictl, MASK_BITSLIP);
}

void FiberlessCapture::setBitslip(int ilink, int bitslip) {
  int ictl = ctl_for(ilink);
  uio_.writeMasked(ictl, MASK_BITSLIP, bitslip);
}

void FiberlessCapture::setAlignPhase(int ilink, int phase) {
  int ictl = ctl_for(ilink);
  uio_.writeMasked(ictl, MASK_PHASE, phase);
}

int FiberlessCapture::getAlignPhase(int ilink) {
  int ictl = ctl_for(ilink);
  return uio_.readMasked(ictl, MASK_PHASE);
}

std::vector<uint32_t> FiberlessCapture::spy(int ilink) {
  std::vector<uint32_t> rv;
  int spy_addr = ADDR_LINK_STATUS_BASE + 2 * ilink;
  rv.push_back(uio_.read(spy_addr));
  return rv;
  ;
}

void FiberlessCapture::reset() {
  uio_.rmw(ADDR_TOP_CTL, MASK_RESET_BUFFER, MASK_RESET_BUFFER);
}
int FiberlessCapture::getEventOccupancy() {
  // use link 0 for occupancy
  return uio_.readMasked(ADDR_LINK_STATUS_BASE + ADDR_OFFSET_BUFSTATUS,
                         MASK_OCCUPANCY);
}
void FiberlessCapture::setupLink(int ilink, int l1a_delay,
                                 int l1a_capture_width) {
  int ictl = ctl_for(ilink);
  // gather lengths if we don't have them
  if (l1a_capture_width_.empty()) {
    int a, b;
    for (int i = 0; i < DAQ::nlinks(); i++) {
      getLinkSetup(i, a, b);
      l1a_capture_width_.push_back(b);
    }
  }
  l1a_capture_width_[ilink] = l1a_capture_width;

  uio_.writeMasked(ictl, MASK_CAPTURE_DELAY, l1a_delay);
  uio_.writeMasked(ictl, MASK_CAPTURE_WIDTH, l1a_capture_width);
}
void FiberlessCapture::getLinkSetup(int ilink, int& l1a_delay,
                                    int& l1a_capture_width) {
  int ictl = ctl_for(ilink);
  l1a_delay = uio_.readMasked(ictl, MASK_CAPTURE_DELAY);
  l1a_capture_width = uio_.readMasked(ictl, MASK_CAPTURE_WIDTH);
}
void FiberlessCapture::bufferStatus(int ilink, bool& empty, bool& full) {
  empty = true;
  full = true;  // implauible
  if (ilink < 0 || ilink >= DAQ::nlinks()) return;
  uint32_t ifl =
      uio_.readMasked(ADDR_LINK_STATUS_BASE + ADDR_OFFSET_BUFSTATUS + ilink * 2,
                      MASK_BUFFER_FULL);
  uint32_t iemp =
      uio_.readMasked(ADDR_LINK_STATUS_BASE + ADDR_OFFSET_BUFSTATUS + ilink * 2,
                      MASK_BUFFER_EMPTY);
  empty = (iemp != 0);
  full = (ifl != 0);
}

std::vector<uint32_t> FiberlessCapture::getLinkData(int ilink) {
  std::vector<uint32_t> rv;
  if (ilink < 0 || ilink >= DAQ::nlinks()) return rv;
  // gather lengths if we don't have them
  if (l1a_capture_width_.empty()) {
    int a, b;
    for (int i = 0; i < DAQ::nlinks(); i++) {
      getLinkSetup(i, a, b);
      l1a_capture_width_.push_back(b);
    }
  }

  size_t addr = 0x200 | (ilink << 6);
  // printf("%d %d %x\n", ilink, addr, addr);
  for (size_t i = 0; i < l1a_capture_width_[ilink]; i++)
    rv.push_back(uio_.read(addr + i));
  return rv;
}

void FiberlessCapture::advanceLinkReadPtr() {
  if (getEventOccupancy() > 0)
    uio_.rmw(ADDR_TOP_CTL, MASK_ADVANCE_FIFO, MASK_ADVANCE_FIFO);
}

void HcalFiberless::setup_run(int run, DaqFormat format, int contrib_id) {
  run_ = run;
  daqformat_ = format;
  if (contrib_id < 0)
    contribid_ = 255;
  else
    contribid_ = contrib_id & 0xFF;
  ievt_ = 0;
  l1a_ = 0;
  daq().reset();
  fc().clear_run();
}

std::vector<uint32_t> HcalFiberless::read_event() {
  std::vector<uint32_t> buffer;
  if (has_event()) {
    ievt_++;
    switch (daqformat_) {
      case DaqFormat::SIMPLEROC: {
        buffer.push_back(0x11888811);
        buffer.push_back(0xbeef2025);

        buffer.push_back(0);  // come back to this
        for (int i = 0; i < (daq().nlinks() + 1) / 2; i++)
          buffer.push_back(0);  // come back to this
        size_t len_total = buffer.size() - 2;

        for (int i = 0; i < daq().nlinks(); i++) {
          std::vector<uint32_t> data = daq().getLinkData(i);
          if (i >= 2) {  // trigger links
            uint32_t theader = 0x30000000 | ((i - 2)) | (data.size() << 8);
            data.insert(data.begin(), theader);
          }
          size_t len = data.size();
          len_total += len;
          buffer.insert(buffer.end(), data.begin(), data.end());
          // insert the subpacket length
          if (i % 2)
            buffer[2 + 1 + i / 2] |= (len << 16);
          else
            buffer[2 + 1 + i / 2] |= (len);
        }
        // record the total length
        buffer[2] |= len_total;
        buffer.push_back(0xd07e2025);
        buffer.push_back(0x12345678);
        daq().advanceLinkReadPtr();
      } break;
      case DaqFormat::ECOND_SW_HEADERS: {
        const int bc = 0;  // bx number...
        for (int il1a = 0; il1a < daq().samples_per_ror(); il1a++) {
          // assume orbit zero, L1A spaced by two
          formatter_.startEvent(bc + il1a * 2, l1a_ + il1a, 0);
          // only consuming DAQ links in ECOND (D for DAQ)
          for (int i = 0; i < 2; i++) {
            formatter_.add_elink_packet(i, daq().getLinkData(i));
          }
          formatter_.finishEvent();

          // add header giving specs around ECOND packet
          buffer.push_back(pflib::packing::DAQSampleHeader{
              .version = 1,
              .econd_id = static_cast<uint32_t>(daq().econid()),
              .i_l1a = static_cast<uint32_t>(il1a),
              .is_soi = (il1a == daq().soi()),
              .econd_len = static_cast<uint32_t>(formatter_.getPacket().size())}
                               .to());

          // insert ECOND packet into buffer
          buffer.insert(buffer.end(), formatter_.getPacket().begin(),
                        formatter_.getPacket().end());

          // advance L1A pointer
          daq().advanceLinkReadPtr();
        }
        l1a_ += daq().samples_per_ror();
        // add a special "header" to mark that we have no more ECON packets
        buffer.push_back(pflib::packing::DAQSampleHeader::ending_trailer());
      } break;
      default: {
        PFEXCEPTION_RAISE("NoImpl", "DaqFormat provided is not implemented");
      }
    }
  }
  return buffer;
}

Target* makeTargetFiberless() { return new HcalFiberless(); }

}  // namespace pflib
