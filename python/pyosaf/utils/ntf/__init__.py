############################################################################
#
# (C) Copyright 2015 The OpenSAF Foundation
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
# Author(s): Ericsson
#
############################################################################

'''
    NTF common utilities
'''

from pyosaf import saNtf, saAis
import ctypes

from pyosaf.utils import decorate

saNtfInitialize                 = decorate(saNtf.saNtfInitialize)
saNtfLocalizedMessageFree       = decorate(saNtf.saNtfLocalizedMessageFree)
saNtfStateChangeNotificationFilterAllocate = decorate(saNtf.saNtfStateChangeNotificationFilterAllocate)
saNtfNotificationUnsubscribe    = decorate(saNtf.saNtfNotificationUnsubscribe)
saNtfNotificationReadInitialize = decorate(saNtf.saNtfNotificationReadInitialize)
saNtfInitialize_2               = decorate(saNtf.saNtfInitialize_2)
saNtfNotificationReadInitialize_2 = decorate(saNtf.saNtfNotificationReadInitialize_2)
saNtfNotificationSubscribe      = decorate(saNtf.saNtfNotificationSubscribe)
saNtfInitialize_3               = decorate(saNtf.saNtfInitialize_3)
saNtfSelectionObjectGet         = decorate(saNtf.saNtfSelectionObjectGet)
saNtfDispatch                   = decorate(saNtf.saNtfDispatch)
saNtfFinalize                   = decorate(saNtf.saNtfFinalize)
saNtfObjectCreateDeleteNotificationAllocate = decorate(saNtf.saNtfObjectCreateDeleteNotificationAllocate)
saNtfAttributeChangeNotificationAllocate = decorate(saNtf.saNtfAttributeChangeNotificationAllocate)
saNtfStateChangeNotificationAllocate = decorate(saNtf.saNtfStateChangeNotificationAllocate)
saNtfStateChangeNotificationAllocate_3 = decorate(saNtf.saNtfStateChangeNotificationAllocate_3)
saNtfAlarmNotificationAllocate = decorate(saNtf.saNtfAlarmNotificationAllocate)
saNtfSecurityAlarmNotificationAllocate = decorate(saNtf.saNtfSecurityAlarmNotificationAllocate)
saNtfMiscellaneousNotificationAllocate = decorate(saNtf.saNtfMiscellaneousNotificationAllocate)
saNtfPtrValAllocate             = decorate(saNtf.saNtfPtrValAllocate)
saNtfArrayValAllocate           = decorate(saNtf.saNtfArrayValAllocate)
saNtfIdentifierAllocate         = decorate(saNtf.saNtfIdentifierAllocate)
saNtfNotificationSend           = decorate(saNtf.saNtfNotificationSend)
saNtfNotificationSendWithId     = decorate(saNtf.saNtfNotificationSendWithId)
saNtfNotificationFree           = decorate(saNtf.saNtfNotificationFree)
saNtfVariableDataSizeGet        = decorate(saNtf.saNtfVariableDataSizeGet)
saNtfLocalizedMessageGet        = decorate(saNtf.saNtfLocalizedMessageGet)
saNtfLocalizedMessageFree_2     = decorate(saNtf.saNtfLocalizedMessageFree_2)
saNtfPtrValGet                  = decorate(saNtf.saNtfPtrValGet)
saNtfArrayValGet                = decorate(saNtf.saNtfArrayValGet)
saNtfObjectCreateDeleteNotificationFilterAllocate = decorate(saNtf.saNtfObjectCreateDeleteNotificationFilterAllocate)
saNtfAttributeChangeNotificationFilterAllocate = decorate(saNtf.saNtfAttributeChangeNotificationFilterAllocate)
saNtfStateChangeNotificationFilterAllocate_2 = decorate(saNtf.saNtfStateChangeNotificationFilterAllocate_2)
saNtfAlarmNotificationFilterAllocate = decorate(saNtf.saNtfAlarmNotificationFilterAllocate)
saNtfSecurityAlarmNotificationFilterAllocate = decorate(saNtf.saNtfSecurityAlarmNotificationFilterAllocate)
saNtfNotificationFilterFree     = decorate(saNtf.saNtfNotificationFilterFree)
saNtfNotificationSubscribe_3    = decorate(saNtf.saNtfNotificationSubscribe_3)
saNtfNotificationReadInitialize_3 = decorate(saNtf.saNtfNotificationReadInitialize_3)
saNtfNotificationUnsubscribe_2  = decorate(saNtf.saNtfNotificationUnsubscribe_2)
saNtfNotificationReadNext       = decorate(saNtf.saNtfNotificationReadNext)
saNtfNotificationReadNext_3     = decorate(saNtf.saNtfNotificationReadNext_3)
saNtfNotificationReadFinalize   = decorate(saNtf.saNtfNotificationReadFinalize)


