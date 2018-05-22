#!/usr/bin/env python2
# -*- coding: utf-8 -*-
##################################################
# GNU Radio Python Flow Graph
# Title: Nbiot
# Generated: Tue May 22 18:42:26 2018
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
import psutil
import threading
import time


class nbiot(gr.top_block):

    def __init__(self):
        gr.top_block.__init__(self, "Nbiot")

        ##################################################
        # Variables
        ##################################################
        self.zcpu = zcpu = psutil
        self._samprate_config = ConfigParser.ConfigParser()
        self._samprate_config.read('./default')
        try: samprate = self._samprate_config.getfloat("usrp_hydra", "samprate2")
        except: samprate = 200e3
        self.samprate = samprate
        self.rx_rate = rx_rate = 0
        self.rx_goodput = rx_goodput = 0
        self._freq_config = ConfigParser.ConfigParser()
        self._freq_config.read('./default')
        try: freq = self._freq_config.getfloat("usrp_hydra", "txfreq2")
        except: freq = 950.5e6
        self.freq = freq
        self.cpu_percent = cpu_percent = 0

        ##################################################
        # Blocks
        ##################################################
        self.probeiq = blocks.probe_rate(gr.sizeof_gr_complex*1, 500.0, 0.15)
        self.probe1_1 = blocks.probe_rate(gr.sizeof_char*1, 500.0, 0.15)
        self.xmlrpc_server_0 = SimpleXMLRPCServer.SimpleXMLRPCServer(('localhost', 1235), allow_none=True)
        self.xmlrpc_server_0.register_instance(self)
        self.xmlrpc_server_0_thread = threading.Thread(target=self.xmlrpc_server_0.serve_forever)
        self.xmlrpc_server_0_thread.daemon = True
        self.xmlrpc_server_0_thread.start()
        self.uhd_usrp_source_0 = uhd.usrp_source(
        	",".join(("", "")),
        	uhd.stream_args(
        		cpu_format="fc32",
        		channels=range(1),
        	),
        )
        self.uhd_usrp_source_0.set_samp_rate(samprate)
        self.uhd_usrp_source_0.set_center_freq(freq, 0)
        self.uhd_usrp_source_0.set_normalized_gain(0, 0)
        self.uhd_usrp_source_0.set_antenna('TX/RX', 0)

        def _rx_rate_probe():
            while True:
                val = self.probeiq.rate()
                try:
                    self.set_rx_rate(val)
                except AttributeError:
                    pass
                time.sleep(1.0 / (10))
        _rx_rate_thread = threading.Thread(target=_rx_rate_probe)
        _rx_rate_thread.daemon = True
        _rx_rate_thread.start()


        def _rx_goodput_probe():
            while True:
                val = self.probe1_1.rate()
                try:
                    self.set_rx_goodput(val)
                except AttributeError:
                    pass
                time.sleep(1.0 / (10))
        _rx_goodput_thread = threading.Thread(target=_rx_goodput_probe)
        _rx_goodput_thread.daemon = True
        _rx_goodput_thread.start()

        self.digital_ofdm_rx_0 = digital.ofdm_rx(
        	  fft_len=64, cp_len=16,
        	  frame_length_tag_key='frame_'+"length",
        	  packet_length_tag_key="length",
        	  bps_header=1,
        	  bps_payload=1,
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

        self.blocks_tag_debug_0 = blocks.tag_debug(gr.sizeof_char*1, "Rx'd Packet", ""); self.blocks_tag_debug_0.set_display(True)

        ##################################################
        # Connections
        ##################################################
        self.connect((self.digital_ofdm_rx_0, 0), (self.blocks_tag_debug_0, 0))
        self.connect((self.digital_ofdm_rx_0, 0), (self.probe1_1, 0))
        self.connect((self.uhd_usrp_source_0, 0), (self.digital_ofdm_rx_0, 0))
        self.connect((self.uhd_usrp_source_0, 0), (self.probeiq, 0))

    def get_zcpu(self):
        return self.zcpu

    def set_zcpu(self, zcpu):
        self.zcpu = zcpu

    def get_samprate(self):
        return self.samprate

    def set_samprate(self, samprate):
        self.samprate = samprate
        self.uhd_usrp_source_0.set_samp_rate(self.samprate)

    def get_rx_rate(self):
        return self.rx_rate

    def set_rx_rate(self, rx_rate):
        self.rx_rate = rx_rate

    def get_rx_goodput(self):
        return self.rx_goodput

    def set_rx_goodput(self, rx_goodput):
        self.rx_goodput = rx_goodput

    def get_freq(self):
        return self.freq

    def set_freq(self, freq):
        self.freq = freq
        self.uhd_usrp_source_0.set_center_freq(self.freq, 0)

    def get_cpu_percent(self):
        return self.cpu_percent

    def set_cpu_percent(self, cpu_percent):
        self.cpu_percent = cpu_percent


def main(top_block_cls=nbiot, options=None):

    tb = top_block_cls()
    tb.start()
    tb.wait()


if __name__ == '__main__':
    main()
