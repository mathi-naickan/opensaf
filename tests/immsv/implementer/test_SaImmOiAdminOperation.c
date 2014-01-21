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

static SaNameT rdn = 
    {sizeof("Obj3"), 
     "Obj3"};
static SaAisErrorT saImmOiAdminOperationCallback_response = SA_AIS_OK;
static SaImmAdminOperationIdT operationId = 0xdead;
static SaUint64T value = 0xbad;
static SaBoolT objectImplementerIsSet;
static SaAisErrorT immReturnValue;
static SaAisErrorT operationReturnValue;
static SaInvocationT userInvocation = 0xdead;

/* SaImmAdminOperationError */
static SaStringT adminOperationErrorString = NULL;
static SaImmAdminOperationParamsT_2 adminOperationErrorParam = {
		SA_IMM_PARAM_ADMOP_ERROR,
		SA_IMM_ATTR_SASTRINGT,
		&adminOperationErrorString };
static const SaImmAdminOperationParamsT_2 *adminOperationErrorParams[2] = {
		&adminOperationErrorParam,
		NULL };

/* SaImmAdminOperationName */
static SaStringT adminOperationName = NULL;
static SaImmAdminOperationParamsT_2 adminOperationNameParam = {
		SA_IMM_PARAM_ADMOP_NAME,
		SA_IMM_ATTR_SASTRINGT,
		&adminOperationName };


static void saImmOiAdminOperationCallback(SaImmOiHandleT immOiHandle,
    SaInvocationT invocation,
    const SaNameT *objectName,
    SaImmAdminOperationIdT opId,
    const SaImmAdminOperationParamsT_2 **params)
{
    TRACE_ENTER2("Returning %s\n", get_saf_error(saImmOiAdminOperationCallback_response));
    assert(opId == operationId);
    assert(*((SaUint64T*) params[0]->paramBuffer) == value);

    if (saImmOiAdminOperationCallback_response != SA_AIS_ERR_TIMEOUT)
        safassert(saImmOiAdminOperationResult(immOiHandle, invocation,
            saImmOiAdminOperationCallback_response), SA_AIS_OK);
    TRACE_LEAVE();
}

/*
 * If opId is SA_IMM_PARAM_ADMOP_ID_ESC, then the callback returns SA_AIS_ERR_BAD_OPERATION
 * and set the admin-operation name to return parameter SA_IMM_PARAM_ADMOP_ERROR.
 * If adminOperationErrorString is not NULL, then the return parameter is adminOperationErrorParams.
 * Otherwise, the return parameter is params.
 */
static void saImmOiAdminOperationCallback_o2(SaImmOiHandleT immOiHandle,
    SaInvocationT invocation,
    const SaNameT *objectName,
    SaImmAdminOperationIdT opId,
    const SaImmAdminOperationParamsT_2 **params)
{   
    TRACE_ENTER2("Returning %s\n", get_saf_error(saImmOiAdminOperationCallback_response));

    if(opId == SA_IMM_PARAM_ADMOP_ID_ESC) {
    	int ix = 0;
        /* skip SaImm* parameters */
        SaImmAdminOperationParamsT_2 *param = NULL;
        while(params[ix]) {
            if(!strcmp(params[ix]->paramName, SA_IMM_PARAM_ADMOP_NAME)) {
            	TRACE("Admin operation name: %s", params[ix]->paramName);
            } else if(!param) {
            	param = (SaImmAdminOperationParamsT_2 *)params[ix];
            }
            ix++;
        }

        /* SaImmAdminOperationName */
        if(param) {
            adminOperationErrorString = (SaStringT)(param->paramBuffer);
            safassert(saImmOiAdminOperationResult_o2(immOiHandle, invocation,
                SA_AIS_ERR_BAD_OPERATION, adminOperationErrorParams), SA_AIS_OK);
            adminOperationErrorString = NULL;
        } else {
            adminOperationErrorString = "Cannot find admin-operation name";
            safassert(saImmOiAdminOperationResult_o2(immOiHandle, invocation,
                SA_AIS_ERR_INVALID_PARAM, adminOperationErrorParams), SA_AIS_OK);
            adminOperationErrorString = NULL;
        }
    } else if (saImmOiAdminOperationCallback_response != SA_AIS_ERR_TIMEOUT) {
        if(adminOperationErrorString) {
        	/* SaImmAdminOperationError */
            safassert(saImmOiAdminOperationResult_o2(immOiHandle, invocation,
                saImmOiAdminOperationCallback_response, adminOperationErrorParams), SA_AIS_OK);
        } else {
            assert(opId == operationId);
            assert(*((SaUint64T*) params[0]->paramBuffer) == value);

            safassert(saImmOiAdminOperationResult_o2(immOiHandle, invocation,
                saImmOiAdminOperationCallback_response, params), SA_AIS_OK);
        }
    }
    TRACE_LEAVE2();
}

static void saImmOiAdminOperationCallback_o2_copyParams(SaImmOiHandleT immOiHandle,
    SaInvocationT invocation,
    const SaNameT *objectName,
    SaImmAdminOperationIdT opId,
    const SaImmAdminOperationParamsT_2 **params)
{
    saImmOiAdminOperationResult_o2(immOiHandle, invocation, SA_AIS_OK, params);
}

static const SaImmOiCallbacksT_2 oiCallbacks =
    {.saImmOiAdminOperationCallback = saImmOiAdminOperationCallback};
static const SaImmOiCallbacksT_2 oiCallbacks_o2 =
    {.saImmOiAdminOperationCallback = saImmOiAdminOperationCallback_o2};
static const SaImmOiCallbacksT_2 oiCallbacks_o2_copyParams =
    {.saImmOiAdminOperationCallback = saImmOiAdminOperationCallback_o2_copyParams};

static const SaImmOiCallbacksT_2 *callbacks = &oiCallbacks_o2;

static void saImmOmAdminOperationInvokeCallback(
    SaInvocationT invocation,
    SaAisErrorT opRetVal,
    SaAisErrorT error,
    const SaImmAdminOperationParamsT_2 **returnParams)
{
	int ix = 0;
    TRACE_ENTER2("%llu (0x%llx), %u, %u\n", invocation, invocation, opRetVal, error);
    assert(invocation == userInvocation);
    operationReturnValue = opRetVal;
    immReturnValue = error;

    /* skip SaImm* attributes */
    while(returnParams[ix]) {
    	if(strncmp(returnParams[ix]->paramName, "SaImm", 5))
    		break;
    	ix++;
    }
    assert(returnParams[ix]);

    TRACE("saImmOmAdminOperationInvokeCallback return param:%llx\n",
	    *((SaUint64T*) returnParams[ix]->paramBuffer));
    assert(*((SaUint64T*) returnParams[ix]->paramBuffer) == value);
    TRACE_LEAVE();
}

