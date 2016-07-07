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
 * Author(s): Ericsson AB
 *            Wind River Systems
 *
 */

#include <new>
#include <poll.h>
#include <sched.h>
#include <stdlib.h>

#include "avnd.h"
#include "amf_d2nedu.h"
#include "amf_n2avaedu.h"
#include "amf_nd2ndmsg.h"
#include "avnd_mon.h"
#include "configmake.h"
#include "daemon.h"
#include "immutil.h"
#include "logtrace.h"
#include "nid_api.h"
#include <imm.h>

#define FD_MBX   0
#define FD_TERM  1
#define FD_CLM   2 
#define FD_MBCSV 3

static const char* internal_version_id_  __attribute__ ((used)) = "@(#) $Id: " INTERNAL_VERSION_ID " $";

static NCS_SEL_OBJ term_sel_obj; /* Selection object for TERM signal events */

static void avnd_evt_process(AVND_EVT *);

static uint32_t avnd_evt_invalid_evh(AVND_CB *cb, AVND_EVT *evt);

/* list of all the function pointers related to handling the events
*/
extern const AVND_EVT_HDLR g_avnd_func_list[AVND_EVT_MAX] = {
	avnd_evt_invalid_evh,	/* AVND_EVT_INVALID */

	/* AvD message event types */
	avnd_evt_avd_node_up_evh,	/* AVND_EVT_AVD_NODE_UP_MSG */
	avnd_evt_avd_reg_su_evh,	/* AVND_EVT_AVD_REG_SU_MSG */
	avnd_evt_invalid_evh,	/* AVND_EVT_AVD_REG_COMP_MSG */
	avnd_evt_avd_info_su_si_assign_evh,	/* AVND_EVT_AVD_INFO_SU_SI_ASSIGN_MSG */
	avnd_evt_avd_pg_track_act_rsp_evh,	/* AVND_EVT_AVD_PG_TRACK_ACT_RSP_MSG */
	avnd_evt_avd_pg_upd_evh,	/* AVND_EVT_AVD_PG_UPD_MSG */
	avnd_evt_avd_operation_request_evh,	/* AVND_EVT_AVD_OPERATION_REQUEST_MSG */
	avnd_evt_avd_su_pres_evh,	/* AVND_EVT_AVD_SU_PRES_MSG */
	avnd_evt_avd_verify_evh,	/* AVND_EVT_AVD_VERIFY_MSG */
	avnd_evt_avd_ack_evh,	/* AVND_EVT_AVD_ACK_MSG */
	avnd_evt_invalid_evh,	/* AVND_EVT_AVD_SHUTDOWN_APP_SU_MSG */
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
	avnd_evt_tmr_qscing_cmpl_evh,	/* AVND_EVT_TMR_QSCING_CMPL */
	avnd_evt_ir_evh	/* AVND_EVT_IR */
};

/* global task handle */
NCSCONTEXT gl_avnd_task_hdl = 0;

/* control block and global pointer to it  */
static AVND_CB _avnd_cb;
AVND_CB *avnd_cb = &_avnd_cb;

/* static function declarations */

static AVND_CB *avnd_cb_create(void);

static uint32_t avnd_mbx_create(AVND_CB *);

static uint32_t avnd_ext_intf_create(AVND_CB *);


static int __init_avnd(void)
{
	if (ncs_agents_startup() != NCSCC_RC_SUCCESS)
		return m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);

	return (NCSCC_RC_SUCCESS);
}

// c++ new handler will be called if new fails.
static void new_handler()
{
        syslog(LOG_ERR, "new failed, calling abort");
        abort();
}
 
int main(int argc, char *argv[])
{
	uint32_t error;

        // function to be called if new fails. The alternative of using catch of std::bad_alloc
        // will unwind the stack and thus no call chain will be available.
        std::set_new_handler(new_handler);

	if (avnd_failed_state_file_exist()) {
		syslog(LOG_ERR, "system is in repair pending state, reboot or "
				"manual cleanup needed");
		syslog(LOG_ERR, "cleanup components manually and delete %s",
				avnd_failed_state_file_location());
		goto done;
	}

	daemonize_as_user("root", argc, argv);

	if (__init_avnd() != NCSCC_RC_SUCCESS) {
		syslog(LOG_ERR, "__init_avd() failed");
		goto done;
	}

	/* should never return */
	avnd_main_process();

	opensaf_reboot(avnd_cb->node_info.nodeId, (char *)avnd_cb->node_info.executionEnvironment.value,
			"avnd_main_proc exited");

	exit(1);

done:
	(void) nid_notify(const_cast<char*>("AMFND"), NCSCC_RC_FAILURE, &error);
	syslog(LOG_ERR, "main() failed, exiting");
	exit(1);
}

