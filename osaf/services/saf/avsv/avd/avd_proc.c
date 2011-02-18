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

  DESCRIPTION: This module contains the array of function pointers, indexed by
  the events for both active and standby AvD. It also has the processing
  function that calls into one of these function pointers based on the event.
  
******************************************************************************
*/

#include <poll.h>
#include <logtrace.h>
#include <nid_api.h>
#include <avd.h>
#include <avd_imm.h>
#include <avd_cluster.h>

enum {
	FD_MBX = 0,
	FD_MBCSV,
	FD_CLM,
	FD_IMM
} AVD_FDS;

static nfds_t nfds = FD_IMM + 1;
static struct pollfd fds[FD_IMM + 1];

static void avd_process_event(AVD_CL_CB *cb_now, AVD_EVT *evt);
static void avd_invalid_evh(AVD_CL_CB *cb, AVD_EVT *evt);
static void avd_standby_invalid_evh(AVD_CL_CB *cb, AVD_EVT *evt);
static void avd_qsd_invalid_evh(AVD_CL_CB *cb, AVD_EVT *evt);
static void avd_qsd_ignore_evh(AVD_CL_CB *cb, AVD_EVT *evt);

/* list of all the function pointers related to handling the events
 * for active AvD.
 */
static const AVD_EVT_HDLR g_avd_actv_list[AVD_EVT_MAX] = {
	avd_invalid_evh,	/* AVD_EVT_INVALID */

	/* active AvD message events processing */
	avd_node_up_evh,	/* AVD_EVT_NODE_UP_MSG */
	avd_reg_su_evh,	        /* AVD_EVT_REG_SU_MSG */
	avd_reg_comp_evh,	/* AVD_EVT_REG_COMP_MSG */
	avd_su_oper_state_evh,	/* AVD_EVT_OPERATION_STATE_MSG */
	avd_su_si_assign_evh,	/* AVD_EVT_INFO_SU_SI_ASSIGN_MSG */
	avd_pg_trk_act_evh,	/* AVD_EVT_PG_TRACK_ACT_MSG */
	avd_oper_req_evh,	/* AVD_EVT_OPERATION_REQUEST_MSG */
	avd_data_update_req_evh,	/* AVD_EVT_DATA_REQUEST_MSG */
	avd_shutdown_app_su_resp_evh,	/* AVD_EVT_SHUTDOWN_APP_SU_MSG */
	avd_ack_nack_evh,	        /* AVD_EVT_VERIFY_ACK_NACK_MSG */
	avd_comp_validation_evh,	/* AVD_EVT_COMP_VALIDATION_MSG */

	/* active AvD timer events processing */
	avd_tmr_snd_hb_evh,       /* AVD_EVT_TMR_SND_HB */
	avd_cluster_tmr_init_evh, /* AVD_EVT_TMR_CL_INIT */
	avd_tmr_si_dep_tol_evh,   /* AVD_EVT_TMR_SI_DEP_TOL */

	/* active AvD MDS events processing */
	avd_mds_avd_up_evh,	/* AVD_EVT_MDS_AVD_UP */
	avd_mds_avd_down_evh,	/* AVD_EVT_MDS_AVD_DOWN */
	avd_mds_avnd_up_evh,	/* AVD_EVT_MDS_AVND_UP */
	avd_mds_avnd_down_evh,	/* AVD_EVT_MDS_AVND_DOWN */
	avd_mds_qsd_role_evh,	/* AVD_EVT_MDS_QSD_ACK */

	/* Role change Event processing */
	avd_role_change_evh,	     /* AVD_EVT_ROLE_CHANGE */
	avd_role_switch_ncs_su_evh,  /* AVD_EVT_SWITCH_NCS_SU */
	avd_process_si_dep_state_evh /* AVD_EVT_SI_DEP_STATE */
};

/* list of all the function pointers related to handling the events
 * for standby AvD.
 */

