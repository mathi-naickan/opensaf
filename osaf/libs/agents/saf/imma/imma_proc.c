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
 * Author(s): Ericsson AB
 *
 */

/*****************************************************************************
  DESCRIPTION:
  
  This file contains the IMMA processing routines callback
  processing routines etc.
*****************************************************************************/

#include "imma.h"
#include "immsv_api.h"
#include "ncssysf_mem.h"

#include <string.h>

/* For some reason I have to declare the strnlen function prototype.
   It does not help to include string.h */
size_t strnlen(const char *s, size_t maxlen);

static void imma_proc_ccbaug_setup(IMMA_CLIENT_NODE *cl_node, IMMA_CALLBACK_INFO *callback);

extern SaAisErrorT immsv_om_augment_ccb_get_result (SaImmOiHandleT privateOmHandle, SaUint32T ccbId) __attribute__((weak));
extern void  immsv_om_handle_finalize(SaImmHandleT privateOmHandle) __attribute__((weak));

static void imma_process_callback_info(IMMA_CB *cb, IMMA_CLIENT_NODE *cl_node, 
	IMMA_CALLBACK_INFO *callback, SaImmHandleT immHandle);

static void imma_proc_free_callback(IMMA_CALLBACK_INFO *callback);


/****************************************************************************
  Name          : imma_version_validate
 
  Description   : This routine Validates the received version
 
  Arguments     : SaVersionT *version - Version Info
 
  Return Values : SA_AIS_OK/SA_AIS<ERROR>
 
  Notes         : None
******************************************************************************/
uint32_t imma_version_validate(SaVersionT *version)
{
	SaAisErrorT retCode = SA_AIS_OK;

	if ((version->releaseCode == IMMA_RELEASE_CODE) && (version->majorVersion <= IMMA_MAJOR_VERSION)) {
		if ((version->releaseCode == 'A') && (version->majorVersion == 0x01)) {
			LOG_NO("ERR_VERSION: Version SAI-AIS-IMM-A.01.01 is not supported");
			retCode = SA_AIS_ERR_VERSION;
		} else if((version->releaseCode == 'A') && (version->majorVersion == 0x00)) {
			LOG_NO("ERR_VERSION: Version SAI-AIS-IMM-A.00.XX does not exist");
			retCode = SA_AIS_ERR_VERSION;
		}
	} else {
		TRACE_2("ERR_VERSION: IMMA - Version Incompatible %c %u not supported",
			version->releaseCode, version->majorVersion);
		retCode = SA_AIS_ERR_VERSION;
	}

	version->releaseCode = IMMA_RELEASE_CODE;
	version->majorVersion = IMMA_MAJOR_VERSION;
	version->minorVersion = IMMA_MINOR_VERSION;

	return retCode;
}

/****************************************************************************
  Name          : imma_callback_ipc_init
  
  Description   : This routine is used to initialize the queue for the callbacks.
 
  Arguments     : client_info - pointer to the client info
                   
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None
******************************************************************************/
uint32_t imma_callback_ipc_init(IMMA_CLIENT_NODE *client_info)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	if ((rc = m_NCS_IPC_CREATE(&client_info->callbk_mbx)) == NCSCC_RC_SUCCESS) {
		if (m_NCS_IPC_ATTACH(&client_info->callbk_mbx) == NCSCC_RC_SUCCESS) {
			return NCSCC_RC_SUCCESS;
		}
		m_NCS_IPC_RELEASE(&client_info->callbk_mbx, NULL);
		TRACE_3("Failed to initialize callback queue");
	}
	return rc;
}

/****************************************************************************
  Name          : imma_client_cleanup_mbx
  
  Description   : This routine is used to destroy the queue for the callbacks.
 
  Arguments     : cl_node - pointer to the client info
                   
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None
******************************************************************************/
static bool imma_client_cleanup_mbx(NCSCONTEXT arg, NCSCONTEXT msg)
{
	IMMA_CALLBACK_INFO *callback, *pnext;

	pnext = callback = (IMMA_CALLBACK_INFO *)msg;

	while (pnext) {
		pnext = callback->next;
		imma_proc_free_callback(callback);
		callback = pnext;
	}

	return true;
}

/****************************************************************************
  Name          : imma_callback_ipc_destroy
  
  Description   : This routine used to destroy the queue for the callbacks.
 
  Arguments     : client_info - pointer to the client info
                   
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None
******************************************************************************/
void imma_callback_ipc_destroy(IMMA_CLIENT_NODE *cl_node)
{
	TRACE_ENTER();
	/* detach the mail box */
	m_NCS_IPC_DETACH(&cl_node->callbk_mbx, imma_client_cleanup_mbx, cl_node);

	/* delete the mailbox */
	m_NCS_IPC_RELEASE(&cl_node->callbk_mbx, NULL);
}

/****************************************************************************
  Name          : imma_finalize_client
  
  Description   : This routine is used to process the finalize request at IMMA.
 
  Arguments     : cb - IMMA CB.
                  cl_node - pointer to the client info
                   
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None
******************************************************************************/
uint32_t imma_finalize_client(IMMA_CB *cb, IMMA_CLIENT_NODE *cl_node)
{
	SaImmAdminOwnerHandleT temp_hdl, *temp_ptr = NULL;
	IMMA_ADMIN_OWNER_NODE *adm_node = NULL;
	SaImmSearchHandleT search_tmp_hdl, *search_tmp_ptr = NULL;
	IMMA_SEARCH_NODE *search_node = NULL;

	/* Scan the entire Adm Owner DB and close the handles opened by client */
	while ((adm_node = (IMMA_ADMIN_OWNER_NODE *)
		ncs_patricia_tree_getnext(&cb->admin_owner_tree, (uint8_t *)temp_ptr))) {
		temp_hdl = adm_node->admin_owner_hdl;
		temp_ptr = &temp_hdl;

		if (adm_node->mImmHandle == cl_node->handle) {
			TRACE("Deleting admin owner node");
			imma_admin_owner_node_delete(cb, adm_node);
			temp_ptr = NULL;	/* Redo iteration from start after delete. */
		}
	}

	/* Ccb nodes are removed by imma_admin_owner_delete */

	/* Remove any search nodes opened by the client */
	while ((search_node = (IMMA_SEARCH_NODE *)
		ncs_patricia_tree_getnext(&cb->search_tree, (uint8_t *)search_tmp_ptr))) {
		search_tmp_hdl = search_node->search_hdl;
		search_tmp_ptr = &search_tmp_hdl;
		if (search_node->mImmHandle == cl_node->handle) {
			if(imma_search_node_delete(cb, search_node)!= NCSCC_RC_SUCCESS) {
				TRACE_4("ERROR imma_finalize_client could not delete search_node");
				break;
			}
			search_tmp_ptr = NULL;	/*Redo iteration from start after delete. */
		}
	}

	imma_callback_ipc_destroy(cl_node);

	TRACE("Deleting client node");
	osafassert(imma_client_node_delete(cb, cl_node) == NCSCC_RC_SUCCESS);

	return NCSCC_RC_SUCCESS;

}

/****************************************************************************
  Name          : entry point for SaImmOmAdminOperationInvokeAsyncCallbackT.
  Description   : This function will process the AdministartiveOperation
                  result up-call for the OM API.
  Arguments     : cb - IMMA CB.
                  evt - IMMA_EVT.
  Return Values : None
  Notes         : None
******************************************************************************/
static void imma_proc_admin_op_async_rsp(IMMA_CB *cb, IMMA_EVT *evt)
{
	TRACE_ENTER();
	IMMA_CALLBACK_INFO *callback;
	IMMA_CLIENT_NODE *cl_node = NULL;

	SaImmHandleT immHandleCont;
	SaInvocationT userInvoc;
	SaInt32T inv = m_IMMSV_UNPACK_HANDLE_LOW(evt->info.admOpRsp.invocation);

	/* get the CB Lock */
	if (m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
		TRACE_3("Lock failure");
		return;
	}

	/*NOTE: should get handle from immnd also and verify. */
	if (!imma_popAsyncAdmOpContinuation(cb, inv, &immHandleCont, &userInvoc)) {
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
		TRACE_3("Missmatch on continuation for SaImmOmAdminOperationInvokeCallbackT");
		return;
	}

	/* Get the Client info */
	imma_client_node_get(&cb->client_tree, &immHandleCont, &cl_node);
	if (!(cl_node && cl_node->isOm)) {
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
		TRACE_3("Failed to find client node");
		return;
	}

	imma_proc_decrement_pending_reply(cl_node);

	/* Allocate the Callback info */
	callback = calloc(1, sizeof(IMMA_CALLBACK_INFO));
	if (callback) {
		/* Fill the Call Back Info */
		callback->type = IMMA_CALLBACK_OM_ADMIN_OP_RSP;
		callback->lcl_imm_hdl = immHandleCont;

		TRACE_1("Creating callback for async admop inv:%llx rslt:%u err:%u",
		      userInvoc, evt->info.admOpRsp.result, evt->info.admOpRsp.error);

		callback->invocation = userInvoc;
		callback->sa_err = evt->info.admOpRsp.error;
		if (callback->sa_err == SA_AIS_OK) {
			callback->retval = evt->info.admOpRsp.result;
		} else {
			callback->retval = SA_AIS_ERR_NO_SECTIONS;	//Bogus result since error is set
		}

		if(evt->info.admOpRsp.parms) {
			callback->params = imma_proc_get_params(evt->info.admOpRsp.parms);
			evt->info.admOpRsp.parms = NULL;
		}

		/* Send the event */
		(void) m_NCS_IPC_SEND(&cl_node->callbk_mbx, callback, NCS_IPC_PRIORITY_NORMAL);
	}

	/* Release The Lock */
	m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);

	TRACE_LEAVE();
}

SaImmAdminOperationParamsT_2 **imma_proc_get_params(IMMSV_ADMIN_OPERATION_PARAM *in_params)
{
	int noOfParams = 0;
	IMMSV_ADMIN_OPERATION_PARAM *p = in_params;
	size_t paramDataSize = 0;
	SaImmAdminOperationParamsT_2 **out_params;
	int i = 0;

	while (p) {
		++noOfParams;
		p = p->next;
	}

	paramDataSize = sizeof(SaImmAdminOperationParamsT_2 *) * (noOfParams + 1);
	out_params = (SaImmAdminOperationParamsT_2 **)
	    calloc(1, paramDataSize);	/*alloc-1 */
	p = in_params;
	for (; i < noOfParams; i++) {
		IMMSV_ADMIN_OPERATION_PARAM *prev = p;
		out_params[i] = (SaImmAdminOperationParamsT_2 *)
		    malloc(sizeof(SaImmAdminOperationParamsT_2));	/*alloc-2 */
		out_params[i]->paramName = malloc(p->paramName.size + 1);	/*alloc-3 */
		strncpy(out_params[i]->paramName, p->paramName.buf, p->paramName.size + 1);
		out_params[i]->paramName[p->paramName.size] = 0;	/*string too long=>truncate */
		free(p->paramName.buf);
		p->paramName.buf = NULL;
		p->paramName.size = 0;
		out_params[i]->paramType = (SaImmValueTypeT)p->paramType;
		out_params[i]->paramBuffer = imma_copyAttrValue3(p->paramType,	/*alloc-4 */
								 &(p->paramBuffer));
		immsv_evt_free_att_val(&(p->paramBuffer), p->paramType);
		p = p->next;
		prev->next = NULL;
		free(prev);
	}
	return (SaImmAdminOperationParamsT_2 **)out_params;
}

/****************************************************************************
  Name          : imma_proc_admop
  Description   : This function will process the AdministartiveOperation
                  up-call for the Object Implementer API.
  Arguments     : cb - IMMA CB.
                  evt - IMMA_EVT.
  Return Values : None
  Notes         : None
******************************************************************************/
static void imma_proc_admop(IMMA_CB *cb, IMMA_EVT *evt)
{
	IMMA_CALLBACK_INFO *callback;
	IMMA_CLIENT_NODE *cl_node = NULL;
	SaBoolT isPbeAdmOp=(evt->type == IMMA_EVT_ND2A_IMM_PBE_ADMOP);

	/*TODO: correct this, ugly use of continuationId */
	SaImmOiHandleT implHandle = evt->info.admOpReq.continuationId;

	/* get the CB Lock */
	if (m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
		TRACE_3("Lock failure");
		return;
	}

	/* Get the Client info */
	imma_client_node_get(&cb->client_tree, &implHandle, &cl_node);
	if (!cl_node || cl_node->isOm) {
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
		TRACE_3("Failed to find client node");
		return;
	}

	if(isPbeAdmOp) {
		if(cl_node->isPbe) {
			TRACE_3("PBE-OI received PBE admin operation");
		} else {
			m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
			LOG_ER("Apparent PBE class create received at OI which is not PBE- ignoring");
			return;
		}
	}

	/* Allocate the Callback info */
	callback = calloc(1, sizeof(IMMA_CALLBACK_INFO));
	if (callback) {
		/* Fill the Call Back Info */
		if(isPbeAdmOp) {
			callback->type = IMMA_CALLBACK_PBE_ADMIN_OP;
		} else {
			callback->type = IMMA_CALLBACK_OM_ADMIN_OP;
		}
		callback->lcl_imm_hdl = implHandle;

		SaInvocationT saInv = m_IMMSV_PACK_HANDLE(evt->info.admOpReq.adminOwnerId,
							  evt->info.admOpReq.invocation);

		callback->invocation = saInv;

		callback->name.length = strnlen(evt->info.admOpReq.objectName.buf, evt->info.admOpReq.objectName.size);
		osafassert(callback->name.length <= SA_MAX_NAME_LENGTH);
		memcpy((char *)callback->name.value, evt->info.admOpReq.objectName.buf, callback->name.length);
		free(evt->info.admOpReq.objectName.buf);
		evt->info.admOpReq.objectName.buf = NULL;
		evt->info.admOpReq.objectName.size = 0;
		callback->operationId = evt->info.admOpReq.operationId;
		callback->params = imma_proc_get_params(evt->info.admOpReq.params);
		evt->info.admOpReq.params = NULL;
		/* Send the event */
		(void) m_NCS_IPC_SEND(&cl_node->callbk_mbx, callback, NCS_IPC_PRIORITY_NORMAL);
	}

	/* Release The Lock */
	m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
}

