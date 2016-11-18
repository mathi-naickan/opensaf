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

SaClmClusterNotificationBufferT notificationBuffer_1;
SaClmClusterNotificationBufferT_4 notificationBuffer_4;
SaUint8T trackFlags;
SaClmNodeIdT nodeId;
SaInvocationT invocation;

static void clmTrackCallback44(const SaClmClusterNotificationBufferT_4 *notificationBuffer,
			SaUint32T numberOfMembers,
			SaInvocationT invocation,
			const SaNameT *rootCauseEntity,
			const SaNtfCorrelationIdsT *correlationIds,
			SaClmChangeStepT step,
			SaTimeT timeSupervision,
			SaAisErrorT error) 
{
printf("\n");
printf("Inside TrackCallback4\n");
printf("invocation : %llu\n",invocation);
printf("rc = %d\n", error);
printf("\n");
}

static void clmTrackCallback11(const SaClmClusterNotificationBufferT *notificationBuffer,
			SaUint32T numberOfMembers,
			SaAisErrorT error)
{
printf("\n");
printf("\n Inside TrackCallback");
printf("\n No of items = %d", notificationBuffer->numberOfItems);
printf("\n rc = %d", error);
printf("\n");
}

static void nodeGetCallBack11(SaInvocationT invocation,
                             const SaClmClusterNodeT *clusterNode,
                             SaAisErrorT error) {
printf("\n inside of node get callback1");
}

static void nodeGetCallBack44(SaInvocationT invocation,
                             const SaClmClusterNodeT_4 *clusterNode,
                             SaAisErrorT error) {

printf("\n inside of node get callback4");
}

SaClmCallbacksT_4 clmCallback44 = {nodeGetCallBack44, clmTrackCallback44};
SaClmCallbacksT clmCallback11 = {nodeGetCallBack11, clmTrackCallback11};


void saClmClusterTrackStop_01(void)
{
        trackFlags = SA_TRACK_CURRENT;
        nodeId = 131343;
        invocation = 600;
        safassert(saClmInitialize(&clmHandle, &clmCallback11, &clmVersion_1), SA_AIS_OK);
        rc = saClmClusterTrack(clmHandle, trackFlags, NULL);
        rc = saClmClusterTrack(clmHandle, trackFlags, NULL);
	rc = saClmClusterNodeGetAsync(clmHandle, invocation, nodeId);
        rc = saClmClusterTrack(clmHandle, trackFlags, NULL);
	rc = saClmClusterNodeGetAsync(clmHandle, invocation, nodeId);
	rc = saClmClusterNodeGetAsync(clmHandle, invocation, nodeId);
        rc = saClmClusterTrack(clmHandle, trackFlags, NULL);
        safassert(saClmClusterTrackStop(clmHandle), SA_AIS_ERR_NOT_EXIST);
        safassert(saClmDispatch(clmHandle,SA_DISPATCH_ALL), SA_AIS_OK);
        rc = saClmClusterTrack(clmHandle, trackFlags, NULL);
        safassert(saClmClusterTrackStop(clmHandle), SA_AIS_ERR_NOT_EXIST);
        safassert(saClmFinalize(clmHandle), SA_AIS_OK);
        test_validate(rc, SA_AIS_OK);
}

void saClmClusterTrackStop_02(void)
{
	rc = saClmClusterTrackStop(clmHandle);
	test_validate(rc, SA_AIS_ERR_BAD_HANDLE);
}

__attribute__ ((constructor)) static void saClmClusterTrackStop_constructor(void)
{
        test_suite_add(9, "Test case for saClmClusterTrackStop");
	test_case_add(9, saClmClusterTrackStop_01, "saClmClusterTrackStop with valid arguments, SA_AIS_OK");
	test_case_add(9, saClmClusterTrackStop_02, "saClmClusterTrackStop with invalid handle, SA_AIS_ERR_BAD_HANDLE");
	
}
