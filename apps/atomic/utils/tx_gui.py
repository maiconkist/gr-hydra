#!/usr/bin/env python3

import pickle
import subprocess
from Tkinter import *

# parse options
from optparse import OptionParser

import xmlrpclib


root = Tk()
widget = None
options = None

rx_var = IntVar()


class AppGui():
    def __init__(self, root, options):
        self.vr1_throughput_info, self.vr1_cf_info, self.vr1_bw_info = self.build_vr_menu(0, root, "VR LTE", self.vr1_tx_amplitude_update, self.vr1_center_freq, self.vr1_bandwidth_update)

        self.vr2_throughput_info, self.vr2_cf_info, self.vr2_bw_info = self.build_vr_menu(1, root, "VR NB-IoT", self.vr2_tx_amplitude_update, self.vr2_center_freq, self.vr2_bandwidth_update)

    def build_vr_menu(self, vr_id, root, title, tx_amplitude_callback, center_freq_callback, bandwidth_callback):
        vr_frame = LabelFrame(root, text=title)
        vr_frame.pack(fill="both", expand="yes")

        # throughput
        vr_throughput_frame = Frame(vr_frame)
        Label(vr_throughput_frame, text="RX Throughput: ").pack(side=LEFT)
        vr_throughput_info = Label(vr_throughput_frame, text="0")
        vr_throughput_info.pack(side=LEFT)
        vr_throughput_frame.pack()

        #center freq
        vr_cf_frame = Frame(vr_frame)
        Label(vr_cf_frame, text="Center frequency: ").pack(side=LEFT)
        vr_cf_info = Label(vr_cf_frame, text="0")
        vr_cf_info.pack(side=LEFT)
        vr_cf_frame.pack()

        # bandwidth
        vr_bw_frame = Frame(vr_frame)
        Label(vr_bw_frame, text="Bandwidth: ").pack(side=LEFT)
        vr_bw_info = Label(vr_bw_frame, text="0")
        vr_bw_info.pack(side=LEFT)
        vr_bw_frame.pack()

        # vr tx-amplitude
        vr_tx_amplitude_frame = Frame(vr_frame)
        Label(vr_tx_amplitude_frame, text="TX Amplitude [0:1]: ").pack(side=LEFT)
        scale = Scale(vr_tx_amplitude_frame, from_ = 0.0, to = 1.0, resolution= 0.001, orient = HORIZONTAL, command=tx_amplitude_callback)
        scale.pack(anchor=CENTER)
        vr_tx_amplitude_frame.pack()
        scale.set(0.3)

        #cf scale
        vr_center_frequency_frame = Frame(vr_frame)
        Label(vr_center_frequency_frame, text="Center frequency [-2MHz:+2MHz]: ").pack(side=LEFT)
        scale = Scale(vr_center_frequency_frame, from_ = -2e6, to = 2e6, resolution= 100e3, orient = HORIZONTAL, command=center_freq_callback)
        scale.pack(anchor=CENTER)
        vr_center_frequency_frame.pack()

        # ::WARNING::
        # shift comes from gr-hydra-apps/video_benchmark_tx.py
        scale.set(-500e3 if vr_id == 0 else 400e3)

	# bw scale
        #vr_bw_frame = Frame(vr_frame)
        #Label(vr_bw_frame, text="Bandwidth [100kHz:2MHz]: ").pack(side=LEFT)
        #scale = Scale(vr_bw_frame, from_ = 100e3, to = 2e6, resolution= 100e3, orient = HORIZONTAL, command=bandwidth_callback)
        #scale.pack(anchor=CENTER)
        #vr_bw_frame.pack()

        ## ::WARNING::
        ## bandwidth comes from gr-hydra-apps/video_benchmark_tx.py
        #scale.set(1e6 if vr_id == 0 else 200e3)

        return vr_throughput_info, vr_cf_info, vr_bw_info

    def vr1_tx_amplitude_update(self, val):
        if val is None:
            return
        val = float(val)
        s = xmlrpclib.ServerProxy("http://%s" % (options.tx_ip))
        s.set_amplitude1(val)

    def vr2_tx_amplitude_update(self, val):
        if val is None:
            return
        val = float(val)
        s = xmlrpclib.ServerProxy("http://%s" % (options.tx_ip))
        s.set_amplitude2(val)

    def vr1_center_freq(self, val):
        if val is None:
            return
        val = float(val)
        s = xmlrpclib.ServerProxy("http://%s" % (options.tx_ip))
        s.set_freq1(val)

        #s = xmlrpclib.ServerProxy("http://%s" % (options.lte_ip))
        #s.set_center_freq(val)

    def vr2_center_freq(self, val):
        if val is None:
            return
        val = float(val)
        s = xmlrpclib.ServerProxy("http://%s" % (options.tx_ip))
        s.set_freq2(val)

        #s = xmlrpclib.ServerProxy("http://%s" % (options.nbiot_ip))
        #s.set_center_freq(val)

    def vr1_bandwidth_update(self, val):
        pass

    def vr2_bandwidth_update(self, val):
        pass

def update_vars():
        v = {}

        try:
                lte_s = xmlrpclib.ServerProxy("http://%s" % (options.lte_ip))
                v['rx1_throughput']  = lte_s.get_rx_goodput()
                v['rx1_center_freq'] = lte_s.get_freq()
                v['rx1_bandwidth']   = lte_s.get_samprate()
        except:
                print("Could not get from LTE receiver. Trying again later")

        try:
                nbiot_s = xmlrpclib.ServerProxy("http://%s" % (options.nbiot_ip))
                v['rx2_throughput']  = nbiot_s.get_rx_goodput()
                v['rx2_center_freq'] = nbiot_s.get_freq()
                v['rx2_bandwidth']   = nbiot_s.get_samprate()
        except:
                print("Could not get from NB-IoT receiver. Trying again later")

        return v


def update():
    try:
        the_variables = update_vars()

        widget.vr1_throughput_info['text'] = the_variables['rx1_throughput'] if 'rx1_throughput' in the_variables else 'NA'
        widget.vr2_throughput_info['text'] = the_variables['rx2_throughput'] if 'rx2_throughput' in the_variables else 'NA'

        widget.vr1_cf_info['text'] = the_variables['rx1_center_freq'] if 'rx1_center_freq' in the_variables else 'NA'
        widget.vr2_cf_info['text'] = the_variables['rx2_center_freq'] if 'rx2_center_freq' in the_variables else 'NA'

        widget.vr1_bw_info['text'] = the_variables['rx1_bandwidth'] if 'rx1_bandwidth' in the_variables else 'NA'
        widget.vr2_bw_info['text'] = the_variables['rx2_bandwidth'] if 'rx2_bandwidth' in the_variables else 'NA'


    except Exception as e:
        print(e)

    root.after(1000, update)
    
if __name__ == '__main__':

    options = OptionParser(conflict_handler="resolve")
    options.add_option("", "--tx-ip", type="string", default="localhost:1235",
            help="TX host IP address [default=%default]")
    options.add_option("", "--lte-ip", type="string", default="134.226.55.25:1235",
            help="TX host IP address [default=%default]")
    options.add_option("", "--nbiot-ip", type="string", default="134.226.55.93:1235",
            help="TX host IP address [default=%default]")
    (options, args) = options.parse_args()

    widget = AppGui(root, options)

    root.after(5000, update)
    root.mainloop()
