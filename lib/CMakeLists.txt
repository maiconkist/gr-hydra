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
list(APPEND svl_sources
    svl_virtual_radio.cc
	 svl_hypervisor.cc
    svl_sink_impl.cc
    svl_source_impl.cc )

set(svl_sources "${svl_sources}" PARENT_SCOPE)
if(NOT svl_sources)
	MESSAGE(STATUS "No C++ sources... skipping lib/")
	return()
endif(NOT svl_sources)

add_library(gnuradio-svl SHARED ${svl_sources})
target_link_libraries(gnuradio-svl ${Boost_LIBRARIES} ${GNURADIO_ALL_LIBRARIES} ${FFTW_LIBRARIES})
set_target_properties(gnuradio-svl PROPERTIES DEFINE_SYMBOL "gnuradio_svl_EXPORTS")

if(APPLE)
    set_target_properties(gnuradio-svl PROPERTIES
        INSTALL_NAME_DIR "${CMAKE_INSTALL_PREFIX}/lib"
    )
endif(APPLE)

########################################################################
# Install built library files
########################################################################
install(TARGETS gnuradio-svl
    LIBRARY DESTINATION lib${LIB_SUFFIX} # .so/.dylib file
    ARCHIVE DESTINATION lib${LIB_SUFFIX} # .lib file
    RUNTIME DESTINATION bin              # .dll file
)

########################################################################
# Build and register unit test
########################################################################
include(GrTest)

include_directories(${CPPUNIT_INCLUDE_DIRS})

list(APPEND test_svl_sources
    ${CMAKE_CURRENT_SOURCE_DIR}/test_svl.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/qa_svl.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/qa_svl_sink.cc
)

add_executable(test-svl ${test_svl_sources})

target_link_libraries(
  test-svl
  ${GNURADIO_RUNTIME_LIBRARIES}
  ${Boost_LIBRARIES}
  ${CPPUNIT_LIBRARIES}
  gnuradio-svl
)

GR_ADD_TEST(test_svl test-svl)
