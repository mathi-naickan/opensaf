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

/*****************************************************************************
  FILe NAME: immd_evt.c

  DESCRIPTION: IMMD Event handling routines

  FUNCTIONS INCLUDED in this module:
  immd_process_evt .........IMMD Event processing routine.
******************************************************************************/

#include "immsv.h"
#include "immsv_evt.h"
#include "immd.h"
#include "ncssysf_mem.h"

uns32 immd_evt_proc_cb_dump(IMMD_CB *cb);

static uns32 immd_evt_proc_immnd_intro(IMMD_CB *cb, IMMD_EVT *evt, IMMSV_SEND_INFO *sinfo);
static uns32 immd_evt_proc_immnd_req_sync(IMMD_CB *cb, IMMD_EVT *evt, IMMSV_SEND_INFO *sinfo);
static uns32 immd_evt_proc_immnd_announce_load(IMMD_CB *cb, IMMD_EVT *evt, IMMSV_SEND_INFO *sinfo);
static uns32 immd_evt_proc_immnd_announce_sync(IMMD_CB *cb, IMMD_EVT *evt, IMMSV_SEND_INFO *sinfo);

static uns32 immd_evt_proc_immnd_abort_sync(IMMD_CB *cb, IMMD_EVT *evt, IMMSV_SEND_INFO *sinfo);

static uns32 immd_evt_proc_immnd_loading_failed(IMMD_CB *cb, IMMD_EVT *evt, IMMSV_SEND_INFO *sinfo);

static uns32 immd_evt_proc_immnd_prto_purge_mutations(IMMD_CB *cb, IMMD_EVT *evt, IMMSV_SEND_INFO *sinfo);

static uns32 immd_evt_proc_immnd_announce_dump(IMMD_CB *cb, IMMD_EVT *evt, IMMSV_SEND_INFO *sinfo);
static uns32 immd_evt_proc_adminit_req(IMMD_CB *cb, IMMD_EVT *evt, IMMSV_SEND_INFO *sinfo);
static uns32 immd_evt_proc_impl_set_req(IMMD_CB *cb, IMMD_EVT *evt, IMMSV_SEND_INFO *sinfo);
static uns32 immd_evt_proc_ccbinit_req(IMMD_CB *cb, IMMD_EVT *evt, IMMSV_SEND_INFO *sinfo);
static uns32 immd_evt_proc_rt_modify_req(IMMD_CB *cb, IMMD_EVT *evt, IMMSV_SEND_INFO *sinfo);
static uns32 immd_evt_proc_discard_impl(IMMD_CB *cb, IMMD_EVT *evt, IMMSV_SEND_INFO *sinfo);
static uns32 immd_evt_proc_abort_ccb(IMMD_CB *cb, IMMD_EVT *evt, IMMSV_SEND_INFO *sinfo);
static uns32 immd_evt_proc_admo_hard_finalize(IMMD_CB *cb, IMMD_EVT *evt, IMMSV_SEND_INFO *sinfo);

static uns32 immd_evt_proc_mds_evt(IMMD_CB *cb, IMMD_EVT *evt);

static uns32 immd_evt_mds_quiesced_ack_rsp(IMMD_CB *cb, IMMD_EVT *evt, IMMSV_SEND_INFO *sinfo);
static uns32 immd_evt_proc_lga_callback(IMMD_CB *cb, IMMD_EVT *evt);

/****************************************************************************
 * Name          : immd_process_evt
 *
 * Description   : This is the top level function to process the events posted
 *                  to IMMD.
 * Arguments     : 
 *   evt          : Pointer to IMMSV_EVT
 * Return Values : None
 *
 * Notes         : None.
 ***************************************************************************/
void immd_process_evt(void)
{
	IMMD_CB *cb = immd_cb;
	uns32 rc = NCSCC_RC_SUCCESS;
	IMMSV_EVT *evt;

	TRACE_ENTER();
	evt = (IMMSV_EVT *)ncs_ipc_non_blk_recv(&immd_cb->mbx);

	if (evt == NULL) {
		LOG_ER("No mbx message although indicated in fd!");
		TRACE_LEAVE();
		return;
	}

	if (evt->type != IMMSV_EVT_TYPE_IMMD) {
		LOG_ER("Received a non IMMD message!");
		TRACE_LEAVE();
		return;
	}

	immsv_msg_trace_rec(evt->sinfo.dest, evt);

	switch (evt->info.immd.type) {
	case IMMD_EVT_MDS_INFO:
		rc = immd_evt_proc_mds_evt(cb, &evt->info.immd);
		break;
		/*
		   case IMMD_EVT_TIME_OUT:
		   rc = immd_evt_proc_timer_expiry(cb, &evt->info.immd);
		   break;
		 */
	case IMMD_EVT_ND2D_INTRO:
		rc = immd_evt_proc_immnd_intro(cb, &evt->info.immd, &evt->sinfo);
		break;
	case IMMD_EVT_ND2D_REQ_SYNC:
		rc = immd_evt_proc_immnd_req_sync(cb, &evt->info.immd, &evt->sinfo);
		break;
	case IMMD_EVT_ND2D_ANNOUNCE_LOADING:
		rc = immd_evt_proc_immnd_announce_load(cb, &evt->info.immd, &evt->sinfo);
		break;
	case IMMD_EVT_ND2D_SYNC_START:
		rc = immd_evt_proc_immnd_announce_sync(cb, &evt->info.immd, &evt->sinfo);
		break;
	case IMMD_EVT_ND2D_SYNC_ABORT:
		rc = immd_evt_proc_immnd_abort_sync(cb, &evt->info.immd, &evt->sinfo);
		break;
	case IMMD_EVT_ND2D_LOADING_FAILED:
		rc = immd_evt_proc_immnd_loading_failed(cb, &evt->info.immd, &evt->sinfo);
		break;
	case IMMD_EVT_ND2D_PBE_PRTO_PURGE_MUTATIONS:
		rc = immd_evt_proc_immnd_prto_purge_mutations(cb, &evt->info.immd,
			&evt->sinfo);
		break;
	case IMMD_EVT_ND2D_ANNOUNCE_DUMP:
		rc = immd_evt_proc_immnd_announce_dump(cb, &evt->info.immd, &evt->sinfo);
		break;
	case IMMD_EVT_ND2D_ADMINIT_REQ:
		rc = immd_evt_proc_adminit_req(cb, &evt->info.immd, &evt->sinfo);
		break;
	case IMMD_EVT_ND2D_IMPLSET_REQ:
		rc = immd_evt_proc_impl_set_req(cb, &evt->info.immd, &evt->sinfo);
		break;
	case IMMD_EVT_ND2D_DISCARD_IMPL:
		rc = immd_evt_proc_discard_impl(cb, &evt->info.immd, &evt->sinfo);
		break;
	case IMMD_EVT_ND2D_ABORT_CCB:
		rc = immd_evt_proc_abort_ccb(cb, &evt->info.immd, &evt->sinfo);
		break;
	case IMMD_EVT_ND2D_ADMO_HARD_FINALIZE:
		rc = immd_evt_proc_admo_hard_finalize(cb, &evt->info.immd, &evt->sinfo);
		break;
	case IMMD_EVT_ND2D_CCBINIT_REQ:
		rc = immd_evt_proc_ccbinit_req(cb, &evt->info.immd, &evt->sinfo);
		break;
	case IMMD_EVT_ND2D_OI_OBJ_MODIFY:
		rc = immd_evt_proc_rt_modify_req(cb, &evt->info.immd, &evt->sinfo);
		break;
	case IMMD_EVT_ND2D_FEVS_REQ:
		rc = immd_evt_proc_fevs_req(cb, &evt->info.immd, &evt->sinfo, TRUE);
		break;
	case IMMD_EVT_MDS_QUIESCED_ACK_RSP:
		rc = immd_evt_mds_quiesced_ack_rsp(cb, &evt->info.immd, &evt->sinfo);
		break;

	case IMMD_EVT_CB_DUMP:
		rc = immd_evt_proc_cb_dump(cb);
		break;
	case IMMD_EVT_LGA_CB:
		rc = immd_evt_proc_lga_callback(cb, &evt->info.immd);
		break;
	default:
		/* Log the error TBD */
		TRACE_5("UNRECOGNIZED IMMD EVENT");
		break;
	}

	if (rc != NCSCC_RC_SUCCESS) {
		LOG_WA("Error returned from processing message err:%u msg-type:%u", rc, evt->info.immd.type);
	}

	/* Free the Event */
	free(evt);
	TRACE_LEAVE();
	return;
}

static uns32 immd_immnd_guard(IMMD_CB *cb, MDS_DEST *dest)
{
	IMMD_IMMND_INFO_NODE *node_info = NULL;

	immd_immnd_info_node_get(&cb->immnd_tree, dest, &node_info);

	if (!node_info) {
		TRACE_5("No node found for dest: reject call");
		return 0;
	}

	if (node_info->immnd_execPid == 0) {
		TRACE_5("immnd_execPid == 0: reject call");
		return 0;
	}

	return 1;
}

/****************************************************************************
 * Name          : immd_evt_proc_fevs_req
 *
 * Description   : Function to process the IMMD_EVT_ND2D_FEVS_REQ event 
 *                 from IMMND (or from IMMD in some calls requiring dedicated
 *                 processing). 
 *
 * Arguments     : IMMD_CB *cb - IMMD CB pointer
 *                 IMMSV_EVT *evt - Received Event structure
 *
 * Return Values : NCSCC_RC_SUCCESS/Error.
 *****************************************************************************/

