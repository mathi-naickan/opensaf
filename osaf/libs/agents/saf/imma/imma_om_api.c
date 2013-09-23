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

#define _GNU_SOURCE
#include <string.h>

#include "imma.h"
#include "immsv_api.h"

static const char *immLoaderName = IMMSV_LOADERNAME;	/*Defined in immsv_evt.h */

/* TODO: ABT move these to cb ? OR stop using the cb*/
static SaInt32T immInvocations = 0;
static SaBoolT immOmIsLoader = SA_FALSE;

static const char *sysaClName = SA_IMM_ATTR_CLASS_NAME;
static const char *sysaAdmName = SA_IMM_ATTR_ADMIN_OWNER_NAME;
static const char *sysaImplName = SA_IMM_ATTR_IMPLEMENTER_NAME;

static int imma_om_resurrect(IMMA_CB *cb, IMMA_CLIENT_NODE *cl_node, bool *locked);


/****************************************************************************
  Name          :  SaImmOmInitialize
 
  Description   :  This function initializes the ImmOm Service for the
                   invoking process and registers the callback functions.
                   The handle 'immHandle' is returned as the reference to this
                   association between the process and  the ImmOm Service.
                   
 
  Arguments     :  immHandle -  A pointer to the handle designating this 
                                particular initialization of the IMM OM
                                service that is to be returned by the 
                                ImmOm Service.
                   immCallbacks  - Pointer to a SaImmCallbacksT structure, 
                                containing the callback functions of the
                                process that the ImmOm Service may invoke.
                   version    - Is a pointer to the version of the Imm
                                Service that the invoking process is using.
 


  Return Values :  Refer to SAI-AIS specification for various return values.
 
  Notes         :
******************************************************************************/
static SaAisErrorT initialize_common(SaImmHandleT *immHandle, IMMA_CLIENT_NODE *cl_node, SaVersionT *version);

SaAisErrorT saImmOmInitialize_o2(SaImmHandleT *immHandle, const SaImmCallbacksT_o2 *immCallbacks, SaVersionT *inout_version)
{
	IMMA_CLIENT_NODE *cl_node=NULL;
	SaAisErrorT rc = SA_AIS_OK;
	SaVersionT requested_version;

	TRACE_ENTER();
	if ((!immHandle) || (!inout_version)) {
		TRACE_2("ERR_INVALID_PARAM: immHandle is NULL or version is NULL");
		return SA_AIS_ERR_INVALID_PARAM;
	}

	requested_version = *inout_version;

	if ((requested_version.releaseCode != 'A') || (requested_version.majorVersion != 0x02) ||
	    (requested_version.minorVersion < 11)) {
		TRACE_2("ERR_VERSION: THIS SHOULD BE A VERSION A.2.11 initialize but claims to be"
		      "%c %u %u", requested_version.releaseCode, requested_version.majorVersion, 
			requested_version.minorVersion);
		imma_version_validate(inout_version);
		return SA_AIS_ERR_VERSION;
	}

	/* Draft Validations : Version */
	rc = imma_version_validate(inout_version);
	if (rc != SA_AIS_OK) {
		TRACE_2("ERR_VERSION: Version validation failed");
		return rc;
	}

	/* Alloc the client info data structure */
	cl_node = (IMMA_CLIENT_NODE *)calloc(1, sizeof(IMMA_CLIENT_NODE));

	if (cl_node == NULL) {
		TRACE_4("ERR_NO_MEMORY: IMMA_CLIENT_NODE alloc failed");
		return SA_AIS_ERR_NO_MEMORY;
	}

	cl_node->isImmA2b = true;

	/* Store the callback functions, if set */
	if (immCallbacks) {
		cl_node->o.mCallbkA2b = *immCallbacks;
		cl_node->isImmA2bCbk = true;

	}

	return initialize_common(immHandle, cl_node, &requested_version);
}

SaAisErrorT saImmOmInitialize(SaImmHandleT *immHandle, const SaImmCallbacksT *immCallbacks, SaVersionT *inout_version)
{
	IMMA_CLIENT_NODE *cl_node=NULL;
	SaAisErrorT rc = SA_AIS_OK;
	SaVersionT requested_version;
	TRACE_ENTER();

	if ((!immHandle) || (!inout_version)) {
		TRACE_2("ERR_INVALID_PARAM: immHandle is NULL or version is NULL");
		return SA_AIS_ERR_INVALID_PARAM;
	}

	requested_version = *inout_version;

	/* Draft Validations : Version */
	rc = imma_version_validate(inout_version);
	if (rc != SA_AIS_OK) {
		TRACE_2("ERR_VERSION: Version validation failed");
		return rc;
	}

	/* Alloc the client info data structure */
	cl_node = (IMMA_CLIENT_NODE *)calloc(1, sizeof(IMMA_CLIENT_NODE));

	if (cl_node == NULL) {
		TRACE_4("ERR_NO_MEMORY: IMMA_CLIENT_NODE alloc failed");
		return SA_AIS_ERR_NO_MEMORY;
	}

	if ((requested_version.releaseCode == 'A') &&
            (requested_version.majorVersion == 0x02) &&
	    (requested_version.minorVersion >= 0x0b)) {
		TRACE("OM client version A.2.11");
		cl_node->isImmA2b = true;
	}

	/* Store the callback functions, if set */
	cl_node->isImmA2bCbk = false; /* redundant */
	if (immCallbacks) {
		cl_node->o.mCallbk = *immCallbacks;
	}

	rc = initialize_common(immHandle, cl_node, &requested_version);
	TRACE_LEAVE();
	return rc;
}

static SaAisErrorT initialize_common(SaImmHandleT *immHandle, IMMA_CLIENT_NODE *cl_node, SaVersionT *version)
{
	IMMA_CB *cb = &imma_cb;
	SaAisErrorT rc = SA_AIS_OK;
	IMMSV_EVT init_evt;
	IMMSV_EVT *out_evt = NULL;
	uint32_t proc_rc = NCSCC_RC_SUCCESS;
	bool locked = true;
	char *timeout_env_value = NULL;
	TRACE_ENTER();
	osafassert(immHandle && cl_node);

	proc_rc = imma_startup(NCSMDS_SVC_ID_IMMA_OM);
	if (NCSCC_RC_SUCCESS != proc_rc) {
		TRACE_4("ERR_LIBRARY: imma startup failed:%u", proc_rc);
		rc = SA_AIS_ERR_LIBRARY;
		goto lock_fail;
	}

	if (false == cb->is_immnd_up) {
		TRACE_2("ERR_TRY_AGAIN: IMMND is DOWN");
		rc = SA_AIS_ERR_TRY_AGAIN;
		goto lock_fail;
	}

	if((timeout_env_value = getenv("IMMA_SYNCR_TIMEOUT"))!=NULL) {
		cl_node->syncr_timeout = atoi(timeout_env_value);
		TRACE_2("IMMA library syncronous timeout set to:%u", cl_node->syncr_timeout);
	}

	if(cl_node->syncr_timeout < NCS_SAF_MIN_ACCEPT_TIME) {
		cl_node->syncr_timeout = IMMSV_WAIT_TIME; /* Default */
	}
	
	*immHandle = 0;

	if (m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
		TRACE_4("ERR_LIBRARY: Lock failed");
		rc = SA_AIS_ERR_LIBRARY;
		goto lock_fail;
	}
	/* locked == true already */

	/* Put the client info data structure in the Pat tree */

	if ((cl_node->isImmA2bCbk && cl_node->o.mCallbkA2b.saImmOmAdminOperationInvokeCallback) ||
		(!(cl_node->isImmA2bCbk) && cl_node->o.mCallbk.saImmOmAdminOperationInvokeCallback))
	{
		proc_rc = imma_callback_ipc_init(cl_node);
		if (proc_rc != NCSCC_RC_SUCCESS) {
			rc = SA_AIS_ERR_LIBRARY;
			/* ALready log'ed by imma_callback_ipc_init */
			goto ipc_init_fail;
		}
	}

	/* populate the EVT structure */
	memset(&init_evt, 0, sizeof(IMMSV_EVT));
	init_evt.type = IMMSV_EVT_TYPE_IMMND;
	init_evt.info.immnd.type = IMMND_EVT_A2ND_IMM_INIT;
	init_evt.info.immnd.info.initReq.version = *version;
	init_evt.info.immnd.info.initReq.client_pid = getpid();

	/* Release the CB lock Before MDS Send */
	m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
	locked = false;
	/* cl_node has not been added to tree in cb yet so safe to access 
	   without cb-lock.
	 */

	if (false == cb->is_immnd_up) {
		rc = SA_AIS_ERR_TRY_AGAIN;
		TRACE_3("ERR_TRY_AGAIN: IMMND is DOWN");
		goto mds_fail;
	}

	/* send the request to the IMMND */
	proc_rc = imma_mds_msg_sync_send(cb->imma_mds_hdl, &cb->immnd_mds_dest, &init_evt, &out_evt, cl_node->syncr_timeout);

	/* Error Handling */
	switch (proc_rc) {
		case NCSCC_RC_SUCCESS:
			break;
		case NCSCC_RC_REQ_TIMOUT:
			rc = SA_AIS_ERR_TIMEOUT;
			goto mds_fail;
		default:
			TRACE_4("ERR_LIBRARY: Mds returned unexpected error code: %u", proc_rc);
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
			TRACE_4("ERR_LIBRARY: Lock failed");
			rc = SA_AIS_ERR_LIBRARY;
			goto lock_fail1;
		} 

		locked = true;

		if (false == cb->is_immnd_up) {
			/*IMMND went down during THIS call! */
			rc = SA_AIS_ERR_TRY_AGAIN;
			TRACE_3("ERR_TRY_AGAIN: IMMND is DOWN");
			goto rsp_not_ok;
		}

		cl_node->handle = out_evt->info.imma.info.initRsp.immHandle;
		cl_node->isOm = true;

		TRACE_1("Trying to add OM client id:%u node:%x",
			m_IMMSV_UNPACK_HANDLE_HIGH(cl_node->handle),
			m_IMMSV_UNPACK_HANDLE_LOW(cl_node->handle));
		proc_rc = imma_client_node_add(&cb->client_tree, cl_node);
		if (proc_rc != NCSCC_RC_SUCCESS) {
			IMMA_CLIENT_NODE *stale_node = NULL;
			imma_client_node_get(&cb->client_tree, &(cl_node->handle), &stale_node);

			if ((stale_node != NULL) && stale_node->stale) {
				TRACE_1("Removing stale client");
				imma_finalize_client(cb, stale_node);
				proc_rc = imma_shutdown(NCSMDS_SVC_ID_IMMA_OM);
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
		finalize_evt.info.immnd.type = IMMND_EVT_A2ND_IMM_FINALIZE;
		finalize_evt.info.immnd.info.finReq.client_hdl = cl_node->handle;

		if (locked) {
			m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
			locked = false;
		}

		/* send the request to the IMMND */
		proc_rc = imma_mds_msg_sync_send(cb->imma_mds_hdl,
			&(cb->immnd_mds_dest), &finalize_evt, &out_evt1, cl_node->syncr_timeout);

		if (out_evt1) {
			free(out_evt1);
			out_evt1 =NULL;
		}
	}

 rsp_not_ok:
 mds_fail:

	/* Free the IPC initialized for this client */
	if (rc != SA_AIS_OK)
		imma_callback_ipc_destroy(cl_node);

 ipc_init_fail:
 lock_fail:
	if (rc != SA_AIS_OK) {
		free(cl_node);
		cl_node=NULL;
	}

	if (locked)
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);


	if (out_evt) {
		free(out_evt);
		out_evt=NULL;
	}

	if (rc == SA_AIS_OK) {
		/* Went well, return immHandle to the application */
		*immHandle = cl_node->handle;
	}

	if (rc != SA_AIS_OK) {
		if (NCSCC_RC_SUCCESS != imma_shutdown(NCSMDS_SVC_ID_IMMA_OM)) {
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
  Name          :  saImmOmSelectionObjectGet
 
  Description   :  This function returns the operating system handle 
                   associated with the immHandle.
 
  Arguments     :  immHandle - Imm OM service handle.
                   selectionObject - Pointer to the operating system handle.
 
  Return Values :  Refer to SAI-AIS specification for various return values.
 
  Notes         :
******************************************************************************/
SaAisErrorT saImmOmSelectionObjectGet(SaImmHandleT immHandle, SaSelectionObjectT *selectionObject)
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
		TRACE_3("ERR_TRY_AGAIN: IMMND_DOWN");
		return SA_AIS_ERR_TRY_AGAIN;
	}

	*selectionObject = (-1);	/* Ensure non valid descriptor in case of failure. */

	/* Take the CB lock */
	if (m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
		TRACE_4("ERR_LIBRARY: Lock failed");
		rc = SA_AIS_ERR_LIBRARY;
		goto lock_fail;
	}
	/* locked == true already */

	imma_client_node_get(&cb->client_tree, &immHandle, &cl_node);

	if (!(cl_node && cl_node->isOm)) {
		TRACE_2("ERR_BAD_HANDLE: Bad handle %llx", immHandle);
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto node_not_found;
	}

	if (cl_node->isImmA2bCbk) {/* cl_node->isImmA2bCbk is true only if there is an A.2.11 callback */
		if(cl_node->o.mCallbkA2b.saImmOmAdminOperationInvokeCallback==NULL) {
			TRACE_2("ERR_INVALID_PARAM: saImmOmSelectionObjectGet not allowed "
				"when saImmOmAdminOperationInvokeCallback is NULL");
			rc = SA_AIS_ERR_INVALID_PARAM;
			goto no_callback;
		}
	} else {
		if(cl_node->o.mCallbk.saImmOmAdminOperationInvokeCallback==NULL) {
			TRACE_2("ERR_INVALID_PARAM: saImmOmSelectionObjectGet not allowed "
				"when saImmOmAdminOperationInvokeCallback is NULL");
			rc = SA_AIS_ERR_INVALID_PARAM;
			goto no_callback;
		}
	}

	if (cl_node->stale) {
		TRACE_1("Handle %llx is stale", immHandle);
		bool resurrected = imma_om_resurrect(cb, cl_node, &locked);

		if (!locked && m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
			TRACE_4("ERR_LIBRARY: LOCK failed");
			rc = SA_AIS_ERR_LIBRARY;
			goto lock_fail;
		}
		locked = true;

		imma_client_node_get(&cb->client_tree, &immHandle, &cl_node);

		if (!resurrected || !cl_node || !(cl_node->isOm) || cl_node->stale) {
			TRACE_3("ERR_BAD_HANDLE: Reactive ressurect of handle %llx failed", immHandle);
			if (cl_node && cl_node->stale) {cl_node->exposed = true;}
			rc = SA_AIS_ERR_BAD_HANDLE;
			goto stale_handle;
		}

		TRACE_1("Reactive resurrect of handle %llx succeeded", immHandle);
	}

	*selectionObject = (SaSelectionObjectT)
		m_GET_FD_FROM_SEL_OBJ(m_NCS_IPC_GET_SEL_OBJ(&cl_node->callbk_mbx));

	cl_node->selObjUsable = true;

 no_callback:
 stale_handle:
 node_not_found:
	if (locked)
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);

 lock_fail:
	TRACE_LEAVE();
	return rc;
}


