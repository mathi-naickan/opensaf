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
/*************************************************************************//** 
 * @file	plma_api.c
 * @brief	This file contains the APIs which are needed to be supported by
 *		agent in order for the client to communicate with PLM service.
 *		All the below calls will be implemented as sync calls from Agent
 *		to Server.
 *
 * @author	Emerson Network Power
*****************************************************************************/


#include "plma.h"

#define PLMS_MDS_SYNC_TIME 1000000 

#define m_PLM_DISPATCH_FLAG_IS_VALID(flags) \
	((flags & SA_DISPATCH_ONE) ||       \
	 (flags & SA_DISPATCH_ALL) ||       \
	 (flags & SA_DISPATCH_BLOCKING))
/** Macro to validate track flags */
#define m_VALIDATE_TRACK_FLAGS(attr) \
	(((attr & SA_TRACK_CHANGES) && (attr & SA_TRACK_CHANGES_ONLY)) || \
        !((attr & SA_TRACK_CHANGES) || (attr & SA_TRACK_CHANGES_ONLY) || \
	(attr & SA_TRACK_CURRENT)))

#define m_PLM_IS_SA_TRACK_CHANGES_SET(flags) (flags & SA_TRACK_CHANGES)
#define m_PLM_IS_SA_TRACK_CHANGES_ONLY_SET(flags) \
		(flags & SA_TRACK_CHANGES_ONLY)
#define m_PLM_IS_SA_TRACK_CURRENT_SET(flags) (flags & SA_TRACK_CURRENT)

#define  PLMA_RELEASE_CODE  'A'
#define	 PLMA_MAJOR_VERSION 0x01
#define  PLMA_MINOR_VERSION 0x02

/** Macro to validate the PLM version */
#define m_PLM_VER_IS_VALID(ver) \
              ( (ver->releaseCode == PLMA_RELEASE_CODE) && \
                ((ver->majorVersion <= PLMA_MAJOR_VERSION) && \
				(ver->majorVersion > 0)) )


uint32_t plm_process_dispatch_cbk(PLMA_CLIENT_INFO *client_info,
                               SaDispatchFlagsT  flags);

/***********************************************************************//**
* @brief	Routine for freeing entities allocated in a previous call to 
*		the saPlmReadinessTrack()
*
* @param[in,out]  listHead	pointer to the head of the  list of array of 
* 				entities added by using SaPlmReadinessTrack() 
*				function.    
* @param[in]      entities	A pointer to the array of entities that was
*				allocated by the PLM Service library in the
*				SaPlmReadinessTrack() function.
*
* @return   	  NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE. 
***************************************************************************/
SaAisErrorT  plm_add_entity_addr_to_list(PLMA_RDNS_TRK_MEM_LIST** listHead, 
			SaPlmReadinessTrackedEntityT* entities)
{
	PLMA_RDNS_TRK_MEM_LIST *current, *new;
	current = *listHead;
	
	TRACE_ENTER();
	new = (PLMA_RDNS_TRK_MEM_LIST *)
				malloc(sizeof(PLMA_RDNS_TRK_MEM_LIST));	
	if(!new){
		LOG_CR("PLMA : PLMA_RDNS_TRK_MEM_LIST memory alloc failed," 
			       "error val:%s",strerror(errno));
		return NCSCC_RC_FAILURE;
	}
	new->entities = entities;
	new->next = NULL;
	
	if (*listHead == NULL){
		/** see if its an empty list */
		*listHead = new;
	}else{
		while(current->next != NULL){
			current = current->next;
		}
		/** add it to the end of the list */
		current->next = new;
	}
	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}


/***********************************************************************//**
* @brief	This routine invokes a single pending callback in the context
*		calling thread, and then return from the dispatch.
*
* @param[in]	client_info -	A pointer to the PLMA_CLIENT_INFO structure.
*
* @return	NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE. 
***************************************************************************/
uint32_t plm_process_single_dispatch(PLMA_CLIENT_INFO *client_info)
{
	uint32_t proc_rc = NCSCC_RC_SUCCESS;
	/** Info. for callback */   
	PLMS_EVT *msg = NULL;
	SaPlmEntityGroupHandleT grp_hdl;
	PLMA_CB *plma_cb = plma_ctrlblk;
	PLMA_ENTITY_GROUP_INFO *group_info;
	
	TRACE_ENTER();
	
	/** Receive a message from the mailbox */
	if(NULL != (msg = (PLMS_EVT *) m_NCS_IPC_NON_BLK_RECEIVE
                                (&client_info->callbk_mbx,NULL)))
	{
		/** Process the message */
		if ((msg->req_res == PLMS_REQ) && 
		    (PLMS_AGENT_TRACK_EVT_T == msg->req_evt.req_type)){
			grp_hdl = msg->req_evt.agent_track.grp_handle;
			
			group_info = (PLMA_ENTITY_GROUP_INFO *)
			ncs_patricia_tree_get(&plma_cb->entity_group_info,
					      (uint8_t *)&grp_hdl);
			if(!group_info){
				LOG_ER("PLMA : INVALID GRP HDL.");
				goto end;
			}
			if(group_info->trk_strt_stop){
				if (msg->req_evt.agent_track.evt_type ==
						PLMS_AGENT_TRACK_CBK_EVT) {
					if(client_info->track_callback.saPlmReadinessTrackCallback){
						/** Invoke the callback */
						client_info->track_callback.saPlmReadinessTrackCallback(grp_hdl,
			msg->req_evt.agent_track.track_cbk.track_cookie,
			msg->req_evt.agent_track.track_cbk.invocation_id,
			msg->req_evt.agent_track.track_cbk.track_cause,
			msg->req_evt.agent_track.track_cbk.root_cause_entity.length ? 
                        &msg->req_evt.agent_track.track_cbk.root_cause_entity : NULL,
			msg->req_evt.agent_track.track_cbk.root_correlation_id,
			&msg->req_evt.agent_track.track_cbk.tracked_entities,
			msg->req_evt.agent_track.track_cbk.change_step,
			SA_AIS_OK);
						TRACE_5("PLMA: Callback invoked. Dispatch flag : SA_DISPATCH_ONE");
					}
				}
			}
		}else{
			LOG_ER("PLMA : INVALID MSG FORMAT");
			goto end;
		}

	}
end:
	/** free the memory allocated for msg */
	if(msg){
		plms_free_evt(msg);
	}
	TRACE_LEAVE();
	return proc_rc;
}

/***********************************************************************//**
* @brief	This routine invokes all of the pending callbacks in the 
*		context of the calling thread if callbacks are pending before 
*		returning from dispatch.
*
* @param[in]	client_info -	A pointer to the PLMA_CLIENT_INFO structure.
*
* @return	NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE. 
***************************************************************************/
uint32_t plm_process_dispatch_all(PLMA_CLIENT_INFO *client_info)
{
	uint32_t proc_rc = NCSCC_RC_SUCCESS;
        /** Info. for callback */
        PLMS_EVT *msg;
	PLMA_CB *plma_cb = plma_ctrlblk;
	PLMA_ENTITY_GROUP_INFO *group_info;
        SaPlmEntityGroupHandleT grp_hdl;

	TRACE_ENTER();

        /** Receive a message from the mailbox */
        while(NULL != (msg = (PLMS_EVT *) m_NCS_IPC_NON_BLK_RECEIVE
                                (&client_info->callbk_mbx,NULL)))
        {
		/** Process the message here */
		if ((msg->req_res == PLMS_REQ) && 
		    (PLMS_AGENT_TRACK_EVT_T == msg->req_evt.req_type)){
		
			grp_hdl = msg->req_evt.agent_track.grp_handle;
			group_info = (PLMA_ENTITY_GROUP_INFO *)
			ncs_patricia_tree_get(&plma_cb->entity_group_info,
					      (uint8_t *)&grp_hdl);
			if(!group_info){
				LOG_ER("PLMA : INVALID GRP HDL.");
				plms_free_evt(msg);
				continue;	
			}
			if(group_info->trk_strt_stop){
				if (msg->req_evt.agent_track.evt_type == 
						PLMS_AGENT_TRACK_CBK_EVT){
					if (client_info->track_callback.saPlmReadinessTrackCallback){
		
						/** Invoke the callback */
						client_info->track_callback.saPlmReadinessTrackCallback(grp_hdl,
			msg->req_evt.agent_track.track_cbk.track_cookie,
			msg->req_evt.agent_track.track_cbk.invocation_id,
			msg->req_evt.agent_track.track_cbk.track_cause,
                        msg->req_evt.agent_track.track_cbk.root_cause_entity.length ?
			&msg->req_evt.agent_track.track_cbk.root_cause_entity : NULL,
			msg->req_evt.agent_track.track_cbk.root_correlation_id,
			&msg->req_evt.agent_track.track_cbk.tracked_entities,
			msg->req_evt.agent_track.track_cbk.change_step,
			SA_AIS_OK);
						TRACE_5("PLMA: Callback invoked. Dispatch flag : SA_DISPATCH_ALL");
					}
				}
			}
		}else{
			LOG_ER("PLMA : INVALID MSG FORMAT");
		}
			
		plms_free_evt(msg);
	}
	
	TRACE_LEAVE();
	
	return proc_rc;
}

/***********************************************************************//**
* @brief	One or more threads calling dispatch remain within dispatch
*		and execute callbacks as they become pending. The thread or
*		threads do not return from dispatch until the corresponding
*		finalize function is executed by one thread of the process.
*
* @param[in]	client_info -	A pointer to the PLMA_CLIENT_INFO structure
*
* @return	NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE. 
***************************************************************************/
uint32_t plm_process_blocking_dispatch(PLMA_CLIENT_INFO *client_info)
{
	uint32_t proc_rc = NCSCC_RC_SUCCESS;
        PLMS_EVT *msg;
        SaPlmEntityGroupHandleT grp_hdl;
	PLMA_CB *plma_cb = plma_ctrlblk;
	PLMA_ENTITY_GROUP_INFO *group_info;
	
	TRACE_ENTER();

	/** Receive a message from the mailbox */
	while (NULL != (msg = (PLMS_EVT *)m_NCS_IPC_RECEIVE(&client_info->callbk_mbx, NULL)))
	{
		/** Process the message here */
		if ((msg->req_res == PLMS_REQ) && 
		    (PLMS_AGENT_TRACK_EVT_T == msg->req_evt.req_type)){
		
			grp_hdl = msg->req_evt.agent_track.grp_handle;
			group_info = (PLMA_ENTITY_GROUP_INFO *)
			ncs_patricia_tree_get(&plma_cb->entity_group_info,
					      (uint8_t *)&grp_hdl);
			if(!group_info){
				LOG_ER("PLMA : INVALID GRP HDL.");
				plms_free_evt(msg);
				continue;	
			}
			if(group_info->trk_strt_stop){
				if (msg->req_evt.agent_track.evt_type == PLMS_AGENT_TRACK_CBK_EVT){
					if (client_info->track_callback.saPlmReadinessTrackCallback){
						client_info->track_callback.saPlmReadinessTrackCallback(grp_hdl,
			msg->req_evt.agent_track.track_cbk.track_cookie,
			msg->req_evt.agent_track.track_cbk.invocation_id,
			msg->req_evt.agent_track.track_cbk.track_cause,
                        msg->req_evt.agent_track.track_cbk.root_cause_entity.length ?
			&msg->req_evt.agent_track.track_cbk.root_cause_entity : NULL,
			msg->req_evt.agent_track.track_cbk.root_correlation_id,
			&msg->req_evt.agent_track.track_cbk.tracked_entities,
			msg->req_evt.agent_track.track_cbk.change_step,
			SA_AIS_OK);
						TRACE_5("PLMA: Callback invoked. Dispatch flag : SA_DISPATCH_BLOCKING");
					}
				}
			}
		}else{
			LOG_ER("PLMA : INVALID MSG FORMAT");
		}
	
		plms_free_evt(msg);
	}

	TRACE_LEAVE();
	return proc_rc;
}

