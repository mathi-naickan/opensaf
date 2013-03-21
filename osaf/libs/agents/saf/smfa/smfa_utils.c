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
* Author(s): GoAhead Software
*
*/

/*************************************************************************** 
 * @file	smfa_utils.c
 * @brief	This file contains all SMFA utility functions.	
 * @author	GoAhead Software
*****************************************************************************/
#include "smfa.h"

extern SMFA_CB _smfa_cb;

/*************************************************************************** 
@brief		: Get the client info node for the corresponding hdl.
@param[in]	: hdl - Handle for which the client info is to be returned.
@return		: NULL - If corresponding client info is not found.
		  SMFA_CLIENT_INFO * - Corresponding client info.
*****************************************************************************/
SMFA_CLIENT_INFO * smfa_client_info_get(SaSmfHandleT hdl)
{
	SMFA_CB	*cb = &_smfa_cb;
	SMFA_CLIENT_INFO *client_info_list = cb->smfa_client_info_list;
	
	while (NULL != client_info_list){
		if (hdl == client_info_list->client_hdl){
			return client_info_list;
		}else
			client_info_list = client_info_list->next_client;
	}

	return NULL;
}

/*************************************************************************** 
@brief		: Add the client info node at the head of the 
		  cb->smfa_client_info_list.  Always added at the head.
@param[in]	: client_info - Client to be added. 
*****************************************************************************/
void smfa_client_info_add(SMFA_CLIENT_INFO *client_info)
{
	SMFA_CB	*cb = &_smfa_cb;

	/* The list is empty, first node to be added.*/
	if (NULL == cb->smfa_client_info_list){
		cb->smfa_client_info_list = client_info;
	}else {/* Append at the head.*/
		client_info->next_client = cb->smfa_client_info_list;
		cb->smfa_client_info_list = client_info;
	}
	return;
}
/*************************************************************************** 
@brief		: Delete the client info node for the corresponding hdl from 
		  the cb->smfa_client_info_list.
@param[in]	: hdl - Client to be removed. 
@return		: NCSCC_RC_FAILURE/NCSCC_RC_SUCCESS
*****************************************************************************/
uint32_t smfa_client_info_rmv(SaSmfHandleT hdl)
{
	SMFA_CB *cb = &_smfa_cb;
	SMFA_CLIENT_INFO *client_info_list = cb->smfa_client_info_list;
	SMFA_CLIENT_INFO *client_info_prev = client_info_list;

	while(NULL != client_info_list){
		if (hdl == client_info_list->client_hdl){
			if (client_info_list->client_hdl == client_info_prev->client_hdl){
				/* Deleting the head node.*/
				cb->smfa_client_info_list = client_info_list->next_client;
			}else{
				client_info_prev->next_client = client_info_list->next_client;
			}
			free(client_info_list);
			client_info_list = NULL;
			return NCSCC_RC_SUCCESS;
		}else{
			client_info_prev = client_info_list;
			client_info_list = client_info_list->next_client;
		}
	}
	return NCSCC_RC_FAILURE;
}

/*************************************************************************** 
@brief		: Get the scope info node scorresponding to the scope_id for 
		  the client client_info.
@param[in]	: client_info - Client in which the scope to be searched.
@param[in]	: scope_id - Scope to be searched.
@return		: NULL - If scope id not found.
		  SMFA_SCOPE_INFO * - If found.
*****************************************************************************/
SMFA_SCOPE_INFO* smfa_scope_info_get(SMFA_CLIENT_INFO *client_info, SaSmfCallbackScopeIdT scope_id)
{
	SMFA_SCOPE_INFO *scope_info_list = client_info->scope_info_list;

	while(NULL != scope_info_list){
		if (scope_id == scope_info_list->scope_id)
			return scope_info_list;
		else
			scope_info_list = scope_info_list->next_scope;
	}

	return NULL;
}
/*************************************************************************** 
@brief		: Add the scope_info node to the client_info->scope_info_list 
		  at the head. 
@param[in]	: client_info - Client in which the scope to be added.
@param[in]	: scope_info - Scope to be added.
*****************************************************************************/
void smfa_scope_info_add(SMFA_CLIENT_INFO *client_info, SMFA_SCOPE_INFO *scope_info)
{
	/* First node to be added.*/
	if (NULL == client_info->scope_info_list){
		client_info->scope_info_list = scope_info;
	}else{/* Add at the head.*/
		scope_info->next_scope = client_info->scope_info_list;
		client_info->scope_info_list = scope_info;
	}

	return;
}

