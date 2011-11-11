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

#include <configmake.h>

#include "clms.h"

static uns32 process_api_evt(CLMSV_CLMS_EVT * evt);
static uns32 proc_clma_updn_mds_msg(CLMSV_CLMS_EVT * evt);
static uns32 proc_mds_node_evt(CLMSV_CLMS_EVT * evt);
static uns32 proc_rda_evt(CLMSV_CLMS_EVT * evt);
static uns32 proc_mds_quiesced_ack_msg(CLMSV_CLMS_EVT * evt);
static uns32 proc_node_lock_tmr_exp_msg(CLMSV_CLMS_EVT * evt);
static uns32 proc_node_up_msg(CLMS_CB * cb, CLMSV_CLMS_EVT * evt);
static uns32 proc_initialize_msg(CLMS_CB * cb, CLMSV_CLMS_EVT * evt);
static uns32 proc_finalize_msg(CLMS_CB * cb, CLMSV_CLMS_EVT * evt);
static uns32 proc_track_start_msg(CLMS_CB * cb, CLMSV_CLMS_EVT * evt);
static uns32 proc_track_stop_msg(CLMS_CB * cb, CLMSV_CLMS_EVT * evt);
static uns32 proc_node_get_msg(CLMS_CB * cb, CLMSV_CLMS_EVT * evt);
static uns32 proc_node_get_async_msg(CLMS_CB * cb, CLMSV_CLMS_EVT * evt);
static uns32 proc_clm_response_msg(CLMS_CB * cb, CLMSV_CLMS_EVT * evt);
static uns32 clms_ack_to_response_msg(CLMS_CB * cb, CLMSV_CLMS_EVT * evt, SaAisErrorT ais_rc);
static uns32 clms_track_current_resp(CLMS_CB * cb, CLMS_CLUSTER_NODE * node, uns32 sync_resp,
				     MDS_DEST *dest, MDS_SYNC_SND_CTXT *ctxt,
				     uns32 client_id, SaAisErrorT ais_rc);
static void clms_track_current_apiresp(SaAisErrorT ais_rc, uns32 num_mem,
				       SaClmClusterNotificationT_4 * notify, MDS_DEST *dest, MDS_SYNC_SND_CTXT *ctxt);
static void clms_send_track_current_cbkresp(SaAisErrorT ais_rc, uns32 num_mem,
					    SaClmClusterNotificationT_4 * notify, MDS_DEST *dest,
					    uns32 client_id);

static const CLMSV_CLMS_EVT_HANDLER clms_clmsv_top_level_evt_dispatch_tbl[] = {
	process_api_evt,
	proc_clma_updn_mds_msg,
	proc_clma_updn_mds_msg,
	proc_mds_quiesced_ack_msg,
	proc_node_lock_tmr_exp_msg,
	proc_mds_node_evt,
	proc_rda_evt
};

static const CLMSV_CLMS_CLMA_API_MSG_HANDLER clms_clma_api_msg_dispatcher[] = {
	proc_initialize_msg,
	proc_finalize_msg,
	proc_track_start_msg,
	proc_track_stop_msg,
	proc_node_get_msg,
	proc_node_get_async_msg,
	proc_clm_response_msg,
	proc_node_up_msg
};

/**
* Clear any pending clma_down records or node_down list
*
*/
static void clms_process_clma_down_list(void)
{

	TRACE_ENTER();
	if (clms_cb->ha_state == SA_AMF_HA_ACTIVE) {
		/* Process The Agent Downs during the role change */
		CLMA_DOWN_LIST *clma_down_rec = NULL;
		CLMA_DOWN_LIST *temp_clma_down_rec = NULL;

		clma_down_rec = clms_cb->clma_down_list_head;
		while (clma_down_rec) {
			/*Remove the CLMA DOWN REC from the CLMA_DOWN_LIST */
			/* Free the CLMA_DOWN_REC */
			/* Remove this CLMA entry from our processing lists */
			temp_clma_down_rec = clma_down_rec;
			(void)clms_client_delete_by_mds_dest(clma_down_rec->mds_dest);
			clma_down_rec = clma_down_rec->next;
			free(temp_clma_down_rec);
		}
		clms_cb->clma_down_list_head = NULL;
		clms_cb->clma_down_list_tail = NULL;

		/*Process pending admin op for each node,walk thru node list to find if any pending admin op */
		clms_adminop_pending();

	}

	TRACE_LEAVE();
}

/**
 * Get client record from client ID
 * @param client_id
 * 
 * @return CLMS_CLIENT_INFO *
 */
CLMS_CLIENT_INFO *clms_client_get_by_id(uns32 client_id)
{
	uns32 client_id_net;
	CLMS_CLIENT_INFO *rec;

	client_id_net = m_NCS_OS_HTONL(client_id);
	rec = (CLMS_CLIENT_INFO *) ncs_patricia_tree_get(&clms_cb->client_db, (uns8 *)&client_id_net);

	if (NULL == rec)
		TRACE("client_id: %u lookup failed", client_id);

	return rec;
}

/**
 * Get the next client record from client db
 * @param client_id
 * 
 * @return CLMS_CLIENT_INFO *
 */
CLMS_CLIENT_INFO *clms_client_getnext_by_id(uns32 client_id)
{
	uns32 client_id_net;
	CLMS_CLIENT_INFO *rec;

	if (client_id == 0) {
		rec = (CLMS_CLIENT_INFO *) ncs_patricia_tree_getnext(&clms_cb->client_db, (uns8 *)0);
	} else {
		client_id_net = m_NCS_OS_HTONL(client_id);
		rec = (CLMS_CLIENT_INFO *) ncs_patricia_tree_getnext(&clms_cb->client_db, (uns8 *)&client_id_net);
	}

	return rec;
}

/**
 * Search for a client that matches the MDS dest and delete all the associated
 * resources.
 * @param cb
 * @param mds_dest
 * 
 * @return int
 */
uns32 clms_client_delete_by_mds_dest(MDS_DEST mds_dest)
{
	uns32 rc = 0;
	CLMS_CLIENT_INFO *client = NULL;
	uns32 client_id;

	TRACE_ENTER2("mds_dest %llx", mds_dest);

	client = (CLMS_CLIENT_INFO *) ncs_patricia_tree_getnext(&clms_cb->client_db, (uns8 *)0);

	while (client != NULL) {
		/** Store the client_id for get Next  */
		client_id = m_NCS_OS_HTONL(client->client_id);
		if (m_NCS_MDS_DEST_EQUAL(&client->mds_dest, &mds_dest)) {
			rc = clms_client_delete(client->client_id);

			/* Delete this client data from the clmresp tracking list */
			rc = clms_client_del_trackresp(client->client_id);
			if (rc != NCSCC_RC_SUCCESS) {
				TRACE("clms_client_delete_trackresp FAILED: %u", rc);
			}
			break;
		}

		client = (CLMS_CLIENT_INFO *) ncs_patricia_tree_getnext(&clms_cb->client_db, (uns8 *)&client_id);
	}
	TRACE_LEAVE();
	return rc;
}

/**
 * Delete a client record.
 * @param cb
 * @param client_id
 * 
 * @return uns32
 */
uns32 clms_client_delete(uns32 client_id)
{
	CLMS_CLIENT_INFO *client;
	uns32 status = 0;

	TRACE_ENTER2("client_id %u", client_id);

	if ((client = clms_client_get_by_id(client_id)) == NULL) {
		status = 1;
		goto done;
	}

	if (NCSCC_RC_SUCCESS != ncs_patricia_tree_del(&clms_cb->client_db, &client->pat_node)) {
		TRACE("ncs_patricia_tree_del FAILED");
		status = 2;
		goto done;
	}

	free(client);

 done:
	TRACE_LEAVE();
	return status;
}

/**
 * Allocates client_id for the new client and add it to 
 * the current client database
 *		  
 * @param  mds_dest  mds_dest to send
 * @param  client_id  client_id 
 *
 * @return CLMS_CLIENT_INFO *
 *
 */
