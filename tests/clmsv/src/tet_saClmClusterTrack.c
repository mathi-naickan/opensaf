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
#include <pthread.h>

SaClmClusterNotificationBufferT notificationBuffer_1;
SaClmClusterNotificationBufferT_4 notificationBuffer_4;
SaUint8T trackFlags;
SaClmNodeIdT nodeId;
SaInvocationT invocation;
SaInvocationT lock_inv;

static void* admin_lock(void* dummy)
{
        int rc;
        char command[256];
        char name[]= "safNode=SC_2_1,safCluster=myClmCluster";

        sprintf(command, "immadm -o 2 %s",name);
        assert((rc = system(command)) != -1);
        /*test_validate(WEXITSTATUS(rc), 0);*/
	return NULL;
}

static void* admin_unlock(void* dummy)
{
        int rc;
        char command[256];
        char name[]= "safNode=SC_2_1,safCluster=myClmCluster";

        sprintf(command, "immadm -o 1 %s",name);
        assert((rc = system(command)) != -1);
        /*test_validate(WEXITSTATUS(rc), 0);*/
	return NULL;
}

static void* admin_shutdown(void* dummy)
{
        int rc;
        char command[256];
        char name[]= "safNode=SC_2_1,safCluster=myClmCluster";

        sprintf(command, "immadm -o 3 %s",name);
        assert((rc = system(command)) != -1);
        /*test_validate(WEXITSTATUS(rc), 0);*/
	return NULL;
}

static void saClmadmin_lock1(void)
{
        int rc;
        char command[256];
        char name[]= "safNode=SC_2_1,safCluster=myClmCluster";

        sprintf(command, "immadm -o 2 %s",name);
        assert((rc = system(command)) != -1);
        test_validate(WEXITSTATUS(rc), 1);
}

static void saClmadmin_unlock1(void)
{
        int rc;
        char command[256];
        char name[]= "safNode=SC_2_1,safCluster=myClmCluster";

        sprintf(command, "immadm -o 1 %s",name);
        assert((rc = system(command)) != -1);
        test_validate(WEXITSTATUS(rc), 1);
}

static void saClmadmin_shutdown1(void)
{
        int rc;
        char command[256];
        char name[]= "safNode=SC_2_1,safCluster=myClmCluster";

        sprintf(command, "immadm -o 3 %s",name);
        assert((rc = system(command)) != -1);
        test_validate(WEXITSTATUS(rc), 1);
}

static void* plm_admin_trylock(void* dummy)
{

	int rc;
        char command[256];
        char name[]="safEE=Linux_os_hosting_clm_node,safDomain=domain_1";

        sprintf(command, "immadm -o 1 -p SA_PLM_ADMIN_LOCK_OPTION:SA_STRING_T:trylock  %s",name);
        assert((rc = system(command)) != -1);
	return NULL;
}

static void saClmPlm_unlock(void)
{

	int rc;
        char command[256];
        char name[]="safEE=Linux_os_hosting_clm_node,safDomain=domain_1";

        sprintf(command, "immadm -o 2 %s",name);
        assert((rc = system(command)) != -1);

}


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

