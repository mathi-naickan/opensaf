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

ntfdll = CDLL('libSaNtf.so.0')

SaNtfHandleT = SaUint64T
SaNtfNotificationHandleT = SaUint64T
SaNtfNotificationFilterHandleT = SaUint64T  
SaNtfReadHandleT = SaUint64T  

SaNtfNotificationTypeT = SaEnumT
eSaNtfNotificationTypeT = Enumeration((
	('SA_NTF_TYPE_OBJECT_CREATE_DELETE', 0x1000),
	('SA_NTF_TYPE_ATTRIBUTE_CHANGE', 0x2000),
	('SA_NTF_TYPE_STATE_CHANGE', 0x3000),
	('SA_NTF_TYPE_ALARM', 0x4000),
	('SA_NTF_TYPE_SECURITY_ALARM', 0x5000),
	('SA_NTF_TYPE_MISCELLANEOUS', 0x6000),
))

saNtf = Const()

saNtf.SA_NTF_NOTIFICATIONS_TYPE_MASK = 0xF000

SaNtfEventTypeT = SaEnumT
eSaNtfEventTypeT = Enumeration((
	('SA_NTF_OBJECT_NOTIFICATIONS_START',
	    eSaNtfNotificationTypeT.SA_NTF_TYPE_OBJECT_CREATE_DELETE),
	'SA_NTF_OBJECT_CREATION',
	'SA_NTF_OBJECT_DELETION',

	('SA_NTF_ATTRIBUTE_NOTIFICATIONS_START',
	    eSaNtfNotificationTypeT.SA_NTF_TYPE_ATTRIBUTE_CHANGE),
	'SA_NTF_ATTRIBUTE_ADDED',
	'SA_NTF_ATTRIBUTE_REMOVED',
	'SA_NTF_ATTRIBUTE_CHANGED',
	'SA_NTF_ATTRIBUTE_RESET',

	('SA_NTF_STATE_CHANGE_NOTIFICATIONS_START',
	    eSaNtfNotificationTypeT.SA_NTF_TYPE_STATE_CHANGE),
	'SA_NTF_OBJECT_STATE_CHANGE',

	('SA_NTF_ALARM_NOTIFICATIONS_START',
	    eSaNtfNotificationTypeT.SA_NTF_TYPE_ALARM),
	'SA_NTF_ALARM_COMMUNICATION',
	'SA_NTF_ALARM_QOS',
	'SA_NTF_ALARM_PROCESSING',
	'SA_NTF_ALARM_EQUIPMENT',
	'SA_NTF_ALARM_ENVIRONMENT',

	('SA_NTF_SECURITY_ALARM_NOTIFICATIONS_START', 
	     eSaNtfNotificationTypeT.SA_NTF_TYPE_SECURITY_ALARM),
	'SA_NTF_INTEGRITY_VIOLATION',
	'SA_NTF_OPERATION_VIOLATION',
	'SA_NTF_PHYSICAL_VIOLATION',
	'SA_NTF_SECURITY_SERVICE_VIOLATION',
	'SA_NTF_TIME_VIOLATION', 

	('SA_NTF_MISCELLANEOUS_NOTIFICATIONS_START',
	    eSaNtfNotificationTypeT.SA_NTF_TYPE_MISCELLANEOUS),
	'SA_NTF_APPLICATION_EVENT',
	'SA_NTF_ADMIN_OPERATION_START',
	'SA_NTF_ADMIN_OPERATION_END',
	'SA_NTF_CONFIG_UPDATE_START',
	'SA_NTF_CONFIG_UPDATE_END',
	'SA_NTF_ERROR_REPORT',
	'SA_NTF_ERROR_CLEAR',
	'SA_NTF_HPI_EVENT_RESOURCE',
	'SA_NTF_HPI_EVENT_SENSOR',
	'SA_NTF_HPI_EVENT_WATCHDOG',
	'SA_NTF_HPI_EVENT_DIMI',
	'SA_NTF_HPI_EVENT_FUMI',
	'SA_NTF_HPI_EVENT_OTHER',
))

#if defined(SA_NTF_A01) || defined(SA_NTF_A02)
SaNtfNotificationTypeBitsT = SaEnumT
eSaNtfNotificationTypeBitsT = Enumeration((
	('SA_NTF_TYPE_OBJECT_CREATE_DELETE_BIT', 0x0001),
	('SA_NTF_TYPE_ATTRIBUTE_CHANGE_BIT', 0x0002),
	('SA_NTF_TYPE_STATE_CHANGE_BIT', 0x0004),
	('SA_NTF_TYPE_ALARM_BIT', 0x0008),
	('SA_NTF_TYPE_SECURITY_ALARM_BIT', 0x0010),
))
#endif /* SA_NTF_A01 || SA_NTF_A02 */

saNtf.SA_NTF_OBJECT_CREATION_BIT = 0x01
saNtf.SA_NTF_OBJECT_DELETION_BIT = 0x02
saNtf.SA_NTF_ATTRIBUTE_ADDED_BIT = 0x04
saNtf.SA_NTF_ATTRIBUTE_REMOVED_BIT = 0x08
saNtf.SA_NTF_ATTRIBUTE_CHANGED_BIT = 0x10
saNtf.SA_NTF_ATTRIBUTE_RESET_BIT = 0x20
saNtf.SA_NTF_OBJECT_STATE_CHANGE_BIT = 0x40
saNtf.SA_NTF_ALARM_COMMUNICATION_BIT = 0x80
saNtf.SA_NTF_ALARM_QOS_BIT = 0x100
saNtf.SA_NTF_ALARM_PROCESSING_BIT = 0x200
saNtf.SA_NTF_ALARM_EQUIPMENT_BIT = 0x400
saNtf.SA_NTF_ALARM_ENVIRONMENT_BIT = 0x800
saNtf.SA_NTF_INTEGRITY_VIOLATION_BIT = 0x1000
saNtf.SA_NTF_OPERATION_VIOLATION_BIT = 0x2000
saNtf.SA_NTF_PHYSICAL_VIOLATION_BIT = 0x4000
saNtf.SA_NTF_SECURITY_SERVICE_VIOLATION_BIT = 0x8000
saNtf.SA_NTF_TIME_VIOLATION_BIT = 0x10000
saNtf.SA_NTF_ADMIN_OPERATION_START_BIT = 0x20000
saNtf.SA_NTF_ADMIN_OPERATION_END_BIT = 0x40000
saNtf.SA_NTF_CONFIG_UPDATE_START_BIT = 0x80000
saNtf.SA_NTF_CONFIG_UPDATE_END_BIT = 0x100000
saNtf.SA_NTF_ERROR_REPORT_BIT = 0x200000
saNtf.SA_NTF_ERROR_CLEAR_BIT = 0x400000
saNtf.SA_NTF_HPI_EVENT_RESOURCE_BIT = 0x800000
saNtf.SA_NTF_HPI_EVENT_SENSOR_BIT = 0x1000000
saNtf.SA_NTF_HPI_EVENT_WATCHDOG_BIT = 0x2000000
saNtf.SA_NTF_HPI_EVENT_DIMI_BIT = 0x4000000
saNtf.SA_NTF_HPI_EVENT_FUMI_BIT = 0x8000000
saNtf.SA_NTF_HPI_EVENT_OTHER_BIT = 0x10000000
saNtf.SA_NTF_APPLICATION_EVENT_BIT = 0x100000000000

SaNtfEventTypeBitmapT = SaUint64T  

class SaNtfClassIdT(Structure):
	"""Contain unique identifier for the situation that triggered
	notification.
	"""
	_fields_ = [('vendorId', SaUint32T),
		('majorId', SaUint16T),
		('minorId', SaUint16T)]

saNtf.SA_NTF_VENDOR_ID_SAF = 18568

