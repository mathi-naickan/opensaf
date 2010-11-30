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
@file	: plms_utils.c

@brief	: PLMS utility functions.

@author	:  Emerson Network Power.
*****************************************************************************/

#include "plms.h"
#include "plms_notifications.h"
#include "plms_mbcsv.h"
#include "plms_utils.h"
#include "immutil.h"

/******************************************************************************
@brief		: If admin_op_in_progress is set for any the following then
                  return failure.
                  1. Any of the ancestors.
		  2. Any entity, I am dependent on.
                  3. Any of the affected entity.
		  4. Myself
		  
@param[in]	: ent - PLMS_ENTITY representing an EE or HE.
@param[in]	: aff_ent_list - List of affected entities.

@return		: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
******************************************************************************/
SaUint32T plms_anc_chld_dep_adm_flag_is_set( 
					PLMS_ENTITY *ent,
					PLMS_GROUP_ENTITY *aff_ent_list)
{
	PLMS_ENTITY *tmp_ent;
	PLMS_GROUP_ENTITY *tmp_ent_head;

	TRACE_ENTER2("Entity: %s",ent->dn_name_str);
	
	/* I should not be the affected entity of any other admin operation.*/
	if(ent->am_i_aff_ent){
		LOG_ER("Root entity %s is in other operational cnxt.",
					ent->dn_name_str);
		TRACE_LEAVE2("Return Val: %d",TRUE);
		return TRUE;
	}
		
	tmp_ent = ent;
	/* If admin context is set for any of the ancestor.*/
	while( NULL != tmp_ent->parent) {
		if (tmp_ent->parent->am_i_aff_ent){
			LOG_ER("Ancestor %s is in other operatinal cnxt",
						tmp_ent->parent->dn_name_str);
			TRACE_LEAVE2("Return Val: %d",TRUE);
			return TRUE;
		} else{
			tmp_ent = tmp_ent->parent;
		}
	}

	/* Entities, I am dependent on.*/
	tmp_ent_head = ent->dependency_list;
	while(tmp_ent_head){
		if (tmp_ent_head->plm_entity->am_i_aff_ent){
			LOG_ER("Dep %s is in other operational cnxt.",
					tmp_ent_head->plm_entity->dn_name_str);
			TRACE_LEAVE2("Return Val: %d",TRUE);
			return TRUE;
		}else{
			tmp_ent_head = tmp_ent_head->next;
		}
	}

	tmp_ent_head = aff_ent_list;
	/* Scan through the list to see if any of the affected ent is in
	   admin context.
	*/
	while(tmp_ent_head){
		if (tmp_ent_head->plm_entity->am_i_aff_ent){
			LOG_ER("Aff_ent %s is in other operational cnxt.",
					tmp_ent_head->plm_entity->dn_name_str);
			TRACE_LEAVE2("Return Val: %d",TRUE);
			return TRUE;
		}else{
			tmp_ent_head = tmp_ent_head->next;
		}
	}
	
	TRACE_LEAVE2("Return Val: %d",FALSE);
	return FALSE;
}

/******************************************************************************
@brief		: Find all the affected entities of the entity ent.
		  1. All the children and rev dep of all the children.
		  2. Scan through the rev dependency list recursively and all
		  the children of the ent part of rev_dep_list.
		  
		Notes: 
		1. root_ent is not part of the affected entity list.
		2. If the function is called with is_insvc as 1, then make sure
		the function is called only if the ent will move to insvc.
		Similarly if the function is called with is_insvc as 0, then make
		sure the function is called only if the ent will move to OOS.	

@param[in]	: root_ent - Root entity for which the affected entities will be
		calculated.
@param[out]	: ent_list - Head of the resulting affected entity list.
@param[in]	: is_insvc - If set to 1, then find the affeted entities as the
		root entity will be moved to insvc. If set to 0, then OOS.
******************************************************************************/
void plms_affected_ent_list_get( PLMS_ENTITY *root_ent,
				PLMS_GROUP_ENTITY **ent_list,
				SaBoolT is_insvc)

{
	PLMS_GROUP_ENTITY *chld_aff_fg_list = NULL,*chld_aff_list = NULL;
	PLMS_GROUP_ENTITY *dep_aff_fg_list = NULL,*dep_aff_list = NULL;
	PLMS_GROUP_ENTITY *tail,*log_head;

	plms_aff_chld_ent_list_get(root_ent,root_ent->leftmost_child,
				&chld_aff_list,&chld_aff_fg_list,is_insvc);
	plms_aff_dep_ent_list_get(root_ent,root_ent->rev_dep_list,
				&dep_aff_list,&dep_aff_fg_list,is_insvc);


	if ( NULL != chld_aff_list){
		
		tail = chld_aff_list;
		while(tail){
			plms_ent_to_ent_list_add(tail->plm_entity,ent_list);
			tail = tail->next;
		}

		plms_ent_list_free(chld_aff_list);
	}

	if ( NULL != chld_aff_fg_list){
		
		tail = chld_aff_fg_list;
		while(tail){
			plms_ent_to_ent_list_add(tail->plm_entity,ent_list);
			tail = tail->next;
		}

		plms_ent_list_free(chld_aff_fg_list);
	}
	
	if ( NULL != dep_aff_list){
		
		tail = dep_aff_list;
		while(tail){
			plms_ent_to_ent_list_add(tail->plm_entity,ent_list);
			tail = tail->next;
		}

		plms_ent_list_free(dep_aff_list);
	}

	if ( NULL != dep_aff_fg_list){
		
		tail = dep_aff_fg_list;
		while(tail){
			plms_ent_to_ent_list_add(tail->plm_entity,ent_list);
			tail = tail->next;
		}

		plms_ent_list_free(dep_aff_fg_list);
	}

	TRACE("Affected entities for ent %s: ", root_ent->dn_name_str);
	log_head = *ent_list;
	while(log_head){
		TRACE(" %s",log_head->plm_entity->dn_name_str);
		log_head = log_head->next;
	}
	
	return;	
}
/******************************************************************************
@brief		:Find out the dependent entities which are affected because 
		the root ent(root_ent) is going to insvc/oos(is_insvc). We get to 
		traverse the rev_dep_list recursively. We also get to see the
		children of the ent part of rev_dep_list.

		if is_insvc is 1 ==>
		-- If the ent is insvc then return.
		-- If the ent is in OOS, then scan through the dependency list
		of the ent and see how many of them are part of the aff_ent_list
		Take that no into consideration to find out if the min dep 
		criteria for the ent is satisfied. If yes then add this ent 
		to aff_ent_list.	
		if is_insvc is 0 ==>
		-- if the ent is OOS and dep flag is set ==> return
		-- if the ent is OOS and dep flag is not set ==> If dep flag can
		be set, then add the ent to aff_ent_list and return.
		-- if the ent is insvc, then scan through the dependecy list
		of the ent and see how many of them are part of the aff_ent_list
		Take that no into consideration to find out if the min dep
		criteria for the ent is satisfied. If yes then add this
		ent to aff_ent_list.

		This is a recurssive function works in conjuction with 
		plms_aff_chld_ent_list_get
		
@param[in]	: root_ent - Root entity for which the affected entities will be
		calculated.
@param[in]	: Entity for which this function is called. Means we have to traverse
		ent`s rev_dep_list.
@param[out]	: ent_list - Head of list of affected entities whose readiness state
		is affected.
@param[out]	: ent_fg_list- Head of list of affected entities whose readiness flag
		is affected.
@param[in]	: is_insvc - 1 ==> In service
			     0 ==> Out o service.	
******************************************************************************/
void plms_aff_dep_ent_list_get(PLMS_ENTITY *root_ent,
				PLMS_GROUP_ENTITY *ent,
				PLMS_GROUP_ENTITY **ent_list,
				PLMS_GROUP_ENTITY **ent_fg_list,
				SaBoolT is_insvc)
{
	PLMS_GROUP_ENTITY *head;
	SaUint32T count = 0;
	
	if (NULL == ent)
		return;
	plms_aff_dep_ent_list_get(root_ent,ent->next,ent_list,
						ent_fg_list,is_insvc);
	if (is_insvc){
		switch(ent->plm_entity->entity_type){
		case PLMS_HE_ENTITY:
			/* If I am insvc before then return.*/
			if (SA_PLM_READINESS_IN_SERVICE == 
			ent->plm_entity->entity.he_entity.
			saPlmHEReadinessState){
				return;
			}
			
			/* If the parent is OOS, then neither the ent can be
			 * moved to insvc nor the dependency flag can be
			 * cleared
			 * */
			if(NULL != ent->plm_entity->parent){
				
				if(!(plms_is_rdness_state_set(
				ent->plm_entity->parent,
				SA_PLM_READINESS_IN_SERVICE)) &&
				
				(!plms_is_ent_in_ent_list(*ent_list,
				ent->plm_entity->parent) &&
				
				(0 != strcmp(root_ent->dn_name_str,
				ent->plm_entity->parent->dn_name_str)))){
				
					/* If dep flag is not set, 
					then should be set.*/
					if (!plms_rdness_flag_is_set(
						ent->plm_entity,
						SA_PLM_RF_DEPENDENCY)){
						plms_ent_to_ent_list_add(
						ent->plm_entity,
						ent_fg_list);
					}
					return;
				}
			}
			
			/* Check for min dep. */
			head = ent->plm_entity->dependency_list;
			while (head){
				if ((SA_PLM_READINESS_IN_SERVICE == 
				head->plm_entity->entity.he_entity.
				saPlmHEReadinessState) || 
				(plms_is_ent_in_ent_list(*ent_list,
				head->plm_entity)) ||  
				(0 == (strcmp(head->plm_entity->
				dn_name_str,root_ent->
				dn_name_str)))){
					count++;	
				}
				head = head->next;
			}
			/* Dep flag can be cleared.*/
			if (count >= ent->plm_entity->min_no_dep){
				/* If dep flag is not set, 
				then should be set.*/
				if (plms_rdness_flag_is_set(
					ent->plm_entity,
					SA_PLM_RF_DEPENDENCY)){
					plms_ent_to_ent_list_add(
					ent->plm_entity,
					ent_fg_list);
				}
			}else{
				if (!plms_rdness_flag_is_set(
					ent->plm_entity,
					SA_PLM_RF_DEPENDENCY)){
					plms_ent_to_ent_list_add(
					ent->plm_entity,
					ent_fg_list);
				}
				/* I can not move to insvc.*/
				return;
			}

			
			if ((SA_PLM_OPERATIONAL_DISABLED == 
			ent->plm_entity->entity.he_entity.saPlmHEOperationalState) || 
			((SA_PLM_HE_PRESENCE_ACTIVE != 
			ent->plm_entity->entity.he_entity.saPlmHEPresenceState) &&
			(SA_PLM_HE_PRESENCE_DEACTIVATING != 
			ent->plm_entity->entity.he_entity.saPlmHEPresenceState)) ||
			(SA_PLM_HE_ADMIN_UNLOCKED != 
			ent->plm_entity->entity.he_entity.saPlmHEAdminState)){
				/* I can not be insvc and hence no point in
				 * traversing my children and rev dependency 
				 list.*/
				return; 
			}
			/* This entity can be moved to insvc. */
			plms_ent_to_ent_list_add(ent->plm_entity,ent_list);
			break;
		case PLMS_EE_ENTITY:
			/* If I am insvc before then return.*/
			if (SA_PLM_READINESS_IN_SERVICE == 
					ent->plm_entity->entity.ee_entity. 
					saPlmEEReadinessState){
				return;
			}
			
			/* If the parent is OOS, then neither the entity
			 * can be moved to insvc nor the dependency flag
			 * can be cleared. Hence return.*/
			if(NULL != ent->plm_entity->parent){
				
				if(!(plms_is_rdness_state_set(
				ent->plm_entity->parent,
				SA_PLM_READINESS_IN_SERVICE)) &&
				
				(!plms_is_ent_in_ent_list(*ent_list,
				ent->plm_entity->parent) &&
				
				(0 != strcmp(root_ent->dn_name_str,
				ent->plm_entity->parent->dn_name_str)))){
					/* If dep flag is not set, 
					then should be set.*/
					if (!plms_rdness_flag_is_set(
						ent->plm_entity,
						SA_PLM_RF_DEPENDENCY)){
						plms_ent_to_ent_list_add(
						ent->plm_entity,
						ent_fg_list);
					}
					return;
				}
			}

			/* Check min dep.*/
			head = ent->plm_entity->dependency_list;
			while (head){
				if ((SA_PLM_READINESS_IN_SERVICE == 
				head->plm_entity->entity.ee_entity.
				saPlmEEReadinessState) || 
				(plms_is_ent_in_ent_list(*ent_list,
				head->plm_entity)) ||  
				(0 == (strcmp(head->plm_entity->
				dn_name_str, root_ent->dn_name_str)))){
					count++;	
				}
				head = head->next;
			}
			/* Dep flag can be cleared.*/
			if (count >= ent->plm_entity->min_no_dep){
				/* If dep flag is not set, 
				then should be set.*/
				if (plms_rdness_flag_is_set(
					ent->plm_entity,
					SA_PLM_RF_DEPENDENCY)){
					plms_ent_to_ent_list_add(
					ent->plm_entity,
					ent_fg_list);
				}
			}else{
				if (!plms_rdness_flag_is_set(
					ent->plm_entity,
					SA_PLM_RF_DEPENDENCY)){
					plms_ent_to_ent_list_add(
					ent->plm_entity,
					ent_fg_list);
				}
				
				/* I can not move to insvc.*/
				return;
			}
			
			if ((SA_PLM_OPERATIONAL_DISABLED == 
			ent->plm_entity->entity.ee_entity.saPlmEEOperationalState) || 
			((SA_PLM_EE_PRESENCE_INSTANTIATED != 
			ent->plm_entity->entity.ee_entity.saPlmEEPresenceState) &&
			(SA_PLM_EE_PRESENCE_TERMINATING != 
			ent->plm_entity->entity.ee_entity.saPlmEEPresenceState)) ||
			(SA_PLM_EE_ADMIN_UNLOCKED != 
			ent->plm_entity->entity.ee_entity.saPlmEEAdminState)){

				/* If I can not go insvc, then no point
				 * traversing my children and rev dependency
				 * list.*/
				return;
			}
			/* I can move to insvc. */
			plms_ent_to_ent_list_add(ent->plm_entity,ent_list);
			break;
		}
	}else{
		switch(ent->plm_entity->entity_type){
		case PLMS_HE_ENTITY:
			
			if ((SA_PLM_READINESS_OUT_OF_SERVICE == 
			ent->plm_entity->entity.he_entity.
			saPlmHEReadinessState) && (SA_PLM_RF_DEPENDENCY == 
			(ent->plm_entity->entity.he_entity.
			saPlmHEReadinessFlags & SA_PLM_RF_DEPENDENCY ))){
				/* I am already in OOS, then no point
				 * in traversing my children and rev dep list
				 * */
				return;
			}else if ((SA_PLM_READINESS_OUT_OF_SERVICE == 
			ent->plm_entity->entity.he_entity.
			saPlmHEReadinessState) && (SA_PLM_RF_DEPENDENCY != 
			(ent->plm_entity->entity.he_entity.
			saPlmHEReadinessFlags & SA_PLM_RF_DEPENDENCY ))){
				/* Check if the dependency flag can be set.*/
				head = ent->plm_entity->dependency_list;
				while (head){
					if ((SA_PLM_READINESS_IN_SERVICE == 
					head->plm_entity->entity.he_entity.
					saPlmHEReadinessState) && 
					((!plms_is_ent_in_ent_list(*ent_list,
					head->plm_entity)) &&  (0 !=  strcmp(head->plm_entity->
					dn_name_str,root_ent->dn_name_str)))){
						count++;	
					}
					head = head->next;
				}
				/* I am the affected ent.*/
				if (count < ent->plm_entity->min_no_dep)
					plms_ent_to_ent_list_add(
						ent->plm_entity,ent_fg_list);
				/* I am already in OOS and hence no point in 
				traversing my children and rev dep list.*/
				return;
			}else{/* If the entity is insvc or stopping. */
				head = ent->plm_entity->dependency_list;
				while (head){
					if ((SA_PLM_READINESS_IN_SERVICE == 
					head->plm_entity->entity.he_entity.
					saPlmHEReadinessState) && 
					((!plms_is_ent_in_ent_list(*ent_list,
					head->plm_entity)) &&  (0 != strcmp(head->plm_entity->
					dn_name_str,root_ent->dn_name_str)))){
						count++;	

					}
					head = head->next;
				}
				if (count < ent->plm_entity->min_no_dep)
					/* I can move to OOS.*/
					plms_ent_to_ent_list_add(
					ent->plm_entity,ent_list);
				else
					/* I can not move to OOS and hence no
					point in traversing my children and 
					rev dep list.*/
					return;
			}
			break;
		case PLMS_EE_ENTITY:
			if ((SA_PLM_READINESS_OUT_OF_SERVICE == 
			ent->plm_entity->entity.ee_entity.
			saPlmEEReadinessState) && (SA_PLM_RF_DEPENDENCY == 
			(ent->plm_entity->entity.ee_entity.
			saPlmEEReadinessFlags & SA_PLM_RF_DEPENDENCY ))){
				/* I am already in OOS and hence no point in
				 * traversing my children and rev dep list.
				 * */
				return;
			}else if ((SA_PLM_READINESS_OUT_OF_SERVICE == 
			ent->plm_entity->entity.ee_entity.
			saPlmEEReadinessState) && (SA_PLM_RF_DEPENDENCY != 
			(ent->plm_entity->entity.ee_entity.
			saPlmEEReadinessFlags & SA_PLM_RF_DEPENDENCY ))){
				/* Check if the dependency flag can be set.*/
				head = ent->plm_entity->dependency_list;
				while (head){
					if ((SA_PLM_READINESS_IN_SERVICE == 
					head->plm_entity->entity.ee_entity.
					saPlmEEReadinessState) && 
					((!plms_is_ent_in_ent_list(*ent_list,
					head->plm_entity)) &&  (0 != strcmp(head->plm_entity->
					dn_name_str,root_ent->dn_name_str)))){
						count++;	
					}
					head = head->next;
				}
				/* My dependency flag can be set.*/
				if (count < ent->plm_entity->min_no_dep)
					plms_ent_to_ent_list_add(
						ent->plm_entity,ent_fg_list);
				/* I am already in OOS and hence no point in 
				 * traversing my children and rev dep list.
				 * */
				return;
			}else{/* If the entity is insvc. */
				head = ent->plm_entity->dependency_list;
				while (head){
					if ((SA_PLM_READINESS_IN_SERVICE == 
					head->plm_entity->entity.ee_entity.
					saPlmEEReadinessState) && 
					((!plms_is_ent_in_ent_list(*ent_list,
					head->plm_entity)) &&  (0 != strcmp(head->plm_entity->
					dn_name_str,root_ent->dn_name_str)))){
						count++;	
					}
					head = head->next;
				}
				/* I can be moved to insvc.*/
				if (count < ent->plm_entity->min_no_dep)
					plms_ent_to_ent_list_add(
					ent->plm_entity,ent_list);
				else
					/* I can not move to insvc and hence
					 * no point in traversing the rev dep
					 * list and my children.*/
					return;
			}
			break;
		}
	}
	
	plms_aff_chld_ent_list_get(root_ent,ent->plm_entity->leftmost_child,
					ent_list,ent_fg_list,is_insvc);
	plms_aff_dep_ent_list_get(root_ent,ent->plm_entity->rev_dep_list,
					ent_list,ent_fg_list,is_insvc);
	return;
}
/******************************************************************************
@brief		: A child entity is always affected when the parent goes OOS
		 or insvc, but with the following exception. We also get to see
		 the rev_dep_ent of all children. 

		if is_insvc is 0 ==> If the ent is already in OOS and 
		dependency flag is set, means the entity is OOS because of 
		other ancestor or min dep criteria. Hence do not consider that 
		as affected ent.  	
		
		if is_insvc is 1 ==> As my parent is going to be insvc, I 
		should also go to insvc and clear my dependency flag, if all 
		other conditions are matched. 
		So If I am going to be insvc or my dependency flag is going to
		be cleared because of my parent, then I am affected entity.

		This is a recurssive function works in conjuction with
		plms_aff_dep_ent_list_get.
		
@param[in]	: root_ent - Root entity for which the affected entities will be
		calculated.
@param[in]	: Entity for which this function is called. Means we have to traverse
		ent`s child list. 
@param[out]	: ent_list - Head of list of affected entities whose readiness state
		is affected.
@param[out]	: ent_fg_list- Head of list of affected entities whose readiness flag
		is affected.
@param[in]	: is_insvc - 1 ==> In service
			     0 ==> Out o service.	
******************************************************************************/
void plms_aff_chld_ent_list_get(PLMS_ENTITY *root_ent,
				PLMS_ENTITY *ent, 
				PLMS_GROUP_ENTITY **ent_list,
				PLMS_GROUP_ENTITY **ent_fg_list,
				SaBoolT is_insvc)
{
	PLMS_GROUP_ENTITY *head;
	SaUint32T count = 0;
	
	if (NULL == ent){
		return;
	}
	
	plms_aff_chld_ent_list_get(root_ent,ent->right_sibling,ent_list,
							ent_fg_list,is_insvc);
	
	if(is_insvc){
		switch(ent->entity_type){
		case PLMS_HE_ENTITY:
			/* If I am insvc before then return.*/
			if (SA_PLM_READINESS_IN_SERVICE == 
			ent->entity.he_entity.
			saPlmHEReadinessState){
				return;
			}

			/* Min dep is not OK, then neither the entity can insvc
			 * not the dependency flag ca be cleared. Hence the
			 * ent is not the affected entity.
			 * */
			head = ent->dependency_list;
			while (head){
				if ((SA_PLM_READINESS_IN_SERVICE == 
				head->plm_entity->entity.he_entity.
				saPlmHEReadinessState) || 
				((plms_is_ent_in_ent_list(*ent_list,
				head->plm_entity)) ||  (0 == strcmp(head->plm_entity->
				dn_name_str,root_ent->dn_name_str)))){
					count++;	
				}
				head = head->next;
			}
			if (count < ent->min_no_dep){
				/* If dep flag is not set, then should be set.*/
				if (!plms_rdness_flag_is_set(ent,
						SA_PLM_RF_DEPENDENCY)){
					plms_ent_to_ent_list_add(ent,
					ent_fg_list);
				}
				return;
			}else {
				/* If dep flag is set, clear*/
				if (plms_rdness_flag_is_set(ent,
						SA_PLM_RF_DEPENDENCY)){
					plms_ent_to_ent_list_add(ent,
					ent_fg_list);
				}
			}
			if ((SA_PLM_OPERATIONAL_DISABLED == 
			ent->entity.he_entity.saPlmHEOperationalState) || 
			((SA_PLM_HE_PRESENCE_ACTIVE != 
			ent->entity.he_entity.saPlmHEPresenceState) &&
			(SA_PLM_HE_PRESENCE_DEACTIVATING != 
			ent->entity.he_entity.saPlmHEPresenceState)) ||
			(SA_PLM_HE_ADMIN_UNLOCKED != 
			ent->entity.he_entity.saPlmHEAdminState)){
				/* Parent is moving to insvc as well as min dep
				ents are also insvc. So the dependency flaf can
				be cleared. Hence the ent is one of the affected
				ent.*/
				if (plms_rdness_flag_is_set(ent,
						SA_PLM_RF_DEPENDENCY)){
					plms_ent_to_ent_list_add(ent,
					ent_fg_list);
				}

				/* This entity can not go to insvc, Hence
				 * no point to go ahead with its children and
				 * rev dependent list.*/ 
				return;
			}
			/* This entity can be moved to insvc.*/
			plms_ent_to_ent_list_add(ent,ent_list);
			break;
		case PLMS_EE_ENTITY:
			/* If I am insvc before then return.*/
			if (SA_PLM_READINESS_IN_SERVICE == 
			ent->entity.ee_entity.
			saPlmEEReadinessState){
				return;
			}
			
			/* Min dep is not OK, then neither the entity can insvc
			 * not the dependency flag ca be cleared. Hence the
			 * ent is not the affected entity.*/
			head = ent->dependency_list;
			while (head){
				if ((SA_PLM_READINESS_IN_SERVICE == 
				head->plm_entity->entity.ee_entity.
				saPlmEEReadinessState) || 
				((plms_is_ent_in_ent_list(*ent_list,
				head->plm_entity)) ||  (0 == strcmp(head->plm_entity->
				dn_name_str,root_ent->dn_name_str)))){
					count++;	
				}
				head = head->next;
			}
			if (count < ent->min_no_dep){
				/* If dep flag is not set, then should be set.*/
				if (!plms_rdness_flag_is_set(ent,
						SA_PLM_RF_DEPENDENCY)){
					plms_ent_to_ent_list_add(ent,
					ent_fg_list);
				}
				return;
			}else{
				/* If dep flag is set, clear*/
				if (plms_rdness_flag_is_set(ent,
						SA_PLM_RF_DEPENDENCY)){
					plms_ent_to_ent_list_add(ent,
					ent_fg_list);
				}
			}
			if ((SA_PLM_OPERATIONAL_DISABLED == 
			ent->entity.ee_entity.saPlmEEOperationalState) ||
			((SA_PLM_EE_PRESENCE_INSTANTIATED != 
			ent->entity.ee_entity.saPlmEEPresenceState) &&
			(SA_PLM_EE_PRESENCE_TERMINATING != 
			ent->entity.ee_entity.saPlmEEPresenceState)) ||
			(SA_PLM_EE_ADMIN_UNLOCKED != 
			ent->entity.ee_entity.saPlmEEAdminState)){
				/* Parent is moving to insvc as well as min dep
				ents are also insvc. So the dependency flag can
				be cleared. Hence the ent is one of the affected
				ent.*/	
				if (plms_rdness_flag_is_set(ent,
						SA_PLM_RF_DEPENDENCY)){
					plms_ent_to_ent_list_add(ent,
					ent_fg_list);
				}
				/* This entity can not go to insvc, Hence
				 * no point to go ahead with its children and
				 * rev dependent list.*/ 
				return;
			}
			/* This entity can be moved to insvc.*/
			plms_ent_to_ent_list_add(ent,ent_list);
			break;
		default:
			break;
		}
	}else{
		switch(ent->entity_type){
		case PLMS_HE_ENTITY:
			/* I am insvc and surely the affected ent.*/
			if ( (SA_PLM_READINESS_IN_SERVICE == 
			ent->entity.he_entity.saPlmHEReadinessState) || 
			( SA_PLM_READINESS_STOPPING ==
			ent->entity.he_entity.saPlmHEReadinessState)){
				plms_ent_to_ent_list_add(ent,ent_list);
			}else{
				
				if (!plms_rdness_flag_is_set(ent,
				SA_PLM_RF_DEPENDENCY)){
					plms_ent_to_ent_list_add(
					ent,ent_fg_list);
				}
				
				/* As I am alerady in OOS, then no point in
				 * traversing my children and rev dependency
				 * list*/
				return;
			}

			break;
		case PLMS_EE_ENTITY:
			/* I am insvc and surely the affected ent.*/
			if ( (SA_PLM_READINESS_IN_SERVICE == 
			ent->entity.ee_entity.saPlmEEReadinessState) || 
			(SA_PLM_READINESS_STOPPING ==
			ent->entity.ee_entity.saPlmEEReadinessState)){
				plms_ent_to_ent_list_add(ent,ent_list);
			}else{
				if (!plms_rdness_flag_is_set(ent,
				SA_PLM_RF_DEPENDENCY)){
					plms_ent_to_ent_list_add(
					ent,ent_fg_list);
				}
				
				/* As I am alerady in OOS, then no point in
				 * traversing my children and rev dependency
				 * list*/
				return;
			}
			break;
		default:
			break;
		}
	}
	
	plms_aff_dep_ent_list_get(root_ent,ent->rev_dep_list,ent_list,
							ent_fg_list,is_insvc);
	plms_aff_chld_ent_list_get(root_ent,ent->leftmost_child,ent_list,
							ent_fg_list,is_insvc);
	
	return;
}

