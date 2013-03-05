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
static uint32_t avnd_su_pres_uninst_suinst_hdler(AVND_CB *, AVND_SU *, AVND_COMP *);
static uint32_t avnd_su_pres_insting_suterm_hdler(AVND_CB *, AVND_SU *, AVND_COMP *);
static uint32_t avnd_su_pres_insting_surestart_hdler(AVND_CB *, AVND_SU *, AVND_COMP *);
static uint32_t avnd_su_pres_insting_compinst_hdler(AVND_CB *, AVND_SU *, AVND_COMP *);
static uint32_t avnd_su_pres_insting_compinstfail_hdler(AVND_CB *, AVND_SU *, AVND_COMP *);
static uint32_t avnd_su_pres_inst_suterm_hdler(AVND_CB *, AVND_SU *, AVND_COMP *);
static uint32_t avnd_su_pres_inst_surestart_hdler(AVND_CB *, AVND_SU *, AVND_COMP *);
static uint32_t avnd_su_pres_inst_comprestart_hdler(AVND_CB *, AVND_SU *, AVND_COMP *);
static uint32_t avnd_su_pres_inst_compterming_hdler(AVND_CB *, AVND_SU *, AVND_COMP *);
static uint32_t avnd_su_pres_terming_compinst_hdler(AVND_CB *, AVND_SU *, AVND_COMP *);
static uint32_t avnd_su_pres_terming_comptermfail_hdler(AVND_CB *, AVND_SU *, AVND_COMP *);
static uint32_t avnd_su_pres_terming_compuninst_hdler(AVND_CB *, AVND_SU *, AVND_COMP *);
static uint32_t avnd_su_pres_restart_suterm_hdler(AVND_CB *, AVND_SU *, AVND_COMP *);
static uint32_t avnd_su_pres_restart_compinst_hdler(AVND_CB *, AVND_SU *, AVND_COMP *);
static uint32_t avnd_su_pres_restart_compterming_hdler(AVND_CB *, AVND_SU *, AVND_COMP *);
static uint32_t avnd_su_pres_inst_compinstfail_hdler(AVND_CB *, AVND_SU *, AVND_COMP *);
static uint32_t avnd_su_pres_instfailed_compuninst(AVND_CB *, AVND_SU *, AVND_COMP *);

