/*      -*- OpenSAF  -*-
 *
 * (C) Copyright 2012 The OpenSAF Foundation
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


typedef struct ImmThreadArg {
	void *name;		/* SaNameT of objects (NULL terminated list) or SaImmClassNameT for classes */
	SaImmOiCallbacksT_2 *callbacks;
	SaImmOiImplementerNameT implementerName;
} ImmThreadArg;

static int objectDispatchThreadIsSet = 0;
static int classDispatchThreadIsSet = 0;
static int useAdminOwner = 0;

static const SaNameT rdnObj1 = {sizeof("Obj1"), "Obj1"};
static const SaNameT rdnObj2 = {sizeof("Obj2"), "Obj2"};

static SaAisErrorT saImmOiCcbCompletedCallback_response = SA_AIS_OK;
static SaAisErrorT saImmOiCcbObjectCreateCallback_response = SA_AIS_OK;
static SaAisErrorT saImmOiCcbObjectDeleteCallback_response = SA_AIS_OK;
static SaAisErrorT saImmOiCcbObjectModifyCallback_response = SA_AIS_OK;

/* will be increased every time when any callback is called */
static int callbackCounter = 0;

/* count how many threads is running */
static int threadCounter = 0;

static SaImmAttrValueT attrValues[] = { (SaImmAttrValueT)&rdnObj2, NULL };
static SaImmAttrValuesT_2 rdnAttrValue = {
	.attrName = "rdn",
	.attrValueType = SA_IMM_ATTR_SANAMET,
	.attrValuesNumber = 1,
	.attrValues = attrValues
};

static SaImmAttrValuesT_2 *createAttrValues[] = {
	&rdnAttrValue,
	NULL
};

static void saImmOiCcbAbortCallback(SaImmOiHandleT immOiHandle,
    SaImmOiCcbIdT ccbId);
static void saImmOiCcbApplyCallback(SaImmOiHandleT immOiHandle,
    SaImmOiCcbIdT ccbId);
static SaAisErrorT saImmOiCcbCompletedCallback(SaImmOiHandleT immOiHandle,
    SaImmOiCcbIdT ccbId);
static SaAisErrorT saImmOiCcbObjectCreateCallback(SaImmOiHandleT immOiHandle,
    SaImmOiCcbIdT ccbId,
    const SaImmClassNameT className,
    const SaNameT *parentName,
    const SaImmAttrValuesT_2 **attr);
static SaAisErrorT saImmOiCcbObjectDeleteCallback(SaImmOiHandleT immOiHandle,
    SaImmOiCcbIdT ccbId,
    const SaNameT *objectName);
static SaAisErrorT saImmOiCcbObjectModifyCallback(SaImmOiHandleT immOiHandle,
    SaImmOiCcbIdT ccbId,
    const SaNameT *objectName,
    const SaImmAttrModificationT_2 **attrMods);
static SaAisErrorT saImmOiAugCcbObjectCreateCallback(SaImmOiHandleT immOiHandle,
    SaImmOiCcbIdT ccbId,
    const SaImmClassNameT className,
    const SaNameT *parentName,
    const SaImmAttrValuesT_2 **attr);
static SaAisErrorT saImmOiAugCcbObjectDeleteCallback(SaImmOiHandleT immOiHandle,
    SaImmOiCcbIdT ccbId,
    const SaNameT *objectName);
static SaAisErrorT saImmOiAugCcbObjectModifyCallback(SaImmOiHandleT immOiHandle,
    SaImmOiCcbIdT ccbId,
    const SaNameT *objectName,
    const SaImmAttrModificationT_2 **attrMods);

static const SaImmOiCallbacksT_2 callbacks = {
	NULL,
    saImmOiCcbAbortCallback,
    saImmOiCcbApplyCallback,
    saImmOiCcbCompletedCallback,
    saImmOiCcbObjectCreateCallback,
    saImmOiCcbObjectDeleteCallback,
    saImmOiCcbObjectModifyCallback,
    NULL
};

