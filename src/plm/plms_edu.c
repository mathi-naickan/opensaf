
/*      -*- OpenSAF  -*-

 *
 * (C) Copyright 2010 The OpenSAF Foundation
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

 /*****************************************************************************/

 /************************** Include Files ***********************************/
#include "plms.h"
#include "plms_evt.h"
#include "plms_mbcsv.h"
#include "base/ncs_edu_pub.h"
#include "base/ncs_saf_edu.h"

/****************************************************************************
 * Name          : plms_edp_agent_lib_req
 *
 * Description   : This function is an EDU program for encoding/decoding
 *                 plm agent library request.
 *
 * Arguments     : EDU_HDL - pointer to edu handle,
 *                 EDU_TKN - internal edu token to help encode/decode,
 *                 POINTER to the structure to encode/decode from/to,
 *                 data length specifying number of structures,
 *                 EDU_BUF_ENV - pointer to buffer for encoding/decoding.
 *                 op - operation type being encode/decode.
 *                 EDU_ERR - out param to indicate errors in processing.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : None.
 *****************************************************************************/
		
uint32_t plms_edp_agent_lib_req(EDU_HDL *edu_hdl, EDU_TKN *edu_tkn,NCSCONTEXT ptr, 	uint32_t *ptr_data_len, EDU_BUF_ENV *buf_env,EDP_OP_TYPE op,EDU_ERR *o_err)
{
	 uint32_t rc = NCSCC_RC_SUCCESS;
	 PLMS_AGENT_LIB_REQ *struct_ptr = NULL, **d_ptr;

	 EDU_INST_SET plms_agent_lib_req_rules[] = {
		 {EDU_START, plms_edp_agent_lib_req, 0, 0, 0, sizeof(PLMS_AGENT_LIB_REQ), 0, NULL},
		 {EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((PLMS_AGENT_LIB_REQ *)0)->lib_req_type, 0, NULL},
		 {EDU_EXEC, ncs_edp_uns64, 0, 0, 0, (long)&((PLMS_AGENT_LIB_REQ *)0)->plm_handle, 0, NULL},
		 {EDU_EXEC, ncs_edp_mds_dest, 0, 0, 0, (long)&((PLMS_AGENT_LIB_REQ *)0)->agent_mdest_id, 0, NULL},
		 {EDU_END, 0, 0, 0, 0, 0, 0, NULL},
	};

	if(op == EDP_OP_TYPE_ENC)
        {
                struct_ptr = (PLMS_AGENT_LIB_REQ *)ptr;
        }
        else if(op == EDP_OP_TYPE_DEC)
        {
                d_ptr = (PLMS_AGENT_LIB_REQ **)ptr;
                if(*d_ptr == NULL)
                {
                        *o_err = EDU_ERR_MEM_FAIL;
                        return NCSCC_RC_FAILURE;
                }
                memset(*d_ptr, '\0', sizeof(PLMS_AGENT_LIB_REQ));
                struct_ptr = *d_ptr;
	}
	else
	{
		struct_ptr = ptr;
										        }

	rc = m_NCS_EDU_RUN_RULES(edu_hdl, edu_tkn, plms_agent_lib_req_rules,
				struct_ptr, ptr_data_len, buf_env, op, o_err);
        return rc;
}																												
/****************************************************************************
 * Name          : plms_edp_agent_grp_op
 *
 * Description   : This function is an EDU program for encoding/decoding
 *                 plm agent group operation req.
 *
 * Arguments     : EDU_HDL - pointer to edu handle,
 *                 EDU_TKN - internal edu token to help encode/decode,
 *                 POINTER to the structure to encode/decode from/to,
 *                 data length specifying number of structures,
 *                 EDU_BUF_ENV - pointer to buffer for encoding/decoding.
 *                 op - operation type being encode/decode.
 *                 EDU_ERR - out param to indicate errors in processing.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : None.
 *****************************************************************************/
uint32_t plms_edp_agent_grp_op(EDU_HDL *edu_hdl, EDU_TKN *edu_tkn,NCSCONTEXT ptr, 	uint32_t *ptr_data_len, EDU_BUF_ENV *buf_env,EDP_OP_TYPE op,EDU_ERR *o_err)
{
	 uint32_t rc = NCSCC_RC_SUCCESS;
	 PLMS_AGENT_GRP_OP *struct_ptr = NULL, **d_ptr;

	 EDU_INST_SET plms_agent_grp_op_rules[] = {
		 {EDU_START, plms_edp_agent_grp_op, 0, 0, 0, sizeof(PLMS_AGENT_GRP_OP), 0, NULL},
		 {EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((PLMS_AGENT_GRP_OP *)0)->grp_evt_type, 0, NULL},
		 {EDU_EXEC, ncs_edp_uns64, 0, 0, 0, (long)&((PLMS_AGENT_GRP_OP *)0)->plm_handle, 0, NULL},
		 {EDU_EXEC, ncs_edp_uns64, 0, 0, 0, (long)&((PLMS_AGENT_GRP_OP *)0)->grp_handle, 0, NULL},
		 {EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((PLMS_AGENT_GRP_OP *)0)->entity_names_number, 0, NULL},
		 {EDU_EXEC, ncs_edp_sanamet, EDQ_VAR_LEN_DATA, ncs_edp_uns32, 0, (long)&((PLMS_AGENT_GRP_OP *)0)->entity_names,
		 (long)&((PLMS_AGENT_GRP_OP *)0)->entity_names_number, NULL},
		 {EDU_EXEC_EXT, NULL, NCS_SERVICE_ID_PLMS, NULL, 0, 
		 1, 0, NULL},
		 {EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((PLMS_AGENT_GRP_OP *)0)->grp_add_option, 0, NULL},
		 {EDU_END, 0, 0, 0, 0, 0, 0, NULL},
	};

	if(op == EDP_OP_TYPE_ENC)
        {
                struct_ptr = (PLMS_AGENT_GRP_OP *)ptr;
        }
        else if(op == EDP_OP_TYPE_DEC)
        {
                d_ptr = (PLMS_AGENT_GRP_OP **)ptr;
                if(*d_ptr == NULL)
                {
                        *o_err = EDU_ERR_MEM_FAIL;
                        return NCSCC_RC_FAILURE;
                }
                memset(*d_ptr, '\0', sizeof(PLMS_AGENT_GRP_OP));
                struct_ptr = *d_ptr;
	}
	else
	{
		struct_ptr = ptr;
										        }

	rc = m_NCS_EDU_RUN_RULES(edu_hdl, edu_tkn, plms_agent_grp_op_rules,
				struct_ptr, ptr_data_len, buf_env, op, o_err);
        return rc;
}																												
/****************************************************************************
 * Name          : plms_edp_readiness_status
 *
 * Description   : This function is an EDU program for encoding/decoding
 *                 plm readiness status.
 *
 * Arguments     : EDU_HDL - pointer to edu handle,
 *                 EDU_TKN - internal edu token to help encode/decode,
 *                 POINTER to the structure to encode/decode from/to,
 *                 data length specifying number of structures,
 *                 EDU_BUF_ENV - pointer to buffer for encoding/decoding.
 *                 op - operation type being encode/decode.
 *                 EDU_ERR - out param to indicate errors in processing.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : None.
 *****************************************************************************/
		
