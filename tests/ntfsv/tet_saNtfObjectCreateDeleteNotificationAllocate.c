/*      -*- OpenSAF  -*-
 *
 * (C) Copyright 2008 The OpenSAF Foundation
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. This file and program are licensed
 * under the GNU Lesser General Public License Version 2.1, February 1999.
 * The complete license can be accessed from the following location:
 * http://opensource.org/licenses/lgpl-license.php
 * See the Copying file included with the OpenSAF distribution for full
 * licensing terms.
 *
 * Author(s): Ericsson AB
 *
 */
#include <utest.h>
#include <util.h>
#include "tet_ntf.h"
#include "tet_ntf_common.h"

void saNtfObjectCreateDeleteNotificationAllocate_01(void)
{
    SaNtfObjectCreateDeleteNotificationT myNotification;

    saNotificationAllocationParamsT myNotificationAllocationParams;
    saNotificationFilterAllocationParamsT myNotificationFilterAllocationParams;
    saNotificationParamsT myNotificationParams;

    fillInDefaultValues(&myNotificationAllocationParams,
                        &myNotificationFilterAllocationParams,
                        &myNotificationParams);

    safassert(saNtfInitialize(&ntfHandle, &ntfCallbacks, &ntfVersion), SA_AIS_OK);
    rc = saNtfObjectCreateDeleteNotificationAllocate(
        ntfHandle, /* handle to Notification Service instance */
        &myNotification,
        /* number of correlated notifications */
        myNotificationAllocationParams.numCorrelatedNotifications,
        /* length of additional text */
        myNotificationAllocationParams.lengthAdditionalText,
         /* number of additional info items*/
        myNotificationAllocationParams.numAdditionalInfo,
        /* number of state changes */
        myNotificationAllocationParams.numObjectAttributes,
        /* use default allocation size */
        myNotificationAllocationParams.variableDataSize);

    /* Event type */
    *(myNotification.notificationHeader.eventType) =
        SA_NTF_OBJECT_CREATION;

    /* event time to be set automatically to current
    time by saNtfNotificationSend */
    *(myNotification.notificationHeader.eventTime) =
        myNotificationParams.eventTime;

    /* Set Notification Object */
    myNotification.notificationHeader.notificationObject->length =
        myNotificationParams.notificationObject.length;
    (void)memcpy(myNotification.notificationHeader.notificationObject->value,
                 myNotificationParams.notificationObject.value,
                 myNotificationParams.notificationObject.length);

    /* Set Notifying Object */
    myNotification.notificationHeader.notifyingObject->length =
        myNotificationParams.notifyingObject.length;
    (void)memcpy(myNotification.notificationHeader.notifyingObject->value,
                 myNotificationParams.notifyingObject.value,
                 myNotificationParams.notifyingObject.length);

    /* set Notification Class Identifier */
    /* vendor id 33333 is not an existing SNMP enterprise number.
    Just an example */
    myNotification.notificationHeader.notificationClassId->vendorId =
        myNotificationParams.notificationClassId.vendorId;

    /* sub id of this notification class within "name space" of vendor ID */
    myNotification.notificationHeader.notificationClassId->majorId =
        myNotificationParams.notificationClassId.majorId;
    myNotification.notificationHeader.notificationClassId->minorId =
        myNotificationParams.notificationClassId.minorId;

    /* set additional text and additional info */
    (void)strncpy(myNotification.notificationHeader.additionalText,
                  myNotificationParams.additionalText,
                  myNotificationAllocationParams.lengthAdditionalText);

    /* Set source indicator */
    myNotification.sourceIndicator =
        &myNotificationParams.objectCreateDeleteSourceIndicator;

    /* Set objectAttibutes */
    myNotification.objectAttributes[0].attributeId =
        myNotificationParams.objectAttributes[0].attributeId;
    myNotification.objectAttributes[0].attributeType =
        myNotificationParams.objectAttributes[0].attributeType;
    myNotification.objectAttributes[0].attributeValue.int32Val =
        myNotificationParams.objectAttributes[0].attributeValue.int32Val;

    free(myNotificationParams.additionalText);
    safassert(saNtfNotificationFree(myNotification.notificationHandle), SA_AIS_OK);
    safassert(saNtfFinalize(ntfHandle), SA_AIS_OK);
    test_validate(rc, SA_AIS_OK);
}

/**
 * Test that return value SA_AIS_ERR_BAD_HANDLE works
 *
 * Strategy: Set the handle to zero or invalid number
 *           Create a handle and then destroy it.
 */