HANDLE = saNtf.SaNtfHandleT()
SELECTION_OBJECT = saAis.SaSelectionObjectT()
CALLBACKS = saNtf.SaNtfCallbacksT()


class AdditionalInfo(object):
    ''' Represents a piece of additional info to be included in a
        notification
    '''

    def __init__(self, info_id, info_type, info_value):
        self.info_id = info_id
        self.info_type = info_type
        self.info_value = info_value


class StateChange(object):
    ''' Contains information about a state change event'''

    def __init__(self):
        pass


class AttributeChange(object):
    ''' Contains information about a change in an attribute'''

    def __init__(self):
        pass


class Attribute(object):
    ''' Contains information about the value and value type of an attribute'''

    def __init__(self):
        pass


class SecurityAlarmDetector(object):
    ''' Represents an instance of a security alarm detector'''

    def __init__(self, value=None, value_type=None):
        self.value = value
        self.value_type = value_type


class ServiceUser(object):
    ''' Represents a service user'''

    def __init__(self, value=None, value_type=None):
        self.value = value
        self.value_type = value_type


class ServiceProvider(object):
    ''' Represents a service provider'''

    def __init__(self, value=None, value_type=None):
        self.value = value
        self.value_type = value_type


def dummy_func(*args):
    ''' Dummy function used as a callback when no proper callbacks are set'''
    pass

def initialize(notification_callback=None):
    ''' Initializes the NTF library'''

    # Initialize the NTF API
    version = saAis.SaVersionT('A', 1, 1)

    # Assign default values for callbacks
    CALLBACKS.saNtfNotificationCallback = \
                    saNtf.SaNtfNotificationCallbackT(dummy_func)
    CALLBACKS.saNtfNotificationDiscardedCallback = \
                    saNtf.SaNtfNotificationDiscardedCallbackT(dummy_func)

    # Override the notification subscribe callback if it's passed
    if notification_callback:
        CALLBACKS.saNtfNotificationCallback = \
                    saNtf.SaNtfNotificationCallbackT(notification_callback)

    # Initialize the API
    saNtfInitialize(HANDLE, CALLBACKS, version)

    # Get the selection object
    saNtfSelectionObjectGet(HANDLE, SELECTION_OBJECT)


def assign_ntf_value_to_attribute(attr_value_field, value, value_type):
    ''' Assigns the correct sub-field in the given attribute

        e.g. attr_value_field.uint8Val = ...

    '''

    if value_type == saNtf.eSaNtfValueTypeT.SA_NTF_VALUE_UINT8:
        attr_value_field.uint8Val = value

    elif value_type == saNtf.eSaNtfValueTypeT.SA_NTF_VALUE_INT8:
        attr_value_field.int8Val = value

    elif value_type == saNtf.eSaNtfValueTypeT.SA_NTF_VALUE_UINT16:
        attr_value_field.uint16Val = value

    elif value_type == saNtf.eSaNtfValueTypeT.SA_NTF_VALUE_INT16:
        attr_value_field.int16Val = value

    elif value_type == saNtf.eSaNtfValueTypeT.SA_NTF_VALUE_UINT32:
        attr_value_field.uint32Val = value

    elif value_type == saNtf.eSaNtfValueTypeT.SA_NTF_VALUE_INT32:
        attr_value_field.int32Val = value

    elif value_type == saNtf.eSaNtfValueTypeT.SA_NTF_VALUE_FLOAT:
        attr_value_field.floatVal = value

    elif value_type == saNtf.eSaNtfValueTypeT.SA_NTF_VALUE_UINT64:
        attr_value_field.uint64Val = value

    elif value_type == saNtf.eSaNtfValueTypeT.SA_NTF_VALUE_INT64:
        attr_value_field.int64Val = value

    elif value_type == saNtf.eSaNtfValueTypeT.SA_NTF_VALUE_DOUBLE:
        attr_value_field.doubleVal = value


