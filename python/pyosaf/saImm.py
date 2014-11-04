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
Common IMM OM and OI type definitions
'''

from ctypes import cast, pointer, POINTER, Structure, Union

from pyosaf.saAis import SaStringT, SaEnumT, SaInt32T, SaUint32T, SaInt64T, \
    SaUint64T, SaTimeT, SaNameT, SaFloatT, SaDoubleT, SaStringT, SaAnyT
from pyosaf.saEnumConst import Enumeration, Const
from pyosaf.saAis import SaVoidPtr

saImm = Const()

SaImmClassNameT = SaStringT
SaImmAttrNameT = SaStringT
SaImmAdminOwnerNameT = SaStringT

saImm = Const()

SaImmClassNameT = SaStringT
SaImmAttrNameT = SaStringT
SaImmAdminOwnerNameT = SaStringT

SaImmValueTypeT = SaEnumT
eSaImmValueTypeT = Enumeration((
	('SA_IMM_ATTR_SAINT32T', 1),
	('SA_IMM_ATTR_SAUINT32T', 2),
	('SA_IMM_ATTR_SAINT64T', 3),
	('SA_IMM_ATTR_SAUINT64T', 4),
	('SA_IMM_ATTR_SATIMET', 5),
	('SA_IMM_ATTR_SANAMET', 6),
	('SA_IMM_ATTR_SAFLOATT', 7),
	('SA_IMM_ATTR_SADOUBLET', 8),
	('SA_IMM_ATTR_SASTRINGT', 9),
	('SA_IMM_ATTR_SAANYT', 10),
))

SaImmValueTypeMap = {
	eSaImmValueTypeT.SA_IMM_ATTR_SAINT32T: SaInt32T,
	eSaImmValueTypeT.SA_IMM_ATTR_SAUINT32T: SaUint32T,
	eSaImmValueTypeT.SA_IMM_ATTR_SAINT64T: SaInt64T,
	eSaImmValueTypeT.SA_IMM_ATTR_SAUINT64T: SaUint64T,
	eSaImmValueTypeT.SA_IMM_ATTR_SATIMET: SaTimeT,
	eSaImmValueTypeT.SA_IMM_ATTR_SANAMET: SaNameT,
	eSaImmValueTypeT.SA_IMM_ATTR_SAFLOATT: SaFloatT,
	eSaImmValueTypeT.SA_IMM_ATTR_SADOUBLET: SaDoubleT,
	eSaImmValueTypeT.SA_IMM_ATTR_SASTRINGT: SaStringT,
	eSaImmValueTypeT.SA_IMM_ATTR_SAANYT: SaAnyT
}

def unmarshalSaImmValue(void_ptr, value_type):
	"""Convert void pointer to an instance of value type.
	"""
	val_ptr = SaImmValueTypeMap.get(value_type)
	if val_ptr and void_ptr:
		if val_ptr == SaNameT:
			return cast(void_ptr, POINTER(val_ptr))[0].value
		return cast(void_ptr, POINTER(val_ptr))[0]
	return None

SaImmClassCategoryT = SaEnumT
eSaImmClassCategoryT = Enumeration((
	('SA_IMM_CLASS_CONFIG', 1),
	('SA_IMM_CLASS_RUNTIME', 2),
))

saImm.SA_IMM_ATTR_MULTI_VALUE = 0x00000001
saImm.SA_IMM_ATTR_RDN = 0x00000002
saImm.SA_IMM_ATTR_CONFIG = 0x00000100
saImm.SA_IMM_ATTR_WRITABLE = 0x00000200
saImm.SA_IMM_ATTR_INITIALIZED = 0x00000400
saImm.SA_IMM_ATTR_RUNTIME = 0x00010000
saImm.SA_IMM_ATTR_PERSISTENT = 0x00020000
saImm.SA_IMM_ATTR_CACHED = 0x00040000

SaImmAttrFlagsT = SaUint64T

SaImmAttrValueT = SaVoidPtr

class SaImmAttrDefinitionT_2(Structure):
	"""Contain characteristics of an attribute belonging to an object class.
	"""
	_fields_ = [('attrName', SaImmAttrNameT),
		('attrValueType', SaImmValueTypeT),
		('attrFlags', SaImmAttrFlagsT),
		('attrDefaultValue', SaImmAttrValueT)]


class SaImmAttrValuesT_2(Structure):
	"""Contain the values of one attribute of an object.
	"""
	_fields_ = [('attrName', SaImmAttrNameT),
		('attrValueType', SaImmValueTypeT),
		('attrValuesNumber', SaUint32T),
		('attrValues', POINTER(SaImmAttrValueT))]

SaImmAttrModificationTypeT = SaEnumT
eSaImmAttrModificationTypeT = Enumeration((
	('SA_IMM_ATTR_VALUES_ADD', 1),
	('SA_IMM_ATTR_VALUES_DELETE', 2),
	('SA_IMM_ATTR_VALUES_REPLACE', 3),
))

class SaImmAttrModificationT_2(Structure):
	"""Contain modification type and attribute to be modified.
	"""
	_fields_ = [('modType', SaImmAttrModificationTypeT),
		('modAttr', SaImmAttrValuesT_2)]

SaImmScopeT = SaEnumT
eSaImmScopeT = Enumeration((
	('SA_IMM_ONE', 1),
	('SA_IMM_SUBLEVEL', 2),
	('SA_IMM_SUBTREE', 3),
))

saImm.SA_IMM_SEARCH_ONE_ATTR = 0x0001
saImm.SA_IMM_SEARCH_GET_ALL_ATTR = 0x0100
saImm.SA_IMM_SEARCH_GET_NO_ATTR = 0x0200
saImm.SA_IMM_SEARCH_GET_SOME_ATTR = 0x0400

SaImmSearchOptionsT = SaUint64T

class SaImmSearchOneAttrT_2(Structure):
	"""Contain the attribute description for SA_IMM_SEARCH_ONE_ATTR search
	operations.
	"""
	_fields_ = [('attrName', SaImmAttrNameT),
		('attrValueType', SaImmValueTypeT),
		('attrValue', SaImmAttrValueT)]

class SaImmSearchParametersT_2(Union):
	""" used to provide the criteria parameters for search operations
	"""
	_fields_ = [('searchOneAttr', SaImmSearchOneAttrT_2)]

saImm.SA_IMM_CCB_REGISTERED_OI = 0x00000001

SaImmCcbFlagsT = SaUint64T

SaImmContinuationIdT = SaUint64T

SaImmAdminOperationIdT = SaUint64T

class SaImmAdminOperationParamsT_2(Structure):
	"""Contain the parameters of an administrative operation performed on
	an object.
	"""
	_fields_ = [('paramName', SaStringT),
		('paramType', SaImmValueTypeT),
		('paramBuffer', SaImmAttrValueT)]
	def __init__(self, name, ptype, val):
		pname = SaStringT(name)
		pval = cast(pointer(SaImmValueTypeMap[ptype](val)), SaVoidPtr)
		super(SaImmAdminOperationParamsT_2, self).__init__(
				pname, ptype, pval)