SaNtfElementIdT = SaUint16T  

SaNtfIdentifierT = SaUint64T
saNtf.SA_NTF_IDENTIFIER_UNUSED = 0

class SaNtfCorrelationIdsT(Structure):
	"""Contain required notification identifiers to correlate related
	notifications.
	"""
	_fields_ = [('rootCorrelationId', SaNtfIdentifierT),
		('parentCorrelationId', SaNtfIdentifierT),
		('notificationId', SaNtfIdentifierT)]

SaNtfValueTypeT = SaEnumT
eSaNtfValueTypeT = Enumeration((
	'SA_NTF_VALUE_UINT8',
	'SA_NTF_VALUE_INT8',
	'SA_NTF_VALUE_UINT16',
	'SA_NTF_VALUE_INT16',
	'SA_NTF_VALUE_UINT32',
	'SA_NTF_VALUE_INT32',
	'SA_NTF_VALUE_FLOAT',
	'SA_NTF_VALUE_UINT64',
	'SA_NTF_VALUE_INT64',
	'SA_NTF_VALUE_DOUBLE',
	'SA_NTF_VALUE_LDAP_NAME',
	'SA_NTF_VALUE_STRING',
	'SA_NTF_VALUE_IPADDRESS',
	'SA_NTF_VALUE_BINARY',
	'SA_NTF_VALUE_ARRAY',
))

class _ptrVal(Structure):
	_fields_ = [('dataOffset', SaUint16T),
		('dataSize', SaUint16T)]

class _arrayVal(Structure):
	_fields_ = [('arrayOffset', SaUint16T),
		('numElements', SaUint16T),
		('elementSize', SaUint16T)]

class SaNtfValueT(Union):
	"""Contain variable length fields used in notifications for parameters
	or parameter elements of varying data types.
	"""
	_fields_ = [('uint8Val', SaUint8T),
		('int8Val', SaInt8T),
		('uint16Val', SaUint16T),
		('int16Val', SaInt16T),
		('uint32Val', SaUint32T),
		('int32Val', SaInt32T),
		('floatVal', SaFloatT),
		('uint64Val', SaUint64T),
		('int64Val', SaInt64T),
		('doubleVal', SaDoubleT),
		('ptrVal', _ptrVal),
		('arrayVal', _arrayVal)]

class SaNtfAdditionalInfoT(Structure):
	"""Contain a single element in the additional information parameter
	of a notification.
	"""
	_fields_ = [('infoId', SaNtfElementIdT),
		('infoType', SaNtfValueTypeT),
		('infoValue', SaNtfValueT)]

class SaNtfNotificationHeaderT(Structure):
	"""Contain pointers to common fields in internal notification structure.
	"""
	_fields_ = [('eventType', POINTER(SaNtfEventTypeT)),
		('notificationObject', POINTER(SaNameT)),
		('notifyingObject', POINTER(SaNameT)),
		('notificationClassId', POINTER(SaNtfClassIdT)),
		('eventTime', POINTER(SaTimeT)),
		('numCorrelatedNotifications', SaUint16T),
		('lengthAdditionalText', SaUint16T),
		('numAdditionalInfo', SaUint16T),
		('notificationId', POINTER(SaNtfIdentifierT)),
		('correlatedNotifications', POINTER(SaNtfIdentifierT)),
		('additionalText', SaStringT),
		('additionalInfo', POINTER(SaNtfAdditionalInfoT))]

SaNtfSourceIndicatorT = SaEnumT
eSaNtfSourceIndicatorT = Enumeration((
    ('SA_NTF_OBJECT_OPERATION', 1),
    ('SA_NTF_MANAGEMENT_OPERATION', 2),
    ('SA_NTF_UNKNOWN_OPERATION', 3),
))

class SaNtfAttributeChangeT(Structure):
	"""Contain attribute changes in a notification.
	"""
	_fields_ = [('attributeId', SaNtfElementIdT),
		('attributeType', SaNtfValueTypeT),
		('oldAttributePresent', SaBoolT),
		('oldAttributeValue', SaNtfValueT),
		('newAttributeValue', SaNtfValueT)]

class SaNtfAttributeChangeNotificationT(Structure):
	"""Contain pointers to the fields in an attribute change notification.
	"""
	_fields_ = [('notificationHandle', SaNtfNotificationHandleT),
		('notificationHeader', SaNtfNotificationHeaderT),
		('numAttributes', SaUint16T),
		('sourceIndicator', POINTER(SaNtfSourceIndicatorT)),
		('changedAttributes', POINTER(SaNtfAttributeChangeT))]

class SaNtfAttributeT(Structure):
	"""Contain object attributes in an object creation or deletion
	notification.
	"""
	_fields_ = [('attributeId', SaNtfElementIdT),
		('attributeType', SaNtfValueTypeT),
		('attributeValue', SaNtfValueT)]

class SaNtfObjectCreateDeleteNotificationT(Structure):
	"""Contain pointers to the fields in an object creation or deletion
	notification.
	"""
	_fields_ = [('notificationHandle', SaNtfNotificationHandleT),
		('notificationHeader', SaNtfNotificationHeaderT),
		('numAttributes', SaUint16T),
		('sourceIndicator', POINTER(SaNtfSourceIndicatorT)),
		('objectAttributes', POINTER(SaNtfAttributeT))]

#if defined(SA_NTF_A01) || defined(SA_NTF_A02)
class SaNtfStateChangeT(Structure):
	"""Contain the state changes as part of a notification.
	"""
	_fields_ = [('stateId', SaNtfElementIdT),
		('oldStatePresent', SaBoolT),
		('oldState', SaUint16T),
		('newState', SaUint16T)]

class SaNtfStateChangeNotificationT(Structure):
	"""Contain pointers to the fields in a state change notification.
	"""
	_fields_ = [('notificationHandle', SaNtfNotificationHandleT),
		('notificationHeader', SaNtfNotificationHeaderT),
		('numStateChanges', SaUint16T),
		('sourceIndicator', POINTER(SaNtfSourceIndicatorT)),
		('changedStates', POINTER(SaNtfStateChangeT))]
#endif /* SA_NTF_A01 || SA_NTF_A02 */

class SaNtfStateChangeT_3(Structure):
	"""Contain the state changes as part of a notification.
	"""
	_fields_ = [('stateId', SaNtfElementIdT),
		('oldStatePresent', SaBoolT),
		('oldState', SaUint64T),
		('newState', SaUint64T)]

class SaNtfStateChangeNotificationT_3(Structure):
	"""Contain pointers to the fields in a state change notification.
	"""
	_fields_ = [('notificationHandle', SaNtfNotificationHandleT),
		('notificationHeader', SaNtfNotificationHeaderT),
		('numStateChanges', SaUint16T),
		('sourceIndicator', POINTER(SaNtfSourceIndicatorT)),
		('changedStates', POINTER(SaNtfStateChangeT_3))]

