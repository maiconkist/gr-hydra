#!/usr/bin/python2
#!/usr/bin/env python
#
# Copyright 2005,2006,2011,2013 Free Software Foundation, Inc.
# 
# This file is part of GNU Radio
# 
# GNU Radio is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3, or (at your option)
# any later version.
# 
# GNU Radio is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with GNU Radio; see the file COPYING.  If not, write to
# the Free Software Foundation, Inc., 51 Franklin Street,
# Boston, MA 02110-1301, USA.
# 


# GNURadio blocks
from gnuradio import gr
from gnuradio.eng_option import eng_option
from gnuradio import blocks

# parse options
from optparse import OptionParser

# Server for remote commands
import SimpleXMLRPCServer
import threading

# from current dir
from transmit_path import ReadThread, XMLRPCThread
from uhd_interface import uhd_transmitter

import radio_hydra, radio_lte, radio_nbiot

class my_top_block(gr.top_block):
    def __init__(self, options):
        gr.top_block.__init__(self)

        # change it to false to use a file
        if (False):
            print("Using USRP")
            self.sink = uhd_transmitter(options.args,
                                        options.hydra_bandwidth, options.hydra_freq,
                                        options.lo_offset, options.gain,
                                        options.spec, options.antenna,
                                        options.clock_source, options.verbose)
        else:
            print("Using USRP")
            self.sink = blocks.null_sink(gr.sizeof_gr_complex)

        # do this after for any adjustments to the options that may
        # occur in the sinks (specifically the UHD sink)
        print("Creating 2 VRs")
        self.lte_path   = radio_lte.build(options)
        self.nbiot_path = radio_nbiot.build(options)
        self.hydra      = radio_hydra.build(options)

        self.connect(self.lte_path, (self.hydra, 0), self.sink)
        self.connect(self.nbiot_path, (self.hydra, 1))

# /////////////////////////////////////////////////////////////////////////////
#                                   main
# /////////////////////////////////////////////////////////////////////////////
def main():

    parser = OptionParser(option_class=eng_option, conflict_handler="resolve")

    radio_hydra.add_options(parser)
    radio_lte.add_options(parser)
    radio_nbiot.add_options(parser)
    uhd_transmitter.add_options(parser)

    (options, args) = parser.parse_args()

    tb = my_top_block(options)
    tb.start()                       # start flow graph

    print 'Starting VR1 data thread...'
    t1 = ReadThread(options.lte_file, options.lte_buffersize, tb.lte_path)
    t1.start()

    print 'Starting VR2 data thread...'
    t2 = ReadThread(options.lte_file, options.nbiot_buffersize, tb.nbiot_path)
    t2.start()

    print 'Starting'
    return tb

if __name__ == '__main__':
    tb = None
    try:
       tb = main()
       tb.wait()
    except KeyboardInterrupt:
       print "Closing ..."
       tb.xmlrpc_server.stop()
       tb.stop()
