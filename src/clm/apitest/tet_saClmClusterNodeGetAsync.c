/*      -*- OpenSAF  -*-
 *
 * (C) Copyright 13134308 The OpenSAF Foundation
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

SaClmNodeIdT nodeId;
SaInvocationT invocation;

static void nodeGetCallBack1(SaInvocationT invocation, 
			     const SaClmClusterNodeT *clusterNode, 
			     SaAisErrorT error) {
	printf("Inside nodeGetCallBack1");
	printf("error= %d",error);

}

static void nodeGetCallBack4(SaInvocationT invocation, 
                             const SaClmClusterNodeT_4 *clusterNode,   
                             SaAisErrorT error) {

	printf("Inside nodeGetCallBack4");
	printf("error= %d",error);

}
SaClmCallbacksT_4 clmCallbacks4 = {nodeGetCallBack4, NULL};
SaClmCallbacksT clmCallbacks1 = {nodeGetCallBack1, NULL};

void saClmClusterNodeGetAsync_01(void)
{	
	/*struct pollfd fds[1];*/
        /*int ret;*/
        nodeId = 131343;
        invocation = 600;
        SaAisErrorT rc;

        safassert(saClmInitialize(&clmHandle, &clmCallbacks1, &clmVersion_1), SA_AIS_OK);
        safassert(saClmSelectionObjectGet(clmHandle, &selectionObject),SA_AIS_OK);
        rc = saClmClusterNodeGetAsync(clmHandle, invocation, nodeId);
/*
        fds[0].fd = (int) selectionObject;
        fds[0].events = POLLIN;
        ret = poll(fds, 1, 1000);
        assert(ret == 1);
*/
        safassert(saClmDispatch(clmHandle,SA_DISPATCH_ALL), SA_AIS_OK);
        safassert(saClmFinalize(clmHandle), SA_AIS_OK);
        test_validate(rc, SA_AIS_OK);
}

void saClmClusterNodeGetAsync_02(void)
{
	 struct pollfd fds[1];
        int ret;
        nodeId = 131343;
        invocation = 700;
        SaAisErrorT rc;

        safassert(saClmInitialize_4(&clmHandle, &clmCallbacks4, &clmVersion_4), SA_AIS_OK);
        safassert(saClmSelectionObjectGet(clmHandle, &selectionObject),SA_AIS_OK);
        rc = saClmClusterNodeGetAsync(clmHandle, invocation, nodeId);

        fds[0].fd = (int) selectionObject;
        fds[0].events = POLLIN;
        ret = poll(fds, 1, 1000);
        assert(ret == 1);

        safassert(saClmDispatch(clmHandle,SA_DISPATCH_ALL), SA_AIS_OK);
        safassert(saClmFinalize(clmHandle), SA_AIS_OK);
        test_validate(rc, SA_AIS_OK);
}

void saClmClusterNodeGetAsync_03(void)
{
        nodeId = 131343;
	invocation = 300;
        safassert(saClmInitialize(&clmHandle, &clmCallbacks1, &clmVersion_1), SA_AIS_OK);
        rc = saClmClusterNodeGetAsync(0, invocation, nodeId);
        safassert(saClmFinalize(clmHandle), SA_AIS_OK);
        test_validate(rc, SA_AIS_ERR_BAD_HANDLE);

        safassert(saClmInitialize(&clmHandle, &clmCallbacks1, &clmVersion_1), SA_AIS_OK);
        rc = saClmClusterNodeGetAsync(-1, invocation, nodeId);
        safassert(saClmFinalize(clmHandle), SA_AIS_OK);
        test_validate(rc, SA_AIS_ERR_BAD_HANDLE);

        safassert(saClmInitialize_4(&clmHandle, &clmCallbacks4, &clmVersion_4), SA_AIS_OK);
        rc = saClmClusterNodeGetAsync(0, invocation, nodeId);
        safassert(saClmFinalize(clmHandle), SA_AIS_OK);
        test_validate(rc, SA_AIS_ERR_BAD_HANDLE);

        safassert(saClmInitialize_4(&clmHandle, &clmCallbacks4, &clmVersion_4), SA_AIS_OK);
        rc = saClmClusterNodeGetAsync(-1, invocation, nodeId);
        safassert(saClmFinalize(clmHandle), SA_AIS_OK);
        test_validate(rc, SA_AIS_ERR_BAD_HANDLE);
}

void saClmClusterNodeGetAsync_04(void)
{
        nodeId = 131343;
	invocation = 400;
        safassert(saClmInitialize(&clmHandle, &clmCallbacks1, &clmVersion_1), SA_AIS_OK);
        rc = saClmClusterNodeGetAsync(clmHandle, 0, nodeId);
        safassert(saClmFinalize(clmHandle), SA_AIS_OK);
        test_validate(rc, SA_AIS_ERR_INVALID_PARAM);
}

