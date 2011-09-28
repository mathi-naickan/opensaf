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

  This file contains routines for SU operation.
..............................................................................

  FUNCTIONS INCLUDED in this module:

  
******************************************************************************
*/

#include <logtrace.h>
#include <avnd.h>

static uns32 avnd_avd_su_update_on_fover(AVND_CB *cb, AVSV_D2N_REG_SU_MSG_INFO *info);

/****************************************************************************
  Name          : avnd_evt_avd_reg_su_msg
 
  Description   : This routine processes the SU addition message from AvD. SU
                  deletion is handled as a part of operation request message 
                  processing.
 
  Arguments     : cb  - ptr to the AvND control block
                  evt - ptr to the AvND event
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
uns32 avnd_evt_avd_reg_su_evh(AVND_CB *cb, AVND_EVT *evt)
{
	AVSV_D2N_REG_SU_MSG_INFO *info = 0;
	AVSV_SU_INFO_MSG *su_info = 0;
	AVND_SU *su = 0;
	uns32 rc = NCSCC_RC_SUCCESS;

	TRACE_ENTER();

	/* dont process unless AvD is up */
	if (!m_AVND_CB_IS_AVD_UP(cb))
		goto done;

	info = &evt->info.avd->msg_info.d2n_reg_su;

	avnd_msgid_assert(info->msg_id);
	cb->rcv_msg_id = info->msg_id;

	/* 
	 * Check whether SU updates are received after fail-over then
	 * call a separate processing function.
	 */
	if (info->msg_on_fover) {
		rc = avnd_avd_su_update_on_fover(cb, info);
		goto done;
	}

	/* scan the su list & add each su to su-db */
	for (su_info = info->su_list; su_info; su = 0, su_info = su_info->next) {
		su = avnd_sudb_rec_add(cb, su_info, &rc);
		if (!su)
			break;

		m_AVND_SEND_CKPT_UPDT_ASYNC_ADD(cb, su, AVND_CKPT_SU_CONFIG);

		/* add components belonging to this SU */
		if (avnd_comp_config_get_su(su) != NCSCC_RC_SUCCESS) {
			m_AVND_SU_REG_FAILED_SET(su);
			/* Will transition to instantiation-failed when instantiated */
			LOG_ER("%s: FAILED", __FUNCTION__);
			rc = NCSCC_RC_FAILURE;
			break;
		}
	}

	/*** send the response to AvD ***/
	rc = avnd_di_reg_su_rsp_snd(cb, &info->su_list->name, rc);

done:
	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
  Name          : avnd_avd_su_update_on_fover
 
  Description   : This routine processes the SU update message sent by AVD 
                  on fail-over.
 
  Arguments     : cb  - ptr to the AvND control block
                  info - ptr to the AvND event
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
static uns32 avnd_avd_su_update_on_fover(AVND_CB *cb, AVSV_D2N_REG_SU_MSG_INFO *info)
{
	AVSV_SU_INFO_MSG *su_info = 0;
	AVND_SU *su = 0;
	AVND_COMP *comp = 0;
	uns32 rc = NCSCC_RC_SUCCESS;
	SaNameT su_name;

	TRACE_ENTER();

	/* scan the su list & add each su to su-db */
	for (su_info = info->su_list; su_info; su = 0, su_info = su_info->next) {
		if (NULL == (su = m_AVND_SUDB_REC_GET(cb->sudb, su_info->name))) {
			/* SU is not present so add it */
			su = avnd_sudb_rec_add(cb, su_info, &rc);
			if (!su) {
				avnd_di_reg_su_rsp_snd(cb, &su_info->name, rc);
				/* Log Error, we are not able to update at this time */
				LOG_EM("%s, %u, SU update failed",__FUNCTION__,__LINE__);
				return rc;
			}

			m_AVND_SEND_CKPT_UPDT_ASYNC_ADD(cb, su, AVND_CKPT_SU_CONFIG);
			avnd_di_reg_su_rsp_snd(cb, &su_info->name, rc);
		} else {
			/* SU present, so update its contents */
			/* update error recovery escalation parameters */
			su->comp_restart_prob = su_info->comp_restart_prob;
			su->comp_restart_max = su_info->comp_restart_max;
			su->su_restart_prob = su_info->su_restart_prob;
			su->su_restart_max = su_info->su_restart_max;
			su->is_ncs = su_info->is_ncs;
			m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, su, AVND_CKPT_SU_CONFIG);
		}

		su->avd_updt_flag = TRUE;
	}

	/*
	 * Walk through the entire SU table, and remove SU for which 
	 * updates are not received in the message.
	 */
	memset(&su_name, 0, sizeof(SaNameT));
	while (NULL != (su = (AVND_SU *)ncs_patricia_tree_getnext(&cb->sudb, (uns8 *)&su_name))) {
		su_name = su->name;

		if (FALSE == su->avd_updt_flag) {
			/* First walk entire comp list of this SU and delete all the
			 * component records which are there in the list.
			 */
			while ((comp = m_AVND_COMP_FROM_SU_DLL_NODE_GET(m_NCS_DBLIST_FIND_FIRST(&su->comp_list)))) {
				/* delete the record */
				m_AVND_SEND_CKPT_UPDT_ASYNC_RMV(cb, comp, AVND_CKPT_COMP_CONFIG);
				rc = avnd_compdb_rec_del(cb, &comp->name);
				if (NCSCC_RC_SUCCESS != rc) {
					/* Log error */
					LOG_EM("%s, %u, SU update failed",__FUNCTION__,__LINE__);
					goto err;
				}
			}

			/* Delete SU from the list */
			/* delete the record */
			m_AVND_SEND_CKPT_UPDT_ASYNC_RMV(cb, su, AVND_CKPT_SU_CONFIG);
			rc = avnd_sudb_rec_del(cb, &su->name);
			if (NCSCC_RC_SUCCESS != rc) {
				/* Log error */
				LOG_EM("%s, %u, SU update failed",__FUNCTION__,__LINE__);
				goto err;
			}

		} else
			su->avd_updt_flag = FALSE;
	}

 err:
	TRACE_LEAVE2("%u", rc);
	return rc;
}

