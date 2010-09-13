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
*                                                                            *
*  MODULE NAME:  plms_adm_fsm.c                                              *
*                                                                            *
*                                                                            *
*  DESCRIPTION: This file contains PLMS HE and EE admin state FSM. 	     *
*                                                                            *
*****************************************************************************/

#include "plms.h"
#include "plms_mbcsv.h"
#include "plms_utils.h"

EXTERN_C PLMS_ADM_FUNC_PTR plm_HE_adm_state_op[SA_PLM_HE_ADMIN_STATE_MAX]
						[SA_PLM_ADMIN_OP_MAX];

EXTERN_C PLMS_ADM_FUNC_PTR plm_EE_adm_state_op[SA_PLM_EE_ADMIN_STATE_MAX]
						[SA_PLM_ADMIN_OP_MAX];

/* HE adm FP.*/
SaUint32T plms_HE_adm_unlock_state_unlock_op(PLMS_EVT *);
SaUint32T plms_HE_adm_unlock_state_lock_op(PLMS_EVT *);
SaUint32T plms_HE_adm_unlock_state_shutdown_op(PLMS_EVT *);
SaUint32T plms_HE_adm_unlock_state_act_op(PLMS_EVT *);
SaUint32T plms_HE_adm_unlock_state_deact_op(PLMS_EVT *);

SaUint32T plms_HE_adm_locked_state_unlock_op(PLMS_EVT *);
SaUint32T plms_HE_adm_locked_state_lock_op(PLMS_EVT *);
SaUint32T plms_HE_adm_locked_state_shutdown_op(PLMS_EVT *);
SaUint32T plms_HE_adm_locked_state_act_op(PLMS_EVT *);
SaUint32T plms_HE_adm_locked_state_deact_op(PLMS_EVT *);

SaUint32T plms_HE_adm_shutdown_state_unlock_op(PLMS_EVT *);
SaUint32T plms_HE_adm_shutdown_state_lock_op(PLMS_EVT *);
SaUint32T plms_HE_adm_shutdown_state_shutdown_op(PLMS_EVT *);
SaUint32T plms_HE_adm_shutdown_state_act_op(PLMS_EVT *);
SaUint32T plms_HE_adm_shutdown_state_deact_op(PLMS_EVT *);

SaUint32T plms_HE_adm_lckinact_state_unlock_op(PLMS_EVT *);
SaUint32T plms_HE_adm_lckinact_state_lock_op(PLMS_EVT *);
SaUint32T plms_HE_adm_lckinact_state_shutdown_op(PLMS_EVT *);
SaUint32T plms_HE_adm_lckinact_state_act_op(PLMS_EVT *);
SaUint32T plms_HE_adm_lckinact_state_deact_op(PLMS_EVT *);

/* EE adm FP.*/
SaUint32T plms_EE_adm_unlock_state_unlock_op(PLMS_EVT *);
SaUint32T plms_EE_adm_unlock_state_lock_op(PLMS_EVT *);
SaUint32T plms_EE_adm_unlock_state_shutdown_op(PLMS_EVT *);
SaUint32T plms_EE_adm_unlock_state_unlckinst_op(PLMS_EVT *);
SaUint32T plms_EE_adm_unlock_state_lckinst_op(PLMS_EVT *);

SaUint32T plms_EE_adm_locked_state_unlock_op(PLMS_EVT *);
SaUint32T plms_EE_adm_locked_state_lock_op(PLMS_EVT *);
SaUint32T plms_EE_adm_locked_state_shutdown_op(PLMS_EVT *);
SaUint32T plms_EE_adm_locked_state_unlckinst_op(PLMS_EVT *);
SaUint32T plms_EE_adm_locked_state_lckinst_op(PLMS_EVT *);

SaUint32T plms_EE_adm_shutdown_state_unlock_op(PLMS_EVT *);
SaUint32T plms_EE_adm_shutdown_state_lock_op(PLMS_EVT *);
SaUint32T plms_EE_adm_shutdown_state_shutdown_op(PLMS_EVT *);
SaUint32T plms_EE_adm_shutdown_state_unlckinst_op(PLMS_EVT *);
SaUint32T plms_EE_adm_shutdown_state_lckinst_op(PLMS_EVT *);

SaUint32T plms_EE_adm_lckinst_state_unlock_op(PLMS_EVT *);
SaUint32T plms_EE_adm_lckinst_state_lock_op(PLMS_EVT *);
SaUint32T plms_EE_adm_lckinst_state_shutdown_op(PLMS_EVT *);
SaUint32T plms_EE_adm_lckinst_state_unlckinst_op(PLMS_EVT *);
SaUint32T plms_EE_adm_lckinst_state_lckinst_op(PLMS_EVT *);

/* Other prototypes.*/
static SaUint32T plms_ent_unlock_to_shutdown(PLMS_EVT *);
static SaUint32T plms_ent_shutdown_to_lock( PLMS_EVT *);
static SaUint32T plms_ent_shutdown_to_unlock( PLMS_EVT *);
static SaUint32T plms_ent_locked_to_unlock( PLMS_EVT *);
static SaUint32T plms_try_lock(PLMS_ENTITY *,PLMS_IMM_ADMIN_OP,
					PLMS_GROUP_ENTITY *);
static SaUint32T plms_default_lock(PLMS_ENTITY *,PLMS_IMM_ADMIN_OP,
					PLMS_GROUP_ENTITY *);

static SaUint32T plms_forced_lock(PLMS_ENTITY *,PLMS_IMM_ADMIN_OP,
					PLMS_GROUP_ENTITY *);

static SaUint32T plms_forced_lock_cbk( PLMS_ENTITY *,PLMS_TRACK_INFO *, 
					PLMS_IMM_ADMIN_OP, SaUint32T,SaUint32T);

static SaUint32T plms_ent_unlock( PLMS_ENTITY *,PLMS_TRACK_INFO *,
				PLMS_IMM_ADMIN_OP ,SaUint32T,SaUint32T );

static SaUint32T plms_ee_graceful_restart_process(PLMS_ENTITY *, PLMS_EVT *,
PLMS_GROUP_ENTITY *);
static SaUint32T plms_ee_abrupt_restart_process(PLMS_ENTITY *, PLMS_EVT *,
PLMS_GROUP_ENTITY *);

static void plms_shutdown_completed_cbk_call(PLMS_ENTITY *,PLMS_TRACK_INFO  *);
static void plms_lock_completed_cbk_call(PLMS_ENTITY *, PLMS_TRACK_INFO *);
static void plms_lock_start_cbk_call(PLMS_ENTITY *, PLMS_TRACK_INFO *);
/******************************************************************************
@brief		: Initializes the HE admin FSM function pointers.
@param[in]	: plm_HE_adm_state_op - Array of function pointers of type 
                  PLMS_ADM_FUNC_PTR representing HE adm FSM. 
******************************************************************************/
void plms_he_adm_fsm_init(PLMS_ADM_FUNC_PTR plm_HE_adm_state_op[]
						[SA_PLM_ADMIN_OP_MAX])
{
	TRACE_ENTER2();
	
	plm_HE_adm_state_op[SA_PLM_HE_ADMIN_UNLOCKED][SA_PLM_ADMIN_UNLOCK] = 
					plms_HE_adm_unlock_state_unlock_op;
	plm_HE_adm_state_op[SA_PLM_HE_ADMIN_UNLOCKED][SA_PLM_ADMIN_LOCK] = 
					plms_HE_adm_unlock_state_lock_op;
	plm_HE_adm_state_op[SA_PLM_HE_ADMIN_UNLOCKED][SA_PLM_ADMIN_SHUTDOWN] = 
					plms_HE_adm_unlock_state_shutdown_op;
	plm_HE_adm_state_op[SA_PLM_HE_ADMIN_UNLOCKED][SA_PLM_ADMIN_ACTIVATE] = 
					plms_HE_adm_unlock_state_act_op;
	plm_HE_adm_state_op[SA_PLM_HE_ADMIN_UNLOCKED][SA_PLM_ADMIN_DEACTIVATE] =
					plms_HE_adm_unlock_state_deact_op;


	plm_HE_adm_state_op[SA_PLM_HE_ADMIN_LOCKED][SA_PLM_ADMIN_UNLOCK] = 
					plms_HE_adm_locked_state_unlock_op;
	plm_HE_adm_state_op[SA_PLM_HE_ADMIN_LOCKED][SA_PLM_ADMIN_LOCK] =
					plms_HE_adm_locked_state_lock_op;
	plm_HE_adm_state_op[SA_PLM_HE_ADMIN_LOCKED][SA_PLM_ADMIN_SHUTDOWN] = 
					plms_HE_adm_locked_state_shutdown_op;
	plm_HE_adm_state_op[SA_PLM_HE_ADMIN_LOCKED][SA_PLM_ADMIN_ACTIVATE] = 
					plms_HE_adm_locked_state_act_op;
	plm_HE_adm_state_op[SA_PLM_HE_ADMIN_LOCKED][SA_PLM_ADMIN_DEACTIVATE] = 
					plms_HE_adm_locked_state_deact_op;


	plm_HE_adm_state_op[SA_PLM_HE_ADMIN_SHUTTING_DOWN][SA_PLM_ADMIN_UNLOCK]
					= plms_HE_adm_shutdown_state_unlock_op;
	plm_HE_adm_state_op[SA_PLM_HE_ADMIN_SHUTTING_DOWN][SA_PLM_ADMIN_LOCK] = 
					plms_HE_adm_shutdown_state_lock_op;
	plm_HE_adm_state_op[SA_PLM_HE_ADMIN_SHUTTING_DOWN]
		[SA_PLM_ADMIN_SHUTDOWN] = plms_HE_adm_shutdown_state_shutdown_op;
	plm_HE_adm_state_op[SA_PLM_HE_ADMIN_SHUTTING_DOWN]
		[SA_PLM_ADMIN_ACTIVATE] = plms_HE_adm_shutdown_state_act_op;
	plm_HE_adm_state_op[SA_PLM_HE_ADMIN_SHUTTING_DOWN]
		[SA_PLM_ADMIN_DEACTIVATE] = plms_HE_adm_shutdown_state_deact_op;


	plm_HE_adm_state_op[SA_PLM_HE_ADMIN_LOCKED_INACTIVE]
		[SA_PLM_ADMIN_UNLOCK] = plms_HE_adm_lckinact_state_unlock_op;
	plm_HE_adm_state_op[SA_PLM_HE_ADMIN_LOCKED_INACTIVE][SA_PLM_ADMIN_LOCK]
					= plms_HE_adm_lckinact_state_lock_op;
	plm_HE_adm_state_op[SA_PLM_HE_ADMIN_LOCKED_INACTIVE]
		[SA_PLM_ADMIN_SHUTDOWN] = plms_HE_adm_lckinact_state_shutdown_op;
	plm_HE_adm_state_op[SA_PLM_HE_ADMIN_LOCKED_INACTIVE]
		[SA_PLM_ADMIN_ACTIVATE] = plms_HE_adm_lckinact_state_act_op;
	plm_HE_adm_state_op[SA_PLM_HE_ADMIN_LOCKED_INACTIVE]
		[SA_PLM_ADMIN_DEACTIVATE] = plms_HE_adm_lckinact_state_deact_op;

	TRACE_LEAVE2();	
	return;
}

/******************************************************************************
@brief		: Initializes the EE admin FSM function pointers.
@param[in]	: plm_EE_adm_state_op - Array of function pointers of type 
                  PLMS_ADM_FUNC_PTR representing EE adm FSM. 
******************************************************************************/
void plms_ee_adm_fsm_init(PLMS_ADM_FUNC_PTR plm_EE_adm_state_op[]
						[SA_PLM_ADMIN_OP_MAX])
{
	TRACE_ENTER2();

	plm_EE_adm_state_op[SA_PLM_EE_ADMIN_UNLOCKED]
		[SA_PLM_ADMIN_UNLOCK] = plms_EE_adm_unlock_state_unlock_op;
	plm_EE_adm_state_op[SA_PLM_EE_ADMIN_UNLOCKED]
		[SA_PLM_ADMIN_LOCK]=plms_EE_adm_unlock_state_lock_op;
	plm_EE_adm_state_op[SA_PLM_EE_ADMIN_UNLOCKED]
		[SA_PLM_ADMIN_SHUTDOWN]=plms_EE_adm_unlock_state_shutdown_op;
	plm_EE_adm_state_op[SA_PLM_EE_ADMIN_UNLOCKED]
				[SA_PLM_ADMIN_UNLOCK_INSTANTIATION]=
					plms_EE_adm_unlock_state_unlckinst_op;
	plm_EE_adm_state_op[SA_PLM_EE_ADMIN_UNLOCKED]
				[SA_PLM_ADMIN_LOCK_INSTANTIATION]=
					plms_EE_adm_unlock_state_lckinst_op;


	plm_EE_adm_state_op[SA_PLM_EE_ADMIN_LOCKED]
		[SA_PLM_ADMIN_UNLOCK]=plms_EE_adm_locked_state_unlock_op;
	plm_EE_adm_state_op[SA_PLM_EE_ADMIN_LOCKED]
		[SA_PLM_ADMIN_LOCK]=plms_EE_adm_locked_state_lock_op;
	plm_EE_adm_state_op[SA_PLM_EE_ADMIN_LOCKED]
		[SA_PLM_ADMIN_SHUTDOWN]=plms_EE_adm_locked_state_shutdown_op;
	plm_EE_adm_state_op[SA_PLM_EE_ADMIN_LOCKED]
				[SA_PLM_ADMIN_UNLOCK_INSTANTIATION]=
					plms_EE_adm_locked_state_unlckinst_op;
	plm_EE_adm_state_op[SA_PLM_EE_ADMIN_LOCKED]
				[SA_PLM_ADMIN_LOCK_INSTANTIATION]=
					plms_EE_adm_locked_state_lckinst_op;

	plm_EE_adm_state_op[SA_PLM_EE_ADMIN_SHUTTING_DOWN]
		[SA_PLM_ADMIN_UNLOCK]=plms_EE_adm_shutdown_state_unlock_op;
	plm_EE_adm_state_op[SA_PLM_EE_ADMIN_SHUTTING_DOWN]
		[SA_PLM_ADMIN_LOCK]=plms_EE_adm_shutdown_state_lock_op;
	plm_EE_adm_state_op[SA_PLM_EE_ADMIN_SHUTTING_DOWN]
		[SA_PLM_ADMIN_SHUTDOWN]=plms_EE_adm_shutdown_state_shutdown_op;
	plm_EE_adm_state_op[SA_PLM_EE_ADMIN_SHUTTING_DOWN]
				[SA_PLM_ADMIN_UNLOCK_INSTANTIATION]=
					plms_EE_adm_shutdown_state_unlckinst_op;
	plm_EE_adm_state_op[SA_PLM_EE_ADMIN_SHUTTING_DOWN]
				[SA_PLM_ADMIN_LOCK_INSTANTIATION]=
					plms_EE_adm_shutdown_state_lckinst_op;


	plm_EE_adm_state_op[SA_PLM_EE_ADMIN_LOCKED_INSTANTIATION]
		[SA_PLM_ADMIN_UNLOCK]=plms_EE_adm_lckinst_state_unlock_op;
	plm_EE_adm_state_op[SA_PLM_EE_ADMIN_LOCKED_INSTANTIATION]
		[SA_PLM_ADMIN_LOCK]=plms_EE_adm_lckinst_state_lock_op;
	plm_EE_adm_state_op[SA_PLM_EE_ADMIN_LOCKED_INSTANTIATION]
		[SA_PLM_ADMIN_SHUTDOWN]=plms_EE_adm_lckinst_state_shutdown_op;
	plm_EE_adm_state_op[SA_PLM_EE_ADMIN_LOCKED_INSTANTIATION]
				[SA_PLM_ADMIN_UNLOCK_INSTANTIATION]=
					plms_EE_adm_lckinst_state_unlckinst_op;
	plm_EE_adm_state_op[SA_PLM_EE_ADMIN_LOCKED_INSTANTIATION]
				[SA_PLM_ADMIN_LOCK_INSTANTIATION]=
					plms_EE_adm_lckinst_state_lckinst_op;

	TRACE_LEAVE2();	
	return;
}


/******************************************************************************
@brief		: Unlock admin operation performed on HE which is already in 
                  unlocked state. 
@param[in]	: evt - PLMS evt structure holding IMM admin operation info      
@return		: SaAisErrorT. 
******************************************************************************/
SaUint32T plms_HE_adm_unlock_state_unlock_op(PLMS_EVT *evt)
{
	SaUint32T ret_err = NCSCC_RC_FAILURE;
	PLMS_CB *cb = plms_cb;
	SaInt8T tmp[SA_MAX_NAME_LENGTH +1];

	plms_get_str_from_dn_name(&(evt->req_evt.admin_op.dn_name),tmp);
	TRACE_ENTER2("Entity: %s",tmp);
	
	LOG_ER("Bad Admin Op, unlock to unlock. Ent: %s",tmp);
	ret_err = saImmOiAdminOperationResult(cb->oi_hdl,
				evt->req_evt.admin_op.inv_id,SA_AIS_ERR_NO_OP);  
	if (SA_AIS_OK != ret_err) {
		LOG_ER("Sending response to IMM failed. Ent: %s, \
		Operations: Unlock, ret_err: %d",tmp,ret_err);
	}
	
	TRACE_LEAVE2("ret_err: %d",ret_err);
	return ret_err;
}


/******************************************************************************
@brief		: Unlocked ==> Lock
@param[in]	: evt - PLMS evt structure holding IMM admin operation info      
@return		: NCSCC_RC_SUCCESS/SA_AIS_OK(1) - For success.
		  NCSCC_RC_FAILURE(0) - For failure.
		  others(SaAisErrorT) - For failure.
******************************************************************************/
SaUint32T plms_HE_adm_unlock_state_lock_op(PLMS_EVT *evt)
{
	PLMS_ENTITY *ent;
	SaUint32T ret_err = NCSCC_RC_FAILURE;
	PLMS_GROUP_ENTITY *aff_ent_list = NULL;
	PLMS_TRACK_INFO *trk_info;
	PLMS_CB *cb = plms_cb;
	PLMS_ENTITY_GROUP_INFO_LIST *grp_list_head;
	SaInt8T tmp[SA_MAX_NAME_LENGTH +1];

	plms_get_str_from_dn_name(&(evt->req_evt.admin_op.dn_name),tmp);
	TRACE_ENTER2("Entity: %s. Unlock to Lock, Variant: %s",tmp,
	evt->req_evt.admin_op.option);

	ent = (PLMS_ENTITY *)ncs_patricia_tree_get(&(cb->entity_info),
			   (SaUint8T *)&(evt->req_evt.admin_op.dn_name));
	if (NULL == ent) {
		LOG_ER("Ent not found in entity_info tree for dn_name: %s",tmp);
		ret_err = saImmOiAdminOperationResult(cb->oi_hdl,
			evt->req_evt.admin_op.inv_id,SA_AIS_ERR_NO_RESOURCES);
		if (SA_AIS_OK != ret_err){
			LOG_ER("Sending response to IMM \
			failed. Ent: %s, Op: Lock, ret_err: \
			%d",tmp,ret_err);
		}
		TRACE_LEAVE2("ret_err: %d",ret_err);
		return ret_err;
	}
	
	/* If not forced lock, check the feasibility. */
	if ( (0 == (strcmp(evt->req_evt.admin_op.option,
	SA_PLM_ADMIN_LOCK_OPTION_TRYLOCK))) || 
	('\0' == evt->req_evt.admin_op.option[0])){
		/* If I am OOS, then I am the only affected entity.*/
		if ((SA_PLM_READINESS_OUT_OF_SERVICE ==
		ent->entity.he_entity.saPlmHEReadinessState)){ 
			TRACE("Entity %s is already in OOS",tmp);
			if (ent->am_i_aff_ent){
				LOG_ER("Lock Admin operation can not be \
				performed as the entity %s is the affected \
				entity for some other operation.",tmp);
				
				ret_err = saImmOiAdminOperationResult(
				cb->oi_hdl, evt->req_evt.admin_op.inv_id,
				SA_AIS_ERR_TRY_AGAIN);
				if (SA_AIS_OK != ret_err){
					LOG_ER("Sending response to IMM \
					failed. Ent: %s, Op: Lock, ret_err: \
					%d",tmp,ret_err);
				}
				
				TRACE_LEAVE2("ret_err: %d",ret_err);
				return ret_err;
			}else{
				plms_admin_state_set(ent,SA_PLM_HE_ADMIN_LOCKED
				,NULL,SA_NTF_MANAGEMENT_OPERATION,
				SA_PLM_NTFID_STATE_CHANGE_ROOT);
		
				/* Respond to IMM.*/
				ret_err = saImmOiAdminOperationResult(
				cb->oi_hdl, evt->req_evt.admin_op.inv_id,
				SA_AIS_OK);
				
				if (SA_AIS_OK != ret_err){
					LOG_ER("Sending response to IMM failed.\
					Ent: %s, Operation: Lock, ret_err: %d",
					tmp,ret_err);
				}

				TRACE_LEAVE2("ret_err: %d",ret_err);
				return ret_err;
			}
		/* If am not in OOS.*/	
		}else{
			TRACE("Entity %s is not in OOS",tmp);
			/* Get all the affected entities.*/ 
			plms_affected_ent_list_get( ent,&aff_ent_list,0);
			
			if (plms_anc_chld_dep_adm_flag_is_set(
							ent,aff_ent_list)) {
				LOG_ER("Admin operation can not be performed on\
				ent %s as one of the affected entity is in \
				another operation context.",tmp);
				
				ret_err = saImmOiAdminOperationResult(
				cb->oi_hdl, evt->req_evt.admin_op.inv_id,
				SA_AIS_ERR_TRY_AGAIN);
				if (SA_AIS_OK != ret_err){
					LOG_ER("Sending response to IMM \
					failed. Ent: %s, Op: Lock, ret_err: \
					%d",tmp,ret_err);
				}
				
				plms_ent_list_free(aff_ent_list);
				aff_ent_list = NULL;
				
				TRACE_LEAVE2("ret_err: %d",ret_err);
				return ret_err;    
			}

		}
	/* If forced lock.*/	
	}else if (0 == (strcmp(evt->req_evt.admin_op.option,
	SA_PLM_ADMIN_LOCK_OPTION_FORCED))) {  
		/* If I am aff, then I should be in adm context, otherwise
		reject the forced lock.*/
		if (ent->am_i_aff_ent && !(ent->adm_op_in_progress)){
			
			LOG_ER("Forced lock can not be performed on \
			this %s ent. am_i_aff_ent = 1,\
			adm_op_in_progress = 0",ent->dn_name_str);

			ret_err = saImmOiAdminOperationResult(
			cb->oi_hdl, evt->req_evt.admin_op.inv_id,
			SA_AIS_ERR_TRY_AGAIN);

			TRACE_LEAVE2("ret_err: %d",ret_err);
			return ret_err;    
		}
		
		if (SA_PLM_READINESS_OUT_OF_SERVICE !=
				ent->entity.he_entity.saPlmHEReadinessState){
			TRACE("Entity %s is not in OOS",tmp);
			/* Get all the affected entities.*/ 
			plms_affected_ent_list_get(ent, &aff_ent_list,0);
		}else {
			TRACE("Entity %s is in OOS",tmp);
			trk_info = ent->trk_info;
			if (ent->adm_op_in_progress && (NULL != trk_info)){
				TRACE("Admin operation in progress: %d. Ent:%s",
				trk_info->track_cause,
				trk_info->root_entity->dn_name_str);
				
				grp_list_head = trk_info->group_info_list;
				while (grp_list_head){
					/* Remove the inv_to_cbk nodes from the
					grp->innvocation list, if pointing to 
					the trk_info */
					
					plms_inv_to_cbk_in_grp_trk_rmv(
					grp_list_head->ent_grp_inf,trk_info);
					grp_list_head = grp_list_head->next;
				}
				
				plms_aff_ent_exp_rdness_status_clear(
				trk_info->aff_ent_list);
				plms_ent_exp_rdness_status_clear(
				trk_info->root_entity);

				ent->am_i_aff_ent = FALSE;
				ent->adm_op_in_progress = FALSE;
				plms_aff_ent_mark_unmark(trk_info->aff_ent_list,
									FALSE);
				plms_trk_info_free(trk_info);
			}
			/* Clean up done. Process lock operation.*/
			plms_admin_state_set(ent,SA_PLM_HE_ADMIN_LOCKED,NULL,
						SA_NTF_MANAGEMENT_OPERATION,
						SA_PLM_NTFID_STATE_CHANGE_ROOT);
			
			/* Respond to IMM.*/
			ret_err = saImmOiAdminOperationResult(cb->oi_hdl,
					evt->req_evt.admin_op.inv_id,SA_AIS_OK);
			if (SA_AIS_OK != ret_err){
				LOG_ER("Sending response to IMM failed. Ent:%s,\
				Operation: Lock, ret_err: %d",tmp,ret_err);
			}

			TRACE_LEAVE2("ret_err: %d",ret_err);
			return ret_err;
		}
	}else{/* Invalid lock option.*/
		LOG_ER("Invalid Lock option: %s",evt->req_evt.admin_op.option);	
		ret_err = saImmOiAdminOperationResult(cb->oi_hdl,evt->req_evt.admin_op.inv_id,
		SA_AIS_ERR_INVALID_PARAM);
		TRACE_LEAVE2("ret_err: %d",ret_err);
		return ret_err;
	}
	
	if(0 == (strcmp(evt->req_evt.admin_op.option,
		SA_PLM_ADMIN_LOCK_OPTION_TRYLOCK))) {
		/* Admin operation started. */
		ent->adm_op_in_progress = SA_PLM_CAUSE_LOCK;
		ent->am_i_aff_ent = TRUE;
		plms_aff_ent_flag_mark_unmark(aff_ent_list,TRUE);
		
		ret_err = plms_try_lock(ent,evt->req_evt.admin_op,
		aff_ent_list);
		
	} else if(0 == (strcmp(evt->req_evt.admin_op.option,
		SA_PLM_ADMIN_LOCK_OPTION_FORCED))){
		ret_err = plms_forced_lock(ent,evt->req_evt.admin_op,
		aff_ent_list);
		
	} else {/* Default lock.*/
		/* Admin operation started. */
		ent->adm_op_in_progress = SA_PLM_CAUSE_LOCK;
		ent->am_i_aff_ent = TRUE;
		plms_aff_ent_flag_mark_unmark(aff_ent_list,TRUE);
		
		ret_err = plms_default_lock(ent,evt->req_evt.admin_op,
		aff_ent_list);
	}
	TRACE_LEAVE2("ret_err: %d",ret_err);
	return ret_err;
}


