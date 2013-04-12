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

  This file contains routines to process the component errors and execute 
  various recoveries for those errors.

  The following table enumerates various sources of component error detection,
  the detection responsibility & the source of recommended recovery.
 +--------------------------------------------------------------------------+
 |    Error Source    |  Error Detection  |       Recommended Recovery      |
 |                    |   Responsibility  |                                 |
 +--------------------+-------------------+---------------------------------+
 | a) Error Report    | Component         | Specified by component as a part|
 |                    |                   | of error report.                |
 +--------------------+-------------------+---------------------------------+
 | b) Healthcheck     | AMF & Component   | Specified by component as a part|
 |    Failure         |                   | of healthcheck start.           |
 +--------------------+-------------------+---------------------------------+
 | c) Callback Resp   | AMF               | Same as b) for healtcheck       |
 |    Timer Expiry    |                   | callback failure.               |
 |                    |                   | Else default comp recovery      |
 |                    |                   | specified as a part of          |
 |                    |                   | configuration.                  |
 +--------------------+-------------------+---------------------------------+
 | d) Callback Failed | Component         | same as c)                      |
 |    Response (thru  |                   |                                 |
 |    saAmfResponse)  |                   |                                 |
 +--------------------+-------------------+---------------------------------+
..............................................................................

  FUNCTIONS INCLUDED in this module:

  
******************************************************************************
*/

#include "avnd.h"

/* static function declarations */

static uint32_t avnd_err_escalate(AVND_CB *, AVND_SU *, AVND_COMP *, AVSV_ERR_RCVR *);

static uint32_t avnd_err_recover(AVND_CB *, AVND_SU *, AVND_COMP *, AVSV_ERR_RCVR);

uint32_t avnd_err_rcvr_comp_restart(AVND_CB *, AVND_COMP *);
static uint32_t avnd_err_rcvr_su_restart(AVND_CB *, AVND_SU *, AVND_COMP *);
static uint32_t avnd_err_rcvr_comp_failover(AVND_CB *, AVND_COMP *);
static uint32_t avnd_err_rcvr_su_failover(AVND_CB *, AVND_SU *, AVND_COMP *);
static uint32_t avnd_err_rcvr_node_switchover(AVND_CB *, AVND_SU *, AVND_COMP *);
static uint32_t avnd_err_rcvr_node_failover(AVND_CB *, AVND_SU *, AVND_COMP *);
static uint32_t avnd_err_rcvr_node_failfast(AVND_CB *, AVND_SU *, AVND_COMP *);

static AVSV_ERR_RCVR avnd_err_esc_comp_restart(AVND_CB *, AVND_SU *, AVSV_ERR_RCVR *);
static AVSV_ERR_RCVR avnd_err_esc_su_failover(AVND_CB *, AVND_SU *, AVSV_ERR_RCVR *);
static uint32_t avnd_err_restart_esc_level_0(AVND_CB *, AVND_SU *, AVND_ERR_ESC_LEVEL *, AVSV_ERR_RCVR *);
static uint32_t avnd_err_restart_esc_level_1(AVND_CB *, AVND_SU *, AVND_ERR_ESC_LEVEL *, AVSV_ERR_RCVR *);
static uint32_t avnd_err_restart_esc_level_2(AVND_CB *, AVND_SU *, AVND_ERR_ESC_LEVEL *, AVSV_ERR_RCVR *);

/* LSB Changes. Strings to represent source of component Error */

static const char *g_comp_err[] = {
	"noError",
	"errorReport",
	"healthCheckFailed",
	"passiveMonitorFailed",
	"activeMonitorFailed",
	"cLCCommandFailed",
	"healthCheckcallbackTimeout",
	"healthCheckcallbackFailed",
	"avaDown",
	"prxRegTimeout",
	"csiSetcallbackTimeout",
	"csiRemovecallbackTimeout",
	"csiSetcallbackFailed",
	"csiRemovecallbackFailed",
	"qscingCompleteTimeout"
};

static const char *g_comp_rcvr[] = {
	"noRecommendation",
	"componentRestart",
	"componentFailover",
	"nodeSwitchover",
	"nodeFailover",
	"nodeFailfast",
	"clusterReset",
	"applicationRestart",
	"containerRestart",
	"suRestart",
	"suFailover"
};

