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
#include <daemon.h>
#include <logtrace.h>

#include <nid_api.h>
#include "fm.h"


FM_CB *fm_cb = NULL;
char *role_string[] = { "Undefined", "ACTIVE", "STANDBY", "QUIESCED",
	"ASSERTING", "YIELDING", "UNDEFINED"
};

/*****************************************************************
*                                                               *
*         Prototypes for static functions                       *
*                                                               *
*****************************************************************/

static uns32 fm_agents_startup(void);
static uns32 fm_agents_shutdown(void);
static uns32 fm_get_args(FM_CB *);
static uns32 fms_fms_exchange_node_info(FM_CB *);
static uns32 fm_nid_notify(uns32);
static uns32 fm_tmr_start(FM_TMR *, SaTimeT);
static void fm_mbx_msg_handler(FM_CB *, FM_EVT *);
static void fm_tmr_exp(void *);

uns32 gl_fm_hdl;

/*****************************************************************************

PROCEDURE NAME:       main

DESCRIPTION:          Main routine for FM

*****************************************************************************/
int main(int argc, char *argv[])
{
	NCS_SEL_OBJ mbx_sel_obj, pipe_sel_obj, highest_sel_obj;
	NCS_SEL_OBJ amf_sel_obj = {0, 0};
	NCS_SEL_OBJ_SET sel_obj_set;
	FM_EVT *fm_mbx_evt = NULL;

	daemonize(argc, argv);

	if (fm_agents_startup() != NCSCC_RC_SUCCESS) {
/* notify the NID */
		fm_nid_notify((uns32)NCSCC_RC_FAILURE);
		goto fm_agents_startup_failed;
	}

/* Allocate memory for FM_CB. */
	fm_cb = m_MMGR_ALLOC_FM_CB;
	if (NULL == fm_cb) {
/* notify the NID */
		syslog(LOG_ERR, "CB Allocation failed.");
		fm_nid_notify((uns32)NCSCC_RC_FAILURE);
		goto fm_agents_startup_failed;
	}

	memset(fm_cb, 0, sizeof(FM_CB));

/* Create CB handle */
	gl_fm_hdl = ncshm_create_hdl(NCS_HM_POOL_ID_COMMON, NCS_SERVICE_ID_GFM, (NCSCONTEXT)fm_cb);

/* Take CB handle */
	ncshm_take_hdl(NCS_SERVICE_ID_GFM, gl_fm_hdl);

	if (fm_get_args(fm_cb) != NCSCC_RC_SUCCESS) {
/* notify the NID */
		fm_nid_notify((uns32)NCSCC_RC_FAILURE);
		goto fm_get_args_failed;
	}

/* Create MBX. */
	if (m_NCS_IPC_CREATE(&fm_cb->mbx) != NCSCC_RC_SUCCESS) {
		syslog(LOG_ERR, "m_NCS_IPC_CREATE() failed.");
		fm_nid_notify((uns32)NCSCC_RC_FAILURE);
		goto fm_get_args_failed;
	}

/* Attach MBX */
	if (m_NCS_IPC_ATTACH(&fm_cb->mbx) != NCSCC_RC_SUCCESS) {
		syslog(LOG_ERR, "m_NCS_IPC_ATTACH() failed.");
		fm_nid_notify((uns32)NCSCC_RC_FAILURE);
		goto fm_mbx_attach_failure;
	}

/* MDS initialization */
	if (fm_mds_init(fm_cb) != NCSCC_RC_SUCCESS) {
		fm_nid_notify((uns32)NCSCC_RC_FAILURE);
		goto fm_mds_init_failed;
	}

/* RDA initialization */
	if (fm_rda_init(fm_cb) != NCSCC_RC_SUCCESS) {
		fm_nid_notify((uns32)NCSCC_RC_FAILURE);
		goto fm_rda_init_failed;
	}

/* Open FM pipe for receiving AMF up intimation */
	if (fm_amf_open(&fm_cb->fm_amf_cb) != NCSCC_RC_SUCCESS) {
		fm_nid_notify((uns32)NCSCC_RC_FAILURE);
		goto fm_hpl_lib_init_failed;
	}

/* Get mailbox selection object */
	mbx_sel_obj = m_NCS_IPC_GET_SEL_OBJ(&fm_cb->mbx);

/* Get pipe selection object */
	m_SET_FD_IN_SEL_OBJ(fm_cb->fm_amf_cb.pipe_fd, pipe_sel_obj);

/* Give CB hdl */
	ncshm_give_hdl(gl_fm_hdl);

/* notify the NID */
	fm_nid_notify(NCSCC_RC_SUCCESS);

/* clear selection object set */
	m_NCS_SEL_OBJ_ZERO(&sel_obj_set);

/* Set only one pipe eelection object in set */
	m_NCS_SEL_OBJ_SET(pipe_sel_obj, &sel_obj_set);

	highest_sel_obj = pipe_sel_obj;

/* Wait for pipe selection object */
	if (m_NCS_SEL_OBJ_SELECT(highest_sel_obj, &sel_obj_set, NULL, NULL, NULL) != -1) {
/* following if will be true only first time */
		if (m_NCS_SEL_OBJ_ISSET(pipe_sel_obj, &sel_obj_set)) {
/* Take handle */
			ncshm_take_hdl(NCS_SERVICE_ID_GFM, gl_fm_hdl);

/* Process the message received on pipe */
			if (fm_amf_pipe_process_msg(&fm_cb->fm_amf_cb) != NCSCC_RC_SUCCESS)
				goto done;

/* Get amf selection object */
			m_SET_FD_IN_SEL_OBJ(fm_cb->fm_amf_cb.amf_fd, amf_sel_obj);

/* Release handle */
			ncshm_give_hdl(gl_fm_hdl);
		}
	}

/* clear selection object set */
	m_NCS_SEL_OBJ_ZERO(&sel_obj_set);
	m_NCS_SEL_OBJ_SET(amf_sel_obj, &sel_obj_set);
	m_NCS_SEL_OBJ_SET(mbx_sel_obj, &sel_obj_set);
	highest_sel_obj = m_GET_HIGHER_SEL_OBJ(amf_sel_obj, mbx_sel_obj);

/* Wait infinitely on  */
	while ((m_NCS_SEL_OBJ_SELECT(highest_sel_obj, &sel_obj_set, NULL, NULL, NULL) != -1)) {
		if (m_NCS_SEL_OBJ_ISSET(mbx_sel_obj, &sel_obj_set)) {
/* Take handle */
			ncshm_take_hdl(NCS_SERVICE_ID_GFM, gl_fm_hdl);

			fm_mbx_evt = (FM_EVT *)m_NCS_IPC_NON_BLK_RECEIVE(&fm_cb->mbx, msg);
			if (fm_mbx_evt) {
				fm_mbx_msg_handler(fm_cb, fm_mbx_evt);
			}
/* Release handle */
			ncshm_give_hdl(gl_fm_hdl);
		}

		if (m_NCS_SEL_OBJ_ISSET(amf_sel_obj, &sel_obj_set)) {
/* Take handle */
			ncshm_take_hdl(NCS_SERVICE_ID_GFM, gl_fm_hdl);

/* Process the message received on amf selection object */
			fm_amf_process_msg(&fm_cb->fm_amf_cb);

/* Release handle */
			ncshm_give_hdl(gl_fm_hdl);
		}

		m_NCS_SEL_OBJ_SET(amf_sel_obj, &sel_obj_set);
		m_NCS_SEL_OBJ_SET(mbx_sel_obj, &sel_obj_set);
		highest_sel_obj = m_GET_HIGHER_SEL_OBJ(amf_sel_obj, mbx_sel_obj);
	}

	fm_amf_close(&fm_cb->fm_amf_cb);

 fm_hpl_lib_init_failed:

	fm_rda_finalize(fm_cb);

 fm_rda_init_failed:

	fm_mds_finalize(fm_cb);

 fm_mds_init_failed:

	m_NCS_IPC_DETACH(&fm_cb->mbx, NULL, NULL);

 fm_mbx_attach_failure:

	m_NCS_IPC_RELEASE(&fm_cb->mbx, NULL);

 fm_get_args_failed:

	ncshm_destroy_hdl(NCS_SERVICE_ID_GFM, gl_fm_hdl);
	gl_fm_hdl = 0;
	m_MMGR_FREE_FM_CB(fm_cb);

 fm_agents_startup_failed:

	fm_agents_shutdown();
 done:
	return 1;
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
static uns32 fm_agents_startup(void)
{
	uns32 rc = NCSCC_RC_SUCCESS;

/* Start agents */
	rc = ncs_agents_startup();
	if (rc != NCSCC_RC_SUCCESS) {
		syslog(LOG_ERR, "ncs_agents_startup failed");
		return rc;
	}

	return rc;
}

/****************************************************************************
* Name          : fm_agents_shutdown
*
* Description   :  Shutdown NCS agents
*
* Arguments     :  None.
*
* Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
* 
* Notes         : None. 
*****************************************************************************/
static uns32 fm_agents_shutdown(void)
{
	uns32 rc = NCSCC_RC_SUCCESS;

/* Start agents */
	rc = ncs_agents_shutdown();
	if (rc != NCSCC_RC_SUCCESS) {
		syslog(LOG_ERR, "ncs_agents_shutdown failed");
		return rc;
	}

	return rc;
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
static uns32 fm_get_args(FM_CB *fm_cb)
{
	char *value;

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

	return NCSCC_RC_SUCCESS;
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

	switch (fm_mbx_evt->evt_code) {
	case FM_EVT_NODE_DOWN:
		LOG_IN("Role: %s, Node Down for node id: %u", role_string[fm_cb->role], fm_mbx_evt->node_id);
		if ((fm_cb->role == PCS_RDA_STANDBY)||(fm_cb->role == PCS_RDA_QUIESCED)) {
			if ((fm_mbx_evt->node_id == fm_cb->peer_node_id)) {
/* Start Promote active timer */
				if ((fm_cb->peer_node_name.length != 0) && (fm_cb->role != PCS_RDA_QUIESCED)) {

					LOG_IN("Promote active timer started");
					fm_tmr_start(&fm_cb->promote_active_tmr, fm_cb->active_promote_tmr_val);
				} else {
					fm_cb->role = PCS_RDA_ACTIVE;
					opensaf_reboot(fm_cb->peer_node_id, (char *)fm_cb->peer_node_name.value,
						"Received Node Down for Active peer");
					fm_rda_set_role(fm_cb, PCS_RDA_ACTIVE);
				}
			}
		} else if (fm_cb->role == PCS_RDA_ACTIVE) {
				opensaf_reboot(fm_cb->peer_node_id, (char *)fm_cb->peer_node_name.value,
					"Received Node Down for standby peer");
		}
		break;
	case FM_EVT_PEER_UP:
/* Peer fm came up so sending ee_id of this node */
		if (fm_cb->node_name.length != 0)
			fms_fms_exchange_node_info(fm_cb);
		break;
	case FM_EVT_TMR_EXP:
/* Timer Expiry event posted */
		if (fm_mbx_evt->info.fm_tmr->type == FM_TMR_PROMOTE_ACTIVE) {
/* Now. Try resetting other blade */
			fm_cb->role = PCS_RDA_ACTIVE;

			LOG_IN("Reseting peer controller node_id=%u", fm_cb->peer_node_id);
			opensaf_reboot(fm_cb->peer_node_id, (char *)fm_cb->peer_node_name.value,
				       "Received Node Down for Active peer");
			fm_rda_set_role(fm_cb, PCS_RDA_ACTIVE);
		}
		break;

	default:
		break;
	}

/* Free the event. */
	if (fm_mbx_evt != NULL) {
		m_MMGR_FREE_FM_EVT(fm_mbx_evt);
		fm_mbx_evt = NULL;
	}

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
uns32 fm_tmr_start(FM_TMR *tmr, SaTimeT period)
{
	uns32 tmr_period;

	tmr_period = (uns32)period;

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
static uns32 fms_fms_exchange_node_info(FM_CB *fm_cb)
{
	GFM_GFM_MSG gfm_msg;

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
static uns32 fm_nid_notify(uns32 nid_err)
{
	uns32 error;
	uns32 nid_stat_code;

	if (nid_err > NCSCC_RC_SUCCESS)
		nid_err = NCSCC_RC_FAILURE;

	nid_stat_code = nid_err;
	return nid_notify("HLFM", nid_stat_code, &error);
}
