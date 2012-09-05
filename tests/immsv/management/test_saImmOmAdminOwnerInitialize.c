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

void saImmOmAdminOwnerInitialize_01(void)
{
    const SaImmAdminOwnerNameT adminOwnerName = (SaImmAdminOwnerNameT) __FUNCTION__;
    SaImmAdminOwnerHandleT adminOwnerHandle;

    safassert(saImmOmInitialize(&immOmHandle, &immOmCallbacks, &immVersion), SA_AIS_OK);
    rc = saImmOmAdminOwnerInitialize(immOmHandle, adminOwnerName, SA_TRUE, &adminOwnerHandle);
    test_validate(rc, SA_AIS_OK);
    safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);
}

void saImmOmAdminOwnerInitialize_02(void)
{
    const SaImmAdminOwnerNameT adminOwnerName = (SaImmAdminOwnerNameT) __FUNCTION__;
    SaImmAdminOwnerHandleT adminOwnerHandle;

    safassert(saImmOmInitialize(&immOmHandle, &immOmCallbacks, &immVersion), SA_AIS_OK);
    rc = saImmOmAdminOwnerInitialize(-1, adminOwnerName, SA_TRUE, &adminOwnerHandle);
    test_validate(rc, SA_AIS_ERR_BAD_HANDLE);
    safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);
}

void saImmOmAdminOwnerInitialize_03(void)
{
    const SaImmAdminOwnerNameT adminOwnerName = (SaImmAdminOwnerNameT) __FUNCTION__;
    SaImmAdminOwnerHandleT adminOwnerHandle;

    safassert(saImmOmInitialize(&immOmHandle, &immOmCallbacks, &immVersion), SA_AIS_OK);
    /* invalid releaseOwnershipOnFinalize */
    rc = saImmOmAdminOwnerInitialize(immOmHandle, adminOwnerName, -1, &adminOwnerHandle);
    test_validate(rc, SA_AIS_ERR_INVALID_PARAM);
    safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);
}

void saImmOmAdminOwnerInitialize_04(void)
{
    SaVersionT immOlderVersion = {'A', 0x02, 0x1};
    SaNameT aName = {sizeof("testRdn=0"), "testRdn=0"};
    const SaNameT *aNames[] = {&aName, NULL};
    const SaImmAdminOwnerNameT adminOwnerName = (SaImmAdminOwnerNameT) __FUNCTION__;
    SaImmAdminOwnerHandleT adminOwnerHandle;
    SaAisErrorT retVal = SA_AIS_OK;
    static SaUint64T value = 0xbad;
    SaImmAdminOperationParamsT_2 param = {
        "TEST",
        SA_IMM_ATTR_SAUINT64T,
        &value
    };
    const SaImmAdminOperationParamsT_2 *params[] = {&param, NULL};

    safassert(saImmOmInitialize(&immOmHandle, &immOmCallbacks, &immOlderVersion), SA_AIS_OK);
    safassert(saImmOmAdminOwnerInitialize(immOmHandle, adminOwnerName, SA_TRUE, &adminOwnerHandle), SA_AIS_OK);

    /* Ignoring return code*/ saImmOmAdminOwnerSet(adminOwnerHandle, aNames, SA_IMM_ONE); 

    rc = saImmOmAdminOperationInvoke_o2(adminOwnerHandle, &aName, 0LL, 1, params, &retVal, 0LL, NULL);

    test_validate(rc, SA_AIS_ERR_VERSION);
    safassert(saImmOmAdminOwnerFinalize(adminOwnerHandle), SA_AIS_OK);
    safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);
}

void saImmOmAdminOwnerInitialize_05(void)
{
    const SaImmAdminOwnerNameT adminOwnerName = (SaImmAdminOwnerNameT) "\001\002\003";
    SaImmAdminOwnerHandleT adminOwnerHandle;

    safassert(saImmOmInitialize(&immOmHandle, &immOmCallbacks, &immVersion), SA_AIS_OK);
    rc = saImmOmAdminOwnerInitialize(immOmHandle, adminOwnerName, SA_TRUE, &adminOwnerHandle);
    test_validate(rc, SA_AIS_ERR_INVALID_PARAM);
    safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);
}


