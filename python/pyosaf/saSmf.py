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

# Only mirrors API calls implemented in osaf/libs/agents/saf/smfa/smfa_api.c
# If it ain't implemented here (or is commented out here),
# it probably ain't implemented there!

smfdll = CDLL('libSaSmf.so.0')

SaSmfHandleT = SaUint64T
SaSmfCallbackScopeIdT = SaUint32T

SaSmfPhaseT = SaEnumT
eSaSmfPhaseT = Enumeration((
	('SA_SMF_UPGRADE', 1),
	('SA_SMF_ROLLBACK', 2),
))

SaSmfUpgrMethodT = SaEnumT
eSaSmfUpgrMethodT = Enumeration((
	('SA_SMF_ROLLING', 1),
	('SA_SMF_SINGLE_STEP', 2),
))

SaSmfCmpgStateT = SaEnumT
eSaSmfCmpgStateT = Enumeration((
	('SA_SMF_CMPG_INITIAL', 1),
	('SA_SMF_CMPG_EXECUTING', 2),
	('SA_SMF_CMPG_SUSPENDING_EXECUTION', 3),
	('SA_SMF_CMPG_EXECUTION_SUSPENDED', 4),
	('SA_SMF_CMPG_EXECUTION_COMPLETED', 5),
	('SA_SMF_CMPG_CAMPAIGN_COMMITTED', 6),
	('SA_SMF_CMPG_ERROR_DETECTED', 7),
	('SA_SMF_CMPG_SUSPENDED_BY_ERROR_DETECTED', 8),
	('SA_SMF_CMPG_ERROR_DETECTED_IN_SUSPENDING', 9),
	('SA_SMF_CMPG_EXECUTION_FAILED', 10),
	('SA_SMF_CMPG_ROLLING_BACK', 11),
	('SA_SMF_CMPG_SUSPENDING_ROLLBACK', 12),
	('SA_SMF_CMPG_ROLLBACK_SUSPENDED', 13),
	('SA_SMF_CMPG_ROLLBACK_COMPLETED', 14),
	('SA_SMF_CMPG_ROLLBACK_COMMITTED', 15),
	('SA_SMF_CMPG_ROLLBACK_FAILED', 16),
))

SaSmfProcStateT = SaEnumT
eSaSmfProcStateT = Enumeration((
	('SA_SMF_PROC_INITIAL', 1),
	('SA_SMF_PROC_EXECUTING', 2),
	('SA_SMF_PROC_SUSPENDED', 3),
	('SA_SMF_PROC_COMPLETED', 4),
	('SA_SMF_PROC_STEP_UNDONE', 5),
	('SA_SMF_PROC_FAILED', 6),
	('SA_SMF_PROC_ROLLING_BACK', 7),
	('SA_SMF_PROC_ROLLBACK_SUSPENDED', 8),
	('SA_SMF_PROC_ROLLED_BACK', 9),
	('SA_SMF_PROC_ROLLBACK_FAILED', 10),
))

SaSmfStepStateT = SaEnumT
eSaSmfStepStateT = Enumeration((
	('SA_SMF_STEP_INITIAL', 1),
	('SA_SMF_STEP_EXECUTING', 2),
	('SA_SMF_STEP_UNDOING', 3),
	('SA_SMF_STEP_COMPLETED', 4),
	('SA_SMF_STEP_UNDONE', 5),
	('SA_SMF_STEP_FAILED', 6),
	('SA_SMF_STEP_ROLLING_BACK', 7),
	('SA_SMF_STEP_UNDOING_ROLLBACK', 8),
	('SA_SMF_STEP_ROLLED_BACK', 9),
	('SA_SMF_STEP_ROLLBACK_UNDONE', 10),
	('SA_SMF_STEP_ROLLBACK_FAILED', 11),
))

SaSmfOfflineCommandScopeT = SaEnumT
eSaSmfOfflineCommandScopeT = Enumeration((
	('SA_SMF_CMD_SCOPE_AMF_SU', 1),
	('SA_SMF_CMD_SCOPE_AMF_NODE', 2),
	('SA_SMF_CMD_SCOPE_CLM_NODE', 3),
	('SA_SMF_CMD_SCOPE_PLM_EE', 4),
	('SA_SMF_CMD_SCOPE_PLM_HE', 5),
))

SaSmfStateT = SaEnumT
eSaSmfStateT = Enumeration((
	('SA_SMF_CAMPAIGN_STATE', 1),
	('SA_SMF_PROCEDURE_STATE', 2),
	('SA_SMF_STEP_STATE', 3),
))

