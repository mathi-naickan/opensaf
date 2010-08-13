#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

#include <configmake.h>
#include "ncssysf_def.h"

#include "plms.h"


/***********************************************************************
 * Name          :  plms_cb_dump_routine_process
 *
 *
 * Description   :  This function is to dump PLMS control block
 *
 * Arguments     :  plms_cb
 *
 * Return Values :
 *
***********************************************************************/

void
plms_cb_dump_routine ()
{
    	time_t          tod;
	char            asc_tod[70],tmp_file[100];
	FILE            *fp;
	PLMS_ENTITY 	*ent;
	PLMS_GROUP_ENTITY *ent_group;
	PLMS_GROUP_ENTITY_ROOT_LIST *ent_group_root;
	PLMS_ENTITY_GROUP_INFO_LIST *group_list;
	PLMS_HE_BASE_INFO *he_base_info;
	PLMS_HE_TYPE_INFO *he_type_info;
	PLMS_EE_BASE_INFO *ee_base_info;
	PLMS_EE_TYPE_INFO *ee_type_info;
	PLMS_DEPENDENCY *plm_dep;
	PLMS_EPATH_TO_ENTITY_MAP_INFO *epath_emap;
	PLMS_CLIENT_INFO *client_info;
	PLMS_ENTITY_GROUP_INFO *group_info;
	PLMS_INVOCATION_TO_TRACK_INFO *track_info;
	char tmp[SA_MAX_NAME_LENGTH +1]="",count;
	SaUint8T max = 0;
	PLMS_CB *cb = plms_cb;

	ee_base_info = (PLMS_EE_BASE_INFO *)ncs_patricia_tree_getnext(&cb->base_ee_info,NULL);
	he_base_info = (PLMS_HE_BASE_INFO *)ncs_patricia_tree_getnext(&cb->base_he_info,NULL);
	ent = (PLMS_ENTITY *)ncs_patricia_tree_getnext(&cb->entity_info,NULL);

	m_GET_ASCII_DATE_TIME_STAMP(tod, asc_tod);
	
	strcpy(tmp_file, PKGLOGDIR "/plms_dump.out");
	strcat(tmp_file, asc_tod);

	fp = fopen(tmp_file, "w");
	if (fp == NULL)
	return;
	fprintf(fp,"\t\t\t----------------------- GOOD LUCK --------------------------------------\n");
	fprintf(fp,"\n\n=================== General INFO and FLAGS =============================");
	if (cb->ha_state == SA_AMF_HA_ACTIVE)
		fprintf(fp,"\n HA STATE: SA_AMF_HA_ACTIVE.");
	else if (cb->ha_state == SA_AMF_HA_STANDBY)
		fprintf(fp,"\n HA STATE: SA_AMF_HA_STANDBY.");
	else if (cb->ha_state == SA_AMF_HA_QUIESCED)
		fprintf(fp,"\n HA STATE: SA_AMF_HA_QUIESCED.");
	else if (cb->ha_state == SA_AMF_HA_QUIESCING)
		fprintf(fp,"\n HA STATE: SA_AMF_HA_QUIESCING.");
		else
			fprintf(fp,"\n HA STATE: Invalid.");

	fprintf(fp,"\n CSI Assigned: %u.", (unsigned int )cb->csi_assigned);
	fprintf(fp,"\n PLMS Shelf ID: %u.",(unsigned int )cb->plms_self_id);	
	fprintf(fp,"\n PLMS Remote ID: %u.",(unsigned int )cb->plms_remote_id);
	fprintf(fp,"\n MDS Handle: %u.", (unsigned int )cb->mds_hdl);
	fprintf(fp,"\n PLMS Remote ID: %llu.",(unsigned long long)cb->mdest_id);
	fprintf(fp,"\n PLMS HRB ID: %llu.", (unsigned long long)cb->hrb_dest);
	fprintf(fp,"\n PLMS Node ID: %u.", (unsigned int)cb->node_id);
	
	/* ALL Handle */
	fprintf(fp,"\n AMF handle: %llu.", (unsigned long long)cb->amf_hdl);
	fprintf(fp,"\n AMF Invocation ID: %llu.", (unsigned long long)cb->amf_invocation_id);
	fprintf(fp,"\n is_quisced_set : %d.", cb->is_quisced_set);
	fprintf(fp,"\n Imm handle: %llu.", (unsigned long long)cb->imm_hdl);
	fprintf(fp,"\n ImmOi handle: %llu.", (unsigned long long)cb->oi_hdl);
	fprintf(fp,"\n Ntf handle: %llu.",(unsigned long long) cb->ntf_hdl);
	fprintf(fp,"\n MBCSV handle: %u.",(unsigned int)cb->mbcsv_hdl);
	fprintf(fp,"\n MBCSV_CKPT handle: %u.", (unsigned int)cb->mbcsv_ckpt_hdl);
	fprintf(fp,"\n Health Check Started: %u.", (unsigned int)cb->healthCheckStarted);
	fprintf(fp,"\n EDU handle: %u.",(unsigned int) cb->edu_hdl.is_inited);
	
	/* Current MDS Role */
	if (cb->mds_role == V_DEST_RL_QUIESCED)
		fprintf(fp,"\n Current MDS role: V_DEST_RL_QUIESCED");
	else if (cb->mds_role == V_DEST_RL_ACTIVE)
		fprintf(fp,"\n Current MDS role: V_DEST_RL_ACTIVE");
	else if (cb->mds_role == V_DEST_RL_STANDBY)
		fprintf(fp,"\n Current MDS role: V_DEST_RL_STANDBY");
	else if (cb->mds_role == V_DEST_RL_INVALID)
		fprintf(fp,"\n Current MDS role: V_DEST_RL_INVALID");
		else
			fprintf(fp,"\n Current MDS role: Invalid");

	if (cb->hpi_cfg.safHpiCfg)
		fprintf(fp,"\n safHpiCfg :%s",cb->hpi_cfg.safHpiCfg);
	else
		fprintf(fp,"\n safHpiCfg :NULL");
	if (cb->is_initialized) 
		fprintf(fp,"\n PLM_CB is initialized");
	else
		fprintf(fp,"\n PLM_CB is not initialized");
	
	fprintf(fp,"\n Async_update_cnt: %d", cb->async_update_cnt);

	
	
	fprintf(fp,"\n\n=================== Entity INFO ========================================");
	count=1;
        while (ent) {
			fprintf(fp,"\n\nENTITY[%d]:\n",count);
			if(ent->dn_name_str)
				fprintf(fp,"\n Domain Name:%s",ent->dn_name_str);
			else
				fprintf(fp,"\n\n DN name of the Entity[%d] is not available",count); 
			if (ent->entity_type == PLMS_HE_ENTITY) 
				fprintf(fp,"\n Entity-Type: PLMS_HE_ENTITY");
			else if (ent->entity_type == PLMS_EE_ENTITY) 
				fprintf(fp,"\n Entity-Type: PLMS_EE_ENTITY");
			     else
				fprintf(fp,"\n Entity-Type: NULL");
			fprintf(fp,"\n Number of configured entity path:%d",ent->num_cfg_ent_paths);

			if (ent->exp_readiness_status.readinessState == SA_PLM_READINESS_OUT_OF_SERVICE)
				fprintf(fp,"\n Expected Readiness state:SA_PLM_READINESS_OUT_OF_SERVICE");
			else if (ent->exp_readiness_status.readinessState == SA_PLM_READINESS_IN_SERVICE)
				fprintf(fp,"\n Expected Readiness state:SA_PLM_READINESS_IN_SERVICE");
			else if (ent->exp_readiness_status.readinessState == SA_PLM_READINESS_STOPPING)
				fprintf(fp,"\n Expected Readiness state:SA_PLM_READINESS_STOPPING");
			     else 
				fprintf(fp,"\n Expected Readiness state:Not match");
				     
			fprintf(fp,"\n Expected Readiness Flag:%llu",(unsigned long long)ent->exp_readiness_status.readinessFlags);
			
			fprintf(fp,"\n DN Name of the entity groups on which this entity belongs to");
			group_list = (PLMS_ENTITY_GROUP_INFO_LIST *)ent->part_of_entity_groups;
			if (!group_list)
				fprintf(fp,":NULL");
			else {	
				while (group_list)	{
					fprintf(fp,":%llu",(unsigned long long)group_list->ent_grp_inf->entity_grp_hdl);
					group_list = group_list->next;
				}
			}
			if (ent->invoke_callback == SA_FALSE)
				fprintf(fp,"\n Invoke Callback: SA_FALSE");
			else if (ent->invoke_callback == SA_TRUE)
				fprintf(fp,"\n Invoke Callback: SA_TRUE");
			else
				fprintf(fp,"\n Invoke Callback: NULL");
			fprintf(fp,"\n First PLM notify id:%llu",(unsigned long long)ent->first_plm_ntf_id);
			fprintf(fp,"\n PLMS Timer id: %lx", (long)ent->tmr.timer_id);
			if (ent->tmr.tmr_type == PLMS_TMR_NONE)
				fprintf(fp,"\n PLMS Timer type:PLMS_TMR_NONE");
			else if (ent->tmr.tmr_type == PLMS_TMR_EE_INSTANTIATING)
				fprintf(fp,"\n PLMS Timer type:PLMS_TMR_EE_INSTANTIATING");
			else if (ent->tmr.tmr_type == PLMS_TMR_EE_TERMINATING)
				fprintf(fp,"\n PLMS Timer type:PLMS_TMR_EE_TERMINATING");
			     else 	
				fprintf(fp,"\n PLMS Timer type:NULL");
			
			if (ent->state_model == PLMS_HPI_TWO_HOTSWAP_MODEL)
				fprintf(fp,"\n State model: PLMS_HPI_TWO_HOTSWAP_MODEL");
			else if (ent->state_model == PLMS_HPI_THREE_HOTSWAP_MODEL)
				fprintf(fp,"\n State model: PLMS_HPI_THREE_HOTSWAP_MODEL");
			else if (ent->state_model == PLMS_HPI_PARTIAL_FIVE_HOTSWAP_MODEL)
				fprintf(fp,"\n State model: PLMS_HPI_PARTIAL_FIVE_HOTSWAP_MODEL");
			else if (ent->state_model == PLMS_HPI_FULL_FIVE_HOTSWAP_MODEL)
				fprintf(fp,"\n State model: PLMS_HPI_FULL_FIVE_HOTSWAP_MODEL");
			     else 	
				fprintf(fp,"\n State model: NULL");
			fprintf(fp,"\n Socket ID:%u",(unsigned int)ent->plmc_addr);
			fprintf(fp,"\n adm_op_in_progress:%u",(unsigned int)ent->adm_op_in_progress);
			fprintf(fp,"\n Invocation :%llu",(unsigned long long)ent->adm_op_in_progress);
			fprintf(fp,"\n am_i_aff_ent:%u",(unsigned int)ent->am_i_aff_ent);
				
			/*Dependency List */
			fprintf(fp,"\n Minimum number of dependencies :%u",(unsigned int)ent->min_no_dep);
			fprintf(fp,"\n Dependency List");
			if ( ent->min_no_dep < 1 ) 
				fprintf(fp,":NULL");
			else {	
				ent_group = ent->dependency_list;
		 		while (ent_group) {
					if (ent_group->plm_entity->dn_name_str)
						fprintf(fp,":%s",ent_group->plm_entity->dn_name_str);
					else	
						fprintf(fp,":NULL");
					ent_group = ent_group->next;
				}
			}		
			fprintf(fp,"\n Reverse dependency list");
			ent_group = ent->rev_dep_list;
			if(!ent_group)
				fprintf(fp,":NULL");
			else {	
				while (ent_group) {
					if (ent_group->plm_entity->dn_name_str)
						fprintf(fp,":%s",ent_group->plm_entity->dn_name_str);
					else
						fprintf(fp,":DN name is NULL");
					ent_group = ent_group->next;
				}
			}		
			if (ent->iso_method == PLMS_ISO_DEFAULT)	
				fprintf(fp,"\n Isolation method:PLMS_ISO_DEFAULT");
			else if (ent->iso_method == PLMS_ISO_EE_TERMINATED) 		
				fprintf(fp,"\n Isolation method:PLMS_ISO_EE_TERMINATED");
			else if (ent->iso_method == PLMS_ISO_HE_DEACTIVATED)
				fprintf(fp,"\n Isolation method:PLMS_ISO_HE_DEACTIVATED");
			else if (ent->iso_method == PLMS_ISO_HE_RESET_ASSERT)
				fprintf(fp,"\n Isolation method:PLMS_ISO_HE_RESET_ASSERT");
			else
				fprintf(fp,"\n Isolation method:Invalid");

			fprintf(fp,"\n Management lost trigger:%u",(unsigned int)ent->mngt_lost_tri);
			if (ent->parent) {
			   if (ent->parent->dn_name_str)
				fprintf(fp,"\n DN Name of the Parent entity:%s",ent->parent->dn_name_str);
			   else	
				fprintf(fp,"\n DN Name of the Parent entity:NULL");
			} else	
				fprintf(fp,"\n Parent entity:NULL");
			if (ent->leftmost_child) {
			   if (ent->leftmost_child->dn_name_str)
				fprintf(fp,"\n DN Name of the Left most child entity:%s",ent->leftmost_child->dn_name_str);
			   else	
				fprintf(fp,"\n DN Name of the Left most child entity:NULL");
			} else	
				fprintf(fp,"\n Left most child entity:NULL");
			if (ent->right_sibling) {
			   if (ent->right_sibling->dn_name_str)
				fprintf(fp,"\n DN Name of the right sibling entity:%s",ent->right_sibling->dn_name_str);
			   else	
				fprintf(fp,"\n DN Name of the right sibling entity:NULL");
			} else	
				fprintf(fp,"\n Right sibling entity:NULL");
			fprintf(fp,"\n PLMS Track info:");
			if (ent->trk_info)	{
				fprintf(fp,"\n\t Imm admin operation id:%llu",(unsigned long long)ent->trk_info->imm_adm_opr_id);
				if (ent->trk_info->root_entity) {
			   	   if (ent->trk_info->root_entity->dn_name_str)
					fprintf(fp,"\n\t PLMS root entity dn name:%s",ent->trk_info->root_entity->dn_name_str);
				   else	
					fprintf(fp,"\n\t PLMS root entity dn name:NULL");
				} else	
					fprintf(fp,"\n\t PLMS root entity :NULL");
				fprintf(fp,"\n\t Track count:%u",(unsigned int)ent->trk_info->track_count);
				if ( ent->trk_info->change_step == SA_PLM_CHANGE_VALIDATE)
					fprintf(fp,"\n\t Change step: SA_PLM_CHANGE_VALIDATE");
				else if ( ent->trk_info->change_step == SA_PLM_CHANGE_START)
					fprintf(fp,"\n\t Change step: SA_PLM_CHANGE_START");
				else if ( ent->trk_info->change_step == SA_PLM_CHANGE_ABORTED)
					fprintf(fp,"\n\t Change step: SA_PLM_CHANGE_ABORTED");
				else if ( ent->trk_info->change_step == SA_PLM_CHANGE_COMPLETED)
					fprintf(fp,"\n\t Track info change step: SA_PLM_CHANGE_COMPLETED");
					else
						fprintf(fp,"\n\t Track info Change step: Invalid");
			
				fprintf(fp,"\n\t Track cause:%u",(unsigned int)ent->trk_info->track_cause);
				fprintf(fp,"\n\t Root correlation id:%llu",(unsigned long long)ent->trk_info->root_correlation_id);
			} else 
				fprintf(fp,"NULL");
			

			/* PLM HE entity */
			if (ent->entity_type == PLMS_HE_ENTITY)	{
				fprintf(fp,"\n\n----- HE Entity-----");
				if (ent->entity.he_entity.safHE)
					fprintf(fp,"\n safHE:%s",ent->entity.he_entity.safHE);
				else	
					fprintf(fp,"\n safHE:NULL");
				if (ent->entity.he_entity.saPlmHEBaseHEType.length)	
					fprintf(fp,"\n saPlmHEBaseHEType:%s",ent->entity.he_entity.saPlmHEBaseHEType.value);
				else 
					fprintf(fp,"\n safHEBaseHEType:NULL");
					
				for (max=0; max < SAHPI_MAX_ENTITY_PATH; max++){
					if (NULL == ent->entity.he_entity.saPlmHEEntityPaths[max]){
						fprintf(fp,"\n saPlmHEEntityPaths:NULL");
						break;
					}else {
						fprintf(fp,"\n saPlmHEEntityPaths:%s",
						(ent->entity.he_entity.saPlmHEEntityPaths[max]));
					}
				}
				
				if (ent->entity.he_entity.saPlmHECurrHEType.length)	
					fprintf(fp,"\n saPlmHECurrHEType:%s",ent->entity.he_entity.saPlmHECurrHEType.value);
				else 
					fprintf(fp,"\n saPlmHECurrHEType:NULL");
				if (ent->entity.he_entity.saPlmHECurrEntityPath)	
					fprintf(fp,"\n saPlmHECurrEntityPath:%s",ent->entity.he_entity.saPlmHECurrEntityPath);
				else
					fprintf(fp,"\n saPlmHECurrEntityPath:NULL");
				if (ent->entity.he_entity.saPlmHEAdminState == SA_PLM_HE_ADMIN_UNLOCKED)
					fprintf(fp,"\n HE Admin state:SA_PLM_HE_ADMIN_UNLOCKED");
				else if (ent->entity.he_entity.saPlmHEAdminState == SA_PLM_HE_ADMIN_LOCKED)	
					fprintf(fp,"\n HE Admin state:SA_PLM_HE_ADMIN_LOCKED");
				else if (ent->entity.he_entity.saPlmHEAdminState == SA_PLM_HE_ADMIN_LOCKED_INACTIVE)
					fprintf(fp,"\n HE Admin state:SA_PLM_HE_ADMIN_LOCKED_INACTIVE");
				else if (ent->entity.he_entity.saPlmHEAdminState == SA_PLM_HE_ADMIN_SHUTTING_DOWN)	
						fprintf(fp,"\n HE Admin state:SA_PLM_HE_ADMIN_SHUTTING_DOWN");
				     else 
						fprintf(fp,"\n HE Admin state:Not match");
				if (ent->entity.he_entity.saPlmHEReadinessState == SA_PLM_READINESS_OUT_OF_SERVICE)
					fprintf(fp,"\n HE Readiness state:SA_PLM_READINESS_OUT_OF_SERVICE");
				else if (ent->entity.he_entity.saPlmHEReadinessState == SA_PLM_READINESS_IN_SERVICE)
					fprintf(fp,"\n HE Readiness state:SA_PLM_READINESS_IN_SERVICE");
				else if (ent->entity.he_entity.saPlmHEReadinessState == SA_PLM_READINESS_STOPPING)
					fprintf(fp,"\n HE Readiness state:SA_PLM_READINESS_STOPPING");
					else		
						fprintf(fp,"\n HE Readiness state:Not match");
				fprintf(fp,"\n HE Readiness flags:%llu",(unsigned long long)ent->entity.he_entity.saPlmHEReadinessFlags);
				if (ent->entity.he_entity.saPlmHEPresenceState	== SA_PLM_HE_PRESENCE_NOT_PRESENT)
					fprintf(fp,"\n HE Presence state:SA_PLM_HE_PRESENCE_NOT_PRESENT");
				else if (ent->entity.he_entity.saPlmHEPresenceState == SA_PLM_HE_PRESENCE_INACTIVE)
					fprintf(fp,"\n HE Presence state:SA_PLM_HE_PRESENCE_INACTIVE");
				else if (ent->entity.he_entity.saPlmHEPresenceState == SA_PLM_HE_PRESENCE_ACTIVATING)
					fprintf(fp,"\n HE Presence state:SA_PLM_HE_PRESENCE_ACTIVATING");
				else if (ent->entity.he_entity.saPlmHEPresenceState == SA_PLM_HE_PRESENCE_ACTIVE)
					fprintf(fp,"\n HE Presence state:SA_PLM_HE_PRESENCE_ACTIVE");
				else if (ent->entity.he_entity.saPlmHEPresenceState == SA_PLM_HE_PRESENCE_DEACTIVATING)
					fprintf(fp,"\n HE Presence state:SA_PLM_HE_PRESENCE_DEACTIVATING");
					else
						fprintf(fp,"\n HE Presence state:Not match");
				if (ent->entity.he_entity.saPlmHEOperationalState == SA_PLM_OPERATIONAL_ENABLED)
					fprintf(fp,"\n HE Operation state:SA_PLM_OPERATIONAL_ENABLED");
				else if (ent->entity.he_entity.saPlmHEOperationalState == SA_PLM_OPERATIONAL_DISABLED)
					fprintf(fp,"\n HE Operation state:SA_PLM_OPERATIONAL_DISABLED");
				else
					fprintf(fp,"\n HE Operation state:Not match");
			} else if (ent->entity_type == PLMS_EE_ENTITY)	{	
				fprintf(fp,"\n\n\t----- EE Entity-----");
				if (ent->entity.ee_entity.safEE)
					fprintf(fp,"\n safEE:%s",ent->entity.ee_entity.safEE);
				else
					fprintf(fp,"\n safEE:NULL");
				if (ent->entity.ee_entity.saPlmEEType.length)	
					fprintf(fp,"\n saPlmEEType:%s",ent->entity.ee_entity.saPlmEEType.value);
				else
					fprintf(fp,"\n saPlmEEType:NULL");
                		fprintf(fp,"\n saPlmEEInstantiateTimeout:%lld",(long long int)ent->entity.ee_entity.saPlmEEInstantiateTimeout);
		                fprintf(fp,"\n saPlmEETerminateTimeout:%lld",(long long int)ent->entity.ee_entity.saPlmEETerminateTimeout);
				if (ent->entity.ee_entity.saPlmEEAdminState == SA_PLM_EE_ADMIN_UNLOCKED)
                			fprintf(fp,"\n EE Admin state:SA_PLM_EE_ADMIN_UNLOCKED");
				else if (ent->entity.ee_entity.saPlmEEAdminState == SA_PLM_EE_ADMIN_LOCKED)
                			fprintf(fp,"\n EE Admin state:SA_PLM_EE_ADMIN_LOCKED");
				else if (ent->entity.ee_entity.saPlmEEAdminState == SA_PLM_EE_ADMIN_LOCKED_INSTANTIATION)
                			fprintf(fp,"\n EE Admin state:SA_PLM_EE_ADMIN_LOCKED_INSTANTIATION");
				else if (ent->entity.ee_entity.saPlmEEAdminState == SA_PLM_EE_ADMIN_SHUTTING_DOWN)
                			fprintf(fp,"\n EE Admin state:SA_PLM_EE_ADMIN_SHUTTING_DOWN");
					else
	 		               		fprintf(fp,"\n EE Admin state:Not match");
				if (ent->entity.ee_entity.saPlmEEReadinessState == SA_PLM_READINESS_OUT_OF_SERVICE)
                			fprintf(fp,"\n EE Readiness state:SA_PLM_READINESS_OUT_OF_SERVICE");
				else if (ent->entity.ee_entity.saPlmEEReadinessState == SA_PLM_READINESS_IN_SERVICE)
                			fprintf(fp,"\n EE Readiness state:SA_PLM_READINESS_IN_SERVICE");
				else if (ent->entity.ee_entity.saPlmEEReadinessState == SA_PLM_READINESS_STOPPING)
                			fprintf(fp,"\n EE Readiness state:SA_PLM_READINESS_STOPPING");
					else
		                		fprintf(fp,"\n EE Readiness state:Not match");
				fprintf(fp,"\n EE Readiness flags:%llu",(unsigned long long)ent->entity.ee_entity.saPlmEEReadinessFlags);
				if (ent->entity.ee_entity.saPlmEEPresenceState == SA_PLM_EE_PRESENCE_UNINSTANTIATED)
					fprintf(fp,"\n EE Presence state:SA_PLM_EE_PRESENCE_UNINSTANTIATED");
				else if (ent->entity.ee_entity.saPlmEEPresenceState == SA_PLM_EE_PRESENCE_INSTANTIATING)
					fprintf(fp,"\n EE Presence state:SA_PLM_EE_PRESENCE_INSTANTIATING");
				else if (ent->entity.ee_entity.saPlmEEPresenceState == SA_PLM_EE_PRESENCE_INSTANTIATED)
					fprintf(fp,"\n EE Presence state:SA_PLM_EE_PRESENCE_INSTANTIATED");
				else if (ent->entity.ee_entity.saPlmEEPresenceState == SA_PLM_EE_PRESENCE_TERMINATING)
					fprintf(fp,"\n EE Presence state:SA_PLM_EE_PRESENCE_TERMINATING");
				else if (ent->entity.ee_entity.saPlmEEPresenceState == SA_PLM_EE_PRESENCE_INSTANTIATION_FAILED)
					fprintf(fp,"\n EE Presence state:SA_PLM_EE_PRESENCE_INSTANTIATION_FAILED");
				else if (ent->entity.ee_entity.saPlmEEPresenceState == SA_PLM_EE_PRESENCE_TERMINATION_FAILED)
					fprintf(fp,"\n EE Presence state:SA_PLM_EE_PRESENCE_TERMINATION_FAILED");
					else
						fprintf(fp,"\n EE Presence state:Not match");
				if (ent->entity.ee_entity.saPlmEEOperationalState == SA_PLM_OPERATIONAL_ENABLED)
					fprintf(fp,"\n EE Operation state:SA_PLM_OPERATIONAL_ENABLED");
				else if (ent->entity.ee_entity.saPlmEEOperationalState == SA_PLM_OPERATIONAL_DISABLED)
					fprintf(fp,"\n EE Operation state:SA_PLM_OPERATIONAL_DISABLED");
				else 
					fprintf(fp,"\n EE Operation state:Not match");
			} else
					fprintf(fp,"\n Entity type is invalid");
				
			count++;
			ent = (PLMS_ENTITY *)ncs_patricia_tree_getnext(
                                        &cb->entity_info,
                                        (SaUint8T *)&(ent->dn_name));			
		}	
				
	
	fprintf(fp,"\n\n=================== Base HE INFO =======================================");

	count = 1;
	while (he_base_info)
	{
		plms_get_str_from_dn_name(&(he_base_info->dn_name),tmp);
		fprintf(fp,"\n\n DN name of the base he[%d]:%s",count,tmp);
		if (he_base_info->he_base_type.safHEType)	
			fprintf(fp,"\n RDN name of he base type:%s",he_base_info->he_base_type.safHEType);
		else
			fprintf(fp,"\n RDN name of he base type:NULL");
		if (he_base_info->he_base_type.saPlmHetHpiEntityType)	
			fprintf(fp,"\n HE base type saPlmHetHpiHEEntityType:%s",he_base_info->he_base_type.saPlmHetHpiEntityType);
		else
			fprintf(fp,"\n HE base type saPlmHetHpiHEEntityType:NULL");
		he_type_info = he_base_info->he_type_info_list;
		if (!he_type_info) {
			fprintf(fp,"\n HE Type info list is not available");
		} else {	
			while (he_type_info) {
				if (he_type_info->he_type.safVersion)
					fprintf(fp,"\n HE type safversion:%s",he_type_info->he_type.safVersion);
				else
					fprintf(fp,"\n HE type safversion:NULL");
				if (he_type_info->he_type.saPlmHetIdr)	
					fprintf(fp,"\n HE type saPlmHetldr:%s",he_type_info->he_type.saPlmHetIdr);
				else
					fprintf(fp,"\n HE type saPlmHetldr:NULL");
				he_type_info = he_type_info->next;
			}	
		}
		count++;	
	 	he_base_info = (PLMS_HE_BASE_INFO *)ncs_patricia_tree_getnext(
                                        &cb->base_he_info,
                                        (SaUint8T *)&(he_base_info->dn_name));	
	}			
	


	fprintf(fp,"\n\n=================== Base EE INFO =======================================");

	
	count = 1;
	while (ee_base_info)
	{
		plms_get_str_from_dn_name(&(ee_base_info->dn_name),tmp);
		fprintf(fp,"\n\n DN name of the base ee[%d]:%s",count,tmp);
		if (ee_base_info->ee_base_type.safEEType)
			fprintf(fp,"\n RDN name of EE base type :%s",ee_base_info->ee_base_type.safEEType);
		else
			fprintf(fp,"\n RDN name of EE base type :NULL");
	 	if (ee_base_info->ee_base_type.saPlmEetProduct)		
			fprintf(fp,"\n EE base type saplmEetProduct:%s",ee_base_info->ee_base_type.saPlmEetProduct);
		else
			fprintf(fp,"\n EE base type saplmEetProduct :NULL");
		if (ee_base_info->ee_base_type.saPlmEetVendor)
			fprintf(fp,"\n EE base type saPlmEetVendor:%s",ee_base_info->ee_base_type.saPlmEetVendor);
		else
			fprintf(fp,"\n EE base type saPlmEetVendor:NULL");
		if (ee_base_info->ee_base_type.saPlmEetRelease)
			fprintf(fp,"\n EE base type saPlmEetRelease:%s",ee_base_info->ee_base_type.saPlmEetRelease);
		else
			fprintf(fp,"\n EE base type saPlmEetRelease:NULL");
		ee_type_info = ee_base_info->ee_type_info_list;
		if (!ee_type_info) {
			fprintf(fp,"\n EE Type info list is not available");
		} else {	
			while (ee_type_info) {
				if (ee_type_info->ee_type.safVersion)
					fprintf(fp,"\n EE type safversion:%s",ee_type_info->ee_type.safVersion);
				else
					fprintf(fp,"\n EE type safversion:NULL");
				fprintf(fp,"\n EE type saPlmEeDefaultInstantiateTimeout:%lld",(long long int)ee_type_info->ee_type.saPlmEeDefaultInstantiateTimeout);
				fprintf(fp,"\n EE type saPlmEeDefTerminateTimeout:%lld",(long long int)ee_type_info->ee_type.saPlmEeDefTerminateTimeout);
				ee_type_info = ee_type_info->next;
			}	
		}
		count++;	
	 	ee_base_info = (PLMS_EE_BASE_INFO *)ncs_patricia_tree_getnext(
                                        &cb->base_ee_info,
                                        (SaUint8T *)&(ee_base_info->dn_name));	
	}			
	

	fprintf(fp,"\n\n=================== Dependency List ====================================");
	
	count=1;
	plm_dep = cb->dependency_list;	
	if (!plm_dep) {
		fprintf(fp,"\n Dependeny list is not available");
	} else {	
		while (plm_dep)
		{
			plms_get_str_from_dn_name(&(plm_dep->dn_name),tmp);
			fprintf(fp,"\n\n DN Name of the dependency list[%d]:%s",count,tmp);
			fprintf(fp,"\n Number of dependencies:%d",plm_dep->no_of_dep);
			if (plm_dep->dependency.safDependency)
				fprintf(fp,"\n DN Name of its dependency:%s",plm_dep->dependency.safDependency);
			count++;
			plm_dep = plm_dep->next;	
		}	
	}


	fprintf(fp,"\n\n=================== Epath to Entity map info ===========================");
	count=1;
	epath_emap = (PLMS_EPATH_TO_ENTITY_MAP_INFO *)ncs_patricia_tree_getnext(&cb->epath_to_entity_map_info,NULL);
	if (!epath_emap) {
		fprintf(fp,"\n Epath to Entity map info is not available");
	} else {	
		while (epath_emap)
		{
			if (epath_emap->entity_path)
				fprintf(fp,"\n Entity path for epath_to_entity_map_info list [%d]:%s",count,epath_emap->entity_path);
			else
				fprintf(fp,"\n Entity path for epath_to_entity_map_info list [%d]:NULL",count);
			if (epath_emap->plms_entity->dn_name_str)
				fprintf(fp,"\n DN Name of PLM entity:%s",epath_emap->plms_entity->dn_name_str);
			else
				fprintf(fp,"\n DN Name of PLM entity:NULL");
			count++;	
		 	epath_emap = (PLMS_EPATH_TO_ENTITY_MAP_INFO *)ncs_patricia_tree_getnext(
											&cb->epath_to_entity_map_info,
											(SaUint8T *)&(epath_emap->entity_path));
		}
	}

	fprintf(fp,"\n\n======================== Client info ===================================");
	
	count=1;
	client_info = (PLMS_CLIENT_INFO *)ncs_patricia_tree_getnext(&cb->client_info,NULL);
	if (!client_info) {
		fprintf(fp,"\n Client info is not available");
	} else {	
		while (client_info)
		{
			fprintf(fp,"\n\n Client info list[%d]",count);
			fprintf(fp,"\n Client info handle:%llu",(unsigned long long)client_info->plm_handle);
			fprintf(fp,"\n Client info mds dest id:%llu",(unsigned long long)client_info->mdest_id);
			fprintf(fp,"\n Client info group list handle id:");
			group_list = (PLMS_ENTITY_GROUP_INFO_LIST *)client_info->entity_group_list;
		        while (group_list)  {
        			fprintf(fp,"\n \t%llu",(unsigned long long)group_list->ent_grp_inf->entity_grp_hdl);
        			group_list = group_list->next;
       			}
		count++;	
	 	client_info = (PLMS_CLIENT_INFO *)ncs_patricia_tree_getnext(
									&cb->client_info,
									(SaUint8T *)&(client_info->plm_handle));
		}
	}
	
	fprintf(fp,"\n\n=================== Entity group info ==================================");
	
	count=1;
	group_info = (PLMS_ENTITY_GROUP_INFO *)ncs_patricia_tree_getnext(&cb->entity_group_info,NULL);
	if (!group_info) {
		fprintf(fp,"\n Group info is not available");
	} else {	
		while (group_info)
		{
			fprintf(fp,"\n\n Entity group info: [%d]",count); 	
			fprintf(fp,"\n Entity group handle: %llu",(unsigned long long)group_info->entity_grp_hdl); 	
			fprintf(fp,"\n Agent mdest id: %llu",(unsigned long long)group_info->agent_mdest_id); 	
			fprintf(fp,"\n PLM entity list"); 	
			ent_group_root = group_info->plm_entity_list;
    		    	while (ent_group_root) {
				if (ent_group_root->plm_entity->dn_name_str)
			        	fprintf(fp,"\n \t%s",ent_group_root->plm_entity->dn_name_str);
				else
					fprintf(fp,"\n Dn name is NULL");
				if (ent_group_root->subtree_root->dn_name_str)	
			        	fprintf(fp,"\n \t%s",ent_group_root->subtree_root->dn_name_str);
				else
					fprintf(fp,"\n Subtree DN Name is NULL"); 
		        	ent_group_root = ent_group_root->next;
       			}
		if (group_info->options == SA_PLM_GROUP_SINGLE_ENTITY)
			fprintf(fp,"\n PLM Group info option:SA_PLM_GROUP_SINGLE_ENTITY");
		else if(group_info->options == SA_PLM_GROUP_SUBTREE) 
			fprintf(fp,"\n PLM Group info option:SA_PLM_GROUP_SUBTREE");
		else if(group_info->options == SA_PLM_GROUP_SUBTREE_HES_ONLY)
			fprintf(fp,"\n PLM Group info option:SA_PLM_GROUP_SUBTREE_HES_ONLY");
		else if(group_info->options == SA_PLM_GROUP_SUBTREE_EES_ONLY)
			fprintf(fp,"\n PLM Group info option:SA_PLM_GROUP_SUBTREE_EES_ONLY");
			else
				fprintf(fp,"\n PLM Group info option:Not match");
		
		fprintf(fp,"\n PLM Group info track flags:%x",group_info->track_flags);
		fprintf(fp,"\n PLM Group info track cookie:%llu",(unsigned long long)group_info->track_cookie);
		track_info = group_info->invocation_list;
		while (track_info)
		{
			fprintf(fp,"\n PLM Group info invocation:%llu",(unsigned long long)track_info->invocation);
			fprintf(fp,"\n PLM Group info track info imm_adm_opr_id:%llu",(unsigned long long)track_info->track_info->imm_adm_opr_id);
			track_info = track_info->next;
		}
		fprintf(fp,"\n PLM Group info invocation callback:%x",group_info->invoke_callback);
		count++;	
	 	client_info = (PLMS_CLIENT_INFO *)ncs_patricia_tree_getnext(
									&cb->client_info,
									(SaUint8T *)&(client_info->plm_handle));
		}
	}
	fclose(fp);
}