/***********************************************************************//**
* @brief	Routine to despatch the call back based on the flags.
*
* @param[in]	client_info -	A pointer to the PLMA_CLIENT_INFO structure.
* @param[in]	flags	    -   Flags to specify the call back execution 
*				behavior.
*
* @return	NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE. 
***************************************************************************/
uint32_t plm_process_dispatch_cbk(
                         PLMA_CLIENT_INFO *client_info,
                         SaDispatchFlagsT  flags)
{
	uint32_t proc_rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER();
	switch (flags)
	{
	case SA_DISPATCH_ONE:
		proc_rc = plm_process_single_dispatch(client_info);
		break;
	case SA_DISPATCH_ALL:
		proc_rc = plm_process_dispatch_all(client_info);
		break;
	case SA_DISPATCH_BLOCKING:
		proc_rc = plm_process_blocking_dispatch(client_info);
		break;
	default:
		LOG_ER("PLMA : INVALID DISPATCH FLAG");
		proc_rc = NCSCC_RC_FAILURE;
		break;
	}
	TRACE_LEAVE();
	return proc_rc;
}

/***********************************************************************//**
* @brief		This function initializes the PLM Service for the 
*			invoking process and registers the various callback 
*			functions. The handle 'plmHandle' is returned as the 
*			reference to this association between the process and
*			the PLM Service.
*
* @param[out]		plmHandle    -	A pointer to the handle designating this
*					particular initialization of the PLM 
*					service, that is to be returned by the 
*					PLM service.
* @param[in]		plmCallbacks -  Pointer to a SaPlmCallbacksT structure,
*					containing the callback functions of 
*					the process that the PLM Service may 
*					invoke.
* @param[in,out]	version      -  A pointer to the version of the PLM 
*					service that the invoking process 
*					is using.
*
* @return		Refer to SAI-AIS specification for various return 
*			values. 
***************************************************************************/
SaUint32T saPlmInitialize  (SaPlmHandleT *plmHandle,
		      const SaPlmCallbacksT *plmCallbacks,
		      SaVersionT *version)

{
	PLMA_CB           *plma_cb;
	PLMA_CLIENT_INFO  *client_info;
	PLMS_EVT          plm_init_evt;
	PLMS_EVT          *plm_init_resp = NULL;
	PLMS_EVT          plm_fin_evt, *plm_fin_res;
	uint32_t             proc_rc = NCSCC_RC_SUCCESS;
	SaUint32T         rc = SA_AIS_OK;


	TRACE_ENTER();

	if ( !plmHandle || !version )
	{
		LOG_ER("PLMA : INVALID HANDLE/INPUT VERSION");
		rc = SA_AIS_ERR_INVALID_PARAM;
		goto end;
	}

	/* Initialize the environment */
	if (ncs_agents_startup() != NCSCC_RC_SUCCESS) {
		LOG_ER("PLMA : AGENTS STARTUP FAILED");
		rc = SA_AIS_ERR_LIBRARY;
		goto end;
	}
					
	/* Create and Initialize PLMA CB */
	if (ncs_plma_startup() != NCSCC_RC_SUCCESS) {
		LOG_ER("PLMA : PLMA LIB INIT FAILED");
		rc =  SA_AIS_ERR_LIBRARY;
		goto plma_start_fail;
	}
	plma_cb = plma_ctrlblk;
							       
	TRACE_5(" Version received: %c %d %d", version->releaseCode, version->majorVersion, version->minorVersion);
	/* validate the version */
	if (!m_PLM_VER_IS_VALID(version)){

		LOG_ER("PLMA : INVALID VERSION");
		/** fill the supported version no */
		version->releaseCode = PLMA_RELEASE_CODE;
		version->majorVersion = PLMA_MAJOR_VERSION;
		version->minorVersion = PLMA_MINOR_VERSION;
		rc = SA_AIS_ERR_VERSION;
		goto plma_start_fail;
	}
	/** fill the supported version no */
	version->releaseCode = PLMA_RELEASE_CODE;
	version->majorVersion = PLMA_MAJOR_VERSION;
	version->minorVersion = PLMA_MINOR_VERSION;
	
	if(!plma_cb->plms_svc_up){
		LOG_ER("PLMA : PLM SERVICE DOWN");
		rc = SA_AIS_ERR_TRY_AGAIN;
		goto plma_start_fail;
	}
	
	/** Allocate memory for Client Info */
	client_info = (PLMA_CLIENT_INFO *)malloc(sizeof(PLMA_CLIENT_INFO));
	memset(client_info, 0, sizeof(PLMA_CLIENT_INFO));

	if (!client_info)
	{
		LOG_CR("PLMA : PLMA_CLIENT_INFO memory alloc failed," 
			       "error val:%s",strerror(errno));
		rc = SA_AIS_ERR_NO_MEMORY;
		goto plma_start_fail;
	}
	
	
	/** Fill the init event structure */
	memset(&plm_init_evt, 0, sizeof(PLMS_EVT));
	plm_init_evt.req_res = PLMS_REQ;
	plm_init_evt.req_evt.req_type = PLMS_AGENT_LIB_REQ_EVT_T;
	plm_init_evt.req_evt.agent_lib_req.lib_req_type = PLMS_AGENT_INIT_EVT;
	
	/** Send a sync msg to PLMS to obtain plmHandle for this client */
	proc_rc = plm_mds_msg_sync_send(plma_cb->mds_hdl,NCSMDS_SVC_ID_PLMA,
			 NCSMDS_SVC_ID_PLMS,plma_cb->plms_mdest_id,
			 &plm_init_evt,&plm_init_resp,PLMS_MDS_SYNC_TIME);

	if (NCSCC_RC_SUCCESS != proc_rc){
		LOG_ER("PLMA : plm_mds_msg_sync_send FAILED");
		rc = SA_AIS_ERR_TRY_AGAIN;
		goto mds_fail;
	}
	
	/** Process the response */
	if(!plm_init_resp){
		LOG_ER("PLMA : plm_mds_msg_sync_send ERROR RESPONSE");
		rc = SA_AIS_ERR_TRY_AGAIN;
		goto no_resp;      	
	}else{
		rc = plm_init_resp->res_evt.error;
		if (SA_AIS_OK != rc){
			LOG_ER("PLMA : plm_mds_msg_sync_send ERROR RESPONSE");
			goto resp_err;
		}
		*plmHandle = plm_init_resp->res_evt.hdl;
		TRACE_5(" PLM handle got: %Lu", plm_init_resp->res_evt.hdl);
		/** Fill client info and store it in CB */
		client_info->plm_handle = plm_init_resp->res_evt.hdl;
		client_info->grp_info_list = NULL;
		
		if(plmCallbacks){
			/** Store the callback function pointer */
			client_info->track_callback = *plmCallbacks;
			
			proc_rc = plma_callback_ipc_init(client_info);

			if (proc_rc != NCSCC_RC_SUCCESS) {
				LOG_ER("PLMA : IPC INIT FAILED");
				rc = SA_AIS_ERR_LIBRARY;
				goto ipc_init_fail;
			}
		}
		
		if (m_NCS_LOCK(&plma_cb->cb_lock, NCS_LOCK_WRITE) != 
							NCSCC_RC_SUCCESS) {
			LOG_ER("PLMA : GET CB LOCK FAILED");
			rc = SA_AIS_ERR_NO_RESOURCES;
			goto lock_fail;
		}

		/** Add client info to patricia tree in CB */
		client_info->pat_node.key_info = 
					(uint8_t *)&client_info->plm_handle;
		proc_rc = ncs_patricia_tree_add(&plma_cb->client_info, 
						&client_info->pat_node);
		m_NCS_UNLOCK(&plma_cb->cb_lock, NCS_LOCK_WRITE);

		if (proc_rc != NCSCC_RC_SUCCESS) {
			LOG_ER("PLMA :CLIENT NODE ADD TO CLIENT TREE FAILED");
			goto node_add_fail; 
		}


	}
	goto end;
	node_add_fail:
	lock_fail:
		plma_callback_ipc_destroy(client_info);
	ipc_init_fail:
		/** Finalize the service */
		plm_fin_res = NULL;
		/** Fill the finalize event structure */
		memset(&plm_fin_evt, 0, sizeof(PLMS_EVT));
		plm_fin_evt.req_res = PLMS_REQ;
		plm_fin_evt.req_evt.req_type = PLMS_AGENT_LIB_REQ_EVT_T;
		plm_fin_evt.req_evt.agent_lib_req.lib_req_type = 
					PLMS_AGENT_FINALIZE_EVT;
		plm_fin_evt.req_evt.agent_lib_req.plm_handle = *plmHandle;
		
		/** send the request to the PLMS */
		proc_rc = plm_mds_msg_sync_send(plma_cb->mds_hdl,
					NCSMDS_SVC_ID_PLMA, NCSMDS_SVC_ID_PLMS,
			 		plma_cb->plms_mdest_id,&plm_fin_evt,
			 		&plm_fin_res,PLMS_MDS_SYNC_TIME);
		if (NCSCC_RC_SUCCESS != proc_rc){
			LOG_ER("PLMA : plm_mds_msg_sync_send for finialize FAILED");
			rc = SA_AIS_ERR_TRY_AGAIN;
			goto mds_fail;
		}
		if (!plm_fin_res){
			LOG_ER("PLMA : FINALIZE REQUEST FAILED");
			rc = SA_AIS_ERR_TRY_AGAIN;
			goto no_resp;
		}else{
			plms_free_evt(plm_fin_res);
		}
	resp_err:
	no_resp:
	mds_fail:
		if (rc != SA_AIS_OK)
			if(client_info)
				free(client_info);
	plma_start_fail:
		ncs_plma_shutdown();
		ncs_agents_shutdown();
	end:
		if(plm_init_resp){
			plms_free_evt(plm_init_resp); 
		}

	TRACE_LEAVE();
	return rc;
	

}
			