/****************************************************************************
  Name          : avnd_evt_avd_info_su_si_assign_msg
 
  Description   : This routine processes the SU-SI assignment message from 
                  AvD. It buffers the message if already some assignment is on.
                  Else it initiates SI addition, deletion or removal.
 
  Arguments     : cb  - ptr to the AvND control block
                  evt - ptr to the AvND event
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
uns32 avnd_evt_avd_info_su_si_assign_evh(AVND_CB *cb, AVND_EVT *evt)
{
	AVSV_D2N_INFO_SU_SI_ASSIGN_MSG_INFO *info = &evt->info.avd->msg_info.d2n_su_si_assign;
	AVND_SU_SIQ_REC *siq = 0;
	AVND_SU *su = 0;
	uns32 rc = NCSCC_RC_SUCCESS;

	TRACE_ENTER2("'%s'", info->su_name.value);

	/* get the su */
	su = m_AVND_SUDB_REC_GET(cb->sudb, info->su_name);
	if (!su) {
		LOG_ER("'%s' record not found", info->su_name.value);
		goto done;
	}

	avnd_msgid_assert(info->msg_id);
	cb->rcv_msg_id = info->msg_id;

	/* buffer the msg (if no assignment / removal is on) */
	siq = avnd_su_siq_rec_buf(cb, su, info);
	if (siq) {
		/* Send async update for SIQ Record for external SU only. */
		if (TRUE == su->su_is_external) {
			m_AVND_SEND_CKPT_UPDT_ASYNC_ADD(cb, &(siq->info), AVND_CKPT_SIQ_REC);
		}
		goto done;
	}

	/* the msg isn't buffered, process it */
	rc = avnd_su_si_msg_prc(cb, su, info);

done:
	TRACE_LEAVE2("%u", rc);
	return rc;
}

/****************************************************************************
  Name          : avnd_evt_tmr_su_err_esc
 
  Description   : This routine handles the the expiry of the 'su error 
                  escalation' timer. It indicates the end of the comp/su 
                  restart probation period for the SU.
 
  Arguments     : cb  - ptr to the AvND control block
                  evt - ptr to the AvND event
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
uns32 avnd_evt_tmr_su_err_esc_evh(AVND_CB *cb, AVND_EVT *evt)
{
	AVND_SU *su;
	uns32 rc = NCSCC_RC_SUCCESS;

	TRACE_ENTER();

	/* retrieve avnd cb */
	if (0 == (su = (AVND_SU *)ncshm_take_hdl(NCS_SERVICE_ID_AVND, (uns32)evt->info.tmr.opq_hdl))) {
		LOG_CR("Unable to retrieve handle");
		goto done;
	}

	TRACE("'%s'", su->name.value);

	if (NCSCC_RC_SUCCESS == m_AVND_CHECK_FOR_STDBY_FOR_EXT_COMP(cb, su->su_is_external))
		goto done;

	m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, su, AVND_CKPT_SU_ERR_ESC_TMR);

	switch (su->su_err_esc_level) {
	case AVND_ERR_ESC_LEVEL_0:
		su->comp_restart_cnt = 0;
		su->su_err_esc_level = AVND_ERR_ESC_LEVEL_0;
		m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, su, AVND_CKPT_SU_COMP_RESTART_CNT);
		break;
	case AVND_ERR_ESC_LEVEL_1:
		su->su_restart_cnt = 0;
		su->su_err_esc_level = AVND_ERR_ESC_LEVEL_0;
		cb->node_err_esc_level = AVND_ERR_ESC_LEVEL_0;
		m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, su, AVND_CKPT_SU_RESTART_CNT);
		break;
	case AVND_ERR_ESC_LEVEL_2:
		cb->su_failover_cnt = 0;
		su->su_err_esc_level = AVND_ERR_ESC_LEVEL_0;
		cb->node_err_esc_level = AVND_ERR_ESC_LEVEL_0;
		break;
	default:
		assert(0);
	}
	m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, su, AVND_CKPT_SU_ERR_ESC_LEVEL);

