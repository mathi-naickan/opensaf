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
from saNtf import *

# Only mirrors API calls implemented in osaf/libs/agents/saf/ava/ava_api.c
# If it ain't implemented here (or is commented out here),
# it probably ain't implemented there!

amfdll = CDLL('libSaAmf.so.0')

SaAmfHandleT = SaUint64T

saAmf = Const()

saAmf.SA_AMF_PM_ZERO_EXIT = 0x1
saAmf.SA_AMF_PM_NON_ZERO_EXIT = 0x2
saAmf.SA_AMF_PM_ABNORMAL_END = 0x4

SaAmfPmErrorsT = SaUint32T  

SaAmfPmStopQualifierT = SaEnumT
eSaAmfPmStopQualifierT = Enumeration((
	('SA_AMF_PM_PROC', 1),
	('SA_AMF_PM_PROC_AND_DESCENDENTS', 2),
	('SA_AMF_PM_ALL_PROCESSES', 3),
))

SaAmfHealthcheckInvocationT = SaEnumT
eSaAmfHealthcheckInvocationT = Enumeration((
	('SA_AMF_HEALTHCHECK_AMF_INVOKED', 1),
	('SA_AMF_HEALTHCHECK_COMPONENT_INVOKED', 2),
))

saAmf.SA_AMF_HEALTHCHECK_KEY_MAX = 32

class SaAmfHealthcheckKeyT(Structure):
	"""Contain the healtcheck key.
	"""
	_fields_ = [('key', (SaInt8T*saAmf.SA_AMF_HEALTHCHECK_KEY_MAX)),
		('keyLen', SaUint16T)]
	def __init__(self, key=''):
		"""Construct instance with contents of 'key'.
		"""
		super(SaAmfHealthcheckKeyT, self).__init__(key, len(key))

SaAmfHAStateT = SaEnumT
eSaAmfHAStateT = Enumeration((
	('SA_AMF_HA_ACTIVE', 1),
	('SA_AMF_HA_STANDBY', 2),
	('SA_AMF_HA_QUIESCED', 3),
	('SA_AMF_HA_QUIESCING', 4),
))

SaAmfReadinessStateT = SaEnumT
eSaAmfReadinessStateT = Enumeration((
	('SA_AMF_READINESS_OUT_OF_SERVICE', 1),
	('SA_AMF_READINESS_IN_SERVICE', 2),
	('SA_AMF_READINESS_STOPPING', 3),
))

SaAmfPresenceStateT = SaEnumT
eSaAmfPresenceStateT = Enumeration((
	('SA_AMF_PRESENCE_UNINSTANTIATED', 1),
	('SA_AMF_PRESENCE_INSTANTIATING', 2),
	('SA_AMF_PRESENCE_INSTANTIATED', 3),
	('SA_AMF_PRESENCE_TERMINATING', 4),
	('SA_AMF_PRESENCE_RESTARTING', 5),
	('SA_AMF_PRESENCE_INSTANTIATION_FAILED', 6),
	('SA_AMF_PRESENCE_TERMINATION_FAILED', 7),
))

SaAmfOperationalStateT = SaEnumT
eSaAmfOperationalStateT = Enumeration((
	('SA_AMF_OPERATIONAL_ENABLED', 1),
	('SA_AMF_OPERATIONAL_DISABLED', 2),
))

SaAmfAdminStateT = SaEnumT
eSaAmfAdminStateT = Enumeration((
	('SA_AMF_ADMIN_UNLOCKED', 1),
	('SA_AMF_ADMIN_LOCKED', 2),
	('SA_AMF_ADMIN_LOCKED_INSTANTIATION', 3),
	('SA_AMF_ADMIN_SHUTTING_DOWN', 4),
))

SaAmfAssignmentStateT = SaEnumT
eSaAmfAssignmentStateT = Enumeration((
	('SA_AMF_ASSIGNMENT_UNASSIGNED', 1),
	('SA_AMF_ASSIGNMENT_FULLY_ASSIGNED', 2),
	('SA_AMF_ASSIGNMENT_PARTIALLY_ASSIGNED', 3),
))

SaAmfHAReadinessStateT = SaEnumT
eSaAmfHAReadinessStateT = Enumeration((
	('SA_AMF_HARS_READY_FOR_ASSIGNMENT', 1),
	('SA_AMF_HARS_READY_FOR_ACTIVE_DEGRADED', 2),
	('SA_AMF_HARS_NOT_READY_FOR_ACTIVE', 3),
	('SA_AMF_HARS_NOT_READY_FOR_ASSIGNMENT', 4),
))

SaAmfProxyStatusT = SaEnumT
eSaAmfProxyStatusT = Enumeration((
	('SA_AMF_PROXY_STATUS_UNPROXIED', 1),
	('SA_AMF_PROXY_STATUS_PROXIED', 2),
))

SaAmfStateT = SaEnumT
eSaAmfStateT = Enumeration((
	('SA_AMF_READINESS_STATE', 1),
	('SA_AMF_HA_STATE', 2),
	('SA_AMF_PRESENCE_STATE', 3),
	('SA_AMF_OP_STATE', 4),
	('SA_AMF_ADMIN_STATE', 5),
	('SA_AMF_ASSIGNMENT_STATE', 6),
	('SA_AMF_PROXY_STATUS', 7),
	('SA_AMF_HA_READINESS_STATE', 8),
))

saAmf.SA_AMF_CSI_ADD_ONE = 0X1
saAmf.SA_AMF_CSI_TARGET_ONE = 0X2
saAmf.SA_AMF_CSI_TARGET_ALL = 0X4