CLMS_CLIENT_INFO *clms_client_new(MDS_DEST mds_dest, uns32 client_id)
{
	CLMS_CLIENT_INFO *client = NULL;

	TRACE_ENTER2("MDS dest %llx", mds_dest);

	if (NULL == (client = calloc(1, sizeof(CLMS_CLIENT_INFO)))) {
		LOG_WA("calloc FAILED");
		goto done;
	}

    /** Initialize the record **/
	if ((clms_cb->ha_state == SA_AMF_HA_STANDBY) || (clms_cb->ha_state == SA_AMF_HA_QUIESCED))
		clms_cb->last_client_id = client_id;
	client->client_id = client_id;
	client->mds_dest = mds_dest;
	client->client_id_net = m_NCS_OS_HTONL(client->client_id);
	client->pat_node.key_info = (uns8 *)&client->client_id_net;

    /** Insert the record into the patricia tree **/
	if (NCSCC_RC_SUCCESS != ncs_patricia_tree_add(&clms_cb->client_db, &client->pat_node)) {
		LOG_WA("FAILED: ncs_patricia_tree_add, client_id %u", client_id);
		free(client);
		client = NULL;
		goto done;
	}

 done:
	TRACE_LEAVE2("client_id %u", client_id);
	return client;
}

/**
 * Processing Node up mesg from the node_agent utility
 *                
 * @param evt     : Ptr to CLMSV_CLMS_EVT  
 *
 * @return  NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE 
 *
 */
uns32 proc_node_up_msg(CLMS_CB * cb, CLMSV_CLMS_EVT * evt)
{
	clmsv_clms_node_up_info_t *nodeup_info = &(evt->info.msg.info.api_info.param).nodeup_info;
	CLMS_CLUSTER_NODE *node = NULL;
	SaUint32T nodeid;
	uns32 rc = NCSCC_RC_SUCCESS;
	SaNameT node_name = { 0 };
	CLMSV_MSG clm_msg;
	SaBoolT check_member;

	TRACE_ENTER2("Node up mesg for nodename length %d %s", nodeup_info->node_name.length,
		     nodeup_info->node_name.value);

	/* Generate a CLM node DN with the help of cluster DN */
	node_name.length = snprintf((char *)node_name.value, sizeof(node_name.value),
				    "safNode=%s,%s", nodeup_info->node_name.value, osaf_cluster->name.value);

	node = clms_node_get_by_name(&node_name);
	if (node == NULL) {
		LOG_ER("CLM NodeName %s doesn't match entry in imm.xml. Specify a correct DN name in node_name file ",
		       nodeup_info->node_name.value);
		clm_msg.evt_type = CLMSV_CLMS_TO_CLMA_API_RESP_MSG;
		clm_msg.info.api_resp_info.type = CLMSV_CLUSTER_JOIN_RESP;
		clm_msg.info.api_resp_info.rc = SA_AIS_ERR_NOT_EXIST;
		clm_msg.info.api_resp_info.param.node_name = node_name;
		rc = clms_mds_msg_send(cb, &clm_msg, &evt->fr_dest, &evt->mds_ctxt, MDS_SEND_PRIORITY_HIGH,
				       NCSMDS_SVC_ID_CLMNA);
		/*as this is failure case of node == NULL, making rc = success to avoid irrelevant error from process_api_evt() */
		rc = NCSCC_RC_SUCCESS;
		goto done;
	}

	/*we got node, lets send node_agent responce */
	clm_msg.evt_type = CLMSV_CLMS_TO_CLMA_API_RESP_MSG;
	clm_msg.info.api_resp_info.type = CLMSV_CLUSTER_JOIN_RESP;
	clm_msg.info.api_resp_info.rc = SA_AIS_OK;
	clm_msg.info.api_resp_info.param.node_name = node_name;
	/*rc will be updated down in the positive flow */
	rc = clms_mds_msg_send(cb, &clm_msg, &evt->fr_dest, &evt->mds_ctxt, MDS_SEND_PRIORITY_HIGH,
			       NCSMDS_SVC_ID_CLMNA);
	/*if mds send failed, we need to report failure */
	if (rc != NCSCC_RC_SUCCESS)
		goto done;

	nodeid = evt->info.msg.info.api_info.param.nodeup_info.node_id;

	/* This has to be updated always */
	node->nodeup = SA_TRUE;
	check_member = node->member;
	

	/* Self Node needs to be added tp patricia tree before hand during init */
	if (NULL ==  clms_node_get_by_id(nodeid)) {
		node->node_id = evt->info.msg.info.api_info.param.nodeup_info.node_id;

		TRACE("node->node_id %u node->nodeup %d", node->node_id, node->nodeup);

		if (clms_node_add(node, 0) != NCSCC_RC_SUCCESS) {
			LOG_ER("Patricia tree add failed:crosscheck " PKGSYSCONFDIR "/node_name configuration");
		}
	}
	node->boot_time = clms_get_SaTime();

	/*When plm not in model,membership status depends only on the nodeup */
	if (node->admin_state == SA_CLM_ADMIN_UNLOCKED) {

		if (clms_cb->reg_with_plm == SA_FALSE) {
			node->member = SA_TRUE;
		}
#ifdef ENABLE_AIS_PLM
		else if (node->ee_red_state == SA_PLM_READINESS_IN_SERVICE) {
			node->member = SA_TRUE;
		}
#endif

		if (node->member == SA_TRUE) {
			if(check_member == SA_FALSE){
				++(osaf_cluster->num_nodes);
			}
			node->stat_change = SA_TRUE;
			node->init_view = ++(clms_cb->cluster_view_num);
			TRACE("node->init_view %llu", node->init_view);
			node->change = SA_CLM_NODE_JOINED;
			clms_send_track(clms_cb, node, SA_CLM_CHANGE_COMPLETED);
			/* Clear node->stat_change after sending the callback to its clients */
			node->stat_change = SA_FALSE;

			/* Send Node join notification */
			rc = clms_node_join_ntf(clms_cb, node);
			if (rc != NCSCC_RC_SUCCESS) {
				TRACE("clms_node_join_ntf failed %u", rc);
			}
			clms_node_update_rattr(node);
			clms_cluster_update_rattr(osaf_cluster);
			ckpt_node_rec(node);
			ckpt_cluster_rec();
			node->change = SA_CLM_NODE_NO_CHANGE;	
		}
	}

 done:
	TRACE_LEAVE();
	return rc;

}

/**
 * This is the function which is called when clms lock timer expires
 *
 * @param  evt  - Message that was posted to the CLMS Mail box.
 *
 * @return NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 */
static uns32 proc_node_lock_tmr_exp_msg(CLMSV_CLMS_EVT * evt)
{
	uns32 rc = NCSCC_RC_SUCCESS;
	SaClmNodeIdT node_id;
	CLMS_CLUSTER_NODE *op_node = NULL;

	TRACE_ENTER();
	/*Get the node details */
	node_id = evt->info.tmr_info.node_id;
	op_node = clms_node_get_by_id(node_id);
	assert(op_node != NULL);

	rc = clms_node_trackresplist_empty(op_node);
	if (rc != NCSCC_RC_SUCCESS)
		goto done;
	clms_clmresp_error_timeout(clms_cb, op_node);

	rc = clms_node_exit_ntf(clms_cb, op_node);
	if (rc != NCSCC_RC_SUCCESS) {
		LOG_ER("clms_node_exit_ntf failed %u", rc);
	}
	rc = clms_node_admin_state_change_ntf(clms_cb, op_node, SA_CLM_ADMIN_LOCKED);
	if (rc != NCSCC_RC_SUCCESS) {
		LOG_ER("clms_node_admin_state_change_ntf failed %d", rc);
	}

	/*you have to reboot the node in case of imm */
	if (clms_cb->reg_with_plm == SA_TRUE)
		clms_reboot_remote_node(op_node,"Rebooting,timer expired for start step callback response");

	/*Checkpoint */

 done:
	TRACE_LEAVE();
	return rc;
}

void clms_track_send_node_down(CLMS_CLUSTER_NODE *node)
{
	uns32 rc = NCSCC_RC_SUCCESS;

	node->nodeup = 0;
	TRACE_ENTER2("MDS Down nodeup info %d", node->nodeup);

	if(node->member == SA_FALSE)
		goto done;

	if(node->member == SA_TRUE){
		--(osaf_cluster->num_nodes);
	}

	/*Irrespective of plm in system or not,toggle the membership status for MDS NODE DOWN*/
	node->member = SA_FALSE;
	node->stat_change = SA_TRUE;
	node->change = SA_CLM_NODE_LEFT;
	++(clms_cb->cluster_view_num);
	clms_send_track(clms_cb, node, SA_CLM_CHANGE_COMPLETED);
	/* Clear node->stat_change after sending the callback to its clients */
	node->stat_change = SA_FALSE;

	rc = clms_node_exit_ntf(clms_cb, node);
	if (rc != NCSCC_RC_SUCCESS) {
		TRACE("clms_node_exit_ntf failed %u", rc);
	}
	/*Update IMMSV */
	clms_node_update_rattr(node);
	clms_cluster_update_rattr(osaf_cluster);

	if(clms_cb->ha_state == SA_AMF_HA_ACTIVE){
		ckpt_node_rec(node);
		ckpt_node_down_rec(node);
		ckpt_cluster_rec();
	}

	/*For the NODE DOWN, boottimestamp will not be updated */

	/* Delete the node reference from the nodeid database */
done:
	if (clms_node_delete(node, 0) != NCSCC_RC_SUCCESS) {
		LOG_ER("CLMS node delete by nodeid failed");
	}

	TRACE_LEAVE();
}

