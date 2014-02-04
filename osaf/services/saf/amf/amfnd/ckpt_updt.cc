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

  DESCRIPTION: This module contain all the supporting functions for the
               checkpointing operation. 

..............................................................................

  FUNCTIONS INCLUDED in this module:

  
******************************************************************************
*/

/*
 * Module Inclusion Control...
 */

#include "avnd.h"


extern AVND_COMP_HC_REC *avnd_comp_hc_rec_add(AVND_CB *, AVND_COMP *, AVSV_AMF_HC_START_PARAM *, MDS_DEST *);
/****************************************************************************\
 * Function: avnd_ckpt_add_rmv_updt_su_data
 *
 * Purpose:  Add new SU entry if action is ADD, remove SU from the tree if 
 *           action is to remove and update data if request is to update.
 *
 * Input: cb  - CB pointer.
 *        su - Decoded structur.
 *        action - ADD/RMV/UPDT
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
uint32_t avnd_ckpt_add_rmv_updt_su_data(AVND_CB *cb, AVND_SU *su, NCS_MBCSV_ACT_TYPE action)
{
	uint32_t status = NCSCC_RC_SUCCESS;
	AVND_SU *su_ptr = NULL;
	AVSV_SU_INFO_MSG su_info;
	uint32_t rc = NCSCC_RC_SUCCESS;

	su_ptr = m_AVND_SUDB_REC_GET(cb->sudb, su->name);


	switch (action) {
	case NCS_MBCSV_ACT_ADD:
		{
			if ((NULL == su_ptr) && (true == su->su_is_external)) {
				su_info.name = su->name;
				su_info.comp_restart_prob = su->comp_restart_prob;
				su_info.comp_restart_max = su->comp_restart_max;
				su_info.su_restart_prob = su->su_restart_prob;
				su_info.su_restart_max = su->su_restart_max;
				su_info.su_is_external = su->su_is_external;
				su_info.next = NULL;
				su_info.is_ncs = false;	/*External component is not part of NCS */
				su_ptr = avnd_sudb_rec_add(cb, &su_info, &rc);
				if (NULL == su_ptr) {
					return NCSCC_RC_FAILURE;
				}
			} else {
				return NCSCC_RC_FAILURE;
			}
			/* Don't put break here, let the other parameters get updated. */
		}
	case NCS_MBCSV_ACT_UPDATE:
		{
			if ((NULL != su_ptr) && (true == su->su_is_external)) {
				/*
				 * Update all the data. Except SU name which would have got
				 * updated with ADD.
				 */
				/* Fill the other parameters. */
				su_ptr->flag = su->flag;
				su_ptr->su_err_esc_level = su->su_err_esc_level;
				su_ptr->comp_restart_prob = su->comp_restart_prob;
				su_ptr->comp_restart_max = su->comp_restart_max;
				su_ptr->su_restart_prob = su->su_restart_prob;
				su_ptr->su_restart_max = su->su_restart_max;
				su_ptr->comp_restart_cnt = su->comp_restart_cnt;
				su_ptr->su_restart_cnt = su->su_restart_cnt;
				su_ptr->oper = su->oper;
				su_ptr->pres = su->pres;
				su_ptr->su_err_esc_tmr.is_active = su->su_err_esc_tmr.is_active;
				su_ptr->si_active_cnt = su->si_active_cnt;
				su_ptr->si_standby_cnt = su->si_standby_cnt;
			} else {
				return NCSCC_RC_FAILURE;
			}
		}
		break;

	case NCS_MBCSV_ACT_RMV:
		{
			if (NULL != su_ptr) {
				status = avnd_sudb_rec_del(cb, &su_ptr->name);
				if (NCSCC_RC_SUCCESS != status) {
					return NCSCC_RC_FAILURE;
				}
			} else {
				return NCSCC_RC_FAILURE;
			}
		}
		break;

	default:
		/* Log error */
		return NCSCC_RC_FAILURE;
	}

	return status;
}