SaAmfCSIFlagsT = SaUint32T  

SaAmfCSITransitionDescriptorT = SaEnumT
eSaAmfCSITransitionDescriptorT = Enumeration((
	('SA_AMF_CSI_NEW_ASSIGN', 1),
	('SA_AMF_CSI_QUIESCED', 2),
	('SA_AMF_CSI_NOT_QUIESCED', 3),
	('SA_AMF_CSI_STILL_ACTIVE', 4),
))

class SaAmfCSIActiveDescriptorT(Structure):
	"""Contain information associated with active assignment.
	"""
	_fields_ = [('transitionDescriptor',SaAmfCSITransitionDescriptorT),
		('activeCompName', SaNameT)]

class SaAmfCSIStandbyDescriptorT(Structure):
	"""Contain information associated with standby assignment.
	"""
	_fields_ = [('activeCompName', SaNameT),
		('standbyRank', SaUint32T)]

class SaAmfCSIStateDescriptorT(Union):
	"""Contain information associated with active or standby assignment.
	"""
	_fields_ = [('activeDescriptor', SaAmfCSIActiveDescriptorT),
		('standbyDescriptor', SaAmfCSIStandbyDescriptorT)]
	
class SaAmfCSIAttributeT(Structure):
	"""Contain a single CSI attribute's name and value strings.
	"""
	_fields_ = [('attrName', POINTER(SaUint8T)),
		('attrValue', POINTER(SaUint8T))]


class SaAmfCSIAttributeListT(Structure):
	"""Contain a list of all attributes for a single CSI.
	"""
	_fields_ = [('attr', POINTER(SaAmfCSIAttributeT)),
		('number', SaUint32T)]

class SaAmfCSIDescriptorT(Structure):
	"""Contain information about CSI targeted by saAmfCSISetCallback().
	"""
	_fields_ = [('csiFlags', SaAmfCSIFlagsT),
		('csiName', SaNameT),
		('csiStateDescriptor', SaAmfCSIStateDescriptorT),
		('csiAttr', SaAmfCSIAttributeListT)]


SaAmfProtectionGroupChangesT = SaEnumT
eSaAmfProtectionGroupChangesT = Enumeration((
	('SA_AMF_PROTECTION_GROUP_NO_CHANGE', 1),
	('SA_AMF_PROTECTION_GROUP_ADDED', 2),
	('SA_AMF_PROTECTION_GROUP_REMOVED', 3),
	('SA_AMF_PROTECTION_GROUP_STATE_CHANGE', 4),
))

#if defined(SA_AMF_B01) || defined(SA_AMF_B02) || defined(SA_AMF_B03)
class SaAmfProtectionGroupMemberT(Structure):
	"""Contain information about a member of a protection group.
	"""
	_fields_ = [('compName', SaNameT),
		('haState', SaAmfHAStateT),
		('rank', SaUint32T)]

class SaAmfProtectionGroupNotificationT(Structure):
	"""Contain information about members targeted by
	saAmfProtectionGroupTrackCallback().
	"""
	_fields_ = [('member', SaAmfProtectionGroupMemberT),
		('change', SaAmfProtectionGroupChangesT)]

def createProtectionGroupNotificationVector(size):
	"""Create sized array of SaAmfProtectionGroupNotificationT.
	"""
	return (SaAmfProtectionGroupNotificationT*size)() if size else None

class SaAmfProtectionGroupNotificationBufferT(Structure):
	"""Contain a list of SaAmfProtectionGroupNotificationT.
	"""
	_fields_ = [('numberOfItems', SaUint32T),
		('notification', POINTER(SaAmfProtectionGroupNotificationT))]
	def __init__(self, size=0):
		"""Construct instance of 'size'.
		"""
		super(SaAmfProtectionGroupNotificationBufferT, self).__init__(
			size, createProtectionGroupNotificationVector(size))
#endif /* SA_AMF_B01 || SA_AMF_B02 || SA_AMF_B03 */

class SaAmfProtectionGroupMemberT_4(Structure):
	"""Contain information about a member of a protection group.
	"""
	_fields_ = [('compName', SaNameT),
		('haState', SaAmfHAStateT),
		('haReadinessState', SaAmfHAReadinessStateT),
		('rank', SaUint32T)]

class SaAmfProtectionGroupNotificationT_4(Structure):
	"""Contain information about members targeted by
	saAmfProtectionGroupTrackCallback().
	"""
	_fields_ = [('member', SaAmfProtectionGroupMemberT_4),
		('change', SaAmfProtectionGroupChangesT)]

class SaAmfProtectionGroupNotificationBufferT_4(Structure):
	"""Contain a list of SaAmfProtectionGroupNotificationT_4.
	"""
	_fields_ = [('numberOfItems', SaUint32T),
		('notification', POINTER(SaAmfProtectionGroupNotificationT_4))]

SaAmfRecommendedRecoveryT = SaEnumT
eSaAmfRecommendedRecoveryT = Enumeration((
	('SA_AMF_NO_RECOMMENDATION', 1),
	('SA_AMF_COMPONENT_RESTART', 2),
	('SA_AMF_COMPONENT_FAILOVER', 3),
	('SA_AMF_NODE_SWITCHOVER', 4),
	('SA_AMF_NODE_FAILOVER', 5),
	('SA_AMF_NODE_FAILFAST', 6),
	('SA_AMF_CLUSTER_RESET', 7),
	('SA_AMF_APPLICATION_RESTART', 8),
	('SA_AMF_CONTAINER_RESTART', 9),
))

