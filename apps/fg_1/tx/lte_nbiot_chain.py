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
from gnuradio import zeromq
import hydra

# parse options
from optparse import OptionParser

# Server for remote commands
import SimpleXMLRPCServer
import threading

# from current dir
from transmit_path import TransmitPath, ReadThread, XMLRPCThread
from uhd_interface import uhd_transmitter

hydra_center_frequency = 5.5e9
vr1_initial_shift = -500e3
vr2_initial_shift =  400e3


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
    def __init__(self, options):
        gr.top_block.__init__(self)

        # do this after for any adjustments to the options that may
        # occur in the sinks (specifically the UHD sink)
        self.txpath = TransmitPath(options)

        self.sink   = zeromq.push_sink(gr.sizeof_gr_complex, 1, options.next_container_ip, 100, False, -1)

        self.connect(self.txpath, self.sink)

    def set_gain(self, gain):
        print("called: set_gain")
        return self.txpath.set_tx_amplitude(gain)

# /////////////////////////////////////////////////////////////////////////////
#                                   main
# /////////////////////////////////////////////////////////////////////////////
def main():

    parser = OptionParser(option_class=eng_option, conflict_handler="resolve")

    global_options = parser.add_option_group("GLOBAL Option")
    global_options.add_option("", "--vr-configuration", type="intx", default=1,
            help="Selct the air-interface to execute [1=LTE, 2=NB-IoT] [default=%default]")

    global_options = parser.add_option_group("Host Options")
    global_options.add_option("", "--host-ip", type="string", default="localhost",
            help="This host command interface IP address [default=%default]")

    vr1_options = parser.add_option_group("VR 1 Options")
    vr1_options.add_option("", "--vr1-next-container-ip", type="string", default="tcp://127.0.0.1:4000",
            help="IP:PORT of the next container if the processing chain [default=%default]")
    vr1_options.add_option("", "--vr1-bandwidth", type="eng_float", default=1e6,
            help="set bandwidth for VR 1 [default=%default]")
    vr1_options.add_option("", "--vr1-freq", type="eng_float", default=hydra_center_frequency+vr1_initial_shift,
            help="set central frequency for VR 1 [default=%default]")
    vr1_options.add_option("", "--vr1-tx-amplitude", type="eng_float", default=0.1, metavar="AMPL",
            help="set transmitter digital amplitude: 0 <= AMPL < 1.0 [default=%default]")
    vr1_options.add_option("", "--vr1-file", type="string", default='/home/ctvr/.wishful/radio/vr1fifo',
            help="set the file to obtain data [default=%default]")
    vr1_options.add_option("", "--vr1-buffersize", type="intx", default=3072,
            help="set number of bytes to read from buffer size for VR1 [default=%default]")
    vr1_options.add_option("-m", "--vr1-modulation", type="string", default="qpsk",
            help="set modulation type (bpsk, qpsk, 8psk, qam{16,64}) [default=%default]")
    vr1_options.add_option("", "--vr1-fft-length", type="intx", default=1024,
            help="set the number of FFT bins [default=%default]")
    vr1_options.add_option("", "--vr1-occupied-tones", type="intx", default=800,
            help="set the number of occupied FFT bins [default=%default]")
    vr1_options.add_option("", "--vr1-cp-length", type="intx", default=4,
            help="set the number of bits in the cyclic prefix [default=%default]")

    vr2_options = parser.add_option_group("VR 2 Options")
    vr2_options.add_option("", "--vr2-next-container-ip", type="string", default="tcp://127.0.0.1:4001",
            help="IP:PORT of the next container if the processing chain [default=%default]")
    vr2_options.add_option("", "--vr2-bandwidth", type="eng_float", default=200e3,
            help="set bandwidth for VR 2 [default=%default]")
    vr2_options.add_option("", "--vr2-freq", type="eng_float", default=hydra_center_frequency + vr2_initial_shift,
            help="set central frequency for VR 2 [default=%default]")
    vr2_options.add_option("", "--vr2-tx-amplitude", type="eng_float", default=0.125, metavar="AMPL",
            help="set transmitter digital amplitude: 0 <= AMPL < 1.0 [default=%default]")
    vr2_options.add_option("", "--vr2-file", type="string", default='/home/ctvr/.wishful/radio/vr2fifo',
            help="set the file to obtain data [default=%default]")
    vr2_options.add_option("", "--vr2-buffersize", type="intx", default=3072,
            help="set number of bytes to read from buffer size for VR2 [default=%default]")
    vr2_options.add_option("-m", "--vr2-modulation", type="string", default="bpsk",
            help="set modulation type (bpsk, qpsk, 8psk, qam{16,64}) [default=%default]")
    vr2_options.add_option("", "--vr2-fft-length", type="intx", default=64,
            help="set the number of FFT bins [default=%default]")
    vr2_options.add_option("", "--vr2-occupied-tones", type="intx", default=48,
            help="set the number of occupied FFT bins [default=%default]")
    vr2_options.add_option("", "--vr2-cp-length", type="intx", default=2,
            help="set the number of bits in the cyclic prefix [default=%default]")
    uhd_transmitter.add_options(parser)

    (options, args) = parser.parse_args()

    # build the graph
    options_vr1 = dict2obj({'tx_amplitude': options.vr1_tx_amplitude,
                            'next_container_ip': options.vr1_next_container_ip,
                            'freq': options.vr1_freq,
                            'bandwidth': options.vr1_bandwidth,
                            'file': options.vr1_file,
                            'buffersize': options.vr1_buffersize,
                            'modulation': options.vr1_modulation,
                            'fft_length': options.vr1_fft_length,
                            'occupied_tones': options.vr1_occupied_tones,
                            'cp_length': options.vr1_cp_length,
                            'modulation': options.vr1_modulation,
                            'verbose': False,
                            'log': False})
    options_vr2 = dict2obj({'tx_amplitude': options.vr2_tx_amplitude,
                            'next_container_ip': options.vr2_next_container_ip,
                            'freq': options.vr2_freq,
                            'bandwidth': options.vr2_bandwidth,
                            'file': options.vr2_file,
                            'buffersize': options.vr2_buffersize,
                            'modulation': options.vr2_modulation,
                            'fft_length': options.vr2_fft_length,
                            'occupied_tones': options.vr2_occupied_tones,
                            'cp_length': options.vr2_cp_length,
                            'modulation': options.vr2_modulation,
                            'verbose': False,
                            'log': False})

    r = gr.enable_realtime_scheduling()
    if r != gr.RT_OK:
        print("Warning: failed to enable realtime scheduling")

    tb = my_top_block(options_vr1 if options.vr_configuration == 1 else options_vr2)
    tb.start() # start flow graph

    if options.vr_configuration == 1:
        data_thread = ReadThread(options_vr1.file, options_vr1.buffersize, tb.txpath)
    else:
        data_thread = ReadThread(options_vr2.file, options_vr2.buffersize, tb.txpath)
    data_thread.start()

    tb.wait() # wait for it to finish

if __name__ == '__main__':
    try:
       main()
    except KeyboardInterrupt:
       data_thread._run = False
       tb.xmlrpc_server.stop()
       tb.stop()