static const AVD_EVT_HDLR g_avd_stndby_list[AVD_EVT_MAX] = {
	avd_invalid_evh,	/* AVD_EVT_INVALID */

	/* standby AvD message events processing */
	avd_standby_invalid_evh,	/* AVD_EVT_NODE_UP_MSG */
	avd_standby_invalid_evh,	/* AVD_EVT_REG_SU_MSG */
	avd_standby_invalid_evh,	/* AVD_EVT_REG_COMP_MSG */
	avd_standby_invalid_evh,	/* AVD_EVT_OPERATION_STATE_MSG */
	avd_standby_invalid_evh,	/* AVD_EVT_INFO_SU_SI_ASSIGN_MSG */
	avd_standby_invalid_evh,	/* AVD_EVT_PG_TRACK_ACT_MSG */
	avd_standby_invalid_evh,	/* AVD_EVT_OPERATION_REQUEST_MSG */
	avd_standby_invalid_evh,	/* AVD_EVT_DATA_REQUEST_MSG */
	avd_standby_invalid_evh,	/* AVD_EVT_SHUTDOWN_APP_SU_MSG */
	avd_standby_invalid_evh,	/* AVD_EVT_VERIFY_ACK_NACK_MSG */
	avd_standby_invalid_evh,	/* AVD_EVT_COMP_VALIDATION_MSG */

	/* standby AvD timer events processing */
	avd_tmr_snd_hb_evh,           /* AVD_EVT_TMR_SND_HB */
	avd_standby_invalid_evh,      /* AVD_EVT_TMR_CL_INIT */
	avd_standby_invalid_evh,      /* AVD_EVT_TMR_SI_DEP_TOL */

	/* standby AvD MDS events processing */
	avd_mds_avd_up_evh,       /* AVD_EVT_MDS_AVD_UP */
	avd_standby_avd_down_evh, /* AVD_EVT_MDS_AVD_DOWN */
	avd_mds_avnd_up_evh,      /* AVD_EVT_MDS_AVND_UP */
	avd_mds_avnd_down_evh,    /* AVD_EVT_MDS_AVND_DOWN */
	avd_standby_invalid_evh,  /* AVD_EVT_MDS_QSD_ACK */

	/* Role change Event processing */
	avd_role_change_evh,	/* AVD_EVT_ROLE_CHANGE */
	avd_standby_invalid_evh,	/* AVD_EVT_SWITCH_NCS_SU */
	avd_standby_invalid_evh	/* AVD_EVT_SI_DEP_STATE */
};

/* list of all the function pointers related to handling the events
 * for quiesced AvD.
 */

static const AVD_EVT_HDLR g_avd_quiesc_list[AVD_EVT_MAX] = {
	/* invalid event */
	avd_invalid_evh,	/* AVD_EVT_INVALID */

	/* active AvD message events processing */
	avd_qsd_ignore_evh,	/* AVD_EVT_NODE_UP_MSG */
	avd_reg_su_evh,	/* AVD_EVT_REG_SU_MSG */
	avd_reg_comp_evh,	/* AVD_EVT_REG_COMP_MSG */
	avd_su_oper_state_evh,	/* AVD_EVT_OPERATION_STATE_MSG */
	avd_su_si_assign_evh,	/* AVD_EVT_INFO_SU_SI_ASSIGN_MSG */
	avd_qsd_ignore_evh,	/* AVD_EVT_PG_TRACK_ACT_MSG */
	avd_oper_req_evh,	/* AVD_EVT_OPERATION_REQUEST_MSG */
	avd_data_update_req_evh,	/* AVD_EVT_DATA_REQUEST_MSG */
	avd_shutdown_app_su_resp_evh,	/* AVD_EVT_SHUTDOWN_APP_SU_MSG */
	avd_qsd_invalid_evh,	/* AVD_EVT_VERIFY_ACK_NACK_MSG */
	avd_comp_validation_evh,	/* AVD_EVT_COMP_VALIDATION_MSG */

	/* active AvD timer events processing */
	avd_tmr_snd_hb_evh,     /* AVD_EVT_TMR_SND_HB */
	avd_qsd_ignore_evh,	/* AVD_EVT_TMR_CL_INIT */
	avd_qsd_ignore_evh,	/* AVD_EVT_TMR_SI_DEP_TOL */

	/* active AvD MDS events processing */
	avd_mds_avd_up_evh,	/* AVD_EVT_MDS_AVD_UP */
	avd_mds_avd_down_evh,	/* AVD_EVT_MDS_AVD_DOWN */
	avd_mds_avnd_up_evh,	/* AVD_EVT_MDS_AVND_UP */
	avd_mds_avnd_down_evh,	/* AVD_EVT_MDS_AVND_DOWN */
	avd_mds_qsd_role_evh,	/* AVD_EVT_MDS_QSD_ACK */

	/* Role change Event processing */
	avd_role_change_evh,	/* AVD_EVT_ROLE_CHANGE */
	avd_qsd_invalid_evh,	/* AVD_EVT_SWITCH_NCS_SU */
	avd_qsd_invalid_evh	/* AVD_EVT_TMR_SI_DEP_TOL */
};