static const SaImmCallbacksT_o2 omCallbacks = {saImmOmAdminOperationInvokeCallback};

static const SaImmAdminOwnerNameT adminOwnerName = (SaImmAdminOwnerNameT) __FILE__;


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
    const SaImmOiImplementerNameT implementerName = (SaImmOiImplementerNameT) __FUNCTION__;
    SaSelectionObjectT selObj;
    SaImmHandleT handle;
    const SaNameT *objectName = arg;

    TRACE_ENTER();
    safassert(saImmOiInitialize_2(&handle, callbacks, &immVersion), SA_AIS_OK);
    safassert(saImmOiImplementerSet(handle, implementerName), SA_AIS_OK);
    TRACE("Setting implementer %s for object %s", implementerName, objectName->value);
    safassert(saImmOiObjectImplementerSet(handle, objectName, SA_IMM_ONE), SA_AIS_OK);
    safassert(saImmOiSelectionObjectGet(handle, &selObj), SA_AIS_OK);
    objectImplementerIsSet = SA_TRUE;

    fds[0].fd = (int) selObj;
    fds[0].events = POLLIN;

    ret = poll(fds, 1, 1000);
    if (ret == 0)
    {
        TRACE("poll timeout\n");
        goto done;
    }
    if (ret == -1)
    {
        fprintf(stderr, "poll error: %s\n", strerror(errno));
        goto done;
    }

    safassert(saImmOiDispatch(handle, SA_DISPATCH_ONE), SA_AIS_OK);

 done:
    safassert(saImmOiObjectImplementerRelease(handle, objectName, SA_IMM_ONE), SA_AIS_OK);
    safassert(saImmOiFinalize(handle), SA_AIS_OK);

    TRACE_LEAVE();
    return NULL;
}

static SaAisErrorT om_admin_exec(SaAisErrorT *imm_rc, const SaNameT *objectName, SaAisErrorT in_rc)
{
    SaAisErrorT rc;
    SaImmHandleT handle;
    SaImmAdminOwnerHandleT ownerHandle;
    const SaNameT *objectNames[] = {objectName, NULL};
    SaImmAdminOperationParamsT_2 param = {
        "TEST",
        SA_IMM_ATTR_SAUINT64T,
        &value
    };
    const SaImmAdminOperationParamsT_2 *params[] = {&param, NULL};

    if (in_rc == SA_AIS_ERR_INVALID_PARAM)
        param.paramType = -1;

    TRACE_ENTER();
    safassert(saImmOmInitialize(&handle, NULL, &immVersion), SA_AIS_OK);
    safassert(saImmOmAdminOwnerInitialize(handle, adminOwnerName, SA_TRUE, &ownerHandle), SA_AIS_OK);
    safassert(saImmOmAdminOwnerSet(ownerHandle, objectNames, SA_IMM_SUBTREE), SA_AIS_OK);

    *imm_rc = saImmOmAdminOperationInvoke_2(
        ownerHandle, &rdn, 0, operationId,
        params, &rc, SA_TIME_ONE_SECOND);

    saImmOmAdminOwnerFinalize(ownerHandle); /* Tend to get timeout here with PBE */
    saImmOmFinalize(handle);

    TRACE_LEAVE();
    return rc;
}

void SaImmOiAdminOperation_01(void)
{
    int ret;
    pthread_t thread;
    SaAisErrorT imm_rc;

    TRACE_ENTER();
    SaAisErrorT rc;

    SaImmAdminOwnerHandleT ownerHandle;
    SaImmCcbHandleT ccbHandle;
    const SaNameT* nameValues[] = {&rdn, NULL};
    SaImmAttrValuesT_2 v2 = {"rdn",  SA_IMM_ATTR_SANAMET, 1, (void**)nameValues};
    SaUint32T  int1Value1 = 7;
    SaUint32T* int1Values[] = {&int1Value1};
    SaImmAttrValuesT_2 v1 = {"attr1", SA_IMM_ATTR_SAUINT32T, 1, (void**)int1Values};
    const SaImmAttrValuesT_2 * attrValues[] = {&v1, &v2, NULL};
    safassert(saImmOmInitialize(&immOmHandle, NULL, &immVersion), SA_AIS_OK);
    safassert(saImmOmAdminOwnerInitialize(immOmHandle, adminOwnerName, SA_TRUE, &ownerHandle), SA_AIS_OK);
    safassert(saImmOmCcbInitialize(ownerHandle, 0, &ccbHandle), SA_AIS_OK);
    /* Create test object under root */
    rc = saImmOmCcbObjectCreate_2(ccbHandle, "TestClassConfig", NULL, attrValues);
    if(rc != SA_AIS_OK && rc != SA_AIS_ERR_EXIST) {
        /* Do the create again to provoke safassert */
        safassert(saImmOmCcbObjectCreate_2(ccbHandle, "TestClassConfig", NULL, attrValues), SA_AIS_OK);
    }

    if(rc == SA_AIS_OK) {
        safassert(saImmOmCcbApply(ccbHandle), SA_AIS_OK);
    } else {
        assert(rc == SA_AIS_ERR_EXIST);
        safassert(saImmOmAdminOwnerSet(ownerHandle, nameValues, SA_IMM_ONE), SA_AIS_OK);
    }

    /* Set admin operation callback to use saImmOiAdminOperationResult */
    callbacks = &oiCallbacks;
    objectImplementerIsSet = SA_FALSE;
    ret = pthread_create(&thread, NULL, objectImplementerThreadMain, &rdn);
    assert(ret == 0);

    while (!objectImplementerIsSet)
        usleep(100);

    rc = om_admin_exec(&imm_rc, &rdn, SA_AIS_OK);

    pthread_join(thread, NULL);

    if (imm_rc != SA_AIS_OK)
        TRACE("om_admin_exec failed: %s\n", get_saf_error(imm_rc));

    /* set callbacks to default callbacks */
    callbacks = &oiCallbacks_o2;

    safassert(saImmOmCcbObjectDelete(ccbHandle, &rdn), SA_AIS_OK);
    safassert(saImmOmCcbApply(ccbHandle), SA_AIS_OK);
    safassert(saImmOmCcbFinalize(ccbHandle), SA_AIS_OK);
    safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);

    assert(imm_rc == SA_AIS_OK);
    test_validate(rc, SA_AIS_OK);
    TRACE_LEAVE();
}

