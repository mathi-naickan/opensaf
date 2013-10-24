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
  
  This file contains the IMMSv SAF API definitions.

TRACE GUIDE:
 Policy is to not use logging/syslog from library code.
 Only the trace part of logtrace is used from library. 

 It is possible to turn on trace for the IMMA library used
 by an application process. This is done by the application 
 defining the environment variable: IMMA_TRACE_PATHNAME.
 The trace will end up in the file defined by that variable.

 TRACE   debug traces                 - aproximates DEBUG  
 TRACE_1 normal but important events  - aproximates INFO.
 TRACE_2 user errors with return code - aproximates NOTICE.
 TRACE_3 unusual or strange events    - aproximates WARNING
 TRACE_4 library errors ERR_LIBRARY   - aproximates ERROR
*****************************************************************************/

#include "imma.h"
#include "immsv_api.h"

static const char *sysaClName = SA_IMM_ATTR_CLASS_NAME;
static const char *sysaAdmName = SA_IMM_ATTR_ADMIN_OWNER_NAME;
static const char *sysaImplName = SA_IMM_ATTR_IMPLEMENTER_NAME;

static int imma_oi_resurrect(IMMA_CB *cb, IMMA_CLIENT_NODE *cl_node, bool *locked, SaAisErrorT * err_cli_res);