static const SaImmOiCallbacksT_2 augCallbacks = {
	NULL,
    saImmOiCcbAbortCallback,
    saImmOiCcbApplyCallback,
    saImmOiCcbCompletedCallback,
    saImmOiAugCcbObjectCreateCallback,
    saImmOiAugCcbObjectDeleteCallback,
    saImmOiAugCcbObjectModifyCallback,
    NULL
};


static void saImmOiCcbAbortCallback(SaImmOiHandleT immOiHandle,
    SaImmOiCcbIdT ccbId)
{
    TRACE_ENTER2();
    callbackCounter++;
    TRACE_LEAVE2();
}

static void saImmOiCcbApplyCallback(SaImmOiHandleT immOiHandle,
    SaImmOiCcbIdT ccbId)
{
    TRACE_ENTER2();
    callbackCounter++;
    TRACE_LEAVE2();
}

static SaAisErrorT saImmOiCcbCompletedCallback(SaImmOiHandleT immOiHandle,
    SaImmOiCcbIdT ccbId)
{
    TRACE_ENTER2();
    callbackCounter++;
    TRACE_LEAVE2();
    return saImmOiCcbCompletedCallback_response;
}

static SaAisErrorT saImmOiCcbObjectCreateCallback(SaImmOiHandleT immOiHandle,
    SaImmOiCcbIdT ccbId,
    const SaImmClassNameT className,
    const SaNameT *parentName,
    const SaImmAttrValuesT_2 **attr)
{
    TRACE_ENTER2("%llu, %s, %s\n", ccbId, className, parentName->value);
    callbackCounter++;
    if(saImmOiCcbObjectCreateCallback_response != SA_AIS_OK)
    	safassert(saImmOiCcbSetErrorString(immOiHandle, ccbId, (SaStringT)"Set error string in saImmOiCcbObjectCreateCallback"), SA_AIS_OK);
    TRACE_LEAVE2();
    return saImmOiCcbObjectCreateCallback_response;
}

static SaAisErrorT saImmOiCcbObjectDeleteCallback(SaImmOiHandleT immOiHandle,
    SaImmOiCcbIdT ccbId,
    const SaNameT *objectName)
{
    TRACE_ENTER2("%llu, %s\n", ccbId, objectName->value);
    callbackCounter++;
    if(saImmOiCcbObjectDeleteCallback_response != SA_AIS_OK)
    	safassert(saImmOiCcbSetErrorString(immOiHandle, ccbId, (SaStringT)"Set error string in saImmOiCcbObjectDeleteCallback"), SA_AIS_OK);
    TRACE_LEAVE2();
    return saImmOiCcbObjectDeleteCallback_response;
}

static SaAisErrorT saImmOiCcbObjectModifyCallback(SaImmOiHandleT immOiHandle,
    SaImmOiCcbIdT ccbId,
    const SaNameT *objectName,
    const SaImmAttrModificationT_2 **attrMods)
{
    TRACE_ENTER2("%llu, %s\n", ccbId, objectName->value);
    callbackCounter++;
    if(saImmOiCcbObjectModifyCallback_response != SA_AIS_OK)
    	safassert(saImmOiCcbSetErrorString(immOiHandle, ccbId, (SaStringT)"Set error string in saImmOiCcbObjectModifyCallback"), SA_AIS_OK);
    TRACE_LEAVE2();
    return saImmOiCcbObjectModifyCallback_response;
}

static SaAisErrorT saImmOiAugCcbObjectCreateCallback(SaImmOiHandleT immOiHandle,
    SaImmOiCcbIdT ccbId,
    const SaImmClassNameT className,
    const SaNameT *parentName,
    const SaImmAttrValuesT_2 **attr)
{
	SaAisErrorT rc;
    SaImmCcbHandleT ccbHandle;
    SaImmAdminOwnerHandleT ownerHandle;

    TRACE_ENTER2("%llu, %s, %s\n", ccbId, className, parentName->value);
    callbackCounter++;
    safassert(saImmOiAugmentCcbInitialize(immOiHandle, ccbId, &ccbHandle, &ownerHandle), SA_AIS_OK);
    if((rc = saImmOmCcbObjectCreate_2(ccbHandle, className, parentName, (const SaImmAttrValuesT_2 **)createAttrValues)) == SA_AIS_OK)
    	rc = saImmOmCcbApply(ccbHandle);
    TRACE_LEAVE2();
    return rc;
}

