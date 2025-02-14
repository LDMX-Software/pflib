#pragma once

#include <boost/interprocess/sync/named_mutex.hpp>
#include <boost/thread/thread.hpp> 

#include <uhal/uhal.hpp>
#include "uhal/SigBusGuard.hpp"

typedef std::shared_ptr<uhal::HwInterface> UHALHwInterfacePTR;
namespace bip = boost::interprocess;
typedef std::unique_ptr<bip::named_mutex> MutexPtr;

class HWInterface
{
public:
  HWInterface( )
  {;}

  HWInterface( const std::string mutexName )
  {
    mMutex = std::make_unique<bip::named_mutex>(bip::open_or_create, mutexName.c_str());
  }

  HWInterface( const std::string connection_file,
	       const std::string device,
	       const std::string mutexName )
  {
    uhal::setLogLevelTo(uhal::Warning());
    uhal::ConnectionManager manager( connection_file );
    uhal::SigBusGuard::blockSIGBUS();
    mUhal = std::make_shared<uhal::HwInterface>( manager.getDevice(device) );
    mMutex = std::make_unique<bip::named_mutex>(bip::open_or_create, mutexName.c_str());
  }

  void lock()
  {
    mMutex->lock();
  }
  
  void unlock() 
  {
    mMutex->unlock();
  }
    
protected:
  UHALHwInterfacePTR mUhal;
  MutexPtr mMutex;
};