/*************************************************************************** 
@brief		: Free the scope info node. This does not delete the scope info 
		  node from the client info, rather frees the internal allocated 
		  memory for the filters inside the scope info. After calling this 
		  function, free the scope info node itself.
@param[in]	: scope_info - Scope to be cleaned up.
*****************************************************************************/
void smfa_scope_info_free(SMFA_SCOPE_INFO *scope_info)
{
	uint32_t no_of_filters;
	for (no_of_filters=0; no_of_filters < scope_info->scope_of_interest.filtersNumber; no_of_filters++){
		if (scope_info->scope_of_interest.filters[no_of_filters].filter.label){
			free(scope_info->scope_of_interest.filters[no_of_filters].filter.label);
			scope_info->scope_of_interest.filters[no_of_filters].filter.label = NULL;
		}
	}

	free(scope_info->scope_of_interest.filters);
	free(scope_info);
	return;


}
/*************************************************************************** 
@brief		: Delete the scope info node coresponding to the scope_id 
		  from the client_info->scope_info_list. 
@param[in]	: client_info - Client from which the scope info to be removed.
@param[in]	: scope_id - Scope to be removed.
@return		: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
*****************************************************************************/
uint32_t smfa_scope_info_rmv(SMFA_CLIENT_INFO *client_info, SaSmfCallbackScopeIdT scope_id)
{
	SMFA_SCOPE_INFO *scope_info_list = client_info->scope_info_list;
	SMFA_SCOPE_INFO *scope_info_prev = scope_info_list;

	while(NULL != scope_info_list){
		if (scope_id == scope_info_list->scope_id){
			if (scope_info_prev->scope_id == scope_info_list->scope_id){
				/* Deleting the Head node.*/
				client_info->scope_info_list = scope_info_list->next_scope;
			}else{
				/* Deleting the non-head node.*/
				scope_info_prev->next_scope = scope_info_list->next_scope;
			}
			smfa_scope_info_free(scope_info_list);
			scope_info_list = NULL;
			return NCSCC_RC_SUCCESS;
		}else{
			scope_info_prev = scope_info_list;
			scope_info_list = scope_info_list->next_scope;
		}
	}
	return NCSCC_RC_FAILURE;
}

/*************************************************************************** 
@brief		: Delete all the scope registered for this client. This function 
		  does not free the client info itself, rather frees all the 
		  internal memory allocated for the scope infos. 
@param[in]	: client_info - Client to be cleand up.
*****************************************************************************/
void smfa_client_info_clean(SMFA_CLIENT_INFO *client_info)
{
	SMFA_SCOPE_INFO *scope_info_list = client_info->scope_info_list;
	
	while(NULL != scope_info_list){
		/* Delete from the head of the list.*/
		client_info->scope_info_list = scope_info_list->next_scope;
		smfa_scope_info_free(scope_info_list);
		scope_info_list = client_info->scope_info_list;
	}
}