/****************************************************************************
  Name          :  saImmOmDispatch
 
  Description   :  This function invokes, in the context of the calling
                   thread. pending callbacks for the handle immHandle in a 
                   way that is specified by the dispatchFlags parameter.
 
  Arguments     :  immHandle - IMM OM Service handle
                   dispatchFlags - Flags that specify the callback execution
                                   behaviour of this function.
 
  Return Values :  Refer to SAI-AIS specification for various return values.
 
  Notes         :
******************************************************************************/
SaAisErrorT saImmOmDispatch(SaImmHandleT immHandle, SaDispatchFlagsT dispatchFlags)
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
		TRACE_4("ERR_LIBRARY: Lock failed");
		rc = SA_AIS_ERR_LIBRARY;
		goto fail;
	}
	locked = true;

	/* get the client_info */
	imma_client_node_get(&cb->client_tree, &immHandle, &cl_node);
	if (!(cl_node && cl_node->isOm)) {
		TRACE_2("ERR_BAD_HANDLE: client-node_get failed");
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto fail;
	}

	if (cl_node->stale) {
		TRACE_1("Handle %llx is stale, trying to resurrect it.", immHandle);

		if (cb->dispatch_clients_to_resurrect == 0) {
			rc = SA_AIS_ERR_BAD_HANDLE;
			cl_node->exposed = true;
			goto fail;
		} 

		--(cb->dispatch_clients_to_resurrect);
		TRACE_1("Remaining clients to actively resurrect: %d", 
			cb->dispatch_clients_to_resurrect);

		if (!imma_om_resurrect(cb, cl_node, &locked)) {
			TRACE_3("ERR_BAD_HANDLE: Failed to resurrect stale OM handle <c:%u, n:%x>",
				m_IMMSV_UNPACK_HANDLE_HIGH(immHandle),
				m_IMMSV_UNPACK_HANDLE_LOW(immHandle));
			rc = SA_AIS_ERR_BAD_HANDLE;
			goto fail;
		}

		TRACE_1("Successfully resurrected OM handle <c:%u, n:%x>",
			m_IMMSV_UNPACK_HANDLE_HIGH(immHandle),
			m_IMMSV_UNPACK_HANDLE_LOW(immHandle));

		if (!locked && m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS){
			TRACE_4("ERR_LIBRARY: Lock failure");
			rc = SA_AIS_ERR_LIBRARY;
			goto fail;
		}
		locked = true;

		/* get the client again. */
		imma_client_node_get(&cb->client_tree, &immHandle, &cl_node);
		if (!(cl_node && cl_node->isOm)) {
			TRACE_3("ERR_BAD_HANDLE: client_node_get failed AFTER successful resurrect!");
			rc = SA_AIS_ERR_BAD_HANDLE;
			goto fail;
		}

		if (cl_node->stale) {
			TRACE_3("ERR_BAD_HANDLE: client became stale AGAIN after successful resurrect!");
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
	cl_node = NULL; /* Prevent unsafe use.*/
	/* unlocked */
	
  	/* Increment Dispatch usgae count */
   	cb->pend_dis++;

	switch (dispatchFlags) {
		case SA_DISPATCH_ONE:
			rc = imma_hdl_callbk_dispatch_one(cb, immHandle);
			break;

		case SA_DISPATCH_ALL:
			rc = imma_hdl_callbk_dispatch_all(cb, immHandle);
			break;

		case SA_DISPATCH_BLOCKING:
			rc = imma_hdl_callbk_dispatch_block(cb, immHandle);
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

	/* see if we are still in any dispact context */ 
	if(pend_dis == 0) {
		while(pend_fin != 0) {
			/* call agent shutdown,for each finalize done before */
			cb->pend_fin --;
			imma_shutdown(NCSMDS_SVC_ID_IMMA_OM);
			pend_fin--;
		}
	}

	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
  Name          :  saImmOmFinalize
 
  Description   :  This function closes the association, represented by
                   immHandle, between the invoking process and the IMM OM
                   Service.
 
  Arguments     :  immHandle - IMM OM Service handle.
 
  Return Values :  Refer to SAI-AIS specification for various return values.
 
  Notes         :
******************************************************************************/
SaAisErrorT saImmOmFinalize(SaImmHandleT immHandle)
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
		TRACE_4("ERR_LIBRARY: Lock failed");
		rc = SA_AIS_ERR_LIBRARY;
		goto lock_fail;
	}
	/* locked == true already */

	/* get the client_info */
	imma_client_node_get(&cb->client_tree, &immHandle, &cl_node);
	if (!(cl_node && cl_node->isOm)) {
		rc = SA_AIS_ERR_BAD_HANDLE;
		TRACE_2("ERR_BAD_HANDLE: client_node_get failed");
		goto node_not_found;
	}

	/*Increment before stale check to get uniform stale handling 
	  before and after send (see stale_handle:)
	*/
	imma_proc_increment_pending_reply(cl_node);

	if (cl_node->stale) {
		TRACE_1("Handle %llx is stale", immHandle);
		rc = SA_AIS_OK;	/*Dont punish the client for closing stale handle */
		/* Dont try to resurrect since this is finalize. */
		cl_node->exposed = true; /* But dont allow anyone else to resurrect it either */
		goto stale_handle;
	}

	/* populate the structure */
	memset(&finalize_evt, 0, sizeof(IMMSV_EVT));
	finalize_evt.type = IMMSV_EVT_TYPE_IMMND;
	finalize_evt.info.immnd.type = IMMND_EVT_A2ND_IMM_FINALIZE;
	finalize_evt.info.immnd.info.finReq.client_hdl = cl_node->handle;

	timeout = cl_node->syncr_timeout;
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
			TRACE_3("Got ERR_TIMEOUT in saImmOmFinalize - ignoring");
			/* Yes could be a stale handle, but this is handle finalize.
			   Dont cause unnecessary problems by returnign an error code. 
			   If this is a true timeout caused by an unusually sluggish but
			   up IMMND, then this connection at the IMMND side may linger,
			   but on this IMMA side we will drop it.
			*/
			goto stale_handle;

		default:
			TRACE_4("ERR_LIBRARY: Mds returned unexpected error code: %u", proc_rc);
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
		/* rc = SA_AIS_ERR_LIBRARY;
		   This is a finalize, no point in disturbing the user with
		   a communication error. 
		*/
		TRACE_3("Received empty reply from server");
	}

 stale_handle:
	/* Do the finalize processing at IMMA */
	if (rc == SA_AIS_OK) {
		/* Take the CB lock  */
		if (!locked && m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
			rc = SA_AIS_ERR_LIBRARY;
			/* Losing track of the pending reply count, but ERR_LIBRARY dominates*/
			TRACE_4("ERR_LIBRARY: Lock failed");
			goto lock_fail1;
		}

		locked = true;

		/* get the client_info */
		imma_client_node_get(&cb->client_tree, &immHandle, &cl_node);
		if (!(cl_node && cl_node->isOm)) {
			/*rc = SA_AIS_ERR_BAD_HANDLE; This is finalize.*/
			TRACE_3("client_node_get failed");
			goto node_not_found;
		}

		if (cl_node->stale) {
			TRACE_3("Handle %llx is stale", immHandle);
			/*Dont punish the client for closing stale handle rc == SA_AIS_OK */

			cl_node->exposed = true;  /* But dont resurrect it either */
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
    /* we are not in any dispatch context, we can do agent shutdown */
  	if((agent_flag == true) && (rc == SA_AIS_OK) &&
		(NCSCC_RC_SUCCESS != imma_shutdown(NCSMDS_SVC_ID_IMMA_OM))) {

		TRACE_4("ERR_LIBRARY: Call to imma_shutdown failed");
		rc = SA_AIS_ERR_LIBRARY;
	}
	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
  Name          :  saImmOmAdminOwnerInitialize
 
  Description   :  Initialize an admin owner handle.
                   This a blocking syncronous call.

                   
  Arguments     :  immHandle - IMM OM handle
                   adminOwnerName - The name of the admin owner.
                   releaseOwnershipOnFinalize - se the IMM spec.
                   adminOwnerHandle - The handle that is returned.

  Return Values :  Refer to SAI-AIS specification for various return values.
 
  Notes         : 
******************************************************************************/
SaAisErrorT saImmOmAdminOwnerInitialize(SaImmHandleT immHandle,
					const SaImmAdminOwnerNameT adminOwnerName,
					SaBoolT releaseOwnershipOnFinalize, SaImmAdminOwnerHandleT *adminOwnerHandle)
{
	SaAisErrorT rc = SA_AIS_OK;
	IMMA_CB *cb = &imma_cb;
	IMMSV_EVT evt;
	IMMSV_EVT *out_evt = NULL;
	IMMA_CLIENT_NODE *cl_node = NULL;
	IMMA_ADMIN_OWNER_NODE *ao_node = NULL;
	SaUint32T proc_rc = NCSCC_RC_SUCCESS;
	bool locked = true;
	bool isLoaderName = false;
	SaUint32T nameLen = 0;
	SaUint32T timeout = 0;
	TRACE_ENTER();

	if (cb->sv_id == 0) {
		TRACE_2("ERR_BAD_HANDLE: No initialized handle exists!");
		return SA_AIS_ERR_BAD_HANDLE;
	}

	if ((adminOwnerHandle == NULL) || (adminOwnerName == NULL) || (nameLen = strlen(adminOwnerName)) == 0) {
		TRACE_2("ERR_INVALID_PARAM: 'adminOwnerHandle is NULL, or adminOwnerName is NULL, or adminOwnerName "
				"has zero length");
		return SA_AIS_ERR_INVALID_PARAM;
	}

	if (releaseOwnershipOnFinalize && (releaseOwnershipOnFinalize != 1)) {
		TRACE_2("ERR_INVALID_PARAM: releaseOwnershipOnFinalize must be 1 or 0");
		return SA_AIS_ERR_INVALID_PARAM;
	}

	if (nameLen >= SA_MAX_NAME_LENGTH) {
		TRACE_2("ERR_INVALID_PARAM: Admin owner name too long, size: %u max:%u", nameLen, SA_MAX_NAME_LENGTH - 1);
		return SA_AIS_ERR_INVALID_PARAM;
	}

	if (cb->is_immnd_up == false) {
		TRACE_2("ERR_TRY_AGAIN: IMMND is DOWN");
		return SA_AIS_ERR_TRY_AGAIN;
	}

	*adminOwnerHandle = 0;
	isLoaderName = (strncmp(adminOwnerName, immLoaderName, nameLen) == 0);

	/* get the CB Lock */
	if (m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
		rc = SA_AIS_ERR_LIBRARY;
		TRACE_4("ERR_LIBRARY: Lock failed");
		goto lock_fail;
	}

	/*locked == true already */

	imma_client_node_get(&cb->client_tree, &immHandle, &cl_node);
	if (!(cl_node && cl_node->isOm)) {
		rc = SA_AIS_ERR_BAD_HANDLE;
		TRACE_2("ERR_BAD_HANDLE: client_node_get failed");
		goto bad_handle;
	}

	if (cl_node->stale) {
		TRACE_1("Handle %llx is stale", immHandle);
		bool resurrected = imma_om_resurrect(cb, cl_node, &locked);

		if (!locked && m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
			TRACE_4("ERR_LIBRARY: LOCK failed");
			rc = SA_AIS_ERR_LIBRARY;
			goto lock_fail;
		}
		locked = true;

		imma_client_node_get(&cb->client_tree, &immHandle, &cl_node);

		if (!resurrected || !cl_node || !(cl_node->isOm) || cl_node->stale) {
			TRACE_3("ERR_BAD_HANDLE: Reactive ressurect of handle %llx failed", immHandle);
			if (cl_node && cl_node->stale) {cl_node->exposed = true;}
			rc = SA_AIS_ERR_BAD_HANDLE;
			goto bad_handle;
		}

		TRACE_1("Reactive resurrect of handle %llx succeeded", immHandle);
	}

	/* Allocate the IMMA_ADMIN_OWNER_NODE & Populate */
	ao_node = (IMMA_ADMIN_OWNER_NODE *)calloc(1, sizeof(IMMA_ADMIN_OWNER_NODE));
	if (ao_node == NULL) {
		rc = SA_AIS_ERR_NO_MEMORY;
		TRACE_4("ERR_MEMORY: Memory allocation error");
		goto ao_node_alloc_fail;
	}

	ao_node->admin_owner_hdl = (SaImmAdminOwnerHandleT)m_NCS_GET_TIME_NS;
	/*This is the external handle that the application uses.
	  Internally we use the gloablId provided by the director. */

	ao_node->mImmHandle = immHandle;

	/* Populate & Send the Event to IMMND */
	memset(&evt, 0, sizeof(IMMSV_EVT));
	evt.type = IMMSV_EVT_TYPE_IMMND;
	evt.info.immnd.type = IMMND_EVT_A2ND_IMM_ADMINIT;
	evt.info.immnd.info.adminitReq.client_hdl = immHandle;
	evt.info.immnd.info.adminitReq.i.adminOwnerName.length = nameLen;
	memcpy(evt.info.immnd.info.adminitReq.i.adminOwnerName.value, adminOwnerName, nameLen + 1);
	if (releaseOwnershipOnFinalize) {
		evt.info.immnd.info.adminitReq.i.releaseOwnershipOnFinalize = true;
		/* Release on finalize can not be undone in case of IMMND crash.
		   The om-handle can then not be resurrected unless this admin-owner
		   has been finalized before the IMMND crash.
		*/
		ao_node->mReleaseOnFinalize = true;
	}

	imma_proc_increment_pending_reply(cl_node);
	timeout = cl_node->syncr_timeout;

	/* Unlock before MDS Send */
	m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
	locked = false;
	cl_node = NULL; /* Prevent unsafe use. */

	if (cb->is_immnd_up == false) {
		rc = SA_AIS_ERR_TRY_AGAIN;
		TRACE_3("ERR_TRY_AGAIN: IMMND is DOWN");
		goto mds_send_fail;
	}

	/* Send the evt to IMMND */
	proc_rc = imma_mds_msg_sync_send(cb->imma_mds_hdl, &(cb->immnd_mds_dest), &evt, &out_evt, timeout);
	/* Generate rc from proc_rc */
	switch (proc_rc) {
		case NCSCC_RC_SUCCESS:
			break;
		case NCSCC_RC_REQ_TIMOUT:
			rc = imma_proc_check_stale(cb, immHandle, SA_AIS_ERR_TIMEOUT);
			break; /* i.e. goto mds_send_fail */
		default:
			rc = SA_AIS_ERR_LIBRARY;
			TRACE_4("ERR_LIBRARY: MDS returned unexpected error code %u", proc_rc);
			/* Losing track of the pending reply count, but ERR_LIBRARY dominates*/
			goto admin_owner_node_free;
	}

 mds_send_fail:
	if (m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
		rc = SA_AIS_ERR_LIBRARY;
		TRACE_4("ERR_LIBRARY: Lock failed");
		/* Losing track of the pending reply count, but ERR_LIBRARY dominates*/
		goto lock_fail;
	}
	locked = true;

	imma_client_node_get(&cb->client_tree, &immHandle, &cl_node);	
	if (!(cl_node && cl_node->isOm)) {
		rc = SA_AIS_ERR_BAD_HANDLE;
		TRACE_3("ERR_BAD_HANDLE: client_node_get failed after down-call");
		goto  admin_owner_node_free;
	}

	imma_proc_decrement_pending_reply(cl_node);

	if (cl_node->stale) {
		if (isExposed(cb, cl_node)) {
			TRACE_3("ERR_BAD_HANDLE: Handle %llx became stale and exposed", immHandle);
			rc = SA_AIS_ERR_BAD_HANDLE;
		} else {
			TRACE_3("ERR_TRY_AGAIN: Handle %llx became stale but possible to resurrect", 
				immHandle);
			/* Can override BAD_HANDLE/TIMEOUT set by check_stale */
			rc = SA_AIS_ERR_TRY_AGAIN;
		}
		goto admin_owner_node_free;
	}

	if(rc != SA_AIS_OK) {
		TRACE_2("Error already set %u", rc);
		goto admin_owner_node_free;
	}

	if (out_evt) {
		/* Process the received Event */
		rc = out_evt->info.imma.info.admInitRsp.error;
		if (rc == SA_AIS_OK) {		
			ao_node->mAdminOwnerId = out_evt->info.imma.info.admInitRsp.ownerId;
			ao_node->mAdminOwnerName = calloc(1, nameLen+1);
			strncpy(ao_node->mAdminOwnerName, adminOwnerName, nameLen);
		} else {goto admin_owner_node_free;}
	} else {
		TRACE_4("ERR_LIBRARY: Empty reply message for AdminOwnerInitialize");
		rc = SA_AIS_ERR_LIBRARY;
		goto admin_owner_node_free;
	}

	do {
		proc_rc = imma_admin_owner_node_add(&cb->admin_owner_tree, ao_node);

		if (proc_rc != NCSCC_RC_SUCCESS ) {
			IMMA_ADMIN_OWNER_NODE *ao_node_tmp = NULL;
			imma_admin_owner_node_get(&cb->admin_owner_tree, &(ao_node->admin_owner_hdl), &ao_node_tmp);
			if(!ao_node_tmp) {
				LOG_NO("Failed to add node to the admin owner tree - aborting");
				abort();
			}
			ao_node->admin_owner_hdl++;
			TRACE_4("Duplicate admin owner handle %llu (poor clock resolution) adjusting it to %llu",
				ao_node_tmp->admin_owner_hdl, ao_node->admin_owner_hdl);
		}
	} while (proc_rc != NCSCC_RC_SUCCESS);

	*adminOwnerHandle = ao_node->admin_owner_hdl;

	if (isLoaderName) {
		TRACE_1("This appears to be a LOADER client");
		immOmIsLoader = true;
	}

	if (locked)
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);

	free(out_evt);
	out_evt = NULL;

	TRACE_1("Admin owner init successful");
	TRACE_LEAVE();
	return SA_AIS_OK;

	/* Error Handling */

 admin_owner_node_free:
	if (ao_node != NULL) {
		free(ao_node);
		ao_node=NULL;
	}

 ao_node_alloc_fail:
	/* Do Nothing */

 bad_handle:
	if (locked)
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);

 lock_fail:
	if (out_evt)
		free(out_evt);
	TRACE_1("Returning with FAILURE:%u", rc);
	TRACE_LEAVE();
	return rc;
}

/*******************************************************************
 * imma_newCcbId internal function
 *
 * NOTE: The CB must be LOCKED on entry of this function!!
 *       It will normally be locked on exit, as reflected in the 'locked' 
 *       in/out parameter. If SA_AIS_OK is returned, then ccb_node
 *       is still valid and locked is true.
 *       The ccb_node must be in state mApplied == true, that is it must
 *       either be a pristine ccb_node or an old ccb_node where the previous
 *       CCB was successfully applied. If the previous CCB was aborted
 *       by the system (ERR_FAILED_OPERATION), then ccb-chaining is no 
 *       longer possible with the same ccb_node/ccbHandle. 
 *******************************************************************/
static SaAisErrorT imma_newCcbId(IMMA_CB *cb, IMMA_CCB_NODE *ccb_node,
	SaUint32T adminOwnerId, bool *locked, SaUint32T timeout)
{
	SaAisErrorT rc = SA_AIS_OK;
	SaUint32T proc_rc = NCSCC_RC_SUCCESS;
	IMMSV_EVT evt;
	IMMSV_EVT *out_evt = NULL;
	IMMA_CCB_NODE *old_ccb_node = NULL;
	SaImmHandleT immHandle = ccb_node->mImmHandle;
	SaImmCcbHandleT ccbHandle = ccb_node->ccb_hdl;
	SaUint32T ccbId = 0;

	TRACE_ENTER();
	TRACE("imma_newCcbId:create new ccb id with admoId:%u",	adminOwnerId);

	osafassert(locked && *locked);

	if (ccb_node->mAborted) {
		rc = SA_AIS_ERR_FAILED_OPERATION;
		goto fail;
	}

	if (ccb_node->mAugCcb) {
		rc = SA_AIS_ERR_BAD_OPERATION;
		goto fail;
	}

	osafassert(ccb_node->mApplied);
	ccb_node->mCcbId = 0;

	/* Guard against race with ourselves. */
	if (ccb_node->mExclusive) {
		rc = SA_AIS_ERR_TRY_AGAIN;
		goto fail;
	}
	ccb_node->mExclusive = true; 

	/* Populate & Send the Open Event to IMMND */
	memset(&evt, 0, sizeof(IMMSV_EVT));
	evt.type = IMMSV_EVT_TYPE_IMMND;
	evt.info.immnd.type = IMMND_EVT_A2ND_CCBINIT;
	evt.info.immnd.info.ccbinitReq.client_hdl = immHandle;
	evt.info.immnd.info.ccbinitReq.adminOwnerId = adminOwnerId;
	evt.info.immnd.info.ccbinitReq.ccbFlags = ccb_node->mCcbFlags;

	TRACE("Sending request for new ccbid with admin OwnerId:%u",
		adminOwnerId);

	/* Unlock before MDS Send */
	m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
	*locked = false;
	old_ccb_node = ccb_node;
	ccb_node = NULL; /* avoid unsafe use */

	/* IMMND GOES DOWN */
	if (cb->is_immnd_up == false) {
		rc = SA_AIS_ERR_NO_RESOURCES;
		TRACE_3("ERR_NO_RESOURCES: IMMND is DOWN");
		goto mds_send_fail;
	}

	/* Send the evt to IMMND */
	proc_rc = imma_mds_msg_sync_send(cb->imma_mds_hdl, &(cb->immnd_mds_dest), &evt, &out_evt, timeout);
	/* Generate rc from proc_rc */
	switch (proc_rc) {
		case NCSCC_RC_SUCCESS:
			break;
		case NCSCC_RC_REQ_TIMOUT:
			rc = imma_proc_check_stale(cb, immHandle, SA_AIS_ERR_TIMEOUT);
			goto mds_send_fail;
		default:
			rc = SA_AIS_ERR_LIBRARY;
			TRACE_4("ERR_LIBRARY: MDS returned unexpected error code %u", proc_rc);
			goto mds_send_fail;
	}

	osafassert(out_evt);
	/* Process the received Event */
	osafassert(out_evt->type == IMMSV_EVT_TYPE_IMMA);
	osafassert(out_evt->info.imma.type == IMMA_EVT_ND2A_CCBINIT_RSP);
	rc = out_evt->info.imma.info.ccbInitRsp.error;
	ccbId = out_evt->info.imma.info.ccbInitRsp.ccbId;
	/* Free the out event */
	free(out_evt);
	out_evt = NULL;

 mds_send_fail:
	/* Take the CB lock */
	if (m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
		rc = SA_AIS_ERR_LIBRARY;
		TRACE_4("ERR_LIBRARY: Lock failed");
		if(out_evt) {free(out_evt); out_evt=NULL;}
		goto fail;
	}
	*locked = true;

	imma_ccb_node_get(&cb->ccb_tree, &ccbHandle, &ccb_node);
	if (!ccb_node) {
		TRACE_4("ERR_LIBRARY: Ccb node gone, ccb_node->mExclusive not respected by "
			"another thread ?");
		rc = SA_AIS_ERR_LIBRARY;
		goto fail;
	}

	/* It should not be possible that the ccb_node was replaced. */
	osafassert(ccb_node == old_ccb_node);

	ccb_node->mExclusive = false;

	if (rc == SA_AIS_OK) {
		ccb_node->mApplied = false;
		ccb_node->mCcbId = ccbId;
		TRACE("CcbId:%u admin ownerId:%u\n", ccb_node->mCcbId, adminOwnerId);
	}

 fail:
	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
  Name          :  saImmOmCcbInitialize
 
  Description   :  Initialize an admin owner handle.
                   This a blocking syncronous call.

                   
  Arguments     :  adminOwnerHandle - The handle for the admin owner to
                                      which the Ccb will be associated.
                   ccbFlags         - Refer to the SA_AIS_IMM specification
                                      for explanation.
                   ccbHandle        - The address of a Ccb handle which
                                      will be initialized. 

  Return Values :  Refer to SAI-AIS specification for various return values.
 
  Notes         : 
******************************************************************************/
SaAisErrorT saImmOmCcbInitialize(SaImmAdminOwnerHandleT adminOwnerHandle,
				 SaImmCcbFlagsT ccbFlags, SaImmCcbHandleT *ccbHandle)
{
	SaAisErrorT rc = SA_AIS_OK;
	SaUint32T proc_rc = NCSCC_RC_SUCCESS;
	IMMA_CB *cb = &imma_cb;
	IMMA_CLIENT_NODE *cl_node = NULL;
	IMMA_ADMIN_OWNER_NODE *ao_node = NULL;
	IMMA_CCB_NODE *ccb_node = NULL;
	bool locked = false;
	SaImmHandleT immHandle=0LL;
	SaUint32T adminOwnerId = 0;
	TRACE_ENTER();

	if (cb->sv_id == 0) {
		TRACE_2("ERR_BAD_HANDLE: No initialized handle exists!");
		return SA_AIS_ERR_BAD_HANDLE;
	}



	if (ccbHandle == NULL) {
		return SA_AIS_ERR_INVALID_PARAM;
	}

	*ccbHandle = 0LL; /* User may have forgotten to initialize to zero */

	if (cb->is_immnd_up == false) {
		TRACE_3("ERR_TRY_AGAIN: IMMND is DOWN");
		return SA_AIS_ERR_TRY_AGAIN;
	}

	/* get the CB Lock */
	if (m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
		rc = SA_AIS_ERR_LIBRARY;
		TRACE_4("ERR_LIBRARY: Lock failed");
		goto done;
	}
	locked = true;

	imma_admin_owner_node_get(&cb->admin_owner_tree, &adminOwnerHandle, &ao_node);
	if (!ao_node) {
		TRACE_2("ERR_BAD_HANDLE: Admin owner handle not valid");
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto done;
	}

	immHandle = ao_node->mImmHandle;
	adminOwnerId = ao_node->mAdminOwnerId;
	/* Dont trust adminOwnerId just yet, resurrect possible. See *** below */
	ao_node=NULL;

	/* Look up client node also to verify that the client handle is still active. */
	imma_client_node_get(&cb->client_tree, &immHandle, &cl_node);
	if (!(cl_node && cl_node->isOm)) {
		rc = SA_AIS_ERR_LIBRARY;
		TRACE_4("ERR_LIBRARY: No client associated with Admin Owner");
		goto done;
	}

	if (cl_node->stale) {
		TRACE_1("IMM Handle %llx is stale", immHandle);
		bool resurrected = imma_om_resurrect(cb, cl_node, &locked);
		cl_node = NULL;

		if (!locked && m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
			TRACE_4("ERR_LIBRARY: LOCK failed");
			rc = SA_AIS_ERR_LIBRARY;
			goto done;
		}
		locked = true;

		imma_client_node_get(&cb->client_tree, &immHandle, &cl_node);

		if (!resurrected || !cl_node || !(cl_node->isOm) || cl_node->stale) {
			TRACE_3("ERR_BAD_HANDLE: Reactive ressurect of handle %llx failed", immHandle);
			if (cl_node && cl_node->stale) {cl_node->exposed = true;}
			rc = SA_AIS_ERR_BAD_HANDLE;
			goto done;
		}

		TRACE_1("Reactive resurrect of handle %llx succeeded", immHandle);

		/* Look up admin owner again since successful resurrect implies
		   new admin-owner-id ! */
		imma_admin_owner_node_get(&cb->admin_owner_tree, &adminOwnerHandle, &ao_node);
		if (!ao_node) {
			TRACE_3("ERR_LIBRARY: Admin owner node dissapeared during resurrect");
			rc = SA_AIS_ERR_LIBRARY;
			/* Very strange case, resurrect succeeded yet admin owner node is gone.  */
			goto done;
		}
		TRACE_1("Admin-owner-id should have changed(?) Before: %u After: %u",
			adminOwnerId, ao_node->mAdminOwnerId);
		adminOwnerId = ao_node->mAdminOwnerId; /* *** */
		ao_node = NULL;
	}

	if (ccbFlags) {
		SaImmCcbFlagsT ccbFlagsTmp = ccbFlags;

		if(ccbFlagsTmp & SA_IMM_CCB_REGISTERED_OI) {
			TRACE("SA_IMM_CCB_REGISTERED_OI is set");
			ccbFlagsTmp &= ~SA_IMM_CCB_REGISTERED_OI;
			if(ccbFlagsTmp & SA_IMM_CCB_ALLOW_NULL_OI) {
				TRACE("SA_IMM_CCB_ALLOW_NULL_OI is set");
				if(!(cl_node->isImmA2b)) {
					TRACE("ERR_VERSION: SA_IMM_CCB_ALLOW_NULL_OI"
						"requires IMM version A.02.11");
					rc = SA_AIS_ERR_VERSION;
					goto done;
				}

				ccbFlagsTmp &= ~SA_IMM_CCB_ALLOW_NULL_OI;
			}
		}

		if(ccbFlagsTmp) {
			TRACE("ERR_INVALID_PARAM: Unknon flags in ccbFlags: 0x%llx", ccbFlags);
			rc = SA_AIS_ERR_INVALID_PARAM;
			goto done;
		}
	}


	/* Allocate the IMMA_CCB_NODE & Populate */

	ccb_node = (IMMA_CCB_NODE *)calloc(1, sizeof(IMMA_CCB_NODE));
	if (ccb_node == NULL) {
		rc = SA_AIS_ERR_NO_MEMORY;
		TRACE_4("ERR_NO_MEMORY: Memory allocation failure");
		goto done;
	}

	ccb_node->ccb_hdl = (SaImmCcbHandleT)m_NCS_GET_TIME_NS;
	/*This is the external handle that the application uses.
	   Internally we use the gloablId provided by the Director. */

	do {
		proc_rc = imma_ccb_node_add(&cb->ccb_tree, ccb_node);

		if (proc_rc != NCSCC_RC_SUCCESS) {
			IMMA_CCB_NODE *ccb_node_tmp = NULL;
			imma_ccb_node_get(&cb->ccb_tree, &(ccb_node->ccb_hdl), &ccb_node_tmp);
			if(!ccb_node_tmp) {
				LOG_NO("Failed to add ccb-node to ccb-tree - aborting");
				abort();
			}
			ccb_node->ccb_hdl++;
			TRACE_4("Duplicate ccb handle %llu (poor clock resolution) adjusting it to %llu",
				ccb_node_tmp->ccb_hdl, ccb_node->ccb_hdl);
		}
	} while (proc_rc != NCSCC_RC_SUCCESS);

	ccb_node->mCcbFlags = ccbFlags;	/*Save flags in client for repeated init */
	ccb_node->mImmHandle = immHandle;
	ccb_node->mAdminOwnerHdl = adminOwnerHandle;
	ccb_node->mApplied = true;

	imma_proc_increment_pending_reply(cl_node);
	rc = imma_newCcbId(cb, ccb_node, adminOwnerId, &locked, cl_node->syncr_timeout);
	cl_node=NULL;
	if(rc == SA_AIS_ERR_LIBRARY) {goto done;}
	/* ccb_node still valid if rc == SA_AIS_OK. */
	if(rc == SA_AIS_OK) {
		osafassert(!(ccb_node->mExclusive));
		osafassert(locked);
	}

	if (!locked) {
		if (m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
			rc = SA_AIS_ERR_LIBRARY;
			/* Loses track of pending reply count, but ERR_LIBRARY dominates */
			TRACE_4("ERR_LIBRARY: Lock failed");
			goto done;
		}
		locked = true;
	}


	imma_client_node_get(&cb->client_tree, &immHandle, &cl_node);
	if (!(cl_node && cl_node->isOm)) {
		rc = SA_AIS_ERR_BAD_HANDLE;
		TRACE_3("ERR_BAD_HANDLE: Client node not found after down call");
		goto remove_ccb_node;
	} 

	imma_proc_decrement_pending_reply(cl_node);

	if (cl_node->stale) {
		if (isExposed(cb, cl_node)) {
			TRACE_3("ERR_BAD_HANDLE: IMM Handle %llx became stale during down-call", 
				immHandle);
			rc = SA_AIS_ERR_BAD_HANDLE;
		} else {
			TRACE_3("ERR_TRY_AGAIN: Handle %llx became stale but possible to resurrect", 
				immHandle);
			/* Can override BAD_HANDLE/TIMEOUT set in check_stale 
			   A retry attempt may succeed in resurrecting immhandle/adminowner
			   and obtain a fresh ccb-id.
			*/
			rc = SA_AIS_ERR_TRY_AGAIN;
		}
	}

 remove_ccb_node:
	if (rc == SA_AIS_OK) {
		*ccbHandle = ccb_node->ccb_hdl;
	} else {
		osafassert(rc != SA_AIS_ERR_LIBRARY) ;
		imma_ccb_node_delete(cb, ccb_node);	/*Remove node from tree and free it. */
		ccb_node = NULL;
	}

 done:
	if (locked)
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);

	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
  Name          :  saImmOmCcbObjectCreate/_2
 
  Description   :  Creates a config object in the IMM.
                   This a blocking syncronous call.

                   
  Arguments     :  ccbHandle - Ccb Handle
                   className - A pointer to the name of the class of the object
                   parentName - A pointer to the name of the parent.
                   attrValues - Initial values for some attributes.

  Return Values :  Refer to SAI-AIS specification for various return values.
******************************************************************************/
SaAisErrorT saImmOmCcbObjectCreate_2(SaImmCcbHandleT ccbHandle,
				     const SaImmClassNameT className,
				     const SaNameT *parentName, const SaImmAttrValuesT_2 **attrValues)
{
	SaAisErrorT rc = SA_AIS_OK;
	IMMA_CB *cb = &imma_cb;
	IMMSV_EVT evt;
	IMMSV_EVT *out_evt = NULL;
	IMMA_ADMIN_OWNER_NODE *ao_node = NULL;
	IMMA_CLIENT_NODE *cl_node = NULL;
	IMMA_CCB_NODE *ccb_node = NULL;
	bool locked = false;
	SaImmHandleT immHandle=0LL;
	SaUint32T adminOwnerId = 0;
	SaStringT *newErrorStrings = NULL;
	TRACE_ENTER();

	if (cb->sv_id == 0) {
		TRACE_2("ERR_BAD_HANDLE: No initialized handle exists!");
		return SA_AIS_ERR_BAD_HANDLE;
	}

	if (className == NULL) {
		TRACE_2("ERR_INVALID_PARAM: classname is NULL");
		TRACE_LEAVE();
		return SA_AIS_ERR_INVALID_PARAM;
	}

	if (attrValues == NULL) {
		TRACE_2("ERR_INVALID_PARAM: attrValues is NULL");
		TRACE_LEAVE();
		return SA_AIS_ERR_INVALID_PARAM;
	}

	if (cb->is_immnd_up == false) {
		TRACE_3("ERR_TRY_AGAIN: IMMND is DOWN");
		/* ##1## Any on going ccb has been aborted and this will be detected
		   in resurrect processing. But this could be the first operation
		   attempted for a new ccb-id, i.e. there is no on-going ccb-id.
		   In that case resurrection may succeed.
		   (A ccb-handle is associated with a chain of actual ccb-ids). 
		*/
		return SA_AIS_ERR_TRY_AGAIN;
	}

	/* get the CB Lock */
	if (m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
		rc = SA_AIS_ERR_LIBRARY;
		TRACE_4("ERR_LIBRARY: Lock failed");
		goto lock_fail;
	}
	locked = true;

	/* Get the CCB info */
	imma_ccb_node_get(&cb->ccb_tree, &ccbHandle, &ccb_node);
	if (!ccb_node) {
		rc = SA_AIS_ERR_BAD_HANDLE;
		TRACE_2("ERR_BAD_HANDLE: Ccb handle not valid");
		goto done;
	}

	if (ccb_node->mExclusive) {
		rc = SA_AIS_ERR_TRY_AGAIN;
		TRACE_3("ERR_TRY_AGAIN: Ccb-id %u being created or in critical phase, in another thread",
			ccb_node->mCcbId);
		goto done;
	}

	if (ccb_node->mAborted) {
		TRACE_2("ERR_FAILED_OPERATION: CCB %u has already been aborted",
			ccb_node->mCcbId);
		rc = SA_AIS_ERR_FAILED_OPERATION;
		goto done;
	}

	immHandle = ccb_node->mImmHandle;

	/* Free string from previous ccb-op */
	imma_free_errorStrings(ccb_node->mErrorStrings); 
	ccb_node->mErrorStrings = NULL;

	/*Look up client node also, to verify that the client handle
	   is still active. */
	imma_client_node_get(&cb->client_tree, &immHandle, &cl_node);
	if (!(cl_node && cl_node->isOm)) {
		rc = SA_AIS_ERR_LIBRARY;
		TRACE_4("ERR_LIBRARY: SaImmHandleT associated with Ccb is not valid");
		goto done;
	}

	if (cl_node->stale) {
		TRACE_1("IMM Handle %llx is stale", immHandle);

		/* Why do we bother trying resurrect? See ##1## above. */
		
		if(!(ccb_node->mApplied)) {
			TRACE_3("ERR_FAILED_OPERATION: IMMND DOWN discards ccb "
				"in active but non-critical state");
			ccb_node->mAborted = true;
			rc = SA_AIS_ERR_FAILED_OPERATION;
			/* We drop the resurrect task since this ccb is doomed. */
			goto done;
		}

		bool resurrected = imma_om_resurrect(cb, cl_node, &locked);
		cl_node = NULL;
		ccb_node = NULL;

		if (!locked && m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
			TRACE_4("ERR_LIBRARY: LOCK failed");
			rc = SA_AIS_ERR_LIBRARY;
			goto done;
		}
		locked = true;

		imma_client_node_get(&cb->client_tree, &immHandle, &cl_node);

		if (!resurrected || !cl_node || !(cl_node->isOm) || cl_node->stale) {
			TRACE_3("ERR_BAD_HANDLE: Reactive ressurect of handle %llx failed", immHandle);
			if (cl_node && cl_node->stale) {cl_node->exposed = true;}
			rc = SA_AIS_ERR_BAD_HANDLE;
			goto done;
		}

		TRACE_1("Reactive resurrect of handle %llx succeeded", immHandle);

		/* Look up ccb_node again */
		imma_ccb_node_get(&cb->ccb_tree, &ccbHandle, &ccb_node);
		if (!ccb_node) {
			rc = SA_AIS_ERR_BAD_HANDLE;
			TRACE_3("ERR_BAD_HANDLE: Ccb handle not valid after successful resurrect - "
					"ccb closed in some other thread?");
			goto done;
		}

		if (ccb_node->mExclusive) {
			rc = SA_AIS_ERR_TRY_AGAIN;
			TRACE_3("ERR_TRY_AGAIN: Ccb-id %u being created or in critical phase, in another thread",
				ccb_node->mCcbId);
			goto done;
		}

		if (ccb_node->mAborted) {
			TRACE_3("ERR_FAILED_OPERATION: Ccb-id %u aborted in another thread?",
				ccb_node->mCcbId);
			rc = SA_AIS_ERR_FAILED_OPERATION;
			goto done;
		}
	}

	/* Get the Admin Owner info  */
	imma_admin_owner_node_get(&cb->admin_owner_tree, &(ccb_node->mAdminOwnerHdl), &ao_node);
	if (!ao_node) {
		rc = SA_AIS_ERR_LIBRARY;
		TRACE_4("ERR_LIBRARY: No Amin-Owner associated with Ccb");
		goto done;
	}

	osafassert(ccb_node->mImmHandle == ao_node->mImmHandle);
	adminOwnerId = ao_node->mAdminOwnerId;
	ao_node=NULL;

	if (ccb_node->mApplied) { /* Current ccb-id is closed, get a new one.*/
		imma_proc_increment_pending_reply(cl_node);
		rc = imma_newCcbId(cb, ccb_node, adminOwnerId, &locked, cl_node->syncr_timeout);
		cl_node = NULL;
		if(rc == SA_AIS_ERR_LIBRARY) {goto done;}
		/* ccb_node still valid if rc == SA_AIS_OK. */
		if(rc == SA_AIS_OK) {
			osafassert(!(ccb_node->mExclusive));
			osafassert(locked);
		}

		if (!locked) {
			if (m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
				rc = SA_AIS_ERR_LIBRARY;
				TRACE_4("ERR_LIBRARY: Lock failed");
				goto done;
			}
			locked = true;
		}

		imma_client_node_get(&cb->client_tree, &immHandle, &cl_node);
		if (!(cl_node && cl_node->isOm)) {
			rc = SA_AIS_ERR_LIBRARY;
			TRACE_4("ERR_LIBRARY: No client associated with Admin Owner");
			goto done;
		}

		imma_proc_decrement_pending_reply(cl_node);

		if (rc != SA_AIS_OK) {
			goto done;
		}

		/* successfully obtained new ccb-id */

		if (cl_node->stale) {
			/* Became stale AFTER we successfully obtained new ccb-id ! */
			TRACE_3("ERR_FAILED_OPERATION: IMM Handle %llx became stale", immHandle);
			
			rc = SA_AIS_ERR_FAILED_OPERATION;
			/* The above is safe because we know the ccb WAS terminated*/
			ccb_node->mCcbId = 0;
			ccb_node->mAborted = true;
			goto done;
		}
	}

	osafassert(locked);
	osafassert(cl_node);
	osafassert(ccb_node);

	imma_proc_increment_pending_reply(cl_node);

	/* Populate the Object-Create event */
	memset(&evt, 0, sizeof(IMMSV_EVT));
	evt.type = IMMSV_EVT_TYPE_IMMND;
	evt.info.immnd.type = IMMND_EVT_A2ND_OBJ_CREATE;

	evt.info.immnd.info.objCreate.adminOwnerId = adminOwnerId;
	evt.info.immnd.info.objCreate.ccbId = ccb_node->mCcbId;

	evt.info.immnd.info.objCreate.className.size = strlen(className) + 1;

	/*alloc-1 */
	evt.info.immnd.info.objCreate.className.buf = malloc(evt.info.immnd.info.objCreate.className.size);
	if (evt.info.immnd.info.objCreate.className.buf == NULL) {
		rc = SA_AIS_ERR_NO_MEMORY;
		goto mds_send_fail;
	}
	strncpy(evt.info.immnd.info.objCreate.className.buf, className, evt.info.immnd.info.objCreate.className.size);

	if (parentName && parentName->length) {
		evt.info.immnd.info.objCreate.parentName.size = strlen((char *)parentName->value) + 1;

		if (parentName->length + 1 < evt.info.immnd.info.objCreate.parentName.size) {
			evt.info.immnd.info.objCreate.parentName.size = parentName->length + 1;
		}

		if (evt.info.immnd.info.objCreate.parentName.size > SA_MAX_NAME_LENGTH) {
			rc = SA_AIS_ERR_INVALID_PARAM;
			TRACE_2("ERR_INVALID_PARAM: Parent name too long for SaNameT: %u",
				evt.info.immnd.info.objCreate.parentName.size);
			goto mds_send_fail;

		}

		/*alloc-2 */
		evt.info.immnd.info.objCreate.parentName.buf = malloc(evt.info.immnd.info.objCreate.parentName.size);
		if (evt.info.immnd.info.objCreate.parentName.buf == NULL) {
			rc = SA_AIS_ERR_NO_MEMORY;
			goto mds_send_fail;
		}
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
			if (immOmIsLoader) {
				/*I am loader => will allow the classname attribute to be defaulted */
				continue;
			}
			/*Non loaders are not allowed to explicitly assign className attribute */
			rc = SA_AIS_ERR_INVALID_PARAM;
			TRACE_2("ERR_INVALID_PARAM: Not allowed to set attribute %s ", sysaClName);
			goto mds_send_fail;
		} else if (strcmp(attr->attrName, sysaAdmName) == 0) {
			if (immOmIsLoader) {
				/*Loader => clear admName attribute, not others */
				/*This is controversial! The standard is not explicit on this. */
				/* Removing curent admin owner name allows the imm to set IMMLOADER */
				continue;
			}
			rc = SA_AIS_ERR_INVALID_PARAM;
			TRACE_2("ERR_INVALID_PARAM: Not allowed to set attribute %s", sysaAdmName);
			goto mds_send_fail;
		} else if ((strcmp(attr->attrName, sysaImplName) == 0) && (!immOmIsLoader)) {
			/*Loader allowed to explicitly assign implName attribute, not others
			   The only point of allowing this is that a class/object implementer 
			   set with identical implementer name may be faster after cluster
			   restart because ImmAttrValue::setValueC_st checks for equality
			   before overwrite.
			 */

			rc = SA_AIS_ERR_INVALID_PARAM;
			TRACE_2("ERR_INVALID_PARAM: Not allowed to set attribute %s", sysaImplName);
			goto mds_send_fail;
		} else if (attr->attrValuesNumber == 0) {
			TRACE("CcbObjectCreate ignoring attribute %s with no values", attr->attrName);
			continue;
		}

		/*alloc-3 */
		p = calloc(1, sizeof(IMMSV_ATTR_VALUES_LIST));

		p->n.attrName.size = strlen(attr->attrName) + 1;
		if (p->n.attrName.size >= SA_MAX_NAME_LENGTH) {
			TRACE_2("ERR_INVALID_PARAM: Attribute name too long");
			rc = SA_AIS_ERR_INVALID_PARAM;
			free(p);
			p=NULL;
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

	rc = imma_evt_fake_evs(cb, &evt, &out_evt, cl_node->syncr_timeout, cl_node->handle, &locked, false);
	cl_node = NULL;
	ccb_node = NULL;

	TRACE("objectCreate send RETURNED:%u", rc);

	if (out_evt) {
		/* Process the outcome, note this is after a blocking call. */
		osafassert(out_evt->type == IMMSV_EVT_TYPE_IMMA);
		osafassert((out_evt->info.imma.type == IMMA_EVT_ND2A_IMM_ERROR) ||
			(out_evt->info.imma.type == IMMA_EVT_ND2A_IMM_ERROR_2));
		if (rc == SA_AIS_OK) {
			rc = out_evt->info.imma.info.errRsp.error;
			if(out_evt->info.imma.type == IMMA_EVT_ND2A_IMM_ERROR_2) {
				newErrorStrings = 
					imma_getErrorStrings(&(out_evt->info.imma.info.errRsp));
			}

		}
		/* Free the out event */
		/*Will free the root event only, not any pointer structures. */
		free(out_evt);
		out_evt = NULL;
	}

 mds_send_fail:
	/*We may be un-locked here but this should not matter. */

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
		p=NULL;
	}

	if (!locked && m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
		TRACE_4("ERR_LIBRARY: Lock failed");
		rc = SA_AIS_ERR_LIBRARY;
		goto lock_fail;
	}
	locked = true;

	imma_client_node_get(&cb->client_tree, &immHandle, &cl_node);
	if (!(cl_node && cl_node->isOm)) {
		if (rc == SA_AIS_OK) {
			TRACE_3("ERR_BAD_HANDLE: client_node gone on return from down-call");
			rc = SA_AIS_ERR_BAD_HANDLE;
		}
		goto done;
	}

	imma_proc_decrement_pending_reply(cl_node);

	imma_ccb_node_get(&cb->ccb_tree, &ccbHandle, &ccb_node);
	if (!ccb_node) {	
		TRACE_3("ERR_BAD_HANDLE: ccb-node gone on return from down call");
		/* BAD_HANDLE overrides any other return code already assigned. */
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto done;
	}

	osafassert(ccb_node->mErrorStrings == NULL);
	ccb_node->mErrorStrings = newErrorStrings;
	newErrorStrings = NULL; /* Dont free the strings on exit from this func */

	if (rc == SA_AIS_OK) {
		if (cl_node->stale) { 
			/* Became stale during the blocked call yet the call
			   succeeded! This should be very rare.
			   We know the ccb is aborted. So we dont need to expose
			   the stale handle.
			*/

			TRACE_3("ERR_FAILED_OPERATION: Handle %llx became stale "
				"during the down-call",  immHandle);
			ccb_node->mAborted = true;
			rc = SA_AIS_ERR_FAILED_OPERATION;
		}

		if(ccb_node->mAugCcb) {
			/* This is an OI augmented ccb. 
			   Successfull object create added by OI to root CCB =>
			   this aug-ccb must be applied (confirmed by the OI)
			   or the imm will interpret the missing confirmation
			   as abort and abort the entire root ccb.
			 */
			ccb_node->mAugIsTainted = true;
		}
	}
	else if (rc == SA_AIS_ERR_TRY_AGAIN && (cb->is_immnd_up == false)) {
		/* 
		   We lost contact with the IMMND which means we KNOW the current
		   ccb-id associated with this ccb-handle was aborted in non-critical
		   phase. Better then to handle this as FAILED_OPERATION, since this
		   can avoid exposing the immHandle as BAD_HANDLE.
		 */
		TRACE_3("ERR_FAILED_OPERATION: Converting TRY_AGAIN to "
			"FAILED_OPERATION in ccbObjectCreate in IMMA");
		rc = SA_AIS_ERR_FAILED_OPERATION;
		ccb_node->mAborted = true;
	} else if (rc == SA_AIS_ERR_FAILED_OPERATION) {
		ccb_node->mAborted = true;
	}


 done:
	imma_free_errorStrings(newErrorStrings); /* In case of failed resurrect only */

	if (locked)
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);

 lock_fail:

	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
  Name          :  saImmOmCcbObjectModify/_2
 
  Description   :  Modifies a config object in the IMM.
                   This a blocking syncronous call.

                   
  Arguments     :  ccbHandle - Ccb Handle
                   objectName - A pointer to the name of the object.
                   attrMods - The attribute modifications (see the standard)

  Return Values :  Refer to SAI-AIS specification for various return values.
******************************************************************************/
SaAisErrorT saImmOmCcbObjectModify_2(SaImmCcbHandleT ccbHandle,
				     const SaNameT *objectName, const SaImmAttrModificationT_2 **attrMods)
{
	SaAisErrorT rc = SA_AIS_OK;
	IMMA_CB *cb = &imma_cb;
	IMMSV_EVT evt;
	IMMSV_EVT *out_evt = NULL;
	IMMA_ADMIN_OWNER_NODE *ao_node = NULL;
	IMMA_CLIENT_NODE *cl_node = NULL;
	IMMA_CCB_NODE *ccb_node = NULL;
	bool locked = false;
	SaImmHandleT immHandle=0LL;
	SaUint32T adminOwnerId = 0;
	SaStringT *newErrorStrings = NULL;
	TRACE_ENTER();

	if (cb->sv_id == 0) {
		TRACE_2("ERR_BAD_HANDLE: No initialized handle exists!");
		return SA_AIS_ERR_BAD_HANDLE;
	}

	if (objectName == NULL) {
		TRACE_2("ERR_INVALID_PARAM: objectName is NULL");
		TRACE_LEAVE();
		return SA_AIS_ERR_INVALID_PARAM;
	}

	if (attrMods == NULL) {
		TRACE_2("ERR_INVALID_PARAM: attrMods is NULL");
		TRACE_LEAVE();
		return SA_AIS_ERR_INVALID_PARAM;
	}

	if (cb->is_immnd_up == false) {
		TRACE_3("ERR_TRY_AGAIN: IMMND is DOWN");
		/* ##1## Any on going ccb has been aborted and this will be detected
		   in resurrect processing. But this could be the first operation
		   attempted for a new ccb-id, i.e. there is no on-going ccb-id,
		   (remember that a ccb-handle is associated with a chain of actual
		   ccb-ids). In that case resurrection may succeed.
		*/
		return SA_AIS_ERR_TRY_AGAIN;
	}

	/* get the CB Lock */
	if (m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
		rc = SA_AIS_ERR_LIBRARY;
		TRACE_4("ERR_LIBRARY: Lock failed");
		goto lock_fail;
	}
	locked = true;

	/* Get the CCB info */
	imma_ccb_node_get(&cb->ccb_tree, &ccbHandle, &ccb_node);
	if (!ccb_node) {
		rc = SA_AIS_ERR_BAD_HANDLE;
		TRACE_2("ERR_BAD_HANDLE: Ccb handle not valid");
		goto done;
	}

	if (ccb_node->mExclusive) {
		rc = SA_AIS_ERR_TRY_AGAIN;
		TRACE_3("ERR_TRY_AGAIN: Ccb-id %u being created or in critical phase, in another thread",
			ccb_node->mCcbId);
		goto done;
	}

	if (ccb_node->mAborted) {
		TRACE_2("ERR_FAILED_OPERATION: CCB %u has already been aborted",
			ccb_node->mCcbId);
		rc = SA_AIS_ERR_FAILED_OPERATION;
		goto done;
	}

	immHandle = ccb_node->mImmHandle;

	/* Free string from previous ccb-op */
	imma_free_errorStrings(ccb_node->mErrorStrings);
	ccb_node->mErrorStrings = NULL;

	/*Look up client node also, to verify that the client handle
	   is still active. */
	imma_client_node_get(&cb->client_tree, &immHandle, &cl_node);
	if (!(cl_node && cl_node->isOm)) {
		rc = SA_AIS_ERR_LIBRARY;
		TRACE_4("ERR_LIBRARY: No valid SaImmHandleT associated with Ccb");
		goto done;
	}

	if (cl_node->stale) {
		TRACE_1("IMM Handle %llx is stale", immHandle);

		if(!(ccb_node->mApplied)) {
			TRACE_3("ERR_FAILED_OPERATION: IMMND DOWN discards ccb "
				"in active but non-critical state");
			ccb_node->mAborted = true;
			rc = SA_AIS_ERR_FAILED_OPERATION;
			/* We drop the resurrect task since this ccb is doomed. */
			goto done;
		}

		/* Why do we bother trying resurrect? See ##1## above. */

		bool resurrected = imma_om_resurrect(cb, cl_node, &locked);
		cl_node = NULL;
		ccb_node =NULL;

		if (!locked && m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
			TRACE_4("ERR_LIBRARY: Lock failed");
			rc = SA_AIS_ERR_LIBRARY;
			goto done;
		}
		locked = true;

		imma_client_node_get(&cb->client_tree, &immHandle, &cl_node);

		if (!resurrected || !cl_node || !(cl_node->isOm) || cl_node->stale) {
			TRACE_3("ERR_BAD_HANDLE: Reactive ressurect of handle %llx failed", immHandle);
			if (cl_node && cl_node->stale) {cl_node->exposed = true;}
			rc = SA_AIS_ERR_BAD_HANDLE;
			goto done;
		}

		TRACE_1("Reactive resurrect of handle %llx succeeded", immHandle);

		/* Look up ccb_node again */
		imma_ccb_node_get(&cb->ccb_tree, &ccbHandle, &ccb_node);
		if (!ccb_node) {
			rc = SA_AIS_ERR_BAD_HANDLE;
			TRACE_3("ERR_BAD_HANDLE: Ccb handle not valid after successful resurrect");
			goto done;
		}

		if (ccb_node->mExclusive) {
			rc = SA_AIS_ERR_TRY_AGAIN;
			TRACE_3("ERR_TRY_AGAIN: Ccb-id %u being created or in critical phase, "
				"in another thread", ccb_node->mCcbId);
			goto done;
		}

		if (ccb_node->mAborted) {
			TRACE_3("ERR_FAILED_OPERATION: Ccb-id %u was aborted", ccb_node->mCcbId);
			rc = SA_AIS_ERR_FAILED_OPERATION;
			goto done;
		}
	}

	/* Get the Admin Owner info  */
	imma_admin_owner_node_get(&cb->admin_owner_tree, &(ccb_node->mAdminOwnerHdl), &ao_node);
	if (!ao_node) {
		rc = SA_AIS_ERR_LIBRARY;
		TRACE_4("ERR_LIBRARY: No Amin-Owner associated with Ccb");
		goto done;
	}

	osafassert(ccb_node->mImmHandle == ao_node->mImmHandle);
	adminOwnerId = ao_node->mAdminOwnerId;
	ao_node=NULL;

	if (ccb_node->mApplied) {  /* Current ccb-id is closed, get a new one.*/
		imma_proc_increment_pending_reply(cl_node);
		rc = imma_newCcbId(cb, ccb_node, adminOwnerId, &locked, cl_node->syncr_timeout);
		cl_node = NULL;
		if(rc == SA_AIS_ERR_LIBRARY) {goto done;}
		/* ccb_node still valid if rc == SA_AIS_OK. */
		if(rc == SA_AIS_OK) {
			osafassert(!(ccb_node->mExclusive));
			osafassert(locked);
		}

		if (!locked) {
			if (m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
				rc = SA_AIS_ERR_LIBRARY;
				TRACE_4("ERR_LIBRARY: Lock failed");
				goto done;
			}
			locked = true;
		}

		imma_client_node_get(&cb->client_tree, &immHandle, &cl_node);
		if (!(cl_node && cl_node->isOm)) {
			rc = SA_AIS_ERR_LIBRARY;
			TRACE_4("ERR_LIBRARY: No client associated with Admin Owner");
			goto done;
		}

		imma_proc_decrement_pending_reply(cl_node);

		if (rc != SA_AIS_OK) {
			goto done;
		}

		/* successfully obtained new ccb-id */

		if (cl_node->stale) {
			/* Became stale AFTER we successfully obtained new ccb-id ! */
			TRACE_3("ERR_FAILED_OPERATION: IMM Handle %llx became stale", immHandle);

			rc = SA_AIS_ERR_FAILED_OPERATION;
			/* We know the ccb WAS terminated*/
			ccb_node->mCcbId = 0;
			ccb_node->mAborted = true;
			goto done;
		}
	}

	osafassert(locked);
	osafassert(cl_node);
	osafassert(ccb_node);

	imma_proc_increment_pending_reply(cl_node);

	/* Populate the Object-Modify event */
	memset(&evt, 0, sizeof(IMMSV_EVT));
	evt.type = IMMSV_EVT_TYPE_IMMND;
	evt.info.immnd.type = IMMND_EVT_A2ND_OBJ_MODIFY;

	evt.info.immnd.info.objModify.adminOwnerId = adminOwnerId;
	evt.info.immnd.info.objModify.ccbId = ccb_node->mCcbId;

	if (objectName->length) {
		evt.info.immnd.info.objModify.objectName.size = strlen((char *)objectName->value) + 1;

		if (objectName->length + 1 < evt.info.immnd.info.objModify.objectName.size) {
			evt.info.immnd.info.objModify.objectName.size = objectName->length + 1;
		}

		/*alloc-1 */
		evt.info.immnd.info.objModify.objectName.buf = malloc(evt.info.immnd.info.objModify.objectName.size);
		if (evt.info.immnd.info.objModify.objectName.buf == NULL) {
			rc = SA_AIS_ERR_NO_MEMORY;
			goto mds_send_fail;
		}
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

		/*NOTE: Check that user does not set values for System attributes. */

		IMMSV_ATTR_MODS_LIST *p = evt.info.immnd.info.objModify.attrMods;
		while(p!=NULL) {
			if(strcmp(attrMod->modAttr.attrName,  p->attrValue.attrName.buf) == 0) {
				rc = SA_AIS_ERR_INVALID_PARAM;
				TRACE_2("ERR_INVALID_PARAM: Attribute %s occurs multiple times "
					"in attrMods parameter", attrMod->modAttr.attrName);
				goto mds_send_fail;
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
		}
		/*At least one value */
		p->next = evt.info.immnd.info.objModify.attrMods;	/*NULL initially. */
		evt.info.immnd.info.objModify.attrMods = p;
	}

	rc = imma_evt_fake_evs(cb, &evt, &out_evt, cl_node->syncr_timeout, cl_node->handle, &locked, false);
	cl_node = NULL;
	ccb_node = NULL;

	TRACE("objectModify send RETURNED:%u", rc);

	if (out_evt) {
		/* Process the outcome, note this is after a blocking call. */
		osafassert(out_evt->type == IMMSV_EVT_TYPE_IMMA);
		osafassert((out_evt->info.imma.type == IMMA_EVT_ND2A_IMM_ERROR) ||
			(out_evt->info.imma.type == IMMA_EVT_ND2A_IMM_ERROR_2));
		if (rc == SA_AIS_OK) {
			rc = out_evt->info.imma.info.errRsp.error;
			if(out_evt->info.imma.type == IMMA_EVT_ND2A_IMM_ERROR_2) {
				newErrorStrings = 
					imma_getErrorStrings(&(out_evt->info.imma.info.errRsp));
			}
		}
		free(out_evt);
		out_evt = NULL;
	}

 mds_send_fail:
	/*We may be un-locked here but this should not matter. */

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

	if (!locked && m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
		TRACE_4("ERR_LIBRARY: Lock failed");
		rc = SA_AIS_ERR_LIBRARY;
		goto lock_fail;
	}
	locked = true;

	imma_client_node_get(&cb->client_tree, &immHandle, &cl_node);
	if (!(cl_node && cl_node->isOm)) {
		if (rc == SA_AIS_OK) {
			TRACE_3("ERR_BAD_HANDLE: client_node gone on return from down-call");
			rc = SA_AIS_ERR_BAD_HANDLE;
		}
		goto done;
	}

	imma_proc_decrement_pending_reply(cl_node);

	imma_ccb_node_get(&cb->ccb_tree, &ccbHandle, &ccb_node);
	if (!ccb_node) {	
		TRACE_3("ERR_BAD_HANDLE: ccb-node gone on return from down-call");
		/* BAD_HANDLE overrides any other return code already assigned. */
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto done;
	}

	osafassert(ccb_node->mErrorStrings == NULL);
	ccb_node->mErrorStrings = newErrorStrings;
	newErrorStrings = NULL; /* Dont free the strings on exit from this func */

	if (rc == SA_AIS_OK) {
		if (cl_node->stale) {
			/* Became stale during the blocked call yet the call
			   succeeded! This should be very rare.
			   We know the ccb is aborted. So we dont need to expose
			   the stale handle.
			*/

			TRACE_3("ERR_FAILED_OPERATION: Handle %llx became stale "
				"during the down-call",  immHandle);
			ccb_node->mAborted = true;
			rc = SA_AIS_ERR_FAILED_OPERATION;
		}

		if(ccb_node->mAugCcb) {
			/* This is an OI augmented ccb. 
			   Successfull object modify added by OI to root CCB =>
			   this aug-ccb must be applied (confirmed by the OI)
			   or the imm will interpret the missing confirmation
			   as abort and abort the entire root ccb.
			 */
			ccb_node->mAugIsTainted = true;
		}
	}
	else if (rc == SA_AIS_ERR_TRY_AGAIN && (cb->is_immnd_up == false)) {
		/* 
		   We lost contact with the IMMND which means we KNOW the current
		   ccb-id associated with this ccb-handle was aborted in non-critical
		   phase. Better then to handle this as FAILED_OPERATION, since this
		   can avoid exposing the immHandle as BAD_HANDLE.
		 */
		TRACE_3("ERR_FAILED_OPERATION: Converting TRY_AGAIN to "
			"FAILED_OPERATION in ccbObjectModify in IMMA");
		rc = SA_AIS_ERR_FAILED_OPERATION;
		ccb_node->mAborted = true;
	} else if (rc == SA_AIS_ERR_FAILED_OPERATION) {
		ccb_node->mAborted = true;
	}

 done:
	imma_free_errorStrings(newErrorStrings); /* In case of failed resurrect only */

	if (locked)
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);

 lock_fail:

	TRACE("objectModify really RETURNING:%u", rc);
	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
  Name          :  saImmOmCcbObjectDelete
 
  Description   :  Deletes a config object in the IMM.
                   This a blocking syncronous call.

                   
  Arguments     :  ccbHandle - Ccb Handle
                   objectName - A pointer to the name of the object where
                                delete starts from.

  Return Values :  Refer to SAI-AIS specification for various return values.
******************************************************************************/
SaAisErrorT saImmOmCcbObjectDelete(SaImmCcbHandleT ccbHandle, const SaNameT *objectName)
{
	SaAisErrorT rc = SA_AIS_OK;
	IMMA_CB *cb = &imma_cb;
	IMMSV_EVT evt;
	IMMSV_EVT *out_evt = NULL;
	IMMA_ADMIN_OWNER_NODE *ao_node = NULL;
	IMMA_CLIENT_NODE *cl_node = NULL;
	IMMA_CCB_NODE *ccb_node = NULL;
	bool locked = false;
	SaImmHandleT immHandle=0LL;
	SaUint32T adminOwnerId = 0;
	SaStringT *newErrorStrings = NULL;
	TRACE_ENTER();

	if (cb->sv_id == 0) {
		TRACE_2("ERR_BAD_HANDLE: No initialized handle exists!");
		return SA_AIS_ERR_BAD_HANDLE;
	}

	if (!objectName || (objectName->length == 0)) {
		TRACE_2("ERR_INVALID_PARAM: Empty object-name");
		return SA_AIS_ERR_INVALID_PARAM;
	}

	if (cb->is_immnd_up == false) {
		TRACE_3("ERR_TRY_AGAIN: IMMND is DOWN");
		/* ##1## Any on going ccb has been aborted and this will be detected
		   in resurrect processing. But this could be the first operation
		   attempted for a new ccb-id, i.e. there is no on-going ccb-id,
		   (remember that a ccb-handle is associated with a chain of actual
		   ccbs). In that case resurrection may succeed.
		*/
		return SA_AIS_ERR_TRY_AGAIN;
	}

	/* get the CB Lock */
	if (m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
		rc = SA_AIS_ERR_LIBRARY;
		TRACE_4("ERR_LIBRARY: Lock failed");
		goto lock_fail;
	}
	locked = true;

	/*Get the CCB info */
	imma_ccb_node_get(&cb->ccb_tree, &ccbHandle, &ccb_node);
	if (!ccb_node) {
		rc = SA_AIS_ERR_BAD_HANDLE;
		TRACE_2("ERR_BAD_HANDLE: Ccb handle not valid");
		goto done;
	}

	if (ccb_node->mExclusive) {
		rc = SA_AIS_ERR_TRY_AGAIN;
		TRACE_3("ERR_TRY_AGAIN: Ccb-id %u being created or in critical phase, in another thread",
			ccb_node->mCcbId);
		goto done;
	}

	if (ccb_node->mAborted) {
		TRACE_2("ERR_FAILED_OPERATION: CCB %u already aborted", 
			ccb_node->mCcbId);
		rc = SA_AIS_ERR_FAILED_OPERATION;
		goto done;
	}

	immHandle = ccb_node->mImmHandle;

	/* Free string from previous ccb-op */
	imma_free_errorStrings(ccb_node->mErrorStrings); 
	ccb_node->mErrorStrings = NULL;

	/*Look up client node also, to verify that the client handle
	   is still active. */
	imma_client_node_get(&cb->client_tree, &immHandle, &cl_node);
	if (!(cl_node && cl_node->isOm)) {
		rc = SA_AIS_ERR_LIBRARY;
		TRACE_4("ERR_LIBRARY: No valid SaImmHandleT associated with Ccb");
		goto done;
	}

	if (cl_node->stale) {
		TRACE_1("IMM Handle %llx is stale exposed?:%u", immHandle, cl_node->exposed);

		if(!(ccb_node->mApplied)) {
			TRACE_3("ERR_FAILED_OPERATION: IMMND DOWN discards ccb "
				"in active but non-critical state");
			ccb_node->mAborted = true;
			rc = SA_AIS_ERR_FAILED_OPERATION;
			/* We drop the resurrect task since this ccb is doomed. */
			goto done;
		}

		/* Why do we bother trying resurrect? See ##1## above. */

		bool resurrected = imma_om_resurrect(cb, cl_node, &locked);
		cl_node = NULL;
		ccb_node = NULL;

		if (!locked && m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
			TRACE_4("ERR_LIBRARY: Lock failed");
			rc = SA_AIS_ERR_LIBRARY;
			goto done;
		}
		locked = true;

		imma_client_node_get(&cb->client_tree, &immHandle, &cl_node);

		if (!resurrected || !cl_node || !(cl_node->isOm) || cl_node->stale) {
			TRACE_3("ERR_BAD_HANDLE: Reactive ressurect of handle %llx failed", immHandle);
			if (cl_node && cl_node->stale) {cl_node->exposed = true;}
			rc = SA_AIS_ERR_BAD_HANDLE;
			goto done;
		}

		TRACE_1("Reactive resurrect of handle %llx succeeded", immHandle);

		/* Look up ccb_node again */
		imma_ccb_node_get(&cb->ccb_tree, &ccbHandle, &ccb_node);
		if (!ccb_node) {
			rc = SA_AIS_ERR_BAD_HANDLE;
			TRACE_3("ERR_BAD_HANDLE: Ccb handle not valid after successful resurrect");
			goto done;
		}

		if (ccb_node->mExclusive) {
			rc = SA_AIS_ERR_TRY_AGAIN;
			TRACE_3("ERR_TRY_AGAIN: Ccb-id %u being created or in critical phase, "
				"in another thread", ccb_node->mCcbId);
			goto done;
		}

		if (ccb_node->mAborted) {
			rc = SA_AIS_ERR_FAILED_OPERATION;
			TRACE_3("ERR_FAILED_OPERATION: Ccb-id %u was already aborted", ccb_node->mCcbId);
			goto done;
		}
	}

	/*Get the Admin Owner info  */
	imma_admin_owner_node_get(&cb->admin_owner_tree, &(ccb_node->mAdminOwnerHdl), &ao_node);
	if (!ao_node) {
		rc = SA_AIS_ERR_LIBRARY;
		TRACE_4("ERR_LIBRARY: No Amin-Owner associated with Ccb");
		goto done;
	}

	osafassert(ccb_node->mImmHandle == ao_node->mImmHandle);
	adminOwnerId = ao_node->mAdminOwnerId;
	ao_node=NULL;

	if (ccb_node->mApplied) {  /* Current ccb-id is closed, get a new one.*/
		imma_proc_increment_pending_reply(cl_node);
		rc = imma_newCcbId(cb, ccb_node, adminOwnerId, &locked, cl_node->syncr_timeout);
		cl_node = NULL;
		if(rc == SA_AIS_ERR_LIBRARY) {goto done;}
		/* ccb_node still valid if rc == SA_AIS_OK. */
		if(rc == SA_AIS_OK) {
			osafassert(!(ccb_node->mExclusive));
			osafassert(locked);
		}

		if (!locked) {
			if (m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
				rc = SA_AIS_ERR_LIBRARY;
				TRACE_4("ERR_LIBRARY: Lock failed");
				goto done;
			}
			locked = true;
		}

		imma_client_node_get(&cb->client_tree, &immHandle, &cl_node);
		if (!(cl_node && cl_node->isOm)) {
			rc = SA_AIS_ERR_LIBRARY;
			TRACE_4("ERR_LIBRARY: No client associated with Admin Owner");
			goto done;
		}

		imma_proc_decrement_pending_reply(cl_node);

		if (rc != SA_AIS_OK) {
			goto done;
		}

		/* successfully obtained new ccb-id */

		if (cl_node->stale) {
			/* Became stale AFTER we successfully obtained new ccb-id ! */
			TRACE_3("ERR_FAILED_OPERATION: IMM Handle %llx became stale", immHandle);

			rc = SA_AIS_ERR_FAILED_OPERATION;
			/* We know the ccb WAS terminated*/
			ccb_node->mCcbId = 0;
			ccb_node->mAborted = true;
			goto done;
		}
	}

	osafassert(locked);
	osafassert(cl_node);
	osafassert(ccb_node);

	imma_proc_increment_pending_reply(cl_node);

	/* Populate the Object-Delete event */
	memset(&evt, 0, sizeof(IMMSV_EVT));
	evt.type = IMMSV_EVT_TYPE_IMMND;
	evt.info.immnd.type = IMMND_EVT_A2ND_OBJ_DELETE;

	evt.info.immnd.info.objDelete.adminOwnerId = adminOwnerId;
	evt.info.immnd.info.objDelete.ccbId = ccb_node->mCcbId;

	evt.info.immnd.info.objDelete.objectName.size = strlen((char *)objectName->value) + 1;

	if (objectName->length + 1 < evt.info.immnd.info.objDelete.objectName.size) {
		evt.info.immnd.info.objDelete.objectName.size = objectName->length + 1;
	}

	/*alloc-1 */
	evt.info.immnd.info.objDelete.objectName.buf = malloc(evt.info.immnd.info.objDelete.objectName.size);
	if (evt.info.immnd.info.objDelete.objectName.buf == NULL) {
		rc = SA_AIS_ERR_NO_MEMORY;
		goto mds_send_fail;
	}
	strncpy(evt.info.immnd.info.objDelete.objectName.buf,
		(char *)objectName->value, evt.info.immnd.info.objDelete.objectName.size);
	evt.info.immnd.info.objDelete.objectName.buf[evt.info.immnd.info.objDelete.objectName.size - 1] = '\0';

	rc = imma_evt_fake_evs(cb, &evt, &out_evt, cl_node->syncr_timeout, cl_node->handle, &locked, false);
	cl_node = NULL;
	ccb_node = NULL;

	TRACE("objectDelete send RETURNED:%u", rc);

	if (out_evt) {
		/* Process the outcome, note this is after a blocking call. */
		osafassert(out_evt->type == IMMSV_EVT_TYPE_IMMA);
		osafassert((out_evt->info.imma.type == IMMA_EVT_ND2A_IMM_ERROR) ||
			(out_evt->info.imma.type == IMMA_EVT_ND2A_IMM_ERROR_2));
		if (rc == SA_AIS_OK) {
			rc = out_evt->info.imma.info.errRsp.error;
			if(out_evt->info.imma.type == IMMA_EVT_ND2A_IMM_ERROR_2) {
				newErrorStrings = 
					imma_getErrorStrings(&(out_evt->info.imma.info.errRsp));
			}
		}
		free(out_evt);
		out_evt=NULL;
	}

 mds_send_fail:
	/*We may be un-locked here but this should not matter. */

	if (evt.info.immnd.info.objDelete.objectName.buf) {	/*free-1 */
		free(evt.info.immnd.info.objDelete.objectName.buf);
		evt.info.immnd.info.objDelete.objectName.buf = NULL;
	}

	if (!locked && m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
		TRACE_4("ERR_LIBRARY: Lock failed");
		rc = SA_AIS_ERR_LIBRARY;
		goto lock_fail;
	}
	locked = true;

	imma_client_node_get(&cb->client_tree, &immHandle, &cl_node);
	if (!(cl_node && cl_node->isOm)) {
		if (rc == SA_AIS_OK) {
			TRACE_3("ERR_BAD_HANDLE: client_node gone on return from down-call");
			rc = SA_AIS_ERR_BAD_HANDLE;
		}
		goto done;
	}

	imma_proc_decrement_pending_reply(cl_node);

	imma_ccb_node_get(&cb->ccb_tree, &ccbHandle, &ccb_node);
	if (!ccb_node) {	
		TRACE_3("ERR_BAD_HANDLE: ccb-node gone on return from down-call");
		/* BAD_HANDLE overrides any other return code already assigned. */
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto done;
	}

	osafassert(ccb_node->mErrorStrings == NULL);
	ccb_node->mErrorStrings = newErrorStrings;
	newErrorStrings = NULL; /* Dont free the strings on exit from this func */	

	if (rc == SA_AIS_OK) {
		if (cl_node->stale) {
			/* Became stale during the blocked call yet the call
			   succeeded! This should be very rare.
			   We know the ccb is aborted. So we dont need to expose
			   the stale handle.
			*/
			TRACE_3("ERR_FAILED_OPERATION: Handle %llx became stale "
				"during the down-call", immHandle);
			ccb_node->mAborted = true;
			rc = SA_AIS_ERR_FAILED_OPERATION;
		}
		if(ccb_node->mAugCcb) {
			/* This is an OI augmented ccb. 
			   Successfull object delete added by OI to root CCB =>
			   this aug-ccb must be applied (confirmed by the OI)
			   or the imm will interpret the missing confirmation
			   as abort and abort the entire root ccb.
			 */
			ccb_node->mAugIsTainted = true;
		}		
	}
	else if (rc == SA_AIS_ERR_TRY_AGAIN && (cb->is_immnd_up == false)) {
		/* 
		   We lost contact with the IMMND which means we KNOW the current
		   ccb-id associated with this ccb-handle was aborted in non-critical
		   phase. Better then to handle this as FAILED_OPERATION, since this
		   can avoid exposing the immHandle as BAD_HANDLE.
		 */
		TRACE_3("ERR_FAILED_OPERATION: Converting TRY_AGAIN to FAILED_OPERATION "
			"in ccbObjectDelete in IMMA");
		rc = SA_AIS_ERR_FAILED_OPERATION;
		ccb_node->mAborted = true;
	} else if (rc == SA_AIS_ERR_FAILED_OPERATION) {
		ccb_node->mAborted = true;
	}

 done:
	imma_free_errorStrings(newErrorStrings); /* In case of failed resurrect only */

	if (locked)
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);

 lock_fail:
	TRACE("objectDelete really RETURNING:%u", rc);
	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
  Name          :  saImmOmCcbApply
 
  Description   :  Applies (commits) a CCB.
                   This a blocking syncronous call.

                   
  Arguments     :  ccbHandle - Ccb Handle

  Return Values :  Refer to SAI-AIS specification for various return values.
 
******************************************************************************/
SaAisErrorT saImmOmCcbApply(SaImmCcbHandleT ccbHandle)
{
	SaAisErrorT rc = SA_AIS_OK;
	IMMA_CB *cb = &imma_cb;
	IMMSV_EVT evt;
	IMMSV_EVT *out_evt = NULL;
	IMMA_CLIENT_NODE *cl_node = NULL;
	IMMA_CCB_NODE *ccb_node = NULL;
	bool locked = false;
	SaImmHandleT immHandle=0LL;
	SaUint32T ccbId = 0;
	SaStringT *newErrorStrings = NULL;
	TRACE_ENTER();

	if (cb->sv_id == 0) {
		TRACE_2("ERR_BAD_HANDLE :No initialized handle exists!");
		return SA_AIS_ERR_BAD_HANDLE;
	}

	/* No point in checking for cb->is_immnd_up here because there 
	   is no point in giving ERR_RETRY on a CcbApply.
	   An apply would not be the first op for a ccb-id.
	 */

	/* get the CB Lock */
	if (m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
		rc = SA_AIS_ERR_LIBRARY;
		TRACE_4("ERR_LIBRARY: Lock failed");
		goto lock_fail;
	}
	locked = true;

	/*Get the CCB info */
	imma_ccb_node_get(&cb->ccb_tree, &ccbHandle, &ccb_node);
	if (!ccb_node) {
		rc = SA_AIS_ERR_BAD_HANDLE;
		TRACE_2("ERR_BAD_HANDLE: Ccb handle not valid");
		goto done;
	}
	TRACE("CCb node found for ccbhandle %llx ccbid:%u", ccbHandle, ccb_node->mCcbId);

	if (ccb_node->mExclusive) {
		rc = SA_AIS_ERR_TRY_AGAIN;
		TRACE_3("ERR_TRY_AGAIN: Ccb-id %u being created or in critical phase, in another thread",
			ccb_node->mCcbId);
		goto done;
	}

	if (ccb_node->mAborted)  {
		/* mApplied must only be set to true when the current
		   ccb-id has been EXPLICITLY applied by the user. 
		   This can only be done by a successful explicit 
		   saImmOmCcbApply. A ccb-handle with an aborted ccb-id
		   can not be used any more. Only finalize is allowed on handle.

		   Setting mApplied to true opens for the IMPLICIT 
		   start of a new ccb-id with the current and same SaImmCcbHandleT value.
		   Because start of ccb-id can be implicit, we insist that
		   termination of ccb-id is explicit from the user. 
		   The user could otherwise miss seeing the current ccb-id aborted
		   and think the ccb was simply a continuation of the same 
		   (actually aborted) ccb with its operations. 
		*/
		rc = SA_AIS_ERR_FAILED_OPERATION;
		TRACE_2("ERR_FAILED_OPERATION: Ccb %u has already been aborted", 
			ccb_node->mCcbId);
		goto done;
	}

	if (ccb_node->mApplied) {
		ccb_node->mApplied = false;
		ccb_node->mAborted = true;
		rc = SA_AIS_ERR_FAILED_OPERATION;
		TRACE_2("ERR_FAILED_OPERATION: Ccb %u has already been applied",
			ccb_node->mCcbId);
		goto done;
	}

	immHandle = ccb_node->mImmHandle;

	/* Free string from previous ccb-op */
	imma_free_errorStrings(ccb_node->mErrorStrings);
	ccb_node->mErrorStrings = NULL;

	imma_client_node_get(&cb->client_tree, &immHandle, &cl_node);
	if (!(cl_node && cl_node->isOm)) {
		rc = SA_AIS_ERR_LIBRARY;
		TRACE_4("ERR_LIBRARY: SaImmHandleT associated with Ccb %u is not valid",
			ccb_node->mCcbId);
		goto done;
	}

	if (cl_node->stale || !(cb->is_immnd_up)) {
		TRACE_3("ERR_FAILED_OPERATION: IMMND DOWN and/or IMM Handle %llx "
			"is stale - Ccb %u has to be aborted.", 
			ccb_node->mImmHandle, ccb_node->mCcbId);
		/* No point in resurrecting here since we know the ccb-id failed.
		   But we can avoid exposing with BAD_HANDLE because the ccb-id
		   failed in non critical phase. Instead discard the ccb-id and
		   force the user to refresh the ccb-id.
		*/
		ccb_node->mAborted = true;
		rc = SA_AIS_ERR_FAILED_OPERATION;
		goto done;
	}

	if(ccb_node->mAugCcb) {
		TRACE("Apply for augmentation for CcbId %u", ccb_node->mCcbId);
		ccb_node->mApplied = true;
		goto done;
	}

	/* Skip checking Admin Owner info  */

	/* Populate the CcbApply event */
	memset(&evt, 0, sizeof(IMMSV_EVT));
	evt.type = IMMSV_EVT_TYPE_IMMND;
	evt.info.immnd.type = IMMND_EVT_A2ND_CCB_APPLY;
	evt.info.immnd.info.ccbId = ccb_node->mCcbId;

	imma_proc_increment_pending_reply(cl_node);
	ccb_node->mExclusive = true; 
	ccb_node->mApplying = true;
	ccbId = ccb_node->mCcbId;

	rc = imma_evt_fake_evs(cb, &evt, &out_evt, cl_node->syncr_timeout, cl_node->handle, &locked, false);
	cl_node = NULL;
	ccb_node = NULL;

	TRACE("fake_evs for ccb-apply returned %u", rc);

	if (out_evt) {
		/* Process the outcome, note this is after a blocking call. */
		osafassert(out_evt->type == IMMSV_EVT_TYPE_IMMA);
		osafassert((out_evt->info.imma.type == IMMA_EVT_ND2A_IMM_ERROR) ||
			(out_evt->info.imma.type == IMMA_EVT_ND2A_IMM_ERROR_2));
		if (rc != SA_AIS_OK) {
			TRACE_4("CCB-APPLY - Error return from fevs!:%u, "
				"yet reply message also received (?) with error %u", rc,
				out_evt->info.imma.info.errRsp.error);
			osafassert(out_evt->info.imma.info.errRsp.error != SA_AIS_OK);
		} else {
			rc = out_evt->info.imma.info.errRsp.error;
			TRACE_1("CCB APPLY - reply received from IMMND rc:%u", rc);
			if(out_evt->info.imma.type == IMMA_EVT_ND2A_IMM_ERROR_2) {
				newErrorStrings = 
					imma_getErrorStrings(&(out_evt->info.imma.info.errRsp));
			}			
		}

		free(out_evt);
		out_evt=NULL;
	} else if (rc == SA_AIS_OK) {
		TRACE_4("CCB_APPLY - ERR_TIMEOUT: Got OK from fake_evs but no "
			"reply message!, from IMMND. Raising ERR_TIMEOUT "
			"to convey the unknown result from cb-apply!");
		rc = SA_AIS_ERR_TIMEOUT;
	}

	if (!locked && m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
		TRACE_4("CCB APPLY - ERR_LIBRARY: Lock failed");
		/* Error library overrides any existing rc including OK!*/
		rc = SA_AIS_ERR_LIBRARY;
		goto lock_fail;
	}
	locked = true;

	imma_client_node_get(&cb->client_tree, &immHandle, &cl_node);
	if (!(cl_node && cl_node->isOm)) {
		TRACE_3("CCB APPLY - client node gone on return from IMMND!");
		/* Client node removed during the blocked call. */
		if (rc == SA_AIS_OK) {
			/* yet the call succeeded! This should be very rare.
			   OK reply from ccbApply is precious. 
			   Ignore the nonexistent client_node for now. 
			*/
			TRACE_3("CCB APPLY - OK reply from IMMND on ccb-apply, but "
				"immHandle was closed on return. OK on CCB overrides "
				"immHandle problem.");
		} 
	} else {
		/* Normal case, cl_node still exists. */
		imma_proc_decrement_pending_reply(cl_node);
		if (cl_node->stale) {
			TRACE_3("CCB APPLY - Handle %llx is stale on return, exposed:%u",
				immHandle, cl_node->exposed);
			/* Became stale during the blocked call */
			if (rc == SA_AIS_OK) {
				/* yet the call succeeded! This should be very rare.
				   OK reply from ccbApply is precious. 
				   Ignore the stale handle for now. 
				   It may get resurrected since there will be no active ccb-id
				   associated with the ccb-handle.
				*/
				TRACE_3("CCB APPLY - Ok reply from IMMND overrides"
					"stale handle");
			} else if((rc != SA_AIS_ERR_TIMEOUT) && (rc != SA_AIS_ERR_BAD_HANDLE)) {
				TRACE_3("CCB APPLY - ERR_BAD_HANDLE(%u) overrides rc %u",
					SA_AIS_ERR_BAD_HANDLE, rc);
				rc = SA_AIS_ERR_BAD_HANDLE;
				/* is converted to ERR_TIMEOUT below */
				/* Dont set exposed just yet */
			}
		}

		imma_ccb_node_get(&cb->ccb_tree, &ccbHandle, &ccb_node);
		if (ccb_node) {
			ccb_node->mExclusive = false;
			ccb_node->mApplying = false;
			osafassert(ccb_node->mErrorStrings == NULL);
			if (rc == SA_AIS_OK) {
				ccb_node->mApplied = true;  
				TRACE_1("CCB APPLY - Successful Apply for ccb id %u", 
					ccb_node->mCcbId);
			} else {
				ccb_node->mErrorStrings = newErrorStrings;
				newErrorStrings = NULL; /* Dont free strings on exit */
			}
		} else {
			/*This branch should not be possible if ccb_node->mExclusive is respected. */
			TRACE_4("CCB APPLY - Ccb-handle closed on return from "
				"ccb-apply down-call. Should not be possible!");
			if (rc == SA_AIS_OK) {
				TRACE_3("CCB APPLY - Ok reply from server on ccb-apply, "
					"but ccbHandle was closed on return. OK on CCB "
					"overrides ccbHandle problem.");
			} else {
				TRACE_3("CCB APPLY - CcbHandle no longer valid on "
					"return, Error %u already set", rc);
			}
		}
	}

	if (rc == SA_AIS_OK) {
		goto done;
	}

	/* Now do some special conversion of error codes particular for ccb-apply */

	if (rc == SA_AIS_ERR_TRY_AGAIN) {
		TRACE_3("CCB APPLY: Error in send/receive of ccb-apply message! "
			"converting ERR_TRY_AGAIN to ...");
		/* No point in returning TRY_AGAIN for ccb-apply.
		   We KNOW the ccb is aborted.
		   TRY_AGAIN could only have come from imma_evt_fake_evs 
		   detecting IMMND DOWN *before* sending the request. 
		   There is no TRY_AGAIN case from server side ccbApply.
		   Convert ERR_TRY_AGAIN to FAILED_OPERATION !
		   Handle will also be stale, but that is taken care of later.
		*/

		if(ccb_node) {
			osafassert(!(ccb_node->mApplied));
			ccb_node->mAborted = true;
			rc = SA_AIS_ERR_FAILED_OPERATION;
			TRACE_3("CCB APPLY - .... ERR_FAILED_OPERATION");
		} else {
			rc = SA_AIS_ERR_LIBRARY;
			TRACE_4("CCB APPLY - .... ERR_LIBRARY: ccb-node removed during ccb-apply, "
					"ccb_node->mExclusive violated");
		}
	}

	if (rc == SA_AIS_ERR_BAD_HANDLE) {
		TRACE_3("CCB APPLY - ERR_TIMEOUT: Error during execution of "
			"ccb-apply! converting ERR_BAD_HANDLE to ERR_TIMEOUT");
		/* For the special case of ccb-apply we convert ERR_BAD_HANDLE to ERR_TIMEOUT.
		   This to converge all error handling for UNKNOWN OUTCOME of a ccb to a 
		   single error code. Unknown result for a ccb is what the user least of all
		   wants to deal with from apply, but it can happen.

		   ERR_BAD_HANDLE >>here<< means either IMMND went down during
		   processing of the apply down call, OR that the client-node or
		   ccb-node was removed by another thread in this client during
		   the down call.
		   
		   There is a small risk that the ccb managed to be applied
		   just before an IMMND crash. This is particularly the case for 
		   CCBs that do not invovle implementers, since they commit
		   independently at all IMMNDs. 
		   ERR_TIMEOUT is in fact converted *to* ERR_BAD_HANDLE by 
		   fevs/imma_proc_check_stale. This is preferred for most other calls, 
		   but not for ccb-apply!.

		   For the case of client_node or ccb_node removed the CCB
		   outcome should definitely be that the CCB was not applied.
		   But for simplicity we converge also these cases of ERR_BAD_HANDLE
		   to ERR_TIMEOUT.

		   The typical client would interpret ERR_BAD_HANDLE from
		   ccb-apply as if the CCB has not applied, but that could 
		   be wrong. 

		   Therefore we convert ERR_BAD_HANDLE back ERR_TIMEOUT here,
		   to convey the honest uncertatinty of the ccb result towards
		   the client. 

		   Recovery of the ccb outcome may in turn be attempted next.
		*/

		rc = SA_AIS_ERR_TIMEOUT;
	}

	if (rc == SA_AIS_ERR_TIMEOUT) {
		/* Try to contact IMMND when up again and recover the result
		  of the ccb. If we fail here also then remove the ccb-node since
		  we dont want it to indicate aborted, nor applied, nor active.
		*/
		TRACE_3("client_node %p exposed :%u", cl_node, cl_node ? (cl_node->exposed) : 0);

		/* Reset the ccb_node flags back to indicate apply is in progress. */
		ccb_node->mExclusive = true; 
		ccb_node->mApplying = true;

		/* Release the CB lock Before MDS Send */
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
		locked = false;

		rc = imma_proc_recover_ccb_result(cb, ccbId);

		if(ccb_node) { 
			/* ccb_node existed before recover_ccb_outcome downcall, try to
			   obtain it again.
			*/
			if(m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
				TRACE_4("CCB APPLY - ERR_LIBRARY: Lock failed");
				/* Error library overrides any existing rc including OK!*/
				rc = SA_AIS_ERR_LIBRARY;
				goto lock_fail;
			} 
			locked = true;

			ccb_node = NULL;
			imma_ccb_node_get(&cb->ccb_tree, &ccbHandle, &ccb_node);
			if(ccb_node) {
				ccb_node->mExclusive = false; 
				ccb_node->mApplying = false;
			}
		}

		if(rc == SA_AIS_OK) {
			TRACE_3("CCB APPLY - SA_AIS_OK: CCB result recovery SUCCEEDED - "
				"Apply/Commit for ccb id %u result was OK", ccbId);
			if (ccb_node) {
				ccb_node->mApplied = true;  
			}
			goto done;
		} 

		if(rc == SA_AIS_ERR_TIMEOUT) {
			TRACE_4("CCB APPLY: CCB result recovery FAILED for "
				"ccb id %u returning ERR_TIMEOUT", ccbId);
			if (ccb_node) {
				ccb_node->mExclusive = true;
				imma_ccb_node_delete(cb, ccb_node);
				ccb_node = NULL;
			}
			goto done;
		}

		osafassert(rc == SA_AIS_ERR_FAILED_OPERATION);
		TRACE_3("CCB APPLY: CCB result recovery SUCCEEDED - "
				"ABORT for ccb id %u", ccbId);

	} /*if(rc == SA_AIS_ERR_TIMEOUT)*/

	if(rc == SA_AIS_ERR_NO_RESOURCES) {
		/* NO_RESOUCES from server is in the ccbApply case to be interpreted
		   as the PBE having too much backlog. */
		rc = SA_AIS_ERR_TRY_AGAIN;
		goto done;
	}

	if (ccb_node) {
		ccb_node->mAborted = true;
	}


 done:
	imma_free_errorStrings(newErrorStrings); /* In case of failed resurrect only */

	if (locked)
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);

 lock_fail:
	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
  Name          :  saImmOmAdminOperationInvoke_2/_o2
 
  Description   :  Invoke an Administrative Operation on an object in the IMM.
                   This a blocking syncronous call.

                   
  Arguments     :  ownerHandle - AdminOwner handle.
                   objectName - A pointer to the name of the object
                           on which the operation is to be invoked.
                   operationId - An id identifying the operation.
                   params - Parameters to be supplied to the operation.
                   operationReturnValue - The return value from implementer.
                   timeout - Invocation is considered to be failed if this 
                             function does not complete by the time specified.

  Return Values :  Refer to SAI-AIS specification for various return values.
 
  Notes         : Note the TWO return values!
******************************************************************************/
static SaAisErrorT admin_op_invoke_common(
					  SaImmAdminOwnerHandleT ownerHandle,
					  const SaNameT *objectName,
					  SaImmContinuationIdT continuationId,
					  SaImmAdminOperationIdT operationId,
					  const SaImmAdminOperationParamsT_2 **params,
					  SaAisErrorT *operationReturnValue,
					  SaTimeT timeout,
					  SaImmAdminOperationParamsT_2 ***returnParams,
					  bool isA2bCall);

SaAisErrorT saImmOmAdminOperationInvoke_2(SaImmAdminOwnerHandleT ownerHandle,
					  const SaNameT *objectName,
					  SaImmContinuationIdT continuationId,
					  SaImmAdminOperationIdT operationId,
					  const SaImmAdminOperationParamsT_2 **params,
					  SaAisErrorT *operationReturnValue, 
					  SaTimeT timeout)
{
	return admin_op_invoke_common(ownerHandle, objectName, continuationId,
		operationId, params, operationReturnValue, timeout, NULL, false);
}


SaAisErrorT saImmOmAdminOperationInvoke_o2(SaImmAdminOwnerHandleT ownerHandle,
					   const SaNameT *objectName,
					   SaImmContinuationIdT continuationId,
					   SaImmAdminOperationIdT operationId,
					   const SaImmAdminOperationParamsT_2 **params,
					   SaAisErrorT *operationReturnValue,
					   SaTimeT timeout,
					   SaImmAdminOperationParamsT_2 ***returnParams)
{
	return admin_op_invoke_common(ownerHandle, objectName, continuationId,
		operationId, params, operationReturnValue, timeout, returnParams, true);
}

static SaAisErrorT admin_op_invoke_common(
				   SaImmAdminOwnerHandleT ownerHandle,
				   const SaNameT *objectName,
				   SaImmContinuationIdT continuationId,
				   SaImmAdminOperationIdT operationId,
				   const SaImmAdminOperationParamsT_2 **params,
				   SaAisErrorT *operationReturnValue,
				   SaTimeT timeout,
				   SaImmAdminOperationParamsT_2 ***returnParams,
				   bool isA2bCall)
{
	SaAisErrorT rc = SA_AIS_OK;
	IMMA_CB *cb = &imma_cb;
	IMMSV_EVT evt;
	IMMSV_EVT *out_evt = NULL;
	IMMA_ADMIN_OWNER_NODE *ao_node = NULL;
	IMMA_CLIENT_NODE *cl_node = NULL;
	bool locked = true;
	SaImmHandleT immHandle=0LL;
	SaUint32T adminOwnerId = 0;
	bool opIdEsc = (operationId >= SA_IMM_PARAM_ADMOP_ID_ESC);
	bool opNamePar = false;
	TRACE_ENTER();

	if (cb->sv_id == 0) {
		TRACE_2("ERR_BAD_HANDLE: No initialized handle exists!");
		return SA_AIS_ERR_BAD_HANDLE;
	}

	if ((objectName == NULL) || (operationReturnValue == NULL) ||
	    (objectName->length == 0) || (objectName->length >= SA_MAX_NAME_LENGTH)
	    || (params == NULL)) {
		rc = SA_AIS_ERR_INVALID_PARAM;
		goto done;
	}

	if (!imma_proc_is_adminop_params_valid(params)) {
		rc = SA_AIS_ERR_INVALID_PARAM;
		goto done;
	}

	if(returnParams) {
		/* Detach from any old lingering result. */
		*returnParams = NULL;
	}

	if (cb->is_immnd_up == false) {
		TRACE_3("ERR_TRY_AGAIN: IMMND is DOWN");
		return SA_AIS_ERR_TRY_AGAIN;
	}

	/*Overwrite any old/uninitialized value. */
	*operationReturnValue = SA_AIS_ERR_NO_SECTIONS;	/* Set to bad value to prevent user mistakes. */

	/* get the CB Lock */
	if (m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
		rc = SA_AIS_ERR_LIBRARY;
		TRACE_4("ERR_LIBRARY: Lock failed");
		goto lock_fail;
	}
	/*locked == true already */

	/* Get the Admin Owner info  */
	imma_admin_owner_node_get(&cb->admin_owner_tree, &ownerHandle, &ao_node);
	if (!ao_node) {
		TRACE_2("ERR_BAD_HANDLE: No admin owner associated with admin owner handle!");
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto ao_not_found;
	}

	if(ao_node->mAugCcb) {
		TRACE_2("ERR_NO_RESOURCES: Augmented CCB AdminOwner handle not allowed here");
		rc = SA_AIS_ERR_NO_RESOURCES;
		goto ao_not_found;
	}

	immHandle = ao_node->mImmHandle;
	adminOwnerId = ao_node->mAdminOwnerId;
	ao_node = NULL;

	/* Look up client node also, to verify that the client handle
	   is still active. */

	imma_client_node_get(&cb->client_tree, &immHandle, &cl_node);
	if (!(cl_node && cl_node->isOm)) {
		rc = SA_AIS_ERR_LIBRARY;
		TRACE_4("ERR_LIBRARY: No valid SaImmHandleT associated with Admin owner");
		goto client_not_found;
	}

	if (cl_node->stale) {
		TRACE_1("IMM Handle %llx is stale", immHandle);
		bool resurrected = imma_om_resurrect(cb, cl_node, &locked);
		cl_node = NULL;

		if (!locked && m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
			TRACE_4("ERR_LIBRARY: Lock failed");
			rc = SA_AIS_ERR_LIBRARY;
			goto lock_fail;
		}
		locked = true;

		imma_client_node_get(&cb->client_tree, &immHandle, &cl_node);
		if (!resurrected || !cl_node || !(cl_node->isOm) || cl_node->stale) {
			TRACE_3("ERR_BAD_HANDLE: Reactive ressurect of handle %llx failed", immHandle);
			if (cl_node && cl_node->stale) {cl_node->exposed = true;}
			rc = SA_AIS_ERR_BAD_HANDLE;
			goto stale_handle;
		}

		TRACE_1("Reactive resurrect of handle %llx succeeded", immHandle);
	}

	if(isA2bCall && !(cl_node->isImmA2b)) {
		rc = SA_AIS_ERR_VERSION;
		TRACE_2("ERR_VERSION: saImmOmAdminOperationInvoke_o2 only supported for "
			"A.02.11 and above");
		goto stale_handle;

	}

	/* convert the timeout to 10 ms value and add it to the sync send 
	   timeout */
	timeout = m_IMMSV_CONVERT_SATIME_TEN_MILLI_SEC(timeout);

	if (timeout < NCS_SAF_MIN_ACCEPT_TIME) {
		timeout = IMMSV_WAIT_TIME;
	}

	imma_proc_increment_pending_reply(cl_node);

	/* Populate the Admin-op event */
	memset(&evt, 0, sizeof(IMMSV_EVT));
	evt.type = IMMSV_EVT_TYPE_IMMND;
	evt.info.immnd.type = IMMND_EVT_A2ND_IMM_ADMOP;

	evt.info.immnd.info.admOpReq.adminOwnerId = adminOwnerId;
	evt.info.immnd.info.admOpReq.operationId = operationId;
	evt.info.immnd.info.admOpReq.continuationId = continuationId;	/*New API */

	/* The timeout is packed here and goes over fevs, need to
	   consider how the timing of FEVS itself should be subtracted.
	   The timeout is also set up in the FEVS call so it IS handled
	   on the agent side. Not sure why we need it on the server, other than
	   as a "leniency indicator". That is if timeout is large we may need to
	   increase the wait time in the server above some default. */
	evt.info.immnd.info.admOpReq.timeout = timeout;

	if ((immInvocations < 0) || (immInvocations == 0x7fffffff)) {
		immInvocations = 0;
	}
	TRACE("immInvocations:%i", immInvocations);
	evt.info.immnd.info.admOpReq.invocation = ++immInvocations;

	evt.info.immnd.info.admOpReq.objectName.size = strnlen((char *)objectName->value, objectName->length) + 1;
	if (objectName->length + 1 < evt.info.immnd.info.admOpReq.objectName.size) {
		evt.info.immnd.info.admOpReq.objectName.size = objectName->length + 1;
	}

	/*alloc-1 */
	evt.info.immnd.info.admOpReq.objectName.buf = malloc(evt.info.immnd.info.admOpReq.objectName.size);

	strncpy(evt.info.immnd.info.admOpReq.objectName.buf,
		(char *)objectName->value, evt.info.immnd.info.admOpReq.objectName.size);
	evt.info.immnd.info.admOpReq.objectName.buf[evt.info.immnd.info.admOpReq.objectName.size - 1] = '\0';

	osafassert(evt.info.immnd.info.admOpReq.params == NULL);
	const SaImmAdminOperationParamsT_2 *param = NULL;
	int i;
	for (i = 0; params[i]; ++i) {
		param = params[i];
		/*alloc-2 */
		IMMSV_ADMIN_OPERATION_PARAM *p = malloc(sizeof(IMMSV_ADMIN_OPERATION_PARAM));
		memset(p, 0, sizeof(IMMSV_ADMIN_OPERATION_PARAM));
		TRACE("PARAM:%s \n", param->paramName);

		if(!opNamePar) {
			opNamePar = opIdEsc && (strcmp(param->paramName, SA_IMM_PARAM_ADMOP_NAME) == 0);
			if(opNamePar && (param->paramType != SA_IMM_ATTR_SASTRINGT)) {
				TRACE_2("ERR_INVALID_PARAM: Param %s must be of type SaStringT",
					SA_IMM_PARAM_ADMOP_NAME);
				rc = SA_AIS_ERR_INVALID_PARAM;
				free(p);
				p=NULL;
				goto mds_send_fail;
			}
		}

		p->paramName.size = strlen(param->paramName) + 1;
		if (p->paramName.size >= SA_MAX_NAME_LENGTH) {
			TRACE_2("ERR_INVALID_PARAM: Param name too long");
			rc = SA_AIS_ERR_INVALID_PARAM;
			free(p);
			p=NULL;
			goto mds_send_fail;
		}
		/*alloc-3 */
		p->paramName.buf = malloc(p->paramName.size);
		strncpy(p->paramName.buf, param->paramName, p->paramName.size);

		p->paramType = param->paramType;
		/*alloc-4 */
		imma_copyAttrValue(&(p->paramBuffer), param->paramType, param->paramBuffer);

		p->next = evt.info.immnd.info.admOpReq.params;	/*NULL initially. */
		evt.info.immnd.info.admOpReq.params = p;
	}

	if(opIdEsc && !opNamePar && cl_node->isImmA2b) {
		TRACE_2("ERR_INVALID_PARAM: Op-id > %llx requires param %s",
			(long long unsigned int) SA_IMM_PARAM_ADMOP_ID_ESC, SA_IMM_PARAM_ADMOP_NAME);
		rc = SA_AIS_ERR_INVALID_PARAM;
		goto mds_send_fail;
	}

	/* NOTE: Re-implement to send ND->ND instead of using FEVS, 
	   less resources used and probably faster.  But we need to
	   uphold exclusiveness relative to ccbs .. see ERR_BUSY
	*/

	rc = imma_evt_fake_evs(cb, &evt, &out_evt, timeout, cl_node->handle, &locked, false);
	cl_node = NULL;

	TRACE("Fevs send RETURNED:%u", rc);

	if (rc != SA_AIS_OK) {
		goto mds_send_fail;
	}

	if (out_evt) {
		/* Process the outcome, note this is after a blocking call. */
		osafassert(out_evt->type == IMMSV_EVT_TYPE_IMMA);
		if (out_evt->info.imma.type == IMMA_EVT_ND2A_IMM_ERROR) {
			rc = out_evt->info.imma.info.errRsp.error;
			TRACE("ERROR returned:%u", rc);
			osafassert(rc != SA_AIS_OK);
			*operationReturnValue = IMMSV_IMPOSSIBLE_ERROR;	//Bogus result since error is set.
		} else {
			TRACE("Normal return");
			osafassert((out_evt->info.imma.type == IMMA_EVT_ND2A_ADMOP_RSP) ||
				(out_evt->info.imma.type == IMMA_EVT_ND2A_ADMOP_RSP_2));
			*operationReturnValue = out_evt->info.imma.info.admOpRsp.result;
			if((*operationReturnValue) == IMMSV_IMPOSSIBLE_ERROR) {
				rc = SA_AIS_ERR_FAILED_OPERATION;
			}
			if((out_evt->info.imma.type == IMMA_EVT_ND2A_ADMOP_RSP_2) && 
				(out_evt->info.imma.info.admOpRsp.parms)) {
				TRACE("Decoding return params");
				if(returnParams) {
					*returnParams = imma_proc_get_params(out_evt->info.imma.info.admOpRsp.parms);
					//imma_proc_free_pointers(cb, &(out_evt->info.imma)); ABT: Crashes, figure out why!
				} else {
					imma_proc_free_pointers(cb, &(out_evt->info.imma));
				}
			}
		}


		free(out_evt);
		out_evt=NULL;
	}

 mds_send_fail:
	/*We may be un-locked here but this should not matter.
	   We are freing heap objects that should only be vissible from this
	   thread. */

	/*TODO 12345 The code below should be moved to common ? 
	   This to allow re-use by all allocators of this event type.
	   At least it needs to be duplicated in immnd_evt.c/immnd_evt_destroy */

	if (evt.info.immnd.info.admOpReq.objectName.buf) {	/*free-1 */
		free(evt.info.immnd.info.admOpReq.objectName.buf);
		evt.info.immnd.info.admOpReq.objectName.buf = NULL;
	}
	while (evt.info.immnd.info.admOpReq.params) {
		IMMSV_ADMIN_OPERATION_PARAM *p = evt.info.immnd.info.admOpReq.params;
		evt.info.immnd.info.admOpReq.params = p->next;

		if (p->paramName.buf) {	/*free-3 */
			free(p->paramName.buf);
			p->paramName.buf = NULL;
		}
		immsv_evt_free_att_val(&(p->paramBuffer), p->paramType);	/*free-4 */
		p->next = NULL;
		free(p);	/*free-2 */
	}

	if (!locked && m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
		TRACE_4("ERR_LIBRARY: Lock failed");
		rc = SA_AIS_ERR_LIBRARY;
		goto lock_fail1;
	}
	locked = true;

	imma_client_node_get(&cb->client_tree, &immHandle, &cl_node);	
	if (cl_node && cl_node->isOm) {
		imma_proc_decrement_pending_reply(cl_node);
	} else {
		/*rc = SA_AIS_ERR_BAD_HANDLE;*/
		TRACE_3("client_node_get failed. Handle closed during admop call");
		/* Ignore the lost client node. The admop as such has 
		   completed anyway, with or without error.
		   Any attempt to use the same immHandle later will discover the problem. 
		*/
	}

	if(rc == SA_AIS_ERR_BAD_HANDLE && cl_node && cl_node->stale) {
		/* This ERR_BAD_HANDLE would have come from imma_fake_evs 
		   and check_stale_handle (immnd crash => timeout => bad_handle) if IMMND
		   crashed during the down call.
		   We can not convert this to TRY_AGAIN because the admin-op may actually
		   have been performed at some other IMMND where the receiving OI resides.
		   But we can convert it back to TIMEOUT, which could allow a resurrect.
		 */
		TRACE_3("Client became stale during down call");

		if(!cl_node->exposed) {
			TRACE_3("ADMIN_OP - ERR_TIMEOUT: converting BAD_HANDLE to TIMEOUT");
			rc = SA_AIS_ERR_TIMEOUT;
		}
	}

	

 ao_not_found:
 client_not_found:
 stale_handle:
	if (locked)
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);

 lock_fail1:
	if( out_evt && rc == SA_AIS_ERR_LIBRARY && returnParams && *returnParams != NULL ) {
		SaImmAdminOperationParamsT_2 **Params = *returnParams;
		SaImmAdminOperationParamsT_2 *q = NULL;
		unsigned int ix = 0;
		while(Params[ix]) {
			q = Params[ix];
			imma_freeAttrValue3(q->paramBuffer, q->paramType);
			free(q->paramName);
			q->paramName = NULL;
			free(q);
			++ix;
		}

		free(Params);
	}


 lock_fail:

 done:
	TRACE_LEAVE();
	return rc;
}


SaAisErrorT saImmOmAdminOperationMemoryFree(SaImmAdminOwnerHandleT ownerHandle,
					    SaImmAdminOperationParamsT_2 **returnParams)
{
	IMMA_CB *cb = &imma_cb;
	IMMA_CLIENT_NODE *cl_node = NULL;
	IMMA_ADMIN_OWNER_NODE *ao_node = NULL;
	SaImmHandleT immHandle=0LL;
	bool locked = true;
	SaAisErrorT rc = SA_AIS_OK;
	TRACE_ENTER();

	if (cb->sv_id == 0) {
		TRACE_2("ERR_BAD_HANDLE: No initialized handle exists!");
		return SA_AIS_ERR_BAD_HANDLE;
	}

	/* get the CB Lock */
	if (m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
		TRACE_4("ERR_LIBRARY: Lock failed");
		TRACE_LEAVE();
		return SA_AIS_ERR_LIBRARY;
	}
	/*locked == true already */

	imma_admin_owner_node_get(&cb->admin_owner_tree, &ownerHandle, &ao_node);
	if (!ao_node) {
		TRACE_2("ERR_BAD_HANDLE: No admin owner associated with admin owner handle!");
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto done;
	}
	immHandle = ao_node->mImmHandle;

	imma_client_node_get(&cb->client_tree, &immHandle, &cl_node);
	if (!(cl_node && cl_node->isOm)) {
		TRACE_2("ERR_BAD_HANDLE: Client not found");
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto done;
	}

	if (cl_node->stale) {
		TRACE_1("IMM Handle %llx is stale - ignoring", immHandle);
		/*return SA_AIS_ERR_BAD_HANDLE;*/
		/* Dont let a stale handle prevent the deallocation. */
	}

	if(!(cl_node->isImmA2b)) {
		rc = SA_AIS_ERR_VERSION;
		TRACE_2("ERR_VERSION: saImmOmAdminOperationMemoryFree only supported for "
			"A.02.11 and above");
		rc = SA_AIS_ERR_VERSION;
		goto done;
	}

	if(returnParams != NULL) {
		SaImmAdminOperationParamsT_2 *q = NULL;
		unsigned int ix = 0;
		while(returnParams[ix]) {
			q = returnParams[ix];
			imma_freeAttrValue3(q->paramBuffer, q->paramType);
			free(q->paramName);
			q->paramName = NULL;
			free(q);
			++ix;
		}

		free(returnParams);
	}

 done:
	if (locked) {
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
		locked = false;
	}
	
	TRACE_LEAVE();
	return rc;
}

static int push_async_adm_op_continuation(IMMA_CB *cb,	//in
					  SaInt32T invocation,	//in
					  SaImmHandleT immHandle,	//in
					  SaInvocationT userInvoc)	//in
{
	/* popAsyncAdmOpContinuation is in imma_proc.c */

	IMMA_CONTINUATION_RECORD *cr = cb->imma_continuations;

	TRACE("PUSH %i %llx %llx", invocation, immHandle, userInvoc);

	/* Check that there is not already a continuation with the same */
	/* user invocation id. */

	while (cr) {
		if (cr->userInvoc == userInvoc) {
			return 0;
		}
		cr = cr->next;
	}

	/* Ok add the new continuation. */
	cr = (IMMA_CONTINUATION_RECORD *)
	    malloc(sizeof(IMMA_CONTINUATION_RECORD));
	cr->invocation = invocation;
	cr->userInvoc = userInvoc;
	cr->immHandle = immHandle;
	cr->next = cb->imma_continuations;
	cb->imma_continuations = cr;
	return 1;
}

/****************************************************************************
  Name          :  saImmOmAdminOperationInvokeAsync/_2
 
  Description   :  Invoke an Administrative Operation on an object in the IMM.
                   This an asynchronous non-blocking call.
                   
  Arguments     :  ownerHandle - AdminOwner handle.
                   userInvocation - An invocation id allowing the user
                        to match the asynchronous return with the call.
                   objectName - A pointer to the name of the object
                           on which the operation is to be invoked.
                   operationId - An id identifying the operation.
                   params - Parameters to be supplied to the operation.

  Return Values :  Refer to SAI-AIS specification for various return values.
 
  Notes         :
******************************************************************************/
SaAisErrorT saImmOmAdminOperationInvokeAsync_2(SaImmAdminOwnerHandleT ownerHandle,
					       SaInvocationT userInvocation,
					       const SaNameT *objectName,
					       SaImmContinuationIdT continuationId,
					       SaImmAdminOperationIdT operationId,
					       const SaImmAdminOperationParamsT_2 **params)
{
	SaAisErrorT rc = SA_AIS_OK;
	IMMA_CB *cb = &imma_cb;
	IMMSV_EVT evt;
	IMMA_ADMIN_OWNER_NODE *ao_node = NULL;
	IMMA_CLIENT_NODE *cl_node = NULL;
	bool locked = true;
	SaImmHandleT immHandle=0LL;
	SaUint32T adminOwnerId = 0;
	bool opIdEsc = (operationId & SA_IMM_PARAM_ADMOP_ID_ESC);
	bool opNamePar = false;
	TRACE_ENTER();

	if (cb->sv_id == 0) {
		TRACE_2("ERR_BAD_HANDLE: No initialized handle exists!");
		return SA_AIS_ERR_BAD_HANDLE;
	}

	if ((objectName == NULL) || (objectName->length == 0) ||
	    (objectName->length >= SA_MAX_NAME_LENGTH) || (params == NULL)) {
		rc = SA_AIS_ERR_INVALID_PARAM;
		goto done;
	}

	if (!imma_proc_is_adminop_params_valid(params)) {
		rc = SA_AIS_ERR_INVALID_PARAM;
		goto done;
	}

	if (cb->is_immnd_up == false) {
		TRACE_3("ERR_TRY_AGAIN: IMMND is DOWN");
		return SA_AIS_ERR_TRY_AGAIN;
	}

	/* get the CB Lock */
	if (m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
		rc = SA_AIS_ERR_LIBRARY;
		TRACE_4("ERR_LIBRARY: Lock failed");
		goto lock_fail;
	}
	/*locked == true already */

	/* Get the Admin Owner info  */
	imma_admin_owner_node_get(&cb->admin_owner_tree, &ownerHandle, &ao_node);
	if (!ao_node) {
		TRACE_2("ERR_BAD_HANDLE: No admin owner associated with admin owner handle!");
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto ao_not_found;
	}

	if(ao_node->mAugCcb) {
		TRACE_2("ERR_NO_RESOURCES: Augmented CCB AdminOwner handle not allowed here");
		rc = SA_AIS_ERR_NO_RESOURCES;
		goto ao_not_found;
	}

	immHandle = ao_node->mImmHandle;
	adminOwnerId = ao_node->mAdminOwnerId;
	ao_node = NULL;

	/* Look up client node also, to verify that the client handle
	   is still active. */

	imma_client_node_get(&cb->client_tree, &immHandle, &cl_node);
	if (!(cl_node && cl_node->isOm)) {
		rc = SA_AIS_ERR_LIBRARY;
		TRACE_4("ERR_LIBRARY: No valid SaImmHandleT associated with Admin owner");
		goto client_not_found;
	}

	if (cl_node->stale) {
		TRACE_1("IMM Handle %llx is stale", immHandle);
		bool resurrected = imma_om_resurrect(cb, cl_node, &locked);
		cl_node = NULL;

		if (!locked && m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
			TRACE_4("ERR_LIBRARY: Lock failed");
			rc = SA_AIS_ERR_LIBRARY;
			goto lock_fail;
		}
		locked = true;

		imma_client_node_get(&cb->client_tree, &immHandle, &cl_node);
		if (!resurrected || !cl_node || !(cl_node->isOm) || cl_node->stale) {
			TRACE_3("ERR_BAD_HANDLE: Reactive ressurect of handle %llx failed", immHandle);
			if (cl_node && cl_node->stale) {cl_node->exposed = true;}
			rc = SA_AIS_ERR_BAD_HANDLE;
			goto stale_handle;
		}

		TRACE_1("Reactive resurrect of handle %llx succeeded", immHandle);
	}

	/* TODO: Should the timeout value be set to any sensible value ? */

	if (cl_node->isImmA2bCbk) {/* cl_node->isImmA2bCbk is true only if there is an A.2.11 callback */
		if (cl_node->o.mCallbkA2b.saImmOmAdminOperationInvokeCallback==NULL) {
			rc = SA_AIS_ERR_INIT;
			TRACE_2("ERR_INIT: The SaImmOmAdminOperationInvokeCallbackT_o2 "
				"function was not set in the initialization of the handle "
				"=> saImmOmAdminOperationInvokeAsync can not reply");
			goto no_callback;
		}
	} else {
		if (cl_node->o.mCallbk.saImmOmAdminOperationInvokeCallback==NULL) {
			rc = SA_AIS_ERR_INIT;
			TRACE_2("ERR_INIT: The SaImmOmAdminOperationInvokeCallbackT "
				"function was not set in the initialization of the handle "
				"=> saImmOmAdminOperationInvokeAsync can not reply");
			goto no_callback;
		}
	}

	/* Populate & Send the Open Event to IMMND */
	memset(&evt, 0, sizeof(IMMSV_EVT));

	evt.type = IMMSV_EVT_TYPE_IMMND;
	evt.info.immnd.type = IMMND_EVT_A2ND_IMM_ADMOP_ASYNC;

	evt.info.immnd.info.admOpReq.adminOwnerId = adminOwnerId;
	evt.info.immnd.info.admOpReq.operationId = operationId;
	evt.info.immnd.info.admOpReq.continuationId = continuationId;	/*New API */
	evt.info.immnd.info.admOpReq.timeout = 0;	/* or IMMSV_WAIT_TIME? */
	if ((immInvocations < 0) || (immInvocations == 0x7fffffff)) {
		immInvocations = 0;
	}
	TRACE("immInvocations:%i", immInvocations);

	evt.info.immnd.info.admOpReq.invocation = -(++immInvocations);
	/*Negate invoc to encode async */

	evt.info.immnd.info.admOpReq.objectName.size = strnlen((char *)objectName->value, objectName->length) + 1;
	if (objectName->length + 1 < evt.info.immnd.info.admOpReq.objectName.size) {
		evt.info.immnd.info.admOpReq.objectName.size = objectName->length + 1;
	}

	/*alloc-1 */
	evt.info.immnd.info.admOpReq.objectName.buf = calloc(1, evt.info.immnd.info.admOpReq.objectName.size);

	strncpy(evt.info.immnd.info.admOpReq.objectName.buf,
		(char *)objectName->value, evt.info.immnd.info.admOpReq.objectName.size);
	/* evt.info.immnd.info.admOpReq.objectName.buf[objectName->length] = '\0'; */

	osafassert(evt.info.immnd.info.admOpReq.params == NULL);

	const SaImmAdminOperationParamsT_2 *param;
	int i;
	for (i = 0; params[i]; ++i) {
		param = params[i];
		/*alloc-2 */
		IMMSV_ADMIN_OPERATION_PARAM *p = malloc(sizeof(IMMSV_ADMIN_OPERATION_PARAM));
		memset(p, 0, sizeof(IMMSV_ADMIN_OPERATION_PARAM));
		TRACE("PARAM:%s ", param->paramName);

		if(!opNamePar) {
			opNamePar = opIdEsc && (strcmp(param->paramName, SA_IMM_PARAM_ADMOP_NAME) == 0);
			if(opNamePar && (param->paramType != SA_IMM_ATTR_SASTRINGT)) {
				TRACE_2("ERR_INVALID_PARAM: Param %s must be of type SaStringT",
					SA_IMM_PARAM_ADMOP_NAME);
				rc = SA_AIS_ERR_INVALID_PARAM;
				free(p);
				p=NULL;
				goto mds_send_fail;
			}
		}

		p->paramName.size = strlen(param->paramName) + 1;
		if (p->paramName.size >= SA_MAX_NAME_LENGTH) {
			TRACE_2("ERR_INVALID_PARAM: Param name too long");
			rc = SA_AIS_ERR_INVALID_PARAM;
			free(p);
			p=NULL;
			goto mds_send_fail;
		}

		/*alloc-3 */
		p->paramName.buf = malloc(p->paramName.size);
		strncpy(p->paramName.buf, param->paramName, p->paramName.size);

		p->paramType = param->paramType;
		/*alloc-4 */
		imma_copyAttrValue(&(p->paramBuffer), param->paramType, param->paramBuffer);

		p->next = evt.info.immnd.info.admOpReq.params;	/*NULL initially. */
		evt.info.immnd.info.admOpReq.params = p;
	}

	if(opIdEsc && !opNamePar) {
		TRACE_2("ERR_INVALID_PARAM: Op-id > %llx requires param %s",
			(long long unsigned int) SA_IMM_PARAM_ADMOP_ID_ESC, SA_IMM_PARAM_ADMOP_NAME);
		rc = SA_AIS_ERR_INVALID_PARAM;
		goto mds_send_fail;
	}

	if (!push_async_adm_op_continuation(cb,
					    evt.info.immnd.info.admOpReq.invocation,
					    immHandle, userInvocation)) {
		TRACE_2("ERR_INVALID_PARAM: Provided invocation id (%llx) "
			"is not unique, not even in this client instance", userInvocation);
		rc = SA_AIS_ERR_INVALID_PARAM;
		goto mds_send_fail;
        }


	/* NOTE: Re-implement to send ND->ND instead of using FEVS, 
	   less resources used and probably faster. */

	/* Even though this is an asyncronous down call, we increment pending-reply, 
	   because a crash of the local IMMND before the asyncronous reply of this
	   call is received will mean a lost reply, i.e. broken contract with the
	   OM application, i.e. exposed handle. In fact we need to increment 
	   pending-reply to *prevent* a reactive resurrect of the OM handle from
	   succeeding. The OM application *should* instead exit the dispatch loop
	   with BAD_HANDLE so that it is made aware of the broken contract.
	 */
	imma_proc_increment_pending_reply(cl_node);

	/* out_evt is NULL => fevs is async */
	rc = imma_evt_fake_evs(cb, &evt, NULL, 0, cl_node->handle, &locked, false);
	TRACE("Fevs send RETURNED:%u", rc);

	if (rc != SA_AIS_OK) {

		if (!locked) {
			if (m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
				rc = SA_AIS_ERR_LIBRARY;
				TRACE_4("ERR_LIBRARY: Lock failed");
				goto mds_send_fail;
			} else {
				locked = true;
			}
		}
		if (!imma_popAsyncAdmOpContinuation(cb, evt.info.immnd.info.admOpReq.invocation, 
							&immHandle, &userInvocation)) {
			TRACE_3("Missmatch on continuation for saImmOmAdminOperationInvokeAsync_2");
		}
	}

 mds_send_fail:
	/*We may be un-locked here but this should not matter.
	   We are freing heap objects that should only be vissible from this
	   thread. */

	/* NOTE: 12345 The code below should be moved to common ? 
	   This to allow re-use by all allocators of this event type.
	   At least it needs to be duplicated in immnd_evt.c/immnd_evt_destroy */
	if (evt.info.immnd.info.admOpReq.objectName.buf) {	/*free-1 */
		free(evt.info.immnd.info.admOpReq.objectName.buf);
		evt.info.immnd.info.admOpReq.objectName.buf = NULL;
		evt.info.immnd.info.admOpReq.objectName.size = 0;
	}
	while (evt.info.immnd.info.admOpReq.params) {
		IMMSV_ADMIN_OPERATION_PARAM *p = evt.info.immnd.info.admOpReq.params;
		evt.info.immnd.info.admOpReq.params = p->next;

		if (p->paramName.buf) {	/*free-3 */
			free(p->paramName.buf);
			p->paramName.buf = NULL;
		}
		immsv_evt_free_att_val(&(p->paramBuffer), p->paramType);	/*free-4 */
		p->next = NULL;
		free(p);	/*free-2 */
		p=NULL;
	}

	/* Note the imma_proc_decrement_pending_reply is done in the OM upcall
	   with the reply for the asyncronous operation. 
	 */
 ao_not_found:
 client_not_found:
 stale_handle:
 no_callback:
	if (locked)
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);

 lock_fail:
 done:
	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
  Name          :  saImmOmAdminOperationContinue
 
  Description   :  Allows a process to take over the continuation of an
                   administrative operation that was initiated with a 
                   admin owner handle that has been finalized before the
                   reply was received.
                   
                   
  Arguments     :  ownerHandle - AdminOwner handle.
                   userInvocation - An invocation id allowing the user
                        to match the asynchronous return with the call.
                   objectName - A pointer to the name of the object
                           on which the operation is to be invoked.
                           Actually redundant information.
                   continuationId - identifies the corresponding pervious
                   invocation.

  Return Values :  Refer to SAI-AIS specification for various return values.
 
  Notes         :
******************************************************************************/

SaAisErrorT saImmOmAdminOperationContinue(SaImmAdminOwnerHandleT ownerHandle,
					  const SaNameT *objectName,
					  SaImmContinuationIdT continuationId, SaAisErrorT *operationReturnValue)
{
	TRACE_2("saImmOmAdminOperationContinue not implemented use saImmOmAdminOperationContinueAsync instead");
	return SA_AIS_ERR_NOT_SUPPORTED;
}

SaAisErrorT saImmOmAdminOperationContinue_o2(SaImmAdminOwnerHandleT ownerHandle,
					     const SaNameT *objectName,
					     SaImmContinuationIdT continuationId,
					     SaAisErrorT *operationReturnValue,
					     SaImmAdminOperationParamsT_2 ***returnParams)
{
	TRACE_2("saImmOmAdminOperationContinue_o2 not implemented use "
		"saImmOmAdminOperationContinueAsync instead");
	return SA_AIS_ERR_NOT_SUPPORTED;
}


SaAisErrorT saImmOmAdminOperationContinueAsync(SaImmAdminOwnerHandleT ownerHandle,
					       SaInvocationT invocation,
					       const SaNameT *objectName, SaImmContinuationIdT continuationId)
{
	TRACE_2("saImmOmAdminOperationContinueAsync not yet implemented");
	return SA_AIS_ERR_NOT_SUPPORTED;
}

SaAisErrorT saImmOmAdminOperationContinuationClear(SaImmAdminOwnerHandleT ownerHandle,
						   const SaNameT *objectName, SaImmContinuationIdT continuationId)
{
	TRACE_2("saImmOmAdminOperationContinuationClear not yet implemented");
	return SA_AIS_ERR_NOT_SUPPORTED;
}


SaAisErrorT saImmOmClassCreate_2(SaImmHandleT immHandle,
				 const SaImmClassNameT className,
				 SaImmClassCategoryT classCategory, const SaImmAttrDefinitionT_2 **attrDefinitions)
{
	SaAisErrorT rc = SA_AIS_OK;
	bool locked = true;
	IMMA_CB *cb = &imma_cb;
	IMMSV_EVT evt;
	IMMSV_EVT *out_evt = NULL;
	IMMA_CLIENT_NODE *cl_node = NULL;
	SaTimeT timeout = 0;
	IMMSV_ATTR_DEF_LIST *sysattr = NULL;
	const SaImmAttrDefinitionT_2 *attr;
	int i;
	TRACE_ENTER();

	if (cb->sv_id == 0) {
		TRACE_2("ERR_BAD_HANDLE: No initialized handle exists!");
		TRACE_LEAVE();
		return SA_AIS_ERR_BAD_HANDLE;
	}

	if ((className == NULL) || (attrDefinitions == NULL)) {
		TRACE_LEAVE();
		return SA_AIS_ERR_INVALID_PARAM;
	}

	attr = attrDefinitions[0];
	for (i = 0; attr != 0; attr = attrDefinitions[++i]) {
		if (attr->attrName == NULL)  {
			TRACE("NULL attrName , not allowed.");
                	TRACE_LEAVE();
			return SA_AIS_ERR_INVALID_PARAM;
		}

		if(attr->attrFlags & SA_IMM_ATTR_RDN) {
			if(((attr->attrValueType != SA_IMM_ATTR_SANAMET ) && 
						(attr->attrValueType != SA_IMM_ATTR_SASTRINGT))) {
				TRACE("ERR_INVALID_PARAM: RDN '%s' must be of type SaNameT or SaStringT", attr->attrName);
                		TRACE_LEAVE();
				return SA_AIS_ERR_INVALID_PARAM;
			}

			if(attr->attrFlags & SA_IMM_ATTR_MULTI_VALUE) {
				TRACE("ERR_INVALID_PARAM: RDN '%s' can not be multivalued", attr->attrName);
				TRACE_LEAVE();
                       		 return SA_AIS_ERR_INVALID_PARAM;
    			}
		}
	}

	if (cb->is_immnd_up == false) {
		TRACE_2("ERR_TRY_AGAIN: IMMND is DOWN");
                TRACE_LEAVE();
		return SA_AIS_ERR_TRY_AGAIN;
	}

	/* get the CB Lock */
	if (m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
		rc = SA_AIS_ERR_LIBRARY;
		TRACE_4("ERR_LIBRARY: Lock failed");
		goto lock_fail;
	}
	/*locked is true already */

	imma_client_node_get(&cb->client_tree, &immHandle, &cl_node);
	if (!(cl_node && cl_node->isOm)) {
		TRACE_2("ERR_BAD_HANDLE: Client node is missing");
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto client_not_found;
	}

	if (cl_node->stale) {
		TRACE_1("IMM Handle %llx is stale", immHandle);
		bool resurrected = imma_om_resurrect(cb, cl_node, &locked);

		if (!locked && m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
			TRACE_4("ERR_LIBRARY: Lock failed");
			rc = SA_AIS_ERR_LIBRARY;
			goto lock_fail;
		}
		locked = true;

		imma_client_node_get(&cb->client_tree, &immHandle, &cl_node);

		if (!resurrected || !cl_node || !(cl_node->isOm) || cl_node->stale) {
			TRACE_3("ERR_BAD_HANDLE: Reactive ressurect of handle %llx failed", immHandle);
			if (cl_node && cl_node->stale) {cl_node->exposed = true;}
			rc = SA_AIS_ERR_BAD_HANDLE;
			goto stale_handle;
		}

		TRACE_1("Reactive resurrect of handle %llx succeeded", immHandle);
	}

	timeout = cl_node->syncr_timeout;

	imma_proc_increment_pending_reply(cl_node);

	/* Populate the ClassCreate event */
	memset(&evt, 0, sizeof(IMMSV_EVT));
	evt.type = IMMSV_EVT_TYPE_IMMND;
	evt.info.immnd.type = IMMND_EVT_A2ND_CLASS_CREATE;

	evt.info.immnd.info.classDescr.className.size = strlen(className) + 1;
	if (evt.info.immnd.info.classDescr.className.size == 1) {
		/*ABT bugfix 081029 Empty classname should not be allowed. */
		rc = SA_AIS_ERR_INVALID_PARAM;
		TRACE("Zero length class name, not allowed.");
		goto mds_send_fail;
	}
	evt.info.immnd.info.classDescr.className.buf = malloc(evt.info.immnd.info.classDescr.className.size);	/*alloc-1 */
	strncpy(evt.info.immnd.info.classDescr.className.buf, className,
		(size_t)evt.info.immnd.info.classDescr.className.size);

	evt.info.immnd.info.classDescr.classCategory = classCategory;
	TRACE("name: %s category:%u", className, classCategory);
	int persistent = 0;
	attr = attrDefinitions[0];
	int attrClNameExist = 0;
	int attrAdmNameExist = 0;
	int attrImplNameExist = 0;
	for (i = 0; attr != 0; attr = attrDefinitions[++i]) {
		/* Ignore system attribute definitions that are
		   loaded since they are indistinguishable from being set
		   by the user. Better that they are redefined below each 
		   time by the system, even when the class is loaded from
		   backup. */
		if (strcmp(attr->attrName, sysaClName) == 0) {
			continue;
		} else if (strcmp(attr->attrName, sysaAdmName) == 0) {
			continue;
		} else if (strcmp(attr->attrName, sysaImplName) == 0) {
			continue;
		}

		IMMSV_ATTR_DEF_LIST *p =	/*alloc-2 */
		    malloc(sizeof(IMMSV_ATTR_DEF_LIST));
		memset(p, 0, sizeof(IMMSV_ATTR_DEF_LIST));

		p->d.attrName.size = strlen(attr->attrName) + 1;
		if (p->d.attrName.size == 1) {
			rc = SA_AIS_ERR_INVALID_PARAM;
			TRACE("Zero length attribute name in class %s def, not allowed.",
			      evt.info.immnd.info.classDescr.className.buf);
			free(p);
			p=NULL;
			goto mds_send_fail;
		}
		p->d.attrName.buf = malloc(p->d.attrName.size);	/* alloc-3 */
		strncpy(p->d.attrName.buf, attr->attrName, p->d.attrName.size);

		p->d.attrValueType = attr->attrValueType;
		if (!imma_proc_is_valid_type(p->d.attrValueType)) {
			rc = SA_AIS_ERR_INVALID_PARAM;
			TRACE("Unknown type not allowed for attr:%s class%s",
			      p->d.attrName.buf, evt.info.immnd.info.classDescr.className.buf);
			free(p->d.attrName.buf);	/* free-3 */
			p->d.attrName.buf = NULL;
			free(p);	/* free-2 */
			p=NULL;
			goto mds_send_fail;
		}
		p->d.attrFlags = attr->attrFlags;
		persistent = persistent | (attr->attrFlags & SA_IMM_ATTR_PERSISTENT);

		if (attr->attrDefaultValue) {
			p->d.attrDefaultValue =	/*alloc-4.1 */
			    malloc(sizeof(IMMSV_EDU_ATTR_VAL));
			memset(p->d.attrDefaultValue, 0, sizeof(IMMSV_EDU_ATTR_VAL));
			/*alloc-4.2 */
			imma_copyAttrValue(p->d.attrDefaultValue, attr->attrValueType, attr->attrDefaultValue);
		}

		p->next = evt.info.immnd.info.classDescr.attrDefinitions;	/*NULL initially */
		evt.info.immnd.info.classDescr.attrDefinitions = p;
	}

	/* Add system attribute class-name  */
	if (!attrClNameExist) {
		/*TRACE("Creating class name attribute def"); */
		sysattr =	/*alloc-2 */
		    malloc(sizeof(IMMSV_ATTR_DEF_LIST));
		memset(sysattr, 0, sizeof(IMMSV_ATTR_DEF_LIST));

		sysattr->d.attrName.size = strlen(sysaClName) + 1;
		sysattr->d.attrName.buf = malloc(sysattr->d.attrName.size);	/*alloc-3 */
		strncpy(sysattr->d.attrName.buf, sysaClName, sysattr->d.attrName.size);
		sysattr->d.attrValueType = SA_IMM_ATTR_SASTRINGT;
		if (classCategory == SA_IMM_CLASS_CONFIG) {
			sysattr->d.attrFlags |= SA_IMM_ATTR_CONFIG;
		} else if (classCategory == SA_IMM_CLASS_RUNTIME) {
			sysattr->d.attrFlags |= (persistent) ?
			    (SA_IMM_ATTR_RUNTIME | SA_IMM_ATTR_CACHED |
			     SA_IMM_ATTR_PERSISTENT) : (SA_IMM_ATTR_RUNTIME | SA_IMM_ATTR_CACHED);
		}
		sysattr->d.attrNtfId = 0;	/*alloc-4.1 */
		sysattr->d.attrDefaultValue = malloc(sizeof(IMMSV_EDU_ATTR_VAL));
		memset(sysattr->d.attrDefaultValue, 0, sizeof(IMMSV_EDU_ATTR_VAL));
		/*alloc-4.2 */
		imma_copyAttrValue(sysattr->d.attrDefaultValue, SA_IMM_ATTR_SASTRINGT, (SaImmAttrValueT)&className);
		sysattr->next = evt.info.immnd.info.classDescr.attrDefinitions;
		evt.info.immnd.info.classDescr.attrDefinitions = sysattr;
	}

	/* Add system attribute admin-owner */
	if (!attrAdmNameExist) {
		/*TRACE("Creating admin-owner name attribute def"); */
		sysattr =	/*alloc-2 */
		    malloc(sizeof(IMMSV_ATTR_DEF_LIST));
		memset(sysattr, 0, sizeof(IMMSV_ATTR_DEF_LIST));

		sysattr->d.attrName.size = strlen(sysaAdmName) + 1;
		sysattr->d.attrName.buf = malloc(sysattr->d.attrName.size);	/*alloc-3 */
		strncpy(sysattr->d.attrName.buf, sysaAdmName, sysattr->d.attrName.size);
		sysattr->d.attrValueType = SA_IMM_ATTR_SASTRINGT;
		/* Should this attribute really be a config attribute ?
		   Should it really be allowed to be persistent ? */
		if (classCategory == SA_IMM_CLASS_CONFIG) {
			sysattr->d.attrFlags |= SA_IMM_ATTR_CONFIG;
		} else if (classCategory == SA_IMM_CLASS_RUNTIME) {
			sysattr->d.attrFlags |= (persistent) ?
			    (SA_IMM_ATTR_RUNTIME | SA_IMM_ATTR_CACHED |
			     SA_IMM_ATTR_PERSISTENT) : (SA_IMM_ATTR_RUNTIME | SA_IMM_ATTR_CACHED);
		}
		sysattr->d.attrNtfId = 0;
		sysattr->d.attrDefaultValue = NULL;
		sysattr->next = evt.info.immnd.info.classDescr.attrDefinitions;
		evt.info.immnd.info.classDescr.attrDefinitions = sysattr;
	}

	/* Add system attribute implementer-name */
	if (!attrImplNameExist) {
		/*TRACE("Creating implementer name attribute def"); */
		sysattr =	/*alloc-2 */
		    malloc(sizeof(IMMSV_ATTR_DEF_LIST));
		memset(sysattr, 0, sizeof(IMMSV_ATTR_DEF_LIST));

		sysattr->d.attrName.size = strlen(sysaImplName) + 1;
		sysattr->d.attrName.buf = malloc(sysattr->d.attrName.size);	/*alloc-3 */
		strncpy(sysattr->d.attrName.buf, sysaImplName, sysattr->d.attrName.size);
		sysattr->d.attrValueType = SA_IMM_ATTR_SASTRINGT;
		/* Should this attribute really be a config attribute ?
		   Should it really be allowed to be persistent ? 
		   Config=> persistent but Runtime => can be changed by implementer.
		 */
		if (classCategory == SA_IMM_CLASS_CONFIG) {
			sysattr->d.attrFlags |= SA_IMM_ATTR_CONFIG;
		} else if (classCategory == SA_IMM_CLASS_RUNTIME) {
			sysattr->d.attrFlags |= (persistent) ?
			    (SA_IMM_ATTR_RUNTIME | SA_IMM_ATTR_CACHED |
			     SA_IMM_ATTR_PERSISTENT) : (SA_IMM_ATTR_RUNTIME | SA_IMM_ATTR_CACHED);
		}
		sysattr->d.attrNtfId = 0;
		sysattr->d.attrDefaultValue = NULL;
		sysattr->next = evt.info.immnd.info.classDescr.attrDefinitions;
		evt.info.immnd.info.classDescr.attrDefinitions = sysattr;
	}

	rc = imma_evt_fake_evs(cb, &evt, &out_evt, timeout, cl_node->handle, &locked, true);
	cl_node = NULL;

	if (rc != SA_AIS_OK) {
		goto mds_send_fail;
	}

	if (out_evt) {
		osafassert(out_evt->type == IMMSV_EVT_TYPE_IMMA);
		osafassert(out_evt->info.imma.type == IMMA_EVT_ND2A_IMM_ERROR);
		rc = out_evt->info.imma.info.errRsp.error;
		TRACE("Return code:%u", rc);
		free(out_evt);
		out_evt=NULL;
	}

 mds_send_fail:

	free(evt.info.immnd.info.classDescr.className.buf);	/*free-1 */
	evt.info.immnd.info.classDescr.className.buf = NULL;
	evt.info.immnd.info.classDescr.className.size = 0;

	/*free-2 free-3 free-4.1 free-4.2 */
	immsv_free_attrdefs_list(evt.info.immnd.info.classDescr.attrDefinitions);
	evt.info.immnd.info.classDescr.attrDefinitions = NULL;

	if (!locked && m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
		rc = SA_AIS_ERR_LIBRARY;
		TRACE_4("ERR_LIBRARY: Lock failed");
		/* Losing track of the pending reply count, but ERR_LIBRARY dominates*/
		goto lock_fail;
	}
	locked = true;

	imma_client_node_get(&cb->client_tree, &immHandle, &cl_node);	
	if (!(cl_node && cl_node->isOm)) {
		/*rc = SA_AIS_ERR_BAD_HANDLE;*/
		/* Let the result reflect if the class create succeeded or not. */
		TRACE_3("client_node_get failed");
		goto  client_not_found;
	}

	imma_proc_decrement_pending_reply(cl_node);

	if ((rc == SA_AIS_ERR_BAD_HANDLE) && cl_node->stale) {
		/* BAD_HANDLE from imma_proc_check_stale */
		/* ABT: We could possibly convert back to TIMEOUT to allow a resurrect?*/
		cl_node->exposed = true;
	}

 client_not_found:
 stale_handle:
	if (locked)
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);

 lock_fail:
	TRACE_LEAVE();
	return rc;
}

SaAisErrorT saImmOmClassDescriptionGet_2(SaImmHandleT immHandle,
					 const SaImmClassNameT className,
					 SaImmClassCategoryT *classCategory, SaImmAttrDefinitionT_2 ***attrDefinition)
{
	TRACE_ENTER();
	SaAisErrorT rc = SA_AIS_OK;
	uint32_t proc_rc = NCSCC_RC_SUCCESS;
	bool locked = false;
	IMMA_CB *cb = &imma_cb;
	IMMSV_EVT evt;
	IMMSV_EVT *out_evt = NULL;
	IMMA_CLIENT_NODE *cl_node = NULL;
	SaUint32T timeout = 0;
	TRACE_ENTER();

	if (cb->sv_id == 0) {
		TRACE_2("ERR_BAD_HANDLE: No initialized handle exists!");
		return SA_AIS_ERR_BAD_HANDLE;
	}

	if ((className == NULL) || (classCategory == NULL) || (attrDefinition == NULL)) {
		TRACE_LEAVE();
		return SA_AIS_ERR_INVALID_PARAM;
	}

	if (cb->is_immnd_up == false) {
		TRACE_3("ERR_TRY_AGAIN: IMMND is DOWN");
		return SA_AIS_ERR_TRY_AGAIN;
	}	

	/* get the CB Lock */
	if (m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
		rc = SA_AIS_ERR_LIBRARY;
		TRACE_4("ERR_LIBRARY: Lock failed");
		goto lock_fail;
	}
	locked = true;

	imma_client_node_get(&cb->client_tree, &immHandle, &cl_node);
	if (!(cl_node && cl_node->isOm)) {
		TRACE_2("ERR_BAD_HANDLE: Client node is missing");
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto client_not_found;
	}

	if (cl_node->stale) {
		TRACE_1("IMM Handle %llx is stale", immHandle);
		bool resurrected = imma_om_resurrect(cb, cl_node, &locked);

		if (!locked && m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
			TRACE_4("ERR_LIBRARY: Lock failed");
			rc = SA_AIS_ERR_LIBRARY;
			goto lock_fail;
		}
		locked = true;

		imma_client_node_get(&cb->client_tree, &immHandle, &cl_node);

		if (!resurrected || !cl_node || !(cl_node->isOm) || cl_node->stale) {
			TRACE_3("ERR_BAD_HANDLE: Reactive ressurect of handle %llx failed", immHandle);
			if (cl_node && cl_node->stale) {cl_node->exposed = true;}
			rc = SA_AIS_ERR_BAD_HANDLE;
			goto stale_handle;
		}

		TRACE_1("Reactive resurrect of handle %llx succeeded", immHandle);
	}

	/* Populate the ClassDescriptionGet event */
	memset(&evt, 0, sizeof(IMMSV_EVT));
	evt.type = IMMSV_EVT_TYPE_IMMND;
	evt.info.immnd.type = IMMND_EVT_A2ND_CLASS_DESCR_GET;

	evt.info.immnd.info.classDescr.className.size = strlen(className) + 1;
	evt.info.immnd.info.classDescr.className.buf = malloc(evt.info.immnd.info.classDescr.className.size);	/*alloc-0 */
	strncpy(evt.info.immnd.info.classDescr.className.buf,
		className, (size_t)evt.info.immnd.info.classDescr.className.size);

	TRACE("ClassName: %s", className);

	imma_proc_increment_pending_reply(cl_node);
	timeout = cl_node->syncr_timeout;
	cl_node = NULL;

	/* Release the CB lock Before MDS Send */
	m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
	locked = false;

	/* IMMND GOES DOWN */
	if (false == cb->is_immnd_up) {
		rc = SA_AIS_ERR_TRY_AGAIN;
		TRACE_3("ERR_TRY_AGAIN: IMMND is DOWN");
		goto mds_send_fail;
	}

	/* send the request to the IMMND */
	TRACE("ClassDescrGet 3");
	proc_rc = imma_mds_msg_sync_send(cb->imma_mds_hdl, &cb->immnd_mds_dest, &evt, &out_evt, timeout);

	switch (proc_rc) {
	case NCSCC_RC_SUCCESS:
		break;
	case NCSCC_RC_REQ_TIMOUT:
		rc = imma_proc_check_stale(cb, immHandle, SA_AIS_ERR_TIMEOUT);
		goto mds_send_fail;
	default:
		rc = SA_AIS_ERR_LIBRARY;
		TRACE_4("ERR_LIBRARY: MDS returned unexpected error code %u", proc_rc);
		goto mds_send_fail;
	}

	if (out_evt) {
		/* Process the outcome, this was a blocking call. */
		osafassert(out_evt->type == IMMSV_EVT_TYPE_IMMA);
		if (out_evt->info.imma.type == IMMA_EVT_ND2A_IMM_ERROR) {
			rc = out_evt->info.imma.info.errRsp.error;
			osafassert(rc != SA_AIS_OK);
		} else {
			osafassert(out_evt->info.imma.type == IMMA_EVT_ND2A_CLASS_DESCR_GET_RSP);
			int noOfAttributes = 0;
			int i = 0;
			SaImmAttrDefinitionT_2 **attr = NULL;
			size_t attrDataSize = 0;

			*classCategory = out_evt->info.imma.info.classDescr.classCategory;
			IMMSV_ATTR_DEF_LIST *p = out_evt->info.imma.info.classDescr.attrDefinitions;

			while (p) {
				++noOfAttributes;
				p = p->next;
			}

			attrDataSize = sizeof(SaImmAttrDefinitionT_2 *) * (noOfAttributes + 1);
			attr = malloc(attrDataSize);	/*alloc-1 */
			TRACE("Alloc attrdefs array:%p", attr);
			memset(attr, 0, attrDataSize);
			p = out_evt->info.imma.info.classDescr.attrDefinitions;
			for (; i < noOfAttributes; i++) {
				IMMSV_ATTR_DEF_LIST *prev = p;
				IMMSV_ATTR_DEFINITION *q = &(p->d);
				attr[i] = malloc(sizeof(SaImmAttrDefinitionT_2));	/*alloc-2 */
				attr[i]->attrName = malloc(q->attrName.size + 1);	/*alloc-3 */
				strncpy(attr[i]->attrName, (const char *)q->attrName.buf, q->attrName.size + 1);
				attr[i]->attrName[q->attrName.size] = 0;
				attr[i]->attrValueType = q->attrValueType;
				attr[i]->attrFlags = q->attrFlags;
				/* attr[i]->attrNtfId = q->attrNtfId; */

				/*free-Y */
				free(q->attrName.buf);
				q->attrName.buf = NULL;
				q->attrName.size = 0;

				if (!q->attrDefaultValue) {
					attr[i]->attrDefaultValue = 0;
				} else {
					int size = 0;
					switch (q->attrValueType) {
						case SA_IMM_ATTR_SAINT32T:	/*intended fall through */
						case SA_IMM_ATTR_SAUINT32T:
							size = sizeof(SaInt32T);
							break;

						case SA_IMM_ATTR_SAINT64T:	/*intended fall through */
						case SA_IMM_ATTR_SAUINT64T:
							size = sizeof(SaInt64T);
							break;

						case SA_IMM_ATTR_SATIMET:
							size = sizeof(SaTimeT);
							break;

						case SA_IMM_ATTR_SAFLOATT:
							size = sizeof(SaFloatT);
							break;

						case SA_IMM_ATTR_SADOUBLET:
							size = sizeof(SaDoubleT);
							break;

						case SA_IMM_ATTR_SASTRINGT:	/*A bit harsh with osafassert here ? */
							osafassert(strlen((char *)q->attrDefaultValue->val.x.buf)
								<= q->attrDefaultValue->val.x.size);
							size = sizeof(SaStringT);
							break;

						case SA_IMM_ATTR_SANAMET:
							osafassert(q->attrDefaultValue->val.x.size <= SA_MAX_NAME_LENGTH);
							size = sizeof(SaNameT);
							break;

						case SA_IMM_ATTR_SAANYT:
							size = sizeof(SaAnyT);
							break;
						default:
							abort();
					}	/*switch */

					SaNameT *namep = NULL;
					SaAnyT *anyp = NULL;
					SaStringT *strp = NULL;
					SaImmAttrValueT copyv;
					copyv = (SaImmAttrValueT)	/*alloc-4 */
						malloc(size);
					switch (q->attrValueType) {
						case SA_IMM_ATTR_SAINT32T:
							*((SaInt32T *)copyv) = q->attrDefaultValue->val.saint32;
							break;
						case SA_IMM_ATTR_SAUINT32T:
							*((SaUint32T *)copyv) = q->attrDefaultValue->val.sauint32;
							break;
						case SA_IMM_ATTR_SAINT64T:
							*((SaInt64T *)copyv) = q->attrDefaultValue->val.saint64;
							break;
						case SA_IMM_ATTR_SAUINT64T:
							*((SaUint64T *)copyv) = q->attrDefaultValue->val.sauint64;
							break;
						case SA_IMM_ATTR_SATIMET:
							/* I once got a segv on the line below.
							   Allignement problem ? */
							*((SaTimeT *)copyv) = q->attrDefaultValue->val.satime;
							break;
						case SA_IMM_ATTR_SAFLOATT:
							*((SaFloatT *)copyv) = q->attrDefaultValue->val.safloat;
							break;
						case SA_IMM_ATTR_SADOUBLET:
							*((SaDoubleT *)copyv) = q->attrDefaultValue->val.sadouble;
							break;

						case SA_IMM_ATTR_SASTRINGT:
							strp = (SaStringT *)copyv;
							*strp =	/*alloc-5 */
								malloc(q->attrDefaultValue->val.x.size);
							memcpy(*strp, q->attrDefaultValue->val.x.buf,
								q->attrDefaultValue->val.x.size);
							break;

						case SA_IMM_ATTR_SANAMET:
							namep = (SaNameT *)copyv;
							memset(namep, 0, sizeof(SaNameT));
							namep->length = strnlen(q->attrDefaultValue->val.x.buf,
								q->attrDefaultValue->val.x.size);
							osafassert(namep->length <= SA_MAX_NAME_LENGTH);
							memcpy(namep->value, q->attrDefaultValue->val.x.buf, namep->length);
							break;

						case SA_IMM_ATTR_SAANYT:
							anyp = (SaAnyT *)copyv;
							memset(anyp, 0, sizeof(SaAnyT));
							anyp->bufferSize = q->attrDefaultValue->val.x.size - 1;
							anyp->bufferAddr = (SaUint8T *)
								malloc(anyp->bufferSize);	/*alloc-5 */
							memcpy(anyp->bufferAddr, q->attrDefaultValue->val.x.buf,
								anyp->bufferSize);
							break;
						default:
							abort();
					}/*switch */

					attr[i]->attrDefaultValue = copyv;
					/*Delete source attr-value */
					immsv_evt_free_att_val(q->attrDefaultValue, q->attrValueType);

					free(q->attrDefaultValue);
					q->attrDefaultValue = NULL;
				}
				p = p->next;
				prev->next = NULL;
				free(prev);
				prev=NULL;
			}
			attr[noOfAttributes] = 0;

			*attrDefinition = attr;
			/*Will return a 0 terminated array of pointers to defs */

		}/*if (out_evtt->info.imma.type == IMMA_EVT_ND2A_IMM_ERROR){}else{ */

		/*For better performance we should follow the pattern of searchNext
		   and avoid copying the attribute values and keep the reply struct
		   and free it in ClassDescriptionMemoryFree.
		   But ClassDescriptionGet is not expected to be invoked often.
		 */

		/* We dont need the lock here */

		free(out_evt);
		out_evt=NULL;
	}
	/*if (out_evt) */
 mds_send_fail:
	if (evt.info.immnd.info.classDescr.className.buf) {	/*free-0 */
		free(evt.info.immnd.info.classDescr.className.buf);
		evt.info.immnd.info.classDescr.className.buf = NULL;
	}

	if (!locked && m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
		rc = SA_AIS_ERR_LIBRARY;
		TRACE_4("ERR_LIBRARY: Lock failed");
		/* Losing track of the pending reply count, but ERR_LIBRARY dominates*/
		goto lock_fail1;
	}
	locked = true;

	imma_client_node_get(&cb->client_tree, &immHandle, &cl_node);	
	if (!(cl_node && cl_node->isOm)) {
		/*rc = SA_AIS_ERR_BAD_HANDLE;*/
		/* Let the result reflect if the class descr get succeeded or not. */
		TRACE_3("client_node_get failed");
		goto  client_not_found;
	}

	imma_proc_decrement_pending_reply(cl_node);

	if((rc == SA_AIS_ERR_BAD_HANDLE) && cl_node->stale){
		/* BAD_HANDLE from imma_proc_check_stale */
		cl_node->exposed = true;
	}

 client_not_found:
 stale_handle:
	if (locked)
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);

 lock_fail1:

       if ( out_evt && rc == SA_AIS_ERR_LIBRARY && *attrDefinition ) {
		SaImmAttrDefinitionT_2 **attr1 = *attrDefinition;
		int i;
		for (i = 0; attr1[i]; ++i) {
			if (attr1[i]->attrDefaultValue) {
				imma_freeAttrValue3(attr1[i]->attrDefaultValue, attr1[i]->attrValueType);  /* free-4, free-5 */	
				attr1[i]->attrDefaultValue = 0;
			}
			free(attr1[i]->attrName);      /*free-3 */
			attr1[i]->attrName = 0;
			free(attr1[i]);        /*free-2 */
			attr1[i] = 0;
		}
		free(attr1);   /*free-1 */
	}
	
 lock_fail:

	TRACE_LEAVE();
	return rc;
}

