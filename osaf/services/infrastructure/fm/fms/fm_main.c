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
DESCRIPTION:

This file contains the main() routine for FM.

******************************************************************************/

#include <configmake.h>
#include <stdlib.h>
#include <stdbool.h>
#include <daemon.h>
#include <logtrace.h>

#include <nid_api.h>
#include "fm.h"


enum {
	FD_TERM = 0,
	FD_AMF = 1,
	FD_MBX
};

FM_CB *fm_cb = NULL;
char *role_string[] = { "Undefined", "ACTIVE", "STANDBY", "QUIESCED",
	"ASSERTING", "YIELDING", "UNDEFINED"
};

/*****************************************************************
*                                                               *
*         Prototypes for static functions                       *
*                                                               *
*****************************************************************/

static uint32_t fm_agents_startup(void);
static uint32_t fm_get_args(FM_CB *);
static uint32_t fms_fms_exchange_node_info(FM_CB *);
static uint32_t fm_nid_notify(uint32_t);
static uint32_t fm_tmr_start(FM_TMR *, SaTimeT);
static void fm_mbx_msg_handler(FM_CB *, FM_EVT *);
static void fm_tmr_exp(void *);
void handle_mbx_event(void);
extern uint32_t fm_amf_init(FM_AMF_CB *fm_amf_cb);
uint32_t gl_fm_hdl;
static NCS_SEL_OBJ usr1_sel_obj;
void fm_proc_svc_down(FM_CB *cb, FM_EVT *fm_mbx_evt);

/**
 * USR1 signal is used when AMF wants instantiate us as a
 * component. Wake up the main thread so it can register with
 * AMF.
 *
 * @param i_sig_num
 */
static void sigusr1_handler(int sig)
{
	(void)sig;
	signal(SIGUSR1, SIG_IGN);
	ncs_sel_obj_ind(usr1_sel_obj);
}

/**
 * Callback from RDA. Post a message/event to the FM mailbox.
 * @param cb_hdl
 * @param cb_info
 * @param error_code
 */
static void rda_cb(uint32_t cb_hdl, PCS_RDA_CB_INFO *cb_info, PCSRDA_RETURN_CODE error_code)
{
	uint32_t rc;
	FM_EVT *evt = NULL;

	TRACE_ENTER();

	evt = calloc(1, sizeof(FM_EVT));
	if (NULL == evt) {
		LOG_ER("calloc failed");
		goto done;
	}

	evt->evt_code = FM_EVT_RDA_ROLE;
	evt->info.rda_info.role = cb_info->info.io_role;

	rc = ncs_ipc_send(&fm_cb->mbx, (NCS_IPC_MSG *)evt, MDS_SEND_PRIORITY_HIGH);
	if (rc != NCSCC_RC_SUCCESS) {
		syslog(LOG_ERR, "IPC send failed %d", rc);
		free(evt);	
	}

 done:
	TRACE_LEAVE();
}


