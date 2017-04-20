#!/usr/bin/env python3

from Tkinter import *

# parse options
from optparse import OptionParser


root = Tk()
widget = None
options = None

import SimpleXMLRPCServer
import threading

class AppGui():
    def __init__(self, root, options):
        self._t = 70

        # pkt total 
        self.label = Label(root, text="Temperature: ", font=('Courier', 66))
        self.label.pack(side=TOP)
        self.temperature = Label(root, text=str(self._t), font=('Courier', 66))
        self.temperature.pack(side=TOP)


    def set_temperature(self, val):
        print 'called'
        try:
            self._t = str(val)
        except:
            print 'error there boy'

    def update(self):
        print 'updating label'
        self.temperature['text'] = self._t 
        root.after(1000, self.update)
    

if __name__ == '__main__':

    options = OptionParser(conflict_handler="resolve")
    options.add_option("", "--rpc-ip", type="string", default="localhost",
            help="UDP IP to receive data [default=%default]")
    options.add_option("", "--rpc-port", type="int", default=23451,
            help="UDP port to receive data [default=%default]")
    (options, args) = options.parse_args()

    widget = AppGui(root, options)


    xmlrpc_server = SimpleXMLRPCServer.SimpleXMLRPCServer((options.rpc_ip, options.rpc_port), allow_none=True)
    xmlrpc_server.register_instance(widget)
    threading.Thread(target=xmlrpc_server.serve_forever).start()

    root.after(1000, widget.update)
    root.mainloop()
