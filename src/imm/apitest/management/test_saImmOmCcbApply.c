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

#include "imm/apitest/immtest.h"
#include <unistd.h>

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
    const SaNameT *objectNames[] = {&rootObj, NULL};
    const SaNameT objectName =
        {strlen("Obj1,rdn=root"), "Obj1,rdn=root"};
    safassert(saImmOmInitialize(&immOmHandle, &immOmCallbacks, &immVersion),
        SA_AIS_OK);
    safassert(saImmOmAdminOwnerInitialize(immOmHandle, adminOwnerName,
				  /*SA_TRUE*/SA_FALSE, &ownerHandle), SA_AIS_OK);
    safassert(saImmOmAdminOwnerSet(ownerHandle, objectNames, SA_IMM_ONE), 
        SA_AIS_OK);
    safassert(saImmOmCcbInitialize(ownerHandle, 0, &ccbHandle), SA_AIS_OK);
    safassert(saImmOmCcbObjectCreate_2(ccbHandle, configClassName,
        &rootObj, attrValues), SA_AIS_OK);

    test_validate(saImmOmCcbApply(ccbHandle), SA_AIS_OK);

    safassert(saImmOmCcbObjectDelete(ccbHandle, &objectName), SA_AIS_OK);
    safassert(saImmOmCcbApply(ccbHandle), SA_AIS_OK);
    safassert(saImmOmCcbFinalize(ccbHandle), SA_AIS_OK);
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
    const SaNameT *objectNames[] = {&rootObj, NULL};

    safassert(saImmOmInitialize(&immOmHandle, &immOmCallbacks, &immVersion), SA_AIS_OK);
    safassert(saImmOmAdminOwnerInitialize(immOmHandle, adminOwnerName,
        SA_TRUE, &ownerHandle), SA_AIS_OK);
    safassert(saImmOmAdminOwnerSet(ownerHandle, objectNames, SA_IMM_ONE), SA_AIS_OK);
    safassert(saImmOmCcbInitialize(ownerHandle, 0, &ccbHandle), SA_AIS_OK);
    safassert(saImmOmCcbObjectCreate_2(ccbHandle, configClassName,
        &rootObj, attrValues), SA_AIS_OK);

    /* ccbHandle is invalid */
    if ((rc = saImmOmCcbApply(-1)) != SA_AIS_ERR_BAD_HANDLE)
        goto done;

    safassert(saImmOmCcbFinalize(ccbHandle), SA_AIS_OK);

    /* ccbHandle has already been finalized. */
    if ((rc = saImmOmCcbApply(ccbHandle)) != SA_AIS_ERR_BAD_HANDLE)
        goto done;

done:
    test_validate(rc, SA_AIS_ERR_BAD_HANDLE);
    safassert(saImmOmAdminOwnerFinalize(ownerHandle), SA_AIS_OK);
    safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);
}

void saImmOmCcbAbort_01(void)
{
    TRACE_ENTER();
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
    const SaNameT *objectNames[] = {&rootObj, NULL};

    safassert(saImmOmInitialize(&immOmHandle, &immOmCallbacks, &immVersion), SA_AIS_OK);
    safassert(saImmOmAdminOwnerInitialize(immOmHandle, adminOwnerName,
        SA_TRUE, &ownerHandle), SA_AIS_OK);
    safassert(saImmOmAdminOwnerSet(ownerHandle, objectNames, SA_IMM_ONE), SA_AIS_OK);
    safassert(saImmOmCcbInitialize(ownerHandle, 0, &ccbHandle), SA_AIS_OK);
    safassert(saImmOmCcbAbort(ccbHandle), SA_AIS_OK); /* Abort on pristine ccb handle should be ok. */
    safassert(saImmOmCcbObjectCreate_2(ccbHandle, configClassName,
        &rootObj, attrValues), SA_AIS_OK);

    /* abort the ccb */
    if ((rc = saImmOmCcbAbort(ccbHandle)) != SA_AIS_OK)
        goto done;

    safassert(saImmOmCcbAbort(ccbHandle), SA_AIS_OK); /* Redundant abort should be ok. */

    safassert(saImmOmCcbFinalize(ccbHandle), SA_AIS_OK);

done:
    test_validate(rc, SA_AIS_OK);
    safassert(saImmOmAdminOwnerFinalize(ownerHandle), SA_AIS_OK);
    safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);
    TRACE_LEAVE();
}