/****************************************************************************
  Name          : avnd_evt_ava_err_rep
 
  Description   : This routine processes the AMF Error Report message from 
                  AvA. It validates the message, updates the component record
                  with the error info & starts the component error processing.
 
  Arguments     : cb  - ptr to the AvND control block
                  evt - ptr to the AvND event
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
uint32_t avnd_evt_ava_err_rep_evh(AVND_CB *cb, AVND_EVT *evt)
{
	AVSV_AMF_API_INFO *api_info = &evt->info.ava.msg->info.api_info;
	AVSV_AMF_ERR_REP_PARAM *err_rep = &api_info->param.err_rep;
	AVND_COMP *comp = 0;
	AVND_ERR_INFO err;
	uint32_t rc = NCSCC_RC_SUCCESS;
	SaAisErrorT amf_rc = SA_AIS_OK;
	bool msg_from_avnd = false, int_ext_comp = false;

	TRACE_ENTER();

	if (AVND_EVT_AVND_AVND_MSG == evt->type) {
		/* This means that the message has come from proxy AvND to this AvND. */
		msg_from_avnd = true;
	}

	if (false == msg_from_avnd) {
		/* Check for internode or external coomponent first
		   If it is, then forward it to the respective AvND. */
		rc = avnd_int_ext_comp_hdlr(cb, api_info, &evt->mds_ctxt, &amf_rc, &int_ext_comp);
		if (true == int_ext_comp) {
			goto done;
		}
	}

	/* get the comp */
	comp = m_AVND_COMPDB_REC_GET(cb->compdb, err_rep->err_comp);

	/* determine the error code, if any */
	if (!comp)
		amf_rc = SA_AIS_ERR_NOT_EXIST;

	/* We need not entertain errors when comp is not in shape */
	if (comp && (m_AVND_COMP_PRES_STATE_IS_UNINSTANTIATED(comp) ||
		     m_AVND_COMP_PRES_STATE_IS_INSTANTIATIONFAILED(comp) ||
		     m_AVND_COMP_PRES_STATE_IS_TERMINATIONFAILED(comp)))
		amf_rc = SA_AIS_ERR_INVALID_PARAM;

	if (comp && (true == comp->su->su_is_external) && ((true == msg_from_avnd))) {
		/* This request has come from other AvND and it is for external component.
		   Recommended recovery SA_AMF_NODE_SWITCHOVER, 
		   SA_AMF_NODE_FAILOVER, SA_AMF_NODE_FAILFAST or SA_AMF_CLUSTER_RESET
		   are not allowed for external component. */
		if ((SA_AMF_NODE_SWITCHOVER == err_rep->rec_rcvr.saf_amf) ||
		    (SA_AMF_NODE_FAILOVER == err_rep->rec_rcvr.saf_amf) ||
		    (SA_AMF_NODE_FAILFAST == err_rep->rec_rcvr.saf_amf) || (SA_AMF_CLUSTER_RESET == err_rep->rec_rcvr.saf_amf))
			amf_rc = SA_AIS_ERR_INVALID_PARAM;
	}

	if(comp && ((err_rep->rec_rcvr.saf_amf == SA_AMF_CLUSTER_RESET) || 
			(err_rep->rec_rcvr.saf_amf == SA_AMF_APPLICATION_RESTART)|| 
			(err_rep->rec_rcvr.saf_amf == SA_AMF_CONTAINER_RESTART)))
		amf_rc = SA_AIS_ERR_NOT_SUPPORTED;

	/* send the response back to AvA */
	rc = avnd_amf_resp_send(cb, AVSV_AMF_ERR_REP, amf_rc, 0, &api_info->dest, &evt->mds_ctxt, comp, msg_from_avnd);

	/* thru with validation.. process the rest */
	if ((SA_AIS_OK == amf_rc) && (NCSCC_RC_SUCCESS == rc)) {
		/* if time's not specified, use current time TBD */
		if (err_rep->detect_time > 0)
			comp->err_info.detect_time = err_rep->detect_time;
		else
			m_GET_TIME_STAMP(comp->err_info.detect_time);

		m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, comp, AVND_CKPT_COMP_ERR_INFO);

      /*** process the error ***/
		err.src = AVND_ERR_SRC_REP;
		err.rec_rcvr.raw = err_rep->rec_rcvr.raw;
		rc = avnd_err_process(cb, comp, &err);
	}

 done:
	if (NCSCC_RC_SUCCESS != rc) {
		LOG_ER("avnd_evt_ava_err_rep():%s:Hdl=%llx, rec_rcvr:%u",\
				    err_rep->err_comp.value, err_rep->hdl, err_rep->rec_rcvr.raw);
	}

	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
  Name          : avnd_evt_ava_err_clear
 
  Description   : This routine processes the AMF Error Clear message from 
                  AvA. It clears the component error info.
 
  Arguments     : cb  - ptr to the AvND control block
                  evt - ptr to the AvND event
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
uint32_t avnd_evt_ava_err_clear_evh(AVND_CB *cb, AVND_EVT *evt)
{
	AVSV_AMF_API_INFO *api_info = &evt->info.ava.msg->info.api_info;
	AVSV_AMF_ERR_CLEAR_PARAM *err_clear = &api_info->param.err_clear;
	AVND_COMP *comp = 0;
	AVND_CERR_INFO *err_info;
	uint32_t rc = NCSCC_RC_SUCCESS;
	SaAisErrorT amf_rc = SA_AIS_OK;
	bool msg_from_avnd = false, int_ext_comp = false;

	TRACE_ENTER();

	if (AVND_EVT_AVND_AVND_MSG == evt->type) {
		/* This means that the message has come from proxy AvND to this AvND. */
		msg_from_avnd = true;
	}

	if (false == msg_from_avnd) {
		/* Check for internode or external coomponent first
		   If it is, then forward it to the respective AvND. */
		rc = avnd_int_ext_comp_hdlr(cb, api_info, &evt->mds_ctxt, &amf_rc, &int_ext_comp);
		if (true == int_ext_comp) {
			goto done;
		}
	}

	/* get the comp */
	comp = m_AVND_COMPDB_REC_GET(cb->compdb, err_clear->comp_name);

	/* determine the error code, if any */
	if (!comp || !m_AVND_COMP_IS_REG(comp) ||
	    (!m_AVND_COMP_TYPE_IS_PREINSTANTIABLE(comp) && !m_AVND_COMP_TYPE_IS_PROXIED(comp)))
		amf_rc = SA_AIS_ERR_BAD_OPERATION;

	/* send the response back to AvA */
	rc = avnd_amf_resp_send(cb, AVSV_AMF_ERR_CLEAR, amf_rc, 0,
				&api_info->dest, &evt->mds_ctxt, comp, msg_from_avnd);

	/* thru with validation.. process the rest */
	if ((SA_AIS_OK == amf_rc) && (NCSCC_RC_SUCCESS == rc)) {
      /*** clear the err params ***/
		err_info = &comp->err_info;
		err_info->src = 0;
		err_info->detect_time = 0;

		m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, comp, AVND_CKPT_COMP_ERR_INFO);
	}

 done:
	if (NCSCC_RC_SUCCESS != rc) {
		LOG_ER("avnd_evt_ava_err_clear():%s: Hdl=%llx",
				    err_clear->comp_name.value, err_clear->hdl);
	}

	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
  Name          : avnd_err_process
 
  Description   : This routine is a top-level routine to process all the 
                  component errors. It runs the recovery escalation algorithm
                  & determines the escalated recovery. This escalated recovery
                  is then executed.
 
  Arguments     : cb       - ptr to the AvND control block
                  comp     - ptr to the component
                  err_info - ptr to error-info
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
uint32_t avnd_err_process(AVND_CB *cb, AVND_COMP *comp, AVND_ERR_INFO *err_info)
{
	uint32_t esc_rcvr = err_info->rec_rcvr.raw;
	uint32_t rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER2("Comp:'%s' esc_rcvr:'%u'", comp->name.value, esc_rcvr);

	// Handle errors differently when shutdown has started
	if (AVND_TERM_STATE_OPENSAF_SHUTDOWN_STARTED == cb->term_state) {
		LOG_NO("'%s' faulted due to '%s'", comp->name.value, g_comp_err[err_info->src]);
		avnd_comp_cleanup_launch(comp);
		goto done;
	}

	/* when undergoing admin oper do not process any component errors */
	if (comp->admin_oper == SA_TRUE)
		goto done;

	/* Level3 escalation is node failover. we dont expect any
	 ** more errors from application as the node is already failed over. 
	 ** Except AVND_ERR_SRC_AVA_DN, AvSv will ignore any error which happens
	 ** in level3 escalation
	 */
	/* (AVND_ERR_SRC_AVA_DN != err_info->src) check is added so that comp state is marked as failed
	 * and error cleanup (Clearing pending assignments, deleting dynamic info associated with the
	 * component) is done for components which goes down after AVND_ERR_ESC_LEVEL_3 is set
	 */
	if ((cb->node_err_esc_level == AVND_ERR_ESC_LEVEL_3) &&
	    (comp->su->is_ncs == SA_FALSE) && (esc_rcvr != SA_AMF_NODE_FAILFAST) &&
					(AVND_ERR_SRC_AVA_DN != err_info->src)) {
		/* For external component, comp->su->is_ncs is false, so no need to
		   put a check here for external component explicitely. */
		goto done;
	} else if ((cb->node_err_esc_level == AVND_ERR_ESC_LEVEL_3) && (esc_rcvr != SA_AMF_NODE_FAILFAST)) {
		esc_rcvr = SA_AMF_NODE_FAILOVER;
	}

	/* We need not entertain errors when comp is not in shape. 
	   In case of terminating, ava down may come before clc response */
	if (comp && (m_AVND_COMP_PRES_STATE_IS_UNINSTANTIATED(comp) ||
		     m_AVND_COMP_PRES_STATE_IS_INSTANTIATIONFAILED(comp) ||
		     m_AVND_COMP_PRES_STATE_IS_TERMINATIONFAILED(comp) || m_AVND_COMP_PRES_STATE_IS_TERMINATING(comp)))
		goto done;

   	/* update the err params */
	comp->err_info.src = err_info->src;

	/* if time's not specified, use current time TBD */
	if (err_info->src != AVND_ERR_SRC_REP)
		m_GET_TIME_STAMP(comp->err_info.detect_time);

	m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, comp, AVND_CKPT_COMP_ERR_INFO);

	/* determine the escalated recovery */
	rc = avnd_err_escalate(cb, comp->su, comp, &esc_rcvr);
	if (NCSCC_RC_SUCCESS != rc)
		goto done;

	LOG_NO("'%s' faulted due to '%s' : Recovery is '%s'",
		comp->name.value, g_comp_err[err_info->src], g_comp_rcvr[esc_rcvr - 1]);

	if (((comp->su->is_ncs == true) && (esc_rcvr != SA_AMF_COMPONENT_RESTART)) || esc_rcvr == SA_AMF_NODE_FAILFAST) {
		LOG_ER("%s Faulted due to:%s Recovery is:%s",
		       comp->name.value, g_comp_err[comp->err_info.src], g_comp_rcvr[esc_rcvr - 1]);
		/* do the local node reboot for node_failfast or ncs component failure*/
		opensaf_reboot(avnd_cb->node_info.nodeId, (char *)avnd_cb->node_info.executionEnvironment.value,
				"Component faulted: recovery is node failfast");
	}

	/* execute the recovery */
	rc = avnd_err_recover(cb, comp->su, comp, esc_rcvr);
	if (NCSCC_RC_SUCCESS != rc)
		goto done;

 done:
	TRACE_LEAVE2("Return value:'%u'", rc);
	return rc;
}