/******************************************************************************
@brief		: Unlocked ==> Shutdown 
@param[in]	: evt - PLMS evt structure holding IMM admin operation info      
@return		: NCSCC_RC_SUCCESS/SA_AIS_OK(1) - For success.
		  NCSCC_RC_FAILURE(0) - For failure.
		  others(SaAisErrorT) - For failure.
******************************************************************************/
SaUint32T plms_HE_adm_unlock_state_shutdown_op(PLMS_EVT *evt)
{
	SaUint32T ret_err = NCSCC_RC_FAILURE;
	SaInt8T tmp[SA_MAX_NAME_LENGTH +1];

	plms_get_str_from_dn_name(&(evt->req_evt.admin_op.dn_name),tmp);
	TRACE_ENTER2("Entity: %s. Operation: unlock to shutdown.",tmp);
	
	ret_err = plms_ent_unlock_to_shutdown(evt);
	if (NCSCC_RC_SUCCESS != ret_err){
	}
	
	TRACE_LEAVE2("ret_err: %d",ret_err);
	return ret_err;
}
/******************************************************************************
@brief		: Unlocked ==> Activate 
@param[in]	: evt - PLMS evt structure holding IMM admin operation info      
@return		: SaAisErrorT 
******************************************************************************/
SaUint32T plms_HE_adm_unlock_state_act_op(PLMS_EVT *evt)
{
	SaUint32T ret_err = NCSCC_RC_FAILURE;
	PLMS_CB *cb = plms_cb;
	SaInt8T tmp[SA_MAX_NAME_LENGTH +1];

	plms_get_str_from_dn_name(&(evt->req_evt.admin_op.dn_name),tmp);
	TRACE_ENTER2("Entity: %s",tmp);

	LOG_ER("Bad Admin Op, unlock to activate. Ent: %s",tmp);
	ret_err = saImmOiAdminOperationResult(cb->oi_hdl,
			evt->req_evt.admin_op.inv_id,SA_AIS_ERR_BAD_OPERATION);  
	if (SA_AIS_OK != ret_err) {
		LOG_ER("Sending response to IMM failed. Ent: %s, \
		Operations: Activate, ret_err: %d",tmp,ret_err);
	}

	TRACE_LEAVE2("ret_err: %d",ret_err);
	return ret_err;
}
/******************************************************************************
@brief		: Unlocked ==> Deactivate 
@param[in]	: evt - PLMS evt structure holding IMM admin operation info      
@return		: SaAisErrorT 
******************************************************************************/
SaUint32T plms_HE_adm_unlock_state_deact_op(PLMS_EVT *evt)
{
	SaUint32T ret_err = NCSCC_RC_FAILURE;
	PLMS_CB *cb = plms_cb;
	SaInt8T tmp[SA_MAX_NAME_LENGTH +1];

	plms_get_str_from_dn_name(&(evt->req_evt.admin_op.dn_name),tmp);
	TRACE_ENTER2("Entity: %s",tmp);

	LOG_ER("Bad Admin Op, unlock to deactivate. Ent: %s",tmp);
	ret_err = saImmOiAdminOperationResult(cb->oi_hdl,
			evt->req_evt.admin_op.inv_id,SA_AIS_ERR_BAD_OPERATION);  
	if (SA_AIS_OK != ret_err) {
		LOG_ER("Sending response to IMM failed. Ent: %s, \
		Operations: Deactivate, ret_err: %d",tmp,ret_err);
	}

	TRACE_LEAVE2("ret_err: %d",ret_err);
	return ret_err;
}
/******************************************************************************
@brief		: Locked ==> Unlock 
@param[in]	: evt - PLMS evt structure holding IMM admin operation info      
@return		: NCSCC_RC_SUCCESS/SA_AIS_OK(1) - For success.
		  NCSCC_RC_FAILURE(0) - For failure.
		  others(SaAisErrorT) - For failure.
******************************************************************************/
SaUint32T plms_HE_adm_locked_state_unlock_op(PLMS_EVT *evt)
{

	SaUint32T ret_err = NCSCC_RC_FAILURE;
	SaInt8T tmp[SA_MAX_NAME_LENGTH +1];

	plms_get_str_from_dn_name(&(evt->req_evt.admin_op.dn_name),tmp);
	TRACE_ENTER2("Entity: %s. Operation: lock to unlock.",tmp);

	ret_err = plms_ent_locked_to_unlock(evt);
	if (NCSCC_RC_SUCCESS != ret_err){
	}

	TRACE_LEAVE2("ret_err: %d",ret_err);
	return ret_err;
}
/******************************************************************************
@brief		: Locked ==> Activate 
@param[in]	: evt - PLMS evt structure holding IMM admin operation info      
@return		: SaAisErrorT.
******************************************************************************/
SaUint32T plms_HE_adm_locked_state_act_op(  PLMS_EVT *evt)
{
	SaUint32T ret_err = NCSCC_RC_FAILURE;
	PLMS_CB *cb = plms_cb;
	SaInt8T tmp[SA_MAX_NAME_LENGTH +1];

	plms_get_str_from_dn_name(&(evt->req_evt.admin_op.dn_name),tmp);
	TRACE_ENTER2("Entity: %s",tmp);

	LOG_ER("Bad Admin Op, lock to activate. Ent: %s",tmp);
	ret_err = saImmOiAdminOperationResult(cb->oi_hdl,
			evt->req_evt.admin_op.inv_id,SA_AIS_ERR_NO_OP);  

	if (SA_AIS_OK != ret_err) {
		LOG_ER("Sending response to IMM failed. Ent: %s, \
		Operations: Activate, ret_err: %d",tmp,ret_err);
	}

	TRACE_LEAVE2("ret_err: %d",ret_err);
	return ret_err;
}
/******************************************************************************
@brief		: Locked ==> Shutdown 
@param[in]	: evt - PLMS evt structure holding IMM admin operation info      
@return		: SaAisErrorT.
******************************************************************************/
SaUint32T plms_HE_adm_locked_state_shutdown_op(  PLMS_EVT *evt)
{
	SaUint32T ret_err = NCSCC_RC_FAILURE;
	PLMS_CB *cb = plms_cb;
	SaInt8T tmp[SA_MAX_NAME_LENGTH +1];

	plms_get_str_from_dn_name(&(evt->req_evt.admin_op.dn_name),tmp);
	TRACE_ENTER2("Entity: %s",tmp);

	LOG_ER("Bad Admin Op, lock to shutdown. Ent: %s",tmp);
	ret_err = saImmOiAdminOperationResult(cb->oi_hdl,
			evt->req_evt.admin_op.inv_id,SA_AIS_ERR_BAD_OPERATION);  

	if (SA_AIS_OK != ret_err) {
		LOG_ER("Sending response to IMM failed. Ent: %s, \
		Operations: Shutdown, ret_err: %d",tmp,ret_err);
	}

	TRACE_LEAVE2("ret_err: %d",ret_err);
	return ret_err;
}
/******************************************************************************
@brief		: Locked ==> Lock
		This is possible if any of the application is not responding
		to START callback for the try/default lock and the adminstator
		issues forced lock.
@param[in]	: evt - PLMS evt structure holding IMM admin operation info
@return		: NCSCC_RC_SUCCESS/SA_AIS_OK(1) - For success.
		  NCSCC_RC_FAILURE(0) - For failure.
		  others(SaAisErrorT) - For failure.
******************************************************************************/
SaUint32T plms_HE_adm_locked_state_lock_op(  PLMS_EVT *evt)
{
	
	SaUint32T ret_err = NCSCC_RC_FAILURE;
	SaInt8T tmp[SA_MAX_NAME_LENGTH +1];
	PLMS_CB *cb = plms_cb;

	plms_get_str_from_dn_name(&(evt->req_evt.admin_op.dn_name),tmp);
	TRACE_ENTER2("NO Operation performed. Entity: %s. Operation: lock to lock, Variant: %s",tmp,
	evt->req_evt.admin_op.option);

	ret_err = saImmOiAdminOperationResult(cb->oi_hdl,
			evt->req_evt.admin_op.inv_id,SA_AIS_ERR_NO_OP);  
	if (SA_AIS_OK != ret_err) {
		LOG_ER("Sending response to IMM failed. Ent: %s, \
		Operation: lock to lock, ret_err: %d",tmp,ret_err);
	}

	TRACE_LEAVE2("ret_err: %d",ret_err);
	return ret_err;
}

/******************************************************************************
@brief		: Locked ==> Deactivate 
@param[in]	: evt - PLMS evt structure holding IMM admin operation info
@return		: NCSCC_RC_SUCCESS/SA_AIS_OK(1) - For success.
		  NCSCC_RC_FAILURE(0) - For failure.
		  others(SaAisErrorT) - For failure.
******************************************************************************/
SaUint32T plms_HE_adm_locked_state_deact_op (  PLMS_EVT *evt)
{
	SaUint32T ret_err = NCSCC_RC_FAILURE;
	PLMS_CB *cb = plms_cb;
	PLMS_ENTITY *ent;
	SaInt8T tmp[SA_MAX_NAME_LENGTH +1];

	plms_get_str_from_dn_name(&(evt->req_evt.admin_op.dn_name),tmp);
	TRACE_ENTER2("Entity: %s. Operation: lock to deact.",tmp);

	ent = (PLMS_ENTITY *)ncs_patricia_tree_get(&(cb->entity_info),
			   (SaUint8T *)&(evt->req_evt.admin_op.dn_name));
	if (NULL == ent) {
		LOG_ER("Ent not found in entity_info tree for dn_name: %s",tmp);
		ret_err = saImmOiAdminOperationResult(cb->oi_hdl,
			evt->req_evt.admin_op.inv_id,SA_AIS_ERR_NO_RESOURCES);  
		if (SA_AIS_OK != ret_err){
			LOG_ER("Sending response to IMM failed. Ent: %s, \
			Operation: Deactivate, ret_err: %d",tmp,ret_err);
		}
		TRACE_LEAVE2("ret_err: %d",ret_err);
		return ret_err;
	}
	/* I should not be in mid of any operation.
	*/
	if (ent->am_i_aff_ent){
		LOG_ER("Admin operation can not be performed as the \
		entity %s is in another admin context: %d.",tmp,
		ent->adm_op_in_progress);

		ret_err = saImmOiAdminOperationResult(cb->oi_hdl,
			evt->req_evt.admin_op.inv_id,SA_AIS_ERR_TRY_AGAIN);
		if (SA_AIS_OK != ret_err){
			LOG_ER("Sending response to IMM failed. Ent: %s, \
			Operation: Deactivate, ret_err: %d",tmp,ret_err);
		}
		TRACE_LEAVE2("ret_err: %d",ret_err);
		return ret_err;    
	}
	/* TODO: HE is already in locked state. So all affected entities
	would be in OOS. So the affected entities for this operation are
	none.*/
	/* Mark the admin state to locked-inactive.*/
	plms_admin_state_set(ent, SA_PLM_HE_ADMIN_LOCKED_INACTIVE,NULL,
				SA_NTF_MANAGEMENT_OPERATION,
				SA_PLM_NTFID_STATE_CHANGE_ROOT);

	/* Deactivate the HE. */
	ret_err = plms_he_deactivate(ent,TRUE,TRUE);
	if (NCSCC_RC_SUCCESS != ret_err){
		LOG_ER("Ent %s deactivation failed.",tmp);
		
		ret_err = saImmOiAdminOperationResult(cb->oi_hdl,
			evt->req_evt.admin_op.inv_id,
			SA_AIS_ERR_DEPLOYMENT);
		if (NCSCC_RC_SUCCESS != ret_err){
			LOG_ER("Sending response to IMM failed. Ent: \
			%s,Operation: Deactivate, ret_err: %d",
			tmp,ret_err);
		}
	}else{
		ret_err = saImmOiAdminOperationResult(cb->oi_hdl,
				evt->req_evt.admin_op.inv_id,SA_AIS_OK);
		if (NCSCC_RC_SUCCESS != ret_err){
			LOG_ER("Sending response to IMM failed. Ent: \
			%s, Operation: Deactivate, ret_err: %d",
			tmp,ret_err);
		}
	}

	TRACE_LEAVE2("ret_err: %d",ret_err);
	return ret_err;

}
/******************************************************************************
@brief		: Shutdown ==> Deactivate 
@param[in]	: evt - PLMS evt structure holding IMM admin operation info
@return		: SaAisErrorT.
******************************************************************************/
SaUint32T plms_HE_adm_shutdown_state_deact_op(  PLMS_EVT *evt)
{
	SaUint32T ret_err = NCSCC_RC_FAILURE;
	PLMS_CB *cb = plms_cb;
	SaInt8T tmp[SA_MAX_NAME_LENGTH +1];

	plms_get_str_from_dn_name(&(evt->req_evt.admin_op.dn_name),tmp);
	TRACE_ENTER2("Entity: %s",tmp);

	LOG_ER("Bad Admin Op, shutdown to deactivate. Ent: %s",tmp);
	ret_err = saImmOiAdminOperationResult(cb->oi_hdl,
			evt->req_evt.admin_op.inv_id,SA_AIS_ERR_BAD_OPERATION);  
	if (SA_AIS_OK != ret_err) {
		LOG_ER("Sending response to IMM failed. Ent: %s, \
		Operations: Deactivate, ret_err: %d",tmp,ret_err);
	}

	TRACE_LEAVE2("ret_err: %d",ret_err);
	return ret_err;
}
/******************************************************************************
@brief		: Shutdown ==> Activate 
@param[in]	: evt - PLMS evt structure holding IMM admin operation info
@return		: SaAisErrorT.
******************************************************************************/
SaUint32T plms_HE_adm_shutdown_state_act_op(  PLMS_EVT *evt)
{
	SaUint32T ret_err = NCSCC_RC_FAILURE;
	PLMS_CB *cb = plms_cb;
	SaInt8T tmp[SA_MAX_NAME_LENGTH +1];

	plms_get_str_from_dn_name(&(evt->req_evt.admin_op.dn_name),tmp);
	TRACE_ENTER2("Entity: %s",tmp);

	LOG_ER("Bad Admin Op, shutdown to activate. Ent: %s",tmp);
	ret_err = saImmOiAdminOperationResult(cb->oi_hdl,
			evt->req_evt.admin_op.inv_id,SA_AIS_ERR_BAD_OPERATION);  
	if (SA_AIS_OK != ret_err) {
		LOG_ER("Sending response to IMM failed. Ent: %s, \
		Operations: Activate, ret_err: %d",tmp,ret_err);
	}

	TRACE_LEAVE2("ret_err: %d",ret_err);
	return ret_err;
}
/******************************************************************************
@brief		: Shutdown ==> Shutdown 
@param[in]	: evt - PLMS evt structure holding IMM admin operation info
@return		: SaAisErrorT.
******************************************************************************/
SaUint32T plms_HE_adm_shutdown_state_shutdown_op(  PLMS_EVT *evt)
{
	SaUint32T ret_err = NCSCC_RC_FAILURE;
	PLMS_CB *cb = plms_cb;
	SaInt8T tmp[SA_MAX_NAME_LENGTH +1];

	plms_get_str_from_dn_name(&(evt->req_evt.admin_op.dn_name),tmp);
	TRACE_ENTER2("Entity: %s",tmp);

	LOG_ER("Bad Admin Op, Shutdown to shutdown. Ent: %s",tmp);
	ret_err = saImmOiAdminOperationResult(cb->oi_hdl,
			evt->req_evt.admin_op.inv_id,SA_AIS_ERR_NO_OP);  

	if (SA_AIS_OK != ret_err) {
		LOG_ER("Sending response to IMM failed. Ent: %s, \
		Operations: Shutdown, ret_err: %d",tmp,ret_err);
	}

	TRACE_LEAVE2("ret_err: %d",ret_err);
	return ret_err;
}
/******************************************************************************
@brief		: Shutdown ==> Lock
		The assumption is, the adminstrator can issue only forced
		lock if any of the application does not respond to the start
		callback of the shutdown operation.
		If try/default lock is issued, we will reject although
		spec does not speak this explicitly.
@param[in]	: evt - PLMS evt structure holding IMM admin operation info
@return		: NCSCC_RC_SUCCESS/SA_AIS_OK(1) - For success.
		  NCSCC_RC_FAILURE(0) - For failure.
		  others(SaAisErrorT) - For failure.
******************************************************************************/
SaUint32T plms_HE_adm_shutdown_state_lock_op(PLMS_EVT *evt)
{
	SaUint32T ret_err = NCSCC_RC_FAILURE;
	SaInt8T tmp[SA_MAX_NAME_LENGTH +1];

	plms_get_str_from_dn_name(&(evt->req_evt.admin_op.dn_name),tmp);
	TRACE_ENTER2("Entity: %s. Operation: shutdown to lock.",tmp);

	ret_err = plms_ent_shutdown_to_lock(evt);
	if (NCSCC_RC_SUCCESS != ret_err){
	}
	
	TRACE_LEAVE2("ret_err: %d",ret_err);
	return ret_err;
}
/******************************************************************************
@brief		: Shutdown ==> unlock
@param[in]	: evt - PLMS evt structure holding IMM admin operation info
@return		: NCSCC_RC_SUCCESS/SA_AIS_OK(1) - For success.
		  NCSCC_RC_FAILURE(0) - For failure.
		  others(SaAisErrorT) - For failure.
******************************************************************************/
SaUint32T plms_HE_adm_shutdown_state_unlock_op(  PLMS_EVT *evt)
{
	SaUint32T ret_err = NCSCC_RC_FAILURE;
	SaInt8T tmp[SA_MAX_NAME_LENGTH +1];

	plms_get_str_from_dn_name(&(evt->req_evt.admin_op.dn_name),tmp);
	TRACE_ENTER2("Entity: %s. Operation: shutdown to unlock.",tmp);

	ret_err = plms_ent_shutdown_to_unlock(evt);
	if (NCSCC_RC_SUCCESS != ret_err){
	}

	TRACE_LEAVE2("ret_err: %d",ret_err);
	return ret_err;
}
/******************************************************************************
@brief		: Lckinact ==> unlock
@param[in]	: evt - PLMS evt structure holding IMM admin operation info
@return		: SaAisErrorT.
******************************************************************************/
SaUint32T plms_HE_adm_lckinact_state_unlock_op(  PLMS_EVT *evt)
{
	SaUint32T ret_err = NCSCC_RC_FAILURE;
	PLMS_CB *cb = plms_cb;
	SaInt8T tmp[SA_MAX_NAME_LENGTH +1];

	plms_get_str_from_dn_name(&(evt->req_evt.admin_op.dn_name),tmp);
	TRACE_ENTER2("Entity: %s",tmp);

	LOG_ER("Bad Admin Op, lockinact to unlock. Ent: %s",tmp);
	ret_err = saImmOiAdminOperationResult(cb->oi_hdl,
			evt->req_evt.admin_op.inv_id,SA_AIS_ERR_BAD_OPERATION);  
	if (SA_AIS_OK != ret_err) {
		LOG_ER("Sending response to IMM failed. Ent: %s, \
		Operations: Unlock, ret_err: %d",tmp,ret_err);
	}
	
	TRACE_LEAVE2("ret_err: %d",ret_err);
	return ret_err;
}
/******************************************************************************
@brief		: Lckinact ==> shutdown 
@param[in]	: evt - PLMS evt structure holding IMM admin operation info
@return		: SaAisErrorT.
******************************************************************************/
SaUint32T plms_HE_adm_lckinact_state_shutdown_op(  PLMS_EVT *evt)
{
	SaUint32T ret_err = NCSCC_RC_FAILURE;
	PLMS_CB *cb = plms_cb;
	SaInt8T tmp[SA_MAX_NAME_LENGTH +1];

	plms_get_str_from_dn_name(&(evt->req_evt.admin_op.dn_name),tmp);
	TRACE_ENTER2("Entity: %s",tmp);

	LOG_ER("Bad Admin Op, lockinact to shutdown. Ent: %s",tmp);
	ret_err = saImmOiAdminOperationResult(cb->oi_hdl,
			evt->req_evt.admin_op.inv_id,SA_AIS_ERR_BAD_OPERATION);  
	if (SA_AIS_OK != ret_err) {
		LOG_ER("Sending response to IMM failed. Ent: %s, \
		Operations: Shutdown, ret_err: %d",tmp,ret_err);
	}

	TRACE_LEAVE2("ret_err: %d",ret_err);
	return ret_err;
}
/******************************************************************************
@brief		: Lckinact ==> lock 
@param[in]	: evt - PLMS evt structure holding IMM admin operation info
@return		: SaAisErrorT.
******************************************************************************/
SaUint32T plms_HE_adm_lckinact_state_lock_op(  PLMS_EVT *evt)
{
	SaUint32T ret_err = NCSCC_RC_FAILURE;
	PLMS_CB *cb = plms_cb;
	SaInt8T tmp[SA_MAX_NAME_LENGTH +1];

	plms_get_str_from_dn_name(&(evt->req_evt.admin_op.dn_name),tmp);
	TRACE_ENTER2("Entity: %s",tmp);

	LOG_ER("Bad Admin Op, lockinact to lock. Ent: %s",tmp);
	ret_err = saImmOiAdminOperationResult(cb->oi_hdl,
			evt->req_evt.admin_op.inv_id,SA_AIS_ERR_BAD_OPERATION);  
	if (SA_AIS_OK != ret_err) {
		LOG_ER("Sending response to IMM failed. Ent: %s, \
		Operations: Lock, ret_err: %d",tmp,ret_err);
	}

	TRACE_LEAVE2("ret_err: %d",ret_err);
	return ret_err;
}
/******************************************************************************
@brief		: Lckinact ==> act 
@param[in]	: evt - PLMS evt structure holding IMM admin operation info
@return		: NCSCC_RC_SUCCESS/SA_AIS_OK(1) - For success.
		  NCSCC_RC_FAILURE(0) - For failure.
		  others(SaAisErrorT) - For failure.
******************************************************************************/
SaUint32T plms_HE_adm_lckinact_state_act_op(  PLMS_EVT *evt)
{
	SaUint32T ret_err = NCSCC_RC_FAILURE;
	PLMS_ENTITY *ent;
	PLMS_CB *cb = plms_cb;
	SaInt8T tmp[SA_MAX_NAME_LENGTH +1];

	plms_get_str_from_dn_name(&(evt->req_evt.admin_op.dn_name),tmp);
	TRACE_ENTER2("Entity: %s. Operation: lckinact to act.",tmp);

	ent = (PLMS_ENTITY *)ncs_patricia_tree_get(&(cb->entity_info),
			   (SaUint8T *)&(evt->req_evt.admin_op.dn_name));
	if (NULL == ent) {
		LOG_ER("Entity for the dn_name %s is not found \
		in entity_info tree.",tmp);
		ret_err = saImmOiAdminOperationResult(cb->oi_hdl,
			evt->req_evt.admin_op.inv_id,SA_AIS_ERR_NO_RESOURCES);  
		if (SA_AIS_OK != ret_err){
			LOG_ER("Sending response to IMM failed. Ent: %s, \
			Operation: Activate, ret_err: %d",tmp,ret_err);
		}

		TRACE_LEAVE2("ret_err: %d",ret_err);
		return ret_err;
	}
	/* I should not be in mid of any operation.
	*/
	if (ent->am_i_aff_ent){
		LOG_ER("Admin operation activate can not be performed as\
		the ent %s is in another admin context: %d.",
		tmp,ent->adm_op_in_progress);

		ret_err = saImmOiAdminOperationResult(cb->oi_hdl,
			evt->req_evt.admin_op.inv_id,SA_AIS_ERR_TRY_AGAIN);
		if (SA_AIS_OK != ret_err){
			LOG_ER("Sending response to IMM failed. Ent: %s, \
			Operation: Activate, ret_err: %d",tmp,ret_err);
		}

		TRACE_LEAVE2("ret_err: %d",ret_err);
		return ret_err;    
	}
	/* TODO: Even if the HE is activated, the admin state still
	will be locked and hence the readiness state will still be
	OOS. So the there would not be any entity which would be 
	affected by this operation.*/
	/* Mark the admin state to locked.*/
	plms_admin_state_set(ent,SA_PLM_HE_ADMIN_LOCKED,NULL,
				SA_NTF_MANAGEMENT_OPERATION,
				SA_PLM_NTFID_STATE_CHANGE_ROOT);

	/* Activate the HE. */
	ret_err = plms_he_activate(ent,TRUE,TRUE);
	if (NCSCC_RC_SUCCESS != ret_err){
		LOG_ER("HE %s activation failed. ret_err = %d",ent->dn_name_str,
		ret_err);
		
		ret_err = saImmOiAdminOperationResult(cb->oi_hdl,
			evt->req_evt.admin_op.inv_id,SA_AIS_ERR_DEPLOYMENT);
		if (SA_AIS_OK != ret_err){
			LOG_ER("Sending response to IMM failed. Ent: %s, \
			Operation: Activate, ret_err: %d",tmp,ret_err);
		}
	}else{
		ret_err = saImmOiAdminOperationResult(cb->oi_hdl,
				evt->req_evt.admin_op.inv_id,SA_AIS_OK);
		if (SA_AIS_OK != ret_err){
			LOG_ER("Sending response to IMM failed. Ent: %s, \
			Operation: Activate, ret_err: %d",tmp,ret_err);
		}
	}
	TRACE_LEAVE2("ret_err: %d",ret_err);
	return ret_err;

}
/******************************************************************************
@brief		: Lckinact ==> deacitvate
@param[in]	: evt - PLMS evt structure holding IMM admin operation info
@return		: SaAisErrorT.
******************************************************************************/
SaUint32T plms_HE_adm_lckinact_state_deact_op(  PLMS_EVT *evt)
{
	SaUint32T ret_err = NCSCC_RC_FAILURE;
	PLMS_CB *cb = plms_cb;
	SaInt8T tmp[SA_MAX_NAME_LENGTH +1];

	plms_get_str_from_dn_name(&(evt->req_evt.admin_op.dn_name),tmp);
	TRACE_ENTER2("Entity: %s",tmp);

	LOG_ER("Bad Admin Op, lockinact to deact. Ent: %s",tmp);
	ret_err = saImmOiAdminOperationResult(cb->oi_hdl,
			evt->req_evt.admin_op.inv_id,SA_AIS_ERR_NO_OP);  

	if (SA_AIS_OK != ret_err) {
		LOG_ER("Sending response to IMM failed. Ent: %s, \
		Operations: Deactivate, ret_err: %d",tmp,ret_err);
	}

	TRACE_LEAVE2("ret_err: %d",ret_err);
	return ret_err;
}
/******************************************************************************
@brief		: Perform HE admin reset. Remember this does not cause HE
		power cycle.
		Three set of completed callbacks are called for this operation.
		1. Completed callback when the targeted HE and affected
		entities are moved to inactive/uninstantiated and OOS.
		2. When all the affected HEs and targeted HE is moved to
		active and InSvc.
		3. Each affected EEs are moved to instantiated and
		InSvc.
@param[in]	: evt - PLMS evt structure holding IMM admin operation info
@return		: NCSCC_RC_SUCCESS/SA_AIS_OK(1) - For success.
		  NCSCC_RC_FAILURE(0) - For failure.
		  others(SaAisErrorT) - For failure.
******************************************************************************/
SaUint32T plms_HE_adm_reset_op(PLMS_EVT *evt)
{
	PLMS_ENTITY *ent,*chld_head;
	PLMS_CB *cb = plms_cb;
	SaUint32T ret_err = NCSCC_RC_FAILURE;
	PLMS_GROUP_ENTITY *aff_ent_list=NULL,*act_aff_ent_list=NULL;
	PLMS_GROUP_ENTITY *head,*temp;
	PLMS_GROUP_ENTITY *aff_chld_ee_list=NULL,*dep_chld_ee_list=NULL;
	PLMS_GROUP_ENTITY *aff_he_list=NULL;
	PLMS_TRACK_INFO *trk_info;
	SaUint32T count = 0;
	PLMS_GROUP_ENTITY *log_head;
	SaInt8T tmp[SA_MAX_NAME_LENGTH +1];
	SaUint8T is_flag_aff = 0;

	plms_get_str_from_dn_name(&(evt->req_evt.admin_op.dn_name),tmp);
	TRACE_ENTER2("Entity: %s. Operation: adm_reset.",tmp);

	ent = (PLMS_ENTITY *)ncs_patricia_tree_get(&(cb->entity_info),
			(SaUint8T *)&(evt->req_evt.admin_op.dn_name));
	if (NULL == ent) {
		LOG_ER("Admin_reset can not be performed as the ent is\
		not found in entity_info tree for dn_name %s",tmp);
		
		ret_err = saImmOiAdminOperationResult(cb->oi_hdl,
			evt->req_evt.admin_op.inv_id,SA_AIS_ERR_NO_RESOURCES); 

		if (SA_AIS_OK != ret_err){
			LOG_ER("Sending response to IMM failed. Ent: %s, \
			Operation: Activate, ret_err: %d",tmp,ret_err);
		}
		TRACE_LEAVE2("ret_err: %d",ret_err);
		return ret_err;
	}
	
	/* If the HE presence state is active then return.*/
	if ( SA_PLM_HE_PRESENCE_ACTIVE != 
		ent->entity.he_entity.saPlmHEPresenceState){
		
		LOG_ER("HE %s is not in active presence state, admin_reset \
		can not be performed.",tmp);

		ret_err = saImmOiAdminOperationResult(cb->oi_hdl,
			evt->req_evt.admin_op.inv_id,SA_AIS_ERR_BAD_OPERATION);
		if (SA_AIS_OK != ret_err){
			LOG_ER("Sending response to IMM failed. Ent: %s, \
			Operation: Activate, ret_err: %d",tmp,ret_err);
		}
		TRACE_LEAVE2("ret_err: %d",ret_err);
		return ret_err;    
	}
	
	/* Get all the affected entities.*/ 
	plms_affected_ent_list_get(ent,&aff_ent_list,0/* OOS */);
	
	if (plms_anc_chld_dep_adm_flag_is_set(ent,aff_ent_list)) {
		
		LOG_ER("Admin reset can not be performed as one of the  \
		affected entity is in other admin context. Ent: %s",tmp);
		
		ret_err = saImmOiAdminOperationResult(cb->oi_hdl,
			evt->req_evt.admin_op.inv_id,SA_AIS_ERR_TRY_AGAIN);
		if (SA_AIS_OK != ret_err){
			LOG_ER("Sending response to IMM failed. Ent: %s, \
			Operation: HE reset, ret_err: %d",tmp,ret_err);
		}
		plms_ent_list_free(aff_ent_list);
		aff_ent_list = NULL;

		TRACE_LEAVE2("ret_err: %d",ret_err);
		return ret_err;    
	}
	
	/* Reset the target HE.*/
	ret_err = plms_he_reset(ent,1/*adm_op*/,1/*mngt_cbk*/,SAHPI_COLD_RESET);
	if ( NCSCC_RC_SUCCESS != ret_err ){
		LOG_ER("HE reset failed. Admin-reser operation \
		aborted. Ent: %s",tmp);
		ret_err = saImmOiAdminOperationResult(cb->oi_hdl,
			evt->req_evt.admin_op.inv_id,SA_AIS_ERR_TRY_AGAIN);
		if (SA_AIS_OK != ret_err){
			LOG_ER("Sending response to IMM failed. Ent: %s, \
			Operation: adm_reset, ret_err: %d",tmp,ret_err);
		}
		plms_ent_list_free(aff_ent_list);
		aff_ent_list = NULL;
		
		TRACE_LEAVE2("ret_err: %d",ret_err);
		return ret_err;    
	}
	if(plms_is_rdness_state_set(ent,SA_PLM_READINESS_OUT_OF_SERVICE)){
		TRACE("HE reset: Ent in OOS state. No cbk called. Ent: %s",tmp);
		ret_err = saImmOiAdminOperationResult(cb->oi_hdl,
			evt->req_evt.admin_op.inv_id,SA_AIS_OK);
		if (SA_AIS_OK != ret_err){
			LOG_ER("Sending response to IMM failed. Ent: %s, \
			Operation: adm_reset, ret_err: %d",tmp,ret_err);
		}
		plms_ent_list_free(aff_ent_list);
		aff_ent_list = NULL;
		
		TRACE_LEAVE2("ret_err: %d",ret_err);
		return ret_err;    
	}
	
	/* Admin operation started. */ 
	ent->adm_op_in_progress = SA_PLM_CAUSE_HE_RESET;
	ent->am_i_aff_ent = TRUE;
	plms_aff_ent_flag_mark_unmark(aff_ent_list,TRUE);
	
	plms_presence_state_set(ent,SA_PLM_HE_PRESENCE_INACTIVE,NULL,
				SA_NTF_MANAGEMENT_OPERATION,
				SA_PLM_NTFID_STATE_CHANGE_ROOT);

	plms_readiness_state_set(ent,SA_PLM_READINESS_OUT_OF_SERVICE,
				NULL,SA_NTF_MANAGEMENT_OPERATION,
				SA_PLM_NTFID_STATE_CHANGE_ROOT);
	
	/* Get the trk_info ready.*/
	trk_info = (PLMS_TRACK_INFO *)calloc(1,sizeof(PLMS_TRACK_INFO));
	trk_info->root_entity = ent;
	
	/* Find all the affected HEs.*/
	plms_aff_he_find(aff_ent_list,&aff_he_list);
	TRACE("Affected HEs for ent %s ", ent->dn_name_str);
	log_head = aff_he_list;
	while(log_head){
		TRACE("%s,",log_head->plm_entity->dn_name_str);
		log_head = log_head->next;
	}
			
	plms_ee_chld_of_aff_he_find(ent,aff_he_list,
					aff_ent_list,&aff_chld_ee_list);	
	TRACE("Chld of affected HEs. Root Ent: %s ", ent->dn_name_str);
	log_head = aff_chld_ee_list;
	while(log_head){
		TRACE("%s,",log_head->plm_entity->dn_name_str);
		log_head = log_head->next;
	}
	
	plms_ee_not_chld_of_aff_he_find(aff_ent_list,
					aff_chld_ee_list,&dep_chld_ee_list);
	TRACE("EEs not chld of aff HEs. Root ent: %s ", ent->dn_name_str);
	log_head = dep_chld_ee_list;
	while(log_head){
		TRACE("%s,",log_head->plm_entity->dn_name_str);
		log_head = log_head->next;
	}
	
	/* For all aff HEs.*/
	head = aff_he_list;
	while (head){
		ret_err = plms_he_reset(head->plm_entity,0/*adm_op*/,
						1/*mngt_cbk*/,SAHPI_COLD_RESET);
#if 0			
		plms_presence_state_set(head->plm_entity,
					SA_PLM_HE_PRESENCE_INACTIVE,ent,
					SA_NTF_MANAGEMENT_OPERATION,
					SA_PLM_NTFID_STATE_CHANGE_DEP);
#endif
		plms_readiness_flag_mark_unmark(head->plm_entity,
				SA_PLM_RF_DEPENDENCY,1/*mark*/,ent,
				SA_NTF_MANAGEMENT_OPERATION,
				SA_PLM_NTFID_STATE_CHANGE_DEP);
		plms_readiness_state_set(head->plm_entity,
					SA_PLM_READINESS_OUT_OF_SERVICE,ent,
					SA_NTF_MANAGEMENT_OPERATION,
					SA_PLM_NTFID_STATE_CHANGE_DEP);

		if (NCSCC_RC_SUCCESS != ret_err){
			/* Store the pointer. This node has to be
			removed from the aff_he_list.*/
			temp = head;
			
			LOG_ER("Reset failed for ent: %s",
			head->plm_entity->dn_name_str);

			/* HE reset fails, so got to reboot the direct child EEs
			runnig on the HE. */
			chld_head = head->plm_entity->leftmost_child;
			while (chld_head){
				/* TODO: Is this check required?*/
				if(plms_is_ent_in_ent_list(aff_chld_ee_list,
				chld_head)){

					TRACE("Parent HE reset failed, hence \
					reboot the child EE. Parent HE: %s, \
					Chld EE: %s",
					head->plm_entity->dn_name_str,
					chld_head->dn_name_str);

					ret_err = plms_ee_reboot(chld_head,
								FALSE,TRUE);
					if (NCSCC_RC_SUCCESS != ret_err){	
						LOG_ER("EE reset failed.EE: %s",
						chld_head->dn_name_str);
						
						/* Oh my God,this is irritating.
						How many failures... Just saving
						my grace. Dont blame me.*/
						plms_readiness_flag_mark_unmark(
						chld_head,
						SA_PLM_RF_DEPENDENCY,1/*mark*/,
						ent,SA_NTF_MANAGEMENT_OPERATION
						,SA_PLM_NTFID_STATE_CHANGE_DEP);
						
						plms_readiness_state_set(
						chld_head,
						SA_PLM_READINESS_OUT_OF_SERVICE,
						ent,SA_NTF_MANAGEMENT_OPERATION,
						SA_PLM_NTFID_STATE_CHANGE_DEP);
						
						plms_ent_from_ent_list_rem(
						chld_head,&aff_chld_ee_list);
					}

				}	
			chld_head = chld_head->right_sibling;	
			}
		
		head = head->next;
		plms_ent_from_ent_list_rem(temp->plm_entity,&aff_he_list);
		
		}else{
			plms_presence_state_set(head->plm_entity,
					SA_PLM_HE_PRESENCE_INACTIVE,ent,
					SA_NTF_MANAGEMENT_OPERATION,
					SA_PLM_NTFID_STATE_CHANGE_DEP);
			head = head->next;
		}
	}
	
	/* For all affected children EEs(successfully restarted).*/
	head = aff_chld_ee_list;
	while (head){
		plms_presence_state_set(head->plm_entity,
					SA_PLM_EE_PRESENCE_UNINSTANTIATED,ent,
					SA_NTF_MANAGEMENT_OPERATION,
					SA_PLM_NTFID_STATE_CHANGE_DEP);
		plms_readiness_flag_mark_unmark(head->plm_entity,
				SA_PLM_RF_DEPENDENCY,1/*mark*/,ent,
				SA_NTF_MANAGEMENT_OPERATION,
				SA_PLM_NTFID_STATE_CHANGE_DEP);

		plms_readiness_state_set(head->plm_entity,
					SA_PLM_READINESS_OUT_OF_SERVICE,ent,
					SA_NTF_MANAGEMENT_OPERATION,
					SA_PLM_NTFID_STATE_CHANGE_DEP);

		head->plm_entity->trk_info = trk_info;
		count++;

		head = head->next;
	}

	/* For all the dep EEs. */
	head = dep_chld_ee_list;
	while (head){
		ret_err = plms_ee_reboot(head->plm_entity,FALSE,TRUE);
		plms_readiness_state_set(head->plm_entity,
					SA_PLM_READINESS_OUT_OF_SERVICE,ent,
					SA_NTF_MANAGEMENT_OPERATION,
					SA_PLM_NTFID_STATE_CHANGE_DEP);
		plms_readiness_flag_mark_unmark(head->plm_entity,
					SA_PLM_RF_DEPENDENCY,1/* mark */,ent,
					SA_NTF_MANAGEMENT_OPERATION,
					SA_PLM_NTFID_STATE_CHANGE_DEP);	
		
		if (ret_err){
			plms_presence_state_set(head->plm_entity,
					SA_PLM_EE_PRESENCE_UNINSTANTIATED,ent,
					SA_NTF_MANAGEMENT_OPERATION,
					SA_PLM_NTFID_STATE_CHANGE_DEP);
			
			head->plm_entity->trk_info = trk_info;
			count++;
		}else{
			LOG_ER("EE Reset failed. EE: %s",
			head->plm_entity->dn_name_str);
			/* TODO: Should I reset the parent HE. */
		}
		head = head->next;	
	}

	/* Overwrite the expected readiness status with the current.*/
	plms_aff_ent_exp_rdness_state_ow(aff_ent_list);
	
	/*Fill the expected readiness state of the root ent.*/
	plms_ent_exp_rdness_state_ow(ent);

	trk_info->aff_ent_list = aff_ent_list;
	
	/* Add the groups, root entity(ent) belong to.*/
	plms_ent_grp_list_add(ent,&(trk_info->group_info_list));
	
	/* Find out all the groups, all affected entities belong to and add 
	the groups to trk_info->group_info_list. 
	TODO: We have to free trk_info->group_info_list. */
	plms_ent_list_grp_list_add(aff_ent_list,
				&(trk_info->group_info_list));	
	
	trk_info->imm_adm_opr_id = evt->req_evt.admin_op.operation_id; 
	trk_info->inv_id = evt->req_evt.admin_op.inv_id;
	trk_info->change_step = SA_PLM_CHANGE_COMPLETED;
	trk_info->track_cause = SA_PLM_CAUSE_HE_RESET;
	trk_info->root_correlation_id = SA_NTF_IDENTIFIER_UNUSED;
	trk_info->grp_op = SA_PLM_GROUP_MEMBER_READINESS_CHANGE; 
	trk_info->root_entity = ent;
	/* 1. Call the 1st callback for reset.*/
	plms_cbk_call(trk_info,1);
	
	plms_aff_ent_exp_rdness_status_clear(aff_ent_list);
	plms_ent_exp_rdness_status_clear(ent);
	
	plms_ent_grp_list_free(trk_info->group_info_list);
	
	/**********************************************************************/
	
	/* TODO: As we have no trigger for knowing that
	the HEs are activated, we assume that if the hpi-API
	returns success then the HE is in activated state.*/		
	/* Mark the presence state of the target HE to ACTIVE*/
	plms_presence_state_set(ent,SA_PLM_HE_PRESENCE_ACTIVE,NULL,
				SA_NTF_MANAGEMENT_OPERATION,
				SA_PLM_NTFID_STATE_CHANGE_ROOT);
	
	/* Mark the presence state of all aff HEs to ACTIVE*/
	head = aff_he_list;
	while (head){
		
		plms_presence_state_set(head->plm_entity,
				SA_PLM_HE_PRESENCE_ACTIVE,ent,
				SA_NTF_MANAGEMENT_OPERATION,
				SA_PLM_NTFID_STATE_CHANGE_DEP);
		head = head->next;
	}

	/* If there is no reason, take the target HE to insvc.*/
	if ( NCSCC_RC_SUCCESS == plms_move_ent_to_insvc(ent,&is_flag_aff)){
		/* Move the children to insvc and clear the dependency flag.*/
		plms_move_chld_ent_to_insvc(ent->leftmost_child,
		&act_aff_ent_list,0,0);
		
		/* Move the dependent entities to InSvc and clear the
		dependency flag.*/
		plms_move_dep_ent_to_insvc(ent->rev_dep_list,
		&act_aff_ent_list,0);
				
		/* Overwrite the expected readiness status with the current.*/
		plms_aff_ent_exp_rdness_state_ow(act_aff_ent_list);
	
		/*Fill the expected readiness state of the root ent.*/
		plms_ent_exp_rdness_state_ow(ent);

		trk_info->aff_ent_list = act_aff_ent_list;
		trk_info->group_info_list = NULL;
		/* Add the groups, root entity(ent) belong to.*/
		plms_ent_grp_list_add(ent,&(trk_info->group_info_list));
	
		/* Find out all the groups, all affected entities belong to*/ 
		plms_ent_list_grp_list_add(act_aff_ent_list,
				&(trk_info->group_info_list));	
	
		trk_info->change_step = SA_PLM_CHANGE_COMPLETED;
		trk_info->track_cause = SA_PLM_CAUSE_HE_ACTIVATED;
		trk_info->root_correlation_id = SA_NTF_IDENTIFIER_UNUSED;
		trk_info->grp_op = SA_PLM_GROUP_MEMBER_READINESS_CHANGE; 
		/* 2. Call the 2nd callback for reset.*/
		plms_cbk_call(trk_info,1);
	
		plms_aff_ent_exp_rdness_status_clear(act_aff_ent_list);
		plms_ent_exp_rdness_status_clear(ent);
		plms_ent_grp_list_free(trk_info->group_info_list);
	}else if (is_flag_aff){
		/*Fill the expected readiness state of the root ent.*/
		plms_ent_exp_rdness_state_ow(ent);

		trk_info->aff_ent_list = NULL; 
		trk_info->group_info_list = NULL;
		/* Add the groups, root entity(ent) belong to.*/
		plms_ent_grp_list_add(ent,&(trk_info->group_info_list));
	
		trk_info->change_step = SA_PLM_CHANGE_COMPLETED;
		trk_info->track_cause = SA_PLM_CAUSE_HE_ACTIVATED;
		trk_info->root_correlation_id = SA_NTF_IDENTIFIER_UNUSED;
		trk_info->grp_op = SA_PLM_GROUP_MEMBER_READINESS_CHANGE; 
		/* 2. Call the 2nd callback for reset.*/
		plms_cbk_call(trk_info,1);
	
		plms_ent_exp_rdness_status_clear(ent);
		plms_ent_grp_list_free(trk_info->group_info_list);
		
	}

	/**********************************************************************/	
	/* When EACH EE is instantiated and moved to insvc, do the following.
	1. Find out the entities whose readiness status is changed because this
	ee is moving to insvc.
	2. For all those ee, call completed callback with track cause as 
	SA_PLM_CAUSE_HE_ACTIVATED.
	3. Free trk_info->group_info_list.
	4. Decrement the trk_info->count.
	When all the EEs are instantiated(trk_info->count becomes 0), do the 
	following.
	1. Return to IMM.
	2. Unmark the admin context.
	3. Free aff_ent_list.
	4. Free trk_info->group_info_list.
	5. Free trk_info.

	Follow plms_plmc_instantiated_process().
	*/
	
	trk_info->imm_adm_opr_id = evt->req_evt.admin_op.operation_id; 
	trk_info->inv_id = evt->req_evt.admin_op.inv_id;
	trk_info->change_step = SA_PLM_CHANGE_COMPLETED;
	trk_info->track_cause = SA_PLM_CAUSE_HE_ACTIVATED;
	trk_info->root_correlation_id = SA_NTF_IDENTIFIER_UNUSED; 
	trk_info->root_entity = ent;
	trk_info->track_count = count;
	trk_info->aff_ent_list = aff_ent_list;
	trk_info->group_info_list = NULL;

	plms_ent_list_free(aff_he_list);
	aff_he_list = NULL;
	plms_ent_list_free(aff_chld_ee_list);
	aff_chld_ee_list = NULL;
	plms_ent_list_free(dep_chld_ee_list);
	dep_chld_ee_list = NULL;
	plms_ent_list_free(act_aff_ent_list);
	act_aff_ent_list = NULL;

	TRACE_LEAVE2("ret_err: %d",ret_err);
	return ret_err;
}

