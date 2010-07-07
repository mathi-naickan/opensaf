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

#include "immtest.h"

static const SaNameT parentName = {sizeof("opensafImm=opensafImm,safApp=safImmService"), "opensafImm=opensafImm,safApp=safImmService"};
static const SaNameT rdnObj1 = {sizeof("Obj1"), "Obj1"};
static const SaNameT rdnObj2 = {sizeof("Obj2"), "Obj2"};

static SaNameT dnObj1;
static SaNameT dnObj2;
static const SaNameT *dnObjs[] = {&dnObj1, NULL};

static SaAisErrorT config_object_create(SaImmHandleT immHandle,
    SaImmAdminOwnerHandleT ownerHandle,
    const SaNameT *parentName)
{
    SaImmCcbHandleT ccbHandle;
    const SaNameT* nameValues[] = {&rdnObj1, NULL};
    SaImmAttrValuesT_2 v2 = {"rdn",  SA_IMM_ATTR_SANAMET, 1, (void**)nameValues};
    SaUint32T  int1Value1 = 7;
    SaUint32T* int1Values[] = {&int1Value1};
    SaImmAttrValuesT_2 v1 = {"attr1", SA_IMM_ATTR_SAUINT32T, 1, (void**)int1Values};
    const SaImmAttrValuesT_2 * attrValues[] = {&v1, &v2, NULL};

    safassert(saImmOmCcbInitialize(ownerHandle, 0, &ccbHandle), SA_AIS_OK);
    safassert(saImmOmCcbObjectCreate_2(ccbHandle, configClassName, parentName, attrValues), SA_AIS_OK);
    safassert(saImmOmCcbApply(ccbHandle), SA_AIS_OK);
    return saImmOmCcbFinalize(ccbHandle);
}

static SaAisErrorT config_object_delete(SaImmHandleT immHandle,
    SaImmAdminOwnerHandleT ownerHandle)
{
    SaImmCcbHandleT ccbHandle;

    safassert(saImmOmCcbInitialize(ownerHandle, 0, &ccbHandle), SA_AIS_OK);
    safassert(saImmOmCcbObjectDelete(ccbHandle, &dnObj1), SA_AIS_OK);
    safassert(saImmOmCcbApply(ccbHandle), SA_AIS_OK);
    return saImmOmCcbFinalize(ccbHandle);
}

void saImmOmCcbObjectDelete_01(void)
{
    const SaImmAdminOwnerNameT adminOwnerName = (SaImmAdminOwnerNameT) __FUNCTION__;
    SaImmAdminOwnerHandleT ownerHandle;
    SaImmCcbHandleT ccbHandle;
    const SaNameT *objectNames[] = {&parentName, NULL};

    safassert(saImmOmInitialize(&immOmHandle, NULL, &immVersion), SA_AIS_OK);
    safassert(saImmOmAdminOwnerInitialize(immOmHandle, adminOwnerName, SA_TRUE, &ownerHandle), SA_AIS_OK);
    safassert(saImmOmAdminOwnerSet(ownerHandle, objectNames, SA_IMM_ONE), SA_AIS_OK);
    safassert(config_class_create(immOmHandle), SA_AIS_OK);
    safassert(config_object_create(immOmHandle, ownerHandle, &parentName), SA_AIS_OK);
    safassert(saImmOmAdminOwnerSet(ownerHandle, dnObjs, SA_IMM_ONE), SA_AIS_OK);
    safassert(saImmOmCcbInitialize(ownerHandle, 0, &ccbHandle), SA_AIS_OK);

    test_validate(saImmOmCcbObjectDelete(ccbHandle, &dnObj1), SA_AIS_OK);

    safassert(saImmOmCcbApply(ccbHandle), SA_AIS_OK);
    safassert(saImmOmCcbFinalize(ccbHandle), SA_AIS_OK);
    safassert(config_class_delete(immOmHandle), SA_AIS_OK);
    safassert(saImmOmAdminOwnerFinalize(ownerHandle), SA_AIS_OK);
    safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);
}

