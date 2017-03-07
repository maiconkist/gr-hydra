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
    def __init__(self, options, options_vr1, options_vr2):
        gr.top_block.__init__(self)

        if (options.file_sink is False):
            print("Using USRP")
            self.sink = uhd_transmitter(options.args,
                                        options.bandwidth, options.tx_freq,
                                        options.lo_offset, options.tx_gain,
                                        options.spec, options.antenna,
                                        options.clock_source, options.verbose)
        else:
            self.sink = blocks.null_sink(gr.sizeof_gr_complex)


        vr_configs = []
        vr_configs.append([options_vr1.freq, options_vr1.bandwidth])
        vr_configs.append([options_vr2.freq, options_vr2.bandwidth])
        hydra_sink = hydra.hydra_sink(2,
                options.fft_length,
                int(options.tx_freq),
                int(options.bandwidth),
                vr_configs)

        self.vr1_source = zeromq.pull_source(gr.sizeof_gr_complex, 1, 'tcp://127.0.0.1:4000', 1000, False, -1)
        self.vr2_source = zeromq.pull_source(gr.sizeof_gr_complex, 1, 'tcp://127.0.0.1:4001', 100, False, -1)

        self.connect(self.vr1_source, (hydra_sink, 0), self.sink)
        self.connect(self.vr2_source, (hydra_sink, 1))
        self.hydra = hydra_sink

        self.xmlrpc_server = SimpleXMLRPCServer.SimpleXMLRPCServer(("localhost", 12345), allow_none=True)
        self.xmlrpc_server.register_instance(self)
        threading.Thread(target=self.xmlrpc_server.serve_forever).start()


    def set_vr1_gain(self, gain):
        print("called: set_vr1_gain")
        return self.txpath1.set_tx_amplitude(gain)

    def set_vr1_center_freq(self, cf):
        print("called: set_vr1_cf")
        return self.hydra.get_hypervisor().get_vradio(0).set_central_frequency(hydra_center_frequency+cf)

    def set_vr2_center_freq(self, cf):
        print("called: set_vr2_cf")
        return self.hydra.get_hypervisor().get_vradio(1).set_central_frequency(hydra_center_frequency+cf)

    def set_vr2_bandwidth(self, bandwidth):
        print("called: set_vr2_bandwidth")
        return self.hydra.get_hypervisor().get_vradio(1).set_bandwidth(bandwidth)

    def get_hydra_center_freq(self):
        print("called: get_hydra_center_freq")
        return self.sink.get_freq()

    def get_hydra_bandwidth(self):
        print("called: get_hydra_bandwidth")
        return self.sink.get_sample_rate()


# /////////////////////////////////////////////////////////////////////////////
#                                   main
# /////////////////////////////////////////////////////////////////////////////
def main():

    parser = OptionParser(option_class=eng_option, conflict_handler="resolve")

    hydra_options = parser.add_option_group("HyDRA Options")
    hydra_options.add_option("-2", "--one-virtual-radio",
            action="store_true", default=False, help="Run with ONE virtual radio instead [default=%default]")
    hydra_options.add_option("", "--file-sink",
            action="store_true", default=False, help="Do not use USRP as sink. Use file instead [default=%default]")
    hydra_options.add_option("", "--fft-length", type="intx", default=5120,
            help="HyDRA FFT M size [default=%default]")
    parser.add_option("", "--tx-freq", type="eng_float", default=hydra_center_frequency,
            help="Hydra transmit frequency [default=%default]", metavar="FREQ")
    parser.add_option("-W", "--bandwidth", type="eng_float", default=4e6,
            help="Hydra sample_rate [default=%default]")

    vr1_options = parser.add_option_group("VR 1 Options")
    vr1_options.add_option("", "--vr1-bandwidth", type="eng_float", default=1e6,
            help="set bandwidth for VR 1 [default=%default]")
    vr1_options.add_option("", "--vr1-freq", type="eng_float", default=hydra_center_frequency+vr1_initial_shift,
            help="set central frequency for VR 1 [default=%default]")

    vr2_options = parser.add_option_group("VR 2 Options")
    vr2_options.add_option("", "--vr2-bandwidth", type="eng_float", default=200e3,
            help="set bandwidth for VR 2 [default=%default]")
    vr2_options.add_option("", "--vr2-freq", type="eng_float", default=hydra_center_frequency + vr2_initial_shift,
            help="set central frequency for VR 2 [default=%default]")

    uhd_transmitter.add_options(parser)

    (options, args) = parser.parse_args()

    # build the graph
    options_vr1 = dict2obj({'freq': options.vr1_freq,
                            'bandwidth': options.vr1_bandwidth})
    options_vr2 = dict2obj({'freq': options.vr2_freq,
                            'bandwidth': options.vr2_bandwidth})

    r = gr.enable_realtime_scheduling()
    if r != gr.RT_OK:
        print("Warning: failed to enable realtime scheduling")

    tb = my_top_block(options, options_vr1, options_vr2)
    tb.start()                       # start flow graph

    tb.wait()                       # wait for it to finish

if __name__ == '__main__':
    try:
       main()
    except KeyboardInterrupt:
       tb.xmlrpc_server.stop()
       tb.stop()
