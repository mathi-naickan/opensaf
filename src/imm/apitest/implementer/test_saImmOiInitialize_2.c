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

void saImmOiInitialize_2_01(void)
{
    immOiHandle=4711;
    immVersion = constImmVersion;
    safassert(saImmOiFinalize(immOiHandle), SA_AIS_ERR_BAD_HANDLE);

    if ((rc = saImmOiInitialize_2(&immOiHandle, &immOiCallbacks, &immVersion)) != SA_AIS_OK)
        goto done;

    safassert(saImmOiFinalize(immOiHandle), SA_AIS_OK);
    immVersion = constImmVersion;
    if ((rc = saImmOiInitialize_2(&immOiHandle, NULL, &immVersion)) != SA_AIS_OK)
        goto done;

    safassert(saImmOiFinalize(immOiHandle), SA_AIS_OK);

    immOiHandle=4711;
    safassert(saImmOiFinalize(immOiHandle), SA_AIS_ERR_BAD_HANDLE);

done:
    test_validate(rc, SA_AIS_OK);
}

void saImmOiInitialize_2_02(void)
{
    SaVersionT version1 = {0, 0, 0};
    SaVersionT version2 = {'B', 1, 1};
    SaVersionT version3 = {'A', 3, 0};

    if ((rc = saImmOiInitialize_2(&immOiHandle, &immOiCallbacks, &version1)) != SA_AIS_ERR_VERSION)
        goto done;

    if ((rc = saImmOiInitialize_2(&immOiHandle, &immOiCallbacks, &version2)) != SA_AIS_ERR_VERSION)
        goto done;

    if ((rc = saImmOiInitialize_2(&immOiHandle, &immOiCallbacks, &version3)) != SA_AIS_ERR_VERSION)
        goto done;

done:
    test_validate(rc, SA_AIS_ERR_VERSION);
}

void saImmOiInitialize_2_03(void)
{
    SaVersionT version = {'A', 2, 2};

    test_validate(saImmOiInitialize_2(&immOiHandle, &immOiCallbacks, &version), SA_AIS_OK);
    safassert(saImmOiFinalize(immOiHandle), SA_AIS_OK);
}

extern void saImmOiSelectionObjectGet_01(void);
extern void saImmOiSelectionObjectGet_02(void);
extern void saImmOiDispatch_01(void);
extern void saImmOiDispatch_02(void);
extern void saImmOiDispatch_03(void);
extern void saImmOiDispatch_04(void);
extern void saImmOiFinalize_01(void);
extern void saImmOiFinalize_02(void);

__attribute__ ((constructor)) static void saImmOiInitialize_2_constructor(void)
{
    test_suite_add(1, "Library Life Cycle");
    test_case_add(1, saImmOiInitialize_2_01, "saImmOiInitialize_2 - SA_AIS_OK");
    test_case_add(1, saImmOiInitialize_2_02, "saImmOiInitialize_2 - SA_AIS_ERR_VERSION");
    test_case_add(1, saImmOiInitialize_2_03, "saImmOiInitialize_2 - SA_AIS_OK - minor version set to 2");

    test_case_add(1, saImmOiSelectionObjectGet_01, "saImmOiSelectionObjectGet - SA_AIS_OK");
    test_case_add(1, saImmOiSelectionObjectGet_02, "saImmOiSelectionObjectGet - SA_AIS_ERR_BAD_HANDLE");

    test_case_add(1, saImmOiDispatch_01, "saImmOiDispatch - SA_AIS_OK SA_DISPATCH_ALL");
    test_case_add(1, saImmOiDispatch_02, "saImmOiDispatch - SA_AIS_OK SA_DISPATCH_ONE");
    test_case_add(1, saImmOiDispatch_03, "saImmOiDispatch - SA_AIS_ERR_BAD_HANDLE");
    test_case_add(1, saImmOiDispatch_04, "saImmOiDispatch - SA_AIS_ERR_INVALID_PARAM");

    test_case_add(1, saImmOiFinalize_01, "saImmOiFinalize - SA_AIS_OK");
    test_case_add(1, saImmOiFinalize_02, "saImmOiFinalize - SA_AIS_ERR_BAD_HANDLE");
}

