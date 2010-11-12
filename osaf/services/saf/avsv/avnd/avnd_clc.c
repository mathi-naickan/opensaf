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

  This file includes the macros & routines to manage the component life cycle.
  This includes the component presence state FSM.
 
..............................................................................

  FUNCTIONS INCLUDED in this module:

  
******************************************************************************
*/

#include <logtrace.h>

#include <avnd.h>

/* static function declarations */
static uns32 avnd_comp_clc_uninst_inst_hdler(AVND_CB *, AVND_COMP *);
static uns32 avnd_comp_clc_xxxing_cleansucc_hdler(AVND_CB *, AVND_COMP *);
static uns32 avnd_comp_clc_xxxing_instfail_hdler(AVND_CB *, AVND_COMP *);
static uns32 avnd_comp_clc_insting_inst_hdler(AVND_CB *, AVND_COMP *);
static uns32 avnd_comp_clc_insting_instsucc_hdler(AVND_CB *, AVND_COMP *);
static uns32 avnd_comp_clc_insting_term_hdler(AVND_CB *, AVND_COMP *);
static uns32 avnd_comp_clc_insting_clean_hdler(AVND_CB *, AVND_COMP *);
static uns32 avnd_comp_clc_insting_cleanfail_hdler(AVND_CB *, AVND_COMP *);
static uns32 avnd_comp_clc_insting_restart_hdler(AVND_CB *, AVND_COMP *);
static uns32 avnd_comp_clc_inst_term_hdler(AVND_CB *, AVND_COMP *);
static uns32 avnd_comp_clc_inst_clean_hdler(AVND_CB *, AVND_COMP *);
static uns32 avnd_comp_clc_inst_restart_hdler(AVND_CB *, AVND_COMP *);
static uns32 avnd_comp_clc_inst_orph_hdler(AVND_CB *, AVND_COMP *);
static uns32 avnd_comp_clc_terming_termsucc_hdler(AVND_CB *, AVND_COMP *);
static uns32 avnd_comp_clc_terming_termfail_hdler(AVND_CB *, AVND_COMP *);
static uns32 avnd_comp_clc_terming_cleansucc_hdler(AVND_CB *, AVND_COMP *);
static uns32 avnd_comp_clc_terming_cleanfail_hdler(AVND_CB *, AVND_COMP *);
static uns32 avnd_comp_clc_restart_instsucc_hdler(AVND_CB *, AVND_COMP *);
static uns32 avnd_comp_clc_restart_term_hdler(AVND_CB *, AVND_COMP *);
static uns32 avnd_comp_clc_restart_termsucc_hdler(AVND_CB *, AVND_COMP *);
static uns32 avnd_comp_clc_restart_termfail_hdler(AVND_CB *, AVND_COMP *);
static uns32 avnd_comp_clc_restart_clean_hdler(AVND_CB *, AVND_COMP *);
static uns32 avnd_comp_clc_restart_cleanfail_hdler(AVND_CB *, AVND_COMP *);
static uns32 avnd_comp_clc_orph_instsucc_hdler(AVND_CB *, AVND_COMP *);
static uns32 avnd_comp_clc_orph_term_hdler(AVND_CB *, AVND_COMP *);
static uns32 avnd_comp_clc_orph_clean_hdler(AVND_CB *, AVND_COMP *);
static uns32 avnd_comp_clc_orph_restart_hdler(AVND_CB *, AVND_COMP *);

uns32 avnd_comp_clc_st_chng_prc(AVND_CB *, AVND_COMP *, SaAmfPresenceStateT, SaAmfPresenceStateT);

static uns32 avnd_comp_clc_resp(NCS_OS_PROC_EXECUTE_TIMED_CB_INFO *);
static uns32 avnd_instfail_su_failover(AVND_CB *, AVND_SU *, AVND_COMP *);
static char *avnd_prep_attr_env_var(AVND_COMP *, uns32 *);

/***************************************************************************
 ** C O M P O N E N T   C L C   F S M   M A T R I X   D E F I N I T I O N **
 ***************************************************************************/

/* evt handlers are named in this format: avnd_comp_clc_<st>_<ev>_hdler() */
static AVND_COMP_CLC_FSM_FN avnd_comp_clc_fsm[][AVND_COMP_CLC_PRES_FSM_EV_MAX - 1] = {
	/* SA_AMF_PRESENCE_UNINSTANTIATED */
	{
	 avnd_comp_clc_uninst_inst_hdler,	/* INST EV */
	 0,			/* INST_SUCC EV */
	 0,			/* INST_FAIL EV */
	 0,			/* TERM EV */
	 0,			/* TERM_SUCC EV */
	 0,			/* TERM_FAIL EV */
	 0,			/* CLEANUP EV */
	 0,			/* CLEANUP_SUCC EV */
	 0,			/* CLEANUP_FAIL EV */
	 0,			/* RESTART EV */
	 0,			/* ORPH EV */
	 },

	/* SA_AMF_PRESENCE_INSTANTIATING */
	{
	 avnd_comp_clc_insting_inst_hdler,	/* INST EV */
	 avnd_comp_clc_insting_instsucc_hdler,	/* INST_SUCC EV */
	 avnd_comp_clc_xxxing_instfail_hdler,	/* INST_FAIL EV */
	 avnd_comp_clc_insting_term_hdler,	/* TERM EV */
	 0,			/* TERM_SUCC EV */
	 0,			/* TERM_FAIL EV */
	 avnd_comp_clc_insting_clean_hdler,	/* CLEANUP EV */
	 avnd_comp_clc_xxxing_cleansucc_hdler,	/* CLEANUP_SUCC EV */
	 avnd_comp_clc_insting_cleanfail_hdler,	/* CLEANUP_FAIL EV */
	 avnd_comp_clc_insting_restart_hdler,	/* RESTART EV */
	 0,			/* ORPH EV */
	 },

	/* SA_AMF_PRESENCE_INSTANTIATED */
	{
	 0,			/* INST EV */
	 0,			/* INST_SUCC EV */
	 0,			/* INST_FAIL EV */
	 avnd_comp_clc_inst_term_hdler,	/* TERM EV */
	 0,			/* TERM_SUCC EV */
	 0,			/* TERM_FAIL EV */
	 avnd_comp_clc_inst_clean_hdler,	/* CLEANUP EV */
	 0,			/* CLEANUP_SUCC EV */
	 0,			/* CLEANUP_FAIL EV */
	 avnd_comp_clc_inst_restart_hdler,	/* RESTART EV */
	 avnd_comp_clc_inst_orph_hdler,	/* ORPH EV */
	 },

	/* SA_AMF_PRESENCE_TERMINATING */
	{
	 0,			/* INST EV */
	 0,			/* INST_SUCC EV */
	 0,			/* INST_FAIL EV */
	 0,			/* TERM EV */
	 avnd_comp_clc_terming_termsucc_hdler,	/* TERM_SUCC EV */
	 avnd_comp_clc_terming_termfail_hdler,	/* TERM_FAIL EV */
	 avnd_comp_clc_terming_termfail_hdler,	/* CLEANUP EV */
	 avnd_comp_clc_terming_cleansucc_hdler,	/* CLEANUP_SUCC EV */
	 avnd_comp_clc_terming_cleanfail_hdler,	/* CLEANUP_FAIL EV */
	 0,			/* RESTART EV */
	 0,			/* ORPH EV */
	 },

	/* SA_AMF_PRESENCE_RESTARTING */
	{
	 0,			/* INST EV */
	 avnd_comp_clc_restart_instsucc_hdler,	/* INST_SUCC EV */
	 avnd_comp_clc_xxxing_instfail_hdler,	/* INST_FAIL EV */
	 avnd_comp_clc_restart_term_hdler,	/* TERM EV */
	 avnd_comp_clc_restart_termsucc_hdler,	/* TERM_SUCC EV */
	 avnd_comp_clc_restart_termfail_hdler,	/* TERM_FAIL EV */
	 avnd_comp_clc_restart_clean_hdler,	/* CLEANUP EV */
	 avnd_comp_clc_xxxing_cleansucc_hdler,	/* CLEANUP_SUCC EV */
	 avnd_comp_clc_restart_cleanfail_hdler,	/* CLEANUP_FAIL EV */
	 0,			/* RESTART EV */
	 0,			/* ORPH EV */
	 },

	/* SA_AMF_PRESENCE_INSTANTIATION_FAILED */
	{
	 0,			/* INST EV */
	 0,			/* INST_SUCC EV */
	 0,			/* INST_FAIL EV */
	 0,			/* TERM EV */
	 0,			/* TERM_SUCC EV */
	 0,			/* TERM_FAIL EV */
	 0,			/* CLEANUP EV */
	 0,			/* CLEANUP_SUCC EV */
	 0,			/* CLEANUP_FAIL EV */
	 0,			/* RESTART EV */
	 0,			/* ORPH EV */
	 },

	/* SA_AMF_PRESENCE_TERMINATION_FAILED */
	{
	 0,			/* INST EV */
	 0,			/* INST_SUCC EV */
	 0,			/* INST_FAIL EV */
	 0,			/* TERM EV */
	 0,			/* TERM_SUCC EV */
	 0,			/* TERM_FAIL EV */
	 0,			/* CLEANUP EV */
	 0,			/* CLEANUP_SUCC EV */
	 0,			/* CLEANUP_FAIL EV */
	 0,			/* RESTART EV */
	 0,			/* ORPH EV */
	 },

	/* SA_AMF_PRESENCE_ORPHANED */
	{
	 0,			/* INST EV */
	 avnd_comp_clc_orph_instsucc_hdler,	/* INST_SUCC EV */
	 0,			/* INST_FAIL EV */
	 avnd_comp_clc_orph_term_hdler,	/* TERM EV */
	 0,			/* TERM_SUCC EV */
	 0,			/* TERM_FAIL EV */
	 avnd_comp_clc_orph_clean_hdler,	/* CLEANUP EV */
	 0,			/* CLEANUP_SUCC EV */
	 0,			/* CLEANUP_FAIL EV */
	 avnd_comp_clc_orph_restart_hdler,	/* RESTART EV */
	 0,			/* ORPH EV */
	 }

};

/**
 * Convert NCS_OS_PROC_EXEC_STATUS to a string
 * @param status
 * 
 * @return const char*
 */
static const char *str_exec_status(NCS_OS_PROC_EXEC_STATUS status)
{
	switch (status) {
	case NCS_OS_PROC_EXEC_FAIL:
		return "Exec of script failed (script not readable or path wrong)";
	case NCS_OS_PROC_EXIT_NORMAL:
		return "Exec of script success, and script exits with status zero";
	case NCS_OS_PROC_EXIT_WAIT_TIMEOUT:
		return "Script did not exit within time";
	case NCS_OS_PROC_EXIT_WITH_CODE:
		return "Exec of script success, but script exits with non-zero status";
	case NCS_OS_PROC_EXIT_ON_SIGNAL:
		return "Exec of script success, but script exits due to a signal";
	default:
		return "unknown";
	}
}

/**
 * Log detailed cause of failed exec
 * @param exec_stat
 * @param comp
 * @param exec_cmd
 */
static void log_failed_exec(NCS_OS_PROC_EXEC_STATUS_INFO *exec_stat,
	AVND_COMP *comp, AVND_COMP_CLC_CMD_TYPE exec_cmd)
{
	assert(exec_cmd <= AVND_COMP_CLC_CMD_TYPE_MAX);

	LOG_NO("Reason: %s", str_exec_status(exec_stat->value));

	if (NCS_OS_PROC_EXIT_WITH_CODE == exec_stat->value)
		LOG_NO("Exit code: %u", exec_stat->info.exit_with_code.exit_code);

	if (NCS_OS_PROC_EXEC_FAIL == exec_stat->value)
		LOG_NO("CLC CLI script: %s", comp->clc_info.cmds[exec_cmd-1].cmd);

	if (NCS_OS_PROC_EXIT_ON_SIGNAL == exec_stat->value)
		LOG_NO("Signal: %u, CLC CLI script: %s", exec_stat->info.exit_on_signal.signal_num,comp->clc_info.cmds[exec_cmd-1].cmd);
}