/******************************************************************************
@brief		: Find out if ent is a child of root.

@param[in]	: root - parent
@param[in]	: ent - child

@return		: FALSE - If ent is not a child of root.
		  TRUE - If ent is a child of root.
******************************************************************************/
SaUint32T plms_is_chld(PLMS_ENTITY *root,PLMS_ENTITY *ent)
{
	SaUint32T ret_err;
	PLMS_GROUP_ENTITY *chld_list = NULL;

	TRACE_ENTER2("Root Entity: %s",root->dn_name_str);

	plms_chld_get(root->leftmost_child,&chld_list);
	if (NULL == chld_list)
		return FALSE;
	
	ret_err = plms_is_ent_in_ent_list(chld_list,ent);
	
	plms_ent_list_free(chld_list);
	
	TRACE_LEAVE2("Return Val: %d",ret_err);
	return ret_err;
}

/******************************************************************************
@brief		: Find out if ent is a child of root.

@param[in]	: root - parent
@param[in]	: ent - child

@return		: FALSE - If ent is not a child of root.
		  TRUE - If ent is a child of root.
******************************************************************************/
SaUint32T plms_chld_get(PLMS_ENTITY *ent, 
			PLMS_GROUP_ENTITY **chld_list)
{
	if (NULL == ent){
		return NCSCC_RC_SUCCESS; 
	}
	plms_ent_to_ent_list_add(ent,chld_list);
	plms_chld_get(ent->right_sibling,chld_list);
	plms_chld_get(ent->leftmost_child,chld_list);

	return NCSCC_RC_SUCCESS;
}
/******************************************************************************
@brief		: Get all the children of ent.

@param[in]	: ent - Parent entity whose children are to be found out.
@param[out]	: chld_list - Head of the children list.

@return		: NCSCC_RC_SUCCESS
******************************************************************************/
void plms_aff_ent_list_imnt_failure_get(PLMS_ENTITY *ent,
				PLMS_GROUP_ENTITY **ent_list,
				SaBoolT mark_unmark)
{
	PLMS_GROUP_ENTITY *chld_aff_list = NULL;
	PLMS_GROUP_ENTITY *dep_aff_list = NULL;
	PLMS_GROUP_ENTITY *tail;

	plms_aff_chld_list_imnt_failure_get(ent,ent->leftmost_child,
					mark_unmark,&chld_aff_list);

	plms_aff_dep_list_imnt_failure_get(ent,ent->rev_dep_list,
					mark_unmark,&dep_aff_list);

	if ( NULL != chld_aff_list){
		
		tail = chld_aff_list;
		while(tail){
			plms_ent_to_ent_list_add(tail->plm_entity,ent_list);
			tail = tail->next;
		}

		plms_ent_list_free(chld_aff_list);
	}

	if ( NULL != dep_aff_list){
		
		tail = dep_aff_list;
		while(tail){
			plms_ent_to_ent_list_add(tail->plm_entity,ent_list);
			tail = tail->next;
		}

		plms_ent_list_free(dep_aff_list);
	}
	
	return;
}

/******************************************************************************
@brief		: Find the affected entities whose dependency-imminent-failure 
		flag is to be marked/unmarked.	

@param[in]	: ent - Entity whose affected entities to be found out.
@param[out]	: ent_list - Head of list of affecte entities.
@param[in]	: mark_unmark - 1 ==> mark dependency-imminent-failure
			      - 0 ==> unmark dependency-imminent-failure	
******************************************************************************/
void plms_aff_chld_list_imnt_failure_get(PLMS_ENTITY *root_ent,
					PLMS_ENTITY *ent,
					SaUint32T mark_unmark,
					PLMS_GROUP_ENTITY **aff_ent_list)
{
	PLMS_GROUP_ENTITY *head;
	SaUint32T count = 0;
	
	if (NULL == ent)
		return;

	plms_aff_chld_list_imnt_failure_get(root_ent,ent->right_sibling,
	mark_unmark,aff_ent_list);
	
	if (!mark_unmark){/* Clear the dep-imminent-failure flag.*/
		
		/* The dep-imminet-failure flag is not set, then return.*/
		if (!plms_rdness_flag_is_set(ent,
			SA_PLM_RF_DEPENDENCY_IMMINENT_FAILURE)){
			
			plms_aff_chld_list_imnt_failure_get(root_ent,ent->leftmost_child,
			mark_unmark,aff_ent_list);
			return;
		}
		
		/* If the min dep criteria is met, then have to clear the
		dep-imminet-failure flag.*/
		head = ent->dependency_list;
		while(head){
			
			if ( ((PLMS_HE_ENTITY == head->plm_entity->entity_type) && 
			(SA_PLM_OPERATIONAL_ENABLED == head->plm_entity->entity.he_entity.saPlmHEOperationalState)) || 
			((PLMS_EE_ENTITY == head->plm_entity->entity_type) && 
			SA_PLM_OPERATIONAL_ENABLED == head->plm_entity->entity.ee_entity.saPlmEEOperationalState)){

				if (!plms_rdness_flag_is_set(head->plm_entity,SA_PLM_RF_DEPENDENCY_IMMINENT_FAILURE) &&
				!(plms_rdness_flag_is_set(head->plm_entity,SA_PLM_RF_IMMINENT_FAILURE))){
					count++;
				}else if (plms_is_ent_in_ent_list(*aff_ent_list,head->plm_entity) || 
				(0==strcmp(root_ent->dn_name_str,head->plm_entity->dn_name_str))){
					count++;
				}
			}
			head = head->next;
		}
		if (count >= ent->min_no_dep){
			/* Add the ent to aff ent ist.*/
			plms_ent_to_ent_list_add(ent,aff_ent_list);
			
			if (plms_rdness_flag_is_set(ent,
			SA_PLM_RF_IMMINENT_FAILURE)){
				return;
			}
		}else{
			/* If my dep-imminent-failure flag can not be cleared,
			the no point in traversing the chld and dep entities.*/
			return;
		}

	}else{ /* Set the dep-imminent-failure flag.*/
		/* If operational state is already disabled,
		 * then return.*/
		if ( ((PLMS_HE_ENTITY == ent->entity_type) && 
		(SA_PLM_OPERATIONAL_DISABLED == 
		ent->entity.he_entity.saPlmHEOperationalState)) ||
		((PLMS_EE_ENTITY == ent->entity_type) &&
		(SA_PLM_OPERATIONAL_DISABLED ==
		ent->entity.ee_entity.saPlmEEOperationalState))) {
			plms_aff_chld_list_imnt_failure_get(root_ent,ent->leftmost_child,
			mark_unmark,aff_ent_list);
			return;
		}
		/* If the dep-imminent failure is set for the entity then,
		no need to traverse the dep and chld as their dep-imminent-
		failure flag would be set.*/		
		if (plms_rdness_flag_is_set(ent,
			SA_PLM_RF_DEPENDENCY_IMMINENT_FAILURE)){
			return;
		}
		/* If the imminent failure is set for the entity then,
		no need to traverse the dep and chld as their dep-imminent-
		failure flag would be set. But we have to mark the dep-imminet
		-failure flag for this entity.*/		
		if (plms_rdness_flag_is_set(ent,
			SA_PLM_RF_IMMINENT_FAILURE)){

			/* Add the ent to aff ent ist.*/
			plms_ent_to_ent_list_add(ent,aff_ent_list);
			return;
		}

		/* Add the ent to aff ent ist.*/
		plms_ent_to_ent_list_add(ent,aff_ent_list);

	}

	plms_aff_dep_list_imnt_failure_get(root_ent,ent->rev_dep_list,
	mark_unmark,aff_ent_list);

	plms_aff_chld_list_imnt_failure_get(root_ent,ent->leftmost_child,
	mark_unmark,aff_ent_list);

	return;
}

/******************************************************************************
@brief		: Find out the dep entities (whose imminent-failure-flag is 
		to be set/reset) of the ent(ent not root_ent).
		 
@param[in]	: root_ent - Root entity.
@param[in]	: ent - Entity for which this recurssive function is called and
		whose affected dep entities are to be calculated.
@param[in]	: mark_unmark - If set to 1 ==> marked.
					  0 ==> unmarked.
@param[out]	: aff_ent_list - Head of affected entity list.					  
******************************************************************************/
void plms_aff_dep_list_imnt_failure_get(PLMS_ENTITY *root_ent,
					PLMS_GROUP_ENTITY *ent,
					SaUint32T mark_unmark,
					PLMS_GROUP_ENTITY **aff_ent_list)
{
	SaUint32T count = 0;
	PLMS_GROUP_ENTITY *head;

	if (NULL == ent)
		return;

	plms_aff_dep_list_imnt_failure_get(root_ent,ent->next,mark_unmark,
	aff_ent_list);

	if(!mark_unmark){
		/* The dep-imminet-failure flag is not set, then return.*/
		if (!plms_rdness_flag_is_set(ent->plm_entity,
			SA_PLM_RF_DEPENDENCY_IMMINENT_FAILURE)){
			return;
		}

		/* If my parent`s imminent-failure
		flag is set, then return.*/
		if (NULL != ent->plm_entity->parent){
			if (plms_rdness_flag_is_set(ent->plm_entity->parent,
			SA_PLM_RF_IMMINENT_FAILURE)){
				return;
			}
		}
		
		/* Check for min dep criteria.*/
		head = ent->plm_entity->dependency_list;
		while(head){
			
			if ( ((PLMS_HE_ENTITY == head->plm_entity->entity_type) && 
			(SA_PLM_OPERATIONAL_ENABLED == head->plm_entity->entity.he_entity.saPlmHEOperationalState)) || 
			((PLMS_EE_ENTITY == head->plm_entity->entity_type) && 
			SA_PLM_OPERATIONAL_ENABLED == head->plm_entity->entity.ee_entity.saPlmEEOperationalState)){

				if (!plms_rdness_flag_is_set(head->plm_entity,SA_PLM_RF_DEPENDENCY_IMMINENT_FAILURE) &&
				!(plms_rdness_flag_is_set(head->plm_entity,SA_PLM_RF_IMMINENT_FAILURE))){
					count++;
				}else if (plms_is_ent_in_ent_list(*aff_ent_list,head->plm_entity) || 
				(0==strcmp(root_ent->dn_name_str,head->plm_entity->dn_name_str))){
					count++;
				}
			}
			head = head->next;
		}

		if (count >= ent->plm_entity->min_no_dep){
			plms_ent_to_ent_list_add(ent->plm_entity,aff_ent_list);
			
			if (plms_rdness_flag_is_set(ent->plm_entity,
			SA_PLM_RF_IMMINENT_FAILURE)){
				return;
			}
				
		}else{
			return;
		}
			
	}else{
		/* If the operational state is disabled, then return.*/
		if ( ((PLMS_HE_ENTITY == ent->plm_entity->entity_type) && 
		(SA_PLM_OPERATIONAL_DISABLED == 
		ent->plm_entity->entity.he_entity.saPlmHEOperationalState)) ||
		((PLMS_EE_ENTITY == ent->plm_entity->entity_type) &&
		(SA_PLM_OPERATIONAL_DISABLED ==
		ent->plm_entity->entity.ee_entity.saPlmEEOperationalState)) ) {
			return;
		}
		
		/* If the imminent dependency flag is set, then
		 * return.*/		
		if (plms_rdness_flag_is_set(ent->plm_entity,
			SA_PLM_RF_DEPENDENCY_IMMINENT_FAILURE)){
			return;
		}
		
		/* Check for min dep criteria.*/
		head = ent->plm_entity->dependency_list;
		while(head){
			if (((!plms_rdness_flag_is_set(head->plm_entity,
			SA_PLM_RF_DEPENDENCY_IMMINENT_FAILURE)) && 
			(!plms_rdness_flag_is_set(head->plm_entity,
			SA_PLM_RF_IMMINENT_FAILURE))) && 
			(!plms_is_ent_in_ent_list(*aff_ent_list,
			head->plm_entity)) && (0!=strcmp(root_ent->dn_name_str,
			head->plm_entity->dn_name_str)) &&
			
			(((PLMS_HE_ENTITY == head->plm_entity->entity_type) &&
			(SA_PLM_OPERATIONAL_ENABLED ==
			head->plm_entity->entity.he_entity.
			saPlmHEOperationalState)) ||((PLMS_EE_ENTITY == 
			head->plm_entity->entity_type) && 
			(SA_PLM_OPERATIONAL_ENABLED == head->plm_entity->entity.
			ee_entity.saPlmEEOperationalState))) ){
				count++;
			}
			head = head->next;
		}

		if (count >= ent->plm_entity->min_no_dep){
			return;
		}else{
			/* Add the ent to aff ent ist.*/
			plms_ent_to_ent_list_add(ent->plm_entity,aff_ent_list);
		}
	
		/* If the imminent failure is set for the entity then,
		no need to traverse the dep and chld as their dep-imminent-
		failure flag would be set. But we have to mark the dep-imminet
		-failure flag for this entity.*/		
		if (plms_rdness_flag_is_set(ent->plm_entity,
			SA_PLM_RF_IMMINENT_FAILURE)){
			return;
		}
	}

	plms_aff_chld_list_imnt_failure_get(root_ent,
	ent->plm_entity->leftmost_child, mark_unmark,aff_ent_list);

	plms_aff_dep_list_imnt_failure_get(root_ent,
	ent->plm_entity->rev_dep_list, mark_unmark,aff_ent_list);
	
	return;
}
/******************************************************************************
@brief		: Find out the affected HEs part of aff_ent_list.

@param[in]	: aff_ent_list - Head of list of affected entities.
@param[out]	: aff_he_list - Head of the list of affected HEs.

@return		: NCSCC_RC_SUCCESS
******************************************************************************/
SaUint32T plms_aff_he_find(PLMS_GROUP_ENTITY *aff_ent_list,
				PLMS_GROUP_ENTITY **aff_he_list)
{
	PLMS_GROUP_ENTITY *head;

	head = aff_ent_list;
	while(head){
		if (PLMS_HE_ENTITY == head->plm_entity->entity_type){
			plms_ent_to_ent_list_add(head->plm_entity,aff_he_list);
		}
		head = head->next;	
	}

	return NCSCC_RC_SUCCESS;
}
/******************************************************************************
@brief		: Find all the affected EEs which are children of affected 
		HEs as well as children of target ent.

@param[in]	: ent - Root entity.
@param[in]	: aff_he_list - List of affected HEs.
@param[in]	: aff_ent_list - List of all affected entities.
@param[out]	: aff_chld_ee_list - List of affected EEs which are children of
		affected HEs.
		
@return		: NCSCC_RC_SUCCESS
******************************************************************************/
SaUint32T plms_ee_chld_of_aff_he_find(PLMS_ENTITY *ent,
				PLMS_GROUP_ENTITY *aff_he_list,
				PLMS_GROUP_ENTITY *aff_ent_list,
				PLMS_GROUP_ENTITY **aff_chld_ee_list)
{
	PLMS_GROUP_ENTITY *head;
	PLMS_ENTITY *parent;

	head = aff_ent_list;
	while(head){
		if (PLMS_EE_ENTITY == head->plm_entity->entity_type){
			parent = head->plm_entity->parent;
			while(parent){
				if(plms_is_ent_in_ent_list(aff_he_list,
							parent)){
					plms_ent_to_ent_list_add(
							head->plm_entity,
							aff_chld_ee_list);
					break;
				}else if(0 == strcmp(parent->dn_name_str,ent->dn_name_str)){
					plms_ent_to_ent_list_add(
							head->plm_entity,
							aff_chld_ee_list);
					break;
					
				}else{
				}
				parent = parent->parent;
			}
		}
		head = head->next;
	}

	return NCSCC_RC_SUCCESS;
}
/******************************************************************************
@brief		: Find out all the EEs which are affected entities but not child
		of any of the affected HEs.

@param[in]	: aff_ent_list - List of all affected entities.
@param[in]	: aff_chld_ee_list - List of affected child EEs.
@param[out]	: dep_chld_ee_list - List of affected EEs which are not children
		of any of the affected HEs.

@return		: NCSCC_RC_SUCCESS.
******************************************************************************/
SaUint32T plms_ee_not_chld_of_aff_he_find(PLMS_GROUP_ENTITY *aff_ent_list,
					PLMS_GROUP_ENTITY *aff_chld_ee_list,
					PLMS_GROUP_ENTITY **dep_chld_ee_list)
{
	PLMS_GROUP_ENTITY *head;

	head = aff_ent_list;
	while(head){
		if (PLMS_HE_ENTITY == head->plm_entity->entity_type){
			head = head->next;
			continue;
		}
		if (!(plms_is_ent_in_ent_list(aff_chld_ee_list,
					head->plm_entity))){
			plms_ent_to_ent_list_add(head->plm_entity,
						dep_chld_ee_list);
		}
		head = head->next;
	}
	return NCSCC_RC_SUCCESS;
}
/******************************************************************************
@brief		: Add all the groups to list, ent belongs to. 

@param[in]	: ent - Root entity.
@param[out]	: list - List of groups, ent belongs to.

@return		: NCSCC_RC_SUCCESS
******************************************************************************/
SaUint32T plms_ent_grp_list_add(PLMS_ENTITY *ent, 
			PLMS_ENTITY_GROUP_INFO_LIST **list)
{
	SaUint32T is_present;
	PLMS_ENTITY_GROUP_INFO_LIST *tail,*prev;
	PLMS_ENTITY_GROUP_INFO_LIST *ent_grp;

	ent_grp = ent->part_of_entity_groups;
	while (ent_grp){	
		tail = *list;
		/* Ist node. */
		if (NULL == tail){
			tail = (PLMS_ENTITY_GROUP_INFO_LIST *)calloc(1,
				sizeof(PLMS_ENTITY_GROUP_INFO_LIST));
			if (NULL == tail){
				LOG_CR("plms_ent_grp_list_add, calloc FAILED.\
				Error: %s",strerror(errno));
				assert(0);
			}
			tail->ent_grp_inf = ent_grp->ent_grp_inf;
			tail->next = NULL;
			*list = tail;
		}else{
			is_present = 0;
			/* Get the tail.*/
			while (tail){
				/* If the grp is part of the list.*/
				if (tail->ent_grp_inf->entity_grp_hdl ==
					ent_grp->ent_grp_inf->entity_grp_hdl){
					is_present = 1;
					break;
				}
				prev = tail;
				tail = tail->next;
			}
			if(is_present){
				ent_grp = ent_grp->next;
				continue;
			}

			/* Add the node to the tail of the list.*/
			prev->next = (PLMS_ENTITY_GROUP_INFO_LIST *)calloc(1,
					sizeof(PLMS_ENTITY_GROUP_INFO_LIST));
			if (NULL == prev->next){
				LOG_CR("plms_ent_grp_list_add, calloc FAILED.\
				Error: %s",strerror(errno));
				assert(0);
			}
			prev->next->ent_grp_inf = ent_grp->ent_grp_inf;
			prev->next->next = NULL;
			
		}
		ent_grp = ent_grp->next;
	}
	return NCSCC_RC_SUCCESS;
}
/******************************************************************************
@brief		: Add all the groups to grp_list,entities of ent_list belong to. 

@param[in]	: ent_list - List of entities.
@param[out]	: list - List of groups, all the entities of ent_list belong to.
******************************************************************************/
void plms_ent_list_grp_list_add(PLMS_GROUP_ENTITY *ent_list, 
			PLMS_ENTITY_GROUP_INFO_LIST **list)
{
	PLMS_GROUP_ENTITY *ent;

	ent = ent_list;
	while(ent){
		plms_ent_grp_list_add(ent->plm_entity,list);
		ent = ent->next;
	}
	return;
}

/******************************************************************************
@brief		: Add the ent to ent_list only if it is not part of the list.

@param[in]	: ent - Root entity.
@param[in/out]	: list - List of entities to which ent will be added.
******************************************************************************/
void plms_ent_to_ent_list_add(PLMS_ENTITY *ent,
			PLMS_GROUP_ENTITY **list)
{
	PLMS_GROUP_ENTITY *tail,*prev;

	tail = *list;
	/* Ist node. */
	if (NULL == tail){
		tail = (PLMS_GROUP_ENTITY *)calloc(1,sizeof(PLMS_GROUP_ENTITY));
		if (NULL == tail){
			LOG_CR("plms_ent_to_ent_list_add, calloc FAILED.\
			Error: %s",strerror(errno));
			assert(0);
		}
		tail->plm_entity = ent;
		tail->next = NULL;
		*list = tail;
	}else{
		/* Get the tail.*/
		while (tail){
			/* If the ent is part of the list, then return.*/
			if (0 == strcmp(tail->plm_entity->dn_name_str,
							ent->dn_name_str)){
				return;
			}
			prev = tail;
			tail = tail->next;
		}
		/* Add the ent to the tail of the list.*/
		prev->next = (PLMS_GROUP_ENTITY *)calloc(1,
					sizeof(PLMS_GROUP_ENTITY));
		if (NULL == prev->next){
			LOG_CR("plms_ent_to_ent_list_add, calloc FAILED.\
			Error: %s",strerror(errno));
			assert(1);
		}
		prev->next->plm_entity = ent;
		prev->next->next = NULL;
		
	}
	return;
}
/******************************************************************************
@brief		: If the grp, represented by tmp_ent_grp is present in grp_list. 

@param[in]	: tmp_ent_grp - Entity group to be serached for.
@param[in]	: grp_list - List of groups.

@return		: NCSCC_RC_FAILURE - If tmp_ent_grp is not part of grp_list.
		  NCSCC_RC_SUCCESS - If tmp_ent_grp is part of grp_list.	
******************************************************************************/