extern void saImmOmAdminOwnerSet_01(void);
extern void saImmOmAdminOwnerSet_02(void);
extern void saImmOmAdminOwnerSet_03(void);
extern void saImmOmAdminOwnerSet_04(void);
extern void saImmOmAdminOwnerSet_05(void);
extern void saImmOmAdminOwnerSet_06(void);
extern void saImmOmAdminOwnerRelease_01(void);
extern void saImmOmAdminOwnerRelease_02(void);
extern void saImmOmAdminOwnerRelease_03(void);
extern void saImmOmAdminOwnerRelease_04(void);
extern void saImmOmAdminOwnerRelease_05(void);
extern void saImmOmAdminOwnerFinalize_01(void);
extern void saImmOmAdminOwnerFinalize_02(void);
extern void saImmOmAdminOwnerFinalize_03(void);
extern void saImmOmAdminOwnerClear_01(void);
extern void saImmOmAdminOwnerClear_02(void);
extern void saImmOmAdminOwnerClear_03(void);
extern void saImmOmAdminOwnerClear_04(void);

__attribute__ ((constructor)) static void saImmOmAdminOwnerInitialize_constructor(void)
{
    test_suite_add(5, "Object Administration Ownership");
    test_case_add(5, saImmOmAdminOwnerInitialize_01, "saImmOmAdminOwnerInitialize - SA_AIS_OK");
    test_case_add(5, saImmOmAdminOwnerInitialize_02, "saImmOmAdminOwnerInitialize - SA_AIS_ERR_BAD_HANDLE");
    test_case_add(5, saImmOmAdminOwnerInitialize_03,
        "saImmOmAdminOwnerInitialize - SA_AIS_ERR_INVALID_PARAM, invalid releaseOwnershipOnFinalize");

    test_case_add(5, saImmOmAdminOwnerSet_01, "saImmOmAdminOwnerSet - SA_AIS_OK");
    test_case_add(5, saImmOmAdminOwnerSet_02, "saImmOmAdminOwnerSet - SA_AIS_ERR_BAD_HANDLE");
    test_case_add(5, saImmOmAdminOwnerSet_03, "saImmOmAdminOwnerSet - SA_AIS_ERR_INVALID_PARAM, Invalid scope");
    test_case_add(5, saImmOmAdminOwnerSet_04, "saImmOmAdminOwnerSet - SA_AIS_ERR_INVALID_PARAM, no object names");
    test_case_add(5, saImmOmAdminOwnerSet_05, "saImmOmAdminOwnerSet - SA_AIS_ERR_NOT_EXIST, no existing object");
    test_case_add(5, saImmOmAdminOwnerSet_06, "saImmOmAdminOwnerSet - SA_AIS_ERR_EXIST, different admin owner already exist");
    test_case_add(5, saImmOmAdminOwnerRelease_01, "saImmOmAdminOwnerRelease - SA_AIS_OK");
    test_case_add(5, saImmOmAdminOwnerRelease_02, "saImmOmAdminOwnerRelease - SA_AIS_ERR_BAD_HANDLE");
    test_case_add(5, saImmOmAdminOwnerRelease_04, "saImmOmAdminOwnerRelease - SA_AIS_ERR_INVALID_PARAM, Invalid scope");
    test_case_add(5, saImmOmAdminOwnerRelease_05, "saImmOmAdminOwnerRelease - SA_AIS_ERR_NOT_EXIST, no existing object");
    test_case_add(5, saImmOmAdminOwnerFinalize_01, "saImmOmAdminOwnerFinalize - SA_AIS_OK");
    test_case_add(5, saImmOmAdminOwnerFinalize_02, "saImmOmAdminOwnerFinalize - SA_AIS_ERR_BAD_HANDLE");
    test_case_add(5, saImmOmAdminOwnerFinalize_03, "ABT: saImmOmAdminOwnerFinalize - SA_AIS_OK");

    test_case_add(5, saImmOmAdminOwnerClear_01, "saImmOmAdminOwnerClear - SA_AIS_OK");
    test_case_add(5, saImmOmAdminOwnerClear_02, "saImmOmAdminOwnerClear - SA_AIS_ERR_BAD_HANDLE");
    test_case_add(5, saImmOmAdminOwnerClear_03, "saImmOmAdminOwnerClear - SA_AIS_ERR_INVALID_PARAM");
    test_case_add(5, saImmOmAdminOwnerClear_04, "saImmOmAdminOwnerClear - SA_AIS_ERR_NOT_EXIST");

    test_case_add(5, saImmOmAdminOwnerInitialize_04,
        "saImmOmAdminOperationInvoke_o2 - SA_AIS_ERR_VERSION, invalid invocation of A.2.11 API using A.2.1 version");
    test_case_add(5, saImmOmAdminOwnerInitialize_05, "saImmOmAdminOwnerInitialize - SA_AIS_ERR_INVALID_PARAM, empty admin-owner-name");
}

