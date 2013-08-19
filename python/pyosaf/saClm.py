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

clmdll = CDLL('libSaClm.so.0')

SaClmHandleT = SaUint64T
SaClmNodeIdT = SaUint32T

saClm = Const()

saClm.SA_CLM_LOCAL_NODE_ID = 0XFFFFFFFF
saClm.SA_CLM_MAX_ADDRESS_LENGTH = 64

SaClmNodeAddressFamilyT = SaEnumT
eSaClmNodeAddressFamilyT = Enumeration((
	('SA_CLM_AF_INET', 1),
	('SA_CLM_AF_INET6', 2),
))

SaClmClusterChangesT = SaEnumT
eSaClmClusterChangesT = Enumeration((
	('SA_CLM_NODE_NO_CHANGE', 1),
	('SA_CLM_NODE_JOINED', 2),
	('SA_CLM_NODE_LEFT', 3),
	('SA_CLM_NODE_RECONFIGURED', 4),
	('SA_CLM_NODE_UNLOCK', 5),
	('SA_CLM_NODE_SHUTDOWN', 6),
))

SaClmStateT = SaEnumT
eSaClmStateT = Enumeration((
	('SA_CLM_CLUSTER_CHANGE_STATUS', 1),
	('SA_CLM_ADMIN_STATE', 2),
))

SaClmAdminOperationIdT = SaEnumT
eSaClmAdminOperationIdT = Enumeration((
	('SA_CLM_ADMIN_UNLOCK', 1),
	('SA_CLM_ADMIN_LOCK', 2),
	('SA_CLM_ADMIN_SHUTDOWN', 3),
))

SaClmAdminStateT = SaEnumT
eSaClmAdminStateT = Enumeration((
	('SA_CLM_ADMIN_UNLOCKED', 1),
	('SA_CLM_ADMIN_LOCKED', 2),
	('SA_CLM_ADMIN_SHUTTING_DOWN', 3),
))

#ifdef SA_CLM_B03
SaClmAdditionalInfoIdT = SaEnumT
eSaClmAdditionalInfoIdT = Enumeration((
	('SA_CLM_NODE_NAME', 1),
))
#endif /* SA_CLM_B03 */

SaClmAdditionalInfoIdT_4 = SaEnumT
eSaClmAdditionalInfoIdT_4 = Enumeration((
	('SA_CLM_ROOT_CAUSE_ENTITY', 1),
))

class SaClmNodeAddressT(Structure):
	"""Contain string representation of communication address associated
	with cluster node.
	"""
	_fields_ = [('family', SaClmNodeAddressFamilyT),
		('length', SaUint16T),
		('value', SaInt8T*saClm.SA_CLM_MAX_ADDRESS_LENGTH)]
	def __init__(self, family=0, address=''):
		"""Construct instance of 'family' with contents of 'name'.
		"""
		super(SaClmNodeAddressT, self).__init__(family,
				len(address), address)

SaClmChangeStepT = SaEnumT
eSaClmChangeStepT = Enumeration((
	('SA_CLM_CHANGE_VALIDATE', 1),
	('SA_CLM_CHANGE_START', 2),
	('SA_CLM_CHANGE_ABORTED', 3),
	('SA_CLM_CHANGE_COMPLETED', 4),
))

SaClmResponseT = SaEnumT
eSaClmResponseT = Enumeration((
	('SA_CLM_CALLBACK_RESPONSE_OK', 1),
	('SA_CLM_CALLBACK_RESPONSE_REJECTED', 2),
	('SA_CLM_CALLBACK_RESPONSE_ERROR', 3),
))

SaClmNotificationMinorIdT = SaEnumT
eSaClmNotificationMinorIdT = Enumeration((
	('SA_CLM_NTFID_NODE_JOIN', 0x065),
	('SA_CLM_NTFID_NODE_LEAVE', 0x066),
	('SA_CLM_NTFID_NODE_RECONFIG', 0x067),
	('SA_CLM_NTFID_NODE_ADMIN_STATE', 0x068),
))

#if defined(SA_CLM_B01) || defined(SA_CLM_B02) || defined(SA_CLM_B03)
class SaClmClusterNodeT(Structure):
	"""Contain detailed information about a cluster node.
	"""
	_fields_ = [('nodeId', SaClmNodeIdT),
		('nodeAddress', SaClmNodeAddressT),
		('nodeName', SaNameT),
		('member', SaBoolT),
		('bootTimestamp', SaTimeT),
		('initialViewNumber', SaUint64T)]

