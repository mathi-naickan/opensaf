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

  This file contains the routines to manage the SU-SI assignments (& removals) 
  and SU presence state FSM.
 
..............................................................................

  FUNCTIONS INCLUDED in this module:

  
******************************************************************************
*/

#include <stdbool.h>
#include "avnd.h"

/* static function declarations */
static uns32 avnd_su_pres_uninst_suinst_hdler(AVND_CB *, AVND_SU *, AVND_COMP *);
static uns32 avnd_su_pres_insting_suterm_hdler(AVND_CB *, AVND_SU *, AVND_COMP *);
static uns32 avnd_su_pres_insting_surestart_hdler(AVND_CB *, AVND_SU *, AVND_COMP *);
static uns32 avnd_su_pres_insting_compinst_hdler(AVND_CB *, AVND_SU *, AVND_COMP *);
static uns32 avnd_su_pres_insting_compinstfail_hdler(AVND_CB *, AVND_SU *, AVND_COMP *);
static uns32 avnd_su_pres_inst_suterm_hdler(AVND_CB *, AVND_SU *, AVND_COMP *);
static uns32 avnd_su_pres_inst_surestart_hdler(AVND_CB *, AVND_SU *, AVND_COMP *);
static uns32 avnd_su_pres_inst_comprestart_hdler(AVND_CB *, AVND_SU *, AVND_COMP *);
static uns32 avnd_su_pres_inst_compterming_hdler(AVND_CB *, AVND_SU *, AVND_COMP *);
static uns32 avnd_su_pres_terming_compinst_hdler(AVND_CB *, AVND_SU *, AVND_COMP *);
static uns32 avnd_su_pres_terming_comptermfail_hdler(AVND_CB *, AVND_SU *, AVND_COMP *);
static uns32 avnd_su_pres_terming_compuninst_hdler(AVND_CB *, AVND_SU *, AVND_COMP *);
static uns32 avnd_su_pres_restart_suterm_hdler(AVND_CB *, AVND_SU *, AVND_COMP *);
static uns32 avnd_su_pres_restart_compinst_hdler(AVND_CB *, AVND_SU *, AVND_COMP *);
static uns32 avnd_su_pres_restart_compterming_hdler(AVND_CB *, AVND_SU *, AVND_COMP *);
static uns32 avnd_su_pres_inst_compinstfail_hdler(AVND_CB *, AVND_SU *, AVND_COMP *);
static uns32 avnd_su_pres_instfailed_compuninst(AVND_CB *, AVND_SU *, AVND_COMP *);

static uns32 avnd_su_pres_st_chng_prc(AVND_CB *, AVND_SU *, SaAmfPresenceStateT, SaAmfPresenceStateT);

/****************************************************************************
 * S E R V I C E  U N I T  P R E S  F S M  M A T R I X  D E F I N I T I O N *
 ****************************************************************************/

/* evt handlers are named in this format: avnd_su_pres_<st>_<ev>_hdler() */
static AVND_SU_PRES_FSM_FN avnd_su_pres_fsm[][AVND_SU_PRES_FSM_EV_MAX - 1] = {
	/* SA_AMF_PRESENCE_UNINSTANTIATED */
	{
	 avnd_su_pres_uninst_suinst_hdler,	/* SU INST */
	 0,			/* SU TERM */
	 0,			/* SU RESTART */
	 0,			/* COMP INSTANTIATED */
	 0,			/* COMP INST_FAIL */
	 0,			/* COMP RESTARTING */
	 0,			/* COMP TERM_FAIL */
	 0,			/* COMP UNINSTANTIATED */
	 0,			/* COMP TERMINATING */
	 },

	/* SA_AMF_PRESENCE_INSTANTIATING */
	{
	 0,			/* SU INST */
	 avnd_su_pres_insting_suterm_hdler,	/* SU TERM */
	 avnd_su_pres_insting_surestart_hdler,	/* SU RESTART */
	 avnd_su_pres_insting_compinst_hdler,	/* COMP INSTANTIATED */
	 avnd_su_pres_insting_compinstfail_hdler,	/* COMP INST_FAIL */
	 0,			/* COMP RESTARTING */
	 avnd_su_pres_terming_comptermfail_hdler,	/* COMP TERM_FAIL */
	 0,			/* COMP UNINSTANTIATED */
	 0,			/* COMP TERMINATING */
	 },

	/* SA_AMF_PRESENCE_INSTANTIATED */
	{
	 0,			/* SU INST */
	 avnd_su_pres_inst_suterm_hdler,	/* SU TERM */
	 avnd_su_pres_inst_surestart_hdler,	/* SU RESTART */
	 0,			/* COMP INSTANTIATED */
	 avnd_su_pres_inst_compinstfail_hdler,	/* COMP INST_FAIL */
	 avnd_su_pres_inst_comprestart_hdler,	/* COMP RESTARTING */
	 avnd_su_pres_terming_comptermfail_hdler,	/* COMP TERM_FAIL */
	 0,			/* COMP UNINSTANTIATED */
	 avnd_su_pres_inst_compterming_hdler,	/* COMP TERMINATING */
	 },

	/* SA_AMF_PRESENCE_TERMINATING */
	{
	 0,			/* SU INST */
	 avnd_su_pres_restart_suterm_hdler,	/* SU TERM */
	 0,			/* SU RESTART */
	 avnd_su_pres_terming_compinst_hdler,	/* COMP INSTANTIATED */
	 avnd_su_pres_inst_compinstfail_hdler,	/* COMP INST_FAIL */
	 0,			/* COMP RESTARTING */
	 avnd_su_pres_terming_comptermfail_hdler,	/* COMP TERM_FAIL */
	 avnd_su_pres_terming_compuninst_hdler,	/* COMP UNINSTANTIATED */
	 0,			/* COMP TERMINATING */
	 },

	/* SA_AMF_PRESENCE_RESTARTING */
	{
	 0,			/* SU INST */
	 avnd_su_pres_restart_suterm_hdler,	/* SU TERM */
	 0,			/* SU RESTART */
	 avnd_su_pres_restart_compinst_hdler,	/* COMP INSTANTIATED */
	 avnd_su_pres_inst_compinstfail_hdler,	/* COMP INST_FAIL */
	 0,			/* COMP RESTARTING */
	 avnd_su_pres_terming_comptermfail_hdler,	/* COMP TERM_FAIL */
	 0,			/* COMP UNINSTANTIATED */
	 avnd_su_pres_restart_compterming_hdler,	/* COMP TERMINATING */
	 },

	/* SA_AMF_PRESENCE_INSTANTIATION_FAILED */
	{
	 0,			/* SU INST */
	 0,			/* SU TERM */
	 0,			/* SU RESTART */
	 0,			/* COMP INSTANTIATED */
	 avnd_su_pres_instfailed_compuninst,	/* COMP INST_FAIL */
	 0,			/* COMP RESTARTING */
	 0,			/* COMP TERM_FAIL */
	 avnd_su_pres_instfailed_compuninst,	/* COMP UNINSTANTIATED */
	 0,			/* COMP TERMINATING */
	 },

	/* SA_AMF_PRESENCE_TERMINATION_FAILED */
	{
	 0,			/* SU INST */
	 0,			/* SU TERM */
	 0,			/* SU RESTART */
	 0,			/* COMP INSTANTIATED */
	 0,			/* COMP INST_FAIL */
	 0,			/* COMP RESTARTING */
	 0,			/* COMP TERM_FAIL */
	 0,			/* COMP UNINSTANTIATED */
	 0,			/* COMP TERMINATING */
	 }
};

static unsigned int update_avd_presence_state(const AVND_SU *su)
{
	AVSV_PARAM_INFO param;

	memset(&param, 0, sizeof(AVSV_PARAM_INFO));
	param.class_id = AVSV_SA_AMF_SU;
	param.attr_id = saAmfSUPresenceState_ID;
	param.name = su->name;
	param.act = AVSV_OBJ_OPR_MOD;
	*((uns32 *)param.value) = m_NCS_OS_HTONL(su->pres);
	param.value_len = sizeof(uns32);

	return avnd_di_object_upd_send(avnd_cb, &param);
}

/***************************************************************************
 * S U - S I  Q U E U E  M A N A G E M E N T   P O R T I O N   S T A R T S *
 ***************************************************************************/

/****************************************************************************
  Name          : avnd_su_siq_rec_buf
 
  Description   : This routine determines if the SU-SI assign message from 
                  AvD is to be buffered. If the message has to be buffered, 
                  it is added to the SIQ (maintained on SU). Else the message
                  is ready for assignment.
 
  Arguments     : cb    - ptr to AvND control block
                  su    - ptr to the AvND SU
                  param - ptr to the SI parameters
 
  Return Values : ptr to the siq record, if the message is buffered
                  0, otherwise
 
  Notes         : None
******************************************************************************/
AVND_SU_SIQ_REC *avnd_su_siq_rec_buf(AVND_CB *cb, AVND_SU *su, AVND_SU_SI_PARAM *param)
{
	AVND_SU_SIQ_REC *siq = 0;
	AVND_SU_SI_REC *si_rec = 0;
	NCS_BOOL is_tob_buf = FALSE;
	uns32 rc = NCSCC_RC_SUCCESS;

	/* buffer the msg, if SU is inst-failed and all comps are not terminated */
	if (((su->pres == SA_AMF_PRESENCE_INSTANTIATION_FAILED) && (!m_AVND_SU_IS_ALL_TERM(su))) ||
			(cb->term_state == AVND_TERM_STATE_OPENSAF_SHUTDOWN) || (m_AVND_SU_IS_RESTART(su))) {
		siq = avnd_su_siq_rec_add(cb, su, param, &rc);
		return siq;
	}

	/* dont buffer quiesced assignments during failure */
	if (m_AVND_SU_IS_FAILED(su) &&
	    ((SA_AMF_HA_QUIESCED == param->ha_state) || (AVSV_SUSI_ACT_DEL == param->msg_act)))
		return 0;

	/*
	 * Determine if msg is to be buffered.
	 */
	if (!m_AVSV_SA_NAME_IS_NULL(param->si_name)) {	/* => msg is for a single si */
		/* determine if an (non-quiescing) assign / remove operation is on */
		si_rec = avnd_su_si_rec_get(cb, &su->name, &param->si_name);
		if (si_rec && ((m_AVND_SU_SI_CURR_ASSIGN_STATE_IS_ASSIGNING(si_rec) &&
				(SA_AMF_HA_QUIESCING != si_rec->curr_state)) ||
			       (m_AVND_SU_SI_CURR_ASSIGN_STATE_IS_REMOVING(si_rec))))
			is_tob_buf = TRUE;
	} else {		/* => msg is for all sis */
		/* determine if an (non-quiescing) assign / remove operation is on */
		for (si_rec = (AVND_SU_SI_REC *)m_NCS_DBLIST_FIND_FIRST(&su->si_list);
		     si_rec && !((m_AVND_SU_SI_CURR_ASSIGN_STATE_IS_ASSIGNING(si_rec) &&
				  (SA_AMF_HA_QUIESCING != si_rec->curr_state)) ||
				 (m_AVND_SU_SI_CURR_ASSIGN_STATE_IS_REMOVING(si_rec)));
		     si_rec = (AVND_SU_SI_REC *)m_NCS_DBLIST_FIND_NEXT(&si_rec->su_dll_node)) ;
		if (si_rec)
			is_tob_buf = TRUE;
	}

	/*
	 * Buffer the msg (if reqd).
	 */
	if (TRUE == is_tob_buf)
		siq = avnd_su_siq_rec_add(cb, su, param, &rc);

	return siq;
}

/****************************************************************************
  Name          : avnd_su_siq_prc
 
  Description   : This routine picks up the least-recent buffered message & 
                  initiates the su-si assignment / removal.
 
  Arguments     : cb   - ptr to AvND control block
                  su   - ptr to the AvND SU
 
  Return Values : NCSCC_RC_SUCCESS / NCSCC_RC_FAILURE
 
  Notes         : None
******************************************************************************/
uns32 avnd_su_siq_prc(AVND_CB *cb, AVND_SU *su)
{
	AVND_SU_SIQ_REC *siq = 0;
	AVND_SU_SI_REC *si = 0;
	uns32 rc = NCSCC_RC_SUCCESS;

	/* get the least-recent buffered msg, if any */
	siq = (AVND_SU_SIQ_REC *)m_NCS_DBLIST_FIND_LAST(&su->siq);
	if (!siq)
		return rc;

	/* determine if an (non-quiescing) assign / remove operation is on */
	for (si = (AVND_SU_SI_REC *)m_NCS_DBLIST_FIND_FIRST(&su->si_list);
	     si && !((m_AVND_SU_SI_CURR_ASSIGN_STATE_IS_ASSIGNING(si) &&
		      (SA_AMF_HA_QUIESCING != si->curr_state)) ||
		     (m_AVND_SU_SI_CURR_ASSIGN_STATE_IS_REMOVING(si)));
	     si = (AVND_SU_SI_REC *)m_NCS_DBLIST_FIND_NEXT(&si->su_dll_node)) ;
	if (si)
		return rc;

	/* unlink the buffered msg from the queue */
	ncs_db_link_list_delink(&su->siq, &siq->su_dll_node);

	/* initiate si asignment / removal */
	rc = avnd_su_si_msg_prc(cb, su, &siq->info);

	if (TRUE == su->su_is_external) {
		m_AVND_SEND_CKPT_UPDT_ASYNC_RMV(cb, siq, AVND_CKPT_SIQ_REC);
	}

	/* delete the buffered msg */
	avnd_su_siq_rec_del(cb, su, siq);

	return rc;
}

