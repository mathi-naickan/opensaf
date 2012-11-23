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

from ctypes import *
from saEnumConst import Enumeration, Const

SaInt8T = c_char
SaInt16T = c_short
SaInt32T = c_int
SaInt64T = c_longlong
SaUint8T = c_ubyte
SaUint16T = c_ushort
SaUint32T = c_uint
SaUint64T = c_ulonglong
SaEnumT = SaInt32T
SaVoidPtr = c_void_p

# Types used by the NTF/IMMS service
SaFloatT = c_float
SaDoubleT = c_double
SaStringT = c_char_p

SaTimeT = SaInt64T
SaInvocationT = SaUint64T
SaSizeT = SaUint64T
SaOffsetT = SaUint64T
SaSelectionObjectT = SaUint64T

saAis = Const()

saAis.SA_TIME_END = 0x7FFFFFFFFFFFFFFF
saAis.SA_TIME_BEGIN = 0x0
saAis.SA_TIME_UNKNOWN = 0x8000000000000000

saAis.SA_TIME_ONE_MICROSECOND = 1000
saAis.SA_TIME_ONE_MILLISECOND = 1000000
saAis.SA_TIME_ONE_SECOND = 1000000000
saAis.SA_TIME_ONE_MINUTE = 60000000000
saAis.SA_TIME_ONE_HOUR = 3600000000000
saAis.SA_TIME_ONE_DAY = 86400000000000
saAis.SA_TIME_MAX = saAis.SA_TIME_END

saAis.SA_MAX_NAME_LENGTH = 256

saAis.SA_TRACK_CURRENT = 0x01
saAis.SA_TRACK_CHANGES = 0x02
saAis.SA_TRACK_CHANGES_ONLY = 0x04
saAis.SA_TRACK_LOCAL = 0x08
saAis.SA_TRACK_START_STEP = 0x10
saAis.SA_TRACK_VALIDATE_STEP = 0x20

SaBoolT = SaEnumT
eSaBoolT = Enumeration((
	('SA_FALSE', 0),
	('SA_TRUE', 1),
))

SaDispatchFlagsT = SaEnumT
eSaDispatchFlagsT = Enumeration((
	('SA_DISPATCH_ONE', 1),
	('SA_DISPATCH_ALL', 2),
	('SA_DISPATCH_BLOCKING', 3),
))

SaAisErrorT = SaEnumT
eSaAisErrorT = Enumeration((
	('SA_AIS_OK', 1),
	('SA_AIS_ERR_LIBRARY', 2),
	('SA_AIS_ERR_VERSION', 3),
	('SA_AIS_ERR_INIT', 4),
	('SA_AIS_ERR_TIMEOUT', 5),
	('SA_AIS_ERR_TRY_AGAIN', 6),
	('SA_AIS_ERR_INVALID_PARAM', 7),
	('SA_AIS_ERR_NO_MEMORY', 8),
	('SA_AIS_ERR_BAD_HANDLE', 9),
	('SA_AIS_ERR_BUSY', 10),
	('SA_AIS_ERR_ACCESS', 11),
	('SA_AIS_ERR_NOT_EXIST', 12),
	('SA_AIS_ERR_NAME_TOO_LONG', 13),
	('SA_AIS_ERR_EXIST', 14),
	('SA_AIS_ERR_NO_SPACE', 15),
	('SA_AIS_ERR_INTERRUPT', 16),
	('SA_AIS_ERR_NAME_NOT_FOUND', 17),
	('SA_AIS_ERR_NO_RESOURCES', 18),
	('SA_AIS_ERR_NOT_SUPPORTED', 19),
	('SA_AIS_ERR_BAD_OPERATION', 20),
	('SA_AIS_ERR_FAILED_OPERATION', 21),
	('SA_AIS_ERR_MESSAGE_ERROR', 22),
	('SA_AIS_ERR_QUEUE_FULL', 23),
	('SA_AIS_ERR_QUEUE_NOT_AVAILABLE', 24),
	('SA_AIS_ERR_BAD_FLAGS', 25),
	('SA_AIS_ERR_TOO_BIG', 26),
	('SA_AIS_ERR_NO_SECTIONS', 27),
	('SA_AIS_ERR_NO_OP', 28),
	('SA_AIS_ERR_REPAIR_PENDING', 29),
	('SA_AIS_ERR_NO_BINDINGS', 30),
	('SA_AIS_ERR_UNAVAILABLE', 31),
	('SA_AIS_ERR_CAMPAIGN_ERROR_DETECTED', 32),
	('SA_AIS_ERR_CAMPAIGN_PROC_FAILED', 33),
	('SA_AIS_ERR_CAMPAIGN_CANCELED', 34),
	('SA_AIS_ERR_CAMPAIGN_FAILED', 35),
	('SA_AIS_ERR_CAMPAIGN_SUSPENDED', 36),
	('SA_AIS_ERR_CAMPAIGN_SUSPENDING', 37),
	('SA_AIS_ERR_ACCESS_DENIED', 38),
	('SA_AIS_ERR_NOT_READY', 39),
	('SA_AIS_ERR_DEPLOYMENT', 40),
))

