/**
 * dma_read.cpp helping develop a rogue source to readout data written 
 * to DMA via firmware.
 *
 * Adapted from:
 *-----------------------------------------------------------------------------
 * Title      : DMA read utility
 * ----------------------------------------------------------------------------
 * File       : dmaRead.cpp
 * Author     : Ryan Herbst, rherbst@slac.stanford.edu
 * Created    : 2016-08-08
 * Last update: 2016-08-08
 * ----------------------------------------------------------------------------
 * Description:
 * This program will open up a AXIS DMA port and attempt to read data.
 * ----------------------------------------------------------------------------
 * This file is part of the aes_stream_drivers package. It is subject to
 * the license terms in the LICENSE.txt file found in the top-level directory
 * of this distribution and at:
 *    https://confluence.slac.stanford.edu/display/ppareg/LICENSE.html.
 * No part of the aes_stream_drivers package, including this file, may be
 * copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE.txt file.
 * ----------------------------------------------------------------------------
 */

#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <argp.h>
#include <iostream>
#include <rogue/hardware/drivers/AxisDriver.h>
#include <rogue/interfaces/stream/Master.h>
#include <rogue/interfaces/stream/Frame.h>
#include <rogue/interfaces/stream/FrameIterator.h>
#include <rogue/utilities/fileio/StreamWriter.h>
#include <rogue/utilities/fileio/StreamWriterChannel.h>

using namespace std;

const  char * argp_program_version = "pgpRead 1.0";
const  char * argp_program_bug_address = "rherbst@slac.stanford.edu";

struct PrgArgs {
   const char * path;
   const char * dest;
   uint32_t     prbsDis;
   uint32_t     idxEn;
   uint32_t     rawEn;
};

static struct PrgArgs DefArgs = { "/dev/axi_stream_dma_0", NULL, 0x0, 0x0, 0 };

static char   args_doc[] = "";
static char   doc[]      = "";

static struct argp_option options[] = {
   { "path",    'p', "PATH",   OPTION_ARG_OPTIONAL, "Path of pgpcard device to use. Default=/dev/axi_stream_dma_0.",0},
   { "dest",    'm', "PATH",   OPTION_ARG_OPTIONAL, "Output file",0},
   { "indexen", 'i', 0,        OPTION_ARG_OPTIONAL, "Use index based receive buffers.",0},
   { "rawEn",   'r', "COUNT",  OPTION_ARG_OPTIONAL, "Show raw data up to count.",0},
   {0}
};

error_t parseArgs ( int key,  char *arg, struct argp_state *state ) {
   struct PrgArgs *args = (struct PrgArgs *)state->input;
   switch(key) {
      case 'p': args->path = arg; break;
      case 'm': args->dest = arg; break;
      case 'd': args->prbsDis = 1; break;
      case 'i': args->idxEn = 1; break;
      case 'r': args->rawEn = strtol(arg,NULL,10); break;
      default: return ARGP_ERR_UNKNOWN; break;
   }
   return(0);
}

static struct argp argp = {options,parseArgs,args_doc,doc};

/**
 * The actual data source (or "master") that we use to watch the DMA device
 * and send event packets along.
 */
class DMASource : public rogue::interfaces::stream::Master {
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
  static std::shared_ptr<DMASource> create() {
    static auto ptr = std::make_shared<DMASource>();
    return ptr;
  }
  */
  DMASource(const char* dma_device_path, bool use_index = false) 
    : use_index_{use_index},
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
  ~DMASource() {
    if (use_index_) dmaUnMapDma(dma_device_handle_,dma_buffers_);
    else free(rx_data_);
    close(dma_device_handle_);
  }
  /**
   * Watch and send events when shit happens
   *
   * This has the infinite while(true) loop in it so make sure
   * everything is setup before this.
   */
  void watch() {
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
        printf("Read timeout\n");
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
          printf("Read ret=%i, Dest=%i, Fuser=0x%.2x, Luser=0x%.2x, prbs=%i, count=%i, t=%d.%d\n",
              ret,rxDest,rxFuser,rxLuser,prbRes,count,tv.tv_sec,tv.tv_usec);
          //rx_data_ is what we want to send here
          //ret has the number of bytes pointed to in contiquous array rx_data_
          // allocate a frame with ret bytes
          auto frame = this->reqFrame(ret /*n bytes*/, true);
          frame->setPayload(ret);
          // copy our data into frame
          std::memcpy(frame->begin().ptr(), rx_data_, ret);
          // SEND IT
          this->sendFrame(frame);
        }
      }
    }
  }
};

int main (int argc, char **argv) {
  struct PrgArgs args;

  memcpy(&args,&DefArgs,sizeof(struct PrgArgs));
  argp_parse(&argp,argc,argv,0,0,&args);

  try {
    DMASource src(args.path, args.idxEn);
    if (args.dest) {
      rogue::utilities::fileio::StreamWriterPtr writer{
        rogue::utilities::fileio::StreamWriter::create()};
      writer->setBufferSize(10000);
      writer->open(args.dest);
      src.addSlave(writer->getChannel(0));
    }
    // enter watching loop
    src.watch();
  } catch (const std::exception& e) {
    std::cerr << "ERROR: " << e.what() << std::endl;
    return 1;
  }
  return 0;
}