/***************************************************************************
 *****  S U - S I   M A N A G E M E N T   P O R T I O N   S T A R T S  *****
 ***************************************************************************/

/****************************************************************************
  Name          : avnd_su_si_msg_prc
 
  Description   : This routine initiates the su-si assignment / removal. It 
                  updates the database & then starts the assignment / removal.
 
  Arguments     : cb   - ptr to AvND control block
                  su   - ptr to the AvND SU
                  info - ptr to the si param
 
  Return Values : NCSCC_RC_SUCCESS / NCSCC_RC_FAILURE
 
  Notes         : None
******************************************************************************/
uns32 avnd_su_si_msg_prc(AVND_CB *cb, AVND_SU *su, AVND_SU_SI_PARAM *info)
{
	AVND_SU_SI_REC *si = 0;
	uns32 rc = NCSCC_RC_SUCCESS;
	AVND_COMP_CSI_REC *csi = NULL;

	TRACE_ENTER2("%s, act=%u, ha_state=%u", su->name.value, info->msg_act, info->ha_state);

	/* we have started the su si msg processing */
	m_AVND_SU_ASSIGN_PEND_SET(su);
	m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, su, AVND_CKPT_SU_FLAG_CHANGE);

	switch (info->msg_act) {
	case AVSV_SUSI_ACT_ASGN:	/* new assign */
		{
			/* add to the database */
			si = avnd_su_si_rec_add(cb, su, info, &rc);
			if (NULL != si) {
				/* Send the ASYNC updates for this SI and its CSI. First of all 
				   send SU_SI record and then all the CSIs. */
				m_AVND_SEND_CKPT_UPDT_ASYNC_ADD(cb, si, AVND_CKPT_SU_SI_REC);
				for (csi = (AVND_COMP_CSI_REC *)m_NCS_DBLIST_FIND_FIRST(&si->csi_list);
				     csi; csi = (AVND_COMP_CSI_REC *)m_NCS_DBLIST_FIND_NEXT(&csi->si_dll_node)) {
					m_AVND_SEND_CKPT_UPDT_ASYNC_ADD(cb, csi, AVND_CKPT_CSI_REC);
				}
			}
			/* initiate si assignment */
			if (si)
				rc = avnd_su_si_assign(cb, su, si);
		}
		break;

	case AVSV_SUSI_ACT_DEL:	/* remove */
		{
			/* get the si */
			si = avnd_su_si_rec_get(cb, &su->name, &info->si_name);
			if (!si && !m_AVSV_SA_NAME_IS_NULL(info->si_name))
				goto done;

			/* 
			 * si may point to si-rec to be deleted or be 0 (signifying all).
			 * initiate si removal.
			 */
			rc = avnd_su_si_remove(cb, su, si);
		}
		break;

	case AVSV_SUSI_ACT_MOD:	/* modify */
		{
			/* modify in the database */
			if (!m_AVSV_SA_NAME_IS_NULL(info->si_name))
				/* modify a specific si-rec */
				si = avnd_su_si_rec_modify(cb, su, info, &rc);
			else
				/* modify all the comp-csi records that belong to this su */
				rc = avnd_su_si_all_modify(cb, su, info);

			/* initiate si modification */
			if (NCSCC_RC_SUCCESS == rc)
				rc = avnd_su_si_assign(cb, su, si);
		}
		break;

	default:
		assert(0);
	}			/* switch */

done:
	TRACE_LEAVE();
	return rc;
}

/**
 * Assign a SI to an SU
 * @param si
 * @param su
 * @param single_csi if true just one CSI is assigned. if false all CSIs
 *                   are assigned in one shot.
 * 
 * @return uns32
 */
static uns32 assign_si_to_su(AVND_SU_SI_REC *si, AVND_SU *su, int single_csi)
{
	uns32 rc = NCSCC_RC_SUCCESS;
	AVND_COMP_CSI_REC *curr_csi;

	TRACE_ENTER2("%s %s, single_csi=%u", si->name.value, su->name.value, single_csi);

	/* initiate the si assignment for pi su */
	if (m_AVND_SU_IS_PREINSTANTIABLE(su)) {
		uns32 rank;

		if (SA_AMF_HA_ACTIVE == si->curr_state) {
			for (curr_csi = (AVND_COMP_CSI_REC *)m_NCS_DBLIST_FIND_FIRST(&si->csi_list),
				 rank = curr_csi->rank;
				  (curr_csi != NULL) && (curr_csi->rank == rank);
				  curr_csi = (AVND_COMP_CSI_REC *)m_NCS_DBLIST_FIND_NEXT(&curr_csi->si_dll_node)) {

				rc = avnd_comp_csi_assign(avnd_cb, curr_csi->comp, (single_csi) ? curr_csi : NULL);
				if (NCSCC_RC_SUCCESS != rc)
					goto done;
			}
		} else {
			for (curr_csi = (AVND_COMP_CSI_REC *)m_NCS_DBLIST_FIND_LAST(&si->csi_list),
				 rank = curr_csi->rank;
				  (curr_csi != NULL) && (curr_csi->rank == rank);
				  curr_csi = (AVND_COMP_CSI_REC *)m_NCS_DBLIST_FIND_PREV(&curr_csi->si_dll_node)) {

				rc = avnd_comp_csi_assign(avnd_cb, curr_csi->comp, (single_csi) ? curr_csi : NULL);
				if (NCSCC_RC_SUCCESS != rc)
					goto done;
			}
		}
	}

	/* initiate the si assignment for npi su */
	if (!m_AVND_SU_IS_PREINSTANTIABLE(su)) {
		NCS_BOOL npi_prv_inst = TRUE, npi_curr_inst = TRUE;
		AVND_SU_PRES_FSM_EV su_ev = AVND_SU_PRES_FSM_EV_MAX;

		/* determine the instantiation state of npi su */
		if (SA_AMF_HA_ACTIVE != si->prv_state)
			npi_prv_inst = FALSE;
		if (SA_AMF_HA_ACTIVE != si->curr_state)
			npi_curr_inst = FALSE;

		/* Quiesced while Quiescing */
		if (m_AVND_SU_SI_PRV_ASSIGN_STATE_IS_ASSIGNING(si) && (SA_AMF_HA_QUIESCING == si->prv_state))
			npi_prv_inst = TRUE;

		/* determine the event for the su fsm */
		if (m_AVND_SU_IS_RESTART(su) && (TRUE == npi_curr_inst))
			su_ev = AVND_SU_PRES_FSM_EV_RESTART;
		else if (!m_AVND_SU_IS_RESTART(su) && (npi_prv_inst != npi_curr_inst))
			su_ev = (TRUE == npi_curr_inst) ? AVND_SU_PRES_FSM_EV_INST : AVND_SU_PRES_FSM_EV_TERM;

		/* we cant do anything on inst-failed SU, so just resp success for quiesced */
		if (su->pres == SA_AMF_PRESENCE_INSTANTIATION_FAILED)
			su_ev = AVND_SU_PRES_FSM_EV_MAX;

		/* trigger the su fsm */
		if (AVND_SU_PRES_FSM_EV_MAX != su_ev) {
			if (!single_csi) {
				m_AVND_SU_ALL_SI_SET(su);
				m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, su, AVND_CKPT_SU_FLAG_CHANGE);
			}
			rc = avnd_su_pres_fsm_run(avnd_cb, su, 0, su_ev);
		} else
			rc = avnd_su_si_oper_done(avnd_cb, su, ((single_csi) ? si:NULL));
		if (NCSCC_RC_SUCCESS != rc)
			goto done;
	}

done:
	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
  Name          : avnd_su_si_assign
 
  Description   : This routine triggers the su-si assignment. 
                  For PI SU, it picks up the lowest/highest ranked csi from
                  the csi-list (maintained on si). It then invokes csi-set 
                  callback for CSI belonging to PI components and triggers 
                  comp FSM with InstEv/TermEv trigger for CSI belonging to
                  NPI comps. 
                  For NPI SU, the SU FSM is triggered with SUInstEv/SUTerm 
                  event. 
                  The decision to pick the lowest/highest rank or generate 
                  instantiate/terminate trigger depends on the current and 
                  previous HA state.
                  If the SU is in restart state (i.e. su-restart recovery is 
                  in progress), comp & SU FSM are triggered with Restart 
                  events instead (in the above flow).
 
  Arguments     : cb - ptr to AvND control block
                  su - ptr to the AvND SU
                  si - ptr to the si record (if 0, all the CSIs corresponding
                       to a comp are assigned in one shot)
 
  Return Values : NCSCC_RC_SUCCESS / NCSCC_RC_FAILURE
 
  Notes         : None
******************************************************************************/
uns32 avnd_su_si_assign(AVND_CB *cb, AVND_SU *su, AVND_SU_SI_REC *si)
{
	uns32 rc = NCSCC_RC_SUCCESS;
	AVND_SU_SI_REC *curr_si;

	TRACE_ENTER2("%s %p", su->name.value, si);

	/* mark the si(s) assigning and assign to su */
	if (si) {
		m_AVND_SU_SI_CURR_ASSIGN_STATE_SET(si, AVND_SU_SI_ASSIGN_STATE_ASSIGNING);
		m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, si, AVND_CKPT_SU_SI_REC_CURR_ASSIGN_STATE);
	} else {
		/* if no si is specified, the action is aimed at all the sis... loop */
		for (curr_si = (AVND_SU_SI_REC *)m_NCS_DBLIST_FIND_FIRST(&su->si_list);
		     curr_si != NULL;
			 curr_si = (AVND_SU_SI_REC *)m_NCS_DBLIST_FIND_NEXT(&curr_si->su_dll_node)) {

			m_AVND_SU_SI_CURR_ASSIGN_STATE_SET(curr_si, AVND_SU_SI_ASSIGN_STATE_ASSIGNING);
			m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, curr_si, AVND_CKPT_SU_SI_REC_CURR_ASSIGN_STATE);
		}
	}

	/* if no si is specified, the action is aimed at all the sis... pick up any si */
	curr_si = (si) ? si : (AVND_SU_SI_REC *)m_NCS_DBLIST_FIND_FIRST(&su->si_list);
	if (!curr_si)
		goto done;

	rc = assign_si_to_su(curr_si, su, ((si) ? true:false));
done:
	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
  Name          : avnd_su_si_remove
 
  Description   : This routine triggers su-si assignment removal. For PI SU, 
                  it picks up the highest ranked CSIs from the csi-list. It 
                  then invokes csi-remove callback for CSIs belonging to PI 
                  components and triggers comp FSM with TermEv for CSIs 
                  belonging to NPI comp. For NPI SU, the SU FSM is triggered 
                  with SUTermEv.
 
  Arguments     : cb - ptr to AvND control block
                  su - ptr to the AvND SU
                  si - ptr to the si record (if 0, all the CSIs corresponding
                       to a comp are removed in one shot)
 
  Return Values : NCSCC_RC_SUCCESS / NCSCC_RC_FAILURE
 
  Notes         : None
