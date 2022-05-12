#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <algorithm>
#include <numeric>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdint.h>
#include <fcntl.h>
#include <iostream>
void usage() {
  fprintf(stderr, "Usage: [-d] [-v verbosity] [-r roclink] [file]\n");
  fprintf(stderr, "  -d  : show data words (default is header only)\n");
  fprintf(stderr, "  -l  : print link alignment counters at end\n");
  fprintf(stderr, "  -e : print spills and timestamps at end\n" );
  fprintf(stderr, "  -v [verbosity level] (default is 3)\n");
  fprintf(stderr, "     0 : report error conditions only\n");
  fprintf(stderr, "     1 : report superheaders\n");
  fprintf(stderr, "     2 : report superheaders and packet headers\n");
  fprintf(stderr, "     3 : report superheaders, packet, and per-ROClink headers\n");
  fprintf(stderr, "    10 : report every line\n");
  fprintf(stderr, "  -r [roc link to focus on for data, headers] \n");
  fprintf(stderr, "  -a  : pretty output of ADC data\n");
}

void renderADC(const std::vector<uint32_t>& data);


int main(int argc, char* argv[]) {
  int opt;
  int infile=0;
  int verbosity=5;
  int rocfocus=-1;
  std::vector<uint32_t> data;
  bool showdata=false;
  bool prettyadc=false;
  bool printevents=false;
  bool printlink=false;
  
  while ((opt = getopt(argc, argv, "dlv:r:ae")) != -1) {
    switch (opt) {
    case 'd':
      showdata=true;
      break;
    case 'l':
      printlink=true;
      break;
    case 'a':
      prettyadc=true;
      break;
    case 'e':
      printevents=true;
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

  infile=open(argv[optind],O_RDONLY);

  if (infile<0) {
    fprintf(stderr, "Unable to open file '%s'\n",
	    optarg);
    exit(EXIT_FAILURE);
  }

  struct stat finfo;
  
  fstat(infile,&finfo);
  int words=finfo.st_size/sizeof(uint32_t);
  data=std::vector<uint32_t>(words,0);
  read(infile,&(data[0]),finfo.st_size);

  int fmt=1;
  int superpacket_header=-10;
  int packet_header=-1;
  int link_header=-1;
  int samples=-1;
  int isample=-1;
  int links=-1;
  int ilink=-1;
  int packet_bx=0;

  // these are for within the link packet
  //  helpful for debugging link connections
  int num_good_idles[8] = {0,0,0,0,0,0,0,0};
  int num_good_bxheaders[8] = {0,0,0,0,0,0,0,0};
  int num_bad_idles[8] = {0,0,0,0,0,0,0,0};
  int num_bad_bxheaders[8] = {0,0,0,0,0,0,0,0};

  enum WordType { wt_unknown, wt_superheader, wt_packetheader, wt_linkheader, wt_data, wt_junk } word_type;
  std::vector<int> packet_pointers;
  std::vector<int> link_pointers;
  std::vector<int> link_len;
  std::vector<uint32_t> link_data;

  std::vector<std::vector<int>> timestamps_per_spill{};
  int spillcounter {0};
  int timestamp_counter{0};
  for (int i=0; i<int(data.size()); i++) {
    word_type=wt_unknown;
    if (data[i]==0x11111111 && data[i+1]==0xbeef2021) { // detect master header
      superpacket_header=i;
      word_type=wt_junk;
    }
    if (superpacket_header<-1 && data[i]==0xbeef2022) {
      fmt=2;
      superpacket_header=i-1;
      word_type=wt_junk;
    }
    if (superpacket_header<-1) continue; // still searching...
    int rel_super=i-superpacket_header;
    int rel_packet=i-packet_header;
    int rel_link=i-link_header;
    if (rel_super==1) word_type=wt_junk; // the 0xbeef2021
    else if (rel_super==2) {
      word_type=wt_superheader;
      link_data.clear();
      samples=(data[i]>>16)&0xF;
      if (verbosity>0) printf("%04d S%02d %08x  FPGA=%3d  SAMPLES=%d  Total Length=%d (0x%x)\n",
          i,rel_super,data[i],(data[i]>>20)&0xFF,samples,(data[i])&0xFFFF,(data[i])&0xFFFF);
      packet_pointers.clear();
      if (fmt==1) packet_pointers.push_back(i+(samples+1)/2+1);
      else if (fmt==2) packet_pointers.push_back(i+8+4+1); // hardcoded for four tagging words
      isample=-1;
      packet_header=-1;
      link_header=-1;
    } else if (rel_super>2 && rel_super<=2+(samples+1)/2) {
      word_type=wt_superheader;
      if (verbosity>0) printf("%04d S%02d %08x  ",i,rel_super,data[i]);
      int jsample=(rel_super-3)*2;
      int len=(data[i]&0xFFF);
      
      if (verbosity>0) printf("Sample %d length=%d (0x%x)",jsample, len, len);
      if (fmt>=2 && (len%2==1)) len++; // 64-bit alignment
      packet_pointers.push_back(packet_pointers.back()+len);
      
      if (samples>jsample+1) {
	      jsample++;
	      len=(data[i]>>16)&0xFFF;
	      if (verbosity>0) printf("  Sample %d length=%d (0x%x)",jsample,len,len);
        if (fmt>=2 && (len%2==1)) len++; // 64-bit alignment
        packet_pointers.push_back(packet_pointers.back()+len);
      }
      if (verbosity>0) printf("\n");
    } else if (rel_super>2+(samples+1)/2 && rel_super<=2+8 && fmt==2) {
      word_type=wt_junk; // empty sample words
    } else if (int(packet_pointers.size())>isample+1 && i==packet_pointers[isample+1]) {
      packet_header=i;
      isample++;
      word_type=wt_packetheader;
      links=(data[i]>>14)&0x3F;
      link_len.clear();
      link_pointers.clear();
      link_pointers.push_back(packet_header+2+(links+3)/4);
      if (verbosity>1) printf("%04d P%02d %08x  FPGA=%3d  LINKS=%d  Length=%d (0x%x)\n",
          i,0,data[i],(data[i]>>20)&0x3F,links,(data[i])&0xFFF,(data[i])&0xFFF);
      ilink=-1;
    } else if (fmt==2 && rel_super>2+8 && packet_header<0) { // extended superheader
      int rel_super_tag=rel_super-(2+8+1);
      if (verbosity>1 || printevents) {
        if (rel_super_tag==0) {
          const int spill = (data[i] >> 12) &0xFFF;
          if (spill != spillcounter) {
            timestamps_per_spill.push_back({});
            ++spillcounter;
          }
          printf("%04d S%02d %08x    Spill = %d  BX = %d (0x%x)\n",
                 i,rel_super,data[i],spill,data[i]&0xFFF, data[i]&0xFFF);
        }
        else if (rel_super_tag==1) {
          const int ticks_since_start_of_spill = data[i];
          const double time_since_start_of_spill = ticks_since_start_of_spill / 5e6; // 5Mhz
          printf("%04d S%02d %08x    "
            "Time since start of spill %.5fs (%d 5 Mhz tics)\n",
            i,rel_super,data[i],time_since_start_of_spill,ticks_since_start_of_spill);
          timestamps_per_spill.back().push_back(ticks_since_start_of_spill);
        }
        else if (rel_super_tag==2) printf("%04d S%02d %08x    "
                                                   "Evt Number = %d \n",i,rel_super,data[i],data[i]);
        else if (rel_super_tag==3) printf("%04d S%02d %08x    "
            "Run %d start DD-MM hh:mm : %2d-%d %02d:%02d\n",
            i,rel_super,data[i],(data[i]&0xFFF),(data[i]>>23)&0x1F,
            (data[i]>>28)&0xF,(data[i]>>18)&0x1F, (data[i]>>12)&0x3F);      
      }
    } else if (isample>=0 && i+1==packet_pointers[isample+1]) { // packet trailer
      if (verbosity>1) printf("%04d PXX %08x  CRC\n",i,0,data[i]);
      if (isample+1==samples) {
	      superpacket_header=-10;
	      if (prettyadc) renderADC(link_data);
      }
    } else if (packet_header>=0 && i==packet_header+1) { // second packet header word
      word_type=wt_packetheader;
      packet_bx=(data[i]>>20)&0xFFF;
      if (verbosity>1) printf("%04d P%02d %08x  BX=%3d  RREQ=%d  OR=%d\n",
          i,rel_packet,data[i],packet_bx,(data[i]>>10)&0x3FF,(data[i])&0x3FF);
    } else if (packet_header>0 && i>(packet_header+1) && i<link_pointers[0]) { // link lens
      int ilink=(i-packet_header-2)*4;
      word_type=wt_packetheader;
      if (verbosity>1) printf("%04d P%02d %08x",i,rel_packet,data[i]);
      for (int j=0; j<4 && ilink+j<links; j++) {
	      int alen=((data[i]>>(j*8))&0x3F);
        link_len.push_back(alen);
	      link_pointers.push_back(link_pointers.back()+alen);
	      if (verbosity>1) printf("  Len[%2d]=%2d",j+ilink,alen);
      }
      if (verbosity>1) printf("\n");
    } else if (links>ilink+1 && i==link_pointers[ilink+1]) { // start of link headers
      word_type=wt_linkheader;
      ilink++;
      link_header=i;
      if (verbosity>2) printf("%04d L%02d %08x  Link %d ID=0x%03x\n",i,0,data[i],ilink,data[i]>>16);
    } else if (ilink > 0 and ilink < links and rel_link >= link_len[ilink]) {
      word_type=wt_junk;
    } else if (rel_link==1) {
      word_type=wt_linkheader;
      if (verbosity>2) printf("%04d L%02d %08x  \n",i,rel_link,data[i]);         
    } else if (rel_link==2) {
      word_type=wt_linkheader;
      bool bad=false;
      if ((data[i]&0xFF000000)!=0xAA000000) { bad=true; num_bad_bxheaders[ilink]++; }
      else num_good_bxheaders[ilink]++;
      if (verbosity>2) printf("%04d L%02d %08x  %s",i,rel_link,data[i],bad?"BAD HEADER!":"");     
      int linkbx=(data[i]>>12)&0xFFF;
      if (verbosity>2) printf(" BX=%3d (delta=%d) \n",linkbx,packet_bx-linkbx);
    } else if (rel_link==41) {
      // should be idle packet in hgcroc v2
      bool bad=false;
      if (data[i]!=0xACCCCCCC) { bad=true; num_bad_idles[ilink]++; }
      else num_good_idles[ilink]++;
      if (verbosity>2) printf("%04d L%02d %08x  %s\n",i,rel_link,data[i],bad?"BAD IDLE!":"");     
    } else if (link_header>0 && rel_link<41) {
      if (rel_link!=21 && ilink==rocfocus) link_data.push_back(data[i]);
      word_type=wt_data;
      if (showdata) printf("%04d L%02d %08x  \n",i,rel_link,data[i]);
    } else {
      if (verbosity>=10 || (word_type!=wt_junk && (word_type!=wt_data || showdata))) 
        printf("%04d %08x\n",i,data[i]);
    }
  }

  close(infile);


  if (printevents) {
    std::vector<double> duplicate_rates{};
    duplicate_rates.reserve(timestamps_per_spill.size());

    for (int spill_index {0}; spill_index < timestamps_per_spill.size(); ++spill_index) {
      std::cout << "Spill index: " << spill_index << std::endl;
      const auto timestamps = timestamps_per_spill[spill_index];
      for (auto timestamp : timestamps) {
        std::cout << "-- " << timestamp << '\n';
      }
    }
    for (int spill_index {0}; spill_index < timestamps_per_spill.size(); ++spill_index) {
      const int spill {spill_index + 1};
      std::cout << "Spill: " << spill << std::endl;
      const auto& timestamps = timestamps_per_spill[spill_index];
      for (const auto timestamp : timestamps ){
        // std::cout << "Timestamp: " << timestamp << '\n';
      }

      auto found_duplicate = std::adjacent_find(std::cbegin(timestamps),
                                                std::cend(timestamps));
      int duplicate_counter {0};
      while (found_duplicate != std::cend(timestamps)) {
        ++duplicate_counter;
        auto duplicate_ptr  = found_duplicate + 1;
        std::cout << "Found duplicate in spill "
                  << spill
                  << " at position "
                  << std::distance(std::cbegin(timestamps), found_duplicate)
                  << " with value " << *found_duplicate
                  << " duplicate " << *(duplicate_ptr)
                  << std::endl;
        found_duplicate = std::adjacent_find(duplicate_ptr,
                                             std::cend(timestamps));
      }
      const double duplicate_rate = static_cast<double>(duplicate_counter) / timestamps.size() * 100.;
      std::cout << "Total duplicates " << duplicate_counter
                << " out of " << timestamps.size()
                << " timestamps in spill " << spill
                << " or " << duplicate_rate
                << " %" << std::endl;
      duplicate_rates.push_back(duplicate_rate);
  }
  const auto average_rate_of_duplicates = std::accumulate(std::cbegin(duplicate_rates),
                                                          std::cend(duplicate_rates),
                                                          0.) / duplicate_rates.size();
  std::cout << "Average rate of duplicates: "
            << average_rate_of_duplicates << " % for "
            << timestamps_per_spill.size() << " spills" << std::endl;

  }
  if (printlink) {
    if (verbosity>0) printf("\n");
    printf("Link Alignment Checks\n");

    printf("     %26s | %26s\n","BX Headers","Idles");
    printf("Link %10s %10s %4s | %10s %10s %4s\n","Good","Bad","B/T","Good","Bad","B/T");
    for (int ilink{0}; ilink < 8; ilink++) {
      float bg_bxheaders = 0.;
      if (num_good_bxheaders[ilink]+num_bad_bxheaders[ilink]>0) 
        bg_bxheaders = float(num_bad_bxheaders[ilink])/
          (num_good_bxheaders[ilink]+num_bad_bxheaders[ilink]);
      float bg_idles = 0.;
      if (num_good_idles[ilink]+num_bad_idles[ilink]>0) 
        bg_idles = float(num_bad_idles[ilink])/
          (num_good_idles[ilink]+num_bad_idles[ilink]);
      printf("%4d %10d %10d %.2f | %10d %10d %.2f\n", ilink, 
          num_good_bxheaders[ilink], num_bad_bxheaders[ilink], bg_bxheaders,
          num_good_idles[ilink], num_bad_idles[ilink], bg_idles);
    }
  }

  return 0;
}


void renderADC(const std::vector<uint32_t>& data) {
  const int CHAN=37;
  int samples=int(data.size())/CHAN;

  for (int i=0; i<CHAN; i++) {
    printf(" %2d ",i);
    for (int j=0; j<samples; j++) {
      printf(" %4d",data[j*CHAN+i]&0x3FF);
    }
    for (int j=0; j<samples; j++) {
      printf(" %8x",data[j*CHAN+i]);
    }
    printf("\n");
  }
}
