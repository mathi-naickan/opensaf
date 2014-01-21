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

#include <poll.h>
#include <unistd.h>
#include <pthread.h>
#include "immtest.h"

static const SaNameT rdnObj1 = {sizeof("Obj1"), "Obj1"};
static const SaNameT rdnObj2 = {sizeof("Obj2"), "Obj2"};
static const SaNameT rdnObj3 = {sizeof("Obj3"), "Obj3"};
static SaNameT dnObj1; /* object to be deleted */
static SaNameT dnObj2; /* object to be modified */
static SaNameT dnObj3; /* object to be created */

static SaAisErrorT saImmOiCcbCompletedCallback_response = SA_AIS_OK;
static SaAisErrorT saImmOiCcbObjectCreateCallback_response = SA_AIS_OK;
static SaAisErrorT saImmOiCcbObjectDeleteCallback_response = SA_AIS_OK;
static SaAisErrorT saImmOiCcbObjectModifyCallback_response = SA_AIS_OK;
static SaAisErrorT saImmOiRtAttrUpdateCallback_response = SA_AIS_OK;

static int saveErrorStrings = 0;	/* boolean */
static SaStringT *returnErrorStrings = NULL;


static void saImmOiAdminOperationCallback(SaImmOiHandleT immOiHandle,
    SaInvocationT invocation,
    const SaNameT *objectName,
    SaImmAdminOperationIdT operationId,
    const SaImmAdminOperationParamsT_2 **params)
{
    TRACE_ENTER();
}

static void saImmOiCcbAbortCallback(SaImmOiHandleT immOiHandle,
    SaImmOiCcbIdT ccbId)
{
    TRACE_ENTER();
}

static void saImmOiCcbApplyCallback(SaImmOiHandleT immOiHandle,
    SaImmOiCcbIdT ccbId)
{
    TRACE_ENTER();
}

static SaAisErrorT saImmOiCcbCompletedCallback(SaImmOiHandleT immOiHandle,
    SaImmOiCcbIdT ccbId)
{
    TRACE_ENTER();
    return saImmOiCcbCompletedCallback_response;
}

static SaAisErrorT saImmOiCcbObjectCreateCallback(SaImmOiHandleT immOiHandle,
    SaImmOiCcbIdT ccbId,
    const SaImmClassNameT className,
    const SaNameT *parentName,
    const SaImmAttrValuesT_2 **attr)
{
    TRACE_ENTER2("%llu, %s, %s\n", ccbId, className, parentName->value);
    if(saImmOiCcbObjectCreateCallback_response != SA_AIS_OK)
    	safassert(saImmOiCcbSetErrorString(immOiHandle, ccbId, (SaStringT)"Set error string in saImmOiCcbObjectCreateCallback"), SA_AIS_OK);
    return saImmOiCcbObjectCreateCallback_response;
}

static SaAisErrorT saImmOiCcbObjectDeleteCallback(SaImmOiHandleT immOiHandle,
    SaImmOiCcbIdT ccbId,
    const SaNameT *objectName)
{
    TRACE_ENTER2("%llu, %s\n", ccbId, objectName->value);
    if(saImmOiCcbObjectDeleteCallback_response != SA_AIS_OK)
    	safassert(saImmOiCcbSetErrorString(immOiHandle, ccbId, (SaStringT)"Set error string in saImmOiCcbObjectDeleteCallback"), SA_AIS_OK);
    return saImmOiCcbObjectDeleteCallback_response;
}

static SaAisErrorT saImmOiCcbObjectModifyCallback(SaImmOiHandleT immOiHandle,
    SaImmOiCcbIdT ccbId,
    const SaNameT *objectName,
    const SaImmAttrModificationT_2 **attrMods)
{
    TRACE_ENTER2("%llu, %s\n", ccbId, objectName->value);
    if(saImmOiCcbObjectModifyCallback_response != SA_AIS_OK)
    	safassert(saImmOiCcbSetErrorString(immOiHandle, ccbId, (SaStringT)"Set error string in saImmOiCcbObjectModifyCallback"), SA_AIS_OK);
    return saImmOiCcbObjectModifyCallback_response;
}