SaAisErrorT saImmOmClassDescriptionMemoryFree_2(SaImmHandleT immHandle, SaImmAttrDefinitionT_2 **attrDefinition)
{
	IMMA_CB *cb = &imma_cb;
	IMMA_CLIENT_NODE *cl_node = NULL;
	TRACE_ENTER();

	if (cb->sv_id == 0) {
		TRACE_2("ERR_BAD_HANDLE: No initialized handle exists!");
		return SA_AIS_ERR_BAD_HANDLE;
	}

	/* get the CB Lock */
	if (m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
		TRACE_4("ERR_LIBRARY: Lock failed");
		TRACE_LEAVE();
		return SA_AIS_ERR_LIBRARY;
	}

	imma_client_node_get(&cb->client_tree, &immHandle, &cl_node);
	if (!(cl_node && cl_node->isOm)) {
		TRACE_2("ERR_BAD_HANDLE: Client not found");
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
		TRACE_LEAVE();
		return SA_AIS_ERR_BAD_HANDLE;
	}

	if (cl_node->stale) {
		TRACE_1("IMM Handle %llx is stale - ignoring", immHandle);
		/*return SA_AIS_ERR_BAD_HANDLE;*/
		/* Dont let a stale handle prevent the deallocation. */
	}

	if (attrDefinition) {
		int i;
		for (i = 0; attrDefinition[i]; ++i) {
			if (attrDefinition[i]->attrDefaultValue) {
				imma_freeAttrValue3(attrDefinition[i]->attrDefaultValue, attrDefinition[i]->attrValueType);  /* free-4, free-5 */	
				attrDefinition[i]->attrDefaultValue = 0;
			}
			free(attrDefinition[i]->attrName);	/*free-3 */
			attrDefinition[i]->attrName = 0;
			free(attrDefinition[i]);	/*free-2 */
			attrDefinition[i] = 0;
		}
		free(attrDefinition);	/*free-1 */
	}

	m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
	TRACE_LEAVE();
	return SA_AIS_OK;
}