uint32_t plms_edp_readiness_status(EDU_HDL *edu_hdl, EDU_TKN *edu_tkn,NCSCONTEXT ptr,uint32_t *ptr_data_len, EDU_BUF_ENV *buf_env,EDP_OP_TYPE op,EDU_ERR *o_err)
{
	 uint32_t rc = NCSCC_RC_SUCCESS;
	 SaPlmReadinessStatusT *struct_ptr = NULL, **d_ptr;

	 EDU_INST_SET plms_readiness_status_rules[] = {
		 {EDU_START, plms_edp_readiness_status, 0, 0, 0, sizeof(SaPlmReadinessStatusT), 0, NULL},
		 {EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((SaPlmReadinessStatusT *)0)->readinessState, 0, NULL},
		 {EDU_EXEC, ncs_edp_uns64, 0, 0, 0, (long)&((SaPlmReadinessStatusT *)0)->readinessFlags, 0, NULL},
		 {EDU_END, 0, 0, 0, 0, 0, 0, NULL},
	};

	if(op == EDP_OP_TYPE_ENC)
        {
                struct_ptr = (SaPlmReadinessStatusT *)ptr;
        }
        else if(op == EDP_OP_TYPE_DEC)
        {
                d_ptr = (SaPlmReadinessStatusT **)ptr;
                if(*d_ptr == NULL)
                {
                        *o_err = EDU_ERR_MEM_FAIL;
                        return NCSCC_RC_FAILURE;
                }
                memset(*d_ptr, '\0', sizeof(SaPlmReadinessStatusT));
                struct_ptr = *d_ptr;
	}
	else
	{
		struct_ptr = ptr;
										        }

	rc = m_NCS_EDU_RUN_RULES(edu_hdl, edu_tkn, plms_readiness_status_rules,
				struct_ptr, ptr_data_len, buf_env, op, o_err);
        return rc;
}																												
/****************************************************************************
 * Name          : plms_edp_readiness_tracked_entity
 *
 * Description   : This function is an EDU program for encoding/decoding
 *                 plm readiness tracked entity.
 *
 * Arguments     : EDU_HDL - pointer to edu handle,
 *                 EDU_TKN - internal edu token to help encode/decode,
 *                 POINTER to the structure to encode/decode from/to,
 *                 data length specifying number of structures,
 *                 EDU_BUF_ENV - pointer to buffer for encoding/decoding.
 *                 op - operation type being encode/decode.
 *                 EDU_ERR - out param to indicate errors in processing.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : None.
 *****************************************************************************/
uint32_t plms_edp_readiness_tracked_entity(EDU_HDL *edu_hdl, EDU_TKN *edu_tkn,NCSCONTEXT ptr,uint32_t *ptr_data_len, EDU_BUF_ENV *buf_env,EDP_OP_TYPE op,EDU_ERR *o_err)
{
	 uint32_t rc = NCSCC_RC_SUCCESS;
	 SaPlmReadinessTrackedEntityT *struct_ptr = NULL, **d_ptr;

	 EDU_INST_SET plms_readiness_tracked_entity_rules[] = {
		 {EDU_START, plms_edp_readiness_tracked_entity, 0, 0, 0, sizeof(SaPlmReadinessTrackedEntityT), 0, NULL},
		 {EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((SaPlmReadinessTrackedEntityT *)0)->change, 0, NULL},
		 {EDU_EXEC, ncs_edp_sanamet, 0, 0, 0, (long)&((SaPlmReadinessTrackedEntityT *)0)->entityName, 0, NULL},
		 {EDU_EXEC, plms_edp_readiness_status, 0, 0, 0, (long)&((SaPlmReadinessTrackedEntityT *)0)->currentReadinessStatus, 0, NULL},
		 {EDU_EXEC, plms_edp_readiness_status, 0, 0, 0, (long)&((SaPlmReadinessTrackedEntityT *)0)->expectedReadinessStatus, 0, NULL},
		 {EDU_EXEC, ncs_edp_uns64, 0, 0, 0, (long)&((SaPlmReadinessTrackedEntityT *)0)->plmNotificationId, 0, NULL},
		 {EDU_END, 0, 0, 0, 0, 0, 0, NULL},
	};

	if(op == EDP_OP_TYPE_ENC)
        {
                struct_ptr = (SaPlmReadinessTrackedEntityT *)ptr;
        }
        else if(op == EDP_OP_TYPE_DEC)
        {
                d_ptr = (SaPlmReadinessTrackedEntityT **)ptr;
                if(*d_ptr == NULL)
                {
                        *o_err = EDU_ERR_MEM_FAIL;
                        return NCSCC_RC_FAILURE;
                }
                memset(*d_ptr, '\0', sizeof(SaPlmReadinessTrackedEntityT));
                struct_ptr = *d_ptr;
	}
	else
	{
		struct_ptr = ptr;
										        }

	rc = m_NCS_EDU_RUN_RULES(edu_hdl, edu_tkn, plms_readiness_tracked_entity_rules, struct_ptr, ptr_data_len, buf_env, op, o_err);
        return rc;
}																												
/****************************************************************************
 * Name          : plms_edp_readiness_tracked_entities
 *
 * Description   : This function is an EDU program for encoding/decoding
 *                 plm readiness tracked entities.
 *
 * Arguments     : EDU_HDL - pointer to edu handle,
 *                 EDU_TKN - internal edu token to help encode/decode,
 *                 POINTER to the structure to encode/decode from/to,
 *                 data length specifying number of structures,
 *                 EDU_BUF_ENV - pointer to buffer for encoding/decoding.
 *                 op - operation type being encode/decode.
 *                 EDU_ERR - out param to indicate errors in processing.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : None.
 *****************************************************************************/