/*****************************************************************************

PROCEDURE NAME:       main

DESCRIPTION:          Main routine for FM

*****************************************************************************/
int main(int argc, char *argv[])
{
	NCS_SEL_OBJ mbx_sel_obj;
	nfds_t nfds = 3;
	struct pollfd fds[nfds];
	int ret=0;
	int rc = NCSCC_RC_FAILURE;
	int term_fd;
	bool nid_started = false;
	char *control_tipc = NULL;

	opensaf_reboot_prepare();

	daemonize(argc, argv);

	/* Determine how this process was started, by NID or AMF */
	if (getenv("SA_AMF_COMPONENT_NAME") == NULL)
		nid_started = true;

	if (fm_agents_startup() != NCSCC_RC_SUCCESS) {
		goto fm_init_failed;
	}

	/* Allocate memory for FM_CB. */
	fm_cb = m_MMGR_ALLOC_FM_CB;
	if (NULL == fm_cb) {
		syslog(LOG_ERR, "CB Allocation failed.");
		goto fm_init_failed;
	}

	memset(fm_cb, 0, sizeof(FM_CB));
	fm_cb->fm_amf_cb.nid_started = nid_started;

	/* Variable to control whether FM will trigger failover immediately
	 * upon recieving down event of critical services or will wait
	 * for node_downevent. By default, OPENSAF_MANAGE_TIPC = yes,
	 * If this variable is set to yes, OpenSAF will rmmod tipc which means
	 * NODE_down will happen immediately and there is no need to act on service down.
	 * If this variable is set to no, user will only do /etc/init.d/opensafd stop
	 * and no rmmod is done, which means no NODE_DOWN event will arrive, and
	 * therefore FM has to act on service down events of critical service,
	 * In the short term, FM will use service down events of AMFND, IMMD and IMMND
	 * to trigger failover.
	 */
	fm_cb->control_tipc = true; /* Default behaviour */

	/* Create CB handle */
	gl_fm_hdl = ncshm_create_hdl(NCS_HM_POOL_ID_COMMON, NCS_SERVICE_ID_GFM, (NCSCONTEXT)fm_cb);

	/* Take CB handle */
	ncshm_take_hdl(NCS_SERVICE_ID_GFM, gl_fm_hdl);

	if (fm_get_args(fm_cb) != NCSCC_RC_SUCCESS) {
	/* notify the NID */
		syslog(LOG_ERR, "fm_get args failed.");
		goto fm_init_failed;
	}

	/* Create MBX. */
	if (m_NCS_IPC_CREATE(&fm_cb->mbx) != NCSCC_RC_SUCCESS) {
		syslog(LOG_ERR, "m_NCS_IPC_CREATE() failed.");
		goto fm_init_failed;
	}

/* Attach MBX */
	if (m_NCS_IPC_ATTACH(&fm_cb->mbx) != NCSCC_RC_SUCCESS) {
		syslog(LOG_ERR, "m_NCS_IPC_ATTACH() failed.");
		goto fm_init_failed;
	}

/* MDS initialization */
	if (fm_mds_init(fm_cb) != NCSCC_RC_SUCCESS) {
		goto fm_init_failed;
	}

/* RDA initialization */
	if (fm_rda_init(fm_cb) != NCSCC_RC_SUCCESS) {
		goto fm_init_failed;
	}

	if ((control_tipc = getenv("OPENSAF_MANAGE_TIPC")) == NULL)
		fm_cb->control_tipc = false;
	else if (strncmp(control_tipc, "yes", 3) == 0)
		fm_cb->control_tipc = true;
	else
		fm_cb->control_tipc = false;

	if ((rc = rda_register_callback(0, rda_cb)) != NCSCC_RC_SUCCESS) {
		syslog(LOG_ERR, "rda_register_callback FAILED %u", rc);
		goto fm_init_failed;
	}

	if (nid_started &&
		(rc = ncs_sel_obj_create(&usr1_sel_obj)) != NCSCC_RC_SUCCESS) {
		LOG_ER("ncs_sel_obj_create FAILED");
		goto fm_init_failed;
	}


	if (nid_started &&
		signal(SIGUSR1, sigusr1_handler) == SIG_ERR) {
		LOG_ER("signal USR1 FAILED: %s", strerror(errno));
		goto fm_init_failed;
	}

	if (!nid_started &&
		fm_amf_init(&fm_cb->fm_amf_cb) != NCSCC_RC_SUCCESS)
		goto done;

	fm_cb->csi_assigned = false;

	/* Get mailbox selection object */
	mbx_sel_obj = m_NCS_IPC_GET_SEL_OBJ(&fm_cb->mbx);

	/* Give CB hdl */
	ncshm_give_hdl(gl_fm_hdl);

	daemon_sigterm_install(&term_fd);

	fds[FD_TERM].fd = term_fd;
	fds[FD_TERM].events = POLLIN;

	/* USR1/AMF fd */
	fds[FD_AMF].fd = nid_started ?
		usr1_sel_obj.rmv_obj : fm_cb->fm_amf_cb.amf_fd;
	fds[FD_AMF].events = POLLIN;

	/* Mailbox */
	fds[FD_MBX].fd = mbx_sel_obj.rmv_obj;
	fds[FD_MBX].events = POLLIN;

 
	/* notify the NID */
	if (nid_started)
		fm_nid_notify(NCSCC_RC_SUCCESS);

	while (1) {
		ret = poll(fds, nfds, -1);

		if (ret == -1) {
			if (errno == EINTR)
			continue;
			LOG_ER("poll failed - %s", strerror(errno));
			break;
		}

		if (fds[FD_TERM].revents & POLLIN) {
			daemon_exit();
		}

		if (fds[FD_AMF].revents & POLLIN) {
			if (fm_cb->fm_amf_cb.amf_hdl != 0) {
			SaAisErrorT error;
			TRACE("AMF event rec");
			if ((error = saAmfDispatch(fm_cb->fm_amf_cb.amf_hdl, SA_DISPATCH_ALL)) != SA_AIS_OK) {
				LOG_ER("saAmfDispatch failed: %u", error);
				goto done;
			}
			} else {
				TRACE("SIGUSR1 event rec");
				ncs_sel_obj_destroy(usr1_sel_obj);
				if (fm_amf_init(&fm_cb->fm_amf_cb) != NCSCC_RC_SUCCESS)
					goto done;
				fds[FD_AMF].fd = fm_cb->fm_amf_cb.amf_fd;
			}
		}

		if (fds[FD_MBX].revents & POLLIN)
			handle_mbx_event();
	}

 fm_init_failed:
	if (nid_started)
		fm_nid_notify((uint32_t) NCSCC_RC_FAILURE);
 done:
	syslog(LOG_ERR, "Exiting...");
	exit(1);
}


