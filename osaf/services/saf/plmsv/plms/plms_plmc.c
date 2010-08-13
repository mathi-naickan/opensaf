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
*  MODULE NAME:  plms_plmc.c                                                 *
*                                                                            *
*                                                                            *
*  DESCRIPTION: Processing PLMC events.				  	     *
*                                                                            *
*****************************************************************************/

#include "plms.h"
#include "plms_utils.h"
#include "plmc_lib.h"
#include "plmc_lib_internal.h"
#include "plmc.h"

static int32  plms_plmc_tcp_cbk(tcp_msg *);
static SaUint32T plms_ee_verify(PLMS_ENTITY *, PLMS_PLMC_EE_OS_INFO *);
static SaUint32T plms_os_info_parse(SaInt8T *, PLMS_PLMC_EE_OS_INFO *);
static SaUint32T plms_plmc_unlck_insvc(PLMS_ENTITY *, PLMS_TRACK_INFO *,PLMS_GROUP_ENTITY *,SaUint32T);
void plms_os_info_free(PLMS_PLMC_EE_OS_INFO *);
static SaUint32T plms_inst_term_failed_isolate(PLMS_ENTITY *);

static SaUint32T plms_ee_terminated_mngt_flag_clear(PLMS_ENTITY *);
static SaUint32T plms_tcp_discon_mngt_flag_clear(PLMS_ENTITY *);
static SaUint32T plms_tcp_connect_mngt_flag_clear(PLMS_ENTITY *);
static SaUint32T plms_restart_resp_mngt_flag_clear(PLMS_ENTITY *);
static SaUint32T plms_lck_resp_mngt_flag_clear(PLMS_ENTITY *);
static SaUint32T plms_unlck_resp_mngt_flag_clear(PLMS_ENTITY *);
static SaUint32T plms_lckinst_resp_mngt_flag_clear(PLMS_ENTITY *);
static SaUint32T plms_os_info_resp_mngt_flag_clear(PLMS_ENTITY *);

static void plms_insted_dep_immi_failure_cbk_call(PLMS_ENTITY *, PLMS_GROUP_ENTITY *);
static void plms_is_dep_set_cbk_call(PLMS_ENTITY *);
/******************************************************************************
@brief		: Process instantiating event from PLMC.
		  1. Do the OS verification irrespective of previous state.
		  2. If successful then, mark the presence state to 
		  instantiating.
		  3. Start instantiating timer if not running. 

		  This is almost dummy event for PLMS. I dont mind missing
		  this event as it is UDP broadcasted.
@param[in]	: ent - EE
@param[in]	: os_inso - OS info from PLMC to be verified.
@return		: NCSCC_RC_FAILURE/NCSCC_RC_SUCCESS.
******************************************************************************/
SaUint32T plms_plmc_instantiating_process(PLMS_ENTITY *ent,
					PLMS_PLMC_EE_OS_INFO *os_info)
{
	SaUint32T ret_err;

	TRACE_ENTER2("Entity: %s",ent->dn_name_str);

	/* If the admin state of the ent is lckinst, then ignore the 
	state as we cannot terminate the entity at this point of time.*/
	if ( (SA_PLM_EE_ADMIN_LOCKED_INSTANTIATION == ent->entity.ee_entity.saPlmEEAdminState) || 
	((NULL != ent->parent) && (plms_is_rdness_state_set(ent->parent,SA_PLM_READINESS_OUT_OF_SERVICE))) || 
	(!plms_min_dep_is_ok(ent))){
		
		LOG_ER("EE insting ignored, as its state or parent/dep state not proper:%s", ent->dn_name_str);
		TRACE_LEAVE2("Return Val: %d",NCSCC_RC_FAILURE);
		return NCSCC_RC_FAILURE; 	
	}
	
	/* Verify the EE Os.*/
	ret_err = plms_ee_verify(ent,os_info);
	plms_os_info_free(os_info);
	/* If verification failed then ignore the EE.*/
	if (NCSCC_RC_SUCCESS != ret_err){
		LOG_ER("EE Verification FAILED for ent: %s",ent->dn_name_str);	
		return ret_err;	
	}
	
	TRACE("EE verification is SUCCESSFUL for ent: %s",ent->dn_name_str);
	/* Verification is successful. Update the presence state.*/
	if ( SA_PLM_EE_PRESENCE_INSTANTIATING != 
		ent->entity.ee_entity.saPlmEEPresenceState){
		plms_presence_state_set(ent,
				SA_PLM_EE_PRESENCE_INSTANTIATING,
				NULL,SA_NTF_OBJECT_OPERATION,
				SA_PLM_NTFID_STATE_CHANGE_ROOT);
	}
	
	/* Start istantiation-failed timer if it is not in progress.*/
	if (!(ent->tmr.timer_id)){
		ent->tmr.tmr_type = PLMS_TMR_EE_INSTANTIATING;
		ret_err = plms_timer_start(&ent->tmr.timer_id,ent,
		ent->entity.ee_entity.saPlmEEInstantiateTimeout);
		if (NCSCC_RC_SUCCESS != ret_err){
			ent->tmr.timer_id = 0;
			ent->tmr.tmr_type = PLMS_TMR_NONE;
			ent->tmr.context_info = NULL;
		}
	}else if (PLMS_TMR_EE_TERMINATING == ent->tmr.tmr_type){
		/* Stop the term timer and start the inst timer.*/
		plms_timer_stop(ent);
		ent->tmr.tmr_type = PLMS_TMR_EE_INSTANTIATING;
		ret_err = plms_timer_start(&ent->tmr.timer_id,ent,
		ent->entity.ee_entity.saPlmEEInstantiateTimeout);
		if (NCSCC_RC_SUCCESS != ret_err){
			ent->tmr.timer_id = 0;
			ent->tmr.tmr_type = PLMS_TMR_NONE;
			ent->tmr.context_info = NULL;
		}
	} else {
		/* The timer is already running.*/
	}

	TRACE_LEAVE2("Return Val: %d",ret_err);
	return ret_err; 	
}	