/****************************************************************************\
 * Function: avnd_ckpt_add_rmv_updt_su_si_rec
 *
 * Purpose:  Add new SU_SI_REC entry if action is ADD, remove SU_SI_REC from 
 *           the tree if action is to remove and update data if request is to 
 *           update.
 *
 * Input: cb  - CB pointer.
 *        si - Decoded structur.
 *        action - ADD/RMV/UPDT
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
uint32_t avnd_ckpt_add_rmv_updt_su_si_rec(AVND_CB *cb, AVND_SU_SI_REC *su_si_ckpt, NCS_MBCSV_ACT_TYPE action)
{
	uint32_t status = NCSCC_RC_SUCCESS;
	AVND_SU_SI_REC *su_si_rec_ptr = NULL;
	AVND_SU_SI_PARAM info;
	AVND_SU *su_ptr = NULL;
	uint32_t rc = NCSCC_RC_SUCCESS;
	AVND_COMP_CSI_REC *csi_rec = NULL;

	su_si_rec_ptr = avnd_su_si_rec_get(cb, &su_si_ckpt->su_name, &su_si_ckpt->name);

	switch (action) {
	case NCS_MBCSV_ACT_ADD:
		{
			if (NULL == su_si_rec_ptr) {
				memset(&info, 0, sizeof(AVND_SU_SI_PARAM));
				info.su_name = su_si_ckpt->su_name;
				info.si_name = su_si_ckpt->name;
				su_ptr = m_AVND_SUDB_REC_GET(cb->sudb, su_si_ckpt->su_name);
				if (NULL == su_ptr) {
					return NCSCC_RC_FAILURE;
				}
				su_si_rec_ptr = avnd_su_si_rec_add(cb, su_ptr, &info, &rc);
				if (NULL == su_si_rec_ptr) {
					return NCSCC_RC_FAILURE;
				}
			} else {
				return NCSCC_RC_FAILURE;
			}
		}
		/* 
		 * Don't break...continue updating data of this SU SI Relation 
		 * as done during normal update request.
		 */

	case NCS_MBCSV_ACT_UPDATE:
		{
			if (NULL != su_si_rec_ptr) {
				su_si_rec_ptr->curr_state = su_si_ckpt->curr_state;
				su_si_rec_ptr->prv_state = su_si_ckpt->prv_state;
				su_si_rec_ptr->curr_assign_state = su_si_ckpt->curr_assign_state;
				su_si_rec_ptr->prv_assign_state = su_si_ckpt->prv_assign_state;
			} else {
				return NCSCC_RC_FAILURE;
			}
		}
		break;

	case NCS_MBCSV_ACT_RMV:
		{
			if (NULL != su_si_rec_ptr) {
				su_ptr = m_AVND_SUDB_REC_GET(cb->sudb, su_si_ckpt->su_name);
				if (NULL == su_ptr) {
					return NCSCC_RC_FAILURE;
				}
				/* Delete the CSI first. */
				while (0 != (csi_rec =
					     (AVND_COMP_CSI_REC *)m_NCS_DBLIST_FIND_FIRST(&su_si_rec_ptr->csi_list))) {
					rc = m_AVND_COMPDB_REC_CSI_REM(*(csi_rec->comp), *csi_rec);
					if (rc != NCSCC_RC_SUCCESS) {
						return NCSCC_RC_FAILURE;
					}
					/* remove from the si-csi list */
					rc = m_AVND_SU_SI_CSI_REC_REM(*su_si_rec_ptr, *csi_rec);
					if (rc != NCSCC_RC_SUCCESS) {
						return NCSCC_RC_FAILURE;
					}
					// free the csi attributes
					// AVSV_ATTR_NAME_VAL variables
					// are malloc'ed, use free()
					free(csi_rec->attrs.list);
					
					/* finally free this record */
					delete csi_rec;
				}	/* while ( 0 != csi_rec) */

				/* Now delete the SU_SI record */
				rc = m_AVND_SUDB_REC_SI_REM(*su_ptr, *su_si_rec_ptr);
				if (rc != NCSCC_RC_SUCCESS) {
					return NCSCC_RC_FAILURE;
				}
				delete su_si_rec_ptr;
			} /*if (NULL != su_si_rec_ptr) */
			else {
				return NCSCC_RC_FAILURE;
			}
		}
		break;

	default:
		/* Log error */
		return NCSCC_RC_FAILURE;
	}

	return status;
}

/****************************************************************************\
 * Function: avnd_ckpt_add_rmv_updt_su_siq_rec
 *
 * Purpose:  Add new AVND_SU_SIQ_REC entry if action is ADD, remove AVND_SU_SIQ_REC from 
 *           the tree if action is to remove and update data if request is to 
 *           update.
 *
 * Input: cb  - CB pointer.
 *        si - Decoded structur.
 *        action - ADD/RMV/UPDT
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
uint32_t avnd_ckpt_add_rmv_updt_su_siq_rec(AVND_CB *cb, AVND_SU_SIQ_REC *su_siq_ckpt, NCS_MBCSV_ACT_TYPE action)
{
	uint32_t status = NCSCC_RC_SUCCESS;
	AVND_SU_SIQ_REC *siq_ptr = NULL;
	AVND_SU *su_ptr;
	uint32_t rc = NCSCC_RC_SUCCESS;

	switch (action) {
	case NCS_MBCSV_ACT_ADD:
		{
			if (NULL != su_siq_ckpt) {
				su_ptr = m_AVND_SUDB_REC_GET(cb->sudb, su_siq_ckpt->info.su_name);
				if (NULL == su_ptr) {
					return NCSCC_RC_FAILURE;
				}

				siq_ptr = avnd_su_siq_rec_add(cb, su_ptr, &su_siq_ckpt->info, &rc);
				if (NULL == siq_ptr) {
					return NCSCC_RC_FAILURE;
				}
			} else {
				return NCSCC_RC_FAILURE;
			}
			break;
		}

	case NCS_MBCSV_ACT_RMV:
		{
			su_ptr = m_AVND_SUDB_REC_GET(cb->sudb, su_siq_ckpt->info.su_name);
			if (NULL == su_ptr) {
				return NCSCC_RC_FAILURE;
			}

			siq_ptr = (AVND_SU_SIQ_REC *)m_NCS_DBLIST_FIND_LAST(&su_ptr->siq);

			if (NULL != siq_ptr) {
				/* unlink the buffered msg from the queue */
				ncs_db_link_list_delink(&su_ptr->siq, &siq_ptr->su_dll_node);

				avnd_su_siq_rec_del(cb, su_ptr, siq_ptr);
			} else {
				return NCSCC_RC_FAILURE;
			}
		}
		break;

	default:
		/* Log error */
		return NCSCC_RC_FAILURE;
	}

	return status;
}