uns32 immd_evt_proc_fevs_req(IMMD_CB *cb, IMMD_EVT *evt, IMMSV_SEND_INFO *sinfo, NCS_BOOL deallocate)
{
	IMMSV_EVT send_evt;
	IMMD_MBCSV_MSG mbcp_msg;
	uns32 proc_rc = NCSCC_RC_SUCCESS;
	IMMSV_FEVS *fevs_req = &evt->info.fevsReq;
	uns8 isResend = FALSE;
	TRACE_ENTER();

	/* First check that source IMMND is in legal state 
	   If sinfo is NULL then the sender is the active IMMD itself
	   and we always accept that sender. It may also be the standby
	   IMMD in a failover situation.
	 */

	if (sinfo && !immd_immnd_guard(cb, &sinfo->dest)) {
		/*Send reply to IMMND telling it it must restart/sync? */
		TRACE_LEAVE();
		return 0;
	}

	/* Populate & Send the FEVS Event to IMMND */
	memset(&send_evt, 0, sizeof(IMMSV_EVT));
	send_evt.type = IMMSV_EVT_TYPE_IMMND;
	send_evt.info.immnd.type = IMMND_EVT_D2ND_GLOB_FEVS_REQ;
	if ((evt->type == 0) && (fevs_req->sender_count > 0)) {
		TRACE_5("Re-sending fevs message %llu", fevs_req->sender_count);
		send_evt.info.immnd.info.fevsReq.sender_count = fevs_req->sender_count;
		isResend = TRUE;
	} else {
		send_evt.info.immnd.info.fevsReq.sender_count = ++(cb->fevsSendCount);
	}
	send_evt.info.immnd.info.fevsReq.reply_dest = fevs_req->reply_dest;
	send_evt.info.immnd.info.fevsReq.client_hdl = fevs_req->client_hdl;
	send_evt.info.immnd.info.fevsReq.msg.size = fevs_req->msg.size;
	/*Borrow the buffer from the input message instead of copying */
	send_evt.info.immnd.info.fevsReq.msg.buf = fevs_req->msg.buf;

	memset(&mbcp_msg, 0, sizeof(IMMD_MBCSV_MSG));
	mbcp_msg.type = IMMD_A2S_MSG_FEVS;
	mbcp_msg.info.fevsReq = send_evt.info.immnd.info.fevsReq;

	TRACE_5("immd_evt_proc_fevs_req send_count:%llu size:%u",
		send_evt.info.immnd.info.fevsReq.sender_count, send_evt.info.immnd.info.fevsReq.msg.size);

	if (isResend) {
		LOG_IN("Resend of fevs message %llu, will not mbcp to peer IMMD",
		       send_evt.info.immnd.info.fevsReq.sender_count);
	} else {
		/*Checkpoint the message to standby director. 
		   Syncronous call=>wait for ack */
		proc_rc = immd_mbcsv_sync_update(cb, &mbcp_msg);

		if (proc_rc != NCSCC_RC_SUCCESS) {
			/* This case should be handled better.
			   Reply with try again to ND ?
			   We apparently failed to replicate the message to stby.
			   This could be because the standby is crashed ?
			   Is there any way to check this  in the cb ?
			   In any case we do NOT want to disturb this active IMMD
			 */
			LOG_ER("failed to replicate message to stdby send_count:%llu",
			       send_evt.info.immnd.info.fevsReq.sender_count);
			/*Revert the fevs count since we will not send this message. */
			--(cb->fevsSendCount);
			TRACE_LEAVE();
			return proc_rc;
		}
	}

	/* See ticket #523, uncomment to generate the case
	   if(sinfo && cb->fevsSendCount == 101) {
	   LOG_ER("Fault injected for fevs msg 101 - exiting");
	   exit(1);
	   }
	 */

	/* Also save the message in cb for resends untill all NDs have pushed
	   their counts passed thiss messageNo. */

	proc_rc = immd_mds_bcast_send(cb, &send_evt, NCSMDS_SVC_ID_IMMND);

	if (proc_rc != NCSCC_RC_SUCCESS) {
		/*
		   This is a problematic error case. 
		   The director apparently has problems with MDS.
		   We can retry a few times, but how many ?
		   We dont actually know if the message was sent to some IMMNDs but not 
		   others. If we implement the re-sending mechanism, we could actually 
		   do nothing and rely on it for catch-up.
		   For now we exit, the idea being to make this case obvious
		   should it ever occurr.
		   Possibly we should try to tell the standby immd what has happened.
		 */
		LOG_ER("Failure in sending of fevs broadcast- exiting");
		sleep(2);
		exit(1);
	}

	if (deallocate) {
		free(fevs_req->msg.buf);
		fevs_req->msg.buf = NULL;
		fevs_req->msg.size = 0;
	}

	TRACE_LEAVE();
	return proc_rc;
}

static void immd_start_sync_ok(IMMD_CB *cb, SaUint32T rulingEpoch, IMMD_IMMND_INFO_NODE *node_info)
{
	uns32 proc_rc = NCSCC_RC_SUCCESS;
	IMMSV_EVT sync_evt;
	IMMD_MBCSV_MSG mbcp_msg;
	TRACE_ENTER();

	memset(&sync_evt, 0, sizeof(IMMSV_EVT));
	memset(&mbcp_msg, 0, sizeof(IMMD_MBCSV_MSG));

	node_info->syncStarted = TRUE;

	sync_evt.type = IMMSV_EVT_TYPE_IMMND;
	sync_evt.info.immnd.type = IMMND_EVT_D2ND_SYNC_START;
	sync_evt.info.immnd.info.ctrl.rulingEpoch = cb->mRulingEpoch;
	sync_evt.info.immnd.info.ctrl.fevsMsgStart = cb->fevsSendCount;
	sync_evt.info.immnd.info.ctrl.nodeId = node_info->immnd_key;
	sync_evt.info.immnd.info.ctrl.canBeCoord = node_info->isOnController;
	sync_evt.info.immnd.info.ctrl.ndExecPid = node_info->immnd_execPid;
	sync_evt.info.immnd.info.ctrl.isCoord = node_info->isCoord;
	sync_evt.info.immnd.info.ctrl.syncStarted = node_info->syncStarted;
	sync_evt.info.immnd.info.ctrl.nodeEpoch = node_info->epoch;
	sync_evt.info.immnd.info.ctrl.pbeEnabled = 
		(cb->mRim == SA_IMM_KEEP_REPOSITORY);

	mbcp_msg.type = IMMD_A2S_MSG_SYNC_START;
	mbcp_msg.info.ctrl = sync_evt.info.immnd.info.ctrl;

	proc_rc = immd_mbcsv_sync_update(cb, &mbcp_msg);
	if (proc_rc != NCSCC_RC_SUCCESS) {
		LOG_ER("failed to replicate start_sync_ok message to stdby err:%u", proc_rc);
		TRACE_LEAVE();
		return;
	}

	proc_rc = immd_mds_bcast_send(cb, &sync_evt, NCSMDS_SVC_ID_IMMND);

	if (proc_rc != NCSCC_RC_SUCCESS) {
		TRACE_5("failed to send message to IMMNDs");
	}

	immd_cb_dump();
	TRACE_LEAVE();
}

static void immd_abort_sync_ok(IMMD_CB *cb, IMMD_IMMND_INFO_NODE *node_info)
{
	uns32 proc_rc = NCSCC_RC_SUCCESS;
	IMMSV_EVT sync_evt;
	IMMD_MBCSV_MSG mbcp_msg;
	TRACE_ENTER();

	memset(&sync_evt, 0, sizeof(IMMSV_EVT));
	memset(&mbcp_msg, 0, sizeof(IMMD_MBCSV_MSG));

	node_info->syncStarted = FALSE;

	sync_evt.type = IMMSV_EVT_TYPE_IMMND;
	sync_evt.info.immnd.type = IMMND_EVT_D2ND_SYNC_ABORT;
	sync_evt.info.immnd.info.ctrl.nodeId = node_info->immnd_key;
	sync_evt.info.immnd.info.ctrl.rulingEpoch = cb->mRulingEpoch;
	sync_evt.info.immnd.info.ctrl.fevsMsgStart = cb->fevsSendCount;
	sync_evt.info.immnd.info.ctrl.ndExecPid = node_info->immnd_execPid;
	sync_evt.info.immnd.info.ctrl.canBeCoord = node_info->isOnController;
	sync_evt.info.immnd.info.ctrl.isCoord = node_info->isCoord;
	sync_evt.info.immnd.info.ctrl.syncStarted = node_info->syncStarted;
	sync_evt.info.immnd.info.ctrl.nodeEpoch = node_info->epoch;
	sync_evt.info.immnd.info.ctrl.pbeEnabled = 
		(cb->mRim == SA_IMM_KEEP_REPOSITORY);

	mbcp_msg.type = IMMD_A2S_MSG_SYNC_ABORT;
	mbcp_msg.info.ctrl = sync_evt.info.immnd.info.ctrl;

	proc_rc = immd_mbcsv_sync_update(cb, &mbcp_msg);
	if (proc_rc != NCSCC_RC_SUCCESS) {
		LOG_ER("failed to replicate start_sync_ok message to stdby err:%u", proc_rc);
		TRACE_LEAVE();
		return;
	}

	proc_rc = immd_mds_bcast_send(cb, &sync_evt, NCSMDS_SVC_ID_IMMND);

	if (proc_rc != NCSCC_RC_SUCCESS) {
		TRACE_5("failed to send message to IMMNDs");
	}

	immd_cb_dump();
	TRACE_LEAVE();
}

static void immd_prto_purge_mutations(IMMD_CB *cb, IMMD_IMMND_INFO_NODE *node_info)
{
	uns32 proc_rc = NCSCC_RC_SUCCESS;
	IMMSV_EVT sync_evt;
	TRACE_ENTER();

	memset(&sync_evt, 0, sizeof(IMMSV_EVT));

	sync_evt.type = IMMSV_EVT_TYPE_IMMND;
	sync_evt.info.immnd.type = IMMND_EVT_D2ND_PBE_PRTO_PURGE_MUTATIONS;
	sync_evt.info.immnd.info.ctrl.nodeId = node_info->immnd_key;
	sync_evt.info.immnd.info.ctrl.rulingEpoch = cb->mRulingEpoch;
	sync_evt.info.immnd.info.ctrl.fevsMsgStart = cb->fevsSendCount;
	sync_evt.info.immnd.info.ctrl.ndExecPid = node_info->immnd_execPid;
	sync_evt.info.immnd.info.ctrl.canBeCoord = node_info->isOnController;
	sync_evt.info.immnd.info.ctrl.isCoord = node_info->isCoord;
	sync_evt.info.immnd.info.ctrl.syncStarted = node_info->syncStarted;
	sync_evt.info.immnd.info.ctrl.nodeEpoch = node_info->epoch;
	sync_evt.info.immnd.info.ctrl.pbeEnabled = 
		(cb->mRim == SA_IMM_KEEP_REPOSITORY);

	/* Nothing to checkpoint to sby for this message. */
	proc_rc = immd_mds_bcast_send(cb, &sync_evt, NCSMDS_SVC_ID_IMMND);

	if (proc_rc != NCSCC_RC_SUCCESS) {
		TRACE_5("failed to send message to IMMNDs");
	}

	TRACE_LEAVE();
}

static int immd_dump_ok(IMMD_CB *cb, SaUint32T rulingEpoch, IMMD_IMMND_INFO_NODE *node_info)
{
	uns32 proc_rc = NCSCC_RC_SUCCESS;
	IMMSV_EVT dump_evt;
	IMMD_MBCSV_MSG mbcp_msg;
	TRACE_ENTER();

	memset(&dump_evt, 0, sizeof(IMMSV_EVT));
	memset(&mbcp_msg, 0, sizeof(IMMD_MBCSV_MSG));

	dump_evt.type = IMMSV_EVT_TYPE_IMMND;
	dump_evt.info.immnd.type = IMMND_EVT_D2ND_DUMP_OK;
	dump_evt.info.immnd.info.ctrl.rulingEpoch = cb->mRulingEpoch;
	dump_evt.info.immnd.info.ctrl.nodeId = node_info->immnd_key;
	dump_evt.info.immnd.info.ctrl.canBeCoord = node_info->isOnController;
	dump_evt.info.immnd.info.ctrl.ndExecPid = node_info->immnd_execPid;
	dump_evt.info.immnd.info.ctrl.isCoord = node_info->isCoord;
	dump_evt.info.immnd.info.ctrl.fevsMsgStart = cb->fevsSendCount;
	dump_evt.info.immnd.info.ctrl.syncStarted = node_info->syncStarted;
	dump_evt.info.immnd.info.ctrl.nodeEpoch = node_info->epoch;
	dump_evt.info.immnd.info.ctrl.pbeEnabled = 
		(cb->mRim == SA_IMM_KEEP_REPOSITORY);

	mbcp_msg.type = IMMD_A2S_MSG_DUMP_OK;
	mbcp_msg.info.ctrl = dump_evt.info.immnd.info.ctrl;

	proc_rc = immd_mbcsv_sync_update(cb, &mbcp_msg);
	if (proc_rc != NCSCC_RC_SUCCESS) {
		LOG_ER("failed to replicate dump_ok message to stdby err:%u", proc_rc);
		TRACE_LEAVE();
		return 0;
	}

	proc_rc = immd_mds_bcast_send(cb, &dump_evt, NCSMDS_SVC_ID_IMMND);
	/* Possibly have a tr-again loop here ??? */

	if (proc_rc != NCSCC_RC_SUCCESS) {
		LOG_ER("Failed to broadcast message to IMMNDs - exiting");
		exit(1);
	}
	TRACE_LEAVE();
	return 1;
}