/*************************************************************************** 
@brief		: Dipatch one. Read one msg from the MBX. 
@param[in]	: client_info - Client for which dispatch is called.
@return		: SA_AIS_OK.
*****************************************************************************/
SaAisErrorT smfa_dispatch_cbk_one(SMFA_CLIENT_INFO *client_info)
{	
	SMF_EVT *msg;

	TRACE_ENTER();
	if(NULL != (msg = (SMF_EVT *) m_NCS_IPC_NON_BLK_RECEIVE(&client_info->cbk_mbx,NULL))){
		/* You have reached this far means, everything is fine.
		 1. Client is not yet finalized.
		 2. Cbk is also registered.
		 So you can call the cbk without any pre-requisite check.
		 */
		if (SMF_CLBK_EVT != msg->evt_type){
			LOG_ER("SMFA: Wrong evt is posted to client MBX. evt_type: %d",msg->evt_type);
			free(msg);
			TRACE_LEAVE();
			return SA_AIS_OK;
		}

		client_info->reg_cbk.saSmfCampaignCallback(client_info->client_hdl, msg->evt.cbk_evt.inv_id, 
				msg->evt.cbk_evt.scope_id, &msg->evt.cbk_evt.object_name, msg->evt.cbk_evt.camp_phase, 
				&msg->evt.cbk_evt.cbk_label, msg->evt.cbk_evt.params);
		smfa_evt_free(msg);
	}
	TRACE_LEAVE();
	return SA_AIS_OK;
}

/*************************************************************************** 
@brief		: Dipatch all. Read all the msgs from the MBX. 
@param[in]	: client_info - Client for which dispatch is called. 
@return		: SA_AIS_OK.
*****************************************************************************/
SaAisErrorT smfa_dispatch_cbk_all(SMFA_CLIENT_INFO *client_info)
{	
	SMF_EVT *msg;

	TRACE_ENTER();
	while(NULL != (msg = (SMF_EVT *) m_NCS_IPC_NON_BLK_RECEIVE(&client_info->cbk_mbx,NULL))){
		/* You have reached this far means, everything is fine.
		 1. Client is not yet finalized.
		 2. Cbk is also registered.
		 So you can call the cbk without any pre-requisite check.
		 */
		if (SMF_CLBK_EVT != msg->evt_type){
			LOG_ER("SMFA: Wrong evt is posted to client MBX. evt_type: %d",msg->evt_type);
			free(msg);
			continue;	
		}

		client_info->reg_cbk.saSmfCampaignCallback(client_info->client_hdl, msg->evt.cbk_evt.inv_id, 
				msg->evt.cbk_evt.scope_id, 
				&msg->evt.cbk_evt.object_name, msg->evt.cbk_evt.camp_phase, &msg->evt.cbk_evt.cbk_label, 
				msg->evt.cbk_evt.params);
		smfa_evt_free(msg);
	}

	TRACE_LEAVE();
	return SA_AIS_OK;
}