/****************************************************************************
  Name          : avnd_err_escalate
 
  Description   : This routine runs the recovery escalation algorithm & 
                  determines the escalated recovery.
 
  Arguments     : cb          - ptr to the AvND control block
                  su          - ptr to the SU to which the comp belongs
                  comp        - ptr to the component
                  io_esc_rcvr - escalated recovery
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
uint32_t avnd_err_escalate(AVND_CB *cb, AVND_SU *su, AVND_COMP *comp, uint32_t *io_esc_rcvr)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	if (comp->pres == SA_AMF_PRESENCE_UNINSTANTIATED ||
	    comp->pres == SA_AMF_PRESENCE_INSTANTIATION_FAILED || comp->pres == SA_AMF_PRESENCE_TERMINATION_FAILED)
		return NCSCC_RC_FAILURE;

	/* use default recovery if nothing is recommended */
	if (*io_esc_rcvr == SA_AMF_NO_RECOMMENDATION)
		*io_esc_rcvr = comp->err_info.def_rec;

	/* disallow comp-restart if it's disabled */
	if ((SA_AMF_COMPONENT_RESTART == *io_esc_rcvr) && m_AVND_COMP_IS_RESTART_DIS(comp))
		*io_esc_rcvr = AVSV_ERR_RCVR_SU_FAILOVER;

	switch (*io_esc_rcvr) {
	case SA_AMF_COMPONENT_FAILOVER:	/* treat it as su failover */
	case AVSV_ERR_RCVR_SU_FAILOVER:
		rc = avnd_err_esc_su_failover(cb, su, io_esc_rcvr);
		break;

	case SA_AMF_COMPONENT_RESTART:
		rc = avnd_err_esc_comp_restart(cb, su, io_esc_rcvr);
		break;

	case SA_AMF_NODE_SWITCHOVER:
	case SA_AMF_NODE_FAILOVER:
	case SA_AMF_NODE_FAILFAST:
	case SA_AMF_CLUSTER_RESET:
		break;

	case AVSV_ERR_RCVR_SU_RESTART:
	default:
		osafassert(0);
		break;
	}			/* switch */

	return rc;
}

/****************************************************************************
  Name          : avnd_err_recover
 
  Description   : This routine executes the component failure recovery.
 
  Arguments     : cb   - ptr to the AvND control block
                  su   - ptr to the SU to which the comp belongs
                  comp - ptr to the component
                  rcvr - recovery to be executed
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
uint32_t avnd_err_recover(AVND_CB *cb, AVND_SU *su, AVND_COMP *comp, uint32_t rcvr)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER2("SU:%s Comp:%s",su->name.value,comp->name.value);

	if (comp->pres == SA_AMF_PRESENCE_INSTANTIATING) {
		/* mark the comp failed */
		m_AVND_COMP_FAILED_SET(comp);
		m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, comp, AVND_CKPT_COMP_FLAG_CHANGE);

		/* update comp oper state */
		m_AVND_COMP_OPER_STATE_SET(comp, SA_AMF_OPERATIONAL_DISABLED);
		m_AVND_COMP_OPER_STATE_AVD_SYNC(cb, comp, rc);
		if (NCSCC_RC_SUCCESS != rc)
			return rc;
		m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, comp, AVND_CKPT_COMP_OPER_STATE);

		rc = avnd_comp_clc_fsm_run(cb, comp, AVND_COMP_CLC_PRES_FSM_EV_INST_FAIL);
		return rc;
	}

	/* if we are already inst-failed,  do nothing */
	if ((su->pres == SA_AMF_PRESENCE_INSTANTIATION_FAILED) &&
	    (comp->pres == SA_AMF_PRESENCE_TERMINATING) && (rcvr != SA_AMF_NODE_FAILOVER)
	    && (rcvr != SA_AMF_NODE_FAILFAST)) {
		rc = avnd_comp_clc_fsm_run(cb, comp, AVND_COMP_CLC_PRES_FSM_EV_CLEANUP);
		return rc;
	}

	/* if we are already terminating do nothing */
	if ((comp->pres == SA_AMF_PRESENCE_TERMINATING) && (rcvr == SA_AMF_COMPONENT_RESTART)) {
		rc = avnd_comp_clc_fsm_run(cb, comp, AVND_COMP_CLC_PRES_FSM_EV_CLEANUP);
		return rc;
	}

	/* When SU is in TERMINATING state, higher level recovery (SA_AMF_NODE_FAILOVER,
	   SA_AMF_NODE_FAILFAST and SA_AMF_NODE_SWITCHOVER) should be processed because higher
	   level recovery will terminate the component. If the faulted component has recovery
	   other than these, then clean the component and don't process any recovery for that
	   component. */

	if ((su->pres == SA_AMF_PRESENCE_TERMINATING) && (rcvr != SA_AMF_NODE_FAILOVER)
	    && (rcvr != SA_AMF_NODE_FAILFAST) && (rcvr != SA_AMF_NODE_SWITCHOVER)) {
		/* mark the comp failed */
		m_AVND_COMP_FAILED_SET(comp);
		m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, comp, AVND_CKPT_COMP_FLAG_CHANGE);

		/* update comp oper state */
		m_AVND_COMP_OPER_STATE_SET(comp, SA_AMF_OPERATIONAL_DISABLED);
		m_AVND_COMP_OPER_STATE_AVD_SYNC(cb, comp, rc);
		if (NCSCC_RC_SUCCESS != rc)
			return rc;
		m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, comp, AVND_CKPT_COMP_OPER_STATE);

		/*
		 * SU may be in the middle of SU_SI in assigning/removing state.
		 * signal csi assign/remove done so that su-si assignment/removal
		 * algo can proceed.
		 */
		avnd_comp_cmplete_all_assignment(cb, comp);

		/* clean up the comp */
		rc = avnd_comp_clc_fsm_run(cb, comp, AVND_COMP_CLC_PRES_FSM_EV_CLEANUP);

		return rc;
	}

	switch (rcvr) {
	case SA_AMF_NO_RECOMMENDATION:
		/*m_AVSV_ASSERT(0);
		   break; */

	case SA_AMF_COMPONENT_RESTART:
		rc = avnd_err_rcvr_comp_restart(cb, comp);
		break;

	case SA_AMF_COMPONENT_FAILOVER:
		/* not supported */
		rc = avnd_err_rcvr_comp_failover(cb, comp);
		break;

	case SA_AMF_NODE_SWITCHOVER:
		rc = avnd_err_rcvr_node_switchover(cb, su, comp);
		break;

	case SA_AMF_NODE_FAILOVER:
		rc = avnd_err_rcvr_node_failover(cb, su, comp);
		break;

	case SA_AMF_NODE_FAILFAST:
		rc = avnd_err_rcvr_node_failfast(cb, su, comp);
		break;

	case SA_AMF_CLUSTER_RESET:
		/* not supported */
		break;

	case AVSV_ERR_RCVR_SU_RESTART:
		rc = avnd_err_rcvr_su_restart(cb, su, comp);
		break;

	case AVSV_ERR_RCVR_SU_FAILOVER:
		rc = avnd_err_rcvr_su_failover(cb, su, comp);
		break;

	default:
		osafassert(0);
		break;
	}
	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
  Name          : avnd_err_rcvr_comp_restart
 
  Description   : This routine executes component restart recovery.
 
  Arguments     : cb   - ptr to the AvND control block
                  comp - ptr to the comp
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 
  Notes         : None.
