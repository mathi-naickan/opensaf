/*      -*- OpenSAF  -*-
 *
 * (C) Copyright 2008 The OpenSAF Foundation
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
..............................................................................

  DESCRIPTION:

  This file contains all Public APIs for the MBCSV.

  ..............................................................................

  FUNCTIONS INCLUDED in this module:

   ncs_mbcsv_svc                    - MBCSV API, processes different MBCSV client requests.
   mbcsv_process_initialize_request - Process Initialize request.
   mbcsv_process_get_sel_obj_request- Process get selection object request.
   mbcsv_process_dispatch_request  - Process dispatch request.
   mbcsv_process_finalize_request  - Process finalize request.
   mbcsv_process_open_request      - Process open request.
   mbcsv_process_close_request     - Process close request.
   mbcsv_process_chg_role_request  - Process change role request.
   mbcsv_process_snd_ckpt_request  - Process send checkpoint request.
   mbcsv_process_snd_ntfy_request  - Process Notify request.
   mbcsv_process_snd_data_req      - Process send data request.
   mbcsv_process_get_request       - Process get object request.
   mbcsv_process_set_request       - Process set object request.

  TRACEs in MBCSv adopt the following strategy:
  TRACE   debug traces                 - aproximates DEBUG.
  TRACE_1 normal but important events  - aproximates INFO.
  TRACE_2 user errors with return code - aproximates NOTICE.
  TRACE_3 unusual or strange events    - aproximates WARNING
  TRACE_4 library errors ERR_LIBRARY   - aproximates ERROR.

@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
*/
#include "mbcsv.h"
#include "base/ncssysf_mem.h"

static const MBCSV_PROCESS_REQ_FUNC_PTR
 mbcsv_init_process_req_func[NCS_MBCSV_OP_OBJ_SET + 1] = {
	mbcsv_process_initialize_request,
	mbcsv_process_get_sel_obj_request,
	mbcsv_process_dispatch_request,
	mbcsv_process_finalize_request,
	mbcsv_process_open_request,
	mbcsv_process_close_request,
	mbcsv_process_chg_role_request,
	mbcsv_process_snd_ckpt_request,
	mbcsv_process_snd_ntfy_request,
	mbcsv_process_snd_data_req,
	mbcsv_process_get_request,
	mbcsv_process_set_request
};

/*
 * Enable tracing early - GCC constructor
 */
__attribute__ ((constructor))
static void logtrace_init_constructor(void)
{
        char *value;
        /* Initialize trace system first of all so we can see what is going. */
        if ((value = getenv("MBCA_TRACE_PATHNAME")) != NULL) {
                if (logtrace_init("mbca", value, CATEGORY_ALL) != 0) {
                        /* error, we cannot do anything */
                        return;
                }
        }
}

/**********************************************************************************************
 * MBCSV protocol between vdests in ACTIVE and STANDBY role
 * ========================================================
 * COLD_SYNC_REQUEST		: STANDBY sends to ACTIVE requesting first/initial sync
 *				  of whole DB.
 * COLD_SYNC_RESPONSE		: ACTIVE responds to STANDBY as 1 message or
 *				  multiple response messages.
 * COLD_SYNC_COMPLETE		: ACTIVE sends to STANDBY indicating
 *				  this is the end of COLD_SYNC_RESPONS
 *				  sequence.
 *
 * ASYNC_UPDATE 		: ACTIVE sends to STANDBY for all delta updates.
 *
 * WARM_SYNC_REQUEST		: STANDBY sends to ACTIVE periodically requesting sync status.
 * WARM_SYNC_RESPONSE		: ACTIVE responds to STANDBY as 1 message or
 *				  multiple response messages.
 * WARM_SYNC_RESPONSE_COMPLETE	: ACTIVE sends to STANDBY indicating 
 *				  this is the end of WARM_SYNC_RESPONSE.
 *				  sequence.
 *
 * DATA_REQUEST			: STANDBY sends to ACTIVE requesting for
 *				  data when its out-of-sync.
 * DATA_RESPONSE		: ACTIVE responds to STANDBY as 1 message or
 *				  multiple response messages.
 * DATA_RESPONSE_COMPLETE	: ACTIVE sends to STANDBY indicating
 *				  this is the end of WARM_SYNC_RESPONSE
 *				  sequence.
 * NOTIFY			: ACTIVE or STANDBY notify
 *
 **********************************************************************************************/

/***********************************************************************************\
*
*                     MBCSV API
*
*  PROCEDURE:             ncs_mbcsv_svc
*
*  DESCRIPTION:       This is the API for MBCSV clinet to send reuests to MBCA.
*                     MBCA will take action as per the request type specified.
*
*  REQUEST TYPES : 
*
*                     NCS_MBCSV_OP_INITIALIZE - put an MBCSv instance in start state
*                     NCS_MBCSV_OP_FINALIZE   - Recover MBCSv instance resources
*                     NCS_MBCSV_OP_SELECT     -  Get SELECT obj to coordinate Dispatch
*                     NCS_MBCSV_OP_DISPATCH   - Service any pending Callbacks
*                     NCS_MBCSV_OP_OPEN       -  Initiate contact with peer entity
*                     NCS_MBCSV_OP_CLOSE      - Dis-engage from peer entity relation
*                     NCS_MBCSV_OP_CHG_ROLE   - Change client's HA state-machine
*                     NCS_MBCSV_OP_SEND_CKPT  - Send a Checkpoint to my peer entity
*                     NCS_MBCSV_OP_SEND_NOTIFY - Send a free form msg to my peer entity
*                     NCS_MBCSV_OP_SUB_ONESHOT - Subscribe for ONE occurrence of event
*                     NCS_MBCSV_OP_SUB_PERSIST - Set up a persistent subscription
*                     NCS_MBCSV_OP_SUB_CANCEL  - cancel a subscription
*                     NCS_MBCSV_OP_OBJ_GET     - GET val scoped to client.
*                     NCS_MBCSV_OP_OBJ_SET     - SET val scoped to client.
*
*  RETURNS:           SUCCESS - All went well
*                     FAILURE - API fail.
*
*****************************************************************************/