/*************************************************************************** 
@brief		: Dispatch block. Block while reading the MBX if there is no msg in the MBX. 
@param[in]	: client_info - Client for which dispatch is called. 
@return		: SA_AIS_OK.
*****************************************************************************/
SaAisErrorT smfa_dispatch_cbk_block(SMFA_CLIENT_INFO *client_info)
{	
	SMF_EVT *msg;
	
	TRACE_ENTER();
	while(NULL != (msg = (SMF_EVT *) m_NCS_IPC_RECEIVE(&client_info->cbk_mbx,NULL))){
		/* You have reached this far means, everything is fine.
		 1. Client is not yet finalized.
		 2. Cbk is also registered.
		 So you can call the cbk without any pre-requisite check.
		 */
		if (SMF_CLBK_EVT != msg->evt_type){
			LOG_ER("SMFA: Wrong evt is posted to client MBX. evt_type: %d",msg->evt_type);
			free(msg);
			continue;	
		}

		client_info->reg_cbk.saSmfCampaignCallback(client_info->client_hdl, msg->evt.cbk_evt.inv_id, 
				msg->evt.cbk_evt.scope_id, 
				&msg->evt.cbk_evt.object_name, msg->evt.cbk_evt.camp_phase, &msg->evt.cbk_evt.cbk_label, 
				msg->evt.cbk_evt.params);
		smfa_evt_free(msg);
	}

	TRACE_LEAVE();
	return SA_AIS_OK;
}
/*************************************************************************** 
@brief		: Preocess the OK response from the client. Agent takes care 
		  of consolidating all the responses for this agent and finally 
		  sending the response ti SMFND. This function does the following.
 		  1. Get to the inv node in cb->cbk_list.
		  2. If does not match, drop the msg. If matches, get to the 
		  hdl node in cbk_list->hdl_list.
 		  3. If does not match, drop the msg. If matches, decrement the 
		  resp count.
		  4. If the response count reaches to zero, implies there are no 
		  clients for this handle to respond and hence delete the hdl node 
		  from the cbk_list->hdl_list. If the response count is not zero, 
		  implies that there are responses expected from this hdl and hence 
		  return from there.
		  5. After deleting the hdl node from cbk_list->hdl_list, check if 
		  the list is empty.
 		  6. If the list is not empty, implies there are other clients yet 
		  to respond. If the list is empty, implies all the clients have 
		  responded for this cbk. And hence remove this inv node from 
		  cb->cbk_list and return SUCCESS so that the consolidated response 
		  can be sent to SMFND.
@param[in]	: smfHandle - Handle.
@param[in]	: invocation - Inv id sent in response by the application.
@return		: NCSCC_RC_FAILURE/NCSCC_RC_SUCCESS.
*****************************************************************************/
uint32_t smfa_cbk_ok_resp_process(SaSmfHandleT smfHandle,SaInvocationT invocation)
{
	SMFA_CB *cb = &_smfa_cb;
	SMFA_CBK_LIST *cbk_list = cb->cbk_list, *cbk_prev = NULL;
	SMFA_CBK_HDL_LIST *hdl_list = NULL, *hdl_prev;
	uint32_t rc = NCSCC_RC_FAILURE;

	TRACE_ENTER2("Handle: %llu, ind_id: %llu.",smfHandle,invocation);
	/* Find the invocation node.*/
	cbk_prev = cbk_list;
	while(cbk_list){
		/* Inv node found.*/
		if (invocation == cbk_list->inv_id){
			hdl_list = cbk_list->hdl_list;
			hdl_prev = hdl_list;
			while(hdl_list){
				/* Hdl found.*/
				if (smfHandle == hdl_list->hdl){
					hdl_list->cnt--;
					/* Final response for this hdl.*/
					if (0 == hdl_list->cnt){
						/* Remove the hdl node*/
						hdl_prev->next_hdl = hdl_list->next_hdl;
						/* Head node deleted.*/
						if ((hdl_list->hdl == hdl_prev->hdl)){
							cbk_list->hdl_list =  hdl_prev->next_hdl;
							/* No more resp expected.*/
							if (NULL == cbk_list->hdl_list){
								free(hdl_list);
								rc = NCSCC_RC_SUCCESS;
								goto rmv_inv;
							}
						}
						/* Still resp expected from other hdls.*/
						free(hdl_list);
					}
					TRACE_2("SMFA: Waiting for remaining resp.");
					/* Not the final resp from this hdl.*/
					rc = NCSCC_RC_SUCCESS;
					goto done;
				}
				hdl_prev = hdl_list;
				hdl_list = hdl_list->next_hdl;
			}
			goto done;
		}
		cbk_prev = cbk_list;	
		cbk_list = cbk_list->next_cbk;
	}
	goto done;

rmv_inv:
	cbk_prev->next_cbk = cbk_list->next_cbk;
	/* Last inv node.*/
	if ((cbk_list->inv_id == cbk_prev->inv_id)){
		cb->cbk_list = cbk_prev->next_cbk;
	}
	free(cbk_list);
done:
	TRACE_LEAVE();
	return rc;
}