saAmf.SA_AMF_COMP_SA_AWARE = 0x0001
saAmf.SA_AMF_COMP_PROXY = 0x0002
saAmf.SA_AMF_COMP_PROXIED = 0x0004
saAmf.SA_AMF_COMP_LOCAL = 0x0008
saAmf.SA_AMF_COMP_CONTAINER = 0x0010
saAmf.SA_AMF_COMP_CONTAINED = 0x0020
saAmf.SA_AMF_COMP_PROXIED_NPI = 0x0040

SaAmfCompCategoryT = SaUint32T  

SaAmfRedundancyModelT = SaEnumT
eSaAmfRedundancyModelT = Enumeration((
	('SA_AMF_2N_REDUNDANCY_MODEL', 1),
	('SA_AMF_NPM_REDUNDANCY_MODEL', 2),
	('SA_AMF_N_WAY_REDUNDANCY_MODEL', 3),
	('SA_AMF_N_WAY_ACTIVE_REDUNDANCY_MODEL', 4),
	('SA_AMF_NO_REDUNDANCY_MODEL', 5),
))

SaAmfCompCapabilityModelT = SaEnumT
eSaAmfCompCapabilityModelT = Enumeration((
	('SA_AMF_COMP_X_ACTIVE_AND_Y_STANDBY', 1),
	('SA_AMF_COMP_X_ACTIVE_OR_Y_STANDBY', 2),
	('SA_AMF_COMP_ONE_ACTIVE_OR_Y_STANDBY', 3),
	('SA_AMF_COMP_ONE_ACTIVE_OR_ONE_STANDBY', 4),
	('SA_AMF_COMP_X_ACTIVE', 5),
	('SA_AMF_COMP_1_ACTIVE', 6),
	('SA_AMF_COMP_NON_PRE_INSTANTIABLE', 7),
))

SaAmfAdditionalInfoIdT = SaEnumT
eSaAmfAdditionalInfoIdT = Enumeration((
	('SA_AMF_NODE_NAME', 1),
	('SA_AMF_SI_NAME', 2),
	('SA_AMF_MAINTENANCE_CAMPAIGN_DN', 3),
	('SA_AMF_AI_RECOMMENDED_RECOVERY', 4),
	('SA_AMF_AI_APPLIED_RECOVERY', 5),
))

SaAmfNotificationMinorIdT = SaEnumT
eSaAmfNotificationMinorIdT = Enumeration((
# alarms
	('SA_AMF_NTFID_COMP_INSTANTIATION_FAILED', 0x02),
	('SA_AMF_NTFID_COMP_CLEANUP_FAILED', 0x03),
	('SA_AMF_NTFID_CLUSTER_RESET', 0x04),
	('SA_AMF_NTFID_SI_UNASSIGNED', 0x05),
	('SA_AMF_NTFID_COMP_UNPROXIED', 0x06),
# state change
	('SA_AMF_NTFID_NODE_ADMIN_STATE', 0x065),
	('SA_AMF_NTFID_SU_ADMIN_STATE', 0x066),
	('SA_AMF_NTFID_SG_ADMIN_STATE', 0x067),
	('SA_AMF_NTFID_SI_ADMIN_STATE', 0x068),
	('SA_AMF_NTFID_APP_ADMIN_STATE', 0x069),
	('SA_AMF_NTFID_CLUSTER_ADMIN_STATE', 0x06A),
	('SA_AMF_NTFID_NODE_OP_STATE', 0x06B),
	('SA_AMF_NTFID_SU_OP_STATE', 0x06C),
	('SA_AMF_NTFID_SU_PRESENCE_STATE', 0x06D),
	('SA_AMF_NTFID_SU_SI_HA_STATE', 0x06E),
	('SA_AMF_NTFID_SI_ASSIGNMENT_STATE', 0x06F),
	('SA_AMF_NTFID_COMP_PROXY_STATUS', 0x070),
	('SA_AMF_NTFID_SU_SI_HA_READINESS_STATE', 0x071),
# miscellaneous
	('SA_AMF_NTFID_ERROR_REPORT', 0x0191),
	('SA_AMF_NTFID_ERROR_CLEAR', 0x0192),
))

#*************************************************/
#************ Defs for AMF Admin API *************/
#*************************************************/

SaAmfAdminOperationIdT = SaEnumT
eSaAmfAdminOperationIdT = Enumeration((
	('SA_AMF_ADMIN_UNLOCK', 1),
	('SA_AMF_ADMIN_LOCK', 2),
	('SA_AMF_ADMIN_LOCK_INSTANTIATION', 3),
	('SA_AMF_ADMIN_UNLOCK_INSTANTIATION', 4),
	('SA_AMF_ADMIN_SHUTDOWN', 5),
	('SA_AMF_ADMIN_RESTART', 6),
	('SA_AMF_ADMIN_SI_SWAP', 7),
	('SA_AMF_ADMIN_SG_ADJUST', 8),
	('SA_AMF_ADMIN_REPAIRED', 9),
	('SA_AMF_ADMIN_EAM_START', 10),
	('SA_AMF_ADMIN_EAM_STOP', 11),
))

#*************************************************/
#******** AMF API function declarations **********/
#*************************************************/

SaAmfHealthcheckCallbackT = CFUNCTYPE(None,
	SaInvocationT, POINTER(SaNameT), POINTER(SaAmfHealthcheckKeyT))