/**
* Process the rda callback and change the role
*/
static uns32 proc_rda_evt(CLMSV_CLMS_EVT * evt)
{
	uns32 rc = NCSCC_RC_SUCCESS;
	SaAmfHAStateT prev_haState;
	NCS_BOOL role_change = TRUE;

	TRACE_ENTER2("%u", evt->info.rda_info.io_role);

	prev_haState = clms_cb->ha_state;

	if (evt->info.rda_info.io_role == PCS_RDA_ACTIVE) {
		LOG_NO("ACTIVE request");
		clms_cb->mds_role = V_DEST_RL_ACTIVE;
		clms_cb->ha_state = SA_AMF_HA_ACTIVE;

		/*osaf_cluster->init_time is still 0 means before checkpointing \
		   the active went down in that case get it gain from local node */
		if (osaf_cluster->init_time == 0)
			osaf_cluster->init_time = clms_get_SaTime();

		/* Handle active to active role change. */
		if ((prev_haState == SA_AMF_HA_ACTIVE) && (clms_cb->ha_state == SA_AMF_HA_ACTIVE))
			role_change = FALSE;

		if (role_change == TRUE){

			/* fail over, become implementer */
			clms_imm_impl_set(clms_cb);



			if ((rc = clms_mds_change_role(clms_cb)) != NCSCC_RC_SUCCESS) {
				LOG_ER("clms_mds_change_role FAILED %u", rc);
				goto done;
			}

			if (NCSCC_RC_SUCCESS != clms_mbcsv_change_HA_state(clms_cb))
				goto done;
		}

	}
	clms_process_clma_down_list();
 done:
	TRACE_LEAVE();
	return rc;
}

/**
 * This is the function which is called when clms receives any
 * a Cluster Node UP/DN message via MDS subscription.
 *
 * @param evt  - Message that was posted to the CLMS Mail box.
 *
 * @return NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 */
static uns32 proc_mds_node_evt(CLMSV_CLMS_EVT * evt)
{
	uns32 rc = NCSCC_RC_SUCCESS;
	CLMS_CLUSTER_NODE *node = NULL;
	SaUint32T node_id = evt->info.node_mds_info.node_id;

	node = clms_node_get_by_id(node_id);

	if (node == NULL) {
		LOG_ER("Node %d doesn't exist", node_id);
		rc = NCSCC_RC_FAILURE;
		goto done;
	}

	if ((clms_cb->ha_state == SA_AMF_HA_ACTIVE) ||(clms_cb->ha_state == SA_AMF_HA_QUIESCED)) {
		clms_track_send_node_down(node);

	} else if (clms_cb->ha_state == SA_AMF_HA_STANDBY) {
		NODE_DOWN_LIST *node_down_rec = NULL;
		if (NULL == (node_down_rec = (NODE_DOWN_LIST *) malloc(sizeof(NODE_DOWN_LIST)))) {
			rc = SA_AIS_ERR_NO_MEMORY;
			LOG_ER("Memory Allocation for NODE_DOWN_LIST failed");
			goto done;
		}
		memset(node_down_rec, 0, sizeof(NODE_DOWN_LIST));
		node_down_rec->node_id = node_id;
		if (clms_cb->node_down_list_head == NULL) {
			clms_cb->node_down_list_head = node_down_rec;
		} else {
			if (clms_cb->node_down_list_tail)
				clms_cb->node_down_list_tail->next = node_down_rec;
		}
		clms_cb->node_down_list_tail = node_down_rec;
	}

 done:
	TRACE_LEAVE();
	return rc;

}

/**
 * This is the function which is called when clms receives any
 * a CLMA UP/DN message via MDS subscription.
 *
 * @param evt  - Message that was posted to the CLMS Mail box.
 *
 * @return  NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 */

static uns32 proc_clma_updn_mds_msg(CLMSV_CLMS_EVT * evt)
{
	TRACE_ENTER();
	CLMS_CKPT_REC ckpt;
	uns32 rc = NCSCC_RC_SUCCESS;

	switch (evt->type) {
	case CLMSV_CLMS_CLMA_UP:
		break;
	case CLMSV_CLMS_CLMA_DOWN:
		if ((clms_cb->ha_state == SA_AMF_HA_ACTIVE) || (clms_cb->ha_state == SA_AMF_HA_QUIESCED)) {
			/* Remove this CLMA entry from our processing lists */
			(void)clms_client_delete_by_mds_dest(evt->fr_dest);

			/*Send an async checkpoint update to STANDBY EDS peer */
			if (clms_cb->ha_state == SA_AMF_HA_ACTIVE) {
				memset(&ckpt, 0, sizeof(ckpt));
				ckpt.header.type = CLMS_CKPT_AGENT_DOWN_REC;
				ckpt.header.num_ckpt_records = 1;
				ckpt.header.data_len = 1;
				ckpt.param.agent_rec.mds_dest = evt->fr_dest;
				rc = clms_send_async_update(clms_cb, &ckpt, NCS_MBCSV_ACT_ADD);
				if (rc == NCSCC_RC_SUCCESS) {
					TRACE_4("ASYNC UPDATE SEND SUCCESS for CLMA_DOWN event..");
				}
			}
		} else if (clms_cb->ha_state == SA_AMF_HA_STANDBY) {
			CLMA_DOWN_LIST *clma_down_rec = NULL;
			if (clms_clma_entry_valid(clms_cb, evt->fr_dest)) {
				if (NULL == (clma_down_rec = (CLMA_DOWN_LIST *) malloc(sizeof(CLMA_DOWN_LIST)))) {
					/* Log it */
					rc = SA_AIS_ERR_NO_MEMORY;
					TRACE("memory allocation for the CLMA_DOWN_LIST failed");
					break;
				}
				memset(clma_down_rec, 0, sizeof(CLMA_DOWN_LIST));
				clma_down_rec->mds_dest = evt->fr_dest;
				if (clms_cb->clma_down_list_head == NULL) {
					clms_cb->clma_down_list_head = clma_down_rec;
				} else {
					if (clms_cb->clma_down_list_tail)
						clms_cb->clma_down_list_tail->next = clma_down_rec;
				}
				clms_cb->clma_down_list_tail = clma_down_rec;
			}
		}

		break;
	default:
		TRACE("Unknown evt type!!!");
		break;
	}

	TRACE_LEAVE();
	return rc;
}

/**
 * This is the function which is called when clms receives an
 * quiesced ack event from MDS 
 *	      
 * @param     : evt  - Message that was posted to the CLMS Mail box.
 *      
 * @return  NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 */
