INCLUDE(FindPkgConfig)
PKG_CHECK_MODULES(PC_SVL svl)

FIND_PATH(
    SVL_INCLUDE_DIRS
    NAMES svl/api.h
    HINTS $ENV{SVL_DIR}/include
        ${PC_SVL_INCLUDEDIR}
    PATHS ${CMAKE_INSTALL_PREFIX}/include
          /usr/local/include
          /usr/include
)

FIND_LIBRARY(
    SVL_LIBRARIES
    NAMES gnuradio-svl
    HINTS $ENV{SVL_DIR}/lib
        ${PC_SVL_LIBDIR}
    PATHS ${CMAKE_INSTALL_PREFIX}/lib
          ${CMAKE_INSTALL_PREFIX}/lib64
          /usr/local/lib
          /usr/local/lib64
          /usr/lib
          /usr/lib64
)

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(SVL DEFAULT_MSG SVL_LIBRARIES SVL_INCLUDE_DIRS)
MARK_AS_ADVANCED(SVL_LIBRARIES SVL_INCLUDE_DIRS)

