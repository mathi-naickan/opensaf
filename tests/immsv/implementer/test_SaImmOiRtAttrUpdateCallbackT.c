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

static SaImmClassNameT className = (SaImmClassNameT) "SaAmfComp_TEST";
static SaUint32T int1Value = 0xdeadbeef;
static SaImmAttrValueT attrUpdateValues[] ={&int1Value};
static SaImmAttrModificationT_2 attrMod1 =
{
    .modType = SA_IMM_ATTR_VALUES_REPLACE,
    .modAttr.attrName = "saAmfCompRestartCount",
    .modAttr.attrValuesNumber = 1,
    .modAttr.attrValueType = SA_IMM_ATTR_SAINT32T,
    .modAttr.attrValues = attrUpdateValues
};

static SaImmAttrModificationT_2* attrMods[] = {&attrMod1, NULL};

static SaNameT testObjectName = 
{
    .value = "safComp=MyConfigHandler1",
    .length = sizeof("safComp=MyConfigHandler1")
};

SaNameT* nameValues[1] = { &testObjectName };

static SaImmAccessorHandleT accessorHandle;
static SaImmAttrValuesT_2** attributes;

static SaImmHandleT localImmHandle;
static SaImmAdminOwnerHandleT localOwnerHandle;
static SaImmCcbHandleT localCcbHandle;
static SaBoolT  classCreated=SA_FALSE;

static void setup(SaImmAdminOwnerNameT admoName)
{
    SaAisErrorT err = SA_AIS_OK;

    SaImmAttrDefinitionT_2 d1 = 
        { "safComp", SA_IMM_ATTR_SANAMET, 
          SA_IMM_ATTR_CONFIG | SA_IMM_ATTR_RDN | SA_IMM_ATTR_INITIALIZED,
          NULL };

    SaImmAttrDefinitionT_2 d2 =
        { "saAmfCompRestartCount", SA_IMM_ATTR_SAINT32T, 
          SA_IMM_ATTR_RUNTIME, NULL };

    /* Note class description not the complete standard class! 
       If the standard class exists we use it, if not we temporarily install
       this dummy class.
    */

    const SaImmAttrDefinitionT_2* attrDefs[4] = {&d1, &d2, 0};


     SaImmAttrValuesT_2 v1=
       { "safComp",  SA_IMM_ATTR_SANAMET, 1, (void**)nameValues };
     
     const SaImmAttrValuesT_2* attrValues[2] = {&v1, 0};


    safassert(saImmOmInitialize(&localImmHandle, NULL, &immVersion), SA_AIS_OK);

    err = saImmOmClassCreate_2(localImmHandle, className, SA_IMM_CLASS_CONFIG, attrDefs);
    if(err == SA_AIS_OK) 
    {
        classCreated = SA_TRUE;
    } else if(err == SA_AIS_ERR_EXIST)
    {
        err = SA_AIS_OK;
    }

    safassert(err, SA_AIS_OK);

    /* Create the test object */

    safassert(saImmOmAdminOwnerInitialize(localImmHandle, admoName, SA_TRUE, &localOwnerHandle), SA_AIS_OK);
    safassert(saImmOmCcbInitialize(localOwnerHandle, 0LL, &localCcbHandle), SA_AIS_OK);
    safassert(saImmOmCcbObjectCreate_2(localCcbHandle, className, NULL, attrValues), SA_AIS_OK);
    safassert(saImmOmCcbApply(localCcbHandle), SA_AIS_OK);
    sleep(1);
}

static void tearDown(void)
{
    sleep(1);
    safassert(saImmOmCcbObjectDelete(localCcbHandle, &testObjectName), SA_AIS_OK);
    safassert(saImmOmCcbApply(localCcbHandle), SA_AIS_OK);
    if(classCreated)
    {
        safassert(saImmOmClassDelete(localImmHandle, className), SA_AIS_OK);
    }

    safassert(saImmOmCcbFinalize(localCcbHandle), SA_AIS_OK);
    safassert(saImmOmAdminOwnerFinalize(localOwnerHandle), SA_AIS_OK);
    safassert(saImmOmFinalize(localImmHandle), SA_AIS_OK);
}

static SaAisErrorT saImmOiRtAttrUpdateCallback(SaImmOiHandleT handle,
    const SaNameT *objectName,
    const SaImmAttrNameT *attributeNames)
{
    assert(immOiHandle == handle);
    assert(objectName != NULL);
    assert(attributeNames != NULL);

    TRACE_ENTER();

    return saImmOiRtObjectUpdate_2(handle, objectName, (const SaImmAttrModificationT_2**) attrMods);
}

static void *test_saImmOmAccessorGet_2(void *arg)
{
    SaNameT *dn = (SaNameT *) arg;
    SaImmAttrNameT attributeName = attrMod1.modAttr.attrName;
    SaImmAttrNameT attributeNames[2] = {attributeName, NULL};

    TRACE_ENTER();

    safassert(saImmOmInitialize(&immOmHandle, &immOmCallbacks, &immVersion), SA_AIS_OK);
    safassert(saImmOmAccessorInitialize(immOmHandle, &accessorHandle), SA_AIS_OK);
    safassert(saImmOmAccessorGet_2(accessorHandle, dn, attributeNames, &attributes), SA_AIS_OK);
    assert(attributes[0]->attrValueType == SA_IMM_ATTR_SAINT32T);
    assert(*((SaInt32T*) attributes[0]->attrValues[0]) == int1Value);
    safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);

    TRACE_LEAVE();
    return NULL;
}

void SaImmOiRtAttrUpdateCallbackT_01(void)
{
    SaImmOiImplementerNameT implementerName = (SaImmOiImplementerNameT) __FUNCTION__;
    SaImmAdminOwnerNameT admoName = (SaImmAdminOwnerNameT) implementerName;
    struct pollfd fds[1];
    int ret;
    pthread_t thread;

    TRACE_ENTER();
    setup(admoName);
    immOiCallbacks.saImmOiRtAttrUpdateCallback = saImmOiRtAttrUpdateCallback;

    safassert(saImmOiInitialize_2(&immOiHandle, &immOiCallbacks, &immVersion), SA_AIS_OK);
    safassert(saImmOiImplementerSet(immOiHandle, implementerName), SA_AIS_OK);
    safassert(saImmOiClassImplementerSet(immOiHandle, className), SA_AIS_OK);
    safassert(saImmOiSelectionObjectGet(immOiHandle, &selectionObject), SA_AIS_OK);

    ret = pthread_create(&thread, NULL, test_saImmOmAccessorGet_2, &testObjectName);
    assert(ret == 0);

    fds[0].fd = (int) selectionObject;
    fds[0].events = POLLIN;
    ret = poll(fds, 1, 1000);
    assert(ret == 1);

    safassert(saImmOiDispatch(immOiHandle, SA_DISPATCH_ONE), SA_AIS_OK);

    pthread_join(thread, NULL);

    test_validate(SA_AIS_OK, SA_AIS_OK);
    safassert(saImmOiClassImplementerRelease(immOiHandle, className), SA_AIS_OK);
    safassert(saImmOiImplementerClear(immOiHandle), SA_AIS_OK);
    safassert(saImmOiFinalize(immOiHandle), SA_AIS_OK);
    immOiCallbacks.saImmOiRtAttrUpdateCallback = NULL;
    tearDown();

    TRACE_LEAVE();
}

