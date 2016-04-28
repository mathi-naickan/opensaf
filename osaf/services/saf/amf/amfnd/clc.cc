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

#include <string.h>

#include <logtrace.h>

#include <avnd.h>

/* static function declarations */
static uint32_t avnd_comp_clc_uninst_inst_hdler(AVND_CB *, AVND_COMP *);
static uint32_t avnd_comp_clc_xxxing_cleansucc_hdler(AVND_CB *, AVND_COMP *);
static uint32_t avnd_comp_clc_xxxing_instfail_hdler(AVND_CB *, AVND_COMP *);
static uint32_t avnd_comp_clc_insting_inst_hdler(AVND_CB *, AVND_COMP *);
static uint32_t avnd_comp_clc_insting_instsucc_hdler(AVND_CB *, AVND_COMP *);
static uint32_t avnd_comp_clc_insting_term_hdler(AVND_CB *, AVND_COMP *);
static uint32_t avnd_comp_clc_insting_clean_hdler(AVND_CB *, AVND_COMP *);
static uint32_t avnd_comp_clc_insting_cleanfail_hdler(AVND_CB *, AVND_COMP *);
static uint32_t avnd_comp_clc_insting_restart_hdler(AVND_CB *, AVND_COMP *);
static uint32_t avnd_comp_clc_inst_term_hdler(AVND_CB *, AVND_COMP *);
static uint32_t avnd_comp_clc_inst_clean_hdler(AVND_CB *, AVND_COMP *);
static uint32_t avnd_comp_clc_inst_restart_hdler(AVND_CB *, AVND_COMP *);
static uint32_t avnd_comp_clc_inst_orph_hdler(AVND_CB *, AVND_COMP *);
static uint32_t avnd_comp_clc_terming_termsucc_hdler(AVND_CB *, AVND_COMP *);
static uint32_t avnd_comp_clc_terming_termfail_hdler(AVND_CB *, AVND_COMP *);
static uint32_t avnd_comp_clc_terming_cleansucc_hdler(AVND_CB *, AVND_COMP *);
static uint32_t avnd_comp_clc_terming_cleanfail_hdler(AVND_CB *, AVND_COMP *);
static uint32_t avnd_comp_clc_restart_inst_hdler(AVND_CB *, AVND_COMP *);
static uint32_t avnd_comp_clc_restart_instsucc_hdler(AVND_CB *, AVND_COMP *);
static uint32_t avnd_comp_clc_restart_term_hdler(AVND_CB *, AVND_COMP *);
static uint32_t avnd_comp_clc_restart_termsucc_hdler(AVND_CB *, AVND_COMP *);
static uint32_t avnd_comp_clc_restart_termfail_hdler(AVND_CB *, AVND_COMP *);
static uint32_t avnd_comp_clc_restart_clean_hdler(AVND_CB *, AVND_COMP *);
static uint32_t avnd_comp_clc_restart_cleanfail_hdler(AVND_CB *, AVND_COMP *);
static uint32_t avnd_comp_clc_restart_restart_hdler(AVND_CB *cb, AVND_COMP *comp);
static uint32_t avnd_comp_clc_orph_instsucc_hdler(AVND_CB *, AVND_COMP *);
static uint32_t avnd_comp_clc_orph_term_hdler(AVND_CB *, AVND_COMP *);
static uint32_t avnd_comp_clc_orph_clean_hdler(AVND_CB *, AVND_COMP *);
static uint32_t avnd_comp_clc_orph_restart_hdler(AVND_CB *, AVND_COMP *);

uint32_t avnd_comp_clc_st_chng_prc(AVND_CB *, AVND_COMP *, SaAmfPresenceStateT, SaAmfPresenceStateT);

static uint32_t avnd_instfail_su_failover(AVND_CB *, AVND_SU *, AVND_COMP *);

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
	 avnd_comp_clc_restart_inst_hdler,	/* INST EV */
	 avnd_comp_clc_restart_instsucc_hdler,	/* INST_SUCC EV */
	 avnd_comp_clc_xxxing_instfail_hdler,	/* INST_FAIL EV */
	 avnd_comp_clc_restart_term_hdler,	/* TERM EV */
	 avnd_comp_clc_restart_termsucc_hdler,	/* TERM_SUCC EV */
	 avnd_comp_clc_restart_termfail_hdler,	/* TERM_FAIL EV */
	 avnd_comp_clc_restart_clean_hdler,	/* CLEANUP EV */
	 avnd_comp_clc_xxxing_cleansucc_hdler,	/* CLEANUP_SUCC EV */
	 avnd_comp_clc_restart_cleanfail_hdler,	/* CLEANUP_FAIL EV */
	 avnd_comp_clc_restart_restart_hdler,			/* RESTART EV */
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

static const char *pres_state_evt[] =
{
	"OUT_OF_RANGE",
	"AVND_COMP_CLC_PRES_FSM_EV_INST",
	"AVND_COMP_CLC_PRES_FSM_EV_INST_SUCC",
	"AVND_COMP_CLC_PRES_FSM_EV_INST_FAIL",
	"AVND_COMP_CLC_PRES_FSM_EV_TERM",
	"AVND_COMP_CLC_PRES_FSM_EV_TERM_SUCC",
	"AVND_COMP_CLC_PRES_FSM_EV_TERM_FAIL",
	"AVND_COMP_CLC_PRES_FSM_EV_CLEANUP",
	"AVND_COMP_CLC_PRES_FSM_EV_CLEANUP_SUCC",
	"AVND_COMP_CLC_PRES_FSM_EV_CLEANUP_FAIL",
	"AVND_COMP_CLC_PRES_FSM_EV_RESTART",
	"AVND_COMP_CLC_PRES_FSM_EV_ORPH",
	"AVND_COMP_CLC_PRES_FSM_EV_MAX"
};

static const char *pres_state[] =
{
	"OUT_OF_RANGE",
	"SA_AMF_PRESENCE_UNINSTANTIATED(1)",
	"SA_AMF_PRESENCE_INSTANTIATING(2)",
	"SA_AMF_PRESENCE_INSTANTIATED(3)",
	"SA_AMF_PRESENCE_TERMINATING(4)",
	"SA_AMF_PRESENCE_RESTARTING(5)",
	"SA_AMF_PRESENCE_INSTANTIATION_FAILED(6)",
	"SA_AMF_PRESENCE_TERMINATION_FAILED(7)"
};