/******************************************************************************
@brief		: Unlocked ==> unlock 
@param[in]	: evt - PLMS evt structure holding IMM admin operation info      
@return		: SaAisErrorT. 
******************************************************************************/
SaUint32T plms_EE_adm_unlock_state_unlock_op(  PLMS_EVT *evt)
{
	SaUint32T ret_err = NCSCC_RC_FAILURE;
	PLMS_CB *cb = plms_cb;
	SaInt8T tmp[SA_MAX_NAME_LENGTH +1];

	plms_get_str_from_dn_name(&(evt->req_evt.admin_op.dn_name),tmp);
	TRACE_ENTER2("Entity: %s",tmp);

	LOG_ER("Bad Admin Op, unlock to unlock. Ent: %s",tmp);
	ret_err = saImmOiAdminOperationResult(cb->oi_hdl,
				evt->req_evt.admin_op.inv_id,SA_AIS_ERR_NO_OP);  
	if (SA_AIS_OK != ret_err) {
		LOG_ER("Sending response to IMM failed. Ent: %s, \
		Operations: Unlock, ret_err: %d",tmp,ret_err);
	}

	TRACE_LEAVE2("ret_err: %d",ret_err);
	return ret_err;
}
/******************************************************************************
@brief		: Unlocked ==> Lock
@param[in]	: evt - PLMS evt structure holding IMM admin operation info      
@return		: NCSCC_RC_SUCCESS/SA_AIS_OK(1) - For success.
		  NCSCC_RC_FAILURE(0) - For failure.
		  others(SaAisErrorT) - For failure.
******************************************************************************/
SaUint32T plms_EE_adm_unlock_state_lock_op(  PLMS_EVT *evt)
{
	PLMS_ENTITY *ent;
	SaUint32T ret_err = NCSCC_RC_FAILURE;
	PLMS_GROUP_ENTITY *aff_ent_list=NULL;
	PLMS_CB *cb = plms_cb;
	PLMS_TRACK_INFO *trk_info;
	PLMS_ENTITY_GROUP_INFO_LIST *grp_list_head;
	SaInt8T tmp[SA_MAX_NAME_LENGTH +1];

	plms_get_str_from_dn_name(&(evt->req_evt.admin_op.dn_name),tmp);
	TRACE_ENTER2("Entity: %s. Unlock to Lock, Variant: %s",tmp,
	evt->req_evt.admin_op.option);

	ent = (PLMS_ENTITY *)ncs_patricia_tree_get(&(cb->entity_info),
			   (SaUint8T *)&(evt->req_evt.admin_op.dn_name));
	if (NULL == ent) {
		LOG_ER("Lock operation failed, as ent in entity_info tree\
		is not found for dn_name: %s",tmp);
		ret_err = saImmOiAdminOperationResult(cb->oi_hdl,
			evt->req_evt.admin_op.inv_id,SA_AIS_ERR_NO_RESOURCES);  
		if (SA_AIS_OK != ret_err){
			LOG_ER("Sending response to IMM failed. Ent: %s, \
			Operation: Lock, ret_err: %d",tmp,ret_err);
		}
		TRACE_LEAVE2("ret_err: %d",ret_err);
		return ret_err;
	}
	
	/* If not forced lock, check the feasibility. */
	if ( (0 == (strcmp(evt->req_evt.admin_op.option,
	SA_PLM_ADMIN_LOCK_OPTION_TRYLOCK))) || 
	('\0' == evt->req_evt.admin_op.option[0])){
		/* If I am OOS, then I am the only affected entity.*/
		if ((SA_PLM_READINESS_OUT_OF_SERVICE ==
		ent->entity.ee_entity.saPlmEEReadinessState)){
			
			TRACE("Ent: %s is already in OOS.",ent->dn_name_str);
			if (ent->am_i_aff_ent){

				LOG_ER("Lock operation failed as the entity %s\
				is already in other operaional context.",tmp);
				
				ret_err = saImmOiAdminOperationResult(
				cb->oi_hdl, evt->req_evt.admin_op.inv_id,
				SA_AIS_ERR_TRY_AGAIN);
				
				if (SA_AIS_OK != ret_err){
					LOG_ER("Sending response to IMM failed.\
					Ent: %s, Operation: Lock, \
					ret_err: %d",tmp,ret_err);
				}
				TRACE_LEAVE2("ret_err: %d",ret_err);
				return ret_err;
			}else{
				plms_admin_state_set(ent,SA_PLM_EE_ADMIN_LOCKED
				,NULL,SA_NTF_MANAGEMENT_OPERATION,
				SA_PLM_NTFID_STATE_CHANGE_ROOT);
		
				/* Respond to IMM.*/
				ret_err = saImmOiAdminOperationResult(
				cb->oi_hdl, evt->req_evt.admin_op.inv_id,
				SA_AIS_OK);
				
				if (SA_AIS_OK != ret_err){
					LOG_ER("Sending response to IMM failed.\
					Ent: %s, Operation: Lock, ret_err: %d",
					tmp,ret_err);
				}

				TRACE_LEAVE2("ret_err: %d",ret_err);
				return ret_err;
			}
		/* If am not in OOS.*/	
		}else{
			TRACE("Ent: %s is not in OOS.",ent->dn_name_str);
			/* Get all the affected entities.*/ 
			plms_affected_ent_list_get(ent, &aff_ent_list,0);
			
			if (plms_anc_chld_dep_adm_flag_is_set(
							ent,aff_ent_list)) {
				
				LOG_ER("Admin operation can not be performed on\
				ent %s as one of the affected entity is in \
				another operation context.",tmp);

				plms_ent_list_free(aff_ent_list);
				aff_ent_list = NULL;
				
				ret_err = saImmOiAdminOperationResult(
				cb->oi_hdl, evt->req_evt.admin_op.inv_id,
				SA_AIS_ERR_TRY_AGAIN);
				
				if (SA_AIS_OK != ret_err){
					LOG_ER("Sending response to IMM failed.\
					Ent: %s, Operation: Activate, \
					ret_err: %d",tmp,ret_err);
				}
				
				TRACE_LEAVE2("ret_err: %d",ret_err);
				return ret_err;    
			}
		}
	/* If forced lock.*/	
	}else if (0 == (strcmp(evt->req_evt.admin_op.option,
	SA_PLM_ADMIN_LOCK_OPTION_FORCED))) {  
		/* If I am aff, then I should be in admin context,
		otherwise reject the forced lock.*/
		if (ent->am_i_aff_ent && !(ent->adm_op_in_progress)){
			
			LOG_ER("Forced lock can not be performed on \
			this %s ent. am_i_aff_ent = 1, \
			adm_op_in_progress = 0",ent->dn_name_str);

			ret_err = saImmOiAdminOperationResult(
			cb->oi_hdl, evt->req_evt.admin_op.inv_id,
			SA_AIS_ERR_TRY_AGAIN);

			TRACE_LEAVE2("ret_err: %d",ret_err);
			return ret_err;    
		}

		if (SA_PLM_READINESS_OUT_OF_SERVICE !=
				ent->entity.ee_entity.saPlmEEReadinessState){
			TRACE("Ent: %s is not in OOS.",ent->dn_name_str);
			/* Get all the affected entities.*/ 
			plms_affected_ent_list_get(ent, &aff_ent_list,0);
			
		}else{
			TRACE("Ent: %s is already in OOS.",ent->dn_name_str);
			trk_info = ent->trk_info;
			if (ent->adm_op_in_progress && (NULL != trk_info)){
				grp_list_head = trk_info->group_info_list;
				while (grp_list_head){
					/* Remove the inv_to_cbk nodes from the 
					grp->innvocation list, if pointing to 
					this trk_info.*/
					plms_inv_to_cbk_in_grp_trk_rmv(
					grp_list_head->ent_grp_inf,trk_info);
					grp_list_head = grp_list_head->next;
				}

				plms_aff_ent_exp_rdness_status_clear(
				trk_info->aff_ent_list);
				plms_ent_exp_rdness_status_clear(
				trk_info->root_entity);
				
				ent->am_i_aff_ent = FALSE;
				ent->adm_op_in_progress = FALSE;
				plms_aff_ent_mark_unmark(trk_info->aff_ent_list,
									FALSE);
				plms_trk_info_free(trk_info);
			}

			/* Clean up done. Process lock operation.*/
			plms_admin_state_set(ent,SA_PLM_EE_ADMIN_LOCKED,NULL,
						SA_NTF_MANAGEMENT_OPERATION,
						SA_PLM_NTFID_STATE_CHANGE_ROOT);
			
			/* Respond to IMM.*/
			ret_err = saImmOiAdminOperationResult(cb->oi_hdl,
					evt->req_evt.admin_op.inv_id,SA_AIS_OK);
			if (SA_AIS_OK != ret_err){
				LOG_ER("Sending response to IMM failed. Ent:%s,\
				Operation: Lock, ret_err: %d",tmp,ret_err);
			}
			TRACE_LEAVE2("ret_err: %d",ret_err);
			return ret_err;
		}
	}else{/* Invalid lock option.*/
		LOG_ER("Invalid Lock option: %s",evt->req_evt.admin_op.option);	
		ret_err = saImmOiAdminOperationResult(cb->oi_hdl,evt->req_evt.admin_op.inv_id,
		SA_AIS_ERR_INVALID_PARAM);
		TRACE_LEAVE2("ret_err: %d",ret_err);
		return ret_err;
	}
	
	if(0 == (strcmp(evt->req_evt.admin_op.option,
		SA_PLM_ADMIN_LOCK_OPTION_TRYLOCK))) {
		/* Admin operation started. */
		ent->adm_op_in_progress = SA_PLM_CAUSE_LOCK;
		ent->am_i_aff_ent = TRUE;
		plms_aff_ent_flag_mark_unmark(aff_ent_list,TRUE);
		
		ret_err = plms_try_lock(ent,evt->req_evt.admin_op,
							aff_ent_list);
	
	} else if(0 == (strcmp(evt->req_evt.admin_op.option,
		SA_PLM_ADMIN_LOCK_OPTION_FORCED))){
		ret_err = plms_forced_lock(ent,evt->req_evt.admin_op,
							aff_ent_list);
	}else {
		/* Admin operation started. */
		ent->adm_op_in_progress = SA_PLM_CAUSE_LOCK;
		ent->am_i_aff_ent = TRUE;
		plms_aff_ent_flag_mark_unmark(aff_ent_list,TRUE);

		ret_err = plms_default_lock(ent,evt->req_evt.admin_op,
							aff_ent_list);
	}
	TRACE_LEAVE2("ret_err: %d",ret_err);
	return ret_err;
	
}

/******************************************************************************
@brief		: Unlocked ==> Shutdown 
@param[in]	: evt - PLMS evt structure holding IMM admin operation info      
@return		: NCSCC_RC_SUCCESS/SA_AIS_OK(1) - For success.
		  NCSCC_RC_FAILURE(0) - For failure.
		  others(SaAisErrorT) - For failure.
******************************************************************************/
SaUint32T plms_EE_adm_unlock_state_shutdown_op( PLMS_EVT *evt)
{
	SaUint32T ret_err = NCSCC_RC_FAILURE;
	SaInt8T tmp[SA_MAX_NAME_LENGTH +1];

	plms_get_str_from_dn_name(&(evt->req_evt.admin_op.dn_name),tmp);
	TRACE_ENTER2("Entity: %s. Unlock to shutdown",tmp);
	
	ret_err = plms_ent_unlock_to_shutdown(evt);
	if (NCSCC_RC_SUCCESS != ret_err){
	}

	TRACE_LEAVE2("ret_err: %d",ret_err);
	return ret_err;
}
/******************************************************************************
@brief		: Unlocked ==> unlckinst 
@param[in]	: evt - PLMS evt structure holding IMM admin operation info      
@return		: SaAisErrorT. 
******************************************************************************/
SaUint32T plms_EE_adm_unlock_state_unlckinst_op(  PLMS_EVT *evt)
{
	SaUint32T ret_err = NCSCC_RC_FAILURE;
	PLMS_CB *cb = plms_cb;
	SaInt8T tmp[SA_MAX_NAME_LENGTH +1];

	plms_get_str_from_dn_name(&(evt->req_evt.admin_op.dn_name),tmp);
	TRACE_ENTER2("Entity: %s",tmp);

	LOG_ER("Bad Admin Op, unlock to unlckinst. Ent: %s",tmp);
	ret_err = saImmOiAdminOperationResult(cb->oi_hdl,
			evt->req_evt.admin_op.inv_id,SA_AIS_ERR_BAD_OPERATION);  
	if (SA_AIS_OK != ret_err) {
		LOG_ER("Sending response to IMM failed. Ent: %s, \
		Operations: Unlckinst, ret_err: %d",tmp,ret_err);
	}

	TRACE_LEAVE2("ret_err: %d",ret_err);
	return ret_err;
}
/******************************************************************************
@brief		: Unlocked ==> lckinst 
@param[in]	: evt - PLMS evt structure holding IMM admin operation info      
@return		: SaAisErrorT. 
******************************************************************************/
SaUint32T plms_EE_adm_unlock_state_lckinst_op(  PLMS_EVT *evt)
{
	SaUint32T ret_err = NCSCC_RC_FAILURE;
	PLMS_CB *cb = plms_cb;
	SaInt8T tmp[SA_MAX_NAME_LENGTH +1];

	plms_get_str_from_dn_name(&(evt->req_evt.admin_op.dn_name),tmp);
	TRACE_ENTER2("Entity: %s",tmp);

	LOG_ER("Bad Admin Op, unlock to lckinst. Ent: %s",tmp);
	ret_err = saImmOiAdminOperationResult(cb->oi_hdl,
			evt->req_evt.admin_op.inv_id,SA_AIS_ERR_BAD_OPERATION);  
	if (SA_AIS_OK != ret_err) {
		LOG_ER("Sending response to IMM failed. Ent: %s, \
		Operations: Lckinst, ret_err: %d",tmp,ret_err);
	}

	TRACE_LEAVE2("ret_err: %d",ret_err);
	return ret_err;
}

/******************************************************************************
@brief		: Locked ==> unlock
@param[in]	: evt - PLMS evt structure holding IMM admin operation info      
@return		: NCSCC_RC_SUCCESS/SA_AIS_OK(1) - For success.
		  NCSCC_RC_FAILURE(0) - For failure.
		  others(SaAisErrorT) - For failure.
******************************************************************************/
SaUint32T plms_EE_adm_locked_state_unlock_op( PLMS_EVT *evt)
{
	SaUint32T ret_err = NCSCC_RC_FAILURE;
	SaInt8T tmp[SA_MAX_NAME_LENGTH +1];

	plms_get_str_from_dn_name(&(evt->req_evt.admin_op.dn_name),tmp);
	TRACE_ENTER2("Entity: %s. Lock to unlock.",tmp);

	ret_err = plms_ent_locked_to_unlock(evt);
	if (NCSCC_RC_SUCCESS != ret_err){
	}

	TRACE_LEAVE2("ret_err: %d",ret_err);
	return ret_err;
}
/******************************************************************************
@brief		: Locked ==> lock
@param[in]	: evt - PLMS evt structure holding IMM admin operation info      
@return		: NCSCC_RC_SUCCESS/SA_AIS_OK(1) - For success.
		  NCSCC_RC_FAILURE(0) - For failure.
		  others(SaAisErrorT) - For failure.
******************************************************************************/
SaUint32T plms_EE_adm_locked_state_lock_op(  PLMS_EVT *evt)
{
	SaUint32T ret_err = NCSCC_RC_FAILURE;
	SaInt8T tmp[SA_MAX_NAME_LENGTH +1];
	PLMS_CB *cb = plms_cb;

	plms_get_str_from_dn_name(&(evt->req_evt.admin_op.dn_name),tmp);
	TRACE_ENTER2("NO Operation performed. Entity: %s. Operation: lock to lock, Variant: %s",tmp,
	evt->req_evt.admin_op.option);

	ret_err = saImmOiAdminOperationResult(cb->oi_hdl,
			evt->req_evt.admin_op.inv_id,SA_AIS_ERR_NO_OP);  
	if (SA_AIS_OK != ret_err) {
		LOG_ER("Sending response to IMM failed. Ent: %s, \
		Operation: lock to lock, ret_err: %d",tmp,ret_err);
	}

	TRACE_LEAVE2("ret_err: %d",ret_err);
	return ret_err;
	
}
/******************************************************************************
@brief		: Locked ==> unlckinst 
@param[in]	: evt - PLMS evt structure holding IMM admin operation info      
@return		: SaAisErrorT. 
******************************************************************************/
SaUint32T plms_EE_adm_locked_state_unlckinst_op(  PLMS_EVT *evt)
{
	SaUint32T ret_err = NCSCC_RC_FAILURE;
	PLMS_CB *cb = plms_cb;
	SaInt8T tmp[SA_MAX_NAME_LENGTH +1];

	plms_get_str_from_dn_name(&(evt->req_evt.admin_op.dn_name),tmp);
	TRACE_ENTER2("Entity: %s",tmp);

	LOG_ER("Bad Admin Op, lock to unlckinst. Ent: %s",tmp);
	ret_err = saImmOiAdminOperationResult(cb->oi_hdl,
			evt->req_evt.admin_op.inv_id,SA_AIS_ERR_NO_OP);  

	if (SA_AIS_OK != ret_err) {
		LOG_ER("Sending response to IMM failed. Ent: %s, \
		Operations: Unlckinst, ret_err: %d",tmp,ret_err);
	}

	TRACE_LEAVE2("ret_err: %d",ret_err);
	return ret_err;
}
/******************************************************************************
@brief		: Locked ==> shutdown 
@param[in]	: evt - PLMS evt structure holding IMM admin operation info      
@return		: SaAisErrorT. 
******************************************************************************/
SaUint32T plms_EE_adm_locked_state_shutdown_op(  PLMS_EVT *evt)
{
	SaUint32T ret_err = NCSCC_RC_FAILURE;
	PLMS_CB *cb = plms_cb;
	SaInt8T tmp[SA_MAX_NAME_LENGTH +1];

	plms_get_str_from_dn_name(&(evt->req_evt.admin_op.dn_name),tmp);
	TRACE_ENTER2("Entity: %s",tmp);

	LOG_ER("Bad Admin Op, lock to shutdown. Ent: %s",tmp);
	ret_err = saImmOiAdminOperationResult(cb->oi_hdl,
			evt->req_evt.admin_op.inv_id,SA_AIS_ERR_BAD_OPERATION);  

	if (SA_AIS_OK != ret_err) {
		LOG_ER("Sending response to IMM failed. Ent: %s, \
		Operations: Shutdown, ret_err: %d",tmp,ret_err);
	}

	TRACE_LEAVE2("ret_err: %d",ret_err);
	return ret_err;
}
                  