uint32_t plms_edp_readiness_tracked_entities(EDU_HDL *edu_hdl, EDU_TKN *edu_tkn,NCSCONTEXT ptr,uint32_t *ptr_data_len, EDU_BUF_ENV *buf_env,EDP_OP_TYPE op,EDU_ERR *o_err)
{
	 uint32_t rc = NCSCC_RC_SUCCESS;
	 SaPlmReadinessTrackedEntitiesT *struct_ptr = NULL, **d_ptr;

	 EDU_INST_SET plms_readiness_tracked_entities_rules[] = {
		 {EDU_START, plms_edp_readiness_tracked_entities, 0, 0, 0, sizeof(SaPlmReadinessTrackedEntitiesT), 0, NULL},
		 {EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((SaPlmReadinessTrackedEntitiesT *)0)->numberOfEntities, 0, NULL},
		 {EDU_EXEC, plms_edp_readiness_tracked_entity, EDQ_VAR_LEN_DATA, ncs_edp_uns32, 0, (long)&((SaPlmReadinessTrackedEntitiesT *)0)->entities, (long)&((SaPlmReadinessTrackedEntitiesT *)0)->numberOfEntities, NULL},
		  {EDU_EXEC_EXT, NULL, NCS_SERVICE_ID_PLMS, NULL, 0,
		                   1, 0, NULL},
		 {EDU_END, 0, 0, 0, 0, 0, 0, NULL},
	};

	if(op == EDP_OP_TYPE_ENC)
        {
                struct_ptr = (SaPlmReadinessTrackedEntitiesT *)ptr;
        }
        else if(op == EDP_OP_TYPE_DEC)
        {
                d_ptr = (SaPlmReadinessTrackedEntitiesT **)ptr;
                if(*d_ptr == NULL)
                {
                        *o_err = EDU_ERR_MEM_FAIL;
                        return NCSCC_RC_FAILURE;
                }
                memset(*d_ptr, '\0', sizeof(SaPlmReadinessTrackedEntitiesT));
                struct_ptr = *d_ptr;
	}
	else
	{
		struct_ptr = ptr;
										        }

	rc = m_NCS_EDU_RUN_RULES(edu_hdl, edu_tkn, plms_readiness_tracked_entities_rules, struct_ptr, ptr_data_len, buf_env, op, o_err);
        return rc;
}																												
/****************************************************************************
 * Name          : plms_edp_agent_track_start
 *
 * Description   : This function is an EDU program for encoding/decoding
 *                 plm agent track start operation req.
 *
 * Arguments     : EDU_HDL - pointer to edu handle,
 *                 EDU_TKN - internal edu token to help encode/decode,
 *                 POINTER to the structure to encode/decode from/to,
 *                 data length specifying number of structures,
 *                 EDU_BUF_ENV - pointer to buffer for encoding/decoding.
 *                 op - operation type being encode/decode.
 *                 EDU_ERR - out param to indicate errors in processing.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : None.
 *****************************************************************************/
uint32_t plms_edp_agent_track_start(EDU_HDL *edu_hdl, EDU_TKN *edu_tkn,NCSCONTEXT ptr,uint32_t *ptr_data_len, EDU_BUF_ENV *buf_env,EDP_OP_TYPE op,EDU_ERR *o_err)
{
	 uint32_t rc = NCSCC_RC_SUCCESS;
	 PLMS_AGENT_TRACK_START *struct_ptr = NULL, **d_ptr;

	 EDU_INST_SET plms_agent_track_start_rules[] = {
		 {EDU_START, plms_edp_agent_track_start, 0, 0, 0, sizeof(PLMS_AGENT_TRACK_START), 0, NULL},
		 {EDU_EXEC, ncs_edp_uns8, 0, 0, 0, (long)&((PLMS_AGENT_TRACK_START *)0)->track_flags, 0, NULL},
		 {EDU_EXEC, ncs_edp_uns64, 0, 0, 0, (long)&((PLMS_AGENT_TRACK_START *)0)->track_cookie, 0, NULL},
		 {EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((PLMS_AGENT_TRACK_START *)0)->no_of_entities, 0, NULL},
		 {EDU_END, 0, 0, 0, 0, 0, 0, NULL},
	};

	if(op == EDP_OP_TYPE_ENC)
        {
                struct_ptr = (PLMS_AGENT_TRACK_START *)ptr;
        }
        else if(op == EDP_OP_TYPE_DEC)
        {
                d_ptr = (PLMS_AGENT_TRACK_START **)ptr;
                if(*d_ptr == NULL)
                {
                        *o_err = EDU_ERR_MEM_FAIL;
                        return NCSCC_RC_FAILURE;
                }
                memset(*d_ptr, '\0', sizeof(PLMS_AGENT_TRACK_START));
                struct_ptr = *d_ptr;
	}
	else
	{
		struct_ptr = ptr;
										        }

	rc = m_NCS_EDU_RUN_RULES(edu_hdl, edu_tkn, plms_agent_track_start_rules,
				struct_ptr, ptr_data_len, buf_env, op, o_err);
        return rc;
}																												
/****************************************************************************
 * Name          : plms_edp_ntf_correlation_ids
 *
 * Description   : This function is an EDU program for encoding/decoding
 *                 plm ntf correlations ids.
 *
 * Arguments     : EDU_HDL - pointer to edu handle,
 *                 EDU_TKN - internal edu token to help encode/decode,
 *                 POINTER to the structure to encode/decode from/to,
 *                 data length specifying number of structures,
 *                 EDU_BUF_ENV - pointer to buffer for encoding/decoding.
 *                 op - operation type being encode/decode.
 *                 EDU_ERR - out param to indicate errors in processing.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : None.
 *****************************************************************************/
