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

void saImmOmCcbInitialize_01(void)
{
    const SaImmAdminOwnerNameT adminOwnerName = (SaImmAdminOwnerNameT) __FUNCTION__;
    SaImmAdminOwnerHandleT ownerHandle;
    SaImmCcbHandleT ccbHandle;

    safassert(saImmOmInitialize(&immOmHandle, &immOmCallbacks, &immVersion),
        SA_AIS_OK);
    safassert(saImmOmAdminOwnerInitialize(immOmHandle, adminOwnerName, SA_TRUE,
        &ownerHandle), SA_AIS_OK);

    test_validate(saImmOmCcbInitialize(ownerHandle, 0, &ccbHandle), SA_AIS_OK);

    safassert(saImmOmAdminOwnerFinalize(ownerHandle), SA_AIS_OK);
    safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);
}

void saImmOmCcbInitialize_02(void)
{
    const SaImmAdminOwnerNameT adminOwnerName = (SaImmAdminOwnerNameT) __FUNCTION__;
    SaImmAdminOwnerHandleT ownerHandle;
    SaImmCcbHandleT ccbHandle;

    safassert(saImmOmInitialize(&immOmHandle, &immOmCallbacks, &immVersion), SA_AIS_OK);
    safassert(saImmOmAdminOwnerInitialize(immOmHandle, adminOwnerName, SA_TRUE,
        &ownerHandle), SA_AIS_OK);
    safassert(saImmOmAdminOwnerFinalize(ownerHandle), SA_AIS_OK);

    /* already finalized ownerHandle */
    rc = saImmOmCcbInitialize(ownerHandle, 0, &ccbHandle);
    if (rc != SA_AIS_ERR_BAD_HANDLE)
        goto done;

    /* invalid ownerHandle */
    rc = saImmOmCcbInitialize(-1, 0, &ccbHandle);

done:
    test_validate(rc, SA_AIS_ERR_BAD_HANDLE);
    safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);
}

void saImmOmCcbInitialize_03(void)
{
    const SaImmAdminOwnerNameT adminOwnerName = (SaImmAdminOwnerNameT) __FUNCTION__;
    SaImmAdminOwnerHandleT ownerHandle;
    SaImmCcbHandleT ccbHandle;

    safassert(saImmOmInitialize(&immOmHandle, &immOmCallbacks, &immVersion), SA_AIS_OK);
    safassert(saImmOmAdminOwnerInitialize(immOmHandle, adminOwnerName, SA_TRUE,
        &ownerHandle), SA_AIS_OK);

    test_validate(saImmOmCcbInitialize(ownerHandle, -1, &ccbHandle),
        SA_AIS_ERR_INVALID_PARAM);
    safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);
}

void saImmOmCcbInitialize_04(void)
{
    const SaImmAdminOwnerNameT adminOwnerName = (SaImmAdminOwnerNameT) __FUNCTION__;
    SaImmAdminOwnerHandleT ownerHandle;
    SaImmCcbHandleT ccbHandle;

    safassert(saImmOmInitialize(&immOmHandle, &immOmCallbacks, &immVersion), SA_AIS_OK);
    safassert(saImmOmAdminOwnerInitialize(immOmHandle, adminOwnerName, SA_TRUE,
        &ownerHandle), SA_AIS_OK);

    safassert(saImmOmCcbInitialize(ownerHandle, 0, &ccbHandle), SA_AIS_OK);

    test_validate(saImmOmCcbGetErrorStrings(ccbHandle, NULL), SA_AIS_ERR_INVALID_PARAM);

    safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);
}

void saImmOmCcbInitialize_05(void)
{
    const SaImmAdminOwnerNameT adminOwnerName = (SaImmAdminOwnerNameT) __FUNCTION__;
    SaImmAdminOwnerHandleT ownerHandle;
    SaImmCcbHandleT ccbHandle;
    const SaStringT *errorStrings;

    safassert(saImmOmInitialize(&immOmHandle, &immOmCallbacks, &immVersion), SA_AIS_OK);
    safassert(saImmOmAdminOwnerInitialize(immOmHandle, adminOwnerName, SA_TRUE,
        &ownerHandle), SA_AIS_OK);

    safassert(saImmOmCcbInitialize(ownerHandle, 0, &ccbHandle), SA_AIS_OK);

    test_validate(saImmOmCcbGetErrorStrings(ccbHandle, &errorStrings), SA_AIS_OK);

    safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);
}


