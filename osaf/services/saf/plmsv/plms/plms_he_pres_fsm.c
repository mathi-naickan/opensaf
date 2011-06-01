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
@file	: plms_he_pres_fsm.c

@brief	: HE presence state FSM.

@author	:  Emerson Network Power.
*****************************************************************************/

#include "plms.h"
#include "plms_utils.h"
#include "plms_notifications.h"

EXTERN_C PLMS_PRES_FUNC_PTR plms_HE_pres_state_op[SA_PLM_HE_PRES_STATE_MAX]
						[SA_PLM_HPI_HE_PRES_STATE_MAX];

static SaUint32T plms_HE_inact_np_to_inspending(PLMS_EVT *);
static SaUint32T plms_HE_inact_np_to_act(PLMS_EVT *);
static SaUint32T plms_HE_inact_to_extpending_op(PLMS_EVT *);
static SaUint32T plms_HE_inact_to_np_op(PLMS_EVT *);
static SaUint32T plms_HE_inact_to_inact_op(PLMS_EVT *);

static SaUint32T plms_HE_actving_to_act_op(PLMS_EVT *);
static SaUint32T plms_HE_actving_to_extpending_op(PLMS_EVT *);
static SaUint32T plms_HE_actving_to_inact_op(PLMS_EVT *);
static SaUint32T plms_HE_actving_to_np_op(PLMS_EVT *);

static SaUint32T plms_HE_act_to_extpending_op(PLMS_EVT *);
static SaUint32T plms_HE_act_to_inact_op(PLMS_EVT *);
static SaUint32T plms_HE_act_to_np_op(PLMS_EVT *);
static SaUint32T plms_HE_act_to_inspending_op(PLMS_EVT *);
static SaUint32T plms_HE_act_to_act_op(PLMS_EVT *);

static SaUint32T plms_HE_deacting_to_inact_op(PLMS_EVT *);
static SaUint32T plms_HE_deacting_to_inspending_op(PLMS_EVT *);
static SaUint32T plms_HE_deacting_to_act_op(PLMS_EVT *);
static SaUint32T plms_HE_deacting_to_np_op(PLMS_EVT *);

static SaUint32T plms_HE_pres_state_invalid(PLMS_EVT *);

static SaUint32T plms_he_insvc_to_acting_process(PLMS_ENTITY *);
static SaUint32T plms_adm_cnxt_clean_up(PLMS_ENTITY *);
static SaUint32T plms_he_verify(PLMS_HPI_EVT *, SaPlmHETypeT **,PLMS_ENTITY **);
static SaUint32T plms_he_active_process(PLMS_ENTITY *);
static SaUint32T plms_inv_data_compare(PLMS_INV_DATA, PLMS_INV_DATA);
static SaUint32T plms_hrb_req(PLMS_ENTITY *,PLMS_HPI_CMD ,SaUint32T );

static SaUint32T plms_extpending_mngt_flag_clear(PLMS_ENTITY *);
static SaUint32T plms_inspending_mngt_flag_clear(PLMS_ENTITY *);
static SaUint32T plms_deact_resp_mngt_flag_clear(PLMS_ENTITY *);
static SaUint32T plms_act_resp_mngt_flag_clear(PLMS_ENTITY *);


/******************************************************************************
@brief		: Initializes the HE presence state FSM function pointers.
@param[in]	: plms_HE_pres_state_op - Array of function pointers of type 
                  PLM_PRES_FUNC_PTR representing HE presence state FSM. 
@return		: None.
******************************************************************************/
void plms_he_pres_fsm_init(PLMS_PRES_FUNC_PTR plms_HE_pres_state_op[]
						[SA_PLM_HPI_HE_PRES_STATE_MAX])
{
	TRACE_ENTER2();

	plms_HE_pres_state_op[SA_PLM_HE_PRESENCE_NOT_PRESENT] 
	[SAHPI_HS_STATE_INSERTION_PENDING] = plms_HE_inact_np_to_inspending;
	plms_HE_pres_state_op[SA_PLM_HE_PRESENCE_NOT_PRESENT]
		[SAHPI_HS_STATE_ACTIVE] = plms_HE_inact_np_to_act;
	plms_HE_pres_state_op[SA_PLM_HE_PRESENCE_NOT_PRESENT]
	[SAHPI_HS_STATE_EXTRACTION_PENDING] = plms_HE_pres_state_invalid;
	plms_HE_pres_state_op[SA_PLM_HE_PRESENCE_NOT_PRESENT]
		[SAHPI_HS_STATE_NOT_PRESENT] = plms_HE_pres_state_invalid;
	plms_HE_pres_state_op[SA_PLM_HE_PRESENCE_NOT_PRESENT]
		[SAHPI_HS_STATE_INACTIVE] = plms_HE_pres_state_invalid;
		
	plms_HE_pres_state_op[SA_PLM_HE_PRESENCE_INACTIVE] 
	[SAHPI_HS_STATE_INSERTION_PENDING] = plms_HE_inact_np_to_inspending;
	plms_HE_pres_state_op[SA_PLM_HE_PRESENCE_INACTIVE]
		[SAHPI_HS_STATE_ACTIVE] = plms_HE_inact_np_to_act;
	plms_HE_pres_state_op[SA_PLM_HE_PRESENCE_INACTIVE]
	[SAHPI_HS_STATE_EXTRACTION_PENDING] = plms_HE_inact_to_extpending_op;
	plms_HE_pres_state_op[SA_PLM_HE_PRESENCE_INACTIVE]
		[SAHPI_HS_STATE_NOT_PRESENT] = plms_HE_inact_to_np_op;
	plms_HE_pres_state_op[SA_PLM_HE_PRESENCE_INACTIVE]
		[SAHPI_HS_STATE_INACTIVE] = plms_HE_inact_to_inact_op;


	plms_HE_pres_state_op[SA_PLM_HE_PRESENCE_ACTIVATING]
		[SAHPI_HS_STATE_ACTIVE] = plms_HE_actving_to_act_op;
	plms_HE_pres_state_op[SA_PLM_HE_PRESENCE_ACTIVATING]
	[SAHPI_HS_STATE_EXTRACTION_PENDING] = plms_HE_actving_to_extpending_op;
	plms_HE_pres_state_op[SA_PLM_HE_PRESENCE_ACTIVATING]
		[SAHPI_HS_STATE_INACTIVE] = plms_HE_actving_to_inact_op;
	plms_HE_pres_state_op[SA_PLM_HE_PRESENCE_ACTIVATING]
		[SAHPI_HS_STATE_NOT_PRESENT] = plms_HE_actving_to_np_op;
	plms_HE_pres_state_op[SA_PLM_HE_PRESENCE_ACTIVATING]
		[SAHPI_HS_STATE_INSERTION_PENDING] = plms_HE_pres_state_invalid;
			  

	plms_HE_pres_state_op[SA_PLM_HE_PRESENCE_ACTIVE]
	[SAHPI_HS_STATE_EXTRACTION_PENDING] = plms_HE_act_to_extpending_op;
	plms_HE_pres_state_op[SA_PLM_HE_PRESENCE_ACTIVE]
		[SAHPI_HS_STATE_INACTIVE] = plms_HE_act_to_inact_op;
	plms_HE_pres_state_op[SA_PLM_HE_PRESENCE_ACTIVE]
		[SAHPI_HS_STATE_NOT_PRESENT] = plms_HE_act_to_np_op;
	plms_HE_pres_state_op[SA_PLM_HE_PRESENCE_ACTIVE]
	[SAHPI_HS_STATE_INSERTION_PENDING] = plms_HE_act_to_inspending_op;
	plms_HE_pres_state_op[SA_PLM_HE_PRESENCE_ACTIVE]
		[SAHPI_HS_STATE_ACTIVE] = plms_HE_act_to_act_op;


	plms_HE_pres_state_op[SA_PLM_HE_PRESENCE_DEACTIVATING]
		[SAHPI_HS_STATE_INACTIVE] = plms_HE_deacting_to_inact_op;
	plms_HE_pres_state_op[SA_PLM_HE_PRESENCE_DEACTIVATING]
	[SAHPI_HS_STATE_INSERTION_PENDING] = plms_HE_deacting_to_inspending_op;
	plms_HE_pres_state_op[SA_PLM_HE_PRESENCE_DEACTIVATING]
		[SAHPI_HS_STATE_ACTIVE] = plms_HE_deacting_to_act_op;
	plms_HE_pres_state_op[SA_PLM_HE_PRESENCE_DEACTIVATING]
		[SAHPI_HS_STATE_NOT_PRESENT] = plms_HE_deacting_to_np_op;
	plms_HE_pres_state_op[SA_PLM_HE_PRESENCE_DEACTIVATING]
	[SAHPI_HS_STATE_EXTRACTION_PENDING] = plms_HE_pres_state_invalid;

	TRACE_LEAVE2();
}
     
/******************************************************************************
@brief		: M0/M1->M2/M3 valid transition for 5 state and 5 partial state model. 
		1. Deactivate the board if the admin state is lock-inactive.
		2. Nothing to do for partial 5 state model.
		3. Activate the board for full 5 state model, as HS/HRB would have cancelled
		the activation timer.
	
@param[in]	: evt - PLMS_EVT type representing HPI event.

@return		: NCSCC_RC_FAILURE/NCSCC_RC_SUCCESS
******************************************************************************/
static SaUint32T plms_HE_inact_np_to_inspending(PLMS_EVT *evt)
{
	PLMS_CB *cb = plms_cb;
	SaUint32T ret_err = NCSCC_RC_FAILURE;
	PLMS_EPATH_TO_ENTITY_MAP_INFO *epath_to_ent;
	PLMS_ENTITY *ent;

	TRACE_ENTER2("Entity: %s",evt->req_evt.hpi_evt.entity_path);
	
	epath_to_ent = (PLMS_EPATH_TO_ENTITY_MAP_INFO *)ncs_patricia_tree_get
				(&(cb->epath_to_entity_map_info),
				(SaUint8T *)&(evt->req_evt.hpi_evt.epath_key));
	if (NULL == epath_to_ent ) {
		LOG_ER("Entity corresponding to ent_path %s not found. Possibly\
		the HE is yet to be verified.",
		evt->req_evt.hpi_evt.entity_path);

		TRACE_LEAVE2("Return Val: %d",ret_err);
		return ret_err;
	}
	
	ent = epath_to_ent->plms_entity;

	if (NULL == ent){
		LOG_ER("Entity path found but epath_to_ent->plms_entity is \
		NULL. Ent_Path: %s", evt->req_evt.hpi_evt.entity_path);

		TRACE_LEAVE2("Return Val: %d",ret_err);
		return ret_err;
	}
	/* Mark the presence state of the HE to activating.*/
	plms_presence_state_set(ent,SA_PLM_HE_PRESENCE_ACTIVATING,NULL,
					SA_NTF_OBJECT_OPERATION,
					SA_PLM_NTFID_STATE_CHANGE_ROOT);
	/* If admin state is lckinact, then move this entity back to inact
	state.*/
	if (SA_PLM_HE_ADMIN_LOCKED_INACTIVE  == 
		ent->entity.he_entity.saPlmHEAdminState){
		
		ret_err = plms_he_deactivate(ent,0,1);
		LOG_ER("Entity is deactivated as the\
		admin state is lckinact. Ent: %s",ent->dn_name_str);
		
		TRACE_LEAVE2("Return Val: %d",ret_err);
		return ret_err;
	}
	/* Managemnet lost flag handling.*/
	if(plms_rdness_flag_is_set(ent,SA_PLM_RF_MANAGEMENT_LOST)){

		if ( NCSCC_RC_SUCCESS != plms_inspending_mngt_flag_clear(ent))
			return ret_err;
	}
	/* Activate the board only if the board supports full 5 state model.*/
	if (PLMS_HPI_FULL_FIVE_HOTSWAP_MODEL == ent->state_model){
		/* Activate the HE. */
		ret_err = plms_ent_enable(ent,FALSE,TRUE);
		if (NCSCC_RC_SUCCESS != ret_err){
			LOG_ER("HE %s activation FAILED.",ent->dn_name_str);
		}else{
			TRACE("HE %s activation SUCCESSFUL",ent->dn_name_str);
		}
	}

	TRACE_LEAVE2("Return Val: %d",ret_err);
	return ret_err;
}
  
/******************************************************************************
@brief		: Valid for 
		1. M0/M1->M4 for 3 state model.
		2. M0/M1->M2/M3->M4 for 5 state and 5 partial state when M2/M3 is missed. 

		Deactivate the board if the admin state is lock-inactive. Otherwise
		mark the presence state to active and take the board to insvc if
		other conditions are satisfied.

@param[in]	: evt - PLMS_EVT type representing HPI event.

@return		: NCSCC_RC_FAILURE/NCSCC_RC_SUCCESS
******************************************************************************/
static SaUint32T plms_HE_inact_np_to_act( PLMS_EVT *evt)
{
	
	SaUint32T ret_err = NCSCC_RC_FAILURE;
	PLMS_CB *cb = plms_cb;
	PLMS_EPATH_TO_ENTITY_MAP_INFO *epath_to_ent;
	PLMS_ENTITY *ent;

	TRACE_ENTER2("Entity: %s",evt->req_evt.hpi_evt.entity_path);

	
	epath_to_ent = (PLMS_EPATH_TO_ENTITY_MAP_INFO *)ncs_patricia_tree_get
				(&(cb->epath_to_entity_map_info),
				(SaUint8T *)&(evt->req_evt.hpi_evt.epath_key));
	if (NULL == epath_to_ent ) {
		LOG_ER("Entity corresponding to ent_path %s not found. Possibly\
			the HE is yet to be verified.",
			evt->req_evt.hpi_evt.entity_path);
		TRACE_LEAVE2("Return Val: %d",NCSCC_RC_FAILURE);
		return NCSCC_RC_FAILURE;
	}
	
	ent = epath_to_ent->plms_entity;

	if (NULL == ent){
		LOG_ER("Entity path %s found but epath_to_ent->plms_entity is \
				NULL", evt->req_evt.hpi_evt.entity_path);
		TRACE_LEAVE2("Return Val: %d",NCSCC_RC_FAILURE);
		return NCSCC_RC_FAILURE;
	}

	ent->act_in_pro = FALSE;
	/* Mark the presence state of the HE to active.*/
	plms_presence_state_set(ent,SA_PLM_HE_PRESENCE_ACTIVE,
				NULL,SA_NTF_OBJECT_OPERATION,
				SA_PLM_NTFID_STATE_CHANGE_ROOT);
	/* If the entity is in lock-inactive state, then deactivate
	the entity.*/
	if (SA_PLM_HE_ADMIN_LOCKED_INACTIVE == 
			ent->entity.he_entity.saPlmHEAdminState){

		ret_err = plms_he_deactivate(ent,0,1);
		LOG_ER("Entity is deactivated as the admin state is lckinact. \
		Ent: %s",ent->dn_name_str);

		TRACE_LEAVE2("Return Val: %d",ret_err);
		return ret_err;
	}
	
	/* Managemnet lost flag handling.*/
	if(plms_rdness_flag_is_set(ent,SA_PLM_RF_MANAGEMENT_LOST)){

		if ( NCSCC_RC_SUCCESS != plms_act_resp_mngt_flag_clear(ent))
			return ret_err;
	}
	
	/* Process M4 event.*/
	ret_err = plms_he_active_process(ent);

	TRACE_LEAVE2("Return Val: %d",ret_err);
	return ret_err;
}

/******************************************************************************
@brief		: M1->M2/M3->M4->M5/M6 valid for 5 state and 5 partial state.
		This transition does not make much sence as the HE never went to 
		activated state. But for 5 state model, HSM would have canceled the
		extraction timer,just deactivate the HE.

@param[in]	: evt - PLMS_EVT type representing HPI event.

@return		: NCSCC_RC_FAILURE/NCSCC_RC_SUCCESS
******************************************************************************/
static SaUint32T plms_HE_inact_to_extpending_op( PLMS_EVT *evt)
{
	SaUint32T ret_err = NCSCC_RC_FAILURE;
	PLMS_CB *cb = plms_cb;
	PLMS_EPATH_TO_ENTITY_MAP_INFO *epath_to_ent;
	PLMS_ENTITY *ent;

	TRACE_ENTER2("Entity: %s",evt->req_evt.hpi_evt.entity_path);
	
	epath_to_ent = (PLMS_EPATH_TO_ENTITY_MAP_INFO *)ncs_patricia_tree_get
				(&(cb->epath_to_entity_map_info),
				(SaUint8T *)&(evt->req_evt.hpi_evt.epath_key));
	if (NULL == epath_to_ent ) {
		LOG_ER("Entity corresponding to ent_path %s not found. Possibly\
			the HE is yet to be verified.",
			evt->req_evt.hpi_evt.entity_path);
		TRACE_LEAVE2("Return Val: %d",NCSCC_RC_FAILURE);
		return NCSCC_RC_FAILURE;
	}
	
	ent = epath_to_ent->plms_entity;

	if (NULL == ent){
		LOG_ER("Entity path %s found but epath_to_ent->plms_entity is \
				NULL", evt->req_evt.hpi_evt.entity_path);
		TRACE_LEAVE2("Return Val: %d",NCSCC_RC_FAILURE);
		return NCSCC_RC_FAILURE;
	}

	/* Mark the presence state of the HE to deactivating.*/
	plms_presence_state_set(ent, SA_PLM_HE_PRESENCE_DEACTIVATING,NULL,
					SA_NTF_OBJECT_OPERATION,
					SA_PLM_NTFID_STATE_CHANGE_ROOT);
	
	if(plms_rdness_flag_is_set(ent,SA_PLM_RF_MANAGEMENT_LOST)){
		if ( NCSCC_RC_SUCCESS != plms_extpending_mngt_flag_clear(ent))
			return NCSCC_RC_FAILURE;
	}		

	ret_err = plms_he_deactivate(ent,0,1);
	if (NCSCC_RC_SUCCESS != ret_err){
		LOG_ER("HE %s deactivation FAILED.",ent->dn_name_str);
	}else{
		TRACE("HE %s deactivation SUCCESSFUL",ent->dn_name_str);
	}

	TRACE_LEAVE2("Return Val: %d",ret_err);
	return ret_err;
}
/******************************************************************************
@brief		: M0<-M1 valid for 3 state, 5 state and 5 partial state model.
		1. Clear all the flags.
		2. Wipe out the current entity path.
		3. Wipe out the current HETpye.

@param[in]	: evt - PLMS_EVT type representing HPI event.

@return		: NCSCC_RC_FAILURE/NCSCC_RC_SUCCESS
******************************************************************************/
static SaUint32T plms_HE_inact_to_np_op(PLMS_EVT *evt)
{
	SaUint32T ret_err = NCSCC_RC_FAILURE;
	PLMS_CB *cb = plms_cb;
	PLMS_EPATH_TO_ENTITY_MAP_INFO *epath_to_ent;
	PLMS_ENTITY *ent;
	
	epath_to_ent = (PLMS_EPATH_TO_ENTITY_MAP_INFO *)ncs_patricia_tree_get
				(&(cb->epath_to_entity_map_info),
				(SaUint8T *)&(evt->req_evt.hpi_evt.epath_key));
	if (NULL == epath_to_ent ) {
		LOG_ER("Entity corresponding to ent_path %s not found. Possibly\
			the HE is yet to be verified.",
			evt->req_evt.hpi_evt.entity_path);
		TRACE_LEAVE2("Return Val: %d",NCSCC_RC_FAILURE);
		return NCSCC_RC_FAILURE;
	}
	
	ent = epath_to_ent->plms_entity;

	if (NULL == ent){
		LOG_ER("Entity path %s found but epath_to_ent->plms_entity is \
				NULL", evt->req_evt.hpi_evt.entity_path);
		TRACE_LEAVE2("Return Val: %d",NCSCC_RC_FAILURE);
		return NCSCC_RC_FAILURE;
	}

	ret_err = plms_he_oos_to_np_process(ent);
	
	TRACE_LEAVE2("Return Val: %d",ret_err);
	return ret_err;
}
/******************************************************************************
@brief		: M1->M1 transition. This is the situation when HPI rediscovers the
		resources. PLMS uses this to clear the management lost flag if set for
		that HE.

@param[in]	: evt - PLMS_EVT type representing HPI event.

@return		: NCSCC_RC_FAILURE/NCSCC_RC_SUCCESS
******************************************************************************/
static SaUint32T plms_HE_inact_to_inact_op(PLMS_EVT *evt)
{
	PLMS_CB *cb = plms_cb;
	PLMS_EPATH_TO_ENTITY_MAP_INFO *epath_to_ent;
	PLMS_ENTITY *ent;

	TRACE_ENTER2("Entity: %s",evt->req_evt.hpi_evt.entity_path);
	
	epath_to_ent = (PLMS_EPATH_TO_ENTITY_MAP_INFO *)ncs_patricia_tree_get
				(&(cb->epath_to_entity_map_info),
				(SaUint8T *)&(evt->req_evt.hpi_evt.epath_key));
	if (NULL == epath_to_ent ) {
		LOG_ER("Entity corresponding to ent_path %s not found. Possibly\
			the HE is yet to be verified.",
			evt->req_evt.hpi_evt.entity_path);
		TRACE_LEAVE2("Return Val: %d",NCSCC_RC_FAILURE);
		return NCSCC_RC_FAILURE;
	}
	
	ent = epath_to_ent->plms_entity;

	if (NULL == ent){
		LOG_ER("Entity path %s found but epath_to_ent->plms_entity is \
				NULL", evt->req_evt.hpi_evt.entity_path);
		TRACE_LEAVE2("Return Val: %d",NCSCC_RC_FAILURE);
		return NCSCC_RC_FAILURE;
	}
	ent->deact_in_pro = FALSE;

	/* Handle clearing management lost flag.*/
	if (plms_rdness_flag_is_set(ent,SA_PLM_RF_MANAGEMENT_LOST)){
		if ( NCSCC_RC_SUCCESS != plms_deact_resp_mngt_flag_clear(ent))
			TRACE_LEAVE2("Return Val: %d",NCSCC_RC_FAILURE);
			return NCSCC_RC_FAILURE;
	}
	
	TRACE_LEAVE2("Return Val: %d",NCSCC_RC_SUCCESS);
	return NCSCC_RC_SUCCESS;
}
/******************************************************************************
@brief		: M2/M3->M4 valid for 5 state and 5 partial state model. 
		1. Mark the presence state to activated.
		2. Take the HE to insvc if all required conditions are
		satisfied.

@param[in]	: evt - PLMS_EVT type representing HPI event.

@return		: NCSCC_RC_FAILURE/NCSCC_RC_SUCCESS
******************************************************************************/
static SaUint32T plms_HE_actving_to_act_op(PLMS_EVT *evt)
{
	SaUint32T ret_err = NCSCC_RC_FAILURE;
	PLMS_CB *cb = plms_cb;
	PLMS_EPATH_TO_ENTITY_MAP_INFO *epath_to_ent;
	PLMS_ENTITY *ent;

	TRACE_ENTER2("Entity: %s",evt->req_evt.hpi_evt.entity_path);
	
	epath_to_ent = (PLMS_EPATH_TO_ENTITY_MAP_INFO *)ncs_patricia_tree_get
				(&(cb->epath_to_entity_map_info),
				(SaUint8T *)&(evt->req_evt.hpi_evt.epath_key));
	if (NULL == epath_to_ent ) {
		LOG_ER("Entity corresponding to ent_path %s not found. Possibly\
			the HE is yet to be verified.",
			evt->req_evt.hpi_evt.entity_path);
		TRACE_LEAVE2("Return Val: %d",NCSCC_RC_FAILURE);
		return NCSCC_RC_FAILURE;
	}
	
	ent = epath_to_ent->plms_entity;

	if (NULL == ent){
		LOG_ER("Entity path %s found but epath_to_ent->plms_entity is \
				NULL", evt->req_evt.hpi_evt.entity_path);
		TRACE_LEAVE2("Return Val: %d",NCSCC_RC_FAILURE);
		return NCSCC_RC_FAILURE;
	}

	ent->act_in_pro = FALSE;
	/* Mark the presence state of the HE to active.*/
	plms_presence_state_set(ent,SA_PLM_HE_PRESENCE_ACTIVE,
				NULL,SA_NTF_OBJECT_OPERATION,
				SA_PLM_NTFID_STATE_CHANGE_ROOT);
	/* Managemnet lost flag handling.*/
	if(plms_rdness_flag_is_set(ent,SA_PLM_RF_MANAGEMENT_LOST)){

		if ( NCSCC_RC_SUCCESS != plms_act_resp_mngt_flag_clear(ent))
			return ret_err;
	}

	/* Process M4 event.*/
	ret_err = plms_he_active_process(ent);

	TRACE_LEAVE2("Return Val: %d",ret_err);
	return ret_err;
	
}

