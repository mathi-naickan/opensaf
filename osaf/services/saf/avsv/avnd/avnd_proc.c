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

  DESCRIPTION:This module contains the array of function pointers, indexed by
  the events for the AvND. It also has the processing
  function that calls into one of these function pointers based on the event.

 
..............................................................................

  FUNCTIONS INCLUDED in this module:

  
******************************************************************************
*/

/*
 * Module Inclusion Control...
 */

#include <poll.h>
#include <sched.h>

#include <logtrace.h>

#include <nid_api.h>
#include "avnd.h"
#include "avnd_mon.h"

#define FD_MBX   0
#define FD_TERM  1
#define FD_CLM   2 
#define FD_MBCSV 3

static NCS_SEL_OBJ term_sel_obj; /* Selection object for TERM signal events */

static void avnd_evt_process(AVND_EVT *);

static uns32 avnd_evt_invalid_evh(AVND_CB *cb, AVND_EVT *evt);

/* list of all the function pointers related to handling the events
*/
const AVND_EVT_HDLR g_avnd_func_list[AVND_EVT_MAX] = {
	avnd_evt_invalid_evh,	/* AVND_EVT_INVALID */

	/* AvD message event types */
	avnd_evt_avd_node_up_evh,	/* AVND_EVT_AVD_NODE_UP_MSG */
	avnd_evt_avd_reg_su_evh,	/* AVND_EVT_AVD_REG_SU_MSG */
	avnd_evt_avd_reg_comp_evh,	/* AVND_EVT_AVD_REG_COMP_MSG */
	avnd_evt_avd_info_su_si_assign_evh,	/* AVND_EVT_AVD_INFO_SU_SI_ASSIGN_MSG */
	avnd_evt_avd_pg_track_act_rsp_evh,	/* AVND_EVT_AVD_PG_TRACK_ACT_RSP_MSG */
	avnd_evt_avd_pg_upd_evh,	/* AVND_EVT_AVD_PG_UPD_MSG */
	avnd_evt_avd_operation_request_evh,	/* AVND_EVT_AVD_OPERATION_REQUEST_MSG */
	avnd_evt_avd_su_pres_evh,	/* AVND_EVT_AVD_SU_PRES_MSG */
	avnd_evt_avd_verify_evh,	/* AVND_EVT_AVD_VERIFY_MSG */
	avnd_evt_avd_ack_evh,	/* AVND_EVT_AVD_ACK_MSG */
	avnd_evt_avd_shutdown_app_su_evh,	/* AVND_EVT_AVD_SHUTDOWN_APP_SU_MSG */
	avnd_evt_avd_set_leds_evh,	/* AVND_EVT_AVD_SET_LEDS_MSG */
	avnd_evt_avd_comp_validation_resp_evh,	/* AVND_EVT_AVD_COMP_VALIDATION_RESP_MSG */
	avnd_evt_avd_role_change_evh,	/* AVND_EVT_AVD_ROLE_CHANGE_MSG */
	avnd_evt_avd_admin_op_req_evh,  /* AVND_EVT_AVD_ADMIN_OP_REQ_MSG */
	avnd_evt_avd_hb_evh,            /* AVND_EVT_AVD_HEARTBEAT_MSG */
	avnd_evt_avd_reboot_evh,            /* /AVND_EVT_AVD_REBOOT_MSG */

	/* AvA event types */
	avnd_evt_ava_finalize_evh,	/* AVND_EVT_AVA_AMF_FINALIZE */
	avnd_evt_ava_comp_reg_evh,	/* AVND_EVT_AVA_COMP_REG */
	avnd_evt_ava_comp_unreg_evh,	/* AVND_EVT_AVA_COMP_UNREG */
	avnd_evt_ava_pm_start_evh,	/* AVND_EVT_AVA_PM_START */
	avnd_evt_ava_pm_stop_evh,	/* AVND_EVT_AVA_PM_STOP */
	avnd_evt_ava_hc_start_evh,	/* AVND_EVT_AVA_HC_START */
	avnd_evt_ava_hc_stop_evh,	/* AVND_EVT_AVA_HC_STOP */
	avnd_evt_ava_hc_confirm_evh,	/* AVND_EVT_AVA_HC_CONFIRM */
	avnd_evt_ava_csi_quiescing_compl_evh,	/* AVND_EVT_AVA_CSI_QUIESCING_COMPL */
	avnd_evt_ava_ha_get_evh,	/* AVND_EVT_AVA_HA_GET */
	avnd_evt_ava_pg_start_evh,	/* AVND_EVT_AVA_PG_START */
	avnd_evt_ava_pg_stop_evh,	/* AVND_EVT_AVA_PG_STOP */
	avnd_evt_ava_err_rep_evh,	/* AVND_EVT_AVA_ERR_REP */
	avnd_evt_ava_err_clear_evh,	/* AVND_EVT_AVA_ERR_CLEAR */
	avnd_evt_ava_resp_evh,	/* AVND_EVT_AVA_RESP */

	/* timer event types */
	avnd_evt_tmr_hc_evh,	/* AVND_EVT_TMR_HC */
	avnd_evt_tmr_cbk_resp_evh,	/* AVND_EVT_TMR_CBK_RESP */
	avnd_evt_tmr_rcv_msg_rsp_evh,	/* AVND_EVT_TMR_RCV_MSG_RSP */
	avnd_evt_tmr_clc_comp_reg_evh,	/* AVND_EVT_TMR_CLC_COMP_REG */
	avnd_evt_tmr_su_err_esc_evh,	/* AVND_EVT_TMR_SU_ERR_ESC */
	avnd_evt_tmr_node_err_esc_evh,	/* AVND_EVT_TMR_NODE_ERR_ESC */
	avnd_evt_tmr_clc_pxied_comp_inst_evh,	/* AVND_EVT_TMR_CLC_PXIED_COMP_INST */
	avnd_evt_tmr_clc_pxied_comp_reg_evh,	/* AVND_EVT_TMR_CLC_PXIED_COMP_REG */
	avnd_evt_tmr_avd_hb_duration_evh,

	/* mds event types */
	avnd_evt_mds_avd_up_evh,	/* AVND_EVT_MDS_AVD_UP */
	avnd_evt_mds_avd_dn_evh,	/* AVND_EVT_MDS_AVD_DN */
	avnd_evt_mds_ava_dn_evh,	/* AVND_EVT_MDS_AVA_DN */
	avnd_evt_mds_avnd_dn_evh,	/* AVND_EVT_MDS_AVND_DN */
	avnd_evt_mds_avnd_up_evh,	/* AVND_EVT_MDS_AVND_UP */

	/* HA State Change event types */
	avnd_evt_ha_state_change_evh,	/* AVND_EVT_HA_STATE_CHANGE */

	/* clc event types */
	avnd_evt_clc_resp_evh,	/* AVND_EVT_CLC_RESP */

	/* AvND to AvND Events. */
	avnd_evt_avnd_avnd_evh,	/* AVND_EVT_AVND_AVND_MSG */

	/* internal event types */
	avnd_evt_comp_pres_fsm_evh,	/* AVND_EVT_COMP_PRES_FSM_EV */
	avnd_evt_last_step_term_evh,	/* AVND_EVT_LAST_STEP_TERM */
	avnd_evt_pid_exit_evh,	/* AVND_EVT_PID_EXIT */
	avnd_evt_tmr_qscing_cmpl_evh	/* AVND_EVT_TMR_QSCING_CMPL */
};

