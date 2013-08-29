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

from saImm import *

oidll = CDLL('libSaImmOi.so.0')

SaImmOiHandleT = SaUint64T

SaImmOiImplementerNameT = SaStringT

SaImmOiCcbIdT = SaUint64T

SaImmOiRtAttrUpdateCallbackT = CFUNCTYPE(SaAisErrorT,
	SaImmOiHandleT, POINTER(SaNameT), POINTER(SaImmAttrNameT))

SaImmOiCcbObjectCreateCallbackT_2 = CFUNCTYPE(SaAisErrorT,
	SaImmOiHandleT, SaImmOiCcbIdT, SaImmClassNameT, POINTER(SaNameT),
	POINTER(POINTER(SaImmAttrValuesT_2)))

SaImmOiCcbObjectDeleteCallbackT = CFUNCTYPE(SaAisErrorT,
	SaImmOiHandleT, SaImmOiCcbIdT, POINTER(SaNameT))

SaImmOiCcbObjectModifyCallbackT_2 = CFUNCTYPE(SaAisErrorT,
	SaImmOiHandleT, SaImmOiCcbIdT, POINTER(SaNameT),
	POINTER(POINTER(SaImmAttrModificationT_2)))

SaImmOiCcbCompletedCallbackT = CFUNCTYPE(SaAisErrorT,
	SaImmOiHandleT, SaImmOiCcbIdT)

SaImmOiCcbApplyCallbackT = CFUNCTYPE(None,
	SaImmOiHandleT, SaImmOiCcbIdT)

SaImmOiCcbAbortCallbackT = CFUNCTYPE(None,
	SaImmOiHandleT, SaImmOiCcbIdT)

SaImmOiAdminOperationCallbackT_2 = CFUNCTYPE(None,
	SaImmOiHandleT, SaInvocationT, POINTER(SaNameT),
	SaImmAdminOperationIdT, POINTER(POINTER(SaImmAdminOperationParamsT_2)))

class SaImmOiCallbacksT_2(Structure):
	"""Contain various callbacks IMM may invoke on a registrant.
	"""
#	_fields_ = [('SaImmOiAdminOperationCallbackT', ),
	#The downloaded includefile contained the above line which is a bug!! */
	_fields_ = [('saImmOiAdminOperationCallback',
			SaImmOiAdminOperationCallbackT_2),
		('saImmOiCcbAbortCallback',
			SaImmOiCcbAbortCallbackT),
		('saImmOiCcbApplyCallback',
			SaImmOiCcbApplyCallbackT),
		('saImmOiCcbCompletedCallback',
			SaImmOiCcbCompletedCallbackT),
		('saImmOiCcbObjectCreateCallback', 
			SaImmOiCcbObjectCreateCallbackT_2),
		('saImmOiCcbObjectDeleteCallback',
			SaImmOiCcbObjectDeleteCallbackT),
		('saImmOiCcbObjectModifyCallback',
			SaImmOiCcbObjectModifyCallbackT_2),
		('saImmOiRtAttrUpdateCallback',
			SaImmOiRtAttrUpdateCallbackT)]

def saImmOiInitialize_2(immOiHandle, immOiCallbacks, version):
	"""
	Register invoking process with IMM.

	type arguments:
		SaImmOiHandleT immOiHandle
		SaImmOiCallbacksT_2 immOiCallbacks
		SaVersionT version

	returns:
		SaAisErrorT

	"""
	oidll.saImmOiInitialize_2.argtypes = [POINTER(SaImmOiHandleT),
										  POINTER(SaImmOiCallbacksT_2),
										  POINTER(SaVersionT)]
	oidll.saImmOiInitialize_2.restype = SaAisErrorT
	
	return oidll.saImmOiInitialize_2(BYREF(immOiHandle),
			BYREF(immOiCallbacks),
			BYREF(version))

def saImmOiSelectionObjectGet(immOiHandle, selectionObject):
	"""Return operating system handle associated with IMM handle to detect
	pending callbacks.

	type arguments:
		SaImmOiHandleT immOiHandle
		SaSelectionObjectT selectionObject

	returns:
		SaAisErrorT

	"""
	
	oidll.saImmOiSelectionObjectGet.argtypes = [SaImmOiHandleT,
												POINTER(SaSelectionObjectT)]
	
	oidll.saImmOiSelectionObjectGet.restype = SaAisErrorT
	
	return oidll.saImmOiSelectionObjectGet(immOiHandle,
			BYREF(selectionObject))

def saImmOiDispatch(immOiHandle, dispatchFlags):
	"""Invoke callbacks pending for the IMM handle.

	type arguments:
		SaImmOiHandleT immOiHandle
		SaDispatchFlagsT dispatchFlags

	returns:
		SaAisErrorT

	"""
	
	oidll.saImmOiDispatch.argtypes = [SaImmOiHandleT,SaDispatchFlagsT]
	
	oidll.saImmOiDispatch.restype = SaAisErrorT
	
	return oidll.saImmOiDispatch(immOiHandle, dispatchFlags)

