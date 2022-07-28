#include "PolarfireWB.h"
#include <rogue/interfaces/memory/TransactionLock.h>
#include <rogue/interfaces/memory/Constants.h>
#include <rogue/interfaces/memory/Transaction.h>
#include <rogue/interfaces/memory/TransactionLock.h>
#include <rogue/GeneralError.h>
#include <rogue/GilRelease.h>
#include <memory>
#include <cstring>
#include <thread>
#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define MAP_SIZE 4096
#define MAPDAQ_SIZE (4096*4)

PolarfireWB::PolarfireWB(uint64_t baseAddr) : rogue::interfaces::memory::Slave(4,1024), fd_(-1), base_(0), base_fc_(0), base_daq_(0) {

  log_ = rogue::Logging::create("PolarfireWB");

  /// open /dev/mem for mapping (must be root of course)
  if ((fd_ = open("/dev/mem", O_RDWR | O_SYNC)) == -1) {
    throw(rogue::GeneralError::create("PolarfireWB::PolarfireWB", "Failed to open device file: /dev/mem"));
  }
  /// map the memory
  base_=(uint32_t*) (mmap(0,MAP_SIZE,PROT_READ|PROT_WRITE,MAP_SHARED, fd_, baseAddr));
  if (base_ == (uint32_t*)-1) {
    throw(rogue::GeneralError::create("PolarfireWB::PolarfireWB", "Failed to map memory 0x%8x to user space.",baseAddr));
  }

   // Start read thread
   threadEn_ = true;
   thread_ = new std::thread(&PolarfireWB::runThread, this);
}

void PolarfireWB::map_fc(uint64_t baseAddr_fc) {
  if (!base_fc_) {
    base_fc_=(uint32_t*) (mmap(0,MAP_SIZE,PROT_READ|PROT_WRITE,MAP_SHARED, fd_, baseAddr_fc&0xFFFFF000));
    if (base_fc_ == (uint32_t*)-1) {
      throw(rogue::GeneralError::create("PolarfireWB::PolarfireWB", "Failed to map memory 0x%8x to user space.",baseAddr_fc));
    }
  }
}

void PolarfireWB::map_daq(uint64_t baseAddr_daq) {
  if (!base_daq_) {
    base_daq_=(uint32_t*) (mmap(0,MAPDAQ_SIZE,PROT_READ|PROT_WRITE,MAP_SHARED, fd_, baseAddr_daq&0xFFFFC000));
    if (base_daq_ == (uint32_t*)-1) {
      throw(rogue::GeneralError::create("PolarfireWB::PolarfireWB", "Failed to map memory 0x%8x to user space.",baseAddr_daq));
    }
  }
}


void PolarfireWB::stop() {
  if ( threadEn_ ) {
    rogue::GilRelease noGil;
    threadEn_ = false;
    queue_.stop();
    thread_->join();
    munmap((void *)base_,MAP_SIZE);
    if (base_daq_) munmap((void*)base_daq_,MAPDAQ_SIZE);
    if (base_fc_) munmap((void*)base_fc_,MAP_SIZE);
    ::close(fd_);
  }
}

PolarfireWB::~PolarfireWB() {
  this->stop();
}

void PolarfireWB::doTransaction(rogue::interfaces::memory::TransactionPtr tran) {
  rogue::GilRelease noGil;
  queue_.push(tran);
}

static const int REG_CTL                            = 1;
static const uint32_t START                         = 0x1;
static const uint32_t RECOVER                       = 0x2;
static const int REG_ADDR_DIR                       = 2;
static const uint32_t MASK_ADDR                     = 0x3FFFF;
static const uint32_t SHIFT_ADDR                    = 0;
static const uint32_t MASK_TARGET                   = 0x1F;
static const uint32_t SHIFT_TARGET                  = 20;
static const uint32_t DIR_IS_WRITE                  = 0x10000000;
static const uint32_t WB_RESET                      = 0x20000000;

static const int REG_DATO                           = 3;

static const int REG_DATI                           = 64+1;
static const int REG_STATUS                         = 64;
static const uint32_t WB_DONE                       = 0x2;
static const int target_Backend_FC                  = 257;
static const int target_Backend_DAQ                 = 258;

namespace rim = rogue::interfaces::memory;

