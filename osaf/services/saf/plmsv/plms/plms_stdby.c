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

/*****************************************************************************
*  MODULE NAME:  plms_stdby.c                                                *
*                                                                            *
*                                                                            *
*  DESCRIPTION: This module contains routines to rebuild the database on     *
*		state change from standby to active. Also contains routines  *
*		to perform pending admin operations			     *
*                                                                            *
*****************************************************************************/

/***********************************************************************
 *   INCLUDE FILES
***********************************************************************/
#include "pthread.h"
#include "plms.h"
#include "plms_evt.h"
#include "plms_hsm.h"
#include "plms_utils.h"


/***********************************************************************
*   FUNCTION PROTOTYPES
***********************************************************************/
SaUint32T plms_proc_standby_active_role_change();
SaUint32T plms_build_ent_grp_tree();
void plms_perform_pending_admin_operation();
SaUint32T plms_perform_pending_admin_lock(
				PLMS_CKPT_TRACK_STEP_INFO  *tack_step,
				PLMS_ENTITY   *ent);
SaUint32T plms_perform_pending_admin_shutdown(
				PLMS_CKPT_TRACK_STEP_INFO  *tack_step,
				PLMS_ENTITY   *ent);
SaUint32T plms_perform_pending_admin_deactivate(
				PLMS_CKPT_TRACK_STEP_INFO  *track_step,
				PLMS_ENTITY   *ent);
static SaUint32T plms_perform_pending_admin_clbk(
					PLMS_ENTITY_GROUP_INFO_LIST *grp_list,
		  			PLMS_GROUP_ENTITY *aff_ent_list,
					PLMS_CKPT_TRACK_STEP_INFO *track_step);
void plms_process_client_down_list();

/***********************************************************************
 * Name          :plms_proc_standby_active_role_change
 *
 *
 * Description   : 
 *                        
 * Arguments     : 
 *                        
 * Return Values :NCSCC_RC_SUCCESS 
 *		  NCSCC_RC_FAILURE
 *                        
***********************************************************************/
SaUint32T plms_proc_standby_active_role_change()
{
	PLMS_CB       *cb = plms_cb;
	SaUint32T     rc;

	TRACE_ENTER();

	/*Register with IMM and build database */
	plms_imm_intf_initialize();

	/* Build entity_group tree */
	rc = plms_build_ent_grp_tree();
	if( NCSCC_RC_SUCCESS != rc ){
		LOG_ER("Failed to build entity group tree");
		return NCSCC_RC_FAILURE;
	}

	plms_perform_pending_admin_operation();

	plms_process_client_down_list();
	
	cb->is_initialized = TRUE;

	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}
