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

  DESCRIPTION:This module deals with the creation, accessing and deletion of
  the SU SI relationship list on the AVD.
  
******************************************************************************
*/

/*
 * Module Inclusion Control...
 */

#include <immutil.h>
#include <logtrace.h>
#include <saflog.h>

#include <amf_util.h>
#include <util.h>
#include <susi.h>
#include <imm.h>
#include <csi.h>
#include <proc.h>
#include <si_dep.h>

/**
 * Create an SaAmfSIAssignment runtime object in IMM.
 * @param ha_state
 * @param si_dn
 * @param su_dn
 */
static void avd_create_susi_in_imm(SaAmfHAStateT ha_state,
       const SaNameT *si_dn, const SaNameT *su_dn)
{
	SaNameT dn;
	SaAmfHAReadinessStateT saAmfSISUHAReadinessState =
			SA_AMF_HARS_READY_FOR_ASSIGNMENT;
	void *arr1[] = { &dn };
	void *arr2[] = { &ha_state };
	void *arr3[] = { &saAmfSISUHAReadinessState };
	const SaImmAttrValuesT_2 attr_safSISU = {
			const_cast<SaImmAttrNameT>("safSISU"),
			SA_IMM_ATTR_SANAMET, 1, arr1
	};
	const SaImmAttrValuesT_2 attr_saAmfSISUHAState = {
			const_cast<SaImmAttrNameT>("saAmfSISUHAState"),
			SA_IMM_ATTR_SAUINT32T, 1, arr2
	};
	const SaImmAttrValuesT_2 attr_saAmfSISUHAReadinessState = {
			const_cast<SaImmAttrNameT>("saAmfSISUHAReadinessState"),
			SA_IMM_ATTR_SAUINT32T, 1, arr3
	};
	const SaImmAttrValuesT_2 *attrValues[] = {
			&attr_safSISU,
			&attr_saAmfSISUHAState,
			&attr_saAmfSISUHAReadinessState,
			NULL
	};

	if (avd_cb->avail_state_avd != SA_AMF_HA_ACTIVE)
		return;

	avsv_create_association_class_dn(su_dn, NULL, "safSISU", &dn);
	avd_saImmOiRtObjectCreate(const_cast<SaImmClassNameT>("SaAmfSIAssignment"),
			si_dn, attrValues);
}

/** Delete an SaAmfSIAssignment from IMM
 * 
 * @param si_dn
 * @param su_dn
 */
static void avd_delete_siassignment_from_imm(const SaNameT *si_dn, const SaNameT *su_dn)
{
       SaNameT dn;

       if (avd_cb->avail_state_avd != SA_AMF_HA_ACTIVE)
	       return;

       avsv_create_association_class_dn(su_dn, si_dn, "safSISU", &dn);
       avd_saImmOiRtObjectDelete(&dn);
}

/**
 * Update an SaAmfSIAssignment runtime object in IMM.
 * @param ha_state
 * @param si_dn
 * @param su_dn
 */
void avd_susi_update(AVD_SU_SI_REL *susi, SaAmfHAStateT ha_state)
{
       SaNameT dn;
       AVD_COMP_CSI_REL *compcsi;

       if (avd_cb->avail_state_avd != SA_AMF_HA_ACTIVE)
	       return;

       avsv_create_association_class_dn(&susi->su->name, &susi->si->name, "safSISU", &dn);

       TRACE("HA State %s of %s for %s", avd_ha_state[ha_state],
	       susi->su->name.value, susi->si->name.value);
       saflog(LOG_NOTICE, amfSvcUsrName, "HA State %s of %s for %s",
	       avd_ha_state[ha_state], susi->su->name.value, susi->si->name.value);

       avd_saImmOiRtObjectUpdate(&dn,const_cast<SaImmAttrNameT>("saAmfSISUHAState"), SA_IMM_ATTR_SAUINT32T, &ha_state);

       /* Update all CSI assignments */
       for (compcsi = susi->list_of_csicomp; compcsi != NULL; compcsi = compcsi->susi_csicomp_next) {
	       avsv_create_association_class_dn(&compcsi->comp->comp_info.name,
		       &compcsi->csi->name, "safCSIComp", &dn);

	       avd_saImmOiRtObjectUpdate(&dn,const_cast<SaImmAttrNameT>("saAmfCSICompHAState"), SA_IMM_ATTR_SAUINT32T, &ha_state);
       }
}

