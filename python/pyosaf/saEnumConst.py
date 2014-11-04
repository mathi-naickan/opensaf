############################################################################
#
# (C) Copyright 2011 The OpenSAF Foundation
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
# or FITNESS FOR A PARTICULAR PURPOSE. This file and program are licensed
# under the GNU Lesser General Public License Version 2.1, February 1999.
# The complete license can be accessed from the following location:
# http://opensource.org/licenses/lgpl-license.php
# See the Copying file included with the OpenSAF distribution for full
# licensing terms.
#
# Author(s): Wind River Systems, Inc.
#
############################################################################

'''
Common classes for pyosaf modules
'''

class EnumException(Exception):
	"""Contain information on exceptions raised using Enumeration class.
	"""
	pass


class Enumeration(object):
	"""Contain table of enumerations.

	Enable the definition and use of named symbols to represent numeric
	literals.  The same numeric literal can be represented by more than one
	named symbol if required.  The class ensures that, once named symbols are
	bound to a number, they cannot be rebound.

	"""
	def __init__(self, enumlist, are_unique=True):
		"""Construct table of enumerations from list.

		The table of enumerations is constructed from enumlist; this
		list can contain either (1)symbols, (2)tuples pairing a
		symbol with a numeric literal, or (3)combination of both.
		For a symbol not paired with a numeric literal, its literal is
		resolved via an increment on the last known literal (analogous
		to c-style enumerations).

		"""
		self.lookup = lookup = {}
		self.reverse_lookup = reverse_lookup = {}

		i = 0
		for node in enumlist:
			if type(node) is tuple:
				try:
					node, i = node
				except ValueError:
					raise EnumException('%r:' % (node,))
			if type(node) is not str:
				raise EnumException('Enum name not a string: %r' % (node,))
			if type(i) is not int:
				raise EnumException('Enum value not integer: %r' % (node,))
			if node in lookup:
				raise EnumException('Enum name not unique: %r' % (node,))
			if are_unique and i in reverse_lookup:
				raise EnumException('Enum value %r not unique: %r' % (i, node))
			lookup[node] = i
			reverse_lookup[i] = node
			i += 1

	def __getattr__(self, attr):
		"""Get numeric literal associated with symbol.

		Enables notation 'enum.symbol'; i.e. eSaAisErrorT.SA_AIS_OK

		"""
		try:
			return self.lookup[attr]
		except KeyError:
			raise AttributeError(attr)

	def whatis(self, value):
		"""Return string symbol of numeric literal.

		eSaAisErrorT.whatis(1) will return 'SA_AIS_OK'.

		"""
		return self.reverse_lookup.get(value, 'UNKNOWN_ENUM')


class Const(object):
	class ConstError(TypeError):
		pass

	def __setattr__(self, name, value):
		if name in self.__dict__:
			raise self.ConstError('Cannot rebind "%s"' % name)
		self.__dict__[name] = value

	def __delattr__(self, name):
		if name in self.__dict__:
			raise self.ConstError('Cannot unbind "%s"' % name)
		raise NameError(name)
