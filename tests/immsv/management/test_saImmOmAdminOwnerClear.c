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

void saImmOmAdminOwnerClear_01(void)
{
    const SaImmAdminOwnerNameT adminOwnerName = (SaImmAdminOwnerNameT) __FUNCTION__;
    SaImmAdminOwnerHandleT ownerHandle;
    const SaNameT *objectNames[] = {&rootObj, NULL};

    safassert(saImmOmInitialize(&immOmHandle, &immOmCallbacks, &immVersion), SA_AIS_OK);

    /* releaseOwnershipOnFinalize == false */
    safassert(saImmOmAdminOwnerInitialize(immOmHandle, adminOwnerName, SA_FALSE, &ownerHandle), SA_AIS_OK);
    safassert(saImmOmAdminOwnerSet(ownerHandle, objectNames, SA_IMM_ONE), SA_AIS_OK);
    if (saImmOmAdminOwnerClear(immOmHandle, objectNames, SA_IMM_ONE) != SA_AIS_OK)
        goto done;
    safassert(saImmOmAdminOwnerFinalize(ownerHandle), SA_AIS_OK);

    /* releaseOwnershipOnFinalize == true (old bug) */
    safassert(saImmOmAdminOwnerInitialize(immOmHandle, adminOwnerName, SA_TRUE, &ownerHandle), SA_AIS_OK);
    safassert(saImmOmAdminOwnerSet(ownerHandle, objectNames, SA_IMM_ONE), SA_AIS_OK);
    rc = saImmOmAdminOwnerClear(immOmHandle, objectNames, SA_IMM_ONE);

done:
    test_validate(rc, SA_AIS_OK);
    safassert(saImmOmAdminOwnerFinalize(ownerHandle), SA_AIS_OK);
    safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);
}

void saImmOmAdminOwnerClear_02(void)
{
    const SaImmAdminOwnerNameT adminOwnerName = (SaImmAdminOwnerNameT) __FUNCTION__;
    SaImmAdminOwnerHandleT ownerHandle;
    const SaNameT *objectNames[] = {&rootObj, NULL};

    safassert(saImmOmInitialize(&immOmHandle, &immOmCallbacks, &immVersion), SA_AIS_OK);
    safassert(saImmOmAdminOwnerInitialize(immOmHandle, adminOwnerName, SA_TRUE, &ownerHandle), SA_AIS_OK);
    safassert(saImmOmAdminOwnerSet(ownerHandle, objectNames, SA_IMM_ONE), SA_AIS_OK);

    test_validate(saImmOmAdminOwnerClear(-1, objectNames, SA_IMM_ONE), SA_AIS_ERR_BAD_HANDLE);

    safassert(saImmOmAdminOwnerClear(immOmHandle, objectNames, SA_IMM_ONE), SA_AIS_OK);
    safassert(saImmOmAdminOwnerFinalize(ownerHandle), SA_AIS_OK);
    safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);
}

void saImmOmAdminOwnerClear_03(void)
{
    const SaImmAdminOwnerNameT adminOwnerName = (SaImmAdminOwnerNameT) __FUNCTION__;
    SaImmAdminOwnerHandleT ownerHandle;
    const SaNameT *objectNames[] = {&rootObj, NULL};

    safassert(saImmOmInitialize(&immOmHandle, &immOmCallbacks, &immVersion), SA_AIS_OK);
    safassert(saImmOmAdminOwnerInitialize(immOmHandle, adminOwnerName, SA_TRUE, &ownerHandle), SA_AIS_OK);
    safassert(saImmOmAdminOwnerSet(ownerHandle, objectNames, SA_IMM_ONE), SA_AIS_OK);

    test_validate(saImmOmAdminOwnerClear(immOmHandle, objectNames, -1), SA_AIS_ERR_INVALID_PARAM);

    safassert(saImmOmAdminOwnerClear(immOmHandle, objectNames, SA_IMM_ONE), SA_AIS_OK);
    safassert(saImmOmAdminOwnerFinalize(ownerHandle), SA_AIS_OK);
    safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);
}

void saImmOmAdminOwnerClear_04(void)
{
    const SaImmAdminOwnerNameT adminOwnerName = (SaImmAdminOwnerNameT) __FUNCTION__;
    SaImmAdminOwnerHandleT ownerHandle;
    const SaNameT objectName = {strlen("nonExistingObject"), "nonExistingObject"};
    const SaNameT *objectNames1[] = {&rootObj, NULL};
    const SaNameT *objectNames2[] = {&rootObj, &objectName, NULL};
    const SaNameT *objectNames3[] = {&objectName, &rootObj, NULL};

    safassert(saImmOmInitialize(&immOmHandle, &immOmCallbacks, &immVersion), SA_AIS_OK);
    safassert(saImmOmAdminOwnerInitialize(immOmHandle, adminOwnerName, SA_TRUE, &ownerHandle), SA_AIS_OK);
    safassert(saImmOmAdminOwnerSet(ownerHandle, objectNames1, SA_IMM_ONE), SA_AIS_OK);

    rc = saImmOmAdminOwnerClear(immOmHandle, objectNames2, SA_IMM_ONE);
    if (rc != SA_AIS_ERR_NOT_EXIST)
    {
        TRACE_ENTER();
        goto done;
    }

    /* Check some permutations */
    rc = saImmOmAdminOwnerSet(ownerHandle, objectNames3, SA_IMM_ONE);

done:
    test_validate(rc, SA_AIS_ERR_NOT_EXIST);
    safassert(saImmOmAdminOwnerFinalize(ownerHandle), SA_AIS_OK);
    safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);
}