SaSmfEntityInfoT = SaEnumT
eSaSmfEntityInfoT = Enumeration((
	('SA_SMF_ENTITY_NAME', 1),
))

SaSmfLabelFilterTypeT = SaEnumT
eSaSmfLabelFilterTypeT = Enumeration((
	('SA_SMF_PREFIX_FILTER', 1),
	('SA_SMF_SUFFIX_FILTER', 2),
	('SA_SMF_EXACT_FILTER', 3),
	('SA_SMF_PASS_ALL_FILTER', 4),
))

class SaSmfCallbackLabelT(Structure):
	"""Contain label identifying the stage of the campaign.
	"""
	_fields_ = [('labelSize', SaSizeT),
		('label', POINTER(SaUint8T))]

class SaSmfLabelFilterT(Structure):
	"""Contain filter information applied to a callback label.
	"""
	_fields_ = [('filterType', SaSmfLabelFilterTypeT),
		('filter', SaSmfCallbackLabelT)]

class SaSmfLabelFilterArrayT(Structure):
	"""Contain one or more filters.
	"""
	_fields_ = [('filtersNumber', SaSizeT),
		('filters', POINTER(SaSmfLabelFilterT))]

SaSmfCampaignCallbackT = CFUNCTYPE(None,
	SaSmfHandleT, SaInvocationT, SaSmfCallbackScopeIdT, POINTER(SaNameT),
		SaSmfPhaseT, POINTER(SaSmfCallbackLabelT), SaStringT)

class SaSmfCallbacksT(Structure):
	"""Contain various callbacks SMF may invoke on registrant.
	"""
	_fields_ = [('saSmfCampaignCallback', SaSmfCampaignCallbackT)]

SaSmfAdminOperationIdT = SaEnumT
eSaSmfAdminOperationIdT = Enumeration((
	('SA_SMF_ADMIN_EXECUTE', 1),
	('SA_SMF_ADMIN_ROLLBACK', 2),
	('SA_SMF_ADMIN_SUSPEND', 3),
	('SA_SMF_ADMIN_COMMIT', 4),
))

def saSmfInitialize(smfHandle, smfCallbacks, version):
	"""Register invoking process with SMF.

	type arguments:
		SaSmfHandleT smfHandle
		SaSmfCallbacksT smfCallbacks
		SaVersionT version

	returns:
		SaAisErrorT

	"""
	return smfdll.saSmfInitialize(BYREF(smfHandle),
		BYREF(smfCallbacks),
		BYREF(version))

def saSmfSelectionObjectGet(smfHandle, selectionObject):
	"""Return operating system handle associated with SMF handle to detect
	pending callbacks.

	type arguments:
		SaSmfHandleT smfHandle
		SaSelectionObjectT selectionObject

	returns:
		SaAisErrorT

	"""
	return smfdll.saSmfSelectionObjectGet(smfHandle,
		BYREF(selectionObject))

def saSmfDispatch(smfHandle, dispatchFlags):
	"""Invoke callbacks pending for the SMF handle.

	type arguments:
		SaSmfHandleT smfHandle
		SaDispatchFlagsT dispatchFlags

	returns:
		SaAisErrorT

	"""
	return smfdll.saSmfDispatch(smfHandle, dispatchFlags)

def saSmfFinalize(smfHandle):
	"""Close association between SMF and the handle.

	type arguments:
		SaSmfHandleT smfHandle

	returns:
		SaAisErrorT

	"""
	return smfdll.saSmfFinalize(smfHandle)

def saSmfCallbackScopeRegister(smfHandle, scopeId, scopeOfInterest):
	"""Register the process's scope of interest with SMF.

	type arguments:
		SaSmfHandleT smfHandle
		SaSmfCallbackScopeIdT scopeId
		SaSmfLabelFilterArrayT scopeOfInterest

	returns:
		SaAisErrorT

	"""
	return smfdll.saSmfCallbackScopeRegister(smfHandle,
		scopeId,
		BYREF(scopeOfInterest))

def saSmfCallbackScopeUnregister(smfHandle, scopeId):
	"""Unregister a previously registered scope of interest.

	type arguments:
		SaSmfHandleT smfHandle
		SaSmfCallbackScopeIdT scopeId

	returns:
		SaAisErrorT

	"""
	return smfdll.saSmfCallbackScopeUnregister(smfHandle, scopeId)

def saSmfResponse(smfHandle, invocation, error):
	"""Respond to SMF with result of callback invoked by SMF.

	type arguments:
		SaSmfHandleT smfHandle
		SaInvocationT invocation
		SaAisErrorT error

	returns:
		SaAisErrorT

	"""
	return smfdll.saSmfResponse(smfHandle, invocation, error)
