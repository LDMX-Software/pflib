#include "DMAReader.h"
#include <rogue/hardware/drivers/AxisDriver.h>

DMAReader::DMAReader(const char* dma_device_path, bool use_index, bool debug) 
  : use_index_{use_index}, debug_{debug},
    rogue::interfaces::stream::Master() {
  if ( (dma_device_handle_ = open(dma_device_path, O_RDWR)) <= 0 ) {
    throw std::runtime_error("Error opening dma device "+std::string(dma_device_path));
  }

  uint8_t mask[DMA_MASK_SIZE];
  dmaInitMaskBytes(mask);
  memset(mask,0xFF,DMA_MASK_SIZE);
  dmaSetMaskBytes(dma_device_handle_,mask);
  max_size_ = 1024*1024*2;
  if (use_index_) {
     if ((dma_buffers_ = dmaMapDma(dma_device_handle_,&dma_count_,&dma_size_)) == NULL ) {
       throw std::runtime_error("Failed to map dma buffers.");
     }
  } else {
     if ((rx_data_ = malloc(max_size_)) == NULL ) {
       throw std::runtime_error("Failed to allocate raw memory.");
     }
  }
}

DMAReader::~DMAReader() {
  if (use_index_) dmaUnMapDma(dma_device_handle_,dma_buffers_);
  else free(rx_data_);
  close(dma_device_handle_);
}

void DMAReader::watch() {
  uint32_t count  = 0;
  bool prbRes{false};
  fd_set fds;
  struct timeval timeout;
  uint32_t rxDest, rxFuser, rxLuser, rxFlags;
  uint32_t dmaIndex;
  while(true) {
    // Setup fds for select call
    FD_ZERO(&fds);
    FD_SET(dma_device_handle_,&fds);

    // Setup select timeout for 1 second
    timeout.tv_sec=2;
    timeout.tv_usec=0;

    // Wait for Socket data ready
    ret = select(dma_device_handle_+1,&fds,NULL,NULL,&timeout);
    if (ret <= 0) {
      if (debug_) printf("Read timeout\n");
    } else {
      timeval tv;
      gettimeofday(&tv,0);
     
      // DMA Read
      if (use_index_) {
        ret = dmaReadIndex(dma_device_handle_,&dmaIndex,&rxFlags,NULL,&rxDest);
        rx_data_ = dma_buffers_[dmaIndex];
      } else 
        ret = dmaRead(dma_device_handle_,rx_data_,max_size_,&rxFlags,NULL,&rxDest);

      rxFuser = axisGetFuser(rxFlags);
      rxLuser = axisGetFuser(rxFlags);

      if ( ret > 0 ) {
        if (use_index_) dmaRetIndex(dma_device_handle_,dmaIndex);
        count++;
        if (debug_) {
          printf("Read ret=%i, Dest=%i, Fuser=0x%.2x, Luser=0x%.2x, prbs=%i, count=%i, t=%d.%d\n",
            ret,rxDest,rxFuser,rxLuser,prbRes,count,tv.tv_sec,tv.tv_usec);
        }
        //rx_data_ is what we want to send here
        //ret has the number of bytes pointed to in contiquous array rx_data_
        // allocate a frame with ret bytes
        auto frame = this->reqFrame(ret /*n bytes*/, true);
        frame->setPayload(ret);
        // copy our data into frame
        std::memcpy(frame->begin().ptr(), rx_data_, ret);
        // SEND IT
        this->sendFrame(frame);
        // pause to prevent copying of frame
        usleep(500);
      }
    }
  }
}
