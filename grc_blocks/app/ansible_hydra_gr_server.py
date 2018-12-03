#!/usr/bin/env python2
# -*- coding: utf-8 -*-
##################################################
# GNU Radio Python Flow Graph
# Title: Ansible Hydra Gr Server
# Generated: Mon Dec  3 18:58:29 2018
##################################################


from gnuradio import eng_notation
from gnuradio import gr
from gnuradio.eng_option import eng_option
from gnuradio.filter import firdes
from optparse import OptionParser
import hydra
import threading


class ansible_hydra_gr_server(gr.top_block):

    def __init__(self, freqrx=1.2e9, freqtx=1.1e9):
        gr.top_block.__init__(self, "Ansible Hydra Gr Server")

        ##################################################
        # Parameters
        ##################################################
        self.freqrx = freqrx
        self.freqtx = freqtx

        ##################################################
        # Blocks
        ##################################################
        self.ahydra_gr_server_0 = hydra.hydra_gr_server(5000)
        self.ahydra_gr_server_0.set_tx_config(freqtx, 2e6, 2048, "USRP")
        self.ahydra_gr_server_0.set_rx_config(freqrx, 2e6, 1024, "USRP")
        self.ahydra_gr_server_0_thread = threading.Thread(target=self.ahydra_gr_server_0.start_server)
        self.ahydra_gr_server_0_thread.daemon = True
        self.ahydra_gr_server_0_thread.start()

    def get_freqrx(self):
        return self.freqrx

    def set_freqrx(self, freqrx):
        self.freqrx = freqrx

    def get_freqtx(self):
        return self.freqtx

    def set_freqtx(self, freqtx):
        self.freqtx = freqtx


def argument_parser():
    parser = OptionParser(usage="%prog: [options]", option_class=eng_option)
    parser.add_option(
        "", "--freqrx", dest="freqrx", type="eng_float", default=eng_notation.num_to_str(1.2e9),
        help="Set freqrx [default=%default]")
    parser.add_option(
        "", "--freqtx", dest="freqtx", type="eng_float", default=eng_notation.num_to_str(1.1e9),
        help="Set freqtx [default=%default]")
    return parser


def main(top_block_cls=ansible_hydra_gr_server, options=None):
    if options is None:
        options, _ = argument_parser().parse_args()

    tb = top_block_cls(freqrx=options.freqrx, freqtx=options.freqtx)
    tb.start()
    tb.wait()


if __name__ == '__main__':
    main()