void saClmClusterNodeGetAsync_05(void)
{
        nodeId = 131343;
	invocation = 500;
        safassert(saClmInitialize(&clmHandle, &clmCallbacks1, &clmVersion_1), SA_AIS_OK);
        rc = saClmClusterNodeGetAsync(clmHandle, invocation, 0);
        safassert(saClmFinalize(clmHandle), SA_AIS_OK);
        test_validate(rc, SA_AIS_ERR_INVALID_PARAM);
}
void saClmClusterNodeGetAsync_06(void)
{
        struct pollfd fds[1];
        int ret;
        nodeId = 255;/*node does not exist*/
	invocation = 600;
	SaAisErrorT rc;

        safassert(saClmInitialize_4(&clmHandle, &clmCallbacks4, &clmVersion_4), SA_AIS_OK);
        safassert(saClmSelectionObjectGet(clmHandle, &selectionObject),SA_AIS_OK);
        rc = saClmClusterNodeGetAsync(clmHandle, invocation, nodeId);

        fds[0].fd = (int) selectionObject;
        fds[0].events = POLLIN;
        ret = poll(fds, 1, 1000);
        assert(ret == 1);       

        safassert(saClmDispatch(clmHandle,SA_DISPATCH_ALL), SA_AIS_OK);
        safassert(saClmFinalize(clmHandle), SA_AIS_OK);
        test_validate(rc, SA_AIS_OK);
}

void saClmClusterNodeGetAsync_07(void)
{
	struct pollfd fds[1];
        int ret;

        nodeId = 131855;/*node is non member*/
	invocation = 700;
        safassert(saClmInitialize(&clmHandle, &clmCallbacks1, &clmVersion_1), SA_AIS_OK);
	safassert(saClmSelectionObjectGet(clmHandle, &selectionObject),SA_AIS_OK);	
        rc = saClmClusterNodeGetAsync(clmHandle, invocation, nodeId);
	
	fds[0].fd = (int) selectionObject;
        fds[0].events = POLLIN;
        ret = poll(fds, 1, 1000);
        assert(ret == 1);

        safassert(saClmFinalize(clmHandle), SA_AIS_OK);
        test_validate(rc, SA_AIS_OK);
}

void saClmClusterNodeGetAsync_08(void)
{
	struct pollfd fds[1];
	SaAisErrorT rc;
        int ret;
	invocation = 900;
        safassert(saClmInitialize(&clmHandle, &clmCallbacks1, &clmVersion_1), SA_AIS_OK);
	safassert(saClmSelectionObjectGet(clmHandle, &selectionObject),SA_AIS_OK);
        rc = saClmClusterNodeGetAsync(clmHandle, invocation, SA_CLM_LOCAL_NODE_ID);

	fds[0].fd = (int) selectionObject;
        fds[0].events = POLLIN;
        ret = poll(fds, 1, 1000);
        assert(ret == 1);
	
	safassert(saClmDispatch(clmHandle,SA_DISPATCH_ALL), SA_AIS_OK);	
        safassert(saClmFinalize(clmHandle), SA_AIS_OK);
        test_validate(rc, SA_AIS_OK);
}

void saClmClusterNodeGetAsync_09(void)
{
        /*struct pollfd fds[1];*/
        /*int ret;*/
        nodeId = 131343;
        invocation = 600;
        SaAisErrorT rc;

        safassert(saClmInitialize(&clmHandle, NULL, &clmVersion_1), SA_AIS_OK);
        safassert(saClmSelectionObjectGet(clmHandle, &selectionObject),SA_AIS_OK);
        rc = saClmClusterNodeGetAsync(clmHandle, invocation, nodeId);
/*
        fds[0].fd = (int) selectionObject;
        fds[0].events = POLLIN;
        ret = poll(fds, 1, 1000);
        assert(ret == 1);
*/
        safassert(saClmDispatch(clmHandle,SA_DISPATCH_ALL), SA_AIS_OK);
        safassert(saClmFinalize(clmHandle), SA_AIS_OK);
        test_validate(rc, SA_AIS_ERR_INIT);

        safassert(saClmInitialize_4(&clmHandle, NULL, &clmVersion_4), SA_AIS_OK);
        safassert(saClmSelectionObjectGet(clmHandle, &selectionObject),SA_AIS_OK);
        rc = saClmClusterNodeGetAsync(clmHandle, invocation, nodeId);
/*
        fds[0].fd = (int) selectionObject;
        fds[0].events = POLLIN;
        ret = poll(fds, 1, 1000);
        assert(ret == 1);
*/
        safassert(saClmDispatch(clmHandle,SA_DISPATCH_ALL), SA_AIS_OK);
        safassert(saClmFinalize(clmHandle), SA_AIS_OK);
        test_validate(rc, SA_AIS_ERR_INIT);
}


__attribute__ ((constructor)) static void  saClmClusterNodeGetAsyncAsync_constructor(void)
{
        test_suite_add(6, "Test case for saClmClusterNodeGetAsync");
	test_case_add(6, saClmClusterNodeGetAsync_01, "saClmClusterNodeGetAsync with valid arguments, SA_AIS_OK");
	test_case_add(6, saClmClusterNodeGetAsync_02, "saClmClusterNodeGetAsync_4 with valid arguments, SA_AIS_OK");
	test_case_add(6, saClmClusterNodeGetAsync_03, "saClmClusterNodeGetAsync with invlaid handle"); 
	test_case_add(6, saClmClusterNodeGetAsync_04, "saClmClusterNodeGetAsync with null invocation"); 
	test_case_add(6, saClmClusterNodeGetAsync_05, "saClmClusterNodeGetAsync with null nodeId"); 
        test_case_add(6, saClmClusterNodeGetAsync_06, "saClmClusterNodeGetAsync with nodeId which does not exist");
        test_case_add(6, saClmClusterNodeGetAsync_07, "saClmClusterNodeGetAsync with nodeId of non member node");
        test_case_add(6, saClmClusterNodeGetAsync_08, "saClmClusterNodeGetAsync with nodeId as SA_CLM_LOCAL_NODE_ID");
        test_case_add(6, saClmClusterNodeGetAsync_09, "saClmClusterNodeGetAsync with null callback");
}
