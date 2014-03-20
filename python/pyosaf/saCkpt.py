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

ckptdll = CDLL('libSaCkpt.so.1')

SaCkptHandleT = SaUint64T  
SaCkptCheckpointHandleT = SaUint64T
SaCkptSectionIterationHandleT = SaUint64T

saCkpt = Const()

saCkpt.SA_CKPT_WR_ALL_REPLICAS = 0X1
saCkpt.SA_CKPT_WR_ACTIVE_REPLICA = 0X2
saCkpt.SA_CKPT_WR_ACTIVE_REPLICA_WEAK = 0X4
saCkpt.SA_CKPT_CHECKPOINT_COLLOCATED = 0X8

SaCkptCheckpointCreationFlagsT = SaUint32T

class SaCkptCheckpointCreationAttributesT(Structure):
	"""Contain checkpoint creation attributes.
	"""
	_fields_ = [('creationFlags', SaCkptCheckpointCreationFlagsT),
		('checkpointSize', SaSizeT),
		('retentionDuration', SaTimeT),
		('maxSections', SaUint32T),
		('maxSectionSize', SaSizeT),
		('maxSectionIdSize', SaSizeT)]

saCkpt.SA_CKPT_CHECKPOINT_READ = 0X1
saCkpt.SA_CKPT_CHECKPOINT_WRITE = 0X2
saCkpt.SA_CKPT_CHECKPOINT_CREATE = 0X4

SaCkptCheckpointOpenFlagsT = SaUint32T

class SaCkptSectionIdT(Structure):
	"""Contain section identifier descriptor.
	"""
	_fields_ = [('idLen', SaUint16T),
		('id', POINTER(SaUint8T))]

def SA_CKPT_DEFAULT_SECTION_ID():
	"""Return section identifier descriptor of default section.
	"""
	return SaCkptSectionIdT(0, None)

def SA_CKPT_GENERATED_SECTION_ID():
	"""Return section identifier descriptor of generated section.
	"""
	return SaCkptSectionIdT(0, None)

class SaCkptSectionCreationAttributesT(Structure):
	"""Contain checkpoint section creation attributes.
	"""
	_fields_ = [('sectionId', POINTER(SaCkptSectionIdT)),
		('expirationTime', SaTimeT)]

SaCkptSectionStateT = SaEnumT
eSaCkptSectionStateT = Enumeration((
	('SA_CKPT_SECTION_VALID', 1),
	('SA_CKPT_SECTION_CORRUPTED', 2),
))

class SaCkptSectionDescriptorT(Structure):
	"""Contain checkpoint section descriptor.
	"""
	_fields_ = [('sectionId', SaCkptSectionIdT),
		('expirationTime', SaTimeT),
		('sectionSize', SaSizeT),
		('sectionState', SaCkptSectionStateT),
		('lastUpdate', SaTimeT)]

SaCkptSectionsChosenT = SaEnumT
eSaCkptSectionsChosenT = Enumeration((
	('SA_CKPT_SECTIONS_FOREVER', 1),
	('SA_CKPT_SECTIONS_LEQ_EXPIRATION_TIME', 2),
	('SA_CKPT_SECTIONS_GEQ_EXPIRATION_TIME', 3),
	('SA_CKPT_SECTIONS_CORRUPTED', 4),
	('SA_CKPT_SECTIONS_ANY', 5),
))

class SaCkptIOVectorElementT(Structure):
	"""Contain IO vector element.
	"""
	_fields_ = [('sectionId', SaCkptSectionIdT),
		('dataBuffer', SaVoidPtr),
		('dataSize', SaSizeT),
		('dataOffset', SaOffsetT),
		('readSize', SaSizeT)]

class SaCkptCheckpointDescriptorT(Structure):
	"""Contain checkpoint descriptor.
	"""
	_fields_ = [('checkpointCreationAttributes',
			SaCkptCheckpointCreationAttributesT),
		('numberOfSections', SaUint32T),
		('memoryUsed', SaSizeT)]

SaCkptCheckpointStatusT = SaEnumT
eSaCkptCheckpointStatusT = Enumeration((
	('SA_CKPT_SECTION_FULL', 1),
	('SA_CKPT_SECTION_AVAILABLE', 2),
))

SaCkptStateT = SaEnumT
eSaCkptStateT = Enumeration((
	('SA_CKPT_CHECKPOINT_STATUS', 1),
))

