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
 * Author(s): Emerson Network Power
 *
 */
#include "clmtest.h"


static void nodeGetCallBack(SaInvocationT invocation,
                             const SaClmClusterNodeT *clusterNode,
                             SaAisErrorT error) 
{
        printf("Inside nodeGetCallBack1");
        printf("error= %d",error);

}

SaClmCallbacksT clmCallback = {nodeGetCallBack, NULL};



void saClmSelectionObjectGet_01(void)
{
    safassert(saClmInitialize(&clmHandle, &clmCallback, &clmVersion_1) , SA_AIS_OK);
    rc = saClmSelectionObjectGet(clmHandle, &selectionObject);
    test_validate(rc, SA_AIS_OK);
    safassert(saClmFinalize(clmHandle) , SA_AIS_OK);
}

void saClmSelectionObjectGet_02(void)
{
    safassert(saClmInitialize(&clmHandle, &clmCallback, &clmVersion_1) , SA_AIS_OK);
    rc = saClmSelectionObjectGet(0, &selectionObject);
    safassert(saClmFinalize(clmHandle) , SA_AIS_OK);
    test_validate(rc, SA_AIS_ERR_INVALID_PARAM);
}

void saClmSelectionObjectGet_03(void)
{
    safassert(saClmInitialize(&clmHandle, &clmCallback, &clmVersion_1) , SA_AIS_OK);
    rc = saClmSelectionObjectGet(-1, &selectionObject);
    safassert(saClmFinalize(clmHandle) , SA_AIS_OK);
    test_validate(rc, SA_AIS_ERR_BAD_HANDLE);
}

void saClmSelectionObjectGet_04(void)
{
    safassert(saClmInitialize(&clmHandle, &clmCallback, &clmVersion_1) , SA_AIS_OK);
    rc = saClmSelectionObjectGet(clmHandle, NULL);
    safassert(saClmFinalize(clmHandle) , SA_AIS_OK);
    test_validate(rc, SA_AIS_ERR_INVALID_PARAM);
}

__attribute__ ((constructor)) static void saClmSelectionObjectGet_constructor(void)
{
    test_suite_add(2, "Life Cykel API 3");
    test_case_add(2, saClmSelectionObjectGet_01, "saClmSelectionObjectGet SA_AIS_OK");
    test_case_add(2, saClmSelectionObjectGet_02, "saClmSelectionObjectGet NULL handle SA_AIS_ERR_BAD_HANDLE");
    test_case_add(2, saClmSelectionObjectGet_03, "saClmSelectionObjectGet invalid handle SA_AIS_ERR_BAD_HANDLE");
    test_case_add(2, saClmSelectionObjectGet_03, "saClmSelectionObjectGet NULL selectionObject SA_AIS_ERR_INVALID_PARAM");
}
