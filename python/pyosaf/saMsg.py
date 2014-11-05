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

msgdll = CDLL('libSaMsg.so.1')

SaMsgHandleT = SaUint64T
SaMsgQueueHandleT = SaUint64T
SaMsgSenderIdT = SaUint64T

saMsg = Const()

saMsg.SA_MSG_MESSAGE_DELIVERED_ACK = 0x1
SaMsgAckFlagsT = SaUint32T

saMsg.SA_MSG_QUEUE_PERSISTENT = 0x1
SaMsgQueueCreationFlagsT = SaUint32T

saMsg.SA_MSG_MESSAGE_HIGHEST_PRIORITY = 0
saMsg.SA_MSG_MESSAGE_LOWEST_PRIORITY = 3

class SaMsgQueueCreationAttributesT(Structure):
	"""Contain message queue creation information required when creating a
	new message queue.
	"""
	_fields_ = [('creationFlags', SaMsgQueueCreationFlagsT),
		('size', (SaSizeT*(saMsg.SA_MSG_MESSAGE_LOWEST_PRIORITY+1))),
		('retentionTime', SaTimeT)]

saMsg.SA_MSG_QUEUE_CREATE = 0x1
saMsg.SA_MSG_QUEUE_RECEIVE_CALLBACK = 0x2
saMsg.SA_MSG_QUEUE_EMPTY = 0x4
SaMsgQueueOpenFlagsT = SaUint32T

class SaMsgQueueUsageT(Structure):
	"""Contain information valid for a given priority area of a message
	queue.
	"""
	_fields_ = [('queueSize', SaSizeT),
		('queueUsed', SaSizeT),
		('numberOfMessages', SaUint32T)]

class SaMsgQueueStatusT(Structure):
	"""Contain status information valid for a message queue.
	"""
	_fields_ = [('creationFlags', SaMsgQueueCreationFlagsT),
		('retentionTime', SaTimeT),
		('closeTime', SaTimeT),
		('saMsgQueueUsage', (SaMsgQueueUsageT*
			(saMsg.SA_MSG_MESSAGE_LOWEST_PRIORITY+1)))]

SaMsgQueueGroupPolicyT = SaEnumT
eSaMsgQueueGroupPolicyT = Enumeration((
	('SA_MSG_QUEUE_GROUP_ROUND_ROBIN', 1),
	('SA_MSG_QUEUE_GROUP_LOCAL_ROUND_ROBIN', 2),
	('SA_MSG_QUEUE_GROUP_LOCAL_BEST_QUEUE', 3),
	('SA_MSG_QUEUE_GROUP_BROADCAST', 4),
))

SaMsgQueueGroupChangesT = SaEnumT
eSaMsgQueueGroupChangesT = Enumeration((
	('SA_MSG_QUEUE_GROUP_NO_CHANGE', 1),
	('SA_MSG_QUEUE_GROUP_ADDED', 2),
	('SA_MSG_QUEUE_GROUP_REMOVED', 3),
	('SA_MSG_QUEUE_GROUP_STATE_CHANGED', 4),
))

class SaMsgQueueGroupMemberT(Structure):
	"""Contain required information for queue membership in a queue group.
	"""
	_fields_ = [('queueName', SaNameT)]

class SaMsgQueueGroupNotificationT(Structure):
	"""Contain information associated with queue group notification.
	"""
	_fields_ = [('member', SaMsgQueueGroupMemberT),
		('change', SaMsgQueueGroupChangesT)]

class SaMsgQueueGroupNotificationBufferT(Structure):
	"""Contain array of queue group notifications.
	"""
	_fields_ = [('numberOfItems', SaUint32T),
		('notification', POINTER(SaMsgQueueGroupNotificationT)),
		('queueGroupPolicy', SaMsgQueueGroupPolicyT)]


class SaMsgMessageT(Structure):
	"""Contain description of messages being sent and received.
	"""
	_fields_ = [('type', SaUint32T),
		('version', SaUint32T),
		('size', SaSizeT),
		('senderName', POINTER(SaNameT)),
		('data', SaVoidPtr),
		('priority', SaUint8T)]