static uint32_t avnd_su_pres_st_chng_prc(AVND_CB *, AVND_SU *, SaAmfPresenceStateT, SaAmfPresenceStateT);

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
	bool is_tob_buf = false;
	uint32_t rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER2("'%s'", su->name.value);

	/* buffer the msg, if SU is inst-failed and all comps are not terminated */
	if (((su->pres == SA_AMF_PRESENCE_INSTANTIATION_FAILED) && (!m_AVND_SU_IS_ALL_TERM(su))) ||
	    m_AVND_IS_SHUTTING_DOWN(cb) || (m_AVND_SU_IS_RESTART(su))) {
		siq = avnd_su_siq_rec_add(cb, su, param, &rc);
		TRACE_LEAVE();
		return siq;
	}

	/* dont buffer quiesced assignments during failure */
	if (m_AVND_SU_IS_FAILED(su) &&
	    ((SA_AMF_HA_QUIESCED == param->ha_state) || (AVSV_SUSI_ACT_DEL == param->msg_act))) {
		TRACE_LEAVE();
		return 0;
	}

	/*
	 * Determine if msg is to be buffered.
	 */
	if (!m_AVSV_SA_NAME_IS_NULL(param->si_name)) {	/* => msg is for a single si */
		/* determine if an (non-quiescing) assign / remove operation is on */
		si_rec = avnd_su_si_rec_get(cb, &su->name, &param->si_name);
		if (si_rec && ((m_AVND_SU_SI_CURR_ASSIGN_STATE_IS_ASSIGNING(si_rec) &&
				(SA_AMF_HA_QUIESCING != si_rec->curr_state)) ||
			       (m_AVND_SU_SI_CURR_ASSIGN_STATE_IS_REMOVING(si_rec))))
			is_tob_buf = true;
	} else {		/* => msg is for all sis */
		/* determine if an (non-quiescing) assign / remove operation is on */
		for (si_rec = (AVND_SU_SI_REC *)m_NCS_DBLIST_FIND_FIRST(&su->si_list);
		     si_rec && !((m_AVND_SU_SI_CURR_ASSIGN_STATE_IS_ASSIGNING(si_rec) &&
				  (SA_AMF_HA_QUIESCING != si_rec->curr_state)) ||
				 (m_AVND_SU_SI_CURR_ASSIGN_STATE_IS_REMOVING(si_rec)));
		     si_rec = (AVND_SU_SI_REC *)m_NCS_DBLIST_FIND_NEXT(&si_rec->su_dll_node)) ;
		if (si_rec)
			is_tob_buf = true;
	}

	/*
	 * Buffer the msg (if reqd).
	 */
	if (true == is_tob_buf)
		siq = avnd_su_siq_rec_add(cb, su, param, &rc);

	TRACE_LEAVE();
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
uint32_t avnd_su_siq_prc(AVND_CB *cb, AVND_SU *su)
{
	AVND_SU_SIQ_REC *siq = 0;
	AVND_SU_SI_REC *si = 0;
	uint32_t rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER2("SU '%s'", su->name.value);

	/* get the least-recent buffered msg, if any */
	siq = (AVND_SU_SIQ_REC *)m_NCS_DBLIST_FIND_LAST(&su->siq);
	if (!siq) {
		TRACE_LEAVE();
		return rc;
	}

	/* determine if an (non-quiescing) assign / remove operation is on */
	for (si = (AVND_SU_SI_REC *)m_NCS_DBLIST_FIND_FIRST(&su->si_list);
	     si && !((m_AVND_SU_SI_CURR_ASSIGN_STATE_IS_ASSIGNING(si) &&
		      (SA_AMF_HA_QUIESCING != si->curr_state)) ||
		     (m_AVND_SU_SI_CURR_ASSIGN_STATE_IS_REMOVING(si)));
	     si = (AVND_SU_SI_REC *)m_NCS_DBLIST_FIND_NEXT(&si->su_dll_node)) ;
	if (si) {
		TRACE_LEAVE();
		return rc;
	}

	/* unlink the buffered msg from the queue */
	ncs_db_link_list_delink(&su->siq, &siq->su_dll_node);

	/* initiate si asignment / removal */
	rc = avnd_su_si_msg_prc(cb, su, &siq->info);

	if (true == su->su_is_external) {
		m_AVND_SEND_CKPT_UPDT_ASYNC_RMV(cb, siq, AVND_CKPT_SIQ_REC);
	}

	/* delete the buffered msg */
	avnd_su_siq_rec_del(cb, su, siq);

	TRACE_LEAVE2("%u", rc);
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
uint32_t avnd_su_si_msg_prc(AVND_CB *cb, AVND_SU *su, AVND_SU_SI_PARAM *info)
{
	AVND_SU_SI_REC *si = 0;
	uint32_t rc = NCSCC_RC_SUCCESS;
	AVND_COMP_CSI_REC *csi = NULL;

	TRACE_ENTER2("'%s', act=%u, ha_state=%u, single_csi=%u",
				 su->name.value, info->msg_act, info->ha_state, info->single_csi);

	/* we have started the su si msg processing */
	m_AVND_SU_ASSIGN_PEND_SET(su);
	m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, su, AVND_CKPT_SU_FLAG_CHANGE);

	/* If the request targets all SIs, set flag once early for all cases */
	if (avsv_sa_name_is_null(&info->si_name))
		m_AVND_SU_ALL_SI_SET(su);

	switch (info->msg_act) {
	case AVSV_SUSI_ACT_ASGN:	/* new assign */
		{
			if (false == info->single_csi) {
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
			} else {
				AVND_COMP_CSI_PARAM *csi_param;
				/* verify if su-si relationship already exists */
				if (0 == (si = avnd_su_si_rec_get(cb, &info->su_name, &info->si_name))) {
					LOG_ER("No SUSI Rec exists"); 
					goto done;
				}

				osafassert((info->num_assigns == 1));
				csi_param = info->list;
				osafassert(csi_param);
				osafassert(!(csi_param->next));
				avnd_su_si_csi_rec_add(cb, su, si, csi_param, &rc);
				m_AVND_SEND_CKPT_UPDT_ASYNC_ADD(cb, si_rec, AVND_CKPT_CSI_REC);
				si->single_csi_add_rem_in_si =  AVSV_SUSI_ACT_ASGN;
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
			if (true == info->single_csi) {
				AVND_COMP_CSI_PARAM *csi_param;
				AVND_COMP_CSI_REC *csi_rec;
				si->single_csi_add_rem_in_si =  AVSV_SUSI_ACT_DEL;
				osafassert((info->num_assigns == 1));
				csi_param = info->list;
				osafassert(csi_param);
				osafassert(!(csi_param->next));
				if (NULL ==  (csi_rec = avnd_compdb_csi_rec_get(cb, &csi_param->comp_name, &csi_param->csi_name))) {
					LOG_ER("No CSI Rec exists for comp='%s'and csi=%s",csi_param->comp_name.value, 
							csi_param->csi_name.value); 
					goto done;
				}
				csi_rec->single_csi_add_rem_in_si = AVSV_SUSI_ACT_DEL;
			}
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
		osafassert(0);
	}			/* switch */

done:
	TRACE_LEAVE2("%u", rc);
	return rc;
}
/**
 * @brief	Checks if all csi of same SI assigned to the component are in assigning state 
 *
 * @param [in]	cmp
 *		si
 *
 * @returns     true/false  
 **/
static bool csi_of_same_si_in_assigning_state(const AVND_COMP *cmp, const AVND_SU_SI_REC *si)
{
	AVND_COMP_CSI_REC *curr_csi;
	bool all_csi_assigned = true;

	for (curr_csi = m_AVND_CSI_REC_FROM_COMP_DLL_NODE_GET(m_NCS_DBLIST_FIND_FIRST(&cmp->csi_list));
		curr_csi;
		curr_csi = m_AVND_CSI_REC_FROM_COMP_DLL_NODE_GET(m_NCS_DBLIST_FIND_NEXT(&curr_csi->comp_dll_node))) {
		if ((curr_csi->si == si) && !m_AVND_COMP_CSI_CURR_ASSIGN_STATE_IS_ASSIGNING(curr_csi)) {
			all_csi_assigned = false;
			break;
		}
	}

	return all_csi_assigned;
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
static uint32_t assign_si_to_su(AVND_SU_SI_REC *si, AVND_SU *su, int single_csi)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	AVND_COMP_CSI_REC *curr_csi;

	TRACE_ENTER2("'%s' si-state'%d' '%s' single_csi=%u",
				 si->name.value, si->curr_state, su->name.value, single_csi);

	LOG_NO("Assigning '%s' %s to '%s'", si->name.value, ha_state[si->curr_state], su->name.value);

	/* initiate the si assignment for pi su */
	if (m_AVND_SU_IS_PREINSTANTIABLE(su)) {
		uint32_t rank;

		if (SA_AMF_HA_ACTIVE == si->curr_state) {
			if (single_csi) {
				for (curr_csi = (AVND_COMP_CSI_REC *)m_NCS_DBLIST_FIND_FIRST(&si->csi_list),
					rank = curr_csi->rank;
					(curr_csi != NULL) && ((curr_csi->rank == rank) || (curr_csi->si->single_csi_add_rem_in_si == AVSV_SUSI_ACT_ASGN));
					curr_csi = (AVND_COMP_CSI_REC *)m_NCS_DBLIST_FIND_NEXT(&curr_csi->si_dll_node)) {
					curr_csi->comp->assigned_flag = false;
				}
			}
			for (curr_csi = (AVND_COMP_CSI_REC *)m_NCS_DBLIST_FIND_FIRST(&si->csi_list),
				 rank = curr_csi->rank;
				  (curr_csi != NULL) && ((curr_csi->rank == rank) || (curr_csi->si->single_csi_add_rem_in_si == AVSV_SUSI_ACT_ASGN));
				  curr_csi = (AVND_COMP_CSI_REC *)m_NCS_DBLIST_FIND_NEXT(&curr_csi->si_dll_node)) {

				if (AVND_COMP_CSI_ASSIGN_STATE_ASSIGNED != curr_csi->curr_assign_state) {
					if (false == curr_csi->comp->assigned_flag) {
						/* Dont assign, if already assignd */
						if (AVND_SU_SI_ASSIGN_STATE_ASSIGNED != curr_csi->si->curr_assign_state) {
							rc = avnd_comp_csi_assign(avnd_cb, curr_csi->comp, (single_csi) ? curr_csi : NULL);
							if (NCSCC_RC_SUCCESS != rc)
								goto done;
						}
						if ((false == curr_csi->comp->assigned_flag) && (m_AVND_COMP_CSI_PRV_ASSIGN_STATE_IS_ASSIGNED(curr_csi))) {
							if (csi_of_same_si_in_assigning_state(curr_csi->comp, si))
								curr_csi->comp->assigned_flag = true;
						}
					} else {
						m_AVND_COMP_CSI_CURR_ASSIGN_STATE_SET(curr_csi, AVND_COMP_CSI_ASSIGN_STATE_ASSIGNED);
						m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, curr_csi, AVND_CKPT_COMP_CSI_CURR_ASSIGN_STATE);
					}
				}
			}
		} else {
			if (single_csi) {
				for (curr_csi = (AVND_COMP_CSI_REC *)m_NCS_DBLIST_FIND_LAST(&si->csi_list),
					rank = curr_csi->rank;
					(curr_csi != NULL) && ((curr_csi->rank == rank) || (curr_csi->si->single_csi_add_rem_in_si == AVSV_SUSI_ACT_ASGN));
					curr_csi = (AVND_COMP_CSI_REC *)m_NCS_DBLIST_FIND_PREV(&curr_csi->si_dll_node)) {
					curr_csi->comp->assigned_flag = false;
				}
			}
			for (curr_csi = (AVND_COMP_CSI_REC *)m_NCS_DBLIST_FIND_LAST(&si->csi_list),
				 rank = curr_csi->rank;
				  (curr_csi != NULL) && ((curr_csi->rank == rank) || (curr_csi->si->single_csi_add_rem_in_si == AVSV_SUSI_ACT_ASGN));
				  curr_csi = (AVND_COMP_CSI_REC *)m_NCS_DBLIST_FIND_PREV(&curr_csi->si_dll_node)) {

				/* We need to send only one csi set for a comp having  more than one CSI assignment for
				   csi mod/rem.*/
				if (AVND_COMP_CSI_ASSIGN_STATE_ASSIGNED != curr_csi->curr_assign_state) {
					if (false == curr_csi->comp->assigned_flag) {
						/* Dont assign, if already assignd */
						if (AVND_SU_SI_ASSIGN_STATE_ASSIGNED != curr_csi->si->curr_assign_state) {
							rc = avnd_comp_csi_assign(avnd_cb, curr_csi->comp, (single_csi) ? curr_csi : NULL);
							if (NCSCC_RC_SUCCESS != rc)
								goto done;
						}
					/*if ((!single_csi) && (false == curr_csi->comp->assigned_flag) && 
								(m_AVND_COMP_CSI_PRV_ASSIGN_STATE_IS_ASSIGNED(curr_csi)))*/
						if ((false == curr_csi->comp->assigned_flag) && 
							(m_AVND_COMP_CSI_PRV_ASSIGN_STATE_IS_ASSIGNED(curr_csi))) {
							if (csi_of_same_si_in_assigning_state(curr_csi->comp, si))
								curr_csi->comp->assigned_flag = true;
						}
					} else {
						m_AVND_COMP_CSI_CURR_ASSIGN_STATE_SET(curr_csi, AVND_COMP_CSI_ASSIGN_STATE_ASSIGNED);
						m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, curr_csi, AVND_CKPT_COMP_CSI_CURR_ASSIGN_STATE);
					}
				}
			}
		}
	}

	/* initiate the si assignment for npi su */
	if (!m_AVND_SU_IS_PREINSTANTIABLE(su)) {
		TRACE("SU is NPI");
		bool npi_prv_inst = true, npi_curr_inst = true;
		AVND_SU_PRES_FSM_EV su_ev = AVND_SU_PRES_FSM_EV_MAX;

		/* determine the instantiation state of npi su */
		if (SA_AMF_HA_ACTIVE != si->prv_state)
			npi_prv_inst = false;
		if (SA_AMF_HA_ACTIVE != si->curr_state)
			npi_curr_inst = false;

		/* Quiesced while Quiescing */
		if (m_AVND_SU_SI_PRV_ASSIGN_STATE_IS_ASSIGNING(si) && (SA_AMF_HA_QUIESCING == si->prv_state))
			npi_prv_inst = true;

		/* determine the event for the su fsm */
		if (m_AVND_SU_IS_RESTART(su) && (true == npi_curr_inst))
			su_ev = AVND_SU_PRES_FSM_EV_RESTART;
		else if (!m_AVND_SU_IS_RESTART(su) && (npi_prv_inst != npi_curr_inst))
			su_ev = (true == npi_curr_inst) ? AVND_SU_PRES_FSM_EV_INST : AVND_SU_PRES_FSM_EV_TERM;

		/* we cant do anything on inst-failed SU or term-failed SU, so just resp success for quiesced */
		if (su->pres == SA_AMF_PRESENCE_INSTANTIATION_FAILED || su->pres == SA_AMF_PRESENCE_TERMINATION_FAILED)
			su_ev = AVND_SU_PRES_FSM_EV_MAX;

		/* trigger the su fsm */
		if (AVND_SU_PRES_FSM_EV_MAX != su_ev) {
			rc = avnd_su_pres_fsm_run(avnd_cb, su, 0, su_ev);
		} else
			rc = avnd_su_si_oper_done(avnd_cb, su, ((single_csi) ? si:NULL));
		if (NCSCC_RC_SUCCESS != rc)
			goto done;
	}

done:
	TRACE_LEAVE2("%u", rc);
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
uint32_t avnd_su_si_assign(AVND_CB *cb, AVND_SU *su, AVND_SU_SI_REC *si)
{
	uint32_t rc = NCSCC_RC_SUCCESS, rank;
	AVND_SU_SI_REC *curr_si;
	AVND_COMP_CSI_REC *curr_csi;

	TRACE_ENTER2("'%s' '%s'", su->name.value, si ? si->name.value : NULL);

	/* mark the si(s) assigning and assign to su */
	if (si) {
		m_AVND_SU_SI_CURR_ASSIGN_STATE_SET(si, AVND_SU_SI_ASSIGN_STATE_ASSIGNING);
		m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, si, AVND_CKPT_SU_SI_REC_CURR_ASSIGN_STATE);
		rc = assign_si_to_su(si, su, true);
	} else {
		for (curr_si = (AVND_SU_SI_REC *)m_NCS_DBLIST_FIND_FIRST(&su->si_list);
		     curr_si != NULL;
			 curr_si = (AVND_SU_SI_REC *)m_NCS_DBLIST_FIND_NEXT(&curr_si->su_dll_node)) {
			if (SA_AMF_HA_ACTIVE == curr_si->curr_state) {
				for (curr_csi = (AVND_COMP_CSI_REC *)m_NCS_DBLIST_FIND_FIRST(&curr_si->csi_list),
					rank = curr_csi->rank;
					(curr_csi != NULL) && (curr_csi->rank == rank);
					curr_csi = (AVND_COMP_CSI_REC *)m_NCS_DBLIST_FIND_NEXT(&curr_csi->si_dll_node)) {
					curr_csi->comp->assigned_flag = false;
				}
			}
			else { 
				for (curr_csi = (AVND_COMP_CSI_REC *)m_NCS_DBLIST_FIND_LAST(&curr_si->csi_list),
					rank = curr_csi->rank;
					(curr_csi != NULL) && (curr_csi->rank == rank);
					curr_csi = (AVND_COMP_CSI_REC *)m_NCS_DBLIST_FIND_PREV(&curr_csi->si_dll_node)) {
					curr_csi->comp->assigned_flag = false;
				}
			}
			m_AVND_SU_SI_CURR_ASSIGN_STATE_SET(curr_si, AVND_SU_SI_ASSIGN_STATE_ASSIGNING);
			m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, curr_si, AVND_CKPT_SU_SI_REC_CURR_ASSIGN_STATE);
		}
		/* if no si is specified, the action is aimed at all the sis... loop */
		for (curr_si = (AVND_SU_SI_REC *)m_NCS_DBLIST_FIND_FIRST(&su->si_list);
		     curr_si != NULL;
			 curr_si = (AVND_SU_SI_REC *)m_NCS_DBLIST_FIND_NEXT(&curr_si->su_dll_node)) {

			rc = assign_si_to_su(curr_si, su, false);
			if (NCSCC_RC_SUCCESS != rc)
				goto done;
		}
	}