static const char *clc_cmd_type[] =
{
	"OUT_OF_RANGE",
	"AVND_COMP_CLC_CMD_TYPE_INSTANTIATE(1)",
	"AVND_COMP_CLC_CMD_TYPE_TERMINATE(2)",
	"AVND_COMP_CLC_CMD_TYPE_CLEANUP(3)",
	"AVND_COMP_CLC_CMD_TYPE_AMSTART(4)",
	"AVND_COMP_CLC_CMD_TYPE_AMSTOP(5)",
	"AVND_COMP_CLC_CMD_TYPE_HC(6)",
	"AVND_COMP_CLC_CMD_TYPE_MAX"
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
	osafassert(exec_cmd <= AVND_COMP_CLC_CMD_TYPE_MAX);

	LOG_NO("Reason:'%s'", str_exec_status(exec_stat->value));

	if (NCS_OS_PROC_EXIT_WITH_CODE == exec_stat->value)
		LOG_NO("Exit code: %u", exec_stat->info.exit_with_code.exit_code);

	if (NCS_OS_PROC_EXEC_FAIL == exec_stat->value)
		LOG_NO("CLC CLI script:'%s'", comp->clc_info.cmds[exec_cmd-1].cmd);

	if (NCS_OS_PROC_EXIT_ON_SIGNAL == exec_stat->value)
		LOG_NO("Signal: %u, CLC CLI script:'%s'", exec_stat->info.exit_on_signal.signal_num,comp->clc_info.cmds[exec_cmd-1].cmd);
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
uint32_t avnd_evt_clc_resp_evh(AVND_CB *cb, AVND_EVT *evt)
{
	AVND_COMP_CLC_PRES_FSM_EV ev = AVND_COMP_CLC_PRES_FSM_EV_MAX;
	AVND_CLC_EVT *clc_evt = &evt->info.clc;
	AVND_COMP *comp = 0;
	uint32_t rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER();

	comp = m_AVND_COMPDB_REC_GET(cb->compdb, clc_evt->comp_name);

	/* the comp has been deleted? */
	if (comp == nullptr) {
		 LOG_WA("%s: could not find comp '%s'", __FUNCTION__,
				 clc_evt->comp_name.value);
		 goto done;
	}

	TRACE("'%s', command type:%s", comp->name.value, clc_cmd_type[clc_evt->cmd_type]);

	comp->clc_info.exec_cmd = AVND_COMP_CLC_CMD_TYPE_NONE;

	switch (clc_evt->cmd_type) {
	case AVND_COMP_CLC_CMD_TYPE_INSTANTIATE:
		/*
		 * note that inst-succ evt is generated for the fsm only
		 * when the comp is registered too
		 */
		if (NCS_OS_PROC_EXIT_NORMAL == clc_evt->exec_stat.value) {
			/* mark that the inst cmd has been successfully executed */
			m_AVND_COMP_INST_CMD_SUCC_SET(comp);

			if (!m_AVND_COMP_TYPE_IS_PREINSTANTIABLE(comp) || m_AVND_COMP_IS_REG(comp)) {
				/* all set... proceed with inst-succ evt for the fsm */
				ev = AVND_COMP_CLC_PRES_FSM_EV_INST_SUCC;
				TRACE("Comp '%s' Inst. flag '%u'", comp->name.value, comp->flag);
			} else {
				/* start the comp-reg timer */
				m_AVND_TMR_COMP_REG_START(cb, *comp, rc);
			}
		} else {
			ev = AVND_COMP_CLC_PRES_FSM_EV_INST_FAIL;

			if (NCS_OS_PROC_EXIT_WITH_CODE == clc_evt->exec_stat.value) {
				comp->clc_info.inst_code_rcvd =
					clc_evt->exec_stat.info.exit_with_code.exit_code;
			}

			LOG_NO("Instantiation of '%s' failed", comp->name.value);
			log_failed_exec(&clc_evt->exec_stat, comp, clc_evt->cmd_type);
		}
		break;

	case AVND_COMP_CLC_CMD_TYPE_TERMINATE:
		if (NCS_OS_PROC_EXIT_NORMAL == clc_evt->exec_stat.value) {
			ev = AVND_COMP_CLC_PRES_FSM_EV_TERM_SUCC;
			TRACE("Comp '%s' Term.", comp->name.value);
		} else {
			ev = AVND_COMP_CLC_PRES_FSM_EV_CLEANUP;
			m_AVND_COMP_TERM_FAIL_SET(comp);
			LOG_NO("Termination of '%s' failed", comp->name.value);
			log_failed_exec(&clc_evt->exec_stat, comp, clc_evt->cmd_type);
		}

		break;

	case AVND_COMP_CLC_CMD_TYPE_AMSTART:
		rc = avnd_comp_amstart_clc_res_process(cb, comp, clc_evt->exec_stat.value);
		break;

	case AVND_COMP_CLC_CMD_TYPE_AMSTOP:
		rc = avnd_comp_amstop_clc_res_process(cb, comp, clc_evt->exec_stat.value);
		break;

	case AVND_COMP_CLC_CMD_TYPE_CLEANUP:
		if (NCS_OS_PROC_EXIT_NORMAL == clc_evt->exec_stat.value) {
			ev = AVND_COMP_CLC_PRES_FSM_EV_CLEANUP_SUCC;
			TRACE("Comp '%s' Cleanup.", comp->name.value);
		} else {
			ev = AVND_COMP_CLC_PRES_FSM_EV_CLEANUP_FAIL;
			LOG_NO("Cleanup of '%s' failed", comp->name.value);
			log_failed_exec(&clc_evt->exec_stat, comp, clc_evt->cmd_type);
		}
		break;

	case AVND_COMP_CLC_CMD_TYPE_HC:
		if (NCS_OS_PROC_EXIT_NORMAL == clc_evt->exec_stat.value) {
			avnd_comp_hc_cmd_restart(comp);
		} else {
			AVND_ERR_INFO err_info;
			LOG_NO("Healthcheck failed for '%s'", comp->name.value);
			log_failed_exec(&clc_evt->exec_stat, comp, clc_evt->cmd_type);
			err_info.src = AVND_ERR_SRC_HC;
			err_info.rec_rcvr.avsv_ext = static_cast<AVSV_ERR_RCVR>(comp->err_info.def_rec);
			rc = avnd_err_process(cb, comp, &err_info);
			goto done;
		}

		break;

	default:
		osafassert(0);
		break;
	}

	/* run the fsm */
	if (AVND_COMP_CLC_PRES_FSM_EV_MAX != ev)
		rc = avnd_comp_clc_fsm_run(cb, comp, ev);

done:
	TRACE_LEAVE2("%u", rc);
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
uint32_t avnd_evt_comp_pres_fsm_evh(AVND_CB *cb, AVND_EVT *evt)
{
	AVND_COMP_FSM_EVT *comp_fsm_evt = &evt->info.comp_fsm;
	AVND_COMP *comp = 0;
	uint32_t rc = NCSCC_RC_SUCCESS;
	bool is_uninst = false;
	TRACE_ENTER();

	/* get the comp */
	comp = m_AVND_COMPDB_REC_GET(cb->compdb, comp_fsm_evt->comp_name);
	if (!comp)
		goto done;

	if (all_comps_terminated_in_su(comp->su) == true)
		is_uninst = true;


	/* run the fsm */
	if (AVND_COMP_CLC_PRES_FSM_EV_MAX != comp_fsm_evt->ev)
		rc = avnd_comp_clc_fsm_run(cb, comp, static_cast<AVND_COMP_CLC_PRES_FSM_EV>(comp_fsm_evt->ev));

	if ((is_uninst == true) &&
			(comp->pres == SA_AMF_PRESENCE_INSTANTIATING))
		avnd_su_pres_state_set(cb, comp->su, SA_AMF_PRESENCE_INSTANTIATING);

done:
	TRACE_LEAVE2("%u", rc);
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
uint32_t avnd_evt_tmr_clc_comp_reg_evh(AVND_CB *cb, AVND_EVT *evt)
{
	AVND_TMR_EVT *tmr = &evt->info.tmr;
	AVND_COMP *comp = 0;
	uint32_t rc = NCSCC_RC_SUCCESS;

	TRACE_ENTER();

	/* retrieve the comp */
	comp = (AVND_COMP *)ncshm_take_hdl(NCS_SERVICE_ID_AVND, tmr->opq_hdl);
	if (!comp)
		goto done;

	if (NCSCC_RC_SUCCESS == m_AVND_CHECK_FOR_STDBY_FOR_EXT_COMP(cb, comp->su->su_is_external))
		goto done;

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
uint32_t avnd_evt_tmr_clc_pxied_comp_inst_evh(AVND_CB *cb, AVND_EVT *evt)
{
	AVND_TMR_EVT *tmr = &evt->info.tmr;
	AVND_COMP *comp = 0;
	uint32_t rc = NCSCC_RC_SUCCESS;

	TRACE_ENTER();

	/* retrieve the comp */
	comp = (AVND_COMP *)ncshm_take_hdl(NCS_SERVICE_ID_AVND, tmr->opq_hdl);
	if (!comp)
		goto done;

	if (NCSCC_RC_SUCCESS == m_AVND_CHECK_FOR_STDBY_FOR_EXT_COMP(cb, comp->su->su_is_external))
		goto done;

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
uint32_t avnd_evt_tmr_clc_pxied_comp_reg_evh(AVND_CB *cb, AVND_EVT *evt)
{
	AVND_TMR_EVT *tmr = &evt->info.tmr;
	AVND_COMP *comp = 0;
	AVND_ERR_INFO err_info;
	uint32_t rc = NCSCC_RC_SUCCESS;

	TRACE_ENTER();

	/* retrieve the comp */
	comp = (AVND_COMP *)ncshm_take_hdl(NCS_SERVICE_ID_AVND, tmr->opq_hdl);
	if (!comp)
		goto done;

	if (NCSCC_RC_SUCCESS == m_AVND_CHECK_FOR_STDBY_FOR_EXT_COMP(cb, comp->su->su_is_external))
		goto done;

	/* process comp failure */
	err_info.src = AVND_ERR_SRC_PXIED_REG_TIMEOUT;
	err_info.rec_rcvr.saf_amf = SA_AMF_COMPONENT_FAILOVER;
	rc = avnd_err_process(cb, comp, &err_info);

done:
	if (comp)
		ncshm_give_hdl(tmr->opq_hdl);
	return rc;
	TRACE_LEAVE();
}

/****************************************************************************
 
  Description   : This is a callback routine that is invoked when either the 
                  command finishes execution (success or failure) or it times
                  out. It sends the corresponding event to the main thread.
 
  Arguments     : info - ptr to the cbk info
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
******************************************************************************/
static uint32_t comp_clc_resp_callback(NCS_OS_PROC_EXECUTE_TIMED_CB_INFO *info)
{
	AVND_CLC_EVT *clc_evt = nullptr;
	AVND_EVT *evt = 0;
	uint32_t rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER();	

	assert(info != nullptr);
	clc_evt = (AVND_CLC_EVT *)info->i_usr_hdl;

	/* fill the clc-evt param */
	clc_evt->exec_stat = info->exec_stat;

	/* create the event */
	evt = avnd_evt_create(avnd_cb, AVND_EVT_CLC_RESP, 0, 0, 0, clc_evt, static_cast<AVND_COMP_FSM_EVT*>(0));
	if (!evt) {
		rc = NCSCC_RC_FAILURE;
		goto done;
	}

	/* send the event */
	rc = avnd_evt_send(avnd_cb, evt);

 done:
	delete clc_evt;
	/* free the event */
	if (NCSCC_RC_SUCCESS != rc && evt)
		avnd_evt_destroy(evt);

	TRACE_LEAVE2("%u", rc);
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
uint32_t avnd_comp_clc_fsm_trigger(AVND_CB *cb, AVND_COMP *comp, AVND_COMP_CLC_PRES_FSM_EV ev)
{
	AVND_COMP_FSM_EVT comp_fsm_evt;
	AVND_EVT *evt = 0;
	uint32_t rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER2("Comp '%s', Ev '%u'", comp->name.value, ev);

	memset(&comp_fsm_evt, 0, sizeof(AVND_COMP_FSM_EVT));

	/* fill the comp-fsm-evt param */
	comp_fsm_evt.comp_name = comp->name;
	comp_fsm_evt.ev = ev;

	/* create the event */
	evt = avnd_evt_create(cb, AVND_EVT_COMP_PRES_FSM_EV, 0, 0, 0, 0, &comp_fsm_evt);
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
	TRACE_LEAVE2("%u", rc);

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
	TRACE_ENTER();

	/* Scan all components to see if we're done terminating all comps */
	comp = (AVND_COMP *)ncs_patricia_tree_getnext(&avnd_cb->compdb, (uint8_t *)0);
	while (comp != 0) {
		if ((comp->pres != SA_AMF_PRESENCE_UNINSTANTIATED) &&
		    (comp->pres != SA_AMF_PRESENCE_INSTANTIATION_FAILED) &&
		    (comp->pres != SA_AMF_PRESENCE_TERMINATION_FAILED)) {
			all_comps_terminated = 0;
			TRACE("'%s' not terminated, pres.st=%u", comp->name.value, comp->pres);
			break;
		}

		comp = (AVND_COMP *) ncs_patricia_tree_getnext(&avnd_cb->compdb, (uint8_t *)&comp->name);
	}

	TRACE_LEAVE2("%d", all_comps_terminated);
	return all_comps_terminated;
}

/**
 * Check if all application components have been terminated.
 * 
 * @return bool
 */
static bool all_app_comps_terminated(void)
{
	AVND_COMP *comp;

	for (comp = (AVND_COMP *)ncs_patricia_tree_getnext(&avnd_cb->compdb, (uint8_t *)0);
		  comp;
		  comp = (AVND_COMP *) ncs_patricia_tree_getnext(&avnd_cb->compdb, (uint8_t *)&comp->name)) {

		/* Skip OpenSAF and external components */
		if (comp->su->is_ncs || comp->su->su_is_external)
			continue;

		if ((comp->pres != SA_AMF_PRESENCE_UNINSTANTIATED) &&
		    (comp->pres != SA_AMF_PRESENCE_INSTANTIATION_FAILED) &&
			(comp->pres != SA_AMF_PRESENCE_TERMINATION_FAILED)) {

			TRACE("'%s' not terminated, pres.st=%u", comp->name.value, comp->pres);
			return false;
		}
	}

	return true;
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
uint32_t avnd_comp_clc_fsm_run(AVND_CB *cb, AVND_COMP *comp, AVND_COMP_CLC_PRES_FSM_EV ev)
{
	SaAmfPresenceStateT prv_st, final_st;
	uint32_t rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER2("Comp '%s', Ev '%u'", comp->name.value, ev);

	if (cb->term_state == AVND_TERM_STATE_NODE_FAILOVER_TERMINATING) {
		TRACE("Term state is NODE_FAILOVER, event '%s'", pres_state_evt[ev]);
		switch (ev) {
		case AVND_COMP_CLC_PRES_FSM_EV_CLEANUP_SUCC:
			avnd_comp_pres_state_set(cb, comp, SA_AMF_PRESENCE_UNINSTANTIATED);
			if (all_app_comps_terminated()) {
				AVND_SU *tmp_su;
				cb->term_state = AVND_TERM_STATE_NODE_FAILOVER_TERMINATED;
				LOG_NO("Terminated all application components");
				LOG_NO("Informing director of node fail-over");
				rc = avnd_di_oper_send(cb, cb->failed_su, SA_AMF_NODE_FAILOVER);
				osafassert(NCSCC_RC_SUCCESS == rc);
				/* delete all SUSI record in amfnd database */
				tmp_su = (AVND_SU *)ncs_patricia_tree_getnext(&cb->sudb, (uint8_t *)0);
				while (tmp_su != nullptr) {
					if (tmp_su->is_ncs || tmp_su->su_is_external) {
						/* Don't delete middleware SUSI. We are only performing appl
						   failover in case of node failover recovery. This will help
						   when we implement node repair later.*/
					} else {
						avnd_su_si_del(cb, &tmp_su->name);
					}
					tmp_su = (AVND_SU *) ncs_patricia_tree_getnext(&cb->sudb,
							(uint8_t *)&tmp_su->name);

				}
			}
			break;
		case AVND_COMP_CLC_PRES_FSM_EV_CLEANUP_FAIL:
			LOG_ER("'%s' termination failed", comp->name.value);
			opensaf_reboot(avnd_cb->node_info.nodeId,
						   (char *)avnd_cb->node_info.executionEnvironment.value,
						   "Component termination failed at node failover");
			LOG_ER("Exiting (due to comp term failed) to aid fast node reboot");
			exit(0);
			break;
		case AVND_COMP_CLC_PRES_FSM_EV_CLEANUP:
			break;
		default:
			LOG_ER("Ignoring event '%s' for '%s' during node failover",
					pres_state_evt[ev], comp->name.value);
			goto done;
		}
	}

	/* get the prv presence state */
	prv_st = comp->pres;

	/* if already enabled, stop PM & AM */
	if (prv_st == SA_AMF_PRESENCE_INSTANTIATED &&
	    (ev == AVND_COMP_CLC_PRES_FSM_EV_TERM ||
	     ev == AVND_COMP_CLC_PRES_FSM_EV_CLEANUP || ev == AVND_COMP_CLC_PRES_FSM_EV_RESTART)) {

		TRACE("stopping all monitoring for this component");

		/* stop all passive monitoring from this comp */
		avnd_comp_pm_finalize(cb, comp, comp->reg_hdl);

		/* stop command based health check */
		if (comp->is_hc_cmd_configured)
			avnd_comp_hc_cmd_stop(cb, comp);

		/* stop the active monitoring */
		if (comp->is_am_en) {
			rc = avnd_comp_clc_cmd_execute(cb, comp, AVND_COMP_CLC_CMD_TYPE_AMSTOP);
			if (NCSCC_RC_SUCCESS != rc)
				goto done;
		}
	}

	/* can we clean up the proxy now? */
	if ((m_AVND_COMP_TYPE_IS_PROXIED(comp)) &&
			((cb->term_state == AVND_TERM_STATE_OPENSAF_SHUTDOWN_STARTED)  ||
			 (cb->term_state == AVND_TERM_STATE_NODE_FAILOVER_TERMINATING)) &&
			((ev == AVND_COMP_CLC_PRES_FSM_EV_CLEANUP_SUCC ||
			 ev == AVND_COMP_CLC_PRES_FSM_EV_CLEANUP_FAIL)))
	{
		AVND_COMP *proxy = comp->pxy_comp;
		rc = avnd_comp_unreg_prc(cb, comp, proxy);

		/* if proxy got unset then we can continue with proxy's termination */
		if ((rc == NCSCC_RC_SUCCESS) && !m_AVND_COMP_TYPE_IS_PROXY(proxy))
		{
			rc = avnd_comp_clc_fsm_run(avnd_cb, proxy, AVND_COMP_CLC_PRES_FSM_EV_CLEANUP);
		}
	}

	TRACE_1("'%s':Entering CLC FSM: presence state:'%s', Event:'%s'",
					comp->name.value,pres_state[prv_st],pres_state_evt[ev]);

	/* run the fsm */
	if (0 != avnd_comp_clc_fsm[prv_st - 1][ev - 1]) {
		rc = avnd_comp_clc_fsm[prv_st - 1][ev - 1] (cb, comp);
		if (NCSCC_RC_SUCCESS != rc){
			LOG_NO("Component CLC fsm exited with error for comp:%s",comp->name.value);
			goto done;
		}
	}

	/* get the final presence state */
	final_st = comp->pres;

	TRACE_1("Exited CLC FSM");
	TRACE_1("'%s':FSM Enter presence state: '%s':FSM Exit presence state:%s",
					comp->name.value,pres_state[prv_st],pres_state[final_st]);

	/*
	    A restartable component will trigger SU FSM in the context of:
            1.su restart recovery or
            2.restart admin op on su. 
	    In these cases comp will be instantiated only when all the components
            are cleaned up. So after successful cleanup/termination of restarting comp,
	    trigger SU FSM to terminate/cleanup more components or instantiat the SU.
	 */
	if ((prv_st != final_st) || (((prv_st == SA_AMF_PRESENCE_RESTARTING) &&
				(final_st == SA_AMF_PRESENCE_RESTARTING)) &&
				((ev == AVND_COMP_CLC_PRES_FSM_EV_INST_SUCC) ||
				 (ev == AVND_COMP_CLC_PRES_FSM_EV_TERM_SUCC) || 
				 (ev == AVND_COMP_CLC_PRES_FSM_EV_CLEANUP_SUCC)) && 
				(comp->clc_info.exec_cmd == AVND_COMP_CLC_CMD_TYPE_NONE)))
		rc = avnd_comp_clc_st_chng_prc(cb, comp, prv_st, final_st);

 done:
	TRACE_LEAVE2("%u", rc);
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
uint32_t avnd_comp_clc_st_chng_prc(AVND_CB *cb, AVND_COMP *comp, SaAmfPresenceStateT prv_st, SaAmfPresenceStateT final_st)
{
	AVND_SU_PRES_FSM_EV ev = AVND_SU_PRES_FSM_EV_MAX;
	AVND_COMP_CSI_REC *csi = 0;
	bool is_en;
	uint32_t rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER2("Comp '%s', Prv_state '%s', Final_state '%s'", comp->name.value, 
			presence_state[prv_st],presence_state[final_st]);

	/* 
	 * Process state change
	 */

	/* Count the number of restarts if not due to admin restart*/
	if (SA_AMF_PRESENCE_RESTARTING == final_st &&
			prv_st != final_st && comp->admin_oper != true) {
		comp->err_info.restart_cnt++;

		TRACE_1("Component restart not through admin operation");
		/* inform avd of the change in restart count */
		if (cb->is_avd_down == false) {
			avnd_di_uns32_upd_send(AVSV_SA_AMF_COMP, saAmfCompRestartCount_ID,
				&comp->name, comp->err_info.restart_cnt);
		}
	}
	/* reset the admin-oper flag to false */
	if ((comp->admin_oper == true) && (final_st == SA_AMF_PRESENCE_INSTANTIATED)) {
		TRACE_1("Component restart is through admin opration, admin oper flag reset");
		comp->admin_oper = false;
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

	if (comp->su->is_ncs == true) {
		if(SA_AMF_PRESENCE_INSTANTIATION_FAILED == final_st) {
			LOG_ER("'%s'got Inst failed", comp->name.value);
			opensaf_reboot(avnd_cb->node_info.nodeId, (char *)avnd_cb->node_info.executionEnvironment.value,
					"NCS component Instantiation failed");
			LOG_ER("Amfnd is exiting (due to ncs comp inst failed) to aid fast reboot");
			exit(0);
		}
		if(SA_AMF_PRESENCE_TERMINATION_FAILED == final_st) {
			LOG_ER("'%s'got Term failed", comp->name.value);
			opensaf_reboot(avnd_cb->node_info.nodeId, (char *)avnd_cb->node_info.executionEnvironment.value,
					"NCS component Termination failed");
			LOG_ER("Amfnd is exiting (due to ncs comp term failed) to aid fast reboot");
			exit(0);
		}
	}

	/* pi comp in pi su */
	if (m_AVND_SU_IS_PREINSTANTIABLE(comp->su) && m_AVND_COMP_TYPE_IS_PREINSTANTIABLE(comp)) {
		TRACE("SU and Comp Preinst. comp->su->flag '%u', comp->flag '%u'",  comp->su->flag,  comp->flag);
		/* instantiating -> instantiated */
		if ((SA_AMF_PRESENCE_INSTANTIATING == prv_st) && (SA_AMF_PRESENCE_INSTANTIATED == final_st)) {
			m_AVND_COMP_FAILED_RESET(comp);
			m_AVND_COMP_OPER_STATE_SET(comp, SA_AMF_OPERATIONAL_ENABLED);
			m_AVND_COMP_OPER_STATE_AVD_SYNC(cb, comp, rc);
			if (NCSCC_RC_SUCCESS != rc)
				goto done;
			clear_error_report_alarm(comp);
		}

		/* instantiating -> inst-failed/term-failed */
		if ((SA_AMF_PRESENCE_INSTANTIATING == prv_st) && 
				((final_st == SA_AMF_PRESENCE_INSTANTIATION_FAILED) ||
				 (final_st == SA_AMF_PRESENCE_TERMINATION_FAILED))) {
			/* instantiation failed.. log it */
			m_AVND_COMP_OPER_STATE_SET(comp, SA_AMF_OPERATIONAL_DISABLED);
			if (cb->is_avd_down == false) {
				m_AVND_COMP_OPER_STATE_AVD_SYNC(cb, comp, rc);
			}
			m_AVND_COMP_FAILED_SET(comp);
		}

		/* terminating -> term-failed */
		if ((SA_AMF_PRESENCE_TERMINATING == prv_st) && (SA_AMF_PRESENCE_TERMINATION_FAILED == final_st)) {
			/* termination failed.. log it */
		}
		//Instantiated -> Restarting.
		if ((prv_st == SA_AMF_PRESENCE_INSTANTIATED) && (final_st == SA_AMF_PRESENCE_RESTARTING)) {
			/*
			   This presence state transition involving RESTARTING state may originate
			   with or without any SU FSM event :
			   	a)Without SU FSM event: when component is restartable
				   and event is fault or RESTART admin op on component.
			    	b)With SU FSM event: when all comps are restartable and RESTART 
				  admin op on SU. 
			   In the case b) SU FSM takes care of moving presence state of SU to
			   RESTARTING when all of its components (all restartable) are 
			   in RESTARTING state at any given point of time.

			   In case a), SU FSM never gets triggered because restart of component
			   is totally restricted to comp FSM. So in this case comp FSM itself 
			   will have to mark SU's presence state RESTARTING whenever all the components
			   are in Restarting state. This can occur in:
				-confs with single restartable component because of fault with comp-restart 
				 recovery or RESTART admin op on comp. OR
				-confs with all comp restartable when all comps faults with comp-restart recovery.
			   So if I am here because of case a) check if I can mark SU RESTARTING. 		
			 */
			if ((isRestartSet(comp->su) == false) &&
				((m_AVND_COMP_IS_FAILED(comp)) || (comp->admin_oper == true)) &&
				(su_evaluate_restarting_state(comp->su) == true)) {
				TRACE_1("Comp RESTARTING due to comp-restart recovery or RESTART admin op");
				avnd_su_pres_state_set(cb, comp->su, SA_AMF_PRESENCE_RESTARTING);
			}
			 
		}
		/* restarting -> instantiated */
		if ((SA_AMF_PRESENCE_RESTARTING == prv_st) && (SA_AMF_PRESENCE_INSTANTIATED == final_st)) {
			/* reset the comp failed flag & set the oper state to enabled */
			if (m_AVND_COMP_IS_FAILED(comp)) {
				m_AVND_COMP_FAILED_RESET(comp);
				m_AVND_COMP_OPER_STATE_SET(comp, SA_AMF_OPERATIONAL_ENABLED);
				if (cb->is_avd_down == false) {
					m_AVND_COMP_OPER_STATE_AVD_SYNC(cb, comp, rc);
				}
				if (NCSCC_RC_SUCCESS != rc)
					goto done;
				clear_error_report_alarm(comp);
			}

			/* reassign the comp-csis.. 
			   1)if su-restart recovery is not active which means comp was admin restarted
			     or faulted with restart recovery.
			   2)if comp with restartable recovery faulted during the reassignment phase of 
			     of surestart recovery. This can happen when surestart prob timer expires 	 
			     in the surestart recovery phase.
			 */
			if ((!m_AVND_SU_IS_RESTART(comp->su)) || (((m_AVND_SU_IS_RESTART(comp->su))
							&& (!m_AVND_SU_IS_FAILED(comp->su)) &&
                                        (comp->su->admin_op_Id != SA_AMF_ADMIN_RESTART)))) {
				rc = avnd_comp_csi_reassign(cb, comp);
				if (NCSCC_RC_SUCCESS != rc)
					goto done;
				/*
				   Mark SU Instantiated when atleast one component moves to instantiated state.
				   Single comp restarting case or fault of all restartable comps with comp-restart
				   recovery.  For more details read in transition from INSTANTIATED to RESTARTING.
				 */
				if ((comp->su->pres == SA_AMF_PRESENCE_RESTARTING) &&
						(isRestartSet(comp->su) == false)) {
					TRACE_1("Comp INSTANTIATED due to comp-restart recovery or RESTART admin op");
					avnd_su_pres_state_set(cb, comp->su, SA_AMF_PRESENCE_INSTANTIATED);
				}
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
				if (true == is_en) {
					/*Clear SU failed state as all components are enabled.*/
					m_AVND_SU_FAILED_RESET(comp->su);
					m_AVND_SU_OPER_STATE_SET(comp->su, SA_AMF_OPERATIONAL_ENABLED);
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
			if (m_AVND_COMP_IS_FAILED(comp) && !comp->csi_list.n_nodes &&
			    !m_AVND_SU_IS_ADMN_TERM(comp->su) &&
			    (cb->oper_state == SA_AMF_OPERATIONAL_ENABLED)) {
				/* No need to restart component during shutdown, during surestart
				   and during sufailover.It will be instantiated as part of repair.
				   For surestart recovery, SU FSM will instantiate all comps after successful
				   clean up of all of them.*/
				if (!m_AVND_IS_SHUTTING_DOWN(cb) && !sufailover_in_progress(comp->su) &&
						(!m_AVND_SU_IS_RESTART(comp->su)))
					rc = avnd_comp_clc_fsm_trigger(cb, comp, AVND_COMP_CLC_PRES_FSM_EV_INST);
			} else if (m_AVND_COMP_IS_FAILED(comp) && !comp->csi_list.n_nodes) {
				m_AVND_COMP_FAILED_RESET(comp);	/*if we moved from restart -> term
												due to admn operation */
			}
		}
	}

	/* npi comp in pi su */
	if (m_AVND_SU_IS_PREINSTANTIABLE(comp->su) && !m_AVND_COMP_TYPE_IS_PREINSTANTIABLE(comp)) {
		TRACE("SU Preinst and Comp Non-Preinst. comp->su->flag '%u', comp->flag '%u'",  comp->su->flag,  
				comp->flag);
		/* instantiating -> instantiated */
		if ((SA_AMF_PRESENCE_INSTANTIATING == prv_st) && (SA_AMF_PRESENCE_INSTANTIATED == final_st)) {
			/* csi-set succeeded.. generate csi-done indication */
			csi = m_AVND_CSI_REC_FROM_COMP_DLL_NODE_GET(m_NCS_DBLIST_FIND_FIRST(&comp->csi_list));
			osafassert(csi);
			rc = avnd_comp_csi_assign_done(cb, comp, m_AVND_COMP_IS_ALL_CSI(comp) ? 0 : csi);
			if (NCSCC_RC_SUCCESS != rc)
				goto done;
		}

		/* terminating -> uninstantiated */
		if ((SA_AMF_PRESENCE_TERMINATING == prv_st) && (SA_AMF_PRESENCE_UNINSTANTIATED == final_st)) {
			/* npi comps are enabled in uninstantiated state */
			m_AVND_COMP_OPER_STATE_SET(comp, SA_AMF_OPERATIONAL_ENABLED);
			if (cb->is_avd_down == false) {
				m_AVND_COMP_OPER_STATE_AVD_SYNC(cb, comp, rc);
			}
			if (NCSCC_RC_SUCCESS != rc)
				goto done;

			if (sufailover_in_progress(comp->su) || (sufailover_during_nodeswitchover(comp->su))) {
				/* Do not reset any flag during:
				   -sufailover.
				   -nodeswitchover when faulted su has sufailover flag enabled.
				   Reset of all flags will be done as a part of repair.
				 */
			} 
			else { 
				if (!m_AVND_COMP_IS_FAILED(comp)) {
					TRACE("Not a failed comp");
					/* csi-set / csi-rem succeeded.. generate csi-done indication */
					csi = m_AVND_CSI_REC_FROM_COMP_DLL_NODE_GET(m_NCS_DBLIST_FIND_FIRST(&comp->csi_list));
					osafassert(csi);
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
						TRACE("Comp has no CSIs assigned");
						m_AVND_SU_IS_ENABLED(comp->su, is_en);
						if (true == is_en) {
							m_AVND_SU_OPER_STATE_SET(comp->su,SA_AMF_OPERATIONAL_ENABLED);
							rc = avnd_di_oper_send(cb, comp->su, 0);
							if (NCSCC_RC_SUCCESS != rc)
								goto done;
						}
						m_AVND_COMP_FAILED_RESET(comp);
					} else if ((m_AVND_SU_IS_RESTART(comp->su)) && (m_AVND_SU_IS_FAILED(comp->su))) {
						TRACE("suRestart escalation context");
						m_AVND_COMP_FAILED_RESET(comp);
					}
				}
			}
		}

		/* restarting -> instantiated */
		if ((SA_AMF_PRESENCE_RESTARTING == prv_st) && (SA_AMF_PRESENCE_INSTANTIATED == final_st)) {
			/* reset the comp failed flag & set the oper state to enabled */
			if (m_AVND_COMP_IS_FAILED(comp)) {
				m_AVND_COMP_FAILED_RESET(comp);
				m_AVND_COMP_OPER_STATE_SET(comp, SA_AMF_OPERATIONAL_ENABLED);
				if (cb->is_avd_down == false) {
					m_AVND_COMP_OPER_STATE_AVD_SYNC(cb, comp, rc);
				}
				if (NCSCC_RC_SUCCESS != rc)
					goto done;
				clear_error_report_alarm(comp);
			}

			/* csi-set succeeded.. generate csi-done indication */
			csi = m_AVND_CSI_REC_FROM_COMP_DLL_NODE_GET(m_NCS_DBLIST_FIND_FIRST(&comp->csi_list));
			osafassert(csi);

			if (!m_AVND_COMP_CSI_CURR_ASSIGN_STATE_IS_UNASSIGNED(csi)) {
				rc = avnd_comp_csi_assign_done(cb, comp, m_AVND_COMP_IS_ALL_CSI(comp) ? 0 : csi);
				if (NCSCC_RC_SUCCESS != rc)
					goto done;
			}
		}

		/* instantiating -> instantiation failed */
		if ((SA_AMF_PRESENCE_INSTANTIATING == prv_st) && (SA_AMF_PRESENCE_INSTANTIATION_FAILED == final_st)) {
			m_AVND_COMP_FAILED_SET(comp);

			/* update comp oper state */
			m_AVND_COMP_OPER_STATE_SET(comp, SA_AMF_OPERATIONAL_DISABLED);
			if (cb->is_avd_down == false) {
				m_AVND_COMP_OPER_STATE_AVD_SYNC(cb, comp, rc);
			}

			m_AVND_SU_FAILED_SET(comp->su);
			/* csi-set Failed.. Respond failure for Su-Si */
			csi = m_AVND_CSI_REC_FROM_COMP_DLL_NODE_GET(m_NCS_DBLIST_FIND_FIRST(&comp->csi_list));
			osafassert(csi);
			avnd_di_susi_resp_send(cb, comp->su, m_AVND_COMP_IS_ALL_CSI(comp) ? 0 : csi->si);
		}

	}

	/* npi comp in npi su */
	if (!m_AVND_SU_IS_PREINSTANTIABLE(comp->su) && !m_AVND_COMP_TYPE_IS_PREINSTANTIABLE(comp)) {
		TRACE("SU and Comp Non-Preinst. comp->su->flag '%u', comp->flag '%u'",  comp->su->flag,  
				comp->flag);
		/* restarting -> instantiated */
		if ((SA_AMF_PRESENCE_RESTARTING == prv_st) && (SA_AMF_PRESENCE_INSTANTIATED == final_st)) {
			m_AVND_COMP_FAILED_RESET(comp);
			m_AVND_COMP_OPER_STATE_SET(comp, SA_AMF_OPERATIONAL_ENABLED);
			if (cb->is_avd_down == false) {
				m_AVND_COMP_OPER_STATE_AVD_SYNC(cb, comp, rc);
			}
			if (NCSCC_RC_SUCCESS != rc)
				goto done;
			clear_error_report_alarm(comp);
			/*
			   Mark SU Instantiated when atleast one component moves to instantiated state.
			   Single comp restarting case or fault of all restartable comps with comp-restart
			   recovery. Please read detailed explanation in state transition for PI 
			   comp in PI SU from INSTANTIATED to RESTARTING in this function.
			 */
			if ((comp->su->pres == SA_AMF_PRESENCE_RESTARTING)
					&& (isRestartSet(comp->su) == false)) {
				TRACE_1("Comp INSTANTIATED due to comp-restart recovery or RESTART admin op");
				avnd_su_pres_state_set(cb, comp->su, SA_AMF_PRESENCE_INSTANTIATED);
			}
			csi = m_AVND_CSI_REC_FROM_COMP_DLL_NODE_GET(m_NCS_DBLIST_FIND_FIRST(&comp->csi_list));
			//Mark CSI ASSIGNED in case of comp-restart recovery and RESTART admin op on comp.
			if ((isRestartSet(comp->su) == false) &&
					(m_AVND_COMP_CSI_CURR_ASSIGN_STATE_IS_RESTARTING(csi)))
				m_AVND_COMP_CSI_CURR_ASSIGN_STATE_SET(csi, AVND_COMP_CSI_ASSIGN_STATE_ASSIGNED);
		}

		/* terminating -> uninstantiated */
		if ((SA_AMF_PRESENCE_TERMINATING == prv_st) && (SA_AMF_PRESENCE_UNINSTANTIATED == final_st)) {
			if (sufailover_in_progress(comp->su)) {
				/*Do not reset any flag, this will be done as a part of repair.*/
			} 
			else {
				/* npi comps are enabled in uninstantiated state */
				m_AVND_COMP_OPER_STATE_SET(comp, SA_AMF_OPERATIONAL_ENABLED);
				if (cb->is_avd_down == false) {
					m_AVND_COMP_OPER_STATE_AVD_SYNC(cb, comp, rc);
				}
				if (NCSCC_RC_SUCCESS != rc)
					goto done;
			}
		}

		/* Instantiating -> Instantiationfailed/Terminationfailed */
		if ((SA_AMF_PRESENCE_INSTANTIATING == prv_st) && 
				((final_st == SA_AMF_PRESENCE_INSTANTIATION_FAILED) ||
				 (final_st == SA_AMF_PRESENCE_TERMINATION_FAILED))) {
			m_AVND_COMP_FAILED_SET(comp);
			m_AVND_COMP_OPER_STATE_SET(comp, SA_AMF_OPERATIONAL_DISABLED);
			if (cb->is_avd_down == false) {
				m_AVND_COMP_OPER_STATE_AVD_SYNC(cb, comp, rc);
			}
			if (NCSCC_RC_SUCCESS != rc)
				goto done;
		}
		//Instantiated -> Restarting.
                if ((prv_st == SA_AMF_PRESENCE_INSTANTIATED) && (final_st == SA_AMF_PRESENCE_RESTARTING)) {
			//Please read detailed explanation in same state transition for PI comp in PI SU.

			csi = m_AVND_CSI_REC_FROM_COMP_DLL_NODE_GET(m_NCS_DBLIST_FIND_FIRST(&comp->csi_list));
                        if ((isRestartSet(comp->su) == false) &&
                                ((m_AVND_COMP_IS_FAILED(comp)) || (comp->admin_oper == true))) {
				//Check if all COMPCSIs are in RESTARTING state execpt this compcsi. 
				if (all_csis_in_restarting_state(comp->su, csi) == true) {
					TRACE_1("Comp RESTARTING due to comp-restart recovery or RESTART admin op");
					avnd_su_pres_state_set(cb, comp->su, SA_AMF_PRESENCE_RESTARTING);
				}
			}

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
				avnd_comp_cbq_rec_pop_and_del(cb, comp, curr_rec, true);
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

				/* send it */
				rc = avnd_comp_cbq_rec_send(cb, comp, curr_rec, true);
				if (NCSCC_RC_SUCCESS != rc && curr_rec) {
					avnd_comp_cbq_rec_pop_and_del(cb, comp, curr_rec, true);
				}
			}	/* while loop */
		}
	}

	 /*Trigger the SU FSM from a NPI component in PI SU.*/
	if (m_AVND_SU_IS_PREINSTANTIABLE(comp->su) && !m_AVND_COMP_TYPE_IS_PREINSTANTIABLE(comp)) {

		if (SA_AMF_PRESENCE_INSTANTIATION_FAILED == final_st)
			ev = AVND_SU_PRES_FSM_EV_COMP_INST_FAIL;
		else if (SA_AMF_PRESENCE_TERMINATION_FAILED == final_st)
			ev = AVND_SU_PRES_FSM_EV_COMP_TERM_FAIL;
		else if ((SA_AMF_PRESENCE_TERMINATING == final_st) && (comp->su->pres == SA_AMF_PRESENCE_RESTARTING))
			ev = AVND_SU_PRES_FSM_EV_COMP_TERMINATING;
		else if ((sufailover_in_progress(comp->su) || (m_AVND_SU_IS_RESTART(comp->su)) || 
					(avnd_cb->term_state == AVND_TERM_STATE_NODE_SWITCHOVER_STARTED) || 
					(all_comps_terminated_in_su(comp->su) == true)) &&
				(SA_AMF_PRESENCE_UNINSTANTIATED == final_st))
			/* If sufailover flag is enabled, then SU FSM needs to be triggered in 
			   sufailover, surestart and nodeswitchover escalation. Also in case of
			   restart admin operation on a non restartable component or SU trigger SU FSM.
			 */
			ev = AVND_SU_PRES_FSM_EV_COMP_UNINSTANTIATED;
		else if ((final_st == SA_AMF_PRESENCE_RESTARTING) && (prv_st == SA_AMF_PRESENCE_RESTARTING) &&
                                (comp->su->admin_op_Id == SA_AMF_ADMIN_RESTART))
			/* If a restartable PI SU consists of NPI comp, then trigger SU FSM to terminate
		          other components or start instnatiation.*/
			ev = AVND_SU_PRES_FSM_EV_COMP_RESTARTING;
	}

	 /* Trigger the SU FSM from a PI component in PI SU or
	    from a NPI component in any SU.*/
	if ((m_AVND_SU_IS_PREINSTANTIABLE(comp->su) &&
	     m_AVND_COMP_TYPE_IS_PREINSTANTIABLE(comp)) || !m_AVND_SU_IS_PREINSTANTIABLE(comp->su)) {
		if (SA_AMF_PRESENCE_UNINSTANTIATED == final_st)
			ev = AVND_SU_PRES_FSM_EV_COMP_UNINSTANTIATED;
		else if ((SA_AMF_PRESENCE_INSTANTIATED == final_st) && (SA_AMF_PRESENCE_ORPHANED != prv_st) &&
				((prv_st == SA_AMF_PRESENCE_INSTANTIATING) || 
				 (prv_st == SA_AMF_PRESENCE_TERMINATING) ||
				 (comp->su->admin_op_Id == SA_AMF_ADMIN_RESTART)))
			ev = AVND_SU_PRES_FSM_EV_COMP_INSTANTIATED;
		else if (SA_AMF_PRESENCE_INSTANTIATION_FAILED == final_st)
			ev = AVND_SU_PRES_FSM_EV_COMP_INST_FAIL;
		else if (SA_AMF_PRESENCE_TERMINATION_FAILED == final_st)
			ev = AVND_SU_PRES_FSM_EV_COMP_TERM_FAIL;
		else if ((SA_AMF_PRESENCE_RESTARTING == final_st) &&
				(prv_st == SA_AMF_PRESENCE_RESTARTING) && (comp->admin_oper == false) &&
				(comp->su->admin_op_Id == SA_AMF_ADMIN_RESTART))
			/* A restartable component is restarting state will not trigger SU FSM:
			   - when it has faulted with comprestart recovery or
			   - when it is admin restarted.
			   But in the context of restart admin operatin on SU, trigger SU FSM
			   to make way for furter termination or instantiation. */
			ev = AVND_SU_PRES_FSM_EV_COMP_RESTARTING;
		else if (SA_AMF_PRESENCE_TERMINATING == final_st)
			ev = AVND_SU_PRES_FSM_EV_COMP_TERMINATING;
	}

	if (AVND_SU_PRES_FSM_EV_MAX != ev)
		rc = avnd_su_pres_fsm_run(cb, comp->su, comp, ev);
	if (NCSCC_RC_SUCCESS != rc)
		goto done;

	/* Possibly start some monitoring */
	if (SA_AMF_PRESENCE_INSTANTIATED == final_st && SA_AMF_PRESENCE_ORPHANED != prv_st) {
		if (comp->is_am_en) {
			rc = avnd_comp_am_start(cb, comp);
			if (NCSCC_RC_SUCCESS != rc)
				goto done;
		}

		if (comp->is_hc_cmd_configured) {
			rc = avnd_comp_hc_cmd_start(cb, comp);
			if (NCSCC_RC_SUCCESS != rc)
				goto done;
		}
	}

	/* This is a case of 'component delete' when su is in intantiated
	   state. Component can go into uninstantiated state or termination
	   failed state, but the component information needs to be deleted from
	   data base. */
	if ((comp->pending_delete == true) &&
			((comp->pres == SA_AMF_PRESENCE_UNINSTANTIATED) ||
			 (comp->pres == SA_AMF_PRESENCE_TERMINATION_FAILED)) &&
			(comp->su->pres == SA_AMF_PRESENCE_INSTANTIATED))
		rc = avnd_compdb_rec_del(cb, &comp->name);

 done:
	TRACE_LEAVE2("%u", rc);
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
uint32_t avnd_comp_clc_uninst_inst_hdler(AVND_CB *cb, AVND_COMP *comp)
{
	uint32_t rc = NCSCC_RC_SUCCESS;

	TRACE_ENTER2("'%s' : Instantiate event in the Uninstantiated state", comp->name.value);

	/* Refresh the component configuration, it may have changed */
	if (!m_AVND_IS_SHUTTING_DOWN(cb) && (avnd_comp_config_reinit(comp) != 0)) {
		rc = NCSCC_RC_FAILURE;
		goto done;
	}

	/*if proxied component check whether the proxy exists, if so continue 
	   instantiating by calling the proxied callback. else start timer and 
	   wait for inst timeout duration */
	if (m_AVND_COMP_TYPE_IS_PROXIED(comp)) {
		if (comp->pxy_comp != nullptr) {
			if (m_AVND_COMP_TYPE_IS_PREINSTANTIABLE(comp))
				/* call the proxied instantiate callback   */
				rc = avnd_comp_cbk_send(cb, comp, AVSV_AMF_PXIED_COMP_INST, 0, 0);
			else
				/* do a csi set with active ha state */
				rc = avnd_comp_cbk_send(cb, comp, AVSV_AMF_CSI_SET, 0, 0);
			if (NCSCC_RC_SUCCESS == rc) {
				/* increment the retry count */
				comp->clc_info.inst_retry_cnt++;
			}
		} else {
			m_AVND_TMR_PXIED_COMP_INST_START(cb, *comp, rc);	/* start a timer for proxied instantiating timeout duration */
		}

		/* transition to 'instantiating' state */
		avnd_comp_pres_state_set(cb, comp, SA_AMF_PRESENCE_INSTANTIATING);

		goto done;
	}

	/* instantiate the comp */
	rc = avnd_comp_clc_cmd_execute(cb, comp, AVND_COMP_CLC_CMD_TYPE_INSTANTIATE);

	if (NCSCC_RC_SUCCESS == rc) {
		/* timestamp the start of this instantiation phase */
		m_GET_TIME_STAMP(comp->clc_info.inst_cmd_ts);

		/* increment the retry count */
		comp->clc_info.inst_retry_cnt++;

		/* transition to 'instantiating' state */
		avnd_comp_pres_state_set(cb, comp, SA_AMF_PRESENCE_INSTANTIATING);
	}

done:
	TRACE_LEAVE2("%u", rc);
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
uint32_t avnd_comp_clc_insting_inst_hdler(AVND_CB *cb, AVND_COMP *comp)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER2("'%s': Instantiate event in the Instantiating state", comp->name.value);

	/* increment the retry count */
	comp->clc_info.inst_retry_cnt++;

	if (m_AVND_COMP_TYPE_IS_PREINSTANTIABLE(comp)){
		TRACE("component type is preinstantiable");
		/* call the proxied instantiate callback, start a timer for callback responce */
		rc = avnd_comp_cbk_send(cb, comp, AVSV_AMF_PXIED_COMP_INST, 0, 0);
	}
	else{
		TRACE("component is NPI: performing a CSI set with ACTIVE HA state");
		/* do a csi set with active ha state */
		rc = avnd_comp_cbk_send(cb, comp, AVSV_AMF_CSI_SET, 0, 0);
	}

	/* no state transition */

	TRACE_LEAVE();
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
uint32_t avnd_comp_clc_insting_instsucc_hdler(AVND_CB *cb, AVND_COMP *comp)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER2("'%s': Instantiate success event in the Instantiating state", comp->name.value);

	/* stop the reg tmr */
	if (m_AVND_TMR_IS_ACTIVE(comp->clc_info.clc_reg_tmr))
		m_AVND_TMR_COMP_REG_STOP(cb, *comp);

	/* reset the retry count */
	comp->clc_info.inst_retry_cnt = 0;

	/* transition to 'instantiated' state */
	avnd_comp_pres_state_set(cb, comp, SA_AMF_PRESENCE_INSTANTIATED);

	TRACE_LEAVE();
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
uint32_t avnd_comp_clc_xxxing_instfail_hdler(AVND_CB *cb, AVND_COMP *comp)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER2("'%s': Instantiate fail event in Instantiating/Restarting State", comp->name.value);

	/* reset the comp-reg & instantiate params */
	if (!m_AVND_COMP_TYPE_IS_PROXIED(comp))
		m_AVND_COMP_REG_PARAM_RESET(cb, comp);
	m_AVND_COMP_CLC_INST_PARAM_RESET(comp);

	if (m_AVND_COMP_TYPE_IS_PROXY(comp))
		rc = avnd_comp_proxy_unreg(cb, comp);

	/* delete hc-list, cbk-list, pg-list & pm-list */
	avnd_comp_hc_rec_del_all(cb, comp);
	avnd_comp_cbq_del(cb, comp, true);

	/* re-using the funtion to stop all PM started by this comp */
	avnd_comp_pm_finalize(cb, comp, comp->reg_hdl);
	avnd_comp_pm_rec_del_all(cb, comp);	/*if at all anythnig is left behind */

	/* no state transition */

	/* cleanup the comp */
	if (m_AVND_COMP_TYPE_IS_PROXIED(comp) && comp->pxy_comp != 0)
		rc = avnd_comp_cbk_send(cb, comp, AVSV_AMF_PXIED_COMP_CLEAN, 0, 0);
	else
		rc = avnd_comp_clc_cmd_execute(cb, comp, AVND_COMP_CLC_CMD_TYPE_CLEANUP);

	TRACE_LEAVE2("%u", rc);
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
uint32_t avnd_comp_clc_insting_term_hdler(AVND_CB *cb, AVND_COMP *comp)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER2("'%s': Terminate event in the Instantiating state", comp->name.value);

	/* as the comp has not fully instantiated, it is cleaned up */
	rc = avnd_comp_clc_cmd_execute(cb, comp, AVND_COMP_CLC_CMD_TYPE_CLEANUP);
	if (NCSCC_RC_SUCCESS == rc) {
		/* reset the comp-reg & instantiate params */
		m_AVND_COMP_REG_PARAM_RESET(cb, comp);
		m_AVND_COMP_CLC_INST_PARAM_RESET(comp);

		/* delete hc-list, cbk-list, pg-list & pm-list */
		avnd_comp_hc_rec_del_all(cb, comp);
		avnd_comp_cbq_del(cb, comp, true);

		/* re-using the funtion to stop all PM started by this comp */
		avnd_comp_pm_finalize(cb, comp, comp->reg_hdl);
		avnd_comp_pm_rec_del_all(cb, comp);	/*if at all anythnig is left behind */

		/* transition to 'terminating' state */
		avnd_comp_pres_state_set(cb, comp, SA_AMF_PRESENCE_TERMINATING);
	}

	TRACE_LEAVE();
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
uint32_t avnd_comp_clc_insting_clean_hdler(AVND_CB *cb, AVND_COMP *comp)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER2("'%s': Cleanup event in the Instantiating state", comp->name.value);

	/* cleanup the comp */
	rc = avnd_comp_clc_cmd_execute(cb, comp, AVND_COMP_CLC_CMD_TYPE_CLEANUP);
	if (NCSCC_RC_SUCCESS == rc) {
		/* reset the comp-reg & instantiate params */
		m_AVND_COMP_REG_PARAM_RESET(cb, comp);
		m_AVND_COMP_CLC_INST_PARAM_RESET(comp);

		m_AVND_COMP_TERM_FAIL_RESET(comp);

		/* transition to 'terminating' state */
		avnd_comp_pres_state_set(cb, comp, SA_AMF_PRESENCE_TERMINATING);
	}

	TRACE_LEAVE();
	return rc;
}

/**
 * @brief  Checks for eligibility of a failed component for instantiation 
 *	   when it has been successfully cleaned up. Instantiation depends
 *	   upon recovery context or Restart admin op. 
 *	   
 * @return true/false
 */
static bool is_failed_comp_eligible_for_instantiation(AVND_COMP *comp) {

  if (isRestartSet(comp->su)) { //SU is restarting (RESTART admin op or recovery policy).
	  if (isFailed(comp->su)) { //SU is failed (case surestart recovery).
		  /*During surestart recovery, after cleanup of all components, amfnd starts 
		    instantiation of components. A component may fault at this stage. Such a 
		    component is eligible for instantiation.*/
		  if ((comp->pres == SA_AMF_PRESENCE_INSTANTIATING) &&
			  (comp->su->pres == SA_AMF_PRESENCE_INSTANTIATING))
				return true;

		  /*This component is being cleaned up because of su restart recovery. 
		    In this case component is not eligible for instantiation.*/
		  if ((comp->pres == SA_AMF_PRESENCE_RESTARTING) &&
				  (comp->su->pres == SA_AMF_PRESENCE_TERMINATING)) {
			  /* Component got cleaned up in RESTARTING state and surestart recovery is going on.
			     In surestart recovery component never enters in RESTARTING state. This means
			     component was cleaned up in the context of comp-restart recovery.
			     Since further escalation has reached to surestart, same cleanup can be used
			     and thus comp can be marked uninstantiated.*/
			  avnd_comp_pres_state_set(avnd_cb, comp, SA_AMF_PRESENCE_UNINSTANTIATED);
			  return false;
		 }
	  } else { //Case of RESTART admin op or assignment phase of surestart recovery.
		if (isAdminRestarted(comp->su)) { //RESTART admin op.
			/*During RESTART admin op on SU, after termination of all components, amfnd
			  starts instantiation of components. A component may fault at this stage. 
			  Such a component is eligible for instantiation.*/
			if  ((comp->pres == SA_AMF_PRESENCE_RESTARTING) ||
				(comp->pres == SA_AMF_PRESENCE_INSTANTIATING)) 
				return true;
		} else {//Assignment phase during RESTART admin op or during surestart recovery. 
			/*After successful instantiation of all the components of SU because of either
			  su restart recovery or RESTART admin op on SU, amfnd starts reassigning
			  the components in SU. A component may fault during reassignment. Such a
			  component is eligible for instantiation.*/
			return true;
		}
	  }
  } else {//SU is not restarting.
	 if (!isFailed(comp->su)) {//SU is not failed.
		 /*A failed component is eligible for instantiation 
                   in the context of comp-restart recovery.*/ 
		return true;
	 }
  }

  return false;
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
uint32_t avnd_comp_clc_xxxing_cleansucc_hdler(AVND_CB *cb, AVND_COMP *comp)
{
	AVND_COMP_CLC_INFO *clc_info = &comp->clc_info;
	uint32_t rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER2("'%s': Cleanup success event in the instantiating/restarting state", comp->name.value);
	/* Refresh the component configuration, it may have changed */
	if (!m_AVND_IS_SHUTTING_DOWN(cb) && (avnd_comp_config_reinit(comp) != 0)) {
		rc = NCSCC_RC_FAILURE;
		goto done;
	}

	/*
	 *  su-sis may be in assigning/removing state. signal csi
	 * assign/remove done so that su-si assignment/removal algo can proceed.
	 */
	avnd_comp_cmplete_all_assignment(cb, comp);

	//Comp instantiation depends on recovery policy. Check for instantiation eligibility.
	if (is_failed_comp_eligible_for_instantiation(comp) == false)
		goto done;

	TRACE("inst_retry_cnt:%u, inst_retry_max:%u",clc_info->inst_retry_cnt,clc_info->inst_retry_max);
	if ((clc_info->inst_retry_cnt < clc_info->inst_retry_max) &&
	    (AVND_COMP_INST_EXIT_CODE_NO_RETRY != clc_info->inst_code_rcvd)) {
		/* => keep retrying */
		/* instantiate the comp */
		if (m_AVND_COMP_TYPE_IS_PROXIED(comp)) {
			if (comp->pxy_comp != nullptr) {
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
			}
		} else
			rc = avnd_comp_clc_cmd_execute(cb, comp, AVND_COMP_CLC_CMD_TYPE_INSTANTIATE);

		if (NCSCC_RC_SUCCESS == rc) {
			/* timestamp the start of this instantiation phase */
			m_GET_TIME_STAMP(clc_info->inst_cmd_ts);

			/* increment the retry count */
			clc_info->inst_retry_cnt++;
		}
	} else {
		/* stop the inst timer, inst timer might still be running */
		if (m_AVND_COMP_TYPE_IS_PROXIED(comp) && m_AVND_TMR_IS_ACTIVE(comp->clc_info.clc_reg_tmr)) {
			m_AVND_TMR_PXIED_COMP_INST_STOP(cb, *comp);
		}
		/* => retries over... transition to inst-failed state */
		avnd_comp_pres_state_set(cb, comp, SA_AMF_PRESENCE_INSTANTIATION_FAILED);
	}
done:
	TRACE_LEAVE();
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
uint32_t avnd_comp_clc_insting_cleanfail_hdler(AVND_CB *cb, AVND_COMP *comp)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER2("'%s': Cleanup Fail event in the instantiating state", comp->name.value);

	/* nothing can be done now.. just transition to term-failed state */
	avnd_comp_pres_state_set(cb, comp, SA_AMF_PRESENCE_TERMINATION_FAILED);

	TRACE_LEAVE();
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
uint32_t avnd_comp_clc_insting_restart_hdler(AVND_CB *cb, AVND_COMP *comp)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER2("'%s': Restart event in the Instantiating state", comp->name.value);

	/* cleanup the comp */
	rc = avnd_comp_clc_cmd_execute(cb, comp, AVND_COMP_CLC_CMD_TYPE_CLEANUP);
	if (NCSCC_RC_SUCCESS == rc) {
		/* reset the comp-reg & instantiate params */
		m_AVND_COMP_REG_PARAM_RESET(cb, comp);
		m_AVND_COMP_CLC_INST_PARAM_RESET(comp);

		/* transition to 'restarting' state */
		avnd_comp_pres_state_set(cb, comp, SA_AMF_PRESENCE_RESTARTING);
	}

	TRACE_LEAVE();
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
uint32_t avnd_comp_clc_inst_term_hdler(AVND_CB *cb, AVND_COMP *comp)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER2("'%s': Terminate event in the Instantiated state", comp->name.value);

	/* terminate the comp */
	if (m_AVND_COMP_TYPE_IS_PROXIED(comp) && !m_AVND_COMP_TYPE_IS_PREINSTANTIABLE(comp))
		/* invoke csi remove callback */
		rc = avnd_comp_cbk_send(cb, comp, AVSV_AMF_CSI_REM, 0, 0);
	else if (m_AVND_COMP_TYPE_IS_SAAWARE(comp) && (!m_AVND_COMP_IS_REG(comp))) {
		rc = avnd_comp_clc_cmd_execute(cb, comp, AVND_COMP_CLC_CMD_TYPE_CLEANUP);
		m_AVND_COMP_REG_PARAM_RESET(cb, comp);

		/* delete hc-list, cbk-list, pg-list & pm-list */
		avnd_comp_hc_rec_del_all(cb, comp);
		avnd_comp_cbq_del(cb, comp, true);

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
	}

	if (NCSCC_RC_SUCCESS == rc) {
		/* reset the comp-reg & instantiate params */
		m_AVND_COMP_CLC_INST_PARAM_RESET(comp);

		/* transition to 'terminating' state */
		avnd_comp_pres_state_set(cb, comp, SA_AMF_PRESENCE_TERMINATING);
	}

	TRACE_LEAVE();
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
uint32_t avnd_comp_clc_inst_clean_hdler(AVND_CB *cb, AVND_COMP *comp)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER2("'%s': Cleanup event in the instantiated state", comp->name.value);

	if (m_AVND_COMP_TYPE_IS_PROXIED(comp)) {
		avnd_comp_cbq_del(cb, comp, true);
		/* call the cleanup callback */
		rc = avnd_comp_cbk_send(cb, comp, AVSV_AMF_PXIED_COMP_CLEAN, 0, 0);
	} else if (m_AVND_COMP_TYPE_IS_PROXY(comp) && comp->pxied_list.n_nodes) {
		/* if there are still outstanding proxied components we can't terminate right now */
		return rc;
	} else {
		if (m_AVND_SU_IS_RESTART(comp->su) && m_AVND_COMP_IS_RESTART_DIS(comp) &&
				(comp->csi_list.n_nodes > 0) &&
				(m_AVND_SU_IS_PREINSTANTIABLE(comp->su) &&
				 (comp->su->su_err_esc_level != AVND_ERR_ESC_LEVEL_2))) {
			/* A non-restartable assigned healthy component(DisableRestart=1) and
			   context is surestart recovery, first perform reassignment for this 
			   component to other SU then clean it up.
			   At present assignment of whole SU will be gracefully reassigned 
			   instead of only this comp. Thus for PI applications modeled on NWay and 
			   Nway Active model this is spec deviation.
			   So as of now clean up of components due to surestart recovery policy 
			   is halted to remove assignment from a healthy non restartable comp.
 			   Further cleanup will be resumed after removal of assignments. 
			 */
			/*Send amfd to gracefully remove assignments for thus SU.*/
			su_send_suRestart_recovery_msg(comp->su);
			goto done;
		} else
			rc = avnd_comp_clc_cmd_execute(cb, comp, AVND_COMP_CLC_CMD_TYPE_CLEANUP);
	}
	if (NCSCC_RC_SUCCESS == rc) {
		/* reset the comp-reg & instantiate params */
		if (!m_AVND_COMP_TYPE_IS_PROXIED(comp))
			m_AVND_COMP_REG_PARAM_RESET(cb, comp);
		m_AVND_COMP_CLC_INST_PARAM_RESET(comp);

		m_AVND_COMP_TERM_FAIL_RESET(comp);
		/* transition to 'terminating' state */
		avnd_comp_pres_state_set(cb, comp, SA_AMF_PRESENCE_TERMINATING);
	}

done:
	TRACE_LEAVE();
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
uint32_t avnd_comp_clc_inst_restart_hdler(AVND_CB *cb, AVND_COMP *comp)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER2("'%s': Restart event in the instantiated state", comp->name.value);

	/* terminate / cleanup the comp */
	if (m_AVND_COMP_IS_FAILED(comp) && m_AVND_COMP_TYPE_IS_PROXIED(comp))
		rc = avnd_comp_cbk_send(cb, comp, AVSV_AMF_PXIED_COMP_CLEAN, 0, 0);
	else if (m_AVND_COMP_IS_FAILED(comp)) {
		rc = avnd_comp_clc_cmd_execute(cb, comp, AVND_COMP_CLC_CMD_TYPE_CLEANUP);
		m_AVND_COMP_REG_PARAM_RESET(cb, comp);
	} else {
		if (m_AVND_COMP_TYPE_IS_PROXIED(comp) && !m_AVND_COMP_TYPE_IS_PREINSTANTIABLE(comp))
			/* invoke csi set callback with active ha state */
			rc = avnd_comp_cbk_send(cb, comp, AVSV_AMF_CSI_REM, 0, 0);
		else if (m_AVND_COMP_TYPE_IS_SAAWARE(comp) && (!m_AVND_COMP_IS_REG(comp))) {
			rc = avnd_comp_clc_cmd_execute(cb, comp, AVND_COMP_CLC_CMD_TYPE_CLEANUP);
			m_AVND_COMP_REG_PARAM_RESET(cb, comp);

			/* delete hc-list, cbk-list, pg-list & pm-list */
			avnd_comp_hc_rec_del_all(cb, comp);
			avnd_comp_cbq_del(cb, comp, true);

			/* re-using the funtion to stop all PM started by this comp */
			avnd_comp_pm_finalize(cb, comp, comp->reg_hdl);
			avnd_comp_pm_rec_del_all(cb, comp);	/*if at all anythnig is left behind */
		} else if (m_AVND_COMP_TYPE_IS_SAAWARE(comp) ||
			   (m_AVND_COMP_TYPE_IS_PROXIED(comp) && m_AVND_COMP_TYPE_IS_PREINSTANTIABLE(comp)))
			if (m_AVND_COMP_IS_RESTART_DIS(comp) && (comp->csi_list.n_nodes > 0)) {
				/* A non-restartable assigned healthy component(DisableRestart=1) and
				   context is restart admin op on su or this comp itself,
				   first perform reassignment for this component to other SU then term it.
				   At present assignment of whole SU will be gracefully reassigned
				   instead of only this comp. Thus for PI applications modeled on NWay and
				   Nway Active model this is spec deviation.
				 */
				/*Send amfd to gracefully remove assignments for thus SU.*/
				su_send_suRestart_recovery_msg(comp->su);
				goto done;
			} else
				/* invoke terminate callback */
				rc = avnd_comp_cbk_send(cb, comp, AVSV_AMF_COMP_TERM, 0, 0);
		else {
			rc = avnd_comp_clc_cmd_execute(cb, comp, AVND_COMP_CLC_CMD_TYPE_TERMINATE);
			m_AVND_COMP_REG_PARAM_RESET(cb, comp);
		}
	}

	if (NCSCC_RC_SUCCESS == rc) {
		/* reset the comp-reg & instantiate params */

		/* For proxied components, registration is treated as 
		   "some one is proxying for him", so we need not reset the
		   reg parameters 
		 */

		m_AVND_COMP_CLC_INST_PARAM_RESET(comp);

		/* If DisableRestart=0 then transition to 'restarting' state and 
		   DisableRestart=1 then  transition to 'terminating' state */
		if (!m_AVND_COMP_IS_RESTART_DIS(comp))
			avnd_comp_pres_state_set(cb, comp, SA_AMF_PRESENCE_RESTARTING);
		else
			avnd_comp_pres_state_set(cb, comp, SA_AMF_PRESENCE_TERMINATING);
	}
done:
	TRACE_LEAVE();
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
uint32_t avnd_comp_clc_inst_orph_hdler(AVND_CB *cb, AVND_COMP *comp)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER2("'%s': Orphaned event in the Instantiated state", comp->name.value);

	/* start the orphaned timer */
	m_AVND_TMR_PXIED_COMP_REG_START(cb, *comp, rc);

	if (NCSCC_RC_SUCCESS == rc) {
		avnd_comp_pres_state_set(cb, comp, static_cast<SaAmfPresenceStateT>(SA_AMF_PRESENCE_ORPHANED));
	}

	TRACE_LEAVE();
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
uint32_t avnd_comp_clc_terming_termsucc_hdler(AVND_CB *cb, AVND_COMP *comp)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER2("'%s': Terminate success event in the terminating state", comp->name.value);

	/* just transition to 'uninstantiated' state */
	avnd_comp_pres_state_set(cb, comp, SA_AMF_PRESENCE_UNINSTANTIATED);

	/* reset the comp-reg & instantiate params */
	if (!m_AVND_COMP_TYPE_IS_PROXIED(comp)) {
		m_AVND_COMP_REG_PARAM_RESET(cb, comp);
		avnd_comp_curr_info_del(cb, comp);
	}

	TRACE_LEAVE();
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
uint32_t avnd_comp_clc_terming_termfail_hdler(AVND_CB *cb, AVND_COMP *comp)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER2("'%s': Terminate fail event in the terminating state", comp->name.value);

	if (m_AVND_COMP_TYPE_IS_PROXIED(comp))
		avnd_comp_cbq_del(cb, comp, true);

	/* cleanup the comp */
	if (m_AVND_COMP_TYPE_IS_PROXIED(comp) && comp->pxy_comp != 0)
		rc = avnd_comp_cbk_send(cb, comp, AVSV_AMF_PXIED_COMP_CLEAN, 0, 0);
	else
		rc = avnd_comp_clc_cmd_execute(cb, comp, AVND_COMP_CLC_CMD_TYPE_CLEANUP);

	m_AVND_COMP_TERM_FAIL_RESET(comp);

	/* reset the comp-reg & instantiate params */
	if (!m_AVND_COMP_TYPE_IS_PROXIED(comp)) {
		m_AVND_COMP_REG_PARAM_RESET(cb, comp);
	}

	TRACE_LEAVE();
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
uint32_t avnd_comp_clc_terming_cleansucc_hdler(AVND_CB *cb, AVND_COMP *comp)
{
	const AVND_SU *su = comp->su;
	uint32_t rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER2("'%s': Cleanup success event in the terminating state", comp->name.value);

	/* just transition to 'uninstantiated' state */
	avnd_comp_pres_state_set(cb, comp, SA_AMF_PRESENCE_UNINSTANTIATED);

	if (AVND_TERM_STATE_OPENSAF_SHUTDOWN_STARTED == cb->term_state) {
		/*
		 * If for example a CSI remove callback times out during system shutdown
		 * we end up here. comp is terminated, indicate that all its CSIs are
		 * removed so we can proceed with the next step in the shutdown sequence.
		 */
		/* This is only for PI SU. */
		if ((!comp->su->is_ncs) && (comp->csi_list.n_nodes > 0) && (m_AVND_SU_IS_PREINSTANTIABLE(comp->su))) {
			AVND_COMP_CSI_REC *csi;

			for (csi = m_AVND_CSI_REC_FROM_COMP_DLL_NODE_GET(m_NCS_DBLIST_FIND_FIRST(&comp->csi_list));
				csi != nullptr;
				csi = m_AVND_CSI_REC_FROM_COMP_DLL_NODE_GET(m_NCS_DBLIST_FIND_NEXT(&csi->comp_dll_node))) {
				/* In shutdown phase SIs are removed honoring saAmfSIRank in reverse
				   order. A component which is having a CSI from a higher rank assigned 
				   SI can fault while removal of lower rank SIs is going on. In such 
				   case CSIs of this component will be in assigned state only, so remove 
				   done indication cannot be generated for assigned CSIs. 
				 */
				if (m_AVND_COMP_CSI_CURR_ASSIGN_STATE_IS_REMOVING(csi))
					/* Csi remove done for PI comp only and not for NPI comp in PI SU.
					   When avnd_comp_clc_st_chng_prc will be called and it will be a
					   case of "SU Preinst and Comp Non-Preinst" and
					   TERMINATING == prv_st and UNINSTANTIATED == final_st, then
					   avnd_comp_csi_remove_done will be called. */
					if (m_AVND_COMP_TYPE_IS_PREINSTANTIABLE(comp))
						rc = avnd_comp_csi_remove_done(cb, comp, csi); 

				/* Removal of last CSI from this component may lead to SUSI assign/remove 
				   done indication, which eventually deletes all COMP-CSI record.
				   In such a case there will not be any CSI in comp->csi_list, so come 
				   out of the loop.
				 */
				if (comp->csi_list.n_nodes == 0)
					break;
			}
		}

		if ((!comp->su->is_ncs) && (comp->csi_list.n_nodes > 0) &&
				(!m_AVND_SU_IS_PREINSTANTIABLE(comp->su))) {
			AVND_COMP_CSI_REC *csi = nullptr;
			/*
			   Explantion written above for PI SU case is valid here also.
			   However for a NPI comp in NPI SU, mark it REMOVED instead of 
			   generating remove done indication. 
			 */
			csi = m_AVND_CSI_REC_FROM_COMP_DLL_NODE_GET(m_NCS_DBLIST_FIND_FIRST(&comp->csi_list));
			if (csi != nullptr)
				m_AVND_COMP_CSI_CURR_ASSIGN_STATE_SET(csi, AVND_COMP_CSI_ASSIGN_STATE_REMOVED);
		}
		if (all_comps_terminated()) {
			LOG_NO("Terminated all AMF components");
			LOG_NO("Shutdown completed, exiting");
			exit(0);
		}
	}
	/*
	   Cleanup of failed component is over. If there is some pernding component-failover
	   report for AMFD, send it.
	 */
	if (m_AVND_COMP_IS_FAILED(comp) && m_AVND_SU_IS_FAILED(su) &&
			m_AVND_SU_IS_PREINSTANTIABLE(su) && (su->sufailover == false) &&
			(avnd_cb->oper_state != SA_AMF_OPERATIONAL_DISABLED) && 
			(su->su_err_esc_level == AVND_ERR_ESC_LEVEL_2)) {
		/* yes, request director to orchestrate component failover */
		rc = avnd_di_oper_send(cb, su, SA_AMF_COMPONENT_FAILOVER);
	}

	/*
	 *  su-sis may be in assigning/removing state. Except su-failover case, 
	 *  signal csi assign/remove done so that su-si assignment/removal algo
	 *  can proceed.
	 */
	if (sufailover_in_progress(su) == false) 
		avnd_comp_cmplete_all_assignment(cb, comp);
	avnd_comp_curr_info_del(cb, comp);

	/* reset the comp-reg & instantiate params */
	if (!m_AVND_COMP_TYPE_IS_PROXIED(comp)) {
		m_AVND_COMP_REG_PARAM_RESET(cb, comp);
	}

	TRACE_LEAVE();
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
uint32_t avnd_comp_clc_terming_cleanfail_hdler(AVND_CB *cb, AVND_COMP *comp)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER2("'%s': Cleanup fail event in the terminating state", comp->name.value);

	/* just transition to 'term-failed' state */
	avnd_comp_pres_state_set(cb, comp, SA_AMF_PRESENCE_TERMINATION_FAILED);
	avnd_comp_curr_info_del(cb, comp);
	if ((cb->term_state == AVND_TERM_STATE_OPENSAF_SHUTDOWN_STARTED) &&
			all_comps_terminated()) {
		LOG_WA("Terminated all AMF components (with failures)");
		LOG_NO("Shutdown completed, exiting");
		exit(0);
	}

	TRACE_LEAVE();
	return rc;
}

/**
 * @brief  handler for instantiating a component in restarting state.
 *	   It will be used during restart admin operation on su.  
 * @param  ptr to avnd_cb. 
 * @param  ptr to comp. 
 * @return NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 */

uint32_t avnd_comp_clc_restart_inst_hdler (AVND_CB *cb, AVND_COMP *comp)
{
	uint32_t rc = NCSCC_RC_SUCCESS;

	TRACE_ENTER2("'%s' : Instantiate event in the restart state", comp->name.value);

	/* Refresh the component configuration, it may have changed */
	if (!m_AVND_IS_SHUTTING_DOWN(cb) && (avnd_comp_config_reinit(comp) != 0)) {
		rc = NCSCC_RC_FAILURE;
		goto done;
	}
	/*if proxied component check whether the proxy exists, if so continue 
	   instantiating by calling the proxied callback. else start timer and 
	   wait for inst timeout duration */
	if (m_AVND_COMP_TYPE_IS_PROXIED(comp)) {
		if (comp->pxy_comp != nullptr) {
			if (m_AVND_COMP_TYPE_IS_PREINSTANTIABLE(comp))
				/* call the proxied instantiate callback   */
				rc = avnd_comp_cbk_send(cb, comp, AVSV_AMF_PXIED_COMP_INST, 0, 0);
			else
				/* do a csi set with active ha state */
				rc = avnd_comp_cbk_send(cb, comp, AVSV_AMF_CSI_SET, 0, 0);
			if (NCSCC_RC_SUCCESS == rc) {
				/* increment the retry count */
				comp->clc_info.inst_retry_cnt++;
			}
		} else {
			/* start a timer for proxied instantiating timeout duration */
			m_AVND_TMR_PXIED_COMP_INST_START(cb, *comp, rc);
		}
		goto done;
	}

	rc = avnd_comp_clc_cmd_execute(cb, comp, AVND_COMP_CLC_CMD_TYPE_INSTANTIATE);
	if (NCSCC_RC_SUCCESS == rc) {
		/* timestamp the start of this instantiation phase */
		m_GET_TIME_STAMP(comp->clc_info.inst_cmd_ts);
		/* increment the retry count */
		comp->clc_info.inst_retry_cnt++;
	}
done:
	TRACE_LEAVE2("%u", rc);
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
uint32_t avnd_comp_clc_restart_instsucc_hdler(AVND_CB *cb, AVND_COMP *comp)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER2("'%s': Instantiate success event in the restarting state", comp->name.value);

	/* stop the reg tmr */
	if (m_AVND_TMR_IS_ACTIVE(comp->clc_info.clc_reg_tmr)) {
		m_AVND_TMR_COMP_REG_STOP(cb, *comp);
	}

	/* just transition back to 'instantiated' state */
	avnd_comp_pres_state_set(cb, comp, SA_AMF_PRESENCE_INSTANTIATED);

	TRACE_LEAVE();
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
uint32_t avnd_comp_clc_restart_term_hdler(AVND_CB *cb, AVND_COMP *comp)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER2("'%s': Terminate event in the restarting state", comp->name.value);

	/* cleanup the comp */
	if (m_AVND_COMP_TYPE_IS_PROXIED(comp) && comp->pxy_comp != 0)
		rc = avnd_comp_cbk_send(cb, comp, AVSV_AMF_PXIED_COMP_CLEAN, 0, 0);
	else {
		rc = avnd_comp_clc_cmd_execute(cb, comp, AVND_COMP_CLC_CMD_TYPE_CLEANUP);

		/* delete hc-list, cbk-list, pg-list & pm-list */
		avnd_comp_hc_rec_del_all(cb, comp);
		avnd_comp_cbq_del(cb, comp, true);

		/* re-using the funtion to stop all PM started by this comp */
		avnd_comp_pm_finalize(cb, comp, comp->reg_hdl);
		avnd_comp_pm_rec_del_all(cb, comp);	/*if at all anythnig is left behind */
	}

	if (NCSCC_RC_SUCCESS == rc) {
		/* reset the comp-reg & instantiate params */
		m_AVND_COMP_REG_PARAM_RESET(cb, comp);
		m_AVND_COMP_CLC_INST_PARAM_RESET(comp);

		/* transition to 'terminating' state */
		avnd_comp_pres_state_set(cb, comp, SA_AMF_PRESENCE_TERMINATING);
	}

	TRACE_LEAVE();
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
uint32_t avnd_comp_clc_restart_termsucc_hdler(AVND_CB *cb, AVND_COMP *comp)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER2("'%s': Terminate success event in the restarting state", comp->name.value);

	/* Refresh the component configuration, it may have changed */
	if (!m_AVND_IS_SHUTTING_DOWN(cb) && (avnd_comp_config_reinit(comp) != 0)) {
		rc = NCSCC_RC_FAILURE;
		goto done;
	}

	if (!m_AVND_COMP_TYPE_IS_PROXIED(comp)) {
		m_AVND_COMP_REG_PARAM_RESET(cb, comp);
	}

	/*A NPI comp in PI SU can get terminated in other than admin restartop on it viz.
	  lock on SI, SG, NODE etc. In these cases this component cannot be instantiated.
	  In the remaining case of admin restart on it, it will be instantiated. This handler
	  is not invoked when such a componet fails with comp restart recovery, in that case 
	  cleanup success/fail handler will be invoked.
	 */
	if (!(m_AVND_COMP_TYPE_IS_PREINSTANTIABLE(comp)) && m_AVND_SU_IS_PREINSTANTIABLE(comp->su) &&
		(comp->admin_oper != true))
		goto done;

	//During restart admin operation on su, all components will be instantiated from SU FSM. 	
	if (m_AVND_SU_IS_RESTART(comp->su))  {
		goto done;
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
	}
done:
	TRACE_LEAVE();
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
uint32_t avnd_comp_clc_restart_termfail_hdler(AVND_CB *cb, AVND_COMP *comp)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER2("Terminate fail event in the restarting state:'%s'",comp->name.value);

	/* cleanup the comp */
	if (m_AVND_COMP_TYPE_IS_PROXIED(comp) && comp->pxy_comp != 0)
		rc = avnd_comp_cbk_send(cb, comp, AVSV_AMF_PXIED_COMP_CLEAN, 0, 0);
	else
		rc = avnd_comp_clc_cmd_execute(cb, comp, AVND_COMP_CLC_CMD_TYPE_CLEANUP);

	/* transition to 'term-failed' state */
	if (NCSCC_RC_SUCCESS == rc) {
		avnd_comp_pres_state_set(cb, comp, SA_AMF_PRESENCE_TERMINATION_FAILED);
	}

	TRACE_LEAVE();
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
uint32_t avnd_comp_clc_restart_clean_hdler(AVND_CB *cb, AVND_COMP *comp)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER2("'%s': Cleanup event in the restarting state", comp->name.value);

	if (m_AVND_COMP_TYPE_IS_PROXIED(comp))
		avnd_comp_cbq_del(cb, comp, true);

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
			avnd_comp_pres_state_set(cb, comp, SA_AMF_PRESENCE_TERMINATING);
		else
			m_AVND_COMP_TERM_FAIL_RESET(comp);
	}

	TRACE_LEAVE();
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
uint32_t avnd_comp_clc_restart_cleanfail_hdler(AVND_CB *cb, AVND_COMP *comp)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER2("'%s': Cleanup fail event in the restarting state", comp->name.value);

	/* transition to 'term-failed' state */
	avnd_comp_pres_state_set(cb, comp, SA_AMF_PRESENCE_TERMINATION_FAILED);

	TRACE_LEAVE();
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
uint32_t avnd_comp_clc_orph_instsucc_hdler(AVND_CB *cb, AVND_COMP *comp)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER2("'%s': Instantiate success event in the Orphaned state", comp->name.value);

	/* stop the proxied registration timer */
	if (m_AVND_TMR_IS_ACTIVE(comp->orph_tmr)) {
		m_AVND_TMR_PXIED_COMP_REG_STOP(cb, *comp);
	}

	/* just transition to 'instantiated' state */
	avnd_comp_pres_state_set(cb, comp, SA_AMF_PRESENCE_INSTANTIATED);

	TRACE_LEAVE();
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
uint32_t avnd_comp_clc_orph_term_hdler(AVND_CB *cb, AVND_COMP *comp)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER2("'%s': Terminate event in the Orphaned state", comp->name.value);

	/* queue up this event, we will process it in inst state */
	comp->pend_evt = AVND_COMP_CLC_PRES_FSM_EV_TERM;

	TRACE_LEAVE();
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
uint32_t avnd_comp_clc_orph_clean_hdler(AVND_CB *cb, AVND_COMP *comp)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER2("'%s': Cleanup event in the Orphaned state", comp->name.value);

	/* stop orphan timer if still running */
	if (m_AVND_TMR_IS_ACTIVE(comp->orph_tmr)) {
		m_AVND_TMR_PXIED_COMP_REG_STOP(cb, *comp);
	}

	avnd_comp_cbq_del(cb, comp, true);

	/* cleanup the comp */
	rc = avnd_comp_clc_cmd_execute(cb, comp, AVND_COMP_CLC_CMD_TYPE_CLEANUP);
	if (NCSCC_RC_SUCCESS == rc) {
		/* reset the comp-reg & instantiate params */
		m_AVND_COMP_REG_PARAM_RESET(cb, comp);
		m_AVND_COMP_CLC_INST_PARAM_RESET(comp);

		/* transition to 'terminating' state */
		avnd_comp_pres_state_set(cb, comp, SA_AMF_PRESENCE_TERMINATING);
	}

	TRACE_LEAVE();
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
uint32_t avnd_comp_clc_orph_restart_hdler(AVND_CB *cb, AVND_COMP *comp)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER2("'%s': Restart event in the Orphaned state", comp->name.value);

	/* In orphaned state we can't restart the comp
	   just queue the event , we will handle it 
	   on reaching inst state */
	comp->pend_evt = AVND_COMP_CLC_PRES_FSM_EV_RESTART;

	TRACE_LEAVE();
	return rc;
}

/**
 * Determine if name is in the environment variable set
 * 
 * @param name
 * @param env_set
 * @param env_counter
 * 
 * @return bool
 */
static bool var_in_envset(const char *name, const NCS_OS_ENVIRON_SET_NODE *env_set, unsigned int env_counter)
{
	unsigned int i;
	const char *var;

	for (i = 0, var = env_set[i].name; i < env_counter; i++, var = env_set[i].name) {
		if (strcmp(var, name) == 0)
			return true;
	}

	return false;
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
uint32_t avnd_comp_clc_cmd_execute(AVND_CB *cb, AVND_COMP *comp, AVND_COMP_CLC_CMD_TYPE cmd_type)
{
	NCS_OS_PROC_EXECUTE_TIMED_INFO cmd_info;
	NCS_OS_ENVIRON_ARGS arg;
	NCS_OS_ENVIRON_SET_NODE *env_set;
	char env_val_nodeid[11];
	char env_val_comp_err[11];	/*we req only 10 */
	char env_var_name[] = "SA_AMF_COMPONENT_NAME";
	char env_var_nodeid[] = "NCS_ENV_NODE_ID";
	char env_var_comp_err[] = "OSAF_COMPONENT_ERROR_SOURCE";
	AVND_CLC_EVT *clc_evt;
	AVND_EVT *evt = 0;
	AVND_COMP_CLC_INFO *clc_info = &comp->clc_info;
	char scr[SAAMF_CLC_LEN];
	char *argv[AVND_COMP_CLC_PARAM_MAX + 2];
	char tmp_argv[AVND_COMP_CLC_PARAM_MAX + 2][AVND_COMP_CLC_PARAM_SIZE_MAX];
	uint32_t argc = 0, rc = NCSCC_RC_SUCCESS,count=0;
	unsigned int env_counter;
	unsigned int i;
	SaStringT env;
	size_t env_set_nmemb;

	TRACE_ENTER2("'%s':CLC CLI command type:'%s'",comp->name.value,clc_cmd_type[cmd_type]);

	if (comp->clc_info.exec_cmd != cmd_type) {
		comp->clc_info.exec_cmd = cmd_type;
	} else {
		TRACE("Another clc cmd of the same type: '%s' is in progress", clc_cmd_type[cmd_type]);
		goto done;
	}

	/* the allocated memory is normally freed in comp_clc_resp_callback */
	clc_evt = new AVND_CLC_EVT;
	memcpy(&clc_evt->comp_name, &comp->name, sizeof(SaNameT));
	clc_evt->cmd_type = cmd_type;

	/* For external component, there is no cleanup command. So, we will send a
	   SUCCESS message to the mail box for external components. There wouldn't
	   be any other command for external component comming. */
	if (true == comp->su->su_is_external) {
		if (AVND_COMP_CLC_CMD_TYPE_CLEANUP == cmd_type) {
			clc_evt->exec_stat.value = NCS_OS_PROC_EXIT_NORMAL;

			/* create the event */
			evt = avnd_evt_create(cb, AVND_EVT_CLC_RESP, 0, 0, 0, clc_evt, 0);
			if (!evt) {
				delete clc_evt;
				rc = NCSCC_RC_FAILURE;
				goto err;
			}

			/* send the event */
			rc = avnd_evt_send(cb, evt);
			if (NCSCC_RC_SUCCESS != rc) {
				delete clc_evt;
				goto err;
			}

			delete clc_evt;
			goto done;
		} else {
			LOG_ER("Command other than cleanup recvd for ext comp: Comp and cmd_type are '%s': %u",\
					    comp->name.value,cmd_type);
		}
	}

	/* Allocate environment variable set */
	env_set_nmemb = comp->numOfCompCmdEnv + 3;
	env_set = static_cast<NCS_OS_ENVIRON_SET_NODE*>(calloc(env_set_nmemb, sizeof(NCS_OS_ENVIRON_SET_NODE)));
	memset(&cmd_info, 0, sizeof(NCS_OS_PROC_EXECUTE_TIMED_INFO));
	memset(&arg, 0, sizeof(NCS_OS_ENVIRON_ARGS));

	/*** populate the env variable set ***/
	env_counter = 0;

	if (comp->saAmfCompCmdEnv != nullptr) {
		while ((env = comp->saAmfCompCmdEnv[env_counter]) != nullptr) {
			char* equalPos = strchr(env, '=');
			if (equalPos == nullptr) {
				LOG_ER("Unknown enviroment variable format '%s'. Should be 'var=value'", env);
				env_counter++;
				continue;
			}
			env_set[env_counter].name = strndup(env, equalPos - env);
			env_set[env_counter].value = strdup(equalPos + 1);
			env_set[env_counter].overwrite = 1; 
			arg.num_args++;
			env_counter++;
		}
	}

	/* comp name env */
	env_set[env_counter].overwrite = 1;
	env_set[env_counter].name = strdup(env_var_name);
	env_set[env_counter].value = strndup ((char*)comp->name.value, comp->name.length);
	arg.num_args++;
	env_counter++;

	/* node id env */
	env_set[env_counter].overwrite = 1;
	env_set[env_counter].name = strdup(env_var_nodeid);
	sprintf(env_val_nodeid, "%u", (uint32_t)(cb->node_info.nodeId));
	env_set[env_counter].value = strdup(env_val_nodeid);
	arg.num_args++;
	env_counter++;

	/* Note:- we will set OSAF_COMPONENT_ERROR_SOURCE only for 
	 * cleanup script
	 */

	/* populate the env arg */
	if (cmd_type == AVND_COMP_CLC_CMD_TYPE_CLEANUP) {
		/* error code, will be set only if we are cleaning up */
		memset(env_val_comp_err, '\0', sizeof(env_val_comp_err));
		env_set[env_counter].overwrite = 1;
		env_set[env_counter].name = strdup(env_var_comp_err);
		sprintf((char *)env_val_comp_err, "%u", (uint32_t)(comp->err_info.src));
		env_set[env_counter].value = strdup(env_val_comp_err);
		arg.num_args++;
		env_counter++;
	}

	/* 
	** 4.3 in B.04 states:
	** "In case of a non-proxied, non-SA-aware component, the Availability Management Framework passes
	** the name/value pairs of the component service instance as environment variables to each CLC-CLI
	** command."
	*/
	if (!m_AVND_COMP_TYPE_IS_PREINSTANTIABLE(comp) && !m_AVND_COMP_TYPE_IS_PROXIED(comp)) {
		AVND_COMP_CSI_REC *csi;
		AVSV_ATTR_NAME_VAL *csiattr;
		unsigned int i;

		TRACE_1("Component is NPI, %u", comp->csi_list.n_nodes);

		if (comp->csi_list.n_nodes == 1) {
			csi = m_AVND_CSI_REC_FROM_COMP_DLL_NODE_GET(m_NCS_DBLIST_FIND_FIRST(&comp->csi_list));

			osafassert(csi);

			/* allocate additional env_set memory for the CSI attributes */
			env_set = static_cast<NCS_OS_ENVIRON_SET_NODE*>(realloc(env_set, sizeof(NCS_OS_ENVIRON_SET_NODE) * (env_set_nmemb + 
						csi->attrs.number)));
			osafassert(env_set);

			/* initialize newly allocated memory */
			memset(&env_set[env_set_nmemb], 0, sizeof(NCS_OS_ENVIRON_SET_NODE) * csi->attrs.number);

			for (i = 0, csiattr = csi->attrs.list; i < csi->attrs.number; i++, csiattr++) {
				if (var_in_envset((char*)csiattr->name.value, env_set, env_counter)) {
					LOG_NO("Ignoring second (or more) value '%s' for '%s' CSI attr '%s'",
							csiattr->string_ptr, comp->name.value, 
							csiattr->name.value);
					continue;
				}

				TRACE("%s=%s", csiattr->name.value, csiattr->string_ptr);
				env_set[env_counter].overwrite = 1;
				env_set[env_counter].name = strdup((char*)csiattr->name.value);
				osafassert(env_set[env_counter].name != nullptr);
				if (nullptr != csiattr->string_ptr) {
					env_set[env_counter].value = strdup(csiattr->string_ptr);
					osafassert(env_set[env_counter].value != nullptr);
				} else {
					env_set[env_counter].value = new char(); 
				}
				arg.num_args++;
				env_counter++;
			}
		} /* if (comp->csi_list.n_nodes == 1) */
	}

	arg.env_arg = env_set;

	/* tokenize the cmd */
	m_AVND_COMP_CLC_STR_PARSE(clc_info->cmds[cmd_type - 1].cmd, scr, argc, argv, tmp_argv);

	/* populate the cmd-info */
	cmd_info.i_script = argv[0];
	cmd_info.i_argv = argv;
	cmd_info.i_timeout_in_ms = (uint32_t)((clc_info->cmds[cmd_type - 1].timeout) / 1000000);
	cmd_info.i_cb = comp_clc_resp_callback;
	cmd_info.i_set_env_args = &arg;
	cmd_info.i_usr_hdl = (NCS_EXEC_USR_HDL) clc_evt;

	TRACE_1("CLC CLI script:'%s'",cmd_info.i_script);
	for(count=1;count<argc;count++)
		TRACE_1("CLC CLI command arguments[%d] ='%s'",count, cmd_info.i_argv[count]);

	TRACE_1("CLC CLI command timeout: In nano secs:%llu In milli secs: %u",
						clc_info->cmds[cmd_type - 1].timeout, cmd_info.i_timeout_in_ms);

	for(count=0;count<cmd_info.i_set_env_args->num_args;count++)
		TRACE_1("CLC CLI command env variable name = '%s': value ='%s'",
				cmd_info.i_set_env_args->env_arg->name,cmd_info.i_set_env_args->env_arg->value);

	/* finally execute the command */
	rc = ncs_os_process_execute_timed(&cmd_info);

	/* Remove the env_set structure */
	for (i = 0; i < env_counter; i++) {
		free(env_set[i].name);
		free(env_set[i].value);
	}
	free(env_set);

	if (NCSCC_RC_SUCCESS != rc) {
		TRACE_2("The CLC CLI command execution failed");
		/* generate a cmd failure event; it'll be executed asynchronously */

		/* create the event */
		evt = avnd_evt_create(cb, AVND_EVT_CLC_RESP, 0, 0, 0, clc_evt, 0);
		if (!evt) {
			delete clc_evt;
			rc = NCSCC_RC_FAILURE;
			goto err;
		}

		/* send the event */
		rc = avnd_evt_send(cb, evt);
		if (NCSCC_RC_SUCCESS != rc) {
			delete clc_evt;
			goto err;
		}
		delete clc_evt;
	} else {
		TRACE_2("The CLC CLI command execution success");
		// outcome of command is reported in comp_clc_resp_callback()
	}

	TRACE_2("success");
	goto done;

 err:
	/* free the event */
	if (evt)
		avnd_evt_destroy(evt);

 done:
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
uint32_t avnd_instfail_su_failover(AVND_CB *cb, AVND_SU *su, AVND_COMP *failed_comp)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER2("Executing Component Failover: Instantiation failed SU: '%s' : Failed component: '%s'",
								su->name.value, failed_comp->name.value);

	/* mark the comp failed */
	m_AVND_COMP_FAILED_SET(failed_comp);

	/* update comp oper state */
	m_AVND_COMP_OPER_STATE_SET(failed_comp, SA_AMF_OPERATIONAL_DISABLED);
	m_AVND_COMP_OPER_STATE_AVD_SYNC(cb, failed_comp, rc);
	if (NCSCC_RC_SUCCESS != rc)
		goto done;

	/* if we are in the middle of su restart, reset the flag and go ahead */
	if (m_AVND_SU_IS_RESTART(su)) {
		m_AVND_SU_RESTART_RESET(su);
	}

	/* mark the su failed */
	if (!m_AVND_SU_IS_FAILED(su)) {
		m_AVND_SU_FAILED_SET(su);
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

		/* inform AvD */
		rc = avnd_di_oper_send(cb, su, SA_AMF_COMPONENT_FAILOVER);

		if (cb->is_avd_down == true) {
			// remove assignment if instantiation fails and leads
			// to comp failover in headless mode for PI SU
			if (m_AVND_SU_IS_PREINSTANTIABLE(su)) {
				LOG_WA("Director is down. Remove all SIs from '%s'", su->name.value);
				avnd_su_si_del(avnd_cb, &su->name);
			}
		}
	}

 done:
	if (rc == NCSCC_RC_SUCCESS) {
		LOG_NO("Component Failover trigerred for '%s': Failed component: '%s'",
			su->name.value, failed_comp->name.value);
	}
	TRACE_LEAVE2("%u", rc);
	return rc;
}


/**
 * @brief	This function processes component restart event in RESTARTING state.
 *
 * @param 	ptr to avnd_cb 
 * @param	ptr to component
 * 
 * @return	NCSCC_RC_FAILURE/NCSCC_RC_SUCCESS
 */
static uint32_t avnd_comp_clc_restart_restart_hdler(AVND_CB *cb, AVND_COMP *comp)
{
	uint32_t rc;
	TRACE_ENTER2("'%s': Restart event in the restarting state", comp->name.value);

	if (m_AVND_COMP_TYPE_IS_PROXIED(comp))
		avnd_comp_cbq_del(cb, comp, true);

	if (m_AVND_COMP_TYPE_IS_PROXIED(comp) && comp->pxy_comp != 0)
		rc = avnd_comp_cbk_send(cb, comp, AVSV_AMF_PXIED_COMP_CLEAN, 0, 0);
	else
		rc = avnd_comp_clc_cmd_execute(cb, comp, AVND_COMP_CLC_CMD_TYPE_CLEANUP);

	if (rc == NCSCC_RC_SUCCESS) {
		if (!m_AVND_COMP_TYPE_IS_PROXIED(comp))
			m_AVND_COMP_REG_PARAM_RESET(cb, comp);
		m_AVND_COMP_CLC_INST_PARAM_RESET(comp);
	}

	TRACE_LEAVE();
	return rc;
}