******************************************************************************/
uns32 avnd_su_si_remove(AVND_CB *cb, AVND_SU *su, AVND_SU_SI_REC *si)
{
	AVND_COMP_CSI_REC *curr_csi = 0;
	AVND_SU_SI_REC *curr_si = 0;
	uns32 rc = NCSCC_RC_SUCCESS;

	TRACE_ENTER2("%s %p", su->name.value, si);

	/* mark the si(s) removing */
	if (si) {
		m_AVND_SU_SI_CURR_ASSIGN_STATE_SET(si, AVND_SU_SI_ASSIGN_STATE_REMOVING);
		m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, si, AVND_CKPT_SU_SI_REC_CURR_ASSIGN_STATE);
	} else {
		for (curr_si = (AVND_SU_SI_REC *)m_NCS_DBLIST_FIND_FIRST(&su->si_list);
		     curr_si; curr_si = (AVND_SU_SI_REC *)m_NCS_DBLIST_FIND_NEXT(&curr_si->su_dll_node)) {
			m_AVND_SU_SI_CURR_ASSIGN_STATE_SET(curr_si, AVND_SU_SI_ASSIGN_STATE_REMOVING);
			m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, curr_si, AVND_CKPT_SU_SI_REC_CURR_ASSIGN_STATE);
		}
	}

	/* if no si is specified, the action is aimed at all the sis... pick up any si */
	curr_si = (si) ? si : (AVND_SU_SI_REC *)m_NCS_DBLIST_FIND_FIRST(&su->si_list);
	if (!curr_si)
		goto done;

	/* initiate the si removal for pi su */
	if (m_AVND_SU_IS_PREINSTANTIABLE(su)) {
		/* pick up the last csi */
		curr_csi = (AVND_COMP_CSI_REC *)m_NCS_DBLIST_FIND_LAST(&curr_si->csi_list);

		/* remove the csi */
		if (curr_csi) {
			rc = avnd_comp_csi_remove(cb, curr_csi->comp, (si) ? curr_csi : 0);
			if (NCSCC_RC_SUCCESS != rc)
				goto done;
		}
	}

	/* initiate the si removal for npi su */
	if (!m_AVND_SU_IS_PREINSTANTIABLE(su)) {
		/* nothing to be done, termination already done in
		   quiescing/quiesced state */
		rc = avnd_su_si_oper_done(cb, su, si);
		if (NCSCC_RC_SUCCESS != rc)
			goto done;
	}

 done:
	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
  Name          : avnd_su_si_oper_done
 
  Description   : This routine is invoked to indicate that the SI state 
                  assignment / removal that was pending on a SI is completed.
                  It is generated when all the CSIs are assigned / removed OR
                  when SU FSM transitions to INSTANTIATED / UNINSTANTIATED.
                  AvD is informed that this SU-SI was successfully assigned /
                  removed.
 
  Arguments     : cb - ptr to AvND control block
                  su - ptr to the AvND SU
                  si - ptr to the si record (if 0, it indicates that the 
                       assign/remove operation was done for all the SIs 
                       belonging to the SU)
 
  Return Values : NCSCC_RC_SUCCESS / NCSCC_RC_FAILURE
 
  Notes         : None
******************************************************************************/
uns32 avnd_su_si_oper_done(AVND_CB *cb, AVND_SU *su, AVND_SU_SI_REC *si)
{
	AVND_SU_SI_REC *curr_si = 0;
	AVND_COMP_CSI_REC *curr_csi = 0;
	NCS_BOOL are_si_assigned;
	uns32 rc = NCSCC_RC_SUCCESS;

	TRACE_ENTER2("%s %p", su->name.value, si);

	/* mark the individual sis */
	for (curr_si = (si) ? si : (AVND_SU_SI_REC *)m_NCS_DBLIST_FIND_FIRST(&su->si_list);
	     curr_si; curr_si = (si) ? 0 : (AVND_SU_SI_REC *)m_NCS_DBLIST_FIND_NEXT(&curr_si->su_dll_node)) {
		/* mark the si assigned / removed */
		if (m_AVND_SU_SI_CURR_ASSIGN_STATE_IS_ASSIGNING(curr_si)) {
			m_AVND_SU_SI_CURR_ASSIGN_STATE_SET(curr_si, AVND_SU_SI_ASSIGN_STATE_ASSIGNED);
			m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, curr_si, AVND_CKPT_SU_SI_REC_CURR_ASSIGN_STATE);
			m_AVND_LOG_SU_DB(AVND_LOG_SU_DB_SI_ASSIGN, AVND_LOG_SU_DB_SUCCESS,
					 &su->name, &curr_si->name, NCSFL_SEV_INFO);
		} else if (m_AVND_SU_SI_CURR_ASSIGN_STATE_IS_REMOVING(curr_si)) {
			m_AVND_SU_SI_CURR_ASSIGN_STATE_SET(curr_si, AVND_SU_SI_ASSIGN_STATE_REMOVED);
			m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, curr_si, AVND_CKPT_SU_SI_REC_CURR_ASSIGN_STATE);
			m_AVND_LOG_SU_DB(AVND_LOG_SU_DB_SI_REMOVE, AVND_LOG_SU_DB_SUCCESS,
					 &su->name, &curr_si->name, NCSFL_SEV_INFO);
		} else {
			m_AVND_LOG_INVALID_VAL_FATAL((long)si);
			m_AVND_LOG_INVALID_VAL_FATAL(curr_si->curr_assign_state);
			m_AVND_LOG_INVALID_VAL_FATAL(curr_si->prv_assign_state);
			m_AVND_LOG_INVALID_VAL_FATAL(curr_si->curr_state);
			m_AVND_LOG_INVALID_VAL_FATAL(curr_si->prv_state);
			m_AVND_LOG_INVALID_NAME_VAL_FATAL(curr_si->name.value, curr_si->name.length);
			assert(0);
		}

		/* 
		 * It is possible that the su fsm for npi su is never triggered (this 
		 * happens when the si-state change does not lead to su instantition or
		 * termination). Mark the corresponding csis assigned/removed.
		 */
		if (!m_AVND_SU_IS_PREINSTANTIABLE(su)) {
			for (curr_csi = (AVND_COMP_CSI_REC *)m_NCS_DBLIST_FIND_FIRST(&curr_si->csi_list);
			     curr_csi; curr_csi = (AVND_COMP_CSI_REC *)m_NCS_DBLIST_FIND_NEXT(&curr_csi->si_dll_node)) {
				if (m_AVND_SU_SI_CURR_ASSIGN_STATE_IS_ASSIGNED(curr_si))
					m_AVND_COMP_CSI_CURR_ASSIGN_STATE_SET(curr_csi,
									      AVND_COMP_CSI_ASSIGN_STATE_ASSIGNED);
				else if (m_AVND_SU_SI_CURR_ASSIGN_STATE_IS_REMOVED(curr_si))
					m_AVND_COMP_CSI_CURR_ASSIGN_STATE_SET(curr_csi,
									      AVND_COMP_CSI_ASSIGN_STATE_REMOVED);
				m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, curr_csi, AVND_CKPT_COMP_CSI_CURR_ASSIGN_STATE);
			}
		}
	}			/* for */

	/* inform AvD */
	if (!m_AVND_SU_IS_RESTART(su)) {
		rc = avnd_di_susi_resp_send(cb, su, si);
		if (NCSCC_RC_SUCCESS != rc)
			goto done;
	}

	/* finally delete the si(s) if they are removed */
	curr_si = (si) ? si : (AVND_SU_SI_REC *)m_NCS_DBLIST_FIND_FIRST(&su->si_list);
	if (m_AVND_SU_SI_CURR_ASSIGN_STATE_IS_REMOVED(curr_si)) {
		NCS_BOOL one_rec_del = FALSE;
		one_rec_del = (si) ? TRUE : FALSE;

		if (one_rec_del) {
			m_AVND_SEND_CKPT_UPDT_ASYNC_RMV(cb, curr_si, AVND_CKPT_SU_SI_REC);
		}
		rc = (si) ? avnd_su_si_rec_del(cb, &su->name, &si->name) : avnd_su_si_del(cb, &su->name);
		if (NCSCC_RC_SUCCESS != rc)
			goto done;

		/* removal signifies an end to the recovery phase.. initiate repair */
		if (m_AVND_SU_IS_FAILED(su) && !su->si_list.n_nodes)
			rc = avnd_err_su_repair(cb, su);
	}

	/* 
	 * reset the su-restart flag if all the sis are in assigned 
	 * state (signifying the end of su-restart phase)
	 */
	if (m_AVND_SU_IS_RESTART(su)) {
		m_AVND_SU_ARE_ALL_SI_ASSIGNED(su, are_si_assigned);
		if (TRUE == are_si_assigned) {
			m_AVND_SU_RESTART_RESET(su);
			m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, su, AVND_CKPT_SU_FLAG_CHANGE);
		}
	}

	/* finally initiate buffered assignments, if any */
	rc = avnd_su_siq_prc(cb, su);

done:
	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
  Name          : avnd_su_si_unmark
 
  Description   : This routine stops the SU-SI assignment & brings the all 
                  the SIs (including the corresponding CSIs) to the initial 
                  unassigned state.
 
  Arguments     : cb - ptr to the AvND control block
                  su - ptr to the comp
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
uns32 avnd_su_si_unmark(AVND_CB *cb, AVND_SU *su)
{
	AVND_SU_SI_REC *si = 0;
	uns32 rc = NCSCC_RC_SUCCESS;

	/* scan the su-si list & unmark the sis */
	for (si = (AVND_SU_SI_REC *)m_NCS_DBLIST_FIND_FIRST(&su->si_list);
	     si; si = (AVND_SU_SI_REC *)m_NCS_DBLIST_FIND_NEXT(&si->su_dll_node)) {
		rc = avnd_su_si_rec_unmark(cb, su, si);
		if (NCSCC_RC_SUCCESS != rc)
			break;
	}			/* for */

	return rc;
}

/****************************************************************************
  Name          : avnd_su_si_rec_unmark
 
  Description   : This routine unmarks a particular SU-SI assignment.
 
  Arguments     : cb - ptr to the AvND control block
                  su - ptr to the comp
                  si - ptr to the si rec
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
uns32 avnd_su_si_rec_unmark(AVND_CB *cb, AVND_SU *su, AVND_SU_SI_REC *si)
{
	AVND_COMP_CSI_REC *csi = 0;
	uns32 rc = NCSCC_RC_SUCCESS;

	/* reset the prv state & update the new assign-state */
	si->prv_state = 0;
	m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, si, AVND_CKPT_SU_SI_REC_PRV_STATE);
	si->prv_assign_state = 0;
	m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, si, AVND_CKPT_SU_SI_REC_PRV_ASSIGN_STATE);
	m_AVND_SU_SI_CURR_ASSIGN_STATE_SET(si, AVND_SU_SI_ASSIGN_STATE_UNASSIGNED);
	m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, si, AVND_CKPT_SU_SI_REC_CURR_ASSIGN_STATE);

	/* scan the si-csi list & unmark the csis */
	for (csi = (AVND_COMP_CSI_REC *)m_NCS_DBLIST_FIND_FIRST(&si->csi_list);
	     csi; csi = (AVND_COMP_CSI_REC *)m_NCS_DBLIST_FIND_NEXT(&csi->si_dll_node)) {
		csi->prv_assign_state = 0;
		m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, csi, AVND_CKPT_COMP_CSI_PRV_ASSIGN_STATE);
		m_AVND_COMP_CSI_CURR_ASSIGN_STATE_SET(csi, AVND_COMP_CSI_ASSIGN_STATE_UNASSIGNED);
		m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, csi, AVND_CKPT_COMP_CSI_CURR_ASSIGN_STATE);

		/* remove any pending callbacks for pi comps */
		if (m_AVND_COMP_TYPE_IS_PREINSTANTIABLE(csi->comp))
			avnd_comp_cbq_csi_rec_del(cb, csi->comp, &csi->name);
	}			/* for */

	return rc;
}

/***************************************************************************
 ******  S U   P R E S E N C E   F S M   P O R T I O N   S T A R T S  ******
 ***************************************************************************/

