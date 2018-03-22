#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import datetime
import logging
import wishful_controller
import gevent
import wishful_upis as upis
import os
import sys
import time
import pickle
import wishful_module_gnuradio

__author__ = "Maicon Kist"
__copyright__ = "Copyright (c) 2017 Connect Centre - Trinity College Dublin" 
__version__ = "0.1.0"
__email__ = "kistm@tcd.ie"


log = logging.getLogger('wishful_controller')
log_level = logging.INFO
logging.basicConfig(level=log_level, format='%(asctime)s - %(name)s.%(funcName)s() - %(levelname)s - %(message)s')

#Create controller
# ::TRICKY:: update IP addresses to external interface
controller = wishful_controller.Controller(dl="tcp://172.16.16.5:8990", ul="tcp://172.16.16.5:8989")

#Configure controller
controller.set_controller_info(name="WishfulController", info="WishfulControllerInfo")
# ::TRICKY:: update IP addresses to external interface
controller.add_module(moduleName="discovery",
	pyModuleName="wishful_module_discovery_pyre",
	className="PyreDiscoveryControllerModule",
	kwargs={"iface":"eth0", "groupName":"tcd_hydra", "downlink":"tcp://172.16.16.5:8990", "uplink":"tcp://172.16.16.5:8989"})


nodes = {}
program_running = None
the_variables = {}

TOTAL_NODES = 2
NODE_NAMES = ["tx", "lte", "nbiot"]

conf = {

    # list of files that will be send to agents
    'files' : {

    	"tx" :  "/users/kistm/gr-hydra/apps/atomic/tx/vr1_vr2_tx.grc", 
    	"lte" : "/users/kistm/gr-hydra/apps/atomic/rx/lte.grc", 
    	"nbiot" : "/users/kistm/gr-hydra/apps/atomic/rx/nbiot.grc", 
    },

    'program_getters' : {
        "tx":    ["bandwidth", "center_freq"],
        "lte":   ["bandwidth", "center_freq", "throughput" ],
        "nbiot": ["bandwidth", "center_freq", "throughput"],
    },

    'program_args': {
        "tx":  [""], 
        "lte": [""],
        "nbiot": [""],
    },
}


SETTER_FILE = "./setter.bin"


@controller.new_node_callback()
def new_node(node):
    log.info("New node appeared: Name: %s" % (node.name, ))
    nodes[node.name] = node

    #if node.name not in NODE_NAMES:
    #    log.info("Node '%s' is not part of this showcase. Ignoring it" % (node.name, ))
    #else:
    if node.name in ['tx', 'lte', 'nbiot']:
        program_name = node.name
        program_code = open(conf['files'][program_name], "r").read()
        program_args = conf['program_args'][node.name]

        controller.blocking(False).node(node).radio.iface('usrp').activate_radio_program({'program_name': program_name, 'program_code': program_code, 'program_args': program_args,'program_type': 'grc', 'program_port': 1235})

@controller.node_exit_callback()
def node_exit(node, reason):

    if node in nodes.values():
        if node.name == 'rx':
            program_running = None
        del nodes[node.name]

    log.info(("NodeExit : NodeID : {} Reason : {}".format(node.id, reason)))

@controller.set_default_callback()
def default_callback(group, node, cmd, data):
    log.info("{} DEFAULT CALLBACK : Group: {}, NodeName: {}, Cmd: {}, Returns: {}".format(datetime.datetime.now(), group, node.name, cmd, data))


@controller.add_callback(upis.radio.get_parameters)
def get_vars_response(group, node, data):
    log.info("{} get_vars_response : Group:{}, NodeId:{}, msg:{}".format(datetime.datetime.now(), group, node.id, data))

    if node.name == 'lte':
        the_variables['rx1_pkt_rcv'] = data['pkt_rcvd'] if 'pkt_rcvd' in data else 'NA'
        the_variables['rx1_pkt_right'] = data['pkt_right'] if 'pkt_right' in data else 'NA'
        the_variables['rx1_center_freq'] = data['center_freq'] if 'center_freq' in data else 'NA'
        the_variables['rx1_bandwidth'] = data['bandwidth'] if 'bandwidth' in data else 'NA'

        if 'throughput' in data:
           the_variables['rx1_throughput'] = str( float(data['throughput'])/1000.0) + " Kbps"
        else:
           the_variables['rx1_throughput'] = 'NA'

    elif node.name == 'nbiot':
        the_variables['rx2_pkt_rcv'] = data['pkt_rcvd'] if 'pkt_rcvd' in data else 'NA'
        the_variables['rx2_pkt_right'] = data['pkt_right'] if 'pkt_right' in data else 'NA'
        the_variables['rx2_center_freq'] = data['center_freq'] if 'center_freq' in data else 'NA'
        the_variables['rx2_bandwidth'] = data['bandwidth'] if 'bandwidth' in data else 'NA'

        if 'throughput' in data:
           the_variables['rx2_throughput'] = str( float(data['throughput'])/1000.0) + " Kbps"
        else:
           the_variables['rx2_throughput'] = 'NA'

    pickle.dump(the_variables, open("./getter.bin", "wb"))

def exec_loop():
    #Start controller
    controller.start()

    running = False
    first_time = True

    while len(nodes) < TOTAL_NODES:
    # Waiting for 2 nodes
        log.info("%d nodes connected. Waiting for %d more" % (len(nodes), TOTAL_NODES - len(nodes)))
        gevent.sleep(2)

    log.info("All nodes connected. Starting showcase...")

    #control loop
    while nodes:
            # TRICKY: gets are assynchronous. callback for get_parameters is called automatically
            if 'tx' in nodes:
                log.info("Requesting data to VR TX")
                controller.blocking(False).node(nodes['tx']).radio.iface('usrp').get_parameters(conf['program_getters']['tx'])

            if 'lte' in nodes:
                log.info("Requesting data to VR LTE")
                controller.blocking(False).node(nodes['lte']).radio.iface('usrp').get_parameters(conf['program_getters']['lte'])

            if 'nbiot' in nodes:
                log.info("Requesting data to VR NB-IoT")
                controller.blocking(False).node(nodes['nbiot']).radio.iface('usrp').get_parameters(conf['program_getters']['nbiot'])


            ## set variables
            #setters = {}
            #try:
            #    setters = pickle.load(open(SETTER_FILE, "rb"))
            #    pickle.dump({}, open(SETTER_FILE, "wb"))
            #except Exception as e:
            #    log.info("Could not open setters file")
            #    log.info(e)

            #if 'tx' in setters.keys() and 'tx' in nodes:
            #        log.info("Setting configuration of node TX") 
            #        controller.blocking(False).node(nodes['tx']).radio.iface('usrp').set_parameters(setters['tx'])

            #if 'lte' in setters.keys() and 'lte' in nodes:
            #        log.info("Setting configuration of node LTE") 
            #        controller.blocking(False).node(nodes['lte']).radio.iface('usrp').set_parameters(setters['lte'])

            #if 'nbiot' in setters.keys() and 'nbiot' in nodes:
            #        log.info("Setting configuration of node NB-IoT") 
            #        controller.blocking(False).node(nodes['nbiot']).radio.iface('usrp').set_parameters(setters['nbiot'])

            gevent.sleep(2)

    log.info("All nodes disconnected. Exiting controller")
    controller.stop()

if __name__ == '__main__':
   try:
   	exec_loop()
   except KeyboardInterrupt:
        controller.stop() 