static SaAisErrorT saImmOiAugCcbObjectDeleteCallback(SaImmOiHandleT immOiHandle,
    SaImmOiCcbIdT ccbId,
    const SaNameT *objectName)
{
	SaAisErrorT rc;
    SaImmCcbHandleT ccbHandle;
    SaImmAdminOwnerHandleT ownerHandle;
    const SaNameT *objectNames[] = { &rdnObj2, NULL };

    TRACE_ENTER2("%llu, %s\n", ccbId, objectName->value);
    callbackCounter++;
    safassert(saImmOiAugmentCcbInitialize(immOiHandle, ccbId, &ccbHandle, &ownerHandle), SA_AIS_OK);
    if(useAdminOwner)
    	safassert(saImmOmAdminOwnerSet(ownerHandle, objectNames, SA_IMM_ONE), SA_AIS_OK);
    if((rc = saImmOmCcbObjectDelete(ccbHandle, &rdnObj2)) == SA_AIS_OK)
    	rc = saImmOmCcbApply(ccbHandle);
    TRACE_LEAVE2();
    return rc;
}

static SaAisErrorT saImmOiAugCcbObjectModifyCallback(SaImmOiHandleT immOiHandle,
    SaImmOiCcbIdT ccbId,
    const SaNameT *objectName,
    const SaImmAttrModificationT_2 **attrMods)
{
	SaAisErrorT rc;
    SaImmCcbHandleT ccbHandle;
    SaImmAdminOwnerHandleT ownerHandle;
    const SaNameT *objectNames[] = { &rdnObj2, NULL };

    TRACE_ENTER2("%llu, %s\n", ccbId, objectName->value);
    callbackCounter++;
    safassert(saImmOiAugmentCcbInitialize(immOiHandle, ccbId, &ccbHandle, &ownerHandle), SA_AIS_OK);
    if(useAdminOwner)
    	safassert(saImmOmAdminOwnerSet(ownerHandle, objectNames, SA_IMM_ONE), SA_AIS_OK);
    if((rc = saImmOmCcbObjectModify_2(ccbHandle, &rdnObj2, attrMods)) == SA_AIS_OK)
    	rc = saImmOmCcbApply(ccbHandle);
    TRACE_LEAVE2();
    return rc;
}

static SaAisErrorT config_object_create(
    SaImmCcbHandleT ccbHandle,
    const SaNameT *pName,
    const SaNameT *rdn)
{
    const SaNameT* nameValues[] = {NULL, NULL};
    SaImmAttrValuesT_2 v2 = {"rdn",  SA_IMM_ATTR_SANAMET, 1, (void**)nameValues};
    SaUint32T  int1Value1 = __LINE__;
    SaUint32T* int1Values[] = {&int1Value1};
    SaImmAttrValuesT_2 v1 = {"attr1", SA_IMM_ATTR_SAUINT32T, 1, (void**)int1Values};
    const SaImmAttrValuesT_2 * attrValues[] = {&v1, &v2, NULL};
    SaAisErrorT rc;

    TRACE_ENTER2("'%s'\n", rdn->value);

    nameValues[0] = rdn;
    rc = saImmOmCcbObjectCreate_2(ccbHandle, configClassName, pName, attrValues);

    TRACE_LEAVE2();

    return rc;
}

static SaAisErrorT config_object_modify(
    SaImmCcbHandleT ccbHandle,
    const SaNameT *rdn)
{
    SaAisErrorT rc;
    SaUint32T  int1Value = __LINE__;
    SaUint32T* int1Values[] = { &int1Value, NULL };

    SaImmAttrModificationT_2 attrMod = {
            .modType = SA_IMM_ATTR_VALUES_REPLACE,
            .modAttr = { "attr1",
						SA_IMM_ATTR_SAUINT32T,
						1,
						(SaImmAttrValueT *)int1Values } };

    SaImmAttrModificationT_2 *attrMods[] = { &attrMod, NULL };

    TRACE_ENTER2("'%s'\n", rdn->value);

    rc = saImmOmCcbObjectModify_2(ccbHandle, rdn, (const SaImmAttrModificationT_2 **)attrMods);

    TRACE_LEAVE2();

    return rc;
}