uint32_t plms_edp_ntf_correlation_ids(EDU_HDL *edu_hdl, EDU_TKN *edu_tkn,NCSCONTEXT ptr,uint32_t *ptr_data_len, EDU_BUF_ENV *buf_env,EDP_OP_TYPE op,EDU_ERR *o_err)
{
	 uint32_t rc = NCSCC_RC_SUCCESS;
	 SaNtfCorrelationIdsT *struct_ptr = NULL, **d_ptr;

	 EDU_INST_SET plms_ntf_correlation_ids_rules[] = {
		 {EDU_START, plms_edp_ntf_correlation_ids, 0, 0, 0, sizeof(SaNtfCorrelationIdsT), 0, NULL},
		 {EDU_EXEC, ncs_edp_uns64, 0, 0, 0, (long)&((SaNtfCorrelationIdsT *)0)->rootCorrelationId, 0, NULL},
		 {EDU_EXEC, ncs_edp_uns64, 0, 0, 0, (long)&((SaNtfCorrelationIdsT *)0)->parentCorrelationId, 0, NULL},
		 {EDU_EXEC, ncs_edp_uns64, 0, 0, 0, (long)&((SaNtfCorrelationIdsT *)0)->notificationId, 0, NULL},
		 {EDU_END, 0, 0, 0, 0, 0, 0, NULL},
	};

	if(op == EDP_OP_TYPE_ENC)
        {
                struct_ptr = (SaNtfCorrelationIdsT *)ptr;
        }
        else if(op == EDP_OP_TYPE_DEC)
        {
                d_ptr = (SaNtfCorrelationIdsT **)ptr;
                if(*d_ptr == NULL)
                {
                        *o_err = EDU_ERR_MEM_FAIL;
                        return NCSCC_RC_FAILURE;
                }
                memset(*d_ptr, '\0', sizeof(SaNtfCorrelationIdsT));
                struct_ptr = *d_ptr;
	}
	else
	{
		struct_ptr = ptr;
										        }

	rc = m_NCS_EDU_RUN_RULES(edu_hdl, edu_tkn, plms_ntf_correlation_ids_rules, struct_ptr, ptr_data_len, buf_env, op, o_err);
        return rc;
}																												
/****************************************************************************
 * Name          : plms_edp_agent_readiness_impact
 *
 * Description   : This function is an EDU program for encoding/decoding
 *                 plm agent readiness impact.
 *
 * Arguments     : EDU_HDL - pointer to edu handle,
 *                 EDU_TKN - internal edu token to help encode/decode,
 *                 POINTER to the structure to encode/decode from/to,
 *                 data length specifying number of structures,
 *                 EDU_BUF_ENV - pointer to buffer for encoding/decoding.
 *                 op - operation type being encode/decode.
 *                 EDU_ERR - out param to indicate errors in processing.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : None.
 *****************************************************************************/
uint32_t plms_edp_agent_readiness_impact(EDU_HDL *edu_hdl, EDU_TKN *edu_tkn,NCSCONTEXT ptr,uint32_t *ptr_data_len, EDU_BUF_ENV *buf_env,EDP_OP_TYPE op,EDU_ERR *o_err)
{
	 uint32_t rc = NCSCC_RC_SUCCESS;
	 PLMS_AGENT_READINESS_IMPACT *struct_ptr = NULL, **d_ptr;

	 EDU_INST_SET plms_agent_readiness_impact_rules[] = {
		 {EDU_START, plms_edp_agent_readiness_impact, 0, 0, 0, sizeof(PLMS_AGENT_READINESS_IMPACT), 0, NULL},
		 {EDU_EXEC, ncs_edp_sanamet, EDQ_POINTER, 0, 0, (long)&((PLMS_AGENT_READINESS_IMPACT *)0)->impacted_entity, 0, NULL},
		 {EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((PLMS_AGENT_READINESS_IMPACT *)0)->impact, 0, NULL},
		 {EDU_EXEC, plms_edp_ntf_correlation_ids, 0, 0, 0, (long)&((PLMS_AGENT_READINESS_IMPACT *)0)->correlation_ids, 0, NULL},
		 {EDU_END, 0, 0, 0, 0, 0, 0, NULL},
	};

	if(op == EDP_OP_TYPE_ENC)
        {
                struct_ptr = (PLMS_AGENT_READINESS_IMPACT *)ptr;
        }
        else if(op == EDP_OP_TYPE_DEC)
        {
                d_ptr = (PLMS_AGENT_READINESS_IMPACT **)ptr;
                if(*d_ptr == NULL)
                {
                        *o_err = EDU_ERR_MEM_FAIL;
                        return NCSCC_RC_FAILURE;
                }
                memset(*d_ptr, '\0', sizeof(PLMS_AGENT_READINESS_IMPACT));
                struct_ptr = *d_ptr;
		(*d_ptr)->impacted_entity = malloc(sizeof(SaNameT));
		memset((*d_ptr)->impacted_entity, 0, sizeof(SaNameT));
	}
	else
	{
		struct_ptr = ptr;
										        }

	rc = m_NCS_EDU_RUN_RULES(edu_hdl, edu_tkn, plms_agent_readiness_impact_rules, struct_ptr, ptr_data_len, buf_env, op, o_err);
        return rc;
}																												
/****************************************************************************
 * Name          : plms_edp_agent_track_cbk_response
 *
 * Description   : This function is an EDU program for encoding/decoding
 *                 plm agent track callback response.
 *
 * Arguments     : EDU_HDL - pointer to edu handle,
 *                 EDU_TKN - internal edu token to help encode/decode,
 *                 POINTER to the structure to encode/decode from/to,
 *                 data length specifying number of structures,
 *                 EDU_BUF_ENV - pointer to buffer for encoding/decoding.
 *                 op - operation type being encode/decode.
 *                 EDU_ERR - out param to indicate errors in processing.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : None.
 *****************************************************************************/