/****************************************************************************\
 * Function: avnd_ckpt_add_rmv_updt_comp_data
 *
 * Purpose:  Add new COMP entry if action is ADD, remove COMP from the tree if 
 *           action is to remove and update data if request is to update.
 *
 * Input: cb  - CB pointer.
 *        comp - Decoded structur.
 *        action - ADD/RMV/UPDT
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
uint32_t avnd_ckpt_add_rmv_updt_comp_data(AVND_CB *cb, AVND_COMP *comp, NCS_MBCSV_ACT_TYPE action)
{
	uint32_t status = NCSCC_RC_SUCCESS;
	AVND_COMP *comp_ptr = NULL;
	AVND_COMP *pxy_comp = NULL;
	AVND_COMP_PARAM comp_info;
	SaNameT temp_comp_name;

	/* Add/Updt/Rmv might be coming from any one of the following:
	   1. an internode, which is a proxy for an external component.
	   2. An external component, local to Ctrl.

	   In case 1. => We need to search the component in EXT_COMPDB_REC.
	   In case 2. => Search in COMPDB_REC.
	 */
	if (m_AVND_COMP_TYPE_IS_INTER_NODE(comp)) {
		comp_ptr = m_AVND_INT_EXT_COMPDB_REC_GET(cb->internode_avail_comp_db, comp->name);
	} else
		comp_ptr = m_AVND_COMPDB_REC_GET(cb->compdb, comp->name);


	switch (action) {
	case NCS_MBCSV_ACT_ADD:
		{
			if (m_AVND_COMP_TYPE_IS_INTER_NODE(comp)) {
				if (NULL == comp_ptr) {
					comp_ptr = avnd_internode_comp_add(&(cb->internode_avail_comp_db),
									   &(comp->name), comp->node_id, &status,
									   m_AVND_PROXY_IS_FOR_EXT_COMP(comp),
									   m_AVND_COMP_TYPE_IS_PROXY(comp));
					/* For internode too, we are expecting some update, so continue
					   to mbcsv update section. */
				} else {
					return NCSCC_RC_FAILURE;
				}
			}

			if (!m_AVND_COMP_TYPE_IS_INTER_NODE(comp)) {
				if (NULL == comp_ptr) {
					/* This is external component i.e. proxied component. */
					memset(&comp_info, 0, sizeof(AVND_COMP_PARAM));
					/* We care for the name when adding, the rest of the param
					   will be updated in NCS_MBCSV_ACT_UPDATE switch case. */
					comp_info.name = comp->name;
					comp_info.inst_level = comp->inst_level;
					/* If comp_info.comp_restart is true, then in avnd_compdb_rec_add,
					   it marks comp->is_restart_en as false and vice-versa. So, to
					   make data sync at both ACT and STDBY, we need to send reverse
					   of what has come. */
					comp_info.comp_restart = ((comp->is_restart_en == true) ? false : true);
					comp_info.cap = comp->cap;
					comp_ptr = avnd_compdb_rec_add(cb, &comp_info, &status);
					if (NULL == comp_ptr) {
						return NCSCC_RC_FAILURE;
					}
					comp_ptr->node_id = comp->node_id;
					comp_ptr->comp_type = comp->comp_type;
					memcpy(&(comp_ptr->mds_ctxt), &(comp->mds_ctxt), sizeof(MDS_SYNC_SND_CTXT));
					comp_ptr->reg_resp_pending = comp->reg_resp_pending;
					comp_ptr->proxy_comp_name = comp->proxy_comp_name;

					/* This is for Cold Sync. Read NOTE inside the function
					   avnd_encode_cold_sync_rsp_comp_config() 

					   Check whether the proxied component has proxy component or it
					   is in orphan state and we need not add it to proxy, adding it
					   in our data base is enough. */

					memset(&temp_comp_name, 0, sizeof(SaNameT));
					if (memcmp(&temp_comp_name, &comp->proxy_comp_name, sizeof(SaNameT)) != 0) {
						/* This has proxy name associated with it. So, add it in the
						   pxied_list of proxy component. */
						pxy_comp =
						    m_AVND_INT_EXT_COMPDB_REC_GET(cb->internode_avail_comp_db,
										  comp->proxy_comp_name);
						if (NULL == pxy_comp) {
							return NCSCC_RC_FAILURE;
						}
						status = avnd_comp_proxied_add(cb, comp_ptr, pxy_comp, false);
						if (NCSCC_RC_SUCCESS != status) {
							return NCSCC_RC_FAILURE;
						}

					}	/* memcmp() */
				} /* if (NULL == comp_ptr) */
				else {
					return NCSCC_RC_FAILURE;
				}
			}	/*if(!m_AVND_COMP_TYPE_IS_INTER_NODE(comp)) */
		}

		/* 
		 * Don't break...continue updating data to this COMP 
		 * as done during normal update request.
		 */

	case NCS_MBCSV_ACT_UPDATE:
		{
			if (NULL != comp_ptr) {
				/*
				 * Update all the data. Except COMP name which would have got
				 * updated with ADD.
				 */
				comp_ptr->flag = comp->flag;

				comp_ptr->clc_info.cmds[AVND_COMP_CLC_CMD_TYPE_INSTANTIATE - 1].len =
				    comp->clc_info.cmds[AVND_COMP_CLC_CMD_TYPE_INSTANTIATE - 1].len;

				memcpy(&comp_ptr->clc_info.cmds[AVND_COMP_CLC_CMD_TYPE_INSTANTIATE - 1].cmd,
				       &comp->clc_info.cmds[AVND_COMP_CLC_CMD_TYPE_INSTANTIATE - 1].cmd,
				       sizeof(SAAMF_CLC_LEN));

				comp_ptr->clc_info.cmds[AVND_COMP_CLC_CMD_TYPE_INSTANTIATE - 1].timeout =
				    comp->clc_info.cmds[AVND_COMP_CLC_CMD_TYPE_INSTANTIATE - 1].timeout;

				comp_ptr->clc_info.cmds[AVND_COMP_CLC_CMD_TYPE_TERMINATE - 1].len =
				    comp->clc_info.cmds[AVND_COMP_CLC_CMD_TYPE_TERMINATE - 1].len;

				memcpy(&comp_ptr->clc_info.cmds[AVND_COMP_CLC_CMD_TYPE_TERMINATE - 1].cmd,
				       &comp->clc_info.cmds[AVND_COMP_CLC_CMD_TYPE_TERMINATE - 1].cmd,
				       sizeof(SAAMF_CLC_LEN));

				comp_ptr->clc_info.cmds[AVND_COMP_CLC_CMD_TYPE_TERMINATE - 1].timeout =
				    comp->clc_info.cmds[AVND_COMP_CLC_CMD_TYPE_TERMINATE - 1].timeout;

				comp_ptr->clc_info.inst_retry_max = comp->clc_info.inst_retry_max;
				comp_ptr->clc_info.inst_retry_cnt = comp->clc_info.inst_retry_cnt;
				comp_ptr->clc_info.exec_cmd = comp->clc_info.exec_cmd;
				comp_ptr->clc_info.cmd_exec_ctxt = comp->clc_info.cmd_exec_ctxt;
				comp_ptr->clc_info.inst_cmd_ts = comp->clc_info.inst_cmd_ts;
				comp_ptr->clc_info.clc_reg_tmr.is_active = comp->clc_info.clc_reg_tmr.is_active;
				comp_ptr->clc_info.clc_reg_tmr.type = comp->clc_info.clc_reg_tmr.type;
				comp_ptr->clc_info.inst_code_rcvd = comp->clc_info.inst_code_rcvd;

				comp_ptr->reg_hdl = comp->reg_hdl;
				comp_ptr->reg_dest = comp->reg_dest;
				comp_ptr->oper = comp->oper;
				comp_ptr->pres = comp->pres;

				comp_ptr->term_cbk_timeout = comp->term_cbk_timeout;
				comp_ptr->csi_set_cbk_timeout = comp->csi_set_cbk_timeout;
				comp_ptr->quies_complete_cbk_timeout = comp->quies_complete_cbk_timeout;
				comp_ptr->csi_rmv_cbk_timeout = comp->csi_rmv_cbk_timeout;
				comp_ptr->pxied_inst_cbk_timeout = comp->pxied_inst_cbk_timeout;
				comp_ptr->pxied_clean_cbk_timeout = comp->pxied_clean_cbk_timeout;

				comp_ptr->err_info.src = comp->err_info.src;
				comp_ptr->err_info.def_rec = comp->err_info.def_rec;
				comp_ptr->err_info.detect_time = comp->err_info.detect_time;
				comp_ptr->err_info.restart_cnt = comp->err_info.restart_cnt;

				comp_ptr->pend_evt = comp->pend_evt;
				comp_ptr->orph_tmr.is_active = comp->orph_tmr.is_active;
				comp_ptr->proxy_comp_name = comp->proxy_comp_name;
			} else {
				return NCSCC_RC_FAILURE;
			}
		}
		break;

	case NCS_MBCSV_ACT_RMV:
		{
			if (m_AVND_COMP_TYPE_IS_INTER_NODE(comp)) {
				if (NULL != comp_ptr)
					status = avnd_internode_comp_del(cb,
									 &(cb->internode_avail_comp_db),
									 &(comp->name));
				else {
					return NCSCC_RC_FAILURE;
				}
			}

			if (!m_AVND_COMP_TYPE_IS_INTER_NODE(comp)) {
				if (NULL != comp_ptr) {
					status = avnd_compdb_rec_del(cb, &comp->name);
				} else {
					return NCSCC_RC_FAILURE;
				}

				if (NCSCC_RC_SUCCESS != status) {
					return NCSCC_RC_FAILURE;
				}
			}
		}
		break;

	default:
		/* Log error */
		return NCSCC_RC_FAILURE;
	}

	return status;
}

