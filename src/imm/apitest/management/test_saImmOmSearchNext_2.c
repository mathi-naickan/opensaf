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

void saImmOmSearchNext_2_01(void)
{
    SaImmSearchHandleT searchHandle;
    SaNameT objectName;
    SaImmAttrValuesT_2 **attributes;

    safassert(saImmOmInitialize(&immOmHandle, &immOmCallbacks, &immVersion), SA_AIS_OK);
    safassert(saImmOmSearchInitialize_2(immOmHandle, NULL, SA_IMM_SUBTREE,
        SA_IMM_SEARCH_ONE_ATTR | SA_IMM_SEARCH_GET_NO_ATTR, NULL, NULL, &searchHandle), SA_AIS_OK);

    /* We should find at least one object */
    safassert(saImmOmSearchNext_2(searchHandle, &objectName, &attributes), SA_AIS_OK);

    do
        rc = saImmOmSearchNext_2(searchHandle, &objectName, &attributes);
    while (rc == SA_AIS_OK);

    /* Search should end with SA_AIS_ERR_NOT_EXIST */
    test_validate(rc, SA_AIS_ERR_NOT_EXIST);
    safassert(saImmOmSearchFinalize(searchHandle), SA_AIS_OK);
    safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);
}

void saImmOmSearchNext_2_02(void)
{
    SaImmSearchHandleT searchHandle;
    SaNameT objectName;
    SaImmAttrValuesT_2 **attributes;

    safassert(saImmOmInitialize(&immOmHandle, &immOmCallbacks, &immVersion), SA_AIS_OK);
    safassert(saImmOmSearchInitialize_2(immOmHandle, NULL, SA_IMM_SUBTREE,
        SA_IMM_SEARCH_ONE_ATTR | SA_IMM_SEARCH_GET_NO_ATTR, NULL, NULL, &searchHandle), SA_AIS_OK);

    rc = saImmOmSearchNext_2(-1, &objectName, &attributes);
    test_validate(rc, SA_AIS_ERR_BAD_HANDLE);
    safassert(saImmOmSearchFinalize(searchHandle), SA_AIS_OK);
    safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);
}

void saImmOmSearchNext_2_03(void)
{
    SaImmSearchHandleT searchHandle;
    SaNameT objectName;
    SaImmAttrValuesT_2 **attributes;

    safassert(saImmOmInitialize(&immOmHandle, &immOmCallbacks, &immVersion), SA_AIS_OK);
    safassert(saImmOmSearchInitialize_2(immOmHandle, NULL, SA_IMM_SUBTREE,
        (SA_IMM_SEARCH_ONE_ATTR | SA_IMM_SEARCH_GET_CONFIG_ATTR), NULL, NULL, &searchHandle), SA_AIS_OK);

    do {
        rc = saImmOmSearchNext_2(searchHandle, &objectName, &attributes);
    } while (rc == SA_AIS_OK);

    /* Search should end with SA_AIS_ERR_NOT_EXIST */
    test_validate(rc, SA_AIS_ERR_NOT_EXIST);
    safassert(saImmOmSearchFinalize(searchHandle), SA_AIS_OK);
    safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);
}
