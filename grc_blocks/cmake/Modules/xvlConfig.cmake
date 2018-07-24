INCLUDE(FindPkgConfig)
PKG_CHECK_MODULES(PC_XVL xvl)

FIND_PATH(
    XVL_INCLUDE_DIRS
    NAMES xvl/api.h
    HINTS $ENV{XVL_DIR}/include
        ${PC_XVL_INCLUDEDIR}
    PATHS ${CMAKE_INSTALL_PREFIX}/include
          /usr/local/include
          /usr/include
)

FIND_LIBRARY(
    XVL_LIBRARIES
    NAMES gnuradio-xvl
    HINTS $ENV{XVL_DIR}/lib
        ${PC_XVL_LIBDIR}
    PATHS ${CMAKE_INSTALL_PREFIX}/lib
          ${CMAKE_INSTALL_PREFIX}/lib64
          /usr/local/lib
          /usr/local/lib64
          /usr/lib
          /usr/lib64
)

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(XVL DEFAULT_MSG XVL_LIBRARIES XVL_INCLUDE_DIRS)
MARK_AS_ADVANCED(XVL_LIBRARIES XVL_INCLUDE_DIRS)