/****************************************************************************
  Name          : imma_determine_clients_to_resurrect
  Description   : Attempts to determine how many clients to attempt active
                  resurrection for. Active resurrection can only be done
                  when there is a selection object to generate an up-call on.

                  Re-active resurrection is also possible, but that is done
                  as a side effect when a stale handle is used in a blocking
                  IMM API call. 
                  
  Arguments     : cb - IMMA CB.
******************************************************************************/
void imma_determine_clients_to_resurrect(IMMA_CB *cb, bool* locked)
{
	/* We are LOCKED already, but we may unlock here => locked will be false*/
	IMMA_CLIENT_NODE  * clnode;
	SaImmHandleT *temp_ptr=0;
	SaImmHandleT temp_hdl=0;
	SaUint32T clientHigh=0;
	IMMSV_EVT clientHigh_evt;

	TRACE_ENTER();
	osafassert(locked && *locked);  /* We must be entering locked. */

	/* Determine clientHigh count and if there are any clients with
	   selection objects that can be resurrected */

	if (cb->dispatch_clients_to_resurrect) {
		/* 
		   Resurrections alredy in progress, possibly due to repeated
		   init/close of IMMA library (first/last handle).
		*/

		TRACE_3("Active resurrection of %u clients already ongoing",
			cb->dispatch_clients_to_resurrect);
		return;
	}

	while ((clnode = (IMMA_CLIENT_NODE *)
		       ncs_patricia_tree_getnext(&cb->client_tree, (uint8_t *)temp_ptr)))
	{
		temp_hdl = clnode->handle;
		temp_ptr = &temp_hdl;
		SaUint32T clientId = m_IMMSV_UNPACK_HANDLE_HIGH(clnode->handle);
		SaUint32T nodeId = m_IMMSV_UNPACK_HANDLE_LOW(clnode->handle);
		if (clientId > clientHigh) {
			clientHigh = clientId;
		}

		if (!clnode->stale) {
			TRACE_3("Found NON stale handle <%u, %x> when analyzing handles, "
				"bailing from attempt to actively resurrect.", 
				clientId, nodeId);

			/* This case means we must have gotten IMMND DOWN/UP at least 
			   twice in quick succession.
			*/

			cb->dispatch_clients_to_resurrect = 0; /* Reset. */
			goto done;
		}

		if (isExposed(cb, clnode)) {continue;}
		if (!clnode->selObjUsable) {continue;}
		++(cb->dispatch_clients_to_resurrect);
		/* Only clients with selection objects can be resurrected actively.
		   If selObjUsable is false then it means an attempt to actively 
		   resurrect has already started. 
		*/
	}

	if (clientHigh) 
	{
		/* Inform the IMMND of highest used client ID. */
		memset(&clientHigh_evt, 0, sizeof(IMMSV_EVT));
		clientHigh_evt.type = IMMSV_EVT_TYPE_IMMND;
		clientHigh_evt.info.immnd.type = (cb->sv_id == NCSMDS_SVC_ID_IMMA_OM)?
			IMMND_EVT_A2ND_IMM_OM_CLIENTHIGH:IMMND_EVT_A2ND_IMM_OI_CLIENTHIGH;
		clientHigh_evt.info.immnd.info.initReq.client_pid = clientHigh;
		TRACE_1("ClientHigh message high %u", clientHigh);
		/* Unlock before MDS Send */
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
		*locked = false;
		clnode = NULL;

		if (cb->is_immnd_up == false)
		{
			TRACE_3("IMMND is DOWN - clientHigh attempt failed. ");
			goto done;
		}

		/* send the clientHigh message to the IMMND asyncronously */
		if (imma_mds_msg_send(cb->imma_mds_hdl, &cb->immnd_mds_dest, 
			    &clientHigh_evt, NCSMDS_SVC_ID_IMMND) != NCSCC_RC_SUCCESS)
		{
			/* Failure to send clientHigh simply means the risk is higher that
			   resurrects will fail, exposing the handle as BAD to the client.
			*/
			TRACE_3("imma_determine_clients_to_resurrect: send failed");
		}
	}

 done:
	TRACE_LEAVE();
}

void imma_proc_terminate_oi_ccbs(IMMA_CB *cb, IMMA_CLIENT_NODE *cl_node)
{
	TRACE_ENTER();
	/* We are NOT LOCKED on entry */
	osafassert(m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) == NCSCC_RC_SUCCESS);

	struct imma_oi_ccb_record *oiCcb = cl_node->activeOiCcbs;

	while (oiCcb != NULL) {
		SaAisErrorT err = SA_AIS_ERR_TIMEOUT;
		struct imma_oi_ccb_record *nextOiCcb = oiCcb->next;
		if (!(oiCcb->isStale)) {
			oiCcb = nextOiCcb;
			continue; /* Already processed or CCB created after resurrect. */
		}

		oiCcb->isStale = false; /* Avoid ccb termination upcall again. */

		if (oiCcb->isCritical) {			
			SaImmHandleT handle = cl_node->handle;
			cl_node = NULL;

			SaImmOiCcbIdT ccbId =  oiCcb->ccbId;
			osafassert(ccbId < 0xffffffff);
			osafassert(m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE) == NCSCC_RC_SUCCESS);
			err = imma_proc_recover_ccb_result(cb, (SaUint32T) ccbId);
			osafassert(m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) == NCSCC_RC_SUCCESS);

			/* We have been unlocked. Look up the client_node & ccb-record again. 
			   New ccb records are pushed on to top of list. Tail should be stable. 
			 */
			imma_client_node_get(&cb->client_tree, &handle, &cl_node);
			if (!cl_node) {
				LOG_WA("Client node removed (handle closed) during termination processing");
				m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
				return;
			}

			if(!imma_oi_ccb_record_exists(cl_node, ccbId)) {
				LOG_WA("Ccb OI record lost while recovering ccb %llx result", ccbId);
				oiCcb =  cl_node->activeOiCcbs;
				continue;
			}
		} else {
			/* We expected non-critical stales to have been terminated by abort in 
			   imma_proc_stale_dispatch() */
			LOG_WA("Discovered non critical and stale oi_ccb_record %llx in "
				"imma_proc_terminate_oi_ccbs", oiCcb->ccbId);
			err = SA_AIS_ERR_FAILED_OPERATION;
		}

		IMMA_CALLBACK_INFO *callback = calloc(1, sizeof(IMMA_CALLBACK_INFO));
		osafassert(callback);
		if (err == SA_AIS_OK) {
			callback->type = IMMA_CALLBACK_OI_CCB_APPLY;
		} else if (err == SA_AIS_ERR_FAILED_OPERATION) {
			callback->type = IMMA_CALLBACK_OI_CCB_ABORT;
		} else {
			TRACE_3("WARNING: Failed to recover ccb outcome for critical oi ccb %llx err:%u",
				oiCcb->ccbId, err);
			free(callback);
			callback = NULL;
			osafassert(err == SA_AIS_ERR_TIMEOUT);
			continue;
		}

		callback->lcl_imm_hdl = cl_node->handle;
		callback->ccbID = (SaUint32T) oiCcb->ccbId;

		if (m_NCS_IPC_SEND(&cl_node->callbk_mbx, callback,
				NCS_IPC_PRIORITY_NORMAL) != NCSCC_RC_SUCCESS) {
			/* Cant make it high priority because it could bypass a normal
			   ccb-op upcall. That would confuse the OI! */
			TRACE_4("Failed to post ccb %llx stale-terminate ipc-message", oiCcb->ccbId);
		} else {TRACE_3("Posted ccb %llx stale-terminate ipc-message: %s", oiCcb->ccbId,
					(err == SA_AIS_OK)?"APPLY":"ABORT");}
		osafassert(oiCcb->isStale == false); 

		TRACE_3("imma_proc_terminate_oi_ccbs: oi_ccb_record for %llx terminated",
			oiCcb->ccbId);
		osafassert(imma_oi_ccb_record_terminate(cl_node, oiCcb->ccbId));
		oiCcb = nextOiCcb;
		callback = NULL;
	}

	m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
	TRACE_LEAVE();
}

/****************************************************************************
  Name          : imma_proc_stale_dispatch
  Description   : Dispatch a stale-handle callback for a particular client.

******************************************************************************/
void imma_proc_stale_dispatch(IMMA_CB *cb, IMMA_CLIENT_NODE *cl_node)
{
	TRACE_ENTER();
	/* We are LOCKED already */
	IMMA_CALLBACK_INFO *callback = NULL;
	if (cl_node->selObjUsable) {
		struct imma_oi_ccb_record *oiCcb = cl_node->activeOiCcbs;

		/* Send the stale handle triggering ipc-message */
		callback = calloc(1, sizeof(IMMA_CALLBACK_INFO));
		osafassert(callback);
		callback->type = IMMA_CALLBACK_STALE_HANDLE;
		callback->lcl_imm_hdl = 0LL;
		callback->ccbID = 0;

		if (m_NCS_IPC_SEND(&cl_node->callbk_mbx, callback,
				NCS_IPC_PRIORITY_HIGH) != NCSCC_RC_SUCCESS) {
			TRACE_4("Failed to post stale handle ipc-message");
		} else {TRACE_3("Posted stale handle ipc-message");} 

		/*Avoid redoing this dispatch for the same stale connection*/
		cl_node->selObjUsable=false; 
		/*If a resurrect succeds cl_node->selObjUsable will be set back to true*/

		/* Abort any active but non-critical OI CCBs */
		while (oiCcb != NULL) {
			struct imma_oi_ccb_record *nextOiCcb = oiCcb->next;
			if (!(oiCcb->isStale)) {
				TRACE_4("ERROR?: Discovered non stale oi_ccb_record %llx in stale dispatch",
					oiCcb->ccbId);
				oiCcb = nextOiCcb;
				continue;
			} 

			if (oiCcb->isCritical) {
				TRACE_3("Postponing termination upcall apply/abort for critical CCB %llx",
					oiCcb->ccbId);
				oiCcb = nextOiCcb;
				continue;
			} 

			/* Non critical & stale CCB must have been aborted by server side. 
			   Generate abort upcall immediately, i.e. no need to wait for resurrect. 
			 */
			callback = calloc(1, sizeof(IMMA_CALLBACK_INFO));
			osafassert(callback);
			callback->type = IMMA_CALLBACK_OI_CCB_ABORT;
			callback->lcl_imm_hdl = cl_node->handle;
			callback->ccbID = oiCcb->ccbId;
			if (m_NCS_IPC_SEND(&cl_node->callbk_mbx, callback,
					NCS_IPC_PRIORITY_NORMAL) != NCSCC_RC_SUCCESS) {
				/* Cant make it high priority because it could bypass a normal
				   ccb-op upcall. That would confuse the OI! */
				TRACE_4("Failed to post ccb stale abort ipc-message");
			} else {TRACE_3("Posted ccb %llx stale abort ipc-message", oiCcb->ccbId);}
			oiCcb->isStale = false; /* Avoid sending the abort message again. */

			TRACE_3("imma_proc_stale_dispatch: oi_ccb_record for %llx terminated",
				oiCcb->ccbId);
			osafassert(imma_oi_ccb_record_terminate(cl_node, oiCcb->ccbId));
			oiCcb = nextOiCcb;
			callback = NULL;
		}
    }
    TRACE_LEAVE();
}

SaAisErrorT imma_proc_recover_ccb_result(IMMA_CB *cb, SaUint32T ccbId)
{
	IMMSV_EVT evt;
	IMMSV_EVT *out_evt = NULL;
	uint32_t proc_rc = NCSCC_RC_SUCCESS;
	SaAisErrorT err = SA_AIS_ERR_TIMEOUT;
	unsigned int sleep_delay_ms = 500;
	unsigned int max_waiting_time_ms = 10 * 1000;	/* 10 secs */
	unsigned int msecs_waited = 0;

	TRACE_ENTER();
	/* We are NOT locked on entry. */
	memset(&evt, 0, sizeof(IMMSV_EVT));
	evt.type = IMMSV_EVT_TYPE_IMMND;
	evt.info.immnd.type = IMMND_EVT_A2ND_RECOVER_CCB_OUTCOME;
	evt.info.immnd.info.ccbId = ccbId;

	do {
		if(err == SA_AIS_ERR_TRY_AGAIN)  {
			usleep(sleep_delay_ms * 1000);
			msecs_waited += sleep_delay_ms;
			err = SA_AIS_ERR_TIMEOUT;
			proc_rc = NCSCC_RC_SUCCESS;
		}

		if (cb->is_immnd_up == false) {
			err = SA_AIS_ERR_TRY_AGAIN;
			continue;
		} 

		proc_rc = imma_mds_msg_sync_send(cb->imma_mds_hdl, &cb->immnd_mds_dest, &evt, 
			&out_evt, IMMSV_WAIT_TIME);

		if(proc_rc != NCSCC_RC_SUCCESS) {
			if(proc_rc != SA_AIS_ERR_TIMEOUT) {
				TRACE_4("ERR_TRY_AGAIN: Mds returned unexpected error code: %u", proc_rc);
			}
			err = SA_AIS_ERR_TRY_AGAIN;
		}

		if(!out_evt) {
			TRACE("No out_evt");
			err = SA_AIS_ERR_TRY_AGAIN;
		} else {
			err = out_evt->info.imma.info.errRsp.error;
			free(out_evt);
			out_evt = NULL;
			if((err != SA_AIS_OK) && 
				(err != SA_AIS_ERR_FAILED_OPERATION) &&
				(err != SA_AIS_ERR_TRY_AGAIN)) {
				/* We have a problem, abandon the effort.*/
				/* ERR_NO_RESOURCES is IMMND saying it cant find the ccb-id*/
				if(err != SA_AIS_ERR_NO_RESOURCES) {
					TRACE_4("ERR_TIMEOUT: Received unexpected error %u from IMMND, "
						"returning ERR_TIMEOUT", err);
				}
				err = SA_AIS_ERR_TIMEOUT;
			}
		}

		if(msecs_waited >= max_waiting_time_ms) {
			err = SA_AIS_ERR_TIMEOUT;
		}

	} while (err == SA_AIS_ERR_TRY_AGAIN);
	TRACE_5("imma_proc_recover_ccb_result returning err %u after waiting %u secs",
		err, msecs_waited/1000);

	TRACE_LEAVE();
	return err;
}
/****************************************************************************
  Name          : imma_proc_rt_attr_update
  Description   : This function will generate the SaImmOiRtAttrUpdateCallbackT
                  up-call for the Object Implementer API.
  Arguments     : cb - IMMA CB.
                  evt - IMMA_EVT.
  Return Values : None
  Notes         : None
******************************************************************************/
static void imma_proc_rt_attr_update(IMMA_CB *cb, IMMA_EVT *evt)
{
	IMMA_CALLBACK_INFO *callback;
	IMMA_CLIENT_NODE *cl_node = NULL;

	/* NOTE: correct this, ugly use of continuationId */
	SaImmOiHandleT implHandle = evt->info.searchRemote.client_hdl;
	TRACE_ENTER();

	/* get the CB Lock */
	if (m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
		TRACE_3("Lock failure");
		TRACE_LEAVE();
		return;
	}

	/* Get the Client info */
	imma_client_node_get(&cb->client_tree, &implHandle, &cl_node);
	if (!cl_node || cl_node->isOm) {
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
		TRACE_3("Failed to find client node");
		TRACE_LEAVE();
		return;
	}

	/* Allocate the Callback info */
	callback = calloc(1, sizeof(IMMA_CALLBACK_INFO));
	if (callback) {
		/* Fill the Call Back Info */
		callback->type = IMMA_CALLBACK_OI_RT_ATTR_UPDATE;
		callback->lcl_imm_hdl = implHandle;

		callback->name.length = strnlen(evt->info.searchRemote.objectName.buf,
						evt->info.searchRemote.objectName.size);
		osafassert(callback->name.length <= SA_MAX_NAME_LENGTH);
		memcpy((char *)callback->name.value, evt->info.searchRemote.objectName.buf, callback->name.length);
		free(evt->info.searchRemote.objectName.buf);
		evt->info.searchRemote.objectName.buf = NULL;
		evt->info.searchRemote.objectName.size = 0;

		/* steal the attributeNames list. */
		callback->attrNames = evt->info.searchRemote.attributeNames;
		evt->info.searchRemote.attributeNames = NULL;

		SaInvocationT saInv = m_IMMSV_PACK_HANDLE(evt->info.searchRemote.remoteNodeId,
							  evt->info.searchRemote.searchId);
		callback->invocation = saInv;

		callback->requestNodeId = evt->info.searchRemote.requestNodeId;

		/* Send the event */
		TRACE("Posting RT UPDATE CALLBACK to IPC mailbox");
		(void) m_NCS_IPC_SEND(&cl_node->callbk_mbx, callback, NCS_IPC_PRIORITY_NORMAL);
	}

	/* Release The Lock */
	m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
	TRACE_LEAVE();
}