SaAmfComponentTerminateCallbackT = CFUNCTYPE(None,
	SaInvocationT, POINTER(SaNameT))

SaAmfCSISetCallbackT = CFUNCTYPE(None,
	SaInvocationT, POINTER(SaNameT), SaAmfHAStateT,
	SaAmfCSIDescriptorT)

SaAmfCSIRemoveCallbackT = CFUNCTYPE(None,
	SaInvocationT, POINTER(SaNameT), POINTER(SaNameT), SaAmfCSIFlagsT)

#if defined(SA_AMF_B01) || defined(SA_AMF_B02) || defined(SA_AMF_B03)
SaAmfProtectionGroupTrackCallbackT = CFUNCTYPE(None,
	POINTER(SaNameT), POINTER(SaAmfProtectionGroupNotificationBufferT),
	SaUint32T, SaAisErrorT)
#endif /* SA_AMF_B01 || SA_AMF_B02 || SA_AMF_B03 */

SaAmfProtectionGroupTrackCallbackT_4 = CFUNCTYPE(None,
	POINTER(SaNameT), POINTER(SaAmfProtectionGroupNotificationBufferT_4),
	SaUint32T, SaAisErrorT)

SaAmfProxiedComponentInstantiateCallbackT = CFUNCTYPE(None,
	SaInvocationT, POINTER(SaNameT))

SaAmfProxiedComponentCleanupCallbackT = CFUNCTYPE(None,
	SaInvocationT, POINTER(SaNameT))

SaAmfContainedComponentInstantiateCallbackT = CFUNCTYPE(None,
	SaInvocationT, POINTER(SaNameT))

SaAmfContainedComponentCleanupCallbackT = CFUNCTYPE(None,
	SaInvocationT, POINTER(SaNameT))

#if defined(SA_AMF_B01) || defined(SA_AMF_B02)
class SaAmfCallbacksT(Structure):
	"""Contain various callbacks AMF may invoke on a component.
	"""
	_fields_ = [('saAmfHealthcheckCallback',
			SaAmfHealthcheckCallbackT),
		('saAmfComponentTerminateCallback',
			SaAmfComponentTerminateCallbackT),
		('saAmfCSISetCallback',
			SaAmfCSISetCallbackT),
		('saAmfCSIRemoveCallback',
			SaAmfCSIRemoveCallbackT),
		('saAmfProtectionGroupTrackCallback',
			SaAmfProtectionGroupTrackCallbackT),
		('saAmfProxiedComponentInstantiateCallback',
			SaAmfProxiedComponentInstantiateCallbackT),
		('saAmfProxiedComponentCleanupCallback',
			SaAmfProxiedComponentCleanupCallbackT)]

def saAmfInitialize(amfHandle, amfCallbacks, version):
	"""Register invoking process with AMF.

	type arguments:
		SaAmfHandleT amfHandle
		SaAmfCallbacksT amfCallbacks
		SaVersionT version

	returns:
		SaAisErrorT

	"""
	
	amfdll.saAmfInitialize.argtypes = [POINTER(SaAmfHandleT),
                                       POINTER(SaAmfCallbacksT),
                                       POINTER(SaVersionT)]

	amfdll.saAmfInitialize.restype = SaAisErrorT

	return amfdll.saAmfInitialize(BYREF(amfHandle),
			BYREF(amfCallbacks),
			BYREF(version))

def saAmfPmStart(amfHandle, compName, processId,
		descendentsTreeDepth, pmErrors, recommendedRecovery):
	"""Start AMF passive monitoring of specific errors.

	type arguments:
		SaAmfHandleT amfHandle
		SaNameT compName
		SaUint64T processId
		SaInt32T descendentsTreeDepth
		SaAmfPmErrorsT pmErrors
		SaAmfRecommendedRecoveryT recommendedRecovery

	returns:
		SaAisErrorT

	"""
	
	amfdll.saAmfPmStart.argtypes = [SaAmfHandleT,
                                    POINTER(SaNameT),
                                    SaUint64T,
                                    SaInt32T,
                                    SaAmfPmErrorsT,
                                    SaAmfRecommendedRecoveryT]

	amfdll.saAmfPmStart.restype = SaAisErrorT

	return amfdll.saAmfPmStart(amfHandle, BYREF(compName), processId,
			descendentsTreeDepth, pmErrors, recommendedRecovery)
#endif /* SA_AMF_B01 || SA_AMF_B02 */

#ifdef SA_AMF_B03 
class SaAmfCallbacksT_3(Structure):
	"""Contain various callbacks AMF may invoke on a component.
	"""
	_fields_ = [('saAmfHealthcheckCallback',
			SaAmfHealthcheckCallbackT),
		('saAmfComponentTerminateCallback',
			SaAmfComponentTerminateCallbackT),
		('saAmfCSISetCallback',
			SaAmfCSISetCallbackT),
		('saAmfCSIRemoveCallback',
			SaAmfCSIRemoveCallbackT),
		('saAmfProtectionGroupTrackCallback',
			SaAmfProtectionGroupTrackCallbackT),
		('saAmfProxiedComponentInstantiateCallback',
			SaAmfProxiedComponentInstantiateCallbackT),
		('saAmfProxiedComponentCleanupCallback',
			SaAmfProxiedComponentCleanupCallbackT),
		('saAmfContainedComponentInstantiateCallback',
			SaAmfContainedComponentInstantiateCallbackT),
		('saAmfContainedComponentCleanupCallback',
			SaAmfContainedComponentCleanupCallbackT)]