/*****************************************************************************
 * Function: avd_invalid_evh
 *
 * Purpose:  This function is the handler for invalid events. It just
 * dumps the event to the debug log and returns.
 *
 * Input: cb - the AVD control block
 *        evt - The event information.
 *
 * Returns: None.
 *
 *
 * NOTES:
 *
 * 
 **************************************************************************/

static void avd_invalid_evh(AVD_CL_CB *cb, AVD_EVT *evt)
{
	/* This function should never be called
	 * log the event to the debug log and return
	 */

	/* we need not send sync update to stanby */
	cb->sync_required = FALSE;

	LOG_NO("avd_invalid_evh: %u", evt->rcv_evt);
}

/*****************************************************************************
 * Function: avd_standby_invalid_evh
 *
 * Purpose:  This function is the handler for invalid events in standby state.
 * This function might be called during system trauma. It just dumps the
 * event to the debug log at a information level and returns.
 *
 *
 * Input: cb - the AVD control block
 *        evt - The event information.
 *
 * Returns: None.
 *
 * NOTES:
 *
 * 
 **************************************************************************/

static void avd_standby_invalid_evh(AVD_CL_CB *cb, AVD_EVT *evt)
{
	/* This function should generally never be called
	 * log the event to the debug log at information level and return
	 */
	AVD_DND_MSG *n2d_msg;

	LOG_IN("avd_standby_invalid_evh: %u", evt->rcv_evt);

	if ((evt->rcv_evt >= AVD_EVT_NODE_UP_MSG) && (evt->rcv_evt <= AVD_EVT_VERIFY_ACK_NACK_MSG)) {
		if (evt->info.avnd_msg == NULL) {
			LOG_ER("avd_standby_invalid_evh: no msg");
			return;
		}

		n2d_msg = evt->info.avnd_msg;
		avsv_dnd_msg_free(n2d_msg);
		evt->info.avnd_msg = NULL;
	}
}

/*****************************************************************************
 * Function: avd_quiesced_invalid_evh
 *
 * Purpose:  This function is the handler for invalid events in quiesced state.
 * This function might be called during system trauma. It just dumps the
 * event to the debug log at a information level and returns.
 *
 *
 * Input: cb - the AVD control block
 *        evt - The event information.
 *
 * Returns: None.
 *
 * NOTES:
 *
 * 
 **************************************************************************/

static void avd_qsd_invalid_evh(AVD_CL_CB *cb, AVD_EVT *evt)
{
	/* This function should generally never be called
	 * log the event to the debug log at information level and return
	 */
	AVD_DND_MSG *n2d_msg;

	LOG_IN("avd_qsd_invalid_evh: %u", evt->rcv_evt);

	/* we need not send sync update to stanby */
	cb->sync_required = FALSE;

	if ((evt->rcv_evt >= AVD_EVT_NODE_UP_MSG) && (evt->rcv_evt <= AVD_EVT_VERIFY_ACK_NACK_MSG)) {
		if (evt->info.avnd_msg == NULL) {
			LOG_ER("avd_qsd_invalid_evh: no msg");
			return;
		}

		n2d_msg = evt->info.avnd_msg;
		avsv_dnd_msg_free(n2d_msg);
		evt->info.avnd_msg = NULL;
	}
}

/*****************************************************************************
 * Function: avd_qsd_ignore_evh 
 *
 * Purpose:  This function is the handler for events in quiesced state which
 * which are to be ignored.
 *
 *
 * Input: cb - the AVD control block
 *        evt - The event information.
 *
 * Returns: None.
 *
 * NOTES:
 *
 * 
 **************************************************************************/

static void avd_qsd_ignore_evh(AVD_CL_CB *cb, AVD_EVT *evt)
{
	/* Ignore this Event. Free this msg */
	AVD_DND_MSG *n2d_msg;

	LOG_IN("avd_qsd_ignore_evh: %u", evt->rcv_evt);

	/* we need not send sync update to stanby */
	cb->sync_required = FALSE;

	if ((evt->rcv_evt >= AVD_EVT_NODE_UP_MSG) && (evt->rcv_evt <= AVD_EVT_VERIFY_ACK_NACK_MSG)) {
		if (evt->info.avnd_msg == NULL) {
			LOG_ER("avd_qsd_ignore_evh: no msg");
			return;
		}

		n2d_msg = evt->info.avnd_msg;
		avsv_dnd_msg_free(n2d_msg);
		evt->info.avnd_msg = NULL;
	}
}