/****************************************************************************
  Name          : avnd_evt_avd_su_pres_msg
 
  Description   : This routine processes SU instantiate or terminate trigger 
                  as per the wishes of AvD.
 
  Arguments     : cb  - ptr to the AvND control block
                  evt - ptr to the AvND event
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
uns32 avnd_evt_avd_su_pres_evh(AVND_CB *cb, AVND_EVT *evt)
{
	AVSV_D2N_PRESENCE_SU_MSG_INFO *info = 0;
	AVND_SU *su = 0;
	uns32 rc = NCSCC_RC_SUCCESS;

	TRACE_ENTER();

	/* dont process unless AvD is up */
	if (!m_AVND_CB_IS_AVD_UP(cb))
		goto done;

	/* since AMFND is going down no need to process SU instantiate/terminate msg 
	 * because SU instantiate will cause component information to be read from IMMND
	 * and IMMND might have been already terminated and in that case AMFND will assert */
	if (cb->term_state == AVND_TERM_STATE_OPENSAF_SHUTDOWN)
		goto done;

	info = &evt->info.avd->msg_info.d2n_prsc_su;

	if (info->msg_id != (cb->rcv_msg_id + 1)) {
		/* Log Error */
		rc = NCSCC_RC_FAILURE;
		m_AVND_LOG_FOVER_EVTS(NCSFL_SEV_EMERGENCY, AVND_LOG_MSG_ID_MISMATCH, info->msg_id);

		goto done;
	}

	cb->rcv_msg_id = info->msg_id;

	/* get the su */
	su = m_AVND_SUDB_REC_GET(cb->sudb, info->su_name);
	if (!su)
		goto done;

	switch (info->term_state) {
	case TRUE:		/* => terminate the su */
		/* no si must be assigned to the su */
		assert(!m_NCS_DBLIST_FIND_FIRST(&su->si_list));

		/* Mark SU as terminated by admn operation */
		m_AVND_SU_ADMN_TERM_SET(su);
		m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, su, AVND_CKPT_SU_FLAG_CHANGE);

		/* delete all the curr info on su & comp */
		rc = avnd_su_curr_info_del(cb, su);

		/* now terminate the su */
		if (m_AVND_SU_IS_PREINSTANTIABLE(su)) {
			/* trigger su termination for pi su */
			rc = avnd_su_pres_fsm_run(cb, su, 0, AVND_SU_PRES_FSM_EV_TERM);
			if (NCSCC_RC_SUCCESS != rc)
				goto done;
		}
		break;

	case FALSE:		/* => instantiate the su */
		/* Reset admn term operation flag */
		m_AVND_SU_ADMN_TERM_RESET(su);
		m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, su, AVND_CKPT_SU_FLAG_CHANGE);
		/* add components belonging to this SU if components were not added before.
		   This can heppen when runtime first su is added and then comp is added. When su is added amfd will 
		   send su info to amfnd, at this point of time no component exists inimm db, so su list of comp is 
		   empty. When comp are added later, it is not sent to amfnd as amfnd reads comp info from immdb. 
		   Now when unlock-inst is done then avnd_evt_avd_su_pres_msg is called. At this point of time, we 
		   are not having Comp info in SU list, so need to add it.

		   If component exists as it would be in case controller is comming up with all entity configured,
		   then avnd_comp_config_get_su() will not read anything, it will return sucess. */
                if (avnd_comp_config_get_su(su) != NCSCC_RC_SUCCESS) {
                        m_AVND_SU_REG_FAILED_SET(su);
                        /* Will transition to instantiation-failed when instantiated */
                        LOG_ER("%s: FAILED", __FUNCTION__); 
                        rc = NCSCC_RC_FAILURE;
                        break;
                }
		/* trigger su instantiation for pi su */
		if (m_AVND_SU_IS_PREINSTANTIABLE(su))
			rc = avnd_su_pres_fsm_run(cb, su, 0, AVND_SU_PRES_FSM_EV_INST);
		else {
			if (m_AVND_SU_IS_REG_FAILED(su)) {
				/* The SU configuration is bad, we cannot do much other transition to failed state */
				m_AVND_SU_PRES_STATE_SET(su, SA_AMF_PRESENCE_INSTANTIATION_FAILED);
				m_AVND_SU_ALL_TERM_RESET(su);
				update_avd_presence_state(su);
			} else
				assert(0);
		}
		break;
	}			/* switch */

done:
	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
  Name          : avnd_su_pres_fsm_run
 
  Description   : This routine runs the service presence state FSM. It also 
                  processes the change in su presence state.
 
  Arguments     : cb   - ptr to the AvND control block
                  su   - ptr to the su
                  comp - ptr to the comp that generated the ev (may be null)
                  ev   - fsm event
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
uns32 avnd_su_pres_fsm_run(AVND_CB *cb, AVND_SU *su, AVND_COMP *comp, AVND_SU_PRES_FSM_EV ev)
{
	SaAmfPresenceStateT prv_st, final_st;
	uns32 rc = NCSCC_RC_SUCCESS;

	/* get the prv presence state */
	prv_st = su->pres;

	m_AVND_LOG_SU_FSM(AVND_LOG_SU_FSM_ENTER, prv_st, ev, &su->name, NCSFL_SEV_NOTICE);

	/* run the fsm */
	if (0 != avnd_su_pres_fsm[prv_st - 1][ev - 1]) {
		rc = avnd_su_pres_fsm[prv_st - 1][ev - 1] (cb, su, comp);
		if (NCSCC_RC_SUCCESS != rc)
			goto done;
	}

	/* get the final presence state */
	final_st = su->pres;

	m_AVND_LOG_SU_FSM(AVND_LOG_SU_FSM_EXIT, final_st, 0, &su->name, NCSFL_SEV_NOTICE);

	/* process state change */
	if (prv_st != final_st)
		rc = avnd_su_pres_st_chng_prc(cb, su, prv_st, final_st);

 done:
	return rc;
}

/****************************************************************************
  Name          : avnd_su_pres_st_chng_prc
 
  Description   : This routine processes the change in  the presence state 
                  resulting due to running the SU presence state FSM.
 
  Arguments     : cb       - ptr to the AvND control block
                  su       - ptr to the su
                  prv_st   - previous presence state
                  final_st - final presence state
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
uns32 avnd_su_pres_st_chng_prc(AVND_CB *cb, AVND_SU *su, SaAmfPresenceStateT prv_st, SaAmfPresenceStateT final_st)
{
	AVND_SU_SI_REC *si = 0;
	NCS_BOOL is_en;
	uns32 rc = NCSCC_RC_SUCCESS;

	/* pi su */
	if (m_AVND_SU_IS_PREINSTANTIABLE(su)) {
		/* instantiating -> instantiated */
		if ((SA_AMF_PRESENCE_INSTANTIATING == prv_st) && (SA_AMF_PRESENCE_INSTANTIATED == final_st)) {
			/* reset the su failed flag */
			if (m_AVND_SU_IS_FAILED(su)) {
				m_AVND_SU_FAILED_RESET(su);
				m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, su, AVND_CKPT_SU_FLAG_CHANGE);
			}

			/* determine the su oper state. if enabled, inform avd. */
			m_AVND_SU_IS_ENABLED(su, is_en);
			if (TRUE == is_en) {
				m_AVND_SU_OPER_STATE_SET(su, SA_AMF_OPERATIONAL_ENABLED);
				m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, su, AVND_CKPT_SU_OPER_STATE);
				rc = avnd_di_oper_send(cb, su, 0);
				if (NCSCC_RC_SUCCESS != rc)
					goto done;
			}
		}

		/* restarting -> instantiated */
		if ((SA_AMF_PRESENCE_RESTARTING == prv_st) && (SA_AMF_PRESENCE_INSTANTIATED == final_st)) {
			/* reset the su failed flag & set the oper state to enabled */
			if (m_AVND_SU_IS_FAILED(su)) {
				m_AVND_SU_FAILED_RESET(su);
				m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, su, AVND_CKPT_SU_FLAG_CHANGE);
				m_AVND_SU_OPER_STATE_SET(su, SA_AMF_OPERATIONAL_ENABLED);
				m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, su, AVND_CKPT_SU_OPER_STATE);
			}

			/* 
			 * reassign all the sis... 
			 * it's possible that the si was never assigned. send su-oper 
			 * enable msg instead.
			 */
			if (su->si_list.n_nodes)
				rc = avnd_su_si_reassign(cb, su);
			else
				rc = avnd_di_oper_send(cb, su, 0);
			if (NCSCC_RC_SUCCESS != rc)
				goto done;

		}

		/* terminating -> instantiated */
		if ((SA_AMF_PRESENCE_TERMINATING == prv_st) && (SA_AMF_PRESENCE_INSTANTIATED == final_st)) {
			/* reset the su failed flag */
			if (m_AVND_SU_IS_FAILED(su)) {
				m_AVND_SU_FAILED_RESET(su);
				m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, su, AVND_CKPT_SU_FLAG_CHANGE);
			}

			/* determine su oper state. if enabled, inform avd. */
			m_AVND_SU_IS_ENABLED(su, is_en);
			if (TRUE == is_en) {
				m_AVND_SU_OPER_STATE_SET(su, SA_AMF_OPERATIONAL_ENABLED);
				m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, su, AVND_CKPT_SU_OPER_STATE);
				rc = avnd_di_oper_send(cb, su, 0);
				if (NCSCC_RC_SUCCESS != rc)
					goto done;
			}
		}

		/* terminating -> uninstantiated */
		if ((SA_AMF_PRESENCE_TERMINATING == prv_st) && (SA_AMF_PRESENCE_UNINSTANTIATED == final_st)) {
			/* reset the su failed flag */
			if (m_AVND_SU_IS_FAILED(su)) {
				m_AVND_SU_FAILED_RESET(su);
				m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, su, AVND_CKPT_SU_FLAG_CHANGE);
			}

			/* reset the su restart falg */
			if (m_AVND_SU_IS_RESTART(su)) {
				m_AVND_SU_RESTART_RESET(su);
				m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, su, AVND_CKPT_SU_FLAG_CHANGE);
			}
		}

		/* instantiating -> inst-failed */
		if ((SA_AMF_PRESENCE_INSTANTIATING == prv_st) && (SA_AMF_PRESENCE_INSTANTIATION_FAILED == final_st)) {
			/* send the su-oper state msg (to indicate that instantiation failed) */
			m_AVND_SU_OPER_STATE_SET(su, SA_AMF_OPERATIONAL_DISABLED);
			m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, su, AVND_CKPT_SU_OPER_STATE);
			rc = avnd_di_oper_send(cb, su, AVSV_ERR_RCVR_SU_FAILOVER);
			if (NCSCC_RC_SUCCESS != rc)
				goto done;
		}

		/* instantiated/restarting -> inst-failed */
		if (((SA_AMF_PRESENCE_INSTANTIATED == prv_st) || (SA_AMF_PRESENCE_RESTARTING == prv_st)) &&
		    (SA_AMF_PRESENCE_INSTANTIATION_FAILED == final_st)) {
			/* send the su-oper state msg (to indicate that instantiation failed) */
			if (m_AVND_SU_OPER_STATE_IS_ENABLED(su)) {
				m_AVND_SU_OPER_STATE_SET(su, SA_AMF_OPERATIONAL_DISABLED);
				m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, su, AVND_CKPT_SU_OPER_STATE);
				rc = avnd_di_oper_send(cb, su, AVSV_ERR_RCVR_SU_FAILOVER);
				if (NCSCC_RC_SUCCESS != rc)
					goto done;
			}
		}

	}

	/* npi su */
	if (!m_AVND_SU_IS_PREINSTANTIABLE(su)) {
		/* get the only si */
		si = (AVND_SU_SI_REC *)m_NCS_DBLIST_FIND_FIRST(&su->si_list);
		assert(si);

		/* instantiating -> instantiated */
		if ((SA_AMF_PRESENCE_INSTANTIATING == prv_st) && (SA_AMF_PRESENCE_INSTANTIATED == final_st)) {
			/* si assignment success.. generate si-oper done indication */
			rc = avnd_su_si_oper_done(cb, su, m_AVND_SU_IS_ALL_SI(su) ? 0 : si);
			m_AVND_SU_ALL_SI_RESET(su);
			m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, su, AVND_CKPT_SU_FLAG_CHANGE);
		}

		/* instantiating/instantiated/restarting -> inst-failed */
		if (((SA_AMF_PRESENCE_INSTANTIATING == prv_st) ||
		     (SA_AMF_PRESENCE_INSTANTIATED == prv_st) ||
		     (SA_AMF_PRESENCE_RESTARTING == prv_st)) && (SA_AMF_PRESENCE_INSTANTIATION_FAILED == final_st)) {
			/* si-assignment failed .. inform avd */
			rc = avnd_di_susi_resp_send(cb, su, si);
			if (NCSCC_RC_SUCCESS != rc)
				goto done;

			/* mark su as failed */
			m_AVND_SU_FAILED_SET(su);
			m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, su, AVND_CKPT_SU_FLAG_CHANGE);

			/* npi su is disabled in inst-fail state */
			m_AVND_SU_OPER_STATE_SET(su, SA_AMF_OPERATIONAL_DISABLED);
			m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, su, AVND_CKPT_SU_OPER_STATE);

			rc = avnd_di_oper_send(cb, su, AVSV_ERR_RCVR_SU_FAILOVER);

		}

		/* restarting -> instantiated */
		if ((SA_AMF_PRESENCE_RESTARTING == prv_st) && (SA_AMF_PRESENCE_INSTANTIATED == final_st)) {
			/* reset the failed & restart flag */
			m_AVND_SU_FAILED_RESET(su);
			m_AVND_SU_RESTART_RESET(su);
			m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, su, AVND_CKPT_SU_FLAG_CHANGE);
		}

		/* terminating -> uninstantiated */
		if ((SA_AMF_PRESENCE_TERMINATING == prv_st) && (SA_AMF_PRESENCE_UNINSTANTIATED == final_st)) {
			/* si assignment/removal success.. generate si-oper done indication */
			rc = avnd_su_si_oper_done(cb, su, m_AVND_SU_IS_ALL_SI(su) ? 0 : si);
			m_AVND_SU_ALL_SI_RESET(su);
			m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, su, AVND_CKPT_SU_FLAG_CHANGE);

			/* npi su is enabled in uninstantiated state */
			m_AVND_SU_OPER_STATE_SET(su, SA_AMF_OPERATIONAL_ENABLED);
			m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, su, AVND_CKPT_SU_OPER_STATE);
			rc = avnd_di_oper_send(cb, su, 0);
		}

		/* terminating/instantiated/restarting -> term-failed */
		if (((SA_AMF_PRESENCE_TERMINATING == prv_st) ||
		     (SA_AMF_PRESENCE_INSTANTIATED == prv_st) ||
		     (SA_AMF_PRESENCE_RESTARTING == prv_st)) && (SA_AMF_PRESENCE_TERMINATION_FAILED == final_st)) {
			/* si assignment/removal failed.. inform AvD */
			rc = avnd_di_susi_resp_send(cb, su, si);
		}
	}

	/* inform avd of the change in presence state */
	rc = update_avd_presence_state(su);

	if (NCSCC_RC_SUCCESS != rc)
		goto done;

 done:
	return rc;
}