uint32_t ncs_mbcsv_svc(NCS_MBCSV_ARG *arg)
{
	if (mbcsv_cb.created == false) {
		TRACE_4("MBCA instance is not created. First call mbcsv dl api.");
		return SA_AIS_ERR_NOT_EXIST;
	}

	if ((arg->i_op < NCS_MBCSV_OP_INITIALIZE) || (arg->i_op > NCS_MBCSV_OP_OBJ_SET)) {
 		TRACE_2("MBCA: Invalid Operation type is used.");
		return SA_AIS_ERR_INVALID_PARAM;
	}

	/* Process the request using operation type */
	return mbcsv_init_process_req_func[arg->i_op] (arg);
}

/**************************************************************************\
* PROCEDURE: mbcsv_process_initialize_request
*
* Purpose:  Process MBCSv initialize request. It adds new service in the
*           MBCSv registration list. Registers callback function provided 
*           by the MBCSv clinet. It also creates MBCSv service specufic
*           mailbox.
*
* Input:    arg -  is of type NCS_MBCSV_ARG.
*
* Returns:  NCSCC_RC_SUCCESSS/NCSCC_RC_FAILURE
*
* Notes:  
*
\**************************************************************************/
uint32_t mbcsv_process_initialize_request(NCS_MBCSV_ARG *arg)
{
	MBCSV_REG *new_reg;
	NCS_PATRICIA_PARAMS pt_params;
	SaAisErrorT rc = SA_AIS_OK;
	SS_SVC_ID svc_id = arg->info.initialize.i_service;
	TRACE_ENTER2("Register and obtain an MBCSV handle for svc_id: %u", svc_id);


	m_NCS_LOCK(&mbcsv_cb.global_lock, NCS_LOCK_WRITE);

	/* Check version, cannot be NULL */
	if (arg->info.initialize.i_version == 0) {
		TRACE_2("NULL version has been passed for service");
		rc = SA_AIS_ERR_INVALID_PARAM;
		goto err1;
	}

	/* 
	 * Walk through MBCSv reg list and find whether service is already registered
	 * with MBCSv. If yes then return error.
	 */
	if (NULL != (MBCSV_REG *)ncs_patricia_tree_get(&mbcsv_cb.reg_list, (const uint8_t *)&svc_id)) {
		TRACE_2("registration already exist for service");
		rc = SA_AIS_ERR_INVALID_PARAM;
		goto err1;
	}

	if (NULL == (new_reg = m_MMGR_ALLOC_REG_INFO)) {
		TRACE_4("malloc failed");
		rc = SA_AIS_ERR_NO_MEMORY;
		goto err1;
	}

	memset(new_reg, '\0', sizeof(MBCSV_REG));
	/* 
	 * Validate all the input parameters.
	 */
	if (NULL == (new_reg->mbcsv_cb_func = arg->info.initialize.i_mbcsv_cb)) {
		TRACE_2("Null callback function");
		rc = SA_AIS_ERR_INVALID_PARAM;
		goto err2;
	}

	/* Version and clinet handle information is stored as it is witout validating */
	new_reg->svc_id = svc_id;
	new_reg->pat_node.key_info = (uint8_t *)&new_reg->svc_id;
	new_reg->version = arg->info.initialize.i_version;

	/* 
	 * Create patricia tree for the checkpoint instance  Handle used for this tree
	 * is the PWE handle.
	 */
	pt_params.key_size = sizeof(uint32_t);

	if (ncs_patricia_tree_init(&new_reg->ckpt_ssn_list, &pt_params) != NCSCC_RC_SUCCESS) {
		TRACE_2("patricia tree init failed");
		rc = SA_AIS_ERR_FAILED_OPERATION;
		goto err2;
	}

	/*
	 * Now create service specific lock 
	 */
	m_NCS_LOCK_INIT(&new_reg->svc_lock);

	/* 
	 * Create Handle manager handle which will later be passed back to caller 
	 */
	new_reg->mbcsv_hdl = m_MBCSV_CREATE_HANDLE(new_reg);

	/* Initialize and create MBCSv Queue */
	if (NCSCC_RC_SUCCESS != mbcsv_client_queue_init(new_reg)) {
		TRACE_2("Failed to initialize client queue");
		rc = SA_AIS_ERR_FAILED_OPERATION;

		/* Destroy the initiated lock */
		m_NCS_LOCK_DESTROY(&new_reg->svc_lock);

		/* Pat tree destroy */
		ncs_patricia_tree_destroy(&new_reg->ckpt_ssn_list);

		m_MBCSV_DESTROY_HANDLE(new_reg->mbcsv_hdl);
		goto err2;
	}

	/*
	 * Add new registration in the registration list.
	 */
	if (NCSCC_RC_SUCCESS != ncs_patricia_tree_add(&mbcsv_cb.reg_list, (NCS_PATRICIA_NODE *)new_reg)) {
		TRACE_2("pat node add failed");
		rc = NCSCC_RC_FAILURE;
		m_MBCSV_DESTROY_HANDLE(new_reg->mbcsv_hdl);

		mbcsv_client_queue_destroy(new_reg);

		/* Destroy the initiated lock */
		m_NCS_LOCK_DESTROY(&new_reg->svc_lock);

		/* Pat tree destroy */
		ncs_patricia_tree_destroy(&new_reg->ckpt_ssn_list);
		goto err2;
	}

	/* 
	 * Passback the MBCSv handle which application will use for calling in
	 * next time onwards.
	 */
	arg->info.initialize.o_mbcsv_hdl = new_reg->mbcsv_hdl;

	goto noerr;

 err2:
	m_MMGR_FREE_REG_INFO(new_reg);
 err1:
 noerr:
	m_NCS_UNLOCK(&mbcsv_cb.global_lock, NCS_LOCK_WRITE);

	TRACE_LEAVE2("retval: %u", rc);
	return rc;

}