uint32_t plms_edp_agent_track_cbk_resp(EDU_HDL *edu_hdl, EDU_TKN *edu_tkn,NCSCONTEXT ptr,uint32_t *ptr_data_len, EDU_BUF_ENV *buf_env,EDP_OP_TYPE op,EDU_ERR *o_err)
{
	 uint32_t rc = NCSCC_RC_SUCCESS;
	 PLMS_TRACK_CBK_RES *struct_ptr = NULL, **d_ptr;

	 EDU_INST_SET plms_agent_track_cbk_resp_rules[] = {
		 {EDU_START, plms_edp_agent_track_cbk_resp, 0, 0, 0, sizeof(PLMS_TRACK_CBK_RES), 0, NULL},
		 {EDU_EXEC, ncs_edp_uns64, 0, 0, 0, (long)&((PLMS_TRACK_CBK_RES *)0)->invocation_id, 0, NULL},
		 {EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((PLMS_TRACK_CBK_RES *)0)->response, 0, NULL},
		 {EDU_END, 0, 0, 0, 0, 0, 0, NULL},
	};

	if(op == EDP_OP_TYPE_ENC)
        {
                struct_ptr = (PLMS_TRACK_CBK_RES *)ptr;
        }
        else if(op == EDP_OP_TYPE_DEC)
        {
                d_ptr = (PLMS_TRACK_CBK_RES **)ptr;
                if(*d_ptr == NULL)
                {
                        *o_err = EDU_ERR_MEM_FAIL;
                        return NCSCC_RC_FAILURE;
                }
                memset(*d_ptr, '\0', sizeof(PLMS_TRACK_CBK_RES));
                struct_ptr = *d_ptr;
	}
	else
	{
		struct_ptr = ptr;
										        }

	rc = m_NCS_EDU_RUN_RULES(edu_hdl, edu_tkn, plms_agent_track_cbk_resp_rules, struct_ptr, ptr_data_len, buf_env, op, o_err);
        return rc;
}																												
/****************************************************************************
 * Name          : plms_edp_agent_track_cbk
 *
 * Description   : This function is an EDU program for encoding/decoding
 *                 plm agent track callback.
 *
 * Arguments     : EDU_HDL - pointer to edu handle,
 *                 EDU_TKN - internal edu token to help encode/decode,
 *                 POINTER to the structure to encode/decode from/to,
 *                 data length specifying number of structures,
 *                 EDU_BUF_ENV - pointer to buffer for encoding/decoding.
 *                 op - operation type being encode/decode.
 *                 EDU_ERR - out param to indicate errors in processing.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : None.
 *****************************************************************************/
uint32_t plms_edp_agent_track_cbk(EDU_HDL *edu_hdl, EDU_TKN *edu_tkn,NCSCONTEXT ptr,uint32_t *ptr_data_len, EDU_BUF_ENV *buf_env,EDP_OP_TYPE op,EDU_ERR *o_err)
{
	 uint32_t rc = NCSCC_RC_SUCCESS;
	 PLMS_AGENT_TRACK_CBK *struct_ptr = NULL, **d_ptr;

	 EDU_INST_SET plms_agent_track_cbk_rules[] = {
		 {EDU_START, plms_edp_agent_track_cbk, 0, 0, 0, sizeof(PLMS_AGENT_TRACK_CBK), 0, NULL},
		 {EDU_EXEC, ncs_edp_uns64, 0, 0, 0, (long)&((PLMS_AGENT_TRACK_CBK *)0)->track_cookie, 0, NULL},
		 {EDU_EXEC, ncs_edp_uns64, 0, 0, 0, (long)&((PLMS_AGENT_TRACK_CBK *)0)->invocation_id, 0, NULL},
		 {EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((PLMS_AGENT_TRACK_CBK *)0)->track_cause, 0, NULL},
		 {EDU_EXEC, ncs_edp_sanamet, 0, 0, 0, (long)&((PLMS_AGENT_TRACK_CBK *)0)->root_cause_entity, 0, NULL},
		 {EDU_EXEC, ncs_edp_uns64, 0, 0, 0, (long)&((PLMS_AGENT_TRACK_CBK *)0)->root_correlation_id, 0, NULL},
		 {EDU_EXEC, plms_edp_readiness_tracked_entities, 0, 0, 0, (long)&((PLMS_AGENT_TRACK_CBK *)0)->tracked_entities, 0, NULL},
		 {EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((PLMS_AGENT_TRACK_CBK *)0)->change_step, 0, NULL},
		 {EDU_END, 0, 0, 0, 0, 0, 0, NULL},
	};

	if(op == EDP_OP_TYPE_ENC)
        {
                struct_ptr = (PLMS_AGENT_TRACK_CBK *)ptr;
        }
        else if(op == EDP_OP_TYPE_DEC)
        {
                d_ptr = (PLMS_AGENT_TRACK_CBK **)ptr;
                if(*d_ptr == NULL)
                {
                        *o_err = EDU_ERR_MEM_FAIL;
                        return NCSCC_RC_FAILURE;
                }
                memset(*d_ptr, '\0', sizeof(PLMS_AGENT_TRACK_CBK));
                struct_ptr = *d_ptr;
	}
	else
	{
		struct_ptr = ptr;
										        }

	rc = m_NCS_EDU_RUN_RULES(edu_hdl, edu_tkn, plms_agent_track_cbk_rules, struct_ptr, ptr_data_len, buf_env, op, o_err);
        return rc;
}																												
/****************************************************************************
 * Name          : plms_evt_test_type_func
 *
 * Description   : This function is to test the plms event type for enc/dec
 *
 * Arguments     : arg :- pointer to the event type 
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : None.
 *****************************************************************************/
