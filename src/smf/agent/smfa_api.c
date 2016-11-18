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
 * @file	smfa_api.c
 * @brief	This file contains the APIs which are needed to be supported by
 *		agent in order for the client to communicate with SMF service.
 *
 * @author	GoAhead Software
*****************************************************************************/
#include "osaf/saf/saAis.h"
#include "smf/saf/saSmf.h"
#include "smf/smfsv_defs.h"
#include "smfa.h"
#include <limits.h>

#define MAX_SMFA_HDL LONG_MAX

/* Minor version is ignored.*/
#define m_SMFA_VERSION_VALIDATE(ver) \
		((SMF_RELEASE_CODE == ver->releaseCode) && \
		 (SMF_MAJOR_VERSION >= ver->majorVersion) && \
		 (0x0 < ver->majorVersion) )

SaUint64T  smfa_hdl;
extern SMFA_CB _smfa_cb;

/*************************************************************************** 
@brief		: saSmfInitialize. Even if no cbk is registered, we create 
		  MBX for the client as we have to return the sel obj later.
@param[out]	: smfHandle - Will have a valid handle if initialization is
 		  successful.
@param[in]	: smfCallbacks - Cbks to be registered.
@param[in]	: version - Supported smf-api version.
@return		: SA_AIS_OK if successful otherwise appropiate err code.
*****************************************************************************/
SaAisErrorT saSmfInitialize(SaSmfHandleT *smfHandle, 
			const SaSmfCallbacksT *smfCallbacks,
			SaVersionT *version)
{
	SMFA_CB			*cb = &_smfa_cb;
	SMFA_CLIENT_INFO	*client_info;
	SaAisErrorT		rc = SA_AIS_OK;

	TRACE_ENTER();

	if (!smfHandle || !version){
		LOG_ER("SMFA: smfHandle or version is NULL.");
		return SA_AIS_ERR_INVALID_PARAM;
	}
	
	if (!m_SMFA_VERSION_VALIDATE(version)){
		LOG_ER("SMFA: Unsupported version. ReleaseCode: %c, MajorVersion: %x",
		version->releaseCode,version->majorVersion);
		version->releaseCode = SMF_RELEASE_CODE;
		version->majorVersion = SMF_MAJOR_VERSION;
		version->minorVersion = SMF_MINOR_VERSION;
		TRACE_LEAVE();
		return SA_AIS_ERR_VERSION;
	}
	version->minorVersion = SMF_MINOR_VERSION;

	if (NCSCC_RC_SUCCESS != smfa_init()){
		LOG_ER("smfa_init() FAILED.");
		ncs_agents_shutdown();
		TRACE_LEAVE();
		return SA_AIS_ERR_LIBRARY;	
	}
		
	client_info = (SMFA_CLIENT_INFO *)calloc(1,sizeof(SMFA_CLIENT_INFO));
	if (NULL == client_info){
		LOG_ER("SMFA client info: calloc FAILED, error: %s",strerror(errno));
		rc = SA_AIS_ERR_NO_MEMORY;
		goto smfa_init_failed;
	}
	
	/* Fill the handle.*/
	if ( MAX_SMFA_HDL <= smfa_hdl){
		smfa_hdl = 0;
	}
	smfa_hdl++;
	client_info->client_hdl = smfa_hdl;
	 
	/* Fill the callback info. */
	if (NULL != smfCallbacks)
		client_info->reg_cbk = *smfCallbacks;
	else
		TRACE_2("SMFA: No cbk registered.");

	/* Create MBX for this client.*/
	if (NCSCC_RC_SUCCESS != m_NCS_IPC_CREATE(&client_info->cbk_mbx)){
		LOG_ER("SMFA: MBX create FAILED.");
		rc = SA_AIS_ERR_LIBRARY;
		goto smfa_mbx_create_failed;
	}else {
		if (NCSCC_RC_SUCCESS != m_NCS_IPC_ATTACH(&client_info->cbk_mbx)){
			LOG_ER("SMFA: MBX attach FAILED.");
			rc = SA_AIS_ERR_LIBRARY;
			goto smfa_mbx_attach_failed;
		}
	}
	
	/* Add the client info to the database. */
	if (NCSCC_RC_SUCCESS != m_NCS_LOCK(&cb->cb_lock,NCS_LOCK_WRITE)){
		LOG_ER("SMFA: Cb lock acquire FAILED.");
		rc = SA_AIS_ERR_NO_RESOURCES;
		goto smfa_take_lock_failed;
	}
	smfa_client_info_add(client_info);
	m_NCS_UNLOCK(&cb->cb_lock,NCS_LOCK_WRITE);
	
	/* Initialization is successful. Pass the handle to the application.*/
	*smfHandle = client_info->client_hdl;
	TRACE_2("SMFA: Handle returned: %llu.",*smfHandle);
	TRACE_LEAVE();
	return SA_AIS_OK;
	
smfa_take_lock_failed:
	m_NCS_IPC_DETACH(&client_info->cbk_mbx,smfa_client_mbx_clnup,client_info);
smfa_mbx_attach_failed:
	m_NCS_IPC_RELEASE(&client_info->cbk_mbx,NULL);
smfa_mbx_create_failed:
	if (client_info){
		free(client_info);
		client_info = NULL;
	}
smfa_init_failed:
	smfa_finalize();
	TRACE_LEAVE();
	return rc;
}