/****************************************************************************
  Name          :  SaImmOiInitialize
 
  Description   :  This function initializes the IMM OI Service for the
                   invoking process and registers the callback functions.
                   The handle 'immOiHandle' is returned as the reference to
                   this association between the process and the ImmOi Service.
                   
 
  Arguments     :  immOiHandle -  A pointer to the handle designating this 
                                particular initialization of the IMM OI
                                service that is to be returned by the 
                                ImmOi Service.
                   immOiCallbacks  - Pointer to a SaImmOiCallbacksT structure, 
                                containing the callback functions of the
                                process that the ImmOi Service may invoke.
                   version    - Is a pointer to the version of the Imm
                                Service that the invoking process is using.
 
  Return Values :  Refer to SAI-AIS specification for various return values.
 
  Notes         :
******************************************************************************/
SaAisErrorT saImmOiInitialize_2(SaImmOiHandleT *immOiHandle,
				const SaImmOiCallbacksT_2 *immOiCallbacks, SaVersionT *inout_version)
{
	IMMA_CB *cb = &imma_cb;
	SaAisErrorT rc = SA_AIS_OK;
	IMMSV_EVT init_evt;
	IMMSV_EVT *out_evt = NULL;
	uint32_t proc_rc = NCSCC_RC_SUCCESS;
	IMMA_CLIENT_NODE *cl_node = 0;
	bool locked = true;
	SaVersionT requested_version;
	char *timeout_env_value = NULL;

	TRACE_ENTER();

	proc_rc = imma_startup(NCSMDS_SVC_ID_IMMA_OI);
	if (NCSCC_RC_SUCCESS != proc_rc) {
		TRACE_4("ERR_LIBRARY: Agents_startup failed");
		TRACE_LEAVE();
		return SA_AIS_ERR_LIBRARY;
	}

	if ((!immOiHandle) || (!inout_version)) {
		TRACE_2("ERR_INVALID_PARAM: immOiHandle is NULL or version is NULL");
		rc = SA_AIS_ERR_INVALID_PARAM;
		goto end;
	}

        requested_version = (*inout_version);

	if (false == cb->is_immnd_up) {
		TRACE_2("ERR_TRY_AGAIN: IMMND is DOWN");
		rc = SA_AIS_ERR_TRY_AGAIN;
		goto end;
	}

	*immOiHandle = 0;

	/* Alloc the client info data structure & put it in the Pat tree */
	cl_node = (IMMA_CLIENT_NODE *)calloc(1, sizeof(IMMA_CLIENT_NODE));
	if (cl_node == NULL) {
		TRACE_2("ERR_NO_MEMORY: IMMA_CLIENT_NODE alloc failed");
		rc = SA_AIS_ERR_NO_MEMORY;
		goto cnode_alloc_fail;
	}

	cl_node->isOm = false;

	if ((requested_version.releaseCode == 'A') &&
		(requested_version.majorVersion == 0x02) &&
		(requested_version.minorVersion >= 0x0b)) {
		TRACE_2("OI client version A.2.11");
		cl_node->isImmA2b = 0x1;
	}

	if((timeout_env_value = getenv("IMMA_SYNCR_TIMEOUT"))!=NULL) {
		cl_node->syncr_timeout = atoi(timeout_env_value);
		TRACE_2("IMMA library syncronous timeout set to:%u", cl_node->syncr_timeout);
	}

	if(cl_node->syncr_timeout < NCS_SAF_MIN_ACCEPT_TIME) {
		cl_node->syncr_timeout = IMMSV_WAIT_TIME; /* Default */
	}

	/* Take the CB Lock */
	if (m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
		TRACE_4("ERR_LIBRARY: LOCK failed");
		rc = SA_AIS_ERR_LIBRARY;
		goto lock_fail;
	}
	/* locked == true already */

	/* Draft Validations : Version this is the politically correct validatioin
	   distinct from the pragmatic validation we do above. */
	rc = imma_version_validate(inout_version);
	if (rc != SA_AIS_OK) {
		TRACE_2("ERR_VERSION");
		goto version_fail;
	}

	/* Store the callback functions, if set */
	if (immOiCallbacks) {
		cl_node->o.iCallbk = *immOiCallbacks;
	}

	proc_rc = imma_callback_ipc_init(cl_node);

	if (proc_rc != NCSCC_RC_SUCCESS) {
		rc = SA_AIS_ERR_LIBRARY;
		/* ALready log'ed by imma_callback_ipc_init */
		goto ipc_init_fail;
	}

	/* populate the EVT structure */
	memset(&init_evt, 0, sizeof(IMMSV_EVT));
	init_evt.type = IMMSV_EVT_TYPE_IMMND;
	init_evt.info.immnd.type = IMMND_EVT_A2ND_IMM_OI_INIT;
	init_evt.info.immnd.info.initReq.version = requested_version;
	init_evt.info.immnd.info.initReq.client_pid = getpid();

	/* Release the CB lock Before MDS Send */
	m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
	locked = false;

	/* cl_node has not been added to tree in cb yet so safe to access 
	   without cb-lock.
	 */	

	if (false == cb->is_immnd_up) {
		rc = SA_AIS_ERR_TRY_AGAIN;
		TRACE_2("ERR_TRY_AGAIN: IMMND is DOWN");
		goto mds_fail;
	}

	/* send the request to the IMMND */
	proc_rc = imma_mds_msg_sync_send(cb->imma_mds_hdl, &cb->immnd_mds_dest, &init_evt, &out_evt, 
		cl_node->syncr_timeout);

	/* Error Handling */
	switch (proc_rc) {
		case NCSCC_RC_SUCCESS:
			break;
		case NCSCC_RC_REQ_TIMOUT:
			rc = SA_AIS_ERR_TIMEOUT;
			goto mds_fail;
		default:
			TRACE_4("ERR_LIBRARY: MDS returned unexpected error code %u", proc_rc);
			rc = SA_AIS_ERR_LIBRARY;
			goto mds_fail;
	}

	if (out_evt) {
		rc = out_evt->info.imma.info.initRsp.error;
		if (rc != SA_AIS_OK) {
			goto rsp_not_ok;
		}

		/* Take the CB lock after MDS Send */
		if (m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
			TRACE_4("ERR_LIBRARY: LOCK failed");
			rc = SA_AIS_ERR_LIBRARY;
			goto lock_fail1;
		} 

		locked = true;

		if (false == cb->is_immnd_up) {
			/*IMMND went down during THIS call! */
			rc = SA_AIS_ERR_TRY_AGAIN;
			TRACE_2("ERR_TRY_AGAIN: IMMND is DOWN");
			goto rsp_not_ok;
		}

		cl_node->handle = out_evt->info.imma.info.initRsp.immHandle;

		TRACE_1("Trying to add OI client id:%u node:%x handle:%llx",
			m_IMMSV_UNPACK_HANDLE_HIGH(cl_node->handle),
			m_IMMSV_UNPACK_HANDLE_LOW(cl_node->handle), cl_node->handle);
		proc_rc = imma_client_node_add(&cb->client_tree, cl_node);
		if (proc_rc != NCSCC_RC_SUCCESS) {
			IMMA_CLIENT_NODE *stale_node = NULL;
			imma_client_node_get(&cb->client_tree, &(cl_node->handle), &stale_node);

			if ((stale_node != NULL) && stale_node->stale) {
				TRACE_1("Removing stale client");
				imma_finalize_client(cb, stale_node);
				proc_rc = imma_shutdown(NCSMDS_SVC_ID_IMMA_OI);
				if (proc_rc != NCSCC_RC_SUCCESS) {
					TRACE_4("ERR_LIBRARY: Call to imma_shutdown FAILED");
					rc = SA_AIS_ERR_LIBRARY;
					goto node_add_fail;
				}
				TRACE_1("Retrying add of client node");
				proc_rc = imma_client_node_add(&cb->client_tree, cl_node);
			}
			
			if (proc_rc != NCSCC_RC_SUCCESS) {
				rc = SA_AIS_ERR_LIBRARY;
				TRACE_4("ERR_LIBRARY: client_node_add failed");
				goto node_add_fail;
			}
		}
	} else {
		TRACE_4("ERR_LIBRARY: Empty reply received");
		rc = SA_AIS_ERR_LIBRARY;
	}

	/*Error handling */
 node_add_fail:
 lock_fail1:
	if (rc != SA_AIS_OK) {
		IMMSV_EVT finalize_evt, *out_evt1;

		out_evt1 = NULL;
		memset(&finalize_evt, 0, sizeof(IMMSV_EVT));
		finalize_evt.type = IMMSV_EVT_TYPE_IMMND;
		finalize_evt.info.immnd.type = IMMND_EVT_A2ND_IMM_OI_FINALIZE;
		finalize_evt.info.immnd.info.finReq.client_hdl = cl_node->handle;

		if (locked) {
			m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
			locked = false;
		}

		/* send the request to the IMMND */
		imma_mds_msg_sync_send(cb->imma_mds_hdl, &(cb->immnd_mds_dest),
			&finalize_evt, &out_evt1, cl_node->syncr_timeout);
		if (out_evt1) {
			free(out_evt1);
		}
	}

 rsp_not_ok:
 mds_fail:

	/* Free the IPC initialized for this client */
	if (rc != SA_AIS_OK)
		imma_callback_ipc_destroy(cl_node);

 ipc_init_fail:
 version_fail:

	if (locked)
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
	if (out_evt) {
		free(out_evt);
	}

 lock_fail:
	if (rc != SA_AIS_OK)
		free(cl_node);

 cnode_alloc_fail:

	if (rc == SA_AIS_OK) {
		/* Went well, return immHandle to the application */
		*immOiHandle = cl_node->handle;
	}

 end:
	if (rc != SA_AIS_OK) {
		if (NCSCC_RC_SUCCESS != imma_shutdown(NCSMDS_SVC_ID_IMMA_OI)) {
			/* Oh boy. Failure in imma_shutdown when we already have
			   some other problem. */
			TRACE_4("ERR_LIBRARY: Call to imma_shutdown failed, prior error %u", rc);
			rc = SA_AIS_ERR_LIBRARY;
		}
	}
	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
  Name          :  saImmOiSelectionObjectGet
 
  Description   :  This function returns the operating system handle 
                   associated with the immOiHandle.
 
  Arguments     :  immOiHandle - Imm OI service handle.
                   selectionObject - Pointer to the operating system handle.
 
  Return Values :  Refer to SAI-AIS specification for various return values.
 
  Notes         :
******************************************************************************/
SaAisErrorT saImmOiSelectionObjectGet(SaImmOiHandleT immOiHandle, SaSelectionObjectT *selectionObject)
{
	SaAisErrorT rc = SA_AIS_OK;
	IMMA_CB *cb = &imma_cb;
	IMMA_CLIENT_NODE *cl_node = 0;
	bool locked = true;
	TRACE_ENTER();

	if (!selectionObject)
		return SA_AIS_ERR_INVALID_PARAM;

	if (cb->sv_id == 0) {
		TRACE_2("ERR_BAD_HANDLE: No initialized handle exists!");
		return SA_AIS_ERR_BAD_HANDLE;
	}

	if (cb->is_immnd_up == false) {
		/* Normally this call will not go remote. But if IMMND is down, 
		   then it is highly likely that immOiHandle is stale marked. 
		   The reactive resurrect will fail as long as IMMND is down. 
		*/
		TRACE_2("ERR_TRY_AGAIN: IMMND_DOWN");
		return SA_AIS_ERR_TRY_AGAIN;
	}

	*selectionObject = (-1);	/* Ensure non valid descriptor in case of failure. */

	/* Take the CB lock */
	if (m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
		TRACE_4("ERR_LIBRARY: LOCK failed");
		rc = SA_AIS_ERR_LIBRARY;
		goto lock_fail;
	}
	/* locked == true already */

	imma_client_node_get(&cb->client_tree, &immOiHandle, &cl_node);

	if (!cl_node || cl_node->isOm) {
		TRACE_2("ERR_BAD_HANDLE: Bad handle %llx", immOiHandle);
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto node_not_found;
	}

	if (cl_node->stale) {
		TRACE_1("Handle %llx is stale", immOiHandle);
		bool resurrected = imma_oi_resurrect(cb, cl_node, &locked, &rc);
		if (rc == SA_AIS_ERR_TRY_AGAIN) {
			goto resurrect_failed;
		} 

		if (!locked && m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
			TRACE_4("ERR_LIBRARY: LOCK failed");
			rc = SA_AIS_ERR_LIBRARY;
			goto lock_fail;
		}
		locked = true;

		imma_client_node_get(&cb->client_tree, &immOiHandle, &cl_node);

		if (!resurrected || !cl_node || cl_node->isOm || cl_node->stale) {
			TRACE_2("ERR_BAD_HANDLE: Reactive ressurect of handle %llx failed",
				immOiHandle);
			if (cl_node && cl_node->stale) {cl_node->exposed = true;}
			rc = SA_AIS_ERR_BAD_HANDLE;
			goto resurrect_failed;
		}

		TRACE_1("Reactive resurrect of handle %llx succeeded", immOiHandle);
	}


	*selectionObject = (SaSelectionObjectT)
		m_GET_FD_FROM_SEL_OBJ(m_NCS_IPC_GET_SEL_OBJ(&cl_node->callbk_mbx));

	cl_node->selObjUsable = true;

 node_not_found:
 resurrect_failed:
	if (locked)
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);

 lock_fail:
	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
  Name          :  saImmOiDispatch
 
  Description   :  This function invokes, in the context of the calling
                   thread, pending callbacks for the handle immOiHandle in a 
                   way that is specified by the dispatchFlags parameter.
 
  Arguments     :  immOiHandle - IMM OI Service handle
                   dispatchFlags - Flags that specify the callback execution
                                   behaviour of this function.
 
  Return Values :  Refer to SAI-AIS specification for various return values.
 
  Notes         :
******************************************************************************/
SaAisErrorT saImmOiDispatch(SaImmOiHandleT immOiHandle, SaDispatchFlagsT dispatchFlags)
{
	SaAisErrorT rc = SA_AIS_OK;
	IMMA_CB *cb = &imma_cb;
	IMMA_CLIENT_NODE *cl_node = 0;
	bool locked = false;
	uint32_t pend_fin = 0;
   	uint32_t pend_dis = 0;
	TRACE_ENTER();

	if (cb->sv_id == 0) {
		TRACE_2("ERR_BAD_HANDLE: No initialized handle exists!");
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto fail;
	}

	if (m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
		TRACE_4("ERR_LIBRARY: LOCK failed");
		rc = SA_AIS_ERR_LIBRARY;
		goto fail;
	}
	locked = true;

	/* get the client_info */
	imma_client_node_get(&cb->client_tree, &immOiHandle, &cl_node);
	if (!cl_node || cl_node->isOm) {
		TRACE_2("ERR_BAD_HANDLE: client_node_get failed");
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto fail;
	}

	if (cl_node->stale) {
		TRACE_1("Handle %llx is stale, trying to resurrect it.", immOiHandle);

		if (cb->dispatch_clients_to_resurrect == 0) {
			rc = SA_AIS_ERR_BAD_HANDLE;
			cl_node->exposed = true;
			goto fail;
		} 

		--(cb->dispatch_clients_to_resurrect);
		TRACE_1("Remaining clients to acively resurrect: %d",
			cb->dispatch_clients_to_resurrect);

		if (!imma_oi_resurrect(cb, cl_node, &locked, &rc)) {
			if(rc != SA_AIS_ERR_TRY_AGAIN) {
				TRACE_2("ERR_BAD_HANDLE: Failed to resurrect stale OI handle <c:%u, n:%x>",
					m_IMMSV_UNPACK_HANDLE_HIGH(immOiHandle),
					m_IMMSV_UNPACK_HANDLE_LOW(immOiHandle));
				rc = SA_AIS_ERR_BAD_HANDLE;
			}
			goto fail;
		}

		TRACE_1("Successfully resurrected OI handle <c:%u, n:%x>",
			m_IMMSV_UNPACK_HANDLE_HIGH(immOiHandle),
			m_IMMSV_UNPACK_HANDLE_LOW(immOiHandle));

		if (!locked && m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS){
			TRACE_4("ERR_LIBRARY: Lock failure");
			rc = SA_AIS_ERR_LIBRARY;
			goto fail;
		}
		locked = true;

		/* get the client again. */
		imma_client_node_get(&cb->client_tree, &immOiHandle, &cl_node);
		if (!cl_node|| cl_node->isOm)
		{
			TRACE_2("ERR_BAD_HANDLE: client_node_get failed AFTER successfull  resurrect!");
			rc = SA_AIS_ERR_BAD_HANDLE;
			goto fail;
		}

		if (cl_node->stale) {
			TRACE_2("ERR_BAD_HANDLE: client became stale AGAIN after successfull resurrect!");
			cl_node->exposed = true;
			rc = SA_AIS_ERR_BAD_HANDLE;
			goto fail;
		}

		cl_node->selObjUsable = true; /* success */
	}

	/* Back to normal case of non stale (possibly resurrected) handle. */
	/* Unlock & do the dispatch to avoid deadlock in arrival callback. */
	if (locked) {
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
		locked = false;
	}
	cl_node = NULL; /*Prevent unsafe use.*/
	/* unlocked */

	/* Increment Dispatch usgae count */
	cb->pend_dis++;
	
	switch (dispatchFlags) {
		case SA_DISPATCH_ONE:
			rc = imma_hdl_callbk_dispatch_one(cb, immOiHandle);
			break;

		case SA_DISPATCH_ALL:
			rc = imma_hdl_callbk_dispatch_all(cb, immOiHandle);
			break;

		case SA_DISPATCH_BLOCKING:
			rc = imma_hdl_callbk_dispatch_block(cb, immOiHandle);
			break;

		default:
			rc = SA_AIS_ERR_INVALID_PARAM;
			break;
	}/* switch */

	/* Decrement Dispatch usage count */
 	cb->pend_dis--;

	/* can't use cb after we do agent shutdown, so copy all counts */
   	pend_dis = cb->pend_dis;
   	pend_fin = cb->pend_fin;

 fail:
	if (locked)
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);

	/* see if we are still in any dispatch context */ 
	if(pend_dis == 0) {
		while(pend_fin != 0)
		{
			/* call agent shutdown,for each finalize done before */
			cb->pend_fin --;
			imma_shutdown(NCSMDS_SVC_ID_IMMA_OI);
			pend_fin--;
		}
	}
	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
  Name          :  saImmOiFinalize
 
  Description   :  This function closes the association, represented by
                   immOiHandle, between the invoking process and the IMM OI
                   service.
 
  Arguments     :  immOiHandle - IMM OI Service handle.
 
  Return Values :  Refer to SAI-AIS specification for various return values.
 
  Notes         :
******************************************************************************/
SaAisErrorT saImmOiFinalize(SaImmOiHandleT immOiHandle)
{
	SaAisErrorT rc = SA_AIS_OK;
	IMMA_CB *cb = &imma_cb;
	IMMSV_EVT finalize_evt;
	IMMSV_EVT *out_evt = NULL;
	IMMA_CLIENT_NODE *cl_node = 0;
	uint32_t proc_rc = NCSCC_RC_SUCCESS;
	bool locked = true;
	bool agent_flag = false; /* flag = false, we should not call agent shutdown */
	SaUint32T timeout = 0;
	TRACE_ENTER();


	if (cb->sv_id == 0) {
		TRACE_2("ERR_BAD_HANDLE: No initialized handle exists!");
		return SA_AIS_ERR_BAD_HANDLE;
	}

	/* No check for immnd_up here because this is finalize, see below. */

	if (m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
		TRACE_4("ERR_LIBRARY: LOCK failed");
		rc = SA_AIS_ERR_LIBRARY;
		goto lock_fail;
	}
	/* locked == true already */

	/* get the client_info */
	imma_client_node_get(&cb->client_tree, &immOiHandle, &cl_node);
	if (!cl_node || cl_node->isOm) {
		rc = SA_AIS_ERR_BAD_HANDLE;
		TRACE_2("ERR_BAD_HANDLE: client_node_get failed");
		goto node_not_found;
	}

	/*Increment before stale check to get uniform stale handling 
	  before and after send (see stale_handle:)
	*/
	imma_proc_increment_pending_reply(cl_node);

	if (cl_node->stale) {
		TRACE_1("Handle %llx is stale", immOiHandle);
		rc = SA_AIS_OK;	/* Dont punish the client for closing stale handle */
		/* Dont try to resurrect since this is finalize. */
		cl_node->exposed = true;
		goto stale_handle;
	}

	timeout = cl_node->syncr_timeout;

	/* populate the structure */
	memset(&finalize_evt, 0, sizeof(IMMSV_EVT));
	finalize_evt.type = IMMSV_EVT_TYPE_IMMND;
	finalize_evt.info.immnd.type = IMMND_EVT_A2ND_IMM_OI_FINALIZE;
	finalize_evt.info.immnd.info.finReq.client_hdl = cl_node->handle;

	/* Unlock before MDS Send */
	m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
	locked = false;
	cl_node = NULL; /* avoid unsafe use */

	if (cb->is_immnd_up == false) {
		TRACE_3("IMMND is DOWN");
		/* IF IMMND IS DOWN then we know handle is stale. 
		   Since this is a handle finalize, we simply discard the handle.
		   No error return!
		 */
		goto stale_handle;
	}

	/* send the request to the IMMND */
	proc_rc = imma_mds_msg_sync_send(cb->imma_mds_hdl, &(cb->immnd_mds_dest),
					 &finalize_evt, &out_evt, timeout);

	/* MDS error handling */
	switch (proc_rc) {
		case NCSCC_RC_SUCCESS:
			break;
		case NCSCC_RC_REQ_TIMOUT:
			TRACE_3("Got ERR_TIMEOUT in saImmOiFinalize - ignoring");
			/* Yes could be a stale handle, but this is handle finalize.
			   Dont cause unnecessary problems by returning an error code. 
			   If this is a true timeout caused by an unusually sluggish but
			   up IMMND, then this connection at the IMMND side may linger,
			   but on this IMMA side we will drop it. 
			*/
			goto stale_handle;

		default:
			TRACE_4("ERR_LIBRARY: MDS returned unexpected error code %u", proc_rc);
			rc = SA_AIS_ERR_LIBRARY;
			/* We lose the pending reply count in this case but ERR_LIBRARY dominates. */
			goto mds_send_fail; 
	}

	/* Read the received error (if any)  */
	if (out_evt) {
		rc = out_evt->info.imma.info.errRsp.error;
		free(out_evt);
		out_evt = NULL;
		if(rc == SA_AIS_ERR_BAD_HANDLE) {
			rc = SA_AIS_OK;
			/* Client was already gone in local immnd server, but client node
			   still present in the imma library. This can happen as a result
			   of timeout on syncronous downcall. Continue the finalize here 
			   in the library to deallocate the client-node in the library.
			   No need to disturb the client doing this finalize, return OK.
			*/
		}
		
	} else {
		/* rc = SA_AIS_ERR_LIBRARY
		   This is a finalize, no point in disturbing the user with
		   a communication error. 
		*/
		TRACE_3("Received empty reply from server");
	}

 stale_handle:
	/* Do the finalize processing at IMMA */
	if (rc == SA_AIS_OK) {
		/* Take the CB lock  */
		if (!locked && m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != 
			NCSCC_RC_SUCCESS) {
			rc = SA_AIS_ERR_LIBRARY;
			/* Losing track of the pending reply count, but ERR_LIBRARY dominates*/
			TRACE_4("ERR_LIBRARY: LOCK failed");
			goto lock_fail1;
		}

		locked = true;

		/* get the client_info */
		imma_client_node_get(&cb->client_tree, &immOiHandle, &cl_node);
		if (!cl_node || cl_node->isOm) {
			/*rc = SA_AIS_ERR_BAD_HANDLE; This is finalize*/
			TRACE_3("client_node_get failed");
			goto node_not_found;
		}

		if (cl_node->stale) {
			TRACE_1("Handle %llx is stale", immOiHandle);
			/*Dont punish the client for closing stale handle rc == SA_AIS_OK */

			cl_node->exposed = true; /* But dont resurrect it either. */
		}

		imma_proc_decrement_pending_reply(cl_node);
		imma_finalize_client(cb, cl_node);

		/* Finalize the environment */  
		if ( cb->pend_dis == 0) {
			agent_flag = true;
		} else if(cb->pend_dis > 0) {
			cb->pend_fin++;
		}
	}

 lock_fail1:
 mds_send_fail:
 node_not_found:
	if (locked)
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);

 lock_fail:
	if (rc == SA_AIS_OK) {
		
		/* we are not in any dispatch context, we can do agent shutdown */
		if(agent_flag == true) 
			if (NCSCC_RC_SUCCESS != imma_shutdown(NCSMDS_SVC_ID_IMMA_OI)) {
				TRACE_4("ERR_LIBRARY: Call to imma_shutdown failed");
				rc = SA_AIS_ERR_LIBRARY;
			}
	}
	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
  Name          :  saImmOiAdminOperationResult/_o2
 
  Description   :  Send the result for an admin-op, supposedly invoked inside 
                   the upcall for an admin op. 
                   This is normally a NON blocking call (except when resurrecting client)
 
  Arguments     :  immOiHandle - Same as A.02.01 spec.
                   invocation  - Same as A.02.01 spec.
                   result      - Same as A.02.01 spec.
                   returnParams- Parameters to return with the reply (new for A.2.11)
 
  Return Values :  Refer to SAI-AIS specification for various possible return codes.
                   No new return codes for A.2.11 version, except SA_AIS_ERR_VERSION
                   for the case when saImmOiAdminOperationResult_o2 is used but
                   not with A.2.11 set for the immOihandle.
 
******************************************************************************/
static SaAisErrorT admin_op_result_common(
					  SaImmOiHandleT immOiHandle,
					  SaInvocationT invocation,
					  SaAisErrorT result,
					  const SaImmAdminOperationParamsT_2 **returnParams,
					  bool isA2bCall);

SaAisErrorT saImmOiAdminOperationResult(
					SaImmOiHandleT immOiHandle, 
					SaInvocationT invocation,
					SaAisErrorT result)
{
	return admin_op_result_common(immOiHandle, invocation, result, NULL, false);
}

SaAisErrorT saImmOiAdminOperationResult_o2(
					   SaImmOiHandleT immOiHandle,
					   SaInvocationT invocation,
					   SaAisErrorT result,
					   const SaImmAdminOperationParamsT_2 **returnParams)
{
	return admin_op_result_common(immOiHandle, invocation, result, returnParams, true);
}

static SaAisErrorT admin_op_result_common(
					  SaImmOiHandleT immOiHandle,
					  SaInvocationT invocation,
					  SaAisErrorT result,
					  const SaImmAdminOperationParamsT_2 **returnParams,
					  bool isA2bCall)
{
	SaAisErrorT rc = SA_AIS_OK;
	IMMA_CB *cb = &imma_cb;
	IMMSV_EVT adminOpRslt_evt;
	IMMA_CLIENT_NODE *cl_node = 0;
	uint32_t proc_rc = NCSCC_RC_SUCCESS;
	bool locked = true;
	bool errStringPar = false;
	TRACE_ENTER();

	if (cb->sv_id == 0) {
		/* Error return on AdminOperationResult => No reply is sent from OI.
		   The OI is typically a server. Log this error instead of trace,
		   to simplify troubleshooting for the OI maintainer.
		 */
		LOG_IN("ERR_BAD_HANDLE: No initialized handle exists!");
		TRACE_LEAVE();
		return SA_AIS_ERR_BAD_HANDLE;
	}

	if (cb->is_immnd_up == false) {
		TRACE_2("ERR_TRY_AGAIN: IMMND_DOWN");
		TRACE_LEAVE();
		return SA_AIS_ERR_TRY_AGAIN;
	}

	if (m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
		/* Log this error instead of trace, to simplify troubleshooting
		   for the OI maintainer. */
		LOG_IN("ERR_LIBRARY: LOCK failed");
		rc = SA_AIS_ERR_LIBRARY;
		goto lock_fail;
	}
	/* locked == true already */

	/* get the client_info */
	imma_client_node_get(&cb->client_tree, &immOiHandle, &cl_node);
	if (!cl_node || cl_node->isOm) {
		rc = SA_AIS_ERR_BAD_HANDLE;
		/* Log this error instead of trace, to simplify troubleshooting
		   for the OI maintainer. */
		LOG_IN("ERR_BAD_HANDLE: client_node_get failed");
		goto node_not_found;
	}

	if (cl_node->stale) {
		TRACE_1("Handle %llx is stale", immOiHandle);
		bool resurrected = imma_oi_resurrect(cb, cl_node, &locked, &rc);
		if(rc == SA_AIS_ERR_TRY_AGAIN) {
			osafassert(!resurrected);
			goto stale_handle; 
		}

		if (!locked && m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
			/* Log this error instead of trace, to simplify troubleshooting
			   for the OI maintainer. */
			LOG_IN("ERR_LIBRARY: LOCK failed");
			rc = SA_AIS_ERR_LIBRARY;
			goto lock_fail;
		}
		locked = true;

		imma_client_node_get(&cb->client_tree, &immOiHandle, &cl_node);

		if (!resurrected || !cl_node || cl_node->isOm || cl_node->stale) {
			/* Log this error instead of trace, to simplify troubleshooting
			   for the OI maintainer. */
			LOG_IN("ERR_BAD_HANDLE: Reactive ressurect of handle %llx failed",
				immOiHandle);
			if (cl_node && cl_node->stale) {cl_node->exposed = true;}
			rc = SA_AIS_ERR_BAD_HANDLE;
			goto stale_handle;
		}

		TRACE_1("Reactive ressurect of handle %llx succeeded", immOiHandle);
	}

	if (cl_node->isApplier) {
		rc = SA_AIS_ERR_BAD_HANDLE;
		TRACE_2("ERR_BAD_HANDLE: The SaImmOiHandleT is associated with an >>applier<< name");
		goto stale_handle;
	}

	if(isA2bCall && !(cl_node->isImmA2b)) {
		rc = SA_AIS_ERR_VERSION;
		TRACE_2("ERR_VERSION: saImmOmAdminOperationInvoke_o2 only supported for "
			"A.02.11 and above");
		goto stale_handle;

	}

	/* Note NOT unsigned since negative means async invoc. */
	SaInt32T inv = m_IMMSV_UNPACK_HANDLE_LOW(invocation);
	SaInt32T owner = m_IMMSV_UNPACK_HANDLE_HIGH(invocation);

	/* populate the structure */
	memset(&adminOpRslt_evt, 0, sizeof(IMMSV_EVT));
	adminOpRslt_evt.type = IMMSV_EVT_TYPE_IMMND;
	/*Need to encode async/sync variant. */
	if (inv < 0) {
		adminOpRslt_evt.info.immnd.type = returnParams ? IMMND_EVT_A2ND_ASYNC_ADMOP_RSP_2:
			IMMND_EVT_A2ND_ASYNC_ADMOP_RSP;
	} else {
		if(owner) {
			adminOpRslt_evt.info.immnd.type = returnParams ? IMMND_EVT_A2ND_ADMOP_RSP_2:
				IMMND_EVT_A2ND_ADMOP_RSP;
		} else {
			TRACE_1("PBE_ADMOP_RSP");
			if(!(cl_node->isPbe)) {
				/* Log this error instead of trace, to simplify troubleshooting
				   for the OI maintainer. */
				LOG_IN("ERR_INVALID_PARAM: Illegal SaInvocationT value provided in saImmOiAdminOperationResult");
				rc = SA_AIS_ERR_INVALID_PARAM;
				goto mds_send_fail;
			}
			adminOpRslt_evt.info.immnd.type = IMMND_EVT_A2ND_PBE_ADMOP_RSP;
		}
	}
	adminOpRslt_evt.info.immnd.info.admOpRsp.oi_client_hdl = immOiHandle;
	adminOpRslt_evt.info.immnd.info.admOpRsp.invocation = invocation;
	adminOpRslt_evt.info.immnd.info.admOpRsp.result = result;
	adminOpRslt_evt.info.immnd.info.admOpRsp.error = SA_AIS_OK;

	if(returnParams) {
		const SaImmAdminOperationParamsT_2 *param=NULL;
		int i;

		if(!(cl_node->isImmA2b)) {
			/* Log this error instead of trace, to simplify troubleshooting
			   for the OI maintainer. */
			LOG_IN("ERR_VERSION: saImmOiAdminOperationResult_o2 with return params" 
			       "only allowed when handle is registered as A.2.11");
			rc = SA_AIS_ERR_VERSION;
			goto mds_send_fail;
		}

		if (!imma_proc_is_adminop_params_valid(returnParams)) {
			/* Log this error instead of trace, to simplify troubleshooting
			   for the OI maintainer. */
			LOG_IN("ERR_INVALID_PARAM: Not a valid parameter for reply");
			rc = SA_AIS_ERR_INVALID_PARAM;
			goto mds_send_fail;
		}

		osafassert(adminOpRslt_evt.info.immnd.info.admOpRsp.parms == NULL);
		for (i = 0; returnParams[i]; ++i) {
			param = returnParams[i];
			/*alloc-a */
			IMMSV_ADMIN_OPERATION_PARAM *p = malloc(sizeof(IMMSV_ADMIN_OPERATION_PARAM));
			memset(p, 0, sizeof(IMMSV_ADMIN_OPERATION_PARAM));
			TRACE("PARAM:%s \n", param->paramName);

			errStringPar = (strcmp(param->paramName, SA_IMM_PARAM_ADMOP_ERROR) == 0);
			if(errStringPar && (param->paramType != SA_IMM_ATTR_SASTRINGT)) {
				/* Log this error instead of trace, to simplify troubleshooting
				   for the OI maintainer. */
				LOG_IN("ERR_INVALID_PARAM: Param %s must be of type SaStringT", 
					SA_IMM_PARAM_ADMOP_ERROR);
				rc = SA_AIS_ERR_INVALID_PARAM;
				free(p);
				p=NULL;
				goto mds_send_fail;
			}

			if(errStringPar && (result == SA_AIS_OK)) {
				/* Log this error instead of trace, to simplify troubleshooting
				   for the OI maintainer. */
				LOG_IN("ERR_INVALID_PARAM: Param %s only allowed when result != SA_AIS_OK", 
					SA_IMM_PARAM_ADMOP_ERROR);
				rc = SA_AIS_ERR_INVALID_PARAM;
				free(p);
				p=NULL;
				goto mds_send_fail;
			}

			p->paramName.size = strlen(param->paramName) + 1;
			if (p->paramName.size >= SA_MAX_NAME_LENGTH) {
				/* Log this error instead of trace, to simplify troubleshooting
				   for the OI maintainer. */
				LOG_IN("ERR_INVALID_PARAM: Param name too long");
				rc = SA_AIS_ERR_INVALID_PARAM;
				free(p);
				p=NULL;
				goto mds_send_fail;
			}
			/*alloc-b */
			p->paramName.buf = malloc(p->paramName.size);
			strncpy(p->paramName.buf, param->paramName, p->paramName.size);

			p->paramType = param->paramType;
			/*alloc-c */
			imma_copyAttrValue(&(p->paramBuffer), param->paramType, param->paramBuffer);

			p->next = adminOpRslt_evt.info.immnd.info.admOpRsp.parms; /*NULL initially. */
			adminOpRslt_evt.info.immnd.info.admOpRsp.parms = p;
		}
	}

	if (cb->is_immnd_up == false) {
		rc = SA_AIS_ERR_TRY_AGAIN;
		/* IMMND must have gone down after we locked above */
		TRACE_2("ERR_TRY_AGAIN: IMMND_DOWN");
		goto mds_send_fail;
	}

	/* send the reply to the IMMND ASYNCronously */
	if(adminOpRslt_evt.info.immnd.type == IMMND_EVT_A2ND_PBE_ADMOP_RSP) {
		proc_rc = imma_evt_fake_evs(cb, &adminOpRslt_evt, NULL, 0, cl_node->handle, &locked, false);
	} else {
		/* Unlock before MDS Send */
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
		locked = false;
		cl_node=NULL; /* avoid unsafe use */

		proc_rc = imma_mds_msg_send(cb->imma_mds_hdl, &cb->immnd_mds_dest,
			&adminOpRslt_evt, NCSMDS_SVC_ID_IMMND);
	}

	/* MDS error handling */
	switch (proc_rc) {
		case NCSCC_RC_SUCCESS:
			break;
		case NCSCC_RC_REQ_TIMOUT:
			/*The timeout case should be impossible on asyncronous send.. */
			rc = SA_AIS_ERR_TIMEOUT;
			goto mds_send_fail;
		default:
			/* Log this error instead of trace, to simplify troubleshooting
			   for the OI maintainer. */
			LOG_IN("ERR_LIBRARY: MDS returned unexpected error code %u", proc_rc);
			rc = SA_AIS_ERR_LIBRARY;
			goto mds_send_fail;
	}

 mds_send_fail:
	while (adminOpRslt_evt.info.immnd.info.admOpRsp.parms) {
		IMMSV_ADMIN_OPERATION_PARAM *p = adminOpRslt_evt.info.immnd.info.admOpRsp.parms;
		adminOpRslt_evt.info.immnd.info.admOpRsp.parms = p->next;

		if (p->paramName.buf) {	/*free-b */
			free(p->paramName.buf);
			p->paramName.buf = NULL;
		}
		immsv_evt_free_att_val(&(p->paramBuffer), p->paramType);	/*free-c */
		p->next = NULL;
		free(p);	/*free-a */
	}

 stale_handle:
 node_not_found:
	

	if (locked)
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);

 lock_fail:
	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
  Name          :  saImmOiImplementerSet
 
  Description   :  Initialize an object implementer, associating this process
                   with an implementer name.
                   This is a blocking call.

                   
  Arguments     :  immOiHandle - IMM OI handle
                   implementerName - The name of the implementer.

  Dont need any library local implementerInstance corresponding to
  say the adminowner instance of the OM-API. Because there is
  nothing corresponding to the adminOwnerHandle.
  Instead the fact that implementer has been set can be associated with
  the immOiHandle.

  A given oi-handle can be associated with at most one implementer name.
  This then means that any new invocation of this method using the same
  immOiHandle must either return with error, or perform an implicit clear 
  of the previous implementer name. We choose to return error.

  Return Values :  Refer to SAI-AIS specification for various return values.
******************************************************************************/
SaAisErrorT saImmOiImplementerSet(SaImmOiHandleT immOiHandle, const SaImmOiImplementerNameT implementerName)
{
	SaAisErrorT rc = SA_AIS_OK;
	IMMA_CB *cb = &imma_cb;
	IMMSV_EVT evt;
	IMMSV_EVT *out_evt = NULL;
	IMMA_CLIENT_NODE *cl_node = NULL;
	SaUint32T proc_rc = NCSCC_RC_SUCCESS;
	bool locked = true;
	SaUint32T nameLen = 0;
	SaUint32T timeout = 0;

	if (cb->sv_id == 0) {
		TRACE_2("ERR_BAD_HANDLE: No initialized handle exists!");
		return SA_AIS_ERR_BAD_HANDLE;
	}

	if ((implementerName == NULL) || (nameLen = strlen(implementerName)) == 0) {
		TRACE_2("ERR_INVALID_PARAM: Parameter 'implementerName' is NULL, or is a "
			"string of 0 length");
		return SA_AIS_ERR_INVALID_PARAM;
	}
	++nameLen;		/*Add 1 for the null. */

	if (nameLen >= SA_MAX_NAME_LENGTH) {
		TRACE_4("ERR_LIBRARY: Implementer name too long, size: %u max:%u", 
			nameLen, SA_MAX_NAME_LENGTH);
		return SA_AIS_ERR_LIBRARY;
	}

	if (cb->is_immnd_up == false) {
		TRACE_2("ERR_TRY_AGAIN: IMMND is DOWN");
		return SA_AIS_ERR_TRY_AGAIN;
	}

	/* get the CB Lock */
	if (m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
		rc = SA_AIS_ERR_LIBRARY;
		TRACE_4("ERR_LIBRARY: Lock failed");
		goto lock_fail;
	}

	/*locked == true already */

	imma_client_node_get(&cb->client_tree, &immOiHandle, &cl_node);
	if (!cl_node || cl_node->isOm) {
		rc = SA_AIS_ERR_BAD_HANDLE;
		TRACE_2("ERR_BAD_HANDLE: client_node_get failed");
		goto bad_handle;
	}

	if (cl_node->mImplementerId) {
		rc = SA_AIS_ERR_INVALID_PARAM;
		/* This is a tricky one #3086. This is not the ERR_EXIST case.
		   The ERR_EXIST case is where this client is trying to
		   set implementer-name for the oi-handle and that
		   implementer-name is occupied by some *other* handle/user.
		   The problem here is that *this* handle is already occupied
		   with an implementer name, possibly the same name possibly
		   not. This is an interface violation since the OI is not
		   making sense in this request on the handle. Ideally this
		   should result in ERR_INVALID_PARAM. That error code
		   happens not to be allowed on saImmOiImplementerSet according
		   to SAF. The legal alternatives available are BAD_HANDLE or
		   ERR_LIBRARY, both of which are missleading. We choose to
		   return ERR_INVALID_PARAM. This is a deviation from the spec,
		   but only happens to an OI that is incorrectly implemented.
		 */
		LOG_NO("ERR_INVALID_PARAM: Implementer %s already set for this handle "
			"when trying to set %s", cl_node->mImplementerName, implementerName);
		goto bad_handle;
	}

	if (cl_node->stale) {
		TRACE_1("Handle %llx is stale", immOiHandle);
		bool resurrected = imma_oi_resurrect(cb, cl_node, &locked, &rc);
		if(rc == SA_AIS_ERR_TRY_AGAIN) {
			osafassert(!resurrected);
			goto bad_handle; /* Handle is actually not bad yet. */
		}

		if (!locked && m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
			TRACE_4("ERR_LIBRARY: LOCK failed");
			rc = SA_AIS_ERR_LIBRARY;
			goto lock_fail;
		}
		locked = true;

		imma_client_node_get(&cb->client_tree, &immOiHandle, &cl_node);

		if (!resurrected || !cl_node || cl_node->isOm || cl_node->stale) {
			TRACE_2("ERR_BAD_HANDLE: Reactive ressurect of handle %llx failed", 
				immOiHandle);
			if (cl_node && cl_node->stale) {cl_node->exposed = true;}
			rc = SA_AIS_ERR_BAD_HANDLE;
			goto bad_handle;
		}

		TRACE_1("Reactive resurrect of handle %llx succeeded", immOiHandle);
	}

	timeout = cl_node->syncr_timeout;

	/* Check for API Version! Appliers only allow for A.02.11 and above. */
	if(implementerName[0] == '@') {
		if(!(cl_node->isImmA2b)) {
			rc = SA_AIS_ERR_VERSION;
			TRACE_2("ERR_VERSION: Applier OIs only supported for A.02.11 and above");
			goto bad_handle;
		}

		if (cl_node->o.iCallbk.saImmOiCcbApplyCallback == NULL) {
			rc = SA_AIS_ERR_INIT;
			TRACE_2("ERR_INIT: The SaImmOiCcbApplyCallbackT "
				"function was not set in the initialization of the handle "
				"=> Can not register as applier!");
			goto bad_handle;
		}

		if (cl_node->o.iCallbk.saImmOiCcbAbortCallback == NULL) {
			rc = SA_AIS_ERR_INIT;
			TRACE_2("ERR_INIT: The SaImmOiCcbAbortCallbackT "
				"function was not set in the initialization of the handle "
				"=> Can not register as applier!");
			goto bad_handle;
		}

		if (cl_node->o.iCallbk.saImmOiCcbObjectModifyCallback == NULL) {
			rc = SA_AIS_ERR_INIT;
			TRACE_2("ERR_INIT: The SaImmOiCcbObjectModifyCallbackT "
				"function was not set in the initialization of the handle "
				"=> Can not register as applier!");
			goto bad_handle;
		}

		if (cl_node->o.iCallbk.saImmOiCcbObjectDeleteCallback == NULL) {
			rc = SA_AIS_ERR_INIT;
			TRACE_2("ERR_INIT: The SaImmOiCcbObjectDeleteCallbackT "
				"function was not set in the initialization of the handle "
				"=> Can not register as applier!");
			goto bad_handle;
		}
		/* Optimistically turn on cl_node->isApplier since callbacks may arrive to
		   the mds thread before reply on the implementerSet request arrives back here. 
		   If error is returned by implementerSet then isApplier is reset to 0 below.
		 */
		cl_node->isApplier = 0x1;
	} else if(strncmp(implementerName, OPENSAF_IMM_PBE_IMPL_NAME, nameLen) == 0) {
		/* Optimistically turn on cl-node->isPbe since callbacks may arrive to the mds
		   thread before reply on the implementerSet request arrives back here. 
		   If error is returned by implementerSet then isPbe is reset to 0 below.
		 */
		TRACE("Special implementer %s detected and noted.", OPENSAF_IMM_PBE_IMPL_NAME);
		cl_node->isPbe = 0x1;
	}

	/* Populate & Send the Open Event to IMMND */
	memset(&evt, 0, sizeof(IMMSV_EVT));
	evt.type = IMMSV_EVT_TYPE_IMMND;
	evt.info.immnd.type = IMMND_EVT_A2ND_OI_IMPL_SET;
	evt.info.immnd.info.implSet.client_hdl = immOiHandle;
	evt.info.immnd.info.implSet.impl_name.size = nameLen;
	evt.info.immnd.info.implSet.impl_name.buf = implementerName;

	imma_proc_increment_pending_reply(cl_node);

	/* Unlock before MDS Send */
	m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
	locked = false;
	cl_node = NULL; /* avoid unsafe use */
	
	if (cb->is_immnd_up == false) {
		rc = SA_AIS_ERR_TRY_AGAIN;
		TRACE_2("ERR_TRY_AGAIN: IMMND is DOWN");
		goto mds_send_fail;
	}

	/* Send the evt to IMMND */
	proc_rc = imma_mds_msg_sync_send(cb->imma_mds_hdl, &(cb->immnd_mds_dest), &evt, &out_evt, timeout);

	evt.info.immnd.info.implSet.impl_name.buf = NULL;
	evt.info.immnd.info.implSet.impl_name.size = 0;

	/* Generate rc from proc_rc */
	switch (proc_rc) {
		case NCSCC_RC_SUCCESS:
			break;
		case NCSCC_RC_REQ_TIMOUT:
			rc = imma_proc_check_stale(cb, immOiHandle, SA_AIS_ERR_TIMEOUT);
			break; /* i.e. goto mds_send_fail */
		default:
			rc = SA_AIS_ERR_LIBRARY;
			TRACE_4("ERR_LIBRARY: MDS returned unexpected error code %u", proc_rc);
			/* Losing track of the pending reply count, but ERR_LIBRARY dominates*/
			goto bad_handle;
			break; 
	}

 mds_send_fail:

	if (m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
		rc = SA_AIS_ERR_LIBRARY;
		/* Losing track of the pending reply count, but ERR_LIBRARY dominates*/
		TRACE_4("ERR_LIBRARY: Lock failed");
		goto lock_fail;
	}
	locked = true;

	imma_client_node_get(&cb->client_tree, &immOiHandle, &cl_node);
	if (!cl_node || cl_node->isOm) {
		rc = SA_AIS_ERR_BAD_HANDLE;
		TRACE_2("ERR_BAD_HANDLE: client_node_get failed");
		goto bad_handle;
	}

	imma_proc_decrement_pending_reply(cl_node);

	if (cl_node->stale) {
		TRACE_1("Handle %llx is stale after successfull call", immOiHandle);
		/* This is implementer set => user expects this to be 
		   upheld, handle is stale but implementorship SURVIVES
		   resurrection. But we can not reply ok on this call since
		   we should make the user aware that they are actually detached
		   from the implementership that this call could have established
		 */
		rc = SA_AIS_ERR_BAD_HANDLE;
		cl_node->exposed = true;
		goto bad_handle;
	}

	if (out_evt) {
		/* Process the received Event */
		osafassert(out_evt->type == IMMSV_EVT_TYPE_IMMA);
		osafassert(out_evt->info.imma.type == IMMA_EVT_ND2A_IMPLSET_RSP);
		if (rc == SA_AIS_OK) {
			rc = out_evt->info.imma.info.implSetRsp.error;
			if (rc == SA_AIS_OK) {
				cl_node->mImplementerId = out_evt->info.imma.info.implSetRsp.implId;
				cl_node->mImplementerName = calloc(1, nameLen);
				strncpy(cl_node->mImplementerName, implementerName, nameLen);
			}
		}
	}

	if(rc != SA_AIS_OK && cl_node) { /* Revert any flags set optimistically. */
		cl_node->isApplier = 0x0;
		cl_node->isPbe = 0x0;
	}

 bad_handle:
	if (locked)
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);

 lock_fail:
	if (out_evt)
		free(out_evt);

	return rc;

}