/*************************************************************************** 
@brief		: Process the error response. This function does the following.
		  1. Delete the inv node from the cb->cbk_list corresponding to 
		  the invocation.
		  2. Make sure all the hdl nodes are freed from cbk_list->hdl_list 
		  before deleting inv node.
@param[in]	: hdl - Handle.
@param[in]	: invocation - Inv id sent in response by the application.
@return		: NCSCC_RC_FAILURE/NCSCC_RC_SUCCESS.
*****************************************************************************/
uint32_t smfa_cbk_err_resp_process(SaInvocationT invocation,SaSmfHandleT hdl)
{
	SMFA_CB *cb = &_smfa_cb;
	SMFA_CBK_LIST *cbk_list = cb->cbk_list, *cbk_prev = NULL;
	SMFA_CBK_HDL_LIST *hdl_list = NULL ;
	uint32_t rc = NCSCC_RC_FAILURE;
	uint32_t is_delay_resp = 1;
	
	TRACE_ENTER2("Handle: %llu, ind_id: %llu.",hdl,invocation);
	/* Find the invocation node.*/
	cbk_prev = cbk_list;
	while(cbk_list){
		/* Inv node found.*/
		if (invocation == cbk_list->inv_id){
			hdl_list = cbk_list->hdl_list;
			/* Check for duplicate or delay resp first.*/
			while (hdl_list){
				if (hdl == hdl_list->hdl){
					is_delay_resp = 0;
					break;
				}
				hdl_list = hdl_list->next_hdl;
			}
			/* Return failure if delayed/duplicate resp.*/
			if (is_delay_resp)
				goto done;

			hdl_list = cbk_list->hdl_list;
			while(hdl_list){
				cbk_list->hdl_list = hdl_list->next_hdl;
				free(hdl_list);
				hdl_list = cbk_list->hdl_list;
			}
			goto rmv_inv;
		}
		cbk_prev = cbk_list;	
		cbk_list = cbk_list->next_cbk;
	}
	goto done;

rmv_inv:
	cbk_prev->next_cbk = cbk_list->next_cbk;
	/* Last inv node.*/
	if ((cbk_list->inv_id == cbk_prev->inv_id)){
		cb->cbk_list = cbk_prev->next_cbk;
	}
	free(cbk_list);
	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
done:
	TRACE_LEAVE();
	return rc;
}
/*************************************************************************** 
@brief		: Does filter matching depending upon filter type. 
@param[in]	: stored_flt - Application registered filters.
@param[in]	: passed_flt - Filters passed by SMFD part of XML file. 
@return		: NCSCC_RC_FAILURE/NCSCC_RC_SUCCESS.
***************************************************************************/ 
uint32_t smfa_cbk_filter_type_match(SaSmfLabelFilterT *stored_flt,SaSmfCallbackLabelT *passed_flt)
{
	uint32_t rc = NCSCC_RC_FAILURE;

	switch(stored_flt->filterType){
		case SA_SMF_PREFIX_FILTER:
			/* Match only the string of length registered by the application from the 
			   begining of the label.*/
			if (0 != stored_flt->filter.labelSize){
				if (0 == strncmp((char *)stored_flt->filter.label,(char *)passed_flt->label,
				(unsigned)stored_flt->filter.labelSize)){
					rc = NCSCC_RC_SUCCESS;
				}
			}
			break;
		case SA_SMF_SUFFIX_FILTER:
			/* Match only the string of length registered by the application from the 
			   end of the label.*/
			if (0 != stored_flt->filter.labelSize){
				if (0 == strncmp((char *)stored_flt->filter.label,
				(char *)&passed_flt->label[passed_flt->labelSize - stored_flt->filter.labelSize],
				(unsigned)stored_flt->filter.labelSize)){
					rc = NCSCC_RC_SUCCESS;
				}
			}
			break;
		case SA_SMF_EXACT_FILTER:
			if (stored_flt->filter.labelSize == passed_flt->labelSize){
				if (0 == strncmp((char *)stored_flt->filter.label,(char *)passed_flt->label,(unsigned)passed_flt->labelSize)){
					rc = NCSCC_RC_SUCCESS;
				}
			}
			break;
		/* Always matches.*/	
		case SA_SMF_PASS_ALL_FILTER:
			rc = NCSCC_RC_SUCCESS;
			break;
		default:
			rc = NCSCC_RC_FAILURE;
	}
	return rc;
}