def saAmfInitialize_3(amfHandle, amfCallbacks, version):
	"""Register invoking process with AMF.

	type arguments:
		SaAmfHandleT amfHandle
		SaAmfCallbacksT_3 amfCallbacks
		SaVersionT version

	returns:
		SaAisErrorT

	"""
	
	amfdll.saAmfInitialize_3.argtypes = [POINTER(SaAmfHandleT),
                                         POINTER(SaAmfCallbacksT_3),
                                         POINTER(SaVersionT)]

	amfdll.saAmfInitialize_3.restype = SaAisErrorT

	return amfdll.saAmfInitialize_3(BYREF(amfHandle),
			BYREF(amfCallbacks),
			BYREF(version))
#endif /* SA_AMF_B03 */

#if defined(SA_AMF_B01) || defined(SA_AMF_B02) || defined(SA_AMF_B03)
def saAmfComponentUnregister(amfHandle, compName, proxyCompName):
	"""Unregister component or proxied component with AMF.

	type arguments:
		SaAmfHandleT amfHandle
		SaNameT compName
		SaNameT proxyCompName

	returns:
		SaAisErrorT

	"""
	
	amfdll.saAmfComponentUnregister.argtypes = [SaAmfHandleT,
                                                POINTER(SaNameT),
                                                POINTER(SaNameT)]

	amfdll.saAmfComponentUnregister.restype = SaAisErrorT

	return amfdll.saAmfComponentUnregister(amfHandle,
			BYREF(compName),
			BYREF(proxyCompName))

def saAmfProtectionGroupTrack(amfHandle,
		csiName, trackFlags, notificationBuffer):
	"""Start AMF tracking changes for the protection group.

	type arguments:
		SaAmfHandleT amfHandle
		SaNameT csiName
		SaUint8T trackFlags
		SaAmfProtectionGroupNotificationBufferT notificationBuffer

	returns:
		SaAisErrorT

	"""

	amfdll.saAmfProtectionGroupTrack.argtypes = [SaAmfHandleT,
                                                 POINTER(SaNameT),
                                                 SaUint8T,
                                                 POINTER(SaAmfProtectionGroupNotificationBufferT)]

	amfdll.saAmfProtectionGroupTrack.restype = SaAisErrorT

	return amfdll.saAmfProtectionGroupTrack(amfHandle,
			BYREF(csiName), trackFlags,
			BYREF(notificationBuffer))

def saAmfProtectionGroupNotificationFree(amfHandle, notification):
	"""Free notification buffer allocated by AMF in
	saAmfProtectionGroupTrack() function.

	type arguments:
		SaAmfHandleT amfHandle
		SaAmfProtectionGroupNotificationT notification

	returns:
		SaAisErrorT

	"""
	
	amfdll.saAmfProtectionGroupNotificationFree.argtypes = [SaAmfHandleT,
                                                            POINTER(SaAmfProtectionGroupNotificationT)]

	amfdll.saAmfProtectionGroupNotificationFree.restype = SaAisErrorT

	return amfdll.saAmfProtectionGroupNotificationFree(amfHandle,
			BYREF(notification))

def saAmfComponentErrorReport(amfHandle,
		compName, errorDetectionTime, recommendedRecovery,
		ntfIdentifier):
	"""Report error and provide recovery recommendation to AMF.

	type arguments:
		SaAmfHandleT amfHandle
		SaNameT compName
		SaTimeT errorDetectionTime
		SaAmfRecommendedRecoveryT recommendedRecovery
		SaNtfIdentifierT ntfIdentifier

	returns:
		SaAisErrorT

	"""
	
	amfdll.saAmfComponentErrorReport.argtypes = [SaAmfHandleT,
												POINTER(SaNameT),
												SaTimeT,
												SaAmfRecommendedRecoveryT,
												SaNtfIdentifierT]

	amfdll.saAmfComponentErrorReport.restype = SaAisErrorT

	return amfdll.saAmfComponentErrorReport(amfHandle, BYREF(compName),
			errorDetectionTime, recommendedRecovery, ntfIdentifier)

def saAmfComponentErrorClear(amfHandle, compName, ntfIdentifier):
	"""Cancel error previously reported for component.

	type arguments:
		SaAmfHandleT amfHandle
		SaNameT compName
		SaNtfIdentifierT ntfIdentifier

	returns:
		SaAisErrorT

	"""
	return amfdll.saAmfComponentErrorClear(amfHandle,
			BYREF(compName), ntfIdentifier)

def saAmfResponse(amfHandle, invocation, error):
	"""Respond to AMF with result of callback invoked by AMF.

	type arguments:
		SaAmfHandleT amfHandle
		SaInvocationT invocation
		SaAisErrorT error

	returns:
		SaAisErrorT

	"""
	
	amfdll.saAmfResponse.argtypes = [SaAmfHandleT,
                                     SaInvocationT,
                                     SaAisErrorT]

	amfdll.saAmfResponse.restype = SaAisErrorT

	return amfdll.saAmfResponse(amfHandle, invocation, error)
#endif /* SA_AMF_B01 || SA_AMF_B02 || SA_AMF_B03 */

