#!/usr/bin/python2
#!/usr/bin/env python
#
# Copyright 2006,2007,2011,2013 Free Software Foundation, Inc.
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
from gnuradio import gr
from gnuradio.eng_option import eng_option
from gnuradio import blocks

from optparse import OptionParser
import time

# Server for remote commands
import SimpleXMLRPCServer
import threading
import struct, sys

import basicconfig as bc
import radio_master, radio_lte, radio_nbiot
from uhd_interface import uhd_receiver

class my_top_block(gr.top_block):
    def __init__(self, rx_callback, options):
        gr.top_block.__init__(self)

        if(options.from_file is None):
            self.source = radio_master.build_usrp_rx(options.vr_configuration, options)
        elif (options.from_file is not None):
            self.source = blocks.file_source(gr.sizeof_gr_complex, options.from_file)

        # Set up receive path
        # do this after for any adjustments to the options that may
        # occur in the sinks (specifically the UHD sink)
        self.radio = radio_master.build_rx_from_name(options.vr_configuration, rx_callback, options)
        self.connect(self.source, self.radio)

# /////////////////////////////////////////////////////////////////////////////
#                                   main
# /////////////////////////////////////////////////////////////////////////////
def main():
    parser = OptionParser(option_class=eng_option, conflict_handler="resolve")
    parser.add_option("", "--vr-configuration", type="string", default=None,
                     help="Default configuration for VR RX (matches the configuration of TX) [default=%default]")
    parser.add_option("", "--from-file", type="string", default=None,
                     help="Specify a file source if USRP is not used [default=%default]")

    radio_lte.add_options(parser)
    radio_nbiot.add_options(parser)
    uhd_receiver.add_options(parser)
    (options, args) = parser.parse_args()

    if options.vr_configuration not in bc.VIRTUAL_RADIO:
        print("Invalid virtual radios. Valid radios are: %s" % (",".join(bc.VIRTUAL_RADIO)))
        return 1

    def generic_rx_callback(ok, payload):
        (pktno,) = struct.unpack('!H', payload[0:2])
        print "ok: %r \t pktno: %d \t len: %d, \t timestamp: %f" % (ok, pktno, len(payload). time.time())

    # build the graph
    tb = my_top_block(generic_rx_callback, options)
    tb.start()                      # start flow graph
    tb.wait()                       # wait for it to finish

if __name__ == '__main__':
    try:
        main()
    except KeyboardInterrupt:
        pass
