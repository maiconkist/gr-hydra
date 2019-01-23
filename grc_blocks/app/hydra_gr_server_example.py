#!/usr/bin/env python2
# -*- coding: utf-8 -*-
##################################################
# GNU Radio Python Flow Graph
# Title: Hydra Gr Server Example
# Generated: Wed Jan 23 18:37:05 2019
##################################################


from gnuradio import eng_notation
from gnuradio import gr
from gnuradio.eng_option import eng_option
from gnuradio.filter import firdes
from optparse import OptionParser
import hydra
import threading


class hydra_gr_server_example(gr.top_block):

    def __init__(self, freqrx=1.1e9, freqtx=1.1e9, hydraServerIP='192.168.5.77:5000'):
        gr.top_block.__init__(self, "Hydra Gr Server Example")

        ##################################################
        # Parameters
        ##################################################
        self.freqrx = freqrx
        self.freqtx = freqtx
        self.hydraServerIP = hydraServerIP

        ##################################################
        # Blocks
        ##################################################
        self.ahydra_gr_server_0_0 = hydra.hydra_gr_server(hydraServerIP)
        if freqtx > 0 and 2e6 > 0 and 2048 > 0:
           self.ahydra_gr_server_0_0.set_tx_config(freqtx, 2e6, 2048, "USRP")
        if freqrx > 0 and 2e6 > 0 and 2048 > 0:
           self.ahydra_gr_server_0_0.set_rx_config(freqrx, 2e6, 2048, "USRP")
        self.ahydra_gr_server_0_0_thread = threading.Thread(target=self.ahydra_gr_server_0_0.start_server)
        self.ahydra_gr_server_0_0_thread.daemon = True
        self.ahydra_gr_server_0_0_thread.start()

    def get_freqrx(self):
        return self.freqrx

    def set_freqrx(self, freqrx):
        self.freqrx = freqrx

    def get_freqtx(self):
        return self.freqtx

    def set_freqtx(self, freqtx):
        self.freqtx = freqtx

    def get_hydraServerIP(self):
        return self.hydraServerIP

    def set_hydraServerIP(self, hydraServerIP):
        self.hydraServerIP = hydraServerIP


def argument_parser():
    parser = OptionParser(usage="%prog: [options]", option_class=eng_option)
    parser.add_option(
        "", "--freqrx", dest="freqrx", type="eng_float", default=eng_notation.num_to_str(1.1e9),
        help="Set freqrx [default=%default]")
    parser.add_option(
        "", "--freqtx", dest="freqtx", type="eng_float", default=eng_notation.num_to_str(1.1e9),
        help="Set freqtx [default=%default]")
    parser.add_option(
        "", "--hydraServerIP", dest="hydraServerIP", type="string", default='192.168.5.77:5000',
        help="Set hydraServerIP [default=%default]")
    return parser


def main(top_block_cls=hydra_gr_server_example, options=None):
    if options is None:
        options, _ = argument_parser().parse_args()

    tb = top_block_cls(freqrx=options.freqrx, freqtx=options.freqtx, hydraServerIP=options.hydraServerIP)
    tb.start()
    tb.wait()


if __name__ == '__main__':
    main()