/******************************************************************************
@brief		: M2/M3->M4->M5/M6. Valid for 5 state and 5 partial state
		model when M4 is missed.
		
		This transition does not make much sence. On getting M2/M3,
		the HE would have been activated. But PLMS missed M4. So the HE
		is never moved to insvc. Hence nothing to do. HSM would have 
		cancelled extaction timer. Just deactivate the board.

@param[in]	: evt - PLMS_EVT type representing HPI event.

@return		: NCSCC_RC_FAILURE/NCSCC_RC_SUCCESS
******************************************************************************/
static SaUint32T plms_HE_actving_to_extpending_op(PLMS_EVT *evt)
{
	SaUint32T ret_err = NCSCC_RC_FAILURE;
	PLMS_CB *cb = plms_cb;
	PLMS_EPATH_TO_ENTITY_MAP_INFO *epath_to_ent;
	PLMS_ENTITY *ent;

	TRACE_ENTER2("Entity: %s",evt->req_evt.hpi_evt.entity_path);
	
	epath_to_ent = (PLMS_EPATH_TO_ENTITY_MAP_INFO *)ncs_patricia_tree_get
				(&(cb->epath_to_entity_map_info),
				(SaUint8T *)&(evt->req_evt.hpi_evt.epath_key));
	if (NULL == epath_to_ent ) {
		LOG_ER("Entity corresponding to ent_path %s not found. Possibly\
			the HE is yet to be verified.",
			evt->req_evt.hpi_evt.entity_path);
		TRACE_LEAVE2("Return Val: %d",NCSCC_RC_FAILURE);
		return NCSCC_RC_FAILURE;
	}
	
	ent = epath_to_ent->plms_entity;

	if (NULL == ent){
		LOG_ER("Entity path %s found but epath_to_ent->plms_entity is \
				NULL", evt->req_evt.hpi_evt.entity_path);
		TRACE_LEAVE2("Return Val: %d",NCSCC_RC_FAILURE);
		return NCSCC_RC_FAILURE;
	}
	
	
	/* Mark the presence state of the HE to Deactivating.*/
	plms_presence_state_set(ent,SA_PLM_HE_PRESENCE_DEACTIVATING,NULL,
						SA_NTF_OBJECT_OPERATION,
						SA_PLM_NTFID_STATE_CHANGE_ROOT);

	if(plms_rdness_flag_is_set(ent,SA_PLM_RF_MANAGEMENT_LOST)){
		if ( NCSCC_RC_SUCCESS != plms_extpending_mngt_flag_clear(ent))
			return NCSCC_RC_FAILURE;
	}		

	/* Deactivate the HE. */
	ret_err =plms_he_deactivate(ent,FALSE/*adm_op*/,TRUE/*mngt_cbk*/);

	if (NCSCC_RC_SUCCESS != ret_err){
		LOG_ER("HE %s deactivation FAILED.",ent->dn_name_str);
	}else{
		TRACE("HE %s deactivation SUCCESSFUL",ent->dn_name_str);
	}

	TRACE_LEAVE2("Return Val: %d",ret_err);
	return ret_err;

}
/******************************************************************************
@brief		: M1<-M2/M3. The HE is still in OOS. Hence nothing much to
		do. Mark the presence state to Inactive and return.	
	        
		Use Case:
		1. The HE is in activating state(M2 in HPI world) and admin
		deactivate is issued. The HE will be moved back to M1. 

@param[in]	: evt - PLMS_EVT type representing HPI event.

@return		: NCSCC_RC_FAILURE/NCSCC_RC_SUCCESS
******************************************************************************/
static SaUint32T plms_HE_actving_to_inact_op(PLMS_EVT *evt)
{
	SaUint32T ret_err = NCSCC_RC_SUCCESS;
	PLMS_CB *cb = plms_cb;
	PLMS_EPATH_TO_ENTITY_MAP_INFO *epath_to_ent;
	PLMS_ENTITY *ent;

	TRACE_ENTER2("Entity: %s",evt->req_evt.hpi_evt.entity_path);
	
	epath_to_ent = (PLMS_EPATH_TO_ENTITY_MAP_INFO *)ncs_patricia_tree_get
				(&(cb->epath_to_entity_map_info),
				(SaUint8T *)&(evt->req_evt.hpi_evt.epath_key));
	if (NULL == epath_to_ent ) {
		LOG_ER("Entity corresponding to ent_path %s not found. Possibly\
			the HE is yet to be verified.",
			evt->req_evt.hpi_evt.entity_path);
		TRACE_LEAVE2("Return Val: %d",NCSCC_RC_FAILURE);
		return NCSCC_RC_FAILURE;
	}
	
	ent = epath_to_ent->plms_entity;

	if (NULL == ent){
		LOG_ER("Entity path %s found but epath_to_ent->plms_entity is \
				NULL", evt->req_evt.hpi_evt.entity_path);
		TRACE_LEAVE2("Return Val: %d",NCSCC_RC_FAILURE);
		return NCSCC_RC_FAILURE;
	}

	ent->deact_in_pro = FALSE;
	ent->act_in_pro = FALSE;

	/* Mark the presence state of the HE to inactive.*/
	plms_presence_state_set(ent,SA_PLM_HE_PRESENCE_INACTIVE,NULL,
					SA_NTF_OBJECT_OPERATION,
					SA_PLM_NTFID_STATE_CHANGE_ROOT);

	/* Handle clearing management lost flag.*/
	if (plms_rdness_flag_is_set(ent,SA_PLM_RF_MANAGEMENT_LOST)){
		plms_deact_resp_mngt_flag_clear(ent);
	}

	return ret_err;
}

/******************************************************************************
@brief		: M0<-M1<-M2/M3. Valid for 5 state and 5 partial state model. 
		1. Clear all the flags.
		2. Wipe out the current entity path.
		3. Wipe out the current HEType.

@param[in]	: evt - PLMS_EVT type representing HPI event.

@return		: NCSCC_RC_FAILURE/NCSCC_RC_SUCCESS
******************************************************************************/
static SaUint32T plms_HE_actving_to_np_op(PLMS_EVT *evt)
{
	SaUint32T ret_err = NCSCC_RC_FAILURE;
	PLMS_CB *cb = plms_cb;
	PLMS_EPATH_TO_ENTITY_MAP_INFO *epath_to_ent;
	PLMS_ENTITY *ent;

	TRACE_ENTER2("Entity: %s",evt->req_evt.hpi_evt.entity_path);
	
	epath_to_ent = (PLMS_EPATH_TO_ENTITY_MAP_INFO *)ncs_patricia_tree_get
				(&(cb->epath_to_entity_map_info),
				(SaUint8T *)&(evt->req_evt.hpi_evt.epath_key));
	if (NULL == epath_to_ent ) {
		LOG_ER("Entity corresponding to ent_path %s not found. Possibly\
			the HE is yet to be verified.",
			evt->req_evt.hpi_evt.entity_path);
		TRACE_LEAVE2("Return Val: %d",NCSCC_RC_FAILURE);
		return NCSCC_RC_FAILURE;
	}
	
	ent = epath_to_ent->plms_entity;

	if (NULL == ent){
		LOG_ER("Entity path %s found but epath_to_ent->plms_entity is \
				NULL", evt->req_evt.hpi_evt.entity_path);
		TRACE_LEAVE2("Return Val: %d",NCSCC_RC_FAILURE);
		return NCSCC_RC_FAILURE;
	}

	ret_err = plms_he_oos_to_np_process(ent);
	
	TRACE_LEAVE2("Return Val: %d",ret_err);
	return ret_err;
}

