#pragma once

#include <hw_interface.hpp>

class SCTInterface : public HWInterface
{
public:
  SCTInterface()
  {;}
  
  SCTInterface( const std::string connection_file,
		const std::string device,
		const std::string mutexName ) : HWInterface(connection_file,device,mutexName)
  {
    ResetSlowControl();
  }

  void ResetSlowControl()
  {
    mUhal->getNode("slow_control_config.ResetN.rstn").write(0x0);
    mUhal->dispatch();
    mUhal->getNode("slow_control_config.ResetN.rstn").write(0x1);
    mUhal->dispatch();
  }

  void run_transactions(const std::vector<uint32_t>& data)
  {
    send_transactions(data);
  }

  std::vector<uint32_t> run_transactions_and_get_replies(const std::vector<uint32_t>& data)
  {
    send_transactions(data);
    return get_transaction_replies(int(data.size()/4));
  }

  void send_transactions(const std::vector<uint32_t>& data)
  {
    assert( data.size()%4 == 0 );

    auto nbr_transactions = int(data.size()/4);

    mUhal->getNode("slow_control_data.IC_TX_BRAM0.Data").writeBlock(data);
    mUhal->getNode("slow_control_config.IC_Control0.NbrTransactions").write(nbr_transactions);
    mUhal->dispatch();
    mUhal->getNode("slow_control_config.IC_Control0.Start").write(0x1);
    mUhal->dispatch();
    mUhal->getNode("slow_control_config.IC_Control0.Start").write(0x0);
    mUhal->dispatch();
  }

  const std::vector<uint32_t> get_transaction_replies(unsigned int nbr_transactions)
  {
    while(1){
      auto busy = mUhal->getNode("slow_control_config.IC_Status0.Busy").read();
      mUhal->dispatch();
      if( busy == 0 )
	break;
    }
    auto valvec = mUhal->getNode("slow_control_data.IC_RX_BRAM0.Data").readBlock(nbr_transactions*4);
    mUhal->dispatch();
    std::vector<uint32_t> data(nbr_transactions*4);
    std::copy(valvec.begin(),valvec.end(),data.begin());
    return data;
  }
  
};