SaCkptCheckpointOpenCallbackT = CFUNCTYPE(None,
		SaInvocationT, SaCkptCheckpointHandleT, SaAisErrorT)

SaCkptCheckpointSynchronizeCallbackT = CFUNCTYPE(None,
		SaInvocationT, SaAisErrorT)

SaCkptCheckpointTrackCallbackT = CFUNCTYPE(None,
                SaCkptCheckpointHandleT, POINTER(SaCkptIOVectorElementT), SaUint32T)


class SaCkptCallbacksT(Structure):
	"""Contain various callbacks Checkpoint Service may invoke on process.
	"""
	_fields_ = [('saCkptCheckpointOpenCallback',
			SaCkptCheckpointOpenCallbackT),
		('saCkptCheckpointSynchronizeCallback',
			SaCkptCheckpointSynchronizeCallbackT)]

class SaCkptCallbacksT_2(Structure):
	"""Contain various callbacks Checkpoint Service may invoke on process.
	"""
	_fields_ = [('saCkptCheckpointOpenCallback',
			SaCkptCheckpointOpenCallbackT),
		('saCkptCheckpointSynchronizeCallback',
			SaCkptCheckpointSynchronizeCallbackT),
                ('saCkptCheckpointTrackCallback',
			SaCkptCheckpointTrackCallbackT)]


def saCkptInitialize(ckptHandle, callbacks, version):
	"""Register invoking process with the Checkpoint Service.

	type arguments:
		SaCkptHandleT ckptHandle
		SaCkptCallbacksT callbacks
		SaVersionT version

	returns:
		SaAisErrorT

	"""
	ckptdll.saCkptInitialize.argtypes = [
								POINTER(SaCkptHandleT),
								POINTER(SaCkptCallbacksT),
	                        	POINTER(SaVersionT)
	                            ]

	ckptdll.saCkptInitialize.restype = SaAisErrorT

	return ckptdll.saCkptInitialize(BYREF(ckptHandle),
			BYREF(callbacks),
			BYREF(version))

def saCkptInitialize_2(ckptHandle, callbacks, version):
	"""Register invoking process with the Checkpoint Service.

	type arguments:
		SaCkptHandleT ckptHandle
		SaCkptCallbacksT_2 callbacks
		SaVersionT version

	returns:
		SaAisErrorT

	"""
	ckptdll.saCkptInitialize_2.argtypes = [
						POINTER(SaCkptHandleT),
						POINTER(SaCkptCallbacksT_2),
                                                POINTER(SaVersionT)
	                            ]

	ckptdll.saCkptInitialize_2.restype = SaAisErrorT

	return ckptdll.saCkptInitialize_2(BYREF(ckptHandle),
			BYREF(callbacks),
			BYREF(version))

def saCkptSelectionObjectGet(ckptHandle, selectionObject):
	"""Return the operating system handle associated with ckptHandle
	to detect pending callbacks.

	type arguments:
		SaCkptHandleT ckptHandle
		SaSelectionObjectT selectionObject

	returns:
		SaAisErrorT

	"""
	ckptdll.saCkptSelectionObjectGet.argtypes = [
	                            SaCkptHandleT,
	                            POINTER(SaSelectionObjectT)
	                            ]

	ckptdll.saCkptSelectionObjectGet.restype = SaAisErrorT

	return ckptdll.saCkptSelectionObjectGet(ckptHandle,
			BYREF(selectionObject))

def saCkptDispatch(ckptHandle, dispatchFlags):
	"""Invoke callbacks pending for ckptHandle.

	type arguments:
		SaCkptHandleT ckptHandle
		SaDispatchFlagsT dispatchFlags

	returns:
		SaAisErrorT

	"""
	ckptdll.saCkptDispatch.argtypes = [
								SaCkptHandleT,
	                            SaDispatchFlagsT
	                            ]

	ckptdll.saCkptDispatch.restype = SaAisErrorT

	return ckptdll.saCkptDispatch(ckptHandle, dispatchFlags)

def saCkptFinalize(ckptHandle):
	"""Close association between Checkpoint Service and ckptHandle.

	type arguments:
		SaCkptHandleT ckptHandle

	returns:
		SaAisErrorT

	"""
	ckptdll.saCkptFinalize.argtypes = [
	                            SaCkptHandleT
	                            ]

	ckptdll.saCkptFinalize.restype = SaAisErrorT

	return ckptdll.saCkptFinalize(ckptHandle)