static void *immOiObjectDispatchThread(void *arg)
{
    struct ImmThreadArg *immArg = (struct ImmThreadArg *)arg;
    SaNameT **objectName = (SaNameT **)immArg->name;
    SaImmOiImplementerNameT implementerName = immArg->implementerName;
    SaImmOiHandleT handle;
    SaSelectionObjectT selObj;
    struct pollfd fds[1];
    int ret;

    TRACE_ENTER();

    threadCounter++;

    safassert(saImmOiInitialize_2(&handle, immArg->callbacks, &immVersion), SA_AIS_OK);
    safassert(saImmOiImplementerSet(handle, implementerName), SA_AIS_OK);
    safassert(saImmOiObjectImplementerSet(handle, objectName[0], SA_IMM_ONE), SA_AIS_OK);
    if(objectName[1])
    	safassert(saImmOiObjectImplementerSet(handle, objectName[1], SA_IMM_ONE), SA_AIS_OK);
    safassert(saImmOiSelectionObjectGet(handle, &selObj), SA_AIS_OK);

    fds[0].fd = (int) selObj;
    fds[0].events = POLLIN;

    objectDispatchThreadIsSet = 1;

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

    /* Objects might be deleted, so in saImmOiObjectImplementerRelease
     * safassert will be skipped */
    if(objectName[1])
    	saImmOiObjectImplementerRelease(handle, objectName[1], SA_IMM_ONE);
    saImmOiObjectImplementerRelease(handle, objectName[0], SA_IMM_ONE);

    safassert(saImmOiImplementerClear(handle), SA_AIS_OK);
    safassert(saImmOiFinalize(handle), SA_AIS_OK);

    threadCounter--;

    TRACE_LEAVE();

    return NULL;
}

static void *immOiClassDispatchThread(void *arg) {
    struct ImmThreadArg *immArg = (struct ImmThreadArg *)arg;
    SaImmClassNameT className = (SaImmClassNameT)immArg->name;
    SaImmOiImplementerNameT implementerName = immArg->implementerName;
    SaImmOiHandleT handle;
    SaSelectionObjectT selObj;
    struct pollfd fds[1];
    int ret;

    TRACE_ENTER();

    threadCounter++;

    safassert(saImmOiInitialize_2(&handle, immArg->callbacks, &immVersion), SA_AIS_OK);
    safassert(saImmOiImplementerSet(handle, implementerName), SA_AIS_OK);
    safassert(saImmOiClassImplementerSet(handle, className), SA_AIS_OK);
    safassert(saImmOiSelectionObjectGet(handle, &selObj), SA_AIS_OK);

    fds[0].fd = (int) selObj;
    fds[0].events = POLLIN;

    classDispatchThreadIsSet = 1;

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
    safassert(saImmOiImplementerClear(handle), SA_AIS_OK);
    safassert(saImmOiFinalize(handle), SA_AIS_OK);

    threadCounter--;

    TRACE_LEAVE();

    return NULL;
}