/*************************************************************************** 
@brief		: saSmfSelectionObjectGet. If this api is called twice, same
		  sel obj will be returned. 
@param[in]	: smfHandle - Handle returned by successful intialize. 
@param[out]	: selectionObject - Will have a valid sel obj in successful
		  return of the api.
@return		: SA_AIS_OK if successful otherwise appropiate err code.
*****************************************************************************/
SaAisErrorT saSmfSelectionObjectGet( SaSmfHandleT smfHandle,
				SaSelectionObjectT *selectionObject)
{
	SMFA_CB *cb = &_smfa_cb;
	SMFA_CLIENT_INFO *client_info;
	
	TRACE_ENTER2("SMFA: Handle: %llu.",smfHandle);
	if (NULL == selectionObject){
		LOG_ER("SMFA: selectionObject is NULL.");
		TRACE_LEAVE();
		return SA_AIS_ERR_INVALID_PARAM;
	}
	
	/* To check if finalize is already called.*/
	if (cb->is_finalized){
		LOG_ER("SMFA: Already finalized, Bad handle: %llu.",smfHandle);
		TRACE_LEAVE();
		return SA_AIS_ERR_BAD_HANDLE;
	}
	
	if (NCSCC_RC_SUCCESS != m_NCS_LOCK(&cb->cb_lock,NCS_LOCK_READ)){
		LOG_ER("SMFA: Cb lock acquire FAILED.");
		TRACE_LEAVE();
		return SA_AIS_ERR_NO_RESOURCES;
	}
	/* Get the client info structure for the handle.*/ 
	client_info = smfa_client_info_get(smfHandle);
	if (NULL == client_info){
		LOG_ER("SMFA: Could not retrieve client info, Bad handle: %llu",smfHandle);
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_READ);
		TRACE_LEAVE();
		return SA_AIS_ERR_BAD_HANDLE;
	}
	
	*selectionObject = (SaSelectionObjectT)m_GET_FD_FROM_SEL_OBJ(
			m_NCS_IPC_GET_SEL_OBJ(&client_info->cbk_mbx));
	
	m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_READ);
	TRACE_LEAVE();
	return SA_AIS_OK;
}