/****************************************************************************
  Name          :  saImmOiImplementerClear
 
  Description   :  Clear implementer. Severs the association between
                   this process/connection and the implementer name set
                   by saImmOiImplementerSet.
                   This is a blocking call.

                   
  Arguments     :  immOiHandle - IMM OI handle

  This call will fail if not a prior call to implementerSet has been made.

  Return Values :  Refer to SAI-AIS specification for various return values.
******************************************************************************/
SaAisErrorT saImmOiImplementerClear(SaImmOiHandleT immOiHandle)
{
	SaAisErrorT rc = SA_AIS_OK;
	IMMA_CB *cb = &imma_cb;
	IMMSV_EVT evt;
	IMMSV_EVT *out_evt = NULL;
	IMMA_CLIENT_NODE *cl_node = NULL;
	bool locked = true;

	if (cb->sv_id == 0) {
		TRACE_2("ERR_BAD_HANDLE: No initialized handle exists!");
		return SA_AIS_ERR_BAD_HANDLE;
	}

	if (cb->is_immnd_up == false) {
		TRACE_2("ERR_TRY_AGAIN: IMMND is DOWN");
		return SA_AIS_ERR_TRY_AGAIN;
	}

	/* get the CB Lock */
	if (m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
		rc = SA_AIS_ERR_LIBRARY;
		TRACE_4("ERR_LIBRARY: Lock failed");
		goto lock_fail;
	}

	/*locked == true already */

	imma_client_node_get(&cb->client_tree, &immOiHandle, &cl_node);
	if (!cl_node || cl_node->isOm) {
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto bad_handle;
	}

	if (cl_node->mImplementerId == 0) {
		rc = SA_AIS_ERR_BAD_HANDLE;
		/* Yes BAD_HANDLE and not ERR_EXIST, see standard. */
		TRACE_2("ERR_BAD_HANDLE: No implementer is set for this handle");
		goto bad_handle;
	}

	if (cl_node->stale) {
		TRACE_1("Handle %llx is stale", immOiHandle);
		/* Note that this is implementer clear. We dont want a resurrect to
		   set implementer, just so we can clear it right after!
		   Instead we try to only resurrect, but avoid setting implementer,
		   which produces the desired result towards the invoker.
		 */
		cl_node->mImplementerId = 0;
		free(cl_node->mImplementerName);
		cl_node->mImplementerName = NULL;

		bool resurrected = imma_oi_resurrect(cb, cl_node, &locked, &rc);
		if(rc == SA_AIS_ERR_TRY_AGAIN) {
			osafassert(!resurrected);
			goto bad_handle; /* Handle is actually not bad yet. */
		}

		if (!locked && m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
			TRACE_4("ERR_LIBRARY: LOCK failed");
			rc = SA_AIS_ERR_LIBRARY;
			goto lock_fail;
		}
		locked = true;

		imma_client_node_get(&cb->client_tree, &immOiHandle, &cl_node);

		if (!resurrected || !cl_node || cl_node->isOm || cl_node->stale) {
			TRACE_2("ERR_BAD_HANDLE: Reactive ressurect of handle %llx failed", 
				immOiHandle);
			if (cl_node && cl_node->stale) {cl_node->exposed = true;}
			rc = SA_AIS_ERR_BAD_HANDLE;
			goto bad_handle;
		}

		TRACE_1("Reactive resurrect of handle %llx succeeded", immOiHandle);
		goto skip_impl_clear; /* Implementer already cleared by stale => resurrect */
	}

	/* Populate & Send the Open Event to IMMND */
	memset(&evt, 0, sizeof(IMMSV_EVT));
	evt.type = IMMSV_EVT_TYPE_IMMND;
	evt.info.immnd.type = IMMND_EVT_A2ND_OI_IMPL_CLR;
	evt.info.immnd.info.implSet.client_hdl = immOiHandle;
	evt.info.immnd.info.implSet.impl_id = cl_node->mImplementerId;

	imma_proc_increment_pending_reply(cl_node);

	rc = imma_evt_fake_evs(cb, &evt, &out_evt, cl_node->syncr_timeout, cl_node->handle, &locked, true);

	cl_node=NULL;
	/* Take the CB lock  */
	if (!locked && m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != 
		NCSCC_RC_SUCCESS) {
		rc = SA_AIS_ERR_LIBRARY;
		/* Losing track of the pending reply count, but ERR_LIBRARY dominates*/
		TRACE_4("ERR_LIBRARY: LOCK failed");
		goto lock_fail1;
	}
	locked = true;

	imma_client_node_get(&cb->client_tree, &immOiHandle, &cl_node);
	if (!cl_node || cl_node->isOm) {
		rc = SA_AIS_ERR_BAD_HANDLE;
		TRACE_2("ERR_BAD_HANDLE: client_node_get failed");
		goto bad_handle;
	}

	imma_proc_decrement_pending_reply(cl_node);

	if (rc != SA_AIS_OK) {
		/* fake_evs returned error */
		goto skip_impl_clear;
	}

	if (cl_node->stale) {
		TRACE_1("Handle %llx is stale after successfull call - ignoring", 
			immOiHandle);
		/*
		  This is implementer clear => a relaxation
		  => not necessary to expose when the call succeeded.
		  rc = SA_AIS_ERR_BAD_HANDLE;
		  cl_node->exposed = true;
		  goto bad_handle;
		*/
	}

	osafassert(out_evt);
	/* Process the received Event */
	osafassert(out_evt->type == IMMSV_EVT_TYPE_IMMA);
	osafassert(out_evt->info.imma.type == IMMA_EVT_ND2A_IMM_ERROR);

	rc = out_evt->info.imma.info.errRsp.error;

	if (rc == SA_AIS_OK) {
		cl_node->mImplementerId = 0;
		free(cl_node->mImplementerName);
		cl_node->mImplementerName = NULL;
		if(cl_node->isApplier)
			cl_node->isApplier = 0;
	}

 skip_impl_clear:
 bad_handle:
	if (locked) 
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);

 lock_fail1:
	if (out_evt) 
		free(out_evt);

 lock_fail:

	return rc;
}

