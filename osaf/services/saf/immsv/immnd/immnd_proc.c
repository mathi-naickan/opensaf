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
  FILE NAME: immnd_proc.c

  DESCRIPTION: IMMND Processing routines

  FUNCTIONS INCLUDED in this module:

******************************************************************************/

#define _GNU_SOURCE
#include <libgen.h>
#include <signal.h>
#include <sys/types.h>

#include <configmake.h>
#include <nid_api.h>

#include "immnd.h"
#include "immsv_api.h"

static const char *loaderBase = "osafimmloadd";
static const char *pbeBase = "osafimmpbed";

void immnd_ackToNid(uint32_t rc)
{
	if (immnd_cb->nid_started == 0)
		return;

	if (nid_notify("IMMND", rc, NULL) != NCSCC_RC_SUCCESS) {
		LOG_ER("nid_notify failed");
		rc = NCSCC_RC_FAILURE;
	}
}

/****************************************************************************
 * Name          : immnd_proc_immd_down 
 *
 * Description   : Function to handle Director going down
 *
 * Arguments     : IMMND_CB *cb - IMMND CB pointer
 *
 * Notes         : Policy used for handling immd down is to blindly cleanup 
 *                :immnd_cb 
 ****************************************************************************/
void immnd_proc_immd_down(IMMND_CB *cb)
{
	LOG_WA("immmnd_proc_immd_down - not implemented");
}

/****************************************************************************
 * Name          : immnd_proc_imma_discard_connection
 *
 * Description   : Function to handle lost connection.
 *
 * Arguments     : IMMND_CB *cb - IMMND CB pointer
 *                 IMMND_IMM_CLIENT_NODE *cl_node - a client node.
 *
 * Return Values : false => connection was NOT completely removed (IMMD down)
 * Notes         : Policy used for handling immd down is to blindly cleanup 
 *                :immnd_cb 
 ****************************************************************************/
uint32_t immnd_proc_imma_discard_connection(IMMND_CB *cb, IMMND_IMM_CLIENT_NODE *cl_node)
{
	SaUint32T client_id;
	SaUint32T node_id;
	IMMND_OM_SEARCH_NODE *sn = NULL;
	SaUint32T implId = 0;
	IMMSV_EVT send_evt;
	SaUint32T *idArr = NULL;
	SaUint32T arrSize = 0;

	TRACE_ENTER();

	client_id = m_IMMSV_UNPACK_HANDLE_HIGH(cl_node->imm_app_hdl);
	node_id = m_IMMSV_UNPACK_HANDLE_LOW(cl_node->imm_app_hdl);
	TRACE_5("Attempting discard connection id:%llx <n:%x, c:%u>", cl_node->imm_app_hdl, node_id, client_id);
	/* Corresponds to "discardConnection" in the older EVS based IMM 
	   implementation. */

	/* 1. Discard searchOps. */
	while (cl_node->searchOpList) {
		sn = cl_node->searchOpList;
		cl_node->searchOpList = sn->next;
		sn->next = NULL;
		TRACE_5("Discarding search op %u", sn->searchId);
		immModel_deleteSearchOp(sn->searchOp);
		sn->searchOp = NULL;
		free(sn);
	}

	/* 2. Discard any continuations (locally only) that are associated with 
	   the dead connection. */

	immModel_discardContinuations(cb, client_id);

	/* No need to broadcast the discarding of the connection (as compared
	   with the EVS based implementation. In the new implementation we 
	   always look up the connection in the client_tree. Any late arrivals
	   destined for this connection will simply not find it and be discarded.
	 */

	/* 3. Take care of (at most one!) implementer associated with the dead
	   connection. Broadcast the dissapearance of the impementer via IMMD. 
	 */

	implId =		/* Assumes there is at most one implementer/conn  */
	    immModel_getImplementerId(cb, client_id);

	if (implId) {
		TRACE_5("Discarding implementer id:%u for connection: %u", implId, client_id);
		memset(&send_evt, '\0', sizeof(IMMSV_EVT));
		send_evt.type = IMMSV_EVT_TYPE_IMMD;
		send_evt.info.immd.type = IMMD_EVT_ND2D_DISCARD_IMPL;
		send_evt.info.immd.info.impl_set.r.impl_id = implId;
		if (immnd_mds_msg_send(cb, NCSMDS_SVC_ID_IMMD, cb->immd_mdest_id, &send_evt) != NCSCC_RC_SUCCESS) {
			if (immnd_is_immd_up(cb)) {
				LOG_ER("Discard implementer failed for implId:%u "
				       "but IMMD is up !? - case not handled. Client will be orphanded", implId);
			} else {
				LOG_WA("Discard implementer failed for implId:%u "
				       "(immd_down)- will retry later", implId);
			}
			cl_node->mIsStale = true;
		}
		/*Discard the local implementer directly and redundantly to avoid 
		   race conditions using this implementer (ccb's causing abort upcalls).
		 */
		immModel_discardImplementer(cb, implId, SA_FALSE);
	}

	if (cl_node->mIsStale) {
		TRACE_LEAVE();
		return false;
	}

	/* 4. Check for Ccbs that where originated from the dead connection.
	   Abort all such ccbs via broadcast over IMMD. 
	 */

	immModel_getCcbIdsForOrigCon(cb, client_id, &arrSize, &idArr);
	if (arrSize) {
		SaUint32T ix;
		memset(&send_evt, '\0', sizeof(IMMSV_EVT));
		send_evt.type = IMMSV_EVT_TYPE_IMMD;
		send_evt.info.immd.type = IMMD_EVT_ND2D_ABORT_CCB;
		for (ix = 0; ix < arrSize && !(cl_node->mIsStale); ++ix) {
			send_evt.info.immd.info.ccbId = idArr[ix];
			TRACE_5("Discarding Ccb id:%u originating at dead connection: %u", idArr[ix], client_id);
			if (immnd_mds_msg_send(cb, NCSMDS_SVC_ID_IMMD, cb->immd_mdest_id,
					       &send_evt) != NCSCC_RC_SUCCESS) {
				if (immnd_is_immd_up(cb)) {
					LOG_ER("Failure to broadcast discard Ccb for ccbId:%u "
					       "but IMMD is up !? - case not handled. Client will "
					       "be orphanded", implId);
				} else {
					LOG_WA("Failure to broadcast discard Ccb for ccbId:%u "
					       "(immd down)- will retry later", idArr[ix]);
				}
				cl_node->mIsStale = true;
			}
		}
		free(idArr);
		idArr = NULL;
		arrSize = 0;
	}

	if (cl_node->mIsStale) {
		TRACE_LEAVE();
		return false;
	}

	/* 5. 
	   Check for admin owners that where associated with the dead connection.
	   Finalize all such admin owners via broadcast over IMMD. */

	immModel_getAdminOwnerIdsForCon(cb, client_id, &arrSize, &idArr);
	if (arrSize) {
		SaUint32T ix;
		memset(&send_evt, '\0', sizeof(IMMSV_EVT));
		send_evt.type = IMMSV_EVT_TYPE_IMMD;
		send_evt.info.immd.type = IMMD_EVT_ND2D_ADMO_HARD_FINALIZE;
		for (ix = 0; ix < arrSize && !(cl_node->mIsStale); ++ix) {
			send_evt.info.immd.info.admoId = idArr[ix];
			TRACE_5("Hard finalize of AdmOwner id:%u originating at "
				"dead connection: %u", idArr[ix], client_id);
			if (immnd_mds_msg_send(cb, NCSMDS_SVC_ID_IMMD, cb->immd_mdest_id,
					       &send_evt) != NCSCC_RC_SUCCESS) {
				if (immnd_is_immd_up(cb)) {
					LOG_ER("Failure to broadcast discard admo0wner for ccbId:%u "
					       "but IMMD is up !? - case not handled. Client will "
					       "be orphanded", implId);
				} else {
					LOG_WA("Failure to broadcast discard admowner for id:%u "
					       "(immd down)- will retry later", idArr[ix]);
				}
				cl_node->mIsStale = true;
			}
		}
		free(idArr);
		idArr = NULL;
		arrSize = 0;
	}

	if (!cl_node->mIsStale) {
		TRACE_5("Discard connection id:%llx succeeded", cl_node->imm_app_hdl);
	}

	TRACE_LEAVE();
	return !(cl_node->mIsStale);
}

/****************************************************************************
 * Name          : immnd_proc_imma_down 
 *
 * Description   : Function to process agent going down    
 *                 from Applications. 
 *
 * Arguments     : IMMND_CB *cb - IMMND CB pointer
 *                 MDS_DEST dest - Agent MDS_DEST
 *
 * Notes         : None.
 *****************************************************************************/
void immnd_proc_imma_down(IMMND_CB *cb, MDS_DEST dest, NCSMDS_SVC_ID sv_id)
{
	IMMND_IMM_CLIENT_NODE *cl_node = NULL;
	SaImmHandleT prev_hdl;
	unsigned int count = 0;
	unsigned int failed = 0;

	/* go through the client tree  */
	immnd_client_node_getnext(cb, 0, &cl_node);

	while (cl_node) {
		prev_hdl = cl_node->imm_app_hdl;

		if ((memcmp(&dest, &cl_node->agent_mds_dest, sizeof(MDS_DEST)) == 0) && sv_id == cl_node->sv_id) {
			if (immnd_proc_imma_discard_connection(cb, cl_node)) {
				TRACE_5("Removing client id:%llx sv_id:%u", cl_node->imm_app_hdl, cl_node->sv_id);
				immnd_client_node_del(cb, cl_node);
				memset(cl_node, '\0', sizeof(IMMND_IMM_CLIENT_NODE));
				free(cl_node);
				prev_hdl = 0LL;	/* Restart iteration */
				cl_node = NULL;
				++count;
			} else {
				TRACE_5("Stale marked client id:%llx sv_id:%u", cl_node->imm_app_hdl, cl_node->sv_id);
				++failed;
			}
		}
		immnd_client_node_getnext(cb, prev_hdl, &cl_node);
	}
	if (failed) {
		TRACE_5("Removed %u IMMA clients, %u STALE IMMA clients pending", count, failed);
	} else {
		TRACE_5("Removed %u IMMA clients", count);
	}
}

/****************************************************************************
 * Name          : immnd_proc_imma_discard_stales
 *
 * Description   : Function to garbage collect stale client nodes
 *                 and to send TRY_AGAIN to a sync process blocked
 *                 waiting for reply.
 *                 Only used at IMMD UP events (failover/switchover).
 *
 * Arguments     : IMMND_CB *cb - IMMND CB pointer
 *
 * Notes         : None.
 *****************************************************************************/

void immnd_proc_imma_discard_stales(IMMND_CB *cb)
{
	IMMND_IMM_CLIENT_NODE *cl_node = NULL;
	SaImmHandleT prev_hdl;
	unsigned int count = 0;
	unsigned int failed = 0;
	TRACE_ENTER();

	/* go through the client tree  */
	immnd_client_node_getnext(cb, 0, &cl_node);
	while (cl_node) {
		prev_hdl = cl_node->imm_app_hdl;
		if (cl_node->mIsStale) {
			cl_node->mIsStale = false;
			if (immnd_proc_imma_discard_connection(cb, cl_node)) {
				TRACE_5("Removing client id:%llx sv_id:%u", cl_node->imm_app_hdl, cl_node->sv_id);
				immnd_client_node_del(cb, cl_node);
				memset(cl_node, '\0', sizeof(IMMND_IMM_CLIENT_NODE));
				free(cl_node);
				prev_hdl = 0LL;	/* Restart iteration */
				cl_node = NULL;
				++count;
			} else {
				TRACE_5("Stale marked (again) client id:%llx sv_id:%u",
					cl_node->imm_app_hdl, cl_node->sv_id);
				++failed;
				/*cl_node->mIsStale = true; done in discard_connection */
			}
		} else if (cl_node->mIsSync && cl_node->mSyncBlocked) {
			IMMSV_EVT send_evt;
			LOG_NO("Detected a sync client waiting on reply, send TRY_AGAIN");
			/* Dont need any delay of this. Worst case that could happen
			   is a resend of a sync message (new fevs, not same) that has 
			   actually already been processed. But such a duplicate sync
			   message (class_create/sync_object) will be rejected and
			   cause the sync to fail. This logic at least gives the sync
			   a chance to succeed. The finalize sync message is replied
			   immediately to agent, before it is sent over fevs. If the
			   immd is down then the agent gets immediate reply. So blocking
			   on immd down is not relevant for finalize sync.
			 */
			memset(&send_evt, '\0', sizeof(IMMSV_EVT));
			send_evt.type = IMMSV_EVT_TYPE_IMMA;
			send_evt.info.imma.info.errRsp.error = SA_AIS_ERR_TRY_AGAIN;
			send_evt.info.imma.type = IMMA_EVT_ND2A_IMM_ERROR;
			if (immnd_mds_send_rsp(cb, &(cl_node->tmpSinfo), &send_evt) != NCSCC_RC_SUCCESS) {
				LOG_ER("Failed to send result to Sync Agent");
			}
			cl_node->mSyncBlocked = false;
		} else {
			TRACE_5("No action client id :%llx sv_id:%u", cl_node->imm_app_hdl, cl_node->sv_id);
		}

		immnd_client_node_getnext(cb, prev_hdl, &cl_node);
	}
	if (failed) {
		TRACE_5("Removed %u STALE IMMA clients, %u STALE clients pending", count, failed);
	} else {
		TRACE_5("Removed %u STALE IMMA clients", count);
	}

	TRACE_LEAVE();
}