/******************************************************************************
@brief		: Process instantiated event from PLMC. If required  OS verification
		is done and the entity is moved to instantiated. But still the
		EE is not moved to insvc. Waiting for tcp-connect.
@param[in]	: ent - EE
@param[in]	: os_info - OS_INFO from PLMC to be verified.
@return		: NCSCC_RC_FAILURE/NCSCC_RC_SUCCESS
******************************************************************************/
SaUint32T plms_plmc_instantiated_process(PLMS_ENTITY *ent,
				PLMS_PLMC_EE_OS_INFO *os_info)
{
	SaUint32T ret_err = NCSCC_RC_SUCCESS;

	TRACE_ENTER2("Entity: %s",ent->dn_name_str);
	
	/* If the admin state of the ent is lckinst, then ignore the 
	state as we cannot terminate the entity at this point of time.*/
	if ( (SA_PLM_EE_ADMIN_LOCKED_INSTANTIATION == ent->entity.ee_entity.saPlmEEAdminState) || 
	((NULL != ent->parent) && (plms_is_rdness_state_set(ent->parent,SA_PLM_READINESS_OUT_OF_SERVICE))) || 
	(!plms_min_dep_is_ok(ent))){
		
		LOG_ER("EE insted ignored, as its state or parent/dep state not proper:%s", ent->dn_name_str);
		TRACE_LEAVE2("Return Val: %d",NCSCC_RC_FAILURE);
		return NCSCC_RC_FAILURE; 	
	}

	/* Verify the EE if the presence state is not instantiating.*/
	if ( (SA_PLM_EE_PRESENCE_INSTANTIATING != 
		ent->entity.ee_entity.saPlmEEPresenceState) &&
		(SA_PLM_EE_PRESENCE_INSTANTIATED != 
		ent->entity.ee_entity.saPlmEEPresenceState)){

		TRACE("EE is not yet verified, verify the ent: %s",
		ent->dn_name_str);
		/* Verify the EE Os.*/
		ret_err = plms_ee_verify(ent,os_info);
		plms_os_info_free(os_info);
		/* If verification failed then ignore the EE.*/
		if (NCSCC_RC_SUCCESS != ret_err){
			LOG_ER("EE Verification FAILED for ent: %s",
							ent->dn_name_str);	
			return ret_err;	
		}
		TRACE("EE verification is SUCCESSFUL for ent: %s",
		ent->dn_name_str);
	}else{
		TRACE("EE is already verified. Ent: %s",ent->dn_name_str);
	}
	
	/* Verification is successful. Update the presence state.*/
	plms_presence_state_set(ent,
				SA_PLM_EE_PRESENCE_INSTANTIATED,
				NULL,SA_NTF_OBJECT_OPERATION,
				SA_PLM_NTFID_STATE_CHANGE_ROOT);

	/*Although this is the instantiated message, but we are not
	depending upon it. We are stopping the instantiation-failed timer
	only after getting the TCP connection.*/

	/*TODO: If the entity is in disable state and the isolation reason is
	terminated, then should I take the entity to enable state?*/

	TRACE_LEAVE2("Return Val: %d",ret_err);
	return ret_err;
}
/******************************************************************************
@brief		: Process terminating event from PLMC.
@param[in]	: ent - EE.
@return		: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
******************************************************************************/
SaUint32T plms_plmc_terminating_process(PLMS_ENTITY *ent)
{
	SaUint32T ret_err = NCSCC_RC_SUCCESS;

	TRACE_ENTER2("Entity: %s",ent->dn_name_str);
	
	if ( SA_PLM_EE_PRESENCE_UNINSTANTIATED == 
		ent->entity.ee_entity.saPlmEEPresenceState){
		
		TRACE("Ent %s is already in uninstantiated state.",
		ent->dn_name_str);
		
		TRACE_LEAVE2("Return Val: %d",ret_err);
		return ret_err;
	}
	
	if ( SA_PLM_EE_PRESENCE_TERMINATING != 
		ent->entity.ee_entity.saPlmEEPresenceState){		
		plms_presence_state_set(ent,
				SA_PLM_EE_PRESENCE_TERMINATING,
				NULL,SA_NTF_OBJECT_OPERATION,
				SA_PLM_NTFID_STATE_CHANGE_ROOT);
	}
	
	/* Start the termination-failed timer if not started.*/
	if (!(ent->tmr.timer_id)){
		ent->tmr.tmr_type = PLMS_TMR_EE_TERMINATING;
		ret_err = plms_timer_start(&ent->tmr.timer_id,ent,
		ent->entity.ee_entity.saPlmEETerminateTimeout);
		if (NCSCC_RC_SUCCESS != ret_err){
			ent->tmr.timer_id = 0;
			ent->tmr.tmr_type = PLMS_TMR_NONE;
			ent->tmr.context_info = NULL;
		}
	}else if (PLMS_TMR_EE_INSTANTIATING == ent->tmr.tmr_type){
		/* Stop the inst timer and start the term timer.*/
		plms_timer_stop(ent);
		ent->tmr.tmr_type = PLMS_TMR_EE_TERMINATING;
		ret_err = plms_timer_start(&ent->tmr.timer_id,ent,
		ent->entity.ee_entity.saPlmEETerminateTimeout);
		if (NCSCC_RC_SUCCESS != ret_err){
			ent->tmr.timer_id = 0;
			ent->tmr.tmr_type = PLMS_TMR_NONE;
			ent->tmr.context_info = NULL;
		}
	} else {
		/* The timer is already running.*/
	}

	TRACE_LEAVE2("Return Val: %d",ret_err);
	return ret_err;
}
/******************************************************************************
@brief		: Process terminated event.
		I can not depend on this PLMC message as for abrupt
		termination, I will not get this. Anyway this is UDP.
		So do the corresponding processing on getting tcp-disconnect
		message for this EE.
		
******************************************************************************/
SaUint32T plms_plmc_terminated_process(PLMS_ENTITY *ent)
{
	SaUint8T ret_err = NCSCC_RC_SUCCESS;

	TRACE_ENTER2("Entity: %s",ent->dn_name_str);
	
	if (SA_PLM_EE_PRESENCE_UNINSTANTIATED != 
		ent->entity.ee_entity.saPlmEEPresenceState){
		plms_presence_state_set(ent,
				SA_PLM_EE_PRESENCE_UNINSTANTIATED,
				NULL,SA_NTF_OBJECT_OPERATION,
				SA_PLM_NTFID_STATE_CHANGE_ROOT);
	}
	
	/* Stop the termination-failed timer if running.*/
	if (ent->tmr.timer_id && 
		(PLMS_TMR_EE_TERMINATING == ent->tmr.tmr_type)) {
		plms_timer_stop(ent);
	}
	
	/* Take care of clearing management lost flag if required.*/
	if (plms_rdness_flag_is_set(ent,SA_PLM_RF_MANAGEMENT_LOST))
		plms_ee_terminated_mngt_flag_clear(ent);

	TRACE_LEAVE2("Return Val: %d",ret_err);
	return ret_err; 
}
/******************************************************************************
@brief		: Process tcp-connect msg for the EE.
		Unlock the EE, if successful take the EE to insvc if all other
		conditions are matched.

		If the EE is not yet verififed, then request for the OS-info.
		After getting the OS info, verify and unlock the EE and take
		the EE to insvc.

		If the admin state is lock-instantiated then terminate the EE.
@param[in]	: ent - EE
@return		: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
******************************************************************************/
SaUint32T plms_plmc_tcp_connect_process(PLMS_ENTITY *ent)
{
	SaUint32T ret_err = NCSCC_RC_SUCCESS;
	SaUint32T prev_adm_op = 0,is_set = 0;
	PLMS_TRACK_INFO *trk_info = NULL;
	PLMS_GROUP_ENTITY *aff_ent_list_flag = NULL;
	PLMS_CB *cb = plms_cb; 

	TRACE_ENTER2("Entity: %s",ent->dn_name_str);

	/* Stop the instantiate-failed timer if running.*/
	if (ent->tmr.timer_id && 
		(PLMS_TMR_EE_INSTANTIATING == ent->tmr.tmr_type)) {
		plms_timer_stop(ent);
	}
	
	/* Clear management lost flag.*/
	if(plms_rdness_flag_is_set(ent,SA_PLM_RF_MANAGEMENT_LOST)){ 
		if( NCSCC_RC_SUCCESS != plms_tcp_connect_mngt_flag_clear(ent))
			return NCSCC_RC_FAILURE;
	}
	/* Socket flip-flop. No need to process, return from here.*/
	if (ent->ee_sock_ff){
		TRACE("Socket connection Restored. EE: %s",ent->dn_name_str);
		ent->ee_sock_ff = FALSE;
		return NCSCC_RC_SUCCESS;
	}
	
	/* If the admin state of the ent is lckinst, terminate the EE. 
	OH, if my parent is in OOS, then also terminate the EE.*/ 
	if ( (SA_PLM_EE_ADMIN_LOCKED_INSTANTIATION == ent->entity.ee_entity.saPlmEEAdminState) || 
	((NULL != ent->parent) && (plms_is_rdness_state_set(ent->parent,SA_PLM_READINESS_OUT_OF_SERVICE))) || 
	(!plms_min_dep_is_ok(ent))){
		
		/* Check if the dependency flag can be set. If yes then set the dependency flag and
		call appropiate callback with cause as .*/
		plms_is_dep_set_cbk_call(ent);

		LOG_ER("Term the entity, as the admin state is lckinst or parent/dep is in OOS. Ent: %s",
		ent->dn_name_str);
		ret_err = plmc_sa_plm_admin_lock_instantiation(ent->dn_name_str,
		plms_plmc_tcp_cbk);
		
		if(ret_err){
			if (!plms_rdness_flag_is_set(ent,
				SA_PLM_RF_MANAGEMENT_LOST)){
				
				/* Set management lost flag.*/
				plms_readiness_flag_mark_unmark(ent,
				SA_PLM_RF_MANAGEMENT_LOST,1/*mark*/,
				NULL,SA_NTF_MANAGEMENT_OPERATION,
				SA_PLM_NTFID_STATE_CHANGE_ROOT);

				/* Call managemnet lost callback.*/
				plms_mngt_lost_clear_cbk_call(ent,1/*mark*/);
			}
			ent->mngt_lost_tri = PLMS_MNGT_EE_TERM;
		}
		TRACE_LEAVE2("Return Val: %d",NCSCC_RC_FAILURE);
		return NCSCC_RC_FAILURE; 	
	}

	if (plms_is_rdness_state_set(ent,SA_PLM_READINESS_IN_SERVICE)){
		TRACE("Ent %s is already in insvc.",ent->dn_name_str);
		return NCSCC_RC_SUCCESS;
	}
	/*If previous state is not instantiating/intantiated, then get os info
	and verify. If verification failed, then return.*/
	if ((SA_PLM_EE_PRESENCE_INSTANTIATED != 
		ent->entity.ee_entity.saPlmEEPresenceState) && 
		(SA_PLM_EE_PRESENCE_INSTANTIATING != 
		ent->entity.ee_entity.saPlmEEPresenceState)){

		TRACE("Get the OS info to verify the entity: %s",
							ent->dn_name_str);
		
#if 0		
		/* I have missed instantiated as well as intantiating
		but as tcp connection is established, the EE must be in
		instantiated state.*/
		plms_presence_state_set(ent,
				SA_PLM_EE_PRESENCE_INSTANTIATED,
				NULL,SA_NTF_OBJECT_OPERATION,
				SA_PLM_NTFID_STATE_CHANGE_ROOT);
#endif			
		ret_err = plmc_get_os_info(ent->dn_name_str,plms_plmc_tcp_cbk);
		if (ret_err){
			LOG_ER("Request to get OS info FAILED for ent: %s",
							ent->dn_name_str);
			plms_readiness_flag_mark_unmark(ent,
			SA_PLM_RF_MANAGEMENT_LOST,1/*mark*/,NULL,
			SA_NTF_OBJECT_OPERATION,
			SA_PLM_NTFID_STATE_CHANGE_ROOT);

			ent->mngt_lost_tri = PLMS_MNGT_EE_GET_OS_INFO;
			plms_mngt_lost_clear_cbk_call(ent,1/*mark*/);

			TRACE_LEAVE2("Return Val: %d",NCSCC_RC_FAILURE);
			return NCSCC_RC_FAILURE;

		}
		/* On getting the OS info, verify the EE. If the verification
		is successful, start the services if the EE is in unlocked
		state and make an attempt to take the entity to insvc.*/
		TRACE_LEAVE2("Return Val: %d",ret_err);
		return ret_err;
	}
	
	/* I missed INSTANTIATED.*/
	if (SA_PLM_EE_PRESENCE_INSTANTIATED != 
		ent->entity.ee_entity.saPlmEEPresenceState){
		
		TRACE("PLMS missed instantiated for the entity. Set the \
		Presence state to instantiated for ent: %s",ent->dn_name_str);
		plms_presence_state_set(ent,
					SA_PLM_EE_PRESENCE_INSTANTIATED,
					NULL,SA_NTF_OBJECT_OPERATION,
					SA_PLM_NTFID_STATE_CHANGE_ROOT);

	}

	plms_op_state_set(ent,SA_PLM_OPERATIONAL_ENABLED,NULL,
	SA_NTF_OBJECT_OPERATION,SA_PLM_NTFID_STATE_CHANGE_ROOT);
			
	ent->iso_method = PLMS_ISO_DEFAULT;

	/* Set the dep-imminet-failure flag if required.*/
	plms_dep_immi_flag_check_and_set(ent,&aff_ent_list_flag,&is_set);
	
	/* Fault clear case. Admin repair and readiness impact.*/
	if ((NULL != ent->trk_info) && (SA_PLM_CAUSE_FAILURE_CLEARED == ent->trk_info->track_cause)){
		prev_adm_op = 1;
		trk_info = ent->trk_info;
		ent->trk_info = NULL;
		if (SA_PLM_ADMIN_REPAIRED == trk_info->imm_adm_opr_id){
			trk_info->root_entity->adm_op_in_progress = FALSE;
			trk_info->root_entity->am_i_aff_ent = FALSE;
			ret_err = saImmOiAdminOperationResult(cb->oi_hdl,trk_info->inv_id, SA_AIS_OK);
		}
	/* Admin RESTART amd Admin RESET case.*/	
	}else if (NULL != ent->trk_info /* && ent->am_i_aff_ent*/){
		
		prev_adm_op = 1;
		trk_info = ent->trk_info;
		ent->trk_info = NULL;
		trk_info->track_count--;
		
		TRACE("Perform the in progress admin op for entity: %s,\
		adm-op: %d",ent->dn_name_str,
		trk_info->root_entity->adm_op_in_progress);
		
		if (!(trk_info->track_count)){
			/* Clean up the trk_info.*/
			trk_info->root_entity->adm_op_in_progress = FALSE;
			trk_info->root_entity->am_i_aff_ent = FALSE;
			plms_aff_ent_flag_mark_unmark(
					trk_info->aff_ent_list,FALSE);
			
			plms_ent_list_free(trk_info->aff_ent_list);	
			trk_info->aff_ent_list = NULL;
			
			/* Send response to IMM.
			TODO: Am I sending this a bit earlier?
			*/
			ret_err = saImmOiAdminOperationResult( cb->oi_hdl,
					trk_info->inv_id, SA_AIS_OK);
			if (NCSCC_RC_SUCCESS != ret_err){
				LOG_ER("Adm operation result sending to IMM \
				failed for ent: %s,imm-ret-val: %d",
				trk_info->root_entity->dn_name_str,ret_err);
			}
			TRACE("Adm operation result sending to IMM successful \
			for ent: %s",trk_info->root_entity->dn_name_str);
		}
	}
		
	if (SA_PLM_EE_ADMIN_UNLOCKED != 
		ent->entity.ee_entity.saPlmEEAdminState){
	
		TRACE("Admin state of the entity %s is not unlocked, not \
		starting the required services.",ent->dn_name_str);
		/* As the admin state is not unlocked, this entity
		can not be moved to insvc. Return.*/
		
		if ( prev_adm_op && !(trk_info->track_count))
			plms_trk_info_free(trk_info);

		if (is_set){
			plms_insted_dep_immi_failure_cbk_call(ent,aff_ent_list_flag);
		}

		TRACE_LEAVE2("Return Val: %d",ret_err);
		return NCSCC_RC_SUCCESS;
	}else{
		ret_err = plms_ee_unlock(ent,FALSE,TRUE/*mngt_cbk*/);
		if (NCSCC_RC_SUCCESS != ret_err){
			LOG_ER("Request to unlock the entity %s FAILED",
							ent->dn_name_str);
			TRACE_LEAVE2("Return Val: %d",ret_err);
			return ret_err;
		}
		TRACE("Request to unlock the EE: %s SUCCESSFUL",
							ent->dn_name_str);

		/* Make an attempt to make the readiness state
		to insvc.*/
		ret_err = plms_plmc_unlck_insvc(ent,trk_info,aff_ent_list_flag,is_set);
	}
	TRACE_LEAVE2("Return Val: %d",ret_err);
	return ret_err;
}
/******************************************************************************
@brief		: Process tcp-disconnect msg for the EE. Move the EE to OOS
		if required.
@param[in]	: ent - EE
@return		: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
******************************************************************************/
SaUint32T plms_plmc_tcp_disconnect_process(PLMS_ENTITY *ent)
{
	SaUint32T ret_err = NCSCC_RC_SUCCESS;
	PLMS_GROUP_ENTITY *aff_ent_list = NULL;
	PLMS_GROUP_ENTITY *head;
	PLMS_TRACK_INFO trk_info;
	PLMS_ENTITY_GROUP_INFO_LIST *log_head_grp;

	TRACE_ENTER2("Entity: %s",ent->dn_name_str);

	/* If the termination-failed timer is running then stop 
	the timer.*/
	if (ent->tmr.timer_id && 
		(PLMS_TMR_EE_TERMINATING == ent->tmr.tmr_type)) {
		plms_timer_stop(ent);
	}
	/* Clear socket flip-flop flag.*/
	ent->ee_sock_ff = FALSE;
	
	if ( SA_PLM_EE_PRESENCE_UNINSTANTIATED != 
			ent->entity.ee_entity.saPlmEEPresenceState){
		TRACE("PLMS has not yet received uninstantiated for the entity.\
		 Set the Presence state to uninstantiated for ent: %s",
		ent->dn_name_str);
		plms_presence_state_set(ent,
					SA_PLM_EE_PRESENCE_UNINSTANTIATED,
					NULL,SA_NTF_OBJECT_OPERATION,
					SA_PLM_NTFID_STATE_CHANGE_ROOT);
	}
	
	/* Clear the management lost flag.*/
	if (plms_rdness_flag_is_set(ent,SA_PLM_RF_MANAGEMENT_LOST))
		plms_tcp_discon_mngt_flag_clear(ent);
	
	/* If the EE is already in OOS, then nothing to do.*/
	if (SA_PLM_READINESS_OUT_OF_SERVICE == 
		ent->entity.ee_entity.saPlmEEReadinessState){
		TRACE("Entity %s is already in OOS.",ent->dn_name_str);
	
	}else{ /* If the entity is in insvc, then got to make the entity 
		to move to OOS.*/
		/* Get all the affected entities.*/ 
		plms_affected_ent_list_get(ent,&aff_ent_list,0);
		
		plms_readiness_state_set(ent,SA_PLM_READINESS_OUT_OF_SERVICE,
					NULL,SA_NTF_OBJECT_OPERATION,
					SA_PLM_NTFID_STATE_CHANGE_ROOT);
		
		/* Mark the readiness state and expected readiness state  
		of all the affected entities to OOS and set the dependency 
		flag.*/
		head = aff_ent_list;
		while (head){
			plms_readiness_state_set(head->plm_entity,
					SA_PLM_READINESS_OUT_OF_SERVICE,
					ent,SA_NTF_OBJECT_OPERATION,
					SA_PLM_NTFID_STATE_CHANGE_DEP);
			plms_readiness_flag_mark_unmark(head->plm_entity,
					SA_PLM_RF_DEPENDENCY,TRUE,
					ent,SA_NTF_OBJECT_OPERATION,
					SA_PLM_NTFID_STATE_CHANGE_DEP);

			head = head->next;
		}

		
		plms_aff_ent_exp_rdness_state_ow(aff_ent_list);
		plms_ent_exp_rdness_state_ow(ent);

		head = aff_ent_list;
		while(head){
			
			if ((NULL != head->plm_entity->trk_info) && 
			(SA_PLM_CAUSE_EE_RESTART == 
			head->plm_entity->trk_info->track_cause)){
				head = head->next;
				continue;
			}
			
			/* Terminate all the dependent EEs.*/
			if (NCSCC_RC_SUCCESS != 
					plms_is_chld(ent,head->plm_entity)){
				ret_err = plms_ee_term(head->plm_entity,
							FALSE,TRUE/*mngt_cbk*/);
				if (NCSCC_RC_SUCCESS != ret_err){
					LOG_ER("Request for EE %s termination \
					FAILED",head->plm_entity->dn_name_str);
				}
			}	
			head = head->next;
		}
		
		memset(&trk_info,0,sizeof(PLMS_TRACK_INFO));
		trk_info.aff_ent_list = aff_ent_list;
		trk_info.group_info_list = NULL;
		/* Add the groups, root entity(ent) belong to.*/
		plms_ent_grp_list_add(ent, &(trk_info.group_info_list));
	
		/* Find out all the groups, all affected entities 
		 * belong to and add the groups to trk_info->group_info_list.*/ 
		plms_ent_list_grp_list_add(trk_info.aff_ent_list,
					&(trk_info.group_info_list));	

		TRACE("Affected groups for ent %s: ",ent->dn_name_str);
		log_head_grp = trk_info.group_info_list;
		while(log_head_grp){
			TRACE("%llu,",log_head_grp->ent_grp_inf->entity_grp_hdl);
			log_head_grp = log_head_grp->next;
		}

		trk_info.change_step = SA_PLM_CHANGE_COMPLETED;
		trk_info.root_correlation_id = SA_NTF_IDENTIFIER_UNUSED;
		trk_info.grp_op = SA_PLM_GROUP_MEMBER_READINESS_CHANGE;
		
		if ((NULL != ent->trk_info) && (SA_PLM_CAUSE_EE_RESTART
			                == ent->trk_info->track_cause)){
			plms_ent_to_ent_list_add(ent,&(trk_info.aff_ent_list));
			trk_info.root_entity = ent->trk_info->root_entity;
			trk_info.track_cause = ent->trk_info->track_cause;
			plms_cbk_call(&trk_info,0);
		}else{
			trk_info.root_entity = ent;
			trk_info.track_cause = SA_PLM_CAUSE_EE_UNINSTANTIATED;
			plms_cbk_call(&trk_info,1);
		}
	
		plms_ent_exp_rdness_status_clear(ent);
		plms_aff_ent_exp_rdness_status_clear(aff_ent_list);
		
		plms_ent_list_free(trk_info.aff_ent_list);
		trk_info.aff_ent_list = NULL;
		plms_ent_grp_list_free(trk_info.group_info_list);
		trk_info.group_info_list = NULL;
	}

	TRACE_LEAVE2("Return Val: %d",ret_err);
	return ret_err;
}
/******************************************************************************
@brief		: Process EE-unlock response from PLMC. If unlock is not
		successful, then set the management lost flag.
@param[in]	: ent - EE
@param[in]	: unlock_rc - return code for ee-unlock.
@return		: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
******************************************************************************/
SaUint32T plms_plmc_unlock_response(PLMS_ENTITY *ent,SaUint32T unlock_rc)
{
	SaUint32T ret_err = NCSCC_RC_SUCCESS;

	TRACE_ENTER2("Entity: %s",ent->dn_name_str);

	if (PLMS_PLMC_SUCCESS == unlock_rc){
		/* I am happy as I have already made an attempt to
		take this entity to insvc*/
		TRACE("Unlock SUCCESSFUL for ent %s",ent->dn_name_str);
		
		/* Clear the management lost flag only if set for unlock.*/
		if (plms_rdness_flag_is_set(ent,SA_PLM_RF_MANAGEMENT_LOST)){
			plms_unlck_resp_mngt_flag_clear(ent);
		}
	}else{
		/* TODO: Set management lost flag. I do not know why the
		EE was unlocked. So I am not setting admin-op-pending
		flag.*/
		plms_readiness_flag_mark_unmark(ent,SA_PLM_RF_MANAGEMENT_LOST,
						TRUE,NULL,
						SA_NTF_MANAGEMENT_OPERATION,
						SA_PLM_NTFID_STATE_CHANGE_ROOT);

		ent->mngt_lost_tri = PLMS_MNGT_EE_UNLOCK;

		LOG_ER("Unlock FAILED for ent %s",ent->dn_name_str);
		/* Call the callback.*/
		ret_err = plms_mngt_lost_clear_cbk_call(ent,1/*lost*/);

		if (ret_err != NCSCC_RC_SUCCESS){
			LOG_ER("PLMS to PLMA track callback event sending\
				FAILED, Operation: mngt_lost(unlock-failed),\
				ent: %s.",ent->dn_name_str);
		}else{
			TRACE("PLMS to PLMA track callback event sending \
				SUCCEEDED, Operation: mngt_lost(unlock-failed),\
				ent: %s.",ent->dn_name_str);
		}
	}
	TRACE_LEAVE2("Return Val: %d",ret_err);
	return ret_err;
}

