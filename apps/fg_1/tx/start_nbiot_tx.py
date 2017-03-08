#!/bin/env sh

./lte_nbiot_chain.py --vr-configuration 2 --vr2-next-container-ip tcp://192.168.122.208:4001 --vr2-file ../../vr2fifo
#./lte_nbiot_chain.py --vr-configuration 2 --vr2-next-container-ip tcp://127.0.0.1:4001 --vr2-file ../../vr2fifo