SaNtfProbableCauseT = SaEnumT
eSaNtfProbableCauseT = Enumeration((
	'SA_NTF_ADAPTER_ERROR',
	'SA_NTF_APPLICATION_SUBSYSTEM_FAILURE',
	'SA_NTF_BANDWIDTH_REDUCED',
	'SA_NTF_CALL_ESTABLISHMENT_ERROR',
	'SA_NTF_COMMUNICATIONS_PROTOCOL_ERROR',
	'SA_NTF_COMMUNICATIONS_SUBSYSTEM_FAILURE',
	'SA_NTF_CONFIGURATION_OR_CUSTOMIZATION_ERROR',
	'SA_NTF_CONGESTION',
	'SA_NTF_CORRUPT_DATA',
	'SA_NTF_CPU_CYCLES_LIMIT_EXCEEDED',
	'SA_NTF_DATASET_OR_MODEM_ERROR',
	'SA_NTF_DEGRADED_SIGNAL',
	'SA_NTF_D_T_E',
	'SA_NTF_ENCLOSURE_DOOR_OPEN',
	'SA_NTF_EQUIPMENT_MALFUNCTION',
	'SA_NTF_EXCESSIVE_VIBRATION',
	'SA_NTF_FILE_ERROR',
	'SA_NTF_FIRE_DETECTED',
	'SA_NTF_FLOOD_DETECTED',
	'SA_NTF_FRAMING_ERROR',
	'SA_NTF_HEATING_OR_VENTILATION_OR_COOLING_SYSTEM_PROBLEM',
	'SA_NTF_HUMIDITY_UNACCEPTABLE',
	'SA_NTF_INPUT_OUTPUT_DEVICE_ERROR',
	'SA_NTF_INPUT_DEVICE_ERROR',
	'SA_NTF_L_A_N_ERROR',
	'SA_NTF_LEAK_DETECTED',
	'SA_NTF_LOCAL_NODE_TRANSMISSION_ERROR',
	'SA_NTF_LOSS_OF_FRAME',
	'SA_NTF_LOSS_OF_SIGNAL',
	'SA_NTF_MATERIAL_SUPPLY_EXHAUSTED',
	'SA_NTF_MULTIPLEXER_PROBLEM',
	'SA_NTF_OUT_OF_MEMORY',
	'SA_NTF_OUTPUT_DEVICE_ERROR',
	'SA_NTF_PERFORMANCE_DEGRADED',
	'SA_NTF_POWER_PROBLEM',
	'SA_NTF_PRESSURE_UNACCEPTABLE',
	'SA_NTF_PROCESSOR_PROBLEM',
	'SA_NTF_PUMP_FAILURE',
	'SA_NTF_QUEUE_SIZE_EXCEEDED',
	'SA_NTF_RECEIVE_FAILURE',
	'SA_NTF_RECEIVER_FAILURE',
	'SA_NTF_REMOTE_NODE_TRANSMISSION_ERROR',
	'SA_NTF_RESOURCE_AT_OR_NEARING_CAPACITY',
	'SA_NTF_RESPONSE_TIME_EXCESSIVE',
	'SA_NTF_RETRANSMISSION_RATE_EXCESSIVE',
	'SA_NTF_SOFTWARE_ERROR',
	'SA_NTF_SOFTWARE_PROGRAM_ABNORMALLY_TERMINATED',
	'SA_NTF_SOFTWARE_PROGRAM_ERROR',
	'SA_NTF_STORAGE_CAPACITY_PROBLEM',
	'SA_NTF_TEMPERATURE_UNACCEPTABLE',
	'SA_NTF_THRESHOLD_CROSSED',
	'SA_NTF_TIMING_PROBLEM',
	'SA_NTF_TOXIC_LEAK_DETECTED',
	'SA_NTF_TRANSMIT_FAILURE',
	'SA_NTF_TRANSMITTER_FAILURE',
	'SA_NTF_UNDERLYING_RESOURCE_UNAVAILABLE',
	'SA_NTF_VERSION_MISMATCH',
	'SA_NTF_AUTHENTICATION_FAILURE',
	'SA_NTF_BREACH_OF_CONFIDENTIALITY',
	'SA_NTF_CABLE_TAMPER',
	'SA_NTF_DELAYED_INFORMATION',
	'SA_NTF_DENIAL_OF_SERVICE',
	'SA_NTF_DUPLICATE_INFORMATION',
	'SA_NTF_INFORMATION_MISSING',
	'SA_NTF_INFORMATION_MODIFICATION_DETECTED',
	'SA_NTF_INFORMATION_OUT_OF_SEQUENCE',
	'SA_NTF_INTRUSION_DETECTION',
	'SA_NTF_KEY_EXPIRED',
	'SA_NTF_NON_REPUDIATION_FAILURE',
	'SA_NTF_OUT_OF_HOURS_ACTIVITY',
	'SA_NTF_OUT_OF_SERVICE',
	'SA_NTF_PROCEDURAL_ERROR',
	'SA_NTF_UNAUTHORIZED_ACCESS_ATTEMPT',
	'SA_NTF_UNEXPECTED_INFORMATION',
	'SA_NTF_UNSPECIFIED_REASON',
))

class SaNtfSpecificProblemT(Structure):
	"""Contain a single element in the specific problem parameter of a
	notification.
	"""
	_fields_ = [('problemId', SaNtfElementIdT),
		('problemClassId', SaNtfClassIdT),
		('problemType', SaNtfValueTypeT),
		('problemValue', SaNtfValueT)]

SaNtfSeverityT = SaEnumT
eSaNtfSeverityT = Enumeration((
	'SA_NTF_SEVERITY_CLEARED',
	'SA_NTF_SEVERITY_INDETERMINATE',
	'SA_NTF_SEVERITY_WARNING',
	'SA_NTF_SEVERITY_MINOR',
	'SA_NTF_SEVERITY_MAJOR',
	'SA_NTF_SEVERITY_CRITICAL',
))

SaNtfSeverityTrendT = SaEnumT
eSaNtfSeverityTrendT = Enumeration((
	'SA_NTF_TREND_MORE_SEVERE',
	'SA_NTF_TREND_NO_CHANGE',
	'SA_NTF_TREND_LESS_SEVERE',
))

class SaNtfThresholdInformationT(Structure):
	"""Contain information about thresholds.
	"""
	_fields_ = [('thresholdId', SaNtfElementIdT),
		('thresholdValueType', SaNtfValueTypeT),
		('thresholdValue', SaNtfValueT),
		('thresholdHysteresis', SaNtfValueT),
		('observedValue', SaNtfValueT),
		('armTime', SaTimeT)]

class SaNtfProposedRepairActionT(Structure):
	"""Contain a single proposed repair action in an alarm notification.
	"""
	_fields_ = [('actionId', SaNtfElementIdT),
		('actionValueType', SaNtfValueTypeT),
		('actionValue', SaNtfValueT)]


class SaNtfAlarmNotificationT(Structure):
	"""Contain pointers to fields in an alarm notification.
	"""
	_fields_ = [('notificationHandle', SaNtfNotificationHandleT),
		('notificationHeader', SaNtfNotificationHeaderT),
		('numSpecificProblems', SaUint16T),
		('numMonitoredAttributes', SaUint16T),
		('numProposedRepairActions', SaUint16T),
		('probableCause', POINTER(SaNtfProbableCauseT)),
		('specificProblems', POINTER(SaNtfSpecificProblemT)),
		('perceivedSeverity', POINTER(SaNtfSeverityT)),
		('trend', POINTER(SaNtfSeverityTrendT)),
		('thresholdInformation', POINTER(SaNtfThresholdInformationT)),
		('monitoredAttributes', POINTER(SaNtfAttributeT)),
		('proposedRepairActions', POINTER(SaNtfProposedRepairActionT))]

class SaNtfSecurityAlarmDetectorT(Structure):
	"""Contain the security alarm detector in a security alarm notification.
	"""
	_fields_ = [('valueType', SaNtfValueTypeT),
		('value', SaNtfValueT)]

class SaNtfServiceUserT(Structure):
	"""Contain the service user and service provider in a security alarm
	notification.
	"""
	_fields_ = [('valueType', SaNtfValueTypeT),
		('value', SaNtfValueT)]

class SaNtfSecurityAlarmNotificationT(Structure):
	"""Contain pointers to the fields in a security alarm notification.
	"""
	_fields_ = [('notificationHandle', SaNtfNotificationHandleT),
		('notificationHeader', SaNtfNotificationHeaderT),
		('probableCause', POINTER(SaNtfProbableCauseT)),
		('severity', POINTER(SaNtfSeverityT)),
		('securityAlarmDetector',
			POINTER(SaNtfSecurityAlarmDetectorT)),
		('serviceUser', POINTER(SaNtfServiceUserT)),
		('serviceProvider', POINTER(SaNtfServiceUserT))]


