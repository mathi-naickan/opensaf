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

void saImmOmAdminOwnerSet_01(void)
{
    const SaImmAdminOwnerNameT adminOwnerName = (SaImmAdminOwnerNameT) __FUNCTION__;
    SaImmAdminOwnerHandleT ownerHandle;
    const SaNameT *objectNames[] = {&rootObj, NULL};

    safassert(saImmOmInitialize(&immOmHandle, &immOmCallbacks, &immVersion), SA_AIS_OK);
    safassert(saImmOmAdminOwnerInitialize(immOmHandle, adminOwnerName, SA_TRUE, &ownerHandle), SA_AIS_OK);
    test_validate(saImmOmAdminOwnerSet(ownerHandle, objectNames, SA_IMM_ONE), SA_AIS_OK);
    safassert(saImmOmAdminOwnerFinalize(ownerHandle), SA_AIS_OK);
    safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);
}

void saImmOmAdminOwnerSet_02(void)
{
    const SaImmAdminOwnerNameT adminOwnerName = (SaImmAdminOwnerNameT) __FUNCTION__;
    SaImmAdminOwnerHandleT ownerHandle;
    const SaNameT *objectNames[] = {&rootObj, NULL};

    safassert(saImmOmInitialize(&immOmHandle, &immOmCallbacks, &immVersion), SA_AIS_OK);
    safassert(saImmOmAdminOwnerInitialize(immOmHandle, adminOwnerName, SA_TRUE, &ownerHandle), SA_AIS_OK);
    test_validate(saImmOmAdminOwnerSet(-1, objectNames, SA_IMM_ONE), SA_AIS_ERR_BAD_HANDLE);
    safassert(saImmOmAdminOwnerFinalize(ownerHandle), SA_AIS_OK);
    safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);
}

void saImmOmAdminOwnerSet_03(void)
{
    const SaImmAdminOwnerNameT adminOwnerName = (SaImmAdminOwnerNameT) __FUNCTION__;
    SaImmAdminOwnerHandleT ownerHandle;
    const SaNameT *objectNames[] = {&rootObj, NULL};

    safassert(saImmOmInitialize(&immOmHandle, &immOmCallbacks, &immVersion), SA_AIS_OK);
    safassert(saImmOmAdminOwnerInitialize(immOmHandle, adminOwnerName, SA_TRUE, &ownerHandle), SA_AIS_OK);
    /* Invalid scope */
    test_validate(saImmOmAdminOwnerSet(ownerHandle, objectNames, -1), SA_AIS_ERR_INVALID_PARAM);
    safassert(saImmOmAdminOwnerFinalize(ownerHandle), SA_AIS_OK);
    safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);
}

void saImmOmAdminOwnerSet_04(void)
{
    const SaImmAdminOwnerNameT adminOwnerName = (SaImmAdminOwnerNameT) __FUNCTION__;
    SaImmAdminOwnerHandleT ownerHandle;
    const SaNameT *objectNames[] = {NULL};

    safassert(saImmOmInitialize(&immOmHandle, &immOmCallbacks, &immVersion), SA_AIS_OK);
    safassert(saImmOmAdminOwnerInitialize(immOmHandle, adminOwnerName, SA_TRUE, &ownerHandle), SA_AIS_OK);
    /* No object names */
    test_validate(saImmOmAdminOwnerSet(ownerHandle, objectNames, SA_IMM_ONE), SA_AIS_ERR_INVALID_PARAM);
    safassert(saImmOmAdminOwnerFinalize(ownerHandle), SA_AIS_OK);
    safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);
}

void saImmOmAdminOwnerSet_05(void)
{
    const SaImmAdminOwnerNameT adminOwnerName = (SaImmAdminOwnerNameT) __FUNCTION__;
    SaImmAdminOwnerHandleT ownerHandle;
    const SaNameT objectName = {strlen("nonExistingObject"), "nonExistingObject"};
    const SaNameT *objectNames1[] = {&objectName, NULL};
    const SaNameT *objectNames2[] = {&rootObj, &objectName, NULL};
    const SaNameT *objectNames3[] = {&objectName, &rootObj, NULL};

    safassert(saImmOmInitialize(&immOmHandle, &immOmCallbacks, &immVersion), SA_AIS_OK);
    safassert(saImmOmAdminOwnerInitialize(immOmHandle, adminOwnerName, SA_TRUE, &ownerHandle), SA_AIS_OK);

    rc = saImmOmAdminOwnerSet(ownerHandle, objectNames1, SA_IMM_ONE);
    if (rc != SA_AIS_ERR_NOT_EXIST)
    {
        TRACE_ENTER();
        goto done;
    }

    /* Check some permutations */
    rc = saImmOmAdminOwnerSet(ownerHandle, objectNames2, SA_IMM_ONE);
    if (rc != SA_AIS_ERR_NOT_EXIST)
    {
        TRACE_ENTER();
        goto done;
    }

    rc = saImmOmAdminOwnerSet(ownerHandle, objectNames3, SA_IMM_ONE);

done:
    test_validate(rc, SA_AIS_ERR_NOT_EXIST);
    safassert(saImmOmAdminOwnerFinalize(ownerHandle), SA_AIS_OK);
    safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);
}