SaMsgMessageCapacityStatusT = SaEnumT
eSaMsgMessageCapacityStatusT = Enumeration((
	('SA_MSG_QUEUE_CAPACITY_REACHED', 1),
	('SA_MSG_QUEUE_CAPACITY_AVAILABLE', 2),
	('SA_MSG_QUEUE_GROUP_CAPACITY_REACHED', 3),
	('SA_MSG_QUEUE_GROUP_CAPACITY_AVAILABLE', 4),
))

class SaMsgQueueThresholdsT(Structure):
	"""Contain information used to set and retrieve critical capacity
	thresholds for the priority areas of a message queue.
	"""
	_fields_ = [('capacityReached',
			(SaSizeT*(saMsg.SA_MSG_MESSAGE_LOWEST_PRIORITY+1))),
		('capacityAvailable',
			(SaSizeT*(saMsg.SA_MSG_MESSAGE_LOWEST_PRIORITY+1)))]

SaMsgStateT = SaEnumT
eSaMsgStateT = Enumeration((
	('SA_MSG_DEST_CAPACITY_STATUS', 1),
))

SaMsgLimitIdT = SaEnumT
eSaMsgLimitIdT = Enumeration((
	('SA_MSG_MAX_PRIORITY_AREA_SIZE_ID', 1),
	('SA_MSG_MAX_QUEUE_SIZE_ID', 2),
	('SA_MSG_MAX_NUM_QUEUES_ID', 3),
	('SA_MSG_MAX_NUM_QUEUE_GROUPS_ID', 4),
	('SA_MSG_MAX_NUM_QUEUES_PER_GROUP_ID', 5),
	('SA_MSG_MAX_MESSAGE_SIZE_ID', 6),
	('SA_MSG_MAX_REPLY_SIZE_ID', 7),
))

SaMsgQueueOpenCallbackT = CFUNCTYPE(None,
	SaInvocationT, SaMsgQueueHandleT, SaAisErrorT)

SaMsgQueueGroupTrackCallbackT = CFUNCTYPE(None,
	POINTER(SaNameT), POINTER(SaMsgQueueGroupNotificationBufferT),
	SaUint32T, SaAisErrorT)

SaMsgMessageDeliveredCallbackT = CFUNCTYPE(None,
	SaInvocationT, SaAisErrorT)

SaMsgMessageReceivedCallbackT = CFUNCTYPE(None, SaMsgQueueHandleT)

class SaMsgCallbacksT(Structure):
	"""Contain various callbacks Message Service may invoke on registrant.
	"""
	_fields_ = [('saMsgQueueOpenCallback',
			SaMsgQueueOpenCallbackT),
		('saMsgQueueGroupTrackCallback',
			SaMsgQueueGroupTrackCallbackT),
		('saMsgMessageDeliveredCallback',
			SaMsgMessageDeliveredCallbackT),
		('saMsgMessageReceivedCallback',
			SaMsgMessageReceivedCallbackT)]

def saMsgInitialize(msgHandle, msgCallbacks, version):
	"""Register invoking process with Message Service.

	type arguments:
		SaMsgHandleT msgHandle
		SaMsgCallbacksT msgCallbacks
		SaVersionT version

	returns:
		SaAisErrorT

	"""
	return msgdll.saMsgInitialize(BYREF(msgHandle),
			BYREF(msgCallbacks),
			BYREF(version))

def saMsgSelectionObjectGet(msgHandle, selectionObject):
	"""Return operating system handle associated with MSG handle to detect
	pending callbacks.

	type arguments:
		SaMsgHandleT msgHandle
		SaSelectionObjectT selectionObject

	returns:
		SaAisErrorT

	"""
	return msgdll.saMsgSelectionObjectGet(msgHandle,
			BYREF(selectionObject))