/*****************************************************************************
 * Function: avd_susi_create
 *
 * Purpose:  This function will create and add a AVD_SU_SI_REL structure to 
 * the list of susi in both si and su.
 *
 * Input: cb - the AVD control block
 *        su - The SU structure that needs to have the SU SI relation.
 *        si - The SI structure that needs to have the SU SI relation.
 *
 * Returns: The AVD_SU_SI_REL structure that was created and added
 *
 * NOTES:
 *
 * 
 **************************************************************************/

AVD_SU_SI_REL *avd_susi_create(AVD_CL_CB *cb, AVD_SI *si, AVD_SU *su, SaAmfHAStateT state, bool ckpt)
{
	AVD_SU_SI_REL *su_si, *p_su_si, *i_su_si;
	AVD_SU *curr_su = 0;
	AVD_SUS_PER_SI_RANK_INDX i_idx;
	AVD_SUS_PER_SI_RANK *su_rank_rec = 0, *i_su_rank_rec = 0;
	uint32_t rank1, rank2;

	TRACE_ENTER2("%s %s state=%u", su->name.value, si->name.value, state);

	/* Allocate a new block structure now
	 */
	su_si = new AVD_SU_SI_REL();

	su_si->state = state;
	su_si->fsm = AVD_SU_SI_STATE_ABSENT;
	su_si->list_of_csicomp = NULL;
	su_si->si = si;
	su_si->su = su;

	/* 
	 * Add the susi rel rec to the ordered si-list
	 */

	/* determine if the su is ranked per si */
	memset((uint8_t *)&i_idx, '\0', sizeof(i_idx));
	i_idx.si_name = si->name;
	i_idx.su_rank = 0;
	for (su_rank_rec = avd_sirankedsu_getnext(cb, i_idx);
	     su_rank_rec && (m_CMP_HORDER_SANAMET(su_rank_rec->indx.si_name, si->name) == 0);
	     su_rank_rec = avd_sirankedsu_getnext(cb, su_rank_rec->indx)) {
		curr_su = avd_su_get(&su_rank_rec->su_name);
		if (curr_su == su)
			break;
	}

	/* set the ranking flag */
	su_si->is_per_si = (curr_su == su) ? true : false;

	/* determine the insert position */
	for (p_su_si = NULL, i_su_si = si->list_of_sisu;
	     i_su_si; p_su_si = i_su_si, i_su_si = i_su_si->si_next) {
		if (i_su_si->is_per_si == true) {
			if (false == su_si->is_per_si)
				continue;

			/* determine the su_rank rec for this rec */
			memset((uint8_t *)&i_idx, '\0', sizeof(i_idx));
			i_idx.si_name = si->name;
			i_idx.su_rank = 0;
			for (i_su_rank_rec = avd_sirankedsu_getnext(cb, i_idx);
			     i_su_rank_rec
			     && (m_CMP_HORDER_SANAMET(i_su_rank_rec->indx.si_name, si->name) == 0);
			     i_su_rank_rec = avd_sirankedsu_getnext(cb, i_su_rank_rec->indx)) {
				curr_su = avd_su_get(&i_su_rank_rec->su_name);
				if (curr_su == i_su_si->su)
					break;
			}

			osafassert(i_su_rank_rec);

			rank1 = su_rank_rec->indx.su_rank;
			rank2 = i_su_rank_rec->indx.su_rank;
			if (rank1 <= rank2)
				break;
		} else {
			if (true == su_si->is_per_si)
				break;

			if (su->saAmfSURank <= i_su_si->su->saAmfSURank)
				break;
		}
	}			/* for */

	/* now insert the susi rel at the correct position */
	if (p_su_si) {
		su_si->si_next = p_su_si->si_next;
		p_su_si->si_next = su_si;
	} else {
		su_si->si_next = si->list_of_sisu;
		si->list_of_sisu = su_si;
	}

	/* keep the list in su inascending order */
	if (su->list_of_susi == NULL) {
		su->list_of_susi = su_si;
		su_si->su_next = NULL;
		goto done;
	}

	p_su_si = NULL;
	i_su_si = su->list_of_susi;
	while ((i_su_si != NULL) &&
	       (m_CMP_HORDER_SANAMET(i_su_si->si->name, su_si->si->name) < 0)) {
		p_su_si = i_su_si;
		i_su_si = i_su_si->su_next;
	}

	if (p_su_si == NULL) {
		su_si->su_next = su->list_of_susi;
		su->list_of_susi = su_si;
	} else {
		su_si->su_next = p_su_si->su_next;
		p_su_si->su_next = su_si;
	}

done:
	if ((ckpt == false) && (su_si != NULL)) {
		avd_create_susi_in_imm(state, &si->name, &su->name);
		avd_susi_update_assignment_counters(su_si, AVSV_SUSI_ACT_ASGN, state, state);
		avd_gen_su_ha_state_changed_ntf(cb, su_si);
	}

	TRACE_LEAVE();
	return su_si;
}