static int get_node_type(void)
{
        size_t bytes;
        int type;
        char buf[32];
        FILE *f = fopen(PKGSYSCONFDIR "/node_type", "r");

        if (!f) {
                LOG_ER("Could not open file %s - %s", PKGSYSCONFDIR "/node_type", strerror(errno));
                return AVSV_AVND_CARD_PAYLOAD;
        }

        if ((bytes = fscanf(f, "%s", buf)) > 0) {
                if (strncmp(buf, "controller", sizeof(buf)) == 0) {
			TRACE("Node type: controller");
                        type = AVSV_AVND_CARD_SYS_CON;
		}
                else if (strncmp(buf, "payload", sizeof(buf)) == 0) {
			TRACE("Node type: payload");
                        type = AVSV_AVND_CARD_PAYLOAD;
		}
                else {
                        LOG_ER("Unknown node type %s", buf);
                        type = AVSV_AVND_CARD_PAYLOAD;
                }
        } else {
		LOG_ER("fscanf FAILED for %s - %s", PKGSYSCONFDIR "/node_type", strerror(errno));
                type = AVSV_AVND_CARD_PAYLOAD;
        }

        (void)fclose(f);

	return type;
}


/****************************************************************************
  Name          : avnd_create
 
  Description   : This routine creates & initializes the PWE of AvND. It does
                  the following:
                  a) create & initialize AvND control block.
                  b) create & attach AvND mailbox.
                  c) initialize external interfaces (logging service being the
                     exception).
 
  Arguments     : -
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None
******************************************************************************/
uint32_t avnd_create(void)
{
	AVND_CB *cb = 0;
	uint32_t rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER();

	/* create & initialize AvND cb */
	cb = avnd_cb_create();
	if (!cb) {
		rc = NCSCC_RC_FAILURE;
		goto done;
	}

	/* create & attach AvND mailbox */
	rc = avnd_mbx_create(cb);
	if (NCSCC_RC_SUCCESS != rc) {
		rc = NCSCC_RC_FAILURE;
		goto done;
	}

	/* initialize external interfaces */
	rc = avnd_ext_intf_create(cb);
	if (NCSCC_RC_SUCCESS != rc) {
		rc = NCSCC_RC_FAILURE;
		goto done;
	}

	/* initialize external interfaces */
	rc = avnd_clm_init();
	if (SA_AIS_OK != rc) {
		rc = NCSCC_RC_FAILURE;
		goto done;
	}

done:
	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
  Name          : avnd_cb_create
 
  Description   : This routine creates & initializes AvND control block.
 
  Arguments     : None.
 
  Return Values : if successfull, ptr to AvND control block
                  else, 0
 
  Notes         : None
******************************************************************************/
AVND_CB *avnd_cb_create()
{
	AVND_CB *cb = avnd_cb;
	char *val;

	TRACE_ENTER();

	/* assign the AvND pool-id (used by hdl-mngr) */
	cb->pool_id = NCS_HM_POOL_ID_COMMON;

	/* assign the default states */
	cb->admin_state = SA_AMF_ADMIN_UNLOCKED;
	cb->oper_state = SA_AMF_OPERATIONAL_ENABLED;
	cb->term_state = AVND_TERM_STATE_UP;
	cb->led_state = AVND_LED_STATE_RED;
	cb->stby_sync_state = AVND_STBY_IN_SYNC;

	/* assign the default timeout values (in nsec) */
	cb->msg_resp_intv = AVND_AVD_MSG_RESP_TIME * 1000000;

	cb->hb_duration_tmr.is_active = false;
	cb->hb_duration_tmr.type = AVND_TMR_HB_DURATION;
	cb->hb_duration = AVSV_DEF_HB_DURATION;

	if ((val = getenv("AVSV_HB_DURATION")) != NULL) {
		cb->hb_duration = strtoll(val, NULL, 0);
		if (cb->hb_duration == 0) {
			/* no value or non convertable value, revert to default */
			cb->hb_duration = AVSV_DEF_HB_DURATION;
		}
	}

	/* initialize the PID monitor lock */
	m_NCS_LOCK_INIT(&cb->mon_lock);

	/* iniialize the error escaltion paramaets */
	cb->node_err_esc_level = AVND_ERR_ESC_LEVEL_0;

	/*** initialize avnd dbs ***/

	avnd_silist_init(cb);

	/* initialize su db */
	if (NCSCC_RC_SUCCESS != avnd_sudb_init(cb))
		goto err;

	/* initialize comp db */
	if (NCSCC_RC_SUCCESS != avnd_compdb_init(cb))
		goto err;

	/* initialize healthcheck db */
	avnd_hcdb_init(cb);

	avnd_cb->type = static_cast<AVSV_AVND_CARD>(get_node_type());

	/* initialize pg db */
	if (NCSCC_RC_SUCCESS != avnd_pgdb_init(cb))
		goto err;

	/* initialize pid_mon list */
	avnd_pid_mon_list_init(cb);

	/* initialize nodeid to mdsdest mapping db */
	if (NCSCC_RC_SUCCESS != avnd_nodeid_to_mdsdest_map_db_init(cb))
		goto err;

	/* initialize available internode components db */
	if (NCSCC_RC_SUCCESS != avnd_internode_avail_comp_db_init(cb))
		goto err;

	TRACE_LEAVE();
	return cb;

 err:
	TRACE_LEAVE();
	return 0;
}

/****************************************************************************
  Name          : avnd_mbx_create
 
  Description   : This routine creates & attaches AvND mailbox.
 
  Arguments     : cb - ptr to AvND control block
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None
******************************************************************************/
uint32_t avnd_mbx_create(AVND_CB *cb)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER();

	/* create the mail box */
	rc = m_NCS_IPC_CREATE(&cb->mbx);
	if (NCSCC_RC_SUCCESS != rc) {
		LOG_CR("AvND Mailbox creation failed");
		goto err;
	}
	TRACE("AvND mailbox creation success");

	/* attach the mail box */
	rc = m_NCS_IPC_ATTACH(&cb->mbx);
	if (NCSCC_RC_SUCCESS != rc) {
		LOG_CR("AvND mailbox attach failed");
		goto err;
	}
	TRACE_LEAVE2("AvND mailbox attach success");

	return rc;

 err:
	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
  Name          : avnd_ext_intf_create
 
  Description   : This routine initialize external interfaces (logging 
                  service being the exception).
 
  Arguments     : cb - ptr to AvND control block
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None
******************************************************************************/
uint32_t avnd_ext_intf_create(AVND_CB *cb)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	EDU_ERR err = EDU_NORMAL;
	TRACE_ENTER();

	/* EDU initialisation */
	m_NCS_EDU_HDL_INIT(&cb->edu_hdl);

	rc = m_NCS_EDU_COMPILE_EDP(&cb->edu_hdl, avsv_edp_dnd_msg, &err);

	if (rc != NCSCC_RC_SUCCESS) {
		LOG_ER("%u, EDU compilation failed",__LINE__);
		goto err;
	}

	m_NCS_EDU_HDL_INIT(&cb->edu_hdl_avnd);

	rc = m_NCS_EDU_COMPILE_EDP(&cb->edu_hdl_avnd, avsv_edp_ndnd_msg, &err);

	if (rc != NCSCC_RC_SUCCESS) {
		LOG_ER("%u, EDU compilation failed",__LINE__);
		goto err;
	}

	m_NCS_EDU_HDL_INIT(&cb->edu_hdl_ava);

	rc = m_NCS_EDU_COMPILE_EDP(&cb->edu_hdl_ava, avsv_edp_nda_msg, &err);

	if (rc != NCSCC_RC_SUCCESS) {
		LOG_ER("%u, EDU compilation failed",__LINE__);
		goto err;
	}

	TRACE("EDU Init success");

	/* MDS registration */
	rc = avnd_mds_reg(cb);
	if (NCSCC_RC_SUCCESS != rc) {
		LOG_CR("MDS registration failed");
		goto err;
	}
	TRACE("MDS registration success");
#if FIXME
	if (cb->type == AVSV_AVND_CARD_SYS_CON) {
		rc = avnd_mds_mbcsv_reg(cb);
		if (NCSCC_RC_SUCCESS != rc) {
			LOG_CR("MBCSv registration failed");
			goto err;
		}
	}
#endif
	TRACE_LEAVE();
	return rc;

 err:
	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
   Name          : avnd_sigusr1_handler
  
   Description   : This routine handles the TERM signal sent by the start/stop script.
                   This routine posts the message to mailbox to clean 
                   all the NCS components also. This is the signal to perform 
                   the last step of termination including db clean-up.

   Arguments     : 
  
   Return Values : true/false
  
   Notes         : None.
 *****************************************************************************/
void avnd_sigterm_handler(void)
{
	AVND_EVT *evt = 0;
	uint32_t rc = NCSCC_RC_SUCCESS;

	/* create the evt with evt type indicating last step of termination */
	evt = avnd_evt_create(avnd_cb, AVND_EVT_LAST_STEP_TERM, NULL, NULL, NULL, 0, 0);
	if (!evt) {
		LOG_ER("SIG term event create failed");
		rc = NCSCC_RC_FAILURE;
		goto done;
	}

	/* send the event */
	rc = avnd_evt_send(avnd_cb, evt);

done:
	/* free the event */
	if (NCSCC_RC_SUCCESS != rc && evt) {
		LOG_ER("SIG term event send failed");
		avnd_evt_destroy(evt);
	}
}

/**
 * TERM signal is used when user wants to stop OpenSAF (AMF).
 * 
 * @param sig
 */
static void sigterm_handler(int sig)
{
	ncs_sel_obj_ind(&term_sel_obj);
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
		LOG_ER("avnd_create failed");
		goto done;
	}
	if (ncs_sel_obj_create(&term_sel_obj) != NCSCC_RC_SUCCESS) {
		LOG_ER("ncs_sel_obj_create failed");
		goto done;
	}

	if (signal(SIGTERM, sigterm_handler) == SIG_ERR) {
		LOG_ER("signal TERM failed: %s", strerror(errno));
		goto done;
	}
	ImmReader::imm_reader_thread_create();
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

	/* now wait forever */
	while (1) {
		int ret = poll(fds, nfds, -1);

		if (ret == -1) {
			if (errno == EINTR) {
				continue;
			}

			LOG_ER("%s: poll failed - %s", __FUNCTION__, strerror(errno));
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
			ncs_sel_obj_rmv_ind(&term_sel_obj, true, true);
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
	LOG_NO("exiting");
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
	uint32_t rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER();

	/* validate the event type */
	if ((evt->type <= AVND_EVT_INVALID) || (evt->type >= AVND_EVT_MAX)) {
		LOG_ER("%s: Unknown event %u",__FUNCTION__,evt->type);
		goto done;
	}

	/* Temp: AvD Down Handling */
	if (true == cb->is_avd_down){
		LOG_IN("%s: AvD is down, dropping event %u",__FUNCTION__,evt->type);
		goto done;
	}

	/* log the event reception */
	TRACE("Evt type:%u",evt->type);

	/* invoke the event handler */
	rc = g_avnd_func_list[evt->type] (cb, evt);

	if ((SA_AMF_HA_ACTIVE == cb->avail_state_avnd) || (SA_AMF_HA_QUIESCED == cb->avail_state_avnd)) {
		m_AVND_SEND_CKPT_UPDT_SYNC(cb, NCS_MBCSV_ACT_UPDATE, 0);
	}

	/* log the result of event processing */
	TRACE("Evt Type:%u %s",evt->type,((rc == NCSCC_RC_SUCCESS) ? "success" : "failure"));

done:
	if (evt)
		avnd_evt_destroy(evt);
	TRACE_LEAVE();
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

static uint32_t avnd_evt_invalid_evh(AVND_CB *cb, AVND_EVT *evt)
{
	LOG_NO("avnd_evt_invalid_func: %u", evt->type);
	return NCSCC_RC_SUCCESS;
}