/******************************************************************************
@brief		: Process EE-lock response from PLMC. If lock is not
		successful, then set the management lost flag.
@param[in]	: ent - EE
@param[in]	: lock_rc - return code for ee-lock.
@return		: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
******************************************************************************/
SaUint32T plms_plmc_lock_response(PLMS_ENTITY *ent,SaUint32T lock_rc)
{
	SaUint32T ret_err = NCSCC_RC_SUCCESS;
	TRACE_ENTER2("Entity: %s",ent->dn_name_str);

	if (PLMS_PLMC_SUCCESS == lock_rc){
		/* Nothing to do.*/
		TRACE("Lock SUCCESSFUL for ent %s",ent->dn_name_str);
		/* Clear the management lost flag only if set for unlock.*/
		if (plms_rdness_flag_is_set(ent,SA_PLM_RF_MANAGEMENT_LOST)){
			plms_lck_resp_mngt_flag_clear(ent);
		}
			
	}else{
		/* TODO: Set management lost flag. I do not know why the
		EE was locked. So I am not setting admin-op-pending
		flag.*/
		if (!plms_rdness_flag_is_set(ent,SA_PLM_RF_MANAGEMENT_LOST)){
			plms_readiness_flag_mark_unmark(ent,
						SA_PLM_RF_MANAGEMENT_LOST,
						TRUE,NULL,
						SA_NTF_MANAGEMENT_OPERATION,
						SA_PLM_NTFID_STATE_CHANGE_ROOT);
		

			LOG_ER("Lock FAILED for ent %s",ent->dn_name_str);
			/* Call callback.*/
			plms_mngt_lost_clear_cbk_call(ent,1/*lost*/);
		}
		
		ent->mngt_lost_tri = PLMS_MNGT_EE_LOCK;
	
	}
	TRACE_LEAVE2("Return Val: %d",ret_err);
	return ret_err;
}
/******************************************************************************
@brief		: Process EE-lckinst response from PLMC. If lock-inst is not
		successful, then set the management lost flag.
		Termination-failed timer is not stopped here. If the
		termination is successful, then it will be taken care on
		getting EE presence state events. If the termination fails,
		then it will be taken care on getting timer expiry event.
@param[in]	: ent - EE
@param[in]	: lckinst_rc - return code for ee-lckinst.
@return		: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
******************************************************************************/
SaUint32T plms_plmc_lckinst_response(PLMS_ENTITY *ent,SaUint32T lckinst_rc)
{
	SaUint32T ret_err = NCSCC_RC_SUCCESS;
	TRACE_ENTER2("Entity: %s",ent->dn_name_str);

	if (PLMS_PLMC_SUCCESS == lckinst_rc){
		/*Nothing to do.*/
		TRACE("Lock-inst SUCCESSFUL for ent %s",ent->dn_name_str);
		/* Clear the management lost flag only if set for unlock.*/
		if (plms_rdness_flag_is_set(ent,SA_PLM_RF_MANAGEMENT_LOST)){
			plms_lckinst_resp_mngt_flag_clear(ent);
		}
	}else{
		/* TODO: Set management lost flag. I do not know why the
		EE was terminated. So I am not setting admin-op-pending
		flag.*/
		if (!plms_rdness_flag_is_set(ent,SA_PLM_RF_MANAGEMENT_LOST)){
			plms_readiness_flag_mark_unmark(ent,
						SA_PLM_RF_MANAGEMENT_LOST,
						TRUE,NULL,
						SA_NTF_MANAGEMENT_OPERATION,
						SA_PLM_NTFID_STATE_CHANGE_ROOT);

		
			LOG_ER("Lock-inst FAILED for ent %s",ent->dn_name_str);
			/* Call the callback.*/
			plms_mngt_lost_clear_cbk_call(ent,1/*lost*/);
		}

		ent->mngt_lost_tri = PLMS_MNGT_EE_TERM;
	}

	TRACE_LEAVE2("Return Val: %d",ret_err);
	return ret_err;
}
/******************************************************************************
@brief		: Process EE-restart response from PLMC. If restart is not
		successful, then set the management lost flag.
@param[in]	: ent - EE
@param[in]	: lckinst_rc - return code for ee-restart.
@return		: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
******************************************************************************/
SaUint32T plms_plmc_restart_response(PLMS_ENTITY *ent,SaUint32T restart_rc)
{
	SaUint32T ret_err = NCSCC_RC_SUCCESS;
	TRACE_ENTER2("Entity: %s",ent->dn_name_str);

	if (PLMS_PLMC_SUCCESS == restart_rc){
		/*Nothing to do.*/
		LOG_ER("Restart SUCCESSFUL for ent %s",ent->dn_name_str);
		
		/* Clear the management lost flag only if set for unlock.*/
		if (plms_rdness_flag_is_set(ent,SA_PLM_RF_MANAGEMENT_LOST)){
			plms_restart_resp_mngt_flag_clear(ent);
		}
	}else{
		/* TODO: Set management lost flag. I do not know why the
		EE was restarted. So I am not setting admin-op-pending
		flag.*/
		if (!plms_rdness_flag_is_set(ent,SA_PLM_RF_MANAGEMENT_LOST)){
			plms_readiness_flag_mark_unmark(ent,
						SA_PLM_RF_MANAGEMENT_LOST,
						TRUE,NULL,
						SA_NTF_MANAGEMENT_OPERATION,
						SA_PLM_NTFID_STATE_CHANGE_ROOT);

		
			LOG_ER("Restart FAILED for ent %s",ent->dn_name_str);
			/* Call the callback.*/
			plms_mngt_lost_clear_cbk_call(ent,1/*lost*/);
		}
		
		ent->mngt_lost_tri = PLMS_MNGT_EE_RESTART;

	}

	TRACE_LEAVE2("Return Val: %d",ret_err);
	return ret_err;
}
/******************************************************************************
@brief		: Process the error reported by PLMC.
@param[in]	: ent - EE
@param[in]	: err_info - PLMC reported error.  
@return		: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
******************************************************************************/
SaUint32T plms_plmc_err_cbk_proc(PLMS_ENTITY *ent,PLMS_PLMC_ERR err_info)
{
	TRACE_ENTER2("Entity: %s",ent->dn_name_str);
	/* This is the case when plmcd is killed. No need to process this
	as we are expecting tcp callback.*/
	if (PLMC_LIBACT_CLOSE_SOCKET == err_info.acode){
		if (PLMC_LIBERR_LOST_CONNECTION == err_info.ecode){
			return NCSCC_RC_SUCCESS;
		}else{
			/* Case of flip-flop. Mark the flag. If the conntection is
			restored or the EE is terminated, clear this flag.*/
			ent->ee_sock_ff = TRUE;
		}
	}
	
	/* For any other error, set management lost flag. Admin pending flag is
	not set, as PLMS is not in admin context.*/
	if (!plms_rdness_flag_is_set(ent,SA_PLM_RF_MANAGEMENT_LOST)){ 
		/*Set management lost flag. */
		plms_readiness_flag_mark_unmark(ent, SA_PLM_RF_MANAGEMENT_LOST,
		TRUE,NULL,SA_NTF_OBJECT_OPERATION, SA_PLM_NTFID_STATE_CHANGE_ROOT);
		
		/* Call the callback.*/
		plms_mngt_lost_clear_cbk_call(ent,1/*lost*/);
	}
	TRACE_LEAVE2("Return Val: %d",NCSCC_RC_SUCCESS);
	return NCSCC_RC_SUCCESS;
}
/******************************************************************************
@brief		:Process get_prot_ver response.
		TODO: Not implemented for release-1. 
******************************************************************************/
SaUint32T plms_plmc_get_prot_ver_response(PLMS_ENTITY *ent,
					SaUint8T *prot_ver)
{
	TRACE_ENTER2("Not implemented, Entity: %s",ent->dn_name_str);
	TRACE_LEAVE2("Not implemented, Return Val: %d",NCSCC_RC_SUCCESS);
	return NCSCC_RC_SUCCESS;
}
/******************************************************************************
@brief		: Process get_os_info response. Verify the OS info. If verification
		is successful, unlock the EE and change the readiness state to 
		insvc.
@param[in]	: ent - EE
@param[in]	: os_info - OS info provided by PLMC to be verified.
@return		: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
******************************************************************************/
SaUint32T plms_plmc_get_os_info_response(PLMS_ENTITY *ent,
			PLMS_PLMC_EE_OS_INFO *os_info)
{
	SaUint32T ret_err = NCSCC_RC_SUCCESS;
	SaUint32T prev_adm_op = 0,is_set = 0;
	PLMS_TRACK_INFO *trk_info = NULL;
	PLMS_GROUP_ENTITY *aff_ent_list_flag = NULL;
	PLMS_CB *cb = plms_cb;	
	
	TRACE_ENTER2("Entity: %s",ent->dn_name_str);

	if (NULL != os_info){
	
		/* Clear management lost flag.*/
		if(plms_rdness_flag_is_set(ent,SA_PLM_RF_MANAGEMENT_LOST)){ 
			if(NCSCC_RC_SUCCESS != plms_os_info_resp_mngt_flag_clear(ent)){
				return NCSCC_RC_FAILURE;
			}
		}
		ret_err = plms_ee_verify(ent,os_info);
		plms_os_info_free(os_info);
		if (NCSCC_RC_SUCCESS != ret_err){
			/* Verification failed. Ignore and return from here.*/
			LOG_ER("OS verification FAILED for EE: %s",ent->dn_name_str);
			TRACE_LEAVE2("Return Val: %d",ret_err);
			return ret_err;
		}else{
			TRACE("OS verification SUCCESSFUL for EE: %s",
						ent->dn_name_str);
			
			plms_presence_state_set(ent,
					SA_PLM_EE_PRESENCE_INSTANTIATED,
					NULL,SA_NTF_OBJECT_OPERATION,
					SA_PLM_NTFID_STATE_CHANGE_ROOT);
			
			plms_op_state_set(ent,SA_PLM_OPERATIONAL_ENABLED,NULL,
			SA_NTF_OBJECT_OPERATION,SA_PLM_NTFID_STATE_CHANGE_ROOT);

			ent->iso_method = PLMS_ISO_DEFAULT;
			
			/* Set the dep-imminet-failure flag if required.*/
			plms_dep_immi_flag_check_and_set(ent,&aff_ent_list_flag,&is_set);
			
			/* Fault clear case. Admin repair and readiness impact.*/
			if ((NULL != ent->trk_info) && (SA_PLM_CAUSE_FAILURE_CLEARED == ent->trk_info->track_cause)){
				prev_adm_op = 1;
				trk_info = ent->trk_info;
				ent->trk_info = NULL;
				if (SA_PLM_ADMIN_REPAIRED == trk_info->imm_adm_opr_id){
					trk_info->root_entity->adm_op_in_progress = FALSE;
					trk_info->root_entity->am_i_aff_ent = FALSE;
					ret_err = saImmOiAdminOperationResult(cb->oi_hdl,trk_info->inv_id, SA_AIS_OK);
				}
			/* Admin RESTART amd Admin RESET case.*/	
			}else if (NULL != ent->trk_info /* && ent->am_i_aff_ent*/){
				prev_adm_op = 1;
				trk_info = ent->trk_info;
				ent->trk_info = NULL;
				trk_info->track_count--;
				TRACE("Perform the in progress admin op for entity: %s, adm-op: %d",ent->dn_name_str,
				trk_info->root_entity->adm_op_in_progress);
				
				if (!(trk_info->track_count)){
					/* Clean up the trk_info.*/
					trk_info->root_entity->adm_op_in_progress = FALSE;
					trk_info->root_entity->am_i_aff_ent = FALSE;
					plms_aff_ent_flag_mark_unmark(trk_info->aff_ent_list,FALSE);
					
					plms_ent_list_free(trk_info->aff_ent_list);	
					trk_info->aff_ent_list = NULL;
					
					/* Send response to IMM.
					TODO: Am I sending this a bit earlier?
					*/
					ret_err = saImmOiAdminOperationResult(cb->oi_hdl,trk_info->inv_id,SA_AIS_OK);
					if (NCSCC_RC_SUCCESS != ret_err){
						LOG_ER("Adm operation result sending to IMM failed for ent:\
						%s,imm-ret-val: %d", ent->dn_name_str,ret_err);
					}
					TRACE("Adm operation result sending to IMM successful for ent: %s", 
					ent->dn_name_str);
				}
			}
			
			if (SA_PLM_HE_ADMIN_UNLOCKED != 
				ent->entity.ee_entity.saPlmEEAdminState){

				TRACE("Admin state of the entity %s is not \
				unlocked, not starting the required services.",
				ent->dn_name_str);
				/* As the admin state is not unlocked, 
				this entity can not be moved to insvc. Return.*/
				
				if ( prev_adm_op && !(trk_info->track_count))
					plms_trk_info_free(trk_info);
				
				if (is_set){
					plms_insted_dep_immi_failure_cbk_call(ent,aff_ent_list_flag);
				}

				TRACE_LEAVE2("Return Val: %d",ret_err);
				return NCSCC_RC_SUCCESS;
			}else{
				
				ret_err = plms_ee_unlock(ent,FALSE,TRUE);
				if (NCSCC_RC_SUCCESS != ret_err){
					TRACE("Sending unlock request for ent: \
						%s FAILED", ent->dn_name_str);
					return ret_err;
				}
				TRACE("Sending unlock request for ent: \
					%s SUCCESSFUL", ent->dn_name_str);
				/* Make an attempt to make the readiness state
				to insvc.*/
				ret_err = plms_plmc_unlck_insvc(ent,trk_info,aff_ent_list_flag,is_set);
			}

		}
	}else{
		if (!plms_rdness_flag_is_set(ent,SA_PLM_RF_MANAGEMENT_LOST)){
			/*Set management lost flag. */
			plms_readiness_flag_mark_unmark(ent,
						SA_PLM_RF_MANAGEMENT_LOST,
						TRUE,NULL,
						SA_NTF_OBJECT_OPERATION,
						SA_PLM_NTFID_STATE_CHANGE_ROOT);
		
			/* Call the callback.*/
			plms_mngt_lost_clear_cbk_call(ent,1/*lost*/);
		}
		
		ent->mngt_lost_tri = PLMS_MNGT_EE_GET_OS_INFO;
	}

	TRACE_LEAVE2("Return Val: %d",ret_err);
	return ret_err;
}
/******************************************************************************
@brief		:Process osaf_start response.
		TODO: Not implemented for release-1. 
******************************************************************************/
SaUint32T plms_plmc_osaf_start_response(PLMS_ENTITY *ent,
			SaUint32T osaf_start_rc)
{
	TRACE_ENTER2("Not implemented, Entity: %s",ent->dn_name_str);
	TRACE_LEAVE2("Not implemented, Return Val: %d",NCSCC_RC_SUCCESS);
	return NCSCC_RC_SUCCESS;
}
/******************************************************************************
@brief		:Process osaf_stop response.
		TODO: Not implemented for release-1. 
******************************************************************************/
SaUint32T plms_plmc_osaf_stop_response(PLMS_ENTITY *ent,
			SaUint32T osaf_stop_rc)
{
	TRACE_ENTER2("Not implemented, Entity: %s",ent->dn_name_str);
	TRACE_LEAVE2("Not implemented, Return Val: %d",NCSCC_RC_SUCCESS);
	return NCSCC_RC_SUCCESS;
}
/******************************************************************************
@brief		:Process svc_start response.
		TODO: Not implemented for release-1. 
******************************************************************************/
SaUint32T plms_plmc_svc_start_response(PLMS_ENTITY *ent,
			SaUint32T svc_start_rc)
{
	TRACE_ENTER2("Not implemented, Entity: %s",ent->dn_name_str);
	TRACE_LEAVE2("Not implemented, Return Val: %d",NCSCC_RC_SUCCESS);
	return NCSCC_RC_SUCCESS;
}
/******************************************************************************
@brief		:Process svc_stop response.
		TODO: Not implemented for release-1. 
******************************************************************************/
SaUint32T plms_plmc_svc_stop_response(PLMS_ENTITY *ent,
			SaUint32T svc_stop_rc)
{
	TRACE_ENTER2("Not implemented, Entity: %s",ent->dn_name_str);
	TRACE_LEAVE2("Not implemented, Return Val: %d",NCSCC_RC_SUCCESS);
	return NCSCC_RC_SUCCESS;
}
/******************************************************************************
@brief		:Process plmcd restart response.
		TODO: Not implemented for release-1. 
******************************************************************************/
SaUint32T plms_plmc_plmd_restart_response(PLMS_ENTITY *ent,
			SaUint32T plmd_rest_rc)
{
	TRACE_ENTER2("Not implemented, Entity: %s",ent->dn_name_str);
	TRACE_LEAVE2("Not implemented, Return Val: %d",NCSCC_RC_SUCCESS);
	return NCSCC_RC_SUCCESS;
}
/******************************************************************************
@brief		:Process get ee_id response.
		TODO: Not implemented for release-1. 
******************************************************************************/
SaUint32T plms_plmc_ee_id_response(PLMS_ENTITY *ent,
			SaUint32T plmd_rest_rc)
{
	TRACE_ENTER2("Not implemented, Entity: %s",ent->dn_name_str);
	TRACE_LEAVE2("Not implemented, Return Val: %d",NCSCC_RC_SUCCESS);
	return NCSCC_RC_SUCCESS;
}