static void immd_announce_load_ok(IMMD_CB *cb, SaUint32T rulingEpoch)
{
	uns32 proc_rc = NCSCC_RC_SUCCESS;
	IMMSV_EVT load_evt;
	TRACE_ENTER();

	memset(&load_evt, 0, sizeof(IMMSV_EVT));

	/* No state change in IMMD => No mbcp message to sby.
	   Currently cant handle failover during loading anyway.
	   Instead that would cause a restart of cluster.
	 */

	load_evt.type = IMMSV_EVT_TYPE_IMMND;
	load_evt.info.immnd.type = IMMND_EVT_D2ND_LOADING_OK;
	load_evt.info.immnd.info.ctrl.rulingEpoch = cb->mRulingEpoch;
	load_evt.info.immnd.info.ctrl.fevsMsgStart = cb->fevsSendCount;

	/*Use fevs instead !! */
	proc_rc = immd_mds_bcast_send(cb, &load_evt, NCSMDS_SVC_ID_IMMND);

	if (proc_rc != NCSCC_RC_SUCCESS) {
		LOG_ER("Failed to send message to IMMNDs");
	}
	TRACE_LEAVE();
}

static void immd_req_sync(IMMD_CB *cb, IMMD_IMMND_INFO_NODE *node_info)
{
	uns32 proc_rc = NCSCC_RC_SUCCESS;
	IMMSV_EVT rqsync_evt;
	IMMD_MBCSV_MSG mbcp_msg;

	TRACE_ENTER();
	memset(&rqsync_evt, 0, sizeof(IMMSV_EVT));
	memset(&mbcp_msg, 0, sizeof(IMMD_MBCSV_MSG));

	/*TODO: Send mbcp message to sby <dest, pid, epoch>? */

	rqsync_evt.type = IMMSV_EVT_TYPE_IMMND;
	rqsync_evt.info.immnd.type = IMMND_EVT_D2ND_SYNC_REQ;
	rqsync_evt.info.immnd.info.ctrl.nodeId = node_info->immnd_key;
	rqsync_evt.info.immnd.info.ctrl.rulingEpoch = cb->mRulingEpoch;
	rqsync_evt.info.immnd.info.ctrl.canBeCoord = node_info->isOnController;
	rqsync_evt.info.immnd.info.ctrl.fevsMsgStart = cb->fevsSendCount;
	rqsync_evt.info.immnd.info.ctrl.ndExecPid = node_info->immnd_execPid;
	rqsync_evt.info.immnd.info.ctrl.isCoord = node_info->isCoord;
	rqsync_evt.info.immnd.info.ctrl.syncStarted = node_info->syncStarted;
	rqsync_evt.info.immnd.info.ctrl.nodeEpoch = node_info->epoch;
	rqsync_evt.info.immnd.info.ctrl.pbeEnabled = 
		(cb->mRim == SA_IMM_KEEP_REPOSITORY);

	if (!(cb->immnd_coord)) {
		LOG_WA("No IMMND coord exists - ignore sync");
		goto done;
	}

	TRACE_5("Send-1 SYNC_REQ back to requesting IMMND  to dest:%llu", node_info->immnd_dest);
	TRACE_5("nodeid:%x epoch:%u isOnController:%u",
		node_info->immnd_key, cb->mRulingEpoch, node_info->isOnController);
	proc_rc = immd_mds_msg_send(cb, NCSMDS_SVC_ID_IMMND, node_info->immnd_dest, &rqsync_evt);
	if (proc_rc != NCSCC_RC_SUCCESS) {
		LOG_ER("Failed to send rqsync message err:%u back to requesting IMMND %x",
		       proc_rc, node_info->immnd_key);
		goto done;
	}

	if (cb->immnd_coord == cb->node_id) {	/*Coord immnd is local, i.e. at active SC. */
		if (!(cb->is_loc_immnd_up)) {
			LOG_ER("No coordinator IMMND known - ignoring sync request");
			goto done;
		}

		/*Send rqsync message to immnd at active SC. */
		TRACE_5("Send-2 SYNC_REQ to local coord IMMND at active SC, dest: %llu", cb->loc_immnd_dest);
		TRACE_5("nodeid:%x epoch:%u isOnController:%u",
			node_info->immnd_key, cb->mRulingEpoch, node_info->isOnController);

		proc_rc = immd_mds_msg_send(cb, NCSMDS_SVC_ID_IMMND, cb->loc_immnd_dest, &rqsync_evt);

		if (proc_rc != NCSCC_RC_SUCCESS) {
			LOG_ER("Failed to send rqsync message err:%u to coord IMMND (%x)",
			       proc_rc, node_info->immnd_key);
			goto done;
		}
	} else {		/*Coord immnd is at remote, i.e. at standby SC. */

		if (!(cb->is_rem_immnd_up)) {
			LOG_ER("No coordinator IMMND known - ignoring sync request");
			goto done;
		}

		TRACE_5("Send-3 SYNC_REQ to remote coord IMMND at standby SC, dest:%llu", cb->rem_immnd_dest);
		proc_rc = immd_mds_msg_send(cb, NCSMDS_SVC_ID_IMMND, cb->rem_immnd_dest, &rqsync_evt);
		if (proc_rc != NCSCC_RC_SUCCESS) {
			LOG_ER("Failed to send rqsync message err:%u to coord IMMND "
			       "at standby dest:%llu", proc_rc, cb->rem_immnd_dest);
			goto done;
		}
	}

 done:
	TRACE_LEAVE();
}

static void immd_accept_node(IMMD_CB *cb, IMMD_IMMND_INFO_NODE *node_info, NCS_BOOL doReply)
{
	uns32 proc_rc = NCSCC_RC_SUCCESS;
	IMMSV_EVT accept_evt;
	IMMD_MBCSV_MSG mbcp_msg;
	NCS_BOOL isOnController = node_info->isOnController;
	TRACE_ENTER();

	memset(&accept_evt, 0, sizeof(IMMSV_EVT));
	memset(&mbcp_msg, 0, sizeof(IMMD_MBCSV_MSG));

	accept_evt.type = IMMSV_EVT_TYPE_IMMND;
	accept_evt.info.immnd.type = IMMND_EVT_D2ND_INTRO_RSP;
	accept_evt.info.immnd.info.ctrl.nodeId = node_info->immnd_key;
	accept_evt.info.immnd.info.ctrl.rulingEpoch = cb->mRulingEpoch;
	accept_evt.info.immnd.info.ctrl.canBeCoord = isOnController;
	accept_evt.info.immnd.info.ctrl.ndExecPid = node_info->immnd_execPid;
	accept_evt.info.immnd.info.ctrl.fevsMsgStart = cb->fevsSendCount;
	accept_evt.info.immnd.info.ctrl.nodeEpoch = node_info->epoch;
	accept_evt.info.immnd.info.ctrl.pbeEnabled = 
		(cb->mRim == SA_IMM_KEEP_REPOSITORY);

	if (isOnController && (cb->immnd_coord == 0)) {
		LOG_IN("First IMMND on controller found at %x this IMMD at %x."
		       "\n\nCluster must be loading => designating this IMMND as coordinator",
		       node_info->immnd_key, cb->node_id);
		cb->immnd_coord = node_info->immnd_key;
		node_info->isCoord = TRUE;
	}

	if (node_info->isCoord) {
		accept_evt.info.immnd.info.ctrl.isCoord = TRUE;
		accept_evt.info.immnd.info.ctrl.syncStarted = node_info->syncStarted;
	}

	mbcp_msg.type = IMMD_A2S_MSG_INTRO_RSP;
	mbcp_msg.info.ctrl = accept_evt.info.immnd.info.ctrl;

	/*Checkpoint the message to standby director. 
	   Syncronous call=>wait for ack */
	proc_rc = immd_mbcsv_sync_update(cb, &mbcp_msg);

	if (proc_rc != NCSCC_RC_SUCCESS) {
		LOG_ER("failed to replicate node accept message to stdby err:%u", proc_rc);
		goto done;
	}

	if (doReply) {		/*If doReply is false then this was only an epoch refresh
				   from an IMMND. */
		/* Send reply on intro (accept) message back to sending IMMND */
		proc_rc = immd_mds_msg_send(cb, NCSMDS_SVC_ID_IMMND, node_info->immnd_dest, &accept_evt);
		if (proc_rc != NCSCC_RC_SUCCESS) {
			LOG_ER("Failed to send accept message to IMMND %x", node_info->immnd_key);
			goto done;
		}

		if (isOnController) {
			/* If introduced IMMND is on a controller, then send the
			   accept message also to the IMMND on the other controller
			   (either may be coord). */

			if ((node_info->immnd_key != cb->node_id) && cb->is_loc_immnd_up) {
				/*Send IMMND on stby acceptance message also to 
				   immnd at active. */
				proc_rc = immd_mds_msg_send(cb, NCSMDS_SVC_ID_IMMND, cb->loc_immnd_dest, &accept_evt);
				if (proc_rc != NCSCC_RC_SUCCESS) {
					LOG_WA("Failed to send immnd-sby accept message to IMMND at "
					       "active %x error:%u", node_info->immnd_key, proc_rc);
					goto done;
				}
			} else if (cb->is_rem_immnd_up) {
				/*Send IMMND on active acceptance message also to 
				   immnd at standby. */
				proc_rc = immd_mds_msg_send(cb, NCSMDS_SVC_ID_IMMND, cb->rem_immnd_dest, &accept_evt);
				if (proc_rc != NCSCC_RC_SUCCESS) {
					LOG_WA("Failed to send immnd-act accept message to IMMND at "
					       "sby %llu error:%u", cb->rem_immnd_dest, proc_rc);
					goto done;
				}
			}
		}
	} else {		/* Not doReply => epoch refresh => probably a sync => reset sync request. */

		/*Reset any syncRequester to normal. */
		node_info->syncRequested = 0;
	}

 done:
	immd_cb_dump();
	TRACE_LEAVE();
}

/****************************************************************************
 * Name          : immd_evt_proc_immnd_announce_dump
 *
 * Description   : Function to process the IMMD_EVT_ND2D_ANNOUNCE_DUMP msg
 *
 * Arguments     : IMMD_CB *cb - IMMD CB pointer
 *                 IMMSV_EVT *evt - Received Event structure
 *
 * Return Values : NCSCC_RC_SUCCESS/Error.
 *
 *****************************************************************************/
static uns32 immd_evt_proc_immnd_announce_dump(IMMD_CB *cb, IMMD_EVT *evt, IMMSV_SEND_INFO *sinfo)
{
	uns32 proc_rc = NCSCC_RC_SUCCESS;
	IMMD_IMMND_INFO_NODE *node_info = NULL;
	TRACE_ENTER();

	immd_immnd_info_node_get(&cb->immnd_tree, &sinfo->dest, &node_info);
	if (node_info) {
		if (node_info->immnd_execPid != evt->info.ctrl_msg.ndExecPid) {
			LOG_ER("Wrong PID %u != %u", node_info->immnd_execPid, evt->info.ctrl_msg.ndExecPid);
			proc_rc = NCSCC_RC_FAILURE;
		}

		if (node_info->immnd_key != cb->immnd_coord) {
			LOG_IN("Dump at non coord controller %x != %x", node_info->immnd_key, cb->immnd_coord);
		} else {
			/* From coord. */
			SaImmRepositoryInitModeT oldRim = cb->mRim;
			cb->mRim = (evt->info.ctrl_msg.pbeEnabled)?SA_IMM_KEEP_REPOSITORY:SA_IMM_INIT_FROM_FILE;
			if(oldRim != cb->mRim) {
				LOG_NO("SaImmRepositoryInitModeT noted as being: %s", 
					(cb->mRim == SA_IMM_KEEP_REPOSITORY)?"SA_IMM_KEEP_REPOSITORY":"SA_IMM_INIT_FROM_FILE");
			}
		}


		if (node_info->epoch != cb->mRulingEpoch) {
			LOG_ER("Wrong Epoch %u != %u", node_info->epoch, cb->mRulingEpoch);
			proc_rc = NCSCC_RC_FAILURE;
		}

		if (evt->info.ctrl_msg.epoch != cb->mRulingEpoch) {
			LOG_ER("Wrong Epoch %u!=%u in request dump message ",
			       evt->info.ctrl_msg.epoch, cb->mRulingEpoch);
			proc_rc = NCSCC_RC_FAILURE;
		}

		if (proc_rc == NCSCC_RC_SUCCESS) {
			/*Verify epochs for all nodes! 
			   if not correct return error to coord, remove coord.
			   See ImmEvs::syncNeeded() 
			   Loop through all nodes */

			cb->mRulingEpoch++;
			/*Only updates epoch for dumping controller */
			node_info->epoch = cb->mRulingEpoch;

			if (immd_dump_ok(cb, cb->mRulingEpoch, node_info)) {
				LOG_IN("Successfully announced dump at node %x. New Epoch:%u",
				       node_info->immnd_key, node_info->epoch);
			} else {
				/*Back out from the epoch change. */
				cb->mRulingEpoch--;
				node_info->epoch = cb->mRulingEpoch;
			}
		}
	} else {
		LOG_ER("Node not found %llu", sinfo->dest);
		proc_rc = NCSCC_RC_FAILURE;
	}
	TRACE_LEAVE();
	immd_cb_dump();
	return proc_rc;
}

