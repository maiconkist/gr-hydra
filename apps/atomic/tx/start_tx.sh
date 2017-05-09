#!/usr/bin/env sh


# works with:
#     B210, direct cable connection
python hydra_tx.py --lte-file ../../vr1fifo --nbiot-file ../../vr2fifo --lte-tx-amplitude 0.2 --nbiot-tx-amplitude 0.1 -A J1 --tx-gain 60