/**************************************************************************
 * Name     :  immnd_introduceMe
 *
 * Description   : Generate initial message to IMMD, providing epoch etc
 *                 allowing IMMND coord to begin IMM global load or sync
 *                 sequence. 
 *
 * Return Values : NONE
 *
 *****************************************************************************/
uint32_t immnd_introduceMe(IMMND_CB *cb)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	IMMSV_EVT send_evt;
	char * mdirDup = NULL;
	memset(&send_evt, '\0', sizeof(IMMSV_EVT));

	send_evt.type = IMMSV_EVT_TYPE_IMMD;
	send_evt.info.immd.type = IMMD_EVT_ND2D_INTRO;
	send_evt.info.immd.info.ctrl_msg.ndExecPid = cb->mMyPid;
	send_evt.info.immd.info.ctrl_msg.epoch = cb->mMyEpoch;
	if(cb->mIntroduced) {
		send_evt.info.immd.info.ctrl_msg.refresh = SA_TRUE;
		send_evt.info.immd.info.ctrl_msg.pbeEnabled = /*see immsv_d2nd_control in immsv_ev.h*/
			(cb->mPbeFile)?((cb->mRim == SA_IMM_KEEP_REPOSITORY)?4:3):2;
		TRACE("Refresher immnd_introduceMe pbeEnabled:%u NOT with file params",
			send_evt.info.immd.info.ctrl_msg.pbeEnabled);
	} else if(cb->mPbeFile) {
		/* OpenSAF 4.4: First time send dir/file/pbe-file info so that IMMD can
		   verify cluster consistent file setup. Pbe is by default disabled.
		   May turn out to be enabled after loading/sync. 
		   OpenSAF 4.4 uses an extension of the intro protocol that is 
		   backwards compatible. By setting msg.pbeEnabled to 2, 3 or 4
		   an IMMD executing 4.4 is aware of the potential extension and will
		   act on the added info. Old implementaitons simply test on pbeEnabled
		   being zero or non-zero, discarding the trailing file params in the message.
		   OpenSAF 4.4 will not verify file params for nodes introducing themselves
		   using 0/1 for pbeEnabled (i.e. pre 4.4 nodes), to allow upgrade to 4.4.
		*/
		send_evt.info.immd.info.ctrl_msg.pbeEnabled = 
			(cb->mPbeFile)?((cb->mRim == SA_IMM_KEEP_REPOSITORY)?4:3):2;
		TRACE("First immnd_introduceMe, sending pbeEnabled:%u WITH params",
			send_evt.info.immd.info.ctrl_msg.pbeEnabled);
		if(cb->mDir) {
			int len = strlen(cb->mDir);
			if( cb->mDir[len-1] == '/') {
				mdirDup = strndup((char *)cb->mDir, len-1);
			} else {
				mdirDup = strdup(cb->mDir);
			}	
		}
		send_evt.info.immd.info.ctrl_msg.dir.size = strlen(mdirDup)+1;
		send_evt.info.immd.info.ctrl_msg.dir.buf = (char *) mdirDup;
		send_evt.info.immd.info.ctrl_msg.xmlFile.size = strlen(cb->mFile)+1;
		send_evt.info.immd.info.ctrl_msg.xmlFile.buf = (char *) cb->mFile;
		send_evt.info.immd.info.ctrl_msg.pbeFile.size = strlen(cb->mPbeFile)+1;
		send_evt.info.immd.info.ctrl_msg.pbeFile.buf = (char *) cb->mPbeFile;
		/*
		if(cb->node_id == 0x2040f) {
			LOG_NO("ABT Fault inject on node 0x2040f");
			send_evt.info.immd.info.ctrl_msg.pbeFile.size = 0;
		}
		*/
	} else {
		send_evt.info.immd.info.ctrl_msg.pbeEnabled = 2; 
		/* Pbe not configured and then of course can not be enabled */
	}

	TRACE("Possibly extended intro from this IMMND pbeEnabled: %u  dirsize:%u",
		send_evt.info.immd.info.ctrl_msg.pbeEnabled,
		send_evt.info.immd.info.ctrl_msg.dir.size);

	if (!immnd_is_immd_up(cb)) {
		return NCSCC_RC_FAILURE;
	}

	rc = immnd_mds_msg_send(cb, NCSMDS_SVC_ID_IMMD, cb->immd_mdest_id, &send_evt);
	free(mdirDup);
	return rc;
}

/**************************************************************************
 * Name     :  immnd_iAmCoordinator
 *
 * Description   : Determines if this IMMND can take on the role of
 *                 coordinator at this moment in time.
 *
 * Return Values : NONE
 *
 * Notes         : 
 *****************************************************************************/
static int32_t immnd_iAmCoordinator(IMMND_CB *cb)
{
	if (!cb->mIntroduced)
		return (-1);

	if(cb->m2Pbe && cb->mCanBeCoord == 2) {
	        TRACE("2PBE enabled, coord not chosen yet.");
		return (-1);
	}

	if (cb->mIsCoord) {
		if (cb->mRulingEpoch > cb->mMyEpoch) {
			LOG_ER("%u > %u, exiting", cb->mRulingEpoch, cb->mMyEpoch);
			exit(1);
		}

		if(cb->m2Pbe == 2  && cb->mMyEpoch) {
			LOG_WA("Verified epochs, but not zero at loading RE:%u ME:%u",
				cb->mRulingEpoch, cb->mMyEpoch);
		}

		return 1;
	}

	return 0;
}

static int32_t immnd_iAmLoader(IMMND_CB *cb)
{
	int coord = immnd_iAmCoordinator(cb);
	if (coord == -1) {
		LOG_IN("Loading is not possible, no coordinator");
		return (-1);
	}

	if(cb->preLoadPid) {
		TRACE_5("Loading is not possible, preLoader still attached");
		return (-3);
	}

	if (cb->mMyEpoch != cb->mRulingEpoch) {
		/*We are joining the cluster, need to sync this IMMND. */
		return (-2);
	}

	/*Loading is possible.
	   Allow all fevs messages to be handled by this node. */
	cb->mAccepted = true;

	if (coord == 1) {
		if ((cb->mRulingEpoch != 0) || (cb->mMyEpoch != 0)) {
			return (-1);
		}
		return (cb->mMyEpoch + 1);
	}

	return 0;
}

static uint32_t immnd_announceLoading(IMMND_CB *cb, int32_t newEpoch)
{
	TRACE_ENTER();
	uint32_t rc = NCSCC_RC_SUCCESS;
	IMMSV_EVT send_evt;
	memset(&send_evt, '\0', sizeof(IMMSV_EVT));

	send_evt.type = IMMSV_EVT_TYPE_IMMD;
	send_evt.info.immd.type = IMMD_EVT_ND2D_ANNOUNCE_LOADING;
	send_evt.info.immd.info.ctrl_msg.ndExecPid = cb->mMyPid;
	send_evt.info.immd.info.ctrl_msg.epoch = newEpoch;
	send_evt.info.immd.info.ctrl_msg.pbeEnabled = /*see immsv_d2nd_control in immsv_ev.h*/
		(cb->mPbeFile)?((cb->mRim == SA_IMM_KEEP_REPOSITORY)?4:3):2;

	if (immnd_is_immd_up(cb)) {
		rc = immnd_mds_msg_send(cb, NCSMDS_SVC_ID_IMMD, cb->immd_mdest_id, &send_evt);
	} else {
		rc = NCSCC_RC_FAILURE;
	}

	if (rc != NCSCC_RC_SUCCESS) {
		LOG_ER("Coord failed to send start of load to IMMD");
		/*Revert state back to IMM_SERVER_CLUSTER_WAITING.
		   cb->mState = IMM_SERVER_CLUSTER_WAITING; */
	} else {
		cb->mMyEpoch = newEpoch;
	}
	TRACE_LEAVE();
	return rc;
}

static uint32_t immnd_requestSync(IMMND_CB *cb)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	IMMSV_EVT send_evt;
	memset(&send_evt, '\0', sizeof(IMMSV_EVT));

	send_evt.type = IMMSV_EVT_TYPE_IMMD;
	send_evt.info.immd.type = IMMD_EVT_ND2D_REQ_SYNC;
	send_evt.info.immd.info.ctrl_msg.ndExecPid = cb->mMyPid;
	send_evt.info.immd.info.ctrl_msg.epoch = cb->mMyEpoch;
	send_evt.info.immd.info.ctrl_msg.pbeEnabled = /*see immsv_d2nd_control in immsv_ev.h*/
		(cb->mPbeFile)?((cb->mRim == SA_IMM_KEEP_REPOSITORY)?4:3):2;
	if (immnd_is_immd_up(cb)) {
		rc = immnd_mds_msg_send(cb, NCSMDS_SVC_ID_IMMD, cb->immd_mdest_id, &send_evt);
	} else {
		rc = NCSCC_RC_FAILURE;
	}
	return (rc == NCSCC_RC_SUCCESS);
}

void immnd_announceDump(IMMND_CB *cb)
{
	IMMSV_EVT send_evt;
	memset(&send_evt, '\0', sizeof(IMMSV_EVT));

	send_evt.type = IMMSV_EVT_TYPE_IMMD;
	send_evt.info.immd.type = IMMD_EVT_ND2D_ANNOUNCE_DUMP;
	send_evt.info.immd.info.ctrl_msg.ndExecPid = cb->mMyPid;
	send_evt.info.immd.info.ctrl_msg.epoch = cb->mMyEpoch;
	send_evt.info.immd.info.ctrl_msg.pbeEnabled = /*see immsv_d2nd_control in immsv_ev.h*/
		(cb->mPbeFile)?((cb->mRim == SA_IMM_KEEP_REPOSITORY)?4:3):2;
	if (immnd_is_immd_up(cb)) {
		(void)immnd_mds_msg_send(cb, NCSMDS_SVC_ID_IMMD, cb->immd_mdest_id, &send_evt);
	}
	/* Failure not handled. Should not be a problem. */

	if(cb->mState == IMM_SERVER_READY) {
		/* Reset jobStart timer to delay potential start of sync.
		   Reduces risk of epoch race with dump.
		 */
		cb->mJobStart = time(NULL);
		osafassert(cb->mJobStart >= ((time_t) 0));
	}
}

static SaInt32T immnd_syncNeeded(IMMND_CB *cb)
{
	/*if(immnd_iAmCoordinator(cb) != 1) {return 0;} Only coord can start sync
	   The fact that we are IMMND coord has laready been checked. */

	if (cb->mSyncRequested) {
		if(cb->mLostNodes) {
			/* We have lost more nodes than have returned to request sync
			   Wait a bit more to allow them to join.
			 */
			LOG_IN("Postponing sync, waiting for %u nodes", cb->mLostNodes);
			cb->mLostNodes--; 
		} else {
			cb->mSyncRequested = false;	/*reset sync request marker */
			return (cb->mMyEpoch + 1);
		}
	}

	return 0;
}

