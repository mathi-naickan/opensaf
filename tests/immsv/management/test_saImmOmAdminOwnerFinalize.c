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

void saImmOmAdminOwnerFinalize_01(void)
{
    const SaImmAdminOwnerNameT adminOwnerName = (SaImmAdminOwnerNameT) __FUNCTION__;
    SaImmAdminOwnerHandleT ownerHandle;

    safassert(saImmOmInitialize(&immOmHandle, &immOmCallbacks, &immVersion), SA_AIS_OK);
    safassert(saImmOmAdminOwnerInitialize(immOmHandle, adminOwnerName, SA_TRUE, &ownerHandle), SA_AIS_OK);
    test_validate(saImmOmAdminOwnerFinalize(ownerHandle), SA_AIS_OK);
    safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);
}

void saImmOmAdminOwnerFinalize_02(void)
{
    const SaImmAdminOwnerNameT adminOwnerName = (SaImmAdminOwnerNameT) __FUNCTION__;
    SaImmAdminOwnerHandleT ownerHandle;

    safassert(saImmOmInitialize(&immOmHandle, &immOmCallbacks, &immVersion), SA_AIS_OK);

    /* handle corrupted or uninitialized */
    rc = saImmOmAdminOwnerFinalize(-1);
    if (rc != SA_AIS_ERR_BAD_HANDLE)
        goto done;

    /* handle already finalized */
    safassert(saImmOmAdminOwnerInitialize(immOmHandle, adminOwnerName, SA_TRUE, &ownerHandle), SA_AIS_OK);
    safassert(saImmOmAdminOwnerFinalize(ownerHandle), SA_AIS_OK);
    rc = saImmOmAdminOwnerFinalize(ownerHandle);
    safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);
done:
    test_validate(rc, SA_AIS_ERR_BAD_HANDLE);
}

void saImmOmAdminOwnerFinalize_03(void)
{
    /* test release on finalize after someone else has done clear and set */
    const SaImmAdminOwnerNameT adminOwnerName = (SaImmAdminOwnerNameT) __FUNCTION__;
    const SaImmAdminOwnerNameT adminOwnerName2 = (SaImmAdminOwnerNameT) "RudeGuyAdminOwner";
    SaImmAdminOwnerHandleT ownerHandle;
    SaImmAdminOwnerHandleT rudeGuyHandle;
    const SaNameT *objectNames[] = {&rootObj, NULL};

    /* setup */
    safassert(saImmOmInitialize(&immOmHandle, &immOmCallbacks, &immVersion), SA_AIS_OK);
    safassert(saImmOmAdminOwnerInitialize(immOmHandle, adminOwnerName, SA_TRUE, &ownerHandle), SA_AIS_OK);
    safassert(saImmOmAdminOwnerSet(ownerHandle, objectNames, SA_IMM_ONE), SA_AIS_OK);

    /* rude guy interferes */
    safassert(saImmOmAdminOwnerInitialize(immOmHandle, adminOwnerName2, SA_TRUE, &rudeGuyHandle), SA_AIS_OK);
    safassert(saImmOmAdminOwnerClear(immOmHandle, objectNames, SA_IMM_ONE), SA_AIS_OK);
    safassert(saImmOmAdminOwnerSet(rudeGuyHandle, objectNames, SA_IMM_ONE), SA_AIS_OK);

    /* Now test finalizing the original users handle. */
    test_validate(saImmOmAdminOwnerFinalize(ownerHandle), SA_AIS_OK);

    /* Cleanup */ 
    safassert(saImmOmAdminOwnerFinalize(rudeGuyHandle), SA_AIS_OK);
    safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);
}
