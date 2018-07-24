########################################################################
# Setup library
########################################################################
include_directories(${Boost_INCLUDE_DIR})
link_directories(${Boost_LIBRARY_DIRS})
list(APPEND hydra_sources
    hydra_fft.cc
    hydra_virtual_radio.cc
    hydra_hypervisor.cc
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

add_library(hydra STATIC ${hydra_sources})
target_link_libraries(hydra
	${Boost_LIBRARIES}
	${FFTW_LIBRARIES}
   ${ZeroMQ_LIBRARY}
   ${UHD_LIBRARIES}
   ${OpenCV_LIBRARIES}
   ${OpenCV_LIBRARY}
   fftw3f
   fftw3l
   pthread
)
set_target_properties(hydra PROPERTIES DEFINE_SYMBOL "hydra_EXPORTS")

if(APPLE)
    set_target_properties(hydra PROPERTIES
        INSTALL_NAME_DIR "${CMAKE_INSTALL_PREFIX}/lib"
    )
endif(APPLE)

########################################################################
# Install built library files
########################################################################
install(TARGETS hydra
    LIBRARY DESTINATION lib${LIB_SUFFIX} # .so/.dylib file
    ARCHIVE DESTINATION lib${LIB_SUFFIX} # .lib file
    RUNTIME DESTINATION bin              # .dll file
)