/***********************************************************************//**
* @brief	This function returns the operating system handle associated
*		with the handle plmHandle.
*
* @param[out]	plmHandle    	-  A pointer to the handle designating this  
*				   particular initialization of the PLM service
*				   that is to be returned by the PLM service.
* @param[out]	selectionObject -  A pointer to the Operating system handle 
*				   that the process can use to detect the 
*				   pending callbacks.
*
* @return	Refer to SAI-AIS specification for various return values.
***************************************************************************/
SaUint32T saPlmSelectionObjectGet(SaPlmHandleT plmHandle,
		    		  SaSelectionObjectT *selectionObject)
{
	PLMA_CB       *plma_cb = plma_ctrlblk;
	NCS_SEL_OBJ   sel_obj;
	PLMA_CLIENT_INFO  *client_info;
	SaUint32T rc = SA_AIS_OK;

	TRACE_ENTER();
	/** See if the service is already finalized */
	if(!plma_cb){
		LOG_ER("PLMA : PLMA INITIALIZE IS NOT DONE");
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto end;
	}

	if (NULL == selectionObject){
		LOG_ER("PLMA : INVALID HANDLE/INPUT VERSION");
		rc = SA_AIS_ERR_INVALID_PARAM;
		goto end;
	}

	if(!plmHandle){
		LOG_ER("PLMA : INVALID HANDLE/INPUT VERSION");
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto end;
	}
	if(!plma_cb->plms_svc_up){
		LOG_ER("PLMA : PLM SERVICE DOWN");
		rc = SA_AIS_ERR_TRY_AGAIN;
		goto end;
	}
		
	/** Acquire lock for CB. Wait till you get the lock */
	if (m_NCS_LOCK(&plma_cb->cb_lock,NCS_LOCK_READ) != NCSCC_RC_SUCCESS){
		LOG_ER("PLMA : GET CB LOCK FAILED");
		rc =  SA_AIS_ERR_NO_RESOURCES;
		goto end;
	}

	/** Find the client_info record from pat tree */
	client_info = (PLMA_CLIENT_INFO *)
			ncs_patricia_tree_get(&plma_cb->client_info,
					      (uint8_t *)&plmHandle);
	/** Release lock for CB */
	m_NCS_UNLOCK(&plma_cb->cb_lock, NCS_LOCK_READ);
	
	if (!client_info){ 
		LOG_ER("PLMA : GET CLIENT NODE FAILED");
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto end;
	}
	/** Obtain selection object from IPC queue */
	sel_obj = m_NCS_IPC_GET_SEL_OBJ(&client_info->callbk_mbx);


	*selectionObject = (SaSelectionObjectT)m_GET_FD_FROM_SEL_OBJ(sel_obj);
	TRACE_5("selection object got : %Lu", *selectionObject);

end:
	TRACE_LEAVE();
	return rc;


}

/***********************************************************************//**
* @brief	In the context of the calling thread, this function invokes
*		pending callbacks for the handle plmHandle in a way that is
*		specified by the dispatchFlags parameter.
*
* @param[in]	plmHandle    -  A pointer to the handle designating this  
*				particular initialization of the PLM service
*				 that is to be returned by the PLM service.
* @param[in]	flags	     -  Flags that specify the callback execution 
*				behavior of the saPlmDispatch() function, which 
*				have thevalues SA_DISPATCH_ONE, SA_DISPATCH_ALL
*				or SA_DISPATCH_BLOCKING.
*
* @return	Refer to SAI-AIS specification for various return values.
***************************************************************************/
SaAisErrorT saPlmDispatch(SaPlmHandleT plmHandle,
                          SaDispatchFlagsT flags)
{
	PLMA_CB           *plma_cb = plma_ctrlblk;
	PLMA_CLIENT_INFO  *client_info;
	SaAisErrorT       rc = SA_AIS_OK;
	
	TRACE_ENTER();
	/** See if the service is already finalized */
	if(!plma_cb){
		LOG_ER("PLMA : PLMA INITIALIZE IS NOT DONE");
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto end;
	}
	
	if (!m_PLM_DISPATCH_FLAG_IS_VALID(flags)){
		LOG_ER("PLMA : INVALID DISPATCH FLAGS");
		rc = SA_AIS_ERR_INVALID_PARAM;
		goto end;
	}

	if(!plmHandle){
		LOG_ER("PLMA : BAD PLM HANDLE");
		rc =  SA_AIS_ERR_BAD_HANDLE;
		goto end;
	}
	/** Acquire lock for CB. Wait till you get the lock */
	if (m_NCS_LOCK(&plma_cb->cb_lock,NCS_LOCK_READ) != NCSCC_RC_SUCCESS){
		LOG_ER("PLMA : GET CB LOCK FAILED");
		rc =  SA_AIS_ERR_NO_RESOURCES;
		goto end;
	}
	
	client_info = (PLMA_CLIENT_INFO *)
			ncs_patricia_tree_get(&plma_cb->client_info,
					      (uint8_t *)&plmHandle);
	m_NCS_UNLOCK(&plma_cb->cb_lock, NCS_LOCK_READ);
	if (!client_info){
		/** Release lock for CB */
		LOG_ER("PLMA : CLIENT NODE GET FAILED");
		return SA_AIS_ERR_BAD_HANDLE;
	}
	if(plm_process_dispatch_cbk(client_info, flags) != NCSCC_RC_SUCCESS){
		LOG_ER("PLMA : DISPATCH FAILED");
		rc = SA_AIS_ERR_INVALID_PARAM;
	}
	
end:
	TRACE_LEAVE();
	return rc;

}

/***********************************************************************//**
* @brief	Cleans up the client info node when the service is finalized.
*
* @param[in]	client_node  -  A pointer to the PLMA_CLIENT_INFO structure 
*				which needs to be cleaned before freeing it. 
*
* @return	Returns nothing.
***************************************************************************/
void clean_client_info_node(PLMA_CLIENT_INFO *client_node)
{
	PLMA_CB *plma_cb = plma_ctrlblk;
	PLMA_ENTITY_GROUP_INFO_LIST* entity_grp_list = client_node->grp_info_list;
	PLMA_ENTITY_GROUP_INFO_LIST *list_next;
	
	TRACE_ENTER();
	
	while(entity_grp_list)
	{
		PLMA_RDNS_TRK_MEM_LIST *start = entity_grp_list->entity_group_info->rdns_trk_mem_list;
		PLMA_RDNS_TRK_MEM_LIST *next;
		while(start){
			next = start->next;
			/** free the entities pointer in the node */
			/* free(start->entities);*/
			/** free the list node */
			free(start);
			start = next;
		}
		list_next = entity_grp_list->next;
		/** delete the grp info from patricia tree. */
		ncs_patricia_tree_del(&plma_cb->entity_group_info, 
				&entity_grp_list->entity_group_info->pat_node);
		/** free the entity grp info structure in that node. */	
		free(entity_grp_list->entity_group_info);
		
		/** free the entity grp info list pointer in the client info */
		free(entity_grp_list);
		entity_grp_list = list_next;
	}
	TRACE_LEAVE();
	return;
}
	