def saMsgDispatch(msgHandle, dispatchFlags):
	"""Invoke callbacks pending for the MSG handle.

	type arguments:
		SaMsgHandleT msgHandle
		SaDispatchFlagsT dispatchFlags

	returns:
		SaAisErrorT

	"""
	return msgdll.saMsgDispatch(msgHandle, dispatchFlags)

def saMsgFinalize(msgHandle):
	"""Close association between Message Service and the handle.

	type arguments:
		SaMsgHandleT msgHandle

	returns:
		SaAisErrorT

	"""
	return msgdll.saMsgFinalize(msgHandle)

def saMsgQueueOpen(msgHandle,
		queueName, creationAttributes, openFlags, timeout,
		queueHandle):
	"""Create and open a new message queue or open an existing message
	queue.

	type arguments:
		SaMsgHandleT msgHandle
		SaNameT queueName
		SaMsgQueueCreationAttributesT creationAttributes
		SaMsgQueueOpenFlagsT openFlags
		SaTimeT timeout
		SaMsgQueueHandleT queueHandle

	returns:
		SaAisErrorT

	"""
	return msgdll.saMsgQueueOpen(msgHandle,
			BYREF(queueName),
			BYREF(creationAttributes),
			openFlags,
			timeout,
			BYREF(queueHandle))

def saMsgQueueOpenAsync(msgHandle,
		invocation, queueName, creationAttributes, openFlags):
	"""Create and open a new message queue or open an existing message
	queue asynchronously.

	type arguments:
		SaMsgHandleT msgHandle
		SaInvocationT invocation
		SaNameT queueName
		SaMsgQueueCreationAttributesT creationAttributes
		SaMsgQueueOpenFlagsT openFlags

	returns:
		SaAisErrorT

	"""
	return msgdll.saMsgQueueOpenAsync(msgHandle,
			invocation,
			BYREF(queueName),
			BYREF(creationAttributes),
			openFlags)

def saMsgQueueClose(queueHandle):
	"""Close the message queue associated with queueHandle.

	type arguments:
		SaMsgQueueHandleT queueHandle

	returns:
		SaAisErrorT

	"""
	return msgdll.saMsgQueueClose(queueHandle)

def saMsgQueueStatusGet(msgHandle, queueName, queueStatus):
	"""Retrieve information about the status of the message queue.

	type arguments:
		SaMsgHandleT msgHandle
		SaNameT queueName
		SaMsgQueueStatusT queueStatus

	returns:
		SaAisErrorT

	"""
	return msgdll.saMsgQueueStatusGet(msgHandle,
			BYREF(queueName),
			BYREF(queueStatus))

def saMsgQueueRetentionTimeSet(queueHandle, retentionTime):
	"""Set the retention time of the message queue.

	type arguments:
		SaMsgQueueHandleT queueHandle
		SaTimeT retentionTime

	returns:
		SaAisErrorT

	"""
	return msgdll.saMsgQueueRetentionTimeSet(queueHandle,
			BYREF(retentionTime))

def saMsgQueueUnlink(msgHandle, queueName):
	"""Delete the message queue from the cluster.

	type arguments:
		SaMsgHandleT msgHandle
		SaNameT queueName

	returns:
		SaAisErrorT

	"""
	return msgdll.saMsgQueueUnlink(msgHandle,
			BYREF(queueName))

def saMsgQueueGroupCreate(msgHandle, queueGroupName, queueGroupPolicy):
	"""Create a message queue group with a particular policy.

	type arguments:
		SaMsgHandleT msgHandle
		SaNameT queueGroupName
		SaMsgQueueGroupPolicyT queueGroupPolicy

	returns:
		SaAisErrorT

	"""
	return msgdll.saMsgQueueGroupCreate(msgHandle,
			BYREF(queueGroupName),
			queueGroupPolicy)

def saMsgQueueGroupDelete(msgHandle, queueGroupName):
	"""Delete a message queue group immediately.

	type arguments:
		SaMsgHandleT msgHandle
		SaNameT queueGroupName

	returns:
		SaAisErrorT

	"""
	return msgdll.saMsgQueueGroupDelete(msgHandle,
			BYREF(queueGroupName))