class SaClmClusterNotificationT(Structure):
	"""Contain information regarding changes in cluster membership.
	"""
	_fields_ = [('clusterNode', SaClmClusterNodeT),
		('clusterChange', SaClmClusterChangesT)]

class SaClmClusterNotificationBufferT(Structure):
	"""Contain array of SaClmClusterNotificationT structures.
	"""
	_fields_ = [('viewNumber', SaUint64T),
		('numberOfItems', SaUint32T),
		('notification', POINTER(SaClmClusterNotificationT))]
#endif /* SA_CLM_B01 || SA_CLM_B02 || SA_CLM_B03 */

class SaClmClusterNodeT_4(Structure):
	"""Contain detailed information about a cluster node.
	"""
	_fields_ = [('nodeId', SaClmNodeIdT),
		('nodeAddress', SaClmNodeAddressT),
		('nodeName', SaNameT),
		('executionEnvironment', SaNameT),
		('member', SaBoolT),
		('bootTimestamp', SaTimeT),
		('initialViewNumber', SaUint64T)]

class SaClmClusterNotificationT_4(Structure):
	"""Contain information regarding changes in cluster membership.
	"""
	_fields_ = [('clusterNode', SaClmClusterNodeT_4),
		('clusterChange', SaClmClusterChangesT)]

class SaClmClusterNotificationBufferT_4(Structure):
	"""Contain array of SaClmClusterNotificationT structures.
	"""
	_fields_ = [('viewNumber', SaUint64T),
		('numberOfItems', SaUint32T),
		('notification', POINTER(SaClmClusterNotificationT_4))]

#if defined(SA_CLM_B01) || defined(SA_CLM_B02) || defined(SA_CLM_B03)
SaClmClusterTrackCallbackT = CFUNCTYPE(None,
		POINTER(SaClmClusterNotificationBufferT),
		SaUint32T,
		SaAisErrorT)

SaClmClusterNodeGetCallbackT = CFUNCTYPE(None,
		SaInvocationT,
		POINTER(SaClmClusterNodeT),
		SaAisErrorT)
#endif /* SA_CLM_B01 || SA_CLM_B02 || SA_CLM_B03 */

#if defined(SA_CLM_B01) || defined(SA_CLM_B02)
class SaClmCallbacksT(Structure):
	"""Contain various callbacks CLM service may invoke on registrant.
	"""
	_fields_ = [('saClmClusterNodeGetCallback',
			SaClmClusterNodeGetCallbackT),
		('saClmClusterTrackCallback',
			SaClmClusterTrackCallbackT)]
#endif /* SA_CLM_B01 || SA_CLM_B02 */

#ifdef SA_CLM_B03
SaClmClusterNodeEvictionCallbackT = CFUNCTYPE(None,
		SaClmHandleT,
		SaInvocationT,
		SaClmAdminOperationIdT)

class SaClmCallbacksT_3(Structure):
	"""Contain various callbacks CLM service may invoke on registrant.
	"""
	_fields_ = [('saClmClusterNodeGetCallback',
			SaClmClusterNodeGetCallbackT),
		('saClmClusterTrackCallback',
			SaClmClusterTrackCallbackT),
		('saClmClusterNodeEvictionCallback',
			SaClmClusterNodeEvictionCallbackT)]
#endif /* SA_CLM_B03 */

SaClmClusterNodeGetCallbackT_4 = CFUNCTYPE(None,
		SaInvocationT,
		POINTER(SaClmClusterNodeT_4),
		SaAisErrorT)

SaClmClusterTrackCallbackT_4 = CFUNCTYPE(None,
		POINTER(SaClmClusterNotificationBufferT_4),
		SaUint32T,
		SaInvocationT,
		POINTER(SaNameT),
		POINTER(saNtf.SaNtfCorrelationIdsT),
		SaClmChangeStepT,
		SaTimeT,
		SaAisErrorT)

class SaClmCallbacksT_4(Structure):
	"""Contain various callbacks CLM service may invoke on registrant.
	"""
	_fields_ = [('saClmClusterNodeGetCallback',
			SaClmClusterNodeGetCallbackT_4),
		('saClmClusterTrackCallback',
			SaClmClusterTrackCallbackT_4)]

#*************************************************/
#******** CLM API function declarations **********/
#*************************************************/
#if defined(SA_CLM_B01) || defined(SA_CLM_B02)
def saClmInitialize(clmHandle, clmCallbacks, version):
	"""Register invoking process with CLM.

	type arguments:
		SaClmHandleT clmHandle
		SaClmCallbacksT clmCallbacks
		SaVersionT version

	returns:
		SaAisErrorT
	"""
	
	clmdll.saClmInitialize.argtypes = [POINTER(SaClmHandleT),
									   POINTER(SaClmCallbacksT),
									   POINTER(SaVersionT)]
	
	clmdll.saClmInitialize.restype = SaAisErrorT
	
	return clmdll.saClmInitialize(BYREF(clmHandle),
			BYREF(clmCallbacks), BYREF(version))