static uint32_t immnd_announceSync(IMMND_CB *cb, SaUint32T newEpoch)
{
	/* announceSync can get into a race on epoch with announceDump. This is because
	   announcedump can be generated at the non-coord SC. The epoch sequence
	   should be corrected by the IMMD before replying with dumpOk/syncOk. 
	 */
	uint32_t rc = NCSCC_RC_SUCCESS;
	IMMSV_EVT send_evt;
	memset(&send_evt, '\0', sizeof(IMMSV_EVT));
	LOG_NO("Announce sync, epoch:%u", newEpoch);

	send_evt.type = IMMSV_EVT_TYPE_IMMD;
	send_evt.info.immd.type = IMMD_EVT_ND2D_SYNC_START;
	send_evt.info.immd.info.ctrl_msg.ndExecPid = cb->mMyPid;
	send_evt.info.immd.info.ctrl_msg.epoch = newEpoch;
	send_evt.info.immd.info.ctrl_msg.pbeEnabled = /*see immsv_d2nd_control in immsv_ev.h*/
		(cb->mPbeFile)?((cb->mRim == SA_IMM_KEEP_REPOSITORY)?4:3):2;

	if (immnd_is_immd_up(cb)) {
		rc = immnd_mds_msg_send(cb, NCSMDS_SVC_ID_IMMD, cb->immd_mdest_id, &send_evt);
	} else {
		rc = NCSCC_RC_FAILURE;
	}

	if (rc != NCSCC_RC_SUCCESS) {
		LOG_ER("Coord failed to send start of sync to IMMD");
		cb->mSyncRequested = true; /* revert sync request marker back to on */
		return 0;
	}

	cb->mPendSync = 1;	//Avoid starting the sync before everyone is readonly

	/*cb->mMyEpoch = newEpoch; */
	/* Wait, let immd decide if sync will be started */

	return 1;
}

IMMSV_ADMIN_OPERATION_PARAM *
immnd_getOsafImmPbeAdmopParam(SaImmAdminOperationIdT operationId, void* evt,
	IMMSV_ADMIN_OPERATION_PARAM * param)
{
	const char * classNameParamName = "className";
	const char* epochStr = OPENSAF_IMM_ATTR_EPOCH;

	IMMSV_OM_CLASS_DESCR* classDescr=NULL;
	IMMND_CB *cb = NULL;
	switch(operationId) {
		case OPENSAF_IMM_PBE_CLASS_CREATE:
		case OPENSAF_IMM_PBE_CLASS_DELETE:
			classDescr = (IMMSV_OM_CLASS_DESCR *) evt;
			param->paramName.size = (SaUint32T) strlen(classNameParamName) + 1;
			param->paramName.buf = (char *) classNameParamName;
			param->paramType = SA_IMM_ATTR_SASTRINGT;
			param->paramBuffer.val.x = classDescr->className;
			param->next = NULL;
			break;

		case OPENSAF_IMM_PBE_UPDATE_EPOCH:
			cb = (IMMND_CB *) evt;
			param->paramName.size = (SaUint32T) strlen(epochStr) + 1;
			param->paramName.buf = (char *) epochStr;
			param->paramType = SA_IMM_ATTR_SAUINT32T;
			param->paramBuffer.val.sauint32 = cb->mMyEpoch;
			param->next = NULL;
	}
	return param;
}


void immnd_adjustEpoch(IMMND_CB *cb, SaBoolT increment)
{
	SaUint32T pbeConn = 0;
	NCS_NODE_ID pbeNodeId = 0;
	NCS_NODE_ID *pbeNodeIdPtr = NULL;
	SaUint32T continuationId = 0;
	uint16_t retryCount = 0;
	TRACE_ENTER2("Epoch on entry:%u", cb->mMyEpoch);

	/*Correct epoch for counter loaded from backup/sync.
	  Also push the PRTO epoch attribute to PBE if present.
	 */

	if(cb->mPbeFile && (cb->mRim == SA_IMM_KEEP_REPOSITORY)) {
		pbeNodeIdPtr = &pbeNodeId;
		TRACE("We expect there to be a PBE");
		/* If pbeNodeIdPtr is NULL then rtObjectUpdate skips the lookup
		   of the pbe implementer.
		 */
	}


	int newEpoch = immModel_adjustEpoch(cb, cb->mMyEpoch,
		&continuationId, &pbeConn, pbeNodeIdPtr, increment);
	if (newEpoch != cb->mMyEpoch) {
		/*This case only relevant when persistent epoch overrides
		   last epoch, i.e. after reload at cluster start. */
		TRACE_5("Adjusting epoch to:%u", newEpoch);
		cb->mMyEpoch = newEpoch;
		if (cb->mRulingEpoch != newEpoch) {
			osafassert(cb->mRulingEpoch < newEpoch);
			cb->mRulingEpoch = newEpoch;
		}

	}

	while((immnd_introduceMe(cb) != NCSCC_RC_SUCCESS) && (retryCount++ < 20)) {
		LOG_WA("Coord blocked in globalizing epoch change when IMMD is DOWN %u", retryCount);
		sleep(1);
	}
	osafassert(retryCount < 20);

	if(pbeNodeId && pbeConn) {
		IMMND_IMM_CLIENT_NODE *pbe_cl_node = NULL;
		SaImmOiHandleT implHandle = 0LL;


		/*The persistent back-end is executing at THIS node. */
		osafassert(cb->mIsCoord);
		osafassert(pbeNodeId == cb->node_id);
		implHandle = m_IMMSV_PACK_HANDLE(pbeConn, pbeNodeId);

		/*Fetch client node for PBE */
		immnd_client_node_get(cb, implHandle, &pbe_cl_node);
		osafassert(pbe_cl_node);
		if (pbe_cl_node->mIsStale) {
			LOG_WA("PBE is down => persistify of epoch is dropped!");
		} else {
			IMMSV_EVT send_evt;
			const char* osafImmDn = OPENSAF_IMM_OBJECT_DN;
			IMMSV_ADMIN_OPERATION_PARAM param;
			memset(&param, '\0', sizeof(IMMSV_ADMIN_OPERATION_PARAM));
			memset(&send_evt, '\0', sizeof(IMMSV_EVT));
			send_evt.type = IMMSV_EVT_TYPE_IMMA;
			send_evt.info.imma.type = IMMA_EVT_ND2A_IMM_PBE_ADMOP;
			send_evt.info.imma.info.admOpReq.adminOwnerId = 0; /* Only allowed for PBE. */
			send_evt.info.imma.info.admOpReq.operationId = OPENSAF_IMM_PBE_UPDATE_EPOCH;

			send_evt.info.imma.info.admOpReq.continuationId = implHandle;
			send_evt.info.imma.info.admOpReq.invocation = continuationId;
			send_evt.info.imma.info.admOpReq.timeout = 0;
			send_evt.info.imma.info.admOpReq.objectName.size = (SaUint32T) strlen(osafImmDn) + 1;
			send_evt.info.imma.info.admOpReq.objectName.buf = (char *) osafImmDn;
			send_evt.info.imma.info.admOpReq.params =
				immnd_getOsafImmPbeAdmopParam(OPENSAF_IMM_PBE_UPDATE_EPOCH,
					cb, &param);

			TRACE_2("MAKING PBE-IMPLEMENTER PERSISTENT UPDATE EPOCH upcall");
			if (immnd_mds_msg_send(cb, NCSMDS_SVC_ID_IMMA_OI,
				    pbe_cl_node->agent_mds_dest, &send_evt) != NCSCC_RC_SUCCESS)
			{
				LOG_WA("Upcall over MDS for persistent class create "
					"to PBE failed!");
			}
		}
	}

	TRACE_LEAVE();
}

static void immnd_abortLoading(IMMND_CB *cb)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	IMMSV_EVT send_evt;
	memset(&send_evt, '\0', sizeof(IMMSV_EVT));

	if (!immnd_is_immd_up(cb)) {
		LOG_ER("IMMD IS DOWN - Coord can not send 'loading failed' message to IMMD");
		return;
	}

	send_evt.type = IMMSV_EVT_TYPE_IMMD;
	send_evt.info.immd.type = IMMD_EVT_ND2D_LOADING_FAILED;
	send_evt.info.immd.info.ctrl_msg.ndExecPid = cb->mMyPid;
	send_evt.info.immd.info.ctrl_msg.epoch = cb->mMyEpoch;
	send_evt.info.immd.info.ctrl_msg.pbeEnabled = /*see immsv_d2nd_control in immsv_ev.h*/
		(cb->mPbeFile)?((cb->mRim == SA_IMM_KEEP_REPOSITORY)?4:3):2;

	rc = immnd_mds_msg_send(cb, NCSMDS_SVC_ID_IMMD, cb->immd_mdest_id, &send_evt);

	if (rc != NCSCC_RC_SUCCESS) {
		LOG_ER("Coord failed to send 'laoding failed' message to IMMD");
	}
}

SaBoolT immnd_syncComplete(IMMND_CB *cb, SaBoolT coordinator, SaUint32T jobDuration)
{				/*Invoked by sync-coordinator and sync-clients.
				   Other old-member nodes do not invoke.
				*/

#if 0   /* Enable this code only to test logic for handling coord crash in
	   sync */
	if (coordinator && cb->mMyEpoch == 4) {
		LOG_NO("FAULT INJECTION crash during sync, exiting");
		exit(1);
	}
#endif

	SaBoolT completed;
	osafassert(cb->mSync || coordinator);
	osafassert(!(cb->mSync) || !coordinator);
	completed = immModel_syncComplete(cb);
	if (completed) {
		if (cb->mSync) {
			cb->mSync = SA_FALSE;

			/*cb->mAccepted = SA_TRUE; */
			/*BUGFIX this is too late! We arrive here not on fevs basis, 
			   but on timing basis from immnd_proc. */
			osafassert(cb->mAccepted);
		} else if(coordinator) {
			if(cb->mSyncFinalizing) {
				osafassert(cb->mState == IMM_SERVER_SYNC_SERVER);
				TRACE("Coord: Sync done, but waiting for confirmed "
					"finalizeSync. Out queue:%u", cb->fevs_out_count);
				/* We are FULLY_AVAIL in model, but wait for 
				   finalizeSync to come back to coord over fevs
				   before allowing server state to change. This
				   to avoid the start of a new sync before the
				   current sync is cleared form the system.
				 */
				completed = SA_FALSE;
			}
		}
	} else if(jobDuration > 300) { /* Five minutes! */
		/* Avoid having entire cluster write locked indefinitely.
		   Note that loader and sync *client* wait indefinitely.
		   These two are monitored by NID. 

		   But the sync server side (coord plus rest of cluster that is up and running)
		   are not monitored by NID and must not be blocked indefinitely due to a hung
		   (or extreeemely slow) sync.
		 */
		if(cb->syncPid > 0) {
			LOG_WA("STOPPING sync process pid %u after five minutes", cb->syncPid);
			kill(cb->syncPid, SIGTERM);
		}
	}
	return completed;
}

void immnd_abortSync(IMMND_CB *cb)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	IMMSV_EVT send_evt;
	uint16_t retryCount = 0;
	memset(&send_evt, '\0', sizeof(IMMSV_EVT));
	TRACE_ENTER();
	TRACE("ME:%u RE:%u", cb->mMyEpoch, cb->mRulingEpoch);
	osafassert(cb->mIsCoord);
	cb->mPendSync = 0;
	if(cb->mSyncFinalizing) {
		cb->mSyncFinalizing = 0x0;

		LOG_WA("AbortSync when finalizesync already generated, possibly sent. "
			" Epoch before adjust:%u", cb->mMyEpoch);
		/* Clear the out-queue */
		while(cb->fevs_out_count) {
			IMMSV_OCTET_STRING msg;
			SaImmHandleT clnt_hdl;
			LOG_WA("immnd_abortSync: Discarding message from fevs out-queue, remaining:%u",
				immnd_dequeue_outgoing_fevs_msg(cb, &msg, &clnt_hdl));

			free(msg.buf);
			msg.size=0;
		}
	}
	immModel_abortSync(cb);

	if (cb->mRulingEpoch == cb->mMyEpoch + 1) {
		cb->mMyEpoch = cb->mRulingEpoch;
	} else if (cb->mRulingEpoch != cb->mMyEpoch) {
		LOG_ER("immnd_abortSync not clean on epoch: RE:%u ME:%u", cb->mRulingEpoch, cb->mMyEpoch);
	}

	while (!immnd_is_immd_up(cb) && (retryCount++ < 20)) {
		LOG_WA("Coord blocked in sending ABORT_SYNC because IMMD is DOWN %u", retryCount);
		sleep(1);
	}

	immnd_adjustEpoch(cb, SA_TRUE);
	send_evt.type = IMMSV_EVT_TYPE_IMMD;
	send_evt.info.immd.type = IMMD_EVT_ND2D_SYNC_ABORT;
	send_evt.info.immd.info.ctrl_msg.ndExecPid = cb->mMyPid;
	send_evt.info.immd.info.ctrl_msg.epoch = cb->mMyEpoch;
	send_evt.info.immd.info.ctrl_msg.pbeEnabled = /*see immsv_d2nd_control in immsv_ev.h*/
		(cb->mPbeFile)?((cb->mRim == SA_IMM_KEEP_REPOSITORY)?4:3):2;

	LOG_NO("Coord broadcasting ABORT_SYNC, epoch:%u", cb->mRulingEpoch);
	rc = immnd_mds_msg_send(cb, NCSMDS_SVC_ID_IMMD, cb->immd_mdest_id, &send_evt);
	if (rc != NCSCC_RC_SUCCESS) {
		LOG_ER("Coord failed to send ABORT_SYNC over MDS err:%u", rc);
	}
	TRACE_LEAVE();
}