/****************************************************************************
  Name          : avnd_evt_clc_resp
 
  Description   : This routine processes response of a command execution. It 
                  identifies the component & the command from the execution 
                  context & then generates events for the component presence 
                  state FSM. This routine also processes the failure in 
                  launching the command i.e. fork, exec failures etc.
 
  Arguments     : cb  - ptr to the AvND control block
                  evt - ptr to the AvND event
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
uns32 avnd_evt_clc_resp_evh(AVND_CB *cb, AVND_EVT *evt)
{
	AVND_COMP_CLC_PRES_FSM_EV ev = AVND_COMP_CLC_PRES_FSM_EV_MAX;
	AVND_CLC_EVT *clc_evt = &evt->info.clc;
	AVND_COMP *comp = 0;
	uns32 rc = NCSCC_RC_SUCCESS;
	uns32 amcmd = 0;

	TRACE_ENTER();

	/* get the comp */
	if (clc_evt->exec_ctxt) {
		/* => cmd successfully launched (scan the entire comp-database) */
		for (comp = (AVND_COMP *)ncs_patricia_tree_getnext(&cb->compdb,
								   (uns8 *)0); comp;
		     comp = (AVND_COMP *)ncs_patricia_tree_getnext(&cb->compdb, (uns8 *)&comp->name)) {
			if (comp->clc_info.cmd_exec_ctxt == clc_evt->exec_ctxt)
				break;
			if (comp->clc_info.am_cmd_exec_ctxt == clc_evt->exec_ctxt) {
				amcmd = 1;
				break;
			}
		}
	} else
		/* => cmd did not launch successfully (comp is available) */
		comp = m_AVND_COMPDB_REC_GET(cb->compdb, clc_evt->comp_name);

	/* either other cmd is currently executing or the comp is deleted */
	if (!comp)
		goto done;

	if (amcmd || clc_evt->cmd_type == AVND_COMP_CLC_CMD_TYPE_AMSTART ||
	    clc_evt->cmd_type == AVND_COMP_CLC_CMD_TYPE_AMSTOP) {
		switch (comp->clc_info.am_exec_cmd) {
		case AVND_COMP_CLC_CMD_TYPE_AMSTART:
			rc = avnd_comp_amstart_clc_res_process(cb, comp, clc_evt->exec_stat.value);
			break;
		case AVND_COMP_CLC_CMD_TYPE_AMSTOP:
			rc = avnd_comp_amstop_clc_res_process(cb, comp, clc_evt->exec_stat.value);
			break;

		default:
			assert(0);
		}
	} else {
		/* determine the event for the fsm */
		switch (comp->clc_info.exec_cmd) {
		case AVND_COMP_CLC_CMD_TYPE_INSTANTIATE:
			/* 
			 * note that inst-succ evt is generated for the fsm only 
			 * when the comp is registered too 
			 */
			if (NCS_OS_PROC_EXIT_NORMAL == clc_evt->exec_stat.value) {
				/* mark that the inst cmd has been successfully executed */
				m_AVND_COMP_INST_CMD_SUCC_SET(comp);
				m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, comp, AVND_CKPT_COMP_FLAG_CHANGE);

				if (!m_AVND_COMP_TYPE_IS_PREINSTANTIABLE(comp) || m_AVND_COMP_IS_REG(comp))
					/* all set... proceed with inst-succ evt for the fsm */
					ev = AVND_COMP_CLC_PRES_FSM_EV_INST_SUCC;
				else {
					/* start the comp-reg timer */
					m_AVND_TMR_COMP_REG_START(cb, *comp, rc);
					m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, comp, AVND_CKPT_COMP_CLC_REG_TMR);
				}
			} else {
				ev = AVND_COMP_CLC_PRES_FSM_EV_INST_FAIL;

				if (NCS_OS_PROC_EXIT_WITH_CODE == clc_evt->exec_stat.value) {
					comp->clc_info.inst_code_rcvd =
					    clc_evt->exec_stat.info.exit_with_code.exit_code;
					m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, comp, AVND_CKPT_COMP_INST_CODE_RCVD);
				}

				LOG_NO("Instantiation of '%s' failed", comp->name.value);
				log_failed_exec(&clc_evt->exec_stat, comp, comp->clc_info.exec_cmd);
			}
			break;

		case AVND_COMP_CLC_CMD_TYPE_TERMINATE:

			if (NCS_OS_PROC_EXIT_NORMAL == clc_evt->exec_stat.value)
				ev = AVND_COMP_CLC_PRES_FSM_EV_TERM_SUCC;
			else {
				ev = AVND_COMP_CLC_PRES_FSM_EV_CLEANUP;
				m_AVND_COMP_TERM_FAIL_SET(comp);
				m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, comp, AVND_CKPT_COMP_FLAG_CHANGE);
				LOG_NO("Termination of '%s' failed", comp->name.value);
				log_failed_exec(&clc_evt->exec_stat, comp, comp->clc_info.exec_cmd);
			}

			break;

		case AVND_COMP_CLC_CMD_TYPE_CLEANUP:
			if (NCS_OS_PROC_EXIT_NORMAL == clc_evt->exec_stat.value) {
				ev = AVND_COMP_CLC_PRES_FSM_EV_CLEANUP_SUCC;
			} else {
				ev = AVND_COMP_CLC_PRES_FSM_EV_CLEANUP_FAIL;
				LOG_NO("Cleanup of '%s' failed", comp->name.value);
				log_failed_exec(&clc_evt->exec_stat, comp, comp->clc_info.exec_cmd);
			}
			break;

		default:
			assert(0);
		}

		/* reset the cmd exec context params */
		comp->clc_info.exec_cmd = AVND_COMP_CLC_CMD_TYPE_MAX;
		m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, comp, AVND_CKPT_COMP_EXEC_CMD);
		comp->clc_info.cmd_exec_ctxt = 0;
		m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, comp, AVND_CKPT_COMP_CMD_EXEC_CTXT);
	}			/* if */

	/* run the fsm */
	if (AVND_COMP_CLC_PRES_FSM_EV_MAX != ev)
		rc = avnd_comp_clc_fsm_run(cb, comp, ev);

done:
	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
  Name          : avnd_evt_comp_pres_fsm_ev
 
  Description   : This routine processes the event to trigger the component
                  FSM.
 
  Arguments     : cb  - ptr to the AvND control block
                  evt - ptr to the AvND event
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
uns32 avnd_evt_comp_pres_fsm_evh(AVND_CB *cb, AVND_EVT *evt)
{
	AVND_COMP_FSM_EVT *comp_fsm_evt = &evt->info.comp_fsm;
	AVND_COMP *comp = 0;
	uns32 rc = NCSCC_RC_SUCCESS;

	TRACE_ENTER();

	/* get the comp */
	comp = m_AVND_COMPDB_REC_GET(cb->compdb, comp_fsm_evt->comp_name);
	if (!comp)
		goto done;

	/* run the fsm */
	if (AVND_COMP_CLC_PRES_FSM_EV_MAX != comp_fsm_evt->ev)
		rc = avnd_comp_clc_fsm_run(cb, comp, comp_fsm_evt->ev);

done:
	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
  Name          : avnd_evt_tmr_clc_comp_reg
 
  Description   : This routine processes the component registration timer 
                  expiry. An instantiation failed event is generated for the 
                  fsm.
 
  Arguments     : cb  - ptr to the AvND control block
                  evt - ptr to the AvND event
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
uns32 avnd_evt_tmr_clc_comp_reg_evh(AVND_CB *cb, AVND_EVT *evt)
{
	AVND_TMR_EVT *tmr = &evt->info.tmr;
	AVND_COMP *comp = 0;
	uns32 rc = NCSCC_RC_SUCCESS;

	TRACE_ENTER();

	/* retrieve the comp */
	comp = (AVND_COMP *)ncshm_take_hdl(NCS_SERVICE_ID_AVND, tmr->opq_hdl);
	if (!comp)
		goto done;

	if (NCSCC_RC_SUCCESS == m_AVND_CHECK_FOR_STDBY_FOR_EXT_COMP(cb, comp->su->su_is_external))
		goto done;

	m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, comp, AVND_CKPT_COMP_CLC_REG_TMR);

	LOG_NO("Instantiation of '%s' failed", comp->name.value);
	LOG_NO("Reason: component registration timer expired");

	/* trigger the fsm with inst-fail event */
	rc = avnd_comp_clc_fsm_run(cb, comp, AVND_COMP_CLC_PRES_FSM_EV_INST_FAIL);

done:
	if (comp)
		ncshm_give_hdl(tmr->opq_hdl);
	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
  Name          : avnd_evt_tmr_clc_pxied_comp_inst
 
  Description   : This routine processes the proxied component instantiation
                  timer expiry. An instantiation failed event is generated for
                  the fsm.
 
  Arguments     : cb  - ptr to the AvND control block
                  evt - ptr to the AvND event
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : The proxied component instantiation timer is started when AMF
                  tries to instantiate a proxied component and no component has
                  registered "to proxy" this particular proxied.
******************************************************************************/
uns32 avnd_evt_tmr_clc_pxied_comp_inst_evh(AVND_CB *cb, AVND_EVT *evt)
{
	AVND_TMR_EVT *tmr = &evt->info.tmr;
	AVND_COMP *comp = 0;
	uns32 rc = NCSCC_RC_SUCCESS;

	TRACE_ENTER();

	/* retrieve the comp */
	comp = (AVND_COMP *)ncshm_take_hdl(NCS_SERVICE_ID_AVND, tmr->opq_hdl);
	if (!comp)
		goto done;

	if (NCSCC_RC_SUCCESS == m_AVND_CHECK_FOR_STDBY_FOR_EXT_COMP(cb, comp->su->su_is_external))
		goto done;

	m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, comp, AVND_CKPT_COMP_CLC_REG_TMR);

	/* trigger the fsm with inst-fail event */
	rc = avnd_comp_clc_fsm_run(cb, comp, AVND_COMP_CLC_PRES_FSM_EV_INST_FAIL);

done:
	if (comp)
		ncshm_give_hdl(tmr->opq_hdl);
	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
  Name          : avnd_evt_tmr_clc_pxied_comp_reg
 
  Description   : This routine processes the component registration timer 
                  (alias orph timer) expiry for proxied comp in orphaned state.
                  Component Error processing is done for this proxied component.
                  The defualt recovery being component failover.
 
  Arguments     : cb  - ptr to the AvND control block
                  evt - ptr to the AvND event
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : Proxied component registration timer is started when AMF finds 
                  that a component is not able to proxy for a proxied component 
                  anymore. It can be triggered by a proxy when it unregisters a 
                  proxied. It can also be triggered when AMF detects proxy failure.
******************************************************************************/
uns32 avnd_evt_tmr_clc_pxied_comp_reg_evh(AVND_CB *cb, AVND_EVT *evt)
{
	AVND_TMR_EVT *tmr = &evt->info.tmr;
	AVND_COMP *comp = 0;
	AVND_ERR_INFO err_info;
	uns32 rc = NCSCC_RC_SUCCESS;

	TRACE_ENTER();

	/* retrieve the comp */
	comp = (AVND_COMP *)ncshm_take_hdl(NCS_SERVICE_ID_AVND, tmr->opq_hdl);
	if (!comp)
		goto done;

	if (NCSCC_RC_SUCCESS == m_AVND_CHECK_FOR_STDBY_FOR_EXT_COMP(cb, comp->su->su_is_external))
		goto done;

	m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, comp, AVND_CKPT_COMP_ORPH_TMR);

	/* process comp failure */
	err_info.src = AVND_ERR_SRC_PXIED_REG_TIMEOUT;
	err_info.rcvr = SA_AMF_COMPONENT_FAILOVER;
	rc = avnd_err_process(cb, comp, &err_info);

done:
	if (comp)
		ncshm_give_hdl(tmr->opq_hdl);
	return rc;
	TRACE_LEAVE();
}

/****************************************************************************
  Name          : avnd_comp_clc_resp
 
  Description   : This is a callback routine that is invoked when either the 
                  command finishes execution (success or failure) or it times
                  out. It sends the corresponding event to the AvND thread.
 
  Arguments     : info - ptr to the cbk info
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
uns32 avnd_comp_clc_resp(NCS_OS_PROC_EXECUTE_TIMED_CB_INFO *info)
{
	AVND_CLC_EVT clc_evt;
	AVND_EVT *evt = 0;
	uns32 rc = NCSCC_RC_SUCCESS;

	if (!info)
		goto done;

	memset(&clc_evt, 0, sizeof(AVND_CLC_EVT));

	/* fill the clc-evt param */
	clc_evt.exec_ctxt = info->i_exec_hdl;
	clc_evt.exec_stat = info->exec_stat;

	/* create the event */
	evt = avnd_evt_create(avnd_cb, AVND_EVT_CLC_RESP, 0, 0, 0, (void *)&clc_evt, 0);
	if (!evt) {
		rc = NCSCC_RC_FAILURE;
		goto done;
	}

	/* send the event */
	rc = avnd_evt_send(avnd_cb, evt);

 done:
	/* free the event */
	if (NCSCC_RC_SUCCESS != rc && evt)
		avnd_evt_destroy(evt);

	return rc;
}

/****************************************************************************
  Name          : avnd_comp_clc_fsm_trigger
 
  Description   : This routine generates an asynchronous event for the 
                  component FSM. This is invoked by SU FSM.
 
  Arguments     : cb   - ptr to the AvND control block
                  comp - ptr to the comp
                  ev   - fsm event
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : An asynchronous event is required to avoid recursion.
******************************************************************************/
uns32 avnd_comp_clc_fsm_trigger(AVND_CB *cb, AVND_COMP *comp, AVND_COMP_CLC_PRES_FSM_EV ev)
{
	AVND_COMP_FSM_EVT comp_fsm_evt;
	AVND_EVT *evt = 0;
	uns32 rc = NCSCC_RC_SUCCESS;

	memset(&comp_fsm_evt, 0, sizeof(AVND_COMP_FSM_EVT));

	/* fill the comp-fsm-evt param */
	comp_fsm_evt.comp_name = comp->name;
	comp_fsm_evt.ev = ev;

	/* create the event */
	evt = avnd_evt_create(cb, AVND_EVT_COMP_PRES_FSM_EV, 0, 0, 0, 0, (void *)&comp_fsm_evt);
	if (!evt) {
		rc = NCSCC_RC_FAILURE;
		goto done;
	}

	/* send the event */
	rc = avnd_evt_send(cb, evt);

 done:
	/* free the event */
	if (NCSCC_RC_SUCCESS != rc && evt)
		avnd_evt_destroy(evt);

	return rc;
}

/**
 * Check if all components have been terminated, if so exit this process.
 * Only to be used in the AVND_TERM_STATE_OPENSAF_STOP state.
 * 
 * @param term_comp last terminated comp
 * 
 * @return int
 */
