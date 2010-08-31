/*      -*- OpenSAF  -*-
 *
 * (C) Copyright 2010 The OpenSAF Foundation
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

#include "clms.h"

static const SaNameT _clmSvcUsrName = {
	.value = "safApp=safClmService",
	.length = sizeof("safApp=safClmService")
};

const SaNameT *clmSvcUsrName = &_clmSvcUsrName;

/**
* Retrieve node from node db by nodename
* @param[in] Node_name
* @return node
*/
CLMS_CLUSTER_NODE *clms_node_get_by_name(const SaNameT *name)
{
	CLMS_CLUSTER_NODE *clms_node = NULL;

	TRACE_ENTER2("name input %s length %d", name->value, name->length);

	clms_node = (CLMS_CLUSTER_NODE *) ncs_patricia_tree_get(&clms_cb->nodes_db, (uns8 *)name);
	if (clms_node != NULL) {
		/* Adjust the pointer */
		clms_node = (CLMS_CLUSTER_NODE *) (((char *)clms_node)
						   - (((char *)&(((CLMS_CLUSTER_NODE *) 0)->pat_node_name))
						      - ((char *)((CLMS_CLUSTER_NODE *) 0))));
		TRACE("nodename after patricia tree get %s", clms_node->node_name.value);
	}

	TRACE_LEAVE();
	return clms_node;
}

/**
* Retreive next node from nodedb by node_name
* @param[in] node_name
* @return node
**/
CLMS_CLUSTER_NODE *clms_node_getnext_by_name(const SaNameT *name)
{
	CLMS_CLUSTER_NODE *clms_node = NULL;

	TRACE_ENTER();

	if (name->length == 0) {
		clms_node = (CLMS_CLUSTER_NODE *) ncs_patricia_tree_getnext(&clms_cb->nodes_db, (uns8 *)0);
	} else
		clms_node = (CLMS_CLUSTER_NODE *) ncs_patricia_tree_getnext(&clms_cb->nodes_db, (uns8 *)name);
	if (clms_node != NULL) {
		/* Adjust the pointer */
		clms_node = (CLMS_CLUSTER_NODE *) (((char *)clms_node)
						   - (((char *)&(((CLMS_CLUSTER_NODE *) 0)->pat_node_name))
						      - ((char *)((CLMS_CLUSTER_NODE *) 0))));
		TRACE("nodename after patricia tree get %s", clms_node->node_name.value);
	}

	TRACE_LEAVE();
	return clms_node;
}

/**
* Retrieve node from node db by eename
* @param[in] ee_name
* @return node
*/
CLMS_CLUSTER_NODE *clms_node_get_by_eename(SaNameT *name)
{
	CLMS_CLUSTER_NODE *clms_node = NULL;

	TRACE_ENTER2("name->value %s,name->length %d", name->value, name->length);

	clms_node = (CLMS_CLUSTER_NODE *) ncs_patricia_tree_get(&clms_cb->ee_lookup, (uns8 *)name);

	if (clms_node != NULL) {
		/* Adjust the pointer */
		clms_node = (CLMS_CLUSTER_NODE *) (((char *)clms_node)
						   - (((char *)&(((CLMS_CLUSTER_NODE *) 0)->pat_node_eename))
						      - ((char *)((CLMS_CLUSTER_NODE *) 0))));

		TRACE("nodename after patricia tree get %s", clms_node->ee_name.value);
	}

	TRACE_LEAVE();
	return clms_node;
}

/**
* Retrieve node from node db by nodeid
* @param[in] nodeid
* @return node
*/
CLMS_CLUSTER_NODE *clms_node_get_by_id(SaUint32T nodeid)
{
	CLMS_CLUSTER_NODE *node = NULL;
	TRACE_ENTER();

	node = (CLMS_CLUSTER_NODE *) ncs_patricia_tree_get(&clms_cb->id_lookup, (uns8 *)&nodeid);
	if (node != (CLMS_CLUSTER_NODE *) NULL) {
		/* Adjust the pointer */
		node = (CLMS_CLUSTER_NODE *) (((char *)node)
					      - (((char *)&(((CLMS_CLUSTER_NODE *) 0)->pat_node_id))
						 - ((char *)((CLMS_CLUSTER_NODE *) 0))));
		TRACE("Node found %d", node->node_id);
	}

	TRACE_LEAVE();
	return node;
}

/**
* Retrieve next node from node db by nodeid
* @param[in] nodeis
* @return node
*/
CLMS_CLUSTER_NODE *clms_node_getnext_by_id(SaUint32T node_id)
{
	CLMS_CLUSTER_NODE *node = NULL;

	if (node_id == 0) {

		node = (CLMS_CLUSTER_NODE *) ncs_patricia_tree_getnext(&clms_cb->id_lookup, (uns8 *)0);
	} else
		node = (CLMS_CLUSTER_NODE *) ncs_patricia_tree_getnext(&clms_cb->id_lookup, (uns8 *)&node_id);

	if (node != (CLMS_CLUSTER_NODE *) NULL) {
		/* Adjust the pointer */
		node = (CLMS_CLUSTER_NODE *) (((char *)node)
					      - (((char *)&(((CLMS_CLUSTER_NODE *) 0)->pat_node_id))
						 - ((char *)((CLMS_CLUSTER_NODE *) 0))));
		TRACE("Node found %d", node->node_id);
	}

	return node;
}