static void saImmOiCcbAugmentInitialize_01(void)
{
    SaImmHandleT handle;
    ImmThreadArg arg;
    const SaImmAdminOwnerNameT adminOwnerName = (SaImmAdminOwnerNameT) __FILE__;
    SaImmAdminOwnerHandleT ownerHandle;
    SaImmCcbHandleT ccbHandle;
    SaAisErrorT rc;
    pthread_t threadid;

    TRACE_ENTER();

    safassert(saImmOmInitialize(&handle, NULL, &immVersion), SA_AIS_OK);
    safassert(saImmOmAdminOwnerInitialize(handle, adminOwnerName, SA_TRUE, &ownerHandle), SA_AIS_OK);
    safassert(saImmOmCcbInitialize(ownerHandle, 0, &ccbHandle), SA_AIS_OK);

    safassert(config_class_create(handle), SA_AIS_OK);

    arg.name = (void *)configClassName;
    arg.callbacks = (SaImmOiCallbacksT_2 *)&augCallbacks;
    arg.implementerName = (SaImmOiImplementerNameT)__FUNCTION__;
    classDispatchThreadIsSet = 0;
    threadCounter = 0;
    assert(pthread_create(&threadid, NULL, immOiClassDispatchThread, (void *)&arg) == 0);
    while(!classDispatchThreadIsSet)
    	usleep(500);

    safassert(saImmOmCcbInitialize(ownerHandle, 0, &ccbHandle), SA_AIS_OK);

    /* create objects */
    callbackCounter = 0;	/* reset callback counter */
    if ((rc = config_object_create(ccbHandle, NULL, &rdnObj1)) != SA_AIS_OK)
        goto done;

    assert(callbackCounter == 1);

    callbackCounter = 0;	/* reset callback counter */
    if ((rc = config_object_modify(ccbHandle, &rdnObj1)) != SA_AIS_OK)
        goto done;

    assert(callbackCounter == 1);

    safassert(saImmOmCcbApply(ccbHandle), SA_AIS_OK);

    /* Wait for completed and apply callbacks */
    while(callbackCounter != 3 && threadCounter == 1)
        usleep(500);
    assert(callbackCounter == 3);

	/* Delete objects */
    callbackCounter = 0;	/* reset callback counter */
    if ((rc = saImmOmCcbObjectDelete(ccbHandle, &rdnObj1)) != SA_AIS_OK)
    	goto done;

    assert(callbackCounter == 1);

    safassert(saImmOmCcbApply(ccbHandle), SA_AIS_OK);

done:
    pthread_join(threadid, NULL);

    test_validate(rc, SA_AIS_OK);

    safassert(saImmOmCcbFinalize(ccbHandle), SA_AIS_OK);
    safassert(saImmOmAdminOwnerFinalize(ownerHandle), SA_AIS_OK);

    /* Delete class */
    safassert(config_class_delete(handle), SA_AIS_OK);

    safassert(saImmOmFinalize(handle), SA_AIS_OK);

    TRACE_LEAVE();
}