SaUint32T plms_grp_is_in_grp_list(PLMS_ENTITY_GROUP_INFO *tmp_ent_grp,
				PLMS_ENTITY_GROUP_INFO_LIST *grp_list)
{
	PLMS_ENTITY_GROUP_INFO_LIST *tmp_grp_list;
	
	tmp_grp_list= grp_list;
	while (tmp_grp_list){
		if (tmp_grp_list->ent_grp_inf->entity_grp_hdl == 
				tmp_ent_grp->entity_grp_hdl)
			return NCSCC_RC_SUCCESS;
		else
			tmp_grp_list = tmp_grp_list->next;
	}
	return NCSCC_RC_FAILURE;
}

/******************************************************************************
@brief		: Add inv_to_trk node to list.

@param[in]	: node - inv_to_trk node to be added to the list.
@param[out]	: list - list of inv_to_trk.
******************************************************************************/
void plms_inv_to_trk_grp_add(PLMS_INVOCATION_TO_TRACK_INFO **list,
				PLMS_INVOCATION_TO_TRACK_INFO *node)
{
	PLMS_INVOCATION_TO_TRACK_INFO *tail,*prev;

	tail = *list;
	/* Ist node. */
	if (NULL == tail){
		tail = node;
		*list = tail;
	}else{
		/* Get the tail.*/
		while (tail){
			prev = tail;
			tail = tail->next;
		}
		prev->next = node;
	}
	return;
}

/******************************************************************************
@brief		: Remove all the inv_to_trk node from the grp->invocation_list
		if the inv_to_trk points to the track info passed as trk_info.

@param[in]	: grp - Group from where inv_to_trk nodes are to be removed.
@trk_info[in]	: trk_info - Track info. 
******************************************************************************/
void plms_inv_to_cbk_in_grp_trk_rmv(PLMS_ENTITY_GROUP_INFO *grp,
				PLMS_TRACK_INFO *trk_info)
{
	PLMS_INVOCATION_TO_TRACK_INFO **inv_list,**prev;

	inv_list = &(grp->invocation_list);
	prev = &(grp->invocation_list);
	while(*inv_list){
		if (trk_info == (*inv_list)->track_info){
			(*prev)->next = (*inv_list)->next;
			(*inv_list)->track_info = NULL;
			(*inv_list)->next = NULL;
			free(*inv_list);
			*inv_list = NULL;
			return;
		}
		*prev = *inv_list;
		*inv_list = (*inv_list)->next;
	}

	return;
		
}
/******************************************************************************
@brief		: Remove the inv_to_trk node from the grp->invocation_list
		if the inv id of inv_to_trk node matches with inv_id passed.

@param[in]      : grp - Group from where inv_to_trk nodes are to be removed.
@param[in]	: inv_id - innvocation id.
******************************************************************************/
void plms_inv_to_cbk_in_grp_inv_rmv(PLMS_ENTITY_GROUP_INFO *grp,
				SaInvocationT inv_id)
{
	PLMS_INVOCATION_TO_TRACK_INFO **inv_list,**prev;

	inv_list = &(grp->invocation_list);
	prev = &(grp->invocation_list);
	while(*inv_list){
		if (inv_id == (*inv_list)->invocation){
			(*prev)->next = (*inv_list)->next;
			(*inv_list)->track_info = NULL;
			(*inv_list)->next = NULL;
			free(*inv_list);
			*inv_list = NULL;
			return;
		}
		*prev = *inv_list;
		*inv_list = (*inv_list)->next;
	}

	return;
}
/******************************************************************************
@brief		: Find inv_to_cbk node from list whose inv_id matches with inv.

@param[in]	: list - List of inv_to_trk nodes.
@param[in]	: inv - innvocation id.

@return		: NULL - If inv does not match.
		  PLMS_INVOCATION_TO_TRACK_INFO - If the inv matches.
******************************************************************************/
PLMS_INVOCATION_TO_TRACK_INFO * plms_inv_to_cbk_in_grp_find(
				PLMS_INVOCATION_TO_TRACK_INFO *list, 
				SaInvocationT inv)
{
	PLMS_INVOCATION_TO_TRACK_INFO *head=list;
	while (head){
		if (inv == head->invocation)
			return head;
		else
			head = head->next;
	}
	return NULL;
}

/******************************************************************************
@brief		: Find the no of entities to be packed for calling the callback.
		  track_flag should be taken into consideration.

@param[in]	: grp - Group for which the callback will be called.
@param[in]	: aff_ent_list - List of affected entities.

@return		: No of entities to be packed(0/count_changes/count_changes_only).
******************************************************************************/
SaUint32T plms_no_aff_ent_in_grp_get(PLMS_ENTITY_GROUP_INFO *grp,
				PLMS_GROUP_ENTITY *aff_ent_list)
{
	PLMS_GROUP_ENTITY_ROOT_LIST *head;
	int count_changes = 0;
	int count_changes_only = 0;
	
	head = grp->plm_entity_list;
	while (head){
		if (plms_is_ent_in_ent_list(aff_ent_list,head->plm_entity))
			count_changes_only++;
		count_changes++;
		head = head->next;
	}
	
	if ( m_PLM_IS_SA_TRACK_CHANGES_ONLY_SET(grp->track_flags)){
			return count_changes_only;	
	}else{
			return count_changes;
	}
	return 0;	
}

/******************************************************************************
@brief		: Fill the affected entities to be sent as part of the readiness
		callback.

@param[in]	: ent - Pointer of type SaPlmReadinessTrackedEntityT which holds
		the entities to be sent as part of the readiness callback.
@param[in]	: grp - Group for which the callback will be called.
@param[in]	: aff_ent_list - List of affected entities.
@param[in]	: grp_op - Type of change (group change/readiness status change).
******************************************************************************/
void plms_grp_aff_ent_fill(SaPlmReadinessTrackedEntityT *ent,
				PLMS_ENTITY_GROUP_INFO *grp,
				PLMS_GROUP_ENTITY *aff_ent_list,
				SaPlmGroupChangesT grp_op,
				SaPlmChangeStepT change_step)
{
	int count=0;
	PLMS_GROUP_ENTITY_ROOT_LIST *head;

	head = grp->plm_entity_list;
	if ( m_PLM_IS_SA_TRACK_CHANGES_ONLY_SET(grp->track_flags)){
		while (head){
			if (plms_is_ent_in_ent_list(aff_ent_list,
							head->plm_entity)){
				ent[count].change = grp_op;
				ent[count].entityName = 
						head->plm_entity->dn_name;
				/* TODO: For the time being UNUSED is filled.
				This has to be the ntf-id */		
				ent[count].plmNotificationId = 
						SA_NTF_IDENTIFIER_UNUSED; 
				if (PLMS_HE_ENTITY == head->plm_entity->
								entity_type){
				
					ent[count].currentReadinessStatus.
					readinessState = 
					head->plm_entity->entity.he_entity.
							saPlmHEReadinessState;

					ent[count].currentReadinessStatus.
					readinessFlags = 
					head->plm_entity->entity.he_entity.
							saPlmHEReadinessFlags;
				}else{
					ent[count].currentReadinessStatus.
					readinessState =  
					head->plm_entity->entity.ee_entity.
							saPlmEEReadinessState;

					ent[count].currentReadinessStatus.
					readinessFlags =  
					head->plm_entity->entity.ee_entity.
							saPlmEEReadinessFlags;
				
				}
				if (SA_PLM_CHANGE_COMPLETED == change_step){
					ent[count].expectedReadinessStatus = ent[count].currentReadinessStatus;
				}else{
					ent[count].expectedReadinessStatus = head->plm_entity->exp_readiness_status; 
				}
			count++;
			}
			head = head->next;
		}
	}else{ 
		
		while (head){
			if ( plms_is_ent_in_ent_list(aff_ent_list,
						head->plm_entity)){
				ent[count].change = grp_op;
			}else {
				ent[count].change = SA_PLM_GROUP_NO_CHANGE;
			}

			ent[count].entityName = 
					head->plm_entity->dn_name;

			/* TODO: For the time being UNUSED is filled.
			This has to be the ntf-id */		
			ent[count].plmNotificationId = 
					SA_NTF_IDENTIFIER_UNUSED;
			if (PLMS_HE_ENTITY == head->plm_entity->
							entity_type){
					
				ent[count].currentReadinessStatus.
				readinessState = 
				head->plm_entity->entity.he_entity.
						saPlmHEReadinessState;

				ent[count].currentReadinessStatus.
				readinessFlags = 
				head->plm_entity->entity.he_entity.
						saPlmHEReadinessFlags;
			}else{
				ent[count].currentReadinessStatus.
				readinessState =  
				head->plm_entity->entity.ee_entity.
						saPlmEEReadinessState;

				ent[count].currentReadinessStatus.
				readinessFlags =  
				head->plm_entity->entity.ee_entity.
						saPlmEEReadinessFlags;
			}
			if (SA_PLM_CHANGE_COMPLETED == change_step){
				ent[count].expectedReadinessStatus = ent[count].currentReadinessStatus;
			}else{
				if (SA_PLM_GROUP_NO_CHANGE == ent[count].change ){
					ent[count].expectedReadinessStatus = ent[count].currentReadinessStatus;
				}else{
					ent[count].expectedReadinessStatus = head->plm_entity->exp_readiness_status; 
				}
			}
		count++;
		head = head->next;
		}
	}
	return;  
}
/******************************************************************************
@brief		: Free inv_to_cbk list.
@param[in]	: inv_list - List of inv_to_cbk nodes to be freed.
******************************************************************************/
void plms_inv_to_trk_list_free(PLMS_INVOCATION_TO_TRACK_INFO *inv_list)
{
	PLMS_INVOCATION_TO_TRACK_INFO *free_node;
	while (inv_list){
		free_node = inv_list;
		inv_list = inv_list->next;
		free_node->track_info = NULL;
		free_node->next = NULL;
		free(free_node);
	}
	return;
}


/******************************************************************************
@brief		: Free entity list.
@param[in]	: ent_list - List of entities to be freed.
******************************************************************************/
void plms_ent_list_free(PLMS_GROUP_ENTITY *ent_list)
{
	PLMS_GROUP_ENTITY *free_node;
	while (ent_list){
		free_node = ent_list;
		ent_list = ent_list->next;
		free_node->plm_entity = NULL;
		free_node->next = NULL;
		free(free_node);
	}
	return;
}

/******************************************************************************
@brief		: Free entity list.
@param[in]	: ent_list - List of entities to be freed.
******************************************************************************/
void plms_ent_grp_list_free(PLMS_ENTITY_GROUP_INFO_LIST *grp_list)
{
	PLMS_ENTITY_GROUP_INFO_LIST *free_node;
	while (grp_list){
		free_node = grp_list;
		grp_list = grp_list->next;
		free_node->ent_grp_inf = NULL;
		free_node->next = NULL;
		free(free_node);
	}
	return;
}

/******************************************************************************
@brief		: Mark/unmark am_i_aff_ent flag of the entities.
@param[in]	: ent_list - List of entities.
@param[in]	: mark_unmark - 0 ==> Unmark
				1 ==> Mark
******************************************************************************/
void plms_aff_ent_flag_mark_unmark(PLMS_GROUP_ENTITY *ent_list,
						SaBoolT mark_unmark)
{
	PLMS_GROUP_ENTITY *head;

	head = ent_list;
	while(head){
		head->plm_entity->am_i_aff_ent = mark_unmark;
		head = head->next;
	}
	return;
}
/******************************************************************************
@brief		: Check if the specified readiness flag is set.
@param[in]	: ent - PLM entity.
@param[in]	: flag - Readiness flag.
@return		: TRUE - If set
		  FALSE - If not set.
******************************************************************************/
SaUint32T plms_rdness_flag_is_set(PLMS_ENTITY *ent,SaPlmReadinessFlagsT flag)
{
	SaUint32T ret_err;
	
	if (PLMS_HE_ENTITY == ent->entity_type){
		if ((flag & ent->entity.he_entity.saPlmHEReadinessFlags) == 
									flag ){
			ret_err = TRUE;	
		}else{
			ret_err = FALSE;
		}

	}else{
		if ((flag & ent->entity.ee_entity.saPlmEEReadinessFlags) == 
									flag ){
			ret_err = TRUE;	
		}else{
			ret_err = FALSE;
		}
	}
	return ret_err;
}
/******************************************************************************
@brief		: Clear readiness flag. Takes care of updating IMM and sending 
		the corresponding notification.
@param[in]	: ent - Entity whose readiness flag is to be cleared.
@param[in]	: root_ent - Root entity for which readiness flag to be cleared
		of the ent.
@param[in]	: src_ind - Source indicator to be used for notification.
@param[in]	: minor_id - Minor id to be used to send notification.

@return		: SA_NTF_IDENTIFIER_UNUSED - If notification not sent.
		SaNtfIdentifierT - Notfication id.	
******************************************************************************/
SaNtfIdentifierT plms_readiness_flag_clear(PLMS_ENTITY *ent,
					PLMS_ENTITY *root_ent,
					SaUint16T src_ind,
					SaUint16T minor_id)
{
	SaUint32T ntf = 0;
	SaUint32T old_flag;
	SaUint32T ret_err = NCSCC_RC_FAILURE;
	SaUint32T ret_val;
	PLMS_CB *cb = plms_cb;
	SaNtfIdentifierT ntf_id = SA_NTF_IDENTIFIER_UNUSED;
	SaPlmNtfStateChangeT ntf_state;

	if(PLMS_HE_ENTITY == ent->entity_type){
		if (PLMS_READINESS_FLAG_RESET != 
			ent->entity.he_entity.saPlmHEReadinessFlags){ 
			
			old_flag = ent->entity.he_entity.saPlmHEReadinessFlags;
			
			ent->entity.he_entity.saPlmHEReadinessFlags = 
						PLMS_READINESS_FLAG_RESET;

			ent->mngt_lost_tri = PLMS_MNGT_NONE;
			ent->iso_method = PLMS_ISO_DEFAULT;
		
			TRACE("Readiness Flags cleared for the entity %s",
			ent->dn_name_str);

			/* Update IMM.*/
			ret_val = plms_attr_imm_update(ent,
					"saPlmHEReadinessFlags",
					SA_IMM_ATTR_SAUINT64T,
					PLMS_READINESS_FLAG_RESET);
			if (NCSCC_RC_SUCCESS != ret_val){
				LOG_ER("Cleared readiness flag for the ent %s,\
				Updating IMM FAILED",ent->dn_name_str);
			}

			ntf = 1;
			ret_err = NCSCC_RC_SUCCESS;

		}	
	}else{
		if (PLMS_READINESS_FLAG_RESET != 
			ent->entity.ee_entity.saPlmEEReadinessFlags){ 
	
			old_flag = ent->entity.ee_entity.saPlmEEReadinessFlags;

			ent->entity.ee_entity.saPlmEEReadinessFlags = 
						PLMS_READINESS_FLAG_RESET;
			
			ent->mngt_lost_tri = PLMS_MNGT_NONE;	
			ent->iso_method = PLMS_ISO_DEFAULT;

			TRACE("Readiness Flags cleared for the entity %s",
			ent->dn_name_str);

			/* Update IMM.*/
			ret_val = plms_attr_imm_update(ent,
					"saPlmEEReadinessFlags",
					SA_IMM_ATTR_SAUINT64T,
					PLMS_READINESS_FLAG_RESET);
			if (NCSCC_RC_SUCCESS != ret_val){
				LOG_ER("Cleared readiness flag for the ent %s,\
				Updating IMM FAILED",ent->dn_name_str);
			}

			ntf = 1;
			ret_err = NCSCC_RC_SUCCESS;
		}	
	
	}
	
	/* Send notification.*/
	if (ntf){
		ntf_state.next = NULL;
		ntf_state.state.stateId = SA_PLM_READINESS_FLAGS;
		ntf_state.state.oldStatePresent = TRUE;
		ntf_state.state.oldState = old_flag;
		ntf_state.state.newState = PLMS_READINESS_FLAG_RESET;
		ret_val = plms_state_change_ntf_send(cb->ntf_hdl,&(ent->dn_name),
				&ntf_id,src_ind,1/*no_of_st_change*/,&ntf_state,
				minor_id,0/*no_cor_ids*/,NULL/*cor_ids*/,
				(NULL != root_ent)?(&(root_ent->dn_name)):NULL);
		if (NCSCC_RC_SUCCESS != ret_val){
			LOG_ER("Cleared readiness flag for the ent %s,\
				Sending notification FAILED.",ent->dn_name_str);
		}
	}

	return ntf_id;
}
/******************************************************************************
@brief		: Clear readiness flag.
@param[in]	: ent - Entity whose readiness flag is to be marked/unmarked.
@param[in]	: flag - Flag to be marked/unmarked.
@param[in]	: mark_unmark - 0 ==> Unmark
				1 ==> Mark
@param[in]	: root_ent - Root entity for which readiness flag to be marked/
		unmarked of the ent.
@param[in]	: src_ind - Source indicator to be used for notification.
@param[in]	: minor_id - Minor id to be used to send notification.

@return		: SA_NTF_IDENTIFIER_UNUSED - If notification not sent.
		SaNtfIdentifierT - Notfication id.	
******************************************************************************/
SaNtfIdentifierT plms_readiness_flag_mark_unmark(PLMS_ENTITY *ent,
					SaPlmReadinessFlagsT flag,
					SaBoolT mark_unmark,
					PLMS_ENTITY *root_ent,
					SaUint16T src_ind,
					SaUint16T minor_id)
{
	SaUint32T he_mark_unmark = 0;
	SaUint32T ee_mark_unmark = 0;
	SaPlmReadinessFlagsT flag_imm;
	SaPlmReadinessFlagsT old_flag;
	SaUint32T ntf = 0;
	PLMS_CB *cb = plms_cb;
	SaNtfIdentifierT ntf_id = SA_NTF_IDENTIFIER_UNUSED;
	SaPlmNtfStateChangeT ntf_state;
	SaUint32T ret_err;

	switch(mark_unmark){
		case 0:
			/* if the flag is set, then clear.*/
			if (PLMS_HE_ENTITY == ent->entity_type){
				if ((flag & 
				ent->entity.he_entity.saPlmHEReadinessFlags) == 
				flag ){
					old_flag = ent->entity.he_entity.
							saPlmHEReadinessFlags;
							
					ent->entity.he_entity.
					saPlmHEReadinessFlags = 
					(ent->entity.he_entity.
					saPlmHEReadinessFlags & ~flag);
					
					flag_imm = ent->entity.he_entity.
					saPlmHEReadinessFlags;	
					
					TRACE("Readiness flag unmarked.Ent: %s,\
					New_flag: %llu, Flag unmarked: %llu",
					ent->dn_name_str,flag_imm,flag);

					he_mark_unmark = 1;
					ntf = 1;
				}
			}else{
				if ((flag & 
				ent->entity.ee_entity.saPlmEEReadinessFlags) == 
				flag ){
					old_flag = ent->entity.ee_entity.
							saPlmEEReadinessFlags;
							
					ent->entity.ee_entity.
					saPlmEEReadinessFlags = 
					(ent->entity.ee_entity.
					saPlmEEReadinessFlags & ~flag);
					
					flag_imm = ent->entity.ee_entity.
					saPlmEEReadinessFlags;	
					
					TRACE("Readiness flag unmarked.Ent: %s,\
					New_flag: %llu, Flag unmarked: %llu",
					ent->dn_name_str,flag_imm,flag);

					ee_mark_unmark = 1;
					ntf = 1;
				}
			}
			break;
		case 1:
			/* if the flag is not set, then set.*/
			if (PLMS_HE_ENTITY == ent->entity_type){
				if ((flag & 
				ent->entity.he_entity.saPlmHEReadinessFlags) != 
				flag ){
					old_flag = ent->entity.he_entity.
							saPlmHEReadinessFlags;

					ent->entity.he_entity.
					saPlmHEReadinessFlags = 
					(ent->entity.he_entity.
					saPlmHEReadinessFlags | flag);
				
					flag_imm = ent->entity.he_entity.
					saPlmHEReadinessFlags;	
					
					TRACE("Readiness flag marked.Ent: %s,\
					New_flag: %llu, Flag marked: %llu",
					ent->dn_name_str,flag_imm,flag);
					
					he_mark_unmark = 1;
					ntf = 1;
				}
			}else{
				if ((flag & 
				ent->entity.ee_entity.saPlmEEReadinessFlags) != 
				flag ){
					old_flag = ent->entity.ee_entity.
							saPlmEEReadinessFlags;
					
					ent->entity.ee_entity.
					saPlmEEReadinessFlags = 
					(ent->entity.ee_entity.
					saPlmEEReadinessFlags | flag);

					flag_imm = ent->entity.ee_entity.
					saPlmEEReadinessFlags;	
					
					TRACE("Readiness flag marked.Ent: %s,\
					New_flag: %llu, Flag marked: %llu",
					ent->dn_name_str,flag_imm,flag);

					ee_mark_unmark = 1;
					ntf = 1;
				}
			}
			break;
		default:
			return ntf_id;
	}
	/* IMM updation.*/
	if (he_mark_unmark){
		/* Update IMM.*/
		ret_err = plms_attr_imm_update(ent,"saPlmHEReadinessFlags",
					SA_IMM_ATTR_SAUINT64T,flag_imm);
		if (NCSCC_RC_SUCCESS != ret_err){
			LOG_ER("Readiness Flag updated, updating IMM FAILED.\
			Ent: %s, New_flag: %llu, Flag unmarked: %llu",
			ent->dn_name_str,flag_imm,flag);
		}
	
	}else if (ee_mark_unmark){
		/* Update IMM.*/
		ret_err = plms_attr_imm_update(ent,"saPlmEEReadinessFlags",
					SA_IMM_ATTR_SAUINT64T,flag_imm);
		if (NCSCC_RC_SUCCESS != ret_err){
			LOG_ER("Readiness Flag updated, updating IMM FAILED.\
			Ent: %s, New_flag: %llu, Flag unmark: %llu",
			ent->dn_name_str,flag_imm,flag);
		}

	}else{
	}
	/* Send notification.*/
	if (ntf){
		ntf_state.next = NULL;
		ntf_state.state.stateId = SA_PLM_READINESS_FLAGS;
		ntf_state.state.oldStatePresent = TRUE;
		ntf_state.state.oldState = old_flag;
		ntf_state.state.newState = flag_imm;
		ret_err = plms_state_change_ntf_send(cb->ntf_hdl,&(ent->dn_name),
				&ntf_id,src_ind,1/*no_of_st_change*/,&ntf_state,
				minor_id,0/*no_cor_ids*/, NULL/*cor_ids*/,
				(NULL != root_ent)?(&(root_ent->dn_name)):NULL);

		if (NCSCC_RC_SUCCESS != ret_err){
			LOG_ER("Readiness Flag updated, Sending notification \
			FAILED.  Ent: %s, mark/unmark: %d, New Flag: %llu, \
			Flag: %llu",ent->dn_name_str,mark_unmark,flag_imm,flag);
		}
	}
	return ntf_id;
}
/******************************************************************************
@brief		: Set the readiness state if not set.
@param[in]	: ent - Entity whose readiness state is to be set.
@param[in]	: state - Readiness state to be set.
@param[in]	: root_ent - Root entity for which readiness state to be set
		for the ent.
@param[in]	: src_ind - Source indicator to be used for notification.
@param[in]	: minor_id - Minor id to be used to send notification.

@return		: SA_NTF_IDENTIFIER_UNUSED - If notification not sent.
		SaNtfIdentifierT - Notfication id.	
******************************************************************************/
SaNtfIdentifierT plms_readiness_state_set(PLMS_ENTITY *ent,
				SaPlmReadinessStateT state,
				PLMS_ENTITY *root_ent,
				SaUint16T src_ind,
				SaUint16T minor_id)
{
	SaPlmNtfStateChangeT ntf_state;
	SaUint32T ntf=0,ret_err;
	SaUint32T old_state;
	SaNtfIdentifierT ntf_id = SA_NTF_IDENTIFIER_UNUSED;
	PLMS_CB *cb = plms_cb;
	
	if (PLMS_HE_ENTITY == ent->entity_type){
		if (state != ent->entity.he_entity.saPlmHEReadinessState){
			
			old_state = ent->entity.he_entity.saPlmHEReadinessState;
			
			ent->entity.he_entity.saPlmHEReadinessState = state;
			
			TRACE("Readiness state set. Ent: %s, New_state: %d, \
			Old_State: %d",ent->dn_name_str,state,old_state); 
			
			/* Update IMM.*/
			ret_err = plms_attr_imm_update(ent,
						"saPlmHEReadinessState",
						SA_IMM_ATTR_SAUINT32T,state);
			if (NCSCC_RC_SUCCESS != ret_err){
				LOG_ER("Readiness state updated, Updating IMM \
				FAILED. Ent: %s, New St: %d, Old St: %d",
				ent->dn_name_str,state,old_state);
			}
			ntf = 1;	
		}
	}else{
		if (state != ent->entity.ee_entity.saPlmEEReadinessState){
			
			old_state = ent->entity.ee_entity.saPlmEEReadinessState;
			ent->entity.ee_entity.saPlmEEReadinessState = state;
			
			TRACE("Readiness state set. Ent: %s, New_state: %d, \
			Old_State: %d",ent->dn_name_str,state,old_state); 
			
			/* Update IMM.*/
			ret_err = plms_attr_imm_update(ent,
						"saPlmEEReadinessState",
						SA_IMM_ATTR_SAUINT32T,state);
			if (NCSCC_RC_SUCCESS != ret_err){
				LOG_ER("Readiness state updated, Updating IMM \
				FAILED. Ent: %s, New St: %d, Old St: %d",
				ent->dn_name_str,state,old_state);
			}
			
			ntf = 1;
		}
	}

	if (ntf){
		ntf_state.next = NULL;
		ntf_state.state.stateId = SA_PLM_READINESS_STATE;
		ntf_state.state.oldStatePresent = TRUE;
		ntf_state.state.oldState = old_state;
		ntf_state.state.newState = state;
		ret_err = plms_state_change_ntf_send(cb->ntf_hdl,&(ent->dn_name),
				&ntf_id,src_ind,1/*no_of_st_change*/,&ntf_state,
				minor_id,0/*no_cor_ids*/, NULL/*cor_ids*/,
				(NULL != root_ent)?(&(root_ent->dn_name)):NULL);
		if (NCSCC_RC_SUCCESS != ret_err){
			LOG_ER("Readiness state updated, Sending notification \
			FAILED. Ent: %s, New St: %d, Old St: %d",
			ent->dn_name_str,state,old_state);
		}
	}
	return ntf_id;
}
/******************************************************************************
@brief		: Check if the mentioned readiness state is set or not.

@param[in]	: ent - Entity whose readiness state if to be checked.
@param[in]	: state - Readiness state to be checked.

@return		: TRUE - If set.
		  FALSE - If not set.	
******************************************************************************/
SaUint32T plms_is_rdness_state_set(PLMS_ENTITY *ent,SaPlmReadinessStateT state)
{
	SaUint32T ret_err;
	
	if (PLMS_HE_ENTITY == ent->entity_type){
		if (state != ent->entity.he_entity.saPlmHEReadinessState){
			ret_err = FALSE;
		}else{
			ret_err = TRUE;
		}
	}else{
		if (state != ent->entity.ee_entity.saPlmEEReadinessState){
			ret_err = FALSE;
		}else{
			ret_err = TRUE;
		}
	}
	return ret_err;
}