/****************************************************************************
  Name          :  saImmOiClassImplementerSet
 
  Description   :  Set implementer for class and all instances.
                   This is a blocking call.

                   
  Arguments     :  immOiHandle - IMM OI handle
                   className - The name of the class.

  This call will fail if not a prior call to implementerSet has been made.

  Return Values :  Refer to SAI-AIS specification for various return values.
******************************************************************************/
SaAisErrorT saImmOiClassImplementerSet(SaImmOiHandleT immOiHandle, const SaImmClassNameT className)
{
	SaAisErrorT rc = SA_AIS_OK;
	IMMA_CB *cb = &imma_cb;
	IMMSV_EVT evt;
	IMMSV_EVT *out_evt = NULL;
	IMMA_CLIENT_NODE *cl_node = NULL;
	bool locked = true;
	SaUint32T nameLen = 0;

	if (cb->sv_id == 0) {
		TRACE_2("ERR_BAD_HANDLE: No initialized handle exists!");
		return SA_AIS_ERR_BAD_HANDLE;
	}

	if ((className == NULL) || (nameLen = strlen(className)) == 0) {
		TRACE_2("ERR_INVALID_PARAM: Parameter 'className' is NULL or has length 0");
		return SA_AIS_ERR_INVALID_PARAM;
	}
	++nameLen;		/*Add 1 for the null. */

	if (nameLen >= SA_MAX_NAME_LENGTH) {
		TRACE_4("ERR_LIBRARY: ClassName too long, size: %u max:%u", 
			nameLen, SA_MAX_NAME_LENGTH);
		return SA_AIS_ERR_LIBRARY;
	}

	if (cb->is_immnd_up == false) {
		TRACE_2("ERR_TRY_AGAIN: IMMND is DOWN");
		return SA_AIS_ERR_TRY_AGAIN;
	}

	/* get the CB Lock */
	if (m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
		rc = SA_AIS_ERR_LIBRARY;
		TRACE_4("ERR_LIBRARY: Lock failed");
		goto lock_fail;
	}

	/*locked == true already */

	imma_client_node_get(&cb->client_tree, &immOiHandle, &cl_node);
	if (!cl_node || cl_node->isOm) {
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto bad_handle;
	}

	if (cl_node->stale) {
		TRACE_1("Handle %llx is stale", immOiHandle);
		bool resurrected = imma_oi_resurrect(cb, cl_node, &locked, &rc);
		if(rc == SA_AIS_ERR_TRY_AGAIN) {
			osafassert(!resurrected);
			goto bad_handle; /* Handle is actually not bad yet. */
		}

		if (!locked && m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
			TRACE_4("ERR_LIBRARY: LOCK failed");
			rc = SA_AIS_ERR_LIBRARY;
			goto lock_fail;
		}
		locked = true;

		imma_client_node_get(&cb->client_tree, &immOiHandle, &cl_node);

		if (!resurrected || !cl_node || cl_node->isOm || cl_node->stale) {
			TRACE_2("ERR_BAD_HANDLE: Reactive ressurect of handle %llx failed", 
				immOiHandle);
			if (cl_node && cl_node->stale) {cl_node->exposed = true;}
			rc = SA_AIS_ERR_BAD_HANDLE;
			goto bad_handle;
		}

		TRACE_1("Reactive resurrect of handle %llx succeeded", immOiHandle);
	}

	if (cl_node->mImplementerId == 0) {
		rc = SA_AIS_ERR_BAD_HANDLE;
		TRACE_2("ERR_BAD_HANDLE: No implementer is set for this handle");
		goto bad_handle;
	}

	if(cl_node->isApplier) {
		if (cl_node->o.iCallbk.saImmOiCcbObjectCreateCallback == NULL) {
			rc = SA_AIS_ERR_INIT;
			TRACE_2("ERR_INIT: The SaImmOiCcbObjectCreateCallbackT "
				"function was not set in the initialization of the handle "
				"=> Can not register as class applier!");
			goto bad_handle;
		}
	}

	/* Populate & Send the Open Event to IMMND */
	memset(&evt, 0, sizeof(IMMSV_EVT));
	evt.type = IMMSV_EVT_TYPE_IMMND;
	evt.info.immnd.type = IMMND_EVT_A2ND_OI_CL_IMPL_SET;
	evt.info.immnd.info.implSet.client_hdl = cl_node->handle;
	evt.info.immnd.info.implSet.impl_name.size = nameLen;
	evt.info.immnd.info.implSet.impl_name.buf = className;
	evt.info.immnd.info.implSet.impl_id = cl_node->mImplementerId;

	imma_proc_increment_pending_reply(cl_node);

	rc = imma_evt_fake_evs(cb, &evt, &out_evt, cl_node->syncr_timeout, cl_node->handle, &locked, true);

	cl_node=NULL;
	evt.info.immnd.info.implSet.impl_name.buf = NULL;
	evt.info.immnd.info.implSet.impl_name.size = 0;

	if (!locked && m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != 
		NCSCC_RC_SUCCESS) {
		rc = SA_AIS_ERR_LIBRARY; /* Overwrites any error from fake_evs() */
		/* Losing track of the pending reply count, but ERR_LIBRARY dominates*/
		TRACE_4("ERR_LIBRARY: LOCK failed");
		goto lock_fail;
	}
	locked = true;

	/* get the client_info */
	imma_client_node_get(&cb->client_tree, &immOiHandle, &cl_node);
	if (!cl_node || cl_node->isOm) {
		rc = SA_AIS_ERR_BAD_HANDLE;
		TRACE_2("ERR_BAD_HANDLE: client_node_get failed");
		goto bad_handle;
	}

	imma_proc_decrement_pending_reply(cl_node);

	if (rc != SA_AIS_OK) {
		/* fake_evs returned error */
		goto fevs_error;
	}

	if (cl_node->stale) {
		TRACE_3("Handle %llx is stale after successfull call", 
			immOiHandle);

		/* This is class implementer set => user expects this to be 
		   upheld, but handle is stale => exposed.
		 */		
		rc = SA_AIS_ERR_BAD_HANDLE;
		cl_node->exposed = true;
		goto bad_handle;
	}

	osafassert(out_evt);
	/* Process the received Event */
	osafassert(out_evt->type == IMMSV_EVT_TYPE_IMMA);
	osafassert(out_evt->info.imma.type == IMMA_EVT_ND2A_IMM_ERROR);
	rc = out_evt->info.imma.info.errRsp.error;

 fevs_error:
 bad_handle:
	if (locked)
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);

 lock_fail:
	if (out_evt)
		free(out_evt);

	return rc;

}

/****************************************************************************
  Name          :  saImmOiClassImplementerRelease
 
  Description   :  Release implementer for class and all instances.
                   This is a blocking call.

                   
  Arguments     :  immOiHandle - IMM OI handle
                   className - The name of the class.

  This call will fail if not a prior call to ClassImplementerSet has been made.

  Return Values :  Refer to SAI-AIS specification for various return values.
******************************************************************************/
SaAisErrorT saImmOiClassImplementerRelease(SaImmOiHandleT immOiHandle, const SaImmClassNameT className)
{
	SaAisErrorT rc = SA_AIS_OK;
	IMMA_CB *cb = &imma_cb;
	IMMSV_EVT evt;
	IMMSV_EVT *out_evt = NULL;
	IMMA_CLIENT_NODE *cl_node = NULL;
	bool locked = true;
	SaUint32T nameLen = 0;

	if (cb->sv_id == 0) {
		TRACE_2("ERR_BAD_HANDLE: No initialized handle exists!");
		return SA_AIS_ERR_BAD_HANDLE;
	}

	if ((className == NULL) || (nameLen = strlen(className)) == 0) {
		TRACE_2("ERR_INVALID_PARAM: Parameter 'className' is NULL or has length 0");
		return SA_AIS_ERR_INVALID_PARAM;
	}
	++nameLen;		/*Add 1 for the null. */

	if (nameLen >= SA_MAX_NAME_LENGTH) {
		TRACE_4("ERR_LIBRARY: ClassName too long, size: %u max:%u", 
			nameLen, SA_MAX_NAME_LENGTH);
		return SA_AIS_ERR_LIBRARY;
	}

	if (cb->is_immnd_up == false) {
		TRACE_2("ERR_TRY_AGAIN: IMMND is DOWN");
		return SA_AIS_ERR_TRY_AGAIN;
	}

	/* get the CB Lock */
	if (m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
		rc = SA_AIS_ERR_LIBRARY;
		TRACE_4("ERR_LIBRARY: Lock failed");
		goto lock_fail;
	}

	/*locked == true already */

	imma_client_node_get(&cb->client_tree, &immOiHandle, &cl_node);
	if (!cl_node || cl_node->isOm) {
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto bad_handle;
	}

	if (cl_node->stale) {
		TRACE_1("Handle %llx is stale", immOiHandle);
		bool resurrected = imma_oi_resurrect(cb, cl_node, &locked, &rc);
		if(rc == SA_AIS_ERR_TRY_AGAIN) {
			osafassert(!resurrected);
			goto bad_handle; /* Handle is actually not bad yet. */
		}

		if (!locked && m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
			TRACE_4("ERR_LIBRARY: LOCK failed");
			rc = SA_AIS_ERR_LIBRARY;
			goto lock_fail;
		}
		locked = true;

		imma_client_node_get(&cb->client_tree, &immOiHandle, &cl_node);

		if (!resurrected || !cl_node || cl_node->isOm || cl_node->stale) {
			TRACE_2("ERR_BAD_HANDLE: Reactive ressurect of handle %llx failed",
				immOiHandle);
			if (cl_node && cl_node->stale) {cl_node->exposed = true;}
			rc = SA_AIS_ERR_BAD_HANDLE;
			goto bad_handle;
		}

		TRACE_1("Reactive resurrect of handle %llx succeeded", immOiHandle);
	}

	if (cl_node->mImplementerId == 0) {
		rc = SA_AIS_ERR_BAD_HANDLE;
		TRACE_2("ERR_BAD_HANDLE: No implementer is set for this handle");
		goto bad_handle;
	}

	/* Populate & Send the Open Event to IMMND */
	memset(&evt, 0, sizeof(IMMSV_EVT));
	evt.type = IMMSV_EVT_TYPE_IMMND;
	evt.info.immnd.type = IMMND_EVT_A2ND_OI_CL_IMPL_REL;
	evt.info.immnd.info.implSet.client_hdl = cl_node->handle;
	evt.info.immnd.info.implSet.impl_name.size = nameLen;
	evt.info.immnd.info.implSet.impl_name.buf = className;
	evt.info.immnd.info.implSet.impl_id = cl_node->mImplementerId;

	imma_proc_increment_pending_reply(cl_node);

	rc = imma_evt_fake_evs(cb, &evt, &out_evt, cl_node->syncr_timeout, cl_node->handle, &locked, true);

	cl_node=NULL;
	evt.info.immnd.info.implSet.impl_name.buf = NULL;
	evt.info.immnd.info.implSet.impl_name.size = 0;

	if (!locked && m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != 
		NCSCC_RC_SUCCESS) {
		rc = SA_AIS_ERR_LIBRARY; /* Overwrites any error from fake_evs() */
		/* Losing track of the pending reply count, but ERR_LIBRARY dominates*/
		TRACE_4("ERR_LIBRARY: LOCK failed");
		goto lock_fail;
	}
	locked = true;

	/* get the client_info */
	imma_client_node_get(&cb->client_tree, &immOiHandle, &cl_node);
	if (!cl_node || cl_node->isOm) {
		rc = SA_AIS_ERR_BAD_HANDLE;
		TRACE_2("ERR_BAD_HANDLE: client_node_get failed");
		goto bad_handle;
	}

	imma_proc_decrement_pending_reply(cl_node);

	if (rc != SA_AIS_OK) {
		/* fake_evs returned error */
		goto fevs_error;
	}

	if (cl_node->stale) {
		TRACE_3("Handle %llx is stale after successfull call - ignoring", 
			immOiHandle);
		/*
		  This is a class implementer release => a relaxation
		  => not necessary to expose when the call succeeded.
		  rc = SA_AIS_ERR_BAD_HANDLE;
		  cl_node->exposed = true;
		  goto bad_handle;
		*/
	}

	osafassert(out_evt);
	/* Process the received Event */
	osafassert(out_evt->type == IMMSV_EVT_TYPE_IMMA);
	osafassert(out_evt->info.imma.type == IMMA_EVT_ND2A_IMM_ERROR);
	rc = out_evt->info.imma.info.errRsp.error;


 fevs_error:
 bad_handle:
	if (locked)
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);

 lock_fail:
	if (out_evt)
		free(out_evt);
	return rc;

}