/*************************************************************************** 
@brief		: saSmfCallbackScopeRegister. If no cbk is registered, then 
		  call to this api fails.
@param[in]	: smfHandle - Handle returned by successful intialize. 
@param[in]	: scopeId - Scope id provided by the client. 
@param[in]	: scopeOfInterest - Set of filters registered.
@return		: SA_AIS_OK if successful otherwise appropiate err code.
*****************************************************************************/
SaAisErrorT saSmfCallbackScopeRegister( SaSmfHandleT smfHandle, 
				SaSmfCallbackScopeIdT scopeId,
				const SaSmfLabelFilterArrayT *scopeOfInterest)
{
	SMFA_CB *cb = &_smfa_cb;
	SMFA_CLIENT_INFO *client_info;
	SMFA_SCOPE_INFO *scope_info;
	uint32_t no_of_filters = 0;
	uint32_t size_of_label = 0;

	TRACE_ENTER2("SMFA:Handle %llu, scopeId: %u",smfHandle,scopeId);
	if (NULL == scopeOfInterest){
		LOG_ER("SMFA: scopeOfInterest is NULL.");
		TRACE_LEAVE();
		return SA_AIS_ERR_INVALID_PARAM;
	}
	/* Should we still add the scope id to the client db??*/
	if (0 >= scopeOfInterest->filtersNumber){
		LOG_ER("SMFA: filtersNumber is ZERO.");
		TRACE_LEAVE();
		return SA_AIS_ERR_INVALID_PARAM;
	}
	
	if (cb->is_finalized){
		LOG_ER("SMFA: Called after finalize, Bad handle.");
		TRACE_LEAVE();
		return SA_AIS_ERR_BAD_HANDLE;
	}
	
	if (NCSCC_RC_SUCCESS != m_NCS_LOCK(&cb->cb_lock,NCS_LOCK_WRITE)){
		LOG_ER("SMFA: Cb lock acquire FAILED.");
		TRACE_LEAVE();
		return SA_AIS_ERR_NO_RESOURCES;
	}
	
	/* Get the client info structure for the handle.*/
	client_info = smfa_client_info_get(smfHandle);
	if (NULL == client_info){
		LOG_ER("SMFA: Bad handle %llu.",smfHandle);
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
		TRACE_LEAVE();
		return SA_AIS_ERR_BAD_HANDLE;
	}

	/* If no callback is registered during initialization.*/
	if (NULL == client_info->reg_cbk.saSmfCampaignCallback){
		LOG_ER("SMFA: No cbk registered.");
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
		TRACE_LEAVE();
		return SA_AIS_ERR_INIT;
	}

	/* Check if the scope id exists.*/
	if (NULL != smfa_scope_info_get(client_info,scopeId)){
		LOG_ER("SMFA: Scope id exists: %x",scopeId);
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
		TRACE_LEAVE();
		return SA_AIS_ERR_EXIST;
	}
	
	scope_info = (SMFA_SCOPE_INFO *)calloc(1,sizeof(SMFA_SCOPE_INFO));	
	if (NULL == scope_info){
		LOG_ER("SMFA scope info: calloc FAILED, error: %s",strerror(errno));
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
		TRACE_LEAVE();
		return SA_AIS_ERR_NO_MEMORY; 
	}
	
	/* Cpy the filter info if mentioned.*/
	scope_info->scope_of_interest.filtersNumber = scopeOfInterest->filtersNumber;
	scope_info->scope_of_interest.filters = (SaSmfLabelFilterT *)
	calloc(scopeOfInterest->filtersNumber,sizeof(SaSmfLabelFilterT));
	if (NULL == scope_info->scope_of_interest.filters){
		LOG_ER("SMFA filters: calloc FAILED, error: %s",strerror(errno));
		smfa_scope_info_free(scope_info);
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
		TRACE_LEAVE();
		return SA_AIS_ERR_NO_MEMORY;
	}
	
	for (no_of_filters=0; no_of_filters < scopeOfInterest->filtersNumber; no_of_filters++){
		
		scope_info->scope_of_interest.filters[no_of_filters].filterType = 
		scopeOfInterest->filters[no_of_filters].filterType;
		
		size_of_label = scopeOfInterest->filters[no_of_filters].filter.labelSize;

		scope_info->scope_of_interest.filters[no_of_filters].filter.label = 
		(SaUint8T *)calloc(1,size_of_label);
		/* Fail the API.*/
		if (NULL == scope_info->scope_of_interest.filters[no_of_filters].filter.label){
			LOG_ER("SMFA filter label: calloc FAILED, error: %s",strerror(errno));
			smfa_scope_info_free(scope_info);
			m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
			TRACE_LEAVE();
			return SA_AIS_ERR_NO_MEMORY;
		}

		scope_info->scope_of_interest.filters[no_of_filters].filter.labelSize = size_of_label;
		
		memcpy(scope_info->scope_of_interest.filters[no_of_filters].filter.label,
		scopeOfInterest->filters[no_of_filters].filter.label,size_of_label);
	}
	/* Store scope id.*/
	scope_info->scope_id = scopeId;

	/*Add the scope info to the client db.*/
	smfa_scope_info_add(client_info,scope_info);	
	
	m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
	TRACE_LEAVE();
	return SA_AIS_OK;
}

