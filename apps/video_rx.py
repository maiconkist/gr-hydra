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
from gnuradio import eng_notation
from gnuradio.eng_option import eng_option
from gnuradio import blocks
from gnuradio import digital

from optparse import OptionParser
import socket

# Server for remote commands
import SimpleXMLRPCServer
import threading

# from current dir
from receive_path import receive_path
from uhd_interface import uhd_receiver

import struct, sys


n_rcvd = 0
n_right = 0

# For a nested dict, you need to recursively update __dict__
def dict2obj(d):
        if isinstance(d, list):
            d = [dict2obj(x) for x in d]
        if not isinstance(d, dict):
            return d

        class C(object):
            pass

        o = C()
        for k in d:
            o.__dict__[k] = dict2obj(d[k])
        return o

class my_top_block(gr.top_block):
    def __init__(self, callback, options):
        gr.top_block.__init__(self)

        # rpc server to receive remote commands
        self.xmlrpc_server = SimpleXMLRPCServer.SimpleXMLRPCServer(("localhost", options.rpc_port), allow_none=True)
        self.xmlrpc_server.register_instance(self)
        threading.Thread(target=self.xmlrpc_server.serve_forever).start()


        if(options.freq is not None):
            self.source = uhd_receiver(options.args,
                                       options.bandwidth, options.freq, 
                                       options.lo_offset, options.gain,
                                       options.spec, options.antenna,
                                       options.clock_source, options.verbose)
        elif(options.from_file is not None):
            self.source = blocks.file_source(gr.sizeof_gr_complex, options.from_file)
        else:
            self.source = blocks.null_source(gr.sizeof_gr_complex)

        # Set up receive path
        # do this after for any adjustments to the options that may
        # occur in the sinks (specifically the UHD sink)
        self.rxpath = receive_path(callback, options)

        self.connect(self.source, self.rxpath)


    def get_center_freq(self):
        return self.source.get_center_freq()

    def get_bandwidth(self):
        return self.source.get_sample_rate()

    def get_pkt_rcvd(self):
        global n_rcvd
        return n_rcvd

    def get_pkt_right(self):
        global n_right
        return n_right

    def set_center_freq(self, freq):
        self.source.set_freq(freq)
        return self.get_center_freq()

    def set_bandwidth(self, samp_rate):
        self.source.set_sample_rate(samp_rate)
        return self.get_bandwidth()

    def set_gain(self, gain):
        return self.source.set_gain(gain)

        
# /////////////////////////////////////////////////////////////////////////////
#                                   main
# /////////////////////////////////////////////////////////////////////////////

def main():

    global n_rcvd, n_right
        
    n_rcvd = 0
    n_right = 0

    parser = OptionParser(option_class=eng_option, conflict_handler="resolve")
    parser.add_option("", "--vr-configuration", type="int", default=1,
                      help="Default configuration for VR RX (matches the configuration of TX) [default=%default]")  

    expert_grp = parser.add_option_group("Expert")
    
    expert_grp.add_option("-p", "--port", type="intx", default=12346,
                      help="set UDP socket port number [default=%default]")
    expert_grp.add_option("-p", "--rpc-port", type="intx", default=12345,
                      help="set UDP socket port number [default=%default]")
    expert_grp.add_option("", "--host", default="127.0.0.1",
                      help="set host IP address [default=%default]")  

    receive_path.add_options(expert_grp, expert_grp)
    uhd_receiver.add_options(expert_grp)
    digital.ofdm_demod.add_options(expert_grp, expert_grp)
    (options, args) = parser.parse_args ()

    svl_center_freq = 3.5e9
    options_vr1 = dict2obj({'tx_amplitude': 0.125,
                    'freq': svl_center_freq - 500e3,
                    'bandwidth': 1e6,
                    'gain': 15,
                    'snr' : options.snr,
                    'file': None,
                    'buffersize': 4072,
                    'modulation': 'qpsk',
                    'fft_length': 512,
                    'occupied_tones': 200,
                    'cp_length': 4,
                    'host' : options.host,
                    'rpc_port' : options.rpc_port,
                    'port' : options.port,
                    'args' : options.args,
                    'lo_offset' : options.lo_offset,
                    'spec' : options.spec,
                    'antenna' : options.antenna,
                    'clock_source' : options.clock_source,
                    'verbose': False,
                    'log': False})
    options_vr2 = dict2obj({'tx_amplitude': 0.125,
                    'freq': svl_center_freq + 200e3,
                    'bandwidth': 200e3,
                    'gain': 15,
                    'snr' : options.snr,
                    'file': None,
                    'buffersize': 4072,
                    'modulation': 'bpsk',
                    'fft_length': 64,
                    'occupied_tones': 48,
                    'cp_length': 2,
                    'host' : options.host,
                    'rpc_port' : options.rpc_port,
                    'port' : options.port,
                    'args' : options.args,
                    'lo_offset' : options.lo_offset,
                    'spec' : options.spec,
                    'antenna' : options.antenna,
                    'clock_source' : options.clock_source,
                    'verbose': False,
                    'log': False})

    vr_configuration = [options_vr1, options_vr2]
    if options.vr_configuration is not None:
        options = vr_configuration[options.vr_configuration - 1]
		
    if options.freq is None:
        sys.stderr.write("You must specify -f FREQ or --freq FREQ\n")
        parser.print_help(sys.stderr)
        sys.exit(1)


    cs = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    def rx_callback(ok, payload):
        global n_rcvd, n_right
        n_rcvd += 1
        (pktno,) = struct.unpack('!H', payload[0:2])
        if ok:
            n_right += 1
        print "ok: %r \t pktno: %d \t n_rcvd: %d \t n_right: %d" % (ok, pktno, n_rcvd, n_right)
        print "Sent packet length = %4d" % (len(payload[2:])) 
        cs.sendto(payload[2:], (options.host, options.port))

    # build the graph
    tb = my_top_block(rx_callback, options)

    r = gr.enable_realtime_scheduling()
    if r != gr.RT_OK:
        print "Warning: failed to enable realtime scheduling"

    tb.start()                      # start flow graph
    tb.wait()                       # wait for it to finish
    tb.xmlrpc_server.shutdown()

if __name__ == '__main__':
    try:
        main()
    except KeyboardInterrupt:
        pass