#endif /* SA_CLM_B01 || SA_CLM_B02 */

#ifdef SA_CLM_B03
def saClmInitialize_3(clmHandle, clmCallbacks, version):
	"""Register invoking process with CLM.

	type arguments:
		SaClmHandleT clmHandle
		SaClmCallbacksT_3 clmCallbacks
		SaVersionT version

	returns:
		SaAisErrorT
	"""

	return clmdll.saClmInitialize_3(BYREF(clmHandle), 
			BYREF(clmCallbacks), BYREF(version))
#endif /* SA_CLM_B03 */

def saClmInitialize_4(clmHandle, clmCallbacks, version):
	"""Register invoking process with CLM.

	type arguments:
		SaClmHandleT clmHandle
		SaClmCallbacksT_4 clmCallbacks
		SaVersionT version

	returns:
		SaAisErrorT
	"""
	clmdll.saClmInitialize_4.argtypes = [POINTER(SaClmHandleT),
										 POINTER(SaClmCallbacksT_4),
										 POINTER(SaVersionT)]
	
	clmdll.saClmInitialize_4.restype = SaAisErrorT
	
	return clmdll.saClmInitialize_4(BYREF(clmHandle),
			BYREF(clmCallbacks), BYREF(version))

def saClmSelectionObjectGet(clmHandle, selectionObject):
	"""Return operating system handle associated with CLM handle to detect
	pending callbacks.

	type arguments:
		SaClmHandleT clmHandle
		SaSelectionObjectT selectionObject

	returns:
		SaAisErrorT
	"""
	
	clmdll.saClmSelectionObjectGet.argtypes = [SaClmHandleT,
											   POINTER(SaSelectionObjectT)]
	
	clmdll.saClmSelectionObjectGet.restype = SaAisErrorT
	
	return clmdll.saClmSelectionObjectGet(clmHandle,
			BYREF(selectionObject))

def saClmDispatch(clmHandle, dispatchFlags):
	"""Invoke callbacks pending for the CLM handle.

	type arguments:
		SaClmHandleT clmHandle
		SaDispatchFlagsT dispatchFlags

	returns:
		SaAisErrorT
	"""
	
	clmdll.saClmDispatch.argtypes = [SaClmHandleT, SaDispatchFlagsT]
	
	clmdll.saClmDispatch.restype = SaAisErrorT
	
	return clmdll.saClmDispatch(clmHandle, dispatchFlags)

def saClmFinalize(clmHandle):
	"""Close association between CLM and the handle.

	type arguments:
		SaClmHandleT clmHandle

	returns:
		SaAisErrorT
	"""
	
	clmdll.saClmFinalize.argtypes = [SaClmHandleT]
	
	clmdll.saClmFinalize.restype = SaAisErrorT
	
	return clmdll.saClmFinalize(clmHandle)

#if defined(SA_CLM_B01) || defined(SA_CLM_B02) || defined(SA_CLM_B03)
def saClmClusterTrack(clmHandle, trackFlags, notificationBuffer):
	"""Obtain current cluster membership and request notification of changes
	in attributes of member nodes.

	type arguments:
		SaClmHandleT clmHandle
		SaUint8T trackFlags
		SaClmClusterNotificationBufferT notificationBuffer

	returns:
		SaAisErrorT
	"""
	
	clmdll.saClmClusterTrack.argtypes = [SaClmHandleT,
										 SaUint8T,
										 POINTER(SaClmClusterNotificationBufferT)]
	
	clmdll.saClmClusterTrack.restype = SaAisErrorT
	
	return clmdll.saClmClusterTrack(clmHandle,
			trackFlags,
			BYREF(notificationBuffer))

def saClmClusterNodeGet(clmHandle, nodeId, timeout, clusterNode):
	"""Synchronously retrieve information about identified member node.

	type arguments:
		SaClmHandleT clmHandle
		SaClmNodeIdT nodeId
		SaTimeT timeout
		SaClmClusterNodeT clusterNode

	returns:
		SaAisErrorT
	"""
	
	clmdll.saClmClusterNodeGet.argtypes = [SaClmHandleT, 
										SaClmNodeIdT,
										SaTimeT,
										POINTER(SaClmClusterNodeT)]
	
	clmdll.saClmClusterNodeGet.restype = SaAisErrorT
	
	return clmdll.saClmClusterNodeGet(clmHandle,
			nodeId, 
			timeout,
			BYREF(clusterNode))
