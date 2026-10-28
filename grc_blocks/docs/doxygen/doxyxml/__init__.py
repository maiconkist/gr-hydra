#
# Copyright 2010 Free Software Foundation, Inc.
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
#
"""
Python interface to contents of doxygen xml documentation.

Example use:
See the contents of the example folder for the C++ and
doxygen-generated xml used in this example.

>>> # Parse the doxygen docs.
>>> import os
>>> this_dir = os.path.dirname(globals()['__file__'])
>>> xml_path = this_dir + "/example/xml/"
>>> di = DoxyIndex(xml_path)

Get a list of all top-level objects.

>>> print([mem.name() for mem in di.members()])
['Aadvark', 'aadvarky_enough', 'main']

Get all functions.

>>> print([mem.name() for mem in di.in_category(DoxyFunction)])
['aadvarky_enough', 'main']

Check if an object is present.

>>> di.has_member('Aadvark')
True
>>> di.has_member('Fish')
False

Get an item by name and check its properties.

>>> aad = di.get_member('Aadvark')
>>> print(aad.brief_description)
Models the mammal Aadvark.
>>> print(aad.detailed_description)
Sadly the model is incomplete and cannot capture all aspects of an aadvark yet.
<BLANKLINE>
This line is uninformative and is only to test line breaks in the comments.
>>> [mem.name() for mem in aad.members()]
['aadvarkness', 'print', 'Aadvark', 'get_aadvarkness']
>>> aad.get_member('print').brief_description
'Outputs the vital aadvark statistics.'

"""

from .doxyindex import DoxyIndex, DoxyFunction, DoxyParam, DoxyClass, DoxyFile, DoxyNamespace, DoxyGroup, DoxyFriend, DoxyOther