uint32_t plms_evt_test_type_func(NCSCONTEXT arg)
{
	PLMS_EVT_REQ_RES type;
	enum {
		PLMS_EDU_PLMS_REQ=1,
		PLMS_EDU_PLMS_RES
	};

	if (arg == NULL)
	{
		return EDU_FAIL;
	}

	type = *(PLMS_EVT_REQ_RES *)arg;

	switch(type)
	{
		case PLMS_REQ:
			return PLMS_EDU_PLMS_REQ;
		case PLMS_RES:
			return PLMS_EDU_PLMS_RES;
		default:
			return EDU_FAIL;
	}
}
/****************************************************************************
 * Name          : plms_evt_test_req_type
 *
 * Description   : This function is to test the plms event req type for enc/dec
 *
 * Arguments     : arg :- pointer to the req type 
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : None.
 *****************************************************************************/
uint32_t plms_evt_test_req_type(NCSCONTEXT arg)
{
	PLMS_EVT_TYPE  req_type;
	enum {
		PLMS_EDU_PLMS_AGENT_LIB_REQ=1,
		PLMS_EDU_PLMS_AGENT_GRP_OP,
		PLMS_EDU_PLMS_AGENT_TRACK_OP
	};
	if (arg == NULL)
	{
		return EDU_FAIL;
	}
	req_type = *(PLMS_EVT_TYPE *)arg;

	switch(req_type)
	{
		case PLMS_AGENT_LIB_REQ_EVT_T:
			return PLMS_EDU_PLMS_AGENT_LIB_REQ;
		case PLMS_AGENT_GRP_OP_EVT_T:
			return PLMS_EDU_PLMS_AGENT_GRP_OP;
		case PLMS_AGENT_TRACK_EVT_T:
			return PLMS_EDU_PLMS_AGENT_TRACK_OP;
		default:
			return EDU_FAIL;
	}
}

/****************************************************************************
 * Name          : plms_evt_test_res_type
 *
 * Description   : This function is to test the plms event res type for enc/dec
 *
 * Arguments     : arg :- pointer to the res type 
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : None.
 *****************************************************************************/
uint32_t plms_evt_test_res_type(NCSCONTEXT arg)
{
	PLMS_EVT_RES_TYPE  res_type;
	enum {
		PLMS_EDU_PLMS_COMMON_RESP=1,
		PLMS_EDU_PLMS_TRACK_START_RESP,
		PLMS_EDU_PLMS_AGENT_TRACK_OP
	};
	if (arg == NULL)
	{
		return EDU_FAIL;
	}
	res_type = *(PLMS_EVT_RES_TYPE *)arg;

	switch(res_type)
	{
		case PLMS_AGENT_INIT_RES:
	        case PLMS_AGENT_FIN_RES:
	        case PLMS_AGENT_GRP_CREATE_RES:
	        case PLMS_AGENT_GRP_REMOVE_RES:
	        case PLMS_AGENT_GRP_ADD_RES:
	        case PLMS_AGENT_GRP_DEL_RES:
	        case PLMS_AGENT_TRACK_STOP_RES:
	        case PLMS_AGENT_TRACK_READINESS_IMPACT_RES:
			return PLMS_EDU_PLMS_COMMON_RESP;;
	        case PLMS_AGENT_TRACK_START_RES:
			return PLMS_EDU_PLMS_TRACK_START_RESP;
		default:
			return EDU_FAIL;
	}
}
/****************************************************************************
 * Name          : plms_evt_test_track_op_type
 *
 * Description   : This function is to test the type of track op for enc/dec 
 *
 * Arguments     : arg :- pointer to the track operation type 
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : None.
 *****************************************************************************/
