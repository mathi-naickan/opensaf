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

/*****************************************************************************
..............................................................................
    
..............................................................................
      
DESCRIPTION:
        
This include file contains evt processing subroutines
          
******************************************************************************
*/
#include <limits.h>		/* for MAX_INT definition */
#include "eds.h"
#include "eds_ckpt.h"
#include "logtrace.h"

static uint32_t eds_proc_eda_api_msg(EDSV_EDS_EVT *);
static uint32_t eds_proc_eda_misc_msg(EDSV_EDS_EVT *);
static uint32_t eds_proc_invalid_msg(EDSV_EDS_EVT *);
static uint32_t eds_proc_init_msg(EDS_CB *, EDSV_EDS_EVT *);
static uint32_t eds_proc_unexpected_msg(EDS_CB *, EDSV_EDS_EVT *);
static uint32_t eds_proc_finalize_msg(EDS_CB *, EDSV_EDS_EVT *);
static uint32_t eds_proc_chan_open_sync_msg(EDS_CB *, EDSV_EDS_EVT *);
static uint32_t eds_proc_chan_open_async_msg(EDS_CB *, EDSV_EDS_EVT *);
static uint32_t eds_proc_chan_close_msg(EDS_CB *, EDSV_EDS_EVT *);
static uint32_t eds_proc_chan_unlink_msg(EDS_CB *, EDSV_EDS_EVT *);
static uint32_t eds_proc_publish_msg(EDS_CB *, EDSV_EDS_EVT *);
static uint32_t eds_proc_subscribe_msg(EDS_CB *, EDSV_EDS_EVT *);
static uint32_t eds_proc_unsubscribe_msg(EDS_CB *, EDSV_EDS_EVT *);
static uint32_t eds_proc_retention_time_clr_msg(EDS_CB *, EDSV_EDS_EVT *);
static uint32_t eds_proc_limit_get_msg(EDS_CB *, EDSV_EDS_EVT *);

static uint32_t eds_proc_eda_updn_mds_msg(EDSV_EDS_EVT *evt);
static uint32_t eds_process_api_evt(EDSV_EDS_EVT *evt);
static uint32_t eds_proc_ret_tmr_exp_evt(EDSV_EDS_EVT *evt);
static uint32_t eds_proc_quiesced_ack_evt(EDSV_EDS_EVT *evt);

static void eds_publish_log_event(EDS_WORKLIST *wp, EDSV_EDA_PUBLISH_PARAM *publish_param, SaTimeT publish_time);

/* Top level EDS dispatcher */
static const
EDSV_EDS_EVT_HANDLER eds_edsv_top_level_evt_dispatch_tbl[EDSV_EDS_EVT_MAX - EDSV_EDS_EVT_BASE] = {
	eds_process_api_evt,
	eds_proc_eda_updn_mds_msg,
	eds_proc_eda_updn_mds_msg,
	eds_proc_ret_tmr_exp_evt,
	eds_proc_quiesced_ack_evt
};

/* Main EDS dispatch table for EDSV messages */
static const
EDSV_EDS_EVT_HANDLER eds_edsv_evt_dispatch_tbl[EDSV_MSG_MAX - EDSV_BASE_MSG] = {
	eds_proc_eda_api_msg,
	eds_proc_eda_misc_msg,
	/* THE EDS should logically not recv any
	 * other messages
	 */
	eds_proc_invalid_msg,
	eds_proc_invalid_msg,
	eds_proc_invalid_msg
};

/* Dispatch table for EDA_API realted messages */
static const
EDSV_EDS_EDA_API_MSG_HANDLER eds_eda_api_msg_dispatcher[EDSV_API_MAX - EDSV_API_BASE_MSG] = {
	eds_proc_init_msg,
	eds_proc_unexpected_msg,
	eds_proc_unexpected_msg,
	eds_proc_finalize_msg,
	eds_proc_chan_open_sync_msg,
	eds_proc_chan_open_async_msg,
	eds_proc_chan_close_msg,
	eds_proc_chan_unlink_msg,
	eds_proc_unexpected_msg,
	eds_proc_unexpected_msg,
	eds_proc_unexpected_msg,
	eds_proc_unexpected_msg,
	eds_proc_unexpected_msg,
	eds_proc_publish_msg,
	eds_proc_subscribe_msg,
	eds_proc_unsubscribe_msg,
	eds_proc_retention_time_clr_msg,
	eds_proc_limit_get_msg
};

/* Pattern for 'LOST EVENT' event */
SaEvtEventPatternT gl_lost_evt_pattern[1] = {
	{25, 25, (SaUint8T *)SA_EVT_LOST_EVENT}
};

/****************************************************************************
 * Name          : eds_proc_init_msg
 *
 * Description   : This is the function which is called when eds receives a
 *                 EDSV_EDA_INITIALIZE message.
 *
 * Arguments     : msg  - Message that was posted to the EDS Mail box.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : None.
 *****************************************************************************/

