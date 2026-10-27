# Set minimum CMAKE version
cmake_minimum_required(VERSION 2.8.9)
# Define project name
project (HyDRA)

# set(CMAKE_BUILD_TYPE Debug)

# Use C++17
set(CMAKE_CXX_FLAGS "-std=c++17")
# set(CMAKE_CXX_STANDARD 17)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

# Find Threads
find_package(Threads)

# Find openCV
find_package(OpenCV)

# Find UHD
find_package(UHD REQUIRED)

# FFTW
find_package(FFTW REQUIRED)

# Find pkg-config
find_package(PkgConfig)
# Use pkg-config to get hints for zmq locations
pkg_check_modules(PC_ZeroMQ QUIET zmq)

# Use the hint from above to find where 'zmq.hpp' is located
find_path(
  ZeroMQ_INCLUDE_DIR
  NAMES zmq.hpp
  PATHS ${PC_ZeroMQ_INCLUDE_DIRS}
)

## use the hint from about to find the location of libzmq
find_library(
  ZeroMQ_LIBRARY
  NAMES zmq
  PATHS ${PC_ZeroMQ_LIBRARY_DIRS}
)

# Find Boost
find_package(Boost "1.58" COMPONENTS
  system
  filesystem
  program_options
  log
  log_setup REQUIRED)

ADD_DEFINITIONS(-DBOOST_LOG_DYN_LINK)

# Include the project's directories
include_directories(include)


subdirs(app lib include/hydra)


########################################################################
# Create uninstall target
########################################################################
configure_file(
    ${CMAKE_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake
@ONLY)

add_custom_target(uninstall
    ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake
)



########################LIBRARIES################################################
# Install cmake search helper for this library
########################################################################
if(NOT CMAKE_MODULES_DIR)
  set(CMAKE_MODULES_DIR lib${LIB_SUFFIX}/cmake)
endif(NOT CMAKE_MODULES_DIR)

install(FILES cmake/Modules/hydraConfig.cmake
    DESTINATION ${CMAKE_MODULES_DIR}/hydra
)