def saCkptCheckpointOpen(ckptHandle,
		checkpointName,
		checkpointCreationAttributes,
		checkpointOpenFlags,
		timeout,
		checkpointHandle):
	"""Open existing checkpoint or create a new one.

	type arguments:
		SaCkptHandleT ckptHandle
		SaNameT checkpointName
		SaCkptCheckpointCreationAttributesT checkpointCreationAttributes
		SaCkptCheckpointOpenFlagsT checkpointOpenFlags
		SaTimeT timeout
		SaCkptCheckpointHandleT checkpointHandle

	returns:
		SaAisErrorT

	"""
	ckptdll.saCkptCheckpointOpen.argtypes = [
	                            SaCkptHandleT,
	                            POINTER(SaNameT),
	                            POINTER(SaCkptCheckpointCreationAttributesT),
	                            SaCkptCheckpointOpenFlagsT,
	                            SaTimeT,
	                            POINTER(SaCkptCheckpointHandleT)
	                            ]

	ckptdll.saCkptCheckpointOpen.restype = SaAisErrorT

	return ckptdll.saCkptCheckpointOpen(ckptHandle,
			BYREF(checkpointName),
			BYREF(checkpointCreationAttributes),
			checkpointOpenFlags,
			timeout,
			BYREF(checkpointHandle))

def saCkptCheckpointOpenAsync(ckptHandle,
		invocation,
		checkpointName,
		checkpointCreationAttributes,
		checkpointOpenFlags):
	"""Open existing checkpoint or create a new one without blocking.

	type arguments:
		SaCkptHandleT ckptHandle
		SaInvocationT invocation
		SaNameT checkpointName
		SaCkptCheckpointCreationAttributesT checkpointCreationAttributes
		SaCkptCheckpointOpenFlagsT checkpointOpenFlags

	returns:
		SaAisErrorT

	"""
	ckptdll.saCkptCheckpointOpenAsync.argtypes = [
								SaCkptHandleT,
	                            SaInvocationT,
	                            POINTER(SaNameT),
	                            POINTER(SaCkptCheckpointCreationAttributesT),
	                            SaCkptCheckpointOpenFlagsT
	                            ]

	ckptdll.saCkptCheckpointOpenAsync.restype = SaAisErrorT

	return ckptdll.saCkptCheckpointOpenAsync(ckptHandle,
			invocation,
			BYREF(checkpointName),
			BYREF(checkpointCreationAttributes),
			checkpointOpenFlags)

def saCkptCheckpointClose(checkpointHandle):
	"""Close checkpoint associated with checkpointHandle.

	type arguments:
		SaCkptCheckpointHandleT checkpointHandle

	returns:
		SaAisErrorT

	"""
	ckptdll.saCkptCheckpointClose.argtypes = [
	                            SaCkptCheckpointHandleT
	                            ]

	ckptdll.saCkptCheckpointClose.restype = SaAisErrorT

	return ckptdll.saCkptCheckpointClose(checkpointHandle)

def saCkptCheckpointUnlink(ckptHandle, checkpointName):
	"""Delete named checkpoint from the cluster.

	type arguments:
		SaCkptHandleT ckptHandle
		SaNameT checkpointName

	returns:
		SaAisErrorT

	"""
	ckptdll.saCkptCheckpointUnlink.argtypes = [
								SaCkptHandleT,
	                            POINTER(SaNameT)
	                            ]

	ckptdll.saCkptCheckpointUnlink.restype = SaAisErrorT

	return ckptdll.saCkptCheckpointUnlink(ckptHandle,
			BYREF(checkpointName))

def saCkptCheckpointRetentionDurationSet(checkpointHandle, retentionDuration):
	"""Set retention duration of checkpoint associated with checkpointHandle.

	type arguments:
		SaCkptCheckpointHandleT checkpointHandle
		SaTimeT retentionDuration

	returns:
		SaAisErrorT

	"""
	ckptdll.saCkptCheckpointRetentionDurationSet.argtypes = [
	                            SaCkptCheckpointHandleT,
	                            SaTimeT
	                            ]

	ckptdll.saCkptCheckpointRetentionDurationSet.restype = SaAisErrorT

	return ckptdll.saCkptCheckpointRetentionDurationSet(checkpointHandle,
			retentionDuration)