/****************************************************************************
 * Name          : immd_evt_proc_immnd_announce_sync
 *
 * Description   : Function to process the IMMD_EVT_ND2D_SYNC_START event 
 *
 * Arguments     : IMMD_CB *cb - IMMD CB pointer
 *                 IMMSV_EVT *evt - Received Event structure
 *
 * Return Values : NCSCC_RC_SUCCESS/Error.
 *
 ****************************************************************************/
static uns32 immd_evt_proc_immnd_announce_sync(IMMD_CB *cb, IMMD_EVT *evt, IMMSV_SEND_INFO *sinfo)
{
	uns32 proc_rc = NCSCC_RC_SUCCESS;
	IMMD_IMMND_INFO_NODE *node_info = NULL;
	TRACE_ENTER();
	immd_immnd_info_node_get(&cb->immnd_tree, &sinfo->dest, &node_info);
	if (node_info) {
		if (node_info->immnd_execPid != evt->info.ctrl_msg.ndExecPid) {
			/* TODO: Return error to coord, remove coord designation ?? */
			LOG_ER("Wrong PID %u != %u", node_info->immnd_execPid, evt->info.ctrl_msg.ndExecPid);
			proc_rc = NCSCC_RC_FAILURE;
		}

		if (node_info->immnd_key != cb->immnd_coord) {
			LOG_ER("Not Coord! %x != %x", node_info->immnd_key, cb->immnd_coord);
			/* TODO: Return error to coord, remove coord designation ?? */
			proc_rc = NCSCC_RC_FAILURE;
		}

		if (node_info->epoch != cb->mRulingEpoch) {
			LOG_ER("Wrong Epoch %u != %u", node_info->epoch, cb->mRulingEpoch);
			/* TODO: Return error to coord, remove coord designation ?? */
			proc_rc = NCSCC_RC_FAILURE;
		}

		if (evt->info.ctrl_msg.epoch != cb->mRulingEpoch + 1) {
			LOG_ER("Wrong new Epoch %u!=%u +1", evt->info.ctrl_msg.epoch, cb->mRulingEpoch);
			/* TODO: Return error to coord, remove coord designation ?? */
			proc_rc = NCSCC_RC_FAILURE;
		}

		if (proc_rc == NCSCC_RC_SUCCESS) {
			/*Verify epochs for all nodes! 
			   if not correct return error to coord, remove coord.
			   See ImmEvs::syncNeeded() 
			   Loop through all nodes */

			cb->mRulingEpoch++;

			/*Only updates epoch for coord. */
			/*node_info->epoch = cb->mRulingEpoch; */
			immd_start_sync_ok(cb, cb->mRulingEpoch, node_info);
			LOG_IN("Successfully announced sync. New ruling epoch:%u", cb->mRulingEpoch);
		}
	} else {
		LOG_ER("Node not found %llu", sinfo->dest);
		proc_rc = NCSCC_RC_FAILURE;
	}
	TRACE_LEAVE();
	return proc_rc;
}

/****************************************************************************
 * Name          : immd_evt_proc_immnd_abort_sync
 *
 * Description   : Function to process the IMMD_EVT_ND2D_SYNC_ABORT event 
 *
 * Arguments     : IMMD_CB *cb - IMMD CB pointer
 *                 IMMSV_EVT *evt - Received Event structure
 *
 * Return Values : NCSCC_RC_SUCCESS/Error.
 *
 ****************************************************************************/
uns32 immd_evt_proc_immnd_abort_sync(IMMD_CB *cb, IMMD_EVT *evt, IMMSV_SEND_INFO *sinfo)
{
	uns32 proc_rc = NCSCC_RC_SUCCESS;
	IMMD_IMMND_INFO_NODE *node_info = NULL;
	TRACE_ENTER();
	immd_immnd_info_node_get(&cb->immnd_tree, &sinfo->dest, &node_info);
	if (node_info) {
		if (node_info->immnd_execPid != evt->info.ctrl_msg.ndExecPid) {
			LOG_ER("Abort sync: wrong PID %u != %u",
			       node_info->immnd_execPid, evt->info.ctrl_msg.ndExecPid);
			proc_rc = NCSCC_RC_FAILURE;
		}

		if (node_info->immnd_key != cb->immnd_coord) {
			LOG_ER("Abort sync: not Coord! %x != %x", node_info->immnd_key, cb->immnd_coord);
			proc_rc = NCSCC_RC_FAILURE;
		}

		if (node_info->epoch != cb->mRulingEpoch) {
			LOG_ER("Abort sync: wrong Epoch %u != %u", node_info->epoch, cb->mRulingEpoch);
			proc_rc = NCSCC_RC_FAILURE;
		}

		if (proc_rc == NCSCC_RC_SUCCESS) {
			/*Updates epoch for coord. */
			immd_abort_sync_ok(cb, node_info);

			LOG_IN("Successfully aborted sync. Epoch:%u", cb->mRulingEpoch);
		}
	} else {
		LOG_ER("Node not found %llu", sinfo->dest);
		proc_rc = NCSCC_RC_FAILURE;
	}
	TRACE_LEAVE();
	return proc_rc;
}

/****************************************************************************
 * Name          : immd_evt_proc_immnd_loading_failed
 *
 * Description   : Function to process the IMMD_EVT_ND2D_LOADING_FAILED event 
 *
 * Arguments     : IMMD_CB *cb - IMMD CB pointer
 *                 IMMSV_EVT *evt - Received Event structure
 *
 * Return Values : NCSCC_RC_SUCCESS/Error.
 *
 ****************************************************************************/
uns32 immd_evt_proc_immnd_loading_failed(IMMD_CB *cb, IMMD_EVT *evt, IMMSV_SEND_INFO *sinfo)
{
	uns32 proc_rc = NCSCC_RC_SUCCESS;
	IMMD_IMMND_INFO_NODE *node_info = NULL;
	TRACE_ENTER();
	immd_immnd_info_node_get(&cb->immnd_tree, &sinfo->dest, &node_info);
	if (node_info) {
		if (node_info->immnd_execPid != evt->info.ctrl_msg.ndExecPid) {
			LOG_ER("Loading Failed: wrong PID %u != %u",
			       node_info->immnd_execPid, evt->info.ctrl_msg.ndExecPid);
			proc_rc = NCSCC_RC_FAILURE;
		}

		if (node_info->immnd_key != cb->immnd_coord) {
			LOG_ER("Loading failed: not Coord! %x != %x", node_info->immnd_key, cb->immnd_coord);
			proc_rc = NCSCC_RC_FAILURE;
		}

		LOG_ER("******** LOADING FAILED. File(s) possibly missing, inaccessible or corrupt .. ? *********");
	} else {
		LOG_ER("Node not found %llu", sinfo->dest);
		proc_rc = NCSCC_RC_FAILURE;
	}
	TRACE_LEAVE();
	return proc_rc;
}

/****************************************************************************
 * Name          : immd_evt_proc_immnd_prto_purge_mutations
 *
 * Description   : Function to process the IMMD_EVT_ND2D_PRTO_PURGE_MUTATIONS event
 *                 Persistent runtime objects (PRTOs) are persistified via the PBE.
 *                 Each mutation (create/delete/update) leaves a record in ImmModel
 *                 which is removed when we receive an ack from PBE that the mutaton
 *                 has been persistified. If the PBE crashes or in general needs 
 *                 restarting, then old mutation records are discarded. This also
 *                 implies that the PBE has to be restarted without the --restore flag,
 *                 forcing a complete regeneration of the DB file based on current 
 *                 ImmModel contents. In essense we drop the unack'ed mutations, violating
 *                 the persistency requirement. But the regeneration of the DB file
 *                 compensates for this. 
 * Arguments     : IMMD_CB *cb - IMMD CB pointer
 *                 IMMSV_EVT *evt - Received Event structure
 *
 * Return Values : NCSCC_RC_SUCCESS/Error.
 *
 ****************************************************************************/
uns32 immd_evt_proc_immnd_prto_purge_mutations(IMMD_CB *cb, IMMD_EVT *evt, IMMSV_SEND_INFO *sinfo)
{
	uns32 proc_rc = NCSCC_RC_SUCCESS;
	IMMD_IMMND_INFO_NODE *node_info = NULL;
	TRACE_ENTER();
	immd_immnd_info_node_get(&cb->immnd_tree, &sinfo->dest, &node_info);
	if (node_info) {
		if (node_info->immnd_execPid != evt->info.ctrl_msg.ndExecPid) {
			LOG_ER("Prto purge mutations: wrong PID %u != %u",
			       node_info->immnd_execPid, evt->info.ctrl_msg.ndExecPid);
			proc_rc = NCSCC_RC_FAILURE;
		}

		if (node_info->immnd_key != cb->immnd_coord) {
			LOG_ER("Prto purge mutations: not Coord! %x != %x", node_info->immnd_key, cb->immnd_coord);
			proc_rc = NCSCC_RC_FAILURE;
		}

		if (node_info->epoch > cb->mRulingEpoch) {
			LOG_ER("Prto purge mutations: wrong Epoch %u != %u", node_info->epoch, cb->mRulingEpoch);
			proc_rc = NCSCC_RC_FAILURE;
		}

		if (proc_rc == NCSCC_RC_SUCCESS) {
			immd_prto_purge_mutations(cb, node_info);

			LOG_IN("Purge prto mutations broadcast. Epoch:%u", cb->mRulingEpoch);
		}
	} else {
		LOG_ER("Node not found %llu", sinfo->dest);
		proc_rc = NCSCC_RC_FAILURE;
	}
	TRACE_LEAVE();
	return proc_rc;

}

/****************************************************************************
 * Name          : immd_evt_proc_immnd_announce_load
 *
 * Description   : Function to process the IMMD_EVT_ND2D_ANNOUNCE_LOAD event 
 *
 * Arguments     : IMMD_CB *cb - IMMD CB pointer
 *                 IMMSV_EVT *evt - Received Event structure
 *
 * Return Values : NCSCC_RC_SUCCESS/Error.
 *
 *****************************************************************************/
