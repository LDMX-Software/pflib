#ifndef DMAREADER_H
#define DMAREADER_H

#include <rogue/interfaces/stream/Master.h>

/**
 * The actual data source (or "master") that we use to watch the DMA device
 * and send event packets along.
 */
class DMAReader : public rogue::interfaces::stream::Master {
  bool use_index_; // should we use dma index? or raw memory?
  uint32_t max_size_; // maximum size of raw memory
  uint32_t dma_size_; // dma size
  uint32_t dma_count_; // number of dma slots?
  void **  dma_buffers_; // only used if use_index_
  void *   rx_data_; // only used if not use_index_
  uint32_t ret; // dummy return value to check status
  int32_t dma_device_handle_; // handle to dma device
 public:
   /*
  static std::shared_ptr<DMAReader> create() {
    static auto ptr = std::make_shared<DMAReader>();
    return ptr;
  }
  */
  DMAReader(const char* dma_device_path, bool use_index = false);
  ~DMAReader();
  /**
   * Watch and send events when shit happens
   *
   * This has the infinite while(true) loop in it so make sure
   * everything is setup before this.
   */
  void watch();
}; // DMAReader

#endif // DMAREADER_H
