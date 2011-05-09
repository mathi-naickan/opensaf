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

static int imma_oi_resurrect(IMMA_CB *cb, IMMA_CLIENT_NODE *cl_node, NCS_BOOL *locked);

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
#ifdef IMM_A_01_01
SaAisErrorT saImmOiInitialize(SaImmOiHandleT *immOiHandle,
			      const SaImmOiCallbacksT * immOiCallbacks, SaVersionT *version)
{
	if ((version->releaseCode != 'A') || (version->majorVersion != 0x01)) {
		TRACE_2("ERR_VERSION: THIS SHOULD BE A VERSION A.1.x implementer but claims to be"
		      "%c %u %u", version->releaseCode, version->majorVersion, 
			version->minorVersion);
		return SA_AIS_ERR_VERSION;
	}

	return saImmOiInitialize_2(immOiHandle, (const SaImmOiCallbacksT_2 *)immOiCallbacks, version);
}
#endif

SaAisErrorT saImmOiInitialize_2(SaImmOiHandleT *immOiHandle,
				const SaImmOiCallbacksT_2 *immOiCallbacks, SaVersionT *version)
{
	IMMA_CB *cb = &imma_cb;
	SaAisErrorT rc = SA_AIS_OK;
	IMMSV_EVT init_evt;
	IMMSV_EVT *out_evt = NULL;
	uns32 proc_rc = NCSCC_RC_SUCCESS;
	IMMA_CLIENT_NODE *cl_node = 0;
	NCS_BOOL locked = TRUE;

	TRACE_ENTER();

	proc_rc = imma_startup(NCSMDS_SVC_ID_IMMA_OI);
	if (NCSCC_RC_SUCCESS != proc_rc) {
		TRACE_4("ERR_LIBRARY: Agents_startup failed");
		TRACE_LEAVE();
		return SA_AIS_ERR_LIBRARY;
	}

	if ((!immOiHandle) || (!version)) {
		TRACE_2("ERR_INVALID_PARAM: immOiHandle is NULL or version is NULL");
		rc = SA_AIS_ERR_INVALID_PARAM;
		goto end;
	}

	if (FALSE == cb->is_immnd_up) {
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

	cl_node->isOm = FALSE;
#ifdef IMM_A_01_01
	if ((version->releaseCode == 'A') && (version->majorVersion == 0x01)) {
		TRACE_1("THIS IS A VERSION A.1.x implementer %c %u %u",
		      version->releaseCode, version->majorVersion, version->minorVersion);
		cl_node->isOiA1 = TRUE;
	} else {
		TRACE_1("THIS IS A VERSION A.2.x implementer %c %u %u",
		      version->releaseCode, version->majorVersion, version->minorVersion);
		cl_node->isOiA1 = FALSE;
	}
#endif

	/* Take the CB Lock */
	if (m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
		TRACE_4("ERR_LIBRARY: LOCK failed");
		rc = SA_AIS_ERR_LIBRARY;
		goto lock_fail;
	}

	/* Draft Validations : Version this is the politically correct validatioin
	   distinct from the pragmatic validation we do above. */
	rc = imma_version_validate(version);
	if (rc != SA_AIS_OK) {
		TRACE_2("ERR_VERSION");
		goto version_fail;
	}

	/* Store the callback functions, if set */
	if (immOiCallbacks) {
#ifdef IMM_A_01_01
		if (cl_node->isOiA1) {
			cl_node->o.iCallbk1 = *((const SaImmOiCallbacksT *)immOiCallbacks);
			TRACE("Version A.1.x callback registered:");
			TRACE("RtAttrUpdateCb:%p", cl_node->o.iCallbk1.saImmOiRtAttrUpdateCallback);
			TRACE("CcbObjectCreateCb:%p", cl_node->o.iCallbk1.saImmOiCcbObjectCreateCallback);
			TRACE("CcbObjectDeleteCb:%p", cl_node->o.iCallbk1.saImmOiCcbObjectDeleteCallback);
			TRACE("CcbObjectModifyCb:%p", cl_node->o.iCallbk1.saImmOiCcbObjectModifyCallback);
			TRACE("CcbCompletedCb:%p", cl_node->o.iCallbk1.saImmOiCcbCompletedCallback);
			TRACE("CcbApplyCb:%p", cl_node->o.iCallbk1.saImmOiCcbApplyCallback);
			TRACE("CcbAbortCb:%p", cl_node->o.iCallbk1.saImmOiCcbAbortCallback);
			TRACE("AdminOperationCb:%p", cl_node->o.iCallbk1.saImmOiAdminOperationCallback);
		} else {
			cl_node->o.iCallbk = *immOiCallbacks;
		}
#else
		cl_node->o.iCallbk = *immOiCallbacks;
#endif
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
	init_evt.info.immnd.info.initReq.version = *version;
	init_evt.info.immnd.info.initReq.client_pid = cb->process_id;

	/* Release the CB lock Before MDS Send */
	m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
	locked = FALSE;

	if (FALSE == cb->is_immnd_up) {
		rc = SA_AIS_ERR_TRY_AGAIN;
		TRACE_2("ERR_TRY_AGAIN: IMMND is DOWN");
		goto mds_fail;
	}

	/* send the request to the IMMND */
	proc_rc = imma_mds_msg_sync_send(cb->imma_mds_hdl, &cb->immnd_mds_dest, &init_evt, &out_evt, IMMSV_WAIT_TIME);

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

		locked = TRUE;

		if (FALSE == cb->is_immnd_up) {
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
			locked = FALSE;
		}

		/* send the request to the IMMND */
		imma_mds_msg_sync_send(cb->imma_mds_hdl, &(cb->immnd_mds_dest),
		      &finalize_evt, &out_evt1, IMMSV_WAIT_TIME);
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
	NCS_BOOL locked = TRUE;
	TRACE_ENTER();

	if (!selectionObject)
		return SA_AIS_ERR_INVALID_PARAM;

	if (cb->sv_id == 0) {
		TRACE_2("ERR_BAD_HANDLE: No initialized handle exists!");
		return SA_AIS_ERR_BAD_HANDLE;
	}

	if (cb->is_immnd_up == FALSE) {
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
	/* locked == TRUE already */

	imma_client_node_get(&cb->client_tree, &immOiHandle, &cl_node);

	if (!cl_node || cl_node->isOm) {
		TRACE_2("ERR_BAD_HANDLE: Bad handle %llx", immOiHandle);
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto node_not_found;
	}

	if (cl_node->stale) {
		TRACE_1("Handle %llx is stale", immOiHandle);
		NCS_BOOL resurrected = imma_oi_resurrect(cb, cl_node, &locked);

		if (!locked && m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
			TRACE_4("ERR_LIBRARY: LOCK failed");
			rc = SA_AIS_ERR_LIBRARY;
			goto lock_fail;
		}
		locked = TRUE;

		imma_client_node_get(&cb->client_tree, &immOiHandle, &cl_node);

		if (!resurrected || !cl_node || cl_node->isOm || cl_node->stale) {
			TRACE_2("ERR_BAD_HANDLE: Reactive ressurect of handle %llx failed",
				immOiHandle);
			if (cl_node && cl_node->stale) {cl_node->exposed = TRUE;}
			rc = SA_AIS_ERR_BAD_HANDLE;
			goto resurrect_failed;
		}

		TRACE_1("Reactive resurrect of handle %llx succeeded", immOiHandle);
	}


	*selectionObject = (SaSelectionObjectT)
	    m_GET_FD_FROM_SEL_OBJ(m_NCS_IPC_GET_SEL_OBJ(&cl_node->callbk_mbx));

	cl_node->selObjUsable = TRUE;

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
	NCS_BOOL locked = FALSE;
	uns32 pend_fin = 0;
   	uns32 pend_dis = 0;
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
	locked = TRUE;

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
			cl_node->exposed = TRUE;
			goto fail;
		} 

		--(cb->dispatch_clients_to_resurrect);
		TRACE_1("Remaining clients to acively resurrect: %d",
			cb->dispatch_clients_to_resurrect);

		if (!imma_oi_resurrect(cb, cl_node, &locked)) {
            TRACE_2("ERR_BAD_HANDLE: Failed to resurrect stale OI handle <c:%u, n:%x>",
                m_IMMSV_UNPACK_HANDLE_HIGH(immOiHandle),
                m_IMMSV_UNPACK_HANDLE_LOW(immOiHandle));
            rc = SA_AIS_ERR_BAD_HANDLE;
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
		locked = TRUE;

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
			cl_node->exposed = TRUE;
			rc = SA_AIS_ERR_BAD_HANDLE;
			goto fail;
		}

		cl_node->selObjUsable = TRUE; /* success */
	}

    /* Back to normal case of non stale (possibly resurrected) handle. */
    /* Unlock & do the dispatch to avoid deadlock in arrival callback. */
	if (locked) {
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
		locked = FALSE;
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
	}			/* switch */

	/* Decrement Dispatch usage count */
 	cb->pend_dis--;

	/* can't use cb after we do agent shutdown, so copy all counts */
   	pend_dis = cb->pend_dis;
   	pend_fin = cb->pend_fin;

 fail:
	if (locked)
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);

	/* see if we are still in any dispact context */ 
    if(pend_dis == 0)
      while(pend_fin != 0)
      {
         /* call agent shutdown,for each finalize done before */
	 cb->pend_fin --;
         imma_shutdown(NCSMDS_SVC_ID_IMMA_OI);
         pend_fin--;
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
	uns32 proc_rc = NCSCC_RC_SUCCESS;
	NCS_BOOL locked = TRUE;
	NCS_BOOL agent_flag = FALSE; /* flag = FALSE, we should not call agent shutdown */
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
		cl_node->exposed = TRUE;
		goto stale_handle;
	}

	/* populate the structure */
	memset(&finalize_evt, 0, sizeof(IMMSV_EVT));
	finalize_evt.type = IMMSV_EVT_TYPE_IMMND;
	finalize_evt.info.immnd.type = IMMND_EVT_A2ND_IMM_OI_FINALIZE;
	finalize_evt.info.immnd.info.finReq.client_hdl = cl_node->handle;

	/* Unlock before MDS Send */
	m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
	locked = FALSE;
	cl_node = NULL; /* avoid unsafe use */

	if (cb->is_immnd_up == FALSE) {
		TRACE_3("IMMND is DOWN");
		/* IF IMMND IS DOWN then we know handle is stale. 
		   Since this is a handle finalize, we simply discard the handle.
		   No error return!
		 */
		goto stale_handle;
	}

	/* send the request to the IMMND */
	proc_rc = imma_mds_msg_sync_send(cb->imma_mds_hdl, &(cb->immnd_mds_dest),
					 &finalize_evt, &out_evt, IMMSV_WAIT_TIME);

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

		locked = TRUE;

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

			cl_node->exposed = TRUE; /* But dont resurrect it either. */
		}

		imma_proc_decrement_pending_reply(cl_node);
		imma_finalize_client(cb, cl_node);

		/* Fialize the environment */  
		if ( cb->pend_dis == 0)
		   agent_flag = TRUE;
		else if(cb->pend_dis > 0)
		   cb->pend_fin++;
	}

 lock_fail1:
 mds_send_fail:
 node_not_found:
	if (locked)
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);

 lock_fail:
	if (rc == SA_AIS_OK) {
		
    /* we are not in any dispatch context, we can do agent shutdown */
  	if(agent_flag == TRUE) 
		if (NCSCC_RC_SUCCESS != imma_shutdown(NCSMDS_SVC_ID_IMMA_OI)) {
            TRACE_4("ERR_LIBRARY: Call to imma_shutdown failed");
            rc = SA_AIS_ERR_LIBRARY;
        }
	}
	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
  Name          :  saImmOiAdminOperationResult
 
  Description   :  Send the result for an admin-op, supposedly invoked inside 
                   the upcall for an admin op. 
                   This is normally a NON blocking call (except when resurrecting client)
 
  Arguments     :  immOiHandle - IMM OI Service handle.
 
  Return Values :  Refer to SAI-AIS specification for various return values.
 
  Notes         :