static uint32_t eds_proc_init_msg(EDS_CB *cb, EDSV_EDS_EVT *evt)
{
	uint32_t rc = SA_AIS_OK;
	uint32_t async_rc = NCSCC_RC_SUCCESS;
	uint32_t loop_count = 0;
	SaVersionT *version = NULL;
	EDSV_MSG msg;
	EDS_CKPT_DATA ckpt;
	TRACE_ENTER2("agent dest: %" PRIx64, evt->fr_dest);

	/* Validate the version */
	version = &(evt->info.msg.info.api_info.param.init.version);
	if (!m_EDA_VER_IS_VALID(version)) {
		TRACE_2("Invalid version");
		rc = SA_AIS_ERR_VERSION;
	}

	/* Check if this EDA is a b03 client, and is on a cluster member node. 
	 */
	if (m_IS_B03_CLIENT(version)) {
		TRACE("B.03 client. HA state = %d",cb->ha_state);
		/* Are we already assigned HA_STATE and is our node DB is populated ? */
		if (cb->ha_state != EDS_HA_INIT_STATE) {
			TRACE("Checking if node is a member");
			/* Check if this node is in the cluster */
			if (!is_node_a_member(cb, evt->fr_node_id)) {
				LOG_IN("SA_AIS_ERR_UNAVAILABLE: saEvtInitialize attempted from a non-member node.");
				rc = SA_AIS_ERR_UNAVAILABLE;
			} else
				TRACE("Node is a member, allow initialize");
		}
	}

	if (rc != SA_AIS_OK) {
		/* Return a default reg_id */
		m_EDS_EDSV_INIT_MSG_FILL(msg, rc, 0)
		rc = eds_mds_msg_send(cb, &msg, &evt->fr_dest, &evt->mds_ctxt, MDS_SEND_PRIORITY_HIGH);
		if (rc != NCSCC_RC_SUCCESS) {
			LOG_ER("MDS send to %" PRIx64 " failed for saEvtInitialize() response", evt->fr_dest);
		}
		return (rc);
	}

	/*
	 * Allocating new reg_id's this way on the wild chance we wrap
	 * around the MAX_INT value and try to use a value that's still in use
	 * and valid at the begining of the range. This way we'll keep trying
	 * until we find the next open slot. But we'll only try a max of
	 * MAX_REG_RETRIES times before giving up completly and returning an error.
	 * It's highly unlikely we'll ever wrap, but at least we'll handle it.
	 */
#define MAX_REG_RETRIES 100
	loop_count = 0;
	do {
		loop_count++;

		/* Allocate a new regID */
		if (cb->last_reg_id == INT_MAX)	/* Handle integer wrap-around */ {
			LOG_IN("Integer wrap around for reg_id: %u", cb->last_reg_id);
			cb->last_reg_id = 0;
		}

		cb->last_reg_id++;

		/* Add this regid to the registration linked list. */
		rc = eds_add_reglist_entry(cb, evt->fr_dest, cb->last_reg_id);
	} while ((rc == NCSCC_RC_FAILURE) && (loop_count < MAX_REG_RETRIES));

	/* If we still have a bad status, return failure */
	if (rc != NCSCC_RC_SUCCESS) {
		/* Send response back with error code */
		rc = SA_AIS_ERR_LIBRARY;
		TRACE("SA_AIS_ERR_LIBRARY: for saEvtInitialize request %" PRIx64, evt->fr_dest);
		m_EDS_EDSV_INIT_MSG_FILL(msg, rc, 0)
		rc = eds_mds_msg_send(cb, &msg, &evt->fr_dest, &evt->mds_ctxt, MDS_SEND_PRIORITY_HIGH);
		if (rc != NCSCC_RC_SUCCESS)
			LOG_ER("MDS send to %" PRIx64 " failed for saEvtInitialize() response", evt->fr_dest);
		TRACE_LEAVE2("SA_AIS_ERR_LIBRARY");
		return (rc);
	}

	/* Send response back with assigned reg_id value */
	rc = SA_AIS_OK;
	m_EDS_EDSV_INIT_MSG_FILL(msg, rc, cb->last_reg_id)
	rc = eds_mds_msg_send(cb, &msg, &evt->fr_dest, &evt->mds_ctxt, MDS_SEND_PRIORITY_HIGH);
	if (rc != NCSCC_RC_SUCCESS)
		LOG_ER("MDS send to %" PRIx64 " failed for saEvtInitialize() response", evt->fr_dest);

	if (cb->ha_state == SA_AMF_HA_ACTIVE) {	/*Revisit this */
		memset(&ckpt, 0, sizeof(ckpt));
		m_EDSV_FILL_ASYNC_UPDATE_REG(ckpt, cb->last_reg_id, evt->fr_dest)
		async_rc = send_async_update(cb, &ckpt, NCS_MBCSV_ACT_ADD);
		if (async_rc != NCSCC_RC_SUCCESS)
			TRACE("Async Update Send failed");
	}
	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
 * Name          : eds_proc_finalize_msg
 *
 * Description   : This is the function which is called when eds receives a
 *                 EDSV_EDA_FINALIZE message.
 *
 * Arguments     : msg  - Message that was posted to the EDS Mail box.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : None.
 *****************************************************************************/

static uint32_t eds_proc_finalize_msg(EDS_CB *cb, EDSV_EDS_EVT *evt)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	EDS_CKPT_DATA ckpt;
	TRACE_ENTER2("reg_id: %u", evt->info.msg.info.api_info.param.finalize.reg_id);

	/* This call will ensure all open subscriptions, channels, and any other
	* resources allocated by this registration are freed up.
	*/
	rc = eds_remove_reglist_entry(cb, evt->info.msg.info.api_info.param.finalize.reg_id, false);
	if (rc == NCSCC_RC_SUCCESS) {
		if (cb->ha_state == SA_AMF_HA_ACTIVE) {	/*Revisit this */
			memset(&ckpt, 0, sizeof(ckpt));
			m_EDSV_FILL_ASYNC_UPDATE_FINALIZE(ckpt, evt->info.msg.info.api_info.param.finalize.reg_id)
			(void)send_async_update(cb, &ckpt, NCS_MBCSV_ACT_ADD);
		}
	} else
		TRACE("Finalize cleanup failed for agent_dest: %" PRIx64 " ,reg_id: %u", evt->fr_dest,
								 evt->info.msg.info.api_info.param.finalize.reg_id);

	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
 * Name          : eds_proc_chan_open_sync_msg
 *
 * Description   : This is the function which is called when eds receives a
 *                 EDSV_EDA_CHAN_OPEN message.
 *
 * Arguments     : msg  - Message that was posted to the EDS Mail box.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : None.
 *****************************************************************************/

static uint32_t eds_proc_chan_open_sync_msg(EDS_CB *cb, EDSV_EDS_EVT *evt)
{
	uint32_t rc = NCSCC_RC_SUCCESS, rs = NCSCC_RC_SUCCESS, async_rc = NCSCC_RC_SUCCESS;
	uint32_t chan_id;
	uint32_t chan_open_id;
	EDSV_MSG msg;
	time_t time_of_day;
	SaTimeT chan_create_time = 0;
	EDSV_EDA_CHAN_OPEN_SYNC_PARAM *open_sync_param;
	EDS_CKPT_DATA ckpt;
	TRACE_ENTER2("agent dest: %" PRIx64, evt->fr_dest);

	open_sync_param = &(evt->info.msg.info.api_info.param.chan_open_sync);

	/* Set the open  time here */
	m_NCS_OS_GET_TIME_STAMP(time_of_day);

	/* convert time_t to SaTimeT */
	chan_create_time = (SaTimeT)time_of_day *SA_TIME_ONE_SECOND;

	rs = eds_channel_open(cb,
			      open_sync_param->reg_id,
			      open_sync_param->chan_open_flags,
			      open_sync_param->chan_name.length,
			      open_sync_param->chan_name.value,
			      evt->fr_dest, &chan_id, &chan_open_id, chan_create_time);
	if (rs != SA_AIS_OK)
		TRACE("Channel Open failed for agent destination: %" PRIx64, evt->fr_dest);

	/* Send response back */
	m_EDS_EDSV_CHAN_OPEN_SYNC_MSG_FILL(msg, rs, chan_id, chan_open_id)
	rc = eds_mds_msg_send(cb, &msg, &evt->fr_dest, &evt->mds_ctxt, MDS_SEND_PRIORITY_HIGH);
	if (rc != SA_AIS_OK)
		LOG_ER("MDS send to %" PRIx64 " failed for saEvtChannelOpen()", evt->fr_dest);

	if (rs == SA_AIS_OK) {
		/* Send an Async update to STANDBY EDS peer */
		if (cb->ha_state == SA_AMF_HA_ACTIVE) {
			memset(&ckpt, 0, sizeof(ckpt));
			m_EDSV_FILL_ASYNC_UPDATE_CHAN(ckpt, open_sync_param, chan_id, chan_open_id, evt->fr_dest,
						      chan_create_time)
			async_rc = send_async_update(cb, &ckpt, NCS_MBCSV_ACT_ADD);
			if (async_rc != NCSCC_RC_SUCCESS)
				TRACE("Async Update Send failed for chan open sync msg");
		}
	}
	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
 * Name          : eds_proc_chan_open_async_msg
 *
 * Description   : This is the function which is called when eds receives a
 *                 EDSV_EDA_CHAN_OPEN_ASYNC message.
 *
 * Arguments     : msg  - Message that was posted to the EDS Mail box.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : None.
 *****************************************************************************/

static uint32_t eds_proc_chan_open_async_msg(EDS_CB *cb, EDSV_EDS_EVT *evt)
{
	uint32_t rc = NCSCC_RC_SUCCESS, rs = NCSCC_RC_SUCCESS, async_rc = NCSCC_RC_SUCCESS;
	uint32_t chan_id;
	uint32_t chan_open_id;
	EDSV_MSG msg;
	time_t time_of_day;
	SaTimeT chan_create_time = 0;
	EDSV_EDA_CHAN_OPEN_ASYNC_PARAM *open_async_param;
	EDS_CKPT_DATA ckpt;
	TRACE_ENTER2("agent dest: %" PRIx64, evt->fr_dest);

	open_async_param = &(evt->info.msg.info.api_info.param.chan_open_async);

	/* Set the open  time here */
	m_NCS_OS_GET_TIME_STAMP(time_of_day);

	/* convert time_t to SaTimeT */
	chan_create_time = (SaTimeT)time_of_day *SA_TIME_ONE_SECOND;

	rs = eds_channel_open(cb,
			      open_async_param->reg_id,
			      open_async_param->chan_open_flags,
			      open_async_param->chan_name.length,
			      open_async_param->chan_name.value,
			      evt->fr_dest, &chan_id, &chan_open_id, chan_create_time);

	if (rs != SA_AIS_OK)
		TRACE("ChannelOpenAsync failed for agent destination: %" PRIx64, evt->fr_dest);

	/* Send async response back */
	m_EDS_EDSV_CHAN_OPEN_CB_MSG_FILL(msg, open_async_param->reg_id,
					 open_async_param->inv,
					 open_async_param->chan_name,
					 chan_id, chan_open_id, open_async_param->chan_open_flags, rs)
	rc = eds_mds_msg_send(cb, &msg, &evt->fr_dest, &evt->mds_ctxt, MDS_SEND_PRIORITY_MEDIUM);
	if (rc != NCSCC_RC_SUCCESS)
		LOG_ER("MDS send to %" PRIx64 "failed for saEvtChannelOpenAsync()", evt->fr_dest);

	if (rs == SA_AIS_OK) {
		/* Send an Async update to STANDBY EDS peer */
		if (cb->ha_state == SA_AMF_HA_ACTIVE) {
			memset(&ckpt, 0, sizeof(ckpt));
			m_EDSV_FILL_ASYNC_UPDATE_CHAN(ckpt, open_async_param, chan_id, chan_open_id, evt->fr_dest,
						      chan_create_time)
			async_rc = send_async_update(cb, &ckpt, NCS_MBCSV_ACT_ADD);
			if (async_rc != NCSCC_RC_SUCCESS)
				TRACE("Async Update send failed for chan open (async) msg");
		}
	}

	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
 * Name          : eds_proc_chan_close_msg
 *
 * Description   : This is the function which is called when eds receives a
 *                 EDSV_EDA_CHAN_CLOSE message.
 *
 * Arguments     : msg  - Message that was posted to the EDS Mail box.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : None.
 *****************************************************************************/
static uint32_t eds_proc_chan_close_msg(EDS_CB *cb, EDSV_EDS_EVT *evt)
{
	uint32_t rc = NCSCC_RC_SUCCESS, async_rc = NCSCC_RC_SUCCESS;
	EDSV_EDA_CHAN_CLOSE_PARAM *close_param;
	EDS_CKPT_DATA ckpt;
	TRACE_ENTER2("agent dest: %" PRIx64, evt->fr_dest);

	close_param = &(evt->info.msg.info.api_info.param.chan_close);
	rc = eds_channel_close(cb, close_param->reg_id, close_param->chan_id, close_param->chan_open_id, false);
	if (rc == NCSCC_RC_SUCCESS) {
		/* Send an Async update to STANDBY EDS peer */
		if (cb->ha_state == SA_AMF_HA_ACTIVE) {
			memset(&ckpt, 0, sizeof(ckpt));
			m_EDSV_FILL_ASYNC_UPDATE_CCLOSE(ckpt, close_param->reg_id, close_param->chan_id,
							close_param->chan_open_id)
			async_rc = send_async_update(cb, &ckpt, NCS_MBCSV_ACT_ADD);
			if (async_rc != NCSCC_RC_SUCCESS)
				TRACE("Async Update send failed for chan close msg");
		}
	} else
		TRACE("Channel close failed for agent_dest: %" PRIx64, evt->fr_dest);
	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
 * Name          : eds_proc_chan_unlink_msg
 *
 * Description   : This is the function which is called when eds receives a
 *                 EDSV_EDA_CHAN_UNLINK message.
 *
 * Arguments     : msg  - Message that was posted to the EDS Mail box.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : None.
 *****************************************************************************/
static uint32_t eds_proc_chan_unlink_msg(EDS_CB *cb, EDSV_EDS_EVT *evt)
{
	uint32_t rc = NCSCC_RC_SUCCESS, rs = NCSCC_RC_SUCCESS, async_rc = NCSCC_RC_SUCCESS;
	EDSV_EDA_CHAN_UNLINK_PARAM *unlink_param;
	EDSV_MSG msg;
	EDS_CKPT_DATA ckpt;
	TRACE_ENTER2("agent dest: %" PRIx64, evt->fr_dest);

	unlink_param = &(evt->info.msg.info.api_info.param.chan_unlink);
	rs = eds_channel_unlink(cb, unlink_param->chan_name.length, unlink_param->chan_name.value);
	if (rs != SA_AIS_OK)
		TRACE("Channel unlink failed for :%" PRIx64, evt->fr_dest);

	/* Send response back */
	m_EDS_EDSV_CHAN_UNLINK_SYNC_MSG_FILL(msg, rs, unlink_param->chan_name)
	    rc = eds_mds_msg_send(cb, &msg, &evt->fr_dest, &evt->mds_ctxt, MDS_SEND_PRIORITY_HIGH);
	if (rc != NCSCC_RC_SUCCESS)
		LOG_ER("MDS send to %" PRIx64 " failed for saEvtChannelUnlink()", evt->fr_dest);

	if (rs == SA_AIS_OK) {	/* Send an Async update to STANDBY EDS peer */
		if (cb->ha_state == SA_AMF_HA_ACTIVE) {
			memset(&ckpt, 0, sizeof(ckpt));
			m_EDSV_FILL_ASYNC_UPDATE_CUNLINK(ckpt, unlink_param->chan_name.value,
							 unlink_param->chan_name.length)
			async_rc = send_async_update(cb, &ckpt, NCS_MBCSV_ACT_ADD);
			if (async_rc != NCSCC_RC_SUCCESS)
				TRACE("Async update send failed for unlink msg");
		}
	}
	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
 * Name          : eds_proc_publish_msg
 *
 * Description   : This is the function which is called when eds receives a
 *                 EDSV_EDA_PUBLISH message.
 *
 * Arguments     : msg  - Message that was posted to the EDS Mail box.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : None.
 *****************************************************************************/
static uint32_t eds_proc_publish_msg(EDS_CB *cb, EDSV_EDS_EVT *evt)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	uint32_t async_rc = NCSCC_RC_SUCCESS;
	uint32_t copen_id_Net;
	SaTimeT publish_time = 0;	/* placeholder for now */
	MDS_SEND_PRIORITY_TYPE prio;
	EDS_WORKLIST *wp;
	CHAN_OPEN_REC *co;
	SUBSC_REC *subrec;
	EDSV_MSG msg;
	time_t time_of_day;
	EDSV_EDA_PUBLISH_PARAM *publish_param;
	uint32_t retd_evt_chan_open_id = 0;
	EDS_CKPT_DATA ckpt;
	publish_param = &(evt->info.msg.info.api_info.param).publish;
	TRACE_ENTER2("agent dest: %" PRIx64, evt->fr_dest);

	/* Get worklist ptr for this chan */
	wp = eds_get_worklist_entry(cb->eds_work_list, publish_param->chan_id);
	if (!wp) {
		TRACE_LEAVE();
		return (NCSCC_RC_FAILURE);
	}
	/* Set the publish time here */
	m_NCS_OS_GET_TIME_STAMP(time_of_day);

	/* convert time_t to SaTimeT */
	publish_time = (SaTimeT)time_of_day;

	eds_publish_log_event(wp, publish_param, publish_time);

   /** If this event needs to be retained then 
    ** we need to validate the publisher's copen
    ** id and retain the event
    **/
	if (publish_param->retention_time > 0) {
		/* Grab the chan_open_rec for this id */
		copen_id_Net = m_NCS_OS_HTONL(publish_param->chan_open_id);
		if (NULL == (co = (CHAN_OPEN_REC *)ncs_patricia_tree_get(&wp->chan_open_rec, (uint8_t *)&copen_id_Net))) {
			TRACE_LEAVE2("patricia get failed");
			return (NCSCC_RC_FAILURE);
		}

		rc = eds_store_retained_event(cb, wp, co, publish_param, publish_time);
		if (rc != NCSCC_RC_SUCCESS) {
			TRACE_LEAVE();
			return (NCSCC_RC_FAILURE);
		} else
	 /** store the parent channel's event id 
          **/
			retd_evt_chan_open_id = co->chan_open_id;

	}

   /** Let's try to publish
    ** this event now
    **/

	/* Go through all chan_open_rec's under this channel */
	co = (CHAN_OPEN_REC *)ncs_patricia_tree_getnext(&wp->chan_open_rec, (uint8_t *)0);
	while (co) {
		subrec = co->subsc_rec_head;
		while (subrec) {	/* while subscriptions... */
			/* Does the patterns/filters match? */
			if (eds_pattern_match(publish_param->pattern_array, subrec->filters)) {

				/* Fill in the event record to send */
				m_EDS_EDSV_DELIVER_EVENT_CB_MSG_FILL(msg,
								     co->reg_id,
								     subrec->subscript_id,
								     subrec->chan_id,
								     subrec->chan_open_id,
								     publish_param->pattern_array,
								     publish_param->priority,
								     publish_param->publisher_name,
								     publish_time,
								     publish_param->retention_time,
								     publish_param->event_id,
								     retd_evt_chan_open_id,
								     publish_param->data_len, publish_param->data)

				    /* Determine evt to MDS priority mapping */
				    prio = edsv_map_ais_prio_to_mds_snd_prio(publish_param->priority);

				/* Send the event. Only once per match/per open_id */
				if (NCSCC_RC_SUCCESS != (rc = eds_mds_msg_send(cb, &msg, &co->chan_opener_dest,
									       NULL, prio))) {
					LOG_ER("Event Publish(MDS send) failed. From publisher dest: %" PRIx64
						", To subscriber dest: %" PRIx64 ",on Node_id: %u", evt->fr_dest,
					co->chan_opener_dest,  m_NCS_NODE_ID_FROM_MDS_DEST(co->chan_opener_dest)); 
				}

				break;
			}
			subrec = subrec->next;
		}
		co = (CHAN_OPEN_REC *)ncs_patricia_tree_getnext(&wp->chan_open_rec, (uint8_t *)&co->copen_id_Net);
	}

  /** If this event has been retained, send an async update &
   ** transfer memory ownership here.
   **/
	if (0 != retd_evt_chan_open_id) {
		/* Send an Async update to STANDBY EDS peer */
		if (cb->ha_state == SA_AMF_HA_ACTIVE) {
			memset(&ckpt, 0, sizeof(ckpt));
			m_EDSV_FILL_ASYNC_UPDATE_RETAIN_EVT(ckpt, publish_param, publish_time)
			async_rc = send_async_update(cb, &ckpt, NCS_MBCSV_ACT_ADD);
			if (async_rc != NCSCC_RC_SUCCESS)
				TRACE("Async Update Send Failed for Publish");
		}
		/* Now give the patternarray & data ownership to the reten record */
		publish_param->pattern_array = NULL;
		publish_param->data_len = 0;
		publish_param->data = NULL;
	}
	/* End retd event chan open id */
	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
 * Name          : eds_proc_subscribe_msg
 *
 * Description   : This is the function which is called when eds receives a
 *                 EDSV_EDA_SUBSCRIBE message.
 *
 * Arguments     : msg  - Message that was posted to the EDS Mail box.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : None.
 *****************************************************************************/
static uint32_t eds_proc_subscribe_msg(EDS_CB *cb, EDSV_EDS_EVT *evt)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	uint32_t async_rc = NCSCC_RC_SUCCESS;
	EDA_REG_REC *reglst;
	SUBSC_REC *subrec;
	EDSV_EDA_SUBSCRIBE_PARAM *subscribe_param;
	EDS_WORKLIST *channel_entry;
	EDS_RETAINED_EVT_REC *retd_evt_rec;
	MDS_SEND_PRIORITY_TYPE prio;
	EDSV_MSG msg;
	EDS_CKPT_DATA ckpt;
	SaUint8T list_iter;
	TRACE_ENTER2("agent dest: %" PRIx64, evt->fr_dest);

	subscribe_param = &(evt->info.msg.info.api_info.param.subscribe);

	/* Make sure this is a valid regID */
	reglst = eds_get_reglist_entry(cb, subscribe_param->reg_id);
	if (reglst == NULL) {
		TRACE_LEAVE();
		return (NCSCC_RC_FAILURE);
	}
	/* Allocate a new Subscription Record */
	subrec = m_MMGR_ALLOC_EDS_SUBREC(sizeof(SUBSC_REC));
	if (subrec == NULL) {
		LOG_ER("malloc failed for event subscription record");
		TRACE_LEAVE();
		return (NCSCC_RC_FAILURE);
	}
	memset(subrec, 0, sizeof(SUBSC_REC));

	/* Fill in some data */
	subrec->reg_list = reglst;
	subrec->subscript_id = subscribe_param->sub_id;
	subrec->chan_id = subscribe_param->chan_id;
	subrec->chan_open_id = subscribe_param->chan_open_id;

	/* Take ownership of the filters memory.           */
	/* We'll free it when the subscription is removed. */
	subrec->filters = subscribe_param->filter_array;

	/* Add the subscription to our internal lists */
	rc = eds_add_subscription(cb, subscribe_param->reg_id, subrec);
	if (rc != NCSCC_RC_SUCCESS) {
		subscribe_param->filter_array = NULL;
		TRACE_LEAVE();
		return (NCSCC_RC_FAILURE);	/* and return failure */
	}

   /** Find the channel entry to look for 
    ** retained events.
    **/
	channel_entry = eds_get_worklist_entry(cb->eds_work_list, subrec->chan_id);

	/* Check for any retained events in this channel 
	 * that need to be published as 
	 * they might match the subscription.
	 */
	if (NULL != channel_entry) {

		for (list_iter = SA_EVT_HIGHEST_PRIORITY; list_iter <= SA_EVT_LOWEST_PRIORITY; list_iter++) {
			retd_evt_rec = channel_entry->ret_evt_list_head[list_iter];
			while (retd_evt_rec) {
				if (eds_pattern_match(retd_evt_rec->patternArray, subscribe_param->filter_array)) {
					/* Fill in the event record to send */
					m_EDS_EDSV_DELIVER_EVENT_CB_MSG_FILL(msg,
									     subscribe_param->reg_id,
									     subrec->subscript_id,
									     subrec->chan_id,
									     subrec->chan_open_id,
									     retd_evt_rec->patternArray,
									     retd_evt_rec->priority,
									     retd_evt_rec->publisherName,
									     retd_evt_rec->publishTime,
									     retd_evt_rec->retentionTime,
									     retd_evt_rec->event_id,
									     retd_evt_rec->retd_evt_chan_open_id,
									     retd_evt_rec->data_len, retd_evt_rec->data)

					    /* Determine evt to MDS priority mapping */
					    prio = edsv_map_ais_prio_to_mds_snd_prio(retd_evt_rec->priority);
					if (NCSCC_RC_SUCCESS != (rc = eds_mds_msg_send(cb, &msg, &evt->fr_dest,
										       NULL, prio))) {
					LOG_ER("Event Publish(MDS send) failed. To subscriber dest: %" PRIx64
								", on Node_id: %u", evt->fr_dest, evt->fr_node_id);
					}

				}	/*end pattern match */
				retd_evt_rec = retd_evt_rec->next;
			}
		}
	}

	/* Send an async update to the STANDBY EDS peer */
	if (cb->ha_state == SA_AMF_HA_ACTIVE) {
		memset(&ckpt, 0, sizeof(ckpt));
		m_EDSV_FILL_ASYNC_UPDATE_SUBSCRIBE(ckpt, subscribe_param)
		async_rc = send_async_update(cb, &ckpt, NCS_MBCSV_ACT_ADD);
		if (async_rc != NCSCC_RC_SUCCESS)
			TRACE("Async Update send failed for Subscribe");
	}

	/* Transfer memory ownership to the 
	 * subscription record now 
	 */
	subscribe_param->filter_array = NULL;

	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
 * Name          : eds_proc_unsubscribe_msg
 *
 * Description   : This is the function which is called when eds receives a
 *                 EDSV_EDA_UNSUBSCRIBE message.
 *
 * Arguments     : msg  - Message that was posted to the EDS Mail box.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : None.
 *****************************************************************************/
static uint32_t eds_proc_unsubscribe_msg(EDS_CB *cb, EDSV_EDS_EVT *evt)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	uint32_t async_rc = NCSCC_RC_SUCCESS;
	EDSV_EDA_UNSUBSCRIBE_PARAM *unsubscribe_param;
	EDS_CKPT_DATA ckpt;
	TRACE_ENTER2("agent dest: %" PRIx64, evt->fr_dest);

	unsubscribe_param = &(evt->info.msg.info.api_info.param.unsubscribe);
	/* Remove subscription from our lists */
	rc = eds_remove_subscription(cb,
				     unsubscribe_param->reg_id,
				     unsubscribe_param->chan_id,
				     unsubscribe_param->chan_open_id, unsubscribe_param->sub_id);
	if (rc == NCSCC_RC_SUCCESS) {
		/* Send an async checkpoint update to STANDBY EDS */
		if (cb->ha_state == SA_AMF_HA_ACTIVE) {
			memset(&ckpt, 0, sizeof(ckpt));
			m_EDSV_FILL_ASYNC_UPDATE_UNSUBSCRIBE(ckpt, unsubscribe_param)
			async_rc = send_async_update(cb, &ckpt, NCS_MBCSV_ACT_ADD);
			if (async_rc != NCSCC_RC_SUCCESS)
				TRACE("Async Update send failed for unsubscribe");
		}
	} else
		rc = NCSCC_RC_FAILURE;

	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
 * Name          : eds_proc_retention_time_clr_msg
 *
 * Description   : This is the function which is called when eds receives a
 *                 EDSV_EDA_RETENTION_TIME_CLR message.
 *
 * Arguments     : msg  - Message that was posted to the EDS Mail box.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : None.
 *****************************************************************************/
static uint32_t eds_proc_retention_time_clr_msg(EDS_CB *cb, EDSV_EDS_EVT *evt)
{
	uint32_t rc = NCSCC_RC_SUCCESS, rs = NCSCC_RC_SUCCESS;
	uint32_t async_rc = NCSCC_RC_SUCCESS;
	EDSV_MSG msg;

	EDSV_EDA_RETENTION_TIME_CLR_PARAM *param = &(evt->info.msg.info.api_info.param.rettimeclr);
	EDS_CKPT_DATA ckpt;
	TRACE_ENTER2("agent dest: %" PRIx64, evt->fr_dest);

	/* Lock the EDS_CB */
	m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE);

	rc = eds_clear_retained_event(cb, param->chan_id, param->chan_open_id, param->event_id, false);
	if (rc != NCSCC_RC_SUCCESS)
		TRACE("Retained event clear failed");

	/* Unlock the EDS_CB */
	m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);

	/* Send response back */
	m_EDS_EDSV_CHAN_RETENTION_TMR_CLEAR_SYNC_MSG_FILL(msg, rc)
	rs = eds_mds_msg_send(cb, &msg, &evt->fr_dest, &evt->mds_ctxt, MDS_SEND_PRIORITY_HIGH);
		LOG_IN("MDS send to %" PRIx64 " failed for saEvtEventRetentionTimeClear()", evt->fr_dest);

	if (rs != NCSCC_RC_SUCCESS)

	if (rc == NCSCC_RC_SUCCESS) {
		/*Send an async checkpoint update to STANDBY EDS peer */
		if (cb->ha_state == SA_AMF_HA_ACTIVE) {
			memset(&ckpt, 0, sizeof(ckpt));
			m_EDSV_FILL_ASYNC_UPDATE_RETEN_CLEAR(ckpt, param)
			async_rc = send_async_update(cb, &ckpt, NCS_MBCSV_ACT_ADD);
			if (async_rc != NCSCC_RC_SUCCESS)
				TRACE("Async Update send failed for retentime clear msg");
		}
	}
	TRACE_LEAVE();
	return rs;
}

/****************************************************************************
 * Name          : eds_proc_limit_get_msg
 *
 * Description   : This function is called when eds receives a limit_get 
 *                 message from the EDA.
 * Arguments     : msg  - Message that was posted to the EDS Mail box.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : None.
 *****************************************************************************/

static uint32_t eds_proc_limit_get_msg(EDS_CB *cb, EDSV_EDS_EVT *evt)
{
	uint32_t rc = SA_AIS_OK;
	EDSV_MSG msg;
	TRACE_ENTER2("agent dest: %" PRIx64, evt->fr_dest);

	m_EDS_EDSV_LIMIT_GET_MSG_FILL(msg, rc)
	rc = eds_mds_msg_send(cb, &msg, &evt->fr_dest, &evt->mds_ctxt, MDS_SEND_PRIORITY_HIGH);
	if (rc != NCSCC_RC_SUCCESS)
		LOG_ER("MDS send to %" PRIx64 "failed for saEvtLimitGet()", evt->fr_dest);

	return (rc);

}

/****************************************************************************
 * Name          : eds_proc_unexpected_msg
 *
 * Description   : This is the function which is called when EDS receives a
 *                 an unexpected message which not expected to be sent to the
 *                 EDS.
 *
 * Arguments     : msg  - Message that was posted to the EDS Mail box.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : None.
 *****************************************************************************/
static uint32_t eds_proc_unexpected_msg(EDS_CB *cb, EDSV_EDS_EVT *evt)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	TRACE("Unexpected message");
	return rc;
}

/****************************************************************************
 * Name          : eds_proc_invalid_msg
 *
 * Description   : This is the function which is called when EDS receives a
 *                 an unexpected message which not expected to be sent to the
 *                 EDS.
 *
 * Arguments     : msg  - Message that was posted to the EDS Mail box.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : None.
 *****************************************************************************/
static uint32_t eds_proc_invalid_msg(EDSV_EDS_EVT *evt)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	TRACE("Invalid message");
	return rc;
}

/****************************************************************************
 * Name          : eds_proc_eda_misc_msg
 *
 * Description   : This is the function which is called when eds receives any
 *                 internal messages from EDA which are not directly API invocation
 *                 related.
 *
 * Arguments     : msg  - Message that was posted to the EDS Mail box.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : None.
 *****************************************************************************/

static uint32_t eds_proc_eda_misc_msg(EDSV_EDS_EVT *evt)
{
   /** 
    ** Currently there are no messages of this 
    ** category, so just destroy the message
    ** and return. This must have been dispatched in error.
    **/
	TRACE("Miscellaneous message");
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
 * Name          : eds_proc_ret_tmr_exp_evt
 *
 * Description   : This is the function which is called when eds receives any
 *                 a retention tmr expiry evt
 *
 * Arguments     : evt  - Evt that was posted to the EDS Mail box.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : None.
 *****************************************************************************/
static uint32_t eds_proc_ret_tmr_exp_evt(EDSV_EDS_EVT *evt)
{
	EDS_RETAINED_EVT_REC *ret_evt;
	uint32_t rc = NCSCC_RC_SUCCESS;
	EDS_CB *eds_cb;
	TRACE_ENTER();

	/* retrieve retained evt */
	if (NULL == (eds_cb = (EDS_CB *)ncshm_take_hdl(NCS_SERVICE_ID_EDS, evt->cb_hdl))) {
		TRACE_LEAVE2("take handle failed for cb");
		return NCSCC_RC_FAILURE;
	}

	/* retrieve retained evt */
	if (NULL == (ret_evt = (EDS_RETAINED_EVT_REC *)ncshm_take_hdl(NCS_SERVICE_ID_EDS, evt->info.tmr_info.opq_hdl))) {
		TRACE_LEAVE2("take handle failed for retained evt rec.");
		return NCSCC_RC_FAILURE;
	}

	m_NCS_LOCK(&eds_cb->cb_lock, NCS_LOCK_WRITE);

   /** store the chan id as we would need it
    ** after the event is freed
    **/

/* CHECKPOINT:
   if ( EDS_CB->ha_state == standby)
        compose a EDSV_CKPT_RETENTION_TIME_CLEAR_MSG and send to standby peer. 
*/
   /** This also frees the event **/
	rc = eds_clear_retained_event(eds_cb,
				      ret_evt->chan_id, ret_evt->retd_evt_chan_open_id, ret_evt->event_id, true);

	m_NCS_UNLOCK(&eds_cb->cb_lock, NCS_LOCK_WRITE);

	if (NCSCC_RC_SUCCESS != rc) {
		ncshm_give_hdl(evt->info.tmr_info.opq_hdl);
		ncshm_give_hdl(evt->cb_hdl);
		TRACE_LEAVE();
		return rc;
	}

	ncshm_give_hdl(evt->cb_hdl);
	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
 * Name          : eds_proc_eda_updn_mds_msg
 *
 * Description   : This is the function which is called when eds receives any
 *                 a EDA UP/DN message via MDS subscription.
 *
 * Arguments     : msg  - Message that was posted to the EDS Mail box.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : None.
 *****************************************************************************/

static uint32_t eds_proc_eda_updn_mds_msg(EDSV_EDS_EVT *evt)
{
	EDS_CB *cb = NULL;
	EDS_CKPT_DATA ckpt;
	uint32_t async_rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER2("agent dest: %" PRIx64, evt->fr_dest);

	/* Retreive the cb handle */
	if (NULL == (cb = (EDS_CB *)ncshm_take_hdl(NCS_SERVICE_ID_EDS, evt->cb_hdl))) {
		TRACE_LEAVE();
		return NCSCC_RC_FAILURE;
	}

	switch (evt->evt_type) {
	case EDSV_EDS_EVT_EDA_UP:
		TRACE("Agent UP");
		break;
	case EDSV_EDS_EVT_EDA_DOWN:
		TRACE("Agent DOWN");
		if ((cb->ha_state == SA_AMF_HA_ACTIVE) || (cb->ha_state == SA_AMF_HA_QUIESCED)) {
			/* Remove this EDA entry from our processing lists */
			eds_remove_regid_by_mds_dest(cb, evt->fr_dest);
			/*Send an async checkpoint update to STANDBY EDS peer */
			if (cb->ha_state == SA_AMF_HA_ACTIVE) {
				memset(&ckpt, 0, sizeof(ckpt));
				m_EDSV_FILL_ASYNC_UPDATE_AGENT_DOWN(ckpt, evt->fr_dest)
				async_rc = send_async_update(cb, &ckpt, NCS_MBCSV_ACT_ADD);
				if (async_rc != NCSCC_RC_SUCCESS)
					TRACE("Agent down async update send failed");
			TRACE("sent async update to standby(if present)");
			}
		} else if (cb->ha_state == SA_AMF_HA_STANDBY) {
			EDA_DOWN_LIST *eda_down_rec = NULL;
			if (eds_eda_entry_valid(cb, evt->fr_dest)) {
				if (NULL == (eda_down_rec = m_MMGR_ALLOC_EDA_DOWN_LIST(sizeof(EDA_DOWN_LIST)))) {
					/* Log it */
					LOG_ER("malloc failed for EDA down list");
					break;
				}
				memset(eda_down_rec, 0, sizeof(EDA_DOWN_LIST));
				eda_down_rec->mds_dest = evt->fr_dest;
				if (cb->eda_down_list_head == NULL) {
					cb->eda_down_list_head = eda_down_rec;
				} else {
					if (cb->eda_down_list_tail)
						cb->eda_down_list_tail->next = eda_down_rec;
				}
				cb->eda_down_list_tail = eda_down_rec;
			}
		}

/* CHECKPOINT:
    1) Add a MDS_UP_DOWN_FLAG==(TIMER_STARTED | TIMER_RESET) in EDS_CB.
    2) if ( MDS_UP_DOWN_FLAG == TIMER_RESET)
            start the mds_up_down_timer
       else
            do nothing
    3) In the mds_up_down_timer_timeout_handler(), invoke eds_remove_regid_by_mds_dest(). 
*/
		break;
	default:
		break;
	}

	/* Give the cb handle back */
	ncshm_give_hdl(evt->cb_hdl);

	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
 * Name          : eds_proc_eda_api_msg
 *
 * Description   : This is the function which is called when eds receives any
 *                 EDA_API message.
 *
 * Arguments     : msg  - Message that was posted to the EDS Mail box.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : None.
 *****************************************************************************/

static uint32_t eds_proc_eda_api_msg(EDSV_EDS_EVT *evt)
{
	EDS_CB *cb;
	TRACE_ENTER();

	if ((evt->info.msg.info.api_info.type >= EDSV_API_BASE_MSG) &&
	    (evt->info.msg.info.api_info.type < EDSV_API_MAX)) {
		/* Retrieve the cb handle */
		if (NULL == (cb = (EDS_CB *)ncshm_take_hdl(NCS_SERVICE_ID_EDS, evt->cb_hdl))) {
			TRACE_LEAVE();
			return NCSCC_RC_FAILURE;
		}

		if (eds_eda_api_msg_dispatcher[evt->info.msg.info.api_info.type] (cb, evt) != NCSCC_RC_SUCCESS) {
			ncshm_give_hdl(evt->cb_hdl);
			TRACE_LEAVE();
			return NCSCC_RC_FAILURE;
		}

		/* Give the cb handle back */
		ncshm_give_hdl(evt->cb_hdl);

	} else
		LOG_ER("Invalid API event received from agent dest %" PRIx64, evt->fr_dest);

	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
 * Name          : eds_process_api_evt
 *
 * Description   : This is the function which is called when eds receives an
 *                 event either because of an API Invocation or other internal
 *                 messages from EDA clients
 *
 * Arguments     : evt  - Message that was posted to the EDS Mail box.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : None.
 *****************************************************************************/
static uint32_t eds_process_api_evt(EDSV_EDS_EVT *evt)
{
	TRACE_ENTER();
	if (evt->evt_type == EDSV_EDS_EDSV_MSG) {
		if ((evt->info.msg.type >= EDSV_BASE_MSG) && (evt->info.msg.type < EDSV_MSG_MAX)) {
			if (eds_edsv_evt_dispatch_tbl[evt->info.msg.type] (evt) != NCSCC_RC_SUCCESS) {
			}
		}
	}

	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
 * Name          : eds_proc_quiesced_ack_evt
 *
 * Description   : This is the function which is called when eds receives an
 *                       quiesced ack event from MDS 
 *
 * Arguments     : evt  - Message that was posted to the EDS Mail box.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : None.
 *****************************************************************************/
static uint32_t eds_proc_quiesced_ack_evt(EDSV_EDS_EVT *evt)
{
	EDS_CB *cb;
	TRACE_ENTER();

	/* Retrieve the cb handle */
	if (NULL == (cb = (EDS_CB *)ncshm_take_hdl(NCS_SERVICE_ID_EDS, evt->cb_hdl))) {
		TRACE_LEAVE();
		return NCSCC_RC_FAILURE;
	}

	if (cb->is_quisced_set == true) {
		cb->ha_state = SA_AMF_HA_QUIESCED;
		/* Inform MBCSV of HA state change */
		if (eds_mbcsv_change_HA_state(cb) != NCSCC_RC_SUCCESS) {
			TRACE("EDS-MBCSV change role failed");
		}

		/* Update control block */
		saAmfResponse(cb->amf_hdl, cb->amf_invocation_id, SA_AIS_OK);
		cb->is_quisced_set = false;
		/* Give the cb handle back */
		ncshm_give_hdl(evt->cb_hdl);
	} else {
		TRACE("Quiesced set is false");
		ncshm_give_hdl(evt->cb_hdl);
	}

	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
 * Name          : eds_process_evt
 *
 * Description   : This is the function which is called when eds receives an
 *                 event of any kind.
 *
 * Arguments     : msg  - Message that was posted to the EDS Mail box.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : None.
 *****************************************************************************/
uint32_t eds_process_evt(EDSV_EDS_EVT *evt)
{
	EDS_CB *cb;
	TRACE_ENTER();

	/* Retrieve the cb handle */
	if (NULL == (cb = (EDS_CB *)ncshm_take_hdl(NCS_SERVICE_ID_EDS, evt->cb_hdl))) {
		TRACE_LEAVE();
		return NCSCC_RC_FAILURE;
	}

	if (cb->ha_state == SA_AMF_HA_ACTIVE) {
		if ((evt->evt_type >= EDSV_EDS_EVT_BASE) && (evt->evt_type <= EDSV_EDS_RET_TIMER_EXP)) {
	  /** Invoke the evt dispatcher **/
			eds_edsv_top_level_evt_dispatch_tbl[evt->evt_type] (evt);
		} else if (evt->evt_type == EDSV_EVT_QUIESCED_ACK) {
			eds_proc_quiesced_ack_evt(evt);
		} else
			TRACE("Event processing failed");
	} else {
		if ((evt->evt_type == EDSV_EDS_RET_TIMER_EXP) || (evt->evt_type == EDSV_EDS_EVT_EDA_DOWN))
	    /** Invoke the evt dispatcher **/
			eds_edsv_top_level_evt_dispatch_tbl[evt->evt_type] (evt);
	}

	ncshm_give_hdl(evt->cb_hdl);
	/* Free the event */
	if (NULL != evt)
		eds_evt_destroy(evt);

	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
 * Name          : eds_evt_destroy
 *
 * Description   : This is the function which is called to destroy an EDS event.
 *
 * Arguments     : struct edsv_eds_evt_tag *.
 *
 * Return Values : NONE
 *
 * Notes         : None.
 *****************************************************************************/
void eds_evt_destroy(EDSV_EDS_EVT *evt)
{
	TRACE_ENTER();

	if (EDSV_EDS_EDSV_MSG == evt->evt_type) {
		if (EDSV_EDA_API_MSG == evt->info.msg.type) {
			if (EDSV_EDA_PUBLISH == evt->info.msg.info.api_info.type) {
				/* free the pattern array */
				if (NULL != evt->info.msg.info.api_info.param.publish.pattern_array)
					edsv_free_evt_pattern_array(evt->info.msg.info.api_info.param.publish.
								    pattern_array);

				/* free event data */
				if (NULL != evt->info.msg.info.api_info.param.publish.data)
					m_MMGR_FREE_EDSV_EVENT_DATA(evt->info.msg.info.api_info.param.publish.data);

			} else if (EDSV_EDA_SUBSCRIBE == evt->info.msg.info.api_info.type) {
				/* free the filter array */
				if (NULL != evt->info.msg.info.api_info.param.subscribe.filter_array)
					edsv_free_evt_filter_array(evt->info.msg.info.api_info.param.subscribe.
								   filter_array);
			}
		}
	}

   /** There are no other pointers 
    ** off the evt, so free the evt
    **/
	m_MMGR_FREE_EDSV_EDS_EVT(evt);
	evt = NULL;

	TRACE_LEAVE();
	return;
}

/****************************************************************************
 * Name          : eds_publish_log_event
 *
 * Description   : Logs header info for a published event.
 *
 * Arguments     : worklist pointer to the event channel.
 *                 publish_param of the event.
 *                 publish time of the event.
 *
 * Return Values : NONE
 *
 * Notes         : None.
 *****************************************************************************/
static void eds_publish_log_event(EDS_WORKLIST *wp, EDSV_EDA_PUBLISH_PARAM *publish_param, SaTimeT publish_time)
{
	uint32_t x;
	uint32_t is_ascii;
	char *ptr;
	char str[524] = { 0 };
	TRACE_ENTER();

	/* See if the channel name is a printable string.
	 * If not, just give the length for now.
	 */
	is_ascii = true;
	ptr = (char *)wp->cname;
	for (x = 0; x < wp->cname_len; x++) {
		if (isascii(*ptr++))
			continue;
		else {
			is_ascii = false;
			break;
		}
	}

	if (is_ascii)
		snprintf(str, sizeof(str) - 1, "Channel:%s, ", wp->cname);
	else
		snprintf(str, sizeof(str) - 1, "Channel:<BINARY VALUE Length:%d>, ", wp->cname_len);

	/*
	 *  <<< TODO >>>
	 * For now we append the channel name and publisher name together.
	 * This should be split when the new logging format NCSFL_TYPE_TICCLLL
	 * is added. Also will need to modify the format string in eds_logstr.c
	 */

	/* See if the publisher name is a printable string.
	 * If not, just say it's binary of some length.
	 */
	is_ascii = true;
	ptr = (char *)publish_param->publisher_name.value;
	for (x = 0; x < publish_param->publisher_name.length; x++) {
		if (isascii(*ptr++))
			continue;
		else {
			is_ascii = false;
			break;
		}
	}

	ptr = &str[strlen(str)];	/* Get to end of previous string */
	if (is_ascii)
		snprintf(ptr, sizeof(str) - strlen(str) - 1, "Publisher:%s", publish_param->publisher_name.value);
	else
		snprintf(ptr, sizeof(str) - strlen(str) - 1, "Publisher:<BINARY VALUE Length:%d>, ",
			 publish_param->publisher_name.length);

	TRACE("%s : event_id: %u, publish_time: %llx, reten time: %llu",
						(int8_t *)str, publish_param->event_id, publish_time,
						publish_param->retention_time);
	TRACE_LEAVE();
}