done:
	TRACE_LEAVE2("%u", rc);
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
uint32_t avnd_su_si_remove(AVND_CB *cb, AVND_SU *su, AVND_SU_SI_REC *si)
{
	AVND_COMP_CSI_REC *curr_csi = 0;
	AVND_SU_SI_REC *curr_si = 0;
	uint32_t rc = NCSCC_RC_SUCCESS;
	char *siname = si ? (char*) si->name.value : "all SIs";

	TRACE_ENTER2("'%s' '%s'", su->name.value, si ? si->name.value : NULL);
	LOG_NO("Removing '%s' from '%s'", siname, su->name.value);

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
		if (si) {
			if (AVSV_SUSI_ACT_DEL == si->single_csi_add_rem_in_si) {
				AVND_COMP_CSI_REC *next_csi;

				/* We have to remove single csi. */
				for (curr_csi = (AVND_COMP_CSI_REC *)m_NCS_DBLIST_FIND_FIRST(&si->csi_list); curr_csi;) {
					next_csi = (AVND_COMP_CSI_REC *)m_NCS_DBLIST_FIND_NEXT(&curr_csi->si_dll_node);
					if (AVSV_SUSI_ACT_DEL == curr_csi->single_csi_add_rem_in_si) {
						rc = avnd_comp_csi_remove(cb, curr_csi->comp, curr_csi);
						if (NCSCC_RC_SUCCESS != rc)
							goto done;
					}
					curr_csi = next_csi;
				}
			} else {
				/* pick up the last csi */
                        	curr_csi = (AVND_COMP_CSI_REC *)m_NCS_DBLIST_FIND_LAST(&curr_si->csi_list);

                        	/* remove the csi */
                        	if (curr_csi) {
                        	        rc = avnd_comp_csi_remove(cb, curr_csi->comp, (si) ? curr_csi : 0);
                        	        if (NCSCC_RC_SUCCESS != rc)
                        	                goto done;
                        	}
			}
		} else {
			AVND_SU_SI_REC *next_si;

			/* removal for all SIs, do all in paralell */
			for (curr_si = (AVND_SU_SI_REC *)m_NCS_DBLIST_FIND_FIRST(&su->si_list); curr_si;) {

				/* Save next ptr here since it might get deleted deep down in avnd_comp_csi_remove */
				next_si = (AVND_SU_SI_REC *)m_NCS_DBLIST_FIND_NEXT(&curr_si->su_dll_node);

				/* pick up the last csi */
				curr_csi = (AVND_COMP_CSI_REC *)m_NCS_DBLIST_FIND_LAST(&curr_si->csi_list);

				/* remove the csi */
				if (curr_csi) {
					rc = avnd_comp_csi_remove(cb, curr_csi->comp, NULL);
					if (NCSCC_RC_SUCCESS != rc || !su->si_list.n_nodes) 
						goto done;
				}
					curr_si = next_si;
			}
		}
	}

	/* initiate the si removal for npi su */
	if (!m_AVND_SU_IS_PREINSTANTIABLE(su)) {
		/* nothing to be done, termination already done in
		   quiescing/quiesced state */
		if (su->pres == SA_AMF_PRESENCE_INSTANTIATED) {
			rc = avnd_su_pres_fsm_run(cb, su, 0, AVND_SU_PRES_FSM_EV_TERM);
			if (NCSCC_RC_SUCCESS != rc)
				goto done;
		} else {
			rc = avnd_su_si_oper_done(cb, su, si);
			if (NCSCC_RC_SUCCESS != rc)
				goto done;
		}
	}

 done:
	TRACE_LEAVE2("%u", rc);
	return rc;
}
/**
 * @brief	Checks if any susi operation(assigning or removing) is in progress    
 *
 * @param[in]   su
 *		si 
 *
 * @return      true/false 
 **/
static bool susi_operation_in_progress(AVND_SU *su, AVND_SU_SI_REC *si)
{
	AVND_SU_SI_REC *curr_si, *t_si;
	AVND_COMP_CSI_REC *curr_csi, *t_csi;
	bool opr_done = true;

	for (curr_si = (si) ? si : (AVND_SU_SI_REC *)m_NCS_DBLIST_FIND_FIRST(&su->si_list);
			curr_si && opr_done; curr_si = (si) ? 0 : (AVND_SU_SI_REC *)m_NCS_DBLIST_FIND_NEXT(&curr_si->su_dll_node)) {
		for (curr_csi = (AVND_COMP_CSI_REC *)m_NCS_DBLIST_FIND_FIRST(&curr_si->csi_list);
				curr_csi; curr_csi = (AVND_COMP_CSI_REC *)m_NCS_DBLIST_FIND_NEXT(&curr_csi->si_dll_node)) {
			if (m_AVND_COMP_CSI_CURR_ASSIGN_STATE_IS_ASSIGNING(curr_csi) || m_AVND_COMP_CSI_CURR_ASSIGN_STATE_IS_REMOVING(curr_csi)) {
				opr_done = false;       
				break;
			} else if (m_AVND_COMP_IS_FAILED(curr_csi->comp)){
				for (t_si = (si) ? si : (AVND_SU_SI_REC *)m_NCS_DBLIST_FIND_FIRST(&su->si_list);
						t_si; t_si = (si) ? 0 : (AVND_SU_SI_REC *)m_NCS_DBLIST_FIND_NEXT(&t_si->su_dll_node)) {
					for (t_csi = (AVND_COMP_CSI_REC *)m_NCS_DBLIST_FIND_FIRST(&t_si->csi_list);
							t_csi; t_csi = (AVND_COMP_CSI_REC *)m_NCS_DBLIST_FIND_NEXT(&t_csi->si_dll_node)) {
						if (m_AVND_COMP_IS_FAILED(t_csi->comp)
								|| (su->pres == SA_AMF_PRESENCE_INSTANTIATION_FAILED)
								|| (su->pres == SA_AMF_PRESENCE_TERMINATION_FAILED))
							continue;
						else if (m_AVND_COMP_CSI_CURR_ASSIGN_STATE_IS_ASSIGNING(t_csi) ||
								m_AVND_COMP_CSI_CURR_ASSIGN_STATE_IS_REMOVING(t_csi) ||
								m_AVND_COMP_CSI_CURR_ASSIGN_STATE_IS_UNASSIGNED(t_csi)) {
							opr_done = false;
							break;
						}
					}
				}
			}
		} 
	}

	return opr_done;
}

/**
 * Get a higher ranked SI (if any)
 * @param si
 * 
 * @return AVND_SU_SI_REC*
 */
static AVND_SU_SI_REC *get_higher_ranked_si(const AVND_SU_SI_REC *si)
{
	AVND_SU_SI_REC *tmp = avnd_silist_getprev(si);

	if (tmp) {
		for (; tmp && (tmp->rank == si->rank); tmp = avnd_silist_getprev(tmp))
			;
	}

	return tmp;
}

/**
 * Are all SIs of the same rank as the passed SI removed/unassigned?
 * 
 * @param si
 * 
 * @return bool
 */
static bool all_sis_atrank_removed(const AVND_SU_SI_REC *si)
{
	const AVND_SU_SI_REC *tmp;

	/* search forwards */
	for (tmp = si; tmp && (tmp->rank == si->rank); tmp = avnd_silist_getnext(tmp)) {
		if (!m_AVND_SU_SI_CURR_ASSIGN_STATE_IS_REMOVED(tmp)) {
			TRACE("%s not removed", tmp->name.value);
			return false;
		}
	}

	/* search backwards */
	for (tmp = si; tmp && (tmp->rank == si->rank); tmp = avnd_silist_getprev(tmp)) {
		if (!m_AVND_SU_SI_CURR_ASSIGN_STATE_IS_REMOVED(tmp)) {
			TRACE("%s not removed", tmp->name.value);
			return false;
		}
	}

	TRACE("all SIs at rank %u removed", si->rank);
	return true;
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
uint32_t avnd_su_si_oper_done(AVND_CB *cb, AVND_SU *su, AVND_SU_SI_REC *si)
{
	AVND_SU_SI_REC *curr_si = 0;
	AVND_COMP_CSI_REC *curr_csi = 0, *t_csi = 0;
	bool are_si_assigned;
	uint32_t rc = NCSCC_RC_SUCCESS;
	bool opr_done;

	TRACE_ENTER2("'%s' '%s'", su->name.value, si ? si->name.value : NULL);

	/* mark the individual sis after operation is completed for all the SI's */
	opr_done = susi_operation_in_progress(su, si);

	for (curr_si = (si) ? si : (AVND_SU_SI_REC *)m_NCS_DBLIST_FIND_FIRST(&su->si_list);
	     curr_si; curr_si = (si) ? 0 : (AVND_SU_SI_REC *)m_NCS_DBLIST_FIND_NEXT(&curr_si->su_dll_node)) {
		/* mark the si assigned / removed */
		TRACE("SI '%s'", curr_si->name.value);
		
		if (opr_done) {
			if (m_AVND_SU_SI_CURR_ASSIGN_STATE_IS_ASSIGNING(curr_si)) {
				m_AVND_SU_SI_CURR_ASSIGN_STATE_SET(curr_si, AVND_SU_SI_ASSIGN_STATE_ASSIGNED);
				m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, curr_si, AVND_CKPT_SU_SI_REC_CURR_ASSIGN_STATE);
				LOG_NO("Assigned '%s' %s to '%s'", curr_si->name.value,
					ha_state[curr_si->curr_state], su->name.value);
			} else if (m_AVND_SU_SI_CURR_ASSIGN_STATE_IS_REMOVING(curr_si)) {
				m_AVND_SU_SI_CURR_ASSIGN_STATE_SET(curr_si, AVND_SU_SI_ASSIGN_STATE_REMOVED);
				m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, curr_si, AVND_CKPT_SU_SI_REC_CURR_ASSIGN_STATE);
				LOG_NO("Removed '%s' from '%s'", curr_si->name.value, su->name.value);
			} else {
				LOG_CR("current si name ='%s'",curr_si->name.value);
				LOG_CR("SI: curr_assign_state = %u, prv_assign_state = %u, curr_state = %u, prv_state = %u",\
					curr_si->curr_assign_state,curr_si->prv_assign_state,curr_si->curr_state,curr_si->prv_state);
				osafassert(0);
			}
		}
		/*
		 * It is possible that the su fsm for npi su is never triggered (this
		 * happens when the si-state change does not lead to su instantition or
		 * termination). Mark the corresponding csis assigned/removed.
		 */
		if (!m_AVND_SU_IS_PREINSTANTIABLE(su)) {
			TRACE("SU is NPI");
			for (curr_csi = (AVND_COMP_CSI_REC *)m_NCS_DBLIST_FIND_FIRST(&curr_si->csi_list);
			     curr_csi; curr_csi = (AVND_COMP_CSI_REC *)m_NCS_DBLIST_FIND_NEXT(&curr_csi->si_dll_node)) {
				TRACE("CSI '%s'", curr_csi->name.value);
				if (m_AVND_SU_SI_CURR_ASSIGN_STATE_IS_ASSIGNED(curr_si)) {
					TRACE("Setting CSI '%s' assigned", curr_csi->name.value);
					m_AVND_COMP_CSI_CURR_ASSIGN_STATE_SET(curr_csi,
									      AVND_COMP_CSI_ASSIGN_STATE_ASSIGNED);
				}
				else if (m_AVND_SU_SI_CURR_ASSIGN_STATE_IS_REMOVED(curr_si))
					m_AVND_COMP_CSI_CURR_ASSIGN_STATE_SET(curr_csi,
									      AVND_COMP_CSI_ASSIGN_STATE_REMOVED);
				m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, curr_csi, AVND_CKPT_COMP_CSI_CURR_ASSIGN_STATE);
			}
		}
	} /* for */

	/* inform AvD */
	if (opr_done && !m_AVND_SU_IS_RESTART(su)) {
		rc = avnd_di_susi_resp_send(cb, su, m_AVND_SU_IS_ALL_SI(su) ? NULL : si);
		if (NCSCC_RC_SUCCESS != rc)
			goto done;
	}

	if (si && (cb->term_state == AVND_TERM_STATE_OPENSAF_SHUTDOWN_INITIATED)) {
		(void) avnd_evt_last_step_term_evh(cb, NULL);
	} else if (si && (cb->term_state == AVND_TERM_STATE_OPENSAF_SHUTDOWN_STARTED) &&
		   m_AVND_SU_SI_CURR_ASSIGN_STATE_IS_REMOVED(si)) {

		if (all_sis_atrank_removed(si)) {
			AVND_SU_SI_REC *tmp = get_higher_ranked_si(si);

			if (tmp != NULL) {
				uint32_t sirank = tmp->rank;

				for (; tmp && (tmp->rank == sirank); tmp = avnd_silist_getprev(tmp)) {
					uint32_t rc = avnd_su_si_remove(cb, tmp->su, tmp);
					osafassert(rc == NCSCC_RC_SUCCESS);
				}
			} else {
				LOG_NO("Removed assignments from AMF components");
				avnd_last_step_clean(cb);
			}
		}
	}

	/* Now correct si state if single csi was being removed. For the rest of csi assigned, si should be in 
	   Assigned state*/
	if ((si) && (AVSV_SUSI_ACT_DEL == si->single_csi_add_rem_in_si) && 
			(m_AVND_SU_SI_CURR_ASSIGN_STATE_IS_REMOVED(si))) {
		m_AVND_SU_SI_CURR_ASSIGN_STATE_SET(si, AVND_SU_SI_ASSIGN_STATE_ASSIGNED);
		/* Also, we need to remove links of si, csi and comp. */
		/* We have to remove single csi. */
		for (t_csi = (AVND_COMP_CSI_REC *)m_NCS_DBLIST_FIND_FIRST(&si->csi_list);
				t_csi; t_csi = (AVND_COMP_CSI_REC *)m_NCS_DBLIST_FIND_NEXT(&t_csi->si_dll_node)) {
			if (AVSV_SUSI_ACT_DEL == t_csi->single_csi_add_rem_in_si) {
				break;
			}
		}
		osafassert(t_csi);
		/* free the csi attributes */
		if (t_csi->attrs.list)
			free(t_csi->attrs.list);

		m_AVND_SU_SI_CSI_REC_REM(*si, *t_csi);
		m_AVND_COMPDB_REC_CSI_REM(*(t_csi->comp), *t_csi);
		free(t_csi);
	}
	/* Reset the single add/del */
	if (si)
		si->single_csi_add_rem_in_si = AVSV_SUSI_ACT_BASE;

	/* finally delete the si(s) if they are removed */
	curr_si = (si) ? si : (AVND_SU_SI_REC *)m_NCS_DBLIST_FIND_FIRST(&su->si_list);
	if (m_AVND_SU_SI_CURR_ASSIGN_STATE_IS_REMOVED(curr_si)) {
		bool one_rec_del = false;
		one_rec_del = (si) ? true : false;

		if (one_rec_del) {
			m_AVND_SEND_CKPT_UPDT_ASYNC_RMV(cb, curr_si, AVND_CKPT_SU_SI_REC);
		}
		rc = (si) ? avnd_su_si_rec_del(cb, &su->name, &si->name) : avnd_su_si_del(cb, &su->name);
		if (NCSCC_RC_SUCCESS != rc)
			goto done;

		/* 
		 * Removal signifies an end to the recovery phase. Initiate repair
		 * unless a NODE level recovery action is in progress.
		 */
		if (m_AVND_SU_IS_FAILED(su) && !su->si_list.n_nodes &&
		    (cb->oper_state == SA_AMF_OPERATIONAL_ENABLED))
			rc = avnd_err_su_repair(cb, su);
	}

	if ((NULL == si) && (cb->term_state == AVND_TERM_STATE_OPENSAF_SHUTDOWN_INITIATED)) {
		(void) avnd_evt_last_step_term_evh(cb, NULL);
	} else if ((NULL == si) && (cb->term_state == AVND_TERM_STATE_OPENSAF_SHUTDOWN_STARTED)) {

		AVND_SU_SI_REC *tmp = avnd_silist_getlast();
		
		if (tmp != NULL) {
			uint32_t sirank = tmp->rank;

			for (; tmp && (tmp->rank == sirank); tmp = avnd_silist_getprev(tmp)) {
				uint32_t rc = avnd_su_si_remove(cb, tmp->su, tmp);
				osafassert(rc == NCSCC_RC_SUCCESS);
			}
		} else {
			LOG_NO("Removed assignments from AMF components");
			avnd_last_step_clean(cb);
		}
	}

	/* 
	 * reset the su-restart flag if all the sis are in assigned 
	 * state (signifying the end of su-restart phase)
	 */
	if (m_AVND_SU_IS_RESTART(su)) {
		m_AVND_SU_ARE_ALL_SI_ASSIGNED(su, are_si_assigned);
		if (true == are_si_assigned) {
			m_AVND_SU_RESTART_RESET(su);
			m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, su, AVND_CKPT_SU_FLAG_CHANGE);
		}
	}

	/* finally initiate buffered assignments, if any */
	rc = avnd_su_siq_prc(cb, su);