static int all_comps_terminated(void)
{
	AVND_COMP *comp;
	int all_comps_terminated = 1;

	assert(avnd_cb->term_state == AVND_TERM_STATE_OPENSAF_SHUTDOWN);

	/* Scan all components to see if we're done terminating all comps */
	comp = (AVND_COMP *)ncs_patricia_tree_getnext(&avnd_cb->compdb, (uns8 *)0);
	while (comp != 0) {
		if ((comp->pres != SA_AMF_PRESENCE_UNINSTANTIATED) &&
		    (comp->pres != SA_AMF_PRESENCE_INSTANTIATION_FAILED)) {
			all_comps_terminated = 0;
			TRACE("'%s' not terminated, pres.st=%u", comp->name.value, comp->pres);
		}

		comp = (AVND_COMP *) ncs_patricia_tree_getnext(&avnd_cb->compdb, (uns8 *)&comp->name);
	}

	return all_comps_terminated;
}

/****************************************************************************
  Name          : avnd_comp_clc_fsm_run
 
  Description   : This routine runs the component presence state FSM. It also 
                  generates events for the SU presence state FSM.
 
  Arguments     : cb   - ptr to the AvND control block
                  comp - ptr to the comp
                  ev   - fsm event
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
uns32 avnd_comp_clc_fsm_run(AVND_CB *cb, AVND_COMP *comp, AVND_COMP_CLC_PRES_FSM_EV ev)
{
	SaAmfPresenceStateT prv_st, final_st;
	uns32 rc = NCSCC_RC_SUCCESS;

	if (cb->term_state == AVND_TERM_STATE_OPENSAF_SHUTDOWN) {
		switch (ev) {
		case AVND_COMP_CLC_PRES_FSM_EV_CLEANUP_FAIL:
			LOG_ER("%s termination failed", comp->name.value);
			opensaf_reboot(avnd_cb->node_info.nodeId, (char *)avnd_cb->node_info.executionEnvironment.value,
					"Stopping OpenSAF failed");
			LOG_ER("Amfnd is exiting (due to comp term failed) to aid fast reboot");
			exit(0);
			break;
		case AVND_COMP_CLC_PRES_FSM_EV_CLEANUP_SUCC:
			m_AVND_COMP_PRES_STATE_SET(comp, SA_AMF_PRESENCE_UNINSTANTIATED);
			if (all_comps_terminated()) {
				LOG_NO("All AMF components terminated successfully, exiting");
				exit(0);
			}
			break;
		default:
			LOG_NO("%s: ignoring event=%u in state OPENSAF_SHUTDOWN", __FUNCTION__, ev);
			break;
		}

		goto done; /* skip execution of event handlers in this state */
	}

	/* get the prv presence state */
	prv_st = comp->pres;

	/* if already enabled, stop PM & AM */
	if (prv_st == SA_AMF_PRESENCE_INSTANTIATED &&
	    (ev == AVND_COMP_CLC_PRES_FSM_EV_TERM ||
	     ev == AVND_COMP_CLC_PRES_FSM_EV_CLEANUP || ev == AVND_COMP_CLC_PRES_FSM_EV_RESTART)) {
		/* stop all passive monitoring from this comp */
		avnd_comp_pm_finalize(cb, comp, comp->reg_hdl);

		/* stop the active monitoring */
		if (comp->is_am_en)
			rc = avnd_comp_clc_cmd_execute(cb, comp, AVND_COMP_CLC_CMD_TYPE_AMSTOP);
		if (NCSCC_RC_SUCCESS != rc)
			goto done;
	}

	m_AVND_LOG_COMP_FSM(AVND_LOG_COMP_FSM_ENTER, prv_st, ev, &comp->name, NCSFL_SEV_NOTICE);

	/* run the fsm */
	if (0 != avnd_comp_clc_fsm[prv_st - 1][ev - 1]) {
		rc = avnd_comp_clc_fsm[prv_st - 1][ev - 1] (cb, comp);
		if (NCSCC_RC_SUCCESS != rc)
			goto done;
	}

	/* get the final presence state */
	final_st = comp->pres;

	if (ev == AVND_COMP_CLC_PRES_FSM_EV_CLEANUP || ev == AVND_COMP_CLC_PRES_FSM_EV_TERM_SUCC) {
		/* we need to delete all curr_info, pxied will have cbk for cleanup */
		if (!m_AVND_COMP_TYPE_IS_PROXIED(comp)) {
			avnd_comp_curr_info_del(cb, comp);
		}
	}

	m_AVND_LOG_COMP_FSM(AVND_LOG_COMP_FSM_EXIT, final_st, 0, &comp->name, NCSFL_SEV_NOTICE);

	/* process state change */
	if (prv_st != final_st)
		rc = avnd_comp_clc_st_chng_prc(cb, comp, prv_st, final_st);

 done:
	return rc;
}