static uns32 immd_evt_proc_immnd_announce_load(IMMD_CB *cb, IMMD_EVT *evt, IMMSV_SEND_INFO *sinfo)
{
	uns32 proc_rc = NCSCC_RC_SUCCESS;
	IMMD_IMMND_INFO_NODE *node_info = NULL;
	TRACE_ENTER();

	immd_immnd_info_node_get(&cb->immnd_tree, &sinfo->dest, &node_info);
	if (node_info) {
		if (node_info->immnd_execPid != evt->info.ctrl_msg.ndExecPid) {
			/*TODO Return error to coord, remove coord designation ?? */
			LOG_ER("Wrong PID %u != %u", node_info->immnd_execPid, evt->info.ctrl_msg.ndExecPid);
			proc_rc = NCSCC_RC_FAILURE;
			goto fail;
		}

		if (node_info->immnd_key != cb->immnd_coord) {
			LOG_ER("Not Coord! %x != %x", node_info->immnd_key, cb->immnd_coord);
			proc_rc = NCSCC_RC_FAILURE;
			goto fail;
			/*TODO Return error to coord, remove coord designation ?? */
		}

		if (node_info->epoch != cb->mRulingEpoch) {
			LOG_ER("Wrong Epoch %u != %u", node_info->epoch, cb->mRulingEpoch);
			proc_rc = NCSCC_RC_FAILURE;
			goto fail;
			/*TODO Return error to coord, remove coord designation ?? */
		}

		if (evt->info.ctrl_msg.epoch != cb->mRulingEpoch + 1) {
			LOG_ER("Wrong new Epoch %u!=%u +1", evt->info.ctrl_msg.epoch, cb->mRulingEpoch);
			proc_rc = NCSCC_RC_FAILURE;
			goto fail;
			/*TODO Return error to coord, remove coord designation ?? */
		}

		/*Verify epochs for all nodes ? */

		cb->mRulingEpoch++;
		node_info->epoch = cb->mRulingEpoch;
		immd_announce_load_ok(cb, cb->mRulingEpoch);
		LOG_IN("Successfully announced loading. New ruling epoch:%u", cb->mRulingEpoch);
	} else {
		LOG_ER("Node not found %llu", sinfo->dest);
		proc_rc = NCSCC_RC_FAILURE;
	}
 fail:
	TRACE_LEAVE();
	return proc_rc;
}

/****************************************************************************
 * Name          : immd_evt_proc_immnd_req_sync
 *
 * Description   : Function to process the IMMD_EVT_ND2D_REQ_SYNC message.
 *
 * Arguments     : IMMD_CB *cb - IMMD CB pointer
 *                 IMMSV_EVT *evt - Received Event structure
 *
 * Return Values : NCSCC_RC_SUCCESS/Error.
 *
 *****************************************************************************/
static uns32 immd_evt_proc_immnd_req_sync(IMMD_CB *cb, IMMD_EVT *evt, IMMSV_SEND_INFO *sinfo)
{
	uns32 proc_rc = NCSCC_RC_SUCCESS;
	IMMD_IMMND_INFO_NODE *node_info = NULL;
	int oldPid, newPid;
	int oldEpoch, newEpoch;
	TRACE_ENTER();

	immd_immnd_info_node_get(&cb->immnd_tree, &sinfo->dest, &node_info);
	if (node_info) {
		oldPid = node_info->immnd_execPid;
		oldEpoch = node_info->epoch;
		newPid = evt->info.ctrl_msg.ndExecPid;
		newEpoch = evt->info.ctrl_msg.epoch;

		if (oldPid != newPid || oldEpoch != newEpoch) {
			if (oldEpoch > newEpoch) {
				LOG_WA("IMMND process at node %x "
				       "with old epoch: %u > new epoch:%u", node_info->immnd_key, oldEpoch, newEpoch);
			}

			node_info->immnd_execPid = newPid;
			node_info->epoch = newEpoch;

			/*is this check necessary ?? */
			if (oldPid && (oldPid != newPid)) {
				LOG_IN("Detected new IMMND process at node %x "
				       "old epoch: %u  new epoch:%u", node_info->immnd_key, oldEpoch, newEpoch);
			}
		}

		if (node_info->isCoord) {
			LOG_ER("SERIOUS INCONSISTENCY, current IMMND coord " "requests sync!");
			immd_proc_immd_reset(cb, TRUE);
			proc_rc = NCSCC_RC_FAILURE;
		} else {
			node_info->syncRequested = 1;
			if (node_info->isOnController) {
				LOG_WA("IMMND on controller (not currently coord) requests sync");
			}
			immd_req_sync(cb, node_info);
		}

		LOG_IN("Node %x request sync sync-pid:%d epoch:%u ",
		       node_info->immnd_key, node_info->immnd_execPid, node_info->epoch);
	} else {
		LOG_ER("Node not found %llu", sinfo->dest);
		proc_rc = NCSCC_RC_FAILURE;
	}

	TRACE_LEAVE();
	return proc_rc;
}

/****************************************************************************
 * Name          : immd_evt_proc_immnd_intro
 *
 * Description   : Function to process the IMMD_EVT_ND2D_INTRO event 
 *
 * Arguments     : IMMD_CB *cb - IMMD CB pointer
 *                 IMMSV_EVT *evt - Received Event structure
 *
 * Return Values : NCSCC_RC_SUCCESS/Error.
 *
 *****************************************************************************/

static uns32 immd_evt_proc_immnd_intro(IMMD_CB *cb, IMMD_EVT *evt, IMMSV_SEND_INFO *sinfo)
{
	uns32 proc_rc = NCSCC_RC_SUCCESS;
	IMMD_IMMND_INFO_NODE *node_info = NULL;
	int oldPid, newPid;
	int oldEpoch, newEpoch;

	TRACE_ENTER();

	immd_immnd_info_node_get(&cb->immnd_tree, &sinfo->dest, &node_info);
	if (node_info) {
		oldPid = node_info->immnd_execPid;
		oldEpoch = node_info->epoch;
		newPid = evt->info.ctrl_msg.ndExecPid;
		newEpoch = evt->info.ctrl_msg.epoch;

		if (oldPid != newPid || oldEpoch != newEpoch) {
			if (oldEpoch != newEpoch) {
				LOG_IN("New Epoch for IMMND process at node %x "
				       "old epoch: %u  new epoch:%u", node_info->immnd_key, oldEpoch, newEpoch);
			}

			node_info->immnd_execPid = newPid;
			node_info->epoch = newEpoch;

			if (node_info->syncStarted) {
				assert(oldPid == newPid);
				assert(node_info->isCoord);
				assert(node_info->isOnController);
				if(node_info->epoch != cb->mRulingEpoch) {
					LOG_ER("immd_evt_proc_immnd_intro: syncStarted true for node with "
						"strange epoch node_info->epoch(%u) != cb->mRulingEpoc(%u)",	
						node_info->epoch, cb->mRulingEpoch);
					assert(0);
				}
				/*node_info->epoch = cb->mRulingEpoch;*/
				node_info->syncStarted = FALSE;
			}

			/*is this check necessary ?? 
			   It could happen with EVS but not with FEVS. */
			if (oldPid && (oldPid != newPid)) {
				LOG_IN("Detected new IMMND process at node %x "
				       "old epoch: %u  new epoch:%u", node_info->immnd_key, oldEpoch, newEpoch);
			}
		}

		TRACE_5("Node %x FOUND pid:%d epoch:%u syncR:%u",
			node_info->immnd_key, node_info->immnd_execPid, node_info->epoch, node_info->syncRequested);

		if (evt->info.ctrl_msg.refresh) {
			TRACE_5("ONLY A REFRESH OF epoch for %x, newE:%u RulngE:%u",
				node_info->immnd_key, node_info->epoch, cb->mRulingEpoch);
			if (cb->mRulingEpoch < node_info->epoch) {
				cb->mRulingEpoch = node_info->epoch;
				LOG_IN("Ruling epoch changed to:%u", cb->mRulingEpoch);
			}
			immd_accept_node(cb, node_info, FALSE);
		} else {
			if (sinfo->dest == cb->loc_immnd_dest) {
				node_info->isOnController = TRUE;
				LOG_IN("New IMMND process is on ACTIVE Controller at %x", node_info->immnd_key);
			} else if (cb->immd_remote_id &&
				   m_IMMND_IS_ON_SCXB(cb->immd_remote_id,
						      immd_get_slot_and_subslot_id_from_mds_dest(sinfo->dest))) {
				node_info->isOnController = TRUE;

				if (cb->is_rem_immnd_up) {
					assert(cb->rem_immnd_dest == sinfo->dest);
				} else {
					cb->is_rem_immnd_up = TRUE;	/*ABT BUGFIX 080811 */
					cb->rem_immnd_dest = sinfo->dest;
				}
				LOG_IN("New IMMND process is on STANDBY Controller at %x", node_info->immnd_key);
			} else {	/*if(cb->immd_remote_id) */

				/*node_info->isOnPayload = TRUE; */
				LOG_IN("New IMMND process is on PAYLOAD at:%x", node_info->immnd_key);
			}
			immd_accept_node(cb, node_info, TRUE);
		}
	} else {
		LOG_ER("Node not found %llu", sinfo->dest);
		proc_rc = NCSCC_RC_FAILURE;
	}
	TRACE_LEAVE();
	return proc_rc;
}

/****************************************************************************
 * Name          : immd_evt_proc_adminit_req
 *
 * Description   : Function to process the IMMD_EVT_ND2D_ADMINIT_REQ event 
 *                 from IMMND.
 *
 * Arguments     : IMMD_CB *cb - IMMD CB pointer
 *                 IMMSV_EVT *evt - Received Event structure
 *
 * Return Values : NCSCC_RC_SUCCESS/Error.
 *
 * Notes         : None.
 *****************************************************************************/

static uns32 immd_evt_proc_adminit_req(IMMD_CB *cb, IMMD_EVT *evt, IMMSV_SEND_INFO *sinfo)
{
	IMMSV_EVT fevs_evt;
	IMMD_MBCSV_MSG mbcp_msg;
	SaAisErrorT rc = SA_AIS_OK;
	uns32 proc_rc = NCSCC_RC_SUCCESS;
	IMMSV_ND2D_ADMINIT_REQ *adminit_req = &evt->info.admown_init;
	SaUint32T globalId = 0;
	NCS_UBAID uba;
	char *tmpData = NULL;
	uba.start = NULL;
	TRACE_ENTER();

	TRACE_5("Admin owner name:%s", adminit_req->i.adminOwnerName.value);

	globalId = ++(cb->admo_id_count);
	if (cb->admo_id_count == 0xffffffff) {
		cb->admo_id_count = 0;
		LOG_WA("Active: admo_id_count wrap arround");
	}

	memset(&mbcp_msg, 0, sizeof(IMMD_MBCSV_MSG));
	mbcp_msg.type = IMMD_A2S_MSG_ADMINIT;
	mbcp_msg.info.count = globalId;

	/*Checkpoint message to standby D. Asyncronous call=>dont wait for ack.
	   We can do this because we generate a fevs below which generates 
	   a checkpoint message that is syncronous, which pushes over this message.
	   Even the order of the messages will eb preserved. */
	rc = immd_mbcsv_async_update(cb, &mbcp_msg);

	if (rc != NCSCC_RC_SUCCESS) {
		/*TODO handle this case better.
		   Reply with try again to ND.
		   We apparently failed to replicate the message to stby.
		   This could be because the standby is crashed ?
		   Is there any way to check this towards the cb. */
		LOG_ER("Failed to replicate message to stdby send_count:%u", globalId);
		goto fail;
	}
	TRACE_5("Global Admin owner ID:%u", globalId);

	/*Create and pack the core message for fevs. */

	memset(&fevs_evt, 0, sizeof(IMMSV_EVT));
	fevs_evt.type = IMMSV_EVT_TYPE_IMMND;
	fevs_evt.info.immnd.type = IMMND_EVT_D2ND_ADMINIT;
	fevs_evt.info.immnd.info.adminitGlobal.globalOwnerId = globalId;
	fevs_evt.info.immnd.info.adminitGlobal.i = adminit_req->i;

	proc_rc = ncs_enc_init_space(&uba);
	if (proc_rc != NCSCC_RC_SUCCESS) {
		LOG_ER("Failed init ubaid");
		uba.start = NULL;
		goto fail;
	}

	proc_rc = immsv_evt_enc(&fevs_evt, &uba);
	if (proc_rc != NCSCC_RC_SUCCESS) {
		LOG_ER("Failed encode fevs");
		uba.start = NULL;
		goto fail;
	}

	int32 size = uba.ttl;
	tmpData = malloc(size);
	assert(tmpData);
	char *data = m_MMGR_DATA_AT_START(uba.start, size, tmpData);

	memset(&fevs_evt, 0, sizeof(IMMSV_EVT));	/*No pointers=>no leak */
	fevs_evt.type = IMMSV_EVT_TYPE_IMMD;
	fevs_evt.info.immd.type = 0;
	fevs_evt.info.immd.info.fevsReq.client_hdl = adminit_req->client_hdl;
	fevs_evt.info.immd.info.fevsReq.reply_dest = 0LL;
	fevs_evt.info.immd.info.fevsReq.msg.size = size;
	fevs_evt.info.immd.info.fevsReq.msg.buf = data;

	/*This invocation makes it appear as if a fevs call arrived at the IMMD.
	   In actuality, this call needed extra processing (the global id) so
	   it was not sent IMMND->IMMD as a fevs call but as a dedicated call. */
	proc_rc = immd_evt_proc_fevs_req(cb, &(fevs_evt.info.immd), sinfo, FALSE);

 fail:
	if (tmpData) {
		free(tmpData);
	}

	if (uba.start) {
		m_MMGR_FREE_BUFR_LIST(uba.start);
	}

	TRACE_LEAVE();
	return proc_rc;
}