static SaAisErrorT saImmOiRtAttrUpdateCallback(SaImmOiHandleT immOiHandle,
    const SaNameT *objectName,
    const SaImmAttrNameT *attributeNames)
{
    TRACE_ENTER();
    return saImmOiRtAttrUpdateCallback_response;
}

static const SaImmOiCallbacksT_2 callbacks = {
    saImmOiAdminOperationCallback,
    saImmOiCcbAbortCallback,
    saImmOiCcbApplyCallback,
    saImmOiCcbCompletedCallback,
    saImmOiCcbObjectCreateCallback,
    saImmOiCcbObjectDeleteCallback,
    saImmOiCcbObjectModifyCallback,
    saImmOiRtAttrUpdateCallback
};

static SaAisErrorT config_object_create(
    SaImmAdminOwnerHandleT ownerHandle,
    const SaNameT *pName,
    const SaNameT *rdn)
{
    SaImmCcbHandleT ccbHandle;
    const SaNameT* nameValues[] = {NULL, NULL};
    SaImmAttrValuesT_2 v2 = {"rdn",  SA_IMM_ATTR_SANAMET, 1, (void**)nameValues};
    SaUint32T  int1Value1 = __LINE__;
    SaUint32T* int1Values[] = {&int1Value1};
    SaImmAttrValuesT_2 v1 = {"attr1", SA_IMM_ATTR_SAUINT32T, 1, (void**)int1Values};
    const SaImmAttrValuesT_2 * attrValues[] = {&v1, &v2, NULL};

    TRACE_ENTER2("'%s'\n", rdn->value);

    nameValues[0] = rdn;
    safassert(saImmOmCcbInitialize(ownerHandle, 0, &ccbHandle), SA_AIS_OK);
    if (saImmOmCcbObjectCreate_2(ccbHandle, configClassName, pName, attrValues) == SA_AIS_ERR_EXIST)
        goto done;

    safassert(saImmOmCcbApply(ccbHandle), SA_AIS_OK);

done:
    return saImmOmCcbFinalize(ccbHandle);
}

static SaAisErrorT config_object_delete(
    SaImmAdminOwnerHandleT ownerHandle,
    const SaNameT *dn)
{
    SaImmCcbHandleT ccbHandle;

    TRACE_ENTER2("'%s'\n", dn->value);

    safassert(saImmOmCcbInitialize(ownerHandle, 0, &ccbHandle), SA_AIS_OK);
    if (saImmOmCcbObjectDelete(ccbHandle, dn) == SA_AIS_ERR_NOT_EXIST)
        goto done;

    safassert(saImmOmCcbApply(ccbHandle), SA_AIS_OK);

done:
    return saImmOmCcbFinalize(ccbHandle);
}

/**
 * 
 * @param arg - ptr to objectName
 * 
 * @return void*
 */
static void *objectImplementerThreadMain(void *arg)
{
    struct pollfd fds[1];
    int ret;
    char buf[256];
    const SaImmOiImplementerNameT implementerName = buf;
    SaSelectionObjectT selObj;
    SaImmHandleT handle;
    const SaNameT *objectName = arg;

    TRACE_ENTER();

    sprintf(buf, "%s_%s", __FUNCTION__, objectName->value);
    safassert(saImmOiInitialize_2(&handle, &callbacks, &immVersion), SA_AIS_OK);
    safassert(saImmOiImplementerSet(handle, implementerName), SA_AIS_OK);
    safassert(saImmOiObjectImplementerSet(handle, objectName, SA_IMM_ONE), SA_AIS_OK);
    safassert(saImmOiSelectionObjectGet(handle, &selObj), SA_AIS_OK);

    fds[0].fd = (int) selObj;
    fds[0].events = POLLIN;

    /* We can receive five callbacks: create, delete, modify, completed & apply */
    while(1)
    {
        ret = poll(fds, 1, 2000);
        if (ret == 0)
        {
            TRACE("poll timeout\n");
            break;
        }
        if (ret == -1)
            fprintf(stderr, "poll error: %s\n", strerror(errno));

        safassert(saImmOiDispatch(handle, SA_DISPATCH_ONE), SA_AIS_OK);
    }

    if (saImmOiObjectImplementerRelease(handle, objectName, SA_IMM_ONE) == SA_AIS_ERR_NOT_EXIST)
        TRACE("Cannot release object implementer for '%s'", objectName->value);

    safassert(saImmOiFinalize(handle), SA_AIS_OK);

    TRACE_LEAVE();

    return NULL;
}