class SaNtfMiscellaneousNotificationT(Structure):
	"""Contain pointers to the fields in a miscellaneous notification.
	"""
	_fields_ = [('notificationHandle', SaNtfNotificationHandleT),
		('notificationHeader', SaNtfNotificationHeaderT )]

saNtf.SA_NTF_ALLOC_SYSTEM_LIMIT = -1

SaNtfSubscriptionIdT = SaUint32T  

class SaNtfNotificationFilterHeaderT(Structure):
	"""Contain filter elements common to all notification types.
	"""
	_fields_ = [('numEventTypes', SaUint16T),
		('eventTypes', POINTER(SaNtfEventTypeT)),
		('numNotificationObjects', SaUint16T),
		('notificationObjects', POINTER(SaNameT)),
		('numNotifyingObjects', SaUint16T),
		('notifyingObjects', POINTER(SaNameT)),
		('numNotificationClassIds', SaUint16T),
		('notificationClassIds', POINTER(SaNtfClassIdT))]

class SaNtfObjectCreateDeleteNotificationFilterT(Structure):
	"""Contain filter elements for an object create/delete notification
	filter.
	"""
	_fields_ = [('notificationFilterHandle',
			SaNtfNotificationFilterHandleT),
		('notificationFilterHeader', SaNtfNotificationFilterHeaderT),
		('numSourceIndicators', SaUint16T),
		('sourceIndicators', POINTER(SaNtfSourceIndicatorT))]

class SaNtfAttributeChangeNotificationFilterT(Structure):
	"""Contain filter elements for an attribute change notification
	filter.
	"""
	_fields_ = [('notificationFilterHandle',
			SaNtfNotificationFilterHandleT),
		('notificationFilterHeader', SaNtfNotificationFilterHeaderT),
		('numSourceIndicators', SaUint16T),
		('sourceIndicators', POINTER(SaNtfSourceIndicatorT))]

#ifdef SA_NTF_A01
class SaNtfStateChangeNotificationFilterT(Structure):
	"""Contain filter elements for a state change notification filter.
	"""
	_fields_ = [('notificationFilterHandle',
			SaNtfNotificationFilterHandleT),
		('notificationFilterHeader', SaNtfNotificationFilterHeaderT),
		('numSourceIndicators', SaUint16T),
		('sourceIndicators', POINTER(SaNtfSourceIndicatorT)),
		('numStateChanges', SaUint16T),
		('changedStates', POINTER(SaNtfStateChangeT))]
#endif /* SA_NTF_A01 */

class SaNtfStateChangeNotificationFilterT_2(Structure):
	"""Contain filter elements for a state change notification filter.
	"""
	_fields_ = [('notificationFilterHandle',
			SaNtfNotificationFilterHandleT),
		('notificationFilterHeader', SaNtfNotificationFilterHeaderT),
		('numSourceIndicators', SaUint16T),
		('sourceIndicators', POINTER(SaNtfSourceIndicatorT)),
		('numStateChanges', SaUint16T),
		('stateId', POINTER(SaNtfElementIdT))]

class SaNtfAlarmNotificationFilterT(Structure):
	"""Contain filter elements for an alarm notification filter.
	"""
	_fields_ = [('notificationFilterHandle',
			SaNtfNotificationFilterHandleT),
		('notificationFilterHeader', SaNtfNotificationFilterHeaderT),
		('numProbableCauses', SaUint16T),
		('numPerceivedSeverities', SaUint16T),
		('numTrends', SaUint16T),
		('probableCauses', POINTER(SaNtfProbableCauseT)),
		('perceivedSeverities', POINTER(SaNtfSeverityT)),
		('trends', POINTER(SaNtfSeverityTrendT))]

class SaNtfSecurityAlarmNotificationFilterT(Structure):
	"""Contain filter elements for a security alarm notification filter.
	"""
	_fields_ = [('notificationFilterHandle',
			SaNtfNotificationFilterHandleT),
		('notificationFilterHeader',SaNtfNotificationFilterHeaderT ),
		('numProbableCauses',SaUint16T ),
		('numSeverities', SaUint16T),
		('numSecurityAlarmDetectors', SaUint16T),
		('numServiceUsers', SaUint16T),
		('numServiceProviders', SaUint16T),
		('probableCauses', POINTER(SaNtfProbableCauseT)),
		('severities', POINTER(SaNtfSeverityT)),
		('securityAlarmDetectors',
			POINTER(SaNtfSecurityAlarmDetectorT)),
		('serviceUsers', POINTER(SaNtfServiceUserT)),
		('serviceProviders', POINTER(SaNtfServiceUserT))]

class SaNtfMiscellaneousNotificationFilterT(Structure):
	"""Contain filter elements for a miscellaneous notification filter.
	"""
	_fields_ = [('notificationFilterHandle',
			SaNtfNotificationFilterHandleT),
		('notificationFilterHeader', SaNtfNotificationFilterHeaderT)]

SaNtfSearchModeT = SaEnumT
eSaNtfSearchModeT = Enumeration((
	('SA_NTF_SEARCH_BEFORE_OR_AT_TIME', 1),
	('SA_NTF_SEARCH_AT_TIME', 2),
	('SA_NTF_SEARCH_AT_OR_AFTER_TIME', 3),
	('SA_NTF_SEARCH_BEFORE_TIME', 4),
	('SA_NTF_SEARCH_AFTER_TIME', 5),
	('SA_NTF_SEARCH_NOTIFICATION_ID', 6),
	('SA_NTF_SEARCH_ONLY_FILTER', 7),
))

class SaNtfSearchCriteriaT(Structure):
	"""Contain the search criteria for the Reader API.
	"""
	_fields_ = [('searchMode', SaNtfSearchModeT),
		('eventTime', SaTimeT),
		('notificationId', SaNtfIdentifierT)]

SaNtfSearchDirectionT = SaEnumT
eSaNtfSearchDirectionT = Enumeration((
	('SA_NTF_SEARCH_OLDER', 1),
	('SA_NTF_SEARCH_YOUNGER', 2),
))

#if defined(SA_NTF_A01) || defined(SA_NTF_A02)
class SaNtfNotificationTypeFilterHandlesT(Structure):
	"""Contain the fields for notification filter handles of all
	notification types.
	"""
	_fields_ = [('objectCreateDeleteFilterHandle',
			SaNtfNotificationFilterHandleT),
		('attributeChangeFilterHandle',
			SaNtfNotificationFilterHandleT),
		('stateChangeFilterHandle', SaNtfNotificationFilterHandleT),
		('alarmFilterHandle', SaNtfNotificationFilterHandleT),
		('securityAlarmFilterHandle', SaNtfNotificationFilterHandleT)]
#endif /* SA_NTF_A01 || SA_NTF_A02 */

class SaNtfNotificationTypeFilterHandlesT_3(Structure):
	"""Contain the fields for notification filter handles of all
	notification types.
	"""
	_fields_ = [('objectCreateDeleteFilterHandle',
			SaNtfNotificationFilterHandleT),
		('attributeChangeFilterHandle',
			SaNtfNotificationFilterHandleT),
		('stateChangeFilterHandle', SaNtfNotificationFilterHandleT),
		('alarmFilterHandle', SaNtfNotificationFilterHandleT),
		('securityAlarmFilterHandle', SaNtfNotificationFilterHandleT),
		('miscellaneousFilterHandle', SaNtfNotificationFilterHandleT)]

saNtf.SA_NTF_FILTER_HANDLE_NULL = None