/******************************************************************************
@brief		: Set the expected readiness status of the entities.

@param[in]	: aff_ent_list - List of affected entities.
@param[in]	: rdyness_state - Readiness state to be marked.
@param[in]	: flag - Readiness flag to be marked.
******************************************************************************/
void plms_aff_ent_exp_rdness_state_mark(PLMS_GROUP_ENTITY *aff_ent_list,
					SaPlmHEAdminStateT rdyness_state,
					SaPlmReadinessFlagsT flag)
{
	PLMS_GROUP_ENTITY *head;

	head = aff_ent_list;
	while (head){
		head->plm_entity->exp_readiness_status.readinessState =
								rdyness_state;

		if (PLMS_HE_ENTITY == head->plm_entity->entity_type){
			head->plm_entity->exp_readiness_status.readinessFlags =
			(head->plm_entity->entity.he_entity.
			saPlmHEReadinessFlags | flag);
		}else{
			head->plm_entity->exp_readiness_status.readinessFlags =
			(head->plm_entity->entity.ee_entity.
			saPlmEEReadinessFlags | flag);
		}
		head = head->next;
	}
	return;
}
/******************************************************************************
@brief		: Clear the expected readiness status of the affected entities.
@param[in]	: aff_ent_list - List of affected entities.
******************************************************************************/
void plms_aff_ent_exp_rdness_status_clear(PLMS_GROUP_ENTITY *aff_ent_list)
{
	PLMS_GROUP_ENTITY *head;

	head = aff_ent_list;
	while (head){
		head->plm_entity->exp_readiness_status.readinessState = 0;
		head->plm_entity->exp_readiness_status.readinessFlags = 0;
		head = head->next;
	}
	return;
}
/******************************************************************************
@brief		: Clear the expected readiness status of the ent. 
@param[in]	: aff_ent_list - Entity whose expected readiness status is to
		be cleared.
******************************************************************************/
void plms_ent_exp_rdness_status_clear(PLMS_ENTITY *ent)
{
	ent->exp_readiness_status.readinessState = 0;
	ent->exp_readiness_status.readinessFlags = 0;	
	return;
}
/******************************************************************************
@brief		: Overwrite the expected readiness state of the affected entities
		with the current readiness status.
@param[in]	: aff_ent_list - List of affected entities.		
******************************************************************************/
void plms_aff_ent_exp_rdness_state_ow(PLMS_GROUP_ENTITY *aff_ent_list)
{
	PLMS_GROUP_ENTITY *ent_head;
	
	ent_head = aff_ent_list;
	while (ent_head){
		if (PLMS_HE_ENTITY == ent_head->plm_entity->entity_type){
			ent_head->plm_entity->
			exp_readiness_status.readinessState = 
			ent_head-> plm_entity->entity.he_entity.
			saPlmHEReadinessState;
			
			ent_head->plm_entity->
			exp_readiness_status.readinessFlags = 
			ent_head-> plm_entity->entity.he_entity.
			saPlmHEReadinessFlags;
			
		}else {
			ent_head->plm_entity->
			exp_readiness_status.readinessState = 
			ent_head-> plm_entity->entity.ee_entity.
			saPlmEEReadinessState;
			
			ent_head->plm_entity->
			exp_readiness_status.readinessFlags = 
			ent_head->plm_entity->entity.ee_entity.
			saPlmEEReadinessFlags;
		}
		ent_head = ent_head->next;
	}
	return;
}
/******************************************************************************
@brief		: Overwrite the expected readiness state of the ent
		with the current readiness status.
@param[in]	: ent - Entity whose expected readiness status is to be 
		overwritten.
******************************************************************************/
void plms_ent_exp_rdness_state_ow(PLMS_ENTITY *ent)
{
	
	if (PLMS_HE_ENTITY == ent->entity_type){
		
		ent->exp_readiness_status.readinessState = 
		ent->entity.he_entity.saPlmHEReadinessState;
		
		ent->exp_readiness_status.readinessFlags = 
		ent->entity.he_entity.saPlmHEReadinessFlags;
		
	}else {
		ent->exp_readiness_status.readinessState = 
		ent->entity.ee_entity.saPlmEEReadinessState;
		
		ent->exp_readiness_status.readinessFlags = 
		ent->entity.ee_entity.saPlmEEReadinessFlags;
	}

	return;
}

/******************************************************************************
@brief		: Set the operational state if not set.
@param[in]	: ent - Entity whose operational state is to be set.
@param[in]	: state - operational state to be set.
@param[in]	: root_ent - Root entity for which operational state to be set
		for the ent.
@param[in]	: src_ind - Source indicator to be used for notification.
@param[in]	: minor_id - Minor id to be used to send notification.

@return		: SA_NTF_IDENTIFIER_UNUSED - If notification not sent.
		SaNtfIdentifierT - Notfication id.	
******************************************************************************/
SaNtfIdentifierT plms_op_state_set(PLMS_ENTITY *ent,
				SaPlmOperationalStateT state,
				PLMS_ENTITY *root_ent,
				SaUint16T src_ind,
				SaUint16T minor_id)
{
	SaUint32T ret_err;
	SaPlmNtfStateChangeT ntf_state;
	SaUint32T ntf=0;
	SaUint32T old_state;
	PLMS_CB *cb = plms_cb;
	SaNtfIdentifierT ntf_id = SA_NTF_IDENTIFIER_UNUSED;

	if (PLMS_HE_ENTITY == ent->entity_type){
		if (state != ent->entity.he_entity.saPlmHEOperationalState){

			old_state=ent->entity.he_entity.saPlmHEOperationalState;
			ent->entity.he_entity.saPlmHEOperationalState = state;
			
			TRACE("Operational state set. Ent: %s, New_state: %d, \
			Old_State: %d",ent->dn_name_str,state,old_state); 
			
			ret_err = plms_attr_imm_update(ent,
						"saPlmHEOperationalState",
						SA_IMM_ATTR_SAUINT32T,state);
			if (NCSCC_RC_SUCCESS != ret_err){
				LOG_ER("Operational state updated, IMM update \
				FAILED. Ent: %s, New St: %d, Old St: %d",
				ent->dn_name_str,state,old_state);	
			}
			ntf = 1;
		}
	}else{
		if (state != ent->entity.ee_entity.saPlmEEOperationalState){
			
			old_state=ent->entity.ee_entity.saPlmEEOperationalState;
			ent->entity.ee_entity.saPlmEEOperationalState = state;
			
			TRACE("Operational state set. Ent: %s, New_state: %d, \
			Old_State: %d",ent->dn_name_str,state,old_state); 

			ret_err = plms_attr_imm_update(ent,
						"saPlmEEOperationalState",
						SA_IMM_ATTR_SAUINT32T,state);
			if (NCSCC_RC_SUCCESS != ret_err){
				LOG_ER("Operational state updated, IMM update \
				FAILED. Ent: %s, New St: %d, Old St: %d",
				ent->dn_name_str,state,old_state);	
			}

			ntf = 1;
		}
	}
	
	if (ntf){
		ntf_state.next = NULL;
		ntf_state.state.stateId = SA_PLM_OPERATIONAL_STATE;
		ntf_state.state.oldStatePresent = TRUE;
		ntf_state.state.oldState = old_state;
		ntf_state.state.newState = state;
		ret_err = plms_state_change_ntf_send(cb->ntf_hdl,&(ent->dn_name),
				&ntf_id,src_ind,1/*no_of_st_change*/,&ntf_state,
				minor_id,0/*no_cor_ids*/, NULL/*cor_ids*/,
				(NULL != root_ent)?(&(root_ent->dn_name)):NULL);
		if (NCSCC_RC_SUCCESS != ret_err){
			LOG_ER("Operational state updated, Sending \
			notification FAILED. Ent: %s, New St: %d, Old St: %d",
			ent->dn_name_str,state,old_state);	
		}
	}
	return ntf_id;
}

/******************************************************************************
@brief		: Set the Admin state if not set.
@param[in]	: ent - Entity whose Admin state is to be set.
@param[in]	: state - Admin state to be set.
@param[in]	: root_ent - Root entity for which Admin state to be set
		for the ent.
@param[in]	: src_ind - Source indicator to be used for notification.
@param[in]	: minor_id - Minor id to be used to send notification.

@return		: SA_NTF_IDENTIFIER_UNUSED - If notification not sent.
		SaNtfIdentifierT - Notfication id.	
******************************************************************************/
SaNtfIdentifierT plms_admin_state_set(PLMS_ENTITY *ent, SaUint32T state,
				PLMS_ENTITY *root_ent,
				SaUint16T src_ind,
				SaUint16T minor_id)
{
	SaUint32T ret_err;
	SaPlmNtfStateChangeT ntf_state;
	SaUint32T ntf=0;
	SaUint64T state_id;
	SaUint32T old_state;
	PLMS_CB *cb = plms_cb;
	SaNtfIdentifierT ntf_id = SA_NTF_IDENTIFIER_UNUSED;

	if (PLMS_HE_ENTITY == ent->entity_type){
		if ((SaPlmHEAdminStateT)state != 
				ent->entity.he_entity.saPlmHEAdminState){
			
			old_state = ent->entity.he_entity.saPlmHEAdminState;
			state_id = SA_PLM_HE_ADMIN_STATE;

			ent->entity.he_entity.saPlmHEAdminState = 
						(SaPlmHEAdminStateT)state;
			
			TRACE("Admin state set. Ent: %s, New_state: %d, \
			Old_State: %d",ent->dn_name_str,state,old_state); 
			
			/* Update IMM.*/
			ret_err = plms_attr_imm_update(ent,"saPlmHEAdminState",
						SA_IMM_ATTR_SAUINT32T,state);
			if (NCSCC_RC_SUCCESS != ret_err){
				LOG_ER("Admin state updated, IMM update \
				FAILED. Ent: %s, New St: %d, Old St: %d",
				ent->dn_name_str,state,old_state);	
			}
			ntf = 1;
		}
	}else{
		if ((SaPlmEEAdminStateT)state != 
				ent->entity.ee_entity.saPlmEEAdminState){
			
			old_state = ent->entity.ee_entity.saPlmEEAdminState;
			state_id = SA_PLM_EE_ADMIN_STATE;
			
			ent->entity.ee_entity.saPlmEEAdminState = 
						(SaPlmEEAdminStateT)state;
			
			TRACE("Admin state set. Ent: %s, New_state: %d, \
			Old_State: %d",ent->dn_name_str,state,old_state); 

			/* Update IMM.*/
			ret_err = plms_attr_imm_update(ent,"saPlmEEAdminState",
						SA_IMM_ATTR_SAUINT32T,state);
			if (NCSCC_RC_SUCCESS != ret_err){
				LOG_ER("Admin state updated, IMM update  \
				FAILED. Ent: %s, New St: %d, Old St: %d",
				ent->dn_name_str,state,old_state);	
			}
			ntf = 1;
		}
	}
	
	if (ntf){
		ntf_state.next = NULL;
		ntf_state.state.stateId = state_id; 
		ntf_state.state.oldStatePresent = TRUE;
		ntf_state.state.oldState = old_state;
		ntf_state.state.newState = state;
		ret_err = plms_state_change_ntf_send(cb->ntf_hdl,&(ent->dn_name),
				&ntf_id,src_ind,1/*no_of_st_change*/,&ntf_state,
				minor_id,0/*no_cor_ids*/, NULL/*cor_ids*/,
				(NULL != root_ent)?(&(root_ent->dn_name)):NULL);
		if (NCSCC_RC_SUCCESS != ret_err){
			LOG_ER("Admin state updated, Sending notification \
			FAILED. Ent: %s,  New St: %d, Old St: %d",
			ent->dn_name_str,state,old_state);	
		}
	}
	return ntf_id;
}

/******************************************************************************
@brief		: Set the presence state if not set.
@param[in]	: ent - Entity whose presence state is to be set.
@param[in]	: state - presence state to be set.
@param[in]	: root_ent - Root entity for which presence state to be set
		for the ent.
@param[in]	: src_ind - Source indicator to be used for notification.
@param[in]	: minor_id - Minor id to be used to send notification.

@return		: SA_NTF_IDENTIFIER_UNUSED - If notification not sent.
		SaNtfIdentifierT - Notfication id.	
******************************************************************************/
SaNtfIdentifierT plms_presence_state_set(PLMS_ENTITY *ent, SaUint32T state,
				PLMS_ENTITY *root_ent,
				SaUint16T src_ind,
				SaUint16T minor_id)
{
	SaUint32T ret_err;
	SaPlmNtfStateChangeT ntf_state;
	SaUint32T ntf=0;
	SaUint64T state_id;
	SaUint32T old_state;	
	PLMS_CB *cb = plms_cb;
	SaNtfIdentifierT ntf_id = SA_NTF_IDENTIFIER_UNUSED;

	if (PLMS_HE_ENTITY == ent->entity_type){
		if ((SaPlmHEPresenceStateT)state != 
				ent->entity.he_entity.saPlmHEPresenceState){
			
			old_state =  ent->entity.he_entity.saPlmHEPresenceState;
			state_id = SA_PLM_HE_PRESENCE_STATE;

			ent->entity.he_entity.saPlmHEPresenceState = 
						(SaPlmHEPresenceStateT)state;
			
			TRACE("Presence state set. Ent: %s, New_state: %d, \
			Old_State: %d",ent->dn_name_str,state,old_state); 
			
			/* Update IMM.*/
			ret_err=plms_attr_imm_update(ent,"saPlmHEPresenceState",
						SA_IMM_ATTR_SAUINT32T,state);
			if (NCSCC_RC_SUCCESS != ret_err){
				LOG_ER("Presence state updated, IMM update \
				FAILED. Ent: %s, New St: %d, Old St: %d",
				ent->dn_name_str,state,old_state);	
			}

			ntf = 1;

		}
	}else{
		if ((SaPlmEEPresenceStateT)state != 
				ent->entity.ee_entity.saPlmEEPresenceState){

			/* TODO: If the new state is uninstantiated then should
			I stop the instantiated-failed timer if running.*/
			
			old_state = ent->entity.ee_entity.saPlmEEPresenceState;
			state_id = SA_PLM_EE_PRESENCE_STATE;

			ent->entity.ee_entity.saPlmEEPresenceState = 
						(SaPlmEEPresenceStateT)state;
			
			TRACE("Presence state set. Ent: %s, New_state: %d, \
			Old_State: %d",ent->dn_name_str,state,old_state); 

			/* Update IMM.*/
			ret_err=plms_attr_imm_update(ent,"saPlmEEPresenceState",
						SA_IMM_ATTR_SAUINT32T,state);
			if (NCSCC_RC_SUCCESS != ret_err){
				LOG_ER("Presence state updated, IMM update \
				FAILED. Ent: %s, New St: %d, Old St: %d",
				ent->dn_name_str,state,old_state);	
			}

			ntf = 1;
		}
	}
	if (ntf){
		ntf_state.next = NULL;
		ntf_state.state.stateId = state_id; 
		ntf_state.state.oldStatePresent = TRUE;
		ntf_state.state.oldState = old_state;
		ntf_state.state.newState = state;
		ret_err = plms_state_change_ntf_send(cb->ntf_hdl,&(ent->dn_name),
				&ntf_id,src_ind,1/*no_of_st_change*/,&ntf_state,
				minor_id,0/*no_cor_ids*/, NULL/*cor_ids*/,
				(NULL != root_ent)?(&(root_ent->dn_name)):NULL);
		
		if (NCSCC_RC_SUCCESS != ret_err){
			
			LOG_ER("Presence state updated, NTF Sent \
			FAILED. Ent: %s, New_st: %d, Old_st: %d",
			ent->dn_name_str,state,old_state);	
		}
	}
	return ntf_id;
}

/******************************************************************************
@brief		: Update integer type IMM object atrribute. This is a specific
		function for integer type of attribute and modification method
		REPLACE. This is not a general function.

@param[in]	: ent - Attribute of this entity is to be updated.
@param[in]	: attr_name - Attribute name.
@param[in]	: val_type - Type of attribute. It has to be of integer type.
@param[in]	: val - Attribute val.

@return		: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
******************************************************************************/
SaUint32T plms_attr_imm_update(PLMS_ENTITY *ent,SaInt8T *attr_name,
				SaImmValueTypeT val_type,SaUint64T val)
{
	SaAisErrorT err;
	SaImmAttrModificationT_2 temp;
	const SaImmAttrModificationT_2 *imm_attr[2];
	SaUint64T *value;
	PLMS_CB *cb = plms_cb;

	temp.modType = SA_IMM_ATTR_VALUES_REPLACE;
	temp.modAttr.attrName = attr_name;
	temp.modAttr.attrValueType = val_type;
	temp.modAttr.attrValuesNumber = 1;
	value = &val;
	temp.modAttr.attrValues = (void **)&value;

	imm_attr[0] = &temp;
	imm_attr[1] = NULL;

	err = immutil_saImmOiRtObjectUpdate_2(cb->oi_hdl,&(ent->dn_name),imm_attr);
	if (SA_AIS_OK != err){
		LOG_ER("IMM Updation FAILED. Ret_val: %d",err);
		return NCSCC_RC_FAILURE;
	}
	
	TRACE("IMM atribute updated. Ent: %s, attr_name: %s, val: %llu",
	ent->dn_name_str,attr_name,val);
	
	return NCSCC_RC_SUCCESS;
}
/******************************************************************************
@brief		: Update SaName type IMM object atrribute.

@param[in]	: ent - Attribute of this entity is to be updated.
@param[in]	: attr_name - Attribute name.
@param[in]	: val - Attribute val.
@param[in]	: mod_type - IMM modification type.

@return		: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
******************************************************************************/
SaUint32T plms_attr_saname_imm_update(PLMS_ENTITY *ent,
SaInt8T *attr_name,SaNameT val,SaImmAttrModificationTypeT mod_type)
{
	SaAisErrorT err;
	SaImmAttrModificationT_2 temp;
	const SaImmAttrModificationT_2 *imm_attr[2];
	SaNameT *value;
	SaInt8T he_type[SA_MAX_NAME_LENGTH +1];
	PLMS_CB *cb = plms_cb;
	
	plms_get_str_from_dn_name(&val,he_type);

	temp.modType = mod_type;
	temp.modAttr.attrName = attr_name;
	temp.modAttr.attrValueType = SA_IMM_ATTR_SANAMET;
	temp.modAttr.attrValuesNumber = 1;
	value = &val;
	temp.modAttr.attrValues = (void **)&value;

	imm_attr[0] = &temp;
	imm_attr[1] = NULL;

	err = immutil_saImmOiRtObjectUpdate_2(cb->oi_hdl,&(ent->dn_name),imm_attr);
	if (SA_AIS_OK != err){
		LOG_ER("IMM attr update failed.Ent: %s, attr_name: %s, val: %s\
		,ret_val: %d",ent->dn_name_str,attr_name,he_type,err);
		return NCSCC_RC_FAILURE;
	}
	
	TRACE("IMM atribute updated. Ent: %s, attr_name: %s, val: %s",
	ent->dn_name_str,attr_name,he_type);
	
	return NCSCC_RC_SUCCESS;
}
/******************************************************************************
@brief		: Update SaString type IMM object atrribute.

@param[in]	: ent - Attribute of this entity is to be updated.
@param[in]	: attr_name - Attribute name.
@param[in]	: val - Attribute val.
@param[in]	: mod_type - IMM modification type.

@return		: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
******************************************************************************/
SaUint32T plms_attr_sastring_imm_update(PLMS_ENTITY *ent,
	SaInt8T *attr_name,SaStringT val,SaImmAttrModificationTypeT mod_type)
{
	SaAisErrorT err;
	SaImmAttrModificationT_2 temp;
	const SaImmAttrModificationT_2 *imm_attr[2];
	SaStringT *value;
	PLMS_CB *cb = plms_cb;

	temp.modType = mod_type;
	temp.modAttr.attrName = attr_name;
	temp.modAttr.attrValueType = SA_IMM_ATTR_SASTRINGT;
	temp.modAttr.attrValuesNumber = 1;
	value = &val;
	temp.modAttr.attrValues = (void **)&value;

	imm_attr[0] = &temp;
	imm_attr[1] = NULL;

	err = immutil_saImmOiRtObjectUpdate_2(cb->oi_hdl,&(ent->dn_name),imm_attr);
	if (SA_AIS_OK != err){
		LOG_ER("IMM Updation FAILED. Ret_val: %d",err);
		return NCSCC_RC_FAILURE;
	}
	
	TRACE("IMM atribute updated. Ent: %s, attr_name: %s, val: %s",
	ent->dn_name_str,attr_name,val);
	
	return NCSCC_RC_SUCCESS;
}
/******************************************************************************
@brief		: Mark/Unmark the am_i_aff_ent flag of the entities.

@param[in]	: ent_list - List of entities.
@param[in]	: flag - 0 or 1
******************************************************************************/
void plms_aff_ent_mark_unmark(PLMS_GROUP_ENTITY *ent_list, SaBoolT flag)
{
	PLMS_GROUP_ENTITY *head;

	head = ent_list;
	while(head){
		head->plm_entity->am_i_aff_ent = flag;
		head = head->next;
	}
	return;
}
/******************************************************************************
@brief		: Free trk_info.
@param[in]	: trk_info - structure to be freed.
******************************************************************************/
void plms_trk_info_free(PLMS_TRACK_INFO *trk_info)
{
	if (NULL == trk_info)
		return;
	
	if (trk_info->group_info_list)
		plms_ent_grp_list_free(trk_info->group_info_list);
	if (trk_info->aff_ent_list)	
		plms_ent_list_free(trk_info->aff_ent_list);
	
	trk_info->group_info_list = NULL;
	trk_info->aff_ent_list = NULL;
	if (trk_info->root_entity)
		trk_info->root_entity->trk_info = NULL;
	free(trk_info);
	return;
}

/******************************************************************************
@brief		: Check if the ent is persent in list.
@param[in]	: list - List of entities.
@param[in]	: ent - Entity to be checked for.
@return		: TRUE - If ent is present in list.
		  FALSE - If ent is not present in list.
******************************************************************************/
SaBoolT plms_is_ent_in_ent_list(PLMS_GROUP_ENTITY *list,
				PLMS_ENTITY *ent)
{
	PLMS_GROUP_ENTITY *tail;

	tail = list;
	while (tail){
		if ( 0 == strcmp(tail->plm_entity->dn_name_str,ent->dn_name_str))
			return TRUE;
		tail = tail->next;	
	}
	return FALSE;
}

