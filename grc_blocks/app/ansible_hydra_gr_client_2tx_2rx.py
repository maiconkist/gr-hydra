#!/usr/bin/env python2
# -*- coding: utf-8 -*-
##################################################
# GNU Radio Python Flow Graph
# Title: Ansible Hydra Gr Client 2Tx 2Rx
# Generated: Thu Nov 29 17:31:54 2018
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


class ansible_hydra_gr_client_2tx_2rx(gr.top_block):

    def __init__(self, freq=1.1e9, samp_rate=200e3):
        gr.top_block.__init__(self, "Ansible Hydra Gr Client 2Tx 2Rx")

        ##################################################
        # Parameters
        ##################################################
        self.freq = freq
        self.samp_rate = samp_rate

        ##################################################
        # Blocks
        ##################################################
        self.hydra_gr_sink_0_0 = hydra.hydra_gr_client_sink(2, 'hydraServerIP', 5000)
        self.hydra_gr_sink_0_0.start_client(freq + 700e3, samp_rate, 1024)
        self.hydra_gr_sink_0 = hydra.hydra_gr_client_sink(1, 'hydraServerIP', 5000)
        self.hydra_gr_sink_0.start_client(freq - 100e3, samp_rate * 2, 1024)
        self.hydra_gr__source_0_0_0 = hydra.hydra_gr_client_source(2, 'hydraClientIP', 'hydraServerIP', 5000)
        self.hydra_gr__source_0_0_0.start_client(freq + 700e3, samp_rate, 10000)

        self.hydra_gr__source_0_0 = hydra.hydra_gr_client_source(1, 'hydraClientIP', 'hydraServerIP', 5000)
        self.hydra_gr__source_0_0.start_client(freq - 100e3, samp_rate * 2, 10000)

        self.digital_ofdm_tx_0_0 = digital.ofdm_tx(
        	  fft_len=64, cp_len=16,
        	  packet_length_tag_key="len",
        	  bps_header=1,
        	  bps_payload=1,
        	  rolloff=0,
        	  debug_log=False,
        	  scramble_bits=False
        	 )
        self.digital_ofdm_tx_0 = digital.ofdm_tx(
        	  fft_len=64, cp_len=16,
        	  packet_length_tag_key="len",
        	  bps_header=1,
        	  bps_payload=1,
        	  rolloff=0,
        	  debug_log=False,
        	  scramble_bits=False
        	 )
        self.digital_ofdm_rx_0_0 = digital.ofdm_rx(
        	  fft_len=64, cp_len=16,
        	  frame_length_tag_key='frame_'+"len",
        	  packet_length_tag_key="len",
        	  bps_header=1,
        	  bps_payload=1,
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
        self.blocks_vector_source_x_0_0 = blocks.vector_source_b([x for x in range(0,128)], True, 1, [])
        self.blocks_vector_source_x_0 = blocks.vector_source_b([x for x in range(0,256)], True, 1, [])
        self.blocks_throttle_0_0 = blocks.throttle(gr.sizeof_gr_complex*1, samp_rate,True)
        self.blocks_throttle_0 = blocks.throttle(gr.sizeof_gr_complex*1, samp_rate * 2,True)
        self.blocks_tag_debug_0_0 = blocks.tag_debug(gr.sizeof_char*1, 'VR2 RX', ""); self.blocks_tag_debug_0_0.set_display(True)
        self.blocks_tag_debug_0 = blocks.tag_debug(gr.sizeof_char*1, 'VR1 RX', ""); self.blocks_tag_debug_0.set_display(True)
        self.blocks_stream_to_tagged_stream_0_0 = blocks.stream_to_tagged_stream(gr.sizeof_char, 1, 100, "len")
        self.blocks_stream_to_tagged_stream_0 = blocks.stream_to_tagged_stream(gr.sizeof_char, 1, 100, "len")
        self.blocks_null_sink_1 = blocks.null_sink(gr.sizeof_float*1)
        self.blocks_null_sink_0 = blocks.null_sink(gr.sizeof_float*1)
        self.blocks_multiply_const_vxx_0_0 = blocks.multiply_const_vcc((0.1, ))
        self.blocks_multiply_const_vxx_0 = blocks.multiply_const_vcc((0.1, ))
        self.blocks_char_to_float_0_0 = blocks.char_to_float(1, 1)
        self.blocks_char_to_float_0 = blocks.char_to_float(1, 1)

        ##################################################
        # Connections
        ##################################################
        self.connect((self.blocks_char_to_float_0, 0), (self.blocks_null_sink_1, 0))
        self.connect((self.blocks_char_to_float_0_0, 0), (self.blocks_null_sink_0, 0))
        self.connect((self.blocks_multiply_const_vxx_0, 0), (self.hydra_gr_sink_0, 0))
        self.connect((self.blocks_multiply_const_vxx_0_0, 0), (self.hydra_gr_sink_0_0, 0))
        self.connect((self.blocks_stream_to_tagged_stream_0, 0), (self.digital_ofdm_tx_0, 0))
        self.connect((self.blocks_stream_to_tagged_stream_0_0, 0), (self.digital_ofdm_tx_0_0, 0))
        self.connect((self.blocks_throttle_0, 0), (self.blocks_multiply_const_vxx_0, 0))
        self.connect((self.blocks_throttle_0_0, 0), (self.blocks_multiply_const_vxx_0_0, 0))
        self.connect((self.blocks_vector_source_x_0, 0), (self.blocks_stream_to_tagged_stream_0, 0))
        self.connect((self.blocks_vector_source_x_0_0, 0), (self.blocks_stream_to_tagged_stream_0_0, 0))
        self.connect((self.digital_ofdm_rx_0, 0), (self.blocks_char_to_float_0, 0))
        self.connect((self.digital_ofdm_rx_0, 0), (self.blocks_tag_debug_0, 0))
        self.connect((self.digital_ofdm_rx_0_0, 0), (self.blocks_char_to_float_0_0, 0))
        self.connect((self.digital_ofdm_rx_0_0, 0), (self.blocks_tag_debug_0_0, 0))
        self.connect((self.digital_ofdm_tx_0, 0), (self.blocks_throttle_0, 0))
        self.connect((self.digital_ofdm_tx_0_0, 0), (self.blocks_throttle_0_0, 0))
        self.connect((self.hydra_gr__source_0_0, 0), (self.digital_ofdm_rx_0, 0))
        self.connect((self.hydra_gr__source_0_0_0, 0), (self.digital_ofdm_rx_0_0, 0))

    def get_freq(self):
        return self.freq

    def set_freq(self, freq):
        self.freq = freq

    def get_samp_rate(self):
        return self.samp_rate

    def set_samp_rate(self, samp_rate):
        self.samp_rate = samp_rate
        self.blocks_throttle_0_0.set_sample_rate(self.samp_rate)
        self.blocks_throttle_0.set_sample_rate(self.samp_rate * 2)


def argument_parser():
    parser = OptionParser(usage="%prog: [options]", option_class=eng_option)
    return parser


def main(top_block_cls=ansible_hydra_gr_client_2tx_2rx, options=None):
    if options is None:
        options, _ = argument_parser().parse_args()

    tb = top_block_cls()
    tb.start()
    tb.wait()


if __name__ == '__main__':
    main()