/**
* Adds the node to patricia tree 
* @param[in] node node to add to patricia tree
* @param[in] i    Integer that specifies which tree to add
* @return NCSCC_RC_SUCESS/NCSCC_RC_FAILURE
*/
uns32 clms_node_add(CLMS_CLUSTER_NODE * node, int i)
{
	uns32 rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER();
	TRACE("value of i %d", i);
	switch (i) {

	case 0:
		TRACE("Adding node_id to the patricia tree with node_id %u as key", node->node_id);
		node->pat_node_id.key_info = (uns8 *)&(node->node_id);
		rc = ncs_patricia_tree_add(&clms_cb->id_lookup, &node->pat_node_id);
		if (rc != NCSCC_RC_SUCCESS) {
			LOG_WA("ncs_patricia_tree_add for node_id  FAILED for '%d' %u", node->node_id, rc);
			node->pat_node_id.key_info = NULL;
			goto done;
		}
		break;
	case 1:
		TRACE("Adding eename to the patricia tree");
		node->pat_node_eename.key_info = (uns8 *)&(node->ee_name);
		rc = ncs_patricia_tree_add(&clms_cb->ee_lookup, &node->pat_node_eename);
		if (rc != NCSCC_RC_SUCCESS) {
			LOG_WA("ncs_patricia_tree_add for eename FAILED for eename = %s as key,rc =  %u",
			       node->ee_name.value, rc);
			node->pat_node_eename.key_info = NULL;
			goto done;
		}
		break;
	case 2:
		TRACE("Adding nodename to the patricia tree");
		node->pat_node_name.key_info = (uns8 *)&(node->node_name);
		rc = ncs_patricia_tree_add(&clms_cb->nodes_db, &node->pat_node_name);
		if (rc != NCSCC_RC_SUCCESS) {
			LOG_WA("ncs_patricia_tree_add for nodename FAILED for nodename %s as key, rc = %u",
			       node->node_name.value, rc);
			node->pat_node_name.key_info = NULL;
			goto done;
		}
		break;

	default:
		TRACE("Invalid Patricia add");
		rc = NCSCC_RC_FAILURE;
		goto done;
	}

 done:
	TRACE_LEAVE();
	return rc;
}

/**
* Delete the node from the nodedb
* @param[in] node to delete
* @param[in] i 0 ==> delete fron id_lookup tree
* 	       1 ==> delete from ee_lookup tree
* 	       2 ==> delete from node_db tree
* @return NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
*/
uns32 clms_node_delete(CLMS_CLUSTER_NODE * nd, int i)
{
	uns32 rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER2("value of i %d", i);

	assert(nd != NULL);

	switch (i) {

	case 0:
		if (nd->node_id != 0){
			if ((rc = ncs_patricia_tree_del(&clms_cb->id_lookup, &nd->pat_node_id)) != NCSCC_RC_SUCCESS) {
				LOG_WA("ncs_patricia_tree_del FAILED for nodename %s rc %u", nd->node_name.value, rc);
				goto done;
			}	
		}
		break;
	case 1:

		if (clms_cb->reg_with_plm == SA_TRUE) {
			if ((rc = ncs_patricia_tree_del(&clms_cb->ee_lookup, &nd->pat_node_eename)) != NCSCC_RC_SUCCESS) {
				LOG_WA("ncs_patricia_tree_del FAILED for nodename %s rc %u", nd->node_name.value, rc);
				goto done;
			}
		}
		break;
	case 2:
		if ((rc = ncs_patricia_tree_del(&clms_cb->nodes_db, &nd->pat_node_name)) != NCSCC_RC_SUCCESS) {
			LOG_WA("ncs_patricia_tree_del FAILED for nodename %s rc %u", nd->node_name.value, rc);
			goto done;
		}
		break;
	default:
		assert(0);
	}

 done:
	TRACE_LEAVE();
	return rc;
}

/**
* Get the time
*/
SaTimeT clms_get_SaTime(void)
{
	return time(NULL) * SA_TIME_ONE_SECOND;
}

/**
* Validate the saClmNode DN
* @param[in] DN 
* @return NCSCC_RC_SUCESS/NCSCC_RC_FAILURE
*/
uns32 clms_node_dn_chk(SaNameT *objName)
{
	char *tmpstr;
	TRACE_ENTER();

	if (!strncmp((char *)objName->value, "safNode=", 8)) {
		tmpstr = strchr((char *)objName->value, ',');
		if (tmpstr != NULL)
			if (!strncmp(++tmpstr, "safCluster=", 11)) {
				if (!strncmp(tmpstr, (char *)osaf_cluster->name.value, osaf_cluster->name.length)) {
					TRACE("Node is child of our cluster %s", osaf_cluster->name.value);
					TRACE_LEAVE();
					return NCSCC_RC_SUCCESS;
				} else {
					TRACE("Node is not the child of our cluster %s", osaf_cluster->name.value);
					TRACE_LEAVE();
					return NCSCC_RC_FAILURE;
				}
			}
	}
	TRACE_LEAVE();
	return NCSCC_RC_FAILURE;
}

/**
* Validate the saClmCluster DN
* @param[in] DN 
* @return NCSCC_RC_SUCESS/NCSCC_RC_FAILURE
*/
uns32 clms_cluster_dn_chk(SaNameT *objName)
{
	TRACE_ENTER();

	if (!strncmp((char *)objName->value, "safCluster=", 11)) {
		TRACE("DN name is correct");
		TRACE_LEAVE();
		return NCSCC_RC_SUCCESS;
	}
	TRACE("Dn name is incorrect");
	TRACE_LEAVE();
	return NCSCC_RC_FAILURE;

}

