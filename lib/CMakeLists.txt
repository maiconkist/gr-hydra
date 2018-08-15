add_library(hydraclient hydra_client.cc)
target_link_libraries(hydraclient PUBLIC ${ZeroMQ_LIBRARY})

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

add_library(hydra ${hydra_sources})
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

########################################################################
# Install built library files
########################################################################
install(TARGETS hydra
    LIBRARY DESTINATION lib${LIB_SUFFIX} # .so/.dylib file
    ARCHIVE DESTINATION lib${LIB_SUFFIX} # .lib file
    RUNTIME DESTINATION bin              # .dll file
)



add_executable(hydra_test_resource hydra_resource.cc)
set_target_properties(hydra_test_resource PROPERTIES COMPILE_FLAGS "-DTEST_RESOURCE")