/*****************************************************************************
 * Function: avd_imm_reinit_thread
 * 
 * Purpose: This function waits a while on IMM and initialize, sel obj get &
 *          implementer set.
 *
 * Input: cb  - AVD control block
 * 
 * Returns: Void pointer.
 *
 * NOTES: None.
 *
 **************************************************************************/
static void *avd_imm_reinit_thread(void *_cb)
{
	SaAisErrorT rc = SA_AIS_OK;
	AVD_CL_CB *cb = (AVD_CL_CB *)_cb;

	/* If this is the active server, become implementer again. */
	if (cb->avail_state_avd == SA_AMF_HA_ACTIVE) {
		if ((rc = avd_imm_re_init(cb)) != SA_AIS_OK) {
			LOG_ER("re-saImmOiInitialize failed %u", rc);
			exit(1);
		}
		avd_imm_impl_set_task_create();
	}
	return NULL;
}

/*****************************************************************************
 * Function: avd_imm_reinit_bg
 *
 * Purpose: This function start a background thread to do IMM reinitialization
 *
 * Input: cb  - AVD control block
 *
 * Returns: None. 
 *
 * NOTES: None.
 *
 **************************************************************************/
static void avd_imm_reinit_bg(AVD_CL_CB *cb)
{
	pthread_t thread;
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

	TRACE_ENTER();
	if (pthread_create(&thread, NULL, avd_imm_reinit_thread, cb) != 0) {
		LOG_ER("pthread_create FAILED: %s", strerror(errno));
		exit(EXIT_FAILURE);
	}
	pthread_attr_destroy(&attr);
	TRACE_LEAVE();
}

/**
 * Calculate new poll timeout based on return value from job queue
 * execution
 * 
 * @param job_exec_result
 * 
 * @return int
 */
static int retval_to_polltmo(AvdJobDequeueResultT job_exec_result)
{
	int polltmo = -1;

	switch (job_exec_result) {
	case JOB_EXECUTED:
		polltmo = 0;   // Try again asap assuming there are more jobs waiting
		break;
	case JOB_ETRYAGAIN:
		polltmo = 500; // Try again soon
		break;
	case JOB_ENOTEXIST:
	case JOB_EINVH:
	case JOB_ERR:
		polltmo = -1;  // Infinite timeout
		break;
	default:
		assert(0);
		break;
	}

	return polltmo;
}

/**
 * Handle events during failover state
 * @param evt
 */
static void handle_event_in_failover_state(AVD_EVT *evt)
{
	AVD_CL_CB *cb = avd_cb;

	assert(cb->avd_fover_state == TRUE);

	TRACE_ENTER();

	/* 
	 * We are in fail-over, so process only 
	 * verify ack/nack and timer expiry events and
	 * MDS down events. Enqueue all other events.
	 */
	if ((evt->rcv_evt == AVD_EVT_VERIFY_ACK_NACK_MSG) ||
	    (evt->rcv_evt == AVD_EVT_MDS_AVND_DOWN) ||
	    (evt->rcv_evt == AVD_EVT_TMR_SND_HB)) {
		avd_process_event(cb, evt);
	} else {
		AVD_EVT_QUEUE *queue_evt;
		/* Enqueue this event */
		if (NULL != (queue_evt = calloc(1, sizeof(AVD_EVT_QUEUE)))) {
			queue_evt->evt = evt;
			m_AVD_EVT_QUEUE_ENQUEUE(cb, queue_evt);
		} else {
			LOG_ER("%s: calloc failed", __FUNCTION__);
			abort();
		}
	}

	if (cb->node_list.n_nodes == 0) {
		AVD_EVT_QUEUE *queue_evt;
		AVD_AVND *node;
		SaClmNodeIdT node_id = 0;

		/* We have received the info from all the nodes. */
		cb->avd_fover_state = FALSE;

		/* Dequeue, all the messages from the queue
		   and process them now */
		m_AVD_EVT_QUEUE_DEQUEUE(cb, queue_evt);

		while (NULL != queue_evt) {
			avd_process_event(cb, queue_evt->evt);
			free(queue_evt);
			m_AVD_EVT_QUEUE_DEQUEUE(cb, queue_evt);
		}

		/* Walk through all the nodes to check if any of the nodes state is
		 * AVD_AVND_STATE_ABSENT, if so means that node(payload) went down 
		 * during the failover state and the susi failover is differed till
		 * failover completes 
		 */
		while (NULL != (node = avd_node_getnext_nodeid(node_id))) {
			node_id = node->node_info.nodeId;

			if (AVD_AVND_STATE_ABSENT == node->node_state) {
				/* Fail over all SI assignments */
				avd_node_susi_fail_func(avd_cb, node);

				/* Remove the node from the node_id tree. */
				avd_node_delete_nodeid(node);
			}
		}
	}

	TRACE_LEAVE();
}