def saCkptActiveReplicaSet(checkpointHandle):
	"""Set local checkpoint replica to be active.

	type arguments:
		SaCkptCheckpointHandleT checkpointHandle

	returns:
		SaAisErrorT

	"""
	ckptdll.saCkptActiveReplicaSet.argtypes = [
	                            SaCkptCheckpointHandleT
	                            ]

	ckptdll.saCkptActiveReplicaSet.restype = SaAisErrorT

	return ckptdll.saCkptActiveReplicaSet(checkpointHandle)

def saCkptCheckpointStatusGet(checkpointHandle, checkpointStatus):
	"""Get status of checkpoint associated with checkpointHandle.

	type arguments:
		SaCkptCheckpointHandleT checkpointHandle
		SaCkptCheckpointDescriptorT checkpointStatus

	returns:
		SaAisErrorT

	"""
	ckptdll.saCkptCheckpointStatusGet.argtypes = [
	                            SaCkptCheckpointHandleT,
	                            POINTER(SaCkptCheckpointDescriptorT)
	                            ]

	ckptdll.saCkptCheckpointStatusGet.restype = SaAisErrorT

	return ckptdll.saCkptCheckpointStatusGet(checkpointHandle,
			BYREF(checkpointStatus))

def saCkptSectionCreate(checkpointHandle,
		sectionCreationAttributes,
		initialData,
		initialDataSize):
	"""Create new section in checkpoint associated with checkpointHandle.

	type arguments:
		SaCkptCheckpointHandleT checkpointHandle
		SaCkptSectionCreationAttributesT sectionCreationAttributes
		void initialData
		SaSizeT initialDataSize

	returns:
		SaAisErrorT

	"""
	ckptdll.saCkptSectionCreate.argtypes = [
	                            SaCkptCheckpointHandleT,
	                            POINTER(SaCkptSectionCreationAttributesT),
	                            SaVoidPtr,
                                    SaSizeT
	                            ]

	ckptdll.saCkptSectionCreate.restype = SaAisErrorT

	return ckptdll.saCkptSectionCreate(checkpointHandle,
			BYREF(sectionCreationAttributes),
			initialData,
			initialDataSize)

def saCkptSectionDelete(checkpointHandle, sectionId):
	"""Delete identified section within checkpoint associated with
	checkpointHandle.

	type arguments:
		SaCkptCheckpointHandleT checkpointHandle
		SaCkptSectionIdT sectionId

	returns:
		SaAisErrorT

	"""
	ckptdll.saCkptSectionDelete.argtypes = [
	                            SaCkptCheckpointHandleT,
	                            POINTER(SaCkptSectionIdT)
	                            ]

	ckptdll.saCkptSectionDelete.restype = SaAisErrorT

	return ckptdll.saCkptSectionDelete(checkpointHandle,
			BYREF(sectionId))

def saCkptSectionExpirationTimeSet(checkpointHandle,
		sectionId,
		expirationTime):
	"""Set expiration of identified section within checkpoint identified
	by checkpointHandle.

	type arguments:
		SaCkptCheckpointHandleT checkpointHandle
		SaCkptSectionIdT sectionId
		SaTimeT expirationTime

	returns:
		SaAisErrorT

	"""
	ckptdll.saCkptSectionExpirationTimeSet.argtypes = [
	                            SaCkptCheckpointHandleT,
	                            POINTER(SaCkptSectionIdT),
	                            SaTimeT
	                            ]

	ckptdll.saCkptSectionExpirationTimeSet.restype = SaAisErrorT

	return ckptdll.saCkptSectionExpirationTimeSet(checkpointHandle,
			BYREF(sectionId),
			expirationTime)

def saCkptSectionIterationInitialize(checkpointHandle,
		sectionsChosen,
		expirationTime,
		sectionIterationHandle):
	"""Get iteration handle for checkpoint associated with checkpointHandle.

	type arguments:
		SaCkptCheckpointHandleT checkpointHandle
		SaCkptSectionsChosenT sectionsChosen
		SaTimeT expirationTime
		SaCkptSectionIterationHandleT sectionIterationHandle

	returns:
		SaAisErrorT

	"""
	ckptdll.saCkptSectionIterationInitialize.argtypes = [
	                            SaCkptCheckpointHandleT,
	                            SaCkptSectionsChosenT,
	                            SaTimeT,
	                            POINTER(SaCkptSectionIterationHandleT)
	                            ]

	ckptdll.saCkptSectionIterationInitialize.restype = SaAisErrorT

	return ckptdll.saCkptSectionIterationInitialize(checkpointHandle,
			sectionsChosen,
			expirationTime,
			BYREF(sectionIterationHandle))