SaAisErrorT saImmOmClassDelete(SaImmHandleT immHandle, const SaImmClassNameT className)
{
	SaAisErrorT rc = SA_AIS_OK;
	bool locked = true;
	IMMA_CB *cb = &imma_cb;
	IMMSV_EVT evt;
	IMMSV_EVT *out_evt = NULL;
	IMMA_CLIENT_NODE *cl_node = NULL;
	SaTimeT timeout = 0;
	TRACE_ENTER();

	if (cb->sv_id == 0) {
		TRACE_2("ERR_BAD_HANDLE: No initialized handle exists!");
		return SA_AIS_ERR_BAD_HANDLE;
	}

	if (className == NULL) {
		return SA_AIS_ERR_INVALID_PARAM;
	}

	if (cb->is_immnd_up == false) {
		TRACE_3("ERR_TRY_AGAIN: IMMND is DOWN");
		return SA_AIS_ERR_TRY_AGAIN;
	}	

	/* get the CB Lock */
	if (m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
		rc = SA_AIS_ERR_LIBRARY;
		TRACE_4("ERR_LIBRARY: Lock failed");
		goto lock_fail;
	}
	/*locked is true already */

	imma_client_node_get(&cb->client_tree, &immHandle, &cl_node);
	if (!(cl_node && cl_node->isOm)) {
		TRACE_2("ERR_BAD_HANDLE: Client node is missing");
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto client_not_found;
	}

	if (cl_node->stale) {
		TRACE_1("IMM Handle %llx is stale", immHandle);
		bool resurrected = imma_om_resurrect(cb, cl_node, &locked);

		if (!locked && m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
			TRACE_4("ERR_LIBRARY: Lock failed");
			rc = SA_AIS_ERR_LIBRARY;
			goto lock_fail;
		}
		locked = true;

		imma_client_node_get(&cb->client_tree, &immHandle, &cl_node);

		if (!resurrected || !cl_node || !(cl_node->isOm) || cl_node->stale) {
			TRACE_3("ERR_BAD_HANDLE: Reactive ressurect of handle %llx failed", immHandle);
			if (cl_node && cl_node->stale) {cl_node->exposed = true;}
			rc = SA_AIS_ERR_BAD_HANDLE;
			goto stale_handle;
		}

		TRACE_1("Reactive resurrect of handle %llx succeeded", immHandle);
	}

	timeout = cl_node->syncr_timeout;

	/* Populate the ClassDelete event */
	memset(&evt, 0, sizeof(IMMSV_EVT));
	evt.type = IMMSV_EVT_TYPE_IMMND;
	evt.info.immnd.type = IMMND_EVT_A2ND_CLASS_DELETE;

	evt.info.immnd.info.classDescr.className.size = strlen(className) + 1;
	evt.info.immnd.info.classDescr.className.buf = malloc(evt.info.immnd.info.classDescr.className.size);	/*alloc-1 */
	strncpy(evt.info.immnd.info.classDescr.className.buf, className,
		(size_t)evt.info.immnd.info.classDescr.className.size);

	imma_proc_increment_pending_reply(cl_node);

	rc = imma_evt_fake_evs(cb, &evt, &out_evt, timeout, cl_node->handle, &locked, true);

	if (rc != SA_AIS_OK) {
		goto mds_send_fail;
	}

	if (out_evt) {
		osafassert(out_evt->type == IMMSV_EVT_TYPE_IMMA);
		osafassert(out_evt->info.imma.type == IMMA_EVT_ND2A_IMM_ERROR);
		rc = out_evt->info.imma.info.errRsp.error;
		TRACE("Return code:%u", rc);

		/* We dont need the lock here */

		free(out_evt);
		out_evt=NULL;
	}

 mds_send_fail:
	free(evt.info.immnd.info.classDescr.className.buf);
	evt.info.immnd.info.classDescr.className.buf = NULL;
	evt.info.immnd.info.classDescr.className.size = 0;

	if (!locked && m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
		rc = SA_AIS_ERR_LIBRARY;
		TRACE_4("ERR_LIBRARY: Lock failed");
		goto lock_fail;
	}

        locked = true;

	imma_client_node_get(&cb->client_tree, &immHandle, &cl_node);	
	if (!(cl_node && cl_node->isOm)) {
		/*rc = SA_AIS_ERR_BAD_HANDLE;*/
		/* Let the result reflect if the class delete succeeded or not. */
		TRACE_3("client_node_get failed");
		goto  client_not_found;
	}

	imma_proc_decrement_pending_reply(cl_node);

	if ((rc == SA_AIS_ERR_BAD_HANDLE) && cl_node->stale) {
		/* BAD_HANDLE from imma_proc_check_stale */
		/* ABT: We could possibly convert back to TIMEOUT to allow a resurrect?*/
		cl_node->exposed = true;
	}

 stale_handle:
 client_not_found:
	if (locked)
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);

 lock_fail:
	TRACE_LEAVE();
	return rc;
}

