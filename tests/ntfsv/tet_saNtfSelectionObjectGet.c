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

void saNtfSelectionObjectGet_01(void)
{
    safassert(saNtfInitialize(&ntfHandle, &ntfCallbacks, &ntfVersion) , SA_AIS_OK);
    rc = saNtfSelectionObjectGet(ntfHandle, &selectionObject);
    test_validate(rc, SA_AIS_OK);
    safassert(saNtfFinalize(ntfHandle) , SA_AIS_OK);
}

void saNtfSelectionObjectGet_02(void)
{
    safassert(saNtfInitialize(&ntfHandle, &ntfCallbacks, &ntfVersion) , SA_AIS_OK);
    rc = saNtfSelectionObjectGet(0, &selectionObject);
    safassert(saNtfFinalize(ntfHandle) , SA_AIS_OK);
    test_validate(rc, SA_AIS_ERR_BAD_HANDLE);
}

void saNtfSelectionObjectGet_03(void)
{
    safassert(saNtfInitialize(&ntfHandle, &ntfCallbacks, &ntfVersion) , SA_AIS_OK);
    rc = saNtfSelectionObjectGet(-1, &selectionObject);
    safassert(saNtfFinalize(ntfHandle) , SA_AIS_OK);
    test_validate(rc, SA_AIS_ERR_BAD_HANDLE);
}

void saNtfSelectionObjectGet_04(void)
{
    safassert(saNtfInitialize(&ntfHandle, &ntfCallbacks, &ntfVersion) , SA_AIS_OK);
    rc = saNtfSelectionObjectGet(ntfHandle, NULL);
    safassert(saNtfFinalize(ntfHandle) , SA_AIS_OK);
    test_validate(rc, SA_AIS_ERR_INVALID_PARAM);
}

__attribute__ ((constructor)) static void saNtfSelectionObjectGet_constructor(void)
{
    test_suite_add(3, "Life cycle, selectObject, API 3");
    test_case_add(3, saNtfSelectionObjectGet_01, "saNtfSelectionObjectGet SA_AIS_OK");
    test_case_add(3, saNtfSelectionObjectGet_02, "saNtfSelectionObjectGet NULL handle SA_AIS_ERR_BAD_HANDLE");
    test_case_add(3, saNtfSelectionObjectGet_03, "saNtfSelectionObjectGet invalid handle SA_AIS_ERR_BAD_HANDLE");
    test_case_add(3, saNtfSelectionObjectGet_03, "saNtfSelectionObjectGet NULL selectionObject SA_AIS_ERR_INVALID_PARAM");
}