/******************************************************************************
@brief		: Locked ==> lckinst 
@param[in]	: evt - PLMS evt structure holding IMM admin operation info      
@return		: NCSCC_RC_SUCCESS/SA_AIS_OK(1) - For success.
		  NCSCC_RC_FAILURE(0) - For failure.
		  others(SaAisErrorT) - For failure.
******************************************************************************/
SaUint32T plms_EE_adm_locked_state_lckinst_op(   PLMS_EVT *evt )
{
	SaUint32T ret_err = NCSCC_RC_FAILURE;
	PLMS_CB *cb = plms_cb;
	PLMS_ENTITY *ent;
	SaInt8T tmp[SA_MAX_NAME_LENGTH +1];

	plms_get_str_from_dn_name(&(evt->req_evt.admin_op.dn_name),tmp);
	TRACE_ENTER2("Entity: %s. Lock to lckinst.",tmp);

	ent = (PLMS_ENTITY *)ncs_patricia_tree_get(&(cb->entity_info),
			   (SaUint8T *)&(evt->req_evt.admin_op.dn_name));
	if (NULL == ent) {
		LOG_ER("Lckinst can not be performed as the ent in entity_info \
		tree is not found for dn_name: %s",tmp);
		ret_err = saImmOiAdminOperationResult(cb->oi_hdl,
			evt->req_evt.admin_op.inv_id,SA_AIS_ERR_NO_RESOURCES);  
		if (SA_AIS_OK != ret_err){
			LOG_ER("Sending response to IMM failed. Ent: %s, \
			Operation: LckInst, ret_err: %d",tmp,ret_err);
		}
		TRACE_LEAVE2("ret_err: %d",ret_err);
		return ret_err;
	}
	/* I should not be in mid of any operation.*/
	if (ent->am_i_aff_ent){
		LOG_ER("Admin Lckinst can not be performed as the entity %s \
		is in another admin context.",tmp);
		ret_err = saImmOiAdminOperationResult(cb->oi_hdl,
			evt->req_evt.admin_op.inv_id,SA_AIS_ERR_TRY_AGAIN);
		if (SA_AIS_OK != ret_err){
			LOG_ER("Sending response to IMM failed. Ent: %s, \
			Operation: Lckinst, ret_err: %d",tmp,ret_err);
		}
		return ret_err;    
	}
	/* TODO: EE is already in locked state. So all affected entities
	would be in OOS. So the affected entities for this operation are
	none.*/
	/* Mark the admin state to locked-inactive.*/
	plms_admin_state_set(ent, SA_PLM_EE_ADMIN_LOCKED_INSTANTIATION,
				NULL,SA_NTF_MANAGEMENT_OPERATION,
				SA_PLM_NTFID_STATE_CHANGE_ROOT);

	/* Terminate the EE. */
	ret_err = plms_ee_term(ent,TRUE,1/*mngt_cbk*/);
	if (NCSCC_RC_SUCCESS != ret_err){
		LOG_ER("EE Termination failed. Ent: %s",tmp);
		
		ret_err = saImmOiAdminOperationResult(cb->oi_hdl,
			evt->req_evt.admin_op.inv_id,SA_AIS_ERR_DEPLOYMENT);
		if (SA_AIS_OK != ret_err){
			LOG_ER("Sending response to IMM failed. Ent: %s, \
			Operation: Lckinst, ret_err: %d",tmp,ret_err);
		}
	}else{

		/* I am done here. Uninstantiation process will be handled by 
		presence state FSM. I am not waiting till I get the 
		uninstantiated from PLMc for this entity. */
		ret_err = saImmOiAdminOperationResult(cb->oi_hdl,
			evt->req_evt.admin_op.inv_id,SA_AIS_OK);
		if (SA_AIS_OK != ret_err){
			LOG_ER("Sending response to IMM failed. Ent: %s, \
			Operation: Lckinst, ret_err: %d",tmp,ret_err);
		}
	}

	TRACE_LEAVE2("ret_err: %d",ret_err);
	return ret_err;

}
/******************************************************************************
@brief		: Shutdown ==> unlock
@param[in]	: evt - PLMS evt structure holding IMM admin operation info      
@return		: NCSCC_RC_SUCCESS/SA_AIS_OK(1) - For success.
		  NCSCC_RC_FAILURE(0) - For failure.
		  others(SaAisErrorT) - For failure.
******************************************************************************/
SaUint32T plms_EE_adm_shutdown_state_unlock_op(  PLMS_EVT *evt)
{
	SaUint32T ret_err = NCSCC_RC_FAILURE;
	SaInt8T tmp[SA_MAX_NAME_LENGTH +1];

	plms_get_str_from_dn_name(&(evt->req_evt.admin_op.dn_name),tmp);
	TRACE_ENTER2("Entity: %s. Shutdown to unlock",tmp);

	ret_err = plms_ent_shutdown_to_unlock(evt);
	if (NCSCC_RC_SUCCESS != ret_err){
	}

	TRACE_LEAVE2("ret_err: %d",ret_err);
	return ret_err;
}
/******************************************************************************
@brief		: Shutdown ==> lock
@param[in]	: evt - PLMS evt structure holding IMM admin operation info      
@return		: NCSCC_RC_SUCCESS/SA_AIS_OK(1) - For success.
		  NCSCC_RC_FAILURE(0) - For failure.
		  others(SaAisErrorT) - For failure.
******************************************************************************/
SaUint32T plms_EE_adm_shutdown_state_lock_op(  PLMS_EVT *evt)
{
	SaUint32T ret_err = NCSCC_RC_FAILURE;
	SaInt8T tmp[SA_MAX_NAME_LENGTH +1];

	plms_get_str_from_dn_name(&(evt->req_evt.admin_op.dn_name),tmp);
	TRACE_ENTER2("Entity: %s. Shutdown to lock",tmp);

	ret_err = plms_ent_shutdown_to_lock(evt);
	if (NCSCC_RC_SUCCESS != ret_err){
	}

	TRACE_LEAVE2("ret_err: %d",ret_err);
	return ret_err;
}
/******************************************************************************
@brief		: Shutdown ==> shutdown 
@param[in]	: evt - PLMS evt structure holding IMM admin operation info      
@return		: SaAisErrorT. 
******************************************************************************/
SaUint32T plms_EE_adm_shutdown_state_shutdown_op(  PLMS_EVT *evt)
{
	SaUint32T ret_err = NCSCC_RC_FAILURE;
	PLMS_CB *cb = plms_cb;
	SaInt8T tmp[SA_MAX_NAME_LENGTH +1];

	plms_get_str_from_dn_name(&(evt->req_evt.admin_op.dn_name),tmp);
	TRACE_ENTER2("Entity: %s",tmp);

	LOG_ER("Bad Admin Op, shutdown to shutdown. Ent: %s",tmp);
	ret_err = saImmOiAdminOperationResult(cb->oi_hdl,
			evt->req_evt.admin_op.inv_id,SA_AIS_ERR_NO_OP);  

	if (SA_AIS_OK != ret_err) {
		LOG_ER("Sending response to IMM failed. Ent: %s, \
		Operations: Shutdown, ret_err: %d",tmp,ret_err);
	}

	TRACE_LEAVE2("ret_err: %d",ret_err);
	return ret_err;
}
/******************************************************************************
@brief		: Shutdown ==> lckinst 
@param[in]	: evt - PLMS evt structure holding IMM admin operation info      
@return		: SaAisErrorT. 
******************************************************************************/
SaUint32T plms_EE_adm_shutdown_state_lckinst_op(  PLMS_EVT *evt)
{
	SaUint32T ret_err = NCSCC_RC_FAILURE;
	PLMS_CB *cb = plms_cb;
	SaInt8T tmp[SA_MAX_NAME_LENGTH +1];

	plms_get_str_from_dn_name(&(evt->req_evt.admin_op.dn_name),tmp);
	TRACE_ENTER2("Entity: %s",tmp);

	LOG_ER("Bad Admin Op, shutdown to lckinst. Ent: %s",tmp);
	ret_err = saImmOiAdminOperationResult(cb->oi_hdl,
			evt->req_evt.admin_op.inv_id,SA_AIS_ERR_BAD_OPERATION);  
	if (SA_AIS_OK != ret_err) {
		LOG_ER("Sending response to IMM failed. Ent: %s, \
		Operation: Lckinst, ret_err: %d",tmp,ret_err);
	}

	TRACE_LEAVE2("ret_err: %d",ret_err);
	return ret_err;
}
/******************************************************************************
@brief		: Shutdown ==> unlckinst 
@param[in]	: evt - PLMS evt structure holding IMM admin operation info      
@return		: SaAisErrorT. 
******************************************************************************/
SaUint32T plms_EE_adm_shutdown_state_unlckinst_op(  PLMS_EVT *evt)
{
	SaUint32T ret_err = NCSCC_RC_FAILURE;
	PLMS_CB *cb = plms_cb;
	SaInt8T tmp[SA_MAX_NAME_LENGTH +1];

	plms_get_str_from_dn_name(&(evt->req_evt.admin_op.dn_name),tmp);
	TRACE_ENTER2("Entity: %s",tmp);

	LOG_ER("Bad Admin Op, shutdown to unlckinst. Ent: %s",tmp);
	ret_err = saImmOiAdminOperationResult(cb->oi_hdl,
			evt->req_evt.admin_op.inv_id,SA_AIS_ERR_BAD_OPERATION);  
	if (SA_AIS_OK != ret_err) {
		LOG_ER("Sending response to IMM failed. Ent: %s, \
		Operations: Unlckinst, ret_err: %d",tmp,ret_err);
	}

	TRACE_LEAVE2("ret_err: %d",ret_err);
	return ret_err;
}
/******************************************************************************
@brief		: lckinst ==> shutdown
@param[in]	: evt - PLMS evt structure holding IMM admin operation info      
@return		: SaAisErrorT. 
******************************************************************************/
SaUint32T plms_EE_adm_lckinst_state_shutdown_op(  PLMS_EVT *evt)
{
	SaUint32T ret_err = NCSCC_RC_FAILURE;
	PLMS_CB *cb = plms_cb;
	SaInt8T tmp[SA_MAX_NAME_LENGTH +1];

	plms_get_str_from_dn_name(&(evt->req_evt.admin_op.dn_name),tmp);
	TRACE_ENTER2("Entity: %s",tmp);

	LOG_ER("Bad Admin Op, lckinst to shutdown. Ent: %s",tmp);
	ret_err = saImmOiAdminOperationResult(cb->oi_hdl,
			evt->req_evt.admin_op.inv_id,SA_AIS_ERR_BAD_OPERATION);  
	if (SA_AIS_OK != ret_err) {
		LOG_ER("Sending response to IMM failed. Ent: %s, \
		Operations: Shutdown, ret_err: %d",tmp,ret_err);
	}

	TRACE_LEAVE2("ret_err: %d",ret_err);
	return ret_err;
}
/******************************************************************************
@brief		: lckinst ==> lock 
@param[in]	: evt - PLMS evt structure holding IMM admin operation info      
@return		: SaAisErrorT. 
******************************************************************************/
SaUint32T plms_EE_adm_lckinst_state_lock_op(  PLMS_EVT *evt)
{
	SaUint32T ret_err = NCSCC_RC_FAILURE;
	PLMS_CB *cb = plms_cb;
	SaInt8T tmp[SA_MAX_NAME_LENGTH +1];

	plms_get_str_from_dn_name(&(evt->req_evt.admin_op.dn_name),tmp);
	TRACE_ENTER2("Entity: %s",tmp);

	LOG_ER("Bad Admin Op, lckinst to lock. Ent: %s",tmp);
	ret_err = saImmOiAdminOperationResult(cb->oi_hdl,
			evt->req_evt.admin_op.inv_id,SA_AIS_ERR_BAD_OPERATION);  
	if (SA_AIS_OK != ret_err) {
		LOG_ER("Sending response to IMM failed. Ent: %s, \
		Operations: Lock, ret_err: %d",tmp,ret_err);
	}

	TRACE_LEAVE2("ret_err: %d",ret_err);
	return ret_err;
}
/******************************************************************************
@brief		: lckinst ==> unlock
@param[in]	: evt - PLMS evt structure holding IMM admin operation info      
@return		: SaAisErrorT. 
******************************************************************************/
SaUint32T plms_EE_adm_lckinst_state_unlock_op(  PLMS_EVT *evt)
{
	SaUint32T ret_err = NCSCC_RC_FAILURE;
	PLMS_CB *cb = plms_cb;
	SaInt8T tmp[SA_MAX_NAME_LENGTH +1];

	plms_get_str_from_dn_name(&(evt->req_evt.admin_op.dn_name),tmp);
	TRACE_ENTER2("Entity: %s",tmp);

	LOG_ER("Bad Admin Op, lckinst to unlock. Ent: %s",tmp);
	ret_err = saImmOiAdminOperationResult(cb->oi_hdl,
			evt->req_evt.admin_op.inv_id,SA_AIS_ERR_BAD_OPERATION);  
	if (SA_AIS_OK != ret_err) {
		LOG_ER("Sending response to IMM failed. Ent: %s, \
		Operations: Unlock, ret_err: %d",tmp,ret_err);
	}

	TRACE_LEAVE2("ret_err: %d",ret_err);
	return ret_err;
}
/******************************************************************************
@brief		: lckinst ==> unlckinst
@param[in]	: evt - PLMS evt structure holding IMM admin operation info      
@return		: NCSCC_RC_SUCCESS/SA_AIS_OK(1) - For success.
		  NCSCC_RC_FAILURE(0) - For failure.
		  others(SaAisErrorT) - For failure.
******************************************************************************/
SaUint32T plms_EE_adm_lckinst_state_unlckinst_op(  PLMS_EVT *evt)
{

	SaUint32T ret_err = NCSCC_RC_FAILURE;
	PLMS_CB *cb = plms_cb;
	PLMS_ENTITY *ent;
	SaInt8T tmp[SA_MAX_NAME_LENGTH +1];

	plms_get_str_from_dn_name(&(evt->req_evt.admin_op.dn_name),tmp);
	TRACE_ENTER2("Entity: %s. Lckinst to Unlckinst.",tmp);

	ent = (PLMS_ENTITY *)ncs_patricia_tree_get(&(cb->entity_info),
			   (SaUint8T *)&(evt->req_evt.admin_op.dn_name));
	if (NULL == ent) {
		LOG_ER("Unlckinst can not be performed as the ent in \
		entity_info tree is not found for dn_name: %s",tmp);

		ret_err = saImmOiAdminOperationResult(cb->oi_hdl,
			evt->req_evt.admin_op.inv_id,SA_AIS_ERR_NO_RESOURCES);  
		if (SA_AIS_OK != ret_err){
			LOG_ER("Sending response to IMM failed. Ent: %s, \
			Operation: unlckinst, ret_err: %d",tmp,ret_err);
		}

		TRACE_LEAVE2("ret_err: %d",ret_err);
		return ret_err;
	}
	/* I should not be in mid of any operation.*/
	if (ent->am_i_aff_ent){
		LOG_ER("Admin unlckinst can not be performed as the ent \
		is in another admin operaional context. Ent: %s", \
		ent->dn_name_str);

		ret_err = saImmOiAdminOperationResult(cb->oi_hdl,
			evt->req_evt.admin_op.inv_id,SA_AIS_ERR_TRY_AGAIN);
		if (SA_AIS_OK != ret_err){
			LOG_ER("Sending response to IMM failed. Ent: %s, \
			Operation: unlckinst, ret_err: %d",tmp,ret_err);
		}

		TRACE_LEAVE2("ret_err: %d",ret_err);
		return ret_err;    
	}
	/* TODO: Even if the EE is in instantiated state , the admin state 
	still will be locked and hence the readiness state will still be
	OOS. So the there would not be any entity which would be 
	affected by this operation.*/
	/* Mark the admin state to locked.*/
	plms_admin_state_set(ent,SA_PLM_EE_ADMIN_LOCKED,NULL,
				SA_NTF_MANAGEMENT_OPERATION,
				SA_PLM_NTFID_STATE_CHANGE_ROOT);

	/* Instantiate the EE. */
	ret_err = plms_ee_instantiate(ent,TRUE,TRUE/*mngt_cbk*/);
	if (NCSCC_RC_SUCCESS != ret_err){
		LOG_ER("EE instantiation failed. Ent: %s",tmp);
		
		ret_err = saImmOiAdminOperationResult(cb->oi_hdl,
			evt->req_evt.admin_op.inv_id,SA_AIS_ERR_DEPLOYMENT);
		if (SA_AIS_OK != ret_err){
			LOG_ER("Sending response to IMM failed. Ent: %s, \
			Operation: lckinst, ret_err: %d",tmp,ret_err);
		}
	}else{
	
		ret_err = saImmOiAdminOperationResult(cb->oi_hdl,
			evt->req_evt.admin_op.inv_id,SA_AIS_OK);
		if (SA_AIS_OK != ret_err){
			LOG_ER("Sending response to IMM failed. Ent: %s, \
			Operation: lckinst, ret_err: %d",tmp,ret_err);
		}
	}
	
	TRACE_LEAVE2("ret_err: %d",ret_err);
	return ret_err;
}
/******************************************************************************
@brief		: lckinst ==> lckinst 
@param[in]	: evt - PLMS evt structure holding IMM admin operation info      
@return		: SaAisErrorT. 
******************************************************************************/
SaUint32T plms_EE_adm_lckinst_state_lckinst_op(  PLMS_EVT *evt)
{
	SaUint32T ret_err = NCSCC_RC_FAILURE;
	PLMS_CB *cb = plms_cb;
	SaInt8T tmp[SA_MAX_NAME_LENGTH +1];

	plms_get_str_from_dn_name(&(evt->req_evt.admin_op.dn_name),tmp);
	TRACE_ENTER2("Entity: %s",tmp);

	LOG_ER("Bad Admin Op, lckinst to lckinst. Ent: %s",tmp);
	ret_err = saImmOiAdminOperationResult(cb->oi_hdl,
			evt->req_evt.admin_op.inv_id,SA_AIS_ERR_NO_OP);  
	if (SA_AIS_OK != ret_err) {
		LOG_ER("Sending response to IMM failed. Ent: %s, \
		Operations: lckinst, ret_err: %d",tmp,ret_err);
	}

	TRACE_LEAVE2("ret_err: %d",ret_err);
	return ret_err;
}