SaAisErrorT saImmOmAccessorInitialize(SaImmHandleT immHandle, SaImmAccessorHandleT *accessorHandle)
{

	SaAisErrorT rc = SA_AIS_OK;
	uint32_t proc_rc = NCSCC_RC_SUCCESS;
	bool locked = true;
	IMMA_CB *cb = &imma_cb;
	IMMA_CLIENT_NODE *cl_node = NULL;
	IMMA_SEARCH_NODE *search_node = NULL;
	TRACE_ENTER();

	if (cb->sv_id == 0) {
		TRACE_2("ERR_BAD_HANDLE: No initialized handle exists!");
		return SA_AIS_ERR_BAD_HANDLE;
	}

	if (accessorHandle == NULL) {
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
		goto release_cb;
	}
	/*locked is true already */

	imma_client_node_get(&cb->client_tree, &immHandle, &cl_node);
	if (!(cl_node && cl_node->isOm)) {
		TRACE_2("ERR_BAD_HANDLE: Client node is missing");
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto release_lock;
	}

	if (cl_node->stale) {
		TRACE_1("IMM Handle %llx is stale", immHandle);
		bool resurrected = imma_om_resurrect(cb, cl_node, &locked);

		if (!locked && m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
			TRACE_4("ERR_LIBRARY: Lock failed");
			rc = SA_AIS_ERR_LIBRARY;
			goto release_cb;
		}
		locked = true;

		imma_client_node_get(&cb->client_tree, &immHandle, &cl_node);

		if (!resurrected || !cl_node || !(cl_node->isOm) || cl_node->stale) {
			TRACE_3("ERR_BAD_HANDLE: Reactive ressurect of handle %llx failed", immHandle);
			if (cl_node && cl_node->stale) {cl_node->exposed = true;}
			rc = SA_AIS_ERR_BAD_HANDLE;
			goto release_lock;
		}

		TRACE_1("Reactive resurrect of handle %llx succeeded", immHandle);
	}

	/*Create search-node & handle */
	search_node = (IMMA_SEARCH_NODE *)calloc(1, sizeof(IMMA_SEARCH_NODE));
	if (!search_node) {
		rc = SA_AIS_ERR_NO_MEMORY;
		goto release_lock;
	}

	search_node->search_hdl = (SaImmSearchHandleT)m_NCS_GET_TIME_NS;
	search_node->mImmHandle = immHandle;
	/*This is the external handle that the application uses.
	   Internally we use the searchId provided by the Node Director. */

	/* Add IMMA_SEARCH_NODE to search_tree */
	do {
		proc_rc = imma_search_node_add(&cb->search_tree, search_node);

		if (proc_rc != NCSCC_RC_SUCCESS) {
			IMMA_SEARCH_NODE *search_node_tmp = NULL;
			imma_search_node_get(&cb->search_tree, &(search_node->search_hdl), &search_node_tmp);
			if(!search_node_tmp) {
				LOG_NO("Failed to add search node to search tree - aborting");
				abort();
			}
			search_node->search_hdl++;
			TRACE_4("Duplicate search handle %llu (poor clock resolution) adjusting it to %llu",
				search_node_tmp->search_hdl, search_node->search_hdl);
		}
	}  while (proc_rc != NCSCC_RC_SUCCESS);

	*accessorHandle = search_node->search_hdl;

 release_lock:
	if (locked) {
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
	}

 release_cb:
	TRACE_LEAVE();
	return rc;
}

