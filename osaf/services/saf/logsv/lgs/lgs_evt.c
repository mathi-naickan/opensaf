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
 * Author(s): Ericsson AB
 *
 */

#include <alloca.h>
#include <time.h>
#include <limits.h>

#include "immutil.h"

#include "lgs.h"
#include "lgs_util.h"
#include "lgs_fmt.h"

/* used when fixed log record size is zero */
#define LOG_MAX_LOGRECSIZE 1024

/* Macro to validate the version */
#define m_LOG_VER_IS_VALID(ver)   \
   ( (ver->releaseCode == LOG_RELEASE_CODE) && \
     (ver->majorVersion == LOG_MAJOR_VERSION || ver->minorVersion == LOG_MINOR_VERSION))

static uns32 process_api_evt(lgsv_lgs_evt_t *evt);
static uns32 proc_lga_updn_mds_msg(lgsv_lgs_evt_t *evt);
static uns32 proc_mds_quiesced_ack_msg(lgsv_lgs_evt_t *evt);
static uns32 proc_initialize_msg(lgs_cb_t *, lgsv_lgs_evt_t *evt);
static uns32 proc_finalize_msg(lgs_cb_t *, lgsv_lgs_evt_t *evt);
static uns32 proc_stream_open_msg(lgs_cb_t *, lgsv_lgs_evt_t *evt);
static uns32 proc_stream_close_msg(lgs_cb_t *, lgsv_lgs_evt_t *evt);
static uns32 proc_write_log_async_msg(lgs_cb_t *, lgsv_lgs_evt_t *evt);

static const LGSV_LGS_EVT_HANDLER lgs_lgsv_top_level_evt_dispatch_tbl[] = {
	process_api_evt,
	proc_lga_updn_mds_msg,
	proc_lga_updn_mds_msg,
	proc_mds_quiesced_ack_msg
};

/* Dispatch table for LGA_API realted messages */
static const
LGSV_LGS_LGA_API_MSG_HANDLER lgs_lga_api_msg_dispatcher[] = {
	proc_initialize_msg,
	proc_finalize_msg,
	proc_stream_open_msg,
	proc_stream_close_msg,
	proc_write_log_async_msg,
};

/**
 * Get client record from client ID
 * @param client_id
 * 
 * @return log_client_t*
 */
log_client_t *lgs_client_get_by_id(uns32 client_id)
{
	uns32 client_id_net;
	log_client_t *rec;

	client_id_net = m_NCS_OS_HTONL(client_id);
	rec = (log_client_t *)ncs_patricia_tree_get(&lgs_cb->client_tree, (uns8 *)&client_id_net);

	if (NULL == rec)
		TRACE("client_id: %u lookup failed", client_id);

	return rec;
}

/**
 * 
 * @param mds_dest
 * @param client_id set to zero if this is a new client
 * @param stream_list
 * 
 * @return log_client_t*
 */
log_client_t *lgs_client_new(MDS_DEST mds_dest, uns32 client_id, lgs_stream_list_t *stream_list)
{
	log_client_t *client;

	TRACE_ENTER2("MDS dest %llx", mds_dest);

	if (client_id == 0) {
		lgs_cb->last_client_id++;
		if (lgs_cb->last_client_id == 0)
			lgs_cb->last_client_id++;
	}

	if (NULL == (client = calloc(1, sizeof(log_client_t)))) {
		LOG_WA("calloc FAILED");
		goto done;
	}

    /** Initialize the record **/
	if ((lgs_cb->ha_state == SA_AMF_HA_STANDBY) || (lgs_cb->ha_state == SA_AMF_HA_QUIESCED))
		lgs_cb->last_client_id = client_id;
	client->client_id = lgs_cb->last_client_id;
	client->mds_dest = mds_dest;
	client->client_id_net = m_NCS_OS_HTONL(client->client_id);
	client->pat_node.key_info = (uns8 *)&client->client_id_net;
	client->stream_list_root = stream_list;

    /** Insert the record into the patricia tree **/
	if (NULL == ncs_patricia_tree_get(&lgs_cb->client_tree,client->pat_node.key_info)){
		if (NCSCC_RC_SUCCESS != ncs_patricia_tree_add(&lgs_cb->client_tree, &client->pat_node)) {
			LOG_WA("FAILED: ncs_patricia_tree_add, client_id %u", client_id);
			free(client);
			client = NULL;
			goto done;
		}
	}

 done:
	TRACE_LEAVE2("client_id %u", client_id);
	return client;
}

/**
 * Delete a client record and close all associated streams.
 * @param cb
 * @param client_id
 * 
 * @return uns32
 */
int lgs_client_delete(uns32 client_id)
{
	log_client_t *client;
	uns32 status = 0;
	lgs_stream_list_t *cur_rec;

	TRACE_ENTER2("client_id %u", client_id);

	if ((client = lgs_client_get_by_id(client_id)) == NULL) {
		status = -1;
		goto done;
	}

	cur_rec = client->stream_list_root;
	while (NULL != cur_rec) {
		lgs_stream_list_t *tmp_rec;
		log_stream_t *stream = log_stream_get_by_id(cur_rec->stream_id);
		TRACE_4("client_id: %u, REMOVE stream id: %u", client->client_id, cur_rec->stream_id);
		log_stream_close(&stream);
		tmp_rec = cur_rec->next;
		free(cur_rec);
		cur_rec = tmp_rec;
	}

	if (ncs_patricia_tree_get(&lgs_cb->client_tree,client->pat_node.key_info)){
		if (NCSCC_RC_SUCCESS != ncs_patricia_tree_del(&lgs_cb->client_tree, &client->pat_node)) {
			TRACE("ncs_patricia_tree_del FAILED");
			status = -2;
			goto done;
		}

		free(client);
	}
 done:
	TRACE_LEAVE();
	return status;
}

