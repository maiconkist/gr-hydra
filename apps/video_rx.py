#!/usr/bin/env python
#
# Copyright 2005,2006,2007 Free Software Foundation, Inc.
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
# GNU General Public License	for more details.
# 
# You should have received a copy of the GNU General Public License
# along with GNU Radio; see the file COPYING.  If not, write to
# the Free Software Foundation, Inc., 51 Franklin Street,
# Boston, MA 02110-1301, USA.
# 

# Ver 1.0, Feb 6, 2009, Qin Chen
# Packets are received via GNU Radio
# UDP socket client then sends packets to FFplay

# Ver 1.1, Feb 9, 2009, Qin Chen
# Add command line option to specify socket IP address and port number

from gnuradio import gr, gru
from gnuradio.digital import modulation_utils
from gnuradio import usrp
from gnuradio import eng_notation
from gnuradio.eng_option import eng_option
from optparse import OptionParser

import random
import struct
import sys

# from current dir
from receive_path import receive_path
import fusb_options

# Qin 020609
import socket

#import os
#print os.getpid()
#raw_input('Attach and press enter: ')

class my_top_block(gr.top_block):
    def __init__(self, demodulator, rx_callback, options):
        gr.top_block.__init__(self)
        self.rxpath = receive_path(demodulator, rx_callback, options) 
        self.connect(self.rxpath)

# /////////////////////////////////////////////////////////////////////////////
#                                   main
# /////////////////////////////////////////////////////////////////////////////

global n_rcvd, n_right

def main():
    global n_rcvd, n_right

    n_rcvd = 0
    n_right = 0

    demods = modulation_utils.type_1_demods()

    # Create Options Parser:
    parser = OptionParser (option_class=eng_option, conflict_handler="resolve")
    expert_grp = parser.add_option_group("Expert")

    parser.add_option("-m", "--modulation", type="choice", choices=demods.keys(), 
                      default='gmsk',
                      help="Select modulation from: %s [default=%%default]"
                            % (', '.join(demods.keys()),))

    # Qin 020909
    parser.add_option("-p", "--port", type="intx", default=12346,
                      help="set UDP socket port number [default=%default]")
    parser.add_option("", "--host", default="127.0.0.1",
                      help="set host IP address [default=%default]")       

    receive_path.add_options(parser, expert_grp)

    for mod in demods.values():
        mod.add_options(expert_grp)

    fusb_options.add_options(expert_grp)
    (options, args) = parser.parse_args ()

    if len(args) != 0:
        parser.print_help(sys.stderr)
        sys.exit(1)

    if options.rx_freq is None:
        sys.stderr.write("You must specify -f FREQ or --freq FREQ\n")
        parser.print_help(sys.stderr)
        sys.exit(1)


    # Qin 020909
    # print options.host
    # chost = '10.227.180.171'
    # print options.port
    cs = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    cs.connect((options.host, options.port))
    # global_pl = ''
    
    def rx_callback(ok, payload):
        global n_rcvd, n_right
	print "Received packet length = %4d" % (len(payload))
        (pktno,) = struct.unpack('!H', payload[0:2])
        n_rcvd += 1
        if ok:
            n_right += 1

        print "ok = %5s  pktno = %4d  n_rcvd = %4d  n_right = %4d" % (
            ok, pktno, n_rcvd, n_right)

        # Qin 020609
	print "Sent packet length = %4d" % (len(payload[2:])) 
        cs.send(payload[2:])
        # global_pl = payload[2:]

    # build the graph
    tb = my_top_block(demods[options.modulation], rx_callback, options)

    # Qin 021609
    # send packet to ffplay
    # cs.send(global_pl)
    
    r = gr.enable_realtime_scheduling()
    if r != gr.RT_OK:
        print "Warning: Failed to enable realtime scheduling."

    # close the socket
    # cs.close()

    tb.start()        # start flow graph
    tb.wait()         # wait for it to finish

if __name__ == '__main__':
    try:
        main()
    except KeyboardInterrupt:
        pass
