#include "ic_ec_with_sct.hpp"
#include "sct_tx_encoder.hpp"
#include "sct_rx_decoder.hpp"
#include <sstream>
#include <math.h>

#define PAYLOAD_LENGTH 8 //in bytes !
#define TRANSACTION_LENGTH 4 //in 32-bits word !

#include <sys/utsname.h>

ICECWithSCT::ICECWithSCT( const std::string& name, const YAML::Node& config ) : Transport(name,config)
{
  std::string connection_file = std::string("file:///opt/cms-hgcal-firmware/hgc-test-systems/active/uHAL_xml/connections.xml");
  if( config["connection_file"].IsDefined() )
    connection_file = config["connection_file"].as<std::string>();
  
  std::string device = std::string("TOP");
  if( config["device"].IsDefined() )
    device = config["device"].as<std::string>();

  struct utsname sysinfo;
  uname(&sysinfo);
  std::ostringstream os(std::ostringstream::ate);
  os.str("");
  os << sysinfo.nodename << ":transactor0";  
  m_sct_interface = SCTInterface(connection_file,device,os.str());

  m_broadcast_address = 0;
  if( config["broadcast_address"].IsDefined() )
    m_broadcast_address = config["broadcast_address"].as<unsigned int>();
  
  m_reply_address = std::log2(m_broadcast_address & -m_broadcast_address);

  spdlog::apply_all([&](LoggerPtr l) {l->debug( "{} initialized with broadcast_address and reply_address = ({},{}) ",m_name,m_broadcast_address,m_reply_address  );});
}

void ICECWithSCT::write_regs(const unsigned int addr, const unsigned int reg_addr_width, const unsigned int reg_addr, const std::vector<unsigned int>& reg_vals)
{
  auto n_regs = reg_vals.size();
  auto n_transactions = (unsigned int)(std::ceil(n_regs/(float)PAYLOAD_LENGTH)); // up to 8 registers per transaction
  int read_write=0;
  ic_tx_frame base_frame( m_broadcast_address,  m_reply_address, addr, read_write, reg_addr, 0x0, 0x0 ); //we update reg_addr, nreg and payload in the following lines

  
  std::vector<ic_tx_frame> ic_frames(n_transactions,base_frame);
  unsigned int global_index=0;
  std::transform( ic_frames.begin(), ic_frames.end(), ic_frames.begin(),
		  [&](auto &frame){
		    uint64_t payload=0;
		    unsigned int local_index=0;
		    auto first = reg_vals.cbegin()+global_index*PAYLOAD_LENGTH;
		    auto last  = (global_index+1)*PAYLOAD_LENGTH>reg_vals.size() ? reg_vals.end() : reg_vals.cbegin()+(global_index+1)*PAYLOAD_LENGTH;
		    std::for_each(first, last,
				  [&payload,&local_index](const auto& val){
				    payload |= (uint64_t(val&0xFF))<<(PAYLOAD_LENGTH*local_index);
				    local_index++;
				  });
		    frame.n_reg = local_index;
		    frame.reg_address += global_index*PAYLOAD_LENGTH;
		    frame.payload = payload;
		    global_index++;
		    return frame;
		  });
  std::vector<uint32_t> encodedData;
  encodedData.reserve(4*n_transactions);
  std::for_each( ic_frames.begin(), ic_frames.end(),
		 [&](const auto& frame){
		   auto vec = sct_tx_encoder::encode(frame);
		   encodedData.insert( encodedData.end(), vec.begin(), vec.end() );		   
		   spdlog::apply_all([&](LoggerPtr l) {
				       l->trace( "{} writing IC/EC 0x{:08x} 0x{:08x} 0x{:08x} 0x{:08x}",m_name,
					       vec[0],vec[1],vec[2],vec[3] );
				     });
		 } );
  assert( encodedData.size()==4*n_transactions );

  /*
  // Debugging block
  for(unsigned int index=0; index<n_transactions; index++){
    std::vector<uint32_t> encoded( encodedData.begin()+index*TRANSACTION_LENGTH, encodedData.begin()+(index+1)*TRANSACTION_LENGTH );
    spdlog::apply_all([&](LoggerPtr l) {
			l->trace( "{} IC/EC read 0x{:08x} 0x{:08x} 0x{:08x} 0x{:08x}",m_name,
				 encoded[0],encoded[1],encoded[2],encoded[3] );
		      });
    auto frame = sct_tx_encoder::decode(encoded);
    std::cout << frame << std::endl;
  } 
  */ 
  m_sct_interface.run_transactions( encodedData );
}

void ICECWithSCT::write_regs(const unsigned int addr, const unsigned int reg_addr_width, const std::vector<unsigned int>& reg_addr_vec, const std::vector<std::vector<unsigned int>>& reg_vals_vec)
{
  for(size_t i(0); i<reg_addr_vec.size(); i++)
    write_regs(addr, reg_addr_width, reg_addr_vec[i], reg_vals_vec[i]);
}