void saNtfObjectCreateDeleteNotificationAllocate_02(void)
{
	int errors = 0;

    SaNtfObjectCreateDeleteNotificationT myNotification;

    saNotificationAllocationParamsT myNotificationAllocationParams;
    saNotificationFilterAllocationParamsT myNotificationFilterAllocationParams;
    saNotificationParamsT myNotificationParams;

    fillInDefaultValues(&myNotificationAllocationParams,
                        &myNotificationFilterAllocationParams,
                        &myNotificationParams);

    ntfHandle = 0;
    rc = saNtfObjectCreateDeleteNotificationAllocate(
        ntfHandle, /* handle to Notification Service instance */
        &myNotification,
        /* number of correlated notifications */
        myNotificationAllocationParams.numCorrelatedNotifications,
        /* length of additional text */
        myNotificationAllocationParams.lengthAdditionalText,
         /* number of additional info items*/
        myNotificationAllocationParams.numAdditionalInfo,
        /* number of state changes */
        myNotificationAllocationParams.numObjectAttributes,
        /* use default allocation size */
        myNotificationAllocationParams.variableDataSize);
	if(rc != SA_AIS_ERR_BAD_HANDLE) {
		errors++;
	}

    free(myNotificationParams.additionalText);
    safassert(saNtfInitialize(&ntfHandle, &ntfCallbacks, &ntfVersion), SA_AIS_OK);
    safassert(saNtfFinalize(ntfHandle), SA_AIS_OK);

    rc = saNtfObjectCreateDeleteNotificationAllocate(
        ntfHandle, /* handle to Notification Service instance */
        &myNotification,
        /* number of correlated notifications */
        myNotificationAllocationParams.numCorrelatedNotifications,
        /* length of additional text */
        myNotificationAllocationParams.lengthAdditionalText,
         /* number of additional info items*/
        myNotificationAllocationParams.numAdditionalInfo,
        /* number of state changes */
        myNotificationAllocationParams.numObjectAttributes,
        /* use default allocation size */
        myNotificationAllocationParams.variableDataSize);
    if(rc != SA_AIS_ERR_BAD_HANDLE) {
    	errors++;
    }

	rc = (errors == 0)? SA_AIS_OK:  SA_AIS_ERR_BAD_HANDLE;

    test_validate(rc, SA_AIS_OK);
}

/**
 * Test that the return value SA_AIS_ERR_INVALID_PARAM works
 *
 * Strategy:
 */
void saNtfObjectCreateDeleteNotificationAllocate_03(void)
{
    saNotificationAllocationParamsT myNotificationAllocationParams;
    saNotificationFilterAllocationParamsT myNotificationFilterAllocationParams;
    saNotificationParamsT myNotificationParams;

    fillInDefaultValues(&myNotificationAllocationParams,
                        &myNotificationFilterAllocationParams,
                        &myNotificationParams);

    safassert(saNtfInitialize(&ntfHandle, &ntfCallbacks, &ntfVersion), SA_AIS_OK);
    rc = saNtfObjectCreateDeleteNotificationAllocate(
        ntfHandle, /* handle to Notification Service instance */
        NULL,
        /* number of correlated notifications */
        myNotificationAllocationParams.numCorrelatedNotifications,
        /* length of additional text */
        myNotificationAllocationParams.lengthAdditionalText,
         /* number of additional info items*/
        myNotificationAllocationParams.numAdditionalInfo,
        /* number of state changes */
        myNotificationAllocationParams.numObjectAttributes,
        /* use default allocation size */
        myNotificationAllocationParams.variableDataSize);

    free(myNotificationParams.additionalText);
    safassert(saNtfFinalize(ntfHandle), SA_AIS_OK);
    test_validate(rc, SA_AIS_ERR_INVALID_PARAM);
}

__attribute__ ((constructor)) static void saNtfObjectCreateDeleteNotificationAllocate_constructor(void)
{
    test_suite_add(6, "Producer API 2 allocate");
    test_case_add(6, saNtfObjectCreateDeleteNotificationAllocate_01, "saNtfObjectCreateDeleteNotificationAllocate SA_AIS_OK");
    test_case_add(6, saNtfObjectCreateDeleteNotificationAllocate_02, "saNtfObjectCreateDeleteNotificationAllocate SA_AIS_ERR_BAD_HANDLE");
    test_case_add(6, saNtfObjectCreateDeleteNotificationAllocate_03, "saNtfObjectCreateDeleteNotificationAllocate SA_AIS_ERR_INVALID_PARAM");
}