******************************************************************************/
uint32_t avnd_err_rcvr_comp_restart(AVND_CB *cb, AVND_COMP *comp)
{
	AVND_COMP_CSI_REC *csi = 0;
	uint32_t rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER();

	/* mark the comp failed */
	m_AVND_COMP_FAILED_SET(comp);
	m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, comp, AVND_CKPT_COMP_FLAG_CHANGE);

	/* delete the comp current info */
	rc = avnd_comp_curr_info_del(cb, comp);
	if (NCSCC_RC_SUCCESS != rc)
		goto done;

	/* trigger comp-fsm with restart event */
	rc = avnd_comp_clc_fsm_run(cb, comp, AVND_COMP_CLC_PRES_FSM_EV_RESTART);
	if (NCSCC_RC_SUCCESS != rc)
		goto done;

	/* mark the csi assigning for npi comp */
	if (!m_AVND_COMP_TYPE_IS_PREINSTANTIABLE(comp)) {
		csi = m_AVND_CSI_REC_FROM_COMP_DLL_NODE_GET(m_NCS_DBLIST_FIND_FIRST(&comp->csi_list));
		if (m_AVND_COMP_CSI_CURR_ASSIGN_STATE_IS_ASSIGNED(csi)
		    || m_AVND_COMP_CSI_CURR_ASSIGN_STATE_IS_RESTARTING(csi)) {
			m_AVND_COMP_CSI_CURR_ASSIGN_STATE_SET(csi, AVND_COMP_CSI_ASSIGN_STATE_RESTARTING);
			m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, csi, AVND_CKPT_COMP_CSI_CURR_ASSIGN_STATE);
		} else if (m_AVND_COMP_CSI_CURR_ASSIGN_STATE_IS_UNASSIGNED(csi)) {
			/* we need not change the csi state. let it be in unassigned state.
			 * The instantiation success will not trigger any csi assignment done.
			 * If this component is assigned afterwards before completing restart,
			 * the csi will move to assinging.
			 */
		} else {
			m_AVND_COMP_CSI_CURR_ASSIGN_STATE_SET(csi, AVND_COMP_CSI_ASSIGN_STATE_ASSIGNING);
			m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, csi, AVND_CKPT_COMP_CSI_CURR_ASSIGN_STATE);
		}
	}

 done:
	TRACE_LEAVE2("%u", rc);
	return rc;
}

/****************************************************************************
  Name          : avnd_err_rcvr_su_restart
 
  Description   : This routine executes SU restart recovery.
 
  Arguments     : cb          - ptr to the AvND control block
                  su          - ptr to the SU to which the comp belongs
                  failed_comp - ptr to the failed comp that triggered this 
                                recovery
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 
  Notes         : None.
******************************************************************************/
uint32_t avnd_err_rcvr_su_restart(AVND_CB *cb, AVND_SU *su, AVND_COMP *failed_comp)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER();

	/* mark the su & comp failed */
	m_AVND_SU_FAILED_SET(su);
	m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, su, AVND_CKPT_SU_FLAG_CHANGE);
	m_AVND_COMP_FAILED_SET(failed_comp);
	m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, failed_comp, AVND_CKPT_COMP_FLAG_CHANGE);

	/* delete su current info */
	rc = avnd_su_curr_info_del(cb, su);
	if (NCSCC_RC_SUCCESS != rc)
		goto done;

	/* prepare su-sis for fresh assignment */
	rc = avnd_su_si_unmark(cb, su);
	if (NCSCC_RC_SUCCESS != rc)
		goto done;

	/* 
	 * Trigger su-fsm with restart event.
	 */
	rc = avnd_su_pres_fsm_run(cb, su, 0, AVND_SU_PRES_FSM_EV_RESTART);
	if (NCSCC_RC_SUCCESS != rc)
		goto done;

	/* change the comp & su oper state to disabled */
	m_AVND_SU_OPER_STATE_SET(su, SA_AMF_OPERATIONAL_DISABLED);
	m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, su, AVND_CKPT_SU_OPER_STATE);
	m_AVND_COMP_OPER_STATE_SET(failed_comp, SA_AMF_OPERATIONAL_DISABLED);
	m_AVND_COMP_OPER_STATE_AVD_SYNC(cb, failed_comp, rc);
	if (NCSCC_RC_SUCCESS != rc)
		goto done;
	m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, failed_comp, AVND_CKPT_COMP_OPER_STATE);

	/* finally... set su-restart flag */
	m_AVND_SU_RESTART_SET(su);
	m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, su, AVND_CKPT_SU_FLAG_CHANGE);

 done:
	TRACE_LEAVE2("%u", rc);
	return rc;
}

