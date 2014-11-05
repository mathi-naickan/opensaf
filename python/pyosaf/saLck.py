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

lckdll = CDLL('libSaLck.so.0')

SaLckHandleT = SaUint64T
SaLckLockIdT = SaUint64T
SaLckResourceHandleT = SaUint64T
SaLckWaiterSignalT = SaUint64T

saLck = Const()

saLck.SA_LCK_RESOURCE_CREATE = 0x1
SaLckResourceOpenFlagsT = SaUint32T

saLck.SA_LCK_LOCK_NO_QUEUE = 0x1
saLck.SA_LCK_LOCK_ORPHAN = 0x2
SaLckLockFlagsT = SaUint32T

saLck.SA_LCK_OPT_ORPHAN_LOCKS = 0x1
saLck.SA_LCK_OPT_DEADLOCK_DETECTION = 0x2
SaLckOptionsT = SaUint32T

SaLckLockStatusT = SaEnumT
eSaLckLockStatusT = Enumeration((
	('SA_LCK_LOCK_GRANTED', 1),
	('SA_LCK_LOCK_DEADLOCK', 2),
	('SA_LCK_LOCK_NOT_QUEUED', 3),
	('SA_LCK_LOCK_ORPHANED', 4),
	('SA_LCK_LOCK_NO_MORE', 5),
	('SA_LCK_LOCK_DUPLICATE_EX', 6),
))

SaLckLockModeT = SaEnumT
eSaLckLockModeT = Enumeration((
	('SA_LCK_PR_LOCK_MODE', 1),
	('SA_LCK_EX_LOCK_MODE', 2),
))

SaLckResourceOpenCallbackT = CFUNCTYPE(None,
		SaInvocationT, SaLckResourceHandleT, SaAisErrorT)

SaLckLockGrantCallbackT = CFUNCTYPE(None,
		SaInvocationT, SaLckLockStatusT, SaAisErrorT)

SaLckLockWaiterCallbackT = CFUNCTYPE(None,
		SaLckWaiterSignalT, SaLckLockIdT, SaLckLockModeT,
		SaLckLockModeT)

SaLckResourceUnlockCallbackT = CFUNCTYPE(None,
		SaInvocationT, SaAisErrorT)

class SaLckCallbacksT(Structure):
	"""Contain various callbacks Lock Service may invoke on the registrant.
	"""
	_fields_ = [('saLckResourceOpenCallback', SaLckResourceOpenCallbackT),
		('saLckLockGrantCallback', SaLckLockGrantCallbackT),
		('saLckLockWaiterCallback', SaLckLockWaiterCallbackT),
		('saLckResourceUnlockCallback',SaLckResourceUnlockCallbackT )]

def saLckInitialize(lckHandle, lckCallbacks, version):
	"""Register invoking process with Lock Service.

	type arguments:
		SaLckHandleT lckHandle
		SaLckCallbacksT lckCallbacks
		SaVersionT version

	returns:
		SaAisErrorT

	"""
	return lckdll.saLckInitialize(BYREF(lckHandle),
			BYREF(lckCallbacks),
			BYREF(version))

def saLckSelectionObjectGet(lckHandle, selectionObject):
	"""Return operating system handle associated with LCK handle to detect
	pending callbacks.

	type arguments:
		SaLckHandleT lckHandle
		SaSelectionObjectT selectionObject

	returns:
		SaAisErrorT

	"""
	return lckdll.saLckSelectionObjectGet(lckHandle,
			BYREF(selectionObject))

def saLckOptionCheck(lckHandle, lckOptions):
	"""Determine if optional Lock Service features are provided.

	type arguments:
		SaLckHandleT lckHandle
		SaLckOptionsT lckOptions

	returns:
		SaAisErrorT

	"""
	return lckdll.saLckOptionCheck(lckHandle, BYREF(lckOptions))

def saLckDispatch(lckHandle, dispatchFlags):
	"""Invoke callbacks pending for the LCK handle.

	type arguments:
		SaLckHandleT lckHandle
		SaDispatchFlagsT dispatchFlags

	returns:
		SaAisErrorT

	"""
	return lckdll.saLckDispatch(lckHandle, dispatchFlags)

