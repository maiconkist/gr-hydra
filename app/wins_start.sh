#!/bin/bash

if [[ $HOSTNAME == *"server"* ]]; then
	echo "Running server app"
	python ~/gr-hydra/grc_blocks/app/ansible_hydra_gr_server.py
elif [[ $HOSTNAME == *"rx1"* ]]; then
	echo "Running rx1 app"
	python ~/gr-hydra/grc_blocks/app/ansible_hydra_vr1_rx.py
elif [[ $HOSTNAME == *"rx2"* ]]; then
	echo "Running rx2 app"
	python ~/gr-hydra/grc_blocks/app/ansible_hydra_vr2_rx.py
elif [[ $HOSTNAME == *"client"* ]]; then
	echo "Running client app"
	python ~/gr-hydra/grc_blocks/app/ansible_hydra_gr_client_2tx_2rx.py
fi