/*****************************************************************************
 * Function: avd_main_proc
 *
 * Purpose: This is the main infinite loop in which both the active and
 * standby AvDs execute waiting for events to happen. When woken up
 * due to an event, based on the HA state it moves to either the active
 * or standby processing modules. Even in Init state the same arrays are used.
 *
 * Input: cb - AVD control block
 *
 * Returns: NONE.
 *
 * NOTES: This function will never return execept in case of init errors.
 *
 * 
 **************************************************************************/
void avd_main_proc(void)
{
	AVD_CL_CB *cb = avd_cb;
	AVD_EVT *evt;
	NCS_SEL_OBJ mbx_fd;
	SaAisErrorT error = SA_AIS_OK;
	int polltmo = -1;

	if (avd_initialize() != NCSCC_RC_SUCCESS) {
		LOG_ER("main: avd_initialize FAILED, exiting...");
		(void) nid_notify("AMFD", NCSCC_RC_FAILURE, NULL);
		exit(EXIT_FAILURE);
	}

	mbx_fd = ncs_ipc_get_sel_obj(&cb->avd_mbx);

	fds[FD_MBX].fd = mbx_fd.rmv_obj;
	fds[FD_MBX].events = POLLIN;
	fds[FD_MBCSV].fd = cb->mbcsv_sel_obj;
	fds[FD_MBCSV].events = POLLIN;
	fds[FD_CLM].fd = cb->clm_sel_obj;
	fds[FD_CLM].events = POLLIN;
	fds[FD_IMM].fd = cb->imm_sel_obj;
	fds[FD_IMM].events = POLLIN;

	(void) nid_notify("AMFD", NCSCC_RC_SUCCESS, NULL);

	LOG_NO("Started");

	while (1) {
		
		if (cb->immOiHandle != 0) {
			fds[FD_IMM].fd = cb->imm_sel_obj;
			fds[FD_IMM].events = POLLIN;
			nfds = FD_IMM + 1;
		} else {
			nfds = FD_IMM;
		}
		
		int pollretval = poll(fds, nfds, polltmo);

		if (pollretval == -1) {
			if (errno == EINTR)
				continue;

			LOG_ER("main: poll FAILED - %s", strerror(errno));
			break;
		}

		if (pollretval == 0) {
			// poll time out, submit some jobs (if any)
			polltmo = retval_to_polltmo(avd_job_fifo_execute(cb->immOiHandle));
			continue;
		}

		if (fds[FD_MBX].revents & POLLIN) {
			evt = (AVD_EVT *)ncs_ipc_non_blk_recv(&cb->avd_mbx);

			if (evt == NULL) {
				LOG_ER("main: NULL evt");
				continue;
			}

			if ((AVD_EVT_INVALID <= evt->rcv_evt) && (evt->rcv_evt >= AVD_EVT_MAX)) {
				LOG_ER("main: invalid evt %u", evt->rcv_evt);
				continue;
			}

			if (cb->avd_fover_state) {
				handle_event_in_failover_state(evt);
			} else if (FALSE == cb->avd_fover_state) {
				avd_process_event(cb, evt);
			} else
				assert(0);
		}

		if (fds[FD_MBCSV].revents & POLLIN) {
			if (NCSCC_RC_SUCCESS != avsv_mbcsv_dispatch(cb, SA_DISPATCH_ALL))
				LOG_ER("avsv_mbcsv_dispatch FAILED");
		}

		if (fds[FD_CLM].revents & POLLIN) {
			TRACE("CLM event rec");
			error = saClmDispatch(cb->clmHandle, SA_DISPATCH_ALL);

			if (error != SA_AIS_OK)
				LOG_ER("main: saClmDispatch FAILED %u", error);

			/* commit async updated possibly sent in the callback */
			m_AVSV_SEND_CKPT_UPDT_SYNC(cb, NCS_MBCSV_ACT_UPDATE, 0);

			/* flush messages possibly queued in the callback */
			avd_d2n_msg_dequeue(cb);
		}

		if (cb->immOiHandle && fds[FD_IMM].revents & POLLIN) {
			TRACE("IMM event rec");
			error = saImmOiDispatch(cb->immOiHandle, SA_DISPATCH_ALL);

			/*
			 ** BAD_HANDLE is interpreted as an IMM service restart. Try
			 ** reinitialize the IMM OI API in a background thread and let
			 ** this thread do business as usual especially handling write
			 ** requests.
			 **
			 ** All other errors are treated as non-recoverable (fatal) and will
			 ** cause an exit of the process.
			 */
			if (error == SA_AIS_ERR_BAD_HANDLE) {
				TRACE("main: saImmOiDispatch returned BAD_HANDLE");

				/*
				 ** Invalidate the IMM OI handle, this info is used in other
				 ** locations. E.g. giving TRY_AGAIN responses to a create and
				 ** close app stream requests. That is needed since the IMM OI
				 ** is used in context of these functions.
				 */
				saImmOiFinalize(cb->immOiHandle);
				cb->immOiHandle = 0;

				/* Initiate IMM reinitializtion in the background */
				avd_imm_reinit_bg(cb);
			} else if (error != SA_AIS_OK) {
				LOG_ER("main: saImmOiDispatch FAILED %u", error);
				break;
			}
			else {
				/* commit async updated possibly sent in the callback */
				m_AVSV_SEND_CKPT_UPDT_SYNC(cb, NCS_MBCSV_ACT_UPDATE, 0);
				/* flush messages possibly queued in the callback */
				avd_d2n_msg_dequeue(cb);
			}
		}		/* End of if (cb->immOiHandle && fds[FD_IMM].revents & POLLIN)  */

		// submit some jobs (if any)
		polltmo = retval_to_polltmo(avd_job_fifo_execute(cb->immOiHandle));
	}

	syslog(LOG_CRIT, "AVD Thread Failed");
	sleep(3);		/* Allow logs to be printed */
	exit(EXIT_FAILURE);
}

