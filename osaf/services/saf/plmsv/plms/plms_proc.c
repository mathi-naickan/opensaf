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

#include <limits.h>

#include <limits.h>

#include "plms.h"
#include "plms_mbcsv.h"
#include "plms_utils.h"

/* This file contains the routines of PLMS processing PLMA requests */

/* The following structure is used for allocating PLM handles */

#define MAX_PLM_POOL_HANDLES LONG_MAX 
#define MAX_ENTITY_GROUP_HANDLES LONG_MAX




SaUint64T  plm_handle_pool;
SaUint64T  entity_grp_hdl_pool;



PLMS_GROUP_ENTITY *plm_get_aff_ent_list(PLMS_GROUP_ENTITY_ROOT_LIST *entity_list);

/***********************************************************************//**
* @brief	Routine for freeing list of entities in the entity group info 
*		structure.
*
* @param[in]	grp_info - entity group whose entities have to be freeed.
*
* @return	NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE. 
***************************************************************************/
SaUint32T plm_free_entity_root_list_frm_grp(PLMS_ENTITY_GROUP_INFO *grp_info)
{
	PLMS_ENTITY_GROUP_INFO_LIST *start, *prev = NULL;
	PLMS_GROUP_ENTITY_ROOT_LIST *ent_next;

	PLMS_GROUP_ENTITY_ROOT_LIST *grp_entity_list = 
						grp_info->plm_entity_list;
	TRACE_ENTER();
	/** traverse the entity list in the entity group info */
	while(grp_entity_list)
	{		
		/** 
		 * remove this group from part of entity grp list in 
		 * each entity 
		 */
		start = grp_entity_list->plm_entity->part_of_entity_groups; 
		prev = NULL;
		while(start)
		{
			if(grp_info == start->ent_grp_inf){
				if(NULL == prev){
					grp_entity_list->plm_entity->part_of_entity_groups = start->next;
				}else{
					prev->next = start->next;
				}
				free(start);
				start = NULL;
				break;
			}else{
				prev = start;
				start = start->next;
			}
		}
		ent_next = grp_entity_list->next;
		free(grp_entity_list);
		grp_entity_list = ent_next;
	}
	grp_info->plm_entity_list = NULL;
	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

/***********************************************************************//**
* @brief	Routine for freeing invocation list in the entity group info 
*		structure.
*
* @param[in]	invocation_list - invocation list of group which has to be 
*				  freed.
*
* @return	Returns nothing. 
***************************************************************************/
void plm_free_invocation_list_frm_grp(PLMS_ENTITY_GROUP_INFO *grp_info) 
{
	PLMS_INVOCATION_TO_TRACK_INFO *invocation_list = grp_info->invocation_list;
	SaPlmEntityGroupHandleT grp_hdl = grp_info->entity_grp_hdl;
	PLMS_INVOCATION_TO_TRACK_INFO *next_inv_node = NULL;
	PLMS_ENTITY_GROUP_INFO_LIST *grp_list,*grp_prev;
	PLMS_ENTITY *ent;
	PLMS_CB *cb = plms_cb;
	TRACE_ENTER();	
	/* Remove the grp from track_info.*/
	ent = (PLMS_ENTITY *)ncs_patricia_tree_getnext(&cb->entity_info,(SaUint8T *)0);
	while (ent){
		if (NULL != ent->trk_info){
			grp_list = ent->trk_info->group_info_list;
			grp_prev = NULL;
			while ( grp_list ){
				if ( grp_hdl == grp_list->ent_grp_inf->entity_grp_hdl){
					if ( NULL == grp_prev ){
						ent->trk_info->group_info_list = grp_list->next; 
						free(grp_list);
						grp_list = NULL;
					}else {
						grp_prev->next = grp_list->next;
						free(grp_list);
						grp_list = NULL;
					}
					break;
				} else {
					grp_prev = grp_list;
					grp_list = grp_list->next;
				}
						
			}
		}

		ent = (PLMS_ENTITY *)ncs_patricia_tree_getnext(&cb->entity_info,(SaUint8T *)&ent->dn_name);
	}
	while(invocation_list)
	{
		/** Decrement the track count */
		invocation_list->track_info->track_count--;
		/** This is the last response expected  */
		if(invocation_list->track_info->track_count == 0){
			/* Got the all the responses. Send next cbk. */
			if (SA_PLM_CHANGE_VALIDATE == 
				invocation_list->track_info->change_step){
				plms_cbk_validate_resp_ok_err_proc(
						invocation_list->track_info);
			 }else if(SA_PLM_CHANGE_START == 
				invocation_list->track_info->change_step){
				plms_cbk_start_resp_ok_err_proc(
						invocation_list->track_info);
			}

		}
		next_inv_node = invocation_list->next;
		free(invocation_list);
		invocation_list = NULL;
		invocation_list = next_inv_node;	
	}
	grp_info->invocation_list = NULL;
	TRACE_LEAVE();
	return;
}

/***********************************************************************//**
* @brief	Routine for cleaning the entity group info and 
*		deleting it from patricia tree.
*
* @param[in]	grp_info - group info strucutre which has to be deleted 
*			   fron patricia tree. 
*
* @return	Returns nothing. 
***************************************************************************/
void plm_clean_grp_info(PLMS_ENTITY_GROUP_INFO *grp_info)
{
	PLMS_CB *cb = plms_cb;
	TRACE_ENTER();
	/** free the list of entities of this group */
	plm_free_entity_root_list_frm_grp(grp_info);
	/** free the invocation list of this group */
	plm_free_invocation_list_frm_grp(grp_info);
	ncs_patricia_tree_del(&cb->entity_group_info, &grp_info->pat_node);
	free(grp_info);
	grp_info = NULL;
	TRACE_LEAVE();
	return;
}
/***********************************************************************//**
* @brief	This function cleans the client info structure, deletes 
*		the client info node from patricia tree and frees it.
*
* @param[in]	client_info  - client info structure whose reference 
*			       has to be deleted from the patricia tree. 
*
* @return	Returns nothing. 
***************************************************************************/
void plm_clean_client_info(PLMS_CLIENT_INFO *client_info)
{
	PLMS_ENTITY_GROUP_INFO_LIST *start = client_info->entity_group_list;
	PLMS_ENTITY_GROUP_INFO_LIST  *next;
	PLMS_CB *cb = plms_cb;

	TRACE_ENTER();
	while (start)
	{
		next = start->next;
		/** clean grp info */	
		plm_clean_grp_info(start->ent_grp_inf);
		/** Free grp info list node from grp info list */
		free(start);
		start = next; 
	}
	if(NCSCC_RC_SUCCESS != 
	(ncs_patricia_tree_del(&cb->client_info, &client_info->pat_node))){
		LOG_ER("PLMS: PATRICIA DEL OF PLMS_CLIENT_INFO FAILED");
			return;
	}
	free(client_info);
	
	TRACE_LEAVE();
	return;
}

/***********************************************************************//**
* @brief	Routine to get the list of affected entities.
*
* @param[in]	entity_list  - entity list of type PLMS_GROUP_ENTITY_ROOT_LIST
*			       from which, affected entity list of type 
*			       PLMS_GROUP_ENTITY has to be formed.
*
* @return	Returns list of entities of type PLMS_GROUP_ENTITY. 
***************************************************************************/
PLMS_GROUP_ENTITY *plm_get_aff_ent_list(PLMS_GROUP_ENTITY_ROOT_LIST *entity_list)
{
	PLMS_GROUP_ENTITY *node, *prev = NULL;
	PLMS_GROUP_ENTITY *list = NULL;
	TRACE_ENTER();
	while(entity_list)
	{
		node = (PLMS_GROUP_ENTITY *)malloc(sizeof(PLMS_GROUP_ENTITY));
		if(!node){
			LOG_CR("PLMS : PLMS_GROUP_ENTITY memory alloc failed, error val:%s",strerror(errno));
			assert(0);
		}
		node->plm_entity = entity_list->plm_entity;
		node->next = NULL;
		if(prev == NULL){
			list = node;
		}else{
			prev->next = node;
		}
		prev = node;
		entity_list = entity_list->next;
	}
	TRACE_LEAVE();
	return list;
	
}

/***********************************************************************//**
* @brief	Routine to get the list of affected entities.
*
* @param[in]	entity_list  - entity list of type PLMS_GROUP_ENTITY_ROOT_LIST
*			       from which, affected entity list of type 
*			       PLMS_GROUP_ENTITY has to be formed.
*
* @return	Returns list of entities of type PLMS_GROUP_ENTITY. 
***************************************************************************/
void free_ent_root_list(PLMS_GROUP_ENTITY_ROOT_LIST *grp_entity_list)
{
	PLMS_GROUP_ENTITY_ROOT_LIST *next;
	TRACE_ENTER();
	while(grp_entity_list)
	{
		next = grp_entity_list->next;
		free(grp_entity_list);
		grp_entity_list = next;
	}
	TRACE_LEAVE();
	return;
}

/***********************************************************************//**
* @brief	Routine to free the list of affected entities.
*
* @param[in]	aff_entity_list  - aff entity list to be freed.
*
* @return	Returns nothing. 
***************************************************************************/
void free_aff_ent_list(PLMS_GROUP_ENTITY *aff_entity_list)
{
	PLMS_GROUP_ENTITY *next;
	TRACE_ENTER();
	while(aff_entity_list)
	{
		next = aff_entity_list->next;
		free(aff_entity_list);
		aff_entity_list = next;
	}
	TRACE_LEAVE();
	return;
}



/***********************************************************************//**
* @brief	
*
* @return	NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.	
***************************************************************************/
SaUint32T plms_proc_quiesced_ack_evt()
{
	PLMS_CB *cb = plms_cb;

	TRACE_ENTER();

	if (cb->is_quisced_set == TRUE) {
		cb->ha_state = SA_AMF_HA_QUIESCED;
		/* Inform MBCSV of HA state change */
		if (plms_mbcsv_chgrole() != NCSCC_RC_SUCCESS)
		LOG_ER("PLMS-MBCSV- CHANGE ROLE FAILED.....\n");

		/* Update control block */
		saAmfResponse(cb->amf_hdl, cb->amf_invocation_id, SA_AIS_OK);
		cb->is_quisced_set = FALSE;

		TRACE_LEAVE();
		return NCSCC_RC_SUCCESS;
	}
	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}


/***********************************************************************//**
* @brief	This function Initializes the agent.	
*
* @param[in]	plm_evt - plm evt structure received.
*
* @return	Returns nothing.	
***************************************************************************/
void  plms_process_agent_initialize(PLMS_EVT *plm_evt) 
{
	PLMS_CB *cb = plms_cb;
	PLMS_EVT resp_evt;
	SaUint32T rc = SA_AIS_OK;
	SaUint32T proc_rc = NCSCC_RC_SUCCESS;
	PLMS_CLIENT_INFO *client_info = NULL;
	PLMS_MBCSV_MSG mbcsv_msg;

	TRACE_ENTER();

	memset(&resp_evt , 0 , sizeof(PLMS_EVT));
	client_info = (PLMS_CLIENT_INFO *)malloc(sizeof(PLMS_CLIENT_INFO));
	if (!client_info)
	{
		LOG_CR("PLMS : PLMS_CLIENT_INFO memory alloc failed, error val:%s",strerror(errno));
		rc = SA_AIS_ERR_NO_MEMORY;
		goto send_resp;
	}
	memset(client_info,0,sizeof(PLMS_CLIENT_INFO));
	client_info->mdest_id = plm_evt->sinfo.dest;
	client_info->pat_node.key_info = (uns8*)&client_info->plm_handle;

	if (plm_handle_pool == MAX_PLM_POOL_HANDLES){
		TRACE_5("plm_handle_pool reached max limit, hence wrapped around");
		plm_handle_pool = 0;
		/** 
		 * Allocate a plm handle for this request.
		 * If allocation crossed max limit, wrap around.
		 */
		do{
			plm_handle_pool++;
			client_info->plm_handle = plm_handle_pool;
			proc_rc = ncs_patricia_tree_add(&cb->client_info,
							&client_info->pat_node);
		}while(proc_rc != NCSCC_RC_SUCCESS);
	}else{
		plm_handle_pool++;
		client_info->plm_handle = plm_handle_pool;
		proc_rc = ncs_patricia_tree_add(&cb->client_info,
					&client_info->pat_node);				if(NCSCC_RC_SUCCESS != proc_rc){
			LOG_ER("PLMS : Patricia tree add failed. For plm handle : %Lu ",plm_handle_pool);
			rc = SA_AIS_ERR_NO_RESOURCES;
			free(client_info);
			goto send_resp;
		}
	}
	if (cb->ha_state == SA_AMF_HA_ACTIVE)
	{
		memset(&mbcsv_msg, 0, sizeof(PLMS_MBCSV_MSG));
		/* Update same to standby */
		mbcsv_msg.header.msg_type = PLMS_A2S_MSG_CLIENT_INFO;
		mbcsv_msg.header.num_records = 1; /* Since async */
		mbcsv_msg.info.client_info.plm_handle = plm_handle_pool;
		mbcsv_msg.info.client_info.mdest_id = plm_evt->sinfo.dest; 
		proc_rc =  plms_mbcsv_send_async_update(&mbcsv_msg, NCS_MBCSV_ACT_ADD);
		if (proc_rc != NCSCC_RC_SUCCESS)
		{
			LOG_ER("PLMS :Async update to stdby failed");
			/* FIXME  What to do ?? */
		}
	}
	resp_evt.res_evt.hdl = plm_handle_pool;	
		

send_resp:	
	/* form a response and send to caller */
	
	resp_evt.req_res = PLMS_RES;
	resp_evt.res_evt.res_type = PLMS_AGENT_INIT_RES;
	resp_evt.res_evt.error = rc;
	
	/** send mds response to plma */
	proc_rc =  plm_send_mds_rsp(cb->mds_hdl,
                    NCSMDS_SVC_ID_PLMS,
                    &plm_evt->sinfo,
                    &resp_evt);
	if(NCSCC_RC_SUCCESS != proc_rc){
		LOG_ER("PLMS: plm_send_mds_rsp FAILED");
		ncs_patricia_tree_del(&cb->client_info, &client_info->pat_node);
		free(client_info);
	}	
	
		

	TRACE_LEAVE();
	return;
}


/***********************************************************************//**
* @brief	This function Finalizes the agent.	
*
* @param[in]	plm_evt - plm evt structure received.
*
* @return	Returns nothing.	
***************************************************************************/
void plms_process_agent_finalize(PLMS_EVT *plm_evt)
{

	PLMS_CLIENT_INFO               *client_info;
	PLMS_CB			      *cb = plms_cb; 
	PLMS_EVT                      plm_resp;
	SaUint32T                      rc = SA_AIS_OK;
	PLMS_MBCSV_MSG mbcsv_msg;
	SaUint32T proc_rc = NCSCC_RC_SUCCESS; 
	TRACE_ENTER();

	client_info = (PLMS_CLIENT_INFO *)ncs_patricia_tree_get(
			&cb->client_info, 
			(uns8 *)&plm_evt->req_evt.agent_lib_req.plm_handle);
	if (!client_info)
	{
		LOG_ER("PLMS : RECEIVED BAD PLM HANDLE");
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto send_resp;
	}
	
	if (cb->ha_state == SA_AMF_HA_ACTIVE)
	{
		memset(&mbcsv_msg, 0, sizeof(PLMS_MBCSV_MSG));
		/* Update same to standby */
		mbcsv_msg.header.msg_type = PLMS_A2S_MSG_CLIENT_INFO;
		mbcsv_msg.header.num_records = 1; /* Since async */
		mbcsv_msg.info.client_info.plm_handle = client_info->plm_handle;
		mbcsv_msg.info.client_info.mdest_id = client_info->mdest_id; 
		proc_rc =  plms_mbcsv_send_async_update(&mbcsv_msg, NCS_MBCSV_ACT_RMV);
		if (proc_rc != NCSCC_RC_SUCCESS)
		{
			LOG_ER("PLMS : Async update to stdby failed");
			/* FIXME  What to do ?? */
		}
	}
	/** clean up entity info list and and entity infogroups  */
	plm_clean_client_info(client_info);

send_resp:
	memset(&plm_resp, 0, sizeof(PLMS_EVT));
	plm_resp.req_res = PLMS_RES;
	plm_resp.res_evt.res_type = PLMS_AGENT_FIN_RES;
	plm_resp.res_evt.error = rc;

	/** send the response */
	plm_send_mds_rsp(cb->mds_hdl,
                    NCSMDS_SVC_ID_PLMS,
                    &plm_evt->sinfo,
                    &plm_resp);

	TRACE_LEAVE();
	return;
}

/***********************************************************************//**
* @brief	This function fills the SaPlmReadinessTrackedEntityT type
*		structure with with the appropriate values.
*
* @param[out]	dest_entity - the structure of type 
*			      SaPlmReadinessTrackedEntityT which has to be 
*			      filled.
* @param[in]	src_entity  - the structure of type PLMS_ENTITY which contains
*			      the information of the entity.
*
* @return	Returns nothing.	
***************************************************************************/
void plms_fill_entity(SaPlmReadinessTrackedEntityT *dest_entity,
		 PLMS_ENTITY *src_entity)
{
	
	TRACE_ENTER();
	dest_entity->change = SA_PLM_GROUP_NO_CHANGE;
	dest_entity->entityName = src_entity->dn_name;
	if(PLMS_HE_ENTITY ==  src_entity->entity_type){
		dest_entity->currentReadinessStatus.readinessState = 
			src_entity->entity.he_entity.saPlmHEReadinessState;
		dest_entity->currentReadinessStatus.readinessFlags = 
			src_entity->entity.he_entity.saPlmHEReadinessFlags;
		dest_entity->expectedReadinessStatus.readinessState = 
			src_entity->entity.he_entity.saPlmHEReadinessState;
		dest_entity->expectedReadinessStatus.readinessFlags = 
			src_entity->entity.he_entity.saPlmHEReadinessFlags;
	}else{
		dest_entity->currentReadinessStatus.readinessState = 
			src_entity->entity.ee_entity.saPlmEEReadinessState;
		dest_entity->currentReadinessStatus.readinessFlags = 
			src_entity->entity.ee_entity.saPlmEEReadinessFlags;
		dest_entity->expectedReadinessStatus.readinessState = 
			src_entity->entity.ee_entity.saPlmEEReadinessState;
		dest_entity->expectedReadinessStatus.readinessFlags = 
			src_entity->entity.ee_entity.saPlmEEReadinessFlags;
		
	}
	dest_entity->plmNotificationId = src_entity->first_plm_ntf_id;

	TRACE_LEAVE();
	return;
}

/***********************************************************************//**
* @brief	This function gets the number of entities in a group.
*
* @param[in]	entity_list - List of entities in the given group.
*
* @return	Returns number of entities in a group.	
***************************************************************************/
SaUint32T plms_get_num_entities_of_grp(PLMS_GROUP_ENTITY_ROOT_LIST *entity_list)
{
	PLMS_GROUP_ENTITY_ROOT_LIST *head = entity_list;
	SaUint32T num = 0;
	TRACE_ENTER();
	while(head)
	{
		num++;
		head = head->next;
	}
	TRACE_LEAVE();
	return num;
}

/***********************************************************************//**
* @brief	This function starts the tracking.
*
* @param[in]	plm_evt - plm evt structure received.
*
* @return	Returns nothing.	
***************************************************************************/
void plms_process_trk_start_evt(PLMS_EVT *plm_evt)
{
	PLMS_CB *cb = plms_cb;
	SaUint8T track_flags;
	PLMS_ENTITY_GROUP_INFO *grp_info;
	PLMS_GROUP_ENTITY_ROOT_LIST *temp;
	PLMS_EVT plm_resp;
	SaUint8T rc = SA_AIS_OK;
	SaUint32T ii = 0;
	SaUint32T proc_rc = NCSCC_RC_SUCCESS; 
	SaPlmReadinessTrackedEntityT *entity_list;
	SaUint64T orig_tc;
	PLMS_TRACK_INFO trk_info;
	SaUint32T no_of_ent_recd;
	SaUint32T no_of_ent_in_grp = 0;
	PLMS_MBCSV_MSG mbcsv_msg;

	TRACE_ENTER();
	
	
	no_of_ent_recd = plm_evt->req_evt.agent_track.track_start.no_of_entities;
	TRACE_5("Number of entities recd : %d", no_of_ent_recd);
	track_flags = plm_evt->req_evt.agent_track.track_start.track_flags;
	TRACE_5("Track Flags recd : %x", track_flags);
	
	/** get the entity group info node from the grp hdl */
	grp_info = (PLMS_ENTITY_GROUP_INFO *)ncs_patricia_tree_get(
	            &cb->entity_group_info, 
		    (uns8 *)&plm_evt->req_evt.agent_track.grp_handle);
		
	if(!grp_info){
		LOG_ER("PLMS : BAD GROUP HDL RECEIVED");
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto send_resp;
	}
	/** memset the response evt. */
	memset(&plm_resp, 0, sizeof(PLMS_EVT));

	orig_tc = grp_info->track_cookie; 
	no_of_ent_in_grp = plms_get_num_entities_of_grp(grp_info->plm_entity_list);
	if((0 == no_of_ent_recd) || (no_of_ent_recd > no_of_ent_in_grp)){
		no_of_ent_recd = no_of_ent_in_grp;
	}
	
	plm_resp.res_evt.entities = (SaPlmReadinessTrackedEntitiesT *)malloc(sizeof(SaPlmReadinessTrackedEntitiesT));

	if(!plm_resp.res_evt.entities){
		rc = SA_AIS_ERR_NO_MEMORY;
		LOG_CR("PLMS :  memory alloc for tracked_entity structure failed, error val:%s",strerror(errno));
		goto send_resp;
	}
	if(no_of_ent_in_grp != no_of_ent_recd){ 
		LOG_ER("PLMS: no of entities sent is != entities in grp");
		plm_resp.res_evt.entities->numberOfEntities = no_of_ent_in_grp;
		rc = SA_AIS_ERR_NO_SPACE;
		goto send_resp;
	}
	

	if(m_PLM_IS_SA_TRACK_CHANGES_SET(track_flags) || 
	m_PLM_IS_SA_TRACK_CHANGES_ONLY_SET(track_flags)){
		/** 
		 * for other combination of flags send 
		 * resp in callback
		 */
		grp_info->track_flags = track_flags & (~SA_TRACK_CURRENT);
		grp_info->track_cookie = plm_evt->req_evt.agent_track.track_start.track_cookie;

	}
	if (cb->ha_state == SA_AMF_HA_ACTIVE)
	{
		/* Update same to standby */
		memset(&mbcsv_msg, 0, sizeof(PLMS_MBCSV_MSG));
		mbcsv_msg.header.msg_type = PLMS_A2S_MSG_ENT_GRP_INFO;
		mbcsv_msg.header.num_records = 1; /* Since async */
		mbcsv_msg.info.ent_grp_info.entity_group_handle = grp_info->entity_grp_hdl; 
		mbcsv_msg.info.ent_grp_info.agent_mdest_id = plm_evt->sinfo.dest;
		mbcsv_msg.info.ent_grp_info.track_flags = grp_info->track_flags;
		mbcsv_msg.info.ent_grp_info.track_cookie = grp_info->track_cookie; 
		
		proc_rc =  plms_mbcsv_send_async_update(&mbcsv_msg, NCS_MBCSV_ACT_UPDATE);
		if (proc_rc != NCSCC_RC_SUCCESS)
		{
			LOG_ER("Async update to stdby failed");
		}
	}

	/**
	 * Get the number of entities in the group if the falg is set to 
	 * SA_TRACK_CURRENT 
	 */
	if((m_PLM_IS_SA_TRACK_CURRENT_SET(track_flags)) &&((plm_evt->sinfo.stype == MDS_SENDTYPE_SNDRSP) || (plm_evt->sinfo.stype == MDS_SENDTYPE_RSP))){ 
		
		if(!no_of_ent_in_grp){
			plm_resp.res_evt.entities->entities = NULL;
			plm_resp.res_evt.entities->numberOfEntities = 0;
			rc = SA_AIS_OK;
			goto send_resp;
		}
		entity_list = (SaPlmReadinessTrackedEntityT *)malloc(
		no_of_ent_in_grp * sizeof(SaPlmReadinessTrackedEntityT));
		if(!entity_list){
			rc = SA_AIS_ERR_NO_MEMORY;
			LOG_CR("PLMS :  memory alloc for entities failed, error val:%s",strerror(errno));
			goto send_resp;
		}
		memset(entity_list, 0, no_of_ent_in_grp * sizeof(SaPlmReadinessTrackedEntityT));
		
		/** pack all the entities in the group */
		ii = 0;
		temp = grp_info->plm_entity_list;
		while(temp)
		{
			plms_fill_entity(&entity_list[ii], temp->plm_entity);
			temp = temp->next;
			ii++;
		}
		
		plm_resp.res_evt.entities->numberOfEntities = no_of_ent_in_grp;
		plm_resp.res_evt.entities->entities = entity_list;
send_resp:
		plm_resp.req_res = PLMS_RES;
		plm_resp.res_evt.res_type = PLMS_AGENT_TRACK_START_RES;
		plm_resp.res_evt.error = rc;							
		/**  Send response */
		plm_send_mds_rsp(cb->mds_hdl,
                	    NCSMDS_SVC_ID_PLMS,
                    	&plm_evt->sinfo,
                    	&plm_resp);
	}else{ /** for async response */ 
		if(m_PLM_IS_SA_TRACK_CURRENT_SET(track_flags)){	
	
			memset(&trk_info, 0, sizeof(PLMS_TRACK_INFO));
			trk_info.root_entity = NULL;
			trk_info.change_step = SA_PLM_CHANGE_COMPLETED;
			trk_info.track_cause = 
					SA_PLM_CAUSE_STATUS_INFO;
			trk_info.root_correlation_id = 
					SA_NTF_IDENTIFIER_UNUSED;
			PLMS_ENTITY_GROUP_INFO_LIST *grp_info_node = 
				(PLMS_ENTITY_GROUP_INFO_LIST *)malloc
				(sizeof(PLMS_ENTITY_GROUP_INFO_LIST));
			if(!grp_info_node){
				LOG_CR("PLMS :  PLMS_ENTITY_GROUP_INFO_LIST memory alloc failed, error val:%s",strerror(errno));
				return;
			}
			grp_info->track_cookie = 
			plm_evt->req_evt.agent_track.track_start.track_cookie;
			grp_info_node->ent_grp_inf = grp_info;
			grp_info_node->next = NULL;
			trk_info.group_info_list = grp_info_node;
			trk_info.aff_ent_list = plm_get_aff_ent_list(grp_info->plm_entity_list); 
			trk_info.grp_op = SA_PLM_GROUP_NO_CHANGE;
			proc_rc = plms_cbk_call(&trk_info, 1);
			free_aff_ent_list(trk_info.aff_ent_list);
		
			/* If the flag is only current then restore the prev cookie.*/
			if (!m_PLM_IS_SA_TRACK_CHANGES_SET(track_flags) && 
			!m_PLM_IS_SA_TRACK_CHANGES_ONLY_SET(track_flags)){	
				grp_info->track_cookie = orig_tc;
			}
		
		} /* Current */
		
	}
		
	TRACE_LEAVE();	
	return;
}

/***********************************************************************//**
* @brief	This function stops the tracking.
*
* @param[in]	plm_evt - plm evt structure received.
*
* @return	Returns number of entities in a group.	
***************************************************************************/
void plms_process_trk_stop_evt(PLMS_EVT *plm_evt)
{
	PLMS_CB *cb = plms_cb;
	PLMS_ENTITY_GROUP_INFO *grp_info;
	PLMS_EVT plm_resp;
	SaUint32T rc = SA_AIS_OK;
	PLMS_INVOCATION_TO_TRACK_INFO *invocation_list, *next_inv_node;
	PLMS_MBCSV_MSG mbcsv_msg;

	TRACE_ENTER();
	
	/** get the entity group info node from the grp hdl */
	grp_info = (PLMS_ENTITY_GROUP_INFO *)ncs_patricia_tree_get(
	            &cb->entity_group_info, 
		    (uns8 *)&plm_evt->req_evt.agent_track.grp_handle);
	if(!grp_info){
		LOG_ER("PLMS : BAD GROUP HDL RECEIVED");
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto send_resp;
	}
	if(!grp_info->track_flags){
		LOG_ER("PLMS : TRACKING IS NOT STARTED YET");
		rc = SA_AIS_ERR_NOT_EXIST;
	}
	invocation_list = grp_info->invocation_list;
	while(invocation_list)
	{
		/** Decrement the track count */
		invocation_list->track_info->track_count--;
		/** This is the last response expected  */
		if(invocation_list->track_info->track_count == 0){
			/* Got the all the responses. Send next cbk. */
			if (SA_PLM_CHANGE_VALIDATE == 
				invocation_list->track_info->change_step){
				plms_cbk_validate_resp_ok_err_proc(
						invocation_list->track_info);
			 }else if(SA_PLM_CHANGE_START == 
				invocation_list->track_info->change_step){
				plms_cbk_start_resp_ok_err_proc(
						invocation_list->track_info);
			}

		}
		next_inv_node = invocation_list->next;
		free(invocation_list);
		invocation_list = next_inv_node;	
	}
	grp_info->invocation_list = NULL;
	
	/** clear the track flags and cookie */
	grp_info->track_flags = 0; 
	grp_info->track_cookie = 0;
	if (cb->ha_state == SA_AMF_HA_ACTIVE)
	{
		/* Checkpointing with track_flags & cookie as NULL, so that on Standy it will be set to 0 */
		memset(&mbcsv_msg, 0, sizeof(PLMS_MBCSV_MSG));
		mbcsv_msg.header.msg_type = PLMS_A2S_MSG_ENT_GRP_INFO;
		mbcsv_msg.header.num_records = 1; /* Since async */
		mbcsv_msg.info.ent_grp_info.entity_group_handle = grp_info->entity_grp_hdl; 
		mbcsv_msg.info.ent_grp_info.agent_mdest_id = plm_evt->sinfo.dest;
		
		rc =  plms_mbcsv_send_async_update(&mbcsv_msg, NCS_MBCSV_ACT_UPDATE);
		if (rc != NCSCC_RC_SUCCESS)
		{
			LOG_ER("Async update to stdby failed");
		}
	}
send_resp:	
	
	memset(&plm_resp, 0, sizeof(PLMS_EVT));
	plm_resp.req_res = PLMS_RES;
	plm_resp.res_evt.res_type = PLMS_AGENT_TRACK_STOP_RES;
	plm_resp.res_evt.error = rc;

	/** Send resp */
	plm_send_mds_rsp(cb->mds_hdl,
                    NCSMDS_SVC_ID_PLMS,
                    &plm_evt->sinfo,
                    &plm_resp);
	TRACE_LEAVE();
	return;	

	
}

/***********************************************************************//**
* @brief	This function adds the new entity group patricia tree.
*
* @param[in]	grp_info    - grp info structure to be added to patricia tree.
* @param[in]	client_info - info about the client who requested for 
*			      grp create.
*
* @return	Returns NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.	
***************************************************************************/
SaUint32T plms_ent_grp_add_to_db(PLMS_ENTITY_GROUP_INFO *grp_info, 
			PLMS_CLIENT_INFO *client_info)
{
	PLMS_CB *cb = plms_cb;
	PLMS_ENTITY_GROUP_INFO_LIST *client_grp_list, *grp_node;
	SaUint32T	  proc_rc = NCSCC_RC_SUCCESS; 
	
	TRACE_ENTER();
	
	/** Allocate the group hdl and add grp info into patricia tree */
	if (entity_grp_hdl_pool == MAX_ENTITY_GROUP_HANDLES){
		TRACE_5("entity_grp_hdl_pool has reached max limit. Hence wrapped around");
		entity_grp_hdl_pool = 0;
		do{
			entity_grp_hdl_pool++;
			grp_info->entity_grp_hdl = entity_grp_hdl_pool;
			proc_rc = ncs_patricia_tree_add(&cb->entity_group_info,
							&grp_info->pat_node);
		}while(NCSCC_RC_SUCCESS != proc_rc);
	}else{
		entity_grp_hdl_pool++;
		grp_info->entity_grp_hdl = entity_grp_hdl_pool;
		proc_rc = ncs_patricia_tree_add(&cb->entity_group_info,
						&grp_info->pat_node);
		if(NCSCC_RC_SUCCESS != proc_rc){
			LOG_ER("PLMS :Patricia tree add failed. For gep handle : %Lu ",entity_grp_hdl_pool);
			free(grp_info);
			return NCSCC_RC_FAILURE;
		}
	}

	/** add the new group created in entity group list of this client */
	client_grp_list = client_info->entity_group_list;
	grp_node = (PLMS_ENTITY_GROUP_INFO_LIST *)malloc(
				sizeof(PLMS_ENTITY_GROUP_INFO_LIST));
	if(!grp_node){
		LOG_CR("PLMS :  PLMS_ENTITY_GROUP_INFO_LIST memory alloc failed, error val:%s",strerror(errno));
		return NCSCC_RC_FAILURE;
	}
	grp_node->ent_grp_inf = grp_info;
	grp_node->next = NULL;
	if(!client_grp_list){
		client_info->entity_group_list = grp_node;
	}else{
		while(client_grp_list->next)
		{
			client_grp_list = client_grp_list->next;	
		}
		client_grp_list->next = grp_node;
	}
	grp_info->client_info = client_info;
	TRACE_LEAVE();
	
	return NCSCC_RC_SUCCESS;
}

/***********************************************************************//**
* @brief	This function removes the group info node from the grp info
*		list of the client.
*
* @param[in]	grp_info    - grp info structure which has to be removed.
*
* @return	Returns nothing.	
***************************************************************************/
void plm_rem_grp_info_from_client(PLMS_ENTITY_GROUP_INFO *grp_info)
{
	PLMS_ENTITY_GROUP_INFO_LIST *grp_info_list, *prev_grp_node = NULL;
	TRACE_ENTER();
	
	grp_info_list = grp_info->client_info->entity_group_list;
	while(grp_info_list)
	{
		if(grp_info == 
			grp_info_list->ent_grp_inf){
			if(NULL == prev_grp_node){
				grp_info->client_info->entity_group_list =
							grp_info_list->next;
			}else{
				prev_grp_node->next = grp_info_list->next; 
			}	
			free(grp_info_list);
			break;
		}
		prev_grp_node = grp_info_list;
		grp_info_list = grp_info_list->next;
	}
	TRACE_LEAVE();
	return;
}

/***********************************************************************//**
* @brief	This function creates the grp info.
*
* @param[in]	plm_evt    - plm evt structure received.
*
* @return	Returns nothing.	
***************************************************************************/
void plms_process_grp_create_evt(PLMS_EVT *plm_evt)
{

	PLMS_CB           *cb = plms_cb;
	PLMS_CLIENT_INFO  *client_info;
	SaUint32T	  rc = SA_AIS_OK;
	SaUint32T	  proc_rc = NCSCC_RC_SUCCESS; 
	PLMS_EVT          plm_resp;
	PLMS_ENTITY_GROUP_INFO *grp_info = NULL;
	PLMS_MBCSV_MSG mbcsv_msg;

	TRACE_ENTER();

	memset(&plm_resp, 0, sizeof(PLMS_EVT));
	client_info = (PLMS_CLIENT_INFO *)ncs_patricia_tree_get(
			&cb->client_info, 
			(uns8 *)&plm_evt->req_evt.agent_grp_op.plm_handle);
	if (!client_info)
	{
		LOG_ER("PLMS : Received bad PLM Handle");
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto send_resp;
	}

	grp_info = (PLMS_ENTITY_GROUP_INFO *)malloc(
					sizeof(PLMS_ENTITY_GROUP_INFO));
	if (!grp_info){
		LOG_CR("PLMS :  PLMS_ENTITY_GROUP_INFO memory alloc failed, error val:%s",strerror(errno));
		assert(0);
	}
	memset(grp_info, 0, sizeof(PLMS_ENTITY_GROUP_INFO));

	grp_info->agent_mdest_id = plm_evt->sinfo.dest;
	grp_info->pat_node.key_info = (uns8*)&grp_info->entity_grp_hdl;
	/** 
	 * Allocate the grp hdl and update the grp info strucute and 
	 * add it to the patricia tree. 
	 */
	proc_rc = plms_ent_grp_add_to_db(grp_info, client_info);

	if(proc_rc != NCSCC_RC_SUCCESS){
		LOG_ER("PLMS :Grp create failed");
		rc = SA_AIS_ERR_INVALID_PARAM;
		goto send_resp;
	}

	if (cb->ha_state == SA_AMF_HA_ACTIVE)
	{
		/* Update same to standby */
		memset(&mbcsv_msg, 0, sizeof(PLMS_MBCSV_MSG));
		
		mbcsv_msg.header.msg_type = PLMS_A2S_MSG_ENT_GRP_INFO; 
		mbcsv_msg.header.num_records = 1; /* Since async */
		mbcsv_msg.info.ent_grp_info.entity_group_handle = 
		                          entity_grp_hdl_pool;
		mbcsv_msg.info.ent_grp_info.agent_mdest_id = 
						plm_evt->sinfo.dest; 
		proc_rc =  plms_mbcsv_send_async_update(&mbcsv_msg, 
							NCS_MBCSV_ACT_ADD);
		if (proc_rc != NCSCC_RC_SUCCESS)
		{
			LOG_ER("Async update to stdby failed");
			/* FIXME  What to do ?? */
		}
	}
		
	plm_resp.res_evt.hdl = entity_grp_hdl_pool;

send_resp:
	
	plm_resp.req_res = PLMS_RES;
	plm_resp.res_evt.res_type = PLMS_AGENT_GRP_CREATE_RES;
	plm_resp.res_evt.error = rc;

	/* send mds response to plma */
	proc_rc = plm_send_mds_rsp(cb->mds_hdl,
			   NCSMDS_SVC_ID_PLMS,
			   &plm_evt->sinfo,
			   &plm_resp);
	if(NCSCC_RC_SUCCESS != proc_rc){
		/** 
		 * remove the grp info node from grp info list in client info 
		 * delete the grp info node from patricia tree 
		 */
		 
		LOG_ER("PLMS :plm_send_mds_rsp failed");
		plm_rem_grp_info_from_client(grp_info);
		ncs_patricia_tree_del(&cb->entity_group_info, &grp_info->pat_node);
		free(grp_info);

	}

	TRACE_LEAVE();
	return;
}

/***********************************************************************//**
* @brief	This function gets the list of entities to be removed 
*		from the grp.
*
* @param[out]	new_entity_list - list of entities to be removed.
* @param[in]	grp_entity_list - list of entities in a grp.
* @param[in]	ent_names       - array of entities received.
* @param[in]	no_of_ents      - num of entities received.
*
* @return	Returns NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE;.	
***************************************************************************/
SaUint32T plm_rem_entity(PLMS_GROUP_ENTITY_ROOT_LIST **new_entity_list,
		PLMS_GROUP_ENTITY_ROOT_LIST *grp_entity_list, 
		SaNameT *ent_names, SaUint32T no_of_ents)
{
	PLMS_GROUP_ENTITY_ROOT_LIST *list_start = NULL, *ent_list;
	PLMS_GROUP_ENTITY_ROOT_LIST *prev = NULL, *node;
	SaUint32T ii=0;
	SaNameT *key_dn;
	
	TRACE_ENTER();
	
	for(ii=0; ii < no_of_ents; ii++)
	{	
			
		key_dn = &ent_names[ii];
		list_start = grp_entity_list;
		while(list_start)
		{
			if((key_dn->length == list_start->plm_entity->dn_name.length) && (strncmp((SaInt8T *)key_dn->value, (SaInt8T *)list_start->plm_entity->dn_name.value, key_dn->length) == 0)){
			break;
			}

			list_start =  list_start->next;
		}
		node = (PLMS_GROUP_ENTITY_ROOT_LIST *)malloc(
				sizeof(PLMS_GROUP_ENTITY_ROOT_LIST));
		if(!node){
			LOG_CR("PLMS :  PLMS_GROUP_ENTITY_ROOT_LIST memory alloc failed, error val:%s",strerror(errno));
			assert(0);
		}
		memset(node ,0, sizeof(PLMS_GROUP_ENTITY_ROOT_LIST));
		node->plm_entity = list_start->plm_entity;
		node->subtree_root = list_start->subtree_root;
		node->next = NULL;
		if(prev == NULL){
			*new_entity_list = node;
		}else{
			prev->next = node;
		}
		prev = node;
		/**
		 * If it is root of subtree, add the children pointing to the 
		 * root to the delete list. 
		 */
		if(list_start->plm_entity == list_start->subtree_root){
			ent_list = grp_entity_list;
			while(ent_list)
			{
				if(list_start->plm_entity == 
						ent_list->plm_entity){
					ent_list = ent_list->next;
					continue;
				}
				if(list_start->plm_entity == 
						ent_list->subtree_root){
					node = (PLMS_GROUP_ENTITY_ROOT_LIST *)
				malloc(sizeof(PLMS_GROUP_ENTITY_ROOT_LIST));
					if(!node){
						LOG_CR("PLMS :  PLMS_GROUP_ENTITY_ROOT_LIST memory alloc failed, error val:%s",strerror(errno));
						assert(0);
					}
					memset(node, 0, 
					sizeof(PLMS_GROUP_ENTITY_ROOT_LIST));
					node->plm_entity = ent_list->plm_entity;
					node->subtree_root = 
							ent_list->subtree_root;
					node->next = NULL;
					prev->next = node;
					prev = node;
				}
				ent_list = ent_list->next;
			}
		}
	}
	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}
		
/***********************************************************************//**
* @brief	This function removes entities from the grp.
*
* @param[in]	plm_entity  - entity to be removed from grp.
* @param[in]	grp_info    - entity grp from which entity has to be removed.
*
* @return	Returns NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE;.	
***************************************************************************/
void plm_rem_ent_frm_grp(PLMS_ENTITY *plm_entity,  
			PLMS_ENTITY_GROUP_INFO *grp_info) 
{

	PLMS_GROUP_ENTITY_ROOT_LIST *start = grp_info->plm_entity_list;
	PLMS_GROUP_ENTITY_ROOT_LIST *prev = NULL;
	
	TRACE_ENTER();
	while(start)
	{
		if(plm_entity == start->plm_entity){
			if(NULL == prev){
				grp_info->plm_entity_list = start->next;
			}else{
				prev->next = start->next;
			}
			TRACE_5("Entity removed from grp :  %s", plm_entity->dn_name.value);
			free(start);

			break;
		}else{
			prev = start;
			start = start->next;
		}
	}
	PLMS_ENTITY_GROUP_INFO_LIST *start_grp = 
					plm_entity->part_of_entity_groups; 
			
	PLMS_ENTITY_GROUP_INFO_LIST *prev_grp = NULL;
	while(start_grp)
	{
		if(grp_info == start_grp->ent_grp_inf){
			if(NULL == prev_grp){
				plm_entity->part_of_entity_groups = 
							start_grp->next;
			}else{
				prev_grp->next = start_grp->next;
			}
				free(start_grp);
			break;
		}else{
			prev_grp = start_grp;
			start_grp = start_grp->next;
		}
	}
	TRACE_LEAVE();
	return;
}
/***********************************************************************//**
* @brief	This function frees the ckpt entity list.
*
* @param[in]	ckpt_ent_list  - ckpt entity list to be freed.
*
* @return	Returns nothing.	
***************************************************************************/
void plm_free_ckpt_ent_list(PLMS_CKPT_ENTITY_LIST *ckpt_ent_list)
{
	PLMS_CKPT_ENTITY_LIST *next;
	TRACE_ENTER();
	while(ckpt_ent_list)
	{
		next = ckpt_ent_list->next;
		free(ckpt_ent_list);
		ckpt_ent_list = next;
	}
	TRACE_LEAVE();
	return;
	
}
/***********************************************************************//**
* @brief	This function gets the ckpt entity list.
*
* @param[in]	ent_list  - entity list from which the ckpt entity list 
*			    has to be formed.
*
* @return	Returns check point entity list.	
***************************************************************************/
PLMS_CKPT_ENTITY_LIST *plm_get_ckpt_ent_list(PLMS_GROUP_ENTITY_ROOT_LIST *ent_list)
{
	PLMS_GROUP_ENTITY_ROOT_LIST *list = ent_list;
	PLMS_CKPT_ENTITY_LIST *ckpt_list = NULL, *node, *head = NULL;

	TRACE_ENTER();
	
	while(list)
	{
		node = (PLMS_CKPT_ENTITY_LIST *)malloc(sizeof(PLMS_CKPT_ENTITY_LIST));
		if(!node){
			LOG_CR("PLMS :  PLMS_CKPT_ENTITY_LIST memory alloc failed, error val:%s",strerror(errno));
			assert(0);
		}
		memset(node, 0, sizeof(PLMS_CKPT_ENTITY_LIST));
		node->entity_name = list->plm_entity->dn_name;
		if(list->subtree_root){	
			node->root_entity_name = list->subtree_root->dn_name;
		}
		node->next = NULL;
		if(!ckpt_list){
			ckpt_list = node;
			head = ckpt_list;
		}else{
			ckpt_list->next = node;
			ckpt_list=node;
		}
		list = list->next;	
	}
	TRACE_LEAVE();
	return head;
}

/***********************************************************************//**
* @brief	This function removes the entity from entity grp.
*
* @param[in]	plm_evt  -  plm evt received 
*
* @return	Returns nothing.	
***************************************************************************/
void plms_process_grp_rem_evt(PLMS_EVT *plm_evt)
{
	PLMS_CB *cb = plms_cb;
	SaUint32T ii = 0;
	PLMS_EVT plm_resp;
	PLMS_ENTITY_GROUP_INFO  *grp_info;
	SaUint32T               rc = SA_AIS_OK;
	SaUint32T               proc_rc = NCSCC_RC_SUCCESS;
	PLMS_GROUP_ENTITY_ROOT_LIST     *tmp_entity;
	PLMS_GROUP_ENTITY_ROOT_LIST     *del_list = NULL,*tmp_del_list = NULL;
	PLMS_TRACK_INFO trk_info;
	PLMS_MBCSV_MSG mbcsv_msg;
	SaNameT *key_dn;

	TRACE_ENTER();

	
	memset(&trk_info, 0, sizeof(PLMS_TRACK_INFO));
	/* Check if handle is valid and group is created */
	grp_info = (PLMS_ENTITY_GROUP_INFO *)ncs_patricia_tree_get(
				&cb->entity_group_info,
				(uns8 *)&plm_evt->req_evt.agent_grp_op.grp_handle);
	if (!grp_info) 
	{
		LOG_ER("PLMS: BAD GROUP HANDLE RECEIVED");
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto send_resp;
	} 

	/* Add entity referred by entity names to group */
	/* Check if the entity is already added to group */
	for ( ii = 0; 
	      ii < plm_evt->req_evt.agent_grp_op.entity_names_number; 
	      ii++)
	{
		key_dn = &plm_evt->req_evt.agent_grp_op.entity_names[ii];
		memset((key_dn->value)+(key_dn->length), 0,
                        SA_MAX_NAME_LENGTH-(key_dn->length));
		tmp_entity = grp_info->plm_entity_list;
		while(tmp_entity)
		{
			if((key_dn->length == tmp_entity->plm_entity->dn_name.length) && (strncmp((SaInt8T *)key_dn->value, (SaInt8T *)tmp_entity->plm_entity->dn_name.value, key_dn->length) == 0)){
				if((tmp_entity->plm_entity == tmp_entity->subtree_root) || (NULL == tmp_entity->subtree_root)){
					
					break;
				}else{
					LOG_ER("The entity : %s cant be removed", tmp_entity->plm_entity->dn_name.value);
					rc = SA_AIS_ERR_INVALID_PARAM;
					goto send_resp;
					
				}
			
			}else{
				tmp_entity = tmp_entity->next;
			}
	
		}
		if(!tmp_entity){
			LOG_ER("The entity %s Does not exist", key_dn->value);
			rc = SA_AIS_ERR_NOT_EXIST;
			goto send_resp;
		}
				
	}
	
	
	if(NCSCC_RC_SUCCESS != plm_rem_entity(&del_list, 
		grp_info->plm_entity_list, 
		plm_evt->req_evt.agent_grp_op.entity_names,
		plm_evt->req_evt.agent_grp_op.entity_names_number)){
		LOG_ER("PLMS :Entity remove failed");
		rc = SA_AIS_ERR_TRY_AGAIN;
		goto send_resp;
	}
	if(grp_info->track_flags){	
		trk_info.root_entity = NULL;
		trk_info.change_step = SA_PLM_CHANGE_COMPLETED;
		trk_info.track_cause = SA_PLM_CAUSE_GROUP_CHANGE;
		trk_info.root_correlation_id = 
				SA_NTF_IDENTIFIER_UNUSED;
		PLMS_ENTITY_GROUP_INFO_LIST *grp_info_node = 
			(PLMS_ENTITY_GROUP_INFO_LIST *)malloc
			(sizeof(PLMS_ENTITY_GROUP_INFO_LIST));
		if(!grp_info_node){
			LOG_CR("PLMS :  PLMS_ENTITY_GROUP_INFO_LIST memory alloc failed, error val:%s",strerror(errno));
			rc = SA_AIS_ERR_NO_MEMORY;
			goto send_resp;
		}
		grp_info_node->ent_grp_inf = grp_info;
		grp_info_node->next = NULL;
		trk_info.group_info_list = grp_info_node; 
		trk_info.aff_ent_list = plm_get_aff_ent_list(del_list);
		trk_info.grp_op = SA_PLM_GROUP_MEMBER_REMOVED;
	
		if(plms_cbk_call(&trk_info, 1)!= NCSCC_RC_SUCCESS)
		{
			LOG_ER("PLMS : plms_cbk_call failed");
			rc = SA_AIS_ERR_TRY_AGAIN;
			goto send_resp;
		}
	}
	/** remove the entities from the grp. Before that copy del list to tmp ptr so that del list it can be used later. */
	tmp_del_list = del_list;
	while(tmp_del_list)
	{
		plm_rem_ent_frm_grp(tmp_del_list->plm_entity, grp_info);	
		tmp_del_list = tmp_del_list->next;
		
	}
	if (cb->ha_state == SA_AMF_HA_ACTIVE)
	{
		/* Update same to standby */
		memset(&mbcsv_msg, 0, sizeof(PLMS_MBCSV_MSG));
		mbcsv_msg.header.msg_type = PLMS_A2S_MSG_ENT_GRP_INFO;
		mbcsv_msg.header.num_records = 1; /* Since async */
		mbcsv_msg.info.ent_grp_info.entity_group_handle = 
						grp_info->entity_grp_hdl;
		mbcsv_msg.info.ent_grp_info.agent_mdest_id = plm_evt->sinfo.dest;
		mbcsv_msg.info.ent_grp_info.entity_list = 
					plm_get_ckpt_ent_list(del_list);
		mbcsv_msg.info.ent_grp_info.track_flags = grp_info->track_flags;
		mbcsv_msg.info.ent_grp_info.track_cookie = 
							grp_info->track_cookie;
		
		proc_rc =  plms_mbcsv_send_async_update(&mbcsv_msg, NCS_MBCSV_ACT_RMV);
		if (proc_rc != NCSCC_RC_SUCCESS)
		{
			LOG_ER("Async update to stdby failed");
			/* FIXME  What to do ?? */
		}
		plm_free_ckpt_ent_list(mbcsv_msg.info.ent_grp_info.entity_list);
	}

send_resp:
	free_aff_ent_list(trk_info.aff_ent_list);
	free_ent_root_list(del_list);
	
	memset(&plm_resp, 0, sizeof(PLMS_EVT));
	plm_resp.req_res = PLMS_RES;
		
	plm_resp.res_evt.res_type = PLMS_AGENT_GRP_REMOVE_RES;
	plm_resp.res_evt.error = rc;
	/** send response */	
	proc_rc =  plm_send_mds_rsp(cb->mds_hdl,
                    NCSMDS_SVC_ID_PLMS,
                    &plm_evt->sinfo,
                    &plm_resp);
	TRACE_LEAVE();
	return;
}
	

/***********************************************************************//**
* @brief	This function gets the list of single entities to be added 
*		to the entity grp.
*
* @param[out]	entity_list  -  entity list to be filled with the single 
*				entities to be added to the group.
* @param[in]	ent_names    -  array of entity names received from client. 
* @param[in]	no_of_ents   -  num of entity names received from client. 
*
* @return	Refer to SAI-AIS specification for various return values.	
***************************************************************************/
SaUint32T plm_add_single_entity(PLMS_GROUP_ENTITY_ROOT_LIST **entity_list, 
				SaNameT *ent_names, SaUint32T no_of_ents)
{
	PLMS_CB                *cb = plms_cb;
	int ii = 0;
	PLMS_ENTITY            *new_entity;
	SaUint32T              rc = SA_AIS_OK;
	SaNameT		       *key_dn;
	PLMS_GROUP_ENTITY_ROOT_LIST *node,*prev = *entity_list;

	TRACE_ENTER();
	/** Add entity referred by entity names to group */
	for ( ii = 0; 
	      ii < no_of_ents; 
	      ii++)
	{
		key_dn = &ent_names[ii];
		/** Get the entity to be added from patricia tree */
		new_entity = (PLMS_ENTITY *)ncs_patricia_tree_get(
				&cb->entity_info, (uns8 *)key_dn);

		if(!new_entity){
			LOG_ER("The entity %s does not exist", key_dn->value);
			rc = SA_AIS_ERR_NOT_EXIST;
			goto end;
		}
		/** Add the entity to be added to end of entity_list */ 
		node = (PLMS_GROUP_ENTITY_ROOT_LIST *)malloc(
			sizeof(PLMS_GROUP_ENTITY_ROOT_LIST));
		if(!node){
			LOG_CR("PLMS : PLMS_GROUP_ENTITY_ROOT_LIST memory alloc failed, error val:%s",strerror(errno));
			rc = SA_AIS_ERR_NO_MEMORY;
			goto end;
		}
		
		memset(node ,0, sizeof(PLMS_GROUP_ENTITY_ROOT_LIST));
		node->plm_entity = new_entity;
		node->subtree_root = NULL;
		node->next = NULL;
		
		if(prev == NULL){
			*entity_list = node;
		}else{
			prev->next = node;
		}
		prev = node;
		TRACE_5("Entity added : %s", key_dn->value);
	}
end:		
	TRACE_LEAVE();
	return rc;
		
}


/***********************************************************************//**
* @brief	This function gets the list of entities to be added to the 
*		entity group
*
* @param[out]	entity_list  -  entity list to be filled with entities to be 
*				added to the group.
* @param[in]	ent_names    -  array of entity names received from client. 
* @param[in]	no_of_ents   -  num of entity names received from client. 
* @param[in]	option       -  represent how the entity is to be added. 
*				The oprions include SA_PLM_GROUP_SUBTREE or 
*				SA_PLM_GROUP_SUBTREE_HES_ONLY or
*				SA_PLM_GROUP_SUBTREE_EES_ONLY.
*			
* @return	Refer to SAI-AIS specification for various return values.	
***************************************************************************/
SaUint32T plm_add_sub_tree(PLMS_GROUP_ENTITY_ROOT_LIST **entity_list,
			PLMS_GROUP_ENTITY_ROOT_LIST *grp_entity_list,
			SaNameT *ent_names, 
			SaUint32T no_of_ents,
			SaPlmGroupOptionsT option)


{
	int ii = 0;
	PLMS_CB                *cb = plms_cb;      
	PLMS_ENTITY            *new_entity;
	SaUint32T              rc = SA_AIS_OK;
	PLMS_GROUP_ENTITY_ROOT_LIST *node, *prev = NULL, *added_ent;
	SaNameT *key_dn;


	/** Add entities referred by entity names to group */
	for ( ii = 0; 
	      ii < no_of_ents; 
	      ii++)
	{
		
		key_dn = &ent_names[ii];
		/** get the root entity to be added from the patricia tree */
		new_entity = (PLMS_ENTITY *)ncs_patricia_tree_get(
				&cb->entity_info, (uns8 *)key_dn);
		if(!new_entity){
			LOG_ER("the entity %s does not exist", key_dn->value);
			rc = SA_AIS_ERR_NOT_EXIST;
			return rc;
		}
		/** See if this entity is repeated in the entity names array */
		added_ent = *entity_list;
		while(added_ent)
		{
			if(added_ent->plm_entity == new_entity){
				free_ent_root_list(*entity_list);
				*entity_list=NULL;
				LOG_ER("The entity %s is repeated and  cant be added again", new_entity->dn_name.value);
				rc = SA_AIS_ERR_EXIST;
				return rc;
			}
			added_ent = added_ent->next;
		}
		/** Add the entity root to the entity_list */
		node = (PLMS_GROUP_ENTITY_ROOT_LIST *)malloc(
			sizeof(PLMS_GROUP_ENTITY_ROOT_LIST));
		if(!node){
			if(*entity_list){
				free_ent_root_list(*entity_list);
				*entity_list=NULL;
			}
			LOG_CR("PLMS : PLMS_GROUP_ENTITY_ROOT_LIST memory alloc failed, error val:%s",strerror(errno));
			rc = SA_AIS_ERR_NO_MEMORY;
			return rc;
		}

		memset(node ,0, sizeof(PLMS_GROUP_ENTITY_ROOT_LIST));
		node->plm_entity = new_entity;
		node->subtree_root = new_entity;
		node->next = NULL;
		if(prev == NULL){
			*entity_list = node;
		}else{
			prev->next = node;
		}
		prev = node;
		TRACE_5("Entity added : %s",key_dn->value);
		
		/** Get the children of this root. */
		PLMS_GROUP_ENTITY *child_list = NULL;
		plms_chld_get(new_entity->leftmost_child,
					&child_list);
		PLMS_GROUP_ENTITY *start = child_list;
		while(start)
		{
			PLMS_GROUP_ENTITY_ROOT_LIST *tmp_grp_entity_list = grp_entity_list;
			added_ent = *entity_list;
			/** 
			 * See if this child entity is already added in this 
			 * turn when entities in the entity_name list prior to 
			 * this are added.
			 */
			while(added_ent)
			{
				if(added_ent->plm_entity == start->plm_entity){
					TRACE("The entity %s is added just before this turn",
						start->plm_entity->dn_name.value);
					break;
				}
				added_ent = added_ent->next;
			}
			/** 
			 * Skip the child entity if its already mentioned 
			 * in this go.
			 */ 
			if(added_ent){
				start = start->next;
				continue;
			}
			while(tmp_grp_entity_list){
				if(start->plm_entity == 
					tmp_grp_entity_list->plm_entity)
				{
					break;
				}else{
					tmp_grp_entity_list = 
						tmp_grp_entity_list->next;
				}
			}
			if(tmp_grp_entity_list){
				start = start->next;
				continue;
			}
					
			if(SA_PLM_GROUP_SUBTREE_HES_ONLY == option){
				if(start->plm_entity->entity_type != 
						PLMS_HE_ENTITY){
					start = start->next;
					continue;
				}
				
			}else if (SA_PLM_GROUP_SUBTREE_EES_ONLY == option){
				if(start->plm_entity->entity_type != 
						PLMS_EE_ENTITY){
					start = start->next;
					continue;
				}
			}
				
			node = (PLMS_GROUP_ENTITY_ROOT_LIST *)malloc(
			sizeof(PLMS_GROUP_ENTITY_ROOT_LIST));
			if(!node){
				if(*entity_list){
					free_ent_root_list(*entity_list);
				        *entity_list=NULL;
				}
				LOG_CR("PLMS : PLMS_GROUP_ENTITY_ROOT_LIST memory alloc failed, error val:%s",strerror(errno));
				rc = SA_AIS_ERR_NO_MEMORY;
				return rc;
			}
			
			memset(node ,0, sizeof(PLMS_GROUP_ENTITY_ROOT_LIST));
			node->plm_entity = start->plm_entity;
			node->subtree_root = new_entity;
			node->next = NULL;
			prev->next = node;
			prev = node;
			start = start->next;
			TRACE_5("Root Entity %s, child added : %s",new_entity->dn_name.value,  node->plm_entity->dn_name.value);
		}
	}
	return rc;
}

/***********************************************************************//**
* @brief	This function adds the entities to the entity group
*
* @param[in]	plm_evt -  plm evt received from client. 
*			
* @return	Returns nothing.	
***************************************************************************/
void plms_process_grp_add_evt(PLMS_EVT *plm_evt)
{

	PLMS_CB *cb = plms_cb;
	SaPlmGroupOptionsT options;
	PLMS_ENTITY_GROUP_INFO  *grp_info;
	PLMS_GROUP_ENTITY_ROOT_LIST      *new_entity_list = NULL, *tmp_entity;
	PLMS_EVT               plm_resp;
	SaUint32T               rc = SA_AIS_OK;
	SaNameT 		*key_dn;
	PLMS_TRACK_INFO         trk_info;
	PLMS_MBCSV_MSG mbcsv_msg;
	SaUint32T               proc_rc = NCSCC_RC_SUCCESS;
	SaUint32T  ii = 0;


	TRACE_ENTER();

	options = plm_evt->req_evt.agent_grp_op.grp_add_option;

	if (!plm_evt)
	{
		LOG_ER("PLMS :INVALID PLM EVT STRUCTURE");
		rc = SA_AIS_ERR_TRY_AGAIN;
		goto send_resp;
	}

	/* Check is handle is valid and group is created */
	grp_info = (PLMS_ENTITY_GROUP_INFO *)ncs_patricia_tree_get(
				&cb->entity_group_info,
			(uns8 *)&plm_evt->req_evt.agent_grp_op.grp_handle);
	if (!grp_info) 
	{
		LOG_ER("PLMS: BAD GROUP HANDLE RECEIVED");
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto send_resp;
	} 
	/* Add entity referred by entity names to group */
	/* Check if the entity is already added to group */
	for ( ii = 0; 
	      ii < plm_evt->req_evt.agent_grp_op.entity_names_number; 
	      ii++)
	{
		key_dn = &plm_evt->req_evt.agent_grp_op.entity_names[ii];
		memset((key_dn->value)+(key_dn->length), 0,
                        SA_MAX_NAME_LENGTH-(key_dn->length));
		tmp_entity = grp_info->plm_entity_list;
		while(tmp_entity)
		{
			if(key_dn->length == tmp_entity->plm_entity->dn_name.length){
				/** See if the entity already exists */
				if((strncmp((SaInt8T *)key_dn->value,
			(SaInt8T *)tmp_entity->plm_entity->dn_name.value , 
				key_dn->length)) == 0){
					/* Added as single entity */
					LOG_ER("PLMS : The entity %s already exists", key_dn->value);
					rc = SA_AIS_ERR_EXIST;
					goto send_resp;	
				}
				
			}
			tmp_entity = tmp_entity->next;
		}
	}

	/* Check for different option types and proceed accordingly */
	if (options == SA_PLM_GROUP_SINGLE_ENTITY)
	{
		/* Add entity referred by entity names to group */
		
		rc = plm_add_single_entity(&new_entity_list,
			plm_evt->req_evt.agent_grp_op.entity_names, 
			plm_evt->req_evt.agent_grp_op.entity_names_number);
		if(rc != SA_AIS_OK){
			goto send_resp;
		}

	}else{
	
		/* Add entity referred by entity names to group */
		rc = plm_add_sub_tree(&new_entity_list,
		grp_info->plm_entity_list,
		plm_evt->req_evt.agent_grp_op.entity_names,
		plm_evt->req_evt.agent_grp_op.entity_names_number,
		options);
		if(rc != SA_AIS_OK){
			goto send_resp;
		}
			
	}
					
						
	/** Add the entities to the entity list */
	PLMS_GROUP_ENTITY_ROOT_LIST *list_start = new_entity_list;
	while(list_start)
	{
		PLMS_ENTITY_GROUP_INFO_LIST *list = 
				list_start->plm_entity->part_of_entity_groups;
		PLMS_ENTITY_GROUP_INFO_LIST *new;
		new = (PLMS_ENTITY_GROUP_INFO_LIST *)malloc(
				sizeof(PLMS_ENTITY_GROUP_INFO_LIST));
		if(!new){
			LOG_CR("PLMS : PLMS_ENTITY_GROUP_INFO_LIST memory alloc failed, error val:%s",strerror(errno));
			rc = SA_AIS_ERR_NO_MEMORY;
			goto send_resp;
		}
		new->ent_grp_inf = grp_info;
		new->next = NULL;
		if(list == NULL){
			
			list_start->plm_entity->part_of_entity_groups = new;
		
		}else{
			while(list->next)
			{
				list = list->next;
			}
			list->next = new;
		}
		
		list_start = list_start->next;
	}
	
	PLMS_GROUP_ENTITY_ROOT_LIST *entity_head = grp_info->plm_entity_list;
	if(NULL == entity_head){
		grp_info->plm_entity_list = new_entity_list;
		rc = SA_AIS_OK;
	}else{
		while(entity_head->next){
			entity_head = entity_head->next;
		}
		entity_head->next = new_entity_list;
		TRACE_5("Num of entities added to grp with grp hdl %Lu are : %u ",  plm_evt->req_evt.agent_grp_op.grp_handle, plm_evt->req_evt.agent_grp_op.entity_names_number);
	}
	if (cb->ha_state == SA_AMF_HA_ACTIVE)
	{
		/* Update same to standby */
		memset(&mbcsv_msg, 0, sizeof(PLMS_MBCSV_MSG));
		mbcsv_msg.header.msg_type = PLMS_A2S_MSG_ENT_GRP_INFO;
		mbcsv_msg.header.num_records = 1; /* Since async */
		mbcsv_msg.info.ent_grp_info.entity_group_handle = 
						grp_info->entity_grp_hdl;
		mbcsv_msg.info.ent_grp_info.agent_mdest_id = plm_evt->sinfo.dest;
		mbcsv_msg.info.ent_grp_info.entity_list = 
					plm_get_ckpt_ent_list(new_entity_list);
		mbcsv_msg.info.ent_grp_info.track_flags = grp_info->track_flags;
		mbcsv_msg.info.ent_grp_info.track_cookie = 
							grp_info->track_cookie;
		
		proc_rc =  plms_mbcsv_send_async_update(&mbcsv_msg, NCS_MBCSV_ACT_ADD);
		if (proc_rc != NCSCC_RC_SUCCESS)
		{
			LOG_ER("Async update to stdby failed");
			/* FIXME  What to do ?? */
		}
		plm_free_ckpt_ent_list(mbcsv_msg.info.ent_grp_info.entity_list);
	}

	if(grp_info->track_flags){	
		memset(&trk_info, 0, sizeof(PLMS_TRACK_INFO));
		trk_info.root_entity = NULL;
		trk_info.change_step = SA_PLM_CHANGE_COMPLETED;
		trk_info.track_cause = SA_PLM_CAUSE_GROUP_CHANGE;
		trk_info.root_correlation_id = 
				SA_NTF_IDENTIFIER_UNUSED;
		PLMS_ENTITY_GROUP_INFO_LIST *grp_info_node = 
			(PLMS_ENTITY_GROUP_INFO_LIST *)malloc
			(sizeof(PLMS_ENTITY_GROUP_INFO_LIST));
		if(!grp_info_node){
			LOG_CR("PLMS : PLMS_ENTITY_GROUP_INFO_LIST memory alloc failed, error val:%s",strerror(errno));
			rc = SA_AIS_ERR_NO_MEMORY;
			goto send_resp;
		}
		grp_info_node->ent_grp_inf = grp_info;
		grp_info_node->next = NULL;
		trk_info.group_info_list = grp_info_node;
		trk_info.root_entity = NULL;
		trk_info.aff_ent_list = plm_get_aff_ent_list(new_entity_list);
		trk_info.grp_op = SA_PLM_GROUP_MEMBER_ADDED;
	
		if(plms_cbk_call(&trk_info, 1)!= NCSCC_RC_SUCCESS)
		{
			rc = SA_AIS_ERR_TRY_AGAIN;
			goto send_resp;
		}
		free_aff_ent_list(trk_info.aff_ent_list);
	}
	
send_resp:
	memset(&plm_resp, 0, sizeof(PLMS_EVT));
	plm_resp.req_res = PLMS_RES;
	plm_resp.res_evt.res_type = PLMS_AGENT_GRP_ADD_RES;
	plm_resp.res_evt.error = rc;
	plm_resp.req_res  = PLMS_RES;
	
	/** Send response */
	plm_send_mds_rsp(cb->mds_hdl,
                    NCSMDS_SVC_ID_PLMS,
                    &plm_evt->sinfo,
                    &plm_resp);

	
	TRACE_LEAVE();
	return;
}

void plms_process_grp_del_evt(PLMS_EVT *plm_evt)
{
	PLMS_CB                *cb = plms_cb;  
	PLMS_ENTITY_GROUP_INFO  *grp_info;
	SaUint32T               rc = SA_AIS_OK;
	PLMS_EVT                plm_resp; 
	PLMS_MBCSV_MSG mbcsv_msg;
	SaUint32T proc_rc = NCSCC_RC_SUCCESS;
	
	/* Check is handle is valid and group is created */
	grp_info = (PLMS_ENTITY_GROUP_INFO *)ncs_patricia_tree_get(
				&cb->entity_group_info,
			(uns8 *)&plm_evt->req_evt.agent_grp_op.grp_handle);
	if (!grp_info) 
	{
		LOG_ER("PLMS: BAD GROUP HANDLE RECEIVED");
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto send_resp;
	} 

	if (cb->ha_state == SA_AMF_HA_ACTIVE)
	{
		/* Update same to standby */
		memset(&mbcsv_msg, 0, sizeof(PLMS_MBCSV_MSG));
		
		mbcsv_msg.header.msg_type = PLMS_A2S_MSG_ENT_GRP_INFO; 
		mbcsv_msg.header.num_records = 1; /* Since async */
		mbcsv_msg.info.ent_grp_info.entity_group_handle = 
		                          grp_info->entity_grp_hdl;
		mbcsv_msg.info.ent_grp_info.agent_mdest_id = 
						plm_evt->sinfo.dest; 
		proc_rc =  plms_mbcsv_send_async_update(&mbcsv_msg, 
							NCS_MBCSV_ACT_RMV);
		if (proc_rc != NCSCC_RC_SUCCESS)
		{
			LOG_ER("Async update to stdby failed");
			/* FIXME  What to do ?? */
		}
	}
	/** remove entity group from entity grp list in client info */
	plm_rem_grp_info_from_client(grp_info);
	plm_clean_grp_info(grp_info);
send_resp:	
	memset(&plm_resp, 0, sizeof(PLMS_EVT));
	plm_resp.req_res = PLMS_RES;
	plm_resp.res_evt.res_type = PLMS_AGENT_GRP_DEL_RES;
	plm_resp.res_evt.error = rc;

	/** send the response */
	plm_send_mds_rsp(cb->mds_hdl,
                    NCSMDS_SVC_ID_PLMS,
                    &plm_evt->sinfo,
                    &plm_resp);

	TRACE_LEAVE();	
	return;
}

void plms_clean_agent_db(MDS_DEST agent_mdest_id,SaAmfHAStateT ha_state)
{
	PLMS_CB *cb = plms_cb;
	PLMS_CLIENT_INFO *client_info;
	PLMS_CKPT_CLIENT_INFO_LIST *client_node;
	PLMS_MBCSV_MSG msg;
	SaPlmHandleT plm_hdl;
	TRACE_ENTER();	
	
	client_info = (PLMS_CLIENT_INFO *)ncs_patricia_tree_getnext(
					&cb->client_info, NULL);
	while(client_info)
	{
		plm_hdl = client_info->plm_handle;
		if(client_info->mdest_id == agent_mdest_id)

	        {	
			if(client_info){
				if(SA_AMF_HA_STANDBY == ha_state){
					client_node = (PLMS_CKPT_CLIENT_INFO_LIST *)malloc(sizeof(PLMS_CKPT_CLIENT_INFO_LIST));
					if(!client_node){
						LOG_CR("PLMS : PLMS_CKPT_CLIENT_INFO_LIST memory alloc failed, error val:%s",strerror(errno));
						assert(0);
					}
					memset(client_node, 0, 
						sizeof(PLMS_CKPT_CLIENT_INFO_LIST));
				
					client_node->plm_handle = 
							client_info->plm_handle;
					client_node->mdest_id = 
							client_info->mdest_id;		
					client_node->next = NULL;
					
					PLMS_CKPT_CLIENT_INFO_LIST *start = 
							cb->client_down_list;
					
					if(start == NULL){
						cb->client_down_list = client_node; 
					}else{
						while(start->next != NULL)	
						{
							start = start->next;
						}
						start->next = client_node; 
					}
						
				}else{
					memset(&msg, 0, sizeof(PLMS_MBCSV_MSG));
					msg.header.msg_type = 
						PLMS_A2S_MSG_CLIENT_DOWN_INFO;
					msg.info.client_info.plm_handle = 
						client_info->plm_handle;
					msg.info.client_info.mdest_id= 
							client_info->mdest_id;
					
					/** 
					 * send the sync update to Standby 
					 * indicating that the clean up of 
					 * client database is done.
					 */
					 plms_mbcsv_send_async_update(&msg,
					 		NCS_MBCSV_ACT_UPDATE); 
					 plm_clean_client_info(client_info); 
				}
				
			}
		}
		client_info = (PLMS_CLIENT_INFO *)ncs_patricia_tree_getnext(
			&cb->client_info, (uns8 *)&plm_hdl);
	}
	TRACE_LEAVE();
	return;
}



void plms_process_agent_lib_req(PLMS_EVT *plm_evt)
{
	TRACE_ENTER();
	switch (plm_evt->req_evt.agent_lib_req.lib_req_type)
	{
		case PLMS_AGENT_INIT_EVT:
			plms_process_agent_initialize(plm_evt);
			break;
		case PLMS_AGENT_FINALIZE_EVT:
			plms_process_agent_finalize(plm_evt);
			break;
		default:
			LOG_ER("PLMS-PLMA- INVALID AGENT LIB REQUEST");
			break;
	}
	TRACE_LEAVE();
	return;
}

void plms_process_agent_grp_op_req(PLMS_EVT *plm_evt)
{

	switch(plm_evt->req_evt.agent_grp_op.grp_evt_type)
	{
		case PLMS_AGENT_GRP_CREATE_EVT:
			 plms_process_grp_create_evt(plm_evt);
		 	break;
		case PLMS_AGENT_GRP_REMOVE_EVT:
			 plms_process_grp_rem_evt(plm_evt);
			 break;
		case PLMS_AGENT_GRP_ADD_EVT:
			 plms_process_grp_add_evt(plm_evt); 
			 break;
		case PLMS_AGENT_GRP_DEL_EVT:
			 plms_process_grp_del_evt(plm_evt);
			 break;
		default:
			 break;
	}
	return;
}

void plms_process_agent_track_req(PLMS_EVT *plm_evt)
{
	switch(plm_evt->req_evt.agent_track.evt_type)
	{
		case PLMS_AGENT_TRACK_START_EVT:
			 plms_process_trk_start_evt(plm_evt);
		 	break;
		case PLMS_AGENT_TRACK_STOP_EVT:
			 plms_process_trk_stop_evt(plm_evt);
			 break;
		case PLMS_AGENT_TRACK_READINESS_IMPACT_EVT:
			 plms_readiness_impact_process(plm_evt);
			 break;
		case PLMS_AGENT_TRACK_RESPONSE_EVT:
			 plms_cbk_response_process(plm_evt);
			 break;
		default:
			 break;
	}
}

void plms_process_mds_info_event(PLMS_EVT *plm_evt)
{
	PLMS_MDS_INFO  *mds_info;
	PLMS_CB *cb = plms_cb;

	TRACE_ENTER();

	mds_info = &plm_evt->req_evt.mds_info;
	if (mds_info->svc_id == NCSMDS_SVC_ID_PLMS)
		TRACE_5("Received PLMS service event");
	else if (mds_info->svc_id == NCSMDS_SVC_ID_PLMA)
	        TRACE_5("Received PLMA service event");
	else if (mds_info->svc_id == NCSMDS_SVC_ID_PLMS_HRB)
	        TRACE_5("Received HRB service event");
	else {
										                LOG_ER("Received a service event for an unknown service %u", mds_info->svc_id);
		return;
	}

	switch(mds_info->change)
	{
		case NCSMDS_RED_UP:
			break;
		case NCSMDS_UP:
			if (mds_info->svc_id == NCSMDS_SVC_ID_PLMS_HRB) {
				cb->hrb_dest = plm_evt->req_evt.mds_info.dest; 
				LOG_ER("Received MDS UP for PLMS_HRB");
			}
			break;
		case NCSMDS_DOWN:
			if (mds_info->svc_id == NCSMDS_SVC_ID_PLMA) {
				TRACE_5("unhandled DOWN case ? 	NCSMDS_DOWN for PLMS");
				plms_clean_agent_db(plm_evt->req_evt.mds_info.dest,cb->ha_state);
			}

			break;
		default:
			TRACE_1("Unhandled MDS change: %u", mds_info->change);
			break;
	}
	TRACE_LEAVE();
	return;
}

/****************************************************************************
 * Name          : plms_process_event
 *
 * Description   : This is the entry function which parses the event type and .
 *                 calls different routines depending upon the event type.
 *
 * Arguments     : PLMS_EVT *  
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * Notes         : None.
 *****************************************************************************/
	     
SaUint32T plms_process_event()
{
	PLMS_EVT   *plm_evt = NULL;
	PLMS_CB    *cb = plms_cb;

	TRACE_ENTER();
	
	plm_evt = (PLMS_EVT *)m_NCS_IPC_NON_BLK_RECEIVE(&cb->mbx, plm_evt);

	if (!plm_evt)
	{
		/* FIXME : Select appropriate return type */
		return NCSCC_RC_FAILURE;
	}

	/* Acquqire lock for PLMS CB */
	/*m_NCS_LOCK(&plms_cb->cb_lock,NCS_LOCK_WRITE);*/

	
	if (plm_evt->req_res == PLMS_REQ)
	{

		switch(plm_evt->req_evt.req_type)
		{
			case PLMS_IMM_ADM_OP_EVT_T:
				plms_imm_adm_op_req_process(plm_evt);
				break;
			case PLMS_HPI_EVT_T:
				plms_hpi_hs_evt_process(plm_evt);
				break;
			case PLMS_AGENT_LIB_REQ_EVT_T:
				plms_process_agent_lib_req(plm_evt);
				break;
			case PLMS_AGENT_GRP_OP_EVT_T:
				plms_process_agent_grp_op_req(plm_evt);
				break;
			case PLMS_AGENT_TRACK_EVT_T:
				plms_process_agent_track_req(plm_evt);
				break;
			case PLMS_PLMC_EVT_T:
				plms_plmc_mbx_evt_process(plm_evt);
				break;
			case PLMS_TMR_EVT_T:
				plms_mbx_tmr_handler(plm_evt);
				break;
			case PLMS_MDS_INFO_EVT_T:
				plms_process_mds_info_event(plm_evt);
				break;
			case PLMS_DUMP_CB_EVT_T:
				plms_cb_dump_routine();
				break;
				
			default :
				break;
		}
	} else if ( plm_evt->req_res == PLMS_RES )
	{
	}	
/*	m_NCS_UNLOCK(&plms_cb->cb_lock,NCS_LOCK_WRITE); */
	plms_free_evt(plm_evt);
	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}