void saImmOmCcbObjectDelete_02(void)
{
    const SaImmAdminOwnerNameT adminOwnerName = (SaImmAdminOwnerNameT) __FUNCTION__;
    SaImmAdminOwnerHandleT ownerHandle;
    SaImmCcbHandleT ccbHandle;
    const SaNameT *objectNames[] = {&parentName, NULL};

    safassert(saImmOmInitialize(&immOmHandle, &immOmCallbacks, &immVersion), SA_AIS_OK);
    safassert(saImmOmAdminOwnerInitialize(immOmHandle, adminOwnerName, SA_TRUE, &ownerHandle), SA_AIS_OK);
    safassert(saImmOmAdminOwnerSet(ownerHandle, objectNames, SA_IMM_ONE), SA_AIS_OK);
    safassert(config_class_create(immOmHandle), SA_AIS_OK);
    safassert(config_object_create(immOmHandle, ownerHandle, &parentName), SA_AIS_OK);
    safassert(saImmOmCcbInitialize(ownerHandle, 0, &ccbHandle), SA_AIS_OK);

    /* invalid ccbHandle */
    if ((rc = saImmOmCcbObjectDelete(-1, &dnObj1)) != SA_AIS_ERR_BAD_HANDLE)
        goto done;

    /* already finalized ccbHandle */
    safassert(saImmOmCcbInitialize(ownerHandle, 0, &ccbHandle), SA_AIS_OK);
    safassert(saImmOmCcbFinalize(ccbHandle), SA_AIS_OK);
    rc = saImmOmCcbObjectDelete(ccbHandle, &dnObj1);

done:
    test_validate(rc, SA_AIS_ERR_BAD_HANDLE);
    safassert(config_object_delete(immOmHandle, ownerHandle), SA_AIS_OK);
    safassert(config_class_delete(immOmHandle), SA_AIS_OK);
    safassert(saImmOmAdminOwnerFinalize(ownerHandle), SA_AIS_OK);
    safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);
}

void saImmOmCcbObjectDelete_03(void)
{
    const SaImmAdminOwnerNameT adminOwnerName = (SaImmAdminOwnerNameT) __FUNCTION__;
    SaImmAdminOwnerHandleT ownerHandle;
    SaImmCcbHandleT ccbHandle;
    const SaNameT *objectNames[] = {&parentName, NULL};

    safassert(saImmOmInitialize(&immOmHandle, &immOmCallbacks, &immVersion), SA_AIS_OK);
    safassert(saImmOmAdminOwnerInitialize(immOmHandle, adminOwnerName, SA_TRUE, &ownerHandle), SA_AIS_OK);
    safassert(saImmOmAdminOwnerSet(ownerHandle, objectNames, SA_IMM_ONE), SA_AIS_OK);
    safassert(config_class_create(immOmHandle), SA_AIS_OK);
    safassert(config_object_create(immOmHandle, ownerHandle, &parentName), SA_AIS_OK);
    safassert(saImmOmAdminOwnerRelease(ownerHandle, objectNames, SA_IMM_SUBTREE), SA_AIS_OK);
    safassert(saImmOmCcbInitialize(ownerHandle, 0, &ccbHandle), SA_AIS_OK);

    /* 
    ** at least one of the targeted objects is not a configuration object that is owned by
    ** the administrative owner of the CCB
    */
    if ((rc = saImmOmCcbObjectDelete(ccbHandle, &dnObj1)) != SA_AIS_ERR_BAD_OPERATION)
    {
        TRACE_ENTER();
        goto done;
    }

    /*
    ** at least one of the targeted objects has some registered continuation identifiers
    */

    /*
    ** the Object Implementer has rejected the deletion of at least one of the targeted
    ** objects.
    */

done:
    safassert(saImmOmAdminOwnerSet(ownerHandle, objectNames, SA_IMM_SUBTREE), SA_AIS_OK);
    safassert(config_object_delete(immOmHandle, ownerHandle), SA_AIS_OK);
    safassert(config_class_delete(immOmHandle), SA_AIS_OK);
    safassert(saImmOmAdminOwnerFinalize(ownerHandle), SA_AIS_OK);
    safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);
    test_validate(rc, SA_AIS_ERR_BAD_OPERATION);
}