/*****************************************************************************
 * Function: avd_su_susi_find
 *
 * Purpose:  This function will find a AVD_SU_SI_REL structure from the
 * list of susis in a su.
 *
 * Input: cb - the AVD control block
 *        su - The pointer to the SU . 
 *        si_name - The SI name of the SU SI relation.
 *        
 * Returns: The AVD_SU_SI_REL structure that was found.
 *
 * NOTES:
 *
 * 
 **************************************************************************/

AVD_SU_SI_REL *avd_su_susi_find(AVD_CL_CB *cb, AVD_SU *su, const SaNameT *si_name)
{
	AVD_SU_SI_REL *su_si;
	SaNameT lsi_name;

	su_si = su->list_of_susi;

	memset((char *)&lsi_name, '\0', sizeof(SaNameT));
	memcpy(lsi_name.value, si_name->value, si_name->length);
	lsi_name.length = si_name->length;

	while ((su_si != NULL) && (m_CMP_HORDER_SANAMET(su_si->si->name, lsi_name) < 0)) {
		su_si = su_si->su_next;
	}

	if ((su_si != NULL) && (m_CMP_HORDER_SANAMET(su_si->si->name, lsi_name) == 0)) {
		return su_si;
	}

	return NULL;
}

/*****************************************************************************
 * Function: avd_susi_find
 *
 * Purpose:  This function will find a AVD_SU_SI_REL structure from the
 * list of susis in a su.
 *
 * Input: cb - the AVD control block
 *        su_name - The SU name of the SU SI relation. 
 *        si_name - The SI name of the SU SI relation.
 *        
 * Returns: The AVD_SU_SI_REL structure that was found.
 *
 * NOTES:
 *
 * 
 **************************************************************************/

AVD_SU_SI_REL *avd_susi_find(AVD_CL_CB *cb, const SaNameT *su_name, const SaNameT *si_name)
{
	AVD_SU *su;

	if ((su = avd_su_get(su_name)) == NULL)
		return NULL;

	return avd_su_susi_find(cb, su, si_name);
}

/*****************************************************************************
 * Function: avd_susi_find_next
 *
 * Purpose:  This function will find the next AVD_SU_SI_REL structure from the
 * list of susis in a su. If NULL, it will find the first SUSI for the next
 * SU in the tree.
 *
 * Input: cb - the AVD control block
 *        su_name - The SU name of the SU SI relation. 
 *        si_name - The SI name of the SU SI relation.
 *        
 * Returns: The AVD_SU_SI_REL structure that was found.
 *
 * NOTES:
 *
 * 
 **************************************************************************/