/****************************************************************************
  Name          :  saImmOiObjectImplementerSet
 
  Description   :  Set implementer for the objects identified by scope
                   and objectName. This is a blocking call.

                   
  Arguments     :  immOiHandle - IMM OI handle
                   objectName - The name of the top object.

  This call will fail if not a prior call to implementerSet has been made.

  Return Values :  Refer to SAI-AIS specification for various return values.
******************************************************************************/
SaAisErrorT saImmOiObjectImplementerSet(SaImmOiHandleT immOiHandle, const SaNameT *objectName, SaImmScopeT scope)
{
	SaAisErrorT rc = SA_AIS_OK;
	IMMA_CB *cb = &imma_cb;
	IMMSV_EVT evt;
	IMMSV_EVT *out_evt = NULL;
	IMMA_CLIENT_NODE *cl_node = NULL;
	bool locked = true;
	SaUint32T nameLen = 0;

	if (cb->sv_id == 0) {
		TRACE_2("ERR_BAD_HANDLE: No initialized handle exists!");
		return SA_AIS_ERR_BAD_HANDLE;
	}

	if ((objectName == NULL) || (objectName->length >= SA_MAX_NAME_LENGTH) || 
		(objectName->length == 0)) {
		TRACE_2("ERR_INVALID_PARAM: Parameter 'objectName' is NULL or too long "
			"or zero length");
		return SA_AIS_ERR_INVALID_PARAM;
	}

	TRACE_1("value:'%s' len:%u", objectName->value, objectName->length);
	nameLen = strlen((char *) objectName->value);
	if(objectName->length < nameLen) {
		nameLen = objectName->length;
	}
	++nameLen;  /* Add 1 for the null */

	switch (scope) {
		case SA_IMM_ONE:
		case SA_IMM_SUBLEVEL:
		case SA_IMM_SUBTREE:
			break;
		default:
			TRACE_2("ERR_INVALID_PARAM: Parameter 'scope' has incorrect value %u",
				scope);
			return SA_AIS_ERR_INVALID_PARAM;
	}

	if (cb->is_immnd_up == false) {
		TRACE_2("ERR_TRY_AGAIN: IMMND is DOWN");
		return SA_AIS_ERR_TRY_AGAIN;
	}

	/* get the CB Lock */
	if (m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
		rc = SA_AIS_ERR_LIBRARY;
		TRACE_4("ERR_LIBRARY: Lock failed");
		goto lock_fail;
	}

	/*locked == true already */

	imma_client_node_get(&cb->client_tree, &immOiHandle, &cl_node);
	if (!cl_node || cl_node->isOm) {
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto bad_handle;
	}

	if (cl_node->stale) {
		TRACE_1("Handle %llx is stale", immOiHandle);
		bool resurrected = imma_oi_resurrect(cb, cl_node, &locked, &rc);
		if(rc == SA_AIS_ERR_TRY_AGAIN) {
			osafassert(!resurrected);
			goto bad_handle; /* Handle is actually not bad yet. */
		}

		if (!locked && m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
			TRACE_4("ERR_LIBRARY: LOCK failed");
			rc = SA_AIS_ERR_LIBRARY;
			goto lock_fail;
		}
		locked = true;

		imma_client_node_get(&cb->client_tree, &immOiHandle, &cl_node);

		if (!resurrected || !cl_node || cl_node->isOm || cl_node->stale) {
			TRACE_2("ERR_BAD_HANDLE: Reactive ressurect of handle %llx failed",
				immOiHandle);
			if (cl_node && cl_node->stale) {cl_node->exposed = true;}
			rc = SA_AIS_ERR_BAD_HANDLE;
			goto bad_handle;
		}

		TRACE_1("Reactive resurrect of handle %llx succeeded", immOiHandle);
	}

	if (cl_node->mImplementerId == 0) {
		rc = SA_AIS_ERR_BAD_HANDLE;
		TRACE_2("ERR_BAD_HANDLE: No implementer is set for this handle");
		goto bad_handle;
	}

	/* Populate & Send the Open Event to IMMND */
	memset(&evt, 0, sizeof(IMMSV_EVT));
	evt.type = IMMSV_EVT_TYPE_IMMND;
	evt.info.immnd.type = IMMND_EVT_A2ND_OI_OBJ_IMPL_SET;
	evt.info.immnd.info.implSet.client_hdl = cl_node->handle;
	evt.info.immnd.info.implSet.impl_name.size = nameLen;
	evt.info.immnd.info.implSet.impl_name.buf = malloc(nameLen);
	strncpy(evt.info.immnd.info.implSet.impl_name.buf, 
		(char *)objectName->value, nameLen - 1);
	evt.info.immnd.info.implSet.impl_name.buf[nameLen - 1] = 0;
	TRACE("Sending size:%u val:'%s'", nameLen - 1, 
		evt.info.immnd.info.implSet.impl_name.buf);
	evt.info.immnd.info.implSet.impl_id = cl_node->mImplementerId;
	evt.info.immnd.info.implSet.scope = scope;

	imma_proc_increment_pending_reply(cl_node);

	rc = imma_evt_fake_evs(cb, &evt, &out_evt, cl_node->syncr_timeout, cl_node->handle, &locked, true);

	cl_node=NULL;
	free(evt.info.immnd.info.implSet.impl_name.buf);
	evt.info.immnd.info.implSet.impl_name.buf = NULL;
	evt.info.immnd.info.implSet.impl_name.size = 0;

	if (!locked && m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != 
		NCSCC_RC_SUCCESS) {
		rc = SA_AIS_ERR_LIBRARY; /* Overwrites any error from fake_evs() */
		/* Losing track of the pending reply count, but ERR_LIBRARY dominates*/
		TRACE_4("ERR_LIBRARY: LOCK failed");
		goto lock_fail;
	}
	locked = true;

	/* get the client_info */
	imma_client_node_get(&cb->client_tree, &immOiHandle, &cl_node);
	if (!cl_node || cl_node->isOm) {
		rc = SA_AIS_ERR_BAD_HANDLE;
		TRACE_2("ERR_BAD_HANDLE: client_node_get failed");
		goto bad_handle;
	}

	imma_proc_decrement_pending_reply(cl_node);

	if (rc != SA_AIS_OK) {
		/* fake_evs returned error */
		goto fevs_error;
	}

	if (cl_node->stale) {
		TRACE_3("Handle %llx is stale after successfull call", 
			immOiHandle);

		/* This is an object implementer set => user expects this to be 
		   upheld, but handle is stale => exposed.
		 */
		rc = SA_AIS_ERR_BAD_HANDLE;
		cl_node->exposed = true;
		goto bad_handle;
	}

	osafassert(out_evt);
	osafassert(out_evt->type == IMMSV_EVT_TYPE_IMMA);
	osafassert(out_evt->info.imma.type == IMMA_EVT_ND2A_IMM_ERROR);
	rc = out_evt->info.imma.info.errRsp.error;

 fevs_error:
 bad_handle:
	if (locked)
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);

 lock_fail:
	if (out_evt)
		free(out_evt);

	return rc;
}

/****************************************************************************
  Name          :  saImmOiObjectImplementerRelease
 
  Description   :  Release implementer for the objects identified by scope
                   and objectName. This is a blocking call.

                   
  Arguments     :  immOiHandle - IMM OI handle
                   objectName - The name of the top object.

  This call will fail if not a prior call to objectImplementerSet has been 
  made.

  Return Values :  Refer to SAI-AIS specification for various return values.
******************************************************************************/
SaAisErrorT saImmOiObjectImplementerRelease(SaImmOiHandleT immOiHandle, const SaNameT *objectName, SaImmScopeT scope)
{
	SaAisErrorT rc = SA_AIS_OK;
	IMMA_CB *cb = &imma_cb;
	IMMSV_EVT evt;
	IMMSV_EVT *out_evt = NULL;
	IMMA_CLIENT_NODE *cl_node = NULL;
	bool locked = true;
	SaUint32T nameLen = 0;

	if (cb->sv_id == 0) {
		TRACE_2("ERR_BAD_HANDLE: No initialized handle exists!");
		return SA_AIS_ERR_BAD_HANDLE;
	}

	if ((objectName == NULL) || (objectName->length == 0) || 
		(objectName->length >= SA_MAX_NAME_LENGTH)) {
		TRACE_2("ERR_INVALID_PARAM: Parameter 'objectName' is NULL or too long "
			"or zero length");
		return SA_AIS_ERR_INVALID_PARAM;
	}
	nameLen = strlen((char *)objectName->value);

	if (objectName->length < nameLen) {
		nameLen = objectName->length;
	}
	++nameLen;		/*Add 1 for the null. */

	switch (scope) {
		case SA_IMM_ONE:
		case SA_IMM_SUBLEVEL:
		case SA_IMM_SUBTREE:
			break;
		default:
			TRACE_2("ERR_INVALID_PARAM: Parameter 'scope' has incorrect value %u",
				scope);
			return SA_AIS_ERR_INVALID_PARAM;
	}

	if (cb->is_immnd_up == false) {
		TRACE_2("ERR_TRY_AGAIN: IMMND is DOWN");
		return SA_AIS_ERR_TRY_AGAIN;
	}

	/* get the CB Lock */
	if (m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
		rc = SA_AIS_ERR_LIBRARY;
		TRACE_4("ERR_LIBRARY: Lock failed");
		goto lock_fail;
	}

	/*locked == true already */

	imma_client_node_get(&cb->client_tree, &immOiHandle, &cl_node);
	if (!cl_node || cl_node->isOm) {
		TRACE_2("ERR_BAD_HANDLE: Not a valid SaImmOiHandleT");
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto bad_handle;
	}

	if (cl_node->stale) {
		TRACE_1("Handle %llx is stale", immOiHandle);
		bool resurrected = imma_oi_resurrect(cb, cl_node, &locked, &rc);
		if(rc == SA_AIS_ERR_TRY_AGAIN) {
			osafassert(!resurrected);
			goto bad_handle; /* Handle is actually not bad yet. */
		}

		if (!locked && m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
			TRACE_4("ERR_LIBRARY: LOCK failed");
			rc = SA_AIS_ERR_LIBRARY;
			goto lock_fail;
		}
		locked = true;

		imma_client_node_get(&cb->client_tree, &immOiHandle, &cl_node);

		if (!resurrected || !cl_node || cl_node->isOm || cl_node->stale) {
			TRACE_2("ERR_BAD_HANDLE: Reactive ressurect of handle %llx failed",
				immOiHandle);
			if (cl_node && cl_node->stale) {cl_node->exposed = true;}
			rc = SA_AIS_ERR_BAD_HANDLE;
			goto bad_handle;
		}

		TRACE_1("Reactive resurrect of handle %llx succeeded", immOiHandle);
	}

	if (cl_node->mImplementerId == 0) {
		rc = SA_AIS_ERR_BAD_HANDLE;
		TRACE_2("ERR_BAD_HANDLE: No implementer is set for this handle");
		goto bad_handle;
	}

	/* Populate & Send the Open Event to IMMND */
	memset(&evt, 0, sizeof(IMMSV_EVT));
	evt.type = IMMSV_EVT_TYPE_IMMND;
	evt.info.immnd.type = IMMND_EVT_A2ND_OI_OBJ_IMPL_REL;
	evt.info.immnd.info.implSet.client_hdl = cl_node->handle;
	evt.info.immnd.info.implSet.impl_name.size = nameLen;
	evt.info.immnd.info.implSet.impl_name.buf = calloc(1, nameLen);
	strncpy(evt.info.immnd.info.implSet.impl_name.buf, (char *)objectName->value, nameLen);
	evt.info.immnd.info.implSet.impl_id = cl_node->mImplementerId;
	evt.info.immnd.info.implSet.scope = scope;

	imma_proc_increment_pending_reply(cl_node);

	rc = imma_evt_fake_evs(cb, &evt, &out_evt, cl_node->syncr_timeout, cl_node->handle, &locked, true);

	cl_node=NULL;
	free(evt.info.immnd.info.implSet.impl_name.buf);
	evt.info.immnd.info.implSet.impl_name.buf = NULL;
	evt.info.immnd.info.implSet.impl_name.size = 0;

	if (!locked && m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != 
		NCSCC_RC_SUCCESS) {
		rc = SA_AIS_ERR_LIBRARY; /* Overwrites any error from fake_evs() */
		/* Losing track of the pending reply count, but ERR_LIBRARY dominates*/
		TRACE_4("ERR_LIBRARY: LOCK failed");
		goto lock_fail;
	}
	locked = true;

	/* get the client_info */
	imma_client_node_get(&cb->client_tree, &immOiHandle, &cl_node);
	if (!cl_node || cl_node->isOm) {
		rc = SA_AIS_ERR_BAD_HANDLE;
		TRACE_2("ERR_BAD_HANDLE: client_node_get failed");
		goto bad_handle;
	}

	imma_proc_decrement_pending_reply(cl_node);

	if (rc != SA_AIS_OK) {
		/* fake_evs returned error */
		goto fevs_error;
	}

	if (cl_node->stale) {
		TRACE_3("Handle %llx is stale after successfull call - ignoring", 
			immOiHandle);
		/*
		  This is object implementer release => a relaxation
		  => not necessary to expose when the call succeeded.
		  rc = SA_AIS_ERR_BAD_HANDLE;
		  cl_node->exposed = true;
		  goto bad_handle;
		*/
	}

	osafassert(out_evt);
	osafassert(out_evt->type == IMMSV_EVT_TYPE_IMMA);
	osafassert(out_evt->info.imma.type == IMMA_EVT_ND2A_IMM_ERROR);
	rc = out_evt->info.imma.info.errRsp.error;

 fevs_error:
 bad_handle:
	if (locked)
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);

 lock_fail:
	if (out_evt)
		free(out_evt);

	return rc;
}