/******************************************************************************
@brief		: Take the ent to insvc if the following conditions are matched
		1. All the ancestors are in svc.
		2. It satisfies the min dependency criteria.
		3. Its admin state is unlock.
		4. Its operational state is enable.
		5. Its presence state is either instantiated or terminating(for
		EE) and active or deactivating(for HE).
@param[in]	: chld_ent - Entity to be taken to insvc.
@param[out]	: is_flag_aff - If the readiness flag is affected, then this
		will have value 1, otherwise 0.
@return		: NCSCC_RC_FAILURE/NCSCC_RC_SUCCESS		
******************************************************************************/
SaUint32T plms_move_ent_to_insvc(PLMS_ENTITY *chld_ent, SaUint8T *is_flag_aff)
{
	*is_flag_aff = 0;

	/* If adm state is not unlock. */	
	if ( (PLMS_HE_ENTITY == chld_ent->entity_type) && 
		(SA_PLM_HE_ADMIN_UNLOCKED != 
		chld_ent->entity.he_entity.saPlmHEAdminState )){
		LOG_ER("Entity %s can not be moved to insvc, as admin \
		state is not unlocked.",chld_ent->dn_name_str);
		return NCSCC_RC_FAILURE;
	}else if ((PLMS_EE_ENTITY == chld_ent->entity_type) &&
		(SA_PLM_EE_ADMIN_UNLOCKED != 
		chld_ent->entity.ee_entity.saPlmEEAdminState)){
		LOG_ER("Entity %s can not be moved to insvc, as admin\
		state is not unlocked.",chld_ent->dn_name_str);
		return NCSCC_RC_FAILURE;
	}
	
	/* If the operational state is disabled. */
	if ( (PLMS_HE_ENTITY == chld_ent->entity_type) &&
		(SA_PLM_OPERATIONAL_DISABLED == 
		chld_ent->entity.he_entity.saPlmHEOperationalState)){
		LOG_ER("Entity %s can not be moved to insvc, as operational \
		state is disabled.",chld_ent->dn_name_str);
		return NCSCC_RC_FAILURE;
	}else if ((PLMS_EE_ENTITY == chld_ent->entity_type) && 
		(SA_PLM_OPERATIONAL_DISABLED == 
		chld_ent->entity.ee_entity.saPlmEEOperationalState)){
		LOG_ER("Entity %s can not be moved to insvc, as operational \
		state is disabled.",chld_ent->dn_name_str);
		return NCSCC_RC_FAILURE;
	}
	
	/* If the presence state of the HE is either active or deactivating
	*/
	if ((PLMS_HE_ENTITY == chld_ent->entity_type) && 
		(( SA_PLM_HE_PRESENCE_ACTIVE !=
		chld_ent->entity.he_entity.saPlmHEPresenceState) && 
		(SA_PLM_HE_PRESENCE_DEACTIVATING != 
		chld_ent->entity.he_entity.saPlmHEPresenceState))){
		LOG_ER("Entity %s can not be moved to insvc, as presence \
		state is neither active nor deacting",chld_ent->dn_name_str);
		return NCSCC_RC_FAILURE;
		/* If the presence state of the EE is either instantiated or
		terminating.*/	
	}else if ((PLMS_EE_ENTITY == chld_ent->entity_type) &&
		(SA_PLM_EE_PRESENCE_INSTANTIATED != 
		chld_ent->entity.ee_entity.saPlmEEPresenceState) && 
		(SA_PLM_EE_PRESENCE_TERMINATING != 
		chld_ent->entity.ee_entity.saPlmEEPresenceState)){
		LOG_ER("Entity %s can not be moved to insvc, as presence \
		state is neither active nor deacting",chld_ent->dn_name_str);
		return NCSCC_RC_FAILURE;
	}
	
	/* If my parent is OOS, then forget. Return from here.*/
	if ((NULL != chld_ent->parent) && 
		!plms_is_rdness_state_set(chld_ent->parent,
		SA_PLM_READINESS_IN_SERVICE)){
		
		if (!plms_rdness_flag_is_set(chld_ent,
					SA_PLM_RF_DEPENDENCY)){
			
			plms_readiness_flag_mark_unmark(chld_ent,
					SA_PLM_RF_DEPENDENCY,1/*mark*/,NULL,
					SA_NTF_OBJECT_OPERATION,
					SA_PLM_NTFID_STATE_CHANGE_ROOT);

			*is_flag_aff = 1;

		}
		LOG_ER("Entity %s can not be moved to insvc, as parent is \
		not in service",chld_ent->dn_name_str);
		return NCSCC_RC_FAILURE;
		
	}
	/* If the min dependency condition is matched. 
	Clear the dependecy flag.*/
	if (plms_min_dep_is_ok(chld_ent)){
		if (plms_rdness_flag_is_set(chld_ent,
						SA_PLM_RF_DEPENDENCY)){

			plms_readiness_flag_mark_unmark(chld_ent,
					SA_PLM_RF_DEPENDENCY,0/*unmark*/,NULL,
					SA_NTF_OBJECT_OPERATION,
					SA_PLM_NTFID_STATE_CHANGE_ROOT);
			*is_flag_aff = 1;
		}

	}else{
		if (!plms_rdness_flag_is_set(chld_ent,
						SA_PLM_RF_DEPENDENCY)){
			
			plms_readiness_flag_mark_unmark(chld_ent,
					SA_PLM_RF_DEPENDENCY,1/*unmark*/,NULL,
					SA_NTF_OBJECT_OPERATION,
					SA_PLM_NTFID_STATE_CHANGE_ROOT);
			*is_flag_aff = 1;
		
		}
		LOG_ER("Entity %s can not be moved to insvc, as min dep \
		is not matched.",chld_ent->dn_name_str);
		
		return NCSCC_RC_FAILURE;
	}

	/* All conditioned are matched. Move the ent to InSvc. */
	plms_readiness_state_set(chld_ent,SA_PLM_READINESS_IN_SERVICE,NULL,
				SA_NTF_OBJECT_OPERATION,
				SA_PLM_NTFID_STATE_CHANGE_ROOT);
	
	TRACE("Ent %s is moved to insvc.",chld_ent->dn_name_str);
	return NCSCC_RC_SUCCESS;
}

/******************************************************************************
@brief		: A recurssive function takes care of moving the children and
		its dependends to insvc. This function works in cojuction with
		plms_move_dep_ent_to_insvc.
@param[in]	: chld_ent - The leftmost child of the entity whose children 
		are to move to insvc.
@param[out]	: ent_list - List of entities whose readiness status has been
		changed during this operation.
@param[in]	: inst_chld_ee - 1 ==> Instantiate the chld EE if all the conditions
		are matched.
				 0 ==> Do not instantiate chld EE.
@param[in]	: inst_dep_ee - 1 ==> Instantiate the dep EE if all the conditions
		are matched.
				 0 ==> Do not instantiate dep EE.
******************************************************************************/
void plms_move_chld_ent_to_insvc(PLMS_ENTITY *chld_ent,
	PLMS_GROUP_ENTITY **ent_list,SaUint8T inst_chld_ee,SaUint8T inst_dep_ee)
{
	SaUint32T ret_err;
	/* Terminating condition. */
	if (NULL == chld_ent)
		return ;
	

	/* If chld_ent is already insvc then return.*/
	if (plms_is_rdness_state_set(chld_ent,SA_PLM_READINESS_IN_SERVICE)){
		
		LOG_ER("Entity %s is already  insvc",chld_ent->dn_name_str);
		return;
	}
	
	/* Traverse the right-node */
	plms_move_chld_ent_to_insvc(chld_ent->right_sibling,
	ent_list,inst_chld_ee,inst_dep_ee);
	
	/* If my parent is OOS, then forget. Return from here.*/
	if ((NULL != chld_ent->parent) && 
		!plms_is_rdness_state_set(chld_ent->parent,
		SA_PLM_READINESS_IN_SERVICE)){
		
		if (!plms_rdness_flag_is_set(chld_ent,
					SA_PLM_RF_DEPENDENCY)){
			
			plms_readiness_flag_mark_unmark(chld_ent,
					SA_PLM_RF_DEPENDENCY,1/*mark*/,NULL,
					SA_NTF_OBJECT_OPERATION,
					SA_PLM_NTFID_STATE_CHANGE_ROOT);
			plms_ent_to_ent_list_add(chld_ent,ent_list);

		}
		LOG_ER("Entity %s can not be moved to insvc, as parent is \
		not in service",chld_ent->dn_name_str);
		return;
		
	}
	/* If the min dependency condition is matched. 
	Clear the dependecy flag.*/
	if (plms_min_dep_is_ok(chld_ent)){
		if (plms_rdness_flag_is_set(chld_ent,
						SA_PLM_RF_DEPENDENCY)){

			plms_readiness_flag_mark_unmark(chld_ent,
					SA_PLM_RF_DEPENDENCY,0/* unmark*/,
					chld_ent->parent,
					SA_NTF_OBJECT_OPERATION,
					SA_PLM_NTFID_STATE_CHANGE_DEP);
			plms_ent_to_ent_list_add(chld_ent,ent_list);
		}
	}else{
		if (!plms_rdness_flag_is_set(chld_ent,
						SA_PLM_RF_DEPENDENCY)){
			
			plms_readiness_flag_mark_unmark(chld_ent,
					SA_PLM_RF_DEPENDENCY,1/*unmark*/,NULL,
					SA_NTF_OBJECT_OPERATION,
					SA_PLM_NTFID_STATE_CHANGE_ROOT);
		
			plms_ent_to_ent_list_add(chld_ent,ent_list);
		}
		LOG_ER("Entity %s can not be moved to insvc, as min dep \
		is not matched.",chld_ent->dn_name_str);
		/* Min dependency condition is not matched. Hence
		can not go insvc. So return from here.*/
		return;
	}
	/* If adm state is not unlock. */	
	if ( (PLMS_HE_ENTITY == chld_ent->entity_type) && 
		(SA_PLM_HE_ADMIN_UNLOCKED != 
		chld_ent->entity.he_entity.saPlmHEAdminState )){
		return ;
	}else if ((PLMS_EE_ENTITY == chld_ent->entity_type) &&
		(SA_PLM_EE_ADMIN_UNLOCKED !=
		chld_ent->entity.ee_entity.saPlmEEAdminState)){
		return ;
	}
	
	/* If the operational state is enabled.*/
	if ( (PLMS_HE_ENTITY == chld_ent->entity_type) &&
		(SA_PLM_OPERATIONAL_DISABLED == 
		chld_ent->entity.he_entity.saPlmHEOperationalState)){
		return ;
	}else if ((PLMS_EE_ENTITY == chld_ent->entity_type) &&
		(SA_PLM_OPERATIONAL_DISABLED == 
		chld_ent->entity.ee_entity.saPlmEEOperationalState)){
		return ;
	}
	/* If the presence state of the HE is either active or deactivating
	*/
	if ((PLMS_HE_ENTITY == chld_ent->entity_type) && 
		(( SA_PLM_HE_PRESENCE_ACTIVE !=
		chld_ent->entity.he_entity.saPlmHEPresenceState) && 
		(SA_PLM_HE_PRESENCE_DEACTIVATING != 
		chld_ent->entity.he_entity.saPlmHEPresenceState))){
			return ;
	}
	/* If the presence state of the EE is either instantiated or
	terminating.*/	
	if ((PLMS_EE_ENTITY == chld_ent->entity_type) &&
		(SA_PLM_EE_PRESENCE_INSTANTIATED != 
		chld_ent->entity.ee_entity.saPlmEEPresenceState) &&
		(SA_PLM_EE_PRESENCE_TERMINATING != 
		chld_ent->entity.ee_entity.saPlmEEPresenceState)){
		if (inst_chld_ee){
			ret_err = plms_ent_enable(chld_ent,FALSE,0/*mngt_cbk*/);
			if ( NCSCC_RC_SUCCESS != ret_err){
				LOG_ER("EE Instantiation failed, EE: %s",
							chld_ent->dn_name_str);	
			}
		}
		/* Even if the EE instantiattion is successful,
		dont take it to insvc. Wait for the instantiated msg.*/
		return;
	}
	
	/* All conditioned are matched. Move the ent to InSvc. */
	plms_readiness_state_set(chld_ent,SA_PLM_READINESS_IN_SERVICE,
				chld_ent->parent,SA_NTF_OBJECT_OPERATION,
				SA_PLM_NTFID_STATE_CHANGE_DEP);

	TRACE("Ent %s is moved to insvc.",chld_ent->dn_name_str);

	plms_ent_to_ent_list_add(chld_ent,ent_list);
	/* Take care of dep of this child. */
	plms_move_dep_ent_to_insvc(chld_ent->rev_dep_list,ent_list,
	inst_dep_ee);

	/* Traverse the left-node */
	plms_move_chld_ent_to_insvc(chld_ent->leftmost_child,ent_list,
	inst_chld_ee,inst_dep_ee);
	
	return;
}	
/******************************************************************************
@brief		: A recurssive function takes care of moving the dep and
		their childrens to insvc. This function works in cojuction with
		plms_move_chld_ent_to_insvc.
@param[in]	: dep_ent_list - list of dependend entities. 
@param[out]	: ent_list - List of entities whose readiness status has been
		changed during this operation.
@param[in]	: inst_dep_ee - 1 ==> Instantiate the dep EE if all the conditions
		are matched.
				 0 ==> Do not instantiate dep EE.
******************************************************************************/
void plms_move_dep_ent_to_insvc(PLMS_GROUP_ENTITY *dep_ent_list,
			PLMS_GROUP_ENTITY **ent_list,SaUint8T inst_dep_ee)
{
	SaUint32T ret_err;
	SaUint32T inst = 1;
	/* Terminating condition. */
	if (NULL == dep_ent_list)
		return;
	/* If dep_ent_list->plm_entity is already in isvc, then return.*/	
	if (plms_is_rdness_state_set(dep_ent_list->plm_entity,
				SA_PLM_READINESS_IN_SERVICE)){
		
		LOG_ER("Entity %s is already  insvc",
		dep_ent_list->plm_entity->dn_name_str);
		return;
	}
	
	/* I should complete one dependend list before moving to the dep of 
	dep(BFS algo).*/	
	if (dep_ent_list->next)
		plms_move_dep_ent_to_insvc(dep_ent_list->next,ent_list,
		inst_dep_ee);
	
	/* If my parent is OOS, then forget. Return from here.*/
	if ((NULL != dep_ent_list->plm_entity->parent) && 
		!plms_is_rdness_state_set(dep_ent_list->plm_entity->parent,
		SA_PLM_READINESS_IN_SERVICE)){
		
		if (!plms_rdness_flag_is_set(dep_ent_list->plm_entity,
					SA_PLM_RF_DEPENDENCY)){
			
			plms_readiness_flag_mark_unmark(
					dep_ent_list->plm_entity,
					SA_PLM_RF_DEPENDENCY,1/*mark*/,NULL,
					SA_NTF_OBJECT_OPERATION,
					SA_PLM_NTFID_STATE_CHANGE_ROOT);

			plms_ent_to_ent_list_add(dep_ent_list->plm_entity,ent_list);
		}
		LOG_ER("Entity %s can not be moved to insvc, as parent is \
		not in service",dep_ent_list->plm_entity->dn_name_str);
		return;
		
	}
	/* If the min dependency condition is matched. 
	Clear the dependecy flag.*/
	if (plms_min_dep_is_ok(dep_ent_list->plm_entity)){
		if (plms_rdness_flag_is_set(dep_ent_list->plm_entity,
						SA_PLM_RF_DEPENDENCY)){

			plms_readiness_flag_mark_unmark(
					dep_ent_list->plm_entity,
					SA_PLM_RF_DEPENDENCY,0/* unmark*/,
					NULL,SA_NTF_OBJECT_OPERATION,
					SA_PLM_NTFID_STATE_CHANGE_ROOT);
			plms_ent_to_ent_list_add(dep_ent_list->plm_entity,ent_list);
		}else{
			inst = 0;
		}
	}else{
		if (!plms_rdness_flag_is_set(dep_ent_list->plm_entity,
						SA_PLM_RF_DEPENDENCY)){
			
			plms_readiness_flag_mark_unmark(
					dep_ent_list->plm_entity,
					SA_PLM_RF_DEPENDENCY,1/*mark*/,NULL,
					SA_NTF_OBJECT_OPERATION,
					SA_PLM_NTFID_STATE_CHANGE_ROOT);
			plms_ent_to_ent_list_add(dep_ent_list->plm_entity,ent_list);
		
		}
		LOG_ER("Entity %s can not be moved to insvc, as min dep \
		is not matched.",dep_ent_list->plm_entity->dn_name_str);
		/* Min dependency condition is not matched. Hence
		can not go insvc. So return from here.*/
		return;
	}
	
	/* If adm state is not unlock. */	
	if ( (PLMS_HE_ENTITY ==  dep_ent_list->plm_entity->entity_type) && 
		(SA_PLM_HE_ADMIN_UNLOCKED != 
		 dep_ent_list->plm_entity->entity.he_entity.saPlmHEAdminState)){
		return ;
	}else if ((PLMS_EE_ENTITY == dep_ent_list->plm_entity->entity_type) &&
		(SA_PLM_EE_ADMIN_UNLOCKED !=
		 dep_ent_list->plm_entity->entity.ee_entity.saPlmEEAdminState)){
		return ;
	}
	
	/* If the operational state is enabled.*/
	if ( (PLMS_HE_ENTITY ==  dep_ent_list->plm_entity->entity_type) &&
		(SA_PLM_OPERATIONAL_DISABLED == 
		 dep_ent_list->plm_entity->entity.he_entity.
		 saPlmHEOperationalState)){
		return ;
	}else if ((PLMS_EE_ENTITY == dep_ent_list->plm_entity->entity_type) &&
		(SA_PLM_OPERATIONAL_DISABLED == 
		 dep_ent_list->plm_entity->entity.ee_entity.
		 saPlmEEOperationalState)){
		return ;
	}
	
	/* If the presence state of the HE is either active or deactivating
	*/
	if ((PLMS_HE_ENTITY ==  dep_ent_list->plm_entity->entity_type) && 
		(( SA_PLM_HE_PRESENCE_ACTIVE !=
		 dep_ent_list->plm_entity->entity.he_entity.
		 saPlmHEPresenceState) && 
		(SA_PLM_HE_PRESENCE_DEACTIVATING != 
		 dep_ent_list->plm_entity->entity.he_entity.
		 saPlmHEPresenceState))){
			return ;
	}
	/* If the presence state of the EE is either instantiated or
	terminating.*/	
	if ((PLMS_EE_ENTITY ==  dep_ent_list->plm_entity->entity_type) &&
		(SA_PLM_EE_PRESENCE_INSTANTIATED != 
		 dep_ent_list->plm_entity->entity.ee_entity.
		 saPlmEEPresenceState) && 
		(SA_PLM_EE_PRESENCE_TERMINATING != 
		 dep_ent_list->plm_entity->entity.ee_entity.
		 saPlmEEPresenceState)){
		if (inst_dep_ee && inst){
			ret_err = plms_ent_enable(dep_ent_list->plm_entity,
			FALSE,0/*mngt_cbk*/);
			if ( NCSCC_RC_SUCCESS != ret_err){
				LOG_ER("EE Instantiation failed, EE: %s",
				dep_ent_list->plm_entity->dn_name_str);	
			}
		}
		/* Even if the EE instantiattion is successful,
		dont take it to insvc. Wait for the instantiated msg.*/
		return;
	}
	/* All conditioned are matched. Move the ent to InSvc. */
	plms_readiness_state_set(dep_ent_list->plm_entity,
					SA_PLM_READINESS_IN_SERVICE,NULL,
					SA_NTF_OBJECT_OPERATION,
					SA_PLM_NTFID_STATE_CHANGE_ROOT);
	
	TRACE("Ent %s is moved to insvc.",
	dep_ent_list->plm_entity->dn_name_str);

	plms_ent_to_ent_list_add(dep_ent_list->plm_entity,ent_list);
	/* Take care of the children of dep.*/
	plms_move_chld_ent_to_insvc(dep_ent_list->plm_entity->leftmost_child,
	ent_list,inst_dep_ee,inst_dep_ee);
	
	/* Dep list of dep.*/
	if (dep_ent_list->plm_entity->rev_dep_list)
		plms_move_dep_ent_to_insvc(
		dep_ent_list->plm_entity->rev_dep_list,ent_list,inst_dep_ee);
	
	return;
}	
/******************************************************************************
@brief		: Check if the minimum no of dependent entities of the ent,
		are insvc.
@param[in]	: ent - HE or EE.
@return		: TRUE - If minimum no of dependent entities are insvc.
		  FALSE - If minimum no of dependent entities are not insvc.
******************************************************************************/
SaUint32T plms_min_dep_is_ok(PLMS_ENTITY *ent)	
{
	SaUint32T count;
	PLMS_GROUP_ENTITY *head;
	
	/* Min no of dep is configured as zero.*/
	if (!(ent->min_no_dep))
		return NCSCC_RC_SUCCESS;

	count = ent->min_no_dep;
	head = ent->dependency_list;

	while (head){
		if (((PLMS_HE_ENTITY == head->plm_entity->entity_type) && 
		(SA_PLM_READINESS_IN_SERVICE == 
		head->plm_entity->entity.he_entity.saPlmHEReadinessState)) || 
		((PLMS_EE_ENTITY == head->plm_entity->entity_type) && 
		(SA_PLM_READINESS_IN_SERVICE ==
		 head->plm_entity->entity.ee_entity.saPlmEEReadinessState)))
			count--;
		if (!count)
			return TRUE;
		head = head->next;	
	}
	TRACE("Min insvc dep ent not OK.\
	Min required dep: %d, No of insvc dep: %d, Ent: %s",\
	ent->min_no_dep,( ent->min_no_dep - count),ent->dn_name_str);
	
	return FALSE;
}