/****************************************************************************\
 * Function: avnd_ckpt_add_rmv_updt_csi_data
 *
 * Purpose:  Add new CSI entry if action is ADD, remove CSI from the tree if 
 *           action is to remove and update data if request is to update.
 *
 * Input: cb  - CB pointer.
 *        csi - Decoded structur.
 *        action - ADD/RMV/UPDT
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
uint32_t avnd_ckpt_add_rmv_updt_csi_data(AVND_CB *cb, AVND_COMP_CSI_REC *csi, NCS_MBCSV_ACT_TYPE action)
{
	uint32_t status = NCSCC_RC_SUCCESS;
	AVND_COMP_CSI_REC *csi_ptr = NULL;
	AVND_SU_SI_REC *su_si_rec = NULL;
	AVSV_SUSI_ASGN csi_param;
	AVND_SU *su = NULL;
	uint32_t rc = NCSCC_RC_SUCCESS;

	su_si_rec = avnd_su_si_rec_get(cb, &csi->su_name, &csi->si_name);
	csi_ptr = avnd_compdb_csi_rec_get(cb, &csi->comp_name, &csi->name);

	switch (action) {
	case NCS_MBCSV_ACT_ADD:
		{
			if ((NULL == csi_ptr) && (NULL != su_si_rec)) {
				memset(&csi_param, 0, sizeof(AVSV_SUSI_ASGN));
				csi_param.comp_name = csi->comp_name;
				csi_param.csi_name = csi->name;
				csi_param.csi_rank = csi->rank;
				csi_param.attrs.number = csi->attrs.number;
				csi_param.attrs.list = csi->attrs.list;
				su = m_AVND_SUDB_REC_GET(cb->sudb, csi->su_name);
				if (NULL == su) {
					return NCSCC_RC_FAILURE;
				}
				csi_ptr = avnd_mbcsv_su_si_csi_rec_add(cb, su, su_si_rec, &csi_param, &rc);
				if (NULL == csi_ptr) {
					return NCSCC_RC_FAILURE;
				}
			} else {
				return NCSCC_RC_FAILURE;
			}

		}

		/* 
		 * Don't break...continue updating data to this CSI 
		 * as done during normal update request.
		 */

	case NCS_MBCSV_ACT_UPDATE:
		{
			if (NULL != csi_ptr) {
				csi_ptr->act_comp_name = csi->act_comp_name;
				csi_ptr->trans_desc = csi->trans_desc;
				csi_ptr->standby_rank = csi->standby_rank;
				csi_ptr->curr_assign_state = csi->curr_assign_state;
				csi_ptr->prv_assign_state = csi->prv_assign_state;
			} else {
			}
		}
		break;

	case NCS_MBCSV_ACT_RMV:
		{
			/* CSI is getting deleted as part of SU_SI_REC delete, so this part
			   of the code will not be called at this point of time. */
			if ((NULL != csi_ptr) && (NULL != su_si_rec)) {
				su = m_AVND_SUDB_REC_GET(cb->sudb, csi->su_name);
				if (NULL == su) {
					return NCSCC_RC_FAILURE;
				}
				rc = avnd_mbcsv_su_si_csi_rec_del(cb, su, su_si_rec, csi_ptr);
				if (NCSCC_RC_SUCCESS != rc) {
					return NCSCC_RC_FAILURE;
				}
			} else {
				return NCSCC_RC_FAILURE;
			}
		}
		break;

	default:
		/* Log error */
		return NCSCC_RC_FAILURE;
	}

	return status;
}