def saMsgQueueGroupInsert(msgHandle, queueGroupName, queueName):
	"""Insert a message queue into a message queue group.

	type arguments:
		SaMsgHandleT msgHandle
		SaNameT queueGroupName
		SaNameT queueName

	returns:
		SaAisErrorT

	"""
	return msgdll.saMsgQueueGroupInsert(msgHandle,
			BYREF(queueGroupName),
			BYREF(queueName))

def saMsgQueueGroupRemove(msgHandle, queueGroupName, queueName):
	"""Remove a message queue from a message queue group.

	type arguments:
		SaMsgHandleT msgHandle
		SaNameT queueGroupName
		SaNameT queueName

	returns:
		SaAisErrorT

	"""
	return msgdll.saMsgQueueGroupRemove(msgHandle,
			BYREF(queueGroupName),
			BYREF(queueName))

def saMsgQueueGroupTrack(msgHandle,
		queueGroupName, trackFlags, notificationBuffer):
	"""Track changes in the membership of a message queue group.

	type arguments:
		SaMsgHandleT msgHandle
		SaNameT queueGroupName
		SaUint8T trackFlags
		SaMsgQueueGroupNotificationBufferT notificationBuffer

	returns:
		SaAisErrorT

	"""
	return msgdll.saMsgQueueGroupTrack(msgHandle,
			BYREF(queueGroupName),
			trackFlags,
			BYREF(notificationBuffer))

def saMsgQueueGroupTrackStop(msgHandle, queueGroupName):
	"""Stop tracking changes in membership of a message queue group.

	type arguments:
		SaMsgHandleT msgHandle
		SaNameT queueGroupName

	returns:
		SaAisErrorT

	"""
	return msgdll.saMsgQueueGroupTrackStop(msgHandle,
			BYREF(queueGroupName))

def saMsgQueueGroupNotificationFree(msgHandle, notification):
	"""Free notification memory allocated by Message Service during call to
	saMsgQueueGroupTrack().

	type arguments:
		SaMsgHandleT msgHandle
		SaMsgQueueGroupNotificationT notification

	returns:
		SaAisErrorT

	"""
	return msgdll.saMsgQueueGroupNotificationFree(msgHandle,
			notification)

def saMsgMessageSend(msgHandle, destination, message, timeout):
	"""Send message to message queue or message queue group.

	type arguments:
		SaMsgHandleT msgHandle
		SaNameT destination
		SaMsgMessageT message
		SaTimeT timeout

	returns:
		SaAisErrorT

	"""
	return msgdll.saMsgMessageSend(msgHandle,
			BYREF(destination),
			BYREF(message),
			timeout)

def saMsgMessageSendAsync(msgHandle,
		invocation, destination, message, ackFlags):
	"""Send message to message queue or message queue group asynchronously.

	type arguments:
		SaMsgHandleT msgHandle
		SaInvocationT invocation
		SaNameT destination
		SaMsgMessageT message
		SaMsgAckFlagsT ackFlags

	returns:
		SaAisErrorT

	"""
	return msgdll.saMsgMessageSendAsync(msgHandle,
			invocation,
			BYREF(destination),
			BYREF(message),
			ackFlags)

def saMsgMessageGet(queueHandle, message, sendTime, senderId, timeout):
	"""Get message from message queue associated with queueHandle.

	type arguments:
		SaMsgQueueHandleT queueHandle
		SaMsgMessageT message
		SaTimeT sendTime
		SaMsgSenderIdT senderId
		SaTimeT timeout

	returns:
		SaAisErrorT

	"""
	return msgdll.saMsgMessageGet(queueHandle,
			BYREF(message),
			BYREF(sendTime),
			BYREF(senderId),
			timeout)