AVD_SU_SI_REL *avd_susi_find_next(AVD_CL_CB *cb, SaNameT su_name, SaNameT si_name)
{
	AVD_SU *su;
	AVD_SU_SI_REL *su_si = NULL;
	SaNameT lsu_name, lsi_name;

	/* check if exact match of SU is found so that the next SU SI
	 * in the list of SU can be found. If not select the next SUs
	 * first SU SI relation
	 */
	if (su_name.length != 0) {
		su = avd_su_get(&su_name);
		if (su == NULL)
			su_si = NULL;
		else
			su_si = su->list_of_susi;
	}

	memset((char *)&lsi_name, '\0', sizeof(SaNameT));
	memcpy(lsi_name.value, si_name.value, si_name.length);
	lsi_name.length = si_name.length;
	
	while ((su_si != NULL) && (m_CMP_HORDER_SANAMET(su_si->si->name, lsi_name) <= 0)) {
		su_si = su_si->su_next;
	}

	if (su_si != NULL) {
		return su_si;
	}

	/* Find the the first SU SI relation in the next SU with
	 * a SU SI relation.
	 */
	lsu_name = su_name;

	while ((su = avd_su_getnext(&lsu_name)) != NULL) {
		if (su->list_of_susi != NULL)
			break;

		lsu_name = su->name;
	}

	/* The given element didn't have a exact match but an element with
	 * a greater SI name was found in the list
	 */

	if (su == NULL)
		return NULL;
	else
		return su->list_of_susi;

	return su_si;
}

/*****************************************************************************
 * Function: avd_susi_delete
 *
 * Purpose:  This function will delete and free AVD_SU_SI_REL structure both
 * the su and si list of susi structures.
 *
 * Input: cb - the AVD control block
 *        susi - The SU SI relation structure that needs to be deleted. 
 *
 * Returns:  NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE 
 *
 *
 * NOTES:
 *
 * 
 **************************************************************************/