void PolarfireWB::runThread() {
  rim::TransactionPtr        tran;

  log_->logThreadId();

  while(threadEn_) {
    if ((tran = queue_.pop()) != NULL) {
      rim::TransactionLockPtr lock = tran->lock();

      if ( tran->expired() ) {
        log_->warning("Transaction expired. Id=%i",tran->id());
        tran.reset();
        continue;
      }

      if ( (tran->size() % 4) != 0 ) {
        tran->error("Invalid transaction size %i, must be an integer number of 4 bytes",tran->size());
        tran.reset();
        continue;
      }

      log_->debug("Transaction correct size and not expired");

      /// gather information
      uint32_t addr=tran->address()&MASK_ADDR;
      uint32_t target=(tran->address()>>32);
      bool isWrite= ( tran->type() == rogue::interfaces::memory::Write ||
                      tran->type() == rogue::interfaces::memory::Post );
      uint32_t* p_buffer=(uint32_t*)tran->begin();
      int naddr=tran->size()/4;

      log_->debug("Transaction { addr : %i target: %i isWrite: %d naddr: %d buff: 0x%08x }",
          addr,target,isWrite,naddr,p_buffer);

      if (target==target_Backend_FC) {
        log_->debug("Backend FC Transaction");
        if (base_fc_==0) {
          tran->error("Backend fast control not mapped");
        } else {
          log_->debug("Backed FC mapped");
          for (int iw=0; iw<naddr; iw++) {
            if (isWrite) base_fc_[0x100+iw+addr]=p_buffer[iw];
            else p_buffer[iw]=base_fc_[0x100+iw+addr];
          }
          tran->done();
        }
        continue;
      } else if (target==target_Backend_DAQ) {
        log_->debug("Backend DAQ transaction");
        if (base_daq_==0) {
          tran->error("Backend DAQ not mapped");
        } else {
          log_->debug("Backed DAQ mapped");
          for (int iw=0; iw<naddr; iw++) {
            if (isWrite) base_daq_[iw+addr]=p_buffer[iw];
            else p_buffer[iw]=base_daq_[iw+addr];
          }
          tran->done();
        }
        continue; 
      }

      target&=MASK_TARGET;

      uint32_t common_reg2;
      common_reg2=(target<<SHIFT_TARGET);
      if (isWrite) common_reg2|=DIR_IS_WRITE;

      log_->debug("common_reg2 = 0x%08x", common_reg2);

      bool failed=false;
      for (int iw=0; iw<naddr; iw++) {
        // set address, target, direction
        base_[REG_ADDR_DIR]=((addr+iw)&MASK_ADDR) | common_reg2;
        log_->debug("Set REG_ADDR_DIR 0x%08x", base_[REG_ADDR_DIR]);
        // if appropriate, set data
        if (isWrite) {
          base_[REG_DATO]=p_buffer[iw];
          log_->debug("Set REG_DATAO 0x%x", base_[REG_DATO]);
        }
        // start the transaction
        log_->debug("Set base_[REG_CTL] to 0x%x", START);
        base_[REG_CTL]=START;
        log_->debug("Signal start (base_[REG_CTL] == 0x%x)", base_[REG_CTL]);
        log_->debug("begin spin wait with status 0x%x", base_[REG_STATUS]);
        int ispin=0;
        static const int TIMEOUT=100000;
        bool done=false;
        // spin-wait
        do {
          ispin=ispin+1;
          done=(base_[REG_STATUS]&WB_DONE)!=0;
        } while (!done && ispin<TIMEOUT);
        log_->debug("completed spin wait with status 0x%x", base_[REG_STATUS]);
        if (!done) {
          // clear the WB transactor
          log_->error("unable to complete transaction within alloted timeout %d", ispin);
          log_->debug("attempting to recover (set REG_CTL = 0x%x)", RECOVER);
          base_[REG_CTL]=RECOVER;
          log_->debug("recovering with base_[REG_CTL] == 0x%x", base_[REG_CTL]);
          failed=true;
          // report failure
          log_->debug("reporting failure to transaction");
          tran->error("Wishbone bus timeout");
          log_->debug("leaving naddr loop");
          break;
        } else {
          log_->debug("successful transation");
          if (!isWrite) p_buffer[iw]=base_[REG_DATI];
        }
      }
      if (!failed) {
        log_->debug("Transaction id=0x%08x, addr 0x%08x. Size=%i, type=%i",
            tran->id(),tran->address(),tran->size(),tran->type());
        tran->done();
      }
      log_->debug("done with transaction");
    }
  }
}
