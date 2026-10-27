cmake_minimum_required(VERSION 3.16)
project(HyDRAQA LANGUAGES CXX)
enable_testing()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../cmake/Modules")

find_package(Boost 1.58 REQUIRED COMPONENTS system filesystem log log_setup)
find_package(Threads REQUIRED)
find_package(UHD REQUIRED)
find_package(OpenCV REQUIRED)
find_package(FFTW REQUIRED)
find_path(ZeroMQ_INCLUDE_DIR NAMES zmq.hpp)
find_library(ZeroMQ_LIBRARY NAMES zmq)
find_library(HYDRA_LIBRARY NAMES hydra
  HINTS "${CMAKE_CURRENT_SOURCE_DIR}/../build/lib")

if(NOT HYDRA_LIBRARY)
  message(FATAL_ERROR
    "Build the core library first and set HYDRA_LIBRARY to its libhydra.so path.")
endif()
if(NOT ZeroMQ_INCLUDE_DIR OR NOT ZeroMQ_LIBRARY)
  message(FATAL_ERROR "ZeroMQ and its C++ headers are required for QA.")
endif()

foreach(suite IN ITEMS resource hypervisor)
  add_executable(hydra_test_${suite} qa_${suite}.cc)
  target_compile_features(hydra_test_${suite} PRIVATE cxx_std_17)
  set_target_properties(hydra_test_${suite} PROPERTIES CXX_EXTENSIONS OFF)
  target_compile_definitions(hydra_test_${suite} PRIVATE BOOST_LOG_DYN_LINK)
  target_include_directories(hydra_test_${suite} PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/../include"
    ${UHD_INCLUDE_DIRS}
    ${OpenCV_INCLUDE_DIRS}
    ${FFTW_INCLUDES}
    ${ZeroMQ_INCLUDE_DIR})
  target_link_libraries(hydra_test_${suite} PRIVATE
    "${HYDRA_LIBRARY}"
    Boost::system
    Boost::filesystem
    Boost::log
    Boost::log_setup
    Threads::Threads
    ${UHD_LIBRARIES}
    ${OpenCV_LIBRARIES}
    ${FFTW_LIBRARIES}
    ${ZeroMQ_LIBRARY})
endforeach()

add_test(NAME hydra_resource_release COMMAND hydra_test_resource)
add_test(NAME hydra_repeated_release COMMAND hydra_test_hypervisor)