/****************************************************************************\
 * Function: avnd_ckpt_add_rmv_updt_hlt_data
 *
 * Purpose:  Add new HLT entry if action is ADD, remove HLT from the tree if 
 *           action is to remove and update data if request is to update.
 *
 * Input: cb  - CB pointer.
 *        csi - Decoded structur.
 *        action - ADD/RMV/UPDT
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
uint32_t avnd_ckpt_add_rmv_updt_hlt_data(AVND_CB *cb, AVND_HC *hlt, NCS_MBCSV_ACT_TYPE action)
{
	uint32_t status = NCSCC_RC_SUCCESS;
	AVND_HC *hlt_ptr = NULL;
	AVSV_HLT_INFO_MSG hc_info;
	uint32_t rc = NCSCC_RC_SUCCESS;

	hlt_ptr = avnd_hcdb_rec_get(cb, &hlt->key);


	switch (action) {
	case NCS_MBCSV_ACT_ADD:
		{
			if (NULL == hlt_ptr) {
				memset(&hc_info, 0, sizeof(AVSV_HLT_INFO_MSG));
				memcpy(&hc_info.name, &hlt->key, sizeof(AVSV_HLT_KEY));
				hc_info.period = hlt->period;
				hc_info.max_duration = hlt->max_dur;
				hc_info.is_ext = hlt->is_ext;
				hlt_ptr = avnd_hcdb_rec_add(cb, &hc_info, &rc);
				if (!hlt_ptr) {
					return NCSCC_RC_FAILURE;
				}
			} else {
				return NCSCC_RC_FAILURE;
			}

		}

	case NCS_MBCSV_ACT_UPDATE:
		{
			if (NULL == hlt_ptr) {
				return NCSCC_RC_FAILURE;
			} else {
				hlt_ptr->period = hlt->period;
				hlt_ptr->max_dur = hlt->max_dur;
			}
			break;
		}
	case NCS_MBCSV_ACT_RMV:
		{
			if (NULL == hlt_ptr) {
				return NCSCC_RC_FAILURE;
			} else {
				avnd_hcdb_rec_del(cb, &hlt_ptr->key);
			}
			break;
		}

	default:
		return NCSCC_RC_FAILURE;
	}

	return status;
}

/****************************************************************************\
 * Function: avnd_ckpt_add_rmv_updt_comp_hlt_rec
 *
 * Purpose:  Add new HLT entry if action is ADD, remove HLT from the tree if 
 *           action is to remove and update data if request is to update.
 *
 * Input: cb  - CB pointer.
 *        csi - Decoded structur.
 *        action - ADD/RMV/UPDT
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
uint32_t avnd_ckpt_add_rmv_updt_comp_hlt_rec(AVND_CB *cb, AVND_COMP_HC_REC *hlt, NCS_MBCSV_ACT_TYPE action)
{
	uint32_t status = NCSCC_RC_SUCCESS;
	AVND_COMP_HC_REC *hc_rec = NULL;
	AVSV_AMF_HC_START_PARAM hc_info;
	AVND_COMP *comp = NULL;
	AVND_COMP_HC_REC tmp_hc_rec;

	memset(&tmp_hc_rec, '\0', sizeof(AVND_COMP_HC_REC));
	tmp_hc_rec.key = hlt->key;
	tmp_hc_rec.req_hdl = hlt->req_hdl;
	/* determine if this healthcheck is already active */
	comp = m_AVND_COMPDB_REC_GET(cb->compdb, hlt->comp_name);
	if (comp == NULL) {
		return NCSCC_RC_FAILURE;
	}
	hc_rec = m_AVND_COMPDB_REC_HC_GET(*comp, tmp_hc_rec);

	switch (action) {
	case NCS_MBCSV_ACT_ADD:
		{
			if ((NULL == hc_rec) && (NULL != comp)) {
				memset(&hc_info, 0, sizeof(AVSV_AMF_HC_START_PARAM));
				hc_info.comp_name = hlt->comp_name;
				hc_info.hc_key = hlt->key;
				hc_info.hdl = hlt->req_hdl;
				hc_rec = avnd_mbcsv_comp_hc_rec_add(cb, comp, &hc_info, &hlt->dest);
				if (!hc_rec) {
					return NCSCC_RC_FAILURE;
				}
			} else {
				return NCSCC_RC_FAILURE;
			}

		}

	case NCS_MBCSV_ACT_UPDATE:
		{
			if ((NULL != hc_rec) && (NULL != comp)) {
				hc_rec->inv = hlt->inv;
				hc_rec->rec_rcvr = hlt->rec_rcvr;
				hc_rec->period = hlt->period;
				hc_rec->max_dur = hlt->max_dur;
				hc_rec->status = hlt->status;
				hc_rec->req_hdl = hlt->req_hdl;
				hc_rec->dest = hlt->dest;
				hc_rec->tmr.is_active = hlt->tmr.is_active;
			} else {
				return NCSCC_RC_FAILURE;
			}
		}
		break;

	case NCS_MBCSV_ACT_RMV:
		{
			if ((NULL != hc_rec) && (NULL != comp)) {
				avnd_mbcsv_comp_hc_rec_del(cb, comp, hc_rec);
			} else {
				return NCSCC_RC_FAILURE;
			}
		}
		break;

	default:
		return NCSCC_RC_FAILURE;
	}

	return status;
}

