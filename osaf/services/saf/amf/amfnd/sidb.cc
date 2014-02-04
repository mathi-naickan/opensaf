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
 
  This module contains routines to create, modify, delete & fetch the SU-SI
  and component CSI relationship records.

..............................................................................

  FUNCTIONS INCLUDED in this module:

  
******************************************************************************
*/

#include "avnd.h"
#include <immutil.h>

/* static function declarations */

static uint32_t avnd_su_si_csi_rec_modify(AVND_CB *, AVND_SU *, AVND_SU_SI_REC *, AVND_COMP_CSI_PARAM *);

static uint32_t avnd_su_si_csi_all_modify(AVND_CB *, AVND_SU *, AVND_COMP_CSI_PARAM *);

static uint32_t avnd_su_si_csi_rec_del(AVND_CB *, AVND_SU *, AVND_SU_SI_REC *, AVND_COMP_CSI_REC *);

static uint32_t avnd_su_si_csi_del(AVND_CB *, AVND_SU *, AVND_SU_SI_REC *);

/* macro to add a csi-record to the si-csi list */
#define m_AVND_SU_SI_CSI_REC_ADD(si, csi, rc) \
{ \
   (csi).si_dll_node.key = (uint8_t *)&(csi).rank; \
   rc = ncs_db_link_list_add(&(si).csi_list, &(csi).si_dll_node); \
};

/* macro to remove a csi-record from the si-csi list */
#define m_AVND_SU_SI_CSI_REC_REM(si, csi) \
           ncs_db_link_list_delink(&(si).csi_list, &(csi).si_dll_node)

/* macro to add a susi record to the beginning of the susi queue */
#define m_AVND_SUDB_REC_SIQ_ADD(su, susi, rc) \
           (rc) = ncs_db_link_list_add(&(su).siq, &(susi).su_dll_node);

/**
 * Initialize the SI list
 * @param cb
 */
void avnd_silist_init(AVND_CB *cb)
{
	cb->si_list.order = NCS_DBLIST_ASSCEND_ORDER;
	cb->si_list.cmp_cookie = avsv_dblist_uns32_cmp;
	cb->si_list.free_cookie = 0;
}

/**
 * Get first SI from SI list
 * 
 * @return AVND_SU_SI_REC*
 */
AVND_SU_SI_REC *avnd_silist_getfirst(void)
{
	NCS_DB_LINK_LIST_NODE *p = m_NCS_DBLIST_FIND_FIRST(&avnd_cb->si_list);

	if (p != NULL)
		return (AVND_SU_SI_REC *) ((char*)p - ((char*) &((AVND_SU_SI_REC *)NULL)->cb_dll_node));
	else
		return NULL;
}

/**
 * Get next SI from SI list
 * @param si if NULL first SI is returned
 * 
 * @return AVND_SU_SI_REC*
 */
AVND_SU_SI_REC *avnd_silist_getnext(const AVND_SU_SI_REC *si)
{
	if (si) {
		NCS_DB_LINK_LIST_NODE *p = m_NCS_DBLIST_FIND_NEXT(&si->cb_dll_node);

		if (p != NULL)
			return (AVND_SU_SI_REC *) ((char*)p - ((char*) &((AVND_SU_SI_REC *)NULL)->cb_dll_node));
		else
			return NULL;
	}
	else
		return avnd_silist_getfirst();
}

/**
 * Get previous SI from SI list
 * @param si
 * 
 * @return AVND_SU_SI_REC*
 */
AVND_SU_SI_REC *avnd_silist_getprev(const AVND_SU_SI_REC *si)
{
	NCS_DB_LINK_LIST_NODE *p = m_NCS_DBLIST_FIND_PREV(&si->cb_dll_node);

	if (p != NULL)
		return (AVND_SU_SI_REC *) ((char*)p - ((char*) &((AVND_SU_SI_REC *)NULL)->cb_dll_node));
	else
		return NULL;
}

/**
 * Get last SI from SI list
 * 
 * @return AVND_SU_SI_REC*
 */

AVND_SU_SI_REC *avnd_silist_getlast(void)
{
	NCS_DB_LINK_LIST_NODE *p = m_NCS_DBLIST_FIND_LAST(&avnd_cb->si_list);

	if (p != NULL)
		return (AVND_SU_SI_REC *) ((char*)p - ((char*) &((AVND_SU_SI_REC *)NULL)->cb_dll_node));
	else
		return NULL;
}

/**
 * Return SI rank read from IMM
 * 
 * @param dn DN of SI
 * 
 * @return      rank of SI or -1 if not configured for SI
 */