class SaAmfCallbacksT_4(Structure):
	"""Contain various callbacks AMF may invoke on a component.
	"""
	_fields_ = [('saAmfHealthcheckCallback',
			SaAmfHealthcheckCallbackT),
		('saAmfComponentTerminateCallback',
			SaAmfComponentTerminateCallbackT),
		('saAmfCSISetCallback',
			SaAmfCSISetCallbackT),
		('saAmfCSIRemoveCallback',
			SaAmfCSIRemoveCallbackT),
		('saAmfProtectionGroupTrackCallback',
			SaAmfProtectionGroupTrackCallbackT_4),
		('saAmfProxiedComponentInstantiateCallback',
			SaAmfProxiedComponentInstantiateCallbackT),
		('saAmfProxiedComponentCleanupCallback',
			SaAmfProxiedComponentCleanupCallbackT),
		('saAmfContainedComponentInstantiateCallback',
			SaAmfContainedComponentInstantiateCallbackT),
		('saAmfContainedComponentCleanupCallback',
			SaAmfContainedComponentCleanupCallbackT)]

def saAmfInitialize_4(amfHandle, amfCallbacks, version):
	"""Register invoking process with AMF.

	type arguments:
		SaAmfHandleT amfHandle
		SaAmfCallbacksT_4 amfCallbacks
		SaVersionT version

	returns:
		SaAisErrorT

	"""
	
	amfdll.saAmfInitialize_4.argtypes = [POINTER(SaAmfHandleT),
                                         POINTER(SaAmfCallbacksT_4),
                                         POINTER(SaVersionT)]

	amfdll.saAmfInitialize_4.restype = SaAisErrorT

	return amfdll.saAmfInitialize_4(BYREF(amfHandle),
			BYREF(amfCallbacks), BYREF(version))

def saAmfSelectionObjectGet(amfHandle, selectionObject):
	"""Return operating system handle associated with AMF handle to detect
	pending callbacks.

	type arguments:
		SaAmfHandleT amfHandle
		SaSelectionObjectT selectionObject

	returns:
		SaAisErrorT

	"""
	
	amfdll.saAmfSelectionObjectGet.argtypes = [SaAmfHandleT,
                                               POINTER(SaSelectionObjectT)]

	amfdll.saAmfSelectionObjectGet.restype = SaAisErrorT

	return amfdll.saAmfSelectionObjectGet(amfHandle,
			BYREF(selectionObject))

def saAmfDispatch(amfHandle, dispatchFlags):
	"""Invoke callbacks pending for the AMF handle.

	type arguments:
		SaAmfHandleT amfHandle
		SaDispatchFlagsT dispatchFlags

	returns:
		SaAisErrorT

	"""
	
	amfdll.saAmfDispatch.argtypes = [SaAmfHandleT,
                                     SaDispatchFlagsT]

	amfdll.saAmfDispatch.restype = SaAisErrorT

	return amfdll.saAmfDispatch(amfHandle, dispatchFlags)

def saAmfFinalize(amfHandle):
	"""Close association between AMF and the handle.

	type arguments:
		SaAmfHandleT amfHandle

	returns:
		SaAisErrorT

	"""
	
	amfdll.saAmfFinalize.argtypes = [SaAmfHandleT]

	amfdll.saAmfFinalize.restype = SaAisErrorT

	return amfdll.saAmfFinalize(amfHandle)

def saAmfComponentRegister(amfHandle, compName, proxyCompName):
	"""Register component with AMF.

	type arguments:
		SaAmfHandleT amfHandle
		SaNameT compName
		SaNameT proxyCompName

	returns:
		SaAisErrorT

	"""
	
	amfdll.saAmfComponentRegister.argtypes = [SaAmfHandleT,
                                              POINTER(SaNameT),
                                              POINTER(SaNameT)]

	amfdll.saAmfComponentRegister.restype = SaAisErrorT

	return amfdll.saAmfComponentRegister(amfHandle,
			BYREF(compName),
			BYREF(proxyCompName))

def saAmfComponentNameGet(amfHandle, compName):
	"""Return the name of the component to which invoking process belongs.

	type arguments:
		SaAmfHandleT amfHandle
		SaNameT compName

	returns:
		SaAisErrorT

	"""
	
	amfdll.saAmfComponentNameGet.argtypes = [SaAmfHandleT,
                                             POINTER(SaNameT)]

	amfdll.saAmfComponentNameGet.restype = SaAisErrorT

	return amfdll.saAmfComponentNameGet(amfHandle, BYREF(compName))

def saAmfPmStart_3(amfHandle,
		compName, processId, descendentsTreeDepth, pmErrors,
		recommendedRecovery):
	"""Start AMF passive monitoring of specific errors.

	type arguments:
		SaAmfHandleT amfHandle
		SaNameT compName
		SaInt64T processId
		SaInt32T descendentsTreeDepth
		SaAmfPmErrorsT pmErrors
		SaAmfRecommendedRecoveryT recommendedRecovery

	returns:
		SaAisErrorT

	"""
	
	amfdll.saAmfPmStart_3.argtypes = [SaAmfHandleT,
                                      POINTER(SaNameT),
                                      SaInt64T,
                                      SaInt32T,
                                      SaAmfPmErrorsT,
                                      SaAmfRecommendedRecoveryT]

	amfdll.saAmfPmStart_3.restype = SaAisErrorT

	return amfdll.saAmfPmStart_3(amfHandle,
			BYREF(compName), processId, descendentsTreeDepth,
			pmErrors, recommendedRecovery)