void saImmOmCcbAbort_02(void)
{
    TRACE_ENTER();
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
    const SaNameT *objectNames[] = {&rootObj, NULL};
    const SaNameT objectName =
        {strlen("Obj1,rdn=root"), "Obj1,rdn=root"};

    safassert(saImmOmInitialize(&immOmHandle, &immOmCallbacks, &immVersion), SA_AIS_OK);
    safassert(saImmOmAdminOwnerInitialize(immOmHandle, adminOwnerName,
        SA_TRUE, &ownerHandle), SA_AIS_OK);
    safassert(saImmOmAdminOwnerSet(ownerHandle, objectNames, SA_IMM_ONE), SA_AIS_OK);
    safassert(saImmOmCcbInitialize(ownerHandle, 0, &ccbHandle), SA_AIS_OK);
    safassert(saImmOmCcbObjectCreate_2(ccbHandle, configClassName,
        &rootObj, attrValues), SA_AIS_OK);

    /* abort the ccb with the object-create. */
    if ((rc = saImmOmCcbAbort(ccbHandle)) != SA_AIS_OK)
        goto done;

    /* continue with ccb handle after abort. New attempt to create same object
       should work. */
    if ((rc = saImmOmCcbObjectCreate_2(ccbHandle, configClassName, &rootObj,
        attrValues) != SA_AIS_OK)) {
        goto done;
    }

    /* apply should work. */
    if((rc = saImmOmCcbApply(ccbHandle)) != SA_AIS_OK)
        goto done;

    /* Set up delete and abort it. */
    safassert(saImmOmCcbObjectDelete(ccbHandle, &objectName), SA_AIS_OK);
    safassert(saImmOmCcbAbort(ccbHandle), SA_AIS_OK);

    /* Finaly use ccb-handle to delete object. */
    safassert(saImmOmCcbObjectDelete(ccbHandle, &objectName), SA_AIS_OK);
    safassert(saImmOmCcbApply(ccbHandle), SA_AIS_OK);
    safassert(saImmOmCcbFinalize(ccbHandle), SA_AIS_OK);

done:
    test_validate(rc, SA_AIS_OK);
    safassert(saImmOmAdminOwnerFinalize(ownerHandle), SA_AIS_OK);
    safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);
    TRACE_LEAVE();
}

void saImmOmCcbValidate_01(void)
{
    TRACE_ENTER();
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
    const SaNameT *objectNames[] = {&rootObj, NULL};

    safassert(saImmOmInitialize(&immOmHandle, &immOmCallbacks, &immVersion), SA_AIS_OK);
    safassert(saImmOmAdminOwnerInitialize(immOmHandle, adminOwnerName,
        SA_TRUE, &ownerHandle), SA_AIS_OK);
    safassert(saImmOmAdminOwnerSet(ownerHandle, objectNames, SA_IMM_ONE), SA_AIS_OK);
    safassert(saImmOmCcbInitialize(ownerHandle, 0, &ccbHandle), SA_AIS_OK);
    safassert(saImmOmCcbObjectCreate_2(ccbHandle, configClassName,
        &rootObj, attrValues), SA_AIS_OK);

    /* validate the ccb */
    if ((rc = saImmOmCcbValidate(ccbHandle)) != SA_AIS_OK)
	    goto done;

    if ((rc = saImmOmCcbAbort(ccbHandle)) != SA_AIS_OK) /* Abort after valiadtion should be ok */
	    goto done;

    if ((rc = saImmOmCcbApply(ccbHandle)) != SA_AIS_OK) /* Aply after abort (empty ccb) should be ok */
	    goto done;

    safassert(saImmOmCcbFinalize(ccbHandle), SA_AIS_OK);

done:
    test_validate(rc, SA_AIS_OK);
    safassert(saImmOmAdminOwnerFinalize(ownerHandle), SA_AIS_OK);
    safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);
    TRACE_LEAVE();
}