static uint32_t get_sirank(const SaNameT *dn)
{
	SaAisErrorT error;
	SaImmAccessorHandleT accessorHandle;
	const SaImmAttrValuesT_2 **attributes;
	SaImmAttrNameT attributeNames[2] = {const_cast<SaImmAttrNameT>("saAmfSIRank"), NULL};
	SaImmHandleT immOmHandle;
	SaVersionT immVersion = {'A', 2, 1};
	uint32_t rank = -1; // lowest possible rank if uninitialized

	immutil_saImmOmInitialize(&immOmHandle, NULL, &immVersion);
	immutil_saImmOmAccessorInitialize(immOmHandle, &accessorHandle);

	osafassert((error = immutil_saImmOmAccessorGet_2(accessorHandle, dn,
		attributeNames, (SaImmAttrValuesT_2 ***)&attributes)) == SA_AIS_OK);

	osafassert((error = immutil_getAttr(attributeNames[0], attributes, 0, &rank)) == SA_AIS_OK);

	// saAmfSIRank attribute has a default value of zero (returned by IMM)
	if (rank == 0) {
		// Unconfigured ranks are treated as lowest possible rank
		rank = -1;
	}

	immutil_saImmOmAccessorFinalize(accessorHandle);
	immutil_saImmOmFinalize(immOmHandle);

	return rank;
}

/****************************************************************************
  Name          : avnd_su_si_rec_add
 
  Description   : This routine creates an su-si relationship record. It 
                  updates the su-si record & creates the records in the 
                  comp-csi list. If an su-si relationship record already
                  exists, nothing is done.
 
  Arguments     : cb    - ptr to AvND control block
                  su    - ptr to the AvND SU
                  param - ptr to the SI parameters
                  rc    - ptr to the operation result
 
  Return Values : ptr to the su-si relationship record
 
  Notes         : None
******************************************************************************/
AVND_SU_SI_REC *avnd_su_si_rec_add(AVND_CB *cb, AVND_SU *su, AVND_SU_SI_PARAM *param, uint32_t *rc)
{
	AVND_SU_SI_REC *si_rec = 0;
	AVND_COMP_CSI_PARAM *csi_param = 0;

	TRACE_ENTER();

	*rc = NCSCC_RC_SUCCESS;

	/* verify if su-si relationship already exists */
	if (0 != avnd_su_si_rec_get(cb, &param->su_name, &param->si_name)) {
		*rc = AVND_ERR_DUP_SI;
		goto err;
	}

	/* a fresh si... */
	si_rec = new AVND_SU_SI_REC();

	/*
	 * Update the supplied parameters.
	 */
	/* update the si-name (key) */
	memcpy(&si_rec->name, &param->si_name, sizeof(SaNameT));
	si_rec->curr_state = param->ha_state;

	/*
	 * Update the rest of the parameters with default values.
	 */
	m_AVND_SU_SI_CURR_ASSIGN_STATE_SET(si_rec, AVND_SU_SI_ASSIGN_STATE_UNASSIGNED);

	/*
	 * Add the csi records.
	 */
	/* initialize the csi-list (maintained by si) */
	si_rec->csi_list.order = NCS_DBLIST_ASSCEND_ORDER;
	si_rec->csi_list.cmp_cookie = avsv_dblist_uns32_cmp;
	si_rec->csi_list.free_cookie = 0;

	/*
	 * Add to the si-list (maintained by su)
	 */
	m_AVND_SUDB_REC_SI_ADD(*su, *si_rec, *rc);
	if (NCSCC_RC_SUCCESS != *rc) {
		*rc = AVND_ERR_DLL;
		goto err;
	}

	/* Add to global SI list sorted by rank if appl SU */
	if (!su->is_ncs) {
		uint32_t res;
		si_rec->rank = get_sirank(&param->si_name);
		si_rec->cb_dll_node.key = (uint8_t *)&si_rec->rank;
		res = ncs_db_link_list_add(&cb->si_list, &si_rec->cb_dll_node);
		osafassert(res == NCSCC_RC_SUCCESS);
	}

	/*
	 * Update links to other entities.
	 */
	si_rec->su = su;
	si_rec->su_name = su->name;

	/* now add the csi records */
	csi_param = param->list;
	while (0 != csi_param) {
		avnd_su_si_csi_rec_add(cb, su, si_rec, csi_param, rc);
		if (NCSCC_RC_SUCCESS != *rc)
			goto err;
		csi_param = csi_param->next;
	}

	TRACE_1("SU-SI record added, SU= %s : SI=%s",param->su_name.value,param->si_name.value);
	return si_rec;

 err:
	if (si_rec) {
		avnd_su_si_csi_del(cb, su, si_rec);
		delete si_rec;
	}

	LOG_CR("SU-SI record addition failed, SU= %s : SI=%s",param->su_name.value,param->si_name.value);
	TRACE_LEAVE();
	return 0;
}