SaAisErrorT saImmOiRtObjectUpdate_2(SaImmOiHandleT immOiHandle,
				    const SaNameT *objectName, const SaImmAttrModificationT_2 **attrMods)
{
	SaAisErrorT rc = SA_AIS_OK;
	uint32_t proc_rc = NCSCC_RC_SUCCESS;
	IMMA_CB *cb = &imma_cb;
	IMMSV_EVT evt;
	IMMSV_EVT *out_evt = NULL;
	IMMA_CLIENT_NODE *cl_node = NULL;
	bool locked = true;
	SaUint32T timeout = 0;

	if (cb->sv_id == 0) {
		TRACE_2("ERR_BAD_HANDLE: No initialized handle exists!");
		return SA_AIS_ERR_BAD_HANDLE;
	}

	TRACE_ENTER();

	if ((objectName == NULL) || (objectName->length >= SA_MAX_NAME_LENGTH) || 
		(objectName->length == 0)) {
		TRACE_2("ERR_INVALID_PARAM: objectName is NULL or length is 0 or "
			"length is greater than %u", SA_MAX_NAME_LENGTH);
		TRACE_LEAVE();
		return SA_AIS_ERR_INVALID_PARAM;
	}

	if (attrMods == NULL) {
		TRACE_2("ERR_INVALID_PARAM: attrMods is NULL");
		TRACE_LEAVE();
		return SA_AIS_ERR_INVALID_PARAM;
	}

	if (false == cb->is_immnd_up) {
		TRACE_2("ERR_TRY_AGAIN: IMMND is DOWN");
		return SA_AIS_ERR_TRY_AGAIN;
	}

	/* get the CB Lock */
	if (m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
		rc = SA_AIS_ERR_LIBRARY;
		TRACE_4("ERR_LIBRARY: Lock failed");
		goto lock_fail;
	}
	/*locked == true already */

	imma_client_node_get(&cb->client_tree, &immOiHandle, &cl_node);
	if (!cl_node || cl_node->isOm) {
		rc = SA_AIS_ERR_BAD_HANDLE;
		TRACE_2("ERR_BAD_HANDLE: Non valid SaImmOiHandleT");
		goto bad_handle;
	}

	if (cl_node->stale) {
		TRACE_1("Handle %llx is stale", immOiHandle);
		bool resurrected = imma_oi_resurrect(cb, cl_node, &locked, &rc);
		if(rc == SA_AIS_ERR_TRY_AGAIN) {
			osafassert(!resurrected);
			goto bad_handle; /* Handle is actually not bad yet. */
		}

		if (!locked && m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
			TRACE_4("ERR_LIBRARY: LOCK failed");
			rc = SA_AIS_ERR_LIBRARY;
			goto lock_fail;
		}
		locked = true;

		imma_client_node_get(&cb->client_tree, &immOiHandle, &cl_node);

		if (!resurrected || !cl_node || cl_node->isOm || cl_node->stale) {
			TRACE_2("ERR_BAD_HANDLE: Reactive ressurect of handle %llx failed",
				immOiHandle);
			if (cl_node && cl_node->stale) {cl_node->exposed = true;}
			rc = SA_AIS_ERR_BAD_HANDLE;
			goto bad_handle;
		}

		TRACE_1("Reactive resurrect of handle %llx succeeded", immOiHandle);
	}

	if (cl_node->mImplementerId == 0) {
		rc = SA_AIS_ERR_BAD_HANDLE;
		TRACE_2("ERR_BAD_HANDLE: The SaImmOiHandleT is not associated with any implementer name");
		goto bad_handle;
	}

	if (cl_node->isApplier) {
		rc = SA_AIS_ERR_BAD_HANDLE;
		TRACE_2("ERR_BAD_HANDLE: The SaImmOiHandleT is associated with an >>applier<< name");
		goto bad_handle;
	}

	timeout = cl_node->syncr_timeout;

	/* Populate the Object-Update event */
	memset(&evt, 0, sizeof(IMMSV_EVT));
	evt.type = IMMSV_EVT_TYPE_IMMND;
	evt.info.immnd.type = IMMND_EVT_A2ND_OI_OBJ_MODIFY;

	evt.info.immnd.info.objModify.immHandle = immOiHandle;

	/*NOTE: should rename member adminOwnerId !!! */
	evt.info.immnd.info.objModify.adminOwnerId = cl_node->mImplementerId;

	if (objectName->length) {
		evt.info.immnd.info.objModify.objectName.size = strlen((char *)objectName->value) + 1;

		if (objectName->length + 1 < evt.info.immnd.info.objModify.objectName.size) {
			evt.info.immnd.info.objModify.objectName.size = objectName->length + 1;
		}

		/*alloc-1 */
		evt.info.immnd.info.objModify.objectName.buf = calloc(1, evt.info.immnd.info.objModify.objectName.size);
		strncpy(evt.info.immnd.info.objModify.objectName.buf,
			(char *)objectName->value, evt.info.immnd.info.objModify.objectName.size);
		evt.info.immnd.info.objModify.objectName.buf[evt.info.immnd.info.objModify.objectName.size - 1] = '\0';
	} else {
		evt.info.immnd.info.objModify.objectName.size = 0;
		evt.info.immnd.info.objModify.objectName.buf = NULL;
	}

	osafassert(evt.info.immnd.info.objModify.attrMods == NULL);

	const SaImmAttrModificationT_2 *attrMod;
	int i;
	for (i = 0; attrMods[i]; ++i) {
		attrMod = attrMods[i];

		/* TODO Check that the user does not set values for System attributes. */

		/* Prevent duplicate attribute assignments */
		IMMSV_ATTR_MODS_LIST *p = evt.info.immnd.info.objModify.attrMods;
		while(p!=NULL) {
			if(strcmp(attrMod->modAttr.attrName,  p->attrValue.attrName.buf) == 0) {
				rc = SA_AIS_ERR_INVALID_PARAM;
				TRACE_2("ERR_INVALID_PARAM: Attribute %s occurs multiple times "
					"in attrMods parameter", attrMod->modAttr.attrName);
				goto skip_over_send;
			}

			p = p->next;
		}

		/*alloc-2 */
		p = calloc(1, sizeof(IMMSV_ATTR_MODS_LIST));
		p->attrModType = attrMod->modType;
		p->attrValue.attrName.size = strlen(attrMod->modAttr.attrName) + 1;

		/* alloc 3 */
		p->attrValue.attrName.buf = malloc(p->attrValue.attrName.size);
		strncpy(p->attrValue.attrName.buf, attrMod->modAttr.attrName, p->attrValue.attrName.size);

		p->attrValue.attrValuesNumber = attrMod->modAttr.attrValuesNumber;
		p->attrValue.attrValueType = attrMod->modAttr.attrValueType;

		if (attrMod->modAttr.attrValuesNumber) {	/*At least one value */
			const SaImmAttrValueT *avarr = attrMod->modAttr.attrValues;
			/*alloc-4 */
			imma_copyAttrValue(&(p->attrValue.attrValue), attrMod->modAttr.attrValueType, avarr[0]);

			if (attrMod->modAttr.attrValuesNumber > 1) {	/*Multiple values */
				unsigned int numAdded = attrMod->modAttr.attrValuesNumber - 1;
				unsigned int i;
				for (i = 1; i <= numAdded; ++i) {
					/*alloc-5 */
					IMMSV_EDU_ATTR_VAL_LIST *al = calloc(1, sizeof(IMMSV_EDU_ATTR_VAL_LIST));
					/*alloc-6 */
					imma_copyAttrValue(&(al->n), attrMod->modAttr.attrValueType, avarr[i]);
					al->next = p->attrValue.attrMoreValues;	/*NULL initially */
					p->attrValue.attrMoreValues = al;
				}	/*for */
			}	/*Multiple values */
		} /*At least one value */
		else {
			TRACE_3("Strange update of attribute %s, without any modifications", 
				attrMod->modAttr.attrName);
		}
		p->next = evt.info.immnd.info.objModify.attrMods;	/*NULL initially. */
		evt.info.immnd.info.objModify.attrMods = p;
	}

	/* We do not send the rt update over fevs, because the update may
	   often be a PURELY LOCAL update, by an object implementor reacting to
	   the SaImmOiRtAttrUpdateCallbackT upcall. In that local case, the 
	   return of the upcall is the signal that the new values are ready, not
	   the invocation of this update call. */

	imma_proc_increment_pending_reply(cl_node);

	/* Release the CB lock Before MDS Send */
	m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
	locked = false;
	cl_node =NULL;

	if (false == cb->is_immnd_up) {
		rc = SA_AIS_ERR_TRY_AGAIN;
		TRACE_2("ERR_TRY_AGAIN: IMMND is DOWN");
		goto mds_send_fail;
	}

	/* send the request to the IMMND */
	proc_rc = imma_mds_msg_sync_send(cb->imma_mds_hdl, &cb->immnd_mds_dest, &evt, &out_evt, timeout);

	/* Error Handling */
	switch (proc_rc) {
		case NCSCC_RC_SUCCESS:
			break;
		case NCSCC_RC_REQ_TIMOUT:
			rc = imma_proc_check_stale(cb, immOiHandle, SA_AIS_ERR_TIMEOUT);
			break;  /* i.e. goto mds_send_fail */
		default:
			TRACE_4("ERR_LIBRARY: MDS returned unexpected error code %u", proc_rc);
			rc = SA_AIS_ERR_LIBRARY;
			/* Losing track of the pending reply count, but ERR_LIBRARY dominates*/
			goto bad_handle;
	}

 mds_send_fail:

	if (m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
		rc = SA_AIS_ERR_LIBRARY;
		/* Losing track of the pending reply count, but ERR_LIBRARY dominates*/
		TRACE_4("ERR_LIBRARY: Lock failed");
		goto free_obj; /* locked will be false */
	}
	locked = true;

	imma_client_node_get(&cb->client_tree, &immOiHandle, &cl_node);
	if (!cl_node || cl_node->isOm) {
		rc = SA_AIS_ERR_BAD_HANDLE;
		TRACE_2("ERR_BAD_HANDLE: client_node_get failed");
		goto bad_handle1;
	}

	imma_proc_decrement_pending_reply(cl_node);

	if (cl_node->stale) {
		TRACE_1("Handle %llx is stale", immOiHandle);
		rc = SA_AIS_ERR_BAD_HANDLE;
		cl_node->exposed = true;
		goto bad_handle1;
	}

	if (out_evt) {
		/* Process the outcome, note this is after a blocking call. */
		osafassert(out_evt->type == IMMSV_EVT_TYPE_IMMA);
		osafassert(out_evt->info.imma.type == IMMA_EVT_ND2A_IMM_ERROR);
		if (rc == SA_AIS_OK) {
			rc = out_evt->info.imma.info.errRsp.error;
		}
	}

 skip_over_send:
 free_obj:
 bad_handle1:
	if (evt.info.immnd.info.objModify.objectName.buf) {	/*free-1 */
		free(evt.info.immnd.info.objModify.objectName.buf);
		evt.info.immnd.info.objModify.objectName.buf = NULL;
	}

	while (evt.info.immnd.info.objModify.attrMods) {

		IMMSV_ATTR_MODS_LIST *p = evt.info.immnd.info.objModify.attrMods;
		evt.info.immnd.info.objModify.attrMods = p->next;
		p->next = NULL;

		if (p->attrValue.attrName.buf) {
			free(p->attrValue.attrName.buf);	/*free-3 */
			p->attrValue.attrName.buf = NULL;
		}

		if (p->attrValue.attrValuesNumber) {
			immsv_evt_free_att_val(&(p->attrValue.attrValue),	/*free-4 */
					       p->attrValue.attrValueType);

			while (p->attrValue.attrMoreValues) {
				IMMSV_EDU_ATTR_VAL_LIST *al = p->attrValue.attrMoreValues;
				p->attrValue.attrMoreValues = al->next;
				al->next = NULL;
				immsv_evt_free_att_val(&(al->n), p->attrValue.attrValueType);	/*free-6 */
				free(al);	/*free-5 */
			}
		}

		free(p);	/*free-2 */
	}

 bad_handle:
	if (locked) {
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
	}

 lock_fail:
	if (out_evt)
		free(out_evt);

	TRACE_LEAVE();
	return rc;
}

extern SaAisErrorT saImmOiRtObjectCreate_2(SaImmOiHandleT immOiHandle,
					   const SaImmClassNameT className,
					   const SaNameT *parentName, const SaImmAttrValuesT_2 **attrValues)
{
	SaAisErrorT rc = SA_AIS_OK;
	IMMA_CB *cb = &imma_cb;
	IMMSV_EVT evt;
	IMMSV_EVT *out_evt = NULL;
	IMMA_CLIENT_NODE *cl_node = NULL;
	bool locked = true;

	if (cb->sv_id == 0) {
		TRACE_2("ERR_BAD_HANDLE: No initialized handle exists!");
		return SA_AIS_ERR_BAD_HANDLE;
	}

	if (className == NULL) {
		TRACE_2("ERR_INVALID_PARAM: classname is NULL");
		return SA_AIS_ERR_INVALID_PARAM;
	}

	if (attrValues == NULL) {
		TRACE_2("ERR_INVALID_PARAM: attrValues is NULL");
		return SA_AIS_ERR_INVALID_PARAM;
	}

	if (parentName && parentName->length >= SA_MAX_NAME_LENGTH) {
		return SA_AIS_ERR_NAME_TOO_LONG;
	}

	if (cb->is_immnd_up == false) {
		TRACE_2("ERR_TRY_AGAIN: IMMND is DOWN");
		return SA_AIS_ERR_TRY_AGAIN;
	}

	TRACE_ENTER();
	/* get the CB Lock */
	if (m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
		rc = SA_AIS_ERR_LIBRARY;
		TRACE_4("ERR_LIBRARY: Lock failed");
		goto lock_fail;
	}
	/* locked == true already*/

	imma_client_node_get(&cb->client_tree, &immOiHandle, &cl_node);
	if (!cl_node || cl_node->isOm) {
		rc = SA_AIS_ERR_BAD_HANDLE;
		TRACE_2("ERR_BAD_HANDLE: Non valid SaImmOiHandleT");
		goto bad_handle;
	}

	if (cl_node->stale) {
		TRACE_1("Handle %llx is stale", immOiHandle);
		bool resurrected = imma_oi_resurrect(cb, cl_node, &locked, &rc);
		if(rc == SA_AIS_ERR_TRY_AGAIN) {
			osafassert(!resurrected);
			goto bad_handle; /* Handle is actually not bad yet. */
		}

		if (!locked && m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
			TRACE_4("ERR_LIBRARY: LOCK failed");
			rc = SA_AIS_ERR_LIBRARY;
			goto lock_fail;
		}
		locked = true;

		imma_client_node_get(&cb->client_tree, &immOiHandle, &cl_node);

		if (!resurrected || !cl_node || cl_node->isOm || cl_node->stale) {
			TRACE_2("ERR_BAD_HANDLE: Reactive ressurect of handle %llx failed",
				immOiHandle);
			if (cl_node && cl_node->stale) {cl_node->exposed = true;}
			rc = SA_AIS_ERR_BAD_HANDLE;
			goto bad_handle;
		}

		TRACE_1("Reactive resurrect of handle %llx succeeded", immOiHandle);
	}

	if (cl_node->mImplementerId == 0) {
		rc = SA_AIS_ERR_BAD_HANDLE;
		TRACE_2("ERR_BAD_HANDLE: The SaImmOiHandleT is not associated with any implementer name");
		goto bad_handle;
	}

	if (cl_node->isApplier) {
		rc = SA_AIS_ERR_BAD_HANDLE;
		TRACE_2("ERR_BAD_HANDLE: The SaImmOiHandleT is associated with an >>applier<< name");
		goto bad_handle;
	}

	/* Populate the Object-Create event */
	memset(&evt, 0, sizeof(IMMSV_EVT));
	evt.type = IMMSV_EVT_TYPE_IMMND;
	evt.info.immnd.type = IMMND_EVT_A2ND_OI_OBJ_CREATE;

	/* NOTE: should rename member adminOwnerId !!! */
	evt.info.immnd.info.objCreate.adminOwnerId = cl_node->mImplementerId;
	evt.info.immnd.info.objCreate.className.size = strlen(className) + 1;

	/*alloc-1 */
	evt.info.immnd.info.objCreate.className.buf = malloc(evt.info.immnd.info.objCreate.className.size);
	strncpy(evt.info.immnd.info.objCreate.className.buf, className, evt.info.immnd.info.objCreate.className.size);

	if (parentName && parentName->length) {
		evt.info.immnd.info.objCreate.parentName.size = strlen((char *)parentName->value) + 1;

		if (parentName->length + 1 < evt.info.immnd.info.objCreate.parentName.size) {
			evt.info.immnd.info.objCreate.parentName.size = parentName->length + 1;
		}

		/*alloc-2 */
		evt.info.immnd.info.objCreate.parentName.buf = calloc(1, evt.info.immnd.info.objCreate.parentName.size);
		strncpy(evt.info.immnd.info.objCreate.parentName.buf,
			(char *)parentName->value, evt.info.immnd.info.objCreate.parentName.size);
		evt.info.immnd.info.objCreate.parentName.buf[evt.info.immnd.info.objCreate.parentName.size - 1] = '\0';
	} else {
		evt.info.immnd.info.objCreate.parentName.size = 0;
		evt.info.immnd.info.objCreate.parentName.buf = NULL;
	}

	osafassert(evt.info.immnd.info.objCreate.attrValues == NULL);

	const SaImmAttrValuesT_2 *attr;
	int i;
	for (i = 0; attrValues[i]; ++i) {
		attr = attrValues[i];
		TRACE("attr:%s \n", attr->attrName);

		/* Prevent duplicate attribute assignments */
		IMMSV_ATTR_VALUES_LIST *p = evt.info.immnd.info.objCreate.attrValues;
		while(p!= NULL) {
			if(strcmp(attr->attrName, p->n.attrName.buf) == 0) {
				rc = SA_AIS_ERR_INVALID_PARAM;
				TRACE_2("ERR_INVALID_PARAM: Attribute %s occurs multiple times "
					"in attrValues parameter", attr->attrName);
				goto mds_send_fail;
			}
			p = p->next;
		}

		/*Check that the user does not set value for System attributes. */

		if (strcmp(attr->attrName, sysaClName) == 0) {
			rc = SA_AIS_ERR_INVALID_PARAM;
			TRACE_2("ERR_INVALID_PARAM: Not allowed to set attribute %s ", sysaClName);
			goto mds_send_fail;
		} else if (strcmp(attr->attrName, sysaAdmName) == 0) {
			rc = SA_AIS_ERR_INVALID_PARAM;
			TRACE_2("ERR_INVALID_PARAM: Not allowed to set attribute %s", sysaAdmName);
			goto mds_send_fail;
		} else if (strcmp(attr->attrName, sysaImplName) == 0) {
			rc = SA_AIS_ERR_INVALID_PARAM;
			TRACE_2("ERR_INVALID_PARAM: Not allowed to set attribute %s", sysaImplName);
			goto mds_send_fail;
		} else if (attr->attrValuesNumber == 0) {
			TRACE("RtObjectCreate ignoring attribute %s with no values", attr->attrName);
			continue;
		}

		/*alloc-3 */
		p = calloc(1, sizeof(IMMSV_ATTR_VALUES_LIST));

		p->n.attrName.size = strlen(attr->attrName) + 1;
		if (p->n.attrName.size >= SA_MAX_NAME_LENGTH) {
			TRACE_2("ERR_INVALID_PARAM: Attribute name too long");
			rc = SA_AIS_ERR_INVALID_PARAM;
			free(p);
			goto mds_send_fail;
		}

		/*alloc-4 */
		p->n.attrName.buf = malloc(p->n.attrName.size);
		strncpy(p->n.attrName.buf, attr->attrName, p->n.attrName.size);

		p->n.attrValuesNumber = attr->attrValuesNumber;
		p->n.attrValueType = attr->attrValueType;

		const SaImmAttrValueT *avarr = attr->attrValues;
		/*alloc-5 */
		imma_copyAttrValue(&(p->n.attrValue), attr->attrValueType, avarr[0]);

		if (attr->attrValuesNumber > 1) {
			unsigned int numAdded = attr->attrValuesNumber - 1;
			unsigned int i;
			for (i = 1; i <= numAdded; ++i) {
				/*alloc-6 */
				IMMSV_EDU_ATTR_VAL_LIST *al = calloc(1, sizeof(IMMSV_EDU_ATTR_VAL_LIST));

				/*alloc-7 */
				imma_copyAttrValue(&(al->n), attr->attrValueType, avarr[i]);
				al->next = p->n.attrMoreValues;
				p->n.attrMoreValues = al;
			}
		}

		p->next = evt.info.immnd.info.objCreate.attrValues;	/*NULL initially. */
		evt.info.immnd.info.objCreate.attrValues = p;
	}

	imma_proc_increment_pending_reply(cl_node);

	rc = imma_evt_fake_evs(cb, &evt, &out_evt, cl_node->syncr_timeout, cl_node->handle, &locked, true);

	cl_node=NULL;

	if (!locked && m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != 
		NCSCC_RC_SUCCESS) {
		rc = SA_AIS_ERR_LIBRARY; /* Overwrites any error from fake_evs() */
		/* Losing track of the pending reply count, but ERR_LIBRARY dominates*/
		TRACE_4("ERR_LIBRARY: LOCK failed");
		goto mds_send_fail;
	}
	locked = true;

	/* get the client_info */
	imma_client_node_get(&cb->client_tree, &immOiHandle, &cl_node);
	if (!cl_node || cl_node->isOm) {
		rc = SA_AIS_ERR_BAD_HANDLE;
		TRACE_2("ERR_BAD_HANDLE: client_node_get failed");
		goto mds_send_fail;
	}

	imma_proc_decrement_pending_reply(cl_node);

	if (rc != SA_AIS_OK) {
		/* fake_evs returned error */
		goto mds_send_fail;
	}

	if (cl_node->stale) {
		rc = SA_AIS_ERR_BAD_HANDLE;
		cl_node->exposed = true;
		TRACE_2("ERR_BAD_HANDLE: Handle %llx is stale", immOiHandle);
		goto mds_send_fail;
	}

	osafassert(out_evt);
	osafassert(out_evt->type == IMMSV_EVT_TYPE_IMMA);
	osafassert(out_evt->info.imma.type == IMMA_EVT_ND2A_IMM_ERROR);
	rc = out_evt->info.imma.info.errRsp.error;

 mds_send_fail:
	if (evt.info.immnd.info.objCreate.className.buf) {	/*free-1 */
		free(evt.info.immnd.info.objCreate.className.buf);
		evt.info.immnd.info.objCreate.className.buf = NULL;
	}

	if (evt.info.immnd.info.objCreate.parentName.buf) {	/*free-2 */
		free(evt.info.immnd.info.objCreate.parentName.buf);
		evt.info.immnd.info.objCreate.parentName.buf = NULL;
	}

	while (evt.info.immnd.info.objCreate.attrValues) {
		IMMSV_ATTR_VALUES_LIST *p = evt.info.immnd.info.objCreate.attrValues;
		evt.info.immnd.info.objCreate.attrValues = p->next;
		p->next = NULL;
		if (p->n.attrName.buf) {	/*free-4 */
			free(p->n.attrName.buf);
			p->n.attrName.buf = NULL;
		}

		immsv_evt_free_att_val(&(p->n.attrValue), p->n.attrValueType);	/*free-5 */

		while (p->n.attrMoreValues) {
			IMMSV_EDU_ATTR_VAL_LIST *al = p->n.attrMoreValues;
			p->n.attrMoreValues = al->next;
			al->next = NULL;
			immsv_evt_free_att_val(&(al->n), p->n.attrValueType);	/*free-7 */
			free(al);	/*free-6 */
		}

		p->next = NULL;
		free(p);	/*free-3 */
	}

 bad_handle:
	if (locked) {
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
	}

 lock_fail:
	if (out_evt)
		free(out_evt);

	TRACE_LEAVE();
	return rc;
}