/****************************************************************************
* Name          : handle_mbx_event
*
* Description   : Processes the mail box message. 
*
* Arguments     :  void.
*
* Return Values : void.
* 
* Notes         : None. 
*****************************************************************************/
void handle_mbx_event(void)
{
	FM_EVT *fm_mbx_evt = NULL;
	TRACE_ENTER();     
	ncshm_take_hdl(NCS_SERVICE_ID_GFM, gl_fm_hdl);
	fm_mbx_evt = (FM_EVT *)m_NCS_IPC_NON_BLK_RECEIVE(&fm_cb->mbx, msg);
	if (fm_mbx_evt)
		fm_mbx_msg_handler(fm_cb, fm_mbx_evt);

	ncshm_give_hdl(gl_fm_hdl);
	TRACE_LEAVE();
}


/****************************************************************************
* Name          : fm_agents_startup
*
* Description   : Starts NCS agents. 
*
* Arguments     :  None.
*
* Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
* 
* Notes         : None. 
*****************************************************************************/
static uint32_t fm_agents_startup(void)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER();
	/* Start agents */
	rc = ncs_agents_startup();
	if (rc != NCSCC_RC_SUCCESS) {
		syslog(LOG_ERR, "ncs_agents_startup failed");
		return rc;
	}

	return rc;
	TRACE_LEAVE();
}

/****************************************************************************
* Name          : fm_get_args
*
* Description   :  Parses configuration and store the values in Data str.
*
* Arguments     : Pointer to Control block 
*
* Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
* 
* Notes         : None. 
*****************************************************************************/
static uint32_t fm_get_args(FM_CB *fm_cb)
{
	char *value;
	TRACE_ENTER();
	value = getenv("EE_ID");
	if (value != NULL) {
		fm_cb->node_name.length = strlen(value);
		memcpy(fm_cb->node_name.value, value, fm_cb->node_name.length);
		LOG_NO("EE_ID : %s", fm_cb->node_name.value);
	} else
		fm_cb->node_name.length = 0;

/* Update fm_cb configuration fields */
	fm_cb->node_id = m_NCS_GET_NODE_ID;

	fm_cb->active_promote_tmr_val = atoi(getenv("FMS_PROMOTE_ACTIVE_TIMER"));

/* Set timer variables */
	fm_cb->promote_active_tmr.type = FM_TMR_PROMOTE_ACTIVE;
	
  	TRACE_LEAVE();  
	return NCSCC_RC_SUCCESS;
}

