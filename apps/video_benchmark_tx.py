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

from gnuradio import gr
from gnuradio import eng_notation
from gnuradio.eng_option import eng_option
from optparse import OptionParser
import struct
import sys
import socket
import svl

from gnuradio import digital
from gnuradio import blocks

# from current dir
from transmit_path import transmit_path
from uhd_interface import uhd_transmitter

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
    def __init__(self, options, options_vr1, options_vr2):
        gr.top_block.__init__(self)

        if(options.tx_freq is not None):
            self.sink = uhd_transmitter(options.args,
                                        options.bandwidth, options.tx_freq,
                                        options.lo_offset, options.tx_gain,
                                        options.spec, options.antenna,
                                        options.clock_source, options.verbose)
        elif(options.to_file is not None):
            self.sink = blocks.file_sink(gr.sizeof_gr_complex, options.to_file)
        else:
            self.sink = blocks.null_sink(gr.sizeof_gr_complex)

        # do this after for any adjustments to the options that may
        # occur in the sinks (specifically the UHD sink)
        self.txpath1 = transmit_path(options_vr1)
        self.txpath2 = transmit_path(options_vr2)

	print options
        svl_sink = svl.svl_sink(1,
                                options.fft_length,
                                int(options.tx_freq),
                                int(options.bandwidth),
                                ((options_vr1.freq, options_vr1.bandwidth), )
                    )

        self.connect(self.txpath1, svl_sink, self.sink)
        #self.connect(self.txpath1, self.sink)

# /////////////////////////////////////////////////////////////////////////////
#                                   main
# /////////////////////////////////////////////////////////////////////////////