/****************************************************************************
  Name          : avnd_su_si_csi_rec_add
 
  Description   : This routine creates a comp-csi relationship record & adds 
                  it to the 2 csi lists (maintained by si & comp).
 
  Arguments     : cb     - ptr to AvND control block
                  su     - ptr to the AvND SU
                  si_rec - ptr to the SI record
                  param  - ptr to the CSI parameters
                  rc     - ptr to the operation result
 
  Return Values : ptr to the comp-csi relationship record
 
  Notes         : None
******************************************************************************/
AVND_COMP_CSI_REC *avnd_su_si_csi_rec_add(AVND_CB *cb,
					  AVND_SU *su, AVND_SU_SI_REC *si_rec, AVND_COMP_CSI_PARAM *param, uint32_t *rc)
{
	AVND_COMP_CSI_REC *csi_rec = 0;
	AVND_COMP *comp = 0;

	TRACE_ENTER2("Comp'%s', Csi'%s' and Rank'%u'",param->csi_name.value, param->comp_name.value, param->csi_rank);

	*rc = NCSCC_RC_SUCCESS;

	/* verify if csi record already exists */
	if (0 != avnd_compdb_csi_rec_get(cb, &param->comp_name, &param->csi_name)) {
		TRACE("csi rec get Failed from compdb");
		*rc = AVND_ERR_DUP_CSI;
		goto err;
	}

	/* get the comp */
	comp = m_AVND_COMPDB_REC_GET(cb->compdb, param->comp_name);
	if (!comp) {
		/* This could be becasue of NPI components, NPI components are not added in to DB
		   because amfd doesn't send SU presence message to amfnd when SU is unlock-in.
		   So, add the component into DB now. */
		if (avnd_comp_config_get_su(su) != NCSCC_RC_SUCCESS) {
			m_AVND_SU_REG_FAILED_SET(su);
			/* Will transition to instantiation-failed when instantiated */
			LOG_ER("su comp config get failed for NPI component:%s",param->comp_name.value);
			*rc = AVND_ERR_NO_SU;
			goto err;
		}
		comp = m_AVND_COMPDB_REC_GET(cb->compdb, param->comp_name);
		if (!comp) {
			LOG_ER("comp rec get failed component:%s",param->comp_name.value);
			*rc = AVND_ERR_NO_COMP;
			osafassert(0);
			goto err;
		}
	}

	/* a fresh csi... */
	csi_rec = new AVND_COMP_CSI_REC();

	/*
	 * Update the supplied parameters.
	 */
	/* update the csi-name & csi-rank (keys to comp-csi & si-csi lists resp) */
	memcpy(&csi_rec->name, &param->csi_name, sizeof(SaNameT));
	csi_rec->rank = param->csi_rank;

	{
		/* Fill the cs type param from imm db, it will be used in finding comp capability */
		SaAisErrorT error;
		SaImmAccessorHandleT accessorHandle;
		const SaImmAttrValuesT_2 **attributes;
		SaImmAttrNameT attributeNames[2] = {const_cast<SaImmAttrNameT>("saAmfCSType"), NULL};
		SaImmHandleT immOmHandle;
		SaVersionT immVersion = { 'A', 2, 1 };

		immutil_saImmOmInitialize(&immOmHandle, NULL, &immVersion);
		immutil_saImmOmAccessorInitialize(immOmHandle, &accessorHandle);

		if ((error = immutil_saImmOmAccessorGet_2(accessorHandle, &param->csi_name, attributeNames, 
						(SaImmAttrValuesT_2 ***)&attributes)) != SA_AIS_OK) {
			LOG_ER("saImmOmAccessorGet FAILED %u for %s", error, param->csi_name.value);
			osafassert(0);
		}

		if (immutil_getAttr(const_cast<SaImmAttrNameT>("saAmfCSType"), attributes, 0, &csi_rec->saAmfCSType) != SA_AIS_OK)
			osafassert(0);

		immutil_saImmOmAccessorFinalize(accessorHandle);
		immutil_saImmOmFinalize(immOmHandle);
	}

	/* update the assignment related parameters */
	memcpy(&csi_rec->act_comp_name, &param->active_comp_name, sizeof(SaNameT));
	csi_rec->trans_desc = param->active_comp_dsc;
	csi_rec->standby_rank = param->stdby_rank;

	/* update the csi-attrs.. steal it from param */
	csi_rec->attrs.number = param->attrs.number;
	csi_rec->attrs.list = param->attrs.list;
	param->attrs.number = 0;
	param->attrs.list = 0;

	/*
	 * Update the rest of the parameters with default values.
	 */
	m_AVND_COMP_CSI_CURR_ASSIGN_STATE_SET(csi_rec, AVND_COMP_CSI_ASSIGN_STATE_UNASSIGNED);
	m_AVND_COMP_CSI_PRV_ASSIGN_STATE_SET(csi_rec, AVND_COMP_CSI_ASSIGN_STATE_UNASSIGNED);

	/*
	 * Add to the csi-list (maintained by si).
	 */
	m_AVND_SU_SI_CSI_REC_ADD(*si_rec, *csi_rec, *rc);
	if (NCSCC_RC_SUCCESS != *rc) {
		*rc = AVND_ERR_DLL;
		goto err;
	}

	/*
	 * Add to the csi-list (maintained by comp).
	 */
	m_AVND_COMPDB_REC_CSI_ADD(*comp, *csi_rec, *rc);
	if (NCSCC_RC_SUCCESS != *rc) {
		*rc = AVND_ERR_DLL;
		goto err;
	}

	/*
	 * Update links to other entities.
	 */
	csi_rec->si = si_rec;
	csi_rec->comp = comp;
	csi_rec->comp_name = comp->name;
	csi_rec->si_name = si_rec->name;
	csi_rec->su_name = su->name;

	return csi_rec;

 err:
	if (csi_rec) {
		/* remove from comp-csi & si-csi lists */
		ncs_db_link_list_delink(&si_rec->csi_list, &csi_rec->si_dll_node);
		m_AVND_COMPDB_REC_CSI_REM(*comp, *csi_rec);
		delete csi_rec;
	}

	LOG_CR("Comp-CSI record addition failed, Comp=%s : CSI=%s",param->comp_name.value,param->csi_name.value);
	TRACE_LEAVE();
	return 0;
}

