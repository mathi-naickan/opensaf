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

SaClmClusterNodeT clusterNode_1;
SaClmClusterNodeT_4 clusterNode_4;
SaClmNodeIdT nodeId;
SaTimeT timeout = 10000000000ll;

void saClmClusterNodeGet_01(void)
{	
	nodeId = 131343;
        safassert(saClmInitialize(&clmHandle, &clmCallbacks_1, &clmVersion_1), SA_AIS_OK);
	rc = saClmClusterNodeGet(clmHandle, nodeId, timeout, &clusterNode_1);
        safassert(saClmFinalize(clmHandle), SA_AIS_OK);
        test_validate(rc, SA_AIS_OK);
}

void saClmClusterNodeGet_02(void)
{
        nodeId = 131343;
        safassert(saClmInitialize_4(&clmHandle, &clmCallbacks_4, &clmVersion_4), SA_AIS_OK);
        rc = saClmClusterNodeGet_4(clmHandle, nodeId, timeout, &clusterNode_4);
        safassert(saClmFinalize(clmHandle), SA_AIS_OK);
        test_validate(rc, SA_AIS_OK);
}

void saClmClusterNodeGet_03(void)
{
        nodeId = 131343;
        safassert(saClmInitialize(&clmHandle, &clmCallbacks_1, &clmVersion_1), SA_AIS_OK);
        rc = saClmClusterNodeGet(0, nodeId, timeout, &clusterNode_1);
        safassert(saClmFinalize(clmHandle), SA_AIS_OK);
        test_validate(rc, SA_AIS_ERR_BAD_HANDLE);

        nodeId = 131343;
        safassert(saClmInitialize(&clmHandle, &clmCallbacks_1, &clmVersion_1), SA_AIS_OK);
        rc = saClmClusterNodeGet(-1, nodeId, timeout, &clusterNode_1);
        safassert(saClmFinalize(clmHandle), SA_AIS_OK);
        test_validate(rc, SA_AIS_ERR_BAD_HANDLE);

        nodeId = 131343;
        safassert(saClmInitialize_4(&clmHandle, &clmCallbacks_4, &clmVersion_4), SA_AIS_OK);
        rc = saClmClusterNodeGet_4(0, nodeId, timeout, &clusterNode_4);
        safassert(saClmFinalize(clmHandle), SA_AIS_OK);
        test_validate(rc, SA_AIS_ERR_BAD_HANDLE);

        nodeId = 131343;
        safassert(saClmInitialize_4(&clmHandle, &clmCallbacks_4, &clmVersion_4), SA_AIS_OK);
        rc = saClmClusterNodeGet_4(-1, nodeId, timeout, &clusterNode_4);
        safassert(saClmFinalize(clmHandle), SA_AIS_OK);
        test_validate(rc, SA_AIS_ERR_BAD_HANDLE);
}

void saClmClusterNodeGet_04(void)
{
        nodeId = 131343;
        safassert(saClmInitialize(&clmHandle, &clmCallbacks_1, &clmVersion_1), SA_AIS_OK);
        rc = saClmClusterNodeGet(clmHandle, nodeId, 0, &clusterNode_1);
        safassert(saClmFinalize(clmHandle), SA_AIS_OK);
        test_validate(rc, SA_AIS_ERR_TIMEOUT);

        nodeId = 131343;
        safassert(saClmInitialize_4(&clmHandle, &clmCallbacks_4, &clmVersion_4), SA_AIS_OK);
        rc = saClmClusterNodeGet_4(clmHandle, nodeId, 0, &clusterNode_4);
        safassert(saClmFinalize(clmHandle), SA_AIS_OK);
        test_validate(rc, SA_AIS_ERR_TIMEOUT);

}

void saClmClusterNodeGet_05(void)
{
        nodeId = 131343;
        safassert(saClmInitialize(&clmHandle, &clmCallbacks_1, &clmVersion_1), SA_AIS_OK);
        rc = saClmClusterNodeGet(clmHandle, nodeId, timeout, NULL);
        safassert(saClmFinalize(clmHandle), SA_AIS_OK);
        test_validate(rc, SA_AIS_ERR_INVALID_PARAM);

        nodeId = 131343;
        safassert(saClmInitialize_4(&clmHandle, &clmCallbacks_4, &clmVersion_4), SA_AIS_OK);
        rc = saClmClusterNodeGet_4(clmHandle, nodeId, timeout, NULL);
        safassert(saClmFinalize(clmHandle), SA_AIS_OK);
        test_validate(rc, SA_AIS_ERR_INVALID_PARAM);
}

