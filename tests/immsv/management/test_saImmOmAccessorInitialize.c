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

static SaImmAccessorHandleT accessorHandle;

void saImmOmAccessorInitialize_01(void)
{
    safassert(saImmOmInitialize(&immOmHandle, &immOmCallbacks, &immVersion), SA_AIS_OK);
    rc = saImmOmAccessorInitialize(immOmHandle, &accessorHandle);
    test_validate(rc, SA_AIS_OK);
    safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);
}

void saImmOmAccessorInitialize_02(void)
{
    safassert(saImmOmInitialize(&immOmHandle, &immOmCallbacks, &immVersion), SA_AIS_OK);
    rc = saImmOmAccessorInitialize(-1, &accessorHandle);
    test_validate(rc, SA_AIS_ERR_BAD_HANDLE);
    safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);
}

extern void saImmOmAccessorGet_2_01(void);
extern void saImmOmAccessorGet_2_02(void);
extern void saImmOmAccessorGet_2_03(void);
extern void saImmOmAccessorGet_2_04(void);
extern void saImmOmAccessorGet_2_05(void);
extern void saImmOmAccessorGet_2_06(void);
extern void saImmOmAccessorGet_2_07(void);
extern void saImmOmAccessorGet_2_08(void);
extern void saImmOmAccessorGet_2_09(void);
extern void saImmOmAccessorGet_2_10(void);
extern void saImmOmAccessorFinalize_01(void);
extern void saImmOmAccessorFinalize_02(void);
extern void saImmOmAccessorFinalize_03(void);

__attribute__ ((constructor)) static void saImmOmAccessorInitialize_constructor(void)
{
    test_suite_add(4, "Object Access");
    test_case_add(4, saImmOmAccessorInitialize_01, "saImmOmAccessorInitialize - SA_AIS_OK");
    test_case_add(4, saImmOmAccessorInitialize_02, "saImmOmAccessorInitialize - SA_AIS_ERR_BAD_HANDLE - invalid handle");

    test_case_add(4, saImmOmAccessorGet_2_01, "saImmOmAccessorGet_2 - SA_AIS_OK");
    test_case_add(4, saImmOmAccessorGet_2_02, "saImmOmAccessorGet_2 - SA_AIS_ERR_BAD_HANDLE - invalid handle");
    test_case_add(4, saImmOmAccessorGet_2_03, "saImmOmAccessorGet_2 - SA_AIS_ERR_NOT_EXIST - objectName does not exist");
    test_case_add(4, saImmOmAccessorGet_2_04, "saImmOmAccessorGet_2 - SA_AIS_OK - accessor get for config attributes");
    test_case_add(4, saImmOmAccessorGet_2_05, "saImmOmAccessorGet_2 - SA_AIS_OK - accessor get for attributeNames[0] == NULL and attributes == NULL");
    test_case_add(4, saImmOmAccessorGet_2_06, "saImmOmAccessorGet_2 - SA_AIS_ERR_INVALID_PARAM - accessor get for attributeNames == NULL and attributes == NULL");
    test_case_add(4, saImmOmAccessorGet_2_07, "saImmOmAccessorGet_2 - SA_AIS_ERR_INVALID_PARAM - accessor get for attributeNames[0] != NULL and attributes == NULL");
    test_case_add(4, saImmOmAccessorGet_2_08, "saImmOmAccessorGet_2 - SA_AIS_ERR_NOT_EXIST - accessor get for non-existing object, attributeNames[0] == NULL and attributes == NULL");
    test_case_add(4, saImmOmAccessorGet_2_09, "saImmOmAccessorGet_2 - SA_AIS_ERR_INVALID_PARAM - empty dn");

    test_case_add(4, saImmOmAccessorGet_2_10, "saImmOmAccessorGet_2 - SA_AIS_OK - repeated use of accesor get 100 times");

    test_case_add(4, saImmOmAccessorFinalize_01, "saImmOmAccessorFinalize - SA_AIS_OK");
    test_case_add(4, saImmOmAccessorFinalize_02, "saImmOmAccessorFinalize - SA_AIS_ERR_BAD_HANDLE - invalid handle");
    test_case_add(4, saImmOmAccessorFinalize_03, "saImmOmAccessorFinalize - SA_AIS_ERR_BAD_HANDLE - already finalized");
}