/****************************************************************************
  Name          : avnd_su_si_rec_modify
 
  Description   : This routine modifies an su-si relationship record. It 
                  updates the su-si record & modifies the records in the 
                  comp-csi list.
 
  Arguments     : cb    - ptr to AvND control block
                  su    - ptr to the AvND SU
                  param - ptr to the SI parameters
                  rc    - ptr to the operation result
 
  Return Values : ptr to the modified su-si relationship record
 
  Notes         : None
******************************************************************************/
AVND_SU_SI_REC *avnd_su_si_rec_modify(AVND_CB *cb, AVND_SU *su, AVND_SU_SI_PARAM *param, uint32_t *rc)
{
	AVND_SU_SI_REC *si_rec = 0;

	TRACE_ENTER2();

	*rc = NCSCC_RC_SUCCESS;

	/* get the su-si relationship record */
	si_rec = avnd_su_si_rec_get(cb, &param->su_name, &param->si_name);
	if (!si_rec) {
		*rc = AVND_ERR_NO_SI;
		goto err;
	}

	/* store the prv state & update the new state */
	si_rec->prv_state = si_rec->curr_state;
	si_rec->curr_state = param->ha_state;

	/* store the prv assign-state & update the new assign-state */
	si_rec->prv_assign_state = si_rec->curr_assign_state;
	m_AVND_SU_SI_CURR_ASSIGN_STATE_SET(si_rec, AVND_SU_SI_ASSIGN_STATE_UNASSIGNED);

	m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, si_rec, AVND_CKPT_SU_SI_REC);

	/* now modify the csi records */
	*rc = avnd_su_si_csi_rec_modify(cb, su, si_rec,
					((SA_AMF_HA_QUIESCED == param->ha_state) ||
					 (SA_AMF_HA_QUIESCING == param->ha_state)) ? 0 : param->list);
	if (*rc != NCSCC_RC_SUCCESS)
		goto err;
	TRACE_LEAVE();
	return si_rec;

 err:
	TRACE_LEAVE();
	return 0;
}