def saAmfPmStop(amfHandle, compName, stopQualifier, processId, pmErrors):
	"""Stop AMF passive monitoring of specific errors.

	type arguments:
		SaAmfHandleT amfHandle
		SaNameT compName
		SaAmfPmStopQualifierT stopQualifier
		SaInt64T processId
		SaAmfPmErrorsT pmErrors

	returns:
		SaAisErrorT

	"""
	
	amfdll.saAmfPmStop.argtypes = [SaAmfHandleT,
                                   POINTER(SaNameT),
                                   SaAmfPmStopQualifierT,
                                   SaInt64T,
                                   SaAmfPmErrorsT]

	amfdll.saAmfPmStop.restype = SaAisErrorT

	return amfdll.saAmfPmStop(amfHandle,
			BYREF(compName), stopQualifier, processId, pmErrors)

def saAmfHealthcheckStart(amfHandle,
		compName, healthcheckKey, invocationType,
		recommendedRecovery):
	"""Start AMF healthcheck for component.

	type arguments:
		SaAmfHandleT amfHandle
		SaNameT compName
		SaAmfHealthcheckKeyT healthcheckKey
		SaAmfHealthcheckInvocationT invocationType
		SaAmfRecommendedRecoveryT recommendedRecovery

	returns:
		SaAisErrorT

	"""
	
	amfdll.saAmfHealthcheckStart.argtypes = [SaAmfHandleT,
                                             POINTER(SaNameT),
                                             POINTER(SaAmfHealthcheckKeyT),
                                             SaAmfHealthcheckInvocationT,
                                             SaAmfRecommendedRecoveryT]

	amfdll.saAmfHealthcheckStart.restype = SaAisErrorT

	return amfdll.saAmfHealthcheckStart(amfHandle,
			BYREF(compName), BYREF(healthcheckKey),
			invocationType, recommendedRecovery)

def saAmfHealthcheckConfirm(amfHandle,
		compName, healthcheckKey, healthcheckResult):
	"""Confirm with AMF that component has performed healthcheck.

	type arguments:
		SaAmfHandleT amfHandle
		SaNameT compName
		SaAmfHealthcheckKeyT healthcheckKey
		SaAisErrorT healthcheckResult

	returns:
		SaAisErrorT

	"""
	
	amfdll.saAmfHealthcheckConfirm.argtypes = [SaAmfHandleT,
                                               POINTER(SaNameT),
                                               POINTER(SaAmfHealthcheckKeyT),
                                               SaAisErrorT]

	amfdll.saAmfHealthcheckConfirm.restype = SaAisErrorT

	return amfdll.saAmfHealthcheckConfirm(amfHandle,
			BYREF(compName), BYREF(healthcheckKey),
			healthcheckResult)

def saAmfHealthcheckStop(amfHandle, compName, healthcheckKey):
	"""Stop AMF healthcheck for component.

	type arguments:
		SaAmfHandleT amfHandle
		SaNameT compName
		SaAmfHealthcheckKeyT healthcheckKey

	returns:
		SaAisErrorT

	"""
	
	amfdll.saAmfHealthcheckStop.argtypes = [SaAmfHandleT,
                                            POINTER(SaNameT),
                                            POINTER(SaAmfHealthcheckKeyT)]

	amfdll.saAmfHealthcheckStop.restype = SaAisErrorT

	return amfdll.saAmfHealthcheckStop(amfHandle,
			BYREF(compName), BYREF(healthcheckKey))

def saAmfCSIQuiescingComplete(amfHandle, invocation, error):
	"""Inform AMF that component associated with invocation has completed
	quiescing.

	type arguments:
		SaAmfHandleT amfHandle
		SaInvocationT invocation
		SaAisErrorT error

	returns:
		SaAisErrorT

	"""
	
	amfdll.saAmfCSIQuiescingComplete.argtypes = [SaAmfHandleT,
                                                 SaInvocationT,
                                                 SaAisErrorT]

	amfdll.saAmfCSIQuiescingComplete.restype = SaAisErrorT

	return amfdll.saAmfCSIQuiescingComplete(amfHandle, invocation, error)

def saAmfHAReadinessStateSet(amfHandle,
		compName, csiName, haReadinessState, correlationIds):
	"""Set HA readiness state of pre-instantiable component for named CSI.

	type arguments:
		SaAmfHandleT amfHandle
		SaNameT compName
		SaNameT csiName
		SaAmfHAReadinessStateT haReadinessState
		SaNtfCorrelationIdsT correlationIds

	returns:
		SaAisErrorT

	"""
	
	amfdll.saAmfHAReadinessStateSet.argtypes = [SaAmfHandleT,
											    POINTER(SaNameT),
											    POINTER(SaNameT),
											    SaAmfHAReadinessStateT,
											    POINTER(SaNtfCorrelationIdsT)]

	amfdll.saAmfHAReadinessStateSet.restype = SaAisErrorT
	
	return amfdll.saAmfHAReadinessStateSet(amfHandle,
			BYREF(compName), BYREF(csiName), haReadinessState,
			BYREF(correlationIds))

def saAmfHAStateGet(amfHandle, compName, csiName, haState):
	"""Get HA state of component for named CSI.

	type arguments:
		SaAmfHandleT amfHandle
		SaNameT compName
		SaNameT csiName
		SaAmfHAStateT haState

	returns:
		SaAisErrorT

	"""
	
	amfdll.saAmfHAStateGet.argtypes = [SaAmfHandleT,
                                       POINTER(SaNameT),
                                       POINTER(SaNameT),
                                       POINTER(SaAmfHAStateT)]

	amfdll.saAmfHAStateGet.restype = SaAisErrorT

	return amfdll.saAmfHAStateGet(amfHandle,
			BYREF(compName), BYREF(csiName), BYREF(haState))