done:
	TRACE_LEAVE2("%u", rc);
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
uint32_t avnd_su_si_unmark(AVND_CB *cb, AVND_SU *su)
{
	AVND_SU_SI_REC *si = 0;
	uint32_t rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER2("'%s'", su->name.value);

	/* scan the su-si list & unmark the sis */
	for (si = (AVND_SU_SI_REC *)m_NCS_DBLIST_FIND_FIRST(&su->si_list);
	     si; si = (AVND_SU_SI_REC *)m_NCS_DBLIST_FIND_NEXT(&si->su_dll_node)) {
		rc = avnd_su_si_rec_unmark(cb, su, si);
		if (NCSCC_RC_SUCCESS != rc)
			break;
	}			/* for */

	TRACE_LEAVE();
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
uint32_t avnd_su_si_rec_unmark(AVND_CB *cb, AVND_SU *su, AVND_SU_SI_REC *si)
{
	AVND_COMP_CSI_REC *csi = 0;
	uint32_t rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER2("'%s' : '%s'", su->name.value, si->name.value);

	/* reset the prv state & update the new assign-state */
	si->prv_state = 0;
	m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, si, AVND_CKPT_SU_SI_REC_PRV_STATE);
	si->prv_assign_state = 0;
	m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, si, AVND_CKPT_SU_SI_REC_PRV_ASSIGN_STATE);
	m_AVND_SU_SI_CURR_ASSIGN_STATE_SET(si, AVND_SU_SI_ASSIGN_STATE_UNASSIGNED);
	m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, si, AVND_CKPT_SU_SI_REC_CURR_ASSIGN_STATE);
	TRACE("Unmarking SU-SI assignment,SU ='%s' : SI=%s",si->su_name.value,si->name.value);

	/* scan the si-csi list & unmark the csis */
	for (csi = (AVND_COMP_CSI_REC *)m_NCS_DBLIST_FIND_FIRST(&si->csi_list);
	     csi; csi = (AVND_COMP_CSI_REC *)m_NCS_DBLIST_FIND_NEXT(&csi->si_dll_node)) {
		csi->prv_assign_state = 0;
		m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, csi, AVND_CKPT_COMP_CSI_PRV_ASSIGN_STATE);
		m_AVND_COMP_CSI_CURR_ASSIGN_STATE_SET(csi, AVND_COMP_CSI_ASSIGN_STATE_UNASSIGNED);
		m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, csi, AVND_CKPT_COMP_CSI_CURR_ASSIGN_STATE);
		TRACE("Unmarking CSI'%s' corresponding to the si'%s'",csi->name.value,si->name.value);

		/* remove any pending callbacks for pi comps */
		if (m_AVND_COMP_TYPE_IS_PREINSTANTIABLE(csi->comp)) {
			TRACE("Deleting callbacks for the PI component:'%s'",csi->comp->name.value);
			avnd_comp_cbq_csi_rec_del(cb, csi->comp, &csi->name);
		}
	}/* End for */

	TRACE_LEAVE();
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
uint32_t avnd_evt_avd_su_pres_evh(AVND_CB *cb, AVND_EVT *evt)
{
	AVSV_D2N_PRESENCE_SU_MSG_INFO *info = 0;
	AVND_SU *su = 0;
	uint32_t rc = NCSCC_RC_SUCCESS;

	TRACE_ENTER();

	/* dont process unless AvD is up */
	if (!m_AVND_CB_IS_AVD_UP(cb)) {
		TRACE("AVD is not yet up");
		goto done;
	}

	/* since AMFND is going down no need to process SU instantiate/terminate msg 
	 * because SU instantiate will cause component information to be read from IMMND
	 * and IMMND might have been already terminated and in that case AMFND will osafassert */
	if (m_AVND_IS_SHUTTING_DOWN(cb)) {
		TRACE("AMFND is in SHUTDOWN state");
		goto done;
	}

	info = &evt->info.avd->msg_info.d2n_prsc_su;

	avnd_msgid_assert(info->msg_id);
	cb->rcv_msg_id = info->msg_id;

	/* get the su */
	su = m_AVND_SUDB_REC_GET(cb->sudb, info->su_name);
	if (!su) {
		TRACE("SU'%s', not found in DB",info->su_name.value);
		goto done;
	}

	switch (info->term_state) {
	case true:		/* => terminate the su */
		TRACE("SU term state is set to true");
		/* no si must be assigned to the su */
		osafassert(!m_NCS_DBLIST_FIND_FIRST(&su->si_list));

		/* Mark SU as terminated by admn operation */
		TRACE("Marking SU as terminated by admin operation");
		m_AVND_SU_ADMN_TERM_SET(su);
		m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, su, AVND_CKPT_SU_FLAG_CHANGE);

		/* delete all the curr info on su & comp */
		rc = avnd_su_curr_info_del(cb, su);

		/* now terminate the su */
		if (m_AVND_SU_IS_PREINSTANTIABLE(su)) {
			/* trigger su termination for pi su */
			TRACE("SU termination for PI SU, running the SU presence state FSM");
			rc = avnd_su_pres_fsm_run(cb, su, 0, AVND_SU_PRES_FSM_EV_TERM);
			if (NCSCC_RC_SUCCESS != rc)
				goto done;
		}
		break;

	case false:		/* => instantiate the su */
		TRACE("SU term state is set to false");
		/* Reset admn term operation flag */
		m_AVND_SU_ADMN_TERM_RESET(su);
		m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, su, AVND_CKPT_SU_FLAG_CHANGE);
		/* Add components belonging to this SU if components were not added before.
		   This can happen in runtime when SU is first added and then comp. When SU is added amfd will 
		   send SU info to amfnd, at this point of time no component exists in IMM DB, so SU list of comp is 
		   empty. When comp are added later, it is not sent to amfnd as amfnd reads comp info from IMM DB. 
		   Now when unlock-inst is done then avnd_evt_avd_su_pres_msg is called. At this point of time, we 
		   are not having Comp info in SU list, so need to add it.

		   If component exists as it would be in case controller is coming up with all entities configured,
		   then avnd_comp_config_get_su() will not read anything, it will return success. 

		   Trying to read config info of openSAF SUs may result in a deadlock condition when IMMND calls AMF
		   register sync call(after AMF invokes instantiate script in NoRed SU instantiation sequence) and 
		   amfnd reads immsv database using search_initializie sync api(AMF reads database for middleware 
		   2N SUs during 2N Red SU instantiation). This is purely dependent on timing when immnd is 
		   registering with AMF and AMF is reading config from immnd during controller coming up. 
		   Fix for the above problem is that Middleware Components wont be dynamically added in the case 
		   of openSAF SUs, so don't refresh config info if it is openSAF SU. */

		if ((false == su->is_ncs) && (avnd_comp_config_get_su(su) != NCSCC_RC_SUCCESS)) {
			m_AVND_SU_REG_FAILED_SET(su);
			/* Will transition to instantiation-failed when instantiated */
			LOG_ER("'%s':FAILED", __FUNCTION__); 
			rc = NCSCC_RC_FAILURE;
			break;
		}
		/* trigger su instantiation for pi su */
		if (m_AVND_SU_IS_PREINSTANTIABLE(su)) {
			TRACE("SU instantiation for PI SUs, running the SU presence state FSM:'%s'", su->name.value); 
			rc = avnd_su_pres_fsm_run(cb, su, 0, AVND_SU_PRES_FSM_EV_INST);
		}
		else {
			if (m_AVND_SU_IS_REG_FAILED(su)) {
				/* The SU configuration is bad, we cannot do much other transition to failed state */
				TRACE_2("SU Configuration is bad");
				avnd_su_pres_state_set(su, SA_AMF_PRESENCE_INSTANTIATION_FAILED);
				m_AVND_SU_ALL_TERM_RESET(su);
			} else
				osafassert(0);
		}
		break;
	}			/* switch */