/**
* Count the number of nodes whose  cluster membershipship has changed
* @param[in] i 
* @return uns32
*/
/* This might not be needed ,crosscheck*/
uns32 clms_nodedb_lookup(int i)
{
	CLMS_CLUSTER_NODE *node = NULL;
	SaUint32T nodeid = 0;
	uns32 num_nd_changes = 0;
	TRACE_ENTER();

	TRACE("patricia tree size %d", ncs_patricia_tree_size(&clms_cb->id_lookup));

	while (((CLMS_CLUSTER_NODE *) NULL) != (node = clms_node_getnext_by_id(nodeid))) {

		nodeid = node->node_id;

		/*TBD : pass enum value */

		if (i == 0) {
			if (node->stat_change == SA_TRUE)
				num_nd_changes++;
		} else {
			/* SA_TRACK_CHANGES requires to send callback for all the member nodes 
			   that is the current cluster member as well as the ones in the process
			   of leaving the cluster membership */

			if ((node->stat_change == SA_TRUE) || (node->member == SA_TRUE))
				num_nd_changes++;
		}
	}

	TRACE("num_nd_changes %d", num_nd_changes);
	TRACE_LEAVE();
	return num_nd_changes;
}

/**
* Fill the Notification buffer for SA_TRACK_CHANGES_ONLY trackflag
* @param[in] ClmChangestep 
* @return filled notification buffer
*/
SaClmClusterNotificationT_4 *clms_notbuffer_changes_only(SaClmChangeStepT step)
{
	SaClmClusterNotificationT_4 *notify = NULL;
	CLMS_CLUSTER_NODE *node = NULL;
	uns32 num = 0, i = 0;
	SaUint32T nodeid = 0;

	TRACE_ENTER();

	/* number of nodes for SA_TRACK_CHANGES_ONLY */
	num = clms_nodedb_lookup(0);

	if (num == 0) {
		LOG_ER("Zero num of node's changed");
		return NULL;
	}

	/* Allocate memory for the rec which got changed ONLY */
	notify = (SaClmClusterNotificationT_4 *) malloc(num * sizeof(SaClmClusterNotificationT_4));
	if (!notify) {
		LOG_ER("Malloc failed for SaClmClusterNotificationT_4");
		assert(0);
	}

	memset(notify, 0, num * sizeof(SaClmClusterNotificationT_4));

	/*Fill the notify buffer with the node info */
	while ((NULL != (node = clms_node_getnext_by_id(nodeid))) && (i < num)) {
		nodeid = node->node_id;
		if (node->stat_change == SA_TRUE) {

			notify[i].clusterNode.nodeId = node->node_id;
			notify[i].clusterNode.nodeAddress.family = node->node_addr.family;
			notify[i].clusterNode.nodeAddress.length = node->node_addr.length;
			memcpy(notify[i].clusterNode.nodeAddress.value, node->node_addr.value,
			       notify[i].clusterNode.nodeAddress.length);

			notify[i].clusterNode.nodeName.length = node->node_name.length;
			memcpy(notify[i].clusterNode.nodeName.value, node->node_name.value,
			       notify[i].clusterNode.nodeName.length);

			notify[i].clusterNode.executionEnvironment.length = node->ee_name.length;
			memcpy(notify[i].clusterNode.executionEnvironment.value, node->ee_name.value,
			       notify[i].clusterNode.executionEnvironment.length);

			/* If a node leaves the cluster, member field should be false */
			if ((node->change == SA_CLM_NODE_LEFT) && (step == SA_CLM_CHANGE_COMPLETED))
				notify[i].clusterNode.member = SA_FALSE;
			else
				notify[i].clusterNode.member = node->member;

			notify[i].clusterNode.bootTimestamp = node->boot_time;
			notify[i].clusterNode.initialViewNumber = node->init_view;

			notify[i].clusterChange = node->change;
			i++;
		}
	}
	TRACE_LEAVE();
	return notify;
}

/**
* Fill the Notification buffer for SA_TRACK_CHANGES trackflag
* @param[in] ClmChangestep 
* @return filled notification buffer
*/
SaClmClusterNotificationT_4 *clms_notbuffer_changes(SaClmChangeStepT step)
{

	SaClmClusterNotificationT_4 *notify = NULL;
	CLMS_CLUSTER_NODE *node = NULL;
	SaUint32T nodeid = 0;
	uns32 num = 0, i = 0;
	TRACE_ENTER();

	/* number of nodes for SA_TRACK_CHANGES */
	num = clms_nodedb_lookup(1);

	if (num == 0) {
		return NULL;
	}

	/* alloc the notify buffer */
	notify = (SaClmClusterNotificationT_4 *) malloc(num * sizeof(SaClmClusterNotificationT_4));

	if (!notify) {
		LOG_ER("malloc failed for SaClmClusterNotificationT_4");
		assert(0);
	}

	memset(notify, 0, num * sizeof(SaClmClusterNotificationT_4));

	/*Fill the notify buffer with the node info */
	while ((NULL != (node = clms_node_getnext_by_id(nodeid))) && (i < num)) {
		nodeid = node->node_id;
		if ((node->stat_change == SA_TRUE) || (node->member == SA_TRUE)) {

			notify[i].clusterNode.nodeId = node->node_id;
			notify[i].clusterNode.nodeAddress.family = node->node_addr.family;
			notify[i].clusterNode.nodeAddress.length = node->node_addr.length;
			memcpy(notify[i].clusterNode.nodeAddress.value, node->node_addr.value,
			       notify[i].clusterNode.nodeAddress.length);

			notify[i].clusterNode.nodeName.length = node->node_name.length;
			memcpy(notify[i].clusterNode.nodeName.value, node->node_name.value,
			       notify[i].clusterNode.nodeName.length);

			notify[i].clusterNode.executionEnvironment.length = node->ee_name.length;
			memcpy(notify[i].clusterNode.executionEnvironment.value, node->ee_name.value,
			       notify[i].clusterNode.executionEnvironment.length);

			/* If a node leaves the cluster, member field should be false */
			if ((node->change == SA_CLM_NODE_LEFT) && (step == SA_CLM_CHANGE_COMPLETED))
				notify[i].clusterNode.member = SA_FALSE;
			else
				notify[i].clusterNode.member = node->member;

			notify[i].clusterNode.bootTimestamp = node->boot_time;
			notify[i].clusterNode.initialViewNumber = node->init_view;

			notify[i].clusterChange = node->change;
			i++;
		}
	}

	TRACE_LEAVE();
	return notify;
}

