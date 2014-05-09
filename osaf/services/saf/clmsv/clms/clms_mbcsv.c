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

#include <ncsencdec_pub.h>
#include "clms.h"
#include "clms_mbcsv.h"
#include "clms_evt.h"

static uint32_t ckpt_proc_cluster_rec(CLMS_CB * cb, CLMS_CKPT_REC * data);
static uint32_t ckpt_proc_reg_rec(CLMS_CB * cb, CLMS_CKPT_REC * data);
static uint32_t ckpt_proc_finalize_rec(CLMS_CB * cb, CLMS_CKPT_REC * data);
static uint32_t ckpt_proc_track_changes_rec(CLMS_CB * cb, CLMS_CKPT_REC * data);
static uint32_t ckpt_proc_node_csync_rec(CLMS_CB * cb, CLMS_CKPT_REC * data);
static uint32_t ckpt_proc_node_config_rec(CLMS_CB * cb, CLMS_CKPT_REC * data);
static uint32_t ckpt_proc_node_rec(CLMS_CB * cb, CLMS_CKPT_REC * data);
static uint32_t ckpt_proc_node_del_rec(CLMS_CB * cb, CLMS_CKPT_REC * data);
static uint32_t ckpt_proc_agent_down_rec(CLMS_CB * cb, CLMS_CKPT_REC * data);
static uint32_t ckpt_proc_node_down_rec(CLMS_CB * cb, CLMS_CKPT_REC * data);
/* Common Callback interface to mbcsv */
static uint32_t mbcsv_callback(NCS_MBCSV_CB_ARG *arg);
static uint32_t ckpt_encode_cbk_handler(NCS_MBCSV_CB_ARG *cbk_arg);
static uint32_t ckpt_enc_cold_sync_data(CLMS_CB * clms_cb, NCS_MBCSV_CB_ARG *cbk_arg, bool data_req);
uint32_t clms_cold_sync(NCS_UBAID *uba);
uint32_t encode_client_rec(NCS_UBAID *uba);
uint32_t encode_node_rec(NCS_UBAID *uba);
uint32_t encode_cluster_rec(NCS_UBAID *uba);
uint32_t cluster_rec_(NCS_UBAID *uba);
static uint32_t ckpt_encode_async_update(CLMS_CB * clms_cb, NCS_MBCSV_CB_ARG *cbk_arg);
static void enc_ckpt_header(uint8_t *pdata, CLMSV_CKPT_HEADER header);
uint32_t enc_mbcsv_cluster_rec_msg(NCS_UBAID *uba, CLMSV_CKPT_CLUSTER_INFO * param);
uint32_t enc_mbcsv_node_down_rec_msg(NCS_UBAID *uba, CLMSV_CKPT_NODE_DOWN_INFO * param);
uint32_t enc_mbcsv_client_rec_msg(NCS_UBAID *uba, CLMSV_CKPT_CLIENT_INFO * param);
uint32_t enc_mbcsv_client_msg(NCS_UBAID *uba, CLMSV_CKPT_CLIENT_INFO * param);
uint32_t enc_mbcsv_finalize_msg(NCS_UBAID *uba, CLMSV_CKPT_FINALIZE_INFO * param);
uint32_t enc_mbcsv_track_changes_msg(NCS_UBAID *uba, CLMSV_CKPT_CLIENT_INFO * param);
uint32_t enc_mbcsv_node_rec_msg(NCS_UBAID *uba, CLMSV_CKPT_NODE_RUNTIME_INFO * param);
uint32_t enc_mbcsv_node_config_msg(NCS_UBAID *uba, CLMSV_CKPT_NODE_CONFIG_REC * param);
uint32_t enc_mbcsv_node_msg(NCS_UBAID *uba, CLMSV_CKPT_NODE * param);
uint32_t enc_mbcsv_node_del_msg(NCS_UBAID *uba, CLMSV_CKPT_NODE_DEL_REC * param);
uint32_t enc_mbcsv_agent_down_msg(NCS_UBAID *uba, CLMSV_CKPT_AGENT_DOWN_REC * param);
static uint32_t ckpt_decode_cbk_handler(NCS_MBCSV_CB_ARG *cbk_arg);
static uint32_t ckpt_decode_cold_sync(CLMS_CB * cb, NCS_MBCSV_CB_ARG *cbk_arg);
static uint32_t ckpt_decode_async_update(CLMS_CB * cb, NCS_MBCSV_CB_ARG *cbk_arg);
uint32_t decode_cluster_msg(NCS_UBAID *uba, CLMSV_CKPT_CLUSTER_INFO * param);
uint32_t decode_client_rec_msg(NCS_UBAID *uba, CLMSV_CKPT_CLIENT_INFO * param);
static uint32_t decode_agent_down_msg(NCS_UBAID *uba, CLMSV_CKPT_AGENT_DOWN_REC * param);
static uint32_t decode_node_down_msg(NCS_UBAID *uba, CLMSV_CKPT_NODE_DOWN_INFO * param);
static uint32_t decode_client_msg(NCS_UBAID *uba, CLMSV_CKPT_CLIENT_INFO * param);
static uint32_t decode_finalize_msg(NCS_UBAID *uba, CLMSV_CKPT_FINALIZE_INFO * param);
static uint32_t decode_track_changes_msg(NCS_UBAID *uba, CLMSV_CKPT_CLIENT_INFO * param);
static uint32_t decode_node_rec_msg(NCS_UBAID *uba, CLMSV_CKPT_NODE_RUNTIME_INFO * param);
uint32_t decode_node_config_msg(NCS_UBAID *uba, CLMSV_CKPT_NODE_CONFIG_REC * param);
uint32_t decode_node_del_msg(NCS_UBAID *uba, CLMSV_CKPT_NODE_DEL_REC * param);
uint32_t decode_node_msg(NCS_UBAID *uba, CLMSV_CKPT_NODE * param);
static uint32_t decode_ckpt_hdr(NCS_UBAID *uba, CLMSV_CKPT_HEADER * param);
static uint32_t ckpt_peer_info_cbk_handler(NCS_MBCSV_CB_ARG *arg);
static uint32_t ckpt_notify_cbk_handler(NCS_MBCSV_CB_ARG *arg);
static uint32_t ckpt_err_ind_cbk_handler(NCS_MBCSV_CB_ARG *arg);

static CLMS_CKPT_HDLR ckpt_data_handler[CLMS_CKPT_MSG_MAX] = {
	ckpt_proc_cluster_rec,	/*CLMS_CKPT_CLUSTER_REC */
	ckpt_proc_reg_rec,	/* CLMS_CKPT_CLIENT_INFO_REC */
	ckpt_proc_finalize_rec,	/*CLMS_CKPT_FINALIZE_REC */
	ckpt_proc_track_changes_rec,	/*CLMS_CKPT_TRACK_CHANGES_REC */
	ckpt_proc_node_csync_rec,	/*CLMS_CKPT_NODE_REC */
	ckpt_proc_node_config_rec,	/*CLMS_CKPT_NODE_CONFIG_REC */
	ckpt_proc_node_rec,	/*CLMS_CKPT_NODE_RUNTIME_REC */
	ckpt_proc_node_del_rec,	/* CLMS_CKPT_NODE_DEL_REC */
	ckpt_proc_agent_down_rec,	/*CLMS_CKPT_AGENT_DOWN_REC */
	ckpt_proc_node_down_rec	/*CLMS_CKPT_NODE_DOWN_REC */
};

/****************************************************************************
 * Name          : ckpt_proc_reg_rec
 *
 * Description   : This function updates the standby client info based on the 
 *                 info received from the ACTIVE clms peer.
 *
 * Arguments     : cb - pointer to CLMS  ControlBlock.
 *                 data - pointer to  CLMS_CHECKPOINT_DATA. 
 * 
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : None.
 ****************************************************************************/