void SaImmOiAdminOperation_02(void)
{
    SaAisErrorT operationReturnValue;
    SaImmHandleT handle;
    SaImmAdminOwnerHandleT ownerHandle;
    const SaNameT *objectNames[] = {&rdn, NULL};

    SaImmAdminOperationParamsT_2 param = {
        "TEST",
        SA_IMM_ATTR_SAUINT64T,
        &value
    };
    const SaImmAdminOperationParamsT_2 *params[] = {&param, NULL};

    TRACE_ENTER();
    safassert(saImmOmInitialize(&handle, NULL, &immVersion), SA_AIS_OK);
    safassert(saImmOmAdminOwnerInitialize(handle, adminOwnerName, SA_TRUE, &ownerHandle), SA_AIS_OK);

    SaImmCcbHandleT ccbHandle;
    const SaNameT* nameValues[] = {&rdn};
    SaImmAttrValuesT_2 v2 = {"rdn",  SA_IMM_ATTR_SANAMET, 1, (void**)nameValues};
    SaUint32T  int1Value1 = 7;
    SaUint32T* int1Values[] = {&int1Value1};
    SaImmAttrValuesT_2 v1 = {"attr1", SA_IMM_ATTR_SAUINT32T, 1, (void**)int1Values};
    const SaImmAttrValuesT_2 * attrValues[] = {&v1, &v2, NULL};
    safassert(saImmOmCcbInitialize(ownerHandle, 0, &ccbHandle), SA_AIS_OK);
    /* Create test object under root */
    safassert(saImmOmCcbObjectCreate_2(ccbHandle, "TestClassConfig", NULL, attrValues), SA_AIS_OK);
    safassert(saImmOmCcbApply(ccbHandle), SA_AIS_OK);

    rc = saImmOmAdminOperationInvoke_2(
        -1, &rdn, 0, operationId,
        params, &operationReturnValue, SA_TIME_ONE_SECOND);
    if (rc != SA_AIS_ERR_BAD_HANDLE)
        goto done;

    safassert(saImmOmAdminOwnerSet(ownerHandle, objectNames, SA_IMM_SUBTREE), SA_AIS_OK);
    safassert(saImmOmAdminOwnerFinalize(ownerHandle), SA_AIS_OK);
    rc = saImmOmAdminOperationInvoke_2(
        ownerHandle, &rdn, 0, operationId,
        params, &operationReturnValue, SA_TIME_ONE_SECOND);

done:
    safassert(saImmOmAdminOwnerInitialize(handle, adminOwnerName, SA_TRUE, &ownerHandle), SA_AIS_OK);
    safassert(saImmOmAdminOwnerSet(ownerHandle, objectNames, SA_IMM_SUBTREE), SA_AIS_OK);
    safassert(saImmOmCcbInitialize(ownerHandle, 0, &ccbHandle), SA_AIS_OK);
    safassert(saImmOmCcbObjectDelete(ccbHandle, &rdn), SA_AIS_OK);
    safassert(saImmOmCcbApply(ccbHandle), SA_AIS_OK);
    safassert(saImmOmCcbFinalize(ccbHandle), SA_AIS_OK);
    safassert(saImmOmFinalize(handle), SA_AIS_OK);
    test_validate(rc, SA_AIS_ERR_BAD_HANDLE);
    TRACE_LEAVE();
}

void SaImmOiAdminOperation_03(void)
{
    int ret;
    pthread_t thread;
    SaAisErrorT imm_rc;

    TRACE_ENTER();
    SaImmAdminOwnerHandleT ownerHandle;
    SaImmCcbHandleT ccbHandle;
    const SaNameT* nameValues[] = {&rdn};
    SaImmAttrValuesT_2 v2 = {"rdn",  SA_IMM_ATTR_SANAMET, 1, (void**)nameValues};
    SaUint32T  int1Value1 = 7;
    SaUint32T* int1Values[] = {&int1Value1};
    SaImmAttrValuesT_2 v1 = {"attr1", SA_IMM_ATTR_SAUINT32T, 1, (void**)int1Values};
    const SaImmAttrValuesT_2 * attrValues[] = {&v1, &v2, NULL};
    safassert(saImmOmInitialize(&immOmHandle, NULL, &immVersion), SA_AIS_OK);
    safassert(saImmOmAdminOwnerInitialize(immOmHandle, adminOwnerName, SA_TRUE, &ownerHandle), SA_AIS_OK);
    safassert(saImmOmCcbInitialize(ownerHandle, 0, &ccbHandle), SA_AIS_OK);
    /* Create test object under root */
    safassert(saImmOmCcbObjectCreate_2(ccbHandle, "TestClassConfig", NULL, attrValues), SA_AIS_OK);
    safassert(saImmOmCcbApply(ccbHandle), SA_AIS_OK);

    (void) om_admin_exec(&imm_rc, &rdn, SA_AIS_ERR_INVALID_PARAM);
    if (imm_rc != SA_AIS_ERR_INVALID_PARAM)
        goto done;

    objectImplementerIsSet = SA_FALSE;
    ret = pthread_create(&thread, NULL, objectImplementerThreadMain, &rdn);
    assert(ret == 0);

    while (!objectImplementerIsSet)
        usleep(100);

    saImmOiAdminOperationCallback_response = SA_AIS_ERR_INVALID_PARAM;
    rc = om_admin_exec(&imm_rc, &rdn, SA_AIS_OK);

    pthread_join(thread, NULL);

    if (imm_rc != SA_AIS_OK)
    {
        TRACE("om_admin_exec failed: %s\n", get_saf_error(imm_rc));
        goto done;
    }

done:
    safassert(saImmOmCcbObjectDelete(ccbHandle, &rdn), SA_AIS_OK);
    safassert(saImmOmCcbApply(ccbHandle), SA_AIS_OK);
    safassert(saImmOmCcbFinalize(ccbHandle), SA_AIS_OK);
    safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);

    test_validate(rc, SA_AIS_ERR_INVALID_PARAM);
    saImmOiAdminOperationCallback_response = SA_AIS_OK;
    TRACE_LEAVE();
}