static void immnd_pbePrtoPurgeMutations(IMMND_CB *cb)
{
	/* Cleanup before restarting PBE. Send purge message.
	   Set veteran to false.
	   On receipt, clean-up. 
	*/
	uint32_t rc = NCSCC_RC_SUCCESS;
	IMMSV_EVT send_evt;
	memset(&send_evt, '\0', sizeof(IMMSV_EVT));
	TRACE_ENTER();
	osafassert(cb->mIsCoord);
	if(!immnd_is_immd_up(cb)) {
		TRACE("IMMD is DOWN postponing pbePrtoCleanup");
		return;
	}

	if(cb->pbePid) {
		LOG_WA("PBE appears to be still executing pid %u."
			"Can not purge prto mutations.", cb->pbePid);
		return;
	}

	if(cb->mPbeVeteran || cb->mPbeVeteranB) {
		/* Currently we can not recover results for PRTO create/delete/updates
		   from restarted PBE. 
		   If we have non completed PRTO ops toward PBE when it needs to
		   be restarted, then we are forced to restart with regeneration
		   of the DB file and abortion of the non completed PRTO ops.
		 */
		cb->mPbeVeteran = SA_FALSE;
		cb->mPbeVeteranB = SA_FALSE;
	}


	send_evt.type = IMMSV_EVT_TYPE_IMMD;
	send_evt.info.immd.type = IMMD_EVT_ND2D_PBE_PRTO_PURGE_MUTATIONS;
	send_evt.info.immd.info.ctrl_msg.ndExecPid = cb->mMyPid;
	send_evt.info.immd.info.ctrl_msg.epoch = cb->mMyEpoch;
	send_evt.info.immd.info.ctrl_msg.pbeEnabled = /*see immsv_d2nd_control in immsv_ev.h*/
		(cb->mPbeFile)?((cb->mRim == SA_IMM_KEEP_REPOSITORY)?4:3):2;

	LOG_NO("Coord broadcasting PBE_PRTO_PURGE_MUTATIONS, epoch:%u", cb->mRulingEpoch);
	rc = immnd_mds_msg_send(cb, NCSMDS_SVC_ID_IMMD, cb->immd_mdest_id, &send_evt);
	if (rc != NCSCC_RC_SUCCESS) {
		LOG_ER("Coord failed to send PBE_PRTO_PURGE_MUTATIONS over MDS err:%u", rc);
	}
	TRACE_LEAVE();
}

static void immnd_cleanTheHouse(IMMND_CB *cb, SaBoolT iAmCoordNow)
{
	SaInvocationT *admReqArr = NULL;
	SaUint32T admReqArrSize = 0;
	SaInvocationT *searchReqArr = NULL;
	SaUint32T searchReqArrSize = 0;
	SaUint32T *ccbIdArr = NULL;
	SaUint32T ccbIdArrSize = 0;
	SaUint32T *pbePrtoReqArr = NULL;
	SaUint32T pbePrtoReqArrSize = 0;

	SaInvocationT inv;
	SaUint32T reqConn = 0;
	SaUint32T dummyImplConn = 0;
	SaUint64T reply_dest = 0LL;
	unsigned int ix;
	SaImmHandleT tmp_hdl = 0LL;
	IMMND_IMM_CLIENT_NODE *cl_node = NULL;
	uint32_t rc = NCSCC_RC_SUCCESS;
	SaBoolT ccbsStuckInCritical=SA_FALSE;
	SaBoolT pbePrtoStuck=SA_FALSE;
	SaUint32T stuck=0;
	/*TRACE_ENTER(); */

	if(cb->mRim == SA_IMM_KEEP_REPOSITORY) {
		if(!(cb->mPbeVeteran)) {
			/*
			  If we are coord then the PBE has to be LOCAL.
			  If we are not coord then the PBE has to be remote.
			*/
			SaUint32T pbeNodeId = immModel_pbeOiExists(cb);
			cb->mPbeVeteran = pbeNodeId && immModel_pbeIsInSync(cb, false) &&
				(cb->mIsCoord) == (pbeNodeId == cb->node_id);
			if(cb->mPbeVeteran && cb->mCanBeCoord) {
				LOG_NO("PBE-OI established on %s SC. Dumping incrementally "
					"to file %s", (cb->mIsCoord)?"this":"other", 
					cb->mPbeFile);
				if(!(cb->mIsCoord)) {
					cb->other_sc_node_id = pbeNodeId;
					if(!(cb->mIsOtherScUp)) {
						LOG_ER("Late detection of other SC up");
						osafassert(!immModel_oneSafe2PBEAllowed(cb));
						cb->mIsOtherScUp = true;
					}
				}
			}
		}

		if(cb->m2Pbe && !(cb->mPbeVeteranB)) {
			/*
			  If we are coord then the PBE-B has to be remote.
			  If we are SC but not coord then the PBE-B has to be LOCAL.
			  For mPbeVeteranB (slave) we require that we currently
			  dont have ccbs in critical. If the slave crashes in the
			  gap between primary PBE having commited but slave not,
			  then the slave must not re-attach because it would rollback
			  (abort) the ccb in the local (slave's) file.
			  This can only happen in critical, becaus only in critical
			  can the PBE commit a transaction. The commit at PBE includes a 
			  successfully acked reply on a prepare, from PBE-slave to PBE.
			*/
			SaUint32T pbeSlaveNodeId = immModel_pbeBSlaveExists(cb);
		        cb->mPbeVeteranB = pbeSlaveNodeId && immModel_pbeIsInSync(cb, true) &&
				(cb->mIsCoord) != (pbeSlaveNodeId == cb->node_id);
			if(cb->mPbeVeteranB && cb->mCanBeCoord) {
				if(cb->mPbeOldVeteranB) {
					LOG_IN("PBE slave on %s SC is in sync", (cb->mIsCoord)?"other":"this");
				} else {
					LOG_NO("PBE slave established on %s SC. "
						"Dumping incrementally to file %s", 
						(cb->mIsCoord)?"other":"this", 	cb->mPbeFile);
					cb->mPbeOldVeteranB = SA_TRUE;
					if(cb->mIsCoord) {
						cb->other_sc_node_id = pbeSlaveNodeId;
						if(!(cb->mIsOtherScUp)) {
							LOG_ER("Late detection of other SC up");
							osafassert(!immModel_oneSafe2PBEAllowed(cb));
							cb->mIsOtherScUp = true;
						}
					}
				}
			}
		}
	}

	if (!immnd_is_immd_up(cb)) {
		return;
	}

	if(cb->fevs_out_list) {
		/* Push out any lingering buffered asyncronous feves messages. 
		   Normally pushed by arrival of reply over fevs.
		*/
		TRACE("cleanTheHouse push out buffered async fevs");
		dequeue_outgoing(cb); /* function body in immnd_evt.c */
	}

	stuck = immModel_cleanTheBasement(cb,
		&admReqArr,
		&admReqArrSize,
		&searchReqArr, 
		&searchReqArrSize,
		&ccbIdArr,
		&ccbIdArrSize,
		&pbePrtoReqArr,
		&pbePrtoReqArrSize,
		iAmCoordNow);

	if(stuck > 1) {
		pbePrtoStuck = SA_TRUE;
		stuck-=2;
	}
	ccbsStuckInCritical = stuck;

	if (admReqArrSize) {
		/* TODO: Correct for explicit continuation handling in the 
		   new IMM standard. */
		IMMSV_EVT send_evt;
		memset(&send_evt, 0, sizeof(IMMSV_EVT));
		for (ix = 0; ix < admReqArrSize; ++ix) {
			inv = admReqArr[ix];
			reqConn = 0;
			immModel_fetchAdmOpContinuations(cb, inv, SA_FALSE, &dummyImplConn, &reqConn, &reply_dest);
			osafassert(reqConn);
			tmp_hdl = m_IMMSV_PACK_HANDLE(reqConn, cb->node_id);

			immnd_client_node_get(cb, tmp_hdl, &cl_node);
			if (cl_node == NULL || cl_node->mIsStale) {
				LOG_WA("IMMND - Client went down so no response");
				continue;
			}
			send_evt.type = IMMSV_EVT_TYPE_IMMA;
			send_evt.info.imma.type = IMMA_EVT_ND2A_IMM_ERROR;
			send_evt.info.imma.info.admOpRsp.invocation = inv;
			send_evt.info.imma.info.errRsp.error = SA_AIS_ERR_TIMEOUT;

			SaInt32T subinv = m_IMMSV_UNPACK_HANDLE_LOW(inv);
			if (subinv < 0) {	//async-admin-op
				LOG_WA("Timeout on asyncronous admin operation %i", subinv);
				rc = immnd_mds_msg_send(cb, NCSMDS_SVC_ID_IMMA_OM, cl_node->agent_mds_dest, &send_evt);
			} else {	//sync-admin-op
				LOG_WA("Timeout on syncronous admin operation %i", subinv);
				rc = immnd_mds_send_rsp(cb, &(cl_node->tmpSinfo), &send_evt);
			}

			if (rc != NCSCC_RC_SUCCESS) {
				LOG_WA("Failure in sending reply for admin-op over MDS");
			}
		}

		free(admReqArr);
	}

	if(ccbsStuckInCritical) {
		TRACE("Try to recover outcome for ccbs stuck in critical.");
		osafassert(cb->mPbeFile); 
		/* We cant have lingering ccbs in critical if Pbe is not even 
		   configured in the system !
		*/

		if(cb->mRim == SA_IMM_KEEP_REPOSITORY) {
			SaUint32T pbeConn = 0;
			if(cb->pbePid > 0) {/* Pbe is running. */
				/* PBE may have crashed or been restarted.
				   This is the typical case where we could get 
				   ccbs stuck in critical. Fetch the list of stuck
				   ccb ids and probe the pbe for their outcome. 
				   It is also possible that a very large CCB is
				   being committed by the PBE.
				 */
				NCS_NODE_ID pbeNodeId = 0;
				SaUint32T pbeId = 0;
				int ix;
				SaUint32T *ccbIdArr = NULL;
				SaUint32T ccbIdArrSize = 0;
				immModel_getOldCriticalCcbs(cb, &ccbIdArr, &ccbIdArrSize,
					&pbeConn, &pbeNodeId, &pbeId);
				if(ccbIdArrSize) {
					osafassert(pbeConn);
					IMMND_IMM_CLIENT_NODE *oi_cl_node = NULL;
					SaImmOiHandleT implHandle = m_IMMSV_PACK_HANDLE(pbeConn, pbeNodeId);
					IMMSV_EVT send_evt;
					memset(&send_evt, 0, sizeof(IMMSV_EVT));
					send_evt.type = IMMSV_EVT_TYPE_IMMA;
					send_evt.info.imma.type = IMMA_EVT_ND2A_OI_CCB_COMPLETED_UC;
					send_evt.info.imma.info.ccbCompl.immHandle = implHandle;
					send_evt.info.imma.info.ccbCompl.implId = pbeId;
					send_evt.info.imma.info.ccbCompl.invocation = 0;/* anonymous */

					/*Fetch client node for PBE */
					immnd_client_node_get(cb, implHandle, &oi_cl_node);
					osafassert(oi_cl_node);
					osafassert(!(oi_cl_node->mIsStale));
					for (ix = 0; ix < ccbIdArrSize; ++ix) {
						TRACE_2("Fetch ccb outcome for ccb%u, nodeId:%u, conn:%u implId:%u",
							ccbIdArr[ix], pbeNodeId, pbeConn, pbeId);
						/* Generate a IMMA_EVT_ND2A_OI_CCB_COMPLETED_UC towards pbeOi
						   The pbe must somehow not reject this when it cant find the 
						   ccb-record at the IMMA_OI level and the pbe-oi level.
						   Instead generate a new ccb record at IMMA and PBE, fetch
						   ccb outcome (or discard ccb) reply. This should hopefully click
						   into the pending waitingforccbcompletedcontinuations, which should
						   casue the ccb to get resolved. 
						 */
						send_evt.info.imma.info.ccbCompl.ccbId = ccbIdArr[ix];
						TRACE_2("MAKING CCB COMPLETED RECOVERY upcall for ccb:%u", ccbIdArr[ix]);
						if(immnd_mds_msg_send(cb, NCSMDS_SVC_ID_IMMA_OI,
							   oi_cl_node->agent_mds_dest, &send_evt) != NCSCC_RC_SUCCESS) {
							LOG_ER("CCB COMPLETED RECOVERY UPCALL for %u, SEND TO PBE FAILED", 
								ccbIdArr[ix]);
						} else {
							TRACE_2("CCB COMPLETED RECOVERY UPCALL for ccb %u, SEND SUCCEEDED", 
								ccbIdArr[ix]);
						}

					}
				}
				free(ccbIdArr);
			}

			if(!pbeConn)
			{
				LOG_WA("There are ccbs blocked in critical state, "
					"waiting for pbe process to re-attach as OI");
			}

		} else {
			osafassert(cb->mRim == SA_IMM_INIT_FROM_FILE);
			/* This is a problematic case. Apparently PBE has
			   been disabled in such a way that there are ccbs left
			   blocked in critical. This should not be possible with
			   current implementation as the PBE only handles one ccb
			   commit at a time and a change of repository init mode
			   is itself done in a ccb. Future implementations or 
			   alternative backends could allow for severeal ccbs to
			   commit concurrently. 
			   Note that the PBE does handle several concurrent ccbs 
			   in the operational (NON critical) state. The NON critical
			   state includes the execution of completed upcalls by 
			   normal OIs. 
			*/
			LOG_ER("PBE has beeb disabled with ccbs in critical state - "
			       "To resolve: Enable PBE or resart/reload the cluster");
		}
	}

	if (searchReqArrSize) {
		IMMSV_OM_RSP_SEARCH_REMOTE reply;
		memset(&reply, 0, sizeof(IMMSV_OM_RSP_SEARCH_REMOTE));
		reply.requestNodeId = cb->node_id;
		TRACE_5("Timeout on search op waiting on %d implementer(s)", searchReqArrSize);
		for (ix = 0; ix < searchReqArrSize; ++ix) {
			inv = searchReqArr[ix];
			reqConn = 0;
			/*Fetch originating request continuation */
			immModel_fetchSearchReqContinuation(cb, inv, &reqConn);
			osafassert(reqConn);

			reply.searchId = m_IMMSV_UNPACK_HANDLE_LOW(inv);

			reply.result = SA_AIS_ERR_TIMEOUT;
			search_req_continue(cb, &reply, reqConn);
		}
		free(searchReqArr);
	}

	if (ccbIdArrSize) {
		IMMSV_EVT send_evt;
		memset(&send_evt, '\0', sizeof(IMMSV_EVT));
		send_evt.type = IMMSV_EVT_TYPE_IMMD;
		send_evt.info.immd.type = IMMD_EVT_ND2D_ABORT_CCB;
		for (ix = 0; ix < ccbIdArrSize; ++ix) {
			send_evt.info.immd.info.ccbId = ccbIdArr[ix];
			LOG_WA("Timeout while waiting for implementer, aborting ccb:%u", ccbIdArr[ix]);

			if (immnd_mds_msg_send(cb, NCSMDS_SVC_ID_IMMD, cb->immd_mdest_id,
				    &send_evt) != NCSCC_RC_SUCCESS) {
				LOG_ER("Failure to broadcast discard Ccb for ccbId:%u", ccbIdArr[ix]);
			}
		}
		free(ccbIdArr);
	}

	if (pbePrtoReqArrSize) {
		IMMSV_EVT reply;
		memset(&reply, 0, sizeof(IMMSV_EVT));
		reply.type = IMMSV_EVT_TYPE_IMMA;
		reply.info.imma.type = IMMA_EVT_ND2A_IMM_ERROR;
		reply.info.imma.info.errRsp.error = SA_AIS_ERR_TIMEOUT;

		for (ix = 0; ix < pbePrtoReqArrSize; ++ix) {
			SaImmHandleT tmp_hdl = m_IMMSV_PACK_HANDLE(pbePrtoReqArr[ix],
				cb->node_id);
			immnd_client_node_get(cb, tmp_hdl, &cl_node);
			if (cl_node == NULL || cl_node->mIsStale) {
				LOG_WA("IMMND - Client %u went down so no response",
					pbePrtoReqArr[ix]);
				continue;
			}
			LOG_WA("Timeout on Persistent runtime Object Mutation, waiting on PBE");
			rc = immnd_mds_send_rsp(cb, &(cl_node->tmpSinfo), &reply);
			if (rc != NCSCC_RC_SUCCESS) {
				LOG_ER("Failed to send response to agent/client "
					"over MDS rc:%u", rc);
			}
		}
		free(pbePrtoReqArr);
	}

	if(pbePrtoStuck) {
		if(cb->pbePid > 0) {
			osafassert(iAmCoordNow);
			if((cb->mPbeKills++)==0) {
				LOG_WA("PBE process %u appears stuck on runtime data handling "
					"- sending SIGTERM", cb->pbePid);
				kill(cb->pbePid, SIGTERM);
			} else if(cb->mPbeKills > 20) {
				LOG_WA("PBE process %u appears stuck on runtime data handling "
					"- sending SIGKILL", cb->pbePid);
				kill(cb->pbePid, SIGKILL);
			}
		} else if(cb->pbePid2 > 0) {
			if((cb->mPbeKills++)==0) {
				LOG_WA("STOPPING SLAVE PBE process.");
				kill(cb->pbePid2, SIGTERM);
			} else if(cb->mPbeKills > 20) {
				LOG_WA("SLAVE PBE process appears hung, sending SIGKILL");
				kill(cb->pbePid2, SIGKILL);					
			}
		}
	}

	/*TRACE_LEAVE(); */
}

