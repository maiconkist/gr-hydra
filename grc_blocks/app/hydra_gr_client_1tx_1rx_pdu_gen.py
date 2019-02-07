#!/usr/bin/env python2
# -*- coding: utf-8 -*-
##################################################
# GNU Radio Python Flow Graph
# Title: Hydra Gr Client 1Tx 1Rx Pdu Gen
# Generated: Thu Feb  7 10:50:01 2019
##################################################


from gnuradio import blocks
from gnuradio import digital
from gnuradio import eng_notation
from gnuradio import gr
from gnuradio.eng_option import eng_option
from gnuradio.filter import firdes
from optparse import OptionParser
import hydra
import pmt
import threading


class hydra_gr_client_1tx_1rx_pdu_gen(gr.top_block):

    def __init__(self, freqrx=1.1e9, freqtx=1.1e9, hydraClient='192.168.5.77', samp_rate=200e3, vr1offset=-300e3, vr2offset=700e3):
        gr.top_block.__init__(self, "Hydra Gr Client 1Tx 1Rx Pdu Gen")

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
        # Variables
        ##################################################
        self.sync_word2 = sync_word2 = [0, 0, 0, 0, 0, 0, -1, -1, -1, -1, 1, 1, -1, -1, -1, 1, -1, 1, 1, 1, 1, 1, -1, -1, -1, -1, -1, 1, -1, -1, 1, -1, 0, 1, -1, 1, 1, 1, -1, 1, 1, 1, -1, 1, 1, 1, 1, -1, 1, -1, -1, -1, 1, -1, 1, -1, -1, -1, -1, 0, 0, 0, 0, 0]
        self.sync_word1 = sync_word1 = [0., 0., 0., 0., 0., 0., 0., 1.41421356, 0., -1.41421356, 0., 1.41421356, 0., -1.41421356, 0., -1.41421356, 0., -1.41421356, 0., 1.41421356, 0., -1.41421356, 0., 1.41421356, 0., -1.41421356, 0., -1.41421356, 0., -1.41421356, 0., -1.41421356, 0., 1.41421356, 0., -1.41421356, 0., 1.41421356, 0., 1.41421356, 0., 1.41421356, 0., -1.41421356, 0., 1.41421356, 0., 1.41421356, 0., 1.41421356, 0., -1.41421356, 0., 1.41421356, 0., 1.41421356, 0., 1.41421356, 0., 0., 0., 0., 0., 0.]
        self.pilot_symbols = pilot_symbols = ((1, 1, 1, -1,),)
        self.pilot_carriers = pilot_carriers = ((-21, -7, 7, 21,),)
        self.occupied_carriers = occupied_carriers = (range(-26, -21) + range(-20, -7) + range(-6, 0) + range(1, 7) + range(8, 21) + range(22, 27),)

        ##################################################
        # Blocks
        ##################################################
        self.hydra_gr_sink_0 = hydra.hydra_gr_client_sink(1, hydraClient, 5000)
        self.hydra_gr_sink_0.start_client(freqtx + vr1offset, samp_rate * 2, 1000)
        self.hydra_gr__source_0_0 = hydra.hydra_gr_client_source(1, hydraClient, hydraClient, 5000)
        self.hydra_gr__source_0_0.start_client(freqtx + vr1offset, samp_rate * 2, 5000)

        self.digital_ofdm_tx_0 = digital.ofdm_tx(
        	  fft_len=64, cp_len=16,
        	  packet_length_tag_key="len" ,
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
        self.blocks_tag_debug_0 = blocks.tag_debug(gr.sizeof_char*1, 'VR1 RX', ""); self.blocks_tag_debug_0.set_display(True)
        self.blocks_random_pdu_0 = blocks.random_pdu(50, 100, chr(0xFF), 2)
        self.blocks_pdu_to_tagged_stream_0 = blocks.pdu_to_tagged_stream(blocks.byte_t, "len")
        self.blocks_multiply_const_vxx_0 = blocks.multiply_const_vcc((0.06, ))
        self.blocks_message_strobe_0 = blocks.message_strobe(pmt.intern("TEST"), 1000)

        ##################################################
        # Connections
        ##################################################
        self.msg_connect((self.blocks_message_strobe_0, 'strobe'), (self.blocks_random_pdu_0, 'generate'))
        self.msg_connect((self.blocks_random_pdu_0, 'pdus'), (self.blocks_pdu_to_tagged_stream_0, 'pdus'))
        self.connect((self.blocks_multiply_const_vxx_0, 0), (self.hydra_gr_sink_0, 0))
        self.connect((self.blocks_pdu_to_tagged_stream_0, 0), (self.digital_ofdm_tx_0, 0))
        self.connect((self.digital_ofdm_rx_0, 0), (self.blocks_tag_debug_0, 0))
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

    def get_sync_word2(self):
        return self.sync_word2

    def set_sync_word2(self, sync_word2):
        self.sync_word2 = sync_word2

    def get_sync_word1(self):
        return self.sync_word1

    def set_sync_word1(self, sync_word1):
        self.sync_word1 = sync_word1

    def get_pilot_symbols(self):
        return self.pilot_symbols

    def set_pilot_symbols(self, pilot_symbols):
        self.pilot_symbols = pilot_symbols

    def get_pilot_carriers(self):
        return self.pilot_carriers

    def set_pilot_carriers(self, pilot_carriers):
        self.pilot_carriers = pilot_carriers

    def get_occupied_carriers(self):
        return self.occupied_carriers

    def set_occupied_carriers(self, occupied_carriers):
        self.occupied_carriers = occupied_carriers


def argument_parser():
    parser = OptionParser(usage="%prog: [options]", option_class=eng_option)
    parser.add_option(
        "", "--hydraClient", dest="hydraClient", type="string", default='192.168.5.77',
        help="Set hydraClient [default=%default]")
    return parser


def main(top_block_cls=hydra_gr_client_1tx_1rx_pdu_gen, options=None):
    if options is None:
        options, _ = argument_parser().parse_args()

    tb = top_block_cls(hydraClient=options.hydraClient)
    tb.start()
    tb.wait()


if __name__ == '__main__':
    main()
