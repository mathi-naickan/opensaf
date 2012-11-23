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

from saAis import *
import saNtf

# Only mirrors API calls implemented in osaf/libs/agents/saf/lga/lga_api.c
# If it ain't implemented here, it probably ain't implemented there!

logdll = CDLL('libSaLog.so.1')

SaLogHandleT = SaUint64T
SaLogStreamHandleT = SaUint64T

saLog = Const()

saLog.SA_LOG_STREAM_SYSTEM =\
		"safLgStrCfg=saLogSystem,safApp=safLogService"
saLog.SA_LOG_STREAM_NOTIFICATION =\
		"safLgStrCfg=saLogNotification,safApp=safLogService"
saLog.SA_LOG_STREAM_ALARM =\
		"safLgStrCfg=saLogAlarm,safApp=safLogService"

saLog.SA_LOG_SEV_EMERGENCY = 0
saLog.SA_LOG_SEV_ALERT = 1
saLog.SA_LOG_SEV_CRITICAL = 2
saLog.SA_LOG_SEV_ERROR = 3
saLog.SA_LOG_SEV_WARNING = 4
saLog.SA_LOG_SEV_NOTICE = 5
saLog.SA_LOG_SEV_INFO = 6

SaLogSeverityT = SaUint16T

saLog.SA_LOG_SEV_FLAG_EMERGENCY = 0x0001
saLog.SA_LOG_SEV_FLAG_ALERT = 0x0002
saLog.SA_LOG_SEV_FLAG_CRITICAL = 0x0004
saLog.SA_LOG_SEV_FLAG_ERROR = 0x0008
saLog.SA_LOG_SEV_FLAG_WARNING = 0x0010
saLog.SA_LOG_SEV_FLAG_NOTICE = 0x0020
saLog.SA_LOG_SEV_FLAG_INFO = 0x0040

SaLogSeverityFlagsT = SaUint16T

class SaLogBufferT(Structure):
	"""Contain body of the log record.
	"""
	_fields_ = [('logBufSize', SaSizeT),
		('logBuf', POINTER(SaInt8T))]
	def __init__(self, buf=''):
		super(SaLogBufferT, self).__init__(len(buf),
			create_string_buffer(buf) if buf else None)

SaLogAckFlagsT = SaUint32T

saLog.SA_LOG_RECORD_WRITE_ACK = 0x1

SaLogStreamOpenFlagsT = SaUint8T

saLog.SA_LOG_STREAM_CREATE = 0x1

SaLogNtfIdentifiersT = SaEnumT
eSaLogNtfIdentifiersT = Enumeration((
	('SA_LOG_NTF_LOGFILE_PERCENT_FULL', 1),
))

SaLogNtfAttributesT = SaEnumT
eSaLogNtfAttributesT = Enumeration((
	('SA_LOG_NTF_ATTR_LOG_STREAM_NAME', 1),
	('SA_LOG_NTF_ATTR_LOGFILE_NAME', 2),
	('SA_LOG_NTF_ATTR_LOGFILE_PATH_NAME', 3),
))

SaLogHeaderTypeT = SaEnumT
eSaLogHeaderTypeT = Enumeration((
	('SA_LOG_NTF_HEADER', 1),
	('SA_LOG_GENERIC_HEADER', 2),
))

class SaLogNtfLogHeaderT(Structure):
	"""Contain fields specific to a notification or alarm log record header.
	"""
	_fields_ = [('notificationId', saNtf.SaNtfIdentifierT),
		('eventType', saNtf.SaNtfEventTypeT),
		('notificationObject', POINTER(SaNameT)),
		('notifyingObject', POINTER(SaNameT)),
		('notificationClassId', POINTER(saNtf.SaNtfClassIdT)),
		('eventTime', SaTimeT)]

class SaLogGenericLogHeaderT(Structure):
	"""Contain fields specific to a log record header whose destination is
	either the system or an application specific log stream.
	"""
	_fields_ = [('notificationClassId', POINTER(saNtf.SaNtfClassIdT)),
		('logSvcUsrName', POINTER(SaNameT)),
		('logSeverity', SaLogSeverityT)]

class SaLogHeaderT(Union):
	"""Contain log record header information that is specific to the log
	stream for which the log record is destined.
	"""
        _fields_ = [('ntfHdr', SaLogNtfLogHeaderT),
                 ('genericHdr', SaLogGenericLogHeaderT)]

