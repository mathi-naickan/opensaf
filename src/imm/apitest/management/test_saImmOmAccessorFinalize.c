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

static SaImmAccessorHandleT accessorHandle;

void saImmOmAccessorFinalize_01(void)
{
    safassert(saImmOmInitialize(&immOmHandle, &immOmCallbacks, &immVersion), SA_AIS_OK);
    safassert(saImmOmAccessorInitialize(immOmHandle, &accessorHandle), SA_AIS_OK);
    rc = saImmOmAccessorFinalize(accessorHandle);
    test_validate(rc, SA_AIS_OK);
    safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);
}

void saImmOmAccessorFinalize_02(void)
{
    safassert(saImmOmInitialize(&immOmHandle, &immOmCallbacks, &immVersion), SA_AIS_OK);
    safassert(saImmOmAccessorInitialize(immOmHandle, &accessorHandle), SA_AIS_OK);
    rc = saImmOmAccessorFinalize(-1);
    test_validate(rc, SA_AIS_ERR_BAD_HANDLE);
    safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);
}

void saImmOmAccessorFinalize_03(void)
{
    safassert(saImmOmInitialize(&immOmHandle, &immOmCallbacks, &immVersion), SA_AIS_OK);
    safassert(saImmOmAccessorInitialize(immOmHandle, &accessorHandle), SA_AIS_OK);
    safassert(saImmOmAccessorFinalize(accessorHandle), SA_AIS_OK);
    rc = saImmOmAccessorFinalize(accessorHandle);
    test_validate(rc, SA_AIS_ERR_BAD_HANDLE);
    safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);
}

