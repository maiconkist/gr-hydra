#!/usr/bin/env python2
# -*- coding: utf-8 -*-
##################################################
# GNU Radio Python Flow Graph
# Title: Hydra Gr Client 2Tx
# Generated: Thu Nov 29 17:33:00 2018
##################################################

from distutils.version import StrictVersion

if __name__ == '__main__':
    import ctypes
    import sys
    if sys.platform.startswith('linux'):
        try:
            x11 = ctypes.cdll.LoadLibrary('libX11.so')
            x11.XInitThreads()
        except:
            print "Warning: failed to XInitThreads()"

from PyQt5 import Qt, QtCore
from gnuradio import blocks
from gnuradio import digital
from gnuradio import eng_notation
from gnuradio import gr
from gnuradio.eng_option import eng_option
from gnuradio.filter import firdes
from gnuradio.qtgui import Range, RangeWidget
from optparse import OptionParser
import hydra
import sys
import threading
from gnuradio import qtgui


class hydra_gr_client_2tx(gr.top_block, Qt.QWidget):

    def __init__(self, freq=1.1e9, samp_rate=200e3):
        gr.top_block.__init__(self, "Hydra Gr Client 2Tx")
        Qt.QWidget.__init__(self)
        self.setWindowTitle("Hydra Gr Client 2Tx")
        qtgui.util.check_set_qss()
        try:
            self.setWindowIcon(Qt.QIcon.fromTheme('gnuradio-grc'))
        except:
            pass
        self.top_scroll_layout = Qt.QVBoxLayout()
        self.setLayout(self.top_scroll_layout)
        self.top_scroll = Qt.QScrollArea()
        self.top_scroll.setFrameStyle(Qt.QFrame.NoFrame)
        self.top_scroll_layout.addWidget(self.top_scroll)
        self.top_scroll.setWidgetResizable(True)
        self.top_widget = Qt.QWidget()
        self.top_scroll.setWidget(self.top_widget)
        self.top_layout = Qt.QVBoxLayout(self.top_widget)
        self.top_grid_layout = Qt.QGridLayout()
        self.top_layout.addLayout(self.top_grid_layout)

        self.settings = Qt.QSettings("GNU Radio", "hydra_gr_client_2tx")

        if StrictVersion(Qt.qVersion()) < StrictVersion("5.0.0"):
            self.restoreGeometry(self.settings.value("geometry").toByteArray())
        else:
            self.restoreGeometry(self.settings.value("geometry", type=QtCore.QByteArray))

        ##################################################
        # Parameters
        ##################################################
        self.freq = freq
        self.samp_rate = samp_rate

        ##################################################
        # Variables
        ##################################################
        self.mul2 = mul2 = 0.1
        self.mul1 = mul1 = 0.1
        self.freq1 = freq1 = freq - 100e3

        ##################################################
        # Blocks
        ##################################################
        self._mul2_range = Range(0, 1, 0.01, 0.1, 200)
        self._mul2_win = RangeWidget(self._mul2_range, self.set_mul2, 'mul2', "counter_slider", float)
        self.top_layout.addWidget(self._mul2_win)
        self._mul1_range = Range(0, 1, 0.01, 0.1, 200)
        self._mul1_win = RangeWidget(self._mul1_range, self.set_mul1, 'mul1', "counter_slider", float)
        self.top_layout.addWidget(self._mul1_win)
        self._freq1_range = Range(freq - 1e6, freq + 1e6, 100e3, freq - 100e3, 200)
        self._freq1_win = RangeWidget(self._freq1_range, self.set_freq1, 'freq1', "counter_slider", float)
        self.top_layout.addWidget(self._freq1_win)
        self.hydra_gr_sink_0_0 = hydra.hydra_gr_client_sink(2, '134.226.55.93', 5000)
        self.hydra_gr_sink_0_0.start_client(freq + 700e3, samp_rate, 1024)
        self.hydra_gr_sink_0 = hydra.hydra_gr_client_sink(1, '134.226.55.93', 5000)
        self.hydra_gr_sink_0.start_client(freq1, samp_rate * 2, 1024)
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
        self.blocks_vector_source_x_0_0 = blocks.vector_source_b([x for x in range(0,128)], True, 1, [])
        self.blocks_vector_source_x_0 = blocks.vector_source_b([x for x in range(0,256)], True, 1, [])
        self.blocks_throttle_0_0 = blocks.throttle(gr.sizeof_gr_complex*1, samp_rate,True)
        self.blocks_throttle_0 = blocks.throttle(gr.sizeof_gr_complex*1, samp_rate * 2,True)
        self.blocks_stream_to_tagged_stream_0_0 = blocks.stream_to_tagged_stream(gr.sizeof_char, 1, 100, "len")
        self.blocks_stream_to_tagged_stream_0 = blocks.stream_to_tagged_stream(gr.sizeof_char, 1, 100, "len")
        self.blocks_multiply_const_vxx_0_0 = blocks.multiply_const_vcc((mul2, ))
        self.blocks_multiply_const_vxx_0 = blocks.multiply_const_vcc((mul1, ))

        ##################################################
        # Connections
        ##################################################
        self.connect((self.blocks_multiply_const_vxx_0, 0), (self.hydra_gr_sink_0, 0))
        self.connect((self.blocks_multiply_const_vxx_0_0, 0), (self.hydra_gr_sink_0_0, 0))
        self.connect((self.blocks_stream_to_tagged_stream_0, 0), (self.digital_ofdm_tx_0, 0))
        self.connect((self.blocks_stream_to_tagged_stream_0_0, 0), (self.digital_ofdm_tx_0_0, 0))
        self.connect((self.blocks_throttle_0, 0), (self.blocks_multiply_const_vxx_0, 0))
        self.connect((self.blocks_throttle_0_0, 0), (self.blocks_multiply_const_vxx_0_0, 0))
        self.connect((self.blocks_vector_source_x_0, 0), (self.blocks_stream_to_tagged_stream_0, 0))
        self.connect((self.blocks_vector_source_x_0_0, 0), (self.blocks_stream_to_tagged_stream_0_0, 0))
        self.connect((self.digital_ofdm_tx_0, 0), (self.blocks_throttle_0, 0))
        self.connect((self.digital_ofdm_tx_0_0, 0), (self.blocks_throttle_0_0, 0))

    def closeEvent(self, event):
        self.settings = Qt.QSettings("GNU Radio", "hydra_gr_client_2tx")
        self.settings.setValue("geometry", self.saveGeometry())
        event.accept()

    def get_freq(self):
        return self.freq

    def set_freq(self, freq):
        self.freq = freq
        self.set_freq1(self.freq - 100e3)

    def get_samp_rate(self):
        return self.samp_rate

    def set_samp_rate(self, samp_rate):
        self.samp_rate = samp_rate
        self.blocks_throttle_0_0.set_sample_rate(self.samp_rate)
        self.blocks_throttle_0.set_sample_rate(self.samp_rate * 2)

    def get_mul2(self):
        return self.mul2

    def set_mul2(self, mul2):
        self.mul2 = mul2
        self.blocks_multiply_const_vxx_0_0.set_k((self.mul2, ))

    def get_mul1(self):
        return self.mul1

    def set_mul1(self, mul1):
        self.mul1 = mul1
        self.blocks_multiply_const_vxx_0.set_k((self.mul1, ))

    def get_freq1(self):
        return self.freq1

    def set_freq1(self, freq1):
        self.freq1 = freq1


def argument_parser():
    parser = OptionParser(usage="%prog: [options]", option_class=eng_option)
    return parser


def main(top_block_cls=hydra_gr_client_2tx, options=None):
    if options is None:
        options, _ = argument_parser().parse_args()

    if StrictVersion("4.5.0") <= StrictVersion(Qt.qVersion()) < StrictVersion("5.0.0"):
        style = gr.prefs().get_string('qtgui', 'style', 'raster')
        Qt.QApplication.setGraphicsSystem(style)
    qapp = Qt.QApplication(sys.argv)

    tb = top_block_cls()
    tb.start()
    tb.show()

    def quitting():
        tb.stop()
        tb.wait()
    qapp.aboutToQuit.connect(quitting)
    qapp.exec_()


if __name__ == '__main__':
    main()
