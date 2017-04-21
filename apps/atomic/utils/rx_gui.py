#!/usr/bin/env python3

from Tkinter import *

# parse options
from optparse import OptionParser


root = Tk()
widget = None
options = None

import xmlrpclib
import SimpleXMLRPCServer
import threading

class AppGui():
    def __init__(self, radio_id, root, options):
        self._t = 70

        # temperature total 
        if radio_id == 1:
            temperature_frame = Frame(root)
            Label(temperature_frame, text="Temperature: ", font=('Courier', 44)).pack(side=LEFT)
            self.temperature = Label(temperature_frame, text=str(self._t), font=('Courier', 44))
            self.temperature.pack(side=RIGHT)
            temperature_frame.pack()

            root.title('NB-IoT')
        else:
            root.title('LTE')

        # pkt total 
        pkt_recv_frame = Frame(root)
        Label(pkt_recv_frame, text="Pkt total: ", font=('Courier', 44)).pack(side=LEFT)
        self.pkt_recv_info = Label(pkt_recv_frame, text="0", font=('Courier', 44))
        self.pkt_recv_info.pack(side=LEFT)
        pkt_recv_frame.pack()

        #pkt right
        pkt_right_frame = Frame(root)
        Label(pkt_right_frame, text="Pkt right: ", font=('Courier', 44)).pack(side=LEFT)
        self.pkt_right_info = Label(pkt_right_frame, text="0", font=('Courier', 44))
        self.pkt_right_info.pack(side=LEFT)
        pkt_right_frame.pack()

        # throughput
        throughput_frame = Frame(root)
        Label(throughput_frame, text="Throughput: ", font=('Courier', 44)).pack(side=LEFT)
        self.throughput_info = Label(throughput_frame, text="0", font=('Courier', 44))
        self.throughput_info.pack(side=LEFT)
        throughput_frame.pack()

        #center freq
        cf_frame = Frame(root)
        Label(cf_frame, text="Center frequency: ", font=('Courier', 44)).pack(side=LEFT)
        self.cf_info = Label(cf_frame, text="0", font=('Courier', 44))
        self.cf_info.pack(side=LEFT)
        cf_frame.pack()

        # bandwidth
        bw_frame = Frame(root)
        Label(bw_frame, text="Bandwidth: ", font=('Courier', 44)).pack(side=LEFT)
        self.bw_info = Label(bw_frame, text="0", font=('Courier', 44))
        self.bw_info.pack(side=LEFT)
        bw_frame.pack()


    def set_temperature(self, val):
        print 'called'
        try:
            self._t = str(val)
        except:
            print 'error there boy'

    def update(self):

        server = xmlrpclib.ServerProxy("http://%s:%d" % (options.rpc_ip, options.rpc_port))
        self.pkt_recv_info['text']   = server.get_pkt_rcvd()
        self.pkt_right_info['text']  = server.get_pkt_right()
        self.throughput_info['text'] = server.get_throughput()
        self.cf_info['text']         = str(float(server.get_center_freq()/1e9)) + "G"
        if options.radio == 0:
            self.bw_info['text']         = str(float(server.get_bandwidth())/1e6) + "MHz"
        else:
            self.bw_info['text']         = str(float(server.get_bandwidth())/1e3) + "KHz"


        if  options.radio_id == 1:
            print 'updating label'
            self.temperature['text'] = self._t

        root.after(1000, self.update)

if __name__ == '__main__':

    options = OptionParser(conflict_handler="resolve")
    options.add_option("", "--radio-id", type="int", default=1,
            help="0 for LTE GUI interface, 1 for NB-IoT [default=%default]")
    options.add_option("", "--rpc-ip", type="string", default="localhost",
            help="UDP IP to receive data [default=%default]")
    options.add_option("", "--rpc-port", type="int", default=12345,
            help="UDP port to receive data [default=%default]")

    (options, args) = options.parse_args()

    widget = AppGui(options.radio_id, root, options)

    if options.radio_id == 1:
        xmlrpc_server = SimpleXMLRPCServer.SimpleXMLRPCServer((options.rpc_ip, options.rpc_port), allow_none=True)
        xmlrpc_server.register_instance(widget)
        threading.Thread(target=xmlrpc_server.serve_forever).start()

    root.after(1000, widget.update)
    root.mainloop()