/**************************************************************************\
* PROCEDURE: mbcsv_process_finalize_request
*
* Purpose:  Process MBCSv finalize request. 
*           Finalize will first remove all the checkpoint instances of this  
*           service. It will cleanup all the sessions with the peers. It then
*           removes the service registration instance from the MBCSv registration 
*           list. Clear all the memory allocated for this service specific 
*           structure. Clears the mailbox of the service.
*
*
* Input:    arg -  is of type NCS_MBCSV_ARG.
*
* Returns:  NCSCC_RC_SUCCESSS/NCSCC_RC_FAILURE
*
* Notes:  
*
\**************************************************************************/
uint32_t mbcsv_process_finalize_request(NCS_MBCSV_ARG *arg)
{
	MBCSV_REG *mbc_reg;
	SaAisErrorT rc = SA_AIS_OK;
	TRACE_ENTER2("Unregistering with MBCSV");

	m_NCS_LOCK(&mbcsv_cb.global_lock, NCS_LOCK_WRITE);

	/* 
	 * Walk through MBCSv reg list and find whether service is registered
	 * with MBCSv. If not then return error.
	 */
	if (NULL == (mbc_reg = (MBCSV_REG *)m_MBCSV_TAKE_HANDLE(arg->i_mbcsv_hdl))) {
		TRACE_2("Bad handle received");
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto err1;
	}

	m_MBCSV_GIVE_HANDLE(arg->i_mbcsv_hdl);

	mbcsv_rmv_reg_inst((MBCSV_REG *)&mbcsv_cb.reg_list, mbc_reg);

 err1:
	m_NCS_UNLOCK(&mbcsv_cb.global_lock, NCS_LOCK_WRITE);
	TRACE_LEAVE2("retval: %u", rc);
	return rc;
}

/**************************************************************************\
* PROCEDURE: mbcsv_process_get_sel_obj_request
*
* Purpose:  Process MBCSv select request. 
*           This request returns the selection object associated with the 
*           service. 
*
*
* Input:    arg -  is of type NCS_MBCSV_ARG.
*
* Returns:  NCSCC_RC_SUCCESSS/NCSCC_RC_FAILURE
*
* Notes:  
*
\**************************************************************************/
uint32_t mbcsv_process_get_sel_obj_request(NCS_MBCSV_ARG *arg)
{
	MBCSV_REG *mbc_reg;
	TRACE_ENTER2("Obtain the MBCSV selection object");

	if (NULL == (mbc_reg = (MBCSV_REG *)m_MBCSV_TAKE_HANDLE(arg->i_mbcsv_hdl))) {
		TRACE_LEAVE2("Bad handle received");
		return SA_AIS_ERR_BAD_HANDLE;
	}

	m_NCS_LOCK(&mbc_reg->svc_lock, NCS_LOCK_READ);

	arg->info.sel_obj_get.o_select_obj =
	    (SaSelectionObjectT)m_GET_FD_FROM_SEL_OBJ(m_NCS_IPC_GET_SEL_OBJ(&mbc_reg->mbx));

	m_NCS_UNLOCK(&mbc_reg->svc_lock, NCS_LOCK_READ);

	m_MBCSV_GIVE_HANDLE(arg->i_mbcsv_hdl);

	TRACE_LEAVE2("retval: SA_AIS_OK");
	return SA_AIS_OK;
}

/**************************************************************************\
* PROCEDURE: mbcsv_process_dispatch_request
*
* Purpose:  Process MBCSv dispatch request. 
*           Dispatch processes outstanding request from the mailbox.
*
*
* Input:    arg -  is of type NCS_MBCSV_ARG.
*
* Returns:  NCSCC_RC_SUCCESSS/NCSCC_RC_FAILURE
*
* Notes:  
*
\**************************************************************************/
uint32_t mbcsv_process_dispatch_request(NCS_MBCSV_ARG *arg)
{
	MBCSV_REG *mbc_reg;
	SaAisErrorT rc = SA_AIS_OK;
	SYSF_MBX mail_box;
	TRACE_ENTER2("Dispatch MBCSV event");

	if (NULL == (mbc_reg = (MBCSV_REG *)m_MBCSV_TAKE_HANDLE(arg->i_mbcsv_hdl))) {
		TRACE_LEAVE2("Bad handle received");
		return SA_AIS_ERR_BAD_HANDLE;
	}

	mail_box = mbc_reg->mbx;

	m_MBCSV_GIVE_HANDLE(arg->i_mbcsv_hdl);

	switch (arg->info.dispatch.i_disp_flags) {
	case SA_DISPATCH_ONE:
		rc = mbcsv_hdl_dispatch_one(arg->i_mbcsv_hdl, mail_box);
		break;

	case SA_DISPATCH_ALL:
		rc = mbcsv_hdl_dispatch_all(arg->i_mbcsv_hdl, mail_box);
		break;

	case SA_DISPATCH_BLOCKING:
		rc = mbcsv_hdl_dispatch_block(arg->i_mbcsv_hdl, mail_box);
		break;

	default:
		rc = SA_AIS_ERR_INVALID_PARAM;
		break;
	}			/* switch */

	TRACE_LEAVE2("retval: %u", rc);
	return rc;
}

