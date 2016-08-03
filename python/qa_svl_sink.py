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
from gnuradio import analog, blocks
import svl_swig as svl


class qa_svl_sink (gr_unittest.TestCase):

    def setUp(self):
        self.tb = gr.top_block()

    def tearDown(self):
        self.tb = None

    def test_001_t(self):
        # set up fg
        src1 = analog.sig_source_c(300e3, analog.GR_SIN_WAVE, 100e3, 1)

        dst = blocks.vector_sink_c(128)
        op1 = blocks.head(gr.sizeof_gr_complex, 64)
        op2 = blocks.head(gr.sizeof_gr_complex, 64)

        hypervisor = svl.svl_sink()
        vr1 = hypervisor.create_vradio()
        vr2 = hypervisor.create_vradio()

        self.tb.connect(src1, op1, (hypervisor, 0))
        self.tb.connect(op1, (hypervisor, 1))
        self.tb.connect(hypervisor, dst)

        self.tb.run()

        output = dst.data()
        # check data

if __name__ == '__main__':
    gr_unittest.run(qa_svl_sink, "qa_svl_sink.xml")
