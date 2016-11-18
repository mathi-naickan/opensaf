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
#include <sys/types.h>
#include <sys/wait.h>

SaClmClusterNotificationBufferT notificationBuffer_1;
SaClmClusterNotificationBufferT_4 notificationBuffer_4;
SaUint8T trackFlags;
SaClmNodeIdT nodeId;
SaInvocationT invocation;
SaInvocationT lock_inv;

/*Print the Values in SaClmClusterNotificationBufferT_4*/
static void clmTrackbuf_4(SaClmClusterNotificationBufferT_4 *notificationBuffer)
{
	int i;
	printf("No of items = %d\n", notificationBuffer->numberOfItems);

	for(i=0;i<notificationBuffer->numberOfItems;i++){
		printf("\nValue of i = %d\n",i);
		printf("Cluster Change = %d\n",notificationBuffer->notification[i].clusterChange);
		printf("Node Name length = %d, value = %s\n", notificationBuffer->notification[i].clusterNode.nodeName.length,
							notificationBuffer->notification[i].clusterNode.nodeName.value);

		printf("Node Member = %d\n",notificationBuffer->notification[i].clusterNode.member);

		printf("Node  view number  = %llu\n",notificationBuffer->notification[i].clusterNode.initialViewNumber);

		printf("Node  eename length = %d,value  = %s\n",notificationBuffer->notification[i].clusterNode.executionEnvironment.length,
							notificationBuffer->notification[0].clusterNode.executionEnvironment.value);

		printf("Node  boottimestamp  = %llu\n",notificationBuffer->notification[i].clusterNode.bootTimestamp);

		printf("Node  nodeAddress family  = %d,node address length = %d, node address value = %s\n",
					notificationBuffer->notification[i].clusterNode.nodeAddress.family												,notificationBuffer->notification[i].clusterNode.nodeAddress.length,
							notificationBuffer->notification[i].clusterNode.nodeAddress.value);

		printf("Node  nodeid  = %d\n",notificationBuffer->notification[i].clusterNode.nodeId);
	}
}

static void TrackCallback4(const SaClmClusterNotificationBufferT_4 *notificationBuffer,
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
printf("Step : %d\n",step);
printf("error = %d\n", error);
printf("numberOfMembers = %d\n", numberOfMembers);
clmTrackbuf_4((SaClmClusterNotificationBufferT_4 *) notificationBuffer);
lock_inv= invocation;
printf("\n");
}

static void TrackCallback1(const SaClmClusterNotificationBufferT *notificationBuffer,
			SaUint32T numberOfMembers,
			SaAisErrorT error)
{
printf("\n");
printf("\n Inside TrackCallback");
printf("\n No of items = %d", notificationBuffer->numberOfItems);
printf("\n rc = %d", error);
printf("\n");
}

SaClmCallbacksT_4 Callback4 = {NULL, TrackCallback4};
SaClmCallbacksT Callback1 = {NULL, TrackCallback1};

void saClmResponse_01(void)
{
	struct pollfd fds[1];

        trackFlags = SA_TRACK_CHANGES|SA_TRACK_START_STEP;
	printf(" This test case needs (manual) admin op simulation\n");
	return; /* uncomment this and run*/
        safassert(saClmInitialize_4(&clmHandle, &Callback4, &clmVersion_4), SA_AIS_OK);
        safassert(saClmSelectionObjectGet(clmHandle, &selectionObject),SA_AIS_OK);
        rc = saClmClusterTrack_4(clmHandle, trackFlags, NULL);

        fds[0].fd = (int) selectionObject;
        fds[0].events = POLLIN;
        while (1){
                (void)poll(fds, 1, 1000);
                if (fds[0].revents & POLLIN){
                        break;
                }
        }
        safassert(saClmDispatch(clmHandle,SA_DISPATCH_ALL), SA_AIS_OK);
        safassert(saClmResponse_4(clmHandle,lock_inv,SA_CLM_CALLBACK_RESPONSE_OK),SA_AIS_OK);

        fds[0].fd = (int) selectionObject;
        fds[0].events = POLLIN;
        while (1){
                (void)poll(fds, 1, 1000);
                if (fds[0].revents & POLLIN){
                        break;
                }
        }

        safassert(saClmDispatch(clmHandle,SA_DISPATCH_ALL), SA_AIS_OK);
        
        safassert(saClmClusterTrackStop(clmHandle), SA_AIS_OK);
        safassert(saClmFinalize(clmHandle), SA_AIS_OK);
        test_validate(rc, SA_AIS_OK);

}