def fill_in_header(notification_handle,
                   header, notification_object, notifying_object, vendor_id,
                   major_id, minor_id, additional_text, event_type, event_time,
                   additional_info):
    ''' Fills in the given notification header with the provided values'''

    header.eventType.contents.value = event_type
    header.notificationObject.contents.value = notification_object
    header.notificationObject.contents.length = len(notification_object)
    header.notifyingObject.contents.value = notifying_object
    header.notifyingObject.contents.length = len(notifying_object)
    header.notificationClassId.contents.vendorId = vendor_id
    header.notificationClassId.contents.majorId = major_id
    header.notificationClassId.contents.minorId = minor_id
    header.eventTime.contents.value = event_time
    header.numCorrelatedNotifications = 0
    header.lengthAdditionalText = len(additional_text)

    ##
    ## FIXME: this fails to copy the value of the additional_text variable into
    ##        header.additionalText.

    ctypes.memmove(header.additionalText,
                   additional_text,
                   len(additional_text))

    header.numAdditionalInfo = len(additional_info)
    header.thresholdInformation = None

    # Fill in additional info
    if additional_info != []:
        for i in range(0, len(additional_info)):

            header.additionalInfo[i].infoId = additional_info[i].info_id
            header.additionalInfo[i].infoType = additional_info[i].info_type

            dest_ptr = (ctypes.c_char * len(additional_info[i].info_value))()

            saNtf.saNtfPtrValAllocate(notification_handle,
                                      len(additional_info[i].info_value) + 1,
                                      dest_ptr,
                                      header.additionalInfo[i].infoValue)

            ctypes.memmove(ctypes.addressof(dest_ptr),
                           additional_info[i].info_value,
                           len(additional_info[i].info_value) + 1)

    else:
        header.additionalInfo = None


def send_object_create_notification(vendor_id, major_id, minor_id,
                                    additional_text="",
                                    notification_object="",
                                    notifying_object="",
                                    attributes=[],
                                    event_time=saAis.saAis.SA_TIME_UNKNOWN,
                                    additional_info=[]):
    ''' Sends a notification for a created object'''

    _send_object_create_delete_notification(
        saNtf.eSaNtfEventTypeT.SA_NTF_OBJECT_CREATION,
        vendor_id, major_id, minor_id,
        additional_text=additional_text,
        notification_object=notification_object,
        notifying_object=notifying_object,
        attributes=attributes,
        event_time=event_time,
        additional_info=additional_info)


def send_object_delete_notification(vendor_id,
                                    major_id,
                                    minor_id,
                                    additional_text="",
                                    notification_object="",
                                    notifying_object="",
                                    attributes=[],
                                    event_time=saAis.saAis.SA_TIME_UNKNOWN,
                                    additional_info=[]):
    ''' Sends a notification for a deleted object'''

    _send_object_create_delete_notification(
        saNtf.eSaNtfEventTypeT.SA_NTF_OBJECT_DELETION,
        vendor_id, major_id, minor_id,
        additional_text=additional_text,
        notification_object=notification_object,
        notifying_object=notifying_object,
        attributes=attributes,
        event_time=event_time,
        additional_info=additional_info)