void SaImmOiAdminOperation_04(void)
{
    int ret;
    pthread_t thread;
    SaAisErrorT rc;
    SaImmHandleT handle;
    SaImmAdminOwnerHandleT ownerHandle;
    SaImmCcbHandleT ccbHandle;
    SaImmAdminOperationParamsT_2 param = {
        "TEST",
        SA_IMM_ATTR_SAUINT64T,
        &value
    };
    const SaImmAdminOperationParamsT_2 *params[] = {&param, NULL};

    TRACE_ENTER2("BEGIN TEST");
    safassert(saImmOmInitialize(&handle, NULL, &immVersion), SA_AIS_OK);
    safassert(saImmOmAdminOwnerInitialize(handle, adminOwnerName, SA_TRUE, &ownerHandle), SA_AIS_OK);
    const SaNameT* nameValues[] = {&rdn, NULL};
    SaImmAttrValuesT_2 v2 = {"rdn",  SA_IMM_ATTR_SANAMET, 1, (void**)nameValues};
    SaUint32T  int1Value1 = 7;
    SaUint32T* int1Values[] = {&int1Value1};
    SaImmAttrValuesT_2 v1 = {"attr1", SA_IMM_ATTR_SAUINT32T, 1, (void**)int1Values};
    const SaImmAttrValuesT_2 * attrValues[] = {&v1, &v2, NULL};
    safassert(saImmOmCcbInitialize(ownerHandle, 0, &ccbHandle), SA_AIS_OK);
    /* Create test object under root */
    safassert(saImmOmCcbObjectCreate_2(ccbHandle, "TestClassConfig", NULL, attrValues), SA_AIS_OK);
    safassert(saImmOmCcbApply(ccbHandle), SA_AIS_OK);
    safassert(saImmOmAdminOwnerRelease(ownerHandle, nameValues, SA_IMM_ONE), SA_AIS_OK);

    objectImplementerIsSet = SA_FALSE;
    ret = pthread_create(&thread, NULL, objectImplementerThreadMain, &rdn);
    assert(ret == 0);

    while (!objectImplementerIsSet)
        usleep(100);

    TRACE("Invoking Admin Operation");
    rc = saImmOmAdminOperationInvoke_2(
        ownerHandle, &rdn, 0, operationId,
        params, &rc, SA_TIME_ONE_SECOND);

    pthread_join(thread, NULL);

    TRACE("Admin Operation invoked:%u EXPECTED %u", rc, SA_AIS_ERR_BAD_OPERATION);
    safassert(saImmOmAdminOwnerSet(ownerHandle, nameValues, SA_IMM_ONE), SA_AIS_OK);
    safassert(saImmOmCcbObjectDelete(ccbHandle, &rdn), SA_AIS_OK);
    safassert(saImmOmCcbApply(ccbHandle), SA_AIS_OK);
    safassert(saImmOmCcbFinalize(ccbHandle), SA_AIS_OK);

    safassert(saImmOmAdminOwnerFinalize(ownerHandle), SA_AIS_OK);
    safassert(saImmOmFinalize(handle), SA_AIS_OK);
    test_validate(rc, SA_AIS_ERR_BAD_OPERATION);
    TRACE_LEAVE();
}

void SaImmOiAdminOperation_05(void)
{
    int ret;
    pthread_t thread;
    SaAisErrorT imm_rc;

    SaImmHandleT handle;
    SaImmAdminOwnerHandleT ownerHandle;
    SaImmCcbHandleT ccbHandle;
    TRACE_ENTER();
    safassert(saImmOmInitialize(&handle, NULL, &immVersion), SA_AIS_OK);
    safassert(saImmOmAdminOwnerInitialize(handle, adminOwnerName, SA_TRUE, &ownerHandle), SA_AIS_OK);
    const SaNameT* nameValues[] = {&rdn, NULL};
    SaImmAttrValuesT_2 v2 = {"rdn",  SA_IMM_ATTR_SANAMET, 1, (void**)nameValues};
    SaUint32T  int1Value1 = 7;
    SaUint32T* int1Values[] = {&int1Value1};
    SaImmAttrValuesT_2 v1 = {"attr1", SA_IMM_ATTR_SAUINT32T, 1, (void**)int1Values};
    const SaImmAttrValuesT_2 * attrValues[] = {&v1, &v2, NULL};
    safassert(saImmOmCcbInitialize(ownerHandle, 0, &ccbHandle), SA_AIS_OK);
    /* Create test object under root */
    safassert(saImmOmCcbObjectCreate_2(ccbHandle, "TestClassConfig", NULL, attrValues), SA_AIS_OK);
    safassert(saImmOmCcbApply(ccbHandle), SA_AIS_OK);

    objectImplementerIsSet = SA_FALSE;
    ret = pthread_create(&thread, NULL, objectImplementerThreadMain, &rdn);
    assert(ret == 0);

    while (!objectImplementerIsSet)
        usleep(100);

    saImmOiAdminOperationCallback_response = SA_AIS_ERR_TIMEOUT;
    rc = om_admin_exec(&imm_rc, &rdn, SA_AIS_OK);

    pthread_join(thread, NULL);

    if (imm_rc != SA_AIS_ERR_TIMEOUT)
        TRACE("om_admin_exec failed: %s\n", get_saf_error(imm_rc));

    safassert(saImmOmCcbObjectDelete(ccbHandle, &rdn), SA_AIS_OK);
    safassert(saImmOmCcbApply(ccbHandle), SA_AIS_OK);
    safassert(saImmOmCcbFinalize(ccbHandle), SA_AIS_OK);

    safassert(saImmOmAdminOwnerFinalize(ownerHandle), SA_AIS_OK);
    safassert(saImmOmFinalize(handle), SA_AIS_OK);

    test_validate(imm_rc, SA_AIS_ERR_TIMEOUT);
    saImmOiAdminOperationCallback_response = SA_AIS_OK;
    
    TRACE_LEAVE();
}