void saClmResponse_02(void)
{
	struct pollfd fds[1];

        trackFlags = SA_TRACK_CHANGES|SA_TRACK_START_STEP;
	printf("This test case needs (manual) admin op simulation\n");
	return; /* uncomment this and run*/

        safassert(saClmInitialize_4(&clmHandle, &Callback4, &clmVersion_4), SA_AIS_OK);
        safassert(saClmSelectionObjectGet(clmHandle, &selectionObject),SA_AIS_OK);
        rc = saClmClusterTrack_4(clmHandle, trackFlags, NULL);

        fds[0].fd = (int) selectionObject;
        fds[0].events = POLLIN;
        while (1){
                (void)poll(fds, 1, 1000);
                if (fds[0].revents & POLLIN){
                        break;
                }
        }
        safassert(saClmDispatch(clmHandle,SA_DISPATCH_ALL), SA_AIS_OK);
        safassert(saClmResponse_4(clmHandle,lock_inv,SA_CLM_CALLBACK_RESPONSE_ERROR),SA_AIS_OK);

        fds[0].fd = (int) selectionObject;
        fds[0].events = POLLIN;
        while (1){
                (void)poll(fds, 1, 1000);
                if (fds[0].revents & POLLIN){
                        break;
                }
        }

        safassert(saClmDispatch(clmHandle,SA_DISPATCH_ALL), SA_AIS_OK);

        safassert(saClmClusterTrackStop(clmHandle), SA_AIS_OK);
        safassert(saClmFinalize(clmHandle), SA_AIS_OK);
        test_validate(rc, SA_AIS_OK);

}

void saClmResponse_03(void)
{
	struct pollfd fds[1];

        trackFlags = SA_TRACK_CHANGES|SA_TRACK_START_STEP;

	printf("This test case needs (manual) admin op simulation\n");
	return; /* uncomment this and run*/

        safassert(saClmInitialize_4(&clmHandle, &Callback4, &clmVersion_4), SA_AIS_OK);
        safassert(saClmSelectionObjectGet(clmHandle, &selectionObject),SA_AIS_OK);
        rc = saClmClusterTrack_4(clmHandle, trackFlags, NULL);

        fds[0].fd = (int) selectionObject;
        fds[0].events = POLLIN;
        while (1){
                (void)poll(fds, 1, 1000);
                if (fds[0].revents & POLLIN){
                        break;
                }
        }
        safassert(saClmDispatch(clmHandle,SA_DISPATCH_ALL), SA_AIS_OK);
        safassert(saClmResponse_4(clmHandle,lock_inv,SA_CLM_CALLBACK_RESPONSE_REJECTED),SA_AIS_OK);

        fds[0].fd = (int) selectionObject;
        fds[0].events = POLLIN;
        while (1){
                (void)poll(fds, 1, 1000);
                if (fds[0].revents & POLLIN){
                        break;
                }
        }

        safassert(saClmDispatch(clmHandle,SA_DISPATCH_ALL), SA_AIS_OK);

        safassert(saClmClusterTrackStop(clmHandle), SA_AIS_OK);
        safassert(saClmFinalize(clmHandle), SA_AIS_OK);
        test_validate(rc, SA_AIS_OK);

}


void saClmResponse_04(void)
{
	printf("This test case needs (manual) admin op simulation\n");
	return; /* uncomment this and run*/
	rc = saClmResponse_4(0,200,SA_CLM_CALLBACK_RESPONSE_REJECTED);
	test_validate(rc, SA_AIS_ERR_INVALID_PARAM);
        rc = saClmResponse_4(1234,0,SA_CLM_CALLBACK_RESPONSE_REJECTED);
        test_validate(rc, SA_AIS_ERR_INVALID_PARAM);
        rc = saClmResponse_4(1234,200,10);
        test_validate(rc, SA_AIS_ERR_INVALID_PARAM);
        rc = saClmResponse_4(-1,200,SA_CLM_CALLBACK_RESPONSE_REJECTED);
        test_validate(rc, SA_AIS_ERR_BAD_HANDLE);
}

void saClmResponse_05(void)
{
	safassert(saClmInitialize_4(&clmHandle, NULL, &clmVersion_4), SA_AIS_OK);
	rc = saClmResponse_4(clmHandle,100,SA_CLM_CALLBACK_RESPONSE_OK);
	test_validate(rc, SA_AIS_ERR_INVALID_PARAM);
}

__attribute__ ((constructor)) static void saClmResponse_constructor(void)
{
        test_suite_add(8, "Test case for saClmResponse");

	test_case_add(8, saClmResponse_01, "saClmResponse with response as OK");
	test_case_add(8, saClmResponse_02, "saClmResponse with response as error");
	test_case_add(8, saClmResponse_03, "saClmResponse with response as rejected");

	test_case_add(8, saClmResponse_04, "saClmResponse with invalid param");
	test_case_add(8, saClmResponse_05, "saClmResponse with invalid param");
}