/***********************************************************************//**
* @brief	This function closes the association represented by the 
*		plmHandle parameter between the invoking process and the
*		PLM Service.
*
* @param[in]	plmHandle  -	A pointer to the handle designating this
*			 	particular initialization of the PLM service,
*				that is to be returned by the PLM service.	
*
* @return	Refer to SAI-AIS specification for various return values.
***************************************************************************/
SaAisErrorT saPlmFinalize(SaPlmHandleT plmHandle)
{
	PLMA_CB                   *plma_cb = plma_ctrlblk;
	PLMA_CLIENT_INFO          *client_info;
	PLMS_EVT                  plm_fin_evt;
	PLMS_EVT                  *plm_fin_resp = NULL;
	SaAisErrorT               rc = SA_AIS_OK;
	uint32_t                     proc_rc = NCSCC_RC_SUCCESS;
	
	TRACE_ENTER();
	
	/** See if the service is already finalized */
	if(!plma_cb){
		LOG_ER("PLMA : PLMA INITIALIZE IS NOT DONE");
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto end;
	}
	
	if(!plmHandle){
		LOG_ER("PLMA : BAD PLM HANDLE");
		rc =  SA_AIS_ERR_BAD_HANDLE;
		goto end;
	}
	
	if(!plma_cb->plms_svc_up){
		LOG_ER("PLMA : PLM SERVICE DOWN");
		rc = SA_AIS_ERR_TRY_AGAIN;
		goto end;
	}

	/** Acquire lock for the CB */
        if (m_NCS_LOCK(&plma_cb->cb_lock, NCS_LOCK_READ) != NCSCC_RC_SUCCESS) {
		LOG_ER("PLMA : GET CB LOCK FAILED");
		rc = SA_AIS_ERR_LIBRARY;
		goto end;
	}
	/** Find the client_info record from pat tree */

	client_info = (PLMA_CLIENT_INFO *)ncs_patricia_tree_get(&plma_cb->client_info,(uint8_t *)&plmHandle);

	m_NCS_UNLOCK(&plma_cb->cb_lock, NCS_LOCK_READ);
	
	if (!client_info){
		LOG_ER("PLMA : CLIENT NODE GET FAILED");
		rc =  SA_AIS_ERR_BAD_HANDLE;
		goto end;
	}
	if(!plma_cb->plms_svc_up){
		LOG_ER("PLMA : PLM SERVICE DOWN");
		rc = SA_AIS_ERR_TRY_AGAIN;
		goto end;
	}

	/** Fill the finalize event structure */
	memset(&plm_fin_evt, 0, sizeof(PLMS_EVT));
	plm_fin_evt.req_res = PLMS_REQ;
	plm_fin_evt.req_evt.req_type = PLMS_AGENT_LIB_REQ_EVT_T;
	plm_fin_evt.req_evt.agent_lib_req.lib_req_type = PLMS_AGENT_FINALIZE_EVT;
	plm_fin_evt.req_evt.agent_lib_req.plm_handle = plmHandle;
	
	/** Send a sync msg to PLMS to delete plmHandle for this client */
	proc_rc = plm_mds_msg_sync_send(plma_cb->mds_hdl,NCSMDS_SVC_ID_PLMA,
			 NCSMDS_SVC_ID_PLMS,plma_cb->plms_mdest_id,
			 &plm_fin_evt,&plm_fin_resp,PLMS_MDS_SYNC_TIME);


	/** Check if return value is SUCCESS or not */
	if (NCSCC_RC_SUCCESS != proc_rc){
		LOG_ER("PLMA : plm_mds_msg_sync_send FOR FINALIZE REQUEST FAILED");
		rc = SA_AIS_ERR_TRY_AGAIN;
		goto end;
	}

	if(!plm_fin_resp){
		LOG_ER("PLMA : FINALIZE REQUEST RETURNED WITH ERR RESPONSE");
		rc = SA_AIS_ERR_TRY_AGAIN;
		goto end;      	
	}
	
	/** Traverse through the group info list and free all entitiy group info's */
	clean_client_info_node(client_info);
	plma_callback_ipc_destroy(client_info);	
	/** Acquire lock for the CB */
        if (m_NCS_LOCK(&plma_cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
		LOG_ER("PLMA : GET CB LOCK FAILED");
		rc = SA_AIS_ERR_LIBRARY;
		goto end;
	}
	/** Remove the client info from the PLMA_CB patricia tree */
	ncs_patricia_tree_del(&plma_cb->client_info, &client_info->pat_node);
	free(client_info);
	
	m_NCS_UNLOCK(&plma_cb->cb_lock, NCS_LOCK_WRITE);

	
	ncs_plma_shutdown();
	ncs_agents_shutdown();
end:
	if(plm_fin_resp){
		plms_free_evt(plm_fin_resp);
	}

	TRACE_LEAVE();
	return rc;
}

/***********************************************************************//**
* @brief		This function adds the new entity group info created 
*			to the Entity group info list of client info.
*
* @param[in,out]	listHead  - A pointer to the head of the list of 
*				    entity group info.
*			entityNew - A pointer to the new entity group info 
*				    structure which is to be added to the the 
*				    entity group info list. 
*
* @return		Returns nothing.
***************************************************************************/
void add_entity_grp_to_list(PLMA_ENTITY_GROUP_INFO_LIST** listHead, 
					PLMA_ENTITY_GROUP_INFO* entityNew)
{
	PLMA_ENTITY_GROUP_INFO_LIST *current, *new;
	current = *listHead;
	
	TRACE_ENTER();	
	new = (PLMA_ENTITY_GROUP_INFO_LIST *)
				malloc(sizeof(PLMA_ENTITY_GROUP_INFO_LIST));	
	if(!new){	
		LOG_CR("PLMA: PLMA_ENTITY_GROUP_INFO_LIST memory alloc failed," 
					"error val:%s",strerror(errno));
		return;
	}
	new->entity_group_info = entityNew;
	new->next = NULL;
	
	if (*listHead == NULL){
		/** see if its an empty list */
		*listHead = new;
	}else{
		while(current->next != NULL){
			current = current->next;
		}
		/** add it to the end of the list */
		current->next = new;
	}

	TRACE_LEAVE();

	return;
}


/***********************************************************************//**
* @brief		This function creates an entity group that may be used
*			later by the invoking process to track readiness status
*			changes of entities in the group.
*
* @param[in]		plmHandle   - The handle which was obtained by a
*				      previous invocation of the 
*				      saPlmInitialize().
* @param[in,out]	groupHandle - A pointer to a memory area to hold the 
*				      entity group handle.
*
* @return		Refer to SAI-AIS specification for various return 
*			values.
***************************************************************************/
SaAisErrorT saPlmEntityGroupCreate(SaPlmHandleT plmHandle,
                                   SaPlmEntityGroupHandleT *groupHandle)
{
	PLMA_CB 		 *plma_cb = plma_ctrlblk;
	PLMA_CLIENT_INFO         *client_info;
	PLMA_ENTITY_GROUP_INFO   *group_info;
	PLMS_EVT  		 plm_in_evt;
	PLMS_EVT  		 *plm_out_res = NULL;
	SaAisErrorT              rc = SA_AIS_OK;
	uint32_t		 	 proc_rc = NCSCC_RC_SUCCESS;

	TRACE_ENTER();
	if(!plma_cb){
		LOG_ER("PLMA : PLMA INITIALIZE IS NOT DONE");
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto end;
	}
	if(!plmHandle){
		LOG_ER("PLMA : BAD PLM HANDLE");
		rc =  SA_AIS_ERR_BAD_HANDLE;
		goto end;
	}
	if (!groupHandle){
		LOG_ER("PLMA : BAD GROUP HANDLE");
		rc = SA_AIS_ERR_INVALID_PARAM;
		goto end;
	}
	if(!plma_cb->plms_svc_up){
		LOG_ER("PLMA : PLM SERVICE DOWN");
		rc = SA_AIS_ERR_TRY_AGAIN;
		goto end;
	}
	
	group_info = (PLMA_ENTITY_GROUP_INFO *)
				calloc(1,sizeof(PLMA_ENTITY_GROUP_INFO));
	if (!group_info){
		LOG_CR("PLMA: PLMA_ENTITY_GROUP_INFO memory alloc failed," 
					"error val:%s",strerror(errno));
		rc = SA_AIS_ERR_NO_MEMORY;
		goto end;
	}
	

	/** Acquire lock for the CB */
        if (m_NCS_LOCK(&plma_cb->cb_lock, NCS_LOCK_READ) != NCSCC_RC_SUCCESS) {
		LOG_ER("PLMA: GET CB LOCK FAILED");
		rc = SA_AIS_ERR_NO_RESOURCES;
		goto lock_fail1;
	}
							
	/* Search for client info using plm handle given */
	client_info = (PLMA_CLIENT_INFO *)
				ncs_patricia_tree_get(&plma_cb->client_info,
						      (uint8_t *)&plmHandle);
	/* Release lock for CB */
	m_NCS_UNLOCK(&plma_cb->cb_lock, NCS_LOCK_READ);
	if (!client_info){
		LOG_ER("PLMA : GET CLIENT_INFO NODE FAILED");
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto err_get_node;
	}

	memset(&plm_in_evt, 0, sizeof(PLMS_EVT));
	plm_in_evt.req_res = PLMS_REQ;
	plm_in_evt.req_evt.req_type = PLMS_AGENT_GRP_OP_EVT_T;
	plm_in_evt.req_evt.agent_grp_op.grp_evt_type = 
						PLMS_AGENT_GRP_CREATE_EVT;
	plm_in_evt.req_evt.agent_grp_op.plm_handle = plmHandle;
								
	/* Send a mds sync msg to PLMS to obtain group handle for this */
	proc_rc = plm_mds_msg_sync_send(plma_cb->mds_hdl, NCSMDS_SVC_ID_PLMA,
                         NCSMDS_SVC_ID_PLMS, plma_cb->plms_mdest_id,
                         &plm_in_evt, &plm_out_res, PLMS_MDS_SYNC_TIME);
	if (NCSCC_RC_SUCCESS != proc_rc){
		LOG_ER("PLMA: plm_mds_msg_sync_send FOR GRP CREATE REQUEST FAILED");
		rc = SA_AIS_ERR_TRY_AGAIN;
		goto mds_fail;
	}
	/* Verify if the response if ok and send grp handle to aa
	pplication */
	if (!plm_out_res){
		LOG_ER("PLMA: plm_mds_msg_sync_send RETURNS WITH INVALID RESPONSE"); 
		rc = SA_AIS_ERR_TRY_AGAIN;
		goto resp_err;
	}else{
		/** process the response */
		if (plm_out_res->res_evt.error != SA_AIS_OK){
			LOG_ER("PLMA: GRP CREATE RETURNS ERR RESPONSE"); 
			rc = SA_AIS_ERR_TRY_AGAIN;
			goto resp_err;
		}
		*groupHandle = plm_out_res->res_evt.hdl;
		TRACE_5("Group Handle got : %Lu", plm_out_res->res_evt.hdl);	
		/* fill the entity group info structure */
		group_info->entity_group_handle = plm_out_res->res_evt.hdl;
		group_info->client_info = client_info;
		group_info->rdns_trk_mem_list = NULL;

		/* add the entity group created to  the entity group list */
		add_entity_grp_to_list(&client_info->grp_info_list, 
					group_info);
		
		/* acquire cb lock */
	    	if (m_NCS_LOCK(&plma_cb->cb_lock, NCS_LOCK_WRITE) 
					!= NCSCC_RC_SUCCESS) {
		LOG_ER("PLMA : GET CB LOCK FAILED");
		rc = SA_AIS_ERR_NO_RESOURCES;
		goto lock_fail2;
		}
		
		group_info->pat_node.key_info = (uint8_t *)&group_info->entity_group_handle;
		
		proc_rc = ncs_patricia_tree_add(&plma_cb->entity_group_info,
						&group_info->pat_node);
		
		m_NCS_UNLOCK(&plma_cb->cb_lock, NCS_LOCK_WRITE);
		if (proc_rc != NCSCC_RC_SUCCESS){
			LOG_ER("PLMA: PLMA_ENTITY_GROUP_INFO NODE ADD FAILED"); 
			rc = SA_AIS_ERR_LIBRARY;
			goto node_add_fail; 
		}
		
	}
	goto end;
node_add_fail:
lock_fail2:	
resp_err:
mds_fail:
err_get_node:
lock_fail1:
	/** free grp info structure */
	free(group_info);
end:
	if(plm_out_res){
		plms_free_evt(plm_out_res);
	}
	TRACE_LEAVE();
	return rc;
/*return err code */
}

/***********************************************************************//**
* @brief	This function creates an entity group that may be used
*		later by the invoking process to track readiness status
*		changes of entities in the group.
*
* @param[in]	entityGroupHandle  - The handle for an entity group which 
*				     was obtained by a previous invocation 
*				     of the saPlmEntityGroupCreate() function.
* @param[in]	entityNames	   - Pointer to an array of entity names. 
* @param[in]	entityNamesNumber  - Number of names contained in the array
*				     referred to by entityNames.
* @param[in]	options		   - Indicates how entity names provided in the
*				     array referred to by entityNames must be
*				     interpreted.
*
* @return	Refer to SAI-AIS specification for various return values.
***************************************************************************/
SaAisErrorT saPlmEntityGroupAdd(SaPlmEntityGroupHandleT entityGroupHandle,
                                const SaNameT *entityNames,
                                SaUint32T entityNamesNumber,
                                SaPlmGroupOptionsT options)
{ 
	PLMA_CB                  *plma_cb = plma_ctrlblk;
	PLMS_EVT                 plm_in_evt;
	PLMS_EVT                 *plm_out_res = NULL;
	SaAisErrorT              rc = SA_AIS_OK;
	uint32_t			 proc_rc = NCSCC_RC_SUCCESS;
	PLMA_ENTITY_GROUP_INFO   *group_info;
	uint32_t                    ii,i;

	TRACE_ENTER();
	if(!plma_cb){
		LOG_ER("PLMA : PLMA INITIALIZE IS NOT DONE");
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto end;
	}

	if (!entityGroupHandle){
		LOG_ER("PLMA: BAD ENTITY GRP HANDLE"); 
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto end;
	}

	if (entityNamesNumber == 0){
		LOG_ER("PLMA: INVALID entityNamesNumber PARAMETER"); 
		rc = SA_AIS_ERR_INVALID_PARAM;
		goto end;
	}
	if (entityNames == NULL){
		LOG_ER("PLMA: entityNames PARAMETER IS NOT SET PROPERLY"); 
		rc = SA_AIS_ERR_INVALID_PARAM;
		goto end;
	}
	TRACE_5("Group Add options got : %d ", options);
	if((SA_PLM_GROUP_SINGLE_ENTITY != options) && 
		(SA_PLM_GROUP_SUBTREE != options) &&
		(SA_PLM_GROUP_SUBTREE_HES_ONLY != options) &&
		(SA_PLM_GROUP_SUBTREE_EES_ONLY != options)){
		LOG_ER("PLMA : INVALID GRP ADD OPTIONS");
		rc = SA_AIS_ERR_INVALID_PARAM;
		goto end;
	}
	

	if(!plma_cb->plms_svc_up){
		LOG_ER("PLMA : PLM SERVICE DOWN");
		rc = SA_AIS_ERR_TRY_AGAIN;
		goto end;
	}
	/* Acquire lock for the CB */
	if (m_NCS_LOCK(&plma_cb->cb_lock,NCS_LOCK_READ) != NCSCC_RC_SUCCESS){
		LOG_ER("PLMA: GET CB LOCK FAILED");
		rc = SA_AIS_ERR_NO_RESOURCES;
		goto end;
	}
	
	/* Search for entity group info using the group handle */
	group_info = (PLMA_ENTITY_GROUP_INFO *)
			ncs_patricia_tree_get(&plma_cb->entity_group_info,
					      (uint8_t *)&entityGroupHandle);

	m_NCS_UNLOCK(&plma_cb->cb_lock,NCS_LOCK_READ);
	if (!group_info){
		LOG_ER("PLMA: PLMA_ENTITY_GROUP_INFO NODE GET FAILED"); 
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto end;
	}

	/* Verify if client info is filled in group info*/
	if (!group_info->client_info){
		LOG_ER("PLMA: INVALID CLIENT_INFO"); 
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto end;
	}
		
	for (i = 0; i < entityNamesNumber; i++){
		/** Check the entityName is repeated in the entityNames Array*/
		for(ii = 0; ii < entityNamesNumber; ii++ ){
			
			if(i != ii){
				if((entityNames[i].length == 
					entityNames[ii].length) && 
				(strncmp((SaInt8T *)entityNames[i].value, 
				(SaInt8T *)entityNames[ii].value,
				entityNames[i].length) == 0)){
					LOG_ER("PLMA: ENTITY APPREARS MORE THAN ONCE IN entityNames ARRAY");
					rc = SA_AIS_ERR_EXIST;
					goto end;
				}
			}
		}
	}
	for (i = 0; i < entityNamesNumber; i++)
	{
		TRACE_5("Entity received %s", entityNames[i].value);
		/** validate the length field of the entities. */
		if(entityNames[i].length > SA_MAX_NAME_LENGTH){	
			LOG_ER("PLMA : INVALID LENGTH FIELD SET FOR ENTITY %s", entityNames[i].value);
			rc= SA_AIS_ERR_INVALID_PARAM;
			goto end;
		}
	}
	memset(&plm_in_evt, 0, sizeof(PLMS_EVT));
	plm_in_evt.req_res = PLMS_REQ;
	plm_in_evt.req_evt.req_type = PLMS_AGENT_GRP_OP_EVT_T;
	plm_in_evt.req_evt.agent_grp_op.grp_evt_type = PLMS_AGENT_GRP_ADD_EVT;
	plm_in_evt.req_evt.agent_grp_op.plm_handle = group_info->client_info->plm_handle;
	plm_in_evt.req_evt.agent_grp_op.grp_handle = entityGroupHandle;
	plm_in_evt.req_evt.agent_grp_op.entity_names_number = entityNamesNumber;
	plm_in_evt.req_evt.agent_grp_op.entity_names = (SaNameT *)entityNames;
	
	plm_in_evt.req_evt.agent_grp_op.grp_add_option = options;
	
	/* Send a mds sync msg to PLMS to obtain group handle for this. */
	proc_rc = plm_mds_msg_sync_send(plma_cb->mds_hdl,NCSMDS_SVC_ID_PLMA,
			 NCSMDS_SVC_ID_PLMS,plma_cb->plms_mdest_id,
			 &plm_in_evt, &plm_out_res, PLMS_MDS_SYNC_TIME);

	if (NCSCC_RC_SUCCESS != proc_rc){
		LOG_ER("PLMA: plm_mds_msg_sync_send FOR GRP ADD REQUEST FAILED");
		rc = SA_AIS_ERR_TRY_AGAIN;
		goto end;
	}

	
	/* Verify if the response if ok  */
	if (!plm_out_res){
		LOG_ER("PLMA:INVALID RESPONSE FOR ADD EVT REQUEST");
		rc = SA_AIS_ERR_TRY_AGAIN;
		goto end;
	}else{
		if (plm_out_res->res_evt.error != SA_AIS_OK){
			LOG_ER("PLMA:ERR RESPONSE FOR ADD EVT REQUEST");
			rc = plm_out_res->res_evt.error;
			goto end;
		}
	}

end:
	/* free event structure */
	if(plm_out_res){
		plms_free_evt(plm_out_res);
	}
	TRACE_LEAVE();
	return rc;
}

/***********************************************************************//**
* @brief	This function removes entities from the entity group designated
*		by entityGroupHandle.
*
* @param[in]	entityGroupHandle  - The handle for an entity group which 
*				     was obtained by a previous invocation 
*				     of the saPlmEntityGroupCreate() function.
* @param[in]	entityNames	   - Pointer to an array of entity names. 
* @param[in]	entityNamesNumber  - Number of names contained in the array
*				     referred to by entityNames.
* @param[in]	options		   - Indicates how entity names provided in the
*				     array referred to by entityNames must be
*				     interpreted.
*
* @return	Refer to SAI-AIS specification for various return values.
***************************************************************************/
SaAisErrorT saPlmEntityGroupRemove(SaPlmEntityGroupHandleT entityGroupHandle,
                                   const SaNameT *entityNames,
                                   SaUint32T entityNamesNumber)
{
	PLMA_CB                  *plma_cb = plma_ctrlblk;
	PLMS_EVT                 plm_in_evt;
	PLMS_EVT                 *plm_out_res = NULL;
	SaAisErrorT              rc = SA_AIS_OK;
	uint32_t			 proc_rc = NCSCC_RC_SUCCESS;
	PLMA_ENTITY_GROUP_INFO   *group_info;
	uint32_t                    ii, i;
	
	TRACE_ENTER();
	if(!plma_cb){
		LOG_ER("PLMA : PLMA INITIALIZE IS NOT DONE");
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto end;
	}
	
	if (!entityGroupHandle){
		LOG_ER("PLMA: INVALID ENTITY GROUP HANDLE"); 
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto end;
	}

	if (entityNamesNumber == 0){
		LOG_ER("PLMA: INVALID entityNamesNumber parameter"); 
		rc = SA_AIS_ERR_INVALID_PARAM;
		goto end;
	}
	if (entityNames == NULL){
		LOG_ER("PLMA: entityNames PARAMETER IS NOT SET PROPERLY"); 
		rc = SA_AIS_ERR_INVALID_PARAM;
		goto end;
	}
	if(!plma_cb->plms_svc_up){
		LOG_ER("PLMA : PLM SERVICE DOWN");
		rc = SA_AIS_ERR_TRY_AGAIN;
		goto end;
	}

	/* Acquire lock for the CB */
	if (m_NCS_LOCK(&plma_cb->cb_lock,NCS_LOCK_READ) != NCSCC_RC_SUCCESS){
		LOG_ER("PLMA : GET CB LOCK FAILED");
		rc = SA_AIS_ERR_NO_RESOURCES;
		goto end;
	}
	/* Search for entity group info using the group handle */
	group_info = (PLMA_ENTITY_GROUP_INFO *)
			ncs_patricia_tree_get(&plma_cb->entity_group_info,
					      (uint8_t *)&entityGroupHandle);
	m_NCS_UNLOCK(&plma_cb->cb_lock,NCS_LOCK_READ);
	if (!group_info){
		LOG_ER("PLMA: PLMA_ENTITY_GROUP_INFO NODE GET FAILED"); 
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto end;
	}

	/* Verify if client info is filled in group info*/
	if (!group_info->client_info){
		LOG_ER("PLMA: INVALID CLIENT_INFO"); 
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto end;
	}
	
	for (i = 0; i < entityNamesNumber; i++){
		/** Check the entityName is repeated in the entityNames Array*/
		for(ii = 0; ii < entityNamesNumber; ii++ ){
			
			if(i != ii){
				if((entityNames[i].length == 
					entityNames[ii].length) && 
				(strncmp((SaInt8T *)entityNames[i].value, 
				(SaInt8T *)entityNames[ii].value,
				entityNames[i].length) == 0)){
					LOG_ER("PLMA: ENTITY APPREARS MORE THAN ONCE IN entityNames ARRAY");
					rc = SA_AIS_ERR_EXIST;
					goto end;
				}
			}
		}
	}
	for (i = 0; i < entityNamesNumber; i++)
	{
		TRACE_5("Entity received %s", entityNames[i].value);
		/** memset the extra characters of entityNames[i] with 0*/
		if(entityNames[i].length > SA_MAX_NAME_LENGTH){	
			LOG_ER("PLMA : INVALID LENGTH FIELD SET FOR ENTITY %s", entityNames[i].value);
			rc= SA_AIS_ERR_INVALID_PARAM;
			goto end;
		}
	}
        

	memset(&plm_in_evt, 0, sizeof(PLMS_EVT));
	plm_in_evt.req_res = PLMS_REQ;
	plm_in_evt.req_evt.req_type = PLMS_AGENT_GRP_OP_EVT_T;
	plm_in_evt.req_evt.agent_grp_op.grp_evt_type = 
						PLMS_AGENT_GRP_REMOVE_EVT;
	plm_in_evt.req_evt.agent_grp_op.plm_handle = 
					group_info->client_info->plm_handle;
	plm_in_evt.req_evt.agent_grp_op.grp_handle = entityGroupHandle;
	plm_in_evt.req_evt.agent_grp_op.entity_names_number = entityNamesNumber;
	
	plm_in_evt.req_evt.agent_grp_op.entity_names = (SaNameT *)entityNames;


	/* Send a mds sync msg to PLMS to obtain group handle for this */
	proc_rc = plm_mds_msg_sync_send(plma_cb->mds_hdl,NCSMDS_SVC_ID_PLMA,
			 NCSMDS_SVC_ID_PLMS,plma_cb->plms_mdest_id,
			 &plm_in_evt, &plm_out_res, PLMS_MDS_SYNC_TIME);

	if (NCSCC_RC_SUCCESS != proc_rc){
		LOG_ER("PLMA: plm_mds_msg_sync_send FOR GRP REM REQUEST FAILED");
		rc = SA_AIS_ERR_TRY_AGAIN;
		goto end;
	}
	
	/* Verify if the response if ok and send grp handle to application */
	if (!plm_out_res){
		LOG_ER("PLMA:INVALID RESPONSE FOR REM EVT REQUEST");
		rc = SA_AIS_ERR_TRY_AGAIN;
		goto end;
	}

	if (plm_out_res->res_evt.error != SA_AIS_OK){
		LOG_ER("PLMA: ERR RESPONSE FOR ADD EVT REQUEST");
		rc = plm_out_res->res_evt.error;
		goto end;
	}
	
end:
	if(plm_out_res){
		plms_free_evt(plm_out_res);
	}
	TRACE_LEAVE();
	/* return error */
	return rc;
}

/***********************************************************************//**
* @brief	This function cleans the group_info stucture before it is
*		deleted fron the patricia tree.
*
* @param[in]	group_info	 -   A pointer to the PLMA_ENTITY_GROUP_INFO 
*				     structure which has to be cleaned up. 
*
* @return	Returns nothing.
***************************************************************************/
void clean_group_info_node(PLMA_ENTITY_GROUP_INFO *group_info)
{
	PLMA_RDNS_TRK_MEM_LIST *rdns_trk_list = group_info->rdns_trk_mem_list;
	PLMA_RDNS_TRK_MEM_LIST *next;
	
	TRACE_ENTER();
	/** 
	 * 1. Clean the list of pointers to be freed using 
	 *    saPlmReadinessNotificationFree API.
	 * 2. Remove the grp info structure from grp info list that is in 
	 *    client info structure. 
	 */
	while(rdns_trk_list)
	{
		next = rdns_trk_list->next;
		/* free(rdns_trk_list->entities);*/
		free(rdns_trk_list);
		rdns_trk_list = next;
	}
	PLMA_ENTITY_GROUP_INFO_LIST *grp_info_list = group_info->client_info->grp_info_list;
	PLMA_ENTITY_GROUP_INFO_LIST *prev = NULL;
	while(grp_info_list)
	{
		if(grp_info_list->entity_group_info == group_info){
			if(NULL == prev){
				group_info->client_info->grp_info_list = grp_info_list->next;
				free(grp_info_list);
			}else{
				prev->next = grp_info_list->next;
				free(grp_info_list);
			}
			break;
		}
				prev = grp_info_list;
				grp_info_list = grp_info_list->next;
	}
	TRACE_LEAVE();
	return;
}


/***********************************************************************//**
* @brief	This function deletes the entity group designated by its
*		handle entityGroupHandle.
*
* @param[in]	entityGroupHandle - The handle for an entity group which was
*				    obtained by a previous invocation of the
*				    saPlmEntityGroupCreate() function.
*
* @return	Refer to SAI-AIS specification for various return values.
***************************************************************************/
SaAisErrorT saPlmEntityGroupDelete(SaPlmEntityGroupHandleT entityGroupHandle)
{

	PLMA_CB                  *plma_cb = plma_ctrlblk;
	PLMS_EVT                 plm_in_evt;
	PLMS_EVT                 *plm_out_res = NULL;
	SaAisErrorT              rc = SA_AIS_OK;
	uint32_t			 proc_rc = NCSCC_RC_SUCCESS;
	PLMA_ENTITY_GROUP_INFO   *group_info;

	TRACE_ENTER();
	if(!plma_cb){
		LOG_ER("PLMA : PLMA INITIALIZE IS NOT DONE");
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto end;
	}
	if (!entityGroupHandle){
		LOG_ER("PLMA: INVALID GRP HANDLE"); 
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto end;
	}

	if(!plma_cb->plms_svc_up){
		LOG_ER("PLMA : PLM SERVICE DOWN");
		rc = SA_AIS_ERR_TRY_AGAIN;
		goto end;
	}
	/** Acquire lock for the CB */
	if (m_NCS_LOCK(&plma_cb->cb_lock,NCS_LOCK_READ) != NCSCC_RC_SUCCESS){
		LOG_ER("PLMA: GET CB LOCK FAILED");
		rc = SA_AIS_ERR_NO_RESOURCES;
		goto end;
	}

	/* Search for entity group info using the group handle */
	group_info = (PLMA_ENTITY_GROUP_INFO *)
			ncs_patricia_tree_get(&plma_cb->entity_group_info,
					      (uint8_t *)&entityGroupHandle);
	
	m_NCS_UNLOCK(&plma_cb->cb_lock,NCS_LOCK_READ);
	if (!group_info){
		LOG_ER("PLMA: PLMA_ENTITY_GROUP_INFO NODE GET FAILED"); 
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto end;
	}

	/* Verify if client info is filled in group info*/
	if (!group_info->client_info){
		LOG_ER("PLMA: INVALID CLIENT_INFO"); 
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto end;
	}
	if (group_info->is_trk_enabled){
 		LOG_ER("PLMA: Track stop is not yet called. Can not delete the group.");
 		rc = SA_AIS_ERR_BUSY;
 		goto end;
 	}
	memset(&plm_in_evt, 0, sizeof(PLMS_EVT));
	plm_in_evt.req_res = PLMS_REQ;
	plm_in_evt.req_evt.req_type = PLMS_AGENT_GRP_OP_EVT_T;
	plm_in_evt.req_evt.agent_grp_op.grp_evt_type = PLMS_AGENT_GRP_DEL_EVT;
	plm_in_evt.req_evt.agent_grp_op.grp_handle = entityGroupHandle;


	/* Send a mds sync msg to PLMS to obtain group handle for this */
	proc_rc = plm_mds_msg_sync_send(plma_cb->mds_hdl,NCSMDS_SVC_ID_PLMA,
			 NCSMDS_SVC_ID_PLMS,plma_cb->plms_mdest_id,
			 &plm_in_evt, &plm_out_res, PLMS_MDS_SYNC_TIME);

	if (NCSCC_RC_SUCCESS != proc_rc){
		LOG_ER("PLMA: plm_mds_msg_sync_send FOR GRP DEL REQUEST FAILED"); 
		rc = SA_AIS_ERR_TRY_AGAIN;
		goto end;
	}
	
	/* Verify if the response if ok and send grp handle to application */
	if (!plm_out_res){
		LOG_ER("PLMA:INVALID RESPONSE FOR GRP DEL EVT REQUEST");
		rc = SA_AIS_ERR_TRY_AGAIN;
		goto end;
	}
	if (plm_out_res->res_evt.error != SA_AIS_OK){
		LOG_ER("PLMA: ERR RESPONSE FOR GRP DEL REQUEST");
		rc = plm_out_res->res_evt.error;
		goto end;
	}

	/** Clean the group_info stucture and delete it from patricia tree */
	clean_group_info_node(group_info);
	
	/* Acquire lock for the CB */
	if (m_NCS_LOCK(&plma_cb->cb_lock,NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS){
		LOG_ER("PLMA: GET CB LOCK FAILED");
		rc = SA_AIS_ERR_NO_RESOURCES;
		goto end;
	}
	ncs_patricia_tree_del(&plma_cb->entity_group_info, 
							&group_info->pat_node);
	
	free(group_info);
	m_NCS_UNLOCK(&plma_cb->cb_lock,NCS_LOCK_WRITE);

end:
	if(plm_out_res){
		plms_free_evt(plm_out_res);
	}
	TRACE_LEAVE();
	/* return error */
	return rc;
}

/***********************************************************************//**
* @brief		This function adds the pointer to the entity array, 
*			which has to be freed in the 
*			saPlmReadinessNotificationFree function, to the list.
*
* @param[in,out]	entity_list - A pointer to the head of the list of 
*				      entitiy pointers to which the entity 
*				      pointer has to be added.
*			entity      - Pointer to the array of entities which
*				      has to be freed by using the function
*				      saPlmReadinessNotificationFree.
*
* @return	Returns nothing.
***************************************************************************/
void add_entities_to_track_list(PLMA_RDNS_TRK_MEM_LIST **entity_list,
				SaPlmReadinessTrackedEntityT *entity)
{
	PLMA_RDNS_TRK_MEM_LIST *new,*temp;
	
	TRACE_ENTER();
		temp = *entity_list;
		new = (PLMA_RDNS_TRK_MEM_LIST *)malloc(sizeof(PLMA_RDNS_TRK_MEM_LIST));
		if(!new){
		
			LOG_CR("PLMA : PLMA_RDNS_TRK_MEM_LIST memory alloc failed, error val:%s",strerror(errno));
			return;
		}
		new->entities = entity;
		new->next = NULL;
		if(temp == NULL){
			*entity_list = new;
		}else{
			while(temp->next != NULL)
			{
				temp = temp->next;
			}
			temp->next = new;
		}
		
	TRACE_LEAVE();
	return;
}
	 
/***********************************************************************//**
* @brief		This function can be used to retrieve the current 
*			readiness status of all PLM entities that are contained
*			in the entity group referred to by entityGroupHandle, 
*			to start tracking changes of the readiness status of 
*			these entities, or to perform both actions.
*
* @param[in]		entityGroupHandle  - The handle for an entity group 
*					     which was obtained by a previous 
*					     invocation of the 
*					     saPlmEntityGroupCreate() function.
* @param[in]		trackCookie	   - Value provided by the invoking 
*					     process, and which will be passed
*					     as a parameter to all invocations 
*					     of SaPlmReadinessTrackCallbackT
*					     function triggered by this 
*					     invocation of the 
*					     saPlmReadinessTrack() function.
* @param[in,out]	trackedEntities    - A pointer to a structure of type
*					     SaPlmReadinessTrackedEntitiesT
*
* @return		Refer to SAI-AIS specification for various return 
*			values.
***************************************************************************/
SaAisErrorT saPlmReadinessTrack(SaPlmEntityGroupHandleT entityGroupHandle,
                                SaUint8T trackFlags,
                                SaUint64T trackCookie,
                                SaPlmReadinessTrackedEntitiesT *trackedEntities)
{
	PLMA_CB                  *plma_cb = plma_ctrlblk;
	PLMS_EVT                 plm_in_evt;
	PLMS_EVT                 *plm_out_res = NULL;
	SaAisErrorT              rc = SA_AIS_OK;
	uint32_t			 proc_rc = NCSCC_RC_SUCCESS;
	PLMA_ENTITY_GROUP_INFO   *group_info;
	SaUint32T                entities_num = 0; 
	
	TRACE_ENTER();
	if(!plma_cb){
		LOG_ER("PLMA : PLMA INITIALIZE IS NOT DONE");
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto end;
	}
	
	if (!entityGroupHandle){
		LOG_ER("PLMA: INVALID GRP HANDLE");
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto end;
	}
	/**
	 * Check if both SA_TRACK_CHANGES and SA_TRACK_CHANGES_ONLY are set.
	 * Check if any of the SA_TRACK_CHANGES, SA_TRACK_CHANGES_ONLY, 
	 * SA_TRACK_CURRENT are set. return error if none is set. 
	 */
	if (m_VALIDATE_TRACK_FLAGS(trackFlags)){
		LOG_ER("PLMA: INVALID TRACK FLAGS");
		rc = SA_AIS_ERR_BAD_FLAGS;
		goto end;
	}
	TRACE_5("Flags received are : %x", trackFlags);

	/** 
	 * Return error if entities pointer is not NULL and 
	 * numberOfEntities is 0 and only if current flag is passed.
	 */
	if (m_PLM_IS_SA_TRACK_CURRENT_SET(trackFlags)){ 
		if(trackedEntities){
			if ((trackedEntities->entities != NULL) && 
					(trackedEntities->numberOfEntities == 0)){
				LOG_ER("PLMA: INVALID INPUT PARAM. entities pointer != NULL and numberOfEntities = 0 ");
				rc = SA_AIS_ERR_INVALID_PARAM;
				goto end;	
			}
		}
	}
	/** 
	 * Ignore the start and validate flags if track current alone is set.
	 */ 
	if((!m_PLM_IS_SA_TRACK_CHANGES_SET(trackFlags) && !m_PLM_IS_SA_TRACK_CHANGES_ONLY_SET(trackFlags))){
		/** mask the SA_TRACK_START_STEP flag if it is set */
		if(trackFlags & SA_TRACK_START_STEP){
			trackFlags = trackFlags & (~(SA_TRACK_START_STEP));
		}
		/** mask the SA_TRACK_VALIDATE_STEP flag if it is set */
		if(trackFlags & SA_TRACK_VALIDATE_STEP){
			trackFlags = trackFlags & (~(SA_TRACK_VALIDATE_STEP));
		}
		TRACE_5("Flags after masking the validate and start step : %x", trackFlags);
	}
	
	if(!plma_cb->plms_svc_up){
		LOG_ER("PLMA : PLM SERVICE DOWN");
		rc = SA_AIS_ERR_TRY_AGAIN;
		goto end;
	}

	/** Acquire lock for the CB */
	if (m_NCS_LOCK(&plma_cb->cb_lock,NCS_LOCK_READ) != NCSCC_RC_SUCCESS){
		LOG_ER("PLMA: GET CB LOCK FAILED"); 
		rc = SA_AIS_ERR_NO_RESOURCES;
		goto end;
	}
	
	/** Search for entity group info using the group handle */
	group_info = (PLMA_ENTITY_GROUP_INFO *)
			ncs_patricia_tree_get(&plma_cb->entity_group_info,
					      (uint8_t *)&entityGroupHandle);

	m_NCS_UNLOCK(&plma_cb->cb_lock,NCS_LOCK_READ);
	if(!group_info){
		LOG_ER("PLMA: PLMA_ENTITY_GROUP_INFO NODE GET FAILED"); 
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto end;
	}
	/** Verify if client info is filled in group info*/
	if (!group_info->client_info){
		LOG_ER("PLMA: INVALID CLIENT_INFO"); 
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto end;
	}

	/*
	* Check if the call back is registered for flags requiring 
	* callbk processing
	*/
	if ((m_PLM_IS_SA_TRACK_CHANGES_SET(trackFlags) || 
		m_PLM_IS_SA_TRACK_CHANGES_ONLY_SET(trackFlags))){  
			if (!group_info->client_info->track_callback.saPlmReadinessTrackCallback){
			LOG_ER("PLMA: CALLBACK IS NOT REGISTERED PROPERLY"); 
			rc = SA_AIS_ERR_INIT;
			goto end;
		}
	}

	if (m_PLM_IS_SA_TRACK_CURRENT_SET(trackFlags) && (!trackedEntities)){
		if (!group_info->client_info->track_callback.saPlmReadinessTrackCallback){
			LOG_ER("PLMA: CALLBACK IS NOT REGISTERED PROPERLY");
			rc = SA_AIS_ERR_INIT;
			goto end;
		}

	}
	

	memset(&plm_in_evt, 0, sizeof(PLMS_EVT));

	/** Compose the Event to be sent to PLMS */
	plm_in_evt.req_res = PLMS_REQ;
	plm_in_evt.req_evt.req_type = PLMS_AGENT_TRACK_EVT_T;
	plm_in_evt.req_evt.agent_track.agent_handle = group_info->client_info->plm_handle;
	plm_in_evt.req_evt.agent_track.grp_handle = group_info->entity_group_handle;
	plm_in_evt.req_evt.agent_track.evt_type = PLMS_AGENT_TRACK_START_EVT;
	plm_in_evt.req_evt.agent_track.track_start.track_flags = trackFlags;
	plm_in_evt.req_evt.agent_track.track_start.track_cookie = trackCookie;
	
	if (m_PLM_IS_SA_TRACK_CURRENT_SET(trackFlags) && (trackedEntities)){
			plm_in_evt.req_evt.agent_track.track_start.no_of_entities = trackedEntities->numberOfEntities; 
		
		/** Send a mds sync msg to PLMS to start tracking */
		proc_rc = plm_mds_msg_sync_send(plma_cb->mds_hdl,
				NCSMDS_SVC_ID_PLMA,NCSMDS_SVC_ID_PLMS,
				plma_cb->plms_mdest_id,
				&plm_in_evt, &plm_out_res, PLMS_MDS_SYNC_TIME);

		if (NCSCC_RC_SUCCESS != proc_rc){
			LOG_ER("PLMA: plm_mds_msg_sync_send FOR READINESS TRACK REQUEST FAILED");
			rc = SA_AIS_ERR_TRY_AGAIN;
			goto end;
		}
		if ((m_PLM_IS_SA_TRACK_CHANGES_SET(trackFlags) || 
			m_PLM_IS_SA_TRACK_CHANGES_ONLY_SET(trackFlags))){ 
			group_info->trk_strt_stop = 1;
			group_info->is_trk_enabled = 1;
		}
		/** Verify if the response if ok */
		if (!plm_out_res){
			LOG_ER("PLMA:INVALID RESPONSE FOR READINESS TRACK REQUEST");
			rc = SA_AIS_ERR_TRY_AGAIN;
			goto end;
		}
		if (plm_out_res->res_evt.error != SA_AIS_OK){
			if (SA_AIS_ERR_NO_SPACE == plm_out_res->res_evt.error)
				trackedEntities->numberOfEntities = plm_out_res->res_evt.entities->numberOfEntities;

			LOG_ER("PLMA: ERR RESPONSE FOR READINESS TRACK REQUEST");
			rc = plm_out_res->res_evt.error;
			goto end;
		}
	}else{ /** for rest of the flags, Get the response in callback */
		proc_rc = plms_mds_normal_send(plma_cb->mds_hdl, 
					NCSMDS_SVC_ID_PLMA, 
					&plm_in_evt, plma_cb->plms_mdest_id,
					NCSMDS_SVC_ID_PLMS);
		if (NCSCC_RC_SUCCESS != proc_rc){
			LOG_ER("PLMA: plms_mds_normal_send FOR READINESS TRACK REQUEST FAILED");
			rc = SA_AIS_ERR_TRY_AGAIN;
			goto end;
		}
		group_info->trk_strt_stop = 1;	
		/* Dont set is_trk_enable flag if the flag is only current.*/
		if ((m_PLM_IS_SA_TRACK_CHANGES_SET(trackFlags) || 
			m_PLM_IS_SA_TRACK_CHANGES_ONLY_SET(trackFlags))){
			group_info->is_trk_enabled = 1;
		}
		goto end;
		
	}
	entities_num = plm_out_res->res_evt.entities->numberOfEntities;
	if(trackedEntities){
		trackedEntities->numberOfEntities = entities_num;
		TRACE_5(" Number of entites received: %d ", entities_num);
		/** 
		* Add the entities to the list which the application frees 
		* later using saPlmReadinessNotificationFree API. 
		*/
		if((!trackedEntities->entities) && (entities_num)){
			trackedEntities->entities = malloc(entities_num * sizeof(SaPlmReadinessTrackedEntityT));
			add_entities_to_track_list(&group_info->rdns_trk_mem_list, trackedEntities->entities);
		}
		
		if (plm_out_res->res_evt.entities){
			memcpy(trackedEntities->entities,
			plm_out_res->res_evt.entities->entities, 
			entities_num * sizeof(SaPlmReadinessTrackedEntityT));
		}
	}
end:
	if(plm_out_res){
		plms_free_evt(plm_out_res);
	}
	TRACE_LEAVE();
	return rc;
}

/***********************************************************************//**
* @brief	This function is used by a process to provide a response to an
*		SaPlmReadinessTrackCallbackT callback previously invoked with 
*		a step parameter equal to either SA_PLM_CHANGE_VALIDATE or
*		SA_PLM_CHANGE_START.
*
* @param[in]	entityGrpHdl      - The handle for an entity group which was 
*				    obtained by a previous invocation of the
*				    saPlmEntityGroupCreate() function.
* @param[in]	invocation        - This parameter was provided to the process 
*				    by the PLM Service in the 
*				    SaPlmReadinessTrackCallbackT callback.
* @param[in]	response          - This parameter provides the response 
*				    expected by the PLM Service to a previous
*				    invocation of the 
*				    SaPlmReadinessTrackCallbackT track callback.
*
* @return	Refer to SAI-AIS specification for various return values.
****************************************************************************/
SaAisErrorT saPlmReadinessTrackResponse(SaPlmEntityGroupHandleT entityGrpHdl,
                                        SaInvocationT invocation,
                                        SaPlmReadinessTrackResponseT response)
{
	PLMA_CB                  *plma_cb = plma_ctrlblk;
	PLMS_EVT                 plm_in_evt;
	SaAisErrorT              rc = SA_AIS_OK;
	uint32_t			 proc_rc = NCSCC_RC_SUCCESS;
	PLMA_ENTITY_GROUP_INFO   *group_info;

	TRACE_ENTER();
	if(!plma_cb){
		LOG_ER("PLMA : PLMA INITIALIZE IS NOT DONE");
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto end;
	}
	if (!entityGrpHdl){
		LOG_ER("INVALID GROUP HANDLE");
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto end;
	}
	if(!invocation){
		LOG_ER("INVALID INVOCATION ID");
		rc = SA_AIS_ERR_INVALID_PARAM;
		goto end;
	}
	if(!plma_cb->plms_svc_up){
		LOG_ER("PLMA : PLM SERVICE DOWN");
		rc = SA_AIS_ERR_TRY_AGAIN;
		goto end;
	}



	/* Acquire lock for the CB */
	if (m_NCS_LOCK(&plma_cb->cb_lock,NCS_LOCK_READ) != NCSCC_RC_SUCCESS){
		LOG_ER("PLMA: GET CB LOCK FAILED");
		rc = SA_AIS_ERR_NO_RESOURCES;
		goto end;
	}
	/* Search for entity group info using the group handle */
	group_info = (PLMA_ENTITY_GROUP_INFO *)
			ncs_patricia_tree_get(&plma_cb->entity_group_info,
					      (uint8_t *)&entityGrpHdl);
	m_NCS_UNLOCK(&plma_cb->cb_lock,NCS_LOCK_READ);
	
	if (!group_info || !(group_info->client_info)){
		LOG_ER("PLMA: PLMA_ENTITY_GROUP_INFO NODE GET FAILED"); 
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto end;
	}


	
	memset(&plm_in_evt, 0, sizeof(PLMS_EVT));

	/* Compose the Event to be sent to PLMS */
	plm_in_evt.req_res = PLMS_REQ;
	plm_in_evt.req_evt.req_type = PLMS_AGENT_TRACK_EVT_T;
	plm_in_evt.req_evt.agent_track.evt_type = PLMS_AGENT_TRACK_RESPONSE_EVT;
	plm_in_evt.req_evt.agent_track.grp_handle = entityGrpHdl;
	plm_in_evt.req_evt.agent_track.track_cbk_res.invocation_id = invocation;
	plm_in_evt.req_evt.agent_track.track_cbk_res.response = response;
        
	/* Send a mds async msg to PLMS to obtain group handle for this */
	proc_rc = plms_mds_normal_send(plma_cb->mds_hdl, 
					NCSMDS_SVC_ID_PLMA, 
					&plm_in_evt, plma_cb->plms_mdest_id,
					NCSMDS_SVC_ID_PLMS);

	if (NCSCC_RC_SUCCESS != proc_rc){
		LOG_ER("PLMA: plms_mds_normal_send FOR READINESS TRACK RESPONSE REQUEST FAILED"); 
		rc = SA_AIS_ERR_TRY_AGAIN;
		goto end;
	}
end:
	TRACE_LEAVE();
	return rc;
}

/***********************************************************************//**
* @brief	This function stops any further tracking notifications
*		of readiness status changes of the group of entities designated
*		by entityGroupHandle and which were requested by specifying the 
*		handle entityGroupHandle when invoking the saPlmReadinessTrack() 
*		function, and which are still in effect. 
*		Pending callbacks are removed.
*
* @param[in]	entityGroupHandle  - The handle for an entity group which was 
*				     obtained by a previous invocation of the
*				     saPlmEntityGroupCreate() function.
*
* @return	Refer to SAI-AIS specification for various return values.
****************************************************************************/
SaAisErrorT saPlmReadinessTrackStop(SaPlmEntityGroupHandleT entityGroupHandle)
{

	PLMA_CB                  *plma_cb = plma_ctrlblk;
	PLMS_EVT                 plm_in_evt;
	PLMS_EVT                 *plm_out_res = NULL;
	SaAisErrorT              rc = SA_AIS_OK;
	uint32_t			 proc_rc = NCSCC_RC_SUCCESS;
	PLMA_ENTITY_GROUP_INFO   *group_info;
	
	TRACE_ENTER();
	if(!plma_cb){
		LOG_ER("PLMA : PLMA INITIALIZE IS NOT DONE");
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto end;
	}
	
	if (!entityGroupHandle){
		LOG_ER("PLMA : INVALID GROUP HANDLE");
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto end;
	}
	if(!plma_cb->plms_svc_up){
		LOG_ER("PLMA : PLM SERVICE DOWN");
		rc = SA_AIS_ERR_TRY_AGAIN;
		goto end;
	}


	/* Acquire lock for the CB */
	if (m_NCS_LOCK(&plma_cb->cb_lock,NCS_LOCK_READ) != NCSCC_RC_SUCCESS){
		LOG_ER("PLMA : GET CB LOCK FAILED");
		rc = SA_AIS_ERR_NO_RESOURCES;
		goto end;
	}
	/* Search for entity group info using the group handle */
	group_info = (PLMA_ENTITY_GROUP_INFO *)
			ncs_patricia_tree_get(&plma_cb->entity_group_info,
					      (uint8_t *)&entityGroupHandle);
	m_NCS_UNLOCK(&plma_cb->cb_lock,NCS_LOCK_READ);

	if (!group_info){
		LOG_ER("PLMA: PLMA_ENTITY_GROUP_INFO NODE GET FAILED"); 
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto end;
	}

	/* Verify if client info is filled in group info*/
	if (!group_info->client_info){
		LOG_ER("PLMA: INVALID CLIENT_INFO"); 
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto end;
	}
	
	if (!group_info->is_trk_enabled){
		LOG_ER("PLMA: TRACK NOT STARTED.");
		rc = SA_AIS_ERR_NOT_EXIST;
		goto end;
	}
	
	memset(&plm_in_evt, 0, sizeof(PLMS_EVT));

	/* Compose the Event to be sent to PLMS */
	plm_in_evt.req_res = PLMS_REQ;
	plm_in_evt.req_evt.req_type = PLMS_AGENT_TRACK_EVT_T;
	plm_in_evt.req_evt.agent_track.evt_type = PLMS_AGENT_TRACK_STOP_EVT;
	plm_in_evt.req_evt.agent_track.grp_handle = entityGroupHandle;

	/* Send a mds sync msg to PLMS to obtain group handle for this */
	proc_rc = plm_mds_msg_sync_send(plma_cb->mds_hdl,NCSMDS_SVC_ID_PLMA,
			 NCSMDS_SVC_ID_PLMS,plma_cb->plms_mdest_id,
			 &plm_in_evt, &plm_out_res, PLMS_MDS_SYNC_TIME);

	if (NCSCC_RC_SUCCESS != proc_rc){
		LOG_ER("PLMA: plm_mds_msg_sync_send FOR READINESS TRACK STOP REQUEST FAILED");
		rc = SA_AIS_ERR_TRY_AGAIN;
		goto end;
	}
	
	/* Verify if the response if ok and send grp handle to application */
	if (!plm_out_res){
		LOG_ER("PLMA:INVALID RESPONSE FOR READINESS TRACK STOP REQUEST"); 
		rc = SA_AIS_ERR_TRY_AGAIN;
		goto end;
	}
	if (plm_out_res->res_evt.error != SA_AIS_OK){
		LOG_ER("PLMA: ERR RESPONSE FOR READINESS TRACK STOP REQUEST");
		rc = plm_out_res->res_evt.error;
		goto end;
	}
	/** clear the flag so, all the pending call backs can be removed */ 
	group_info->trk_strt_stop = 0;
	group_info->is_trk_enabled = 0;
end:
	if(plm_out_res){
		plms_free_evt(plm_out_res);
	}
	TRACE_LEAVE();
	return rc;
}

/***********************************************************************//**
* @brief	This function validates memory allocated for entitis by PLM 
*		in a previous call to the saPlmReadinessTrack() function.
*
* @param[in]	entity_list	-  A pointer to the head of list of entity 
*				   pointers to be freed by using 
*				   saPlmReadinessNotificationFree() function.
* @param[in]	entity_ptr	-  Pointer to the array of entities to be freed.
*
* @return	NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
****************************************************************************/
uint32_t validate_entities_ptr(PLMA_RDNS_TRK_MEM_LIST **entity_list, 
			    SaPlmReadinessTrackedEntityT *entity_ptr)
{
	PLMA_RDNS_TRK_MEM_LIST *temp = *entity_list, *prev = NULL;
	
	TRACE_ENTER();
	
	if(temp == NULL){
		return NCSCC_RC_FAILURE;
	}
	
	while(temp)
	{
		
		if(entity_ptr == temp->entities){
			if(prev == NULL){
				
				*entity_list = temp->next;
				free(temp);
			}else{
				prev->next = temp->next;
				free(temp);
			}
			return NCSCC_RC_SUCCESS;
		}
		prev = temp;
		temp = temp->next;
	}
	
	TRACE_LEAVE();
	return NCSCC_RC_FAILURE;
}

/***********************************************************************//**
* @brief	This function frees the memory to which entities points and 
*		which was allocated by the PLM Service library in a previous
*		call to the saPlmReadinessTrack() function.
*
* @param[in]	entityGroupHandle   - The handle for an entity group which was
*				      obtained by a previous invocation of the
*				      saPlmEntityGroupCreate() function.
* @param[in]	entities	    - A pointer to the memory array that was 
*				      allocated by the PLM Service library in 
*				      the saPlmReadinessTrack() function and 
*				      is to be deallocated.
*
* @return	Refer to SAI-AIS specification for various return values.
***************************************************************************/
SaAisErrorT saPlmReadinessNotificationFree(
				SaPlmEntityGroupHandleT entityGroupHandle,
                                SaPlmReadinessTrackedEntityT *entities)
{
	PLMA_CB                  *plma_cb = plma_ctrlblk;
	SaAisErrorT              rc = SA_AIS_OK;
	uint32_t			 proc_rc = NCSCC_RC_SUCCESS;
	PLMA_ENTITY_GROUP_INFO   *group_info;

	TRACE_ENTER();
	if(!plma_cb){
		LOG_ER("PLMA : PLMA INITIALIZE IS NOT DONE");
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto end;
	}
	if (!entityGroupHandle){
		LOG_ER("PLA : INVALID GRP HANDLE");
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto end;
	}


	/* Acquire lock for the CB */
	if (m_NCS_LOCK(&plma_cb->cb_lock,NCS_LOCK_READ) != NCSCC_RC_SUCCESS){
		LOG_ER("PLMA : GET CB LOCK FAILED");
		rc = SA_AIS_ERR_NO_RESOURCES;
		goto end;
	}

	/* Search for entity group info using the group handle */
	group_info = (PLMA_ENTITY_GROUP_INFO *)
			ncs_patricia_tree_get(&plma_cb->entity_group_info,
					      (uint8_t *)&entityGroupHandle);

	m_NCS_UNLOCK(&plma_cb->cb_lock,NCS_LOCK_READ);
	if ((!group_info) || (!group_info->client_info)){
		LOG_ER("PLMA: PLMA_ENTITY_GROUP_INFO NODE GET FAILED"); 
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto end;
	}
	if(!entities){
		LOG_ER("PLMA: INVALID ENTITIES INPUT PARAM"); 
		rc = SA_AIS_ERR_INVALID_PARAM;
		goto end;
	}
	proc_rc = validate_entities_ptr(&group_info->rdns_trk_mem_list,	
					entities);
	if(NCSCC_RC_SUCCESS != proc_rc){
		LOG_ER("PLMA: INVALID ENTITIES INPUT PARAM"); 
		rc = SA_AIS_ERR_INVALID_PARAM;
		goto end;
	}
	free(entities);
end:
	TRACE_LEAVE();
	return rc;
}

/***********************************************************************//**
* @brief		This function is used by processes to report that the 
*			state of health of an entity has changed.
*
* @param[in]		plmHandle	- The handle which was obtained by a 
*					  previous invocation of the 
*					  saPlmInitialize() function.
* @param[in]		impactedEntity 	- Pointer to the name of the entity 
*					  whose readiness status should be 
*					  updated.
* @param[in]		impact		- Impact being reported.
* @param[in,out]	correlationIds	- Pointer to a structure that contains
*					  correlation identifiers.
*
* @return		Refer to SAI-AIS specification for various return 
*			values.
****************************************************************************/
SaAisErrorT saPlmEntityReadinessImpact(SaPlmHandleT plmHandle,
                                       const SaNameT *impactedEntity,
                                       SaPlmReadinessImpactT impact,
                                       SaNtfCorrelationIdsT *correlationIds)
{
	PLMA_CB                  *plma_cb = plma_ctrlblk;
	PLMA_CLIENT_INFO         *client_info;
	PLMS_EVT                 plm_in_evt;
	PLMS_EVT                 *plm_out_res = NULL;
	SaAisErrorT              rc = SA_AIS_OK;
	uint32_t			 proc_rc = NCSCC_RC_SUCCESS;

	TRACE_ENTER();
	if(!plma_cb){
		LOG_ER("PLMA : PLMA INITIALIZE IS NOT DONE");
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto end;
	}
	if (!plmHandle){
		LOG_ER("PLMA : INVALID PLM HANDLE");
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto end;
	}
	if(!impactedEntity){
		LOG_ER("PLMA: impactedEntity IS NOT SET PROPERLY");
		rc = SA_AIS_ERR_INVALID_PARAM;
		goto end;
	}
	if(!correlationIds){
		LOG_ER("PLMA: correlationIds IS NOT SET PROPERLY");
		rc = SA_AIS_ERR_INVALID_PARAM;
		goto end;
	}
	if(!plma_cb->plms_svc_up){
		LOG_ER("PLMA : PLM SERVICE DOWN");
		rc = SA_AIS_ERR_TRY_AGAIN;
		goto end;
	}

	
	/* Acquire lock for CB. Wait till you get the lock */
	if (m_NCS_LOCK(&plma_cb->cb_lock,NCS_LOCK_READ) != NCSCC_RC_SUCCESS){
		LOG_ER("PLMA : GET CB LOCK FAILED");
		rc =  SA_AIS_ERR_NO_RESOURCES;
		goto end;
	}
	/* Find the client_info record from pat tree */
	client_info = (PLMA_CLIENT_INFO *)
			ncs_patricia_tree_get(&plma_cb->client_info,
					      (uint8_t *)&plmHandle);
	/* Release lock for CB */
	m_NCS_UNLOCK(&plma_cb->cb_lock, NCS_LOCK_READ);
	
	if (!client_info){ 
		LOG_ER("PLMA : GET CLIENT NODE FAILED");
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto end;
	}

	memset(&plm_in_evt, 0, sizeof(PLMS_EVT));

	/* Compose the Event to be sent to PLMS */
	plm_in_evt.req_res = PLMS_REQ;
	plm_in_evt.req_evt.req_type = PLMS_AGENT_TRACK_EVT_T;
	plm_in_evt.req_evt.agent_track.evt_type = 
					PLMS_AGENT_TRACK_READINESS_IMPACT_EVT;
	plm_in_evt.req_evt.agent_track.readiness_impact.impact = impact;
	
	plm_in_evt.req_evt.agent_track.readiness_impact.correlation_ids = 
					correlationIds;
	
	/** memset the extra characters of impactedEntity with 0*/ 
	if(impactedEntity->length > SA_MAX_NAME_LENGTH){	
			LOG_ER("Invalid length set for the impactedEntity %s", impactedEntity->value);
			rc= SA_AIS_ERR_INVALID_PARAM;
			goto end;
		}
/*	memset((impactedEntity->value)+(impactedEntity->length), 0,
                        SA_MAX_NAME_LENGTH-(impactedEntity->length));*/
	plm_in_evt.req_evt.agent_track.readiness_impact.impacted_entity = (SaNameT *)impactedEntity;

	/* Send a mds sync msg to PLMS to obtain group handle for this */
	proc_rc = plm_mds_msg_sync_send(plma_cb->mds_hdl,NCSMDS_SVC_ID_PLMA,
			 NCSMDS_SVC_ID_PLMS,plma_cb->plms_mdest_id,
			 &plm_in_evt, &plm_out_res, PLMS_MDS_SYNC_TIME);
	
	if (NCSCC_RC_SUCCESS != proc_rc){
		LOG_ER("PLMA: plm_mds_msg_sync_send FOR READINESS IMPACT FAILED"); 
		rc = SA_AIS_ERR_TRY_AGAIN;
		goto end;
	}
	
	/* Verify if the response if ok */
	if (!plm_out_res){
		LOG_ER("PLMA:INVALID RESPONSE FOR READINESS IMPACT");
		rc = SA_AIS_ERR_TRY_AGAIN;
		goto end;
	}
	if (plm_out_res->res_evt.error != SA_AIS_OK){
		LOG_ER("PLMA: ERR RESPONSE FOR READINESS IMPACT");
		rc = plm_out_res->res_evt.error;
		goto end;
	}
	correlationIds->notificationId = plm_out_res->res_evt.ntf_id;
	
	
end:
	if(plm_out_res){
		free(plm_out_res);
	}
	TRACE_LEAVE();
	return rc;
}


