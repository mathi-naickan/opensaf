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

static const SaNameT rdn = {
    .value = "Obj2",
    .length = sizeof("Obj2"),
};

void saImmOiObjectImplementerSet_01(void)
{
    const SaImmOiImplementerNameT implementerName = (SaImmOiImplementerNameT) __FUNCTION__;
    const SaImmAdminOwnerNameT adminOwnerName = (SaImmAdminOwnerNameT) __FILE__;
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
    safassert(config_class_create(immOmHandle), SA_AIS_OK);
    /* Create test object under root */
    safassert(saImmOmCcbObjectCreate_2(ccbHandle, "TestClassConfig", NULL, attrValues), SA_AIS_OK);
    safassert(saImmOmCcbApply(ccbHandle), SA_AIS_OK);

    safassert(saImmOiInitialize_2(&immOiHandle, &immOiCallbacks, &immVersion), SA_AIS_OK);
    safassert(saImmOiImplementerSet(immOiHandle, implementerName), SA_AIS_OK);
    rc = saImmOiObjectImplementerSet(immOiHandle, &rdn, SA_IMM_ONE);
    safassert(saImmOiObjectImplementerRelease(immOiHandle, &rdn, SA_IMM_ONE), SA_AIS_OK);
    safassert(saImmOiFinalize(immOiHandle), SA_AIS_OK);
 
    safassert(saImmOmCcbObjectDelete(ccbHandle, &rdn), SA_AIS_OK);
    safassert(saImmOmCcbApply(ccbHandle), SA_AIS_OK);
    safassert(saImmOmCcbFinalize(ccbHandle), SA_AIS_OK);
    safassert(config_class_delete(immOmHandle), SA_AIS_OK);
    safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);

    test_validate(rc, SA_AIS_OK);
}

void saImmOiObjectImplementerSet_02(void)
{
    const SaImmOiImplementerNameT implementerName = (SaImmOiImplementerNameT) __FUNCTION__;
    const SaImmAdminOwnerNameT adminOwnerName = (SaImmAdminOwnerNameT) __FILE__;
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
    safassert(config_class_create(immOmHandle), SA_AIS_OK);
    /* Create test object under root */
    safassert(saImmOmCcbObjectCreate_2(ccbHandle, "TestClassConfig", NULL, attrValues), SA_AIS_OK);
    safassert(saImmOmCcbApply(ccbHandle), SA_AIS_OK);

    safassert(saImmOiInitialize_2(&immOiHandle, &immOiCallbacks, &immVersion), SA_AIS_OK);
    safassert(saImmOiImplementerSet(immOiHandle, implementerName), SA_AIS_OK);
    rc = saImmOiObjectImplementerSet(-1, &rdn, SA_IMM_ONE);

    safassert(saImmOmCcbObjectDelete(ccbHandle, &rdn), SA_AIS_OK);
    safassert(saImmOmCcbApply(ccbHandle), SA_AIS_OK);
    safassert(saImmOmCcbFinalize(ccbHandle), SA_AIS_OK);
    safassert(config_class_delete(immOmHandle), SA_AIS_OK);
    safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);

    test_validate(rc, SA_AIS_ERR_BAD_HANDLE);
    safassert(saImmOiFinalize(immOiHandle), SA_AIS_OK);
}

void saImmOiObjectImplementerSet_03(void)
{
    const SaImmAdminOwnerNameT adminOwnerName = (SaImmAdminOwnerNameT) __FILE__;
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
    safassert(config_class_create(immOmHandle), SA_AIS_OK);
    /* Create test object under root */
    safassert(saImmOmCcbObjectCreate_2(ccbHandle, "TestClassConfig", NULL, attrValues), SA_AIS_OK);
    safassert(saImmOmCcbApply(ccbHandle), SA_AIS_OK);

    safassert(saImmOiInitialize_2(&immOiHandle, &immOiCallbacks, &immVersion), SA_AIS_OK);
    rc = saImmOiObjectImplementerSet(immOiHandle, &rdn, SA_IMM_ONE);

    safassert(saImmOmCcbObjectDelete(ccbHandle, &rdn), SA_AIS_OK);
    safassert(saImmOmCcbApply(ccbHandle), SA_AIS_OK);
    safassert(saImmOmCcbFinalize(ccbHandle), SA_AIS_OK);
    safassert(config_class_delete(immOmHandle), SA_AIS_OK);
    safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);

    test_validate(rc, SA_AIS_ERR_BAD_HANDLE);
    safassert(saImmOiFinalize(immOiHandle), SA_AIS_OK);
}