void SaImmOiAdminOperation_06(void)
{
    SaAisErrorT imm_rc;

    SaImmHandleT handle;
    SaImmAdminOwnerHandleT ownerHandle;
    SaImmCcbHandleT ccbHandle;
    TRACE_ENTER();
    safassert(saImmOmInitialize(&handle, NULL, &immVersion), SA_AIS_OK);
    safassert(saImmOmAdminOwnerInitialize(handle, adminOwnerName, SA_TRUE, &ownerHandle), SA_AIS_OK);

    const SaNameT* nameValues[] = {&rdn, NULL};
    SaImmAttrValuesT_2 v2 = {"rdn",  SA_IMM_ATTR_SANAMET, 1, (void**)nameValues};
    SaUint32T  int1Value1 = 7;
    SaUint32T* int1Values[] = {&int1Value1};
    SaImmAttrValuesT_2 v1 = {"attr1", SA_IMM_ATTR_SAUINT32T, 1, (void**)int1Values};
    const SaImmAttrValuesT_2 * attrValues[] = {&v1, &v2, NULL};

    safassert(saImmOmCcbInitialize(ownerHandle, 0, &ccbHandle), SA_AIS_OK);
    /* Create test object under root */
    safassert(saImmOmCcbObjectCreate_2(ccbHandle, "TestClassConfig", NULL, attrValues), SA_AIS_OK);
    safassert(saImmOmCcbApply(ccbHandle), SA_AIS_OK);

    rc = om_admin_exec(&imm_rc, &rdn, SA_AIS_OK);

    safassert(saImmOmCcbObjectDelete(ccbHandle, &rdn), SA_AIS_OK);
    safassert(saImmOmCcbApply(ccbHandle), SA_AIS_OK);
    safassert(saImmOmCcbFinalize(ccbHandle), SA_AIS_OK);

    safassert(saImmOmAdminOwnerFinalize(ownerHandle), SA_AIS_OK);
    safassert(saImmOmFinalize(handle), SA_AIS_OK);

    test_validate(imm_rc, SA_AIS_ERR_NOT_EXIST);
    TRACE_LEAVE();
}

void SaImmOiAdminOperation_07(void)
{
    struct pollfd fds[1];
    int ret;
    pthread_t thread;
    SaImmHandleT handle;
    SaImmAdminOwnerHandleT ownerHandle;
    SaImmCcbHandleT ccbHandle;

    const SaNameT *nameValues[] = {&rdn, NULL};
    SaSelectionObjectT selObj;
    SaImmAdminOperationParamsT_2 param = {
        "TEST",
        SA_IMM_ATTR_SAUINT64T,
        &value
    };
    const SaImmAdminOperationParamsT_2 *params[] = {&param, NULL};

    TRACE_ENTER();
    safassert(saImmOmInitialize_o2(&handle, &omCallbacks, &immVersion), SA_AIS_OK);
    safassert(saImmOmAdminOwnerInitialize(handle, adminOwnerName, SA_TRUE, &ownerHandle), SA_AIS_OK);

    SaImmAttrValuesT_2 v2 = {"rdn",  SA_IMM_ATTR_SANAMET, 1, (void**)nameValues};
    SaUint32T  int1Value1 = 7;
    SaUint32T* int1Values[] = {&int1Value1};
    SaImmAttrValuesT_2 v1 = {"attr1", SA_IMM_ATTR_SAUINT32T, 1, (void**)int1Values};
    const SaImmAttrValuesT_2 * attrValues[] = {&v1, &v2, NULL};

    safassert(saImmOmCcbInitialize(ownerHandle, 0, &ccbHandle), SA_AIS_OK);
    safassert(saImmOmCcbObjectCreate_2(ccbHandle, "TestClassConfig", NULL, attrValues), SA_AIS_OK);
    safassert(saImmOmCcbApply(ccbHandle), SA_AIS_OK);

    objectImplementerIsSet = SA_FALSE;
    ret = pthread_create(&thread, NULL, objectImplementerThreadMain, &rdn);
    assert(ret == 0);
    while (!objectImplementerIsSet)
        usleep(100);

    safassert(saImmOmSelectionObjectGet(handle, &selObj), SA_AIS_OK);
    safassert(saImmOmAdminOwnerSet(ownerHandle, nameValues, SA_IMM_SUBTREE), SA_AIS_OK);
    safassert(saImmOmAdminOperationInvokeAsync_2(
        ownerHandle, userInvocation, &rdn, 0, operationId, params), SA_AIS_OK);

    fds[0].fd = (int) selObj;
    fds[0].events = POLLIN;

    ret = poll(fds, 1, 1000);
    if (ret == 0)
    {
        TRACE("poll timeout\n");
        goto done;
    }
    if (ret == -1)
    {
        fprintf(stderr, "poll error: %s\n", strerror(errno));
        goto done;
    }

    immReturnValue = SA_AIS_ERR_FAILED_OPERATION;
    safassert(saImmOmDispatch(handle, SA_DISPATCH_ONE), SA_AIS_OK);

done:
    pthread_join(thread, NULL);

    safassert(saImmOmCcbObjectDelete(ccbHandle, &rdn), SA_AIS_OK);
    safassert(saImmOmCcbApply(ccbHandle), SA_AIS_OK);
    safassert(saImmOmCcbFinalize(ccbHandle), SA_AIS_OK);

    safassert(saImmOmAdminOwnerFinalize(ownerHandle), SA_AIS_OK);
    safassert(saImmOmFinalize(handle), SA_AIS_OK);

    TRACE("%s", get_saf_error(immReturnValue));
    assert(immReturnValue == SA_AIS_OK);
    test_validate(operationReturnValue, SA_AIS_OK);
    TRACE_LEAVE();
}

void SaImmOiAdminOperation_08(void)
{
    int ret;
    pthread_t thread;
    SaImmHandleT handle;
    SaImmAdminOwnerHandleT ownerHandle;
    SaImmCcbHandleT ccbHandle;

    const SaNameT *nameValues[] = {&rdn, NULL};
    SaSelectionObjectT selObj;
    SaImmAdminOperationParamsT_2 param = {
        "TEST",
        SA_IMM_ATTR_SAUINT64T,
        &value
    };
    const SaImmAdminOperationParamsT_2 *params[] = {&param, NULL};
    SaImmAdminOperationParamsT_2 **returnParams = NULL;
    SaAisErrorT operationReturnValue;

    TRACE_ENTER();
    SaImmAttrValuesT_2 v2 = {"rdn",  SA_IMM_ATTR_SANAMET, 1, (void**)nameValues};
    SaUint32T  int1Value1 = 7;
    SaUint32T* int1Values[] = {&int1Value1};
    SaImmAttrValuesT_2 v1 = {"attr1", SA_IMM_ATTR_SAUINT32T, 1, (void**)int1Values};
    const SaImmAttrValuesT_2 * attrValues[] = {&v1, &v2, NULL};

    safassert(saImmOmInitialize_o2(&handle, &omCallbacks, &immVersion), SA_AIS_OK);
    safassert(saImmOmAdminOwnerInitialize(handle, adminOwnerName, SA_TRUE, &ownerHandle), SA_AIS_OK);

    safassert(saImmOmCcbInitialize(ownerHandle, 0, &ccbHandle), SA_AIS_OK);
    safassert(saImmOmCcbObjectCreate_2(ccbHandle, "TestClassConfig", NULL, attrValues), SA_AIS_OK);
    safassert(saImmOmCcbApply(ccbHandle), SA_AIS_OK);

    objectImplementerIsSet = SA_FALSE;
    ret = pthread_create(&thread, NULL, objectImplementerThreadMain, &rdn);
    assert(ret == 0);
    while (!objectImplementerIsSet)
        usleep(100);

    safassert(saImmOmSelectionObjectGet(handle, &selObj), SA_AIS_OK);
    safassert(saImmOmAdminOwnerSet(ownerHandle, nameValues, SA_IMM_SUBTREE), SA_AIS_OK);

    if((rc = saImmOmAdminOperationInvoke_o2(ownerHandle, &rdn, 0, operationId, params,
    		&operationReturnValue, SA_TIME_MAX, &returnParams)) != SA_AIS_OK)
    	goto done;

    assert(returnParams != NULL);

    rc = saImmOmAdminOperationMemoryFree(ownerHandle, returnParams);

done:
    TRACE("%s", get_saf_error(rc));
    test_validate(rc, SA_AIS_OK);

    pthread_join(thread, NULL);

    safassert(saImmOmCcbObjectDelete(ccbHandle, &rdn), SA_AIS_OK);
    safassert(saImmOmCcbApply(ccbHandle), SA_AIS_OK);
    safassert(saImmOmCcbFinalize(ccbHandle), SA_AIS_OK);

    safassert(saImmOmAdminOwnerFinalize(ownerHandle), SA_AIS_OK);
    safassert(saImmOmFinalize(handle), SA_AIS_OK);

    TRACE_LEAVE();
}

