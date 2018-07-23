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

########################################################################
# Setup library
########################################################################
include(GrPlatform) #define LIB_SUFFIX

include_directories(${Boost_INCLUDE_DIR})
link_directories(${Boost_LIBRARY_DIRS})
list(APPEND hydra_sources
    hydra_fft.cc
    hydra_virtual_radio.cc
	 hydra_hypervisor.cc
    hydra_sink.cc
    hydra_async_sink.cc
	 hydra_source.cc
    hydra_buffer.cc
    hydra_socket.cc
    hydra_stats.cc
    hydra_resource.cc
    hydra_server.cc
    hydra_client.cc
    hydra_core.cc
    hydra_main.cc
    hydra_uhd_interface.cc
)

set(hydra_sources "${hydra_sources}" PARENT_SCOPE)
if(NOT hydra_sources)
	MESSAGE(STATUS "No C++ sources... skipping lib/")
	return()
endif(NOT hydra_sources)

add_library(gnuradio-hydra STATIC ${hydra_sources})
target_link_libraries(gnuradio-hydra
	${Boost_LIBRARIES}
	${GNURADIO_ALL_LIBRARIES}
	${FFTW_LIBRARIES}
   ${ZeroMQ_LIBRARY}
   ${UHD_LIBRARIES}
	fftw3f
	fftw3l
   opencv_core
   opencv_imgcodecs
)
set_target_properties(gnuradio-hydra PROPERTIES DEFINE_SYMBOL "gnuradio_hydra_EXPORTS")

if(APPLE)
    set_target_properties(gnuradio-hydra PROPERTIES
        INSTALL_NAME_DIR "${CMAKE_INSTALL_PREFIX}/lib"
    )
endif(APPLE)

########################################################################
# Install built library files
########################################################################
install(TARGETS gnuradio-hydra
    LIBRARY DESTINATION lib${LIB_SUFFIX} # .so/.dylib file
    ARCHIVE DESTINATION lib${LIB_SUFFIX} # .lib file
    RUNTIME DESTINATION bin              # .dll file
)

########################################################################
# Build and register unit test
########################################################################
include(GrTest)

include_directories(${CPPUNIT_INCLUDE_DIRS})

list(APPEND test_hydra_sources
    ${CMAKE_CURRENT_SOURCE_DIR}/test_hydra.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/qa_hydra.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/qa_hydra_sink.cc
)

add_executable(test-hydra ${test_hydra_sources})

target_link_libraries(
  test-hydra
  ${GNURADIO_RUNTIME_LIBRARIES}
  ${Boost_LIBRARIES}
  ${CPPUNIT_LIBRARIES}
  ${FFTW_LIBRARIES}
  ${ZeroMQ_LIBRARY}
  ${UHD_LIBRARY}
  fftw3f
  fftw3l
  gnuradio-hydra
  opencv_core
  opencv_imgcodecs
)

GR_ADD_TEST(test_hydra test-hydra)