void immnd_proc_global_abort_ccb(IMMND_CB *cb, SaUint32T ccbId)
{
	IMMSV_EVT send_evt;
	memset(&send_evt, '\0', sizeof(IMMSV_EVT));
	send_evt.type = IMMSV_EVT_TYPE_IMMD;
	send_evt.info.immd.type = IMMD_EVT_ND2D_ABORT_CCB;
	send_evt.info.immd.info.ccbId = ccbId;
	if (immnd_mds_msg_send(cb, NCSMDS_SVC_ID_IMMD, cb->immd_mdest_id,
		    &send_evt) != NCSCC_RC_SUCCESS) {
		LOG_ER("Failure to broadcast abort Ccb for ccbId:%u", ccbId);
	}
}


static SaBoolT immnd_ccbsTerminated(IMMND_CB *cb, SaUint32T duration, SaBoolT* pbeImmndDeadlock)
{
	osafassert(cb->mIsCoord);
	osafassert(pbeImmndDeadlock);
	(*pbeImmndDeadlock) = SA_FALSE;
	if (cb->mPendSync) {
		TRACE("ccbsTerminated false because cb->mPendSync is still true");
		return SA_FALSE;
	}

	if(!immModel_immNotWritable(cb)) {
		/* Immmodel is writable. => sync completed here at coord. */
		return SA_TRUE;
	}

	SaBoolT ccbsTerminated = immModel_ccbsTerminated(cb, false);
	SaBoolT pbeIsInSync = immModel_pbeIsInSync(cb, false);
	SaUint32T largeAdmoId = immModel_getIdForLargeAdmo(cb);

	if (ccbsTerminated && pbeIsInSync && (!largeAdmoId)) {
		return SA_TRUE;
	}

	if(!(duration % 2) && !pbeIsInSync) { /* Every two seconds check on pbe */
		if(cb->pbePid > 0) {
			/* Force PBE to restart which forces immnd/imm.db syncronization. 
			   See also immnd_cleanTheHouse() above.
			 */
			if((cb->mPbeKills++)==0) {
				LOG_WA("Persistent back end process appears hung, restarting it.");
				kill(cb->pbePid, SIGTERM);
			} else if(cb->mPbeKills > 10) {
				LOG_WA("Persistent back end process appears hung, sending SIGKILL");
				kill(cb->pbePid, SIGKILL);
			}
		} else {
			/* Purge old Prto mutations before restarting PBE */
			TRACE_5("immnd_ccbsTerminated coord/sync invoking "
				"immnd_pbePrtoPurgeMutations");
			immnd_pbePrtoPurgeMutations(cb);
		}
	}

	if(!ccbsTerminated && cb->mPbeFile && (cb->mRim == SA_IMM_KEEP_REPOSITORY) && 
		!immModel_pbeOiExists(cb) && !immModel_pbeIsInSync(cb, true)) {
		/* Started sync is blocked by critical ccbs while PBE is being restarted, See (#556) */
		(*pbeImmndDeadlock) = SA_TRUE;
	}

	if (duration % 5) {/* Wait 5 secs for non critical ccbs to terminate */
		return SA_FALSE;
	}

	if(!ccbsTerminated) {//Preemtively abort non critical Ccbs. 
		IMMSV_EVT send_evt;	//One try should be enough.
		SaUint32T *ccbIdArr = NULL;
		SaUint32T ccbIdArrSize = 0;
		memset(&send_evt, '\0', sizeof(IMMSV_EVT));
		send_evt.type = IMMSV_EVT_TYPE_IMMD;
		send_evt.info.immd.type = IMMD_EVT_ND2D_ABORT_CCB;
		int ix;
		immModel_getNonCriticalCcbs(cb, &ccbIdArr, &ccbIdArrSize);

		for (ix = 0; ix < ccbIdArrSize; ++ix) {
			send_evt.info.immd.info.ccbId = ccbIdArr[ix];
			LOG_WA("Aborting ccbId  %u to start sync", ccbIdArr[ix]);
			if (immnd_mds_msg_send(cb, NCSMDS_SVC_ID_IMMD, cb->immd_mdest_id,
				    &send_evt) != NCSCC_RC_SUCCESS) {
				LOG_ER("Failure to broadcast abort Ccb for ccbId:%u",
					ccbIdArr[ix]);
				break;	/* out of forloop to free ccbIdArr & 
					   return SA_FALSE */
			}
		}
		free(ccbIdArr);
	} else if(largeAdmoId) {
		/* Ccbs are terminated, yet there are admin-owner(s) 
		   lingering with releaseOnFinalize==true and more than
		   9999 touched objects. We need to force such adminOwners
		   to finalize. User will then get bad-handle when using the
		   adminOwnerHandle sooner or later.
		 */
		IMMSV_EVT send_evt;

		memset(&send_evt, '\0', sizeof(IMMSV_EVT));
		send_evt.type = IMMSV_EVT_TYPE_IMMD;
		send_evt.info.immd.type = IMMD_EVT_ND2D_ADMO_HARD_FINALIZE;
		send_evt.info.immd.info.admoId = largeAdmoId;
		TRACE("Sending discard admo for large admo-id:%u", largeAdmoId);

		if (immnd_mds_msg_send(cb, NCSMDS_SVC_ID_IMMD, cb->immd_mdest_id,
			    &send_evt) != NCSCC_RC_SUCCESS) {
			LOG_WA("Failure to broadcast discard admowner for id:%u "
				"- will retry later", largeAdmoId);
		}
	}

	return SA_FALSE;
}

