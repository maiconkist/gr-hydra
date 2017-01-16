#!/usr/bin/env python3

import tkinter as tk
import socket

widget = None
root = None

s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
s.bind(("localhost", 23452))

class AppGui():
    def __init__(self, root):
        self.the_label = tk.Label(root, text="", font=("Helvetica", 256))
        self.the_label.pack()

def update():

    data = s.recv(1024)

    widget.the_label['text'] = data.decode('ascii').replace("\n", "")
    root.after(1, update)

if __name__ == '__main__':
    root = tk.Tk()
    root.wm_title("VR2 RX Recent Value")

    widget = AppGui(root)


    root.after(1, update)
    root.mainloop()