def saImmOiFinalize(immOiHandle):
	"""Close association between IMM and the handle.

	type arguments:
		SaImmOiHandleT immOiHandle

	returns:
		SaAisErrorT

	"""
	
	oidll.saImmOiFinalize.argtypes = [SaImmOiHandleT]
	
	oidll.saImmOiFinalize.restype = SaAisErrorT
	
	return oidll.saImmOiFinalize(immOiHandle)

def saImmOiImplementerSet(immOiHandle, implementerName):
	"""Set implementer name for the handle and register invoking process as
	the object implementer.

	type arguments:
		SaImmOiHandleT immOiHandle
		SaImmOiImplementerNameT implementerName

	returns:
		SaAisErrorT

	"""
	
	oidll.saImmOiImplementerSet.argtypes = [SaImmOiHandleT,
											SaImmOiImplementerNameT]
	oidll.saImmOiImplementerSet.restype = SaAisErrorT
	
	return oidll.saImmOiImplementerSet(immOiHandle, implementerName)

def saImmOiImplementerClear(immOiHandle):
	"""Clear implementer name for the handle and unregister the invoking
	process as the object implementer.

	type arguments:
		SaImmOiHandleT immOiHandle

	returns:
		SaAisErrorT

	"""
	oidll.saImmOiImplementerClear.argtypes = [SaImmOiHandleT]
	oidll.saImmOiImplementerClear.restype = SaAisErrorT
	
	return oidll.saImmOiImplementerClear(immOiHandle)

def saImmOiClassImplementerSet(immOiHandle, className):
	"""Assign object implementer associated with handle to the role(s)
	specified by the role parameter for all objects of class specified
	by className.

	type arguments:
		SaImmOiHandleT immOiHandle
		SaImmClassNameT className

	returns:
		SaAisErrorT

	"""
	oidll.saImmOiClassImplementerSet.argtypes = [SaImmOiHandleT,
												 SaImmClassNameT]
	
	oidll.saImmOiClassImplementerSet.restype = SaAisErrorT
	
	return oidll.saImmOiClassImplementerSet(immOiHandle, className)

def saImmOiClassImplementerRelease(immOiHandle, className):
	"""Release object implementer associated with handle from the role(s)
	specified by the role parameter for all objects of class specified
	by className.

	type arguments:
		SaImmOiHandleT immOiHandle
		SaImmClassNameT className

	returns:
		SaAisErrorT

	"""
	
	oidll.saImmOiClassImplementerRelease.argtypes = [SaImmOiHandleT,
													 SaImmClassNameT]
	
	oidll.saImmOiClassImplementerRelease.restype = SaAisErrorT
	
	return oidll.saImmOiClassImplementerRelease(immOiHandle, className)

def saImmOiObjectImplementerSet(immOiHandle, objectName, scope):
	"""Assign object implementer associated with handle to the role(s)
	specified by the role parameter for all objects identified by
	objectName and scope.

	type arguments:
		SaImmOiHandleT immOiHandle
		SaNameT objectName
		SaImmScopeT scope

	returns:
		SaAisErrorT

	"""
	
	oidll.saImmOiObjectImplementerSet.argtypes = [SaImmOiHandleT,
												  POINTER(SaNameT),
												  SaImmScopeT]
	
	oidll.saImmOiObjectImplementerSet.restype = SaAisErrorT
	
	return oidll.saImmOiObjectImplementerSet(immOiHandle,
			BYREF(objectName),
			scope)

def saImmOiObjectImplementerRelease(immOiHandle, objectName, scope):
	"""Release object implementer associated with handle from the role(s)
	specified by the role parameter for all objects identified by
	objectName and scope.

	type arguments:
		SaImmOiHandleT immOiHandle
		SaNameT objectName
		SaImmScopeT scope

	returns:
		SaAisErrorT

	"""
	
	oidll.saImmOiObjectImplementerRelease.argtypes = [SaImmOiHandleT,
													  POINTER(SaNameT),
													  SaImmScopeT]
	
	oidll.saImmOiObjectImplementerRelease.restype = SaAisErrorT
	
	return oidll.saImmOiObjectImplementerRelease(immOiHandle,
			BYREF(objectName),
			scope)


def saImmOiRtObjectCreate_2(immOiHandle, className, parentName, attrValues):
	"""Create new IMM service runtime object.

	type arguments:
		SaImmOiHandleT immOiHandle
		SaImmClassNameT className
		SaNameT parentName
		SaImmAttrValuesT_2 attrValues

	returns:
		SaAisErrorT

	"""
	
	oidll.saImmOiRtObjectCreate_2.argtypes = [SaImmOiHandleT,
											  SaImmClassNameT,
											  POINTER(SaNameT),
											  POINTER(SaImmAttrValuesT_2)]
	
	oidll.saImmOiRtObjectCreate_2.restype = SaAisErrorT
	
	return oidll.saImmOiRtObjectCreate_2(immOiHandle,
			className,
			BYREF(parentName),
			BYREF(attrValues))

