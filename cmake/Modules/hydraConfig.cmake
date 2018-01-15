INCLUDE(FindPkgConfig)
PKG_CHECK_MODULES(PC_HYDRA hydra)

FIND_PATH(
    HYDRA_INCLUDE_DIRS
    NAMES hydra/api.h
    HINTS $ENV{HYDRA_DIR}/include
    ${PC_HYDRA_INCLUDEDIR}
    PATHS ${CMAKE_INSTALL_PREFIX}/include
          /usr/local/include
          /usr/include
)

FIND_LIBRARY(
    HYDRA_LIBRARIES
    NAMES gnuradio-hydra
    HINTS $ENV{HYDRA_DIR}/lib
    ${PC_HYDRA_LIBDIR}
    PATHS ${CMAKE_INSTALL_PREFIX}/lib
          ${CMAKE_INSTALL_PREFIX}/lib64
          /usr/local/lib
          /usr/local/lib64
          /usr/lib
          /usr/lib64
)

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(HYDRA DEFAULT_MSG HYDRA_LIBRARIES HYDRA_INCLUDE_DIRS)
MARK_AS_ADVANCED(HYDRA_LIBRARIES HYDRA_INCLUDE_DIRS)