uint32_t plms_test_track_op_type(NCSCONTEXT arg)
{
	PLMS_TRACK_EVT_TYPE  evt_type;
	enum {
		PLMS_EDU_PLMS_AGENT_TRACK_START=1,
                PLMS_EDU_PLMS_AGENT_READINESS_IMPACT,
                PLMS_EDU_PLMS_TRACK_CBK_RES,
                PLMS_EDU_PLMS_AGENT_TRACK_CBK,
		PLMS_EDU_PLMS_TRACK_STOP
	};
	if (arg == NULL)
	{
		return EDU_FAIL;
	}
	evt_type = *(PLMS_TRACK_EVT_TYPE *)arg;

	switch(evt_type)
	{
		case PLMS_AGENT_TRACK_START_EVT:
			return PLMS_EDU_PLMS_AGENT_TRACK_START; 
                case PLMS_AGENT_TRACK_READINESS_IMPACT_EVT:
			return PLMS_EDU_PLMS_AGENT_READINESS_IMPACT;
                case PLMS_AGENT_TRACK_RESPONSE_EVT: 
			return PLMS_EDU_PLMS_TRACK_CBK_RES;
                case PLMS_AGENT_TRACK_CBK_EVT:
			return PLMS_EDU_PLMS_AGENT_TRACK_CBK;
		case PLMS_AGENT_TRACK_STOP_EVT:
			return PLMS_EDU_PLMS_TRACK_STOP;
		default:
			return EDU_FAIL;
	}
}
/****************************************************************************
* Name          : plms_edp_agent_track_op
*
* Description   : This is the function which is used to encode decode
*                 PLMS_AGENT_TRACK_OP structure.
*
*
* Notes         : None.
*****************************************************************************/
uint32_t plms_edp_agent_track_op(EDU_HDL *edu_hdl, EDU_TKN *edu_tkn,
			NCSCONTEXT ptr, uint32_t *ptr_data_len, EDU_BUF_ENV 				*buf_env, EDP_OP_TYPE op, EDU_ERR *o_err)
{
	uint32_t       rc = NCSCC_RC_SUCCESS;
	PLMS_AGENT_TRACK_OP   *struct_ptr = NULL, **d_ptr = NULL;

	EDU_INST_SET    plms_edp_agent_track_op_rules[ ] = {
	{EDU_START, plms_edp_agent_track_op, 0, 0, 0, sizeof(PLMS_AGENT_TRACK_OP), 0, NULL},
	{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((PLMS_AGENT_TRACK_OP *)0)->evt_type, 0, NULL},
	{EDU_EXEC, ncs_edp_uns64, 0, 0, 0, (long)&((PLMS_AGENT_TRACK_OP *)0)->agent_handle, 0, NULL},
	{EDU_EXEC, ncs_edp_uns64, 0, 0, 0, (long)&((PLMS_AGENT_TRACK_OP *)0)->grp_handle, 0, NULL},
	{EDU_TEST, ncs_edp_uns32, 0, 0, 0, (long)&((PLMS_AGENT_TRACK_OP *)0)->evt_type, 0, (EDU_EXEC_RTINE)plms_test_track_op_type},


	/* For plms agent track start */
	{EDU_EXEC, plms_edp_agent_track_start, 0, 0, EDU_EXIT, (long)&((PLMS_AGENT_TRACK_OP*)0)->track_start, 0, NULL},
	/* For plms agent readiness impact */
	{EDU_EXEC, plms_edp_agent_readiness_impact, 0, 0, EDU_EXIT, (long)&((PLMS_AGENT_TRACK_OP*)0)->readiness_impact, 0, NULL},
	/* For plms agent track callback response */
	{EDU_EXEC, plms_edp_agent_track_cbk_resp, 0, 0, EDU_EXIT, (long)&((PLMS_AGENT_TRACK_OP*)0)->track_cbk_res, 0, NULL},
	/* For plms agent track callback  */
	{EDU_EXEC, plms_edp_agent_track_cbk, 0, 0, EDU_EXIT, (long)&((PLMS_AGENT_TRACK_OP*)0)->track_cbk, 0, NULL},
        {EDU_END, 0, 0, 0, 0, 0, 0, NULL},
	};

        if(op == EDP_OP_TYPE_ENC)
	{
		struct_ptr = (PLMS_AGENT_TRACK_OP *)ptr;
	}
	else if(op == EDP_OP_TYPE_DEC)
	{
		d_ptr = (PLMS_AGENT_TRACK_OP **)ptr;
		if(*d_ptr == NULL)
		{
			*o_err = EDU_ERR_MEM_FAIL;
			return NCSCC_RC_FAILURE;
		}
		memset(*d_ptr, '\0', sizeof(PLMS_AGENT_TRACK_OP));
		struct_ptr = *d_ptr;
	}
	else
	{
		struct_ptr = ptr;
	}

	rc = m_NCS_EDU_RUN_RULES(edu_hdl, edu_tkn, plms_edp_agent_track_op_rules, struct_ptr, ptr_data_len, buf_env, op, o_err);
	return rc;
}
	
/****************************************************************************
* Name          : plms_edp_req_info
*
* Description   : This is the function which is used to encode decode
*                 PLMS_EVT_REQ structure.
*
*
* Notes         : None.
*****************************************************************************/
uint32_t plms_edp_req_info(EDU_HDL *edu_hdl, EDU_TKN *edu_tkn,
			NCSCONTEXT ptr, uint32_t *ptr_data_len, EDU_BUF_ENV 				*buf_env, EDP_OP_TYPE op, EDU_ERR *o_err)
{
	uint32_t       rc = NCSCC_RC_SUCCESS;
	PLMS_EVT_REQ   *struct_ptr = NULL, **d_ptr = NULL;

	EDU_INST_SET    plms_edp_req_info_rules[ ] = {
		{EDU_START, plms_edp_req_info, 0, 0, 0, sizeof(PLMS_EVT_REQ), 0, NULL},
		{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((PLMS_EVT_REQ *)0)->req_type, 0, NULL},
		{EDU_TEST, ncs_edp_uns32, 0, 0, 0, (long)&((PLMS_EVT_REQ *)0)->req_type, 0, (EDU_EXEC_RTINE)plms_evt_test_req_type},


		/* For PLMS_AGENT_LIB_REQ */
		{EDU_EXEC, plms_edp_agent_lib_req, 0, 0, EDU_EXIT, (long)&((PLMS_EVT_REQ*)0)->agent_lib_req, 0, NULL},
		/* For PLMS_AGENT_GRP_OP */
		{EDU_EXEC, plms_edp_agent_grp_op, 0, 0, EDU_EXIT, (long)&((PLMS_EVT_REQ*)0)->agent_grp_op, 0, NULL},
		/* For PLMS_AGENT_TRACK_OP */
		{EDU_EXEC, plms_edp_agent_track_op, 0, 0, EDU_EXIT, (long)&((PLMS_EVT_REQ*)0)->agent_track, 0, NULL},
		{EDU_END, 0, 0, 0, 0, 0, 0, NULL},
	};

        if(op == EDP_OP_TYPE_ENC)
	{
		struct_ptr = (PLMS_EVT_REQ *)ptr;
	}
	else if(op == EDP_OP_TYPE_DEC)
	{
		d_ptr = (PLMS_EVT_REQ **)ptr;
		if(*d_ptr == NULL)
		{
			*o_err = EDU_ERR_MEM_FAIL;
			return NCSCC_RC_FAILURE;
		}
		memset(*d_ptr, '\0', sizeof(PLMS_EVT_REQ));
		struct_ptr = *d_ptr;
	}
	else
	{
		struct_ptr = ptr;
	}

	rc = m_NCS_EDU_RUN_RULES(edu_hdl, edu_tkn, plms_edp_req_info_rules, struct_ptr, ptr_data_len, buf_env, op, o_err);
	return rc;
}