#if defined(SA_NTF_A01) || defined(SA_NTF_A02)
class _notification(Union):
	_fields_ = [('objectCreateDeleteNotification',
			SaNtfObjectCreateDeleteNotificationT),
		('attributeChangeNotification',
			SaNtfAttributeChangeNotificationT),
		('stateChangeNotification', SaNtfStateChangeNotificationT),
		('alarmNotification', SaNtfAlarmNotificationT),
		('securityAlarmNotification', SaNtfSecurityAlarmNotificationT)]

class SaNtfNotificationsT(Structure):
	"""Contain a union of all notification-type specific structures.
	"""
	_fields_ = [('notificationType', SaNtfNotificationTypeT),
		('notification', _notification)]
#endif /* SA_NTF_A01 || SA_NTF_A02 */

class _notification_3(Union):
	_fields_ = [('objectCreateDeleteNotification',
			SaNtfObjectCreateDeleteNotificationT),
		('attributeChangeNotification',
			SaNtfAttributeChangeNotificationT),
		('stateChangeNotification', SaNtfStateChangeNotificationT_3),
		('alarmNotification', SaNtfAlarmNotificationT),
		('securityAlarmNotification', SaNtfSecurityAlarmNotificationT),
		('miscellaneousNotification', SaNtfMiscellaneousNotificationT)]

class SaNtfNotificationsT_3(Structure):
	"""Contain a union of all notification-type specific structures.
	"""
	_fields_ = [('notificationType', SaNtfNotificationTypeT),
		('notification', _notification_3)]

SaNtfStateT = SaEnumT
eSaNtfStateT = Enumeration((
	('SA_NTF_STATIC_FILTER_STATE', 1),
	('SA_NTF_SUBSCRIBER_STATE', 2),
))

SaNtfStaticFilterStateT = SaEnumT
eSaNtfStaticFilterStateT = Enumeration((
	('SA_NTF_STATIC_FILTER_STATE_INACTIVE', 1),
	('SA_NTF_STATIC_FILTER_STATE_ACTIVE', 2),
))

SaNtfSubscriberStateT = SaEnumT
eSaNtfSubscriberStateT = Enumeration((
	('SA_NTF_SUBSCRIBER_STATE_FORWARD_NOT_OK', 1),
	('SA_NTF_SUBSCRIBER_STATE_FORWARD_OK', 2),
))

SaNtfNotificationMinorIdT = SaEnumT
eSaNtfNotificationMinorIdT = Enumeration((
	('SA_NTF_NTFID_STATIC_FILTER_ACTIVATED', 0x065),
	('SA_NTF_NTFID_STATIC_FILTER_DEACTIVATED', 0x066),
	('SA_NTF_NTFID_CONSUMER_SLOW', 0x067),
	('SA_NTF_NTFID_CONSUMER_FAST_ENOUGH', 0x068),
))

#if defined(SA_NTF_A01) || defined(SA_NTF_A02)
SaNtfNotificationCallbackT = CFUNCTYPE(None,
	SaNtfSubscriptionIdT, POINTER(SaNtfNotificationsT))
#endif /* SA_NTF_A01 || SA_NTF_A02 */

SaNtfNotificationDiscardedCallbackT = CFUNCTYPE(None,
	SaNtfSubscriptionIdT, SaNtfNotificationTypeT, SaUint32T,
	POINTER(SaNtfIdentifierT))

#ifdef SA_NTF_A01
class SaNtfCallbacksT(Structure):
	"""Contain various callbacks Notification Service may invoke on
	registrant.
	"""
	_fields_ = [('saNtfNotificationCallback',
			SaNtfNotificationCallbackT),
		('saNtfNotificationDiscardedCallback',
			SaNtfNotificationDiscardedCallbackT)]
#endif /* SA_NTF_A01 */

#ifdef SA_NTF_A02
SaNtfStaticSuppressionFilterSetCallbackT = CFUNCTYPE(None,
	SaNtfHandleT, SaUint16T)

class SaNtfCallbacksT_2(Structure):
	"""Contain various callbacks Notification Service may invoke on
	registrant.
	"""
	_fields_ = [('saNtfNotificationCallback',
			SaNtfNotificationCallbackT),
		('saNtfNotificationDiscardedCallback',
			SaNtfNotificationDiscardedCallbackT),
		('saNtfStaticSuppressionFilterSetCallback',
			SaNtfStaticSuppressionFilterSetCallbackT)]
#endif /* SA_NTF_A02 */

SaNtfStaticSuppressionFilterSetCallbackT_3 = CFUNCTYPE(None,
	SaNtfHandleT, SaNtfEventTypeBitmapT)
    
SaNtfNotificationCallbackT_3 = CFUNCTYPE(None,
	SaNtfSubscriptionIdT, POINTER(SaNtfNotificationsT_3))

class SaNtfCallbacksT_3(Structure):
	"""Contain various callbacks Notification Service may invoke on
	registrant.
	"""
	_fields_ = [('saNtfNotificationCallback',
			SaNtfNotificationCallbackT_3),
		('saNtfNotificationDiscardedCallback',
			SaNtfNotificationDiscardedCallbackT),
		('saNtfStaticSuppressionFilterSetCallback',
			SaNtfStaticSuppressionFilterSetCallbackT_3)]

SaNtfAdminOperationIdT = SaEnumT
eSaNtfAdminOperationIdT = Enumeration((
	('SA_NTF_ADMIN_ACTIVATE_STATIC_SUPPRESSION_FILTER', 1),
	('SA_NTF_ADMIN_DEACTIVATE_STATIC_SUPPRESSION_FILTER', 2),
))

#ifdef SA_NTF_A01
def saNtfInitialize(ntfHandle, ntfCallbacks, version):
	"""Register invoking process with Notification Service.

	type arguments:
		SaNtfHandleT ntfHandle
		SaNtfCallbacksT ntfCallbacks
		SaVersionT version

	returns:
		SaAisErrorT

	"""
	return ntfdll.saNtfInitialize(BYREF(ntfHandle),
			BYREF(ntfCallbacks),
			BYREF(version))

def saNtfLocalizedMessageFree(message):
	"""Free memory previously allocated for a localized message by
	saNtfLocalizedMessageGet().

	type arguments:
		SaStringT message

	returns:
		SaAisErrorT

	"""
	return ntfdll.saNtfLocalizedMessageFree(message)

def saNtfStateChangeNotificationFilterAllocate(ntfHandle,
		notificationFilter,
		numEventTypes,
		numNotificationObjects,
		numNotifyingObjects,
		numNotificationClassIds,
		numSourceIndicators,
		numChangedStates):
	"""Allocate memory for a state change notification filter.

	type arguments:
		SaNtfHandleT ntfHandle,
		SaNtfStateChangeNotificationFilterT notificationFilter,
		SaUint16T numEventTypes,
		SaUint16T numNotificationObjects,
		SaUint16T numNotifyingObjects,
		SaUint16T numNotificationClassIds,
		SaUint16T numSourceIndicators,
		SaUint16T numChangedStates);

	returns:
		SaAisErrorT

	"""
	return ntfdll.saNtfStateChangeNotificationFilterAllocate(ntfHandle,
			BYREF(notificationFilter),
			numEventTypes,
			numNotificationObjects,
			numNotifyingObjects,
			numNotificationClassIds,
			numSourceIndicators,
			numChangedStates)

def saNtfNotificationUnsubscribe(subscriptionId):
	"""Delete subscription previously established via
	saNtfNotificationSubscribe().

	type arguments:
		SaNtfSubscriptionIdT subscriptionId

	returns:
		SaAisErrorT

	"""
	return ntfdll.saNtfNotificationUnsubscribe(subscriptionId)