/**************************************************************************\
* PROCEDURE: mbcsv_process_open_request
*
* Purpose:  Process MBCSv open request. 
*           Open registers the checkpoint instance with the MBCSv. On registering
*           it will advertise the its presence to all its peers. 
*
*
* Input:    arg -  is of type NCS_MBCSV_ARG.
*
* Returns:  NCSCC_RC_SUCCESSS/NCSCC_RC_FAILURE
*
* Notes:  
*
\**************************************************************************/
uint32_t mbcsv_process_open_request(NCS_MBCSV_ARG *arg)
{
	MBCSV_REG *mbc_reg;
	CKPT_INST *new_ckpt;
	SaAisErrorT rc = SA_AIS_OK;
	uint32_t pwe_hdl = arg->info.open.i_pwe_hdl;
	SS_SVC_ID svc_id = 0;
	TRACE_ENTER2("Opening a checkpoint for pwe_hdl: %u", pwe_hdl);

	/*
	 * Find the service registration instance using the mbc_hdl passed by the 
	 * application.
	 */
	if (NULL == (mbc_reg = (MBCSV_REG *)m_MBCSV_TAKE_HANDLE(arg->i_mbcsv_hdl))) {
		TRACE_2("Bad handle received");
		return SA_AIS_ERR_BAD_HANDLE;
	}

	m_NCS_LOCK(&mbc_reg->svc_lock, NCS_LOCK_WRITE);
	TRACE("svc_id: %u", mbc_reg->svc_id);
	/*
	 * Search checkpoint list for the ckpt instance. If ckpt instance already exist
	 * then the parameter passed are invalid and so reject request.
	 */
	if (NULL != (CKPT_INST *)ncs_patricia_tree_get(&mbc_reg->ckpt_ssn_list, (const uint8_t *)&pwe_hdl)) {
		TRACE_2("CKPT resistration already exist");
		rc = SA_AIS_ERR_INVALID_PARAM;
		goto err1;
	}

	/*
	 * Allocate memory for new ckpt instance.
	 */
	if (NULL == (new_ckpt = m_MMGR_ALLOC_CKPT_INST)) {
		TRACE("malloc failed");
		rc = SA_AIS_ERR_NO_MEMORY;
		goto err1;
	}

	memset(new_ckpt, '\0', sizeof(CKPT_INST));

	new_ckpt->pwe_hdl = pwe_hdl;
	new_ckpt->pat_node.key_info = (uint8_t *)&new_ckpt->pwe_hdl;
	new_ckpt->active_peer = NULL;
	new_ckpt->peer_list = NULL;
	new_ckpt->fsm = (NCS_MBCSV_STATE_ACTION_FUNC_PTR *)ncsmbcsv_init_state_tbl;
	new_ckpt->my_role = MBCSV_HA_ROLE_INIT;
	new_ckpt->my_mbcsv_inst = mbc_reg;
	new_ckpt->peer_up_sent = false;
	new_ckpt->warm_sync_on = true;
	new_ckpt->warm_sync_time = NCS_MBCSV_TMR_SEND_WARM_SYNC_PERIOD;
	new_ckpt->client_hdl = arg->info.open.i_client_hdl;
	new_ckpt->role_set = false;
	new_ckpt->ftm_role_set = false;

	/*
	 * Check whether we need to register MBCSv on this pwe_hdl. If yes then 
	 * register MBCSv with MDS else query MDS for vdest and anchor values.
	 */
	if (0 == (SYSF_MBX)mbcsv_get_next_entry_for_pwe(pwe_hdl, &svc_id)) {
		if (NCSCC_RC_SUCCESS != mbcsv_mds_reg(pwe_hdl, 0, &new_ckpt->my_anchor, &new_ckpt->my_vdest)) {
			TRACE_2("MDS reg failed");
			rc = SA_AIS_ERR_FAILED_OPERATION;
			goto err2;
		}
	} else {
		if (NCSCC_RC_SUCCESS != mbcsv_query_mds(pwe_hdl, &new_ckpt->my_anchor, &new_ckpt->my_vdest)) {
			TRACE_2("MDS query failed");
			rc = SA_AIS_ERR_FAILED_OPERATION;
			goto err2;
		}
	}

	/* 
	 * Create Handle manager handle which will later be passed back to caller 
	 */
	new_ckpt->ckpt_hdl = m_MBCSV_CREATE_HANDLE(new_ckpt);

	/* 
	 * We are all set to add the ckpt instance to the registration.
	 */
	if (NCSCC_RC_SUCCESS != ncs_patricia_tree_add(&mbc_reg->ckpt_ssn_list, (NCS_PATRICIA_NODE *)new_ckpt)) {
		TRACE_2("pat node add failed");
		rc = SA_AIS_ERR_FAILED_OPERATION;

		m_MBCSV_DESTROY_HANDLE(new_ckpt->ckpt_hdl);
		goto err2;
	}

	/*
	 * Add this new entry into the MDS registration list.
	 */
	if (NCSCC_RC_SUCCESS != mbcsv_add_new_mbx(pwe_hdl, mbc_reg->svc_id, mbc_reg->mbx)) {
		TRACE_2("reg list add failed");
		rc = SA_AIS_ERR_FAILED_OPERATION;

		ncs_patricia_tree_del(&mbc_reg->ckpt_ssn_list, (NCS_PATRICIA_NODE *)new_ckpt);

		m_MBCSV_DESTROY_HANDLE(new_ckpt->ckpt_hdl);
		goto err2;
	}

	/* 
	 * So we are successful in open call. Return success and send back checkpoint
	 * handle to caller.
	 */
	arg->info.open.o_ckpt_hdl = new_ckpt->ckpt_hdl;

	goto no_err;

 err2:
	m_MMGR_FREE_CKPT_INST(new_ckpt);

 no_err:
 err1:
	/* Unlock and then return */
	m_NCS_UNLOCK(&mbc_reg->svc_lock, NCS_LOCK_WRITE);
	m_MBCSV_GIVE_HANDLE(arg->i_mbcsv_hdl);

	TRACE_LEAVE2("retval: %u", rc);
	return rc;

}