/**
* Delete client from the track resonse list
* @param[in] client_id
* @return NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
*/
uns32 clms_client_del_trackresp(SaUint32T client_id)
{
	CLMS_CLUSTER_NODE *node = NULL;
	SaUint32T nodeid = 0;
	CLMS_TRACK_INFO *trkrsp_rec = NULL;
	SaInvocationT inv_id = 0;
	uns32 rc = NCSCC_RC_SUCCESS;

	while (NULL != (node = clms_node_getnext_by_id(nodeid))) {
		nodeid = node->node_id;
		if (ncs_patricia_tree_size(&node->trackresp) != 0) {
			trkrsp_rec = (CLMS_TRACK_INFO *) ncs_patricia_tree_getnext(&node->trackresp, (uns8 *)0);
			while (trkrsp_rec != NULL) {
				inv_id = trkrsp_rec->inv_id;
				if (trkrsp_rec->client_id == client_id) {
					if(ncs_patricia_tree_size(&node->trackresp) == 1) {
						/*We need to send callback for completed step if all client 
						  has send response for start step but the client which has 
						  gone down or did finalize has not send response*/
						rc = clms_clmresp_ok(clms_cb, node, trkrsp_rec);
						if(rc != NCSCC_RC_SUCCESS) {
							TRACE("clms_clmresp_ok FAILED");
							goto done;
						}
					} else {
						if (NCSCC_RC_SUCCESS !=
								ncs_patricia_tree_del(&node->trackresp, &trkrsp_rec->pat_node)) {
							TRACE("ncs_patricia_tree_del FAILED");
							rc = NCSCC_RC_FAILURE;
							goto done;
						}
						free(trkrsp_rec); 
					}
					break;
				}
				trkrsp_rec =
					(CLMS_TRACK_INFO *) ncs_patricia_tree_getnext(&node->trackresp, (uns8 *)&inv_id);
			}
		}
	}
done:
	return rc;
}

/**
* Empty the track response list if not null
* @param[in] op_node node on which the trackresponse list exists
* @return NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
*/
uns32 clms_node_trackresplist_empty(CLMS_CLUSTER_NODE * op_node)
{
	CLMS_TRACK_INFO *trkrec = NULL;
	SaInvocationT inv_id = 0;
	uns32 rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER();

	trkrec = (CLMS_TRACK_INFO *) ncs_patricia_tree_getnext(&op_node->trackresp, (uns8 *)0);
	while (trkrec != NULL) {
		inv_id = trkrec->inv_id;
		if (NCSCC_RC_SUCCESS != ncs_patricia_tree_del(&op_node->trackresp, &trkrec->pat_node)) {
			TRACE("ncs_patricia_tree_del FAILED");
			rc = NCSCC_RC_FAILURE;
			goto done;
		}
		free(trkrec);
		trkrec = (CLMS_TRACK_INFO *) ncs_patricia_tree_getnext(&op_node->trackresp, (uns8 *)&inv_id);
	}
 done:
	TRACE_LEAVE();
	return rc;
}

/**
* Clear the node dependency list,made for multiple nodes in the plm callback
*/
void clms_clear_node_dep_list(CLMS_CLUSTER_NODE * node)
{
	CLMS_CLUSTER_NODE *new_node = NULL;

	node->admin_op = 0;
	node->stat_change = SA_FALSE;
	ckpt_node_rec(node);
	while (node->dep_node_list != NULL) {
		new_node = node->dep_node_list;
		new_node->stat_change = SA_FALSE;
		new_node->admin_op = 0;
		ckpt_node_rec(new_node);
		node->dep_node_list = node->dep_node_list->next;
		new_node->next = NULL;
	}
}