/******************************************************************************
@brief		: EE admin restart.
@param[in]	: evt - PLMS evt structure holding IMM admin operation info
@return		: NCSCC_RC_SUCCESS/SA_AIS_OK(1) - For success.
		  NCSCC_RC_FAILURE(0) - For failure.
		  others(SaAisErrorT) - For failure.
******************************************************************************/
SaUint32T plms_EE_adm_restart( PLMS_EVT *evt)
{
	PLMS_ENTITY *ent;
	SaUint32T ret_err = NCSCC_RC_FAILURE;
	PLMS_GROUP_ENTITY *aff_ent_list=NULL;
	PLMS_CB *cb = plms_cb;
	SaInt8T tmp[SA_MAX_NAME_LENGTH +1];

	plms_get_str_from_dn_name(&(evt->req_evt.admin_op.dn_name),tmp);
	TRACE_ENTER2("Entity: %s. Op: Admin restart variant: %s.",tmp,
	(evt->req_evt.admin_op.option));

	ent = (PLMS_ENTITY *)ncs_patricia_tree_get(&(cb->entity_info),
			   (SaUint8T *)&(evt->req_evt.admin_op.dn_name));
	if (NULL == ent) {
		LOG_ER("Admin restart can not be performed as the ent in\
		entity_info tree is not found for dn_name: %s",tmp);
		
		ret_err = saImmOiAdminOperationResult(cb->oi_hdl,
			evt->req_evt.admin_op.inv_id,SA_AIS_ERR_NO_RESOURCES);
		if (SA_AIS_OK != ret_err){
			LOG_ER("Sending response to IMM failed. Ent: %s, \
			Operation: Admin-restart, ret_err: %d",tmp,ret_err);
		}

		TRACE_LEAVE2("ret_err: %d",ret_err);
		return ret_err;
	}
	
	if( (0 != strcmp(evt->req_evt.admin_op.option,
		SA_PLM_ADMIN_RESTART_OPTION_ABRUPT)) && 
		('\0' != evt->req_evt.admin_op.option[0])){
		LOG_ER("Invalid restart type: %s",evt->req_evt.admin_op.option);
		ret_err = saImmOiAdminOperationResult(cb->oi_hdl,evt->req_evt.admin_op.inv_id,
		SA_AIS_ERR_INVALID_PARAM);
		TRACE_LEAVE2("ret_err: %d",ret_err);
		return ret_err;
	}
	/* If the presence state is not instantiated, return.*/
	if ( SA_PLM_EE_PRESENCE_INSTANTIATED != 
		ent->entity.ee_entity.saPlmEEPresenceState){
		LOG_ER("Admin restart is not performed. Reason: EE %s is not \
		in instantiated state.",tmp);

		ret_err = saImmOiAdminOperationResult(cb->oi_hdl,
			evt->req_evt.admin_op.inv_id,SA_AIS_ERR_BAD_OPERATION);
		if (SA_AIS_OK != ret_err){
			LOG_ER("Sending response to IMM failed. Ent: %s, \
			Operation: Adm-restart, ret_err: %d",tmp,ret_err);
		}

		TRACE_LEAVE2("ret_err: %d",ret_err);
		return ret_err;    
	}
	
	/* Get all the affected entities.*/ 
	plms_affected_ent_list_get(ent,&aff_ent_list,0);
	
	/* If admin operation is in progress for any of the following ent
	then return TRY_AGAIN.
	1. any of the ancestors.
	2. Any of the children
	3. Any ent which is dependent on this EE.
	4. Any of the ent on which I am dependent on.
	Remember spec does say anything about this.
	*/
	if (plms_anc_chld_dep_adm_flag_is_set(ent,aff_ent_list)) {
		LOG_ER("Admin restart for EE %s can not be performed as any of\
		the affected entity is in another operational context.",tmp);
		
		ret_err = saImmOiAdminOperationResult(cb->oi_hdl,
			evt->req_evt.admin_op.inv_id,SA_AIS_ERR_TRY_AGAIN);
		if (SA_AIS_OK != ret_err){
			LOG_ER("Sending response to IMM failed. Ent: %s, \
			Operation: Admin restart, ret_err: %d",tmp,ret_err);
		}
		
		plms_ent_list_free(aff_ent_list);
		aff_ent_list = NULL;

		TRACE_LEAVE2("ret_err: %d",ret_err);
		return ret_err;    
	}
	
	if(0 == strcmp(evt->req_evt.admin_op.option,
		SA_PLM_ADMIN_RESTART_OPTION_ABRUPT)){

		ret_err = plms_ee_abrupt_restart_process(ent,evt,aff_ent_list);

	}else{

		ret_err = plms_ee_graceful_restart_process(
		ent,evt,aff_ent_list);
	}
	
	TRACE_LEAVE2("ret_err: %d",ret_err);
	return ret_err;
}
/******************************************************************************
@brief		: Admin remove. This is valid only if management lost flag
		is set.
@param[in]	: evt - PLMS evt structure holding IMM admin operation info      
@return		: SaAisErrorT. 
******************************************************************************/
SaUint32T plms_adm_remove( PLMS_EVT *evt)
{
	SaUint32T ret_err = NCSCC_RC_FAILURE;
	PLMS_CB *cb = plms_cb;
	PLMS_ENTITY *ent;
	PLMS_GROUP_ENTITY *act_aff_ent_list = NULL,*act_aff_list = NULL;
	PLMS_GROUP_ENTITY *chld_list=NULL,*head,*log_head,*aff_ent_list = NULL;
	PLMS_TRACK_INFO trk_info;
	PLMS_ENTITY_GROUP_INFO_LIST *log_head_grp;
	SaInt8T tmp[SA_MAX_NAME_LENGTH +1];

	plms_get_str_from_dn_name(&(evt->req_evt.admin_op.dn_name),tmp);
	TRACE_ENTER2("Entity: %s. Admin remove.",tmp);

	ent = (PLMS_ENTITY *)ncs_patricia_tree_get(&(cb->entity_info),
			   (SaUint8T *)&(evt->req_evt.admin_op.dn_name));
	if (NULL == ent) {
		LOG_ER("Admin remove operation can not be performed as \
		the ent is not found in entity_info tree for dn_name %s",
		tmp);
		
		ret_err = saImmOiAdminOperationResult(cb->oi_hdl,
			evt->req_evt.admin_op.inv_id,SA_AIS_ERR_NO_RESOURCES);  
		if (SA_AIS_OK != ret_err){
			LOG_ER("Sending response to IMM failed. Ent: %s, \
			Operation: Admin remove, ret_err: %d",tmp,ret_err);
		}

		TRACE_LEAVE2("ret_err: %d",ret_err);
		return ret_err;
	}
	
	/* I should not be in mid of any operation.*/
	if(plms_is_rdness_state_set(ent,SA_PLM_READINESS_IN_SERVICE)){
		plms_affected_ent_list_get(ent,&aff_ent_list,0);
		if (plms_anc_chld_dep_adm_flag_is_set(ent,aff_ent_list)){
			LOG_ER("Admin remove can not be performed as the ent\
			is in another operation context. Ent:%s",
			ent->dn_name_str);

			ret_err = saImmOiAdminOperationResult(cb->oi_hdl,
			evt->req_evt.admin_op.inv_id,SA_AIS_ERR_TRY_AGAIN);
			if (SA_AIS_OK != ret_err){
				LOG_ER("Sending response to IMM failed. Ent:%s,\
				Operation: Admin remove, ret_err: %d",
				tmp,ret_err);
			}
			plms_ent_list_free(aff_ent_list);
			aff_ent_list = NULL;
			TRACE_LEAVE2("ret_err: %d",ret_err);
			return ret_err;    
		}
		plms_ent_list_free(aff_ent_list);
		aff_ent_list = NULL;
	}else if (ent->am_i_aff_ent){
		LOG_ER("Admin remove can not be performed as the ent\
		is in another admin operation context. Ent:%s",
		ent->dn_name_str);

		ret_err = saImmOiAdminOperationResult(cb->oi_hdl,
			evt->req_evt.admin_op.inv_id,SA_AIS_ERR_TRY_AGAIN);
		if (SA_AIS_OK != ret_err){
			LOG_ER("Sending response to IMM failed. Ent: %s, \
			Operation: Admin remove, ret_err: %d",tmp,ret_err);
		}
		
		TRACE_LEAVE2("ret_err: %d",ret_err);
		return ret_err;    
	}
	
	/* Admin remove operation is valid only if the management lost
	flag is set.*/
	if (!plms_rdness_flag_is_set(ent,SA_PLM_RF_MANAGEMENT_LOST)){
		LOG_ER("Management lost flag is not set, and hence \
		admin remove is not valid. Ent: %s",tmp);
		
		/* Return BAD Operation.*/
		ret_err = saImmOiAdminOperationResult(cb->oi_hdl,
		evt->req_evt.admin_op.inv_id,SA_AIS_ERR_BAD_OPERATION);
		if (SA_AIS_OK != ret_err){
			LOG_ER("Sending response to IMM failed. Ent: %s, \
			Operation: Admin remove, ret_err: %d",tmp,ret_err);
		}

		TRACE_LEAVE2("ret_err: %d",ret_err);
		return ret_err; 
	}

	memset(&trk_info,0,sizeof(PLMS_TRACK_INFO));
	if (PLMS_HE_ENTITY == ent->entity_type){

		if ( SA_PLM_HE_PRESENCE_NOT_PRESENT == 
			ent->entity.he_entity.saPlmHEPresenceState){
			
			LOG_ER("HE is not present, and hence \
			admin remove is not valid. Ent: %s",tmp);
		
			/* Return NO Operation.*/
			ret_err = saImmOiAdminOperationResult(cb->oi_hdl,
			evt->req_evt.admin_op.inv_id,SA_AIS_ERR_NO_OP);
			if (SA_AIS_OK != ret_err){
				LOG_ER("Sending response to IMM failed. Ent:%s,\
				Operation: Admin remove, ret_err: %d",
				tmp,ret_err);
			}

			TRACE_LEAVE2("ret_err: %d",ret_err);
			return ret_err; 
		}

		if (SA_PLM_READINESS_OUT_OF_SERVICE == 
				ent->entity.he_entity.saPlmHEReadinessState){
			
			TRACE("Ent %s is already in OOS",ent->dn_name_str);
			ret_err = plms_he_oos_to_np_process(ent);
		}else{
			
			TRACE("Ent %s is not in OOS",ent->dn_name_str);
			ret_err = plms_he_insvc_to_np_process(ent);
		}
	}else{/* For EEs.*/

		if ( SA_PLM_EE_PRESENCE_UNINSTANTIATED == 
			ent->entity.ee_entity.saPlmEEPresenceState){
			
			LOG_ER("EE is in uninst state, and hence \
			admin remove is not valid. Ent: %s",tmp);
		
			/* Return NO Operation.*/
			ret_err = saImmOiAdminOperationResult(cb->oi_hdl,
			evt->req_evt.admin_op.inv_id,SA_AIS_ERR_NO_OP);
			if (SA_AIS_OK != ret_err){
				LOG_ER("Sending response to IMM failed. Ent:%s,\
				Operation: Admin remove, ret_err: %d",
				tmp,ret_err);
			}

			TRACE_LEAVE2("ret_err: %d",ret_err);
			return ret_err; 
		}
		
		/* Mark the presence state of that EE to uninst.*/
		plms_presence_state_set(ent,
					SA_PLM_EE_PRESENCE_UNINSTANTIATED,NULL,
					SA_NTF_MANAGEMENT_OPERATION,
					SA_PLM_NTFID_STATE_CHANGE_ROOT);
	
		/* Clear the readiness flag for the EE. */
		plms_readiness_flag_clear(ent,NULL,SA_NTF_MANAGEMENT_OPERATION,
						SA_PLM_NTFID_STATE_CHANGE_ROOT);
		
		if (SA_PLM_READINESS_OUT_OF_SERVICE == 
				ent->entity.ee_entity.saPlmEEReadinessState){
			
			TRACE("Ent %s is already in OOS",ent->dn_name_str);
#if 1 			/* For virtualisation and never be hit in this 
			release.*/
			plms_chld_get(ent->leftmost_child,&chld_list);
			TRACE("Children of ent %s: ", ent->dn_name_str);
			log_head = chld_list;
			while(log_head){
				TRACE("%s,",log_head->plm_entity->dn_name_str);
				log_head = log_head->next;
			}

			head = chld_list;
			while(head){
				if (PLMS_READINESS_FLAG_RESET != 
				head->plm_entity->entity.ee_entity.
				saPlmEEReadinessFlags){
					/*Clear the readiness flag for the EE.*/
					plms_readiness_flag_clear(ent,NULL,
						SA_NTF_MANAGEMENT_OPERATION,
						SA_PLM_NTFID_STATE_CHANGE_ROOT);
				
					plms_ent_to_ent_list_add(
					head->plm_entity,&act_aff_ent_list);
				}
			}
			trk_info.aff_ent_list = act_aff_ent_list;
			plms_ent_list_free(chld_list);
			chld_list = NULL;
#endif			
		}else{
			TRACE("Ent %s is not in OOS",ent->dn_name_str);
			
			plms_readiness_state_set(
					ent,SA_PLM_READINESS_OUT_OF_SERVICE,
					NULL,SA_NTF_MANAGEMENT_OPERATION,
					SA_PLM_NTFID_STATE_CHANGE_ROOT);
			
			plms_affected_ent_list_get(ent, &act_aff_ent_list,0);
			
			head = act_aff_ent_list;
			while(head){
			
				if (!plms_is_rdness_state_set(head->plm_entity,
				SA_PLM_READINESS_OUT_OF_SERVICE)){	
					plms_readiness_state_set(
					head->plm_entity,
					SA_PLM_READINESS_OUT_OF_SERVICE,ent,
					SA_NTF_MANAGEMENT_OPERATION,
					SA_PLM_NTFID_STATE_CHANGE_DEP);

					plms_ent_to_ent_list_add(
					head->plm_entity,
					&act_aff_list);
				}

				if (PLMS_EE_ENTITY == 
						head->plm_entity->entity_type){
					/* Terminate all the affected EEs which 
					are not children of the root EE.*/
					if (!plms_is_chld(ent,
							head->plm_entity)){
						ret_err = plms_ee_term(
						head->plm_entity,FALSE,0);
					
						if(!plms_rdness_flag_is_set(
						head->plm_entity,
						SA_PLM_RF_DEPENDENCY)){
						
						plms_readiness_flag_mark_unmark(
						head->plm_entity,
						SA_PLM_RF_DEPENDENCY,TRUE,
						ent,SA_NTF_MANAGEMENT_OPERATION,
						SA_PLM_NTFID_STATE_CHANGE_DEP);

						plms_ent_to_ent_list_add(
						head->plm_entity,
						&act_aff_list);

						}
					}else{ /* Virtualisation.*/
						plms_presence_state_set(
						head->plm_entity,
						SA_PLM_EE_PRESENCE_UNINSTANTIATED,
						ent,
						SA_NTF_MANAGEMENT_OPERATION,
						SA_PLM_NTFID_STATE_CHANGE_DEP);
						if (PLMS_READINESS_FLAG_RESET !=
						ent->entity.ee_entity.
						saPlmEEReadinessFlags){
							
						plms_readiness_flag_clear(
						head->plm_entity,ent,
						SA_NTF_MANAGEMENT_OPERATION,
						SA_PLM_NTFID_STATE_CHANGE_DEP);

						plms_ent_to_ent_list_add(
						head->plm_entity,
						&act_aff_list);

						}
							
					}

				}else{/* Virtualization and never be hit in 
					this release.*/
					 if(!plms_rdness_flag_is_set(
					 head->plm_entity,
					 SA_PLM_RF_DEPENDENCY)){
						plms_readiness_flag_mark_unmark(
						head->plm_entity,
						SA_PLM_RF_DEPENDENCY, TRUE,
						ent,SA_NTF_MANAGEMENT_OPERATION,
						SA_PLM_NTFID_STATE_CHANGE_DEP);

						plms_ent_to_ent_list_add(
						head->plm_entity,
						&act_aff_list);

					 }
				}

				head = head->next;
			}
			plms_ent_list_free(act_aff_ent_list);
			trk_info.aff_ent_list = act_aff_list;
		}
		
		TRACE("Act aff ent of %s for which the callback will be called\
		: ", ent->dn_name_str);
		log_head = trk_info.aff_ent_list;
		while(log_head){
			TRACE("%s,",log_head->plm_entity->dn_name_str);
			log_head = log_head->next;
		}
	
		plms_aff_ent_exp_rdness_state_ow(trk_info.aff_ent_list);
		plms_ent_exp_rdness_state_ow(ent);
		
		trk_info.group_info_list = NULL;
		/* Add the groups, root entity(ent) belong to.*/
		plms_ent_grp_list_add(ent,
				&(trk_info.group_info_list));
	
		/* Find out all the groups, all affected entities 
		 * belong to and add the groups to trk_info->group_info_list. 
		TODO: We have to free trk_info->group_info_list. */
		plms_ent_list_grp_list_add(trk_info.aff_ent_list,
					&(trk_info.group_info_list));
		TRACE("Affected groups for ent %s: ",ent->dn_name_str);
		log_head_grp = trk_info.group_info_list;
		while(log_head_grp){
			TRACE("%llu,",log_head_grp->ent_grp_inf->entity_grp_hdl);
			log_head_grp = log_head_grp->next;
		}

		trk_info.change_step = SA_PLM_CHANGE_COMPLETED;
		trk_info.root_entity = ent;
		trk_info.root_correlation_id = SA_NTF_IDENTIFIER_UNUSED;
		trk_info.track_cause = SA_PLM_CAUSE_EE_UNINSTANTIATED;
		trk_info.grp_op = SA_PLM_GROUP_MEMBER_READINESS_CHANGE;

		plms_cbk_call(&trk_info,1);

		plms_aff_ent_exp_rdness_status_clear(trk_info.aff_ent_list);
		plms_ent_exp_rdness_status_clear(ent);
		
		plms_ent_list_free(trk_info.aff_ent_list);
		trk_info.aff_ent_list = NULL;
		
		plms_ent_grp_list_free(trk_info.group_info_list);
		trk_info.group_info_list = NULL;	
	}	

	/* Return OK to IMM.*/
	ret_err = saImmOiAdminOperationResult(cb->oi_hdl,
			evt->req_evt.admin_op.inv_id,SA_AIS_OK);
	if (NCSCC_RC_SUCCESS != ret_err){
		LOG_ER("Sending admin response to IMM failed. \
						IMM Ret code: %d",ret_err);
	}else{
		TRACE("Sending admin response to IMM successful.");
	}

	TRACE_LEAVE2("ret_err: %d",ret_err);
	return ret_err;

}	
/******************************************************************************
@brief		: Admin repair, fault is repaired and hence enable the EE. 
@param[in]	: evt - PLMS evt structure holding IMM admin operation info      
@return		: SaAisErrorT. 
******************************************************************************/
SaUint32T plms_adm_repair( PLMS_EVT *evt)
{
	
	SaUint32T ret_err = NCSCC_RC_FAILURE,is_set = 0,is_enabled = 0;
	PLMS_CB *cb = plms_cb;
	PLMS_ENTITY *ent;
	PLMS_TRACK_INFO trk_info;
	PLMS_TRACK_INFO *trk_info_flt;
	PLMS_GROUP_ENTITY *aff_ent_list = NULL,*head,*log_head;
	PLMS_GROUP_ENTITY *aff_ent_list_flag = NULL;
	PLMS_ENTITY_GROUP_INFO_LIST *log_head_grp;
	SaInt8T tmp[SA_MAX_NAME_LENGTH +1];
	SaPlmTrackCauseT trk_cause = 0;

	plms_get_str_from_dn_name(&(evt->req_evt.admin_op.dn_name),tmp);
	TRACE_ENTER2("Entity: %s. Admin repair.",tmp);

	ent = (PLMS_ENTITY *)ncs_patricia_tree_get(&(cb->entity_info),
			   (SaUint8T *)&(evt->req_evt.admin_op.dn_name));
	if (NULL == ent) {
		
		LOG_ER("Admin repair operation can not be performed as \
		the ent is not found in entity_info tree for dn_name %s",
		tmp);

		ret_err = saImmOiAdminOperationResult(cb->oi_hdl,
			evt->req_evt.admin_op.inv_id,SA_AIS_ERR_NO_RESOURCES);  
		if (NCSCC_RC_SUCCESS != ret_err){
			LOG_ER("Sending admin response to IMM failed. \
						IMM Ret code: %d",ret_err);
		}else{
			TRACE("Sending admin response to IMM successful.");
		}
		TRACE_LEAVE2("ret_err: %d",ret_err);
		return ret_err;
	}

	/* I should not be in mid of any operation.*/
	if (ent->am_i_aff_ent){
		
		LOG_ER("Admin repair can not be performed as the ent\
		is in another admin operation context. Ent:%s, Prev Adm op:%d",
		ent->dn_name_str,ent->adm_op_in_progress);

		ret_err = saImmOiAdminOperationResult(cb->oi_hdl,
			evt->req_evt.admin_op.inv_id,SA_AIS_ERR_TRY_AGAIN);
		if (NCSCC_RC_SUCCESS != ret_err){
			LOG_ER("Sending admin response to IMM failed. \
						IMM Ret code: %d",ret_err);
		}else{
			TRACE("Sending admin response to IMM successful.");
		}
		TRACE_LEAVE2("ret_err: %d",ret_err);
		return ret_err;    
	}
	
	/* If the opearational state is enable and the imminent-failure flag 
	 * is not set then return.
	 * */
	if (((PLMS_HE_ENTITY == ent->entity_type) && 
		(SA_PLM_OPERATIONAL_ENABLED == 
		ent->entity.he_entity.saPlmHEOperationalState) && 
		!(plms_rdness_flag_is_set(ent,SA_PLM_RF_IMMINENT_FAILURE))) || 
		((PLMS_EE_ENTITY == ent->entity_type) &&
			( SA_PLM_OPERATIONAL_ENABLED ==
			ent->entity.ee_entity.saPlmEEOperationalState) &&
		!(plms_rdness_flag_is_set(ent,SA_PLM_RF_IMMINENT_FAILURE))) ){
		
		LOG_ER("Operational state is ENABLED and imminent failure flag\
		is not set. Nothing to do. Return.Ent: %s",ent->dn_name_str);
		ret_err = saImmOiAdminOperationResult(cb->oi_hdl,
			evt->req_evt.admin_op.inv_id,SA_AIS_ERR_NO_OP);
		if (NCSCC_RC_SUCCESS != ret_err){
			LOG_ER("Sending admin response to IMM failed. \
						IMM Ret code: %d",ret_err);
		}else{
			TRACE("Sending admin response to IMM successful.");
		}
		TRACE_LEAVE2("ret_err: %d",ret_err);
		return ret_err;
	}
	
	/* If the operational state is enabled and imminent-failure flag
	 * is set then
	 * 1. Failure has not occured yet.
	 * 2. The entity is not yet isolated.
	 * So clear the imminent-failure flag of the ent and 
	 * dep-imminent-flag for all the aff entities.*/
	if (((PLMS_HE_ENTITY == ent->entity_type) && 
		(SA_PLM_OPERATIONAL_ENABLED == 
		ent->entity.he_entity.saPlmHEOperationalState) && 
		(plms_rdness_flag_is_set(ent,SA_PLM_RF_IMMINENT_FAILURE))) || 
		((PLMS_EE_ENTITY == ent->entity_type) &&
			( SA_PLM_OPERATIONAL_ENABLED ==
			ent->entity.ee_entity.saPlmEEOperationalState) &&
		(plms_rdness_flag_is_set(ent,SA_PLM_RF_IMMINENT_FAILURE))) ){

		LOG_ER("Operational state is ENABLED and imminent failure flag\
		is set==>Failure is not yet happened.Ent: %s",ent->dn_name_str);

		/* Find the affected entities whose dep-imminent-failure
		 * flag is set because of this ent. We have to clear the flag.*/
		plms_aff_ent_list_imnt_failure_get(ent,&aff_ent_list,
								0/* unmark */);
		TRACE("Affected(imnt_failure flag to be cleared) entities for \
		ent %s: ", ent->dn_name_str);
		log_head = aff_ent_list;
		while(log_head){
			TRACE("%s,",log_head->plm_entity->dn_name_str);
			log_head = log_head->next;
		}
		if (plms_anc_chld_dep_adm_flag_is_set(ent,aff_ent_list)){
			LOG_ER("Adm repair can not be performed as the ent\
			is in some other operational context. Ent : %s",
			ent->dn_name_str);
			ret_err = saImmOiAdminOperationResult(cb->oi_hdl,
			evt->req_evt.admin_op.inv_id,SA_AIS_ERR_TRY_AGAIN);
			if (NCSCC_RC_SUCCESS != ret_err){
				LOG_ER("Sending admin response to IMM failed. \
						IMM Ret code: %d",ret_err);
			}else{
				TRACE("Sending admin response to IMM \
				successful.");
			}
			plms_ent_list_free(aff_ent_list);
			TRACE_LEAVE2("ret_err: %d",ret_err);
			return ret_err;
		}
		
		plms_readiness_flag_mark_unmark(ent,
				SA_PLM_RF_IMMINENT_FAILURE,0/*unmark*/,
				NULL,SA_NTF_MANAGEMENT_OPERATION,
				SA_PLM_NTFID_STATE_CHANGE_ROOT);

		head = aff_ent_list;
		while(head){
			plms_readiness_flag_mark_unmark(head->plm_entity,
					SA_PLM_RF_DEPENDENCY_IMMINENT_FAILURE,
					0/*unmark*/,ent,
					SA_NTF_MANAGEMENT_OPERATION,
					SA_PLM_NTFID_STATE_CHANGE_DEP);
			head = head->next;
		}
		trk_cause = SA_PLM_CAUSE_IMMINENT_FAILURE_CLEARED;

	/* If the operational state is disabled, means imminent failure flag
	 * no set. Hence 
	 * 1. Mark the operational state to enable.
	 * 2. Clear isolate pending flag.
	 * 3. Clear imminent failure flag(Not required as wont be set).
	 * 4. activate/instantiate the ent (as the entity would have been 
	 *    isolated  and hence would be in inactive/uninstantiated state).
	 * 5. Clear dependency-imminent-failure flag of all the aff ents(
	 *    not needed as wont be set.).
	*/
	}else if (((PLMS_HE_ENTITY == ent->entity_type) && 
		(SA_PLM_OPERATIONAL_DISABLED == 
		ent->entity.he_entity.saPlmHEOperationalState)) || 
		((PLMS_EE_ENTITY == ent->entity_type) &&
			( SA_PLM_OPERATIONAL_DISABLED ==
			ent->entity.ee_entity.saPlmEEOperationalState))){

		TRACE("Operational state of the ent is DISABLED. Ent: %s",
		ent->dn_name_str);	
#if 0		
		/* Return if management lost flag is set as the entity
		 * can not be activated/instantiated even if the fault 
		 * is cleared. */
		if (plms_rdness_flag_is_set(ent,SA_PLM_RF_MANAGEMENT_LOST)){
			/* Return BAD Operation.*/
			ret_err = saImmOiAdminOperationResult(cb->oi_hdl,
			evt->req_evt.admin_op.inv_id,SA_AIS_ERR_TRY_AGAIN);

			return ret_err;
		}
#endif
		/* Take care of myself. */
		plms_op_state_set(ent,SA_PLM_OPERATIONAL_ENABLED,NULL,
				SA_NTF_MANAGEMENT_OPERATION,
				SA_PLM_NTFID_STATE_CHANGE_ROOT);
		
		plms_readiness_flag_mark_unmark(ent,
				SA_PLM_RF_ISOLATE_PENDING,0/* unmark*/,
				NULL,SA_NTF_MANAGEMENT_OPERATION,
				SA_PLM_NTFID_STATE_CHANGE_ROOT);
		
		/* Check is the imminet-failure flag of the entity can be set.*/
		plms_dep_immi_flag_check_and_set(ent,&aff_ent_list_flag,&is_set);
#if 0			
		/* Operational state of the root entity was disabled.
		 * Hence imminent-failure flag would have ben cleared. 
		 So we do not need to clear dep-imminent-failure flag of aff 
		 ents as it would not have been set.*/
		 
		plms_readiness_flag_mark_unmark(ent,
				SA_PLM_RF_IMMINENT_FAILURE,0/*unmark*/);
#endif		
		/* As the ent was in disable state, it would be in
		 * inactive/terminated state. I am activating/instantiating 
		 * the ent. Taking the entity to insvc does not make much sence
		 * here. Hence delay that until the ent is 
		 * activated/instantiated.*/
		trk_cause = SA_PLM_CAUSE_FAILURE_CLEARED;
		ret_err = plms_ent_enable(ent,TRUE,TRUE);
		if (NCSCC_RC_SUCCESS != ret_err){
			is_enabled = 0;
			LOG_ER("Enabling the ent %s FAILED.",ent->dn_name_str);
			if (!is_set){
				/* I am done.*/
				ret_err = saImmOiAdminOperationResult(cb->oi_hdl,evt->req_evt.admin_op.inv_id,
				SA_AIS_OK);
				if (NCSCC_RC_SUCCESS != ret_err){
					LOG_ER("Sending admin response to IMM failed.IMM Ret code: %d",ret_err);
				}else{
					TRACE("Sending admin response to IMM successful.");
				}
				return ret_err;
			}
		}else {
			is_enabled = 1;
			/* There is no change in readiness status in this context. Hence store the context,
			and use the cause when the EE/HE is instantiated/activated.*/
			trk_info_flt =(PLMS_TRACK_INFO *)calloc(1,sizeof(PLMS_TRACK_INFO));
			if (NULL == trk_info_flt){
				LOG_CR("adm_repair, calloc of track info failed.error: %s",strerror(errno));
				assert(0);	
			}
			memset(trk_info_flt,0,sizeof(PLMS_TRACK_INFO));
			trk_info_flt->imm_adm_opr_id = evt->req_evt.admin_op.operation_id; 
			trk_info_flt->inv_id = evt->req_evt.admin_op.inv_id;
			trk_info_flt->track_cause = trk_cause;
			trk_info_flt->root_entity = ent;
			ent->trk_info = trk_info_flt;
			
			/* Admin operation started. */ 
			ent->adm_op_in_progress = SA_PLM_CAUSE_FAILURE_CLEARED; 
			ent->am_i_aff_ent = TRUE;
			
			if (!is_set){
				return ret_err;
			}
		}

	}
	/* Join the two aff ent list.*/
	if (NULL != aff_ent_list_flag){
		head = aff_ent_list_flag;
		while (head){
			plms_ent_to_ent_list_add(head->plm_entity,&aff_ent_list);
			head = head->next;
		}
	}
	plms_aff_ent_exp_rdness_state_ow(aff_ent_list);
	plms_ent_exp_rdness_state_ow(ent);
	
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
	
	trk_info.imm_adm_opr_id = evt->req_evt.admin_op.operation_id; 
	trk_info.inv_id = evt->req_evt.admin_op.inv_id;
	trk_info.change_step = SA_PLM_CHANGE_COMPLETED;
	trk_info.track_cause = trk_cause;
	trk_info.root_correlation_id = SA_NTF_IDENTIFIER_UNUSED;
	trk_info.grp_op = SA_PLM_GROUP_MEMBER_READINESS_CHANGE; 
	trk_info.root_entity = ent;

	plms_cbk_call(&trk_info,1);
	plms_aff_ent_exp_rdness_status_clear(aff_ent_list);
	plms_ent_exp_rdness_status_clear(ent);

	/* This will be hit only if is_set is 1.*/
	if(!is_enabled){ 
		/* I am done.*/
		ret_err = saImmOiAdminOperationResult(cb->oi_hdl,evt->req_evt.admin_op.inv_id,SA_AIS_OK);
		if (NCSCC_RC_SUCCESS != ret_err){
			LOG_ER("Sending admin response to IMM failed.IMM Ret code: %d",ret_err);
		}else{
			TRACE("Sending admin response to IMM successful.");
		}
	}
	plms_ent_list_free(aff_ent_list);
	plms_ent_grp_list_free(trk_info.group_info_list);
	trk_info.group_info_list = NULL;
	trk_info.aff_ent_list = NULL;

	TRACE_LEAVE2("ret_err: %d",ret_err);
	return ret_err;
}
/******************************************************************************
@brief		: Try lock, involves all three callback.
@param[in]	: ent - Entity on which try lock is performed.
@param[in]	: adm_op - IMM admin operation related data.
@param[in]	: aff_ent_list - List of affected entities for this lock operation.
@return		: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
******************************************************************************/
static SaUint32T plms_try_lock(PLMS_ENTITY *ent,PLMS_IMM_ADMIN_OP adm_op,
					PLMS_GROUP_ENTITY *aff_ent_list)
{
	SaUint32T ret_err = NCSCC_RC_SUCCESS;
	PLMS_TRACK_INFO *trk_info;
	PLMS_ENTITY_GROUP_INFO_LIST *log_head_grp;
	
	TRACE_ENTER2("Entity: %s",ent->dn_name_str);
	
	trk_info =(PLMS_TRACK_INFO *)calloc(1,sizeof(PLMS_TRACK_INFO));
	if (NULL == trk_info){
		LOG_CR("Try lock, calloc of track info failed.\
		error: %s",strerror(errno));
		assert(0);	
	}
	
	trk_info->aff_ent_list = aff_ent_list;
	trk_info->group_info_list = NULL;
	ent->trk_info = trk_info;

	/* Add the groups, root entity(ent) belong to.*/
	plms_ent_grp_list_add(ent,&(trk_info->group_info_list));
	
	/* Find out all the groups, all affected entities belong to and add 
	the groups to trk_info->group_info_list.*/ 
	plms_ent_list_grp_list_add(aff_ent_list, &(trk_info->group_info_list));	
	TRACE("Affected groups for ent %s: ",ent->dn_name_str);
	log_head_grp = trk_info->group_info_list;
	while(log_head_grp){
		TRACE("%llu,",log_head_grp->ent_grp_inf->entity_grp_hdl);
		log_head_grp = log_head_grp->next;
	}
	
	plms_aff_ent_exp_rdness_state_mark(trk_info->aff_ent_list,
		SA_PLM_READINESS_OUT_OF_SERVICE,SA_PLM_RF_DEPENDENCY);
	
	/* Fill the expected readiness state of the root entity.*/
	ent->exp_readiness_status.readinessState = 
				SA_PLM_READINESS_OUT_OF_SERVICE;
				
	if (PLMS_HE_ENTITY == ent->entity_type){			
		ent->exp_readiness_status.readinessFlags = 
		ent->entity.he_entity.saPlmHEReadinessFlags;
	}else{
		ent->exp_readiness_status.readinessFlags = 
		ent->entity.ee_entity.saPlmEEReadinessFlags;
	}
	
	/* Populate trk_info. */
	trk_info->imm_adm_opr_id = adm_op.operation_id; 
	trk_info->inv_id = adm_op.inv_id;
	trk_info->change_step = SA_PLM_CHANGE_VALIDATE;
	trk_info->track_cause = SA_PLM_CAUSE_LOCK;
	trk_info->root_correlation_id = SA_NTF_IDENTIFIER_UNUSED;
	trk_info->grp_op = SA_PLM_GROUP_MEMBER_READINESS_CHANGE; 
	trk_info->root_entity = ent;
	
	
	/* Call validate callback for all the interested agents.*/
	plms_cbk_call(trk_info,1);

	/* There are no validate step subcribers, call start callback 
	right away */
	if (0 == trk_info->track_count){
		plms_lock_start_cbk_call(ent,trk_info);	
	}else{
		plms_peer_async_send(ent,SA_PLM_CHANGE_VALIDATE,
		SA_PLM_CAUSE_LOCK);
		TRACE_LEAVE2("ret_err: %d",ret_err);
		return ret_err;
	}
	/* No start step subcriber, call completed right now.*/
	if (0 == trk_info->track_count){
		plms_lock_completed_cbk_call(ent,trk_info);
	}else{
		plms_peer_async_send(ent,SA_PLM_CHANGE_START,
		SA_PLM_CAUSE_LOCK);
	}

	TRACE_LEAVE2("ret_err: %d",ret_err);
	return ret_err;
}

/******************************************************************************
@brief		: Default lock, involves start and completed callback.
@param[in]	: ent - Entity on which try lock is performed.
@param[in]	: adm_op - IMM admin operation related data.
@param[in]	: aff_ent_list - List of affected entities for this lock operation.
@return		: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
******************************************************************************/
static SaUint32T plms_default_lock(PLMS_ENTITY *ent,PLMS_IMM_ADMIN_OP adm_op,
						PLMS_GROUP_ENTITY *aff_ent_list)
{
	SaUint32T ret_err = NCSCC_RC_SUCCESS;
	PLMS_TRACK_INFO *trk_info;
	PLMS_ENTITY_GROUP_INFO_LIST *log_head_grp;
	
	TRACE_ENTER2("Entity: %s",ent->dn_name_str);
	
	trk_info =(PLMS_TRACK_INFO *)calloc(1,sizeof(PLMS_TRACK_INFO));
	if (NULL == trk_info){
		LOG_CR("Default lock, calloc for track info FAILED.\
		error: %s",strerror(errno));
		assert(0);
	}

	trk_info->aff_ent_list = aff_ent_list;
	trk_info->group_info_list = NULL;
	ent->trk_info = trk_info;
	/* Add the groups, root entity(ent) belong to.*/
	plms_ent_grp_list_add(ent,&(trk_info->group_info_list));
	
	/* Find out all the groups, all affected entities belong to and add 
	the groups to trk_info->group_info_list.*/ 
	plms_ent_list_grp_list_add(aff_ent_list, &(trk_info->group_info_list));	
	TRACE("Affected groups for ent %s: ",ent->dn_name_str);
	log_head_grp = trk_info->group_info_list;
	while(log_head_grp){
		TRACE("%llu,",log_head_grp->ent_grp_inf->entity_grp_hdl);
		log_head_grp = log_head_grp->next;
	}

	/* Mark the expected readiness state of the root entity.*/
	ent->exp_readiness_status.readinessState = 
				SA_PLM_READINESS_OUT_OF_SERVICE;
	
	if (PLMS_HE_ENTITY == ent->entity_type){			
		ent->exp_readiness_status.readinessFlags = 
		ent->entity.he_entity.saPlmHEReadinessFlags;
	}else{
		ent->exp_readiness_status.readinessFlags = 
		ent->entity.ee_entity.saPlmEEReadinessFlags;
	}
	
	/* Update expected readiness state of the affected entities.*/
	plms_aff_ent_exp_rdness_state_mark(trk_info->aff_ent_list,
		SA_PLM_READINESS_OUT_OF_SERVICE,SA_PLM_RF_DEPENDENCY);

	/* Populate trk_info. */
	trk_info->imm_adm_opr_id = adm_op.operation_id; 
	trk_info->inv_id = adm_op.inv_id;
	trk_info->change_step = SA_PLM_CHANGE_START;
	trk_info->track_cause = SA_PLM_CAUSE_LOCK;
	trk_info->root_correlation_id = SA_NTF_IDENTIFIER_UNUSED;
	trk_info->grp_op = SA_PLM_GROUP_MEMBER_READINESS_CHANGE; 
	trk_info->root_entity = ent;
	
	/* Call start callback for all the interested agents.*/
	plms_cbk_call(trk_info,1);
	
	/* There are no start step subcriber. Call completed now.*/
	if ( 0 == trk_info->track_count ){
		plms_lock_completed_cbk_call(ent,trk_info);
	}else{
		plms_peer_async_send(ent,SA_PLM_CHANGE_START,SA_PLM_CAUSE_LOCK);
	}

	TRACE_LEAVE2("ret_err: %d",ret_err);
	return ret_err;
}

/******************************************************************************
@brief		: Forced lock, involves only completed callback.
@param[in]	: ent - Entity on which try lock is performed.
@param[in]	: adm_op - IMM admin operation related data.
@param[in]	: aff_ent_list - List of affected entities for this lock operation.
@return		: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
******************************************************************************/
SaUint32T plms_forced_lock(PLMS_ENTITY *ent,PLMS_IMM_ADMIN_OP adm_op,
					PLMS_GROUP_ENTITY *aff_ent_list)
{
	PLMS_TRACK_INFO *trk_info,trk_info_new;
	PLMS_ENTITY_GROUP_INFO_LIST *grp_list_head;
	SaUint32T is_new_req = 0;
	SaUint32T ret_err = NCSCC_RC_SUCCESS;	
	
	TRACE_ENTER2("Entity: %s",ent->dn_name_str);

	/* If the ent is in adm context and forced lock is issued, then 
	clean up the dynamic info before processing this operation.
	This is possible if the validate callback for trylock and deactivation
	is called and an application is yet to send the response.
	*/
	trk_info = ent->trk_info;
	if (ent->adm_op_in_progress && (NULL != trk_info)){

		TRACE("Ent %s is in another operational context.\
		adm_op: %d, step: %d",ent->dn_name_str,ent->adm_op_in_progress,
		trk_info->change_step);

		grp_list_head = trk_info->group_info_list;
		while (grp_list_head){
			/* Remove the inv_to_cbk nodes from the 
			grp->innvocation list, if pointing to this trk_info.*/
			plms_inv_to_cbk_in_grp_trk_rmv(grp_list_head->
							ent_grp_inf,trk_info);
			grp_list_head = grp_list_head->next;
		}
		
		plms_ent_list_free(aff_ent_list);
		aff_ent_list = NULL;

	}else {
		memset(&trk_info_new,0,sizeof(PLMS_TRACK_INFO));
		/* If it is a fresh request. */
		is_new_req = 1;
		trk_info = &trk_info_new;
		/* Get the fresh set of aff entities.*/
		trk_info->aff_ent_list = aff_ent_list;
	}
	

	plms_forced_lock_cbk(ent,trk_info,adm_op,is_new_req/* new aff ent*/,1);

	if (is_new_req){
		/* Free affected entity list and group list.*/
		plms_ent_list_free(trk_info->aff_ent_list);
		trk_info->aff_ent_list = NULL;
		plms_ent_grp_list_free(trk_info->group_info_list);
		trk_info->group_info_list = NULL;
	}else{
		/* I am done with the admin operation.*/
		ent->adm_op_in_progress = FALSE;
		ent->am_i_aff_ent = FALSE;
		plms_aff_ent_mark_unmark(trk_info->aff_ent_list,FALSE);

		plms_trk_info_free(trk_info);
	}
		
	TRACE_LEAVE2("ret_err: %d",ret_err);
	return ret_err;	
}

