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

static void nodeGetCallBack1(SaInvocationT invocation,
                             const SaClmClusterNodeT *clusterNode,
                             SaAisErrorT error) {
        printf("Inside nodeGetCallBack1");
        printf("error= %d",error);

}

SaClmCallbacksT clmCallbacks = {nodeGetCallBack1, NULL};
SaClmNodeIdT nodeId;
SaInvocationT invocation;

void saClmDispatch_01(void)
{
	struct pollfd fds[1];
        int ret;
        nodeId = 131343;/*node does not exist*/
        invocation = 600;
        SaAisErrorT rc;

        safassert(saClmInitialize(&clmHandle, &clmCallbacks, &clmVersion_1), SA_AIS_OK);
        safassert(saClmSelectionObjectGet(clmHandle, &selectionObject),SA_AIS_OK);
        safassert(saClmClusterNodeGetAsync(clmHandle, invocation, nodeId),SA_AIS_OK);

        fds[0].fd = (int) selectionObject;
        fds[0].events = POLLIN;
        ret = poll(fds, 1, 1000);
        assert(ret == 1);

        rc = saClmDispatch(clmHandle,SA_DISPATCH_ALL);
        safassert(saClmFinalize(clmHandle), SA_AIS_OK);
        test_validate(rc, SA_AIS_OK);
}

void saClmDispatch_02(void)
{
    rc = saClmDispatch(0, SA_DISPATCH_ALL);
    test_validate(rc, SA_AIS_ERR_BAD_HANDLE);
}

void saClmDispatch_03(void)
{
    safassert(saClmInitialize(&clmHandle, &clmCallbacks, &clmVersion_1), SA_AIS_OK);
    rc = saClmDispatch(clmHandle, 0);
    safassert(saClmFinalize(clmHandle), SA_AIS_OK);
    test_validate(rc, SA_AIS_ERR_INVALID_PARAM);
}

void saClmDispatch_04(void)
{
        struct pollfd fds[1];
        int ret;
        nodeId = 131343;/*node does not exist*/
        invocation = 600;
        SaAisErrorT rc;

        safassert(saClmInitialize(&clmHandle, &clmCallbacks, &clmVersion_1), SA_AIS_OK);
        safassert(saClmSelectionObjectGet(clmHandle, &selectionObject),SA_AIS_OK);
        safassert(saClmClusterNodeGetAsync(clmHandle, invocation, nodeId),SA_AIS_OK);

        fds[0].fd = (int) selectionObject;
        fds[0].events = POLLIN;
        ret = poll(fds, 1, 1000);
        assert(ret == 1);

        rc = saClmDispatch(clmHandle,SA_DISPATCH_ONE);
        safassert(saClmFinalize(clmHandle), SA_AIS_OK);
        test_validate(rc, SA_AIS_OK);
}



__attribute__ ((constructor)) static void saClmDispatch_constructor(void)
{
    test_suite_add(3, "Life Cykel API 4");
    test_case_add(3, saClmDispatch_01, "saClmDispatch - SA_AIS_OK SA_DISPATCH_ALL");
    test_case_add(3, saClmDispatch_02, "saClmDispatch - invalid handle SA_AIS_ERR_BAD_HANDLE");
    test_case_add(3, saClmDispatch_03, "saClmDispatch - zero flag SA_AIS_ERR_INVALID_PARAM");
    test_case_add(3, saClmDispatch_04, "saClmDispatch - SA_AIS_OK SA_DISPATCH_ONE");
}