static int immnd_forkLoader(IMMND_CB *cb, bool preLoad)
{
	char loaderName[1024];
	const char *base = basename(cb->mProgName);
	int myLen = (int) strlen(base);
	int myAbsLen = (int) strlen(cb->mProgName);
	int loaderBaseLen = (int) strlen(loaderBase);
	int pid = (-1);
	int i, j;

	TRACE_ENTER();
	if (!cb->mDir && !cb->mFile) {
		LOG_WA("No directory and no file-name=>IMM coming up empty");
		return (-1);
	}

	if ((myAbsLen - myLen + loaderBaseLen) > 1023) {
		LOG_ER("Pathname too long: %u max is 1023", myAbsLen - myLen + loaderBaseLen);
		return (-1);
	}

	for (i = 0; myAbsLen > myLen; ++i, --myAbsLen) {
		loaderName[i] = cb->mProgName[i];
	}
	for (j = 0; j < loaderBaseLen; ++i, ++j) {
		loaderName[i] = loaderBase[j];
	}
	loaderName[i] = 0;

	pid = fork();
	if (pid == (-1)) {
		LOG_ER("%s failed to fork, error %u", base, errno);
                if(preLoad) {exit(1);}
		return (-1);
	}
	if (pid == 0) {		/*Child */
		/* TODO Should close file-descriptors ... */
		char *ldrArgs[5] = { loaderName, (char *)(cb->mDir ? cb->mDir : ""),
				     (char *)(cb->mFile ? cb->mFile : "imm.xml"), 
				     (preLoad)?"preload":0, 0
		};

		execvp(loaderName, ldrArgs);
		LOG_ER("%s failed to exec, error %u, exiting", base, errno);
		exit(1);
	}
	TRACE_5("Parent %s, successfully forked loader, pid:%d", base, pid);
	TRACE_LEAVE();
	return pid;
}

static int immnd_forkSync(IMMND_CB *cb)
{
	char loaderName[1024];
	const char *base = basename(cb->mProgName);
	int myLen = (int) strlen(base);
	int myAbsLen = (int) strlen(cb->mProgName);
	int loaderBaseLen = (int) strlen(loaderBase);
	int pid = (-1);
	int i, j;
	int maxSyncBatchSize = immModel_getMaxSyncBatchSize(cb);
	char arg4[16];
	TRACE_ENTER();

	if(maxSyncBatchSize != 0) {
		if(!immModel_protocol41Allowed(cb)) {
			maxSyncBatchSize = 1; /* Revert to old 4.0 behavior */
		}
		snprintf(arg4, 16, "%u", maxSyncBatchSize);
	} else {
		arg4[0] = 0; /* Will result in default batch size, see immsv_api.h */
	}


	if ((myAbsLen - myLen + loaderBaseLen) > 1023) {
		LOG_ER("Pathname too long: %u max is 1023", myAbsLen - myLen + loaderBaseLen);
		return (-1);
	}

	for (i = 0; myAbsLen > myLen; ++i, --myAbsLen) {
		loaderName[i] = cb->mProgName[i];
	}

	for (j = 0; j < loaderBaseLen; ++i, ++j) {
		loaderName[i] = loaderBase[j];
	}
	loaderName[i] = 0;

	pid = fork();
	if (pid == (-1)) {
		LOG_ER("%s failed to fork sync-agent, error %u", base, errno);
		return (-1);
	}

	if (pid == 0) {		/*child */
		/* TODO: Should close file-descriptors ... */
		char *ldrArgs[6] = { loaderName, "", "", "sync", arg4, 0 };
		execvp(loaderName, ldrArgs);
		LOG_ER("%s failed to exec sync, error %u, exiting", base, errno);
		exit(1);
	}
	TRACE_5("Parent %s, successfully forked sync-agent, pid:%d", base, pid);
	TRACE_LEAVE();
	return pid;
}

static int immnd_forkPbe(IMMND_CB *cb)
{
	/* osafimmpbed is in the same directory as immnd,
	 * and the path will be calculated from immnd path
	 */

	const char *base = basename(cb->mProgName);
	char execPath[1024];
	char dbFilePath[1024];
	int pid = (-1);
	int execDirLen = (int) (strlen(cb->mProgName) - strlen(base));
	int dirLen = (int) strlen(cb->mDir);
	int pbeLen = (int) strlen(cb->mPbeFile);
	int i, j;
	int pbeBaseLen = strlen(pbeBase);
	int newLen = execDirLen + pbeBaseLen + (((execDirLen > 0) && (cb->mProgName[execDirLen - 1] == '/')) ? 0 : 1);
	TRACE_ENTER();

	if(newLen > 1023) {
		LOG_ER("Pathname too long: %u max is 1023", newLen);
		return -1;
	}

	strncpy(execPath, cb->mProgName, execDirLen);
	execPath[execDirLen] = 0;
	if((execDirLen == 0) || (cb->mProgName[execDirLen - 1] != '/'))
		strncat(execPath, "/", 2);
	strncat(execPath, pbeBase, pbeBaseLen + 1);

	TRACE("exec-pbe-file-path:%s", execPath);

	for (i = 0; i < dirLen; ++i) {
		dbFilePath[i] = cb->mDir[i];
	}

	dbFilePath[i++] = '/';

	for(j = 0; j < pbeLen && (i < 1024); ++i, ++j) {
		dbFilePath[i] = cb->mPbeFile[j];
	}

	if(cb->m2Pbe) {
		/* 2PBE configured, add nodeId suffix to imm.db */
		/* But just use IMMSV_PBE_FILE_SUFFIX */
		char pbeSuffix[24];
		snprintf(pbeSuffix, 24, ".%x", cb->node_id);
		for(j = 0; (j < 24) && (i < 1024); ++i, ++j) {
			dbFilePath[i] = pbeSuffix[j];
		}
	}
	dbFilePath[i] = '\0';

	LOG_NO("pbe-db-file-path:%s VETERAN:%u B:%u", dbFilePath, cb->mPbeVeteran, cb->mPbeVeteranB);

	if((cb->mPbeVeteran || cb->mPbeVeteranB) && !immModel_pbeIsInSync(cb, false)) {
		/* Currently we can not recover results for PRTO create/delete/updates
		   from restarted PBE. 
		   If we have non completed PRTO ops toward PBE when it needs to
		   be restarted, then we are forced to restart with regeneration
		   of the DB file and abortion of the non completed PRTO ops.
		 */
		cb->mPbeVeteran = SA_FALSE;
		cb->mPbeVeteranB = SA_FALSE;
	}

	pid = fork();
	if (pid == (-1)) {
		LOG_ER("%s failed to fork, error %u", base, errno);
		return (-1);
	}

	if (pid == 0) {		/*child */
		/* TODO: Should close file-descriptors ... */
		/*char * const pbeArgs[5] = { (char *) execPath, "--recover", "--pbeXX", dbFilePath, 0 };*/
		char * pbeArgs[5];
		bool veteran = (cb->mIsCoord) ? (cb->mPbeVeteran) : (cb->m2Pbe && cb->mPbeVeteranB);
		pbeArgs[0] = (char *) execPath;
		if(veteran) {
			pbeArgs[1] =  "--recover";
			pbeArgs[2] = (cb->m2Pbe)?((cb->mIsCoord)?"--pbe2A":"--pbe2B"):"--pbe";
			pbeArgs[3] = dbFilePath; 
			pbeArgs[4] =  0;
		} else {
			pbeArgs[1] = (cb->m2Pbe)?((cb->mIsCoord)?"--pbe2A":"--pbe2B"):"--pbe";
			pbeArgs[2] = dbFilePath;
			pbeArgs[3] =  0;
		}

		execvp(execPath, pbeArgs);
		LOG_ER("%s failed to exec '%s', error %u, exiting", base, pbeBase, errno);
		exit(1);
	}
	TRACE_5("Parent %s, successfully forked %s, pid:%d", base, dbFilePath, pid);
	cb->mPbeKills = 0; /* Rest kill count when we just created a new PBE. */
	if(cb->mIsCoord && cb->mPbeVeteran) {
		cb->mPbeVeteran = SA_FALSE;
		/* If pbe crashes again before succeeding to attach as PBE implementer
		   then dont try to re-attach the DB file, instead regenerate it.
		 */
	} else if(cb->m2Pbe && cb->mPbeVeteranB) {
		cb->mPbeVeteranB = SA_FALSE;
	}

	TRACE_LEAVE();
	return pid;
}

/**************************************************************************
 * Name     :  immnd_proc_server
 *
 * Description   : Function for IMMND internal periodic processing
 *
 * Arguments     : timeout IN/OUT - Current timeout period in milliseconds, can
 *                                 be altered by this function.
 *
 * Return Values : NCSCC_RC_SUCCESS/Error.
 *
 * Notes         : Error return => IMMND task should end.
 *****************************************************************************/