done:
	if (su)
		ncshm_give_hdl((uns32)evt->info.tmr.opq_hdl);
	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
  Name          : avnd_su_si_reassign
 
  Description   : This routine reassigns all the SIs in the su-si list. It is
                  invoked when the SU reinstantiates as a part of SU restart
                  recovery.
 
  Arguments     : cb - ptr to the AvND control block
                  su - ptr to the comp
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
uns32 avnd_su_si_reassign(AVND_CB *cb, AVND_SU *su)
{
	AVND_SU_SI_REC *si = 0;
	uns32 rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER2("'%s'", su->name.value);

	/* scan the su-si list & reassign the sis */
	for (si = (AVND_SU_SI_REC *)m_NCS_DBLIST_FIND_FIRST(&su->si_list);
	     si; si = (AVND_SU_SI_REC *)m_NCS_DBLIST_FIND_NEXT(&si->su_dll_node)) {
		rc = avnd_su_si_assign(cb, su, si);
		if (NCSCC_RC_SUCCESS != rc)
			break;
	}			/* for */

	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
  Name          : avnd_su_curr_info_del
 
  Description   : This routine deletes the dynamic info associated with this 
                  SU. This includes deleting the dynamic info for all it's 
                  components. If the SU is marked failed, the error 
                  escalation parameters are retained.
 
  Arguments     : cb - ptr to the AvND control block
                  su - ptr to the su
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : SIs associated with this SU are not deleted.
******************************************************************************/
uns32 avnd_su_curr_info_del(AVND_CB *cb, AVND_SU *su)
{
	AVND_COMP *comp = 0;
	uns32 rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER2("'%s'", su->name.value);

	/* reset err-esc param & oper state (if su is healthy) */
	if (!m_AVND_SU_IS_FAILED(su)) {
		su->su_err_esc_level = AVND_ERR_ESC_LEVEL_0;
		su->comp_restart_cnt = 0;
		su->su_restart_cnt = 0;
		m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, su, AVND_CKPT_SU_CONFIG);
		/* stop su_err_esc_tmr TBD Later */

		/* disable the oper state (if pi su) */
		if (m_AVND_SU_IS_PREINSTANTIABLE(su)) {
			m_AVND_SU_OPER_STATE_SET(su, SA_AMF_OPERATIONAL_DISABLED);
			m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, su, AVND_CKPT_SU_OPER_STATE);
		}
	}

	/* scan & delete the current info store in each component */
	for (comp = m_AVND_COMP_FROM_SU_DLL_NODE_GET(m_NCS_DBLIST_FIND_FIRST(&su->comp_list));
	     comp; comp = m_AVND_COMP_FROM_SU_DLL_NODE_GET(m_NCS_DBLIST_FIND_NEXT(&comp->su_dll_node))) {
		rc = avnd_comp_curr_info_del(cb, comp);
		if (NCSCC_RC_SUCCESS != rc)
			goto done;
	}

 done:
	TRACE_LEAVE2("%u", rc);
	return rc;
}

/**
 * Process SU admin operation request from director
 *
 * @param cb
 * @param evt
 */
uns32 avnd_evt_su_admin_op_req(AVND_CB *cb, AVND_EVT *evt)
{
	AVSV_D2N_ADMIN_OP_REQ_MSG_INFO *info = &evt->info.avd->msg_info.d2n_admin_op_req_info;
	AVND_SU *su;
	uns32 rc = NCSCC_RC_SUCCESS;

	TRACE_ENTER2("%s op=%u", info->dn.value, info->oper_id);

	avnd_msgid_assert(info->msg_id);
	cb->rcv_msg_id = info->msg_id;

	su = m_AVND_SUDB_REC_GET(cb->sudb, info->dn);
	assert(su != NULL);

	switch(info->oper_id) {
	default:
		LOG_NO("%s: unsupported adm op %u", __FUNCTION__, info->oper_id);
		rc = NCSCC_RC_FAILURE;
		break;
	}

	TRACE_LEAVE();
	return rc;
}