class SaLogRecordT(Structure):
	"""Contain all information associated with a log record.
	"""
        _fields_ = [('logTimeStamp', SaTimeT),
                ('logHdrType', SaLogHeaderTypeT),
                ('logHeader', SaLogHeaderT),
                ('logBuffer', POINTER(SaLogBufferT))]

SaLogFileFullActionT = SaEnumT
eSaLogFileFullActionT = Enumeration((
	('SA_LOG_FILE_FULL_ACTION_WRAP', 1),
	('SA_LOG_FILE_FULL_ACTION_HALT', 2),
	('SA_LOG_FILE_FULL_ACTION_ROTATE', 3),
))

class SaLogFileCreateAttributesT_2(Structure):
	"""Contain log file creation information required when creating a new
	application log stream.
	"""
	_fields_ = [('logFileName', SaStringT),
		('logFilePathName', SaStringT),
		('maxLogFileSize', SaUint64T),
		('maxLogRecordSize', SaUint32T),
		('haProperty', SaBoolT),
		('logFileFullAction', SaLogFileFullActionT),
		('maxFilesRotated', SaUint16T),
		('logFileFmt', SaStringT)]

SaLogAdminOperationIdT = SaEnumT
eSaLogAdminOperationIdT = Enumeration((
	('SA_LOG_ADMIN_CHANGE_FILTER', 1),
))

SaLogFilterSetCallbackT = CFUNCTYPE(None,
		SaLogStreamHandleT, SaLogSeverityFlagsT)

SaLogStreamOpenCallbackT = CFUNCTYPE(None,
		SaInvocationT, SaLogStreamHandleT, SaAisErrorT)

SaLogWriteLogCallbackT = CFUNCTYPE(None, SaInvocationT, SaAisErrorT)

class SaLogCallbacksT(Structure):
	"""Contain various callbacks Log Service may invoke on registrant.
	"""
	_fields_ = [('saLogFilterSetCallback', SaLogFilterSetCallbackT),
		('saLogStreamOpenCallback', SaLogStreamOpenCallbackT),
		('saLogWriteLogCallback', SaLogWriteLogCallbackT)]

def saLogInitialize(logHandle, callbacks, version):
	"""Register invoking process with Log Service.

	type arguments:
		SaLogHandleT logHandle
		SaLogCallbacksT callbacks
		SaVersionT version

	returns:
		SaAisErrorT

	"""
	
	logdll.saLogInitialize.argtypes = [POINTER(SaLogHandleT),
                                       POINTER(SaLogCallbacksT),
                                       POINTER(SaVersionT)]

	logdll.saLogInitialize.restype = SaAisErrorT

	return logdll.saLogInitialize(BYREF(logHandle),
			BYREF(callbacks),
			BYREF(version))

def saLogSelectionObjectGet(logHandle, selectionObject):
	"""Return operating system handle associated with LOG handle to detect
	pending callbacks.

	type arguments:
		SaLogHandleT logHandle
		SaSelectionObjectT selectionObject

	returns:
		SaAisErrorT

	"""
	
	logdll.saLogSelectionObjectGet.argtypes = [SaLogHandleT,
                                               POINTER(SaSelectionObjectT)]

	logdll.saLogSelectionObjectGet.restype = SaAisErrorT

	return logdll.saLogSelectionObjectGet(logHandle,
			BYREF(selectionObject))

def saLogDispatch(logHandle, dispatchFlags):
	"""Invoke callbacks pending for the LOG handle.

	type arguments:
		SaLogHandleT logHandle
		SaDispatchFlagsT dispatchFlags

	returns:
		SaAisErrorT

	"""
	
	logdll.saLogDispatch.argtypes = [SaLogHandleT,
                                     SaDispatchFlagsT]

	logdll.saLogDispatch.restype = SaAisErrorT

	return logdll.saLogDispatch(logHandle, dispatchFlags)

def saLogFinalize(logHandle):
	"""Close association between Log Service and the handle.

	type arguments:
		SaLogHandleT logHandle

	returns:
		SaAisErrorT

	"""
	
	logdll.saLogFinalize.argtypes = [SaLogHandleT]

	logdll.saLogFinalize.restype = SaAisErrorT

	return logdll.saLogFinalize(logHandle)