static void clmTrackCallback4(const SaClmClusterNotificationBufferT_4 *notificationBuffer,
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

static void clmTrackCallback1(const SaClmClusterNotificationBufferT *notificationBuffer,
			SaUint32T numberOfMembers,
			SaAisErrorT error)
{
printf("\n");
printf("\n Inside TrackCallback");
printf("\n No of items = %d", notificationBuffer->numberOfItems);
printf("\n rc = %d", error);
printf("\n");
}

static void nodeGetCallBack1(SaInvocationT invocation,
                             const SaClmClusterNodeT *clusterNode,
                             SaAisErrorT error) {
printf("\n inside of node get callback1");
}

static void nodeGetCallBack4(SaInvocationT invocation,
                             const SaClmClusterNodeT_4 *clusterNode,
                             SaAisErrorT error) {

printf("\n inside of node get callback4");
}

SaClmCallbacksT_4 clmCallback4 = {nodeGetCallBack4, clmTrackCallback4};
SaClmCallbacksT clmCallback1 = {nodeGetCallBack1, clmTrackCallback1};

void saClmClusterTrack_01(void)
{
	trackFlags = SA_TRACK_CURRENT;
	notificationBuffer_1.numberOfItems = 10;
	notificationBuffer_1.notification = (SaClmClusterNotificationT*)
		malloc(sizeof(SaClmClusterNotificationT)*notificationBuffer_1.numberOfItems);

        safassert(saClmInitialize(&clmHandle, &clmCallbacks_1, &clmVersion_1), SA_AIS_OK);
	rc = saClmClusterTrack(clmHandle, trackFlags, &notificationBuffer_1);
	safassert(saClmClusterTrackStop(clmHandle), SA_AIS_OK);
	free(notificationBuffer_1.notification);
        safassert(saClmFinalize(clmHandle), SA_AIS_OK);
        test_validate(rc, SA_AIS_OK);
}

void saClmClusterTrack_02(void)
{
        trackFlags = SA_TRACK_CURRENT;
	notificationBuffer_4.numberOfItems = 10;
	notificationBuffer_4.notification = (SaClmClusterNotificationT_4*)
		malloc(sizeof(SaClmClusterNotificationT_4)*notificationBuffer_4.numberOfItems);

        safassert(saClmInitialize_4(&clmHandle, &clmCallbacks_4, &clmVersion_4), SA_AIS_OK);
        rc = saClmClusterTrack_4(clmHandle, trackFlags, &notificationBuffer_4);
	clmTrackbuf_4(&notificationBuffer_4);
        safassert(saClmClusterTrackStop(clmHandle), SA_AIS_OK);
        safassert(saClmClusterNotificationFree_4(clmHandle, notificationBuffer_4.notification), SA_AIS_OK);
        safassert(saClmFinalize(clmHandle), SA_AIS_OK);
        test_validate(rc, SA_AIS_OK);
}

void saClmClusterTrack_03(void)
{
        trackFlags = (SA_TRACK_CURRENT | SA_TRACK_LOCAL);
        notificationBuffer_1.numberOfItems = 1;
        notificationBuffer_1.notification = (SaClmClusterNotificationT*)
                malloc(sizeof(SaClmClusterNotificationT)*notificationBuffer_1.numberOfItems);

        safassert(saClmInitialize(&clmHandle, &clmCallbacks_1, &clmVersion_1), SA_AIS_OK);
        rc = saClmClusterTrack(clmHandle, trackFlags, &notificationBuffer_1);
        safassert(saClmClusterTrackStop(clmHandle), SA_AIS_OK);
	free(notificationBuffer_1.notification);
        safassert(saClmFinalize(clmHandle), SA_AIS_OK);
        test_validate(rc, SA_AIS_OK);
}

void saClmClusterTrack_04(void)
{
        trackFlags = (SA_TRACK_CURRENT | SA_TRACK_LOCAL);
        notificationBuffer_4.numberOfItems = 1;
        notificationBuffer_4.notification = (SaClmClusterNotificationT_4*)
                malloc(sizeof(SaClmClusterNotificationT_4)*notificationBuffer_4.numberOfItems);

        safassert(saClmInitialize_4(&clmHandle, &clmCallbacks_4, &clmVersion_4), SA_AIS_OK);
        rc = saClmClusterTrack_4(clmHandle, trackFlags, &notificationBuffer_4);
	clmTrackbuf_4(&notificationBuffer_4);
        safassert(saClmClusterTrackStop(clmHandle), SA_AIS_OK);
        safassert(saClmClusterNotificationFree_4(clmHandle, notificationBuffer_4.notification), SA_AIS_OK);
        safassert(saClmFinalize(clmHandle), SA_AIS_OK);
        test_validate(rc, SA_AIS_OK);
}

void saClmClusterTrack_05(void)
{
        trackFlags = (SA_TRACK_CURRENT | SA_TRACK_LOCAL);
        notificationBuffer_1.numberOfItems = 1;
        notificationBuffer_1.notification = (SaClmClusterNotificationT*)
                malloc(sizeof(SaClmClusterNotificationT)*notificationBuffer_1.numberOfItems);

        safassert(saClmInitialize(&clmHandle, &clmCallbacks_1, &clmVersion_1), SA_AIS_OK);
        rc = saClmClusterTrack(0, trackFlags, &notificationBuffer_1);
        /*safassert(saClmClusterTrackStop(clmHandle), SA_AIS_OK);*/
	free(notificationBuffer_1.notification);
        safassert(saClmFinalize(clmHandle), SA_AIS_OK);
        test_validate(rc, SA_AIS_ERR_INVALID_PARAM);

        trackFlags = (SA_TRACK_CURRENT | SA_TRACK_LOCAL);
        notificationBuffer_1.numberOfItems = 1;
        notificationBuffer_1.notification = (SaClmClusterNotificationT*)
                malloc(sizeof(SaClmClusterNotificationT)*notificationBuffer_1.numberOfItems);
        
        safassert(saClmInitialize(&clmHandle, &clmCallbacks_1, &clmVersion_1), SA_AIS_OK);
        rc = saClmClusterTrack(-1, trackFlags, &notificationBuffer_1);
        /*safassert(saClmClusterTrackStop(clmHandle), SA_AIS_OK);*/
	free(notificationBuffer_1.notification);
        safassert(saClmFinalize(clmHandle), SA_AIS_OK);
        test_validate(rc, SA_AIS_ERR_BAD_HANDLE);
}

void saClmClusterTrack_06(void)
{
        trackFlags = (SA_TRACK_CURRENT | SA_TRACK_LOCAL);
        notificationBuffer_1.numberOfItems = 1;
        notificationBuffer_1.notification = (SaClmClusterNotificationT*)
                malloc(sizeof(SaClmClusterNotificationT)*notificationBuffer_1.numberOfItems);
        
        safassert(saClmInitialize(&clmHandle, &clmCallbacks_1, &clmVersion_1), SA_AIS_OK);
        rc = saClmClusterTrack(clmHandle, 0, &notificationBuffer_1);
        /*safassert(saClmClusterTrackStop(clmHandle), SA_AIS_OK);*/
	free(notificationBuffer_1.notification);
        safassert(saClmFinalize(clmHandle), SA_AIS_OK);
        test_validate(rc, SA_AIS_ERR_INVALID_PARAM);
}

void saClmClusterTrack_07(void)
{
        trackFlags = SA_TRACK_CURRENT; 
        notificationBuffer_1.numberOfItems = 1;
        notificationBuffer_1.notification = (SaClmClusterNotificationT*)
                malloc(sizeof(SaClmClusterNotificationT)*notificationBuffer_1.numberOfItems);

        /*safassert(saClmInitialize(&clmHandle, &clmCallbacks_1, &clmVersion_1), SA_AIS_OK);*/
        safassert(saClmInitialize(&clmHandle,NULL, &clmVersion_1), SA_AIS_OK);
        /*rc = saClmClusterTrack(clmHandle, trackFlags, &notificationBuffer_1); */
        rc = saClmClusterTrack(clmHandle, trackFlags, NULL); 
        /*safassert(saClmClusterTrackStop(clmHandle), SA_AIS_OK);*/
	free(notificationBuffer_1.notification);
        safassert(saClmFinalize(clmHandle), SA_AIS_OK);
        test_validate(rc, SA_AIS_ERR_INIT);
}

void saClmClusterTrack_08(void)
{
        trackFlags = (SA_TRACK_CURRENT | SA_TRACK_LOCAL);
        notificationBuffer_4.numberOfItems = 1;
        notificationBuffer_4.notification = (SaClmClusterNotificationT_4*)
                malloc(sizeof(SaClmClusterNotificationT_4)*notificationBuffer_4.numberOfItems);

        safassert(saClmInitialize_4(&clmHandle, &clmCallbacks_4, &clmVersion_4), SA_AIS_OK);
        rc = saClmClusterTrack_4(0, trackFlags, &notificationBuffer_4);
        /*safassert(saClmClusterTrackStop(clmHandle), SA_AIS_OK);*/
        safassert(saClmClusterNotificationFree_4(clmHandle, notificationBuffer_4.notification), SA_AIS_OK);
        safassert(saClmFinalize(clmHandle), SA_AIS_OK);
        test_validate(rc, SA_AIS_ERR_INVALID_PARAM);

        trackFlags = (SA_TRACK_CURRENT | SA_TRACK_LOCAL);
        notificationBuffer_4.numberOfItems = 1;
        notificationBuffer_4.notification = (SaClmClusterNotificationT_4*)
                malloc(sizeof(SaClmClusterNotificationT_4)*notificationBuffer_4.numberOfItems);

        safassert(saClmInitialize_4(&clmHandle, &clmCallbacks_4, &clmVersion_4), SA_AIS_OK);
        rc = saClmClusterTrack_4(-1, trackFlags, &notificationBuffer_4);
        /*safassert(saClmClusterTrackStop(clmHandle), SA_AIS_OK);*/
        safassert(saClmClusterNotificationFree_4(clmHandle, notificationBuffer_4.notification), SA_AIS_OK);
        safassert(saClmFinalize(clmHandle), SA_AIS_OK);
        test_validate(rc, SA_AIS_ERR_BAD_HANDLE);
}

void saClmClusterTrack_09(void)
{
        trackFlags = (SA_TRACK_CURRENT | SA_TRACK_LOCAL);
        notificationBuffer_4.numberOfItems = 1;
        notificationBuffer_4.notification = (SaClmClusterNotificationT_4*)
                malloc(sizeof(SaClmClusterNotificationT_4)*notificationBuffer_4.numberOfItems);

        safassert(saClmInitialize_4(&clmHandle, &clmCallbacks_4, &clmVersion_4), SA_AIS_OK);
        rc = saClmClusterTrack_4(clmHandle, 0, &notificationBuffer_4);
        /*safassert(saClmClusterTrackStop(clmHandle), SA_AIS_OK);*/
        safassert(saClmClusterNotificationFree_4(clmHandle, notificationBuffer_4.notification), SA_AIS_OK);
        safassert(saClmFinalize(clmHandle), SA_AIS_OK);
        test_validate(rc, SA_AIS_ERR_INVALID_PARAM);
}

void saClmClusterTrack_10(void)
{
        trackFlags = (SA_TRACK_CURRENT | SA_TRACK_LOCAL);
        notificationBuffer_4.numberOfItems = 1;
        notificationBuffer_4.notification = (SaClmClusterNotificationT_4*)
                malloc(sizeof(SaClmClusterNotificationT_4)*notificationBuffer_4.numberOfItems);

        safassert(saClmInitialize_4(&clmHandle, &clmCallbacks_4, &clmVersion_4), SA_AIS_OK);
        rc = saClmClusterTrack_4(clmHandle, trackFlags, NULL);
        /*safassert(saClmClusterTrackStop(clmHandle), SA_AIS_OK);*/
        safassert(saClmClusterNotificationFree_4(clmHandle, notificationBuffer_4.notification), SA_AIS_OK);
        safassert(saClmFinalize(clmHandle), SA_AIS_OK);
        test_validate(rc, SA_AIS_ERR_INIT);
}

void saClmClusterTrack_11(void)
{
        trackFlags = SA_TRACK_CURRENT;
        notificationBuffer_4.numberOfItems = 10;
        notificationBuffer_4.notification = (SaClmClusterNotificationT_4*)
                malloc(sizeof(SaClmClusterNotificationT_4)*notificationBuffer_1.numberOfItems);

        safassert(saClmInitialize(&clmHandle, &clmCallbacks_1, &clmVersion_1), SA_AIS_OK);
        rc = saClmClusterTrack_4(clmHandle, trackFlags, &notificationBuffer_4);
        /*safassert(saClmClusterTrackStop(clmHandle), SA_AIS_OK);*/
        free(notificationBuffer_4.notification);
	safassert(saClmFinalize(clmHandle), SA_AIS_OK);
        test_validate(rc, SA_AIS_ERR_VERSION);
}

void saClmClusterTrack_12(void)
{
        trackFlags = SA_TRACK_CURRENT;
        notificationBuffer_1.numberOfItems = 10;
        notificationBuffer_1.notification = (SaClmClusterNotificationT*)
                malloc(sizeof(SaClmClusterNotificationT)*notificationBuffer_1.numberOfItems);

        safassert(saClmInitialize_4(&clmHandle, &clmCallbacks_4, &clmVersion_4), SA_AIS_OK);
        rc = saClmClusterTrack(clmHandle, trackFlags, &notificationBuffer_1);
        /*safassert(saClmClusterTrackStop(clmHandle), SA_AIS_OK);*/
	free(notificationBuffer_1.notification);
        safassert(saClmFinalize(clmHandle), SA_AIS_OK);
        test_validate(rc, SA_AIS_ERR_VERSION);
}

void saClmClusterTrack_13(void)
{
        trackFlags = SA_TRACK_CURRENT;
        safassert(saClmInitialize(&clmHandle, &clmCallback1, &clmVersion_1), SA_AIS_OK);
        rc = saClmClusterTrack(clmHandle, trackFlags, NULL);
        safassert(saClmDispatch(clmHandle,SA_DISPATCH_ALL), SA_AIS_OK);
        safassert(saClmClusterTrackStop(clmHandle), SA_AIS_OK);
        safassert(saClmFinalize(clmHandle), SA_AIS_OK);
        test_validate(rc, SA_AIS_OK);
}

void saClmClusterTrack_14(void)
{
        trackFlags = SA_TRACK_CURRENT;
	struct pollfd fds[1];
	int ret;

        safassert(saClmInitialize_4(&clmHandle, &clmCallback4, &clmVersion_4), SA_AIS_OK);
	safassert(saClmSelectionObjectGet(clmHandle, &selectionObject),SA_AIS_OK);
        rc = saClmClusterTrack_4(clmHandle, trackFlags, NULL);

	fds[0].fd = (int) selectionObject;
	fds[0].events = POLLIN;
    	ret = poll(fds, 1, 1000);
    	assert(ret == 1);	

        safassert(saClmDispatch(clmHandle,SA_DISPATCH_ALL), SA_AIS_OK);
        safassert(saClmClusterTrackStop(clmHandle), SA_AIS_OK);
        safassert(saClmFinalize(clmHandle), SA_AIS_OK);
	test_validate(rc, SA_AIS_OK);
}

/*Bad flags - only start step without changes/changes only*/
void saClmClusterTrack_15(void)
{
	trackFlags = SA_TRACK_START_STEP;
        safassert(saClmInitialize_4(&clmHandle, &clmCallback4, &clmVersion_4), SA_AIS_OK);
        rc = saClmClusterTrack_4(clmHandle, trackFlags, NULL);
        /*safassert(saClmClusterTrackStop(clmHandle), SA_AIS_OK);*/
        safassert(saClmFinalize(clmHandle), SA_AIS_OK);
        test_validate(rc, SA_AIS_ERR_BAD_FLAGS);
}

/*Bad flags - only validate step without changes/changes only*/
void saClmClusterTrack_16(void)
{  
        trackFlags = SA_TRACK_VALIDATE_STEP;
        safassert(saClmInitialize_4(&clmHandle, &clmCallback4, &clmVersion_4), SA_AIS_OK);
        rc = saClmClusterTrack_4(clmHandle, trackFlags, NULL);
        /*safassert(saClmClusterTrackStop(clmHandle), SA_AIS_OK);*/
        safassert(saClmFinalize(clmHandle), SA_AIS_OK);
        test_validate(rc, SA_AIS_ERR_BAD_FLAGS);
} 

/*Bad flags - both changes and changes only set */
void saClmClusterTrack_17(void)
{
	trackFlags = SA_TRACK_CHANGES|SA_TRACK_CHANGES_ONLY;
        safassert(saClmInitialize_4(&clmHandle, &clmCallback4, &clmVersion_4), SA_AIS_OK);
        rc = saClmClusterTrack_4(clmHandle, trackFlags, NULL);
        /*safassert(saClmClusterTrackStop(clmHandle), SA_AIS_OK);*/
        safassert(saClmFinalize(clmHandle), SA_AIS_OK);
        test_validate(rc, SA_AIS_ERR_BAD_FLAGS);
}
/*Bad Flags - only start step without changes/changes only for old API's*/
void saClmClusterTrack_18(void)
{
        trackFlags = SA_TRACK_START_STEP;
        safassert(saClmInitialize(&clmHandle, &clmCallback1, &clmVersion_1), SA_AIS_OK);
        rc = saClmClusterTrack(clmHandle, trackFlags, NULL);
        /*safassert(saClmClusterTrackStop(clmHandle), SA_AIS_OK);*/
        safassert(saClmFinalize(clmHandle), SA_AIS_OK);
        test_validate(rc, SA_AIS_ERR_BAD_FLAGS);
}

/*Bad flags - only validate step without changes/changes only for old API's*/
void saClmClusterTrack_19(void)
{
        trackFlags = SA_TRACK_VALIDATE_STEP;
        safassert(saClmInitialize(&clmHandle, &clmCallback1, &clmVersion_1), SA_AIS_OK);
        rc = saClmClusterTrack(clmHandle, trackFlags, NULL);
        /*safassert(saClmClusterTrackStop(clmHandle), SA_AIS_OK);*/
        safassert(saClmFinalize(clmHandle), SA_AIS_OK);
        test_validate(rc, SA_AIS_ERR_BAD_FLAGS);
}

/*Bad flags - both changes and changes only set for old API's*/
void saClmClusterTrack_20(void)
{
        trackFlags = SA_TRACK_CHANGES|SA_TRACK_CHANGES_ONLY;
        safassert(saClmInitialize(&clmHandle, &clmCallback1, &clmVersion_1), SA_AIS_OK);
        rc = saClmClusterTrack(clmHandle, trackFlags, NULL);
        /*safassert(saClmClusterTrackStop(clmHandle), SA_AIS_OK);*/
        safassert(saClmFinalize(clmHandle), SA_AIS_OK);
        test_validate(rc, SA_AIS_ERR_BAD_FLAGS);
}

void saClmClusterTrack_21(void)
{
	struct pollfd fds[1];
        int ret;
	pthread_t thread;

        trackFlags = SA_TRACK_CHANGES_ONLY;

        safassert(saClmInitialize_4(&clmHandle, &clmCallback4, &clmVersion_4), SA_AIS_OK);
        safassert(saClmSelectionObjectGet(clmHandle, &selectionObject),SA_AIS_OK);
        rc = saClmClusterTrack_4(clmHandle, trackFlags, NULL);

        fds[0].fd = (int) selectionObject;
        fds[0].events = POLLIN;
	
	if (pthread_create(&thread,NULL,admin_lock, NULL) != 0) {
                printf("thread creation failed");
        }

	while(1){
	        ret = poll(fds, 1, 1000);
		assert(ret!=(-1));
		if (fds[0].revents & POLLIN)
                        break;
	}

        safassert(saClmDispatch(clmHandle,SA_DISPATCH_ALL), SA_AIS_OK);
        safassert(saClmClusterTrackStop(clmHandle), SA_AIS_OK);
        safassert(saClmFinalize(clmHandle), SA_AIS_OK);
        test_validate(rc, SA_AIS_OK);

}

void saClmClusterTrack_22(void)
{
        struct pollfd fds[1];
        int ret;
	pthread_t thread1;

        trackFlags = SA_TRACK_CHANGES_ONLY;

        safassert(saClmInitialize_4(&clmHandle, &clmCallback4, &clmVersion_4), SA_AIS_OK);
        safassert(saClmSelectionObjectGet(clmHandle, &selectionObject),SA_AIS_OK);
        rc = saClmClusterTrack_4(clmHandle, trackFlags, NULL);

        fds[0].fd = (int) selectionObject;
        fds[0].events = POLLIN;

        if (pthread_create(&thread1,NULL,admin_unlock, NULL) != 0) {
                printf("thread creation failed");
        }

        while(1){
                ret = poll(fds, 1, 1000);
		assert(ret!=(-1));
                if (fds[0].revents & POLLIN)
                        break;
        }

        safassert(saClmDispatch(clmHandle,SA_DISPATCH_ALL), SA_AIS_OK);
        safassert(saClmClusterTrackStop(clmHandle), SA_AIS_OK);
        safassert(saClmFinalize(clmHandle), SA_AIS_OK);
        test_validate(rc, SA_AIS_OK);

}


void saClmClusterTrack_23(void)
{
        struct pollfd fds[1];
        int ret;
	pthread_t thread2;

        trackFlags = SA_TRACK_CHANGES;

        safassert(saClmInitialize_4(&clmHandle, &clmCallback4, &clmVersion_4), SA_AIS_OK);
        safassert(saClmSelectionObjectGet(clmHandle, &selectionObject),SA_AIS_OK);
        rc = saClmClusterTrack_4(clmHandle, trackFlags, NULL);

        fds[0].fd = (int) selectionObject;
        fds[0].events = POLLIN;

        if (pthread_create(&thread2,NULL,admin_lock, NULL) != 0) {
                printf("thread creation failed");
	}


	while (1){
        	ret = poll(fds, 1, 1000);
		assert(ret!=(-1));
		if (fds[0].revents & POLLIN)
			break;
	}	

        safassert(saClmDispatch(clmHandle,SA_DISPATCH_ALL), SA_AIS_OK);
        safassert(saClmClusterTrackStop(clmHandle), SA_AIS_OK);
        safassert(saClmFinalize(clmHandle), SA_AIS_OK);
        test_validate(rc, SA_AIS_OK);

}

void saClmClusterTrack_24(void)
{
	saClmClusterTrack_22();
}



void saClmClusterTrack_25(void)
{
        struct pollfd fds[1];
        int ret;
	pthread_t thread4;

        trackFlags = SA_TRACK_CHANGES|SA_TRACK_START_STEP;

        safassert(saClmInitialize_4(&clmHandle, &clmCallback4, &clmVersion_4), SA_AIS_OK);
        safassert(saClmSelectionObjectGet(clmHandle, &selectionObject),SA_AIS_OK);
        rc = saClmClusterTrack_4(clmHandle, trackFlags, NULL);

        fds[0].fd = (int) selectionObject;
        fds[0].events = POLLIN;

        if (pthread_create(&thread4,NULL,admin_lock, NULL) != 0) {
                printf("thread creation failed");
	}
        while (1){
                ret = poll(fds, 1, 1000);
		assert(ret!=(-1));
                if (fds[0].revents & POLLIN){
                        break;
		}
        }
	safassert(saClmDispatch(clmHandle,SA_DISPATCH_ALL), SA_AIS_OK);
	safassert(saClmResponse_4(clmHandle,lock_inv,SA_CLM_CALLBACK_RESPONSE_OK),SA_AIS_OK);

	fds[0].fd = (int) selectionObject;
        fds[0].events = POLLIN;
        while (1){
                ret = poll(fds, 1, 1000);
		assert(ret!=(-1));
                if (fds[0].revents & POLLIN){
                        break;
                }
        }

        safassert(saClmDispatch(clmHandle,SA_DISPATCH_ALL), SA_AIS_OK);
	
        safassert(saClmClusterTrackStop(clmHandle), SA_AIS_OK);
        safassert(saClmFinalize(clmHandle), SA_AIS_OK);
        test_validate(rc, SA_AIS_OK);

}

void saClmClusterTrack_26(void)
{
	saClmClusterTrack_22();
}

void saClmClusterTrack_27(void)
{
        struct pollfd fds[1];
        int ret;
	pthread_t thread6;

        trackFlags = SA_TRACK_CHANGES_ONLY|SA_TRACK_START_STEP;

        safassert(saClmInitialize_4(&clmHandle, &clmCallback4, &clmVersion_4), SA_AIS_OK);
        safassert(saClmSelectionObjectGet(clmHandle, &selectionObject),SA_AIS_OK);
        rc = saClmClusterTrack_4(clmHandle, trackFlags, NULL);

        fds[0].fd = (int) selectionObject;
        fds[0].events = POLLIN;

	if (pthread_create(&thread6,NULL,admin_lock, NULL) != 0) {
		printf("thread creation failed");
	}
	
        while (1){
                ret = poll(fds, 1, 1000);
		assert(ret!=(-1));
                if (fds[0].revents & POLLIN){
                        break;
                }
        }
        safassert(saClmDispatch(clmHandle,SA_DISPATCH_ALL), SA_AIS_OK);
        safassert(saClmResponse_4(clmHandle,lock_inv,SA_CLM_CALLBACK_RESPONSE_OK),SA_AIS_OK);

        fds[0].fd = (int) selectionObject;
        fds[0].events = POLLIN;
        while (1){
                ret = poll(fds, 1, 1000);
		assert(ret!=(-1));
                if (fds[0].revents & POLLIN){
                        break;
                }
        }

        safassert(saClmDispatch(clmHandle,SA_DISPATCH_ALL), SA_AIS_OK);

        safassert(saClmClusterTrackStop(clmHandle), SA_AIS_OK);
        safassert(saClmFinalize(clmHandle), SA_AIS_OK);
        test_validate(rc, SA_AIS_OK);

}

void saClmClusterTrack_29(void)
{
	saClmClusterTrack_22();
}

void saClmClusterTrack_30(void)
{
        struct pollfd fds[1];
        int ret;
        pthread_t thread8;

        trackFlags = SA_TRACK_CHANGES_ONLY|SA_TRACK_START_STEP;

        safassert(saClmInitialize_4(&clmHandle, &clmCallback4, &clmVersion_4), SA_AIS_OK);
        safassert(saClmSelectionObjectGet(clmHandle, &selectionObject),SA_AIS_OK);
        rc = saClmClusterTrack_4(clmHandle, trackFlags, NULL);

        fds[0].fd = (int) selectionObject;
        fds[0].events = POLLIN;

        if (pthread_create(&thread8,NULL,admin_shutdown, NULL) != 0) {
                printf("thread creation failed");
        }

        while (1){
                ret = poll(fds, 1, 1000);
		assert(ret!=(-1));
                if (fds[0].revents & POLLIN){
                        break;
                }
        }
        safassert(saClmDispatch(clmHandle,SA_DISPATCH_ALL), SA_AIS_OK);
        safassert(saClmResponse_4(clmHandle,lock_inv,SA_CLM_CALLBACK_RESPONSE_OK),SA_AIS_OK);

        fds[0].fd = (int) selectionObject;
        fds[0].events = POLLIN;
        while (1){
                ret = poll(fds, 1, 1000);
		assert(ret!=(-1));
                if (fds[0].revents & POLLIN){
                        break;
                }
        }

        safassert(saClmDispatch(clmHandle,SA_DISPATCH_ALL), SA_AIS_OK);

        safassert(saClmClusterTrackStop(clmHandle), SA_AIS_OK);
        safassert(saClmFinalize(clmHandle), SA_AIS_OK);
        test_validate(rc, SA_AIS_OK);

}
void saClmClusterTrack_31(void)
{
	saClmClusterTrack_22();
}
void saClmClusterTrack_32(void)
{
        struct pollfd fds[1];
        int ret;
        pthread_t thread8;

        trackFlags = SA_TRACK_CHANGES|SA_TRACK_START_STEP;

        safassert(saClmInitialize_4(&clmHandle, &clmCallback4, &clmVersion_4), SA_AIS_OK);
        safassert(saClmSelectionObjectGet(clmHandle, &selectionObject),SA_AIS_OK);
        rc = saClmClusterTrack_4(clmHandle, trackFlags, NULL);

        fds[0].fd = (int) selectionObject;
        fds[0].events = POLLIN;

        if (pthread_create(&thread8,NULL,admin_shutdown, NULL) != 0) {
                printf("thread creation failed");
        }

        while (1){
                ret = poll(fds, 1, 1000);
		assert(ret!=(-1));
                if (fds[0].revents & POLLIN){
                        break;
                }
        }
        safassert(saClmDispatch(clmHandle,SA_DISPATCH_ALL), SA_AIS_OK);
        safassert(saClmResponse_4(clmHandle,lock_inv,SA_CLM_CALLBACK_RESPONSE_OK),SA_AIS_OK);

        fds[0].fd = (int) selectionObject;
        fds[0].events = POLLIN;
        while (1){
                ret = poll(fds, 1, 1000);
		assert(ret!=(-1));
                if (fds[0].revents & POLLIN){
                        break;
                }
        }

        safassert(saClmDispatch(clmHandle,SA_DISPATCH_ALL), SA_AIS_OK);

        safassert(saClmClusterTrackStop(clmHandle), SA_AIS_OK);
        safassert(saClmFinalize(clmHandle), SA_AIS_OK);
        test_validate(rc, SA_AIS_OK);

}

void saClmClusterTrack_34(void)
{
        saClmClusterTrack_22();
}

/*plm admin trylock*/
void saClmClusterTrack_36(void)
{
	struct pollfd fds[1];
        int ret;
        pthread_t thread4;

        trackFlags = SA_TRACK_CHANGES|SA_TRACK_START_STEP|SA_TRACK_VALIDATE_STEP;

        safassert(saClmInitialize_4(&clmHandle, &clmCallback4, &clmVersion_4), SA_AIS_OK);
        safassert(saClmSelectionObjectGet(clmHandle, &selectionObject),SA_AIS_OK);
        rc = saClmClusterTrack_4(clmHandle, trackFlags, NULL);

        fds[0].fd = (int) selectionObject;
        fds[0].events = POLLIN;

        if (pthread_create(&thread4,NULL,plm_admin_trylock, NULL) != 0) {
                printf("thread creation failed");
        }
        while (1){
                ret = poll(fds, 1, 1000);
		assert(ret!=(-1));
                if (fds[0].revents & POLLIN){
                        break;
                }
        }
        safassert(saClmDispatch(clmHandle,SA_DISPATCH_ALL), SA_AIS_OK); 
        safassert(saClmResponse_4(clmHandle,lock_inv,SA_CLM_CALLBACK_RESPONSE_OK),SA_AIS_OK);

        fds[0].fd = (int) selectionObject;
        fds[0].events = POLLIN;
        while (1){
                ret = poll(fds, 1, 1000);
		assert(ret!=(-1));
                if (fds[0].revents & POLLIN){
                        break;
                }
        }

        safassert(saClmDispatch(clmHandle,SA_DISPATCH_ALL), SA_AIS_OK);
        safassert(saClmResponse_4(clmHandle,lock_inv,SA_CLM_CALLBACK_RESPONSE_OK),SA_AIS_OK);

        fds[0].fd = (int) selectionObject;
        fds[0].events = POLLIN;
        while (1){
                ret = poll(fds, 1, 1000);
		assert(ret!=(-1));
                if (fds[0].revents & POLLIN){
                        break;
                }
        }
        safassert(saClmDispatch(clmHandle,SA_DISPATCH_ALL), SA_AIS_OK);
        safassert(saClmClusterTrackStop(clmHandle), SA_AIS_OK);
	safassert(saClmFinalize(clmHandle), SA_AIS_OK);
        test_validate(rc, SA_AIS_OK);
}

__attribute__ ((constructor)) static void saClmClusterTrack_constructor(void)
{
        test_suite_add(7, "Test case for saClmClusterTrack");
	test_case_add(7, saClmClusterTrack_01, "saClmClusterTrack with valid param");
	test_case_add(7, saClmClusterTrack_02, "saClmClusterTrack_4 with valid param");
	test_case_add(7, saClmClusterTrack_03, "saClmClusterTrack with (SA_TRACK_CURRENT | SA_TRACK_LOCAL)");
	test_case_add(7, saClmClusterTrack_04, "saClmClusterTrack_4 with (SA_TRACK_CURRENT | SA_TRACK_LOCAL)");
	test_case_add(7, saClmClusterTrack_05, "saClmClusterTrack with null handle or invalid handle)");
	test_case_add(7, saClmClusterTrack_06, "saClmClusterTrack with null flags)");
	test_case_add(7, saClmClusterTrack_07, "saClmClusterTrack with null buffer & null callbacks)");
        test_case_add(7, saClmClusterTrack_08, "saClmClusterTrack_4 with null handle or invalid handle)");
        test_case_add(7, saClmClusterTrack_09, "saClmClusterTrack_4 with null flags");
        test_case_add(7, saClmClusterTrack_10, "saClmClusterTrack_4 with null buffer & null callbacks)");
        test_case_add(7, saClmClusterTrack_11, "saClmClusterTrack_4 with handle of B.01.01");
        test_case_add(7, saClmClusterTrack_12, "saClmClusterTrack with handle of B.04.01");
        test_case_add(7, saClmClusterTrack_13, "saClmClusterTrack with valid param & NULL buffer");
        test_case_add(7, saClmClusterTrack_14, "saClmClusterTrack_4 with valid param & NULL buffer");
        test_case_add(7, saClmClusterTrack_15, "saClmClusterTrack_4 with bad flags - only start tarckflag");
        test_case_add(7, saClmClusterTrack_16, "saClmClusterTrack_4 with bad flags - only validate track flag");
        test_case_add(7, saClmClusterTrack_17, "saClmClusterTrack_4 with SA_TRACK_CHANGES and SA_TRACK_CHANGES_ONLY track flags");
        test_case_add(7, saClmClusterTrack_18, "saClmClusterTrack with bad flags - only start tarckflag");
        test_case_add(7, saClmClusterTrack_19, "saClmClusterTrack with with bad flags - only validate track flag");
        test_case_add(7, saClmClusterTrack_20, "saClmClusterTrack with SA_TRACK_CHANGES and SA_TRACK_CHANGES_ONLY track flags");
        test_case_add(7, saClmClusterTrack_21, "saClmClusterTrack_4 with SA_TRACK_CHANGES_ONLY track flags - admin lock");
        test_case_add(7, saClmClusterTrack_22, "saClmClusterTrack_4 with SA_TRACK_CHANGES_ONLY track flags - admin unlock");
        test_case_add(7, saClmClusterTrack_23, "saClmClusterTrack_4 with SA_TRACK_CHANGES track flags - admin lock");
        test_case_add(7, saClmClusterTrack_24, "saClmClusterTrack_4 with SA_TRACK_CHANGES_ONLY track flags - admin unlock");
        test_case_add(7, saClmClusterTrack_25, "saClmClusterTrack_4 with SA_TRACK_CHANGES and START STEP track flags - admin lock");
        test_case_add(7, saClmClusterTrack_26, "saClmClusterTrack_4 with SA_TRACK_CHANGES_ONLY track flags - admin unlock");
        test_case_add(7, saClmClusterTrack_27, "saClmClusterTrack_4 with SA_TRACK_CHANGES_ONLY and START STEP track flags -admin lock");
        test_case_add(7, saClmadmin_lock1,     "admin lock of the already lock node");
        test_case_add(7, saClmClusterTrack_29, "saClmClusterTrack_4 with SA_TRACK_CHANGES_ONLY track flags - admin unlock");
        test_case_add(7, saClmClusterTrack_30, "saClmClusterTrack_4 with SA_TRACK_CHANGES_ONLY and START STEP track flags - adminshutdown ");
        test_case_add(7, saClmClusterTrack_31, "saClmClusterTrack_4 with SA_TRACK_CHANGES_ONLY track flags - admin unlock  ");
        test_case_add(7, saClmClusterTrack_32, "saClmClusterTrack_4 with SA_TRACK_CHANGES track flags - admin shutdown");
        test_case_add(7, saClmadmin_shutdown1, "admin shutdown of already shut node");
        test_case_add(7, saClmClusterTrack_34, "saClmClusterTrack_4 with SA_TRACK_CHANGES_ONLY track flags - admin unlock");
        test_case_add(7, saClmadmin_unlock1,   "unlock already unlocked node");
        test_case_add(7, saClmClusterTrack_36,   "saClmClusterTrack_4 with START STEP,VALIDATE STEP and changes - PLM admin trylock");
        test_case_add(7, saClmPlm_unlock,        "PLM admin Unlock");
}
