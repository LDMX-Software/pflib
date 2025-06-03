#include "pflib/DecodeAndWrite.h"

#include "pflib/packing/BufferReader.h"
#include "pflib/Exception.h"

namespace pflib {

void DecodeAndWrite::consume(std::vector<uint32_t>& event) {
  // we have to manually check the size so that we can do the reinterpret_cast
  if (event.size() == 0) {
    pflib_log(warn) << "event with zero words passed in, skipping";
    return;
  }
  // reinterpret the 32-bit words into a vector of bytes which is
  // what is consummed by the BufferReader
  const auto& buffer{*reinterpret_cast<const std::vector<uint8_t>*>(&event)};
  pflib::packing::BufferReader r{buffer};
  r >> ep_;
  write_event(ep_);
}

DecodeAndWriteToCSV::DecodeAndWriteToCSV(
    const std::string& file_name,
    std::function<void(std::ofstream&)> write_header,
    std::function<void(std::ofstream& f, const pflib::packing::SingleROCEventPacket&)> write_event
) : DecodeAndWrite(), file_{file_name}, write_event_{write_event} {
  if (not file_) {
    PFEXCEPTION_RAISE("FileOpen", "unable to open "+file_name+" for writing");
  }
  write_header(file_);
}

void DecodeAndWriteToCSV::write_event(const pflib::packing::SingleROCEventPacket& ep) {
  write_event_(file_, ep);
}

static void all_channels_header(std::ofstream& f) {
  f << std::boolalpha;
  f << "link,bx,event,orbit,channel,Tp,Tc,adc_tm1,adc,tot,toa\n";
}

static void all_channels_write_event(std::ofstream& f, const pflib::packing::SingleROCEventPacket& ep) {
  ep.to_csv(f);
}

DecodeAndWriteToCSV all_channels_to_csv(const std::string& file_name) {
  return DecodeAndWriteToCSV(file_name, all_channels_header, all_channels_write_event);
}

}