/**
* This sends the callback to its clients when the error is received in the ClmResponse flow
* and responds to immsv with err repair pending
* @param[in]  node  node for which the clmresp was received
* @param[in]  cb    CLMS CB 
*/
void clms_clmresp_error_timeout(CLMS_CB * cb, CLMS_CLUSTER_NODE * node)
{
	node->admin_state = SA_CLM_ADMIN_LOCKED;
	if (node->member == SA_TRUE){
		--(osaf_cluster->num_nodes);
	}
	node->member = SA_FALSE;
	if (node->admin_op == IMM_LOCK) {
		node->change = SA_CLM_NODE_LEFT;
	} else if (node->admin_op == IMM_SHUTDOWN)
		node->change = SA_CLM_NODE_SHUTDOWN;

	++(cb->cluster_view_num);

	/*Update IMMSV before returning with ERR_PENDING */
	clms_node_update_rattr(node);
	clms_admin_state_update_rattr(node);
	clms_cluster_update_rattr(osaf_cluster);

	clms_send_track(clms_cb, node, SA_CLM_CHANGE_COMPLETED);

	node->stat_change = SA_FALSE;
	node->admin_op = 0;
	/* Checkpoint the data */
	ckpt_node_rec(node);
	ckpt_cluster_rec();
	(void)immutil_saImmOiAdminOperationResult(cb->immOiHandle, node->curr_admin_inv, SA_AIS_ERR_REPAIR_PENDING);
}

/**
* This is for rejected  received in the ClmResponse flow
* @param[in]  node  node for which the clmresp was received
* @param[in]  cb    CLMS CB 
*/
uns32 clms_clmresp_rejected(CLMS_CB * cb, CLMS_CLUSTER_NODE * node, CLMS_TRACK_INFO * trk)
{
	uns32 rc = NCSCC_RC_SUCCESS;

	TRACE_ENTER();

	rc = clms_node_trackresplist_empty(node);
	if (rc != NCSCC_RC_SUCCESS)
		goto done;

	switch (node->admin_op) {

	case PLM:
		{
#ifdef ENABLE_AIS_PLM
			CLMS_CLIENT_INFO *client = NULL;
			SaAisErrorT ais_er;

			clms_clear_node_dep_list(node);
			client = clms_client_get_by_id(trk->client_id);
			if (client != NULL){
				if (client->track_flags & SA_TRACK_VALIDATE_STEP) {
					ais_er =
						saPlmReadinessTrackResponse(cb->ent_group_hdl, node->plm_invid,
								SA_CLM_CALLBACK_RESPONSE_REJECTED);
					if (ais_er != SA_AIS_OK) {
						LOG_ER("saPlmReadinessTrackResponse FAILED ais_er = %u", ais_er);
						goto done;
					}
				} 
			} else {
				ais_er =
					saPlmReadinessTrackResponse(cb->ent_group_hdl, node->plm_invid,
							SA_CLM_CALLBACK_RESPONSE_ERROR);
				if (ais_er != SA_AIS_OK) {
					LOG_ER("saPlmReadinessTrackResponse FAILED ais_er = %u", ais_er);
					goto done;
				}
			}
			
#endif
			break;
		}
	case IMM_LOCK:
		{
			/*Delete the timer */
			if (timer_delete(node->lock_timerid) != 0) {
				LOG_ER("Timer Delete failed");
				assert(0);
			}
			node->admin_op = 0;
			node->stat_change = SA_FALSE;

			/*Checkpoint the node data */
			ckpt_node_rec(node);
			TRACE("CLM Client rejected the node lock operation");
			(void)immutil_saImmOiAdminOperationResult(cb->immOiHandle, node->curr_admin_inv,
								  SA_AIS_ERR_FAILED_OPERATION);
			break;
		}
	case IMM_SHUTDOWN:
		{
			node->admin_op = 0;
			node->stat_change = SA_FALSE;
			(void)immutil_saImmOiAdminOperationResult(cb->immOiHandle, node->curr_admin_inv,
								  SA_AIS_ERR_FAILED_OPERATION);
			break;
		}
	default:
		{
			TRACE("Invalid admin_op set");
			break;
		}
	}

 done:
	TRACE_LEAVE();
	return rc;

}

/**
* This is for clmresponse error flow. 
* @param[in]  node  node for which the clmresp was received
* @param[in]  cb    CLMS CB 
*/
uns32 clms_clmresp_error(CLMS_CB * cb, CLMS_CLUSTER_NODE * node)
{
	uns32 rc = NCSCC_RC_SUCCESS;

	TRACE_ENTER();

	rc = clms_node_trackresplist_empty(node);
	if (rc != NCSCC_RC_SUCCESS)
		goto done;

	switch (node->admin_op) {
	case IMM_LOCK:
		{
			/*Delete the timer */
			if (timer_delete(node->lock_timerid) != 0) {
				LOG_ER("Timer Delete failed");
				assert(0);
			}
			clms_clmresp_error_timeout(cb, node);

			rc = clms_node_exit_ntf(cb, node);
			if (rc != NCSCC_RC_SUCCESS) {
				LOG_ER("clms_node_exit_ntf failed %u", rc);
			}
			rc = clms_node_admin_state_change_ntf(cb, node, SA_CLM_ADMIN_LOCKED);
			if (rc != NCSCC_RC_SUCCESS) {
				LOG_ER("clms_node_admin_state_change_ntf failed %d", rc);
			}

			/*you have to reboot the node in case of imm */
			if (clms_cb->reg_with_plm == SA_TRUE)
				clms_reboot_remote_node(node,"client responded with error during admin lock");
			break;
		}
	case IMM_SHUTDOWN:
		{
			clms_clmresp_error_timeout(cb, node);

			rc = clms_node_exit_ntf(cb, node);
			if (rc != NCSCC_RC_SUCCESS) {
				LOG_ER("clms_node_exit_ntf failed %u", rc);
			}
			rc = clms_node_admin_state_change_ntf(cb, node, SA_CLM_ADMIN_LOCKED);
			if (rc != NCSCC_RC_SUCCESS) {
				LOG_ER("clms_node_admin_state_change_ntf failed %d", rc);
			}

			/*you have to reboot the node in case of imm */
			if (clms_cb->reg_with_plm == SA_TRUE)
				clms_reboot_remote_node(node,"client responded with error during admin shutdown");
			break;
		}
	case PLM:
		{
#ifdef ENABLE_AIS_PLM
			SaAisErrorT ais_er = SA_AIS_OK;

			clms_clear_node_dep_list(node);
			ais_er =
			    saPlmReadinessTrackResponse(cb->ent_group_hdl, node->plm_invid,
							SA_CLM_CALLBACK_RESPONSE_ERROR);
			if (ais_er != SA_AIS_OK) {
				TRACE("saPlmReadinessTrackResponse FAILED");
				goto done;
			}
#endif
			break;
		}
	default:
		TRACE("Invalid admin_op set");
		break;

	}
 done:
	TRACE_LEAVE();
	return rc;

}