def saMsgMessageDataFree(msgHandle, data):
	"""Free data memory allocated by the Message Service during call to
	saMsgMessageGet() or saMsgMessageSendReceive().

	type arguments:
		SaMsgHandleT msgHandle
		SaVoidPtr data

	returns:
		SaAisErrorT

	"""
	return msgdll.saMsgMessageDataFree(msgHandle,
			data)

def saMsgMessageCancel(queueHandle):
	"""Cancel blocking calls to saMsgMessageGet().

	type arguments:
		SaMsgQueueHandleT queueHandle

	returns:
		SaAisErrorT

	"""
	return msgdll.saMsgMessageCancel(queueHandle)

def saMsgMessageSendReceive(msgHandle,
		destination, sendMessage, receiveMessage, replySendTime,
		timeout):
	"""Enable transmission and reception of messages in a single invocation.

	type arguments:
		SaMsgHandleT msgHandle
		SaNameT destination
		SaMsgMessageT sendMessage
		SaMsgMessageT receiveMessage
		SaTimeT replySendTime
		SaTimeT timeout

	returns:
		SaAisErrorT

	"""
	return msgdll.saMsgMessageSendReceive(msgHandle,
			BYREF(destination),
			BYREF(sendMessage),
			BYREF(receiveMessage),
			BYREF(replySendTime),
			timeout)

def saMsgMessageReply(msgHandle, replyMessage, senderId, timeout):
	"""Reply to a message set via saMsgMessageSendReceive().

	type arguments:
		SaMsgHandleT msgHandle
		SaMsgMessageT replyMessage
		SaMsgSenderIdT senderId
		SaTimeT timeout

	returns:
		SaAisErrorT

	"""
	return msgdll.saMsgMessageReply(msgHandle,
			BYREF(replyMessage),
			BYREF(senderId),
			timeout)

def saMsgMessageReplyAsync(msgHandle,
		invocation, replyMessage, senderId, ackFlags):
	"""Reply to a message set via saMsgMessageSendReceive() asynchronously.

	type arguments:
		SaMsgHandleT msgHandle
		SaInvocationT invocation
		SaMsgMessageT replyMessage
		SaMsgSenderIdT senderId
		SaMsgAckFlagsT ackFlags

	returns:
		SaAisErrorT

	"""
	return msgdll.saMsgMessageReplyAsync(msgHandle,
			invocation,
			BYREF(replyMessage),
			BYREF(senderId),
			ackFlags)

# TODO: None of the following are actually implemented in C!

def saMsgQueueCapacityThresholdsSet(queueHandle, thresholds):
	"""Set critical capacity thresholds for priority areas of a message
	queue.

	type arguments:
		SaMsgQueueHandleT queueHandle
		SaMsgQueueThresholdsT thresholds

	returns:
		SaAisErrorT

	"""
	return msgdll.saMsgQueueCapacityThresholdsSet(queueHandle,
			BYREF(thresholds))

def saMsgQueueCapacityThresholdsGet(queueHandle, thresholds):
	"""Get critical capacity thresholds for a message queue.

	type arguments:
		SaMsgQueueHandleT queueHandle
		SaMsgQueueThresholdsT thresholds

	returns:
		SaAisErrorT

	"""
	return msgdll.saMsgQueueCapacityThresholdsGet(queueHandle,
			BYREF(thresholds))

def saMsgMetadataSizeGet(msgHandle, metadataSize):
	"""Get the size of the implementation-specific message metadata portion.

	type arguments:
		SaMsgHandleT msgHandle
		SaUint32T metadataSize

	returns:
		SaAisErrorT

	"""
	return msgdll.saMsgMetadataSizeGet(msgHandle,
			BYREF(metadataSize))

def saMsgLimitGet(msgHandle, limitId, limitValue):
	"""Get the current implementation-specific value of a Message Service
	limit.

	type arguments:
		SaMsgHandleT msgHandle
		SaMsgLimitIdT limitId
		SaLimitValueT limitValue

	returns:
		SaAisErrorT

	"""
	return msgdll.saMsgLimitGet(msgHandle,
			limitId,
			BYREF(limitValue))
