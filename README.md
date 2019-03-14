# gr-hydra

[![Build Status](https://travis-ci.org/maiconkist/gr-hydra.svg?branch=bleeding)](https://travis-ci.org/maiconkist/gr-hydra)


## REQUIRED LIBS

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


## TROUBLESHOOTING ==

* Cmake cannot find FFTW ? 

   Put the file in https://github.com/jedbrown/cmake-modules/blob/master/FindFFTW.cmake in the folder /usr/share/cmake-2.8/Modules/
