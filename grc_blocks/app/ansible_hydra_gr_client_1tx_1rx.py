#!/usr/bin/env python2
# -*- coding: utf-8 -*-
##################################################
# GNU Radio Python Flow Graph
# Title: Ansible Hydra Gr Client 1Tx 1Rx
# Generated: Fri Feb  8 12:19:38 2019
##################################################


from gnuradio import blocks
from gnuradio import digital
from gnuradio import eng_notation
from gnuradio import gr
from gnuradio.eng_option import eng_option
from gnuradio.filter import firdes
from optparse import OptionParser
import hydra
import threading


class ansible_hydra_gr_client_1tx_1rx(gr.top_block):

    def __init__(self, freqrx=1.2e9, freqtx=1.1e9, hydraClient='192.168.5.77', samp_rate=200e3, vr1offset=-300e3, vr2offset=700e3):
        gr.top_block.__init__(self, "Ansible Hydra Gr Client 1Tx 1Rx")

        ##################################################
        # Parameters
        ##################################################
        self.freqrx = freqrx
        self.freqtx = freqtx
        self.hydraClient = hydraClient
        self.samp_rate = samp_rate
        self.vr1offset = vr1offset
        self.vr2offset = vr2offset

        ##################################################
        # Blocks
        ##################################################
        self.hydra_gr_sink_0 = hydra.hydra_gr_client_sink(1, hydraClient, 5000)
        self.hydra_gr_sink_0.start_client(freqtx + vr1offset, samp_rate * 2, 1024)
        self.hydra_gr__source_0_0 = hydra.hydra_gr_client_source(1, hydraClient, hydraClient, 5000)
        self.hydra_gr__source_0_0.start_client(freqrx + vr1offset, samp_rate * 2, 10000)

        self.digital_ofdm_tx_0 = digital.ofdm_tx(
        	  fft_len=64, cp_len=16,
        	  packet_length_tag_key="len",
        	  bps_header=1,
        	  bps_payload=1,
        	  rolloff=0,
        	  debug_log=False,
        	  scramble_bits=False
        	 )
        self.digital_ofdm_rx_0 = digital.ofdm_rx(
        	  fft_len=64, cp_len=16,
        	  frame_length_tag_key='frame_'+"len",
        	  packet_length_tag_key="len",
        	  bps_header=1,
        	  bps_payload=1,
        	  debug_log=False,
        	  scramble_bits=False
        	 )
        self.blocks_tuntap_pdu_1 = blocks.tuntap_pdu('tap0', 10000, False)
        self.blocks_tagged_stream_to_pdu_0 = blocks.tagged_stream_to_pdu(blocks.byte_t, "len")
        self.blocks_tag_debug_0 = blocks.tag_debug(gr.sizeof_char*1, 'VR1 RX', ""); self.blocks_tag_debug_0.set_display(True)
        self.blocks_pdu_to_tagged_stream_0 = blocks.pdu_to_tagged_stream(blocks.byte_t, "len")
        self.blocks_multiply_const_vxx_0 = blocks.multiply_const_vcc((0.06, ))

        ##################################################
        # Connections
        ##################################################
        self.msg_connect((self.blocks_tagged_stream_to_pdu_0, 'pdus'), (self.blocks_tuntap_pdu_1, 'pdus'))
        self.msg_connect((self.blocks_tuntap_pdu_1, 'pdus'), (self.blocks_pdu_to_tagged_stream_0, 'pdus'))
        self.connect((self.blocks_multiply_const_vxx_0, 0), (self.hydra_gr_sink_0, 0))
        self.connect((self.blocks_pdu_to_tagged_stream_0, 0), (self.digital_ofdm_tx_0, 0))
        self.connect((self.digital_ofdm_rx_0, 0), (self.blocks_tag_debug_0, 0))
        self.connect((self.digital_ofdm_rx_0, 0), (self.blocks_tagged_stream_to_pdu_0, 0))
        self.connect((self.digital_ofdm_tx_0, 0), (self.blocks_multiply_const_vxx_0, 0))
        self.connect((self.hydra_gr__source_0_0, 0), (self.digital_ofdm_rx_0, 0))

    def get_freqrx(self):
        return self.freqrx

    def set_freqrx(self, freqrx):
        self.freqrx = freqrx

    def get_freqtx(self):
        return self.freqtx

    def set_freqtx(self, freqtx):
        self.freqtx = freqtx

    def get_hydraClient(self):
        return self.hydraClient

    def set_hydraClient(self, hydraClient):
        self.hydraClient = hydraClient

    def get_samp_rate(self):
        return self.samp_rate

    def set_samp_rate(self, samp_rate):
        self.samp_rate = samp_rate

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
        "", "--hydraClient", dest="hydraClient", type="string", default='192.168.5.77',
        help="Set hydraClient [default=%default]")
    return parser


def main(top_block_cls=ansible_hydra_gr_client_1tx_1rx, options=None):
    if options is None:
        options, _ = argument_parser().parse_args()

    tb = top_block_cls(hydraClient=options.hydraClient)
    tb.start()
    tb.wait()


if __name__ == '__main__':
    main()
