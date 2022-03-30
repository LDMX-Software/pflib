#ifndef POLARFIREWB_H_
#define POLARFIREWB_H_ 1

#include <rogue/interfaces/memory/Constants.h>
#include <rogue/interfaces/memory/Slave.h>
#include <rogue/interfaces/memory/Transaction.h>
#include <stdint.h>
#include <thread>
#include <mutex>
#include <memory>
#include <stdint.h>
#include <rogue/Logging.h>
#include <rogue/Queue.h>

class PolarfireWB : public rogue::interfaces::memory::Slave {
public:
  
  // Create a static class creator to return our custom class
  // wrapped with a shared pointer
  static std::shared_ptr<PolarfireWB> create(uint64_t baseAddress) {
    static std::shared_ptr<PolarfireWB> ret =
      std::make_shared<PolarfireWB>(baseAddress);
    return(ret);
  }
  
  // Standard class creator which is called by create
  // The baseAddr is used to memory-map the necessary page for communicating with
  // the Wishbone bridge firmware
  PolarfireWB(uint64_t baseAddr);

  // map the fast control page
  void map_fc(uint64_t baseAddr_fc);
  // map the DAQ pages
  void map_daq(uint64_t baseAddr_daq);
  
  // close and cleanup
  virtual ~PolarfireWB();
  
  // Entry point for incoming transaction
  virtual void doTransaction(rogue::interfaces::memory::TransactionPtr tran);

  // stop the interface
  void stop();
  
private:
  // file descriptor used for mem-map
  int fd_;
  // base pointer for mem-map
  uint32_t* base_;
  uint32_t* base_fc_;
  uint32_t* base_daq_;

  // Is the thread running?
  bool threadEn_;

  // Logging
  std::shared_ptr<rogue::Logging> log_;
  
  std::thread* thread_;
  // Queue
  rogue::Queue<std::shared_ptr<rogue::interfaces::memory::Transaction>> queue_;
  void runThread();
  

};

typedef std::shared_ptr<PolarfireWB> PolarfireWBPtr;

#endif // 
