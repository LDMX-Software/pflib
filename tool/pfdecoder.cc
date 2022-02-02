#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdint.h>
#include <fcntl.h>

void usage() {
  fprintf(stderr, "Usage: [-d] [-v verbosity] [-r roclink] [file]\n");
  fprintf(stderr, "  -d  : show data words (default is header only)\n");
  fprintf(stderr, "  -v [verbosity level] (default is 5)\n");
  fprintf(stderr, "     0 : report error conditions only\n");
  fprintf(stderr, "     1 : report superheaders\n");
  fprintf(stderr, "     2 : report superheaders and packet headers\n");
  fprintf(stderr, "     3 : report superheaders, packet, and per-ROClink headers\n");
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

  int superpacket_header=-1;
  int packet_header=-1;
  int link_header=-1;
  int samples=-1;
  int isample=-1;
  int links=-1;
  int ilink=-1;
  int packet_bx=0;
  enum WordType { wt_unknown, wt_superheader, wt_packetheader, wt_linkheader, wt_data, wt_junk } word_type;
  std::vector<int> packet_pointers;
  std::vector<int> link_pointers;
  std::vector<uint32_t> link_data;
  
  for (int i=0; i<int(data.size()); i++) {

    word_type=wt_unknown;
    if (data[i]==0x11111111 && data[i+1]==0xbeef2021) { // detect master header
      superpacket_header=i;
      word_type=wt_junk;
    }
    if (superpacket_header<0) continue; // still searching...
    int rel_super=i-superpacket_header;
    int rel_packet=i-packet_header;
    int rel_link=i-link_header;
    if (rel_super==1) word_type=wt_junk; // the 0xbeef2021
    else if (rel_super==2) {
      word_type=wt_superheader;
      link_data.clear();
      samples=(data[i]>>16)&0xF;
      if (verbosity>0)
	printf("%04d S%02d %08x  FPGA=%3d  SAMPLES=%d  Total Length=%d (0x%x)\n",i,rel_super,data[i],(data[i]>>20)&0xFF,samples,(data[i])&0xFFFF,(data[i])&0xFFFF);
      packet_pointers.clear();
      packet_pointers.push_back(i+(samples+1)/2+1);
      isample=-1;
      packet_header=-1;
      link_header=-1;
    } else if (rel_super>2 && rel_super<=2+(samples+1)/2) {
      word_type=wt_superheader;
      if (verbosity>0)
	printf("%04d S%02d %08x  ",i,rel_super,data[i]);
      int jsample=(rel_super-3)*2;
      int len=(data[i]&0xFFF);
      
      if (verbosity>0)
	printf("Sample %d length=%d (0x%x)",jsample, len, len);
      packet_pointers.push_back(packet_pointers.back()+len);
      
      if (samples>jsample+1) {
	jsample++;
	len=(data[i]>>16)&0xFFF;
	if (verbosity>0)
	  printf("  Sample %d length=%d (0x%x)",jsample,len,len);
	packet_pointers.push_back(packet_pointers.back()+len);
      }
      if (verbosity>0)
	printf("\n");     
    } else if (int(packet_pointers.size())>isample+1 && i==packet_pointers[isample+1]) {
      packet_header=i;
      isample++;
      word_type=wt_packetheader;
      links=(data[i]>>14)&0x3F;
      link_pointers.clear();
      link_pointers.push_back(packet_header+2+(links+3)/4);
      if (verbosity>1)
	printf("%04d P%02d %08x  FPGA=%3d  LINKS=%d  Length=%d (0x%x)\n",i,0,data[i],(data[i]>>20)&0x3F,links,(data[i])&0xFFF,(data[i])&0xFFF);
      ilink=-1;
    } else if (isample>0 && i+1==packet_pointers[isample+1]) { // packet trailer
      if (verbosity>1)
	printf("%04d PXX %08x  CRC\n",i,0,data[i]);
      if (isample+1==samples) {
	superpacket_header=-1;
	if (prettyadc) renderADC(link_data);
      }
    } else if (packet_header>=0 && i==packet_header+1) { // second packet header word
      word_type=wt_packetheader;
      packet_bx=(data[i]>>20)&0xFFF;
      if (verbosity>1)
	printf("%04d P%02d %08x  BX=%3d  RREQ=%d  OR=%d\n",i,rel_packet,data[i],packet_bx,(data[i]>>10)&0x3FF,(data[i])&0x3FF);
    } else if (packet_header>0 && i>(packet_header+1) && i<link_pointers[0]) { // link lens
      int ilink=(i-packet_header-2)*4;
      word_type=wt_packetheader;
      if (verbosity>1)
	printf("%04d P%02d %08x",i,rel_packet,data[i]);
      for (int j=0; j<4 && ilink+j<links; j++) {
	int alen=((data[i]>>(j*8))&0x3F);
	link_pointers.push_back(link_pointers.back()+alen);
	if (verbosity>1)
	  printf("  Len[%2d]=%2d",j+ilink,alen);
      }
      if (verbosity>1)
	printf("\n");
    } else if (links>ilink+1 && i==link_pointers[ilink+1]) { // start of link headers
      word_type=wt_linkheader;
      ilink++;
      link_header=i;
      if (verbosity>2)
	printf("%04d L%02d %08x  Link %d ID=0x%03x\n",i,0,data[i],ilink,data[i]>>16);
    } else if (rel_link==1) {
      word_type=wt_linkheader;
      if (verbosity>2)
	printf("%04d L%02d %08x  \n",i,rel_link,data[i]);         
    } else if (rel_link==2) {
      word_type=wt_linkheader;
      if (verbosity>2)
	printf("%04d L%02d %08x  ",i,rel_link,data[i]);     
      if ((data[i]&0xFF000000)!=0xAA000000 && verbosity>2) printf("BAD HEADER! ");
      int linkbx=(data[i]>>12)&0xFFF;
      if (verbosity>2)
	printf("BX=%3d (delta=%d) \n",linkbx,packet_bx-linkbx);
    } else if (link_header>0 && rel_link<42) {
      if (rel_link<41 && rel_link!=21 && ilink==rocfocus) link_data.push_back(data[i]);
      word_type=wt_data;
      if (showdata) {
	printf("%04d L%02d %08x  \n",i,rel_link,data[i]);
      }
    } else {
      if (word_type!=wt_junk && (word_type!=wt_data || showdata))
	printf("%04d %08x\n",i,data[i]);
    }
  }

  close(infile);

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