void saImmOmCcbObjectDelete_04(void)
{
    const SaImmAdminOwnerNameT adminOwnerName = (SaImmAdminOwnerNameT) __FUNCTION__;
    SaImmAdminOwnerHandleT ownerHandle;
    SaImmCcbHandleT ccbHandle;
    const SaNameT *objectNames[] = {&parentName, NULL};

    safassert(saImmOmInitialize(&immOmHandle, NULL, &immVersion), SA_AIS_OK);
    safassert(saImmOmAdminOwnerInitialize(immOmHandle, adminOwnerName, SA_TRUE, &ownerHandle), SA_AIS_OK);
    safassert(saImmOmAdminOwnerSet(ownerHandle, objectNames, SA_IMM_ONE), SA_AIS_OK);
    safassert(config_class_create(immOmHandle), SA_AIS_OK);
    safassert(config_object_create(immOmHandle, ownerHandle, &parentName), SA_AIS_OK);
    safassert(saImmOmCcbInitialize(ownerHandle, SA_IMM_CCB_REGISTERED_OI, &ccbHandle), SA_AIS_OK);

    /* 
    ** The name to which the objectName parameter points is not the name of an
    ** existing object.
    */
    if ((rc = saImmOmCcbObjectDelete(ccbHandle, &dnObj2)) != SA_AIS_ERR_NOT_EXIST)
    {
        TRACE_ENTER();
        goto done;
    }

    /*                                                              
    ** There is no registered Object Implementer for at least one of the objects
    ** targeted by this operation, and the CCB has been initialized with the
    ** SA_IMM_CCB_REGISTERED_OI flag set.                                                                
    */
    if ((rc = saImmOmCcbObjectDelete(ccbHandle, &dnObj1)) != SA_AIS_ERR_NOT_EXIST)
    {
        TRACE_ENTER();
        goto done;
    }

done:
    safassert(config_object_delete(immOmHandle, ownerHandle), SA_AIS_OK);
    safassert(config_class_delete(immOmHandle), SA_AIS_OK);
    safassert(saImmOmAdminOwnerFinalize(ownerHandle), SA_AIS_OK);
    safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);

    test_validate(rc, SA_AIS_ERR_NOT_EXIST);
}

void saImmOmCcbObjectDelete_05(void)
{
    const SaImmAdminOwnerNameT adminOwnerName = (SaImmAdminOwnerNameT) __FUNCTION__;
    SaImmAdminOwnerHandleT ownerHandle;
    SaImmCcbHandleT ccbHandle1;
    SaImmCcbHandleT ccbHandle2;
    const SaNameT *objectNames[] = {&parentName, NULL};

    safassert(saImmOmInitialize(&immOmHandle, NULL, &immVersion), SA_AIS_OK);
    safassert(saImmOmAdminOwnerInitialize(immOmHandle, adminOwnerName, SA_TRUE, &ownerHandle), SA_AIS_OK);
    safassert(saImmOmAdminOwnerSet(ownerHandle, objectNames, SA_IMM_ONE), SA_AIS_OK);
    safassert(config_class_create(immOmHandle), SA_AIS_OK);
    safassert(config_object_create(immOmHandle, ownerHandle, &parentName), SA_AIS_OK);
    safassert(saImmOmCcbInitialize(ownerHandle, 0, &ccbHandle1), SA_AIS_OK);
    safassert(saImmOmCcbObjectDelete(ccbHandle1, &dnObj1), SA_AIS_OK);

    /* 
    ** At least one of the targeted objects is already the target of an
    ** administrative operation or of a change request in another CCB.
    */
    safassert(saImmOmCcbInitialize(ownerHandle, 0, &ccbHandle2), SA_AIS_OK);
    rc = saImmOmCcbObjectDelete(ccbHandle2, &dnObj1);

    safassert(saImmOmCcbFinalize(ccbHandle1), SA_AIS_OK);
    safassert(config_object_delete(immOmHandle, ownerHandle), SA_AIS_OK);
    safassert(config_class_delete(immOmHandle), SA_AIS_OK);
    safassert(saImmOmAdminOwnerFinalize(ownerHandle), SA_AIS_OK);
    safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);

    test_validate(rc, SA_AIS_ERR_BUSY);
}

__attribute__ ((constructor)) static void saImmOmCcbObjectDelete_constructor(void)
{
    dnObj1.length = (SaUint16T) sprintf((char*) dnObj1.value, "%s,%s", rdnObj1.value, parentName.value);
    dnObj2.length = (SaUint16T) sprintf((char*) dnObj2.value, "%s,%s", rdnObj2.value, parentName.value);
}