/****************************************************************************
  Name          : imma_proc_ccb_completed
  Description   : This function will process the ccb completed
                  up-call for the Object Implementer API.
  Arguments     : cb - IMMA CB.
                  evt - IMMA_EVT.
  Return Values : None
  Notes         : None
******************************************************************************/
static void imma_proc_ccb_completed(IMMA_CB *cb, IMMA_EVT *evt)
{
	IMMA_CALLBACK_INFO *callback;
	IMMA_CLIENT_NODE *cl_node = NULL;
	SaBoolT isPrtObj=(evt->info.ccbCompl.ccbId == 0);

	SaImmOiHandleT implHandle = evt->info.ccbCompl.immHandle;

	/* get the CB Lock */
	if (m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
		TRACE_3("Lock failure");
		return;
	}

	/* Get the Client info */
	imma_client_node_get(&cb->client_tree, &implHandle, &cl_node);
	if (!cl_node || cl_node->isOm) {
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
		TRACE_3("Could not find client node");
		return;
	}

	if(isPrtObj) {
		if(cl_node->isPbe) {
			TRACE_3("PBE-OI received runtime object deletes completed");
		} else {
			m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
			LOG_ER("Apparent runtime object deletes completed received at OI "
				"which is not PBE - ignoring");
			return;
		}
	}

	if(cl_node->isPbe && !isPrtObj && (evt->info.ccbCompl.invocation==0)) {
		if(imma_oi_ccb_record_exists(cl_node, evt->info.ccbCompl.ccbId)) {
			LOG_WA("Redundant & anonymous CCB-completed UC for %u => ignore",
				evt->info.ccbCompl.ccbId);
			m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
			return;
		} else {
			LOG_WA("Re-inserting Ccb record %u in ccb-completed upcall => PBE recovery,",
				evt->info.ccbCompl.ccbId);
			imma_oi_ccb_record_add(cl_node, evt->info.ccbCompl.ccbId, 1/* Hack! Op was initialized to 1*/);
		}
	} 

	/* Allocate the Callback info */
	callback = calloc(1, sizeof(IMMA_CALLBACK_INFO));
	if (callback) {
		/* Fill the Call Back Info */
		if(isPrtObj) {
			callback->type = IMMA_CALLBACK_PBE_PRTO_DELETES_COMPLETED;
		} else {
			callback->type = IMMA_CALLBACK_OI_CCB_COMPLETED;
		}
		callback->lcl_imm_hdl = implHandle;
		callback->ccbID = evt->info.ccbCompl.ccbId;
		callback->implId = evt->info.ccbCompl.implId;
		callback->inv = evt->info.ccbCompl.invocation;

		/* Send the event */
		(void) m_NCS_IPC_SEND(&cl_node->callbk_mbx, callback, NCS_IPC_PRIORITY_NORMAL);
		TRACE("Posted IMMA_CALLBACK_OI_CCB_COMPLETED");
	}

	m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
}

/****************************************************************************
  Name          : imma_proc_ccb_apply
  Description   : This function will process the ccb apply
                  up-call for the Object Implementer API.
  Arguments     : cb - IMMA CB.
                  evt - IMMA_EVT.
  Return Values : None
  Notes         : None
******************************************************************************/
static void imma_proc_ccb_apply(IMMA_CB *cb, IMMA_EVT *evt)
{
	IMMA_CALLBACK_INFO *callback;
	IMMA_CLIENT_NODE *cl_node = NULL;

	SaImmOiHandleT implHandle = evt->info.ccbCompl.immHandle;

	if (m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
		TRACE_3("Lock failure");
		return;
	}

	imma_client_node_get(&cb->client_tree, &implHandle, &cl_node);
	if (!cl_node || cl_node->isOm) {
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
		TRACE_3("Could not find client node");
		return;
	}

	callback = calloc(1, sizeof(IMMA_CALLBACK_INFO));
	if (callback) {
		callback->type = IMMA_CALLBACK_OI_CCB_APPLY;
		callback->lcl_imm_hdl = implHandle;
		callback->ccbID = evt->info.ccbCompl.ccbId;
		/*callback->implId = evt->info.ccbCompl.implId;*/
		/*callback->inv = evt->info.ccbCompl.invocation;*/

		(void)m_NCS_IPC_SEND(&cl_node->callbk_mbx, callback, NCS_IPC_PRIORITY_NORMAL);
		TRACE("Posted IMMA_CALLBACK_OI_CCB_APPLY");
	}

	m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
}

/****************************************************************************
  Name          : imma_proc_ccb_abort
  Description   : This function will process the ccb abort
                  up-call for the Object Implementer API.
  Arguments     : cb - IMMA CB.
                  evt - IMMA_EVT.
  Return Values : None
  Notes         : None
******************************************************************************/
static void imma_proc_ccb_abort(IMMA_CB *cb, IMMA_EVT *evt)
{
	IMMA_CALLBACK_INFO *callback;
	IMMA_CLIENT_NODE *cl_node = NULL;

	SaImmOiHandleT implHandle = evt->info.ccbCompl.immHandle;

	if (m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
		TRACE_3("Lock failure");
		return;
	}

	imma_client_node_get(&cb->client_tree, &implHandle, &cl_node);
	if (!cl_node || cl_node->isOm) {
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
		TRACE_3("Could not find client node");
		return;
	}

	callback = calloc(1, sizeof(IMMA_CALLBACK_INFO));
	if (callback) {
		callback->type = IMMA_CALLBACK_OI_CCB_ABORT;
		callback->lcl_imm_hdl = implHandle;
		callback->ccbID = evt->info.ccbCompl.ccbId;
		/*callback->implId = evt->info.ccbCompl.implId;*/

		(void)m_NCS_IPC_SEND(&cl_node->callbk_mbx, callback, NCS_IPC_PRIORITY_NORMAL);
		TRACE("Posted IMMA_CALLBACK_OI_CCB_ABORT for ccb %u", evt->info.ccbCompl.ccbId);
		if (imma_oi_ccb_record_abort(cl_node, evt->info.ccbCompl.ccbId)) {
			TRACE_2("CCB-ABORT-UC: oi_ccb_record for %u aborted", 
				evt->info.ccbCompl.ccbId);
		} else {
			TRACE_4("ERROR: CCB-ABORT-UC - CCB record for ccb %u non existent", 
				evt->info.ccbCompl.ccbId);
		}
	}

	m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
}

/****************************************************************************
  Name          : imma_proc_obj_delete
  Description   : This function will process the Object Delete
                  up-call for the Object Implementer API.
  Arguments     : cb - IMMA CB.
                  evt - IMMA_EVT.
  Return Values : None
******************************************************************************/
static void imma_proc_obj_delete(IMMA_CB *cb, IMMA_EVT *evt)
{
	IMMA_CALLBACK_INFO *callback;
	IMMA_CLIENT_NODE *cl_node = NULL;
	SaBoolT isPrtObj=(evt->info.objDelete.ccbId == 0);

	SaImmOiHandleT implHandle = evt->info.objDelete.immHandle;

	if (m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
		TRACE_3("Lock failure");
		return;
	}

	imma_client_node_get(&cb->client_tree, &implHandle, &cl_node);
	if (!cl_node || cl_node->isOm || cl_node->exposed) {
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
		TRACE_3("Could not find valid client node");
		return;
	}

	if(isPrtObj) {
		if(cl_node->isPbe) {
			TRACE_3("PBE-OI received runtime object delete");
		} else {
			m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
			LOG_ER("Apparent runtime object delete received at OI which is not PBE - ignoring");
			return;
		}
	}

	callback = calloc(1, sizeof(IMMA_CALLBACK_INFO));
	if (callback) {
		if(isPrtObj) {
			callback->type = IMMA_CALLBACK_PBE_PRT_OBJ_DELETE;
		} else {
			callback->type = IMMA_CALLBACK_OI_CCB_DELETE;
		}
		callback->lcl_imm_hdl = implHandle;
		callback->ccbID = evt->info.objDelete.ccbId;
		callback->inv = evt->info.objDelete.adminOwnerId;	/*ugly */
		callback->name.length = strnlen(evt->info.objDelete.objectName.buf,
						evt->info.objDelete.objectName.size);
		osafassert(callback->name.length <= SA_MAX_NAME_LENGTH);
		memcpy((char *)callback->name.value, evt->info.objDelete.objectName.buf, callback->name.length);
		free(evt->info.objDelete.objectName.buf);
		evt->info.objDelete.objectName.buf = NULL;
		evt->info.objDelete.objectName.size = 0;

		/* Send the event */
		(void)m_NCS_IPC_SEND(&cl_node->callbk_mbx, callback, NCS_IPC_PRIORITY_NORMAL);
		TRACE("Posted IMMA_CALLBACK_OI_CCB_DELETE for ccb %u", evt->info.objDelete.ccbId);
		if(isPrtObj) {
			/* PRTO delete arrives at PBE. Count how many in same subtree #1809.
			   Sum verified in completed upcall.
			   This to overcome silent message loss in MDS local transport, see #1795.
			 */
			SaImmOiCcbIdT ccbId = 0LL;
			ccbId = callback->inv + 0x100000000LL;  /* pseudo ccb-id */

			imma_oi_ccb_record_add(cl_node, ccbId, 0);			
		} else {
			/* Regular CCB object delete arrives at... */
			if(cl_node->isApplier) { /* applier*/
				osafassert(callback->inv == 0);
			} else if(cl_node->isPbe) { /* PBE. */
				TRACE("PBe case inv:%u", callback->inv);
				if((callback->inv != 0) && 
					(strcmp((char *)callback->name.value, OPENSAF_IMM_OBJECT_DN))) {
					/* callback->inv must be zero, except for operations on
					 OPENSAF_IMM_OBJECT_DN  */
					LOG_ER("PBE: callback->inv != 0, LINE:%u", __LINE__);
					abort();
				}
			} else {/*regular OI. */
				osafassert(callback->inv);
			}
			imma_oi_ccb_record_add(cl_node, evt->info.objDelete.ccbId, callback->inv);
		}
	}

	/* Release The Lock */
	m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
}

/****************************************************************************
  Name          : imma_proc_obj_create
  Description   : This function will process the Object Create
                  up-call for the Object Implementer API.
  Arguments     : cb - IMMA CB.
                  evt - IMMA_EVT.
  Return Values : None
******************************************************************************/
static void imma_proc_obj_create(IMMA_CB *cb, IMMA_EVT *evt)
{
	IMMA_CALLBACK_INFO *callback;
	IMMA_CLIENT_NODE *cl_node = NULL;
	SaBoolT isPrtObj=(evt->info.objCreate.ccbId == 0);

	SaImmOiHandleT implHandle = evt->info.objCreate.immHandle;

	/* get the CB Lock */
	if (m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
		TRACE_3("Lock failure");
		return;
	}

	/* Get the Client info */
	imma_client_node_get(&cb->client_tree, &implHandle, &cl_node);
	if (!cl_node || cl_node->isOm) {
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
		TRACE_3("Could not find client node");
		return;
	}

	if(isPrtObj) {
		if(cl_node->isPbe) {
			TRACE_3("PBE-OI received runtime object create");
		} else {
			m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
			LOG_ER("Apparent runtime object create received at OI which is not PBE - ignoring");
			return;
		}
	}

	/* Allocate the Callback info */
	callback = calloc(1, sizeof(IMMA_CALLBACK_INFO));
	if (callback) {
		/* Fill the Call Back Info */
		if(isPrtObj) {
			callback->type = IMMA_CALLBACK_PBE_PRT_OBJ_CREATE;
		} else {
			callback->type = IMMA_CALLBACK_OI_CCB_CREATE;
		}
		callback->lcl_imm_hdl = implHandle;
		callback->ccbID = evt->info.objCreate.ccbId;
		callback->inv = evt->info.objCreate.adminOwnerId;	/*Actually continuationId */

		callback->name.length = strnlen(evt->info.objCreate.parentName.buf,
						evt->info.objCreate.parentName.size);
		osafassert(callback->name.length <= SA_MAX_NAME_LENGTH);
		memcpy((char *)callback->name.value, evt->info.objCreate.parentName.buf, callback->name.length);
		free(evt->info.objCreate.parentName.buf);
		evt->info.objCreate.parentName.buf = NULL;
		evt->info.objCreate.parentName.size = 0;

		osafassert(strlen(evt->info.objCreate.className.buf) <= evt->info.objCreate.className.size);
		callback->className = evt->info.objCreate.className.buf;
		evt->info.objCreate.className.buf = NULL;	/*steal the string buffer */
		evt->info.objCreate.className.size = 0;

		callback->attrValues = evt->info.objCreate.attrValues;
		evt->info.objCreate.attrValues = NULL;	/*steal attrValues list */

		/* Send the event */
		(void)m_NCS_IPC_SEND(&cl_node->callbk_mbx, callback, NCS_IPC_PRIORITY_NORMAL);
		TRACE("Posted IMMA_CALLBACK_OI_CCB_CREATE for ccb %u", evt->info.objCreate.ccbId);
		if(!isPrtObj) {
			imma_oi_ccb_record_add(cl_node, evt->info.objCreate.ccbId, callback->inv);
		}
	}

	/* Release The Lock */
	m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
}

/****************************************************************************
  Name          : imma_proc_obj_modify
  Description   : This function will process the Object Modify
                  up-call for the Object Implementer API.
  Arguments     : cb - IMMA CB.
                  evt - IMMA_EVT.
  Return Values : None
******************************************************************************/
static void imma_proc_obj_modify(IMMA_CB *cb, IMMA_EVT *evt)
{
	IMMA_CALLBACK_INFO *callback;
	IMMA_CLIENT_NODE *cl_node = NULL;
	SaBoolT isPrtAttrs=(evt->info.objCreate.ccbId == 0);
	/* Can be a PRTO or a config obj with PRTAttrs. */
	TRACE_ENTER();

	SaImmOiHandleT implHandle = evt->info.objModify.immHandle;

	/* get the CB Lock */
	if (m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
		TRACE_3("Lock failure");
		TRACE_LEAVE();
		return;
	}

	/* Get the Client info */
	imma_client_node_get(&cb->client_tree, &implHandle, &cl_node);
	if (!cl_node || cl_node->isOm) {
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
		TRACE_3("Could not find client node");
		TRACE_LEAVE();
		return;
	}

	if(isPrtAttrs) {
		if(cl_node->isPbe) {
			TRACE_3("PBE-OI received runtime attributes update");
		} else {
			m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
			LOG_ER("Apparent runtime attributes update received at OI which is not PBE - ignoring");
			return;
		}
	}


	/* Allocate the Callback info */
	callback = calloc(1, sizeof(IMMA_CALLBACK_INFO));
	if (callback) {
		/* Fill the Call Back Info */
		if(isPrtAttrs) {
			callback->type = IMMA_CALLBACK_PBE_PRT_ATTR_UPDATE;
		} else {
			callback->type = IMMA_CALLBACK_OI_CCB_MODIFY;
		}
		callback->lcl_imm_hdl = implHandle;
		callback->ccbID = evt->info.objModify.ccbId;
		callback->inv = evt->info.objModify.adminOwnerId;
		/*Actually continuationId */

		callback->name.length = strnlen(evt->info.objModify.objectName.buf,
						evt->info.objModify.objectName.size);
		osafassert(callback->name.length <= SA_MAX_NAME_LENGTH);
		memcpy((char *)callback->name.value, evt->info.objModify.objectName.buf, callback->name.length);
		free(evt->info.objModify.objectName.buf);
		evt->info.objModify.objectName.buf = NULL;
		evt->info.objModify.objectName.size = 0;

		callback->attrMods = evt->info.objModify.attrMods;
		evt->info.objModify.attrMods = NULL;	/*steal attrMods list */

		/* Send the event */
		(void)m_NCS_IPC_SEND(&cl_node->callbk_mbx, callback, NCS_IPC_PRIORITY_NORMAL);
		TRACE("IMMA_CALLBACK_OI_CCB_MODIFY Posted for ccb %u", evt->info.objModify.ccbId);
		if(!isPrtAttrs) {
			imma_oi_ccb_record_add(cl_node, evt->info.objModify.ccbId, callback->inv);
		}
	}

	/* Release The Lock */
	m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
	TRACE_LEAVE();
}