/**
 * Associate a stream with a client
 * @param client_id
 * @param stream_id
 * 
 * @return int
 */
int lgs_client_stream_add(uns32 client_id, uns32 stream_id)
{
	uns32 rs = 0;
	log_client_t *client;
	lgs_stream_list_t *stream;

	TRACE_ENTER2("client_id %u, stream ID %u", client_id, stream_id);

	if ((client = lgs_client_get_by_id(client_id)) == NULL) {
		rs = -1;
		goto err_exit;
	}

	if ((stream = malloc(sizeof(lgs_stream_list_t))) == NULL) {
		LOG_WA("malloc FAILED");
		rs = -1;
		goto err_exit;
	}

	stream->next = client->stream_list_root;
	stream->stream_id = stream_id;
	client->stream_list_root = stream;

 err_exit:
	TRACE_LEAVE();
	return rs;
}

/**
 * Remove a stream association from a client
 * @param client_id
 * @param stream_id
 * 
 * @return int
 */
int lgs_client_stream_rmv(uns32 client_id, uns32 stream_id)
{
	int rc = 0;
	lgs_stream_list_t *cur_rec;
	lgs_stream_list_t *last_rec, *tmp_rec;
	log_client_t *client;

	TRACE_ENTER2("client_id %u, stream ID %u", client_id, stream_id);

	if ((client = lgs_client_get_by_id(client_id)) == NULL) {
		rc = -1;
		goto done;
	}

	if (NULL == client->stream_list_root) {
		rc = -2;
		goto done;
	}

	cur_rec = client->stream_list_root;
	last_rec = client->stream_list_root;
	do {
		if (stream_id == cur_rec->stream_id) {
			TRACE_4("client_id: %u, REMOVE stream id: %u", client_id, cur_rec->stream_id);
			tmp_rec = cur_rec->next;
			last_rec->next = tmp_rec;
			if (client->stream_list_root == cur_rec)
				client->stream_list_root = tmp_rec;
			free(cur_rec);
			break;
		}
		last_rec = cur_rec;
		cur_rec = last_rec->next;
	} while (NULL != cur_rec);

 done:
	TRACE_LEAVE();
	return rc;
}

/**
 * Search for a client that matches the MDS dest and delete all the associated
 * resources.
 * @param cb
 * @param mds_dest
 * 
 * @return int
 */