/**
 * 
 * @param arg - ptr to className
 * 
 * @return void*
 */
static void *classImplementerThreadMain(void *arg)
{
    struct pollfd fds[1];
    int ret;
    const SaImmOiImplementerNameT implementerName = (SaImmOiImplementerNameT) __FUNCTION__;
    SaSelectionObjectT selObj;
    SaImmHandleT handle;
    const SaImmClassNameT className = arg;

    TRACE_ENTER();

    safassert(saImmOiInitialize_2(&handle, &callbacks, &immVersion), SA_AIS_OK);
    safassert(saImmOiImplementerSet(handle, implementerName), SA_AIS_OK);
    safassert(saImmOiClassImplementerSet(handle, className), SA_AIS_OK);
    safassert(saImmOiSelectionObjectGet(handle, &selObj), SA_AIS_OK);

    fds[0].fd = (int) selObj;
    fds[0].events = POLLIN;

    while(1)
    {
        ret = poll(fds, 1, 2000);
        if (ret == 0)
        {
            TRACE("poll timeout\n");
            break;
        }
        if (ret == -1)
            fprintf(stderr, "poll error: %s\n", strerror(errno));

        safassert(saImmOiDispatch(handle, SA_DISPATCH_ONE), SA_AIS_OK);
    }

    safassert(saImmOiClassImplementerRelease(handle, className), SA_AIS_OK);
    safassert(saImmOiFinalize(handle), SA_AIS_OK);

    TRACE_LEAVE();

    return NULL;
}