/****************************************************************************
 * Name          : immd_evt_proc_impl_set_req
 *
 * Description   : Function to process the IMMD_EVT_ND2D_IMPLSET_REQ event 
 *                 from IMMND.
 *
 * Arguments     : IMMD_CB *cb - IMMD CB pointer
 *                 IMMSV_EVT *evt - Received Event structure
 *
 * Return Values : NCSCC_RC_SUCCESS/Error.
 *****************************************************************************/

static uns32 immd_evt_proc_impl_set_req(IMMD_CB *cb, IMMD_EVT *evt, IMMSV_SEND_INFO *sinfo)
{
	IMMSV_EVT fevs_evt;
	IMMD_MBCSV_MSG mbcp_msg;
	SaAisErrorT rc = SA_AIS_OK;
	uns32 proc_rc = NCSCC_RC_SUCCESS;
	IMMSV_OI_IMPLSET_REQ *impl_req = &evt->info.impl_set.r;
	SaUint32T globalId = 0;
	NCS_UBAID uba;
	char *tmpData = NULL;
	uba.start = NULL;

	TRACE_5("Implementer name:%s", impl_req->impl_name.buf);

	globalId = ++(cb->impl_count);
	if (cb->impl_count == 0xffffffff) {
		cb->impl_count = 0;
		LOG_WA("Active: impl_count wrap arround");
	}

	memset(&mbcp_msg, 0, sizeof(IMMD_MBCSV_MSG));
	mbcp_msg.type = IMMD_A2S_MSG_IMPLSET;
	mbcp_msg.info.count = globalId;

	/*Checkpoint message to standby D. Asyncronous call=>dont wait for ack.
	   We can do this because we generate a fevs below which generates 
	   a checkpoint message that is syncronous, which pushes over this message.
	   Even the order of the messages will eb preserved. */
	rc = immd_mbcsv_async_update(cb, &mbcp_msg);

	if (rc != NCSCC_RC_SUCCESS) {
		/*TODO handle this case better.
		   Reply with try again to ND.
		   We apparently failed to replicate the message to stby.
		   This could be because the standby is crashed ?
		   Is there any way to check this in cb ? */
		LOG_ER("Failed to replicate message to stdby send_count:%u", globalId);
		goto fail;
	}

	TRACE_5("Global Implementer ID:%u", globalId);

	/*First create and pack the core message for fevs. */

	memset(&fevs_evt, 0, sizeof(IMMSV_EVT));
	fevs_evt.type = IMMSV_EVT_TYPE_IMMND;
	fevs_evt.info.immnd.type = IMMND_EVT_D2ND_IMPLSET_RSP;
	fevs_evt.info.immnd.info.implSet.impl_id = globalId;
	fevs_evt.info.immnd.info.implSet.impl_name.size = impl_req->impl_name.size;
	fevs_evt.info.immnd.info.implSet.impl_name.buf = impl_req->impl_name.buf;	/*Warning, borrowing pointer, dont deallocate */
	fevs_evt.info.immnd.info.implSet.client_hdl = impl_req->client_hdl;	/*redundant */

	proc_rc = ncs_enc_init_space(&uba);
	if (proc_rc != NCSCC_RC_SUCCESS) {
		LOG_ER("Failed init ubaid");
		uba.start = NULL;
		goto fail;
	}

	proc_rc = immsv_evt_enc(&fevs_evt, &uba);
	if (proc_rc != NCSCC_RC_SUCCESS) {
		LOG_ER("Failed encode fevs");
		uba.start = NULL;
		goto fail;
	}

	int32 size = uba.ttl;
	tmpData = malloc(size);
	assert(tmpData);
	char *data = m_MMGR_DATA_AT_START(uba.start, size, tmpData);

	memset(&fevs_evt, 0, sizeof(IMMSV_EVT));	/*No ponters=>no leak */
	fevs_evt.type = IMMSV_EVT_TYPE_IMMD;
	fevs_evt.info.immd.type = 0;
	fevs_evt.info.immd.info.fevsReq.client_hdl = impl_req->client_hdl;
	fevs_evt.info.immd.info.fevsReq.reply_dest = evt->info.impl_set.reply_dest;
	fevs_evt.info.immd.info.fevsReq.msg.size = size;
	fevs_evt.info.immd.info.fevsReq.msg.buf = data;

	/*This invocation makes it appear as if a fevs call arrived at the IMMD.
	   In actuality, this call needed extra processing (the global id) so
	   it was not sent IMMND->IMMD as a fevs call but as a dedicated call. */
	proc_rc = immd_evt_proc_fevs_req(cb, &(fevs_evt.info.immd), sinfo, FALSE);

 fail:
	if (tmpData) {
		free(tmpData);
	}

	if (uba.start) {
		m_MMGR_FREE_BUFR_LIST(uba.start);
	}

	free(impl_req->impl_name.buf);
	impl_req->impl_name.buf = NULL;
	impl_req->impl_name.size = 0;
	return proc_rc;
}

/****************************************************************************
 * Name          : immd_evt_proc_discard_impl
 *
 * Description   : Function to process the IMMD_EVT_ND2D_DISCARD_IMPL
 *                 event, from IMMND.
 *
 * Arguments     : IMMD_CB *cb - IMMD CB pointer
 *                 IMMSV_EVT *evt - Received Event structure
 *
 * Return Values : NCSCC_RC_SUCCESS/Error.
 *****************************************************************************/
static uns32 immd_evt_proc_discard_impl(IMMD_CB *cb, IMMD_EVT *evt, IMMSV_SEND_INFO *sinfo)
{
	IMMSV_EVT fevs_evt;
	uns32 proc_rc = NCSCC_RC_SUCCESS;
	IMMSV_OI_IMPLSET_REQ *impl_req = &evt->info.impl_set.r;
	NCS_UBAID uba;
	char *tmpData = NULL;
	uba.start = NULL;
	TRACE_ENTER();
	TRACE_5("Discard implementer ID:%u", impl_req->impl_id);

	/*Create and pack the core message for fevs. */

	memset(&fevs_evt, 0, sizeof(IMMSV_EVT));
	fevs_evt.type = IMMSV_EVT_TYPE_IMMND;
	fevs_evt.info.immnd.type = IMMND_EVT_D2ND_DISCARD_IMPL;
	fevs_evt.info.immnd.info.implSet.impl_id = impl_req->impl_id;

	proc_rc = ncs_enc_init_space(&uba);
	if (proc_rc != NCSCC_RC_SUCCESS) {
		LOG_ER("Failed init ubaid");
		uba.start = NULL;
		goto fail;
	}

	proc_rc = immsv_evt_enc(&fevs_evt, &uba);
	if (proc_rc != NCSCC_RC_SUCCESS) {
		LOG_ER("Failed encode fevs");
		uba.start = NULL;
		goto fail;
	}

	int32 size = uba.ttl;
	tmpData = malloc(size);
	assert(tmpData);
	char *data = m_MMGR_DATA_AT_START(uba.start, size, tmpData);

	memset(&fevs_evt, 0, sizeof(IMMSV_EVT));	/*No ponters=>no leak */
	fevs_evt.type = IMMSV_EVT_TYPE_IMMD;
	fevs_evt.info.immd.type = 0;
	fevs_evt.info.immd.info.fevsReq.msg.size = size;
	fevs_evt.info.immd.info.fevsReq.msg.buf = data;

	/*This invocation makes it appear as if a fevs call arrived at the IMMD.
	   In actuality, this call needed extra processing (the global id) so
	   it was not sent IMMND->IMMD as a fevs call but as a dedicated call. */
	proc_rc = immd_evt_proc_fevs_req(cb, &(fevs_evt.info.immd), sinfo, FALSE);

 fail:
	if (tmpData) {
		free(tmpData);
	}

	if (uba.start) {
		m_MMGR_FREE_BUFR_LIST(uba.start);
	}
	TRACE_LEAVE();
	return proc_rc;
}

/****************************************************************************
 * Name          : immd_evt_proc_abort_ccb
 *
 * Description   : Function to process the IMMD_EVT_ND2D_ABORT_CCB
 *                 event, from IMMND.
 *
 * Arguments     : IMMD_CB *cb - IMMD CB pointer
 *                 IMMSV_EVT *evt - Received Event structure
 *
 * Return Values : NCSCC_RC_SUCCESS/Error.
 *****************************************************************************/
static uns32 immd_evt_proc_abort_ccb(IMMD_CB *cb, IMMD_EVT *evt, IMMSV_SEND_INFO *sinfo)
{
	IMMSV_EVT fevs_evt;
	uns32 proc_rc = NCSCC_RC_SUCCESS;
	SaUint32T ccbId = evt->info.ccbId;
	NCS_UBAID uba;
	char *tmpData = NULL;
	uba.start = NULL;
	TRACE_ENTER();
	TRACE_5("Abort CCB ID:%u", ccbId);

	/*Create and pack the core message for fevs. */

	memset(&fevs_evt, 0, sizeof(IMMSV_EVT));
	fevs_evt.type = IMMSV_EVT_TYPE_IMMND;
	fevs_evt.info.immnd.type = IMMND_EVT_D2ND_ABORT_CCB;
	fevs_evt.info.immnd.info.ccbId = ccbId;

	proc_rc = ncs_enc_init_space(&uba);
	if (proc_rc != NCSCC_RC_SUCCESS) {
		LOG_ER("Failed init ubaid");
		uba.start = NULL;
		goto fail;
	}

	proc_rc = immsv_evt_enc(&fevs_evt, &uba);
	if (proc_rc != NCSCC_RC_SUCCESS) {
		LOG_ER("Failed encode fevs");
		uba.start = NULL;
		goto fail;
	}

	int32 size = uba.ttl;
	tmpData = malloc(size);
	assert(tmpData);
	char *data = m_MMGR_DATA_AT_START(uba.start, size, tmpData);

	memset(&fevs_evt, 0, sizeof(IMMSV_EVT));	/*No ponters=>no leak */
	fevs_evt.type = IMMSV_EVT_TYPE_IMMD;
	fevs_evt.info.immd.type = 0;
	fevs_evt.info.immd.info.fevsReq.msg.size = size;
	fevs_evt.info.immd.info.fevsReq.msg.buf = data;

	/*This invocation makes it appear as if a fevs call arrived at the IMMD.
	   In actuality, this call needed extra processing (the global id) so
	   it was not sent IMMND->IMMD as a fevs call but as a dedicated call. */
	proc_rc = immd_evt_proc_fevs_req(cb, &(fevs_evt.info.immd), sinfo, FALSE);

 fail:
	if (tmpData) {
		free(tmpData);
	}

	if (uba.start) {
		m_MMGR_FREE_BUFR_LIST(uba.start);
	}
	TRACE_LEAVE();
	return proc_rc;
}