/****************************************************************************
  Name          : avnd_su_si_csi_rec_modify
 
  Description   : This routine modifies a comp-csi relationship record.
 
  Arguments     : cb     - ptr to AvND control block
                  su     - ptr to the AvND SU
                  si_rec - ptr to the SI record
                  param  - ptr to the CSI parameters
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None
******************************************************************************/
uint32_t avnd_su_si_csi_rec_modify(AVND_CB *cb, AVND_SU *su, AVND_SU_SI_REC *si_rec, AVND_COMP_CSI_PARAM *param)
{
	AVND_COMP_CSI_PARAM *curr_param = 0;
	AVND_COMP_CSI_REC *curr_csi = 0;
	uint32_t rc = NCSCC_RC_SUCCESS;

	TRACE_ENTER2("%p", param);
	/* pick up all the csis belonging to the si & modify them */
	if (!param) {
		for (curr_csi = (AVND_COMP_CSI_REC *)m_NCS_DBLIST_FIND_FIRST(&si_rec->csi_list);
		     curr_csi; curr_csi = (AVND_COMP_CSI_REC *)m_NCS_DBLIST_FIND_NEXT(&curr_csi->si_dll_node)) {
			/* store the prv assign-state & update the new assign-state */
			curr_csi->prv_assign_state = curr_csi->curr_assign_state;
			m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, curr_csi, AVND_CKPT_COMP_CSI_PRV_ASSIGN_STATE);
			m_AVND_COMP_CSI_CURR_ASSIGN_STATE_SET(curr_csi, AVND_COMP_CSI_ASSIGN_STATE_UNASSIGNED);
			m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, curr_csi, AVND_CKPT_COMP_CSI_CURR_ASSIGN_STATE);
		}		/* for */
	}

	/* pick up the csis belonging to the comps specified in the param-list */
	for (curr_param = param; curr_param; curr_param = curr_param->next) {
		/* get the comp & csi */
		curr_csi = avnd_compdb_csi_rec_get(cb, &curr_param->comp_name, &curr_param->csi_name);
		if (!curr_csi || (curr_csi->comp->su != su)) {
			rc = NCSCC_RC_FAILURE;
			goto done;
		}

		/* update the assignment related parameters */
		curr_csi->act_comp_name = curr_param->active_comp_name;
		curr_csi->trans_desc = curr_param->active_comp_dsc;
		curr_csi->standby_rank = curr_param->stdby_rank;

		/* store the prv assign-state & update the new assign-state */
		curr_csi->prv_assign_state = curr_csi->curr_assign_state;
		m_AVND_COMP_CSI_CURR_ASSIGN_STATE_SET(curr_csi, AVND_COMP_CSI_ASSIGN_STATE_UNASSIGNED);
		m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, curr_csi, AVND_CKPT_CSI_REC);
	}			/* for */

 done:
	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
  Name          : avnd_su_si_all_modify
 
  Description   : This routine modifies all the SU-SI & comp-csi records in 
                  the database.
 
  Arguments     : cb    - ptr to AvND control block
                  su    - ptr to the AvND SU
                  param - ptr to the SI parameters
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None
******************************************************************************/
uint32_t avnd_su_si_all_modify(AVND_CB *cb, AVND_SU *su, AVND_SU_SI_PARAM *param)
{
	AVND_SU_SI_REC *curr_si = 0;
	uint32_t rc = NCSCC_RC_SUCCESS;

	TRACE_ENTER2();
	/* modify all the si records */
	for (curr_si = (AVND_SU_SI_REC *)m_NCS_DBLIST_FIND_FIRST(&su->si_list);
	     curr_si; curr_si = (AVND_SU_SI_REC *)m_NCS_DBLIST_FIND_NEXT(&curr_si->su_dll_node)) {
		/* store the prv state & update the new state */
		curr_si->prv_state = curr_si->curr_state;
		curr_si->curr_state = param->ha_state;

		/* store the prv assign-state & update the new assign-state */
		curr_si->prv_assign_state = curr_si->curr_assign_state;
		m_AVND_SU_SI_CURR_ASSIGN_STATE_SET(curr_si, AVND_SU_SI_ASSIGN_STATE_UNASSIGNED);
		m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, curr_si, AVND_CKPT_SU_SI_REC);
	}                       /* for */

	if (su->si_list.n_nodes > 1)
		LOG_NO("Assigning 'all (%u) SIs' %s to '%s'", su->si_list.n_nodes,
				ha_state[param->ha_state], su->name.value);

	/* now modify the comp-csi records */
	rc = avnd_su_si_csi_all_modify(cb, su, 0);

	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
  Name          : avnd_su_si_csi_all_modify
 
  Description   : This routine modifies the csi records.
 
  Arguments     : cb     - ptr to AvND control block
                  su     - ptr to the AvND SU
                  param  - ptr to the CSI parameters (if 0, => all the CSIs 
                           belonging to all the SIs in the SU are modified. 
                           Else all the CSIs belonging to all the components 
                           in the SU are modified)
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None
******************************************************************************/
uint32_t avnd_su_si_csi_all_modify(AVND_CB *cb, AVND_SU *su, AVND_COMP_CSI_PARAM *param)
{
	AVND_COMP_CSI_PARAM *curr_param = 0;
	AVND_COMP_CSI_REC *curr_csi = 0;
	AVND_SU_SI_REC *curr_si = 0;
	AVND_COMP *curr_comp = 0;
	uint32_t rc = NCSCC_RC_SUCCESS;

	TRACE_ENTER2("%p", param);
	/* pick up all the csis belonging to all the sis & modify them */
	if (!param) {
		for (curr_si = (AVND_SU_SI_REC *)m_NCS_DBLIST_FIND_FIRST(&su->si_list);
		     curr_si; curr_si = (AVND_SU_SI_REC *)m_NCS_DBLIST_FIND_NEXT(&curr_si->su_dll_node)) {
			for (curr_csi = (AVND_COMP_CSI_REC *)m_NCS_DBLIST_FIND_FIRST(&curr_si->csi_list);
			     curr_csi; curr_csi = (AVND_COMP_CSI_REC *)m_NCS_DBLIST_FIND_NEXT(&curr_csi->si_dll_node)) {
				/* store the prv assign-state & update the new assign-state */
				curr_csi->prv_assign_state = curr_csi->curr_assign_state;
				m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, curr_csi, AVND_CKPT_COMP_CSI_PRV_ASSIGN_STATE);
				m_AVND_COMP_CSI_CURR_ASSIGN_STATE_SET(curr_csi, AVND_COMP_CSI_ASSIGN_STATE_UNASSIGNED);
				m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, curr_csi, AVND_CKPT_COMP_CSI_CURR_ASSIGN_STATE);
			}	/* for */
		}		/* for */
	}

	/* pick up all the csis belonging to the comps specified in the param-list */
	for (curr_param = param; curr_param; curr_param = curr_param->next) {
		/* get the comp */
		curr_comp = m_AVND_COMPDB_REC_GET(cb->compdb, curr_param->comp_name);
		if (!curr_comp || (curr_comp->su != su)) {
			rc = NCSCC_RC_FAILURE;
			goto done;
		}
		curr_comp->assigned_flag = false;
	}

	/* pick up all the csis belonging to the comps specified in the param-list */
	for (curr_param = param; curr_param; curr_param = curr_param->next) {
		/* get the comp */
		curr_comp = m_AVND_COMPDB_REC_GET(cb->compdb, curr_param->comp_name);
		if (!curr_comp || (curr_comp->su != su)) {
			rc = NCSCC_RC_FAILURE;
			goto done;
		}
		if (false == curr_comp->assigned_flag) {
			/* modify all the csi-records */
			for (curr_csi = m_AVND_CSI_REC_FROM_COMP_DLL_NODE_GET(m_NCS_DBLIST_FIND_FIRST(&curr_comp->csi_list));
					curr_csi;
					curr_csi = m_AVND_CSI_REC_FROM_COMP_DLL_NODE_GET(m_NCS_DBLIST_FIND_NEXT(&curr_csi->comp_dll_node)))
			{
				/* update the assignment related parameters */
				curr_csi->act_comp_name = curr_param->active_comp_name;
				curr_csi->trans_desc = curr_param->active_comp_dsc;
				curr_csi->standby_rank = curr_param->stdby_rank;

				/* store the prv assign-state & update the new assign-state */
				curr_csi->prv_assign_state = curr_csi->curr_assign_state;
				m_AVND_COMP_CSI_CURR_ASSIGN_STATE_SET(curr_csi, AVND_COMP_CSI_ASSIGN_STATE_UNASSIGNED);
				m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, curr_csi, AVND_CKPT_CSI_REC);
			}		/* for */
			curr_comp->assigned_flag = true;
		}
	}			/* for */

 done:
	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
  Name          : avnd_su_si_rec_del
 
  Description   : This routine deletes a su-si relationship record. It 
                  traverses the entire csi-list and deletes each comp-csi 
                  relationship record.
 
  Arguments     : cb          - ptr to AvND control block
                  su_name - ptr to the su-name
                  si_name - ptr to the si-name
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None
******************************************************************************/
uint32_t avnd_su_si_rec_del(AVND_CB *cb, SaNameT *su_name, SaNameT *si_name)
{
	AVND_SU *su = 0;
	AVND_SU_SI_REC *si_rec = 0;
	uint32_t rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER2("'%s' : '%s'", su_name->value, si_name->value);

	/* get the su record */
	su = m_AVND_SUDB_REC_GET(cb->sudb, *su_name);
	if (!su) {
		rc = AVND_ERR_NO_SU;
		goto err;
	}

	/* get the si record */
	si_rec = avnd_su_si_rec_get(cb, su_name, si_name);
	if (!si_rec) {
		rc = AVND_ERR_NO_SI;
		goto err;
	}

	/*
	 * Delete the csi-list.
	 */
	rc = avnd_su_si_csi_del(cb, su, si_rec);
	if (NCSCC_RC_SUCCESS != rc)
		goto err;

	/*
	 * Detach from the si-list (maintained by su).
	 */
	rc = m_AVND_SUDB_REC_SI_REM(*su, *si_rec);
	if (NCSCC_RC_SUCCESS != rc)
		goto err;

	/* remove from global SI list */
	(void) ncs_db_link_list_delink(&cb->si_list, &si_rec->cb_dll_node);

	TRACE_1("SU-SI record deleted, SU= %s : SI=%s",su_name->value,si_name->value);

	/* free the memory */
	delete si_rec;

	return rc;

 err:
	LOG_CR("SU-SI record deletion failed, SU= %s : SI=%s",su_name->value,si_name->value);
	return rc;
}

