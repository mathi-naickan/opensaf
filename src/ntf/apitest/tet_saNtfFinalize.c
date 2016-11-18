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
#include "osaf/apitest/utest.h"
#include "osaf/apitest/util.h"
#include "tet_ntf.h"
#include <unistd.h>
#include <pthread.h>

SaNtfStateChangeNotificationT myNotification;
void saNtfFinalize_01(void)
{
    safassert(saNtfInitialize(&ntfHandle, &ntfCallbacks, &ntfVersion), SA_AIS_OK);
    rc = saNtfFinalize(ntfHandle);
    test_validate(rc, SA_AIS_OK);
}

void saNtfFinalize_02(void)
{
    rc = saNtfFinalize(-1);
    test_validate(rc, SA_AIS_ERR_BAD_HANDLE);
}

void saNtfFinalize_03(void)
{
    safassert(saNtfInitialize(&ntfHandle, &ntfCallbacks, &ntfVersion), SA_AIS_OK);
    safassert(saNtfFinalize(ntfHandle), SA_AIS_OK);
    rc = saNtfFinalize(ntfHandle);
    test_validate(rc, SA_AIS_ERR_BAD_HANDLE);
}

SaAisErrorT subscribe()
{
    SaAisErrorT ret;
    SaNtfObjectCreateDeleteNotificationFilterT obcf;
    SaNtfNotificationTypeFilterHandlesT FilterHandles;
    memset(&FilterHandles, 0, sizeof(FilterHandles));

    ret = saNtfObjectCreateDeleteNotificationFilterAllocate(ntfHandle, &obcf,0,0,0,1,0);
    obcf.notificationFilterHeader.notificationClassIds->vendorId = SA_NTF_VENDOR_ID_SAF;
    obcf.notificationFilterHeader.notificationClassIds->majorId = 222;
    obcf.notificationFilterHeader.notificationClassIds->minorId = 222;
    FilterHandles.objectCreateDeleteFilterHandle = obcf.notificationFilterHandle;
    ret = saNtfNotificationSubscribe(&FilterHandles, 111);
    return ret;
}

void *unsubscribe(void *arg)
{
    SaAisErrorT *ret = (SaAisErrorT *)arg;
    *ret = saNtfNotificationUnsubscribe(111);
    pthread_exit(NULL);
}

void saNtfFinalize_04()
{
    SaAisErrorT ret1, ret2;
    safassert(saNtfInitialize(&ntfHandle, &ntfCallbacks, &ntfVersion), SA_AIS_OK);
    safassert(subscribe(),SA_AIS_OK);
    pthread_t thread;
    pthread_create(&thread, NULL, unsubscribe, (void *) &ret2);
    usleep(1);
    ret1 = saNtfFinalize(ntfHandle);
    pthread_join(thread, NULL);
    printf("    Return value from thread:%u\n",ret2);

    //saNtfUnsubscribe() may fail with expected return value BAD_HANDLE or TRY_AGAIN or NOT_EXIST.
    if ((ret2 == SA_AIS_ERR_BAD_HANDLE) || (ret2 == SA_AIS_ERR_TRY_AGAIN) ||
		    (ret2 == SA_AIS_ERR_NOT_EXIST))
	    ret2 = SA_AIS_OK;
    safassert(ret2,SA_AIS_OK);
    test_validate(ret1, SA_AIS_OK);
}
void *allocate(void *arg) 
{
    SaAisErrorT *ret = (SaAisErrorT *)arg;
    SaStringT myAdditionalText = "My additional text";
    SaNameT object = {9, "smallerdn" };

    *ret = saNtfStateChangeNotificationAllocate(
		    ntfHandle, /* handle to Notification Service instance */
		    &myNotification,
		    0 /* number of correlated notifications */,
		    strlen(myAdditionalText) + 1 /* length of additional text */,
		    1 /* number of additional info items*/,
		    1 /* number of changed object states */,
		    SA_NTF_ALLOC_SYSTEM_LIMIT /* use default allocation size */);
    if (*ret != SA_AIS_OK) 
	goto done;
    *(myNotification.notificationHeader.eventType) = SA_NTF_OBJECT_STATE_CHANGE;
    *(myNotification.notificationHeader.eventTime) = SA_TIME_UNKNOWN;

    myNotification.notificationHeader.notificationClassId->vendorId = 33333;
    myNotification.notificationHeader.notificationClassId->majorId = 997;
    myNotification.notificationHeader.notificationClassId->minorId = 1;

    *(myNotification.sourceIndicator) = SA_NTF_OBJECT_OPERATION;

    myNotification.changedStates[0].stateId = 1;
    myNotification.changedStates[0].oldStatePresent = SA_TRUE;
    myNotification.changedStates[0].oldState = 1;
    myNotification.changedStates[0].newState = 2;

    strcpy(myNotification.notificationHeader.additionalText, myAdditionalText);

    myNotification.notificationHeader.additionalInfo[0].infoId = 2;
    myNotification.notificationHeader.additionalInfo[0].infoType = SA_NTF_VALUE_UINT8;
    myNotification.notificationHeader.additionalInfo[0].infoValue.uint8Val = 42;

    myNotification.notificationHeader.notificationObject->length = object.length;
    memcpy(myNotification.notificationHeader.notificationObject->value, object.value, object.length);
done:
    pthread_exit(NULL);
}
void saNtfFinalize_05()
{
    SaAisErrorT ret1, ret2;
    safassert(saNtfInitialize(&ntfHandle, &ntfCallbacks, &ntfVersion), SA_AIS_OK);
    pthread_t thread;
    pthread_create(&thread, NULL, allocate, (void *) &ret2);
    usleep(1);
    ret1 = saNtfFinalize(ntfHandle);
    pthread_join(thread, NULL);
    printf("    Return value from thread:%u\n",ret2);

    //saNtfUnsubscribe() may fail with expected return value BAD_HANDLE or TRY_AGAIN.
    if ((ret2 == SA_AIS_ERR_BAD_HANDLE) || (ret2 == SA_AIS_ERR_TRY_AGAIN))
            ret2 = SA_AIS_OK;
    safassert(ret2,SA_AIS_OK);
    test_validate(ret1, SA_AIS_OK);

}

__attribute__ ((constructor)) static void saNtfFinalize_constructor(void)
{
    test_suite_add(2, "Life cycle, finalize, API 2");
    test_case_add(2, saNtfFinalize_01, "saNtfFinalize SA_AIS_OK");
    test_case_add(2, saNtfFinalize_02, "saNtfFinalize SA_AIS_ERR_BAD_HANDLE - invalid handle");
    test_case_add(2, saNtfFinalize_03, "saNtfFinalize SA_AIS_ERR_BAD_HANDLE - handle already returned");
    test_case_add(2, saNtfFinalize_04, "Finalize and Unsubscribe in parallel SA_AIS_OK");
    test_case_add(2, saNtfFinalize_05, "Finalize and saNtfStateChangeNotificationAllocate in parallel SA_AIS_OK");
}