static uns32 proc_mds_quiesced_ack_msg(CLMSV_CLMS_EVT * evt)
{
	TRACE_ENTER();
	if (clms_cb->is_quiesced_set == TRUE) {
		clms_cb->ha_state = SA_AMF_HA_QUIESCED;
		/* Give up our IMM OI implementer role */
		(void)immutil_saImmOiImplementerClear(clms_cb->immOiHandle);
		/* Inform MBCSV of HA state change */
		if (clms_mbcsv_change_HA_state(clms_cb) != NCSCC_RC_SUCCESS)
			TRACE("clms_mbcsv_change_HA_state FAILED");

		/* Update control block */
		saAmfResponse(clms_cb->amf_hdl, clms_cb->amf_inv, SA_AIS_OK);
		clms_cb->is_quiesced_set = FALSE;
	}
	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

/**
 * This function is called on receiving clmresponse for clm track callback
 * clms_ack_to_response_msg is used to send ack to saClmResponse before 
 * data processing at server, if clms_ack_to_response_msg fails,  
 * we are tracing failure reason and moving forward
 *	
 * @param evt  - Message that was posted to the CLMS Mail box.
 *
 * @return NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
*/
static uns32 proc_clm_response_msg(CLMS_CB * cb, CLMSV_CLMS_EVT * evt)
{
	clmsv_clm_response_param_t *param = &(evt->info.msg.info.api_info.param).clm_resp;
	CLMS_CLUSTER_NODE *op_node = NULL;
	CLMS_TRACK_INFO *trkrec = NULL;
	SaUint32T nodeid;
	SaAisErrorT ais_rc = SA_AIS_OK;
	uns32 rc = NCSCC_RC_SUCCESS;

	TRACE_ENTER2("param->resp %d", param->resp);

	nodeid = (SaUint32T)m_CLMSV_INV_UNPACK_NODEID(param->inv);
	op_node = clms_node_get_by_id(nodeid);

	if (op_node == NULL) {
		LOG_ER("Invalid Invocation Id in saClmResponse");
		ais_rc = SA_AIS_ERR_INVALID_PARAM;
		rc = clms_ack_to_response_msg(cb, evt, ais_rc);
		if(rc != NCSCC_RC_SUCCESS)
			TRACE("clms_ack_to_response_msg failed rc = %u", (unsigned int)rc);
		goto done;
	}

	if (ncs_patricia_tree_size(&op_node->trackresp) == 0) {
		TRACE("Drop the clmresponse message on the new active");
		rc = clms_ack_to_response_msg(cb, evt, ais_rc);
		if(rc != NCSCC_RC_SUCCESS)
			TRACE("clms_ack_to_response_msg failed rc = %u", (unsigned int)rc);
		goto done;
	}

	trkrec = (CLMS_TRACK_INFO *) ncs_patricia_tree_get(&op_node->trackresp, (uns8 *)&param->inv);

	if (trkrec == NULL) {
		LOG_ER("Invalid Invocation Id in saClmResponse");
		ais_rc = SA_AIS_ERR_INVALID_PARAM;
		rc = clms_ack_to_response_msg(cb, evt, ais_rc);
		if(rc != NCSCC_RC_SUCCESS)
			TRACE("clms_ack_to_response_msg failed rc = %u", (unsigned int)rc);
		goto done;
	}

	switch (param->resp) {

	case SA_CLM_CALLBACK_RESPONSE_ERROR:
		TRACE("Clm Client responded with error");
		rc = clms_ack_to_response_msg(cb, evt, ais_rc);
		if(rc != NCSCC_RC_SUCCESS)
			TRACE("clms_ack_to_response_msg failed rc = %u", (unsigned int)rc);

		rc = clms_clmresp_error(cb, op_node);
		if (rc != NCSCC_RC_SUCCESS) {
			LOG_ER("clms_clmresp_error Failed");
		}
		break;

	case SA_CLM_CALLBACK_RESPONSE_REJECTED:
		TRACE("Clm Client rejected the operation");
		rc = clms_ack_to_response_msg(cb, evt, ais_rc);
		if(rc != NCSCC_RC_SUCCESS)
			TRACE("clms_ack_to_response_msg failed rc = %u", (unsigned int)rc);

		rc = clms_clmresp_rejected(cb, op_node, trkrec);
		if (rc != NCSCC_RC_SUCCESS) {
			LOG_ER("clms_clmresp_rejected failed");
		}
		break;
	case SA_CLM_CALLBACK_RESPONSE_OK:
		TRACE("CLM Client responded OK");
		rc = clms_ack_to_response_msg(cb, evt, ais_rc);
		if(rc != NCSCC_RC_SUCCESS)
			TRACE("clms_ack_to_response_msg failed rc = %u", (unsigned int)rc);

		rc = clms_clmresp_ok(cb, op_node, trkrec);
		if (rc != NCSCC_RC_SUCCESS) {
			LOG_ER("clms_clmresp_ok failed");
		}
		break;
	}

done:
	TRACE_LEAVE();
	return rc;
}

/**
 * This function is called on receiving track start request
 *
 * @param evt  - Message that was posted to the CLMS Mail box.
 *
 * @return NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
*/

static uns32 proc_track_start_msg(CLMS_CB * cb, CLMSV_CLMS_EVT * evt)
{
	uns32 rc = NCSCC_RC_SUCCESS;
	clmsv_track_start_param_t *param = &(evt->info.msg.info.api_info.param).track_start;
	CLMS_CLIENT_INFO *client;
	CLMS_CLUSTER_NODE *node = NULL;
	CLMS_CKPT_REC ckpt;
	SaAisErrorT ais_rc = SA_AIS_OK;
	/*SaUint32T  node_id = evt->info.mds_info.node_id; */
	SaUint32T node_id = m_NCS_NODE_ID_FROM_MDS_DEST(evt->fr_dest);

	TRACE_ENTER();

	node = clms_node_get_by_id(node_id);
	TRACE("Node id = %d", node_id);
	if (node == NULL) {
		TRACE("Client is tracking on an unconfigured node");
		ais_rc = SA_AIS_ERR_UNAVAILABLE;
	}

	client = clms_client_get_by_id(param->client_id);
	if (client == NULL) {
		TRACE("Bad client %d", param->client_id);
		ais_rc = SA_AIS_ERR_BAD_HANDLE;
	} else {
		/*Update the client database for the current trackflags */
		if (client->track_flags == 0){
			client->track_flags = param->flags; 
		}else {
			if((param->flags & SA_TRACK_CHANGES) || (param->flags & SA_TRACK_CHANGES_ONLY))
				client->track_flags = param->flags;
			else if ((param->flags & SA_TRACK_LOCAL) && (param->flags & SA_TRACK_CURRENT)) 
				client->track_flags = (client->track_flags | SA_TRACK_LOCAL | SA_TRACK_CURRENT);
			else if (param->flags & SA_TRACK_CURRENT)
				client->track_flags = (client->track_flags | SA_TRACK_CURRENT);
		}
	}

	/*Send only the local node data */
	if (client != NULL){
		if ((client->track_flags & SA_TRACK_LOCAL) && (client->track_flags & SA_TRACK_CURRENT)) {

			TRACE("Send track response for the local node");
			rc = clms_track_current_resp(cb, node, param->sync_resp, &evt->fr_dest, &evt->mds_ctxt, param->client_id, ais_rc);
			if (rc != NCSCC_RC_SUCCESS) {
				TRACE("Sending response for TRACK_LOCAL failed %u", (unsigned int)rc);
				goto done;
			}

		} else if (client->track_flags & SA_TRACK_CURRENT) {
			TRACE("Send response for SA_TRACK_CURRENT");
			if (node != NULL) {
				if (node->member == SA_FALSE) {
					TRACE("Send reponse when the node is not a cluster member");
					rc = clms_track_current_resp(cb, node, param->sync_resp, &evt->fr_dest, &evt->mds_ctxt,
							param->client_id, ais_rc);
				} else {
					TRACE("Send response for the node being cluster member");
					rc = clms_track_current_resp(cb, NULL, param->sync_resp, &evt->fr_dest, &evt->mds_ctxt,
							param->client_id, ais_rc);
				}
			}

			if (rc != NCSCC_RC_SUCCESS) {
				TRACE("Sending response for TRACK_CURRENT failed %d", (unsigned int)rc);
				goto done;
			}
		}

		/*Checkpoint the client trackflags */
		if ((client != NULL) && (cb->ha_state == SA_AMF_HA_ACTIVE) && (ais_rc == SA_AIS_OK)) {

			memset(&ckpt, 0, sizeof(CLMS_CKPT_REC));
			ckpt.header.type = CLMS_CKPT_TRACK_CHANGES_REC;
			ckpt.header.num_ckpt_records = 1;
			ckpt.header.data_len = 1;
			ckpt.param.client_rec.client_id = param->client_id;
			ckpt.param.client_rec.track_flags = client->track_flags;

			rc = clms_send_async_update(clms_cb, &ckpt, NCS_MBCSV_ACT_ADD);
			if (rc != NCSCC_RC_SUCCESS)
				TRACE("send_async_update FAILED");
		}
	}
 done:
	TRACE_LEAVE();

	return rc;
}

/**
 * This function is called on receiving track stop request
 *
 * @parm evt  - Message that was posted to the CLMS Mail box.
 *      
 * @return NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
*/

static uns32 proc_track_stop_msg(CLMS_CB * cb, CLMSV_CLMS_EVT * evt)
{
	uns32 rc = NCSCC_RC_SUCCESS;
	clmsv_track_stop_param_t *param = &(evt->info.msg.info.api_info.param).track_stop;
	CLMS_CLIENT_INFO *client = NULL;
	SaAisErrorT ais_rc = SA_AIS_OK;
	CLMSV_MSG clm_msg;
	CLMS_CKPT_REC ckpt;
	SaUint32T node_id = m_NCS_NODE_ID_FROM_MDS_DEST(evt->fr_dest);
	CLMS_CLUSTER_NODE *node = NULL;

	TRACE_ENTER();

	node = clms_node_get_by_id(node_id);
	TRACE("Node id = %d", node_id);
	if (node == NULL) {
		TRACE("Client tracking on an unconfigured node");
		ais_rc = SA_AIS_ERR_UNAVAILABLE;
		goto snd_rsp;
	}

	client = clms_client_get_by_id(param->client_id);

	if (client == NULL) {
		TRACE("Invalid Client ID");
		ais_rc = SA_AIS_ERR_BAD_HANDLE;
		goto snd_rsp;
	}

	if (!(client->track_flags & (SA_TRACK_CHANGES | SA_TRACK_CHANGES_ONLY))) {
		TRACE("Client didn't subscribe for track start");
		ais_rc = SA_AIS_ERR_NOT_EXIST;
		goto snd_rsp;
	}

	/*Update the client database for the current trackflags */
	client->track_flags = 0;

 snd_rsp:

	clm_msg.evt_type = CLMSV_CLMS_TO_CLMA_API_RESP_MSG;
	clm_msg.info.api_resp_info.type = CLMSV_TRACK_STOP_RESP;
	clm_msg.info.api_resp_info.rc = ais_rc;
	rc = clms_mds_msg_send(cb, &clm_msg, &evt->fr_dest, &evt->mds_ctxt, MDS_SEND_PRIORITY_HIGH, NCSMDS_SVC_ID_CLMA);
	if (rc != NCSCC_RC_SUCCESS) {
		LOG_ER("clms_mds_msg_send failed rc %u", (unsigned int)rc);
		goto done;
	}

	/*Checkpoint the client database to Standby */
	if ((cb->ha_state == SA_AMF_HA_ACTIVE) && (ais_rc == SA_AIS_OK)) {

		memset(&ckpt, 0, sizeof(CLMS_CKPT_REC));
		ckpt.header.type = CLMS_CKPT_TRACK_CHANGES_REC;
		ckpt.header.num_ckpt_records = 1;
		ckpt.header.data_len = 1;
		ckpt.param.client_rec.client_id = param->client_id;
		ckpt.param.client_rec.track_flags = client->track_flags;

		rc = clms_send_async_update(clms_cb, &ckpt, NCS_MBCSV_ACT_ADD);
		if (rc != NCSCC_RC_SUCCESS)
			TRACE("send_async_update FAILED");
	}
 done:
	TRACE_LEAVE();
	return rc;
}

/**
 * This function is called on clma request for node information 
 *
 * @param evt  - Message that was posted to the CLMS Mail box.
 *      
 * @return  NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
*/
static uns32 proc_node_get_msg(CLMS_CB * cb, CLMSV_CLMS_EVT * evt)
{
	clmsv_node_get_param_t *param = &(evt->info.msg.info.api_info.param).node_get;
	CLMSV_MSG clm_msg;
	CLMS_CLUSTER_NODE *node = NULL, *local_node = NULL;
	SaClmNodeIdT nodeid, local_nodeid;
	SaAisErrorT ais_rc = SA_AIS_OK;

	TRACE_ENTER();
	memset(&clm_msg, 0, sizeof(CLMSV_MSG));
	
	local_nodeid = m_NCS_NODE_ID_FROM_MDS_DEST(evt->fr_dest);

	if (param->node_id == SA_CLM_LOCAL_NODE_ID) {
		nodeid = m_NCS_NODE_ID_FROM_MDS_DEST(evt->fr_dest);
		TRACE("nodeid after getting it from mds_dest %d", nodeid);
	} else
		nodeid = param->node_id;

	local_node = clms_node_get_by_id(local_nodeid);
	node = clms_node_get_by_id(nodeid);

	if(local_node) {
		if (node) {
			if (node->member == SA_TRUE && local_node->member == SA_TRUE) {
				clm_msg.evt_type = CLMSV_CLMS_TO_CLMA_API_RESP_MSG;
				clm_msg.info.api_resp_info.type = CLMSV_NODE_GET_RESP;
				clm_msg.info.api_resp_info.rc = ais_rc;
				clm_msg.info.api_resp_info.param.node_get.nodeId = node->node_id;
				clm_msg.info.api_resp_info.param.node_get.nodeAddress = node->node_addr;
				clm_msg.info.api_resp_info.param.node_get.nodeName = node->node_name;
				clm_msg.info.api_resp_info.param.node_get.executionEnvironment = node->ee_name;
				clm_msg.info.api_resp_info.param.node_get.member = node->member;
				clm_msg.info.api_resp_info.param.node_get.bootTimestamp = node->boot_time;
				clm_msg.info.api_resp_info.param.node_get.initialViewNumber = node->init_view;
			} else {
				clm_msg.evt_type = CLMSV_CLMS_TO_CLMA_API_RESP_MSG;
				clm_msg.info.api_resp_info.type = CLMSV_NODE_GET_RESP;
				clm_msg.info.api_resp_info.rc = SA_AIS_ERR_UNAVAILABLE;
			}
		} else {
			clm_msg.evt_type = CLMSV_CLMS_TO_CLMA_API_RESP_MSG;
			clm_msg.info.api_resp_info.type = CLMSV_NODE_GET_RESP;
			clm_msg.info.api_resp_info.rc = SA_AIS_ERR_NOT_EXIST;
		}
	} else {
		clm_msg.evt_type = CLMSV_CLMS_TO_CLMA_API_RESP_MSG;
		clm_msg.info.api_resp_info.type = CLMSV_NODE_GET_RESP;
		clm_msg.info.api_resp_info.rc = SA_AIS_ERR_NOT_EXIST;
	}

	TRACE_LEAVE();
	/*Send the valid node info to clma */
	return clms_mds_msg_send(cb, &clm_msg, &evt->fr_dest, &evt->mds_ctxt, MDS_SEND_PRIORITY_HIGH,
				 NCSMDS_SVC_ID_CLMA);
}

/**
 * This function is called on aynsc clma request for node information 
 *
 * @paran evt  - Message that was posted to the CLMS Mail box.
 *      
 * @return NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
*/
static uns32 proc_node_get_async_msg(CLMS_CB * cb, CLMSV_CLMS_EVT * evt)
{
	clmsv_node_get_async_param_t *param = &(evt->info.msg.info.api_info.param).node_get_async;
	CLMSV_MSG clm_msg;
	CLMS_CLUSTER_NODE *node = NULL, *local_node = NULL;
	SaClmNodeIdT nodeid, local_nodeid;
	SaAisErrorT ais_rc = SA_AIS_OK;

	TRACE_ENTER();

	local_nodeid = m_NCS_NODE_ID_FROM_MDS_DEST(evt->fr_dest);

	memset(&clm_msg, 0, sizeof(CLMSV_MSG));
	if (param->node_id == SA_CLM_LOCAL_NODE_ID)
		nodeid = m_NCS_NODE_ID_FROM_MDS_DEST(evt->fr_dest);
	else
		nodeid = param->node_id;

	local_node = clms_node_get_by_id(local_nodeid);
	node = clms_node_get_by_id(nodeid);

	if (local_node) {
		if (node) {
			if (node->member == SA_TRUE && local_node->member == SA_TRUE) {
				clm_msg.evt_type = CLMSV_CLMS_TO_CLMA_CBK_MSG;
				clm_msg.info.cbk_info.type = CLMSV_NODE_ASYNC_GET_CBK;
				clm_msg.info.cbk_info.param.node_get.info.nodeId = node->node_id;
				clm_msg.info.cbk_info.param.node_get.info.nodeAddress = node->node_addr;
				clm_msg.info.cbk_info.param.node_get.info.nodeName = node->node_name;
				clm_msg.info.cbk_info.param.node_get.info.executionEnvironment = node->ee_name;
				clm_msg.info.cbk_info.param.node_get.info.member = node->member;
				clm_msg.info.cbk_info.param.node_get.info.bootTimestamp = node->boot_time;
				clm_msg.info.cbk_info.param.node_get.info.initialViewNumber = node->init_view;
				clm_msg.info.cbk_info.param.node_get.inv = param->inv;
				clm_msg.info.cbk_info.param.node_get.err = ais_rc;
				clm_msg.info.cbk_info.client_id = param->client_id;
			} else {
				TRACE("Node exists in the database but is non-member");
				clm_msg.evt_type = CLMSV_CLMS_TO_CLMA_CBK_MSG;
				clm_msg.info.cbk_info.type = CLMSV_NODE_ASYNC_GET_CBK;
				clm_msg.info.cbk_info.param.node_get.inv = param->inv;
				clm_msg.info.cbk_info.param.node_get.err = SA_AIS_ERR_UNAVAILABLE;
				clm_msg.info.cbk_info.client_id = param->client_id;
			}
		} else {
			TRACE("Node doesn't exist in the data base");
			clm_msg.evt_type = CLMSV_CLMS_TO_CLMA_CBK_MSG;
			clm_msg.info.cbk_info.type = CLMSV_NODE_ASYNC_GET_CBK;
			clm_msg.info.cbk_info.param.node_get.inv = param->inv;
			clm_msg.info.cbk_info.param.node_get.err = SA_AIS_ERR_NOT_EXIST;
			clm_msg.info.cbk_info.client_id = param->client_id;
		}
	} else {
		TRACE("Node doesn't exist in the data base");
		clm_msg.evt_type = CLMSV_CLMS_TO_CLMA_CBK_MSG;
		clm_msg.info.cbk_info.type = CLMSV_NODE_ASYNC_GET_CBK;
		clm_msg.info.cbk_info.param.node_get.inv = param->inv;
		clm_msg.info.cbk_info.param.node_get.err = SA_AIS_ERR_NOT_EXIST;
		clm_msg.info.cbk_info.client_id = param->client_id;
	}
	TRACE_LEAVE();

	/*Send the valid node info to clma */
	return clms_mds_msg_send(cb, &clm_msg, &evt->fr_dest, 0, MDS_SEND_PRIORITY_MEDIUM, NCSMDS_SVC_ID_CLMA);
}

/**
 * Handle a initialize message
 * @param cb
 * @param evt
 * 
 * @return uns32
 */
static uns32 proc_initialize_msg(CLMS_CB * cb, CLMSV_CLMS_EVT * evt)
{
	uns32 rc = NCSCC_RC_SUCCESS;
	SaAisErrorT ais_rc = SA_AIS_OK;
	CLMS_CKPT_REC ckpt;
	CLMS_CLIENT_INFO *client;
	CLMSV_MSG msg;
	SaUint32T node_id = m_NCS_NODE_ID_FROM_MDS_DEST(evt->fr_dest);
	CLMS_CLUSTER_NODE *node = NULL;

	TRACE_ENTER2("dest %llx", evt->fr_dest);

	/*Handle the wrap around */
	if (clms_cb->last_client_id == INT_MAX)
		clms_cb->last_client_id = 0;

	clms_cb->last_client_id++;

	node = clms_node_get_by_id(node_id);
	TRACE("Node id = %d", node_id);
	if (node == NULL) {
		TRACE("Client on an unconfigured node");
		ais_rc = SA_AIS_ERR_UNAVAILABLE;
	}

	if ((client = clms_client_new(evt->fr_dest, clms_cb->last_client_id)) == NULL) {
		TRACE("Creating a new client failed");
		ais_rc = SA_AIS_ERR_NO_MEMORY;
	}

	msg.evt_type = CLMSV_CLMS_TO_CLMA_API_RESP_MSG;
	msg.info.api_resp_info.type = CLMSV_INITIALIZE_RESP;
	msg.info.api_resp_info.rc = ais_rc;
	if (client != NULL)
		msg.info.api_resp_info.param.client_id = client->client_id;

	rc = clms_mds_msg_send(cb, &msg, &evt->fr_dest, &evt->mds_ctxt, MDS_SEND_PRIORITY_HIGH, NCSMDS_SVC_ID_CLMA);
	if (rc != NCSCC_RC_SUCCESS) {
		TRACE_LEAVE2("clms_mds_msg_send FAILED rc = %u", (unsigned int)rc);
		if (client != NULL)
			clms_client_delete(client->client_id);
		return rc;
	}

	if(node) {
		if(node->member == FALSE) {
			rc = clms_send_is_member_info(clms_cb, node->node_id, node->member, TRUE);
			if (rc != NCSCC_RC_SUCCESS) {
				TRACE("clms_send_is_member_info %u", rc);
				return rc;
			}
		}
	}

	if (client != NULL){

		if ((cb->ha_state == SA_AMF_HA_ACTIVE) && (ais_rc == SA_AIS_OK)) {

			memset(&ckpt, 0, sizeof(CLMS_CKPT_REC));
			ckpt.header.type = CLMS_CKPT_CLIENT_INFO_REC;
			ckpt.header.num_ckpt_records = 1;
			ckpt.header.data_len = 1;
			ckpt.param.client_rec.client_id = client->client_id;
			ckpt.param.client_rec.mds_dest = client->mds_dest;

			rc = clms_send_async_update(clms_cb, &ckpt, NCS_MBCSV_ACT_ADD);
			if (rc != NCSCC_RC_SUCCESS)
				TRACE("clms_send_async_update FAILED rc = %u", (unsigned int)rc);

		}
	}

	TRACE_LEAVE();
	return rc;

}

/**
 * Handle a finalize  message
 * @param cb
 * @param evt
 * 
 * @return uns32
 */
static uns32 proc_finalize_msg(CLMS_CB * cb, CLMSV_CLMS_EVT * evt)
{
	clmsv_finalize_param_t *param = &(evt->info.msg.info.api_info.param).finalize;
	uns32 rc;
	SaAisErrorT ais_rc = SA_AIS_OK;
	CLMS_CKPT_REC ckpt;
	CLMSV_MSG msg;

	TRACE_ENTER2("finalize for client: client_id %d", param->client_id);

	/* Free all resources allocated by this client. */
	if ((rc = clms_client_delete(param->client_id)) != 0) {
		TRACE("clms_client_delete FAILED: %u", rc);
		ais_rc = SA_AIS_ERR_BAD_HANDLE;
	}

	/* Delete this client data from the clmresp tracking list */
	rc = clms_client_del_trackresp(param->client_id);
	if (rc != NCSCC_RC_SUCCESS) {
		TRACE("clms_client_delete_trackresp FAILED: %u", rc);
		ais_rc = SA_AIS_ERR_BAD_HANDLE;
	}

	msg.evt_type = CLMSV_CLMS_TO_CLMA_API_RESP_MSG;
	msg.info.api_resp_info.type = CLMSV_FINALIZE_RESP;
	msg.info.api_resp_info.rc = ais_rc;
	msg.info.api_resp_info.param.client_id = param->client_id;
	rc = clms_mds_msg_send(cb, &msg, &evt->fr_dest, &evt->mds_ctxt, MDS_SEND_PRIORITY_HIGH, NCSMDS_SVC_ID_CLMA);
	if (rc != NCSCC_RC_SUCCESS) {
		TRACE_LEAVE2("clms_mds_msg_send failed rc = %u", (unsigned int)rc);
		return rc;
	}

	/* Checkpoint the client data */
	if ((cb->ha_state == SA_AMF_HA_ACTIVE) && (ais_rc == SA_AIS_OK)) {

		memset(&ckpt, 0, sizeof(CLMS_CKPT_REC));
		ckpt.header.type = CLMS_CKPT_FINALIZE_REC;
		ckpt.header.num_ckpt_records = 1;
		ckpt.header.data_len = 1;
		ckpt.param.finalize_rec.client_id = param->client_id;

		rc = clms_send_async_update(clms_cb, &ckpt, NCS_MBCSV_ACT_RMV);
		if (rc != NCSCC_RC_SUCCESS)
			TRACE("send_async_update FAILED");
	}

	TRACE_LEAVE2("finalize for client:client_id %d", param->client_id);
	return rc;

}

/**
 * This is the function which is called when clms receives an
 * event either because of an API Invocation or other internal
 * messages from CLMA clients
 *
 * @param evt  - Message that was posted to the CLMS Mail box.
 *
 * @return  NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 */
static uns32 process_api_evt(CLMSV_CLMS_EVT * evt)
{
	TRACE_ENTER();

	if (evt->type == CLMSV_CLMS_CLMSV_MSG) {
		/* ignore one level... */
		if (evt->info.msg.evt_type < CLMSV_MSG_MAX) {
			if (evt->info.msg.info.api_info.type < CLMSV_API_MAX) {

				if (clms_clma_api_msg_dispatcher[evt->info.msg.info.api_info.type] (clms_cb, evt) !=
				    NCSCC_RC_SUCCESS) {
					LOG_ER("clms_clma_api_msg_dispatcher FAILED: type %d", evt->info.msg.evt_type);
				}
			} else
				LOG_ER("Invalid msg type %d", evt->info.msg.info.api_info.type);
		} else
			LOG_ER("Invalid event type %d", evt->info.msg.evt_type);
	}
	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

/**
 *      
 * This is the function which process the IPC mail box of CLMS
 *      
 * @param  mbx  - This is the mail box pointer on which CLMS is 
 *			going to block.  
 *
 * @return  None.
 */
void clms_process_mbx(SYSF_MBX *mbx)
{
	CLMSV_CLMS_EVT *msg;

	TRACE_ENTER();

	msg = (CLMSV_CLMS_EVT *) ncs_ipc_non_blk_recv(mbx);

	if (msg == NULL) {
		LOG_ER("No mbx message although indicated in fd");
		goto done;
	}

	switch (msg->type) {

	case CLMSV_CLMS_CLMSV_MSG:
	case CLMSV_CLMS_CLMA_UP:
		if (clms_cb->ha_state == SA_AMF_HA_ACTIVE) {
			clms_clmsv_top_level_evt_dispatch_tbl[msg->type] (msg);
		}
		break;
	case CLMSV_CLMS_CLMA_DOWN:
		clms_clmsv_top_level_evt_dispatch_tbl[msg->type] (msg);
		break;

	case CLSMV_CLMS_QUIESCED_ACK:
		if (clms_cb->ha_state == SA_AMF_HA_ACTIVE) {
			proc_mds_quiesced_ack_msg(msg);
		}
		break;

	case CLMSV_CLMS_NODE_LOCK_TMR_EXP:
		if (clms_cb->ha_state == SA_AMF_HA_ACTIVE) {
			proc_node_lock_tmr_exp_msg(msg);
		}
		break;

	case CLMSV_CLMS_MDS_NODE_EVT:
		proc_mds_node_evt(msg);
		break;
	case CLMSV_CLMS_RDA_EVT:
		proc_rda_evt(msg);
		break;
	default:
		LOG_ER("message type invalid %d", msg->type);
		break;
	}

	clms_evt_destroy(msg);
 done:
	TRACE_LEAVE();
}

/**
* Send the API response for clm  current track
* @param[in] ais_rc   return value to be sent with api_resp
* @param[in] num_mem  number of items to be sent in the notification buffer
* @param[in] notify   pointer to notification buffer
* @param[in] dest     mds_dest to send
* @param[in] ctxt     context for the sync call
* @return             void 	
*/
static void clms_track_current_apiresp(SaAisErrorT ais_rc, uns32 num_mem, SaClmClusterNotificationT_4 * notify,
				       MDS_DEST *dest, MDS_SYNC_SND_CTXT *ctxt)
{
	TRACE_ENTER2("ais_rc %d , num_mem %d", ais_rc, num_mem);
	CLMSV_MSG msg;
	uns32 rc = NCSCC_RC_SUCCESS;

	memset(&msg, 0, sizeof(CLMSV_MSG));

	/* This is a sync call back, fill the api_resp msg */
	msg.evt_type = CLMSV_CLMS_TO_CLMA_API_RESP_MSG;
	msg.info.api_resp_info.type = CLMSV_TRACK_CURRENT_RESP;
	msg.info.api_resp_info.param.track.num = (SaUint16T)(osaf_cluster->num_nodes);
	msg.info.api_resp_info.param.track.notify_info =
	    (SaClmClusterNotificationBufferT_4 *) malloc(sizeof(SaClmClusterNotificationBufferT_4));
	msg.info.api_resp_info.param.track.notify_info->viewNumber = (clms_cb->cluster_view_num);

	if (ais_rc != SA_AIS_OK) {
		msg.info.api_resp_info.param.track.notify_info->numberOfItems = 0;
		msg.info.api_resp_info.param.track.notify_info->notification = NULL;
	} else {
		msg.info.api_resp_info.param.track.notify_info->numberOfItems = num_mem;
		msg.info.api_resp_info.param.track.notify_info->notification = notify;
	}
	msg.info.api_resp_info.rc = ais_rc;

	TRACE("Sending msg to mds");

	rc = clms_mds_msg_send(clms_cb, &msg, dest, ctxt, MDS_SEND_PRIORITY_HIGH, NCSMDS_SVC_ID_CLMA);
	if (rc != NCSCC_RC_SUCCESS) {
		TRACE("clms_mds_msg_send failed");
	}

	free(msg.info.api_resp_info.param.track.notify_info);
	TRACE_LEAVE();
}

/**
* Send callback for CLM current track
* @param[in] ais_rc   return value to be sent with api_resp
* @param[in] num_mem  number of items to be sent in the notification buffer
* @param[in] notify   pointer to notification buffer
* @param[in] dest     mds_dest to send
* @param[in] client   CLM client 
* @return             void 	
*/
static void clms_send_track_current_cbkresp(SaAisErrorT ais_rc, uns32 num_mem, SaClmClusterNotificationT_4 * notify,
					    MDS_DEST *dest, uns32 client_id)
{
	TRACE_ENTER();
	CLMSV_MSG msg;
	uns32 rc = NCSCC_RC_SUCCESS;

	memset(&msg, 0, sizeof(CLMSV_MSG));

	msg.evt_type = CLMSV_CLMS_TO_CLMA_CBK_MSG;
	msg.info.cbk_info.client_id = client_id;
	msg.info.cbk_info.type = CLMSV_TRACK_CBK;
	msg.info.cbk_info.param.track.mem_num = (osaf_cluster->num_nodes);
	msg.info.cbk_info.param.track.err = ais_rc;
	msg.info.cbk_info.param.track.inv = 0;	/* Response not required */
	msg.info.cbk_info.param.track.root_cause_ent = (SaNameT *)malloc(sizeof(SaNameT));
	msg.info.cbk_info.param.track.root_cause_ent->length = 10;	/* to avoid assert in mds */
	msg.info.cbk_info.param.track.cor_ids = (SaNtfCorrelationIdsT *) malloc(sizeof(SaNtfCorrelationIdsT));
	msg.info.cbk_info.param.track.step = SA_CLM_CHANGE_COMPLETED;
	msg.info.cbk_info.param.track.time_super = (SaTimeT)SA_TIME_UNKNOWN;
	msg.info.cbk_info.param.track.buf_info.viewNumber = (clms_cb->cluster_view_num);

	if (ais_rc != SA_AIS_OK) {
		msg.info.cbk_info.param.track.buf_info.numberOfItems = 0;
		msg.info.cbk_info.param.track.buf_info.notification = NULL;
	} else {
		msg.info.cbk_info.param.track.buf_info.numberOfItems = num_mem;
		msg.info.cbk_info.param.track.buf_info.notification = notify;
	}

	rc = clms_mds_msg_send(clms_cb, &msg, dest, NULL, MDS_SEND_PRIORITY_MEDIUM, NCSMDS_SVC_ID_CLMA);
	if (rc != NCSCC_RC_SUCCESS) {
		TRACE("clms_mds_msg_send failed rc = %d", rc);
	}

	free(msg.info.cbk_info.param.track.root_cause_ent);
	free(msg.info.cbk_info.param.track.cor_ids);
	TRACE_LEAVE();
}

 /**
 * Sends the current track response to the process that has requested earlier for tracking.
 *      
 * @param[in]  sync_resp  - Indication for synchronous/asynchronous call
 * @param[in]  dest       - mds_dest to send 
 * @param[in]  client     - the client which requested for tracking		 
 * @param[in]  node       - node = NULL ==> track info only for local node else all nodes
 * @param[in]  ctxt       - context for sync call
 * @param[in]  ais_rc     - return value to sent to clma	
 * @return        	  : NCSCC_RC_FAILURE
 *		   	    NCSCC_RC_SUCCESS	
 */
static uns32 clms_track_current_resp(CLMS_CB * cb, CLMS_CLUSTER_NODE * node, uns32 sync_resp, MDS_DEST *dest,
				     MDS_SYNC_SND_CTXT *ctxt, uns32 client_id, SaAisErrorT ais_rc)
{

	SaClmClusterNotificationT_4 *notify = NULL;
	CLMS_CLUSTER_NODE *rp = NULL;
	uns32 rc = NCSCC_RC_SUCCESS, i = 0;
	uns32 num_mem = 0;
	SaUint32T node_id = 0;

	TRACE_ENTER();

	if (ais_rc != SA_AIS_OK)
		goto snd_rsp;

	if (!node) {
		num_mem = clms_num_mem_node();
		notify = (SaClmClusterNotificationT_4 *) malloc(num_mem * sizeof(SaClmClusterNotificationT_4));

		if (!notify) {
			TRACE("malloc falied for the SaClmClusterNotificationT_4");
			ais_rc = SA_AIS_ERR_NO_MEMORY;
			goto snd_rsp;
		}
		memset(notify, 0, num_mem * sizeof(SaClmClusterNotificationT_4));

		while (((rp = clms_node_getnext_by_id(node_id)) != NULL) && (i < num_mem)) {
			node_id = rp->node_id;
			if (rp->member == SA_TRUE) {
				notify[i].clusterNode.nodeId = rp->node_id;
				notify[i].clusterNode.nodeAddress.family = rp->node_addr.family;
				notify[i].clusterNode.nodeAddress.length = rp->node_addr.length;
				memcpy(notify[i].clusterNode.nodeAddress.value, rp->node_addr.value,
				       notify[i].clusterNode.nodeAddress.length);

				notify[i].clusterNode.nodeName.length = rp->node_name.length;
				memcpy(notify[i].clusterNode.nodeName.value, rp->node_name.value,
				       notify[i].clusterNode.nodeName.length);
				notify[i].clusterNode.executionEnvironment.length = rp->ee_name.length;
				memcpy(notify[i].clusterNode.executionEnvironment.value, rp->ee_name.value,
				       notify[i].clusterNode.executionEnvironment.length);
				notify[i].clusterNode.member = rp->member;
				notify[i].clusterNode.bootTimestamp = rp->boot_time;
				notify[i].clusterNode.initialViewNumber = rp->init_view;
				notify[i].clusterChange = SA_CLM_NODE_NO_CHANGE;
				i++;
			}
		}

		goto snd_rsp;

	} else {
		num_mem = 1;
		notify = (SaClmClusterNotificationT_4 *) malloc(sizeof(SaClmClusterNotificationT_4));

		if (!notify) {
			TRACE("malloc failed for SaClmClusterNotificationT_4");
			ais_rc = SA_AIS_ERR_NO_MEMORY;
			goto snd_rsp;
		}
		memset(notify, 0, sizeof(SaClmClusterNotificationT_4));

		notify->clusterNode.nodeId = node->node_id;

		notify->clusterNode.nodeAddress.family = node->node_addr.family;
		notify->clusterNode.nodeAddress.length = node->node_addr.length;
		memcpy(notify->clusterNode.nodeAddress.value, node->node_addr.value,
		       notify->clusterNode.nodeAddress.length);

		notify->clusterNode.nodeName.length = node->node_name.length;
		memcpy(notify->clusterNode.nodeName.value, node->node_name.value, notify->clusterNode.nodeName.length);

		notify->clusterNode.executionEnvironment.length = node->ee_name.length;
		memcpy(notify->clusterNode.executionEnvironment.value, node->ee_name.value,
		       notify->clusterNode.executionEnvironment.length);

		notify->clusterNode.member = node->member;
		notify->clusterNode.bootTimestamp = node->boot_time;
		notify->clusterNode.initialViewNumber = node->init_view;

		if (node->member == SA_FALSE)
			notify->clusterChange = SA_CLM_NODE_LEFT;
		else
			notify->clusterChange = SA_CLM_NODE_NO_CHANGE;

		goto snd_rsp;
	}

 snd_rsp:

	if (sync_resp == 1) {
		TRACE("Send api response");
		clms_track_current_apiresp(ais_rc, num_mem, notify, dest, ctxt);
	} else {
		TRACE("Callback response");
		clms_send_track_current_cbkresp(ais_rc,num_mem, notify, dest, client_id);
	}

	if (notify)
		free(notify);

	TRACE_LEAVE();
	return rc;
}

 /**
 * clms_remove_clma_down_rec
 * 
 *  Searches the CLMA_DOWN_LIST for an entry whos MDS_DEST equals
 *  that passed in and removes the CLMA rec. 
 *  
 * This routine is typically used to remove the clma down rec from standby 
 * CLMA_DOWN_LIST as  CLMA client has gone away.
 */
uns32 clms_remove_clma_down_rec(CLMS_CB * cb, MDS_DEST mds_dest)
{
	CLMA_DOWN_LIST *clma_down_rec = cb->clma_down_list_head;
	CLMA_DOWN_LIST *prev = NULL;
	while (clma_down_rec) {
		if (m_NCS_MDS_DEST_EQUAL(&clma_down_rec->mds_dest, &mds_dest)) {
			/* Remove the CLMA entry */
			/* Reset pointers */
			if (clma_down_rec == cb->clma_down_list_head) {	/* 1st in the list? */
				if (clma_down_rec->next == NULL) {
					/* Only one in the list? */
					cb->clma_down_list_head = NULL;	/* Clear head sublist pointer */
					cb->clma_down_list_tail = NULL;	/* Clear tail sublist pointer */
				} else {
					/* 1st but not only one */
					cb->clma_down_list_head = clma_down_rec->next;	/* Move next one up */
				}
			} else {
				if (prev) {
					if (clma_down_rec->next == NULL)
						cb->clma_down_list_tail = prev;
					prev->next = clma_down_rec->next;	/* Link previous to next */
				}
			}

			/* Free the CLMA_DOWN_REC */
			free(clma_down_rec);
			clma_down_rec = NULL;
			break;
		}
		prev = clma_down_rec;	/* Remember address of this entry */
		clma_down_rec = clma_down_rec->next;	/* Go to next entry */
	}
	return NCSCC_RC_SUCCESS;
}

/**
 * clms_remove_node_down_rec
 * 
 *  Searches the NODE_DOWN_LIST for an entry whose node_id equals
 *  that passed in and removes the node_down rec. 
 *  
 * This routine is typically used to remove the node down rec from standby 
 * NODE_DOWN_LIST.
 */

void clms_remove_node_down_rec(SaClmNodeIdT node_id)
{
	NODE_DOWN_LIST *node_down_rec = clms_cb->node_down_list_head;
	NODE_DOWN_LIST *prev_rec = NULL;

	while (node_down_rec) {
		if (node_down_rec->node_id == node_id) {
			/* Remove the node down  entry */
			/* Reset pointers */
			if (node_down_rec == clms_cb->node_down_list_head) {	/* 1st in the list? */
				if (node_down_rec->next == NULL) {
					/* Only one in the list? */
					clms_cb->node_down_list_head = NULL;	/* Clear head sublist pointer */
					clms_cb->node_down_list_tail = NULL;	/* Clear tail sublist pointer */
				} else {
					/* 1st but not only one */
					clms_cb->node_down_list_head = node_down_rec->next;	/* Move next one up */
				}
			} else {
				if (prev_rec) {
					if (node_down_rec->next == NULL)
						clms_cb->node_down_list_tail = prev_rec;
					prev_rec->next = node_down_rec->next;	/* Link previous to next */
				}
			}

			/* Free the NODE_DOWN_REC */
			free(node_down_rec);
			node_down_rec = NULL;
			break;
		}
		prev_rec = node_down_rec;	/* Remember address of this entry */
		node_down_rec = node_down_rec->next;	/* Go to next entry */
	}
}

/**
 * This is the function which is called to destroy an event.
 * 
 * @param[in] evt : Event to destroy 
 * @return void
 */
void clms_evt_destroy(CLMSV_CLMS_EVT * evt)
{
	assert(evt != NULL);
	if (evt->info.msg.info.api_info.type == CLMSV_CLUSTER_JOIN_REQ) {
		TRACE("not calloced in server code,don't free it here");
	} else
		free(evt);
}

/*clms ack to response msg from clma
* This is used to avoid mds timeout of saClmResponse
* @param[in] clms evt,cb,ais rc
* @return uns32 
*/

static uns32 clms_ack_to_response_msg(CLMS_CB * cb, CLMSV_CLMS_EVT * evt, SaAisErrorT ais_rc)
{
	uns32 mds_rc = NCSCC_RC_SUCCESS;
	CLMSV_MSG msg;

	TRACE_ENTER();

	msg.evt_type = CLMSV_CLMS_TO_CLMA_API_RESP_MSG;
	msg.info.api_resp_info.type = CLMSV_RESPONSE_RESP;
	msg.info.api_resp_info.rc = ais_rc;
	mds_rc = clms_mds_msg_send(cb, &msg, &evt->fr_dest, &evt->mds_ctxt, MDS_SEND_PRIORITY_HIGH, NCSMDS_SVC_ID_CLMA);
	if (mds_rc != NCSCC_RC_SUCCESS) {
		TRACE_LEAVE2("clms_mds_msg_send failed rc = %u", (unsigned int)mds_rc);
		return mds_rc;
	}
	return mds_rc;
}
