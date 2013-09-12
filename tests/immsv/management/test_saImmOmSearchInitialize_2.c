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

void saImmOmSearchInitialize_2_01(void)
{
    SaImmSearchHandleT searchHandle;

    safassert(saImmOmInitialize(&immOmHandle, &immOmCallbacks, &immVersion), SA_AIS_OK);
    rc = saImmOmSearchInitialize_2(immOmHandle, NULL, SA_IMM_SUBTREE,
        SA_IMM_SEARCH_ONE_ATTR | SA_IMM_SEARCH_GET_NO_ATTR, NULL, NULL, &searchHandle);
    test_validate(rc, SA_AIS_OK);
    safassert(saImmOmSearchFinalize(searchHandle), SA_AIS_OK);
    safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);
}

void saImmOmSearchInitialize_2_02(void)
{
    SaImmSearchHandleT searchHandle;

    safassert(saImmOmInitialize(&immOmHandle, &immOmCallbacks, &immVersion), SA_AIS_OK);
    rc = saImmOmSearchInitialize_2(-1, NULL, SA_IMM_SUBTREE,
        SA_IMM_SEARCH_ONE_ATTR | SA_IMM_SEARCH_GET_NO_ATTR, NULL, NULL, &searchHandle);
    test_validate(rc, SA_AIS_ERR_BAD_HANDLE);
    safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);
}

void saImmOmSearchInitialize_2_03(void)
{
    SaImmSearchHandleT searchHandle;

    safassert(saImmOmInitialize(&immOmHandle, &immOmCallbacks, &immVersion), SA_AIS_OK);
    rc = saImmOmSearchInitialize_2(immOmHandle, NULL, -1,
        SA_IMM_SEARCH_ONE_ATTR | SA_IMM_SEARCH_GET_NO_ATTR, NULL, NULL, &searchHandle);
    test_validate(rc, SA_AIS_ERR_INVALID_PARAM);
    safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);
}

void saImmOmSearchInitialize_2_04(void)
{
    SaImmSearchHandleT searchHandle = -1;

    safassert(saImmOmInitialize(&immOmHandle, &immOmCallbacks, &immVersion), SA_AIS_OK);
    rc = saImmOmSearchInitialize_2(immOmHandle, NULL, SA_IMM_ONE,
        SA_IMM_SEARCH_ONE_ATTR | SA_IMM_SEARCH_GET_NO_ATTR, NULL, NULL, &searchHandle);
    test_validate(rc, SA_AIS_ERR_INVALID_PARAM);
    safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);
}

void saImmOmSearchInitialize_2_05(void)
{
    SaImmSearchHandleT searchHandle;

    safassert(saImmOmInitialize(&immOmHandle, &immOmCallbacks, &immVersion), SA_AIS_OK);
    rc = saImmOmSearchInitialize_2(immOmHandle, NULL, SA_IMM_ONE,
        SA_IMM_SEARCH_ONE_ATTR | SA_IMM_SEARCH_GET_NO_ATTR | SA_IMM_SEARCH_GET_SOME_ATTR,
        NULL, NULL, &searchHandle);
    test_validate(rc, SA_AIS_ERR_INVALID_PARAM);
    safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);
}

void saImmOmSearchInitialize_2_06(void)
{
    SaImmSearchHandleT searchHandle=0LL;

    safassert(saImmOmInitialize(&immOmHandle, &immOmCallbacks, &immVersion), SA_AIS_OK);
    rc = saImmOmSearchInitialize_2(immOmHandle, NULL, SA_IMM_ONE,
        SA_IMM_SEARCH_ONE_ATTR | SA_IMM_SEARCH_GET_NO_ATTR,
        NULL, NULL, &searchHandle);
    test_validate(rc, SA_AIS_ERR_INVALID_PARAM);
    safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);
}

void saImmOmSearchInitialize_2_07(void)
{
    SaImmSearchHandleT searchHandle=0LL;

    safassert(saImmOmInitialize(&immOmHandle, &immOmCallbacks, &immVersion), SA_AIS_OK);
    SaImmSearchParametersT_2 searchParam;
    searchParam.searchOneAttr.attrName = SA_IMM_ATTR_CLASS_NAME;
    searchParam.searchOneAttr.attrValue = NULL;

    SaImmAttrNameT attributeNames[2];
    attributeNames[0] = "safVersion";
    attributeNames[1] = NULL;

    rc = saImmOmSearchInitialize_2(immOmHandle, NULL, SA_IMM_SUBTREE,
        SA_IMM_SEARCH_ONE_ATTR | SA_IMM_SEARCH_GET_SOME_ATTR,
        &searchParam, attributeNames, &searchHandle);
    test_validate(rc, SA_AIS_OK);
    safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);
}

extern void saImmOmSearchNext_2_01(void);
extern void saImmOmSearchNext_2_02(void);
extern void saImmOmSearchFinalize_01(void);
extern void saImmOmSearchFinalize_02(void);

__attribute__ ((constructor)) static void saImmOmInitialize_constructor(void)
{
    test_suite_add(3, "Object Search");
    test_case_add(3, saImmOmSearchInitialize_2_01, "saImmOmSearchInitialize_2 - SA_AIS_OK");
    test_case_add(3, saImmOmSearchInitialize_2_02, "saImmOmSearchInitialize_2 - SA_AIS_ERR_BAD_HANDLE");
    test_case_add(3, saImmOmSearchInitialize_2_03, "saImmOmSearchInitialize_2 - SA_AIS_ERR_INVALID_PARAM, invalid scope (-1)");
    test_case_add(3, saImmOmSearchInitialize_2_04, "saImmOmSearchInitialize_2 - SA_AIS_ERR_INVALID_PARAM, invalid scope (SA_IMM_ONE)");
    test_case_add(3, saImmOmSearchInitialize_2_05, "saImmOmSearchInitialize_2 - SA_AIS_ERR_INVALID_PARAM, invalid searchOptions");
    test_case_add(3, saImmOmSearchInitialize_2_06,
        "saImmOmSearchInitialize_2 - SA_AIS_ERR_INVALID_PARAM, searchHandle set although SA_IMM_SEARCH_GET_SOME_ATTR is not set");
    test_case_add(3, saImmOmSearchInitialize_2_07,
        "saImmOmSearchInitialize_2 - SA_AIS_OK, Match on existence of attribute SA_IMM_ATTR_CLASS_NAME See: #1895");

    test_case_add(3, saImmOmSearchNext_2_01, "saImmOmSearchNext_2 - SA_AIS_OK/SA_AIS_ERR_NOT_EXIST (tree walk)");
    test_case_add(3, saImmOmSearchNext_2_02, "saImmOmSearchNext_2 - SA_AIS_ERR_BAD_HANDLE");

    test_case_add(3, saImmOmSearchFinalize_01, "saImmOmSearchFinalize - SA_AIS_OK");
    test_case_add(3, saImmOmSearchFinalize_02, "saImmOmSearchFinalize - SA_AIS_ERR_BAD_HANDLE");
}