/******************************************************************************
@brief		: M4->M5. Valid for both 5 full and partial state model.
		If this ent in any admin operation context then
		1. For 5 partial state model, do the clean up on getting M1.
		2. For 5 state model, reject the deactivation.	

		If the entity is moving to M5 because of its parent, then just change 
		the presence state.
		As we do not have any control over 5-partial state model, dont do 
		anything, only change the presence state. For 5-full state model 
		make sure the callbacks are called depending upon deactivation policy.

@param[in]	: evt - PLMS_EVT type representing HPI event.

@return		: NCSCC_RC_FAILURE/NCSCC_RC_SUCCESS
******************************************************************************/
static SaUint32T plms_HE_act_to_extpending_op( PLMS_EVT *evt)
{
	SaUint32T ret_err = NCSCC_RC_SUCCESS;
	PLMS_CB *cb = plms_cb;
	PLMS_EPATH_TO_ENTITY_MAP_INFO *epath_to_ent;
	PLMS_ENTITY *ent;
	PLMS_TRACK_INFO *trk_info;
	PLMS_GROUP_ENTITY *aff_ent_list = NULL;
	PLMS_GROUP_ENTITY *head = NULL;
	PLMS_ENTITY_GROUP_INFO_LIST *log_head_grp;

	TRACE_ENTER2("Entity: %s",evt->req_evt.hpi_evt.entity_path);
	
	epath_to_ent = (PLMS_EPATH_TO_ENTITY_MAP_INFO *)ncs_patricia_tree_get
				(&(cb->epath_to_entity_map_info),
				(SaUint8T *)&(evt->req_evt.hpi_evt.epath_key));
	if (NULL == epath_to_ent ) {
		LOG_ER("Entity corresponding to ent_path %s not found. Possibly\
			the HE is yet to be verified.",
			evt->req_evt.hpi_evt.entity_path);
		TRACE_LEAVE2("Return Val: %d",NCSCC_RC_FAILURE);
		return NCSCC_RC_FAILURE;
	}
	
	ent = epath_to_ent->plms_entity;

	if (NULL == ent){
		LOG_ER("Entity path %s found but epath_to_ent->plms_entity is \
				NULL", evt->req_evt.hpi_evt.entity_path);
		TRACE_LEAVE2("Return Val: %d",NCSCC_RC_FAILURE);
		return NCSCC_RC_FAILURE;
	}
	plms_presence_state_set(ent,SA_PLM_HE_PRESENCE_DEACTIVATING,
						NULL,SA_NTF_OBJECT_OPERATION,
						SA_PLM_NTFID_STATE_CHANGE_ROOT);
	if(plms_rdness_flag_is_set(ent,SA_PLM_RF_MANAGEMENT_LOST)){
		if ( NCSCC_RC_SUCCESS != plms_extpending_mngt_flag_clear(ent))
			return NCSCC_RC_FAILURE;
	}		

	/* Do not have control for 5 partial state model. So just change
	the state to deactivating and wait for inactive HPI event to
	call the completed callback.
	*/
	if ( PLMS_HPI_PARTIAL_FIVE_HOTSWAP_MODEL == ent->state_model){
		TRACE("Partial 5-state model, return.");
		TRACE_LEAVE2("Return Val: %d",NCSCC_RC_SUCCESS);
		return NCSCC_RC_SUCCESS;
	}
	/* If I am moving to M5 because any of my parent, then just 
	change the presence state.
	*/
	if (plms_is_anc_in_deact_cnxt(ent)){
		/* TODO: Do we need to deactivate the HE.*/
		TRACE("I am moving to inactive because of my parent.");
		TRACE_LEAVE2("Return Val: %d",NCSCC_RC_SUCCESS);
		return NCSCC_RC_SUCCESS;
	}

	plms_affected_ent_list_get(ent,&aff_ent_list,0);

	/* If this entity is in any other operation context then,
	we have option of rejecting the deactivation process.
	*/
	if (plms_anc_chld_dep_adm_flag_is_set(ent,aff_ent_list)){
		TRACE("Reject the deactivation as the ent %s is in other admin\
						context",ent->dn_name_str);
		/* Make the He to move to M4 state.*/
		ret_err = plms_he_activate(ent,FALSE,TRUE);
		if (NCSCC_RC_SUCCESS != ret_err){
			LOG_ER("HE %s activation FAILED.",ent->dn_name_str);
		}else{
			TRACE("HE %s activation SUCCESSFUL",
							ent->dn_name_str);
		}
		plms_ent_list_free(aff_ent_list);
		TRACE_LEAVE2("Return Val: %d",ret_err);
		return NCSCC_RC_SUCCESS;
	}
			
	switch(cb->domain_info.domain.saPlmHEDeactivationPolicy){
	
	case SA_PLM_DP_REJECT_NOT_OOS:
		TRACE("The deactivation policy is REJECT_NOT_OOS");
		if ( SA_PLM_READINESS_OUT_OF_SERVICE != 
			ent->entity.he_entity.saPlmHEReadinessState){
			
			TRACE("The HE %s is not OOS and deactivation policy\
			is REJECT_NOT_OOS, hence reject deactivation and\
			activate the HE",ent->dn_name_str);
			/* Can not be deactivated. 
			Take the HE to activated state.	
			*/
			ret_err = plms_he_activate(ent,FALSE,TRUE);
			if (NCSCC_RC_SUCCESS != ret_err){
				LOG_ER("HE %s activation FAILED.",
							ent->dn_name_str);
			}else{
				TRACE("HE %s activation SUCCESSFUL",
							ent->dn_name_str);
			}

			plms_ent_list_free(aff_ent_list);
			TRACE_LEAVE2("Return Val: %d",ret_err);
			return ret_err;
		}else{
			/* As the HE is already in OOS, nothing much
			to do. Set the HE state to M1.*/
			ret_err = plms_he_deactivate(ent,FALSE,TRUE);
			
			if (NCSCC_RC_SUCCESS != ret_err){
				LOG_ER("HE %s deactivation FAILED.",
				ent->dn_name_str);
			}else{
				TRACE("HE %s deactivation SUCCESSFUL",
				ent->dn_name_str);
			}
			plms_ent_list_free(aff_ent_list);
		
			TRACE_LEAVE2("Return Val: %d",ret_err);
			return ret_err;
		}
		break;

		
	case SA_PLM_DP_VALIDATE:
		TRACE("The deactivation policy is DP_VALIDATE");
		/* If I am already in OOS, then do nothing. */
		if ((SA_PLM_READINESS_OUT_OF_SERVICE == 
			ent->entity.he_entity.saPlmHEReadinessState) || 
			(ent->am_i_aff_ent) ){

			TRACE("HE is already in OOS.");
			
			/* Make the HE to move to InActive.*/	
			ret_err = plms_he_deactivate(ent,FALSE,TRUE);
			
			if (NCSCC_RC_SUCCESS != ret_err){
				LOG_ER("HE %s deactivation FAILED.",
						ent->dn_name_str);
			}else{
				TRACE("HE %s deactivation SUCCESSFUL",
						ent->dn_name_str);
			}

			plms_ent_list_free(aff_ent_list);
			TRACE_LEAVE2("Return Val: %d",NCSCC_RC_SUCCESS);
			return NCSCC_RC_SUCCESS;
		}
		
		trk_info =(PLMS_TRACK_INFO *)calloc(1,sizeof(PLMS_TRACK_INFO));
		if (NULL == trk_info){
			plms_ent_list_free(aff_ent_list);
			LOG_CR("act_to_extpending, calloc FAILED.\
			error: %s",strerror(errno));
			assert(0);
		}
		/* Setting the context of the operation.*/
		ent->adm_op_in_progress = SA_PLM_CAUSE_HE_DEACTIVATION;
		ent->am_i_aff_ent = TRUE;
		plms_aff_ent_mark_unmark(aff_ent_list,TRUE);
		
		
		/* Set the expected readiness status.*/
		ent->exp_readiness_status.readinessState = 
				SA_PLM_READINESS_OUT_OF_SERVICE;

		/* Set exp readiness status for all the affected entities.*/
		head = aff_ent_list;
		while(head){

			head->plm_entity->exp_readiness_status.
			readinessFlags = (head->plm_entity->
			exp_readiness_status.readinessFlags | 
			SA_PLM_RF_DEPENDENCY);
			
			head->plm_entity->exp_readiness_status.
			readinessState = 
			SA_PLM_READINESS_OUT_OF_SERVICE;
			
			head = head->next;
		}
		
		ent->trk_info = trk_info;
		trk_info->group_info_list = NULL;
		trk_info->aff_ent_list = aff_ent_list;
		/* Add the groups, root entity(ent) belong to.*/
		plms_ent_grp_list_add(ent,&(trk_info->group_info_list));
	
		/* Find out all the groups, all affected entities belong to 
		and add the groups to trk_info->group_info_list. 
		TODO: We have to free trk_info->group_info_list. */
		plms_ent_list_grp_list_add(aff_ent_list,
					&(trk_info->group_info_list));	

		TRACE("Affected groups for ent %s: ",ent->dn_name_str);
		log_head_grp = trk_info->group_info_list;
		while(log_head_grp){
			TRACE("%llu,",log_head_grp->ent_grp_inf->entity_grp_hdl);
			log_head_grp = log_head_grp->next;
		}

		/* Fill track info.*/
		trk_info->root_entity = ent;
		trk_info->change_step = SA_PLM_CHANGE_VALIDATE;
		trk_info->track_cause = SA_PLM_CAUSE_HE_DEACTIVATION;
		trk_info->root_correlation_id = SA_NTF_IDENTIFIER_UNUSED;
		trk_info->grp_op = SA_PLM_GROUP_MEMBER_READINESS_CHANGE; 
		
		plms_cbk_call(trk_info,1);

		/* No validate step subscriber, call start cbk.*/
		if (0 == trk_info->track_count){
			plms_deact_start_cbk_call(ent,trk_info);
		}else{
			plms_peer_async_send(ent,SA_PLM_CHANGE_VALIDATE,
			SA_PLM_CAUSE_HE_DEACTIVATION);
			break;
		}

		/* No start step subscriber, call completed cbk.*/
		if (0 == trk_info->track_count){
			plms_deact_completed_cbk_call(ent,trk_info);
		}else{
			plms_peer_async_send(ent,SA_PLM_CHANGE_START,
			SA_PLM_CAUSE_HE_DEACTIVATION);
		}
		break;
		
	case SA_PLM_DP_UNCONDITIONAL:
		TRACE("The deactivation policy is DP_UNCONDITIONAL");
		/* If I am already in OOS, then do nothing. */
		if ((SA_PLM_READINESS_OUT_OF_SERVICE == 
			ent->entity.he_entity.saPlmHEReadinessState) || 
			(ent->am_i_aff_ent) ){

			TRACE("HE is already in OOS.");
			/* Make the HE to move to InActive.*/	
			ret_err = plms_he_deactivate(ent,FALSE,TRUE);
			
			if (NCSCC_RC_SUCCESS != ret_err){
				LOG_ER("HE %s deactivation FAILED.",
						ent->dn_name_str);
			}else{
				TRACE("HE %s deactivation SUCCESSFUL",
						ent->dn_name_str);
			}

			plms_ent_list_free(aff_ent_list);
			TRACE_LEAVE2("Return Val: %d",NCSCC_RC_SUCCESS);
			return NCSCC_RC_SUCCESS;
		}
		
		trk_info =(PLMS_TRACK_INFO *)calloc(1,sizeof(PLMS_TRACK_INFO));
		if (NULL == trk_info){
			plms_ent_list_free(aff_ent_list);
			LOG_CR("act_to_extpending, calloc FAILED.\
			error: %s",strerror(errno));
			assert(0);
		}
		/* Setting the context of the operation.*/
		ent->adm_op_in_progress = SA_PLM_CAUSE_HE_DEACTIVATION;
		ent->am_i_aff_ent = TRUE;
		plms_aff_ent_mark_unmark(aff_ent_list,TRUE);
		
		
		/* Set the expected readiness status.*/
		ent->exp_readiness_status.readinessState = 
				SA_PLM_READINESS_OUT_OF_SERVICE;

		/* Set exp readiness status for all the affected entities.*/
		head = aff_ent_list;
		while(head){
			
			head->plm_entity->exp_readiness_status.
			readinessFlags = (head->plm_entity->
			exp_readiness_status.readinessFlags | 
			SA_PLM_RF_DEPENDENCY);
			
			head->plm_entity->exp_readiness_status.
			readinessState = 
			SA_PLM_READINESS_OUT_OF_SERVICE;
#if 0			
			/* Set presence state of the EEs to terminating.*/
			if (PLMS_EE_ENTITY == head->plm_entity->entity_type){
				plms_presence_state_set(head->plm_entity,
						SA_PLM_EE_PRESENCE_TERMINATING,
						ent,SA_NTF_OBJECT_OPERATION,
						SA_PLM_NTFID_STATE_CHANGE_DEP);

			}
#endif
			head = head->next;
		}
		
		ent->trk_info = trk_info;
		trk_info->group_info_list = NULL;
		trk_info->aff_ent_list = aff_ent_list;
		
		/* Add the groups, root entity(ent) belong to.*/
		plms_ent_grp_list_add(ent,&(trk_info->group_info_list));
	
		/* Find out all the groups, all affected entities belong to 
		and add the groups to trk_info->group_info_list. */ 
		plms_ent_list_grp_list_add(aff_ent_list,
					&(trk_info->group_info_list));	

		TRACE("Affected groups for ent %s: ",ent->dn_name_str);
		log_head_grp = trk_info->group_info_list;
		while(log_head_grp){
			TRACE("%llu,",log_head_grp->ent_grp_inf->entity_grp_hdl);
			log_head_grp = log_head_grp->next;
		}

		/* Fill track info.*/
		trk_info->root_entity = ent;
		trk_info->change_step = SA_PLM_CHANGE_START;
		trk_info->track_cause = SA_PLM_CAUSE_HE_DEACTIVATION;
		trk_info->root_correlation_id = SA_NTF_IDENTIFIER_UNUSED;
		trk_info->grp_op = SA_PLM_GROUP_MEMBER_READINESS_CHANGE; 

		plms_cbk_call(trk_info,1);
		
		if (0 == trk_info->track_count){
			plms_deact_completed_cbk_call(ent,trk_info);	
		}else{
			plms_peer_async_send(ent,SA_PLM_CHANGE_START,
			SA_PLM_CAUSE_HE_DEACTIVATION);
		}
		break;

	default:
		LOG_ER("Invalid deactivation policy.");
		ret_err = NCSCC_RC_FAILURE;
		break;
	}
	
	TRACE_LEAVE2("Return Val: %d",ret_err);
	return ret_err;
}
/******************************************************************************
@brief		: M4->M1 for three state.
		M4->M5/M6->M1 for 5 state and 5 partial state and M5/M6 is missed.
		1. If I am moving to inactive because of my parent, then nothing to do.
		Only change the presence and readiness state.
		2. If the entity is already OOS, then change only the presence state.
		3. If the entity is insvc, then make sure the entity is moved to OOS
		and the callback is called.

@param[in]	: evt - PLMS_EVT type representing HPI event.

@return		: NCSCC_RC_FAILURE/NCSCC_RC_SUCCESS
******************************************************************************/
static SaUint32T plms_HE_act_to_inact_op( PLMS_EVT *evt)
{
	SaUint32T ret_err = NCSCC_RC_SUCCESS;
	PLMS_CB *cb = plms_cb;
	PLMS_EPATH_TO_ENTITY_MAP_INFO *epath_to_ent;
	PLMS_ENTITY *ent;
	PLMS_TRACK_INFO trk_info;
	PLMS_GROUP_ENTITY *aff_ent_list = NULL;
	PLMS_GROUP_ENTITY *head = NULL;
	PLMS_ENTITY_GROUP_INFO_LIST *log_head_grp;

	TRACE_ENTER2("Entity: %s",evt->req_evt.hpi_evt.entity_path);
	
	epath_to_ent = (PLMS_EPATH_TO_ENTITY_MAP_INFO *)ncs_patricia_tree_get
				(&(cb->epath_to_entity_map_info),
				(SaUint8T *)&(evt->req_evt.hpi_evt.epath_key));
	if (NULL == epath_to_ent ) {
		LOG_ER("Entity corresponding to ent_path %s not found. Possibly\
			the HE is yet to be verified.",
			evt->req_evt.hpi_evt.entity_path);
		TRACE_LEAVE2("Return Val: %d",NCSCC_RC_FAILURE);
		return NCSCC_RC_FAILURE;
	}
	
	ent = epath_to_ent->plms_entity;

	if (NULL == ent){
		LOG_ER("Entity path %s found but epath_to_ent->plms_entity is \
				NULL", evt->req_evt.hpi_evt.entity_path);
		TRACE_LEAVE2("Return Val: %d",NCSCC_RC_FAILURE);
		return NCSCC_RC_FAILURE;
	}
	
	ent->deact_in_pro = FALSE;
	/* Handle clearing management lost flag.*/
	if (plms_rdness_flag_is_set(ent,SA_PLM_RF_MANAGEMENT_LOST)){
		if ( NCSCC_RC_SUCCESS != plms_deact_resp_mngt_flag_clear(ent))
			return NCSCC_RC_FAILURE;
	}
	
	/* If the entity is in any other operation context, then clean up
	before processing this event.*/
	if (ent->adm_op_in_progress){
		ret_err = plms_adm_cnxt_clean_up(ent);
	}
	
	/* If I am moving to M1 because of my parent.
	TODO: Check this carefully.
	*/
	if (plms_is_anc_in_deact_cnxt(ent)){
		TRACE("I am moving to M1 because of my parent.");
		plms_presence_state_set(ent,
					SA_PLM_HE_PRESENCE_INACTIVE,
					NULL,SA_NTF_OBJECT_OPERATION,
					SA_PLM_NTFID_STATE_CHANGE_ROOT);
		
		plms_readiness_state_set(ent,SA_PLM_READINESS_OUT_OF_SERVICE,
					NULL,SA_NTF_OBJECT_OPERATION,
					SA_PLM_NTFID_STATE_CHANGE_ROOT);
		TRACE_LEAVE2("Return Val: %d",NCSCC_RC_SUCCESS);
		return NCSCC_RC_SUCCESS;
	}
	
	plms_presence_state_set(ent,
		       		SA_PLM_HE_PRESENCE_INACTIVE,NULL,
				SA_NTF_OBJECT_OPERATION,
				SA_PLM_NTFID_STATE_CHANGE_ROOT);
	
	/* If the entity is already in OOS, then nothing to do.*/
	if (SA_PLM_READINESS_OUT_OF_SERVICE == 
		ent->entity.he_entity.saPlmHEReadinessState){
		TRACE("HE is already in OOS.");
		TRACE_LEAVE2("Return Val: %d",NCSCC_RC_SUCCESS);
		return NCSCC_RC_SUCCESS;
	}

	plms_readiness_state_set(ent,SA_PLM_READINESS_OUT_OF_SERVICE,
				NULL,SA_NTF_OBJECT_OPERATION,
				SA_PLM_NTFID_STATE_CHANGE_ROOT);

	plms_affected_ent_list_get(ent,&aff_ent_list,0);

	head = aff_ent_list;
	while (head){
		if(!plms_is_chld(ent,head->plm_entity) &&
		PLMS_EE_ENTITY == head->plm_entity->entity_type){
			ret_err = plms_ee_term(head->plm_entity,FALSE,0);
		}
		plms_readiness_state_set(head->plm_entity,
				SA_PLM_READINESS_OUT_OF_SERVICE,
				ent,SA_NTF_OBJECT_OPERATION,
				SA_PLM_NTFID_STATE_CHANGE_DEP);
		plms_readiness_flag_mark_unmark(head->plm_entity,
				SA_PLM_RF_DEPENDENCY,TRUE,ent,
				SA_NTF_OBJECT_OPERATION,
				SA_PLM_NTFID_STATE_CHANGE_DEP);
		head = head->next;
	 }

	plms_aff_ent_exp_rdness_state_ow(aff_ent_list);
	plms_ent_exp_rdness_state_ow(ent);

	memset(&trk_info,0,sizeof(PLMS_TRACK_INFO));
	
	trk_info.group_info_list = NULL;

	/* Add the groups, root entity(ent) belong to.*/
	plms_ent_grp_list_add(ent,
			&(trk_info.group_info_list));
	/* Find out all the groups, all affected entities 
	 * belong to and add the groups to 
	 * trk_info->group_info_list. */ 
	plms_ent_list_grp_list_add(aff_ent_list,
				&(trk_info.group_info_list));	
	
	TRACE("Affected groups for ent %s: ",ent->dn_name_str);
		log_head_grp = trk_info.group_info_list;
	while(log_head_grp){
		TRACE("%llu,",log_head_grp->ent_grp_inf->entity_grp_hdl);
		log_head_grp = log_head_grp->next;
	}

	trk_info.aff_ent_list = aff_ent_list;
	trk_info.root_entity = ent;
	trk_info.change_step = SA_PLM_CHANGE_COMPLETED;
	trk_info.track_cause = SA_PLM_CAUSE_HE_DEACTIVATION;
	trk_info.grp_op = SA_PLM_GROUP_MEMBER_READINESS_CHANGE;	

	plms_cbk_call(&trk_info,1);

	plms_ent_exp_rdness_status_clear(ent);
	plms_aff_ent_exp_rdness_status_clear(aff_ent_list);

	plms_ent_list_free(trk_info.aff_ent_list);
	trk_info.aff_ent_list = NULL;
	plms_ent_grp_list_free(trk_info.group_info_list);
	trk_info.group_info_list = NULL;

	return ret_err;
}
/******************************************************************************
@brief		: M4->M0 : For 2 state
		M4->M1->M0 : For 3 state and M1 is missed.
		M4->M5/M6->M1->M0 : For 5 state and 5 partial state and M5/M6/M1 are 
		missed.
		1. Take the HE to OOS if not in.
		2. Clear the readiness flag.
		3. Wipe out current entity path and current HEType.

@param[in]	: evt - PLMS_EVT type representing HPI event.

@return		: NCSCC_RC_FAILURE/NCSCC_RC_SUCCESS
******************************************************************************/
static SaUint32T plms_HE_act_to_np_op(PLMS_EVT *evt)
{
	SaUint32T ret_err = NCSCC_RC_FAILURE;
	PLMS_CB *cb = plms_cb;
	PLMS_EPATH_TO_ENTITY_MAP_INFO *epath_to_ent;
	PLMS_ENTITY *ent;

	TRACE_ENTER2("Entity: %s",evt->req_evt.hpi_evt.entity_path);
	
	epath_to_ent = (PLMS_EPATH_TO_ENTITY_MAP_INFO *)ncs_patricia_tree_get
				(&(cb->epath_to_entity_map_info),
				(SaUint8T *)&(evt->req_evt.hpi_evt.epath_key));
	if (NULL == epath_to_ent ) {
		LOG_ER("Entity corresponding to ent_path %s not found. Possibly\
			the HE is yet to be verified.",
			evt->req_evt.hpi_evt.entity_path);
		TRACE_LEAVE2("Return Val: %d",NCSCC_RC_FAILURE);
		return NCSCC_RC_FAILURE;
	}
	
	ent = epath_to_ent->plms_entity;

	if (NULL == ent){
		LOG_ER("Entity path %s found but epath_to_ent->plms_entity is \
				NULL", evt->req_evt.hpi_evt.entity_path);
		TRACE_LEAVE2("Return Val: %d",NCSCC_RC_FAILURE);
		return NCSCC_RC_FAILURE;
	}
	
	if (ent->adm_op_in_progress){
		ret_err = plms_adm_cnxt_clean_up(ent);
	}
	
	/* If I am moving to M0 because of my parent.
	*/
	if (plms_is_anc_in_deact_cnxt(ent)){
		TRACE("I am moving to Not-Present because of my parent.");
		plms_presence_state_set(ent,
					SA_PLM_HE_PRESENCE_NOT_PRESENT,
					NULL,SA_NTF_OBJECT_OPERATION,
					SA_PLM_NTFID_STATE_CHANGE_ROOT);
#if 0					
		ent->entity.he_entity.saPlmHEReadinessState = 
					SA_PLM_READINESS_OUT_OF_SERVICE;
#endif					
		/* As the HE is not-present, clean up PLMS database.
		1. Remove the entry from e-path to ent map tree.
		2. Wipe out saPlmHECurrHEType and saPlmHECurrEntityPath.*/
		plms_he_np_clean_up(ent,&epath_to_ent);
		
		TRACE_LEAVE2("Return Val: %d",NCSCC_RC_SUCCESS);
		return NCSCC_RC_SUCCESS;
	}

	/* If the entity is already in OOS.*/
	if (SA_PLM_READINESS_OUT_OF_SERVICE == 
		ent->entity.he_entity.saPlmHEReadinessState){
		TRACE("HE is already in OOS.");
		ret_err = plms_he_oos_to_np_process(ent);
	}else{
		ret_err = plms_he_insvc_to_np_process(ent);
	}
	
	TRACE_LEAVE2("Return Val: %d",ret_err);
	return ret_err;
}
/******************************************************************************
@brief		: M4->M5/M6->M1->M2/M3. Valid only for 5 state and 5 state partial 
		model and M5/M6/M1 are missed.
		 
        	In M4, this HE might be in InSvc. If it is and PLMS misses 
		M5/M6/M1 then, PLMS has to move the affected entities to OOS before
		activating this HE. 

@param[in]	: evt - PLMS_EVT type representing HPI event.

@return		: NCSCC_RC_FAILURE/NCSCC_RC_SUCCESS
******************************************************************************/
static SaUint32T plms_HE_act_to_inspending_op(PLMS_EVT *evt)
{
	SaUint32T ret_err = NCSCC_RC_FAILURE;
	PLMS_CB *cb = plms_cb;
	PLMS_EPATH_TO_ENTITY_MAP_INFO *epath_to_ent;
	PLMS_ENTITY *ent;

	TRACE_ENTER2("Entity: %s",evt->req_evt.hpi_evt.entity_path);
	
	epath_to_ent = (PLMS_EPATH_TO_ENTITY_MAP_INFO *)ncs_patricia_tree_get
				(&(cb->epath_to_entity_map_info),
				(SaUint8T *)&(evt->req_evt.hpi_evt.epath_key));
	if (NULL == epath_to_ent ) {
		LOG_ER("Entity corresponding to ent_path %s not found. Possibly\
			the HE is yet to be verified.",
			evt->req_evt.hpi_evt.entity_path);
		TRACE_LEAVE2("Return Val: %d",NCSCC_RC_FAILURE);
		return NCSCC_RC_FAILURE;
	}
	
	ent = epath_to_ent->plms_entity;

	if (NULL == ent){
		LOG_ER("Entity path %s found but epath_to_ent->plms_entity is \
				NULL", evt->req_evt.hpi_evt.entity_path);
		TRACE_LEAVE2("Return Val: %d",NCSCC_RC_FAILURE);
		return NCSCC_RC_FAILURE;
	}
	/* Mark the presence state of the HE to activating.*/
	plms_presence_state_set(ent,SA_PLM_HE_PRESENCE_ACTIVATING,NULL,
					SA_NTF_OBJECT_OPERATION,
					SA_PLM_NTFID_STATE_CHANGE_ROOT);
	/* TODO: Is it the right place to call mngt clear func?
	Managemnet lost flag handling.*/
	if(plms_rdness_flag_is_set(ent,SA_PLM_RF_MANAGEMENT_LOST)){

		if ( NCSCC_RC_SUCCESS != plms_inspending_mngt_flag_clear(ent))
			return ret_err;
	}

	if (ent->adm_op_in_progress){
		ret_err = plms_adm_cnxt_clean_up(ent);
	}
	
	/* If the entity is already in OOS, then nothing to do.*/
	if (SA_PLM_READINESS_OUT_OF_SERVICE == 
		ent->entity.he_entity.saPlmHEReadinessState){
		TRACE("HE is already in OOS.");
		if (SA_PLM_HE_ADMIN_LOCKED_INACTIVE  ==
		                ent->entity.he_entity.saPlmHEAdminState){
			
			ret_err = plms_he_deactivate(ent,FALSE,TRUE);
			LOG_ER("Deactivate the entity, as the admin state is\
			lck inactive. Ent: %s",ent->dn_name_str);

			TRACE_LEAVE2("Return Val: %d",ret_err);
			return ret_err;
		}
		/* Activate the board only if the the board supports full
		5 state model.*/
		if (PLMS_HPI_FULL_FIVE_HOTSWAP_MODEL == ent->state_model){
			/* Make the HE to move to active state.*/
			ret_err = plms_he_activate(ent,FALSE,TRUE);
			if (NCSCC_RC_SUCCESS != ret_err){
				LOG_ER("HE %s Activation FAILED.",
				ent->dn_name_str);
			}else{
				TRACE("HE %s Activation SUCCESSFUL",
				ent->dn_name_str);
			}
		}

		TRACE_LEAVE2("Return Val: %d",ret_err);
		return ret_err;
	}
	
	ret_err = plms_he_insvc_to_acting_process(ent);

	TRACE_LEAVE2("Return Val: %d",ret_err);
	return ret_err;
}