/******************************************************************************
@brief		: Process the forced lock, responsible for calling the completed
		callback.
@param[in]	: ent - Entity on which force lock is issued.
@param[in]	: trk_info - Track info used for sending completed callback.
@param[in]	: adm_op - IMM admin operation related data.
@param[in]	: new_aff_ent - Affected entities are freshly calculated,
		or we are using the old set of affected entities of the last 
		operation like shutdown/try-lock/deafult-lock.
@param[in]	: term_ee - Terminate the ee (0/1).
@return		: NCSCC_RC_SUCCESS/SA_AIS_OK(1) - For success.
		  NCSCC_RC_FAILURE(0) - For failure.
		  others(SaAisErrorT) - For failure.
******************************************************************************/
static SaUint32T plms_forced_lock_cbk( PLMS_ENTITY *ent, 
		PLMS_TRACK_INFO *trk_info, PLMS_IMM_ADMIN_OP adm_op,
		SaUint32T new_aff_ent,SaUint32T term_ee)
{
	SaUint32T ret_err = NCSCC_RC_FAILURE;
	PLMS_CB *cb = plms_cb;
	PLMS_GROUP_ENTITY *head;
	PLMS_ENTITY_GROUP_INFO_LIST *log_head_grp;
	
	TRACE_ENTER2("Entity: %s",ent->dn_name_str);

	if (new_aff_ent){	
		trk_info->group_info_list = NULL;
		/* Add the groups, root entity(ent) belongs to.*/
		plms_ent_grp_list_add(ent,&(trk_info->group_info_list));
		
		/* Find out all the groups, all affected entities belong to 
		and add the groups to trk_info->group_info_list.*/ 
		plms_ent_list_grp_list_add(trk_info->aff_ent_list,
					&(trk_info->group_info_list));	
	}
	
	TRACE("Affected(to be OOS) groups for ent %s: ",ent->dn_name_str);
	log_head_grp = trk_info->group_info_list;
	while(log_head_grp){
		TRACE("%llu,",log_head_grp->ent_grp_inf->entity_grp_hdl);
		log_head_grp = log_head_grp->next;
	}

	plms_admin_state_set(ent,SA_PLM_HE_ADMIN_LOCKED,NULL,
				SA_NTF_MANAGEMENT_OPERATION,
				SA_PLM_NTFID_STATE_CHANGE_ROOT);

	plms_readiness_state_set(ent,
				SA_PLM_READINESS_OUT_OF_SERVICE,NULL,
				SA_NTF_MANAGEMENT_OPERATION,
				SA_PLM_NTFID_STATE_CHANGE_ROOT);
	

	/* Mark the readiness state and expected readiness state of all the 
	affected entities to OOS and set the dependency flag.*/
	head = trk_info->aff_ent_list;
	while (head){
		plms_readiness_state_set(head->plm_entity,
				SA_PLM_READINESS_OUT_OF_SERVICE,
				ent,SA_NTF_MANAGEMENT_OPERATION,
				SA_PLM_NTFID_STATE_CHANGE_DEP);
		plms_readiness_flag_mark_unmark(head->plm_entity,
				SA_PLM_RF_DEPENDENCY,TRUE,ent,
				SA_NTF_MANAGEMENT_OPERATION,
				SA_PLM_NTFID_STATE_CHANGE_DEP);

		head = head->next;
	}
	plms_aff_ent_exp_rdness_state_ow(trk_info->aff_ent_list);
	plms_ent_exp_rdness_state_ow(ent);
	
	if (term_ee){
		/* Terminate all the affected EEs.*/
		head = trk_info->aff_ent_list;
		while(head){
			if (PLMS_EE_ENTITY == head->plm_entity->entity_type){
				ret_err = plms_ee_term(head->plm_entity,
				FALSE,0);
				if (NCSCC_RC_SUCCESS != ret_err){
					LOG_ER("EE term failed. Ent: %s",
					head->plm_entity->dn_name_str);
				}
			}
			head = head->next;
		}
	}

	if (PLMS_EE_ENTITY == ent->entity_type){
		/* Lock the root EE.*/
		ret_err = plms_ee_lock(ent,TRUE,1/*mngt_cbk*/);
		if(NCSCC_RC_SUCCESS != ret_err){
			LOG_ER("EE lock Failed. Ent: %s",ent->dn_name_str);
		}
	}
	
	/* Populate trk_info. */
	trk_info->imm_adm_opr_id = adm_op.operation_id; 
	trk_info->inv_id = adm_op.inv_id;
	trk_info->change_step = SA_PLM_CHANGE_COMPLETED;
	trk_info->track_cause = SA_PLM_CAUSE_LOCK;
	trk_info->root_correlation_id = SA_NTF_IDENTIFIER_UNUSED;
	trk_info->grp_op = SA_PLM_GROUP_MEMBER_READINESS_CHANGE; 
	trk_info->root_entity = ent;
	trk_info->track_count = 0;
	
	/* Call completed cbk.*/
	plms_cbk_call(trk_info,1);

	plms_aff_ent_exp_rdness_status_clear(trk_info->aff_ent_list);
	plms_ent_exp_rdness_status_clear(ent);
	
	if ( (PLMS_EE_ENTITY == ent->entity_type) && 
		/*plms_rdness_flag_is_set(ent,SA_PLM_RF_MANAGEMENT_LOST)){*/
		(NCSCC_RC_SUCCESS != ret_err)){

		ret_err = saImmOiAdminOperationResult( cb->oi_hdl,
			trk_info->inv_id, SA_AIS_ERR_DEPLOYMENT);
		
		if (NCSCC_RC_SUCCESS != ret_err){
			LOG_ER("Sending admin response to IMM failed. \
					IMM Ret code: %d",ret_err);
		}else{
			TRACE("Sending admin response to IMM \
							successful.");
		}
		
	}else {
		/* Return OK to IMM. */
		ret_err = saImmOiAdminOperationResult(cb->oi_hdl ,
					trk_info->inv_id, SA_AIS_OK);
		
		if (NCSCC_RC_SUCCESS != ret_err){
			LOG_ER("Sending admin response to IMM failed. \
					IMM Ret code: %d",ret_err);
		}else{
			TRACE("Sending admin response to IMM \
							successful.");
		}
	}
	TRACE_LEAVE2("ret_err: %d",ret_err);
	return ret_err;
}
/******************************************************************************
@brief		: Common function to process unlock to shutdown operation for 
		both EE and HE.
@param[in]	: evt - PLMS evt structure holding IMM admin operation info      
@return		: NCSCC_RC_SUCCESS/SA_AIS_OK(1) - For success.
		  NCSCC_RC_FAILURE(0) - For failure.
		  others(SaAisErrorT) - For failure.
******************************************************************************/
static SaUint32T plms_ent_unlock_to_shutdown(PLMS_EVT *evt)
{
	SaUint32T ret_err = NCSCC_RC_SUCCESS;
	PLMS_CB *cb = plms_cb;
	PLMS_ENTITY *ent;
	PLMS_GROUP_ENTITY *aff_ent_list=NULL,*head,*to_be_rmv = NULL;
	PLMS_TRACK_INFO *trk_info;
	PLMS_ENTITY_GROUP_INFO_LIST *log_head_grp;
	SaInt8T tmp[SA_MAX_NAME_LENGTH +1];

	plms_get_str_from_dn_name(&(evt->req_evt.admin_op.dn_name),tmp);
	TRACE_ENTER2("Entity: %s. Unlock to shutdown",tmp);

	ent = (PLMS_ENTITY *)ncs_patricia_tree_get(&(cb->entity_info),
			   (SaUint8T *)&(evt->req_evt.admin_op.dn_name));
	if (NULL == ent) {
		LOG_ER("unlock_to_shutdown can not be performed as the ent is \
		found in entity_info tree for the dn_name %s",tmp);
		
		ret_err = saImmOiAdminOperationResult(cb->oi_hdl,
			evt->req_evt.admin_op.inv_id,SA_AIS_ERR_NO_RESOURCES);
		
		if (NCSCC_RC_SUCCESS != ret_err){
			LOG_ER("Sending admin response to IMM failed. \
						IMM Ret code: %d",ret_err);
		}else{
			TRACE("Sending admin response to IMM successful.");
		}
		
		TRACE_LEAVE2("ret_err: %d",ret_err);
		return ret_err;
	}
	
	/* If the readiness state of the HE/EE is OOS, 
	check the feasibility.*/
	if ( ((PLMS_HE_ENTITY == ent->entity_type)&&
			(SA_PLM_READINESS_OUT_OF_SERVICE == 
			ent->entity.he_entity.saPlmHEReadinessState)) || 
		((PLMS_EE_ENTITY == ent->entity_type) &&
		(SA_PLM_READINESS_OUT_OF_SERVICE ==
		ent->entity.ee_entity.saPlmEEReadinessState))) {

		TRACE("The entity %s is already in OOS",tmp);
		/* If I am affected, then return.*/
		if ( ent->am_i_aff_ent){

			LOG_ER("The entity %s is in another operational\
			context and hence this admin shutdown can not be\
			performed.",tmp);

			ret_err = saImmOiAdminOperationResult(cb->oi_hdl,
			evt->req_evt.admin_op.inv_id,SA_AIS_ERR_TRY_AGAIN);
			if (NCSCC_RC_SUCCESS != ret_err){
				LOG_ER("Sending admin response to IMM failed. \
						IMM Ret code: %d",ret_err);
			}else{
				TRACE("Sending admin response to IMM \
				successful.");
			}
		
			TRACE_LEAVE2("ret_err: %d",ret_err);
			return ret_err;    
		}else{
			plms_admin_state_set(ent, SA_PLM_HE_ADMIN_LOCKED,NULL,
						SA_NTF_MANAGEMENT_OPERATION,
						SA_PLM_NTFID_STATE_CHANGE_ROOT);
			/* Respond to IMM.*/
			ret_err = saImmOiAdminOperationResult(cb->oi_hdl,
					evt->req_evt.admin_op.inv_id,SA_AIS_OK);
			if (NCSCC_RC_SUCCESS != ret_err){
				LOG_ER("Sending admin response to IMM failed. \
						IMM Ret code: %d",ret_err);
			}else{
				TRACE("Sending admin response to IMM \
				successful.");
			}

			TRACE_LEAVE2("ret_err: %d",ret_err);
			return ret_err;
		}
		
	}else {
		/* Get all the affected entities.*/ 
		plms_affected_ent_list_get(ent,&aff_ent_list,0);

		if (plms_anc_chld_dep_adm_flag_is_set(ent,aff_ent_list)) {

			LOG_ER(" Admin shutdown of the ent %s can not be \
			performed as one of the affected entity is in \
			another operational context.",ent->dn_name_str);
			
			ret_err = saImmOiAdminOperationResult(cb->oi_hdl,
			evt->req_evt.admin_op.inv_id,SA_AIS_ERR_TRY_AGAIN);
			
			if (NCSCC_RC_SUCCESS != ret_err){
				LOG_ER("Sending admin response to IMM failed. \
						IMM Ret code: %d",ret_err);
			}else{
				TRACE("Sending admin response to IMM \
				successful.");
			}

			plms_ent_list_free(aff_ent_list);
			aff_ent_list = NULL;
			
			TRACE_LEAVE2("ret_err: %d",ret_err);
			return ret_err;    
		}
	}
	
	/* Admin operation started. */
	ent->adm_op_in_progress = SA_PLM_CAUSE_SHUTDOWN;
	ent->am_i_aff_ent = TRUE;
	plms_aff_ent_flag_mark_unmark(aff_ent_list,TRUE);
	
	trk_info =(PLMS_TRACK_INFO *)calloc(1,sizeof(PLMS_TRACK_INFO));
	if (NULL == trk_info){

		LOG_CR("unlock_to_shutdown, calloc for track info\
		failed. Error: %s",strerror(errno));
		
		ret_err = saImmOiAdminOperationResult(cb->oi_hdl,
			evt->req_evt.admin_op.inv_id,SA_AIS_ERR_NO_RESOURCES);
		
		if (NCSCC_RC_SUCCESS != ret_err){
			LOG_ER("Sending admin response to IMM failed. \
						IMM Ret code: %d",ret_err);
		}else{
			TRACE("Sending admin response to IMM successful.");
		}
		plms_ent_list_free(aff_ent_list);
		assert(0);
	}
	
	ent->trk_info = trk_info;
	trk_info->group_info_list = NULL;
	/* Add the groups, root entity(ent) belong to.*/
	plms_ent_grp_list_add(ent,&(trk_info->group_info_list));
	
	/* Find out all the groups, all affected entities belong to and add 
	the groups to trk_info->group_info_list.*/ 
	plms_ent_list_grp_list_add(aff_ent_list,
				&(trk_info->group_info_list));

	TRACE("Affected groups for ent %s: ",ent->dn_name_str);
	log_head_grp = trk_info->group_info_list;
	while(log_head_grp){
		TRACE("%llu,",log_head_grp->ent_grp_inf->entity_grp_hdl);
		log_head_grp = log_head_grp->next;
	}
	
	/*Mark the admin state of the HE/EE to shutting-down and readiness 
	state to stopping.*/
	plms_admin_state_set(ent,SA_PLM_HE_ADMIN_SHUTTING_DOWN,NULL,
				SA_NTF_MANAGEMENT_OPERATION,
				SA_PLM_NTFID_STATE_CHANGE_ROOT);

	plms_readiness_state_set(ent,
			SA_PLM_READINESS_STOPPING,NULL,
			SA_NTF_MANAGEMENT_OPERATION,
			SA_PLM_NTFID_STATE_CHANGE_ROOT);
	
	/* Readiness state of all the affected entities to stopping.*/
	head = aff_ent_list;
	while (head){
		/* Spec sec: 3.1.3.2.4 => OOS to stopping is not valid
		transition.*/
		if (!plms_is_rdness_state_set(head->plm_entity,
		SA_PLM_READINESS_OUT_OF_SERVICE)){
			plms_readiness_state_set(head->plm_entity,
				SA_PLM_READINESS_STOPPING,
				ent,SA_NTF_MANAGEMENT_OPERATION,
				SA_PLM_NTFID_STATE_CHANGE_DEP);
		}else{/* This entity is not part of affected entity anymore.*/
			plms_ent_to_ent_list_add(head->plm_entity,&to_be_rmv);	
		}
		plms_readiness_flag_mark_unmark(head->plm_entity,
				SA_PLM_RF_DEPENDENCY,TRUE,ent,
				SA_NTF_MANAGEMENT_OPERATION,
				SA_PLM_NTFID_STATE_CHANGE_DEP);
		head = head->next;
	 }

	 /* Send completed callback for insvc to stopping change.*/
	 /****************************************************************/
	 trk_info->aff_ent_list = aff_ent_list;
	 trk_info->imm_adm_opr_id = evt->req_evt.admin_op.operation_id;
	 trk_info->inv_id = evt->req_evt.admin_op.inv_id;
	 trk_info->change_step = SA_PLM_CHANGE_COMPLETED;
	 trk_info->track_cause = SA_PLM_CAUSE_SHUTDOWN;
	 trk_info->root_correlation_id = SA_NTF_IDENTIFIER_UNUSED;
	 trk_info->grp_op = SA_PLM_GROUP_MEMBER_READINESS_CHANGE;
	 trk_info->root_entity = ent;
	 plms_cbk_call(trk_info,1);
	/*******************************************************************/
	/* Clean up the redundant entities and groups.*/
	if (NULL != to_be_rmv){
		plms_aff_ent_flag_mark_unmark(to_be_rmv,FALSE);
		plms_aff_ent_exp_rdness_status_clear(to_be_rmv);
		head = to_be_rmv;
		while (head){
			plms_ent_from_ent_list_rem(head->plm_entity,&aff_ent_list);
			head = head->next;
		}
		
		plms_ent_grp_list_free(trk_info->group_info_list);
		trk_info->group_info_list = NULL;	
		plms_ent_grp_list_add(ent,&(trk_info->group_info_list));
		plms_ent_list_grp_list_add(aff_ent_list,&(trk_info->group_info_list));

		TRACE("Affected groups for ent %s: ",ent->dn_name_str);
		log_head_grp = trk_info->group_info_list;
		while(log_head_grp){
			TRACE("%llu,",log_head_grp->ent_grp_inf->entity_grp_hdl);
			log_head_grp = log_head_grp->next;
		}

		trk_info->aff_ent_list = aff_ent_list;
		plms_ent_list_free(to_be_rmv);
	}
	
	/* Mark the expected readiness state of the root entity.*/
	ent->exp_readiness_status.readinessState = 
				SA_PLM_READINESS_OUT_OF_SERVICE; 
	
	if (PLMS_HE_ENTITY == ent->entity_type){			
		ent->exp_readiness_status.readinessFlags = 
		ent->entity.he_entity.saPlmHEReadinessFlags;
	}else{
		ent->exp_readiness_status.readinessFlags = 
		ent->entity.ee_entity.saPlmEEReadinessFlags;
	}

	/* Expected readiness state to OOS and set dependency flag.*/
	plms_aff_ent_exp_rdness_state_mark(aff_ent_list,
		SA_PLM_READINESS_OUT_OF_SERVICE,SA_PLM_RF_DEPENDENCY);

	/* Populate trk_info. */
	trk_info->change_step = SA_PLM_CHANGE_START;
	/* Ask the agent to call start cbk.*/
	plms_cbk_call(trk_info,1);
	
	/* No start subscriber, call completed cbk right away.*/
	if (0 == trk_info->track_count){
		plms_shutdown_completed_cbk_call(ent,trk_info);	
	}else{
		plms_peer_async_send(ent,SA_PLM_CHANGE_START,
		SA_PLM_CAUSE_SHUTDOWN);
	}

	TRACE_LEAVE2("ret_err: %d",ret_err);
	return ret_err;
}
/******************************************************************************
@brief		: Common function to process shutdown to lock operation for 
		both EE and HE.
@param[in]	: evt - PLMS evt structure holding IMM admin operation info      
@return		: NCSCC_RC_SUCCESS/SA_AIS_OK(1) - For success.
		  NCSCC_RC_FAILURE(0) - For failure.
		  others(SaAisErrorT) - For failure.
******************************************************************************/
static SaUint32T plms_ent_shutdown_to_lock( PLMS_EVT *evt)
{
	SaUint32T ret_err = NCSCC_RC_FAILURE;
	PLMS_CB *cb = plms_cb;
	PLMS_ENTITY *ent;
	PLMS_TRACK_INFO *trk_info;
	PLMS_ENTITY_GROUP_INFO_LIST *grp_list_head;
	SaInt8T tmp[SA_MAX_NAME_LENGTH +1];

	plms_get_str_from_dn_name(&(evt->req_evt.admin_op.dn_name),tmp);
	TRACE_ENTER2("Entity: %s. shutdown to lock",tmp);

	ent = (PLMS_ENTITY *)ncs_patricia_tree_get(&(cb->entity_info),
			   (SaUint8T *)&(evt->req_evt.admin_op.dn_name));
	if (NULL == ent) {
		LOG_ER("shutdown_to_lock can not be performed as the ent is \
		found in entity_info tree for the dn_name %s",tmp);
		
		ret_err = saImmOiAdminOperationResult(cb->oi_hdl,
			evt->req_evt.admin_op.inv_id,SA_AIS_ERR_NO_RESOURCES);

		if (NCSCC_RC_SUCCESS != ret_err){
			LOG_ER("Sending admin response to IMM failed. \
						IMM Ret code: %d",ret_err);
		}else{
			TRACE("Sending admin response to IMM successful.");
		}
		
		TRACE_LEAVE2("ret_err: %d",ret_err);
		return ret_err;
	}
	/*
	 * If I am in admin context(shutdown), then only forced lock is 
	 * allowed.
	*/
	trk_info = ent->trk_info;
	if (ent->adm_op_in_progress && (NULL != trk_info) && 
		(SA_PLM_CAUSE_SHUTDOWN == trk_info->track_cause) &&
		(0 == (strcmp(evt->req_evt.admin_op.option,
		SA_PLM_ADMIN_LOCK_OPTION_FORCED)))){

		TRACE("Admin shutdown is in progress and forced lock \
		is issued. Ent: %s",ent->dn_name_str);

		/* Clean up the shutdown context.*/
		grp_list_head = trk_info->group_info_list;
		while (grp_list_head){
			/* Remove the inv_to_cbk nodes from the 
			grp->invocation list, if pointing to this trk_info.*/
			plms_inv_to_cbk_in_grp_trk_rmv(grp_list_head->
							ent_grp_inf,trk_info);
			grp_list_head = grp_list_head->next;
		}

		/* Response to IMM for the shutdown operation.
		Return TIMEOUT.*/
		ret_err = saImmOiAdminOperationResult(cb->oi_hdl,
			trk_info->inv_id,SA_AIS_ERR_TIMEOUT);

		if (NCSCC_RC_SUCCESS != ret_err){
			LOG_ER("Sending admin response for shutdown to IMM\
			failed. IMM Ret code: %d",ret_err);
		}else{
			TRACE("Sending admin response for shutdown to IMM \
			successful.");
		}
		
		/* Response to IMM for the shutdown operation.
		Return TIMEOUT.*/
		ret_err = saImmOiAdminOperationResult(cb->oi_hdl,
			trk_info->inv_id,SA_AIS_ERR_TIMEOUT);

		if (NCSCC_RC_SUCCESS != ret_err){
			LOG_ER("Sending admin response to IMM failed. \
						IMM Ret code: %d",ret_err);
		}else{
			TRACE("Sending admin response to IMM successful.");
		}
		
#if 0 /* TODO: If new aff ent list is calculated, then merge the new list
		with the old one.*/
		
		/* Do not free the track info. But free the affected ent and 
		grp list.*/
		plms_ent_list_free(trk_info->aff_ent_list);
		plms_ent_grp_list_free(trk_info->group_info_list);
#endif		
	}else {
		/* Oh... Shutdown is not in progress or even if shutdown
		 * is in progress, lock option is TRY/DEFAULT. */
		if (ent->adm_op_in_progress){

			LOG_ER("Operation can not be performed. cur_track_cause: %d, req_lock_option: %s",
			ent->adm_op_in_progress,evt->req_evt.admin_op.option);
		
			ret_err = saImmOiAdminOperationResult(cb->oi_hdl,
			evt->req_evt.admin_op.inv_id,SA_AIS_ERR_BAD_OPERATION);
		
			if (NCSCC_RC_SUCCESS != ret_err){
				LOG_ER("Sending admin response to IMM failed. \
				IMM Ret code: %d",ret_err);
			}else{
				TRACE("Sending admin response to IMM \
				successful.");
			}
		
		}else {/* I am not in admin context still my admin state is
			shutting down. Oh yeah... Remove this code.	
			*/

			LOG_ER("Ent %s is not in admin operation context \
			but still its admin state is shutting down==>Parent \
			or deps in shutting down state. Reject forced lock.",
			ent->dn_name_str);

			ret_err = saImmOiAdminOperationResult(cb->oi_hdl,
			evt->req_evt.admin_op.inv_id,SA_AIS_ERR_NO_OP);
		
			if (NCSCC_RC_SUCCESS != ret_err){
				LOG_ER("Sending admin response to IMM failed. \
				IMM Ret code: %d",ret_err);
			}else{
				TRACE("Sending admin response to IMM \
				successful.");
			}
		}

		TRACE_LEAVE2("ret_err: %d",ret_err);
		return NCSCC_RC_FAILURE;
	}
#if 0	
	/* Get the fresh set of aff entities. */
	plms_affected_ent_list_get(ent, &(trk_info->aff_ent_list),0);
#endif
	ret_err = plms_forced_lock_cbk(ent,trk_info,evt->req_evt.admin_op,0,1);
	if (NCSCC_RC_SUCCESS != ret_err){
	}
	/* I am done with admin operation.*/
	ent->adm_op_in_progress = FALSE;
	ent->am_i_aff_ent = FALSE;
	plms_aff_ent_flag_mark_unmark(trk_info->aff_ent_list,FALSE);
	
	/* Free track info.*/
	plms_trk_info_free(trk_info);

	TRACE_LEAVE2("ret_err: %d",ret_err);
	return ret_err;
}
/******************************************************************************
@brief		: Common function to process shutdown to unlock operation for 
		both EE and HE.
@param[in]	: evt - PLMS evt structure holding IMM admin operation info      
@return		: NCSCC_RC_SUCCESS/SA_AIS_OK(1) - For success.
		  NCSCC_RC_FAILURE(0) - For failure.
		  others(SaAisErrorT) - For failure.
******************************************************************************/
static SaUint32T plms_ent_shutdown_to_unlock( PLMS_EVT *evt)
{
	SaUint32T ret_err = NCSCC_RC_FAILURE;
	PLMS_CB *cb = plms_cb;
	PLMS_ENTITY *ent;
	PLMS_GROUP_ENTITY *aff_ent_list=NULL,*head;
	PLMS_TRACK_INFO *trk_info;
	PLMS_ENTITY_GROUP_INFO_LIST *grp_list_head;
	SaInt8T tmp[SA_MAX_NAME_LENGTH +1];

	plms_get_str_from_dn_name(&(evt->req_evt.admin_op.dn_name),tmp);
	TRACE_ENTER2("Entity: %s. shutdown to unlock",tmp);

	ent = (PLMS_ENTITY *)ncs_patricia_tree_get(&(cb->entity_info),
			   (SaUint8T *)&(evt->req_evt.admin_op.dn_name));
	if (NULL == ent) {
		LOG_ER("shutdown_to_unlock can not be performed as the ent is \
		found in entity_info tree for the dn_name %s",tmp);
		
		ret_err = saImmOiAdminOperationResult(cb->oi_hdl,
			evt->req_evt.admin_op.inv_id,SA_AIS_ERR_NO_RESOURCES);  
		
		if (NCSCC_RC_SUCCESS != ret_err){
			LOG_ER("Sending admin response to IMM failed. \
						IMM Ret code: %d",ret_err);
		}else{
			TRACE("Sending admin response to IMM successful.");
		}
		
		TRACE_LEAVE2("ret_err: %d",ret_err);
		return ret_err;
	}
	/*
	 * If I am in admin context(shutdown), then only allow unlock. 
	*/
	trk_info = ent->trk_info;
	if (ent->adm_op_in_progress && (NULL != trk_info) && 
		(SA_PLM_CAUSE_SHUTDOWN == trk_info->track_cause) &&
		(SA_PLM_ADMIN_UNLOCK == evt->req_evt.admin_op.operation_id)){

		TRACE("Admin shutdown is in progress and unlock \
		is issued. Ent: %s",ent->dn_name_str);
		
		grp_list_head = trk_info->group_info_list;
		while (grp_list_head){
			/* Remove the inv_to_cbk nodes from the 
			grp->innvocation list, if pointing to this trk_info.*/
			plms_inv_to_cbk_in_grp_trk_rmv(grp_list_head->
							ent_grp_inf,trk_info);
			grp_list_head = grp_list_head->next;
		}
		
		/* Response to IMM for the shutdown operation.
		Return TIMEOUT.*/
		ret_err = saImmOiAdminOperationResult(cb->oi_hdl,
			trk_info->inv_id,SA_AIS_ERR_TIMEOUT);

		if (NCSCC_RC_SUCCESS != ret_err){
			LOG_ER("Sending admin response for shutdown to IMM \
			failed. IMM Ret code: %d",ret_err);
		}else{
			TRACE("Sending admin response(shutdown) to IMM \
			successful.");
		}
		
		aff_ent_list = trk_info->aff_ent_list;
		plms_aff_ent_exp_rdness_status_clear(aff_ent_list);
		plms_ent_grp_list_free(trk_info->group_info_list);
		
	}else {
		/* Oh... Shutdown is not in progress or even if shutdown
		 * is in progress, admin op is not unlock. Fool !!! This will
		 * never happen.
		 */
		if (ent->adm_op_in_progress){
			
			LOG_ER("Operation can not be performed. cur_track_change: %d, requested_op: %d",
			ent->adm_op_in_progress,evt->req_evt.admin_op.operation_id);
		
			ret_err = saImmOiAdminOperationResult(cb->oi_hdl,
			evt->req_evt.admin_op.inv_id,SA_AIS_ERR_BAD_OPERATION);
		
			if (NCSCC_RC_SUCCESS != ret_err){
				LOG_ER("Sending admin response to IMM failed. \
				IMM Ret code: %d",ret_err);
			}else{
				TRACE("Sending admin response to IMM \
				successful.");
			}
		}else {/* I am not in admin context still my admin state is
			shutting down. Oh yeah.. This code is not needed.   
			*/
			LOG_ER("Ent %s is not in admin operation context \
			but still its admin state is shutting down. Reject\
			unlock.", ent->dn_name_str);

			ret_err = saImmOiAdminOperationResult(cb->oi_hdl,
			evt->req_evt.admin_op.inv_id,SA_AIS_ERR_NO_OP);
		
			if (NCSCC_RC_SUCCESS != ret_err){
				LOG_ER("Sending admin response to IMM failed. \
				IMM Ret code: %d",ret_err);
			}else{
				TRACE("Sending admin response to IMM \
				successful.");
			}
		}

		TRACE_LEAVE2("ret_err: %d",ret_err);
		return ret_err;
	}
	
	trk_info->aff_ent_list = NULL;

	/* Move back the aff entities to unlock state.*/
	head = aff_ent_list;
	while(head){

		if ( ((PLMS_HE_ENTITY == head->plm_entity->entity_type) &&
		(SA_PLM_HE_ADMIN_SHUTTING_DOWN ==
		head->plm_entity->entity.he_entity.saPlmHEAdminState )) ||
		((PLMS_EE_ENTITY == head->plm_entity->entity_type) &&
		(SA_PLM_EE_ADMIN_SHUTTING_DOWN ==
		head->plm_entity->entity.ee_entity.saPlmEEAdminState ))){
			
			plms_admin_state_set(head->plm_entity,
			SA_PLM_HE_ADMIN_UNLOCKED,
			ent,SA_NTF_MANAGEMENT_OPERATION,
			SA_PLM_NTFID_STATE_CHANGE_DEP);
		}
		head = head->next;
	}
	/* No need to instantiate aff EEs as they are not yet uninstantiated.*/	
	ret_err = plms_ent_unlock(ent,trk_info,evt->req_evt.admin_op,
						0/* inst_ee */,0/*unlck_ee*/);
	if (NCSCC_RC_SUCCESS != ret_err){
	}
	
	/* I am done with the admin operation.*/
	ent->adm_op_in_progress = FALSE;
	ent->am_i_aff_ent = FALSE;
	plms_aff_ent_flag_mark_unmark(aff_ent_list,FALSE);
	plms_ent_list_free(aff_ent_list);
	aff_ent_list = NULL;
	
	plms_trk_info_free(trk_info);

	TRACE_LEAVE2("ret_err: %d",ret_err);
	return ret_err;
}
/******************************************************************************
@brief		: Common function to process lock to unlock operation for 
		both EE and HE.
@param[in]	: evt - PLMS evt structure holding IMM admin operation info      
@return		: NCSCC_RC_SUCCESS/SA_AIS_OK(1) - For success.
		  NCSCC_RC_FAILURE(0) - For failure.
		  others(SaAisErrorT) - For failure.
******************************************************************************/
static SaUint32T plms_ent_locked_to_unlock( PLMS_EVT *evt)
{
	SaUint32T ret_err = NCSCC_RC_FAILURE;
	PLMS_CB *cb = plms_cb;
	PLMS_ENTITY *ent;
	PLMS_GROUP_ENTITY *aff_ent_list=NULL;
	PLMS_TRACK_INFO trk_info;
	SaInt8T tmp[SA_MAX_NAME_LENGTH +1];

	plms_get_str_from_dn_name(&(evt->req_evt.admin_op.dn_name),tmp);
	TRACE_ENTER2("Entity: %s. locked to unlock",tmp);

	memset(&trk_info,0,sizeof(PLMS_TRACK_INFO));

	ent = (PLMS_ENTITY *)ncs_patricia_tree_get(&(cb->entity_info),
			   (SaUint8T *)&(evt->req_evt.admin_op.dn_name));
	if (NULL == ent) {
		LOG_ER("locked_to_unlock can not be performed as the ent is \
		found in entity_info tree for the dn_name %s",tmp);
		
		ret_err = saImmOiAdminOperationResult(cb->oi_hdl,
			evt->req_evt.admin_op.inv_id,SA_AIS_ERR_NO_RESOURCES);  
		
		if (NCSCC_RC_SUCCESS != ret_err){
			LOG_ER("Sending admin response to IMM failed. \
						IMM Ret code: %d",ret_err);
		}else{
			TRACE("Sending admin response to IMM successful.");
		}
		
		TRACE_LEAVE2("ret_err: %d",ret_err);
		return ret_err;
	}
	
	/* I should not be in mid of any operation.*/
	if (ent->am_i_aff_ent){
		
		LOG_ER("Unlock operation cannot be performed,\
		as the ent is in another operaional context.\
		Ent: %s",ent->dn_name_str);
		
		ret_err = saImmOiAdminOperationResult(cb->oi_hdl,
			evt->req_evt.admin_op.inv_id,SA_AIS_ERR_TRY_AGAIN);
		
		if (NCSCC_RC_SUCCESS != ret_err){
			LOG_ER("Sending admin response to IMM failed. \
						IMM Ret code: %d",ret_err);
		}else{
			TRACE("Sending admin response to IMM successful.");
		}
		
		TRACE_LEAVE2("ret_err: %d",ret_err);
		return ret_err;    
	}
	
	/* Even after unlocking the ent, it can not be made insvc.
	Hence nothing to do. Just mark the admin state to unlock.*/
	if(PLMS_HE_ENTITY == ent->entity_type){
		if ( plms_rdness_flag_is_set(ent,SA_PLM_RF_DEPENDENCY) ||
		( SA_PLM_OPERATIONAL_DISABLED == 
		ent->entity.he_entity.saPlmHEOperationalState) ||
		((SA_PLM_HE_PRESENCE_ACTIVE != 
		ent->entity.he_entity.saPlmHEPresenceState) && 
		(SA_PLM_HE_PRESENCE_DEACTIVATING != 
		 ent->entity.he_entity.saPlmHEPresenceState))){

			TRACE("Even if the entity is unlocked, still can not\
			go insvc. Ent: %s",ent->dn_name_str);
			plms_admin_state_set(ent, SA_PLM_HE_ADMIN_UNLOCKED,
					NULL,SA_NTF_MANAGEMENT_OPERATION,
					SA_PLM_NTFID_STATE_CHANGE_ROOT);
			/* TODO: Update IMM and send notification.*/			
			/* Nothing to do. Return to IMM.*/
			ret_err = saImmOiAdminOperationResult(cb->oi_hdl,
				evt->req_evt.admin_op.inv_id,SA_AIS_OK);
			if (NCSCC_RC_SUCCESS != ret_err){
				LOG_ER("Sending admin response to IMM failed. \
				IMM Ret code: %d",ret_err);
			}else{
				TRACE("Sending admin response to IMM \
				successful.");
			}

			TRACE_LEAVE2("ret_err: %d",ret_err);
			return ret_err;	
		}
	}else{
		if (plms_rdness_flag_is_set(ent,SA_PLM_RF_DEPENDENCY) ||
		( SA_PLM_OPERATIONAL_DISABLED == 
		ent->entity.ee_entity.saPlmEEOperationalState) ||
		((SA_PLM_EE_PRESENCE_INSTANTIATED != 
		ent->entity.ee_entity.saPlmEEPresenceState) && 
		(SA_PLM_EE_PRESENCE_TERMINATING != 
		 ent->entity.ee_entity.saPlmEEPresenceState))){
			
			TRACE("Even if the entity is unlocked, still can not\
			go insvc. Ent: %s",ent->dn_name_str);
			plms_admin_state_set(ent, SA_PLM_EE_ADMIN_UNLOCKED,NULL,
						SA_NTF_MANAGEMENT_OPERATION,
						SA_PLM_NTFID_STATE_CHANGE_ROOT);

			/* Nothing to do. Return to IMM.*/
			ret_err = saImmOiAdminOperationResult(cb->oi_hdl,
				evt->req_evt.admin_op.inv_id,SA_AIS_OK);
			if (NCSCC_RC_SUCCESS != ret_err){
				LOG_ER("Sending admin response to IMM failed. \
				IMM Ret code: %d",ret_err);
			}else{
				TRACE("Sending admin response to IMM \
				successful.");
			}

			TRACE_LEAVE2("ret_err: %d",ret_err);
			return ret_err;	
		}
	}
	
	/* Get all the affected entities.*/ 
	plms_affected_ent_list_get(ent,&aff_ent_list,1);

	if (plms_anc_chld_dep_adm_flag_is_set(ent,aff_ent_list)) {

		LOG_ER("Locked_to_unlock operation can not be performed\
		as any of the affected entity is in some other operational\
		context. Ent: %s",ent->dn_name_str);
		
		ret_err = saImmOiAdminOperationResult(cb->oi_hdl,
			evt->req_evt.admin_op.inv_id,SA_AIS_ERR_TRY_AGAIN);
		if (NCSCC_RC_SUCCESS != ret_err){
			LOG_ER("Sending admin response to IMM failed. \
					IMM Ret code: %d",ret_err);
		}else{
			TRACE("Sending admin response to IMM successful.");
		}
		
		plms_ent_list_free(aff_ent_list);
		aff_ent_list = NULL;

		TRACE_LEAVE2("ret_err: %d",ret_err);
		return ret_err;    
	}
	plms_ent_list_free(aff_ent_list);
	aff_ent_list = NULL;
	
	ret_err = plms_ent_unlock(ent,&trk_info,evt->req_evt.admin_op,
						1/* inst_ee*/,1/*unlck_ee*/);
	if (NCSCC_RC_SUCCESS != ret_err){
	}

	/* Free aff ent list anf aff grp list.*/
	plms_ent_list_free(trk_info.aff_ent_list);
	trk_info.aff_ent_list = NULL;
	plms_ent_grp_list_free(trk_info.group_info_list);
	trk_info.group_info_list = NULL;

	TRACE_LEAVE2("ret_err: %d",ret_err);
	return ret_err;
}

