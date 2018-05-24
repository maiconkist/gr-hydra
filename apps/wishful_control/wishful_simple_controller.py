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
from lib.global_controller_proxy import GlobalSolutionControllerProxy

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
controller.set_controller_info(name="TCD_RadioVirtualization", info="WishfulControllerInfo")
# ::TRICKY:: update IP addresses to external interface
controller.add_module(moduleName="discovery",
	pyModuleName="wishful_module_discovery_pyre",
	className="PyreDiscoveryControllerModule",
	kwargs={"iface":"eth0", "groupName":"tcd_hydra", "downlink":"tcp://172.16.16.5:8990", "uplink":"tcp://172.16.16.5:8989"})


lte_enabled = False
nbiot_enabled = False
solutionCtrProxy = None
nodes = {}
the_variables = {}

TOTAL_NODES = 3
NODE_NAMES = ["tx", "lte", "nbiot"]

conf = {

    # list of files that will be send to agents
    'files' : {
		"tx"    : "/root/gr-hydra/apps/atomic/tx/tx.py", 
		"lte"   : "/root/gr-hydra/apps/atomic/rx/lte.py", 
		"nbiot" : "/root/gr-hydra/apps/atomic/rx/nbiot.py", 
    },

    'program_getters' : {
        "tx":    ["cpu_percent", "lte_rate", "nbiot_rate" ],
        "lte":   ["cpu_percent", "rx_goodput", "rx_rate"],
        "nbiot": ["cpu_percent", "rx_goodput", "rx_rate"],
    },

    'program_args': {
        "tx":  [""], 
        "lte": [""],
        "nbiot": [""],
    },
}

def _toogle_solution(mode, toogle):

    print("XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX")
    param = 'amplitude1' if mode == 'LTE' else 'amplitude2'
    val = 0.1 if toogle == 'ON' else 0.0

    if 'tx' in nodes:
        try:
            print("XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX")
            controller.node(nodes['tx']).radio.iface('usrp').set_parameters({param: val})
        except Exception as e:
            print("Caught Except in _toogle_solution:" + str(e))


def enable_lte_solution():
    global lte_enabled
    lte_enabled = True


def disable_lte_solution():
    global lte_enabled
    lte_enabled = False

def enable_nbiot_solution():
    global nbiot_enabled
    nbiot_enabled = True

def disable_nbiot_solution():
    global nbiot_enabled
    nbiot_enabled = False


@controller.new_node_callback()
def new_node(node):
    log.info("New node appeared: Name: %s" % (node.name, ))
    nodes[node.name] = node

    if node.name in ['tx', 'lte', 'nbiot']:
        program_name = node.name
        program_code = open(conf['files'][program_name], "r").read()
        program_args = conf['program_args'][node.name]

        controller.blocking(False).node(node).radio.iface('usrp').activate_radio_program({'program_name': program_name, 'program_code': program_code, 'program_args': program_args,'program_type': 'py', 'program_port': 1235})


@controller.node_exit_callback()
def node_exit(node, reason):
    if node in nodes.values():
        del nodes[node.name]
    log.info(("NodeExit : NodeID : {} Reason : {}".format(node.id, reason)))


@controller.set_default_callback()
def default_callback(group, node, cmd, data):
    log.info("{} DEFAULT CALLBACK : Group: {}, NodeName: {}, Cmd: {}, Returns: {}".format(datetime.datetime.now(), group, node.name, cmd, data))


@controller.add_callback(upis.radio.get_parameters)
def get_vars_response(group, node, data):
    log.info("{} get_vars_response : Group:{}, NodeId:{}, msg:{}".format(datetime.datetime.now(), group, node.id, data))


    if node.name == 'lte':
        value = {
            "THR" : data['rx_goodput'] ,
            "PER" : 0,
            "timestamp" : time.time(),
        }
        solutionCtrProxy.send_monitor_report("performance", "LTE_virt",  value)
    elif node.name == 'nbiot':
        value = {
            "THR" : data['rx_goodput'] ,
            "PER" : 0,
            "timestamp" : time.time(),
        }
        solutionCtrProxy.send_monitor_report("performance", "NB-IoT Virtual",  value)
    elif node.name == "tx":
        pass


def exec_loop():
    global lte_enabled
    global nbiot_enabled
    global solutionCtrProxy

    'Discovered Controller'
    """
    ****** setup the communication with the solution global controller ******
    """
    solutionCtrProxy = GlobalSolutionControllerProxy(ip_address="172.16.16.5", requestPort=7001, subPort=7000)
    commands = {
            "START_LTE": enable_lte_solution,
            "STOP_LTE": disable_lte_solution,

            "START_NBIOT": enable_nbiot_solution,
            "STOP_NBIOT": disable_nbiot_solution,
    }
    solutionCtrProxy.set_solution_attributes(
                    controllerName = "LTE_virt",
                    networkType = "LTE-U",
                    solutionName = ["Radio Virtualization"],
                    commands = commands,
                    monitorList = ["LTE-THR", "LTE-PER"]) 
    # Register SpectrumSensing solution to global solution controller
    response = solutionCtrProxy.register_solution()
    if response:
        print("Radio Virtualization was correctly registered to GlobalSolutionController")
        solutionCtrProxy.start_command_listener()
    else:
        print("Radio Virtualization was not registered to GlobalSolutionController")
        sys.exit(1)

    #Start controller
    controller.start()

    while len(nodes) < TOTAL_NODES or (not lte_enabled and not nbiot_enabled):
        # Waiting for all nodes to connect
        log.info("%d/%d nodes connected. LTE enabled: %s, NB-IoT enabled: %s"  % (len(nodes), TOTAL_NODES, lte_enabled, nbiot_enabled))
        gevent.sleep(2)

    log.info("All nodes connected. Starting showcase...")

    #control loop
    while nodes:
            # TRICKY: gets are assynchronous. callback for get_parameters is called automatically
            if 'tx' in nodes:
                log.info("Requesting data to VR TX")
                controller.blocking(False).node(nodes['tx']).radio.iface('usrp').get_parameters(conf['program_getters']['tx'])

            if 'lte' in nodes:
                _toogle_solution('LTE', 'ON' if lte_enabled else 'OFF') 
                controller.blocking(False).node(nodes['lte']).radio.iface('usrp').get_parameters(conf['program_getters']['lte'])

            if 'nbiot' in nodes:
                _toogle_solution('NB-IoT', 'ON' if nbiot_enabled else 'OFF') 
                controller.blocking(False).node(nodes['nbiot']).radio.iface('usrp').get_parameters(conf['program_getters']['nbiot'])


            gevent.sleep(2)

    log.info("All nodes disconnected. Exiting controller")
    controller.stop()


if __name__ == '__main__':
    try:
        exec_loop()
    except KeyboardInterrupt:
       controller.stop() 