static SaAisErrorT om_ccb_exec(void)
{
    SaAisErrorT rc;
    SaImmHandleT handle;
    const SaImmAdminOwnerNameT adminOwnerName = (SaImmAdminOwnerNameT) __FUNCTION__;
    SaImmAdminOwnerHandleT ownerHandle;
    SaImmCcbHandleT ccbHandle;
    const SaNameT *objectNames[] = {&rootObj, NULL};
    const SaNameT* nameValues[] = {&rdnObj3, NULL};
    SaImmAttrValuesT_2 v2 = {"rdn",  SA_IMM_ATTR_SANAMET, 1, (void**)nameValues};
    SaUint32T  int1Value1 = __LINE__;
    SaUint32T* int1Values[] = {&int1Value1};
    SaImmAttrValuesT_2 v1 = {"attr1", SA_IMM_ATTR_SAUINT32T, 1, (void**)int1Values};
    const SaImmAttrValuesT_2 * attrValues[] = {&v1, &v2, NULL};
    SaImmAttrModificationT_2 attrMod = {SA_IMM_ATTR_VALUES_REPLACE, v1};
    const SaImmAttrModificationT_2 *attrMods[] = {&attrMod, NULL};
    SaStringT *errorStrings = NULL;

    TRACE_ENTER();
    safassert(saImmOmInitialize(&handle, NULL, &immVersion), SA_AIS_OK);
    safassert(saImmOmAdminOwnerInitialize(handle, adminOwnerName, SA_TRUE, &ownerHandle), SA_AIS_OK);
    safassert(saImmOmAdminOwnerSet(ownerHandle, objectNames, SA_IMM_SUBTREE), SA_AIS_OK);
    safassert(saImmOmCcbInitialize(ownerHandle, 0, &ccbHandle), SA_AIS_OK);

    if ((rc = saImmOmCcbObjectDelete(ccbHandle, &dnObj1)) != SA_AIS_OK)
        goto done;

    if ((rc = saImmOmCcbObjectModify_2(ccbHandle, &dnObj2, attrMods)) != SA_AIS_OK)
        goto done;

    if ((rc = saImmOmCcbObjectCreate_2(ccbHandle, configClassName, &rootObj, attrValues)) != SA_AIS_OK)
        goto done;

    safassert(saImmOmCcbApply(ccbHandle), SA_AIS_OK);

done:
    if(rc != SA_AIS_OK) {
        SaStringT *es = NULL;
        safassert(saImmOmCcbGetErrorStrings(ccbHandle, (const SaStringT **)&errorStrings), SA_AIS_OK);
        assert(*errorStrings != NULL);	/* In our tests, there are always error strings from failed callbacks */
        if(saveErrorStrings) {
            es = errorStrings;
            int errorStringSize = 0;
            while(*es) {				/* count error strings */
                errorStringSize++;
                es++;
            }

            returnErrorStrings = (SaStringT *)calloc(1, sizeof(SaStringT) * (errorStringSize + 1));

            es = returnErrorStrings;
        }
        while(*errorStrings) {
            if(saveErrorStrings) {
                *es = strdup(*errorStrings);
                es++;
            }
            TRACE("saImmOmCcbGetErrorStrings(%llu): %s", ccbHandle, *errorStrings);
            errorStrings++;
        }
    }

    safassert(saImmOmCcbFinalize(ccbHandle), SA_AIS_OK);
    safassert(saImmOmAdminOwnerFinalize(ownerHandle), SA_AIS_OK);
    safassert(saImmOmFinalize(handle), SA_AIS_OK);

    TRACE_LEAVE();
    return rc;
}

/**
 * Create a class and some objects.
 * 
 */
static void om_setup(void)
{
    const SaNameT *objectNames[] = {&rootObj, NULL};
    const SaImmAdminOwnerNameT adminOwnerName = (SaImmAdminOwnerNameT) __FUNCTION__;
    SaImmAdminOwnerHandleT ownerHandle;

    TRACE_ENTER();
    safassert(saImmOmInitialize(&immOmHandle, NULL, &immVersion), SA_AIS_OK);
    safassert(saImmOmAdminOwnerInitialize(immOmHandle, adminOwnerName, SA_TRUE, &ownerHandle), SA_AIS_OK);
    safassert(saImmOmAdminOwnerSet(ownerHandle, objectNames, SA_IMM_SUBTREE), SA_AIS_OK);
    safassert(config_object_create(ownerHandle, &rootObj, &rdnObj1), SA_AIS_OK);
    safassert(config_object_create(ownerHandle, &rootObj, &rdnObj2), SA_AIS_OK);
    safassert(saImmOmAdminOwnerRelease(ownerHandle, objectNames, SA_IMM_SUBTREE), SA_AIS_OK);
    safassert(saImmOmAdminOwnerFinalize(ownerHandle), SA_AIS_OK);
    safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);
    TRACE_LEAVE();
}

/**
 * Delete a class and some objects.
 * 
 */
