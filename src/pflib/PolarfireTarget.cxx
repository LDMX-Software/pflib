#include "pflib/PolarfireTarget.h"

#include "pflib/ROC.h"
#include "pflib/Elinks.h"
#include "pflib/Bias.h"
#include "pflib/FastControl.h"
#include "pflib/Compile.h"

#include <iostream>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <unistd.h>

#include <yaml-cpp/yaml.h>

namespace pflib {

/**
 * Modern RAII-styled CSV extractor taken from
 * https://stackoverflow.com/a/1120224/17617632
 * this allows us to **discard white space within the cells**
 * making the CSV more human readable.
 */
static std::vector<int> getNextLineAndExtractValues(std::istream& ss) {
  std::vector<int> result;

  std::string line, cell;
  std::getline(ss, line);

  if (line.empty() or line[0] == '#') {
    // empty line or comment, return empty container
    return result;
  }

  std::stringstream line_stream(line);
  while (std::getline(line_stream, cell, ',')) {
    /**
     * std stoi has a auto-detect base feature
     * https://en.cppreference.com/w/cpp/string/basic_string/stol
     * which we can enable by setting the pre-defined base to 0
     * (the third parameter) - this auto-detect base feature
     * can handle hexidecial (prefix == 0x or 0X), octal (prefix == 0),
     * and decimal (no prefix).
     *
     * The second parameter is an address to put the number of characters processed,
     * which I disregard at this time.
     *
     * Do we allow empty cells?
     */
    result.push_back(cell.empty() ? 0 : std::stoi(cell,nullptr,0));
  }
  // checks for a trailing comma with no data after it
  if (!line_stream and cell.empty()) {
    // trailing comma, put in one more 0
    result.push_back(0);
  }

  return result;
}

/**
 * Check if a given string has a specific ending
 */
static bool endsWith(const std::string& full, const std::string& ending) {
  if (full.length() < ending.length()) return false;
  return (0 == full.compare(full.length()-ending.length(), ending.length(), ending));
}

const int PolarfireTarget::N_PAGES    = 300;
const int PolarfireTarget::N_REGISTERS_PER_PAGE = 16;
int       PolarfireTarget::NLINKS     = 8 ;

PolarfireTarget::PolarfireTarget(WishboneInterface* wbi, Backend *be, bool same)
  : wb_be_same{same}, wb{wbi}, backend{be}, hcal{wbi} {}

PolarfireTarget::~PolarfireTarget() {
  if (wb_be_same) {
    // same object, only delete one
    if (wb != nullptr) delete wb;
  } else {
    // different objects, delete both
    if (wb != nullptr) delete wb;
    if (backend != nullptr) delete backend;
  }
  // both are nullptr -> do nothing
}

std::pair<int,int> PolarfireTarget::getFirmwareVersion() {
  uint32_t w = hcal.getFirmwareVersion();
  return { w >> 8 , w & 0xFF };
}

std::vector<uint32_t> PolarfireTarget::elinksBigSpy(int ilink, int presamples, bool do_l1a) {
  hcal.elinks().setupBigspy(do_l1a, ilink, presamples);
  if (do_l1a) {
    backend->fc_sendL1A();
  }
  return hcal.elinks().bigspy();
}

void PolarfireTarget::elinkStatus(std::ostream& os) {
  std::vector<uint32_t> status;
  for (int i=0; i<NLINKS; i++) 
    status.push_back(hcal.elinks().getStatusRaw(i));

  os << std::setw(20) << "LINK";
  for (int i{0}; i < NLINKS; i++) 
    os << std::setw(4) << i << " ";
  os << "\n";

  auto print_row = [&](const std::string& name, int shift, int mask) {
    os << std::setw(20) << name;
    for (int i{0}; i < NLINKS; i++)
      os << std::setw(4) << ((status[i] >> shift) & mask) << " ";
    os << "\n";
  };

  print_row("DLY_RANGE", 0, 0x1);
  print_row("EYE_EARLY", 1, 0x1);
  print_row("EYE_LATE" , 2, 0x1);
  print_row("IS_IDLE"  , 3, 0x1);
  print_row("IS_ALIGNED", 4, 0x1);

  /*
  os << std::setw(20) << "AUTO_LOCKED";
  for (int i{0}; i < NLINKS; i++) {
    os << std::setw(4);
    if (not hcal.elinks().isBitslipAuto(i)) os << "X";
    else os << ((status[i]>>5)&0x1);
    os << " ";
  }
  os << "\n";
  */
  //  print_row("AUTO_PHASE", 8, 0x7);
  print_row("BAD_COUNT" , 16, 0xFFF);
  //  print_row("AUTO_COUNT", 28, 0xF);
}

void PolarfireTarget::loadIntegerCSV(const std::string& file_name, 
    const std::function<void(const std::vector<int>&)>& Action) {
  if (file_name.empty()) {
    PFEXCEPTION_RAISE("Filename", "No file provided to load CSV function.");
  }
  std::ifstream f{file_name};
  if (not f.is_open()) {
    PFEXCEPTION_RAISE("File", "Unable to open "+file_name+" in load CSV function.");
  }
  while (f) {
    auto cells = getNextLineAndExtractValues(f);
    // skip comments and blank lines
    if (cells.empty()) continue;
    Action(cells);
  }
}

void PolarfireTarget::loadROCRegisters(int roc, const std::string& file_name) {
  loadIntegerCSV(file_name,[&](const std::vector<int>& cells) {
      if (cells.size() == 3) 
        hcal.roc(roc).setValue(cells.at(0),cells.at(1),cells.at(2));
      else {
        std::cout << 
          "WARNING: Ignoring ROC CSV settings line"
          "without exactly three columns." 
          << std::endl;
      }
    });
}

void PolarfireTarget::loadROCParameters(int roc, const std::string& file_name, bool prepend_defaults) {
  if (prepend_defaults) {
    // compile settings with PDF defaults added at bottom,
    // ==> sets ALL parameters on chip
    auto settings = compile(file_name, true);
    hcal.roc(roc).setRegisters(settings);
  } else {
    // only change parameters that YAML file contains
    std::map<std::string,std::map<std::string,int>> parameters;
    extract(std::vector<std::string>{file_name}, parameters);
    hcal.roc(roc).applyParameters(parameters);
  }
}

void PolarfireTarget::dumpSettings(int roc, const std::string& filename, bool should_decompile) {
  if (filename.empty()) {
    PFEXCEPTION_RAISE("Filename", "No filename provided to dump roc settings.");
  }
  std::ofstream f{filename};
  if (not f.is_open()) {
    PFEXCEPTION_RAISE("File", "Unable to open file "+filename+" in dump roc settings.");
  }

  auto roc_obj{hcal.roc(roc)};

  if (should_decompile) {
    // read all the pages and store them in memory
    std::map<int,std::map<int,uint8_t>> register_values;
    for (int page{0}; page<N_PAGES; page++) {
      // all pages have up to 16 registers
      std::vector<uint8_t> v = roc_obj.readPage(page,N_REGISTERS_PER_PAGE);
      for (int reg{0}; reg < N_REGISTERS_PER_PAGE; reg++) {
        register_values[page][reg] = v.at(reg);
      }
    }

    // decompile while being careful
    std::map<std::string,std::map<std::string,int>>
      parameter_values = decompile(register_values,true);

    YAML::Emitter out;
    out << YAML::BeginMap;
    for (const auto& page : parameter_values) {
      out << YAML::Key << page.first;
      out << YAML::Value << YAML::BeginMap;
      for (const auto& param : page.second) {
        out << YAML::Key << param.first << YAML::Value << param.second;
      }
      out << YAML::EndMap;
    }
    out << YAML::EndMap;

    f << out.c_str();
  } else {
    // read all the pages and write to CSV while reading
    std::map<int,std::map<int,uint8_t>> register_values;
    for (int page{0}; page<N_PAGES; page++) {
      // all pages have up to 16 registers
      std::vector<uint8_t> v = roc_obj.readPage(page,N_REGISTERS_PER_PAGE);
      for (int reg{0}; reg < N_REGISTERS_PER_PAGE; reg++) {
        f << page << "," 
          << reg << ","
          << std::hex << std::setfill('0') << std::setw(2)
          << static_cast<int>(v.at(reg)) << std::dec
          << '\n';
      }
    }
  }

  f.flush();
}

void PolarfireTarget::prepareNewRun() {
  auto& daq = hcal.daq();
  backend->fc_bufferclear();
  backend->daq_reset();
  backend->fc_clear_run();

  bool enable;
  int extra_samples;
  hcal.fc().getMultisampleSetup(enable,extra_samples);
  samples_per_event_=extra_samples+1;

  
  daq.enable(true);
}

void PolarfireTarget::daqStatus(std::ostream& os) {
  bool full, empty;
  int events, asize;
  uint32_t reg1, reg2;
  
  os << "-----Front-end FIFO-----\n";
  reg1 = wb->wb_read(tgt_DAQ_Control,4);
  os << " Header occupancy : " << ((reg1 >> 8) & 0x3f)
    << "  Maximum occupancy : " << (reg1 & 0x3f)
    <<" \n";
  reg1 = wb->wb_read(tgt_DAQ_Control,5);
  reg2 = wb->wb_read(tgt_DAQ_Control,2);
  os << " Next event info: 0x"
     << std::hex << std::setw(8) << std::setfill('0') << reg1 << std::dec
     << " (BX=" << (reg1&0xfff)
     << ", RREQ=" << ((reg1>>12)&0x3ff)
     << ", OR=" << ((reg1>>22)&0x3ff)
     << ")  RREQ=" << ((reg2>>22)&0x3ff)
     << "\n";
  os << "-----Per-ROCLINK processing-----\n";
  os << " Link  ID  EN ZS FL EM\n";
  for (int ilink=0; ilink < NLINKS; ilink++) {
    reg1=wb->wb_read(tgt_DAQ_LinkFmt,(ilink<<7)|1);
    reg2=wb->wb_read(tgt_DAQ_LinkFmt,(ilink<<7)|2);
    os << " " << std::setw(4) << ilink
       << " " << std::hex << std::setw(4) << std::setfill('0') << reg2 << std::dec
       << " " << std::setw(2) << ((reg1>>0)&1)
       << " " << std::setw(2) << ((reg1>>1)&1)
       << " " << std::setw(2) << ((reg1>>22)&1)
       << " " << std::setw(2) << ((reg1>>23)&1)
       << " " << std::setw(8) << std::setfill('0') << reg1 << std::dec
       << "\n";
  }
  
  os << "-----Off-detector FIFO-----\n";
  backend->daq_status(full, empty, events,asize);
  os << " " << std::setfill(' ') << std::setw(8) << (full ? "FULL" : "NOT-FULL")
     << " " << std::setw(9) << (empty ? "EMPTY" : "NOT_EMPTY")
     << " Events ready: " << std::setw(3) << events
     << " Next event size: " << asize
     << "\n";
}

void PolarfireTarget::enableZeroSuppression(int link, bool full_suppress) {
  for (int i{0}; i < NLINKS; i++) {
    if (i != link and link > 0) continue;
    uint32_t reg = wb->wb_read(tgt_DAQ_LinkFmt,(i<<7)|1);
    bool wasZS = (reg & 0x2)!=0;
    if (link >= 0 and not wasZS) {
      reg |= 0x4;
      if (not full_suppress) reg ^= 0x4;
    }
    wb->wb_write(tgt_DAQ_LinkFmt,(i<<7)|1,reg^0x2);
  }
}

void PolarfireTarget::daqSoftReset() {
  printf("..Halting the event builder\n");
  for (int ilink=0; ilink<NLINKS; ilink++) {
    uint32_t reg1=wb->wb_read(tgt_DAQ_LinkFmt,(ilink<<7)|1);
    wb->wb_write(tgt_DAQ_LinkFmt,(ilink<<7)|1,(reg1&0xfffffffeu)|0x2);
  }
  printf("..Sending a buffer clear\n");
  backend->fc_bufferclear();
  printf("..Sending DAQ reset\n");
  backend->daq_reset();
}

void PolarfireTarget::daqHardReset() {
  printf("..HARD reset\n");
  hcal.daq().reset();
  backend->daq_reset();
}

std::vector<uint32_t> PolarfireTarget::daqReadDirect() {
  bool full, empty;
  int events, esize;
  backend->daq_status(full, empty, events, esize);
  std::vector<uint32_t> buffer;
  buffer = backend->daq_read_event();
  if (!empty) backend->daq_advance_ptr();
  return buffer;
}

std::vector<uint32_t> PolarfireTarget::daqReadEvent() {
  // listen until we actually get data
  bool full, empty;
  int events, esize;
  do {
    backend->daq_status(full, empty, events, esize);
  } while (empty);

  // initialize event buffer with header words
  //    the non-signal header words are not set 
  //    until _after_ we get our data because
  //    we need to know the lengths
  std::vector<uint32_t> event = { 0x11111111u, 0xBEEF2021u, };
  // n_samples = 1 + extra_samples
  // n sample length words = n_samples/2 rounded up = (n_samples+1)/2
  std::size_t header_len = 1 + (samples_per_event_+1)/2;
  // add unset header words into event, set them after read
  event.resize(event.size() + header_len);

  std::vector<std::size_t> lens;
  std::size_t totlen = header_len;
  unsigned int fpgaid = 0;
  
  for (int i=0; i<(samples_per_event_); i++) {
    std::vector<uint32_t> buffer = backend->daq_read_event();
    if (!empty) backend->daq_advance_ptr();
    lens.push_back(buffer.size());
    // add to the event
    event.insert(event.end(),buffer.begin(),buffer.end());
    // get fpga ID on first event
    if (i==0) fpgaid = (buffer[0]>>(12+2+6))&0xFF;        
    totlen += buffer.size();
  }

  // index of unset header words
  std::size_t i_header = 2;
  uint32_t val=(0x1<<28); // version
  val|=(fpgaid&0xFF)<<20;
  val|=(lens.size())<<16;
  val|=totlen;
  event[i_header++] = val;
  val=0;
  // now the various subpackets
  for (int i=0; i<int(lens.size()); i++) {
    val|=(lens[i])<<(16*(i%2));
    if ((i%2)||(i+1==int(lens.size()))) {
      event[i_header++] = val;
      val=0;
    }
  }

  event.push_back(0xd07e2021u);
  event.push_back(0x12345678u);
  
  return event;
}

void PolarfireTarget::setBiasSetting(int board, bool led, int hdmi, int val) {
  Bias bias = hcal.bias(board);
  if (led) bias.setLED(hdmi,val);
  else bias.setSiPM(hdmi,val);
}

bool PolarfireTarget::loadBiasSettings(const std::string& file_name) {
  if (endsWith(file_name, ".csv")) {
    loadIntegerCSV(file_name,
        [&](const std::vector<int>& cells) {
          if (cells.size() == 4) {
            setBiasSetting(cells.at(0), cells.at(1) == 1, cells.at(2), cells.at(3));
          } else {
            std::cout << 
              "WARNING: Ignoring Bias CSV settings line"
              "without exactly four columns." 
              << std::endl;
          }
        });
  } else if (endsWith(file_name, ".yaml") or endsWith(file_name, ".yml")) {
    std::cerr << "Loading settings from YAML not implemented here yet." << std::endl;
  } else {
    std::cerr << file_name << " has no recognized extension (.csv, .yaml, .yml)" << std::endl;
  }
}

void PolarfireTarget::elink_relink(int verbosity) {
  pflib::Elinks& elinks=hcal.elinks();

  if (verbosity>0) 
    printf("...Setting phase delays\n");    
  for (int ilink=0; ilink<elinks.nlinks(); ilink++) 
    if (elinks.isActive(ilink)) 
      elinks.setDelay(ilink,20);

  
  if (verbosity>0) {
    printf("...Hard reset\n");
  }
  elinks.resetHard();
  sleep(1);
  
  if (verbosity>0) {
    printf("...Scanning bitslip values\n");
  }

  for (int ilink=0; ilink<elinks.nlinks(); ilink++) {
    if (!elinks.isActive(ilink)) continue;
    
    elinks.setBitslipAuto(ilink,false);
    
    int okcount[8];
    
    for (int slip=0; slip<8; slip++) {
      elinks.setBitslip(ilink,slip);
      okcount[slip]=0;
      for (int cycles=0; cycles<5; cycles++) {
        std::vector<uint8_t> spy=elinks.spy(ilink);
        for (size_t i=0; i+4<spy.size(); ) {
          if (spy[i]==0x9c || spy[i]==0xac) {
            if (spy[i+1]==0xcc && spy[i+2]==0xcc && spy[i+3]==0xcc) okcount[slip]++;
            i+=4;
          } else i++;
        } 
      }
    }
    int imax=-1;
    for (int slip=0; slip<8; slip++) {
      if (imax<0 || okcount[slip]>okcount[imax]) imax=slip;
    }
    elinks.setBitslip(ilink,imax);
    printf("    Link %d bitslip %d  (ok count=%d)\n",ilink,imax,okcount[imax]);
  }
  
}

  

}  // namespace pflib