done:
	TRACE_LEAVE2("%u", rc);
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
uint32_t avnd_su_pres_fsm_run(AVND_CB *cb, AVND_SU *su, AVND_COMP *comp, AVND_SU_PRES_FSM_EV ev)
{
	SaAmfPresenceStateT prv_st, final_st;
	uint32_t rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER2("'%s'", su->name.value);

	/* get the prv presence state */
	prv_st = su->pres;

	TRACE_1("Entering SU presence state FSM: current state: %u, event: %u, su name:%s",prv_st, ev, su->name.value);

	/* run the fsm */
	if (0 != avnd_su_pres_fsm[prv_st - 1][ev - 1]) {
		rc = avnd_su_pres_fsm[prv_st - 1][ev - 1] (cb, su, comp);
		if (NCSCC_RC_SUCCESS != rc)
			goto done;
	}

	/* get the final presence state */
	final_st = su->pres;

	TRACE_1("Exited SU presence state FSM: New State = %u",final_st);

	/* process state change */
	if (prv_st != final_st)
		rc = avnd_su_pres_st_chng_prc(cb, su, prv_st, final_st);

 done:
	TRACE_LEAVE2("%u", rc);
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
uint32_t avnd_su_pres_st_chng_prc(AVND_CB *cb, AVND_SU *su, SaAmfPresenceStateT prv_st, SaAmfPresenceStateT final_st)
{
	AVND_SU_SI_REC *si = 0;
	bool is_en;
	uint32_t rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER2("'%s'", su->name.value);

	/* pi su */
	if (m_AVND_SU_IS_PREINSTANTIABLE(su)) {
		TRACE("PI SU :'%s'",su->name.value);
		/* instantiating -> instantiated */
		if ((SA_AMF_PRESENCE_INSTANTIATING == prv_st) && (SA_AMF_PRESENCE_INSTANTIATED == final_st)) {
			TRACE("SU Instantiating -> Instantiated");
			/* reset the su failed flag */
			if (m_AVND_SU_IS_FAILED(su)) {
				m_AVND_SU_FAILED_RESET(su);
				m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, su, AVND_CKPT_SU_FLAG_CHANGE);
			}

			/* determine the su oper state. if enabled, inform avd. */
			m_AVND_SU_IS_ENABLED(su, is_en);
			if (true == is_en) {
				TRACE("SU oper state is enabled");
				m_AVND_SU_OPER_STATE_SET(su, SA_AMF_OPERATIONAL_ENABLED);
				m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, su, AVND_CKPT_SU_OPER_STATE);
				rc = avnd_di_oper_send(cb, su, 0);
				if (NCSCC_RC_SUCCESS != rc)
					goto done;
			}
			else
				TRACE("SU oper state is disabled");
		}

		/* restarting -> instantiated */
		if ((SA_AMF_PRESENCE_RESTARTING == prv_st) && (SA_AMF_PRESENCE_INSTANTIATED == final_st)) {
			TRACE("SU Restarting -> Instantiated");
			/* reset the su failed flag & set the oper state to enabled */
			if (m_AVND_SU_IS_FAILED(su)) {
				m_AVND_SU_FAILED_RESET(su);
				m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, su, AVND_CKPT_SU_FLAG_CHANGE);
				m_AVND_SU_OPER_STATE_SET(su, SA_AMF_OPERATIONAL_ENABLED);
				m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, su, AVND_CKPT_SU_OPER_STATE);
				TRACE("Setting the Oper state to Enabled");
			}

			/* 
			 * reassign all the sis... 
			 * it's possible that the si was never assigned. send su-oper 
			 * enable msg instead.
			 */
			if (su->si_list.n_nodes) {
				TRACE("Reassigning the SIs");
				rc = avnd_su_si_reassign(cb, su);
			}
			else
				rc = avnd_di_oper_send(cb, su, 0);
			if (NCSCC_RC_SUCCESS != rc)
				goto done;

		}

		/* terminating -> instantiated */
		if ((SA_AMF_PRESENCE_TERMINATING == prv_st) && (SA_AMF_PRESENCE_INSTANTIATED == final_st)) {
			TRACE("SU Terminating -> Instantiated");
			/* reset the su failed flag */
			if (m_AVND_SU_IS_FAILED(su)) {
				m_AVND_SU_FAILED_RESET(su);
				m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, su, AVND_CKPT_SU_FLAG_CHANGE);
			}

			/* determine su oper state. if enabled, inform avd. */
			m_AVND_SU_IS_ENABLED(su, is_en);
			if (true == is_en) {
				TRACE("SU oper state is enabled");
				m_AVND_SU_OPER_STATE_SET(su, SA_AMF_OPERATIONAL_ENABLED);
				m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, su, AVND_CKPT_SU_OPER_STATE);
				rc = avnd_di_oper_send(cb, su, 0);
				if (NCSCC_RC_SUCCESS != rc)
					goto done;
			}
			else
				TRACE("SU oper state is disabled");
		}

		/* terminating -> uninstantiated */
		if ((SA_AMF_PRESENCE_TERMINATING == prv_st) && (SA_AMF_PRESENCE_UNINSTANTIATED == final_st)) {
			TRACE("SU Terminating -> Uninstantiated");
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
			TRACE("SU Instantiating -> Instantiation Failed");
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
			TRACE("SU Instantiated/Restarting -> Instantiation Failed");

			/* send the su-oper state msg (to indicate that instantiation failed) */
			if (m_AVND_SU_OPER_STATE_IS_ENABLED(su)) {
				TRACE("SU oper state is enabled");
				m_AVND_SU_OPER_STATE_SET(su, SA_AMF_OPERATIONAL_DISABLED);
				m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, su, AVND_CKPT_SU_OPER_STATE);
				rc = avnd_di_oper_send(cb, su, AVSV_ERR_RCVR_SU_FAILOVER);
				if (NCSCC_RC_SUCCESS != rc)
					goto done;
			}
			else
				TRACE("SU oper state is disabled");
		}

		/* terminating -> term-failed */
		if ((SA_AMF_PRESENCE_TERMINATING == prv_st) && (SA_AMF_PRESENCE_TERMINATION_FAILED == final_st)) {
			TRACE("Terminating -> Termination Failed");
			m_AVND_SU_OPER_STATE_SET(su, SA_AMF_OPERATIONAL_DISABLED);
			m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, su, AVND_CKPT_SU_OPER_STATE);
			/* inform AvD about oper state change */
			rc = avnd_di_oper_send(cb, su, 0);
			if (NCSCC_RC_SUCCESS != rc)
				goto done;

		}

	}

	/* npi su */
	if (!m_AVND_SU_IS_PREINSTANTIABLE(su)) {
		TRACE("NPI SU :'%s'",su->name.value);
		/* get the only si */
		si = (AVND_SU_SI_REC *)m_NCS_DBLIST_FIND_FIRST(&su->si_list);
		osafassert(si);

		/* instantiating -> instantiated */
		if ((SA_AMF_PRESENCE_INSTANTIATING == prv_st) && (SA_AMF_PRESENCE_INSTANTIATED == final_st)) {
			TRACE("SU Instantiating -> Instantiated");
			/* si assignment success.. generate si-oper done indication */
			rc = avnd_su_si_oper_done(cb, su, m_AVND_SU_IS_ALL_SI(su) ? 0 : si);
			m_AVND_SU_ALL_SI_RESET(su);
			m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, su, AVND_CKPT_SU_FLAG_CHANGE);
			TRACE("SI Assignment succeeded, generating si-oper done indication");
		}

		/* instantiating/instantiated/restarting -> inst-failed */
		if (((SA_AMF_PRESENCE_INSTANTIATING == prv_st) ||
		     (SA_AMF_PRESENCE_INSTANTIATED == prv_st) ||
		     (SA_AMF_PRESENCE_RESTARTING == prv_st)) && (SA_AMF_PRESENCE_INSTANTIATION_FAILED == final_st)) {
			TRACE("SU Instantiating/Instantiated/Restarting -> Instantiation Failed");
			/* si-assignment failed .. inform avd */
			TRACE("SI-Assignment failed, Informing AVD");
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
			TRACE("Restarting -> Instantiated");
			/* reset the failed & restart flag */
			m_AVND_SU_FAILED_RESET(su);
			m_AVND_SU_RESTART_RESET(su);
			m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, su, AVND_CKPT_SU_FLAG_CHANGE);
		}

		/* terminating -> uninstantiated */
		if ((SA_AMF_PRESENCE_TERMINATING == prv_st) && (SA_AMF_PRESENCE_UNINSTANTIATED == final_st)) {
			TRACE("Terminating -> UnInstantiated");
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
			TRACE("Terminating/Instantiated/Restarting -> Termination Failed");
			m_AVND_SU_OPER_STATE_SET(su, SA_AMF_OPERATIONAL_DISABLED);
			m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, su, AVND_CKPT_SU_OPER_STATE);
			/* inform AvD about oper state change */
			rc = avnd_di_oper_send(cb, su, 0);

			/* si assignment/removal failed.. inform AvD */
			rc = avnd_di_susi_resp_send(cb, su, m_AVND_SU_IS_ALL_SI(su) ? 0 : si);
		}
	}

 done:
	TRACE_LEAVE2("%u", rc);
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
                  comp - ptr to the comp (can be NULL)
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
uint32_t avnd_su_pres_uninst_suinst_hdler(AVND_CB *cb, AVND_SU *su, AVND_COMP *comp)
{
	AVND_COMP *curr_comp = 0;
	AVND_SU_SI_REC *si = 0;
	AVND_COMP_CSI_REC *csi = 0;
	uint32_t rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER2("SU Instantiate event in the Uninstantiated state: '%s'", su->name.value);

	/* 
	 * If pi su, pick the first pi comp & trigger it's FSM with InstEv.
	 */
	if (m_AVND_SU_IS_PREINSTANTIABLE(su)) {
		TRACE("PI SU:'%s'",su->name.value);
		for (curr_comp = m_AVND_COMP_FROM_SU_DLL_NODE_GET(m_NCS_DBLIST_FIND_FIRST(&su->comp_list));
		     curr_comp;
		     curr_comp = m_AVND_COMP_FROM_SU_DLL_NODE_GET(m_NCS_DBLIST_FIND_NEXT(&curr_comp->su_dll_node))) {
			/* instantiate the pi comp */
			if (m_AVND_COMP_TYPE_IS_PREINSTANTIABLE(curr_comp)) {
				TRACE("Running the component CLC FSM ");
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
		TRACE("NPI SU:'%s'",su->name.value);
		/* get the only si rec */
		si = (AVND_SU_SI_REC *)m_NCS_DBLIST_FIND_FIRST(&su->si_list);
		osafassert(si);

		csi = (AVND_COMP_CSI_REC *)m_NCS_DBLIST_FIND_FIRST(&si->csi_list);
		if (csi) {
			/* mark the csi state assigning */
			m_AVND_COMP_CSI_CURR_ASSIGN_STATE_SET(csi, AVND_COMP_CSI_ASSIGN_STATE_ASSIGNING);
			m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, csi, AVND_CKPT_COMP_CSI_CURR_ASSIGN_STATE);

			TRACE("Running the component CLC FSM ");
			/* instantiate the comp */
			rc = avnd_comp_clc_fsm_run(cb, csi->comp, AVND_COMP_CLC_PRES_FSM_EV_INST);
			if (NCSCC_RC_SUCCESS != rc)
				goto done;
		}
	}

	/* transition to instantiating state */
	avnd_su_pres_state_set(su, SA_AMF_PRESENCE_INSTANTIATING);

 done:
	if (rc == NCSCC_RC_FAILURE)
		avnd_su_pres_state_set(su, SA_AMF_PRESENCE_INSTANTIATION_FAILED);
	TRACE_LEAVE2("%u", rc);
	return rc;
}

/****************************************************************************
  Name          : avnd_su_pres_insting_suterm_hdler
 
  Description   : This routine processes the `SU Terminate` event in 
                  `Instantiating` state.
 
  Arguments     : cb   - ptr to the AvND control block
                  su   - ptr to the su
                  comp - ptr to the comp (can be NULL)
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : Note that this event is only generated for PI SUs. SUTermEv
                  is generated for NPI SUs only during stable states i.e.
                  uninstantiated & instantiated states. This is done by 
                  avoiding overlapping SI assignments.
******************************************************************************/
uint32_t avnd_su_pres_insting_suterm_hdler(AVND_CB *cb, AVND_SU *su, AVND_COMP *comp)
{
	AVND_COMP *curr_comp = 0;
	uint32_t rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER2("SU Terminate event in Instantiating state:'%s'", su->name.value);

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
			TRACE("Running the component clc FSM");
			rc = avnd_comp_clc_fsm_run(cb, curr_comp, AVND_COMP_CLC_PRES_FSM_EV_TERM);
			if (NCSCC_RC_SUCCESS != rc)
				goto done;
		}
	}			/* for */

	/* transition to terminating state */
	avnd_su_pres_state_set(su, SA_AMF_PRESENCE_TERMINATING);

 done:
	TRACE_LEAVE2("%u", rc);
	return rc;
}

