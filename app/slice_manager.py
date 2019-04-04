#!/usr//bin/env python2

import sys, time

import hydra
import xmlrpclib
import json



# This class receives a hydra_client, which interacts with the Hydra-Server,
#       and a xmlrpclib.ServerProxy, which interacts with the UE side.
# Use the methods to configure both the client and ue at the same time.

class Slice:
    def __init__(self, client, ue):
        self.client = client
        self.ue = ue


    def free(self):
        self.client.free_resources()

    def allocate_tx(self, freq, bandwidth):

        # Configure TX freq for slice
        spectrum_conf = hydra.rx_configuration(freq, bandwidth, False)
        ret = self.client.request_tx_resources( spectrum_conf )

        if (ret < 0):
            print "Error allocating HYDRA TX resources: freq: %f, bandwidth %f" % (freq, bandwidth) 
            return -1

        #  Configure the RX freq for UE (to receive from the slice)
        self.ue.set_freqrx(freq)
        self.ue.set_vr1offset(0)
        self.ue.set_vr2offset(0)
        self.ue.set_samp_rate(bandwidth)

        if (self.ue.get_freqrx() != freq or self.ue.get_samp_rate() != bandwidth):
            print "Error allocating UE RX resources: freq: %f, bandwidth %f" % (freq, bandwidth) 
            return -1

        return 0

    def allocate_rx(self, freq, bandwidth):
        # Configure RX freq for slice
        spectrum_conf = hydra.rx_configuration(freq, bandwidth, False)
        ret = self.client.request_rx_resources( spectrum_conf )

        if (ret < 0):
            print "Error allocating TX resources: freq: %f, bandwidth %f" % (freq, bandwidth) 

        #  Configure the RX freq for UE (to receive from the slice)
        self.ue.set_freqtx(freq)
        self.ue.set_vr1offset(0)
        self.ue.set_vr2offset(0)
        self.ue.set_samp_rate(bandwidth)

        if (self.ue.get_freqtx() != freq or self.ue.get_samp_rate() != bandwidth):
            print "Error allocating UE TX resources: freq: %f, bandwidth %f" % (freq, bandwidth) 
            return -1

        return 0


def main():
    ## IMPORTANT: The script ansible_hydra_client_2tx_2rx already allocated resources for transmission and reception
    ##            These resources are under the use of clients ID 1 and ID 2. The trick of this script is to connect
    #           with the server using the same ID (1 and 2), and them releasing the resources allocated.
    #           We can then allocate new resources from this python without impacting the slices.

    client1 = hydra.hydra_client("192.168.5.70", 5000, 1, True)


    ue1 = xmlrpclib.ServerProxy("http://192.168.5.78:8080")

    if client1.check_connection(3) == "":
        print("client1 could not connect to server")
        sys.exit(1)

    # put both the client and the ue in a slice class.
    slice1 = Slice( client1, ue1)
    # This configuration is just to "clear" the offsets in the gnuradio file.
    # Dont remove it.
    slice1.allocate_tx(2.43e9 - 300e3, 400e3)
    slice1.allocate_rx(2.43e9 + 3e6 - 300e3, 400e3)


    if (False):
        client2 = hydra.hydra_client("192.168.5.81", 5000, 2)
        ue2 = xmlrpclib.ServerProxy("http://192.168.5.81:8080")

        if client2.check_connection(3) == "":
            print("client2 could not connect to server")
            sys.exit(1)
        slice2 = Slice( client2, ue2)
        slice2.allocate_tx(2.43e9 + 400e3, 200e3)
        slice2.allocate_rx(2.43e9 + 3e6 + 300e3, 200e3)


    ## YOUR CODE HERE



    print("Press CTRL-C to quit")
    while True:
        time.sleep(1)



if __name__ == "__main__":
    main()