uint32_t immnd_proc_server(uint32_t *timeout)
{
	IMMND_CB *cb = immnd_cb;
	uint32_t rc = NCSCC_RC_SUCCESS;
	int32_t coord, newEpoch;
	int32_t printFrq = (*timeout > 100) ? 5 : 50;
	time_t now = time(NULL);
	osafassert(now >= ((time_t) 0));
	uint32_t jobDuration = (uint32_t) now - cb->mJobStart;
	SaBoolT pbeImmndDeadlock=SA_FALSE;
	if(!jobDuration) {++jobDuration;} /* Avoid jobDuraton of zero */
	/*TRACE_ENTER(); */

	if ((cb->mStep % printFrq) == 0) {
		TRACE_5("tmout:%u ste:%u ME:%u RE:%u crd:%u rim:%s 4.3A:%u 2Pbe:%u VetA/B: %u/%u othsc:%u/%x",
			*timeout, cb->mState, cb->mMyEpoch, cb->mRulingEpoch, cb->mIsCoord,
			(cb->mRim==SA_IMM_KEEP_REPOSITORY)?"KEEP_REPO":"FROM_FILE",
			immModel_protocol43Allowed(cb), cb->m2Pbe, cb->mPbeVeteran, cb->mPbeVeteranB,
			cb->mIsOtherScUp, cb->other_sc_node_id);
	}

	if (cb->mState < IMM_SERVER_DUMP) {
		*timeout = 100;	/* 0.1 sec */
	} else {
		*timeout = 1000;	/* 1 sec */
	}

	switch (cb->mState) {
	case IMM_SERVER_ANONYMOUS:	/*send introduceMe msg. */
		/*TRACE_5("IMM_SERVER_ANONYMOUS");*/
		if (immnd_introduceMe(cb) == NCSCC_RC_SUCCESS) {
			cb->mStep = 0;
			cb->mJobStart = now;
			LOG_NO("SERVER STATE: IMM_SERVER_ANONYMOUS --> IMM_SERVER_CLUSTER_WAITING");
			cb->mState = IMM_SERVER_CLUSTER_WAITING;
		}

		if(cb->is_immd_up) {
			TRACE("IMMD IS UP");
		}

		break;

	case IMM_SERVER_CLUSTER_WAITING:
		TRACE_5("IMM_CLUSTER_WAITING");
		coord = immnd_iAmCoordinator(cb);
		if (coord != (-1)) {	/* (-1) Not ready; (1) coord; (0) non-coord */
			if (coord) {
				if ((cb->mNumNodes >= cb->mExpectedNodes) || 
					(jobDuration >= cb->mWaitSecs)) {
					TRACE_5("Nodes %u >= %u || Secs %u >= %u", cb->mNumNodes,
						cb->mExpectedNodes, jobDuration, cb->mWaitSecs);
					cb->mState = IMM_SERVER_LOADING_PENDING;
					LOG_NO("SERVER STATE: IMM_SERVER_CLUSTER_WAITING -->"
					       " IMM_SERVER_LOADING_PENDING");
					cb->mStep = 0;
					cb->mJobStart = now;
				} else {
					TRACE_5("Nodes %u < %u && %u < %u", cb->mNumNodes,
						cb->mExpectedNodes, jobDuration, cb->mWaitSecs);
				}
			} else {
				/*Non coordinator goes directly to loading pending */
				cb->mState = IMM_SERVER_LOADING_PENDING;
				LOG_NO("SERVER STATE: IMM_SERVER_CLUSTER_WAITING --> IMM_SERVER_LOADING_PENDING");
				cb->mStep = 0;
				cb->mJobStart = now;
			}
		} else {	/*We are not ready to start loading yet */
			if(cb->mIntroduced) {
				if((cb->m2Pbe == 2) && !(cb->preLoadPid)) {
					cb->preLoadPid = immnd_forkLoader(cb, true);
				}
			} else if (!(jobDuration%5)) { /* Every 5 seconds */
				LOG_WA("Resending introduce-me - problems with MDS ? %u", jobDuration);
				immnd_introduceMe(cb);
			}

			if(cb->preLoadPid > 0) {
				int status = 0;
				if (waitpid(cb->preLoadPid, &status, WNOHANG) > 0) {
					cb->preLoadPid = 0;
					if(cb->m2Pbe == 2) {
						LOG_ER("Prealoader failed to obtain stats.");
						rc = NCSCC_RC_FAILURE;	/*terminate. */
						immnd_ackToNid(rc);
					} else {
						LOG_NO("Local preloader at %x completed successfully", cb->node_id);
					}
				}
			}
		}

		if (jobDuration > 50 && jobDuration > cb->mWaitSecs) {	/* No progress in 50 secs */
			LOG_ER("Failed to load/sync. Giving up after %u seconds, restarting..", jobDuration);
			rc = NCSCC_RC_FAILURE;	/*terminate. */
			immnd_ackToNid(rc);
		}
		break;

	case IMM_SERVER_LOADING_PENDING:
		TRACE_5("IMM_SERVER_LOADING_PENDING");

		if(cb->preLoadPid > 0) {
			int status = 0;
			if (waitpid(cb->preLoadPid, &status, WNOHANG) > 0) {
				cb->preLoadPid = 0;
				LOG_NO("Local preloader at %x completed successfully", cb->node_id);
			} else {
				LOG_IN("Wait for preloader to terminate before starting loader");
				break;
			}
		}

		newEpoch = immnd_iAmLoader(cb);
		if (newEpoch < 0) {	/*Loading is not possible */
			if(newEpoch == (-3)) {
				LOG_IN("IM_SERVER_LOADIN_PENDING: preLoader still attached");
			} else if ((immnd_iAmCoordinator(cb) == 1) && (newEpoch == (-2))) {
				LOG_ER("LOADING NOT POSSIBLE, CLUSTER DOES NOT AGREE ON EPOCH.. TERMINATING");
				rc = NCSCC_RC_FAILURE;
				immnd_ackToNid(rc);
			} else {
				/*Epoch == -2 means coord is available but epoch
				   missmatch => go to synch immediately.
				   Epoch == -1 means still waiting for coord. 
				   Be patient, give the coord 30 seconds to come up. */

				if (newEpoch == (-2) || (jobDuration > 30)) {
					/*Request to be synched instead. */
					LOG_IN("REQUESTING SYNC");
					if (immnd_requestSync(cb)) {
						LOG_NO("SERVER STATE: IMM_SERVER_LOADING_PENDING "
						       "--> IMM_SERVER_SYNC_PENDING");
						cb->mState = IMM_SERVER_SYNC_PENDING;
						cb->mStep = 0;
						cb->mJobStart = now;
					}
				}
			}
		} else {	/*Loading is possible */
			if (newEpoch) {
				/*Locks out stragglers */
				if (immnd_announceLoading(cb, newEpoch) == NCSCC_RC_SUCCESS) {
					LOG_NO("SERVER STATE: IMM_SERVER_LOADING_PENDING "
					       "--> IMM_SERVER_LOADING_SERVER");
					cb->mState = IMM_SERVER_LOADING_SERVER;
					cb->mStep = 0;
					cb->mJobStart = now;
				}
			} else {
				/*Non loader goes directly to loading client */
				LOG_NO("SERVER STATE: IMM_SERVER_LOADING_PENDING --> IMM_SERVER_LOADING_CLIENT");
				cb->mState = IMM_SERVER_LOADING_CLIENT;
				cb->mStep = 0;
				cb->mJobStart = now;
				if(cb->mCanBeCoord) {cb->mIsOtherScUp = true;}
			}
		}
		break;

	case IMM_SERVER_SYNC_PENDING:
		if (cb->mSync) {
			/* Sync has started */
			cb->mStep = 0;
			cb->mJobStart = now;
			LOG_NO("SERVER STATE: IMM_SERVER_SYNC_PENDING --> IMM_SERVER_SYNC_CLIENT");
			cb->mState = IMM_SERVER_SYNC_CLIENT;
			if(cb->mCanBeCoord) {cb->mIsOtherScUp = true;}
		} else {
			/* Sync has not started yet. */
			/* Sync client timeout removed. Any timeout handled by nid. */


			if (!(cb->mStep % 1000)) {/* Once 1000 steps. */
				LOG_IN("REQUESTING SYNC AGAIN %u", cb->mStep);
				immnd_requestSync(cb);
			}

			if (!(cb->mStep % 200)) {
				LOG_IN("This node still waiting to be sync'ed after %u seconds", jobDuration);
			}
		}
		break;

	case IMM_SERVER_LOADING_SERVER:
		TRACE_5("IMM_SERVER_LOADING_SERVER");
		if (cb->mMyEpoch > cb->mRulingEpoch) {
			TRACE_5("Wait for permission to start loading "
				"My epoch:%u Ruling epoch:%u", cb->mMyEpoch, cb->mRulingEpoch);
			if (jobDuration > 15) {
				LOG_WA("MDS problem-2, giving up");
				rc = NCSCC_RC_FAILURE;	/*terminate. */
				immnd_ackToNid(rc);
			}
		} else {
			if (cb->loaderPid == (-1) && immModel_readyForLoading(cb)) {
				TRACE_5("START LOADING");
				cb->loaderPid = immnd_forkLoader(cb, false);
				cb->mJobStart = now;
				if (cb->loaderPid > 0) {
					immModel_setLoader(cb, cb->loaderPid);
				} else {
					LOG_ER("Failed to fork loader :%u", cb->loaderPid);
					/* broadcast failure to load?
					   Not clear what I should do in this case.
					   Probably best to restart immnd
					   immModel_setLoader(cb, 0);
					 */
					immnd_abortLoading(cb);
					rc = NCSCC_RC_FAILURE;	/*terminate. */
					immnd_ackToNid(rc);
				}
			} else if (immModel_getLoader(cb) == 0) {	/*Success in loading */
				cb->mState = IMM_SERVER_READY;
				immnd_ackToNid(NCSCC_RC_SUCCESS);
				LOG_NO("SERVER STATE: IMM_SERVER_LOADING_SERVER --> IMM_SERVER_READY");
				cb->mJobStart = now;
				if (cb->mPbeFile) {/* Pbe enabled */
					cb->mRim = immModel_getRepositoryInitMode(cb);

					TRACE("RepositoryInitMode: %s", (cb->mRim==SA_IMM_KEEP_REPOSITORY)?
						"SA_IMM_KEEP_REPOSITORY":"SA_IMM_INIT_FROM_FILE");
				}
				/*Dont zero cb->loaderPid yet. 
				   It is used to control waiting for loader child. */
			} else {
				int status = 0;
				/* posix waitpid & WEXITSTATUS */
				if (waitpid(cb->loaderPid, &status, WNOHANG) > 0) {
					LOG_ER("LOADING APPARENTLY FAILED status:%i", WEXITSTATUS(status));
					cb->loaderPid = 0;
					/*immModel_setLoader(cb, 0); */
					immnd_abortLoading(cb);
					rc = NCSCC_RC_FAILURE;	/*terminate. */
					immnd_ackToNid(rc);
				}
			}
		}
		/* IMMND internal timeout for loading removed. Any timeout handled by nid.
		 */

		break;

	case IMM_SERVER_LOADING_CLIENT:
		TRACE_5("IMM_SERVER_LOADING_CLIENT");
		if (jobDuration > (cb->mWaitSecs ? (cb->mWaitSecs + 300) : 300)) {
			LOG_WA("Loading client timed out, waiting to be loaded - terminating");
			cb->mStep = 0;
			cb->mJobStart = now;
			/*immModel_setLoader(cb,0); */
			/*cb->mState = IMM_SERVER_ANONYMOUS; */
			rc = NCSCC_RC_FAILURE;	/*terminate. */
			immnd_ackToNid(rc);
		}
		if (immModel_getLoader(cb) == 0) {
			immnd_ackToNid(NCSCC_RC_SUCCESS);
			cb->mState = IMM_SERVER_READY;
			cb->mJobStart = now;
			LOG_NO("SERVER STATE: IMM_SERVER_LOADING_CLIENT --> IMM_SERVER_READY");
			if (cb->mPbeFile) {/* Pbe configured */
				cb->mRim = immModel_getRepositoryInitMode(cb);

				TRACE("RepositoryInitMode: %s", (cb->mRim==SA_IMM_KEEP_REPOSITORY)?
					"SA_IMM_KEEP_REPOSITORY":"SA_IMM_INIT_FROM_FILE");
			}
			/*
			   This code case duplicated in immnd_evt.c
			   Search for: "ticket:#598"
			 */
		}
		break;

	case IMM_SERVER_SYNC_CLIENT:
		TRACE_5("IMM_SERVER_SYNC_CLIENT");
		if (immnd_syncComplete(cb, SA_FALSE, 0)) {
			cb->mStep = 0;
			cb->mJobStart = now;
			cb->mState = IMM_SERVER_READY;
			immnd_ackToNid(NCSCC_RC_SUCCESS);
			LOG_NO("SERVER STATE: IMM_SERVER_SYNC_CLIENT --> IMM SERVER READY");
			/*
			   This code case duplicated in immnd_evt.c
			   Search for: "ticket:#599"
			 */
		} else if (immnd_iAmCoordinator(cb) == 1) {
			/*This case is weird. It can only happen if the last
			   operational immexec crashed during this sync. 
			   In this case we may becomme the coordinator even though
			   we are not loaded or synced. We should in this case becomme
			   the loader. */
			LOG_WA("Coordinator apparently crashed during sync. Aborting the sync.");
			rc = NCSCC_RC_FAILURE;	/*terminate. */
			immnd_ackToNid(rc);
		}
		break;

	case IMM_SERVER_SYNC_SERVER:
		if (!immnd_ccbsTerminated(cb, jobDuration, &pbeImmndDeadlock)) {
			/*Phase 1 */
			if(pbeImmndDeadlock) {
				LOG_WA("Apparent deadlock detected between IMMND sync and restarting PBE, "
				       "with Ccbs in critical. Aborting this sync attempt");
				immnd_abortSync(cb);
				if(cb->syncPid > 0) {
					LOG_WA("STOPPING sync process pid %u", cb->syncPid);
					kill(cb->syncPid, SIGTERM);
				}
				cb->mStep = 0;
				cb->mJobStart = now;
				cb->mState = IMM_SERVER_READY;
				LOG_NO("SERVER STATE: IMM_SERVER_SYNC_SERVER --> IMM SERVER READY");
			}
			if (!(cb->mStep % 60)) {
				LOG_IN("Sync Phase-1, waiting for existing "
				       "Ccbs to terminate, seconds waited: %u", jobDuration);
			}
			if (jobDuration > 20) {
				LOG_NO("Still waiting for existing Ccbs to terminate "
				       "after %u seconds. Aborting this sync attempt", jobDuration);
				immnd_abortSync(cb);
				if(cb->syncPid > 0) {
					LOG_WA("STOPPING sync process pid %u", cb->syncPid);
					kill(cb->syncPid, SIGTERM);
				}
				cb->mStep = 0;
				cb->mJobStart = now;
				cb->mState = IMM_SERVER_READY;
				LOG_NO("SERVER STATE: IMM_SERVER_SYNC_SERVER --> IMM SERVER READY");
			}

			/* PBE may intentionally be restarted by sync. Catch this here. */
			if (cb->pbePid > 0) {
				int status = 0;
				if (waitpid(cb->pbePid, &status, WNOHANG) > 0) {
					if (cb->mRim == SA_IMM_KEEP_REPOSITORY) {
						LOG_WA("Persistent back-end process has apparently died.");
					} else {
						LOG_NO("Persistent back-end process has terminated.");
					}
					cb->pbePid = 0;
					cb->mPbeKills = 0;
					if(!immModel_pbeIsInSync(cb, false)) {
						TRACE_5("Sync-server/coord invoking "
							"immnd_pbePrtoPurgeMutations");
						immnd_pbePrtoPurgeMutations(cb);
					}
				}
			}
		} else {
			/*Phase 2 */
			if (cb->syncPid <= 0) {
				/*Fork sync-agent */
				cb->syncPid = immnd_forkSync(cb);
				if (cb->syncPid <= 0) {
					LOG_ER("Failed to fork sync process");
					cb->syncPid = 0;
					cb->mStep = 0;
					cb->mJobStart = now;
					cb->mState = IMM_SERVER_READY;
					immnd_abortSync(cb);
					LOG_NO("SERVER STATE: IMM_SERVER_SYNC_SERVER --> IMM SERVER READY");
				} else {
					LOG_IN("Sync Phase-2: Ccbs are terminated, IMM in "
					       "read-only mode, forked sync process pid:%u", cb->syncPid);
				}
			} else {
				/*Phase 3 */
				if (!(cb->mStep % 60)) {
					LOG_IN("Sync Phase-3: step:%u", cb->mStep);
				}
				if (immnd_syncComplete(cb, true, jobDuration)) {
					cb->mStep = 0;
					cb->mJobStart = now;
					cb->mState = IMM_SERVER_READY;
					LOG_NO("SERVER STATE: IMM_SERVER_SYNC_SERVER --> IMM SERVER READY");
				} else if (!(cb->mSyncFinalizing)) {
					int status = 0;
					if (waitpid(cb->syncPid, &status, WNOHANG) > 0) {
						LOG_ER("SYNC APPARENTLY FAILED status:%i", WEXITSTATUS(status));
						cb->syncPid = 0;
						cb->mStep = 0;
						cb->mJobStart = now;
						LOG_NO("-SERVER STATE: IMM_SERVER_SYNC_SERVER --> IMM_SERVER_READY");
						cb->mState = IMM_SERVER_READY;
						immnd_abortSync(cb);
					}
				}
			}
		}
		break;

	case IMM_SERVER_READY:	/*Normal stable service state. */
		/* Do some periodic housekeeping */

		if (cb->loaderPid > 0) {
			int status = 0;
			if (waitpid(cb->loaderPid, &status, WNOHANG) > 0) {
				/*Expected termination of loader process.
				   Should check status.
				   But since loading apparently succeeded, we dont care. */
				cb->loaderPid = 0;
			}
		}

		if (cb->syncPid > 0) {
			int status = 0;
			if (waitpid(cb->syncPid, &status, WNOHANG) > 0) {
				/*Expected termination of sync process.
				   Should check status.
				   But since sync apparently succeeded, we dont care. */
				cb->syncPid = 0;
			}
		}

		coord = immnd_iAmCoordinator(cb);

		if (cb->pbePid > 0) {
			int status = 0;
			if (waitpid(cb->pbePid, &status, WNOHANG) > 0) {
				if (cb->mRim == SA_IMM_KEEP_REPOSITORY) {
					LOG_WA("Persistent back-end process has apparently died.");
				} else {
					LOG_NO("Persistent back-end process has terminated.");
				}
				cb->pbePid = 0;
				cb->mPbeKills = 0;
				osafassert(coord);
				if(!immModel_pbeIsInSync(cb, false)) {
					TRACE_5("Server-ready/coord invoking "
						"immnd_pbePrtoPurgeMutations");
					immnd_pbePrtoPurgeMutations(cb);
				}
			}
		}

		if(cb->preLoadPid > 0) {
			int status = 0;
			if (waitpid(cb->preLoadPid, &status, WNOHANG) > 0) {
				cb->preLoadPid = 0;
				LOG_NO("Local preLoader has terminated.");
			}
		}

		if (cb->pbePid2 > 0) {
			int status = 0;
			if (waitpid(cb->pbePid2, &status, WNOHANG) > 0) {
				if(coord) {
					LOG_NO("SLAVE PBE process terminated by coord.");
				} else if (cb->mRim == SA_IMM_KEEP_REPOSITORY) {
					LOG_WA("SLAVE PBE process has apparently died at non coord");
				} else {
					LOG_NO("SLAVE PBE process has been terminated");
				}
				cb->pbePid2 = 0;
				cb->mPbeKills = 0;
				cb->mPbeOldVeteranB = 0;
			}
		}

		if((cb->mStep == 0) || (cb->mCleanedHouseAt != jobDuration)) {
			immnd_cleanTheHouse(cb, coord == 1);
			cb->mCleanedHouseAt = jobDuration;
		}

		if ((coord == 1) && (cb->mStep > 1)) {
			if (immModel_immNotWritable(cb)) {
				/*Ooops we have apparently taken over the role of IMMND
				  coordinator during an uncompleted sync. Probably due 
				  to coordinator crash. Abort the sync. */
				LOG_WA("ABORTING UNCOMPLETED SYNC - COORDINATOR MUST HAVE CRASHED");
				immnd_abortSync(cb);
			} else if (jobDuration > 3) {
				newEpoch = immnd_syncNeeded(cb);
				if (newEpoch) {
					if (cb->syncPid > 0) {
						LOG_WA("Will not start new sync when previous "
							"sync process (%u) has not terminated", 
							cb->syncPid);
					} else {
						if (immnd_announceSync(cb, newEpoch)) {
							LOG_NO("SERVER STATE: IMM_SERVER_READY -->"
								" IMM_SERVER_SYNC_SERVER");
							cb->mState = IMM_SERVER_SYNC_SERVER;
							cb->mStep = 0;
							cb->mJobStart = now;
							*timeout = 100;	/* 0.1 sec */
						}
					}
				}
			}
			/* Independently of aborting or coordinating sync, 
			   check if we should be starting/stopping persistent back-end.*/
			if (cb->mPbeFile) {/* Pbe configured */
				if (cb->pbePid <= 0) { /* Pbe is NOT running */
					cb->mBlockPbeEnable = 0x0;
					if (cb->mRim == SA_IMM_KEEP_REPOSITORY) {/* Pbe SHOULD run. */
						if(immModel_pbeIsInSync(cb, false)) {
							if(cb->pbePid2 <= 0) {
								LOG_NO("STARTING PBE process.");
								cb->pbePid = immnd_forkPbe(cb);
							} else {
								LOG_WA("Can not start PBE, waiting for SLAVE PBE to stop");
								/* Primary PBE is only started by coord and remains there 
								   untill the coord crashes or is taken down. However,
								   the opposite is not true. The SLAVE PBE may transiently
								   be executing at the coord. This can be the case, if the
								   coord has crashed and immediately after the other SC immnd
								   has been elected coord. SLAVE PBE must then be stopped
								   at the new coord before the primary PBE can be started at
								   the new coord */
							}
						} else {
							/* ABT Probably remove this one. */
							TRACE_5("Sync-server/coord invoking "
								"immnd_pbePrtoPurgeMutations");
							immnd_pbePrtoPurgeMutations(cb);
						}
					}
				} else { /* Pbe is running. */
					osafassert(cb->pbePid > 0);
					if (cb->mRim == SA_IMM_INIT_FROM_FILE || cb->mBlockPbeEnable) {
						/* Pbe should NOT run.*/
						if((cb->mPbeKills++)==0) { /* Send SIGTERM only once.*/
							LOG_NO("STOPPING PBE process.");
							kill(cb->pbePid, SIGTERM);
						} else if(cb->mPbeKills > 20) {
							LOG_WA("PBE process appears hung, sending SIGKILL");
							kill(cb->pbePid, SIGKILL);
						}
					}
				}
			}

			if(cb->pbePid2 != 0) {
				/* SLAVE PBE should not be running at cord. Can happen after coord failover. */
				if((cb->mPbeKills++)==0) {
					LOG_WA("STOPPING SLAVE PBE process at IMMND COORD.");
					kill(cb->pbePid2, SIGTERM);
				} else if(cb->mPbeKills > 20) {
					LOG_WA("SLAVE PBE process appears hung, sending SIGKILL");
					kill(cb->pbePid2, SIGKILL);					
				}
			}
		} else if(coord != 1 && cb->mCanBeCoord) {/* => this is standby for coord immnd. */
			osafassert(cb->pbePid <= 0); /* Primary PBE must never have been started at non coord. */
			if (cb->mPbeFile && cb->m2Pbe) { /* 2PBE configured => impacts non coord sc immnd */
				if (cb->mRim == SA_IMM_KEEP_REPOSITORY) {
					/* PBE enabled => SLAVE PBE should run here */
					if(immModel_pbeIsInSync(cb, false) && immModel_pbeOiExists(cb)) {
						/* Primary PBE is available. */
						if(cb->pbePid2 <= 0) {
							/* SLAVE PBE is not running, try to start it. */
							LOG_NO("STARTING SLAVE PBE process.");
							cb->pbePid2 = immnd_forkPbe(cb);
						}
					} else if(cb->pbePid2 <= 0) {
						LOG_IN("Postponing start of SLAVE PBE until primary PBE has atached.");
						cb->mPbeVeteranB = SA_FALSE;
					}
				} else if(cb->pbePid2 > 0) { 
					/* PBE disabled, yet SLAVE PBE is running => STOP it. */
					if((cb->mPbeKills++)==0) {
						LOG_NO("STOPPING SLAVE PBE process at non coord because PBE is disabled");
						kill(cb->pbePid2, SIGTERM);
					} else if(cb->mPbeKills > 20) {
						LOG_WA("SLAVE PBE process appears hung, sending SIGKILL");
						kill(cb->pbePid2, SIGKILL);
					}
				}
			}
		}

		break;

	default:
		LOG_ER("Illegal state in ImmServer:%u", cb->mState);
		abort();
		break;
	}

	++(cb->mStep);

	/*TRACE_LEAVE(); */
	return rc;
}