/***********************************************************************
 * Name          : plms_build_epath_to_entity_map_tree
 *
 *
 * Description   :
 *
 * Arguments     :
 *
 * Return Values :
 *
***********************************************************************/
SaUint32T plms_build_epath_to_entity_map_tree()
{
	SaUint32T (* hsm_func_ptr)(SaInt8T *epath_str,SaHpiEntityPathT *epath_ptr) = NULL;
	PLMS_EPATH_TO_ENTITY_MAP_INFO *epath_to_ent;
	PLMS_ENTITY *plm_ent;
	PLMS_CB       *cb = plms_cb;
	SaUint32T ret_err = NCSCC_RC_SUCCESS;

	hsm_func_ptr = dlsym(cb->hpi_intf_hdl, "convert_string_to_epath");
	if ( NULL == hsm_func_ptr ) {
		LOG_ER("dlsym() of HPI lib failed with error %s", dlerror());
		return NCSCC_RC_FAILURE;
	}
	plm_ent = (PLMS_ENTITY *) ncs_patricia_tree_getnext(&cb->entity_info,
			(SaUint8T *) 0);
	while (plm_ent != NULL) {
		if ((plm_ent->entity_type == PLMS_HE_ENTITY) &&
		(plm_ent->entity.he_entity.saPlmHECurrEntityPath != NULL)) {
			/* Add the epath to entity mapping to patricia tree.*/
			epath_to_ent = (PLMS_EPATH_TO_ENTITY_MAP_INFO *)calloc(1,
					sizeof(PLMS_EPATH_TO_ENTITY_MAP_INFO));
			epath_to_ent->entity_path = (SaInt8T *)calloc(1,
			strlen(plm_ent->entity.he_entity.saPlmHECurrEntityPath) +1);
			strcpy(epath_to_ent->entity_path, plm_ent->entity.
				he_entity.saPlmHECurrEntityPath);
			ret_err = (* hsm_func_ptr)(epath_to_ent->entity_path,
				&epath_to_ent->epath_key);
			if ( NCSCC_RC_SUCCESS != ret_err  ){
				LOG_ER("function returned the error %d", ret_err);
				return ret_err;
			}
			epath_to_ent->plms_entity = plm_ent;
			epath_to_ent->pat_node.key_info = (SaUint8T *)
				&(epath_to_ent->epath_key);
			ret_err = ncs_patricia_tree_add(
					&cb->epath_to_entity_map_info,
					&epath_to_ent->pat_node);
			if (NCSCC_RC_SUCCESS != ret_err){
				LOG_ER("Patricia tree add of epath_to_ent to \
				cb->epath_to_entity_map_info failed.\
				ret_err = %d", ret_err);
				return ret_err;
			}
			TRACE("Entity Path: %s of entity: %s added to epath_to_ent_map tree",
				plm_ent->entity.he_entity.saPlmHECurrEntityPath, plm_ent->dn_name_str);
		}
		plm_ent = (PLMS_ENTITY *) ncs_patricia_tree_getnext(&cb->entity_info,
			(SaUint8T *) &plm_ent->dn_name);
	}
	return ret_err;
}
/***********************************************************************
 * Name          : plms_build_ent_grp_tree 
 *
 *
 * Description   :  
 *                        
 * Arguments     : 
 *                        
 * Return Values : 
 *
***********************************************************************/
SaUint32T plms_build_ent_grp_tree()
{
	PLMS_CB       *cb = plms_cb;
	PLMS_CKPT_ENTITY_GROUP_INFO      *ckpt_grp_info = NULL;
	PLMS_CKPT_ENTITY_GROUP_INFO      *tmp_ckpt_grp_info = NULL;
	PLMS_GROUP_ENTITY_ROOT_LIST      *grp_entity_list = NULL;
	PLMS_CKPT_ENTITY_LIST		 *entity_list = NULL;
	PLMS_CKPT_ENTITY_LIST		 *tmp_entity_list = NULL;
	PLMS_ENTITY            		*plm_entity = NULL;
	PLMS_ENTITY            		*root_plm_entity = NULL;
	PLMS_CLIENT_INFO  *client_info = NULL;
	PLMS_ENTITY_GROUP_INFO  *grp_info = NULL;
	PLMS_ENTITY_GROUP_INFO_LIST *client_grp_list = NULL,*grp_node = NULL;

	TRACE_ENTER();

	if(NULL == cb->ckpt_entity_grp_info){
		TRACE("Nothing to be build from ckpt_ent_grp_info");
		return NCSCC_RC_SUCCESS;
	}

	ckpt_grp_info = cb->ckpt_entity_grp_info;
	while(ckpt_grp_info){
		/* Get client corresponding to entity_group */
		client_info = (PLMS_CLIENT_INFO *)ncs_patricia_tree_getnext(
		        &cb->client_info,(SaUint8T *)0);
		if(!client_info){
			LOG_ER("Bad PLM Handle");
			continue;
		}
		while(client_info){
			if(client_info->mdest_id==ckpt_grp_info->agent_mdest_id)
				break;
			client_info =
			(PLMS_CLIENT_INFO *)ncs_patricia_tree_getnext(
			&cb->client_info,(SaUint8T *)&client_info->plm_handle);
		}

		/* If corresponding client_info is not found return failure */
		if(NULL == client_info){
			LOG_ER("Client_info corresponding to mdest:%qu is not \
				found",ckpt_grp_info->agent_mdest_id);
			return NCSCC_RC_SUCCESS;
		}

		/* Check if handle is valid and group is already created */
		grp_info = (PLMS_ENTITY_GROUP_INFO *)ncs_patricia_tree_get(
			&cb->entity_group_info,
			(SaUint8T *)&ckpt_grp_info->entity_group_handle);

		if (!grp_info){
			TRACE("Creating entity_grp_info hdl:%qu",
					ckpt_grp_info->entity_group_handle);
			
			if(ckpt_grp_info->entity_group_handle > entity_grp_hdl_pool)
				entity_grp_hdl_pool = ckpt_grp_info->entity_group_handle; 

			/* Entity_grp_create operation */
			grp_info =(PLMS_ENTITY_GROUP_INFO *)malloc(
			                      sizeof(PLMS_ENTITY_GROUP_INFO));
			if (NULL == grp_info){
				LOG_ER("memory alloc failed error :%s",
							strerror(errno)); 
				assert(0);
			}
			memset(grp_info, 0, sizeof(PLMS_ENTITY_GROUP_INFO));
			grp_info->entity_grp_hdl = 
				ckpt_grp_info->entity_group_handle;
			grp_info->agent_mdest_id=ckpt_grp_info->agent_mdest_id;
			grp_info->pat_node.key_info = 
				(uns8*)&grp_info->entity_grp_hdl;
			grp_info->track_flags =ckpt_grp_info->track_flags;
			grp_info->track_cookie = ckpt_grp_info->track_cookie;

			grp_info->client_info = client_info;

			/* Add entity_group to cb->entity_info*/
			if(NCSCC_RC_SUCCESS != 
				ncs_patricia_tree_add(&cb->entity_group_info,
				                      &grp_info->pat_node)){
				LOG_ER("ENTIRY GROUP INFO NODE ADD FAILED");
				free(grp_info);
				continue;
			}
				
		}
		/* add the new group created in entity group list of this client */
		client_grp_list = client_info->entity_group_list;
		grp_node = (PLMS_ENTITY_GROUP_INFO_LIST *)malloc(sizeof(PLMS_ENTITY_GROUP_INFO_LIST));
		if(!grp_node){
			LOG_CR("PLMS_ENTITY_GROUP_INFO_LIST memory alloc failed, error val:%s",strerror(errno));
			free(grp_info);
			continue;
		}
		memset(grp_node, 0, sizeof(PLMS_ENTITY_GROUP_INFO_LIST));
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

		/* Check and add entities to the entity_group */
		if(NULL == ckpt_grp_info->entity_list){
			ckpt_grp_info = ckpt_grp_info->next;
			/* Entities are not yet added to entity_group */
			continue;
		}
			
		entity_list = ckpt_grp_info->entity_list;

		while(entity_list){
			TRACE("Adding entity :%s to ent_grp_root list",
			entity_list->entity_name.value);
			plm_entity = (PLMS_ENTITY *)ncs_patricia_tree_get(
			        &cb->entity_info,
				(SaUint8T *)&entity_list->entity_name);
			if(NULL == plm_entity){
				LOG_ER("plm_entity corresponding to dn_name:%s\
				  is not found",entity_list->entity_name.value);
				entity_list  = entity_list->next;
				continue;
			}
			if(entity_list->root_entity_name.length){
				root_plm_entity = 
				(PLMS_ENTITY *)ncs_patricia_tree_get(
				&cb->entity_info,
				(SaUint8T *)&entity_list->root_entity_name);
				if(NULL == root_plm_entity){
					LOG_ER("root plm corresponding to \
					dn_name:%s not found",
					entity_list->root_entity_name.value);
					entity_list  = entity_list->next;
					continue;
				}
			}else
				root_plm_entity = NULL;

			grp_entity_list = (PLMS_GROUP_ENTITY_ROOT_LIST *)
			malloc(sizeof(PLMS_GROUP_ENTITY_ROOT_LIST));
			if(NULL == grp_entity_list){
				LOG_ER("memory alloc failed error :%s",
							strerror(errno)); 
				assert(0);
			}
			memset(grp_entity_list,0,
					sizeof(PLMS_GROUP_ENTITY_ROOT_LIST));
			grp_entity_list->plm_entity = plm_entity;
			grp_entity_list->subtree_root = root_plm_entity;

			grp_entity_list->next = grp_info->plm_entity_list;
			grp_info->plm_entity_list = grp_entity_list;

			/* Update part_of_entity_groups list of entity */
			PLMS_ENTITY_GROUP_INFO_LIST *egrp_list = plm_entity->part_of_entity_groups;
			PLMS_ENTITY_GROUP_INFO_LIST *new = (PLMS_ENTITY_GROUP_INFO_LIST *)
							malloc(sizeof(PLMS_ENTITY_GROUP_INFO_LIST));
			if(!new){
				LOG_CR("PLMS : PLMS_ENTITY_GROUP_INFO_LIST memory alloc failed, error val:%s",
											strerror(errno));
				assert(0);
			}
			memset(new,0,sizeof(PLMS_ENTITY_GROUP_INFO_LIST));
			new->ent_grp_inf = grp_info;
			new->next = NULL;
			if(egrp_list == NULL){
				plm_entity->part_of_entity_groups = new;
			}else{
				while(egrp_list->next){
					egrp_list = egrp_list->next;
				}
				egrp_list->next = new;
			}
			entity_list  = entity_list->next;
		}
					
		ckpt_grp_info = ckpt_grp_info->next;
	}


	/* Free the ckpt_grp_info*/
	ckpt_grp_info = cb->ckpt_entity_grp_info;
	while(ckpt_grp_info){
		entity_list = ckpt_grp_info->entity_list;
		/* free entity_group_list */
		while(entity_list){
			tmp_entity_list = entity_list;
			entity_list  = entity_list->next;
			free(tmp_entity_list);
		}
		tmp_ckpt_grp_info = ckpt_grp_info;
		ckpt_grp_info = ckpt_grp_info->next;
		free(tmp_ckpt_grp_info);
	}
	cb->ckpt_entity_grp_info = NULL;

	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}