#endif /* SA_CLM_B01 || SA_CLM_B02 || SA_CLM_B03 */

#if defined(SA_CLM_B02) || defined(SA_CLM_B03)
def saClmClusterNotificationFree(clmHandle, notification):
	"""Free notification memory allocated by saClmClusterTrack() function.

	type arguments:
		SaClmHandleT clmHandle
		SaClmClusterNotificationT notification

	returns:
		SaAisErrorT
	"""

	return clmdll.saClmClusterNotificationFree(clmHandle, notification)
#endif /* SA_CLM_B02 || SA_CLM_B03 */

def saClmClusterTrack_4(clmHandle, trackFlags, notificationBuffer):
	"""Obtain current cluster membership and request notification of changes
	in attributes of member nodes.

	type arguments:
		SaClmHandleT clmHandle
		SaUint8T trackFlags
		SaClmClusterNotificationBufferT_4 notificationBuffer

	returns:
		SaAisErrorT
	"""
	
	clmdll.saClmClusterTrack_4.argtypes = [SaClmHandleT,
										   SaUint8T,
										   POINTER(SaClmClusterNotificationBufferT_4)]

	clmdll.saClmClusterTrack_4.restype = SaAisErrorT
	
	return clmdll.saClmClusterTrack_4(clmHandle,
			trackFlags,
			BYREF(notificationBuffer))

def saClmClusterTrackStop(clmHandle):
	"""Stop any further notifications of cluster membership changes started
	via saClmClusterTrack_4().

	type arguments:
		SaClmHandleT clmHandle

	returns:
		SaAisErrorT
	"""
	
	clmdll.saClmClusterTrackStop.argtypes = [SaClmHandleT] 

	clmdll.saClmClusterTrackStop.restype = SaAisErrorT
	
	return clmdll.saClmClusterTrackStop(clmHandle)

def saClmClusterNotificationFree_4(clmHandle, notification):
	"""Free notification memory allocated by saClmClusterTrack_4() function.

	type arguments:
		SaClmHandleT clmHandle
		SaClmClusterNotificationT_4 notification

	returns:
		SaAisErrorT
	"""
	
	clmdll.saClmClusterNotificationFree_4.argtypes = [SaClmHandleT,
													  POINTER(SaClmClusterNotificationT_4)]
	
	clmdll.saClmClusterNotificationFree_4.restype = SaAisErrorT
	
	return clmdll.saClmClusterNotificationFree_4(clmHandle, 
												 BYREF(notification))

def saClmClusterNodeGet_4(clmHandle, nodeId, timeout, clusterNode):
	"""Synchronously retrieve information about identified member node.

	type arguments:
		SaClmHandleT clmHandle
		SaClmNodeIdT nodeId
		SaTimeT timeout
		SaClmClusterNodeT_4 clusterNode

	returns:
		SaAisErrorT
	"""
	
	clmdll.saClmClusterNodeGet_4.argtypes = [SaClmHandleT,
											 SaClmNodeItT,
											 SaTimeT,
											 POINTER(SaClmClusterNodeT_4)]
	
	clmdll.saClmClusterNodeGet_4.restype = SaAisErrorT
	
	return clmdll.saClmClusterNodeGet_4(clmHandle,
			nodeId,
			timeout,
			BYREF(clusterNode))

def saClmClusterNodeGetAsync(clmHandle, invocation, nodeId):
	"""Asynchronously retrieve information about identified member node.

	type arguments:
		SaClmHandleT clmHandle
		SaInvocationT invocation
		SaClmNodeIdT nodeId

	returns:
		SaAisErrorT
	"""
	
	clmdll.saClmClusterNodeGetAsync.argtypes = [SaClmHandleT,
												SaInvocationT,
												SaClmNodeIdT]
	
	clmdll.saClmClusterNodeGetAsync.restype = SaAisErrorT
	
	return clmdll.saClmClusterNodeGetAsync(clmHandle, invocation, nodeId)

def saClmResponse_4(clmHandle, invocation, response):
	"""Respond to CLM with result of callback invoked by CLM.

	type arguments:
		SaClmHandleT clmHandle
		SaInvocationT invocation
		SaClmResponseT response

	returns:
		SaAisErrorT
	"""
	
	clmdll.saClmResponse_4.argtypes = [SaClmHandleT, 
									   SaInvocationT,
									   SaClmResponseT] 

	clmdll.saClmResponse_4.restype = SaAisErrorT
	
	return clmdll.saClmResponse_4(clmHandle, invocation, response)