/****************************************************************************
  Name          : avnd_su_si_del
 
  Description   : This routine traverses the entire si-list and deletes each
                  record.
 
  Arguments     : cb          - ptr to AvND control block
                  su_name - ptr to the su-name (n/w order)
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None
******************************************************************************/
uint32_t avnd_su_si_del(AVND_CB *cb, SaNameT *su_name)
{
	AVND_SU *su = 0;
	AVND_SU_SI_REC *si_rec = 0;
	uint32_t rc = NCSCC_RC_SUCCESS;

	TRACE_ENTER2("'%s'", su_name->value);

	/* get the su record */
	su = m_AVND_SUDB_REC_GET(cb->sudb, *su_name);
	if (!su) {
		rc = AVND_ERR_NO_SU;
		goto err;
	}

	/* scan & delete each si record */
	while (0 != (si_rec = (AVND_SU_SI_REC *)m_NCS_DBLIST_FIND_FIRST(&su->si_list))) {
		m_AVND_SEND_CKPT_UPDT_ASYNC_RMV(cb, si_rec, AVND_CKPT_SU_SI_REC);
		rc = avnd_su_si_rec_del(cb, su_name, &si_rec->name);
		if (NCSCC_RC_SUCCESS != rc)
			goto err;
	}

 err:
	TRACE_LEAVE2("%u", rc);
	return rc;
}