SaAisErrorT saImmOmAccessorFinalize(SaImmAccessorHandleT accessorHandle)
{
	SaAisErrorT rc = SA_AIS_OK;
	bool locked = true;
	IMMA_CB *cb = &imma_cb;
	IMMA_SEARCH_NODE *search_node = NULL;
	uint32_t proc_rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER();

	if (cb->sv_id == 0) {
		TRACE_2("ERR_BAD_HANDLE: No initialized handle exists!");
		return SA_AIS_ERR_BAD_HANDLE;
	}

	/* get the CB Lock */
	if (m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
		rc = SA_AIS_ERR_LIBRARY;
		TRACE_4("ERR_LIBRARY: Lock error");
		locked = false;
		goto release_cb;
	}
	/* locked == true already */

	imma_search_node_get(&cb->search_tree, &accessorHandle, &search_node);

	if (!search_node) {
		TRACE_2("ERR_BAD_HANDLE: Search node is missing");
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto release_lock;
	}

	if (search_node->mSearchId) {
		/*Finalize as a searchHandle */
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
		locked = false;
		/*NOTE: We return directly in the next statement. */
		return saImmOmSearchFinalize((SaImmSearchHandleT)accessorHandle);

	} else {
		/*Search handle was apparently never used, just remove the node. */
		proc_rc = imma_search_node_delete(cb, search_node);
		search_node = NULL;
		if (proc_rc != NCSCC_RC_SUCCESS) {
			TRACE_4("ERR_LIBRARY: Could not delete search node");
			rc = SA_AIS_ERR_LIBRARY;
		}
	}

 release_lock:
	if (locked) {
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
	}

 release_cb:
	TRACE_LEAVE();
	return rc;
}

SaAisErrorT saImmOmAccessorGet_2(SaImmAccessorHandleT accessorHandle,
				 const SaNameT *objectName,
				 const SaImmAttrNameT *attributeNames, SaImmAttrValuesT_2 ***attributes)
{
	SaAisErrorT rc = SA_AIS_OK;
	bool locked = true;
	IMMA_CB *cb = &imma_cb;
	IMMA_SEARCH_NODE *search_node = NULL;
	SaNameT redundantName;

	if (cb->sv_id == 0) {
		TRACE_2("ERR_BAD_HANDLE: No initialized handle exists!");
		return SA_AIS_ERR_BAD_HANDLE;
	}

	if ((objectName == NULL) || (objectName->length == 0) || (objectName->length >= SA_MAX_NAME_LENGTH)) {
		TRACE_2("ERR_INVALID_PARAM: Incorrect parameter contents: objectName");
		return SA_AIS_ERR_INVALID_PARAM;
	}

	if (!attributes) {
		TRACE_2("ERR_INVALID_PARAM: attributes is NULL");
		return SA_AIS_ERR_INVALID_PARAM;
	}

	/* get the CB Lock */
	if (m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
		rc = SA_AIS_ERR_LIBRARY;
		TRACE_4("ERR_LIBRARY: Lock error");
		goto release_cb;
	}
	/*locked is true already */

	imma_search_node_get(&cb->search_tree, &accessorHandle, &search_node);

	if (!search_node) {
		TRACE_2("ERR_BAD_HANDLE: Search node is missing");
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto release_lock;
	}

	SaImmHandleT immHandle = search_node->mImmHandle;

	/* Stale check etc done in searchInit & searchNext below. */

	/* Release locks and cb for normal cases as it uses SearchInit & 
	   SearchNext.
	 */
	m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
	locked = false;

	/*The accessorHandle is actually just a searchhandle in this 
	   implementation. 
	   SaImmSearchHandleT searchHandle = (SaImmSearchHandleT) accessorHandle;
	 */

	if (search_node->mSearchId) { /* TODO: Use of search_node not safe (unlocked) */
		TRACE("Closing previous allocation :%u\n", search_node->mSearchId);

		/*Perform a search-next, simply to discard any previous allocation. */
		/*Cant do a searchFinalize since that will discard the node. */

		redundantName.length = 0;
		m_IMMSV_SET_SANAMET((&redundantName));

		rc = saImmOmSearchNext_2(accessorHandle, &redundantName, attributes);

		if (rc != SA_AIS_ERR_NOT_EXIST && rc != SA_AIS_ERR_BAD_HANDLE) {
			TRACE_4("ERR_LIBRARY: Unexpected return code from internal searchNext: %u", rc);
			return SA_AIS_ERR_LIBRARY;	/*No handles to close here. */
		}

		/* Close the previous search id.
		   Cant use saImmOmSearchfinalize, because this will also close the 
		   search handle and deallocate the search_node.
		 */

		/* NOTE/TODO: Should Send a ImmOmSearchFinalize message to IMMND !! 
		   This to discard the search on the server side. See searchFinalize. */
		search_node->mSearchId = 0;
	}

	rc = saImmOmSearchInitialize_2(immHandle, objectName,	/*root == object */
				       SA_IMM_ONE,	/* Normally illegal in search */
				       0, NULL, attributeNames, &accessorHandle);

	if (rc != SA_AIS_OK) {
		search_node->mSearchId = 0;
		return rc;
	}

	redundantName.length = 0;
	m_IMMSV_SET_SANAMET((&redundantName));
	rc = saImmOmSearchNext_2(accessorHandle, &redundantName, attributes);
	return rc;

	/*error cases only */
 release_lock:
	if (locked) {
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
	}

 release_cb:

	return rc;
}

static unsigned int get_att_val_size(IMMSV_EDU_ATTR_VAL *p, SaImmValueTypeT t)
{
	switch (t) {
		case SA_IMM_ATTR_SAINT32T:
		case SA_IMM_ATTR_SAUINT32T:
			return sizeof(SaUint32T);
		case SA_IMM_ATTR_SAINT64T:
		case SA_IMM_ATTR_SAUINT64T:
			return sizeof(SaUint64T);
		case SA_IMM_ATTR_SATIMET:
			return sizeof(SaTimeT);
		case SA_IMM_ATTR_SAFLOATT:
			return sizeof(SaFloatT);
		case SA_IMM_ATTR_SADOUBLET:
			return sizeof(SaDoubleT);

		case SA_IMM_ATTR_SANAMET:
		case SA_IMM_ATTR_SASTRINGT:
		case SA_IMM_ATTR_SAANYT:
			return (p->val.x.size + 4);
	}

	abort();
}

static unsigned int get_attr_list_size(IMMSV_EDU_ATTR_VAL_LIST *al, const SaImmValueTypeT t)
{
	unsigned int size = 0;
	while (al) {
		size += (get_att_val_size(&(al->n), t)  + 1);
		al = al->next;
	}

	return size;
}

static unsigned int get_obj_size(const IMMSV_OM_OBJECT_SYNC* batch)
{
	TRACE_ENTER();
	unsigned int obj_size = (batch->className.size + 4);
	obj_size+= (batch->objectName.size + 4);

	IMMSV_ATTR_VALUES_LIST *avl = batch->attrValues;
	while(avl) {
		obj_size += (avl->n.attrName.size + 4 + 1); /* string + size + next-marker */

		if(avl->n.attrValuesNumber) {
			obj_size += get_att_val_size(&(avl->n.attrValue), avl->n.attrValueType);
			if (avl->n.attrValuesNumber > 1) {
				obj_size += get_attr_list_size(avl->n.attrMoreValues,
					avl->n.attrValueType);
			}
		}
		avl=avl->next;
	}

	/* Add about 20% overhead */
	obj_size = (obj_size * 119) / 100;
	TRACE("Object size: %u", obj_size);

	TRACE_LEAVE();
	return obj_size;
}



SaAisErrorT immsv_sync(SaImmHandleT immHandle, const SaImmClassNameT className,
	const SaNameT *objectName, const SaImmAttrValuesT_2 **attrValues, void** batchp,
	int* remainingSpacep, int objsInBatch)
{
	SaAisErrorT rc = SA_AIS_OK;
	IMMA_CB *cb = &imma_cb;
	IMMSV_EVT evt;
	IMMA_CLIENT_NODE *cl_node = NULL;
	bool locked = false;
	IMMSV_OM_OBJECT_SYNC* batch=NULL;
	IMMSV_OM_OBJECT_SYNC* tmp = NULL;
	TRACE_ENTER2("remainingSpace %d objsInBatch:%u", *remainingSpacep, objsInBatch);

	if (cb->sv_id == 0) {
		TRACE_2("ERR_BAD_HANDLE: No initialized handle exists!");
		return SA_AIS_ERR_BAD_HANDLE;
	}

	if ((className == NULL) || (objectName == NULL) || (attrValues == NULL && batchp == NULL)) {
		LOG_ER("(className == NULL) || (objectName == NULL) || (attrValues == NULL && batchp == NULL)");
		abort();
	}

	/*
	  Cases:
	  (A) (attrValues == NULL && batchp != NULL) => send the batch. Client iteration complete.

	  (B) (attrValues != NULL && batchp == NULL) => send a single object (old nonbatched variant).

	  (C) (attrValues != NULL && batchp != NULL) => add to the batch and send if batch full.
	*/

	if(batchp != NULL) {
		osafassert(remainingSpacep);
		batch = (*(IMMSV_OM_OBJECT_SYNC **)batchp);
	}

	memset(&evt, 0, sizeof(IMMSV_EVT));
	evt.type = IMMSV_EVT_TYPE_IMMND;
	evt.info.immnd.type = IMMND_EVT_A2ND_OBJ_SYNC_2;

	if(attrValues == NULL) {
		/* Case A */
		if(!batch) {
			LOG_ER("batch is NULL");
			abort();
		}
		/* Copy top object from batch onto EVT */
		tmp = batch;
		batch = batch->next;
		evt.info.immnd.info.obj_sync.className.size = tmp->className.size;
		tmp->className.size = 0;
		evt.info.immnd.info.obj_sync.className.buf = tmp->className.buf;
		tmp->className.buf = NULL;
		evt.info.immnd.info.obj_sync.objectName.size = tmp->objectName.size;
		tmp->objectName.size = 0;
		evt.info.immnd.info.obj_sync.objectName.buf = tmp->objectName.buf;
		tmp->objectName.buf = NULL;
		evt.info.immnd.info.obj_sync.attrValues = tmp->attrValues;
		tmp->attrValues = NULL;
		evt.info.immnd.info.obj_sync.next = tmp->next;
		tmp->next = NULL;
		free(tmp);
		goto send_batch;
	}

	/* (attrValues != NULL) Case B or C */

	osafassert((objectName->length != 0) && (objectName->length < SA_MAX_NAME_LENGTH));

	evt.info.immnd.info.obj_sync.className.size = strlen(className) + 1;

	/*alloc-1 */
	evt.info.immnd.info.obj_sync.className.buf = malloc(evt.info.immnd.info.obj_sync.className.size);
	strncpy(evt.info.immnd.info.obj_sync.className.buf, className, evt.info.immnd.info.obj_sync.className.size);

	evt.info.immnd.info.obj_sync.objectName.size = strlen((char *)objectName->value) + 1;

	if (objectName->length + 1 < evt.info.immnd.info.obj_sync.objectName.size) {
		evt.info.immnd.info.obj_sync.objectName.size = objectName->length + 1;
	}

	/*alloc-2 */
	evt.info.immnd.info.obj_sync.objectName.buf = malloc(evt.info.immnd.info.obj_sync.objectName.size);
	strncpy(evt.info.immnd.info.obj_sync.objectName.buf,
		(char *)objectName->value, evt.info.immnd.info.obj_sync.objectName.size);
	evt.info.immnd.info.obj_sync.objectName.buf[evt.info.immnd.info.obj_sync.objectName.size - 1] = '\0';

	osafassert(evt.info.immnd.info.obj_sync.attrValues == NULL);
	const SaImmAttrValuesT_2 *attr;
	int i;
	for (i = 0; attrValues[i]; ++i) {
		attr = attrValues[i];

		if (attr->attrValuesNumber == 0) {
			/*TRACE("Attribute without values DN:%s ATT:%s, skipped in sync",
			  objectName->value, attr->attrName);*/
			continue;
		}

		/*alloc-3 */
		IMMSV_ATTR_VALUES_LIST *p = calloc(1, sizeof(IMMSV_ATTR_VALUES_LIST));

		p->n.attrName.size = strlen(attr->attrName) + 1;
		if (p->n.attrName.size >= SA_MAX_NAME_LENGTH) {
			TRACE_2("ERR_INVALID_PARAM: Attribute name too long: %u", p->n.attrName.size);
			rc = SA_AIS_ERR_INVALID_PARAM;
			free(p);
			p=NULL;
			goto free_data;
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

		p->next = evt.info.immnd.info.obj_sync.attrValues;	/* NULL initially */
		evt.info.immnd.info.obj_sync.attrValues = p;
	}

	evt.info.immnd.info.obj_sync.next = batch;

	/* still Case B or C */

	if(batchp) {
		if((objsInBatch - 2) > IMMSV_MAX_OBJS_IN_SYNCBATCH) {
			TRACE("Limit for # of objects in batch reached: %d", objsInBatch);
			*remainingSpacep = 0;
		} else {
			(*remainingSpacep) -= get_obj_size(&evt.info.immnd.info.obj_sync);
		}
		TRACE("Remaining space:%d", *remainingSpacep);

		if((*remainingSpacep) > 0)
		{ 
			/* Case C only. */
			/* There is more space. Push EVT onto batch & get ready for more objects */
			IMMSV_OM_OBJECT_SYNC* tmp = calloc(1, sizeof(IMMSV_OM_OBJECT_SYNC));		
			tmp->next = batch;
			(*batchp) = tmp;

			tmp->className.size = evt.info.immnd.info.obj_sync.className.size;
			evt.info.immnd.info.obj_sync.className.size=0;
			tmp->className.buf = evt.info.immnd.info.obj_sync.className.buf;
			evt.info.immnd.info.obj_sync.className.buf = NULL;

			tmp->objectName.size = evt.info.immnd.info.obj_sync.objectName.size;
			evt.info.immnd.info.obj_sync.objectName.size = 0;
			tmp->objectName.buf = evt.info.immnd.info.obj_sync.objectName.buf;
			evt.info.immnd.info.obj_sync.objectName.buf = NULL;

			tmp->attrValues = evt.info.immnd.info.obj_sync.attrValues;
			evt.info.immnd.info.obj_sync.attrValues = NULL;
			TRACE_LEAVE();
			return SA_AIS_ERR_NOT_READY; /* Not an error, flags object was buffered */
		} else {
			(*batchp) = NULL; /* Send consumes the batch */
			
		}
	}

	/* back to Case B or C */

 send_batch:
	/* Case A or B or C */

	/* BEGIN temporary 4.1 code */
	if(objsInBatch == 1) {
		evt.info.immnd.type = IMMND_EVT_A2ND_OBJ_SYNC;
		osafassert(evt.info.immnd.info.obj_sync.next == NULL);
	}
	/* END temporary 4.1 code */

	(*remainingSpacep) = 0;

	/* get the CB Lock */
	if (m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
		rc = SA_AIS_ERR_LIBRARY;
		TRACE_4("ERR_LIBRARY: Lock failed");
		goto free_data;
	}
	locked = true;

	imma_client_node_get(&cb->client_tree, &immHandle, &cl_node);
	if (!(cl_node && cl_node->isOm)) {
		TRACE_2("ERR_BAD_HANDLE: Client node is missing");
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto free_data;
	}

	if (cl_node->stale) {
		TRACE_3("ERR_BAD_HANDLE: IMM Handle %llx is stale", immHandle);
		/* Dont bother resurrecting. This is a sync operation !! */
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto free_data;
	}

	rc = imma_evt_fake_evs(cb, &evt, NULL, 0, cl_node->handle, &locked, false);

 free_data:
	/*We may be un-locked here but this should not matter.
	   We are freing heap objects that should only be vissible from this
	   thread. */

	tmp = &(evt.info.immnd.info.obj_sync);

	do {

		if (tmp->className.buf) {	/*free-1 */
			free(tmp->className.buf);
			tmp->className.buf = NULL;
		}

		if (tmp->objectName.buf) {	/*free-2 */
			free(tmp->objectName.buf);
			tmp->objectName.buf = NULL;
		}

		while (tmp->attrValues) {
			IMMSV_ATTR_VALUES_LIST *p = tmp->attrValues;
			tmp->attrValues = p->next;
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

		/* Free heap allocated objects, but not the top (stack allocated) EVT object */
		if(tmp == evt.info.immnd.info.obj_sync.next) {
			evt.info.immnd.info.obj_sync.next = tmp->next;
			tmp->next =NULL;
			free(tmp);
		}
		tmp = evt.info.immnd.info.obj_sync.next;
		
	} while (tmp);


	if (locked)
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);

	TRACE_LEAVE();
	return rc;
}

SaAisErrorT immsv_finalize_sync(SaImmHandleT immHandle)
{
	SaAisErrorT rc = SA_AIS_OK;
	uint32_t proc_rc = NCSCC_RC_SUCCESS;
	IMMA_CB *cb = &imma_cb;
	IMMSV_EVT finalize_evt;
	IMMSV_EVT *out_evt = NULL;
	IMMA_CLIENT_NODE *cl_node = 0;
	bool locked = true;
	SaUint32T timeout = 0;
	TRACE_ENTER();

	if (cb->sv_id == 0) {
		TRACE_2("ERR_BAD_HANDLE: No initialized handle exists!");
		return SA_AIS_ERR_BAD_HANDLE;
	}

	if (m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
		TRACE_4("ERR_LIBRARY: Lock failed");
		rc = SA_AIS_ERR_LIBRARY;
		goto lock_fail;
	}
	/* locked == true already */

	imma_client_node_get(&cb->client_tree, &immHandle, &cl_node);

	if (!(cl_node && cl_node->isOm)) {
		rc = SA_AIS_ERR_LIBRARY;
		TRACE_4("ERR_LIBRARY: Missing client node");
		goto client_not_found;
	}

	if (cl_node->stale) {
		TRACE_3("ERR_BAD_HANDLE: IMM Handle %llx is stale", immHandle);
		/* Dont bother resurrecting. This is a sync operation !! */
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto stale_handle;
	}

	/* populate the structure */
	memset(&finalize_evt, 0, sizeof(IMMSV_EVT));
	finalize_evt.type = IMMSV_EVT_TYPE_IMMND;
	finalize_evt.info.immnd.type = IMMND_EVT_A2ND_SYNC_FINALIZE;
	finalize_evt.info.immnd.info.finReq.client_hdl = cl_node->handle;

	timeout = cl_node->syncr_timeout;
	cl_node = NULL;
	/* Unlock before MDS Send */
	m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
	locked = false;

	/* IMMND GOES DOWN */
	if (cb->is_immnd_up == false) {
		rc = SA_AIS_ERR_TRY_AGAIN;
		TRACE_3("ERR_TRY_AGAIN: IMMND is DOWN");
		goto mds_send_fail;
	}

	/* send the request to the IMMND */
	proc_rc = imma_mds_msg_sync_send(cb->imma_mds_hdl,
					 &(cb->immnd_mds_dest), &finalize_evt, &out_evt, timeout);

	/* MDS error handling */
	switch (proc_rc) {
		case NCSCC_RC_SUCCESS:
			break;
		case NCSCC_RC_REQ_TIMOUT:
			/* No point in checking for stale here, since if the IMMND did
			   go down it had to be the coord immnd as that is the only IMMND
			   receiving the finalize sync order. If the IMMND went down we 
			   (the sync process) has to die anyway.
			*/
			rc = SA_AIS_ERR_TIMEOUT;
			goto mds_send_fail;
		default:
			TRACE_4("ERR_LIBRARY: MDS returned unexpected error code %u", proc_rc);
			rc = SA_AIS_ERR_LIBRARY;
			goto mds_send_fail;
	}

	if (out_evt) {
		/* Process the outcome, note this is a blocking op. */
		osafassert(out_evt->type == IMMSV_EVT_TYPE_IMMA);
		osafassert(out_evt->info.imma.type == IMMA_EVT_ND2A_IMM_ERROR);
		rc = out_evt->info.imma.info.errRsp.error;
		if (rc != SA_AIS_OK) {
			TRACE_1("Returned error: %u", rc);
		}

		free(out_evt);
		out_evt=NULL;
	}

 mds_send_fail:
 stale_handle:
 client_not_found:

	if (locked)
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);

 lock_fail:
	/* Release the CB handle */
	TRACE_LEAVE();
	return rc;
}

SaAisErrorT saImmOmSearchInitialize_2(SaImmHandleT immHandle,
				      const SaNameT *rootName,
				      SaImmScopeT scope,
				      SaImmSearchOptionsT searchOptions,
				      const SaImmSearchParametersT_2 *searchParam,
				      const SaImmAttrNameT *attributeNames, SaImmSearchHandleT *searchHandle)
{
	SaAisErrorT rc = SA_AIS_OK;
	uint32_t proc_rc = NCSCC_RC_SUCCESS;
	bool locked = true;
	bool isAccessor = false;
	IMMA_CB *cb = &imma_cb;
	IMMSV_EVT evt;
	IMMSV_EVT *out_evt = NULL;
	IMMA_CLIENT_NODE *cl_node = NULL;
	IMMA_SEARCH_NODE *search_node = NULL;
	SaImmSearchHandleT tmpSearchHandle=0LL;
	SaUint32T timeout = 0;
	TRACE_ENTER();

	if (cb->sv_id == 0) {
		TRACE_2("ERR_BAD_HANDLE: No initialized handle exists!");
		return SA_AIS_ERR_BAD_HANDLE;
	}

	if (searchHandle == NULL) {
		TRACE_2("ERR_INVALID_PARAM: Invalid search handle");
		TRACE_LEAVE();
		return SA_AIS_ERR_INVALID_PARAM;
	}

	if (cb->is_immnd_up == false) {
		TRACE_3("ERR_TRY_AGAIN: IMMND is DOWN");
		return SA_AIS_ERR_TRY_AGAIN;
	}

	if ((searchOptions & SA_IMM_SEARCH_GET_SOME_ATTR) && (attributeNames == NULL)) {
		TRACE_2("ERR_INVALID_PARAM: SA_IMM_SEARCH_GET_SOME_ATTR is set & "
			"AttributeName is NULL");
		TRACE_LEAVE();
		return SA_AIS_ERR_INVALID_PARAM;
	}

	if ((searchParam) && ((searchParam->searchOneAttr.attrName == NULL) && 
		(searchParam->searchOneAttr.attrValue != NULL))) {
		TRACE_2("ERR_INVALID_PARAM: attrName is NULL but attrValue is not NULL");
		TRACE_LEAVE();
		return SA_AIS_ERR_INVALID_PARAM;
	}

	/* get the CB Lock */
	if (m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
		rc = SA_AIS_ERR_LIBRARY;
		TRACE_4("ERR_LIBRARY: Lock failed");
		goto release_cb;
	}
	/*locked is true already */

	imma_client_node_get(&cb->client_tree, &immHandle, &cl_node);
	if (!(cl_node && cl_node->isOm)) {
		TRACE_2("ERR_BAD_HANDLE: Client node is missing");
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto release_lock;
	}

	if (cl_node->stale) {
		TRACE_1("IMM Handle %llx is stale", immHandle);
		bool resurrected = imma_om_resurrect(cb, cl_node, &locked);
		cl_node = NULL;

		if (!locked && m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
			TRACE_4("ERR_LIBRARY: Lock failed");
			rc = SA_AIS_ERR_LIBRARY;
			goto release_cb;
		}
		locked = true;

		imma_client_node_get(&cb->client_tree, &immHandle, &cl_node);

		if (!resurrected || !cl_node || !(cl_node->isOm) || cl_node->stale) {
			TRACE_3("ERR_BAD_HANDLE: Reactive ressurect of handle %llx failed", immHandle);
			if (cl_node && cl_node->stale) {cl_node->exposed = true;}
			rc = SA_AIS_ERR_BAD_HANDLE;
			goto release_lock;
		}

		TRACE_1("Reactive resurrect of handle %llx succeeded", immHandle);
	}

	if ((scope == SA_IMM_ONE) && (*searchHandle) && !searchParam && !searchOptions) {	/*Special ACCESSOR case */
		TRACE("Special accessor case:%llx\n", *searchHandle);
		isAccessor = true;
		searchOptions = SA_IMM_SEARCH_ONE_ATTR |
			(attributeNames ? SA_IMM_SEARCH_GET_SOME_ATTR : SA_IMM_SEARCH_GET_ALL_ATTR);

		/*Look up search handle */
		imma_search_node_get(&cb->search_tree, searchHandle, &search_node);
		if ((!search_node) || (search_node->mImmHandle != immHandle)) {
			TRACE_2("ERR_BAD_HANDLE: Search node is missing");
			rc = SA_AIS_ERR_BAD_HANDLE;
			goto release_lock;
		}
	} else {		/*Normal case */
		*searchHandle = 0LL;

		if ((scope != SA_IMM_SUBLEVEL) && (scope != SA_IMM_SUBTREE)) {
			TRACE_2("ERR_IVALID_PARAM: Invalid scope parameter");
			rc = SA_AIS_ERR_INVALID_PARAM;
			goto release_lock;
		}

		if(attributeNames && (!(searchOptions & SA_IMM_SEARCH_GET_SOME_ATTR))) {
			TRACE_2("ERR_IVALID_PARAM: attributeNames != NULL yet searchOptions set to IMM_SEARCH_GET_SOME_ATTR");
			rc = SA_AIS_ERR_INVALID_PARAM;
			goto release_lock;
		}

		/*Create search-node & handle   */
		search_node = (IMMA_SEARCH_NODE *)
		    calloc(1, sizeof(IMMA_SEARCH_NODE));
		if (!search_node) {
			rc = SA_AIS_ERR_NO_MEMORY;
			goto release_lock;
		}

		search_node->search_hdl = (SaImmSearchHandleT)m_NCS_GET_TIME_NS;
		search_node->mImmHandle = immHandle;


		/* Add IMMA_SEARCH_NODE to search_tree */
		do {
			proc_rc = imma_search_node_add(&cb->search_tree, search_node);

			if (proc_rc != NCSCC_RC_SUCCESS) {
				IMMA_SEARCH_NODE *search_node_tmp = NULL;
				imma_search_node_get(&cb->search_tree, &(search_node->search_hdl), &search_node_tmp);
				if(!search_node_tmp) {
					LOG_NO("Failed to add search node to search tree - aborting");
					abort();
				}
				search_node->search_hdl++;
				TRACE_4("Duplicate search handle %llu (poor clock resolution) adjusting it to %llu",
					search_node_tmp->search_hdl, search_node->search_hdl);
			}
		} while (proc_rc != NCSCC_RC_SUCCESS);
	}

	/* Populate the SearchInit event */
	memset(&evt, 0, sizeof(IMMSV_EVT));
	evt.type = IMMSV_EVT_TYPE_IMMND;
	evt.info.immnd.type = IMMND_EVT_A2ND_SEARCHINIT;
	IMMSV_OM_SEARCH_INIT *req = &(evt.info.immnd.info.searchInit);
	req->client_hdl = immHandle;
	if (rootName && rootName->length && (rootName->length < SA_MAX_NAME_LENGTH)) {
		req->rootName.size = strlen((char *)rootName->value) + 1;
		if (rootName->length + 1 < req->rootName.size)
			req->rootName.size = rootName->length + 1;
		req->rootName.buf = malloc(req->rootName.size);	/* alloc-1 */
		strncpy(req->rootName.buf, (char *)rootName->value, (size_t)req->rootName.size);
		req->rootName.buf[req->rootName.size - 1] = 0;
	} else {
		req->rootName.size = 0;
		req->rootName.buf = NULL;
	}

	req->scope = scope;
	req->searchOptions = searchOptions;
	if (!searchParam || (!searchParam->searchOneAttr.attrName)) {
		req->searchParam.present = ImmOmSearchParameter_PR_NOTHING;
	} else {
		req->searchParam.present = ImmOmSearchParameter_PR_oneAttrParam;
		req->searchParam.choice.oneAttrParam.attrName.size = strlen(searchParam->searchOneAttr.attrName) + 1;
		req->searchParam.choice.oneAttrParam.attrName.buf =	/*alloc-2 */
			malloc(req->searchParam.choice.oneAttrParam.attrName.size);
		strncpy(req->searchParam.choice.oneAttrParam.attrName.buf,
			(char *)searchParam->searchOneAttr.attrName,
			(size_t)req->searchParam.choice.oneAttrParam.attrName.size);
		req->searchParam.choice.oneAttrParam.attrName.buf[req->searchParam.choice.oneAttrParam.attrName.size -
								  1] = 0;
		if (searchParam->searchOneAttr.attrValue) {
			req->searchParam.choice.oneAttrParam.attrValueType = searchParam->searchOneAttr.attrValueType;
			/*alloc-3 */
			imma_copyAttrValue(&req->searchParam.choice.oneAttrParam.attrValue,
					   searchParam->searchOneAttr.attrValueType,
					   searchParam->searchOneAttr.attrValue);
		} else {
			/* Encoding NO VALUE as a string of zero length. */
			req->searchParam.choice.oneAttrParam.attrValueType = SA_IMM_ATTR_SASTRINGT;
			req->searchParam.choice.oneAttrParam.attrValue.val.x.size = 0;
			req->searchParam.choice.oneAttrParam.attrValue.val.x.buf = NULL;
		}
	}

	if (attributeNames) {
		const SaImmAttrNameT *namev;
		for (namev = attributeNames; *namev; namev++) {
			IMMSV_ATTR_NAME_LIST *p = (IMMSV_ATTR_NAME_LIST *)
			    malloc(sizeof(IMMSV_ATTR_NAME_LIST));	/*alloc-4 */
			p->name.size = strlen(*namev) + 1;
			p->name.buf = malloc(p->name.size);	/*alloc-5 */
			strncpy(p->name.buf, *namev, p->name.size);
			p->name.buf[p->name.size - 1] = 0;

			p->next = req->attributeNames;	/*NULL in first iteration */
			req->attributeNames = p;
		}
	}

	if (rootName && rootName->length) {
		TRACE("root: %s param:%p", rootName->value, searchParam);
	}

	imma_proc_increment_pending_reply(cl_node);
	tmpSearchHandle = search_node->search_hdl;
	timeout = cl_node->syncr_timeout;

	/* Release the CB lock Before MDS Send */
	m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
	locked = false;
	cl_node = NULL;
	search_node = NULL;

	/* IMMND GOES DOWN */
	if (cb->is_immnd_up == false) {
		rc = SA_AIS_ERR_TRY_AGAIN;
		TRACE_3("ERR_TRY_AGAIN: IMMND is DOWN");
		goto mds_send_fail;
	}

	/* send the request to the IMMND */
	proc_rc = imma_mds_msg_sync_send(cb->imma_mds_hdl, &cb->immnd_mds_dest, &evt, &out_evt, timeout);

	switch (proc_rc) {
		case NCSCC_RC_SUCCESS:
			break;

		case NCSCC_RC_REQ_TIMOUT:
			rc = imma_proc_check_stale(cb, immHandle, SA_AIS_ERR_TIMEOUT);
			goto mds_send_fail;

		default:
			rc = SA_AIS_ERR_LIBRARY;
			TRACE_4("ERR_LIBRARY: MDS returned unexpected error code %u", proc_rc);
			goto mds_send_fail;
	}

	if (out_evt) {
		/*search_node->mLastResult = 0;  -- already zeroed */
		/*search_node->mLastAttributes = 0; -- already zeroed */
		osafassert(out_evt->type == IMMSV_EVT_TYPE_IMMA);
		osafassert(out_evt->info.imma.type == IMMA_EVT_ND2A_SEARCHINIT_RSP);
		rc = out_evt->info.imma.info.searchInitRsp.error;
	} else {
		TRACE_4("ERR_LIBRARY: Empty return message from IMMND");
		rc = SA_AIS_ERR_LIBRARY;
	}

 mds_send_fail:
	osafassert(req);
	if (req->rootName.buf) {
		free(req->rootName.buf);	/*free-1 */
		req->rootName.buf = NULL;
		req->rootName.size = 0;
	}
	if (req->searchParam.present == ImmOmSearchParameter_PR_oneAttrParam) {
		free(req->searchParam.choice.oneAttrParam.attrName.buf);	/*free-2 */
		req->searchParam.choice.oneAttrParam.attrName.buf = NULL;
		req->searchParam.choice.oneAttrParam.attrName.size = 0;
		if (searchParam->searchOneAttr.attrValue) {	/*free-3 */
			immsv_evt_free_att_val(&(req->searchParam.choice.oneAttrParam.attrValue),
					       searchParam->searchOneAttr.attrValueType);
		}
	}

	if (req->attributeNames) {	/* free-4 & free-5 */
		immsv_evt_free_attrNames(req->attributeNames);
		req->attributeNames = NULL;
	}

	if (m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
		rc = SA_AIS_ERR_LIBRARY;
		TRACE_4("ERR_LIBRARY: Lock error");
		goto lock_fail;
	}
	locked = true;

	imma_client_node_get(&cb->client_tree, &immHandle, &cl_node);
	if (!(cl_node && cl_node->isOm)) {
		TRACE_3("ERR_BAD_HANDLE: Client node is gone after down call");
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto release_lock;
	}

	imma_proc_decrement_pending_reply(cl_node);
	
	imma_search_node_get(&cb->search_tree, &tmpSearchHandle, &search_node);
	if ((!search_node) || (search_node->mImmHandle != immHandle)) {
		TRACE_3("ERR_BAD_HANDLE: Search node is missing after down call");
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto release_lock;
	}

	if (cl_node->stale) {
		if (isExposed(cb, cl_node)) {
			TRACE_2("ERR_BAD_HANDLE: client is stale after down call and exposed");
			rc = SA_AIS_ERR_BAD_HANDLE;
		} else {
			TRACE_2("ERR_TRY_AGAIN: Handle %llx is stale after down call but "
				"possible to resurrect", immHandle);
			/* Can override BAD_HANDLE/TIMEOUT set in check_stale */
			rc = SA_AIS_ERR_TRY_AGAIN;
		}
	}

	if (rc == SA_AIS_OK) {
		osafassert(out_evt);
		search_node->mSearchId = out_evt->info.imma.info.searchInitRsp.searchId;
		if (*searchHandle) {
			osafassert(*searchHandle == search_node->search_hdl);
		} else {
			*searchHandle = search_node->search_hdl;
		}
	} else {
		TRACE_3("Search initialize failed:%u", rc);
		if (!isAccessor) {
			proc_rc = imma_search_node_delete(cb, search_node);
			search_node = NULL;	/*Node was removed from tree AND freed. */
			if (proc_rc != NCSCC_RC_SUCCESS) {
				TRACE_4("ERR_LIBRARY: Could not delete search node");
				rc = SA_AIS_ERR_LIBRARY;
			}
		}
	}

	if (rc != SA_AIS_OK && search_node != NULL && !isAccessor) {
		/*Node never added to tree */
		free(search_node);
	}

 release_lock:
	if (locked) {
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
	}
lock_fail:
	if(out_evt) {
		free(out_evt);
		out_evt=NULL;
	}

 release_cb:
	TRACE_LEAVE();
	return rc;
}

