#ifndef ROGUE_WISHBONE_INTERFACE_H_
#define ROGUE_WISHBONE_INTERFACE_H_ 1

#include <memory>

#include "pflib/Backend.h"
#include "pflib/WishboneInterface.h"

// Forward declaration of the TCP client
namespace rogue {
namespace interfaces {
namespace memory {
class TcpClient;
class Master;
}  // namespace memory
namespace stream {
class TcpClient;
class Slave;
}  // namespace stream
}  // namespace interfaces
namespace utilities {
namespace fileio {
class StreamWriter;
}
}  // namespace utilities
}  // namespace rogue

namespace pflib {
namespace rogue {

class RogueWishboneInterface : public WishboneInterface, public Backend {
 public:
  /** Construct a TCP bridge*/
  RogueWishboneInterface(const std::string& host, int port);

  virtual ~RogueWishboneInterface();

  /**
   * write a 32-bit word to the given target and address
   * @throws pflib::Exception in the case of a timeout or wishbone error
   */
  virtual void wb_write(int target, uint32_t addr, uint32_t data);

  /**
   * read a 32-bit word from the given target and address
   * @throws pflib::Exception in the case of a timeout or wishbone error
   */
  virtual uint32_t wb_read(int target, uint32_t addr);

  /**
   * reset the wishbone bus (on/off cycle)
   */
  virtual void wb_reset();

  /**
   * Read the monitoring counters
   *  crcup_errors -- CRC errors observed on the uplink (wraps)
   *  crcdn_errors -- CRC errors from the downlink reported on the uplink
   * (wraps) wb_errors -- Wishbone errors indicated by the Polarfire
   */
  virtual void wb_errors(uint32_t& crcup_errors, uint32_t& crcdn_errors,
                         uint32_t& wb_errors);

  /**
   * Clear the monitoring counters
   */
  virtual void wb_clear_counters();

  /// Backend implementation
  virtual void fc_sendL1A();
  virtual void fc_linkreset();
  virtual void fc_bufferclear();
  virtual void fc_calibpulse();
  virtual void fc_clear_run();
  virtual void fc_setup_calib(int pulse_len, int l1a_offset);
  virtual void fc_get_setup_calib(int& pulse_len, int& l1a_offset);
  virtual void fc_read_counters(int& spill_count, int& header_occ,
                                int& event_count, int& vetoed);
  virtual void fc_enables_read(bool& ext_l1a, bool& ext_spill, bool& timer_l1a);
  virtual void fc_enables(bool ext_l1a, bool ext_spill, bool timer_l1a);
  virtual int fc_timer_setup_read();
  virtual void fc_timer_setup(int usdelay);

  virtual void daq_reset();
  virtual void daq_advance_ptr();
  virtual void daq_status(bool& full, bool& empty, int& nevents,
                          int& next_event_size);
  virtual std::vector<uint32_t> daq_read_event();
  virtual void daq_setup_event_tag(int run, int day, int month, int hour,
                                   int min);

  /// specific items related to DMA which are not part of the general interface
  void daq_dma_enable(bool enable);
  void daq_dma_setup(uint8_t fpga_id, uint8_t samples_per_event);
  void daq_get_dma_setup(uint8_t& fpga_id, uint8_t& samples_per_event,
                         bool& enabled);
  uint32_t daq_dma_status();
  void daq_dma_dest(const std::string& fname);
  void daq_dma_dest(std::shared_ptr<::rogue::interfaces::stream::Slave> sl);
  void daq_dma_run(const std::string& cmd, int run, int nevents, int rate);
  void daq_dma_close();

 private:
  std::shared_ptr<::rogue::interfaces::memory::TcpClient> client_;
  std::shared_ptr<::rogue::interfaces::memory::Master> intf_;

  // for dma readout, these will just sit silently while dma is disabled
  std::shared_ptr<::rogue::interfaces::stream::TcpClient> dma_client_;
  std::shared_ptr<::rogue::utilities::fileio::StreamWriter> dma_dest_;
};

}  // namespace rogue
}  // namespace pflib

#endif  // ROGUE_WISHBONE_INTERFACE_H_