/****************************************************************************
  Name          : avnd_comp_clc_st_chng_prc
 
  Description   : This routine processes the change in  the presence state 
                  resulting due to running the component presence state FSM.
 
  Arguments     : cb       - ptr to the AvND control block
                  comp     - ptr to the comp
                  prv_st   - previous presence state
                  final_st - final presence state
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
uns32 avnd_comp_clc_st_chng_prc(AVND_CB *cb, AVND_COMP *comp, SaAmfPresenceStateT prv_st, SaAmfPresenceStateT final_st)
{
	AVND_SU_PRES_FSM_EV ev = AVND_SU_PRES_FSM_EV_MAX;
	AVND_COMP_CSI_REC *csi = 0;
	AVSV_PARAM_INFO param;
	NCS_BOOL is_en;
	uns32 rc = NCSCC_RC_SUCCESS;

	/* 
	 * Process state change
	 */

	/* Count the number of restarts if not due to admin restart*/
	if (SA_AMF_PRESENCE_RESTARTING == final_st && comp->admin_oper != SA_TRUE) {
		comp->err_info.restart_cnt++;

		m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, comp, AVND_CKPT_COMP_ERR_INFO);

		/* inform avd of the change in restart count */
		memset(&param, 0, sizeof(AVSV_PARAM_INFO));
		param.class_id = AVSV_SA_AMF_COMP;
		param.attr_id = saAmfCompRestartCount_ID;
		param.name = comp->name;
		param.act = AVSV_OBJ_OPR_MOD;
		*((uns32 *)param.value) = m_NCS_OS_HTONL(comp->err_info.restart_cnt);
		param.value_len = sizeof(uns32);

		rc = avnd_di_object_upd_send(cb, &param);
		if (NCSCC_RC_SUCCESS != rc)
			goto done;
	}
	/* reset the admin-oper flag to false */
	if (comp->admin_oper == SA_TRUE) {
		avnd_log(NCSFL_SEV_NOTICE, "admin oper flag reset");
		comp->admin_oper = SA_FALSE;
	}

	if ((SA_AMF_PRESENCE_INSTANTIATED == prv_st) && (SA_AMF_PRESENCE_UNINSTANTIATED != final_st)) {
		/* proxy comp need to unregister all its proxied before unregistering itself */
		if (m_AVND_COMP_TYPE_IS_PROXY(comp))
			rc = avnd_comp_proxy_unreg(cb, comp);
	}

	if ((SA_AMF_PRESENCE_RESTARTING == prv_st) && 
		((SA_AMF_PRESENCE_INSTANTIATION_FAILED == final_st) ||
		 (SA_AMF_PRESENCE_TERMINATION_FAILED == final_st))) {
		avnd_instfail_su_failover(cb, comp->su, comp);
	}

	if (comp->su->is_ncs == TRUE) {
		if(SA_AMF_PRESENCE_INSTANTIATION_FAILED == final_st) {
			LOG_ER("%s got Inst failed", comp->name.value);
			opensaf_reboot(avnd_cb->node_info.nodeId, (char *)avnd_cb->node_info.executionEnvironment.value,
					"NCS component Instantiation failed");
			LOG_ER("Amfnd is exiting (due to ncs comp inst failed) to aid fast reboot");
			exit(0);
		}
		if(SA_AMF_PRESENCE_TERMINATION_FAILED == final_st) {
			LOG_ER("%s got Term failed", comp->name.value);
			opensaf_reboot(avnd_cb->node_info.nodeId, (char *)avnd_cb->node_info.executionEnvironment.value,
					"NCS component Termination failed");
			LOG_ER("Amfnd is exiting (due to ncs comp term failed) to aid fast reboot");
			exit(0);
		}
	}

	/* pi comp in pi su */
	if (m_AVND_SU_IS_PREINSTANTIABLE(comp->su) && m_AVND_COMP_TYPE_IS_PREINSTANTIABLE(comp)) {
		/* instantiating -> instantiated */
		if ((SA_AMF_PRESENCE_INSTANTIATING == prv_st) && (SA_AMF_PRESENCE_INSTANTIATED == final_st)) {
			m_AVND_COMP_FAILED_RESET(comp);
			m_AVND_COMP_OPER_STATE_SET(comp, SA_AMF_OPERATIONAL_ENABLED);
			m_AVND_COMP_OPER_STATE_AVD_SYNC(cb, comp, rc);
			if (NCSCC_RC_SUCCESS != rc)
				goto done;
			m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, comp, AVND_CKPT_COMP_OPER_STATE);
		}

		/* instantiated -> terminating */
		if ((SA_AMF_PRESENCE_INSTANTIATED == prv_st) && (SA_AMF_PRESENCE_TERMINATING == final_st)) {
			m_AVND_COMP_OPER_STATE_SET(comp, SA_AMF_OPERATIONAL_DISABLED);
			m_AVND_COMP_OPER_STATE_AVD_SYNC(cb, comp, rc);
			if (NCSCC_RC_SUCCESS != rc)
				goto done;
			m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, comp, AVND_CKPT_COMP_OPER_STATE);
		}

		/* instantiating -> inst-failed */
		if ((SA_AMF_PRESENCE_INSTANTIATING == prv_st) && (SA_AMF_PRESENCE_INSTANTIATION_FAILED == final_st)) {
			/* instantiation failed.. log it */
			m_AVND_COMP_FAILED_SET(comp);
			m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, comp, AVND_CKPT_COMP_FLAG_CHANGE);
		}

		/* terminating -> term-failed */
		if ((SA_AMF_PRESENCE_TERMINATING == prv_st) && (SA_AMF_PRESENCE_TERMINATION_FAILED == final_st)) {
			/* termination failed.. log it */
		}

		/* restarting -> instantiated */
		if ((SA_AMF_PRESENCE_RESTARTING == prv_st) && (SA_AMF_PRESENCE_INSTANTIATED == final_st)) {
			/* reset the comp failed flag & set the oper state to enabled */
			if (m_AVND_COMP_IS_FAILED(comp)) {
				m_AVND_COMP_FAILED_RESET(comp);
				m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, comp, AVND_CKPT_COMP_FLAG_CHANGE);
				m_AVND_COMP_OPER_STATE_SET(comp, SA_AMF_OPERATIONAL_ENABLED);
				m_AVND_COMP_OPER_STATE_AVD_SYNC(cb, comp, rc);
				if (NCSCC_RC_SUCCESS != rc)
					goto done;
				m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, comp, AVND_CKPT_COMP_OPER_STATE);
			}

			/* reassign the comp-csis.. if su-restart recovery is not active */
			if (!m_AVND_SU_IS_RESTART(comp->su)) {
				rc = avnd_comp_csi_reassign(cb, comp);
				if (NCSCC_RC_SUCCESS != rc)
					goto done;
			}

			/* wild case */
			/* normal componets when they register, we can take it for granted
			 * that they are already instantiated. but proxied comp is reg first
			 * and inst later. so we should take care to enable the SU when proxied
			 * is instantiated and all other components are enabled. 
			 */
			/* if(m_AVND_COMP_TYPE_IS_PROXIED(comp)) */
			if (m_AVND_SU_OPER_STATE_IS_DISABLED(comp->su)) {
				m_AVND_SU_IS_ENABLED(comp->su, is_en);
				if (TRUE == is_en) {
					m_AVND_SU_OPER_STATE_SET(comp->su, SA_AMF_OPERATIONAL_ENABLED);
					m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, comp->su, AVND_CKPT_SU_OPER_STATE);
					rc = avnd_di_oper_send(cb, comp->su, 0);
					if (NCSCC_RC_SUCCESS != rc)
						goto done;
				}
			}
		}

		/* terminating -> uninstantiated */
		if ((SA_AMF_PRESENCE_TERMINATING == prv_st) && (SA_AMF_PRESENCE_UNINSTANTIATED == final_st)) {
			/* 
			 * If it's a failed comp, it's time to repair (re-instantiate) the 
			 * comp if no csis are assigned to it. If csis are assigned to the 
			 * comp, repair is done after su-si removal.
			 * If ADMN_TERM flag is set, it means that we are in the middle of 
			 * su termination, so we need not instantiate the comp, just reset
			 * the failed flag.
			 */
			if (m_AVND_COMP_IS_FAILED(comp) && !comp->csi_list.n_nodes && !m_AVND_SU_IS_ADMN_TERM(comp->su))
				rc = avnd_comp_clc_fsm_trigger(cb, comp, AVND_COMP_CLC_PRES_FSM_EV_INST);
			else if (m_AVND_COMP_IS_FAILED(comp) && !comp->csi_list.n_nodes) {
				m_AVND_COMP_FAILED_RESET(comp);	/*if we moved from restart -> term
								   due to admn operation */
				m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, comp, AVND_CKPT_COMP_FLAG_CHANGE);
			}
		}
	}

	/* npi comp in pi su */
	if (m_AVND_SU_IS_PREINSTANTIABLE(comp->su) && !m_AVND_COMP_TYPE_IS_PREINSTANTIABLE(comp)) {
		/* instantiating -> instantiated */
		if ((SA_AMF_PRESENCE_INSTANTIATING == prv_st) && (SA_AMF_PRESENCE_INSTANTIATED == final_st)) {
			/* csi-set succeeded.. generate csi-done indication */
			csi = m_AVND_CSI_REC_FROM_COMP_DLL_NODE_GET(m_NCS_DBLIST_FIND_FIRST(&comp->csi_list));
			assert(csi);
			rc = avnd_comp_csi_assign_done(cb, comp, m_AVND_COMP_IS_ALL_CSI(comp) ? 0 : csi);
			if (NCSCC_RC_SUCCESS != rc)
				goto done;
		}

		/* terminating -> uninstantiated */
		if ((SA_AMF_PRESENCE_TERMINATING == prv_st) && (SA_AMF_PRESENCE_UNINSTANTIATED == final_st)) {
			/* npi comps are enabled in uninstantiated state */
			m_AVND_COMP_OPER_STATE_SET(comp, SA_AMF_OPERATIONAL_ENABLED);
			m_AVND_COMP_OPER_STATE_AVD_SYNC(cb, comp, rc);
			if (NCSCC_RC_SUCCESS != rc)
				goto done;
			m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, comp, AVND_CKPT_COMP_OPER_STATE);

			if (!m_AVND_COMP_IS_FAILED(comp)) {
				/* csi-set / csi-rem succeeded.. generate csi-done indication */
				csi = m_AVND_CSI_REC_FROM_COMP_DLL_NODE_GET(m_NCS_DBLIST_FIND_FIRST(&comp->csi_list));
				assert(csi);
				if (m_AVND_COMP_CSI_CURR_ASSIGN_STATE_IS_ASSIGNING(csi))
					rc = avnd_comp_csi_assign_done(cb, comp,
								       m_AVND_COMP_IS_ALL_CSI(comp) ? 0 : csi);
				else if (m_AVND_COMP_CSI_CURR_ASSIGN_STATE_IS_REMOVING(csi))
					rc = avnd_comp_csi_remove_done(cb, comp,
								       m_AVND_COMP_IS_ALL_CSI(comp) ? 0 : csi);
				if (NCSCC_RC_SUCCESS != rc)
					goto done;
			} else {
				/* failed su is ready to take on si assignment.. inform avd */
				if (!comp->csi_list.n_nodes) {
					m_AVND_SU_IS_ENABLED(comp->su, is_en);
					if (TRUE == is_en) {
						m_AVND_SU_OPER_STATE_SET(comp->su,SA_AMF_OPERATIONAL_ENABLED);
						m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, comp->su, AVND_CKPT_SU_OPER_STATE);
						rc = avnd_di_oper_send(cb, comp->su, 0);
						if (NCSCC_RC_SUCCESS != rc)
							goto done;
					}
					m_AVND_COMP_FAILED_RESET(comp);
					m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, comp, AVND_CKPT_COMP_FLAG_CHANGE);
				}

			}
		}

		/* restarting -> instantiated */
		if ((SA_AMF_PRESENCE_RESTARTING == prv_st) && (SA_AMF_PRESENCE_INSTANTIATED == final_st)) {
			/* reset the comp failed flag & set the oper state to enabled */
			if (m_AVND_COMP_IS_FAILED(comp)) {
				m_AVND_COMP_FAILED_RESET(comp);
				m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, comp, AVND_CKPT_COMP_FLAG_CHANGE);
				m_AVND_COMP_OPER_STATE_SET(comp, SA_AMF_OPERATIONAL_ENABLED);
				m_AVND_COMP_OPER_STATE_AVD_SYNC(cb, comp, rc);
				if (NCSCC_RC_SUCCESS != rc)
					goto done;
				m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, comp, AVND_CKPT_COMP_OPER_STATE);
			}

			/* csi-set succeeded.. generate csi-done indication */
			csi = m_AVND_CSI_REC_FROM_COMP_DLL_NODE_GET(m_NCS_DBLIST_FIND_FIRST(&comp->csi_list));
			assert(csi);

			if (!m_AVND_COMP_CSI_CURR_ASSIGN_STATE_IS_UNASSIGNED(csi)) {
				rc = avnd_comp_csi_assign_done(cb, comp, m_AVND_COMP_IS_ALL_CSI(comp) ? 0 : csi);
				if (NCSCC_RC_SUCCESS != rc)
					goto done;
			}
		}

		/* instantiating -> instantiation failed */
		if ((SA_AMF_PRESENCE_INSTANTIATING == prv_st) && (SA_AMF_PRESENCE_INSTANTIATION_FAILED == final_st)) {
			m_AVND_COMP_FAILED_SET(comp);
			m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, comp, AVND_CKPT_COMP_FLAG_CHANGE);

			/* update comp oper state */
			m_AVND_COMP_OPER_STATE_SET(comp, SA_AMF_OPERATIONAL_DISABLED);
			m_AVND_COMP_OPER_STATE_AVD_SYNC(cb, comp, rc);
			m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, comp, AVND_CKPT_COMP_OPER_STATE);

			m_AVND_SU_FAILED_SET(comp->su);
			m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, comp->su, AVND_CKPT_SU_FLAG_CHANGE);
			/* csi-set Failed.. Respond failure for Su-Si */
			csi = m_AVND_CSI_REC_FROM_COMP_DLL_NODE_GET(m_NCS_DBLIST_FIND_FIRST(&comp->csi_list));
			assert(csi);
			avnd_di_susi_resp_send(cb, comp->su, m_AVND_COMP_IS_ALL_CSI(comp) ? 0 : csi->si);
		}

	}

	/* npi comp in npi su */
	if (!m_AVND_SU_IS_PREINSTANTIABLE(comp->su) && !m_AVND_COMP_TYPE_IS_PREINSTANTIABLE(comp)) {
		/* restarting -> instantiated */
		if ((SA_AMF_PRESENCE_RESTARTING == prv_st) && (SA_AMF_PRESENCE_INSTANTIATED == final_st)) {
			m_AVND_COMP_FAILED_RESET(comp);
			m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, comp, AVND_CKPT_COMP_FLAG_CHANGE);
			m_AVND_COMP_OPER_STATE_SET(comp, SA_AMF_OPERATIONAL_ENABLED);
			m_AVND_COMP_OPER_STATE_AVD_SYNC(cb, comp, rc);
			if (NCSCC_RC_SUCCESS != rc)
				goto done;
			m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, comp, AVND_CKPT_COMP_OPER_STATE);
		}

		/* terminating -> uninstantiated */
		if ((SA_AMF_PRESENCE_TERMINATING == prv_st) && (SA_AMF_PRESENCE_UNINSTANTIATED == final_st)) {
			/* npi comps are enabled in uninstantiated state */
			m_AVND_COMP_OPER_STATE_SET(comp, SA_AMF_OPERATIONAL_ENABLED);
			m_AVND_COMP_OPER_STATE_AVD_SYNC(cb, comp, rc);
			if (NCSCC_RC_SUCCESS != rc)
				goto done;
			m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, comp, AVND_CKPT_COMP_OPER_STATE);
		}

		/* Instantiating -> Instantiationfailed */
		if ((SA_AMF_PRESENCE_INSTANTIATING == prv_st) && (SA_AMF_PRESENCE_INSTANTIATION_FAILED == final_st)) {
			m_AVND_COMP_FAILED_SET(comp);
			m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, comp, AVND_CKPT_COMP_FLAG_CHANGE);
			m_AVND_COMP_OPER_STATE_SET(comp, SA_AMF_OPERATIONAL_DISABLED);
			m_AVND_COMP_OPER_STATE_AVD_SYNC(cb, comp, rc);
			if (NCSCC_RC_SUCCESS != rc)
				goto done;
			m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, comp, AVND_CKPT_COMP_OPER_STATE);
		}
	}

	/* when a comp moves from inst->orph, we need to delete the
	 ** healthcheck record and healthcheck pend callbcaks.
	 ** 
	 ** 
	 */

	if ((SA_AMF_PRESENCE_INSTANTIATED == prv_st)
	    && (SA_AMF_PRESENCE_ORPHANED == final_st)) {
		AVND_COMP_CBK *rec = 0, *curr_rec = 0;

		rec = comp->cbk_list;
		while (rec) {
			/* manage the list ptr's */
			curr_rec = rec;
			rec = rec->next;

			/* flush out the cbk related to health check */
			if (curr_rec->cbk_info->type == AVSV_AMF_HC) {
				/* delete the HC cbk */
				avnd_comp_cbq_rec_pop_and_del(cb, comp, curr_rec, TRUE);
				continue;
			}
		}		/* while */

		/* delete the HC records */
		avnd_comp_hc_rec_del_all(cb, comp);
	}

	/* when a comp moves from orph->inst, we need to process the
	 ** pending callbacks and events.
	 ** 
	 ** 
	 */

	if ((SA_AMF_PRESENCE_ORPHANED == prv_st)
	    && (SA_AMF_PRESENCE_INSTANTIATED == final_st)) {
		AVND_COMP_CBK *rec = 0, *curr_rec = 0;

		if (comp->pend_evt == AVND_COMP_CLC_PRES_FSM_EV_TERM)
			rc = avnd_comp_clc_fsm_trigger(cb, comp, AVND_COMP_CLC_PRES_FSM_EV_TERM);

		if (comp->pend_evt == AVND_COMP_CLC_PRES_FSM_EV_RESTART)
			rc = avnd_comp_clc_fsm_trigger(cb, comp, AVND_COMP_CLC_PRES_FSM_EV_RESTART);

		if ((comp->pend_evt != AVND_COMP_CLC_PRES_FSM_EV_TERM) &&
		    (comp->pend_evt != AVND_COMP_CLC_PRES_FSM_EV_RESTART)) {
			/* There are no pending events, lets proccess the callbacks */
			rec = comp->cbk_list;
			while (rec) {
				/* manage the list ptr's */
				curr_rec = rec;
				rec = rec->next;

				/* mds dest & hdl might have changed */
				curr_rec->dest = comp->reg_dest;
				curr_rec->timeout = comp->clc_info.cmds[AVND_COMP_CLC_CMD_TYPE_INSTANTIATE - 1].timeout;
				curr_rec->cbk_info->hdl = comp->reg_hdl;
				m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, curr_rec, AVND_CKPT_COMP_CBK_REC);

				/* send it */
				rc = avnd_comp_cbq_rec_send(cb, comp, curr_rec, TRUE);
				if (NCSCC_RC_SUCCESS != rc && curr_rec) {
					avnd_comp_cbq_rec_pop_and_del(cb, comp, curr_rec, TRUE);
				}
			}	/* while loop */
		}
	}

	/* inform avd of the change in presence state for all 
	 * Component Presence state trasitions except 
	 * INSTANTIATED -> ORPHANED -> INSTANTIATED */
	if ((SA_AMF_PRESENCE_ORPHANED == final_st) ||
			((SA_AMF_PRESENCE_INSTANTIATED == final_st) &&
			 (SA_AMF_PRESENCE_ORPHANED == prv_st))) {
		TRACE("'%s' presence state changed from '%u' => '%u'",
				comp->name.value, prv_st, final_st);
	} else {
		memset(&param, 0, sizeof(AVSV_PARAM_INFO));
		param.class_id = AVSV_SA_AMF_COMP;
		param.attr_id = saAmfCompPresenceState_ID;
		param.name = comp->name;
		param.act = AVSV_OBJ_OPR_MOD;
		*((uns32 *)param.value) = m_NCS_OS_HTONL(comp->pres);
		param.value_len = sizeof(uns32);

		rc = avnd_di_object_upd_send(cb, &param);
		if (NCSCC_RC_SUCCESS != rc)
			goto done;
	}
	/* 
	 * Trigger the SU FSM.
	 * Only PI comps in a PI SU send event to the SU FSM.
	 * All NPI comps in an NPI SU send event to the SU FSM.
	 */
	if (m_AVND_SU_IS_PREINSTANTIABLE(comp->su) && !m_AVND_COMP_TYPE_IS_PREINSTANTIABLE(comp)) {

		if (SA_AMF_PRESENCE_INSTANTIATION_FAILED == final_st)
			ev = AVND_SU_PRES_FSM_EV_COMP_INST_FAIL;
		else if (SA_AMF_PRESENCE_TERMINATION_FAILED == final_st)
			ev = AVND_SU_PRES_FSM_EV_COMP_TERM_FAIL;
		else if ((SA_AMF_PRESENCE_TERMINATING == final_st) && (comp->su->pres == SA_AMF_PRESENCE_RESTARTING))
			ev = AVND_SU_PRES_FSM_EV_COMP_TERMINATING;
	}

	if ((m_AVND_SU_IS_PREINSTANTIABLE(comp->su) &&
	     m_AVND_COMP_TYPE_IS_PREINSTANTIABLE(comp)) || !m_AVND_SU_IS_PREINSTANTIABLE(comp->su)) {
		if (SA_AMF_PRESENCE_UNINSTANTIATED == final_st)
			ev = AVND_SU_PRES_FSM_EV_COMP_UNINSTANTIATED;
		else if (SA_AMF_PRESENCE_INSTANTIATED == final_st && SA_AMF_PRESENCE_ORPHANED != prv_st)
			ev = AVND_SU_PRES_FSM_EV_COMP_INSTANTIATED;
		else if (SA_AMF_PRESENCE_INSTANTIATION_FAILED == final_st)
			ev = AVND_SU_PRES_FSM_EV_COMP_INST_FAIL;
		else if (SA_AMF_PRESENCE_TERMINATION_FAILED == final_st)
			ev = AVND_SU_PRES_FSM_EV_COMP_TERM_FAIL;
		else if (SA_AMF_PRESENCE_RESTARTING == final_st)
			ev = AVND_SU_PRES_FSM_EV_COMP_RESTARTING;
		else if (SA_AMF_PRESENCE_TERMINATING == final_st)
			ev = AVND_SU_PRES_FSM_EV_COMP_TERMINATING;
	}

	if (AVND_SU_PRES_FSM_EV_MAX != ev)
		rc = avnd_su_pres_fsm_run(cb, comp->su, comp, ev);
	if (NCSCC_RC_SUCCESS != rc)
		goto done;

	/* if enabled - Start AM */
	if (comp->is_am_en && SA_AMF_PRESENCE_INSTANTIATED == final_st && SA_AMF_PRESENCE_ORPHANED != prv_st)
		rc = avnd_comp_am_start(cb, comp);

 done:
	return rc;
}