static void om_teardown(void)
{
    const SaNameT *objectNames[] = {&rootObj, NULL};
    const SaImmAdminOwnerNameT adminOwnerName = (SaImmAdminOwnerNameT) __FUNCTION__;
    SaImmAdminOwnerHandleT ownerHandle;

    TRACE_ENTER();
    safassert(saImmOmInitialize(&immOmHandle, NULL, &immVersion), SA_AIS_OK);
    safassert(saImmOmAdminOwnerInitialize(immOmHandle, adminOwnerName, SA_TRUE, &ownerHandle), SA_AIS_OK);
    safassert(saImmOmAdminOwnerSet(ownerHandle, objectNames, SA_IMM_SUBTREE), SA_AIS_OK);
    if (config_object_delete(ownerHandle, &dnObj1) != SA_AIS_OK)
        TRACE("Object '%s' cannot be deleted\n", dnObj3.value);
    if (config_object_delete(ownerHandle, &dnObj2) != SA_AIS_OK)
        TRACE("Object '%s' cannot be deleted\n", dnObj3.value);
    if (config_object_delete(ownerHandle, &dnObj3) != SA_AIS_OK)
        TRACE("Object '%s' cannot be deleted\n", dnObj3.value);
    safassert(saImmOmAdminOwnerFinalize(ownerHandle), SA_AIS_OK);
    safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);
    TRACE_LEAVE();
}

static void saImmOiCcb_01(void)
{
    int res;
    pthread_t thread[2];

    TRACE_ENTER();
    om_setup();

    /* Create implementer threads */
    res = pthread_create(&thread[0], NULL, objectImplementerThreadMain, &dnObj1);
    assert(res == 0);
    res = pthread_create(&thread[1], NULL, objectImplementerThreadMain, &dnObj2);
    assert(res == 0);

    sleep(1); /* Race condition, allow implementer threads to set up !*/
    rc = om_ccb_exec();

    pthread_join(thread[0], NULL);
    pthread_join(thread[1], NULL);

    test_validate(rc, SA_AIS_OK);

    om_teardown();
    TRACE_LEAVE();
}

static void saImmOiCcb_02(void)
{
    int res;
    pthread_t thread[2];

    TRACE_ENTER();
    om_setup();

    /* Create implementer threads */
    res = pthread_create(&thread[0], NULL, objectImplementerThreadMain, &dnObj1);
    assert(res == 0);
    res = pthread_create(&thread[1], NULL, objectImplementerThreadMain, &dnObj2);
    assert(res == 0);

    saImmOiCcbObjectDeleteCallback_response = SA_AIS_ERR_BAD_OPERATION;
    sleep(1); /* Race condition, allow implementer threads to set up!*/
    rc = om_ccb_exec();

    pthread_join(thread[0], NULL);
    pthread_join(thread[1], NULL);

    if(rc != SA_AIS_ERR_BAD_OPERATION) {
        /* Note  that the error code returned by implementer need not
         always be the errro code returned over the OM API. 
        Specifically, when the OI rejects the operation, the IMM may
        in some cases abort the entire CCB with SA_AIS_ERR_FAILED_OPERATON. */
        test_validate(rc, SA_AIS_ERR_FAILED_OPERATION);
    } else 
    {
            test_validate(rc, SA_AIS_ERR_BAD_OPERATION);
    }

    om_teardown();
    saImmOiCcbObjectDeleteCallback_response = SA_AIS_OK;
    TRACE_LEAVE();
}

static void saImmOiCcb_03(void)
{
    int res;
    pthread_t thread[2];

    TRACE_ENTER();
    om_setup();

    /* Create implementer threads */
    res = pthread_create(&thread[0], NULL, classImplementerThreadMain, configClassName);
    assert(res == 0);
    
    sleep(1); /* Race condition, allow implementer threads to set up!*/
    rc = om_ccb_exec();

    pthread_join(thread[0], NULL);
 
    test_validate(rc, SA_AIS_OK);

    om_teardown();
    TRACE_LEAVE();
}

