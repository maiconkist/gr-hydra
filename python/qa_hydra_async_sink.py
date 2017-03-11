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
        op1 = blocks.head(gr.sizeof_gr_complex, 128)
        op2 = blocks.stream_to_tagged_stream(gr.sizeof_gr_complex, 1, 1, "packet_len")
        op3 = blocks.tagged_stream_to_pdu(blocks.complex_t, "packet_len")
        dst = blocks.vector_sink_c(1)

        DUT = hydra.hydra_async_sink(1, 64, 100e3, 200e3, ((100e3, 200e3),))

        self.tb.msg_connect((op3, 'pdus'), (DUT, 'vr0'))
        self.tb.connect(src1, op1, op2, op3)
        self.tb.connect(DUT, dst)
        self.tb.run()

        # check if it is ok
        #print output

if __name__ == '__main__':
    gr_unittest.run(qa_hydra_sink, "qa_hydra_sink.xml")
