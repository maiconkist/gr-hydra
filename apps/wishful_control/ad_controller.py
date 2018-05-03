#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
Usage:
   controller [options] [-q | -v]

Options:
   --logfile name       Name of the logfile
   --config configFile Config file path

Example:
   ./

Other options:
   -h, --help           show this help message and exit
   -q, --quiet           print less text
   -v, --verbose       print more text
   --version           show version and exit
"""

__author__ = "Domenico Garlisi, ...."
__copyright__ = "Copyright (c) ..., "
__version__ = "0.1.0"
__email__ = "domenico.garlisi@cnit.it; ..."

import threading
import datetime
import logging
import sys
import time
import gevent
import signal
import os
import yaml
import zmq
import json
import zmq
import _thread
import struct # for packing integers

sys.path.append('../')
from lib.kvsimple import KVMsg
from lib.zhelpers import zpipe

log = logging.getLogger('wishful_agent.main')

#clear = lambda: os.system('cls')
clear = lambda: os.system('clear')
do_run = True
solutionCompatibilityMatrix = {}


class Solution:
    """
    Structure to store solutions in order to get compatibility matrix
    """
    id = 0
    name = None
    solution = None
    commandList=None
    # eventList = None
    solutionCompatibilityList = None

    def __init__(self, id, name, solution, commandList=None, monitorList=None, solutionCompatibilityList=None):
        self.id = id
        self.name = name
        self.solution = solution
        self.commandList = commandList
        # self.eventList = eventList
        self.monitorList = monitorList
        self.solutionCompatibilityList = solutionCompatibilityList

    def store(self, dikt):
        """Store me in a dict if I have anything to store"""
        if self.id != 0 and self.name is not None:
            dikt[self.id] = self


def start_visualizer_connection(webui_ip_address):
    """
    ****    SETUP LOG GUI VISUALIZER    ****
    This function is used to setup the connection with the experiment webui,
    a ZMQ socket client is created on port 5506, able to send statistical information to GUI
    """
    socket_visualizer_port = "5506"
    ctx = zmq.Context()
    socket_visualizer = ctx.socket(zmq.PUB)
    socket_visualizer.connect("tcp://%s:%s" % (webui_ip_address, socket_visualizer_port))
    print("Connecting to server %s on port %s ... ready to send information to experiment WEBUI" % (webui_ip_address, socket_visualizer_port))
    return socket_visualizer



def start_command_connection():
    """
    ****    SETUP COMMAND GUI VISUALIZER    ****
    This function is used to setup the connection with the experiment GUI,
    a ZMQ socket server is created on port 8500, able to receive command from GUI
    """
    socket_command_visualizer_port = "8500"
    context = zmq.Context()
    socket_command_visualizer = context.socket(zmq.PAIR)
    socket_command_visualizer.bind("tcp://*:%s" % socket_command_visualizer_port)
    print("Create server on port %s ... ready to receive command from experiment GUI" % socket_command_visualizer_port)
    return socket_command_visualizer


def forward_lines(stream, socket):
    """Read lines from `stream` and send them over `socket`."""
    try:
        line = stream.readline()
        while line:
            socket.send(line[:-1].encode('utf-8'))
            line = stream.readline()
        socket.send(''.encode('utf-8'))  # send "eof message".
    finally:
        # NOTE: `zmq.Context.term()` in the main thread will block until this
        #       socket is closed, so we can't run this function in daemon
        #       thread hoping that it will just close itself.
        socket.close()


def forward_standard_input(context):
    """Start a thread that will bridge the standard input to a 0MQ socket and
    return an exclusive pair socket from which you can read lines retrieved
    from the standard input.  You will receive a final empty line when the EOF
    character is input to the keyboard."""
    reader = context.socket(zmq.PAIR)
    reader.connect('inproc://standard-input')
    writer = context.socket(zmq.PAIR)
    writer.bind('inproc://standard-input')
    stdin_thread = threading.Thread(target=forward_lines, args=(sys.stdin, writer))
    stdin_thread.start()
    return [reader, stdin_thread]


def print_log(logString):  ## Your menu design here
    if logString=="START":
        print(30 * "-", "LOG", 30 * "-")
    elif logString=="STOP":
        print(67 * "-")
    else:
        print(logString)
    return


def print_menu():  ## Your menu design here
    print(30 * "-", "MAIN MENU", 30 * "-")
    print("1. Print registered solutions")
    print("2. Send command")
    print("0. Quit")
    print(67 * "-")
    print("Enter your choice >>  ")


def print_solutions():
    global solutionCompatibilityMatrix
    if len(solutionCompatibilityMatrix) > 0:
        solutionIndex = 1
        print("Network list:")
        for key in solutionCompatibilityMatrix:
            print(str(solutionIndex) + " :")
            print("     " + str(solutionCompatibilityMatrix[key].name))
            print("     " + str(solutionCompatibilityMatrix[key].commandList))
            solutionIndex += 1
    print(67 * "-")
    print("0. BACK")
    print(67 * "-")
    print("Press 0 to BACK")
    while True:
        try:
            items = dict(poller.poll())
        except (zmq.ZMQError, KeyboardInterrupt):
            return

        if items.get(reader, 0) & zmq.POLLIN:
            line = reader.recv()
            break
        else:
            continue
    clear()
    return


def print_menu_command():  ## Your menu design here
    global poller
    global reader
    global sequence_publisher
    global publisher

    # Command menu (choice of network)
    print(30 * "-", "COMMAND MENU", 30 * "-")
    if len(solutionCompatibilityMatrix) > 0:
        solutionIndex = 1
        print("Network list:")
        for key in solutionCompatibilityMatrix:
            # print(key)
            print("     " + str(solutionIndex) + " :" +str(solutionCompatibilityMatrix[key].name))
            # print(str(solutionIndex) + " :" + str(solutionCompatibilityMatrix[key].commandList))
            solutionIndex += 1
    print("0. BACK")
    print(67 * "-")
    print("insert network number : ")

    # Poll for input on network choice:
    while True:
        try:
            items = dict(poller.poll())
        except (zmq.ZMQError, KeyboardInterrupt):
            return

        if items.get(reader, 0) & zmq.POLLIN:
            line = reader.recv()
            break
        else:
            continue

    # Parse the selected network:
    network_choice = line.decode("utf-8").lower()
    if network_choice == '':
        return
    else:
        try:
            solutionIndex = int(network_choice)
        except KeyError:
            print("Invalid selection, please try again.\n")
            return

    # Pass the commands of the network to the output:
    for command_index in  range(0,len(solutionCompatibilityMatrix[solutionIndex].commandList)):
        print("     " + str(command_index) + " :" + str(solutionCompatibilityMatrix[solutionIndex].commandList[command_index]))
    print("insert command number : ")

    while True:
        try:
            items = dict(poller.poll())
        except (zmq.ZMQError, KeyboardInterrupt):
            return
        if items.get(reader, 0) & zmq.POLLIN:
            line = reader.recv()
            break
        else:
            continue

    command_choice = line.decode("utf-8").lower()
    if command_choice == '':
        return
    else:
        try:
            commandIndex = int(command_choice)
        except KeyError:
            print("Invalid selection, please try again.\n")
            return

    # send command
    commandToSend = solutionCompatibilityMatrix[solutionIndex].commandList[commandIndex]

    # old json format
    # msg = {"type": "publisherUpdate", "involvedSolutions": solutionCompatibilityMatrix[key2].name, "commandList": commandList}
    # msg = {"type": "publisherUpdate", "involvedSolutions": solutionCompatibilityMatrix[solutionIndex].name, "commandList": commandToSend}
    # msg = {'type': 'publisherUpdate', 'involvedSolutions': 'WIFI', 'commandList': 'START_WIFI'}
    # new json format
    # {
    #     "type": "publisherUpdate",
    #           "involvedController": ["networkB" "networkC],
    #           "commandList": {"solutionName": {
    #          “LTE”: { “2467”:True,  “2484”:False}
    #          “WiFi”: { “2467”:False,  “2484”:True}}},
    #
    # }

    msg = {
        "type": "publisherUpdate",
        "involvedController": [solutionCompatibilityMatrix[solutionIndex].name],
        "commandList": {}
    }

    if commandToSend == "TRAFFIC":
        msg["commandList"] = {
            "TRAFFIC" : 1 # OFF, LOW, MEDIUM, HIGH
        }
    else:
        msg["commandList"] = {solutionCompatibilityMatrix[solutionIndex].solution[0]: {}}

        #WIFI NETWORK COMMAND
        if commandToSend == "START_WIFI":
            msg["commandList"][solutionCompatibilityMatrix[solutionIndex].solution[0]] = {"START_WIFI": {}}
            msg["commandList"][solutionCompatibilityMatrix[solutionIndex].solution[0]]["START_WIFI"]= {"2437": True}
        if commandToSend == "STOP_WIFI":
            msg["commandList"][solutionCompatibilityMatrix[solutionIndex].solution[0]] = {"STOP_WIFI": {}}
            msg["commandList"][solutionCompatibilityMatrix[solutionIndex].solution[0]]["STOP_WIFI"]= {"2437": False}
        if commandToSend == "START_TDMA":
            msg["commandList"][solutionCompatibilityMatrix[solutionIndex].solution[0]] = {"START_TDMA": {}}
            msg["commandList"][solutionCompatibilityMatrix[solutionIndex].solution[0]]["START_TDMA"] = {"2437": False}
        if commandToSend == "STOP_TDMA":
            msg["commandList"][solutionCompatibilityMatrix[solutionIndex].solution[0]] = {"STOP_TDMA": {}}
            msg["commandList"][solutionCompatibilityMatrix[solutionIndex].solution[0]]["STOP_TDMA"] = {"2437": False}

        #LTE NETWORK COMMAND
        if commandToSend == "START_LTE":
            msg["commandList"][solutionCompatibilityMatrix[solutionIndex].solution[0]] = {"START_LTE": {}}
            msg["commandList"][solutionCompatibilityMatrix[solutionIndex].solution[0]]["START_LTE"]= {"2437": True}
        if commandToSend == "STOP_LTE":
            msg["commandList"][solutionCompatibilityMatrix[solutionIndex].solution[0]] = {"STOP_LTE": {}}
            msg["commandList"][solutionCompatibilityMatrix[solutionIndex].solution[0]]["STOP_LTE"]= {"2437": False}
        if commandToSend == "ENABLE_LTE_2_SUBFRAME":
            msg["commandList"][solutionCompatibilityMatrix[solutionIndex].solution[0]] = {"ENABLE_LTE_2_SUBFRAME": {}}
            # msg["commandList"][solutionCompatibilityMatrix[solutionIndex].solution[0]]["ENABLE_LTE_2_SUBFRAME"] = {}
        if commandToSend == "ENABLE_LTE_4_SUBFRAME":
            msg["commandList"][solutionCompatibilityMatrix[solutionIndex].solution[0]] = {"ENABLE_LTE_4_SUBFRAME": {}}
            # msg["commandList"][solutionCompatibilityMatrix[solutionIndex].solution[0]]["ENABLE_LTE_4_SUBFRAME"] = {}
        if commandToSend == "ENABLE_LTE_6_SUBFRAME":
            msg["commandList"][solutionCompatibilityMatrix[solutionIndex].solution[0]] = {"ENABLE_LTE_6_SUBFRAME": {}}
            # msg["commandList"][solutionCompatibilityMatrix[solutionIndex].solution[0]]["ENABLE_LTE_6_SUBFRAME"] = {}

    if msg:
        print('update message %s' % str(msg))
        # Distribute as key-value message
        # sequence_publisher += 1
        kvmsg = KVMsg(sequence_publisher)
        kvmsg.key = b"generic"
        kvmsg.body = json.dumps(msg).encode('utf-8')
        kvmsg.send(publisher)
        print("sent update information with sequence %d" % sequence_publisher)
    else:
        print("Malformed message, sending failed")
        print('update message %s' % str(msg))

    print(67 * "-")
    print("Press 0 to BACK")
    while True:
        try:
            items = dict(poller.poll())
        except (zmq.ZMQError, KeyboardInterrupt):
            return

        if items.get(reader, 0) & zmq.POLLIN:
            line = reader.recv()
            break
        else:
            continue
    clear()
    return

# Exit program
def exit():
    global do_run
    do_run = False
    # sys.exit()

# Back to main menu
# def back():
#     menu_actions['main_menu']()

# =======================
#    MENUS DEFINITIONS
# =======================
# Menu definition
menu_actions = {
    # 'main_menu': main_menu,
    '1': print_solutions,
    '2': print_menu_command,
    # '9': back,
    '0': exit,
}

# Execute menu
def exec_menu(choice):
    clear()
    ch = choice.decode("utf-8").lower()
    if ch == '':
        return
    else:
        try:
            menu_actions[ch]()
        except KeyError:
            print("Invalid selection, please try again.\n")
    return

poller = None
reader = None
sequence_publisher = 0
publisher = None
def main(args):
    global do_run
    global solutionCompatibilityMatrix
    global poller
    global reader
    global sequence_publisher
    global publisher

    #Init variables
    kvmap = {}
    solutionNum = 0
    sequence_publisher = 0

    log.info('*******     WISHFUL *************')
    log.info('********  Starting solution global controller ***************')

    # all_nodes = ['B', 'C', 'D', 'E']
    # nodes_source_rate = []

    # Prepare our context and publisher socket
    ctx = zmq.Context()

    # Init Publisher socket
    publisher = ctx.socket(zmq.PUB)
    publisher.bind("tcp://*:7000")
    updates, peer = zpipe(ctx)

    # Init Request socket
    request = ctx.socket(zmq.ROUTER)
    request.bind("tcp://*:7001")

    poller = zmq.Poller()
    poller.register(request, zmq.POLLIN)

    # Init connection with GUI ffor LOG
    socket_visualizer = start_visualizer_connection("127.0.0.1")

    # Init conneciton with GUI for command
    # socket_command_visualizer = start_command_connection()
    # poller.register(socket_command_visualizer, flags=zmq.POLLIN)

    reader, stdin_thread = forward_standard_input(ctx)
    poller.register(reader, zmq.POLLIN)

    clear()
    print_log("START")
    print_log("STOP")
    do_run = True

    #Start main while, receive request and update information to solution
    while do_run:
        print_menu()  ## Displays menu

        try:
            items = dict(poller.poll())
        except (zmq.ZMQError, KeyboardInterrupt):
            break # interrupt/context shutdown

        if items.get(reader, 0) & zmq.POLLIN:
            line = reader.recv()
            exec_menu(line)

        """
        **** RECEIVED MESSAGE FROM SHOWCASE NETWORK
        """
        if request in items:
            clear()
            print_log("START")

            msg = request.recv_multipart()
            identity = msg[0]
            try:
                received_request = json.loads(msg[3].decode("utf-8"))
            except ValueError:
                print("Received message not in json format")
                continue

            # print(received_request)
            if "type" in received_request:
                # print(msg)

                # print("send to socket visualizer")
                # socket_visualizer.send_json(received_request)

                #Process registration request
                if received_request["type"] == "registerRequest":
                    print("***Received registration request*** : %s\n" % received_request)

                    # {'networkController': 'name', 'solution': ['WIFI_CT'], 'type': 'registerRequest', 'monitorList': ['THR', 'PER'],
                    # 'commandList': ['START_WIFI', 'STOP_WIFI', 'STOP_TDMA', 'START_TDMA']}

                    if "networkController" in received_request and "solution" in received_request and "commandList" in received_request and "monitorList" in received_request:
                        solutionAlreadyPresent = False
                        for key in solutionCompatibilityMatrix:
                            if solutionCompatibilityMatrix[key].name == received_request["networkController"]:
                                solutionAlreadyPresent = True
                                break
                        if not solutionAlreadyPresent:
                            solutionNum += 1
                            solution = Solution(solutionNum, received_request["networkController"], received_request["solution"], received_request["commandList"], received_request["monitorList"])
                            solution.store(solutionCompatibilityMatrix)

                        #TOBE
                        """
                        we need more attention when store the reference for a solution in the solutionCompatibilityMatrix, 
                        we need check the real conflict between the solutions in order to use it in the main logic of the 
                        solution global controller
                        """

                        responseMsg = {"type": "registerResponse", "result": "ok"}
                    else:
                        print("received malformed registration request")
                        responseMsg = {"type": "registerResponse", "result": "bad"}

                    # reply response to client
                    request.send(identity, zmq.SNDMORE)
                    # print('msg %s' % str(responseMsg))
                    # Distribute as key-value message
                    sequence = struct.unpack('!l',msg[2])[0] + 1
                    kvmsg = KVMsg(sequence)
                    kvmsg.key = b"generic"
                    kvmsg.body = json.dumps(responseMsg).encode('utf-8')
                    kvmsg.send(request)

                # Process monitoring Report
                if received_request["type"] == "monitorReport":
                    print("*** Received monitoring report *** : %s" % received_request)
                    if True:# len(solutionCompatibilityMatrix)>0:
                        """
                        This is the main logic of the solution global controller, after receiving a monitor report, the 
                        solution global controller, check the solution compatibility matrix, in order to understand 
                        eventually conflict, and send update/command to solution.
                        Example: when it receive the report of detecting interference (LTE), send command to solution TDMA cross interference, if not conflict are detected  
                        """
                        """
                        data = {
                            "type": "monitorReport", "monitorType": "performance", "networkController": ctrl,
                            "networkType": "80211",
                            "monitorValue": {
                                "timestamp": now,
                                "PER": random.random(),
                                "THR": random.random() * 1e6,
                            },
                        }
                        """
                        socket_visualizer.send_multipart([ b'monitorReport', json.dumps(received_request).encode('utf-8'), ])
                    else:
                        print("Received monitor report but no solution is registered")

                # if received_request["type"] == "eventReport":
                #     print("*** Received event report *** : %s" % received_request)
                #     # print("*** Received event report *** : \n" )
                #     if len(solutionCompatibilityMatrix) > 0:
                #         # TOBE
                #         """
                #         This is the main logic of the solution global controller, after receiving a event report, the
                #         solution global controller, check the solution compatibility matrix, in order to understand
                #         eventually conflict, and send update/command to solution.
                #         Example: when it receive the report of detecting interference (LTE), send command to solution TDMA cross interference, if not conflict are detected
                #         """
                #
                #         for key in solutionCompatibilityMatrix:
                #             print(str(solutionCompatibilityMatrix[key].name))
                #             if received_request["solution"] == solutionCompatibilityMatrix[key].name:
                #
                #                     for key2 in solutionCompatibilityMatrix:
                #                         if solutionCompatibilityMatrix[key2].name == "lte_ct":
                #                             if received_request["eventType"] == "WiFi_DETECTED":
                #                                 if "START_TDMA" in solutionCompatibilityMatrix[key2].commandList:
                #                                     commandList = "START_TDMA"
                #                                     # Distribute as key-value message
                #                                     sequence_publisher += 1
                #                                     kvmsg = KVMsg(sequence_publisher)
                #                                     kvmsg.key = b"generic"
                #                                     msg = {"type": "publisherUpdate", "involvedSolutions": solutionCompatibilityMatrix[key2].name, "commandList": commandList}
                #                                     print('update message %s' % str(msg))
                #                                     kvmsg.body = json.dumps(msg).encode('utf-8')
                #                                     # kvmsg.send(publisher)
                #                                     print("sent update information with sequence %d" % sequence_publisher)
                #
                #                             if received_request["eventType"] == "LTE_DETECTED":
                #                                 if "START_TDMA" in solutionCompatibilityMatrix[key2].commandList:
                #                                     commandList = "START_TDMA"
                #                                     # Distribute as key-value message
                #                                     sequence_publisher += 1
                #                                     kvmsg = KVMsg(sequence_publisher)
                #                                     kvmsg.key = b"generic"
                #                                     msg = {"type": "publisherUpdate", "involvedSolutions": solutionCompatibilityMatrix[key2].name, "commandList": commandList}
                #                                     print('update message %s' % str(msg))
                #                                     kvmsg.body = json.dumps(msg).encode('utf-8')
                #                                     # kvmsg.send(publisher)
                #                                     print("sent update information with sequence %d" % sequence_publisher)
                #
                #                             if received_request["eventType"] == "ZIGBEE_DETECTED":
                #                                 if "START_TDMA" in solutionCompatibilityMatrix[key2].commandList:
                #                                     commandList = "START_TDMA"
                #                                     # Distribute as key-value message
                #                                     sequence_publisher += 1
                #                                     kvmsg = KVMsg(sequence_publisher)
                #                                     kvmsg.key = b"generic"
                #                                     msg = {"type": "publisherUpdate", "involvedSolutions": solutionCompatibilityMatrix[key2].name, "commandList": commandList}
                #                                     print('update message %s' % str(msg))
                #                                     kvmsg.body = json.dumps(msg).encode('utf-8')
                #                                     # kvmsg.send(publisher)
                #                                     print("sent update information with sequence %d" % sequence_publisher)
                #
                #                             if received_request["eventType"] == "NO_interference":
                #                                 if "STOP_TDMA" in solutionCompatibilityMatrix[key2].commandList:
                #                                     commandList = "STOP_TDMA"
                #                                     # Distribute as key-value message
                #                                     sequence_publisher += 1
                #                                     kvmsg = KVMsg(sequence_publisher)
                #                                     kvmsg.key = b"generic"
                #                                     msg = {"type": "publisherUpdate", "involvedSolutions": solutionCompatibilityMatrix[key2].name, "commandList": commandList}
                #                                     print('update message %s' % str(msg))
                #                                     kvmsg.body = json.dumps(msg).encode('utf-8')
                #                                     # kvmsg.send(publisher)
                #                                     print("sent update information with sequence %d" % sequence_publisher)
                #                             break
                #
                #                     break
                #
                #
                #     else:
                #         print("Received event report but no solution is registered")

                # Process command Report
                if received_request["type"] == "commandReport":
                    print("*** Received command report *** : %s\n" % received_request)
                    if len(solutionCompatibilityMatrix) > 0:
                        pass
                    else:
                        print("Received command report but no solution is registered")


        # """
        # **** RECEIVED COMMAND FROM SHOWCASE GUI
        # """
        # if socket_command_visualizer in items:
        #     parsed_json = socket_command_visualizer.recv_json()
        #     print('parsed_json : %s' % str(parsed_json))
        #     type = parsed_json['type']
        #     if type == 'traffic':
        #         node = parsed_json['src']
        #         # node_src_index = all_nodes.index(node)
        #         command = parsed_json['command']
        #         # nodes_source_rate[node_src_index] = 0
        #         if command == 'off_traffic':
        #             # if off traffic is selected for a specific node
        #             if node == 'D':
        #                 #off LTE network
        #                 commandList = "STOP_LTE"
        #                 for key2 in solutionCompatibilityMatrix:
        #                     if solutionCompatibilityMatrix[key2].name == "lte_ct":
        #                         if commandList in solutionCompatibilityMatrix[key2].commandList:
        #                             # Distribute as key-value message
        #                             sequence_publisher += 1
        #                             kvmsg = KVMsg(sequence_publisher)
        #                             kvmsg.key = b"generic"
        #                             msg = {"type": "publisherUpdate", "involvedSolutions": solutionCompatibilityMatrix[key2].name, "commandList": commandList}
        #                             print('update message %s' % str(msg))
        #                             kvmsg.body = json.dumps(msg).encode('utf-8')
        #                             kvmsg.send(publisher)
        #                             print("sent update information with sequence %d" % sequence_publisher)
        #                         break
        #             else:
        #                 # off WiFi network
        #                 commandList = "STOP_WIFI"
        #                 for key2 in solutionCompatibilityMatrix:
        #                     if solutionCompatibilityMatrix[key2].name == "lte_ct":
        #                         if commandList in solutionCompatibilityMatrix[key2].commandList:
        #                             # Distribute as key-value message
        #                             sequence_publisher += 1
        #                             kvmsg = KVMsg(sequence_publisher)
        #                             kvmsg.key = b"generic"
        #                             msg = {"type": "publisherUpdate", "involvedSolutions": solutionCompatibilityMatrix[key2].name, "commandList": commandList}
        #                             print('update message %s' % str(msg))
        #                             kvmsg.body = json.dumps(msg).encode('utf-8')
        #                             kvmsg.send(publisher)
        #                             print("sent update information with sequence %d" % sequence_publisher)
        #                         break
        #
        #         # if start traffic is selected for a specific node
        #         if command == 'set_traffic':
        #             node = parsed_json['dst']
        #             # node_dst_index = all_nodes.index(node)
        #             # value = parsed_json['value']
        #             # nodes_source_rate[node_src_index] = value
        #             # source_rate = float(nodes_source_rate[node_src_index])
        #             # call UPI to start traffic
        #             if node == 'E':
        #                 # on LTE network
        #                 commandList = "START_LTE"
        #                 for key2 in solutionCompatibilityMatrix:
        #                     if solutionCompatibilityMatrix[key2].name == "lte_ct":
        #                         if commandList in solutionCompatibilityMatrix[key2].commandList:
        #                             # Distribute as key-value message
        #                             sequence_publisher += 1
        #                             kvmsg = KVMsg(sequence_publisher)
        #                             kvmsg.key = b"generic"
        #                             msg = {"type": "publisherUpdate", "involvedSolutions": solutionCompatibilityMatrix[key2].name, "commandList": commandList}
        #                             print('update message %s' % str(msg))
        #                             kvmsg.body = json.dumps(msg).encode('utf-8')
        #                             kvmsg.send(publisher)
        #                             print("sent update information with sequence %d" % sequence_publisher)
        #                         break
        #
        #             else:
        #                 # on WiFi network
        #                 commandList = "START_WIFI"
        #                 for key2 in solutionCompatibilityMatrix:
        #                     if solutionCompatibilityMatrix[key2].name == "WIFI":
        #                         if commandList in solutionCompatibilityMatrix[key2].commandList:
        #                             # Distribute as key-value message
        #                             sequence_publisher += 1
        #                             kvmsg = KVMsg(sequence_publisher)
        #                             kvmsg.key = b"generic"
        #
        #                             # old json format
        #                             # msg = {"type": "publisherUpdate", "involvedSolutions": solutionCompatibilityMatrix[key2].name, "commandList": commandList}
        #
        #                             # new json format
        #                             # {
        #                             #     "type": "publisherUpdate",
        #                             #           "involvedController": ["networkB" "networkC],
        #                             #           "commandList": {"solutionName": {
        #                             #          “LTE”: { “2467”:True,  “2484”:False}
        #                             #          “WiFi”: { “2467”:False,  “2484”:True}}},
            #                         #
        #                             # }
        #                             msg = {"type": "publisherUpdate", "involvedController": [solutionCompatibilityMatrix[key2].name], "commandList":{ "solutionName": { "WIFI": { "2437":True} }}}
        #                             print('update message %s' % str(msg))
        #                             kvmsg.body = json.dumps(msg).encode('utf-8')
        #                             kvmsg.send(publisher)
        #                             print("sent update information with sequence %d" % sequence_publisher)
        #                         break
        #
        #     if type == 'radio_program':
        #         command = parsed_json['command']
        #         if command == 'on_tdma':
        #             for key2 in solutionCompatibilityMatrix:
        #                 if solutionCompatibilityMatrix[key2].name == "lte_ct":
        #                     if "START_TDMA" in solutionCompatibilityMatrix[key2].commandList:
        #                         commandList = "START_TDMA"
        #                         # Distribute as key-value message
        #                         sequence_publisher += 1
        #                         kvmsg = KVMsg(sequence_publisher)
        #                         kvmsg.key = b"generic"
        #                         msg = {"type": "publisherUpdate", "involvedSolutions": solutionCompatibilityMatrix[key2].name, "commandList": commandList}
        #                         print('update message %s' % str(msg))
        #                         kvmsg.body = json.dumps(msg).encode('utf-8')
        #                         kvmsg.send(publisher)
        #                         print("sent update information with sequence %d" % sequence_publisher)
        #                     break
        #         if command == 'off_tdma':
        #             for key2 in solutionCompatibilityMatrix:
        #                 if solutionCompatibilityMatrix[key2].name == "lte_ct":
        #                     if "STOP_TDMA" in solutionCompatibilityMatrix[key2].commandList:
        #                         commandList = "STOP_TDMA"
        #                         # Distribute as key-value message
        #                         sequence_publisher += 1
        #                         kvmsg = KVMsg(sequence_publisher)
        #                         kvmsg.key = b"generic"
        #                         msg = {"type": "publisherUpdate", "involvedSolutions": solutionCompatibilityMatrix[key2].name, "commandList": commandList}
        #                         print('update message %s' % str(msg))
        #                         kvmsg.body = json.dumps(msg).encode('utf-8')
        #                         kvmsg.send(publisher)
        #                         print("sent update information with sequence %d" % sequence_publisher)
        #                     break

    # stdin_thread._stop()



if __name__ == "__main__":
    try:
        from docopt import docopt
    except:
        print(""" Please install docopt using:
            pip install docopt==0.6.1
            For more refer to: https://github.com/docopt/docopt """)
        raise

    args = docopt(__doc__, version=__version__)
    log_level = logging.INFO  # default
    if args['--verbose']:
        log_level = logging.DEBUG
    elif args['--quiet']:
        log_level = logging.ERROR

    logfile = None
    if args['--logfile']:
        logfile = args['--logfile']

    logging.basicConfig(filename=logfile, level=log_level,
        format='%(asctime)s - %(name)s.%(funcName)s() - %(levelname)s - %(message)s')

    #Get configuration file
    if args['--config']:
        config_file_path = args['--config']
        config = None
        with open(config_file_path, 'r') as f:
            config = yaml.load(f)
    try:
        main(args)
        print('end main')
    except KeyboardInterrupt:
        log.debug("Controller exits")
    finally:
        log.debug("Exit")
