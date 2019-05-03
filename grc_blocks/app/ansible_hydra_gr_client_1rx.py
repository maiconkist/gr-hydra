#!/usr/bin/env python2
# -*- coding: utf-8 -*-
##################################################
# GNU Radio Python Flow Graph
# Title: Ansible Hydra Gr Client 1Rx
# Generated: Thu Apr 11 17:27:14 2019
##################################################

from gnuradio import blocks
from gnuradio import digital
from gnuradio import eng_notation
from gnuradio import gr
from gnuradio.eng_option import eng_option
from gnuradio.filter import firdes
from optparse import OptionParser
import hydra


class ansible_hydra_gr_client_1rx(gr.top_block):

    def __init__(self, ansibleIP='127.0.0.1',
                 freqrx=2e9, freqtx=2e9, mul=0.01,
                 tx_rate=200e3, rx_rate=200e3,
                 vr1offset=-300e3, vr2offset=700e3):

        gr.top_block.__init__(self, "Ansible Hydra Gr Client 1Rx")

        ##################################################
        # Parameters
        ##################################################
        self.ansibleIP = ansibleIP
        self.freqrx = freqrx
        self.freqtx = freqtx
        self.mul = mul
        self.rx_rate = rx_rate
        self.vr1offset = vr1offset
        self.vr2offset = vr2offset

        ##################################################
        # Blocks
        ##################################################
        self.hydra_gr__source_0_0 = hydra.hydra_gr_client_source(1, ansibleIP, ansibleIP, 5000)
        self.hydra_gr__source_0_0.start_client(freqrx, rx_rate, 10000)

        self.digital_ofdm_rx_0 = digital.ofdm_rx(
        	  fft_len=64, cp_len=16,
        	  frame_length_tag_key='frame_'+"len",
        	  packet_length_tag_key="len",
        	  bps_header=1,
        	  bps_payload=1,
        	  debug_log=False,
        	  scramble_bits=False
        	 )
        self.blocks_tuntap_pdu_1 = blocks.tuntap_pdu('tap0', 1500, False)
        (self.blocks_tuntap_pdu_1).set_max_output_buffer(100000)
        self.blocks_tagged_stream_to_pdu_0 = blocks.tagged_stream_to_pdu(blocks.byte_t, "len")
        self.blocks_tag_debug_0 = blocks.tag_debug(gr.sizeof_char*1, 'VR1 RX', ""); self.blocks_tag_debug_0.set_display(True)

        ##################################################
        # Connections
        ##################################################
        self.msg_connect((self.blocks_tagged_stream_to_pdu_0, 'pdus'), (self.blocks_tuntap_pdu_1, 'pdus'))
        self.connect((self.digital_ofdm_rx_0, 0), (self.blocks_tag_debug_0, 0))
        self.connect((self.digital_ofdm_rx_0, 0), (self.blocks_tagged_stream_to_pdu_0, 0))
        self.connect((self.hydra_gr__source_0_0, 0), (self.digital_ofdm_rx_0, 0))

    def get_ansibleIP(self):
        return self.ansibleIP

    def set_ansibleIP(self, ansibleIP):
        self.ansibleIP = ansibleIP

    def get_freqrx(self):
        return self.freqrx

    def set_freqrx(self, freqrx):
        self.freqrx = freqrx

    def get_freqtx(self):
        return self.freqtx

    def set_freqtx(self, freqtx):
        self.freqtx = freqtx

    def get_mul(self):
        return self.mul

    def set_mul(self, mul):
        self.mul = mul

    def get_samp_rate(self):
        return self.rx_rate

    def set_samp_rate(self, samp_rate):
        self.samp_rate = rx_rate

    def get_vr1offset(self):
        return self.vr1offset

    def set_vr1offset(self, vr1offset):
        self.vr1offset = vr1offset

    def get_vr2offset(self):
        return self.vr2offset

    def set_vr2offset(self, vr2offset):
        self.vr2offset = vr2offset


def argument_parser():
    parser = OptionParser(usage="%prog: [options]", option_class=eng_option)
    parser.add_option(
        "", "--ansibleIP", dest="ansibleIP", type=str, default='127.0.0.1',
        help="Set ansibleIP [default=%default]")
    parser.add_option(
            "", "--tx_rate", dest="tx_rate", type=float, default=200e3,
            help="TX Rate [default=%default]")
    parser.add_option(
            "", "--rx_rate", dest="rx_rate", type=float, default=200e3,
            help="RX Rate [default=%default]")



    return parser


def main(top_block_cls=ansible_hydra_gr_client_1rx, options=None):
    if options is None:
        options, _ = argument_parser().parse_args()

    tb = top_block_cls(ansibleIP=options.ansibleIP,
                       tx_rate=options.tx_rate,
                       rx_rate=options.rx_rate)
    tb.start()
    try:
        raw_input('Press Enter to quit: ')
    except EOFError:
        pass
    tb.stop()
    tb.wait()


if __name__ == '__main__':
    main()