/****************************************************************************
 * Name          : immd_evt_proc_admo_hard-finalize
 *
 * Description   : Function to process the IMMD_EVT_ND2D_ADMO_HARD_FINALIZE
 *                 event, from IMMND.
 *
 * Arguments     : IMMD_CB *cb - IMMD CB pointer
 *                 IMMSV_EVT *evt - Received Event structure
 *
 * Return Values : NCSCC_RC_SUCCESS/Error.
 *****************************************************************************/
static uns32 immd_evt_proc_admo_hard_finalize(IMMD_CB *cb, IMMD_EVT *evt, IMMSV_SEND_INFO *sinfo)
{
	IMMSV_EVT fevs_evt;
	uns32 proc_rc = NCSCC_RC_SUCCESS;
	SaUint32T admoId = evt->info.admoId;
	NCS_UBAID uba;
	char *tmpData = NULL;
	uba.start = NULL;
	TRACE_ENTER();
	TRACE_5("Hard finalize of admin owner ID:%u", admoId);

	/* Create and pack the core message for fevs. */

	memset(&fevs_evt, 0, sizeof(IMMSV_EVT));
	fevs_evt.type = IMMSV_EVT_TYPE_IMMND;
	fevs_evt.info.immnd.type = IMMND_EVT_D2ND_ADMO_HARD_FINALIZE;
	fevs_evt.info.immnd.info.admFinReq.adm_owner_id = admoId;

	proc_rc = ncs_enc_init_space(&uba);
	if (proc_rc != NCSCC_RC_SUCCESS) {
		LOG_ER("Failed init ubaid");
		uba.start = NULL;
		goto fail;
	}

	proc_rc = immsv_evt_enc(&fevs_evt, &uba);
	if (proc_rc != NCSCC_RC_SUCCESS) {
		LOG_ER("Failed encode fevs");
		uba.start = NULL;
		goto fail;
	}

	int32 size = uba.ttl;
	tmpData = malloc(size);
	assert(tmpData);
	char *data = m_MMGR_DATA_AT_START(uba.start, size, tmpData);

	memset(&fevs_evt, 0, sizeof(IMMSV_EVT));	/*No ponters=>no leak */
	fevs_evt.type = IMMSV_EVT_TYPE_IMMD;
	fevs_evt.info.immd.type = 0;
	fevs_evt.info.immd.info.fevsReq.msg.size = size;
	fevs_evt.info.immd.info.fevsReq.msg.buf = data;

	/*This invocation makes it appear as if a fevs call arrived at the IMMD.
	   In actuality, this call needed extra processing (the global id) so
	   it was not sent IMMND->IMMD as a fevs call but as a dedicated call. */
	proc_rc = immd_evt_proc_fevs_req(cb, &(fevs_evt.info.immd), sinfo, FALSE);

 fail:
	if (tmpData) {
		free(tmpData);
	}

	if (uba.start) {
		m_MMGR_FREE_BUFR_LIST(uba.start);
	}
	TRACE_LEAVE();
	return proc_rc;
}

/****************************************************************************
 * Name          : immd_evt_proc_ccbinit_req
 *
 * Description   : Function to process the IMMD_EVT_ND2D_CCBINIT_REQ event 
 *                 from IMMND.
 *
 * Arguments     : IMMD_CB *cb - IMMD CB pointer
 *                 IMMSV_EVT *evt - Received Event structure
 *
 * Return Values : NCSCC_RC_SUCCESS/Error.
 *
 * Notes         : None.
 *****************************************************************************/

static uns32 immd_evt_proc_ccbinit_req(IMMD_CB *cb, IMMD_EVT *evt, IMMSV_SEND_INFO *sinfo)
{
	IMMSV_EVT fevs_evt;
	IMMD_MBCSV_MSG mbcp_msg;
	SaAisErrorT rc = SA_AIS_OK;
	uns32 proc_rc = NCSCC_RC_SUCCESS;
	IMMSV_OM_CCB_INITIALIZE *ccbinit_req = &evt->info.ccb_init;
	SaUint32T globalId = 0;
	NCS_UBAID uba;
	char *tmpData = NULL;
	uba.start = NULL;
	TRACE_ENTER();

	globalId = ++(cb->ccb_id_count);
	if (cb->ccb_id_count == 0xffffffff) {
		cb->ccb_id_count = 0;
		LOG_WA("Active: ccb_id_count wrap arround");
	}

	memset(&mbcp_msg, 0, sizeof(IMMD_MBCSV_MSG));
	mbcp_msg.type = IMMD_A2S_MSG_CCBINIT;
	mbcp_msg.info.count = globalId;

	/*Checkpoint message to standby D. Asyncronous call=>dont wait for ack.
	   We can do this because we generate a fevs below which generates 
	   a checkpoint message that is syncronous, which pushes over this message.
	   Even the order of the messages will be preserved. */
	rc = immd_mbcsv_async_update(cb, &mbcp_msg);

	if (rc != SA_AIS_OK) {
		LOG_ER("Failed to replicate message to stdby send_count:%u", globalId);
		goto fail;
	}

	TRACE_5("Global CCB ID:%u", globalId);

	/*Create and pack the core message for fevs. */

	memset(&fevs_evt, 0, sizeof(IMMSV_EVT));
	fevs_evt.type = IMMSV_EVT_TYPE_IMMND;
	fevs_evt.info.immnd.type = IMMND_EVT_D2ND_CCBINIT;

	fevs_evt.info.immnd.info.ccbinitGlobal.globalCcbId = globalId;
	fevs_evt.info.immnd.info.ccbinitGlobal.i = *ccbinit_req;

	proc_rc = ncs_enc_init_space(&uba);
	if (proc_rc != NCSCC_RC_SUCCESS) {
		LOG_ER("Failed init ubaid");
		uba.start = NULL;
		goto fail;
	}
	proc_rc = immsv_evt_enc(&fevs_evt, &uba);
	if (proc_rc != NCSCC_RC_SUCCESS) {
		LOG_ER("Failed encode fevs");
		uba.start = NULL;
		goto fail;
	}

	int32 size = uba.ttl;
	tmpData = malloc(size);
	assert(tmpData);
	char *data = m_MMGR_DATA_AT_START(uba.start, size, tmpData);

	memset(&fevs_evt, 0, sizeof(IMMSV_EVT));	/*No ponters=>no leak */
	fevs_evt.type = IMMSV_EVT_TYPE_IMMD;
	fevs_evt.info.immd.type = 0;
	fevs_evt.info.immd.info.fevsReq.client_hdl = ccbinit_req->client_hdl;
	fevs_evt.info.immd.info.fevsReq.reply_dest = 0LL;
	fevs_evt.info.immd.info.fevsReq.msg.size = size;
	fevs_evt.info.immd.info.fevsReq.msg.buf = data;

	/*This invocation makes it appear as if a fevs call arrived at the IMMD.
	   In actuality, this call needed extra processing (the global id) so
	   it was not sent IMMND->IMMD as a fevs call but as a dedicated call. */
	proc_rc = immd_evt_proc_fevs_req(cb, &(fevs_evt.info.immd), sinfo, FALSE);

 fail:
	if (tmpData) {
		free(tmpData);
	}

	if (uba.start) {
		m_MMGR_FREE_BUFR_LIST(uba.start);
	}
	TRACE_LEAVE();
	return proc_rc;
}

/****************************************************************************
 * Name          : immd_evt_proc_rt_modify_req
 *
 * Description   : Function to process the IMMD_EVT_ND2D_OI_OBJ_MODIFY event 
 *                 from IMMND.
 *
 * Arguments     : IMMD_CB *cb - IMMD CB pointer
 *                 IMMSV_EVT *evt - Received Event structure
 *
 * Return Values : NCSCC_RC_SUCCESS/Error.
 *
 * Notes         : None.
 *****************************************************************************/

static uns32 immd_evt_proc_rt_modify_req(IMMD_CB *cb, IMMD_EVT *evt, IMMSV_SEND_INFO *sinfo)
{
	IMMSV_EVT fevs_evt;
	IMMSV_OM_CCB_OBJECT_MODIFY *objModifyReq = &evt->info.objModify;
	uns32 proc_rc = NCSCC_RC_SUCCESS;

	NCS_UBAID uba;
	char *tmpData = NULL;
	uba.start = NULL;
	TRACE_ENTER();

	/*
	   TRACE_2("ABT immd_evt_proc_rt_modify_req object:%s",
	   evt->info.objModify.objectName.buf);
	   TRACE_2("ABT attrMods:%p ", 
	   evt->info.objModify.attrMods);
	 */

	/* Create and pack the core message for fevs. The only reason this
	   message was not packet for fevs at the agent was that in one
	   use case, the message is processed dierctly by the IMMND. 
	   Not also the "ugly" use of the message type IMMND_EVT_A2ND_OI_OBJ_MODIFY
	   as the A2ND prefix says that the message came from an agent, which is
	   only half true. 
	 */

	memset(&fevs_evt, 0, sizeof(IMMSV_EVT));
	fevs_evt.type = IMMSV_EVT_TYPE_IMMND;
	fevs_evt.info.immnd.type = IMMND_EVT_A2ND_OI_OBJ_MODIFY;
	fevs_evt.info.immnd.info.objModify = *objModifyReq;
	/* Borrow pointer structures. */

	proc_rc = ncs_enc_init_space(&uba);
	if (proc_rc != NCSCC_RC_SUCCESS) {
		LOG_ER("Failed init ubaid");
		uba.start = NULL;
		goto fail;
	}

	proc_rc = immsv_evt_enc(&fevs_evt, &uba);
	if (proc_rc != NCSCC_RC_SUCCESS) {
		LOG_ER("Failed encode fevs");
		uba.start = NULL;
		goto fail;
	}

	int32 size = uba.ttl;
	tmpData = malloc(size);
	assert(tmpData);
	char *data = m_MMGR_DATA_AT_START(uba.start, size, tmpData);

	memset(&fevs_evt, 0, sizeof(IMMSV_EVT));	/*No ponters=>no leak */
	fevs_evt.type = IMMSV_EVT_TYPE_IMMD;
	fevs_evt.info.immd.type = 0;
	fevs_evt.info.immd.info.fevsReq.client_hdl = objModifyReq->immHandle;
	fevs_evt.info.immd.info.fevsReq.reply_dest = 0LL;
	fevs_evt.info.immd.info.fevsReq.msg.size = size;
	fevs_evt.info.immd.info.fevsReq.msg.buf = data;

	proc_rc = immd_evt_proc_fevs_req(cb, &(fevs_evt.info.immd), sinfo, FALSE);

 fail:
	if (tmpData) {
		free(tmpData);
	}

	if (uba.start) {
		m_MMGR_FREE_BUFR_LIST(uba.start);
	}
	free(objModifyReq->objectName.buf);
	objModifyReq->objectName.buf = NULL;
	objModifyReq->objectName.size = 0;
	immsv_free_attrmods(objModifyReq->attrMods);
	objModifyReq->attrMods = NULL;
	TRACE_LEAVE();
	return proc_rc;
}