/****************************************************************************
  Name          : avnd_comp_clc_uninst_inst_hdler
 
  Description   : This routine processes the `instantiate` event in 
                  `uninstantiated` state.
 
  Arguments     : cb   - ptr to the AvND control block
                  comp - ptr to the comp
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
uns32 avnd_comp_clc_uninst_inst_hdler(AVND_CB *cb, AVND_COMP *comp)
{
	uns32 rc = NCSCC_RC_SUCCESS;

	TRACE_ENTER2("%s", comp->name.value);

	/*if proxied component check whether the proxy exists, if so continue 
	   instantiating by calling the proxied callback. else start timer and 
	   wait for inst timeout duration */
	if (m_AVND_COMP_TYPE_IS_PROXIED(comp)) {
		if (comp->pxy_comp != NULL) {
			if (m_AVND_COMP_TYPE_IS_PREINSTANTIABLE(comp))
				/* call the proxied instantiate callback   */
				rc = avnd_comp_cbk_send(cb, comp, AVSV_AMF_PXIED_COMP_INST, 0, 0);
			else
				/* do a csi set with active ha state */
				rc = avnd_comp_cbk_send(cb, comp, AVSV_AMF_CSI_SET, 0, 0);
			if (NCSCC_RC_SUCCESS == rc) {
				/* increment the retry count */
				comp->clc_info.inst_retry_cnt++;
				m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, comp, AVND_CKPT_COMP_INST_RETRY_CNT);
			}
		} else {
			m_AVND_TMR_PXIED_COMP_INST_START(cb, *comp, rc);	/* start a timer for proxied instantiating timeout duration */
			m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, comp, AVND_CKPT_COMP_CLC_REG_TMR);
		}

		/* transition to 'instantiating' state */
		m_AVND_COMP_PRES_STATE_SET(comp, SA_AMF_PRESENCE_INSTANTIATING);
		m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, comp, AVND_CKPT_COMP_PRES_STATE);

		goto done;
	}

	/* instantiate the comp */
	rc = avnd_comp_clc_cmd_execute(cb, comp, AVND_COMP_CLC_CMD_TYPE_INSTANTIATE);

	if (NCSCC_RC_SUCCESS == rc) {
		/* timestamp the start of this instantiation phase */
		m_GET_TIME_STAMP(comp->clc_info.inst_cmd_ts);

		m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, comp, AVND_CKPT_COMP_INST_CMD_TS);

		/* increment the retry count */
		comp->clc_info.inst_retry_cnt++;

		m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, comp, AVND_CKPT_COMP_INST_RETRY_CNT);

		/* transition to 'instantiating' state */
		m_AVND_COMP_PRES_STATE_SET(comp, SA_AMF_PRESENCE_INSTANTIATING);
		m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, comp, AVND_CKPT_COMP_PRES_STATE);
	}

done:
	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
  Name          : avnd_comp_clc_insting_inst_hdler 
 
  Description   : This routine processes the `instantiate ` event in 
                  `instantiating` state.
 
  Arguments     : cb   - ptr to the AvND control block
                  comp - ptr to the comp
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : This handler will only be called for proxied component.
******************************************************************************/
uns32 avnd_comp_clc_insting_inst_hdler(AVND_CB *cb, AVND_COMP *comp)
{
	uns32 rc = NCSCC_RC_SUCCESS;

	/* increment the retry count */
	comp->clc_info.inst_retry_cnt++;

	m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, comp, AVND_CKPT_COMP_INST_RETRY_CNT);

	if (m_AVND_COMP_TYPE_IS_PREINSTANTIABLE(comp))
		/* call the proxied instantiate callback, start a timer for callback responce */
		rc = avnd_comp_cbk_send(cb, comp, AVSV_AMF_PXIED_COMP_INST, 0, 0);
	else
		/* do a csi set with active ha state */
		rc = avnd_comp_cbk_send(cb, comp, AVSV_AMF_CSI_SET, 0, 0);

	/* no state transition */

	return rc;
}

/****************************************************************************
  Name          : avnd_comp_clc_insting_instsucc_hdler
 
  Description   : This routine processes the `instantiate success` event in 
                  `instantiating` state.
 
  Arguments     : cb   - ptr to the AvND control block
                  comp - ptr to the comp
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
uns32 avnd_comp_clc_insting_instsucc_hdler(AVND_CB *cb, AVND_COMP *comp)
{
	uns32 rc = NCSCC_RC_SUCCESS;

	/* stop the reg tmr */
	if (m_AVND_TMR_IS_ACTIVE(comp->clc_info.clc_reg_tmr))
		m_AVND_TMR_COMP_REG_STOP(cb, *comp);

	/* reset the retry count */
	comp->clc_info.inst_retry_cnt = 0;

	/* transition to 'instantiated' state */
	m_AVND_COMP_PRES_STATE_SET(comp, SA_AMF_PRESENCE_INSTANTIATED);
	m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, comp, AVND_CKPT_COMP_CONFIG);

	return rc;
}

/****************************************************************************
  Name          : avnd_comp_clc_xxxing_instfail_hdler
 
  Description   : This routine processes the `instantiate fail` event in 
                  `instantiating` or 'restarting' states.
 
  Arguments     : cb   - ptr to the AvND control block
                  comp - ptr to the comp
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
uns32 avnd_comp_clc_xxxing_instfail_hdler(AVND_CB *cb, AVND_COMP *comp)
{
	uns32 rc = NCSCC_RC_SUCCESS;

	/* cleanup the comp */
	if (m_AVND_COMP_TYPE_IS_PROXIED(comp) && comp->pxy_comp != 0)
		rc = avnd_comp_cbk_send(cb, comp, AVSV_AMF_PXIED_COMP_CLEAN, 0, 0);
	else
		rc = avnd_comp_clc_cmd_execute(cb, comp, AVND_COMP_CLC_CMD_TYPE_CLEANUP);

	if (NCSCC_RC_SUCCESS == rc) {
		/* reset the comp-reg & instantiate params */
		if (!m_AVND_COMP_TYPE_IS_PROXIED(comp))
			m_AVND_COMP_REG_PARAM_RESET(cb, comp);
		m_AVND_COMP_CLC_INST_PARAM_RESET(comp);

		m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, comp, AVND_CKPT_COMP_CONFIG);

		/* delete hc-list, cbk-list, pg-list & pm-list */
		avnd_comp_hc_rec_del_all(cb, comp);
		avnd_comp_cbq_del(cb, comp, TRUE);

		/* re-using the funtion to stop all PM started by this comp */
		avnd_comp_pm_finalize(cb, comp, comp->reg_hdl);
		avnd_comp_pm_rec_del_all(cb, comp);	/*if at all anythnig is left behind */

		/* no state transition */
	}

	return rc;
}

/****************************************************************************
  Name          : avnd_comp_clc_insting_term_hdler
 
  Description   : This routine processes the `terminate` event in 
                  `instantiating` state.
 
  Arguments     : cb   - ptr to the AvND control block
                  comp - ptr to the comp
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
uns32 avnd_comp_clc_insting_term_hdler(AVND_CB *cb, AVND_COMP *comp)
{
	uns32 rc = NCSCC_RC_SUCCESS;

	/* as the comp has not fully instantiated, it is cleaned up */
	rc = avnd_comp_clc_cmd_execute(cb, comp, AVND_COMP_CLC_CMD_TYPE_CLEANUP);
	if (NCSCC_RC_SUCCESS == rc) {
		/* reset the comp-reg & instantiate params */
		m_AVND_COMP_REG_PARAM_RESET(cb, comp);
		m_AVND_COMP_CLC_INST_PARAM_RESET(comp);

		m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, comp, AVND_CKPT_COMP_CONFIG);

		/* delete hc-list, cbk-list, pg-list & pm-list */
		avnd_comp_hc_rec_del_all(cb, comp);
		avnd_comp_cbq_del(cb, comp, TRUE);

		/* re-using the funtion to stop all PM started by this comp */
		avnd_comp_pm_finalize(cb, comp, comp->reg_hdl);
		avnd_comp_pm_rec_del_all(cb, comp);	/*if at all anythnig is left behind */

		/* transition to 'terminating' state */
		m_AVND_COMP_PRES_STATE_SET(comp, SA_AMF_PRESENCE_TERMINATING);
		m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, comp, AVND_CKPT_COMP_PRES_STATE);
	}

	return rc;
}

/****************************************************************************
  Name          : avnd_comp_clc_insting_clean_hdler
 
  Description   : This routine processes the `cleanup` event in 
                  `instantiating` state.
 
  Arguments     : cb   - ptr to the AvND control block
                  comp - ptr to the comp
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
uns32 avnd_comp_clc_insting_clean_hdler(AVND_CB *cb, AVND_COMP *comp)
{
	uns32 rc = NCSCC_RC_SUCCESS;

	/* cleanup the comp */
	rc = avnd_comp_clc_cmd_execute(cb, comp, AVND_COMP_CLC_CMD_TYPE_CLEANUP);
	if (NCSCC_RC_SUCCESS == rc) {
		/* reset the comp-reg & instantiate params */
		m_AVND_COMP_REG_PARAM_RESET(cb, comp);
		m_AVND_COMP_CLC_INST_PARAM_RESET(comp);

		m_AVND_COMP_TERM_FAIL_RESET(comp);

		/* transition to 'terminating' state */
		m_AVND_COMP_PRES_STATE_SET(comp, SA_AMF_PRESENCE_TERMINATING);

		m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, comp, AVND_CKPT_COMP_CONFIG);
	}

	return rc;
}

/****************************************************************************
  Name          : avnd_comp_clc_xxxing_cleansucc_hdler
 
  Description   : This routine processes the `cleanup success` event in 
                  `instantiating` or 'restarting' states.
 
  Arguments     : cb   - ptr to the AvND control block
                  comp - ptr to the comp
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
uns32 avnd_comp_clc_xxxing_cleansucc_hdler(AVND_CB *cb, AVND_COMP *comp)
{
	AVND_COMP_CLC_INFO *clc_info = &comp->clc_info;
	uns32 rc = NCSCC_RC_SUCCESS;

	if ((clc_info->inst_retry_cnt < clc_info->inst_retry_max) &&
	    (AVND_COMP_INST_EXIT_CODE_NO_RETRY != clc_info->inst_code_rcvd)) {
		/* => keep retrying */
		/* instantiate the comp */
		if (m_AVND_COMP_TYPE_IS_PROXIED(comp)) {
			if (comp->pxy_comp != NULL) {
				if (m_AVND_COMP_TYPE_IS_PREINSTANTIABLE(comp))
					/* call the proxied instantiate callback   */
					rc = avnd_comp_cbk_send(cb, comp, AVSV_AMF_PXIED_COMP_INST, 0, 0);
				else
					/* do a csi set with active ha state */
					rc = avnd_comp_cbk_send(cb, comp, AVSV_AMF_CSI_SET, 0, 0);
			} else {
				/* stop the reg tmr, we are reusing the reg timer here */
				if (m_AVND_TMR_IS_ACTIVE(comp->clc_info.clc_reg_tmr))
					m_AVND_TMR_PXIED_COMP_INST_STOP(cb, *comp);

				/* start a timer for proxied instantiating timeout duration */
				m_AVND_TMR_PXIED_COMP_INST_START(cb, *comp, rc);
				m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, comp, AVND_CKPT_COMP_CLC_REG_TMR);
			}
		} else
			rc = avnd_comp_clc_cmd_execute(cb, comp, AVND_COMP_CLC_CMD_TYPE_INSTANTIATE);

		if (NCSCC_RC_SUCCESS == rc) {
			/* timestamp the start of this instantiation phase */
			m_GET_TIME_STAMP(clc_info->inst_cmd_ts);

			m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, comp, AVND_CKPT_COMP_INST_CMD_TS);

			/* increment the retry count */
			clc_info->inst_retry_cnt++;
			m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, comp, AVND_CKPT_COMP_INST_RETRY_CNT);
		}
	} else {
		/* stop the inst timer, inst timer might still be running */
		if (m_AVND_COMP_TYPE_IS_PROXIED(comp) && m_AVND_TMR_IS_ACTIVE(comp->clc_info.clc_reg_tmr)) {
			m_AVND_TMR_PXIED_COMP_INST_STOP(cb, *comp);
			m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, comp, AVND_CKPT_COMP_CLC_REG_TMR);
		}
		/* => retries over... transition to inst-failed state */
		m_AVND_COMP_PRES_STATE_SET(comp, SA_AMF_PRESENCE_INSTANTIATION_FAILED);
		m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, comp, AVND_CKPT_COMP_PRES_STATE);
	}

	return rc;
}

/****************************************************************************
  Name          : avnd_comp_clc_insting_cleanfail_hdler
 
  Description   : This routine processes the `cleanup fail` event in 
                  `instantiating` state.
 
  Arguments     : cb   - ptr to the AvND control block
                  comp - ptr to the comp
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
uns32 avnd_comp_clc_insting_cleanfail_hdler(AVND_CB *cb, AVND_COMP *comp)
{
	uns32 rc = NCSCC_RC_SUCCESS;

	/* nothing can be done now.. just transition to inst-failed state */
	m_AVND_COMP_PRES_STATE_SET(comp, SA_AMF_PRESENCE_INSTANTIATION_FAILED);
	m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, comp, AVND_CKPT_COMP_PRES_STATE);

	return rc;
}