/******************************************************************************
@brief		: M4->M4. PLMS gets this for those HE which are in M4 state on hpi
		rediscovery. PLMS uses this information to clear the management lost 
		flag, if set for the same HE.

@param[in]	: evt - PLMS_EVT type representing HPI event.

@return		: NCSCC_RC_FAILURE/NCSCC_RC_SUCCESS
******************************************************************************/
static SaUint32T plms_HE_act_to_act_op(PLMS_EVT *evt)
{
	PLMS_CB *cb = plms_cb;
	PLMS_EPATH_TO_ENTITY_MAP_INFO *epath_to_ent;
	PLMS_ENTITY *ent;

	TRACE_ENTER2("Entity: %s",evt->req_evt.hpi_evt.entity_path);
	
	epath_to_ent = (PLMS_EPATH_TO_ENTITY_MAP_INFO *)ncs_patricia_tree_get
				(&(cb->epath_to_entity_map_info),
				(SaUint8T *)&(evt->req_evt.hpi_evt.epath_key));
	if (NULL == epath_to_ent ) {
		LOG_ER("Entity corresponding to ent_path %s not found. Possibly\
			the HE is yet to be verified.",
			evt->req_evt.hpi_evt.entity_path);
		TRACE_LEAVE2("Return Val: %d",NCSCC_RC_FAILURE);
		return NCSCC_RC_FAILURE;
	}
	
	ent = epath_to_ent->plms_entity;

	if (NULL == ent){
		LOG_ER("Entity path %s found but epath_to_ent->plms_entity is \
				NULL", evt->req_evt.hpi_evt.entity_path);
		TRACE_LEAVE2("Return Val: %d",NCSCC_RC_FAILURE);
		return NCSCC_RC_FAILURE;
	}
	ent->act_in_pro = FALSE;

	/* Handle clearing management lost flag.*/
	if (plms_rdness_flag_is_set(ent,SA_PLM_RF_MANAGEMENT_LOST)){
		if ( NCSCC_RC_SUCCESS != plms_act_resp_mngt_flag_clear(ent))
			TRACE_LEAVE2("Return Val: %d",NCSCC_RC_FAILURE);
			return NCSCC_RC_FAILURE;
	}
	
	TRACE_LEAVE2("Return Val: %d",NCSCC_RC_SUCCESS);
	return NCSCC_RC_SUCCESS;
}
/******************************************************************************
@brief		: M5->M1 : For 5 state and 5 partial state model. 

@param[in]	: evt - PLMS_EVT type representing HPI event.

@return		: NCSCC_RC_FAILURE/NCSCC_RC_SUCCESS
******************************************************************************/
static SaUint32T plms_HE_deacting_to_inact_op(PLMS_EVT *evt)
{
	SaUint32T ret_err = NCSCC_RC_SUCCESS;
	PLMS_CB *cb = plms_cb;
	PLMS_EPATH_TO_ENTITY_MAP_INFO *epath_to_ent;
	PLMS_ENTITY *ent;
	PLMS_TRACK_INFO *trk_info;
	PLMS_GROUP_ENTITY *head = NULL;
	PLMS_GROUP_ENTITY *act_aff_ent_list=NULL,*aff_ent_list=NULL;
	PLMS_ENTITY_GROUP_INFO_LIST *log_head_grp;

	TRACE_ENTER2("Entity: %s",evt->req_evt.hpi_evt.entity_path);
	
	epath_to_ent = (PLMS_EPATH_TO_ENTITY_MAP_INFO *)ncs_patricia_tree_get
				(&(cb->epath_to_entity_map_info),
				(SaUint8T *)&(evt->req_evt.hpi_evt.epath_key));
	if (NULL == epath_to_ent ) {
		LOG_ER("Entity corresponding to ent_path %s not found. Possibly\
			the HE is yet to be verified.",
			evt->req_evt.hpi_evt.entity_path);
		TRACE_LEAVE2("Return Val: %d",NCSCC_RC_FAILURE);
		return NCSCC_RC_FAILURE;
	}
	
	ent = epath_to_ent->plms_entity;

	if (NULL == ent){
		LOG_ER("Entity path %s found but epath_to_ent->plms_entity is \
				NULL", evt->req_evt.hpi_evt.entity_path);
		TRACE_LEAVE2("Return Val: %d",NCSCC_RC_FAILURE);
		return NCSCC_RC_FAILURE;
	}
	
	ent->deact_in_pro = FALSE;

	/* Handle clearing management lost flag.*/
	if (plms_rdness_flag_is_set(ent,SA_PLM_RF_MANAGEMENT_LOST)){
		if ( NCSCC_RC_SUCCESS != plms_deact_resp_mngt_flag_clear(ent))
			return NCSCC_RC_FAILURE;
	}
	
	/* If I am moving to M1 because any of my parent, then just 
	change the presence state.
	TODO: Check this carefully. If my parent is being moved to M1
	gracefully/surprise, would I ever get M5 for the children??
	*/
	if (plms_is_anc_in_deact_cnxt(ent)){
		TRACE("The HE is moved to inactive because of its parent.");
		plms_presence_state_set(ent,
				SA_PLM_HE_PRESENCE_INACTIVE,
				NULL,SA_NTF_OBJECT_OPERATION,
				SA_PLM_NTFID_STATE_CHANGE_ROOT);

		TRACE_LEAVE2("Return Val: %d",NCSCC_RC_SUCCESS);
		return NCSCC_RC_SUCCESS;
	}
	
	switch(ent->state_model){
		
		case PLMS_HPI_PARTIAL_FIVE_HOTSWAP_MODEL:
			TRACE("The HE supports partial 5-state model.");
			plms_presence_state_set(ent,
			       		SA_PLM_HE_PRESENCE_INACTIVE,
					NULL,SA_NTF_OBJECT_OPERATION,
					SA_PLM_NTFID_STATE_CHANGE_ROOT);
			
			/* If the ent is in admin context, then clean up.*/
			if(ent->adm_op_in_progress){
				ret_err = plms_adm_cnxt_clean_up(ent); 
			}
			if (SA_PLM_READINESS_OUT_OF_SERVICE == 
				ent->entity.he_entity.saPlmHEReadinessState){
				TRACE("HE is already in OOS.");
				TRACE_LEAVE2("Return Val: %d",NCSCC_RC_SUCCESS);
				return NCSCC_RC_SUCCESS;
			}
			
			plms_readiness_state_set(ent,
					SA_PLM_READINESS_OUT_OF_SERVICE,
					NULL,SA_NTF_OBJECT_OPERATION,
					SA_PLM_NTFID_STATE_CHANGE_ROOT);

			 plms_affected_ent_list_get(ent,&aff_ent_list,0);
			 
			 head = aff_ent_list;
			 while (head){
				if(!plms_is_chld(ent,head->plm_entity) &&
				PLMS_EE_ENTITY == 
				head->plm_entity->entity_type){
					ret_err = plms_ee_term(head->plm_entity,
								FALSE,0);
					if (NCSCC_RC_SUCCESS != ret_err){
						LOG_ER("EE %s term FAILED.",
							ent->dn_name_str);
					}else{
						TRACE("EE %s term SUCCESSFUL.",
							ent->dn_name_str);
					}
				}
				plms_readiness_state_set(head->plm_entity,
					SA_PLM_READINESS_OUT_OF_SERVICE,
					ent,SA_NTF_OBJECT_OPERATION,
					SA_PLM_NTFID_STATE_CHANGE_DEP);

				plms_readiness_flag_mark_unmark(
						head->plm_entity,
						SA_PLM_RF_DEPENDENCY,TRUE,
						ent,SA_NTF_OBJECT_OPERATION,
						SA_PLM_NTFID_STATE_CHANGE_DEP);
				head = head->next;
			 }
			plms_aff_ent_exp_rdness_state_ow(aff_ent_list);
			plms_ent_exp_rdness_state_ow(ent);

			trk_info = (PLMS_TRACK_INFO *)calloc(1,
						sizeof(PLMS_TRACK_INFO));
			if(NULL == trk_info){
				LOG_CR("deacting_to_inact,calloc FAILED.\
				error: %s",strerror(errno));
				assert(0);
			}

			/* Add the groups, root entity(ent) belong to.*/
			plms_ent_grp_list_add(ent,
					&(trk_info->group_info_list));
	
			/* Find out all the groups, all affected entities 
			 * belong to and add the groups to 
			 * trk_info->group_info_list. */ 
			plms_ent_list_grp_list_add(aff_ent_list,
						&(trk_info->group_info_list));	

			TRACE("Affected groups for ent %s: ",ent->dn_name_str);
			log_head_grp = trk_info->group_info_list;
			while(log_head_grp){
				TRACE("%llu,",log_head_grp->ent_grp_inf->
				entity_grp_hdl);
				log_head_grp = log_head_grp->next;
			}

			trk_info->aff_ent_list = aff_ent_list;
			trk_info->root_entity = ent;
			trk_info->change_step = SA_PLM_CHANGE_COMPLETED;
			trk_info->track_cause = SA_PLM_CAUSE_HE_DEACTIVATION;
			trk_info->grp_op = SA_PLM_GROUP_MEMBER_READINESS_CHANGE;

			plms_cbk_call(trk_info,1);

			plms_ent_exp_rdness_status_clear(ent);
			plms_aff_ent_exp_rdness_status_clear(aff_ent_list);

			plms_trk_info_free(trk_info);

			break;
		/* M5->M1. This is possible for below mentioned three cases.
		 * 1. Admin deactivate: Nothing to do. Only state change.
		 * 2. Latch open(Proper HE Deactvate): Move the dep/chld HEs
		 * to OOS and set dependency flag. Call the completed callback.
		 * 3. Child of case-2: Nothing to do. Only state change.
		 * */
		case PLMS_HPI_FULL_FIVE_HOTSWAP_MODEL:
			TRACE("The HE supports 5-state model.");
			plms_presence_state_set(ent,
			       		SA_PLM_HE_PRESENCE_INACTIVE,
					NULL,SA_NTF_OBJECT_OPERATION,
					SA_PLM_NTFID_STATE_CHANGE_ROOT);

			if (SA_PLM_READINESS_OUT_OF_SERVICE == 
				ent->entity.he_entity.saPlmHEReadinessState){
				TRACE("The HE is already in OOS.");
				TRACE_LEAVE2("Return Val: %d",NCSCC_RC_SUCCESS);
				return NCSCC_RC_SUCCESS;
			}
			plms_readiness_state_set(ent,
					SA_PLM_READINESS_OUT_OF_SERVICE,
					NULL,SA_NTF_OBJECT_OPERATION,
					SA_PLM_NTFID_STATE_CHANGE_ROOT);
			
			/* TODO: I have come this far means, trk_info is valid.
			Are you seeing any crash.. ohh... handle is here.*/
			/* handling is needed when trk_info is freed when deactivation 
			fails and still HPI event extraction_pending is received */
			if ( NULL == ent->trk_info){
				TRACE_LEAVE2("Return Val: %d",NCSCC_RC_SUCCESS);
				return NCSCC_RC_SUCCESS;
			}

			trk_info = ent->trk_info;
			/* TODO: Should I clear the inv_info from the group,
			as there is a possibility of getting M1 for the ent
			even before getting the response from the application.
			*/
			log_head_grp = ent->trk_info->group_info_list;
			while (log_head_grp){
				if (log_head_grp->ent_grp_inf){
					plms_inv_to_cbk_in_grp_trk_rmv(
					log_head_grp->ent_grp_inf, ent->trk_info);
				}
				log_head_grp=log_head_grp->next;
			}

			trk_info->track_count = 0;
			
			/* Make child to OOS and mark the dependecy flag.*/
			head = trk_info->aff_ent_list;
			while(head){
				if (!plms_is_rdness_state_set(head->plm_entity,
				SA_PLM_READINESS_OUT_OF_SERVICE)){  
						
					plms_readiness_state_set(
					head->plm_entity,		
					SA_PLM_READINESS_OUT_OF_SERVICE,
					ent,SA_NTF_OBJECT_OPERATION,
					SA_PLM_NTFID_STATE_CHANGE_DEP);
						
					plms_ent_to_ent_list_add(
					head->plm_entity,
					&act_aff_ent_list);
				}
				
				if( !plms_rdness_flag_is_set(
				head->plm_entity,SA_PLM_RF_DEPENDENCY)){
				
					plms_readiness_flag_mark_unmark(
					head->plm_entity,
					SA_PLM_RF_DEPENDENCY,
					TRUE,ent,
					SA_NTF_OBJECT_OPERATION,
					SA_PLM_NTFID_STATE_CHANGE_DEP);

					plms_ent_to_ent_list_add(
					head->plm_entity,
					&act_aff_ent_list);
				}	
					
				head = head->next;
			}
			
			/* Add the groups, root entity(ent) belong to.*/
			plms_ent_grp_list_add(ent,
					&(trk_info->group_info_list));
	
			/* Find out all the groups, all affected entities 
			 * belong to and add the groups to 
			 * trk_info->group_info_list. */ 
			plms_ent_list_grp_list_add(act_aff_ent_list,
						&(trk_info->group_info_list));	
			
			TRACE("Affected groups for ent %s: ",ent->dn_name_str);
			log_head_grp = trk_info->group_info_list;
			while(log_head_grp){
				TRACE("%llu,",log_head_grp->ent_grp_inf->
				entity_grp_hdl);
				log_head_grp = log_head_grp->next;
			}

			ent->adm_op_in_progress = FALSE;
			ent->am_i_aff_ent = FALSE;
			plms_aff_ent_flag_mark_unmark(
						trk_info->aff_ent_list, FALSE);
			plms_aff_ent_exp_rdness_status_clear(
						trk_info->aff_ent_list);
			plms_ent_list_free(trk_info->aff_ent_list);

			
			trk_info->aff_ent_list = act_aff_ent_list;
			plms_aff_ent_exp_rdness_state_ow(act_aff_ent_list);
			plms_ent_exp_rdness_state_ow(ent);
			
			trk_info->change_step = SA_PLM_CHANGE_COMPLETED;
			plms_cbk_call(trk_info,1);
			
			plms_peer_async_send(ent,SA_PLM_CHANGE_COMPLETED,
			SA_PLM_CAUSE_HE_DEACTIVATION);

			plms_aff_ent_exp_rdness_status_clear(act_aff_ent_list);
			plms_ent_exp_rdness_status_clear(ent);

			plms_trk_info_free(trk_info);

			break;
		default:
			LOG_ER("Invalid state model: %d",ent->state_model);
			break;	
	}

	TRACE_LEAVE2("Return Val: %d",ret_err);
	return ret_err;
}
/******************************************************************************
@brief		: M5/M6->M1->M2/M3 valid for 5 state and 5 partial state model. 
                  
        	In M5/M6 means, the HE still can be in InSvc. Hence it is 
		same as M4->M2/M3. If the ent is in deactivation context, then
		do the clean up job.  

@param[in]	: evt - PLMS_EVT type representing HPI event.

@return		: NCSCC_RC_FAILURE/NCSCC_RC_SUCCESS
******************************************************************************/
static SaUint32T plms_HE_deacting_to_inspending_op( PLMS_EVT *evt)
{
	SaUint32T ret_err = NCSCC_RC_SUCCESS;
	PLMS_CB *cb = plms_cb;
	PLMS_EPATH_TO_ENTITY_MAP_INFO *epath_to_ent;
	PLMS_ENTITY *ent;
	PLMS_ENTITY_GROUP_INFO_LIST *head;
	
	TRACE_ENTER2("Entity: %s",evt->req_evt.hpi_evt.entity_path);
	
	epath_to_ent = (PLMS_EPATH_TO_ENTITY_MAP_INFO *)ncs_patricia_tree_get
				(&(cb->epath_to_entity_map_info),
				(SaUint8T *)&(evt->req_evt.hpi_evt.epath_key));
	if (NULL == epath_to_ent ) {
		LOG_ER("Entity corresponding to ent_path %s not found. Possibly\
			the HE is yet to be verified.",
			evt->req_evt.hpi_evt.entity_path);
		TRACE_LEAVE2("Return Val: %d",NCSCC_RC_FAILURE);
		return NCSCC_RC_FAILURE;
	}
	
	ent = epath_to_ent->plms_entity;

	if (NULL == ent){
		LOG_ER("Entity path %s found but epath_to_ent->plms_entity is \
				NULL", evt->req_evt.hpi_evt.entity_path);
		TRACE_LEAVE2("Return Val: %d",NCSCC_RC_FAILURE);
		return NCSCC_RC_FAILURE;
	}

	ent->deact_in_pro = FALSE;
	/* Mark the presence state of the HE to activating.*/
	plms_presence_state_set(ent,SA_PLM_HE_PRESENCE_ACTIVATING,NULL,
					SA_NTF_OBJECT_OPERATION,
					SA_PLM_NTFID_STATE_CHANGE_ROOT);
	
	/* TODO: Is it the right place to call mngt clear func?
	Managemnet lost flag handling.*/
	if(plms_rdness_flag_is_set(ent,SA_PLM_RF_MANAGEMENT_LOST)){
		if ( NCSCC_RC_SUCCESS != plms_inspending_mngt_flag_clear(ent))
			return ret_err;
	}
	
	/* If I am moving to M2 because any of my parent, then just 
	change the presence state.
	*/
	if (plms_is_anc_in_deact_cnxt(ent)){
		TRACE("The HE %s is moving to activating because of his Parent."
							,ent->dn_name_str);
		plms_readiness_state_set(ent,SA_PLM_READINESS_OUT_OF_SERVICE,
					NULL,SA_NTF_OBJECT_OPERATION,
					SA_PLM_NTFID_STATE_CHANGE_ROOT);

		if (SA_PLM_HE_ADMIN_LOCKED_INACTIVE == 
			ent->entity.he_entity.saPlmHEAdminState){

			ret_err = plms_he_deactivate(ent,0,1);
			LOG_ER("Entity is deactivated as the admin state is \
			lckinact. Ent: %s",ent->dn_name_str);

			TRACE_LEAVE2("Return Val: %d",ret_err);
			return ret_err;
		}
		/* Set M4 state.*/
		if (PLMS_HPI_FULL_FIVE_HOTSWAP_MODEL == ent->state_model){
			ret_err = plms_he_activate(ent,FALSE,TRUE);
			if (NCSCC_RC_SUCCESS != ret_err){
				LOG_ER("HE %s activation FAILED.", 
				ent->dn_name_str);
			}else{
				TRACE("HE %s activation SUCCESSFUL.",
				ent->dn_name_str);
			}
		}
#if 0		
		ret_err = plms_he_insvc_to_acting_process(ent);
#endif		
		TRACE_LEAVE2("Return Val: %d",ret_err);
		return ret_err;
	}
	
	if (PLMS_HPI_PARTIAL_FIVE_HOTSWAP_MODEL == ent->state_model){
		TRACE("The HE %s supports partial 5-state model.",
							ent->dn_name_str);
		/* If the ent is in admin context then clean up.*/
		if (ent->adm_op_in_progress)
			plms_adm_cnxt_clean_up(ent);
			
		/* If I am in OOS, then nothing to do.*/
		if ( SA_PLM_READINESS_OUT_OF_SERVICE == 
			ent->entity.he_entity.saPlmHEReadinessState){
			TRACE("The HE %s is already in OOS.",
						ent->dn_name_str);

			if (SA_PLM_HE_ADMIN_LOCKED_INACTIVE == 
			ent->entity.he_entity.saPlmHEAdminState){

				ret_err = plms_he_deactivate(ent,0,1);
				LOG_ER("Entity is deactivated as the \
				admin state is lckinact. Ent: %s",
				ent->dn_name_str);

				TRACE_LEAVE2("Return Val: %d",ret_err);
				return ret_err;
			}
			
			TRACE_LEAVE2("Return Val: %d",ret_err);
			return ret_err;
		}else{
			ret_err = plms_he_insvc_to_acting_process(ent);
		}

	}else if (PLMS_HPI_FULL_FIVE_HOTSWAP_MODEL == ent->state_model){
		
		TRACE("The HE %s supports 5-state model.", ent->dn_name_str);
		/* HE is already in OOS and hence only change the presence
 		state and make the HE move to active state.*/
		if (SA_PLM_READINESS_OUT_OF_SERVICE == 
		ent->entity.he_entity.saPlmHEReadinessState){
			TRACE("The HE %s is already in OOS.", ent->dn_name_str);
			if (SA_PLM_HE_ADMIN_LOCKED_INACTIVE == 
			ent->entity.he_entity.saPlmHEAdminState){

				ret_err = plms_he_deactivate(ent,0,1);
				LOG_ER("Entity is deactivated as the admin \
				state is lckinact. Ent: %s",ent->dn_name_str);

				TRACE_LEAVE2("Return Val: %d",ret_err);
				return ret_err;
			}
			
			ret_err = plms_he_activate(ent,FALSE,TRUE);  	
			if (NCSCC_RC_SUCCESS != ret_err){
				LOG_ER("HE %s activation FAILED.",
							ent->dn_name_str);
			}else{
				TRACE("HE %s activation SUCCESSFUL.",
							ent->dn_name_str);
			}
			TRACE_LEAVE2("Return Val: %d",ret_err);
			return ret_err;
		}

		/* trk_info is null, prev he_deactivate failed and clean up was already done?
		* In anycase since the trk_info is freed there is no list to track against
		*/
		if ( NULL == ent->trk_info){
			TRACE_LEAVE2("Return Val: %d",NCSCC_RC_SUCCESS);
			return NCSCC_RC_SUCCESS;
		}

		switch(ent->trk_info->change_step){
		
		case SA_PLM_CHANGE_VALIDATE:
		case SA_PLM_CHANGE_START:
			TRACE("Clean up the previuos admin context.\
			Ent: %s, Change step: %d, adm_op: %d",ent->dn_name_str,\
			ent->trk_info->change_step,ent->adm_op_in_progress);
			/* Clean up the inv_to_cbk nodes belongs to 
 			this trk_info.*/
			head = ent->trk_info->group_info_list;
			while (head){
				plms_inv_to_cbk_in_grp_trk_rmv(
					head->ent_grp_inf, ent->trk_info);
				head=head->next;
			}
			
		case SA_PLM_CHANGE_COMPLETED:
			/* Clear the admin context.*/
			ent->adm_op_in_progress = FALSE;
			ent->am_i_aff_ent = FALSE;
			plms_aff_ent_flag_mark_unmark(
					ent->trk_info->aff_ent_list,FALSE);
			
			plms_peer_async_send(ent,SA_PLM_CHANGE_COMPLETED,
			ent->trk_info->track_cause);
			
			/* Free the track info.*/
			plms_trk_info_free(ent->trk_info);

			/* All clean up done for the previous deactivation
 			process. Process M2 now.*/
			ret_err = plms_he_insvc_to_acting_process(ent);
			break;
		default:
			TRACE("Invalid change step: %d",
						ent->trk_info->change_step);
			break;

		}		
	}else{
		LOG_ER("Invalid state model for this transition, M5->M2.\
		Ent: %s, State model: %d",ent->dn_name_str,ent->state_model);
		return NCSCC_RC_FAILURE; 
	}	
	
	TRACE_LEAVE2("Return Val: %d",ret_err);
	return ret_err;
}
/******************************************************************************
@brief		: M5->M4 valid transtion for 5 state model.
        	Use case:
		1. Deactivation policy is SA_PLM_DP_REJECT_NOT_OOS but the HE is
		not in OOS.
		2. Any of the application rejects the deactivation.
		
		In both the cases, as the HE is not yet moved to OOS, nothing to
		do. Just change the state. 

@param[in]	: evt - PLMS_EVT type representing HPI event.

@return		: NCSCC_RC_FAILURE/NCSCC_RC_SUCCESS
******************************************************************************/
static SaUint32T plms_HE_deacting_to_act_op( PLMS_EVT *evt)
{
	SaUint32T ret_err = NCSCC_RC_FAILURE;
	PLMS_CB *cb = plms_cb;
	PLMS_EPATH_TO_ENTITY_MAP_INFO *epath_to_ent;
	PLMS_ENTITY *ent;

	TRACE_ENTER2("Entity: %s",evt->req_evt.hpi_evt.entity_path);
	
	epath_to_ent = (PLMS_EPATH_TO_ENTITY_MAP_INFO *)ncs_patricia_tree_get
				(&(cb->epath_to_entity_map_info),
				(SaUint8T *)&(evt->req_evt.hpi_evt.epath_key));
	if (NULL == epath_to_ent ) {
		LOG_ER("Entity corresponding to ent_path %s not found. Possibly\
			the HE is yet to be verified.",
			evt->req_evt.hpi_evt.entity_path);
		TRACE_LEAVE2("Return Val: %d",NCSCC_RC_FAILURE);
		return NCSCC_RC_FAILURE;
	}
	
	ent = epath_to_ent->plms_entity;

	if (NULL == ent){
		LOG_ER("Entity path %s found but epath_to_ent->plms_entity is \
				NULL", evt->req_evt.hpi_evt.entity_path);
		TRACE_LEAVE2("Return Val: %d",NCSCC_RC_FAILURE);
		return NCSCC_RC_FAILURE;
	}

	/* TODO: should I check if the entity is in deact context and
	clean up? Actly not required as when an application rejects
	deactivation, clean up is done there.*/
	
	ent->act_in_pro = FALSE;
	ent->deact_in_pro = FALSE;

	plms_presence_state_set(ent,SA_PLM_HE_PRESENCE_ACTIVE,
				NULL,SA_NTF_OBJECT_OPERATION,
				SA_PLM_NTFID_STATE_CHANGE_ROOT);
	
	if (SA_PLM_HE_ADMIN_LOCKED_INACTIVE == 
			ent->entity.he_entity.saPlmHEAdminState){

		ret_err = plms_he_deactivate(ent,0,1);
		LOG_ER("Entity is deactivated as the admin state is lckinact. \
		Ent: %s",ent->dn_name_str);
		TRACE_LEAVE2("Return Val: %d",ret_err);
		return ret_err;
	}	
	
	/* Managemnet lost flag handling.*/
	if(plms_rdness_flag_is_set(ent,SA_PLM_RF_MANAGEMENT_LOST)){

		if ( NCSCC_RC_SUCCESS != plms_act_resp_mngt_flag_clear(ent))
			return NCSCC_RC_FAILURE;
	}


	TRACE_LEAVE2("Return Val: %d",NCSCC_RC_SUCCESS);
	return NCSCC_RC_SUCCESS;

}
/******************************************************************************
@brief		: M5/M6->M1->M0 valid for 5 state and 5 partial state model and
		M1 is missed.
        	For 5 state model,if deactivation is in progress and hence
		clean up the callbacks related to deactivation and process
		M0 event. 

@param[in]	: evt - PLMS_EVT type representing HPI event.

@return		: NCSCC_RC_FAILURE/NCSCC_RC_SUCCESS
******************************************************************************/
static SaUint32T plms_HE_deacting_to_np_op(PLMS_EVT *evt)
{
	SaUint32T ret_err = NCSCC_RC_FAILURE;
	PLMS_CB *cb = plms_cb;
	PLMS_EPATH_TO_ENTITY_MAP_INFO *epath_to_ent;
	PLMS_ENTITY *ent;
	PLMS_ENTITY_GROUP_INFO_LIST *head = NULL;
	
	TRACE_ENTER2("Entity: %s",evt->req_evt.hpi_evt.entity_path);
	
	epath_to_ent = (PLMS_EPATH_TO_ENTITY_MAP_INFO *)ncs_patricia_tree_get
				(&(cb->epath_to_entity_map_info),
				(SaUint8T *)&(evt->req_evt.hpi_evt.epath_key));
	if (NULL == epath_to_ent ) {
		LOG_ER("Entity corresponding to ent_path %s not found. Possibly\
			the HE is yet to be verified.",
			evt->req_evt.hpi_evt.entity_path);
		TRACE_LEAVE2("Return Val: %d",NCSCC_RC_FAILURE);
		return NCSCC_RC_FAILURE;
	}
	
	ent = epath_to_ent->plms_entity;

	if (NULL == ent){
		LOG_ER("Entity path %s found but epath_to_ent->plms_entity is \
				NULL", evt->req_evt.hpi_evt.entity_path);
		TRACE_LEAVE2("Return Val: %d",NCSCC_RC_FAILURE);
		return NCSCC_RC_FAILURE;
	}
	
	/* If I am moving to M0 because of my parent.
	TODO: Check this carefully.
	*/
	if (plms_is_anc_in_deact_cnxt(ent)){
		TRACE("The HE %s is moving to not-present because \
		of its parent.",ent->dn_name_str);
		plms_presence_state_set(ent,
					SA_PLM_HE_PRESENCE_NOT_PRESENT,
					NULL,SA_NTF_OBJECT_OPERATION,
					SA_PLM_NTFID_STATE_CHANGE_ROOT);
#if 0					
		ent->entity.he_entity.saPlmHEReadinessState = 
					SA_PLM_READINESS_OUT_OF_SERVICE;
#endif					
		/* As the HE is not-present, clean up PLMS database.
		1. Remove the entry from e-path to ent map tree.
		2. Wipe out saPlmHECurrHEType and saPlmHECurrEntityPath.*/
		plms_he_np_clean_up(ent,&epath_to_ent);
		
		return NCSCC_RC_SUCCESS;
	}
	
	if (PLMS_HPI_PARTIAL_FIVE_HOTSWAP_MODEL == ent->state_model){
		TRACE("The HE %s supports partial 5-state model.",
							ent->dn_name_str);
		/* If the ent is in admin context, then clean up.*/
		if ( ent->adm_op_in_progress){
			TRACE("The HE is already in admin context. Clean up.\
			Ent: %s, adm_op: %d",ent->dn_name_str,
			ent->adm_op_in_progress);
			ret_err = plms_adm_cnxt_clean_up(ent);
		}	
		if ( SA_PLM_READINESS_OUT_OF_SERVICE == 
		ent->entity.he_entity.saPlmHEReadinessState){
			TRACE("The HE %s is already in OOS",ent->dn_name_str);
			ret_err = plms_he_oos_to_np_process(ent);
		}else{
			ret_err = plms_he_insvc_to_np_process(ent);
		}
	}else if (PLMS_HPI_FULL_FIVE_HOTSWAP_MODEL == ent->state_model){
		TRACE("The HE %s supports 5-state model.", ent->dn_name_str);
		/* If I am in OOS, then nothing much to do.*/	
		if (SA_PLM_READINESS_OUT_OF_SERVICE == 
			ent->entity.he_entity.saPlmHEReadinessState){
			TRACE("The HE %s is already in OOS", ent->dn_name_str);
			ret_err = plms_he_oos_to_np_process(ent);
			return ret_err;
		}

		switch(ent->trk_info->change_step){
		
		case SA_PLM_CHANGE_VALIDATE:
		case SA_PLM_CHANGE_START:
			TRACE("Clean up the previuos admin context.\
			Ent: %s, adm_op_id: %llu",ent->dn_name_str,\
			ent->trk_info->imm_adm_opr_id);
			/* Clean up the inv_to_cbk nodes belongs to 
 			this trk_info.*/
			head = ent->trk_info->group_info_list;
			while (head){
				plms_inv_to_cbk_in_grp_trk_rmv(
					head->ent_grp_inf, ent->trk_info);
				head=head->next;
			}

		case SA_PLM_CHANGE_COMPLETED:
			/* Clear the admin context.*/
			ent->adm_op_in_progress = FALSE;
			ent->am_i_aff_ent = FALSE;
			plms_aff_ent_flag_mark_unmark(
					ent->trk_info->aff_ent_list,FALSE);
			
			plms_peer_async_send(ent,SA_PLM_CHANGE_COMPLETED,
			ent->trk_info->track_cause);
			
			/* Free the track info.*/
			plms_trk_info_free(ent->trk_info);

			/* All clean up done for the previous deactivation
 			process. Process M0 now.*/
			ret_err = plms_he_insvc_to_np_process(ent);
			break;
		default:
			ret_err = NCSCC_RC_FAILURE;
			LOG_ER("Invalid change step.");
			break;

		}		
	}else{
		LOG_ER("Invalid state model for this operation, M5->M0.\
		Ent: %s, st_model: %d",ent->dn_name_str,ent->state_model);
		TRACE_LEAVE2("Return Val: %d",NCSCC_RC_FAILURE);
		return NCSCC_RC_FAILURE; 
	}	
	
	TRACE_LEAVE2("Return Val: %d",ret_err);
	return ret_err;
}