static void saImmOiCcbAugmentInitialize_02(void)
{
    SaImmHandleT handle;
    ImmThreadArg arg;
    const SaImmAdminOwnerNameT adminOwnerName = (SaImmAdminOwnerNameT) __FILE__;
    SaImmAdminOwnerHandleT ownerHandle;
    const SaNameT *objectNames[] = { &rdnObj1, &rdnObj2, NULL };
    SaImmCcbHandleT ccbHandle;
    SaAisErrorT rc;
    pthread_t threadid;

    TRACE_ENTER();

    safassert(saImmOmInitialize(&handle, NULL, &immVersion), SA_AIS_OK);

    /* Create class */
    safassert(config_class_create(handle), SA_AIS_OK);

    safassert(saImmOmAdminOwnerInitialize(handle, adminOwnerName, SA_TRUE, &ownerHandle), SA_AIS_OK);
    safassert(saImmOmCcbInitialize(ownerHandle, 0, &ccbHandle), SA_AIS_OK);

    /* Create Obj1 and Obj2 */
    if ((rc = config_object_create(ccbHandle, NULL, &rdnObj1)) != SA_AIS_OK)
        goto done;

    if ((rc = config_object_create(ccbHandle, NULL, &rdnObj2)) != SA_AIS_OK)
        goto done;

    safassert(saImmOmCcbApply(ccbHandle), SA_AIS_OK);
    safassert(saImmOmCcbFinalize(ccbHandle), SA_AIS_OK);
    safassert(saImmOmAdminOwnerFinalize(ownerHandle), SA_AIS_OK);

    arg.name = (void *)objectNames;
    arg.callbacks = (SaImmOiCallbacksT_2 *)&augCallbacks;
    arg.implementerName = (SaImmOiImplementerNameT)__FUNCTION__;
    objectDispatchThreadIsSet = 0;
    threadCounter = 0;
    assert(pthread_create(&threadid, NULL, immOiObjectDispatchThread, (void *)&arg) == 0);
    while(!objectDispatchThreadIsSet)
    	usleep(500);

    /* Do modification and delete of objects */
    safassert(saImmOmAdminOwnerInitialize(handle, adminOwnerName, SA_TRUE, &ownerHandle), SA_AIS_OK);
    safassert(saImmOmAdminOwnerSet(ownerHandle, objectNames, SA_IMM_ONE), SA_AIS_OK);
    safassert(saImmOmCcbInitialize(ownerHandle, 0, &ccbHandle), SA_AIS_OK);

    callbackCounter = 0;	/* reset callback counter */
    if ((rc = config_object_modify(ccbHandle, &rdnObj1)) != SA_AIS_OK)
        goto done;

    assert(callbackCounter == 1);

    safassert(saImmOmCcbApply(ccbHandle), SA_AIS_OK);

    /* Wait for completed and apply collbacks */
    while(callbackCounter != 3 && threadCounter == 1)
        usleep(500);
    assert(callbackCounter == 3);

    callbackCounter = 0;	/* reset callback counter */
    if ((rc = saImmOmCcbObjectDelete(ccbHandle, &rdnObj1)) != SA_AIS_OK)
    	goto done;

	assert(callbackCounter == 1);

    safassert(saImmOmCcbApply(ccbHandle), SA_AIS_OK);

done:
	pthread_join(threadid, NULL);

	test_validate(rc, SA_AIS_OK);

    safassert(saImmOmCcbFinalize(ccbHandle), SA_AIS_OK);
    safassert(saImmOmAdminOwnerFinalize(ownerHandle), SA_AIS_OK);

    /* Delete class */
    safassert(config_class_delete(handle), SA_AIS_OK);

    safassert(saImmOmFinalize(handle), SA_AIS_OK);

    TRACE_LEAVE();
}