/****************************************************************************
  Name          : avnd_comp_clc_insting_restart_hdler
 
  Description   : This routine processes the `restart` event in 
                  `instantiating` state.
 
  Arguments     : cb   - ptr to the AvND control block
                  comp - ptr to the comp
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
uns32 avnd_comp_clc_insting_restart_hdler(AVND_CB *cb, AVND_COMP *comp)
{
	uns32 rc = NCSCC_RC_SUCCESS;

	/* cleanup the comp */
	rc = avnd_comp_clc_cmd_execute(cb, comp, AVND_COMP_CLC_CMD_TYPE_CLEANUP);
	if (NCSCC_RC_SUCCESS == rc) {
		/* reset the comp-reg & instantiate params */
		m_AVND_COMP_REG_PARAM_RESET(cb, comp);
		m_AVND_COMP_CLC_INST_PARAM_RESET(comp);

		/* transition to 'restarting' state */
		m_AVND_COMP_PRES_STATE_SET(comp, SA_AMF_PRESENCE_RESTARTING);

		m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, comp, AVND_CKPT_COMP_CONFIG);
	}

	return rc;
}

/****************************************************************************
  Name          : avnd_comp_clc_inst_term_hdler
 
  Description   : This routine processes the `terminate` event in 
                  `instantiated` state.
 
  Arguments     : cb   - ptr to the AvND control block
                  comp - ptr to the comp
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
uns32 avnd_comp_clc_inst_term_hdler(AVND_CB *cb, AVND_COMP *comp)
{
	uns32 rc = NCSCC_RC_SUCCESS;

	/* terminate the comp */
	if (m_AVND_COMP_TYPE_IS_PROXIED(comp) && !m_AVND_COMP_TYPE_IS_PREINSTANTIABLE(comp))
		/* invoke csi remove callback */
		rc = avnd_comp_cbk_send(cb, comp, AVSV_AMF_CSI_REM, 0, 0);
	else if (m_AVND_COMP_TYPE_IS_SAAWARE(comp) && (!m_AVND_COMP_IS_REG(comp))) {
		rc = avnd_comp_clc_cmd_execute(cb, comp, AVND_COMP_CLC_CMD_TYPE_CLEANUP);
		m_AVND_COMP_REG_PARAM_RESET(cb, comp);

		m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, comp, AVND_CKPT_COMP_CONFIG);

		/* delete hc-list, cbk-list, pg-list & pm-list */
		avnd_comp_hc_rec_del_all(cb, comp);
		avnd_comp_cbq_del(cb, comp, TRUE);

		/* re-using the funtion to stop all PM started by this comp */
		avnd_comp_pm_finalize(cb, comp, comp->reg_hdl);
		avnd_comp_pm_rec_del_all(cb, comp);	/*if at all anythnig is left behind */

	} else if (m_AVND_COMP_TYPE_IS_SAAWARE(comp) ||
		   (m_AVND_COMP_TYPE_IS_PROXIED(comp) && m_AVND_COMP_TYPE_IS_PREINSTANTIABLE(comp))) {
		/* invoke terminate callback */
		rc = avnd_comp_cbk_send(cb, comp, AVSV_AMF_COMP_TERM, 0, 0);
	} else {
		/* invoke terminate command */
		rc = avnd_comp_clc_cmd_execute(cb, comp, AVND_COMP_CLC_CMD_TYPE_TERMINATE);
		m_AVND_COMP_REG_PARAM_RESET(cb, comp);
		m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, comp, AVND_CKPT_COMP_CONFIG);
	}

	if (NCSCC_RC_SUCCESS == rc) {
		/* reset the comp-reg & instantiate params */
		m_AVND_COMP_CLC_INST_PARAM_RESET(comp);

		/* transition to 'terminating' state */
		m_AVND_COMP_PRES_STATE_SET(comp, SA_AMF_PRESENCE_TERMINATING);
		m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, comp, AVND_CKPT_COMP_CONFIG);
	}

	return rc;
}

/****************************************************************************
  Name          : avnd_comp_clc_inst_clean_hdler
 
  Description   : This routine processes the `cleanup` event in 
                  `instantiated` state.
 
  Arguments     : cb   - ptr to the AvND control block
                  comp - ptr to the comp
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
uns32 avnd_comp_clc_inst_clean_hdler(AVND_CB *cb, AVND_COMP *comp)
{
	uns32 rc = NCSCC_RC_SUCCESS;

	if (m_AVND_COMP_TYPE_IS_PROXIED(comp)) {
		avnd_comp_cbq_del(cb, comp, TRUE);
		/* call the cleanup callback */
		rc = avnd_comp_cbk_send(cb, comp, AVSV_AMF_PXIED_COMP_CLEAN, 0, 0);
	} else
		/* cleanup the comp */
		rc = avnd_comp_clc_cmd_execute(cb, comp, AVND_COMP_CLC_CMD_TYPE_CLEANUP);
	if (NCSCC_RC_SUCCESS == rc) {
		/* reset the comp-reg & instantiate params */
		if (!m_AVND_COMP_TYPE_IS_PROXIED(comp))
			m_AVND_COMP_REG_PARAM_RESET(cb, comp);
		m_AVND_COMP_CLC_INST_PARAM_RESET(comp);

		m_AVND_COMP_TERM_FAIL_RESET(comp);
		/* transition to 'terminating' state */
		m_AVND_COMP_PRES_STATE_SET(comp, SA_AMF_PRESENCE_TERMINATING);

		m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, comp, AVND_CKPT_COMP_CONFIG);
	}

	return rc;
}

/****************************************************************************
  Name          : avnd_comp_clc_inst_restart_hdler
 
  Description   : This routine processes the `restart` event in 
                  `instantiated` state.
 
  Arguments     : cb   - ptr to the AvND control block
                  comp - ptr to the comp
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : Unregistered instantiated comp's will also be restarted
                  while doing su restart. 
******************************************************************************/
uns32 avnd_comp_clc_inst_restart_hdler(AVND_CB *cb, AVND_COMP *comp)
{
	uns32 rc = NCSCC_RC_SUCCESS;

	/* terminate / cleanup the comp */
	if (m_AVND_COMP_IS_FAILED(comp) && m_AVND_COMP_TYPE_IS_PROXIED(comp))
		rc = avnd_comp_cbk_send(cb, comp, AVSV_AMF_PXIED_COMP_CLEAN, 0, 0);
	else if (m_AVND_COMP_IS_FAILED(comp)) {
		rc = avnd_comp_clc_cmd_execute(cb, comp, AVND_COMP_CLC_CMD_TYPE_CLEANUP);
		m_AVND_COMP_REG_PARAM_RESET(cb, comp);
		m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, comp, AVND_CKPT_COMP_CONFIG);
	} else {
		if (m_AVND_COMP_TYPE_IS_PROXIED(comp) && !m_AVND_COMP_TYPE_IS_PREINSTANTIABLE(comp))
			/* invoke csi set callback with active ha state */
			rc = avnd_comp_cbk_send(cb, comp, AVSV_AMF_CSI_REM, 0, 0);
		else if (m_AVND_COMP_TYPE_IS_SAAWARE(comp) && (!m_AVND_COMP_IS_REG(comp))) {
			rc = avnd_comp_clc_cmd_execute(cb, comp, AVND_COMP_CLC_CMD_TYPE_CLEANUP);
			m_AVND_COMP_REG_PARAM_RESET(cb, comp);

			m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, comp, AVND_CKPT_COMP_CONFIG);

			/* delete hc-list, cbk-list, pg-list & pm-list */
			avnd_comp_hc_rec_del_all(cb, comp);
			avnd_comp_cbq_del(cb, comp, TRUE);

			/* re-using the funtion to stop all PM started by this comp */
			avnd_comp_pm_finalize(cb, comp, comp->reg_hdl);
			avnd_comp_pm_rec_del_all(cb, comp);	/*if at all anythnig is left behind */
		} else if (m_AVND_COMP_TYPE_IS_SAAWARE(comp) ||
			   (m_AVND_COMP_TYPE_IS_PROXIED(comp) && m_AVND_COMP_TYPE_IS_PREINSTANTIABLE(comp)))
			/* invoke terminate callback */
			rc = avnd_comp_cbk_send(cb, comp, AVSV_AMF_COMP_TERM, 0, 0);
		else {
			/* invoke terminate command */
			rc = avnd_comp_clc_cmd_execute(cb, comp, AVND_COMP_CLC_CMD_TYPE_TERMINATE);
			m_AVND_COMP_REG_PARAM_RESET(cb, comp);
			m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, comp, AVND_CKPT_COMP_CONFIG);
		}
	}

	if (NCSCC_RC_SUCCESS == rc) {
		/* reset the comp-reg & instantiate params */

		/* For proxied components, registration is treated as 
		   "some one is proxying for him", so we need not reset the
		   reg parameters 
		 */

		m_AVND_COMP_CLC_INST_PARAM_RESET(comp);

		/* transition to 'restarting' state */
		m_AVND_COMP_PRES_STATE_SET(comp, SA_AMF_PRESENCE_RESTARTING);
		m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, comp, AVND_CKPT_COMP_CONFIG);
	}

	return rc;
}

/****************************************************************************
  Name          : avnd_comp_clc_inst_orph_hdler
 
  Description   : This routine processes the `orph` event in 
                  `instantiated` state.
 
  Arguments     : cb   - ptr to the AvND control block
                  comp - ptr to the comp
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : Once orphaned, some other proxy can register the proxied
                   So the registered handle's has no value. So cleanup all
                   records associated with the registered handle (eg HC,..) 
******************************************************************************/
uns32 avnd_comp_clc_inst_orph_hdler(AVND_CB *cb, AVND_COMP *comp)
{
	uns32 rc = NCSCC_RC_SUCCESS;

	/* start the orphaned timer */
	m_AVND_TMR_PXIED_COMP_REG_START(cb, *comp, rc);

	m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, comp, AVND_CKPT_COMP_ORPH_TMR);

	if (NCSCC_RC_SUCCESS == rc) {
		m_AVND_COMP_PRES_STATE_SET(comp, SA_AMF_PRESENCE_ORPHANED);
		m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, comp, AVND_CKPT_COMP_PRES_STATE);
	}

	return rc;
}

/****************************************************************************
  Name          : avnd_comp_clc_terming_termsucc_hdler
 
  Description   : This routine processes the `terminate success` event in 
                  `terminating` state.
 
  Arguments     : cb   - ptr to the AvND control block
                  comp - ptr to the comp
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
uns32 avnd_comp_clc_terming_termsucc_hdler(AVND_CB *cb, AVND_COMP *comp)
{
	uns32 rc = NCSCC_RC_SUCCESS;

	/* just transition to 'uninstantiated' state */
	m_AVND_COMP_PRES_STATE_SET(comp, SA_AMF_PRESENCE_UNINSTANTIATED);
	m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, comp, AVND_CKPT_COMP_PRES_STATE);

	/* reset the comp-reg & instantiate params */
	if (!m_AVND_COMP_TYPE_IS_PROXIED(comp)) {
		m_AVND_COMP_REG_PARAM_RESET(cb, comp);
		m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, comp, AVND_CKPT_COMP_CONFIG);
	}

	return rc;
}

/****************************************************************************
  Name          : avnd_comp_clc_terming_termfail_hdler
 
  Description   : This routine processes the `terminate fail` event in 
                  `terminating` state.
 
  Arguments     : cb   - ptr to the AvND control block
                  comp - ptr to the comp
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
uns32 avnd_comp_clc_terming_termfail_hdler(AVND_CB *cb, AVND_COMP *comp)
{
	uns32 rc = NCSCC_RC_SUCCESS;

	if (m_AVND_COMP_TYPE_IS_PROXIED(comp))
		avnd_comp_cbq_del(cb, comp, TRUE);

	/* cleanup the comp */
	if (m_AVND_COMP_TYPE_IS_PROXIED(comp) && comp->pxy_comp != 0)
		rc = avnd_comp_cbk_send(cb, comp, AVSV_AMF_PXIED_COMP_CLEAN, 0, 0);
	else
		rc = avnd_comp_clc_cmd_execute(cb, comp, AVND_COMP_CLC_CMD_TYPE_CLEANUP);

	m_AVND_COMP_TERM_FAIL_RESET(comp);
	m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, comp, AVND_CKPT_COMP_FLAG_CHANGE);

	/* reset the comp-reg & instantiate params */
	if (!m_AVND_COMP_TYPE_IS_PROXIED(comp)) {
		m_AVND_COMP_REG_PARAM_RESET(cb, comp);
		m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, comp, AVND_CKPT_COMP_CONFIG);
	}

	return rc;
}

/****************************************************************************
  Name          : avnd_comp_clc_terming_cleansucc_hdler
 
  Description   : This routine processes the `cleanup success` event in 
                  `terminating` state.
 
  Arguments     : cb   - ptr to the AvND control block
                  comp - ptr to the comp
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
uns32 avnd_comp_clc_terming_cleansucc_hdler(AVND_CB *cb, AVND_COMP *comp)
{
	uns32 rc = NCSCC_RC_SUCCESS;

	/* just transition to 'uninstantiated' state */
	m_AVND_COMP_PRES_STATE_SET(comp, SA_AMF_PRESENCE_UNINSTANTIATED);
	m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, comp, AVND_CKPT_COMP_PRES_STATE);

	/* reset the comp-reg & instantiate params */
	if (!m_AVND_COMP_TYPE_IS_PROXIED(comp)) {
		m_AVND_COMP_REG_PARAM_RESET(cb, comp);
		m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, comp, AVND_CKPT_COMP_CONFIG);
	}
	return rc;
}