/******************************************************************************
@brief		: Unlock the entity. Take the entity to in service if other
		conditions are matched.
@param[in]	: ent - Entity to be unlocked.		
@param[in]	: trk_info - used for calling the callback.		
@param[in]	: adm_op - IMM admin operation related data.
@param[in]	: inst_ee - Instantiate EE if required(1/0).		
@param[in]	: unlck_ee - Unlock the EE(1/0).		
@return		: NCSCC_RC_SUCCESS/SA_AIS_OK(1) - For success.
		  NCSCC_RC_FAILURE(0) - For failure.
		  others(SaAisErrorT) - For failure.
******************************************************************************/
static SaUint32T plms_ent_unlock( PLMS_ENTITY *ent,PLMS_TRACK_INFO *trk_info,
		PLMS_IMM_ADMIN_OP adm_op,SaUint32T inst_ee, SaUint32T unlck_ee)
{
	SaUint32T ret_err = NCSCC_RC_SUCCESS,unlck_err = NCSCC_RC_SUCCESS;
	PLMS_CB *cb = plms_cb;
	PLMS_GROUP_ENTITY *act_aff_ent = NULL,*head,*log_head;
	PLMS_ENTITY_GROUP_INFO_LIST *log_head_grp = NULL;
	SaUint8T is_flag_aff = 0;
	
	TRACE_ENTER2("Entity: %s",ent->dn_name_str);

	/* Update the admin state.*/
	/* Even if unlock fails and management lost flag is set,
	I should go ahead complete the admin operation.
	Spec: Page-42/43 */
	plms_admin_state_set(ent,SA_PLM_HE_ADMIN_UNLOCKED,NULL,
				SA_NTF_MANAGEMENT_OPERATION,
				SA_PLM_NTFID_STATE_CHANGE_ROOT);

	if ((PLMS_EE_ENTITY == ent->entity_type) && (unlck_ee)){
		/* Unlock the EE.*/
		unlck_err = plms_ee_unlock(ent,TRUE,1/*mngt_cbk*/);	
		if (NCSCC_RC_SUCCESS != unlck_err){
			/* TODO: Should I return from here, sending failure to 
			IMM and calling management lost callback.*/
			LOG_ER("EE unlock operation failed. Ent: %s",
			ent->dn_name_str);
		}
	}
	
	/* Take the ent to insvc. */
	if (NCSCC_RC_SUCCESS == plms_move_ent_to_insvc(ent,&is_flag_aff)){
		/* Move the children to insvc and clear the dependency flag.*/
		plms_move_chld_ent_to_insvc(ent->leftmost_child,&act_aff_ent,
		1,1);

		/* Move the dependent entities to InSvc and clear the
		dependency flag.*/
		plms_move_dep_ent_to_insvc(ent->rev_dep_list,&act_aff_ent,1);
		
	}else if(is_flag_aff){
		/* Nothing to do.*/
	}else{
		/* No readiness status change of the root. But check if unlock
		operation failed, then have to call management lost callback*/
		if ((NCSCC_RC_SUCCESS != unlck_err) && 
		(PLMS_EE_ENTITY == ent->entity_type) && (unlck_ee)){
			plms_mngt_lost_clear_cbk_call(ent,1/*mark*/);
		}
		/* Respnd to IMM.*/
		ret_err = saImmOiAdminOperationResult(cb->oi_hdl,
		adm_op.inv_id,SA_AIS_ERR_DEPLOYMENT);

		if (NCSCC_RC_SUCCESS != ret_err){
			LOG_ER("Sending admin response to IMM failed. \
						IMM Ret code: %d",ret_err);
		}else{
			TRACE("Sending admin response to IMM successful.");
		}
		
		TRACE_LEAVE2("ret_err: %d",ret_err);
		return ret_err;
	}
	
	trk_info->group_info_list = NULL;
	trk_info->aff_ent_list = act_aff_ent;
	
	TRACE("Act affected(to be insvc) entities for ent %s: ", 
	ent->dn_name_str);

	log_head = act_aff_ent;
	while(log_head){
		TRACE("%s,",log_head->plm_entity->dn_name_str);
		log_head = log_head->next;
	}

	/* Add the groups, root entity(ent) belong to.*/
	plms_ent_grp_list_add(ent,&(trk_info->group_info_list));

	/* Find out all the groups, all affected entities belong to and add 
	the groups to trk_info->group_info_list.*/ 
	plms_ent_list_grp_list_add(trk_info->aff_ent_list,
				&(trk_info->group_info_list));

	TRACE("Affected groups for ent %s: ",ent->dn_name_str);
	log_head_grp = trk_info->group_info_list;
	while(log_head_grp){
		TRACE("%llu,",log_head_grp->ent_grp_inf->entity_grp_hdl);
		log_head_grp = log_head_grp->next;
	}
	
	/* Overwrite the expected readiness state and flag of the 
	root entity.*/
	plms_ent_exp_rdness_state_ow(ent);
	
	/* Overwrite the expected readiness state and flag of 
	all affceted entities.*/
	plms_aff_ent_exp_rdness_state_ow(trk_info->aff_ent_list);
	
	/* Instantiate all the affected EEs.*/
	if (inst_ee){
		head = trk_info->aff_ent_list;
		while(head){

			if ((PLMS_EE_ENTITY == head->plm_entity->entity_type) 
			&& (!plms_rdness_flag_is_set(
			head->plm_entity,SA_PLM_RF_DEPENDENCY))){
				ret_err = plms_ee_instantiate(
				head->plm_entity,FALSE,TRUE);
				if (NCSCC_RC_SUCCESS != ret_err){
					LOG_ER("EE %s instantiation failed.",
					head->plm_entity->dn_name_str);
				}
			}

			head = head->next;
		}
	}
	
	
	/* Fill the track info. */
	trk_info->imm_adm_opr_id = adm_op.operation_id; 
	trk_info->inv_id = adm_op.inv_id;
	trk_info->change_step = SA_PLM_CHANGE_COMPLETED;
	trk_info->track_cause = SA_PLM_CAUSE_UNLOCKED;
	trk_info->root_correlation_id = SA_NTF_IDENTIFIER_UNUSED;
	trk_info->grp_op = SA_PLM_GROUP_MEMBER_READINESS_CHANGE; 
	trk_info->root_entity = ent;

	/* Ask the agent to call completed cbk.*/
	plms_cbk_call(trk_info,1);

	plms_ent_exp_rdness_status_clear(ent);
	plms_aff_ent_exp_rdness_status_clear(trk_info->aff_ent_list);
	/* Respnd to IMM.*/
	if (NCSCC_RC_SUCCESS == unlck_err){
		ret_err = saImmOiAdminOperationResult(cb->oi_hdl,
		adm_op.inv_id,SA_AIS_OK);
	}else{
		ret_err = saImmOiAdminOperationResult(cb->oi_hdl,
		adm_op.inv_id,SA_AIS_ERR_DEPLOYMENT);
	}

	if (NCSCC_RC_SUCCESS != ret_err){
		LOG_ER("Sending admin response to IMM failed. \
						IMM Ret code: %d",ret_err);
	}else{
		TRACE("Sending admin response to IMM successful.");
	}

	TRACE_LEAVE2("ret_err: %d",ret_err);
	return ret_err;
}