def saLckFinalize(lckHandle):
	"""Close association between Lock Service and the handle.

	type arguments:
		SaLckHandleT lckHandle

	returns:
		SaAisErrorT

	"""
	return lckdll.saLckFinalize(lckHandle)

def saLckResourceOpen(lckHandle,
		lockResourceName, resourceFlags, timeout, lockResourceHandle):
	"""Open a named, cluster-wide lock resource for locking operations.

	type arguments:
		SaLckHandleT lckHandle
		SaNameT lockResourceName
		SaLckResourceOpenFlagsT resourceFlags
		SaTimeT timeout
		SaLckResourceHandleT lockResourceHandle

	returns:
		SaAisErrorT

	"""
	return lckdll.saLckResourceOpen(lckHandle,
			BYREF(lockResourceName),
			resourceFlags,
			timeout,
			BYREF(lockResourceHandle))

def saLckResourceOpenAsync(lckHandle,
		invocation, lockResourceName, resourceFlags):
	"""Open a named, cluster-wide lock resource asynchronously for locking
	operations.

	type arguments:
		SaLckHandleT lckHandle
		SaInvocationT invocation
		SaNameT lockResourceName
		SaLckResourceOpenFlagsT resourceFlags

	returns:
		SaAisErrorT

	"""
	return lckdll.saLckResourceOpenAsync(lckHandle,
			invocation,
			BYREF(lockResourceName),
			resourceFlags)

def saLckResourceClose(lockResourceHandle):
	"""Delete the association between the handle and the corresponding lock
	resource.

	type arguments:
		SaLckResourceHandleT lockResourceHandle

	returns:
		SaAisErrorT

	"""
	return lckdll.saLckResourceClose(lockResourceHandle)

def saLckResourceLock(lockResourceHandle,
		lockId, lockMode, lockFlags, waiterSignal, timeout, lockStatus):
	"""Aquire a lock on a lock resource.

	type arguments:
		SaLckResourceHandleT lockResourceHandle
		SaLckLockIdT lockId
		SaLckLockModeT lockMode
		SaLckLockFlagsT lockFlags
		SaLckWaiterSignalT waiterSignal
		SaTimeT timeout
		SaLckLockStatusT lockStatus

	returns:
		SaAisErrorT

	"""
	return lckdll.saLckResourceLock(lockResourceHandle,
			BYREF(lockId),
			lockMode,
			lockFlags,
			waiterSignal,
			timeout,
			BYREF(lockStatus))

def saLckResourceLockAsync(lockResourceHandle,
		invocation, lockId, lockMode, lockFlags, waiterSignal):
	"""Aquire a lock on a lock resource asynchronously.

	type arguments:
		SaLckResourceHandleT lockResourceHandle
		SaInvocationT invocation
		SaLckLockIdT lockId
		SaLckLockModeT lockMode
		SaLckLockFlagsT lockFlags
		SaLckWaiterSignalT waiterSignal

	returns:
		SaAisErrorT

	"""
	return lckdll.saLckResourceLockAsync(lockResourceHandle,
			invocation,
			BYREF(lockId),
			lockMode,
			lockFlags,
			waiterSignal)

def saLckResourceUnlock(lockId, timeout):
	"""Release identified lock.

	type arguments:
		SaLckLockIdT lockId
		SaTimeT timeout

	returns:
		SaAisErrorT

	"""
	return lckdll.saLckResourceUnlock(lockId, timeout)

def saLckResourceUnlockAsync(invocation, lockId):
	"""Release identified lock asynchronously.

	type arguments:
		SaInvocationT invocation
		SaLckLockIdT lockId

	returns:
		SaAisErrorT

	"""
	return lckdll.saLckResourceUnlockAsync(invocation, lockId)

def saLckLockPurge(lockResourceHandle):
	"""Purge existing orphan locks held on the lock resource. Orphaned locks
	are locks aquired with SA_LCK_LOCK_ORPHAN flag set and have not been
	unlocked properly by the owner process before finalization, release of
	associated resource, or exit.

	type arguments:
		SaLckResourceHandleT lockResourceHandle

	returns:
		SaAisErrorT

	"""
	return lckdll.saLckLockPurge(lockResourceHandle)