/***********************************************************************
 * Name          :plms_perform_pending_admin_operation 
 *
 *
 * Description   :  
 *                        
 * Arguments     : 
 *                        
 * Return Values : 
 *
***********************************************************************/
void plms_perform_pending_admin_operation()
{
	PLMS_CB       *cb = plms_cb;
	PLMS_CKPT_TRACK_STEP_INFO  *tmp_track_step = NULL;
	PLMS_CKPT_TRACK_STEP_INFO  *track_step = cb->step_info;
	PLMS_ENTITY   *ent;

	TRACE_ENTER();

	while(track_step){
		if(SA_PLM_CHANGE_START != track_step->change_step && 
		   SA_PLM_CHANGE_VALIDATE != track_step->change_step){
			/* Log the error */
			LOG_ER("Invalid pending admin operation :%d ",
				track_step->change_step);

			tmp_track_step = track_step;
			track_step = track_step->next;	
			free(tmp_track_step);
			continue;
		}
	
		ent = (PLMS_ENTITY *)ncs_patricia_tree_get(&(cb->entity_info),
					   (SaUint8T *)&(track_step->dn_name));
		if (NULL == ent) {
			LOG_ER("Unable to find entity:%s",
				track_step->dn_name.value);

			tmp_track_step = track_step;
			track_step = track_step->next;	
			free(tmp_track_step);
			continue;
		}

		switch(track_step->opr_type){
			case SA_PLM_CAUSE_LOCK:
				plms_perform_pending_admin_lock(
							track_step,ent);
				break;
			case SA_PLM_CAUSE_SHUTDOWN:
				plms_perform_pending_admin_shutdown(
							track_step,ent);
				break;
			case SA_PLM_CAUSE_HE_DEACTIVATION:
				plms_perform_pending_admin_deactivate(
							track_step, ent);
				break;
			default:
				LOG_ER("Invalid pending Admin operation");
				break;
		}

		tmp_track_step = track_step;
		track_step = track_step->next;	
		free(tmp_track_step);

	}
	cb->step_info = NULL;
	TRACE_LEAVE();
}
/***********************************************************************
 * Name          :plms_process_client_down_list
 *
 *
 * Description   :  
 *                        
 * Arguments     : 
 *                        
 * Return Values : 
 *
***********************************************************************/
void plms_process_client_down_list()
{
	PLMS_CB       *cb = plms_cb;
	PLMS_CKPT_CLIENT_INFO_LIST     *client_down_list = cb->client_down_list;
	PLMS_CKPT_CLIENT_INFO_LIST     *tmp_client_list = NULL;

	TRACE_ENTER();

	while(client_down_list){
		plms_clean_agent_db(client_down_list->mdest_id,SA_AMF_HA_ACTIVE);
		tmp_client_list = client_down_list;
		client_down_list = client_down_list->next;
		free(tmp_client_list);
	}
	cb->client_down_list = NULL;
	
	TRACE_LEAVE();
}
/***********************************************************************
 * Name          :plms_perform_pending_admin_clbk
 *
 *
 * Description   :  
 *                        
 * Arguments     : 
 *                        
 * Return Values : 
 *
***********************************************************************/
static SaUint32T plms_perform_pending_admin_clbk(
					PLMS_ENTITY_GROUP_INFO_LIST *grp_list,
		  			PLMS_GROUP_ENTITY *aff_ent_list,
					PLMS_CKPT_TRACK_STEP_INFO *track_step)
{
	PLMS_CB       *cb = plms_cb;
	PLMS_ENTITY   *ent = NULL;
	SaInvocationT inv_id = 0;
        PLMS_EVT agent_evt;
	SaUint32T ret_err = NCSCC_RC_FAILURE;
        SaUint32T cnt;

	TRACE_ENTER();

	if(NULL == grp_list || NULL == track_step)
		return NCSCC_RC_FAILURE;
	
	ent = (PLMS_ENTITY *)ncs_patricia_tree_get(&(cb->entity_info),
                           (SaUint8T *)&(track_step->dn_name));
        if (NULL == ent) {

                LOG_ER("plms_imm_adm_op_req_process:\
                        Ent not found for in patricia tree. dn_name: %s",track_step->dn_name.value);
                TRACE_LEAVE2("ret_err: %d",ret_err);
                return ret_err;
        }

	
	while(grp_list){
		 /* Populate the callback event to be sent to PLMA */
                memset(&agent_evt,0,sizeof(PLMS_EVT));
                agent_evt.req_res = PLMS_REQ;
                agent_evt.req_evt.req_type = PLMS_AGENT_TRACK_EVT_T;
                agent_evt.req_evt.agent_track.evt_type
                                        = PLMS_AGENT_TRACK_CBK_EVT;
                agent_evt.req_evt.agent_track.grp_handle =
                                grp_list->ent_grp_inf->entity_grp_hdl;
                agent_evt.req_evt.agent_track.track_cbk.track_cookie =
                                grp_list->ent_grp_inf->track_cookie;

                agent_evt.req_evt.agent_track.track_cbk.invocation_id =
                                                               inv_id;
                agent_evt.req_evt.agent_track.track_cbk.track_cause =
				track_step->opr_type;

                agent_evt.req_evt.agent_track.track_cbk.root_cause_entity
                	= track_step->dn_name;
                agent_evt.req_evt.agent_track.track_cbk.
                                root_correlation_id =
				track_step->root_correlation_id;

		if(SA_PLM_CHANGE_VALIDATE == track_step->change_step)
                	agent_evt.req_evt.agent_track.track_cbk.change_step =
						SA_PLM_CHANGE_ABORTED;
		else
                	agent_evt.req_evt.agent_track.track_cbk.change_step =
                				SA_PLM_CHANGE_COMPLETED;
                agent_evt.req_evt.agent_track.track_cbk.error = SA_AIS_OK;

		/* Add root as part of aff ent. */
		plms_ent_to_ent_list_add(ent,&aff_ent_list);

		/* Find no of affected entities.*/
                cnt = plms_no_aff_ent_in_grp_get(grp_list->ent_grp_inf,
                                        aff_ent_list);
                if (0 == cnt){
			 grp_list = grp_list->next;
			 continue;
                }
                agent_evt.req_evt.agent_track.track_cbk.
                tracked_entities.numberOfEntities = cnt;
                /* TODO: Remember to free.*/
                agent_evt.req_evt.agent_track.track_cbk.
                tracked_entities.entities =
                (SaPlmReadinessTrackedEntityT*)malloc
                        (cnt * sizeof(SaPlmReadinessTrackedEntityT));
		if(NULL == agent_evt.req_evt.agent_track.track_cbk.tracked_entities.entities){
                        LOG_CR("cbk_call, calloc failed. Error: %s",strerror(errno));
                        assert(0);
                }
                /* Pack the affected entities.*/
                plms_grp_aff_ent_fill(agent_evt.req_evt.agent_track.
                        track_cbk.tracked_entities.entities,
                        grp_list->ent_grp_inf,aff_ent_list,FALSE,
			agent_evt.req_evt.agent_track.track_cbk.change_step);

                /* Send the message to PLMA. */
		ret_err = plms_mds_normal_send(cb->mds_hdl,
				NCSMDS_SVC_ID_PLMS,
				(void *)(&agent_evt),
				grp_list->ent_grp_inf->agent_mdest_id,
				NCSMDS_SVC_ID_PLMA);
		if (NCSCC_RC_SUCCESS != ret_err){
			LOG_ER("Failed to send callback to PLMA");
                }
                /* MDS should have copied the data and hence free
                agent_evt.req_evt.agent_track.track_cbk.
                tracked_entities.entities*/
                free(agent_evt.req_evt.agent_track.track_cbk.
                                        tracked_entities.entities);
                agent_evt.req_evt.agent_track.track_cbk.
                                tracked_entities.entities = NULL;

                grp_list = grp_list->next;
        }

	TRACE_LEAVE();
	
	return ret_err;
}
/***********************************************************************
 * Name          :plms_perform_pending_admin_shutdown
 *
 *
 * Description   :  
 *                        
 * Arguments     : 
 *                        
 * Return Values : 
 *
***********************************************************************/
SaUint32T plms_perform_pending_admin_shutdown(
				PLMS_CKPT_TRACK_STEP_INFO  *track_step,
				PLMS_ENTITY   *ent)
{
        uns32 ret_err = NCSCC_RC_FAILURE;
        PLMS_GROUP_ENTITY *aff_ent_list = NULL, *head = NULL;
	PLMS_ENTITY_GROUP_INFO_LIST *group_info_list = NULL;

	TRACE_ENTER2("Entity:%s",ent->dn_name_str);

	/* Get all the affected entities.*/
	plms_affected_ent_list_get(ent,&aff_ent_list,0);

	/* Add the groups, root entity(ent) belong to.*/
        plms_ent_grp_list_add(ent,&group_info_list);

        /* Find out all the groups, all affected entities belong to and add
        the groups to trk_info->group_info_list.
        TODO: We have to free trk_info->group_info_list. */
        plms_ent_list_grp_list_add(aff_ent_list,
                                &group_info_list);

	/* Update the admin state& update the IMM databe */
	plms_admin_state_set(ent,SA_PLM_HE_ADMIN_LOCKED,NULL,
		SA_NTF_MANAGEMENT_OPERATION,SA_PLM_NTFID_STATE_CHANGE_ROOT);

	plms_readiness_state_set(ent,
			 SA_PLM_READINESS_OUT_OF_SERVICE,NULL,
			 SA_NTF_MANAGEMENT_OPERATION,
			 SA_PLM_NTFID_STATE_CHANGE_ROOT);


	/* Mark readiness state of all the affected ent to OOS and set
	the dependency flag.*/
	head = aff_ent_list;
	while (head){
		plms_readiness_state_set(head->plm_entity,
			SA_PLM_READINESS_OUT_OF_SERVICE,
			NULL,SA_NTF_MANAGEMENT_OPERATION,
			SA_PLM_NTFID_STATE_CHANGE_DEP);
		head = head->next;
	}
	head = aff_ent_list;
	while (head){
		plms_readiness_flag_mark_unmark(head->plm_entity,
		                                SA_PLM_RF_DEPENDENCY,TRUE,
						head->plm_entity->parent,
						SA_NTF_OBJECT_OPERATION,
						SA_PLM_NTFID_STATE_CHANGE_ROOT);
		head = head->next;
	}
	

	/* Terminate all the affected EEs.*/
	head = aff_ent_list;
	while(head){
		ret_err = plms_ee_term(head->plm_entity,FALSE,0);
		head = head->next;
	}

	if (PLMS_EE_ENTITY == ent->entity_type){
		/* Lock the root EE. I am not wating, till I get
		the callback. If this API returns success, I am done.
		*/
		ret_err = plms_ee_lock(ent,TRUE,0);
		if (NCSCC_RC_SUCCESS != ret_err){
			/* TODO:Do I need to update the expected
			readiness state, I hope no. */
		}
	}

	/* Mark the expected readiness state of the root entity.*/
	ent->exp_readiness_status.readinessState = SA_PLM_READINESS_OUT_OF_SERVICE;
	
	if (PLMS_HE_ENTITY == ent->entity_type){
		ent->exp_readiness_status.readinessFlags = ent->entity.he_entity.saPlmHEReadinessFlags;
	}else{
		ent->exp_readiness_status.readinessFlags = ent->entity.ee_entity.saPlmEEReadinessFlags;
	}

	plms_perform_pending_admin_clbk(group_info_list, aff_ent_list,track_step); 

	/* Clear the expected readines state */
	plms_ent_exp_rdness_status_clear(ent);
	plms_aff_ent_exp_rdness_status_clear(aff_ent_list);

	plms_ent_list_free(aff_ent_list);
	plms_ent_grp_list_free(group_info_list);

	TRACE_LEAVE2("ret_err: %d",ret_err);
	return NCSCC_RC_SUCCESS;
}