def saImmOiRtObjectDelete(immOiHandle, objectName):
	"""Delete object and subtree rooted at that object.

	type arguments:
		SaImmOiHandleT immOiHandle
		SaNameT objectName

	returns:
		SaAisErrorT

	"""

	oidll.saImmOiRtObjectDelete.argtypes = [SaImmOiHandleT,
											POINTER(SaNameT)]
	oidll.saImmOiRtObjectDelete.restype = SaAisErrorT
	
	return oidll.saImmOiRtObjectDelete(immOiHandle, BYREF(objectName))

def saImmOiRtObjectUpdate_2(immOiHandle, objectName, attrMods):
	"""Update runtime attributes of configuration or runtime object.

	type arguments:
		SaImmOiHandleT immOiHandle
		SaNameT objectName
		SaImmAttrModificationT_2 attrMods

	returns:
		SaAisErrorT

	"""
	
	oidll.saImmOiRtObjectUpdate_2.argtypes = [SaImmOiHandleT,
											  POINTER(SaNameT),
											  POINTER(SaImmAttrModificationT_2)]
	
	oidll.saImmOiRtObjectUpdate_2.restype = SaAisErrorT
	
	return oidll.saImmOiRtObjectUpdate_2(immOiHandle,
			BYREF(objectName),
			BYREF(attrMods))

def saImmOiAdminOperationResult(immOiHandle, invocation, result):
	"""Respond to IMM with result of saImmOiAdminOperationCallback()
	invoked by IMM.

	type arguments:
		SaImmOiHandleT immOiHandle
		SaInvocationT invocation
		SaAisErrorT result

	returns:
		SaAisErrorT

	"""
	
	oidll.saImmOiAdminOperationResult.argtypes = [SaImmOiHandleT,
												  SaInvocationT,
												  SaAisErrorT]
	
	oidll.saImmOiAdminOperationResult.restype = SaAisErrorT
	
	return oidll.saImmOiAdminOperationResult(immOiHandle, invocation, result)

def saImmOiAdminOperationResult_o2(immOiHandle, invocation, result,
								   returnParams):
	"""Respond to IMM with result of saImmOiAdminOperationCallback()
	invoked by IMM.

	type arguments:
		SaImmOiHandleT immOiHandle
		SaInvocationT invocation
		SaAisErrorT result
		SaImmAdminOperationParamsT_2 returnParams

	returns:
		SaAisErrorT

	"""
	
	oidll.saImmOiAdminOperationResult_o2.argtypes = [SaImmOiHandleT,
												  SaInvocationT,
												  SaAisErrorT,
												  POINTER(POINTER(SaImmAdminOperationParamsT_2))]
	
	oidll.saImmOiAdminOperationResult_o2.restype = SaAisErrorT
	
	return oidll.saImmOiAdminOperationResult_o2(immOiHandle, invocation, result, BYREF(returnParams))

def saImmOiAugmentCcbInitialize(immOiHandle, ccbId64, ccbHandle, 
							    ownerHandle):
	"""Allows the OI implementers to augment ops to a ccb related
    to a current ccb-upcall, before returning from the upcall.
    This is allowed inside:
       SaImmOiCcbObjectCreateCallbackT
       SaImmOiCcbObjectDeleteCallbackT
       SaImmOiCcbObjectModifyCallbackT
    It is NOT allowed inside:
       SaImmOiCcbCompletedCallbackT
       SaImmOiApplyCallbackT
       SaImmOiAbortCallbackT
       
    type arguments:
       SaImmOiHandleT immOiHandle
       SaImmOiCcbIdT ccbId64
       SaImmCcbHandleT *ccbHandle
       SaImmAdminOwnerHandleT *ownerHandle
				     
    returns:
       SaAisErrorT
    """
    
	oidll.saImmOiAugmentCcbInitialize.argtypes = [SaImmOiHandleT, 
												  SaImmOiCcbIdT,
												  POINTER(SaImmCcbHandleT),
												  POINTER(SaImmAdminOwnerHandleT)]

	oidll.saImmOiAugmentCcbInitialize.restype = SaAisErrorT
    
	return oidll.saImmOiAugmentCcbInitialize(immOiHandle, ccbId64,
											 BYREF(ccbHandle),BYREF(ownerHandle))
    
def saImmOiCcbSetErrorString(immOiHandle, ccbId, errorString):
	"""Allows the OI implementers to post an error string related
    to a current ccb-upcall, before returning from the upcall.
    The string will be transmitted to the OM client if the OI
    returns with an error code (not ok). This is allowed inside:
        SaImmOiCcbObjectCreateCallbackT
        SaImmOiCcbObjectDeleteCallbackT
        SaImmOiCcbObjectModifyCallbackT
        SaImmOiCcbCompletedCallbackT
                   
    type arguments:
        SaImmOiHandleT - immOiHandle
        SaImmOiCcbIdT - ccbId
        SaStringT - errorString
        
    returns:
        SaAisErrorT
 
	"""
	oidll.saImmOiCcbSetErrorString.argtypes = [SaImmOiHandleT,
											   SaImmOiCcbItT,
											   SaStringT]
	
	oidll.saImmOiCcbSetErrorString.restype = SaAisErrorT
	
	return oidll.saImmOiCcbSetErrorString(immOiHandle, ccbId, errorString)