/*************************************************************************** 
@brief		: saSmfDispatch 
@param[in]	: smfHandle - Handle returned by successful intialize. 
@param[in]	: dispatchFlags - Dispatch flag. 
@return		: SA_AIS_OK if successful otherwise appropiate err code.
*****************************************************************************/
SaAisErrorT saSmfDispatch(
		SaSmfHandleT smfHandle,
		SaDispatchFlagsT dispatchFlags)
{	
	
	SMFA_CB *cb = &_smfa_cb;
	SMFA_CLIENT_INFO *client_info;
	SaAisErrorT rc = SA_AIS_OK;

	TRACE_ENTER2("SMFA: Handle %llu.",smfHandle);
	if (cb->is_finalized){
		LOG_ER("SMFA: Already finalized, Bad handle: %llu.",smfHandle);
		TRACE_LEAVE();
		return SA_AIS_ERR_BAD_HANDLE;
	}
	
	/* To protect the finalize during dispatch.*/
	if (NCSCC_RC_SUCCESS != m_NCS_LOCK(&cb->cb_lock,NCS_LOCK_READ)){
		LOG_ER("SMFA: Cb lock acquire FAILED.");
		TRACE_LEAVE();
		return SA_AIS_ERR_NO_RESOURCES;
	}
	
	/* Get the client info structure for the handle.*/
	client_info = smfa_client_info_get(smfHandle);
	if (NULL == client_info){
		LOG_ER("SMFA: Bad handle.");
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_READ);
		TRACE_LEAVE();
		return SA_AIS_ERR_BAD_HANDLE;
	}
	m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_READ);

	/* Validate the flags and invoke corresponding functions.*/
	switch(dispatchFlags){
		case SA_DISPATCH_ONE:
			rc = smfa_dispatch_cbk_one(client_info);
			break;
		case SA_DISPATCH_ALL:
			rc = smfa_dispatch_cbk_all(client_info);
			break;
		case SA_DISPATCH_BLOCKING:
			rc = smfa_dispatch_cbk_block(client_info);
			break;
		default:
			LOG_ER("SMFA: Invalid flag: %d",dispatchFlags);
			rc = SA_AIS_ERR_INVALID_PARAM;
	}
	TRACE_LEAVE();
	return rc;
}