void * clms_rem_reboot(void * _reboot_info)
{
	TRACE_ENTER();
	CLMS_REM_REBOOT_INFO * rem_reb = (CLMS_REM_REBOOT_INFO *)_reboot_info;
	opensaf_reboot(rem_reb->node->node_id, (char *)rem_reb->node->ee_name.value,(char *)rem_reb->str);
	free(rem_reb->str);
	free(rem_reb);

	TRACE_LEAVE();
	return NULL;
}

void clms_reboot_remote_node(CLMS_CLUSTER_NODE * op_node, char *str)
{
	TRACE_ENTER();
	CLMS_REM_REBOOT_INFO * rem_reb = NULL;
	pthread_t thread;
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	rem_reb = malloc(sizeof(CLMS_REM_REBOOT_INFO));
	rem_reb->node = op_node;
	rem_reb->str = malloc(strlen(str));
	strcpy(rem_reb->str,str);

	if (pthread_create(&thread, &attr, clms_rem_reboot, (void *)rem_reb) != 0) {
		LOG_ER("pthread_create FAILED: %s", strerror(errno));
		TRACE_LEAVE();
		exit(EXIT_FAILURE);
	}
	pthread_attr_destroy(&attr);
	TRACE_LEAVE();
}

/**
* This is for clmresponse OK flow.Sends the callback to its client 
* specifying the admin op is co*mplete 
* @param[in]  node  node for which the clmresp was received
* @param[in]  cb    CLMS CB 
*/
uns32 clms_clmresp_ok(CLMS_CB * cb, CLMS_CLUSTER_NODE * op_node, CLMS_TRACK_INFO * trkrec)
{
	uns32 rc = NCSCC_RC_SUCCESS;

	TRACE_ENTER();

	if (trkrec != NULL) {
		if (NCSCC_RC_SUCCESS != ncs_patricia_tree_del(&op_node->trackresp, &trkrec->pat_node)) {
			TRACE("ncs_patricia_tree_del FAILED");
			rc = NCSCC_RC_FAILURE;
			goto done;
		}
		free(trkrec);
	}

	TRACE("op_node->admin_op %d", op_node->admin_op);

	if (op_node->admin_op == PLM) {

#ifdef ENABLE_AIS_PLM
		SaAisErrorT ais_er = SA_AIS_OK;

		if (ncs_patricia_tree_size(&op_node->trackresp) == 0) {
			/*Clear the node dependency list */
			clms_clear_node_dep_list(op_node);
			ais_er = saPlmReadinessTrackResponse(cb->ent_group_hdl, op_node->plm_invid, SA_PLM_CALLBACK_RESPONSE_OK);
			if (ais_er != SA_AIS_OK) {
				TRACE("saPlmReadinessTrackResponse FAILED");
				goto done;
			}
			TRACE("PLM Track Response Send Succedeed");

			/*Update the runtime change to IMMSv */
			clms_node_update_rattr(op_node);
			clms_admin_state_update_rattr(op_node);
			clms_cluster_update_rattr(osaf_cluster);

			/*Checkpoint node data */
			ckpt_node_rec(op_node);

			/*Checkpoint Cluster data */
			ckpt_cluster_rec();
		}
#endif

	} else {
		if (ncs_patricia_tree_size(&op_node->trackresp) == 0) {

			if (op_node->admin_op == IMM_LOCK) {
				op_node->change = SA_CLM_NODE_LEFT;
				timer_delete(op_node->lock_timerid);
			} else if (op_node->admin_op == IMM_SHUTDOWN)
				op_node->change = SA_CLM_NODE_SHUTDOWN;

			if (op_node->member == SA_TRUE){
				--(osaf_cluster->num_nodes);
			}
			++(cb->cluster_view_num);
			op_node->admin_state = SA_CLM_ADMIN_LOCKED;
			op_node->member = SA_FALSE;

			/*Update the runtime change to IMMSv */
			/*Updating immsv before sending completed callback to clm agents*/
			clms_node_update_rattr(op_node);
			clms_admin_state_update_rattr(op_node);
			clms_cluster_update_rattr(osaf_cluster);

			/* Send track callback to all start client */
			clms_send_cbk_start_sub(cb, op_node);

			rc = clms_node_exit_ntf(cb, op_node);
			if (rc != NCSCC_RC_SUCCESS) {
				TRACE("clms_node_exit_ntf failed %u", rc);
				goto done;
			}
			rc = clms_node_admin_state_change_ntf(cb, op_node, SA_CLM_ADMIN_LOCKED);
			if (rc != NCSCC_RC_SUCCESS) {
				TRACE("clms_node_admin_state_change_ntf failed %d", rc);
				goto done;
			}
			op_node->admin_op = 0;
			op_node->stat_change = SA_FALSE;
			(void)immutil_saImmOiAdminOperationResult(cb->immOiHandle, op_node->curr_admin_inv, SA_AIS_OK);

			rc = clms_send_is_member_info(clms_cb, op_node->node_id, op_node->member, TRUE);
			if (rc != NCSCC_RC_SUCCESS) {
				TRACE("clms_send_is_member_info failed %u", rc);
				goto done;
			}

			/*Checkpoint node data */
			ckpt_node_rec(op_node);

			/*Checkpoint Cluster data */
			ckpt_cluster_rec();			
			if(op_node->disable_reboot == SA_FALSE) {
				if (clms_cb->reg_with_plm == SA_TRUE){
					clms_reboot_remote_node(op_node,"Clm lock:start subscriber responds ok and  disable reboot set to false");
				} /* Without PLM in system,till now there is no mechanism to reboot remote node*/
			}
		}
	}
done:
	TRACE_LEAVE();
	return rc;

}