/**
 * TERM signal is used when user wants to stop OpenSAF (AMF).
 * 
 * @param sig
 */
static void sigterm_handler(int sig)
{
	ncs_sel_obj_ind(term_sel_obj);
	signal(SIGTERM, SIG_IGN);
}

/****************************************************************************
  Name          : avnd_main_process
 
  Description   : This routine is an entry point for the AvND task.
 
  Arguments     : -
 
  Return Values : None.
 
  Notes         : None
******************************************************************************/
void avnd_main_process(void)
{
	NCS_SEL_OBJ mbx_fd;
	struct pollfd fds[4];
	nfds_t nfds = 3;
	AVND_EVT *evt;

	TRACE_ENTER();

	if (avnd_create() != NCSCC_RC_SUCCESS) {
		syslog(LOG_ERR, "avnd_create");
		goto done;
	}

	/* Change scheduling class to real time. */
	{
		struct sched_param param = {.sched_priority = sched_get_priority_min(SCHED_RR) };

		if (sched_setscheduler(0, SCHED_RR, &param) == -1)
			syslog(LOG_ERR, "Could not set scheduling class for avnd: %s",
				strerror(errno));
	}

	if (ncs_sel_obj_create(&term_sel_obj) != NCSCC_RC_SUCCESS) {
		LOG_ER("ncs_sel_obj_create failed");
		goto done;
	}

	if (signal(SIGTERM, sigterm_handler) == SIG_ERR) {
		LOG_ER("signal TERM failed: %s", strerror(errno));
		goto done;
	}

	mbx_fd = ncs_ipc_get_sel_obj(&avnd_cb->mbx);
	fds[FD_MBX].fd = mbx_fd.rmv_obj;
	fds[FD_MBX].events = POLLIN;

	fds[FD_TERM].fd = term_sel_obj.rmv_obj;
	fds[FD_TERM].events = POLLIN;

	fds[FD_CLM].fd = avnd_cb->clm_sel_obj;
	fds[FD_CLM].events = POLLIN;
         
#if FIXME
	if (avnd_cb->type == AVSV_AVND_CARD_SYS_CON) {
		fds[FD_MBCSV].fd = avnd_cb->avnd_mbcsv_sel_obj;
		fds[FD_MBCSV].events = POLLIN;
		nfds++;
	}
#endif
	(void) nid_notify("AMFND", NCSCC_RC_SUCCESS, NULL);

	LOG_NO("Started");

	/* now wait forever */
	while (1) {
		int ret = poll(fds, nfds, -1);

		if (ret == -1) {
			if (errno == EINTR) {
				continue;
			}

			syslog(LOG_ERR, "%s: poll failed - %s", __FUNCTION__, strerror(errno));
			break;
		}

		if (fds[FD_CLM].revents & POLLIN) {
			TRACE("CLM event recieved");
			saClmDispatch(avnd_cb->clmHandle, SA_DISPATCH_ALL);
		}

		if (fds[FD_MBX].revents & POLLIN) {
			while (NULL != (evt = (AVND_EVT *)ncs_ipc_non_blk_recv(&avnd_cb->mbx)))
				avnd_evt_process(evt);
		}

		if (fds[FD_TERM].revents & POLLIN) {
			ncs_sel_obj_rmv_ind(term_sel_obj, TRUE, TRUE);
			avnd_sigterm_handler();
		}

#if FIXME
		if ((avnd_cb->type == AVSV_AVND_CARD_SYS_CON) &&
		    (fds[FD_MBCSV].revents & POLLIN)) {
			if (NCSCC_RC_SUCCESS != avnd_mbcsv_dispatch(avnd_cb, SA_DISPATCH_ALL)) {
				; /* continue with our loop. */
			}
		}
#endif
	}

done:
	syslog(LOG_NOTICE, "exiting");
	exit(0);
}