/*************************************************************************** 
@brief		: 1. If the cb->cbk_list does not have inv node corersponding 
		  to the inv_id, then create inv node and add at the head of the 
		  cb->cbk_list. Create the hdl node for hdl and add at the head
		  of the cb->cbk_list->hdl_list.
		  2. If inv node exists, then create only hdl node and add at the head
		  of the cb->cbk_list->hdl_list.

		  This function will be hit only for the first time when the hdl node
		  is created and hence optimized taking this into consideration.

@param[in]	: inv_id - Node to be added, represents a cbk.
@param[in]	: hdl - Handle. 
@return		: SMFA_CBK_HDL_LIST * 
***************************************************************************/ 
SMFA_CBK_HDL_LIST * smfa_inv_hdl_add(SaInvocationT inv_id, SaSmfHandleT hdl)
{
	SMFA_CB *cb = &_smfa_cb;
	SMFA_CBK_LIST *cbk_list = cb->cbk_list;
	SMFA_CBK_HDL_LIST *hdl_list;
	
	while (cbk_list){
		if(inv_id == cbk_list->inv_id)
			break;
		cbk_list = cbk_list->next_cbk;
	}

	if (NULL == cbk_list){
		/* First filter matching. 1. Create inv node 2. Create hdl node.*/
		cbk_list = (SMFA_CBK_LIST *)calloc(1,sizeof(SMFA_CBK_LIST));
		if (!cbk_list){
			LOG_ER("SMFA: calloc FAILED, error: %s",strerror(errno));
			osafassert(0);
		}
		cbk_list->inv_id = inv_id;
		cbk_list->hdl_list = (SMFA_CBK_HDL_LIST *)calloc(1,sizeof(SMFA_CBK_HDL_LIST));
		if(!cbk_list->hdl_list){
			LOG_ER("SMFA: calloc FAILED, error: %s",strerror(errno));
			osafassert(0);
		}
		cbk_list->hdl_list->hdl = hdl;
		/* Add at the head.*/
		cbk_list->next_cbk = cb->cbk_list;
		cb->cbk_list = cbk_list;
		return cbk_list->hdl_list;
	}else{
		/* Inv_id matched ==> this might not the first hdl for this inv_id but first filter 
		   matching for this hdl. So create hdl node.*/
		hdl_list = (SMFA_CBK_HDL_LIST *)calloc(1,sizeof(SMFA_CBK_HDL_LIST));
		if(!hdl_list){
			LOG_ER("SMFA: calloc FAILED, error: %s",strerror(errno));
			osafassert(0);
		}
		hdl_list->hdl = hdl;
		/* Add at the head.*/
		hdl_list->next_hdl = cbk_list->hdl_list;
		cbk_list->hdl_list = hdl_list;
		return hdl_list;
	}
}