def _send_object_create_delete_notification(
        event_type,
        vendor_id,
        major_id,
        minor_id,
        additional_text="",
        notification_object="",
        notifying_object="",
        attributes=[],
        event_time=saAis.saAis.SA_TIME_UNKNOWN,
        additional_info=[]):
    ''' Sends an object create delete notification'''

    # Create the notification
    notification = saNtf.SaNtfObjectCreateDeleteNotificationT()

    saNtfObjectCreateDeleteNotificationAllocate(
        HANDLE,
        notification,
        0,
        len(additional_text) + 1,
        len(additional_info),
        len(attributes),
        0)

    # Fill in the header
    fill_in_header(notification.notificationHandle,
                   notification.notificationHeader,
                   notification_object, notifying_object, vendor_id,
                   major_id, minor_id, additional_text,
                   event_type, event_time, additional_info)

    # Fill in attributes
    for i in range(0, notification.numAttributes):

        ptr = notification.objectAttributes[i]

        ptr.attributeId = attributes[i].attribute_id
        ptr.attributeType = attributes[i].attribute_type

        assign_ntf_value_to_attribute(ptr.attributeValue,
                                      attributes[i].attribute_value,
                                      attributes[i].attribute_type)


    # Send the notification
    saNtfNotificationSend(notification.notificationHandle)

    # Free the notification
    saNtfNotificationFree(notification.notificationHandle)


def send_state_change_notification(vendor_id,
                                   major_id,
                                   minor_id,
                                   additional_text="",
                                   notification_object="",
                                   notifying_object="",
                                   event_time=saAis.saAis.SA_TIME_UNKNOWN,
                                   additional_info=[],
                                   state_changes=[]):
    ''' Sends a state change notification'''

    event_type = saNtf.eSaNtfEventTypeT.SA_NTF_OBJECT_STATE_CHANGE

    # Create the notification
    notification = saNtf.SaNtfStateChangeNotificationT()

    saNtfStateChangeNotificationAllocate(
        HANDLE,
        notification,
        0,
        len(additional_text) + 1,
        len(additional_info),
        len(state_changes),
        0)

    # Fill in the header
    fill_in_header(notification.notificationHandle,
                   notification.notificationHeader,
                   notification_object, notifying_object, vendor_id,
                   major_id, minor_id, additional_text, event_type,
                   event_time, additional_info)

    # Fill in state changes

    for i in range(0, len(state_changes)):

        notification.changedStates[i].stateId = state_changes[i].state_id

        if state_changes[i].old_state_present:
            notification.changedStates[i].oldStatePresent = saAis.eSaBoolT.SA_TRUE
            notification.changedStates[i].oldState = state_changes[i].old_state

        else:
            notification.changedStates[i].oldStatePresent = saAis.eSaBoolT.SA_FALSE

        notification.changedStates[i].newState = state_changes[i].new_state

    # Send the alarm notification
    saNtfNotificationSend(notification.notificationHandle)

    # Free the alarm notification
    saNtfNotificationFree(notification.notificationHandle)


def send_attribute_change_notification(vendor_id,
                                       major_id,
                                       minor_id,
                                       additional_text="",
                                       notification_object="",
                                       notifying_object="",
                                       event_type=saNtf.eSaNtfEventTypeT.SA_NTF_ATTRIBUTE_ADDED,
                                       event_time=saAis.saAis.SA_TIME_UNKNOWN,
                                       additional_info=[],
                                       changed_attributes=[]):
    ''' Sends an attribute_change notification'''

    # Create the notification
    notification = saNtf.SaNtfAttributeChangeNotificationT()

    saNtfAttributeChangeNotificationAllocate(
        HANDLE,
        notification,
        0,
        len(additional_text) + 1,
        len(additional_info),
        len(changed_attributes),
        0)

    # Fill in the header
    fill_in_header(notification.notificationHandle,
                   notification.notificationHeader,
                   notification_object, notifying_object, vendor_id,
                   major_id, minor_id, additional_text, event_type,
                   event_time, additional_info)


    # Fill in attributes
    for i in range(0, len(changed_attributes)):

        ptr = notification.changedAttributes[i]

        ptr.attributeId = changed_attributes[i].attribute_id
        ptr.attributeType = changed_attributes[i].attribute_type

        if changed_attributes[i].old_attribute_present:
            ptr.oldAttributePresent = saAis.eSaBoolT.SA_TRUE

            assign_ntf_value_to_attribute(ptr.oldAttributeValue,
                                          changed_attributes[i].old_attribute_value,
                                          changed_attributes[i].attribute_type)

        else:
            ptr.oldAttributePresent = saAis.eSaBoolT.SA_FALSE

        assign_ntf_value_to_attribute(ptr.newAttributeValue,
                                      changed_attributes[i].new_attribute_value,
                                      changed_attributes[i].attribute_type)

    # Send the alarm notification
    saNtfNotificationSend(notification.notificationHandle)

    # Free the alarm notification
    saNtfNotificationFree(notification.notificationHandle)