/****************************************************************************
  Name          : avnd_evt_process
 
  Description   : This routine processes the AvND event.
 
  Arguments     : mbx - ptr to the mailbox
 
  Return Values : None.
 
  Notes         : None
******************************************************************************/
void avnd_evt_process(AVND_EVT *evt)
{
	AVND_CB *cb = avnd_cb;
	uns32 rc = NCSCC_RC_SUCCESS;

	/* validate the event type */
	if ((evt->type <= AVND_EVT_INVALID) || (evt->type >= AVND_EVT_MAX)) {
		LOG_ER("Unknown event %u", evt->type);
		goto done;
	}

	/* Temp: AvD Down Handling */
	if (TRUE == cb->is_avd_down)
		goto done;

	/* log the event reception */
	m_AVND_LOG_EVT(evt->type, 0, 0, NCSFL_SEV_INFO);

	/* acquire cb write lock */
	m_NCS_LOCK(&cb->lock, NCS_LOCK_WRITE);

	/* invoke the event handler */
	rc = g_avnd_func_list[evt->type] (cb, evt);

	if ((SA_AMF_HA_ACTIVE == cb->avail_state_avnd) || (SA_AMF_HA_QUIESCED == cb->avail_state_avnd)) {
		m_AVND_SEND_CKPT_UPDT_SYNC(cb, NCS_MBCSV_ACT_UPDATE, 0);
	}

	/* release cb write lock */
	m_NCS_UNLOCK(&cb->lock, NCS_LOCK_WRITE);

	/* log the result of event processing */
	m_AVND_LOG_EVT(evt->type, 0,
		       (rc == NCSCC_RC_SUCCESS) ? AVND_LOG_EVT_SUCCESS : AVND_LOG_EVT_FAILURE, NCSFL_SEV_INFO);

done:
	if (evt)
		avnd_evt_destroy(evt);
}

/*****************************************************************************
 * Function: avnd_evt_invalid_evh
 *
 * Purpose:  This function is the handler for invalid events. It just
 * dumps the event to the debug log and returns.
 *
 * Input: 
 *
 * Returns: 
 *
 * NOTES:
 *
 * 
 **************************************************************************/

static uns32 avnd_evt_invalid_evh(AVND_CB *cb, AVND_EVT *evt)
{
	LOG_NO("avnd_evt_invalid_func: %u", evt->type);
	return NCSCC_RC_SUCCESS;
}

