#!/usr/bin/env python3
# -*- coding: utf-8 -*-

#
# SPDX-License-Identifier: GPL-3.0
#
# GNU Radio Python Flow Graph
# Title: Ansible Hydra Gr Server
# GNU Radio version: 3.10.9.2

from gnuradio import gr
from gnuradio.filter import firdes
from gnuradio.fft import window
import sys
import signal
from argparse import ArgumentParser
from gnuradio.eng_arg import eng_float, intx
from gnuradio import eng_notation
import threading
import hydra




class ansible_hydra_gr_server(gr.top_block):

    def __init__(self, ansibleIPPort='192.168.5.251:5000', freqrx=(1.1e9+5e6), freqtx=1.1e9):
        gr.top_block.__init__(self, "Ansible Hydra Gr Server", catch_exceptions=True)

        ##################################################
        # Parameters
        ##################################################
        self.ansibleIPPort = ansibleIPPort
        self.freqrx = freqrx
        self.freqtx = freqtx

        ##################################################
        # Blocks
        ##################################################

        self.ahydra_gr_server_0 = hydra.hydra_gr_server(ansibleIPPort, 'default')
        if freqtx > 0 and 2e6 > 0 and 2048 > 0 and 0.6 >= 0 and 0.6 >= 0:
           self.ahydra_gr_server_0.set_tx_config(freqtx, 2e6, 0.6,2048, "USRP")
        if freqrx > 0 and 2e6 > 0 and 2048 > 0 and 0.0 >= 0 and 0.0 >= 0:
           self.ahydra_gr_server_0.set_rx_config(freqrx, 2e6, 0.0, 2048, "USRP")
        self.ahydra_gr_server_0_thread = threading.Thread(target=self.ahydra_gr_server_0.start_server)
        self.ahydra_gr_server_0_thread.daemon = True
        self.ahydra_gr_server_0_thread.start()



    def get_ansibleIPPort(self):
        return self.ansibleIPPort

    def set_ansibleIPPort(self, ansibleIPPort):
        self.ansibleIPPort = ansibleIPPort

    def get_freqrx(self):
        return self.freqrx

    def set_freqrx(self, freqrx):
        self.freqrx = freqrx

    def get_freqtx(self):
        return self.freqtx

    def set_freqtx(self, freqtx):
        self.freqtx = freqtx



def argument_parser():
    parser = ArgumentParser()
    parser.add_argument(
        "--ansibleIPPort", dest="ansibleIPPort", type=str, default='192.168.5.251:5000',
        help="Set ansibleIPPort [default=%(default)r]")
    parser.add_argument(
        "--freqrx", dest="freqrx", type=eng_float, default=eng_notation.num_to_str(float((1.1e9+5e6))),
        help="Set freqrx [default=%(default)r]")
    parser.add_argument(
        "--freqtx", dest="freqtx", type=eng_float, default=eng_notation.num_to_str(float(1.1e9)),
        help="Set freqtx [default=%(default)r]")
    return parser


def main(top_block_cls=ansible_hydra_gr_server, options=None):
    if options is None:
        options = argument_parser().parse_args()
    tb = top_block_cls(ansibleIPPort=options.ansibleIPPort, freqrx=options.freqrx, freqtx=options.freqtx)

    def sig_handler(sig=None, frame=None):
        tb.stop()
        tb.wait()

        sys.exit(0)

    signal.signal(signal.SIGINT, sig_handler)
    signal.signal(signal.SIGTERM, sig_handler)

    tb.start()

    tb.wait()


if __name__ == '__main__':
    main()