/*************************************************************************** 
@brief		: Match the filter. If matches, post the evt to the client MBX
		  and increment the cbk count of the the corresponding hdl node.
		  If for a client, more than one scope matches, then those many
		  no of evts are posted to the MBX.
@param[in]	: client_info - For which filter match to be performed.  
@param[in]	: cbk_evt - Evt received from SMFND.
@return		: NCSCC_RC_FAILURE/NCSCC_RC_SUCCESS.
***************************************************************************/ 
uint32_t smfa_cbk_filter_match(SMFA_CLIENT_INFO *client_info,SMF_CBK_EVT *cbk_evt)
{
	SMFA_SCOPE_INFO *scope_info = client_info->scope_info_list;
	uint32_t rc = NCSCC_RC_FAILURE;
	uint32_t no_of_filters;
	SMF_EVT *evt;
	SMFA_CBK_HDL_LIST *hdl_list = NULL;

	while(scope_info){
		for (no_of_filters=0; no_of_filters < scope_info->scope_of_interest.filtersNumber; no_of_filters++){
			if (NCSCC_RC_SUCCESS == smfa_cbk_filter_type_match(
			&scope_info->scope_of_interest.filters[no_of_filters],&cbk_evt->cbk_label)){
				/* Post to the application MBX.*/
				evt = (SMF_EVT *)calloc(1,sizeof(SMF_EVT));
				if (NULL == evt){
					LOG_ER("SMFA: calloc FAILED, error: %s",strerror(errno));
					osafassert(0);
				}

				evt->evt.cbk_evt.cbk_label.label = (SaUint8T *)calloc(1,
				cbk_evt->cbk_label.labelSize * sizeof(SaUint8T));
				if (NULL == evt->evt.cbk_evt.cbk_label.label){
					LOG_ER("SMFA: calloc FAILED, error: %s",strerror(errno));
					osafassert(0);
				}
				if (cbk_evt->params){
                                        evt->evt.cbk_evt.params = (SaStringT)calloc(1,cbk_evt->params_len + 1);
					if (NULL == evt->evt.cbk_evt.params){
						LOG_ER("SMFA: calloc FAILED, error: %s",strerror(errno));
						osafassert(0);
					}
                                        strncpy(evt->evt.cbk_evt.params,cbk_evt->params,cbk_evt->params_len);
                                        /* calloc above has set the null char at the end of params */
				}

				evt->evt.cbk_evt.cbk_label.labelSize = cbk_evt->cbk_label.labelSize;
				strncpy((char *)evt->evt.cbk_evt.cbk_label.label,(char *)cbk_evt->cbk_label.label,
				cbk_evt->cbk_label.labelSize);

				evt->evt_type = SMF_CLBK_EVT;
				evt->evt.cbk_evt.inv_id = cbk_evt->inv_id;
				evt->evt.cbk_evt.scope_id = scope_info->scope_id;
				evt->evt.cbk_evt.camp_phase = cbk_evt->camp_phase;
				evt->evt.cbk_evt.object_name.length = cbk_evt->object_name.length;
				strncpy((char *)evt->evt.cbk_evt.object_name.value,(char *)cbk_evt->object_name.value,
				cbk_evt->object_name.length);
			
				if (m_NCS_IPC_SEND(&client_info->cbk_mbx,(NCSCONTEXT)evt,NCS_IPC_PRIORITY_NORMAL)){
					/* Increment the cbk count.*/
					if (NULL != hdl_list){
						/* There are two scope id matching for the same hdl.*/
					}else{
						/* First scope id matching for this hdl.*/
						hdl_list = smfa_inv_hdl_add(cbk_evt->inv_id,client_info->client_hdl);
					}
					hdl_list->cnt++;
					rc = NCSCC_RC_SUCCESS;
				}else{
					LOG_ER("SMFA: Posting to MBX failed. hdl: %llu, scoe_id: %u",
					client_info->client_hdl,cbk_evt->scope_id);
				}

				/* If one of the filter matches then go to the next scope id.*/
				break;
			}
		}
		scope_info = scope_info->next_scope;
	}
	return rc;
}