/******************************************************************************
@brief		: Plmc connect callbak. Put the evt into the plms MBX.
@param[in]	: ee_id - EE id.
@param[in]	: msg - msg representing tcp-connect/tcp-disconnect.
@return		: 0 for success.
		  1 for failure.
******************************************************************************/
int32 plms_plmc_connect_cbk(SaInt8T *ee_id,SaInt8T *msg)
{
	PLMS_EVT *evt;
	SaUint8T *tmp;
	SaUint32T len = 0;
	PLMS_CB *cb = plms_cb;

	TRACE_ENTER2("Entity: %s, msg: %s",ee_id,msg);

	/* TODO: Free this event after reading the msg from MBX and 
	 * processing.
	 * */	
	evt = (PLMS_EVT *)calloc(1,sizeof(PLMS_EVT));
	if(NULL == evt){
		LOG_CR("connect_cbk, calloc FAILED. Err no: %s",
						strerror(errno));
		assert(1);
	}
		
	evt->req_res = PLMS_REQ;
	evt->req_evt.req_type = PLMS_PLMC_EVT_T;
	/* Fill ee-id */
	tmp = (SaUint8T *)ee_id;
	while( '\0' != tmp[len] )
		len++;
	evt->req_evt.plms_plmc_evt.ee_id.length = len;
	memcpy(evt->req_evt.plms_plmc_evt.ee_id.value,ee_id,len);
	
	/* Fill the msg type. */
	if (  0 == strcmp(PLMC_CONN_CB_MSG,msg)){
		evt->req_evt.plms_plmc_evt.plmc_evt_type = 
					PLMS_PLMC_EE_TCP_CONCTED; 
	}else if ( 0 == strcmp(PLMC_DISCONN_CB_MSG,msg)){
		evt->req_evt.plms_plmc_evt.plmc_evt_type = 
					PLMS_PLMC_EE_TCP_DISCONCTED;
	}else{
		LOG_ER("Connect cbk with invalid msg, ent: %s, msg: %s",
							ee_id,msg);
	}	

	/* Post the evt to the PLMSv MBX. */
	if (NCSCC_RC_SUCCESS != m_NCS_IPC_SEND(&cb->mbx,evt,
					NCS_IPC_PRIORITY_HIGH))	{
		LOG_ER("Posting to MBX FAILED, ent: %s, msg: %s",ee_id,msg);
		TRACE_LEAVE2("Return Val: %d",FALSE);
		return 1;
	}
	TRACE("Posted to MBX, ent: %s, msg: %s",ee_id,msg);
	TRACE_LEAVE2("Return Val: %d",TRUE);
	return 0;
}
/******************************************************************************
@brief		: Plmc UDP callback. Put the evt into the plms MBX.
@param[in]	: msg - msg representing instantiating/instantiated/terminating/
		terminated events. 
@return		: 0 for success.
		  1 for failure.
******************************************************************************/
int32 plms_plmc_udp_cbk(udp_msg *msg)
{
	PLMS_EVT *evt;
	SaInt8T *os_info;
	SaUint32T len = 0;
	PLMS_CB *cb = plms_cb;
	
	TRACE_ENTER2("Entity: %s, msg: %s",msg->ee_id,msg->msg);
	
	/* TODO: Free this event after reading the msg from MBX and 
	 * processing.*/	
	evt = (PLMS_EVT *)calloc(1,sizeof(PLMS_EVT));
	if(NULL == evt){
		LOG_CR("udp_cbk, calloc FAILED. Err no: %s",
						strerror(errno));
		assert(0);
	}

	evt->req_res = PLMS_REQ;
	evt->req_evt.req_type = PLMS_PLMC_EVT_T;
	/* Fill ee-id */
	len = strlen(msg->ee_id);
	evt->req_evt.plms_plmc_evt.ee_id.length = len;
	memcpy(evt->req_evt.plms_plmc_evt.ee_id.value,msg->ee_id,len);

	switch(msg->msg_idx){
		
	case EE_INSTANTIATING:
		evt->req_evt.plms_plmc_evt.plmc_evt_type =
					PLMS_PLMC_EE_INSTING;
		break;
	
	case EE_TERMINATED:
		evt->req_evt.plms_plmc_evt.plmc_evt_type =
					PLMS_PLMC_EE_TRMTED;
		break;

	case EE_INSTANTIATED:
		evt->req_evt.plms_plmc_evt.plmc_evt_type =
					PLMS_PLMC_EE_INSTED;
		break;
	
	case EE_TERMINATING:
		evt->req_evt.plms_plmc_evt.plmc_evt_type =
					PLMS_PLMC_EE_TRMTING;
		break;
	
	default:
		LOG_ER("UDP cbk with invalid msg. ent: %s, msg %s",
						msg->ee_id,msg->msg);
		free(evt);
		return 1;
		
	}

	len = strlen(msg->os_info);
	os_info = (SaInt8T *)calloc(1,sizeof(SaInt8T)*(len+1));
	memcpy(os_info,msg->os_info,len+1);
	plms_os_info_parse(os_info,
			&(evt->req_evt.plms_plmc_evt.ee_os_info));
	free(os_info);
	os_info = NULL;
	
	/* Post the evt to the PLMSv MBX. */
	if (NCSCC_RC_SUCCESS != m_NCS_IPC_SEND(&cb->mbx,evt,
					NCS_IPC_PRIORITY_HIGH))	{
		LOG_ER("Posting to MBX FAILED, ent: %s, msg: %s",
						msg->ee_id,msg->msg);
		TRACE_LEAVE2("Return Val: %d",FALSE);
		return 1;
	}
	TRACE("Posted to MBX SUCCESSFUL, ent: %s, msg: %s",
						msg->ee_id,msg->msg);

	TRACE_LEAVE2("Return Val: %d",TRUE);
	return 0;	
}
/******************************************************************************
@brief		: Plmc TCP callback. Put the evt into the plms MBX.
@param[in]	: msg - msg representing tcp events.
@return		: 0 for success.
		  1 for failure.
******************************************************************************/
static int32 plms_plmc_tcp_cbk(tcp_msg *msg)
{
	PLMS_EVT *evt;
	SaInt8T *os_info;
	SaUint32T len;
	PLMS_CB *cb = plms_cb;
	
	TRACE_ENTER2("Entity: %s, msg: %s",msg->ee_id,msg->cmd);
	
	/* TODO: Free this event after reading the msg from MBX and 
	 * processing.*/	
	evt = (PLMS_EVT *)calloc(1,sizeof(PLMS_EVT));
	if(NULL == evt){
		LOG_CR("tcp_cbk, calloc FAILED. Err no: %s",
						strerror(errno));
		assert(0);
	}
	
	evt->req_res = PLMS_REQ;
	evt->req_evt.req_type = PLMS_PLMC_EVT_T;
	/* Fill the EE-id.*/			
	len = strlen(msg->ee_id);
	
	evt->req_evt.plms_plmc_evt.ee_id.length = len;
	memcpy(evt->req_evt.plms_plmc_evt.ee_id.value,msg->ee_id,len);
	
	switch(msg->cmd_enum){
	case PLMC_GET_ID_CMD:	
		evt->req_evt.plms_plmc_evt.plmc_evt_type = 
					PLMS_PLMC_EE_ID_RESP;
		break;
		
	case PLMC_GET_PROTOCOL_VER_CMD:	
		evt->req_evt.plms_plmc_evt.plmc_evt_type = 
					PLMS_PLMC_EE_VER_RESP;

		/* Get protocol ver. TODO: Free the memory allocated.*/	
		len = strlen(msg->result);
		evt->req_evt.plms_plmc_evt.ee_ver = (SaUint8T *)calloc(1,
						sizeof(SaUint8T)*(len+1)); 
		memcpy(evt->req_evt.plms_plmc_evt.ee_ver,msg->result,len+1);
		break;

	case PLMC_GET_OSINFO_CMD:	
		evt->req_evt.plms_plmc_evt.plmc_evt_type = 
					PLMS_PLMC_EE_OS_RESP;
					
		/* TODO: Parse the OS info and fill the evt structure. */	
		len = strlen(msg->result);
		os_info = (SaInt8T *)calloc(1,sizeof(SaInt8T)*(len+1));
		memcpy(os_info,msg->result,len+1);
		plms_os_info_parse(os_info,
				&(evt->req_evt.plms_plmc_evt.ee_os_info));
		free(os_info);
		os_info = NULL;
		break;
	case PLMC_SA_PLM_ADMIN_LOCK_INSTANTIATION_CMD:	
		evt->req_evt.plms_plmc_evt.plmc_evt_type = 
					PLMS_PLMC_EE_LOCK_INST_RESP;
		if (0 == strcmp(msg->result,PLMC_ACK_MSG_RECD))
			evt->req_evt.plms_plmc_evt.resp = PLMS_PLMC_SUCCESS;
		else
			evt->req_evt.plms_plmc_evt.resp = PLMS_PLMC_FAILURE;
		break;			
	case PLMC_SA_PLM_ADMIN_RESTART_CMD:	
		evt->req_evt.plms_plmc_evt.plmc_evt_type = 
					PLMS_PLMC_EE_RESTART_RESP;
		if (0 == strcmp(msg->result,PLMC_ACK_MSG_RECD))
			evt->req_evt.plms_plmc_evt.resp = PLMS_PLMC_SUCCESS;
		else
			evt->req_evt.plms_plmc_evt.resp = PLMS_PLMC_FAILURE;
		break;
		
	case PLMC_PLMCD_RESTART_CMD:
		evt->req_evt.plms_plmc_evt.plmc_evt_type = 
					PLMS_PLMC_PLMD_RESTART_RESP;
		if (0 == strcmp(msg->result,PLMC_ACK_MSG_RECD))
			evt->req_evt.plms_plmc_evt.resp = PLMS_PLMC_SUCCESS;
		else
			evt->req_evt.plms_plmc_evt.resp = PLMS_PLMC_FAILURE;
		break;
	
	case PLMC_SA_PLM_ADMIN_UNLOCK_CMD:
		evt->req_evt.plms_plmc_evt.plmc_evt_type = 
					PLMS_PLMC_EE_UNLOCK_RESP;
		if ( 0 == (strcmp(msg->result,PLMC_ACK_SUCCESS)))
			evt->req_evt.plms_plmc_evt.resp = PLMS_PLMC_SUCCESS;
		else
			/* TODO:FAILURE and TIMEOUT are considered as FAILURE.*/
			evt->req_evt.plms_plmc_evt.resp = PLMS_PLMC_FAILURE;
		break;

	case PLMC_SA_PLM_ADMIN_LOCK_CMD:
		evt->req_evt.plms_plmc_evt.plmc_evt_type =
					PLMS_PLMC_EE_LOCK_RESP;
		if ( 0 == (strcmp(msg->result,PLMC_ACK_SUCCESS)))
			evt->req_evt.plms_plmc_evt.resp = PLMS_PLMC_SUCCESS;
		else
			/* TODO:FAILURE and TIMEOUT are considered as FAILURE.*/
			evt->req_evt.plms_plmc_evt.resp = PLMS_PLMC_FAILURE;
		break;
	case PLMC_OSAF_START_CMD:
		evt->req_evt.plms_plmc_evt.plmc_evt_type = 
					PLMS_PLMC_OSAF_START_RESP;
		if ( 0 == (strcmp(msg->result,PLMC_ACK_SUCCESS)))
			evt->req_evt.plms_plmc_evt.resp = PLMS_PLMC_SUCCESS;
		else
			/* TODO:FAILURE and TIMEOUT are considered as FAILURE.*/
			evt->req_evt.plms_plmc_evt.resp = PLMS_PLMC_FAILURE;
		break;
		
	case PLMC_OSAF_STOP_CMD:
		evt->req_evt.plms_plmc_evt.plmc_evt_type = 
					PLMS_PLMC_OSAF_STOP_RESP;
		if ( 0 == (strcmp(msg->result,PLMC_ACK_SUCCESS)))
			evt->req_evt.plms_plmc_evt.resp = PLMS_PLMC_SUCCESS;
		else
			/* TODO:FAILURE and TIMEOUT are considered as FAILURE.*/
			evt->req_evt.plms_plmc_evt.resp = PLMS_PLMC_FAILURE;
		break;

	case PLMC_OSAF_SERVICES_START_CMD:
		evt->req_evt.plms_plmc_evt.plmc_evt_type = 
					PLMS_PLMC_SVC_START_RESP;
		if ( 0 == (strcmp(msg->result,PLMC_ACK_SUCCESS)))
			evt->req_evt.plms_plmc_evt.resp = PLMS_PLMC_SUCCESS;
		else
			/* TODO:FAILURE and TIMEOUT are considered as FAILURE.*/
			evt->req_evt.plms_plmc_evt.resp = PLMS_PLMC_FAILURE;
		break;
	case PLMC_OSAF_SERVICES_STOP_CMD:
		evt->req_evt.plms_plmc_evt.plmc_evt_type = 
					PLMS_PLMC_SVC_STOP_RESP;
		if ( 0 == (strcmp(msg->result,PLMC_ACK_SUCCESS)))
			evt->req_evt.plms_plmc_evt.resp = PLMS_PLMC_SUCCESS;
		else
			/* TODO:FAILURE and TIMEOUT are considered as FAILURE.*/
			evt->req_evt.plms_plmc_evt.resp = PLMS_PLMC_FAILURE;
		break;
	
	default:
		LOG_ER("tcp_cbk with invalid msg, ent: %s, msg: %s",
						msg->ee_id,msg->cmd);
		free(evt);
		TRACE_LEAVE2("Return Val: %d",FALSE);
		return 1;
	}
	
	/* Post the evt to the PLMSv MBX. */
	if (NCSCC_RC_SUCCESS != m_NCS_IPC_SEND(&cb->mbx,evt,
					NCS_IPC_PRIORITY_HIGH))	{
		LOG_ER("Posting to MBX FAILED, ent: %s, msg: %s",
						msg->ee_id,msg->cmd);
		TRACE_LEAVE2("Return Val: %d",FALSE);
		return 1;
	}
	TRACE("Posted to MBX SUCCESSFUL, ent: %s, msg: %s",
						msg->ee_id,msg->cmd);

	TRACE_LEAVE2("Return Val: %d",TRUE);
	return 0;
	
}
/******************************************************************************
@brief 		: There are five actions taken by plmc-lib on encountering 
		different kind of error. These are mentioned nelow.
		
		#define PLMC_LIBACT_UNDEFINED           0
		#define PLMC_LIBACT_CLOSE_SOCKET        1
		#define PLMC_LIBACT_EXIT_THREAD         2
		#define PLMC_LIBACT_DESTROY_LIBRARY     3
		#define PLMC_LIBACT_IGNORING            4

		At this point of time, PLMC_LIBACT_CLOSE_SOCKET is handled
		through tcp callback.
		PLMC_LIBACT_UNDEFINED and PLMC_LIBACT_IGNORING are not handled.
		
		PLMC_LIBACT_EXIT_THREAD and PLMC_LIBACT_DESTROY_LIBRARY are handled
		in this function. As the action is exit(0), no need to process these
		events in PLMS MBX context. If situation arises for any other errors,
		it will be taken care then.
******************************************************************************/
int32 plms_plmc_error_cbk(plmc_lib_error *msg)
{
	PLMS_EVT *evt;
	PLMS_CB *cb = plms_cb;
	SaUint32T len = 0;
	
	TRACE_ENTER2("Cmd: %d, ee_id: %s, err_msg: %s, err_act: %s",
	msg->cmd_enum,msg->ee_id,msg->errormsg,msg->action);
	
	/* Only the action in which plmc-lib is destroyed, is
	handled. PLMS exits in this case an hence it makes no sense
	to put in the mailbox.*/
	
	if (PLMC_LIBACT_DESTROY_LIBRARY == msg->acode){
		LOG_ER("PLMS EXIT !!!!!! PLMC lib destroyed, I am helpless. PLMc act: %s",msg->action);
		exit(0);
	}
	if ('\0' == msg->ee_id[0]){
		TRACE("Not posting to MBX, as no ee_id. PLMS can not handle it.");
		TRACE_LEAVE2("Return Val: %d",TRUE);
		return 0;
	}
	/* For others put the msg to MBX.*/
	evt = (PLMS_EVT *)calloc(1,sizeof(PLMS_EVT));
	if(NULL == evt){
		LOG_CR("err_cbk, calloc FAILED. Err no: %s", strerror(errno));
		assert(0);
	}
	
	evt->req_res = PLMS_REQ;
	evt->req_evt.req_type = PLMS_PLMC_EVT_T;
	evt->req_evt.plms_plmc_evt.plmc_evt_type = PLMS_PLMC_ERR_CBK;

	/* Fill the EE-id.*/			
	len = strlen(msg->ee_id);
	evt->req_evt.plms_plmc_evt.ee_id.length = len;
	memcpy(evt->req_evt.plms_plmc_evt.ee_id.value,msg->ee_id,len);

	evt->req_evt.plms_plmc_evt.err_info.acode = msg->acode;
	evt->req_evt.plms_plmc_evt.err_info.ecode = msg->ecode;
	evt->req_evt.plms_plmc_evt.err_info.cmd = msg->cmd_enum; 
	
	/* Post the evt to the PLMSv MBX. */
	if (NCSCC_RC_SUCCESS != m_NCS_IPC_SEND(&cb->mbx,evt,NCS_IPC_PRIORITY_HIGH))	{
		LOG_ER("Posting to MBX FAILED, ent: %s", msg->ee_id);
		TRACE_LEAVE2("Return Val: %d",FALSE);
		return 1;
	}
	TRACE("Posted to MBX SUCCESSFUL, ent: %s.", msg->ee_id);

	TRACE_LEAVE2("Return Val: %d",TRUE);
	return 0;
}
/******************************************************************************
@brief		: Process PLMC MBX events.
@param[in]	: evt - PLMC events.
@return		: NCSCC_RC_FAILURE/NCSCC_RC_SUCCESS
******************************************************************************/
SaUint32T plms_plmc_mbx_evt_process(PLMS_EVT *evt)
{
	SaUint32T ret_err = NCSCC_RC_FAILURE;
	PLMS_ENTITY *ent = NULL;
	SaUint8T tmp[SA_MAX_NAME_LENGTH+1];
	PLMS_CB *cb = plms_cb;

	if (evt->req_evt.plms_plmc_evt.ee_id.length){
		ent = (PLMS_ENTITY *)ncs_patricia_tree_get(&(cb->entity_info),
			(SaUint8T *)&(evt->req_evt.plms_plmc_evt.ee_id));
		if (NULL == ent){
			memcpy(tmp,evt->req_evt.plms_plmc_evt.ee_id.value,
				 evt->req_evt.plms_plmc_evt.ee_id.length);
			tmp[evt->req_evt.plms_plmc_evt.ee_id.length] = '\0';

			LOG_ER (" Entity not found for PLMC event. ee_id: %s \
			,evt_type: %d",
			tmp,evt->req_evt.plms_plmc_evt.plmc_evt_type);
			return ret_err;
		}
	}else {
		LOG_ER("evt->req_evt.plms_plmc_evt.ee_id.length is ZERO");
		return ret_err;
	}


	switch(evt->req_evt.plms_plmc_evt.plmc_evt_type){
	case PLMS_PLMC_EE_INSTING:
		ret_err = plms_plmc_instantiating_process(ent,
		&(evt->req_evt.plms_plmc_evt.ee_os_info));
			
		break;
	case PLMS_PLMC_EE_INSTED:
		ret_err = plms_plmc_instantiated_process(ent,
		&(evt->req_evt.plms_plmc_evt.ee_os_info));
		break;
	case PLMS_PLMC_EE_TRMTING:
		ret_err = plms_plmc_terminating_process(ent);
		plms_os_info_free(&(evt->req_evt.plms_plmc_evt.ee_os_info));
		break;
	case PLMS_PLMC_EE_TRMTED:
		ret_err = plms_plmc_terminated_process(ent);
		plms_os_info_free(&(evt->req_evt.plms_plmc_evt.ee_os_info));
		break;
	case PLMS_PLMC_EE_TCP_CONCTED:
		ret_err = plms_plmc_tcp_connect_process(ent);
		break;
	case PLMS_PLMC_EE_TCP_DISCONCTED:
		ret_err = plms_plmc_tcp_disconnect_process(ent);
		break;
	case PLMS_PLMC_EE_VER_RESP:
		ret_err = plms_plmc_get_prot_ver_response(ent,
		evt->req_evt.plms_plmc_evt.ee_ver);
		break;
	case PLMS_PLMC_EE_OS_RESP:
		ret_err = plms_plmc_get_os_info_response(ent,
		&evt->req_evt.plms_plmc_evt.ee_os_info);
		break;
	case PLMS_PLMC_EE_LOCK_RESP:
		ret_err = plms_plmc_lock_response(ent,
		evt->req_evt.plms_plmc_evt.resp);
		break;
	case PLMS_PLMC_EE_UNLOCK_RESP:
		ret_err = plms_plmc_unlock_response(ent,
		evt->req_evt.plms_plmc_evt.resp);
		break;
	case PLMS_PLMC_EE_LOCK_INST_RESP:
		ret_err = plms_plmc_lckinst_response(ent,
		evt->req_evt.plms_plmc_evt.resp);
		break;
	case PLMS_PLMC_SVC_START_RESP:
		ret_err = plms_plmc_svc_start_response(ent,
		evt->req_evt.plms_plmc_evt.resp);
		break;
	case PLMS_PLMC_SVC_STOP_RESP:
		ret_err = plms_plmc_svc_stop_response(ent,
		evt->req_evt.plms_plmc_evt.resp);
		break;
	case PLMS_PLMC_OSAF_START_RESP:
		ret_err = plms_plmc_osaf_start_response(ent,
		evt->req_evt.plms_plmc_evt.resp);
		break;
	case PLMS_PLMC_OSAF_STOP_RESP:
		ret_err = plms_plmc_osaf_stop_response(ent,
		evt->req_evt.plms_plmc_evt.resp);
		break;
	case PLMS_PLMC_PLMD_RESTART_RESP:
		ret_err = plms_plmc_plmd_restart_response(ent,
		evt->req_evt.plms_plmc_evt.resp);
		break;
	case PLMS_PLMC_EE_ID_RESP:
		ret_err = plms_plmc_ee_id_response(ent,
		 evt->req_evt.plms_plmc_evt.resp);
		break;
	case PLMS_PLMC_EE_RESTART_RESP:
		ret_err = plms_plmc_restart_response(ent,
		evt->req_evt.plms_plmc_evt.resp);
		break;
	case PLMS_PLMC_ERR_CBK:
		ret_err = plms_plmc_err_cbk_proc(ent,evt->req_evt.plms_plmc_evt.err_info);
		break;
	default:
		break;
	}

	return ret_err;
}
/******************************************************************************
@brief		: Verify the OS info provided by PLMC against the configured
		OS info. If the optional parameters are not configured in 
		XML file, then those parameters are not taken into consideration
		for verification. version field is not optaional.
@param[in]	: ent - EE.
@param[in]	: os_info - PLMC provided OS info.
@retuen		: NCSCC_RC_FAILURE/NCSCC_RC_SUCCESS
******************************************************************************/
static SaUint32T plms_ee_verify(PLMS_ENTITY *ent, PLMS_PLMC_EE_OS_INFO *os_info)
{
	SaInt8T  *dn_ee_type,*dn_ee_type_ver,*dn_ee_base_type;
	SaNameT dn_ee_base_type_key;
	PLMS_EE_BASE_INFO *ee_base_type_node = NULL;
	SaStringT tmp_os_ver;
	PLMS_CB *cb = plms_cb;
	
	TRACE_ENTER2("Entity: %s",ent->dn_name_str);

	memset(&dn_ee_base_type_key,0,sizeof(SaNameT));
	
	dn_ee_type = (SaInt8T *)calloc(1,sizeof(SaInt8T)*
			(ent->entity.ee_entity.saPlmEEType.length + 1));
	if (NULL == dn_ee_type){
		LOG_CR("ee_verify, calloc FAILED.");
		TRACE_LEAVE2();
		assert(0);
	}
	
	dn_ee_type_ver= (SaInt8T *)calloc(1,sizeof(SaInt8T)*
			(ent->entity.ee_entity.saPlmEEType.length + 1));
	if (NULL == dn_ee_type_ver){
		LOG_CR("ee_verify, calloc FAILED.");
		TRACE_LEAVE2();
		assert(0);
	}

	memcpy(dn_ee_type,ent->entity.ee_entity.saPlmEEType.value,
			(ent->entity.ee_entity.saPlmEEType.length));
	dn_ee_type[ent->entity.ee_entity.saPlmEEType.length] = '\0';

	memcpy(dn_ee_type_ver,dn_ee_type,
			(ent->entity.ee_entity.saPlmEEType.length) +1 );

	/* Get the version.*/
	tmp_os_ver = strtok(dn_ee_type_ver,",");
	tmp_os_ver = strtok(tmp_os_ver,"=");
	tmp_os_ver = strtok(NULL,"=");
	if ( 0 != strcmp(tmp_os_ver,os_info->version)){
		LOG_ER("Version did not match, configured:%s,\
		PLMC-provided:%s", tmp_os_ver,os_info->version);
		free(dn_ee_type);
		free(dn_ee_type_ver);
		TRACE_LEAVE2();
		return NCSCC_RC_FAILURE;
	}
	
	/* Strip off the RDN of SaPlmEEType and get the DN of
	SaPlmEEBaseType.*/
	dn_ee_base_type = strstr(dn_ee_type,"safEEType");
	if (NULL == dn_ee_base_type){
		free(dn_ee_type);
		free(dn_ee_type_ver);
		TRACE_LEAVE2("EE Base Type is NULL.");
		return NCSCC_RC_FAILURE;
	}


	/* As the key for the EE base type patricia tree is of 
	SaNameT type, convert the string to the ksy type.*/
	dn_ee_base_type_key.length = strlen(dn_ee_base_type);
	memcpy(dn_ee_base_type_key.value,dn_ee_base_type,
				dn_ee_base_type_key.length); 

	/* Get the EEBaseType.*/
	ee_base_type_node = (PLMS_EE_BASE_INFO *)ncs_patricia_tree_get(
				&(cb->base_ee_info),
				(SaUint8T *)&(dn_ee_base_type_key));
	if (NULL == ee_base_type_node) {
		LOG_ER("EE Base Type node not found in patricia tree.");
		free(dn_ee_type);
		free(dn_ee_type_ver);
		TRACE_LEAVE2();
		return NCSCC_RC_FAILURE;
	}
	/* Match the product id.*/
	if (ee_base_type_node->ee_base_type.saPlmEetProduct){
		if (0 != strcmp(ee_base_type_node->ee_base_type.saPlmEetProduct,
							os_info->product)){
			LOG_ER("Product Id did not match, configured: %s,\
			PLMC-provided: %s",ee_base_type_node->ee_base_type.\
			saPlmEetProduct,os_info->product);
			free(dn_ee_type);
			free(dn_ee_type_ver);
			TRACE_LEAVE2();
			return NCSCC_RC_FAILURE;
		}
	}
	/* Match the vendor id.*/
	if(ee_base_type_node->ee_base_type.saPlmEetVendor){
		if (0 != strcmp(ee_base_type_node->ee_base_type.saPlmEetVendor,
							os_info->vendor)){
			LOG_ER("Vendor Id did not match, configured: %s,\
			PLMC-provided: %s",ee_base_type_node->ee_base_type.\
			saPlmEetVendor,os_info->vendor);
			free(dn_ee_type);
			free(dn_ee_type_ver);
			TRACE_LEAVE2();
			return NCSCC_RC_FAILURE;
		}
	}
	/* Match the release info.*/
	if (ee_base_type_node->ee_base_type.saPlmEetRelease){
		if (0 != strcmp(ee_base_type_node->ee_base_type.saPlmEetRelease,
							os_info->release)){
			LOG_ER("Release info did not match, configured:\
			%s, PLMC-provided: %s",ee_base_type_node->ee_base_type.\
			saPlmEetRelease,os_info->release);
			free(dn_ee_type);
			free(dn_ee_type_ver);
			TRACE_LEAVE2();
			return NCSCC_RC_FAILURE;
		}
	}
	/* Free.*/
	free(dn_ee_type);
	free(dn_ee_type_ver);
		
	TRACE("OS Verification successful for ent: %s", ent->dn_name_str);

	TRACE_LEAVE2("Return Val: %d",NCSCC_RC_SUCCESS);
	return NCSCC_RC_SUCCESS;
}
/******************************************************************************
@brief		: Parse the string os_info and fill evt_os_info. The string format
		of the OS info should be mentioned in plmcd.conf as given 
		below 
		version;product;vendor;release
@param[in]	: os_info - OS info in string format.
@param[out]	: parsed OS info.
@return		: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE		
******************************************************************************/
static SaUint32T plms_os_info_parse(SaInt8T *os_info, 
					PLMS_PLMC_EE_OS_INFO *evt_os_info)
{

	SaUint32T order=1,count;
	SaInt8T *token;
	SaUint8T cpy_cnt;
	SaInt8T *temp_os_info,*temp_os_info_free;

	temp_os_info = (SaInt8T *)calloc(1,sizeof(SaInt8T)*(strlen(os_info)+1));
	if (NULL == temp_os_info){
		LOG_CR("os_info_parse, calloc FAILED.");
		assert(0);
	}

	temp_os_info_free = temp_os_info;

	memcpy(temp_os_info,os_info,(strlen(os_info)+1));

	while(4 >= order){

		token = NULL;
		
		if ((temp_os_info[0] == ';') || ('\0' == temp_os_info[0])){
			token = (SaInt8T *)calloc(1,sizeof(SaInt8T));
			if (NULL == token){
				free(temp_os_info);
				plms_os_info_free(evt_os_info);
				LOG_CR("os_parse, calloc FAILED. Error: %s",
				strerror(errno));
				assert(0);
			}
			(token)[0] = '\0';

			if ( temp_os_info[0] == ';')
				temp_os_info = &temp_os_info[1];
		}else{
			count = 0;
			while( 1){
				if((';' == temp_os_info[count]) || 
				('\0' == temp_os_info[count])){
					break;
					}
				 count++;
			}
			token = (SaInt8T *)calloc(1,sizeof(SaInt8T)*
			(count + 1));

			if (NULL == token){
				free(temp_os_info);
				plms_os_info_free(evt_os_info);
				LOG_CR("os_parse, calloc FAILED. \
				Error: %s", strerror(errno));
				assert(0);
			}
			cpy_cnt = 0;
			while(cpy_cnt <= count-1){
				(token)[cpy_cnt] = temp_os_info[cpy_cnt];
				cpy_cnt++;
			}	
				
			(token)[count] = '\0';
			if ( '\0' != temp_os_info[count])
				temp_os_info = &temp_os_info[count+1];
			else
				temp_os_info = &temp_os_info[count];
		}

		switch(order){
		case 1:
			evt_os_info->version = token;
			break;
		case 2:
			evt_os_info->product = token;
			break;
		case 3:
			evt_os_info->vendor = token;
			break;
		case 4:
			evt_os_info->release = token;
			break;
		}
		order++;
	}
	/* Parsing done. Free the temp allocated buffer.*/
	free(temp_os_info_free);

return NCSCC_RC_SUCCESS;
}
/******************************************************************************
@brief		: Free os info.
@param[in/out]	: os_info - os info to be freed.
******************************************************************************/
void plms_os_info_free(PLMS_PLMC_EE_OS_INFO *os_info)
{
	TRACE("Free OS info.");
	/* Free the OS info.*/
	if (NULL != os_info->version){
		free(os_info->version);
		os_info->version = NULL;
	}
	if (NULL != os_info->product){
		free(os_info->product);
		os_info->product = NULL;
	}
	if (NULL != os_info->vendor){
		free(os_info->vendor);
		os_info->vendor = NULL;
	}
	if (NULL != os_info->release){
		free(os_info->release);
		os_info->release = NULL;
	}
	return;	
}
/******************************************************************************
@brief		: Make an attempt to take the EE to in service and if successful,
		do the same for its dependent and children. If in the process any
		change in readiness status, then call the readiness callback.
@param[in]	: ent - EE which is to be moved to in service.
@param[in]	: trk_info - used to send readiness callback.
@return 	: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
******************************************************************************/
static SaUint32T plms_plmc_unlck_insvc(PLMS_ENTITY *ent,
PLMS_TRACK_INFO *trk_info,PLMS_GROUP_ENTITY *aff_ent_list_flag,SaUint32T is_set)
{
	SaUint32T ret_err = NCSCC_RC_SUCCESS;
	PLMS_GROUP_ENTITY *aff_ent_list = NULL;
	PLMS_TRACK_INFO new_trk_info;
	PLMS_GROUP_ENTITY *log_head;
	PLMS_ENTITY_GROUP_INFO_LIST *log_head_grp;
	SaUint8T is_flag_aff = 0;

	TRACE_ENTER2("Entity: %s",ent->dn_name_str);
	/* Move the EE to InSvc is other conditions are matched.*/
	if (NCSCC_RC_SUCCESS == plms_move_ent_to_insvc(ent,&is_flag_aff)){
		TRACE("Make an attempt to make the children to insvc.");
		/* Move the children to insvc and clear the dependency flag.*/
		plms_move_chld_ent_to_insvc(ent->leftmost_child,
		&aff_ent_list,1,1);
		TRACE("Make an attempt to make the dependents to insvc.");
		/* Move the dependent entities to InSvc and clear the
		dependency flag.*/
		plms_move_dep_ent_to_insvc(ent->rev_dep_list,
		&aff_ent_list,1);
		
		/* Merger aff_ent_list*/
		if (NULL != aff_ent_list_flag){
			log_head = aff_ent_list_flag;
			while (log_head){
				plms_ent_to_ent_list_add(log_head->plm_entity,&aff_ent_list);
				log_head = log_head->next;
			}
		}
		TRACE("Affected entities for ent %s: ",ent->dn_name_str);
		log_head = aff_ent_list;
		while(log_head){
			TRACE("%s,",log_head->plm_entity->dn_name_str);
			log_head = log_head->next;
		}
		/* Overwrite the expected readiness status with the current.*/
		plms_aff_ent_exp_rdness_state_ow(aff_ent_list);
	
		/*Fill the expected readiness state of the root ent.*/
		plms_ent_exp_rdness_state_ow(ent);
		
		memset(&new_trk_info,0,sizeof(PLMS_TRACK_INFO));
		new_trk_info.aff_ent_list = aff_ent_list;
		new_trk_info.group_info_list = NULL;
	
		/* Add the groups, root entity(ent) belong to.*/
		plms_ent_grp_list_add(ent, &(new_trk_info.group_info_list));
	
		/* Find out all the groups, all affected entities belong to 
		and add the groups to trk_info->group_info_list. */
		plms_ent_list_grp_list_add(aff_ent_list,
					&(new_trk_info.group_info_list));	

		TRACE("Affected groups for ent %s: ",ent->dn_name_str);
		log_head_grp = new_trk_info.group_info_list;
		while(log_head_grp){
			TRACE("%llu,",log_head_grp->ent_grp_inf->entity_grp_hdl);
			log_head_grp = log_head_grp->next;
		}
		new_trk_info.change_step = SA_PLM_CHANGE_COMPLETED;
		new_trk_info.root_correlation_id = SA_NTF_IDENTIFIER_UNUSED;
		new_trk_info.grp_op = SA_PLM_GROUP_MEMBER_READINESS_CHANGE;
		if ((NULL != trk_info) && ((SA_PLM_CAUSE_HE_ACTIVATED == trk_info->track_cause) || 
		(SA_PLM_CAUSE_FAILURE_CLEARED == trk_info->track_cause))){
			plms_ent_to_ent_list_add(ent,&(new_trk_info.aff_ent_list));
			new_trk_info.track_cause = trk_info->track_cause;
			new_trk_info.root_entity = trk_info->root_entity;
			plms_cbk_call(&new_trk_info,0/*do not add root */);
		}else{
			new_trk_info.track_cause = SA_PLM_CAUSE_EE_INSTANTIATED;
			new_trk_info.root_entity = ent;
			plms_cbk_call(&new_trk_info,1);
		}

		plms_ent_exp_rdness_status_clear(ent);
		plms_aff_ent_exp_rdness_status_clear(aff_ent_list);
		
		plms_ent_list_free(new_trk_info.aff_ent_list);
		new_trk_info.aff_ent_list = NULL;
		
		plms_ent_grp_list_free(new_trk_info.group_info_list);
		new_trk_info.group_info_list = NULL;
	}else if(is_flag_aff){
		/*Fill the expected readiness state of the root ent.*/
		plms_ent_exp_rdness_state_ow(ent);
		
		memset(&new_trk_info,0,sizeof(PLMS_TRACK_INFO));
		new_trk_info.group_info_list = NULL;
		new_trk_info.aff_ent_list = NULL;
		
		if (NULL != aff_ent_list_flag){
			new_trk_info.aff_ent_list = aff_ent_list_flag;
			plms_ent_list_grp_list_add(aff_ent_list_flag,&(new_trk_info.group_info_list));
			plms_aff_ent_exp_rdness_state_ow(aff_ent_list_flag);
		}
		/* Add the groups, root entity(ent) belong to.*/
		plms_ent_grp_list_add(ent, &(new_trk_info.group_info_list));
	
		TRACE("Affected groups for ent %s: ",ent->dn_name_str);
		log_head_grp = new_trk_info.group_info_list;
		while(log_head_grp){
			TRACE("%llu,",log_head_grp->ent_grp_inf->entity_grp_hdl);
			log_head_grp = log_head_grp->next;
		}
		new_trk_info.change_step = SA_PLM_CHANGE_COMPLETED;
		new_trk_info.root_correlation_id = SA_NTF_IDENTIFIER_UNUSED;
		new_trk_info.grp_op = SA_PLM_GROUP_MEMBER_READINESS_CHANGE;
	
		/* For */
		if ((NULL != trk_info) && ((SA_PLM_CAUSE_HE_ACTIVATED == trk_info->track_cause) || 
		(SA_PLM_CAUSE_FAILURE_CLEARED == trk_info->track_cause))){
			plms_ent_to_ent_list_add(ent,&(new_trk_info.aff_ent_list));
			new_trk_info.track_cause = trk_info->track_cause;
			new_trk_info.root_entity = trk_info->root_entity;
			plms_cbk_call(&new_trk_info,0/*do not add root */);
		}else{
			new_trk_info.track_cause = SA_PLM_CAUSE_EE_INSTANTIATED;
			new_trk_info.root_entity = ent;
			plms_cbk_call(&new_trk_info,1);
		}

		plms_ent_exp_rdness_status_clear(ent);
		plms_aff_ent_exp_rdness_status_clear(aff_ent_list_flag);
		
		plms_ent_grp_list_free(new_trk_info.group_info_list);
		new_trk_info.group_info_list = NULL;
		plms_ent_list_free(aff_ent_list_flag);
		
	}else if (is_set){
		plms_insted_dep_immi_failure_cbk_call(ent,aff_ent_list_flag);
	}
	/* Free the trk_info corresponding to the last admin operation
	if count is zero.*/
	if ((NULL != trk_info) && !(trk_info->track_count))
		plms_trk_info_free(trk_info);
	
	TRACE_LEAVE2("Return Val: %d",ret_err);
	return ret_err;
}