static void saImmOiCcbAugmentInitialize_03(void)
{
    SaImmHandleT handle;
    ImmThreadArg arg[2];
    const SaImmAdminOwnerNameT adminOwnerName = (SaImmAdminOwnerNameT) __FILE__;
    SaImmAdminOwnerHandleT ownerHandle;
    const SaNameT *ownerObjectNames[] = { &rdnObj1, &rdnObj2, NULL };
    const SaNameT *objectNames1[] = { &rdnObj1, NULL };
    const SaNameT *objectNames2[] = { &rdnObj2, NULL };
    SaImmCcbHandleT ccbHandle;
    SaAisErrorT rc;
    pthread_t threadid1;
    pthread_t threadid2;

    TRACE_ENTER();

    safassert(saImmOmInitialize(&handle, NULL, &immVersion), SA_AIS_OK);

    /* Create class */
    safassert(config_class_create(handle), SA_AIS_OK);

    safassert(saImmOmAdminOwnerInitialize(handle, adminOwnerName, SA_TRUE, &ownerHandle), SA_AIS_OK);
    safassert(saImmOmCcbInitialize(ownerHandle, 0, &ccbHandle), SA_AIS_OK);

    /* Create Obj1 and Obj2 */
    safassert(config_object_create(ccbHandle, NULL, &rdnObj1), SA_AIS_OK);
    safassert(config_object_create(ccbHandle, NULL, &rdnObj2), SA_AIS_OK);

    safassert(saImmOmCcbApply(ccbHandle), SA_AIS_OK);
    safassert(saImmOmCcbFinalize(ccbHandle), SA_AIS_OK);
    safassert(saImmOmAdminOwnerFinalize(ownerHandle), SA_AIS_OK);

    arg[0].name = (void *)objectNames1;
    arg[0].callbacks = (SaImmOiCallbacksT_2 *)&augCallbacks;
    arg[0].implementerName = (SaImmOiImplementerNameT)"TestImplementer1";
    arg[1].name = (void *)objectNames2;
    arg[1].callbacks = (SaImmOiCallbacksT_2 *)&callbacks;
    arg[1].implementerName = (SaImmOiImplementerNameT)"TestImplementer2";

    threadCounter = 0;
    objectDispatchThreadIsSet = 0;
    assert(pthread_create(&threadid1, NULL, immOiObjectDispatchThread, (void *)&(arg[0])) == 0);
    while(!objectDispatchThreadIsSet)
    	usleep(500);

    objectDispatchThreadIsSet = 0;
    assert(pthread_create(&threadid2, NULL, immOiObjectDispatchThread, (void *)&(arg[1])) == 0);
    while(!objectDispatchThreadIsSet)
    	usleep(500);

    safassert(saImmOmAdminOwnerInitialize(handle, adminOwnerName, SA_TRUE, &ownerHandle), SA_AIS_OK);
    safassert(saImmOmAdminOwnerSet(ownerHandle, ownerObjectNames, SA_IMM_ONE), SA_AIS_OK);
    safassert(saImmOmCcbInitialize(ownerHandle, 0, &ccbHandle), SA_AIS_OK);

    /* Modify objects */
    callbackCounter = 0;	/* reset callback counter */
    if ((rc = config_object_modify(ccbHandle, &rdnObj1)) != SA_AIS_OK)
        goto done;

    /* Check that there were 2 callbacks */
    assert(callbackCounter == 2);

    safassert(saImmOmCcbApply(ccbHandle), SA_AIS_OK);

    /* Wait for 2 completed and 2 apply callbacks */
    while(callbackCounter != 6 && threadCounter == 2)
        usleep(500);
    assert(callbackCounter == 6);

    /* Delete objects */
    callbackCounter = 0;	/* reset callback counter */
    if ((rc = saImmOmCcbObjectDelete(ccbHandle, &rdnObj1)) != SA_AIS_OK)
    	goto done;

    /* Check that there were 2 callbacks */
    assert(callbackCounter == 2);

    safassert(saImmOmCcbApply(ccbHandle), SA_AIS_OK);

done:
	pthread_join(threadid1, NULL);
	pthread_join(threadid2, NULL);

	test_validate(rc, SA_AIS_OK);

    safassert(saImmOmCcbFinalize(ccbHandle), SA_AIS_OK);
    safassert(saImmOmAdminOwnerFinalize(ownerHandle), SA_AIS_OK);

    /* Delete class */
    safassert(config_class_delete(handle), SA_AIS_OK);

    safassert(saImmOmFinalize(handle), SA_AIS_OK);

    TRACE_LEAVE();
}