def saNtfNotificationReadInitialize(searchCriteria,
		notificationFilterHandles, readHandle):
	"""Initialize reader according to search criteria and filters.

	type arguments:
		SaNtfSearchCriteriaT searchCriteria
		SaNtfNotificationTypeFilterHandlesT notificationFilterHandles,
		SaNtfReadHandleT readHandle);

	returns:
		SaAisErrorT

	"""
	return ntfdll.saNtfNotificationReadInitialize(searchCriteria,
			BYREF(notificationFilterHandles),
			BYREF(readHandle))
#endif /* SA_NTF_A01 */

#ifdef SA_NTF_A02
def saNtfInitialize_2(ntfHandle, ntfCallbacks, version):
	"""Register invoking process with Notification Service.

	type arguments:
		SaNtfHandleT ntfHandle
		SaNtfCallbacksT_2 ntfCallbacks
		SaVersionT version

	returns:
		SaAisErrorT

	"""
	return ntfdll.saNtfInitialize_2(BYREF(ntfHandle),
			BYREF(ntfCallbacks),
			BYREF(version))

def saNtfNotificationReadInitialize_2(searchCriteria,
		notificationFilterHandles,
		readHandle):
	"""Initialize reader according to search criteria and filters.

	type arguments:
		SaNtfSearchCriteriaT searchCriteria
		SaNtfNotificationTypeFilterHandlesT notificationFilterHandles
		SaNtfReadHandleT readHandle

	returns:
		SaAisErrorT

	"""
	return ntfdll.saNtfNotificationReadInitialize_2(BYREF(searchCriteria),
			BYREF(notificationFilterHandles),
			BYREF(readHandle))
#endif /* SA_NTF_A02 */

#if defined(SA_NTF_A01) || defined(SA_NTF_A02)
def saNtfNotificationSubscribe(notificationFilterHandles, subscriptionId):
	"""Subscribe for notifications by registering one or more filters.

	type arguments:
		SaNtfNotificationTypeFilterHandlesT notificationFilterHandles
		SaNtfSubscriptionIdT subscriptionId

	returns:
		SaAisErrorT

	"""
	return ntfdll.saNtfNotificationSubscribe(
			BYREF(notificationFilterHandles), subscriptionId)
#endif /* SA_NTF_A01 || SA_NTF_A02 */

def saNtfInitialize_3(ntfHandle, ntfCallbacks, version):
	"""Register invoking process with Notification Service.

	type arguments:
		SaNtfHandleT ntfHandle
		SaNtfCallbacksT_3 ntfCallbacks
		SaVersionT version

	returns:
		SaAisErrorT

	"""
	return ntfdll.saNtfInitialize_3(BYREF(ntfHandle),
			BYREF(ntfCallbacks),
			BYREF(version))

def saNtfSelectionObjectGet(ntfHandle, selectionObject):
	"""Return operating system handle associated with NTF handle to detect
	pending callbacks.

	type arguments:
		SaNtfHandleT ntfHandle
		SaSelectionObjectT selectionObject

	returns:
		SaAisErrorT

	"""
	return ntfdll.saNtfSelectionObjectGet(ntfHandle,
			BYREF(selectionObject))

def saNtfDispatch(ntfHandle, dispatchFlags):
	"""Invoke callbacks pending for the NTF handle.

	type arguments:
		SaNtfHandleT ntfHandle
		SaDispatchFlagsT dispatchFlags

	returns:
		SaAisErrorT

	"""
	return ntfdll.saNtfDispatch(ntfHandle, dispatchFlags)

def saNtfFinalize(ntfHandle):
	"""Close association between Notification Service and the handle.

	type arguments:
		SaNtfHandleT ntfHandle

	returns:
		SaAisErrorT

	"""
	return ntfdll.saNtfFinalize(ntfHandle)

def saNtfObjectCreateDeleteNotificationAllocate(ntfHandle,
		notification,
		numCorrelatedNotifications,
		lengthAdditionalText,
		numAdditionalInfo,
		numAttributes,
		variableDataSize):
	"""Allocate memory for object create/delete notification.

	type arguments:
		SaNtfHandleT ntfHandle
		SaNtfObjectCreateDeleteNotificationT *notification
		SaUint16T numCorrelatedNotifications
		SaUint16T lengthAdditionalText
		SaUint16T numAdditionalInfo
		SaUint16T numAttributes
		SaInt16T variableDataSize

	returns:
		SaAisErrorT

	"""
	return ntfdll.saNtfObjectCreateDeleteNotificationAllocate(ntfHandle,
			BYREF(notification),
			numCorrelatedNotifications,
			lengthAdditionalText,
			numAdditionalInfo,
			numAttributes,
			variableDataSize)

def saNtfAttributeChangeNotificationAllocate(ntfHandle,
		notification,
		numCorrelatedNotifications,
		lengthAdditionalText,
		numAdditionalInfo,
		numAttributes,
		variableDataSize):
	"""Allocate memory for attribute change notification.

	type arguments:
		SaNtfHandleT ntfHandle
		SaNtfAttributeChangeNotificationT *notification
		SaUint16T numCorrelatedNotifications
		SaUint16T lengthAdditionalText
		SaUint16T numAdditionalInfo
		SaUint16T numAttributes
		SaInt16T variableDataSize

	returns:
		SaAisErrorT

	"""
	return ntfdll.saNtfAttributeChangeNotificationAllocate(ntfHandle,
			BYREF(notification),
			numCorrelatedNotifications,
			lengthAdditionalText,
			numAdditionalInfo,
			numAttributes,
			variableDataSize)

#if defined(SA_NTF_A01) || defined(SA_NTF_A02)
def saNtfStateChangeNotificationAllocate(ntfHandle,
		notification,
		numCorrelatedNotifications,
		lengthAdditionalText,
		numAdditionalInfo,
		numStateChanges,
		variableDataSize):
	"""Allocate memory for state change notification.

	type arguments:
		SaNtfHandleT ntfHandle
		SaNtfStateChangeNotificationT *notification
		SaUint16T numCorrelatedNotifications
		SaUint16T lengthAdditionalText
		SaUint16T numAdditionalInfo
		SaUint16T numStateChanges
		SaInt16T variableDataSize

	returns:
		SaAisErrorT

	"""
	return ntfdll.saNtfStateChangeNotificationAllocate(ntfHandle,
			BYREF(notification),
			numCorrelatedNotifications,
			lengthAdditionalText,
			numAdditionalInfo,
			numStateChanges,
			variableDataSize)
#endif /* SA_NTF_A01 || SA_NTF_A02 */
    
def saNtfStateChangeNotificationAllocate_3(ntfHandle,
		notification,
		numCorrelatedNotifications,
		lengthAdditionalText,
		numAdditionalInfo,
		numStateChanges,
		variableDataSize):
	"""Allocate memory for state change notification.

	type arguments:
		SaNtfHandleT ntfHandle
		SaNtfStateChangeNotificationT_3 *notification
		SaUint16T numCorrelatedNotifications
		SaUint16T lengthAdditionalText
		SaUint16T numAdditionalInfo
		SaUint16T numStateChanges
		SaInt16T variableDataSize

	returns:
		SaAisErrorT

	"""
	return ntfdll.saNtfStateChangeNotificationAllocate_3(ntfHandle,
			BYREF(notification),
			numCorrelatedNotifications,
			lengthAdditionalText,
			numAdditionalInfo,
			numStateChanges,
			variableDataSize)