/*****************************************************************************
 * Function: avd_process_event 
 *
 * Purpose: This function executes the event handler for the current AVD
 *           state. This function will be used in the main AVD thread.
 *
 * Input: cb  - AVD control block
 *        evt - ptr to AVD_EVT got from mailbox 
 *
 * Returns: NONE.
 *
 * NOTES: None.
 *
 * 
 **************************************************************************/

static void avd_process_event(AVD_CL_CB *cb_now, AVD_EVT *evt)
{
	/* check the HA state */
	if (cb_now->avail_state_avd == SA_AMF_HA_ACTIVE) {
		/* if active call g_avd_actv_list functions */
		g_avd_actv_list[evt->rcv_evt] (cb_now, evt);

		/*
		 * Just processed the event.
		 * Time to send sync send the standby and then
		 * all the AVND messages we have queued in our queue.
		 */
		if (cb_now->sync_required == TRUE)
			m_AVSV_SEND_CKPT_UPDT_SYNC(cb_now, NCS_MBCSV_ACT_UPDATE, 0);

		avd_d2n_msg_dequeue(cb_now);
	} else if (cb_now->avail_state_avd == SA_AMF_HA_STANDBY) {
		/* if standby call g_avd_stndby_list functions */
		g_avd_stndby_list[evt->rcv_evt] (cb_now, evt);

		/* Now it might have become standby to active in avd_role_change_evh() during switchover, 
		   so just sent updates to quisced */
                if ((cb_now->avail_state_avd == SA_AMF_HA_ACTIVE) && (cb_now->sync_required == TRUE))
                        m_AVSV_SEND_CKPT_UPDT_SYNC(cb_now, NCS_MBCSV_ACT_UPDATE, 0);

		avd_d2n_msg_dequeue(cb_now);
	} else if (cb_now->avail_state_avd == SA_AMF_HA_QUIESCED) {
		/* if quiesced call g_avd_quiesc_list functions */
		g_avd_quiesc_list[evt->rcv_evt] (cb_now, evt);
		/*
		 * Just processed the event.
		 * Time to send sync send the standby and then
		 * all the AVND messages we have queued in our queue.
		 */
		if (cb_now->sync_required == TRUE)
			m_AVSV_SEND_CKPT_UPDT_SYNC(cb_now, NCS_MBCSV_ACT_UPDATE, 0);

		avd_d2n_msg_dequeue(cb_now);

	} else
		assert(0);

	/* reset the sync falg */
	cb_now->sync_required = TRUE;

	free(evt);
}

