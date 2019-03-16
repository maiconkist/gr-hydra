# gr-hydra

[![Build Status](https://travis-ci.org/maiconkist/gr-hydra.svg?branch=bleeding)](https://travis-ci.org/maiconkist/gr-hydra)


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


## LICENSE

## ACKNOWLEDGEMENTS

## TROUBLESHOOTING ==

* Cmake cannot find FFTW ? 

   Put the file in https://github.com/jedbrown/cmake-modules/blob/master/FindFFTW.cmake in the folder /usr/share/cmake-2.8/Modules/
