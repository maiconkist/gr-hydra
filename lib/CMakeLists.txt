## Include the headers
#include_directories(${HYDRA_SOURCE_DIR}/hydra/include)
#
## Communication client and server
#add_library(hydraserver hydra_server.cc)
#add_library(hydraclient hydra_client.cc)
#
## XVL Resource manager
#add_library(hydraresource hydra_resource.cc)
## XVL Timed buffer
#add_library(hydrabuffer hydra_buffer.cc)
## XVL UDP Socket
#add_library(hydrasocket hydra_socket.cc)
## XVL Core
#add_library(hydracore hydra_core.cc)
## XVL Main
#add_library(hydramain hydra_main.cc)
## XVL Stats
#add_library(hydrastats hydra_stats.cc)
#
#
## Link BOOST to XVL UDP Socket
#target_link_libraries(
#  hydrasocket
#  ${Boost_FILESYSTEM_LIBRARY}
#  ${Boost_SYSTEM_LIBRARY}
#  pthread
#)
#
## Link ZMQ to XVL Stats
#target_link_libraries(hydrastats PUBLIC ${ZeroMQ_LIBRARY})
#
## Link FFTW and thread to the XVL Buffer
#target_link_libraries(hydrabuffer PUBLIC fftw3 pthread)
#
## Link XVL Buffer, XVL Socket, XVL Resource and XVL Stats to the XVL Core
#target_link_libraries(
#  hydracore
#  PUBLIC
#  hydrabuffer
#  hydrasocket
#  hydraresource
#  hydrastats
#)
#
## Add the ZMQ include directory to our compile directives
#target_include_directories(hydraserver PUBLIC ${ZeroMQ_INCLUDE_DIR})
#target_include_directories(hydraclient PUBLIC ${ZeroMQ_INCLUDE_DIR})
#
## Add the ZMQ library to our link directive
#target_link_libraries(hydraserver PUBLIC ${ZeroMQ_LIBRARY} hydracore)
#target_link_libraries(hydraclient PUBLIC ${ZeroMQ_LIBRARY})
#
## Link XVL Core, XVL Server and XVL Stats into the main
#target_link_libraries(hydramain PUBLIC hydracore hydraserver hydrastats)
#
#

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
