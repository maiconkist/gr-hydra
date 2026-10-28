# Copyright 2011,2012,2014,2016 Free Software Foundation, Inc.
#
# This file is part of GNU Radio
#
# GNU Radio is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3, or (at your option)
# any later version.
#
# GNU Radio is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with GNU Radio; see the file COPYING.  If not, write to
# the Free Software Foundation, Inc., 51 Franklin Street,
# Boston, MA 02110-1301, USA.

cmake_minimum_required(VERSION 3.16)
project(gnuradio-hydra VERSION 1.0 LANGUAGES C CXX)

if(DEFINED ENV{PYBOMBS_PREFIX})
  set(CMAKE_INSTALL_PREFIX "$ENV{PYBOMBS_PREFIX}")
endif()
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

include(GNUInstallDirs)
find_package(Gnuradio 3.10 REQUIRED COMPONENTS blocks zeromq)
find_package(Boost REQUIRED COMPONENTS filesystem system log log_setup)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../cmake/Modules")
find_package(UHD REQUIRED)
find_package(OpenCV REQUIRED)
find_package(FFTW REQUIRED)
find_path(ZeroMQ_INCLUDE_DIR NAMES zmq.hpp)
find_path(HYDRA_INCLUDE_DIR NAMES hydra/hydra_main.h
  HINTS "${CMAKE_CURRENT_SOURCE_DIR}/../include")
find_library(HYDRA_LIBRARY NAMES hydra
  HINTS "${CMAKE_CURRENT_SOURCE_DIR}/../build/lib")

if(NOT HYDRA_INCLUDE_DIR OR NOT HYDRA_LIBRARY)
  message(FATAL_ERROR
    "Build the core first; set HYDRA_INCLUDE_DIR and HYDRA_LIBRARY if needed.")
endif()
if(NOT ZeroMQ_INCLUDE_DIR)
  message(FATAL_ERROR "The cppzmq headers are required.")
endif()

set(GR_INCLUDE_DIR "${CMAKE_INSTALL_INCLUDEDIR}/hydra")
add_subdirectory(include/hydra)
add_subdirectory(lib)
add_subdirectory(python)
add_subdirectory(grc)

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake" @ONLY)
add_custom_target(uninstall
  COMMAND "${CMAKE_COMMAND}" -P "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake")

install(FILES cmake/Modules/hydraConfig.cmake
  DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/hydra")