SaAisErrorT saImmOiRtObjectDelete(SaImmOiHandleT immOiHandle, const SaNameT *objectName)
{
	SaAisErrorT rc = SA_AIS_OK;
	IMMA_CB *cb = &imma_cb;
	IMMSV_EVT evt;
	IMMSV_EVT *out_evt = NULL;
	IMMA_CLIENT_NODE *cl_node = NULL;
	bool locked = true;

	if (cb->sv_id == 0) {
		TRACE_2("ERR_BAD_HANDLE: No initialized handle exists!");
		return SA_AIS_ERR_BAD_HANDLE;
	}

	if (!objectName || (objectName->length == 0)) {
		TRACE_2("ERR_INVALID_PARAM: Empty object-name");
		return SA_AIS_ERR_INVALID_PARAM;
	}

	if (objectName->length >= SA_MAX_NAME_LENGTH) {
		TRACE_2("ERR_INVALID_PARAM: Object name too long");
		return SA_AIS_ERR_INVALID_PARAM;
	}

	if (cb->is_immnd_up == false) {
		TRACE_2("ERR_TRY_AGAIN: IMMND is DOWN");
		return SA_AIS_ERR_TRY_AGAIN;
	}

	/* get the CB Lock */
	if (m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
		rc = SA_AIS_ERR_LIBRARY;
		TRACE_4("ERR_LIBRARY: Lock failed");
		goto lock_fail;
	}
	/*locked == true already*/

	imma_client_node_get(&cb->client_tree, &immOiHandle, &cl_node);
	if (!cl_node || cl_node->isOm) {
		rc = SA_AIS_ERR_BAD_HANDLE;
		TRACE_2("ERR_BAD_HANDLE: Not a valid SaImmOiHandleT");
		goto bad_handle;
	}

	if (cl_node->stale) {
		TRACE_1("Handle %llx is stale", immOiHandle);
		bool resurrected = imma_oi_resurrect(cb, cl_node, &locked, &rc);
		if(rc == SA_AIS_ERR_TRY_AGAIN) {
			osafassert(!resurrected);
			goto bad_handle; /* Handle is actually not bad yet. */
		}

		if (!locked && m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
			TRACE_4("ERR_LIBRARY: LOCK failed");
			rc = SA_AIS_ERR_LIBRARY;
			goto lock_fail;
		}
		locked = true;

		imma_client_node_get(&cb->client_tree, &immOiHandle, &cl_node);

		if (!resurrected || !cl_node || cl_node->isOm || cl_node->stale) {
			TRACE_2("ERR_BAD_HANDLE: Reactive ressurect of handle %llx failed",
				immOiHandle);
			if (cl_node && cl_node->stale) {cl_node->exposed = true;}
			rc = SA_AIS_ERR_BAD_HANDLE;
			goto bad_handle;
		}

		TRACE_1("Reactive resurrect of handle %llx succeeded", immOiHandle);
	}

	if (cl_node->mImplementerId == 0) {
		rc = SA_AIS_ERR_BAD_HANDLE;
		TRACE_2("ERR_BAD_HANDLE: The SaImmOiHandleT is not associated with any implementer name");
		goto bad_handle;
	}

	if (cl_node->isApplier) {
		rc = SA_AIS_ERR_BAD_HANDLE;
		TRACE_2("ERR_BAD_HANDLE: The SaImmOiHandleT is associated with an >>applier<< name");
		goto bad_handle;
	}

	/* Populate the Object-Delete event */
	memset(&evt, 0, sizeof(IMMSV_EVT));
	evt.type = IMMSV_EVT_TYPE_IMMND;
	evt.info.immnd.type = IMMND_EVT_A2ND_OI_OBJ_DELETE;

	/* NOTE: should rename member adminOwnerId !!! */
	evt.info.immnd.info.objDelete.adminOwnerId = cl_node->mImplementerId;

	evt.info.immnd.info.objDelete.objectName.size = strlen((char *)objectName->value) + 1;

	if (objectName->length + 1 < evt.info.immnd.info.objDelete.objectName.size) {
		evt.info.immnd.info.objDelete.objectName.size = objectName->length + 1;
	}

	/*alloc-1 */
	evt.info.immnd.info.objDelete.objectName.buf = calloc(1, evt.info.immnd.info.objDelete.objectName.size);
	strncpy(evt.info.immnd.info.objDelete.objectName.buf,
		(char *)objectName->value, evt.info.immnd.info.objDelete.objectName.size);
	evt.info.immnd.info.objDelete.objectName.buf[evt.info.immnd.info.objDelete.objectName.size - 1] = '\0';

	imma_proc_increment_pending_reply(cl_node);

	rc = imma_evt_fake_evs(cb, &evt, &out_evt, cl_node->syncr_timeout, cl_node->handle, &locked, true);

	cl_node = NULL;

	if (!locked && m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != 
		NCSCC_RC_SUCCESS) {
		rc = SA_AIS_ERR_LIBRARY; /* Overwrites any error from fake_evs() */
		/* Losing track of the pending reply count, but ERR_LIBRARY dominates*/
		TRACE_4("ERR_LIBRARY: LOCK failed");
		goto cleanup;
	}
	locked = true;

	/* get the client_info */
	imma_client_node_get(&cb->client_tree, &immOiHandle, &cl_node);
	if (!cl_node || cl_node->isOm) {
		rc = SA_AIS_ERR_BAD_HANDLE;
		TRACE_2("ERR_BAD_HANDLE: client_node_get failed");
		goto cleanup;
	}

	imma_proc_decrement_pending_reply(cl_node);

	if (rc != SA_AIS_OK) {
		/* fake_evs returned error */
		goto cleanup;
	}

	if (cl_node->stale) {
		rc = SA_AIS_ERR_BAD_HANDLE;
		cl_node->exposed = true;
		TRACE_2("ERR_BAD_HANDLE: Handle %llx is stale", immOiHandle);
		goto cleanup;
	}

	osafassert(out_evt);
	osafassert(out_evt->type == IMMSV_EVT_TYPE_IMMA);
	osafassert(out_evt->info.imma.type == IMMA_EVT_ND2A_IMM_ERROR);
	rc = out_evt->info.imma.info.errRsp.error;

 cleanup:
	if (evt.info.immnd.info.objDelete.objectName.buf) {	/*free-1 */
		free(evt.info.immnd.info.objDelete.objectName.buf);
		evt.info.immnd.info.objDelete.objectName.buf = NULL;
	}

 bad_handle:
	if (locked)
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
 lock_fail:
	if (out_evt)
		free(out_evt);

	return rc;
}

/*
  Tries to set implementer for resurrected handle.
  If it fails, then resurrection must be reverted
  and stale+exposed must be set on client node by
  invoking code.

  cb_lock must NOT be locked on entry.
*/
static SaBoolT imma_implementer_set(IMMA_CB *cb, SaImmOiHandleT immOiHandle)
{
	SaAisErrorT err = SA_AIS_OK;
	unsigned int sleep_delay_ms = 200;
	unsigned int max_waiting_time_ms = 1 * 1000;	/* 1 secs */
	unsigned int msecs_waited = 0;
	SaImmOiImplementerNameT implName;
	IMMA_CLIENT_NODE *cl_node = NULL;
	bool locked = false;
	TRACE_ENTER();

	if (m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
		TRACE_3("Lock failure");
		goto fail;
	}
	locked = true;

	imma_client_node_get(&cb->client_tree, &immOiHandle, &cl_node);
	if (!cl_node || cl_node->isOm || cl_node->stale) {
		TRACE_3("client_node_get failed");
		goto fail;
	}

	implName = cl_node->mImplementerName;
	cl_node->mImplementerName = NULL;
	cl_node->mImplementerId = 0;

	m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
	locked = false;
	cl_node=NULL;

	if (!implName) {
		/* No implementer to re-connect with. */
		goto success;
	}

	err = saImmOiImplementerSet(immOiHandle, implName);
	TRACE_1("saImmOiImplementerSet returned %u", err);

	while ((err == SA_AIS_ERR_TRY_AGAIN)&&
		(msecs_waited < max_waiting_time_ms)) {
		usleep(sleep_delay_ms * 1000);
		msecs_waited += sleep_delay_ms;
		err = saImmOiImplementerSet(immOiHandle, implName);
		TRACE_1("saImmOiImplementerSet returned %u", err);
	}
	free(implName);
	/*We dont need to set class/object implementer again 
	  because that association is still maintained by the 
	  distributed IMMSv over IMMND crashes. 
	*/

	if (err != SA_AIS_OK) {
		/* Note: cl_node->mImplementerName is now NULL
		   The failed implementer-set was then destructive
		   on the handle. But the resurrect shall now be
		   reverted (handle finalized towards IMMND) and
		   cl_node->stale/exposed both set to true.
		 */
		goto fail;
	}

 success:
	osafassert(!locked);
	TRACE_LEAVE();
	return SA_TRUE;

 fail:
	if (locked) {m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);}
	TRACE_LEAVE();
	return SA_FALSE;
}

int imma_oi_resurrect(IMMA_CB *cb, IMMA_CLIENT_NODE *cl_node, bool *locked, SaAisErrorT * err_cli_res)
{
	IMMSV_EVT  finalize_evt, *out_evt = NULL;
	TRACE_ENTER();
	osafassert(locked && *locked);
	osafassert(cl_node && cl_node->stale);
	SaImmOiHandleT immOiHandle = cl_node->handle;
	SaUint32T timeout = 0;

	m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
	*locked = false;
	cl_node = NULL;
	if (!imma_proc_resurrect_client(cb, immOiHandle, false, err_cli_res)) {
		TRACE_3("Failed to resurrect OI handle <c:%u, n:%x>",
			m_IMMSV_UNPACK_HANDLE_HIGH(immOiHandle),
			m_IMMSV_UNPACK_HANDLE_LOW(immOiHandle));
		goto fail;
	}

	TRACE_1("Successfully resurrected OI handle <c:%u, n:%x>",
		m_IMMSV_UNPACK_HANDLE_HIGH(immOiHandle),
		m_IMMSV_UNPACK_HANDLE_LOW(immOiHandle));

	/* Set implementer if needed. */
	if (imma_implementer_set(cb, immOiHandle)) {
		goto success;
	}

	TRACE_3("Failed to set implementer for resurrected "
		"OI handle - reverting resurrection");

	if (m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
		TRACE_3("Lock failure");
		goto fail;
	}
	*locked = true;

	imma_client_node_get(&cb->client_tree, &immOiHandle, &cl_node);
	if (cl_node && !cl_node->isOm) {
		cl_node->stale = true;
		cl_node->exposed = true;
	} else {
		TRACE_3("client_node_get failed");
	}

	timeout = cl_node->syncr_timeout;

	m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
	*locked = false;
	cl_node = NULL;

	/* Finalize the just resurrected handle ! */
	memset(&finalize_evt, 0, sizeof(IMMSV_EVT));
	finalize_evt.type = IMMSV_EVT_TYPE_IMMND;
	finalize_evt.info.immnd.type = IMMND_EVT_A2ND_IMM_OI_FINALIZE;
	finalize_evt.info.immnd.info.finReq.client_hdl = immOiHandle;

	/* send a finalize handle req to the IMMND.
	   Dont bother checking the answer. This is just
	   an attempt to deallocate the useless resurrected
	   handle on the server side. 
	*/

	if (cb->is_immnd_up)  {
		imma_mds_msg_sync_send(cb->imma_mds_hdl, 
			&(cb->immnd_mds_dest),&finalize_evt,&out_evt, timeout);

		/* Dont care about the response on finalize. */
		if (out_evt) {free(out_evt);}
	}

	/* Even though we finalized the resurrected handle towards IMMND,
	   we dont remove the client_node because this is just a dispatch.
	   Instead we leave it to the application to explicitly finalize its
	   handle.
	*/

 fail:
	TRACE_LEAVE();
	/* may be locked or unlocked as reflected in *locked */
	return 0;

 success:
	TRACE_LEAVE();
	/* may be locked or unlocked as reflected in *locked */
	return 1;
}

/****************************************************************************
  Name          :  saImmOiCcbSetErrorString
 
  Description   :  Allows the OI implementers to post an error string related
                   to a current ccb-upcall, before returning from the upcall.
                   The string will be transmitted to the OM client if the OI
                   returns with an error code (not ok). This is allowed inside:
                    SaImmOiCcbObjectCreateCallbackT
                    SaImmOiCcbObjectDeleteCallbackT
                    SaImmOiCcbObjectModifyCallbackT
                    SaImmOiCcbCompletedCallbackT
                   

                   
  Arguments     :  immOiHandle - IMM OI handle
                   ccbId  -  The ccbId for the ccb related upcall.
                   errorString - The errorString.
 
  Return Values :  SA_AIS_OK
                   SA_AIS_ERR_BAD_HANDLE
                   SA_AIS_ERR_INVALID_PARAM
                   SA_AIS_ERR_BAD_OPERATION (not inside valid ccb upcall)
                   SA_AIS_ERR_LIBRARY
                   SA_AIS_ERR_TRY_AGAIN
                   
******************************************************************************/
SaAisErrorT saImmOiCcbSetErrorString(
				     SaImmOiHandleT immOiHandle, 
				     SaImmOiCcbIdT ccbId,
				     const SaStringT errorString)
{
	SaAisErrorT rc = SA_AIS_OK;
	IMMA_CB *cb = &imma_cb;
	IMMA_CLIENT_NODE *cl_node = NULL;
	bool locked = true;

	if (cb->sv_id == 0) {
		TRACE_2("ERR_BAD_HANDLE: No initialized handle exists!");
		return SA_AIS_ERR_BAD_HANDLE;
	}

	if ((errorString == NULL) || (strlen(errorString) == 0)) {
		TRACE_2("ERR_INVALID_PARAM: Parameter 'srrorString' is NULL, or is a "
			"string of 0 length");
		return SA_AIS_ERR_INVALID_PARAM;
	}

	/* get the CB Lock */
	if (m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
		rc = SA_AIS_ERR_LIBRARY;
		TRACE_4("ERR_LIBRARY: Lock failed");
		goto lock_fail;
	}

	/*locked == true already */

	imma_client_node_get(&cb->client_tree, &immOiHandle, &cl_node);
	if (!cl_node || cl_node->isOm) {
		rc = SA_AIS_ERR_BAD_HANDLE;
		TRACE_2("ERR_BAD_HANDLE: client_node_get failed");
		goto bad_handle;
	}

	if (cl_node->stale) {
		TRACE_1("Handle %llx is stale", immOiHandle);
		bool resurrected = imma_oi_resurrect(cb, cl_node, &locked, &rc);
		if(rc == SA_AIS_ERR_TRY_AGAIN) {
			osafassert(!resurrected);
			goto bad_handle; /* Handle is actually not bad yet. */
		}

		if (!locked && m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
			TRACE_4("ERR_LIBRARY: LOCK failed");
			rc = SA_AIS_ERR_LIBRARY;
			goto lock_fail;
		}
		locked = true;

		imma_client_node_get(&cb->client_tree, &immOiHandle, &cl_node);

		if (!resurrected || !cl_node || cl_node->isOm || cl_node->stale) {
			TRACE_2("ERR_BAD_HANDLE: Reactive ressurect of handle %llx failed", 
				immOiHandle);
			if (cl_node && cl_node->stale) {cl_node->exposed = true;}
			rc = SA_AIS_ERR_BAD_HANDLE;
			goto bad_handle;
		}

		TRACE_1("Reactive resurrect of handle %llx succeeded", immOiHandle);
	}

	if(!(cl_node->isImmA2b)) {
		rc = SA_AIS_ERR_VERSION;
		TRACE_2("ERR_VERSION: saImmOiCcbSetErrorString only supported for A.02.11 and above");
		goto bad_handle;
	}

	if(!imma_oi_ccb_record_set_error(cl_node, ccbId, errorString)) {
		TRACE_2("ERR_BAD_OPERATION: Ccb %llu, is not in a state that "
			"accepts an errorString", ccbId);
		rc = SA_AIS_ERR_BAD_OPERATION;
		goto bad_handle;
	}

 bad_handle:
	if (locked)
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);

 lock_fail:
	return rc;

}