SaAisErrorT saImmOmSearchNext_2(SaImmSearchHandleT searchHandle, SaNameT *objectName, SaImmAttrValuesT_2 ***attributes)
{
	SaAisErrorT error = SA_AIS_OK;
	uint32_t proc_rc = NCSCC_RC_SUCCESS;
	bool locked = true;
	IMMA_CB *cb = &imma_cb;
	IMMSV_EVT evt;
	IMMSV_EVT *out_evt = NULL;
	IMMA_CLIENT_NODE *cl_node = NULL;
	IMMA_SEARCH_NODE *search_node = NULL;
	SaImmHandleT immHandle=0LL;
	SaUint32T timeout = 0;

	if (cb->sv_id == 0) {
		TRACE_2("ERR_BAD_HANDLE: No initialized handle exists!");
		return SA_AIS_ERR_BAD_HANDLE;
	}

	if (!objectName) {
		TRACE_2("ERR_INVALID_PARAM: Invalid parameter: objectName");
		return SA_AIS_ERR_INVALID_PARAM;
	}

	if (!attributes) {
		TRACE_2("ERR_INVALID_PARAM: Invalid parameter: attributes");
		return SA_AIS_ERR_INVALID_PARAM;
	}

	/* get the CB Lock */
	if (m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
		error = SA_AIS_ERR_LIBRARY;
		TRACE_4("ERR_LIBRARY: Lock failed");
		locked = false;
		goto release_cb;
	}
	/*locked is true already */

	/*Look up search node */
	imma_search_node_get(&cb->search_tree, &searchHandle, &search_node);

	if (!search_node) {
		TRACE_2("ERR_BAD_HANDLE: Search node is missing");
		error = SA_AIS_ERR_BAD_HANDLE;
		goto release_lock;
	}

	immHandle = search_node->mImmHandle;

	imma_client_node_get(&cb->client_tree, &immHandle, &cl_node);

	if (!(cl_node && cl_node->isOm)) {
		error = SA_AIS_ERR_LIBRARY;
		TRACE_4("ERR_LIBRARY: Invalid SaImmHandleT related to search handle");
		goto release_lock;
	}

	if (cl_node->stale) {
		TRACE_3("ERR_BAD_HANDLE: IMM Handle %llx is stale", immHandle);
		cl_node->exposed = true;
		error = SA_AIS_ERR_BAD_HANDLE;
		/* We dont want to resurrect here because we know the search has failed. */
		goto release_lock;
	}

	if (search_node->mLastAttributes) {
		imma_freeSearchAttrs((SaImmAttrValuesT_2 **)search_node->mLastAttributes);
		search_node->mLastAttributes = 0;
	}

	if (search_node->mSearchId == 0) {
		TRACE_3("ERR_BAD_HANDLE: Search id is zero");
		error = SA_AIS_ERR_BAD_HANDLE;
		goto release_lock;
	}

	memset(&evt, 0, sizeof(IMMSV_EVT));
	evt.type = IMMSV_EVT_TYPE_IMMND;
	evt.info.immnd.type = IMMND_EVT_A2ND_SEARCHNEXT;
	evt.info.immnd.info.searchOp.searchId = search_node->mSearchId;
	evt.info.immnd.info.searchOp.client_hdl = immHandle;

	imma_proc_increment_pending_reply(cl_node);

	timeout = cl_node->syncr_timeout;

	/* Release the CB lock Before MDS Send */
	m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
	locked = false;
	cl_node = NULL;
	search_node = NULL;

	/* IMMND GOES DOWN */
	if (cb->is_immnd_up == false) {
		error = SA_AIS_ERR_TRY_AGAIN;
		TRACE_3("ERR_TRY_AGAIN: IMMND is DOWN");
		goto release_cb;
	}

	/* send the request to the IMMND */
	proc_rc = imma_mds_msg_sync_send(cb->imma_mds_hdl, &cb->immnd_mds_dest, &evt, &out_evt, timeout);

	switch (proc_rc) {
		case NCSCC_RC_SUCCESS:
			break;

		case NCSCC_RC_REQ_TIMOUT:
			error = imma_proc_check_stale(cb, immHandle, SA_AIS_ERR_TIMEOUT);
			break;
		
		default:
			error = SA_AIS_ERR_LIBRARY;
			TRACE_4("ERR_LIBRARY: MDS returned unexpected error code %u", proc_rc);
			break;
	}

	if (m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
		TRACE_4("ERR_LIBRARY: Lock error");
		error = SA_AIS_ERR_LIBRARY;
		goto release_evt;
	}
	locked = true;

	imma_client_node_get(&cb->client_tree, &immHandle, &cl_node);
	if (!(cl_node && cl_node->isOm)) {
		error = SA_AIS_ERR_LIBRARY;
		TRACE_4("ERR_LIBRARY: Invalid SaImmHandleT related to search handle");
		goto release_evt;
	}

	imma_proc_decrement_pending_reply(cl_node);

	imma_search_node_get(&cb->search_tree, &searchHandle, &search_node);
	if (!search_node) {
		TRACE_3("ERR_BAD_HANDLE: Search node is gone after down-call");
		error = SA_AIS_ERR_BAD_HANDLE;
		goto release_evt;
	}

	if (cl_node->stale) {
		cl_node->exposed = true;
		error = SA_AIS_ERR_BAD_HANDLE;		
		TRACE_3("ERR_BAD_HANDLE: client is stale and exposed");
		/* 
		   Unfortunately, searchNext is one of the few IMM API operations that is
		   the most difficult to provide transparency for. Even more unfortunate is
		   that we dont have any suitable return code to communicate that only the
		   searchHandle is BAD. The principle is that the immHandle is not exposed
		   as long as we can avoid returning BAD_HANDLE for an operation using it.
		   But here we have to return BAD_HANDLE for an operation using a sub-handle
		   (the search handle) to the innHandle. 

		   We can compare with CCBs and ccbHandles. For a ccb operation we can always
		   return FAILED_OPERATION which just indicates that the CCB is failed/aborted, 
		   without giving away the whole game that the immHandle may be stale.
		   But there is no corresponding return code for searchNext. 

		   For searchInitialize and accessor operations, we can hide the stalenes
		   of a handle by returning TRY_AGAIN, which gives resurrect a chance to make
		   a retry truly possible. But we can not return a TRY_AGAIN on a searchNext
		   to mask a stale handle, because a searchNext is a continuation operation.
		   What we need is a return code telling the user to restart the iteration 
		   from scratch (re initialize). There is no such return code. 

		   TIMEOUT and NO_RESOURCES could be returned here, but the user is not likely to 
		   react to these codes by restarting the iteration, despite that this would
		   be an allowed reaction and quite likley to succeed when it was just a stale
		   immHandle that was resurrect'able. 
		*/
	}

	if (out_evt && error == SA_AIS_OK) {
		int noOfAttributes = 0;
		int i = 0;
		size_t attrDataSize = 0;
		IMMSV_OM_RSP_SEARCH_NEXT *res_body = NULL;
		SaImmAttrValuesT_2 **attr = NULL;

		osafassert(out_evt->type == IMMSV_EVT_TYPE_IMMA);
		if (out_evt->info.imma.type == IMMA_EVT_ND2A_IMM_ERROR) {
			error = out_evt->info.imma.info.errRsp.error;
			osafassert(error && (error != SA_AIS_OK));
			free(out_evt);	/*BUGFIX (leak) 090506 */
			out_evt = NULL;
			goto release_lock;
		}
		osafassert(out_evt->info.imma.type == IMMA_EVT_ND2A_SEARCHNEXT_RSP);

		res_body = out_evt->info.imma.info.searchNextRsp;
		osafassert(res_body);

		objectName->length = 0;
		m_IMMSV_SET_SANAMET(objectName);
		objectName->length = strnlen(res_body->objectName.buf, res_body->objectName.size);
		osafassert(objectName->length <= SA_MAX_NAME_LENGTH);
		memcpy(objectName->value, res_body->objectName.buf, objectName->length);

		IMMSV_ATTR_VALUES_LIST *p = res_body->attrValuesList;
		while (p) {
			++noOfAttributes;
			p = p->next;
		}

		attrDataSize = sizeof(SaImmAttrValuesT_2 *) * (noOfAttributes + 1);
		attr = calloc(1, attrDataSize);	/*alloc-1 */
		p = res_body->attrValuesList;
		for (; i < noOfAttributes; i++, p = p->next) {
			IMMSV_ATTR_VALUES *q = &(p->n);
			attr[i] = calloc(1, sizeof(SaImmAttrValuesT_2));	/*alloc-2 */
			attr[i]->attrName = malloc(q->attrName.size + 1);	/*alloc-3 */
			strncpy(attr[i]->attrName, (const char *)q->attrName.buf, q->attrName.size + 1);
			attr[i]->attrName[q->attrName.size] = 0;	/*redundant. */
			attr[i]->attrValuesNumber = q->attrValuesNumber;
			attr[i]->attrValueType = (SaImmValueTypeT)q->attrValueType;

			if (q->attrValuesNumber) {
				/*SaNameT* namep;
				   SaAnyT*  anyp;
				   SaStringT* strp;
				   Allocate the array of pointers, even if only one value
				 */
				attr[i]->attrValues = calloc(1, q->attrValuesNumber * sizeof(SaImmAttrValueT));	/*alloc-4 */
				/*alloc-5 */
				attr[i]->attrValues[0] = imma_copyAttrValue3(q->attrValueType, &(q->attrValue));

				if (q->attrValuesNumber > 1) {
					int ix;
					IMMSV_EDU_ATTR_VAL_LIST *r = q->attrMoreValues;
					for (ix = 1; ix < q->attrValuesNumber; ++ix) {
						osafassert(r);
						attr[i]->attrValues[ix] = imma_copyAttrValue3(q->attrValueType, &(r->n));/*alloc-5 */
						r = r->next;
					}
				}
			}
		}
		attr[noOfAttributes] = NULL;
		*attributes = attr;
		search_node->mLastAttributes = attr;

		/* ABT We must be carefull only to deallocate the client_node & search_node,
		   to protect the search_node->mLastAttributes structure from being removed
		   while the user is still using it (lock released).
		*/
	}

 release_evt:

	if (out_evt) {
		/* Free the search_next structure. */
		if (out_evt->info.imma.info.searchNextRsp) {
			free(out_evt->info.imma.info.searchNextRsp->objectName.buf);
			out_evt->info.imma.info.searchNextRsp->objectName.buf = NULL;
			out_evt->info.imma.info.searchNextRsp->objectName.size = 0;
			immsv_free_attrvalues_list(out_evt->info.imma.info.searchNextRsp->attrValuesList);
			out_evt->info.imma.info.searchNextRsp->attrValuesList = NULL;
			free(out_evt->info.imma.info.searchNextRsp);
			out_evt->info.imma.info.searchNextRsp = NULL;
		}
		free(out_evt);
		out_evt=NULL;
	}

 release_lock:
	if (locked) {
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
	}

 release_cb:
	return error;
}

SaAisErrorT saImmOmSearchFinalize(SaImmSearchHandleT searchHandle)
{
	SaAisErrorT error = SA_AIS_OK;
	uint32_t proc_rc = NCSCC_RC_SUCCESS;
	bool locked = true;
	IMMA_CB *cb = &imma_cb;
	IMMSV_EVT evt;
	IMMSV_EVT *out_evt = NULL;
	IMMA_CLIENT_NODE *cl_node = NULL;
	IMMA_SEARCH_NODE *search_node = NULL;
	SaImmHandleT immHandle=0LL;
	SaUint32T searchId = 0;
	SaUint32T timeout = 0;
	TRACE_ENTER();

	if (cb->sv_id == 0) {
		TRACE_2("ERR_BAD_HANDLE: No initialized handle exists!");
		return SA_AIS_ERR_BAD_HANDLE;
	}

	/* get the CB Lock */
	if (m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
		error = SA_AIS_ERR_LIBRARY;
		TRACE_4("ERR_LIBRARY: Lock failed");
		locked = false;
		goto release_cb;
	}
	/*locked is true already */

	/*Look up search node */
	imma_search_node_get(&cb->search_tree, &searchHandle, &search_node);

	if (!search_node) {
		TRACE_2("ERR_BAD_HANDLE: Search node is missing");
		error = SA_AIS_ERR_BAD_HANDLE;
		goto release_lock;
	}

	if (search_node->mLastAttributes) {
		TRACE("Freeing last result");
		imma_freeSearchAttrs((SaImmAttrValuesT_2 **)search_node->mLastAttributes);
		search_node->mLastAttributes = 0;
	}

	immHandle = search_node->mImmHandle;
	searchId = search_node->mSearchId;

	imma_client_node_get(&cb->client_tree, &immHandle, &cl_node);

	if (!(cl_node && cl_node->isOm)) {
		error = SA_AIS_ERR_LIBRARY;
		TRACE_4("ERR_LIBRARY: Invalid SaImmHandleT related to search handle");
		goto release_lock;
	}

	if (cl_node->stale) {
		TRACE_1("IMM Handle %llx is stale", immHandle);
		error = SA_AIS_OK;	/*Dont punish the client for closing stale handle */
		imma_search_node_delete(cb, search_node);
		search_node = NULL;
		goto release_lock;
	}

	memset(&evt, 0, sizeof(IMMSV_EVT));
	evt.type = IMMSV_EVT_TYPE_IMMND;
	evt.info.immnd.type = IMMND_EVT_A2ND_SEARCHFINALIZE;
	evt.info.immnd.info.searchOp.searchId = searchId;
	evt.info.immnd.info.searchOp.client_hdl = immHandle;

	/*imma_proc_increment_pending_reply(cl_node);*/
	/* A blocked SearchFinalize need not expose the client handle. */
	timeout = cl_node->syncr_timeout;
	/* Release the CB lock Before MDS Send */
	m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
	locked = false;
	cl_node = NULL;
	search_node = NULL;

	/* IMMND GOES DOWN */
	if (cb->is_immnd_up == false) {
		error = SA_AIS_ERR_TRY_AGAIN;
		TRACE_3("ERR_TRY_AGAIN: IMMND is DOWN");
		goto mds_failed;
	}

	/* send the request to the IMMND */
	proc_rc = imma_mds_msg_sync_send(cb->imma_mds_hdl, &cb->immnd_mds_dest, &evt, &out_evt, timeout);

	switch (proc_rc) {
		case NCSCC_RC_SUCCESS:
			break;

		case NCSCC_RC_REQ_TIMOUT:
			error = imma_proc_check_stale(cb, immHandle, SA_AIS_ERR_TIMEOUT);
			goto mds_failed;
			break;

		default:
			error = SA_AIS_ERR_LIBRARY;
			TRACE_4("ERR_LIBRARY: MDS returned unexpected error code %u", proc_rc);
			break;
	}

	if (out_evt) {
		osafassert(out_evt->type == IMMSV_EVT_TYPE_IMMA);
		osafassert(out_evt->info.imma.type == IMMA_EVT_ND2A_IMM_ERROR);
		if (error == SA_AIS_OK) {
			error = out_evt->info.imma.info.errRsp.error;
		}
		free(out_evt);
		out_evt=NULL;

		if(error == SA_AIS_ERR_BAD_HANDLE) {
			/* Because this is finalize, BAD_HANDLE from the server is
			   here hidden from the client. But remove the search-node from
			   library. BAD_HANDLE detected in the library is not hidden 
			   from the client. */
			error = SA_AIS_OK;
		}

		if(error == SA_AIS_OK) {/* i.e. not TRY_AGAIN or TIMEOUT */
			if(!locked) {
				if (m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
					TRACE_4("ERR_LIBRARY: Lock failed");
					error = SA_AIS_ERR_LIBRARY;
					goto release_cb;
				}
				locked = true;
			}
			imma_search_node_get(&cb->search_tree, &searchHandle, &search_node);
			if(search_node) {
				imma_search_node_delete(cb, search_node);
				search_node = NULL;
			}
		}
	}

 mds_failed:
 release_lock:
	if (locked) {
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
	}

 release_cb:
	TRACE_LEAVE();
	return error;
}

SaAisErrorT saImmOmAdminOwnerSet(SaImmAdminOwnerHandleT adminOwnerHandle,
				 const SaNameT **objectNames, SaImmScopeT scope)
{
	SaAisErrorT rc = SA_AIS_OK;
	IMMA_CB *cb = &imma_cb;
	IMMSV_EVT admo_set_evt;
	IMMSV_EVT *out_evt = NULL;
	IMMA_CLIENT_NODE *cl_node = 0;
	IMMA_ADMIN_OWNER_NODE *ao_node = NULL;
	const SaNameT *objectName;
	bool locked = true;
	SaImmHandleT immHandle=0LL;
	SaUint32T adminOwnerId = 0;
	SaUint32T timeout = 0;
	TRACE_ENTER();

	if (cb->sv_id == 0) {
		TRACE_2("ERR_BAD_HANDLE: No initialized handle exists!");
		return SA_AIS_ERR_BAD_HANDLE;
	}

	if (!objectNames || (objectNames[0] == 0)) {
		return SA_AIS_ERR_INVALID_PARAM;
	}

	switch (scope) {
		case SA_IMM_ONE:
		case SA_IMM_SUBLEVEL:
		case SA_IMM_SUBTREE:
			break;
		default:
			return SA_AIS_ERR_INVALID_PARAM;
	}

	if (cb->is_immnd_up == false) {
		TRACE_3("ERR_TRY_AGAIN: IMMND is DOWN");
		return SA_AIS_ERR_TRY_AGAIN;
	}

	if (m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
		TRACE_4("ERR_LIBRARY: Lock failed");
		rc = SA_AIS_ERR_LIBRARY;
		goto lock_fail;
	}
	/* locked == true already */

	imma_admin_owner_node_get(&cb->admin_owner_tree, &adminOwnerHandle, &ao_node);

	if (!ao_node) {
		TRACE_2("ERR_BAD_HANDLE: Admin owner node is missing or %llx", adminOwnerHandle);
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto done;
	}

	if(ao_node->mAugCcb) {
		TRACE_2("Augmented CCB AdminOwner handle used in saImmOmAdminOwnerSet");
		/* Verify that the ccb-node exists and that it is in an acceptable state.
		   That is, the ccb is in scope of a callback and neither ccbApply or
		   ccbFinalize  has been invoked on the augmentation. See #2426
		 */
		IMMA_CCB_NODE *ccb_node = NULL;
		SaImmCcbHandleT ccbHandle = ao_node->mImmHandle; /* Same handle value used */
		imma_ccb_node_get(&cb->ccb_tree, &ccbHandle, &ccb_node);
		if (!ccb_node || 
			!(ccb_node->mAugCcb) || ccb_node->mApplied || ccb_node->mAborted) {
			TRACE_2("ERR_NO_RESOURCES: AdminOwner handle obtained from "
				"saImmOiAugmentCcbInitialize can not be used in this"
				"context.");
			rc = SA_AIS_ERR_NO_RESOURCES;
			goto done;
		}
	}

	immHandle = ao_node->mImmHandle;
	adminOwnerId = ao_node->mAdminOwnerId;
	/* Dont trust adminOwnerId just yet, resurrect possible. See *** below */
	ao_node=NULL;

	imma_client_node_get(&cb->client_tree, &immHandle, &cl_node);
	if (!(cl_node && cl_node->isOm)) {
		rc = SA_AIS_ERR_LIBRARY;
		/* This case should not be possible here.
		   If the OM-handle is closed then the admin owner should also 
		   have been closed  AUTOMATICALLY.
		   I guess we leak the admin-owner in this case. 
		 */

		TRACE_4("ERR_LIBRARY: Admin owner associated with closed client");
		goto done;
	}

	if (cl_node->stale) {
		TRACE_1("IMM Handle %llx is stale", immHandle);
		bool resurrected = imma_om_resurrect(cb, cl_node, &locked);
		cl_node = NULL;

		if (!locked && m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
			TRACE_4("ERR_LIBRARY: Lock failed");
			rc = SA_AIS_ERR_LIBRARY;
			goto done;
		}
		locked = true;

		imma_client_node_get(&cb->client_tree, &immHandle, &cl_node);

		if (!resurrected || !cl_node || !(cl_node->isOm) || cl_node->stale) {
			TRACE_3("ERR_BAD_HANDLE: Reactive ressurect of handle %llx failed", immHandle);
			if (cl_node && cl_node->stale) {cl_node->exposed = true;}
			rc = SA_AIS_ERR_BAD_HANDLE;
			goto done;
		}

		TRACE_1("Reactive resurrect of handle %llx succeeded", immHandle);

		/* Look up admin owner again since successful resurrect implies
		   new admin-owner-id ! */
		imma_admin_owner_node_get(&cb->admin_owner_tree, &adminOwnerHandle, &ao_node);
		if (!ao_node) {
			TRACE_3("ERR_BAD_HANDLE: Admin owner node dissapeared during resurrect");
			rc = SA_AIS_ERR_BAD_HANDLE;
			goto done;
		}
		TRACE_1("Admin-owner-id should have changed(?) Before: %u After: %u",
			adminOwnerId, ao_node->mAdminOwnerId);
		adminOwnerId = ao_node->mAdminOwnerId; /* *** */
		ao_node = NULL;
	}

	timeout = cl_node->syncr_timeout;

	/* populate the structure */
	memset(&admo_set_evt, 0, sizeof(IMMSV_EVT));
	admo_set_evt.type = IMMSV_EVT_TYPE_IMMND;
	admo_set_evt.info.immnd.type = IMMND_EVT_A2ND_ADMO_SET;
	admo_set_evt.info.immnd.info.admReq.adm_owner_id = adminOwnerId;
	admo_set_evt.info.immnd.info.admReq.scope = scope;

	int i;
	for (i = 0; objectNames[i]; ++i) {
		objectName = objectNames[i];
		osafassert(objectName->length < SA_MAX_NAME_LENGTH);
		IMMSV_OBJ_NAME_LIST *ol = calloc(1, sizeof(IMMSV_OBJ_NAME_LIST));	/*a */
		ol->name.size = strnlen((char *)objectName->value, SA_MAX_NAME_LENGTH) + 1;
		if (ol->name.size > objectName->length) {
			ol->name.size = objectName->length;
		}
		ol->name.buf = malloc(ol->name.size);	/*b */
		memcpy(ol->name.buf, objectName->value, ol->name.size);
		ol->next = admo_set_evt.info.immnd.info.admReq.objectNames;	/*null initially */
		admo_set_evt.info.immnd.info.admReq.objectNames = ol;
	}

	imma_proc_increment_pending_reply(cl_node);

	rc = imma_evt_fake_evs(cb, &admo_set_evt, &out_evt, timeout, cl_node->handle, &locked, true);
	cl_node =NULL;

	if (out_evt) {
		osafassert(out_evt->type == IMMSV_EVT_TYPE_IMMA);
		osafassert(out_evt->info.imma.type == IMMA_EVT_ND2A_IMM_ERROR);
		if (rc == SA_AIS_OK) {
			rc = out_evt->info.imma.info.errRsp.error;
		}
		free(out_evt);
		out_evt=NULL;
	}

	while (admo_set_evt.info.immnd.info.admReq.objectNames != NULL) {
		IMMSV_OBJ_NAME_LIST *ol = admo_set_evt.info.immnd.info.admReq.objectNames;
		admo_set_evt.info.immnd.info.admReq.objectNames = admo_set_evt.info.immnd.info.admReq.objectNames->next;

		free(ol->name.buf);	/*free-b */
		ol->name.buf = NULL;
		ol->name.size = 0;
		ol->next = NULL;
		free(ol);	/*free-a */
	}

	if (!locked && m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
		TRACE_4("ERR_LIBRARY: Lock failed");
		rc = SA_AIS_ERR_LIBRARY;
		goto lock_fail;
	}
	locked = true;

	imma_client_node_get(&cb->client_tree, &immHandle, &cl_node);
	if (!(cl_node && cl_node->isOm)) {
		rc = SA_AIS_ERR_BAD_HANDLE;
		TRACE_3("ERR_BAD_HANDLE: client_node gone after down-call");
		goto done;
	}

	imma_proc_decrement_pending_reply(cl_node);

	/* Ignore possibly stale handle, will be discovered in next op. */

 done:
	if (locked)
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);

 lock_fail:

	TRACE_LEAVE();
	return rc;
}

SaAisErrorT saImmOmAdminOwnerRelease(SaImmAdminOwnerHandleT adminOwnerHandle,
				     const SaNameT **objectNames, SaImmScopeT scope)
{
	SaAisErrorT rc = SA_AIS_OK;
	IMMA_CB *cb = &imma_cb;
	IMMSV_EVT admo_set_evt;
	IMMSV_EVT *out_evt = NULL;
	IMMA_CLIENT_NODE *cl_node = 0;
	IMMA_ADMIN_OWNER_NODE *ao_node = NULL;
	const SaNameT *objectName;
	bool locked = true;
	SaImmHandleT immHandle=0LL;
	SaUint32T adminOwnerId = 0;
	SaUint32T timeout = 0;
	TRACE_ENTER();

	if (cb->sv_id == 0) {
		TRACE_2("ERR_BAD_HANDLE: No initialized handle exists!");
		return SA_AIS_ERR_BAD_HANDLE;
	}

	if (!objectNames || (objectNames[0] == 0)) {
		return SA_AIS_ERR_INVALID_PARAM;
	}

	switch (scope) {
		case SA_IMM_ONE:
		case SA_IMM_SUBLEVEL:
		case SA_IMM_SUBTREE:
			break;
		default:
			return SA_AIS_ERR_INVALID_PARAM;
	}

	if (cb->is_immnd_up == false) {
		TRACE_3("ERR_TRY_AGAIN: IMMND is DOWN");
		return SA_AIS_ERR_TRY_AGAIN;
	}

	if (m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
		TRACE_4("ERR_LIBRARY: Lock failed");
		rc = SA_AIS_ERR_LIBRARY;
		goto lock_fail;
	}
	/* locked == true already */

	imma_admin_owner_node_get(&cb->admin_owner_tree, &adminOwnerHandle, &ao_node);
	if (!ao_node) {
		TRACE_2("ERR_BAD_HANDLE: Admin owner node is missing");
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto done;
	}

	if (ao_node->mAugCcb) {
		TRACE_2("ERR_BUSY: Admin owner handle from augmented CCB cant be "
			"used for release.");
		rc = SA_AIS_ERR_BUSY;
		goto done;
	}

	immHandle = ao_node->mImmHandle;
	adminOwnerId = ao_node->mAdminOwnerId;
	/* Dont trust adminOwnerId just yet, resurrect possible. See *** below */
	ao_node=NULL;

	imma_client_node_get(&cb->client_tree, &immHandle, &cl_node);
	if (!(cl_node && cl_node->isOm)) {
		rc = SA_AIS_ERR_LIBRARY;
		/* This case should not be possible here.
		   If the OM-handle is closed then the admin owner should also 
		   have been closed  AUTOMATICALLY.
		   I guess we leak the admin-owner in this case. 
		*/

		TRACE_4("ERR_LIBRARY: Admin owner associated with closed client");
		goto done;
	}

	if (cl_node->stale) {
		TRACE_1("IMM Handle %llx is stale", immHandle);
		bool resurrected = imma_om_resurrect(cb, cl_node, &locked);
		cl_node = NULL;

		if (!locked && m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
			TRACE_4("ERR_LIBRARY: Lock failed");
			rc = SA_AIS_ERR_LIBRARY;
			goto done;
		}
		locked = true;

		imma_client_node_get(&cb->client_tree, &immHandle, &cl_node);

		if (!resurrected || !cl_node || !(cl_node->isOm) || cl_node->stale) {
			TRACE_3("ERR_BAD_HANDLE: Reactive ressurect of handle %llx failed", immHandle);
			if (cl_node && cl_node->stale) {cl_node->exposed = true;}
			rc = SA_AIS_ERR_BAD_HANDLE;
			goto done;
		}

		TRACE_1("Reactive resurrect of handle %llx succeeded", immHandle);

		/* Look up admin owner again since successful resurrect implies
		   new admin-owner-id ! */
		imma_admin_owner_node_get(&cb->admin_owner_tree, &adminOwnerHandle, &ao_node);
		if (!ao_node) {
			TRACE_3("ERR_BAD_HANDLE: Admin owner node dissapeared during resurrect");
			rc = SA_AIS_ERR_BAD_HANDLE;
			goto done;
		}
		TRACE_1("Admin-owner-id should have changed(?) Before: %u After: %u",
			adminOwnerId, ao_node->mAdminOwnerId);
		adminOwnerId = ao_node->mAdminOwnerId; /* *** */
		ao_node = NULL;
	}

	timeout = cl_node->syncr_timeout;

	/* populate the structure */
	memset(&admo_set_evt, 0, sizeof(IMMSV_EVT));
	admo_set_evt.type = IMMSV_EVT_TYPE_IMMND;
	admo_set_evt.info.immnd.type = IMMND_EVT_A2ND_ADMO_RELEASE;
	admo_set_evt.info.immnd.info.admReq.adm_owner_id = adminOwnerId;
	admo_set_evt.info.immnd.info.admReq.scope = scope;

	int i;
	for (i = 0; objectNames[i]; ++i) {
		objectName = objectNames[i];
		osafassert(objectName->length < SA_MAX_NAME_LENGTH);
		IMMSV_OBJ_NAME_LIST *ol = calloc(1, sizeof(IMMSV_OBJ_NAME_LIST));	/*a */
		ol->name.size = strnlen((char *)objectName->value, SA_MAX_NAME_LENGTH) + 1;
		if (ol->name.size > objectName->length) {
			ol->name.size = objectName->length;
		}
		ol->name.buf = malloc(ol->name.size);	/*b */
		memcpy(ol->name.buf, objectName->value, ol->name.size);
		ol->next = admo_set_evt.info.immnd.info.admReq.objectNames;	/*null initially */
		admo_set_evt.info.immnd.info.admReq.objectNames = ol;
	}

	imma_proc_increment_pending_reply(cl_node);

	rc = imma_evt_fake_evs(cb, &admo_set_evt, &out_evt, timeout, cl_node->handle, &locked, true);
	cl_node = NULL;

	if (out_evt) {
		osafassert(out_evt->type == IMMSV_EVT_TYPE_IMMA);
		osafassert(out_evt->info.imma.type == IMMA_EVT_ND2A_IMM_ERROR);
		if (rc == SA_AIS_OK) {
			rc = out_evt->info.imma.info.errRsp.error;
		}
		free(out_evt);
		out_evt=NULL;
	}

	while (admo_set_evt.info.immnd.info.admReq.objectNames != NULL) {
		IMMSV_OBJ_NAME_LIST *ol = admo_set_evt.info.immnd.info.admReq.objectNames;
		admo_set_evt.info.immnd.info.admReq.objectNames = admo_set_evt.info.immnd.info.admReq.objectNames->next;

		free(ol->name.buf);	/*free-b */
		ol->name.buf = NULL;
		ol->name.size = 0;
		ol->next = NULL;
		free(ol);	/*free-a */
	}

	if (!locked && m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
		TRACE_4("ERR_LIBRARY: Lock failed");
		rc = SA_AIS_ERR_LIBRARY;
		goto lock_fail;
	}
	locked = true;

	imma_client_node_get(&cb->client_tree, &immHandle, &cl_node);
	if (!(cl_node && cl_node->isOm)) {
		if (rc != SA_AIS_OK) {
			/* Override all errors with BAD_HANDLE, but dont override AIS_OK 
			   because this is an admin owner release, i.e. relaxation call.
			 */
			TRACE_3("ERR_BAD_HANDLE: Client node is gone after down-call");
			rc = SA_AIS_ERR_BAD_HANDLE;
		}
		TRACE_3("client_node_get failed");
		goto done;
	}

	imma_proc_decrement_pending_reply(cl_node);

	/* Ignore possibly stale handle */


 done:
	if (locked)
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);

 lock_fail:

	TRACE_LEAVE();
	return rc;
}

SaAisErrorT saImmOmAdminOwnerClear(SaImmHandleT immHandle, const SaNameT **objectNames, SaImmScopeT scope)
{
	SaAisErrorT rc = SA_AIS_OK;
	IMMA_CB *cb = &imma_cb;
	IMMSV_EVT admo_set_evt;
	IMMSV_EVT *out_evt = NULL;
	IMMA_CLIENT_NODE *cl_node = 0;
	const SaNameT *objectName;
	bool locked = true;
	SaUint32T timeout = 0;
	TRACE_ENTER();

	if (cb->sv_id == 0) {
		TRACE_2("ERR_BAD_HANDLE: No initialized handle exists!");
		return SA_AIS_ERR_BAD_HANDLE;
	}

	if (!objectNames || (objectNames[0] == 0)) {
		return SA_AIS_ERR_INVALID_PARAM;
	}

	switch (scope) {
		case SA_IMM_ONE:
		case SA_IMM_SUBLEVEL:
		case SA_IMM_SUBTREE:
			break;
		default:
			return SA_AIS_ERR_INVALID_PARAM;
	}

	if (cb->is_immnd_up == false) {
		TRACE_3("ERR_TRY_AGAIN: IMMND is DOWN");
		return SA_AIS_ERR_TRY_AGAIN;
	}

	if (m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
		TRACE_4("ERR_LIBRARY: Lock failed");
		rc = SA_AIS_ERR_LIBRARY;
		goto lock_fail;
	}
	/* locked == true already */

	imma_client_node_get(&cb->client_tree, &immHandle, &cl_node);
	if (!(cl_node && cl_node->isOm)) {
		TRACE_2("ERR_BAD_HANDLE: Client node is missing");
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto done;
	}

	if (cl_node->stale) {
		TRACE_1("IMM Handle %llx is stale", immHandle);
		bool resurrected = imma_om_resurrect(cb, cl_node, &locked);
		cl_node = NULL;

		if (!locked && m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
			TRACE_4("ERR_LIBRARY: Lock failed");
			rc = SA_AIS_ERR_LIBRARY;
			goto done;
		}
		locked = true;

		imma_client_node_get(&cb->client_tree, &immHandle, &cl_node);

		if (!resurrected || !cl_node || !(cl_node->isOm) || cl_node->stale) {
			TRACE_3("ERR_BAD_HANDLE: Reactive ressurect of handle %llx failed", immHandle);
			if (cl_node && cl_node->stale) {cl_node->exposed = true;}
			rc = SA_AIS_ERR_BAD_HANDLE;
			goto done;
		}

		TRACE_1("Reactive resurrect of handle %llx succeeded", immHandle);
	}

	timeout = cl_node->syncr_timeout;

	/* populate the structure */
	memset(&admo_set_evt, 0, sizeof(IMMSV_EVT));
	admo_set_evt.type = IMMSV_EVT_TYPE_IMMND;
	admo_set_evt.info.immnd.type = IMMND_EVT_A2ND_ADMO_CLEAR;
	admo_set_evt.info.immnd.info.admReq.adm_owner_id = 0;
	admo_set_evt.info.immnd.info.admReq.scope = scope;

	int i;
	for (i = 0; objectNames[i]; ++i) {
		objectName = objectNames[i];
		osafassert(objectName->length < SA_MAX_NAME_LENGTH);
		IMMSV_OBJ_NAME_LIST *ol = calloc(1, sizeof(IMMSV_OBJ_NAME_LIST));	/*a */
		ol->name.size = strnlen((char *)objectName->value, SA_MAX_NAME_LENGTH) + 1;
		if (ol->name.size > objectName->length) {
			ol->name.size = objectName->length;
		}
		ol->name.buf = malloc(ol->name.size);	/*b */
		memcpy(ol->name.buf, objectName->value, ol->name.size);
		ol->next = admo_set_evt.info.immnd.info.admReq.objectNames;	/*null initially */
		admo_set_evt.info.immnd.info.admReq.objectNames = ol;
	}

	imma_proc_increment_pending_reply(cl_node);

	rc = imma_evt_fake_evs(cb, &admo_set_evt, &out_evt, timeout, cl_node->handle, &locked, true);
	cl_node = NULL;

	if (out_evt) {
		osafassert(out_evt->type == IMMSV_EVT_TYPE_IMMA);
		osafassert(out_evt->info.imma.type == IMMA_EVT_ND2A_IMM_ERROR);
		if (rc == SA_AIS_OK) {
			rc = out_evt->info.imma.info.errRsp.error;
		}
		/* We dont need the lock here  */
		free(out_evt);
		out_evt=NULL;
	}

	while (admo_set_evt.info.immnd.info.admReq.objectNames != NULL) {
		IMMSV_OBJ_NAME_LIST *ol = admo_set_evt.info.immnd.info.admReq.objectNames;
		admo_set_evt.info.immnd.info.admReq.objectNames = admo_set_evt.info.immnd.info.admReq.objectNames->next;

		free(ol->name.buf);	/*free-b */
		ol->name.buf = NULL;
		ol->name.size = 0;
		ol->next = NULL;
		free(ol);	/*free-a */
	}

	if (!locked && m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
		TRACE_4("ERR_LIBRARY: Lock failed");
		rc = SA_AIS_ERR_LIBRARY;
		goto lock_fail;
	}
	locked = true;

	imma_client_node_get(&cb->client_tree, &immHandle, &cl_node);
	if (!(cl_node && cl_node->isOm)) {
		if (rc != SA_AIS_OK) {
			/* Override all errors with BAD_HANDLE, but dont override AIS_OK 
			   because this is an admin owner clear, i.e. relaxation call.
			 */
			TRACE_3("ERR_BAD_HANDLE: client_node_get failed");
			rc = SA_AIS_ERR_BAD_HANDLE;
		}
		goto done;
	}

	imma_proc_decrement_pending_reply(cl_node);

	/* Ignore possibly stale handle */

 done:
	if (locked)
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);

 lock_fail:

	TRACE_LEAVE();
	return rc;
}