/****************************************************************************
  Name          : avnd_su_si_csi_del
 
  Description   : This routine traverses the each record in the csi-list 
                  (maintained by si) & deletes them.
 
  Arguments     : cb      - ptr to AvND control block
                  su      - ptr to the AvND SU
                  si_rec  - ptr to the SI record
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None
******************************************************************************/
uint32_t avnd_su_si_csi_del(AVND_CB *cb, AVND_SU *su, AVND_SU_SI_REC *si_rec)
{
	AVND_COMP_CSI_REC *csi_rec = 0;
	uint32_t rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER2("'%s' : '%s'", su->name.value, si_rec->name.value);

	/* scan & delete each csi record */
	while (0 != (csi_rec = (AVND_COMP_CSI_REC *)m_NCS_DBLIST_FIND_FIRST(&si_rec->csi_list))) {
		rc = avnd_su_si_csi_rec_del(cb, si_rec->su, si_rec, csi_rec);
		if (NCSCC_RC_SUCCESS != rc)
			goto err;

	}
	TRACE_LEAVE2("%u", rc);
	return rc;

 err:
	TRACE_LEAVE2("%u", rc);
	return rc;
}

/****************************************************************************
  Name          : avnd_su_si_csi_rec_del
 
  Description   : This routine deletes a comp-csi relationship record.
 
  Arguments     : cb      - ptr to AvND control block
                  su      - ptr to the AvND SU
                  si_rec  - ptr to the SI record
                  csi_rec - ptr to the CSI record
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None
******************************************************************************/
uint32_t avnd_su_si_csi_rec_del(AVND_CB *cb, AVND_SU *su, AVND_SU_SI_REC *si_rec, AVND_COMP_CSI_REC *csi_rec)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER2("'%s' : '%s' : '%s'", su->name.value, si_rec->name.value, csi_rec->name.value);

	/* remove from the comp-csi list */
	rc = m_AVND_COMPDB_REC_CSI_REM(*(csi_rec->comp), *csi_rec);
	if (NCSCC_RC_SUCCESS != rc)
		goto err;

	/* if all csi's are removed & the pres state is un-instantiated for
	 * a  npi comp, its time to mark it as healthy 
	 */
	if (m_AVND_COMP_IS_FAILED(csi_rec->comp) &&
	    !m_AVND_COMP_TYPE_IS_PREINSTANTIABLE(csi_rec->comp) &&
	    m_AVND_COMP_PRES_STATE_IS_UNINSTANTIATED(csi_rec->comp) && csi_rec->comp->csi_list.n_nodes == 0) {
		m_AVND_COMP_FAILED_RESET(csi_rec->comp);
		m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, csi_rec->comp, AVND_CKPT_COMP_FLAG_CHANGE);
	}

	/* remove from the si-csi list */
	rc = m_AVND_SU_SI_CSI_REC_REM(*si_rec, *csi_rec);
	if (NCSCC_RC_SUCCESS != rc)
		goto err;

	/* 
	 * Free the memory alloced for this record.
	 */
	// free the csi attributes
	// use of free() is required as it was
	// malloc'ed (eg. in avsv_edp_susi_asgn())
	free(csi_rec->attrs.list);
	
	/* free the pg list TBD */
	TRACE_1("Comp-CSI record deletion success, Comp=%s : CSI=%s",csi_rec->comp->name.value,csi_rec->name.value);

	/* finally free this record */
	delete csi_rec;

	TRACE_LEAVE();
	return rc;

 err:
	LOG_CR("Comp-CSI record deletion failed, Comp=%s : CSI=%s",csi_rec->comp->name.value,csi_rec->name.value);
	return rc;
}