/****************************************************************************
  Name          : avnd_comp_clc_terming_cleanfail_hdler
 
  Description   : This routine processes the `cleanup fail` event in 
                  `terminating` state.
 
  Arguments     : cb   - ptr to the AvND control block
                  comp - ptr to the comp
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
uns32 avnd_comp_clc_terming_cleanfail_hdler(AVND_CB *cb, AVND_COMP *comp)
{
	uns32 rc = NCSCC_RC_SUCCESS;

	/* just transition to 'term-failed' state */
	m_AVND_COMP_PRES_STATE_SET(comp, SA_AMF_PRESENCE_TERMINATION_FAILED);
	m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, comp, AVND_CKPT_COMP_PRES_STATE);

	return rc;
}

/****************************************************************************
  Name          : avnd_comp_clc_restart_instsucc_hdler
 
  Description   : This routine processes the `instantiate success` event in 
                  `restarting` state.
 
  Arguments     : cb   - ptr to the AvND control block
                  comp - ptr to the comp
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
uns32 avnd_comp_clc_restart_instsucc_hdler(AVND_CB *cb, AVND_COMP *comp)
{
	uns32 rc = NCSCC_RC_SUCCESS;

	/* stop the reg tmr */
	if (m_AVND_TMR_IS_ACTIVE(comp->clc_info.clc_reg_tmr)) {
		m_AVND_TMR_COMP_REG_STOP(cb, *comp);
		m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, comp, AVND_CKPT_COMP_CLC_REG_TMR);
	}

	/* just transition back to 'instantiated' state */
	m_AVND_COMP_PRES_STATE_SET(comp, SA_AMF_PRESENCE_INSTANTIATED);
	m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, comp, AVND_CKPT_COMP_PRES_STATE);

	return rc;
}

/****************************************************************************
  Name          : avnd_comp_clc_restart_term_hdler
 
  Description   : This routine processes the `terminate` event in 
                  `restarting` state.
 
  Arguments     : cb   - ptr to the AvND control block
                  comp - ptr to the comp
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
uns32 avnd_comp_clc_restart_term_hdler(AVND_CB *cb, AVND_COMP *comp)
{
	uns32 rc = NCSCC_RC_SUCCESS;

	/* cleanup the comp */
	if (m_AVND_COMP_TYPE_IS_PROXIED(comp) && comp->pxy_comp != 0)
		rc = avnd_comp_cbk_send(cb, comp, AVSV_AMF_PXIED_COMP_CLEAN, 0, 0);
	else {
		rc = avnd_comp_clc_cmd_execute(cb, comp, AVND_COMP_CLC_CMD_TYPE_CLEANUP);

		/* delete hc-list, cbk-list, pg-list & pm-list */
		avnd_comp_hc_rec_del_all(cb, comp);
		avnd_comp_cbq_del(cb, comp, TRUE);

		/* re-using the funtion to stop all PM started by this comp */
		avnd_comp_pm_finalize(cb, comp, comp->reg_hdl);
		avnd_comp_pm_rec_del_all(cb, comp);	/*if at all anythnig is left behind */
	}

	if (NCSCC_RC_SUCCESS == rc) {
		/* reset the comp-reg & instantiate params */
		m_AVND_COMP_REG_PARAM_RESET(cb, comp);
		m_AVND_COMP_CLC_INST_PARAM_RESET(comp);

		/* transition to 'terminating' state */
		m_AVND_COMP_PRES_STATE_SET(comp, SA_AMF_PRESENCE_TERMINATING);

		m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, comp, AVND_CKPT_COMP_CONFIG);
	}

	return rc;
}

/****************************************************************************
  Name          : avnd_comp_clc_restart_termsucc_hdler
 
  Description   : This routine processes the `terminate success` event in 
                  `restarting` state.
 
  Arguments     : cb   - ptr to the AvND control block
                  comp - ptr to the comp
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
uns32 avnd_comp_clc_restart_termsucc_hdler(AVND_CB *cb, AVND_COMP *comp)
{
	uns32 rc = NCSCC_RC_SUCCESS;

	if (!m_AVND_COMP_TYPE_IS_PROXIED(comp)) {
		m_AVND_COMP_REG_PARAM_RESET(cb, comp);
		m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, comp, AVND_CKPT_COMP_CONFIG);
	}

	/* re-instantiate the comp */
	if (m_AVND_COMP_TYPE_IS_PROXIED(comp) && comp->pxy_comp != 0 && m_AVND_COMP_TYPE_IS_PREINSTANTIABLE(comp))
		/* proxied pre-instantiable comp */
		rc = avnd_comp_cbk_send(cb, comp, AVSV_AMF_PXIED_COMP_INST, 0, 0);
	else if (m_AVND_COMP_TYPE_IS_PROXIED(comp) && comp->pxy_comp != 0 && !m_AVND_COMP_TYPE_IS_PREINSTANTIABLE(comp))
		/* proxied non-pre-instantiable comp */
		rc = avnd_comp_cbk_send(cb, comp, AVSV_AMF_CSI_SET, 0, 0);
	else if (m_AVND_COMP_TYPE_IS_PROXIED(comp)) ;	/* do nothing */
	else
		rc = avnd_comp_clc_cmd_execute(cb, comp, AVND_COMP_CLC_CMD_TYPE_INSTANTIATE);

	if (NCSCC_RC_SUCCESS == rc) {
		/* timestamp the start of this instantiation phase */
		m_GET_TIME_STAMP(comp->clc_info.inst_cmd_ts);
		m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, comp, AVND_CKPT_COMP_INST_CMD_TS);
	}

	return rc;
}

/****************************************************************************
  Name          : avnd_comp_clc_restart_termfail_hdler
 
  Description   : This routine processes the `terminate fail` event in 
                  `restarting` state.
 
  Arguments     : cb   - ptr to the AvND control block
                  comp - ptr to the comp
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
uns32 avnd_comp_clc_restart_termfail_hdler(AVND_CB *cb, AVND_COMP *comp)
{
	uns32 rc = NCSCC_RC_SUCCESS;

	/* cleanup the comp */
	if (m_AVND_COMP_TYPE_IS_PROXIED(comp) && comp->pxy_comp != 0)
		rc = avnd_comp_cbk_send(cb, comp, AVSV_AMF_PXIED_COMP_CLEAN, 0, 0);
	else
		rc = avnd_comp_clc_cmd_execute(cb, comp, AVND_COMP_CLC_CMD_TYPE_CLEANUP);

	/* transition to 'term-failed' state */
	if (NCSCC_RC_SUCCESS == rc) {
		m_AVND_COMP_PRES_STATE_SET(comp, SA_AMF_PRESENCE_TERMINATION_FAILED);
		m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, comp, AVND_CKPT_COMP_PRES_STATE);
	}

	return rc;
}

/****************************************************************************
  Name          : avnd_comp_clc_restart_clean_hdler
 
  Description   : This routine processes the `cleanup` event in 
                  `restarting` state.
 
  Arguments     : cb   - ptr to the AvND control block
                  comp - ptr to the comp
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
uns32 avnd_comp_clc_restart_clean_hdler(AVND_CB *cb, AVND_COMP *comp)
{
	uns32 rc = NCSCC_RC_SUCCESS;

	if (m_AVND_COMP_TYPE_IS_PROXIED(comp))
		avnd_comp_cbq_del(cb, comp, TRUE);

	/* cleanup the comp */
	if (m_AVND_COMP_TYPE_IS_PROXIED(comp) && comp->pxy_comp != 0)
		rc = avnd_comp_cbk_send(cb, comp, AVSV_AMF_PXIED_COMP_CLEAN, 0, 0);
	else
		rc = avnd_comp_clc_cmd_execute(cb, comp, AVND_COMP_CLC_CMD_TYPE_CLEANUP);
	if (NCSCC_RC_SUCCESS == rc) {
		/* reset the comp-reg & instantiate params */
		/* For proxied components, registration is treated as 
		   "some one is proxying for him", so we need not reset the
		   reg parameters 
		 */

		if (!m_AVND_COMP_TYPE_IS_PROXIED(comp))
			m_AVND_COMP_REG_PARAM_RESET(cb, comp);
		m_AVND_COMP_CLC_INST_PARAM_RESET(comp);

		/* transition to 'terminating' state */
		if (!m_AVND_COMP_IS_TERM_FAIL(comp))
			m_AVND_COMP_PRES_STATE_SET(comp, SA_AMF_PRESENCE_TERMINATING);
		else
			m_AVND_COMP_TERM_FAIL_RESET(comp);

		m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, comp, AVND_CKPT_COMP_CONFIG);
	}

	return rc;
}

/****************************************************************************
  Name          : avnd_comp_clc_restart_cleanfail_hdler
 
  Description   : This routine processes the `cleanup fail` event in 
                  `restarting` state.
 
  Arguments     : cb   - ptr to the AvND control block
                  comp - ptr to the comp
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
uns32 avnd_comp_clc_restart_cleanfail_hdler(AVND_CB *cb, AVND_COMP *comp)
{
	uns32 rc = NCSCC_RC_SUCCESS;

	/* transition to 'term-failed' state */
	m_AVND_COMP_PRES_STATE_SET(comp, SA_AMF_PRESENCE_TERMINATION_FAILED);
	m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, comp, AVND_CKPT_COMP_PRES_STATE);

	return rc;
}

/****************************************************************************
  Name          : avnd_comp_clc_orph_instsucc_hdler
 
  Description   : This routine processes the `instantiate success` event in 
                  `orph` state.
 
  Arguments     : cb   - ptr to the AvND control block
                  comp - ptr to the comp
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
uns32 avnd_comp_clc_orph_instsucc_hdler(AVND_CB *cb, AVND_COMP *comp)
{
	uns32 rc = NCSCC_RC_SUCCESS;

	/* stop the proxied registration timer */
	if (m_AVND_TMR_IS_ACTIVE(comp->orph_tmr)) {
		m_AVND_TMR_PXIED_COMP_REG_STOP(cb, *comp);
		m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, comp, AVND_CKPT_COMP_ORPH_TMR);
	}

	/* just transition to 'instantiated' state */
	m_AVND_COMP_PRES_STATE_SET(comp, SA_AMF_PRESENCE_INSTANTIATED);
	m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, comp, AVND_CKPT_COMP_PRES_STATE);

	return rc;
}

/****************************************************************************
  Name          : avnd_comp_clc_orph_term_hdler
 
  Description   : This routine processes the `termate` event in 
                  `orph` state.
 
  Arguments     : cb   - ptr to the AvND control block
                  comp - ptr to the comp
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
uns32 avnd_comp_clc_orph_term_hdler(AVND_CB *cb, AVND_COMP *comp)
{
	uns32 rc = NCSCC_RC_SUCCESS;

	/* queue up this event, we will process it in inst state */
	comp->pend_evt = AVND_COMP_CLC_PRES_FSM_EV_TERM;
	m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, comp, AVND_CKPT_COMP_PEND_EVT);
	return rc;
}

/****************************************************************************
  Name          : avnd_comp_clc_orph_clean_hdler
 
  Description   : This routine processes the `cleanup` event in 
                  `orph` state.
 
  Arguments     : cb   - ptr to the AvND control block
                  comp - ptr to the comp
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
uns32 avnd_comp_clc_orph_clean_hdler(AVND_CB *cb, AVND_COMP *comp)
{
	uns32 rc = NCSCC_RC_SUCCESS;

	/* stop orphan timer if still running */
	if (m_AVND_TMR_IS_ACTIVE(comp->orph_tmr)) {
		m_AVND_TMR_PXIED_COMP_REG_STOP(cb, *comp);
		m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, comp, AVND_CKPT_COMP_ORPH_TMR);
	}

	avnd_comp_cbq_del(cb, comp, TRUE);

	/* cleanup the comp */
	rc = avnd_comp_clc_cmd_execute(cb, comp, AVND_COMP_CLC_CMD_TYPE_CLEANUP);
	if (NCSCC_RC_SUCCESS == rc) {
		/* reset the comp-reg & instantiate params */
		m_AVND_COMP_REG_PARAM_RESET(cb, comp);
		m_AVND_COMP_CLC_INST_PARAM_RESET(comp);

		/* transition to 'terminating' state */
		m_AVND_COMP_PRES_STATE_SET(comp, SA_AMF_PRESENCE_TERMINATING);

		m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, comp, AVND_CKPT_COMP_CONFIG);
	}

	return rc;
}

/****************************************************************************
  Name          : avnd_comp_clc_orph_restart_hdler
 
  Description   : This routine processes the `restart` event in 
                  `orph` state.
 
  Arguments     : cb   - ptr to the AvND control block
                  comp - ptr to the comp
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
uns32 avnd_comp_clc_orph_restart_hdler(AVND_CB *cb, AVND_COMP *comp)
{
	uns32 rc = NCSCC_RC_SUCCESS;

	/* In orphaned state we can't restart the comp
	   just queue the event , we will handle it 
	   on reaching inst state */
	comp->pend_evt = AVND_COMP_CLC_PRES_FSM_EV_RESTART;
	m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, comp, AVND_CKPT_COMP_PEND_EVT);

	return rc;
}