void imma_proc_free_pointers(IMMA_CB *cb, IMMA_EVT *evt)
{
	TRACE_ENTER();
	switch (evt->type) {
		case IMMA_EVT_ND2A_IMM_ADMOP:
		case IMMA_EVT_ND2A_IMM_PBE_ADMOP:
			/*TODO See TODO 12345 code repeated (almost) in imma_om_api.c
			  free-1 */
			if (evt->info.admOpReq.objectName.size) {
				free(evt->info.admOpReq.objectName.buf);
				evt->info.admOpReq.objectName.buf = NULL;
				evt->info.admOpReq.objectName.size = 0;
			}
			while (evt->info.admOpReq.params) {
				IMMSV_ADMIN_OPERATION_PARAM *p = evt->info.admOpReq.params;
				evt->info.admOpReq.params = p->next;

				if (p->paramName.buf) {	/*free-3 */
					free(p->paramName.buf);
					p->paramName.buf = NULL;
					p->paramName.size = 0;
				}
				immsv_evt_free_att_val(&(p->paramBuffer), p->paramType);/*free-4 */
				p->next = NULL;
				free(p);  /*free-2 */
			}
			break;

		case IMMA_EVT_ND2A_ADMOP_RSP_2:
			while (evt->info.admOpRsp.parms) {
				IMMSV_ADMIN_OPERATION_PARAM *p = evt->info.admOpRsp.parms;
				evt->info.admOpRsp.parms = p->next;

				if (p->paramName.buf) {	/*free-3 */
					free(p->paramName.buf);
					p->paramName.buf = NULL;
					p->paramName.size = 0;
				}
				immsv_evt_free_att_val(&(p->paramBuffer), p->paramType);/*free-4 */
				p->next = NULL;
				free(p);  /*free-2 */
			}
			break;			

		case IMMA_EVT_ND2A_ADMOP_RSP:
			break;

		case IMMA_EVT_ND2A_SEARCH_REMOTE:
			free(evt->info.searchRemote.objectName.buf);
			evt->info.searchRemote.objectName.buf = NULL;
			evt->info.searchRemote.objectName.size = 0;
			immsv_evt_free_attrNames(evt->info.searchRemote.attributeNames);
			evt->info.searchRemote.attributeNames = NULL;
			break;

		case IMMA_EVT_ND2A_OI_OBJ_CREATE_UC:
			free(evt->info.objCreate.className.buf);
			evt->info.objCreate.className.buf = NULL;
			evt->info.objCreate.className.size = 0;

			free(evt->info.objCreate.parentName.buf);
			evt->info.objCreate.parentName.buf = NULL;
			evt->info.objCreate.parentName.size = 0;

			immsv_free_attrvalues_list(evt->info.objCreate.attrValues);
			evt->info.objCreate.attrValues = NULL;
			break;

		case IMMA_EVT_ND2A_OI_OBJ_DELETE_UC:
			free(evt->info.objDelete.objectName.buf);
			evt->info.objDelete.objectName.buf = NULL;
			evt->info.objDelete.objectName.size = 0;
			break;

		case IMMA_EVT_ND2A_OI_OBJ_MODIFY_UC:
			free(evt->info.objModify.objectName.buf);
			evt->info.objModify.objectName.buf = NULL;
			evt->info.objModify.objectName.size = 0;

			immsv_free_attrmods(evt->info.objModify.attrMods);
			evt->info.objModify.attrMods = NULL;
			break;

		case IMMA_EVT_ND2A_OI_CCB_COMPLETED_UC:
		case IMMA_EVT_ND2A_OI_CCB_APPLY_UC:
		case IMMA_EVT_ND2A_OI_CCB_ABORT_UC:
			break;

		default:
			TRACE_4("Unknown event type %u", evt->type);
			break;
	}
	TRACE_LEAVE();
}

/****************************************************************************
  Name          : imma_process_evt
  Description   : This routine will process the callback event received from
                  IMMND.
  Arguments     : cb - IMMA CB.
                  evt - IMMSV_EVT.
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
  Notes         : None
******************************************************************************/
void imma_process_evt(IMMA_CB *cb, IMMSV_EVT *evt)
{
	TRACE("** Event type:%u", evt->info.imma.type);
	switch (evt->info.imma.type) {
		case IMMA_EVT_ND2A_IMM_PBE_ADMOP:
		case IMMA_EVT_ND2A_IMM_ADMOP:
			imma_proc_admop(cb, &evt->info.imma);
			break;

		case IMMA_EVT_ND2A_ADMOP_RSP:
		case IMMA_EVT_ND2A_ADMOP_RSP_2:
			imma_proc_admin_op_async_rsp(cb, &evt->info.imma);
			break;

		case IMMA_EVT_ND2A_SEARCH_REMOTE:
			imma_proc_rt_attr_update(cb, &evt->info.imma);
			break;

		case IMMA_EVT_ND2A_OI_OBJ_CREATE_UC:
			imma_proc_obj_create(cb, &evt->info.imma);
			break;

		case IMMA_EVT_ND2A_OI_OBJ_DELETE_UC:
			imma_proc_obj_delete(cb, &evt->info.imma);
			break;

		case IMMA_EVT_ND2A_OI_OBJ_MODIFY_UC:
			imma_proc_obj_modify(cb, &evt->info.imma);
			break;

		case IMMA_EVT_ND2A_OI_CCB_COMPLETED_UC:
			imma_proc_ccb_completed(cb, &evt->info.imma);
			break;

		case IMMA_EVT_ND2A_OI_CCB_APPLY_UC:
			imma_proc_ccb_apply(cb, &evt->info.imma);
			break;

		case IMMA_EVT_ND2A_OI_CCB_ABORT_UC:
			imma_proc_ccb_abort(cb, &evt->info.imma);
			break;

		case IMMA_EVT_ND2A_PROC_STALE_CLIENTS:
			LOG_IN("Received PROC_STALE_CLIENTS");
			imma_process_stale_clients(cb);
			break;

		default:
			TRACE_4("Unknown event type %u", evt->info.imma.type);
			break;
	}
	imma_proc_free_pointers(cb, &evt->info.imma);
	return;
}

/****************************************************************************
  Name          : imma_callback_ipc_rcv
 
  Description   : This routine is used Receive the message posted to callback
                  MBX.
 
  Return Values : pointer to the callback
 
  Notes         : None
******************************************************************************/
IMMA_CALLBACK_INFO *imma_callback_ipc_rcv(IMMA_CLIENT_NODE *cl_node)
{
	IMMA_CALLBACK_INFO *cb_info = NULL;

	/* remove it to the queue */
	cb_info = (IMMA_CALLBACK_INFO *)
		m_NCS_IPC_NON_BLK_RECEIVE(&cl_node->callbk_mbx, NULL);

	return cb_info;
}


/****************************************************************************
  Name          : imma_proc_resurrect_client
 
  Description   : Try to resurrect the provided handle. That is re-use
                  the same handle value (which could be stored by the
                  application) and the same client node. Above all the
                  same IPC MBX which the user may be selecting/polling on.

                  Resurrecting the handle is done with a newly restarted IMMND.
                  If the client was attached as an implementer, then
                  the implementer also needs to be re-attached, but that 
                  is  done elsewhere (in imma_oi_resurrect in imma-oi_api.c).
                  since it is only relevant for OI clients. 
                  If the client was attached via the OM interface, then
                  it may be necessary to re-attach to admin-owner, but that
                  is done elsewhere (imma_om_resurrect in in imma_om_api.c). 

                  This function is used for both active and re-active
                  resurrection.

                  The cb_lock should NOT be locked on entry here.
 
  Return Values : true => resurrect succeeded. false => BAD_HANDLE
 
  Notes         : None
******************************************************************************/
uint32_t imma_proc_resurrect_client(IMMA_CB *cb, SaImmHandleT immHandle, bool isOm, SaAisErrorT *err_resurrect)
{
	TRACE_ENTER();
	IMMA_CLIENT_NODE    *cl_node=NULL;
	IMMSV_EVT resurrect_evt;
	IMMSV_EVT *out_evt=NULL;
	bool locked = false;
	SaAisErrorT err;
	unsigned int sleep_delay_ms = 500;
	unsigned int max_waiting_time_ms = 2 * 1000; /* 2 secs */
	unsigned int msecs_waited = 0;

	if (m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS)
	{
		TRACE_3("Lock failure");
		goto lock_fail;
	}
	locked = true;

	imma_client_node_get(&cb->client_tree, &immHandle, &cl_node);
	if (cl_node == NULL || (cl_node->stale && cl_node->exposed))
	{
		TRACE_3("Client not found %p or already exposed %u - cant resurrect", cl_node,
			cl_node?cl_node->exposed:0);
		goto failure;
	}
	
	if (!cl_node->stale) {
		TRACE_3("imma_proc_resurrect_client: Handle %llx was not stale, "
			"resurrected by another thread ?", immHandle);
		goto skip_resurrect;
	}

	if (cl_node->replyPending) {
		TRACE_4("Can not resurrect client with pending replies, client now exposed");
		/* Catches on-going async admin OM op as well as blocked calls */
		cl_node->exposed = true;
		goto failure;
	}

	/* populate the structure */
	memset(&resurrect_evt, 0, sizeof(IMMSV_EVT));
	resurrect_evt.type = IMMSV_EVT_TYPE_IMMND;
	resurrect_evt.info.immnd.type = (isOm)?IMMND_EVT_A2ND_IMM_OM_RESURRECT:
		IMMND_EVT_A2ND_IMM_OI_RESURRECT;
	resurrect_evt.info.immnd.info.finReq.client_hdl = immHandle;
	TRACE_1("Resurrect message for immHandle: %llx isOm: %u",
		immHandle, isOm);
	/* Unlock before MDS Send */
	m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
	locked = false;
	cl_node = NULL;

	if (cb->is_immnd_up == false) {
		TRACE_3("IMMND is DOWN - resurrect attempt failed. ");
		goto exposed;
	}

	err = SA_AIS_ERR_TRY_AGAIN;
	while ((err == SA_AIS_ERR_TRY_AGAIN) && 
		(msecs_waited < max_waiting_time_ms))
	{
		/* send the request to the IMMND */
		if (imma_mds_msg_sync_send(cb->imma_mds_hdl, &(cb->immnd_mds_dest), 
			    &resurrect_evt,&out_evt, IMMSV_WAIT_TIME) !=  NCSCC_RC_SUCCESS) {
			TRACE_3("Failure in MDS send");
			goto exposed;
		}

		if (!out_evt) {
			TRACE_3("Empty reply");
			goto exposed;
		}

		err = out_evt->info.imma.info.errRsp.error;
		if (err == SA_AIS_ERR_TRY_AGAIN) {
			usleep(sleep_delay_ms * 1000);
			msecs_waited += sleep_delay_ms;
		}
		free(out_evt);
		out_evt = NULL;
	}
	*err_resurrect = err;
	if(err == SA_AIS_ERR_TRY_AGAIN){
		TRACE_3("Recieved TRY_AGAIN while resurrecting");
		goto failure;
	}


	if (err != SA_AIS_OK) {
		TRACE_3("Recieved negative reply from IMMND %u", err);
		goto exposed;
	}

	TRACE("OK reply from IMMND on resurrect of handle %llx", immHandle);

	/* Take the lock again. */
	if (m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
		TRACE_3("Lock failure");
		goto lock_fail;
	}
	locked = true;

	/* Look up the client node again. */
	imma_client_node_get(&cb->client_tree, &immHandle, &cl_node);
	if (cl_node == NULL) {
		TRACE_3("Client node missing after reply");
		goto failure;
	}

	if (cl_node->exposed) {
		TRACE_3("Client node got exposed DURING resurrect attempt");
		/* Could happen in a separate thread trying to use the same handle */
		goto failure;
	}

	/* Clear away stale marking */
	cl_node->stale = false;

	/*cl_node->selObjUsable = true;   Done in OM/OI dispatch if relevant. */

 skip_resurrect:
	if (locked) {
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
	}

	TRACE_LEAVE();
	return true;

 exposed:
	/* Try to mark client as exposed */
	if (locked || 
		(m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) == NCSCC_RC_SUCCESS)) {
		locked = true;
		imma_client_node_get(&cb->client_tree, &immHandle, &cl_node);
		if (cl_node != NULL && cl_node->stale) {
			cl_node->exposed = true; 
		}
	}

 failure:
	if (locked) {
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
	}

 lock_fail:
	TRACE_LEAVE();
	return false;
}

/****************************************************************************
  Name          : imma_hdl_callbk_dispatch_one
 
  Description   : This routine dispatches one pending callback.
 
  Arguments     : cb      - ptr to the IMMA control block
                  immHandle - IMM OM service handle
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None
******************************************************************************/
uint32_t imma_hdl_callbk_dispatch_one(IMMA_CB *cb, SaImmHandleT immHandle)
{
	IMMA_CALLBACK_INFO *callback = NULL;
	IMMA_CLIENT_NODE *cl_node = NULL;

	if (m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
		TRACE_3("Lock failure");
		return SA_AIS_ERR_LIBRARY;
	}

	imma_client_node_get(&cb->client_tree, &immHandle, &cl_node);

	if (cl_node == NULL) {
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
		return SA_AIS_ERR_BAD_HANDLE;
	}

	/* get it from the queue */
	while ((callback = imma_callback_ipc_rcv(cl_node))) {
		imma_client_node_get(&cb->client_tree, &immHandle, &cl_node);
		if (cl_node) {
			if (cl_node->stale) {
				cl_node->exposed = true;
				m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
				return SA_AIS_ERR_BAD_HANDLE;
			}
			imma_proc_ccbaug_setup(cl_node, callback);
			m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
			imma_process_callback_info(cb, cl_node, callback, immHandle);
			return SA_AIS_OK;
		} else {
			imma_proc_free_callback(callback);
			continue;
		}
	}

	m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
	return SA_AIS_OK;
}