/**************************************************************************\
* PROCEDURE: mbcsv_process_close_request
*
* Purpose:  Process MBCSv close request. 
*           Close de-registers checkpoint instnace. It also tells all its 
*           peers that this checkpoint instance is no longer valid. MBCSv removes
*           the peer information of this checkpoint instance.
*
* Input:    arg -  is of type NCS_MBCSV_ARG.
*
* Returns:  NCSCC_RC_SUCCESSS/NCSCC_RC_FAILURE
*
* Notes:  
*
\**************************************************************************/
uint32_t mbcsv_process_close_request(NCS_MBCSV_ARG *arg)
{
	MBCSV_REG *mbc_reg;
	CKPT_INST *ckpt_inst;
	SaAisErrorT rc = SA_AIS_OK;
	TRACE_ENTER2("Closing the checkpoint");

	/*
	 * Find the service registration instance using the mbc_hdl passed by the 
	 * application.
	 */
	if (NULL == (mbc_reg = (MBCSV_REG *)m_MBCSV_TAKE_HANDLE(arg->i_mbcsv_hdl))) {
		TRACE_2("bad handle specified");
		rc = SA_AIS_ERR_BAD_HANDLE;
	}

	m_NCS_LOCK(&mbc_reg->svc_lock, NCS_LOCK_WRITE);

	/*
	 * Search for the ckpt instance in the list. If it does not exist then return 
	 * error since we don't have anything to close.
	 */
	if (NULL == (ckpt_inst = (CKPT_INST *)m_MBCSV_TAKE_HANDLE(arg->info.close.i_ckpt_hdl))) {
		TRACE_2("checkpoint registration does not exist");
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto err1;
	}

	TRACE("svc_id:%u, pwe_id: %u",  mbc_reg->svc_id, ckpt_inst->pwe_hdl);

	m_MBCSV_GIVE_HANDLE(arg->info.close.i_ckpt_hdl);

	if (NCSCC_RC_SUCCESS != mbcsv_remove_ckpt_inst(ckpt_inst)) {
		TRACE_2("failed to remove checkpoint instance");
		rc = SA_AIS_ERR_FAILED_OPERATION;
		goto err1;
	}

 err1:
	/* Unlock and then return */
	m_NCS_UNLOCK(&mbc_reg->svc_lock, NCS_LOCK_WRITE);
	m_MBCSV_GIVE_HANDLE(arg->i_mbcsv_hdl);

	TRACE_LEAVE2("retval: %u", rc);
	return rc;
}

/**************************************************************************\
* PROCEDURE: mbcsv_process_chg_role_request
*
* Purpose:  Process MBCSv change role request. 
*           Change role sets/changes the HA role of the checkpoint instance.
*           With change role, MBCSv will broadcast this new role to all the 
*           peers. It also changes the HA state machine as per the new role.
*
*
* Input:    arg -  is of type NCS_MBCSV_ARG.
*
* Returns:  NCSCC_RC_SUCCESSS/NCSCC_RC_FAILURE
*
* Notes:  
*
\**************************************************************************/
uint32_t mbcsv_process_chg_role_request(NCS_MBCSV_ARG *arg)
{
	MBCSV_REG *mbc_reg;
	CKPT_INST *ckpt_inst;
	SaAisErrorT rc = SA_AIS_OK;
	MBCSV_EVT *evt;
	TRACE_ENTER2("Change HA role for the checkpoint");

	/*
	 * Find the service registration instance using the mbc_hdl passed by the 
	 * application.
	 */
	if (NULL == (mbc_reg = (MBCSV_REG *)m_MBCSV_TAKE_HANDLE(arg->i_mbcsv_hdl))) {
		TRACE_2("bad handle specified");
		rc = SA_AIS_ERR_BAD_HANDLE;
	}

	m_NCS_LOCK(&mbc_reg->svc_lock, NCS_LOCK_READ);

	/*
	 * Search for the ckpt instance in the list. If it does not exist then return 
	 * error.
	 */
	if (NULL == (ckpt_inst = (CKPT_INST *)m_MBCSV_TAKE_HANDLE(arg->info.chg_role.i_ckpt_hdl))) {
		TRACE_2("checkpoint instance does not exist");
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto err1;
	}

	TRACE("svc_id:%u, pwe_hdl:%u", mbc_reg->svc_id, ckpt_inst->pwe_hdl);

	/*
	 * If we are setting this role for the first time then check if we 
	 * setting role to active or standby. If no then indicate the error.
	 */
	if ((ckpt_inst->ftm_role_set == false) &&
	    ((arg->info.chg_role.i_ha_state != SA_AMF_HA_ACTIVE) && (arg->info.chg_role.i_ha_state != SA_AMF_HA_STANDBY))) {
		TRACE_2("invalid role specified");
		rc = SA_AIS_ERR_INVALID_PARAM;
		goto err2;
	} else
		ckpt_inst->ftm_role_set = true;

	if (ckpt_inst->my_role == arg->info.chg_role.i_ha_state) {
		TRACE_2("same role specified");
		rc = SA_AIS_ERR_INVALID_PARAM;
		goto err2;
	}

	/*
	 * Validate the role change.
	 */
	if (((SA_AMF_HA_ACTIVE == ckpt_inst->my_role) &&
	     (SA_AMF_HA_STANDBY == arg->info.chg_role.i_ha_state)) ||
	    ((SA_AMF_HA_STANDBY == ckpt_inst->my_role) && (SA_AMF_HA_QUIESCED == arg->info.chg_role.i_ha_state))) {
		TRACE_2("Illegal role change");
		rc = SA_AIS_ERR_INVALID_PARAM;
		goto err2;
	}

	/* 
	 * Send change role event to the mailbox.
	 */
	if (NULL == (evt = m_MMGR_ALLOC_MBCSV_EVT)) {
		TRACE_2("malloc failed");
		rc = SA_AIS_ERR_NO_MEMORY;
		goto err2;
	}
	evt->msg_type = MBCSV_EVT_INTERNAL;
	evt->info.peer_msg.type = MBCSV_EVT_CHG_ROLE;
	evt->info.peer_msg.info.chg_role.ckpt_hdl = arg->info.chg_role.i_ckpt_hdl;
	evt->info.peer_msg.info.chg_role.new_role = arg->info.chg_role.i_ha_state;

	if (NCSCC_RC_SUCCESS != m_MBCSV_SND_MSG(&mbc_reg->mbx, evt, NCS_IPC_PRIORITY_NORMAL)) {
		m_MMGR_FREE_MBCSV_EVT(evt);
		TRACE_2("ipc send failed");
		rc = SA_AIS_ERR_FAILED_OPERATION;
		goto err2;
	}

 err2:
	m_MBCSV_GIVE_HANDLE(arg->info.chg_role.i_ckpt_hdl);
 err1:
	/* Unlock and then return */
	m_NCS_UNLOCK(&mbc_reg->svc_lock, NCS_LOCK_READ);
	m_MBCSV_GIVE_HANDLE(arg->i_mbcsv_hdl);

	TRACE_LEAVE2("retval: %u", rc);
	return rc;
}

