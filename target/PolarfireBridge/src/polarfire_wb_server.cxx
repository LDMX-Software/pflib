#include <rogue/interfaces/memory/TcpServer.h>
#include "PolarfireWB.h"
#include <unistd.h>

static const int DEFAULT_PORT=5970;

void usage() {
  fprintf(stderr, "Usage: -A [baseaddress] -p [port] -d -D -F\n");
  fprintf(stderr, "      Address is mandatory\n");
  fprintf(stderr, "      Port will default to %d if not specified\n",DEFAULT_PORT);
  fprintf(stderr, "      -d Enable debug output\n");
  fprintf(stderr, "      -D Map DAQ page assuming standard location\n");
  fprintf(stderr, "      -F Map FastControl page assuming standard location\n");
};

int main(int argc, char* argv[]) {

  
  int opt;
  uint64_t addr=0;
  int port=DEFAULT_PORT;
  bool do_daq=false;
  bool do_fc=false;

  while ((opt = getopt(argc, argv, "A:p:dFD")) != -1) {
    switch (opt) {
      case 'A':
        addr=strtoul(optarg,0,0);
        break;
      case 'p':
        port=strtol(optarg,0,0);
        break;
    case 'F':
      do_fc=true;
      break;
    case 'D':
      do_daq=true;
      break;
    case 'd':
      rogue::Logging::setLevel(rogue::Logging::Debug);  
      break;
    default: /* '?' */
        usage();
        exit(EXIT_FAILURE);
    }
  }
  if (addr==0) {
    usage();
    exit(EXIT_FAILURE);
  }

  auto bus = std::make_shared<PolarfireWB>(addr);
  if (do_daq) bus->map_daq((addr&0xFFF00000u)|0x80000);
  if (do_fc) bus->map_fc((addr&0xFFF00000u));
  
  
  // have a Tcp server waiting for commands
  rogue::interfaces::memory::TcpServerPtr tcp = 
    rogue::interfaces::memory::TcpServer::create("*",port);

  // Connect the bus
  tcp->setSlave(bus);

  while (true) {
    sleep(100);
  }

  return 0;
}
