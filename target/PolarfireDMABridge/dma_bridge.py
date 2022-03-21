"""Bridge DMA readout over a TCP connection

Modified from the HPS daq fw/sw:
    https://github.com/slaclab/heavy-photon-daq/blob/Run2021/firmware/common/HpsDaq/python/hps/_RceTcpServer.py
"""

import pyrogue
import rogue
import socket
import ipaddress
import time

# not sure if Rce-specialized Root is necessary
#import RceG3
#
#class RceRoot(pyrogue.Root) :
#    def __init__(self, memBase, **kwargs) :
#        super().__init__(**kwargs)
#        
#        self.add(RceG3.RceVersion(memBase=memBase))
#        self.add(RceG3.RceEthernet(memBase=memBase, offset=0xB0000000))

class RceTcpServer() :
    """Server running on the RCE

    DMA -> TCP Server -> DAQ Computer
     |           |
    dma_dbg    server_dbg

    Parameters
    ----------
    debug : bool
        Setup debug streams
    device_path : str
        path to DMA device
    port : int
        ???
    """
    def __init__(self, debug = True, device_path = '/dev/dma_0', port = 5971) :
        rogue.Logging.setFileter('pyrogue.stream.TcpCore', rogue.Logging.Debug)
        rogue.Logging.setFilter('pyrogue.memory.TcpServer', rogue.Logging.Debug)
        print(f'Starting {self.__class__}')
        
        # Args: 
        #   path – Path to device. i.e /dev/datadev_0 
        #   dest – Destination index for dma transactions 
        #   ssiEnable – Enable SSI user fields

        self.dma = rogue.hardware.axi.AxiStreamDma(device_path, 0, True)
        self.server = rogue.interfaces.stream.TcpServer('*', port)

        # connect dma to sever
        pyrogue.streamConnectBiDir(self.server, self.dma)

        if debug :
            # debugging streamers
            self.server_dbg = rogue.interfaces.stream.Slave()
            self.dma_dbg = rogue.interfaces.stream.Slave()
    
            pyrogue.streamTap(self.server, self.server_dbg)
            self.server_dbg.setDebug(100, 'TCP: ')
    
            pyrogue.streamTap(self.dma, self.dma_dbg)
            self.dma_dbg.setDebug(100, 'DMA: ')

if __name__ ==  '__main__' :
    server = RceTcpServer()
    try :
        while True
            time.sleep(1)
    except KeyboardInterrupt:
        print('Stopping RceTcpServer')
