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

void saImmOmCcbApply_01(void)
{
    const SaImmAdminOwnerNameT adminOwnerName =
        (SaImmAdminOwnerNameT) __FUNCTION__;
    SaImmAdminOwnerHandleT ownerHandle;
    SaImmCcbHandleT ccbHandle;
    SaNameT rdn = {strlen("Obj1"), "Obj1"};
    SaNameT* nameValues[] = {&rdn};
    SaImmAttrValuesT_2 v2 =
        {"rdn",  SA_IMM_ATTR_SANAMET, 1, (void**)nameValues};
    SaUint32T  int1Value1 = 7;
    SaUint32T* int1Values[] = {&int1Value1};
    SaImmAttrValuesT_2 v1 =
        {"attr1", SA_IMM_ATTR_SAUINT32T, 1, (void**)int1Values};
    const SaImmAttrValuesT_2 * attrValues[] = {&v1, &v2, NULL};
    const SaNameT parentName = {strlen("opensafImm=opensafImm,safApp=safImmService"), "opensafImm=opensafImm,safApp=safImmService"};
    const SaNameT *objectNames[] = {&parentName, NULL};
    const SaNameT objectName =
        {strlen("Obj1,opensafImm=opensafImm,safApp=safImmService"), "Obj1,opensafImm=opensafImm,safApp=safImmService"};
    safassert(saImmOmInitialize(&immOmHandle, &immOmCallbacks, &immVersion),
        SA_AIS_OK);
    safassert(config_class_create(immOmHandle), SA_AIS_OK);
    safassert(saImmOmAdminOwnerInitialize(immOmHandle, adminOwnerName,
				  /*SA_TRUE*/SA_FALSE, &ownerHandle), SA_AIS_OK);
    safassert(saImmOmAdminOwnerSet(ownerHandle, objectNames, SA_IMM_ONE), 
        SA_AIS_OK);
    safassert(saImmOmCcbInitialize(ownerHandle, 0, &ccbHandle), SA_AIS_OK);
    safassert(saImmOmCcbObjectCreate_2(ccbHandle, configClassName,
        &parentName, attrValues), SA_AIS_OK);

    test_validate(saImmOmCcbApply(ccbHandle), SA_AIS_OK);

    safassert(saImmOmCcbObjectDelete(ccbHandle, &objectName), SA_AIS_OK);
    safassert(saImmOmCcbApply(ccbHandle), SA_AIS_OK);
    safassert(saImmOmCcbFinalize(ccbHandle), SA_AIS_OK);
    safassert(config_class_delete(immOmHandle), SA_AIS_OK);
    safassert(saImmOmAdminOwnerRelease(ownerHandle, objectNames, SA_IMM_ONE),
		SA_AIS_OK);
    safassert(saImmOmAdminOwnerFinalize(ownerHandle), SA_AIS_OK);
    safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);
}

void saImmOmCcbApply_02(void)
{
    const SaImmAdminOwnerNameT adminOwnerName = (SaImmAdminOwnerNameT) __FUNCTION__;
    SaImmAdminOwnerHandleT ownerHandle;
    SaImmCcbHandleT ccbHandle;
    SaNameT rdn = {strlen("Obj1"), "Obj1"};
    SaNameT* nameValues[] = {&rdn};
    SaImmAttrValuesT_2 v2 = {"rdn",  SA_IMM_ATTR_SANAMET, 1, (void**)nameValues};
    SaUint32T  int1Value1 = 7;
    SaUint32T* int1Values[] = {&int1Value1};
    SaImmAttrValuesT_2 v1 = {"attr1", SA_IMM_ATTR_SAUINT32T, 1, (void**)int1Values};
    const SaImmAttrValuesT_2 * attrValues[] = {&v1, &v2, NULL};
    const SaNameT parentName = {strlen("opensafImm=opensafImm,safApp=safImmService"), "opensafImm=opensafImm,safApp=safImmService"};
    const SaNameT *objectNames[] = {&parentName, NULL};

    safassert(saImmOmInitialize(&immOmHandle, &immOmCallbacks, &immVersion), SA_AIS_OK);
    safassert(config_class_create(immOmHandle), SA_AIS_OK);
    safassert(saImmOmAdminOwnerInitialize(immOmHandle, adminOwnerName,
        SA_TRUE, &ownerHandle), SA_AIS_OK);
    safassert(saImmOmAdminOwnerSet(ownerHandle, objectNames, SA_IMM_ONE), SA_AIS_OK);
    safassert(saImmOmCcbInitialize(ownerHandle, 0, &ccbHandle), SA_AIS_OK);
    safassert(saImmOmCcbObjectCreate_2(ccbHandle, configClassName,
        &parentName, attrValues), SA_AIS_OK);

    /* ccbHandle is invalid */
    if ((rc = saImmOmCcbApply(-1)) != SA_AIS_ERR_BAD_HANDLE)
        goto done;

    safassert(saImmOmCcbFinalize(ccbHandle), SA_AIS_OK);

    /* ccbHandle has already been finalized. */
    if ((rc = saImmOmCcbApply(ccbHandle)) != SA_AIS_ERR_BAD_HANDLE)
        goto done;

done:
    test_validate(rc, SA_AIS_ERR_BAD_HANDLE);
    safassert(config_class_delete(immOmHandle), SA_AIS_OK);
    safassert(saImmOmAdminOwnerFinalize(ownerHandle), SA_AIS_OK);
    safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);
}