def send_security_alarm_notification(vendor_id, major_id, minor_id, severity,
                                     alarm_detector,
                                     user,
                                     provider,
                                     additional_text="",
                                     notification_object="",
                                     notifying_object="",
                                     event_type=saNtf.eSaNtfEventTypeT.SA_NTF_INTEGRITY_VIOLATION,
                                     event_time=saAis.saAis.SA_TIME_UNKNOWN,
                                     additional_info=[],
                                     probable_cause=saNtf.eSaNtfProbableCauseT.SA_NTF_SOFTWARE_ERROR):

    ''' Sends a security alarm notification'''

    # Create the notification
    notification = saNtf.SaNtfSecurityAlarmNotificationT()

    saNtfSecurityAlarmNotificationAllocate(
        HANDLE,
        notification,
        0,
        len(additional_text) + 1,
        len(additional_info),
        0)

    # Fill in the header
    fill_in_header(notification.notificationHandle,
                   notification.notificationHeader,
                   notification_object, notifying_object, vendor_id,
                   major_id, minor_id, additional_text, event_type,
                   event_time, additional_info)

    # Fill in security alarm-specific fields
    notification.probableCause.contents.value = probable_cause
    notification.severity.contents.value = severity

    assign_ntf_value_to_attribute(notification.securityAlarmDetector,
                                  alarm_detector.value,
                                  alarm_detector.value_type)

    assign_ntf_value_to_attribute(notification.serviceUser,
                                  user.value,
                                  user.value_type)

    assign_ntf_value_to_attribute(notification.serviceProvider,
                                  provider.value,
                                  provider.value_type)

    # Send the alarm notification
    saNtfNotificationSend(notification.notificationHandle)

    # Free the alarm notification
    saNtfNotificationFree(notification.notificationHandle)


def send_alarm_notification(vendor_id, major_id, minor_id, severity,
                            additional_text="",
                            notification_object="",
                            notifying_object="",
                            event_type=saNtf.eSaNtfEventTypeT.SA_NTF_ALARM_PROCESSING,
                            event_time=saAis.saAis.SA_TIME_UNKNOWN,
                            additional_info=[]):
    ''' Sends an alarm notification'''

    # Create the notification
    notification = saNtf.SaNtfAlarmNotificationT()

    saNtfAlarmNotificationAllocate(
        HANDLE,
        notification,
        0,
        len(additional_text) + 1,
        len(additional_info),
        0,
        0,
        0,
        0)

    # Fill in the header
    fill_in_header(notification.notificationHandle,
                   notification.notificationHeader,
                   notification_object, notifying_object, vendor_id,
                   major_id, minor_id, additional_text, event_type,
                   event_time, additional_info)

    notification.numMonitoredAttributes = 0
    notification.numSpecificProblems = 0
    notification.numProposedRepairActions = 0
    notification.perceivedSeverity.contents.value = severity
    notification.probableCause.contents.value = \
                                saNtf.eSaNtfProbableCauseT.SA_NTF_DEGRADED_SIGNAL

    # Send the alarm notification
    saNtfNotificationSend(notification.notificationHandle)

    # Free the alarm notification
    saNtfNotificationFree(notification.notificationHandle)


