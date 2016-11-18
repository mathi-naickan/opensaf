/*      -*- OpenSAF -*-
 *
 * (C) Copyright 2016 The OpenSAF Foundation
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
 * Author(s): Oracle 
 *
 */

#include "ntfs_com.h"

/*
 * @brief  CLM callback for tracking node membership status.
 *	   Depending upon the membership status (joining/leaving cluster) 
 *	   of a node, NTFS will add or remove node from its data base.
 *	   A node is added when it is part of the cluster and is removed
 *	   when it leaves the cluster membership. An update of status is 
 *	   sent to the clients on that node. Based on this status NTFA 
 *	   will decide return code for different API calls.
 *          
*/
static void ntfs_clm_track_cbk(const SaClmClusterNotificationBufferT_4 *notificationBuffer,
        SaUint32T numberOfMembers, SaInvocationT invocation,
        const SaNameT *rootCauseEntity, const SaNtfCorrelationIdsT *correlationIds,
        SaClmChangeStepT step, SaTimeT timeSupervision, SaAisErrorT error)
{
	NODE_ID node_id, *ptr_node_id = NULL;
	SaClmClusterChangesT cluster_change;
	SaBoolT is_member;
	uint32_t i = 0;
	
	TRACE_ENTER2("'%llu' '%u' '%u'", invocation, step, error);
        if (error != SA_AIS_OK) {
		TRACE_1("Error received in ClmTrackCallback");
                goto done;
        }
	ntfs_cb->clm_initialized = true;

	for (i = 0; i < notificationBuffer->numberOfItems; i++) {
		switch(step) {
		case SA_CLM_CHANGE_COMPLETED:
			is_member = notificationBuffer->notification[i].clusterNode.member;
			node_id = notificationBuffer->notification[i].clusterNode.nodeId;
			ptr_node_id = find_member_node(node_id);
			if (ptr_node_id != NULL) {
				TRACE_1("'%x' is present in NTFS db", node_id);
				if (!is_member) {
					TRACE("CLM Node : %x Left the cluster", node_id);
					cluster_change = SA_CLM_NODE_LEFT;
					TRACE("Removing node from NTFS db:%x",node_id);
					remove_member_node(node_id);
					if (ntfs_cb->ha_state == SA_AMF_HA_ACTIVE)
						send_clm_node_status_change(cluster_change, node_id);
				}
			} else {
				TRACE_1("'%x' is not present in NTFS db", node_id);
				if (is_member) {
					TRACE("CLM Node : %x Joined the cluster", node_id);
					cluster_change = SA_CLM_NODE_JOINED;
					TRACE("Adding node to NTFS db:%x",node_id);
					add_member_node(node_id);
					if (ntfs_cb->ha_state == SA_AMF_HA_ACTIVE)
						send_clm_node_status_change(cluster_change, node_id);
				} 
			}
			break;
		default:
			break;
		}
	}

	TRACE("Total Member node(s) in NTFS DB : '%d'",count_member_nodes());
	print_member_nodes();

done:
	TRACE_LEAVE();
	return;

}

static SaVersionT clmVersion = { 'B', 0x04, 0x01 };
static const SaClmCallbacksT_4 clm_callbacks = {
        0,
        ntfs_clm_track_cbk /*saClmClusterTrackCallback*/ 
};

/*
 * @brief   Registers with the CLM service (B.04.01). 
 *
 * @return  SaAisErrorT 
*/
void *ntfs_clm_init_thread(void *cb)
{
	ntfs_cb_t *_ntfs_cb = (ntfs_cb_t *) cb;
	SaAisErrorT rc = SA_AIS_OK;
	TRACE_ENTER();
	rc = saClmInitialize_4(&_ntfs_cb->clm_hdl, &clm_callbacks, &clmVersion);
	if (rc != SA_AIS_OK) {
		LOG_ER("saClmInitialize failed with error: %d", rc);
		TRACE_LEAVE();
                exit(EXIT_FAILURE);
	}
	rc = saClmSelectionObjectGet(_ntfs_cb->clm_hdl, &ntfs_cb->clmSelectionObject);
	if (rc != SA_AIS_OK) {
		LOG_ER("saClmSelectionObjectGet failed with error: %d", rc);
		TRACE_LEAVE();
                exit(EXIT_FAILURE);
	} 
	//TODO:subscribe for SA_TRACK_START_STEP also.
	rc = saClmClusterTrack_4(_ntfs_cb->clm_hdl, (SA_TRACK_CURRENT | SA_TRACK_CHANGES), NULL);
	if (rc != SA_AIS_OK) {
		LOG_ER("saClmClusterTrack failed with error: %d", rc);
		TRACE_LEAVE();
                exit(EXIT_FAILURE);
	}
	TRACE("CLM Initialization SUCCESS......");
	TRACE_LEAVE();
	return NULL;
}

/*
 * @brief  Creates a thread to initialize with CLM. 
*/
void init_with_clm(void)
{
	pthread_t thread;
	pthread_attr_t attr;
	TRACE_ENTER();

	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

	if (pthread_create(&thread, &attr, ntfs_clm_init_thread, ntfs_cb) != 0) {
		LOG_ER("pthread_create FAILED: %s", strerror(errno));
		exit(EXIT_FAILURE);
	}
	pthread_attr_destroy(&attr);
	TRACE_LEAVE();
}