void SaImmOiAdminOperation_09(void)
{
    int ret;
    pthread_t thread;
    SaImmHandleT handle;
    SaImmAdminOwnerHandleT ownerHandle;
    SaImmCcbHandleT ccbHandle;
    int ix = 0;

    const SaNameT *nameValues[] = {&rdn, NULL};
    SaSelectionObjectT selObj;
    SaImmAdminOperationParamsT_2 param = {
        "TEST",
        SA_IMM_ATTR_SAUINT64T,
        &value
    };
    const SaImmAdminOperationParamsT_2 *params[] = {&param, NULL};
    SaImmAdminOperationParamsT_2 **returnParams = NULL;
    SaAisErrorT operationReturnValue;

    TRACE_ENTER();
    SaImmAttrValuesT_2 v2 = {"rdn",  SA_IMM_ATTR_SANAMET, 1, (void**)nameValues};
    SaUint32T  int1Value1 = 7;
    SaUint32T* int1Values[] = {&int1Value1};
    SaImmAttrValuesT_2 v1 = {"attr1", SA_IMM_ATTR_SAUINT32T, 1, (void**)int1Values};
    const SaImmAttrValuesT_2 * attrValues[] = {&v1, &v2, NULL};

    safassert(saImmOmInitialize_o2(&handle, &omCallbacks, &immVersion), SA_AIS_OK);
    safassert(saImmOmAdminOwnerInitialize(handle, adminOwnerName, SA_TRUE, &ownerHandle), SA_AIS_OK);

    safassert(saImmOmCcbInitialize(ownerHandle, 0, &ccbHandle), SA_AIS_OK);
    safassert(saImmOmCcbObjectCreate_2(ccbHandle, "TestClassConfig", NULL, attrValues), SA_AIS_OK);
    safassert(saImmOmCcbApply(ccbHandle), SA_AIS_OK);

    objectImplementerIsSet = SA_FALSE;
    ret = pthread_create(&thread, NULL, objectImplementerThreadMain, &rdn);
    assert(ret == 0);
    while (!objectImplementerIsSet)
        usleep(100);

    safassert(saImmOmSelectionObjectGet(handle, &selObj), SA_AIS_OK);
    safassert(saImmOmAdminOwnerSet(ownerHandle, nameValues, SA_IMM_SUBTREE), SA_AIS_OK);

    /* Expected return error string */
    saImmOiAdminOperationCallback_response = SA_AIS_ERR_BAD_OPERATION;
    adminOperationErrorString = "saImmOiAdminOperationCallback_response is set to SA_AIS_ERR_BAD_OPERATION";
    safassert(saImmOmAdminOperationInvoke_o2(
        ownerHandle, &rdn, 0, operationId, params, &operationReturnValue, SA_TIME_MAX, &returnParams), SA_AIS_OK);
    safassert(operationReturnValue, saImmOiAdminOperationCallback_response);
    saImmOiAdminOperationCallback_response = SA_AIS_OK;

    /* Check return parameters */
    assert(returnParams != NULL);
    assert(*returnParams != NULL);

    /* Find SA_IMM_PARAM_ADMOP_ERROR parameter */
    while(returnParams[ix]) {
    	if(!strcmp(returnParams[ix]->paramName, SA_IMM_PARAM_ADMOP_ERROR))
    		break;
    	ix++;
    }
    assert(returnParams[ix] != NULL);

    /* Not exactly nice validation because test_validate validates SaAisErrorT errors.
       Here test_validate is used to track test case counters, and it's possible to use it in the way below */
    test_validate(strcmp(*(SaStringT *)(returnParams[ix]->paramBuffer), adminOperationErrorString), 0);

    adminOperationErrorString = NULL;

    safassert(saImmOmDispatch(handle, SA_DISPATCH_ONE), SA_AIS_OK);

    safassert(saImmOmAdminOperationMemoryFree(ownerHandle, returnParams), SA_AIS_OK);

    pthread_join(thread, NULL);

    safassert(saImmOmCcbObjectDelete(ccbHandle, &rdn), SA_AIS_OK);
    safassert(saImmOmCcbApply(ccbHandle), SA_AIS_OK);
    safassert(saImmOmCcbFinalize(ccbHandle), SA_AIS_OK);

    safassert(saImmOmAdminOwnerFinalize(ownerHandle), SA_AIS_OK);
    safassert(saImmOmFinalize(handle), SA_AIS_OK);

    TRACE_LEAVE();
}

