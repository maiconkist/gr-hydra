#!/usr/bin/env python2
# -*- coding: utf-8 -*-
##################################################
# GNU Radio Python Flow Graph
# Title: Hydra Gr Server Example
# Generated: Wed Aug 15 11:17:52 2018
##################################################


from gnuradio import eng_notation
from gnuradio import gr
from gnuradio.eng_option import eng_option
from gnuradio.filter import firdes
from optparse import OptionParser
import hydra
import threading


class hydra_gr_server_example(gr.top_block):

    def __init__(self):
        gr.top_block.__init__(self, "Hydra Gr Server Example")

        ##################################################
        # Blocks
        ##################################################
        self.ahydra_gr_server_0 = hydra.hydra_gr_server(5000)
        self.ahydra_gr_server_0.set_tx_config(950e6, 2e6, 1024, "USRP")
        self.ahydra_gr_server_0.set_rx_config(950e6, 2e6, 1024, "USRP")
        self.ahydra_gr_server_0_thread = threading.Thread(target=self.ahydra_gr_server_0.start_server)
        self.ahydra_gr_server_0_thread.daemon = True
        self.ahydra_gr_server_0_thread.start()


def main(top_block_cls=hydra_gr_server_example, options=None):

    tb = top_block_cls()
    tb.start()
    tb.wait()


if __name__ == '__main__':
    main()