void ICECWithSCT::write_reg(const unsigned int addr, const unsigned int reg_val)
{
  spdlog::apply_all([&](LoggerPtr l) { l->critical("IC/EC SCT: write_reg is not implemented, please use write_regs methods instead"); });
}
  
unsigned int ICECWithSCT::read_reg(const unsigned int addr)
{
  spdlog::apply_all([&](LoggerPtr l) { l->critical("IC/EC SCT: read_reg is not implemented, please use read_regs methods instead"); });
  return 0;
}
  
std::vector<unsigned int> ICECWithSCT::read_regs(const unsigned int addr, const unsigned int reg_addr_width, const unsigned int reg_addr, const unsigned int read_len)
{
  auto n_transactions = (unsigned int)(std::ceil(read_len/(float)PAYLOAD_LENGTH)); // up to 8 registers per transaction
  int read_write=1;
  ic_tx_frame base_frame( m_broadcast_address,  m_reply_address, addr, read_write, reg_addr, 0x0, 0x0 ); //we update reg_addr, nreg in the following lines

  std::vector<ic_tx_frame> ic_frames(n_transactions,base_frame);
  unsigned int global_index=0;
  auto remaining_number_of_bytes = read_len;
  std::transform( ic_frames.begin(), ic_frames.end(), ic_frames.begin(),
		  [&](auto &frame){
		    frame.n_reg = (global_index+1)<n_transactions ? PAYLOAD_LENGTH : remaining_number_of_bytes;
		    remaining_number_of_bytes-=frame.n_reg;
		    frame.reg_address += global_index*PAYLOAD_LENGTH;
		    global_index++;
		    return frame;
		  });
  std::vector<uint32_t> encodedData;
  encodedData.reserve(4*n_transactions);
  std::for_each( ic_frames.begin(), ic_frames.end(),
		 [&](const auto& frame){
		   auto vec = sct_tx_encoder::encode(frame);
		   encodedData.insert( encodedData.end(), vec.begin(), vec.end() );		   
		   spdlog::apply_all([&](LoggerPtr l) {
				       l->trace( "{} reading IC/EC 0x{:08x} 0x{:08x} 0x{:08x} 0x{:08x}",m_name,
					       vec[0],vec[1],vec[2],vec[3] );
				     });
		 } );
  assert( encodedData.size()==4*n_transactions );
  auto encodedResponse = m_sct_interface.run_transactions_and_get_replies( encodedData );
  assert( encodedResponse.size()==n_transactions*4 );

  std::vector<unsigned int> response(read_len);

  remaining_number_of_bytes = read_len;
  for(unsigned int index=0; index<n_transactions; index++){
    std::vector<uint32_t> encoded( encodedResponse.begin()+index*TRANSACTION_LENGTH, encodedResponse.begin()+(index+1)*TRANSACTION_LENGTH );
    spdlog::apply_all([&](LoggerPtr l) {
  			l->trace( "{} IC/EC read 0x{:08x} 0x{:08x} 0x{:08x} 0x{:08x}",m_name,
  				 encoded[0],encoded[1],encoded[2],encoded[3] );
  		      });
    auto frame = sct_rx_decoder::decode(encoded);
    auto nbytes = (index+1)<n_transactions ? PAYLOAD_LENGTH : remaining_number_of_bytes;
    remaining_number_of_bytes-=nbytes;
    auto byte_index=0;
    std::transform( response.begin()+index*PAYLOAD_LENGTH,
  		    response.begin()+index*PAYLOAD_LENGTH+nbytes,
  		    response.begin()+index*PAYLOAD_LENGTH,
  		    [&frame,&byte_index](auto &val){
  		      unsigned int newval = (frame.payload>>8*byte_index)&0xFF;
  		      byte_index++;
  		      return newval;
  		    });
  }
 
  return response;
}

std::vector<unsigned int> ICECWithSCT::read_regs(const unsigned int addr, const unsigned int reg_addr_width, const std::vector<unsigned int>& reg_addr_vec, const std::vector<unsigned int>& read_len_vec)
{
  auto errorMsg = "IC/EC SCT: read_regs(const unsigned int, const unsigned int, const std::vector<unsigned int>&, const std::vector<unsigned int>&) is not implemented, please use read_regs(const uint,const uint,const uint,const uint) methods instead";
  spdlog::apply_all([&](LoggerPtr l) { l->critical(errorMsg); });
  throw swamp::MethodNotImplementedException(errorMsg);
  return std::vector<unsigned int>(0);
}

void ICECWithSCT::LockImpl()
{
  m_sct_interface.lock();
}

void ICECWithSCT::UnLockImpl()
{
  m_sct_interface.unlock();
}

void ICECWithSCT::ResetImpl()
{
  m_sct_interface.ResetSlowControl();
}

void ICECWithSCT::ConfigureImpl(const YAML::Node& node)
{
}