void saImmOmCcbValidate_02(void)
{
    TRACE_ENTER();
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
    const SaNameT *objectNames[] = {&rootObj, NULL};
    const SaNameT objectName =
        {strlen("Obj1,rdn=root"), "Obj1,rdn=root"};

    safassert(saImmOmInitialize(&immOmHandle, &immOmCallbacks, &immVersion), SA_AIS_OK);
    safassert(saImmOmAdminOwnerInitialize(immOmHandle, adminOwnerName,
        SA_TRUE, &ownerHandle), SA_AIS_OK);
    safassert(saImmOmAdminOwnerSet(ownerHandle, objectNames, SA_IMM_ONE), SA_AIS_OK);
    safassert(saImmOmCcbInitialize(ownerHandle, 0, &ccbHandle), SA_AIS_OK);
    safassert(saImmOmCcbObjectCreate_2(ccbHandle, configClassName,
        &rootObj, attrValues), SA_AIS_OK);

    /* validate the ccb */
    if ((rc = saImmOmCcbValidate(ccbHandle)) != SA_AIS_OK)
	    goto done;

    if ((rc = saImmOmCcbApply(ccbHandle)) != SA_AIS_OK) /* Aply after validate (normal case) should be ok */
	    goto done;

    if ((rc = saImmOmCcbAbort(ccbHandle)) != SA_AIS_OK) /* Abort after apply (empty ccb) should be ok */
	    goto done;

    safassert(saImmOmCcbObjectDelete(ccbHandle, &objectName), SA_AIS_OK);
    safassert(saImmOmCcbApply(ccbHandle), SA_AIS_OK); 

    safassert(saImmOmCcbFinalize(ccbHandle), SA_AIS_OK);

done:
    test_validate(rc, SA_AIS_OK);
    safassert(saImmOmAdminOwnerFinalize(ownerHandle), SA_AIS_OK);
    safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);
    TRACE_LEAVE();
}
void saImmOmCcbAbort_03(void)
{
    TRACE_ENTER();
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
    const SaNameT *objectNames[] = {&rootObj, NULL};
    const SaNameT objectName =
        {strlen("Obj1,rdn=root"), "Obj1,rdn=root"};
    SaVersionT immVersion = {'A', 0x02, 0x0d};

    safassert(saImmOmInitialize(&immOmHandle, &immOmCallbacks, &immVersion), SA_AIS_OK);
    safassert(saImmOmAdminOwnerInitialize(immOmHandle, adminOwnerName,
        SA_TRUE, &ownerHandle), SA_AIS_OK);
    safassert(saImmOmAdminOwnerSet(ownerHandle, objectNames, SA_IMM_ONE), SA_AIS_OK);
    safassert(saImmOmCcbInitialize(ownerHandle, 0, &ccbHandle), SA_AIS_OK);
    safassert(saImmOmCcbObjectCreate_2(ccbHandle, configClassName,
        &rootObj, attrValues), SA_AIS_OK);

    /* validate the ccb */
    if ((rc = saImmOmCcbApply(ccbHandle)) != SA_AIS_OK) /* Aply after validate (normal case) should be ok */
	    goto done;

    test_validate(saImmOmCcbAbort(ccbHandle), SA_AIS_ERR_VERSION);

    safassert(saImmOmCcbInitialize(ownerHandle, 0, &ccbHandle), SA_AIS_OK);
    safassert(saImmOmCcbObjectDelete(ccbHandle, &objectName), SA_AIS_OK);
    safassert(saImmOmCcbApply(ccbHandle), SA_AIS_OK);

    safassert(saImmOmCcbFinalize(ccbHandle), SA_AIS_OK);

done:
    safassert(saImmOmAdminOwnerFinalize(ownerHandle), SA_AIS_OK);
    safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);
    TRACE_LEAVE();
}