/******************************************************************************
@brief		: Invalid State transition.

@param[in]	: evt - PLMS_EVT type representing HPI event.

@return		: NCSCC_RC_FAILURE/NCSCC_RC_SUCCESS
******************************************************************************/
static SaUint32T plms_HE_pres_state_invalid(PLMS_EVT *evt)
{
	PLMS_CB *cb = plms_cb;
	PLMS_EPATH_TO_ENTITY_MAP_INFO *epath_to_ent;
	PLMS_ENTITY *ent;

	TRACE_ENTER2("Entity: %s",evt->req_evt.hpi_evt.entity_path);
	
	epath_to_ent = (PLMS_EPATH_TO_ENTITY_MAP_INFO *)ncs_patricia_tree_get
				(&(cb->epath_to_entity_map_info),
				(SaUint8T *)&(evt->req_evt.hpi_evt.epath_key));
	if (NULL == epath_to_ent ) {
		LOG_ER("Entity corresponding to ent_path %s not found. Possibly\
			the HE is yet to be verified.",
			evt->req_evt.hpi_evt.entity_path);
		TRACE_LEAVE2("Return Val: %d",NCSCC_RC_FAILURE);
		return NCSCC_RC_FAILURE;
	}
	
	ent = epath_to_ent->plms_entity;

	if (NULL == ent){
		LOG_ER("Entity path %s found but epath_to_ent->plms_entity is \
				NULL", evt->req_evt.hpi_evt.entity_path);
		TRACE_LEAVE2("Return Val: %d",NCSCC_RC_FAILURE);
		return NCSCC_RC_FAILURE;
	}
	TRACE("Present state: %d, Received hpi-state: %d. No Operation.",
	ent->entity.he_entity.saPlmHEPresenceState,
	evt->req_evt.hpi_evt.sa_hpi_evt.EventDataUnion.HotSwapEvent.
	HotSwapState);

	return NCSCC_RC_SUCCESS;
}

/******************************************************************************
@brief		: Process M4 HPI event for HE. 

@param[in]	: ent - PLMS_ENTITY type representing a HE.

@return		: NCSCC_RC_FAILURE/NCSCC_RC_SUCCESS
******************************************************************************/
static SaUint32T plms_he_active_process(PLMS_ENTITY *ent)
{
	SaUint32T ret_err = NCSCC_RC_SUCCESS;
	PLMS_GROUP_ENTITY *act_aff_ent_list = NULL;
	PLMS_GROUP_ENTITY *log_head;
	PLMS_TRACK_INFO trk_info;
	PLMS_ENTITY_GROUP_INFO_LIST *log_head_grp;
	SaUint8T is_flag_aff = 0;
	PLMS_CB *cb = plms_cb;
	
	TRACE_ENTER2("Entity: %s",ent->dn_name_str);
	
	if (SA_PLM_HE_ADMIN_LOCKED_INACTIVE == 
			ent->entity.he_entity.saPlmHEAdminState){

		ret_err = plms_he_deactivate(ent,0,1);
		LOG_ER("Entity is deactivated as the admin state is lckinact. \
		Ent: %s",ent->dn_name_str);
		
		TRACE_LEAVE2("Return Val: %d",ret_err);
		return ret_err;
	}	

	/* Move the HE to InSvc is other conditions are matched.*/
	if (NCSCC_RC_SUCCESS == plms_move_ent_to_insvc(ent,&is_flag_aff)){
		TRACE("Entity %s is moved to insvc.",ent->dn_name_str);
		/* Move the children to insvc and clear the dependency flag.*/
		plms_move_chld_ent_to_insvc(ent->leftmost_child,
		&act_aff_ent_list,0,1);
		/* Move the dependent entities to InSvc and clear the
		dependency flag.*/
		plms_move_dep_ent_to_insvc(ent->rev_dep_list,
		&act_aff_ent_list,1);

		 TRACE("Affected entities for ent %s: ", ent->dn_name_str);
		 log_head = act_aff_ent_list;
			 while(log_head){
			 	TRACE("%s,",log_head->plm_entity->dn_name_str);
			 	log_head = log_head->next;
			 }
		/* Overwrite the expected readiness status with the current.*/
		plms_aff_ent_exp_rdness_state_ow(act_aff_ent_list);
	
		/*Fill the expected readiness state of the root ent.*/
		plms_ent_exp_rdness_state_ow(ent);
		
		memset(&trk_info,0,sizeof(PLMS_TRACK_INFO));
		trk_info.aff_ent_list = act_aff_ent_list;
		trk_info.group_info_list = NULL;
	
		/* Add the groups, root entity(ent) belong to.*/
		plms_ent_grp_list_add(ent,
					&(trk_info.group_info_list));
		
		/* Find out all the groups, all affected entities belong to 
		and add the groups to trk_info->group_info_list.*/ 
		plms_ent_list_grp_list_add(act_aff_ent_list,
					&(trk_info.group_info_list));	
		
		TRACE("Affected groups for ent %s: ",ent->dn_name_str);
		log_head_grp = trk_info.group_info_list;
		while(log_head_grp){
			TRACE("%llu,",log_head_grp->ent_grp_inf->entity_grp_hdl);
			log_head_grp = log_head_grp->next;
		}
		
		trk_info.change_step = SA_PLM_CHANGE_COMPLETED;
		trk_info.grp_op = SA_PLM_GROUP_MEMBER_READINESS_CHANGE;
		trk_info.root_correlation_id = SA_NTF_IDENTIFIER_UNUSED; 
		if ((NULL != ent->trk_info) && (SA_PLM_CAUSE_FAILURE_CLEARED == ent->trk_info->track_cause)){
			plms_ent_to_ent_list_add(ent,&(trk_info.aff_ent_list));
			trk_info.track_cause = ent->trk_info->track_cause;
			trk_info.root_entity = ent->trk_info->root_entity;
			plms_cbk_call(&trk_info,0/* Dont add root*/);
		}else{
			trk_info.track_cause = SA_PLM_CAUSE_HE_ACTIVATED;
			trk_info.root_entity = ent;
			plms_cbk_call(&trk_info,1/* Add root*/);
		}
		
		plms_ent_exp_rdness_status_clear(ent);
		plms_aff_ent_exp_rdness_status_clear(act_aff_ent_list);
		
		plms_ent_list_free(trk_info.aff_ent_list);
		trk_info.aff_ent_list = NULL;
		
		plms_ent_grp_list_free(trk_info.group_info_list);
		trk_info.group_info_list = NULL;

	}else if(is_flag_aff){
		/*Fill the expected readiness state of the root ent.*/
		plms_ent_exp_rdness_state_ow(ent);
		
		memset(&trk_info,0,sizeof(PLMS_TRACK_INFO));
		trk_info.group_info_list = NULL;
	
		/* Add the groups, root entity(ent) belong to.*/
		plms_ent_grp_list_add(ent,
					&(trk_info.group_info_list));
		
		TRACE("Affected groups for ent %s: ",ent->dn_name_str);
		log_head_grp = trk_info.group_info_list;
		while(log_head_grp){
			TRACE("%llu,",log_head_grp->ent_grp_inf->entity_grp_hdl);
			log_head_grp = log_head_grp->next;
		}
		trk_info.change_step = SA_PLM_CHANGE_COMPLETED;
		trk_info.grp_op = SA_PLM_GROUP_MEMBER_READINESS_CHANGE;
		trk_info.root_correlation_id = SA_NTF_IDENTIFIER_UNUSED; 
		if ((NULL != ent->trk_info) && (SA_PLM_CAUSE_FAILURE_CLEARED == ent->trk_info->track_cause)){
			plms_ent_to_ent_list_add(ent,&(trk_info.aff_ent_list));
			trk_info.track_cause = ent->trk_info->track_cause;
			trk_info.root_entity = ent->trk_info->root_entity;
			plms_cbk_call(&trk_info,0/* Dont add root*/);
		}else{
			trk_info.track_cause = SA_PLM_CAUSE_HE_ACTIVATED;
			trk_info.root_entity = ent;
			plms_cbk_call(&trk_info,1/* Add root*/);
		}
		
		plms_ent_exp_rdness_status_clear(ent);
		plms_aff_ent_exp_rdness_status_clear(trk_info.aff_ent_list);
	
		plms_ent_list_free(trk_info.aff_ent_list);
		trk_info.aff_ent_list = NULL;

		plms_ent_grp_list_free(trk_info.group_info_list);
		trk_info.group_info_list = NULL;
	}

	if (NULL != ent->trk_info){
		/* For admin repair, return to IMM.*/
		if ( SA_PLM_ADMIN_REPAIRED == ent->trk_info->imm_adm_opr_id){
			ent->trk_info->root_entity->am_i_aff_ent = FALSE;
			ent->trk_info->root_entity->adm_op_in_progress = FALSE;
			saImmOiAdminOperationResult(cb->oi_hdl,ent->trk_info->inv_id,SA_AIS_OK);
		}

		plms_trk_info_free(ent->trk_info);
		ent->trk_info = NULL;
	}
	TRACE_LEAVE2("Return Val: %d",ret_err);
	return ret_err;

}
/******************************************************************************
@brief     	: Process M0 for HE when the HE is already in OOS. 
		1. Clear the readiness flag.
		2. Wipe out current entity path and HEType.

@param[in]	: ent - PLMS_ENTITY representation of the HE. 

@return		: NCSCC_RC_FAILURE/NCSCC_RC_SUCCESS
******************************************************************************/

SaUint32T plms_he_oos_to_np_process(PLMS_ENTITY *ent)
{
	SaUint32T ret_err = NCSCC_RC_SUCCESS, root_aff = 0;
	PLMS_GROUP_ENTITY *chld_list = NULL,*log_head,*head;
	PLMS_GROUP_ENTITY *act_aff_ent_list = NULL;
	PLMS_TRACK_INFO trk_info;
	PLMS_ENTITY_GROUP_INFO_LIST *log_head_grp;
	PLMS_EPATH_TO_ENTITY_MAP_INFO *dummy = NULL;
	
	TRACE_ENTER2("Entity: %s",ent->dn_name_str);

	memset(&trk_info,0,sizeof(PLMS_TRACK_INFO));
	
	/* Mark the presence state of that HE to not-present.*/
	plms_presence_state_set(ent, SA_PLM_HE_PRESENCE_NOT_PRESENT,
				NULL,SA_NTF_OBJECT_OPERATION,
				SA_PLM_NTFID_STATE_CHANGE_ROOT);
	
	if ( ((PLMS_HE_ENTITY == ent->entity_type ) && 
	(PLMS_READINESS_FLAG_RESET != 
	ent->entity.he_entity.saPlmHEReadinessFlags)) || 
	((PLMS_EE_ENTITY == ent->entity_type ) && 
	(PLMS_READINESS_FLAG_RESET !=
	ent->entity.ee_entity.saPlmEEReadinessFlags))){
		/* Clear the readiness flag for the HE. */
		plms_readiness_flag_clear(ent,NULL,SA_NTF_OBJECT_OPERATION,
					SA_PLM_NTFID_STATE_CHANGE_ROOT);
		root_aff = 1;
	}
	
	/* For all the child HEs, mark the presence state to not-present and
	clear the readiness flags.
	For all the child EEs, we have to mark the prsence state to 
	uninstantiated and clear the readiness flag. But all the child EEs 
	should already be in uninstantiated state as the parent is in OOS. 
	Hence only clear the readiness flag.
	*/
	
	plms_chld_get(ent->leftmost_child,&chld_list);
	head = chld_list;
	while(head){

		switch(head->plm_entity->entity_type){

		case PLMS_HE_ENTITY:
			if(SA_PLM_HE_PRESENCE_NOT_PRESENT !=
					head->plm_entity->entity.he_entity.
					saPlmHEPresenceState){

				plms_presence_state_set(head->plm_entity,
						SA_PLM_HE_PRESENCE_NOT_PRESENT,
						NULL,SA_NTF_OBJECT_OPERATION,
						SA_PLM_NTFID_STATE_CHANGE_DEP);
				
				if ( PLMS_READINESS_FLAG_RESET != 
				head->plm_entity->entity.he_entity.
				saPlmHEReadinessFlags){
					plms_readiness_flag_clear(
						head->plm_entity,	
						ent,SA_NTF_OBJECT_OPERATION,
						SA_PLM_NTFID_STATE_CHANGE_DEP);

					plms_ent_to_ent_list_add(
							head->plm_entity,
							&act_aff_ent_list);
				}
				/* Clean up for the HE.*/
				plms_he_np_clean_up(head->plm_entity,&dummy);
			}else{
				/* Nothing to. The HE is already in 
				not-present state.*/
				TRACE("The HE is already is in not-present \
				state.");
			}
			break;

		case PLMS_EE_ENTITY:
			if ( PLMS_READINESS_FLAG_RESET != 
			head->plm_entity->entity.ee_entity.
			saPlmEEReadinessFlags){
		
				plms_readiness_flag_clear(head->plm_entity,
					ent,SA_NTF_OBJECT_OPERATION,
					SA_PLM_NTFID_STATE_CHANGE_DEP);
				
				plms_ent_to_ent_list_add(
					head->plm_entity,&act_aff_ent_list);
			}
			break;
		default:
			LOG_ER("Invalid Entity type: %d",ent->entity_type);
			break;
		}
		head = head->next;
	}
	TRACE("Affected entities for ent %s: ", ent->dn_name_str);
	log_head = act_aff_ent_list;
	while(log_head){
		TRACE("%s,",log_head->plm_entity->dn_name_str);
		log_head = log_head->next;
	}

	plms_aff_ent_exp_rdness_state_ow(act_aff_ent_list);
	plms_ent_exp_rdness_state_ow(ent);
	
	trk_info.group_info_list = NULL;
	if (root_aff){
		/* Add the groups, root entity(ent) belong to.*/
		plms_ent_grp_list_add(ent,&(trk_info.group_info_list));
	}
	
	/* Find out all the groups, all affected entities belong to and add 
	the groups to trk_info->group_info_list.*/ 
	plms_ent_list_grp_list_add(act_aff_ent_list,
				&(trk_info.group_info_list));	

	TRACE("Affected groups for ent %s: ",ent->dn_name_str);
	log_head_grp = trk_info.group_info_list;
	while(log_head_grp){
		TRACE("%llu,",log_head_grp->ent_grp_inf->entity_grp_hdl);
		log_head_grp = log_head_grp->next;
	}
	
	if (NULL != trk_info.group_info_list){
		trk_info.aff_ent_list = act_aff_ent_list;
		trk_info.change_step = SA_PLM_CHANGE_COMPLETED;
		trk_info.root_entity = ent;
		trk_info.root_correlation_id = SA_NTF_IDENTIFIER_UNUSED;
		trk_info.track_cause = SA_PLM_CAUSE_HE_DEACTIVATION;
		trk_info.grp_op = SA_PLM_GROUP_MEMBER_READINESS_CHANGE;

		plms_cbk_call(&trk_info,1);
	}

	plms_ent_exp_rdness_status_clear(ent);
	plms_aff_ent_exp_rdness_status_clear(act_aff_ent_list);

	plms_he_np_clean_up(ent,&dummy);

	plms_ent_list_free(act_aff_ent_list);
	plms_ent_grp_list_free(trk_info.group_info_list);
	trk_info.group_info_list = NULL;
	plms_ent_list_free(chld_list);
	
	TRACE_LEAVE2("Return Val: %d",ret_err);
	return ret_err;
}

/******************************************************************************
@brief     	: Process M0 for HE in insvc.
		1. Take the EE to OOS.
		2. Clear the readiness flag.
		3. Wipe out current entity path and HEType.

@param[in]	: ent - PLMS_ENTITY representation of the HE. 

@return		: NCSCC_RC_FAILURE/NCSCC_RC_SUCCESS
******************************************************************************/
SaUint32T plms_he_insvc_to_np_process(PLMS_ENTITY *ent)
{
	SaUint32T ret_err = NCSCC_RC_FAILURE;
	PLMS_TRACK_INFO trk_info;
	PLMS_GROUP_ENTITY *aff_ent_list = NULL, *act_ent_list = NULL;
	PLMS_GROUP_ENTITY *head = NULL,*log_head, *chld_list = NULL;
	PLMS_EPATH_TO_ENTITY_MAP_INFO *dummy = NULL;
	PLMS_ENTITY_GROUP_INFO_LIST *log_head_grp;
	
	TRACE_ENTER2("Entity: %s",ent->dn_name_str);
	
	memset(&trk_info,0,sizeof(PLMS_TRACK_INFO));
	
	/* Get aff ent and the chld list.*/
	plms_affected_ent_list_get(ent,&aff_ent_list,0);
	plms_chld_get(ent->leftmost_child,&chld_list);

	/* Take the ent to OOS and clean the flags.*/
	plms_presence_state_set(ent, SA_PLM_HE_PRESENCE_NOT_PRESENT,
				NULL,SA_NTF_OBJECT_OPERATION,
				SA_PLM_NTFID_STATE_CHANGE_ROOT);
	
	plms_readiness_state_set(ent, SA_PLM_READINESS_OUT_OF_SERVICE,
				NULL,SA_NTF_OBJECT_OPERATION,
				SA_PLM_NTFID_STATE_CHANGE_ROOT);
	
	plms_readiness_flag_clear(ent,NULL,
				SA_NTF_OBJECT_OPERATION,
				SA_PLM_NTFID_STATE_CHANGE_ROOT);


	/* Merge the chld and aff list.*/
	log_head = chld_list;
	while(log_head){
		plms_ent_to_ent_list_add(log_head->plm_entity,
		&aff_ent_list);
		log_head = log_head->next;
	}
	
	head = aff_ent_list;
	while(head){
		
		if (!plms_is_rdness_state_set(head->plm_entity,
		SA_PLM_READINESS_OUT_OF_SERVICE)){
			
			plms_readiness_state_set(head->plm_entity,
				SA_PLM_READINESS_OUT_OF_SERVICE,
				ent,SA_NTF_OBJECT_OPERATION,
				SA_PLM_NTFID_STATE_CHANGE_DEP);
			plms_ent_to_ent_list_add(head->plm_entity,&act_ent_list);
		}

		if (PLMS_EE_ENTITY == head->plm_entity->entity_type){
			if (!plms_is_chld(ent,head->plm_entity)){
				ret_err = plms_ee_term(head->plm_entity,
							FALSE,FALSE);
				if (NCSCC_RC_SUCCESS != ret_err){
					LOG_ER("EE %s termination FAILED.",
							ent->dn_name_str);
				}else{
					TRACE("EE %s termination SUCCESSFUL",
							ent->dn_name_str);
				}
				if (!plms_rdness_flag_is_set(head->plm_entity,
				SA_PLM_RF_DEPENDENCY)){
					plms_readiness_flag_mark_unmark(
						head->plm_entity,
						SA_PLM_RF_DEPENDENCY,
						TRUE,ent,
						SA_NTF_OBJECT_OPERATION,
						SA_PLM_NTFID_STATE_CHANGE_DEP);	
					plms_ent_to_ent_list_add(
					head->plm_entity,&act_ent_list);
				}
			}else{
				plms_presence_state_set(head->plm_entity,
					SA_PLM_EE_PRESENCE_UNINSTANTIATED,
					ent,SA_NTF_OBJECT_OPERATION,
					SA_PLM_NTFID_STATE_CHANGE_DEP);
				
				if ( PLMS_READINESS_FLAG_RESET != 
				head->plm_entity->entity.ee_entity.
				saPlmEEReadinessFlags ){
					plms_readiness_flag_clear(
					head->plm_entity,ent,
					SA_NTF_OBJECT_OPERATION,
					SA_PLM_NTFID_STATE_CHANGE_DEP);
				
					plms_ent_to_ent_list_add(
					head->plm_entity,&act_ent_list);
				}
					
			}

		}else {
			if (!plms_is_chld(ent,head->plm_entity)){
				if (!plms_rdness_flag_is_set(head->plm_entity,
				SA_PLM_RF_DEPENDENCY)){
					plms_readiness_flag_mark_unmark(
						head->plm_entity,
						SA_PLM_RF_DEPENDENCY,
						TRUE,ent,
						SA_NTF_OBJECT_OPERATION,
						SA_PLM_NTFID_STATE_CHANGE_DEP);	
				
					plms_ent_to_ent_list_add(
					head->plm_entity,&act_ent_list);
				}
				
			}else{
				plms_presence_state_set(head->plm_entity,
						SA_PLM_HE_PRESENCE_NOT_PRESENT,
						ent,SA_NTF_OBJECT_OPERATION,
						SA_PLM_NTFID_STATE_CHANGE_DEP);
				
				if ( PLMS_READINESS_FLAG_RESET != 
				head->plm_entity->entity.he_entity.
				saPlmHEReadinessFlags ){
					plms_readiness_flag_clear(
					head->plm_entity,ent,
					SA_NTF_OBJECT_OPERATION,
					SA_PLM_NTFID_STATE_CHANGE_DEP);
				
					plms_ent_to_ent_list_add(
					head->plm_entity,&act_ent_list);
				}
				/* Clean up the HE.*/
				plms_he_np_clean_up(head->plm_entity,&dummy);
			}
		}
		head = head->next;
	}
	plms_ent_exp_rdness_state_ow(ent);
	plms_aff_ent_exp_rdness_state_ow(act_ent_list);

	memset(&trk_info,0,sizeof(PLMS_TRACK_INFO));
	trk_info.group_info_list = NULL;
	/* Add the groups, root entity(ent) belong to.*/
	plms_ent_grp_list_add(ent,&(trk_info.group_info_list));

	/* Find out all the groups, all affected entities 
	 * belong to and add the groups to trk_info->group_info_list. */ 
	plms_ent_list_grp_list_add(act_ent_list,
				&(trk_info.group_info_list));	

	TRACE("Affected groups for ent %s: ",ent->dn_name_str);
	log_head_grp = trk_info.group_info_list;
	while(log_head_grp){
		TRACE("%llu,",log_head_grp->ent_grp_inf->entity_grp_hdl);
		log_head_grp = log_head_grp->next;
	}

	trk_info.root_entity = ent;
	trk_info.aff_ent_list = act_ent_list;
	/* TODO: What should be the cause.*/
	trk_info.track_cause = SA_PLM_CAUSE_HE_DEACTIVATION;
	trk_info.change_step = 	SA_PLM_CHANGE_COMPLETED;
	trk_info.grp_op = SA_PLM_GROUP_MEMBER_READINESS_CHANGE;
	
	plms_cbk_call(&trk_info,1);
	
	plms_ent_exp_rdness_status_clear(ent);
	plms_aff_ent_exp_rdness_status_clear(act_ent_list);

	plms_ent_list_free(trk_info.aff_ent_list);
	trk_info.aff_ent_list = NULL;
	plms_ent_grp_list_free(trk_info.group_info_list);
	trk_info.group_info_list = NULL;
	plms_ent_list_free(aff_ent_list);
	plms_ent_list_free(chld_list);

	/* As the HE is not-present, clean up PLMS database.
	1. Remove the entry from e-path to ent map tree.
	2. Wipe out saPlmHECurrHEType and saPlmHECurrEntityPath.*/
	plms_he_np_clean_up(ent,&dummy);

	TRACE_LEAVE2("Return Val: %d",ret_err);
	return ret_err;
}