def saCkptSectionIterationNext(sectionIterationHandle, sectionDescriptor):
	"""Iterate over table of sections using iterator obtained via
	saCkptSectionIterationInitialize().

	type arguments:
		SaCkptSectionIterationHandleT sectionIterationHandle
		SaCkptSectionDescriptorT sectionDescriptor

	returns:
		SaAisErrorT

	"""
	ckptdll.saCkptSectionIterationNext.argtypes = [
	                            SaCkptSectionIterationHandleT,
	                            POINTER(SaCkptSectionDescriptorT)
	                            ]

	ckptdll.saCkptSectionIterationNext.restype = SaAisErrorT

	return ckptdll.saCkptSectionIterationNext(sectionIterationHandle,
			BYREF(sectionDescriptor))
 
def saCkptSectionIterationFinalize(sectionIterationHandle):
	"""Free resources allocated for iteration.

	type arguments:
		SaCkptSectionIterationHandleT sectionIterationHandle

	returns:
		SaAisErrorT

	"""
	ckptdll.saCkptSectionIterationFinalize.argtypes = [
	                            SaCkptSectionIterationHandleT
	                            ]

	ckptdll.saCkptSectionIterationFinalize.restype = SaAisErrorT

	return ckptdll.saCkptSectionIterationFinalize(sectionIterationHandle)

def createSaCkptIOVector(size):
	"""Create sized array of SaCkptIOVectorElementT.
	"""
	return (SaCkptIOVectorElementT*size)() if size else None

def saCkptCheckpointWrite(checkpointHandle,
		ioVector,
		numberOfElements,
		erroneousVectorIndex):
	"""Write data from ioVector into checkpoint associated with
	checkpointHandle.

	type arguments:
		SaCkptCheckpointHandleT checkpointHandle
		SaCkptIOVectorElementT* ioVector
		SaUint32T numberOfElements
		SaUint32T erroneousVectorIndex

	returns:
		SaAisErrorT

	"""
	ckptdll.saCkptCheckpointWrite.argtypes = [
	                            SaCkptCheckpointHandleT,
	                            POINTER(SaCkptIOVectorElementT),
	                            SaUint32T,
	                            POINTER(SaUint32T)
	                            ]

	ckptdll.saCkptCheckpointWrite.restype = SaAisErrorT

	return ckptdll.saCkptCheckpointWrite(checkpointHandle,
			BYREF(ioVector),
			numberOfElements,
			BYREF(erroneousVectorIndex))

def saCkptSectionOverwrite(checkpointHandle,
		sectionId,
		dataBuffer,
		dataSize):
	"""Overwrite identified section in checkpoint associated with
	checkpointHandle.

	type arguments:
		SaCkptCheckpointHandleT checkpointHandle
		SaCkptSectionIdT sectionId
		SaVoidPtr dataBuffer
		SaSizeT dataSize

	returns:
		SaAisErrorT

	"""
	ckptdll.saCkptSectionOverwrite.argtypes = [
	                            SaCkptCheckpointHandleT,
	                            POINTER(SaCkptSectionIdT),
	                            SaVoidPtr,
	                            SaSizeT
	                            ]

	ckptdll.saCkptSectionOverwrite.restype = SaAisErrorT

	return ckptdll.saCkptSectionOverwrite(checkpointHandle,
			BYREF(sectionId),
			dataBuffer,
			dataSize)

def saCkptCheckpointRead(checkpointHandle,
		ioVector,
		numberOfElements,
		erroneousVectorIndex):
	"""Read data into ioVector from checkpoint associated with
	checkpointHandle.

	type arguments:
		SaCkptCheckpointHandleT checkpointHandle
		SaCkptIOVectorElementT* ioVector
		SaUint32T numberOfElements
		SaUint32T erroneousVectorIndex

	returns:
		SaAisErrorT

	"""
	ckptdll.saCkptCheckpointRead.argtypes = [
	                            SaCkptCheckpointHandleT,
	                            POINTER(SaCkptIOVectorElementT),
	                            SaUint32T,
	                            POINTER(SaUint32T)
	                            ]
	ckptdll.saCkptCheckpointRead.restype = SaAisErrorT

	return ckptdll.saCkptCheckpointRead(checkpointHandle,
			BYREF(ioVector),
			numberOfElements,
			BYREF(erroneousVectorIndex))

