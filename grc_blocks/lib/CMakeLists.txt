# Copyright 2011,2012 Free Software Foundation, Inc.
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

add_library(gnuradio-hydra SHARED
    hydra_gr_client_sink_impl.cc
    hydra_gr_client_source_impl.cc
    hydra_gr_server_impl.cc)

target_compile_features(gnuradio-hydra PUBLIC cxx_std_17)
target_compile_definitions(gnuradio-hydra PUBLIC BOOST_LOG_DYN_LINK)
target_include_directories(gnuradio-hydra PUBLIC
  "${PROJECT_SOURCE_DIR}/include"
  "${HYDRA_INCLUDE_DIR}"
  ${UHD_INCLUDE_DIRS}
  ${OpenCV_INCLUDE_DIRS}
  ${FFTW_INCLUDES}
  ${ZeroMQ_INCLUDE_DIR})
target_link_libraries(gnuradio-hydra
  PUBLIC
    "${HYDRA_LIBRARY}"
    gnuradio::gnuradio-runtime
    Boost::filesystem
    Boost::system
    Boost::log
    Boost::log_setup
    ${UHD_LIBRARIES}
    ${OpenCV_LIBRARIES}
    ${FFTW_LIBRARIES}
  PRIVATE
    gnuradio::gnuradio-zeromq)
set_target_properties(gnuradio-hydra PROPERTIES
  CXX_EXTENSIONS OFF
  CXX_VISIBILITY_PRESET hidden
  VISIBILITY_INLINES_HIDDEN ON
  DEFINE_SYMBOL gnuradio_hydra_EXPORTS)

install(TARGETS gnuradio-hydra
  LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}")