/****************************************************************************
  Name          : avnd_err_rcvr_comp_failover
 
  Description   : This routine executes component failover recovery.
 
  Arguments     : cb   - ptr to the AvND control block
                  comp - ptr to the comp
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 
  Notes         : None.
******************************************************************************/
uint32_t avnd_err_rcvr_comp_failover(AVND_CB *cb, AVND_COMP *comp)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	LOG_NO("%s, Unsupported",__FUNCTION__);

	return rc;
}

/****************************************************************************
  Name          : avnd_err_rcvr_su_failover
 
  Description   : This routine executes SU failover recovery.
 
  Arguments     : cb          - ptr to the AvND control block
                  su          - ptr to the SU to which the comp belongs
                  failed_comp - ptr to the failed comp that triggered this 
                                recovery
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 
  Notes         : None.
******************************************************************************/
uint32_t avnd_err_rcvr_su_failover(AVND_CB *cb, AVND_SU *su, AVND_COMP *failed_comp)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER();

	/* mark the comp failed */
	m_AVND_COMP_FAILED_SET(failed_comp);
	m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, failed_comp, AVND_CKPT_COMP_FLAG_CHANGE);

	/* update comp oper state */
	m_AVND_COMP_OPER_STATE_SET(failed_comp, SA_AMF_OPERATIONAL_DISABLED);
	m_AVND_COMP_OPER_STATE_AVD_SYNC(cb, failed_comp, rc);
	if (NCSCC_RC_SUCCESS != rc)
		goto done;
	m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, failed_comp, AVND_CKPT_COMP_OPER_STATE);

	/* mark the su failed */
	if (!m_AVND_SU_IS_FAILED(su)) {
		m_AVND_SU_FAILED_SET(su);
		m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, su, AVND_CKPT_SU_FLAG_CHANGE);
	}

	/* update su oper state */
	m_AVND_SU_OPER_STATE_SET(su, SA_AMF_OPERATIONAL_DISABLED);
	m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, su, AVND_CKPT_SU_OPER_STATE);

	/* inform AvD */
	rc = avnd_di_oper_send(cb, su, AVSV_ERR_RCVR_SU_FAILOVER);

	/*
	 *  su-sis may be in assigning/removing state. signal csi
	 * assign/remove done so that su-si assignment/removal algo can proceed.
	 */
	avnd_comp_cmplete_all_assignment(cb, failed_comp);

	/* We are now in the context of failover, forget the restart */
	if (su->pres == SA_AMF_PRESENCE_RESTARTING || m_AVND_SU_IS_RESTART(su)) {
		m_AVND_SU_RESTART_RESET(su);
		m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, su, AVND_CKPT_SU_FLAG_CHANGE);
	}

	/* delete curr info of the failed comp */
	rc = avnd_comp_curr_info_del(cb, failed_comp);
	if (NCSCC_RC_SUCCESS != rc)
		goto done;

	/* clean the failed comp */
	if (m_AVND_SU_IS_PREINSTANTIABLE(su)) {
		rc = avnd_comp_clc_fsm_run(cb, failed_comp, AVND_COMP_CLC_PRES_FSM_EV_CLEANUP);
		if (NCSCC_RC_SUCCESS != rc)
			goto done;
	}

 done:
	TRACE_LEAVE2("retval=%u", rc);
	return rc;
}

/****************************************************************************
  Name          : avnd_err_rcvr_node_switchover
 
  Description   : This routine executes Node switchover recovery.
 
  Arguments     : cb          - ptr to the AvND control block
                  failed_su   - ptr to the failed su
                  failed_comp - ptr to the failed comp
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 
  Notes         : None.

******************************************************************************/
uint32_t avnd_err_rcvr_node_switchover(AVND_CB *cb, AVND_SU *failed_su, AVND_COMP *failed_comp)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER();

	/* increase log level to info */
	setlogmask(LOG_UPTO(LOG_INFO));

	/* mark the comp failed */
	m_AVND_COMP_FAILED_SET(failed_comp);
	m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, failed_comp, AVND_CKPT_COMP_FLAG_CHANGE);

	/* update comp oper state */
	m_AVND_COMP_OPER_STATE_SET(failed_comp, SA_AMF_OPERATIONAL_DISABLED);
	m_AVND_COMP_OPER_STATE_AVD_SYNC(cb, failed_comp, rc);
	if (NCSCC_RC_SUCCESS != rc)
		goto done;
	m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, failed_comp, AVND_CKPT_COMP_OPER_STATE);

	/* mark the su failed */
	if (!m_AVND_SU_IS_FAILED(failed_su)) {
		m_AVND_SU_FAILED_SET(failed_su);
		m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, failed_su, AVND_CKPT_SU_FLAG_CHANGE);
	}

	/* transition the su oper state to disabled */
	m_AVND_SU_OPER_STATE_SET(failed_su, SA_AMF_OPERATIONAL_DISABLED);
	m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, failed_su, AVND_CKPT_SU_OPER_STATE);

	/* If the oper_state is already set to SA_AMF_OPERATIONAL_DISABLED, that means
	   Node failover is already informed to AVD, so no need to resend again */
	if(SA_AMF_OPERATIONAL_DISABLED != cb->oper_state) {
		/* transition the node oper state to disabled */
		cb->oper_state = SA_AMF_OPERATIONAL_DISABLED;

		/* inform avd */
		rc = avnd_di_oper_send(cb, failed_su, SA_AMF_NODE_SWITCHOVER);
		if (NCSCC_RC_SUCCESS != rc)
			goto done;
	}


	/*
	 *  su-sis may be in assigning/removing state. signal csi
	 * assign/remove done so that su-si assignment/removal algo can proceed.
	 */
	avnd_comp_cmplete_all_assignment(cb, failed_comp);

	/* We are now in the context of failover, forget the restart */
	if (failed_su->pres == SA_AMF_PRESENCE_RESTARTING || m_AVND_SU_IS_RESTART(failed_su)) {
		m_AVND_SU_RESTART_RESET(failed_su);
		m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, failed_su, AVND_CKPT_SU_FLAG_CHANGE);
	}

	/* delete curr info of the failed comp */
	rc = avnd_comp_curr_info_del(cb, failed_comp);
	if (NCSCC_RC_SUCCESS != rc)
		goto done;

	/* terminate the failed comp */
	if (m_AVND_SU_IS_PREINSTANTIABLE(failed_su)) {
		rc = avnd_comp_clc_fsm_run(cb, failed_comp, AVND_COMP_CLC_PRES_FSM_EV_CLEANUP);
		if (NCSCC_RC_SUCCESS != rc)
			goto done;
	}

 done:
	TRACE_LEAVE2("%u", rc);
	return rc;
}