******************************************************************************/
SaAisErrorT saImmOiAdminOperationResult(SaImmOiHandleT immOiHandle, SaInvocationT invocation, SaAisErrorT result)
{
	SaAisErrorT rc = SA_AIS_OK;
	IMMA_CB *cb = &imma_cb;
	IMMSV_EVT adminOpRslt_evt;
	IMMA_CLIENT_NODE *cl_node = 0;
	uns32 proc_rc = NCSCC_RC_SUCCESS;
	NCS_BOOL locked = TRUE;

	if (cb->sv_id == 0) {
		TRACE_2("ERR_BAD_HANDLE: No initialized handle exists!");
		return SA_AIS_ERR_BAD_HANDLE;
	}

	if (cb->is_immnd_up == FALSE) {
		TRACE_2("ERR_TRY_AGAIN: IMMND_DOWN");
		return SA_AIS_ERR_TRY_AGAIN;
	}

	if (m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
		TRACE_4("ERR_LIBRARY: LOCK failed");
		rc = SA_AIS_ERR_LIBRARY;
		goto lock_fail;
	}

	/* get the client_info */
	imma_client_node_get(&cb->client_tree, &immOiHandle, &cl_node);
	if (!cl_node || cl_node->isOm) {
		rc = SA_AIS_ERR_BAD_HANDLE;
		TRACE_2("ERR_BAD_HANDLE: client_node_get failed");
		goto node_not_found;
	}

	if (cl_node->stale) {
		TRACE_1("Handle %llx is stale", immOiHandle);
		NCS_BOOL resurrected = imma_oi_resurrect(cb, cl_node, &locked);

		if (!locked && m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
			TRACE_4("ERR_LIBRARY: LOCK failed");
			rc = SA_AIS_ERR_LIBRARY;
			goto lock_fail;
		}
		locked = TRUE;

		imma_client_node_get(&cb->client_tree, &immOiHandle, &cl_node);

		if (!resurrected || !cl_node || cl_node->isOm || cl_node->stale) {
			TRACE_2("ERR_BAD_HANDLE: Reactive ressurect of handle %llx failed",
				immOiHandle);
			if (cl_node && cl_node->stale) {cl_node->exposed = TRUE;}
			rc = SA_AIS_ERR_BAD_HANDLE;
			goto stale_handle;
		}

		TRACE_1("Reactive ressurect of handle %llx succeeded", immOiHandle);
	}

	/* Note NOT unsigned since negative means async invoc. */
	SaInt32T inv = m_IMMSV_UNPACK_HANDLE_LOW(invocation);
	SaInt32T owner = m_IMMSV_UNPACK_HANDLE_HIGH(invocation);

	/* populate the structure */
	memset(&adminOpRslt_evt, 0, sizeof(IMMSV_EVT));
	adminOpRslt_evt.type = IMMSV_EVT_TYPE_IMMND;
	/*Need to encode async/sync variant. */
	if (inv < 0) {
		adminOpRslt_evt.info.immnd.type = IMMND_EVT_A2ND_ASYNC_ADMOP_RSP;
	} else {
		adminOpRslt_evt.info.immnd.type = IMMND_EVT_A2ND_ADMOP_RSP;
		if(owner) {
			adminOpRslt_evt.info.immnd.type = IMMND_EVT_A2ND_ADMOP_RSP;
		} else {
			TRACE_1("PBE_ADMOP_RSP");
			assert(cl_node->isPbe);
			adminOpRslt_evt.info.immnd.type = IMMND_EVT_A2ND_PBE_ADMOP_RSP;
		}
	}
	adminOpRslt_evt.info.immnd.info.admOpRsp.oi_client_hdl = immOiHandle;
	adminOpRslt_evt.info.immnd.info.admOpRsp.invocation = invocation;
	adminOpRslt_evt.info.immnd.info.admOpRsp.result = result;
	adminOpRslt_evt.info.immnd.info.admOpRsp.error = SA_AIS_OK;

	if (cb->is_immnd_up == FALSE) {
		rc = SA_AIS_ERR_TRY_AGAIN;
		/* IMMND must have gone down after we locked above */
		TRACE_2("ERR_TRY_AGAIN: IMMND_DOWN");
		goto mds_send_fail;
	}

	/* send the reply to the IMMND ASYNCronously */
	if(adminOpRslt_evt.info.immnd.type == IMMND_EVT_A2ND_PBE_ADMOP_RSP) {
		proc_rc = imma_evt_fake_evs(cb, &adminOpRslt_evt, NULL, 0, cl_node->handle, &locked, FALSE);
	} else {
		/* Unlock before MDS Send */
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
		locked = FALSE;
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
		TRACE_4("ERR_LIBRARY: MDS returned unexpected error code %u", proc_rc);
		rc = SA_AIS_ERR_LIBRARY;
		goto mds_send_fail;
	}

 mds_send_fail:
 stale_handle:
 node_not_found:
	if (locked)
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);

 lock_fail:
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
	SaBoolT locked = SA_TRUE;
	SaUint32T nameLen = 0;

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

	if (cb->is_immnd_up == FALSE) {
		TRACE_2("ERR_TRY_AGAIN: IMMND is DOWN");
		return SA_AIS_ERR_TRY_AGAIN;
	}

	/* get the CB Lock */
	if (m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
		rc = SA_AIS_ERR_LIBRARY;
		TRACE_4("ERR_LIBRARY: Lock failed");
		goto lock_fail;
	}

	/*locked == TRUE already */

	imma_client_node_get(&cb->client_tree, &immOiHandle, &cl_node);
	if (!cl_node || cl_node->isOm) {
		rc = SA_AIS_ERR_BAD_HANDLE;
		TRACE_2("ERR_BAD_HANDLE: client_node_get failed");
		goto bad_handle;
	}

	if (cl_node->mImplementerId) {
		rc = SA_AIS_ERR_EXIST;
		/* Not BAD_HANDLE. Clarified in SAI-AIS-IMM-A.03.01 */
		TRACE_2("ERR_EXIST: Implementer already set for this handle");
		goto bad_handle;
	}

	if(implementerName[0] == '@') {
		rc = SA_AIS_ERR_INVALID_PARAM;
		TRACE_2("ERR_INVALID_PARAM: Applier OIs (leading '@') only supported for A.02.11 and above");
		goto bad_handle;
	}

	if (cl_node->stale) {
		TRACE_1("Handle %llx is stale", immOiHandle);
		NCS_BOOL resurrected = imma_oi_resurrect(cb, cl_node, &locked);

		if (!locked && m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
			TRACE_4("ERR_LIBRARY: LOCK failed");
			rc = SA_AIS_ERR_LIBRARY;
			goto lock_fail;
		}
		locked = TRUE;

		imma_client_node_get(&cb->client_tree, &immOiHandle, &cl_node);

		if (!resurrected || !cl_node || cl_node->isOm || cl_node->stale) {
			TRACE_2("ERR_BAD_HANDLE: Reactive ressurect of handle %llx failed", 
				immOiHandle);
			if (cl_node && cl_node->stale) {cl_node->exposed = TRUE;}
			rc = SA_AIS_ERR_BAD_HANDLE;
			goto bad_handle;
		}

		TRACE_1("Reactive resurrect of handle %llx succeeded", immOiHandle);
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
	locked = FALSE;
	cl_node = NULL; /* avoid unsafe use */
	
	if (cb->is_immnd_up == FALSE) {
		rc = SA_AIS_ERR_TRY_AGAIN;
		TRACE_2("ERR_TRY_AGAIN: IMMND is DOWN");
		goto mds_send_fail;
	}

	/* Send the evt to IMMND */
	proc_rc = imma_mds_msg_sync_send(cb->imma_mds_hdl, &(cb->immnd_mds_dest), &evt, &out_evt, IMMSV_WAIT_TIME);

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
	locked = TRUE;

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
		cl_node->exposed = TRUE;
		goto bad_handle;
	}

	if (out_evt) {
		/* Process the received Event */
		assert(out_evt->type == IMMSV_EVT_TYPE_IMMA);
		assert(out_evt->info.imma.type == IMMA_EVT_ND2A_IMPLSET_RSP);
		if (rc == SA_AIS_OK) {
			rc = out_evt->info.imma.info.implSetRsp.error;
			if (rc == SA_AIS_OK) {
				cl_node->mImplementerId = out_evt->info.imma.info.implSetRsp.implId;
				cl_node->mImplementerName = calloc(1, nameLen);
				strncpy(cl_node->mImplementerName, implementerName, nameLen);
				if(strncmp(implementerName, OPENSAF_IMM_PBE_IMPL_NAME, nameLen) == 0) {
					TRACE("Special implementer %s detected and noted.", OPENSAF_IMM_PBE_IMPL_NAME);
					cl_node->isPbe = 0x1;
				}
			}
		}
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
	SaBoolT locked = SA_TRUE;

	if (cb->sv_id == 0) {
		TRACE_2("ERR_BAD_HANDLE: No initialized handle exists!");
		return SA_AIS_ERR_BAD_HANDLE;
	}

	if (cb->is_immnd_up == FALSE) {
		TRACE_2("ERR_TRY_AGAIN: IMMND is DOWN");
		return SA_AIS_ERR_TRY_AGAIN;
	}

	/* get the CB Lock */
	if (m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
		rc = SA_AIS_ERR_LIBRARY;
		TRACE_4("ERR_LIBRARY: Lock failed");
		goto lock_fail;
	}

	/*locked == TRUE already */

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

		NCS_BOOL resurrected = imma_oi_resurrect(cb, cl_node, &locked);

		if (!locked && m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
			TRACE_4("ERR_LIBRARY: LOCK failed");
			rc = SA_AIS_ERR_LIBRARY;
			goto lock_fail;
		}
		locked = TRUE;

		imma_client_node_get(&cb->client_tree, &immOiHandle, &cl_node);

		if (!resurrected || !cl_node || cl_node->isOm || cl_node->stale) {
			TRACE_2("ERR_BAD_HANDLE: Reactive ressurect of handle %llx failed", 
				immOiHandle);
			if (cl_node && cl_node->stale) {cl_node->exposed = TRUE;}
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

	rc = imma_evt_fake_evs(cb, &evt, &out_evt, IMMSV_WAIT_TIME, cl_node->handle, &locked, TRUE);

	cl_node=NULL;
	/* Take the CB lock  */
	if (!locked && m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != 
		NCSCC_RC_SUCCESS) {
		rc = SA_AIS_ERR_LIBRARY;
		/* Losing track of the pending reply count, but ERR_LIBRARY dominates*/
		TRACE_4("ERR_LIBRARY: LOCK failed");
		goto lock_fail;
	}
	locked = TRUE;

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
		  cl_node->exposed = TRUE;
		  goto bad_handle;
		*/
	}

	assert(out_evt);
	/* Process the received Event */
	assert(out_evt->type == IMMSV_EVT_TYPE_IMMA);
	assert(out_evt->info.imma.type == IMMA_EVT_ND2A_IMM_ERROR);

	rc = out_evt->info.imma.info.errRsp.error;

	if (rc == SA_AIS_OK) {
		cl_node->mImplementerId = 0;
		free(cl_node->mImplementerName);
		cl_node->mImplementerName = NULL;
	}

 skip_impl_clear:
 bad_handle:
	if (locked) 
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);

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
	SaBoolT locked = SA_TRUE;
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

	if (cb->is_immnd_up == FALSE) {
		TRACE_2("ERR_TRY_AGAIN: IMMND is DOWN");
		return SA_AIS_ERR_TRY_AGAIN;
	}

	/* get the CB Lock */
	if (m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
		rc = SA_AIS_ERR_LIBRARY;
		TRACE_4("ERR_LIBRARY: Lock failed");
		goto lock_fail;
	}

	/*locked == TRUE already */

	imma_client_node_get(&cb->client_tree, &immOiHandle, &cl_node);
	if (!cl_node || cl_node->isOm) {
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto bad_handle;
	}

	if (cl_node->stale) {
		TRACE_1("Handle %llx is stale", immOiHandle);
		NCS_BOOL resurrected = imma_oi_resurrect(cb, cl_node, &locked);

		if (!locked && m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
			TRACE_4("ERR_LIBRARY: LOCK failed");
			rc = SA_AIS_ERR_LIBRARY;
			goto lock_fail;
		}
		locked = TRUE;

		imma_client_node_get(&cb->client_tree, &immOiHandle, &cl_node);

		if (!resurrected || !cl_node || cl_node->isOm || cl_node->stale) {
			TRACE_2("ERR_BAD_HANDLE: Reactive ressurect of handle %llx failed", 
				immOiHandle);
			if (cl_node && cl_node->stale) {cl_node->exposed = TRUE;}
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
	evt.info.immnd.type = IMMND_EVT_A2ND_OI_CL_IMPL_SET;
	evt.info.immnd.info.implSet.client_hdl = cl_node->handle;
	evt.info.immnd.info.implSet.impl_name.size = nameLen;
	evt.info.immnd.info.implSet.impl_name.buf = className;
	evt.info.immnd.info.implSet.impl_id = cl_node->mImplementerId;

	imma_proc_increment_pending_reply(cl_node);

	rc = imma_evt_fake_evs(cb, &evt, &out_evt, IMMSV_WAIT_TIME, cl_node->handle, &locked, TRUE);

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
	locked = TRUE;

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
		cl_node->exposed = TRUE;
		goto bad_handle;
	}

	assert(out_evt);
	/* Process the received Event */
	assert(out_evt->type == IMMSV_EVT_TYPE_IMMA);
	assert(out_evt->info.imma.type == IMMA_EVT_ND2A_IMM_ERROR);
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
	SaBoolT locked = SA_TRUE;
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

	if (cb->is_immnd_up == FALSE) {
		TRACE_2("ERR_TRY_AGAIN: IMMND is DOWN");
		return SA_AIS_ERR_TRY_AGAIN;
	}

	/* get the CB Lock */
	if (m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
		rc = SA_AIS_ERR_LIBRARY;
		TRACE_4("ERR_LIBRARY: Lock failed");
		goto lock_fail;
	}

	/*locked == TRUE already */

	imma_client_node_get(&cb->client_tree, &immOiHandle, &cl_node);
	if (!cl_node || cl_node->isOm) {
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto bad_handle;
	}

	if (cl_node->stale) {
		TRACE_1("Handle %llx is stale", immOiHandle);
		NCS_BOOL resurrected = imma_oi_resurrect(cb, cl_node, &locked);

		if (!locked && m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
			TRACE_4("ERR_LIBRARY: LOCK failed");
			rc = SA_AIS_ERR_LIBRARY;
			goto lock_fail;
		}
		locked = TRUE;

		imma_client_node_get(&cb->client_tree, &immOiHandle, &cl_node);

		if (!resurrected || !cl_node || cl_node->isOm || cl_node->stale) {
			TRACE_2("ERR_BAD_HANDLE: Reactive ressurect of handle %llx failed",
				immOiHandle);
			if (cl_node && cl_node->stale) {cl_node->exposed = TRUE;}
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

	rc = imma_evt_fake_evs(cb, &evt, &out_evt, IMMSV_WAIT_TIME, cl_node->handle, &locked, TRUE);

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
	locked = TRUE;

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
		  cl_node->exposed = TRUE;
		  goto bad_handle;
		*/
	}

	assert(out_evt);
	/* Process the received Event */
	assert(out_evt->type == IMMSV_EVT_TYPE_IMMA);
	assert(out_evt->info.imma.type == IMMA_EVT_ND2A_IMM_ERROR);
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
	SaBoolT locked = SA_TRUE;
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
	nameLen = objectName->length + 1;

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

	if (cb->is_immnd_up == FALSE) {
		TRACE_2("ERR_TRY_AGAIN: IMMND is DOWN");
		return SA_AIS_ERR_TRY_AGAIN;
	}

	/* get the CB Lock */
	if (m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
		rc = SA_AIS_ERR_LIBRARY;
		TRACE_4("ERR_LIBRARY: Lock failed");
		goto lock_fail;
	}

	/*locked == TRUE already */

	imma_client_node_get(&cb->client_tree, &immOiHandle, &cl_node);
	if (!cl_node || cl_node->isOm) {
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto bad_handle;
	}

	if (cl_node->stale) {
		TRACE_1("Handle %llx is stale", immOiHandle);
		NCS_BOOL resurrected = imma_oi_resurrect(cb, cl_node, &locked);

		if (!locked && m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
			TRACE_4("ERR_LIBRARY: LOCK failed");
			rc = SA_AIS_ERR_LIBRARY;
			goto lock_fail;
		}
		locked = TRUE;

		imma_client_node_get(&cb->client_tree, &immOiHandle, &cl_node);

		if (!resurrected || !cl_node || cl_node->isOm || cl_node->stale) {
			TRACE_2("ERR_BAD_HANDLE: Reactive ressurect of handle %llx failed",
				immOiHandle);
			if (cl_node && cl_node->stale) {cl_node->exposed = TRUE;}
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

	rc = imma_evt_fake_evs(cb, &evt, &out_evt, IMMSV_WAIT_TIME, cl_node->handle, &locked, TRUE);

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
	locked = TRUE;

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
		cl_node->exposed = TRUE;
		goto bad_handle;
	}

	assert(out_evt);
	assert(out_evt->type == IMMSV_EVT_TYPE_IMMA);
	assert(out_evt->info.imma.type == IMMA_EVT_ND2A_IMM_ERROR);
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
	SaBoolT locked = SA_TRUE;
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

	if (cb->is_immnd_up == FALSE) {
		TRACE_2("ERR_TRY_AGAIN: IMMND is DOWN");
		return SA_AIS_ERR_TRY_AGAIN;
	}

	/* get the CB Lock */
	if (m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
		rc = SA_AIS_ERR_LIBRARY;
		TRACE_4("ERR_LIBRARY: Lock failed");
		goto lock_fail;
	}

	/*locked == TRUE already */

	imma_client_node_get(&cb->client_tree, &immOiHandle, &cl_node);
	if (!cl_node || cl_node->isOm) {
		TRACE_2("ERR_BAD_HANDLE: Not a valid SaImmOiHandleT");
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto bad_handle;
	}

	if (cl_node->stale) {
		TRACE_1("Handle %llx is stale", immOiHandle);
		NCS_BOOL resurrected = imma_oi_resurrect(cb, cl_node, &locked);

		if (!locked && m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
			TRACE_4("ERR_LIBRARY: LOCK failed");
			rc = SA_AIS_ERR_LIBRARY;
			goto lock_fail;
		}
		locked = TRUE;

		imma_client_node_get(&cb->client_tree, &immOiHandle, &cl_node);

		if (!resurrected || !cl_node || cl_node->isOm || cl_node->stale) {
			TRACE_2("ERR_BAD_HANDLE: Reactive ressurect of handle %llx failed",
				immOiHandle);
			if (cl_node && cl_node->stale) {cl_node->exposed = TRUE;}
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

	rc = imma_evt_fake_evs(cb, &evt, &out_evt, IMMSV_WAIT_TIME, cl_node->handle, &locked, TRUE);

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
	locked = TRUE;

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
		  cl_node->exposed = TRUE;
		  goto bad_handle;
		*/
	}

	assert(out_evt);
	assert(out_evt->type == IMMSV_EVT_TYPE_IMMA);
	assert(out_evt->info.imma.type == IMMA_EVT_ND2A_IMM_ERROR);
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

#ifdef IMM_A_01_01
extern SaAisErrorT saImmOiRtObjectUpdate(SaImmOiHandleT immOiHandle,
					 const SaNameT *objectName, const SaImmAttrModificationT ** attrMods)
{
	return saImmOiRtObjectUpdate_2(immOiHandle, objectName, (const SaImmAttrModificationT_2 **)
				       attrMods);
}
#endif

SaAisErrorT saImmOiRtObjectUpdate_2(SaImmOiHandleT immOiHandle,
				    const SaNameT *objectName, const SaImmAttrModificationT_2 **attrMods)
{
	SaAisErrorT rc = SA_AIS_OK;
	uns32 proc_rc = NCSCC_RC_SUCCESS;
	IMMA_CB *cb = &imma_cb;
	IMMSV_EVT evt;
	IMMSV_EVT *out_evt = NULL;
	IMMA_CLIENT_NODE *cl_node = NULL;
	NCS_BOOL locked = TRUE;

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

	if (FALSE == cb->is_immnd_up) {
		TRACE_2("ERR_TRY_AGAIN: IMMND is DOWN");
		return SA_AIS_ERR_TRY_AGAIN;
	}

	/* get the CB Lock */
	if (m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
		rc = SA_AIS_ERR_LIBRARY;
		TRACE_4("ERR_LIBRARY: Lock failed");
		goto lock_fail;
	}
	/*locked ==TRUE already */

	imma_client_node_get(&cb->client_tree, &immOiHandle, &cl_node);
	if (!cl_node || cl_node->isOm) {
		rc = SA_AIS_ERR_BAD_HANDLE;
		TRACE_2("ERR_BAD_HANDLE: Non valid SaImmOiHandleT");
		goto bad_handle;
	}

	if (cl_node->stale) {
		TRACE_1("Handle %llx is stale", immOiHandle);
		NCS_BOOL resurrected = imma_oi_resurrect(cb, cl_node, &locked);

		if (!locked && m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
			TRACE_4("ERR_LIBRARY: LOCK failed");
			rc = SA_AIS_ERR_LIBRARY;
			goto lock_fail;
		}
		locked = TRUE;

		imma_client_node_get(&cb->client_tree, &immOiHandle, &cl_node);

		if (!resurrected || !cl_node || cl_node->isOm || cl_node->stale) {
			TRACE_2("ERR_BAD_HANDLE: Reactive ressurect of handle %llx failed",
				immOiHandle);
			if (cl_node && cl_node->stale) {cl_node->exposed = TRUE;}
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

	assert(evt.info.immnd.info.objModify.attrMods == NULL);

	const SaImmAttrModificationT_2 *attrMod;
	int i;
	for (i = 0; attrMods[i]; ++i) {
		attrMod = attrMods[i];

		/* TODO Check that the user does not set values for System attributes. */

		/*alloc-2 */
		IMMSV_ATTR_MODS_LIST *p = calloc(1, sizeof(IMMSV_ATTR_MODS_LIST));
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
	locked = FALSE;
	cl_node =NULL;

	if (FALSE == cb->is_immnd_up) {
		rc = SA_AIS_ERR_TRY_AGAIN;
		TRACE_2("ERR_TRY_AGAIN: IMMND is DOWN");
		goto mds_send_fail;
	}

	/* send the request to the IMMND */
	proc_rc = imma_mds_msg_sync_send(cb->imma_mds_hdl, &cb->immnd_mds_dest, &evt, &out_evt, IMMSV_WAIT_TIME);

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
		goto lock_fail;
	}
	locked = TRUE;

	imma_client_node_get(&cb->client_tree, &immOiHandle, &cl_node);
	if (!cl_node || cl_node->isOm) {
		rc = SA_AIS_ERR_BAD_HANDLE;
		TRACE_2("ERR_BAD_HANDLE: client_node_get failed");
		goto bad_handle;
	}

	imma_proc_decrement_pending_reply(cl_node);

	if (cl_node->stale) {
		TRACE_1("Handle %llx is stale", immOiHandle);
		rc = SA_AIS_ERR_BAD_HANDLE;
		cl_node->exposed =TRUE;
		goto bad_handle;
	}

	if (out_evt) {
		/* Process the outcome, note this is after a blocking call. */
		assert(out_evt->type == IMMSV_EVT_TYPE_IMMA);
		assert(out_evt->info.imma.type == IMMA_EVT_ND2A_IMM_ERROR);
		if (rc == SA_AIS_OK) {
			rc = out_evt->info.imma.info.errRsp.error;
		}
	}

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

#ifdef IMM_A_01_01
extern SaAisErrorT saImmOiRtObjectCreate(SaImmOiHandleT immOiHandle,
					 const SaImmClassNameT className,
					 const SaNameT *parentName, const SaImmAttrValuesT ** attrValues)
{
	return saImmOiRtObjectCreate_2(immOiHandle, className, parentName, (const SaImmAttrValuesT_2 **)attrValues);
}
#endif

extern SaAisErrorT saImmOiRtObjectCreate_2(SaImmOiHandleT immOiHandle,
					   const SaImmClassNameT className,
					   const SaNameT *parentName, const SaImmAttrValuesT_2 **attrValues)
{
	SaAisErrorT rc = SA_AIS_OK;
	IMMA_CB *cb = &imma_cb;
	IMMSV_EVT evt;
	IMMSV_EVT *out_evt = NULL;
	IMMA_CLIENT_NODE *cl_node = NULL;
	NCS_BOOL locked = TRUE;

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

	if (cb->is_immnd_up == FALSE) {
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
	/* locked == TRUE already*/

	imma_client_node_get(&cb->client_tree, &immOiHandle, &cl_node);
	if (!cl_node || cl_node->isOm) {
		rc = SA_AIS_ERR_BAD_HANDLE;
		TRACE_2("ERR_BAD_HANDLE: Non valid SaImmOiHandleT");
		goto bad_handle;
	}

	if (cl_node->stale) {
		TRACE_1("Handle %llx is stale", immOiHandle);
		NCS_BOOL resurrected = imma_oi_resurrect(cb, cl_node, &locked);

		if (!locked && m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
			TRACE_4("ERR_LIBRARY: LOCK failed");
			rc = SA_AIS_ERR_LIBRARY;
			goto lock_fail;
		}
		locked = TRUE;

		imma_client_node_get(&cb->client_tree, &immOiHandle, &cl_node);

		if (!resurrected || !cl_node || cl_node->isOm || cl_node->stale) {
			TRACE_2("ERR_BAD_HANDLE: Reactive ressurect of handle %llx failed",
				immOiHandle);
			if (cl_node && cl_node->stale) {cl_node->exposed = TRUE;}
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

	assert(evt.info.immnd.info.objCreate.attrValues == NULL);

	const SaImmAttrValuesT_2 *attr;
	int i;
	for (i = 0; attrValues[i]; ++i) {
		attr = attrValues[i];
		TRACE("attr:%s \n", attr->attrName);

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
		IMMSV_ATTR_VALUES_LIST *p = calloc(1, sizeof(IMMSV_ATTR_VALUES_LIST));

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

	rc = imma_evt_fake_evs(cb, &evt, &out_evt, IMMSV_WAIT_TIME, cl_node->handle, &locked, TRUE);

	cl_node=NULL;

	if (!locked && m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != 
		NCSCC_RC_SUCCESS) {
		rc = SA_AIS_ERR_LIBRARY; /* Overwrites any error from fake_evs() */
		/* Losing track of the pending reply count, but ERR_LIBRARY dominates*/
		TRACE_4("ERR_LIBRARY: LOCK failed");
		goto mds_send_fail;
	}
	locked = TRUE;

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
		cl_node->exposed = TRUE;
		TRACE_2("ERR_BAD_HANDLE: Handle %llx is stale", immOiHandle);
		goto mds_send_fail;
	}

	assert(out_evt);
	assert(out_evt->type == IMMSV_EVT_TYPE_IMMA);
	assert(out_evt->info.imma.type == IMMA_EVT_ND2A_IMM_ERROR);
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
	NCS_BOOL locked = TRUE;

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

	if (cb->is_immnd_up == FALSE) {
		TRACE_2("ERR_TRY_AGAIN: IMMND is DOWN");
		return SA_AIS_ERR_TRY_AGAIN;
	}

	/* get the CB Lock */
	if (m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
		rc = SA_AIS_ERR_LIBRARY;
		TRACE_4("ERR_LIBRARY: Lock failed");
		goto lock_fail;
	}
	/*locked == TRUE already*/

	imma_client_node_get(&cb->client_tree, &immOiHandle, &cl_node);
	if (!cl_node || cl_node->isOm) {
		rc = SA_AIS_ERR_BAD_HANDLE;
		TRACE_2("ERR_BAD_HANDLE: Not a valid SaImmOiHandleT");
		goto bad_handle;
	}

	if (cl_node->stale) {
		TRACE_1("Handle %llx is stale", immOiHandle);
		NCS_BOOL resurrected = imma_oi_resurrect(cb, cl_node, &locked);

		if (!locked && m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
			TRACE_4("ERR_LIBRARY: LOCK failed");
			rc = SA_AIS_ERR_LIBRARY;
			goto lock_fail;
		}
		locked = TRUE;

		imma_client_node_get(&cb->client_tree, &immOiHandle, &cl_node);

		if (!resurrected || !cl_node || cl_node->isOm || cl_node->stale) {
			TRACE_2("ERR_BAD_HANDLE: Reactive ressurect of handle %llx failed",
				immOiHandle);
			if (cl_node && cl_node->stale) {cl_node->exposed = TRUE;}
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

	rc = imma_evt_fake_evs(cb, &evt, &out_evt, IMMSV_WAIT_TIME, cl_node->handle, &locked, TRUE);

	cl_node = NULL;

	if (!locked && m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != 
		NCSCC_RC_SUCCESS) {
		rc = SA_AIS_ERR_LIBRARY; /* Overwrites any error from fake_evs() */
		/* Losing track of the pending reply count, but ERR_LIBRARY dominates*/
		TRACE_4("ERR_LIBRARY: LOCK failed");
		goto cleanup;
	}
	locked = TRUE;

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
		cl_node->exposed = TRUE;
		TRACE_2("ERR_BAD_HANDLE: Handle %llx is stale", immOiHandle);
		goto cleanup;
	}

	assert(out_evt);
	assert(out_evt->type == IMMSV_EVT_TYPE_IMMA);
	assert(out_evt->info.imma.type == IMMA_EVT_ND2A_IMM_ERROR);
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

/* Tries to set implementer for resurrected handle.
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
	SaBoolT locked = SA_FALSE;
	TRACE_ENTER();

	if (m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
		TRACE_3("Lock failure");
		goto fail;
	}
	locked = TRUE;

	imma_client_node_get(&cb->client_tree, &immOiHandle, &cl_node);
	if (!cl_node || cl_node->isOm || cl_node->stale) {
		TRACE_3("client_node_get failed");
		goto fail;
	}

	implName = cl_node->mImplementerName;
	cl_node->mImplementerName = NULL;
	cl_node->mImplementerId = 0;

	m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
	locked = FALSE;
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
		   cl_node->stale/exposed both set to TRUE.
		 */
		goto fail;
	}

 success:
	assert(!locked);
	TRACE_LEAVE();
	return SA_TRUE;

 fail:
	if (locked) {m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);}
	TRACE_LEAVE();
	return SA_FALSE;
}

int imma_oi_resurrect(IMMA_CB *cb, IMMA_CLIENT_NODE *cl_node, NCS_BOOL *locked)
{
	IMMSV_EVT  finalize_evt, *out_evt = NULL;
	TRACE_ENTER();
	assert(locked && *locked);
	assert(cl_node && cl_node->stale);
	SaImmOiHandleT immOiHandle = cl_node->handle;

	m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
	*locked = FALSE;
	cl_node = NULL;
	if (!imma_proc_resurrect_client(cb, immOiHandle, FALSE)) {
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
	*locked = TRUE;

	imma_client_node_get(&cb->client_tree, &immOiHandle, &cl_node);
	if (cl_node && !cl_node->isOm) {
		cl_node->stale = TRUE;
		cl_node->exposed = TRUE;
	} else {
		TRACE_3("client_node_get failed");
	}

	m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
	*locked = FALSE;
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
			&(cb->immnd_mds_dest),&finalize_evt,&out_evt,
			IMMSV_WAIT_TIME);

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