uint32_t avd_susi_delete(AVD_CL_CB *cb, AVD_SU_SI_REL *susi, bool ckpt)
{
	AVD_SU_SI_REL *p_su_si, *p_si_su, *i_su_si;

	TRACE_ENTER2("%s %s", susi->su->name.value, susi->si->name.value);

	/* check the SU list to get the prev pointer */
	i_su_si = susi->su->list_of_susi;
	p_su_si = NULL;
	while ((i_su_si != NULL) && (i_su_si != susi)) {
		p_su_si = i_su_si;
		i_su_si = i_su_si->su_next;
	}

	if (i_su_si == NULL) {
		/* problem it is mssing to delete */
		/* log error */
		return NCSCC_RC_FAILURE;
	}

	/* check the SI list to get the prev pointer */
	i_su_si = susi->si->list_of_sisu;
	p_si_su = NULL;

	while ((i_su_si != NULL) && (i_su_si != susi)) {
		p_si_su = i_su_si;
		i_su_si = i_su_si->si_next;
	}

	if (i_su_si == NULL) {
		/* problem it is mssing to delete */
		/* log error */
		return NCSCC_RC_FAILURE;
	}

	/* now delete it from the SU list */
	if (p_su_si == NULL) {
		susi->su->list_of_susi = susi->su_next;
		susi->su_next = NULL;
	} else {
		p_su_si->su_next = susi->su_next;
		susi->su_next = NULL;
	}

	/* now delete it from the SI list */
	if (p_si_su == NULL) {
		susi->si->list_of_sisu = susi->si_next;
		susi->si_next = NULL;
	} else {
		p_si_su->si_next = susi->si_next;
		susi->si_next = NULL;
	}

	if (ckpt == false) {
		if (susi->fsm == AVD_SU_SI_STATE_MODIFY) {
			/* Dont delete here, if i am in modify state. 
			   only happens when active -> qsd and standby rebooted */
		} else if ((susi->fsm == AVD_SU_SI_STATE_ASGND) || (susi->fsm == AVD_SU_SI_STATE_ASGN)){ 
			if (SA_AMF_HA_STANDBY == susi->state) {
				avd_su_dec_curr_stdby_si(susi->su);
				avd_si_dec_curr_stdby_ass(susi->si);
			} else if ((SA_AMF_HA_ACTIVE == susi->state) || (SA_AMF_HA_QUIESCING == susi->state)) { 
				avd_su_dec_curr_act_si(susi->su);
				avd_si_dec_curr_act_ass(susi->si);
			}
		}
	}

	avd_delete_siassignment_from_imm(&susi->si->name, &susi->su->name);

	susi->si = NULL;
	susi->su = NULL;

	delete susi;

	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

void avd_susi_ha_state_set(AVD_SU_SI_REL *susi, SaAmfHAStateT ha_state)
{
	SaAmfHAStateT old_state = susi->state;

	osafassert(ha_state <= SA_AMF_HA_QUIESCING);
	TRACE_ENTER2("'%s' %s => %s", susi->si->name.value, avd_ha_state[susi->state],
			avd_ha_state[ha_state]);
	saflog(LOG_NOTICE, amfSvcUsrName, "%s HA State %s => %s", susi->si->name.value,
			avd_ha_state[susi->state], avd_ha_state[ha_state]);

	susi->state = ha_state;
	m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(avd_cb, susi, AVSV_CKPT_AVD_SI_ASS);

	/* alarm & notifications */
	avd_send_su_ha_state_chg_ntf(&susi->su->name, &susi->si->name, old_state, susi->state);
}

/* This function serves as a wrapper. avd_susi_ha_state_set should be used for state 
 * changes and ntf but introducing avd_susi_ha_state_set and removing 
 * avd_gen_su_ha_state_changed_ntf (155 occurrences!) have big impact on the code.
 * */
uint32_t avd_gen_su_ha_state_changed_ntf(AVD_CL_CB *avd_cb, AVD_SU_SI_REL *susi)
{
	uint32_t status = NCSCC_RC_FAILURE;

	TRACE_ENTER2("'%s' assigned to '%s' HA state '%s'", susi->si->name.value, 
			susi->su->name.value, avd_ha_state[susi->state]);
	saflog(LOG_NOTICE, amfSvcUsrName, "%s assigned to %s HA State '%s'", 
			susi->si->name.value, susi->su->name.value, avd_ha_state[susi->state]);

	/* alarm & notifications */
	avd_send_su_ha_state_chg_ntf(&susi->su->name, &susi->si->name, static_cast<SaAmfHAStateT>(SA_FALSE), susi->state); /*old state not known*/
	
	return status;
}
/**
 * @brief     Finds the most preferred Standby SISU for a particular Si to do SI failover
 *
 * @param[in] si
 *
 * @return   AVD_SU_SI_REL - pointer to the preferred sisu
 *
 */
AVD_SU_SI_REL * avd_find_preferred_standby_susi(AVD_SI *si)
{
	AVD_SU_SI_REL *curr_sisu, *curr_susi;
	SaUint32T curr_su_act_cnt = 0;

	TRACE_ENTER();

	for (curr_sisu = si->list_of_sisu;curr_sisu != NULL;curr_sisu = curr_sisu->si_next) {
		if ((SA_AMF_READINESS_IN_SERVICE == curr_sisu->su->saAmfSuReadinessState) &&
			(SA_AMF_HA_STANDBY == curr_sisu->state)) {
			/* Find the Current Active assignments on the curr_sisu->su.
			 * We cannot depend on su->saAmfSUNumCurrActiveSIs, because for an Active assignment
			 * saAmfSUNumCurrActiveSIs will be  updated only after completion of the assignment
			 * process(getting response). So if there are any  assignments in the middle  of
			 * assignment process saAmfSUNumCurrActiveSIs wont give the currect value
			 */
			curr_su_act_cnt = 0;
			for (curr_susi = curr_sisu->su->list_of_susi;curr_susi != NULL;
				curr_susi = curr_susi->su_next) {
				if (SA_AMF_HA_ACTIVE == curr_susi->state)
					curr_su_act_cnt++;
			}
			if (curr_su_act_cnt < si->sg_of_si->saAmfSGMaxActiveSIsperSU) {
				TRACE("Found preferred sisu SU: '%s' SI: '%s'",curr_sisu->su->name.value,
					curr_sisu->si->name.value);
				break;
			}
		}
	}
	TRACE_LEAVE();
	return curr_sisu;
}
/**
 * @brief	Does role modification of a susi 
 *
 * @param[in]	susi
 *		hs_state
 *
 * @return	NCSCC_RC_SUCCESS
 *		NCSCC_RC_FAILURE	 
 *
 */
uint32_t avd_susi_mod_send(AVD_SU_SI_REL *susi, SaAmfHAStateT ha_state)
{
	SaAmfHAStateT old_state;
	AVD_SU_SI_STATE old_fsm_state;
	uint32_t rc = NCSCC_RC_SUCCESS;

	TRACE_ENTER2("SI '%s', SU '%s' ha_state:%d", susi->si->name.value, susi->su->name.value, ha_state);

	old_state = susi->state;
	old_fsm_state = susi->fsm;
	susi->state = ha_state;
	susi->fsm = AVD_SU_SI_STATE_MODIFY;
	rc = avd_snd_susi_msg(avd_cb, susi->su, susi, AVSV_SUSI_ACT_MOD, false, NULL);
	if (NCSCC_RC_SUCCESS != rc) {
		LOG_NO("role modification msg send failed %s:%u: SU:%s SI:%s", __FILE__,__LINE__,
			susi->su->name.value,susi->si->name.value);
		susi->state = old_state;
		susi->fsm = old_fsm_state;
		goto done;
	}
	/* Update the assignment counters */
	avd_susi_update_assignment_counters(susi, AVSV_SUSI_ACT_MOD, old_state, ha_state);

	/* Following updations has to be done after getting response, will take care subsequently */
	m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(avd_cb, susi, AVSV_CKPT_AVD_SI_ASS);
	avd_susi_update(susi, ha_state);
	avd_gen_su_ha_state_changed_ntf(avd_cb, susi);

done:
	TRACE_LEAVE2("%u", rc);
	return rc;
}
/**
 * @brief	Sends susi delete message
 *
 * @param[in]	susi
 *		send_notification[true/false]
 *
 * @return	NCSCC_RC_SUCCESS
 *		NCSCC_RC_FAILURE	 
 *
 **/
uint32_t avd_susi_del_send(AVD_SU_SI_REL *susi)
{
	AVD_SU_SI_STATE old_fsm_state;
	uint32_t rc = NCSCC_RC_SUCCESS;

	TRACE_ENTER2("SI '%s', SU '%s' ", susi->si->name.value, susi->su->name.value);

	old_fsm_state = susi->fsm;
	susi->fsm = AVD_SU_SI_STATE_UNASGN;

	avd_snd_susi_msg(avd_cb, susi->su, susi, AVSV_SUSI_ACT_DEL, false, NULL);
	if (NCSCC_RC_SUCCESS != rc) {
		LOG_NO("susi del msg snd failed %s:%u: SU:%s SI:%s", __FILE__,__LINE__,
				susi->su->name.value,susi->si->name.value);
		susi->fsm = old_fsm_state;
		goto done;
	}
	/* Update the assignment counters */
	avd_susi_update_assignment_counters(susi, AVSV_SUSI_ACT_DEL, static_cast<SaAmfHAStateT>(0), static_cast<SaAmfHAStateT>(0));


	/* Checkpointing has to be done after getting response, will take care subsequently */
	m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(avd_cb, susi, AVSV_CKPT_AVD_SI_ASS);

done:
	TRACE_LEAVE2("%u", rc);
	return rc;
}
/**
 * @brief	update si and su assignment counters
 *		SI counters:
 *			saAmfSINumCurrActiveAssignments
 *			saAmfSINumCurrStandbyAssignments
 *		SU counters:
 *			saAmfSUNumCurrActiveSIs
 *			saAmfSUNumCurrStandbySIs
 *
 * @param[in]  susi
 *             current_hs_state
 *             new_ha_state
 */
void avd_susi_update_assignment_counters(AVD_SU_SI_REL *susi, AVSV_SUSI_ACT action, SaAmfHAStateT current_ha_state, SaAmfHAStateT new_ha_state)
{
	TRACE_ENTER2("SI:'%s', SU:'%s' action:%u current_ha_state:%u new_ha_state:%u",
		susi->si->name.value, susi->su->name.value, action, current_ha_state, new_ha_state); 

	switch (action) {
	case AVSV_SUSI_ACT_ASGN:
		if (new_ha_state == SA_AMF_HA_ACTIVE) {
			avd_su_inc_curr_act_si(susi->su);
			avd_si_inc_curr_act_ass(susi->si);
		} else if (new_ha_state == SA_AMF_HA_STANDBY) { 
			avd_su_inc_curr_stdby_si(susi->su);
			avd_si_inc_curr_stdby_ass(susi->si);
		}
		break;
	case AVSV_SUSI_ACT_MOD:
		if ((current_ha_state == SA_AMF_HA_STANDBY) && (new_ha_state == SA_AMF_HA_ACTIVE)) {
			/* standby to active */
			avd_su_inc_curr_act_si(susi->su);
			avd_su_dec_curr_stdby_si(susi->su);
			avd_si_inc_curr_act_dec_std_ass(susi->si);
		} else if (((current_ha_state == SA_AMF_HA_ACTIVE) || (current_ha_state == SA_AMF_HA_QUIESCING))
				&& (new_ha_state == SA_AMF_HA_QUIESCED)) {
			/* active or quiescing to quiesced */
			avd_su_dec_curr_act_si(susi->su);
			avd_si_dec_curr_act_ass(susi->si);
		} else if ((current_ha_state == SA_AMF_HA_QUIESCED) && (new_ha_state == SA_AMF_HA_STANDBY)) {
			/* active or quiescinf to standby */
			avd_su_inc_curr_stdby_si(susi->su);
			avd_si_inc_curr_stdby_ass(susi->si);
		} else if (((current_ha_state == SA_AMF_HA_ACTIVE) || (current_ha_state == SA_AMF_HA_QUIESCING))
				&& (new_ha_state == SA_AMF_HA_STANDBY)) {
			/* active or quiescinf to standby */
			avd_su_dec_curr_act_si(susi->su);
			avd_su_inc_curr_stdby_si(susi->su);
			avd_si_inc_curr_stdby_dec_act_ass(susi->si);
		} else if ((current_ha_state == SA_AMF_HA_QUIESCED) && (new_ha_state == SA_AMF_HA_ACTIVE)) {
			/* quiescing to active */
			avd_su_inc_curr_act_si(susi->su);
			avd_si_inc_curr_act_ass(susi->si);
		}
		break;
	case AVSV_SUSI_ACT_DEL:
		if (susi->state == SA_AMF_HA_STANDBY) {
			avd_su_dec_curr_stdby_si(susi->su);
			avd_si_dec_curr_stdby_ass(susi->si);
		} else if ((susi->state == SA_AMF_HA_ACTIVE) || (susi->state == SA_AMF_HA_QUIESCING)) {
			avd_su_dec_curr_act_si(susi->su);
			avd_si_dec_curr_act_ass(susi->si);
		}
		break;
	default:
		LOG_ER("%s: invalid action %u", __FUNCTION__, action);
		break;
	}

        TRACE_LEAVE();
}
/**
 * @brief       This routine does the following functionality
 *              b. Checks the dependencies of the SI's to see whether
 *                 role failover can be performed or not
 *              c. If so sends D2N-INFO_SU_SI_ASSIGN modify active to  the Stdby SU
 *
 * @param[in]   sisu 
 *              stdby_su
 *
 * @return
 **/
uint32_t avd_susi_role_failover(AVD_SU_SI_REL *sisu, AVD_SU *su)
{
	uint32_t rc = NCSCC_RC_SUCCESS;

	TRACE_ENTER2(" '%s' '%s'",sisu->si->name.value, sisu->su->name.value);

	if ((sisu->si->si_dep_state == AVD_SI_FAILOVER_UNDER_PROGRESS) ||
			(sisu->si->si_dep_state == AVD_SI_READY_TO_UNASSIGN)) {
		goto done;
	}

	if(avd_sidep_is_si_failover_possible(sisu->si, su)) {
		rc = avd_susi_mod_send(sisu, SA_AMF_HA_ACTIVE);
		if (rc == NCSCC_RC_SUCCESS) {
			if (sisu->si->num_dependents > 0) {
				/* This is a Sponsor SI update its dependent states */
				avd_sidep_update_depstate_si_failover(sisu->si, su);
			}
		}
	}

done:
	TRACE_LEAVE2(":%d", rc);
	return rc;
}
bool si_assignment_state_check(AVD_SI *si)
{
	AVD_SU_SI_REL *sisu;
	bool assignmemt_status = false;

	for (sisu = si->list_of_sisu;sisu;sisu = sisu->si_next) {
		if (((sisu->state == SA_AMF_HA_ACTIVE) || (sisu->state == SA_AMF_HA_QUIESCING)) &&
			(sisu->fsm != AVD_SU_SI_STATE_UNASGN)) {
			assignmemt_status = true;
			break;
		}
	}

	return assignmemt_status;
}

/**
 * @brief	  Quiesced role modifications has to be done in reverse order of si-si dependency.	  
 *		  When susi response is received for quiesced modification, this routine finds 
 *		  which is the next susi to be quiesced based on si-si dependency. 
 *
 * @param [in]	  susi for which we got the response 
 *
 * @returns	  pointer to AVD_SU_SI_REL 
 */
AVD_SU_SI_REL *avd_siass_next_susi_to_quiesce(const AVD_SU_SI_REL *susi)
{
	AVD_SU_SI_REL *a_susi;
	AVD_SPONS_SI_NODE *spons_si_node;

	TRACE_ENTER2("'%s' '%s'", susi->si->name.value, susi->su->name.value);

	for (a_susi = susi->su->list_of_susi; a_susi; a_susi = a_susi->su_next) {
		if (a_susi->state == SA_AMF_HA_ACTIVE) {
			for (spons_si_node = susi->si->spons_si_list;spons_si_node;spons_si_node = spons_si_node->next) {
				if (spons_si_node->si == a_susi->si) {
					/* Check if quiesced response came for all of its dependents */ 
					if (avd_sidep_quiesced_done_for_all_dependents(spons_si_node->si, susi->su)) {
						goto done;
					}
				}
			}
		}
	}
done:
	TRACE_LEAVE2("next_susi: %s",a_susi ? a_susi->si->name.value : NULL);
	return a_susi;
}

/**
 * @brief	Checks whether quiesced role can be given to susi or not 
 *
 * @param[in]   susi
 *
 * @return	true/false 
 **/
bool avd_susi_quiesced_canbe_given(const AVD_SU_SI_REL *susi)
{
	AVD_SI_SI_DEP *si_dep_rec;
	AVD_SI *dep_si;
	AVD_SI_SI_DEP_INDX si_indx;
	AVD_SU_SI_REL *sisu;
	bool quiesc_role = true;

	TRACE_ENTER2("%s %s", susi->su->name.value, susi->si->name.value);

	if (!susi->si->num_dependents) {
		/* This SI doesnot have any dependents on it, so quiesced role can be given */
		return quiesc_role;
	} else {
		/* Check if any of its dependents assigned to same SU for which quiesced role is not yet given */
		memset(&si_indx, '\0', sizeof(si_indx));
		si_indx.si_name_prim.length = susi->si->name.length;
		memcpy(si_indx.si_name_prim.value, susi->si->name.value, si_indx.si_name_prim.length);
		si_dep_rec = avd_sidep_find_next(avd_cb, &si_indx, true);

		while (si_dep_rec != NULL) {
			if (m_CMP_HORDER_SANAMET(si_dep_rec->indx_imm.si_name_prim, si_indx.si_name_prim) != 0) {
				/* Seems no more node exists in spons_anchor tree with
				 * "si_indx.si_name_prim" as primary key
				 */
				break;
			}
			dep_si = avd_si_get(&si_dep_rec->indx_imm.si_name_sec);
			if (dep_si == NULL) {
				/* No corresponding SI node?? some thing wrong */
				si_dep_rec = avd_sidep_find_next(avd_cb, &si_dep_rec->indx_imm, true);
				continue;
			}
			for (sisu = dep_si->list_of_sisu; sisu ; sisu = sisu->si_next) {
				if (sisu->su == susi->su) {
					if ((sisu->state == SA_AMF_HA_ACTIVE) ||
						((sisu->state == SA_AMF_HA_QUIESCED) && (sisu->fsm == AVD_SU_SI_STATE_MODIFY))) {
						quiesc_role = false;
						goto done;
					}       
				}       

			}
			si_dep_rec = avd_sidep_find_next(avd_cb, &si_dep_rec->indx_imm, true);
		}
	}
done:
	TRACE_LEAVE2("quiesc_role:%u",quiesc_role);
	return quiesc_role;
}
