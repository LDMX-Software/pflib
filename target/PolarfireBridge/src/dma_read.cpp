/**
 * dma_read.cpp helping develop a rogue source to readout data written 
 * to DMA via firmware.
 *
 * Adapted from:
 *-----------------------------------------------------------------------------
 * Title      : DMA read utility
 * ----------------------------------------------------------------------------
 * File       : dmaRead.cpp
 * Author     : Ryan Herbst, rherbst@slac.stanford.edu
 * Created    : 2016-08-08
 * Last update: 2016-08-08
 * ----------------------------------------------------------------------------
 * Description:
 * This program will open up a AXIS DMA port and attempt to read data.
 * ----------------------------------------------------------------------------
 * This file is part of the aes_stream_drivers package. It is subject to
 * the license terms in the LICENSE.txt file found in the top-level directory
 * of this distribution and at:
 *    https://confluence.slac.stanford.edu/display/ppareg/LICENSE.html.
 * No part of the aes_stream_drivers package, including this file, may be
 * copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE.txt file.
 * ----------------------------------------------------------------------------
 */

#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <argp.h>
#include <iostream>

#include "DMAReader.h"
#include <rogue/utilities/fileio/StreamWriter.h>
#include <rogue/utilities/fileio/StreamWriterChannel.h>
#include <rogue/interfaces/stream/TcpServer.h>

const  char * argp_program_version = "pgpRead 1.0";
const  char * argp_program_bug_address = "rherbst@slac.stanford.edu";

struct PrgArgs {
   const char * path;
   const char * dest;
   int          port;
   uint32_t     idxEn;
   uint32_t     debug;
};

static struct PrgArgs DefArgs = { "/dev/axi_stream_dma_0", NULL, 5972, 0, 0 };

static char   args_doc[] = "";
static char   doc[]      = "";

static struct argp_option options[] = {
   { "path",    'p', "PATH",   OPTION_ARG_OPTIONAL, "Path of pgpcard device to use. Default=/dev/axi_stream_dma_0.",0},
   { "dest",    'd', "PATH",   OPTION_ARG_OPTIONAL, "Output file",0},
   { "port",    'o', "PORT",   OPTION_ARG_OPTIONAL, "Port to send frames to (instead of output file, -1 to disable, default 5972)",0},
   { "indexen", 'i', 0,        OPTION_ARG_OPTIONAL, "Use index based receive buffers.",0},
   { "debug", 'v', 0, OPTION_ARG_OPTIONAL, "Print status messages to terminal while DMAReader is up.", 0},
   {0}
};

error_t parseArgs ( int key,  char *arg, struct argp_state *state ) {
   struct PrgArgs *args = (struct PrgArgs *)state->input;
   switch(key) {
      case 'p': args->path = arg; break;
      case 'd': args->dest = arg; break;
      case 'i': args->idxEn = 1; break;
      case 'o': args->port = strtol(arg,NULL,10); break;
      case 'v': args->debug = 1; break;
      default: return ARGP_ERR_UNKNOWN; break;
   }
   return(0);
}

static struct argp argp = {options,parseArgs,args_doc,doc};

/**
 * Executable to start a server watching the DMA device and forwarding
 * along event packets when the file descriptor changes.
 *
 * Abilities
 * ---------
 * - Write to a _local_ file where this server is being run
 * - Forward frames to a TCP bridge
 *   - WILL hang if TCP client is not up and connected at readout time
 * - CAN do both BUT local file mysteriously has one less event than
 *   the file received over the TCP bridge
 *
 * Future
 * ------
 * - Merge with polarfire_wb_server and make multithreaded
 */
int main (int argc, char **argv) {
  struct PrgArgs args;

  memcpy(&args,&DefArgs,sizeof(struct PrgArgs));
  argp_parse(&argp,argc,argv,0,0,&args);

  try {
    auto src = DMAReader::create(args.path, args.idxEn, args.debug);
    if (args.port > 0) {
      if (args.debug)
        std::cout << "Sending frames to TCP bridge at port " << args.port << std::endl;
      // have a Tcp server waiting for frames
      auto tcp = rogue::interfaces::stream::TcpServer::create("*",args.port);
      src->addSlave(tcp);
    }
    if (args.dest) {
      if (args.debug)
        std::cout << "Sending frames to local file at " << args.dest << std::endl;
      rogue::utilities::fileio::StreamWriterPtr writer{
        rogue::utilities::fileio::StreamWriter::create()};
      writer->setBufferSize(10000);
      writer->open(args.dest);
      src->addSlave(writer->getChannel(0));
    }
    // enter watching loop
    src->watch();
  } catch (const std::exception& e) {
    std::cerr << "ERROR: " << e.what() << std::endl;
    return 1;
  }
  return 0;
}