/******************************************************************************
@brief		: Send unlock command to PLMC. The response to this command
		is received asynchronously through tcp callback.
		This function takes care of setting admin pending and management
		lost flag and also calling the callback if the unlock operation 
		fails.
@param[in]	: ent - EE
@param[in]	: is_adm_op - Is this an admin operation(0/1).
@param[in]	: mngt_cbk - Should callback be called for management lost(0/1).
@return		: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
******************************************************************************/
SaUint32T plms_ee_unlock(PLMS_ENTITY *ent,SaUint32T is_adm_op,SaUint32T mngt_cbk)
{
	SaUint32T ret_err;
	SaUint32T cbk = 0;

	TRACE_ENTER2("Entity: %s",ent->dn_name_str);
	
	/* Get the ee_id in string format.*/	
	ret_err = plmc_sa_plm_admin_unlock(ent->dn_name_str,plms_plmc_tcp_cbk);
	
	/* Unlock operation failed.*/
	if(ret_err){
		if(is_adm_op){
			if (!plms_rdness_flag_is_set(ent,
			SA_PLM_RF_ADMIN_OPERATION_PENDING)){
				plms_readiness_flag_mark_unmark(ent,
					SA_PLM_RF_ADMIN_OPERATION_PENDING, 
					1/*mark*/,NULL,
					SA_NTF_MANAGEMENT_OPERATION,
					SA_PLM_NTFID_STATE_CHANGE_ROOT);
				cbk = 1;	
			}
		}

		if (!plms_rdness_flag_is_set(ent,SA_PLM_RF_MANAGEMENT_LOST)){
			/* Set management lost flag.*/
			plms_readiness_flag_mark_unmark(ent,
				SA_PLM_RF_MANAGEMENT_LOST,1/*mark*/,
				NULL,SA_NTF_MANAGEMENT_OPERATION,
				SA_PLM_NTFID_STATE_CHANGE_ROOT);

			cbk = 1;
		}
		/* Call managemnet lost callback.*/
		if (mngt_cbk && cbk){
			plms_mngt_lost_clear_cbk_call(ent,1/*mark*/);
		}

		ent->mngt_lost_tri = PLMS_MNGT_EE_UNLOCK;
		ret_err = NCSCC_RC_FAILURE;
	}else{
		/* Unlock successful.*/
		ret_err = NCSCC_RC_SUCCESS;
	}	
	TRACE_LEAVE2("Return Val: %d",ret_err);
	return ret_err;
}
/******************************************************************************
@brief		: Send terminate command to PLMC. The response to this command
		is received asynchronously through tcp callback.
		This function takes care of setting admin pending and management
		lost flag and also calling the callback if the term operation 
		fails.
@param[in]	: ent - EE
@param[in]	: is_adm_op - Is this an admin operation(0/1).
@param[in]	: mngt_cbk - Should callback be called for management lost(0/1).
@return		: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
******************************************************************************/
SaUint32T plms_ee_term(PLMS_ENTITY *ent,SaUint32T is_adm_op,SaUint32T mngt_cbk)
{
	SaUint32T ret_err,cbk = 0;

	TRACE_ENTER2("Entity: %s",ent->dn_name_str);

	if (PLMS_EE_ENTITY != ent->entity_type)
		return NCSCC_RC_SUCCESS;

	if ( SA_PLM_EE_PRESENCE_UNINSTANTIATED == 
			ent->entity.ee_entity.saPlmEEPresenceState){
		TRACE("EE %s is already in uninstantiated state.",
		ent->dn_name_str);
		return NCSCC_RC_SUCCESS;
	}

	ret_err = plmc_sa_plm_admin_lock_instantiation(ent->dn_name_str,
	plms_plmc_tcp_cbk);
	
	/* EE termination failed operation failed.*/
	if(ret_err){
		if(is_adm_op){
			if (!plms_rdness_flag_is_set(ent,
			SA_PLM_RF_ADMIN_OPERATION_PENDING)){
				plms_readiness_flag_mark_unmark(ent,
					SA_PLM_RF_ADMIN_OPERATION_PENDING, 
					1/*mark*/,NULL,
					SA_NTF_MANAGEMENT_OPERATION,
					SA_PLM_NTFID_STATE_CHANGE_ROOT);
				cbk = 1;	
			}
		}

		if (!plms_rdness_flag_is_set(ent,SA_PLM_RF_MANAGEMENT_LOST)){
			/* Set management lost flag.*/
			plms_readiness_flag_mark_unmark(ent,
				SA_PLM_RF_MANAGEMENT_LOST,1/*mark*/,
				NULL,SA_NTF_MANAGEMENT_OPERATION,
				SA_PLM_NTFID_STATE_CHANGE_ROOT);

			cbk = 1;
		}
		/* Call managemnet lost callback.*/
		if (mngt_cbk && cbk){
			plms_mngt_lost_clear_cbk_call(ent,1/*mark*/);
		}
		
		ent->mngt_lost_tri = PLMS_MNGT_EE_TERM;
		ret_err = NCSCC_RC_FAILURE;
	}else{
		/* EE termination successful.*/
		/* Mark the presence state of the EE to terminating.*/
		plms_presence_state_set(ent,
					SA_PLM_EE_PRESENCE_TERMINATING,
					NULL,SA_NTF_OBJECT_OPERATION,
					SA_PLM_NTFID_STATE_CHANGE_ROOT);
		/* Start the termination failed timer.*/
		if (!(ent->tmr.timer_id)){
			ent->tmr.tmr_type = PLMS_TMR_EE_TERMINATING;
			ret_err = plms_timer_start(&ent->tmr.timer_id,ent,
			ent->entity.ee_entity.saPlmEETerminateTimeout);
			if (NCSCC_RC_SUCCESS != ret_err){
				ent->tmr.timer_id = 0;
				ent->tmr.tmr_type = PLMS_TMR_NONE;
				ent->tmr.context_info = NULL;
			}
		}else if (PLMS_TMR_EE_INSTANTIATING == ent->tmr.tmr_type){
			/* Stop the inst timer and start the term timer.*/
			plms_timer_stop(ent);
			ent->tmr.tmr_type = PLMS_TMR_EE_TERMINATING;
			ret_err = plms_timer_start(&ent->tmr.timer_id,ent,
			ent->entity.ee_entity.saPlmEETerminateTimeout);
			if (NCSCC_RC_SUCCESS != ret_err){
				ent->tmr.timer_id = 0;
				ent->tmr.tmr_type = PLMS_TMR_NONE;
				ent->tmr.context_info = NULL;
			}
		} else {
			/* The timer is already running.*/
		}
		
		ret_err = NCSCC_RC_SUCCESS;
	}	
	TRACE_LEAVE2("Return Val: %d",ret_err);
	return ret_err;
	
}
/******************************************************************************
@brief		: Send lock command to PLMC. The response to this command
		is received asynchronously through tcp callback.
		This function takes care of setting admin pending and management
		lost flag and also calling the callback if the lock operation 
		fails.
@param[in]	: ent - EE
@param[in]	: is_adm_op - Is this an admin operation(0/1).
@param[in]	: mngt_cbk - Should callback be called for management lost(0/1).
@return		: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
******************************************************************************/
SaUint32T plms_ee_lock(PLMS_ENTITY *ent, SaUint32T is_adm_op, SaUint32T mngt_cbk)
{
	SaUint32T ret_err,cbk = 0;

	TRACE_ENTER2("Entity: %s",ent->dn_name_str);
	
	ret_err = plmc_sa_plm_admin_lock(ent->dn_name_str,plms_plmc_tcp_cbk);
	
	/* Lock operation failed.*/
	if(ret_err){
		if(is_adm_op){
			if (!plms_rdness_flag_is_set(ent,
			SA_PLM_RF_ADMIN_OPERATION_PENDING)){
				plms_readiness_flag_mark_unmark(ent,
					SA_PLM_RF_ADMIN_OPERATION_PENDING, 
					1/*mark*/,NULL,
					SA_NTF_MANAGEMENT_OPERATION,
					SA_PLM_NTFID_STATE_CHANGE_ROOT);
				cbk = 1;	
			}
		}

		if (!plms_rdness_flag_is_set(ent,SA_PLM_RF_MANAGEMENT_LOST)){
			/* Set management lost flag.*/
			plms_readiness_flag_mark_unmark(ent,
				SA_PLM_RF_MANAGEMENT_LOST,1/*mark*/,
				NULL,SA_NTF_MANAGEMENT_OPERATION,
				SA_PLM_NTFID_STATE_CHANGE_ROOT);

			cbk = 1;
		}
		/* Call managemnet lost callback.*/
		if (mngt_cbk && cbk){
			plms_mngt_lost_clear_cbk_call(ent,1/*mark*/);
		}
		ent->mngt_lost_tri = PLMS_MNGT_EE_LOCK;
		ret_err = NCSCC_RC_FAILURE;
	}else{
		/* Lock successful.*/
		ret_err = NCSCC_RC_SUCCESS;
	}	
	TRACE_LEAVE2("Return Val: %d",ret_err);
	return ret_err;
}
/******************************************************************************
@brief		: Send restart command to PLMC. The response to this command
		is received asynchronously through tcp callback.
		This function takes care of setting admin pending and management
		lost flag and also calling the callback if the restart operation 
		fails.
@param[in]	: ent - EE
@param[in]	: is_adm_op - Is this an admin operation(0/1).
@param[in]	: mngt_cbk - Should callback be called for management lost(0/1).
@return		: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
******************************************************************************/
SaUint32T plms_ee_reboot(PLMS_ENTITY *ent, SaUint32T is_adm_op,SaUint32T mngt_cbk)
{
	SaUint32T ret_err,cbk = 0;
	TRACE_ENTER2("Entity: %s",ent->dn_name_str);

	if ( SA_PLM_EE_PRESENCE_UNINSTANTIATED == 
		ent->entity.ee_entity.saPlmEEPresenceState){
		LOG_ER("%s is in uninstantiated state, can not be \
		reboot.", ent->dn_name_str);
		return NCSCC_RC_FAILURE;
	}
	
	ret_err = plmc_sa_plm_admin_restart(ent->dn_name_str,plms_plmc_tcp_cbk);
	
	/* Restart operation failed.*/
	if(ret_err){
		if(is_adm_op){
			if (!plms_rdness_flag_is_set(ent,
			SA_PLM_RF_ADMIN_OPERATION_PENDING)){
				plms_readiness_flag_mark_unmark(ent,
					SA_PLM_RF_ADMIN_OPERATION_PENDING, 
					1/*mark*/,NULL,
					SA_NTF_MANAGEMENT_OPERATION,
					SA_PLM_NTFID_STATE_CHANGE_ROOT);
				cbk = 1;	
			}
		}

		if (!plms_rdness_flag_is_set(ent,SA_PLM_RF_MANAGEMENT_LOST)){
			/* Set management lost flag.*/
			plms_readiness_flag_mark_unmark(ent,
				SA_PLM_RF_MANAGEMENT_LOST,1/*mark*/,
				NULL,SA_NTF_MANAGEMENT_OPERATION,
				SA_PLM_NTFID_STATE_CHANGE_ROOT);

			cbk = 1;
		}
		/* Call managemnet lost callback.*/
		if (mngt_cbk && cbk){
			plms_mngt_lost_clear_cbk_call(ent,1/*mark*/);
		}
		
		ent->mngt_lost_tri = PLMS_MNGT_EE_RESTART;
		ret_err = NCSCC_RC_FAILURE;
	}else{
		/* Restart successful.*/
		ret_err = NCSCC_RC_SUCCESS;
	}	
	TRACE_LEAVE2("Return Val: %d",ret_err);
	return ret_err;
}