/****************************************************************************
  Name          : imma_hdl_callbk_dispatch_all
 
  Description   : This routine dispatches all pending callback.
 
  Arguments     : cb      - ptr to the IMMA control block
                  immHandle - IMM OM service handle
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None
******************************************************************************/
uint32_t imma_hdl_callbk_dispatch_all(IMMA_CB *cb, SaImmHandleT immHandle)
{
	IMMA_CALLBACK_INFO *callback = NULL;
	IMMA_CLIENT_NODE *cl_node = NULL;

	if (m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
		TRACE_3("Lock failure");
		return SA_AIS_ERR_LIBRARY;
	}

	imma_client_node_get(&cb->client_tree, &immHandle, &cl_node);

	if (cl_node == NULL) {
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
		return SA_AIS_ERR_BAD_HANDLE;
	}

	while ((callback = imma_callback_ipc_rcv(cl_node))) {
		imma_client_node_get(&cb->client_tree, &immHandle, &cl_node);
		if (cl_node) {
			if (cl_node->stale) {
				cl_node->exposed = true;
				m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
				return SA_AIS_ERR_BAD_HANDLE;
			}

			imma_proc_ccbaug_setup(cl_node, callback);
			m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
			imma_process_callback_info(cb, cl_node, callback, immHandle);
		} else {
			imma_proc_free_callback(callback);
			break;
		}

		if (m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
			TRACE_3("Lock failure");
			return SA_AIS_ERR_LIBRARY;
		}

		/* Is this a way of detecting closed handle => terminate ? */
		imma_client_node_get(&cb->client_tree, &immHandle, &cl_node);

		if (cl_node == NULL) {
			m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
			return SA_AIS_ERR_BAD_HANDLE;
		}
	}

	m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
	return SA_AIS_OK;
}

/****************************************************************************
  Name          : imma_hdl_callbk_dispatch_block
 
  Description   : This routine dispatches all pending callback and blocks
                  when there are no more
  Arguments     : cb      - ptr to the IMMA control block
                  immHandle - immsv handle
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None
******************************************************************************/
uint32_t imma_hdl_callbk_dispatch_block(IMMA_CB *cb, SaImmHandleT immHandle)
{
	IMMA_CALLBACK_INFO *callback = NULL;
	SYSF_MBX *callbk_mbx = NULL;
	IMMA_CLIENT_NODE *client_info = NULL;

	if (m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
		TRACE_3("Lock failure");
		return SA_AIS_ERR_LIBRARY;
	}

	imma_client_node_get(&cb->client_tree, &immHandle, &client_info);

	if (client_info == NULL) {
		/* Another thread called Finalize */
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
		return SA_AIS_ERR_BAD_HANDLE;
	}

	callbk_mbx = &(client_info->callbk_mbx);

	m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);

	callback = (IMMA_CALLBACK_INFO *)m_NCS_IPC_RECEIVE(callbk_mbx, NULL);
	while (1) {
		/* Take the CB Lock */
		if (m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
			TRACE_3("Lock failure");
			return SA_AIS_ERR_LIBRARY;
		}

		imma_client_node_get(&cb->client_tree, &immHandle, &client_info);

		if (callback) {
			if (client_info) {
				if (client_info->stale) {
					client_info->exposed = true;
					m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
					return SA_AIS_ERR_BAD_HANDLE;
				}

				imma_proc_ccbaug_setup(client_info, callback);
				m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
				imma_process_callback_info(cb, client_info, callback, immHandle);
			} else {
				/* Another thread called Finalize? */
				TRACE_3("Client dead?");

				imma_proc_free_callback(callback);
				m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
				return SA_AIS_OK;
			}
		} else {
			m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
			return SA_AIS_ERR_LIBRARY;
		}

		callback = (IMMA_CALLBACK_INFO *)m_NCS_IPC_RECEIVE(callbk_mbx, NULL);
	}

	return SA_AIS_OK;
}

int imma_popAsyncAdmOpContinuation(IMMA_CB *cb,	//in
				     SaInt32T invocation,	//in
				     SaImmHandleT *immHandle,	//out
				     SaInvocationT *userInvoc)	//out
{
	IMMA_CONTINUATION_RECORD *cr = cb->imma_continuations;
	IMMA_CONTINUATION_RECORD **prevCr = &(cb->imma_continuations);

	TRACE("POP continuation %i", invocation);

	while (cr) {
		if (cr->invocation == invocation) {
			*immHandle = cr->immHandle;
			*userInvoc = cr->userInvoc;
			*prevCr = cr->next;
			cr->next = NULL;
			free(cr);
			return 1;
		}

		prevCr = &(cr->next);
		cr = cr->next;
	}
	TRACE_3("POP continuation %i not found", invocation);
	return 0;
}


static void imma_proc_ccbaug_setup(IMMA_CLIENT_NODE *cl_node, IMMA_CALLBACK_INFO *callback)
{
	if(cl_node->isApplier) {return;}

	/* Locked on entry */
	switch (callback->type) {
		case IMMA_CALLBACK_OI_CCB_CREATE:
		case IMMA_CALLBACK_OI_CCB_DELETE:
		case IMMA_CALLBACK_OI_CCB_MODIFY:
			if(!(imma_oi_ccb_record_note_callback(cl_node, callback->ccbID, callback))) {
				TRACE_3("Failed to note callback for ccb %u, "
					"Ccb augment not possible", callback->ccbID);
			}
			break;

		default:
			imma_oi_ccb_record_note_callback(cl_node, callback->ccbID, NULL);
	}
}