def main():

    def send_pkt(payload='', eof=False):
        return tb.txpath1.send_pkt(payload, eof)

    parser = OptionParser(option_class=eng_option, conflict_handler="resolve")

    svl_centerfrequency = 5.5e9
    svl_options = parser.add_option_group("HyDRA Options")
    svl_options.add_option("", "--fft-length", type="intx", default=2048,
                      help="HyDRA FFT M size [default=%default]")
    parser.add_option("", "--tx-freq", type="eng_float", default=svl_centerfrequency,
		    help="Hydra transmit frequency [default=%default]", metavar="FREQ")
    parser.add_option("-W", "--bandwidth", type="eng_float", default=2e6,
		    help="Hydra sample_rate [default=%default]")

    vr1_options = parser.add_option_group("VR 1 Options")
    vr1_options.add_option("", "--vr1-bandwidth", type="eng_float", default=1e6,
                      help="set bandwidth for VR 1 [default=%default]")
    vr1_options.add_option("", "--vr1-freq", type="eng_float", default=svl_centerfrequency,
                      help="set central frequency for VR 1 [default=%default]")
    vr1_options.add_option("", "--vr1-tx-amplitude", type="eng_float", default=0.1, metavar="AMPL",
                          help="set transmitter digital amplitude: 0 <= AMPL < 1.0 [default=%default]")
    vr1_options.add_option("", "--vr1-port", type="intx", default=12345,
                      help="set UDP socket port number for VR1 [default=%default]")
    vr1_options.add_option("", "--vr1-bufferbytes", type="intx", default=20480,
                      help="set UDP socket receiver buffer size for VR1 [default=%default]")
    vr1_options.add_option("-m", "--vr1-modulation", type="string", default="bpsk",
                      help="set modulation type (bpsk, qpsk, 8psk, qam{16,64}) [default=%default]")
    vr1_options.add_option("", "--vr1-fft-length", type="intx", default=512,
                      help="set the number of FFT bins [default=%default]")
    vr1_options.add_option("", "--vr1-occupied-tones", type="intx", default=200,
                      help="set the number of occupied FFT bins [default=%default]")
    vr1_options.add_option("", "--vr1-cp-length", type="intx", default=128,
                      help="set the number of bits in the cyclic prefix [default=%default]")

    vr2_options = parser.add_option_group("VR 2 Options")
    vr2_options.add_option("", "--vr2-bandwidth", type="eng_float", default=200e3,
                      help="set bandwidth for VR 2 [default=%default]")  
    vr2_options.add_option("", "--vr2-freq", type="eng_float", default=svl_centerfrequency+350e3,
                      help="set central frequency for VR 2 [default=%default]")  
    vr2_options.add_option("", "--vr2-tx-amplitude", type="eng_float", default=0.1, metavar="AMPL",
                          help="set transmitter digital amplitude: 0 <= AMPL < 1.0 [default=%default]")
    vr2_options.add_option("", "--vr2-port", type="intx", default=12345,
                      help="set UDP socket port number for VR 2 [default=%default]")
    vr2_options.add_option("", "--vr2-bufferbytes", type="intx", default=20480,
                      help="set UDP socket receiver buffer size for VR 2 [default=%default]")  
    vr2_options.add_option("-m", "--vr2-modulation", type="string", default="bpsk",
                      help="set modulation type (bpsk, qpsk, 8psk, qam{16,64}) [default=%default]")
    vr2_options.add_option("", "--vr2-fft-length", type="intx", default=512,
                      help="set the number of FFT bins [default=%default]")
    vr2_options.add_option("", "--vr2-occupied-tones", type="intx", default=200,
                      help="set the number of occupied FFT bins [default=%default]")
    vr2_options.add_option("", "--vr2-cp-length", type="intx", default=128,
                      help="set the number of bits in the cyclic prefix [default=%default]")

    expert_grp = parser.add_option_group("Expert")
    expert_grp.add_option("-s", "--size", type="eng_float", default=400,
                      help="set packet size [default=%default]")
    expert_grp.add_option("-M", "--megabytes", type="eng_float", default=1.0,
                      help="set megabytes to transmit [default=%default]")
    expert_grp.add_option("","--to-file", default=None,
                      help="Output file for modulated samples")
    uhd_transmitter.add_options(parser)

    (options, args) = parser.parse_args()

    # build the graph
    options_vr1 = dict2obj({'tx_amplitude': options.vr1_tx_amplitude,
			   'freq': options.vr1_freq,
			   'bandwidth': options.vr1_bandwidth,
                           'modulation': options.vr1_modulation,
                           'fft_length': options.vr1_fft_length,
                           'occupied_tones': options.vr1_occupied_tones,
                           'cp_length': options.vr1_cp_length,
                           'modulation': options.vr1_modulation,
			   'verbose': False,
			   'log': False})
    options_vr2 = dict2obj({'tx_amplitude': options.vr2_tx_amplitude,
			   'freq': options.vr2_freq,
			   'bandwidth': options.vr1_bandwidth,
                           'modulation': options.vr2_modulation,
                           'fft_length': options.vr2_fft_length,
                           'occupied_tones': options.vr2_occupied_tones,
                           'cp_length': options.vr2_cp_length,
                           'modulation': options.vr2_modulation,
			   'verbose': False,
			   'log': False})

    tb = my_top_block(options, options_vr1, options_vr2)

    r = gr.enable_realtime_scheduling()
    if r != gr.RT_OK:
        print "Warning: failed to enable realtime scheduling"

    tb.start()                       # start flow graph

    # generate and send packets
    nbytes = int(1e6 * options.megabytes)
    n = 0
    pktno = 0
    pkt_size = int(options.size)
    max_pkt_size = 3072

    # Qin 020609
    # create a socket
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    host = '' # can leave this blank on the server side
    try:
        s.bind((host, options.vr1_port))
    except socket.error, err:
        print "Could not set up a UDP server on port %d: %s" % (options.port, err)
        raise SystemExit

    f = open("/home/nodeuser/maicon/myfifo.raw", "rb")

    while True:
        # pdb.set_trace()
        # try:
        #data = s.recv(options.bufferbytes)

	data = f.read(max_pkt_size)
	#data = ''.join(['1' for x in range(2048)])


	# add error handling here 021609
        # except (KeyboardInterrupt, SystemExit):
        #    raise
        if not data:
            break

        # Qin 021609
        while (len(data) > max_pkt_size): # (maximum allower packet size: 4096)
            partial_pl = data[0:max_pkt_size]   # extract 3072 bytes
            # print "partial_pl 1 length = %4d" % (len(partial_pl))
            partial_pl = struct.pack('!H', 0x5555) + partial_pl
            # print "partial_pl 2 length = %4d" % (len(partial_pl))
            payload = struct.pack('!H', pktno & 0xffff) + partial_pl
            # print "partial_pl 3 length = %4d" % (len(payload))
            send_pkt(payload)
            n += len(payload)
            pktno += 1
            sys.stderr.write('*')   # "*" to denote split packet

            data = data[max_pkt_size:]  # update "data"

        data = struct.pack('!H', 0xaaaa) + data
        #payload = struct.pack('!H', pktno & 0xffff) + data
        payload = data
        send_pkt(payload)
        n += len(payload)
        sys.stderr.write('.')
        # if options.discontinuous and pktno % 5 == 4:
        # time.sleep(0.001) # Qin 021609
        pktno += 1

    send_pkt(eof=True)

    # close the socket
    s.close()
    tb.wait()                       # wait for it to finish


if __name__ == '__main__':
    try:
        main()
    except KeyboardInterrupt:
        pass