/**
* This updates its peer with the node's  data which went down
* @param[in] node node on which the runtime data got changed
*/
void ckpt_node_down_rec(CLMS_CLUSTER_NODE * node)
{
	CLMS_CKPT_REC ckpt;
	uns32 async_rc;
	TRACE_ENTER();

	if (clms_cb->ha_state == SA_AMF_HA_ACTIVE) {
		memset(&ckpt, 0, sizeof(CLMS_CKPT_REC));
		ckpt.header.type = CLMS_CKPT_NODE_DOWN_REC;
		ckpt.header.num_ckpt_records = 1;
		ckpt.header.data_len = 1;
		ckpt.param.node_down_rec.node_id = node->node_id;
		async_rc = clms_send_async_update(clms_cb, &ckpt, NCS_MBCSV_ACT_ADD);
		if (async_rc != NCSCC_RC_SUCCESS)
			TRACE("send_async_update FAILED");

	}
	TRACE_LEAVE();
}

/**
* This updates its peer with the node's runtime data
* @param[in] node node on which the runtime data got changed
*/
void ckpt_node_rec(CLMS_CLUSTER_NODE * node)
{
	CLMS_CKPT_REC ckpt;
	uns32 async_rc;
	TRACE_ENTER();

	if (clms_cb->ha_state == SA_AMF_HA_ACTIVE) {
		memset(&ckpt, 0, sizeof(CLMS_CKPT_REC));
		ckpt.header.type = CLMS_CKPT_NODE_RUNTIME_REC;
		ckpt.header.num_ckpt_records = 1;
		ckpt.header.data_len = 1;
		ckpt.param.node_rec.node_id = node->node_id;
		ckpt.param.node_rec.node_name.length = node->node_name.length;
		(void)memcpy(ckpt.param.node_rec.node_name.value, node->node_name.value, node->node_name.length);
		ckpt.param.node_rec.member = node->member;
		ckpt.param.node_rec.boot_time = node->boot_time;	/*may be not needed */
		ckpt.param.node_rec.init_view = node->init_view;
		ckpt.param.node_rec.admin_state = node->admin_state;
		ckpt.param.node_rec.admin_op = node->admin_op;
		ckpt.param.node_rec.change = node->change;
		ckpt.param.node_rec.nodeup = node->nodeup;
#ifdef ENABLE_AIS_PLM
		ckpt.param.node_rec.ee_red_state = node->ee_red_state;
#endif
		
		async_rc = clms_send_async_update(clms_cb, &ckpt, NCS_MBCSV_ACT_ADD);
		if (async_rc != NCSCC_RC_SUCCESS)
			TRACE("send_async_update FAILED");

	}
	TRACE_LEAVE();
}

/**
* This updates its peer with runtime cluster changes
*/
void ckpt_cluster_rec(void)
{
	CLMS_CKPT_REC ckpt;
	uns32 async_rc;
	TRACE_ENTER();

	if (clms_cb->ha_state == SA_AMF_HA_ACTIVE) {
		memset(&ckpt, 0, sizeof(CLMS_CKPT_REC));
		ckpt.header.type = CLMS_CKPT_CLUSTER_REC;
		ckpt.header.num_ckpt_records = 1;
		ckpt.header.data_len = 1;
		ckpt.param.cluster_rec.num_nodes = osaf_cluster->num_nodes;
		ckpt.param.cluster_rec.init_time = osaf_cluster->init_time;
		ckpt.param.cluster_rec.cluster_view_num = clms_cb->cluster_view_num;

		async_rc = clms_send_async_update(clms_cb, &ckpt, NCS_MBCSV_ACT_ADD);
		if (async_rc != NCSCC_RC_SUCCESS)
			TRACE("send_async_update FAILED");

	}
	TRACE_LEAVE();
}

/**
 * Walk through the clma list to find a match. 
 */
NCS_BOOL clms_clma_entry_valid(CLMS_CB * cb, MDS_DEST mds_dest)
{
	CLMS_CLIENT_INFO *client = NULL;

	client = clms_client_getnext_by_id(0);

	while (client != NULL) {
		if (m_NCS_MDS_DEST_EQUAL(&client->mds_dest, &mds_dest)) {
			return TRUE;
		}

		client = clms_client_getnext_by_id(client->client_id);
	}

	return FALSE;
}