static uint32_t ckpt_proc_cluster_rec(CLMS_CB * cb, CLMS_CKPT_REC * data)
{
	CLMSV_CKPT_CLUSTER_INFO *param = &data->param.cluster_rec;

	osaf_cluster->num_nodes = param->num_nodes;
	osaf_cluster->init_time = param->init_time;
	cb->cluster_view_num = param->cluster_view_num;

	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
 * Name          : ckpt_proc_reg_rec
 *
 * Description   : This function updates the standby client info based on the 
 *                 info received from the ACTIVE clms peer.
 *
 * Arguments     : cb - pointer to CLMS  ControlBlock.
 *                 data - pointer to  CLMS_CHECKPOINT_DATA. 
 * 
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : None.
 ****************************************************************************/

static uint32_t ckpt_proc_reg_rec(CLMS_CB * cb, CLMS_CKPT_REC * data)
{
	CLMSV_CKPT_CLIENT_INFO *param = &data->param.client_rec;
	CLMS_CLIENT_INFO *client = NULL;

	TRACE_ENTER2("client ID: %d", param->client_id);

	client = clms_client_get_by_id(param->client_id);
	if (client == NULL) {
		/* Client does not exist, create new one */
		if ((client = clms_client_new(param->mds_dest, param->client_id)) == NULL) {
			LOG_ER("new client addtion failed on standby");
			osafassert(0);
		}
		client->track_flags = param->track_flags;
	} else {
		/* Client with ID already exist, check other attributes */
		if (client->mds_dest != param->mds_dest) {
			/* Do not allow standby to get out of sync */
			osafassert(0);
		}
	}

	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
 * Name          : ckpt_proc_finalize_rec
 *
 * Description   : This function updates the standby client info based on the 
 *                 info received from the ACTIVE clms peer.
 *
 * Arguments     : cb - pointer to CLMS  ControlBlock.
 *                 data - pointer to  CLMS_CHECKPOINT_DATA. 
 * 
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : None.
 ****************************************************************************/

static uint32_t ckpt_proc_finalize_rec(CLMS_CB * cb, CLMS_CKPT_REC * data)
{
	CLMSV_CKPT_FINALIZE_INFO *param = &data->param.finalize_rec;
	uint32_t rc;

	TRACE_ENTER2("client: %d", param->client_id);
	if (!param->client_id) {
		TRACE("FAILED client_id = 0");
		TRACE_LEAVE();
		return NCSCC_RC_FAILURE;
	}
	/* Free all resources allocated by this client. */
	if ((rc = clms_client_delete(param->client_id)) != 0) {
		LOG_ER("clms_client_delete FAILED: %u", rc);
		return SA_AIS_ERR_BAD_HANDLE;
	}

	/* Delete this client data from the clmresp tracking list */
	rc = clms_client_del_trackresp(param->client_id);
	if (rc != NCSCC_RC_SUCCESS) {
		LOG_ER("clms_client_delete_trackresp FAILED: %u", rc);
		return SA_AIS_ERR_BAD_HANDLE;
	}

	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
 * Name          : ckpt_proc_track_start_rec
 *
 * Description   : This function updates the standby client info based on the 
 *                 info received from the ACTIVE clms peer.
 *
 * Arguments     : cb - pointer to CLMS  ControlBlock.
 *                 data - pointer to  CLMS_CHECKPOINT_DATA. 
 * 
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : None.
 ****************************************************************************/

static uint32_t ckpt_proc_track_changes_rec(CLMS_CB * cb, CLMS_CKPT_REC * data)
{
	CLMSV_CKPT_CLIENT_INFO *param = &data->param.client_rec;
	CLMS_CLIENT_INFO *client;

	TRACE_ENTER2("client: %d", param->client_id);
	if (!param->client_id) {
		TRACE("FAILED client_id = 0");
		TRACE_LEAVE();
		return NCSCC_RC_FAILURE;
	}

	client = clms_client_get_by_id(param->client_id);
	if (client == NULL) {
		TRACE("Bad client %d", param->client_id);
		return NCSCC_RC_FAILURE;
	}
	/*Update the client database for the current trackflags */
	client->track_flags = param->track_flags;

	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
 * Name          : ckpt_proc_node_csync_rec
 *
 * Description   : This function updates the standby client info based on the 
 *                 info received from the ACTIVE clms peer.
 *
 * Arguments     : cb - pointer to CLMS  ControlBlock.
 *                 data - pointer to  CLMS_CHECKPOINT_DATA. 
 * 
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : None.
 ****************************************************************************/

static uint32_t ckpt_proc_node_csync_rec(CLMS_CB * cb, CLMS_CKPT_REC * data)
{
	CLMSV_CKPT_NODE *param = &data->param.node_csync_rec;
	CLMS_CLUSTER_NODE *node = NULL, *tmp_node = NULL;
	uint32_t rc = NCSCC_RC_SUCCESS;

	TRACE_ENTER2("node_name:%s", param->node_name.value);

	node = clms_node_get_by_name(&param->node_name);
	if (node != NULL) {
		prepare_cluster_node(node, param);
		if (node->node_id != 0) {
			tmp_node = clms_node_get_by_id(node->node_id);
			if (tmp_node == NULL)
				if (NCSCC_RC_SUCCESS != (rc = clms_node_add(node, 0))) {
					LOG_ER("Patricia add failed");
				}
		}
	} else {
		node = (CLMS_CLUSTER_NODE *)malloc(sizeof(CLMS_CLUSTER_NODE));
		if (node == NULL){
			LOG_ER("Malloc failed for cluster node");
			osafassert(0);
		}
		memset(node,0,sizeof(CLMS_CLUSTER_NODE));
		prepare_cluster_node(node, param);      
		/*Init patricia trackresp for each node */
		clms_trackresp_patricia_init(node);	
		clms_node_add_to_model(node);
		if (node->node_id != 0) {
			tmp_node = clms_node_get_by_id(node->node_id);
			if (tmp_node == NULL)
				if (NCSCC_RC_SUCCESS != (rc = clms_node_add(node, 0))) {
					LOG_ER("Patricia add failed");
				}
		}
	}
	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
 * Name          : ckpt_proc_node_del_rec
 *
 * Description   : This function updates the standby client info based on the 
 *                 info received from the ACTIVE clms peer.
 *
 * Arguments     : cb - pointer to CLMS  ControlBlock.
 *                 data - pointer to  CLMS_CHECKPOINT_DATA. 
 * 
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : None.
 ****************************************************************************/

static uint32_t ckpt_proc_node_del_rec(CLMS_CB * cb, CLMS_CKPT_REC * data)
{
	CLMSV_CKPT_NODE_DEL_REC *param = &data->param.node_del_rec;
	CLMS_CLUSTER_NODE *node = NULL;
#ifdef ENABLE_AIS_PLM
	SaNameT *entityNames = NULL;
	SaAisErrorT rc = SA_AIS_OK;
#endif
	/*dude do you wanna have extra chekc in NULL case */
	/*Delete it from the plm entity group */
	if ((node = clms_node_get_by_name(&param->node_name)) == NULL)
		return NCSCC_RC_FAILURE;
	clms_node_delete(node, 0);
	clms_node_delete(node, 1);
	clms_node_delete(node, 2);
#ifdef ENABLE_AIS_PLM
	/*Delete it from the plm entity group */
	entityNames = &node->ee_name;
	if(clms_cb->reg_with_plm == SA_TRUE) {
		rc = saPlmEntityGroupRemove(clms_cb->ent_group_hdl, entityNames,1);
		if (rc != SA_AIS_OK) {
			LOG_ER("saPlmEntityGroupAdd FAILED rc = %d", rc);
			return rc;
		}
	}
#endif
	free(node);

	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

static uint32_t ckpt_proc_node_down_rec(CLMS_CB * cb, CLMS_CKPT_REC * data)
{

	SaClmNodeIdT node_id = data->param.node_down_rec.node_id;
	CLMS_CLUSTER_NODE *node = NULL;

	TRACE_ENTER();

	node = clms_node_get_by_id(node_id);
	if (node == NULL) {
		LOG_ER("Standby node is out of sync");
		osafassert(0);
	}

	/* Delete the node reference from the nodeid database */
	if (clms_node_delete(node, 0) != NCSCC_RC_SUCCESS) {
		LOG_ER("CLMS node delete by nodeid failed");
	}

	/*Remove node_down_rec from the standby node_down_list */
	clms_remove_node_down_rec(node_id);

	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
 * Name          : ckpt_proc_agent_down_rec
 *
 * Description   : This function updates the standby client info based on the 
 *                 info received from the ACTIVE clms peer.
 *
 * Arguments     : cb - pointer to CLMS  ControlBlock.
 *                 data - pointer to  CLMS_CHECKPOINT_DATA. 
 * 
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : None.
 ****************************************************************************/

static uint32_t ckpt_proc_agent_down_rec(CLMS_CB * cb, CLMS_CKPT_REC * data)
{
	CLMSV_CKPT_AGENT_DOWN_REC *param = &data->param.agent_rec;

	clms_remove_clma_down_rec(cb, param->mds_dest);
	/* Remove this CLMA entry from our processing lists */
	(void)clms_client_delete_by_mds_dest(param->mds_dest);

	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
 * Name          : ckpt_proc_node_config_rec
 *
 * Description   : This function updates the standby client info based on the 
 *                 info received from the ACTIVE clms peer.
 *
 * Arguments     : cb - pointer to CLMS  ControlBlock.
 *                 data - pointer to  CLMS_CHECKPOINT_DATA. 
 * 
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : None.
 ****************************************************************************/

static uint32_t ckpt_proc_node_config_rec(CLMS_CB * cb, CLMS_CKPT_REC * data)
{
	CLMSV_CKPT_NODE_CONFIG_REC *param = &data->param.node_config_rec;
	CLMS_CLUSTER_NODE *node = NULL;

	node = clms_node_get_by_name(&param->node_name);
	if (node == NULL) {
		LOG_ER("Node is NULL, Standby is out of sync.");
		/* TBD. Introduce warmsyncs */
		osafassert(0);
	}

	node->node_name.length = param->node_name.length;
	(void)memcpy(node->node_name.value, param->node_name.value, param->node_name.length);
	node->ee_name.length = param->ee_name.length;
	(void)memcpy(node->ee_name.value, param->ee_name.value, param->ee_name.length);
	node->node_addr.family = param->node_addr.family;
	node->node_addr.length = param->node_addr.length;
	(void)memcpy(node->node_addr.value, param->node_addr.value, param->node_addr.length);
	node->disable_reboot = param->disable_reboot;
	node->lck_cbk_timeout = param->lck_cbk_timeout;
	node->admin_state = param->admin_state;

	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
 * Name          : ckpt_proc_node_rec
 *
 * Description   : This function updates the standby client info based on the 
 *                 info received from the ACTIVE clms peer.
 *
 * Arguments     : cb - pointer to CLMS  ControlBlock.
 *                 data - pointer to  CLMS_CHECKPOINT_DATA. 
 * 
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : None.
 ****************************************************************************/

static uint32_t ckpt_proc_node_rec(CLMS_CB * cb, CLMS_CKPT_REC * data)
{
	CLMSV_CKPT_NODE_RUNTIME_INFO *param = &data->param.node_rec;
	CLMS_CLUSTER_NODE *node = NULL;
	IPLIST *ip = NULL;

	TRACE_ENTER2("node_id %u", param->node_id);

	node = clms_node_get_by_name(&param->node_name);

	if (node == NULL) {
		LOG_ER("Node is NULL,problem with the database.");
		osafassert(0);
	}

	node->node_id = param->node_id;
	node->node_name.length = param->node_name.length;
	(void)memcpy(node->node_name.value, param->node_name.value, node->node_name.length);
	node->member = param->member;
	node->boot_time = param->boot_time;
	node->init_view = param->init_view;
	node->admin_state = param->admin_state;
	node->admin_op = param->admin_op;
	node->change = param->change;
	node->nodeup = param->nodeup;

#ifdef ENABLE_AIS_PLM
	node->ee_red_state = param->ee_red_state;
#endif

	if (node->node_id != 0){
		if (NULL ==  clms_node_get_by_id(node->node_id)) {

			if (clms_node_add(node, 0) != NCSCC_RC_SUCCESS) {
				LOG_ER("Patricia tree add failed");
			}
		}
	}

	/* Update the node with ipaddress information */
	if ((ip = (IPLIST *)ncs_patricia_tree_get(&clms_cb->iplist, (uint8_t *)&node->node_id)) == NULL) {
		LOG_NO("IP information not found for: %u", node->node_id);
	} else {
		if (ip->addr.length) {
			node->node_addr.family = ip->addr.family;
			node->node_addr.length = ip->addr.length;
			memcpy(node->node_addr.value, ip->addr.value, ip->addr.length);
		} else {
			node->node_addr.family = 1; /* For backward compatibility */
			node->node_addr.length = 0;
		}
	}

	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
 * Name          : process_ckpt_data
 *
 * Description   : This function updates the ntfs internal databases
 *                 based on the data type. 
 *
 * Arguments     : cb - pointer to CLMS ControlBlock. 
 *                 data - pointer to  CLMS_CHECKPOINT_DATA. 
 * 
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : None.
 *****************************************************************************/

static uint32_t process_ckpt_data(CLMS_CB * cb, CLMS_CKPT_REC * data)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	if ((!cb) || (data == NULL)) {
		TRACE("FAILED: (!cb) || (data == NULL)");
		return (rc = NCSCC_RC_FAILURE);
	}

	TRACE_ENTER2("data->header.type %d", data->header.type);

	if ((cb->ha_state == SA_AMF_HA_STANDBY) || (cb->ha_state == SA_AMF_HA_QUIESCED)) {
		if (data->header.type >= CLMS_CKPT_MSG_MAX) {
			TRACE("FAILED: data->header.type >= CLMS_CKPT_MSG_MAX");
			return NCSCC_RC_FAILURE;
		}
		/* Update the internal database */
		rc = ckpt_data_handler[data->header.type] (cb, data);
		return rc;
	} else {
		return (rc = NCSCC_RC_FAILURE);
	}

	TRACE_LEAVE();
}	/*End process_ckpt_data() */

/****************************************************************************
 * Name          : clms_mbcsv_init 
 *
 * Description   : This function initializes the mbcsv interface and
 *                 obtains a selection object from mbcsv.
 *                 
 * Arguments     : CLMS_CB * - A pointer to the clms control block.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : None.
 *****************************************************************************/
uint32_t clms_mbcsv_init(CLMS_CB * cb)
{
	uint32_t rc;
	NCS_MBCSV_ARG arg;

	TRACE_ENTER();

	/* Initialize with MBCSv library */
	arg.i_op = NCS_MBCSV_OP_INITIALIZE;
	arg.info.initialize.i_mbcsv_cb = mbcsv_callback;
	arg.info.initialize.i_version = CLMS_MBCSV_VERSION;
	arg.info.initialize.i_service = NCS_SERVICE_ID_CLMS;

	if ((rc = ncs_mbcsv_svc(&arg)) != NCSCC_RC_SUCCESS) {
		LOG_ER("NCS_MBCSV_OP_INITIALIZE FAILED");
		goto done;
	}

	cb->mbcsv_hdl = arg.info.initialize.o_mbcsv_hdl;

	/* Open a checkpoint */
	arg.i_op = NCS_MBCSV_OP_OPEN;
	arg.i_mbcsv_hdl = cb->mbcsv_hdl;
	arg.info.open.i_pwe_hdl = (uint32_t)cb->mds_hdl;
	arg.info.open.i_client_hdl = 0;

	if ((rc = ncs_mbcsv_svc(&arg) != NCSCC_RC_SUCCESS)) {
		LOG_ER("NCS_MBCSV_OP_OPEN FAILED");
		goto done;
	}
	cb->mbcsv_ckpt_hdl = arg.info.open.o_ckpt_hdl;

	/* Get Selection Object */
	arg.i_op = NCS_MBCSV_OP_SEL_OBJ_GET;
	arg.i_mbcsv_hdl = cb->mbcsv_hdl;
	arg.info.sel_obj_get.o_select_obj = 0;
	if (NCSCC_RC_SUCCESS != (rc = ncs_mbcsv_svc(&arg))) {
		LOG_ER("NCS_MBCSV_OP_SEL_OBJ_GET FAILED");
		goto done;
	}

	cb->mbcsv_sel_obj = arg.info.sel_obj_get.o_select_obj;
	cb->ckpt_state = COLD_SYNC_IDLE;

	/* Disable warm sync */
	/* TBD: We need swarm sync, sodisabiling is not required */
	arg.i_op = NCS_MBCSV_OP_OBJ_SET;
	arg.i_mbcsv_hdl = cb->mbcsv_hdl;
	arg.info.obj_set.i_ckpt_hdl = cb->mbcsv_ckpt_hdl;
	arg.info.obj_set.i_obj = NCS_MBCSV_OBJ_WARM_SYNC_ON_OFF;
	arg.info.obj_set.i_val = false;
	if (ncs_mbcsv_svc(&arg) != NCSCC_RC_SUCCESS) {
		LOG_ER("NCS_MBCSV_OP_OBJ_SET FAILED");
		goto done;
	}

	rc = clms_mbcsv_change_HA_state(cb);

	/* Set MBCSV role here itself */
 done:
	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
 * Name          : clms_mbcsv_change_HA_state 
 *
 * Description   : This function inform mbcsv of our HA state. 
 *                 All checkpointing operations are triggered based on the 
 *                 state.
 *
 * Arguments     : NTFS_CB * - A pointer to the ntfs control block.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : This function should be ideally called only once, i.e.
 *                 during the first CSI assignment from AVSv  .
 *****************************************************************************/

uint32_t clms_mbcsv_change_HA_state(CLMS_CB * cb)
{
	NCS_MBCSV_ARG mbcsv_arg;
	uint32_t rc = SA_AIS_OK;
	TRACE_ENTER();
	memset(&mbcsv_arg, '\0', sizeof(NCS_MBCSV_ARG));

	/* Set the mbcsv args */
	mbcsv_arg.i_op = NCS_MBCSV_OP_CHG_ROLE;
	mbcsv_arg.i_mbcsv_hdl = cb->mbcsv_hdl;
	mbcsv_arg.info.chg_role.i_ckpt_hdl = cb->mbcsv_ckpt_hdl;
	mbcsv_arg.info.chg_role.i_ha_state = cb->ha_state;

	if (SA_AIS_OK != (rc = ncs_mbcsv_svc(&mbcsv_arg))) {
		LOG_ER("ncs_mbcsv_svc FAILED");
		rc = NCSCC_RC_FAILURE;
	}

	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
 * Name          : clms_mbcsv_dispatch
 *
 * Description   : dispatch all mbcsv events 
 *
 * Arguments     : NCS_MBCSV_HDL - Handle provided by MBCSV during op_init. 
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : This function should be ideally called only once, i.e.
 *                 during the first CSI assignment from AVSv  .
 *****************************************************************************/
uint32_t clms_mbcsv_dispatch(NCS_MBCSV_HDL mbcsv_hdl)
{
	NCS_MBCSV_ARG mbcsv_arg;

	memset(&mbcsv_arg, 0, sizeof(NCS_MBCSV_ARG));
	mbcsv_arg.i_op = NCS_MBCSV_OP_DISPATCH;
	mbcsv_arg.i_mbcsv_hdl = mbcsv_hdl;
	mbcsv_arg.info.dispatch.i_disp_flags = SA_DISPATCH_ALL;

	return ncs_mbcsv_svc(&mbcsv_arg);
}

/****************************************************************************
 * Name          : mbcsv_callback
 *
 * Description   : This callback is the single entry point for mbcsv to 
 *                 notify clms of all checkpointing operations. 
 *
 * Arguments     : NCS_MBCSV_CB_ARG - Callback Info pertaining to the mbcsv
 *                 event from ACTIVE/STANDBY CLMS peer.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : Based on the mbcsv message type, the corresponding mbcsv
 *                 message handler shall be invoked.
 *****************************************************************************/
static uint32_t mbcsv_callback(NCS_MBCSV_CB_ARG *arg)
{
	uint32_t rc = NCSCC_RC_SUCCESS;

	TRACE_ENTER();

	osafassert(arg != NULL);

	switch (arg->i_op) {
	case NCS_MBCSV_CBOP_ENC:
		/* Encode Request from MBCSv */
		rc = ckpt_encode_cbk_handler(arg);
		break;
	case NCS_MBCSV_CBOP_DEC:
		/* Decode Request from MBCSv */
		rc = ckpt_decode_cbk_handler(arg);
		if (rc != NCSCC_RC_SUCCESS)
			TRACE("ckpt_decode_cbk_handler FAILED");
		break;
	case NCS_MBCSV_CBOP_PEER:
		/* CLMS Peer info from MBCSv */
		rc = ckpt_peer_info_cbk_handler(arg);
		if (rc != NCSCC_RC_SUCCESS)
			LOG_ER("ckpt_peer_info_cbk_handler FAILED");
		break;
	case NCS_MBCSV_CBOP_NOTIFY:
		/* NOTIFY info from CLMS peer */
		rc = ckpt_notify_cbk_handler(arg);
		if (rc != NCSCC_RC_SUCCESS)
			LOG_ER("ckpt_notify_cbk_handler FAILED");
		break;
	case NCS_MBCSV_CBOP_ERR_IND:
		/* Peer error indication info */
		rc = ckpt_err_ind_cbk_handler(arg);
		if (rc != NCSCC_RC_SUCCESS)
			LOG_ER("ckpt_err_ind_cbk_handler FAILED");
		break;
	default:
		rc = NCSCC_RC_FAILURE;
		if (rc != NCSCC_RC_SUCCESS)
			TRACE("default FAILED");
		break;
	}

	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
 * Name          : clms_send_async_update
 *
 * Description   : This function makes a request to MBCSV to send an async
 *                 update to the STANDBY CLMS for the record held at
 *                 the address i_reo_hdl.
 *
 * Arguments     : cb - A pointer to the clms control block.
 *                 ckpt_rec - pointer to the checkpoint record to be
 *                 sent as an async update.
 *                 action - type of async update to indiciate whether
 *                 this update is for addition, deletion or modification of
 *                 the record being sent.
 *      
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : MBCSV, inturn calls our encode callback for this async
 *                 update. We use the reo_hdl in the encode callback to
 *                 retrieve the record for encoding the same.
 *****************************************************************************/

uint32_t clms_send_async_update(CLMS_CB * cb, CLMS_CKPT_REC * ckpt_rec, uint32_t action)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	NCS_MBCSV_ARG mbcsv_arg;
	TRACE_ENTER();
	/* Fill mbcsv specific data */
	memset(&mbcsv_arg, '\0', sizeof(NCS_MBCSV_ARG));
	mbcsv_arg.i_op = NCS_MBCSV_OP_SEND_CKPT;
	mbcsv_arg.i_mbcsv_hdl = cb->mbcsv_hdl;
	mbcsv_arg.info.send_ckpt.i_action = action;
	mbcsv_arg.info.send_ckpt.i_ckpt_hdl = (NCS_MBCSV_CKPT_HDL)cb->mbcsv_ckpt_hdl;
	mbcsv_arg.info.send_ckpt.i_reo_hdl = NCS_PTR_TO_UNS64_CAST(ckpt_rec);	/*Will be used in encode callback */

	/* Just store the address of the data to be send as an 
	 * async update record in reo_hdl. The same shall then be 
	 *dereferenced during encode callback */

	mbcsv_arg.info.send_ckpt.i_reo_type = ckpt_rec->header.type;
	mbcsv_arg.info.send_ckpt.i_send_type = NCS_MBCSV_SND_SYNC;

	/* Send async update */
	if (NCSCC_RC_SUCCESS != (rc = ncs_mbcsv_svc(&mbcsv_arg))) {
		LOG_ER(" MBCSV send data operation !! rc=%u.", rc);
		TRACE_LEAVE();
		return NCSCC_RC_FAILURE;
	}
	TRACE_LEAVE();
	return rc;
}	/*End send_async_update() */

/****************************************************************************
 * Name          : ckpt_encode_cbk_handler
 *
 * Description   : This function invokes the corresponding encode routine
 *                 based on the MBCSv encode request type.
 *
 * Arguments     : arg - Pointer to NCS_MBCSV_CB_ARG with encode info
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : None.
 *****************************************************************************/

static uint32_t ckpt_encode_cbk_handler(NCS_MBCSV_CB_ARG *cbk_arg)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	uint16_t mbcsv_version;

	TRACE_ENTER();
	osafassert(cbk_arg != NULL);

	mbcsv_version = m_NCS_MBCSV_FMT_GET(cbk_arg->info.encode.i_peer_version,
					    CLMS_MBCSV_VERSION, CLMS_MBCSV_VERSION_MIN);
	if (0 == mbcsv_version) {
		TRACE("Wrong mbcsv_version!!!\n");
		return NCSCC_RC_FAILURE;
	}
	TRACE("cbk_arg->info.encode.io_msg_type type %d", cbk_arg->info.encode.io_msg_type);
	switch (cbk_arg->info.encode.io_msg_type) {
	case NCS_MBCSV_MSG_ASYNC_UPDATE:
		/* Encode async update */
		if ((rc = ckpt_encode_async_update(clms_cb, cbk_arg)) != NCSCC_RC_SUCCESS)
			LOG_ER("  ckpt_encode_async_update FAILED");
		break;

	case NCS_MBCSV_MSG_COLD_SYNC_REQ:
		TRACE_2("COLD SYNC REQ ENCODE CALLED");
		break;

	case NCS_MBCSV_MSG_COLD_SYNC_RESP:
		/* Encode cold sync response */
		rc = ckpt_enc_cold_sync_data(clms_cb, cbk_arg, false);
		if (rc != NCSCC_RC_SUCCESS) {
			TRACE(" COLD SYNC ENCODE FAIL....");
		} else {
			clms_cb->ckpt_state = COLD_SYNC_COMPLETE;
			TRACE_2(" COLD SYNC RESPONSE SEND SUCCESS....");
		}
		break;

	case NCS_MBCSV_MSG_WARM_SYNC_REQ:
	case NCS_MBCSV_MSG_WARM_SYNC_RESP:
		break;

	case NCS_MBCSV_MSG_DATA_REQ:
		break;

	case NCS_MBCSV_MSG_DATA_RESP:
	case NCS_MBCSV_MSG_DATA_RESP_COMPLETE:
		if ((rc = ckpt_enc_cold_sync_data(clms_cb, cbk_arg, true)) != NCSCC_RC_SUCCESS)
			LOG_ER("  ckpt_enc_cold_sync_data FAILED");
		break;
	default:
		rc = NCSCC_RC_FAILURE;
		TRACE("  default FAILED");
		break;
	}			/*End switch(io_msg_type) */

	TRACE_LEAVE();
	return rc;
}	/*End ckpt_encode_cbk_handler() */

/****************************************************************************
 * Name          : ckpt_enc_cold_sync_data
 *
 * Description   : This function encodes cold sync data., viz
 *                 1. cluster info
 *                 2. node db
 *                 3. client db
		   4. async update counter
	
 *                 in that order.
 *                 Each records contain a header specifying the record type
 *                 and number of such records.
 *
 * Arguments     : clms_cb - pointer to the ntfs control block. 
 *                 cbk_arg - Pointer to NCS_MBCSV_CB_ARG with encode info.
 *                 data_req - Flag to specify if its for cold sync or data
 *                 request for warm sync.
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : None.
 *****************************************************************************/

static uint32_t ckpt_enc_cold_sync_data(CLMS_CB * clms_cb, NCS_MBCSV_CB_ARG *cbk_arg, bool data_req)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	/* asynsc Update Count */
	uint8_t *async_upd_cnt = NULL;

	/* Currently, we shall send all data in one send.
	 * This shall avoid "delta data" problems that are associated during
	 * multiple sends
	 */
	TRACE_2("COLD SYNC ENCODE START........");
	rc = clms_cold_sync(&cbk_arg->info.encode.io_uba);
	if (rc != NCSCC_RC_SUCCESS) {
		TRACE("COLD_SYNC_ENCODE FAILED");
		return NCSCC_RC_FAILURE;
	}

	/* This will have the count of async updates that have been sent,
	   this will be 0 initially */
	async_upd_cnt = ncs_enc_reserve_space(&cbk_arg->info.encode.io_uba, sizeof(uint32_t));
	if (async_upd_cnt == NULL) {
		LOG_ER("ncs_enc_reserve_space FAILED");
		return NCSCC_RC_FAILURE;
	}
	ncs_encode_32bit(&async_upd_cnt, clms_cb->async_upd_cnt);
	ncs_enc_claim_space(&cbk_arg->info.encode.io_uba, sizeof(uint32_t));

	/* Set response mbcsv msg type to complete */
	if (data_req == true)
		cbk_arg->info.encode.io_msg_type = NCS_MBCSV_MSG_DATA_RESP_COMPLETE;
	else
		cbk_arg->info.encode.io_msg_type = NCS_MBCSV_MSG_COLD_SYNC_RESP_COMPLETE;
	TRACE_2("COLD SYNC ENCODE END........");
	return rc;
}	/*End  ckpt_enc_cold_sync_data() */

uint32_t clms_cold_sync(NCS_UBAID *uba)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER();

	/*encoding cluster runtime data */
	rc = encode_cluster_rec(uba);
	if (rc != NCSCC_RC_SUCCESS) {
		TRACE("Cluster Encode Failed");
		TRACE_LEAVE();
		return NCSCC_RC_FAILURE;
	}

	/*encode node db */
	rc = encode_node_rec(uba);
	if (rc != NCSCC_RC_SUCCESS) {
		TRACE("Node Encode Failed");
		TRACE_LEAVE();
		return NCSCC_RC_FAILURE;
	}

	/*encode client db */
	rc = encode_client_rec(uba);
	if (rc != NCSCC_RC_SUCCESS) {
		TRACE("Node Encode Failed");
		TRACE_LEAVE();
		return NCSCC_RC_FAILURE;
	}
	TRACE_LEAVE();
	return rc;
}

uint32_t encode_client_rec(NCS_UBAID *uba)
{
	CLMSV_CKPT_HEADER ckpt_hdr;
	CLMSV_CKPT_CLIENT_INFO *client;
	CLMS_CLIENT_INFO *client_db = NULL;
	uint32_t client_id = 0;
	uint8_t *pheader = NULL;
	uint32_t count = 0;

	client = malloc(sizeof(CLMSV_CKPT_CLIENT_INFO));
	if (client == NULL) {
		TRACE("Malloc Failed");
		return 0;	/*dude can you return NCSCC_RC_FAILURE */
	}
	memset(client, 0, sizeof(CLMSV_CKPT_CLIENT_INFO));

	/*Reserve space for "Checkpoint Header" */
	pheader = ncs_enc_reserve_space(uba, sizeof(CLMSV_CKPT_HEADER));
	if (pheader == NULL) {
		TRACE("  ncs_enc_reserve_space FAILED");
		free(client);
		return 0;
	}
	ncs_enc_claim_space(uba, sizeof(CLMSV_CKPT_HEADER));

	while ((client_db = clms_client_getnext_by_id(client_id)) != NULL) {
		client_id = client_db->client_id;
		client->client_id = client_db->client_id;
		client->mds_dest = client_db->mds_dest;
		client->track_flags = client_db->track_flags;
		if (enc_mbcsv_client_rec_msg(uba, client) == 0) {
			TRACE("ckpt client encode failed FAILED");
			free(client);
			return 0;
		}
		++count;
	}

	ckpt_hdr.type = CLMS_CKPT_CLIENT_INFO_REC;	/*dude you need one more of this */
	ckpt_hdr.num_ckpt_records = count;
	ckpt_hdr.data_len = 0;
	enc_ckpt_header(pheader, ckpt_hdr);

	free(client);
	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

uint32_t encode_node_rec(NCS_UBAID *uba)
{
	CLMSV_CKPT_HEADER ckpt_hdr;
	CLMSV_CKPT_NODE *node = NULL;
	CLMS_CLUSTER_NODE *cluster_node = NULL;
	uint8_t *pheader = NULL;
	uint32_t count = 0;

	node = malloc(sizeof(CLMSV_CKPT_NODE));
	if (node == NULL) {
		TRACE("Malloc Failed");
		return 0;	/*dude can you return NCSCC_RC_FAILURE */
	}
	memset(node, 0, sizeof(CLMSV_CKPT_NODE));
	/*Reserve space for "Checkpoint Header" */
	pheader = ncs_enc_reserve_space(uba, sizeof(CLMSV_CKPT_HEADER));
	if (pheader == NULL) {
		TRACE("  ncs_enc_reserve_space FAILED");
		free(node);
		return 0;
	}
	ncs_enc_claim_space(uba, sizeof(CLMSV_CKPT_HEADER));

	cluster_node = (CLMS_CLUSTER_NODE *) clms_node_getnext_by_id(0);;
	while (cluster_node != NULL) {
		prepare_ckpt_node(node, cluster_node);
		if (enc_mbcsv_node_msg(uba, node) == 0) {
			TRACE("ckpt node encode failed FAILED");
			free(node);
			return 0;
		}
		++count;
		cluster_node = (CLMS_CLUSTER_NODE *) clms_node_getnext_by_id(node->node_id);
	}

	ckpt_hdr.type = CLMS_CKPT_NODE_REC;
	ckpt_hdr.num_ckpt_records = count;
	ckpt_hdr.data_len = 0;
	enc_ckpt_header(pheader, ckpt_hdr);

	free(node);
	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

void prepare_ckpt_node(CLMSV_CKPT_NODE * node, CLMS_CLUSTER_NODE * cluster_node)
{
	node->node_id = cluster_node->node_id;
	node->node_addr.family = cluster_node->node_addr.family;
	node->node_addr.length = cluster_node->node_addr.length;
	(void)memcpy(node->node_addr.value, cluster_node->node_addr.value, cluster_node->node_addr.length);
	node->node_name.length = cluster_node->node_name.length;
	(void)memcpy(node->node_name.value, cluster_node->node_name.value, cluster_node->node_name.length);
	node->ee_name.length = cluster_node->ee_name.length;
	(void)memcpy(node->ee_name.value, cluster_node->ee_name.value, cluster_node->ee_name.length);
	node->member = cluster_node->member;
	node->boot_time = cluster_node->boot_time;
	node->init_view = cluster_node->init_view;
	node->disable_reboot = cluster_node->disable_reboot;
	node->lck_cbk_timeout = cluster_node->lck_cbk_timeout;
	node->admin_state = cluster_node->admin_state;
	node->change = cluster_node->change;
#ifdef ENABLE_AIS_PLM
	node->ee_red_state = cluster_node->ee_red_state;
#endif
	node->nodeup = cluster_node->nodeup;
	node->curr_admin_inv = cluster_node->curr_admin_inv;
	node->stat_change = cluster_node->stat_change;
	node->admin_op = cluster_node->admin_op;
	node->plm_invid = cluster_node->plm_invid;
}

/*Dude - can be improved let see how*/
void prepare_cluster_node(CLMS_CLUSTER_NODE * node, CLMSV_CKPT_NODE * cluster_node)
{
	TRACE_ENTER();
	node->node_id = cluster_node->node_id;
	node->node_addr.family = cluster_node->node_addr.family;
	node->node_addr.length = cluster_node->node_addr.length;
	(void)memcpy(node->node_addr.value, cluster_node->node_addr.value, cluster_node->node_addr.length);
	node->node_name.length = cluster_node->node_name.length;
	(void)memcpy(node->node_name.value, cluster_node->node_name.value, cluster_node->node_name.length);
	node->ee_name.length = cluster_node->ee_name.length;
	(void)memcpy(node->ee_name.value, cluster_node->ee_name.value, cluster_node->ee_name.length);
	node->member = cluster_node->member;
	node->boot_time = cluster_node->boot_time;
	node->init_view = cluster_node->init_view;
	node->disable_reboot = cluster_node->disable_reboot;
	node->lck_cbk_timeout = cluster_node->lck_cbk_timeout;
	node->admin_state = cluster_node->admin_state;
	node->change = cluster_node->change;
#ifdef ENABLE_AIS_PLM
	node->ee_red_state = cluster_node->ee_red_state;
#endif
	node->nodeup = cluster_node->nodeup;
	node->curr_admin_inv = cluster_node->curr_admin_inv;
	node->stat_change = cluster_node->stat_change;
	node->admin_op = cluster_node->admin_op;
	node->plm_invid = cluster_node->plm_invid;
	TRACE_LEAVE();
}

void prepare_ckpt_to_ckpt_node(CLMSV_CKPT_NODE * node, CLMSV_CKPT_NODE * cluster_node)
{
	node->node_id = cluster_node->node_id;
	node->node_addr.family = cluster_node->node_addr.family;
	node->node_addr.length = cluster_node->node_addr.length;
	(void)memcpy(node->node_addr.value, cluster_node->node_addr.value, cluster_node->node_addr.length);
	node->node_name.length = cluster_node->node_name.length;
	(void)memcpy(node->node_name.value, cluster_node->node_name.value, cluster_node->node_name.length);
	node->ee_name.length = cluster_node->ee_name.length;
	(void)memcpy(node->ee_name.value, cluster_node->ee_name.value, cluster_node->ee_name.length);
	node->member = cluster_node->member;
	node->boot_time = cluster_node->boot_time;
	node->init_view = cluster_node->init_view;
	node->disable_reboot = cluster_node->disable_reboot;
	node->lck_cbk_timeout = cluster_node->lck_cbk_timeout;
	node->admin_state = cluster_node->admin_state;
	node->change = cluster_node->change;
#ifdef ENABLE_AIS_PLM
	node->ee_red_state = cluster_node->ee_red_state;
#endif
	node->nodeup = cluster_node->nodeup;
	node->curr_admin_inv = cluster_node->curr_admin_inv;
	node->stat_change = cluster_node->stat_change;
	node->admin_op = cluster_node->admin_op;
	node->plm_invid = cluster_node->plm_invid;
}

void prepare_ckpt_config_node(CLMSV_CKPT_NODE_CONFIG_REC * node, CLMS_CLUSTER_NODE * cluster_node)
{
	node->node_name.length = cluster_node->node_name.length;
	(void)memcpy(node->node_name.value, cluster_node->node_name.value, cluster_node->node_name.length);
	node->ee_name.length = cluster_node->ee_name.length;
	(void)memcpy(node->ee_name.value, cluster_node->ee_name.value, cluster_node->ee_name.length);
	node->node_addr.family = cluster_node->node_addr.family;
	node->node_addr.length = cluster_node->node_addr.length;
	(void)memcpy(node->node_addr.value, cluster_node->node_addr.value, cluster_node->node_addr.length);
	node->disable_reboot = cluster_node->disable_reboot;
	node->lck_cbk_timeout = cluster_node->lck_cbk_timeout;
	node->admin_state = cluster_node->admin_state;
}

void prepare_ckpt_to_ckpt_config_node(CLMSV_CKPT_NODE_CONFIG_REC * node, CLMSV_CKPT_NODE_CONFIG_REC * cluster_node)
{
	node->node_name.length = cluster_node->node_name.length;
	(void)memcpy(node->node_name.value, cluster_node->node_name.value, cluster_node->node_name.length);
	node->ee_name.length = cluster_node->ee_name.length;
	(void)memcpy(node->ee_name.value, cluster_node->ee_name.value, cluster_node->ee_name.length);
	node->node_addr.family = cluster_node->node_addr.family;
	node->node_addr.length = cluster_node->node_addr.length;
	(void)memcpy(node->node_addr.value, cluster_node->node_addr.value, cluster_node->node_addr.length);
	node->disable_reboot = cluster_node->disable_reboot;
	node->lck_cbk_timeout = cluster_node->lck_cbk_timeout;
	node->admin_state = cluster_node->admin_state;
}

uint32_t encode_cluster_rec(NCS_UBAID *uba)
{
	CLMSV_CKPT_HEADER ckpt_hdr;
	uint8_t *pheader = NULL;
	/*Reserve space for "Checkpoint Header" */
	pheader = ncs_enc_reserve_space(uba, sizeof(CLMSV_CKPT_HEADER));
	if (pheader == NULL) {
		TRACE("  ncs_enc_reserve_space FAILED");
		return 0;
	}
	ncs_enc_claim_space(uba, sizeof(CLMSV_CKPT_HEADER));

	if (!(cluster_rec_(uba))) {
		TRACE("  ncs_enc_reserve_space FAILED");
		return 0;
	}

	ckpt_hdr.type = CLMS_CKPT_CLUSTER_REC;
	ckpt_hdr.num_ckpt_records = 1;
	ckpt_hdr.data_len = 0;

	enc_ckpt_header(pheader, ckpt_hdr);
	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

uint32_t cluster_rec_(NCS_UBAID *uba)
{
	CLMSV_CKPT_CLUSTER_INFO cluster_rec;
	cluster_rec.num_nodes = osaf_cluster->num_nodes;
	cluster_rec.init_time = osaf_cluster->init_time;
	cluster_rec.cluster_view_num = clms_cb->cluster_view_num;
	if (enc_mbcsv_cluster_rec_msg(uba, &cluster_rec) == 0)
		return 0;
	return 1;

}

/****************************************************************************
 * Name          : ckpt_encode_async_update
 *
 * Description   : This function encodes data to be sent as an async update.
 *                 The caller of this function would set the address of the
 *                 record to be encoded in the reo_hdl field(while invoking
 *                 SEND_CKPT option of ncs_mbcsv_svc. 
 *
 * Arguments     : cb - pointer to the CLMS control block.
 *                 cbk_arg - Pointer to NCS MBCSV callback argument struct.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : None.
 ****************************************************************************/

static uint32_t ckpt_encode_async_update(CLMS_CB * clms_cb, NCS_MBCSV_CB_ARG *cbk_arg)
{
	CLMS_CKPT_REC *data = NULL;
	uint32_t rc = NCSCC_RC_SUCCESS, num_bytes;
	uint8_t *pheader = NULL;
	CLMSV_CKPT_HEADER ckpt_hdr;
	NCS_UBAID *uba = &cbk_arg->info.encode.io_uba;
	CLMSV_CKPT_CLIENT_INFO ckpt_client_rec;
	CLMSV_CKPT_FINALIZE_INFO ckpt_finalize_rec;
	CLMSV_CKPT_NODE_RUNTIME_INFO ckpt_node_rec;
	CLMSV_CKPT_NODE ckpt_node_csync_rec;
	CLMSV_CKPT_NODE_CONFIG_REC ckpt_node_config_rec;
	CLMSV_CKPT_NODE_DEL_REC ckpt_node_del_rec;
	CLMSV_CKPT_AGENT_DOWN_REC ckpt_agent_rec;
	CLMSV_CKPT_CLUSTER_INFO ckpt_cluster_rec;
	CLMSV_CKPT_NODE_DOWN_INFO ckpt_node_down_rec;

	TRACE_ENTER();
	/* Set reo_hdl from callback arg to ckpt_rec */
	data = (CLMS_CKPT_REC *) (long)cbk_arg->info.encode.io_reo_hdl;
	if (data == NULL) {
		TRACE("   data == NULL, FAILED");
		TRACE_LEAVE();
		return NCSCC_RC_FAILURE;
	}

	/*Reserve space for "Checkpoint Header" */
	pheader = ncs_enc_reserve_space(uba, sizeof(CLMSV_CKPT_HEADER));
	if (pheader == NULL) {
		TRACE("  ncs_enc_reserve_space FAILED");
		TRACE_LEAVE();
		return (rc = EDU_ERR_MEM_FAIL);
	}
	ncs_enc_claim_space(uba, sizeof(CLMSV_CKPT_HEADER));
	TRACE("data->header.type %d", data->header.type);

	/* Encode async record */
	switch (data->header.type) {
	case CLMS_CKPT_CLIENT_INFO_REC:

		TRACE("Async update CLMS_CKPT_CLIENT_INFO_REC");
		/* Encode RegHeader */
		ckpt_hdr.type = CLMS_CKPT_CLIENT_INFO_REC;
		ckpt_hdr.num_ckpt_records = 1;
		ckpt_hdr.data_len = 0;
		enc_ckpt_header(pheader, ckpt_hdr);

		ckpt_client_rec.client_id = data->param.client_rec.client_id;
		ckpt_client_rec.mds_dest = data->param.client_rec.mds_dest;
		num_bytes = enc_mbcsv_client_msg(uba, &ckpt_client_rec);
		if (num_bytes == 0) {
			return NCSCC_RC_FAILURE;
		}
		break;
	case CLMS_CKPT_FINALIZE_REC:

		TRACE("Async update CLMS_CKPT_FINALIZE_REC");
		/* Encode RegHeader */
		ckpt_hdr.type = CLMS_CKPT_FINALIZE_REC;
		ckpt_hdr.num_ckpt_records = 1;
		ckpt_hdr.data_len = 0;
		enc_ckpt_header(pheader, ckpt_hdr);

		ckpt_finalize_rec.client_id = data->param.finalize_rec.client_id;
		num_bytes = enc_mbcsv_finalize_msg(uba, &ckpt_finalize_rec);
		if (num_bytes == 0) {
			return NCSCC_RC_FAILURE;
		}
		break;
	case CLMS_CKPT_TRACK_CHANGES_REC:

		TRACE("Async update CLMS_CKPT_TRACK_START");
		/* Encode RegHeader */
		ckpt_hdr.type = CLMS_CKPT_TRACK_CHANGES_REC;
		ckpt_hdr.num_ckpt_records = 1;
		ckpt_hdr.data_len = 0;
		enc_ckpt_header(pheader, ckpt_hdr);

		ckpt_client_rec.client_id = data->param.client_rec.client_id;
		ckpt_client_rec.track_flags = data->param.client_rec.track_flags;
		num_bytes = enc_mbcsv_track_changes_msg(uba, &ckpt_client_rec);
		if (num_bytes == 0) {
			return NCSCC_RC_FAILURE;
		}
		break;
	case CLMS_CKPT_NODE_RUNTIME_REC:

		TRACE("Async update CLMS_CKPT_NODE_RUNTIME_REC");
		/* Encode RegHeader */
		ckpt_hdr.type = CLMS_CKPT_NODE_RUNTIME_REC;
		ckpt_hdr.num_ckpt_records = 1;
		ckpt_hdr.data_len = 0;
		enc_ckpt_header(pheader, ckpt_hdr);

		ckpt_node_rec.node_id = data->param.node_rec.node_id;
		ckpt_node_rec.node_name.length = data->param.node_rec.node_name.length;
		(void)memcpy(ckpt_node_rec.node_name.value, data->param.node_rec.node_name.value,
			     data->param.node_rec.node_name.length);
		ckpt_node_rec.member = data->param.node_rec.member;
		ckpt_node_rec.boot_time = data->param.node_rec.boot_time;	/*may be not needed */
		ckpt_node_rec.init_view = data->param.node_rec.init_view;
		ckpt_node_rec.admin_state = data->param.node_rec.admin_state;
		ckpt_node_rec.admin_op = data->param.node_rec.admin_op;
		ckpt_node_rec.change = data->param.node_rec.change;
		ckpt_node_rec.nodeup = data->param.node_rec.nodeup;
#ifdef ENABLE_AIS_PLM
		ckpt_node_rec.ee_red_state = data->param.node_rec.ee_red_state;
#endif

		num_bytes = enc_mbcsv_node_rec_msg(uba, &ckpt_node_rec);
		if (num_bytes == 0) {
			return NCSCC_RC_FAILURE;
		}
		break;
	case CLMS_CKPT_NODE_REC:

		TRACE("Async update CLMS_CKPT_NODE_REC");
		/* Encode RegHeader */
		ckpt_hdr.type = CLMS_CKPT_NODE_REC;
		ckpt_hdr.num_ckpt_records = 1;
		ckpt_hdr.data_len = 0;
		enc_ckpt_header(pheader, ckpt_hdr);

		prepare_ckpt_to_ckpt_node(&ckpt_node_csync_rec, &data->param.node_csync_rec);
		num_bytes = enc_mbcsv_node_msg(uba, &ckpt_node_csync_rec);
		if (num_bytes == 0) {
			return NCSCC_RC_FAILURE;
		}
		break;
	case CLMS_CKPT_NODE_CONFIG_REC:

		TRACE("Async update CLMS_CKPT_NODE_CONFIG_REC");
		/* Encode RegHeader */
		ckpt_hdr.type = CLMS_CKPT_NODE_CONFIG_REC;
		ckpt_hdr.num_ckpt_records = 1;
		ckpt_hdr.data_len = 0;
		enc_ckpt_header(pheader, ckpt_hdr);

		prepare_ckpt_to_ckpt_config_node(&ckpt_node_config_rec, &data->param.node_config_rec);
		num_bytes = enc_mbcsv_node_config_msg(uba, &ckpt_node_config_rec);
		if (num_bytes == 0) {
			return NCSCC_RC_FAILURE;
		}
		break;
	case CLMS_CKPT_NODE_DEL_REC:

		TRACE("Async update CLMS_CKPT_NODE_DEL_REC");
		/* Encode RegHeader */
		ckpt_hdr.type = CLMS_CKPT_NODE_DEL_REC;
		ckpt_hdr.num_ckpt_records = 1;
		ckpt_hdr.data_len = 0;
		enc_ckpt_header(pheader, ckpt_hdr);

		ckpt_node_del_rec.node_name.length = data->param.node_del_rec.node_name.length;
		(void)memcpy(ckpt_node_del_rec.node_name.value, data->param.node_del_rec.node_name.value,
			     data->param.node_del_rec.node_name.length);

		num_bytes = enc_mbcsv_node_del_msg(uba, &ckpt_node_del_rec);
		if (num_bytes == 0) {
			return NCSCC_RC_FAILURE;
		}
		break;
	case CLMS_CKPT_AGENT_DOWN_REC:
		TRACE("Async Update CLMS_CKPT_AGENT_DOWN_REC");
		/* Encode RegHeader */
		ckpt_hdr.type = CLMS_CKPT_AGENT_DOWN_REC;
		ckpt_hdr.num_ckpt_records = 1;
		ckpt_hdr.data_len = 0;
		enc_ckpt_header(pheader, ckpt_hdr);

		ckpt_agent_rec.mds_dest = data->param.agent_rec.mds_dest;
		num_bytes = enc_mbcsv_agent_down_msg(uba, &ckpt_agent_rec);
		if (num_bytes == 0) {
			return NCSCC_RC_FAILURE;
		}
		break;
	case CLMS_CKPT_CLUSTER_REC:
		TRACE("Async Update CLMS_CKPT_CLUSTER_REC");
		ckpt_hdr.type = CLMS_CKPT_CLUSTER_REC;
		ckpt_hdr.num_ckpt_records = 1;
		ckpt_hdr.data_len = 0;
		enc_ckpt_header(pheader, ckpt_hdr);

		ckpt_cluster_rec.num_nodes = data->param.cluster_rec.num_nodes;
		ckpt_cluster_rec.init_time = data->param.cluster_rec.init_time;
		ckpt_cluster_rec.cluster_view_num = data->param.cluster_rec.cluster_view_num;

		num_bytes = enc_mbcsv_cluster_rec_msg(uba, &ckpt_cluster_rec);
		if (num_bytes == 0) {
			return NCSCC_RC_FAILURE;
		}
		break;
	case CLMS_CKPT_NODE_DOWN_REC:
		TRACE("Async Update CLMS_CKPT_NODE_DOWN_REC");
		ckpt_hdr.type = CLMS_CKPT_NODE_DOWN_REC;
		ckpt_hdr.num_ckpt_records = 1;
		ckpt_hdr.data_len = 0;
		enc_ckpt_header(pheader, ckpt_hdr);

		ckpt_node_down_rec.node_id = data->param.node_down_rec.node_id;

		num_bytes = enc_mbcsv_node_down_rec_msg(uba, &ckpt_node_down_rec);
		if (num_bytes == 0) {
			return NCSCC_RC_FAILURE;
		}
		break;
	default:
		TRACE_3("FAILED no type: %d", data->header.type);
		break;
	}

	/* Update the Async Update Count at standby */
	clms_cb->async_upd_cnt++;
	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
 * Name          : clms_enc_ckpt_header
 *
 * Description   : This function encodes the checkpoint message header
 *                 using leap provided apis. 
 *
 * Arguments     : pdata - pointer to the buffer to encode this struct in. 
 *                 NTFS_CKPT_HEADER - ntfsv checkpoint message header. 
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : None.
 *****************************************************************************/

static void enc_ckpt_header(uint8_t *pdata, CLMSV_CKPT_HEADER header)
{
	ncs_encode_32bit(&pdata, header.type);
	ncs_encode_32bit(&pdata, header.num_ckpt_records);
	ncs_encode_32bit(&pdata, header.data_len);
}

uint32_t enc_mbcsv_cluster_rec_msg(NCS_UBAID *uba, CLMSV_CKPT_CLUSTER_INFO * param)
{
	uint8_t *p8;
	uint32_t total_bytes = 0;

	TRACE_ENTER();

	/** encode the contents **/
	p8 = ncs_enc_reserve_space(uba, 20);
	if (!p8) {
		TRACE("NULL pointer");
		return 0;
	}
	ncs_encode_32bit(&p8, param->num_nodes);
	ncs_encode_64bit(&p8, param->init_time);
	ncs_encode_64bit(&p8, param->cluster_view_num);
	ncs_enc_claim_space(uba, 20);
	total_bytes += 20;

	TRACE_LEAVE();
	return total_bytes;
}

uint32_t enc_mbcsv_node_down_rec_msg(NCS_UBAID *uba, CLMSV_CKPT_NODE_DOWN_INFO * param)
{
	uint8_t *p8;
	uint32_t total_bytes = 0;

	TRACE_ENTER();

	/** encode the contents **/
	p8 = ncs_enc_reserve_space(uba, 4);
	if (!p8) {
		TRACE("NULL pointer");
		return 0;
	}
	ncs_encode_32bit(&p8, param->node_id);
	ncs_enc_claim_space(uba, 4);
	total_bytes += 4;

	TRACE_LEAVE();
	return total_bytes;
}

uint32_t enc_mbcsv_client_rec_msg(NCS_UBAID *uba, CLMSV_CKPT_CLIENT_INFO * param)
{
	uint8_t *p8;
	uint32_t total_bytes = 0;
	SaUint32T mds_dest1 = 0;
	SaUint32T mds_dest2 = 0;

	mds_dest1 = (param->mds_dest & 0x00000000ffffffff);
	mds_dest2 = (param->mds_dest >> 32);

	TRACE_ENTER();

    /** encode the contents **/
	p8 = ncs_enc_reserve_space(uba, 13);
	if (!p8) {
		TRACE("NULL pointer");
		return 0;
	}
	ncs_encode_32bit(&p8, param->client_id);
	ncs_encode_32bit(&p8, mds_dest1);
	ncs_encode_32bit(&p8, mds_dest2);
	/*ncs_encode_64bit(&p8, param->mds_dest); */
	ncs_encode_8bit(&p8, param->track_flags);
	ncs_enc_claim_space(uba, 13);
	total_bytes += 13;

	TRACE_LEAVE();
	return total_bytes;
}

uint32_t enc_mbcsv_agent_down_msg(NCS_UBAID *uba, CLMSV_CKPT_AGENT_DOWN_REC * param)
{
	uint8_t *p8;
	uint32_t total_bytes = 0;
	SaUint32T mds_dest1 = 0;
	SaUint32T mds_dest2 = 0;

	TRACE_ENTER();

	mds_dest1 = (param->mds_dest & 0x00000000ffffffff);
	mds_dest2 = (param->mds_dest >> 32);

	/** encode the contents **/
	p8 = ncs_enc_reserve_space(uba, 8);
	if (!p8) {
		TRACE("NULL pointer");
		return 0;
	}
	ncs_encode_32bit(&p8, mds_dest1);
	ncs_encode_32bit(&p8, mds_dest2);
	/*ncs_encode_64bit(&p8, param->mds_dest); */
	ncs_enc_claim_space(uba, 8);
	total_bytes += 8;

	TRACE_LEAVE();
	return total_bytes;
}

uint32_t enc_mbcsv_client_msg(NCS_UBAID *uba, CLMSV_CKPT_CLIENT_INFO * param)
{
	uint8_t *p8;
	uint32_t total_bytes = 0;
	SaUint32T mds_dest1 = 0;
	SaUint32T mds_dest2 = 0;

	TRACE_ENTER();

	mds_dest1 = (param->mds_dest & 0x00000000ffffffff);
	mds_dest2 = (param->mds_dest >> 32);

    /** encode the contents **/
	p8 = ncs_enc_reserve_space(uba, 12);
	if (!p8) {
		TRACE("NULL pointer");
		return 0;
	}
	ncs_encode_32bit(&p8, param->client_id);
	ncs_encode_32bit(&p8, mds_dest1);
	ncs_encode_32bit(&p8, mds_dest2);
	/*ncs_encode_64bit(&p8, param->mds_dest); */
	ncs_enc_claim_space(uba, 12);
	total_bytes += 12;

	TRACE_LEAVE();
	return total_bytes;
}

uint32_t enc_mbcsv_finalize_msg(NCS_UBAID *uba, CLMSV_CKPT_FINALIZE_INFO * param)
{
	uint8_t *p8;
	uint32_t total_bytes = 0;

	TRACE_ENTER();

    /** encode the contents **/
	p8 = ncs_enc_reserve_space(uba, 4);
	if (!p8) {
		TRACE("NULL pointer");
		return 0;
	}
	ncs_encode_32bit(&p8, param->client_id);
	ncs_enc_claim_space(uba, 4);
	total_bytes += 4;

	TRACE_LEAVE();
	return total_bytes;
}

uint32_t enc_mbcsv_track_changes_msg(NCS_UBAID *uba, CLMSV_CKPT_CLIENT_INFO * param)
{
	uint8_t *p8;
	uint32_t total_bytes = 0;

	TRACE_ENTER();

    /** encode the contents **/
	p8 = ncs_enc_reserve_space(uba, 5);
	if (!p8) {
		TRACE("NULL pointer");
		return 0;
	}
	ncs_encode_32bit(&p8, param->client_id);
	ncs_encode_8bit(&p8, param->track_flags);
	ncs_enc_claim_space(uba, 5);
	total_bytes += 5;

	TRACE_LEAVE();
	return total_bytes;
}

uint32_t enc_mbcsv_node_rec_msg(NCS_UBAID *uba, CLMSV_CKPT_NODE_RUNTIME_INFO * param)
{
	uint8_t *p8;
	uint32_t total_bytes = 0;

	TRACE_ENTER();

    /** encode the contents **/
	p8 = ncs_enc_reserve_space(uba, 4);
	if (!p8) {
		TRACE("NULL pointer");
		return 0;
	}
	ncs_encode_32bit(&p8, param->node_id);
	ncs_enc_claim_space(uba, 4);
	total_bytes += 4;

	total_bytes += clmsv_encodeSaNameT(uba, &param->node_name);

	p8 = ncs_enc_reserve_space(uba, 4);
	if (!p8) {
		TRACE("NULL pointer");
		return 0;
	}
	ncs_encode_32bit(&p8, param->member);
	ncs_enc_claim_space(uba, 4);
	total_bytes += 4;

	p8 = ncs_enc_reserve_space(uba, 8);
	if (!p8) {
		TRACE("NULL pointer");
		return 0;
	}
	ncs_encode_64bit(&p8, param->boot_time);
	ncs_enc_claim_space(uba, 8);
	total_bytes += 8;

	p8 = ncs_enc_reserve_space(uba, 8);
	if (!p8) {
		TRACE("NULL pointer");
		return 0;
	}
	ncs_encode_64bit(&p8, param->init_view);
	ncs_enc_claim_space(uba, 8);
	total_bytes += 8;

	p8 = ncs_enc_reserve_space(uba, 4);
	if (!p8) {
		TRACE("NULL pointer");
		return 0;
	}
	ncs_encode_32bit(&p8, param->admin_state);
	ncs_enc_claim_space(uba, 4);
	total_bytes += 4;

	p8 = ncs_enc_reserve_space(uba, 4);
	if (!p8) {
		TRACE("NULL pointer");
		return 0;
	}
	ncs_encode_32bit(&p8, param->admin_op);
	ncs_enc_claim_space(uba, 4);
	total_bytes += 4;

	p8 = ncs_enc_reserve_space(uba, 4);
	if (!p8) {
		TRACE("NULL pointer");
		return 0;
	}
	ncs_encode_32bit(&p8, param->change);
	ncs_enc_claim_space(uba, 4);
	total_bytes += 4;

	p8 = ncs_enc_reserve_space(uba, 4);
	if (!p8) {
		TRACE("NULL pointer");
		return 0;
	}
	ncs_encode_32bit(&p8, param->nodeup);
	ncs_enc_claim_space(uba, 4);
	total_bytes += 4;

#ifdef ENABLE_AIS_PLM
	p8 = ncs_enc_reserve_space(uba, 4);
	if (!p8) {
		TRACE("NULL pointer");
		return 0;
	}
	ncs_encode_32bit(&p8, param->ee_red_state);
	ncs_enc_claim_space(uba, 4);
	total_bytes += 4;
#endif

	TRACE_LEAVE();
	return total_bytes;
}

uint32_t enc_mbcsv_node_config_msg(NCS_UBAID *uba, CLMSV_CKPT_NODE_CONFIG_REC * param)
{
	uint8_t *p8;
	uint32_t total_bytes = 0;
	SaUint32T lck_timeout1 = 0;
	SaUint32T lck_timeout2 = 0;

	lck_timeout1 = (param->lck_cbk_timeout & 0x00000000ffffffff);
	lck_timeout2 = (param->lck_cbk_timeout >> 32);

	TRACE_ENTER();

    /** encode the contents **/
	total_bytes += clmsv_encodeSaNameT(uba, &param->node_name);
	total_bytes += clmsv_encodeSaNameT(uba, &param->ee_name);
	total_bytes += encodeNodeAddressT(uba, &param->node_addr);

	p8 = ncs_enc_reserve_space(uba, 4);
	if (!p8) {
		TRACE("NULL pointer");
		return 0;
	}
	ncs_encode_32bit(&p8, param->disable_reboot);
	ncs_enc_claim_space(uba, 4);
	total_bytes += 4;

	p8 = ncs_enc_reserve_space(uba, 8);
	if (!p8) {
		TRACE("NULL pointer");
		return 0;
	}
	ncs_encode_32bit(&p8, lck_timeout1);
	ncs_encode_32bit(&p8, lck_timeout2);
	/*ncs_encode_64bit(&p8, param->lck_cbk_timeout); */
	ncs_enc_claim_space(uba, 8);
	total_bytes += 8;

	p8 = ncs_enc_reserve_space(uba, 4);
	if (!p8) {
		TRACE("NULL pointer");
		return 0;
	}
	ncs_encode_32bit(&p8, param->admin_state);
	ncs_enc_claim_space(uba, 4);
	total_bytes += 4;

	TRACE_LEAVE();
	return total_bytes;
}

uint32_t enc_mbcsv_node_msg(NCS_UBAID *uba, CLMSV_CKPT_NODE * param)
{
	uint8_t *p8;
	uint32_t total_bytes = 0;
	SaUint32T lck_timeout1 = 0;
	SaUint32T lck_timeout2 = 0;

	lck_timeout1 = (param->lck_cbk_timeout & 0x00000000ffffffff);
	lck_timeout2 = (param->lck_cbk_timeout >> 32);

	TRACE_ENTER();

    /** encode the contents **/
	p8 = ncs_enc_reserve_space(uba, 4);
	if (!p8) {
		TRACE("NULL pointer");
		return 0;
	}
	ncs_encode_32bit(&p8, param->node_id);
	ncs_enc_claim_space(uba, 4);
	total_bytes += 4;

	total_bytes += encodeNodeAddressT(uba, &param->node_addr);
	total_bytes += clmsv_encodeSaNameT(uba, &param->node_name);
	total_bytes += clmsv_encodeSaNameT(uba, &param->ee_name);

	p8 = ncs_enc_reserve_space(uba, 4);
	if (!p8) {
		TRACE("NULL pointer");
		return 0;
	}
	ncs_encode_32bit(&p8, param->member);
	ncs_enc_claim_space(uba, 4);
	total_bytes += 4;

	p8 = ncs_enc_reserve_space(uba, 8);
	if (!p8) {
		TRACE("NULL pointer");
		return 0;
	}
	ncs_encode_64bit(&p8, param->boot_time);
	ncs_enc_claim_space(uba, 8);
	total_bytes += 8;

	p8 = ncs_enc_reserve_space(uba, 8);
	if (!p8) {
		TRACE("NULL pointer");
		return 0;
	}
	ncs_encode_64bit(&p8, param->init_view);
	ncs_enc_claim_space(uba, 8);
	total_bytes += 8;

	p8 = ncs_enc_reserve_space(uba, 4);
	if (!p8) {
		TRACE("NULL pointer");
		return 0;
	}
	ncs_encode_32bit(&p8, param->disable_reboot);
	ncs_enc_claim_space(uba, 4);
	total_bytes += 4;

	p8 = ncs_enc_reserve_space(uba, 8);
	if (!p8) {
		TRACE("NULL pointer");
		return 0;
	}
	ncs_encode_32bit(&p8, lck_timeout1);
	ncs_encode_32bit(&p8, lck_timeout2);
	/*ncs_encode_64bit(&p8, param->lck_cbk_timeout); */
	ncs_enc_claim_space(uba, 8);
	total_bytes += 8;

	p8 = ncs_enc_reserve_space(uba, 4);
	if (!p8) {
		TRACE("NULL pointer");
		return 0;
	}
	ncs_encode_32bit(&p8, param->admin_state);
	ncs_enc_claim_space(uba, 4);
	total_bytes += 4;

	p8 = ncs_enc_reserve_space(uba, 4);
	if (!p8) {
		TRACE("NULL pointer");
		return 0;
	}
	ncs_encode_32bit(&p8, param->change);
	ncs_enc_claim_space(uba, 4);
	total_bytes += 4;

#ifdef ENABLE_AIS_PLM
	p8 = ncs_enc_reserve_space(uba, 4);
	if (!p8) {
		TRACE("NULL pointer");
		return 0;
	}
	ncs_encode_32bit(&p8, param->ee_red_state);
	ncs_enc_claim_space(uba, 4);
	total_bytes += 4;
#endif

	p8 = ncs_enc_reserve_space(uba, 4);
	if (!p8) {
		TRACE("NULL pointer");
		return 0;
	}
	ncs_encode_32bit(&p8, param->nodeup);
	ncs_enc_claim_space(uba, 4);
	total_bytes += 4;

	p8 = ncs_enc_reserve_space(uba, 8);
	if (!p8) {
		TRACE("NULL pointer");
		return 0;
	}
	ncs_encode_64bit(&p8, param->curr_admin_inv);
	ncs_enc_claim_space(uba, 8);
	total_bytes += 8;

	p8 = ncs_enc_reserve_space(uba, 4);
	if (!p8) {
		TRACE("NULL pointer");
		return 0;
	}
	ncs_encode_32bit(&p8, param->stat_change);
	ncs_enc_claim_space(uba, 4);
	total_bytes += 4;

	p8 = ncs_enc_reserve_space(uba, 4);
	if (!p8) {
		TRACE("NULL pointer");
		return 0;
	}
	ncs_encode_32bit(&p8, param->admin_op);
	ncs_enc_claim_space(uba, 4);
	total_bytes += 4;

	p8 = ncs_enc_reserve_space(uba, 8);
	if (!p8) {
		TRACE("NULL pointer");
		return 0;
	}
	ncs_encode_64bit(&p8, param->plm_invid);
	ncs_enc_claim_space(uba, 8);
	total_bytes += 8;

	TRACE_LEAVE();
	return total_bytes;
}

uint32_t enc_mbcsv_node_del_msg(NCS_UBAID *uba, CLMSV_CKPT_NODE_DEL_REC * param)
{
	uint32_t total_bytes = 0;

	TRACE_ENTER();

    /** encode the contents **/
	total_bytes += clmsv_encodeSaNameT(uba, &param->node_name);

	TRACE_LEAVE();
	return total_bytes;
}

/****************************************************************************
 * Name          : ckpt_decode_cbk_handler
 *
 * Description   : This function is the single entry point to all decode
 *                 requests from mbcsv. 
 *                 Invokes the corresponding decode routine based on the 
 *                 MBCSv decode request type.
 *
 * Arguments     : arg - Pointer to NCS_MBCSV_CB_ARG with decode info
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : None.
 *****************************************************************************/
static uint32_t ckpt_decode_cbk_handler(NCS_MBCSV_CB_ARG *cbk_arg)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	uint16_t msg_fmt_version;

	TRACE_ENTER();

	msg_fmt_version = m_NCS_MBCSV_FMT_GET(cbk_arg->info.decode.i_peer_version,
					      CLMS_MBCSV_VERSION, CLMS_MBCSV_VERSION_MIN);
	if (0 == msg_fmt_version) {
		TRACE("wrong msg_fmt_version!!!\n");
		return NCSCC_RC_FAILURE;
	}

	TRACE_2("decode msg type: %u", (unsigned int)cbk_arg->info.decode.i_msg_type);

	switch (cbk_arg->info.decode.i_msg_type) {
	case NCS_MBCSV_MSG_COLD_SYNC_REQ:
		TRACE_2(" COLD SYNC REQ DECODE called");
		break;

	case NCS_MBCSV_MSG_COLD_SYNC_RESP:
	case NCS_MBCSV_MSG_COLD_SYNC_RESP_COMPLETE:
		TRACE_2(" COLD SYNC RESP DECODE called");
		if (clms_cb->ckpt_state != COLD_SYNC_COMPLETE) {	/*this check is needed to handle repeated requests */
			if ((rc = ckpt_decode_cold_sync(clms_cb, cbk_arg)) != NCSCC_RC_SUCCESS) {
				TRACE(" COLD SYNC RESPONSE DECODE ....");
			} else {
				TRACE(" COLD SYNC RESPONSE DECODE SUCCESS....");
				clms_cb->ckpt_state = COLD_SYNC_COMPLETE;
			}
		}
		break;

	case NCS_MBCSV_MSG_ASYNC_UPDATE:
		TRACE_2(" ASYNC UPDATE DECODE called");
		if ((rc = ckpt_decode_async_update(clms_cb, cbk_arg)) != NCSCC_RC_SUCCESS)
			TRACE("  ckpt_decode_async_update FAILED %u", rc);
		break;

	case NCS_MBCSV_MSG_WARM_SYNC_REQ:
	case NCS_MBCSV_MSG_WARM_SYNC_RESP:
	case NCS_MBCSV_MSG_WARM_SYNC_RESP_COMPLETE:
	case NCS_MBCSV_MSG_DATA_REQ:
		TRACE_2("WARM SYNC called, not used");
		break;
	case NCS_MBCSV_MSG_DATA_RESP:
	case NCS_MBCSV_MSG_DATA_RESP_COMPLETE:
		TRACE_2("DATA RESP COMPLETE DECODE called");
		if ((rc = ckpt_decode_cold_sync(clms_cb, cbk_arg)) != NCSCC_RC_SUCCESS)
			LOG_ER("ckpt_decode_cold_sync  FAILED");
		break;

	default:
		TRACE_2(" INCORRECT DECODE called");
		rc = NCSCC_RC_FAILURE;
		TRACE("  INCORRECT DECODE called, FAILED");
		m_LEAP_DBG_SINK_VOID;
		break;
	}			/*End switch(io_msg_type) */

	TRACE_LEAVE();
	return rc;

}	/*End ckpt_decode_cbk_handler() */

/****************************************************************************
 * Name          : ckpt_decode_cold_sync 
 *
 * Description   : This function decodes cold sync update data, based on the 
 *                 record type contained in the header. 
 *
 * Arguments     : arg - Pointer to NCS_MBCSV_CB_ARG with decode info
 *                 cb - pointer to clms cb.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : COLD SYNC RECORDS are expected in an order
 *                 1. cluster rec
 *                 2. node rec
 *                 3. client rec
 *                 For each record type,
 *                     a) decode header.
 *                     b) decode individual records for 
 *                        header->num_records times, 
 *****************************************************************************/

static uint32_t ckpt_decode_cold_sync(CLMS_CB * cb, NCS_MBCSV_CB_ARG *cbk_arg)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	CLMS_CKPT_REC msg;
	CLMS_CKPT_REC *data = NULL;
	uint32_t num_rec = 0;
	TRACE_ENTER();

	/* 
	   -------------------------------------------------------------
	   | Header|Cluster rec|Header|node rec..n|Header|client rec..n|
	   -------------------------------------------------------------
	 */

	memset(&msg, 0, sizeof(CLMS_CKPT_REC));
	data = &msg;

	TRACE_2("COLD SYNC DECODE START........");
	if (decode_ckpt_hdr(&cbk_arg->info.decode.i_uba, &data->header) == 0) {
		rc = NCSCC_RC_FAILURE;
		goto done;
	}

	/* Check if the first in the order of records is reg record */
	if (data->header.type != CLMS_CKPT_CLUSTER_REC) {
		TRACE("FAILED data->header.type != CLMS_CKPT_CLUSTER_REC");
		rc = NCSCC_RC_FAILURE;
		goto done;
	}

	if (decode_cluster_msg(&cbk_arg->info.decode.i_uba, &data->param.cluster_rec) == 0) {
		rc = NCSCC_RC_FAILURE;
		goto done;
	}

	/* Update our database */
	rc = process_ckpt_data(cb, data);
	if (rc != NCSCC_RC_SUCCESS) {
		goto done;
	}

	if (decode_ckpt_hdr(&cbk_arg->info.decode.i_uba, &data->header) == 0) {
		rc = NCSCC_RC_FAILURE;
		goto done;
	}

	/* Check if the first in the order of records is reg record */
	if (data->header.type != CLMS_CKPT_NODE_REC) {
		TRACE("FAILED data->header.type != CLMS_CKPT_CLUSTER_REC");
		rc = NCSCC_RC_FAILURE;
		goto done;
	}

	num_rec = data->header.num_ckpt_records;
	TRACE("num_rec %d", num_rec);
	while (num_rec) {
		if (decode_node_msg(&cbk_arg->info.decode.i_uba, &data->param.node_csync_rec) == 0) {
			rc = NCSCC_RC_FAILURE;
			goto done;
		}

		/* Update our database */
		rc = process_ckpt_data(cb, data);
		if (rc != NCSCC_RC_SUCCESS) {
			goto done;
		}
		memset(&data->param, 0, sizeof(data->param));
		--num_rec;
	}

	if (decode_ckpt_hdr(&cbk_arg->info.decode.i_uba, &data->header) == 0) {
		rc = NCSCC_RC_FAILURE;
		goto done;
	}

	/* Check if the first in the order of records is reg record */
	if (data->header.type != CLMS_CKPT_CLIENT_INFO_REC) {
		TRACE("FAILED data->header.type != CLMS_CKPT_CLUSTER_REC");
		rc = NCSCC_RC_FAILURE;
		goto done;
	}

	num_rec = data->header.num_ckpt_records;
	while (num_rec) {
		if (decode_client_rec_msg(&cbk_arg->info.decode.i_uba, &data->param.client_rec) == 0) {
			rc = NCSCC_RC_FAILURE;
			goto done;
		}

		/* Update our database */
		rc = process_ckpt_data(cb, data);
		if (rc != NCSCC_RC_SUCCESS) {
			goto done;
		}

		memset(&data->param, 0, sizeof(data->param));
		--num_rec;
	}

 done:
	if (rc != NCSCC_RC_SUCCESS) {
		/* Do not allow standby to get out of sync */
		/*clms_exit("Cold sync failed", SA_AMF_COMPONENT_RESTART); */	/*dude chk it out */
	}
	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
 * Name          : ckpt_decode_async_update 
 *
 * Description   : This function decodes async update data, based on the 
 *                 record type contained in the header. 
 *
 * Arguments     : arg - Pointer to NCS_MBCSV_CB_ARG with decode info
 *                 cb - pointer to clms cb.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : None.
 *****************************************************************************/

static uint32_t ckpt_decode_async_update(CLMS_CB * cb, NCS_MBCSV_CB_ARG *cbk_arg)
{
	uint32_t rc = NCSCC_RC_SUCCESS, num_bytes = 0;
	CLMS_CKPT_REC *ckpt_msg = NULL;
	CLMSV_CKPT_HEADER *hdr = NULL;
	CLMSV_CKPT_CLIENT_INFO *ckpt_client_rec = NULL;
	CLMSV_CKPT_FINALIZE_INFO *ckpt_finalize_rec = NULL;
	CLMSV_CKPT_NODE_RUNTIME_INFO *ckpt_node_rec = NULL;
	CLMSV_CKPT_NODE *ckpt_csync_node_rec = NULL;
	CLMSV_CKPT_NODE_CONFIG_REC *ckpt_node_config_rec = NULL;
	CLMSV_CKPT_NODE_DEL_REC *ckpt_node_del_rec = NULL;
	CLMSV_CKPT_AGENT_DOWN_REC *ckpt_agent_down = NULL;
	CLMSV_CKPT_CLUSTER_INFO *ckpt_cluster_rec = NULL;
	CLMSV_CKPT_NODE_DOWN_INFO *ckpt_node_down_rec = NULL;

	TRACE_ENTER();

	/* Allocate memory to hold the checkpoint message */
	ckpt_msg = calloc(1, sizeof(CLMS_CKPT_REC));

	/* Decode the message header */
	hdr = &ckpt_msg->header;
	num_bytes = decode_ckpt_hdr(&cbk_arg->info.decode.i_uba, hdr);
	if (num_bytes == 0) {
		TRACE("decode_ckpt_hdr FAILED");
		rc = NCSCC_RC_FAILURE;
		goto done;
	}

	TRACE("ckpt_rec_type: %d ", (int)hdr->type);
	/* Call decode routines appropriately */
	switch (hdr->type) {

	case CLMS_CKPT_CLIENT_INFO_REC:
		TRACE("INITIALIZE REC: UPDATE");
		ckpt_client_rec = &ckpt_msg->param.client_rec;
		num_bytes = decode_client_msg(&cbk_arg->info.decode.i_uba, ckpt_client_rec);
		if (num_bytes == 0) {
			TRACE("decode_client_msg FAILED");
			rc = NCSCC_RC_FAILURE;
			goto done;
		}
		break;
	case CLMS_CKPT_FINALIZE_REC:
		TRACE_2("CLMS_CKPT_FINALIZE_REC REC: UPDATE");
		ckpt_finalize_rec = &ckpt_msg->param.finalize_rec;
		num_bytes = decode_finalize_msg(&cbk_arg->info.decode.i_uba, ckpt_finalize_rec);
		if (num_bytes == 0) {
			TRACE("decode_finalize_msg FAILED");
			rc = NCSCC_RC_FAILURE;
			goto done;
		}
		break;
	case CLMS_CKPT_TRACK_CHANGES_REC:
		TRACE_2("CLMS_CKPT_TRACK_START REC: UPDATE");
		ckpt_client_rec = &ckpt_msg->param.client_rec;
		num_bytes = decode_track_changes_msg(&cbk_arg->info.decode.i_uba, ckpt_client_rec);
		if (num_bytes == 0) {
			TRACE("decode_track_start_msg FAILED");
			rc = NCSCC_RC_FAILURE;
			goto done;
		}
		break;
	case CLMS_CKPT_NODE_RUNTIME_REC:
		TRACE_2("CLMS_CKPT_NODE_RUNTIME_REC : UPDATE");
		ckpt_node_rec = &ckpt_msg->param.node_rec;
		num_bytes = decode_node_rec_msg(&cbk_arg->info.decode.i_uba, ckpt_node_rec);
		if (num_bytes == 0) {
			TRACE("decode_track_stop_msg FAILED");
			rc = NCSCC_RC_FAILURE;
			goto done;
		}
		break;
	case CLMS_CKPT_NODE_REC:
		TRACE_2("CLMS_CKPT_NODE_REC : UPDATE");
		ckpt_csync_node_rec = &ckpt_msg->param.node_csync_rec;
		num_bytes = decode_node_msg(&cbk_arg->info.decode.i_uba, ckpt_csync_node_rec);
		if (num_bytes == 0) {
			TRACE("decode_node_msg FAILED");
			rc = NCSCC_RC_FAILURE;
			goto done;
		}
		break;
	case CLMS_CKPT_NODE_CONFIG_REC:
		TRACE_2("CLMS_CKPT_NODE_CONFIG_REC : UPDATE");
		ckpt_node_config_rec = &ckpt_msg->param.node_config_rec;
		num_bytes = decode_node_config_msg(&cbk_arg->info.decode.i_uba, ckpt_node_config_rec);
		if (num_bytes == 0) {
			TRACE("decode_node_msg FAILED");
			rc = NCSCC_RC_FAILURE;
			goto done;
		}
		break;
	case CLMS_CKPT_NODE_DEL_REC:
		TRACE_2("CLMS_CKPT_NODE_CONFIG_REC : UPDATE");
		ckpt_node_del_rec = &ckpt_msg->param.node_del_rec;
		num_bytes = decode_node_del_msg(&cbk_arg->info.decode.i_uba, ckpt_node_del_rec);
		if (num_bytes == 0) {
			TRACE("decode_node_del_msg FAILED");
			rc = NCSCC_RC_FAILURE;
			goto done;
		}
		break;
	case CLMS_CKPT_AGENT_DOWN_REC:
		TRACE_2("CLMS_CKPT_AGENT_DOWN_REC : UPDATE");
		ckpt_agent_down = &ckpt_msg->param.agent_rec;
		num_bytes = decode_agent_down_msg(&cbk_arg->info.decode.i_uba, ckpt_agent_down);
		if (num_bytes == 0) {
			TRACE("decode_agent_down_msg FAILED");
			rc = NCSCC_RC_FAILURE;
			goto done;
		}
		break;
	case CLMS_CKPT_CLUSTER_REC:
		TRACE_2("CLMS_CKPT_CLUSTER_REC :UPDATE");
		ckpt_cluster_rec = &ckpt_msg->param.cluster_rec;
		num_bytes = decode_cluster_msg(&cbk_arg->info.decode.i_uba, ckpt_cluster_rec);
		if (num_bytes == 0) {
			TRACE("decode_agent_down_msg FAILED");
			rc = NCSCC_RC_FAILURE;
			goto done;
		}
		break;
	case CLMS_CKPT_NODE_DOWN_REC:
		TRACE_2("CLMS_CKPT_NODE_DOWN_REC: UPDATE:");
		ckpt_node_down_rec = &ckpt_msg->param.node_down_rec;
		num_bytes = decode_node_down_msg(&cbk_arg->info.decode.i_uba, ckpt_node_down_rec);
		if (num_bytes == 0) {
			TRACE("decode_node_down_msg FAILED");
			rc = NCSCC_RC_FAILURE;
			goto done;
		}
		break;
	default:
		rc = NCSCC_RC_FAILURE;
		TRACE("   FAILED");
		goto done;
		break;
	}			/*end of switch */

	rc = process_ckpt_data(cb, ckpt_msg);
	/* Update the Async Update Count at standby */
	cb->async_upd_cnt++;

 done:
	free(ckpt_msg);
	TRACE_LEAVE();
	return rc;
	/* if failure, should an indication be sent to active ? */
}

static uint32_t decode_node_down_msg(NCS_UBAID *uba, CLMSV_CKPT_NODE_DOWN_INFO * param)
{
	uint8_t *p8;
	uint32_t total_bytes = 0;
	uint8_t local_data[12];

	p8 = ncs_dec_flatten_space(uba, local_data, 4);
	param->node_id = ncs_decode_32bit(&p8);
	ncs_dec_skip_space(uba, 4);
	total_bytes += 4;
	TRACE_8("decode_node_down_rec_msg");
	return total_bytes;
}

uint32_t decode_cluster_msg(NCS_UBAID *uba, CLMSV_CKPT_CLUSTER_INFO * param)
{
	uint8_t *p8;
	uint32_t total_bytes = 0;
	uint8_t local_data[12];

	p8 = ncs_dec_flatten_space(uba, local_data, 20);
	param->num_nodes = ncs_decode_32bit(&p8);
	param->init_time = ncs_decode_64bit(&p8);
	param->cluster_view_num = ncs_decode_64bit(&p8);
	ncs_dec_skip_space(uba, 20);
	total_bytes += 20;
	TRACE_8("decode_cluster_rec_msg");
	return total_bytes;
}

uint32_t decode_client_rec_msg(NCS_UBAID *uba, CLMSV_CKPT_CLIENT_INFO * param)
{
	uint8_t *p8;
	uint32_t total_bytes = 0;
	uint8_t local_data[13];
	SaUint32T mds_dest1 = 0;
	SaUint32T mds_dest2 = 0;

	p8 = ncs_dec_flatten_space(uba, local_data, 13);
	param->client_id = ncs_decode_32bit(&p8);
	mds_dest1 = ncs_decode_32bit(&p8);
	mds_dest2 = ncs_decode_32bit(&p8);
	param->mds_dest = ((SaUint64T)mds_dest2 << 32) | mds_dest1;
	/*param->mds_dest = ncs_decode_64bit(&p8); */
	param->track_flags = ncs_decode_8bit(&p8);
	ncs_dec_skip_space(uba, 13);
	total_bytes += 13;
	TRACE_8("decode_client_msg");
	return total_bytes;
}

static uint32_t decode_agent_down_msg(NCS_UBAID *uba, CLMSV_CKPT_AGENT_DOWN_REC * param)
{
	uint8_t *p8;
	uint32_t total_bytes = 0;
	uint8_t local_data[8];
	SaUint32T mds_dest1 = 0;
	SaUint32T mds_dest2 = 0;

	TRACE_ENTER();

	p8 = ncs_dec_flatten_space(uba, local_data, 8);
	if (p8 == NULL)
		TRACE("p8 null");
	else {
		mds_dest1 = ncs_decode_32bit(&p8);
		mds_dest2 = ncs_decode_32bit(&p8);
		param->mds_dest = ((SaUint64T)mds_dest2 << 32) | mds_dest1;
		/*param->mds_dest = ncs_decode_64bit(&p8); */
	}
	ncs_dec_skip_space(uba, 8);
	total_bytes += 8;
	TRACE("decode_agent_down");

	TRACE_LEAVE();
	return total_bytes;

}

static uint32_t decode_client_msg(NCS_UBAID *uba, CLMSV_CKPT_CLIENT_INFO * param)
{
	uint8_t *p8;
	uint32_t total_bytes = 0;
	uint8_t local_data[12];
	SaUint32T mds_dest1 = 0;
	SaUint32T mds_dest2 = 0;
	TRACE_ENTER();

	p8 = ncs_dec_flatten_space(uba, local_data, 12);
	param->client_id = ncs_decode_32bit(&p8);
	mds_dest1 = ncs_decode_32bit(&p8);
	mds_dest2 = ncs_decode_32bit(&p8);
	param->mds_dest = ((SaUint64T)mds_dest2 << 32) | mds_dest1;
	/*param->mds_dest = ncs_decode_64bit(&p8); */
	ncs_dec_skip_space(uba, 12);
	total_bytes += 12;
	TRACE("decode_client_msg");

	TRACE_LEAVE();
	return total_bytes;
}

static uint32_t decode_finalize_msg(NCS_UBAID *uba, CLMSV_CKPT_FINALIZE_INFO * param)
{
	uint8_t *p8;
	uint32_t total_bytes = 0;
	uint8_t local_data[4];

	p8 = ncs_dec_flatten_space(uba, local_data, 4);
	param->client_id = ncs_decode_32bit(&p8);
	ncs_dec_skip_space(uba, 4);
	total_bytes += 4;
	TRACE_8("decode_finalize_msg");
	return total_bytes;
}

static uint32_t decode_track_changes_msg(NCS_UBAID *uba, CLMSV_CKPT_CLIENT_INFO * param)
{
	uint8_t *p8;
	uint32_t total_bytes = 0;
	uint8_t local_data[5];

	p8 = ncs_dec_flatten_space(uba, local_data, 5);
	param->client_id = ncs_decode_32bit(&p8);
	param->track_flags = ncs_decode_8bit(&p8);
	ncs_dec_skip_space(uba, 5);
	total_bytes += 5;
	TRACE_8("decode_track_start_msg");
	return total_bytes;
}

static uint32_t decode_node_rec_msg(NCS_UBAID *uba, CLMSV_CKPT_NODE_RUNTIME_INFO * param)
{
	uint8_t *p8;
	uint32_t total_bytes = 0;
	uint8_t local_data[8];

	p8 = ncs_dec_flatten_space(uba, local_data, 4);
	param->node_id = ncs_decode_32bit(&p8);
	ncs_dec_skip_space(uba, 4);
	total_bytes += 4;

	total_bytes += clmsv_decodeSaNameT(uba, &param->node_name);

	p8 = ncs_dec_flatten_space(uba, local_data, 4);
	param->member = ncs_decode_32bit(&p8);
	ncs_dec_skip_space(uba, 4);
	total_bytes += 4;

	p8 = ncs_dec_flatten_space(uba, local_data, 8);
	param->boot_time = ncs_decode_64bit(&p8);
	ncs_dec_skip_space(uba, 8);
	total_bytes += 8;

	p8 = ncs_dec_flatten_space(uba, local_data, 8);
	param->init_view = ncs_decode_64bit(&p8);
	ncs_dec_skip_space(uba, 8);
	total_bytes += 8;

	p8 = ncs_dec_flatten_space(uba, local_data, 4);
	param->admin_state = ncs_decode_32bit(&p8);
	ncs_dec_skip_space(uba, 4);
	total_bytes += 4;

	p8 = ncs_dec_flatten_space(uba, local_data, 4);
	param->admin_op = ncs_decode_32bit(&p8);
	ncs_dec_skip_space(uba, 4);
	total_bytes += 4;

	p8 = ncs_dec_flatten_space(uba, local_data, 4);
	param->change = ncs_decode_32bit(&p8);
	ncs_dec_skip_space(uba, 4);
	total_bytes += 4;

	p8 = ncs_dec_flatten_space(uba, local_data, 4);
	param->nodeup = ncs_decode_32bit(&p8);
	ncs_dec_skip_space(uba, 4);
	total_bytes += 4;

#ifdef ENABLE_AIS_PLM
	p8 = ncs_dec_flatten_space(uba, local_data, 4);
	param->ee_red_state = ncs_decode_32bit(&p8);
	ncs_dec_skip_space(uba, 4);
	total_bytes += 4;
#endif

	TRACE_8("decode_node_rec_msg");
	return total_bytes;
}

uint32_t decode_node_config_msg(NCS_UBAID *uba, CLMSV_CKPT_NODE_CONFIG_REC * param)
{
	uint8_t *p8;
	uint32_t total_bytes = 0;
	uint8_t local_data[8];
	SaUint32T lck_timeout1 = 0;
	SaUint32T lck_timeout2 = 0;

	total_bytes += clmsv_decodeSaNameT(uba, &param->node_name);
	total_bytes += clmsv_decodeSaNameT(uba, &param->ee_name);

	total_bytes += clmsv_decodeNodeAddressT(uba, &param->node_addr);

	p8 = ncs_dec_flatten_space(uba, local_data, 4);
	param->disable_reboot = ncs_decode_32bit(&p8);
	ncs_dec_skip_space(uba, 4);
	total_bytes += 4;

	p8 = ncs_dec_flatten_space(uba, local_data, 8);
	lck_timeout1 = ncs_decode_32bit(&p8);
	lck_timeout2 = ncs_decode_32bit(&p8);
	param->lck_cbk_timeout = ((SaUint64T)lck_timeout2 << 32) | lck_timeout1;
	/*param->lck_cbk_timeout = ncs_decode_64bit(&p8); */
	ncs_dec_skip_space(uba, 8);
	total_bytes += 8;

	p8 = ncs_dec_flatten_space(uba, local_data, 4);
	param->admin_state = ncs_decode_32bit(&p8);
	ncs_dec_skip_space(uba, 4);
	total_bytes += 4;

	TRACE_8("decode_node_config_msg");
	return total_bytes;
}

uint32_t decode_node_del_msg(NCS_UBAID *uba, CLMSV_CKPT_NODE_DEL_REC * param)
{
	uint32_t total_bytes = 0;

	total_bytes += clmsv_decodeSaNameT(uba, &param->node_name);

	TRACE_8("decode_node_delete_msg");
	return total_bytes;
}

uint32_t decode_node_msg(NCS_UBAID *uba, CLMSV_CKPT_NODE * param)
{
	uint8_t *p8;
	uint32_t total_bytes = 0;
	uint8_t local_data[12];
	SaUint32T lck_timeout1 = 0;
	SaUint32T lck_timeout2 = 0;
	TRACE_ENTER();

	p8 = ncs_dec_flatten_space(uba, local_data, 4);
	param->node_id = ncs_decode_32bit(&p8);
	ncs_dec_skip_space(uba, 4);
	total_bytes += 4;

	total_bytes += clmsv_decodeNodeAddressT(uba, &param->node_addr);
	total_bytes += clmsv_decodeSaNameT(uba, &param->node_name);
	total_bytes += clmsv_decodeSaNameT(uba, &param->ee_name);

	p8 = ncs_dec_flatten_space(uba, local_data, 4);
	param->member = ncs_decode_32bit(&p8);
	ncs_dec_skip_space(uba, 4);
	total_bytes += 4;

	p8 = ncs_dec_flatten_space(uba, local_data, 8);
	param->boot_time = ncs_decode_64bit(&p8);
	ncs_dec_skip_space(uba, 8);
	total_bytes += 8;

	p8 = ncs_dec_flatten_space(uba, local_data, 8);
	param->init_view = ncs_decode_64bit(&p8);
	ncs_dec_skip_space(uba, 8);
	total_bytes += 8;

	p8 = ncs_dec_flatten_space(uba, local_data, 4);
	param->disable_reboot = ncs_decode_32bit(&p8);
	ncs_dec_skip_space(uba, 4);
	total_bytes += 4;

	p8 = ncs_dec_flatten_space(uba, local_data, 8);
	lck_timeout1 = ncs_decode_32bit(&p8);
	lck_timeout2 = ncs_decode_32bit(&p8);
	param->lck_cbk_timeout = ((SaUint64T)lck_timeout2 << 32) | lck_timeout1;
	/*param->lck_cbk_timeout = ncs_decode_64bit(&p8); */
	ncs_dec_skip_space(uba, 8);
	total_bytes += 8;

	p8 = ncs_dec_flatten_space(uba, local_data, 4);
	param->admin_state = ncs_decode_32bit(&p8);
	ncs_dec_skip_space(uba, 4);
	total_bytes += 4;

	p8 = ncs_dec_flatten_space(uba, local_data, 4);
	param->change = ncs_decode_32bit(&p8);
	ncs_dec_skip_space(uba, 4);
	total_bytes += 4;

#ifdef ENABLE_AIS_PLM
	p8 = ncs_dec_flatten_space(uba, local_data, 4);
	param->ee_red_state = ncs_decode_32bit(&p8);
	ncs_dec_skip_space(uba, 4);
	total_bytes += 4;
#endif

	p8 = ncs_dec_flatten_space(uba, local_data, 4);
	param->nodeup = ncs_decode_32bit(&p8);
	ncs_dec_skip_space(uba, 4);
	total_bytes += 4;

	p8 = ncs_dec_flatten_space(uba, local_data, 8);
	param->curr_admin_inv = ncs_decode_64bit(&p8);
	ncs_dec_skip_space(uba, 8);
	total_bytes += 8;

	p8 = ncs_dec_flatten_space(uba, local_data, 4);
	param->stat_change = ncs_decode_32bit(&p8);
	ncs_dec_skip_space(uba, 4);
	total_bytes += 4;

	p8 = ncs_dec_flatten_space(uba, local_data, 4);
	param->admin_op = ncs_decode_32bit(&p8);
	ncs_dec_skip_space(uba, 4);
	total_bytes += 4;

	p8 = ncs_dec_flatten_space(uba, local_data, 8);
	param->plm_invid = ncs_decode_64bit(&p8);
	ncs_dec_skip_space(uba, 8);
	total_bytes += 8;

	TRACE_8("decode_node_msg");
	return total_bytes;
}

static uint32_t decode_ckpt_hdr(NCS_UBAID *uba, CLMSV_CKPT_HEADER * param)
{
	uint8_t *p8;
	uint32_t total_bytes = 0;
	uint8_t local_data[12];

	/* releaseCode, majorVersion, minorVersion */
	p8 = ncs_dec_flatten_space(uba, local_data, 12);
	param->type = ncs_decode_32bit(&p8);
	param->num_ckpt_records = ncs_decode_32bit(&p8);
	param->data_len = ncs_decode_32bit(&p8);
	ncs_dec_skip_space(uba, 12);
	total_bytes += 12;
	TRACE_8("decode_client_msg");
	return total_bytes;
}

/****************************************************************************
 * Name          : ckpt_peer_info_cbk_handler 
 *
 * Description   : This callback is invoked by mbcsv when a peer info message
 *                 is received from CLMS STANDBY. 
 *
 * Arguments     : NCS_MBCSV_ARG containing info pertaining to the STANDBY.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : None.
 ***************************************************************************/
static uint32_t ckpt_peer_info_cbk_handler(NCS_MBCSV_CB_ARG *arg)
{
	uint16_t peer_version;

	peer_version = arg->info.peer.i_peer_version;
	if (peer_version < CLMS_MBCSV_VERSION_MIN) {
		TRACE("peer_version not correct!!\n");
		return NCSCC_RC_FAILURE;
	}
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
 * Name          : ckpt_notify_cbk_handler 
 *
 * Description   : This callback is invoked by mbcsv when a notify message
 *                 is received from CLMS STANDBY. 
 *
 * Arguments     : NCS_MBCSV_ARG - contains notification info from STANDBY.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : None.
 ***************************************************************************/
static uint32_t ckpt_notify_cbk_handler(NCS_MBCSV_CB_ARG *arg)
{
	/* Currently nothing to be done */
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
 * Name          : ckpt_err_ind_cbk_handler 
 *
 * Description   : This callback is invoked by mbcsv for Peer error indication info
 *
 * Arguments     : NCS_MBCSV_ARG - contains notification info from STANDBY.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : None.
 ***************************************************************************/
static uint32_t ckpt_err_ind_cbk_handler(NCS_MBCSV_CB_ARG *arg)
{
	/* Currently nothing to be done. */
	return NCSCC_RC_SUCCESS;
}