static void saImmOiCcb_04(void)
{
    int res;
    pthread_t threadid;

    TRACE_ENTER();
    om_setup();

    /* Create implementer threads */
    res = pthread_create(&threadid, NULL, classImplementerThreadMain, configClassName);
    assert(res == 0);
 
    saImmOiCcbObjectModifyCallback_response = SA_AIS_ERR_BAD_OPERATION;
    sleep(1); /* Race condition, allow implementer threads to set up!*/
    rc = om_ccb_exec();

    pthread_join(threadid, NULL);
 
    if(rc != SA_AIS_ERR_BAD_OPERATION) {
        /* Note  that the error code returned by implementer need not
         always be the errro code returned over the OM API. 
        Specifically, when the OI rejects the operation, the IMM may
        in some cases abort the entire CCB with SA_AIS_ERR_FAILED_OPERATON. */
        test_validate(rc, SA_AIS_ERR_FAILED_OPERATION);
    } else 
    {
        test_validate(rc, SA_AIS_ERR_BAD_OPERATION);
    }

    om_teardown();
    saImmOiCcbObjectModifyCallback_response = SA_AIS_OK;
    TRACE_LEAVE();
}

static void saImmOiCcb_05(void)
{
    int res;
    pthread_t threadid;
    SaStringT *errorStrings;

    TRACE_ENTER();
    om_setup();

    /* Create implementer threads */
    res = pthread_create(&threadid, NULL, classImplementerThreadMain, configClassName);
    assert(res == 0);

    saImmOiCcbObjectModifyCallback_response = SA_AIS_ERR_BAD_OPERATION;
    sleep(1); /* Race condition, allow implementer threads to set up!*/

    /* Set saveErrorStrings to 1 and save error strings in om_ccb_exec() */
    returnErrorStrings = NULL;
    saveErrorStrings = 1;
    rc = om_ccb_exec();
    saveErrorStrings = 0;

    /* There is at least one error string */
    assert(returnErrorStrings != NULL);
    assert(*returnErrorStrings != NULL);

    /* Free allocated memory in om_ccb_exec() */
    errorStrings = returnErrorStrings;
    while(*errorStrings) {
    	free(*errorStrings);
    	errorStrings++;
    }
    free(returnErrorStrings);

    pthread_join(threadid, NULL);

    if(rc != SA_AIS_ERR_BAD_OPERATION) {
        /* Note  that the error code returned by implementer need not
         always be the errro code returned over the OM API.
        Specifically, when the OI rejects the operation, the IMM may
        in some cases abort the entire CCB with SA_AIS_ERR_FAILED_OPERATON. */
        test_validate(rc, SA_AIS_ERR_FAILED_OPERATION);
    } else
    {
        test_validate(rc, SA_AIS_ERR_BAD_OPERATION);
    }

    om_teardown();
    saImmOiCcbObjectModifyCallback_response = SA_AIS_OK;
    TRACE_LEAVE();
}

__attribute__ ((constructor)) static void saImmOiCcb_constructor(void)
{
    dnObj1.length = (SaUint16T) sprintf((char*) dnObj1.value, "%s,%s", rdnObj1.value, rootObj.value);
    dnObj2.length = (SaUint16T) sprintf((char*) dnObj2.value, "%s,%s", rdnObj2.value, rootObj.value);
    dnObj3.length = (SaUint16T) sprintf((char*) dnObj3.value, "%s,%s", rdnObj3.value, rootObj.value);

    test_suite_add(4, "Configuration Objects Implementer");
    test_case_add(4, saImmOiCcb_01, "saImmOiCcb - SA_AIS_OK - 2 objectImplementer threads");
    test_case_add(4, saImmOiCcb_02, "saImmOiCcb - SA_AIS_ERR_BAD_OPERATION - 2 objectImplementer threads");
    test_case_add(4, saImmOiCcb_03, "saImmOiCcb - SA_AIS_OK - 1 classImplementer thread");
    test_case_add(4, saImmOiCcb_04, "saImmOiCcb - SA_AIS_ERR_BAD_OPERATION/FAILED_OPERATION - 1 classImplementer thread");
    test_case_add(4, saImmOiCcb_05, "saImmOiCcb - SA_AIS_ERR_BAD_OPERATION/FAILED_OPERATION - saImmOiCcbSetErrorString and saImmOmCcbGetErrorStrings");
}