/****************************************************************************\
 * Function: avnd_ckpt_add_rmv_updt_comp_cbk_rec
 *
 * Purpose:  Add new HLT entry if action is ADD, remove HLT from the tree if 
 *           action is to remove and update data if request is to update.
 *
 * Input: cb  - CB pointer.
 *        csi - Decoded structur.
 *        action - ADD/RMV/UPDT
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 * 
\**************************************************************************/
uint32_t avnd_ckpt_add_rmv_updt_comp_cbk_rec(AVND_CB *cb, AVND_COMP_CBK *cbk, NCS_MBCSV_ACT_TYPE action)
{
	uint32_t status = NCSCC_RC_SUCCESS;
	AVND_COMP_CBK *cbk_ptr = NULL;
	AVND_COMP *comp = NULL;

	comp = m_AVND_COMPDB_REC_GET(cb->compdb, cbk->comp_name);


	if (NULL == comp) {
		return NCSCC_RC_FAILURE;
	}

	/* Check whether callback exists or not. */
	/* get the matching entry from the cbk list */
	m_AVND_COMP_CBQ_INV_GET(comp, cbk->opq_hdl, cbk_ptr);

	/* it can be a responce from a proxied, see any proxied comp are there */
	if (!cbk_ptr && comp->pxied_list.n_nodes != 0) {
		AVND_COMP_PXIED_REC *rec = 0;

		/* parse thru all proxied comp, look in the list of inv handle of each of them */
		for (rec = (AVND_COMP_PXIED_REC *)m_NCS_DBLIST_FIND_FIRST(&comp->pxied_list); rec;
		     rec = (AVND_COMP_PXIED_REC *)m_NCS_DBLIST_FIND_NEXT(&rec->comp_dll_node)) {
			m_AVND_COMP_CBQ_INV_GET(rec->pxied_comp, cbk->opq_hdl, cbk_ptr);
			if (cbk_ptr)
				break;
		}
	}

	switch (action) {
	case NCS_MBCSV_ACT_ADD:
		{
			if (NULL == cbk_ptr) {
				/* Here another opq_hdl is created by the handle manager, which is
				   not the same as ACT AvND. So, opq_hdl and cbk_ptr->cbk_info->inv
				   are different here. */
				cbk_ptr = avnd_comp_cbq_rec_add(cb, comp, cbk->cbk_info, &cbk->dest, cbk->timeout);
				if (NULL == cbk_ptr) {
					return NCSCC_RC_FAILURE;
				}
				cbk_ptr->red_opq_hdl = cbk_ptr->opq_hdl;
			} else {
				return NCSCC_RC_FAILURE;
			}
		}

	case NCS_MBCSV_ACT_UPDATE:
		{
			if (NULL != cbk_ptr) {
				/*  Not sure to have these fields
				   cbk_ptr->opq_hdl = cbk->opq_hdl;
				   cbk_ptr->orig_opq_hdl = cbk->orig_opq_hdl; */
				cbk_ptr->timeout = cbk->timeout;
				cbk_ptr->dest = cbk->dest;
				cbk_ptr->resp_tmr.is_active = cbk->resp_tmr.is_active;
				cbk_ptr->cbk_info->hdl = cbk->cbk_info->hdl;
			} else {
				return NCSCC_RC_FAILURE;
			}
		}
		break;

	case NCS_MBCSV_ACT_RMV:
		{
			uint32_t found = 0;
			if (NULL != cbk_ptr) {
				m_AVND_COMP_CBQ_REC_POP(comp, cbk_ptr, found);
				if (found)
					avnd_comp_cbq_rec_del(cb, comp, cbk_ptr);
			} else {
				return NCSCC_RC_FAILURE;
			}
		}
		break;

	default:
		/* Log error */
		return NCSCC_RC_FAILURE;
	}

	return status;
}