def saCkptCheckpointSynchronize(checkpointHandle, timeout):
	"""Propagate all operations applied on active checkpoint to other
	checkpoint replicas.

	type arguments:
		SaCkptCheckpointHandleT checkpointHandle
		SaTimeT timeout

	returns:
		SaAisErrorT

	"""
	ckptdll.saCkptCheckpointSynchronize.argtypes = [
	                            SaCkptCheckpointHandleT,
	                            SaTimeT
	                            ]

	ckptdll.saCkptCheckpointSynchronize.restype = SaAisErrorT

	return ckptdll.saCkptCheckpointSynchronize(checkpointHandle, timeout)

def saCkptCheckpointSynchronizeAsync(checkpointHandle, invocation):
	"""Propagate all operations applied on active checkpoint to other
	checkpoint replicas without blocking.

	type arguments:
		SaCkptCheckpointHandleT checkpointHandle
		SaInvocationT invocation

	returns:
		SaAisErrorT

	"""
	ckptdll.saCkptCheckpointSynchronizeAsync.argtypes = [
	                            SaCkptCheckpointHandleT,
	                            SaInvocationT
	                            ]

	ckptdll.saCkptCheckpointSynchronizeAsync.restype = SaAisErrorT

	return ckptdll.saCkptCheckpointSynchronizeAsync(checkpointHandle,
			invocation)

def saCkptSectionIdFree(checkpointHandle, id):
	"""Free memory to which id points, allocated by saCkptSectionCreate().

	type arguments:
		SaCkptCheckpointHandleT checkpointHandle
		POINTER(SaUint8T) id

	returns:
		SaAisErrorT

	"""
	ckptdll.saCkptSectionIdFree.argtypes = [
	                            SaCkptCheckpointHandleT,
	                            POINTER(SaUint8T)
	                            ]

	ckptdll.saCkptSectionIdFree.restype = SaAisErrorT

	return ckptdll.saCkptSectionIdFree(checkpointHandle, 
									BYREF(id))

def saCkptIOVectorElementDataFree(checkpointHandle, data):
	"""Free memory to which data points, allocated by saCkptCheckpointRead().

	type arguments:
		SaCkptCheckpointHandleT checkpointHandle
		SaVoidPtr data

	returns:
		SaAisErrorT

	"""
	ckptdll.saCkptIOVectorElementDataFree.argtypes = [
	                            SaCkptCheckpointHandleT,
	                            SaVoidPtr
	                            ]

	ckptdll.saCkptIOVectorElementDataFree.restype = SaAisErrorT

	return ckptdll.saCkptIOVectorElementDataFree(checkpointHandle, 
                                                    data)

def saCkptTrack(ckptHandle):
	"""enable/starts the Ckpt Track callback.

	type arguments:
		SaCkptHandleT ckptHandle

	returns:
		SaAisErrorT

	"""
	ckptdll.saCkptTrack.argtypes = [
	                            SaCkptHandleT
	                            ]

	ckptdll.saCkptTrack.restype = SaAisErrorT

	return ckptdll.saCkptTrack(ckptHandle)

def saCkptTrackStop(ckptHandle):
	"""disable/stops the Ckpt Track callback.

	type arguments:
		SaCkptHandleT ckptHandle

	returns:
		SaAisErrorT

	"""
	ckptdll.saCkptTrackStop.argtypes = [
	                            SaCkptHandleT
	                            ]

	ckptdll.saCkptTrackStop.restype = SaAisErrorT

	return ckptdll.saCkptTrackStop(ckptHandle)

def saCkptCheckpointTrack(checkpointHandle):
	"""enable/starts the Ckpt Track callback.

	type arguments:
		SaCkptCheckpointHandleT checkpointHandle

	returns:
		SaAisErrorT

	"""
	ckptdll.saCkptCheckpointTrack.argtypes = [
	                            SaCkptCheckpointHandleT
	                            ]

	ckptdll.saCkptCheckpointTrack.restype = SaAisErrorT

	return ckptdll.saCkptCheckpointTrack(checkpointHandle)

def saCkptCheckpointTrackStop(checkpointHandle):
	"""disable/stops the Ckpt Track callback.

	type arguments:
		SaCkptCheckpointHandleT checkpointHandle

	returns:
		SaAisErrorT

	"""
	ckptdll.saCkptCheckpointTrackStop.argtypes = [
	                            SaCkptCheckpointHandleT
	                            ]

	ckptdll.saCkptCheckpointTrackStop.restype = SaAisErrorT

	return ckptdll.saCkptCheckpointTrackStop(checkpointHandle)