extern void saImmOmCcbObjectCreate_01(void);
extern void saImmOmCcbObjectCreate_02(void);
extern void saImmOmCcbObjectCreate_03(void);
extern void saImmOmCcbObjectCreate_04(void);
extern void saImmOmCcbObjectCreate_05(void);
extern void saImmOmCcbObjectCreate_06(void);
extern void saImmOmCcbObjectCreate_07(void);
extern void saImmOmCcbObjectCreate_08(void);
extern void saImmOmCcbObjectCreate_09(void);
extern void saImmOmCcbObjectCreate_10(void);
extern void saImmOmCcbObjectCreate_11(void);
extern void saImmOmCcbObjectCreate_12(void);
extern void saImmOmCcbObjectCreate_13(void);
extern void saImmOmCcbObjectCreate_14(void);
extern void saImmOmCcbObjectCreate_15(void);
extern void saImmOmCcbObjectCreate_16(void);
extern void saImmOmCcbObjectCreate_17(void);
extern void saImmOmCcbObjectCreate_18(void);
extern void saImmOmCcbObjectCreate_19(void);
extern void saImmOmCcbObjectCreate_20(void);
extern void saImmOmCcbObjectCreate_21(void);
extern void saImmOmCcbObjectDelete_01(void);
extern void saImmOmCcbObjectDelete_02(void);
extern void saImmOmCcbObjectDelete_03(void);
extern void saImmOmCcbObjectDelete_04(void);
extern void saImmOmCcbObjectDelete_05(void);
extern void saImmOmCcbObjectDelete_06(void);
extern void saImmOmCcbObjectDelete_07(void);
extern void saImmOmCcbObjectDelete_08(void);
extern void saImmOmCcbObjectDelete_09(void);
extern void saImmOmCcbObjectDelete_10(void);
extern void saImmOmCcbObjectModify_2_01(void);
extern void saImmOmCcbObjectModify_2_02(void);
extern void saImmOmCcbObjectModify_2_03(void);
extern void saImmOmCcbObjectModify_2_04(void);
extern void saImmOmCcbObjectModify_2_05(void);
extern void saImmOmCcbObjectModify_2_06(void);
extern void saImmOmCcbObjectModify_2_07(void);
extern void saImmOmCcbObjectModify_2_08(void);
extern void saImmOmCcbObjectModify_2_09(void);
extern void saImmOmCcbObjectModify_2_10(void);
extern void saImmOmCcbObjectModify_2_11(void);
extern void saImmOmCcbObjectModify_2_12(void);
extern void saImmOmCcbObjectModify_2_13(void);
extern void saImmOmCcbObjectModify_2_14(void);
extern void saImmOmCcbObjectModify_2_15(void);
extern void saImmOmCcbObjectModify_2_16(void);
extern void saImmOmCcbObjectModify_2_17(void);
extern void saImmOmCcbObjectModify_2_18(void);
extern void saImmOmCcbObjectModify_2_19(void);
extern void saImmOmCcbObjectModify_2_20(void);
extern void saImmOmCcbApply_01(void);
extern void saImmOmCcbApply_02(void);
extern void saImmOmCcbFinalize_01(void);
extern void saImmOmCcbFinalize_02(void);