/**************************************************************************\
* PROCEDURE: mbcsv_process_snd_ckpt_request
*
* Purpose:  Process MBCSv send checkpoint request. 
*           Send checkpoint data to all standby peers as per the send type.
*           User can send checkpoint data using following send types:
*           sync send : It holds callers thread till ack is received from all 
*                       standby peers.
*           async send : It holds callers thread till it send the message.
*           mbc async : Queue checkpoint data in the MBCSv queue.
*
*
* Input:    arg -  is of type NCS_MBCSV_ARG.
*
* Returns:  NCSCC_RC_SUCCESSS/NCSCC_RC_FAILURE
*
* Notes:  
*
\**************************************************************************/
uint32_t mbcsv_process_snd_ckpt_request(NCS_MBCSV_ARG *arg)
{
	MBCSV_REG *mbc_reg;
	CKPT_INST *ckpt_inst;
	SaAisErrorT rc = SA_AIS_OK;
	MBCSV_EVT *evt;
	bool check_peer = arg->info.send_ckpt.io_no_peer;
	TRACE_ENTER2("Sending checkpoint data to all STANDBY peers, as per the send-type specified");

	/*
	 * Find the service registration instance using the mbc_hdl passed by the 
	 * application.
	 */
	if (NULL == (mbc_reg = (MBCSV_REG *)m_MBCSV_TAKE_HANDLE(arg->i_mbcsv_hdl))) {
		TRACE_2("bad handle specified");
		rc = SA_AIS_ERR_BAD_HANDLE;
	}

	m_NCS_LOCK(&mbc_reg->svc_lock, NCS_LOCK_READ);

	if ((arg->info.send_ckpt.i_action < NCS_MBCSV_ACT_DONT_CARE) ||
	    (arg->info.send_ckpt.i_action >= NCS_MBCSV_ACT_MAX)) {
		TRACE_2("invalid request specified");
		rc = SA_AIS_ERR_INVALID_PARAM;
		goto err1;
	}

	/*
	 * Search for the ckpt instance in the list. If it does not exist then return 
	 * error.
	 */
	if (NULL == (ckpt_inst = (CKPT_INST *)m_MBCSV_TAKE_HANDLE(arg->info.send_ckpt.i_ckpt_hdl))) {
		TRACE_2("checkpoint instance doesnt exist");
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto err1;
	}

	TRACE("svc_id:%u, pwe_hdl:%u", mbc_reg->svc_id, ckpt_inst->pwe_hdl);

	/* Cannot ckpt update if STDBY peer doesn't exist, so just return */
	if (NULL == ckpt_inst->peer_list) {
		TRACE_1("No STANDBY peers found yet");
		goto err2;
	}

	arg->info.send_ckpt.io_no_peer = false;

	/*
	 * Depending on the send type send message to the peers.
	 */
	switch (arg->info.send_ckpt.i_send_type) {
	case NCS_MBCSV_SND_SYNC:
	case NCS_MBCSV_SND_USR_ASYNC:
		{
			mbcsv_send_ckpt_data_to_all_peers(&arg->info.send_ckpt, ckpt_inst, mbc_reg);
		}
		break;

	case NCS_MBCSV_SND_MBC_ASYNC:
		{
			TRACE("send type specified is NCS_MBCSV_SND_MBC_ASYNC: posting to mail box");
			/*
			 * Create the event message and post it to the mailbox.
			 */
			if (NULL == (evt = m_MMGR_ALLOC_MBCSV_EVT)) {
				TRACE_2("malloc failed");
				rc = SA_AIS_ERR_NO_MEMORY;
				goto err2;
			}

			evt->msg_type = MBCSV_EVT_INTERNAL;
			evt->info.peer_msg.type = MBCSV_EVT_MBC_ASYNC_SEND;
			memcpy(&evt->info.peer_msg.info.usr_msg_info,
			       &arg->info.send_ckpt, sizeof(NCS_MBCSV_SEND_CKPT));

			/* Indicates to check the peer existance while processing this event */
			evt->info.peer_msg.info.usr_msg_info.io_no_peer = check_peer;

			if (NCSCC_RC_SUCCESS != m_MBCSV_SND_MSG(&mbc_reg->mbx, evt, NCS_IPC_PRIORITY_NORMAL)) {
				TRACE_2("ipc send failed");
				rc = SA_AIS_ERR_FAILED_OPERATION;
				m_MMGR_FREE_MBCSV_EVT(evt);
				goto err2;
			}
		}
		break;

	default:
		{
			TRACE_2("invalid send type specified");
			rc = SA_AIS_ERR_INVALID_PARAM;
			break;
		}
	}

 err2:
	m_MBCSV_GIVE_HANDLE(arg->info.send_ckpt.i_ckpt_hdl);
 err1:
	/* Unlock and then return */
	m_NCS_UNLOCK(&mbc_reg->svc_lock, NCS_LOCK_READ);
	m_MBCSV_GIVE_HANDLE(arg->i_mbcsv_hdl);

	TRACE_LEAVE2("retval: %u", rc);
	return rc;
}

