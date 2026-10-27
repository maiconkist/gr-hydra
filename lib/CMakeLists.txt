add_library(hydraclient hydra_client.cc)
target_link_libraries(hydraclient PUBLIC ${ZeroMQ_LIBRARY})

list(APPEND hydra_sources
    util/udp.cc
    hydra_fft.cc
    hydra_virtual_radio.cc
    hydra_hypervisor.cc
    hydra_socket.cc
    hydra_stats.cc
    hydra_log.cc
    hydra_resource.cc
    hydra_server.cc
    hydra_client.cc
    hydra_core.cc
    hydra_main.cc
    hydra_uhd_interface.cc
)

add_library(hydra SHARED ${hydra_sources})
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


add_executable(hydra_test_resource hydra_resource.cc hydra_log.cc)
target_compile_definitions(hydra_test_resource PRIVATE TEST_RESOURCE)
target_include_directories(hydra_test_resource PRIVATE ${Boost_INCLUDE_DIRS})
target_link_libraries(hydra_test_resource ${Boost_LIBRARIES} pthread)
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  target_compile_definitions(hydra_test_resource PRIVATE _GLIBCXX_DEBUG)
endif()
add_test(NAME hydra_resource_release COMMAND hydra_test_resource)

add_executable(hydra_test_hypervisor qa_hypervisor.cc)
target_link_libraries(hydra_test_hypervisor hydra)
add_test(NAME hydra_repeated_release COMMAND hydra_test_hypervisor)