/******************************************************************************
@brief		: If any of the ancestor of this entity is in M5/M1/M0. 
@param[in]	: ent - HE.		
@return		: TRUE - If any of ancestor is in M5/M1/M0.
		  FALSE - If none of the ancestor is in M5/M1/M0.
******************************************************************************/
SaUint32T plms_is_anc_in_deact_cnxt(PLMS_ENTITY *ent)
{
	PLMS_ENTITY *head;

	head = ent->parent;
	while(head){
		if (PLMS_HE_ENTITY == head->entity_type){
			if ((SA_PLM_HE_PRESENCE_ACTIVE != 
				head->entity.he_entity.saPlmHEPresenceState) &&
				(SA_PLM_HE_PRESENCE_ACTIVATING != 
				head->entity.he_entity.saPlmHEPresenceState)){
				return TRUE; 
			}
		}
		head = head->parent;
	}
	return FALSE;
}
/******************************************************************************
@brief		: Call readiness callback for marking/clearing the management lost flag.
@param[in]	: ent - EE or HE.
@param[in]	: lost - management lost flag lost/regained.
@return		: NCSCC_RC_SUCCESS
******************************************************************************/
SaUint32T plms_mngt_lost_clear_cbk_call(PLMS_ENTITY *ent,SaUint32T lost)
{
	PLMS_TRACK_INFO trk_info;
	PLMS_ENTITY_GROUP_INFO_LIST *log_head_grp;
	
	TRACE_ENTER2("Entity: %s",ent->dn_name_str);

	memset(&trk_info,0,sizeof(PLMS_TRACK_INFO));

	trk_info.group_info_list = NULL;
	
	/* Overwrite the expected readiness state and flag of the
	root entity.*/
	plms_ent_exp_rdness_state_ow(ent);

	/* Add the groups, root entity(ent) belong to.*/
	plms_ent_grp_list_add(ent,&(trk_info.group_info_list));
	
	TRACE("Affected groups for ent %s: ",ent->dn_name_str);
	log_head_grp = trk_info.group_info_list;
	while(log_head_grp){
		TRACE("%llu,",log_head_grp->ent_grp_inf->entity_grp_hdl);
		log_head_grp = log_head_grp->next;
	}
	trk_info.aff_ent_list = NULL;
	trk_info.change_step = SA_PLM_CHANGE_COMPLETED;
	trk_info.root_entity = NULL;
	trk_info.root_correlation_id = SA_NTF_IDENTIFIER_UNUSED;
	trk_info.grp_op = SA_PLM_GROUP_MEMBER_READINESS_CHANGE;
	if (!lost)
		trk_info.track_cause = SA_PLM_CAUSE_MANAGEMENT_REGAINED;
	else	
		trk_info.track_cause = SA_PLM_CAUSE_MANAGEMENT_LOST;
	
	plms_ent_to_ent_list_add(ent,&trk_info.aff_ent_list);
	plms_cbk_call(&trk_info,1);

	plms_ent_exp_rdness_status_clear(ent);

	plms_ent_grp_list_free(trk_info.group_info_list);
	
	TRACE_LEAVE2("Return Val: %d",NCSCC_RC_SUCCESS);
	return NCSCC_RC_SUCCESS;
}
/******************************************************************************
@brief		: The failure is yet to be happened. Set the imminent-failure
		flag and call the callback.
@param[in]	: ent - HE or EE
@param[in]	: snd_info - Used for sync send to PLMA.
@param[in/out]	: cor_ids - Unused. PLMA takes care of filling the out parameter
		for the application calling readiness impact call.
@return		: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE			
******************************************************************************/
SaUint32T plms_rdness_imminent_failure_process(PLMS_ENTITY *ent, 
			PLMS_SEND_INFO *snd_info,SaNtfCorrelationIdsT *cor_ids)
{
	SaUint32T ret_err;
	PLMS_GROUP_ENTITY *aff_ent_list = NULL,*head;
	PLMS_TRACK_INFO	trk_info;
	PLMS_ENTITY_GROUP_INFO_LIST *log_head_grp;
	SaNtfIdentifierT ntf_id = SA_NTF_IDENTIFIER_UNUSED;
	PLMS_EVT evt;
	PLMS_CB *cb = plms_cb;

	TRACE_ENTER2("Entity: %s",ent->dn_name_str);

	if (plms_rdness_flag_is_set(ent,SA_PLM_RF_IMMINENT_FAILURE)){

		TRACE("Imminent failure flag is already set.Ent: %s",
		ent->dn_name_str);

		evt.req_res = PLMS_RES;
		evt.res_evt.res_type = PLMS_AGENT_TRACK_READINESS_IMPACT_RES;
		evt.res_evt.ntf_id = ntf_id;
		evt.res_evt.error = SA_AIS_OK;
		ret_err = plm_send_mds_rsp(cb->mds_hdl,NCSMDS_SVC_ID_PLMS,
		snd_info,&evt);
		if(NCSCC_RC_SUCCESS != ret_err){
			LOG_ER("Sync resp to PLMA for readiness impact fault\
			clear FAILED. ret_val = %d",ret_err);
		}
		TRACE_LEAVE2("Return Val: %d",NCSCC_RC_SUCCESS);
		return NCSCC_RC_SUCCESS;
	}
	/* If operational state is disabled, then return.*/
	if ( ((PLMS_HE_ENTITY == ent->entity_type) && 
	(SA_PLM_OPERATIONAL_DISABLED == ent->entity.he_entity.saPlmHEOperationalState)) ||
	((PLMS_EE_ENTITY == ent->entity_type) &&
	(SA_PLM_OPERATIONAL_DISABLED == ent->entity.ee_entity.saPlmEEOperationalState))) {

		TRACE("Operational state is disabled. Ent: %s",
		ent->dn_name_str);

		evt.req_res = PLMS_RES;
		evt.res_evt.res_type = PLMS_AGENT_TRACK_READINESS_IMPACT_RES;
		evt.res_evt.ntf_id = ntf_id;
		evt.res_evt.error = SA_AIS_OK;
		ret_err = plm_send_mds_rsp(cb->mds_hdl,NCSMDS_SVC_ID_PLMS,
		snd_info,&evt);
		if(NCSCC_RC_SUCCESS != ret_err){
			LOG_ER("Sync resp to PLMA for readiness impact fault\
			clear FAILED. ret_val = %d",ret_err);
		}
		TRACE_LEAVE2("Return Val: %d",NCSCC_RC_SUCCESS);
		return NCSCC_RC_SUCCESS;
	}
	
	plms_aff_ent_list_imnt_failure_get(ent,&aff_ent_list,1/* mark */);
	TRACE("Affected entities for ent %s: ", ent->dn_name_str);
	head = aff_ent_list;
	while(head){
		TRACE("%s,",head->plm_entity->dn_name_str);
		head = head->next;
	}
	
	if (plms_anc_chld_dep_adm_flag_is_set(ent,aff_ent_list)){
		TRACE("Imminent failure request can not be set as the ent %s\
		is in some other operational context.",ent->dn_name_str);

		evt.req_res = PLMS_RES;
		evt.res_evt.res_type = PLMS_AGENT_TRACK_READINESS_IMPACT_RES;
		evt.res_evt.ntf_id = ntf_id;
		evt.res_evt.error = SA_AIS_ERR_TRY_AGAIN;
		ret_err = plm_send_mds_rsp(cb->mds_hdl,NCSMDS_SVC_ID_PLMS,
		snd_info,&evt);
		if(NCSCC_RC_SUCCESS != ret_err){
			LOG_ER("Sync resp to PLMA for readiness impact fault\
			clear FAILED. ret_val = %d",ret_err);
		}
		plms_ent_list_free(aff_ent_list);
		TRACE_LEAVE2("Return Val: %d",NCSCC_RC_FAILURE);
		return NCSCC_RC_FAILURE;
	}
	
	ntf_id = plms_readiness_flag_mark_unmark(ent,
				SA_PLM_RF_IMMINENT_FAILURE,1/*mark*/,
				NULL,SA_NTF_OBJECT_OPERATION,
				SA_PLM_NTFID_STATE_CHANGE_ROOT);

	head = aff_ent_list;
	while (head){
		plms_readiness_flag_mark_unmark(head->plm_entity,
					SA_PLM_RF_DEPENDENCY_IMMINENT_FAILURE,
					1/*mark*/,ent,SA_NTF_OBJECT_OPERATION,
					SA_PLM_NTFID_STATE_CHANGE_DEP);
		head = head->next;
	}

	/* Overwrite the expected readiness status with the current.*/
	plms_aff_ent_exp_rdness_state_ow(aff_ent_list);
	
	/* Overwrite the expected readiness state and flag of the 
	root entity.*/
	plms_ent_exp_rdness_state_ow(ent);

	memset(&trk_info,0,sizeof(PLMS_TRACK_INFO));

	/* Fill the track info.*/
	trk_info.aff_ent_list = aff_ent_list;
	trk_info.group_info_list = NULL;
	
	/* Add the groups, root entity(ent) belong to.*/
	plms_ent_grp_list_add(ent,&(trk_info.group_info_list));
	
	/* Find out all the groups, all affected entities belong to and add 
	the groups to trk_info->group_info_list.
	*/
	if (NULL != aff_ent_list){
		plms_ent_list_grp_list_add(aff_ent_list,
		&(trk_info.group_info_list));	
	}
	TRACE("Affected groups for ent %s: ",ent->dn_name_str);
	log_head_grp = trk_info.group_info_list;
	while(log_head_grp){
		TRACE("%llu,",log_head_grp->ent_grp_inf->entity_grp_hdl);
		log_head_grp = log_head_grp->next;
	}
	
	trk_info.change_step = SA_PLM_CHANGE_COMPLETED;
	trk_info.track_cause = SA_PLM_CAUSE_IMMINENT_FAILURE;
	trk_info.root_correlation_id = ntf_id;
	trk_info.grp_op = SA_PLM_GROUP_MEMBER_READINESS_CHANGE; 
	trk_info.root_entity = ent;

	plms_cbk_call(&trk_info,1);

	plms_ent_exp_rdness_status_clear(ent);
	plms_aff_ent_exp_rdness_status_clear(aff_ent_list);
	
	plms_ent_list_free(aff_ent_list);
	aff_ent_list = NULL;
	plms_ent_grp_list_free(trk_info.group_info_list);
	trk_info.group_info_list = NULL;
	
	/* Resp to PLMA.*/
	{
		evt.req_res = PLMS_RES;
		evt.res_evt.res_type = PLMS_AGENT_TRACK_READINESS_IMPACT_RES;
		evt.res_evt.ntf_id = ntf_id;
		evt.res_evt.error = SA_AIS_OK;
		ret_err = plm_send_mds_rsp(cb->mds_hdl,NCSMDS_SVC_ID_PLMS,
		snd_info,&evt);
		if(NCSCC_RC_SUCCESS != ret_err){
			LOG_ER("Sync resp to PLMA for readiness impact fault\
			clear FAILED. ret_val = %d",ret_err);
		}
	}
	TRACE_LEAVE2("Return Val: %d",ret_err);
	return ret_err;
}
/******************************************************************************
@brief		: Failure did not happen as the application has reported.
		Clear imminent failure flag.
@param[in]	: ent - HE or EE
@param[in]	: snd_info - Used for sync send to PLMA.
@param[in/out]	: cor_ids - Unused. PLMA takes care of filling the out parameter
		for the application calling readiness impact call.
@return		: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE			
******************************************************************************/
SaUint32T plms_rdness_imminent_failure_cleared_process(PLMS_ENTITY *ent,
			PLMS_SEND_INFO *snd_info,SaNtfCorrelationIdsT *cor_ids)
{
	SaUint32T ret_err = NCSCC_RC_SUCCESS;
	PLMS_GROUP_ENTITY *aff_ent_list = NULL,*head;
	PLMS_TRACK_INFO	trk_info;
	PLMS_ENTITY_GROUP_INFO_LIST *log_head_grp;
	SaNtfIdentifierT ntf_id = SA_NTF_IDENTIFIER_UNUSED;
	PLMS_CB *cb = plms_cb;
	PLMS_EVT evt;

	TRACE_ENTER2("Entity: %s",ent->dn_name_str);
	
	if (!plms_rdness_flag_is_set(ent,SA_PLM_RF_IMMINENT_FAILURE)){

		TRACE("Imminent failure flag is already cleared.Ent: %s",
		ent->dn_name_str);

		evt.req_res = PLMS_RES;
		evt.res_evt.res_type = PLMS_AGENT_TRACK_READINESS_IMPACT_RES;
		evt.res_evt.ntf_id = ntf_id;
		evt.res_evt.error = SA_AIS_OK;
		ret_err = plm_send_mds_rsp(cb->mds_hdl,NCSMDS_SVC_ID_PLMS,
		snd_info,&evt);
		if(NCSCC_RC_SUCCESS != ret_err){
			LOG_ER("Sync resp to PLMA for readiness impact fault\
			clear FAILED. ret_val = %d",ret_err);
		}
		TRACE_LEAVE2("Return Val: %d",NCSCC_RC_SUCCESS);
		return NCSCC_RC_SUCCESS;
	}
	
	plms_aff_ent_list_imnt_failure_get(ent,&aff_ent_list, 0/*unmark */);

	TRACE("Affected entities for ent %s: ", ent->dn_name_str);
	head = aff_ent_list;
	while(head){
		TRACE("%s,",head->plm_entity->dn_name_str);
		head = head->next;
	}
	
	if (plms_anc_chld_dep_adm_flag_is_set(ent,aff_ent_list)){
		TRACE("Imminent failure request can not be set as the ent %s\
		is in some other operational context.",ent->dn_name_str);

		evt.req_res = PLMS_RES;
		evt.res_evt.res_type = PLMS_AGENT_TRACK_READINESS_IMPACT_RES;
		evt.res_evt.ntf_id = ntf_id;
		evt.res_evt.error = SA_AIS_ERR_TRY_AGAIN;
		ret_err = plm_send_mds_rsp(cb->mds_hdl,NCSMDS_SVC_ID_PLMS,
		snd_info,&evt);
		if(NCSCC_RC_SUCCESS != ret_err){
			LOG_ER("Sync resp to PLMA for readiness impact fault\
			clear FAILED. ret_val = %d",ret_err);
		}
		plms_ent_list_free(aff_ent_list);
		TRACE_LEAVE2("Return Val: %d",NCSCC_RC_FAILURE);
		return NCSCC_RC_FAILURE;
	}

	ntf_id = plms_readiness_flag_mark_unmark(ent,
				SA_PLM_RF_IMMINENT_FAILURE,0/*unmark*/,
				NULL,SA_NTF_OBJECT_OPERATION,
				SA_PLM_NTFID_STATE_CHANGE_ROOT);

	head = aff_ent_list;
	while(head){
		plms_readiness_flag_mark_unmark(head->plm_entity,
				SA_PLM_RF_DEPENDENCY_IMMINENT_FAILURE,
				0/*unmark*/,ent,SA_NTF_OBJECT_OPERATION,
				SA_PLM_NTFID_STATE_CHANGE_DEP);
		head = head->next;
	}
	
	/* Overwrite the expected readiness status with the current.*/
	plms_aff_ent_exp_rdness_state_ow(aff_ent_list);
	
	/* Overwrite the expected readiness state and flag of the 
	root entity.*/
	plms_ent_exp_rdness_state_ow(ent);
	
	memset(&trk_info,0,sizeof(PLMS_TRACK_INFO));

	/* Fill the track info.*/
	trk_info.aff_ent_list = aff_ent_list;
	trk_info.group_info_list = NULL;
	
	/* Add the groups, root entity(ent) belong to.*/
	plms_ent_grp_list_add(ent,&(trk_info.group_info_list));
	
	/* Find out all the groups, all affected entities belong to and add 
	the groups to trk_info->group_info_list.
	*/
	if (NULL != aff_ent_list){
		plms_ent_list_grp_list_add(aff_ent_list,
		&(trk_info.group_info_list));	
	}

	TRACE("Affected groups for ent %s: ",ent->dn_name_str);
	log_head_grp = trk_info.group_info_list;
	while(log_head_grp){
		TRACE("%llu,",log_head_grp->ent_grp_inf->entity_grp_hdl);
		log_head_grp = log_head_grp->next;
	}
	
	trk_info.change_step = SA_PLM_CHANGE_COMPLETED;
	trk_info.track_cause = SA_PLM_CAUSE_IMMINENT_FAILURE_CLEARED;
	trk_info.root_correlation_id = ntf_id;
	trk_info.grp_op = SA_PLM_GROUP_MEMBER_READINESS_CHANGE; 
	trk_info.root_entity = ent;

	plms_cbk_call(&trk_info,1);
	
	plms_ent_exp_rdness_status_clear(ent);
	plms_aff_ent_exp_rdness_status_clear(aff_ent_list);
	
	plms_ent_list_free(aff_ent_list);
	aff_ent_list = NULL;
	plms_ent_grp_list_free(trk_info.group_info_list);
	trk_info.group_info_list = NULL;
	
	/* Resp to PLMA.*/
	{
		evt.req_res = PLMS_RES;
		evt.res_evt.res_type = PLMS_AGENT_TRACK_READINESS_IMPACT_RES;
		evt.res_evt.ntf_id = ntf_id;
		evt.res_evt.error = SA_AIS_OK;
		ret_err = plm_send_mds_rsp(cb->mds_hdl,NCSMDS_SVC_ID_PLMS,
		snd_info,&evt);
		if(NCSCC_RC_SUCCESS != ret_err){
			LOG_ER("Sync resp to PLMA for readiness impact fault\
			clear FAILED. ret_val = %d",ret_err);
		}
	}
	TRACE_LEAVE2("Return Val: %d",ret_err);
	return ret_err;
}
/******************************************************************************
@brief		: Application has reported a failure and hence clear the imminet
		failure flag if set and isolate the entity.
@param[in]	: ent - HE or EE
@param[in]	: snd_info - Used for sync send to PLMA.
@param[in/out]	: cor_ids - Unused. PLMA takes care of filling the out parameter
		for the application calling readiness impact call.
@return		: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE			
******************************************************************************/
SaUint32T plms_rdness_failure_process(PLMS_ENTITY *ent,
			PLMS_SEND_INFO *snd_info,SaNtfCorrelationIdsT *cor_ids)
{
	SaUint32T ret_err;
	PLMS_EVT evt;
	PLMS_TRACK_INFO	trk_info;
	PLMS_CB *cb = plms_cb;
	PLMS_GROUP_ENTITY *aff_ent_list = NULL;
	PLMS_GROUP_ENTITY *head;
	PLMS_ENTITY_GROUP_INFO_LIST *log_head_grp;
	PLMS_GROUP_ENTITY *aff_ent_list_flag = NULL;
	PLMS_GROUP_ENTITY *aff_ent_list_state = NULL;
	SaNtfIdentifierT ntf_id = SA_NTF_IDENTIFIER_UNUSED;
	SaUint32T is_flag = 0;	

	TRACE_ENTER2("Entity: %s",ent->dn_name_str);
	memset(&trk_info,0,sizeof(PLMS_TRACK_INFO));

	/* If the operational state is already disabled, then nothing to do.*/
	if (((PLMS_HE_ENTITY == ent->entity_type) && 
		(SA_PLM_OPERATIONAL_DISABLED == 
		ent->entity.he_entity.saPlmHEOperationalState)) || 
		((PLMS_EE_ENTITY == ent->entity_type) && 
		(SA_PLM_OPERATIONAL_DISABLED ==
		ent->entity.ee_entity.saPlmEEOperationalState))) {

		TRACE("Operational state is already disabled. Return.");
		{
			evt.req_res = PLMS_RES;
			evt.res_evt.res_type = 
			PLMS_AGENT_TRACK_READINESS_IMPACT_RES;
			
			evt.res_evt.ntf_id = ntf_id;
			evt.res_evt.error = SA_AIS_OK;
			ret_err = plm_send_mds_rsp(cb->mds_hdl,
			NCSMDS_SVC_ID_PLMS, snd_info,&evt);
			
			if(NCSCC_RC_SUCCESS != ret_err){
				LOG_ER("Sync resp to PLMA for readiness impact\
				fault clear FAILED. ret_val = %d",ret_err);
			}
		}
		TRACE_LEAVE2("Return Val: %d",NCSCC_RC_SUCCESS);
		/* TODO: Send resp to PLMA.*/
		return NCSCC_RC_SUCCESS;
	}

	if (plms_rdness_flag_is_set(ent,SA_PLM_RF_IMMINENT_FAILURE)){

		is_flag = 1;
		/* Find out the entities whose imminent dependency flag 
	 	* was set because of this entity.*/
		plms_aff_ent_list_imnt_failure_get(ent, &aff_ent_list_flag,
		0/* unmark */);
		
		TRACE("Affected entities(imminet failure flag clear) \
		for ent %s: ", ent->dn_name_str);
		
		head = aff_ent_list_flag;
		while(head){
			TRACE("%s,",head->plm_entity->dn_name_str);
			head = head->next;
		}
		if (plms_anc_chld_dep_adm_flag_is_set(ent,aff_ent_list_flag)){
			TRACE("Imminent failure request can not be set as the \
			ent %s is in some other operational context.",
			ent->dn_name_str);

			evt.req_res = PLMS_RES;
			evt.res_evt.res_type = 
			PLMS_AGENT_TRACK_READINESS_IMPACT_RES;
			
			evt.res_evt.ntf_id = ntf_id;
			evt.res_evt.error = SA_AIS_ERR_TRY_AGAIN;
			ret_err = plm_send_mds_rsp(cb->mds_hdl,
			NCSMDS_SVC_ID_PLMS,snd_info,&evt);
			if(NCSCC_RC_SUCCESS != ret_err){
				LOG_ER("Sync resp to PLMA for readiness impact \
				fault clear FAILED. ret_val = %d",ret_err);
			}
			plms_ent_list_free(aff_ent_list_flag);
			TRACE_LEAVE2("Return Val: %d",NCSCC_RC_FAILURE);
			return NCSCC_RC_FAILURE;
		}
		
		/* Fault is happened, clear imminent failure flag if set. */	
		plms_readiness_flag_mark_unmark(ent,SA_PLM_RF_IMMINENT_FAILURE,
		0,NULL,SA_NTF_OBJECT_OPERATION,SA_PLM_NTFID_STATE_CHANGE_ROOT);
		
		/* Also clear dep-imminet-failure flag is set.*/
		plms_readiness_flag_mark_unmark(ent,SA_PLM_RF_DEPENDENCY_IMMINENT_FAILURE,
		0,NULL,SA_NTF_OBJECT_OPERATION,SA_PLM_NTFID_STATE_CHANGE_ROOT);
		
		head = aff_ent_list_flag;
		while(head){
			plms_readiness_flag_mark_unmark(head->plm_entity,
			SA_PLM_RF_DEPENDENCY_IMMINENT_FAILURE,0/*unmark*/,
			ent,SA_NTF_OBJECT_OPERATION,
			SA_PLM_NTFID_STATE_CHANGE_DEP);
			
			head = head->next;
		}
	}else if (plms_rdness_flag_is_set(ent,SA_PLM_RF_DEPENDENCY_IMMINENT_FAILURE)){
		
		is_flag = 1;
		
		plms_readiness_flag_mark_unmark(ent,SA_PLM_RF_DEPENDENCY_IMMINENT_FAILURE,
		0,NULL,SA_NTF_OBJECT_OPERATION,SA_PLM_NTFID_STATE_CHANGE_ROOT);
		
		plms_aff_dep_list_imnt_failure_get(ent,ent->rev_dep_list,
						0,&aff_ent_list_flag);
		
		head = aff_ent_list_flag;
		while(head){
			plms_readiness_flag_mark_unmark(head->plm_entity,
			SA_PLM_RF_DEPENDENCY_IMMINENT_FAILURE,0/*unmark*/,
			ent,SA_NTF_OBJECT_OPERATION,
			SA_PLM_NTFID_STATE_CHANGE_DEP);
			
			head = head->next;
		}
	}
	
	/* If the entity is already in OOS, then nothing to do.*/
	if ( ((PLMS_HE_ENTITY == ent->entity_type) && 
		(SA_PLM_READINESS_OUT_OF_SERVICE == 
		ent->entity.he_entity.saPlmHEReadinessState)) ||
		((PLMS_EE_ENTITY == ent->entity_type) && 
		(SA_PLM_READINESS_OUT_OF_SERVICE == 
		ent->entity.ee_entity.saPlmEEReadinessState))){
		
		TRACE("The faulty entity is already in OOS.");
		if(ent->am_i_aff_ent){
			TRACE("Imminent failure request can not be set as the \
			ent %s is in some other operational context.",
			ent->dn_name_str);

			evt.req_res = PLMS_RES;
			evt.res_evt.res_type = 
			PLMS_AGENT_TRACK_READINESS_IMPACT_RES;
			
			evt.res_evt.ntf_id = ntf_id;
			evt.res_evt.error = SA_AIS_ERR_TRY_AGAIN;
			ret_err = plm_send_mds_rsp(cb->mds_hdl,
			NCSMDS_SVC_ID_PLMS,snd_info,&evt);
			if(NCSCC_RC_SUCCESS != ret_err){
				LOG_ER("Sync resp to PLMA for readiness impact \
				fault clear FAILED. ret_val = %d",ret_err);
			}
			plms_ent_list_free(aff_ent_list_flag);
			TRACE_LEAVE2("Return Val: %d",NCSCC_RC_FAILURE);
			return NCSCC_RC_FAILURE;
		}
		/* Mark the operational state of the faulty entity to 
		disabled. */  
		ntf_id = plms_op_state_set(ent,SA_PLM_OPERATIONAL_DISABLED,NULL,
		SA_NTF_OBJECT_OPERATION, SA_PLM_NTFID_STATE_CHANGE_ROOT);

		/* Isolate the entity.*/
		ret_err = plms_ent_isolate(ent,FALSE,TRUE);

		/* Check if we need to call any callback.*/
		if (is_flag){
			/* Overwrite the expected readiness status with the current.*/
			plms_aff_ent_exp_rdness_state_ow(aff_ent_list_flag);
			
			/* Overwrite the expected readiness state and flag of the 
			root entity.*/
			plms_ent_exp_rdness_state_ow(ent);
			
			trk_info.aff_ent_list = aff_ent_list_flag;	
			trk_info.group_info_list = NULL;
			
			/* Add the groups, root entity(ent) belong to.*/
			plms_ent_grp_list_add(ent,&(trk_info.group_info_list));
			
			/* Find out all the groups, all affected entities belong to and add 
			the groups to trk_info->group_info_list.
			*/
			if (NULL != aff_ent_list_flag){
				plms_ent_list_grp_list_add(aff_ent_list_flag,
				&(trk_info.group_info_list));	
			}

			TRACE("Affected groups for ent %s: ",ent->dn_name_str);
			log_head_grp = trk_info.group_info_list;
			while(log_head_grp){
				TRACE("%llu,",log_head_grp->ent_grp_inf->entity_grp_hdl);
				log_head_grp = log_head_grp->next;
			}
			
			trk_info.change_step = SA_PLM_CHANGE_COMPLETED;
			trk_info.track_cause = SA_PLM_CAUSE_FAILURE;
			trk_info.root_correlation_id = ntf_id;
			trk_info.grp_op = SA_PLM_GROUP_MEMBER_READINESS_CHANGE; 
			trk_info.root_entity = ent;

			plms_cbk_call(&trk_info,1);
			
			plms_ent_exp_rdness_status_clear(ent);
			plms_aff_ent_exp_rdness_status_clear(aff_ent_list_flag);
			
			plms_ent_list_free(aff_ent_list_flag);
			trk_info.aff_ent_list = NULL;
			plms_ent_grp_list_free(trk_info.group_info_list);
			trk_info.group_info_list = NULL;
		}

		/* Resp to PLMA.*/
		evt.req_res = PLMS_RES;
		evt.res_evt.res_type = PLMS_AGENT_TRACK_READINESS_IMPACT_RES;
		evt.res_evt.ntf_id = ntf_id;
		if ( NCSCC_RC_SUCCESS == ret_err )
			evt.res_evt.error = SA_AIS_OK;
		else
			evt.res_evt.error = SA_AIS_ERR_TRY_AGAIN;

		ret_err = plm_send_mds_rsp(cb->mds_hdl,NCSMDS_SVC_ID_PLMS,
		snd_info,&evt);
		if(NCSCC_RC_SUCCESS != ret_err){
			LOG_ER("Sync resp to PLMA for readiness impact fault\
			clear FAILED. ret_val = %d",ret_err);
		}
		
		TRACE_LEAVE2("Return Val: %d",ret_err);
		return ret_err;

	}else{
		/* Find out the affected entities.*/
		plms_affected_ent_list_get(ent,&aff_ent_list_state,0/*OOS */);

		if (plms_anc_chld_dep_adm_flag_is_set(ent,aff_ent_list_state)){
			TRACE("Imminent failure request can not be set as the \
			ent %s is in some other operational context.",
			ent->dn_name_str);

			evt.req_res = PLMS_RES;
			evt.res_evt.res_type = 
			PLMS_AGENT_TRACK_READINESS_IMPACT_RES;
			
			evt.res_evt.ntf_id = ntf_id;
			evt.res_evt.error = SA_AIS_ERR_TRY_AGAIN;
			ret_err = plm_send_mds_rsp(cb->mds_hdl,
			NCSMDS_SVC_ID_PLMS,snd_info,&evt);
			if(NCSCC_RC_SUCCESS != ret_err){
				LOG_ER("Sync resp to PLMA for readiness impact \
				fault clear FAILED. ret_val = %d",ret_err);
			}
			plms_ent_list_free(aff_ent_list_flag);
			plms_ent_list_free(aff_ent_list_state);
			TRACE_LEAVE2("Return Val: %d",NCSCC_RC_FAILURE);
			return NCSCC_RC_FAILURE;
		}
		
		/* Mark the operational state of the faulty entity to 
		disabled. */  
		ntf_id = plms_op_state_set(ent,SA_PLM_OPERATIONAL_DISABLED,NULL,
		SA_NTF_OBJECT_OPERATION, SA_PLM_NTFID_STATE_CHANGE_ROOT);

		head = aff_ent_list_state;
		while(head){
			plms_readiness_state_set(head->plm_entity,
			SA_PLM_READINESS_OUT_OF_SERVICE,ent,
			SA_NTF_OBJECT_OPERATION,SA_PLM_NTFID_STATE_CHANGE_DEP);
			
			plms_readiness_flag_mark_unmark(head->plm_entity,
			SA_PLM_RF_DEPENDENCY,1/*mark*/,ent,
			SA_NTF_OBJECT_OPERATION,
			SA_PLM_NTFID_STATE_CHANGE_DEP);

			if(PLMS_EE_ENTITY == head->plm_entity->entity_type){
				ret_err = plms_ee_term(head->plm_entity,
				FALSE,0);

				if (NCSCC_RC_SUCCESS != ret_err){
					LOG_ER("EE %s term FAILED.",
					head->plm_entity->dn_name_str);
				}
			}
			head = head->next;
		}
	}
	/* Isolate the faulty entity. HE->Inactive, EE->terminate.*/
	ret_err = plms_ent_isolate(ent,FALSE,FALSE);

	/* Isolation done. Move the entity to OOS.*/
	plms_readiness_state_set(ent, SA_PLM_READINESS_OUT_OF_SERVICE,NULL,
	SA_NTF_OBJECT_OPERATION, SA_PLM_NTFID_STATE_CHANGE_ROOT);
	

	if (NULL != aff_ent_list_flag)
		aff_ent_list = aff_ent_list_flag;

	if (NULL != aff_ent_list_state){
		head = aff_ent_list_state;
		while(head){
			plms_ent_to_ent_list_add( head->plm_entity,
			&aff_ent_list);
			head= head->next;
		}
		plms_ent_list_free(aff_ent_list_state);
		aff_ent_list_state = NULL;
	}
	
	TRACE("Affected entities(all aff by this operation) for ent %s:\
	", ent->dn_name_str);
		
	head = aff_ent_list;
	while(head){
		TRACE("%s,",head->plm_entity->dn_name_str);
		head = head->next;
	}
	/* Overwrite the expected readiness status with the current.*/
	plms_aff_ent_exp_rdness_state_ow(aff_ent_list);
	
	/* Overwrite the expected readiness state and flag of the 
	root entity.*/
	plms_ent_exp_rdness_state_ow(ent);
	
	trk_info.aff_ent_list = aff_ent_list;	
	trk_info.group_info_list = NULL;
	
	/* Add the groups, root entity(ent) belong to.*/
	plms_ent_grp_list_add(ent,&(trk_info.group_info_list));
	
	/* Find out all the groups, all affected entities belong to and add 
	the groups to trk_info->group_info_list.
	*/
	if (NULL != aff_ent_list){
		plms_ent_list_grp_list_add(aff_ent_list,
		&(trk_info.group_info_list));	
	}

	TRACE("Affected groups for ent %s: ",ent->dn_name_str);
	log_head_grp = trk_info.group_info_list;
	while(log_head_grp){
		TRACE("%llu,",log_head_grp->ent_grp_inf->entity_grp_hdl);
		log_head_grp = log_head_grp->next;
	}
	
	trk_info.change_step = SA_PLM_CHANGE_COMPLETED;
	trk_info.track_cause = SA_PLM_CAUSE_FAILURE;
	trk_info.root_correlation_id = ntf_id;
	trk_info.grp_op = SA_PLM_GROUP_MEMBER_READINESS_CHANGE; 
	trk_info.root_entity = ent;

	plms_cbk_call(&trk_info,1);
	
	plms_ent_exp_rdness_status_clear(ent);
	plms_aff_ent_exp_rdness_status_clear(aff_ent_list);
	
	plms_ent_list_free(aff_ent_list);
	trk_info.aff_ent_list = NULL;
	plms_ent_grp_list_free(trk_info.group_info_list);
	trk_info.group_info_list = NULL;
	
	/* Resp to PLMA.*/
	{
		evt.req_res = PLMS_RES;
		evt.res_evt.res_type = PLMS_AGENT_TRACK_READINESS_IMPACT_RES;
		evt.res_evt.ntf_id = ntf_id;
		evt.res_evt.error = SA_AIS_OK;
		ret_err = plm_send_mds_rsp(cb->mds_hdl,NCSMDS_SVC_ID_PLMS,
		snd_info,&evt);
		if(NCSCC_RC_SUCCESS != ret_err){
			LOG_ER("Sync resp to PLMA for readiness impact fault\
			clear FAILED. ret_val = %d",ret_err);
		}
	}
	TRACE_LEAVE2("Return Val: %d",ret_err);
	return ret_err;
}
/******************************************************************************
@brief		: Application has reported that the fault is cleared. Hence 
		enable the isolated the entity.
@param[in]	: ent - HE or EE
@param[in]	: snd_info - Used for sync send to PLMA.
@param[in/out]	: cor_ids - Unused. PLMA takes care of filling the out parameter
		for the application calling readiness impact call.
@return		: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE			
******************************************************************************/
SaUint32T plms_rdness_failure_cleared_process(PLMS_ENTITY *ent,
			PLMS_SEND_INFO *snd_info,SaNtfCorrelationIdsT *cor_ids)
{
	SaUint32T ret_err,is_set = 0;
	SaNtfIdentifierT ntf_id = SA_NTF_IDENTIFIER_UNUSED;
	PLMS_EVT evt;
	PLMS_TRACK_INFO trk_info,*trk_info_flt;
	PLMS_ENTITY_GROUP_INFO_LIST *log_head_grp;
	PLMS_GROUP_ENTITY *aff_ent_list = NULL;
	PLMS_CB *cb = plms_cb;

	TRACE_ENTER2("Entity: %s",ent->dn_name_str);
	memset(&trk_info,0,sizeof(PLMS_TRACK_INFO));

	/* If the operational state is already ENABLED, then nothing to do.*/
	if (((PLMS_HE_ENTITY == ent->entity_type) && 
		(SA_PLM_OPERATIONAL_ENABLED == 
		ent->entity.he_entity.saPlmHEOperationalState)) || 
		((PLMS_EE_ENTITY == ent->entity_type) && 
		(SA_PLM_OPERATIONAL_ENABLED ==
		ent->entity.ee_entity.saPlmEEOperationalState))) {
		
		TRACE("Operational state is already enabled.");
		{
			evt.req_res = PLMS_RES;
			evt.res_evt.res_type = 
			PLMS_AGENT_TRACK_READINESS_IMPACT_RES;
			
			evt.res_evt.ntf_id = ntf_id;
			evt.res_evt.error = SA_AIS_OK;
			ret_err = plm_send_mds_rsp(cb->mds_hdl,
			NCSMDS_SVC_ID_PLMS, snd_info,&evt);
			
			if(NCSCC_RC_SUCCESS != ret_err){
				LOG_ER("Sync resp to PLMA for readiness impact\
				fault clear FAILED. ret_val = %d",ret_err);
			}
		}
		TRACE_LEAVE2("Return Val: %d",NCSCC_RC_SUCCESS);
		/* TODO: Resp to PLMA.*/
		return NCSCC_RC_SUCCESS;
	}

	/* If imminent failure flag is set, then the fault is yet to
	 * happen. Then what to clear. Return from here.
	 * */
	if (plms_rdness_flag_is_set(ent,SA_PLM_RF_IMMINENT_FAILURE)){
		TRACE("Imminent failure flag is set implies the fault is\
		yet to happen. Return.");
		{
			evt.req_res = PLMS_RES;
			evt.res_evt.res_type = 
			PLMS_AGENT_TRACK_READINESS_IMPACT_RES;
			
			evt.res_evt.ntf_id = ntf_id;
			evt.res_evt.error = SA_AIS_OK;
			ret_err = plm_send_mds_rsp(cb->mds_hdl,
			NCSMDS_SVC_ID_PLMS, snd_info,&evt);
			
			if(NCSCC_RC_SUCCESS != ret_err){
				LOG_ER("Sync resp to PLMA for readiness impact\
				fault clear FAILED. ret_val = %d",ret_err);
			}
		}
		TRACE_LEAVE2("Return Val: %d",NCSCC_RC_SUCCESS);
		/* TODO: Resp to PLMA.*/
		return NCSCC_RC_SUCCESS;
	}

	if (ent->am_i_aff_ent){
		TRACE("Imminent failure request can not be set as the \
		ent %s is in some other operational context.",
		ent->dn_name_str);

		evt.req_res = PLMS_RES;
		evt.res_evt.res_type = 
		PLMS_AGENT_TRACK_READINESS_IMPACT_RES;
		
		evt.res_evt.ntf_id = ntf_id;
		evt.res_evt.error = SA_AIS_ERR_TRY_AGAIN;
		ret_err = plm_send_mds_rsp(cb->mds_hdl,
		NCSMDS_SVC_ID_PLMS,snd_info,&evt);
		if(NCSCC_RC_SUCCESS != ret_err){
			LOG_ER("Sync resp to PLMA for readiness impact \
			fault clear FAILED. ret_val = %d",ret_err);
		}
		TRACE_LEAVE2("Return Val: %d",NCSCC_RC_FAILURE);
		return NCSCC_RC_FAILURE;
	}

	/* Mark the operational state of the entity to enabled.
	 *
	 * As part of the isolation, the entity should have been in inactive
	 * uninstantiated state. As the fault is cleared, make the presence
	 * state of the ent to active/instantiated state only if the admin
	 * state is not lock-inactive/lock-instantiation.*/
	
	ntf_id = plms_op_state_set(ent,SA_PLM_OPERATIONAL_ENABLED,
			NULL,SA_NTF_OBJECT_OPERATION,
			SA_PLM_NTFID_STATE_CHANGE_ROOT);

	/* Check is the imminet-failure flag of the entity can be set.*/
	plms_dep_immi_flag_check_and_set(ent,&aff_ent_list,&is_set);

	/* TODO: If the entity is already in active/instantiated state,
	then we need to move the ent to insvc, but then if the opetaional
	state is disable ==> the ent must be in inactive/uninstantiated
	state. FIX this only if needed.*/
	ret_err = plms_ent_enable(ent,FALSE,TRUE);
	if(NCSCC_RC_SUCCESS != ret_err){
		LOG_ER("Fault cleared but inst/act the entity %s FAILED.", ent->dn_name_str);
	}else{
		/* There is no change in readiness status in this context. Hence store the context,
		and use the cause when the EE/HE is instantiated/activated.*/
		trk_info_flt =(PLMS_TRACK_INFO *)calloc(1,sizeof(PLMS_TRACK_INFO));
		if (NULL == trk_info_flt){
			LOG_CR("Rdness Impact: calloc of track info failed.error: %s",strerror(errno));
			assert(0);	
		}
		memset(trk_info_flt,0,sizeof(PLMS_TRACK_INFO));
		trk_info_flt->imm_adm_opr_id = 0; 
		trk_info_flt->track_cause = SA_PLM_CAUSE_FAILURE_CLEARED;
		trk_info_flt->root_entity = ent;
		ent->trk_info = trk_info_flt;
	}
	if (!is_set){
		evt.req_res = PLMS_RES;
		evt.res_evt.res_type = PLMS_AGENT_TRACK_READINESS_IMPACT_RES;
		evt.res_evt.ntf_id = ntf_id;
		evt.res_evt.error = SA_AIS_OK;
		ret_err = plm_send_mds_rsp(cb->mds_hdl,NCSMDS_SVC_ID_PLMS,snd_info,&evt);
		if(NCSCC_RC_SUCCESS != ret_err){
			LOG_ER("Sync resp to PLMA for readiness impact fault clear FAILED.ret_val=%d",ret_err);
		}
		return ret_err;
	}
	/* Overwrite the expected readiness state and flag of the 
	root entity.*/
	plms_ent_exp_rdness_state_ow(ent);
	
	/* Overwrite the expected readiness status with the current.*/
	plms_aff_ent_exp_rdness_state_ow(aff_ent_list);

	/* Fill the track info.*/
	trk_info.group_info_list = NULL;
	
	/* Add the groups, root entity(ent) belong to.*/
	plms_ent_grp_list_add(ent,&(trk_info.group_info_list));
	
	/* Find out all the groups, all affected entities belong to and add 
	the groups to trk_info->group_info_list.
	*/
	if (NULL != aff_ent_list){
		trk_info.aff_ent_list = aff_ent_list;
		plms_ent_list_grp_list_add(aff_ent_list,&(trk_info.group_info_list));	
	}
	
	TRACE("Affected groups for ent %s: ",ent->dn_name_str);
	log_head_grp = trk_info.group_info_list;
	while(log_head_grp){
		TRACE("%llu,",log_head_grp->ent_grp_inf->entity_grp_hdl);
		log_head_grp = log_head_grp->next;
	}
	
	trk_info.change_step = SA_PLM_CHANGE_COMPLETED;
	trk_info.track_cause = SA_PLM_CAUSE_FAILURE_CLEARED;
	trk_info.root_correlation_id = ntf_id;
	trk_info.grp_op = SA_PLM_GROUP_MEMBER_READINESS_CHANGE; 
	trk_info.root_entity = ent;

	plms_cbk_call(&trk_info,1);
	
	plms_ent_exp_rdness_status_clear(ent);
	plms_aff_ent_exp_rdness_status_clear(aff_ent_list);
	
	plms_ent_list_free(aff_ent_list);
	plms_ent_grp_list_free(trk_info.group_info_list);
	
	/* Resp to PLMA.*/
	{
		evt.req_res = PLMS_RES;
		evt.res_evt.res_type = PLMS_AGENT_TRACK_READINESS_IMPACT_RES;
		evt.res_evt.ntf_id = ntf_id;
		evt.res_evt.error = SA_AIS_OK;
		ret_err = plm_send_mds_rsp(cb->mds_hdl,NCSMDS_SVC_ID_PLMS,
		snd_info,&evt);
		if(NCSCC_RC_SUCCESS != ret_err){
			LOG_ER("Sync resp to PLMA for readiness impact fault\
			clear FAILED. ret_val = %d",ret_err);
		}
	}
	TRACE_LEAVE2("Return Val: %d",ret_err);
	return ret_err;
}
/******************************************************************************
@brief		: Handle readiness impact api.
@param[in]	: evt - PLMS_EVT received from PLMA reporting readiness impact.
@return		: NCSCC_RC_SUCCESS/ NCSCC_RC_FAILURE
******************************************************************************/
SaUint32T plms_readiness_impact_process(PLMS_EVT *evt)
{
	SaUint32T ret_err;
	PLMS_ENTITY *ent;
	SaUint8T imp_ent[SA_MAX_NAME_LENGTH];
	PLMS_CB *cb = plms_cb;
	PLMS_AGENT_READINESS_IMPACT *rd_impact;
	PLMS_SEND_INFO *snd_info;
	SaNtfIdentifierT ntf_id = SA_NTF_IDENTIFIER_UNUSED;
	PLMS_EVT evt_resp;
	SaNameT name_key;
	
	snd_info = &(evt->sinfo);
	rd_impact = &(evt->req_evt.agent_track.readiness_impact); 
		
	memcpy(imp_ent,rd_impact->impacted_entity->value,
	rd_impact->impacted_entity->length);
	imp_ent[rd_impact->impacted_entity->length]= '\0';

	TRACE_ENTER2("Entity: %s, impact: %d",imp_ent,rd_impact->impact);

	/* TODO: Application calls readiness impact with plm handle,
	 * Is agent taking care of that handle or PLMS has to ???*/

	/* TODO: Should I reject if this entity or any of the dependent/
	 * children as undergoing any other operation.*/
	
	memset(&name_key,0,sizeof(SaNameT));
	name_key.length = rd_impact->impacted_entity->length;
	memcpy(name_key.value,rd_impact->impacted_entity->value,
	name_key.length);

	ent = (PLMS_ENTITY *)ncs_patricia_tree_get(&(cb->entity_info),
				   (SaUint8T *)(&name_key));
	if (NULL == ent){
		
		LOG_ER("Entity not found in patricia tree. Ent: %s",imp_ent); 
		{
			evt_resp.req_res = PLMS_RES;
			evt_resp.res_evt.res_type = 
			PLMS_AGENT_TRACK_READINESS_IMPACT_RES;
			
			evt_resp.res_evt.ntf_id = ntf_id;
			evt_resp.res_evt.error =  SA_AIS_ERR_NOT_EXIST;
			ret_err = plm_send_mds_rsp(cb->mds_hdl,
			NCSMDS_SVC_ID_PLMS, snd_info,&evt_resp);
			
			if(NCSCC_RC_SUCCESS != ret_err){
				LOG_ER("Sync resp to PLMA for readiness impact\
				fault clear FAILED. ret_val = %d",ret_err);
			}
		}
				
		TRACE_LEAVE2("Return Val: %d",NCSCC_RC_FAILURE);
		return NCSCC_RC_FAILURE;
	}

	switch(rd_impact->impact){
	
	case SA_PLM_RI_IMMINENT_FAILURE:
		ret_err = plms_rdness_imminent_failure_process(ent,snd_info,
		rd_impact->correlation_ids);
		break;
	case SA_PLM_RI_FAILURE:
		ret_err = plms_rdness_failure_process(ent,snd_info,
		rd_impact->correlation_ids);
		break;
	case SA_PLM_RI_IMMINENT_FAILURE_CLEARED:
		ret_err = plms_rdness_imminent_failure_cleared_process(ent,
		snd_info,rd_impact->correlation_ids);
		break;
	case SA_PLM_RI_FAILURE_CLEARED:
		ret_err = plms_rdness_failure_cleared_process(ent,snd_info,
		rd_impact->correlation_ids);
		break;
	default:
		LOG_ER("Invalid rd_impact->impact: %d",rd_impact->impact);
		{
			evt_resp.req_res = PLMS_RES;
			evt_resp.res_evt.res_type = 
			PLMS_AGENT_TRACK_READINESS_IMPACT_RES;
			
			evt_resp.res_evt.ntf_id = ntf_id;
			evt_resp.res_evt.error =  SA_AIS_ERR_INVALID_PARAM;
			ret_err = plm_send_mds_rsp(cb->mds_hdl,
			NCSMDS_SVC_ID_PLMS, snd_info,&evt_resp);
			
			if(NCSCC_RC_SUCCESS != ret_err){
				LOG_ER("Sync resp to PLMA for readiness impact\
				fault clear FAILED. ret_val = %d",ret_err);
			}
		}
		
		TRACE_LEAVE2("Return Val: %d",NCSCC_RC_FAILURE);
		return NCSCC_RC_FAILURE;	
	}
	
	TRACE_LEAVE2("Return Val: %d",ret_err);
	return ret_err;
}