/****************************************************************************
  Name          : avnd_err_rcvr_node_failover
 
  Description   : This routine executes Node failover recovery.
 
  Arguments     : cb          - ptr to the AvND control block
                  failed_su   - ptr to the failed su
                  failed_comp - ptr to the failed comp
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 
  Notes         : None.

******************************************************************************/
uint32_t avnd_err_rcvr_node_failover(AVND_CB *cb, AVND_SU *failed_su, AVND_COMP *failed_comp)
{
	AVND_COMP *comp;
	uint32_t rc = NCSCC_RC_SUCCESS;

	TRACE_ENTER2("'%s' '%s'", failed_su->name.value, failed_comp->name.value);

	LOG_NO("Terminating all application components (abruptly & unordered)");

	/* increase log level to info */
	setlogmask(LOG_UPTO(LOG_INFO));

	cb->term_state = AVND_TERM_STATE_NODE_FAILOVER_TERMINATING;
	cb->oper_state = SA_AMF_OPERATIONAL_DISABLED;
	cb->failed_su = failed_su;

	m_AVND_COMP_FAILED_SET(failed_comp);
	m_AVND_COMP_OPER_STATE_SET(failed_comp, SA_AMF_OPERATIONAL_DISABLED);
	m_AVND_SU_FAILED_SET(failed_su);
	m_AVND_SU_OPER_STATE_SET(failed_su, SA_AMF_OPERATIONAL_DISABLED);

	/* Unordered cleanup of all local application components */
	for (comp = (AVND_COMP *)ncs_patricia_tree_getnext(&cb->compdb, (uint8_t *)NULL);
		  comp != NULL;
		  comp = (AVND_COMP *) ncs_patricia_tree_getnext(&cb->compdb, (uint8_t *)&comp->name)) {

		if (comp->su->is_ncs || comp->su->su_is_external)
			continue;

		rc = avnd_comp_clc_fsm_run(cb, comp, AVND_COMP_CLC_PRES_FSM_EV_CLEANUP);
		if (rc != NCSCC_RC_SUCCESS) {
			LOG_ER("'%s' termination failed", comp->name.value);
			opensaf_reboot(avnd_cb->node_info.nodeId,
						   (char *)avnd_cb->node_info.executionEnvironment.value,
						   "Component termination failed at node failover");
			LOG_ER("Exiting (due to comp term failed) to aid fast node reboot");
			exit(1);
		}
	}

	TRACE_LEAVE2("%u", rc);
	return rc;
}

/****************************************************************************
  Name          : avnd_err_su_repair
 
  Description   : This routine executes the failed SU repair. It picks up the 
                  failed components & instantiates them.
 
  Arguments     : cb   - ptr to the AvND control block
                  su   - ptr to the SU
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
uint32_t avnd_err_su_repair(AVND_CB *cb, AVND_SU *su)
{
	AVND_COMP *comp = 0;
	bool is_en;
	uint32_t rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER();

	osafassert(m_AVND_SU_IS_FAILED(su));

	if (m_AVND_IS_SHUTTING_DOWN(cb))
		goto done;

	/* If the SU is in inst-failed state, do nothing */
	if (su->pres == SA_AMF_PRESENCE_INSTANTIATION_FAILED)
		return rc;

	/* scan & instantiate failed pi comps */
	for (comp = m_AVND_COMP_FROM_SU_DLL_NODE_GET(m_NCS_DBLIST_FIND_FIRST(&su->comp_list));
	     comp; comp = m_AVND_COMP_FROM_SU_DLL_NODE_GET(m_NCS_DBLIST_FIND_NEXT(&comp->su_dll_node))) {
		if (m_AVND_COMP_IS_FAILED(comp)) {
			if (m_AVND_COMP_TYPE_IS_PREINSTANTIABLE(comp)) {
				/* trigger comp-fsm */
				rc = avnd_comp_clc_fsm_run(cb, comp, AVND_COMP_CLC_PRES_FSM_EV_INST);
				if (NCSCC_RC_SUCCESS != rc)
					goto done;
			}

			if (!m_AVND_SU_IS_PREINSTANTIABLE(su)) {
				m_AVND_COMP_FAILED_RESET(comp);
				m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, comp, AVND_CKPT_COMP_FLAG_CHANGE);
			}
		}
	}			/* for */

	if (!m_AVND_SU_IS_PREINSTANTIABLE(su)) {
		m_AVND_SU_FAILED_RESET(su);
		m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, su, AVND_CKPT_SU_FLAG_CHANGE);
	}

	/* if a mix pi su has all the comps enabled, inform AvD */
	if (m_AVND_SU_IS_PREINSTANTIABLE(su)) {
		m_AVND_SU_IS_ENABLED(su, is_en);
		if (true == is_en) {
			m_AVND_SU_OPER_STATE_SET(su, SA_AMF_OPERATIONAL_ENABLED);
			m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, su, AVND_CKPT_SU_OPER_STATE);
			rc = avnd_di_oper_send(cb, su, 0);
			if (NCSCC_RC_SUCCESS != rc)
				goto done;
		}
	}

 done:
	TRACE_LEAVE2("retval=%u", rc);
	return rc;
}

/****************************************************************************
  Name          : avnd_err_esc_comp_restart
 
  Description   : This routine executes the escalation for component restart. 
 
  Arguments     : cb       - ptr to the AvND control block
                  su       - ptr to the SU
                  err_rcvr - ptr to AVSV_ERR_RCVR
                   
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
uint32_t avnd_err_esc_comp_restart(AVND_CB *cb, AVND_SU *su, AVSV_ERR_RCVR *esc_rcvr)
{
	AVND_ERR_ESC_LEVEL *esc_level = &su->su_err_esc_level;
	uint32_t rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER();

	/* taking care of a case where comp_restart_max is zero */
	if (*esc_level == AVND_ERR_ESC_LEVEL_0 && su->comp_restart_cnt == 0)
		m_AVND_ERR_ESC_LVL_GET(su, *esc_level);

	/* Due to fall thru logic, esc level might have reached level3 */
	if (*esc_level == AVND_ERR_ESC_LEVEL_3)
		cb->node_err_esc_level = *esc_level;

	switch (*esc_level) {
	case AVND_ERR_ESC_LEVEL_0:
		rc = avnd_err_restart_esc_level_0(cb, su, esc_level, esc_rcvr);
		break;

	case AVND_ERR_ESC_LEVEL_1:
		rc = avnd_err_restart_esc_level_1(cb, su, esc_level, esc_rcvr);
		break;

	case AVND_ERR_ESC_LEVEL_2:
		rc = avnd_err_restart_esc_level_2(cb, su, esc_level, esc_rcvr);
		break;

	case AVND_ERR_ESC_LEVEL_3:
		*esc_rcvr = SA_AMF_NODE_FAILOVER;
		break;

	default:
		osafassert(0);
	}

	m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, su, AVND_CKPT_SU_ERR_ESC_LEVEL);

	TRACE_LEAVE2("retval=%u", rc);
	return rc;
}