/*************************************************************************** 
@brief		: 1. Find out if there is any pending response from this client
		  be traversing the list cb->cbk_list.
		  2. If yes, then clean up the data structure and check if this 
		  is the last client for which this agent is waiting for the response.
		  3. If yes, then send OK response to SMFND.
@param[in]	: hdl - Handle for which the cbk_list is to be cleaned up. 
@return		: NCSCC_RC_FAILURE/NCSCC_RC_SUCCESS.
***************************************************************************/ 
uint32_t smfa_cbk_list_cleanup(SaSmfHandleT hdl)
{
	SMFA_CB *cb = &_smfa_cb;
	SMFA_CBK_LIST *cbk_list = cb->cbk_list, *prev_cbk = cb->cbk_list;
	SMFA_CBK_HDL_LIST *hdl_list, *prev_hdl;
	SMFSV_EVT resp_evt;
	uint32_t is_cbk_del;

	while (cbk_list){
		is_cbk_del = 0;
		hdl_list = cbk_list->hdl_list;
		prev_hdl = hdl_list;
		while(hdl_list){
			if (hdl == hdl_list->hdl){
				prev_hdl->next_hdl = hdl_list->next_hdl;
				/* Head node deleted. */
				if (prev_hdl->hdl == hdl_list->hdl){
					cbk_list->hdl_list = prev_hdl->next_hdl;
					free(hdl_list);
					/* No more resp expected.*/
					if (NULL == cbk_list->hdl_list){
						/* Send OK resp to SMFND.*/
						{
							/* Populate the resp and send to SMFND.*/
							memset(&resp_evt,0,sizeof(SMFSV_EVT));
							resp_evt.type = SMFSV_EVT_TYPE_SMFND;
							resp_evt.info.smfnd.type = SMFND_EVT_CBK_RSP;
							resp_evt.info.smfnd.event.cbk_req_rsp.evt_type = SMF_RSP_EVT;
							resp_evt.info.smfnd.event.cbk_req_rsp.evt.resp_evt.inv_id = 
							cbk_list->inv_id;;
							resp_evt.info.smfnd.event.cbk_req_rsp.evt.resp_evt.err = 
							SA_AIS_OK;

							if (NCSCC_RC_SUCCESS != smfa_to_smfnd_mds_async_send(
							(NCSCONTEXT)&resp_evt)){
							LOG_ER("SMFA: MDS send to SMFND FAILED.");
							/* TODO: What can we do?? */
							}
						}
						/* Delete the inv node.*/
						prev_cbk->next_cbk = cbk_list->next_cbk;
						is_cbk_del = 1;
						/* inv node deleted from the head of the list.*/
						if (prev_cbk->inv_id == cbk_list->inv_id){
							cb->cbk_list = prev_cbk->next_cbk;
							free(cbk_list);
							cbk_list = cb->cbk_list;
							prev_cbk = cbk_list;
						}else {
							free(cbk_list);
							cbk_list = prev_cbk->next_cbk;
						}
					}
				}else{
					free(hdl_list);
				}
				/* Needed for optimization.*/	
				break; 
			}
			prev_hdl = hdl_list;
			hdl_list = hdl_list->next_hdl;
		}
		if (!cbk_list)
			break;
		if (!is_cbk_del){
			prev_cbk = cbk_list;
			cbk_list = cbk_list->next_cbk;
		}
	}
	return NCSCC_RC_SUCCESS; 
}

/*************************************************************************** 
@brief		: Free SMF_EVT. 
@param[in]	: evt - event structure to be freed. 
***************************************************************************/ 
void smfa_evt_free(SMF_EVT *evt)
{
	if (evt){
		free(evt->evt.cbk_evt.params);
		free(evt->evt.cbk_evt.cbk_label.label);
		free(evt);
	}

	return;
}

/*************************************************************************** 
@brief		: Clean up the MBX before destroying it. Free all the msgs in the MBX.
@param[in]	: arg - 
@param[in]	: msg - 
@return		: true
***************************************************************************/ 
bool smfa_client_mbx_clnup(NCSCONTEXT arg, NCSCONTEXT msg)
{
	SMF_EVT *evt = (SMF_EVT *)msg;
	SMF_EVT *next = evt;

	while(next)
	{
		next = evt->next;
		smfa_evt_free(evt);
		evt = next;
	}
	return true;
}