/******************************************************************************
@brief		: Enable the isolated entity. Enabling means activating/instantiating
		the entity depending upon the isolation method.
@param[in]	: ent - HE or EE
@param[in]	: adm_op - is admin operation. Admin-pending flag will be set if
		enabling the entity fails.
@param[in]	: mngt_cbk - If set, call management lost callback if enabling
		the entity fails.
@return		: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.		
******************************************************************************/
SaUint32T plms_ent_enable(PLMS_ENTITY *ent,SaUint32T adm_op, SaUint32T mngt_cbk)
{
	SaUint32T ret_err = NCSCC_RC_SUCCESS;

	TRACE_ENTER2("Entity: %s",ent->dn_name_str);

	if (PLMS_HE_ENTITY == ent->entity_type){
		if ( SA_PLM_HE_ADMIN_LOCKED_INACTIVE != 
			ent->entity.he_entity.saPlmHEAdminState){
			if((PLMS_ISO_DEFAULT == ent->iso_method) || 
				(PLMS_ISO_HE_DEACTIVATED == ent->iso_method)){
				TRACE("Isolation method is deact/default.");
				ret_err = plms_he_activate(ent,adm_op,mngt_cbk);
				if (NCSCC_RC_SUCCESS == ret_err){
					ent->iso_method = PLMS_ISO_DEFAULT;
				}else{
					LOG_ER("HE Activate FAILED. Ent: %s",
					ent->dn_name_str);
				}
				
			}else if(PLMS_ISO_HE_RESET_ASSERT == ent->iso_method){
				TRACE("Isolation method is reset assert.");
				ret_err = plms_he_reset(ent,adm_op,mngt_cbk,
							SAHPI_RESET_DEASSERT);
				if (NCSCC_RC_SUCCESS == ret_err){
					ent->iso_method = PLMS_ISO_DEFAULT;
				}else{
					LOG_ER("HE Reset Deassert FAILED.\
					Ent: %s", ent->dn_name_str);
				}
			}
		}else{
			TRACE(" Admin state of the HE is LOCKED_INACTIVE, \
			can not be enabled. Ent: %s",ent->dn_name_str);
		}
			
	}else{
		if ( SA_PLM_EE_ADMIN_LOCKED_INSTANTIATION != 
			ent->entity.ee_entity.saPlmEEAdminState){
			if ((PLMS_ISO_DEFAULT == ent->iso_method)||
				(PLMS_ISO_EE_TERMINATED == ent->iso_method)){
				TRACE("Isolation method is term/default.");
				ret_err = plms_ee_instantiate(ent,
							adm_op,mngt_cbk);
				if (NCSCC_RC_SUCCESS == ret_err){
					ent->iso_method = PLMS_ISO_DEFAULT;
				}else{
					LOG_ER("EE Instantiation FAILED.\
					Ent: %s",ent->dn_name_str);
				}

			}else if(PLMS_ISO_HE_RESET_ASSERT == ent->iso_method){

				if (NULL == ent->parent)
					return NCSCC_RC_FAILURE;

				TRACE("Isolation method is reset assert.");
				ret_err = plms_he_reset(ent->parent,adm_op,
						mngt_cbk,SAHPI_RESET_DEASSERT);
				if (NCSCC_RC_SUCCESS == ret_err){
					ent->iso_method = PLMS_ISO_DEFAULT;
				}else{
					LOG_ER("HE Reset Deassert FAILED. \
					Ent(EE): %s, Ent(HE): %s", 
					ent->dn_name_str,
					ent->parent->dn_name_str);
				}
				
			}else if(PLMS_ISO_HE_DEACTIVATED == ent->iso_method){

				if (NULL == ent->parent)
					return NCSCC_RC_FAILURE;
					
				TRACE("Isolation method is deactivate");
				ret_err = plms_he_activate(ent->parent,
							adm_op,mngt_cbk);
				 if (NCSCC_RC_SUCCESS == ret_err){
					ent->iso_method = PLMS_ISO_DEFAULT;
				 }else{
					LOG_ER("HE Activate FAILED. \
					Ent(EE): %s, Ent(HE): %s", 
					ent->dn_name_str,
					ent->parent->dn_name_str);
				 }
			}
		}else{
			TRACE(" Admin state of the EE is LOCKED_INST, \
			can not be enabled. Ent: %s",ent->dn_name_str);
		}
	}

	TRACE_LEAVE2("Return Val: %d",ret_err);
	return ret_err;
}