def saAmfProtectionGroupTrack_4(amfHandle,
		csiName, trackFlags, notificationBuffer):
	"""Start AMF tracking changes for the protection group.

	type arguments:
		SaAmfHandleT amfHandle
		SaNameT csiName
		SaUint8T trackFlags
		SaAmfProtectionGroupNotificationBufferT_4 notificationBuffer

	returns:
		SaAisErrorT

	"""
	
	amfdll.saAmfProtectionGroupTrack_4.argtypes = [SaAmfHandleT,
                                                   POINTER(SaNameT),
                                                   SaUint8T,
                                                   POINTER(SaAmfProtectionGroupNotificationBufferT_4)]

	amfdll.saAmfProtectionGroupTrack_4.restype = SaAisErrorT

	return amfdll.saAmfProtectionGroupTrack_4(amfHandle, BYREF(csiName),
			trackFlags, BYREF(notificationBuffer))

def saAmfProtectionGroupTrackStop(amfHandle, csiName):
	"""Stop AMF tracking changes for the protection group.

	type arguments:
		SaAmfHandleT amfHandle
		SaNameT csiName

	returns:
		SaAisErrorT

	"""
	
	amfdll.saAmfProtectionGroupTrackStop.argtypes = [SaAmfHandleT,
                                                     POINTER(SaNameT)]

	amfdll.saAmfProtectionGroupTrackStop.restype = SaAisErrorT

	return amfdll.saAmfProtectionGroupTrackStop(amfHandle, BYREF(csiName))

def saAmfProtectionGroupNotificationFree_4(amfHandle, notification):
	"""Free notification buffer allocated by AMF in
	saAmfProtectionGroupTrack_4() function.

	type arguments:
		SaAmfHandleT amfHandle
		SaAmfProtectionGroupNotificationT_4 notification

	returns:
		SaAisErrorT

	"""
	
	amfdll.saAmfProtectionGroupNotificationFree_4.argtypes = [SaAmfHandleT,
                                                              POINTER(SaAmfProtectionGroupNotificationT_4)]

	amfdll.saAmfProtectionGroupNotificationFree_4.restype = SaAisErrorT

	return amfdll.saAmfProtectionGroupNotificationFree_4(amfHandle,
			BYREF(notification))

def saAmfComponentErrorReport_4(amfHandle,
		compName, errorDetectionTime, recommendedRecovery,
		correlationIds):
	"""Report error and provide recovery recommendation to AMF.

	type arguments:
		SaAmfHandleT amfHandle
		SaNameT compName
		SaTimeT errorDetectionTime
		SaAmfRecommendedRecoveryT recommendedRecovery
		SaNtfCorrelationIdsT correlationIds

	returns:
		SaAisErrorT

	"""
	
	amfdll.saAmfComponentErrorReport_4.argtypes = [SaAmfHandleT,
                                                   POINTER(SaNameT),
                                                   SaTimeT,
                                                   SaAmfRecommendedRecoveryT,
                                                   POINTER(SaNtfCorrelationIdsT)]

	amfdll.saAmfComponentErrorReport_4.restype = SaAisErrorT

	return amfdll.saAmfComponentErrorReport_4(amfHandle,
			BYREF(compName),
			errorDetectionTime,
			recommendedRecovery,
			BYREF(correlationIds))

def saAmfCorrelationIdsGet(amfHandle, invocation, correlationIds):
	"""Generate valid correlation ID for use in a notification in response
	to a callback invocation.

	type arguments:
		SaAmfHandleT amfHandle
		SaInvocationT invocation
		SaNtfCorrelationIdsT correlationIds

	returns:
		SaAisErrorT

	"""
	
	amfdll.saAmfCorrelationIdsGet.argtypes = [SaAmfHandleT,
                                              SaInvocationT,
                                              POINTER(SaNtfCorrelationIdsT)]

	amfdll.saAmfCorrelationIdsGet.restype = SaAisErrorT

	return amfdll.saAmfCorrelationIdsGet(amfHandle,
			invocation, BYREF(correlationIds))

def saAmfComponentErrorClear_4(amfHandle, compName, correlationIds):
	"""Cancel error previously reported for component.

	type arguments:
		SaAmfHandleT amfHandle
		SaNameT compName
		SaNtfCorrelationIdsT correlationIds

	returns:
		SaAisErrorT

	"""
	
	amfdll.saAmfComponentErrorClear_4.argtypes = [SaAmfHandleT,
                                                  POINTER(SaNtfCorrelationIdsT),
                                                  POINTER(SaNtfCorrelationIdsT)]

	amfdll.saAmfComponentErrorClear_4.restype = SaAisErrorT

	return amfdll.saAmfComponentErrorClear_4(amfHandle,
			BYREF(compName), BYREF(correlationIds))

def saAmfResponse_4(amfHandle, invocation, correlationIds, error):
	"""Respond to AMF with result of callback invoked by AMF.

	type arguments:
		SaAmfHandleT amfHandle
		SaInvocationT invocation
		SaNtfCorrelationIdsT correlationIds
		SaAisErrorT error

	returns:
		SaAisErrorT

	"""
	
	amfdll.saAmfResponse_4.argtypes = [SaAmfHandleT,
                                       SaInvocationT,
                                       POINTER(SaNtfCorrelationIdsT),
                                       SaAisErrorT]

	amfdll.saAmfResponse_4.restype = SaAisErrorT

	return amfdll.saAmfResponse_4(amfHandle, invocation,
			BYREF(correlationIds), error)