/******************************************************************************
@brief		: Process M2/M3 HPI event when the HE is in insvc.

@param[in]	: ent - PLMS_ENTITY representation of the HE. 

@return		: NCSCC_RC_FAILURE/NCSCC_RC_SUCCESS
******************************************************************************/
static SaUint32T plms_he_insvc_to_acting_process(PLMS_ENTITY *ent)
{
	SaUint32T ret_err = NCSCC_RC_FAILURE;
	PLMS_TRACK_INFO trk_info;
	PLMS_GROUP_ENTITY *aff_ent_list = NULL;
	PLMS_GROUP_ENTITY *head = NULL;
	PLMS_ENTITY_GROUP_INFO_LIST *log_head_grp;
	
	TRACE_ENTER2("Entity: %s",ent->dn_name_str);
	
	plms_readiness_state_set(ent,SA_PLM_READINESS_OUT_OF_SERVICE,
				NULL,SA_NTF_OBJECT_OPERATION,
				SA_PLM_NTFID_STATE_CHANGE_ROOT);

	plms_affected_ent_list_get(ent, &aff_ent_list,0);

	head = aff_ent_list;
	while (head){
		if(!plms_is_chld(ent,head->plm_entity) &&
		PLMS_EE_ENTITY == head->plm_entity->entity_type){
			ret_err = plms_ee_term(head->plm_entity,FALSE,0);
			if (NCSCC_RC_SUCCESS != ret_err){
				LOG_ER("EE %s termination FAILED.",
							ent->dn_name_str);
			}else{
				TRACE("EE %s termination SUCCESSFUL",
							ent->dn_name_str);
			}
		}
		plms_readiness_state_set(head->plm_entity,
				SA_PLM_READINESS_OUT_OF_SERVICE,ent,
				SA_NTF_OBJECT_OPERATION,
				SA_PLM_NTFID_STATE_CHANGE_DEP);
		plms_readiness_flag_mark_unmark(head->plm_entity,
				SA_PLM_RF_DEPENDENCY,TRUE,ent,
				SA_NTF_OBJECT_OPERATION,
				SA_PLM_NTFID_STATE_CHANGE_DEP);
		head = head->next;
	 }
	plms_aff_ent_exp_rdness_state_ow(aff_ent_list);
	plms_ent_exp_rdness_state_ow(ent);
	
	memset(&trk_info,0,sizeof(PLMS_TRACK_INFO));
	trk_info.group_info_list = NULL;

	/* Add the groups, root entity(ent) belong to.*/
	plms_ent_grp_list_add(ent, &(trk_info.group_info_list));

	/* Find out all the groups, all affected entities 
	 * belong to and add the groups to 
	 * trk_info->group_info_list. */ 
	plms_ent_list_grp_list_add(aff_ent_list,
				&(trk_info.group_info_list));	

	TRACE("Affected groups for ent %s: ",ent->dn_name_str);
	log_head_grp = trk_info.group_info_list;
	while(log_head_grp){
		TRACE("%llu,",log_head_grp->ent_grp_inf->entity_grp_hdl);
		log_head_grp = log_head_grp->next;
	}

	trk_info.aff_ent_list = aff_ent_list;
	trk_info.root_entity = ent;
	trk_info.change_step = SA_PLM_CHANGE_COMPLETED;
	trk_info.track_cause = SA_PLM_CAUSE_HE_DEACTIVATION;
	trk_info.root_correlation_id = SA_NTF_IDENTIFIER_UNUSED;
	trk_info.grp_op = SA_PLM_GROUP_MEMBER_READINESS_CHANGE;

	plms_cbk_call(&trk_info,1);

	plms_aff_ent_exp_rdness_status_clear(aff_ent_list);
	plms_ent_exp_rdness_status_clear(ent);

	plms_ent_list_free(trk_info.aff_ent_list);
	trk_info.aff_ent_list = NULL;
	plms_ent_grp_list_free(trk_info.group_info_list);
	trk_info.group_info_list = NULL;
	
	if (SA_PLM_HE_ADMIN_LOCKED_INACTIVE == 
			ent->entity.he_entity.saPlmHEAdminState){

		ret_err = plms_he_deactivate(ent,0,1);
		LOG_ER("Entity is deactivated as the admin state is lckinact. \
		Ent: %s",ent->dn_name_str);

		TRACE_LEAVE2("Return Val: %d",ret_err);
		return ret_err;
	}	
	
	/*Done with the clean up job for missed events. Now allow the entity to
	activate only if the board supports full 5 state model.
	*/
	if (PLMS_HPI_FULL_FIVE_HOTSWAP_MODEL == ent->state_model){
		ret_err = plms_he_activate(ent,FALSE,TRUE);
		if (NCSCC_RC_SUCCESS != ret_err){
			LOG_ER("EE %s termination FAILED.", ent->dn_name_str);
		}else{
			TRACE("EE %s termination SUCCESSFUL", ent->dn_name_str);
		}
	}

	TRACE_LEAVE2("Return Val: %d",ret_err);
	return ret_err;
}

/******************************************************************************
@brief		: Clean up the previous admin operation so that the current 
		operation can be performed. This function completes the prev
		admin operation and clean up the context(trk_info).

@param[in]	: ent - PLMS_ENTITY representation of the HE. 

@return		: NCSCC_RC_FAILURE/NCSCC_RC_SUCCESS
******************************************************************************/
static SaUint32T plms_adm_cnxt_clean_up(PLMS_ENTITY *ent)
{
	PLMS_ENTITY_GROUP_INFO_LIST *head;
	PLMS_CB *cb = plms_cb;
	PLMS_GROUP_ENTITY *aff_head = NULL;
	SaUint32T ret_err = NCSCC_RC_FAILURE;
	
	TRACE_ENTER2("Entity: %s",ent->dn_name_str);

	if (SA_PLM_CAUSE_LOCK == ent->adm_op_in_progress){
		
		TRACE("Lock operation is in progress.");
		switch(ent->trk_info->change_step){
		
		case SA_PLM_CHANGE_START:
			TRACE(" Lock operation, START cbk in progress.");
			/* Remove the inv_to_cbk nodes.*/	
			head = ent->trk_info->group_info_list;
			while (head){
				plms_inv_to_cbk_in_grp_trk_rmv(
					head->ent_grp_inf, ent->trk_info);
				head=head->next;
			}
			/* Terminate the affected non-child EEs. */
			aff_head = ent->trk_info->aff_ent_list;
			while (aff_head){
				if ((PLMS_EE_ENTITY == 
					aff_head->plm_entity->entity_type) 
					&& !plms_is_chld(ent,aff_head->plm_entity)){

					ret_err = plms_ee_term(
					aff_head->plm_entity,FALSE,TRUE);

					if (NCSCC_RC_SUCCESS != ret_err){
						LOG_ER(" EE %s termination \
						failed.", aff_head->plm_entity
						->dn_name_str);
					}else{
						TRACE(" EE %s termination \
						Successful.", aff_head->\
						plm_entity->dn_name_str);
					}
				}
				aff_head = aff_head->next;
			}
			ent->trk_info->change_step = SA_PLM_CHANGE_COMPLETED;
			ent->trk_info->track_count = 0;
			plms_cbk_call(ent->trk_info,1);
			
			plms_aff_ent_exp_rdness_status_clear(
			ent->trk_info->aff_ent_list);

			plms_ent_exp_rdness_status_clear(ent);
			
			ret_err = saImmOiAdminOperationResult( cb->oi_hdl,
				ent->trk_info->inv_id, SA_AIS_OK);
			if (NCSCC_RC_SUCCESS != ret_err){
				LOG_ER("Sending admin response to IMM failed. \
						IMM Ret code: %d",ret_err);
			}else{
				TRACE("Sending admin response to IMM \
								successful.");
			}
			
			ent->adm_op_in_progress = FALSE;
			ent->am_i_aff_ent = FALSE;
			plms_aff_ent_flag_mark_unmark(
					ent->trk_info->aff_ent_list,FALSE);

			plms_trk_info_free(ent->trk_info);
			break;	

		case SA_PLM_CHANGE_VALIDATE:
			
			TRACE(" Lock operation, VALIDATE cbk in progress.");
#if 0			
			plms_admin_state_set(ent,SA_PLM_HE_ADMIN_LOCKED,NULL,
						SA_NTF_OBJECT_OPERATION,
						SA_PLM_NTFID_STATE_CHANGE_ROOT);
#endif
			/* Clean up the validate context.*/
			head = ent->trk_info->group_info_list;
			while (head){
				plms_inv_to_cbk_in_grp_trk_rmv(
					head->ent_grp_inf, ent->trk_info);
				head=head->next;
			}
			/* Call abort callback.*/
			ent->trk_info->change_step = SA_PLM_CHANGE_ABORTED;
			plms_ent_exp_rdness_state_ow(ent);
			plms_aff_ent_exp_rdness_state_ow(ent->trk_info->
			aff_ent_list);
			ent->trk_info->track_count = 0;
			plms_cbk_call(ent->trk_info,1);

			/* Send response to IMM.*/
			ret_err = saImmOiAdminOperationResult( cb->oi_hdl,
				ent->trk_info->inv_id, SA_AIS_ERR_DEPLOYMENT);
			if (NCSCC_RC_SUCCESS != ret_err){
				LOG_ER("Sending admin response to IMM failed.\
				IMM Ret code: %d",ret_err);
			}else{
				TRACE("Sending admin response to IMM \
								successful.");
			}
			
			plms_aff_ent_exp_rdness_status_clear(
			ent->trk_info->aff_ent_list);
			plms_ent_exp_rdness_status_clear(ent);

			ent->adm_op_in_progress = FALSE;
			ent->am_i_aff_ent = FALSE;
			plms_aff_ent_flag_mark_unmark(
					ent->trk_info->aff_ent_list,FALSE);

			plms_trk_info_free(ent->trk_info);
			break;
		default:
			LOG_ER("Invalid change step: %d",
						ent->trk_info->change_step);
			break;
		}
	}else if(SA_PLM_CAUSE_SHUTDOWN == ent->adm_op_in_progress){
		
		TRACE("Shutdown operation is in progress.");
		switch(ent->trk_info->change_step){
		
		case SA_PLM_CHANGE_START:
			
			TRACE("Shutdown operation START cbk in progress.");
			/* Remove all inv_to_cbk nodes from the entity group.
			*/
			head = ent->trk_info->group_info_list;
			while (head){
				plms_inv_to_cbk_in_grp_trk_rmv(
					head->ent_grp_inf,ent->trk_info);
				head=head->next;
			}
			
			plms_admin_state_set(ent,SA_PLM_HE_ADMIN_LOCKED,NULL,
						SA_NTF_OBJECT_OPERATION,
						SA_PLM_NTFID_STATE_CHANGE_ROOT);
						
			plms_readiness_state_set(ent,
					SA_PLM_READINESS_OUT_OF_SERVICE,NULL,
					SA_NTF_MANAGEMENT_OPERATION,
					SA_PLM_NTFID_STATE_CHANGE_ROOT);

			/* Mark readiness state of all the affected ent to OOS 
			and set the dependency flag.*/
			aff_head = ent->trk_info->aff_ent_list;
			while (aff_head){
				plms_readiness_state_set(aff_head->plm_entity,
						SA_PLM_READINESS_OUT_OF_SERVICE,
						ent,SA_NTF_MANAGEMENT_OPERATION,
						SA_PLM_NTFID_STATE_CHANGE_DEP);
				plms_readiness_flag_mark_unmark(
						aff_head->plm_entity,
						SA_PLM_RF_DEPENDENCY,TRUE,
						ent,SA_NTF_MANAGEMENT_OPERATION,
						SA_PLM_NTFID_STATE_CHANGE_DEP);

				aff_head = aff_head->next;
			}

			/* Terminate the affected non-child EEs. */
			aff_head = ent->trk_info->aff_ent_list;
			while (aff_head){
				if ((PLMS_EE_ENTITY == 
					aff_head->plm_entity->entity_type) 
					&& !plms_is_chld(ent,aff_head->plm_entity)){
					
					ret_err = plms_ee_term(
					aff_head->plm_entity,FALSE,1);
				}
				aff_head = aff_head->next;
			}

			ent->trk_info->change_step = SA_PLM_CHANGE_COMPLETED;
			ent->trk_info->track_count = 0;
			plms_cbk_call(ent->trk_info,1);
			
			plms_aff_ent_exp_rdness_status_clear(
			ent->trk_info->aff_ent_list);

			plms_ent_exp_rdness_status_clear(ent);

			/* Send response to IMM.*/
			ret_err = saImmOiAdminOperationResult( cb->oi_hdl,
				ent->trk_info->inv_id, SA_AIS_OK);
			if (NCSCC_RC_SUCCESS != ret_err){
				LOG_ER("Sending admin response to IMM failed. \
						IMM Ret code: %d",ret_err);
			}else{
				TRACE("Sending admin response to IMM \
								successful.");
			}
			
			ent->adm_op_in_progress = FALSE;
			ent->am_i_aff_ent = FALSE;
			plms_aff_ent_flag_mark_unmark(
					ent->trk_info->aff_ent_list,FALSE);

			plms_trk_info_free(ent->trk_info);
			break;
		default:
			LOG_ER("Invalid schange step: %d",
						ent->trk_info->change_step);
			break;
		}
	}else{
		LOG_ER("Invalid in-progress adm operation: %d",
						ent->adm_op_in_progress);
	}

	TRACE_LEAVE2("Return Val: %d",ret_err);
	return ret_err;
}
/******************************************************************************
@brief		: Process HPI hot swap events. This function is called from the
		MBX. This function does the following things
		1. If the HE is verified i.e. the transition is not from M0, then
		call the required FSM function.
		2. If the HE is yet to be verified and the hot swap event is M2
		or M4, then verify the HE and call the FSM. If verification fails,
		ignore the HE.

		Note: If the verification fails, PLMS is supposed to send the 
		nearest verified HE as part of the notification and domain if
		HE is directly configured under domain or there is no verifed
		ancestor HE. We are sending the domain name in the all above
		case, which is slight deviation from spec. TODO.

@param[in]	: evt - PLMS_EVT representation of the HE. 

@return		: NCSCC_RC_FAILURE/NCSCC_RC_SUCCESS
******************************************************************************/
SaUint32T plms_hpi_hs_evt_process(PLMS_EVT *evt)
{
	SaUint32T ret_err = NCSCC_RC_FAILURE;
	PLMS_EPATH_TO_ENTITY_MAP_INFO *epath_to_ent;
	PLMS_ENTITY *ent;
	SaPlmHETypeT *curHEType;
	SaNameT cur_he_type_dn;
	SaInt8T cur_he_type_dn_str[SA_MAX_NAME_LENGTH];
	SaNameT hpi_dn;
	PLMS_CB *cb = plms_cb;
	PLMS_HPI_EVT *hpi_evt = &(evt->req_evt.hpi_evt);
	SaNameT domain_name;
	
	TRACE_ENTER2("Entity: %s, evt_state: %d",hpi_evt->entity_path,\
	hpi_evt->sa_hpi_evt.EventDataUnion.HotSwapEvent.HotSwapState);

	if (cb->ha_state == SA_AMF_HA_STANDBY) {
		TRACE_LEAVE2("Ignoring the event as current role is standby");
		return NCSCC_RC_SUCCESS;
	}

	memset(&hpi_dn,0,sizeof(SaNameT));
	/* Send HPI event notification.*/
	hpi_dn.length = strlen(HPI_DN_NAME);
	memcpy(hpi_dn.value,HPI_DN_NAME,hpi_dn.length);
	ret_err= plms_hpi_evt_ntf_send(cb->ntf_hdl,&hpi_dn,
					SAHPI_ET_HOTSWAP,
					hpi_evt->entity_path,
					cb->domain_id,&hpi_evt->sa_hpi_evt,
					0,NULL/*cor_ids*/,NULL/*ntf_buf*/);
	if (NCSCC_RC_SUCCESS != ret_err){
		LOG_ER("Hpi event received notification sending FAILED.\
						Ret Value: %d",ret_err);
	}
	
	/* See if the entity is verified before. If the entity path published
	by HPI is not present in epath_to_entity patricia tree, then the HE
	is yet to be verified.*/
	epath_to_ent = (PLMS_EPATH_TO_ENTITY_MAP_INFO *)ncs_patricia_tree_get(
					&(cb->epath_to_entity_map_info),
					(SaUint8T *)&(hpi_evt->epath_key));
	if (NULL == epath_to_ent){
		TRACE("The HE %s is yet to be verified.",hpi_evt->entity_path);
		/* The HE is not verified and hence call HE verifiaction
		routine. But verification can be done only getting 
		INSERTION-PENDING and ACTIVE as we get inv-data.*/
		if((SAHPI_HS_STATE_INSERTION_PENDING ==  
		hpi_evt->sa_hpi_evt.EventDataUnion.HotSwapEvent.HotSwapState) || 
		(SAHPI_HS_STATE_ACTIVE == 
		hpi_evt->sa_hpi_evt.EventDataUnion.HotSwapEvent.HotSwapState)){
			ret_err = plms_he_verify(hpi_evt,&curHEType,&ent);
			if (NCSCC_RC_SUCCESS != ret_err){
				LOG_ER(" HE %s verification FAILED.",
							hpi_evt->entity_path);
				
				memset(&domain_name,0,sizeof(SaNameT));
				
				domain_name.length = 
				strlen(cb->domain_info.domain.safDomain);
				
				memcpy(domain_name.value,
				cb->domain_info.domain.safDomain,
				domain_name.length); 

				/* Send unmapped HE alaram.*/
				ret_err = plms_alarm_ntf_send(cb->ntf_hdl,
						/*TODO nearest parent*/ 
						&domain_name,	
						SA_NTF_ALARM_EQUIPMENT,
						hpi_evt->entity_path,
						SA_NTF_SEVERITY_CRITICAL,
						SA_NTF_AUTHENTICATION_FAILURE,
						SA_PLM_NTFID_UNMAPPED_HE_ALARM,
						0,NULL/*cor_ids*/,NULL/*buf*/);
				if(NCSCC_RC_SUCCESS != ret_err){
					LOG_ER("Unmapped HE notification \
					sending FAILED. Ent: %s",
					hpi_evt->entity_path);
				}
				/* Ignore the HE.*/
				TRACE_LEAVE2("Return Val: %d",ret_err);
				return ret_err;
			}
			TRACE(" HE %s verification SUCCESSFUL.",
							hpi_evt->entity_path);
			
			/* Verification is successful.*/
			/* Copy RDN of cur he type.*/
			memcpy(cur_he_type_dn.value,curHEType->safVersion,
						strlen(curHEType->safVersion));
			/* Two RDNs are separated by comma.*/
			cur_he_type_dn.value[strlen(curHEType->safVersion)] = 
									',';
			/*Append the DN of base he type to rdn of cur he type.*/
			memcpy(&(cur_he_type_dn.value[
				strlen(curHEType->safVersion) + 1 ]),
				ent->entity.he_entity.saPlmHEBaseHEType.value,
				ent->entity.he_entity.saPlmHEBaseHEType.length);
			/* Fill the length of cur he type.*/	
			cur_he_type_dn.length = strlen(curHEType->safVersion) + 
			ent->entity.he_entity.saPlmHEBaseHEType.length + 1;
			
			/* Finally populate the current HE type.*/
			ent->entity.he_entity.saPlmHECurrHEType.length = 
							cur_he_type_dn.length;
			memcpy(ent->entity.he_entity.saPlmHECurrHEType.value,
			cur_he_type_dn.value,cur_he_type_dn.length);

			/* Update IMM.*/
			plms_attr_saname_imm_update(ent,"saPlmHECurrHEType",
			ent->entity.he_entity.saPlmHECurrHEType,
			SA_IMM_ATTR_VALUES_REPLACE);

			/* For logging.*/
			memcpy(cur_he_type_dn_str,cur_he_type_dn.value,
						cur_he_type_dn.length);
			cur_he_type_dn_str[cur_he_type_dn.length] = '\0';
			TRACE("Matched Current HE Type : %s",
							cur_he_type_dn_str);

			ent->entity.he_entity.saPlmHECurrEntityPath = 
			(SaStringT)calloc(1,strlen(hpi_evt->entity_path) +1);
			
			/* Fill current ent path.*/
			memcpy(ent->entity.he_entity.saPlmHECurrEntityPath,
					hpi_evt->entity_path,
					strlen(hpi_evt->entity_path)+1);
			
			/* Update IMM.*/
			plms_attr_sastring_imm_update(ent,
			"saPlmHECurrEntityPath",
			ent->entity.he_entity.saPlmHECurrEntityPath,
			SA_IMM_ATTR_VALUES_REPLACE);

			/* Populate the entity state model.*/
			ent->state_model = hpi_evt->state_model;

			/* Add the epath to entity mapping to patricia tree.*/
			epath_to_ent = (PLMS_EPATH_TO_ENTITY_MAP_INFO *)calloc(1,
					sizeof(PLMS_EPATH_TO_ENTITY_MAP_INFO));
			
			epath_to_ent->entity_path = (SaInt8T *)calloc(1,
			strlen(hpi_evt->entity_path) +1);

			memcpy(epath_to_ent->entity_path,hpi_evt->entity_path,
					strlen(hpi_evt->entity_path) +1);

			epath_to_ent->epath_key = hpi_evt->epath_key;

			epath_to_ent->plms_entity = ent;
			epath_to_ent->pat_node.key_info = (SaUint8T *)
						&(epath_to_ent->epath_key);
			
			epath_to_ent->pat_node.bit = 0;
			epath_to_ent->pat_node.left = NCS_PATRICIA_NODE_NULL;
			epath_to_ent->pat_node.right = NCS_PATRICIA_NODE_NULL;

			ret_err = ncs_patricia_tree_add(
						&cb->epath_to_entity_map_info,
						&epath_to_ent->pat_node);
			if (NCSCC_RC_SUCCESS != ret_err){
				LOG_ER("Patricia tree add of epath_to_ent to \
				cb->epath_to_entity_map_info failed.\
				ret_err = %d", ret_err);

				free(ent->entity.he_entity.
				saPlmHECurrEntityPath);
				ent->entity.he_entity.saPlmHECurrEntityPath 
				= NULL;

				free(epath_to_ent->entity_path);
				epath_to_ent->entity_path = NULL;

				free(epath_to_ent);
				epath_to_ent = NULL;

				ent->entity.he_entity.saPlmHECurrHEType.length 
				= 0;
				memset(ent->entity.he_entity.
				saPlmHECurrHEType.value,0,SA_MAX_NAME_LENGTH);
	
				TRACE_LEAVE2("Return Val: %d",ret_err);
				return ret_err;
				
			}
			/* Call HE presence FSM.*/
			TRACE(" HE %s verified. Call FP. \
			Current st: %d, Received Hpi st: %d",ent->dn_name_str,\
			ent->entity.he_entity.saPlmHEPresenceState,\
			hpi_evt->sa_hpi_evt.EventDataUnion.HotSwapEvent.\
			HotSwapState);
			
			ret_err = plms_HE_pres_state_op
			[ent->entity.he_entity.saPlmHEPresenceState]
			[hpi_evt->sa_hpi_evt.EventDataUnion.HotSwapEvent.
			HotSwapState](evt); 
		}else{
			LOG_ER("HE %s is yet to be verified, but the hpi event\
			is neither M2 nor M4. Hence can not find Inv-data to\
			verify the HE.Ignore and return",hpi_evt->entity_path);
		}

	}else{

		ent = epath_to_ent->plms_entity;
		TRACE(" HE %s already verified. Call FP. \
		Current st: %d, Received Hpi st: %d",ent->dn_name_str,\
		ent->entity.he_entity.saPlmHEPresenceState,\
		hpi_evt->sa_hpi_evt.EventDataUnion.HotSwapEvent.HotSwapState);

		/* Populate the entity state model.*/
		ent->state_model = hpi_evt->state_model;

		/* Verification is not needed. I am assuming if the entity is 
		present in epath_to_entity tree, then it is verified and hence
		not cheking the CurrEntityPath and CurrHEType. Call FSM.*/
		ret_err = plms_HE_pres_state_op
		[ent->entity.he_entity.saPlmHEPresenceState]
		[hpi_evt->sa_hpi_evt.EventDataUnion.HotSwapEvent.HotSwapState]
		(evt); 
	}

	TRACE_LEAVE2("Return Val: %d",ret_err);
	return ret_err;
}
/******************************************************************************
@brief		: Verifies the HE.
		1. Verifies configured entity paths against hpi published.
		2. Verifies configured inv date against the published one.

		Notes: If the configured inv data if NULL(for string type) or
		0(integer type), then that field is not taken into consideration
		for verification.

@param[in]	: hpi_evt - PLMS_HPI_EVT representation of the HE. 
@param[out]	: o_he_type - If the verification is successful, this points to
		the matched HEType which will be used to populate current HEType.
@param[out]	: o_ent - If the verification is successful, this points to the
		matched entity HE.

@return		: NCSCC_RC_FAILURE/NCSCC_RC_SUCCESS
******************************************************************************/
static SaUint32T plms_he_verify(PLMS_HPI_EVT *hpi_evt,
				SaPlmHETypeT **o_he_type,
				PLMS_ENTITY **o_ent)
{
	SaUint32T ret_err = NCSCC_RC_FAILURE;
	SaUint32T max;
	PLMS_ENTITY *ent;
	SaUint32T epath_matched = 0, he_type_idr_matched = 0;
	PLMS_HE_BASE_INFO *base_he_type;
	PLMS_HE_TYPE_INFO *he_type_list;
	SaUint8T *cur_epath = NULL;
	PLMS_CB *cb = plms_cb;
	TRACE_ENTER2("Entity: %s",hpi_evt->entity_path);
	
	ent = (PLMS_ENTITY *)ncs_patricia_tree_getnext(&cb->entity_info,NULL);
	while (ent){
		/* If the ent is not HE, skip this ent.*/
		if (PLMS_HE_ENTITY != ent->entity_type){
			ent = (PLMS_ENTITY *)ncs_patricia_tree_getnext(
						&cb->entity_info,
						(SaUint8T *)&(ent->dn_name));
			continue;
		}
		/* If the HE is mapped ent, then skip this ent.*/
		if (NULL != ent->entity.he_entity.saPlmHECurrEntityPath){
			ent = (PLMS_ENTITY *)ncs_patricia_tree_getnext(
						&cb->entity_info,
						(SaUint8T *)&(ent->dn_name));
			continue;

		}
		/* If the HPI published entity path is matched with 
		any of the configured entity path. If not then skip this
		entity.*/
		epath_matched = 0;
		for ( max = 0; SAHPI_MAX_ENTITY_PATH > max; max++){
			if ((ent->entity.he_entity.saPlmHEEntityPaths[max])){
				if ( 0 == strcmp(ent->entity.he_entity.
					saPlmHEEntityPaths[max], 
					hpi_evt->entity_path)) {
					
					epath_matched = 1;
					break;
				}
			}else{
				break;
			}
		}
		/* No configured epath matched. So skip this ent.*/
		if (!epath_matched){
			ent = (PLMS_ENTITY *)ncs_patricia_tree_getnext(
						&cb->entity_info,
						(SaUint8T *)&(ent->dn_name));
			if (cur_epath){
				free(cur_epath);
				cur_epath = NULL;
			}
			continue;
		}

		/* One of the unmapped HEs entity path is matched. Match the
		inv data now.*/
		if (cur_epath){
			free(cur_epath);
			cur_epath = NULL;
		}
		/* Get the baseHEType.*/
		base_he_type = (PLMS_HE_BASE_INFO *)ncs_patricia_tree_get(
			&cb->base_he_info,
			(SaUint8T *)&(ent->entity.he_entity.saPlmHEBaseHEType));
		/* Scan through the HEType list and match the INV date.*/
		he_type_list = base_he_type->he_type_info_list;
		while(he_type_list){
			ret_err = plms_inv_data_compare(
				he_type_list->inv_data,hpi_evt->inv_data);
			if (NCSCC_RC_SUCCESS != ret_err){
				/* Did not match. Go for next HEType.*/
				he_type_list = he_type_list->next;
			} else{
				/* Matched*/
				*o_ent = ent;
				*o_he_type = &(he_type_list->he_type);
				return NCSCC_RC_SUCCESS;
			}
		}
		ent = (PLMS_ENTITY *)ncs_patricia_tree_getnext(
					&cb->entity_info,
					(SaUint8T *)&(ent->dn_name));
	}

	if (!(epath_matched & he_type_idr_matched) ){
		LOG_ER("epath_matched: %d, he_type_idr_matched: %d\
		HE. ent_path: %s",epath_matched,
		he_type_idr_matched,hpi_evt->entity_path);
	}

	TRACE_LEAVE2("Return Val: %d",ret_err);
	return NCSCC_RC_FAILURE;
}