int lgs_client_delete_by_mds_dest(MDS_DEST mds_dest)
{
	uns32 rc = 0;
	log_client_t *rp = NULL;
	uns32 client_id_net;

	TRACE_ENTER2("mds_dest %llx", mds_dest);
	rp = (log_client_t *)ncs_patricia_tree_getnext(&lgs_cb->client_tree, (uns8 *)0);

	while (rp != NULL) {
	/** Store the client_id_net for get Next  */
		client_id_net = rp->client_id_net;
		if (m_NCS_MDS_DEST_EQUAL(&rp->mds_dest, &mds_dest))
			rc = lgs_client_delete(rp->client_id);

		rp = (log_client_t *)ncs_patricia_tree_getnext(&lgs_cb->client_tree, (uns8 *)&client_id_net);
	}
	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
 *
 * lgs_remove_lga_down_rec
 * 
 *  Searches the LGA_DOWN_LIST for an entry whos MDS_DEST equals
 *  that passed in and removes the LGA rec.
 *
 * This routine is typically used to remove the lga down rec from standby 
 * LGA_DOWN_LIST as  LGA client has gone away.
 * 
 ****************************************************************************/
uns32 lgs_remove_lga_down_rec(lgs_cb_t *cb, MDS_DEST mds_dest)
{
	LGA_DOWN_LIST *lga_down_rec = cb->lga_down_list_head;
	LGA_DOWN_LIST *prev = NULL;
	while (lga_down_rec) {
		if (m_NCS_MDS_DEST_EQUAL(&lga_down_rec->mds_dest, &mds_dest)) {
			/* Remove the LGA entry */
			/* Reset pointers */
			if (lga_down_rec == cb->lga_down_list_head) {   /* 1st in the list? */
				if (lga_down_rec->next == NULL) {       /* Only one in the list? */
					cb->lga_down_list_head = NULL;  /* Clear head sublist pointer */
					cb->lga_down_list_tail = NULL;  /* Clear tail sublist pointer */
				} else {        /* 1st but not only one */

					cb->lga_down_list_head = lga_down_rec->next;    /* Move next one up */
				}
			} else {        /* Not 1st in the list */

				if (prev) {
					if (lga_down_rec->next == NULL)
						cb->lga_down_list_tail = prev;
					prev->next = lga_down_rec->next;        /* Link previous to next */
				}
			}

			/* Free the EDA_DOWN_REC */
			free(lga_down_rec);
			lga_down_rec = NULL;
			break;
		}
		prev = lga_down_rec;    /* Remember address of this entry */
		lga_down_rec = lga_down_rec->next;      /* Go to next entry */
	}
	return NCSCC_RC_SUCCESS;
}


/****************************************************************************
 * Name          : proc_lga_updn_mds_msg
 *
 * Description   : This is the function which is called when lgs receives any
 *                 a LGA UP/DN message via MDS subscription.
 *
 * Arguments     : msg  - Message that was posted to the LGS Mail box.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : None.
 *****************************************************************************/

static uns32 proc_lga_updn_mds_msg(lgsv_lgs_evt_t *evt)
{
	TRACE_ENTER();
	lgsv_ckpt_msg_t ckpt;
	uns32 rc = NCSCC_RC_SUCCESS;
	uns32 async_rc = NCSCC_RC_SUCCESS;

	switch (evt->evt_type) {
	case LGSV_LGS_EVT_LGA_UP:
		break;
	case LGSV_LGS_EVT_LGA_DOWN:
		if ((lgs_cb->ha_state == SA_AMF_HA_ACTIVE) || (lgs_cb->ha_state == SA_AMF_HA_QUIESCED)) {
		/* Remove this LGA entry from our processing lists */
		(void)lgs_client_delete_by_mds_dest(evt->fr_dest);

			/*Send an async checkpoint update to STANDBY EDS peer */
			if (lgs_cb->ha_state == SA_AMF_HA_ACTIVE) {
				memset(&ckpt, 0, sizeof(ckpt));
				ckpt.header.ckpt_rec_type = LGS_CKPT_CLIENT_DOWN;
				ckpt.header.num_ckpt_records = 1;
				ckpt.header.data_len = 1;
				ckpt.ckpt_rec.agent_dest = evt->fr_dest;
				async_rc = lgs_ckpt_send_async(lgs_cb, &ckpt, NCS_MBCSV_ACT_ADD);
				if (async_rc == NCSCC_RC_SUCCESS) {
					TRACE_4("ASYNC UPDATE SEND SUCCESS for LGA_DOWN event..");
				}
			}
		} else if (lgs_cb->ha_state == SA_AMF_HA_STANDBY) {
			LGA_DOWN_LIST *lga_down_rec = NULL;
			if (lgs_lga_entry_valid(lgs_cb, evt->fr_dest)) {
				if (NULL == (lga_down_rec = (LGA_DOWN_LIST *) malloc(sizeof(LGA_DOWN_LIST)))) {
				/* Log it */
					rc = SA_AIS_ERR_NO_MEMORY;
					TRACE("memory allocation for the LGA_DOWN_LIST failed");
					break;
				}
				memset(lga_down_rec, 0, sizeof(LGA_DOWN_LIST));
				lga_down_rec->mds_dest = evt->fr_dest;
				if (lgs_cb->lga_down_list_head == NULL) {
					lgs_cb->lga_down_list_head = lga_down_rec;
				} else {
					if (lgs_cb->lga_down_list_tail)
						lgs_cb->lga_down_list_tail->next = lga_down_rec;
				}
				lgs_cb->lga_down_list_tail = lga_down_rec;
			}
		}
		break;
	default:
		TRACE("Unknown evt type!!!");
		break;
	}

	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
 * Name          : proc_mds_quiesced_ack_msg
 *
 * Description   : This is the function which is called when lgs receives an
 *                       quiesced ack event from MDS 
 *
 * Arguments     : evt  - Message that was posted to the LGS Mail box.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : None.
 *****************************************************************************/
static uns32 proc_mds_quiesced_ack_msg(lgsv_lgs_evt_t *evt)
{
	TRACE_ENTER();

	if (lgs_cb->is_quiesced_set == TRUE) {
		/* Update control block */
		lgs_cb->is_quiesced_set = FALSE;
		lgs_cb->ha_state = SA_AMF_HA_QUIESCED;

		/* Inform MBCSV of HA state change */
		if (lgs_mbcsv_change_HA_state(lgs_cb) != NCSCC_RC_SUCCESS)
			TRACE("lgs_mbcsv_change_HA_state FAILED");

		/* Finally respond to AMF */
		saAmfResponse(lgs_cb->amf_hdl, lgs_cb->amf_invocation_id, SA_AIS_OK);
	} else
		LOG_ER("Received LGSV_EVT_QUIESCED_ACK message but is_quiesced_set==false");

	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}


/**
* Clear any pending lga_down records 
*
*/
static void lgs_process_lga_down_list(void)
{
	if (lgs_cb->ha_state == SA_AMF_HA_ACTIVE) {
		LGA_DOWN_LIST *lga_down_rec = NULL;
		LGA_DOWN_LIST *temp_lga_down_rec = NULL;

		lga_down_rec = lgs_cb->lga_down_list_head;
		while (lga_down_rec) {
			/*Remove the LGA DOWN REC from the LGA_DOWN_LIST */
			/* Free the LGA_DOWN_REC */
			/* Remove this LGA entry from our processing lists */
			temp_lga_down_rec = lga_down_rec;
			(void)lgs_client_delete_by_mds_dest(lga_down_rec->mds_dest);
			lga_down_rec = lga_down_rec->next;
			free(temp_lga_down_rec);
		}
		lgs_cb->lga_down_list_head = NULL;
		lgs_cb->lga_down_list_tail = NULL;
	}

}


static uns32 proc_rda_cb_msg(lgsv_lgs_evt_t *evt)
{
	log_stream_t *stream;
	uns32 rc;

	TRACE_ENTER2("%u", evt->info.rda_info.io_role);

	if (evt->info.rda_info.io_role == PCS_RDA_ACTIVE) {
		LOG_NO("ACTIVE request");
		lgs_cb->mds_role = V_DEST_RL_ACTIVE;
		lgs_cb->ha_state = SA_AMF_HA_ACTIVE;

		if ((rc = lgs_mds_change_role(lgs_cb)) != NCSCC_RC_SUCCESS) {
			LOG_ER("lgs_mds_change_role FAILED %u", rc);
			goto done;
		}

		if (NCSCC_RC_SUCCESS != lgs_mbcsv_change_HA_state(lgs_cb))
			goto done;

		/* fail over, become implementer */
		lgs_imm_impl_set(lgs_cb);

		/* Open all streams */
		stream = log_stream_getnext_by_name(NULL);
		if (!stream)
			LOG_ER("No streams exist!");
		while (stream != NULL) {
			if (log_stream_open(stream) != SA_AIS_OK)
				goto done;
			
			stream = log_stream_getnext_by_name(stream->name);
		}
		lgs_process_lga_down_list();
	}

	rc = NCSCC_RC_SUCCESS;

done:
	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
 * Name          : lgs_cb_init
 *
 * Description   : This function initializes the LGS_CB including the 
 *                 Patricia trees.
 *                 
 *
 * Arguments     : lgs_cb * - Pointer to the LGS_CB.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * Notes         : None.
 *****************************************************************************/
uns32 lgs_cb_init(lgs_cb_t *lgs_cb)
{
	NCS_PATRICIA_PARAMS reg_param;
	char *val;
	unsigned int max_logrecsize;

	TRACE_ENTER();

	memset(&reg_param, 0, sizeof(NCS_PATRICIA_PARAMS));

	reg_param.key_size = sizeof(uns32);

	/* Assign Initial HA state */
	lgs_cb->ha_state = LGS_HA_INIT_STATE;
	lgs_cb->csi_assigned = FALSE;

	/* Assign Version. Currently, hardcoded, This will change later */
	lgs_cb->log_version.releaseCode = LOG_RELEASE_CODE;
	lgs_cb->log_version.majorVersion = LOG_MAJOR_VERSION;
	lgs_cb->log_version.minorVersion = LOG_MINOR_VERSION;

	lgs_cb->max_logrecsize = LOG_MAX_LOGRECSIZE;

	if ((val = getenv("LOGSV_MAX_LOGRECSIZE")) != NULL) {
		max_logrecsize = strtol(val, NULL, 0);
		if (max_logrecsize >= 256)
			lgs_cb->max_logrecsize = max_logrecsize;
		else
			LOG_NO("Too low LOGSV_MAX_LOGRECSIZE (%u), using default (%u)",
				max_logrecsize, lgs_cb->max_logrecsize);
	}

	/* Initialize patricia tree for reg list */
	if (NCSCC_RC_SUCCESS != ncs_patricia_tree_init(&lgs_cb->client_tree, &reg_param))
		return NCSCC_RC_FAILURE;

	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

/**
 * Send a write log ack (callback request) to a client
 * @param cb
 * @param evt
 * @param error
 * 
 * @return uns32
 */
static uns32 send_write_log_ack(lgs_cb_t *cb, lgsv_lgs_evt_t *evt, SaAisErrorT error)
{
	uns32 rc = NCSCC_RC_SUCCESS;
	lgsv_msg_t msg;

	TRACE_ENTER();
	memset(&msg, 0, sizeof(lgsv_msg_t));
	msg.type = LGSV_LGS_CBK_MSG;
	msg.info.cbk_info.type = LGSV_WRITE_LOG_CALLBACK_IND;
	msg.info.cbk_info.lgs_client_id = evt->info.msg.info.api_info.param.write_log_async.client_id;
	msg.info.cbk_info.inv = evt->info.msg.info.api_info.param.write_log_async.invocation;
	msg.info.cbk_info.write_cbk.error = error;

	rc = lgs_mds_msg_send(cb, &msg, &evt->fr_dest, &evt->mds_ctxt, MDS_SEND_PRIORITY_HIGH);
	if (rc != NCSCC_RC_SUCCESS) {
		TRACE("send failed");	/* TODO: find out why ev. retry? */
	}

	TRACE_LEAVE();
	return (rc);
}

/**
 * Handle a initialize message
 * @param cb
 * @param evt
 * 
 * @return uns32
 */
static uns32 proc_initialize_msg(lgs_cb_t *cb, lgsv_lgs_evt_t *evt)
{
	uns32 rc = NCSCC_RC_SUCCESS;
	uns32 async_rc = NCSCC_RC_SUCCESS;
	SaAisErrorT ais_rc = SA_AIS_OK;
	SaVersionT *version;
	lgsv_msg_t msg;
	lgsv_ckpt_msg_t ckpt;
	log_client_t *client = NULL;

	TRACE_ENTER2("dest %llx", evt->fr_dest);

	/* Validate the version */
	version = &(evt->info.msg.info.api_info.param.init.version);
	if (!m_LOG_VER_IS_VALID(version)) {
		ais_rc = SA_AIS_ERR_VERSION;
		TRACE("version FAILED");
		goto snd_rsp;
	}

	if ((client = lgs_client_new(evt->fr_dest, 0, NULL)) == NULL) {
		ais_rc = SA_AIS_ERR_NO_MEMORY;
		goto snd_rsp;
	}

	if (cb->ha_state == SA_AMF_HA_ACTIVE) {
		memset(&ckpt, 0, sizeof(ckpt));
		ckpt.header.ckpt_rec_type = LGS_CKPT_CLIENT_INITIALIZE;
		ckpt.header.num_ckpt_records = 1;
		ckpt.header.data_len = 1;
		ckpt.ckpt_rec.initialize_client.client_id = cb->last_client_id;
		ckpt.ckpt_rec.initialize_client.mds_dest = evt->fr_dest;
		async_rc = lgs_ckpt_send_async(cb, &ckpt, NCS_MBCSV_ACT_ADD);
	}

 snd_rsp:
	msg.type = LGSV_LGA_API_RESP_MSG;
	msg.info.api_resp_info.type = LGSV_INITIALIZE_RSP;
	msg.info.api_resp_info.rc = ais_rc;
	msg.info.api_resp_info.param.init_rsp.client_id = client ? client->client_id : 0;
	rc = lgs_mds_msg_send(cb, &msg, &evt->fr_dest, &evt->mds_ctxt, MDS_SEND_PRIORITY_HIGH);

	TRACE_LEAVE2("client_id %u", client ? client->client_id : 0);
	return rc;
}

/**
 * Handle an finalize message
 * @param cb
 * @param evt
 * 
 * @return uns32
 */
static uns32 proc_finalize_msg(lgs_cb_t *cb, lgsv_lgs_evt_t *evt)
{
	int rc;
	lgsv_ckpt_msg_t ckpt;
	uns32 client_id = evt->info.msg.info.api_info.param.finalize.client_id;
	lgsv_msg_t msg;
	SaAisErrorT ais_rc = SA_AIS_OK;

	TRACE_ENTER2("client_id %u", client_id);

	/* Free all resources allocated by this client. */
	if ((rc = lgs_client_delete(client_id)) != 0) {
		TRACE("lgs_client_delete FAILED: %u", rc);
		ais_rc = SA_AIS_ERR_BAD_HANDLE;
		goto snd_rsp;
	}

	ckpt.header.ckpt_rec_type = LGS_CKPT_CLIENT_FINALIZE;
	ckpt.header.num_ckpt_records = 1;
	ckpt.header.data_len = 1;
	ckpt.ckpt_rec.finalize_client.client_id = client_id;
	(void)lgs_ckpt_send_async(lgs_cb, &ckpt, NCS_MBCSV_ACT_RMV);

 snd_rsp:
	msg.type = LGSV_LGA_API_RESP_MSG;
	msg.info.api_resp_info.type = LGSV_FINALIZE_RSP;
	msg.info.api_resp_info.rc = ais_rc;
	msg.info.api_resp_info.param.finalize_rsp.client_id = client_id;
	rc = lgs_mds_msg_send(cb, &msg, &evt->fr_dest, &evt->mds_ctxt, MDS_SEND_PRIORITY_HIGH);

	TRACE_LEAVE2("ais_rc: %u", ais_rc);
	return rc;
}

static uns32 lgs_ckpt_stream_open(lgs_cb_t *cb, log_stream_t *logStream, lgsv_stream_open_req_t *open_sync_param)
{
	lgsv_ckpt_msg_t ckpt;
	uns32 async_rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER();
	if (cb->ha_state == SA_AMF_HA_ACTIVE) {
		memset(&ckpt, 0, sizeof(ckpt));
		ckpt.header.ckpt_rec_type = LGS_CKPT_OPEN_STREAM;
		ckpt.header.num_ckpt_records = 1;
		ckpt.header.data_len = 1;
		ckpt.ckpt_rec.stream_open.clientId = open_sync_param->client_id;
		ckpt.ckpt_rec.stream_open.streamId = logStream->streamId;

		ckpt.ckpt_rec.stream_open.logFile = logStream->fileName;
		ckpt.ckpt_rec.stream_open.logPath = logStream->pathName;
		ckpt.ckpt_rec.stream_open.logFileCurrent = logStream->logFileCurrent;
		ckpt.ckpt_rec.stream_open.fileFmt = logStream->logFileFormat;
		ckpt.ckpt_rec.stream_open.logStreamName = (char *)logStream->name;

		ckpt.ckpt_rec.stream_open.maxFileSize = logStream->maxLogFileSize;
		ckpt.ckpt_rec.stream_open.maxLogRecordSize = logStream->fixedLogRecordSize;
		ckpt.ckpt_rec.stream_open.logFileFullAction = logStream->logFullAction;
		ckpt.ckpt_rec.stream_open.maxFilesRotated = logStream->maxFilesRotated;
		ckpt.ckpt_rec.stream_open.creationTimeStamp = logStream->creationTimeStamp;
		ckpt.ckpt_rec.stream_open.numOpeners = logStream->numOpeners;

		ckpt.ckpt_rec.stream_open.streamType = logStream->streamType;
		ckpt.ckpt_rec.stream_open.logRecordId = logStream->logRecordId;

		async_rc = lgs_ckpt_send_async(cb, &ckpt, NCS_MBCSV_ACT_ADD);
		if (async_rc == NCSCC_RC_SUCCESS) {
			TRACE_4("REG_REC ASYNC UPDATE SEND SUCCESS...");
		}
	}
	TRACE_LEAVE();
	return async_rc;
}

static SaAisErrorT create_new_app_stream(lgsv_stream_open_req_t *open_sync_param, log_stream_t **o_stream)
{
	SaAisErrorT rc = SA_AIS_OK;
	log_stream_t *stream;
	SaBoolT twelveHourModeFlag;

	TRACE_ENTER();

	if (open_sync_param->lstr_name.length > SA_MAX_NAME_LENGTH) {
		TRACE("Name too long");
		rc = SA_AIS_ERR_INVALID_PARAM;
		goto done;
	}

	if (open_sync_param->logFileFullAction != SA_LOG_FILE_FULL_ACTION_ROTATE) {
		TRACE("Unsupported logFileFullAction");
		rc = SA_AIS_ERR_NOT_SUPPORTED;
		goto done;
	}

	if (open_sync_param->logFileFmt == NULL) {
		TRACE("Assigning default format string for app stream");
		open_sync_param->logFileFmt = strdup(DEFAULT_APP_SYS_FORMAT_EXP);
	}

	/* Check the format string */
	if (!lgs_is_valid_format_expression(open_sync_param->logFileFmt, STREAM_TYPE_APPLICATION, &twelveHourModeFlag)) {
		TRACE("format expression failure");
		rc = SA_AIS_ERR_INVALID_PARAM;
		goto done;
	}

	/* Verify that path and file are unique */
	stream = log_stream_getnext_by_name(NULL);
	while (stream != NULL) {
		if ((strncmp(stream->fileName, open_sync_param->logFileName, NAME_MAX) == 0) &&
		    (strncmp(stream->pathName, open_sync_param->logFilePathName, SA_MAX_NAME_LENGTH) == 0)) {
			TRACE("pathname already exist");
			rc = SA_AIS_ERR_INVALID_PARAM;
			goto done;
		}
		stream = log_stream_getnext_by_name(stream->name);
	}

	/* Verify that the name seems to be a DN */
	if (strncmp("safLgStr=", (char *)open_sync_param->lstr_name.value, sizeof("safLgStr=") != 0)) {
		TRACE("'%s' is not a valid stream name => invalid param", open_sync_param->lstr_name.value);
		rc = SA_AIS_ERR_INVALID_PARAM;
		goto done;
	}

	stream = log_stream_new(&open_sync_param->lstr_name,
				open_sync_param->logFileName,
				open_sync_param->logFilePathName,
				open_sync_param->maxLogFileSize,
				open_sync_param->maxLogRecordSize,
				open_sync_param->logFileFullAction,
				open_sync_param->maxFilesRotated,
				open_sync_param->logFileFmt,
				STREAM_TYPE_APPLICATION, STREAM_NEW, twelveHourModeFlag, 0);

	if (stream == NULL) {
		rc = SA_AIS_ERR_NO_MEMORY;
		goto done;
	}
	*o_stream = stream;

 done:
	TRACE_LEAVE();
	return rc;
}

static SaAisErrorT file_attribute_cmp(lgsv_stream_open_req_t *open_sync_param, log_stream_t *applicationStream)
{
	SaAisErrorT rs = SA_AIS_OK;

	TRACE_ENTER2("Stream: %s", applicationStream->name);

	if (open_sync_param->maxLogFileSize != applicationStream->maxLogFileSize ||
	    open_sync_param->maxLogRecordSize != applicationStream->fixedLogRecordSize ||
	    open_sync_param->maxFilesRotated != applicationStream->maxFilesRotated) {
		TRACE("create params differ, new: mfs %llu, mlrs %u , mr %u"
		      " existing: mfs %llu, mlrs %u , mr %u",
		      open_sync_param->maxLogFileSize,
		      open_sync_param->maxLogRecordSize,
		      open_sync_param->maxFilesRotated,
		      applicationStream->maxLogFileSize,
		      applicationStream->fixedLogRecordSize, applicationStream->maxFilesRotated);
		rs = SA_AIS_ERR_EXIST;
	} else if (applicationStream->logFullAction != open_sync_param->logFileFullAction) {
		TRACE("logFileFullAction create params differs, new: %d, old: %d",
		      open_sync_param->logFileFullAction, applicationStream->logFullAction);
		rs = SA_AIS_ERR_EXIST;
	} else if (strcmp(applicationStream->fileName, open_sync_param->logFileName) != 0) {
		TRACE("logFileName differs, new: %s existing: %s",
		      open_sync_param->logFileName, applicationStream->fileName);
		rs = SA_AIS_ERR_EXIST;
	} else if (strcmp(applicationStream->pathName, open_sync_param->logFilePathName) != 0) {
		TRACE("log file path differs, new: %s existing: %s",
		      open_sync_param->logFilePathName, applicationStream->pathName);
		rs = SA_AIS_ERR_EXIST;
	} else if ((open_sync_param->logFileFmt != NULL) &&
		   strcmp((const char *)applicationStream->logFileFormat,
			  (const char *)open_sync_param->logFileFmt) != 0) {
		TRACE("logFile format differs, new: %s existing: %s",
		      open_sync_param->logFileFmt, applicationStream->logFileFormat);
		rs = SA_AIS_ERR_EXIST;
	}

	TRACE_LEAVE();
	return rs;
}

/****************************************************************************
 * Name          : proc_stream_open_msg
 *
 * Description   : This is the function which is called when lgs receives a
 *                 LGSV_LGA_LSTR_OPEN (log stream open) message.
 *
 * Arguments     : msg  - Message that was posted to the LGS Mail box.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : None.
 *****************************************************************************/

static uns32 proc_stream_open_msg(lgs_cb_t *cb, lgsv_lgs_evt_t *evt)
{
	uns32 rc = NCSCC_RC_SUCCESS, async_rc = NCSCC_RC_SUCCESS;
	SaAisErrorT ais_rv = SA_AIS_OK;
	uns32 lstr_id = (uns32)-1;
	lgsv_msg_t msg;
	lgsv_stream_open_req_t *open_sync_param = &(evt->info.msg.info.api_info.param.lstr_open_sync);
	log_stream_t *logStream;
	char name[SA_MAX_NAME_LENGTH + 1];

	/* Create null-terminated stream name */
	memcpy(name, open_sync_param->lstr_name.value, open_sync_param->lstr_name.length);
	memset(&name[open_sync_param->lstr_name.length], 0, SA_MAX_NAME_LENGTH + 1 - open_sync_param->lstr_name.length);

	TRACE_ENTER2("stream '%s', client_id %u", name, open_sync_param->client_id);

	logStream = log_stream_get_by_name(name);
	if (logStream != NULL) {
		TRACE("existing stream - id %u", logStream->streamId);
		if (logStream->streamType == STREAM_TYPE_APPLICATION) {
			/* Verify the creation attributes for an existing appl. stream */
			if (open_sync_param->lstr_open_flags & SA_LOG_STREAM_CREATE) {
				ais_rv = file_attribute_cmp(open_sync_param, logStream);
				if (ais_rv != SA_AIS_OK)
					goto snd_rsp;
			}
		} else {
			/* One of the well-known log streams */
			if (open_sync_param->lstr_open_flags & SA_LOG_STREAM_CREATE) {
				ais_rv = SA_AIS_ERR_INVALID_PARAM;
				goto snd_rsp;
			}
		}
	} else {
		if (cb->immOiHandle == 0) {
			TRACE("IMM service unavailable, open stream failed");
			ais_rv = SA_AIS_ERR_TRY_AGAIN;
			goto snd_rsp;
		}

		if ((open_sync_param->lstr_open_flags & SA_LOG_STREAM_CREATE) == 0) {
			ais_rv = SA_AIS_ERR_NOT_EXIST;
			goto snd_rsp;
		}

		ais_rv = create_new_app_stream(open_sync_param, &logStream);
		if (ais_rv != SA_AIS_OK)
			goto snd_rsp;
	}

	ais_rv = log_stream_open(logStream);
	if (ais_rv != SA_AIS_OK) {
		if (logStream != NULL)
			log_stream_delete(&logStream);

		goto snd_rsp;
	}

	log_stream_print(logStream);

	/* Create an association between this client and the stream */
	rc = lgs_client_stream_add(open_sync_param->client_id, logStream->streamId);
	if (rc != 0) {
		log_stream_close(&logStream);
		ais_rv = SA_AIS_ERR_NO_RESOURCES;
		goto snd_rsp;
	}

	TRACE_4("logStream->streamId = %u", logStream->streamId);
	lstr_id = logStream->streamId;

 snd_rsp:
	memset(&msg, 0, sizeof(lgsv_msg_t));
	msg.type = LGSV_LGA_API_RESP_MSG;
	msg.info.api_resp_info.type = LGSV_STREAM_OPEN_RSP;
	msg.info.api_resp_info.rc = ais_rv;
	msg.info.api_resp_info.param.lstr_open_rsp.lstr_id = lstr_id;
	TRACE_4("lstr_open_rsp.lstr_id: %u, rv: %u", lstr_id, ais_rv);
	rc = lgs_mds_msg_send(cb, &msg, &evt->fr_dest, &evt->mds_ctxt, MDS_SEND_PRIORITY_HIGH);

	if (ais_rv == SA_AIS_OK) {
		async_rc = lgs_ckpt_stream_open(cb, logStream, open_sync_param);
	}
	free(open_sync_param->logFileFmt);
	TRACE_LEAVE();
	return rc;
}

/**
 * Handle a stream close message
 * @param cb
 * @param evt
 * 
 * @return uns32
 */
static uns32 proc_stream_close_msg(lgs_cb_t *cb, lgsv_lgs_evt_t *evt)
{
	uns32 rc = NCSCC_RC_SUCCESS;
	lgsv_stream_close_req_t *close_param = &(evt->info.msg.info.api_info.param.lstr_close);
	lgsv_ckpt_msg_t ckpt;
	log_stream_t *stream;
	lgsv_msg_t msg;
	SaAisErrorT ais_rc = SA_AIS_OK;
	uns32 streamId;

	TRACE_ENTER2("client_id %u, stream ID %u", close_param->client_id, close_param->lstr_id);

	if ((stream = log_stream_get_by_id(close_param->lstr_id)) == NULL) {
		TRACE("Bad stream ID");
		ais_rc = SA_AIS_ERR_BAD_HANDLE;
		goto snd_rsp;
	}

	if ((stream->streamType == STREAM_TYPE_APPLICATION) && (cb->immOiHandle == 0)) {
		TRACE("IMM service unavailable, close stream failed");
		ais_rc = SA_AIS_ERR_TRY_AGAIN;
		goto snd_rsp;
	}

	if (lgs_client_stream_rmv(close_param->client_id, close_param->lstr_id) != 0) {
		TRACE("Bad client or stream ID");
		ais_rc = SA_AIS_ERR_BAD_HANDLE;
		goto snd_rsp;
	}

	streamId = stream->streamId;
	(void)log_stream_close(&stream);

	ckpt.header.ckpt_rec_type = LGS_CKPT_CLOSE_STREAM;
	ckpt.header.num_ckpt_records = 1;
	ckpt.header.data_len = 1;
	ckpt.ckpt_rec.stream_close.clientId = close_param->client_id;
	ckpt.ckpt_rec.stream_close.streamId = streamId;

	(void)lgs_ckpt_send_async(cb, &ckpt, NCS_MBCSV_ACT_RMV);

 snd_rsp:
	msg.type = LGSV_LGA_API_RESP_MSG;
	msg.info.api_resp_info.type = LGSV_STREAM_CLOSE_RSP;
	msg.info.api_resp_info.rc = ais_rc;
	msg.info.api_resp_info.param.close_rsp.client_id = close_param->client_id;
	msg.info.api_resp_info.param.close_rsp.lstr_id = close_param->lstr_id;
	rc = lgs_mds_msg_send(cb, &msg, &evt->fr_dest, &evt->mds_ctxt, MDS_SEND_PRIORITY_HIGH);
	TRACE_LEAVE2("ais_rc: %u", ais_rc);
	return rc;
}

/****************************************************************************
 * Name          : proc_write_log_async_msg
 *
 * Description   : This is the function which is called when lgs receives a
 *                 LGSV_LGA_WRITE_LOG_ASYNC message.
 *
 * Arguments     : msg  - Message that was posted to the Mail box.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : None.
 *****************************************************************************/
static uns32 proc_write_log_async_msg(lgs_cb_t *cb, lgsv_lgs_evt_t *evt)
{
	lgsv_write_log_async_req_t *param = &(evt->info.msg.info.api_info.param).write_log_async;
	log_stream_t *stream;
	SaAisErrorT error = SA_AIS_OK;
	SaStringT logOutputString = NULL;
	SaUint32T buf_size;
	int n;

	TRACE_ENTER2("client_id %u, stream ID %u", param->client_id, param->lstr_id);

	if (lgs_client_get_by_id(param->client_id) == NULL) {
		TRACE("Bad client ID: %u", param->client_id);
		error = SA_AIS_ERR_BAD_HANDLE;
		goto done;
	}

	if ((stream = log_stream_get_by_id(param->lstr_id)) == NULL) {
		TRACE("Bad stream ID: %u", param->lstr_id);
		error = SA_AIS_ERR_BAD_HANDLE;
		goto done;
	}

	/* Apply filtering only to system and application streams */
	if ((param->logRecord->logHdrType == SA_LOG_GENERIC_HEADER) &&
	    ((stream->severityFilter & (1 << param->logRecord->logHeader.genericHdr.logSeverity)) == 0)) {
		stream->filtered++;
		goto done;
	}

	/*
	** To avoid truncation we support fixedLogRecordSize==0. We then allocate an
	** a buffer with an implementation defined size instead. We also do not pad in this mode.
	*/
	buf_size = stream->fixedLogRecordSize == 0 ? lgs_cb->max_logrecsize : stream->fixedLogRecordSize;
	logOutputString = calloc(1, buf_size);
	if (logOutputString == NULL) {
		LOG_ER("Could not allocate %d bytes", stream->fixedLogRecordSize + 1);
		error = SA_AIS_ERR_NO_MEMORY;
		goto done;
	}

	if ((n = lgs_format_log_record(param->logRecord, stream->logFileFormat,
		stream->fixedLogRecordSize, buf_size, logOutputString, ++stream->logRecordId)) == 0) {
		error = SA_AIS_ERR_INVALID_PARAM;
		goto done;
	}

	if (log_stream_write(stream, logOutputString, n) == -1) {
		error = SA_AIS_ERR_NO_RESOURCES;
		goto done;
	}

	/*
	 ** FIX: Optimization: we only need to sync when file has been rotated
	 ** Standby does stat() and calculates recordId and curFileSize.
	 */
	/* TODO: send fail back if ack is wanted, Fix counter for application stream!! */
	if (cb->ha_state == SA_AMF_HA_ACTIVE) {
		lgsv_ckpt_msg_t ckpt;
		memset(&ckpt, 0, sizeof(ckpt));
		ckpt.header.ckpt_rec_type = LGS_CKPT_LOG_WRITE;
		ckpt.header.num_ckpt_records = 1;
		ckpt.header.data_len = 1;
		ckpt.ckpt_rec.write_log.recordId = stream->logRecordId;
		ckpt.ckpt_rec.write_log.streamId = stream->streamId;
		ckpt.ckpt_rec.write_log.curFileSize = stream->curFileSize;
		ckpt.ckpt_rec.write_log.logFileCurrent = stream->logFileCurrent;

		(void)lgs_ckpt_send_async(cb, &ckpt, NCS_MBCSV_ACT_ADD);
	}

 done:
	if (logOutputString != NULL)
		free(logOutputString);

	if (param->ack_flags == SA_LOG_RECORD_WRITE_ACK)
		send_write_log_ack(cb, evt, error);

	if (param->logRecord->logHdrType == SA_LOG_GENERIC_HEADER) {
		SaLogGenericLogHeaderT *genLogH = &param->logRecord->logHeader.genericHdr;
		free(param->logRecord->logBuffer->logBuf);
		free(param->logRecord->logBuffer);
		free(genLogH->notificationClassId);
		free((void *)genLogH->logSvcUsrName);
		free(param->logRecord);
	} else {
		SaLogNtfLogHeaderT *ntfLogH = &param->logRecord->logHeader.ntfHdr;
		free(param->logRecord->logBuffer->logBuf);
		free(param->logRecord->logBuffer);
		free(ntfLogH->notificationClassId);
		free(ntfLogH->notifyingObject);
		free(ntfLogH->notificationObject);
		free(param->logRecord);
	}

	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
 * Name          : process_api_evt
 *
 * Description   : This is the function which is called when lgs receives an
 *                 event either because of an API Invocation or other internal
 *                 messages from LGA clients
 *
 * Arguments     : evt  - Message that was posted to the LGS Mail box.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : None.
 *****************************************************************************/
static uns32 process_api_evt(lgsv_lgs_evt_t *evt)
{
	if (evt->evt_type == LGSV_LGS_LGSV_MSG) {
		/* ignore one level... */
		if (evt->info.msg.type < LGSV_MSG_MAX) {
			if (evt->info.msg.info.api_info.type < LGSV_API_MAX) {
				if (lgs_lga_api_msg_dispatcher[evt->info.msg.info.api_info.type] (lgs_cb, evt) !=
				    NCSCC_RC_SUCCESS) {
					LOG_ER("lgs_lga_api_msg_dispatcher FAILED type: %d", evt->info.msg.type);
				}
			} else
				LOG_ER("Invalid msg type %d", evt->info.msg.info.api_info.type);
		} else
			LOG_ER("Invalid event type %d", evt->info.msg.type);
	}
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
 * Name          : lgs_process_mbx
 *
 * Description   : This is the function which process the IPC mail box of 
 *                 LGS 
 *
 * Arguments     : mbx  - This is the mail box pointer on which LGS is 
 *                        going to block.  
 *
 * Return Values : None.
 *
 * Notes         : None.
 *****************************************************************************/
void lgs_process_mbx(SYSF_MBX *mbx)
{
	lgsv_lgs_evt_t *msg;

	msg = (lgsv_lgs_evt_t *)m_NCS_IPC_NON_BLK_RECEIVE(mbx, msg);
	if (msg != NULL) {
		if (lgs_cb->ha_state == SA_AMF_HA_ACTIVE) {
			if (msg->evt_type <= LGSV_LGS_EVT_LGA_DOWN) {
				lgs_lgsv_top_level_evt_dispatch_tbl[msg->evt_type] (msg);
			} else if (msg->evt_type == LGSV_EVT_QUIESCED_ACK) {
				proc_mds_quiesced_ack_msg(msg);
			} else if (msg->evt_type == LGSV_EVT_NO_OP) {
				TRACE("Jolted the main thread so it picks up the new IMM FD");
			} else if (msg->evt_type == LGSV_EVT_RDA) {
				TRACE("ignoring RDA message for role %u", msg->info.rda_info.io_role);
			} else
				LOG_ER("message type invalid");
		} else {
			if (msg->evt_type == LGSV_LGS_EVT_LGA_DOWN) {
				lgs_lgsv_top_level_evt_dispatch_tbl[msg->evt_type] (msg);
			}
			if (msg->evt_type == LGSV_EVT_RDA) {
				proc_rda_cb_msg(msg);
			}
		}

		lgs_evt_destroy(msg);
	}
}