void SaImmOiAdminOperation_10(void)
{
    int ix = 0;
    int ret;
    pthread_t thread;
    SaImmHandleT handle;
    SaImmAdminOwnerHandleT ownerHandle;
    SaImmCcbHandleT ccbHandle;

    const SaNameT *nameValues[] = {&rdn, NULL};
    SaImmAdminOperationParamsT_2 param = {
        "TEST",
        SA_IMM_ATTR_SAUINT64T,
        &value
    };
    /* Set admin-operation as a second parameter */
    const SaImmAdminOperationParamsT_2 *params[] = {&param, &adminOperationNameParam, NULL};
    SaImmAdminOperationParamsT_2 **returnParams = NULL;
    SaAisErrorT operationReturnValue;

    TRACE_ENTER();
    SaImmAttrValuesT_2 v2 = {"rdn",  SA_IMM_ATTR_SANAMET, 1, (void**)nameValues};
    SaUint32T  int1Value1 = 7;
    SaUint32T* int1Values[] = {&int1Value1};
    SaImmAttrValuesT_2 v1 = {"attr1", SA_IMM_ATTR_SAUINT32T, 1, (void**)int1Values};
    const SaImmAttrValuesT_2 * attrValues[] = {&v1, &v2, NULL};

    safassert(saImmOmInitialize_o2(&handle, &omCallbacks, &immVersion), SA_AIS_OK);
    safassert(saImmOmAdminOwnerInitialize(handle, adminOwnerName, SA_TRUE, &ownerHandle), SA_AIS_OK);

    safassert(saImmOmCcbInitialize(ownerHandle, 0, &ccbHandle), SA_AIS_OK);
    safassert(saImmOmCcbObjectCreate_2(ccbHandle, "TestClassConfig", NULL, attrValues), SA_AIS_OK);
    safassert(saImmOmCcbApply(ccbHandle), SA_AIS_OK);

    objectImplementerIsSet = SA_FALSE;
    ret = pthread_create(&thread, NULL, objectImplementerThreadMain, &rdn);
    assert(ret == 0);
    while (!objectImplementerIsSet)
        usleep(100);

    safassert(saImmOmAdminOwnerSet(ownerHandle, nameValues, SA_IMM_SUBTREE), SA_AIS_OK);

    saImmOiAdminOperationCallback_response = SA_AIS_ERR_BAD_OPERATION;
    adminOperationName = "TestOperation";
    safassert(saImmOmAdminOperationInvoke_o2(
        ownerHandle, &rdn, 0, SA_IMM_PARAM_ADMOP_ID_ESC, params, &operationReturnValue, SA_TIME_MAX, &returnParams), SA_AIS_OK);
    safassert(operationReturnValue, saImmOiAdminOperationCallback_response);
    saImmOiAdminOperationCallback_response = SA_AIS_OK;
    adminOperationName = NULL;

    /* Check return parameters */
    assert(returnParams != NULL);
    assert(*returnParams != NULL);

    /* Find SA_IMM_PARAM_ADMOP_ERROR parameter */
    while(returnParams[ix]) {
    	if(!strcmp(returnParams[ix]->paramName, SA_IMM_PARAM_ADMOP_ERROR))
    		break;
    	ix++;
    }
    assert(returnParams[ix] != NULL);

    /* Not exactly nice validation because test_validate validates SaAisErrorT errors.
       Here test_validate is used to track test case counters, and is possible to use it in the way below */
    test_validate(strcmp(*(SaStringT *)(returnParams[ix]->paramBuffer), (char *)&value), 0);

    adminOperationErrorString = NULL;

    safassert(saImmOmDispatch(handle, SA_DISPATCH_ONE), SA_AIS_OK);

    safassert(saImmOmAdminOperationMemoryFree(ownerHandle, returnParams), SA_AIS_OK);

    pthread_join(thread, NULL);

    safassert(saImmOmCcbObjectDelete(ccbHandle, &rdn), SA_AIS_OK);
    safassert(saImmOmCcbApply(ccbHandle), SA_AIS_OK);
    safassert(saImmOmCcbFinalize(ccbHandle), SA_AIS_OK);

    safassert(saImmOmAdminOwnerFinalize(ownerHandle), SA_AIS_OK);
    safassert(saImmOmFinalize(handle), SA_AIS_OK);

    TRACE_LEAVE();
}

void SaImmOiAdminOperation_11(void)
{
	struct pollfd fds[1];
    int ret;
    pthread_t thread;
    SaImmHandleT handle;
    SaImmAdminOwnerHandleT ownerHandle;
    SaImmCcbHandleT ccbHandle;

    const SaNameT *nameValues[] = {&rdn, NULL};
    SaSelectionObjectT selObj;
    SaImmAdminOperationParamsT_2 param = {
        "TEST",
        SA_IMM_ATTR_SAUINT64T,
        &value
    };
    const SaImmAdminOperationParamsT_2 *params[] = {&adminOperationNameParam, &param, NULL};

    TRACE_ENTER();
    SaImmAttrValuesT_2 v2 = {"rdn",  SA_IMM_ATTR_SANAMET, 1, (void**)nameValues};
    SaUint32T  int1Value1 = 7;
    SaUint32T* int1Values[] = {&int1Value1};
    SaImmAttrValuesT_2 v1 = {"attr1", SA_IMM_ATTR_SAUINT32T, 1, (void**)int1Values};
    const SaImmAttrValuesT_2 * attrValues[] = {&v1, &v2, NULL};

    safassert(saImmOmInitialize_o2(&handle, &omCallbacks, &immVersion), SA_AIS_OK);
    safassert(saImmOmAdminOwnerInitialize(handle, adminOwnerName, SA_TRUE, &ownerHandle), SA_AIS_OK);

    safassert(saImmOmCcbInitialize(ownerHandle, 0, &ccbHandle), SA_AIS_OK);
    safassert(saImmOmCcbObjectCreate_2(ccbHandle, "TestClassConfig", NULL, attrValues), SA_AIS_OK);
    safassert(saImmOmCcbApply(ccbHandle), SA_AIS_OK);

    callbacks = &oiCallbacks_o2_copyParams;

    objectImplementerIsSet = SA_FALSE;
    ret = pthread_create(&thread, NULL, objectImplementerThreadMain, &rdn);
    assert(ret == 0);
    while (!objectImplementerIsSet)
        usleep(100);

    callbacks = &oiCallbacks_o2;

    safassert(saImmOmSelectionObjectGet(handle, &selObj), SA_AIS_OK);
    safassert(saImmOmAdminOwnerSet(ownerHandle, nameValues, SA_IMM_SUBTREE), SA_AIS_OK);

    adminOperationName = "TestOperation";
    safassert(rc = saImmOmAdminOperationInvokeAsync_2(
        ownerHandle, userInvocation, &rdn, 0, SA_IMM_PARAM_ADMOP_ID_ESC, params), SA_AIS_OK);

    fds[0].fd = (int) selObj;
    fds[0].events = POLLIN;

    ret = poll(fds, 1, 1000);
    if (ret == 0)
    {
        TRACE("poll timeout\n");
        goto done;
    }
    if (ret == -1)
    {
        fprintf(stderr, "poll error: %s\n", strerror(errno));
        goto done;
    }

    safassert(saImmOmDispatch(handle, SA_DISPATCH_ONE), SA_AIS_OK);

done:
    TRACE("%s", get_saf_error(rc));
    test_validate(rc, SA_AIS_OK);

    pthread_join(thread, NULL);

    adminOperationName = NULL;

    safassert(saImmOmCcbObjectDelete(ccbHandle, &rdn), SA_AIS_OK);
    safassert(saImmOmCcbApply(ccbHandle), SA_AIS_OK);
    safassert(saImmOmCcbFinalize(ccbHandle), SA_AIS_OK);

    safassert(saImmOmAdminOwnerFinalize(ownerHandle), SA_AIS_OK);
    safassert(saImmOmFinalize(handle), SA_AIS_OK);

    TRACE_LEAVE();
}