void saImmOmAdminOwnerSet_06(void)
{
    const SaImmAdminOwnerNameT adminOwnerName1 = (SaImmAdminOwnerNameT) "adminOwnerName1";
    SaImmAdminOwnerHandleT ownerHandle1;
    const SaImmAdminOwnerNameT adminOwnerName2 = (SaImmAdminOwnerNameT) "adminOwnerName2";
    SaImmAdminOwnerHandleT ownerHandle2;
    const SaNameT objectName = {strlen("safApp=safLogService"), "safApp=safLogService"};
    const SaNameT *objectNames1[] = {&rootObj, NULL};
    const SaNameT *objectNames2[] = {&rootObj, &objectName, NULL};
    const SaNameT *objectNames3[] = {&objectName, &rootObj, NULL};

    safassert(saImmOmInitialize(&immOmHandle, &immOmCallbacks, &immVersion), SA_AIS_OK);
    safassert(saImmOmAdminOwnerInitialize(immOmHandle, adminOwnerName1, SA_TRUE, &ownerHandle1), SA_AIS_OK);
    safassert(saImmOmAdminOwnerInitialize(immOmHandle, adminOwnerName2, SA_TRUE, &ownerHandle2), SA_AIS_OK);

    safassert(saImmOmAdminOwnerSet(ownerHandle1, objectNames1, SA_IMM_ONE), SA_AIS_OK);
    rc = saImmOmAdminOwnerSet(ownerHandle2, objectNames1, SA_IMM_ONE);
    if (rc != SA_AIS_ERR_EXIST)
        goto done;
    safassert(saImmOmAdminOwnerFinalize(ownerHandle1), SA_AIS_OK);

    safassert(saImmOmAdminOwnerInitialize(immOmHandle, adminOwnerName1, SA_TRUE, &ownerHandle1), SA_AIS_OK);
    safassert(saImmOmAdminOwnerSet(ownerHandle1, objectNames2, SA_IMM_ONE), SA_AIS_OK);
    rc = saImmOmAdminOwnerSet(ownerHandle2, objectNames2, SA_IMM_ONE);
    if (rc != SA_AIS_ERR_EXIST)
        goto done;
    safassert(saImmOmAdminOwnerFinalize(ownerHandle1), SA_AIS_OK);

    safassert(saImmOmAdminOwnerInitialize(immOmHandle, adminOwnerName1, SA_TRUE, &ownerHandle1), SA_AIS_OK);
    safassert(saImmOmAdminOwnerSet(ownerHandle1, objectNames3, SA_IMM_ONE), SA_AIS_OK);
    rc = saImmOmAdminOwnerSet(ownerHandle2, objectNames3, SA_IMM_ONE);
    if (rc != SA_AIS_ERR_EXIST)
        goto done;

done:
    test_validate(rc, SA_AIS_ERR_EXIST);
    safassert(saImmOmAdminOwnerFinalize(ownerHandle1), SA_AIS_OK);
    safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);
}

void saImmOmAdminOwnerSet_07(void)
{
    const SaImmAdminOwnerNameT adminOwnerName = (SaImmAdminOwnerNameT) __FUNCTION__;
    SaImmAdminOwnerHandleT ownerHandle;
    const SaNameT *objectNames[] = {&rootObj, NULL};
    SaInvocationT inv = 4711;
    SaImmContinuationIdT cont = 0;
    SaImmAdminOperationIdT opId = 1;
    const SaImmAdminOperationParamsT_2* params[1] = {NULL};

    safassert(saImmOmInitialize_o2(&immOmHandle, &immOmA2bCallbacks, &immVersion), SA_AIS_OK);
    safassert(saImmOmAdminOwnerInitialize(immOmHandle, adminOwnerName, SA_TRUE, &ownerHandle), SA_AIS_OK);
    safassert(saImmOmAdminOwnerSet(ownerHandle, objectNames, SA_IMM_ONE), SA_AIS_OK);

    rc = saImmOmAdminOperationInvokeAsync_2(ownerHandle, inv, &rootObj, cont, opId, params);

    test_validate(rc, SA_AIS_ERR_INIT);

    safassert(saImmOmAdminOwnerFinalize(ownerHandle), SA_AIS_OK);
    safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);
}