/****************************************************************************
  Name          : avnd_su_pres_uninst_suinst_hdler
 
  Description   : This routine processes the `SU Instantiate` event in 
                  `Uninstantiated` state. For PI SU, this event is generated
                  when SU is to be instantiated. For NPI SU, this event is 
                  generated when an active SI has to be assigned.
 
  Arguments     : cb   - ptr to the AvND control block
                  su   - ptr to the su
                  comp - ptr to the comp
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
uns32 avnd_su_pres_uninst_suinst_hdler(AVND_CB *cb, AVND_SU *su, AVND_COMP *comp)
{
	AVND_COMP *curr_comp = 0;
	AVND_SU_SI_REC *si = 0;
	AVND_COMP_CSI_REC *csi = 0;
	uns32 rc = NCSCC_RC_SUCCESS;

	/* 
	 * If pi su, pick the first pi comp & trigger it's FSM with InstEv.
	 */
	if (m_AVND_SU_IS_PREINSTANTIABLE(su)) {
		for (curr_comp = m_AVND_COMP_FROM_SU_DLL_NODE_GET(m_NCS_DBLIST_FIND_FIRST(&su->comp_list));
		     curr_comp;
		     curr_comp = m_AVND_COMP_FROM_SU_DLL_NODE_GET(m_NCS_DBLIST_FIND_NEXT(&curr_comp->su_dll_node))) {
			/* instantiate the pi comp */
			if (m_AVND_COMP_TYPE_IS_PREINSTANTIABLE(curr_comp)) {
				rc = avnd_comp_clc_fsm_run(cb, curr_comp, AVND_COMP_CLC_PRES_FSM_EV_INST);
				if (NCSCC_RC_SUCCESS != rc)
					goto done;
				break;
			}
		}		/* for */
	}

	/* 
	 * If npi su, it'll have only one si-rec in the si-list. Pick the 
	 * lowest ranked csi belonging to this si & trigger it's comp fsm.
	 */
	if (!m_AVND_SU_IS_PREINSTANTIABLE(su)) {
		/* get the only si rec */
		si = (AVND_SU_SI_REC *)m_NCS_DBLIST_FIND_FIRST(&su->si_list);
		assert(si);

		csi = (AVND_COMP_CSI_REC *)m_NCS_DBLIST_FIND_FIRST(&si->csi_list);
		if (csi) {
			/* mark the csi state assigning */
			m_AVND_COMP_CSI_CURR_ASSIGN_STATE_SET(csi, AVND_COMP_CSI_ASSIGN_STATE_ASSIGNING);
			m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, csi, AVND_CKPT_COMP_CSI_CURR_ASSIGN_STATE);

			/* instantiate the comp */
			rc = avnd_comp_clc_fsm_run(cb, csi->comp, AVND_COMP_CLC_PRES_FSM_EV_INST);
			if (NCSCC_RC_SUCCESS != rc)
				goto done;
		}
	}

	/* transition to instantiating state */
	m_AVND_SU_PRES_STATE_SET(su, SA_AMF_PRESENCE_INSTANTIATING);
	m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, su, AVND_CKPT_SU_PRES_STATE);

 done:
	return rc;
}

/****************************************************************************
  Name          : avnd_su_pres_insting_suterm_hdler
 
  Description   : This routine processes the `SU Terminate` event in 
                  `Instantiating` state.
 
  Arguments     : cb   - ptr to the AvND control block
                  su   - ptr to the su
                  comp - ptr to the comp
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : Note that this event is only generated for PI SUs. SUTermEv
                  is generated for NPI SUs only during stable states i.e.
                  uninstantiated & instantiated states. This is done by 
                  avoiding overlapping SI assignments.
******************************************************************************/
uns32 avnd_su_pres_insting_suterm_hdler(AVND_CB *cb, AVND_SU *su, AVND_COMP *comp)
{
	AVND_COMP *curr_comp = 0;
	uns32 rc = NCSCC_RC_SUCCESS;

	/* 
	 * If pi su, pick all the instantiated/instantiating pi comps & 
	 * trigger their FSM with TermEv.
	 */
	for (curr_comp = m_AVND_COMP_FROM_SU_DLL_NODE_GET(m_NCS_DBLIST_FIND_FIRST(&su->comp_list));
	     curr_comp; curr_comp = m_AVND_COMP_FROM_SU_DLL_NODE_GET(m_NCS_DBLIST_FIND_NEXT(&curr_comp->su_dll_node))) {
		/* 
		 * skip the npi comps.. as the su is yet to be instantiated, 
		 * there are no SIs assigned.
		 */
		if (!m_AVND_COMP_TYPE_IS_PREINSTANTIABLE(curr_comp))
			continue;

		/* terminate the non-uninstantiated pi comp */
		if (!m_AVND_COMP_PRES_STATE_IS_UNINSTANTIATED(curr_comp)) {
			rc = avnd_comp_clc_fsm_run(cb, curr_comp, AVND_COMP_CLC_PRES_FSM_EV_TERM);
			if (NCSCC_RC_SUCCESS != rc)
				goto done;
		}
	}			/* for */

	/* transition to terminating state */
	m_AVND_SU_PRES_STATE_SET(su, SA_AMF_PRESENCE_TERMINATING);
	m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, su, AVND_CKPT_SU_PRES_STATE);

 done:
	return rc;
}

/****************************************************************************
  Name          : avnd_su_pres_insting_surestart_hdler
 
  Description   : This routine processes the `SU Restart` event in 
                  `Instantiating` state.
 
  Arguments     : cb   - ptr to the AvND control block
                  su   - ptr to the su
                  comp - ptr to the comp
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
uns32 avnd_su_pres_insting_surestart_hdler(AVND_CB *cb, AVND_SU *su, AVND_COMP *comp)
{				/* TBD */
	uns32 rc = NCSCC_RC_SUCCESS;

	return rc;
}

/****************************************************************************
  Name          : avnd_su_pres_insting_compinst_hdler
 
  Description   : This routine processes the `CompInstantiated` event in 
                  `Instantiating` state.
 
  Arguments     : cb   - ptr to the AvND control block
                  su   - ptr to the su
                  comp - ptr to the comp
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
uns32 avnd_su_pres_insting_compinst_hdler(AVND_CB *cb, AVND_SU *su, AVND_COMP *comp)
{
	AVND_COMP *curr_comp = 0;
	AVND_COMP_CSI_REC *curr_csi = 0;
	NCS_BOOL is;
	uns32 rc = NCSCC_RC_SUCCESS;

	/* 
	 * If pi su, pick the next pi comp & trigger it's FSM with InstEv.
	 * If the component is marked failed (=> component has reinstantiated 
	 * after some failure), unmark it & determine the su presence state.
	 */
	if (m_AVND_SU_IS_PREINSTANTIABLE(su)) {
		if (m_AVND_COMP_IS_FAILED(comp)) {
			m_AVND_COMP_FAILED_RESET(comp);
			m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, comp, AVND_CKPT_COMP_FLAG_CHANGE);
		} else {
			for (curr_comp = m_AVND_COMP_FROM_SU_DLL_NODE_GET(m_NCS_DBLIST_FIND_NEXT(&comp->su_dll_node));
			     curr_comp;
			     curr_comp =
			     m_AVND_COMP_FROM_SU_DLL_NODE_GET(m_NCS_DBLIST_FIND_NEXT(&curr_comp->su_dll_node))) {
				/* instantiate the pi comp */
				if (m_AVND_COMP_TYPE_IS_PREINSTANTIABLE(curr_comp)) {
					rc = avnd_comp_clc_fsm_run(cb, curr_comp, AVND_COMP_CLC_PRES_FSM_EV_INST);
					if (NCSCC_RC_SUCCESS != rc)
						goto done;
					break;
				}
			}	/* for */
		}

		/* determine su presence state */
		m_AVND_SU_IS_INSTANTIATED(su, is);
		if (TRUE == is) {
			m_AVND_SU_PRES_STATE_SET(su, SA_AMF_PRESENCE_INSTANTIATED);
			m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, su, AVND_CKPT_SU_PRES_STATE);
		}
	}

	/* 
	 * If npi su, pick the next csi & trigger it's comp fsm with InstEv.
	 */
	if (!m_AVND_SU_IS_PREINSTANTIABLE(su)) {
		/* get the only csi rec */
		curr_csi = m_AVND_CSI_REC_FROM_COMP_DLL_NODE_GET(m_NCS_DBLIST_FIND_FIRST(&comp->csi_list));
		assert(curr_csi);

		/* mark the csi state assigned */
		m_AVND_COMP_CSI_CURR_ASSIGN_STATE_SET(curr_csi, AVND_COMP_CSI_ASSIGN_STATE_ASSIGNED);
		m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, curr_csi, AVND_CKPT_COMP_CSI_CURR_ASSIGN_STATE);

		/* get the next csi */
		curr_csi = (AVND_COMP_CSI_REC *)m_NCS_DBLIST_FIND_NEXT(&curr_csi->si_dll_node);
		if (curr_csi) {
			/* we have another csi. trigger the comp fsm with InstEv */
			rc = avnd_comp_clc_fsm_trigger(cb, curr_csi->comp, AVND_COMP_CLC_PRES_FSM_EV_INST);
			if (NCSCC_RC_SUCCESS != rc)
				goto done;
		} else {
			/* => si assignment done */
			m_AVND_SU_PRES_STATE_SET(su, SA_AMF_PRESENCE_INSTANTIATED);
			m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, su, AVND_CKPT_SU_PRES_STATE);
		}
	}

 done:
	return rc;
}

/****************************************************************************
  Name          : avnd_su_pres_insting_compinstfail_hdler
 
  Description   : This routine processes the `CompInstantiateFailed` event in 
                  `Instantiating` state.
                  For PI SU, a failure in the instantiation of any component
                  means that SU instantiation has failed. Terminate all the 
                  already instantiated (or instantiating) components & 
                  transition to inst-failed state.
                  For NPI SU, a failure in the instantiation of any component
                  indicates that active SI assignment to this SU has failed.
                  Terminate all the components to whom the active CSI has 
                  already been assigned (or assigning).
 
  Arguments     : cb   - ptr to the AvND control block
                  su   - ptr to the su
                  comp - ptr to the comp
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None. 
******************************************************************************/
uns32 avnd_su_pres_insting_compinstfail_hdler(AVND_CB *cb, AVND_SU *su, AVND_COMP *comp)
{
	AVND_COMP *curr_comp = 0;
	AVND_SU_SI_REC *si = 0;
	AVND_COMP_CSI_REC *curr_csi = 0;
	uns32 rc = NCSCC_RC_SUCCESS;

	/* transition to inst-failed state */
	m_AVND_SU_PRES_STATE_SET(su, SA_AMF_PRESENCE_INSTANTIATION_FAILED);
	m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, su, AVND_CKPT_SU_PRES_STATE);
	m_AVND_SU_ALL_TERM_RESET(su);
	m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, su, AVND_CKPT_SU_FLAG_CHANGE);

	/* 
	 * If pi su, pick all the instantiated/instantiating pi comps & 
	 * trigger their FSM with TermEv.
	 */
	if (m_AVND_SU_IS_PREINSTANTIABLE(su)) {
		for (curr_comp = m_AVND_COMP_FROM_SU_DLL_NODE_GET(m_NCS_DBLIST_FIND_LAST(&su->comp_list));
		     curr_comp;
		     curr_comp = m_AVND_COMP_FROM_SU_DLL_NODE_GET(m_NCS_DBLIST_FIND_PREV(&curr_comp->su_dll_node))) {
			/* skip the npi comps */
			if (!m_AVND_COMP_TYPE_IS_PREINSTANTIABLE(curr_comp))
				continue;

			/* terminate the non-uninstantiated pi comp */
			if (!m_AVND_COMP_PRES_STATE_IS_UNINSTANTIATED(curr_comp)) {
				rc = avnd_comp_clc_fsm_run(cb, curr_comp, AVND_COMP_CLC_PRES_FSM_EV_TERM);
				if (NCSCC_RC_SUCCESS != rc)
					goto done;
			}
		}		/* for */
	}

	/* 
	 * If npi su, pick all the assigned/assigning comps &
	 * trigger their comp fsm with TermEv
	 */
	if (!m_AVND_SU_IS_PREINSTANTIABLE(su)) {
		/* get the only si rec */
		si = (AVND_SU_SI_REC *)m_NCS_DBLIST_FIND_FIRST(&su->si_list);
		assert(si);

		for (curr_csi = (AVND_COMP_CSI_REC *)m_NCS_DBLIST_FIND_LAST(&si->csi_list);
		     curr_csi; curr_csi = (AVND_COMP_CSI_REC *)m_NCS_DBLIST_FIND_PREV(&curr_csi->si_dll_node)) {
			/* terminate the component containing non-unassigned csi */
			if (!m_AVND_COMP_CSI_CURR_ASSIGN_STATE_IS_UNASSIGNED(curr_csi) &&
			    (curr_csi->comp->pres != SA_AMF_PRESENCE_INSTANTIATION_FAILED)) {
				rc = avnd_comp_clc_fsm_run(cb, curr_csi->comp, AVND_COMP_CLC_PRES_FSM_EV_TERM);
				if (NCSCC_RC_SUCCESS != rc)
					goto done;
			}
		}		/* for */
	}

 done:
	return rc;
}