void saClmClusterNodeGet_06(void)
{
        nodeId = 255;/*node does not exist*/
        safassert(saClmInitialize(&clmHandle, &clmCallbacks_1, &clmVersion_1), SA_AIS_OK);
        rc = saClmClusterNodeGet(clmHandle, nodeId, timeout, &clusterNode_1);
        safassert(saClmFinalize(clmHandle), SA_AIS_OK);
        test_validate(rc, SA_AIS_ERR_NOT_EXIST);

        nodeId = 255;
        safassert(saClmInitialize_4(&clmHandle, &clmCallbacks_4, &clmVersion_4), SA_AIS_OK);
        rc = saClmClusterNodeGet_4(clmHandle, nodeId, timeout, &clusterNode_4);
        safassert(saClmFinalize(clmHandle), SA_AIS_OK);
        test_validate(rc, SA_AIS_ERR_NOT_EXIST);
}

void saClmClusterNodeGet_07(void)
{
        nodeId = 131855;/*node is non member*/
        safassert(saClmInitialize(&clmHandle, &clmCallbacks_1, &clmVersion_1), SA_AIS_OK);
        rc = saClmClusterNodeGet(clmHandle, nodeId, timeout, &clusterNode_1);
        safassert(saClmFinalize(clmHandle), SA_AIS_OK);
        /*test_validate(rc, SA_AIS_ERR_UNAVAILABLE);*/
        test_validate(rc, SA_AIS_ERR_NOT_EXIST);

        nodeId = 131855;
        safassert(saClmInitialize_4(&clmHandle, &clmCallbacks_4, &clmVersion_4), SA_AIS_OK);
        rc = saClmClusterNodeGet_4(clmHandle, nodeId, timeout, &clusterNode_4);
        safassert(saClmFinalize(clmHandle), SA_AIS_OK);
        /*test_validate(rc, SA_AIS_ERR_UNAVAILABLE);*/
        test_validate(rc, SA_AIS_ERR_NOT_EXIST);
}

void saClmClusterNodeGet_08(void)
{
        nodeId = 5;
        safassert(saClmInitialize(&clmHandle, &clmCallbacks_1, &clmVersion_1), SA_AIS_OK);
        rc = saClmClusterNodeGet_4(clmHandle, nodeId, timeout, &clusterNode_4);
        /*rc = saClmClusterNodeGet(clmHandle, nodeId, timeout, NULL);*/
        safassert(saClmFinalize(clmHandle), SA_AIS_OK);
        test_validate(rc, SA_AIS_ERR_VERSION);

        nodeId = 5;
        safassert(saClmInitialize_4(&clmHandle, &clmCallbacks_4, &clmVersion_4), SA_AIS_OK);
        rc = saClmClusterNodeGet(clmHandle, nodeId, timeout, &clusterNode_1);
        /*rc = saClmClusterNodeGet(clmHandle, nodeId, timeout, NULL);*/
        safassert(saClmFinalize(clmHandle), SA_AIS_OK);
        test_validate(rc, SA_AIS_ERR_VERSION);
}

void saClmClusterNodeGet_09(void)
{
        safassert(saClmInitialize(&clmHandle, &clmCallbacks_1, &clmVersion_1), SA_AIS_OK);
        rc = saClmClusterNodeGet(clmHandle, SA_CLM_LOCAL_NODE_ID, timeout, &clusterNode_1);
        safassert(saClmFinalize(clmHandle), SA_AIS_OK);
        test_validate(rc, SA_AIS_OK);

        safassert(saClmInitialize_4(&clmHandle, &clmCallbacks_4, &clmVersion_4), SA_AIS_OK);
        rc = saClmClusterNodeGet_4(clmHandle, SA_CLM_LOCAL_NODE_ID, timeout, &clusterNode_4);
        safassert(saClmFinalize(clmHandle), SA_AIS_OK);
        test_validate(rc, SA_AIS_OK);
}

__attribute__ ((constructor)) static void  saClmClusterNodeGet_constructor(void)
{
        test_suite_add(5, "Test case for saClmClusterNodeGet");
	test_case_add(5, saClmClusterNodeGet_01, "saClmClusterNodeGet with valid arguments, SA_AIS_OK");
	test_case_add(5, saClmClusterNodeGet_02, "saClmClusterNodeGet_4 with valid arguments, SA_AIS_OK");
	test_case_add(5, saClmClusterNodeGet_03, "saClmClusterNodeGet & saClmClusterNodeGet_4 with Invalid handle");
	test_case_add(5, saClmClusterNodeGet_04, "saClmClusterNodeGet & saClmClusterNodeGet_4 with NULL timeout"); 
	test_case_add(5, saClmClusterNodeGet_05, "saClmClusterNodeGet & saClmClusterNodeGet_4 with NULL ClusterNode");
        test_case_add(5, saClmClusterNodeGet_06, "saClmClusterNodeGet & saClmClusterNodeGet_4 with nodeId which does not exist");
        test_case_add(5, saClmClusterNodeGet_07, "saClmClusterNodeGet & saClmClusterNodeGet_4 with nodeId of non member node");
        test_case_add(5, saClmClusterNodeGet_08, "saClmClusterNodeGet & saClmClusterNodeGet_4 call with cross version");
        test_case_add(5, saClmClusterNodeGet_09, "saClmClusterNodeGet & saClmClusterNodeGet_4 with nodeId as SA_CLM_LOCAL_NODE_ID");
}
