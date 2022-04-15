#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdint.h>
#include <fcntl.h>
#include "pflib/decoding/SuperPacket.h"

void usage() {
  fprintf(stderr, "Usage: -o [output_file] [file1] [file2] \n");
}

std::vector<uint32_t> get_next_superpacket(int infile) {
  static const int header_size=1+1+8+4; // includes BEEF2022
  
  // get the next header
  std::vector<uint32_t> data(header_size,0);

  size_t n1=read(infile,&(data[0]),header_size*sizeof(uint32_t));
  if (n1<header_size*sizeof(uint32_t)) {
    return std::vector<uint32_t>();
  }

  pflib::decoding::SuperPacket sp(&(data[0]),header_size);
  size_t req=sp.length32()+sp.offset_to_header()-header_size;
  data.resize(sp.length32()+sp.offset_to_header()); // sp is now invalid

  n1=read(infile,&(data[header_size]),req*sizeof(uint32_t));
  if (n1<req*sizeof(uint32_t)) {
    return std::vector<uint32_t>();
  }
  return data;
}

int main(int argc, char* argv[]) {
  int opt;
  int infile=0, infile2=0;
  int verbosity=5;
  int rocfocus=-1;
  bool showdata=false;
  bool prettyadc=false;
  bool printlink=false;
  FILE* outfile=0;
  
  while ((opt = getopt(argc, argv, "dlv:r:a")) != -1) {
    switch (opt) {
    case 'o':
      outfile=fopen(optarg,"w");
      break;
    default: /* '?' */
      usage();
      exit(EXIT_FAILURE);
    }
  }

  if (optind+1 >= argc) {
    usage();
    return 0;
  }

  infile=open(argv[optind],O_RDONLY);

  if (infile<0) {
    fprintf(stderr, "Unable to open file '%s'\n",
	    argv[optind]);
    exit(EXIT_FAILURE);
  }

  infile2=open(argv[optind+1],O_RDONLY);
  
  if (infile2<0) {
    fprintf(stderr, "Unable to open file '%s'\n",
	    argv[optind+1]);
    exit(EXIT_FAILURE);
  }

  do {
    using namespace pflib::decoding;
    std::vector<uint32_t> data1=get_next_superpacket(infile);
    if (data1.empty()) {
      printf("Done with file 1\n");
      break;
    }
    std::vector<uint32_t> data2=get_next_superpacket(infile2);
    if (data2.empty()) {
      printf("Done with file 2\n");
      break;
    }

    SuperPacket sp1(&(data1[0]),data1.size());
    SuperPacket sp2(&(data2[0]),data2.size());
    
    printf("Size1 is: %d EVT : %d Time : %d  Offset : %d\n",sp1.length32(),sp1.eventid(),sp1.time_in_spill(),sp1.offset_to_header());
    printf("Size2 is: %d EVT : %d Time : %d  Offset : %d\n",sp2.length32(),sp2.eventid(),sp2.time_in_spill(),sp2.offset_to_header());
    
  } while (1);

  return 0;
}