/******************************************************************************
@brief		: Isolate the entity.
		For EE
		1. Terminate the EE.
		2. If termination fails, reset assert the parent HE.
		3. If reset assert fails, deactivate the parent HE.
		For HE
		1. Deactivate the HE.
		2. If deactivation fails, then reset assert the HE.
@param[in]	: ent - HE or EE
@param[in]	: adm_op - is admin operation. Admin-pending flag will be set if
		isolating the entity fails.
@param[in]	: mngt_cbk - If set, call management lost callback if isolating
		the entity fails.
@return		: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.		
******************************************************************************/
SaUint32T plms_ent_isolate(PLMS_ENTITY *ent,SaUint32T adm_op,SaUint32T mngt_cbk)
{
	SaUint32T ret_err,can_isolate = 1;
	PLMS_GROUP_ENTITY *chld_list = NULL,*head;

	/* Isolate the faulty entity. HE->Inactive, EE->terminate.*/
	if (PLMS_EE_ENTITY == ent->entity_type){
		/* Terminate the EE.*/
		ret_err = plms_ee_term(ent,adm_op,0);
		/* If termination fails, then reset assert the parent HE.*/
		if (NCSCC_RC_SUCCESS != ret_err){
			LOG_ER("EE Isolation(term) FAILED. Try reset assert of parent.Ent: %s",ent->dn_name_str);
			/* See any other children of the HE other than myself, 
			 * is insvc.*/
			if (ent->parent){
				plms_chld_get(ent->parent->leftmost_child, &chld_list);
				head = chld_list;
				while(head){
					if ( 0 == strcmp(head->plm_entity->dn_name_str,ent->dn_name_str)){
						head = head->next;
						continue;
					}
					if (plms_is_rdness_state_set(head->plm_entity,SA_PLM_READINESS_IN_SERVICE)){
						can_isolate = 0;
						break;
					}
					head = head->next;
				}
				if(!can_isolate){
					LOG_ER("Not attemptimg EE isolation as dep/child is insvc. Ent: %s", 
					ent->dn_name_str);
				}
			}else{
				LOG_ER("EE can not be isolated. Parent is domain.  Ent: %s",ent->dn_name_str);
				/* EEs parent is domain. No other means of 
				 * isolation.*/
				can_isolate = 0;
			}

			if (NULL != chld_list)
				plms_ent_list_free(chld_list);
			
			if (can_isolate){
				/* assert reset state on parent HE,
				(parent is HE as no virtualization). */
				ret_err = plms_he_reset(ent->parent,0, 1,SAHPI_RESET_ASSERT);

				if( NCSCC_RC_SUCCESS != ret_err){
					LOG_ER("EE isolation (Reset assert of parent HE) FAILED. Deactivate the \
					parent HE.  Ent: %s",ent->dn_name_str);

					/* Deactivate the HE.*/
					ret_err = plms_he_deactivate(ent->parent,0,1);

					if(NCSCC_RC_SUCCESS != ret_err){
						/* Isolation failed.*/
						LOG_ER("Deactivation of parent HE FAILED ==> Isolation of the\
						EE failed. Ent: %s",
						ent->dn_name_str);

						plms_readiness_flag_mark_unmark(ent,SA_PLM_RF_MANAGEMENT_LOST,1,
						NULL,SA_NTF_OBJECT_OPERATION, SA_PLM_NTFID_STATE_CHANGE_ROOT);

						plms_readiness_flag_mark_unmark( ent,SA_PLM_RF_ISOLATE_PENDING,1,
						NULL,SA_NTF_OBJECT_OPERATION, SA_PLM_NTFID_STATE_CHANGE_ROOT);
						
						if (mngt_cbk)
							plms_mngt_lost_clear_cbk_call(ent, 1);
						/* ent->mngt_lost_tri = PLMS_MNGT_EE_ISOLATE; */
								
					}else{
						TRACE("EE isolation (deactivate the parent) SUCCESSFUL. Ent: %s",
						ent->dn_name_str);

						ent->iso_method = PLMS_ISO_HE_DEACTIVATED;
						
						plms_readiness_flag_mark_unmark( ent,SA_PLM_RF_ISOLATE_PENDING,0,
						NULL,SA_NTF_OBJECT_OPERATION, SA_PLM_NTFID_STATE_CHANGE_ROOT);

						/* Clear admin pending  for the EE.*/	
						plms_readiness_flag_mark_unmark( ent,SA_PLM_RF_ADMIN_OPERATION_PENDING,
						0, NULL, SA_NTF_OBJECT_OPERATION, SA_PLM_NTFID_STATE_CHANGE_ROOT);
					
						/* Clear management lost for the EE.*/
						plms_readiness_flag_mark_unmark( ent,SA_PLM_RF_MANAGEMENT_LOST,0,
						NULL, SA_NTF_OBJECT_OPERATION, SA_PLM_NTFID_STATE_CHANGE_ROOT);
					}
				}else{
					TRACE("EE isolation(reset assert of parent) SUCCESSFUL. Ent: %s",
					ent->dn_name_str);
					ent->iso_method = PLMS_ISO_HE_RESET_ASSERT;
					/* Clear isolate pending flag.*/
					plms_readiness_flag_mark_unmark( ent,SA_PLM_RF_ISOLATE_PENDING,0,
					NULL,SA_NTF_OBJECT_OPERATION, SA_PLM_NTFID_STATE_CHANGE_ROOT);
					
					/* Clear admin pending */	
					plms_readiness_flag_mark_unmark(ent, SA_PLM_RF_ADMIN_OPERATION_PENDING,0,
					NULL, SA_NTF_OBJECT_OPERATION, SA_PLM_NTFID_STATE_CHANGE_ROOT);
					
					/* Clear management lost.*/
					plms_readiness_flag_mark_unmark(ent, SA_PLM_RF_MANAGEMENT_LOST,0,NULL,
					SA_NTF_OBJECT_OPERATION, SA_PLM_NTFID_STATE_CHANGE_ROOT);
				}
			}else{
				LOG_ER("EE isolation FAILED. Ent: %s", ent->dn_name_str);
					
				plms_readiness_flag_mark_unmark(ent, SA_PLM_RF_ISOLATE_PENDING,1,NULL,
				SA_NTF_OBJECT_OPERATION, SA_PLM_NTFID_STATE_CHANGE_ROOT);
				
				plms_readiness_flag_mark_unmark(ent, SA_PLM_RF_MANAGEMENT_LOST,1,NULL,
				SA_NTF_OBJECT_OPERATION, SA_PLM_NTFID_STATE_CHANGE_ROOT);

				if (mngt_cbk)
					plms_mngt_lost_clear_cbk_call(ent, 1);
				
				/*ent->mngt_lost_tri = PLMS_MNGT_EE_ISOLATE; */
			}
		}else{
			TRACE(" EE isolation(terminated) SUCCESSFUL. Ent: %s", ent->dn_name_str);
			/* Mark the isolation method.*/
			ent->iso_method = PLMS_ISO_EE_TERMINATED;
			
			/* Clear isolate pending flag.*/
			plms_readiness_flag_mark_unmark( ent,SA_PLM_RF_ISOLATE_PENDING,0,
			NULL,SA_NTF_OBJECT_OPERATION, SA_PLM_NTFID_STATE_CHANGE_ROOT);
				
			/* Clear admin pending */	
			plms_readiness_flag_mark_unmark(ent, SA_PLM_RF_ADMIN_OPERATION_PENDING,0,
			NULL, SA_NTF_OBJECT_OPERATION, SA_PLM_NTFID_STATE_CHANGE_ROOT);
				
			/* Clear management lost.*/
			plms_readiness_flag_mark_unmark(ent, SA_PLM_RF_MANAGEMENT_LOST,0,NULL,
			SA_NTF_OBJECT_OPERATION, SA_PLM_NTFID_STATE_CHANGE_ROOT);
		}
	}else{
		ret_err = plms_he_deactivate(ent,adm_op,1);
		
		if (NCSCC_RC_SUCCESS != ret_err){
			LOG_ER("HE isolation(deact) FAILED. Try reset assert. Ent: %s",ent->dn_name_str);
			ret_err = plms_he_reset(ent,adm_op,1, SAHPI_RESET_ASSERT);
			
			if (NCSCC_RC_SUCCESS != ret_err){
				/* Can not isolate.*/
				LOG_ER("HE isolation(reset assert) FAILED, ISOLATION FAILED. Ent: %s",
				ent->dn_name_str);

				plms_readiness_flag_mark_unmark(ent, SA_PLM_RF_ISOLATE_PENDING,1,NULL,
				SA_NTF_OBJECT_OPERATION, SA_PLM_NTFID_STATE_CHANGE_ROOT);
				
				plms_readiness_flag_mark_unmark(ent, SA_PLM_RF_MANAGEMENT_LOST,1,NULL,
				SA_NTF_OBJECT_OPERATION, SA_PLM_NTFID_STATE_CHANGE_ROOT);
				
				if (mngt_cbk)
					plms_mngt_lost_clear_cbk_call(ent, 1);
				
				/*ent->mngt_lost_tri = PLMS_MNGT_HE_ISOLATE;*/
				
			}else{
				TRACE("HE isolation (reset assert) SUCCESSFUL.");
				ent->iso_method = PLMS_ISO_HE_RESET_ASSERT;
				
				/* Clear isolate pending flag.*/
				plms_readiness_flag_mark_unmark( ent,SA_PLM_RF_ISOLATE_PENDING,0,
				NULL,SA_NTF_OBJECT_OPERATION, SA_PLM_NTFID_STATE_CHANGE_ROOT);
				/* Clear admin pending */	
				plms_readiness_flag_mark_unmark(ent, SA_PLM_RF_ADMIN_OPERATION_PENDING,0,NULL,
				SA_NTF_OBJECT_OPERATION, SA_PLM_NTFID_STATE_CHANGE_ROOT);
				/* Clear management lost.*/
				plms_readiness_flag_mark_unmark(ent, SA_PLM_RF_MANAGEMENT_LOST,0,NULL,
				SA_NTF_OBJECT_OPERATION, SA_PLM_NTFID_STATE_CHANGE_ROOT);
			}
		}else{
			TRACE("HE isolation (deact) SUCCESSFUL.");
			ent->iso_method = PLMS_ISO_HE_DEACTIVATED;
			
			/* Clear isolate pending flag.*/
			plms_readiness_flag_mark_unmark( ent,SA_PLM_RF_ISOLATE_PENDING,0,
			NULL,SA_NTF_OBJECT_OPERATION, SA_PLM_NTFID_STATE_CHANGE_ROOT);
			/* Clear admin pending */	
			plms_readiness_flag_mark_unmark(ent, SA_PLM_RF_ADMIN_OPERATION_PENDING,0,NULL,
			SA_NTF_OBJECT_OPERATION, SA_PLM_NTFID_STATE_CHANGE_ROOT);
			/* Clear management lost.*/
			plms_readiness_flag_mark_unmark(ent, SA_PLM_RF_MANAGEMENT_LOST,0,NULL,
			SA_NTF_OBJECT_OPERATION, SA_PLM_NTFID_STATE_CHANGE_ROOT);
		}
	}

	return ret_err;
}
/******************************************************************************
@brief		: Send async upadte to peer PLMS.
@param[in]	: ent - HE or EE
@param[in]	: step - change step.
@param[in]	: ope_id - Change step virtually the operation id.
@return		: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
******************************************************************************/
SaUint32T plms_peer_async_send(PLMS_ENTITY *ent,SaPlmChangeStepT step,
					SaPlmTrackCauseT ope_id)
{
	SaUint32T ret_err;
	PLMS_MBCSV_MSG msg;
	
	TRACE_ENTER2("Entity: %s",ent->dn_name_str);

	msg.header.msg_type = PLMS_A2S_MSG_TRK_STP_INFO;
	msg.header.num_records = 1;
	msg.info.step_info.dn_name = ent->dn_name;
	msg.info.step_info.change_step = step;
	msg.info.step_info.opr_type = ope_id;

	/* TODO: Not using root-cor-id.*/
	msg.info.step_info.root_correlation_id = SA_NTF_IDENTIFIER_UNUSED;
	
	if( (SA_PLM_CHANGE_VALIDATE == step) || ( (SA_PLM_CHANGE_START == step) && (SA_PLM_CAUSE_SHUTDOWN == ope_id))){
		ret_err = plms_mbcsv_send_async_update(&msg,NCS_MBCSV_ACT_ADD);
	}else if ( SA_PLM_CHANGE_START == step){
		ret_err = plms_mbcsv_send_async_update(&msg,
		NCS_MBCSV_ACT_UPDATE);
	}else{
		ret_err = plms_mbcsv_send_async_update(&msg,NCS_MBCSV_ACT_RMV);
	}

	if (ret_err != NCSCC_RC_SUCCESS){
		LOG_ER("Peer async send FAILED. Ent: %s, step: %d, ope_id: %d",\
		ent->dn_name_str,step,ope_id);
	}

	TRACE_LEAVE2("Return Val: %d",ret_err);
	return ret_err;
}

/******************************************************************************
@brief		: Remove ent from the ent list.
@param[in]	: plm_ent - Entity to be removed.
@param[in/out]	: ent_list - List of entities.
******************************************************************************/
void plms_ent_from_ent_list_rem(PLMS_ENTITY *plm_ent, 
				PLMS_GROUP_ENTITY **ent_list)
{
	PLMS_GROUP_ENTITY *temp, *trav;
	TRACE_ENTER2("entity: %s", plm_ent->dn_name_str);
	if (strcmp((*ent_list)->plm_entity->dn_name_str, plm_ent->dn_name_str)==0){
		TRACE_2("Entity matched in the first node itself");
		temp = *ent_list;
		*ent_list = (*ent_list)->next;
		free(temp);
	}else {
		trav = *ent_list;
		while (trav->next != NULL) {
			temp = trav->next;
			if (strcmp(temp->plm_entity->dn_name_str, plm_ent->dn_name_str)==0) {
				TRACE_2("Entity to be removed found");
				trav->next = temp->next;
				free(temp);
				break;
			}
			trav = trav->next;
		}
	}
	TRACE_LEAVE();
}
/******************************************************************************
@brief		: Timer handler called on timer expiry.
@param[in]	: sig - signal no.
@param[in]	: info - contains the timer info. As we pack the ent itself as
		the context info when starting the timer, same we can retrieve
		when the timer gets expired.
@param[in]	: r_cnxt - Not used. 		
******************************************************************************/
void plms_timer_handler(int sig, siginfo_t *info,void *r_cnxt)
{
	PLMS_EVT *evt;
	PLMS_CB *cb = plms_cb;
	PLMS_ENTITY *ent = (PLMS_ENTITY *)info->si_value.sival_ptr;
	
	evt = (PLMS_EVT *)calloc(1,sizeof(PLMS_EVT));
	 if(NULL == evt){
		 assert(0);
	 }

	 evt->req_res = PLMS_REQ;
	 evt->req_evt.req_type = PLMS_TMR_EVT_T;
	 evt->req_evt.plm_tmr.timer_id = ent->tmr.timer_id;
	 evt->req_evt.plm_tmr.tmr_type = ent->tmr.tmr_type;
	 evt->req_evt.plm_tmr.context_info = (void *)ent;
	 
	 m_NCS_IPC_SEND(&cb->mbx,evt,NCS_IPC_PRIORITY_HIGH);
	 
	return;
}

/******************************************************************************
@brief		: Create and start the timer. The timer value is in nano-sec.
@param[out]	: timer_id - timer id.
@param[in]	: cnxt - cnxt to be used on timer expiry.
@param[in]	: nsec - timer value in nsec.
@return		: NCSCC_RC_FAILURE/NCSCC_RC_SUCCESS
******************************************************************************/
SaUint32T plms_timer_start(timer_t *timer_id,void *cnxt,SaUint64T nsec)
{
	struct itimerspec timer;
	struct sigevent signal_spec;

	signal_spec.sigev_notify = SIGEV_SIGNAL;
	signal_spec.sigev_signo = SIGALRM;
	signal_spec.sigev_value.sival_ptr = cnxt;
	
	/* Create the timer.*/
	if((timer_create(CLOCK_REALTIME, &signal_spec, timer_id)) != 0){
		LOG_ER("Timer Create failed.");
		return NCSCC_RC_FAILURE;
	}
	TRACE("Timer Created, Timer id: %lu \n",(long unsigned)(*timer_id));
	
	/* Start the timer.*/
	timer.it_value.tv_sec = nsec/1000000000;
	timer.it_value.tv_nsec = nsec%1000000000;
	timer.it_interval.tv_sec = 0;
	timer.it_interval.tv_nsec = 0;
	
	if((timer_settime(*timer_id,0,&timer,NULL)) != 0){
		LOG_ER("\nStarting the timer failed, Timer_id: %ld\n",
		(long int)*timer_id);
		timer_delete(*timer_id);
		TRACE("Timer deleted,Timer_id: %lu\n",(long unsigned)*timer_id);
		return NCSCC_RC_FAILURE;
	}
	TRACE("Timer started, Timer id: %lu\n",(long unsigned)*timer_id);
	
	return NCSCC_RC_SUCCESS;
}

/******************************************************************************
@brief		: Stop and delete the timer.
@param[in]	: ent - Entity whose timer is to be deleted.
@return		: NCSCC_RC_SUCCESS.
******************************************************************************/
SaUint32T plms_timer_stop(PLMS_ENTITY *ent)
{
	struct itimerspec timer;

	timer.it_value.tv_sec = 0;
	timer.it_value.tv_nsec = 0;
	timer.it_interval.tv_sec = 0;
	timer.it_interval.tv_nsec = 0;
	
	if((timer_settime(ent->tmr.timer_id,0,&timer,NULL)) != 0){
		TRACE("Stopping the timer failed, Timer_id: %lu.\n",
					(long unsigned)ent->tmr.timer_id);
	}else{
		TRACE("Timer stopped,Timer id: %lu.\n",
		(long unsigned)ent->tmr.timer_id);
	}
	
	/* Delete the timer.*/
	timer_delete(ent->tmr.timer_id);
	/* Clean up the timer context.*/
	ent->tmr.timer_id = 0;
	ent->tmr.tmr_type = PLMS_TMR_NONE;
	ent->tmr.context_info = NULL;
	 
	return NCSCC_RC_SUCCESS;
}

/******************************************************************************
@brief		: Install the timer handler. Used for instantiation failed and
		termination failed timer.
@return		: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE		
******************************************************************************/
SaUint32T plms_tmr_handler_install()
{
	struct sigaction act;
	
	act.sa_sigaction = plms_timer_handler;
	act.sa_flags = SA_SIGINFO;

	if (0 != sigaction(SIGALRM,&act,NULL)){
		LOG_ER("\nSignal Handler Install failed.\n");
		return NCSCC_RC_FAILURE;
	}

	TRACE("\nSignal Handler Installed.\n");
	
	return NCSCC_RC_SUCCESS;
}


/******************************************************************************
@brief		: The funtion is called to clean up the HE database when a
		HE is removed(M0).
		1. Clear the current entity path and current he type.
		2. Update the above to IMM.
		3. Reemove the HE from epath_to_ent tree.
@param[in]	: ent - HE.
@param[in]	: epath_ent - Place holder for epath_to_ent tree.
******************************************************************************/
void plms_he_np_clean_up(PLMS_ENTITY *ent, PLMS_EPATH_TO_ENTITY_MAP_INFO 
					**epath_ent)
{
	SaHpiEntityPathT epath_key;
	SaInt8T *ent_path;
	PLMS_EPATH_TO_ENTITY_MAP_INFO *epath_to_ent;
	PLMS_CB *cb = plms_cb;
	SaInt8T ret_err;
	SaUint32T (* hsm_func_ptr)(SaInt8T *epath_str,SaHpiEntityPathT *epath_ptr) = NULL;
	
	if ( NULL == *epath_ent ){
		/* For not present HEs.*/
		if (NULL == ent->entity.he_entity.saPlmHECurrEntityPath)
			return;
			
		ent_path = ent->entity.he_entity.saPlmHECurrEntityPath;
	
		hsm_func_ptr = dlsym(cb->hpi_intf_hdl, "convert_string_to_epath");
		if ( NULL == hsm_func_ptr ) {
			LOG_ER("dlsym() of HPI lib failed with error %s", dlerror());
			return;
		}
		ret_err = (* hsm_func_ptr)(ent_path,&epath_key);
		if ( NCSCC_RC_SUCCESS != ret_err  ){
			LOG_ER("Unable to dlsym the hsm library");
			return;
		}
		 
		epath_to_ent = (PLMS_EPATH_TO_ENTITY_MAP_INFO *)
		ncs_patricia_tree_get(
		&(cb->epath_to_entity_map_info),(SaUint8T *)&epath_key);
	
		if ( NULL == epath_to_ent )
			return;
	}else{
		epath_to_ent = *epath_ent;
	}
	
	TRACE("Remove the HE %s from epath_to_ent tree.",ent->dn_name_str);
	ret_err = ncs_patricia_tree_del(&(cb->epath_to_entity_map_info),
	(NCS_PATRICIA_NODE *)&(epath_to_ent->pat_node));

	epath_to_ent->plms_entity = NULL;
	free(epath_to_ent->entity_path);
	epath_to_ent->entity_path = NULL;
	free(epath_to_ent);
	epath_to_ent = NULL;
		
	/* Update IMM.*/
	plms_attr_saname_imm_update(ent,"saPlmHECurrHEType",
	ent->entity.he_entity.saPlmHECurrHEType,SA_IMM_ATTR_VALUES_DELETE);
		
	TRACE("Wipe out current HE type. Ent: %s",ent->dn_name_str);
	ent->entity.he_entity.saPlmHECurrHEType.length = 0;
	memset(ent->entity.he_entity.saPlmHECurrHEType.value,0,
	SA_MAX_NAME_LENGTH);
	
	/* Update IMM*/
	plms_attr_sastring_imm_update(ent,
	"saPlmHECurrEntityPath",
	ent->entity.he_entity.saPlmHECurrEntityPath,
	SA_IMM_ATTR_VALUES_DELETE);
		
	TRACE("Wipe out current entity path. Ent: %s",ent->dn_name_str);
	if ( ent->entity.he_entity.saPlmHECurrEntityPath ){
		free(ent->entity.he_entity.saPlmHECurrEntityPath);
		ent->entity.he_entity.saPlmHECurrEntityPath = NULL;
	}
	

	ent->deact_in_pro = FALSE;
	ent->act_in_pro = FALSE;

	return;
}
/******************************************************************************
@brief		: Check if the dep-imminet-failure flag can be set when the 
		operational state is marked to enabled.
@param[in]	: ent - Entity.
@param[out]	: aff_ent_list - List of affected entities.
@param[out]	: is_set - If the flag is set.
******************************************************************************/
void plms_dep_immi_flag_check_and_set(PLMS_ENTITY *ent, PLMS_GROUP_ENTITY **aff_ent_list,SaUint32T *is_set)
{
	PLMS_GROUP_ENTITY *head = NULL;
	PLMS_ENTITY *anc = NULL;
	SaUint32T count = 0, to_b_set = 0;
	
	*is_set = to_b_set;
	/* If the min dep criteria is met, then have to clear the
	dep-imminet-failure flag.*/
	head = ent->dependency_list;
	while(head){
		
		if ( ((PLMS_HE_ENTITY == head->plm_entity->entity_type) && 
		(SA_PLM_OPERATIONAL_ENABLED == head->plm_entity->entity.he_entity.saPlmHEOperationalState)) || 
		((PLMS_EE_ENTITY == head->plm_entity->entity_type) && 
		SA_PLM_OPERATIONAL_ENABLED == head->plm_entity->entity.ee_entity.saPlmEEOperationalState)){

			if (!plms_rdness_flag_is_set(head->plm_entity,SA_PLM_RF_DEPENDENCY_IMMINENT_FAILURE) &&
			!(plms_rdness_flag_is_set(head->plm_entity,SA_PLM_RF_IMMINENT_FAILURE))){
				count++;
			}
		}
		head = head->next;
	}
	if (count >= ent->min_no_dep){
		/* Min dependency is OK. Check for ancestors.*/
		anc = ent->parent;
		while (anc){
			if ( plms_rdness_flag_is_set(anc,SA_PLM_RF_IMMINENT_FAILURE) || 
			plms_rdness_flag_is_set(anc,SA_PLM_RF_DEPENDENCY_IMMINENT_FAILURE)){
				to_b_set = 1;
				break;
			}
			anc = anc->parent;
		}
	}else{
		to_b_set = 1;
	}
	
	if (to_b_set){
		
		if (!plms_rdness_flag_is_set(ent,SA_PLM_RF_DEPENDENCY_IMMINENT_FAILURE)){
			
			*is_set = to_b_set;
			
			plms_readiness_flag_mark_unmark(ent,SA_PLM_RF_DEPENDENCY_IMMINENT_FAILURE,
					1/*mark*/,NULL,SA_NTF_OBJECT_OPERATION,
					SA_PLM_NTFID_STATE_CHANGE_ROOT);
		
			plms_aff_dep_list_imnt_failure_get(ent,ent->rev_dep_list,1,aff_ent_list);

			head = *aff_ent_list;
			while (head){
				plms_readiness_flag_mark_unmark(head->plm_entity,
					SA_PLM_RF_DEPENDENCY_IMMINENT_FAILURE,
					1/*mark*/,ent,SA_NTF_OBJECT_OPERATION,
					SA_PLM_NTFID_STATE_CHANGE_DEP);
				head = head->next;
			}
		}
	}

	return;
}