/****************************************************************************
  Name          : avnd_su_pres_inst_suterm_hdler
 
  Description   : This routine processes the `SU Terminate` event in 
                  `Instantiated` state.
 
  Arguments     : cb   - ptr to the AvND control block
                  su   - ptr to the su
                  comp - ptr to the comp
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
uns32 avnd_su_pres_inst_suterm_hdler(AVND_CB *cb, AVND_SU *su, AVND_COMP *comp)
{
	AVND_COMP *curr_comp = 0;
	AVND_SU_SI_REC *si = 0;
	AVND_COMP_CSI_REC *csi = 0;
	uns32 rc = NCSCC_RC_SUCCESS;

	/* 
	 * If pi su, pick the last pi comp & trigger it's FSM with TermEv.
	 */
	if (m_AVND_SU_IS_PREINSTANTIABLE(su)) {
		for (curr_comp = m_AVND_COMP_FROM_SU_DLL_NODE_GET(m_NCS_DBLIST_FIND_LAST(&su->comp_list));
		     curr_comp;
		     curr_comp = m_AVND_COMP_FROM_SU_DLL_NODE_GET(m_NCS_DBLIST_FIND_PREV(&curr_comp->su_dll_node))) {
			/* terminate the pi comp */
			if (m_AVND_COMP_TYPE_IS_PREINSTANTIABLE(curr_comp)) {
				rc = avnd_comp_clc_fsm_run(cb, curr_comp, AVND_COMP_CLC_PRES_FSM_EV_TERM);
				if (NCSCC_RC_SUCCESS != rc)
					goto done;
				break;
			}
		}		/* for */
	}

	/* 
	 * If npi su, it'll have only one si-rec in the si-list. Pick the 
	 * highest ranked csi belonging to this si & trigger it's comp fsm.
	 */
	if (!m_AVND_SU_IS_PREINSTANTIABLE(su)) {
		/* get the only si rec */
		si = (AVND_SU_SI_REC *)m_NCS_DBLIST_FIND_FIRST(&su->si_list);
		assert(si);
		csi = (AVND_COMP_CSI_REC *)m_NCS_DBLIST_FIND_LAST(&si->csi_list);
		assert(csi);

		/* mark the csi state assigning/removing */
		if (m_AVND_SU_SI_CURR_ASSIGN_STATE_IS_REMOVING(si))
			m_AVND_COMP_CSI_CURR_ASSIGN_STATE_SET(csi, AVND_COMP_CSI_ASSIGN_STATE_REMOVING);
		else
			m_AVND_COMP_CSI_CURR_ASSIGN_STATE_SET(csi, AVND_COMP_CSI_ASSIGN_STATE_ASSIGNING);

		m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, csi, AVND_CKPT_COMP_CSI_CURR_ASSIGN_STATE);
		/* terminate the comp */
		rc = avnd_comp_clc_fsm_run(cb, csi->comp, (m_AVND_COMP_IS_FAILED(csi->comp)) ?
					   AVND_COMP_CLC_PRES_FSM_EV_CLEANUP : AVND_COMP_CLC_PRES_FSM_EV_TERM);
		if (NCSCC_RC_SUCCESS != rc)
			goto done;
	}

	/* transition to terminating state */
	m_AVND_SU_PRES_STATE_SET(su, SA_AMF_PRESENCE_TERMINATING);
	m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, su, AVND_CKPT_SU_PRES_STATE);

 done:
	return rc;
}

/****************************************************************************
  Name          : avnd_su_pres_inst_surestart_hdler
 
  Description   : This routine processes the `SU Restart` event in 
                  `Instantiated` state.
 
  Arguments     : cb   - ptr to the AvND control block
                  su   - ptr to the su
                  comp - ptr to the comp
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
uns32 avnd_su_pres_inst_surestart_hdler(AVND_CB *cb, AVND_SU *su, AVND_COMP *comp)
{
	AVND_COMP *curr_comp = 0;
	AVND_SU_SI_REC *si = 0;
	AVND_COMP_CSI_REC *csi = 0;
	uns32 rc = NCSCC_RC_SUCCESS;

	/* 
	 * If pi su, pick the first pi comp & trigger it's FSM with RestartEv.
	 */
	if (m_AVND_SU_IS_PREINSTANTIABLE(su)) {
		for (curr_comp = m_AVND_COMP_FROM_SU_DLL_NODE_GET(m_NCS_DBLIST_FIND_FIRST(&su->comp_list));
		     curr_comp;
		     curr_comp = m_AVND_COMP_FROM_SU_DLL_NODE_GET(m_NCS_DBLIST_FIND_NEXT(&curr_comp->su_dll_node))) {
			/* restart the pi comp */
			if (m_AVND_COMP_TYPE_IS_PREINSTANTIABLE(curr_comp)) {
				rc = avnd_comp_clc_fsm_run(cb, curr_comp, AVND_COMP_CLC_PRES_FSM_EV_RESTART);
				if (NCSCC_RC_SUCCESS != rc)
					goto done;
				break;
			}
		}		/* for */
	}

	/* 
	 * If npi su, it'll have only one si-rec in the si-list. Pick the 
	 * lowest ranked csi belonging to this si & trigger it's comp fsm.
	 */
	if (!m_AVND_SU_IS_PREINSTANTIABLE(su)) {
		/* get the only si rec */
		si = (AVND_SU_SI_REC *)m_NCS_DBLIST_FIND_FIRST(&su->si_list);
		assert(si);

		csi = (AVND_COMP_CSI_REC *)m_NCS_DBLIST_FIND_FIRST(&si->csi_list);
		if (csi) {
			/* mark the csi state assigning */
			m_AVND_COMP_CSI_CURR_ASSIGN_STATE_SET(csi, AVND_COMP_CSI_ASSIGN_STATE_ASSIGNING);
			m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, csi, AVND_CKPT_COMP_CSI_CURR_ASSIGN_STATE);

			/* restart the comp */
			rc = avnd_comp_clc_fsm_run(cb, csi->comp, AVND_COMP_CLC_PRES_FSM_EV_RESTART);
			if (NCSCC_RC_SUCCESS != rc)
				goto done;
		}
	}

	/* transition to restarting state */
	m_AVND_SU_PRES_STATE_SET(su, SA_AMF_PRESENCE_RESTARTING);
	m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, su, AVND_CKPT_SU_PRES_STATE);

 done:
	return rc;
}

/****************************************************************************
  Name          : avnd_su_pres_inst_comprestart_hdler
 
  Description   : This routine processes the `Comp Restart` event in 
                  `Instantiated` state.
 
  Arguments     : cb   - ptr to the AvND control block
                  su   - ptr to the su
                  comp - ptr to the comp
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
uns32 avnd_su_pres_inst_comprestart_hdler(AVND_CB *cb, AVND_SU *su, AVND_COMP *comp)
{				/* TBD */
	uns32 rc = NCSCC_RC_SUCCESS;

	return rc;
}

/****************************************************************************
  Name          : avnd_su_pres_inst_compterming_hdler
 
  Description   : This routine processes the `Comp Terminating` event in 
                  `Instantiated` state.
 
  Arguments     : cb   - ptr to the AvND control block
                  su   - ptr to the su
                  comp - ptr to the comp
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
uns32 avnd_su_pres_inst_compterming_hdler(AVND_CB *cb, AVND_SU *su, AVND_COMP *comp)
{
	uns32 rc = NCSCC_RC_SUCCESS;

	if (m_AVND_SU_IS_PREINSTANTIABLE(su)) {
		if (m_AVND_SU_IS_FAILED(su)) {
			/* transition to terminating state */
			m_AVND_SU_PRES_STATE_SET(su, SA_AMF_PRESENCE_TERMINATING);
			m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, su, AVND_CKPT_SU_PRES_STATE);
		}
	}

	return rc;
}

/****************************************************************************
  Name          : avnd_su_pres_terming_compinst_hdler
 
  Description   : This routine processes the `Comp Instantiate` event in 
                  `Terminating` state.
 
  Arguments     : cb   - ptr to the AvND control block
                  su   - ptr to the su
                  comp - ptr to the comp
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
uns32 avnd_su_pres_terming_compinst_hdler(AVND_CB *cb, AVND_SU *su, AVND_COMP *comp)
{
	NCS_BOOL is;
	uns32 rc = NCSCC_RC_SUCCESS;

	if (m_AVND_SU_IS_PREINSTANTIABLE(su)) {
		if (m_AVND_COMP_IS_FAILED(comp)) {
			m_AVND_COMP_FAILED_RESET(comp);
			m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, comp, AVND_CKPT_COMP_FLAG_CHANGE);
		}

		/* determine if su can be transitioned to instantiated state */
		m_AVND_SU_IS_INSTANTIATED(su, is);
		if (TRUE == is) {
			m_AVND_SU_PRES_STATE_SET(su, SA_AMF_PRESENCE_INSTANTIATED);
			m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, su, AVND_CKPT_SU_PRES_STATE);
		}
	}

	return rc;
}

/****************************************************************************
  Name          : avnd_su_pres_terming_comptermfail_hdler
 
  Description   : This routine processes the `Comp Termination Failed` event in 
                  `Terminating` state.
                  For PI SU, a failure in the termination of any component
                  means that SU termination has failed. Terminate all the 
                  already instantiated (or instantiating) components & 
                  transition to term-failed state.
                  For NPI SU, a failure in the termination of any component
                  indicates that active SI assignment to this SU has failed.
                  Terminate all the components to whom the active CSI has 
                  already been assigned (or assigning).
 
  Arguments     : cb   - ptr to the AvND control block
                  su   - ptr to the su
                  comp - ptr to the comp
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : This function is reused for all state changes into termfail
******************************************************************************/
uns32 avnd_su_pres_terming_comptermfail_hdler(AVND_CB *cb, AVND_SU *su, AVND_COMP *comp)
{
	AVND_COMP *curr_comp = 0;
	AVND_SU_SI_REC *si = 0;
	AVND_COMP_CSI_REC *curr_csi = 0;
	uns32 rc = NCSCC_RC_SUCCESS;

	/* 
	 * If pi su, pick all the instantiated/instantiating pi comps & 
	 * trigger their FSM with TermEv.
	 */
	if (m_AVND_SU_IS_PREINSTANTIABLE(su)) {
		for (curr_comp = m_AVND_COMP_FROM_SU_DLL_NODE_GET(m_NCS_DBLIST_FIND_FIRST(&su->comp_list));
		     curr_comp;
		     curr_comp = m_AVND_COMP_FROM_SU_DLL_NODE_GET(m_NCS_DBLIST_FIND_NEXT(&curr_comp->su_dll_node))) {
			/* skip the npi comps */
			if (!m_AVND_COMP_TYPE_IS_PREINSTANTIABLE(curr_comp))
				continue;

			/* terminate the non-uninstantiated pi comp */
			if (!m_AVND_COMP_PRES_STATE_IS_UNINSTANTIATED(curr_comp)) {
				if (m_AVND_COMP_PRES_STATE_IS_INSTANTIATED(curr_comp) &&
				    (!m_AVND_COMP_IS_FAILED(curr_comp)))
					rc = avnd_comp_clc_fsm_trigger(cb, curr_comp, AVND_COMP_CLC_PRES_FSM_EV_TERM);
				else
					rc = avnd_comp_clc_fsm_trigger(cb, curr_comp,
								       AVND_COMP_CLC_PRES_FSM_EV_CLEANUP);

				if (NCSCC_RC_SUCCESS != rc)
					goto done;
			}
		}		/* for */
	}

	/* 
	 * If npi su, pick all the assigned/assigning comps &
	 * trigger their comp fsm with TermEv
	 */
	if (!m_AVND_SU_IS_PREINSTANTIABLE(su)) {
		/* get the only si rec */
		si = (AVND_SU_SI_REC *)m_NCS_DBLIST_FIND_FIRST(&su->si_list);
		assert(si);

		for (curr_csi = (AVND_COMP_CSI_REC *)m_NCS_DBLIST_FIND_FIRST(&si->csi_list);
		     curr_csi; curr_csi = (AVND_COMP_CSI_REC *)m_NCS_DBLIST_FIND_NEXT(&curr_csi->si_dll_node)) {
			/* terminate the component containing non-unassigned csi */
			if (!m_AVND_COMP_CSI_CURR_ASSIGN_STATE_IS_UNASSIGNED(curr_csi)) {
				rc = avnd_comp_clc_fsm_trigger(cb, curr_csi->comp, AVND_COMP_CLC_PRES_FSM_EV_TERM);
				if (NCSCC_RC_SUCCESS != rc)
					goto done;
			}
		}		/* for */
	}

	/* transition to term-failed state */
	m_AVND_SU_PRES_STATE_SET(su, SA_AMF_PRESENCE_TERMINATION_FAILED);
	m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, su, AVND_CKPT_SU_PRES_STATE);

	if (TRUE == su->is_ncs) {
		char reason[SA_MAX_NAME_LENGTH + 64];
		snprintf(reason, sizeof(reason) - 1, "SU '%s' Termination-failed", su->name.value);
		opensaf_reboot(avnd_cb->node_info.nodeId, (char *)avnd_cb->node_info.executionEnvironment.value,
				reason);
	}

	/* Now check if in the context of shutdown all app SUs 
	 ** and do the needful
	 */
	if ((cb->term_state == AVND_TERM_STATE_SHUTTING_APP_SU) || (cb->term_state == AVND_TERM_STATE_SHUTTING_NCS_SU)) {
		avnd_check_su_shutdown_done(cb, su->is_ncs);
	}

 done:
	return rc;
}