/****************************************************************************
 * Name          : immd_evt_proc_cb_dump
 *
 * Description   : Function to dump the IMMD Control Block
 *
 * Arguments     : IMMND_CB *cb - IMMND CB pointer
 *
 * Return Values : NCSCC_RC_SUCCESS/Error.
 *
 * Notes         : None.
 *****************************************************************************/
uns32 immd_evt_proc_cb_dump(IMMD_CB *cb)
{

	immd_cb_dump();
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
 * Name          : immd_evt_proc_lga_callback
 *
 * Description   : Function process the role change message from RDA
 *
 * Arguments     : IMMND_CB *cb - IMMND CB pointer
 *                 IMMSV_EVT *evt - Received Event structure
 *
 * Return Values : NCSCC_RC_SUCCESS/Error.
 *
 * Notes         : None.
 *****************************************************************************/
static uns32 immd_evt_proc_lga_callback(IMMD_CB *cb, IMMD_EVT *evt)
{
	uns32 rc = NCSCC_RC_SUCCESS;

	TRACE_ENTER();
	if (evt->info.rda_info.io_role == PCS_RDA_ACTIVE) {
		cb->mds_role = V_DEST_RL_ACTIVE;
		cb->ha_state = SA_AMF_HA_ACTIVE;

		LOG_NO("ACTIVE request");

		if ((rc = immd_mds_change_role(cb)) != NCSCC_RC_SUCCESS) {
			LOG_ER("immd_mds_change_role FAILED");
			goto done;
		}

		if (immd_mbcsv_chgrole(cb) != NCSCC_RC_SUCCESS) {
			LOG_ER("immd_mbcsv_chgrole FAILED");
			goto done;
		}

		/* Change of role to active => We may need to elect new coord */
		immd_proc_elect_coord(cb, TRUE);
		immd_db_purge_fevs(cb);
	}
done:
	TRACE_LEAVE();
	return rc;
}

/**
 * This function is called when immd receives a MDS quiesced
 * ack. Change mbcsv state and respond to AMF.
 * @param cb
 * @param evt
 * @param sinfo
 * 
 * @return uns32
 */
static uns32 immd_evt_mds_quiesced_ack_rsp(IMMD_CB *cb, IMMD_EVT *evt, IMMSV_SEND_INFO *sinfo)
{
	TRACE_ENTER();

	if (cb->is_quiesced_set) {
		/* Update controler block */
		cb->ha_state = SA_AMF_HA_QUIESCED;
		cb->is_quiesced_set = FALSE;

		/* Inform mbcsv about the changed role */
		if (immd_mbcsv_chgrole(cb) != NCSCC_RC_SUCCESS)
			LOG_ER("mbcsv_chgrole FAILED");

		/* Finally respond to AMF */
		saAmfResponse(cb->amf_hdl, cb->amf_invocation, SA_AIS_OK);
	} else
		LOG_ER("Received IMMD_EVT_MDS_QUIESCED_ACK_RSP message but " "is_quiesced_set==false");

	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
 * Name          : immd_evt_proc_mds_evt
 *
 * Description   : Function to process the Events received from MDS
 *
 * Arguments     : IMMD_CB *cb - IMMD CB pointer
 *                 IMMSV_EVT *evt - Received Event structure
 *
 * Return Values : NCSCC_RC_SUCCESS/Error.
 * Notes: TODO ABT THis code really needs to be scrutinized. I dont
 *        pretend to understand what all of it does. Mainly cloned
 *        from CPSv.
 ****************************************************************************/
static uns32 immd_evt_proc_mds_evt(IMMD_CB *cb, IMMD_EVT *evt)
{
	IMMSV_MDS_INFO *mds_info;
	IMMD_IMMND_INFO_NODE *node_info = NULL;
	NCS_BOOL add_flag = TRUE;
	uns32 phy_slot_sub_slot;
	TRACE_ENTER();

	mds_info = &evt->info.mds_info;

	memset(&phy_slot_sub_slot, 0, sizeof(uns32));

	if (mds_info->svc_id == NCSMDS_SVC_ID_IMMND)
		TRACE_5("Received IMMND service event");
	else if (mds_info->svc_id == NCSMDS_SVC_ID_IMMD)
		TRACE_5("Received IMMD service event");
	else {
		LOG_ER("Received a service event for an unknown service %u", mds_info->svc_id);
		return NCSCC_RC_SUCCESS;
	}

	switch (mds_info->change) {
	case NCSMDS_RED_UP:
		/* get the peer mds_red_up */
		/* from the phy slot get the mds_dest of remote IMMND */
		TRACE_5("Process MDS EVT NCSMDS_RED_UP, my PID:%u", getpid());
		if (cb->node_id != mds_info->node_id) {
			MDS_DEST tmpDest = 0LL;
			TRACE_5("Remote IMMD is UP.");

			cb->immd_remote_id = immd_get_slot_and_subslot_id_from_node_id(mds_info->node_id);

			/*Check if the SBY IMMND has already identified itself. 
			   If so, we need to mark it as residing on a controller and
			   allow it to join. 
			 */
			immd_immnd_info_node_getnext(&cb->immnd_tree, &tmpDest, &node_info);
			while (node_info) {	/* while-1 */
				if (mds_info->node_id == node_info->immnd_key) {
					if (node_info->immnd_execPid && !(node_info->isOnController)) {
						/*This means that the node has already introduced
						   itself, but it did so before the MDS layer (this
						   function) presented the node as the second
						   controller node. */

						node_info->isOnController = TRUE;
						TRACE_5("Located STDBY IMMND =  %x node_id:%x",
							immd_get_slot_and_subslot_id_from_node_id(mds_info->node_id),
							mds_info->node_id);
						immd_accept_node(cb, node_info, TRUE);
					}
					/* Break out of while-1. We found */
					break;
				}
				tmpDest = node_info->immnd_dest;
				immd_immnd_info_node_getnext(&cb->immnd_tree, &tmpDest, &node_info);
			}
			/*ABT intentional fall-through ?? */
		} else
			break;

	case NCSMDS_UP:
		TRACE_5("PROCESS MDS EVT: NCSMDS_UP, my PID:%u", getpid());
		if (mds_info->svc_id == NCSMDS_SVC_ID_IMMD) {
			TRACE_5("unhandled UP case ? NCSMDS_UP for IMMD");
			goto done;
		}

		if (mds_info->svc_id == NCSMDS_SVC_ID_IMMND) {
			phy_slot_sub_slot = immd_get_slot_and_subslot_id_from_mds_dest(mds_info->dest);
			immd_immnd_info_node_find_add(&cb->immnd_tree, &mds_info->dest, &node_info, &add_flag);

			if (m_IMMND_IS_ON_SCXB(cb->immd_self_id,
					       immd_get_slot_and_subslot_id_from_mds_dest(mds_info->dest))) {
				TRACE_5("NCSMDS_UP for IMMND local");
				cb->is_loc_immnd_up = TRUE;
				cb->loc_immnd_dest = mds_info->dest;
				if (cb->ha_state == SA_AMF_HA_ACTIVE) {
					TRACE_5("NCSMDS_UP IMMND THIS IMMD is ACTIVE");
				}
			}

			/* When IMMND ON STANDBY COMES UP */
			if (m_IMMND_IS_ON_SCXB(cb->immd_remote_id,
					       immd_get_slot_and_subslot_id_from_mds_dest(mds_info->dest))) {
				TRACE_5("REMOTE IMMND UP - node:%x", mds_info->node_id);
				cb->is_rem_immnd_up = TRUE;	//ABT BUGFIX 080811
				cb->rem_immnd_dest = mds_info->dest;
				if (cb->ha_state == SA_AMF_HA_ACTIVE) {
					TRACE_5("NCSMDS_UP for REMOTE IMMND, THIS IMMD is ACTIVE");
				}
			}

			if (cb->ha_state == SA_AMF_HA_ACTIVE) {
				immd_immnd_info_node_get(&cb->immnd_tree, &mds_info->dest, &node_info);
				if (!node_info) {
					TRACE_5("ABT WE SHOULD NEVER GET HERE");
					/*ABT NOT SURE WHAT THIS BRANCH MEANS, CHECK Ckpt code */
					goto done;
				} else {
					TRACE_5("NCSMDS_UP and THIS IMMD is ACTIVE: up_proc");
				}
			}

			if (cb->ha_state == SA_AMF_HA_STANDBY) {
				immd_immnd_info_node_get(&cb->immnd_tree, &mds_info->dest, &node_info);
				if (!node_info) {
					TRACE_5("ABT WE SHOULD NEVER GET HERE. NCSMDS_UP and THIS IMMD is SBY");
					goto done;
				} else {
					TRACE_5("NCSMDS_UP and THIS IMMD is SBY");
				}
			}
		}

		break;

	case NCSMDS_DOWN:
		TRACE_5("PROCESS MDS EVT: NCSMDS_DOWN, my PID:%u", getpid());
		if (mds_info->svc_id == NCSMDS_SVC_ID_IMMD) {
			TRACE_5("unhandled DOWN case ? NCSMDS_DOWN for IMMD");
			goto done;
		}

		if (m_IMMND_IS_ON_SCXB(cb->immd_self_id, immd_get_slot_and_subslot_id_from_mds_dest(mds_info->dest))) {
			TRACE_5("NCSMDS_DOWN => local IMMND down");
			cb->is_loc_immnd_up = FALSE;
		} else if (m_IMMND_IS_ON_SCXB(cb->immd_remote_id,
					      immd_get_slot_and_subslot_id_from_mds_dest(mds_info->dest))) {
			TRACE_5("NCSMDS_DOWN, remote IMMND/IMMD ?? down");
		}

		if (cb->ha_state == SA_AMF_HA_ACTIVE) {
			immd_immnd_info_node_get(&cb->immnd_tree, &mds_info->dest, &node_info);
			if (!node_info) {
				TRACE_5("NCSMDS_DOWN and I AM ACTIVE, no info on this "
					"immnd node %llu.", mds_info->dest);
				goto done;
			} else {
				TRACE_5("IMMND DOWN PROCESS detected by IMMD");
				immd_process_immnd_down(cb, node_info, TRUE);
			}
		}

		if (cb->ha_state == SA_AMF_HA_STANDBY) {
			immd_immnd_info_node_get(&cb->immnd_tree, &mds_info->dest, &node_info);
			if (!node_info) {
				TRACE_5("NCSMDS_DOWN detected by SBY IMMD, no info on "
					"immnd node %llu", mds_info->dest);
				goto done;
			} else {
				TRACE_5("IMMND DOWN PROCESS detected by STANDBY IMMD");
				immd_process_immnd_down(cb, node_info, FALSE);
			}
		}

		if ((cb->ha_state == SA_AMF_HA_QUIESCED)  ) {
			immd_immnd_info_node_get(&cb->immnd_tree,&mds_info->dest, &node_info);
			if (!node_info) {
				TRACE_5("NCSMDS_DOWN detected by QUIESCED  IMMD, "
					"no info on immnd node %llu", mds_info->dest);
				goto done;
			} else {
				TRACE_5("IMMND DOWN PROCESS detected by QUIESCED IMMD");
				immd_process_immnd_down(cb, node_info, FALSE);
			}
		}
		break;
	default:
		TRACE_1("Unhandled MDS change: %u", mds_info->change);
		break;
	}
 done:
	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}
