#!/usr/bin/env python2
# -*- coding: utf-8 -*-
##################################################
# GNU Radio Python Flow Graph
# Title: Lte
# Generated: Thu Sep 27 18:01:55 2018
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
import threading
import time


class lte(gr.top_block):

    def __init__(self):
        gr.top_block.__init__(self, "Lte")

        ##################################################
        # Variables
        ##################################################
        self.pilot_carriers = pilot_carriers = ((-27, -14, -7, 7, 14, 27),)
        self.pattern2 = pattern2 = [1, -1, 1, -1]
        self.pattern1 = pattern1 = [0., 1.41421356, 0., -1.41421356]
        self.fft_len = fft_len = 64
        self.bytes_tx = bytes_tx = 0
        self.bytes_rx = bytes_rx = 0
        self.sync_word2 = sync_word2 = [0., 0., 0., 0., 0., 0.,] + pattern2 * ((fft_len-12)/len(pattern2))  +[0., 0., 0., 0., 0., 0.,]
        self.sync_word1 = sync_word1 = [0., 0., 0., 0., 0., 0.,] + pattern1 * ((fft_len-12)/len(pattern1))  +[0., 0., 0., 0., 0., 0.,]
        self._samprate_config = ConfigParser.ConfigParser()
        self._samprate_config.read('./default')
        try: samprate = self._samprate_config.getfloat("usrp_hydra", "samprate1")
        except: samprate = 500e3
        self.samprate = samprate
        self.rx_rate = rx_rate = 0
        self.rx_goodput = rx_goodput = 0
        self.pilot_symbols = pilot_symbols = ((-1,1, 1, -1, -1, -1),)
        self.occupied_carriers = occupied_carriers = (sorted(tuple(set([x for x in range(-26,27)]) - set(pilot_carriers[0]) - set([0,]))),)
        self._freq_config = ConfigParser.ConfigParser()
        self._freq_config.read('./default')
        try: freq = self._freq_config.getfloat("usrp_hydra", "txfreq1")
        except: freq = 2.484e9-500e3
        self.freq = freq
        self.error_rate = error_rate = bytes_rx/(bytes_tx if bytes_tx>0 else 1)
        self.cpu_percent = cpu_percent = 0

        ##################################################
        # Blocks
        ##################################################
        self.probeiq = blocks.probe_rate(gr.sizeof_gr_complex*1, 500.0, 0.15)
        self.probe1_1 = blocks.probe_rate(gr.sizeof_char*1, 500.0, 0.15)
        self.digital_ofdm_rx_0 = digital.ofdm_rx(
        	  fft_len=64, cp_len=16,
        	  frame_length_tag_key='frame_'+"length",
        	  packet_length_tag_key="length",
        	  occupied_carriers=occupied_carriers,
        	  pilot_carriers=pilot_carriers,
        	  pilot_symbols=pilot_symbols,
        	  sync_word1=sync_word1,
        	  sync_word2=sync_word2,
        	  bps_header=1,
        	  bps_payload=1,
        	  debug_log=False,
        	  scramble_bits=False
        	 )
        self.xmlrpc_server_0 = SimpleXMLRPCServer.SimpleXMLRPCServer(('134.226.55.25', 1235), allow_none=True)
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


        def _bytes_tx_probe():
            while True:
                val = self.digital_ofdm_rx_0.crc.nitems_written(0)
                try:
                    self.set_bytes_tx(val)
                except AttributeError:
                    pass
                time.sleep(1.0 / (10))
        _bytes_tx_thread = threading.Thread(target=_bytes_tx_probe)
        _bytes_tx_thread.daemon = True
        _bytes_tx_thread.start()


        def _bytes_rx_probe():
            while True:
                val = self.digital_ofdm_rx_0.crc.nitems_read(0)
                try:
                    self.set_bytes_rx(val)
                except AttributeError:
                    pass
                time.sleep(1.0 / (10))
        _bytes_rx_thread = threading.Thread(target=_bytes_rx_probe)
        _bytes_rx_thread.daemon = True
        _bytes_rx_thread.start()

        self.blocks_tag_debug_0 = blocks.tag_debug(gr.sizeof_char*1, "Rx'd Packet", ""); self.blocks_tag_debug_0.set_display(True)

        ##################################################
        # Connections
        ##################################################
        self.connect((self.digital_ofdm_rx_0, 0), (self.blocks_tag_debug_0, 0))
        self.connect((self.digital_ofdm_rx_0, 0), (self.probe1_1, 0))
        self.connect((self.uhd_usrp_source_0, 0), (self.digital_ofdm_rx_0, 0))
        self.connect((self.uhd_usrp_source_0, 0), (self.probeiq, 0))

    def get_pilot_carriers(self):
        return self.pilot_carriers

    def set_pilot_carriers(self, pilot_carriers):
        self.pilot_carriers = pilot_carriers
        self.set_occupied_carriers((sorted(tuple(set([x for x in range(-26,27)]) - set(self.pilot_carriers[0]) - set([0,]))),))

    def get_pattern2(self):
        return self.pattern2

    def set_pattern2(self, pattern2):
        self.pattern2 = pattern2
        self.set_sync_word2([0., 0., 0., 0., 0., 0.,] + self.pattern2 * ((self.fft_len-12)/len(self.pattern2))  +[0., 0., 0., 0., 0., 0.,] )

    def get_pattern1(self):
        return self.pattern1

    def set_pattern1(self, pattern1):
        self.pattern1 = pattern1
        self.set_sync_word1([0., 0., 0., 0., 0., 0.,] + self.pattern1 * ((self.fft_len-12)/len(self.pattern1))  +[0., 0., 0., 0., 0., 0.,] )

    def get_fft_len(self):
        return self.fft_len

    def set_fft_len(self, fft_len):
        self.fft_len = fft_len
        self.set_sync_word2([0., 0., 0., 0., 0., 0.,] + self.pattern2 * ((self.fft_len-12)/len(self.pattern2))  +[0., 0., 0., 0., 0., 0.,] )
        self.set_sync_word1([0., 0., 0., 0., 0., 0.,] + self.pattern1 * ((self.fft_len-12)/len(self.pattern1))  +[0., 0., 0., 0., 0., 0.,] )

    def get_bytes_tx(self):
        return self.bytes_tx

    def set_bytes_tx(self, bytes_tx):
        self.bytes_tx = bytes_tx
        self.set_error_rate(self.bytes_rx/(self.bytes_tx if self.bytes_tx>0 else 1))

    def get_bytes_rx(self):
        return self.bytes_rx

    def set_bytes_rx(self, bytes_rx):
        self.bytes_rx = bytes_rx
        self.set_error_rate(self.bytes_rx/(self.bytes_tx if self.bytes_tx>0 else 1))

    def get_sync_word2(self):
        return self.sync_word2

    def set_sync_word2(self, sync_word2):
        self.sync_word2 = sync_word2

    def get_sync_word1(self):
        return self.sync_word1

    def set_sync_word1(self, sync_word1):
        self.sync_word1 = sync_word1

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

    def get_pilot_symbols(self):
        return self.pilot_symbols

    def set_pilot_symbols(self, pilot_symbols):
        self.pilot_symbols = pilot_symbols

    def get_occupied_carriers(self):
        return self.occupied_carriers

    def set_occupied_carriers(self, occupied_carriers):
        self.occupied_carriers = occupied_carriers

    def get_freq(self):
        return self.freq

    def set_freq(self, freq):
        self.freq = freq
        self.uhd_usrp_source_0.set_center_freq(self.freq, 0)

    def get_error_rate(self):
        return self.error_rate

    def set_error_rate(self, error_rate):
        self.error_rate = error_rate

    def get_cpu_percent(self):
        return self.cpu_percent

    def set_cpu_percent(self, cpu_percent):
        self.cpu_percent = cpu_percent


def main(top_block_cls=lte, options=None):

    tb = top_block_cls()
    tb.start()
    tb.wait()


if __name__ == '__main__':
    main()