/****************************************************************************
  Name          : avnd_su_pres_terming_compuninst_hdler
 
  Description   : This routine processes the `Comp Uninstantiated` event in 
                  `Terminating` state.
 
  Arguments     : cb   - ptr to the AvND control block
                  su   - ptr to the su
                  comp - ptr to the comp
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
uns32 avnd_su_pres_terming_compuninst_hdler(AVND_CB *cb, AVND_SU *su, AVND_COMP *comp)
{
	AVND_COMP *curr_comp = 0;
	AVND_COMP_CSI_REC *curr_csi = 0;
	NCS_BOOL all_uninst = TRUE;
	uns32 rc = NCSCC_RC_SUCCESS;

	/* This case is for handling the case of admn su term while su is restarting */
	if (m_AVND_SU_IS_PREINSTANTIABLE(su) && m_AVND_SU_IS_FAILED(su) && m_AVND_SU_IS_ADMN_TERM(su)) {
		for (curr_comp = m_AVND_COMP_FROM_SU_DLL_NODE_GET(m_NCS_DBLIST_FIND_FIRST(&su->comp_list));
		     curr_comp;
		     curr_comp = m_AVND_COMP_FROM_SU_DLL_NODE_GET(m_NCS_DBLIST_FIND_NEXT(&curr_comp->su_dll_node))) {
			if ((curr_comp->pres != SA_AMF_PRESENCE_UNINSTANTIATED) &&
			    (m_AVND_COMP_TYPE_IS_PREINSTANTIABLE(curr_comp)))
				all_uninst = FALSE;
		}

		if (all_uninst == TRUE) {
			m_AVND_SU_PRES_STATE_SET(su, SA_AMF_PRESENCE_UNINSTANTIATED);
			m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, su, AVND_CKPT_SU_PRES_STATE);

			/* Now check if in the context of shutdown all app SUs 
			 ** and do the needful
			 */
			if ((cb->term_state == AVND_TERM_STATE_SHUTTING_APP_SU) ||
			    (cb->term_state == AVND_TERM_STATE_SHUTTING_NCS_SU)) {
				avnd_check_su_shutdown_done(cb, su->is_ncs);
			}
		}

	}

	/* 
	 * If pi su, pick the prv pi comp & trigger it's FSM with TermEv.
	 */
	if (m_AVND_SU_IS_PREINSTANTIABLE(su) && !m_AVND_SU_IS_FAILED(su)) {
		for (curr_comp = m_AVND_COMP_FROM_SU_DLL_NODE_GET(m_NCS_DBLIST_FIND_PREV(&comp->su_dll_node));
		     curr_comp;
		     curr_comp = m_AVND_COMP_FROM_SU_DLL_NODE_GET(m_NCS_DBLIST_FIND_PREV(&curr_comp->su_dll_node))) {
			/* terminate the pi comp */
			if (m_AVND_COMP_TYPE_IS_PREINSTANTIABLE(curr_comp)) {
				rc = avnd_comp_clc_fsm_run(cb, curr_comp, AVND_COMP_CLC_PRES_FSM_EV_TERM);
				if (NCSCC_RC_SUCCESS != rc)
					goto done;
				break;
			}
		}		/* for */

		/* 
		 * if curr-comp is null, => all the pi comps are terminated.
		 * transition to terminate state.
		 */
		if (!curr_comp) {
			m_AVND_SU_PRES_STATE_SET(su, SA_AMF_PRESENCE_UNINSTANTIATED);
			m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, su, AVND_CKPT_SU_PRES_STATE);

			/* Now check if in the context of shutdown all app SUs 
			 ** and do the needful
			 */
			if ((cb->term_state == AVND_TERM_STATE_SHUTTING_APP_SU) ||
			    (cb->term_state == AVND_TERM_STATE_SHUTTING_NCS_SU)) {
				avnd_check_su_shutdown_done(cb, su->is_ncs);
			}
		}
	}

	/* 
	 * If npi su, pick the prv csi & trigger it's comp fsm with TermEv.
	 */
	if (!m_AVND_SU_IS_PREINSTANTIABLE(su)) {
		/* get the only csi rec */
		curr_csi = m_AVND_CSI_REC_FROM_COMP_DLL_NODE_GET(m_NCS_DBLIST_FIND_FIRST(&comp->csi_list));
		assert(curr_csi);

		/* mark the csi state assigned/removed */
		if (m_AVND_SU_SI_CURR_ASSIGN_STATE_IS_REMOVING(curr_csi->si))
			m_AVND_COMP_CSI_CURR_ASSIGN_STATE_SET(curr_csi, AVND_COMP_CSI_ASSIGN_STATE_REMOVED);
		else
			m_AVND_COMP_CSI_CURR_ASSIGN_STATE_SET(curr_csi, AVND_COMP_CSI_ASSIGN_STATE_ASSIGNED);

		m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, curr_csi, AVND_CKPT_COMP_CSI_CURR_ASSIGN_STATE);
		/* get the prv csi */
		curr_csi = (AVND_COMP_CSI_REC *)m_NCS_DBLIST_FIND_PREV(&curr_csi->si_dll_node);
		if (curr_csi) {
			/* we have another csi. trigger the comp fsm with TermEv */
			rc = avnd_comp_clc_fsm_trigger(cb, curr_csi->comp, (m_AVND_COMP_IS_FAILED(curr_csi->comp)) ?
						       AVND_COMP_CLC_PRES_FSM_EV_CLEANUP :
						       AVND_COMP_CLC_PRES_FSM_EV_TERM);
			if (NCSCC_RC_SUCCESS != rc)
				goto done;
		} else {
			/* => si assignment done */
			m_AVND_SU_PRES_STATE_SET(su, SA_AMF_PRESENCE_UNINSTANTIATED);
			m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, su, AVND_CKPT_SU_PRES_STATE);

			/* Now check if in the context of shutdown all app SUs 
			 ** and do the needful
			 */
			if ((cb->term_state == AVND_TERM_STATE_SHUTTING_APP_SU) ||
			    (cb->term_state == AVND_TERM_STATE_SHUTTING_NCS_SU)) {
				avnd_check_su_shutdown_done(cb, su->is_ncs);
			}
		}
	}

 done:
	return rc;
}

/****************************************************************************
  Name          : avnd_su_pres_restart_suterm_hdler
 
  Description   : This routine processes the `SU Terminate` event in 
                  `Restarting` state.
 
  Arguments     : cb   - ptr to the AvND control block
                  su   - ptr to the su
                  comp - ptr to the comp
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
uns32 avnd_su_pres_restart_suterm_hdler(AVND_CB *cb, AVND_SU *su, AVND_COMP *comp)
{
	AVND_COMP *curr_comp = 0;
	uns32 rc = NCSCC_RC_SUCCESS;

	/* 
	 * If pi su, pick all the instantiated/instantiating pi comps & 
	 * trigger their FSM with CleanupEv.
	 */
	for (curr_comp = m_AVND_COMP_FROM_SU_DLL_NODE_GET(m_NCS_DBLIST_FIND_FIRST(&su->comp_list));
	     curr_comp; curr_comp = m_AVND_COMP_FROM_SU_DLL_NODE_GET(m_NCS_DBLIST_FIND_NEXT(&curr_comp->su_dll_node))) {

		/* terminate the non-uninstantiated pi comp */
		if ((!m_AVND_COMP_PRES_STATE_IS_UNINSTANTIATED(curr_comp)) &&
		    (m_AVND_COMP_TYPE_IS_PREINSTANTIABLE(curr_comp))) {
			/* mark the comp failed */
			m_AVND_COMP_FAILED_SET(curr_comp);
			m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, curr_comp, AVND_CKPT_COMP_FLAG_CHANGE);

			/* update comp oper state */
			m_AVND_COMP_OPER_STATE_SET(curr_comp, SA_AMF_OPERATIONAL_DISABLED);
			m_AVND_COMP_OPER_STATE_AVD_SYNC(cb, curr_comp, rc);
			if (NCSCC_RC_SUCCESS != rc)
				goto done;
			m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, curr_comp, AVND_CKPT_COMP_OPER_STATE);

			rc = avnd_comp_clc_fsm_run(cb, curr_comp, AVND_COMP_CLC_PRES_FSM_EV_CLEANUP);
			if (NCSCC_RC_SUCCESS != rc)
				goto done;

		}
	}			/* for */

	/* transition to terminating state */
	m_AVND_SU_PRES_STATE_SET(su, SA_AMF_PRESENCE_TERMINATING);
	m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, su, AVND_CKPT_SU_PRES_STATE);
 done:
	return rc;
}