/**************************************************************************\
* PROCEDURE: mbcsv_process_snd_ntfy_request
*
* Purpose:  Process MBCSv send notify request. 
*           Send notify request to the destination asked by MBCSv client.
*
*
* Input:    arg -  is of type NCS_MBCSV_ARG.
*
* Returns:  NCSCC_RC_SUCCESSS/NCSCC_RC_FAILURE
*
* Notes:  
*
\**************************************************************************/
uint32_t mbcsv_process_snd_ntfy_request(NCS_MBCSV_ARG *arg)
{
	MBCSV_REG *mbc_reg;
	CKPT_INST *ckpt_inst;
	SaAisErrorT rc = SA_AIS_OK;
	TRACE_ENTER2("Sending notify");

	/*
	 * Find the service registration instance using the mbc_hdl passed by the 
	 * application.
	 */
	if (NULL == (mbc_reg = (MBCSV_REG *)m_MBCSV_TAKE_HANDLE(arg->i_mbcsv_hdl))) {
		TRACE_2("bad handle specified");
		return SA_AIS_ERR_BAD_HANDLE;
	}

	m_NCS_LOCK(&mbc_reg->svc_lock, NCS_LOCK_READ);

	/*
	 * Search for the ckpt instance in the list. If it does not exist then return 
	 * error.
	 */
	if (NULL == (ckpt_inst = (CKPT_INST *)m_MBCSV_TAKE_HANDLE(arg->info.send_notify.i_ckpt_hdl))) {
		TRACE_2("checkpoint instance does not exist");
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto err1;
	}

	TRACE("svc_id:%u, pwe_hdl:%u", mbc_reg->svc_id, ckpt_inst->pwe_hdl);

	if (NCSCC_RC_SUCCESS != mbcsv_send_notify_msg(arg->info.send_notify.i_msg_dest,
						      ckpt_inst, mbc_reg, arg->info.send_notify.i_msg)) {
		TRACE_2("notify send failed");
		rc = SA_AIS_ERR_FAILED_OPERATION;
	}

	m_MBCSV_GIVE_HANDLE(arg->info.send_notify.i_ckpt_hdl);

 err1:
	/* Unlock and then return */
	m_NCS_UNLOCK(&mbc_reg->svc_lock, NCS_LOCK_READ);
	m_MBCSV_GIVE_HANDLE(arg->i_mbcsv_hdl);

	TRACE_LEAVE2("retval: %u", rc);
	return rc;

}

/**************************************************************************\
* PROCEDURE: mbcsv_process_snd_data_req
*
* Purpose:  Process MBCSv send data request. 
*           Send data request to the active peer.
*
*
* Input:    arg -  is of type NCS_MBCSV_ARG.
*
* Returns:  NCSCC_RC_SUCCESSS/NCSCC_RC_FAILURE
*
* Notes:  
*
\**************************************************************************/
uint32_t mbcsv_process_snd_data_req(NCS_MBCSV_ARG *arg)
{
	MBCSV_REG *mbc_reg;
	CKPT_INST *ckpt_inst;
	SaAisErrorT rc = SA_AIS_OK;
	TRACE_ENTER2("Sending data request");

	/*
	 * Find the service registration instance using the mbc_hdl passed by the 
	 * application.
	 */
	if (NULL == (mbc_reg = (MBCSV_REG *)m_MBCSV_TAKE_HANDLE(arg->i_mbcsv_hdl))) {
		TRACE_2("bad handle specified");
		return SA_AIS_ERR_BAD_HANDLE;
	}

	m_NCS_LOCK(&mbc_reg->svc_lock, NCS_LOCK_READ);

	/*
	 * Search for the ckpt instance in the list. If it does not exist then return 
	 * error.
	 */
	if (NULL == (ckpt_inst = (CKPT_INST *)m_MBCSV_TAKE_HANDLE(arg->info.send_data_req.i_ckpt_hdl))) {
		TRACE_2("checkpoint instance does not exist");
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto err1;
	}

	/* 
	 * Check whether my role is STANDBY. Only then allow to send this 
	 * request. MBCSv client should not send this request in any other role.
	 */
	if ((SA_AMF_HA_STANDBY != ckpt_inst->my_role) && (SA_AMF_HA_QUIESCED != ckpt_inst->my_role)) {
		TRACE_2("HA state is nether STANDBY or QUIESCED");
		rc = SA_AIS_ERR_NOT_SUPPORTED;
		goto err2;
	}

	if (NULL == ckpt_inst->active_peer) {
		TRACE_2("Active peer does not exist");
		rc = SA_AIS_ERR_NOT_EXIST;
		goto err2;
	}

	if (true == ckpt_inst->data_req_sent) {
		TRACE_2("Data request already sent");
		rc = SA_AIS_ERR_TRY_AGAIN;

		goto err2;
	}

	TRACE("svc_id:%u, pwe_hdl:%u", mbc_reg->svc_id, ckpt_inst->pwe_hdl);

	if (NCSCC_RC_SUCCESS != mbcsv_send_data_req(&arg->info.send_data_req.i_uba, ckpt_inst, mbc_reg)) {
		TRACE_2("data request send failed");
		rc = SA_AIS_ERR_FAILED_OPERATION;
	}

 err2:
	m_MBCSV_GIVE_HANDLE(arg->info.send_data_req.i_ckpt_hdl);
 err1:
	/* Unlock and then return */
	m_NCS_UNLOCK(&mbc_reg->svc_lock, NCS_LOCK_READ);
	m_MBCSV_GIVE_HANDLE(arg->i_mbcsv_hdl);

	TRACE_LEAVE2("retval: %u", rc);
	return rc;

}

/**************************************************************************\
* PROCEDURE: mbcsv_process_get_request
*
* Purpose:  Process MBCSv get request. 
*           Get value of this MBCSv object.
*
*
* Input:    arg -  is of type NCS_MBCSV_ARG.
*
* Returns:  NCSCC_RC_SUCCESSS/NCSCC_RC_FAILURE
*
* Notes:  
*
\**************************************************************************/
uint32_t mbcsv_process_get_request(NCS_MBCSV_ARG *arg)
{
	MBCSV_REG *mbc_reg;
	CKPT_INST *ckpt_inst;
	SaAisErrorT rc = SA_AIS_OK;
	TRACE_ENTER2("Get configuration parameters for this checkpoint");

	/*
	 * Find the service registration instance using the mbc_hdl passed by the 
	 * application.
	 */
	if (NULL == (mbc_reg = (MBCSV_REG *)m_MBCSV_TAKE_HANDLE(arg->i_mbcsv_hdl))) {
		TRACE_2("bad handle specified");
		return SA_AIS_ERR_BAD_HANDLE;
	}

	m_NCS_LOCK(&mbc_reg->svc_lock, NCS_LOCK_READ);

	/*
	 * Search for the ckpt instance in the list. If it does not exist then return 
	 * error.
	 */
	if (NULL == (ckpt_inst = (CKPT_INST *)m_MBCSV_TAKE_HANDLE(arg->info.obj_get.i_ckpt_hdl))) {
		TRACE_2("checkpoint instance does not exist");
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto err1;
	}

	TRACE("svc_id:%u, pwe_hdl:%u", mbc_reg->svc_id, ckpt_inst->pwe_hdl);

	switch (arg->info.obj_get.i_obj) {
	case NCS_MBCSV_OBJ_WARM_SYNC_ON_OFF:
		arg->info.obj_get.o_val = ckpt_inst->warm_sync_on;
		break;

	case NCS_MBCSV_OBJ_TMR_WSYNC:
		arg->info.obj_get.o_val = ckpt_inst->warm_sync_time;
		break;

	default:
		TRACE_2("Incorrect option passed");
		rc = SA_AIS_ERR_INVALID_PARAM;
		break;
	}

	m_MBCSV_GIVE_HANDLE(arg->info.obj_get.i_ckpt_hdl);

 err1:
	/* Unlock and then return */
	m_NCS_UNLOCK(&mbc_reg->svc_lock, NCS_LOCK_READ);
	m_MBCSV_GIVE_HANDLE(arg->i_mbcsv_hdl);

	TRACE_LEAVE2("retval: %u", rc);
	return rc;
}