void fm_proc_svc_down(FM_CB *cb, FM_EVT *fm_mbx_evt)
{
	switch (fm_mbx_evt->svc_id) {
		case NCSMDS_SVC_ID_IMMND:
			cb->immnd_down = true;
			LOG_NO("IMMND down on: %x", cb->peer_node_id);
			break;
		case NCSMDS_SVC_ID_AVND:
			cb->amfnd_down = true;
			LOG_NO("AMFND down on: %x", cb->peer_node_id);
			break;
		case NCSMDS_SVC_ID_IMMD:
			cb->immd_down = true;
			LOG_NO("IMMD down on: %x", cb->peer_node_id);
			break;
		case NCSMDS_SVC_ID_AVD:
			cb->amfd_down = true;
			LOG_NO("AVD down on: %x", cb->peer_node_id);
			break;
		case NCSMDS_SVC_ID_GFM:
			cb->fm_down = true;
			LOG_NO("FM down on: %x", cb->peer_node_id);
			break;
		default:
			break;
	}

	/* Processing only for alternate node.
	* Service downs of AMFND, IMMD, IMMND is the same as NODE_DOWN from 4.4 onwards.
	* This is required to handle the usecase involving
	* '/etc/init.d/opensafd stop' without an OS reboot cycle
	* Process service downs only if OpenSAF is not controlling TIPC.
	* If OpenSAF is controlling TIPC, just wait for NODE_DOWN to trigger failover.
	*/
	if (cb->immd_down && cb->immnd_down && cb->amfnd_down && cb->amfd_down && cb->fm_down) {
		LOG_NO("Core services went down on node_id: %x", fm_mbx_evt->node_id);
		fm_send_node_down_to_mbx(cb, fm_mbx_evt->node_id);
		/* Reset peer downs, because we've made MDS RED subscriptions */
		cb->immd_down = false;
		cb->immnd_down = false;
		cb->amfnd_down = false;
		cb->amfd_down = false;
		cb->fm_down = false;
	}
}

