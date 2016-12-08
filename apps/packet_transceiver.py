#!/usr/bin/env python
#
# Packet transceiver
# This module receive packets from video_rx.py
# assemble packets to form a complete a frame (if necessary)
# and send the complete compressed frame to decoder

# Ver 1.0, Feb 17, 2009, Qin Chen
# Module created

from gnuradio import eng_notation
from gnuradio.eng_option import eng_option
from optparse import OptionParser
import socket
import struct

# from current dir
from transmit_path import transmit_path


# /////////////////////////////////////////////////////////////////////////////
#                                   main
# /////////////////////////////////////////////////////////////////////////////

def main():

    parser = OptionParser(option_class=eng_option, conflict_handler="resolve")
    expert_grp = parser.add_option_group("Expert")
    
    parser.add_option("", "--receiveport", type="intx", default=12360,
                      help="set receiver UDP socket port number [default=%default]")
    parser.add_option("", "--sendport", type="intx", default=12349,
                      help="set sender UDP socket port number [default=%default]")
    parser.add_option("", "--host", default="127.0.0.1",
                      help="set decoder host IP address [default=%default]")   

    transmit_path.add_options(parser, expert_grp)
    (options, args) = parser.parse_args ()

    # print options.receiveport
    # print options.sendport
    # print options.host

    if len(args) != 0:
        parser.print_help()
        sys.exit(1)

    # create socket server (packet receiver)
    s1 = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    host1 = '' # can leave this blank on the server side
    # print options.port
    # print options.bufferbytes
    try:
        s1.bind((host1, options.receiveport))
    except socket.error, err:
        print "Could not set up a UDP server on port %d: %s" % (options.receiveport, err)
        raise SystemExit

    # create socket client (packet sender)
    s2 = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    s2.connect((options.host, options.sendport))

    packet_size = 4096
    payload = ""
    while True:
        data = s1.recv(packet_size)
        if not data:
            break

        print "receive packet length = %4d bytes" % (len(data))

        # packet assembler here
        (head, ) = struct.unpack('!H', data[0:2])
        payload = payload + data[2:]
        print "packet length after unpack %4d" % (len(data))
        print "payload length %4d before sending" % (len(payload))
        print "head = %d" % (head)
        if head == 0x5555:
            continue
       
        s2.send(payload)
        print "payload length sent %4d" % (len(payload))
        payload = ""
    


    # close the sockets
    s1.close()
    s2.close()

if __name__ == '__main__':
    try:
        main()
    except KeyboardInterrupt:
        pass

    