extern SaAisErrorT immsv_om_augment_ccb_initialize(
				 SaImmOiHandleT privateOmHandle,
				 SaUint32T ccbId,
				 SaUint32T adminOwnerId,
				 SaImmCcbHandleT *ccbHandle,
				 SaImmAdminOwnerHandleT *ownerHandle) __attribute__((weak));


extern SaAisErrorT immsv_om_augment_ccb_get_admo_name(
				 SaImmHandleT privateOmHandle,
				 SaNameT* objectName,
				 SaNameT* admoNameOut) __attribute__((weak));


extern SaAisErrorT immsv_om_handle_initialize(SaImmHandleT *privateOmHandle, SaVersionT* version) __attribute__((weak));


extern SaAisErrorT immsv_om_admo_handle_initialize(
				  SaImmHandleT immHandle,
				  const SaImmAdminOwnerNameT adminOwnerName,
				  SaImmAdminOwnerHandleT *adminOwnerHandle) __attribute__((weak));

extern SaAisErrorT immsv_om_handle_finalize(
				 SaImmHandleT privateOmHandle) __attribute__((weak));

static SaAisErrorT
getAdmoName(SaImmHandleT privateOmHandle, IMMA_CALLBACK_INFO * cbi, SaNameT* admoNameOut)
{
    SaAisErrorT rc = SA_AIS_OK;
    const SaImmAttrNameT admoNameAttr = SA_IMM_ATTR_ADMIN_OWNER_NAME;
    SaImmAttrValuesT_2 **attributes = NULL;
    SaImmAttrValuesT_2 *attrVal = NULL;
    TRACE_ENTER();

    if(cbi->type == IMMA_CALLBACK_OI_CCB_CREATE) {
	    /* Admo attribute for created object should be in attr list.*/
	    int i=0;
	    attributes = (SaImmAttrValuesT_2 **) cbi->attrValsForCreateUc;
	    while((attrVal = attributes[i++]) != NULL) {
		    if(strcmp(admoNameAttr, attrVal->attrName)==0) {
			    TRACE("Found %s attribute in object create upcall", admoNameAttr);
			    break;
		    }
	    }

	    if(!attrVal || strcmp(attrVal->attrName, admoNameAttr) || (attrVal->attrValuesNumber!=1) ||
		    (attrVal->attrValueType != SA_IMM_ATTR_SASTRINGT)) {
		    LOG_ER("Missmatch on attribute %s", admoNameAttr);
		    abort();
	    }

	    strncpy((char *)admoNameOut->value, *(SaStringT*) attrVal->attrValues[0], SA_MAX_NAME_LENGTH);

    } else {
	    /* modify or delete => fetch admo attribute for object from server. */
	    if((cbi->type != IMMA_CALLBACK_OI_CCB_DELETE) &&
		    (cbi->type != IMMA_CALLBACK_OI_CCB_MODIFY)) {
		    LOG_ER("Inconsistency in callback type:%u", cbi->type);
		    abort();
	    }

	    osafassert(immsv_om_augment_ccb_get_admo_name);
	    rc = immsv_om_augment_ccb_get_admo_name(privateOmHandle, &(cbi->name), admoNameOut);
	    if(rc == SA_AIS_ERR_LIBRARY) {
		    LOG_ER("Missmatch on attribute %s for delete or modify", admoNameAttr);
    	    }
    }

    /* attrVal found either in create callback, or fetched from server. */

    if(rc == SA_AIS_OK) {
	    TRACE("Obtained AdmoName:%s",admoNameOut->value);
    }
    TRACE_LEAVE();
    return rc;
}

/****************************************************************************
  Name          :  saImmOiAugmentCcbInitialize
 
  Description   :  Allows the OI implementers to augment ops to a ccb related
                   to a current ccb-upcall, before returning from the upcall.
                   This is allowed inside:
                     SaImmOiCcbObjectCreateCallbackT
                     SaImmOiCcbObjectDeleteCallbackT
                     SaImmOiCcbObjectModifyCallbackT
                  It is NOT allowed inside:
                    SaImmOiCcbCompletedCallbackT
                    SaImmOiApplyCallbackT
                    SaImmOiAbortCallbackT

                   
  Arguments     :  immOiHandle - IMM OI handle. Received in callback.
                   ccbId  -  The ccbId for the parent ccb. Received in callback.
                   ccbHandle - The ccbHandle that will be initialized to the same 
                               ccbId as the parent CCB. 
                   ownerHandle - The admin owner handle that will be initialized to
                                 the same admin-owner as used by the parent CCB. 
 
  Return Values :  SA_AIS_OK
                   SA_AIS_ERR_BAD_HANDLE
                   SA_AIS_ERR_INVALID_PARAM
                   SA_AIS_ERR_BAD_OPERATION (not inside valid ccb upcall)
                   SA_AIS_ERR_LIBRARY
                   SA_AIS_ERR_TRY_AGAIN
                   
******************************************************************************/
SaAisErrorT saImmOiAugmentCcbInitialize(
				 SaImmOiHandleT immOiHandle,
				 SaImmOiCcbIdT ccbId64,
				 SaImmCcbHandleT *ccbHandle,
				 SaImmAdminOwnerHandleT *ownerHandle)

{
	SaAisErrorT rc = SA_AIS_OK;
	IMMSV_EVT init_evt;
	IMMSV_EVT *out_evt = NULL;
	IMMA_CB *cb = &imma_cb;
	IMMA_CLIENT_NODE *cl_node = 0;
	bool locked = true;
	IMMA_CALLBACK_INFO * cbi=NULL;
	SaImmHandleT privateOmHandle = 0LL;
	SaImmAdminOwnerHandleT privateAoHandle = 0LL;
	SaVersionT version = {'A', 2, 11};
	SaUint32T adminOwnerId = 0;
	SaUint32T ccbId = 0;

	TRACE_ENTER();

	if (cb->sv_id == 0) {
		TRACE_2("ERR_BAD_HANDLE: No initialized handle exists!");
		return SA_AIS_ERR_BAD_HANDLE;
	}

	if (!ccbHandle) {
		TRACE_2("ERR_INVALID_PARAM: ccbHandle is NULL");
		return SA_AIS_ERR_INVALID_PARAM;
	}

	if (!ownerHandle) {
		TRACE_2("ERR_INVALID_PARAM: ownerHandle is NULL");
		return SA_AIS_ERR_INVALID_PARAM;
	}

	ccbId = ccbId64;

	if(ccbId > 0xffffffff) {
		TRACE_2("Invalid ccbId, must be less than %llx", 
			0xffffffffLL);
		return SA_AIS_ERR_INVALID_PARAM;
	}

	if(ccbId == 0) {
		TRACE_2("Invalid ccbId, must no be zero");
		return SA_AIS_ERR_INVALID_PARAM;
	}

	*ccbHandle = 0;
	*ownerHandle = 0;

	if (cb->is_immnd_up == false) {
		TRACE_2("ERR_TRY_AGAIN: IMMND_DOWN");
		return SA_AIS_ERR_TRY_AGAIN;
	}

	/* Take the CB lock */
	if (m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
		TRACE_4("ERR_LIBRARY: LOCK failed");
		rc = SA_AIS_ERR_LIBRARY;
		goto lock_fail;
	}
	/* locked == true already */

	imma_client_node_get(&cb->client_tree, &immOiHandle, &cl_node);
	if (!cl_node || cl_node->isOm) {
		TRACE_2("ERR_BAD_HANDLE: Bad handle %llx", immOiHandle);
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto done;
	}

	if (cl_node->stale) {
		TRACE_1("ERR_BAD_HANDLE: Handle %llx is stale", immOiHandle);
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto done;
	}

	if(!(cl_node->isImmA2b)) {
		/* Log this error instead of trace, to simplify troubleshooting
		   for the OI maintainer. */
		LOG_IN("ERR_VERSION: saImmOiAugmentCcbInitialize " 
			"only allowed when OI handle is registered as A.2.11");
		rc = SA_AIS_ERR_VERSION;
		goto done;
	}

	cbi = imma_oi_ccb_record_ok_augment(cl_node, ccbId, &privateOmHandle, &privateAoHandle);
	if(!cbi) {
		TRACE_2("ERR_BAD_OPERATION: Ccb %u, is not in a state that "
			"can be augmented", ccbId);
		rc = SA_AIS_ERR_BAD_OPERATION;
		goto done;
	}

	if(!privateOmHandle) {
		/* The private om-handle is created at most once pe ccb that reaches the OI.
		   Likewise for the private admo. 
		 */
		osafassert(m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE) == NCSCC_RC_SUCCESS);
		locked = false;
		cl_node = NULL; /* avoid unsafe use */

		if(immsv_om_handle_initialize) {/*This is always the first immsv_om_ call */
			rc = immsv_om_handle_initialize(&privateOmHandle, &version);
		} else {
			TRACE("ERR_LIBRARY: Error in library linkage. libSaImmOm.so is not linked");
			rc = SA_AIS_ERR_LIBRARY;
		}

		if(rc != SA_AIS_OK) {
			TRACE("ERR_TRY_AGAIN: failed to obtain internal om handle rc:%u", rc);
			rc = SA_AIS_ERR_TRY_AGAIN;
			goto done;
		}

		osafassert(m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) == NCSCC_RC_SUCCESS);
		locked = true;

		imma_client_node_get(&cb->client_tree, &immOiHandle, &cl_node);
		if (!cl_node || cl_node->isOm || cl_node->stale) {
			TRACE_2("ERR_BAD_HANDLE: Bad handle %llx", immOiHandle);
			if (cl_node && cl_node->stale) {cl_node->exposed = true;}
			rc = SA_AIS_ERR_BAD_HANDLE;
			goto done;
		}
	}

	/* Downcall to immnd to get adminOwnerId and augmentation */
	imma_proc_increment_pending_reply(cl_node);

	/* populate the EVT structure */
	memset(&init_evt, 0, sizeof(IMMSV_EVT));
	init_evt.type = IMMSV_EVT_TYPE_IMMND;
	init_evt.info.immnd.type = IMMND_EVT_A2ND_OI_CCB_AUG_INIT;
	init_evt.info.immnd.info.ccbUpcallRsp.ccbId = ccbId;
	init_evt.info.immnd.info.ccbUpcallRsp.implId = cbi->implId;
	init_evt.info.immnd.info.ccbUpcallRsp.inv = cbi->inv;
	init_evt.info.immnd.info.ccbUpcallRsp.name = cbi->name;
	
	/* Note that we register using the new OM handle as client for the aug-ccb */
	rc = imma_evt_fake_evs(cb, &init_evt, &out_evt, cl_node->syncr_timeout,
		/*cl_node->handle*/privateOmHandle, &locked, true);

	cl_node = NULL; /* avoid unsafe use */

	if (rc != SA_AIS_OK) {
		/* fake_evs returned error */
		goto done;
	}

	if (!locked && m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != 
		NCSCC_RC_SUCCESS) {
		rc = SA_AIS_ERR_LIBRARY; /* Overwrites any error from fake_evs() */
		/* Losing track of the pending reply count, but ERR_LIBRARY dominates*/
		TRACE_4("ERR_LIBRARY: LOCK failed");
		goto done;
	}
	locked = true;

	/* get the client_info */
	imma_client_node_get(&cb->client_tree, &immOiHandle, &cl_node);
	if (!cl_node || cl_node->isOm) {
		rc = SA_AIS_ERR_BAD_HANDLE; 
		TRACE_3("client_node_get failed");
		goto done;;
	}

	imma_proc_decrement_pending_reply(cl_node); 

	if (cl_node->stale) {
		cl_node->exposed = true;  /* But dont resurrect it either */
		rc = SA_AIS_ERR_BAD_HANDLE;
		TRACE_3("Handle %llx is stale", immOiHandle);
		goto done;
	}

	m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
	locked = false;

	/* Call processed successfully, check result from immnd */
	osafassert(out_evt);
	/* Process the received Event */
	osafassert(out_evt->type == IMMSV_EVT_TYPE_IMMA);
	osafassert(out_evt->info.imma.type == IMMA_EVT_ND2A_CCB_AUG_INIT_RSP);
	adminOwnerId = out_evt->info.imma.info.admInitRsp.ownerId;
	rc = out_evt->info.imma.info.admInitRsp.error;

	if(rc == SA_AIS_ERR_NO_SECTIONS) {
		/* Means almost ok, admo ROF == FALSE. Can not hijack original admo.
		   Must instead create new separate admo with same admo-name. 
		 */
		rc = SA_AIS_OK;

		if(!privateAoHandle) {
			TRACE("AugCcbinit: Admo has ReleaseOnFinalize FALSE "
				"=> init separate admo => must fetch admo-name first");
			osafassert(locked == false);
			SaNameT admName;/* Used to get admo string name copied to stack.*/

			rc = getAdmoName(privateOmHandle, cbi, &admName);
			if(rc != SA_AIS_OK) {
				TRACE("ERR_TRY_AGAIN: failed to obtain SaImmAttrAdminOwnerName %u", rc);
				if(rc != SA_AIS_ERR_TRY_AGAIN) {
					rc = SA_AIS_ERR_TRY_AGAIN;
				}
				goto done;
			}
			TRACE("Obtaned AdminOwnerName:%s", admName.value);
			/* Allocate private admowner with ReleaseOnFinalize as TRUE */
			osafassert(immsv_om_admo_handle_initialize);
			rc = immsv_om_admo_handle_initialize(privateOmHandle,
				(SaImmAdminOwnerNameT) admName.value, &privateAoHandle);

			if(rc != SA_AIS_OK) {
				TRACE("ERR_TRY_AGAIN: failed to obtain internal admo handle rc:%u", rc);
				if(rc != SA_AIS_ERR_TRY_AGAIN) {
					rc = SA_AIS_ERR_TRY_AGAIN;
				}
				goto done;
			}
		}
	} else {TRACE("AugCcbinit: Admo has ROF == TRUE");}

	if (rc != SA_AIS_OK) {
		goto done;
	}

	TRACE("adminOwnerId:%u", adminOwnerId);

	/* Now dip into the OM library & create mockup ccb-node & admo-node for use by OI */
	osafassert(immsv_om_augment_ccb_initialize);
	rc = immsv_om_augment_ccb_initialize(privateOmHandle, ccbId, adminOwnerId,
		ccbHandle, &privateAoHandle);
 done:

	if (locked) {
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
	}

	if(rc == SA_AIS_OK || rc == SA_AIS_ERR_TRY_AGAIN) {
		/* mark oi_ccb_record with privateOmHandle to avoid repeated open/close
		   of private-om-handle for each try again or each ccb op. The handle
		   is closed when the ccb is terminated (apply-uc or abort-uc).
		 */
		imma_oi_ccb_record_augment(cl_node, ccbId, privateOmHandle, privateAoHandle);
		if(privateAoHandle) {*ownerHandle = privateAoHandle;}

		TRACE("ownerHandle:%llx", *ownerHandle);

	} else if(privateOmHandle) {
		osafassert(immsv_om_handle_finalize);
		immsv_om_handle_finalize(privateOmHandle);/* Also finalizes admo handles & ccb handles*/
	}

	if(out_evt) {
		free(out_evt);
		out_evt = NULL;
	}

 lock_fail:
	TRACE_LEAVE();
	return rc;
}
