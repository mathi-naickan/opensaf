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
#include <utest.h>
#include <util.h>
#include "tet_ntf.h"

void saNtfFinalize_01(void)
{
    safassert(saNtfInitialize(&ntfHandle, &ntfCallbacks, &ntfVersion), SA_AIS_OK);
    rc = saNtfFinalize(ntfHandle);
    test_validate(rc, SA_AIS_OK);
}

void saNtfFinalize_02(void)
{
    rc = saNtfFinalize(-1);
    test_validate(rc, SA_AIS_ERR_BAD_HANDLE);
}

void saNtfFinalize_03(void)
{
    safassert(saNtfInitialize(&ntfHandle, &ntfCallbacks, &ntfVersion), SA_AIS_OK);
    safassert(saNtfFinalize(ntfHandle), SA_AIS_OK);
    rc = saNtfFinalize(ntfHandle);
    test_validate(rc, SA_AIS_ERR_BAD_HANDLE);
}

__attribute__ ((constructor)) static void saNtfFinalize_constructor(void)
{
    test_suite_add(2, "Life cycle, finalize, API 2");
    test_case_add(2, saNtfFinalize_01, "saNtfFinalize SA_AIS_OK");
    test_case_add(2, saNtfFinalize_02, "saNtfFinalize SA_AIS_ERR_BAD_HANDLE - invalid handle");
    test_case_add(2, saNtfFinalize_03, "saNtfFinalize SA_AIS_ERR_BAD_HANDLE - handle already returned");
}

