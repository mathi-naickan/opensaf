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

void saImmOiImplementerSet_01(void)
{
    SaImmOiImplementerNameT implementerName = (SaImmOiImplementerNameT) __FUNCTION__;

    safassert(saImmOiInitialize_2(&immOiHandle, &immOiCallbacks, &immVersion), SA_AIS_OK);
    test_validate(saImmOiImplementerSet(immOiHandle, implementerName), SA_AIS_OK);
    safassert(saImmOiFinalize(immOiHandle), SA_AIS_OK);
}

void saImmOiImplementerSet_02(void)
{
    SaImmOiImplementerNameT implementerName = (SaImmOiImplementerNameT) __FUNCTION__;

    safassert(saImmOiInitialize_2(&immOiHandle, &immOiCallbacks, &immVersion), SA_AIS_OK);
    test_validate(saImmOiImplementerSet(-1, implementerName), SA_AIS_ERR_BAD_HANDLE);
    safassert(saImmOiFinalize(immOiHandle), SA_AIS_OK);
}

void saImmOiImplementerSet_03(void)
{
    SaImmOiImplementerNameT implementerName = (SaImmOiImplementerNameT) __FUNCTION__;
    SaImmOiHandleT newHandle;

    safassert(saImmOiInitialize_2(&immOiHandle, &immOiCallbacks, &immVersion), SA_AIS_OK);
    safassert(saImmOiImplementerSet(immOiHandle, implementerName), SA_AIS_OK);

    safassert(saImmOiInitialize_2(&newHandle, &immOiCallbacks, &immVersion), SA_AIS_OK);
    test_validate(saImmOiImplementerSet(newHandle, implementerName), SA_AIS_ERR_EXIST);
    safassert(saImmOiFinalize(immOiHandle), SA_AIS_OK);
    safassert(saImmOiFinalize(newHandle), SA_AIS_OK);
}

void saImmOiImplementerSet_04(void)
{
    SaImmOiImplementerNameT implementerName = (SaImmOiImplementerNameT) __FUNCTION__;
    SaImmOiImplementerNameT implementerName2 = (SaImmOiImplementerNameT) "MuddyWaters";

    safassert(saImmOiInitialize_2(&immOiHandle, &immOiCallbacks, &immVersion), SA_AIS_OK);
    safassert(saImmOiImplementerSet(immOiHandle, implementerName), SA_AIS_OK);

    test_validate(saImmOiImplementerSet(immOiHandle, implementerName2), SA_AIS_ERR_INVALID_PARAM);
    safassert(saImmOiFinalize(immOiHandle), SA_AIS_OK);
}

void saImmOiImplementerSet_05(void)
{
    SaImmOiImplementerNameT implementerName = (SaImmOiImplementerNameT) "\001";/* Bad implementer name */

    safassert(saImmOiInitialize_2(&immOiHandle, &immOiCallbacks, &immVersion), SA_AIS_OK);
    test_validate(saImmOiImplementerSet(immOiHandle, implementerName), SA_AIS_ERR_INVALID_PARAM);
    safassert(saImmOiFinalize(immOiHandle), SA_AIS_OK);
}

extern void saImmOiImplementerClear_01(void);
extern void saImmOiImplementerClear_02(void);
extern void saImmOiClassImplementerSet_01(void);
extern void saImmOiClassImplementerSet_02(void);
extern void saImmOiClassImplementerSet_03(void);
extern void saImmOiClassImplementerSet_04(void);
extern void saImmOiClassImplementerSet_05(void);
extern void saImmOiClassImplementerSet_06(void);
extern void saImmOiClassImplementerRelease_01(void);
extern void saImmOiClassImplementerRelease_02(void);
extern void saImmOiClassImplementerRelease_03(void);
extern void saImmOiObjectImplementerSet_01(void);
extern void saImmOiObjectImplementerSet_02(void);
extern void saImmOiObjectImplementerSet_03(void);
extern void saImmOiObjectImplementerSet_04(void);
extern void saImmOiObjectImplementerSet_05(void);
extern void saImmOiObjectImplementerSet_06(void);
extern void saImmOiObjectImplementerSet_07(void);
extern void saImmOiObjectImplementerRelease_01(void);
extern void saImmOiObjectImplementerRelease_02(void);
extern void saImmOiObjectImplementerRelease_03(void);
extern void saImmOiObjectImplementerRelease_04(void);