def saNtfAlarmNotificationAllocate(ntfHandle,
		notification,
		numCorrelatedNotifications,
		lengthAdditionalText,
		numAdditionalInfo,
		numSpecificProblems,
		numMonitoredAttributes,
		numProposedRepairActions,
		variableDataSize):
	"""Allocate memory for alarm notification.

	type arguments:
		SaNtfHandleT ntfHandle
		SaNtfAlarmNotificationT *notification
		SaUint16T numCorrelatedNotifications
		SaUint16T lengthAdditionalText
		SaUint16T numAdditionalInfo
		SaUint16T numSpecificProblems
		SaUint16T numMonitoredAttributes
		SaUint16T numProposedRepairActions
		SaInt16T variableDataSize

	returns:
		SaAisErrorT

	"""
	return ntfdll.saNtfAlarmNotificationAllocate(ntfHandle,
			BYREF(notification),
			numCorrelatedNotifications,
			lengthAdditionalText,
			numAdditionalInfo,
			numSpecificProblems,
			numMonitoredAttributes,
			numProposedRepairActions,
			variableDataSize)

def saNtfSecurityAlarmNotificationAllocate(ntfHandle,
		notification,
		numCorrelatedNotifications,
		lengthAdditionalText,
		numAdditionalInfo,
		variableDataSize):
	"""Allocate memory for security alarm notification.

	type arguments:
		SaNtfHandleT ntfHandle
		SaNtfSecurityAlarmNotificationT *notification
		SaUint16T numCorrelatedNotifications
		SaUint16T lengthAdditionalText
		SaUint16T numAdditionalInfo
		SaInt16T variableDataSize

	returns:
		SaAisErrorT

	"""
	return ntfdll.saNtfSecurityAlarmNotificationAllocate(ntfHandle,
			BYREF(notification),
			numCorrelatedNotifications,
			lengthAdditionalText,
			numAdditionalInfo,
			variableDataSize)

def saNtfMiscellaneousNotificationAllocate(ntfHandle,
		notification,
		numCorrelatedNotifications,
		lengthAdditionalText,
		numAdditionalInfo,
		variableDataSize):
	"""Allocate memory for miscellaneous notification.

	type arguments:
		SaNtfHandleT ntfHandle
		SaNtfMiscellaneousNotificationT *notification
		SaUint16T numCorrelatedNotifications
		SaUint16T lengthAdditionalText
		SaUint16T numAdditionalInfo
		SaInt16T variableDataSize

	returns:
		SaAisErrorT

	"""
	return ntfdll.saNtfMiscellaneousNotificationAllocate(ntfHandle,
			BYREF(notification),
			numCorrelatedNotifications,
			lengthAdditionalText,
			numAdditionalInfo,
			variableDataSize)

def saNtfPtrValAllocate(notificationHandle,
		dataSize,
		dataPtr,
		value):
	"""Allocate memory for an element of the notification structure.

	type arguments:
		SaNtfNotificationHandleT notificationHandle
		SaUint16T dataSize
		SaVoidPtr dataPtr
		SaNtfValueT value

	returns:
		SaAisErrorT

	"""
	return ntfdll.saNtfPtrValAllocate(notificationHandle,
			dataSize,
			BYREF(dataPtr),
			BYREF(value))

def saNtfArrayValAllocate(notificationHandle,
		numElements,
		elementSize,
		arrayPtr,
		value):
	"""Allocate memory of array type for an element of the notification
	structure.

	type arguments:
		SaNtfNotificationHandleT notificationHandle
		SaUint16T numElements
		SaUint16T elementSize
		SaVoidPtr arrayPtr
		SaNtfValueT value

	returns:
		SaAisErrorT

	"""
	return ntfdll.saNtfArrayValAllocate(notificationHandle,
			numElements,
			elementSize,
			BYREF(arrayPtr),
			BYREF(value))

def saNtfIdentifierAllocate(notificationHandle, notificationIdentifier):
	"""Allocate a notification identifier for notification structure.

	type arguments:
		SaNtfNotificationHandleT notificationHandle
		SaNtfIdentifierT notificationIdentifier

	returns:
		SaAisErrorT

	"""
	return ntfdll.saNtfIdentifierAllocate(notificationHandle,
			BYREF(notificationIdentifier))

def saNtfNotificationSend(notificationHandle):
	"""Send the notification.

	type arguments:
		SaNtfNotificationHandleT notificationHandle

	returns:
		SaAisErrorT

	"""
	return ntfdll.saNtfNotificationSend(notificationHandle)

def saNtfNotificationSendWithId(notificationHandle, notificationIdentifier):
	"""Send the notification with an identifier.

	type arguments:
		SaNtfNotificationHandleT notificationHandle
		SaNtfIdentifierT notificationIdentifier

	returns:
		SaAisErrorT

	"""
	return ntfdll.saNtfNotificationSendWithId(notificationHandle,
			notificationIdentifier)

def saNtfNotificationFree(notificationHandle):
	"""Free memory previously allocated for a notification.

	type arguments:
		SaNtfNotificationHandleT notificationHandle

	returns:
		SaAisErrorT

	"""
	return ntfdll.saNtfNotificationFree(notificationHandle)

def saNtfVariableDataSizeGet(notificationHandle, variableDataSpaceAvailable):
	"""Get the amount of memory space available for data elements of
	variable size in the context of the notification.

	type arguments:
		SaNtfNotificationHandleT notificationHandle
		SaUint16T *variableDataSpaceAvailable

	returns:
		SaAisErrorT

	"""
	return ntfdll.saNtfVariableDataSizeGet(notificationHandle,
			BYREF(variableDataSpaceAvailable))

def saNtfLocalizedMessageGet(notificationHandle, message):
	"""Get a localized textual description of the situation that resulted in
	the notification associated with the handle.

	type arguments:
		SaNtfNotificationHandleT notificationHandle
		SaStringT *message

	returns:
		SaAisErrorT

	"""
	return ntfdll.saNtfLocalizedMessageGet(notificationHandle,
			BYREF(message))

def saNtfLocalizedMessageFree_2(ntfHandle, message):
	"""Free memory previously allocated for a localized message by
	saNtfLocalizedMessageGet().

	type arguments:
		SaNtfHandleT ntfHandle
		SaStringT message

	returns:
		SaAisErrorT

	"""
	return ntfdll.saNtfLocalizedMessageFree_2(ntfHandle, message)

def saNtfPtrValGet(notificationHandle, value, dataPtr, dataSize):
	"""Get a pointer to returned data associated with the notification.

	type arguments:
		SaNtfNotificationHandleT notificationHandle
		SaNtfValueT *value
		void **dataPtr
		SaUint16T *dataSize

	returns:
		SaAisErrorT

	"""
	return ntfdll.saNtfPtrValGet(notificationHandle,
			BYREF(value),
			BYREF(dataPtr),
			BYREF(dataSize))

def saNtfArrayValGet(notificationHandle,
		value, arrayPtr, numElements, elementSize):
	"""Get a pointer to returned data array associated with the
	notification.

	type arguments:
		SaNtfNotificationHandleT notificationHandle
		SaNtfValueT *value
		void **arrayPtr
		SaUint16T *numElements
		SaUint16T *elementSize

	returns:
		SaAisErrorT

	"""
	return ntfdll.saNtfArrayValGet(notificationHandle,
			BYREF(value),
			BYREF(arrayPtr),
			BYREF(numElements),
			BYREF(elementSize))

def saNtfObjectCreateDeleteNotificationFilterAllocate(ntfHandle,
		notificationFilter,
		numEventTypes,
		numNotificationObjects,
		numNotifyingObjects,
		numNotificationClassIds,
		numSourceIndicators):
	"""Allocate memory for object create/delete notification filter.

	type arguments:
		SaNtfHandleT ntfHandle
		SaNtfObjectCreateDeleteNotificationFilterT *notificationFilter
		SaUint16T numEventTypes
		SaUint16T numNotificationObjects
		SaUint16T numNotifyingObjects
		SaUint16T numNotificationClassIds
		SaUint16T numSourceIndicators

	returns:
		SaAisErrorT

	"""
	return ntfdll.saNtfObjectCreateDeleteNotificationFilterAllocate(
			ntfHandle,
			BYREF(notificationFilter),
			numEventTypes,
			numNotificationObjects,
			numNotifyingObjects,
			numNotificationClassIds,
			numSourceIndicators)