void SaImmOiAdminOperation_12(void)
{
    int ret;
    pthread_t thread;
    SaImmHandleT handle;
    SaImmAdminOwnerHandleT ownerHandle;
    SaImmCcbHandleT ccbHandle;

    const SaNameT *nameValues[] = {&rdn, NULL};
    SaSelectionObjectT selObj;
    SaImmAdminOperationParamsT_2 param = {
        "TEST",
        SA_IMM_ATTR_SAUINT64T,
        &value
    };
    const SaImmAdminOperationParamsT_2 *params[] = {&adminOperationNameParam, &param, NULL};
    SaImmAdminOperationParamsT_2 **returnParams = NULL;
    SaAisErrorT operationReturnValue;

    TRACE_ENTER();
    SaImmAttrValuesT_2 v2 = {"rdn",  SA_IMM_ATTR_SANAMET, 1, (void**)nameValues};
    SaUint32T  int1Value1 = 7;
    SaUint32T* int1Values[] = {&int1Value1};
    SaImmAttrValuesT_2 v1 = {"attr1", SA_IMM_ATTR_SAUINT32T, 1, (void**)int1Values};
    const SaImmAttrValuesT_2 * attrValues[] = {&v1, &v2, NULL};

    safassert(saImmOmInitialize_o2(&handle, &omCallbacks, &immVersion), SA_AIS_OK);
    safassert(saImmOmAdminOwnerInitialize(handle, adminOwnerName, SA_TRUE, &ownerHandle), SA_AIS_OK);

    safassert(saImmOmCcbInitialize(ownerHandle, 0, &ccbHandle), SA_AIS_OK);
    safassert(saImmOmCcbObjectCreate_2(ccbHandle, "TestClassConfig", NULL, attrValues), SA_AIS_OK);
    safassert(saImmOmCcbApply(ccbHandle), SA_AIS_OK);

    callbacks = &oiCallbacks_o2_copyParams;

    objectImplementerIsSet = SA_FALSE;
    ret = pthread_create(&thread, NULL, objectImplementerThreadMain, &rdn);
    assert(ret == 0);
    while (!objectImplementerIsSet)
        usleep(100);

    callbacks = &oiCallbacks_o2;

    safassert(saImmOmSelectionObjectGet(handle, &selObj), SA_AIS_OK);
    safassert(saImmOmAdminOwnerSet(ownerHandle, nameValues, SA_IMM_SUBTREE), SA_AIS_OK);

    adminOperationName = "TestOperation";
    if((rc = saImmOmAdminOperationInvoke_o2(ownerHandle, &rdn, 0, SA_IMM_PARAM_ADMOP_ID_ESC, params,
    		&operationReturnValue, SA_TIME_MAX, &returnParams)) != SA_AIS_OK)
    	goto done;

    assert(returnParams != NULL);

    rc = saImmOmAdminOperationMemoryFree(ownerHandle, returnParams);

done:
    TRACE("%s", get_saf_error(rc));
    test_validate(rc, SA_AIS_OK);

    pthread_join(thread, NULL);

    adminOperationName = NULL;

    safassert(saImmOmCcbObjectDelete(ccbHandle, &rdn), SA_AIS_OK);
    safassert(saImmOmCcbApply(ccbHandle), SA_AIS_OK);
    safassert(saImmOmCcbFinalize(ccbHandle), SA_AIS_OK);

    safassert(saImmOmAdminOwnerFinalize(ownerHandle), SA_AIS_OK);
    safassert(saImmOmFinalize(handle), SA_AIS_OK);

    TRACE_LEAVE();
}

__attribute__ ((constructor)) static void saImmOiInitialize_2_constructor(void)
{
    test_suite_add(5, "Administrative Operations");
    test_case_add(5, SaImmOiAdminOperation_01, "SaImmOiAdminOperation - SA_AIS_OK");
    test_case_add(5, SaImmOiAdminOperation_02, "SaImmOiAdminOperation - SA_AIS_ERR_BAD_HANDLE");
    test_case_add(5, SaImmOiAdminOperation_03, "SaImmOiAdminOperation - SA_AIS_ERR_INVALID_PARAM");
    test_case_add(5, SaImmOiAdminOperation_04, "SaImmOiAdminOperation - SA_AIS_ERR_BAD_OPERATION");
    test_case_add(5, SaImmOiAdminOperation_05, "SaImmOiAdminOperation - SA_AIS_ERR_TIMEOUT");
    test_case_add(5, SaImmOiAdminOperation_06, "SaImmOiAdminOperation - SA_AIS_ERR_NOT_EXIST");
    test_case_add(5, SaImmOiAdminOperation_07, "SaImmOiAdminOperation - SA_AIS_OK, OM async");
    test_case_add(5, SaImmOiAdminOperation_08, "SaImmOiAdminOperation - SA_AIS_OK, OM sync with saImmOmAdminOperationMemoryFree");
    test_case_add(5, SaImmOiAdminOperation_09, "SaImmOiAdminOperation - SA_AIS_ERR_BAD_OPERATION, SaImmAdminOperationError");
    test_case_add(5, SaImmOiAdminOperation_10, "SaImmOiAdminOperation - SA_AIS_ERR_BAD_OPERATION, SaImmAdminOperationName");
    test_case_add(5, SaImmOiAdminOperation_11, "SaImmOiAdminOperation - SA_AIS_OK, SaImmAdminOperationName (first param) - async");
    test_case_add(5, SaImmOiAdminOperation_12, "SaImmOiAdminOperation - SA_AIS_OK, SaImmAdminOperationName (first param) - sync");
}

