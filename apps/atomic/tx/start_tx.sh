#!/usr/bin/env sh


# works with:
#     B210, direct cable connection
python hydra_tx.py --vr1-file ../../vr1fifo --vr2-file ../../vr2fifo --vr1-tx-amplitude 0.2 --vr2-tx-amplitude 0.1 -A J1 --tx-gain 60