/**
* If Switchover happens in middle of the IMM admin operation
* Send abort to all the subscribed client
**/
void clms_adminop_pending(void)
{
	CLMS_CLUSTER_NODE *node = NULL;
	SaUint32T nodeid = 0;

	/*Crosscheck: Break after getting the node and send trackcallback out of the loop */
	while (NULL != (node = clms_node_getnext_by_id(nodeid))) {
		nodeid = node->node_id;
		if ((node->admin_op != PLM) && (node->admin_op != 0)) {
			clms_send_track(clms_cb, node, SA_CLM_CHANGE_ABORTED);
			node->admin_op = 0;
		}
	}

}

uns32 clms_send_cbk_start_sub(CLMS_CB * cb, CLMS_CLUSTER_NODE * node)
{
	CLMS_CLIENT_INFO *rec = NULL;
	SaClmClusterNotificationT_4 *notify_changes = NULL;
	SaClmClusterNotificationT_4 *notify_changes_only = NULL;
	uns32 rc = NCSCC_RC_SUCCESS;
	uns32 client_id = 0;
	SaClmChangeStepT step = SA_CLM_CHANGE_COMPLETED;
	SaUint32T node_id;


	TRACE_ENTER();

	if (node->change == SA_CLM_NODE_LEFT)
		LOG_NO("%s LEFT, view number=%llu", node->node_name.value, node->init_view);
	else if (node->change == SA_CLM_NODE_SHUTDOWN)
		LOG_NO("%s SHUTDOWN, view number=%llu", node->node_name.value, node->init_view);

	notify_changes_only = clms_notbuffer_changes_only(step);
	notify_changes = clms_notbuffer_changes(step);

	while (NULL != (rec = clms_client_getnext_by_id(client_id))) {
		client_id = rec->client_id;
		node_id = m_NCS_NODE_ID_FROM_MDS_DEST(rec->mds_dest);
		TRACE("Client ID %d ,track_flags=%d", rec->client_id, rec->track_flags);

		if (rec->track_flags) {
			rec->inv_id = 0;

			if (rec->track_flags & SA_TRACK_CHANGES_ONLY) {
				if(rec->track_flags & SA_TRACK_LOCAL){
					if(node_id == node->node_id){
						/*Implies the change is on this local node */
						rc = clms_send_track_local(node,rec,SA_CLM_CHANGE_COMPLETED);
					}
				}else {
					if (notify_changes_only != NULL) {
						rc = clms_prep_and_send_track(cb, node, rec, step, notify_changes_only);
					} else {
						LOG_ER
							("Inconsistent node db,Unable to send track callback for SA_TRACK_CHANGES_ONLY clients");
					}
				}
			} else if (rec->track_flags & SA_TRACK_CHANGES) {
				if(rec->track_flags & SA_TRACK_LOCAL){
					if(node_id == node->node_id){
						/*Implies the change is on this local node */
						rc = clms_send_track_local(node,rec,SA_CLM_CHANGE_COMPLETED);
					}
				} else {
					if (notify_changes != NULL) {
						rc = clms_prep_and_send_track(cb, node, rec, step, notify_changes);
					} else {
						LOG_ER
							("Inconsistent node db,Unable to send track callback for SA_TRACK_CHANGES clients");
					}
				}
			}

			if (rc != NCSCC_RC_SUCCESS) {
				LOG_ER("Sending track callback failed");
			}

		}
	}

	free(notify_changes_only);
	free(notify_changes);
	TRACE_LEAVE();
	return rc;
}

/*walk though client list, from client mds_dest extract node_id,
if node is match send mds msg to that client*/

uns32 clms_send_is_member_info(CLMS_CB * cb, SaClmNodeIdT node_id,  SaBoolT member, SaBoolT is_configured)
{
	CLMS_CLIENT_INFO *client = NULL;
	CLMSV_MSG msg;
	SaClmNodeIdT local_node_id;
	uns32 rc = NCSCC_RC_SUCCESS;
	
	TRACE_ENTER();
	
	client = clms_client_getnext_by_id(0);
	while(client != NULL) {

		local_node_id = m_NCS_NODE_ID_FROM_MDS_DEST(client->mds_dest);

		if(local_node_id == node_id) {
			msg.evt_type = CLMSV_CLMS_TO_CLMA_IS_MEMBER_MSG; 
			msg.info.is_member_info.is_member = member;
			msg.info.is_member_info.is_configured = is_configured;
			msg.info.is_member_info.client_id = client->client_id;
			rc = clms_mds_msg_send(cb, &msg, &client->mds_dest, 0,
						MDS_SEND_PRIORITY_MEDIUM, NCSMDS_SVC_ID_CLMA);
			if (rc != NCSCC_RC_SUCCESS) {
				TRACE_LEAVE2("clms_mds_msg_send failed rc = %u", (unsigned int)rc);
				return rc;
			}
		}

		client = clms_client_getnext_by_id(client->client_id);
	}

	TRACE_LEAVE();
	return rc;
}

/*Walk through the node db to know the number of member nodes*/
uns32 clms_num_mem_node(void)
{
	SaUint32T node_id = 0;
	SaUint32T i = 0;
	CLMS_CLUSTER_NODE *rp = NULL;
	
	while ((rp = clms_node_getnext_by_id(node_id)) != NULL) {

		node_id = rp->node_id;
		if (rp->member == SA_TRUE) {
			i++;
		}
	}

	return i;
}