/******************************************************************************
@brief		: Instantiate the EE by resetting the parent HE. 
		This function takes care of setting admin pending and management
		lost flag and also calling the callback if the instantiate operation 
		fails.
@param[in]	: ent - EE
@param[in]	: is_adm_op - Is this an admin operation(0/1).
@param[in]	: mngt_cbk - Should callback be called for management lost(0/1).
@return		: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
******************************************************************************/
SaUint32T plms_ee_instantiate(PLMS_ENTITY *ent,SaUint32T is_adm_op,SaUint32T mngt_cbk)
{
	SaUint32T ret_err,cbk = 0;
	PLMS_ENTITY *parent_he;

	if (PLMS_EE_ENTITY != ent->entity_type)
		return NCSCC_RC_SUCCESS;
	
	
	TRACE_ENTER2("Entity: %s",ent->dn_name_str);

	/* As we are not supporting virtualization, the parent of the EE
	will be HE or domain.*/
	parent_he = ent->parent; 
	
	if (NULL == parent_he){
		/*TODO: The EE is direct child of domain. How to
		instantiate the EE??*/
		LOG_ER("Entity %s is direct child of domain. Instantiate\
				FAILED",ent->dn_name_str);
		TRACE_LEAVE2("Return Val: %d",NCSCC_RC_FAILURE);
		return NCSCC_RC_FAILURE;
	}
	/* TODO: Virtualization is not taken into consideration.*/
	if (NULL == parent_he->entity.he_entity.saPlmHECurrEntityPath){
		/* TODO: HE is not present.*/
		LOG_ER("Parent HE of this EE %s is not present.",
						ent->dn_name_str);
		ret_err = NCSCC_RC_FAILURE; 
	}else{
		/* Reset the parent_he.*/
		TRACE("Reset the parent HE %s(ee_inst).",
						parent_he->dn_name_str);
		ret_err = plms_he_reset(parent_he,1/*adm_op*/,1/*mngt_cbk*/,
							SAHPI_COLD_RESET);
	}

	/* EE instantiation operation failed.*/
	if(NCSCC_RC_SUCCESS != ret_err){
		ent->mngt_lost_tri = PLMS_MNGT_EE_INST;
		if(is_adm_op){
			if (!plms_rdness_flag_is_set(ent,
			SA_PLM_RF_ADMIN_OPERATION_PENDING)){
				plms_readiness_flag_mark_unmark(ent,
					SA_PLM_RF_ADMIN_OPERATION_PENDING, 
					1/*mark*/,NULL,
					SA_NTF_MANAGEMENT_OPERATION,
					SA_PLM_NTFID_STATE_CHANGE_ROOT);
				cbk = 1;	
			}
		}

		if (!plms_rdness_flag_is_set(ent,SA_PLM_RF_MANAGEMENT_LOST)){
			/* Set management lost flag.*/
			plms_readiness_flag_mark_unmark(ent,
				SA_PLM_RF_MANAGEMENT_LOST,1/*mark*/,
				NULL,SA_NTF_MANAGEMENT_OPERATION,
				SA_PLM_NTFID_STATE_CHANGE_ROOT);

			cbk = 1;
		}
		/* Call managemnet lost callback.*/
		if (mngt_cbk && cbk){
			plms_mngt_lost_clear_cbk_call(ent,1/*mark*/);
		}
	}else{
		/* EE instantiation successful.*/
		/* Mark the presence state of the EE to terminating.*/
		plms_presence_state_set(ent,
					SA_PLM_EE_PRESENCE_INSTANTIATING,
					NULL,SA_NTF_OBJECT_OPERATION,
					SA_PLM_NTFID_STATE_CHANGE_ROOT);
		/* Start the instantiation failed timer.*/
		if (!(ent->tmr.timer_id)){
			ent->tmr.tmr_type = PLMS_TMR_EE_INSTANTIATING;
			ret_err = plms_timer_start(&ent->tmr.timer_id,ent,
			ent->entity.ee_entity.saPlmEEInstantiateTimeout);
			if (NCSCC_RC_SUCCESS != ret_err){
				ent->tmr.timer_id = 0;
				ent->tmr.tmr_type = PLMS_TMR_NONE;
				ent->tmr.context_info = NULL;
			}
		}else if (PLMS_TMR_EE_TERMINATING == ent->tmr.tmr_type){
			/* Stop the term timer and start the inst timer.*/
			plms_timer_stop(ent);
			ent->tmr.tmr_type = PLMS_TMR_EE_INSTANTIATING;
			ret_err = plms_timer_start(&ent->tmr.timer_id,ent,
			ent->entity.ee_entity.saPlmEEInstantiateTimeout);
			if (NCSCC_RC_SUCCESS != ret_err){
				ent->tmr.timer_id = 0;
				ent->tmr.tmr_type = PLMS_TMR_NONE;
				ent->tmr.context_info = NULL;
			}
		} else {
			/* The timer is already running.*/
		}
		
		ret_err = NCSCC_RC_SUCCESS;
	}
	
	TRACE_LEAVE2("Return Val: %d",ret_err);
	return ret_err;
}
/******************************************************************************
@brief		: Process instantiation failed timer expiry. 
		Logically the presence state of the ent should be
		inst-failed. But as this is a failure, we have to isolate the
		EE which means, uninstantiating the EE. Hence moving the EE to
		uninstantiated state. TODO: Should the presence state be
		inst-failed??
@param[in]	: ent - EE failed to instantiate.
@return 	: NCSCC_RC_SUCCESS	
******************************************************************************/
SaUint32T plms_ee_inst_failed_tmr_exp(PLMS_ENTITY *ent)
{
	PLMS_CB *cb = plms_cb;
	SaUint32T ret_err = NCSCC_RC_SUCCESS;
	PLMS_GROUP_ENTITY *head = NULL;

	TRACE_ENTER2("Entity: %s",ent->dn_name_str);
	
	/* Clean up the timer context.*/
	ent->tmr.timer_id = 0;
	ent->tmr.tmr_type = PLMS_TMR_NONE;
	ent->tmr.context_info = NULL;

	/* TODO: Should I check if my parent is in insvc and
	min dep is ok. If not then return from here.*/

	/* Take the ent to instantiation-failed state.*/
	plms_presence_state_set(ent,
				SA_PLM_EE_PRESENCE_INSTANTIATION_FAILED,
				NULL,SA_NTF_OBJECT_OPERATION,
				SA_PLM_NTFID_STATE_CHANGE_ROOT);

	/* Mark the operational state to disabled.*/
	plms_op_state_set(ent,SA_PLM_OPERATIONAL_DISABLED,
			NULL,SA_NTF_OBJECT_OPERATION,
			SA_PLM_NTFID_STATE_CHANGE_ROOT);
	
	if ((NULL != ent->trk_info) && (SA_PLM_CAUSE_EE_RESTART == 
	ent->trk_info->root_entity->adm_op_in_progress)){
		
		ret_err = saImmOiAdminOperationResult(cb->oi_hdl,
		ent->trk_info->inv_id,SA_AIS_ERR_FAILED_OPERATION);

		if (NCSCC_RC_SUCCESS != ret_err){
			LOG_ER("Adm operation result sending to IMM \
			failed for ent: %s,imm-ret-val: %d",
			ent->dn_name_str,ret_err);
		}
		
		/* Free the trk info and send failure to IMM.*/
		head = ent->trk_info->aff_ent_list;
		while(head){
			head->plm_entity->trk_info = NULL;
		}
		
		plms_trk_info_free(ent->trk_info);
	}
	
	/* Isolate the EE by "assert reset state on parent HE" or taking the
	"paretn HE to inactive".*/
	ret_err = plms_inst_term_failed_isolate(ent);

	TRACE_LEAVE2("Return Val: %d",ret_err);
	return ret_err;	
}
/******************************************************************************
@brief		: Process termination failed timer expiry. 
		Logically the presence state should be marked as termination-
		failed. But termination failed is considered as a failure. 
		This causes to isolate the EE i.e. make its parent HE to go to
		"assert reset state on the HE" or "inactive". In both the cases
		the presence state of the EE is to be set as uninstantiated. If
		the isolation failed, then mark management lost flag and isolate
		pending flag.
@param[in]	: ent - EE fails to terminate.
@return		: NCSCC_RC_FAILURE/NCSCC_RC_SUCCESS
******************************************************************************/
SaUint32T plms_ee_term_failed_tmr_exp(PLMS_ENTITY *ent)
{
	SaUint32T ret_err = NCSCC_RC_SUCCESS;
	PLMS_GROUP_ENTITY *head = NULL, *aff_ent_list = NULL;
	PLMS_TRACK_INFO trk_info;

	TRACE_ENTER2("Entity: %s",ent->dn_name_str);
	
	/* Clean up the timer context.*/
	ent->tmr.timer_id = 0;
	ent->tmr.tmr_type = PLMS_TMR_NONE;
	ent->tmr.context_info = NULL;
	
	/* Take the ent to instantiation-failed state.*/
	plms_presence_state_set(ent,
				SA_PLM_EE_PRESENCE_TERMINATION_FAILED,
				NULL,SA_NTF_OBJECT_OPERATION,
				SA_PLM_NTFID_STATE_CHANGE_ROOT);

	/* Mark the operational state to disabled.*/
	plms_op_state_set(ent, SA_PLM_OPERATIONAL_DISABLED,
			NULL,SA_NTF_OBJECT_OPERATION,
			SA_PLM_NTFID_STATE_CHANGE_ROOT);

	/* If the EE is already in OOS, then nothing to do.*/
	if (SA_PLM_READINESS_OUT_OF_SERVICE == 
		ent->entity.ee_entity.saPlmEEReadinessState){
		TRACE("Entity %s is already in OOS.",ent->dn_name_str);
	
	}else{ /* If the entity is in insvc, then got to make the entity 
		to move to OOS.*/
		/* Get all the affected entities.*/ 
		plms_affected_ent_list_get(ent,&aff_ent_list,0);
		
		plms_readiness_state_set(ent,SA_PLM_READINESS_OUT_OF_SERVICE,
					NULL,SA_NTF_OBJECT_OPERATION,
					SA_PLM_NTFID_STATE_CHANGE_ROOT);
		
		/* Mark the readiness state and expected readiness state  
		of all the affected entities to OOS and set the dependency 
		flag.*/
		head = aff_ent_list;
		while (head){
			plms_readiness_state_set(head->plm_entity,
					SA_PLM_READINESS_OUT_OF_SERVICE,
					ent,SA_NTF_OBJECT_OPERATION,
					SA_PLM_NTFID_STATE_CHANGE_DEP);
			plms_readiness_flag_mark_unmark(head->plm_entity,
					SA_PLM_RF_DEPENDENCY,TRUE,
					ent,SA_NTF_OBJECT_OPERATION,
					SA_PLM_NTFID_STATE_CHANGE_DEP);
			
			/* Terminate all the dependent EEs.*/
			if (NCSCC_RC_SUCCESS != 
					plms_is_chld(ent,head->plm_entity)){
				ret_err = plms_ee_term(head->plm_entity,
							FALSE,TRUE/*mngt_cbk*/);
				if (NCSCC_RC_SUCCESS != ret_err){
					LOG_ER("Request for EE %s termination \
					FAILED",head->plm_entity->dn_name_str);
				}
			}	

			head = head->next;
		}
		
		plms_aff_ent_exp_rdness_state_ow(aff_ent_list);
		plms_ent_exp_rdness_state_ow(ent);

		memset(&trk_info,0,sizeof(PLMS_TRACK_INFO));
		trk_info.aff_ent_list = aff_ent_list;
		trk_info.group_info_list = NULL;
		/* Add the groups, root entity(ent) belong to.*/
		plms_ent_grp_list_add(ent, &(trk_info.group_info_list));
	
		/* Find out all the groups, all affected entities 
		 * belong to and add the groups to trk_info->group_info_list.*/ 
		plms_ent_list_grp_list_add(trk_info.aff_ent_list,
					&(trk_info.group_info_list));	

		trk_info.change_step = SA_PLM_CHANGE_COMPLETED;
		trk_info.root_correlation_id = SA_NTF_IDENTIFIER_UNUSED;
		trk_info.track_cause = SA_PLM_CAUSE_FAILURE;
		trk_info.grp_op = SA_PLM_GROUP_MEMBER_READINESS_CHANGE;
		trk_info.root_entity = ent;

		plms_cbk_call(&trk_info,1);
	
		plms_ent_exp_rdness_status_clear(ent);
		plms_aff_ent_exp_rdness_status_clear(aff_ent_list);
		
		plms_ent_list_free(trk_info.aff_ent_list);
		trk_info.aff_ent_list = NULL;
		plms_ent_grp_list_free(trk_info.group_info_list);
		trk_info.group_info_list = NULL;
	}
	
	/* Isolate the entity if possible. */
	ret_err = plms_inst_term_failed_isolate(ent);

	TRACE_LEAVE2("Return Val: %d",ret_err);
	return ret_err;
}
/******************************************************************************
@brief		: Isolate the entity which fails to instantiate or terminate 
@param[in]	: ent - EE.
@return		: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
******************************************************************************/
SaUint32T plms_inst_term_failed_isolate(PLMS_ENTITY *ent)
{
	SaUint32T ret_err = NCSCC_RC_SUCCESS, can_isolate = 1;
	PLMS_GROUP_ENTITY *head = NULL, *chld_list = NULL;
	/* Isolate the EE by "assert reset state on parent HE" or taking the
	"paretn HE to inactive".*/

	/* See any other children of the HE other than myself, is insvc.*/
	if (ent->parent){
		plms_chld_get(ent->parent->leftmost_child,&chld_list);
		head = chld_list;
		while(head){
			if ( 0 == strcmp(head->plm_entity->dn_name_str,ent->dn_name_str)){
				head = head->next;
				continue;
			}
			if (plms_is_rdness_state_set(head->plm_entity,
						SA_PLM_READINESS_IN_SERVICE)){
				can_isolate = 0;
				break;
			}
			head = head->next;
		}

		plms_ent_list_free(chld_list);
		chld_list = NULL;
	}else{
		/* EEs parent is domain*/
		LOG_ER("EE`s direct parent is Domain or parent not configured.\
		, hence the EE cannot be isolated.");
		
		return NCSCC_RC_FAILURE;
	}
	if (can_isolate){
		/* assert reset state on parent HE,
		(parent is HE as no virtualization). */
		ret_err = plms_he_reset(ent->parent,0/*adm_op*/,1/*mngt_cbk*/, SAHPI_RESET_ASSERT);
		if( NCSCC_RC_SUCCESS != ret_err){
			/* Deactivate the HE.*/
			ret_err = plms_he_deactivate(ent->parent,FALSE/*adm_op*/,TRUE/*mngt_cbk*/);
			if(NCSCC_RC_SUCCESS != ret_err){
				/* Isolation failed.*/
				can_isolate = 0;
				LOG_ER("EE %s Isolation FAILED.", ent->dn_name_str);
			}else{
				TRACE("Isolated the ent %s, Deactivate parent HE successfull.",ent->dn_name_str);
				ent->iso_method = PLMS_ISO_HE_DEACTIVATED;
			}
		}else{
			TRACE("Isolate the ent %s, Reset assert parent HE successfull.",ent->dn_name_str);
			ent->iso_method = PLMS_ISO_HE_RESET_ASSERT;
		}
	}else {
		LOG_ER("EE's siblings are insvc and, hence the EE cannot be isolated.");
		return NCSCC_RC_FAILURE;
	}
	if(!can_isolate){
		/* Set isolate pending flag. Set management lost flag.*/
		plms_readiness_flag_mark_unmark(ent, SA_PLM_RF_ISOLATE_PENDING,1, NULL,
		SA_NTF_OBJECT_OPERATION, SA_PLM_NTFID_STATE_CHANGE_ROOT);
		
		plms_readiness_flag_mark_unmark(ent, SA_PLM_RF_MANAGEMENT_LOST,1, 
		NULL,SA_NTF_OBJECT_OPERATION, SA_PLM_NTFID_STATE_CHANGE_ROOT);
		
		/* ent->mngt_lost_tri = PLMS_MNGT_EE_ISOLATE; */
		ent->mngt_lost_tri = PLMS_MNGT_EE_TERM;
		
		/* Call management lost callback for ent.*/
		plms_mngt_lost_clear_cbk_call(ent,1/*lost*/);

		return NCSCC_RC_FAILURE;
	}else{
		if ( plms_rdness_flag_is_set (ent,SA_PLM_RF_MANAGEMENT_LOST)){
		
			plms_readiness_flag_mark_unmark( ent,SA_PLM_RF_ISOLATE_PENDING,0,
			NULL,SA_NTF_OBJECT_OPERATION, SA_PLM_NTFID_STATE_CHANGE_ROOT);
					
			/* Clear admin pending */	
			plms_readiness_flag_mark_unmark(ent, SA_PLM_RF_ADMIN_OPERATION_PENDING,0,
			NULL, SA_NTF_OBJECT_OPERATION, SA_PLM_NTFID_STATE_CHANGE_ROOT);
					
			/* Clear management lost.*/
			plms_readiness_flag_mark_unmark(ent, SA_PLM_RF_MANAGEMENT_LOST,0,NULL,
			SA_NTF_OBJECT_OPERATION, SA_PLM_NTFID_STATE_CHANGE_ROOT);

			/* Call management lost callback for ent.*/
			plms_mngt_lost_clear_cbk_call(ent,0/*cleared*/);
		}
		
		/* EE is isolated. Mark the presence state to uninstantiated.*/
		plms_presence_state_set(ent,SA_PLM_EE_PRESENCE_UNINSTANTIATED,
		NULL,SA_NTF_OBJECT_OPERATION, SA_PLM_NTFID_STATE_CHANGE_ROOT);
	}

	return NCSCC_RC_SUCCESS;
}
/******************************************************************************
@brief		: Handle tmr events in MBX.
@param[in]	: evt - PLMS_EVT representing timer expiry events.
@return		: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
******************************************************************************/
SaUint32T plms_mbx_tmr_handler(PLMS_EVT *evt)
{
	SaUint32T ret_err = NCSCC_RC_SUCCESS;
	
	switch(evt->req_evt.plm_tmr.tmr_type){
	case PLMS_TMR_EE_INSTANTIATING:
		ret_err = plms_ee_inst_failed_tmr_exp(
		(PLMS_ENTITY *)evt->req_evt.plm_tmr.context_info);
		break;
		
	case PLMS_TMR_EE_TERMINATING:
		ret_err = plms_ee_term_failed_tmr_exp(
		(PLMS_ENTITY *)evt->req_evt.plm_tmr.context_info);
		break;
	default:
		break;
	}

	return ret_err;
}
/******************************************************************************
@brief		: Clear readiness flags on getting EE terminated event.
@param[in]	: ent - Entity whose readinsess flags are to be cleared.
@return		: NCSCC_RC_SUCCESS
******************************************************************************/
static SaUint32T plms_ee_terminated_mngt_flag_clear(PLMS_ENTITY *ent)
{
	TRACE("Clear mngtment lost flag/isolate pending flag/\
	admin pending flag. Reason: %d, Ent: %s",
	ent->mngt_lost_tri, ent->dn_name_str);

	plms_readiness_flag_mark_unmark(ent,
	SA_PLM_RF_MANAGEMENT_LOST,0/*unmark*/,
	NULL,SA_NTF_OBJECT_OPERATION,
	SA_PLM_NTFID_STATE_CHANGE_ROOT);

	/* Clear admin pending flag.*/
	plms_readiness_flag_mark_unmark(ent,
	SA_PLM_RF_ADMIN_OPERATION_PENDING,0,NULL,
	SA_NTF_OBJECT_OPERATION,
	SA_PLM_NTFID_STATE_CHANGE_ROOT);
	
	/* Clear isolate pending flag.*/
	plms_readiness_flag_mark_unmark(ent,
	SA_PLM_RF_ISOLATE_PENDING,0,NULL,
	SA_NTF_OBJECT_OPERATION,
	SA_PLM_NTFID_STATE_CHANGE_ROOT);

	ent->mngt_lost_tri = PLMS_MNGT_NONE;
	/* ent->iso_method = PLMS_ISO_EE_TERMINATED; */
	plms_mngt_lost_clear_cbk_call(ent,0/*unmark*/);
	return NCSCC_RC_SUCCESS;
}
/******************************************************************************
@brief		: Clear readiness flags on getting tcp-disconnect for the EE.
@param[in]	: ent - Entity whose readinsess flags are to be cleared.
@return		: NCSCC_RC_SUCCESS
******************************************************************************/
static SaUint32T plms_tcp_discon_mngt_flag_clear(PLMS_ENTITY *ent)
{
	TRACE("Clear mngtment lost flag/isolate pending flag/\
	admin pending flag. Reason: %d, Ent: %s",ent->mngt_lost_tri,
	ent->dn_name_str);

	plms_readiness_flag_mark_unmark(ent,
	SA_PLM_RF_MANAGEMENT_LOST,0/*unmark*/,
	NULL,SA_NTF_OBJECT_OPERATION,
	SA_PLM_NTFID_STATE_CHANGE_ROOT);

	/* Clear admin pending flag.*/
	plms_readiness_flag_mark_unmark(ent,
	SA_PLM_RF_ADMIN_OPERATION_PENDING,0,NULL,
	SA_NTF_OBJECT_OPERATION,
	SA_PLM_NTFID_STATE_CHANGE_ROOT);
	
	/* Clear isolate pending flag.*/
	plms_readiness_flag_mark_unmark(ent,
	SA_PLM_RF_ISOLATE_PENDING,0,NULL,
	SA_NTF_OBJECT_OPERATION,
	SA_PLM_NTFID_STATE_CHANGE_ROOT);

	ent->mngt_lost_tri = PLMS_MNGT_NONE;
	/* ent->iso_method = PLMS_ISO_EE_TERMINATED; */
	plms_mngt_lost_clear_cbk_call(ent,0/*unmark*/);
	return NCSCC_RC_SUCCESS;
}
/******************************************************************************
@brief		: Clear readiness flags on getting tcp-connect for the EE.
@param[in]	: ent - Entity whose readinsess flags are to be cleared.
@return		: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
******************************************************************************/
static SaUint32T plms_tcp_connect_mngt_flag_clear(PLMS_ENTITY *ent)
{
	SaUint32T ret_err = NCSCC_RC_SUCCESS;
#if 0
	/* If isolate pending flag is set then isolate the entity and return
	from here.*/
	if(plms_rdness_flag_is_set(ent, SA_PLM_RF_ISOLATE_PENDING)) {
		
		if (NCSCC_RC_SUCCESS == plms_isolate_and_mngt_lost_clear(ent)) 
			return NCSCC_RC_FAILURE;
		else
			return NCSCC_RC_SUCCESS;
	}
#endif	
	TRACE("Clear management lost flag. Reason:%d, ent: %s",
	ent->mngt_lost_tri,ent->dn_name_str);

	plms_readiness_flag_mark_unmark(ent,
	SA_PLM_RF_MANAGEMENT_LOST,0/*unmark*/,NULL,
	SA_NTF_OBJECT_OPERATION,
	SA_PLM_NTFID_STATE_CHANGE_ROOT);

	/* Clear admin_pending flag if set.*/
	plms_readiness_flag_mark_unmark(ent,
	SA_PLM_RF_ADMIN_OPERATION_PENDING,0/*unmark*/,
	NULL, SA_NTF_OBJECT_OPERATION,
	SA_PLM_NTFID_STATE_CHANGE_ROOT);
	
	plms_readiness_flag_mark_unmark(ent,
	SA_PLM_RF_ISOLATE_PENDING,0/*unmark*/,
	NULL, SA_NTF_OBJECT_OPERATION,
	SA_PLM_NTFID_STATE_CHANGE_ROOT);

	ent->mngt_lost_tri = PLMS_MNGT_NONE;
	ent->iso_method = PLMS_ISO_DEFAULT;
	plms_mngt_lost_clear_cbk_call(ent,0/*unmark*/);
	return ret_err;
}