/****************************************************************************\
 * Function: avnd_ext_comp_data_clean_up
 *
 * Purpose:  Function is used for cleaning the entire AVND data structure.
 *           Will be called from the standby AVND on failure of warm sync.
 *
 * Input: cb  - CB pointer.
 *        avnd_shut_down - If true then don't reset avnd_async_updt_cnt and
 *                         synced_reo_type.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES:
 *
 *
\**************************************************************************/
uint32_t avnd_ext_comp_data_clean_up(AVND_CB *cb, bool avnd_shut_down)
{
	uint32_t rc = NCSCC_RC_SUCCESS;


  /****************** Destroy healthcheck db *********************************/
	{
		AVND_HC *hc_config = NULL;
		AVSV_HLT_KEY hc_key;

		memset(&hc_key, 0, sizeof(AVSV_HLT_KEY));

		hc_config = (AVND_HC *)ncs_patricia_tree_getnext(&cb->hcdb, (uint8_t *)&hc_key);
		while (hc_config != 0) {
			memcpy(&hc_key, &hc_config->key, sizeof(AVSV_HLT_KEY));
			if (true == hc_config->is_ext) {
				rc = avnd_hcdb_rec_del(cb, &hc_config->key);
				if (rc != NCSCC_RC_SUCCESS) {
					return NCSCC_RC_FAILURE;
				}

			}	/* if(true == hc_config->is_ext) */
			hc_config = (AVND_HC *)ncs_patricia_tree_getnext(&cb->hcdb, (uint8_t *)&hc_key);
		}		/* while(hc_config != 0) */
	}
  /****************** Destroy healthcheck db ends here***********************/

	/* Reset the async count */
	if (false == avnd_shut_down)
		memset(&cb->avnd_async_updt_cnt, 0, sizeof(AVND_ASYNC_UPDT_CNT));

  /****************** Destroy Internode db starts here***********************/
	{
		AVND_COMP *comp = NULL;
		SaNameT comp_name;
		AVND_COMP_PXIED_REC *rec = 0, *curr_rec = 0;

		comp = (AVND_COMP *)ncs_patricia_tree_getnext(&cb->internode_avail_comp_db, (uint8_t *)0);
		while (comp != 0) {
			comp_name = comp->name;

			if (m_AVND_PROXY_IS_FOR_EXT_COMP(comp)) {
				rec = (AVND_COMP_PXIED_REC *)m_NCS_DBLIST_FIND_FIRST(&comp->pxied_list);
				while (rec) {
					curr_rec = rec;
					rec = (AVND_COMP_PXIED_REC *)m_NCS_DBLIST_FIND_NEXT(&rec->comp_dll_node);
					{
						AVND_COMP_PXIED_REC *del_rec = 0;
						AVND_COMP *del_comp = 0, *del_proxy = 0;

						del_comp = curr_rec->pxied_comp;
						del_proxy = comp;
						del_rec =
						    (AVND_COMP_PXIED_REC *)
						    ncs_db_link_list_remove(&del_proxy->pxied_list,
									    (uint8_t *)&del_comp->name);
						delete del_rec;
					}
				}
				rc = avnd_internode_comp_del(cb, &(cb->internode_avail_comp_db), &(comp_name));
			}
			comp = (AVND_COMP *)ncs_patricia_tree_getnext(&cb->internode_avail_comp_db,
								      (uint8_t *)&comp_name);
		}		/* while */
	}
  /****************** Destroy Internode db ends here ***********************/

  /**************** Delete SU_SU and CSI Record ******************************/
	{
		AVND_SU *su = NULL;
		AVND_SU_SI_REC *curr_su_si = NULL;
		AVND_COMP_CSI_REC *csi_rec = NULL;
		SaNameT su_name;

		su = (AVND_SU *)ncs_patricia_tree_getnext(&cb->sudb, (uint8_t *)0);
		while (su != 0) {
			su_name = su->name;
			if (true == su->su_is_external) {
				while (0 != (curr_su_si = (AVND_SU_SI_REC *)m_NCS_DBLIST_FIND_FIRST(&su->si_list))) {
					/*  Delete the CSI Record attached to this SU_SI record. */
					/* scan & delete each csi record */
					while (0 != (csi_rec =
						     (AVND_COMP_CSI_REC *)m_NCS_DBLIST_FIND_FIRST(&curr_su_si->
                                                                                                  csi_list))) {
						rc = m_AVND_COMPDB_REC_CSI_REM(*(csi_rec->comp), *csi_rec);
						if (rc != NCSCC_RC_SUCCESS) {
							return NCSCC_RC_FAILURE;
						}
						/* remove from the si-csi list */
						rc = m_AVND_SU_SI_CSI_REC_REM(*curr_su_si, *csi_rec);
						if (rc != NCSCC_RC_SUCCESS) {
							return NCSCC_RC_FAILURE;
						}
						// free the csi attributes
						// AVSV_ATTR_NAME_VAL variables
						// are malloc'ed, use free()
						free(csi_rec->attrs.list);
						
						/* finally free this record */
						delete csi_rec;
					}	/* while ( 0 != csi_rec) */
					/* Now delete the SU_SI record */
					rc = m_AVND_SUDB_REC_SI_REM(*su, *curr_su_si);
					if (rc != NCSCC_RC_SUCCESS) {
						return NCSCC_RC_FAILURE;
					}
					delete curr_su_si;
				}	/* while(0 != curr_su_si) */
			}	/* if(true == su->su_is_external) */
			su = (AVND_SU *)ncs_patricia_tree_getnext(&cb->sudb, (uint8_t *)&su_name);
		}		/* while(su != 0) */
	}

  /**************** Delete SU_SU and CSI Record ends here ********************/

  /**************** Delete SU SIQ Record ************************************/
	{
		AVND_SU *su = NULL;
		AVND_SU_SIQ_REC *su_siq = NULL;
		SaNameT su_name;

		su = (AVND_SU *)ncs_patricia_tree_getnext(&cb->sudb, (uint8_t *)0);
		while (su != 0) {
			su_name = su->name;
			if (true == su->su_is_external) {
				while (0 != (su_siq = (AVND_SU_SIQ_REC *)m_NCS_DBLIST_FIND_FIRST(&su->siq))) {
					/* unlink the buffered msg from the queue */
					ncs_db_link_list_delink(&su->siq, &su_siq->su_dll_node);
					/* delete the buffered msg */
					avnd_su_siq_rec_del(cb, su, su_siq);

					if (rc != NCSCC_RC_SUCCESS) {
						return NCSCC_RC_FAILURE;
					}
				}	/* while(0 != su_siq) */
			}	/* if(true == su->su_is_external) */
			su = (AVND_SU *)ncs_patricia_tree_getnext(&cb->sudb, (uint8_t *)&su_name);
		}		/* while(su != 0) */

	}
  /**************** Delete SU SIQ Record ends here ***************************/

  /**************** Delete Component, its HC and CBK Record *****************/
	{
		AVND_COMP *comp = NULL;
		SaNameT comp_name;

		comp = (AVND_COMP *)ncs_patricia_tree_getnext(&cb->compdb, (uint8_t *)0);
		while (comp != 0) {
			comp_name = comp->name;
			if (true == comp->su->su_is_external) {
				rc = avnd_compdb_rec_del(cb, &comp->name);

				if (rc != NCSCC_RC_SUCCESS) {
					return NCSCC_RC_FAILURE;
				}
			}	/* if(true == comp->su->su_is_external) */
			comp = (AVND_COMP *)
			    ncs_patricia_tree_getnext(&cb->compdb, (uint8_t *)&comp_name);
		}		/* while(comp != 0) */
	}
  /**************** Delete Component, its HC and CBK Record ends here *******/

  /**************** In the end delete SU Record  *****************/
	{
		AVND_SU *su = NULL;
		SaNameT su_name;

		su = (AVND_SU *)ncs_patricia_tree_getnext(&cb->sudb, (uint8_t *)0);
		while (su != 0) {
			su_name = su->name;
			if (true == su->su_is_external) {
				rc = avnd_sudb_rec_del(cb, &su->name);

				if (rc != NCSCC_RC_SUCCESS) {
					return NCSCC_RC_FAILURE;
				}
			}	/* if(true == su->su_is_external) */
			su = (AVND_SU *)
			    ncs_patricia_tree_getnext(&cb->sudb, (uint8_t *)&su_name);
		}		/* while(su != 0) */
	}
	/* Reset the cold sync done counter */
	if (false == avnd_shut_down)
		cb->synced_reo_type = 0;

	return rc;
}