/**************************************************************************\
* PROCEDURE: mbcsv_process_set_request
*
* Purpose:  Process MBCSv set request. 
*           Set value of this MBCSv object.
*
*
* Input:    arg -  is of type NCS_MBCSV_ARG.
*
* Returns:  NCSCC_RC_SUCCESSS/NCSCC_RC_FAILURE
*
* Notes:  
*
\**************************************************************************/
uint32_t mbcsv_process_set_request(NCS_MBCSV_ARG *arg)
{
	MBCSV_REG *mbc_reg;
	CKPT_INST *ckpt_inst;
	SaAisErrorT rc = SA_AIS_OK;
	TRACE_ENTER2("Set mbcsv configuration parameters for this checkpoint");

	/*
	 * Find the service registration instance using the mbc_hdl passed by the 
	 * application.
	 */
	if (NULL == (mbc_reg = (MBCSV_REG *)m_MBCSV_TAKE_HANDLE(arg->i_mbcsv_hdl))) {
		TRACE_2("bad handle specified");
		return SA_AIS_ERR_BAD_HANDLE;
	}

	m_NCS_LOCK(&mbc_reg->svc_lock, NCS_LOCK_READ);

	/*
	 * Search for the ckpt instance in the list. If it does not exist then return 
	 * error.
	 */
	if (NULL == (ckpt_inst = (CKPT_INST *)m_MBCSV_TAKE_HANDLE(arg->info.obj_set.i_ckpt_hdl))) {
		TRACE_2("checkpoint instance does not exist");
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto err1;
	}

	TRACE("svc_id:%u, pwe_hdl:%u", mbc_reg->svc_id, ckpt_inst->pwe_hdl);

	switch (arg->info.obj_set.i_obj) {
	case NCS_MBCSV_OBJ_WARM_SYNC_ON_OFF:
		{
			PEER_INST *peer_ptr;

			/* check for the valid input. */
			if ((arg->info.obj_set.i_val != true) && (arg->info.obj_set.i_val != false)) {
				TRACE_2("Incorrect option passed, should be 0 or 1");
				rc = SA_AIS_ERR_INVALID_PARAM;
				goto err2;
			}

			if (ckpt_inst->warm_sync_on != arg->info.obj_set.i_val) {
				ckpt_inst->warm_sync_on = (bool)arg->info.obj_set.i_val;

				if (ckpt_inst->my_role == SA_AMF_HA_STANDBY) {
					for (peer_ptr = ckpt_inst->peer_list;
					     peer_ptr != NULL; peer_ptr = peer_ptr->next) {
						if ((ckpt_inst->warm_sync_on == true) &&
						    (peer_ptr->state == NCS_MBCSV_STBY_STATE_STEADY_IN_SYNC)) {
							ncs_mbcsv_start_timer(peer_ptr, NCS_MBCSV_TMR_SEND_WARM_SYNC);
							TRACE("turned on warmsync , started warm sync timer");
						} else {
							ncs_mbcsv_stop_timer(peer_ptr, NCS_MBCSV_TMR_SEND_WARM_SYNC);
							TRACE("stopped warm sync timer");
							if (!ckpt_inst->warm_sync_on)
								TRACE_3("revisit this check.");
							}
					}
				}
			}
		}
		break;

	case NCS_MBCSV_OBJ_TMR_WSYNC:
		{
			PEER_INST *peer_ptr;

			/*
			 * Range check for the configured timer value.
			 */
			if ((arg->info.obj_set.i_val < NCS_MBCSV_MIN_SEND_WARM_SYNC_TIME) ||
			    (arg->info.obj_set.i_val > NCS_MBCSV_MAX_SEND_WARM_SYNC_TIME)) {
				TRACE_2("Invalid timer value. Set bet 1000-360000");
				rc = SA_AIS_ERR_INVALID_PARAM;
				goto err2;
			}

			TRACE("changing the warmsync time period");

			if (ckpt_inst->warm_sync_time != arg->info.obj_set.i_val) {
				ckpt_inst->warm_sync_time = arg->info.obj_set.i_val;

				for (peer_ptr = ckpt_inst->peer_list; peer_ptr != NULL; peer_ptr = peer_ptr->next) {
					peer_ptr->tmr[NCS_MBCSV_TMR_SEND_WARM_SYNC].period = ckpt_inst->warm_sync_time;
				}
			}
		}
		break;

	default:
		TRACE_2("Invalid option specified");
		rc = SA_AIS_ERR_INVALID_PARAM;
		break;
	}

 err2:
	m_MBCSV_GIVE_HANDLE(arg->info.obj_set.i_ckpt_hdl);

 err1:
	/* Unlock and then return */
	m_NCS_UNLOCK(&mbc_reg->svc_lock, NCS_LOCK_READ);
	m_MBCSV_GIVE_HANDLE(arg->i_mbcsv_hdl);
	
	TRACE_LEAVE2("retval: %u", rc);
	return rc;
}