void saImmOiObjectImplementerSet_04(void)
{
    const SaImmOiImplementerNameT implementerName = (SaImmOiImplementerNameT) __FUNCTION__;
    SaNameT nonExistentObjectName = {
        .value = "XXX",
        .length = sizeof("XXX"),
    };

    safassert(saImmOiInitialize_2(&immOiHandle, &immOiCallbacks, &immVersion), SA_AIS_OK);
    safassert(saImmOiImplementerSet(immOiHandle, implementerName), SA_AIS_OK);
    rc = saImmOiObjectImplementerSet(immOiHandle, &nonExistentObjectName, SA_IMM_ONE);
    test_validate(rc, SA_AIS_ERR_NOT_EXIST);
    safassert(saImmOiFinalize(immOiHandle), SA_AIS_OK);
}

void saImmOiObjectImplementerSet_05(void)
{
    SaImmOiHandleT newhandle;
    const SaImmOiImplementerNameT implementerName = (SaImmOiImplementerNameT) __FUNCTION__;
    SaImmOiImplementerNameT implementerName2 = (SaImmOiImplementerNameT) __FILE__;
    const SaImmAdminOwnerNameT adminOwnerName = (SaImmAdminOwnerNameT) __FUNCTION__;

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
    safassert(config_class_create(immOmHandle), SA_AIS_OK);
    /* Create test object under root */
    safassert(saImmOmCcbObjectCreate_2(ccbHandle, "TestClassConfig", NULL, attrValues), SA_AIS_OK);
    safassert(saImmOmCcbApply(ccbHandle), SA_AIS_OK);

    safassert(saImmOiInitialize_2(&immOiHandle, &immOiCallbacks, &immVersion), SA_AIS_OK);
    safassert(saImmOiImplementerSet(immOiHandle, implementerName), SA_AIS_OK);
    safassert(saImmOiObjectImplementerSet(immOiHandle, &rdn, SA_IMM_ONE), SA_AIS_OK);

    safassert(saImmOiInitialize_2(&newhandle, &immOiCallbacks, &immVersion), SA_AIS_OK);
    safassert(saImmOiImplementerSet(newhandle, implementerName2), SA_AIS_OK);
    rc = saImmOiObjectImplementerSet(newhandle, &rdn, SA_IMM_ONE);

    safassert(saImmOiObjectImplementerRelease(immOiHandle, &rdn, SA_IMM_ONE), SA_AIS_OK);
    safassert(saImmOiFinalize(immOiHandle), SA_AIS_OK);
    safassert(saImmOiFinalize(newhandle), SA_AIS_OK);

    safassert(saImmOmCcbObjectDelete(ccbHandle, &rdn), SA_AIS_OK);
    safassert(saImmOmCcbApply(ccbHandle), SA_AIS_OK);
    safassert(saImmOmCcbFinalize(ccbHandle), SA_AIS_OK);
    safassert(config_class_delete(immOmHandle), SA_AIS_OK);
    safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);

    test_validate(rc, SA_AIS_ERR_EXIST);
}

void saImmOiObjectImplementerSet_06(void)
{
    SaImmOiHandleT newhandle;
    const SaImmOiImplementerNameT implementerName = (SaImmOiImplementerNameT) __FUNCTION__;
    SaImmOiImplementerNameT implementerName2 = (SaImmOiImplementerNameT) __FILE__;
    const SaImmAdminOwnerNameT adminOwnerName = (SaImmAdminOwnerNameT) __FUNCTION__;

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
    safassert(config_class_create(immOmHandle), SA_AIS_OK);
    /* Create test object under root */
    safassert(saImmOmCcbObjectCreate_2(ccbHandle, "TestClassConfig", NULL, attrValues), SA_AIS_OK);
    safassert(saImmOmCcbApply(ccbHandle), SA_AIS_OK);

    safassert(saImmOiInitialize_2(&immOiHandle, &immOiCallbacks, &immVersion), SA_AIS_OK);
    safassert(saImmOiImplementerSet(immOiHandle, implementerName), SA_AIS_OK);
    safassert(saImmOiClassImplementerSet(immOiHandle, configClassName), SA_AIS_OK);

    safassert(saImmOiInitialize_2(&newhandle, &immOiCallbacks, &immVersion), SA_AIS_OK);
    safassert(saImmOiImplementerSet(newhandle, implementerName2), SA_AIS_OK);
    rc = saImmOiObjectImplementerSet(newhandle, &rdn, SA_IMM_ONE);

    safassert(saImmOiClassImplementerRelease(immOiHandle, configClassName), SA_AIS_OK);
    safassert(saImmOiImplementerClear(newhandle), SA_AIS_OK);
    safassert(saImmOiImplementerClear(immOiHandle), SA_AIS_OK);
    safassert(saImmOiFinalize(immOiHandle), SA_AIS_OK);
    safassert(saImmOiFinalize(newhandle), SA_AIS_OK);

    safassert(saImmOmCcbObjectDelete(ccbHandle, &rdn), SA_AIS_OK);
    safassert(saImmOmCcbApply(ccbHandle), SA_AIS_OK);
    safassert(saImmOmCcbFinalize(ccbHandle), SA_AIS_OK);
    safassert(config_class_delete(immOmHandle), SA_AIS_OK);
    safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);

    test_validate(rc, SA_AIS_ERR_EXIST);
}