/****************************************************************************
  Name          : imma_process_callback_info
 
  Description   : This routine invokes the registered callback routine.
 
  Arguments     : cb  - ptr to the IMMA control block
                  cl_node - Client Node
                  callback - ptr to the registered callbacks
                  immHandle - handle used to re-fetch cl_node if necessary.
 
  Return Values : None
 
  Notes         : None
******************************************************************************/
static void imma_process_callback_info(IMMA_CB *cb, IMMA_CLIENT_NODE *cl_node, 
	IMMA_CALLBACK_INFO *callback, SaImmHandleT immHandle)
{
	/* Not locked => the use of cl_node is a bit unsafe here. 
	   We should at least have a dont-delete marking in the client node.
	 */
	TRACE_ENTER();
	/* invoke the corresponding callback */
#ifdef IMMA_OM
	switch (callback->type) {
		case IMMA_CALLBACK_OM_ADMIN_OP_RSP:	/*Async reply via OM. */
			/* ABT decide if it is A.2.1 or A.2.11 callback. */
			if (cl_node->isImmA2bCbk) {
				cl_node->o.mCallbkA2b.saImmOmAdminOperationInvokeCallback(callback->invocation,
					callback->retval, callback->sa_err, 
					(const SaImmAdminOperationParamsT_2 **)	callback->params);
			} else if (cl_node->o.mCallbk.saImmOmAdminOperationInvokeCallback) {
				TRACE("Upcall for callback for async admop inv:%llx rslt:%u err:%u",
					callback->invocation, callback->retval, callback->sa_err);

				cl_node->o.mCallbk.saImmOmAdminOperationInvokeCallback(callback->invocation,
					callback->retval, callback->sa_err);
			} else {
				TRACE_3("No callback to deliver AdminOperationInvokeAsync - invoc:%llx ret:%u err:%u",
					callback->invocation, callback->retval, callback->sa_err);
			}

			/* callback->params freed by imma_proc_free_callback(callback) below */

			break;

		case IMMA_CALLBACK_STALE_HANDLE:
			TRACE("Stale OM handle upcall completed");
			/* Do nothing. */
			osafassert(!cl_node->isPbe); /* Should have exited in imma_mark_clients_stale() */
			break;

		default:
			TRACE_3("Unrecognized OM callback type:%u", callback->type);
			break;
	}
#endif

#ifdef IMMA_OI
	bool isPbeOp = false;
	switch (callback->type) {
		case IMMA_CALLBACK_PBE_ADMIN_OP:
			isPbeOp = true;
			osafassert(cl_node->isPbe);
			TRACE("PBE Admin OP callback");
		case IMMA_CALLBACK_OM_ADMIN_OP:
			if (cl_node->o.iCallbk.saImmOiAdminOperationCallback) {
				cl_node->o.iCallbk.saImmOiAdminOperationCallback(callback->lcl_imm_hdl,
					callback->invocation,
					&(callback->name),
					callback->operationId,
					(const SaImmAdminOperationParamsT_2 **)
					callback->params);
			} else {
				/*No callback registered for admin-op!! */
				SaAisErrorT localErr = saImmOiAdminOperationResult(callback->lcl_imm_hdl,
					callback->invocation,
					IMMSV_IMPOSSIBLE_ERROR);
				if (localErr == SA_AIS_OK) {
					TRACE_3("Object %s has implementer but "
						"saImmOiAdminOperationCallback is set to NULL", callback->name.value);
				} else {
					TRACE_3("Object %s has implementer but "
						"saImmOiAdminOperationCallback is set to NULL "
						"and could not send error result, error: %u", callback->name.value, localErr);
				}
			}
			break;

		case IMMA_CALLBACK_PBE_PRTO_DELETES_COMPLETED:
			isPbeOp = true;
			osafassert(cl_node->isPbe);
		case IMMA_CALLBACK_OI_CCB_COMPLETED:
			TRACE("%s-completed op callback", isPbeOp?"Pbe-Prto-Deletes":"ccb");
			do {
				SaAisErrorT localEr = SA_AIS_OK;
				IMMSV_EVT ccbCompletedRpl;
				bool locked = false;
				SaStringT errorStr = NULL;
				if (cl_node->o.iCallbk.saImmOiCcbCompletedCallback)
				{
					SaImmOiCcbIdT ccbid = 0LL;

					if(isPbeOp) {
						osafassert(callback->ccbID == 0);
						/* Pseudo ccb towards PBE for PRTO deletes */
						ccbid = callback->inv + 0x100000000LL;

						/* PRTO delete reply only on completed*/
						//callback->inv = 0; 
						TRACE("Pseudo ccb %llx for PRTO deletes completed upcall on %s",
							ccbid, callback->name.value);
						if(!imma_oi_ccb_record_ok_for_critical(cl_node, ccbid, callback->implId)) {
							TRACE("ERROR: RtObjectDelete record for pseudo-ccb %llx does not have"
								"correct op-count", ccbid); /* Already logged in ok_for_critical */
							imma_oi_ccb_record_terminate(cl_node, ccbid);
							localEr = SA_AIS_ERR_FAILED_OPERATION;
							goto skip_completed_upcall;
						}

						imma_oi_ccb_record_terminate(cl_node, ccbid);
					} else {
						ccbid = callback->ccbID;
					        if(!(cl_node->isApplier)) {imma_oi_ccb_allow_error_string(cl_node, ccbid);}
						/*Verify op-count before PBE commit of ccb. 
						  This to eliminate the risk of the CCB committing in PBE
						  yet failing in imma_oi_ccb_record_set_critical below after the
						  PBE transaction. */
						if(!imma_oi_ccb_record_ok_for_critical(cl_node, ccbid, callback->inv)) {
							LOG_ER("ERROR: CCB record for %u does not have correct op-count",
								callback->ccbID);
							localEr = SA_AIS_ERR_FAILED_OPERATION;
							goto skip_completed_upcall;
						}
					}

					localEr = cl_node->o.iCallbk.saImmOiCcbCompletedCallback(callback->lcl_imm_hdl, ccbid);
					if (!(localEr == SA_AIS_OK ||
						    localEr == SA_AIS_ERR_NO_MEMORY ||
						    localEr == SA_AIS_ERR_NO_RESOURCES || localEr == SA_AIS_ERR_BAD_OPERATION)) {
						TRACE_3("Illegal return value from "
							"saImmOiCcbCompletedCallback %u. "
							"Allowed are %u %u %u %u", localEr, SA_AIS_OK,
							SA_AIS_ERR_NO_MEMORY, SA_AIS_ERR_NO_RESOURCES,
							SA_AIS_ERR_BAD_OPERATION);
						localEr = SA_AIS_ERR_FAILED_OPERATION;
					}

					if(isPbeOp) {
						cl_node->o.iCallbk.saImmOiCcbApplyCallback(callback->lcl_imm_hdl, ccbid);
					}

				} else {
					/* No callback function registered for completed upcall.
					   The standard is not clear on how this case should be handled.
					   We take the strict approach of demanding that, if there is
					   a registered implementer, then that implementer must 
					   implement the completed callback, for the callback to succeed
					*/
					TRACE_2("ERR_FAILED_OPERATION: saImmOiCcbCompletedCallback is not implemented, yet "
						"implementer is registered and CCBs are used. Ccb will fail");
					localEr = SA_AIS_ERR_FAILED_OPERATION;
				}
			skip_completed_upcall:

				memset(&ccbCompletedRpl, 0, sizeof(IMMSV_EVT));
				ccbCompletedRpl.type = IMMSV_EVT_TYPE_IMMND;
				if(isPbeOp) {
					ccbCompletedRpl.info.immnd.type = IMMND_EVT_A2ND_PBE_PRTO_DELETES_COMPLETED_RSP;
				} else {
					ccbCompletedRpl.info.immnd.type = IMMND_EVT_A2ND_CCB_COMPLETED_RSP;
					ccbCompletedRpl.info.immnd.info.ccbUpcallRsp.ccbId = callback->ccbID;
				}
				ccbCompletedRpl.info.immnd.info.ccbUpcallRsp.result = localEr;
				ccbCompletedRpl.info.immnd.info.ccbUpcallRsp.oi_client_hdl = callback->lcl_imm_hdl;
				ccbCompletedRpl.info.immnd.info.ccbUpcallRsp.implId = callback->implId;
				ccbCompletedRpl.info.immnd.info.ccbUpcallRsp.inv = callback->inv;

				osafassert(m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) == NCSCC_RC_SUCCESS);
				locked = true;
				/*async  fevs */

				imma_client_node_get(&cb->client_tree, &(immHandle), &cl_node);
				osafassert(cl_node);

				/* Close window for setting error strings also for the OK case. #2233 */
				errorStr = imma_oi_ccb_record_get_error(cl_node, callback->ccbID);

				if (localEr == SA_AIS_OK)  {
					/* replying OK => entering critical phase for this OI and this CCB.
					   There can be many simultaneous OM clients starting CCBs that impact the same OI. 
					   Some OIs may not accept this and may reject overlapping CCBs, but we can not
					   assume this in the IMMSv implementation. 
					*/
					if (isPbeOp || imma_oi_ccb_record_set_critical(cl_node, callback->ccbID, callback->inv)) {
						TRACE_2("Sending normal OK response on completed for ccb %u. ", callback->ccbID);
						if(!isPbeOp) {TRACE_2("The oi_ccb_record now marked as critical.");}
					} else {
						TRACE_2("CCB record for %u non existent - must have been aborted", callback->ccbID);
						osafassert(m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE) == NCSCC_RC_SUCCESS);
						locked = false;
						break; /* out of while */
					}
				} else {
					TRACE_2("Sending FAILED_OP response on completed. for ccb %u.",	callback->ccbID);
					if(errorStr) {
						ccbCompletedRpl.info.immnd.type = IMMND_EVT_A2ND_CCB_COMPLETED_RSP_2;
						ccbCompletedRpl.info.immnd.info.ccbUpcallRsp.errorString.size = 
							strlen(errorStr) + 1;
						ccbCompletedRpl.info.immnd.info.ccbUpcallRsp.errorString.buf = errorStr;
					}
				}

				if(!(cl_node->isApplier)) {
					localEr = imma_evt_fake_evs(cb, &ccbCompletedRpl, NULL, 0, cl_node->handle, &locked, false);
					if (localEr != NCSCC_RC_SUCCESS) {
						/*Cant do anything but log error and drop this reply. */
						TRACE_3("CcbCompletedCallback: send reply to IMMND failed");
					}
				}

				if (locked) {
					osafassert(m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE) == NCSCC_RC_SUCCESS);
					locked = false;
				}
			} while (0);
			break;

		case IMMA_CALLBACK_OI_CCB_APPLY:
			TRACE("ccb-apply op callback");
			do {
				/* Anoying type diff for ccbid between OM and OI */
				SaImmOiCcbIdT ccbid = callback->ccbID;
				SaImmHandleT privateAugOmHandle = 0LL;

				if (cl_node->o.iCallbk.saImmOiCcbApplyCallback)
				{
					cl_node->o.iCallbk.saImmOiCcbApplyCallback(callback->lcl_imm_hdl, ccbid);
				} else {
					/* No callback function registered for apply upcall.
					   There is nothing we can do about this since the CCB is
					   commited already. It also makes sense that some applications
					   may want to ignore the apply upcall.
					*/
					TRACE_3("saImmOiCcbApplyCallback is not implemented, yet "
						"implementer is registered and CCBs are used. Ccb will commit in any case");
				}
				osafassert(m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) == NCSCC_RC_SUCCESS);
				if(!imma_oi_ccb_record_close_augment(cl_node, ccbid, &privateAugOmHandle, true)) {
					TRACE_2("saImmOiCcbApplyCallback: imma_oi_ccb_record_close_augment returned false "
						"for ccb %llu", ccbid);
				}

				if (imma_oi_ccb_record_terminate(cl_node, ccbid)) {
					TRACE_2("CCB-APPLY-UC for %llu received from IMMND - oi_ccb_record terminated",
						ccbid);
				} else {
					TRACE_4("ERROR: CCB-APPLY-UC - CCB record non existentfor ccb %llu",
						ccbid);
				}
				osafassert(m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE) == NCSCC_RC_SUCCESS);

				if(privateAugOmHandle) {
					osafassert(immsv_om_handle_finalize);
					immsv_om_handle_finalize(privateAugOmHandle);
				}
			} while (0);
			break;

		case IMMA_CALLBACK_PBE_PRT_OBJ_CREATE:
			isPbeOp = true;
			osafassert(cl_node->isPbe);
		case IMMA_CALLBACK_OI_CCB_CREATE:
			TRACE("%sobject-create callback", isPbeOp?"Pbe-Runtime-":"Ccb-");
			do {
				SaAisErrorT localEr = SA_AIS_OK;
				IMMSV_EVT ccbObjCrRpl;
				bool locked = false;
				SaImmAttrValuesT_2 **attr = NULL;
				size_t attrDataSize = 0;
				int i = 0;
				if (cl_node->o.iCallbk.saImmOiCcbObjectCreateCallback)
				{
					/* Anoying type diff for ccbid between OM and OI */
					SaImmOiCcbIdT ccbid = callback->ccbID;

					if(!(cl_node->isApplier)) {imma_oi_ccb_allow_error_string(cl_node, ccbid);}

					SaNameT parentName = callback->name;
					const SaImmClassNameT className = callback->className;	/*0 */
					callback->className = NULL;
					int noOfAttributes = 0;

					/* NOTE: The code below is practically a copy of the code
					   in immOm searchNext, for serving the attrValues structure.
					   This code should be factored out into some common function.
					*/
					IMMSV_ATTR_VALUES_LIST *p = callback->attrValues;
					while (p) {
						++noOfAttributes;
						p = p->next;
					}

					attrDataSize = sizeof(SaImmAttrValuesT_2 *) * (noOfAttributes + 1);
					attr = calloc(1, attrDataSize);	/*alloc-1 */
					p = callback->attrValues;
					for (; i < noOfAttributes; i++, p = p->next) {
						IMMSV_ATTR_VALUES *q = &(p->n);
						attr[i] = calloc(1, sizeof(SaImmAttrValuesT_2));	/*alloc-2 */
						attr[i]->attrName = malloc(q->attrName.size + 1);	/*alloc-3 */
						strncpy(attr[i]->attrName, (const char *)q->attrName.buf, q->attrName.size + 1);
						attr[i]->attrName[q->attrName.size] = 0;	/*redundant. */
						attr[i]->attrValuesNumber = q->attrValuesNumber;
						attr[i]->attrValueType = (SaImmValueTypeT)q->attrValueType;
						if (q->attrValuesNumber) {
							attr[i]->attrValues =
								calloc(q->attrValuesNumber, sizeof(SaImmAttrValueT));	/*alloc-4 */
							/*alloc-5 */
							attr[i]->attrValues[0] =
								imma_copyAttrValue3(q->attrValueType, &(q->attrValue));

							if (q->attrValuesNumber > 1) {
								int ix;
								IMMSV_EDU_ATTR_VAL_LIST *r = q->attrMoreValues;
								for (ix = 1; ix < q->attrValuesNumber; ++ix) {
									osafassert(r);
									attr[i]->attrValues[ix] = /*alloc-5 */
										imma_copyAttrValue3(q->attrValueType, &(r->n));
									r = r->next;
								}//for
							}//if
						}//if
					}//for
					attr[noOfAttributes] = NULL;	/*redundant */

					/*Need a separate const pointer just to avoid an INCORRECT warning
					  by the stupid compiler. This compiler warns when assigning 
					  non-const to a const !!!! and it is not even possible to do the 
					  cast in the function call. Note: const is MORE restrictive than 
					  non-const so assigning to a const should ALWAYS be allowed. 
					*/

					TRACE("ccb-object-create make the callback");
					/* Save a copy of the attrvals pointer  in the callback to allow
					   saImmOiAugmentCcbInitialize to get access to the ccbCreateContext.
					   We cant use callback->attrValues because it is partially stolen
					   from (caused to be incomplete) in the loop above (see imma_copyAttrValue3).
					 */
					callback->attrValsForCreateUc = (const SaImmAttrValuesT_2 **)attr;

					localEr = cl_node->o.iCallbk.saImmOiCcbObjectCreateCallback(callback->lcl_imm_hdl,
						ccbid,
						className,
						&parentName,
						callback->attrValsForCreateUc);

					TRACE("ccb-object-create callback returned RC:%u", localEr);
					if (!(localEr == SA_AIS_OK ||
						    localEr == SA_AIS_ERR_NO_MEMORY ||
						    localEr == SA_AIS_ERR_NO_RESOURCES || localEr == SA_AIS_ERR_BAD_OPERATION)) {
						TRACE_2("ERR_FAILED_OPERATION: Illegal return value from "
							"saImmOiCcbObjectCreateCallback %u. "
							"Allowed are %u %u %u %u", localEr, SA_AIS_OK,
							SA_AIS_ERR_NO_MEMORY, SA_AIS_ERR_NO_RESOURCES,
							SA_AIS_ERR_BAD_OPERATION);
						localEr = SA_AIS_ERR_FAILED_OPERATION;
						/*Change to BAD_OP if only aborting create and not ccb. */
					}

					free(className);	/*free-0 */
					for (i = 0; attr[i]; ++i) {
						free(attr[i]->attrName);	/*free-3 */
						attr[i]->attrName = 0;
						if (attr[i]->attrValuesNumber) {
							int j;
							for (j = 0; j < attr[i]->attrValuesNumber; ++j) {
								imma_freeAttrValue3(attr[i]->attrValues[j], attr[i]->attrValueType);/*free-5 */
								attr[i]->attrValues[j] = 0;
							}
							free(attr[i]->attrValues);	/*4 */
							/* SaImmAttrValueT[] array-of-void* */
							attr[i]->attrValues = 0;
						}
						free(attr[i]);	/*2 */
						/* SaImmAttrValuesT struct */
						attr[i] = 0;
					}
					free(attr);	/*1 */
					/* SaImmAttrValuesT*[]  array-off-pointers */
				} else {
					/* No callback function registered for obj-create upcall.
					   The standard is not clear on how this case should be 
					   handled. We take the strict approach of demanding that, 
					   if there is a registered implementer, then that 
					   implementer must implement the create callback, for the
					   callback to succeed.
					*/
					TRACE_2("ERR_FAILED_OPERATION: saImmOiCcbObjectCreateCallback is not implemented, "
						"yet implementer is registered and CCBs are used. Ccb will fail");

					localEr = SA_AIS_ERR_FAILED_OPERATION;
				}
				if(callback->inv) { 
					SaStringT errorStr = NULL;
					SaImmHandleT privateAugOmHandle = 0LL;
					osafassert(m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) == NCSCC_RC_SUCCESS);
					locked = true;
					memset(&ccbObjCrRpl, 0, sizeof(IMMSV_EVT));
					ccbObjCrRpl.type = IMMSV_EVT_TYPE_IMMND;
					if(isPbeOp) {
						ccbObjCrRpl.info.immnd.type = IMMND_EVT_A2ND_PBE_PRT_OBJ_CREATE_RSP;
					} else {
						ccbObjCrRpl.info.immnd.type = IMMND_EVT_A2ND_CCB_OBJ_CREATE_RSP;
						ccbObjCrRpl.info.immnd.info.ccbUpcallRsp.ccbId = callback->ccbID;
					}
					ccbObjCrRpl.info.immnd.info.ccbUpcallRsp.result = localEr;
					ccbObjCrRpl.info.immnd.info.ccbUpcallRsp.oi_client_hdl = callback->lcl_imm_hdl;
					ccbObjCrRpl.info.immnd.info.ccbUpcallRsp.inv = callback->inv;

					/* Closes window for setting error strings also for the OK case. #2233 */
					errorStr = imma_oi_ccb_record_get_error(cl_node, callback->ccbID);

					if (localEr != SA_AIS_OK)  {
						if(errorStr) {
							ccbObjCrRpl.info.immnd.type = IMMND_EVT_A2ND_CCB_OBJ_CREATE_RSP_2;
							ccbObjCrRpl.info.immnd.info.ccbUpcallRsp.errorString.size =
								strlen(errorStr) + 1;
							ccbObjCrRpl.info.immnd.info.ccbUpcallRsp.errorString.buf = errorStr;
						}
					}

					if(!imma_oi_ccb_record_close_augment(cl_node, callback->ccbID, &privateAugOmHandle, false)) {
						TRACE_3("CcbObjectCreatedCallback: imma_oi_ccb_record_close_augment "
							"returned false for ccb %u", callback->ccbID);
					}

					if(privateAugOmHandle) {
						osafassert(m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE) == NCSCC_RC_SUCCESS);
						locked = false;

						osafassert(immsv_om_augment_ccb_get_result);
						SaAisErrorT augResult = immsv_om_augment_ccb_get_result(privateAugOmHandle,
							callback->ccbID);

						osafassert(m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) == NCSCC_RC_SUCCESS);
						locked = true;

						if(augResult != SA_AIS_OK) {
							TRACE("A non-ok result from an augmented CCB %u overrides the reply from "
								"the OI %u on create-uc", augResult,
								ccbObjCrRpl.info.immnd.info.ccbUpcallRsp.result);

							ccbObjCrRpl.info.immnd.info.ccbUpcallRsp.result = augResult;
						}							
					}

					/*async fevs */
					localEr = imma_evt_fake_evs(cb, &ccbObjCrRpl, NULL, 0, cl_node->handle, &locked, false);
				} else {
					/* callback->inv == 0 means PBE CCB obj create upcall, NO reply.
					   But note that for PBE PRTO, callback->inv != 0 and we reply
					   immediately (no completed upcall. 
					   Added support for applier OI. No reply to server there either.
					*/
					osafassert(cl_node->isPbe || cl_node->isApplier);
				}

				if (locked) {
					osafassert(m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE) == NCSCC_RC_SUCCESS);
					locked = false;
				}

				if (localEr != NCSCC_RC_SUCCESS) {
					/*Cant do anything but log error and drop this reply. */
					TRACE_3("CcbObjectCreatedCallback: send reply to IMMND failed");
				}
			} while (0);
			break;
			
		case IMMA_CALLBACK_PBE_PRT_OBJ_DELETE:
			isPbeOp = true;
			osafassert(cl_node->isPbe);
		case IMMA_CALLBACK_OI_CCB_DELETE:
			TRACE("%sobject-delete op callback", isPbeOp?"Pbe-Runtime-":"Ccb-");
			do {
				SaAisErrorT localEr = SA_AIS_OK;
				IMMSV_EVT ccbObjDelRpl;
				bool locked = false;

				if (cl_node->o.iCallbk.saImmOiCcbObjectDeleteCallback) {
					SaImmOiCcbIdT ccbid = 0LL;
					if(isPbeOp) {
						osafassert(callback->ccbID == 0);
						/* Pseudo ccb towards PBE for PRTO deletes */
						ccbid = callback->inv + 0x100000000LL;

						/* PRTO delete reply only on completed*/
						callback->inv = 0; 
						TRACE("Pseudo ccb %llx for PRTO delete upcall on %s",
							ccbid, callback->name.value);
					} else {
						ccbid = callback->ccbID;
					}

					if(!(cl_node->isApplier)) {imma_oi_ccb_allow_error_string(cl_node, ccbid);}

					localEr = cl_node->o.iCallbk.saImmOiCcbObjectDeleteCallback(callback->lcl_imm_hdl,
						ccbid, &(callback->name));

					TRACE("ccb-object-delete callback returned RC:%u", localEr);
					if (!(localEr == SA_AIS_OK ||
						    localEr == SA_AIS_ERR_NO_MEMORY ||
						    localEr == SA_AIS_ERR_NO_RESOURCES || localEr == SA_AIS_ERR_BAD_OPERATION)) {
						TRACE_2("ERR_FAILED_OPERATION: Illegal return value from "
							"saImmOiCcbObjectDeleteCallback %u. "
							"Allowed are %u %u %u %u", localEr, SA_AIS_OK,
							SA_AIS_ERR_NO_MEMORY, SA_AIS_ERR_NO_RESOURCES,
							SA_AIS_ERR_BAD_OPERATION);
						localEr = SA_AIS_ERR_FAILED_OPERATION;
						/*Change to BAD_OP if only aborting delete and not ccb. */
					}
				} else {
					/* No callback function registered for obj delete upcall.
					   The standard is not clear on how this case should be handled.
					   We take the strict approach of demanding that, if there is
					   a registered implementer, then that implementer must 
					   implement the delete callback, for the callback to succeed
					*/
					TRACE_2("ERR_FAILED_OPERATION: saImmOiCcbObjectDeleteCallback is not implemented, yet "
						"implementer is registered and CCBs are used. Ccb will fail");
					localEr = SA_AIS_ERR_FAILED_OPERATION;
				}

				if(callback->inv) { 
					SaStringT errorStr = NULL;
					SaImmHandleT privateAugOmHandle = 0LL;
					memset(&ccbObjDelRpl, 0, sizeof(IMMSV_EVT));
					ccbObjDelRpl.type = IMMSV_EVT_TYPE_IMMND;
					ccbObjDelRpl.info.immnd.type = IMMND_EVT_A2ND_CCB_OBJ_DELETE_RSP;
					ccbObjDelRpl.info.immnd.info.ccbUpcallRsp.result = localEr;
					ccbObjDelRpl.info.immnd.info.ccbUpcallRsp.oi_client_hdl = callback->lcl_imm_hdl;
					ccbObjDelRpl.info.immnd.info.ccbUpcallRsp.ccbId = callback->ccbID;
					ccbObjDelRpl.info.immnd.info.ccbUpcallRsp.inv = callback->inv;
					ccbObjDelRpl.info.immnd.info.ccbUpcallRsp.name = callback->name;

					osafassert(m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) == NCSCC_RC_SUCCESS);
					locked = true;

					/* Closes window for setting error strings also for the OK case. #2233 */
					errorStr = imma_oi_ccb_record_get_error(cl_node, callback->ccbID);

					if (localEr != SA_AIS_OK)  {
						if(errorStr) {
							ccbObjDelRpl.info.immnd.type = IMMND_EVT_A2ND_CCB_OBJ_DELETE_RSP_2;
							ccbObjDelRpl.info.immnd.info.ccbUpcallRsp.errorString.size =
								strlen(errorStr) + 1;
							ccbObjDelRpl.info.immnd.info.ccbUpcallRsp.errorString.buf = errorStr;
						}
					}

					if(!imma_oi_ccb_record_close_augment(cl_node, callback->ccbID, &privateAugOmHandle, false)) {
						TRACE_3("CcbObjectDeleteCallback: imma_oi_ccb_record_close_augment "
							"returned false for ccb %u", callback->ccbID);
					}

					if(privateAugOmHandle) {
						osafassert(m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE) == NCSCC_RC_SUCCESS);
						locked = false;

						osafassert(immsv_om_augment_ccb_get_result);
						SaAisErrorT augResult = immsv_om_augment_ccb_get_result(privateAugOmHandle,
							callback->ccbID);

						osafassert(m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) == NCSCC_RC_SUCCESS);
						locked = true;

						if(augResult != SA_AIS_OK) {
							TRACE("A non-ok result from an augmented CCB %u overrides the reply from "
								"the OI %u on delete-op", augResult,
								ccbObjDelRpl.info.immnd.info.ccbUpcallRsp.result);

							ccbObjDelRpl.info.immnd.info.ccbUpcallRsp.result = augResult;
						}							
					}

					/*async  fevs */
					localEr = imma_evt_fake_evs(cb, &ccbObjDelRpl, NULL, 0, cl_node->handle, &locked, false);
					if (localEr != NCSCC_RC_SUCCESS) {
						/*Cant do anything but log error and drop this reply. */
						TRACE_3("CcbObjectDeleteCallback: send reply to IMMND failed");
					}
				} else {
					/* callback->inv == 0 means PBE (CCB or PRTO) or applier upcall, no reply. */
					osafassert(cl_node->isPbe || cl_node->isApplier);
				}

				if (locked) {
					osafassert(m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE) == NCSCC_RC_SUCCESS);
					locked = false;
				}
			} while (0);

			break;

		case IMMA_CALLBACK_PBE_PRT_ATTR_UPDATE:
			isPbeOp = true; /*Actually isPrtAttr would be better name here. */
			osafassert(cl_node->isPbe);
		case IMMA_CALLBACK_OI_CCB_MODIFY:
			TRACE("%s op callback", isPbeOp?"Pbe-runtime update":"ccb");
			do {
				SaAisErrorT localEr = SA_AIS_OK;
				IMMSV_EVT ccbObjModRpl;
				bool locked = false;
				SaImmAttrModificationT_2 **attr = NULL;
				int i = 0;
				if (cl_node->o.iCallbk.saImmOiCcbObjectModifyCallback) {
					/* Anoying type diff for ccbid between OM and OI */
					SaImmOiCcbIdT ccbid = callback->ccbID;

					if(!(cl_node->isApplier)) {imma_oi_ccb_allow_error_string(cl_node, ccbid);}

					SaNameT objectName = callback->name;
					int noOfAttrMods = 0;

					IMMSV_ATTR_MODS_LIST *p = callback->attrMods;
					while (p) {
						++noOfAttrMods;
						p = p->next;
					}

					/*alloc-1 */
					attr = calloc(noOfAttrMods + 1, sizeof(SaImmAttrModificationT_2 *));
					p = callback->attrMods;
					for (; i < noOfAttrMods; i++, p = p->next) {

						attr[i] = calloc(1, sizeof(SaImmAttrModificationT_2));	/*alloc-2 */
						attr[i]->modType = p->attrModType;

						attr[i]->modAttr.attrName = p->attrValue.attrName.buf;	/* steal/alloc-3 */
						p->attrValue.attrName.buf = NULL;
						p->attrValue.attrName.size = 0;
						attr[i]->modAttr.attrValuesNumber = p->attrValue.attrValuesNumber;
						attr[i]->modAttr.attrValueType = (SaImmValueTypeT)
							p->attrValue.attrValueType;
						if (p->attrValue.attrValuesNumber) {
							attr[i]->modAttr.attrValues =
								calloc(p->attrValue.attrValuesNumber, sizeof(SaImmAttrValueT));	/*alloc-4 */
							/*alloc-5 */
							attr[i]->modAttr.attrValues[0] =
								imma_copyAttrValue3(p->attrValue.attrValueType, &(p->attrValue.attrValue));

							if (p->attrValue.attrValuesNumber > 1) {
								int ix;
								IMMSV_EDU_ATTR_VAL_LIST *r = p->attrValue.attrMoreValues;
								for (ix = 1; ix < p->attrValue.attrValuesNumber; ++ix) {
									osafassert(r);
									attr[i]->modAttr.attrValues[ix] =
										imma_copyAttrValue3(p->attrValue.attrValueType, &(r->n));/*alloc-5 */

									r = r->next;
								}//for all extra values
							}//if multivalued
						}//if there was any value
					}//for all attrMods
					/*attr[noOfAttrMods] = NULL; redundant */

					/*Need a separate const pointer just to avoid an INCORRECT warning
					  by the stupid compiler. This compiler warns when assigning 
					  non-const to a const !!!! and it is not even possible to do the 
					  cast in the function call. Note: const is MORE restrictive than 
					  non-const so assigning to a const should ALWAYS be allowed. 
					*/
					TRACE("ccb-object-modify: make the callback");
					const SaImmAttrModificationT_2 **constPtrForStupidCompiler =
						(const SaImmAttrModificationT_2 **)attr;

					localEr = cl_node->o.iCallbk.saImmOiCcbObjectModifyCallback(callback->lcl_imm_hdl, ccbid, &objectName,
						constPtrForStupidCompiler);

					TRACE("ccb-object-modify callback returned RC:%u", localEr);
					if (!(localEr == SA_AIS_OK ||
						    localEr == SA_AIS_ERR_NO_MEMORY ||
						    localEr == SA_AIS_ERR_NO_RESOURCES || localEr == SA_AIS_ERR_BAD_OPERATION)) {
						TRACE_2("ERR_FAILED_OPERATION: Illegal return value from "
							"saImmOiCcbObjectModifyCallback %u. "
							"Allowed are %u %u %u %u", localEr, SA_AIS_OK,
							SA_AIS_ERR_NO_MEMORY, SA_AIS_ERR_NO_RESOURCES,
							SA_AIS_ERR_BAD_OPERATION);
						localEr = SA_AIS_ERR_FAILED_OPERATION;
					}

					for (i = 0; attr[i]; ++i) {
						free(attr[i]->modAttr.attrName);	/*free-3 */
						attr[i]->modAttr.attrName = 0;
						if (attr[i]->modAttr.attrValuesNumber) {
							int j;
							for (j = 0; j < attr[i]->modAttr.attrValuesNumber; ++j) {
								imma_freeAttrValue3(attr[i]->modAttr.attrValues[j],
									attr[i]->modAttr.attrValueType);	/*free-5 */
								attr[i]->modAttr.attrValues[j] = 0;
							}
							free(attr[i]->modAttr.attrValues);	/*4 */
							/* SaImmAttrValueT[] array-of-void* */
							attr[i]->modAttr.attrValues = 0;
						}
						free(attr[i]);	/*2 */
						/* SaImmAttrValuesT struct */
						attr[i] = 0;
					}
					free(attr);	/*1 */
					/* SaImmAttrValuesT*[]  array-off-pointers */
				} else {
					/* No callback function registered for obj-modify upcall.
					   The standard is not clear on how this case should be 
					   handled. We take the strict approach of demanding that, 
					   if there is a registered implementer, then that 
					   implementer must implement the create callback, for the
					   callback to succeed.
					*/
					TRACE_2("ERR_FAILED_OPERATION: saImmOiCcbObjectModifyCallback is not implemented, "
						"yet implementer is registered and CCBs are used. Ccb will fail");

					localEr = SA_AIS_ERR_FAILED_OPERATION;
					/*Change to BAD_OP if only aborting modify and not ccb. */
				}
				if(callback->inv) { 
					SaStringT errorStr = NULL;
					SaImmHandleT privateAugOmHandle = 0LL;
					osafassert(m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) == NCSCC_RC_SUCCESS);
					locked = true;
					memset(&ccbObjModRpl, 0, sizeof(IMMSV_EVT));
					ccbObjModRpl.type = IMMSV_EVT_TYPE_IMMND;
					if(isPbeOp) {
						ccbObjModRpl.info.immnd.type = IMMND_EVT_A2ND_PBE_PRT_ATTR_UPDATE_RSP;
					} else {
						ccbObjModRpl.info.immnd.type=IMMND_EVT_A2ND_CCB_OBJ_MODIFY_RSP;
						ccbObjModRpl.info.immnd.info.ccbUpcallRsp.ccbId = callback->ccbID;
					}
					ccbObjModRpl.info.immnd.info.ccbUpcallRsp.result = localEr;
					ccbObjModRpl.info.immnd.info.ccbUpcallRsp.oi_client_hdl = callback->lcl_imm_hdl;
					ccbObjModRpl.info.immnd.info.ccbUpcallRsp.inv = callback->inv;

					/* Closes window for setting error strings also for the OK case. #2233 */
					errorStr = imma_oi_ccb_record_get_error(cl_node, callback->ccbID);

					if (localEr != SA_AIS_OK)  {
						if(errorStr) {
							ccbObjModRpl.info.immnd.type=IMMND_EVT_A2ND_CCB_OBJ_MODIFY_RSP_2;
							ccbObjModRpl.info.immnd.info.ccbUpcallRsp.errorString.size =
								strlen(errorStr) + 1;
							ccbObjModRpl.info.immnd.info.ccbUpcallRsp.errorString.buf = errorStr;
						}
					}

					if(!imma_oi_ccb_record_close_augment(cl_node, callback->ccbID, &privateAugOmHandle, false)) {
						TRACE_3("CcbObjectModifyCallback: imma_oi_ccb_record_close_augment "
							"returned false for ccb %u", callback->ccbID);
					}

					if(privateAugOmHandle) {
						osafassert(m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE) == NCSCC_RC_SUCCESS);
						locked = false;

						osafassert(immsv_om_augment_ccb_get_result);
						SaAisErrorT augResult = immsv_om_augment_ccb_get_result(privateAugOmHandle, 
							callback->ccbID);

						osafassert(m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) == NCSCC_RC_SUCCESS);
						locked = true;

						if(augResult != SA_AIS_OK) {
							TRACE("A non-ok result from an augmented CCB %u overrides the reply from "
								"the OI %u on modify-uc", augResult, 
								ccbObjModRpl.info.immnd.info.ccbUpcallRsp.result);

							ccbObjModRpl.info.immnd.info.ccbUpcallRsp.result = augResult;
						}							
					}

					/*async fevs */
					localEr = imma_evt_fake_evs(cb, &ccbObjModRpl, NULL, 0,
						cl_node->handle, &locked, false);
				} else {
					/* callback->inv == 0 means PBE CCB modify upcall, no reply. */
					osafassert(cl_node->isPbe || cl_node->isApplier);
				}

				if (locked) {
					osafassert(m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE) == NCSCC_RC_SUCCESS);
					locked = false;
				}

				if (localEr != NCSCC_RC_SUCCESS) {
					/*Cant do anything but log error and drop this reply. */
					TRACE_3("CcbObjectModifyCallback: send reply to IMMND failed");
				}
			} while (0);
			break;

		case IMMA_CALLBACK_OI_CCB_ABORT:
			TRACE("ccb-abort op callback");
			do {
				/* Anoying type diff for ccbid between OM and OI */
				SaImmOiCcbIdT ccbid = callback->ccbID;
				SaImmHandleT privateAugOmHandle = 0LL;
				if (cl_node->o.iCallbk.saImmOiCcbAbortCallback)	{
					cl_node->o.iCallbk.saImmOiCcbAbortCallback(callback->lcl_imm_hdl, ccbid);
				} else {
					/* No callback function registered for abort upcall.
					   There is nothing we can do about this since the CCB is
					   commited already.
					*/
					TRACE_3("saImmOiCcbAbortCallback is not implemented, yet "
						"implementer is registered and CCBs are used. "
						"Ccb %llu will abort anyway", ccbid);
				}
				osafassert(m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) == NCSCC_RC_SUCCESS);
				if(!imma_oi_ccb_record_close_augment(cl_node, ccbid, &privateAugOmHandle, true)) {
					TRACE_3("CcbAbortCallback: imma_oi_ccb_record_close_augment "
						"returned false for ccb %llu", ccbid);
				}
				imma_oi_ccb_record_terminate(cl_node, ccbid);
				osafassert(m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE) == NCSCC_RC_SUCCESS);
				if(privateAugOmHandle) {
					osafassert(immsv_om_handle_finalize);
					immsv_om_handle_finalize(privateAugOmHandle);
				}
			} while (0);
			break;

		case IMMA_CALLBACK_OI_RT_ATTR_UPDATE:
			TRACE("rt-attr-update callback");
			do {
				SaAisErrorT localEr = SA_AIS_OK;
				uint32_t proc_rc = NCSCC_RC_SUCCESS;
				IMMSV_EVT rtAttrUpdRpl;

				if (cl_node->o.iCallbk.saImmOiRtAttrUpdateCallback) {
					SaImmAttrNameT *attributeNames;
					int noOfAttrNames = 0;
					int i = 0;

					IMMSV_ATTR_NAME_LIST *p = callback->attrNames;
					while (p) {
						++noOfAttrNames;
						p = p->next;
					}
					osafassert(noOfAttrNames);	/*alloc-1 */
					attributeNames = calloc(noOfAttrNames + 1, sizeof(SaImmAttrNameT));
					p = callback->attrNames;
					for (; i < noOfAttrNames; i++, p = p->next) {
						attributeNames[i] = p->name.buf;
					}

					/*attributeNames[noOfAttrNames] = NULL; calloc=> redundant */

					TRACE("Invoking saImmOiRtAttrUpdateCallback");
					localEr = cl_node->o.iCallbk.saImmOiRtAttrUpdateCallback(callback->lcl_imm_hdl,
						&callback->name,
						attributeNames);

					TRACE("saImmOiRtAttrUpdateCallback returned RC:%u", localEr);
					if (!(localEr == SA_AIS_OK ||
						    localEr == SA_AIS_ERR_NO_MEMORY ||
						    localEr == SA_AIS_ERR_NO_RESOURCES || localEr == SA_AIS_ERR_FAILED_OPERATION)) {
						TRACE_2("ERR_FAILED_OPERATION: Illegal return value from "
							"saImmOiRtAttrUpdateCallback %u. "
							"Allowed are %u %u %u %u", localEr, SA_AIS_OK,
							SA_AIS_ERR_NO_MEMORY, SA_AIS_ERR_NO_RESOURCES,
							SA_AIS_ERR_FAILED_OPERATION);
						localEr = SA_AIS_ERR_FAILED_OPERATION;
					}

					free(attributeNames);	/*We do not leak the attr names here because they are still
								  attached to, and deallocated by, the callback structure. */
				} else {
					/* No callback function registered for rt-attr-update upcall.
					   The standard is not clear on how this case should be 
					   handled. We take the strict approach of demanding that, 
					   if there is a registered implementer, then that 
					   implementer must implement the callback, for the
					   callback to succeed.
					*/
					TRACE_2("ERR_FAILED_OPERATION: saImmOiRtAttrUpdateCallback is not implemented, "
						"yet implementer is registered and pure runtime attrs are fetched.");

					localEr = SA_AIS_ERR_FAILED_OPERATION;
				}
				memset(&rtAttrUpdRpl, 0, sizeof(IMMSV_EVT));
				rtAttrUpdRpl.type = IMMSV_EVT_TYPE_IMMND;
				rtAttrUpdRpl.info.immnd.type = IMMND_EVT_A2ND_RT_ATT_UPPD_RSP;
				rtAttrUpdRpl.info.immnd.info.rtAttUpdRpl.result = localEr;
				rtAttrUpdRpl.info.immnd.info.rtAttUpdRpl.sr.client_hdl = callback->lcl_imm_hdl;

				SaInt32T owner = m_IMMSV_UNPACK_HANDLE_HIGH(callback->invocation);
				SaInt32T inv = m_IMMSV_UNPACK_HANDLE_LOW(callback->invocation);

				rtAttrUpdRpl.info.immnd.info.rtAttUpdRpl.sr.searchId = inv;
				rtAttrUpdRpl.info.immnd.info.rtAttUpdRpl.sr.remoteNodeId = owner;

				/*Adding one to get the terminating null sent */
				rtAttrUpdRpl.info.immnd.info.rtAttUpdRpl.sr.objectName.size = callback->name.length + 1;
				/* Only borowing the name string from the SaName in the callback */
				rtAttrUpdRpl.info.immnd.info.rtAttUpdRpl.sr.objectName.buf = (char *)callback->name.value;
				/* Only borowing the attributeNames list from callback. */
				rtAttrUpdRpl.info.immnd.info.rtAttUpdRpl.sr.attributeNames = callback->attrNames;
				rtAttrUpdRpl.info.immnd.info.rtAttUpdRpl.sr.requestNodeId = callback->requestNodeId;
				/* No structures allocated, all pointers borowed => nothing to free. */

				if (cb->is_immnd_up == false) {
					proc_rc = SA_AIS_ERR_NO_RESOURCES;
					TRACE_2("ERR_NO_RESOURCES: IMMND_DOWN");
				} else {
					/* send the reply to the IMMND asyncronously */
					proc_rc = imma_mds_msg_send(cb->imma_mds_hdl, &cb->immnd_mds_dest,
						&rtAttrUpdRpl, NCSMDS_SVC_ID_IMMND);
					if (proc_rc != NCSCC_RC_SUCCESS) {
						/*Cant do anything but log error and drop this reply. */
						TRACE_3("oiRtAttrUpdateCallback: send reply to IMMND failed");
					}
				}
			} while (0);
			break;

		case IMMA_CALLBACK_STALE_HANDLE:
			TRACE("Stale OI handle upcall completed");
			imma_proc_terminate_oi_ccbs(cb, cl_node);
			break;

		default:
			TRACE_3("Unrecognized OI callback type: %u", callback->type);
			break;
	}