/*************************************************************************** 
@brief		: saSmfResponse
@param[in]	: smfHandle - Handle returned by successful intialize. 
@param[in]	: invocation - inv is provided by SMF in the cbk. The same
		  inv id is to be passed in the response.
@param[in]	: error - err code returned by the application in response 
		  to the cbk.
@return		: SA_AIS_OK if successful otherwise appropiate err code.
*****************************************************************************/
SaAisErrorT saSmfResponse(
		SaSmfHandleT smfHandle,
		SaInvocationT invocation,
		SaAisErrorT error)
{
	SMFA_CB *cb = &_smfa_cb;
	SMFSV_EVT resp_evt;
	SMFA_CLIENT_INFO *client_info;
	
	TRACE_ENTER2("SMFA: Response: Hdl : %llu, Inv_id: %llu, err: %d",smfHandle,invocation,error);
	if (cb->is_finalized){
		LOG_ER("SMFA: Response is called after finalized.");
		TRACE_LEAVE();
		return SA_AIS_ERR_BAD_HANDLE;
	}

	/* If the SMFND is not up, then return TRY_AGAIN.*/
	if (!cb->is_smfnd_up){
		LOG_ER("SMFA: SMFND is not UP.");
		TRACE_LEAVE();
		return SA_AIS_ERR_TRY_AGAIN;
	}
	
	if ((SA_AIS_ERR_NOT_SUPPORTED != error) && (SA_AIS_ERR_FAILED_OPERATION != error) && (SA_AIS_OK != error)){
		LOG_ER("SMFA: Invalid response. %d",error);
		TRACE_LEAVE();
		return SA_AIS_ERR_INVALID_PARAM;
	}
	
	if (!invocation){
		LOG_ER("SMFA: Invocation id is Zero.");
		TRACE_LEAVE();
		return SA_AIS_ERR_INVALID_PARAM;

	}

	if (NCSCC_RC_SUCCESS != m_NCS_LOCK(&cb->cb_lock,NCS_LOCK_WRITE)){
		LOG_ER("SMFA: Cb lock acquire FAILED.");
		TRACE_LEAVE();
		return SA_AIS_ERR_NO_RESOURCES;
	}
	
	/* Get the client info structure for the handle.*/
	client_info = smfa_client_info_get(smfHandle);
	if (NULL == client_info){
		LOG_ER("SMFA: Bad handle.");
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
		TRACE_LEAVE();
		return SA_AIS_ERR_BAD_HANDLE;
	}
	
	/* Drop the duplicate/delay response.*/
	if (NULL == cb->cbk_list){
		LOG_ER("SMFA: Duplicate/Delay/Invalid resp: Hdl : %llu, Inv_id: %llu",smfHandle,invocation);
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
		TRACE_LEAVE();
		/* The correct return code is NOT_EXIST but this is not part of this spec version. 
		 We are returning INVALID_PARAM. */
		return SA_AIS_ERR_INVALID_PARAM;
	}
	
	if (SA_AIS_ERR_FAILED_OPERATION == error){
		TRACE_2("SMFA: Not OK resp from hdl: %llu for inv: %llu.",smfHandle,invocation);
		/* Clean up every thing related to this inv id.*/
		if (NCSCC_RC_SUCCESS != smfa_cbk_err_resp_process(invocation,smfHandle)){
			LOG_ER("SMFA: Duplicate/delay/invalid response. Hdl: %llu, inv: %llu",smfHandle,invocation);
			m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
			TRACE_LEAVE();
			return SA_AIS_ERR_INVALID_PARAM;
		}
	}else{
		TRACE_2("SMFA: OK resp from hdl: %llu for inv: %llu.",smfHandle,invocation);
		/* Preocess the response before sending to ND.*/
		if (NCSCC_RC_SUCCESS != smfa_cbk_ok_resp_process(smfHandle,invocation)) {
			LOG_ER("SMFA: Duplicate/delay/invalid response. Hdl: %llu, inv: %llu",smfHandle,invocation);
			m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
			TRACE_LEAVE();
			return SA_AIS_ERR_INVALID_PARAM;
		}
	}

	TRACE_2("SMFA: Sending consolidated resp to ND for hdl: %llu for inv: %llu.",smfHandle,invocation);
	/* Populate the resp and send to SMFND.*/
	memset(&resp_evt,0,sizeof(SMFSV_EVT));
	resp_evt.type = SMFSV_EVT_TYPE_SMFND;
	resp_evt.info.smfnd.type = SMFND_EVT_CBK_RSP;
	resp_evt.info.smfnd.event.cbk_req_rsp.evt_type = SMF_RSP_EVT;
	resp_evt.info.smfnd.event.cbk_req_rsp.evt.resp_evt.inv_id = invocation;
	resp_evt.info.smfnd.event.cbk_req_rsp.evt.resp_evt.err = error;

	if (NCSCC_RC_SUCCESS != smfa_to_smfnd_mds_async_send((NCSCONTEXT)&resp_evt)){
		LOG_ER("SMFA: MDS send to SMFND FAILED.");
		/* TODO: What can we do?? We have already cleaned up our database.*/
	}

	m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
	TRACE_LEAVE();
	return SA_AIS_OK;
}