static void saImmOiCcbAugmentInitialize_04(void)
{
    SaImmHandleT handle;
    ImmThreadArg arg[2];
    const SaImmAdminOwnerNameT adminOwnerName = (SaImmAdminOwnerNameT) __FILE__;
    SaImmAdminOwnerHandleT ownerHandle;
    const SaNameT *objectNames1[] = { &rdnObj1, NULL };
    const SaNameT *objectNames2[] = { &rdnObj2, NULL };
    SaImmCcbHandleT ccbHandle;
    SaAisErrorT rc;
    pthread_t threadid1;
    pthread_t threadid2;

    TRACE_ENTER();

    safassert(saImmOmInitialize(&handle, NULL, &immVersion), SA_AIS_OK);

    /* Create class */
    safassert(config_class_create(handle), SA_AIS_OK);

    safassert(saImmOmAdminOwnerInitialize(handle, adminOwnerName, SA_TRUE, &ownerHandle), SA_AIS_OK);
    safassert(saImmOmCcbInitialize(ownerHandle, 0, &ccbHandle), SA_AIS_OK);

    /* Create Obj1 and Obj2 */
    safassert(config_object_create(ccbHandle, NULL, &rdnObj1), SA_AIS_OK);
    safassert(config_object_create(ccbHandle, NULL, &rdnObj2), SA_AIS_OK);

    safassert(saImmOmCcbApply(ccbHandle), SA_AIS_OK);
    safassert(saImmOmCcbFinalize(ccbHandle), SA_AIS_OK);
    safassert(saImmOmAdminOwnerFinalize(ownerHandle), SA_AIS_OK);

    arg[0].name = (void *)objectNames1;
    arg[0].callbacks = (SaImmOiCallbacksT_2 *)&augCallbacks;
    arg[0].implementerName = (SaImmOiImplementerNameT)"TestImplementer1";
    arg[1].name = (void *)objectNames2;
    arg[1].callbacks = (SaImmOiCallbacksT_2 *)&callbacks;
    arg[1].implementerName = (SaImmOiImplementerNameT)"TestImplementer2";

    threadCounter = 0;
    objectDispatchThreadIsSet = 0;
    assert(pthread_create(&threadid1, NULL, immOiObjectDispatchThread, (void *)&(arg[0])) == 0);
    while(!objectDispatchThreadIsSet)
    	usleep(500);

    objectDispatchThreadIsSet = 0;
    assert(pthread_create(&threadid2, NULL, immOiObjectDispatchThread, (void *)&(arg[1])) == 0);
    while(!objectDispatchThreadIsSet)
    	usleep(500);

    safassert(saImmOmAdminOwnerInitialize(handle, adminOwnerName, SA_TRUE, &ownerHandle), SA_AIS_OK);
    safassert(saImmOmAdminOwnerSet(ownerHandle, objectNames1, SA_IMM_ONE), SA_AIS_OK);
    safassert(saImmOmCcbInitialize(ownerHandle, 0, &ccbHandle), SA_AIS_OK);

    useAdminOwner = 1;

    /* Modify objects */
    callbackCounter = 0;	/* reset callback counter */
    if ((rc = config_object_modify(ccbHandle, &rdnObj1)) != SA_AIS_OK)
        goto done;

    /* Check that there were 2 callbacks */
    assert(callbackCounter == 2);

    safassert(saImmOmCcbApply(ccbHandle), SA_AIS_OK);

    /* Wait for 2 completed and 2 apply callbacks */
    while(callbackCounter != 6 && threadCounter == 2)
        usleep(500);
    assert(callbackCounter == 6);

    /* Delete objects */
    callbackCounter = 0;	/* reset callback counter */
    if ((rc = saImmOmCcbObjectDelete(ccbHandle, &rdnObj1)) != SA_AIS_OK)
    	goto done;

    /* Check that there were 2 callbacks */
    assert(callbackCounter == 2);

    safassert(saImmOmCcbApply(ccbHandle), SA_AIS_OK);

done:
	useAdminOwner = 0;

	pthread_join(threadid1, NULL);
	pthread_join(threadid2, NULL);

	test_validate(rc, SA_AIS_OK);

    safassert(saImmOmCcbFinalize(ccbHandle), SA_AIS_OK);
    safassert(saImmOmAdminOwnerFinalize(ownerHandle), SA_AIS_OK);

    /* Delete class */
    safassert(config_class_delete(handle), SA_AIS_OK);

    safassert(saImmOmFinalize(handle), SA_AIS_OK);

    TRACE_LEAVE();
}


__attribute__ ((constructor)) static void saImmOiCcbAugmentInitialize_constructor(void)
{
    test_suite_add(6, "Augmented CCBs");
    test_case_add(6, saImmOiCcbAugmentInitialize_01, "saImmOiCcbAugmentInitialize - SA_AIS_OK - class implementer: create, modify, delete");
    test_case_add(6, saImmOiCcbAugmentInitialize_02, "saImmOiCcbAugmentInitialize - SA_AIS_OK - one object implementer: modify, delete");
    test_case_add(6, saImmOiCcbAugmentInitialize_03, "saImmOiCcbAugmentInitialize - SA_AIS_OK - two object implementers: modify, delete");
    test_case_add(6, saImmOiCcbAugmentInitialize_04, "saImmOiCcbAugmentInitialize - SA_AIS_OK - two object implementers: modify and delete with saImmOmAdminOwnerSet");
}