/******************************************************************************
@brief		: Clear readiness flags on getting successful unlock response. 
@param[in]	: ent - Entity whose readinsess flags are to be cleared.
@return		: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
******************************************************************************/
static SaUint32T plms_unlck_resp_mngt_flag_clear(PLMS_ENTITY *ent)
{
	SaUint32T ret_err = NCSCC_RC_SUCCESS;

	/* If isolate pending flag is set then isolate the entity and return
	from here.*/
	if(plms_rdness_flag_is_set(ent, SA_PLM_RF_ISOLATE_PENDING)){
		
		if (NCSCC_RC_SUCCESS == plms_isolate_and_mngt_lost_clear(ent)) 
			return NCSCC_RC_FAILURE;
		else
			return NCSCC_RC_SUCCESS;
	}
	TRACE("Clear management lost flag. Reason:%d, ent: %s",
	ent->mngt_lost_tri,ent->dn_name_str);
	
	plms_readiness_flag_mark_unmark(ent,
	SA_PLM_RF_MANAGEMENT_LOST,0/*unmark*/,NULL,
	SA_NTF_OBJECT_OPERATION,
	SA_PLM_NTFID_STATE_CHANGE_ROOT);

	/* Clear admin_pending flag if set.*/
	plms_readiness_flag_mark_unmark(ent,
	SA_PLM_RF_ADMIN_OPERATION_PENDING,0/*unmark*/,
	NULL, SA_NTF_OBJECT_OPERATION,
	SA_PLM_NTFID_STATE_CHANGE_ROOT);

	ent->mngt_lost_tri = PLMS_MNGT_NONE;
	plms_mngt_lost_clear_cbk_call(ent,0/*unmark*/);
	return ret_err;
}
/******************************************************************************
@brief		: Clear readiness flags on getting successful lock response. 
@param[in]	: ent - Entity whose readinsess flags are to be cleared.
@return		: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
******************************************************************************/
static SaUint32T plms_lck_resp_mngt_flag_clear(PLMS_ENTITY *ent)
{
	SaUint32T ret_err = NCSCC_RC_SUCCESS;

	/* If isolate pending flag is set then isolate the entity and return
	from here.*/
	if(plms_rdness_flag_is_set(ent, SA_PLM_RF_ISOLATE_PENDING)) {
		
		if (NCSCC_RC_SUCCESS == plms_isolate_and_mngt_lost_clear(ent)) 
			return NCSCC_RC_FAILURE;
		else
			return NCSCC_RC_SUCCESS;
	}
	
	TRACE("Clear management lost flag. Reason:%d, ent: %s",
	ent->mngt_lost_tri,ent->dn_name_str);

	plms_readiness_flag_mark_unmark(ent,
	SA_PLM_RF_MANAGEMENT_LOST,0/*unmark*/,NULL,
	SA_NTF_OBJECT_OPERATION,
	SA_PLM_NTFID_STATE_CHANGE_ROOT);

	/* Clear admin_pending flag if set.*/
	plms_readiness_flag_mark_unmark(ent,
	SA_PLM_RF_ADMIN_OPERATION_PENDING,0/*unmark*/,
	NULL, SA_NTF_OBJECT_OPERATION,
	SA_PLM_NTFID_STATE_CHANGE_ROOT);

	ent->mngt_lost_tri = PLMS_MNGT_NONE;
	plms_mngt_lost_clear_cbk_call(ent,0/*unmark*/);
	return ret_err;
}
/******************************************************************************
@brief		: Clear readiness flags on getting successful lock-inst response. 
@param[in]	: ent - Entity whose readinsess flags are to be cleared.
@return		: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
******************************************************************************/
static SaUint32T plms_lckinst_resp_mngt_flag_clear(PLMS_ENTITY *ent)
{

	TRACE("Clear mngtment lost flag/isolate pending flag/\
	admin pending flag. Reason: %d, Ent: %s",
	ent->mngt_lost_tri,ent->dn_name_str);
	
	plms_readiness_flag_mark_unmark(ent,
	SA_PLM_RF_MANAGEMENT_LOST,0/*unmark*/,NULL,
	SA_NTF_OBJECT_OPERATION,
	SA_PLM_NTFID_STATE_CHANGE_ROOT);

	plms_readiness_flag_mark_unmark(ent,
	SA_PLM_RF_ADMIN_OPERATION_PENDING,0/*unmark*/,
	NULL, SA_NTF_OBJECT_OPERATION,
	SA_PLM_NTFID_STATE_CHANGE_ROOT);
	
	plms_readiness_flag_mark_unmark(ent,
	SA_PLM_RF_ISOLATE_PENDING,0,
	NULL, SA_NTF_OBJECT_OPERATION,
	SA_PLM_NTFID_STATE_CHANGE_ROOT);

	ent->mngt_lost_tri = PLMS_MNGT_NONE;
	plms_mngt_lost_clear_cbk_call(ent,0/*unmark*/);
	return NCSCC_RC_SUCCESS; 
}
/******************************************************************************
@brief		: Clear readiness flags on getting successful restart response. 
@param[in]	: ent - Entity whose readinsess flags are to be cleared.
@return		: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
******************************************************************************/
static SaUint32T plms_restart_resp_mngt_flag_clear(PLMS_ENTITY *ent)
{
	SaUint32T ret_err = NCSCC_RC_SUCCESS;

	if(plms_rdness_flag_is_set(ent, SA_PLM_RF_ISOLATE_PENDING)) {
		if (NCSCC_RC_SUCCESS == plms_isolate_and_mngt_lost_clear(ent)) 
			return NCSCC_RC_FAILURE;
		else
			return NCSCC_RC_SUCCESS;
	}
	TRACE("Clear management lost flag. Reason:%d, ent: %s",
	ent->mngt_lost_tri,ent->dn_name_str);

	plms_readiness_flag_mark_unmark(ent,
	SA_PLM_RF_MANAGEMENT_LOST,0/*unmark*/,NULL,
	SA_NTF_OBJECT_OPERATION,
	SA_PLM_NTFID_STATE_CHANGE_ROOT);

	/* Clear admin_pending flag if set.*/
	plms_readiness_flag_mark_unmark(ent,
	SA_PLM_RF_ADMIN_OPERATION_PENDING,0/*unmark*/,
	NULL, SA_NTF_OBJECT_OPERATION,
	SA_PLM_NTFID_STATE_CHANGE_ROOT);

	ent->mngt_lost_tri = PLMS_MNGT_NONE;
	plms_mngt_lost_clear_cbk_call(ent,0/*unmark*/);
	return ret_err;
}
/******************************************************************************
@brief		: Clear readiness flags on getting successful get os info response. 
@param[in]	: ent - Entity whose readinsess flags are to be cleared.
@return		: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
******************************************************************************/
static SaUint32T plms_os_info_resp_mngt_flag_clear(PLMS_ENTITY *ent)
{
	SaUint32T ret_err = NCSCC_RC_SUCCESS;
#if 0
	if(plms_rdness_flag_is_set(ent, SA_PLM_RF_ISOLATE_PENDING)) {
		if (NCSCC_RC_SUCCESS == plms_isolate_and_mngt_lost_clear(ent)) 
			return NCSCC_RC_FAILURE;
		else
			return NCSCC_RC_SUCCESS;
	}
#endif	
	TRACE("Clear management lost flag. Reason:%d, ent: %s",
	ent->mngt_lost_tri,ent->dn_name_str);

	plms_readiness_flag_mark_unmark(ent,
	SA_PLM_RF_MANAGEMENT_LOST,0/*unmark*/,NULL,
	SA_NTF_OBJECT_OPERATION,
	SA_PLM_NTFID_STATE_CHANGE_ROOT);

	/* Clear admin_pending flag if set.*/
	plms_readiness_flag_mark_unmark(ent,
	SA_PLM_RF_ADMIN_OPERATION_PENDING,0/*unmark*/,
	NULL, SA_NTF_OBJECT_OPERATION,
	SA_PLM_NTFID_STATE_CHANGE_ROOT);
	
	plms_readiness_flag_mark_unmark(ent,
	SA_PLM_RF_ISOLATE_PENDING,0/*unmark*/,
	NULL, SA_NTF_OBJECT_OPERATION,
	SA_PLM_NTFID_STATE_CHANGE_ROOT);

	ent->mngt_lost_tri = PLMS_MNGT_NONE;
	ent->iso_method = PLMS_ISO_DEFAULT;
	plms_mngt_lost_clear_cbk_call(ent,0/*unmark*/);
	return ret_err;
}
/******************************************************************************
@brief		: Isolate the entity and clear the readiness flags(admin pending 
		and isolate pending).
@param[in]	: ent - Entity to be isolated.
@return		: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
******************************************************************************/
SaUint32T plms_isolate_and_mngt_lost_clear(PLMS_ENTITY *ent)
{
	SaUint32T ret_err;
	SaUint32T is_iso = 0;

	/* If the operational state is disbled, then isolate the
	entity. Otherwise clear the management lost flag.*/
	if ( ((PLMS_HE_ENTITY == ent->entity_type) &&
	(SA_PLM_OPERATIONAL_DISABLED ==
	ent->entity.he_entity.saPlmHEOperationalState)) ||
	((PLMS_EE_ENTITY == ent->entity_type) &&
	(SA_PLM_OPERATIONAL_DISABLED ==
	ent->entity.ee_entity.saPlmEEOperationalState))) {
		
		ret_err = plms_ent_isolate(ent,FALSE,FALSE);
		if (NCSCC_RC_SUCCESS == ret_err)
			is_iso = 1;
	}else{
		is_iso = 1;
		ret_err = NCSCC_RC_FAILURE;
	}
	if (is_iso){
		TRACE("Clear management lost flag,set for ent isolation.\
		Ent: %s",ent->dn_name_str);
		
		plms_readiness_flag_mark_unmark(ent,
		SA_PLM_RF_MANAGEMENT_LOST,0/*unmark*/,NULL,
		SA_NTF_OBJECT_OPERATION,
		SA_PLM_NTFID_STATE_CHANGE_ROOT);

		/* Clear admin_pending as well as 
		isolate_pending flag if set.*/
		plms_readiness_flag_mark_unmark(ent,
		SA_PLM_RF_ISOLATE_PENDING,0/*unmark*/,
		NULL, SA_NTF_OBJECT_OPERATION,
		SA_PLM_NTFID_STATE_CHANGE_ROOT);
		
		plms_readiness_flag_mark_unmark(ent,
		SA_PLM_RF_ADMIN_OPERATION_PENDING,0/*unmark*/,
		NULL, SA_NTF_OBJECT_OPERATION,
		SA_PLM_NTFID_STATE_CHANGE_ROOT);

		ent->mngt_lost_tri = PLMS_MNGT_NONE;
		plms_mngt_lost_clear_cbk_call(ent,0/*unmark*/);
	}else{
		LOG_ER("Management-lost flag clear(ent_iso) FAILED. Ent: %s",
		ent->dn_name_str);
	}
	return ret_err;
}