/***********************************************************************
 * Name          :plms_perform_pending_admin_lock
 *
 *
 * Description   :  
 *                        
 * Arguments     : 
 *                        
 * Return Values : 
 *
***********************************************************************/
SaUint32T plms_perform_pending_admin_lock(
				PLMS_CKPT_TRACK_STEP_INFO  *track_step,
				PLMS_ENTITY   *ent)
{
	SaUint32T ret_err = NCSCC_RC_FAILURE;
        PLMS_GROUP_ENTITY *aff_ent_list = NULL,*log_head;
        PLMS_GROUP_ENTITY *tmp_aff_ent_list = NULL;
	PLMS_ENTITY_GROUP_INFO_LIST *group_info_list = NULL;
	SaUint32T 	  admin_state;

	TRACE_ENTER2("Entity:%s",ent->dn_name_str);
#if 0
        /* Check if already in Locked state */
	if ((PLMS_HE_ENTITY == ent->entity_type &&
		SA_PLM_HE_ADMIN_LOCKED == 
		ent->entity.he_entity.saPlmHEAdminState) ||
		(PLMS_EE_ENTITY == ent->entity_type &&
		SA_PLM_EE_ADMIN_LOCKED ==
		ent->entity.ee_entity.saPlmEEAdminState)){
		LOG_ER("Entity:%s is already in locked state",
		ent->dn_name_str);
                return NCSCC_RC_FAILURE;
	}

	if(PLMS_HE_ENTITY == ent->entity_type)
		admin_state = SA_PLM_HE_ADMIN_LOCKED;
	if(PLMS_EE_ENTITY == ent->entity_type)
		admin_state = SA_PLM_EE_ADMIN_LOCKED;

	/* If I am OOS, then I am the only affected entity.*/
        if ((SA_PLM_READINESS_OUT_OF_SERVICE ==
        	ent->entity.he_entity.saPlmHEReadinessState)){

		/* Process the lock operation */
                plms_admin_state_set(ent,admin_state,NULL,
		SA_NTF_MANAGEMENT_OPERATION,SA_PLM_NTFID_STATE_CHANGE_ROOT);

		TRACE_LEAVE2("ret_err: %d",NCSCC_RC_SUCCESS);
                return NCSCC_RC_SUCCESS;
	}
#endif
	if(PLMS_HE_ENTITY == ent->entity_type)
		admin_state = SA_PLM_HE_ADMIN_LOCKED;
	if(PLMS_EE_ENTITY == ent->entity_type)
		admin_state = SA_PLM_EE_ADMIN_LOCKED;

	/* Get all the affected entities.*/
	plms_affected_ent_list_get(ent,&aff_ent_list,0);
	TRACE("Affected(to be OOS) entities for ent %s: ",
					ent->dn_name_str);
	log_head = aff_ent_list;
	while(log_head){
	TRACE("%s,",log_head->plm_entity->dn_name_str);
		log_head = log_head->next;
	}

	plms_ent_grp_list_add(ent,&group_info_list);

        /* Find out all the groups, all affected entities belong to*/
        plms_ent_list_grp_list_add(aff_ent_list,
                                        &group_info_list);


	if(SA_PLM_CHANGE_START == track_step->change_step){

		/* Update the admin state */
		plms_admin_state_set(ent,admin_state,NULL,
		SA_NTF_MANAGEMENT_OPERATION,SA_PLM_NTFID_STATE_CHANGE_ROOT);

		plms_readiness_state_set(ent,
			 SA_PLM_READINESS_OUT_OF_SERVICE,NULL,
			 SA_NTF_MANAGEMENT_OPERATION,
			 SA_PLM_NTFID_STATE_CHANGE_ROOT);

		/*Mark the readiness state of all the affected entities to
		 OOS */
		PLMS_GROUP_ENTITY *head = aff_ent_list;
		while (head){
			plms_readiness_state_set(head->plm_entity,
			SA_PLM_READINESS_OUT_OF_SERVICE,
			NULL,SA_NTF_MANAGEMENT_OPERATION,
			SA_PLM_NTFID_STATE_CHANGE_DEP);
			head = head->next;
		}
		
		/* Set the dependency flag for the affected entities */ 
		head = aff_ent_list;
		while (head){
			plms_readiness_flag_mark_unmark(head->plm_entity,
				SA_PLM_RF_DEPENDENCY,TRUE,
				head->plm_entity->parent,
				SA_NTF_OBJECT_OPERATION,
				SA_PLM_NTFID_STATE_CHANGE_ROOT);

			head = head->next;
		}

		/* Terminate all the affected EEs.*/
		tmp_aff_ent_list = aff_ent_list;
		while(tmp_aff_ent_list){
			if(tmp_aff_ent_list->plm_entity){
				if(PLMS_EE_ENTITY == 
				tmp_aff_ent_list->plm_entity->entity_type){
					ret_err = plms_ee_term(
						tmp_aff_ent_list->plm_entity,
								FALSE,0);
					if(NCSCC_RC_SUCCESS != ret_err){
						LOG_ER("EE %s term FAILED.",
				tmp_aff_ent_list->plm_entity->dn_name_str);
					}
				}
			}
			tmp_aff_ent_list = tmp_aff_ent_list->next;
		}

		if (PLMS_EE_ENTITY == ent->entity_type){
			/* Lock the root EE*/
			ret_err = plms_ee_lock(ent,TRUE,0);
			if(NCSCC_RC_SUCCESS != ret_err){
				LOG_ER("EE %s term FAILED.",ent->dn_name_str);
			}
		}
	}
	plms_perform_pending_admin_clbk(group_info_list, aff_ent_list, 
							track_step);

	plms_ent_list_free(aff_ent_list);
	plms_ent_grp_list_free(group_info_list);
	
	TRACE_LEAVE2("ret_err: %d",ret_err);
        return NCSCC_RC_SUCCESS;
}
/***********************************************************************
 * Name          :plms_perform_pending_admin_deactivate
 *
 *
 * Description   :  
 *                        
 * Arguments     : 
 *                        
 * Return Values : 
 *
***********************************************************************/
SaUint32T plms_perform_pending_admin_deactivate(
				PLMS_CKPT_TRACK_STEP_INFO  *track_step,
				PLMS_ENTITY   *ent)
{
        uns32 ret_err;
        PLMS_GROUP_ENTITY *aff_ent_list = NULL, *head;
        PLMS_GROUP_ENTITY *temp_ent_list;
	PLMS_ENTITY_GROUP_INFO_LIST *group_info_list = NULL;

	/* This should be performed only on HEs which are in
	Locked admin state */
	/* If it is already in locked_inactive state, return */

	/* Get all the affected entities.*/
	plms_affected_ent_list_get(ent,&aff_ent_list,0);

	plms_ent_grp_list_add(ent,&group_info_list);

        /* Find out all the groups, all affected entities belong to
	   add the groups to trk_info->group_info_list.
           TODO: We have to free trk_info->group_info_list. */
        plms_ent_list_grp_list_add(aff_ent_list,
                                        &group_info_list);


	if(SA_PLM_CHANGE_START == track_step->change_step ){
		/* Uninstantiate all the EEs */
                temp_ent_list = aff_ent_list;
                head = temp_ent_list;
                while(head){
                        if ((PLMS_EE_ENTITY == head->plm_entity->entity_type) &&
                                (SA_PLM_EE_PRESENCE_UNINSTANTIATED !=
                                head->plm_entity->entity.ee_entity.
                                saPlmEEPresenceState)){
                                        ret_err =plms_ee_term(head->plm_entity,
                                                                FALSE,0);
                                        if (NCSCC_RC_SUCCESS != ret_err){
                                        }
					plms_readiness_state_set(
                                        head->plm_entity,
                                        SA_PLM_READINESS_OUT_OF_SERVICE,ent,
                                        SA_NTF_MANAGEMENT_OPERATION,
                                        SA_PLM_NTFID_STATE_CHANGE_DEP);

                                        plms_ent_to_ent_list_add(
                                        	head->plm_entity,&aff_ent_list);
                                }
                }
                /* Set the HE state to M1.*/
                ret_err = plms_he_deactivate(ent,0,0);
                if (NCSCC_RC_SUCCESS != ret_err){
                        /* TODO: Should I do whatever I do on getting M1.???*/
                }

	}else{
		if(SA_PLM_CHANGE_VALIDATE == track_step->change_step ){

			/* Move HE to M4 */
			ret_err = plms_ent_enable(ent,FALSE,SA_TRUE);
			if (NCSCC_RC_SUCCESS != ret_err){
				LOG_ER(" Move the ent to M4 failed.");
			}

		}
	}
	plms_perform_pending_admin_clbk(group_info_list, aff_ent_list,
							track_step);

	plms_ent_list_free(aff_ent_list);

        plms_ent_grp_list_free(group_info_list);
	return NCSCC_RC_SUCCESS;
}