/******************************************************************************
@brief		: Takes care of packing the entities depending upon the track
		flag and send the callback info to PLMA while PLMA taks care of
		calling readiness callback to the clients.
@param[in]	: trk_info - Carries the required info to be packed as the part
		of callback.
@param[in]	: add_root - Add root as part of affected entities(1/0).
@return		: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
******************************************************************************/
SaUint32T plms_cbk_call(PLMS_TRACK_INFO *trk_info,SaUint8T add_root)
{
	PLMS_ENTITY_GROUP_INFO_LIST *grp_list;
	PLMS_CB *cb = plms_cb;
	PLMS_INVOCATION_TO_TRACK_INFO *inv_to_trk_info;	
	static SaInvocationT act_inv_id;
	SaInvocationT inv_id;
	PLMS_EVT agent_evt;
	SaUint32T ret_err = NCSCC_RC_FAILURE;
	SaUint32T cnt,is_validate;

	grp_list = trk_info->group_info_list;

	while (grp_list){

		if (!grp_list->ent_grp_inf->track_flags && (SA_PLM_CAUSE_STATUS_INFO != trk_info->track_cause)){
			grp_list = grp_list->next;
			continue;
		}
		
		is_validate = 0;
		/* Application is interested in validate or start cbk. 
		Spec:Page-87,88 */

		if ( ((SA_PLM_CHANGE_VALIDATE == trk_info->change_step) && 
			(m_PLM_IS_SA_TRACK_VALIDATE_STEP_SET(
			grp_list->ent_grp_inf->track_flags))) || 
			((SA_PLM_CHANGE_START == trk_info->change_step) && 
			(m_PLM_IS_SA_TRACK_START_STEP_SET(
			grp_list->ent_grp_inf->track_flags)))){

			is_validate = 1;
			act_inv_id++;
			inv_id = act_inv_id;
		}else if (SA_PLM_CHANGE_COMPLETED == trk_info->change_step ){
			inv_id = 0; 
		}else if (SA_PLM_CHANGE_ABORTED == trk_info->change_step){
			inv_id = 0;
		}else{
			grp_list = grp_list->next;
			continue;
		}
		
		/* Populate the agent_evt to be sent to PLMA. */
		memset(&agent_evt,0,sizeof(PLMS_EVT));
		agent_evt.req_res = PLMS_REQ;
		agent_evt.req_evt.req_type = PLMS_AGENT_TRACK_CBK_EVT;
		
		agent_evt.req_evt.agent_track.evt_type =
		PLMS_AGENT_TRACK_CBK_EVT;
		
		agent_evt.req_evt.agent_track.grp_handle = 
		grp_list->ent_grp_inf->entity_grp_hdl;
		
		agent_evt.req_evt.agent_track.track_cbk.track_cookie = 
		grp_list->ent_grp_inf->track_cookie;
		
		agent_evt.req_evt.agent_track.track_cbk.invocation_id = 
		inv_id;
		
		agent_evt.req_evt.agent_track.track_cbk.track_cause = 
		trk_info->track_cause;
		if ( ( NULL != trk_info->root_entity ) )
			agent_evt.req_evt.agent_track.track_cbk.
			root_cause_entity = trk_info->root_entity->dn_name; 
		
		agent_evt.req_evt.agent_track.track_cbk.
		root_correlation_id = trk_info->root_correlation_id; 
		
		agent_evt.req_evt.agent_track.track_cbk.change_step = 
		trk_info->change_step;
		
		agent_evt.req_evt.agent_track.track_cbk.error = 
		SA_AIS_OK;

		/* Add root as part of aff ent. */
		if ((NULL != trk_info->root_entity) && add_root){
			plms_ent_to_ent_list_add(trk_info->root_entity,
			&trk_info->aff_ent_list);
		}
		/* Find no of affected entities.*/
		cnt = plms_no_aff_ent_in_grp_get(grp_list->ent_grp_inf,
					trk_info->aff_ent_list);
		if (0 == cnt){
			LOG_ER("No of affected ent for the group %llu is\
			zero.",grp_list->ent_grp_inf->entity_grp_hdl);
			grp_list = grp_list->next;
			continue;
		}

		agent_evt.req_evt.agent_track.track_cbk.
		tracked_entities.numberOfEntities = cnt;

		agent_evt.req_evt.agent_track.track_cbk.
		tracked_entities.entities = 
		(SaPlmReadinessTrackedEntityT*) calloc
			(cnt,sizeof(SaPlmReadinessTrackedEntityT));

		if (NULL == agent_evt.req_evt.agent_track.track_cbk.
			tracked_entities.entities){
			LOG_CR("cbk_call, calloc failed. Error: %s",
			strerror(errno));
			assert(0);
		}
		/* Pack the affected entities. If the flag is changes only
		then this routine pack the root entity.*/	
		plms_grp_aff_ent_fill(agent_evt.req_evt.agent_track.
			track_cbk.tracked_entities.entities, 
			grp_list->ent_grp_inf,trk_info->aff_ent_list,
			trk_info->grp_op,trk_info->change_step);

		if ((NULL != trk_info->root_entity) && add_root){
			plms_ent_from_ent_list_rem(trk_info->root_entity,
			&trk_info->aff_ent_list);
		}
		/* Send the message to PLMA. */
		ret_err = plms_mds_normal_send(cb->mds_hdl,NCSMDS_SVC_ID_PLMS,
					&agent_evt,
					grp_list->ent_grp_inf->agent_mdest_id,
					NCSMDS_SVC_ID_PLMA);
		if (NCSCC_RC_SUCCESS != ret_err){
			LOG_ER("PLMS to PLMA cbk call MDS send failed for group\
			%llu,agent_dest %llu",
			grp_list->ent_grp_inf->entity_grp_hdl,
			grp_list->ent_grp_inf->agent_mdest_id);
		}else{
			/* Callback is sent to PLMA, Add the inv_id to the grp 
			and increment the trck_count.*/
			if(is_validate){
				inv_to_trk_info = 
				(PLMS_INVOCATION_TO_TRACK_INFO *)
				calloc(1,sizeof(PLMS_INVOCATION_TO_TRACK_INFO));
			
				if (NULL == inv_to_trk_info){
					LOG_CR("cbk_call, calloc failed. Error\
					: %s", strerror(errno));
					assert(0);
				}
				
				inv_to_trk_info->invocation = inv_id;
				inv_to_trk_info->track_info = trk_info;
				inv_to_trk_info->next = NULL;

				plms_inv_to_trk_grp_add(&(grp_list->
					ent_grp_inf->invocation_list),
					inv_to_trk_info);
				trk_info->track_count++;
			}

			TRACE("PLMS to PLMA cbk call MDS SENT for group\
			%llu,track_step: %d, track_cause: %d\
			, no of ent: %d",grp_list->ent_grp_inf->entity_grp_hdl,
			trk_info->change_step,trk_info->track_cause, cnt);
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

	TRACE("Track_count: %d, Track_cause: %d, Track_step: %d", 
	trk_info->track_count, trk_info->track_cause,trk_info->change_step);

	return ret_err;
}
/******************************************************************************
@brief		: Process the response from the client for the validate and 
		start callback.
@param[in]	: evt - PLMS_EVT representing client response.		
@return		: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
******************************************************************************/
SaUint32T plms_cbk_response_process(PLMS_EVT *evt)
{
	SaUint32T ret_err = NCSCC_RC_SUCCESS;
	PLMS_CB *cb = plms_cb;
	PLMS_ENTITY_GROUP_INFO *grp;
	PLMS_INVOCATION_TO_TRACK_INFO *inv_trk;
	PLMS_TRACK_INFO *trk_info;
	PLMS_ENTITY_GROUP_INFO_LIST *head;
	PLMS_AGENT_TRACK_OP *res = &evt->req_evt.agent_track;
	
	TRACE_ENTER2("Grp_hdl: %llu,resp: %d, inv_id: %llu",res->grp_handle,
	res->track_cbk_res.response,res->track_cbk_res.invocation_id);
	
	/* Get to the group */
	grp = (PLMS_ENTITY_GROUP_INFO *)ncs_patricia_tree_get(
		&(cb->entity_group_info), (SaUint8T *)&( res->grp_handle));
	if (NULL == grp) {
		LOG_ER("Response can not be processed as the group\
		corresponding to grp_handle %llu not found in plms\
		datebase.",res->grp_handle);

		TRACE_LEAVE2("ret_err: %d",NCSCC_RC_FAILURE);
		return NCSCC_RC_FAILURE;
	}
	/* If inv is not part of grp->invocation_list, then
		-- PLMS receives RESPONSE_REJECT from one of the 
		agent and sends the ABORT callback.
		-- But before getting ABORT callback, RESPONSE from
		this agent is reached to PLMS MBX and being processed.
		-- So just return from here.
	*/
	inv_trk = plms_inv_to_cbk_in_grp_find(grp->invocation_list,
				res->track_cbk_res.invocation_id); 
	if (NULL == inv_trk){
		LOG_ER("Invocation id mentioned in the resp, is not\
		found in the grp->inocation_list. inv_id: %llu",
		res->track_cbk_res.invocation_id);

		TRACE_LEAVE2("ret_err: %d",NCSCC_RC_FAILURE);
		return NCSCC_RC_FAILURE; 
	}
	switch(res->track_cbk_res.response){
		
	case SA_PLM_CALLBACK_RESPONSE_OK:
	/* If error, ignore even for validate. Spec: Page#85*/
	case SA_PLM_CALLBACK_RESPONSE_ERROR:
		
		TRACE("Response: %d",res->track_cbk_res.response);
		
		trk_info = inv_trk->track_info;
		/* Remove inv node from grp->invocation_list.*/
		plms_inv_to_cbk_in_grp_inv_rmv(grp,res->track_cbk_res.
							invocation_id);
		/* Decrement the track_count.*/
		trk_info->track_count--;
		/* I am expecting more response.*/
		if (trk_info->track_count){
			TRACE("Trackk count is not zero, I am expecting more\
			response. trk_cnt: %d",trk_info->track_count);
			
			TRACE_LEAVE2("ret_err: %d",NCSCC_RC_SUCCESS);
			return NCSCC_RC_SUCCESS;
		}
		/* Oh.. Got the all the responses. Send next cbk. */
		if (SA_PLM_CHANGE_VALIDATE == trk_info->change_step){
			ret_err = plms_cbk_validate_resp_ok_err_proc(trk_info); 
		
		}else if(SA_PLM_CHANGE_START == trk_info->change_step){
			ret_err = plms_cbk_start_resp_ok_err_proc(trk_info);
			
		}else{
			LOG_ER("Change step can not be anything otherthan\
			START/VALIDATE. change_step: %d",trk_info->change_step);
			TRACE_LEAVE2("ret_err: %d",NCSCC_RC_FAILURE);
			return NCSCC_RC_FAILURE; 
		}
		break;

	case SA_PLM_CALLBACK_RESPONSE_REJECTED:
		
		TRACE("Response REJECTED.");
		trk_info = inv_trk->track_info;
		/* Hold on. Application can not reject if the cbk is not 
		a validate cbk. */
		if ( SA_PLM_CHANGE_VALIDATE != trk_info->change_step){
			LOG_ER("Response can not be rejected for callback\
			other than VALIDATE.");

			TRACE_LEAVE2("ret_err: %d",NCSCC_RC_FAILURE);
			return NCSCC_RC_FAILURE;
		}
		/* Clean up the inv_to_cbk nodes belongs to this trk_info.*/
		head = trk_info->group_info_list;
		while (head){
			plms_inv_to_cbk_in_grp_trk_rmv(head->ent_grp_inf,
								trk_info);
			head=head->next;
		}
		/* Change the change_step to ABORT.*/
		trk_info->change_step = SA_PLM_CHANGE_ABORTED;
		/* Over-write the expected readiness state with the current. */
		plms_aff_ent_exp_rdness_state_ow(trk_info->aff_ent_list);
		plms_ent_exp_rdness_state_ow(trk_info->root_entity);

		trk_info->track_count = 0;
		/* Call the ABORT callback.*/
		plms_cbk_call(trk_info,1);
		
		plms_ent_exp_rdness_status_clear(trk_info->root_entity);
		plms_aff_ent_exp_rdness_status_clear(trk_info->aff_ent_list);
		/*If admin context is set, then send failure to IMM.  */
		if (trk_info->root_entity->adm_op_in_progress ==
							SA_PLM_CAUSE_LOCK){

			LOG_ER("LOCK rejected by grp : %llu, agent: %llu",
			res->grp_handle,grp->agent_mdest_id);
			
			ret_err = saImmOiAdminOperationResult(cb->oi_hdl,
				trk_info->inv_id,SA_AIS_ERR_FAILED_OPERATION);
			if (NCSCC_RC_SUCCESS != ret_err){
				LOG_ER("Sending admin response to IMM failed. \
						IMM Ret code: %d",ret_err);
			}else{
				TRACE("Sending admin response to IMM \
								successful.");
			}
			plms_peer_async_send(trk_info->root_entity,
			SA_PLM_CHANGE_ABORTED,SA_PLM_CAUSE_LOCK);		
			
			trk_info->root_entity->adm_op_in_progress = FALSE;
			trk_info->root_entity->am_i_aff_ent = FALSE;
			plms_aff_ent_flag_mark_unmark(
			trk_info->aff_ent_list,FALSE);
		}
		/* If the cause is DEACTIVATION, then move the HE to M4. 
		*/
		if (SA_PLM_CAUSE_HE_DEACTIVATION == 
				trk_info->root_entity->adm_op_in_progress) {
			
			LOG_ER("Deact rejected by grp : %llu, agent: %llu.\
			Take the ent to M4 state.",
			res->grp_handle,grp->agent_mdest_id);
			
			ret_err = plms_he_activate(trk_info->root_entity,
							FALSE,TRUE);
			if (NCSCC_RC_SUCCESS != ret_err){
				LOG_ER(" Move the ent to M4 failed.");
			}
			
			plms_peer_async_send(trk_info->root_entity,
			SA_PLM_CHANGE_ABORTED,SA_PLM_CAUSE_HE_DEACTIVATION);		
			trk_info->root_entity->adm_op_in_progress = FALSE;
			trk_info->root_entity->am_i_aff_ent = FALSE;
			plms_aff_ent_flag_mark_unmark(
			trk_info->aff_ent_list,FALSE);
			
		}
		/* Free trk_info and grp list maintained by trk_info. */
		plms_trk_info_free(trk_info);
		break;
		
		default:
			LOG_ER("Invalid response: %d",
			res->track_cbk_res.response);
			break;
	}

	TRACE_LEAVE2("ret_err: %d",ret_err);
	return ret_err; 
}

/******************************************************************************
@brief		: Process the response from the client for the validate callback.  
@param[in]	: trk_info - Carries the required info to call start callback. 
@return		: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
******************************************************************************/
SaUint32T plms_cbk_validate_resp_ok_err_proc(PLMS_TRACK_INFO *trk_info)
{
	SaUint32T ret_err = NCSCC_RC_SUCCESS;
	PLMS_ENTITY *ent;
	
	TRACE_ENTER2("Ent: %s, track_cause: %d",
	trk_info->root_entity->dn_name_str,trk_info->track_cause);

	ent = trk_info->root_entity;
	switch (trk_info->track_cause){
	
	/* Try Lock. */
	case SA_PLM_CAUSE_LOCK:
		plms_lock_start_cbk_call(ent,trk_info); 
		
		/* There are no start subcriber, call completed cbk.*/
		if (0 == trk_info->track_count){
			plms_lock_completed_cbk_call(ent,trk_info);
		}else{
			plms_peer_async_send(ent,SA_PLM_CHANGE_START,
			SA_PLM_CAUSE_LOCK);
		}
		break;

	case SA_PLM_CAUSE_HE_DEACTIVATION:
		plms_deact_start_cbk_call(ent,trk_info);

		if(0 == trk_info->track_count){
			plms_deact_completed_cbk_call(ent,trk_info);
		}else{
			plms_peer_async_send(ent,SA_PLM_CHANGE_START,
			SA_PLM_CAUSE_HE_DEACTIVATION);
		}
		break;

	default:
		LOG_ER("Invalid track_cause: %d",trk_info->track_cause);
		ret_err = NCSCC_RC_FAILURE;
	}
	
	TRACE_LEAVE2("ret_err: %d",ret_err);
	return ret_err;
}

/******************************************************************************
@brief		: Process the response from the client for the start callback.  
@param[in]	: trk_info - Carries the required info to call completed callback. 
@return		: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
******************************************************************************/
SaUint32T plms_cbk_start_resp_ok_err_proc(PLMS_TRACK_INFO *trk_info)
{
	SaUint32T ret_err = NCSCC_RC_SUCCESS;
	PLMS_ENTITY *ent;
	
	TRACE_ENTER2("Ent: %s, track_cause: %d",
	trk_info->root_entity->dn_name_str,trk_info->track_cause);

	ent = trk_info->root_entity;

	switch(trk_info->track_cause){
		/* Try/Default lock.*/
	case SA_PLM_CAUSE_LOCK:
		plms_lock_completed_cbk_call(ent,trk_info);
		break;

		
	case SA_PLM_CAUSE_HE_DEACTIVATION:
		plms_deact_completed_cbk_call(ent,trk_info);
		break;

		
	case SA_PLM_CAUSE_SHUTDOWN:
		plms_shutdown_completed_cbk_call(ent,trk_info);
		break;
		
	default:
		LOG_ER("Invalid track cause: %d",trk_info->track_cause);
		ret_err =  NCSCC_RC_FAILURE;
	}

	TRACE_LEAVE2("ret_err: %d",ret_err);
	return ret_err;
}

/******************************************************************************
@brief		: Process imm admin event. Call the required function for
		performing admin operation.
@param[in]	: evt - PLMS_EVT representing IMM admin event.
@return		: NCSCC_RC_SUCCESS/SA_AIS_OK(1) - For success.
		  NCSCC_RC_FAILURE(0) - For failure.
		  others(SaAisErrorT) - For failure.
******************************************************************************/
SaUint32T plms_imm_adm_op_req_process(PLMS_EVT *evt)
{
	PLMS_ENTITY *ent;
	PLMS_CB *cb = plms_cb;
	SaInt8T tmp[SA_MAX_NAME_LENGTH+1];
	SaUint32T ret_err = NCSCC_RC_FAILURE;

	plms_get_str_from_dn_name(&(evt->req_evt.admin_op.dn_name),tmp);
	TRACE_ENTER2("Entity: %s. Admin_op id: %d",tmp,
	evt->req_evt.admin_op.operation_id);
	
	ent = (PLMS_ENTITY *)ncs_patricia_tree_get(&(cb->entity_info),
			   (SaUint8T *)&(evt->req_evt.admin_op.dn_name));
	if (NULL == ent) {
		ret_err = saImmOiAdminOperationResult(cb->oi_hdl,
			evt->req_evt.admin_op.inv_id,SA_AIS_ERR_NO_RESOURCES);  

		LOG_ER("plms_imm_adm_op_req_process:\
			Ent not found for in patricia tree. dn_name: %s",tmp);
		TRACE_LEAVE2("ret_err: %d",ret_err);
		return ret_err;
	}

	if ( (SA_PLM_ADMIN_LOCK > evt->req_evt.admin_op.operation_id) || 
		(SA_PLM_ADMIN_REMOVED < evt->req_evt.admin_op.operation_id)){
	
		LOG_ER("Invalid admin op id: %d", 
		evt->req_evt.admin_op.operation_id);
		
		ret_err = saImmOiAdminOperationResult(cb->oi_hdl,
			evt->req_evt.admin_op.inv_id,SA_AIS_ERR_NOT_SUPPORTED);
		if (NCSCC_RC_SUCCESS != ret_err){
			LOG_ER("Sending admin response to IMM failed. \
						IMM Ret code: %d",ret_err);
		}else{
			TRACE("Sending admin response to IMM successful.");
		}

		TRACE_LEAVE2("ret_err: %d",ret_err);
		return ret_err;
	}

	if ( PLMS_HE_ENTITY == ent->entity_type){
		switch(evt->req_evt.admin_op.operation_id){
		case SA_PLM_ADMIN_RESET:
			ret_err = plms_HE_adm_reset_op(evt);
			break;
		case SA_PLM_ADMIN_LOCK_INSTANTIATION:
		case SA_PLM_ADMIN_UNLOCK_INSTANTIATION:
		case SA_PLM_ADMIN_RESTART:
			LOG_ER("Invalid admin op for HE. Op id: %d",
			evt->req_evt.admin_op.operation_id);
			
			ret_err = saImmOiAdminOperationResult(cb->oi_hdl,
			evt->req_evt.admin_op.inv_id,SA_AIS_ERR_NOT_SUPPORTED);

			if (NCSCC_RC_SUCCESS != ret_err){
				LOG_ER("Sending admin response to IMM failed. \
						IMM Ret code: %d",ret_err);
			}else{
				TRACE("Sending admin response to IMM \
								successful.");
			}

			break;

		case SA_PLM_ADMIN_REPAIRED:
			ret_err = plms_adm_repair(evt);
			break;
		case SA_PLM_ADMIN_REMOVED:
			ret_err = plms_adm_remove(evt);
			break;
		default:
			ret_err = plm_HE_adm_state_op
			[ent->entity.he_entity.saPlmHEAdminState]
			[evt->req_evt.admin_op.operation_id](evt);
			break;
		}
	}else if (PLMS_EE_ENTITY == ent->entity_type){
		switch(evt->req_evt.admin_op.operation_id){
		case SA_PLM_ADMIN_RESTART:
			ret_err = plms_EE_adm_restart(evt);
			break;
		case SA_PLM_ADMIN_DEACTIVATE:
		case SA_PLM_ADMIN_ACTIVATE:
		case SA_PLM_ADMIN_RESET:
			LOG_ER("Invalid admin op for EE. Op id: %d",
			evt->req_evt.admin_op.operation_id);
			
			ret_err = saImmOiAdminOperationResult(cb->oi_hdl,
			evt->req_evt.admin_op.inv_id,SA_AIS_ERR_NOT_SUPPORTED);

			if (NCSCC_RC_SUCCESS != ret_err){
				LOG_ER("Sending admin response to IMM failed. \
						IMM Ret code: %d",ret_err);
			}else{
				TRACE("Sending admin response to IMM \
								successful.");
			}
			break;

		case SA_PLM_ADMIN_REPAIRED:
			ret_err = plms_adm_repair(evt);
			break;
		case SA_PLM_ADMIN_REMOVED:
			ret_err = plms_adm_remove(evt);
			break;
		default:
			ret_err = plm_EE_adm_state_op
			[ent->entity.ee_entity.saPlmEEAdminState]
			[evt->req_evt.admin_op.operation_id](evt);
			break;
		}
	}else{
		ret_err = saImmOiAdminOperationResult(cb->oi_hdl,
			evt->req_evt.admin_op.inv_id,SA_AIS_ERR_NOT_SUPPORTED);  
		if (NCSCC_RC_SUCCESS != ret_err){
			LOG_ER("Sending admin response to IMM failed. \
						IMM Ret code: %d",ret_err);
		}else{
			TRACE("Sending admin response to IMM successful.");
		}

		LOG_ER("plms_imm_adm_op_req_process:\
			Entity type wrong. Ent type: %d",ent->entity_type);

	}

	TRACE_LEAVE2("ret_err: %d",ret_err);
	return ret_err;
}

/******************************************************************************
@brief		: Call the start callback for lock operation after getting
		response for the validate callback from all the clients.
@param[in]	: ent - Entity on which lock operation is performed.
@param[in]	: trk_info - Carries required info to call the callback.
******************************************************************************/
static void plms_lock_start_cbk_call(PLMS_ENTITY *ent, PLMS_TRACK_INFO *trk_info)
{
	trk_info->change_step = SA_PLM_CHANGE_START;

	/* Call start callback.*/
	plms_cbk_call(trk_info,1);

	return;
}
/******************************************************************************
@brief		: Call the completed callback for lock operation after getting
		response for the start callback from all the clients.
@param[in]	: ent - Entity on which lock operation is performed.
@param[in]	: trk_info - Carries required info to call the callback.
******************************************************************************/
static void plms_lock_completed_cbk_call(PLMS_ENTITY *ent, PLMS_TRACK_INFO *trk_info)
{
	PLMS_GROUP_ENTITY *head = NULL;
	PLMS_CB *cb = plms_cb;
	SaUint32T ret_err = NCSCC_RC_FAILURE;
	
	plms_admin_state_set(ent, SA_PLM_HE_ADMIN_LOCKED,NULL,
				SA_NTF_MANAGEMENT_OPERATION,
				SA_PLM_NTFID_STATE_CHANGE_ROOT);
	
	plms_readiness_state_set(ent,
				SA_PLM_READINESS_OUT_OF_SERVICE,NULL,
				SA_NTF_MANAGEMENT_OPERATION,
				SA_PLM_NTFID_STATE_CHANGE_ROOT);

	/* Mark the readiness state and expected readiness state of all the 
	affected entities to OOS and set the dependency flag.*/
	head = trk_info->aff_ent_list;
	while(head){
	
		plms_readiness_state_set(head->plm_entity, SA_PLM_READINESS_OUT_OF_SERVICE,
		ent,SA_NTF_MANAGEMENT_OPERATION, SA_PLM_NTFID_STATE_CHANGE_DEP);
		
		plms_readiness_flag_mark_unmark(head->plm_entity,SA_PLM_RF_DEPENDENCY,TRUE,ent,
		SA_NTF_MANAGEMENT_OPERATION,SA_PLM_NTFID_STATE_CHANGE_DEP);
		
		if (PLMS_EE_ENTITY == head->plm_entity->entity_type){
			ret_err = plms_ee_term(head->plm_entity,FALSE,0);
			if (NCSCC_RC_SUCCESS != ret_err){
				LOG_ER("EE term FAILED. Ent: %s",
				head->plm_entity->dn_name_str);
			}
		}
		head = head->next;
	}

	if (PLMS_EE_ENTITY == ent->entity_type){
		/* Lock the root EE. I am not wating, till I get 
		the callback.If this API returns success, I am done.  */
		ret_err = plms_ee_lock(ent,TRUE,1/*mngt_cbk*/);
		if (NCSCC_RC_SUCCESS != ret_err){
			
			LOG_ER("EE lock FAILED. Ent: %s",
			ent->dn_name_str);
		}
	}
	
	trk_info->change_step = SA_PLM_CHANGE_COMPLETED;
	/* Call completed callback callback.*/
	plms_cbk_call(trk_info,1);
	plms_peer_async_send(ent,SA_PLM_CHANGE_COMPLETED,SA_PLM_CAUSE_LOCK);

	plms_ent_exp_rdness_status_clear(trk_info->root_entity);
	plms_aff_ent_exp_rdness_status_clear(trk_info->aff_ent_list);

	/* If management lost of EE is set, return 
	SA_AIS_ERR_DEPLOYMENT to IMM. */
	if ( (PLMS_EE_ENTITY == ent-> entity_type) && 
		/*plms_rdness_flag_is_set(ent,SA_PLM_RF_MANAGEMENT_LOST)){*/
		 (NCSCC_RC_SUCCESS != ret_err)){
		ret_err = saImmOiAdminOperationResult(cb->oi_hdl ,
			trk_info->inv_id, SA_AIS_ERR_DEPLOYMENT);
		
		if (NCSCC_RC_SUCCESS != ret_err){
			LOG_ER("Sending admin response to IMM failed. \
					IMM Ret code: %d",ret_err);
		}else{
			TRACE("Sending admin response to IMM \
							successful.");
		}
		
	}else {
		/* Return OK to IMM. */
		ret_err = saImmOiAdminOperationResult(cb->oi_hdl ,
				trk_info->inv_id, SA_AIS_OK);
		
		if (NCSCC_RC_SUCCESS != ret_err){
			LOG_ER("Sending admin response to IMM failed. \
					IMM Ret code: %d",ret_err);
		}else{
			TRACE("Sending admin response to IMM \
							successful.");
		}

	}
	
	/* I am done with the admin operation. */
	trk_info->root_entity->adm_op_in_progress = FALSE;
	trk_info->root_entity->am_i_aff_ent = FALSE;
	plms_aff_ent_flag_mark_unmark(trk_info->aff_ent_list,FALSE);
	
	/* TODO: Do we need to reset expected readiness state of all
	the affected entities.*/
	plms_trk_info_free(trk_info);

	return;
}

/******************************************************************************
@brief		: Call the completed callback for shutdown operation after getting
		response for the start callback from all the clients.
@param[in]	: ent - Entity on which lock operation is performed.
@param[in]	: trk_info - Carries required info to call the callback.
******************************************************************************/
static void plms_shutdown_completed_cbk_call(PLMS_ENTITY *ent, 
PLMS_TRACK_INFO  *trk_info)
{
	PLMS_GROUP_ENTITY *head = NULL;
	SaUint32T ret_err = NCSCC_RC_FAILURE;
	PLMS_CB *cb = plms_cb;

	/* Mark admin state of root ent to LOCK and 
	readiness state to OOS.*/
	plms_admin_state_set(ent,SA_PLM_HE_ADMIN_LOCKED,NULL,
			SA_NTF_MANAGEMENT_OPERATION,
			SA_PLM_NTFID_STATE_CHANGE_ROOT);

	plms_readiness_state_set(ent,
			SA_PLM_READINESS_OUT_OF_SERVICE,NULL,
			SA_NTF_MANAGEMENT_OPERATION,
			SA_PLM_NTFID_STATE_CHANGE_ROOT);

	/* Mark readiness state of all the affected ent to OOS and set 
	the dependency flag.*/
	head = trk_info->aff_ent_list;
	while (head){
		plms_readiness_state_set(head->plm_entity,
				SA_PLM_READINESS_OUT_OF_SERVICE,
				ent,SA_NTF_MANAGEMENT_OPERATION,
				SA_PLM_NTFID_STATE_CHANGE_DEP);
		
		plms_readiness_flag_mark_unmark(head->plm_entity,
				SA_PLM_RF_DEPENDENCY,TRUE,ent,
				SA_NTF_MANAGEMENT_OPERATION,
				SA_PLM_NTFID_STATE_CHANGE_ROOT);
		head = head->next;
	}

	/* Terminate all the affected EEs.*/
	head = trk_info->aff_ent_list;
	while(head){

		if (PLMS_EE_ENTITY != head->plm_entity->entity_type)
			head = head->next;

		ret_err = plms_ee_term(head->plm_entity,FALSE,0);
		if (NCSCC_RC_SUCCESS != ret_err){
			LOG_ER("EE Term FAILED. Ent: %s",
			ent->dn_name_str);
		}

		head = head->next;
	}
	
	if (PLMS_EE_ENTITY == ent->entity_type){
		/* Lock the root EE. I am not wating, till I get 
		the callback. If this API returns success, I am done.
		*/
		ret_err = plms_ee_lock(ent,1,1/*mngt_cbk*/);
		if (NCSCC_RC_SUCCESS != ret_err){
			LOG_ER("EE Lock FAILED. Ent: %s",
			ent->dn_name_str);
		}
	}
	trk_info->change_step = SA_PLM_CHANGE_COMPLETED;
	/* Call completed callback callback.*/
	plms_cbk_call(trk_info,1);
	plms_peer_async_send(ent,SA_PLM_CHANGE_COMPLETED,SA_PLM_CAUSE_SHUTDOWN);
	
	plms_ent_exp_rdness_status_clear(trk_info->root_entity);
	plms_aff_ent_exp_rdness_status_clear(trk_info->aff_ent_list);
	
	/* If management lost of EE is set, return 
	SA_AIS_ERR_DEPLOYMENT to IMM. */
	if ( (PLMS_EE_ENTITY == ent->entity_type) && 
		/*plms_rdness_flag_is_set(ent,SA_PLM_RF_MANAGEMENT_LOST)){*/
		(NCSCC_RC_SUCCESS != ret_err)){

		ret_err = saImmOiAdminOperationResult( cb->oi_hdl,
			trk_info->inv_id, SA_AIS_ERR_DEPLOYMENT);
		
		if (NCSCC_RC_SUCCESS != ret_err){
			LOG_ER("Sending admin response to IMM failed. \
					IMM Ret code: %d",ret_err);
		}else{
			TRACE("Sending admin response to IMM \
							successful.");
		}
		
	}else {
		/* Return OK to IMM. */
		ret_err = saImmOiAdminOperationResult(cb->oi_hdl ,
					trk_info->inv_id, SA_AIS_OK);
		
		if (NCSCC_RC_SUCCESS != ret_err){
			LOG_ER("Sending admin response to IMM failed. \
					IMM Ret code: %d",ret_err);
		}else{
			TRACE("Sending admin response to IMM \
							successful.");
		}
	}
	
	/* I am done with the admin operation. */
	trk_info->root_entity->adm_op_in_progress = FALSE;
	trk_info->root_entity->am_i_aff_ent = FALSE;
	plms_aff_ent_flag_mark_unmark(trk_info->aff_ent_list,FALSE);
	
	/* TODO: Do we need to reset expected readiness state of all 
	the affected entities.*/						
	plms_trk_info_free(trk_info);

	return;
}

/******************************************************************************
@brief		: Call the start callback for deact operation after getting
		response for the validate callback from all the clients.
@param[in]	: ent - Entity on which lock operation is performed.
@param[in]	: trk_info - Carries required info to call the callback.
******************************************************************************/
void plms_deact_start_cbk_call(PLMS_ENTITY *ent, PLMS_TRACK_INFO *trk_info)
{
#if 0	
	PLMS_GROUP_ENTITY *head = NULL;
	/* Mark the presence state of all the affected EEs to
	terminating.*/
	head = trk_info->aff_ent_list;
	while(head){
		if (PLMS_EE_ENTITY == head->plm_entity->entity_type){
			plms_presence_state_set(head->plm_entity,
				SA_PLM_EE_PRESENCE_TERMINATING,ent,
				SA_NTF_MANAGEMENT_OPERATION,
				SA_PLM_NTFID_STATE_CHANGE_DEP);
		}
		head = head->next;
	}
#endif
	trk_info->change_step = SA_PLM_CHANGE_START;
	plms_cbk_call(trk_info,1);
	
#if 0
	plms_ent_grp_list_free(trk_info->group_info_list);
#endif	
	return;
}

/******************************************************************************
@brief		: Call the completed callback for deact operation after getting
		response for the start callback from all the clients.
@param[in]	: ent - Entity on which lock operation is performed.
@param[in]	: trk_info - Carries required info to call the callback.
******************************************************************************/
void plms_deact_completed_cbk_call(PLMS_ENTITY *ent, PLMS_TRACK_INFO *trk_info)
{
	PLMS_GROUP_ENTITY *head;
	SaUint32T ret_err = NCSCC_RC_FAILURE;
	
	/* Uninstantiate all the EEs.*/
	head = trk_info->aff_ent_list;
	while(head){
		if ((PLMS_EE_ENTITY == head->plm_entity->entity_type) &&
		(SA_PLM_EE_PRESENCE_UNINSTANTIATED != 
		head->plm_entity->entity.ee_entity.
		saPlmEEPresenceState)){
		
			ret_err = plms_ee_term(head->plm_entity, FALSE,0);
			if (NCSCC_RC_SUCCESS != ret_err){
				LOG_ER("EE Term FAILED.Ent: %s",
				head->plm_entity->dn_name_str);
			}
		}

		head = head->next;
	}
		
	/* Set the HE state to M1.*/
	ret_err = plms_he_deactivate(trk_info->root_entity,FALSE,FALSE);
	if (NCSCC_RC_SUCCESS != ret_err){
		LOG_ER("HE Deactivate FAILED. Ent: %s",
		trk_info->root_entity->dn_name_str);

		ent->adm_op_in_progress = FALSE;
		ent->am_i_aff_ent = FALSE;
		plms_aff_ent_flag_mark_unmark(trk_info->aff_ent_list, FALSE);

		plms_aff_ent_exp_rdness_status_clear(trk_info->aff_ent_list);
		plms_ent_list_free(trk_info->aff_ent_list);
		trk_info->aff_ent_list = NULL;
		/*plms_ent_grp_list_free(trk_info->group_info_list);
		trk_info->group_info_list = NULL;*/

		plms_ent_exp_rdness_state_ow(ent);
		trk_info->change_step = SA_PLM_CHANGE_COMPLETED;
		plms_cbk_call(trk_info,1);
			
		plms_peer_async_send(ent,SA_PLM_CHANGE_COMPLETED,
		SA_PLM_CAUSE_HE_DEACTIVATION);

		plms_ent_exp_rdness_status_clear(ent);
		plms_trk_info_free(trk_info);

		return;
	}
	/* Delay the completed callback till the time I get M1 for the event.*/
	plms_ent_grp_list_free(trk_info->group_info_list);
	trk_info->group_info_list = NULL;

	return;
}

/******************************************************************************
@brief		: Process EE abrupt restart.
@param[in]	: ent - EE to be restarted.
@param[in]	: evt - Event carrying imm admin operation.
@param[in]	: aff_ent_list - List of entities whose readiness status is 
		affected because of the restart operation.
@return		: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE		
******************************************************************************/
static SaUint32T plms_ee_abrupt_restart_process(PLMS_ENTITY *ent, PLMS_EVT *evt,
PLMS_GROUP_ENTITY *aff_ent_list)
{
	SaUint32T ret_err = NCSCC_RC_SUCCESS,count = 0;
	PLMS_TRACK_INFO *trk_info;
	PLMS_GROUP_ENTITY *head;
	PLMS_ENTITY_GROUP_INFO_LIST *log_head_grp;
	PLMS_CB *cb = plms_cb;	
	
	TRACE_ENTER2("Entity: %s. Op: Admin restart variant: Abrupt.",
	ent->dn_name_str);

	if (NULL == ent->parent){
		LOG_ER("Parent of EE %s is NULL, Abrupt restart\
		not possible.",ent->dn_name_str);
		
		ret_err= saImmOiAdminOperationResult(cb->oi_hdl,
		evt->req_evt.admin_op.inv_id,
		SA_AIS_ERR_BAD_OPERATION);

		plms_ent_list_free(aff_ent_list);
		aff_ent_list = NULL;

		TRACE_LEAVE2("ret_err: %d",ret_err);
		return ret_err;
	}
	
	/* Restart the EE by resetting the parent. Not cosidering
	the virtualization.
	TODO: Should we check whether the siblings of the target
	EE is affected?? Actually not required. A HE can have only
	one child EE. And child HEs are not affected by the reset
	of parent HE.
	*/
	if ( PLMS_HE_ENTITY == ent->parent->entity_type){
		ret_err = plms_he_reset(ent->parent,0/*adm_op*/,
					1/*mngt_cbk*/,SAHPI_COLD_RESET);
		if (NCSCC_RC_SUCCESS != ret_err){
			LOG_ER("Reset of parent HE %s failed and hence \
			admin restart of EE %s can not be performed.",
			ent->parent->dn_name_str,ent->dn_name_str);
			
			/* Set admin pending and management lost flag fot the entity.*/
			if (!plms_rdness_flag_is_set(ent, SA_PLM_RF_MANAGEMENT_LOST)){
				
				/* Set management lost flag.*/
				plms_readiness_flag_mark_unmark(ent,
					SA_PLM_RF_MANAGEMENT_LOST,1/*mark*/,
					NULL,SA_NTF_MANAGEMENT_OPERATION,
					SA_PLM_NTFID_STATE_CHANGE_ROOT);
				
				plms_readiness_flag_mark_unmark(ent,
					SA_PLM_RF_ADMIN_OPERATION_PENDING,1/*mark*/,
					NULL,SA_NTF_MANAGEMENT_OPERATION,
					SA_PLM_NTFID_STATE_CHANGE_ROOT);

				plms_mngt_lost_clear_cbk_call(ent,1/*mark*/);
			}

			ret_err= saImmOiAdminOperationResult(cb->oi_hdl,
			evt->req_evt.admin_op.inv_id,
			SA_AIS_ERR_FAILED_OPERATION);

			plms_ent_list_free(aff_ent_list);
			aff_ent_list = NULL;

			TRACE_LEAVE2("ret_err: %d",ret_err);
			return ret_err;
		}

	}else{
		/* TODO: Virtualization.Not required for this release.*/
	}
	
	/* Admin operation started. */ 
	ent->adm_op_in_progress = SA_PLM_CAUSE_EE_RESTART; 
	ent->am_i_aff_ent = TRUE;
	plms_aff_ent_flag_mark_unmark(aff_ent_list,TRUE);

	/* Take care of target EE. */
	plms_presence_state_set(ent,SA_PLM_EE_PRESENCE_INSTANTIATING,NULL,
				SA_NTF_MANAGEMENT_OPERATION,
				SA_PLM_NTFID_STATE_CHANGE_ROOT);

	plms_readiness_state_set(ent,SA_PLM_READINESS_OUT_OF_SERVICE,NULL,
				SA_NTF_MANAGEMENT_OPERATION,
				SA_PLM_NTFID_STATE_CHANGE_ROOT);
	count++;
	
	/* Get the trk_info ready.*/
	trk_info = (PLMS_TRACK_INFO *)calloc(1,sizeof(PLMS_TRACK_INFO));
	trk_info->root_entity = ent;
	ent->trk_info = trk_info;

	/* Reset all the dependent EEs.*/
	head = aff_ent_list;
	while(head){
		ret_err = plms_ee_reboot(head->plm_entity,FALSE,TRUE);
		if (NCSCC_RC_SUCCESS == ret_err){
			plms_presence_state_set(head->plm_entity,
					SA_PLM_EE_PRESENCE_UNINSTANTIATED,ent,
					SA_NTF_MANAGEMENT_OPERATION,
					SA_PLM_NTFID_STATE_CHANGE_DEP);
			head->plm_entity->trk_info = trk_info;	
			count++;	
		}else{
			LOG_ER("EE reset failed. Ent: %s",
			head->plm_entity->dn_name_str);
		}
		plms_readiness_state_set(head->plm_entity,
				SA_PLM_READINESS_OUT_OF_SERVICE,ent,
				SA_NTF_MANAGEMENT_OPERATION,
				SA_PLM_NTFID_STATE_CHANGE_DEP);
		plms_readiness_flag_mark_unmark(head->plm_entity,
				SA_PLM_RF_DEPENDENCY,1/* mark */,ent,
				SA_NTF_MANAGEMENT_OPERATION,
				SA_PLM_NTFID_STATE_CHANGE_DEP);			
		head = head->next;
	}

	plms_aff_ent_exp_rdness_state_ow(aff_ent_list);
	plms_ent_exp_rdness_state_ow(ent);

	trk_info->aff_ent_list = aff_ent_list;
	
	/* Add the groups, root entity(ent) belong to.*/
	plms_ent_grp_list_add(ent,&(trk_info->group_info_list));
	
	/* Find out all the groups, all affected entities belong to and add 
	the groups to trk_info->group_info_list.*/ 
	plms_ent_list_grp_list_add(aff_ent_list,
				&(trk_info->group_info_list));	
	
	TRACE("Affected groups for ent %s: ",ent->dn_name_str);
	log_head_grp = trk_info->group_info_list;
	while(log_head_grp){
		TRACE("%llu,",log_head_grp->ent_grp_inf->entity_grp_hdl);
		log_head_grp = log_head_grp->next;
	}
		
	trk_info->imm_adm_opr_id = evt->req_evt.admin_op.operation_id; 
	trk_info->inv_id = evt->req_evt.admin_op.inv_id;
	trk_info->change_step = SA_PLM_CHANGE_COMPLETED;
	trk_info->track_cause = SA_PLM_CAUSE_EE_RESTART;
	trk_info->root_correlation_id = SA_NTF_IDENTIFIER_UNUSED;
	trk_info->grp_op = SA_PLM_GROUP_MEMBER_READINESS_CHANGE; 
	trk_info->root_entity = ent;
	trk_info->track_count = count;
	/* 1. Call the 1st callback for restart.*/
	plms_cbk_call(trk_info,1);
		
	plms_aff_ent_exp_rdness_status_clear(aff_ent_list);
	plms_ent_exp_rdness_status_clear(ent);
	plms_ent_grp_list_free(trk_info->group_info_list);
	trk_info->group_info_list = NULL;
	/* Not needed for abrupt restart.*/
	trk_info->track_cause = 0;
	
	/********************************************************************/

	/* Target EE as well as affected EEs are restarted. When each restarted
	EE is instantiated, move the EE to InSvc and call the completed cbk
	with track cause as SA_PLM_CAUSE_EE_INSTANTIATED and root entity as 
	the target entity. Make sure to free the trk_info->group_info_list
	and decrement the trk_info->count.
	
	After all the aff EEs are instantiated (trk_info->count becomes 0), 
	return to IMM. Clear the admin context. 
	Free trk_info->group_info_list, trk_info, trk_info->aff_ent_list.
	
	*/

	TRACE_LEAVE2("ret_err: %d",ret_err);
	return ret_err;
}
/******************************************************************************
@brief		: Process EE graceful restart.
@param[in]	: ent - EE to be restarted.
@param[in]	: evt - Event carrying imm admin operation.
@param[in]	: aff_ent_list - List of entities whose readiness status is 
		affected because of the restart operation.
@return		: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE		
******************************************************************************/
static SaUint32T plms_ee_graceful_restart_process(PLMS_ENTITY *ent, 
PLMS_EVT *evt,PLMS_GROUP_ENTITY *aff_ent_list)
{
	SaUint32T ret_err = NCSCC_RC_SUCCESS,count = 0;
	PLMS_TRACK_INFO *trk_info;
	PLMS_GROUP_ENTITY *head;
	PLMS_CB *cb = plms_cb;	
	
	TRACE_ENTER2("Entity: %s. Op: Admin restart variant: Abrupt.",
	ent->dn_name_str);
	/* Graceful restart. Reboot the EE itself.*/
	ret_err = plms_ee_reboot(ent,TRUE,1/*mngt_cbk*/);
	if (NCSCC_RC_SUCCESS != ret_err){
		LOG_ER("EE %s reset failed and hence admin restart \
		can not be performed.",ent->dn_name_str);
		ret_err= saImmOiAdminOperationResult(cb->oi_hdl,
		evt->req_evt.admin_op.inv_id,
		SA_AIS_ERR_FAILED_OPERATION);

		plms_ent_list_free(aff_ent_list);
		aff_ent_list = NULL;

		TRACE_LEAVE2("ret_err: %d",ret_err);
		return ret_err;
	}

	
	/* Admin operation started. */ 
	ent->adm_op_in_progress = SA_PLM_CAUSE_EE_RESTART; 
	ent->am_i_aff_ent = TRUE;
	plms_aff_ent_flag_mark_unmark(aff_ent_list,TRUE);

	count++;
	
	/* Get the trk_info ready.*/
	trk_info = (PLMS_TRACK_INFO *)calloc(1,sizeof(PLMS_TRACK_INFO));
	trk_info->root_entity = ent;
	ent->trk_info = trk_info;
	
	/* Reset all the dependent EEs.*/
	head = aff_ent_list;
	while(head){
		ret_err = plms_ee_reboot(head->plm_entity,FALSE,TRUE);
		if (NCSCC_RC_SUCCESS == ret_err){
			head->plm_entity->trk_info = trk_info;	
			count++;	
		}else{
			LOG_ER("EE reset failed. Ent: %s",
			head->plm_entity->dn_name_str);
		}
		head = head->next;
	}
	
	trk_info->imm_adm_opr_id = evt->req_evt.admin_op.operation_id; 
	trk_info->inv_id = evt->req_evt.admin_op.inv_id;
	trk_info->change_step = SA_PLM_CHANGE_COMPLETED;
	trk_info->track_cause = SA_PLM_CAUSE_EE_RESTART;
	trk_info->root_correlation_id = SA_NTF_IDENTIFIER_UNUSED;
	trk_info->grp_op = SA_PLM_GROUP_MEMBER_READINESS_CHANGE; 
	trk_info->root_entity = ent;
	trk_info->track_count = count;

	TRACE_LEAVE2("ret_err: %d",ret_err);
	return ret_err;
}