/******************************************************************************
@brief		: Call readiness callback when dep-imminet-flag is set for an
		Entity, when that entity is enabled after instantiation.
@param[in]	: ent - Entity.
		  aff_ent_list - Affected entity list.
@return		: None.
******************************************************************************/
void plms_insted_dep_immi_failure_cbk_call(PLMS_ENTITY *ent, PLMS_GROUP_ENTITY *aff_ent_list)
{
	PLMS_TRACK_INFO trk_info;
	
	
	memset(&trk_info,0,sizeof(PLMS_TRACK_INFO));
	trk_info.aff_ent_list = NULL;
	trk_info.group_info_list = NULL;
	
	plms_ent_grp_list_add(ent,&(trk_info.group_info_list));
	plms_ent_exp_rdness_state_ow(ent);
	if (NULL != aff_ent_list){
		trk_info.aff_ent_list = aff_ent_list;
		plms_ent_list_grp_list_add(aff_ent_list,&(trk_info.group_info_list));
		plms_aff_ent_exp_rdness_state_ow(aff_ent_list);
	}
	
	trk_info.change_step = SA_PLM_CHANGE_COMPLETED;
	trk_info.track_cause = SA_PLM_CAUSE_IMMINENT_FAILURE;
	trk_info.root_correlation_id = SA_NTF_IDENTIFIER_UNUSED;
	trk_info.grp_op = SA_PLM_GROUP_MEMBER_READINESS_CHANGE;
	trk_info.root_entity = ent;
	plms_cbk_call(&trk_info,1);

	/* *Callback is called. Clean up job.*/
	plms_aff_ent_exp_rdness_status_clear(aff_ent_list);
	plms_ent_exp_rdness_status_clear(ent);
	plms_ent_grp_list_free(trk_info.group_info_list);
	plms_ent_list_free(trk_info.aff_ent_list);

	return;
}

/******************************************************************************
@brief		: Call readiness callback if dependency flag is set. 
@param[in]	: ent - Entity.
@return		: None.
******************************************************************************/
void plms_is_dep_set_cbk_call(PLMS_ENTITY *ent)
{
	PLMS_TRACK_INFO trk_info;
	
	memset(&trk_info,0,sizeof(PLMS_TRACK_INFO));
	trk_info.aff_ent_list = NULL;
	trk_info.group_info_list = NULL;
	
	if (((NULL != ent->parent) && (plms_is_rdness_state_set(ent->parent,SA_PLM_READINESS_OUT_OF_SERVICE))) || 
	(!plms_min_dep_is_ok(ent))){
		if (!plms_rdness_flag_is_set(ent,SA_PLM_RF_DEPENDENCY)){
				
			plms_readiness_flag_mark_unmark(ent,SA_PLM_RF_DEPENDENCY,1/*mark*/,
			NULL,SA_NTF_OBJECT_OPERATION,SA_PLM_NTFID_STATE_CHANGE_ROOT);

			plms_ent_grp_list_add(ent,&(trk_info.group_info_list));
			plms_ent_exp_rdness_state_ow(ent);

			trk_info.change_step = SA_PLM_CHANGE_COMPLETED;
			trk_info.track_cause = SA_PLM_CAUSE_EE_INSTANTIATED;
			trk_info.root_correlation_id = SA_NTF_IDENTIFIER_UNUSED;
			trk_info.grp_op = SA_PLM_GROUP_MEMBER_READINESS_CHANGE;
			trk_info.root_entity = ent;
			plms_cbk_call(&trk_info,1);
			/* Clean up part.*/
			plms_ent_exp_rdness_status_clear(ent);
			plms_ent_grp_list_free(trk_info.group_info_list);
		}
	}
	return;
}
