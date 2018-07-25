# Set minimum CMAKE version
cmake_minimum_required(VERSION 2.8.9)
# Define project name
project (HyDRA)

# Use C++17
set(CMAKE_CXX_FLAGS "-std=c++17")
# set(CMAKE_CXX_STANDARD 17)

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
find_package(Boost "1.58" COMPONENTS system filesystem REQUIRED)

# Include the project's directories
subdirs(app lib include/hydra)
