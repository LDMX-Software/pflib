#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdint.h>
#include <fcntl.h>

#include <bitset>
#include <span>

#include "pflib/packing/FileReader.h"
#include "pflib/packing/Hex.h"
#include "pflib/packing/LinkFrame.h"

using pflib::packing::hex;
using pflib::packing::mask;

struct TriggerLinkFrame {
  // TODO actually decode the data
  std::vector<uint32_t> data;
  uint32_t ilink;

  pflib::packing::Reader& read(pflib::packing::Reader& r) {
    uint32_t header;
    r >> header;

    if ((header >> 28) != 0b0011) {
      // bad!
      std::cout << "bad trigger link header" << std::endl;
      return r;
    }

    uint32_t len = ((header >> 8) & mask<20>);
    ilink = (header & mask<8>);
    r.read(data, len);
    return r;
  }
};

struct SingleROCPacket {
  std::array<pflib::packing::LinkFrame, 2> daq_links;
  std::array<TriggerLinkFrame, 4> trigger_links;
  pflib::packing::Reader& read(pflib::packing::Reader& r) {
    uint32_t prev_word{0}, word{0};
    std::cout << "header scan...";
    while (prev_word != 0x11888811 or word != 0xbeef2025) {
      prev_word = word;
      r >> word;
      std::cout << hex(word) << " ";
    }
    if (!r) {
      std::cout << "no header found" << std::endl;
      return r;
    }
    std::cout << "found header" << std::endl;

    uint32_t total_len;
    r >> total_len;
    std::cout << hex(total_len) << " total len: " << total_len << std::endl;
    if (!r) {
      std::cout << "partially transmitted ROC stream" << std::endl;
      return r;
    }

    std::vector<uint32_t> link_data;
    r.read(link_data, total_len);
    if (!r) {
      std::cout << "partially transmitted ROC stream" << std::endl;
      return r;
    }

    // three words containing the link lengths
    std::array<uint32_t, 6> link_lengths;
    for (std::size_t i_word{0}; i_word < 3; i_word++) {
      word = link_data.at(i_word);
      std::cout << hex(word) << " link lengths" << std::endl;
      link_lengths[2*i_word] = (word >> 16) & mask<16>;
      link_lengths[2*i_word+1] = (word) & mask<16>;
    }

    std::size_t link_start_offset{3};
    for (std::size_t i_link{0}; i_link < 6; i_link++) {
      auto link_len = link_lengths[i_link];
      std::cout << "Link " << i_link << " Length " << link_len << std::endl;
      if (i_link < 2) {
        daq_links[i_link].from(std::span(link_data.begin() + link_start_offset, link_len));
      } else {
        for (const auto& word : std::span(link_data.begin() + link_start_offset, link_len)) {
          std::cout << hex(word) << std::endl;
        }
      }
      link_start_offset += link_len;
    }

    std::cout << "trailer" << std::endl;
    for (const auto& word: std::span(link_data.begin()+link_start_offset, link_data.end())) {
      std::cout << hex(word) << std::endl;
    }

    return r;
  }
};

void usage() {
  fprintf(stderr, "Usage: [-d] [-v verbosity] [-r roclink] [file]\n");
  fprintf(stderr, "  -d  : show data words (default is header only)\n");
  fprintf(stderr, "  -v [verbosity level] (default is 3)\n");
  fprintf(stderr, "     0 : report error conditions only\n");
  fprintf(stderr, "     1 : report superheaders\n");
  fprintf(stderr, "     2 : report superheaders and packet headers\n");
  fprintf(stderr, "     3 : report superheaders, packet, and per-ROClink headers\n");
  fprintf(stderr, "    10 : report every line\n");
  fprintf(stderr, "  -r [roc link to focus on for data, headers] \n");
  fprintf(stderr, "  -a  : pretty output of ADC data\n");
}

int main(int argc, char* argv[]) {
  int opt;
  int infile=0;
  int verbosity=5;
  int rocfocus=-1;
  std::vector<uint32_t> data;
  bool showdata=false;
  bool prettyadc=false;

  
  while ((opt = getopt(argc, argv, "dv:r:a")) != -1) {
    switch (opt) {
    case 'd':
      showdata=true;
      break;
    case 'a':
      prettyadc=true;
      break;
    case 'v':
      verbosity=atoi(optarg);
      break;
    case 'r':
      rocfocus=atoi(optarg);
      break;
    default: /* '?' */
      usage();
      exit(EXIT_FAILURE);
    }
  }

  if (optind >= argc) {
    usage();
    return 0;
  }

  pflib::packing::FileReader r{argv[optind]};
  if (not r) {
    fprintf(stderr, "Unable to open file '%s'\n",
	    optarg);
    exit(EXIT_FAILURE);
  }

  SingleROCPacket ep;
  int count{0};
  while (r) {
    r >> ep;
    if (++count > 2) break;
  }

  return 0;
}