/****************************************************************************
* Name          : plms_edp_res_info
*
* Description   : This is the function which is used to encode decode
*                 PLMS_EVT_RES structure.
*
*
* Notes         : None.
*****************************************************************************/
uint32_t plms_edp_res_info(EDU_HDL *edu_hdl, EDU_TKN *edu_tkn,
			NCSCONTEXT ptr, uint32_t *ptr_data_len, EDU_BUF_ENV 				*buf_env, EDP_OP_TYPE op, EDU_ERR *o_err)
{
	uint32_t       rc = NCSCC_RC_SUCCESS;
	PLMS_EVT_RES   *struct_ptr = NULL, **d_ptr = NULL;

	EDU_INST_SET    plms_edp_res_info_rules[ ] = {
	{EDU_START, plms_edp_res_info, 0, 0, 0, sizeof(PLMS_EVT_RES), 0, NULL},
	{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((PLMS_EVT_RES *)0)->res_type, 0, NULL},
	{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((PLMS_EVT_RES *)0)->error, 0, NULL},
	{EDU_EXEC, ncs_edp_uns64, 0, 0, 0, (long)&((PLMS_EVT_RES *)0)->ntf_id, 0, NULL},
	{EDU_TEST, ncs_edp_uns32, 0, 0, 0, (long)&((PLMS_EVT_RES *)0)->res_type, 0, (EDU_EXEC_RTINE)plms_evt_test_res_type},


	/* For plms responses other than readiness tracked entities */
	{EDU_EXEC, ncs_edp_uns64, 0, 0, EDU_EXIT, (long)&((PLMS_EVT_RES *)0)->hdl, 0, NULL},
	/* For plms response for readiness tracked entities */
	{EDU_EXEC, plms_edp_readiness_tracked_entities, EDQ_POINTER, 0, EDU_EXIT, (long)&((PLMS_EVT_RES*)0)->entities, 0, NULL},
	
        {EDU_END, 0, 0, 0, 0, 0, 0, NULL},
	};

        if(op == EDP_OP_TYPE_ENC)
	{
		struct_ptr = (PLMS_EVT_RES *)ptr;
	}
	else if(op == EDP_OP_TYPE_DEC)
	{
		d_ptr = (PLMS_EVT_RES **)ptr;
		if(*d_ptr == NULL)
		{
			*o_err = EDU_ERR_MEM_FAIL;
			return NCSCC_RC_FAILURE;
		}
		memset(*d_ptr, '\0', sizeof(PLMS_EVT_RES));
		(*d_ptr)->entities = malloc(sizeof(SaPlmReadinessTrackedEntitiesT));
		memset((*d_ptr)->entities, '\0', sizeof(SaPlmReadinessTrackedEntitiesT));
		struct_ptr = *d_ptr;
	}
	else
	{
		struct_ptr = ptr;
	}

	rc = m_NCS_EDU_RUN_RULES(edu_hdl, edu_tkn, plms_edp_res_info_rules, struct_ptr, ptr_data_len, buf_env, op, o_err);
	return rc;
}

/****************************************************************************
 * Name          : plms_edp_plms_evt
 *
 * Description   : This is the function which is used to encode decode
 *                 PLMS_EVT structures.
 *
 *
 * Notes         : None.
 *****************************************************************************/
uint32_t plms_edp_plms_evt(EDU_HDL *edu_hdl, EDU_TKN *edu_tkn,
			NCSCONTEXT ptr, uint32_t *ptr_data_len,
			EDU_BUF_ENV *buf_env, EDP_OP_TYPE op,
			EDU_ERR *o_err)
{
	uint32_t       rc = NCSCC_RC_SUCCESS;
	PLMS_EVT   *struct_ptr = NULL, **d_ptr = NULL;

	EDU_INST_SET    test_plms_evt_rules[ ] = {
	{EDU_START, plms_edp_plms_evt, 0, 0, 0, sizeof(PLMS_EVT), 0, NULL},
	{EDU_EXEC, ncs_edp_uns32, 0, 0, 0, (long)&((PLMS_EVT*)0)->req_res, 0, NULL},
	{EDU_TEST, ncs_edp_uns32, 0, 0, 0, (long)&((PLMS_EVT*)0)->req_res, 0, (EDU_EXEC_RTINE)plms_evt_test_type_func},

	/* For PLM request type */
	{EDU_EXEC, plms_edp_req_info, 0, 0, EDU_EXIT, (long)&((PLMS_EVT*)0)->req_evt, 0, NULL},

	/* For PLM response type */
	{EDU_EXEC, plms_edp_res_info, 0, 0, EDU_EXIT, (long)&((PLMS_EVT*)0)->res_evt, 0, NULL},

        {EDU_END, 0, 0, 0, 0, 0, 0, NULL},
	};

        if(op == EDP_OP_TYPE_ENC)
	{
		struct_ptr = (PLMS_EVT *)ptr;
	}
	else if(op == EDP_OP_TYPE_DEC)
	{
		d_ptr = (PLMS_EVT **)ptr;
		if(*d_ptr == NULL)
		{
			*o_err = EDU_ERR_MEM_FAIL;
			return NCSCC_RC_FAILURE;
		}
		memset(*d_ptr, '\0', sizeof(PLMS_EVT));
		struct_ptr = *d_ptr;
	}
	else
	{
		struct_ptr = ptr;
	}

	rc = m_NCS_EDU_RUN_RULES(edu_hdl, edu_tkn, test_plms_evt_rules, struct_ptr, ptr_data_len, buf_env, op, o_err);
	return rc;
}
	