def saNtfAttributeChangeNotificationFilterAllocate(ntfHandle,
		notificationFilter,
		numEventTypes,
		numNotificationObjects,
		numNotifyingObjects,
		numNotificationClassIds,
		numSourceIndicators):
	"""Allocate memory for attribute change notification filter.

	type arguments:
		SaNtfHandleT ntfHandle
		SaNtfAttributeChangeNotificationFilterT *notificationFilter
		SaUint16T numEventTypes
		SaUint16T numNotificationObjects
		SaUint16T numNotifyingObjects
		SaUint16T numNotificationClassIds
		SaUint16T numSourceIndicators

	returns:
		SaAisErrorT

	"""
	return ntfdll.saNtfAttributeChangeNotificationFilterAllocate(ntfHandle,
			BYREF(notificationFilter),
			numEventTypes,
			numNotificationObjects,
			numNotifyingObjects,
			numNotificationClassIds,
			numSourceIndicators)

def saNtfStateChangeNotificationFilterAllocate_2(ntfHandle,
		notificationFilter,
		numEventTypes,
		numNotificationObjects,
		numNotifyingObjects,
		numNotificationClassIds,
		numSourceIndicators,
		numChangedStates):
	"""Allocate memory for state change notification filter.

	type arguments:
		SaNtfHandleT ntfHandle
		SaNtfStateChangeNotificationFilterT_2 *notificationFilter
		SaUint16T numEventTypes
		SaUint16T numNotificationObjects
		SaUint16T numNotifyingObjects
		SaUint16T numNotificationClassIds
		SaUint16T numSourceIndicators
		SaUint16T numChangedStates)

	returns:
		SaAisErrorT

	"""
	return ntfdll.saNtfStateChangeNotificationFilterAllocate_2(ntfHandle,
			BYREF(notificationFilter),
			numEventTypes,
			numNotificationObjects,
			numNotifyingObjects,
			numNotificationClassIds,
			numSourceIndicators,
			numChangedStates)

def saNtfAlarmNotificationFilterAllocate(ntfHandle,
		notificationFilter,
		numEventTypes,
		numNotificationObjects,
		numNotifyingObjects,
		numNotificationClassIds,
		numProbableCauses,
		numPerceivedSeverities,
		numTrends):
	"""Allocate memory for alarm notification filter.

	type arguments:
		SaNtfHandleT ntfHandle
		SaNtfAlarmNotificationFilterT *notificationFilter
		SaUint16T numEventTypes
		SaUint16T numNotificationObjects
		SaUint16T numNotifyingObjects
		SaUint16T numNotificationClassIds
		SaUint16T numProbableCauses
		SaUint16T numPerceivedSeverities
		SaUint16T numTrends

	returns:
		SaAisErrorT

	"""
	return ntfdll.saNtfAlarmNotificationFilterAllocate(ntfHandle,
			BYREF(notificationFilter),
			numEventTypes,
			numNotificationObjects,
			numNotifyingObjects,
			numNotificationClassIds,
			numProbableCauses,
			numPerceivedSeverities,
			numTrends)

def saNtfSecurityAlarmNotificationFilterAllocate(ntfHandle,
		notificationFilter,
		numEventTypes,
		numNotificationObjects,
		numNotifyingObjects,
		numNotificationClassIds,
		numProbableCauses,
		numSeverities,
		numSecurityAlarmDetectors,
		numServiceUsers,
		numServiceProviders):
	"""Allocate memory for security alarm notification filter.

	type arguments:
		SaNtfHandleT ntfHandle
		SaNtfSecurityAlarmNotificationFilterT *notificationFilter
		SaUint16T numEventTypes
		SaUint16T numNotificationObjects
		SaUint16T numNotifyingObjects
		SaUint16T numNotificationClassIds
		SaUint16T numProbableCauses
		SaUint16T numSeverities
		SaUint16T numSecurityAlarmDetectors
		SaUint16T numServiceUsers
		SaUint16T numServiceProviders

	returns:
		SaAisErrorT

	"""
	return ntfdll.saNtfSecurityAlarmNotificationFilterAllocate(ntfHandle,
			BYREF(notificationFilter),
			numEventTypes,
			numNotificationObjects,
			numNotifyingObjects,
			numNotificationClassIds,
			numProbableCauses,
			numSeverities,
			numSecurityAlarmDetectors,
			numServiceUsers,
			numServiceProviders)

def saNtfNotificationFilterFree(notificationFilterHandle):
	"""Free memory previously allocated for a notification filter.

	type arguments:
		SaNtfNotificationFilterHandleT notificationFilterHandle

	returns:
		SaAisErrorT

	"""
	return ntfdll.saNtfNotificationFilterFree(notificationFilterHandle)

def saNtfNotificationSubscribe_3(notificationFilterHandles, subscriptionId):
	"""Subscribe process for notifications registered by filters.

	type arguments:
		SaNtfNotificationTypeFilterHandlesT_3 *notificationFilterHandles
		SaNtfSubscriptionIdT subscriptionId

	returns:
		SaAisErrorT

	"""
	return ntfdll.saNtfNotificationSubscribe_3(
			BYREF(notificationFilterHandles),
			subscriptionId)

def saNtfNotificationReadInitialize_3(searchCriteria,
		notificationFilterHandles,
		readHandle):
	"""Initialize reading of logged notifications according to search
	criteria and filters.

	type arguments:
		SaNtfSearchCriteriaT *searchCriteria
		SaNtfNotificationTypeFilterHandlesT_3 *notificationFilterHandles
		SaNtfReadHandleT *readHandle

	returns:
		SaAisErrorT

	"""
	return ntfdll.saNtfNotificationReadInitialize_3(BYREF(searchCriteria),
			BYREF(notificationFilterHandles),
			BYREF(readHandle))

def saNtfNotificationUnsubscribe_2(ntfHandle, subscriptionId):
	"""Delete subscription previously established via
	saNtfNotificationSubscribe_3().

	type arguments:
		SaNtfHandleT ntfHandle
		SaNtfSubscriptionIdT subscriptionId

	returns:
		SaAisErrorT

	"""
	return ntfdll.saNtfNotificationUnsubscribe_2(ntfHandle, subscriptionId)

#if defined(SA_NTF_A01) || defined(SA_NTF_A02)
def saNtfNotificationReadNext(readHandle, searchDirection, notification):
	"""Chronologically read ordered, logged notifications initialized via
	saNtfNotificationReadInitialize().

	type arguments:
		SaNtfReadHandleT readHandle
		SaNtfSearchDirectionT searchDirection
		SaNtfNotificationsT *notification

	returns:
		SaAisErrorT

	"""
	return ntfdll.saNtfNotificationReadNext(readHandle,
			searchDirection,
			BYREF(notification))
#endif /* SA_NTF_A01 || SA_NTF_A02 */

def saNtfNotificationReadNext_3(readHandle, searchDirection, notification):
	"""Chronologically read ordered, logged notifications initialized via
	saNtfNotificationReadInitialize_3().

	type arguments:
		SaNtfReadHandleT readHandle
		SaNtfSearchDirectionT searchDirection
		SaNtfNotificationsT_3 *notification

	returns:
		SaAisErrorT

	"""
	return ntfdll.saNtfNotificationReadNext_3(readHandle,
			searchDirection,
			BYREF(notification))
    
def saNtfNotificationReadFinalize(readHandle):
	"""Release resources bound to the read handle.

	type arguments:
		SaNtfReadHandleT readHandle

	returns:
		SaAisErrorT

	"""
	return ntfdll.saNtfNotificationReadFinalize(readHandle)