#endif   /* ifdef IMMA_OI */

	/* free the callback info. Note - we are not locked. Still should be ok since
	   the callback was dequeue'd by this call. */
	/* Also verify that we free any/all pointer structures that have not 
	   been set to NULL */
	imma_proc_free_callback(callback);
	TRACE_LEAVE();
}

static void imma_proc_free_callback(IMMA_CALLBACK_INFO *callback)
{
	if (!callback)
		return;
	if (callback->params) {
		int i;
		for (i = 0; callback->params[i]; ++i) {
			SaImmAdminOperationParamsT_2 *q = callback->params[i];
			imma_freeAttrValue3(q->paramBuffer, q->paramType);	/*free-4 */

			free(q->paramName);	/*free-3 */
			q->paramName = NULL;
			free(q);	/*free-2 */
		}

		free(callback->params);	/*free-1 */
	}

	if (callback->attrValues) {
		immsv_free_attrvalues_list(callback->attrValues);
		callback->attrValues = NULL;
	}

	if (callback->attrMods) {
		immsv_free_attrmods(callback->attrMods);
		callback->attrMods = NULL;
	}

	if (callback->attrNames) {
		immsv_evt_free_attrNames(callback->attrNames);
		callback->attrNames = NULL;
	}

	free(callback);
}