__attribute__ ((constructor)) static void saImmOmInitialize_constructor(void)
{
    test_suite_add(6, "Configuration Changes");
    test_case_add(6, saImmOmCcbInitialize_01, "saImmOmCcbInitialize - SA_AIS_OK");
    test_case_add(6, saImmOmCcbInitialize_02, "saImmOmCcbInitialize - SA_AIS_ERR_BAD_HANDLE");
    test_case_add(6, saImmOmCcbInitialize_03, "saImmOmCcbInitialize - SA_AIS_ERR_INVALID_PARAM");

    test_case_add(6, saImmOmCcbObjectCreate_01, "saImmOmCcbObjectCreate - SA_AIS_OK");
    test_case_add(6, saImmOmCcbObjectCreate_02, "saImmOmCcbObjectCreate - SA_AIS_ERR_BAD_HANDLE");
    test_case_add(6, saImmOmCcbObjectCreate_03, "saImmOmCcbObjectCreate - SA_AIS_ERR_INVALID_PARAM");
    test_case_add(6, saImmOmCcbObjectCreate_04, "saImmOmCcbObjectCreate - SA_AIS_ERR_BAD_OPERATION");
    test_case_add(6, saImmOmCcbObjectCreate_05, "saImmOmCcbObjectCreate - SA_AIS_ERR_EXIST");
    test_case_add(6, saImmOmCcbObjectCreate_06, "saImmOmCcbObjectCreate - SA_AIS_ERR_NOT_EXIST");
    test_case_add(6, saImmOmCcbObjectCreate_07, "saImmOmCcbObjectCreate - SA_AIS_OK, association class");
    test_case_add(6, saImmOmCcbObjectCreate_08, "saImmOmCcbObjectCreate - SA_AIS_OK, empty SaStringT attribute");
    test_case_add(6, saImmOmCcbObjectCreate_09, "saImmOmCcbObjectCreate - SA_AIS_ERR_INVALID_PARAM, non-valid UTF-8 rdn string");
    test_case_add(6, saImmOmCcbObjectCreate_10, "saImmOmCcbObjectCreate - SA_AIS_ERR_INVALID_PARAM, valid UTF-8 non 7-bit ASCII rdn string");
    test_case_add(6, saImmOmCcbObjectCreate_11, "saImmOmCcbObjectCreate - SA_AIS_ERR_INVALID_PARAM, rdn contains control characters");
    test_case_add(6, saImmOmCcbObjectCreate_12, "saImmOmCcbObjectCreate - SA_AIS_ERR_INVALID_PARAM, rdn is empty string");
    test_case_add(6, saImmOmCcbObjectCreate_13, "saImmOmCcbObjectCreate - SA_AIS_OK, empty SaAnyT");
    test_case_add(6, saImmOmCcbObjectCreate_14, "saImmOmCcbObjectCreate - SA_AIS_OK, create object with NO_DANGLING attribute");
    test_case_add(6, saImmOmCcbObjectCreate_15, "saImmOmCcbObjectCreate - SA_AIS_OK, NO_DANGLING reference");
    test_case_add(6, saImmOmCcbObjectCreate_16, "saImmOmCcbObjectCreate - SA_AIS_ERR_FAILED_OPERATION, NO_DANGLING reference to a non-existing object");
    test_case_add(6, saImmOmCcbObjectCreate_17, "saImmOmCcbObjectCreate - SA_AIS_OK, NO_DANGLING - bidirectional references");
    test_case_add(6, saImmOmCcbObjectCreate_18, "saImmOmCcbObjectCreate - SA_AIS_ERR_BAD_OPERATION, create object with NO_DANGLING reference to a deleted object by the same CCB");
    test_case_add(6, saImmOmCcbObjectCreate_19, "saImmOmCcbObjectCreate - SA_AIS_ERR_BUSY, create object with NO_DANGLING reference to a deleted object by another CCB");
    test_case_add(6, saImmOmCcbObjectCreate_20, "saImmOmCcbObjectCreate - SA_AIS_ERR_BUSY, create object with NO_DANGLING reference to a created object by another CCB");
    test_case_add(6, saImmOmCcbObjectCreate_21, "saImmOmCcbObjectCreate - SA_AIS_ERR_INVALID_PARAM, create object with NO_DANGLING reference to non-persistence RTO");

    test_case_add(6, saImmOmCcbObjectDelete_01, "saImmOmCcbObjectDelete - SA_AIS_OK");
    test_case_add(6, saImmOmCcbObjectDelete_02, "saImmOmCcbObjectDelete - SA_AIS_ERR_BAD_HANDLE");
    test_case_add(6, saImmOmCcbObjectDelete_03, "saImmOmCcbObjectDelete - SA_AIS_ERR_BAD_OPERATION");
    test_case_add(6, saImmOmCcbObjectDelete_04, "saImmOmCcbObjectDelete - SA_AIS_ERR_NOT_EXIST");
    test_case_add(6, saImmOmCcbObjectDelete_05, "saImmOmCcbObjectDelete - SA_AIS_ERR_BUSY");
    test_case_add(6, saImmOmCcbObjectDelete_06, "saImmOmCcbObjectDelete - SA_AIS_OK, NO_DANGLING");
    test_case_add(6, saImmOmCcbObjectDelete_07, "saImmOmCcbObjectDelete - SA_AIS_OK, NO_DANGLING - cascade deleting");
    test_case_add(6, saImmOmCcbObjectDelete_08, "saImmOmCcbObjectDelete - SA_AIS_ERR_FAILED_OPERATION, delete NO_DANGLING reference");
    test_case_add(6, saImmOmCcbObjectDelete_09, "saImmOmCcbObjectDelete - SA_AIS_ERR_BUSY, NO_DANGLING - delete two objects with two CCBs");
    test_case_add(6, saImmOmCcbObjectDelete_10, "saImmOmCcbObjectDelete - SA_AIS_OK, NO_DANGLING - delete two objects with bidirectional references in the same CCB");

    test_case_add(6, saImmOmCcbObjectModify_2_01, "saImmOmCcbObjectModify_2 - SA_AIS_OK");
    test_case_add(6, saImmOmCcbObjectModify_2_02, "saImmOmCcbObjectModify_2 - SA_AIS_ERR_BAD_HANDLE");
    test_case_add(6, saImmOmCcbObjectModify_2_03, "saImmOmCcbObjectModify_2 - SA_AIS_ERR_INVALID_PARAM");
    test_case_add(6, saImmOmCcbObjectModify_2_04, "saImmOmCcbObjectModify_2 - SA_AIS_ERR_BAD_OPERATION");
    test_case_add(6, saImmOmCcbObjectModify_2_05, "saImmOmCcbObjectModify_2 - SA_AIS_ERR_NOT_EXIST");
    test_case_add(6, saImmOmCcbObjectModify_2_06, "saImmOmCcbObjectModify_2 - SA_AIS_ERR_BUSY");
    test_case_add(6, saImmOmCcbObjectModify_2_09, "saImmOmCcbObjectModify_2 - SA_AIS_ERR_INVALID_PARAM, non-valid UTF-8 string");
    test_case_add(6, saImmOmCcbObjectModify_2_10, "saImmOmCcbObjectModify_2 - SA_AIS_OK, UTF-8 string");
    test_case_add(6, saImmOmCcbObjectModify_2_11, "saImmOmCcbObjectModify_2 - SA_AIS_OK, control characters");
    test_case_add(6, saImmOmCcbObjectModify_2_12, "saImmOmCcbObjectModify_2 - SA_AIS_OK, empty string");
    test_case_add(6, saImmOmCcbObjectModify_2_13, "saImmOmCcbObjectModify_2 - SA_AIS_OK, empty SaAnyT");
    test_case_add(6, saImmOmCcbObjectModify_2_14, "saImmOmCcbObjectModify_2 - SA_AIS_ERR_INVALID_PARAM, duplicates not allowed");
    test_case_add(6, saImmOmCcbObjectModify_2_15, "saImmOmCcbObjectModify_2 - SA_AIS_OK, set NO_DANGLING reference");
    test_case_add(6, saImmOmCcbObjectModify_2_16, "saImmOmCcbObjectModify_2 - SA_AIS_OK, replace NO_DANGLING reference");
    test_case_add(6, saImmOmCcbObjectModify_2_17, "saImmOmCcbObjectModify_2 - SA_AIS_ERR_INVALID_PARAM, set NO_DANGLING reference to non-persistent RTO");
    test_case_add(6, saImmOmCcbObjectModify_2_18, "saImmOmCcbObjectModify_2 - SA_AIS_ERR_BAD_OPERATION, set NO_DANGLING reference to a deleted object by the same CCB");
    test_case_add(6, saImmOmCcbObjectModify_2_19, "saImmOmCcbObjectModify_2 - SA_AIS_ERR_BUSY, set NO_DANGLING reference to a deleted object by another CCB");
    test_case_add(6, saImmOmCcbObjectModify_2_20, "saImmOmCcbObjectModify_2 - SA_AIS_ERR_BUSY, set NO_DANGLING reference to a create object by another CCB");

    test_case_add(6, saImmOmCcbApply_01, "saImmOmCcbApply - SA_AIS_OK");
    test_case_add(6, saImmOmCcbApply_02, "saImmOmCcbApply - SA_AIS_ERR_BAD_HANDLE");

    test_case_add(6, saImmOmCcbFinalize_01, "saImmOmCcbFinalize - SA_AIS_OK");
    test_case_add(6, saImmOmCcbFinalize_02, "saImmOmCcbFinalize - SA_AIS_ERR_BAD_HANDLE");

    test_case_add(6, saImmOmCcbInitialize_04, "saImmOmCcbGetErrorStrings - SA_AIS_ERR_INVALID_PARAM");
    test_case_add(6, saImmOmCcbInitialize_05, "saImmOmCcbGetErrorStrings - SA_AIS_OK");

    test_case_add(6, saImmOmCcbObjectModify_2_08, "saImmOmCcbObjectModify_2 SA_IMM_ATTR_DELETE multi/single - SA_AIS_OK");
    test_case_add(6, saImmOmCcbObjectModify_2_07, "saImmOmCcbObjectModify_2 SA_IMM_ATTR_DELETE multi/multi - SA_AIS_OK");
}