SaServicesT = SaEnumT
eSaServicesT = Enumeration((
	('SA_SVC_HPI', 1),
	('SA_SVC_AMF', 2),
	('SA_SVC_CLM', 3),
	('SA_SVC_CKPT', 4),
	('SA_SVC_EVT', 5),
	('SA_SVC_MSG', 6),
	('SA_SVC_LCK', 7),
	('SA_SVC_IMMS', 8),
	('SA_SCV_LOG', 9),
	('SA_SVC_NTF', 10),
	('SA_SVC_NAM', 11),
	('SA_SVC_TMR', 12),
	('SA_SVC_SMF', 13),
	('SA_SVC_SEC', 14),
	('SA_SVC_PLM', 15),
))

SaLogLimitIdT = SaEnumT
eSaLogLimitIdT = Enumeration((
	('SA_LOG_MAX_NUM_CLUSTER_APP_LOG_STREAMS_ID', 1),
))

class SaAnyT(Structure):
	"""Contain arbitrary set of octets.

	Must use ctypes.create_string_buffer() to allocate bytes for
	'bufferAddr': ensures mutability of buffer.
	"""
	_fields_ = [('bufferSize', SaSizeT),
		('bufferAddr', POINTER(SaInt8T))]

class SaNameT(Structure):
	"""Contain names.
	"""
	_fields_ = [('length', SaUint16T),
		('value', (SaInt8T*saAis.SA_MAX_NAME_LENGTH))]
	def __init__(self, name=''):
		"""Construct instance with contents of 'name'.
		"""
		super(SaNameT, self).__init__(len(name), name)

class SaVersionT(Structure):
	"""Contain software versions of area implementation.
	"""
	_fields_ = [('releaseCode', SaInt8T),
		('majorVersion', SaUint8T),
		('minorVersion', SaUint8T)]

class SaLimitValueT(Union):
	"""Contain the value of an implementation-specific limit.
	"""
	_fields_ = [('int64Value', SaInt64T),
		('uint64Value', SaUint64T),
		('timeValue', SaTimeT),
		('floatValue', SaFloatT),
		('doubleValue', SaDoubleT)]

def BYREF(data):
	""" Wrap function 'byref' to handle data == None.
	"""
	return None if data == None else byref(data)

def marshalNullArray(plist):
	"""Convert Python list to null-terminated c array.
	"""
	if not plist:
		return None if plist == None else (c_void_p*1)()
	ctype = plist[0].__class__
	if ctype is SaStringT:
		return marshalSaStringTArray(plist)
	c_array = (POINTER(ctype)*(len(plist)+1))()
	for i in range(len(plist)):
		c_array[i] = pointer(plist[i])
	return c_array

def marshalSaStringTArray(plist):
	"""Convert Python list of c-style strings to null-terminated c array.
	"""
	if not plist:
		return None if plist == None else (SaStringT*1)()
	c_array = (SaStringT*(len(plist)+1))()
	for i in range(len(plist)):
		c_array[i] = plist[i]
	return c_array

def unmarshalNullArray(c_array):
	"""Convert c array to Python list.
	"""
	if not c_array:
		return []
	ctype = c_array[0].__class__
	if ctype is str:
		return unmarshalSaStringTArray(c_array)
	val_list = []
	for ptr in c_array:
		if not ptr: break
		val_list.append(ptr[0])
	return val_list

def unmarshalSaStringTArray(c_array):
	"""Convert c array of c-style strings to Python list.
	"""
	if not c_array:
		return []
	val_list = []
	for ptr in c_array:
		if not ptr: break
		val_list.append(ptr)
	return val_list

def marshalNullArrayVoidPtrPtr(plist):
	"""Convert Python list to null-terminated c array, cast as a pointer to
	a void pointer.
	"""
	return pointer(cast(marshalNullArray(plist), SaVoidPtr))