/*******************************************************************
 * imma_proc_check_stale internal function
 *     Checks if the imma handle has turned stale, e.g. on timeout
 *     return from a syncronous call towards IMMND.
 *     Note the timeout could be a "normal" timeout caused say by an
 *     object-implementer not responding. In that case the input
 *     defaultErr will be returned. But if the client is stale
 *     it indicates that the IMMND crashed during the call and we
 *     return ERR_BAD_HANDLE since the handle can not be recovered.
 * NOTE: The CB must be UNLOCKED on entry of this function!!
 *******************************************************************/
SaAisErrorT imma_proc_check_stale(IMMA_CB *cb, 
	SaImmHandleT immHandle,
	SaAisErrorT defaultEr)
{
	SaAisErrorT err = defaultEr;
	IMMSV_EVT cutSyncCall_evt;
	if (m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) == NCSCC_RC_SUCCESS) {
		IMMA_CLIENT_NODE  *cl_node=0;
		imma_client_node_get(&cb->client_tree, &immHandle, &cl_node);
		if (!cl_node || cl_node->stale) {
			/* We dont set exposed here because we are not exposed yet. */
			err = SA_AIS_ERR_BAD_HANDLE;
			TRACE_3("Client handle turned bad, IMMND restarted ?");
		} else {
			/* Prepare message for truncating this syncronous call (which just 
			   got ERR_TIMEOUT) in local immnd. This drastically reduces the
			   number of accepted retries at new use of this handle for 
			   syncronous calls. Before this timeout, any other attempts
			   to use this handle (by other threads concurrently) is actually
			   not allowed. But after the timeout on this call, renewed use
			   of this handle is allowed. On the client side, the previous
			   syncronous call has now terminated with ERR_TIMEOUT. When
			   the server receives the asyncronous CL_TIMEOUT message sent
			   below, the previous syncronous call can also be terminated in
			   the local immnd server, at its discretion.
			 */
			memset(&cutSyncCall_evt, 0, sizeof(IMMSV_EVT));
			cutSyncCall_evt.type = IMMSV_EVT_TYPE_IMMND;
			cutSyncCall_evt.info.immnd.type = IMMND_EVT_A2ND_CL_TIMEOUT;
			cutSyncCall_evt.info.immnd.info.finReq.client_hdl = cl_node->handle;
		}
		
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);

		if(err == SA_AIS_ERR_TIMEOUT && cb->is_immnd_up) {
			if(imma_mds_msg_send(cb->imma_mds_hdl, &cb->immnd_mds_dest,
				   &cutSyncCall_evt, NCSMDS_SVC_ID_IMMND) != NCSCC_RC_SUCCESS){
				/* Inform local immnd that this client has timed out*/
				TRACE_3("imma_proc_check_stale: asyncronous send failed");
			}
		}
	}

	return err;
}

/*******************************************************************
 * imma_evt_fake_evs internal function
 *
 * NOTE: The CB must be LOCKED on entry of this function!!
 *       It will usually be unlocked on exit, as reflected in the 'locked' 
 *       parameter.
 *******************************************************************/
SaAisErrorT imma_evt_fake_evs(IMMA_CB *cb,
	IMMSV_EVT *i_evt,
	IMMSV_EVT **o_evt,
	uint32_t timeout, SaImmHandleT immHandle, bool *locked, bool checkWritable)
{
	SaAisErrorT rc = SA_AIS_OK;
	IMMSV_EVT fevs_evt;
	uint32_t proc_rc;
	char *tmpData = NULL;
	NCS_UBAID uba;
	uba.start = NULL;

	osafassert(locked && (*locked));
	/*Pack the message for sending over multiple hops. */

	if (ncs_enc_init_space(&uba) != NCSCC_RC_SUCCESS) {
		uba.start = NULL;
		TRACE_2("ERR_LIBRARY: Failed init ubaid");
		return SA_AIS_ERR_LIBRARY;
	}

	/* Encode non-flat since we broadcast to unknown receivers. */
	rc = immsv_evt_enc(i_evt, &uba);

	if (rc != NCSCC_RC_SUCCESS) {
		TRACE_2("ERR_LIBRARY: Failed to pre-pack");
		return SA_AIS_ERR_LIBRARY;
	}

	int32_t size = uba.ttl;
	/*NOTE: should check against "payload max-size" */

	tmpData = malloc(size);
	char *data = m_MMGR_DATA_AT_START(uba.start, size, tmpData);

	memset(&fevs_evt, 0, sizeof(IMMSV_EVT));
	fevs_evt.type = IMMSV_EVT_TYPE_IMMND;
	if(i_evt->info.immnd.type == IMMND_EVT_A2ND_OBJ_SYNC_2) {
		fevs_evt.info.immnd.type = IMMND_EVT_A2ND_IMM_FEVS_2;
		fevs_evt.info.immnd.info.fevsReq.isObjSync = 0x1;
	} else {
		fevs_evt.info.immnd.type = IMMND_EVT_A2ND_IMM_FEVS;
		fevs_evt.info.immnd.info.fevsReq.isObjSync = 0x0;

		if((i_evt->info.immnd.type == IMMND_EVT_A2ND_IMM_ADMOP) ||
			(i_evt->info.immnd.type == IMMND_EVT_A2ND_IMM_ADMOP_ASYNC)) {
			/* Overloaded use of sender_count IMMA->IMMND we use values 
			   larger than 1 as a marker for admop to pre-register
			   request continuation, before forwarding request over fevs
			   (see ticket #1690). This is to avoid the rare case of the
			   reply, which does not use fevs, bypassing the request at
			   the client processor.
			*/
			osafassert(!checkWritable);
			SaInvocationT saInv = 
				m_IMMSV_PACK_HANDLE(i_evt->info.immnd.info.admOpReq.adminOwnerId,
					i_evt->info.immnd.info.admOpReq.invocation);

			osafassert(saInv > 1);
			fevs_evt.info.immnd.info.fevsReq.sender_count = (SaUint64T) saInv;
		}
	}

	fevs_evt.info.immnd.info.fevsReq.client_hdl = immHandle;

	if(checkWritable) {
		/*Overloaded use of sender_count IMMA->IMMND we use the value
		  of 1 as a marker if imm writability should be checked before 
		  forwarding request over fevs. This is a pure performance 
		  enhancing feature.
		*/
		fevs_evt.info.immnd.info.fevsReq.sender_count = 0x1;
	}

	fevs_evt.info.immnd.info.fevsReq.msg.size = size;
	fevs_evt.info.immnd.info.fevsReq.msg.buf = data;

	/* Unlock before MDS Send */
	m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
	*locked = false;

	/* IMMND GOES DOWN */
	if (cb->is_immnd_up == false) {
		rc = SA_AIS_ERR_TRY_AGAIN;
		TRACE_2("ERR_TRY_AGAIN: IMMND is DOWN");
		goto fail;
	}

	if (o_evt) {
		/* Send the evt to IMMND syncronously (reply expected) */
		proc_rc = imma_mds_msg_sync_send(cb->imma_mds_hdl, &(cb->immnd_mds_dest), &fevs_evt, o_evt, timeout);
	} else {
		/*Send evt to IMMND asyncronously, no reply expected. */
		osafassert(timeout == 0);
		proc_rc = imma_mds_msg_send(cb->imma_mds_hdl, &(cb->immnd_mds_dest), &fevs_evt, NCSMDS_SVC_ID_IMMND);
	}

	/* Generate rc from proc_rc */
	switch (proc_rc) {
		case NCSCC_RC_SUCCESS:
			break;
		case NCSCC_RC_REQ_TIMOUT:
			osafassert(o_evt); /* timeout has to be on syncronous. */
			rc = imma_proc_check_stale(cb, immHandle, SA_AIS_ERR_TIMEOUT);
			break;

		default:
			rc = SA_AIS_ERR_LIBRARY;
			TRACE_1("ERR_LIBRARY: MDS returned unexpected error code %u", proc_rc);
			break;
	}

 fail:

	if (tmpData) {
		free(tmpData);
		tmpData = NULL;
	}

	if (uba.start) {
		m_MMGR_FREE_BUFR_LIST(uba.start);
	}

	return rc;
}

void
imma_proc_increment_pending_reply(IMMA_CLIENT_NODE *cl_node)
{
	if (cl_node->replyPending < 0xff) {
		cl_node->replyPending++;
	} else {
		TRACE_3("More than 255 concurrent PENDING replies on handle!");
	}
}

void
imma_proc_decrement_pending_reply(IMMA_CLIENT_NODE *cl_node)
{
	if (cl_node->replyPending) {
		if (cl_node->replyPending < 0xff) {
			cl_node->replyPending--;
		} else {
			/* If reply Pending has reached 255 then we stop keeping track.
			   The consequence is: in case of IMMND restart we will not be
			   able to resurrect this handle. The client will be forced to
			   deal with an SA_AIS_ERR_BAD_HANDLE.
			 */
			TRACE_3("Lost track of concurrent pending replies on handle %llx.",
				cl_node->handle);
		}
	} else {
		/* Reaching 255 is sticky. */
		TRACE_3("Will not decrement zero pending reply count for handle %llx",
			cl_node->handle);
		cl_node->replyPending = 0xff;
	}
}


/**
 * Validate a value type
 * @param theType
 * 
 * @return int
 */
int imma_proc_is_valid_type(const SaImmValueTypeT theType)
{
	switch (theType) {
		case SA_IMM_ATTR_SAINT32T:
		case SA_IMM_ATTR_SAUINT32T:
		case SA_IMM_ATTR_SAINT64T:
		case SA_IMM_ATTR_SAUINT64T:
		case SA_IMM_ATTR_SATIMET:
		case SA_IMM_ATTR_SAFLOATT:
		case SA_IMM_ATTR_SADOUBLET:
		case SA_IMM_ATTR_SANAMET:
		case SA_IMM_ATTR_SASTRINGT:
		case SA_IMM_ATTR_SAANYT:
			return 1;
		default:
			return 0;
	}
}

/**
 * Validate admin op params
 * @param params
 * 
 * @return int
 */
int imma_proc_is_adminop_params_valid(const SaImmAdminOperationParamsT_2 **params)
{
	int i = 0;

	/* Validate type for parameters */
	while (params[i] != NULL) {
		if (params[i]->paramName == NULL) {
			TRACE_3("no name for param: %d", i);
			return 0;
		}

		if (!imma_proc_is_valid_type(params[i]->paramType)) {
			TRACE_3("wrong type for param: %s", params[i]->paramName);
			return 0;
		}

		if (params[i]->paramBuffer == NULL) {
			TRACE_3("no value for param: %d", i);
			return 0;
		}

		i++;
	}

	return 1;
}
