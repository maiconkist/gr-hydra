# gr-hydra

[![Build Status](https://travis-ci.org/maiconkist/gr-hydra.svg?branch=bleeding)](https://travis-ci.org/maiconkist/gr-hydra)
[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)

## GETTING STARTED

### REQUIRED LIBS

* build-essential
* cmake
* libfftw3-dev
* uhd-host
* libuhd-dev
* gnuradio-dev
* libopencv-dev
* libzmqpp-dev
* swig

```
sudo apt install build-essential cmake libfftw3-dev uhd-host libuhd-dev gnuradio-dev libopencv-dev libzmqpp-dev swig
```

## INSTALLATION

#### STANDARD VERSION - NO GNURADIO SUPPORT

Clone the repository and enter in the created folder (default: gr-hydra). Execute the following commands:

```
mkdir build; cd build;
cmake ../
make
sudo make install
sudo ldconfig
```

#### WITH GNURADIO SUPPORT

First install the STANDARD VERSION. Go to gr-hydra/grc_blocks and execute the following commands:

```
mkdir build; cd build;
cmake ../
make
sudo make install
```

## RUNNING THE EXAMPLES

### NOMENCLATURE

For all examples we:

- We associate a terminal ID to a all terminals open in the form **T_n**, where **n** is the terminal number. Wherewer needed we are going to refer to a specific terminal by specifying its ID.

- refer to **USRP_n** as a pair of **PC+USRP**, where **n** indentifies one of the pair. Ex: USRP_1 is what you consider your 1st PC+USRP, while **USRP_2** is your second. Whenever we refer to **USRP_1** you execute the instructions in that machine. 


### EXAMPLE 1
#### HARDWARE REQUIREMENTS: 2x [USRP] -- IDEAL: 3x [USRP]


1. Open a terminal in USRP_1 (**T1**). Start the Hydra-Server flowgraph:
```
python grc_blocks/app/hydra_gr_server_example.py
```
2. Open a terminal in USRP_1 (**T2**). Start the client that request 2 end-to-end TX/RX slices:
```
python grc_blocks/app/hydra_gr_client_2tx_2rx_pdu_gen.py
```
3. Open a terminal in USRP_2 (**T3**). Start the flowgraph that receives the messages (PDU) transmitted in slice 1:
```
python grc_blocks/app/usrp_vr1_pdu_gen.grc
```
4. In **T3** you should see the log of messages being received from USRP_1: 
```
LOG
LOG
LOG
LOG
```
5. In **T2** you should see the log of messages being received from USRP_2:
```
LOG
LOG
LOG
LOG
```
6. If you have only 2 USRP: stop the flowgraph in **T3**. Otherwise go to next step.
7.  Open a terminal (**T4**) in USRP_3 (or USRP_2 if you have only 2 USRPs).  Start the flowgraph that receives the messages (PDU) transmitted in slice 2:
```
python grc_blocks/app/usrp_vr2_pdu_gen.grc
```
8. In **T2** you should see the log of messages being received from **USRP_3** (or **USRP_2**):
```
LOG
LOG
LOG
LOG
```
9. In **T4** you should see the log of messages being received from USRP_1:
```
LOG
LOG
LOG
LOG
```


### EXAMPLE 2
#### HARDWARE REQUIREMENTS: 


## LICENSE

## ACKNOWLEDGEMENTS

## TROUBLESHOOTING

* Cmake cannot find FFTW ? 

   Put the file in https://github.com/jedbrown/cmake-modules/blob/master/FindFFTW.cmake in the folder /usr/share/cmake-2.8/Modules/
