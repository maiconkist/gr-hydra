#!/usr/bin/env python
# -*- coding: utf-8 -*-
# 
# Copyright 2016 <+YOU OR YOUR COMPANY+>.
# 
# This is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3, or (at your option)
# any later version.
# 
# This software is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this software; see the file COPYING.  If not, write to
# the Free Software Foundation, Inc., 51 Franklin Street,
# Boston, MA 02110-1301, USA.
#
from gnuradio import gr, gr_unittest
from gnuradio import analog, digital, blocks
from grc_gnuradio import blks2 as grc_blks2
import numpy

import hydra_swig as hydra


class qa_hydra_sink (gr_unittest.TestCase):
    def setUp(self):
        self.tb = gr.top_block()

    def tearDown(self):
        self.tb = None

    def test_001_t(self):
        # set up fg
        src1 = analog.sig_source_c(512, analog.GR_COS_WAVE, 128, 0.5, 0)

        dst = blocks.vector_sink_c(1)
        op1 = blocks.head(gr.sizeof_gr_complex, 64)

        DUT = hydra.hydra_sink(1, 64, 100e3, 200e3, ((100e3, 200e3),))

        self.tb.connect(src1, op1, DUT, dst)
        self.tb.run()

        # get data
        output = dst.data()

        # check if it is ok
        #print output

    def test_002_t(self):
        # set up fg
        analog_random_source = blocks.vector_source_b(map(int, numpy.random.randint(0, 2, 1000)), True)

        digital_ofdm = grc_blks2.packet_mod_b(digital.ofdm_mod(
            options=grc_blks2.options(
                modulation="bpsk",
                fft_length=256,
                occupied_tones=200,
                cp_length=128,
                pad_for_usrp=True,
                log=None,
                verbose=None,

                ),
            ),
            payload_length=0,
        )

        head = blocks.head(gr.sizeof_gr_complex, 256)
        DUT = hydra.hydra_sink(1, 256, 100e3, 200e3, ((100e3, 200e3),))
        sink = blocks.vector_sink_c(1)
        sink = blocks.vector_sink_c(1)

        self.tb.connect(analog_random_source, digital_ofdm, head,  sink)
        self.tb.start()
        import time
        time.sleep(1)
        self.tb.stop()
        print sink.data()[0:255]

if __name__ == '__main__':
    gr_unittest.run(qa_hydra_sink, "qa_hydra_sink.xml")
