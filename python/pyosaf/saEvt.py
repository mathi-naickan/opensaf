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
from saAis import *

evtdll = CDLL('libSaEvt.so.1')

SaEvtHandleT = SaUint64T
SaEvtEventHandleT = SaUint64T
SaEvtChannelHandleT = SaUint64T
SaEvtSubscriptionIdT = SaUint32T

SaEvtChannelOpenCallbackT = CFUNCTYPE(None,
		 SaInvocationT, SaEvtChannelHandleT, SaAisErrorT)

SaEvtEventDeliverCallbackT = CFUNCTYPE(None,
		SaEvtSubscriptionIdT, SaEvtEventHandleT, SaSizeT)

class SaEvtCallbacksT(Structure):
	"""Contain various callbacks Event Service may invoke on a registrant.
	"""
	_fields_ = [('saEvtChannelOpenCallback', SaEvtChannelOpenCallbackT),
		('saEvtEventDeliverCallback', SaEvtEventDeliverCallbackT)]

saEvt = Const()

saEvt.SA_EVT_CHANNEL_PUBLISHER = 0x1
saEvt.SA_EVT_CHANNEL_SUBSCRIBER = 0x2
saEvt.SA_EVT_CHANNEL_CREATE = 0x4
SaEvtChannelOpenFlagsT = SaUint8T

class SaEvtEventPatternT(Structure):
	"""Contain information identifying event pattern.
	"""
	_fields_ = [('allocatedSize', SaSizeT),
		('patternSize', SaSizeT),
		('pattern', POINTER(SaInt8T))]
	def __init__(self, pattern):
		super(SaEvtEventPatternT, self).__init__(len(pattern),
				len(pattern), create_string_buffer(pattern))

def createPatternVector(strlist=[]):
	"""Create an array of SaEvtEventPatternT from a string list.
	"""
	if not strlist:
		return None
	plist = [SaEvtEventPatternT(i) for i in strlist]
	return (SaEvtEventPatternT*len(plist))(*plist)

class SaEvtEventPatternArrayT(Structure):
	"""Contain an array of SaEvtEventPatternT.
	"""
	_fields_ = [('allocatedNumber', SaSizeT),
		('patternsNumber', SaSizeT),
		('patterns', POINTER(SaEvtEventPatternT))]
	def __init__(self, strlist=[]):
		if not strlist:
			strlist = []
		super(SaEvtEventPatternArrayT, self).__init__(len(strlist),
				len(strlist), createPatternVector(strlist))

saEvt.SA_EVT_HIGHEST_PRIORITY = 0
saEvt.SA_EVT_LOWEST_PRIORITY = 3
SaEvtEventPriorityT = SaUint8T

SaEvtEventIdT = SaUint64T
saEvt.SA_EVT_EVENTID_NONE = 0
saEvt.SA_EVT_EVENTID_LOST = 1

saEvt.SA_EVT_LOST_EVENT = "SA_EVT_LOST_EVENT_PATTERN"

SaEvtEventFilterTypeT = SaEnumT
eSaEvtEventFilterTypeT = Enumeration((
	('SA_EVT_PREFIX_FILTER', 1),
	('SA_EVT_SUFFIX_FILTER', 2),
	('SA_EVT_EXACT_FILTER', 3),
	('SA_EVT_PASS_ALL_FILTER', 4),
))

class SaEvtEventFilterT(Structure):
	"""Contain filter type and pattern to be applied on events when
	filtering an event channel.
	"""
	_fields_ = [('filterType', SaEvtEventFilterTypeT),
		('filter', SaEvtEventPatternT)]
	def __init__(self, ftype, pattern):
		super(SaEvtEventFilterT, self).__init__(ftype,
				SaEvtEventPatternT(pattern))

def createFilterVector(tuplelist):
	"""Create an array of SaEvtEventFilterT from a list of tuples containing
	type and pattern.
	"""
	flist = [SaEvtEventFilterT(ftype, pattern)
			for (ftype, pattern) in tuplelist]
	return (SaEvtEventFilterT*len(flist))(*flist) if flist else None

class SaEvtEventFilterArrayT(Structure):
	"""Contain an array of SaEvtEventFilterT.
	"""
	_fields_ = [('filtersNumber', SaSizeT),
		('filters', POINTER(SaEvtEventFilterT))]
	def __init__(self, tuplelist):
		super(SaEvtEventFilterArrayT, self).__init__(
				len(tuplelist), createFilterVector(tuplelist))