/****************************************************************************
  Name          : avnd_err_restart_esc_level_0
 
  Description   : This routine executes the escalation for level0. 
 
  Arguments     : cb        - ptr to the AvND control block
                  su        - ptr to the SU
                  esc_level - ptr to AVND_ERR_ESC_LEVEL
                  err_rcvr  - ptr to AVSV_ERR_RCVR
                   
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
uint32_t avnd_err_restart_esc_level_0(AVND_CB *cb, AVND_SU *su, AVND_ERR_ESC_LEVEL *esc_level, AVSV_ERR_RCVR *esc_rcvr)
{

	uint32_t rc = NCSCC_RC_SUCCESS;
	*esc_rcvr = SA_AMF_COMPONENT_RESTART;
	TRACE_ENTER();

	/* first time in this level */
	if (su->comp_restart_cnt == 0) {
		/*start timer */
		m_AVND_TMR_COMP_ERR_ESC_START(cb, su, rc);
		if (NCSCC_RC_SUCCESS != rc)
			goto done;

		m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, su, AVND_CKPT_SU_ERR_ESC_TMR);
		su->comp_restart_cnt++;
		goto done;
	}

	if (su->comp_restart_cnt < su->comp_restart_max) {
		su->comp_restart_cnt++;
		goto done;
	}

	/* ok! go to next level */
	if (su->comp_restart_cnt >= su->comp_restart_max) {
		/*stop the comp-err-esc-timer */
		m_AVND_TMR_COMP_ERR_ESC_STOP(cb, su);
		m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, su, AVND_CKPT_SU_ERR_ESC_TMR);
		su->comp_restart_cnt = 0;

		/* go to the next possible level, is su restart capable? */
		if (su->su_restart_max != 0 && !m_AVND_SU_IS_SU_RESTART_DIS(su)) {
			*esc_level = AVND_ERR_ESC_LEVEL_1;
			rc = avnd_err_restart_esc_level_1(cb, su, esc_level, esc_rcvr);
			goto done;
		}

		if ((cb->su_failover_max != 0) || (true == su->su_is_external)) {
			/* External component should not contribute to NODE FAILOVER of cluster
			   component. Maximum it can go to SU FAILOVER. */
			*esc_level = AVND_ERR_ESC_LEVEL_2;
			rc = avnd_err_restart_esc_level_2(cb, su, esc_level, esc_rcvr);
			goto done;
		}

		/* level 3 esc level */
		cb->node_err_esc_level = AVND_ERR_ESC_LEVEL_3;
		*esc_rcvr = SA_AMF_NODE_FAILOVER;
	}

 done:
	m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, su, AVND_CKPT_SU_COMP_RESTART_CNT);
	TRACE_LEAVE2("retval=%u", rc);
	return rc;
}

/****************************************************************************
  Name          : avnd_err_restart_esc_level_1
 
  Description   : This routine executes the escalation for level1. 
 
  Arguments     : cb        - ptr to the AvND control block
                  su        - ptr to the SU
                  esc_level - ptr to AVND_ERR_ESC_LEVEL
                  err_rcvr  - ptr to AVSV_ERR_RCVR
                   
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
uint32_t avnd_err_restart_esc_level_1(AVND_CB *cb, AVND_SU *su, AVND_ERR_ESC_LEVEL *esc_level, AVSV_ERR_RCVR *esc_rcvr)
{

	uint32_t rc = NCSCC_RC_SUCCESS;
	AVSV_PARAM_INFO param;
	TRACE_ENTER();

	/* If the SU is still instantiating, do jump to next level */
	if (su->pres == SA_AMF_PRESENCE_INSTANTIATING || su->pres == SA_AMF_PRESENCE_RESTARTING
	    || m_AVND_SU_IS_ASSIGN_PEND(su)) {
		/* go to the next possible level, get escalted recovery and modify count */
		if ((cb->su_failover_max != 0) || (true == su->su_is_external)) {
			/* External component should not contribute to NODE FAILOVER of cluster
			   component. Maximum it can go to SU FAILOVER. */
			*esc_level = AVND_ERR_ESC_LEVEL_2;
			rc = avnd_err_restart_esc_level_2(cb, su, esc_level, esc_rcvr);
			goto done;
		}

		/* level 3 esc level */
		cb->node_err_esc_level = AVND_ERR_ESC_LEVEL_3;
		*esc_rcvr = SA_AMF_NODE_FAILOVER;
		goto done;

	}

	/* first time in this level */
	*esc_rcvr = AVSV_ERR_RCVR_SU_RESTART;

	if (su->su_restart_cnt < su->su_restart_max) {
		if (su->su_restart_cnt == 0) {
			m_AVND_TMR_SU_ERR_ESC_START(cb, su, rc);
			if (NCSCC_RC_SUCCESS != rc)
				goto done;

			m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, su, AVND_CKPT_SU_ERR_ESC_TMR);
		}
		su->su_restart_cnt++;

		/* send su_restart_cnt to AVD */
		memset(&param, 0, sizeof(AVSV_PARAM_INFO));
		param.class_id = AVSV_SA_AMF_SU;
		param.attr_id = saAmfSURestartCount_ID;
		param.name = su->name;
		param.act = AVSV_OBJ_OPR_MOD;
		*((uint32_t *)param.value) = m_NCS_OS_HTONL(su->su_restart_cnt);
		param.value_len = sizeof(uint32_t);

		if (NCSCC_RC_SUCCESS != avnd_di_object_upd_send(cb, &param)) {
			TRACE_2("avnd_di_object_upd_send() failed for su_restart_cnt");
		}

		goto done;
	}

	/* reached max count */
	if (su->su_restart_cnt >= su->su_restart_max) {
		/* stop timer */
		m_AVND_TMR_SU_ERR_ESC_STOP(cb, su);
		m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, su, AVND_CKPT_SU_ERR_ESC_TMR);
		su->su_restart_cnt = 0;

		/* go to the next possible level, get escalted recovery and modify count */
		if ((cb->su_failover_max != 0) || (true == su->su_is_external)) {
			/* External component should not contribute to NODE FAILOVER of cluster
			   component. Maximum it can go to SU FAILOVER. */
			*esc_level = AVND_ERR_ESC_LEVEL_2;
			rc = avnd_err_restart_esc_level_2(cb, su, esc_level, esc_rcvr);
			goto done;
		}

		/* level 3 esc level */
		cb->node_err_esc_level = AVND_ERR_ESC_LEVEL_3;
		*esc_rcvr = SA_AMF_NODE_FAILOVER;
	}

 done:
	m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, su, AVND_CKPT_SU_RESTART_CNT);
	TRACE_LEAVE2("retval=%u", rc);
	return rc;
}