/****************************************************************************
  Name          : avnd_comp_clc_cmd_execute
 
  Description   : This routine executes the specified command for a component.
 
  Arguments     : cb       - ptr to the AvND control block
                  comp     - ptr to the comp
                  cmd_type - command that is to be executed
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : For testing purpose (to let the comp know the node-id), 
                  node-id env variable is set. In the real world, NodeId is 
                  derived from HPI.
******************************************************************************/
uns32 avnd_comp_clc_cmd_execute(AVND_CB *cb, AVND_COMP *comp, AVND_COMP_CLC_CMD_TYPE cmd_type)
{
	NCS_OS_PROC_EXECUTE_TIMED_INFO cmd_info;
	NCS_OS_ENVIRON_ARGS arg;
	NCS_OS_ENVIRON_SET_NODE env_set[3];
	SaNameT env_val_name;
	char env_val_nodeid[11];
	char env_val_comp_err[11];	/*we req only 10 */
	char env_var_name[] = "SA_AMF_COMPONENT_NAME";
	char env_var_nodeid[] = "NCS_ENV_NODE_ID";
	char env_var_comp_err[] = "NCS_ENV_COMPONENT_ERROR_SRC";
	char env_var_attr[] = "NCS_ENV_COMPONENT_CSI_ATTR";
	char *env_attr_val = 0;
	AVND_CLC_EVT clc_evt;
	AVND_EVT *evt = 0;
	AVND_COMP_CLC_INFO *clc_info = &comp->clc_info;
	char scr[SAAMF_CLC_LEN];
	char *argv[AVND_COMP_CLC_PARAM_MAX + 2];
	char tmp_argv[AVND_COMP_CLC_PARAM_MAX + 2][AVND_COMP_CLC_PARAM_SIZE_MAX];
	uns32 argc = 0, result, rc = NCSCC_RC_SUCCESS;

	TRACE_ENTER2("%s, type:%u", comp->name.value, cmd_type);

	/* Refresh the component configuration, it may have changed */
	if (avnd_comp_config_reinit(comp) != 0) {
		rc = NCSCC_RC_FAILURE;
		goto err;
	}

	/* For external component, there is no cleanup command. So, we will send a
	   SUCCESS message to the mail box for external components. There wouldn't
	   be any other command for external component comming. */
	if (TRUE == comp->su->su_is_external) {
		if (AVND_COMP_CLC_CMD_TYPE_CLEANUP == cmd_type) {
			memset(&clc_evt, 0, sizeof(AVND_CLC_EVT));
			memcpy(&clc_evt.comp_name, &comp->name, sizeof(SaNameT));
			clc_evt.cmd_type = cmd_type;
			clc_evt.exec_stat.value = NCS_OS_PROC_EXIT_NORMAL;

			clc_info->exec_cmd = cmd_type;
			m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, comp, AVND_CKPT_COMP_EXEC_CMD);

			clc_info->cmd_exec_ctxt = 0;
			m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, comp, AVND_CKPT_COMP_CMD_EXEC_CTXT);

			/* create the event */
			evt = avnd_evt_create(cb, AVND_EVT_CLC_RESP, 0, 0, 0, (void *)&clc_evt, 0);
			if (!evt) {
				rc = NCSCC_RC_FAILURE;
				goto err;
			}

			/* send the event */
			rc = avnd_evt_send(cb, evt);
			if (NCSCC_RC_SUCCESS != rc)
				goto err;

			return rc;
		} else {
			m_AVND_AVND_ERR_LOG("Command other than cleanup recvd for ext comp: Comp and cmd_type are",
					    &comp->name, cmd_type, 0, 0, 0);
		}
	}

	memset(&cmd_info, 0, sizeof(NCS_OS_PROC_EXECUTE_TIMED_INFO));
	memset(&arg, 0, sizeof(NCS_OS_ENVIRON_ARGS));
	memset(env_set, 0, 3 * sizeof(NCS_OS_ENVIRON_SET_NODE));

	memset(&env_val_name, '\0', sizeof(SaNameT));
	memset(env_val_nodeid, '\0', sizeof(env_val_nodeid));
	memcpy((char*)env_val_name.value, (char*)comp->name.value, comp->name.length);

   /*** populate the env variable set ***/
	/* comp name env */
	env_set[0].overwrite = 1;
	env_set[0].name = env_var_name;
	env_set[0].value = (char *)(env_val_name.value);
	arg.num_args++;

	/* node id env */
	env_set[1].overwrite = 1;
	env_set[1].name = env_var_nodeid;
	sprintf(env_val_nodeid, "%u", (uns32)(cb->node_info.nodeId));
	env_set[1].value = (char *)env_val_nodeid;
	arg.num_args++;

	/* Note:- we will set NCS_ENV_COMPONENT_ERROR_SRC only for 
	 * cleanup script
	 */

	/* populate the env arg */
	if (cmd_type == AVND_COMP_CLC_CMD_TYPE_CLEANUP) {
		/* error code, will be set only if we are cleaning up */
		memset(env_val_comp_err, '\0', sizeof(env_val_comp_err));
		env_set[2].overwrite = 1;
		env_set[2].name = (char *)env_var_comp_err;
		sprintf((char *)env_val_comp_err, "%u", (uns32)(comp->err_info.src));
		env_set[2].value = (char *)env_val_comp_err;
		arg.num_args++;
	}

	/*Note :- For the Npi Components Csi Attributes 
	   are Passed as Env Variable , The List of Csi attr
	   will be send in "Name1=Value1,Name2=value2" form
	   This format will be Retained when Env. Variable will be
	   added in CLC command */

	if ((AVND_COMP_CLC_CMD_TYPE_INSTANTIATE == cmd_type) && (!m_AVND_COMP_TYPE_IS_PREINSTANTIABLE(comp))) {

		/*Special case :The CSI attributes given as Name/Value 
		   pair are to be Passed as Env. for NPI .NPI will have one 
		   CSI assign at a point of time , so get the csi from comp DB */

		env_attr_val = avnd_prep_attr_env_var(comp, &result);

		if (NCSCC_RC_FAILURE == result)
			goto err;

		if (NULL != env_attr_val) {
			/*Csi Attributes env */
			env_set[2].overwrite = 1;
			env_set[2].name = (char *)env_var_attr;
			env_set[2].value = (char *)env_attr_val;
			arg.num_args++;
		}

	}

	arg.env_arg = env_set;

	/* tokenize the cmd */
	m_AVND_COMP_CLC_STR_PARSE(clc_info->cmds[cmd_type - 1].cmd, scr, argc, argv, tmp_argv);

	/* populate the cmd-info */
	cmd_info.i_script = argv[0];
	cmd_info.i_argv = argv;
	cmd_info.i_timeout_in_ms = (uns32)((clc_info->cmds[cmd_type - 1].timeout) / 1000000);
	cmd_info.i_cb = avnd_comp_clc_resp;
	cmd_info.i_set_env_args = &arg;

	/* finally execute the command */
	rc = m_NCS_OS_PROCESS_EXECUTE_TIMED(&cmd_info);
	if (NCSCC_RC_SUCCESS != rc) {
		/* generate a cmd failure event; it'll be executed asynchronously */
		memset(&clc_evt, 0, sizeof(AVND_CLC_EVT));

		/* fill the clc-evt param */
		memcpy(&clc_evt.comp_name, &comp->name, sizeof(SaNameT));
		clc_evt.cmd_type = cmd_type;

		/* create the event */
		evt = avnd_evt_create(cb, AVND_EVT_CLC_RESP, 0, 0, 0, (void *)&clc_evt, 0);
		if (!evt) {
			rc = NCSCC_RC_FAILURE;
			goto err;
		}

		/* send the event */
		rc = avnd_evt_send(cb, evt);
		if (NCSCC_RC_SUCCESS != rc)
			goto err;
	} else {
		if (cmd_type == AVND_COMP_CLC_CMD_TYPE_AMSTART || cmd_type == AVND_COMP_CLC_CMD_TYPE_AMSTOP)
			clc_info->am_cmd_exec_ctxt = cmd_info.o_exec_hdl;
		else {
			clc_info->cmd_exec_ctxt = cmd_info.o_exec_hdl;
			m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, comp, AVND_CKPT_COMP_CMD_EXEC_CTXT);
		}
	}

	/* irrespective of the command execution status, set the current cmd */
	if (cmd_type == AVND_COMP_CLC_CMD_TYPE_AMSTART || cmd_type == AVND_COMP_CLC_CMD_TYPE_AMSTOP)
		clc_info->am_exec_cmd = cmd_type;
	else {
		clc_info->exec_cmd = cmd_type;
		m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, comp, AVND_CKPT_COMP_EXEC_CMD);
	}

	m_AVND_LOG_MISC(cmd_type, &comp->name, NCSFL_SEV_NOTICE);

	return NCSCC_RC_SUCCESS;

 err:
	/* free the event */
	if (evt)
		avnd_evt_destroy(evt);
	if (env_attr_val) {
		/* Free the Memory allocated for CLC command environment Sets */
		free(env_attr_val);
	}

	TRACE_LEAVE2("%u", rc);
	return rc;
}

/****************************************************************************
  Name          : avnd_instfail_su_failover
 
  Description   : This routine executes SU failover for inst failed SU.
 
  Arguments     : cb          - ptr to the AvND control block
                  su          - ptr to the SU to which the comp belongs
                  failed_comp - ptr to the failed comp that triggered this. 
                                
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 
  Notes         : None.
******************************************************************************/
uns32 avnd_instfail_su_failover(AVND_CB *cb, AVND_SU *su, AVND_COMP *failed_comp)
{
	uns32 rc = NCSCC_RC_SUCCESS;

	/* mark the comp failed */
	m_AVND_COMP_FAILED_SET(failed_comp);
	m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, failed_comp, AVND_CKPT_COMP_FLAG_CHANGE);

	/* update comp oper state */
	m_AVND_COMP_OPER_STATE_SET(failed_comp, SA_AMF_OPERATIONAL_DISABLED);
	m_AVND_COMP_OPER_STATE_AVD_SYNC(cb, failed_comp, rc);
	if (NCSCC_RC_SUCCESS != rc)
		goto done;
	m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, failed_comp, AVND_CKPT_COMP_OPER_STATE);

	/* if we are in the middle of su restart, reset the flag and go ahead */
	if (m_AVND_SU_IS_RESTART(su)) {
		m_AVND_SU_RESTART_RESET(su);
		m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, su, AVND_CKPT_SU_FLAG_CHANGE);
	}

	/* mark the su failed */
	if (!m_AVND_SU_IS_FAILED(su)) {
		m_AVND_SU_FAILED_SET(su);
		m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, su, AVND_CKPT_SU_FLAG_CHANGE);
	}

	/*
	 * su is already in the middle of error processing i.e. su-sis may be 
	 * in assigning/removing state. signal csi assign/remove done so 
	 * that su-si assignment/removal algo can proceed.
	 */
	avnd_comp_cmplete_all_assignment(cb, failed_comp);

	/* go and look for all csi's in assigning state and complete the assignment.
	 * take care of assign-one and assign-all flags
	 */
	avnd_comp_cmplete_all_csi_rec(cb, failed_comp);

	/* delete curr info of the failed comp */
	rc = avnd_comp_curr_info_del(cb, failed_comp);
	if (NCSCC_RC_SUCCESS != rc)
		goto done;

	/* update su oper state */
	if (m_AVND_SU_OPER_STATE_IS_ENABLED(su)) {
		m_AVND_SU_OPER_STATE_SET(su, SA_AMF_OPERATIONAL_DISABLED);
		m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, su, AVND_CKPT_SU_OPER_STATE);

		/* inform AvD */
		rc = avnd_di_oper_send(cb, su, AVSV_ERR_RCVR_SU_FAILOVER);
	}

 done:
	return rc;
}

/****************************************************************************
  Name          : avnd_prep_attr_env_var
 
  Description   : This routine prep. the env. variable from csi attribute and
                  cl env variables
 
  Arguments     : comp          - ptr to the comp. 
                  ret_status    - ptr to the return status
                                
 
  Return Values : ptr to Environment attributes.
 
  Notes         : None.
******************************************************************************/
static char *avnd_prep_attr_env_var(AVND_COMP *comp, uns32 *ret_status)
{

	AVND_COMP_CSI_REC *curr_csi = 0;
	AVSV_ATTR_NAME_VAL *attr_list = 0;
	char *env_val = 0;
	uns32 mem_length = 0;
	uns32 count = 0;

	/* There should be a CSI Assigned to the component else will never come
	   this path  */

	*ret_status = NCSCC_RC_SUCCESS;

	curr_csi = m_AVND_CSI_REC_FROM_COMP_DLL_NODE_GET(m_NCS_DBLIST_FIND_FIRST(&comp->csi_list));
	assert(curr_csi);

	if (0 != curr_csi->attrs.number) {
		attr_list = (AVSV_ATTR_NAME_VAL *)curr_csi->attrs.list;

		/*First get the length of the Memory to be allocated */

		for (count = 0; count < (curr_csi->attrs.number); count++, attr_list++) {
			/* Adding 2 to account for the "=" and "," to be added */
			mem_length = mem_length + attr_list->name.length + attr_list->value.length + 2;
		}
	} else {
		/* No attributes present */
		*ret_status = NCSCC_RC_SUCCESS;
		return env_val;
	}

	/*Allocate the Memory of mem_length + 1 */
	env_val = (mem_length) ? malloc(mem_length + 1) : NULL;

	if (NULL == env_val) {	/* Mem failure */
		*ret_status = NCSCC_RC_FAILURE;
		return env_val;
	}

	memset(env_val, '\0', (mem_length + 1));

	/* Now make the Env. variable */
	if (0 != curr_csi->attrs.number) {
		attr_list = (AVSV_ATTR_NAME_VAL *)curr_csi->attrs.list;

		for (count = 0; count < (curr_csi->attrs.number); count++, attr_list++) {
			/* If only one attribute No "," */
			if (0 != count)
				strncat(env_val, ",", 1);

			strncat(env_val, (char*)attr_list->name.value, attr_list->name.length);
			strncat(env_val, "=", 1);
			strncat(env_val, (char*)attr_list->value.value, attr_list->value.length);
		}
	}
	return env_val;

}
