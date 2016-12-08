#!/usr/bin/python2
#
# Copyright 2005,2006,2011 Free Software Foundation, Inc.
# 
# This file is part of GNU Radio
# 
# GNU Radio is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3, or (at your option)
# any later version.
# 
# GNU Radio is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with GNU Radio; see the file COPYING.  If not, write to
# the Free Software Foundation, Inc., 51 Franklin Street,
# Boston, MA 02110-1301, USA.
# 

from gnuradio import gr
from gnuradio import eng_notation
from gnuradio import blocks
from gnuradio.digital import ofdm_mod

# /////////////////////////////////////////////////////////////////////////////
#                              transmit path
# /////////////////////////////////////////////////////////////////////////////

class transmit_path(gr.hier_block2):
    def __init__(self, options):
        '''
        See below for what options should hold
        '''
	gr.hier_block2.__init__(self, "transmit_path",
				gr.io_signature(0, 0, 0),
				gr.io_signature(1, 1, gr.sizeof_gr_complex))

        self._tx_amplitude = options.tx_amplitude # digital amp sent to radio

        self.ofdm_tx = ofdm_mod(options,
                                msgq_limit=4,
                                pad_for_usrp=False)

        self.amp = blocks.multiply_const_cc(1)
        self.set_tx_amplitude(self._tx_amplitude)

        # Create and setup transmit path flow graph
        self.connect(self.ofdm_tx, self.amp, self)

    def set_tx_amplitude(self, ampl):
        """
        Sets the transmit amplitude sent to the USRP

        Args:
            : ampl 0 <= ampl < 1.0.  Try 0.10
        """
        self._tx_amplitude = max(0.0, min(ampl, 1))
        self.amp.set_k(self._tx_amplitude)

    def send_pkt(self, payload='', eof=False):
        """
        Calls the transmitter method to send a packet
        """
        return self.ofdm_tx.send_pkt(payload, eof)


    def _print_verbage(self):
        """
        Prints information about the transmit path
        """
        print "Tx amplitude     %s" % (self._tx_amplitude)