/******************************************************************************
@brief		: Verifies configured inv date against the published one.
		If the configured inv data if NULL(for string type) or
		0(integer type), then that field is not taken into consideration
		for verification.

@param[in]	: conf_inv - configured inv data.
@param[in]      : hpi_inv - hpi published inv data.

@return		: NCSCC_RC_FAILURE/NCSCC_RC_SUCCESS
******************************************************************************/
static SaUint32T plms_inv_data_compare(PLMS_INV_DATA conf_inv, PLMS_INV_DATA hpi_inv)
{

	/************************      CHASSIS INFO        **********/
	
	/* Compare chassis info: format version.*/
	if ((conf_inv.chassis_area.format_version)){
		if(conf_inv.chassis_area.format_version == 
				hpi_inv.chassis_area.format_version){
		} else{
			LOG_ER("Chassis_info->format version did not match.\
			conf: %d, published: %d",
			conf_inv.chassis_area.format_version,
			hpi_inv.chassis_area.format_version);
			return NCSCC_RC_FAILURE;
		}
	}
	
	/* Compare chassis info: chassis_type*/
	if ((conf_inv.chassis_area.chassis_type)){
		if(conf_inv.chassis_area.chassis_type == 
				hpi_inv.chassis_area.chassis_type){
		} else{
			LOG_ER("Chassis_info->chassis type did not match.\
			conf: %d, published: %d",
			conf_inv.chassis_area.chassis_type,
			hpi_inv.chassis_area.chassis_type);
			return NCSCC_RC_FAILURE;
		}
	}
	
	/* Compare chassis info: serial no.*/
	if((conf_inv.chassis_area.serial_no.DataLength)){
		if (conf_inv.chassis_area.serial_no.DataLength !=
			hpi_inv.chassis_area.serial_no.DataLength){
			LOG_ER("Chassis_info->serial no did not match.\
			conf: %d, published: %d",
			conf_inv.chassis_area.serial_no.DataLength,
			hpi_inv.chassis_area.serial_no.DataLength);
			return NCSCC_RC_FAILURE;
		}
		if (0 == memcmp(conf_inv.chassis_area.serial_no.Data,
			hpi_inv.chassis_area.serial_no.Data,
			conf_inv.chassis_area.serial_no.DataLength)){
		}else{
			LOG_ER("Chassis_info->serial no did not match");
			return NCSCC_RC_FAILURE;
		}
	}
	
	/* Compare chassis info: part no.*/
	if((conf_inv.chassis_area.part_no.DataLength)){
		if (conf_inv.chassis_area.part_no.DataLength !=
			hpi_inv.chassis_area.part_no.DataLength){
			LOG_ER("Chassis_info->part no did not match.\
			conf: %d, published: %d",
			conf_inv.chassis_area.part_no.DataLength,
			hpi_inv.chassis_area.part_no.DataLength);
			return NCSCC_RC_FAILURE;
		}
		if (0 == memcmp(conf_inv.chassis_area.part_no.Data,
			hpi_inv.chassis_area.part_no.Data,
			conf_inv.chassis_area.part_no.DataLength)){
		}else{
			LOG_ER("Chassis_info->part no did not match");
			return NCSCC_RC_FAILURE;
		}
	}
	
	/************************      BOARD INFO        **********/

	/* Compare board info: version.*/
	if ((conf_inv.board_area.version)){
		if(conf_inv.board_area.version ==
				hpi_inv.board_area.version){
		}
		else{
			LOG_ER("Board_info->version did not match.\
			Conf: %d, Published: %d",
			conf_inv.board_area.version,
			hpi_inv.board_area.version);
			return NCSCC_RC_FAILURE;
		}
	}
	
	/* Compare board info: manufacturer name*/
	if((conf_inv.board_area.manufacturer_name.DataLength)){
		if (conf_inv.board_area.manufacturer_name.DataLength !=
			hpi_inv.board_area.manufacturer_name.DataLength){
			LOG_ER("Board_info->version did not match.\
			Conf: %d, Published: %d",
			conf_inv.board_area.manufacturer_name.DataLength,
			hpi_inv.board_area.manufacturer_name.DataLength);
			return NCSCC_RC_FAILURE;
		}
		if (0 == memcmp(conf_inv.board_area.manufacturer_name.Data,
			hpi_inv.board_area.manufacturer_name.Data,
			conf_inv.board_area.manufacturer_name.DataLength)){
		}else{
			LOG_ER("Board_info->version did not match");
			return NCSCC_RC_FAILURE;
		}
	}
	/* Compare board info: product name*/
	if((conf_inv.board_area.product_name.DataLength)){
		if (conf_inv.board_area.product_name.DataLength !=
			hpi_inv.board_area.product_name.DataLength){
			LOG_ER("Board_info->product name did not match.\
			Conf: %d, Published: %d",
			conf_inv.board_area.product_name.DataLength,
			hpi_inv.board_area.product_name.DataLength);
			return NCSCC_RC_FAILURE;
		}
		if (0 == memcmp(conf_inv.board_area.product_name.Data,
			hpi_inv.board_area.product_name.Data,
			conf_inv.board_area.product_name.DataLength)){
		}else{
			LOG_ER("Board_info->product name did not match");
			return NCSCC_RC_FAILURE;
		}
	}
	
	/* Compare board info: serial no*/
	if((conf_inv.board_area.serial_no.DataLength)){
		if (conf_inv.board_area.serial_no.DataLength !=
			hpi_inv.board_area.serial_no.DataLength){
			LOG_ER("Board_info->serial no did not match.\
			Conf: %d, Published: %d",
			conf_inv.board_area.serial_no.DataLength,
			hpi_inv.board_area.serial_no.DataLength);
			return NCSCC_RC_FAILURE;
		}
		if (0 == memcmp(conf_inv.board_area.serial_no.Data,
			hpi_inv.board_area.serial_no.Data,
			conf_inv.board_area.serial_no.DataLength)){
		}else{
			LOG_ER("Board_info->serial no did not match");
			return NCSCC_RC_FAILURE;
		}
	}
	/* Compare board info: part no*/
	if((conf_inv.board_area.part_no.DataLength)){
		if (conf_inv.board_area.part_no.DataLength !=
			hpi_inv.board_area.part_no.DataLength){
			LOG_ER("Board_info->serial no did not match.\
			Conf: %d, Published: %d",
			conf_inv.board_area.part_no.DataLength,
			hpi_inv.board_area.part_no.DataLength);
			return NCSCC_RC_FAILURE;
		}
		if (0 == memcmp(conf_inv.board_area.part_no.Data,
			hpi_inv.board_area.part_no.Data,
			conf_inv.board_area.part_no.DataLength)){
		}else{
			LOG_ER("Board_info->part no did not match");
			return NCSCC_RC_FAILURE;
		}
	}

	/************************      PRODUCT INFO        **********/
	
	/* Compare product info: format version.*/
	if ((conf_inv.product_area.format_version)){
		if(conf_inv.product_area.format_version ==
				hpi_inv.product_area.format_version){
		}
		else{
			LOG_ER("Product_info->format_version did not match.\
			Conf: %d, Published: %d",
			conf_inv.product_area.format_version,
			hpi_inv.product_area.format_version);
			return NCSCC_RC_FAILURE;
		}
	}
	/* Compare product info: manufacturer name*/
	if((conf_inv.product_area.manufacturer_name.DataLength)){
		if (conf_inv.product_area.manufacturer_name.DataLength !=
			hpi_inv.product_area.manufacturer_name.DataLength){
			LOG_ER("Product_info->manf name did not match.\
			Conf: %d, Published: %d",
			conf_inv.product_area.manufacturer_name.DataLength,
			hpi_inv.product_area.manufacturer_name.DataLength);
			return NCSCC_RC_FAILURE;
		}
		if (0 == memcmp(conf_inv.product_area.manufacturer_name.Data,
			hpi_inv.product_area.manufacturer_name.Data,
			conf_inv.product_area.manufacturer_name.DataLength)){
		}else{
			LOG_ER("Product_info->manf name did not match.");
			return NCSCC_RC_FAILURE;
		}
	}
	/* Compare product info: product name*/
	if((conf_inv.product_area.product_name.DataLength)){
		if (conf_inv.product_area.product_name.DataLength !=
			hpi_inv.product_area.product_name.DataLength){
			LOG_ER("Product_info->product name did not match.\
			Conf: %d, Published: %d",
			conf_inv.product_area.product_name.DataLength,
			conf_inv.product_area.product_name.DataLength);
			return NCSCC_RC_FAILURE;
		}
		if (0 == memcmp(conf_inv.product_area.product_name.Data,
			hpi_inv.product_area.product_name.Data,
			conf_inv.product_area.product_name.DataLength)){
		}else{
			LOG_ER("Product_info->product name did not match.");
			return NCSCC_RC_FAILURE;
		}
	}
	
	/* Compare product info: serial no*/
	if((conf_inv.product_area.serial_no.DataLength)){
		if (conf_inv.product_area.serial_no.DataLength !=
			hpi_inv.product_area.serial_no.DataLength){
			LOG_ER("Product_info->serial no did not match.\
			Conf: %d, Published: %d",
			conf_inv.product_area.serial_no.DataLength,
			hpi_inv.product_area.serial_no.DataLength);
			return NCSCC_RC_FAILURE;
		}
		if (0 == memcmp(conf_inv.product_area.serial_no.Data,
			hpi_inv.product_area.serial_no.Data,
			conf_inv.product_area.serial_no.DataLength)){
		}else{
			LOG_ER("Product_info->serial no did not match.");
			return NCSCC_RC_FAILURE;
		}
	}
	/* Compare product info: part no*/
	if((conf_inv.product_area.part_no.DataLength)){
		if (conf_inv.product_area.part_no.DataLength !=
			hpi_inv.product_area.part_no.DataLength){
			LOG_ER("Product_info->part no did not match.\
			Conf: %d, Published: %d",
			conf_inv.product_area.part_no.DataLength,
			hpi_inv.product_area.part_no.DataLength);
			return NCSCC_RC_FAILURE;
		}
		if (0 == memcmp(conf_inv.product_area.part_no.Data,
			hpi_inv.product_area.part_no.Data,
			conf_inv.product_area.part_no.DataLength)){
		}else{
			LOG_ER("Product_info->part no did not match.");
			return NCSCC_RC_FAILURE;
		}
	}
	/* Compare product info: product version */
	if((conf_inv.product_area.product_version.DataLength)){
		if (conf_inv.product_area.product_version.DataLength !=
			hpi_inv.product_area.product_version.DataLength){
			LOG_ER("Product_info->version did not match.\
			Conf: %d, Published: %d",
			conf_inv.product_area.product_version.DataLength,
			hpi_inv.product_area.product_version.DataLength);
			return NCSCC_RC_FAILURE;
		}
		if (0 == memcmp(conf_inv.product_area.product_version.Data,
			hpi_inv.product_area.product_version.Data,
			conf_inv.product_area.product_version.DataLength)){
		}else{
			LOG_ER("Product_info->version did not match.");
			return NCSCC_RC_FAILURE;
		}
	}
	/* Compare product info: asset tag */
	if((conf_inv.product_area.asset_tag.DataLength)){
		if (conf_inv.product_area.asset_tag.DataLength !=
			hpi_inv.product_area.asset_tag.DataLength){
			LOG_ER("Product_info->asset did not match.\
			Conf: %d, Published: %d",
			conf_inv.product_area.asset_tag.DataLength,
			hpi_inv.product_area.asset_tag.DataLength);
			return NCSCC_RC_FAILURE;
		}
		if (0 == memcmp(conf_inv.product_area.asset_tag.Data,
			hpi_inv.product_area.asset_tag.Data,
			conf_inv.product_area.asset_tag.DataLength)){
		}else{
			LOG_ER("Product_info->asset tag did not match.");
			return NCSCC_RC_FAILURE;
		}
	}
	/* Compare product info: fru field id */
	if((conf_inv.product_area.fru_field_id.DataLength)){
		if (conf_inv.product_area.fru_field_id.DataLength !=
			hpi_inv.product_area.fru_field_id.DataLength){
			LOG_ER("Product_info->fru id did not match.\
			Conf: %d, Published: %d",
			conf_inv.product_area.fru_field_id.DataLength,
			hpi_inv.product_area.fru_field_id.DataLength);
			return NCSCC_RC_FAILURE;
		}
		if (0 == memcmp(conf_inv.product_area.fru_field_id.Data,
			hpi_inv.product_area.fru_field_id.Data,
			conf_inv.product_area.fru_field_id.DataLength)){
		}else{
			LOG_ER("Product_info->fru id did not match.");
			return NCSCC_RC_FAILURE;
		}
	}

	/* Ivt data verification successful.*/
	TRACE_LEAVE2("Return Val: %d",NCSCC_RC_SUCCESS);
	return NCSCC_RC_SUCCESS;
}