def subscribe_for_notifications(notification_types=None):
    ''' Subscribes for notifications from NTF. The types of notifications to
        subscribe to are passed in the notification_types list and
        subscriptions are set up for all types if the list is empty
    '''

    filters = []

    filter_handles = saNtf.SaNtfNotificationTypeFilterHandlesT()

    filter_handles.objectCreateDeleteFilterHandle = saNtf.SaNtfNotificationFilterHandleT(0)
    filter_handles.attributeChangeFilterHandle = saNtf.SaNtfNotificationFilterHandleT(0)
    filter_handles.stateChangeFilterHandle = saNtf.SaNtfNotificationFilterHandleT(0)
    filter_handles.alarmFilterHandle = saNtf.SaNtfNotificationFilterHandleT(0)
    filter_handles.securityFilterHandle = saNtf.SaNtfNotificationFilterHandleT(0)


    # Create and allocate the alarm filter
    if not notification_types or \
       saNtf.eSaNtfNotificationTypeT.SA_NTF_TYPE_ALARM in notification_types:
        notification_filter = saNtf.SaNtfAlarmNotificationFilterT()

        saNtfAlarmNotificationFilterAllocate(HANDLE, notification_filter,
                                             0, 0, 0, 0, 0, 0, 0)

        filter_handles.alarmFilterHandle = notification_filter.notificationFilterHandle

        filters.append(notification_filter.notificationFilterHandle)

    # Create and allocate the object create delete filter
    if not notification_types or \
       saNtf.eSaNtfNotificationTypeT.SA_NTF_TYPE_OBJECT_CREATE_DELETE in notification_types:
        notification_filter = saNtf.SaNtfObjectCreateDeleteNotificationFilterT()

        saNtfObjectCreateDeleteNotificationFilterAllocate(HANDLE,
                                                          notification_filter,
                                                          0, 0, 0, 0, 0)

        filter_handles.objectCreateDeleteFilterHandle = notification_filter.notificationFilterHandle
        filters.append(notification_filter.notificationFilterHandle)

    # Create and allocate the attribute change filter
    if not notification_types or \
       saNtf.eSaNtfNotificationTypeT.SA_NTF_TYPE_ATTRIBUTE_CHANGE in notification_types:

        notification_filter = saNtf.SaNtfAttributeChangeNotificationFilterT()

        saNtfAttributeChangeNotificationFilterAllocate(HANDLE,
                                                       notification_filter,
                                                       0, 0, 0, 0, 0)

        filter_handles.attributeChangeFilterHandle = notification_filter.notificationFilterHandle

        filters.append(notification_filter.notificationFilterHandle)

    # Create and allocate the state change filter
    if not notification_types or \
       saNtf.eSaNtfNotificationTypeT.SA_NTF_TYPE_STATE_CHANGE in notification_types:

        notification_filter = saNtf.SaNtfStateChangeNotificationFilterT()

        saNtfStateChangeNotificationFilterAllocate(HANDLE,
                                                   notification_filter,
                                                   0, 0, 0, 0, 0, 0)

        filter_handles.stateChangeFilterHandle = notification_filter.notificationFilterHandle

        filters.append(notification_filter.notificationFilterHandle)

    # Create and allocate the security alarm filter
    if not notification_types or \
       saNtf.eSaNtfNotificationTypeT.SA_NTF_TYPE_SECURITY_ALARM in notification_types:

        notification_filter = saNtf.SaNtfSecurityAlarmNotificationFilterT()

        saNtfSecurityAlarmNotificationFilterAllocate(HANDLE,
                                                     notification_filter,
                                                     0, 0, 0, 0, 0, 0, 0, 0, 0)

        filter_handles.securityAlarmFilterHandle = notification_filter.notificationFilterHandle

        filters.append(notification_filter.notificationFilterHandle)

    # Create a unique subscription id
    sub_id = saNtf.SaNtfSubscriptionIdT(1)

    # Start subscription
    saNtfNotificationSubscribe(filter_handles, sub_id)

    # Free up the filters
    for filter_handle in filters:
        saNtfNotificationFilterFree(filter_handle)


def dispatch(mode=saAis.eSaDispatchFlagsT.SA_DISPATCH_ALL):
    ''' Invokes NTF callbacks for queued events. The default is to dispatch all
        available events
    '''

    saNtfDispatch(HANDLE, mode)

