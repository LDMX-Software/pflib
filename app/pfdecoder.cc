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

#include "pflib/packing/FileReader.h"
#include "pflib/packing/Hex.h"

using pflib::packing::hex;

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

  uint32_t word;
  while (r) {
    r >> word;
    std::cout << hex(word) << " ";
    if (word == 0xbeef2025) {
      // begin new event frame
      std::cout << "begin event";
    } else if (word == 0x12345678) {
      // end event frame
      std::cout << "end event";
    }
    std::cout << std::endl;
  }

  return 0;
}