/******************************************************************************
@brief		: Deactivate the HE,taking the state model into consideration.
		If the HE is in not present state or already in deactivated
		state, then return success without initiating hpi deactivation.
		
		Notes: This also takes care of setting management lost flag, admin
		operation pending flag and sending callback for management
		lost.

@param[in]	: ent - PLMS_ENTITY representation of HE.
@param[in]	: is_adm_op - If adm operation or not. If set to 1 and HE deactivation
		fails, then admin pending flag is set.
@param[in]	: mngt_cbk - If set to 1 and HE deactivation fails, then call the
		management lost flag.

@return		: NCSCC_RC_FAILURE/NCSCC_RC_SUCCESS
******************************************************************************/
SaUint32T plms_he_deactivate(PLMS_ENTITY *ent,SaUint32T is_adm_op,SaUint32T mngt_cbk)
{
	SaUint32T cbk = 0;
	SaUint32T ret_err = NCSCC_RC_FAILURE;
	
	if (NULL == ent){
		LOG_ER("HE deact failed. Ent NULL");
		return NCSCC_RC_FAILURE;
	}

	TRACE_ENTER2("Entity: %s",ent->dn_name_str);
	TRACE("State model of the ent: %d",ent->state_model);

	if ((SA_PLM_HE_PRESENCE_INACTIVE == 
	ent->entity.he_entity.saPlmHEPresenceState) || 
	(SA_PLM_HE_PRESENCE_NOT_PRESENT == 
	ent->entity.he_entity.saPlmHEPresenceState)){
		TRACE("HE %s is in %d state, Deact not initiated.",
		ent->dn_name_str,ent->entity.he_entity.saPlmHEPresenceState);

		TRACE_LEAVE2("Return Val: %d",NCSCC_RC_SUCCESS);
		return NCSCC_RC_SUCCESS;
	}
	
	switch(ent->state_model){
	
	case PLMS_HPI_TWO_HOTSWAP_MODEL:
		/* TODO: */
		ret_err = plms_hrb_req(ent,
				PLMS_HPI_CMD_ACTION_REQUEST,
				SAHPI_HS_ACTION_EXTRACTION/*arg*/);
		break;
	case PLMS_HPI_THREE_HOTSWAP_MODEL:
		ret_err = plms_hrb_req(ent,
				PLMS_HPI_CMD_RESOURCE_POWER_OFF,
				SAHPI_POWER_OFF/*arg*/);
		break;

	case PLMS_HPI_PARTIAL_FIVE_HOTSWAP_MODEL:
	case PLMS_HPI_FULL_FIVE_HOTSWAP_MODEL:
		if (SA_PLM_HE_PRESENCE_ACTIVE == 
			ent->entity.he_entity.saPlmHEPresenceState){
			if (!ent->deact_in_pro){	
				ret_err = plms_hrb_req(ent,
					PLMS_HPI_CMD_RESOURCE_POWER_OFF,
					SAHPI_POWER_OFF/*arg*/);
				if (NCSCC_RC_SUCCESS == ret_err)
					ent->deact_in_pro = TRUE;
			}else{
				ret_err = NCSCC_RC_SUCCESS;
			}

		}else if((SA_PLM_HE_PRESENCE_ACTIVATING == 
		ent->entity.he_entity.saPlmHEPresenceState) || 
		(SA_PLM_HE_PRESENCE_DEACTIVATING == 
		ent->entity.he_entity.saPlmHEPresenceState)){
			
			if (!ent->deact_in_pro){	
				ret_err = plms_hrb_req(ent,
					PLMS_HPI_CMD_RESOURCE_INACTIVE_SET,0);
				if (NCSCC_RC_SUCCESS == ret_err)
					ent->deact_in_pro = TRUE;
			}else{
				ret_err = NCSCC_RC_SUCCESS;
			}

		}else{
			ret_err = NCSCC_RC_SUCCESS;
		}
		break;
	default:
		LOG_ER("Invalid state model: %d",ent->state_model);
		ret_err = NCSCC_RC_FAILURE;
		TRACE_LEAVE2("Return Val: %d",ret_err);
		return ret_err;
	}

	if (NCSCC_RC_SUCCESS != ret_err){
		if(is_adm_op){
			if (!plms_rdness_flag_is_set(ent,
			SA_PLM_RF_ADMIN_OPERATION_PENDING)){ 	
				plms_readiness_flag_mark_unmark(ent,
					 SA_PLM_RF_ADMIN_OPERATION_PENDING,
					 1, NULL,SA_NTF_MANAGEMENT_OPERATION,
					SA_PLM_NTFID_STATE_CHANGE_ROOT);
				cbk = 1;
			}
		}
			
		if (!plms_rdness_flag_is_set(ent,
		SA_PLM_RF_MANAGEMENT_LOST)){ 	
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
		
		ent->mngt_lost_tri = PLMS_MNGT_HE_DEACTIVATE;
		
		ret_err = NCSCC_RC_FAILURE;
	}else {/* Deactivation is successful.*/
		ent->iso_method = PLMS_ISO_DEFAULT;
#if 0
		if (plms_rdness_flag_is_set(ent,SA_PLM_RF_MANAGEMENT_LOST))
			plms_deact_resp_mngt_flag_clear(ent);
#endif			
	}

	TRACE_LEAVE2("Return Val: %d",ret_err);
	return ret_err;
}
/******************************************************************************
@brief		: Activate the HE,taking the state model into consideration.
		If the HE is in not present state or already in activated
		state, then return success without initiating hpi activation.
		
		This also takes care of setting management lost flag, adm_op_
		pending flag and calling management lost callback if required.

@param[in]	: ent - PLMS_ENTITY representation of HE.
@param[in]	: is_adm_op - If adm operation or not. If set to 1 and HE activation
		fails, then admin pending flag is set.
@param[in]	: mngt_cbk - If set to 1 and HE activation fails, then call the
		management lost flag.

@return		: NCSCC_RC_FAILURE/NCSCC_RC_SUCCESS
******************************************************************************/
SaUint32T plms_he_activate(PLMS_ENTITY *ent,SaUint32T is_adm_op,SaUint32T mngt_cbk)
{
	SaUint32T cbk = 0;
	SaUint32T ret_err = NCSCC_RC_FAILURE;
	
	TRACE_ENTER2("Entity: %s",ent->dn_name_str);
	TRACE("State model of the ent: %d",ent->state_model);

	if( (SA_PLM_HE_PRESENCE_ACTIVE == 
	ent->entity.he_entity.saPlmHEPresenceState) || 
	((PLMS_HPI_TWO_HOTSWAP_MODEL != ent->state_model) && 
	(SA_PLM_HE_PRESENCE_NOT_PRESENT == 
	ent->entity.he_entity.saPlmHEPresenceState))){
		TRACE("HE %s is in %d state, Act not initiated.",
		ent->dn_name_str,ent->entity.he_entity.saPlmHEPresenceState);

		TRACE_LEAVE2("Return Val: %d",NCSCC_RC_SUCCESS);
		return NCSCC_RC_SUCCESS;
	}

	switch(ent->state_model){
	
	case PLMS_HPI_TWO_HOTSWAP_MODEL:
		/* TODO: */
		ret_err = plms_hrb_req(ent,
				PLMS_HPI_CMD_ACTION_REQUEST,
				SAHPI_HS_ACTION_INSERTION/*arg*/);
		break;
	case PLMS_HPI_THREE_HOTSWAP_MODEL:
		ret_err = plms_hrb_req(ent,
				PLMS_HPI_CMD_RESOURCE_POWER_ON,
				SAHPI_POWER_ON/*arg*/);
		break;
	
	case PLMS_HPI_PARTIAL_FIVE_HOTSWAP_MODEL:
	case PLMS_HPI_FULL_FIVE_HOTSWAP_MODEL:
		if (SA_PLM_HE_PRESENCE_INACTIVE == 
			ent->entity.he_entity.saPlmHEPresenceState){
			if (!ent->act_in_pro){	
				ret_err = plms_hrb_req(ent,
					PLMS_HPI_CMD_RESOURCE_POWER_ON,
					SAHPI_POWER_ON/*arg*/);
				if (NCSCC_RC_SUCCESS == ret_err){
					ent->act_in_pro = TRUE;
				}
			}else{
				ret_err = NCSCC_RC_SUCCESS;
			}
			
		}else if((SA_PLM_HE_PRESENCE_ACTIVATING == 
		ent->entity.he_entity.saPlmHEPresenceState) || 
		(SA_PLM_HE_PRESENCE_DEACTIVATING == 
		ent->entity.he_entity.saPlmHEPresenceState)){

			if (!ent->act_in_pro){
				ret_err = plms_hrb_req(ent,
					PLMS_HPI_CMD_RESOURCE_ACTIVE_SET,0);
				if(NCSCC_RC_SUCCESS == ret_err){
					ent->act_in_pro = TRUE;
				}
			}else{
				ret_err = NCSCC_RC_SUCCESS;
			}

		}else{
			ret_err = NCSCC_RC_SUCCESS;
		}
		break;
	default:
		LOG_ER("Invalid state model: %d",ent->state_model);
		ret_err = NCSCC_RC_FAILURE;
	}

	if (NCSCC_RC_SUCCESS != ret_err){
		if(is_adm_op){
			if (!plms_rdness_flag_is_set(ent,
			SA_PLM_RF_ADMIN_OPERATION_PENDING)){ 	
				plms_readiness_flag_mark_unmark(ent,
					 SA_PLM_RF_ADMIN_OPERATION_PENDING,
					 1, NULL,SA_NTF_MANAGEMENT_OPERATION,
					SA_PLM_NTFID_STATE_CHANGE_ROOT);
				cbk = 1;
			}
		}
			
		if (!plms_rdness_flag_is_set(ent,
		SA_PLM_RF_MANAGEMENT_LOST)){ 	
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
		
		ent->mngt_lost_tri = PLMS_MNGT_HE_ACTIVATE;
		ret_err = NCSCC_RC_FAILURE;
	}else{
#if 0		
		if (plms_rdness_flag_is_set(ent,SA_PLM_RF_MANAGEMENT_LOST))
			plms_act_resp_mngt_flag_clear(ent);
#endif			
	}

	TRACE_LEAVE2("Return Val: %d",ret_err);
	return ret_err;
}
/******************************************************************************
@brief		: Reset the HE. This also takes care of setting management lost flag, 
		adm_op_pending flag and calling management lost callback if required.

@param[in]	: ent - PLMS_ENTITY representation of HE.
@param[in]	: is_adm_op - If adm operation or not. If set to 1 and HE reset 
		fails, then admin pending flag is set.
@param[in]	: mngt_cbk - If set to 1 and HE reset fails, then call the
		management lost flag.
@param[in]	: reset_type - cold/warm/assert/deassert		

@return		: NCSCC_RC_FAILURE/NCSCC_RC_SUCCESS
******************************************************************************/
SaUint32T plms_he_reset(PLMS_ENTITY *ent,SaUint32T is_adm_op, 
					SaUint32T mngt_cbk, SaUint32T reset_type)
{
	SaUint32T cbk = 0;
	SaUint32T ret_err = NCSCC_RC_FAILURE;

	if (NULL == ent){
		LOG_ER("he_reset fail. Ent NULL.");
		return NCSCC_RC_FAILURE;
	}
	
	TRACE_ENTER2("Entity: %s",ent->dn_name_str);

	ret_err = plms_hrb_req(ent,
			PLMS_HPI_CMD_RESOURCE_RESET,reset_type/*arg*/);
	
	if (NCSCC_RC_SUCCESS != ret_err){
		if(is_adm_op){
			if (!plms_rdness_flag_is_set(ent,
			SA_PLM_RF_ADMIN_OPERATION_PENDING)){ 	
				plms_readiness_flag_mark_unmark(ent,
					 SA_PLM_RF_ADMIN_OPERATION_PENDING,
					 1, NULL,SA_NTF_MANAGEMENT_OPERATION,
					SA_PLM_NTFID_STATE_CHANGE_ROOT);
				cbk = 1;
			}
		}
			
		if (!plms_rdness_flag_is_set(ent,
		SA_PLM_RF_MANAGEMENT_LOST)){ 	
			/* Set management lost flag.*/
			plms_readiness_flag_mark_unmark(ent,
				SA_PLM_RF_MANAGEMENT_LOST,1/*mark*/,
				NULL,(is_adm_op ? SA_NTF_MANAGEMENT_OPERATION:SA_NTF_OBJECT_OPERATION),
				SA_PLM_NTFID_STATE_CHANGE_ROOT);
			cbk = 1;
		}
		
		/* Call managemnet lost callback.*/
		if (mngt_cbk && cbk){
			plms_mngt_lost_clear_cbk_call(ent,1/*mark*/);
		}

		switch(reset_type){
		case SAHPI_COLD_RESET:
			ent->mngt_lost_tri = PLMS_MNGT_HE_COLD_RESET;
			break;
		case SAHPI_WARM_RESET:
			ent->mngt_lost_tri = PLMS_MNGT_HE_WARM_RESET;
			break;
		case SAHPI_RESET_ASSERT:
			ent->mngt_lost_tri = PLMS_MNGT_HE_RESET_ASSERT;
			break;
		case SAHPI_RESET_DEASSERT:
			ent->mngt_lost_tri = PLMS_MNGT_HE_RESET_DEASSERT;
			break;
		default:
			break;
		}

		ret_err = NCSCC_RC_FAILURE;
	}else{
	}
	TRACE_LEAVE2("Return Val: %d",ret_err);
	return ret_err;
}

/******************************************************************************
@brief		: MDS sync request to HRB to execute cmd(activate/deactivate/reset). 

@param[in]	: ent - PLMS_ENTITY representation of HE.
@param[in]	: cmd - command to be executed.
@param[in]	: arg - argument to the cmd.

@return		: NCSCC_RC_FAILURE/NCSCC_RC_SUCCESS
******************************************************************************/
static SaUint32T plms_hrb_req(PLMS_ENTITY *ent,PLMS_HPI_CMD cmd,SaUint32T arg)
{
	SaUint32T ret_err = NCSCC_RC_FAILURE;
	PLMS_CB *cb = plms_cb;
	PLMS_HPI_REQ req;
	PLMS_HPI_RSP *resp = NULL;
	SaUint32T (* hrb_mds_send_func_ptr)(MDS_HDL mds_hdl, SaUint32T from_svc,
			                SaUint32T to_svc, MDS_DEST    to_dest,
					PLMS_HPI_REQ *i_evt, PLMS_HPI_RSP **o_evt,
					SaUint32T timeout) = NULL;

	
	TRACE_ENTER2("Entity: %s, hpi_cmd: %d",ent->dn_name_str,cmd);
	
	
	if ( NULL == ent->entity.he_entity.saPlmHECurrEntityPath ){
		
		LOG_ER("plms_hrb_req FAILED. Cur Ent Path is NULL Ent: %s,\
		cmd: %d", ent->dn_name_str,cmd);

		TRACE_LEAVE2("Return Val: %d",NCSCC_RC_FAILURE);
		return NCSCC_RC_FAILURE;
	}

	req.entity_path = ent->entity.he_entity.saPlmHECurrEntityPath;
	req.entity_path_len = strlen(
			ent->entity.he_entity.saPlmHECurrEntityPath);
	req.cmd = cmd;
	req.arg = arg;
	
	/* Get the hrb Init func ptr */
	hrb_mds_send_func_ptr = dlsym(cb->hpi_intf_hdl, 
				"plms_hrb_mds_msg_sync_send");
	if ( NULL == hrb_mds_send_func_ptr) {
		LOG_ER("dlsym()of plms_hrb_mds_msg_sync_send failed, error %s",dlerror());
		TRACE_LEAVE2("Return Val: %d",NCSCC_RC_FAILURE);
		return NCSCC_RC_FAILURE;
	}
	ret_err = (* hrb_mds_send_func_ptr)(cb->mds_hdl,
				NCSMDS_SVC_ID_PLMS,
				NCSMDS_SVC_ID_PLMS_HRB,
				cb->hrb_dest,&req,&resp,
				PLMS_HRB_MDS_TIMEOUT);
	if( NCSCC_RC_SUCCESS != ret_err ) {
		LOG_ER("plms_hrb_req FAILED. Ent: %s, cmd: %d",
		ent->dn_name_str,cmd);
	}else {
		if ( NCSCC_RC_SUCCESS != resp->ret_val){
			LOG_ER("plms_hrb_req FAILED. Ent: %s, cmd: %d\
			resp: %d",ent->dn_name_str,cmd,resp->ret_val);
			ret_err =  NCSCC_RC_FAILURE;
		}
	}
	
	/* Free the resp.*/
	if (NULL != resp)
		free(resp);

	TRACE_LEAVE2("Return Val: %d",ret_err);
	return ret_err;	
}
/******************************************************************************
@brief		: Clear management lost flag, admin pending flag and isolate
		pening flag on getting M1.

@param[in]	: ent - PLMS_ENTITY representation of HE.

@return		: NCSCC_RC_SUCCESS.
******************************************************************************/
static SaUint32T plms_deact_resp_mngt_flag_clear(PLMS_ENTITY *ent)
{

	/* If isolate pending flag is set then clear the flags return
	from here.*/
	if(plms_rdness_flag_is_set(ent, SA_PLM_RF_ISOLATE_PENDING)) {
		TRACE("Clear management lost flag,set for EE_ISOLATE. Ent: %s",
		ent->dn_name_str);
		
		plms_readiness_flag_mark_unmark(ent,
		SA_PLM_RF_MANAGEMENT_LOST,0/*unmark*/,NULL,
		SA_NTF_OBJECT_OPERATION,
		SA_PLM_NTFID_STATE_CHANGE_ROOT);

		/* Clear admin_pending as well as 
		isolate_pending flag if set.*/
		plms_readiness_flag_mark_unmark(ent,
		SA_PLM_RF_ISOLATE_PENDING,0,
		NULL, SA_NTF_OBJECT_OPERATION,
		SA_PLM_NTFID_STATE_CHANGE_ROOT);
		
		plms_readiness_flag_mark_unmark(ent,
		SA_PLM_RF_ADMIN_OPERATION_PENDING,0/*unmark*/,
		NULL, SA_NTF_OBJECT_OPERATION,
		SA_PLM_NTFID_STATE_CHANGE_ROOT);

		ent->mngt_lost_tri = PLMS_MNGT_NONE;
		plms_mngt_lost_clear_cbk_call(ent,0/*unmark*/);
		return NCSCC_RC_SUCCESS;
	}
	
	TRACE("Clear management lost flag. Reason %d, ent: %s",
	ent->mngt_lost_tri,ent->dn_name_str);
	
	plms_readiness_flag_mark_unmark(ent,
	SA_PLM_RF_MANAGEMENT_LOST,0/*unmark*/,
	NULL,SA_NTF_OBJECT_OPERATION,
	SA_PLM_NTFID_STATE_CHANGE_ROOT);

	/* Clear admin_pending flag if set.*/
	plms_readiness_flag_mark_unmark(ent,
	SA_PLM_RF_ADMIN_OPERATION_PENDING,
	0/*unmark*/,NULL,SA_NTF_OBJECT_OPERATION,
	SA_PLM_NTFID_STATE_CHANGE_ROOT);

	plms_mngt_lost_clear_cbk_call(ent,0/*unmark*/);
	ent->mngt_lost_tri = PLMS_MNGT_NONE;
	
	return NCSCC_RC_SUCCESS;
}
/******************************************************************************
@brief		: Clear management lost flag, admin pending flag and isolate
		pening flag on getting M4. If isolate pending flag is set,
		then isolate the entity and return failure if isolation is
		successful.

@param[in]	: ent - PLMS_ENTITY representation of HE.

@return		: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
******************************************************************************/
static SaUint32T plms_act_resp_mngt_flag_clear(PLMS_ENTITY *ent)
{

	if (plms_rdness_flag_is_set(ent,SA_PLM_RF_ISOLATE_PENDING)){
		if ( NCSCC_RC_SUCCESS == plms_isolate_and_mngt_lost_clear(ent))
			return NCSCC_RC_FAILURE; 
		else
			return NCSCC_RC_SUCCESS;
	}
	
	TRACE("Clear management lost flag. Reason %d, ent: %s",
	ent->mngt_lost_tri,ent->dn_name_str);
	
	/* Clear management lost flag.*/
	plms_readiness_flag_mark_unmark(ent,
	SA_PLM_RF_MANAGEMENT_LOST,0/*unmark*/,
	NULL,SA_NTF_OBJECT_OPERATION,
	SA_PLM_NTFID_STATE_CHANGE_ROOT);

	/* Clear admin_pending flag if set.*/
	plms_readiness_flag_mark_unmark(ent,
	SA_PLM_RF_ADMIN_OPERATION_PENDING,
	0/*unmark*/,NULL,SA_NTF_OBJECT_OPERATION,
	SA_PLM_NTFID_STATE_CHANGE_ROOT);

	plms_mngt_lost_clear_cbk_call(ent,0/*unmark*/);
	ent->mngt_lost_tri = PLMS_MNGT_NONE;
	
	return NCSCC_RC_SUCCESS;
}
/******************************************************************************
@brief		: Clear management lost flag, admin pending flag and isolate
		pening flag on getting M2/M3. If isolate pending flag is set,
		then isolate the entity and return failure if isolation is
		successful.

@param[in]	: ent - PLMS_ENTITY representation of HE.

@return		: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
******************************************************************************/
static SaUint32T plms_inspending_mngt_flag_clear(PLMS_ENTITY *ent)
{

	if (plms_rdness_flag_is_set(ent,SA_PLM_RF_ISOLATE_PENDING)){
		if ( NCSCC_RC_SUCCESS == plms_isolate_and_mngt_lost_clear(ent))
			return NCSCC_RC_FAILURE; 
		else
			return NCSCC_RC_SUCCESS;
	}
	
	TRACE("Clear management lost flag. Reason %d, ent: %s",
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

	return NCSCC_RC_SUCCESS;
}

/******************************************************************************
@brief		: Clear management lost flag, admin pending flag and isolate
		pening flag on getting M5/M6. If isolate pending flag is set,
		then isolate the entity and return failure if isolation is
		successful.

@param[in]	: ent - PLMS_ENTITY representation of HE.

@return		: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
******************************************************************************/
static SaUint32T plms_extpending_mngt_flag_clear(PLMS_ENTITY *ent)
{

	if (plms_rdness_flag_is_set(ent,SA_PLM_RF_ISOLATE_PENDING)){
		if ( NCSCC_RC_SUCCESS == plms_isolate_and_mngt_lost_clear(ent))
			return NCSCC_RC_FAILURE; 
		else
			return NCSCC_RC_SUCCESS;
	}

	TRACE("Clear management lost flag. Reason %d, ent: %s",
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
	
	return NCSCC_RC_SUCCESS;
}