/****************************************************************************
  Name          : avnd_su_pres_insting_surestart_hdler
 
  Description   : This routine processes the `SU Restart` event in 
                  `Instantiating` state.
 
  Arguments     : cb   - ptr to the AvND control block
                  su   - ptr to the su
                  comp - ptr to the comp (can be NULL)
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
uint32_t avnd_su_pres_insting_surestart_hdler(AVND_CB *cb, AVND_SU *su, AVND_COMP *comp)
{				/* TBD */
	uint32_t rc = NCSCC_RC_SUCCESS;
	TRACE("SU Restart event in the Instantiating State, returning success:'%s'",su->name.value);
	return rc;
}

/****************************************************************************
  Name          : avnd_su_pres_insting_compinst_hdler
 
  Description   : This routine processes the `CompInstantiated` event in 
                  `Instantiating` state.
 
  Arguments     : cb   - ptr to the AvND control block
                  su   - ptr to the su
                  comp - ptr to the comp (can be NULL)
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
uint32_t avnd_su_pres_insting_compinst_hdler(AVND_CB *cb, AVND_SU *su, AVND_COMP *comp)
{
	AVND_COMP *curr_comp = 0;
	AVND_COMP_CSI_REC *curr_csi = 0;
	bool is;
	uint32_t rc = NCSCC_RC_SUCCESS;
	char *compname = comp ? (char*)comp->name.value : "none";
	TRACE_ENTER2("Component Instantiated event in the Instantiating state:'%s' : '%s'",
				 su->name.value, compname);

	/*
	 * If pi su, pick the next pi comp & trigger it's FSM with InstEv.
	 * If the component is marked failed (=> component has reinstantiated 
	 * after some failure), unmark it & determine the su presence state.
	 */
	if (m_AVND_SU_IS_PREINSTANTIABLE(su)) {
		TRACE("PI SU");
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
					TRACE("Running the component clc FSM");
					rc = avnd_comp_clc_fsm_run(cb, curr_comp, AVND_COMP_CLC_PRES_FSM_EV_INST);
					if (NCSCC_RC_SUCCESS != rc)
						goto done;
					break;
				}
			}	/* for */
		}

		/* determine su presence state */
		m_AVND_SU_IS_INSTANTIATED(su, is);
		if (true == is) {
			avnd_su_pres_state_set(su, SA_AMF_PRESENCE_INSTANTIATED);
		}
	}

	/*
	 * If npi su, pick the next csi & trigger it's comp fsm with InstEv.
	 */
	if (!m_AVND_SU_IS_PREINSTANTIABLE(su)) {
		TRACE("NPI SU");
		/* get the only csi rec */
		curr_csi = m_AVND_CSI_REC_FROM_COMP_DLL_NODE_GET(m_NCS_DBLIST_FIND_FIRST(&comp->csi_list));
		osafassert(curr_csi);

		/* mark the csi state assigned */
		m_AVND_COMP_CSI_CURR_ASSIGN_STATE_SET(curr_csi, AVND_COMP_CSI_ASSIGN_STATE_ASSIGNED);
		m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, curr_csi, AVND_CKPT_COMP_CSI_CURR_ASSIGN_STATE);

		/* get the next csi */
		curr_csi = (AVND_COMP_CSI_REC *)m_NCS_DBLIST_FIND_NEXT(&curr_csi->si_dll_node);
		if (curr_csi) {
			/* we have another csi. trigger the comp fsm with InstEv */
			TRACE("There's another CSI, Running the component clc FSM");
			rc = avnd_comp_clc_fsm_trigger(cb, curr_csi->comp, AVND_COMP_CLC_PRES_FSM_EV_INST);
			if (NCSCC_RC_SUCCESS != rc)
				goto done;
		} else {
			/* => si assignment done */
			TRACE("SI Assignment done");
			avnd_su_pres_state_set(su, SA_AMF_PRESENCE_INSTANTIATED);
		}
	}

 done:
	TRACE_LEAVE2("%u", rc);
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
                  comp - ptr to the comp (can be NULL)
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None. 
******************************************************************************/
uint32_t avnd_su_pres_insting_compinstfail_hdler(AVND_CB *cb, AVND_SU *su, AVND_COMP *comp)
{
	AVND_COMP *curr_comp = 0;
	AVND_SU_SI_REC *si = 0;
	AVND_COMP_CSI_REC *curr_csi = 0;
	uint32_t rc = NCSCC_RC_SUCCESS;
	char *compname = comp ? (char*)comp->name.value : "none";
	TRACE_ENTER2("CompInstantiateFailed event in the Instantiate State: '%s' : '%s'",
				 su->name.value, compname);

	/* transition to inst-failed state */
	avnd_su_pres_state_set(su, SA_AMF_PRESENCE_INSTANTIATION_FAILED);
	m_AVND_SU_ALL_TERM_RESET(su);
	m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, su, AVND_CKPT_SU_FLAG_CHANGE);

	/* 
	 * If pi su, pick all the instantiated/instantiating pi comps & 
	 * trigger their FSM with TermEv.
	 */
	if (m_AVND_SU_IS_PREINSTANTIABLE(su)) {
		TRACE("PI SU:'%s'",su->name.value);
		for (curr_comp = m_AVND_COMP_FROM_SU_DLL_NODE_GET(m_NCS_DBLIST_FIND_LAST(&su->comp_list));
		     curr_comp;
		     curr_comp = m_AVND_COMP_FROM_SU_DLL_NODE_GET(m_NCS_DBLIST_FIND_PREV(&curr_comp->su_dll_node))) {
			/* skip the npi comps */
			if (!m_AVND_COMP_TYPE_IS_PREINSTANTIABLE(curr_comp))
				continue;

			/* terminate the non-uninstantiated pi comp */
			if (!m_AVND_COMP_PRES_STATE_IS_UNINSTANTIATED(curr_comp)) {
				TRACE("Running the component clc FSM");
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
		TRACE("NPI SU:'%s'", su->name.value);
		/* get the only si rec */
		si = (AVND_SU_SI_REC *)m_NCS_DBLIST_FIND_FIRST(&su->si_list);
		osafassert(si);

		for (curr_csi = (AVND_COMP_CSI_REC *)m_NCS_DBLIST_FIND_LAST(&si->csi_list);
		     curr_csi; curr_csi = (AVND_COMP_CSI_REC *)m_NCS_DBLIST_FIND_PREV(&curr_csi->si_dll_node)) {
			/* terminate the component containing non-unassigned csi */
			if (!m_AVND_COMP_CSI_CURR_ASSIGN_STATE_IS_UNASSIGNED(curr_csi) &&
			    (curr_csi->comp->pres != SA_AMF_PRESENCE_INSTANTIATION_FAILED)) {
				TRACE("Running the component clc FSM");
				rc = avnd_comp_clc_fsm_run(cb, curr_csi->comp, AVND_COMP_CLC_PRES_FSM_EV_TERM);
				if (NCSCC_RC_SUCCESS != rc)
					goto done;
			}
		}		/* for */
		m_AVND_SU_ALL_TERM_SET(su);
	}

 done:
	TRACE_LEAVE2("%u", rc);
	return rc;
}

/****************************************************************************
  Name          : avnd_su_pres_inst_suterm_hdler
 
  Description   : This routine processes the `SU Terminate` event in 
                  `Instantiated` state.
 
  Arguments     : cb   - ptr to the AvND control block
                  su   - ptr to the su
                  comp - ptr to the comp (can be NULL)
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
uint32_t avnd_su_pres_inst_suterm_hdler(AVND_CB *cb, AVND_SU *su, AVND_COMP *comp)
{
	AVND_COMP *curr_comp = 0;
	AVND_SU_SI_REC *si = 0;
	AVND_COMP_CSI_REC *csi = 0;
	uint32_t rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER2("SUTerminate event in the Instantiated state: '%s'", su->name.value);

	/* 
	 * If pi su, pick the last pi comp & trigger it's FSM with TermEv.
	 */
	if (m_AVND_SU_IS_PREINSTANTIABLE(su)) {
		TRACE("PI SU:'%s'",su->name.value);
		for (curr_comp = m_AVND_COMP_FROM_SU_DLL_NODE_GET(m_NCS_DBLIST_FIND_LAST(&su->comp_list));
		     curr_comp;
		     curr_comp = m_AVND_COMP_FROM_SU_DLL_NODE_GET(m_NCS_DBLIST_FIND_PREV(&curr_comp->su_dll_node))) {
			/* terminate the pi comp */
			if (m_AVND_COMP_TYPE_IS_PREINSTANTIABLE(curr_comp)) {
				TRACE("Running the component clc FSM, terminate the component");
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
		TRACE("NPI SU:'%s'",su->name.value);
		/* get the only si rec */
		si = (AVND_SU_SI_REC *)m_NCS_DBLIST_FIND_FIRST(&su->si_list);
		osafassert(si);
		csi = (AVND_COMP_CSI_REC *)m_NCS_DBLIST_FIND_LAST(&si->csi_list);
		osafassert(csi);

		/* mark the csi state assigning/removing */
		if (m_AVND_SU_SI_CURR_ASSIGN_STATE_IS_REMOVING(si))
			m_AVND_COMP_CSI_CURR_ASSIGN_STATE_SET(csi, AVND_COMP_CSI_ASSIGN_STATE_REMOVING);
		else
			m_AVND_COMP_CSI_CURR_ASSIGN_STATE_SET(csi, AVND_COMP_CSI_ASSIGN_STATE_ASSIGNING);

		m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, csi, AVND_CKPT_COMP_CSI_CURR_ASSIGN_STATE);
		TRACE("Running the component clc FSM, terminate the component");
		/* terminate the comp */
		rc = avnd_comp_clc_fsm_run(cb, csi->comp, (m_AVND_COMP_IS_FAILED(csi->comp)) ?
					   AVND_COMP_CLC_PRES_FSM_EV_CLEANUP : AVND_COMP_CLC_PRES_FSM_EV_TERM);
		if (NCSCC_RC_SUCCESS != rc)
			goto done;
	}

	/* transition to terminating state */
	avnd_su_pres_state_set(su, SA_AMF_PRESENCE_TERMINATING);

 done:
	TRACE_LEAVE2("%u", rc);
	return rc;
}

/****************************************************************************
  Name          : avnd_su_pres_inst_surestart_hdler
 
  Description   : This routine processes the `SU Restart` event in 
                  `Instantiated` state.
 
  Arguments     : cb   - ptr to the AvND control block
                  su   - ptr to the su
                  comp - ptr to the comp (can be NULL)
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
uint32_t avnd_su_pres_inst_surestart_hdler(AVND_CB *cb, AVND_SU *su, AVND_COMP *comp)
{
	AVND_COMP *curr_comp = 0;
	AVND_SU_SI_REC *si = 0;
	AVND_COMP_CSI_REC *csi = 0;
	uint32_t rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER2("SURestart event in the Instantiated state: '%s'", su->name.value);

	/* 
	 * If pi su, pick the first pi comp & trigger it's FSM with RestartEv.
	 */
	if (m_AVND_SU_IS_PREINSTANTIABLE(su)) {
		TRACE("PI SU:'%s'",su->name.value);
		for (curr_comp = m_AVND_COMP_FROM_SU_DLL_NODE_GET(m_NCS_DBLIST_FIND_FIRST(&su->comp_list));
		     curr_comp;
		     curr_comp = m_AVND_COMP_FROM_SU_DLL_NODE_GET(m_NCS_DBLIST_FIND_NEXT(&curr_comp->su_dll_node))) {
			/* restart the pi comp */
			if (m_AVND_COMP_TYPE_IS_PREINSTANTIABLE(curr_comp)) {
				TRACE("Running the component clc FSM, restart the component");
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
		TRACE("NPI SU:'%s'",su->name.value);
		/* get the only si rec */
		si = (AVND_SU_SI_REC *)m_NCS_DBLIST_FIND_FIRST(&su->si_list);
		osafassert(si);

		csi = (AVND_COMP_CSI_REC *)m_NCS_DBLIST_FIND_FIRST(&si->csi_list);
		if (csi) {
			/* mark the csi state assigning */
			m_AVND_COMP_CSI_CURR_ASSIGN_STATE_SET(csi, AVND_COMP_CSI_ASSIGN_STATE_ASSIGNING);
			m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, csi, AVND_CKPT_COMP_CSI_CURR_ASSIGN_STATE);

			TRACE("Running the component clc FSM, restart the component");
			/* restart the comp */
			rc = avnd_comp_clc_fsm_run(cb, csi->comp, AVND_COMP_CLC_PRES_FSM_EV_RESTART);
			if (NCSCC_RC_SUCCESS != rc)
				goto done;
		}
	}

	/* transition to restarting state */
	avnd_su_pres_state_set(su, SA_AMF_PRESENCE_RESTARTING);

 done:
	TRACE_LEAVE2("%u", rc);
	return rc;
}

/****************************************************************************
  Name          : avnd_su_pres_inst_comprestart_hdler
 
  Description   : This routine processes the `Comp Restart` event in 
                  `Instantiated` state.
 
  Arguments     : cb   - ptr to the AvND control block
                  su   - ptr to the su
                  comp - ptr to the comp (can be NULL)
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
uint32_t avnd_su_pres_inst_comprestart_hdler(AVND_CB *cb, AVND_SU *su, AVND_COMP *comp)
{				/* TBD */
	uint32_t rc = NCSCC_RC_SUCCESS;
	char *compname = comp ? (char*)comp->name.value : "none";
	TRACE_1("Component restart event in the Instantiated state, returning success:'%s' : '%s'",
			su->name.value, compname);
	return rc;
}

