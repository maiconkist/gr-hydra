# Copyright 2011 Free Software Foundation, Inc.
#
# This file is part of GNU Radio
#
# GNU Radio is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3, or (at your option)
# any later version.
#
# GNU Radio is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with GNU Radio; see the file COPYING.  If not, write to
# the Free Software Foundation, Inc., 51 Franklin Street,
# Boston, MA 02110-1301, USA.

find_package(pybind11 CONFIG REQUIRED)

pybind11_add_module(hydra_python bindings.cc)
target_link_libraries(hydra_python PRIVATE gnuradio-hydra)
set_target_properties(hydra_python PROPERTIES
  LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/hydra")
configure_file(__init__.py hydra/__init__.py COPYONLY)

install(TARGETS hydra_python LIBRARY DESTINATION "${GR_PYTHON_DIR}/hydra")
install(FILES __init__.py DESTINATION "${GR_PYTHON_DIR}/hydra")