SaAisErrorT saImmOmAdminOwnerFinalize(SaImmAdminOwnerHandleT adminOwnerHandle)
{
	SaAisErrorT rc = SA_AIS_OK;
	/*uint32_t                proc_rc = NCSCC_RC_SUCCESS; */
	IMMA_CB *cb = &imma_cb;
	IMMSV_EVT finalize_evt;
	IMMSV_EVT *out_evt = NULL;
	IMMA_CLIENT_NODE *cl_node = 0;
	IMMA_ADMIN_OWNER_NODE *ao_node = NULL;
	bool locked = false;
	SaImmHandleT immHandle;
	SaUint32T adminOwnerId;
	SaUint32T timeout = 0;
	TRACE_ENTER();

	if (cb->sv_id == 0) {
		TRACE_2("ERR_BAD_HANDLE: No initialized handle exists!");
		return SA_AIS_ERR_BAD_HANDLE;
	}

	if (m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
		TRACE_4("ERR_LIBRARY: Lock failed");
		rc = SA_AIS_ERR_LIBRARY;
		goto lock_fail;
	}
	locked = true;

	imma_admin_owner_node_get(&cb->admin_owner_tree, &adminOwnerHandle, &ao_node);
	if (!ao_node) {
		TRACE_2("ERR_BAD_HANDLE: Admin owner node is missing");
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto done;
	}

	if(ao_node->mAugCcb) {
		TRACE_2("Augmented CCB AdminOwner handle ignoring admo-finalize here");
		goto done;
	}

	immHandle = ao_node->mImmHandle;
	adminOwnerId = ao_node->mAdminOwnerId;

	imma_client_node_get(&cb->client_tree, &immHandle, &cl_node);
	if (!(cl_node && cl_node->isOm)) {
		rc = SA_AIS_ERR_LIBRARY;
		/*This case should not be possible here.
		   If the OM-handle is closed then the admin owner should also 
		   have been closed  AUTOMATICALLY.
		   I guess we leak the admin-owner in this case. 
		 */

		TRACE_4("ERR_LIBRARY: Admin owner associated with closed client");
		goto done;
	}

	if (cl_node->stale) {
		TRACE_1("IMM Handle %llx is stale", immHandle);
		rc = SA_AIS_OK;	/*Dont punish the client for closing stale handle */
		imma_admin_owner_node_delete(cb, ao_node);
		ao_node = NULL;
		goto done;
	}

	timeout = cl_node->syncr_timeout;

	/* populate the structure */
	memset(&finalize_evt, 0, sizeof(IMMSV_EVT));
	finalize_evt.type = IMMSV_EVT_TYPE_IMMND;
	finalize_evt.info.immnd.type = IMMND_EVT_A2ND_ADMO_FINALIZE;
	finalize_evt.info.immnd.info.admFinReq.adm_owner_id = adminOwnerId;

	/*imma_proc_increment_pending_reply(cl_node);*/
	/*A blocked AdminOwnerFinalize need not expose the client node */

	rc = imma_evt_fake_evs(cb, &finalize_evt, &out_evt, timeout, cl_node->handle, &locked, false);
	cl_node = NULL;
	ao_node = NULL;

	if (out_evt) {
		osafassert(out_evt->type == IMMSV_EVT_TYPE_IMMA);
		osafassert(out_evt->info.imma.type == IMMA_EVT_ND2A_IMM_ERROR);
		if (rc == SA_AIS_OK) {
			rc = out_evt->info.imma.info.errRsp.error;
		}
		TRACE("AdminOwnerFinalize Internally returned: %u", rc);
		
		free(out_evt);
		out_evt=NULL;

		if(rc == SA_AIS_ERR_BAD_HANDLE) {
			/* Because this is finalize, BAD_HANDLE from the server is
			   here hidden from the client. But remove the admo-node from
			   library. BAD_HANDLE detected in the library is not hidden 
			   from the client. */
			rc = SA_AIS_OK;
		}

		if(rc == SA_AIS_OK) {/* i.e. not TRY_AGAIN or TIMEOUT */
			if(!locked) {
				if (m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
					TRACE_4("ERR_LIBRARY: Lock failed");
					rc = SA_AIS_ERR_LIBRARY;
					goto lock_fail;
				}
				locked = true;
			}
			imma_admin_owner_node_get(&cb->admin_owner_tree, &adminOwnerHandle, &ao_node);
			if(ao_node) {
				/* 
				   Any Ccb handles that are open and associated with AO are
				   closed by imma_admin_owner_node_delete. 
				   Note: Any ccb in exclusive mode is orphaned! The ccb_node
				   can then only be deallocated by an explicit saImmOmCcbFinalize
				   (when it is no longer in exclusive mode).
				   Same goes for server side. Any open ccb-ids related to the
				   admin owner are aborted, unless they are in critical.
				*/
				imma_admin_owner_node_delete(cb, ao_node);
				ao_node = NULL;
			}
		}
	}

 done:
	if (locked)
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);

 lock_fail:

	return rc;
}

SaAisErrorT saImmOmCcbFinalize(SaImmCcbHandleT ccbHandle)
{
	SaAisErrorT rc = SA_AIS_OK;
	IMMA_CB *cb = &imma_cb;
	IMMSV_EVT evt;
	IMMSV_EVT *out_evt = NULL;
	IMMA_ADMIN_OWNER_NODE *ao_node = NULL;
	IMMA_CLIENT_NODE *cl_node = NULL;
	IMMA_CCB_NODE *ccb_node = NULL;
	bool locked = false;
	bool ccbActive = false;
	SaUint32T ccbId = 0;
	SaImmHandleT immHandle = 0LL;
	SaImmAdminOwnerHandleT adminOwnerHdl = 0LL;
	SaUint32T timeout = 0;
	TRACE_ENTER();

	if (cb->sv_id == 0) {
		TRACE_2("ERR_BAD_HANDLE: No initialized handle exists!");
		return SA_AIS_ERR_BAD_HANDLE;
	}

	if (m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
		rc = SA_AIS_ERR_LIBRARY;
		TRACE_4("ERR_LIBRARY: Lock failed");
		goto lock_fail;
	}
	locked = true;

	imma_ccb_node_get(&cb->ccb_tree, &ccbHandle, &ccb_node);
	if (!ccb_node) {
		rc = SA_AIS_ERR_BAD_HANDLE;
		TRACE_2("ERR_BAD_HANDLE: Ccb handle not valid");
		goto done;
	}

	TRACE_1("CCb node found for ccbhandle %llx ccbid:%u", 
		ccbHandle, ccb_node->mCcbId);
	if (ccb_node->mExclusive) {
		rc = SA_AIS_ERR_TRY_AGAIN;
		TRACE_3("ERR_TRY_AGAIN: Ccb-id %u being created or in critical phase, in another thread",
			ccb_node->mCcbId);
		goto done;
	}

	if (ccb_node->mAugCcb) {
		if(!(ccb_node->mApplied || ccb_node->mAborted)) {
			if(ccb_node->mAugIsTainted) {
				ccb_node->mAborted = true;
			} else {
				ccb_node->mApplied = true;
			}
		}
		goto done;
	}

	immHandle = ccb_node->mImmHandle;
	adminOwnerHdl = ccb_node->mAdminOwnerHdl;
	ccbActive = ccb_node->mCcbId && !(ccb_node->mApplied);
	ccbId = ccb_node->mCcbId;

	ccb_node->mExclusive = true; 

	if (ccbActive) {
		TRACE("Ccb is active when finalizing");
		/* If the ccb is not active, then there is no CCB (id) in the server.
		   Then the CCB session (node) only exists in the IMMA library.
		   No message should be sent to the server for that case. 
		 */

		/* Get the Admin Owner info  */
		imma_admin_owner_node_get(&cb->admin_owner_tree, &adminOwnerHdl, &ao_node);
		if (!ao_node) {
			rc = SA_AIS_ERR_BAD_HANDLE;
			TRACE_3("ERR_BAD_HANDLE: Amin-Owner node associated with Ccb is missing");
			goto done;
		}

		osafassert(immHandle == ao_node->mImmHandle);
		ao_node = NULL;

		imma_client_node_get(&cb->client_tree, &immHandle, &cl_node);
		if (!(cl_node && cl_node->isOm)) {
			rc = SA_AIS_ERR_LIBRARY;
			TRACE_4("ERR_LIBRARY: No valid SaImmHandleT associated with Ccb");
			goto done;
		}

		if (cl_node->stale) {
			TRACE_1("IMM Handle %llx is stale", immHandle);
			rc = SA_AIS_OK;	/*Dont punish the client for closing stale handle */
			imma_ccb_node_delete(cb, ccb_node);
			ccb_node = NULL;
			goto done;
		}

		timeout = cl_node->syncr_timeout;

		/* Populate the CcbFinalize event */
		memset(&evt, 0, sizeof(IMMSV_EVT));
		evt.type = IMMSV_EVT_TYPE_IMMND;
		evt.info.immnd.type = IMMND_EVT_A2ND_CCB_FINALIZE;
		evt.info.immnd.info.ccbId = ccbId;

		/*imma_proc_increment_pending_reply(cl_node);*/
		/* A blocked CcbFinalize need not expose the client node. */

		rc = imma_evt_fake_evs(cb, &evt, &out_evt, timeout, cl_node->handle, &locked, false);
		cl_node = NULL;
		ccb_node = NULL;

		if (out_evt) {
			osafassert(out_evt->type == IMMSV_EVT_TYPE_IMMA);
			osafassert(out_evt->info.imma.type == IMMA_EVT_ND2A_IMM_ERROR);
			if (rc == SA_AIS_OK) {
				rc = out_evt->info.imma.info.errRsp.error;
			}
			TRACE("CcbFinalize returned %u", rc);

			free(out_evt);
			out_evt=NULL;

			if(rc == SA_AIS_ERR_BAD_HANDLE) {
				/* Because this is finalize, BAD_HANDLE from the server is
				   here hidden from the client. But remove the ccb-node from
				   library. BAD_HANDLE detected in the library is not hidden 
				   from the client. */
				rc = SA_AIS_OK;
			}
		}/*out_evt*/
	}/*ccbActive*/


	if(!locked) {
		if (m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
			TRACE_4("ERR_LIBRARY: Lock failed");
			rc = SA_AIS_ERR_LIBRARY;
			goto lock_fail;
		}
		locked = true;
	}

	imma_ccb_node_get(&cb->ccb_tree, &ccbHandle, &ccb_node);
	if(ccb_node) {
		if(rc == SA_AIS_OK) {/* i.e. not TRY_AGAIN or TIMEOUT */
			imma_ccb_node_delete(cb, ccb_node);
			ccb_node = NULL;
		} else {
			/* TRY_AGAIN or TIMEOUT => allow user to try finalize again. */
			ccb_node->mExclusive = false;
		}
	}

 done:
	if (locked)
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);

 lock_fail:

	TRACE_LEAVE();
	return rc;
}


/* 
   Internal helper function that determines if there are
   AdminOwner handles associated with a newly resurrected client.
   The function attempts to re-attach the AdminOwner handle.

   For now, we cope with at most ONE admin-owner handle.
   This should cover 99% of all cases. If there is more than
   one admin-owner then this function fails => resurrection must
   be reverted and stale+exposed must be set on client node.

   cb must NOT be locked on entry.
*/
static SaBoolT imma_re_initialize_admin_owners(IMMA_CB *cb, SaImmHandleT immHandle)
{
	SaUint32T proc_rc = NCSCC_RC_SUCCESS;
	SaAisErrorT err = SA_AIS_OK;
	unsigned int sleep_delay_ms = 200;
	unsigned int max_waiting_time_ms = 1 * 1000;	/* 1 secs */
	unsigned int msecs_waited = 0;
	SaImmAdminOwnerHandleT temp_hdl, *temp_ptr = NULL;
	IMMA_ADMIN_OWNER_NODE *adm_node = NULL;
	IMMA_ADMIN_OWNER_NODE *adm_found_node = NULL;
	IMMSV_EVT evt;
	IMMSV_EVT *out_evt = NULL;
	SaUint32T nameLen = 0;
	IMMA_CLIENT_NODE *cl_node = NULL;
	bool locked = false;
	SaUint32T timeout = 0;
	TRACE_ENTER();

	if (m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
		TRACE_3("Lock failure");
		goto fail;
	}
	locked = true;

	/* Scan the entire Adm Owner DB and find the handles opened by this client */
	while ((adm_node = (IMMA_ADMIN_OWNER_NODE *)
		ncs_patricia_tree_getnext(&cb->admin_owner_tree, (uint8_t *)temp_ptr))) {
		temp_hdl = adm_node->admin_owner_hdl;
		temp_ptr = &temp_hdl;

		if (adm_node->mImmHandle == immHandle) {
			/* We have a match */
			if (adm_found_node) {
				/* We have more than one match. For now we handle re-attach
				   of at most one admin-owner.
				 */
				goto fail;
			} else {
				adm_found_node = adm_node;
				if (adm_node->mReleaseOnFinalize) {
					/* Will not re-attach if admin-owner had
					   release on finalize set. 
					 */
					TRACE_1("Cant re-attach admin-owner %s because "
						"releaseOnFinalize was set", adm_node->mAdminOwnerName);
					goto fail;
				}
			}
		}
	}

	if (!adm_found_node) {
		TRACE_1("No admin owners associated with handle %llx", immHandle);
		goto success;
	}

	imma_client_node_get(&cb->client_tree, &immHandle, &cl_node);
	if (!cl_node || !(cl_node->isOm) || cl_node->stale) {
		TRACE_3("client_node_get failed");
		goto fail;
	}

	nameLen = strlen(adm_found_node->mAdminOwnerName);

	/* Populate & Send the Event to IMMND */
	memset(&evt, 0, sizeof(IMMSV_EVT));
	evt.type = IMMSV_EVT_TYPE_IMMND;
	evt.info.immnd.type = IMMND_EVT_A2ND_IMM_ADMINIT;
	evt.info.immnd.info.adminitReq.client_hdl = immHandle;
	evt.info.immnd.info.adminitReq.i.adminOwnerName.length = nameLen;
	memcpy(evt.info.immnd.info.adminitReq.i.adminOwnerName.value, 
		adm_found_node->mAdminOwnerName, nameLen + 1);
	evt.info.immnd.info.adminitReq.i.releaseOwnershipOnFinalize = false;
	
	imma_proc_increment_pending_reply(cl_node);
	temp_hdl = adm_found_node->admin_owner_hdl;
	timeout = cl_node->syncr_timeout;
	/* Unlock before MDS Send */
	m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
	locked = false;
	cl_node=NULL;
	adm_found_node = NULL;

	if (cb->is_immnd_up == false) {
		TRACE_3("IMMND is DOWN");
		goto fail;
	}

	do {
		proc_rc = imma_mds_msg_sync_send(cb->imma_mds_hdl, &(cb->immnd_mds_dest), 
			&evt, &out_evt, timeout);

		switch (proc_rc) {
			case NCSCC_RC_SUCCESS:
				break;
			case NCSCC_RC_REQ_TIMOUT:
				goto fail;
			default:
				TRACE_3("MDS returned unexpected error code %u", proc_rc);
				goto fail;
		}

		osafassert(out_evt);
		err = out_evt->info.imma.info.admInitRsp.error;
		if (err != SA_AIS_OK) {
			usleep(sleep_delay_ms * 1000);
			msecs_waited += sleep_delay_ms;
			TRACE_1("admin-owner-set returned: %u", err);
			free(out_evt);
			out_evt = NULL;
		}
	} while ((err == SA_AIS_ERR_TRY_AGAIN)&&
		(msecs_waited < max_waiting_time_ms));

	if (err != SA_AIS_OK) {
		goto fail;
	}

	if (m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
		TRACE_3("Lock failure");
		goto fail;
	}
	locked = true;

	imma_client_node_get(&cb->client_tree, &immHandle, &cl_node);
	if (!(cl_node && cl_node->isOm)) {
		TRACE_3("client_node_get failed");
		goto fail;
	}

	imma_proc_decrement_pending_reply(cl_node);

	if (cl_node->stale) {
		TRACE_3("Handle %llx is stale", immHandle);
		cl_node->exposed = true;
		goto fail;
	}

	imma_admin_owner_node_get(&cb->admin_owner_tree, &temp_hdl, &adm_found_node);
	if (!adm_found_node) {
		TRACE_3("Admin Owner node removed during initialize call");
		goto fail;
	}

	adm_found_node->mAdminOwnerId = out_evt->info.imma.info.admInitRsp.ownerId;
	TRACE_1("Successfully re-initialized AdminOwner %s AOhandle %llx with "
		"id %u for resurrected client handle %llx", adm_found_node->mAdminOwnerName,
		temp_hdl, adm_found_node->mAdminOwnerId, immHandle);

 success:
	if (locked) {m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);}
	if (out_evt) {free(out_evt);}
	TRACE_LEAVE();
	return SA_TRUE;

 fail:
	if (locked) {m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);}
	if (out_evt) {free(out_evt);}
	TRACE_LEAVE();
	return SA_FALSE;
}

int imma_om_resurrect(IMMA_CB *cb, IMMA_CLIENT_NODE *cl_node, bool *locked)
{
	IMMSV_EVT  finalize_evt, *out_evt1 = NULL;
	TRACE_ENTER();
	osafassert(locked && *locked);
	osafassert(cl_node && cl_node->stale);
	SaImmHandleT immHandle = cl_node->handle;
	SaUint32T timeout = 0;
	SaAisErrorT err_resurrect=SA_AIS_OK;

	m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
	*locked = false;
	cl_node = NULL;
	if (!imma_proc_resurrect_client(cb, immHandle, true, &err_resurrect)) {
		TRACE_3("Failed to resurrect OM handle <c:%u, n:%x>",
			m_IMMSV_UNPACK_HANDLE_HIGH(immHandle),
			m_IMMSV_UNPACK_HANDLE_LOW(immHandle));
		goto fail;
	}

	TRACE_1("Successfully resurrected OM handle <c:%u, n:%x>",
		m_IMMSV_UNPACK_HANDLE_HIGH(immHandle),
		m_IMMSV_UNPACK_HANDLE_LOW(immHandle));

	/* Set adminOwners if needed. */
	if (imma_re_initialize_admin_owners(cb, immHandle)) {
		goto success;
	}

	TRACE_3("Failed to re-initialize admin-owner for resurrected "
		"OM handle - reverting resurrection");

	if (m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) 
	{
		TRACE_3("Lock failure");
		goto fail;
	}
	*locked = true;

	imma_client_node_get(&cb->client_tree, &immHandle, &cl_node);
	if (cl_node && cl_node->isOm)
	{
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
	finalize_evt.info.immnd.type = IMMND_EVT_A2ND_IMM_FINALIZE;
	finalize_evt.info.immnd.info.finReq.client_hdl = immHandle;

	/* send a finalize handle req to the IMMND.
	   Dont bother checking the answer. This is just
	   an attempt to deallocate the useless resurrected
	   handle on the server side. 
	*/

	if (cb->is_immnd_up) {
		imma_mds_msg_sync_send(cb->imma_mds_hdl, 
			&(cb->immnd_mds_dest),&finalize_evt,&out_evt1, timeout);

		/* Dont care about the response on finalize. */
		if (out_evt1) {free(out_evt1);}
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
  Name          :  saImmOmCcbGetErrorStrings
 
  Description   :  Allows the OM client to fetch error strings related to a
                   ccb down-call of:

                      saImmOmCcbObjectCreate
                      saImmOmCcbObjectModify
                      saImmOmCcbObjectModify
                      saImmOmCcbCompleted

                   This is only of interest when the down-call returned an error,
                   such as FAILED_OPERATION or BAD_OPERATION.
                   That is, an error that could result from one or more OIs 
                   rejecting the ccb call.
                   
  Arguments     :  ccbHandle    - The ccb handle.
                   errorStrings - Pointer to a pointer to a NULL terminated array
                                  of SaStringT. The list of strings is owned by
                                  the imma om library and is cleared/deallocated
                                  in conjunction with the next downcall using the 
                                  ccb-handle. 
 
  Return Values :  SA_AIS_OK
                   SA_AIS_ERR_BAD_HANDLE
                   SA_AIS_ERR_INVALID_PARAM
                   SA_AIS_ERR_LIBRARY
                   SA_AIS_ERR_TRY_AGAIN
                   
******************************************************************************/
SaAisErrorT saImmOmCcbGetErrorStrings(SaImmCcbHandleT ccbHandle,
	const SaStringT **errorStrings)

{
	TRACE_ENTER();
	SaAisErrorT rc = SA_AIS_OK;
	bool locked = false;
	IMMA_CB *cb = &imma_cb;
	IMMA_CCB_NODE *ccb_node = NULL;
	SaImmHandleT immHandle=0LL;
	IMMA_CLIENT_NODE *cl_node = NULL;
	TRACE_ENTER();

	if (cb->sv_id == 0) {
		TRACE_2("ERR_BAD_HANDLE: No initialized handle exists!");
		return SA_AIS_ERR_BAD_HANDLE;
	}

	if (errorStrings == NULL) {
		TRACE_LEAVE();
		return SA_AIS_ERR_INVALID_PARAM;
	}
	(*errorStrings) = NULL;

	/* get the CB Lock */
	if (m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
		rc = SA_AIS_ERR_LIBRARY;
		TRACE_4("ERR_LIBRARY: Lock failed");
		goto lock_fail;
	}
	locked = true;

	/* Get the CCB info */
	imma_ccb_node_get(&cb->ccb_tree, &ccbHandle, &ccb_node);
	if (!ccb_node) {
		rc = SA_AIS_ERR_BAD_HANDLE;
		TRACE_2("ERR_BAD_HANDLE: Ccb handle not valid");
		goto client_not_found;
	}

	if (ccb_node->mExclusive) {
		rc = SA_AIS_ERR_TRY_AGAIN;
		TRACE_3("ERR_TRY_AGAIN: Ccb-id %u being created or in critical phase, in another thread",
			ccb_node->mCcbId);
		goto client_not_found;
	}

	immHandle = ccb_node->mImmHandle;

	imma_client_node_get(&cb->client_tree, &immHandle, &cl_node);
	if (!(cl_node && cl_node->isOm)) {
		rc = SA_AIS_ERR_LIBRARY;
		TRACE_4("ERR_LIBRARY: SaImmHandleT associated with Ccb is not valid");
		goto client_not_found;
	}

	if(!(cl_node->isImmA2b)) {
		rc = SA_AIS_ERR_VERSION;
		TRACE_2("ERR_VERSION: saImmOmCcbGetErrorStrings only supported for "
			"A.02.11 and above");
		goto client_not_found;
	}

	(*errorStrings) = ccb_node->mErrorStrings;


 client_not_found:
	if (locked)
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);

 lock_fail:

	TRACE_LEAVE();
	return rc;
}


/* Internal function used *only* from saImmOiAugmentCcbInitialize
   Creates a mockup ccb-node and mockup admo-node so that an
   OI client can augment a pre existing ccb with added create,
   delete and modify operations.
*/
SaAisErrorT immsv_om_augment_ccb_initialize(
				 SaImmOiHandleT privateOmHandle,
				 SaUint32T ccbId,
				 SaUint32T adminOwnerId,
				 SaImmCcbHandleT *ccbHandle,
				 SaImmAdminOwnerHandleT *ownerHandle)

{
	SaAisErrorT rc = SA_AIS_OK;
	SaUint32T proc_rc = NCSCC_RC_SUCCESS;
	IMMA_CB *cb = &imma_cb;
	IMMA_CLIENT_NODE *cl_node = 0;
	bool locked = false;
	IMMA_CCB_NODE *ccb_node = NULL;
	IMMA_ADMIN_OWNER_NODE *ao_node = NULL;

	TRACE_ENTER();

	if (cb->sv_id == 0) {
		TRACE_2("ERR_BAD_HANDLE: No initialized handle exists!");
		TRACE_LEAVE();
		return SA_AIS_ERR_BAD_HANDLE;
	}
	TRACE("ccbHandle(%p) && ccbId(%u) && adminOwnerId(%u)", ccbHandle, ccbId, adminOwnerId);
	osafassert(ccbHandle && ccbId && adminOwnerId);

	if (cb->is_immnd_up == false) {
		TRACE_2("ERR_NO_RESOURCES: IMMND_DOWN");
		TRACE_LEAVE();
		return SA_AIS_ERR_NO_RESOURCES;
	}

	/* Take the CB lock */
	if (m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
		TRACE_4("ERR_LIBRARY: LOCK failed");
		rc = SA_AIS_ERR_LIBRARY;
		goto lock_fail;
	}
	locked = true;

	imma_client_node_get(&cb->client_tree, &privateOmHandle, &cl_node);
	if (!(cl_node && cl_node->isOm)) {
		TRACE_2("ERR_BAD_HANDLE: Bad handle %llx", privateOmHandle);
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto done;
	}

	if (cl_node->stale) {
		TRACE_1("ERR_NO_RESOURCES: Handle %llx is stale", privateOmHandle);
		rc = SA_AIS_ERR_NO_RESOURCES;
		goto done;
	}

	if(!(cl_node->isImmA2b)) {
		TRACE("ERR_VERSION: immsv_om_augment_ccb_initialize " 
			"only allowed when OI handle is registered as A.2.11");
		rc = SA_AIS_ERR_VERSION;
		goto done;
	}

	/* Mark client node as a private temporary OM handle,
	   owned by an OI for ccb augmentation. This to avoid
	   the the handle being resurrected. Resurection is
	   poinless since loss of contact with immnd will
	   terminate the CCB anyway. Hence not possible
	   to continue with some augmentation to the ccb. 
	*/
	cl_node->isAug = true;

	/* Check if admo & ccb already exis, easy to do! same handle *value*
	   used for om-handle ccb-handle and admo-handle,
	 */

	imma_admin_owner_node_get(&cb->admin_owner_tree, &privateOmHandle, &ao_node);
	if(ao_node) {
		if(!(ao_node->mAugCcb) || (ao_node->mAdminOwnerId != adminOwnerId)) {
			TRACE_4("ERR_LIBRARY: Re used admo-node does not match!");
			rc = SA_AIS_ERR_LIBRARY;
			goto done;
		}
		TRACE("Admo node already exists for %llu => re-use", privateOmHandle);
		/* The admo-handle already exists => ccb-handle should also exist. */
		imma_ccb_node_get(&cb->ccb_tree, &privateOmHandle, &ccb_node);
		if(ccb_node) {
			if(!(ccb_node->mAugCcb) || (ccb_node->mCcbId != ccbId)) {
				TRACE_4("ERR_LIBRARY: Re used ccb-node does not match!");
				rc = SA_AIS_ERR_LIBRARY;
				goto done;
			}
			ccb_node->mApplied = false;
			TRACE("Ccb node already exists for handle %llu ccb:%u=> re-use",
				privateOmHandle, ccbId);
			/* Both ccb node and admo node alreadhy exist=> skip create of
			   these nodes. This the private om-handle, client-node, 
			   admo-node and ccb-node are reused in retries and in 
			   repeated upcalls for the same ccb towards this OI.
			 */
			goto re_used;
		} else {
			TRACE_1("Admo handle found but ccb handle non existent -"
				" should not happen");
			/* Try to create new admo and ccb, i.e. fall through to
			   the initial branch directly below.
			 */
			ao_node = NULL;
		}
	}

	/* Allocate ccb_node. */
	ccb_node = (IMMA_CCB_NODE *)calloc(1, sizeof(IMMA_CCB_NODE));
	osafassert(ccb_node);

	/*
	  Use the same handle value for om/admo/ccb to simplify re-use of
	  handles in several upcalls to thi OI, in the same ccb. 
	*/
	ccb_node->ccb_hdl = privateOmHandle;

	proc_rc = imma_ccb_node_add(&cb->ccb_tree, ccb_node);

	if (proc_rc != NCSCC_RC_SUCCESS) {
		rc = SA_AIS_ERR_LIBRARY;
		TRACE_4("ERR_LIBRARY: Failed to add ccb-node to ccb-tree");
		free(ccb_node);
		ccb_node = NULL;
		goto done;
	}

	ccb_node->mImmHandle = privateOmHandle;
	ccb_node->mAdminOwnerHdl = ccb_node->ccb_hdl; /* same value */
	ccb_node->mCcbId = ccbId;
	ccb_node->mApplied = false;
	ccb_node->mAugCcb = true;

	/* Allocate the ao_node */
	ao_node = (IMMA_ADMIN_OWNER_NODE *)calloc(1, sizeof(IMMA_ADMIN_OWNER_NODE));
	osafassert(ao_node);
	ao_node->admin_owner_hdl = ccb_node->ccb_hdl; /* same value */
	ao_node->mImmHandle = privateOmHandle;
	ao_node->mAdminOwnerId = adminOwnerId;
	ao_node->mAugCcb = true;
	/*ao_node->mReleaseOnFinalize = true;*/
	/* We dont know what the value should be for rof.
	   But it should not matter, since this ao_node shares admo-id
	   with the real admo and thus in the server will share rof.
	 */
	proc_rc = imma_admin_owner_node_add(&cb->admin_owner_tree, ao_node);
	if (proc_rc != NCSCC_RC_SUCCESS) {
		rc = SA_AIS_ERR_LIBRARY;
		TRACE_4("ERR_LIBRARY: Failed to add node to the admin owner tree");
		free(ao_node);
		ao_node=NULL;

		imma_ccb_node_delete(cb, ccb_node); /* Removes node from tree and frees it. */
		ccb_node = NULL;
		goto done;
	}

 re_used:
	osafassert(locked);
	*ccbHandle = ccb_node->ccb_hdl;
	if(ownerHandle) {
		if(*ownerHandle && (*ownerHandle != ao_node->admin_owner_hdl)) {
			/* Separate admin owner created to have release-on-finalize true */
			imma_admin_owner_node_get(&cb->admin_owner_tree, ownerHandle, &ao_node);
			if(ao_node) {
				ao_node->mAugCcb = true;
				/* Inform the IMMNDs asyncronously of the extra admo*/
				IMMSV_EVT evt;
				memset(&evt, 0, sizeof(IMMSV_EVT));
				evt.type = IMMSV_EVT_TYPE_IMMND;
				evt.info.immnd.type = IMMND_EVT_A2ND_AUG_ADMO;
				evt.info.immnd.info.objDelete.adminOwnerId = ao_node->mAdminOwnerId;
				evt.info.immnd.info.objDelete.ccbId = ccb_node->mCcbId;
				evt.info.immnd.info.objDelete.objectName.size = 0;

				imma_evt_fake_evs(cb, &evt, NULL, 0, privateOmHandle, &locked, false);
			}
			
		} else {
			*ownerHandle = ao_node->admin_owner_hdl;
		}
	}

	TRACE("Augmented CCB CcbId:%u admin ownerId:%u ccb-handle:%llx admo-handle:%llx\n", 
		ccb_node->mCcbId, adminOwnerId, *ccbHandle, ownerHandle?(*ownerHandle):0);
 done:

	if (locked) {
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
	}

 lock_fail:
	TRACE_LEAVE();
	return rc;
}

/* Internal function used *only* from callback processing in imma_proc.c
   Obtains the outcome for an OI augmented CCB. 
   The outcome should be obtained after the create/delete/modify callback 
   to the OI returns, to the callback post processing in imma_proc.c
   See imma_process_callback_info()
*/
SaAisErrorT immsv_om_augment_ccb_get_result(
				 SaImmOiHandleT privateOmHandle,
				 SaUint32T ccbId)

{
	SaAisErrorT rc = SA_AIS_OK;
	IMMA_CB *cb = &imma_cb;
	bool locked = false;
	IMMA_CCB_NODE *ccb_node = NULL;
	SaImmCcbHandleT ccbHandle = privateOmHandle; /* Same value */
	TRACE_ENTER();

	osafassert(cb->sv_id != 0);
	osafassert(m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) == NCSCC_RC_SUCCESS);
	locked = true;

	/* Get the CCB info */
	imma_ccb_node_get(&cb->ccb_tree, &ccbHandle, &ccb_node);
	if (!ccb_node) {
		rc = SA_AIS_ERR_FAILED_OPERATION;
		TRACE_2("Aug Ccb node removed prematurely");
		goto done;
	}

	osafassert(ccb_node->mCcbId == ccbId);
	osafassert(ccb_node->mAugCcb);
	if(ccb_node->mAborted || !(ccb_node->mApplied)) {
		rc = SA_AIS_ERR_FAILED_OPERATION;
	}

 done:
	if (locked) {
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
	}

	TRACE_LEAVE();
	return rc;
}

SaAisErrorT immsv_om_augment_ccb_get_admo_name(
			       SaImmHandleT privateOmHandle,
			       SaNameT* objectName,
			       SaNameT* admoNameOut)

{
	SaAisErrorT rc = SA_AIS_OK;
	const SaImmAttrNameT admoNameAttr = SA_IMM_ATTR_ADMIN_OWNER_NAME;
	SaImmAttrNameT attributeNames[2] = {admoNameAttr, NULL};
	SaImmAttrValuesT_2 **attributes = NULL;
	SaImmAttrValuesT_2 *attrVal = NULL;
	SaImmAccessorHandleT acHdl=0LL;
	TRACE_ENTER();	

	rc = saImmOmAccessorInitialize(privateOmHandle, &acHdl);
	if(rc != SA_AIS_OK) {goto done;}

	rc = saImmOmAccessorGet_2(acHdl, objectName, attributeNames, &attributes);
	if(rc != SA_AIS_OK) {goto finalize;}

	attrVal = attributes[0];
	if(!attrVal) {
		rc = SA_AIS_ERR_LIBRARY;
		goto finalize;
	}

	strncpy((char *)admoNameOut->value, *(SaStringT*) attrVal->attrValues[0], SA_MAX_NAME_LENGTH);

 finalize:
    if(acHdl) {
	    saImmOmAccessorFinalize(acHdl);
    }

 done:	
	
	TRACE_LEAVE();
	return rc;
}

SaAisErrorT immsv_om_handle_initialize(SaImmHandleT *privateOmHandle, SaVersionT* version)
{
	TRACE_ENTER();
	return saImmOmInitialize_o2(privateOmHandle, NULL, version);
}

SaAisErrorT immsv_om_admo_handle_initialize(
				  SaImmHandleT immHandle,
				  const SaImmAdminOwnerNameT adminOwnerName,
				  SaImmAdminOwnerHandleT *adminOwnerHandle)
{
	TRACE_ENTER();
	return saImmOmAdminOwnerInitialize(immHandle, adminOwnerName, SA_TRUE, adminOwnerHandle);
}

void immsv_om_handle_finalize(SaImmHandleT privateOmHandle)
{
	TRACE_ENTER();
	saImmOmFinalize(privateOmHandle);
}