/****************************************************************************
* Name          : fm_mbx_msg_handler
*
* Description   : Processes Mail box messages between FM. 
*
* Arguments     :  Pointer to Control block and Mail box event 
*
* Return Values : None.
* 
* Notes         : None. 
*****************************************************************************/
static void fm_mbx_msg_handler(FM_CB *fm_cb, FM_EVT *fm_mbx_evt)
{
	TRACE_ENTER();
	switch (fm_mbx_evt->evt_code) {
	case FM_EVT_NODE_DOWN:
		LOG_NO("Current role: %s", role_string[fm_cb->role]);
		if ((fm_mbx_evt->node_id == fm_cb->peer_node_id)) {
			/* Check whether node(AMF) initialization is done */
			if (fm_cb->csi_assigned == false) {
				opensaf_reboot(0, NULL,
						"Failover occurred, but this node is not yet ready");
			}
			/* Start Promote active timer */
			if ((fm_cb->role == PCS_RDA_STANDBY) && (fm_cb->active_promote_tmr_val != 0)){
				fm_tmr_start(&fm_cb->promote_active_tmr, fm_cb->active_promote_tmr_val);
				LOG_NO("Promote active timer started");
			} else {
				TRACE("rda role: %s, amf_state: %u", role_string[fm_cb->role], fm_cb->amf_state);
				/* The local node is Standby, Quiesced or Active. Reboot the peer node. 
				 * Note: If local node is Active, there are two interpretations.
				 *	- Normal scenario where the Standby went down
				 *	- Standby went down in the middle of a swtichover and AMF has
				 *	  transitioned CSI state, but not the RDA state.
				 *       In both the cases, this node should be set to ACTIVE.
				 */
				if ((SaAmfHAStateT)fm_cb->role != fm_cb->amf_state )
					TRACE("Failover processing trigerred in the middle of role change");
					/* The previous log message in the above has been changed 
					 * because the log "Failover occurred in the middle of switchover" can
					 * be misleading in the case when it is not a switchover
					 * but just that failover has been trigerred quicker than the
					 * node_down event has been received.
					 */
				opensaf_reboot(fm_cb->peer_node_id, (char *)fm_cb->peer_node_name.value,
						"Received Node Down for peer controller");
				if (!((fm_cb->role == PCS_RDA_ACTIVE) && (fm_cb->amf_state == (SaAmfHAStateT)PCS_RDA_ACTIVE))) {
					fm_cb->role = PCS_RDA_ACTIVE;
					LOG_NO("Controller Failover: Setting role to ACTIVE");
					fm_rda_set_role(fm_cb, PCS_RDA_ACTIVE);
				}
			}
		}
		break;
	case FM_EVT_SVC_DOWN:
		fm_proc_svc_down(fm_cb, fm_mbx_evt);
		break;
	case FM_EVT_PEER_UP:
/* Peer fm came up so sending ee_id of this node */
		if (fm_cb->node_name.length != 0)
			fms_fms_exchange_node_info(fm_cb);
		break;
	case FM_EVT_TMR_EXP:
/* Timer Expiry event posted */
		if (fm_mbx_evt->info.fm_tmr->type == FM_TMR_PROMOTE_ACTIVE) {
			/* Check whether node(AMF) initialization is done */
			if (fm_cb->csi_assigned == false) {
				opensaf_reboot(0, NULL,
				"Failover occurred, but this node is not yet ready");
			}
/* Now. Try resetting other blade */
			fm_cb->role = PCS_RDA_ACTIVE;

			LOG_NO("Reseting peer controller node id: %x", fm_cb->peer_node_id);
			opensaf_reboot(fm_cb->peer_node_id, (char *)fm_cb->peer_node_name.value,
				       "Received Node Down for Active peer");
			fm_rda_set_role(fm_cb, PCS_RDA_ACTIVE);
		}
		break;
	case FM_EVT_RDA_ROLE:
		/* RDA role assignment for this controller node */
		fm_cb->role = fm_mbx_evt->info.rda_info.role;
		syslog(LOG_INFO, "RDA role for this controller node: %s", role_string[fm_cb->role]);
		break;
	default:
		break;
	}

	/* Free the event. */
	if (fm_mbx_evt != NULL) {
		m_MMGR_FREE_FM_EVT(fm_mbx_evt);
		fm_mbx_evt = NULL;
	}
	TRACE_LEAVE();
	return;
}

