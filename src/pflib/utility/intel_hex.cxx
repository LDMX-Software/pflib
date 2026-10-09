#include "pflib/utility/intel_hex.h"

#include "pflib/logging/Logging.h"

namespace pflib::utility::intel_hex {

static ::pflib::logging::logger the_log_{::pflib::logging::get("intel_hex")};

void DataRecord::from_bytes(uint16_t addr, std::vector<uint8_t> data) {
  addr_ = addr;
  data_ = data;
}

const std::vector<uint8_t>& DataRecord::get_bytes() const {
  return data_;
}

const uint16_t& DataRecord::addr() const {
  return addr_;
}

static const char* EOF_LINE = ":00000001FF";

Reader::Reader(const std::string& input_filepath)
  : input_file_{input_filepath} {}

bool Reader::next() {
  // TODO: skip comments
  std::string line;
  std::getline(input_file_, line);

  if (line == EOF_LINE) return false;

  std::vector<uint8_t> bytes;
  int sum{0};
  for (std::size_t i_char{1}; i_char < line.length(); i_char += 2) {
    std::string byte_str = line.substr(i_char, 2);
    uint8_t byte = static_cast<uint8_t>(strtol(byte_str.c_str(), NULL, 16));
    sum += byte;
    bytes.push_back(byte);
  }

  if (sum & 0xff != 0x00) {
    pflib_log(warn) << "reader: bad checksum " << std::hex << (sum & 0xff)
                    << " (should be zero)";
  }
  if (bytes[0] != bytes.size() - 5) {
    pflib_log(warn) << "reader: bad length, report: " << bytes[0]
                    << " actual: " << bytes.size()
                    << " (reported should be 5 less than actual)";
  }

  std::vector<uint8_t> data{bytes.begin() + 4, bytes.end() - 1};
  current_record_.from_bytes(
      static_cast<uint16_t>((bytes[1] << 8) | bytes[2]),
      data
  );
  return true;
}

const DataRecord& Reader::get() {
  return current_record_;
}
 
Writer::Writer(const std::string& output_filepath)
  : output_file_{fopen(output_filepath.c_str(), "w")} {
    if (output_file_ == nullptr) {
      pflib_log(error) << "unable to open output intel_hex file " << output_filepath;
    }
}

Writer::~Writer() {
  close();
}

void Writer::add(const DataRecord& data) {
  if (!output_file_) return;
  // all of the bytes except the checksum
  std::vector<uint8_t> byte_row(data.get_bytes().size() + 4);
  byte_row[0] = data.get_bytes().size();
  byte_row[1] = ((data.addr() >> 8) & 0xff);
  byte_row[2] = ((data.addr() >> 0) & 0xff);
  byte_row[3] = 0x01;
  for (int i_byte{0}; i_byte < data.get_bytes().size(); i_byte++) {
    byte_row[4+i_byte] = data.get_bytes().at(i_byte);
  }
  fprintf(output_file_, ":");
  uint8_t checksum = 0;
  for (auto byte : byte_row) {
    fprintf(output_file_, "%02x", byte);
    checksum += byte;
  }
  checksum = static_cast<uint8_t>(-static_cast<unsigned int>(checksum));
  fprintf(output_file_, "%02x\n", checksum);
}

void Writer::close() {
  fprintf(output_file_, "%s\n", EOF_LINE);
  if (output_file_) fclose(output_file_);
}

}
