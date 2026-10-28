#!/usr/bin/env python3
# -*- coding: utf-8 -*-

#
# SPDX-License-Identifier: GPL-3.0
#
# GNU Radio Python Flow Graph
# Title: Ansible Hydra Gr Client 1Tx 1Rx
# GNU Radio version: 3.10.9.2

from PyQt5 import Qt
from gnuradio import qtgui
from PyQt5 import QtCore
from gnuradio import blocks
from gnuradio import digital
from gnuradio import gr
from gnuradio.filter import firdes
from gnuradio.fft import window
import sys
import signal
from PyQt5 import Qt
from argparse import ArgumentParser
from gnuradio.eng_arg import eng_float, intx
from gnuradio import eng_notation
from gnuradio import gr, pdu
from gnuradio import network
import hydra
import threading



class ansible_hydra_gr_client_1tx_1rx(gr.top_block, Qt.QWidget):

    def __init__(self, ansibleIP='192.168.5.251', freqrx=1.1e9 + 5e6, freqtx=1.1e9, samp_rate=200e3, vr1offset=-300e3, vr2offset=700e3):
        gr.top_block.__init__(self, "Ansible Hydra Gr Client 1Tx 1Rx", catch_exceptions=True)
        Qt.QWidget.__init__(self)
        self.setWindowTitle("Ansible Hydra Gr Client 1Tx 1Rx")
        qtgui.util.check_set_qss()
        try:
            self.setWindowIcon(Qt.QIcon.fromTheme('gnuradio-grc'))
        except BaseException as exc:
            print(f"Qt GUI: Could not set Icon: {str(exc)}", file=sys.stderr)
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

        self.settings = Qt.QSettings("GNU Radio", "ansible_hydra_gr_client_1tx_1rx")

        try:
            geometry = self.settings.value("geometry")
            if geometry:
                self.restoreGeometry(geometry)
        except BaseException as exc:
            print(f"Qt GUI: Could not restore geometry: {str(exc)}", file=sys.stderr)

        ##################################################
        # Parameters
        ##################################################
        self.ansibleIP = ansibleIP
        self.freqrx = freqrx
        self.freqtx = freqtx
        self.samp_rate = samp_rate
        self.vr1offset = vr1offset
        self.vr2offset = vr2offset

        ##################################################
        # Variables
        ##################################################
        self.mul = mul = 0.04

        ##################################################
        # Blocks
        ##################################################

        self._mul_range = qtgui.Range(0, 1, 0.01, 0.04, 200)
        self._mul_win = qtgui.RangeWidget(self._mul_range, self.set_mul, "mul", "counter_slider", float, QtCore.Qt.Horizontal)
        self.top_layout.addWidget(self._mul_win)
        self.hydra_gr_source_0 = hydra.hydra_gr_client_source(1, ansibleIP, 5000, 'default')
        self.hydra_gr_source_0.start_client((freqrx + vr1offset), (samp_rate * 2), 5000)
        self.hydra_gr_sink_0 = hydra.hydra_gr_client_sink(1, ansibleIP, 5000, 'default')
        self.hydra_gr_sink_0.start_client((freqtx + vr1offset), (samp_rate * 2), 1024)
        self.digital_ofdm_tx_0 = digital.ofdm_tx(
            fft_len=64,
            cp_len=16,
            packet_length_tag_key="len",
            bps_header=1,
            bps_payload=1,
            rolloff=0,
            debug_log=False,
            scramble_bits=False)
        self.digital_ofdm_rx_0 = digital.ofdm_rx(
            fft_len=64, cp_len=16,
            frame_length_tag_key='frame_'+"len",
            packet_length_tag_key="len",
            bps_header=1,
            bps_payload=1,
            debug_log=False,
            scramble_bits=False)
        self.blocks_tuntap_pdu_1 = network.tuntap_pdu('tap0', 1000, False)
        self.blocks_tuntap_pdu_1.set_max_output_buffer(100000)
        self.blocks_tagged_stream_to_pdu_0 = pdu.tagged_stream_to_pdu(gr.types.byte_t, "len")
        self.blocks_tag_debug_1 = blocks.tag_debug(gr.sizeof_gr_complex*1, 'VR1 TX', "")
        self.blocks_tag_debug_1.set_display(True)
        self.blocks_tag_debug_0 = blocks.tag_debug(gr.sizeof_char*1, 'VR1 RX', "")
        self.blocks_tag_debug_0.set_display(True)
        self.blocks_pdu_to_tagged_stream_0 = pdu.pdu_to_tagged_stream(gr.types.byte_t, "len")
        self.blocks_multiply_const_vxx_0 = blocks.multiply_const_cc(mul)


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
        self.connect((self.digital_ofdm_tx_0, 0), (self.blocks_tag_debug_1, 0))
        self.connect((self.hydra_gr_source_0, 0), (self.digital_ofdm_rx_0, 0))


    def closeEvent(self, event):
        self.settings = Qt.QSettings("GNU Radio", "ansible_hydra_gr_client_1tx_1rx")
        self.settings.setValue("geometry", self.saveGeometry())
        self.stop()
        self.wait()

        event.accept()

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

    def get_mul(self):
        return self.mul

    def set_mul(self, mul):
        self.mul = mul
        self.blocks_multiply_const_vxx_0.set_k(self.mul)



def argument_parser():
    parser = ArgumentParser()
    parser.add_argument(
        "--ansibleIP", dest="ansibleIP", type=str, default='192.168.5.251',
        help="Set ansibleIP [default=%(default)r]")
    return parser


def main(top_block_cls=ansible_hydra_gr_client_1tx_1rx, options=None):
    if options is None:
        options = argument_parser().parse_args()

    qapp = Qt.QApplication(sys.argv)

    tb = top_block_cls(ansibleIP=options.ansibleIP)

    tb.start()

    tb.show()

    def sig_handler(sig=None, frame=None):
        tb.stop()
        tb.wait()

        Qt.QApplication.quit()

    signal.signal(signal.SIGINT, sig_handler)
    signal.signal(signal.SIGTERM, sig_handler)

    timer = Qt.QTimer()
    timer.start(500)
    timer.timeout.connect(lambda: None)

    qapp.exec_()

if __name__ == '__main__':
    main()