/****************************************************************************
  Name          : avnd_su_pres_restart_compinst_hdler
 
  Description   : This routine processes the `CompInstantiated` event in 
                  `Restarting` state.
 
  Arguments     : cb   - ptr to the AvND control block
                  su   - ptr to the su
                  comp - ptr to the comp
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
uns32 avnd_su_pres_restart_compinst_hdler(AVND_CB *cb, AVND_SU *su, AVND_COMP *comp)
{
	AVND_COMP *curr_comp = 0;
	AVND_COMP_CSI_REC *curr_csi = 0;
	NCS_BOOL all_inst = TRUE;
	uns32 rc = NCSCC_RC_SUCCESS;

	/* 
	 * If pi su, pick the next pi comp & trigger it's FSM with RestartEv.
	 */
	if (m_AVND_SU_IS_PREINSTANTIABLE(su)) {
		for (curr_comp = m_AVND_COMP_FROM_SU_DLL_NODE_GET(m_NCS_DBLIST_FIND_NEXT(&comp->su_dll_node));
		     curr_comp;
		     curr_comp = m_AVND_COMP_FROM_SU_DLL_NODE_GET(m_NCS_DBLIST_FIND_NEXT(&curr_comp->su_dll_node))) {
			/* restart the pi comp */
			if (m_AVND_COMP_TYPE_IS_PREINSTANTIABLE(curr_comp)) {
				rc = avnd_comp_clc_fsm_run(cb, curr_comp, AVND_COMP_CLC_PRES_FSM_EV_RESTART);
				if (NCSCC_RC_SUCCESS != rc)
					goto done;
				break;
			}
		}		/* for */

		/* check whether all comp's are instantiated */
		for (curr_comp = m_AVND_COMP_FROM_SU_DLL_NODE_GET(m_NCS_DBLIST_FIND_FIRST(&su->comp_list));
		     curr_comp;
		     curr_comp = m_AVND_COMP_FROM_SU_DLL_NODE_GET(m_NCS_DBLIST_FIND_NEXT(&curr_comp->su_dll_node))) {
			if ((curr_comp->pres != SA_AMF_PRESENCE_INSTANTIATED) &&
			    (m_AVND_COMP_TYPE_IS_PREINSTANTIABLE(curr_comp)))
				all_inst = FALSE;
		}

		/* OK, all are instantiated */
		if (all_inst == TRUE) {
			m_AVND_SU_PRES_STATE_SET(su, SA_AMF_PRESENCE_INSTANTIATED);
			m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, su, AVND_CKPT_SU_PRES_STATE);
		}
	}

	/* 
	 * If npi su, pick the next csi & trigger it's comp fsm with RestartEv.
	 */
	if (!m_AVND_SU_IS_PREINSTANTIABLE(su)) {
		/* get the only csi rec */
		curr_csi = m_AVND_CSI_REC_FROM_COMP_DLL_NODE_GET(m_NCS_DBLIST_FIND_FIRST(&comp->csi_list));
		assert(curr_csi);

		/* mark the csi state assigned */
		m_AVND_COMP_CSI_CURR_ASSIGN_STATE_SET(curr_csi, AVND_COMP_CSI_ASSIGN_STATE_ASSIGNED);
		m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, curr_csi, AVND_CKPT_COMP_CSI_CURR_ASSIGN_STATE);

		/* get the next csi */
		curr_csi = (AVND_COMP_CSI_REC *)m_NCS_DBLIST_FIND_NEXT(&curr_csi->si_dll_node);
		if (curr_csi) {
			/* we have another csi. trigger the comp fsm with RestartEv */
			rc = avnd_comp_clc_fsm_trigger(cb, curr_csi->comp, AVND_COMP_CLC_PRES_FSM_EV_RESTART);
			if (NCSCC_RC_SUCCESS != rc)
				goto done;
		} else {
			/* => si assignment done */
			m_AVND_SU_PRES_STATE_SET(su, SA_AMF_PRESENCE_INSTANTIATED);
			m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, su, AVND_CKPT_SU_PRES_STATE);
		}
	}

 done:
	return rc;
}

/****************************************************************************
  Name          : avnd_su_pres_restart_compterming_hdler
 
  Description   : This routine processes the `SU Terminate` event in 
                  `Instantiating` state.
 
  Arguments     : cb   - ptr to the AvND control block
                  su   - ptr to the su
                  comp - ptr to the comp
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : Note that this event is only generated for PI SUs. SUTermEv
                  is generated for NPI SUs only during stable states i.e.
                  uninstantiated & instantiated states. This is done by 
                  avoiding overlapping SI assignments.
******************************************************************************/
uns32 avnd_su_pres_restart_compterming_hdler(AVND_CB *cb, AVND_SU *su, AVND_COMP *comp)
{
	AVND_COMP *curr_comp = 0;
	uns32 rc = NCSCC_RC_SUCCESS;

	if (m_AVND_SU_IS_ADMN_TERM(su)) {
		/* This case will be hit, when an SU is already restarting and admin
		 * is trying to terminate it. As a part of admn term in SU restart
		 * we would clean up all the comp in SU. while this cleanup is happening 
		 * for each componet, they will trigger SU FSM and this particular case
		 * will be hit.
		 * we need not do anything here.
		 */
		goto done;
	}

	/* 
	 * If pi su, pick all the instantiated/instantiating pi comps & 
	 * trigger their FSM with TermEv.
	 */
	for (curr_comp = m_AVND_COMP_FROM_SU_DLL_NODE_GET(m_NCS_DBLIST_FIND_FIRST(&su->comp_list));
	     curr_comp; curr_comp = m_AVND_COMP_FROM_SU_DLL_NODE_GET(m_NCS_DBLIST_FIND_NEXT(&curr_comp->su_dll_node))) {

		/* skip the npi comps */
		if (!m_AVND_COMP_TYPE_IS_PREINSTANTIABLE(curr_comp))
			continue;

		/* terminate the non-uninstantiated pi comp */
		if (!m_AVND_COMP_PRES_STATE_IS_UNINSTANTIATED(curr_comp) &&
		    (!m_AVND_COMP_PRES_STATE_IS_TERMINATING(curr_comp))) {
			/* mark the comp failed */
			m_AVND_COMP_FAILED_SET(curr_comp);
			m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, curr_comp, AVND_CKPT_COMP_FLAG_CHANGE);

			/* update comp oper state */
			m_AVND_COMP_OPER_STATE_SET(curr_comp, SA_AMF_OPERATIONAL_DISABLED);
			m_AVND_COMP_OPER_STATE_AVD_SYNC(cb, curr_comp, rc);
			if (NCSCC_RC_SUCCESS != rc)
				goto done;
			m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, curr_comp, AVND_CKPT_COMP_OPER_STATE);

			rc = avnd_comp_clc_fsm_run(cb, curr_comp, AVND_COMP_CLC_PRES_FSM_EV_CLEANUP);
			if (NCSCC_RC_SUCCESS != rc)
				goto done;

		}
	}			/* for */

	/* transition to terminating state */
	m_AVND_SU_PRES_STATE_SET(su, SA_AMF_PRESENCE_TERMINATING);
	m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, su, AVND_CKPT_SU_PRES_STATE);
 done:
	return rc;
}

/****************************************************************************
  Name          : avnd_su_pres_inst_compinstfail_hdler
 
  Description   : This routine processes the `CompInstantiateFailed` event in 
                  `Instantiated` state.
 
  Arguments     : cb   - ptr to the AvND control block
                  su   - ptr to the su
                  comp - ptr to the comp
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : This function is reused for the following su presence 
                  state transitions also
                  INSTANTIATED -> INSTANTIATIONFAIL
                  TERMINATING  -> INSTANTIATIONFAIL
                  RESTARTING   -> INSTANTIATIONFAIL
******************************************************************************/
uns32 avnd_su_pres_inst_compinstfail_hdler(AVND_CB *cb, AVND_SU *su, AVND_COMP *comp)
{
	AVND_COMP *curr_comp = 0;
	AVND_SU_SI_REC *si = 0;
	AVND_COMP_CSI_REC *curr_csi = 0;
	uns32 rc = NCSCC_RC_SUCCESS;
	uns32 comp_count = 0;

	/* transition to inst-failed state */
	m_AVND_SU_PRES_STATE_SET(su, SA_AMF_PRESENCE_INSTANTIATION_FAILED);
	m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, su, AVND_CKPT_SU_PRES_STATE);
	m_AVND_SU_ALL_TERM_RESET(su);
	m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, su, AVND_CKPT_SU_FLAG_CHANGE);

	/* 
	 * If pi su, pick all the instantiated/instantiating pi comps & 
	 * trigger their FSM with TermEv.
	 */
	if (m_AVND_SU_IS_PREINSTANTIABLE(su)) {
		for (curr_comp = m_AVND_COMP_FROM_SU_DLL_NODE_GET(m_NCS_DBLIST_FIND_LAST(&su->comp_list));
		     curr_comp;
		     curr_comp = m_AVND_COMP_FROM_SU_DLL_NODE_GET(m_NCS_DBLIST_FIND_PREV(&curr_comp->su_dll_node))) {

			/* skip the npi comps */
			if (!m_AVND_COMP_TYPE_IS_PREINSTANTIABLE(curr_comp))
				continue;

			comp_count ++;
			/* terminate the non-uninstantiated pi healthy comp && clean the faulty comps */
			if ((!m_AVND_COMP_PRES_STATE_IS_UNINSTANTIATED(curr_comp)) &&
			    (!m_AVND_COMP_IS_FAILED(curr_comp))) {
				/* if this comp was getting assigned, mark it done */
				avnd_comp_cmplete_all_assignment(cb, curr_comp);
				avnd_comp_cmplete_all_csi_rec(cb, curr_comp);
				rc = avnd_comp_clc_fsm_run(cb, curr_comp, AVND_COMP_CLC_PRES_FSM_EV_TERM);
				if (NCSCC_RC_SUCCESS != rc)
					goto done;
			} else if ((!m_AVND_COMP_PRES_STATE_IS_UNINSTANTIATED(curr_comp)) &&
				   (m_AVND_COMP_IS_FAILED(curr_comp))) {
				/* if this comp was getting assigned, mark it done */
				avnd_comp_cmplete_all_assignment(cb, curr_comp);
				avnd_comp_cmplete_all_csi_rec(cb, curr_comp);
				rc = avnd_comp_clc_fsm_run(cb, curr_comp, AVND_COMP_CLC_PRES_FSM_EV_CLEANUP);
				if (NCSCC_RC_SUCCESS != rc)
					goto done;
			}
		}		/* for */
		if (1 == comp_count) {
			/* If the componenet was alone then we need to set SU to term state and process the SUSI 
			   assignment.*/
			m_AVND_SU_ALL_TERM_SET(su);
			m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, su, AVND_CKPT_SU_FLAG_CHANGE);
			avnd_su_siq_prc(cb, su);
		}
	}

	/* 
	 * If npi su, pick all the assigned/assigning comps &
	 * trigger their comp fsm with TermEv
	 */
	if (!m_AVND_SU_IS_PREINSTANTIABLE(su)) {
		/* get the only si rec */
		si = (AVND_SU_SI_REC *)m_NCS_DBLIST_FIND_FIRST(&su->si_list);
		assert(si);

		for (curr_csi = (AVND_COMP_CSI_REC *)m_NCS_DBLIST_FIND_FIRST(&si->csi_list);
		     curr_csi; curr_csi = (AVND_COMP_CSI_REC *)m_NCS_DBLIST_FIND_NEXT(&curr_csi->si_dll_node)) {
			/* terminate the component containing non-unassigned csi */
			if (!m_AVND_COMP_CSI_CURR_ASSIGN_STATE_IS_UNASSIGNED(curr_csi)) {
				rc = avnd_comp_clc_fsm_run(cb, curr_csi->comp, AVND_COMP_CLC_PRES_FSM_EV_TERM);
				if (NCSCC_RC_SUCCESS != rc)
					goto done;
			}
		}		/* for */
	}

 done:
	return rc;
}

/****************************************************************************
  Name          : avnd_su_pres_instfailed_compuninst 
 
  Description   : This routine processes the `CompInstantiateFailed` event &
                  `CompUnInstantiated` event in `Instantiationfailed` state.
 
  Arguments     : cb   - ptr to the AvND control block
                  su   - ptr to the su
                  comp - ptr to the comp
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None. 
******************************************************************************/
uns32 avnd_su_pres_instfailed_compuninst(AVND_CB *cb, AVND_SU *su, AVND_COMP *comp)
{
	AVND_COMP *curr_comp = 0;
	AVND_SU_SIQ_REC *siq = 0;
	uns32 rc = NCSCC_RC_SUCCESS;

	/* check whether all pi comps are terminated  */
	if (m_AVND_SU_IS_PREINSTANTIABLE(su)) {
		for (curr_comp = m_AVND_COMP_FROM_SU_DLL_NODE_GET(m_NCS_DBLIST_FIND_FIRST(&su->comp_list));
		     curr_comp;
		     curr_comp = m_AVND_COMP_FROM_SU_DLL_NODE_GET(m_NCS_DBLIST_FIND_NEXT(&curr_comp->su_dll_node))) {

			/* skip the npi comps */
			if (!m_AVND_COMP_TYPE_IS_PREINSTANTIABLE(curr_comp))
				continue;

			if (!m_AVND_COMP_PRES_STATE_IS_UNINSTANTIATED(curr_comp) &&
			    !m_AVND_COMP_PRES_STATE_IS_INSTANTIATIONFAILED(curr_comp)) {
				m_AVND_SU_ALL_TERM_RESET(su);
				m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, su, AVND_CKPT_SU_FLAG_CHANGE);

				if (m_AVND_COMP_PRES_STATE_IS_TERMINATIONFAILED(curr_comp)) {
					/* why waste memory -free entire queue */
					siq = (AVND_SU_SIQ_REC *)m_NCS_DBLIST_FIND_LAST(&su->siq);
					while (siq) {
						/* unlink the buffered msg from the queue */
						ncs_db_link_list_delink(&su->siq, &siq->su_dll_node);

						if (TRUE == su->su_is_external) {
							m_AVND_SEND_CKPT_UPDT_ASYNC_RMV(cb, siq, AVND_CKPT_SIQ_REC);
						}

						/* delete the buffered msg */
						avnd_su_siq_rec_del(cb, su, siq);

						siq = (AVND_SU_SIQ_REC *)m_NCS_DBLIST_FIND_LAST(&su->siq);
					}
				}
				return rc;
			}

		}		/* for */

		m_AVND_SU_ALL_TERM_SET(su);
		m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, su, AVND_CKPT_SU_FLAG_CHANGE);
		avnd_su_siq_prc(cb, su);
	}

	return rc;
}
