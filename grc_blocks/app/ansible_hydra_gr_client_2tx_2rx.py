#!/usr/bin/env python2
# -*- coding: utf-8 -*-
##################################################
# GNU Radio Python Flow Graph
# Title: Ansible Hydra Gr Client 2Tx 2Rx
# Generated: Mon Dec  3 18:54:54 2018
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
from optparse import OptionParser
import hydra
import sys
import threading
from gnuradio import qtgui


class ansible_hydra_gr_client_2tx_2rx(gr.top_block, Qt.QWidget):

    def __init__(self, freqrx=1.2e9, freqtx=1.1e9, hydraClientIP='192.168.5.54', hydraServerIP='192.168.5.54', samp_rate=200e3, vr1offset=-200e3, vr2offset=700e3):
        gr.top_block.__init__(self, "Ansible Hydra Gr Client 2Tx 2Rx")
        Qt.QWidget.__init__(self)
        self.setWindowTitle("Ansible Hydra Gr Client 2Tx 2Rx")
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

        self.settings = Qt.QSettings("GNU Radio", "ansible_hydra_gr_client_2tx_2rx")

        if StrictVersion(Qt.qVersion()) < StrictVersion("5.0.0"):
            self.restoreGeometry(self.settings.value("geometry").toByteArray())
        else:
            self.restoreGeometry(self.settings.value("geometry", type=QtCore.QByteArray))

        ##################################################
        # Parameters
        ##################################################
        self.freqrx = freqrx
        self.freqtx = freqtx
        self.hydraClientIP = hydraClientIP
        self.hydraServerIP = hydraServerIP
        self.samp_rate = samp_rate
        self.vr1offset = vr1offset
        self.vr2offset = vr2offset

        ##################################################
        # Blocks
        ##################################################
        self.hydra_gr_sink_0_0 = hydra.hydra_gr_client_sink(2, hydraServerIP, 5000)
        self.hydra_gr_sink_0_0.start_client(freqtx + vr2offset, samp_rate, 1024)
        self.hydra_gr_sink_0 = hydra.hydra_gr_client_sink(1, hydraServerIP, 5000)
        self.hydra_gr_sink_0.start_client(freqtx + vr1offset, samp_rate * 2, 512)
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
        self.blocks_multiply_const_vxx_0_0 = blocks.multiply_const_vcc((0.03, ))
        self.blocks_multiply_const_vxx_0 = blocks.multiply_const_vcc((0.03, ))

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
        self.settings = Qt.QSettings("GNU Radio", "ansible_hydra_gr_client_2tx_2rx")
        self.settings.setValue("geometry", self.saveGeometry())
        event.accept()

    def get_freqrx(self):
        return self.freqrx

    def set_freqrx(self, freqrx):
        self.freqrx = freqrx

    def get_freqtx(self):
        return self.freqtx

    def set_freqtx(self, freqtx):
        self.freqtx = freqtx

    def get_hydraClientIP(self):
        return self.hydraClientIP

    def set_hydraClientIP(self, hydraClientIP):
        self.hydraClientIP = hydraClientIP

    def get_hydraServerIP(self):
        return self.hydraServerIP

    def set_hydraServerIP(self, hydraServerIP):
        self.hydraServerIP = hydraServerIP

    def get_samp_rate(self):
        return self.samp_rate

    def set_samp_rate(self, samp_rate):
        self.samp_rate = samp_rate
        self.blocks_throttle_0_0.set_sample_rate(self.samp_rate)
        self.blocks_throttle_0.set_sample_rate(self.samp_rate * 2)

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
        "", "--hydraClientIP", dest="hydraClientIP", type="string", default='192.168.5.54',
        help="Set 192.168.5.54 [default=%default]")
    parser.add_option(
        "", "--hydraServerIP", dest="hydraServerIP", type="string", default='192.168.5.54',
        help="Set 192.168.5.54 [default=%default]")
    return parser


def main(top_block_cls=ansible_hydra_gr_client_2tx_2rx, options=None):
    if options is None:
        options, _ = argument_parser().parse_args()

    if StrictVersion("4.5.0") <= StrictVersion(Qt.qVersion()) < StrictVersion("5.0.0"):
        style = gr.prefs().get_string('qtgui', 'style', 'raster')
        Qt.QApplication.setGraphicsSystem(style)
    qapp = Qt.QApplication(sys.argv)

    tb = top_block_cls(hydraClientIP=options.hydraClientIP, hydraServerIP=options.hydraServerIP)
    tb.start()
    tb.show()

    def quitting():
        tb.stop()
        tb.wait()
    qapp.aboutToQuit.connect(quitting)
    qapp.exec_()


if __name__ == '__main__':
    main()