SaEvtLimitIdT = SaEnumT
eSaEvtLimitIdT = Enumeration((
	('SA_EVT_MAX_NUM_CHANNELS_ID', 1),
	('SA_EVT_MAX_EVT_SIZE_ID', 2),
	('SA_EVT_MAX_PATTERN_SIZE_ID', 3),
	('SA_EVT_MAX_NUM_PATTERNS_ID', 4),
	('SA_EVT_MAX_RETENTION_DURATION_ID', 5),
))

def saEvtInitialize(evtHandle, callbacks, version):
	"""Register invoking process with Event Service.

	type arguments:
		SaEvtHandleT evtHandle
		SaEvtCallbacksT callbacks
		SaVersionT version

	returns:
		SaAisErrorT

	"""
	return evtdll.saEvtInitialize(BYREF(evtHandle),
			BYREF(callbacks),
			BYREF(version))

def saEvtSelectionObjectGet(evtHandle, selectionObject):
	"""Return operating system handle associated with EVT handle to detect
	pending callbacks.

	type arguments:
		SaEvtHandleT evtHandle
		SaSelectionObjectT selectionObject

	returns:
		SaAisErrorT

	"""
	return evtdll.saEvtSelectionObjectGet(evtHandle,
			BYREF(selectionObject))

def saEvtDispatch(evtHandle, dispatchFlags):
	"""Invoke callbacks pending for the EVT handle.

	type arguments:
		SaEvtHandleT evtHandle
		SaDispatchFlagsT dispatchFlags

	returns:
		SaAisErrorT

	"""
	return evtdll.saEvtDispatch(evtHandle, dispatchFlags)

def saEvtFinalize(evtHandle):
	"""Close association between Event Service and the handle.

	type arguments:
		SaEvtHandleT evtHandle

	returns:
		SaAisErrorT

	"""
	return evtdll.saEvtFinalize(evtHandle)

def saEvtChannelOpen(evtHandle,
		channelName, channelOpenFlags, timeout, channelHandle):
	"""Create and open an event channel or open an existing channel.

	type arguments:
		SaEvtHandleT evtHandle
		SaNameT channelName
		SaEvtChannelOpenFlagsT channelOpenFlags
		SaTimeT timeout
		SaEvtChannelHandleT channelHandle

	returns:
		SaAisErrorT

	"""
	return evtdll.saEvtChannelOpen(evtHandle,
			BYREF(channelName),
			channelOpenFlags,
			timeout,
			BYREF(channelHandle))

def saEvtChannelOpenAsync(evtHandle,
		invocation, channelName, channelOpenFlags):
	"""Create and open an event channel or open an existing channel
	asynchronously.

	type arguments:
		SaEvtHandleT evtHandle
		SaInvocationT invocation
		SaNameT channelName
		SaEvtChannelOpenFlagsT channelOpenFlags

	returns:
		SaAisErrorT

	"""
	return evtdll.saEvtChannelOpenAsync(evtHandle,
			invocation,
			BYREF(channelName),
			channelOpenFlags)

def saEvtChannelClose(channelHandle):
	"""Close event channel associated with handle.

	type arguments:
		SaEvtChannelHandleT channelHandle

	returns:
		SaAisErrorT

	"""
	return evtdll.saEvtChannelClose(channelHandle)

def saEvtChannelUnlink(evtHandle, channelName):
	"""Delete the named channel from the cluster.

	type arguments:
		SaEvtHandleT evtHandle
		SaNameT channelName

	returns:
		SaAisErrorT

	"""
	return evtdll.saEvtChannelUnlink(evtHandle, BYREF(channelName))

def saEvtEventAllocate(channelHandle, eventHandle):
	"""Allocate memory for event header and attributes.

	type arguments:
		SaEvtChannelHandleT channelHandle
		SaEvtEventHandleT eventHandle

	returns:
		SaAisErrorT

	"""
	return evtdll.saEvtEventAllocate(channelHandle, BYREF(eventHandle))

def saEvtEventFree(eventHandle):
	"""Free events allocated by saEvtEventAllocate() or obtained from
	saEvtEventDeliverCallback().

	type arguments:
		SaEvtEventHandleT eventHandle

	returns:
		SaAisErrorT

	"""
	return evtdll.saEvtEventFree(eventHandle)