/****************************************************************************
* Name          : fm_tmr_start
*
* Description   : Starts timer with the given period 
*
* Arguments     :  Pointer to TImer Data Str
*
* Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
* 
* Notes         : None. 
*****************************************************************************/
uint32_t fm_tmr_start(FM_TMR *tmr, SaTimeT period)
{
	uint32_t tmr_period;
	TRACE_ENTER();
	tmr_period = (uint32_t)period;

	if (tmr->tmr_id == NULL) {
		m_NCS_TMR_CREATE(tmr->tmr_id, tmr_period, fm_tmr_exp, (void *)tmr);
	}

	if (tmr->status == FM_TMR_RUNNING) {
		return NCSCC_RC_FAILURE;
	}

	m_NCS_TMR_START(tmr->tmr_id, tmr_period, fm_tmr_exp, (void *)tmr);
	tmr->status = FM_TMR_RUNNING;

	if (TMR_T_NULL == tmr->tmr_id) {
		return NCSCC_RC_FAILURE;
	}
	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
* Name          : fm_tmr_exp
*
* Description   : Timer Expiry function 
*
* Arguments     : Pointer to timer data structure 
*
* Return Values : None.
* 
* Notes         : None. 
*****************************************************************************/
void fm_tmr_exp(void *fm_tmr)
{
	FM_CB *fm_cb = NULL;
	FM_TMR *tmr = (FM_TMR *)fm_tmr;
	FM_EVT *evt = NULL;

	TRACE_ENTER();
	if (tmr == NULL) {
		return;
	}

/* Take handle */
	fm_cb = ncshm_take_hdl(NCS_SERVICE_ID_GFM, gl_fm_hdl);

	if (fm_cb == NULL) {
		syslog(LOG_ERR, "Taking handle failed in timer expiry ");
		return;
	}

	if (FM_TMR_STOPPED == tmr->status) {
		return;
	}
	tmr->status = FM_TMR_STOPPED;

/* Create & send the timer event to the FM MBX. */
	evt = m_MMGR_ALLOC_FM_EVT;

	if (evt != NULL) {
		memset(evt, '\0', sizeof(FM_EVT));
		evt->evt_code = FM_EVT_TMR_EXP;
		evt->info.fm_tmr = tmr;

		if (m_NCS_IPC_SEND(&fm_cb->mbx, evt, NCS_IPC_PRIORITY_HIGH) != NCSCC_RC_SUCCESS) {
			syslog(LOG_ERR, "IPC send failed in timer expiry ");
			m_MMGR_FREE_FM_EVT(evt);
		}
	}

/* Give handle */
	ncshm_give_hdl(gl_fm_hdl);
	fm_cb = NULL;
	TRACE_LEAVE();
	return;
}

/****************************************************************************
* Name          : fms_fms_exchange_node_info 
*
* Description   : sends ee_id to peer . 
*
* Arguments     : Pointer to Control Block. 
*
* Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
* 
* Notes         : None. 
*****************************************************************************/
static uint32_t fms_fms_exchange_node_info(FM_CB *fm_cb)
{
	GFM_GFM_MSG gfm_msg;
	TRACE_ENTER();
	if (fm_cb->peer_adest != 0) {
/* peer fms present */
		memset(&gfm_msg, 0, sizeof(GFM_GFM_MSG));
		gfm_msg.msg_type = GFM_GFM_EVT_NODE_INFO_EXCHANGE;
		gfm_msg.info.node_info.node_id = fm_cb->node_id;
		gfm_msg.info.node_info.node_name.length = fm_cb->node_name.length;
		memcpy(gfm_msg.info.node_info.node_name.value, fm_cb->node_name.value, fm_cb->node_name.length);

		if (NCSCC_RC_SUCCESS != fm_mds_async_send(fm_cb, (NCSCONTEXT)&gfm_msg,
							  NCSMDS_SVC_ID_GFM, MDS_SEND_PRIORITY_MEDIUM,
							  0, fm_cb->peer_adest, 0)) {
			syslog(LOG_ERR, "Sending node-info message to peer fms failed");
			return NCSCC_RC_FAILURE;
		}

		return NCSCC_RC_SUCCESS;
	}
	TRACE_LEAVE();
	return NCSCC_RC_FAILURE;
}

/****************************************************************************
* Name          : fm_nid_notify
*
* Description   : Sends notification to NID
*
* Arguments     :  Error Type
*
* Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
* 
* Notes         : None. 
*****************************************************************************/
static uint32_t fm_nid_notify(uint32_t nid_err)
{
	uint32_t error;
	uint32_t nid_stat_code;

	TRACE_ENTER();
	if (nid_err > NCSCC_RC_SUCCESS)
		nid_err = NCSCC_RC_FAILURE;

	nid_stat_code = nid_err;
	TRACE_LEAVE();
	return nid_notify("HLFM", nid_stat_code, &error);
}