/****************************************************************************
  Name          : avnd_su_si_rec_get
 
  Description   : This routine gets the su-si relationship record from the
                  si-list (maintained on su).
 
  Arguments     : cb          - ptr to AvND control block
                  su_name - ptr to the su-name (n/w order)
                  si_name - ptr to the si-name (n/w order)
 
  Return Values : ptr to the su-si record (if any)
 
  Notes         : None
******************************************************************************/
AVND_SU_SI_REC *avnd_su_si_rec_get(AVND_CB *cb, const SaNameT *su_name, const SaNameT *si_name)
{
	AVND_SU_SI_REC *si_rec = 0;
	AVND_SU *su = 0;

	TRACE_ENTER2("'%s' : '%s'", su_name->value, si_name->value);
	/* get the su record */
	su = m_AVND_SUDB_REC_GET(cb->sudb, *su_name);
	if (!su)
		goto done;

	/* get the si record */
	si_rec = (AVND_SU_SI_REC *)ncs_db_link_list_find(&su->si_list, (uint8_t *)si_name);

 done:
	TRACE_LEAVE();
	return si_rec;
}

/****************************************************************************
  Name          : avnd_su_siq_rec_add
 
  Description   : This routine buffers the susi assign message parameters in 
                  the susi queue.
 
  Arguments     : cb    - ptr to AvND control block
                  su    - ptr to the AvND SU
                  param - ptr to the SI parameters
                  rc    - ptr to the operation result
 
  Return Values : ptr to the si queue record
 
  Notes         : None
******************************************************************************/
AVND_SU_SIQ_REC *avnd_su_siq_rec_add(AVND_CB *cb, AVND_SU *su, AVND_SU_SI_PARAM *param, uint32_t *rc)
{
	AVND_SU_SIQ_REC *siq = 0;

	*rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER2("'%s'", su->name.value);

	/* alloc the siq rec */
	siq = new AVND_SU_SIQ_REC();

	/* Add to the siq (maintained by su) */
	m_AVND_SUDB_REC_SIQ_ADD(*su, *siq, *rc);
	if (NCSCC_RC_SUCCESS != *rc) {
		*rc = AVND_ERR_DLL;
		goto err;
	}

	/* update the param */
	siq->info = *param;

	/* memory transferred to the siq-rec.. nullify it in param */
	param->list = 0;

	TRACE_LEAVE();
	return siq;

 err:
	if (siq)
		delete siq;

	TRACE_LEAVE();
	return 0;
}

/****************************************************************************
  Name          : avnd_su_siq_rec_del
 
  Description   : This routine deletes the buffered susi assign message from 
                  the susi queue.
 
  Arguments     : cb  - ptr to AvND control block
                  su  - ptr to the AvND SU
                  siq - ptr to the si queue rec
 
  Return Values : None.
 
  Notes         : None.
******************************************************************************/
void avnd_su_siq_rec_del(AVND_CB *cb, AVND_SU *su, AVND_SU_SIQ_REC *siq)
{
	AVSV_SUSI_ASGN *curr = 0;
	TRACE_ENTER2("'%s'", su->name.value);

	/* delete the comp-csi info */
	while ((curr = siq->info.list) != 0) {
		siq->info.list = curr->next;
		// AVSV_ATTR_NAME_VAL variables
		// are malloc'ed, use free()
		free(curr->attrs.list);
		
		// use of free() is required as it was
		// malloc'ed in avsv_edp_susi_asgn()
		free(curr);
	}

	/* free the rec */
	delete siq;

	TRACE_LEAVE();
	return;
}

/****************************************************************************
  Name          : avnd_mbcsv_su_si_csi_rec_del
 
  Description   : This routine is a wrapper of avnd_su_si_csi_rec_del.
 
  Arguments     : cb      - ptr to AvND control block
                  su      - ptr to the AvND SU
                  si_rec  - ptr to the SI record
                  csi_rec - ptr to the CSI record
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None
******************************************************************************/
uint32_t avnd_mbcsv_su_si_csi_rec_del(AVND_CB *cb, AVND_SU *su, AVND_SU_SI_REC *si_rec, AVND_COMP_CSI_REC *csi_rec)
{
	return (avnd_su_si_csi_rec_del(cb, su, si_rec, csi_rec));
}

/****************************************************************************
  Name          : avnd_mbcsv_su_si_csi_rec_add
 
  Description   : This routine is a wrapper of avnd_su_si_csi_rec_add. 
 
  Arguments     : cb     - ptr to AvND control block
                  su     - ptr to the AvND SU
                  si_rec - ptr to the SI record
                  param  - ptr to the CSI parameters
                  rc     - ptr to the operation result
 
  Return Values : ptr to the comp-csi relationship record
 
  Notes         : None
******************************************************************************/
AVND_COMP_CSI_REC *avnd_mbcsv_su_si_csi_rec_add(AVND_CB *cb,
						AVND_SU *su,
						AVND_SU_SI_REC *si_rec, AVND_COMP_CSI_PARAM *param, uint32_t *rc)
{
	return (avnd_su_si_csi_rec_add(cb, su, si_rec, param, rc));
}