/****************************************************************************
  Name          : avnd_err_restart_esc_level_2
 
  Description   : This routine executes the escalation for level2. 
 
  Arguments     : cb        - ptr to the AvND control block
                  su        - ptr to the SU
                  esc_level - ptr to AVND_ERR_ESC_LEVEL
                  err_rcvr  - ptr to AVSV_ERR_RCVR
                   
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
uint32_t avnd_err_restart_esc_level_2(AVND_CB *cb, AVND_SU *su, AVND_ERR_ESC_LEVEL *esc_level, AVSV_ERR_RCVR *esc_rcvr)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER();

	/* first time in this level */
	*esc_rcvr = AVSV_ERR_RCVR_SU_FAILOVER;

	/* External components are not supposed to escalate SU Failover of
	   cluster components. For Ext component, SU Failover will be limited to
	   them only. */
	if (true == su->su_is_external) {
		/* Once it had been escalted to AVND_ERR_ESC_LEVEL_2, then reset to 
		   AVND_ERR_ESC_LEVEL_0  */
		if (AVND_ERR_ESC_LEVEL_2 == su->su_err_esc_level)
			su->su_err_esc_level = AVND_ERR_ESC_LEVEL_0;
		goto done;

	}

	if (cb->su_failover_cnt == 0) {
		m_AVND_TMR_NODE_ERR_ESC_START(cb, rc);
		if (NCSCC_RC_SUCCESS != rc)
			goto done;

		cb->su_failover_cnt++;
		goto done;
	}

	if (cb->su_failover_cnt < cb->su_failover_max) {
		cb->su_failover_cnt++;
		goto done;
	}

	/* reached max count */
	if (cb->su_failover_cnt >= cb->su_failover_max) {
		/* stop timer */
		m_AVND_TMR_NODE_ERR_ESC_STOP(cb);
		cb->su_failover_cnt = 0;
		cb->node_err_esc_level = AVND_ERR_ESC_LEVEL_3;
		*esc_rcvr = SA_AMF_NODE_FAILOVER;
	}

 done:
	TRACE_LEAVE2("retval=%u", rc);
	return rc;
}

/****************************************************************************
  Name          : avnd_err_esc_comp_failover
 
  Description   : This routine executes the escalation for component failover.
 
  Arguments     : cb       - ptr to the AvND control block
                  su       - ptr to the SU
                  esc_rcvr - ptr to AVSV_ERR_RCVR
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
AVSV_ERR_RCVR avnd_err_esc_su_failover(AVND_CB *cb, AVND_SU *su, AVSV_ERR_RCVR *esc_rcvr)
{
	AVND_ERR_ESC_LEVEL *esc_level = &cb->node_err_esc_level;
	uint32_t rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER();

	/* initalize */
	*esc_rcvr = AVSV_ERR_RCVR_SU_FAILOVER;

	if (true == su->su_is_external) {
		/* External component should not contribute to NODE FAILOVER of cluster
		   component. Maximum it can go to SU FAILOVER. */
		return rc;
	}

	if (cb->su_failover_max == 0) {	/* fall thru */
		*esc_rcvr = SA_AMF_NODE_FAILOVER;
		*esc_level = AVND_ERR_ESC_LEVEL_3;
		goto done;
	}

	if (cb->su_failover_cnt == 0) {
		/* This could be because of component reporting error SA_AMF_COMPONENT_FAILOVER OR because of 
		   component failing, whose SU has SUFailover attr set to true. */
		su->su_err_esc_level = AVND_ERR_ESC_LEVEL_2;
		/* start timer */
		m_AVND_TMR_NODE_ERR_ESC_START(cb, rc);
		if (NCSCC_RC_SUCCESS != rc)
			goto done;

		cb->su_failover_cnt++;
		goto done;
	}

	if (cb->su_failover_cnt < cb->su_failover_max) {
		/* This means NODE_ERR_ESC may be already running because of other SUs escalations.*/
		su->su_err_esc_level = AVND_ERR_ESC_LEVEL_2;
		cb->su_failover_cnt++;
		goto done;
	}

	if (cb->su_failover_cnt >= cb->su_failover_max) {
		/* stop timer */
		m_AVND_TMR_NODE_ERR_ESC_STOP(cb);
		cb->su_failover_cnt = 0;
		*esc_level = AVND_ERR_ESC_LEVEL_3;
		*esc_rcvr = SA_AMF_NODE_FAILOVER;
	}

 done:
	TRACE_LEAVE2("retval=%u", rc);
	return rc;
}

/****************************************************************************
  Name          : avnd_evt_tmr_node_err_esc
 
  Description   : This routine handles the the expiry of the 'node error 
                  escalation' timer. It indicates the end of the su failover 
                  probation period for the node.
 
  Arguments     : cb  - ptr to the AvND control block
                  evt - ptr to the AvND event
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
uint32_t avnd_evt_tmr_node_err_esc_evh(AVND_CB *cb, AVND_EVT *evt)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	AVND_SU *su;

	TRACE_ENTER();

	su = (AVND_SU *)ncs_patricia_tree_getnext(&cb->sudb, (uint8_t *)0);
	while (su != 0) {
		/* Only reset to those Sus, which have affected the node err esc.*/
		if (su->su_err_esc_level == AVND_ERR_ESC_LEVEL_2) {
			su->comp_restart_cnt = 0;
			su->su_restart_cnt = 0;
			su->su_err_esc_level = AVND_ERR_ESC_LEVEL_0;
		}
		su = (AVND_SU *)ncs_patricia_tree_getnext(&cb->sudb, (uint8_t *)&su->name);
	}

	/* reset all parameters */
	cb->su_failover_cnt = 0;
	cb->node_err_esc_level = AVND_ERR_ESC_LEVEL_0;

	TRACE_LEAVE2("returning success");
	return rc;
}

/****************************************************************************
  Name          : avnd_err_rcvr_node_failfast
 
  Description   : This routine executes Node failfast recovery.
 
  Arguments     : cb          - ptr to the AvND control block
                  failed_su   - ptr to the failed su
                  failed_comp - ptr to the failed comp
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 
  Notes         : None.

******************************************************************************/
uint32_t avnd_err_rcvr_node_failfast(AVND_CB *cb, AVND_SU *failed_su, AVND_COMP *failed_comp)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER();

	/* mark the comp & su failed */
	m_AVND_COMP_FAILED_SET(failed_comp);
	m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, failed_comp, AVND_CKPT_COMP_FLAG_CHANGE);
	m_AVND_SU_FAILED_SET(failed_su);
	m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, failed_su, AVND_CKPT_SU_FLAG_CHANGE);

	/* update comp oper state */
	m_AVND_COMP_OPER_STATE_SET(failed_comp, SA_AMF_OPERATIONAL_DISABLED);
	m_AVND_COMP_OPER_STATE_AVD_SYNC(cb, failed_comp, rc);
	if (NCSCC_RC_SUCCESS != rc)
		goto done;
	m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, failed_comp, AVND_CKPT_COMP_OPER_STATE);

	/* delete curr info of the failed comp */
	rc = avnd_comp_curr_info_del(cb, failed_comp);
	if (NCSCC_RC_SUCCESS != rc)
		goto done;

	/* terminate the failed comp */
	if (m_AVND_SU_IS_PREINSTANTIABLE(failed_su)) {
		rc = avnd_comp_clc_fsm_run(cb, failed_comp, AVND_COMP_CLC_PRES_FSM_EV_CLEANUP);
		if (NCSCC_RC_SUCCESS != rc)
			goto done;
	}

	/* transition the su & node oper state to disabled */
	cb->oper_state = SA_AMF_OPERATIONAL_DISABLED;
	m_AVND_SU_OPER_STATE_SET(failed_su, SA_AMF_OPERATIONAL_DISABLED);
	m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, failed_su, AVND_CKPT_SU_OPER_STATE);

	/* inform avd */
	rc = avnd_di_oper_send(cb, failed_su, SA_AMF_NODE_FAILFAST);

 done:
	TRACE_LEAVE2("%u", rc);
	return rc;
}