/*************************************************************************** 
@brief		: saSmfCallbackScopeUnregister. No cbk will be called for
		  the client after successful retutn of this api.
@param[in]	: smfHandle - Handle returned by successful intialize. 
@param[in]	: scopeId - Filters for this scope id will be unregistered.
		  inv id is to be passed in the response.
@return		: SA_AIS_OK if successful otherwise appropiate err code.
*****************************************************************************/
SaAisErrorT saSmfCallbackScopeUnregister( SaSmfHandleT smfHandle,
				SaSmfCallbackScopeIdT scopeId)
{
	SMFA_CB *cb = &_smfa_cb;
	SMFA_CLIENT_INFO *client_info;

	TRACE_ENTER2("Handle: %llu, Scope: %u.",smfHandle,scopeId);
	if (cb->is_finalized){
		LOG_ER("SMFA: Already finalized, Bad handle %llu.",smfHandle);
		TRACE_LEAVE();
		return SA_AIS_ERR_BAD_HANDLE;
	}

	if (NCSCC_RC_SUCCESS != m_NCS_LOCK(&cb->cb_lock,NCS_LOCK_WRITE)){
		LOG_ER("SMFA: Cb lock acquire FAILED.");
		TRACE_LEAVE();
		return SA_AIS_ERR_NO_RESOURCES;
	}
	
	/* Get the client info structure for the handle.*/ 
	client_info = smfa_client_info_get(smfHandle);
	if (NULL == client_info){
		LOG_ER("SMFA: Bad handle.");
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
		TRACE_LEAVE();
		return SA_AIS_ERR_BAD_HANDLE;
	}

	/* Check if the scope id exists. Remove if exists otherwise return NOT_EXIST.*/
	if (NCSCC_RC_FAILURE == smfa_scope_info_rmv(client_info,scopeId)){
		LOG_ER("SMFA: Scope id %x does not exist",scopeId);
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
		TRACE_LEAVE();
		return SA_AIS_ERR_NOT_EXIST;
	}

	m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
	TRACE_LEAVE();
	return SA_AIS_OK;
}

/*************************************************************************** 
@brief		: saSmfFinalize 
@param[in]	: smfHandle - Handle returned by successful intialize. 
@return		: SA_AIS_OK if successful otherwise appropiate err code.
*****************************************************************************/
SaAisErrorT saSmfFinalize(SaSmfHandleT smfHandle)
{
	SMFA_CB *cb = &_smfa_cb;
	SMFA_CLIENT_INFO *client_info;
	
	TRACE_ENTER2("Handle: %llu",smfHandle);
	if (cb->is_finalized){
		LOG_ER("SMFA: Already finalized. Bad handle %llu.",smfHandle);
		TRACE_LEAVE();
		return SA_AIS_ERR_BAD_HANDLE;
	}

	if (NCSCC_RC_SUCCESS != m_NCS_LOCK(&cb->cb_lock,NCS_LOCK_WRITE)){
		LOG_ER("SMFA: Cb lock acquire FAILED.");
		TRACE_LEAVE();
		return SA_AIS_ERR_NO_RESOURCES;
	}
	
	/* Get the client info structure for the handle.*/
	client_info = smfa_client_info_get(smfHandle);
	if (NULL == client_info){
		LOG_ER("SMFA: Could not retrieve client info, Bad handle %llu.",smfHandle);
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
		TRACE_LEAVE();
		return SA_AIS_ERR_BAD_HANDLE;
	}
	/* Free all the scope info registered for this client.*/
	smfa_client_info_clean(client_info);
	
	/* Release the MBX.*/
	m_NCS_IPC_DETACH(&client_info->cbk_mbx,smfa_client_mbx_clnup,client_info);
	m_NCS_IPC_RELEASE(&client_info->cbk_mbx,NULL);

	/* Clear the list cb->cbk_list for this handle.*/
	smfa_cbk_list_cleanup(smfHandle);

	/* Remove the client from the cb.*/
	smfa_client_info_rmv(client_info->client_hdl);
	
	m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
	
	/* If last client, then destroy the agent CB.*/
	if (NCSCC_RC_SUCCESS != smfa_finalize()){
		TRACE_LEAVE();
		return SA_AIS_ERR_LIBRARY;
	}

	TRACE_LEAVE();
	return SA_AIS_OK;
}
