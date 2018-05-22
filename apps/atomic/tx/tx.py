#!/usr/bin/env python2
# -*- coding: utf-8 -*-
##################################################
# GNU Radio Python Flow Graph
# Title: Tx
# Generated: Tue May 22 18:42:29 2018
##################################################

from gnuradio import blocks
from gnuradio import digital
from gnuradio import eng_notation
from gnuradio import gr
from gnuradio import uhd
from gnuradio.eng_option import eng_option
from gnuradio.filter import firdes
from optparse import OptionParser
import ConfigParser
import SimpleXMLRPCServer
import hydra
import numpy
import psutil
import threading
import time


class tx(gr.top_block):

    def __init__(self, amplitude1=0.1, amplitude2=0.1):
        gr.top_block.__init__(self, "Tx")

        ##################################################
        # Parameters
        ##################################################
        self.amplitude1 = amplitude1
        self.amplitude2 = amplitude2

        ##################################################
        # Variables
        ##################################################
        self.zcpu = zcpu = psutil
        self._txfreq_config = ConfigParser.ConfigParser()
        self._txfreq_config.read('./default')
        try: txfreq = self._txfreq_config.getfloat("usrp_hydra", "txfreq")
        except: txfreq = 950e6
        self.txfreq = txfreq
        self._samprate2_config = ConfigParser.ConfigParser()
        self._samprate2_config.read('./default')
        try: samprate2 = self._samprate2_config.getfloat("usrp_hydra", "samprate2")
        except: samprate2 = 200e3
        self.samprate2 = samprate2
        self._samprate1_config = ConfigParser.ConfigParser()
        self._samprate1_config.read('./default')
        try: samprate1 = self._samprate1_config.getfloat("usrp_hydra", "samprate1")
        except: samprate1 = 500e3
        self.samprate1 = samprate1
        self._samprate_config = ConfigParser.ConfigParser()
        self._samprate_config.read('./default')
        try: samprate = self._samprate_config.getfloat("usrp_hydra", "samprate")
        except: samprate = 2e6
        self.samprate = samprate
        self.packet_len = packet_len = 100
        self.nbiot_tx_goodput = nbiot_tx_goodput = 0
        self.nbiot_rate = nbiot_rate = 0
        self.lte_tx_goodput = lte_tx_goodput = 0
        self.lte_rate = lte_rate = 0
        self._freq2_config = ConfigParser.ConfigParser()
        self._freq2_config.read('./default')
        try: freq2 = self._freq2_config.getfloat("usrp_hydra", "txfreq2")
        except: freq2 = 950.5e6
        self.freq2 = freq2
        self._freq1_config = ConfigParser.ConfigParser()
        self._freq1_config.read('./default')
        try: freq1 = self._freq1_config.getfloat("usrp_hydra", "txfreq1")
        except: freq1 = 949.5e6
        self.freq1 = freq1
        self.cpu_percent = cpu_percent = 0

        ##################################################
        # Blocks
        ##################################################
        self.znbiotrate = blocks.probe_rate(gr.sizeof_char*1, 2000, 0.15)
        self.zlterate = blocks.probe_rate(gr.sizeof_char*1, 2000, 0.15)
        self.zznbiot_rate = blocks.probe_rate(gr.sizeof_gr_complex*1, 500.0, 0.15)
        self.zzlte_rate = blocks.probe_rate(gr.sizeof_gr_complex*1, 500.0, 0.15)
        self.xmlrpc_server_0_0 = SimpleXMLRPCServer.SimpleXMLRPCServer(('localhost', 1235), allow_none=True)
        self.xmlrpc_server_0_0.register_instance(self)
        self.xmlrpc_server_0_0_thread = threading.Thread(target=self.xmlrpc_server_0_0.serve_forever)
        self.xmlrpc_server_0_0_thread.daemon = True
        self.xmlrpc_server_0_0_thread.start()
        self.uhd_usrp_sink_0 = uhd.usrp_sink(
        	",".join(("", "")),
        	uhd.stream_args(
        		cpu_format="fc32",
        		channels=range(1),
        	),
        )
        self.uhd_usrp_sink_0.set_samp_rate(samprate)
        self.uhd_usrp_sink_0.set_center_freq(txfreq, 0)
        self.uhd_usrp_sink_0.set_normalized_gain(0.9, 0)
        self.uhd_usrp_sink_0.set_antenna('TX/RX', 0)

        def _nbiot_tx_goodput_probe():
            while True:
                val = self.znbiotrate.rate()
                try:
                    self.set_nbiot_tx_goodput(val)
                except AttributeError:
                    pass
                time.sleep(1.0 / (10))
        _nbiot_tx_goodput_thread = threading.Thread(target=_nbiot_tx_goodput_probe)
        _nbiot_tx_goodput_thread.daemon = True
        _nbiot_tx_goodput_thread.start()


        def _nbiot_rate_probe():
            while True:
                val = self.zznbiot_rate.rate()
                try:
                    self.set_nbiot_rate(val)
                except AttributeError:
                    pass
                time.sleep(1.0 / (1))
        _nbiot_rate_thread = threading.Thread(target=_nbiot_rate_probe)
        _nbiot_rate_thread.daemon = True
        _nbiot_rate_thread.start()


        def _lte_tx_goodput_probe():
            while True:
                val = self.zlterate.rate()
                try:
                    self.set_lte_tx_goodput(val)
                except AttributeError:
                    pass
                time.sleep(1.0 / (10))
        _lte_tx_goodput_thread = threading.Thread(target=_lte_tx_goodput_probe)
        _lte_tx_goodput_thread.daemon = True
        _lte_tx_goodput_thread.start()


        def _lte_rate_probe():
            while True:
                val = self.zzlte_rate.rate()
                try:
                    self.set_lte_rate(val)
                except AttributeError:
                    pass
                time.sleep(1.0 / (1))
        _lte_rate_thread = threading.Thread(target=_lte_rate_probe)
        _lte_rate_thread.daemon = True
        _lte_rate_thread.start()

        self.hydra_hydra_sink_0 = hydra.hydra_sink(2, 1024, txfreq, samprate,
        	 ((freq1, samprate1),
        	 (freq2, samprate2),
        	 ))

        self.digital_ofdm_tx_0_0_0 = digital.ofdm_tx(
        	  fft_len=64, cp_len=16,
        	  packet_length_tag_key='length',
        	  bps_header=1,
        	  bps_payload=1,
        	  rolloff=0,
        	  debug_log=False,
        	  scramble_bits=False
        	 )
        self.digital_ofdm_tx_0_0 = digital.ofdm_tx(
        	  fft_len=64, cp_len=16,
        	  packet_length_tag_key='length',
        	  bps_header=1,
        	  bps_payload=1,
        	  rolloff=0,
        	  debug_log=False,
        	  scramble_bits=False
        	 )

        def _cpu_percent_probe():
            while True:
                val = self.zcpu.cpu_percent()
                try:
                    self.set_cpu_percent(val)
                except AttributeError:
                    pass
                time.sleep(1.0 / (1))
        _cpu_percent_thread = threading.Thread(target=_cpu_percent_probe)
        _cpu_percent_thread.daemon = True
        _cpu_percent_thread.start()

        self.blocks_stream_to_tagged_stream_0_0 = blocks.stream_to_tagged_stream(gr.sizeof_char, 1, packet_len, "length")
        self.blocks_stream_to_tagged_stream_0 = blocks.stream_to_tagged_stream(gr.sizeof_char, 1, packet_len, "length")
        self.blocks_multiply_const_vxx_0_0 = blocks.multiply_const_vcc((amplitude2, ))
        self.blocks_multiply_const_vxx_0 = blocks.multiply_const_vcc((amplitude1, ))
        self.analog_random_source_x_0_0 = blocks.vector_source_b(map(int, numpy.random.randint(0, 255, 1000)), True)
        self.analog_random_source_x_0 = blocks.vector_source_b(map(int, numpy.random.randint(0, 255, 1000)), True)

        ##################################################
        # Connections
        ##################################################
        self.connect((self.analog_random_source_x_0, 0), (self.blocks_stream_to_tagged_stream_0, 0))
        self.connect((self.analog_random_source_x_0_0, 0), (self.blocks_stream_to_tagged_stream_0_0, 0))
        self.connect((self.blocks_multiply_const_vxx_0, 0), (self.hydra_hydra_sink_0, 0))
        self.connect((self.blocks_multiply_const_vxx_0, 0), (self.zzlte_rate, 0))
        self.connect((self.blocks_multiply_const_vxx_0_0, 0), (self.hydra_hydra_sink_0, 1))
        self.connect((self.blocks_multiply_const_vxx_0_0, 0), (self.zznbiot_rate, 0))
        self.connect((self.blocks_stream_to_tagged_stream_0, 0), (self.digital_ofdm_tx_0_0_0, 0))
        self.connect((self.blocks_stream_to_tagged_stream_0, 0), (self.zlterate, 0))
        self.connect((self.blocks_stream_to_tagged_stream_0_0, 0), (self.digital_ofdm_tx_0_0, 0))
        self.connect((self.blocks_stream_to_tagged_stream_0_0, 0), (self.znbiotrate, 0))
        self.connect((self.digital_ofdm_tx_0_0, 0), (self.blocks_multiply_const_vxx_0_0, 0))
        self.connect((self.digital_ofdm_tx_0_0_0, 0), (self.blocks_multiply_const_vxx_0, 0))
        self.connect((self.hydra_hydra_sink_0, 0), (self.uhd_usrp_sink_0, 0))

    def get_amplitude1(self):
        return self.amplitude1

    def set_amplitude1(self, amplitude1):
        self.amplitude1 = amplitude1
        self.blocks_multiply_const_vxx_0.set_k((self.amplitude1, ))

    def get_amplitude2(self):
        return self.amplitude2

    def set_amplitude2(self, amplitude2):
        self.amplitude2 = amplitude2
        self.blocks_multiply_const_vxx_0_0.set_k((self.amplitude2, ))

    def get_zcpu(self):
        return self.zcpu

    def set_zcpu(self, zcpu):
        self.zcpu = zcpu

    def get_txfreq(self):
        return self.txfreq

    def set_txfreq(self, txfreq):
        self.txfreq = txfreq
        self.uhd_usrp_sink_0.set_center_freq(self.txfreq, 0)

    def get_samprate2(self):
        return self.samprate2

    def set_samprate2(self, samprate2):
        self.samprate2 = samprate2

    def get_samprate1(self):
        return self.samprate1

    def set_samprate1(self, samprate1):
        self.samprate1 = samprate1

    def get_samprate(self):
        return self.samprate

    def set_samprate(self, samprate):
        self.samprate = samprate
        self.uhd_usrp_sink_0.set_samp_rate(self.samprate)

    def get_packet_len(self):
        return self.packet_len

    def set_packet_len(self, packet_len):
        self.packet_len = packet_len
        self.blocks_stream_to_tagged_stream_0_0.set_packet_len(self.packet_len)
        self.blocks_stream_to_tagged_stream_0_0.set_packet_len_pmt(self.packet_len)
        self.blocks_stream_to_tagged_stream_0.set_packet_len(self.packet_len)
        self.blocks_stream_to_tagged_stream_0.set_packet_len_pmt(self.packet_len)

    def get_nbiot_tx_goodput(self):
        return self.nbiot_tx_goodput

    def set_nbiot_tx_goodput(self, nbiot_tx_goodput):
        self.nbiot_tx_goodput = nbiot_tx_goodput

    def get_nbiot_rate(self):
        return self.nbiot_rate

    def set_nbiot_rate(self, nbiot_rate):
        self.nbiot_rate = nbiot_rate

    def get_lte_tx_goodput(self):
        return self.lte_tx_goodput

    def set_lte_tx_goodput(self, lte_tx_goodput):
        self.lte_tx_goodput = lte_tx_goodput

    def get_lte_rate(self):
        return self.lte_rate

    def set_lte_rate(self, lte_rate):
        self.lte_rate = lte_rate

    def get_freq2(self):
        return self.freq2

    def set_freq2(self, freq2):
        self.freq2 = freq2
        self.hydra_hydra_sink_0.set_central_frequency(1, self.freq2)

    def get_freq1(self):
        return self.freq1

    def set_freq1(self, freq1):
        self.freq1 = freq1
        self.hydra_hydra_sink_0.set_central_frequency(0, self.freq1)

    def get_cpu_percent(self):
        return self.cpu_percent

    def set_cpu_percent(self, cpu_percent):
        self.cpu_percent = cpu_percent


def argument_parser():
    parser = OptionParser(usage="%prog: [options]", option_class=eng_option)
    return parser


def main(top_block_cls=tx, options=None):
    if options is None:
        options, _ = argument_parser().parse_args()

    tb = top_block_cls()
    tb.start()
    tb.wait()


if __name__ == '__main__':
    main()