/****************************************************************************
  Name          : avnd_su_pres_inst_compterming_hdler
 
  Description   : This routine processes the `Comp Terminating` event in 
                  `Instantiated` state.
 
  Arguments     : cb   - ptr to the AvND control block
                  su   - ptr to the su
                  comp - ptr to the comp (can be NULL)
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
uint32_t avnd_su_pres_inst_compterming_hdler(AVND_CB *cb, AVND_SU *su, AVND_COMP *comp)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	char *compname = comp ? (char*)comp->name.value : "none";
	TRACE_ENTER2("CompTerminating event in the Instantiated state:'%s' : '%s'",
				 su->name.value, compname);

	if (m_AVND_SU_IS_PREINSTANTIABLE(su)) {
		if (m_AVND_SU_IS_FAILED(su)) {
			/* transition to terminating state */
			avnd_su_pres_state_set(su, SA_AMF_PRESENCE_TERMINATING);
		}
	}

	TRACE_LEAVE2("%u", rc);
	return rc;
}

/****************************************************************************
  Name          : avnd_su_pres_terming_compinst_hdler
 
  Description   : This routine processes the `Comp Instantiate` event in 
                  `Terminating` state.
 
  Arguments     : cb   - ptr to the AvND control block
                  su   - ptr to the su
                  comp - ptr to the comp (can be NULL)
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
uint32_t avnd_su_pres_terming_compinst_hdler(AVND_CB *cb, AVND_SU *su, AVND_COMP *comp)
{
	bool is;
	uint32_t rc = NCSCC_RC_SUCCESS;
	char *compname = comp ? (char*)comp->name.value : "none";
	TRACE_ENTER2("ComponentInstantiate event in the terminating state:'%s' : '%s'", 
				 su->name.value, compname);

	if (m_AVND_SU_IS_PREINSTANTIABLE(su)) {
		if (m_AVND_COMP_IS_FAILED(comp)) {
			m_AVND_COMP_FAILED_RESET(comp);
			m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, comp, AVND_CKPT_COMP_FLAG_CHANGE);
		}

		/* determine if su can be transitioned to instantiated state */
		m_AVND_SU_IS_INSTANTIATED(su, is);
		if (true == is) {
			avnd_su_pres_state_set(su, SA_AMF_PRESENCE_INSTANTIATED);
		}
	}

	TRACE_LEAVE2("%u", rc);
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
                  comp - ptr to the comp (can be NULL)
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : This function is reused for all state changes into termfail
******************************************************************************/
uint32_t avnd_su_pres_terming_comptermfail_hdler(AVND_CB *cb, AVND_SU *su, AVND_COMP *comp)
{
	AVND_COMP *curr_comp = 0;
	AVND_SU_SI_REC *si = 0;
	AVND_COMP_CSI_REC *curr_csi = 0;
	uint32_t rc = NCSCC_RC_SUCCESS;
	char *compname = comp ? (char*)comp->name.value : "none";
	TRACE_ENTER2("Component Termination failed event in the Terminating state,'%s': '%s'",
				 su->name.value, compname);

	/* 
	 * If pi su, pick all the instantiated/instantiating pi comps & 
	 * trigger their FSM with TermEv.
	 */
	if (m_AVND_SU_IS_PREINSTANTIABLE(su)) {
		TRACE("PI SU");
		for (curr_comp = m_AVND_COMP_FROM_SU_DLL_NODE_GET(m_NCS_DBLIST_FIND_FIRST(&su->comp_list));
		     curr_comp;
		     curr_comp = m_AVND_COMP_FROM_SU_DLL_NODE_GET(m_NCS_DBLIST_FIND_NEXT(&curr_comp->su_dll_node))) {
			/* skip the npi comps */
			if (!m_AVND_COMP_TYPE_IS_PREINSTANTIABLE(curr_comp))
				continue;

			/* terminate the non-uninstantiated pi comp */
			if (!m_AVND_COMP_PRES_STATE_IS_UNINSTANTIATED(curr_comp)) {
				TRACE("Running the component clc FSM, terminate the non-uninstantiated pi comp");
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
		TRACE("NPI SU");
		/* get the only si rec */
		si = (AVND_SU_SI_REC *)m_NCS_DBLIST_FIND_FIRST(&su->si_list);
		osafassert(si);

		for (curr_csi = (AVND_COMP_CSI_REC *)m_NCS_DBLIST_FIND_FIRST(&si->csi_list);
		     curr_csi; curr_csi = (AVND_COMP_CSI_REC *)m_NCS_DBLIST_FIND_NEXT(&curr_csi->si_dll_node)) {
			TRACE("Running the component clc FSM, terminate the component containing non-unassigned csi");
			/* terminate the component containing non-unassigned csi */
			if (!m_AVND_COMP_CSI_CURR_ASSIGN_STATE_IS_UNASSIGNED(curr_csi)) {
				rc = avnd_comp_clc_fsm_trigger(cb, curr_csi->comp, AVND_COMP_CLC_PRES_FSM_EV_TERM);
				if (NCSCC_RC_SUCCESS != rc)
					goto done;
			}
		}		/* for */
	}
	
	/* transition to term-failed state */
	avnd_su_pres_state_set(su, SA_AMF_PRESENCE_TERMINATION_FAILED);

	if (true == su->is_ncs) {
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
	TRACE_LEAVE2("%u", rc);
	return rc;
}

/****************************************************************************
  Name          : avnd_su_pres_terming_compuninst_hdler
 
  Description   : This routine processes the `Comp Uninstantiated` event in 
                  `Terminating` state.
 
  Arguments     : cb   - ptr to the AvND control block
                  su   - ptr to the su
                  comp - ptr to the comp (can be NULL)
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
uint32_t avnd_su_pres_terming_compuninst_hdler(AVND_CB *cb, AVND_SU *su, AVND_COMP *comp)
{
	AVND_COMP *curr_comp = 0;
	AVND_COMP_CSI_REC *curr_csi = 0;
	bool all_uninst = true;
	uint32_t rc = NCSCC_RC_SUCCESS;
	char *compname = comp ? (char*)comp->name.value : "none";
	TRACE_ENTER2("Component Uninstantiated event in the Terminating state:'%s' : '%s'",
				 su->name.value, compname);

	/* This case is for handling the case of admn su term while su is restarting */
	if (m_AVND_SU_IS_PREINSTANTIABLE(su) && m_AVND_SU_IS_FAILED(su) && m_AVND_SU_IS_ADMN_TERM(su)) {
		TRACE("PI SU");
		for (curr_comp = m_AVND_COMP_FROM_SU_DLL_NODE_GET(m_NCS_DBLIST_FIND_FIRST(&su->comp_list));
		     curr_comp;
		     curr_comp = m_AVND_COMP_FROM_SU_DLL_NODE_GET(m_NCS_DBLIST_FIND_NEXT(&curr_comp->su_dll_node))) {
			if ((curr_comp->pres != SA_AMF_PRESENCE_UNINSTANTIATED) &&
			    (m_AVND_COMP_TYPE_IS_PREINSTANTIABLE(curr_comp)))
				all_uninst = false;
		}

		if (all_uninst == true) {
			avnd_su_pres_state_set(su, SA_AMF_PRESENCE_UNINSTANTIATED);

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
		TRACE("PI SU");
		for (curr_comp = m_AVND_COMP_FROM_SU_DLL_NODE_GET(m_NCS_DBLIST_FIND_PREV(&comp->su_dll_node));
		     curr_comp;
		     curr_comp = m_AVND_COMP_FROM_SU_DLL_NODE_GET(m_NCS_DBLIST_FIND_PREV(&curr_comp->su_dll_node))) {
			/* terminate the pi comp */
			if (m_AVND_COMP_TYPE_IS_PREINSTANTIABLE(curr_comp)) {
				TRACE("Running the component clc FSM");
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
			avnd_su_pres_state_set(su, SA_AMF_PRESENCE_UNINSTANTIATED);

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
		TRACE("NPI SU");
		/* get the only csi rec */
		curr_csi = m_AVND_CSI_REC_FROM_COMP_DLL_NODE_GET(m_NCS_DBLIST_FIND_FIRST(&comp->csi_list));
		osafassert(curr_csi);

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
			TRACE("There's another CSI, Running the component clc FSM");
			rc = avnd_comp_clc_fsm_trigger(cb, curr_csi->comp, (m_AVND_COMP_IS_FAILED(curr_csi->comp)) ?
						       AVND_COMP_CLC_PRES_FSM_EV_CLEANUP :
						       AVND_COMP_CLC_PRES_FSM_EV_TERM);
			if (NCSCC_RC_SUCCESS != rc)
				goto done;
		} else {
			TRACE("SI Assignment done");
			avnd_su_pres_state_set(su, SA_AMF_PRESENCE_UNINSTANTIATED);

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
	TRACE_LEAVE2("%u", rc);
	return rc;
}

/****************************************************************************
  Name          : avnd_su_pres_restart_suterm_hdler
 
  Description   : This routine processes the `SU Terminate` event in 
                  `Restarting` state.
 
  Arguments     : cb   - ptr to the AvND control block
                  su   - ptr to the su
                  comp - ptr to the comp (can be NULL)
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
uint32_t avnd_su_pres_restart_suterm_hdler(AVND_CB *cb, AVND_SU *su, AVND_COMP *comp)
{
	AVND_COMP *curr_comp = 0;
	uint32_t rc = NCSCC_RC_SUCCESS;
	char *compname = comp ? (char*)comp->name.value : "none";
	TRACE_ENTER2("SU Terminate event in the Restarting state:'%s' : '%s'",
				 su->name.value, compname);

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
	avnd_su_pres_state_set(su, SA_AMF_PRESENCE_TERMINATING);

 done:
	TRACE_LEAVE2("%u", rc);
	return rc;
}

/****************************************************************************
  Name          : avnd_su_pres_restart_compinst_hdler
 
  Description   : This routine processes the `CompInstantiated` event in
                  `Restarting` state.
 
  Arguments     : cb   - ptr to the AvND control block
                  su   - ptr to the su
                  comp - ptr to the comp (can be NULL)
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
uint32_t avnd_su_pres_restart_compinst_hdler(AVND_CB *cb, AVND_SU *su, AVND_COMP *comp)
{
	AVND_COMP *curr_comp = 0;
	AVND_COMP_CSI_REC *curr_csi = 0;
	bool all_inst = true;
	uint32_t rc = NCSCC_RC_SUCCESS;
	char *compname = comp ? (char*)comp->name.value : "none";
	TRACE_ENTER2("ComponentInstantiated event in the Restarting state:'%s' : '%s'", 
				 su->name.value, compname);

	/* 
	 * If pi su, pick the next pi comp & trigger it's FSM with RestartEv.
	 */
	if (m_AVND_SU_IS_PREINSTANTIABLE(su)) {
		for (curr_comp = m_AVND_COMP_FROM_SU_DLL_NODE_GET(m_NCS_DBLIST_FIND_NEXT(&comp->su_dll_node));
		     curr_comp;
		     curr_comp = m_AVND_COMP_FROM_SU_DLL_NODE_GET(m_NCS_DBLIST_FIND_NEXT(&curr_comp->su_dll_node))) {
			TRACE("PI SU:'%s'", su->name.value);
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
				all_inst = false;
		}

		/* OK, all are instantiated */
		if (all_inst == true) {
			avnd_su_pres_state_set(su, SA_AMF_PRESENCE_INSTANTIATED);
		}
	}

	/* 
	 * If npi su, pick the next csi & trigger it's comp fsm with RestartEv.
	 */
	if (!m_AVND_SU_IS_PREINSTANTIABLE(su)) {
		TRACE("NPI SU:'%s'", su->name.value);
		/* get the only csi rec */
		curr_csi = m_AVND_CSI_REC_FROM_COMP_DLL_NODE_GET(m_NCS_DBLIST_FIND_FIRST(&comp->csi_list));
		osafassert(curr_csi);

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
			avnd_su_pres_state_set(su, SA_AMF_PRESENCE_INSTANTIATED);
		}
	}

 done:
	TRACE_LEAVE2("%u", rc);
	return rc;
}

/****************************************************************************
  Name          : avnd_su_pres_restart_compterming_hdler
 
  Description   : This routine processes the `SU Terminate` event in 
                  `Instantiating` state.
 
  Arguments     : cb   - ptr to the AvND control block
                  su   - ptr to the su
                  comp - ptr to the comp (can be NULL)
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : Note that this event is only generated for PI SUs. SUTermEv
                  is generated for NPI SUs only during stable states i.e.
                  uninstantiated & instantiated states. This is done by 
                  avoiding overlapping SI assignments.
******************************************************************************/
uint32_t avnd_su_pres_restart_compterming_hdler(AVND_CB *cb, AVND_SU *su, AVND_COMP *comp)
{
	AVND_COMP *curr_comp = 0;
	uint32_t rc = NCSCC_RC_SUCCESS;
	char *compname = comp ? (char*)comp->name.value : "none";
	TRACE_ENTER2("SUTerminate event in the Instantiating state:'%s' : '%s'",
				 su->name.value, compname);

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
	avnd_su_pres_state_set(su, SA_AMF_PRESENCE_TERMINATING);

 done:
	TRACE_LEAVE2("%u", rc);
	return rc;
}

/****************************************************************************
  Name          : avnd_su_pres_inst_compinstfail_hdler
 
  Description   : This routine processes the `CompInstantiateFailed` event in 
                  `Instantiated` state.
 
  Arguments     : cb   - ptr to the AvND control block
                  su   - ptr to the su
                  comp - ptr to the comp (can be NULL)
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : This function is reused for the following su presence 
                  state transitions also
                  INSTANTIATED -> INSTANTIATIONFAIL
                  TERMINATING  -> INSTANTIATIONFAIL
                  RESTARTING   -> INSTANTIATIONFAIL
******************************************************************************/
uint32_t avnd_su_pres_inst_compinstfail_hdler(AVND_CB *cb, AVND_SU *su, AVND_COMP *comp)
{
	AVND_COMP *curr_comp = 0;
	AVND_SU_SI_REC *si = 0;
	AVND_COMP_CSI_REC *curr_csi = 0;
	uint32_t rc = NCSCC_RC_SUCCESS;
	uint32_t comp_count = 0;
	char *compname = comp ? (char*)comp->name.value : "none";
	TRACE_ENTER2("Component Instantiation failed event in the Instantiated state: '%s' : '%s'",
				 su->name.value, compname);

	/* transition to inst-failed state */
	avnd_su_pres_state_set(su, SA_AMF_PRESENCE_INSTANTIATION_FAILED);
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
		osafassert(si);

		for (curr_csi = (AVND_COMP_CSI_REC *)m_NCS_DBLIST_FIND_FIRST(&si->csi_list);
		     curr_csi; curr_csi = (AVND_COMP_CSI_REC *)m_NCS_DBLIST_FIND_NEXT(&curr_csi->si_dll_node)) {
			/* terminate the component containing non-unassigned csi */
			if (!m_AVND_COMP_CSI_CURR_ASSIGN_STATE_IS_UNASSIGNED(curr_csi)) {
				rc = avnd_comp_clc_fsm_run(cb, curr_csi->comp, AVND_COMP_CLC_PRES_FSM_EV_TERM);
				if (NCSCC_RC_SUCCESS != rc)
					goto done;
			}
		}		/* for */
		m_AVND_SU_ALL_TERM_SET(su);
	}

 done:
	TRACE_LEAVE2("%u", rc);
	return rc;
}

/****************************************************************************
  Name          : avnd_su_pres_instfailed_compuninst 
 
  Description   : This routine processes the `CompInstantiateFailed` event &
                  `CompUnInstantiated` event in `Instantiationfailed` state.
 
  Arguments     : cb   - ptr to the AvND control block
                  su   - ptr to the su
                  comp - ptr to the comp (can be NULL)
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None. 
******************************************************************************/
uint32_t avnd_su_pres_instfailed_compuninst(AVND_CB *cb, AVND_SU *su, AVND_COMP *comp)
{
	AVND_COMP *curr_comp = 0;
	AVND_SU_SIQ_REC *siq = 0;
	uint32_t rc = NCSCC_RC_SUCCESS;
	char *compname = comp ? (char*)comp->name.value : "none";
	TRACE_ENTER2("CompInstantiateFailed/CompUnInstantiated event in the InstantiationFailed state:'%s', '%s'",
				 su->name.value, compname);

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

						if (true == su->su_is_external) {
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

	TRACE_LEAVE2("%u", rc);
	return rc;
}