def saEvtEventAttributesSet(eventHandle,
		patternArray, priority, retentionTime, publisherName):
	"""Set all writeable event attributes in the header of the event
	associated with the handle.

	type arguments:
		SaEvtEventHandleT eventHandle
		SaEvtEventPatternArrayT patternArray
		SaEvtEventPriorityT priority
		SaTimeT retentionTime
		SaNameT publisherName

	returns:
		SaAisErrorT

	"""
	return evtdll.saEvtEventAttributesSet(eventHandle,
			BYREF(patternArray),
			priority,
			retentionTime,
			BYREF(publisherName))

def saEvtEventAttributesGet(eventHandle,
		patternArray, priority, retentionTime, publisherName,
		publishTime, eventId):
	"""Get the attribute values of the event associated with the handle.

	type arguments:
		SaEvtEventHandleT eventHandle
		SaEvtEventPatternArrayT patternArray
		SaEvtEventPriorityT priority
		SaTimeT retentionTime
		SaNameT publisherName
		SaTimeT publishTime
		SaEvtEventIdT eventId

	returns:
		SaAisErrorT

	"""
	return evtdll.saEvtEventAttributesGet(eventHandle,
			BYREF(patternArray),
			BYREF(priority),
			BYREF(retentionTime),
			BYREF(publisherName),
			BYREF(publishTime),
			BYREF(eventId))

def saEvtEventPatternFree(eventHandle, patterns):
	"""Free memory allocated by saEvtEventAttributesGet().

	type arguments:
		SaEvtEventHandleT eventHandle
		SaEvtEventPatternT patterns

	returns:
		SaAisErrorT

	"""
	return evtdll.saEvtEventPatternFree(eventHandle, patterns)

def saEvtEventDataGet(eventHandle, eventData, eventDataSize):
	"""Get data associated with an event delivered via
	saEvtEventDeliverCallback().

	type arguments:
		SaEvtEventHandleT eventHandle
		SaVoidPtr eventData
		SaSizeT eventDataSize

	returns:
		SaAisErrorT

	"""
	return evtdll.saEvtEventDataGet(eventHandle,
			eventData,
			BYREF(eventDataSize))

def saEvtEventPublish(eventHandle, eventData, eventDataSize, eventId):
	"""Publish an event on the event channel.

	type arguments:
		SaEvtEventHandleT eventHandle
		SaVoidPtr eventData
		SaSizeT eventDataSize
		SaEvtEventIdT eventId

	returns:
		SaAisErrorT

	"""
	return evtdll.saEvtEventPublish(eventHandle,
			eventData,
			eventDataSize,
			BYREF(eventId))

def saEvtEventSubscribe(channelHandle, filters, subscriptionId):
	"""Subscribe for events on event channel by registering one or more
	filters on the channel.

	type arguments:
		SaEvtChannelHandleT channelHandle
		SaEvtEventFilterArrayT filters
		SaEvtSubscriptionIdT subscriptionId

	returns:
		SaAisErrorT

	"""
	return evtdll.saEvtEventSubscribe(channelHandle,
			BYREF(filters),
			subscriptionId)

def saEvtEventUnsubscribe(channelHandle, subscriptionId):
	"""Remove subscription from event channel to stop receiving events for
	that subscription.

	type arguments:
		SaEvtChannelHandleT channelHandle
		SaEvtSubscriptionIdT subscriptionId

	returns:
		SaAisErrorT

	"""
	return evtdll.saEvtEventUnsubscribe(channelHandle, subscriptionId)

def saEvtEventRetentionTimeClear(channelHandle, eventId):
	"""Clear the retention time of the identified event.

	type arguments:
		SaEvtChannelHandleT channelHandle
		SaEvtEventIdT eventId

	returns:
		SaAisErrorT

	"""
	return evtdll.saEvtEventRetentionTimeClear(channelHandle, eventId)

def saEvtLimitGet(evtHandle, limitId, limitValue):
	"""Get current implementation-specific value of an Event Service limit.

	type arguments:
		SaEvtHandleT evtHandle
		SaEvtLimitIdT limitId
		SaLimitValueT limitValue

	returns:
		SaAisErrorT

	"""
	return evtdll.saEvtLimitGet(evtHandle,
			limitId,
			BYREF(limitValue))