/****************************************************************************
 * Name          : immnd_cb_dump
 *
 * Description   : This is the function dumps the contents of the control block. *
 * Arguments     : immnd_cb     -  Pointer to the control block
 *
 * Return Values : true/false
 *
 * Notes         : None.
 *****************************************************************************/
void immnd_cb_dump(void)
{
	IMMND_CB *cb = immnd_cb;
	IMMND_IMM_CLIENT_NODE *cl_node = NULL;

	TRACE_5("************ IMMND CB Details *************** ");

	TRACE_5("**** Global Cb Details ***************\n");
	TRACE_5("IMMND MDS dest - %x", m_NCS_NODE_ID_FROM_MDS_DEST(cb->immnd_mdest_id));
	if (cb->is_immd_up) {
		TRACE_5("IMMD is UP  ");
		TRACE_5("IMMD MDS dest - %x", m_NCS_NODE_ID_FROM_MDS_DEST(cb->immd_mdest_id));
	} else
		TRACE_5("IMMD is DOWN ");

	TRACE_5("***** Start of Client Details ***************\n");
	immnd_client_node_getnext(cb, 0, &cl_node);
	/*
	   while ( cl_node != NULL)
	   {
	   SaImmHandleT prev_cl_hdl;

	   immnd_dump_client_info(cl_node);

	   while(imm_list != NULL) 
	   {
	   immnd_dump_ckpt_info(imm_list->cnode);
	   imm_list=imm_list->next;
	   }
	   prev_cl_hdl = cl_node->imm_app_hdl;
	   immnd_client_node_getnext(cb,prev_cl_hdl,&cl_node);
	   }
	 */
	TRACE_5("***** End of Client Details ***************\n");
}

/****************************************************************************
 * Name          : immnd_dump_client_info
 *
 * Description   : This is the function dumps the Ckpt Client info
 * Arguments     : cl_node  -  Pointer to the IMMND_IMM_CLIENT_NODE
 *
 * Return Values : true/false
 *
 * Notes         : None.
 *****************************************************************************/
#if 0				/*Only for debug */
static
void immnd_dump_client_info(IMMND_IMM_CLIENT_NODE *cl_node)
{
	TRACE_5("++++++++++++++++++++++++++++++++++++++++++++++++++");

	TRACE_5("Client_hdl  %u\t MDS DEST %x ",
		(uint32_t)(cl_node->imm_app_hdl), (uint32_t)m_NCS_NODE_ID_FROM_MDS_DEST(cl_node->agent_mds_dest));
	TRACE_5("++++++++++++++++++++++++++++++++++++++++++++++++++");
}

#endif