__attribute__ ((constructor)) static void saImmOiImplementerSet_constructor(void)
{
    test_suite_add(2, "Object Implementer");
    test_case_add(2, saImmOiImplementerSet_01, "saImmOiImplementerSet - SA_AIS_OK");
    test_case_add(2, saImmOiImplementerSet_02, "saImmOiImplementerSet - SA_AIS_ERR_BAD_HANDLE");
    test_case_add(2, saImmOiImplementerSet_03, "saImmOiImplementerSet - SA_AIS_ERR_EXIST - som other handle already uses name");
    test_case_add(2, saImmOiImplementerSet_04, "saImmOiImplementerSet - SA_AIS_ERR_INVALID_PARAM - handle already associated with name");

    test_case_add(2, saImmOiImplementerClear_01, "saImmOiImplementerClear - SA_AIS_OK");
    test_case_add(2, saImmOiImplementerClear_02, "saImmOiImplementerClear - SA_AIS_ERR_BAD_HANDLE");

    test_case_add(2, saImmOiClassImplementerSet_01, "saImmOiClassImplementerSet - SA_AIS_OK");
    test_case_add(2, saImmOiClassImplementerSet_02, "saImmOiClassImplementerSet - SA_AIS_ERR_BAD_HANDLE");
    test_case_add(2, saImmOiClassImplementerSet_03, "saImmOiClassImplementerSet - SA_AIS_ERR_BAD_OPERATION");
    test_case_add(2, saImmOiClassImplementerSet_04, "saImmOiClassImplementerSet - SA_AIS_ERR_NOT_EXIST");
    test_case_add(2, saImmOiClassImplementerSet_05, "saImmOiClassImplementerSet - SA_AIS_ERR_EXIST");
    test_case_add(2, saImmOiClassImplementerSet_06, "saImmOiClassImplementerSet - SA_AIS_ERR_EXIST - object implementer already set");

    test_case_add(2, saImmOiClassImplementerRelease_01, "saImmOiClassImplementerRelease - SA_AIS_OK");
    test_case_add(2, saImmOiClassImplementerRelease_02, "saImmOiClassImplementerRelease - SA_AIS_ERR_BAD_HANDLE");
    test_case_add(2, saImmOiClassImplementerRelease_03, "saImmOiClassImplementerRelease - SA_AIS_ERR_NOT_EXIST");

    test_case_add(2, saImmOiObjectImplementerSet_01, "saImmOiObjectImplementerSet - SA_AIS_OK");
    test_case_add(2, saImmOiObjectImplementerSet_02, "saImmOiObjectImplementerSet - SA_AIS_ERR_BAD_HANDLE");
    test_case_add(2, saImmOiObjectImplementerSet_03, "saImmOiObjectImplementerSet - SA_AIS_ERR_BAD_HANDLE - handle not associated with implementer name");
    test_case_add(2, saImmOiObjectImplementerSet_04, "saImmOiObjectImplementerSet - SA_AIS_ERR_NOT_EXIST - objectName non existing");
    test_case_add(2, saImmOiObjectImplementerSet_05, "saImmOiObjectImplementerSet - SA_AIS_ERR_EXIST - objectName already has implementer");
    test_case_add(2, saImmOiObjectImplementerSet_06, "saImmOiObjectImplementerSet - SA_AIS_ERR_EXIST - class already has implementer");

    test_case_add(2, saImmOiObjectImplementerRelease_01, "saImmOiObjectImplementerRelease - SA_AIS_OK");
    test_case_add(2, saImmOiObjectImplementerRelease_02, "saImmOiObjectImplementerRelease - SA_AIS_ERR_BAD_HANDLE");
    test_case_add(2, saImmOiObjectImplementerRelease_03, "saImmOiObjectImplementerRelease - SA_AIS_ERR_NOT_EXIST - objectName non existing");
    test_case_add(2, saImmOiObjectImplementerRelease_04, "saImmOiObjectImplementerRelease - SA_AIS_ERR_INVALID_PARAM - invalid scope");

    test_case_add(2, saImmOiImplementerSet_05, "saImmOiImplementerSet - SA_AIS_ERR_INVALID_PARAM - Bad implementer name");
    test_case_add(2, saImmOiObjectImplementerSet_07, "saImmOiObjectImplementerSet - SA_AIS_ERR_INVALID_PARAM - empty object name with wrong size");
}