def saLogStreamOpen_2(logHandle, logStreamName, logFileCreateAttributes,
		logStreamOpenFlags, timeout, logStreamHandle):
	"""Open a log stream.

	type arguments:
		SaLogHandleT logHandle
		SaNameT logStreamName
		SaLogFileCreateAttributesT_2 logFileCreateAttributes
		SaLogStreamOpenFlagsT logStreamOpenFlags
		SaTimeT timeout
		SaLogStreamHandleT logStreamHandle

	returns:
		SaAisErrorT

	"""
	
	logdll.saLogStreamOpen_2.argtypes = [SaLogHandleT,
                                         POINTER(SaNameT),
                                         POINTER(SaLogFileCreateAttributesT_2),
                                         SaLogStreamOpenFlagsT,
                                         SaTimeT,
                                         POINTER(SaLogStreamHandleT)]

	logdll.saLogStreamOpen_2.restype = SaAisErrorT

	return logdll.saLogStreamOpen_2(logHandle,
			BYREF(logStreamName),
			BYREF(logFileCreateAttributes),
			logStreamOpenFlags,
			timeout,
			BYREF(logStreamHandle))

def saLogStreamOpenAsync_2(logHandle, logStreamName,
		logFileCreateAttributes, logStreamOpenFlags, invocation):
	"""Open a log stream asynchronously.

	type arguments:
		SaLogHandleT logHandle
		SaNameT logStreamName
		SaLogFileCreateAttributesT_2 logFileCreateAttributes
		SaLogStreamOpenFlagsT logStreamOpenFlags
		SaInvocationT invocation

	returns:
		SaAisErrorT

	"""
	
	logdll.saLogStreamOpenAsync_2.argtypes = [SaLogHandleT,
                                              POINTER(SaNameT),
                                              POINTER(SaLogFileCreateAttributesT_2),
                                              SaLogStreamOpenFlagsT,
                                              SaInvocationT]

	logdll.saLogStreamOpenAsync_2.restype = SaAisErrorT

	return logdll.saLogStreamOpenAsync_2(logHandle,
			BYREF(logStreamName),
			BYREF(logFileCreateAttributes),
			logStreamOpenFlags,
			invocation)

def saLogWriteLog(logStreamHandle, timeout, logRecord):
	"""Log a record to the log stream.

	type arguments:
		SaLogStreamHandleT logStreamHandle
		SaTimeT timeout
		SaLogRecordT logRecord

	returns:
		SaAisErrorT

	"""
	
	logdll.saLogWriteLog.argtypes = [SaLogHandleT,
                                     SaTimeT,
                                     POINTER(SaLogRecordT)]

	logdll.saLogWriteLog.restype = SaAisErrorT

	return logdll.saLogWriteLog(logStreamHandle, timeout, BYREF(logRecord))

def saLogWriteLogAsync(logStreamHandle, invocation, ackFlags, logRecord):
	"""Log a record to the log stream asynchronously.

	type arguments:
		SaLogStreamHandleT logStreamHandle
		SaInvocationT invocation
		SaLogAckFlagsT ackFlags
		SaLogRecordT logRecord

	returns:
		SaAisErrorT

	"""
	
	logdll.saLogWriteLogAsync.argtypes = [SaLogStreamHandleT,
                                       SaInvocationT,
                                       SaLogAckFlagsT,
                                       POINTER(SaLogRecordT)]

	logdll.saLogWriteLogAsync.restype = SaAisErrorT

	return logdll.saLogWriteLogAsync(logStreamHandle,
			invocation,
			ackFlags,
			BYREF(logRecord))

def saLogStreamClose(logStreamHandle):
	"""Close the log stream.

	type arguments:
		SaLogStreamHandleT logStreamHandle

	returns:
		SaAisErrorT

	"""
	
	logdll.saLogStreamClose.argtypes = [SaLogStreamHandleT]

	logdll.saLogStreamClose.restype = SaAisErrorT

	return logdll.saLogStreamClose(logStreamHandle)

def saLogLimitGet(logHandle, limitId, limitValue):
	"""Get the current implementation-specific value of a Log Service limit.

	type arguments:
		SaLogHandleT logHandle
		SaLogLimitIdT limitId
		SaLimitValueT limitValue

	returns:
		SaAisErrorT

	"""
	
	logdll.saLogLimitGet.argtypes = [SaLogStreamHandleT,
                                     SaLogLimitIdT,
                                     POINTER(SaLimitValueT)]

	logdll.saLogLimitGet.restype = SaAisErrorT

	return logdll.saLogLimitGet(logHandle, limitId, BYREF(limitValue))
