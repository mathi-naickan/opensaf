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

..............................................................................

  DESCRIPTION:
  
  This file contains the CPSv SAF API definitions.
    
******************************************************************************/

#include "cpa.h"
#define NCS_SAF_MIN_ACCEPT_TIME 10

/****************************************************************************
  Name          :  SaCkptInitialize
 
  Description   :  This function initializes the Checkpoint Service for the
                   invoking process and registers the various callback functions.
                   The handle 'ckptHandle' is returned as the reference to this
                   association between the process and the Checkpoint Service.
                   
 
  Arguments     :  ckptHandle - A pointer to the handle designating this 
                                particular initialization of the Checkpoint 
                                service that it to be returned by the Checkpoint
                                Service.
                   callbacks  - Pointer to a SaCkptCallbacksT structure, 
                                containing the callback functions of the process
                                that the Checkpoint Service may invoke.
                   version    - Is a pointer to the version of the Checkpoint 
                                Service that the invoking process is using.
 
  Return Values :  Refer to SAI-AIS specification for various return values.
 
  Notes         :
******************************************************************************/
SaAisErrorT saCkptInitialize(SaCkptHandleT *ckptHandle, const SaCkptCallbacksT *ckptCallbacks, SaVersionT *version)
{
	CPA_CB *cb = NULL;
	SaAisErrorT rc = SA_AIS_OK;
	CPSV_EVT init_evt;
	CPSV_EVT *out_evt = NULL;
	uint32_t proc_rc = NCSCC_RC_SUCCESS;
	CPA_CLIENT_NODE *cl_node = NULL;
	bool locked = true;
	SaVersionT client_version;

	TRACE_ENTER();
	proc_rc = ncs_agents_startup();
	if (NCSCC_RC_SUCCESS != proc_rc) {
		TRACE_4("cpa CkptInit:agents_startup Api failed with return value:%d", proc_rc);
		return SA_AIS_ERR_LIBRARY;
	}

	proc_rc = ncs_cpa_startup();
	if (NCSCC_RC_SUCCESS != proc_rc) {
		TRACE_4("cpa CkptInit:agents_startup Api failed with return value:%d" , proc_rc);
		ncs_agents_shutdown();
		return SA_AIS_ERR_LIBRARY;
	}

	if ((!ckptHandle) || (!version)) {
		rc = SA_AIS_ERR_INVALID_PARAM;
		TRACE_4("cpa CkptInit Api failed with return value :%d", rc);
		goto end;
	}

	*ckptHandle = 0;

	memcpy(&client_version, version, sizeof(SaVersionT));

	/* Draft Validations : Version */
	rc = cpa_version_validate(version);
	if (rc != SA_AIS_OK) {
		TRACE_4("cpa  CkptInit Api failed with return value:%d", rc);
		goto end;
	}

	/* retrieve CPA CB */
	m_CPA_RETRIEVE_CB(cb);
	if (cb == NULL) {
		TRACE_4("cpa CkptInit:HDL_TAKE");
		return SA_AIS_ERR_LIBRARY;
	}

	/* Take the CB Lock */
	if (m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
		TRACE_4("cpa CkptInit:LOCK");
		rc = SA_AIS_ERR_LIBRARY;
		goto lock_fail;
	}

	/* Alloc the client info data structure & put it in the Pat tree */
	cl_node = (CPA_CLIENT_NODE *)m_MMGR_ALLOC_CPA_CLIENT_NODE;
	if (cl_node == NULL) {
		TRACE_4("cpa mem alloc CkptInit:CPA_CLIENT_NODE");
		rc = SA_AIS_ERR_NO_MEMORY;
		goto cnode_alloc_fail;
	}
	memset(cl_node, 0, sizeof(CPA_CLIENT_NODE));

	/* Store the callback functions, if set */
	if (ckptCallbacks) {
		cl_node->ckpt_callbk = *ckptCallbacks;

	}

        proc_rc = cpa_callback_ipc_init(cl_node);
	if (proc_rc != NCSCC_RC_SUCCESS) {
		/* Error handling */
		rc = SA_AIS_ERR_LIBRARY;
		/* ALready looged by cpa_callback_ipc_init */
		goto ipc_init_fail;
	}

	/* populate the EVT structure */
	memset(&init_evt, 0, sizeof(CPSV_EVT));
	init_evt.type = CPSV_EVT_TYPE_CPND;
	init_evt.info.cpnd.type = CPND_EVT_A2ND_CKPT_INIT;
	init_evt.info.cpnd.info.initReq.version = client_version;

	/* Release the CB lock Before MDS Send */
	if (locked)
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
	locked = false;

	/* CPND GOES DOWN */
	if (false == cb->is_cpnd_up) {
		rc = SA_AIS_ERR_TRY_AGAIN;
		TRACE_4("cpa: api failed in CkptInit:CPND_DOWN");
		goto mds_fail;
	}

	/* send the request to the CPND */
	proc_rc = cpa_mds_msg_sync_send(cb->cpa_mds_hdl, &cb->cpnd_mds_dest, &init_evt, &out_evt, CPSV_WAIT_TIME);

	/* Error Handling */
	switch (proc_rc) {
	case NCSCC_RC_SUCCESS:
		break;
	case NCSCC_RC_REQ_TIMOUT:
		TRACE_4("cpa CkptInit:MDS Api failed with return value:%d,cpnd_mds_dest:%"PRIu64, proc_rc, cb->cpa_mds_dest);
		rc = SA_AIS_ERR_TIMEOUT;
		goto mds_fail;
	default:
		TRACE_4("cpa CkptInit:MDS Api failed with return value:%d,cpnd_mds_dest:%"PRIu64, proc_rc, cb->cpa_mds_dest);
		rc = SA_AIS_ERR_TRY_AGAIN;
		goto mds_fail;
	}

	if (out_evt) {
		rc = out_evt->info.cpa.info.initRsp.error;
		if (rc != SA_AIS_OK) {
			TRACE_4("cpa CkptInit:CPND_ERR Api failed with return value:%d",rc);
			goto rsp_not_ok;
		}

		if (m_CPA_VER_IS_ABOVE_B_1_1(version)) {
			if (rc == SA_AIS_ERR_UNAVAILABLE) {
				cb->is_cpnd_joined_clm = false;
				if (locked)
					m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
				locked = false;
				goto clm_left;
			} else {
				cb->is_cpnd_joined_clm = true;
			}
		}
		/* Take the CB lock after MDS Send */
		if (m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
			TRACE_4("cpa CkptInit:LOCK");
			rc = SA_AIS_ERR_LIBRARY;
			goto lock_fail1;
		} else
			locked = true;

		cl_node->cl_hdl = out_evt->info.cpa.info.initRsp.ckptHandle;
		cl_node->stale = false;
		proc_rc = cpa_client_node_add(&cb->client_tree, cl_node);
		if (proc_rc != NCSCC_RC_SUCCESS) {
			rc = SA_AIS_ERR_LIBRARY;
			TRACE_4("cpa api processing failed in CkptInit:client_node_add Api failed with return value:%d,cl_hdl:%llx ", rc, cl_node->cl_hdl);
			goto node_add_fail;
		}
	} else {
		TRACE_4("cpa api processing failed in CkptInit Api failed with return value:%d", rc);
		rc = SA_AIS_ERR_NO_RESOURCES;
	}

	if (rc != SA_AIS_OK) {
		TRACE_4("cpa CkptInit:CPND_ERR Api failed with return value:%d",rc);
		goto cpnd_rsp_fail;
	}

	/*Error handling */
 cpnd_rsp_fail:
 node_add_fail:
 lock_fail1:
	if (rc != SA_AIS_OK) {
		CPSV_EVT finalize_evt, *out_evt1;

		out_evt1 = NULL;
		/* populate the structure */
		memset(&finalize_evt, 0, sizeof(CPSV_EVT));
		finalize_evt.type = CPSV_EVT_TYPE_CPND;
		finalize_evt.info.cpnd.type = CPND_EVT_A2ND_CKPT_FINALIZE;
		finalize_evt.info.cpnd.info.finReq.client_hdl = cl_node->cl_hdl;

		if (locked)
			m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
		locked = false;

		/* send the request to the CPND */
		proc_rc = cpa_mds_msg_sync_send(cb->cpa_mds_hdl, &(cb->cpnd_mds_dest),
						&finalize_evt, &out_evt1, CPSV_WAIT_TIME);
		if (out_evt1)
			m_MMGR_FREE_CPSV_EVT(out_evt1, NCS_SERVICE_ID_CPA);
	}

 rsp_not_ok:
 mds_fail:

	/* Free the IPC initialized for this client */
	if (rc != SA_AIS_OK)
		cpa_callback_ipc_destroy(cl_node);

 ipc_init_fail:
	/* Free the Client NODE */
	if (rc != SA_AIS_OK)
		m_MMGR_FREE_CPA_CLIENT_NODE(cl_node);

 cnode_alloc_fail:
	/* Release the CB lock */
	if (locked)
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);

 lock_fail:
 clm_left:
	/* Release the CB handle */
	m_CPA_GIVEUP_CB;

	/* Free the Out Event */
	if (out_evt)
		m_MMGR_FREE_CPSV_EVT(out_evt, NCS_SERVICE_ID_CPA);

	if (rc == SA_AIS_OK) {
		memcpy(&(cl_node->version), &client_version, sizeof(SaVersionT));
		/* Went well, return ckptHandle to the application */
		*ckptHandle = cl_node->cl_hdl;
		TRACE_1("cpa ckptInit Api success with return value:%d and cl_hdl:%llx", SA_AIS_OK, cl_node->cl_hdl);
	}
 end:
	if (rc != SA_AIS_OK) {
		ncs_cpa_shutdown();
		ncs_agents_shutdown();
	}

	TRACE_LEAVE2("API Return code = %u", rc);
	return rc;
}

/****************************************************************************
  Name          :  saCkptSelectionObjectGet
 
  Description   :  This function returns the operating system handle 
                   associated with the ckptHandle.
 
  Arguments     :  ckptHandle - Checkpoint service handle.
                   selectionObject - Pointer to the operating system handle.
 
  Return Values :  Refer to SAI-AIS specification for various return values.
 
  Notes         :
******************************************************************************/
SaAisErrorT saCkptSelectionObjectGet(SaCkptHandleT ckptHandle, SaSelectionObjectT *selectionObject)
{
	SaAisErrorT rc = SA_AIS_OK;
	CPA_CB *cb = NULL;
	CPA_CLIENT_NODE *cl_node = NULL;

	TRACE_ENTER2("SaCkptCheckpointHandleT passed is %llx",ckptHandle);

	if (!selectionObject)
		return SA_AIS_ERR_INVALID_PARAM;

	/* retrieve CPA CB */
	m_CPA_RETRIEVE_CB(cb);
	if (cb == NULL) {
		TRACE_4("Cpa SelObjGet:HM_TAKE failed with return value:%d,ckptHandle:%llx", SA_AIS_ERR_BAD_HANDLE, ckptHandle);
		rc = SA_AIS_ERR_BAD_HANDLE;
		TRACE_LEAVE2("API return code = %u", rc);
		return rc;
	}

	/* Take the CB lock */
	if (m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
		TRACE_4("Cpa SelObjGet:LOCK failed with ckptHandle:%llx", ckptHandle);
		rc = SA_AIS_ERR_LIBRARY;
		goto lock_fail;
	}

	(void)cpa_client_node_get(&cb->client_tree, &ckptHandle, &cl_node);

	if (!cl_node) {
		TRACE_4("Cpa SelObjGet:client_node_get Api failed with return value:%d,ckptHandle:%llx",SA_AIS_ERR_BAD_HANDLE, ckptHandle);
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto node_not_found;
	}

	/* Checking Node availability */
	if (m_CPA_VER_IS_ABOVE_B_1_1(&cl_node->version)) {
		if ((cb->is_cpnd_joined_clm != true)  || ( cl_node->stale == true)){
			TRACE_4("Cpa CLM Node left: return value:%d",SA_AIS_ERR_UNAVAILABLE);
			rc = SA_AIS_ERR_UNAVAILABLE;
			m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
			goto clm_left;
		}
	}

	*selectionObject = (SaSelectionObjectT)
	    m_GET_FD_FROM_SEL_OBJ(m_NCS_IPC_GET_SEL_OBJ(&cl_node->callbk_mbx));

 node_not_found:
	/* Unlock CB */
	m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);

 clm_left:
 lock_fail:
	/* Release the CB handle */
	m_CPA_GIVEUP_CB;

	TRACE_LEAVE2("API Return code = %u", rc);
	return rc;
}

/****************************************************************************
  Name          :  saCkptDispatch
 
  Description   :  This function invokes, in the context of the calling
                   thread. pending callbacks for the handle ckptHandle in a 
                   way that is specified by the dispatchFlags parameter.
 
  Arguments     :  ckptHandle - Checkpoint Service handle
                   dispatchFlags - Flags that specify the callback execution
                                   behaviour of this function.
 
  Return Values :  Refer to SAI-AIS specification for various return values.
 
  Notes         :
******************************************************************************/
SaAisErrorT saCkptDispatch(SaCkptHandleT ckptHandle, SaDispatchFlagsT dispatchFlags)
{
	SaAisErrorT rc = SA_AIS_OK;

	CPA_CB *cb = NULL;
	CPA_CLIENT_NODE *cl_node = NULL;

	TRACE_ENTER2("SaCkptCheckpointHandleT passed is %llx",ckptHandle);
	/* retrieve CPA CB */
	m_CPA_RETRIEVE_CB(cb);
	if (!cb) {
		TRACE_4("Cpa CkptDispatch:HDL_TAKE failed :ckptHandle:%llx",ckptHandle);
		rc = SA_AIS_ERR_BAD_HANDLE;
		return rc;
	}

	/* get the client_info */
	if (m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
		TRACE_4("Cpa CkptDispatch:LOCK failed :ckptHandle:%llx",ckptHandle);
		rc = SA_AIS_ERR_LIBRARY;
		goto lock_fail;
	}

	rc = cpa_client_node_get(&cb->client_tree, &ckptHandle, &cl_node);
	if (!cl_node) {
		TRACE_4("Cpa CkptDispatch:client_node_get failed with return value:%d,ckptHandle:%llx",SA_AIS_ERR_BAD_HANDLE, ckptHandle);
		rc = SA_AIS_ERR_BAD_HANDLE;
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
		goto node_not_found;
	}

	/* Checking Node availability */
	if (m_CPA_VER_IS_ABOVE_B_1_1(&cl_node->version)) {
		if ((cb->is_cpnd_joined_clm != true)  || ( cl_node->stale == true)) {
			TRACE_4("Cpa CLM Node left: return value:%d ", SA_AIS_ERR_UNAVAILABLE);
			rc = SA_AIS_ERR_UNAVAILABLE;
			m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
			goto clm_left;
		}
	}
	/*  cpa deadlock in arrival callback ,do unlock and then do the dispatch processing  */
	m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);

	switch (dispatchFlags) {
	case SA_DISPATCH_ONE:
		rc = cpa_hdl_callbk_dispatch_one(cb, ckptHandle);
		break;

	case SA_DISPATCH_ALL:
		rc = cpa_hdl_callbk_dispatch_all(cb, ckptHandle);
		break;

	case SA_DISPATCH_BLOCKING:
		rc = cpa_hdl_callbk_dispatch_block(cb, ckptHandle);
		break;

	default:
		rc = SA_AIS_ERR_INVALID_PARAM;
		break;
	}			/* switch */

	if (rc != SA_AIS_OK) {
		TRACE_4("Cpa CkptDispatch failed with return value:%d,ckptHandle:%llx", rc, ckptHandle);
	}

 node_not_found:
	/* Unlock CB */

 lock_fail:
 clm_left:

	/* Release the CB handle */
	ncshm_give_hdl(gl_cpa_hdl);

	TRACE_LEAVE2("API Return code = %u", rc);
	return rc;
}

/****************************************************************************
  Name          :  saCkptFinalize
 
  Description   :  This function closes the association, represented by
                   ckptHandle, between the invoking process and the Checkpoint
                   Service.
 
  Arguments     :  ckptHandle - Checkpoint Service handle.
 
  Return Values :  Refer to SAI-AIS specification for various return values.
 
  Notes         :
******************************************************************************/
SaAisErrorT saCkptFinalize(SaCkptHandleT ckptHandle)
{
	SaAisErrorT rc = SA_AIS_OK;
	CPA_CB *cb = NULL;
	CPSV_EVT finalize_evt;
	CPSV_EVT *out_evt = NULL;
	CPA_CLIENT_NODE *cl_node = NULL;
	uint32_t proc_rc = NCSCC_RC_SUCCESS;
	bool locked = true;

	TRACE_ENTER2("SaCkptCheckpointHandleT passed is %llx",ckptHandle);
	m_CPA_RETRIEVE_CB(cb);
	if (!cb) {
		TRACE_4("Cpa CkptFin:HM_TAKE failed with return value:%d,ckptHandle:%llx", SA_AIS_ERR_BAD_HANDLE, ckptHandle);
		return SA_AIS_ERR_BAD_HANDLE;
	}

	/* get the client_info */
	if (m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
		TRACE_4("cpa CkptFin:LOCK Api failed with return value:%d,ckptHandle:%llx ", SA_AIS_ERR_LIBRARY, ckptHandle);
		rc = SA_AIS_ERR_LIBRARY;
		goto lock_fail;
	}

	proc_rc = cpa_client_node_get(&cb->client_tree, &ckptHandle, &cl_node);
	if (!cl_node) {
		rc = SA_AIS_ERR_BAD_HANDLE;
		TRACE_4("cpa CkptFin:client_node_get Api failed with return value:%d,ckptHandle:%llx", rc, ckptHandle);
		goto node_not_found;
	}

	if (cl_node->stale == true)
	{	rc = SA_AIS_OK;
		/* Unlock before MDS Send */
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
		locked = false;
		goto clnode_stale;
	}

	/* populate the structure */
	memset(&finalize_evt, 0, sizeof(CPSV_EVT));
	finalize_evt.type = CPSV_EVT_TYPE_CPND;
	finalize_evt.info.cpnd.type = CPND_EVT_A2ND_CKPT_FINALIZE;
	finalize_evt.info.cpnd.info.finReq.client_hdl = cl_node->cl_hdl;

	/* Unlock before MDS Send */
	m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
	locked = false;

	/* CPND GOES DOWN */
	if (false == cb->is_cpnd_up) {
		rc = SA_AIS_ERR_TRY_AGAIN;
		TRACE_1("cpa CkptFin:CPND_DOWN Api failed with return value:%d,ckptHandle:%llx", rc, ckptHandle);
		goto mds_send_fail;
	}

	/* send the request to the CPND */
	proc_rc = cpa_mds_msg_sync_send(cb->cpa_mds_hdl, &(cb->cpnd_mds_dest), &finalize_evt, &out_evt, CPSV_WAIT_TIME);

	/* MDS error handling */
	switch (proc_rc) {
	case NCSCC_RC_SUCCESS:
		break;
	case NCSCC_RC_REQ_TIMOUT:
		rc = SA_AIS_ERR_TIMEOUT;
		TRACE_4("cpa CkptFin:MDS api failed with return value:%d,ckptHandle:%llx,cpnd_mds_dest:%"PRIu64,
                        proc_rc , ckptHandle, cb->cpnd_mds_dest);
		goto mds_send_fail;
	default:
		TRACE_4("cpa CkptFin:MDS api failed with return value:%d,ckptHandle:%llx,cpnd_mds_dest:%"PRIu64,
                        proc_rc , ckptHandle , cb->cpnd_mds_dest);
		rc = SA_AIS_ERR_TRY_AGAIN;
		goto mds_send_fail;
	}

	/* Read the received error (if any)  */
	if (out_evt) {
		rc = out_evt->info.cpa.info.finRsp.error;

		/* Free the Out Event */
		m_MMGR_FREE_CPSV_EVT(out_evt, NCS_SERVICE_ID_CPA);
	} else
		rc = SA_AIS_ERR_NO_RESOURCES;
		
clnode_stale:
	/* Do the finalize processing at CPA */
	if (rc == SA_AIS_OK) {
		/* Take the CB lock  */
		if (m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
			rc = SA_AIS_ERR_LIBRARY;
			TRACE_4("Cpa CkptFin:LOCK failed with return value:%d,ckptHandle:%llx", rc, ckptHandle);
			goto lock_fail1;
		}

		locked = true;

		cpa_ckpt_finalize_proc(cb, cl_node);
	}

 lock_fail1:
 mds_send_fail:
 node_not_found:
	if (locked)
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);

 lock_fail:
	/* Release the CB handle */
	m_CPA_GIVEUP_CB;

	if (rc == SA_AIS_OK) {
		TRACE_4("Cpa CkptFinalize Api success with return value:%d,ckptHandle:%llx", rc, ckptHandle);
		ncs_cpa_shutdown();
		ncs_agents_shutdown();
	}
	
	TRACE_LEAVE2("API return code = %u", rc);
	return rc;
}

/****************************************************************************
  Name          :  saCkptCheckpointOpen
 
  Description   :  Open a Checkpoint, a new checkpoint handle is returned upon
                   completion. A Checkpoint can be opened multiple times for
                   reading and or writing in the same or different process.
                   
  Arguments     :  ckptHandle - Checkpoint handle
                   checkpointName - A pointer to the name of the checkpoint
                           that identifies a checkpoint globally in a cluster.
                   checkpointCreationAttributes - A pointer to the creation 
                           attributes of a checkpoint.
                   checkpointOpenFlags - Flags defined by the
                                  SaCkptCheckpointOpenFlagsT type.
                   timeout - Invocation is considered to be failed if this 
                             function does not complete by the time specified.
                   checkpointHandle - A pointer to the checkpoint handle.

  Return Values :  Refer to SAI-AIS specification for various return values.
 
  Notes         :
******************************************************************************/
SaAisErrorT saCkptCheckpointOpen(SaCkptHandleT ckptHandle, const SaNameT *checkpointName, const SaCkptCheckpointCreationAttributesT
				 *checkpointCreationAttributes,
				 SaCkptCheckpointOpenFlagsT checkpointOpenFlags,
				 SaTimeT timeout, SaCkptCheckpointHandleT *checkpointHandle)
{
	SaAisErrorT rc = SA_AIS_OK;
	CPA_CB *cb = NULL;
	CPSV_EVT evt, *out_evt = NULL;
	CPA_LOCAL_CKPT_NODE *lc_node = NULL;
	CPA_CLIENT_NODE *cl_node = NULL;
	uint32_t proc_rc = NCSCC_RC_SUCCESS;
	bool locked = false;
        uint32_t time_out=0;
	CPA_GLOBAL_CKPT_NODE *gc_node = NULL;
	
	TRACE_ENTER2("SaCkptCheckpointHandleT passed is %llx",ckptHandle);
	if ((checkpointName == NULL) || (checkpointHandle == NULL) || (checkpointName->length == 0)) {
		TRACE_4("Cpa CkptOpen Api failed with return value:%d,ckptHandle:%llx", SA_AIS_ERR_INVALID_PARAM, ckptHandle);
		TRACE_LEAVE2("API return code = %u", rc);
		return SA_AIS_ERR_INVALID_PARAM;
	}

	m_CPSV_SET_SANAMET(checkpointName);

	/* SA_AIS_ERR_INVALID_PARAM, bullet 4 in SAI-AIS-CKPT-B.02.02 
           Section 3.6.1 saCkptCheckpointOpen() and saCkptCheckpointOpenAsync(), Return Values */
        if (strncmp((const char *)checkpointName->value, "safCkpt=", 8) != 0) {
                TRACE_4("Cpa CkptOpen:DN failed with return value:%d,ckptHandle:%llx", SA_AIS_ERR_INVALID_PARAM, ckptHandle);
		TRACE_LEAVE2("API return code = %u", rc);
                return SA_AIS_ERR_INVALID_PARAM;
        }

	/* Draft Validations */
	rc = cpa_open_attr_validate(checkpointCreationAttributes, checkpointOpenFlags, checkpointName);
	if (rc != SA_AIS_OK) {
		/* No need to log, already logged inside the cpa_open_attr_validate */
		goto done;
	}

	m_CPA_RETRIEVE_CB(cb);
	if (!cb) {
		rc = SA_AIS_ERR_BAD_HANDLE;
		TRACE_4("Cpa CkptOpen:HDL_TAKE failed with return value:%d,ckptHandle:%llx", rc, ckptHandle);
		goto done;
	}

	/* get the CB Lock */
	if (m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
		rc = SA_AIS_ERR_LIBRARY;
		TRACE_4("Cpa CkptOpen:LOCK failed with return value:%d,ckptHandle:%llx", rc, ckptHandle);
		goto lock_fail;
	}
	locked = true;

	/* Get the Client info */
	rc = cpa_client_node_get(&cb->client_tree, &ckptHandle, &cl_node);
	if (!cl_node) {
		rc = SA_AIS_ERR_BAD_HANDLE;
		TRACE_4("Cpa CkptOpen:client_node_get failed with return value:%d,ckptHandle:%llx", rc, ckptHandle);
		goto client_not_found;
	}

	/* Checking Node availability */
	if (m_CPA_VER_IS_ABOVE_B_1_1(&cl_node->version)) {
		if ((cb->is_cpnd_joined_clm != true)  || ( cl_node->stale == true)) {
			TRACE_4("Cpa CLM Node left Api failed with return value:%d",SA_AIS_ERR_UNAVAILABLE);
			rc = SA_AIS_ERR_UNAVAILABLE;
			if (locked)
				m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);

			locked = false;
			goto clm_left;
		}
	}

	/* Allocate the CPA_LOCAL_CKPT_NODE & Populate lcl_ckpt_hdl with self 
	   pointer and also fill app_hdl */
	lc_node = (CPA_LOCAL_CKPT_NODE *)m_MMGR_ALLOC_CPA_LOCAL_CKPT_NODE;
	if (lc_node == NULL) {
		rc = SA_AIS_ERR_NO_MEMORY;
		TRACE_4("Cpa CkptOpen:CPA_LOCAL_CKPT_NODE memory alloc failed with return value:%d,ckptHandle:%llx", rc, ckptHandle);
		goto lc_node_alloc_fail;
	}

	memset(lc_node, 0, sizeof(CPA_LOCAL_CKPT_NODE));
	lc_node->lcl_ckpt_hdl = NCS_PTR_TO_UNS64_CAST(lc_node);
	lc_node->cl_hdl = ckptHandle;
	lc_node->open_flags = checkpointOpenFlags;

	lc_node->ckpt_name = *checkpointName;

	/* Add CPA_LOCAL_CKPT_NODE to lcl_ckpt_hdl_tree */
	proc_rc = cpa_lcl_ckpt_node_add(&cb->lcl_ckpt_tree, lc_node);

	if (proc_rc != NCSCC_RC_SUCCESS) {
		rc = SA_AIS_ERR_LIBRARY;
		TRACE_4("Cpa CkptOpen:lcl_ckpt_node_add failed with return value:%d,ckptHandle:%llx,lcl_ckpt_hdl:%llx", 
			proc_rc,ckptHandle, lc_node->lcl_ckpt_hdl);
		goto lc_node_add_fail;
	}

	/* Populate & Send the Open Event to CPND */
	memset(&evt, 0, sizeof(CPSV_EVT));
	evt.type = CPSV_EVT_TYPE_CPND;
	evt.info.cpnd.type = CPND_EVT_A2ND_CKPT_OPEN;
	evt.info.cpnd.info.openReq.client_hdl = ckptHandle;
	evt.info.cpnd.info.openReq.lcl_ckpt_hdl = lc_node->lcl_ckpt_hdl;

	evt.info.cpnd.info.openReq.ckpt_name = *checkpointName;

	if (checkpointCreationAttributes) {
		evt.info.cpnd.info.openReq.ckpt_attrib = *checkpointCreationAttributes;
		evt.info.cpnd.info.openReq.ckpt_attrib.retentionDuration =
		    checkpointCreationAttributes->retentionDuration;

	}

	evt.info.cpnd.info.openReq.ckpt_flags = checkpointOpenFlags;

	/* convert the timeout to 10 ms value and add it to the sync send timeout */
	time_out = m_CPSV_CONVERT_SATIME_TEN_MILLI_SEC(timeout);

	if (time_out < NCS_SAF_MIN_ACCEPT_TIME) {
		rc = SA_AIS_ERR_TIMEOUT;
		TRACE_4("Cpa CkptOpen failed Api failed with return value:%d,ckptHandle:%llx,timeout:%llu", rc, ckptHandle, timeout);
		goto mds_send_fail;
	}

	evt.info.cpnd.info.openReq.timeout = timeout;

	/* Unlock before MDS Send */
	if (locked)
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
	locked = false;

	/* CPND GOES DOWN */
	if (false == cb->is_cpnd_up) {
		rc = SA_AIS_ERR_TRY_AGAIN;
		TRACE_4("cpa CkptOpen:CPND_DOWN Api failed with return value:%d,ckptHandle:%llx", rc, ckptHandle);
		goto mds_send_fail;
	}

       /* Send the evt to CPND */
       proc_rc = cpa_mds_msg_sync_send(cb->cpa_mds_hdl, &(cb->cpnd_mds_dest), &evt, &out_evt, timeout);

       /* Generate rc from proc_rc */
       switch (proc_rc) {
               case NCSCC_RC_SUCCESS:
                       break;
               case NCSCC_RC_REQ_TIMOUT:
                       rc = SA_AIS_ERR_TIMEOUT;
			TRACE_4("Cpa CkptOpen:MDS api failed with return value:%d,ckptHandle:%llx,cpnd_mds_dest:%"PRIu64,
                        proc_rc , ckptHandle, cb->cpnd_mds_dest);
                       goto mds_send_fail;
               default:
                       rc = SA_AIS_ERR_TRY_AGAIN;
			TRACE_4("cpa CkptOpen:MDS api failed with return value:%d,ckptHandle:%llx,cpnd_mds_dest:%"PRIu64,
                        proc_rc , ckptHandle , cb->cpnd_mds_dest);
                       goto mds_send_fail;
       }

       if(out_evt)
       {
               bool   add_flag = true;
               /* Process the received Event */

               rc = out_evt->info.cpa.info.openRsp.error;
               lc_node->gbl_ckpt_hdl = out_evt->info.cpa.info.openRsp.gbl_ckpt_hdl;


               if(rc != SA_AIS_OK)
               {
                       TRACE_4("Cpa CkptOpen failed with return value:%d,ckptHandle:%llx", rc , ckptHandle);
                       /* Free the Local Ckpt Node */
                       m_MMGR_FREE_CPSV_EVT(out_evt, NCS_SERVICE_ID_CPA);
                       goto lcl_ckpt_node_free;
               }

               /* Take the CB lock */
               if (m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS)
               {
                       rc = SA_AIS_ERR_LIBRARY;
                       TRACE_4("Cpa CkptOpen:LOCK failed with return value:%d,ckptHandle:%llx,gbl_ckpt_hdl:%llx", 
				rc , ckptHandle, lc_node->gbl_ckpt_hdl);
                       goto lock_fail1;
               }

               locked = true;

               proc_rc = cpa_gbl_ckpt_node_find_add(&cb->gbl_ckpt_tree,
                               &lc_node->gbl_ckpt_hdl, &gc_node, &add_flag);


               if(proc_rc != NCSCC_RC_SUCCESS)
               {
                       rc = SA_AIS_ERR_NO_MEMORY;
                       TRACE_4("Cpa CkptOpen failed with return value:%d,ckptHandle:%llx,gbl_ckpt_hdl:%llx",
				 proc_rc , ckptHandle, lc_node->gbl_ckpt_hdl);
                       goto gl_node_add_fail;
               }

               if ( add_flag == false)
               {
                       gc_node->open.info.open.o_addr=out_evt->info.cpa.info.openRsp.addr;
                       gc_node->ckpt_creat_attri = out_evt->info.cpa.info.openRsp.creation_attr;

                       /*To store the active MDS_DEST info of checkpoint */
                       if(out_evt->info.cpa.info.openRsp.is_active_exists)
                       {
                               gc_node->is_active_exists = true;
                               gc_node->active_mds_dest = out_evt->info.cpa.info.openRsp.active_dest;
                       }

               }

               gc_node->ref_cnt++;
               /* Free the out event */
               m_MMGR_FREE_CPSV_EVT(out_evt, NCS_SERVICE_ID_CPA);
       }

       	*checkpointHandle = lc_node->lcl_ckpt_hdl;

	if (locked)
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
	locked = false;
	m_CPA_GIVEUP_CB;
	TRACE_1("Cpa CkptOpen Api Success with ckptHandle:%llx,checkpointHandle:%llx,gbl_ckpt_hdl:%llx",
		ckptHandle, *checkpointHandle, lc_node->gbl_ckpt_hdl);

	TRACE_LEAVE2("API return code = %u", rc);

	return SA_AIS_OK;

	/* Error Handling */

lock_fail1: 
gl_node_add_fail:

	/* Call the Ckpt close Processing */
	if (locked)
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
	m_CPA_GIVEUP_CB;
	saCkptCheckpointClose(*checkpointHandle);
	TRACE_LEAVE2("API return code = %u", rc);
	return rc;

 mds_send_fail:
 lcl_ckpt_node_free:
	if ((!locked) && (m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS)) {
		rc = SA_AIS_ERR_LIBRARY;
		TRACE_4("Cpa CkptOpen:LOCK failed with return value:%d,ckptHandle:%llx", rc, ckptHandle);
		goto lock_fail;
	} else
		locked = true;

	cpa_lcl_ckpt_node_delete(cb, lc_node);
	lc_node = NULL;

 lc_node_add_fail:
	if (lc_node != NULL) {
		m_MMGR_FREE_CPA_LOCAL_CKPT_NODE(lc_node);
	}

 lc_node_alloc_fail:
	/* Do Nothing */

 client_not_found:
	if (locked)
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);

 lock_fail:
 clm_left:
	m_CPA_GIVEUP_CB;

 done:
	TRACE_LEAVE2("API return code = %u", rc);
	return rc;

}

/****************************************************************************
  Name          :  saCkptCheckpointOpenAsync
 
  Description   :  Open a Checkpoint, a new checkpoint handle is returned upon
                   completion. A Checkpoint can be opened multiple times for
                   reading and or writing in the same or different process.
 
  Arguments     :  ckptHandle - Checkpoint Service handle
                   invocation - Designates a particular invocation of the 
                                response callback.
                   checkpointName - A pointer to the name of the checkpoint
                           that identifies a checkpoint globally in a cluster.
                   checkpointCreationAttributes - A pointer to the creation 
                           attributes of a checkpoint.
                   checkpointOpenFlags - Flags defined by the
                                  SaCkptCheckpointOpenFlagsT type.
 
  Return Values :  Refer to SAI-AIS specification for various return values.
 
  Notes         :
******************************************************************************/
SaAisErrorT saCkptCheckpointOpenAsync(SaCkptHandleT ckptHandle, SaInvocationT invocation, const SaNameT *checkpointName, const SaCkptCheckpointCreationAttributesT
				      *checkpointCreationAttributes, SaCkptCheckpointOpenFlagsT checkpointOpenFlags)
{
	SaAisErrorT rc = SA_AIS_OK;
	bool locked = true;
	CPA_CB *cb = NULL;
	CPSV_EVT evt;
	CPA_LOCAL_CKPT_NODE *lc_node = NULL;
	CPA_CLIENT_NODE *cl_node = NULL;
	uint32_t proc_rc = NCSCC_RC_SUCCESS;
	
	TRACE_ENTER2("SaCkptCheckpointHandleT passed is %llx",ckptHandle);
	
	if (checkpointName == NULL) {
		TRACE_4("cpa CkptOpenAsync Api failed with return value:%d,ckptHandle:%llx", SA_AIS_ERR_INVALID_PARAM, ckptHandle);
		TRACE_LEAVE2("API return code = %u", rc);
		return SA_AIS_ERR_INVALID_PARAM;
	}

	/* Draft Validations */

	m_CPSV_SET_SANAMET(checkpointName);

	/* SA_AIS_ERR_INVALID_PARAM, bullet 4 in SAI-AIS-CKPT-B.02.02 
           Section 3.6.1 saCkptCheckpointOpen() and saCkptCheckpointOpenAsync(), Return Values */
        if (strncmp((const char *)checkpointName->value, "safCkpt=", 8) != 0) {
                TRACE_4("cpa CkptOpen:DN Api failed with return value:%d,ckptHandle:%llx", SA_AIS_ERR_INVALID_PARAM, ckptHandle);
		TRACE_LEAVE2("API return code = %u", rc);
                return SA_AIS_ERR_INVALID_PARAM;
        }

	rc = cpa_open_attr_validate(checkpointCreationAttributes, checkpointOpenFlags, checkpointName);
	if (rc != SA_AIS_OK) {
		/* No need to log, it is already logged inside */
		goto done;
	}

	m_CPA_RETRIEVE_CB(cb);
	if (!cb) {
		rc = SA_AIS_ERR_BAD_HANDLE;
		TRACE_4("cpa CkptOpenAsync:HDL_TAKE Api failed with return value:%d,ckptHandle:%llx ", rc, ckptHandle);
		goto done;
	}

	/* get the CB Lock */
	if (m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
		rc = SA_AIS_ERR_LIBRARY;
		TRACE_4("cpa CkptOpenAsync:LOCK Api failed with return value:%d,ckptHandle:%llx ", rc, ckptHandle);
		goto lock_fail;
	}

	/* Get the Client info */
	rc = cpa_client_node_get(&cb->client_tree, &ckptHandle, &cl_node);
	if (!cl_node) {
		rc = SA_AIS_ERR_BAD_HANDLE;
		TRACE_4("cpa CkptOpenAsync:client_node_get Api failed with return value:%d,ckptHandle:%llx", rc, ckptHandle);
		goto client_not_found;
	}

	/* Checking Node availability */
	if (m_CPA_VER_IS_ABOVE_B_1_1(&cl_node->version)) {
		if ((cb->is_cpnd_joined_clm != true)  || ( cl_node->stale == true)) {
			TRACE_4("cpa CLM Node left Api failed with return value:%d",SA_AIS_ERR_UNAVAILABLE);
			rc = SA_AIS_ERR_UNAVAILABLE;

			if (locked)
				m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);

			locked = false;
			goto clm_left;
		}
	}

	if (cl_node->ckpt_callbk.saCkptCheckpointOpenCallback == NULL) {
		rc = SA_AIS_ERR_INIT;
		TRACE_4("cpa CkptOpenAsync Api failed with return value:%d,ckptHandle:%llx", rc, ckptHandle);
		goto no_callback;
	}

	/* Allocate the CPA_LOCAL_CKPT_NODE & Populate lcl_ckpt_hdl with self 
	   pointer and also fill app_hdl */
	lc_node = (CPA_LOCAL_CKPT_NODE *)m_MMGR_ALLOC_CPA_LOCAL_CKPT_NODE;
	if (lc_node == NULL) {
		rc = SA_AIS_ERR_NO_MEMORY;
		TRACE_4("cpa memory alloc failed CkptOpenAsync with return value:%d,ckptHandle:%llx", rc, ckptHandle);
		goto lc_node_alloc_fail;
	}

	memset(lc_node, 0, sizeof(CPA_LOCAL_CKPT_NODE));
	lc_node->lcl_ckpt_hdl = NCS_PTR_TO_UNS64_CAST(lc_node);
	lc_node->cl_hdl = ckptHandle;
	lc_node->open_flags = checkpointOpenFlags;
	lc_node->ckpt_name = *checkpointName;

	/* Add CPA_LOCAL_CKPT_NODE to lcl_ckpt_hdl_tree */
	proc_rc = cpa_lcl_ckpt_node_add(&cb->lcl_ckpt_tree, lc_node);

	if (proc_rc != NCSCC_RC_SUCCESS) {
		rc = SA_AIS_ERR_LIBRARY;
		TRACE_4("cpa CkptOpenAsync:lcl_ckpt_node_add Api failed with return value:%d,ckptHandle:%llx,lcl_ckpt_hdl:%llx", rc, ckptHandle,
				 lc_node->lcl_ckpt_hdl);
		goto lc_node_add_fail;
	}

	/* Populate & Send the Open Event to CPND */
	memset(&evt, 0, sizeof(CPSV_EVT));
	evt.type = CPSV_EVT_TYPE_CPND;
	evt.info.cpnd.type = CPND_EVT_A2ND_CKPT_OPEN;
	evt.info.cpnd.info.openReq.client_hdl = ckptHandle;
	evt.info.cpnd.info.openReq.lcl_ckpt_hdl = lc_node->lcl_ckpt_hdl;

	evt.info.cpnd.info.openReq.ckpt_name = *checkpointName;

	if (checkpointCreationAttributes) {
		evt.info.cpnd.info.openReq.ckpt_attrib = *checkpointCreationAttributes;
		evt.info.cpnd.info.openReq.ckpt_attrib.retentionDuration =
		    checkpointCreationAttributes->retentionDuration;
	}

	evt.info.cpnd.info.openReq.ckpt_flags = checkpointOpenFlags;
	evt.info.cpnd.info.openReq.invocation = invocation;

	/* Create the handle to protect the CPA_TMR structure (part of lcl_ckpt_node */
	if (lc_node->async_req_tmr.uarg) {
		/* create the association with hdl-mngr */
		if (!(lc_node->async_req_tmr.uarg = ncshm_create_hdl(cb->pool_id,
								     NCS_SERVICE_ID_CPA,
								     (NCSCONTEXT)&lc_node->async_req_tmr))) {
			TRACE_4("cpa CkptOpenAsync:HDL_CREATE Api failed with return value:%d,ckptHandle:%llx,lcl_ckpt_hdl:%llx", rc, ckptHandle,
					 lc_node->lcl_ckpt_hdl);
			rc = SA_AIS_ERR_LIBRARY;
			goto hm_create_fail;
		}
	}

	lc_node->async_req_tmr.type = CPA_TMR_TYPE_OPEN;
	lc_node->async_req_tmr.info.ckpt.invocation = invocation;
	lc_node->async_req_tmr.info.ckpt.client_hdl = ckptHandle;
	lc_node->async_req_tmr.info.ckpt.lcl_ckpt_hdl = lc_node->lcl_ckpt_hdl;

	/* Start the timer with value timeout */
	cpa_tmr_start(&lc_node->async_req_tmr, CPA_OPEN_ASYNC_WAIT_TIME);

	/* Unlock before MDS Send */
	if (locked)
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
	locked = false;

	/* CPND DOWN */
	if (false == cb->is_cpnd_up) {
		rc = SA_AIS_ERR_TRY_AGAIN;
		TRACE_4("cpa CkptOpenAsync:CPND_DOWN Api failed with return value:%d,ckptHandle:%llx,lcl_ckpt_hdl:%llx", 
			rc, ckptHandle, lc_node->lcl_ckpt_hdl);
		goto mds_send_fail;
	}

	proc_rc = cpa_mds_msg_send(cb->cpa_mds_hdl, &cb->cpnd_mds_dest, &evt, NCSMDS_SVC_ID_CPND);

	/* Generate rc from proc_rc */
	switch (proc_rc) {
	case NCSCC_RC_SUCCESS:
		break;
	case NCSCC_RC_REQ_TIMOUT:
		rc = SA_AIS_ERR_TIMEOUT;
		TRACE_4("cpa CkptOpenAsync:MDS return value:%d,ckptHandle:%llx,cpnd_mds_dest:%"PRIu64, 
			proc_rc, ckptHandle, cb->cpnd_mds_dest);
		goto mds_send_fail;
	default:
		rc = SA_AIS_ERR_NO_RESOURCES;
		TRACE_4("cpa CkptOpenAsync:MDS Api failed with return value:%d,ckptHandle:%llx,cpnd_mds_dest:%"PRIu64, 
			proc_rc, ckptHandle, cb->cpnd_mds_dest);
		goto mds_send_fail;
	}

	m_CPA_GIVEUP_CB;
	TRACE_1("cpa CkptOpenAsync  api success with return value:%d,ckptHandle:%llx",SA_AIS_OK, ckptHandle);
	TRACE_LEAVE2("API return code = %u", rc);
	return SA_AIS_OK;

 mds_send_fail:
 hm_create_fail:

	if (m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
		rc = SA_AIS_ERR_LIBRARY;
		TRACE_4("cpa CkptOpenAsync:LOCK Api failed with return value:%d,ckptHandle:%llx", rc, ckptHandle);
		goto lock_fail;
	} else
		locked = true;

	cpa_lcl_ckpt_node_get(&cb->lcl_ckpt_tree, &lc_node->lcl_ckpt_hdl, &lc_node);

	cpa_lcl_ckpt_node_delete(cb, lc_node);
	lc_node = NULL;

 lc_node_add_fail:
	if (lc_node != NULL) {
		m_MMGR_FREE_CPA_LOCAL_CKPT_NODE(lc_node);
	}

 lc_node_alloc_fail:
	/* Do Nothing */
 no_callback:
	/* Do Nothing */
 client_not_found:
	if (locked)
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);

 lock_fail:
 clm_left:
	m_CPA_GIVEUP_CB;

 done:
	TRACE_LEAVE2("API return code = %u", rc);
	return rc;
}

/****************************************************************************
  Name          :  saCkptCheckpointClose
 
  Description   :  This function frees the resources allocated by the previous
                   invocation of saCkptCheckpointOpen or
                   saCkptCheckpointOpenAsync()
 
  Arguments     :  checkpointHandle - The handle that designates the checkpoint 
                                      to close.
 
  Return Values :  Refer to SAI-AIS specification for various return values.
 
  Notes         :
****************************************************************************/
SaAisErrorT saCkptCheckpointClose(SaCkptCheckpointHandleT checkpointHandle)
{
	SaAisErrorT rc = SA_AIS_OK;
	CPSV_EVT evt;
	CPSV_EVT *out_evt = NULL;
	uint32_t proc_rc = NCSCC_RC_SUCCESS;
	CPA_LOCAL_CKPT_NODE *lc_node = NULL;
	CPA_GLOBAL_CKPT_NODE *gc_node = NULL;
	CPA_CB *cb = NULL;
	bool add_flag = false;
	CPA_SECT_ITER_NODE *sect_iter_node = NULL;
	CPA_CLIENT_NODE *cl_node = NULL;

	TRACE_ENTER2("SaCkptCheckpointHandleT passed is %llx",checkpointHandle);
	m_CPA_RETRIEVE_CB(cb);
	if (!cb) {
		rc = SA_AIS_ERR_BAD_HANDLE;
		TRACE_4("cpa CkptClose:HDL_TAKE Api failed with return value:%d,checkpointHandle:%llx ", rc, checkpointHandle);
		goto done;
	}

	/* get the CB Lock */
	if (m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
		rc = SA_AIS_ERR_LIBRARY;
		TRACE_4("cpa with CkptClose:LOCK return value:%d,ckptHandle:%llx", rc, checkpointHandle);
		goto lock_fail;
	}

	proc_rc = cpa_lcl_ckpt_node_get(&cb->lcl_ckpt_tree, &checkpointHandle, &lc_node);

	if (!lc_node) {
		rc = SA_AIS_ERR_BAD_HANDLE;
		TRACE_4("cpa with CkptClose:ckpt_node_get return value:%d,ckptHandle:%llx ", proc_rc, checkpointHandle);
		goto fail1;
	}

	proc_rc = cpa_client_node_get(&cb->client_tree, &lc_node->cl_hdl, &cl_node);

	if (!cl_node) {
		TRACE_4("cpa with SelObjGet:client_node_get return value:%d,cl_hdl:%llx",SA_AIS_ERR_BAD_HANDLE, lc_node->cl_hdl);
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto fail1;
	}

	/* Checking Node availability */
	if (m_CPA_VER_IS_ABOVE_B_1_1(&cl_node->version)) {
		if ((cb->is_cpnd_joined_clm != true)  || ( cl_node->stale == true)) {
			TRACE_4("cpa with CLM Node left Api failed with return value:%d", SA_AIS_ERR_UNAVAILABLE);
			rc = SA_AIS_ERR_UNAVAILABLE;
			m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
			goto clm_left;
		}
	}

	/* Populate evt.info.cpnd.info.closeReq &Call MDS sync Send */
	memset(&evt, 0, sizeof(CPSV_EVT));
	evt.type = CPSV_EVT_TYPE_CPND;
	evt.info.cpnd.type = CPND_EVT_A2ND_CKPT_CLOSE;
	evt.info.cpnd.info.closeReq.client_hdl = lc_node->cl_hdl;
	evt.info.cpnd.info.closeReq.ckpt_id = lc_node->gbl_ckpt_hdl;
	evt.info.cpnd.info.closeReq.ckpt_flags = lc_node->open_flags;

	/* Unlock before MDS send */
	m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);

	/* CPND IS DOWN */
	if (false == cb->is_cpnd_up) {
		rc = SA_AIS_ERR_TRY_AGAIN;
		TRACE_4("cpa CkptClose:CPND_DOWN Api failed with return value:%d,ckptHandle:%llx", rc, checkpointHandle);
		goto mds_send_fail;
	}

	proc_rc = cpa_mds_msg_sync_send(cb->cpa_mds_hdl, &(cb->cpnd_mds_dest), &evt, &out_evt, CPSV_WAIT_TIME);

	/* Generate rc from proc_rc */
	switch (proc_rc) {
	case NCSCC_RC_SUCCESS:
		break;
	case NCSCC_RC_REQ_TIMOUT:
		rc = SA_AIS_ERR_TIMEOUT;
		TRACE_4("cpa CkptClose:MDS api failed with return value:%d,ckptHandle:%llx,cpnd_mds_dest:%"PRIu64,
                        proc_rc , checkpointHandle, cb->cpnd_mds_dest);
		goto mds_send_fail;
	default:
		rc = SA_AIS_ERR_TRY_AGAIN;
		TRACE_4("cpa CkptClose:MDS api failed with return value:%d,ckptHandle:%llx,cpnd_mds_dest:%"PRIu64,
                        proc_rc , checkpointHandle, cb->cpnd_mds_dest);
		goto mds_send_fail;
	}

	if (out_evt) {
		rc = out_evt->info.cpa.info.closeRsp.error;
		m_MMGR_FREE_CPSV_EVT(out_evt, NCS_SERVICE_ID_CPA);
	} else {
		rc = SA_AIS_ERR_NO_RESOURCES;
		TRACE_4("Cpa CkptClose Api failed with return value:%d,ckptHandle:%llx", rc, checkpointHandle);
	}

 mds_send_fail:
	if (rc != SA_AIS_OK) {
		m_CPA_GIVEUP_CB;
		TRACE_LEAVE2("API return code = %u", rc);
		return rc;
	}

	/* get the CB Lock */
	if (m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
		rc = SA_AIS_ERR_LIBRARY;
		TRACE_4("Cpa CkptClose:LOCK Api failed with return value:%d,ckptHandle:%llx", rc, checkpointHandle);
		goto lock_fail;
	}

	proc_rc = cpa_lcl_ckpt_node_get(&cb->lcl_ckpt_tree, &checkpointHandle, &lc_node);

	if (lc_node == NULL) {
		/* lc_node got deleted, */
		rc = SA_AIS_OK;
		goto fail1;
	} else {
		if (lc_node->sect_iter_cnt) {
			cpa_sect_iter_node_getnext(&cb->sect_iter_tree, 0, &sect_iter_node);
			while (sect_iter_node != NULL) {
				if (checkpointHandle == sect_iter_node->lcl_ckpt_hdl) {
					cpa_sect_iter_node_delete(cb, sect_iter_node);
					cpa_sect_iter_node_getnext(&cb->sect_iter_tree, 0, &sect_iter_node);
				} else
					cpa_sect_iter_node_getnext(&cb->sect_iter_tree, &sect_iter_node->iter_id,
								   &sect_iter_node);
			}
		}
	}

	/* Get the global ckpt node */
	cpa_gbl_ckpt_node_find_add(&cb->gbl_ckpt_tree, &lc_node->gbl_ckpt_hdl, &gc_node, &add_flag);

	/* Delete the global ckpt node */
	if (gc_node) {
		gc_node->ref_cnt--;
		if (!gc_node->ref_cnt)
			cpa_gbl_ckpt_node_delete(cb, gc_node);
	}

	/* Delete the local ckpt node */
	cpa_lcl_ckpt_node_delete(cb, lc_node);

 fail1:
	m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);

 lock_fail:
 clm_left:
	m_CPA_GIVEUP_CB;

 done:
	if (rc == SA_AIS_OK) {
		TRACE_1("Cpa CkptClose Api Success with return value:%d,ckptHandle:%llx", rc, checkpointHandle);
	}

	TRACE_LEAVE2("API return code = %u", rc);
	return rc;

}

/****************************************************************************
  Name          :  saCkptCheckpointUnlink
 
  Description   :  This function deletes an existing checkpoint identified by
                   checkpointName.
 
  Arguments     :  ckptHandle - Checkpoint Service handle
                   checkpointName - A pointer to the name of the checkpoint
                              that is to be unlinked.
 
  Return Values :  Refer to SAI-AIS specification for various return values.
 
  Notes         :
******************************************************************************/
SaAisErrorT saCkptCheckpointUnlink(SaCkptHandleT ckptHandle, const SaNameT *checkpointName)
{
	SaAisErrorT rc = SA_AIS_OK;
	CPSV_EVT evt;
	CPSV_EVT *out_evt = NULL;
	uint32_t proc_rc = NCSCC_RC_SUCCESS;
	CPA_CLIENT_NODE *cl_node = NULL;
	CPA_CB *cb = NULL;

	TRACE_ENTER2("SaCkptCheckpointHandleT passed is %llx",ckptHandle);
	/* For unlink CPA will not perform any operation other than passing
	   the request to CPND */
	if (checkpointName == NULL) {
		rc = SA_AIS_ERR_INVALID_PARAM;
		TRACE_4("Cpa CkptUnlink  api failed with return value:%d,ckptHandle:%llx", rc, ckptHandle);
		return rc;
	}

	m_CPSV_SET_SANAMET(checkpointName);

	/* retrieve CPA CB */
	m_CPA_RETRIEVE_CB(cb);
	if (!cb) {
		rc = SA_AIS_ERR_BAD_HANDLE;
		TRACE_4("Cpa CkptUnlink:HDL_TAKE failed with return value:%d,ckptHandle:%llx", rc, ckptHandle);
		goto done;
	}

	/* get the CB Lock */
	if (m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
		rc = SA_AIS_ERR_LIBRARY;
		TRACE_4("Cpa CkptUnlink:LOCK Api failed with return value:%d,ckptHandle:%llx", rc, ckptHandle);
		goto lock_fail;
	}

	/* Get the Client info */
	rc = cpa_client_node_get(&cb->client_tree, &ckptHandle, &cl_node);
	if (!cl_node) {
		rc = SA_AIS_ERR_BAD_HANDLE;
		TRACE_4("cpa CkptUnlink::client_node_get Api failed with return value:%d,ckptHandle:%llx", rc, ckptHandle);
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
		goto fail1;
	}
	/* Checking Node availability */
	if (m_CPA_VER_IS_ABOVE_B_1_1(&cl_node->version)) {
		if ((cb->is_cpnd_joined_clm != true)  || ( cl_node->stale == true)) {
			TRACE_4("cpa CLM Node left with return value:%d ", SA_AIS_ERR_UNAVAILABLE);
			rc = SA_AIS_ERR_UNAVAILABLE;
			m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
			goto clm_left;
		}
	}
	m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);

	/* Populate evt.info.cpnd.info.unlinkReq & Call MDS sync Send */
	memset(&evt, 0, sizeof(CPSV_EVT));
	evt.type = CPSV_EVT_TYPE_CPND;
	evt.info.cpnd.type = CPND_EVT_A2ND_CKPT_UNLINK;

	/*  evt.info.cpnd.info.ulinkReq.ckpt_name.length = checkpointName->length;
	   memcpy(evt.info.cpnd.info.ulinkReq.ckpt_name.value,checkpointName->value,checkpointName->length);   */

	evt.info.cpnd.info.ulinkReq.ckpt_name = *checkpointName;

	/* IF CPND IS DOWN  */
	if (false == cb->is_cpnd_up) {
		rc = SA_AIS_ERR_TRY_AGAIN;
		TRACE_4("Cpa CkptUnlink:LOCK Api failed with return value:%d,ckptHandle:%llx", rc, ckptHandle);
		goto fail1;
	}
	proc_rc = cpa_mds_msg_sync_send(cb->cpa_mds_hdl, &(cb->cpnd_mds_dest), &evt, &out_evt, CPSV_WAIT_TIME);

	/* Generate rc from proc_rc */
	switch (proc_rc) {
	case NCSCC_RC_SUCCESS:
		break;
	case NCSCC_RC_REQ_TIMOUT:
		rc = SA_AIS_ERR_TIMEOUT;
		TRACE_4("cpa CkptUnlink:MDS api failed with return value:%d,ckptHandle:%llx,cpnd_mds_dest:%"PRIu64,
                        proc_rc , ckptHandle, cb->cpnd_mds_dest);
		goto fail1;
	default:
		rc = SA_AIS_ERR_TRY_AGAIN;
		TRACE_4("cpa CkptUnlink:MDS api failed with return value:%d,ckptHandle:%llx,cpnd_mds_dest:%"PRIu64,
                        proc_rc , ckptHandle, cb->cpnd_mds_dest);
		goto fail1;
	}

	if (out_evt) {
		rc = out_evt->info.cpa.info.ulinkRsp.error;
		m_MMGR_FREE_CPSV_EVT(out_evt, NCS_SERVICE_ID_CPA);
	} else {
		rc = SA_AIS_ERR_NO_RESOURCES;
		TRACE_4("cpa CkptUnlink Api failed with return value:%d,ckptHandle:%llx", rc, ckptHandle);
	}

 fail1:
 lock_fail:
 clm_left:
	m_CPA_GIVEUP_CB;

 done:
	if (rc == SA_AIS_OK) {
		TRACE_1("Cpa CkptUnlink Api Success with return value:%d,ckptHandle:%llx", rc, ckptHandle);
	}
	TRACE_LEAVE2("API return code = %u", rc);
	return rc;
}

/****************************************************************************
  Name          :  saCkptCheckpointRetentionDurationSet
 
  Description   :  This function sets the retention duration of the Checkpoint.
 
  Arguments     :  checkpointHandle - The checkpoint whose retention time is 
                                      being set.
                   retentionDuration - The value of the retention duration to
                                      be set.
                   
  Return Values :  Refer to SAI-AIS specification for various return values.
 
  Notes         :
******************************************************************************/
SaAisErrorT saCkptCheckpointRetentionDurationSet(SaCkptCheckpointHandleT checkpointHandle, SaTimeT retentionDuration)
{
	SaAisErrorT rc = SA_AIS_OK;
	CPSV_EVT evt;
	CPSV_EVT *out_evt = NULL;
	uint32_t proc_rc = NCSCC_RC_SUCCESS;
	CPA_LOCAL_CKPT_NODE *lc_node = NULL;
	CPA_CB *cb = NULL;
	CPA_CLIENT_NODE *cl_node = NULL;

	TRACE_ENTER2("SaCkptCheckpointHandleT passed is %llx",checkpointHandle);
	/* For Retention duration set CPA will not perform any operation 
	   other than passing the request to CPND */

	/* retrieve CPA CB */
	m_CPA_RETRIEVE_CB(cb);
	if (!cb) {
		rc = SA_AIS_ERR_BAD_HANDLE;
		TRACE_4("Cpa RetDurSet:HDL_TAKE failed with return value:%d,ckptHandle:%llx", rc, checkpointHandle);
		goto done;
	}

	/* get the CB Lock */
	if (m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
		rc = SA_AIS_ERR_LIBRARY;
		TRACE_4("Cpa RetDurSet:LOCK failed with return value:%d,ckptHandle:%llx", rc, checkpointHandle);
		goto lock_fail;
	}

	/* CPND STATUS */

	proc_rc = cpa_lcl_ckpt_node_get(&cb->lcl_ckpt_tree, &checkpointHandle, &lc_node);
	if (!lc_node) {
		rc = SA_AIS_ERR_BAD_HANDLE;
		TRACE_4("Cpa RetDurSet failed with return value:%d,ckptHandle:%llx", proc_rc, checkpointHandle);
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
		goto fail1;
	}

	proc_rc = cpa_client_node_get(&cb->client_tree, &lc_node->cl_hdl, &cl_node);

	if (!cl_node) {
		TRACE_4("Cpa SelObjGet:client_node_get failed with return value:%d,cl_hdl:%llx",SA_AIS_ERR_BAD_HANDLE, lc_node->cl_hdl);
		rc = SA_AIS_ERR_BAD_HANDLE;
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
		goto fail1;
	}

	/* Checking Node availability */
	if (m_CPA_VER_IS_ABOVE_B_1_1(&cl_node->version)) {

		if ((cb->is_cpnd_joined_clm != true)  || ( cl_node->stale == true)) {
			TRACE_4("Cpa CLM Node left: return value:%d",SA_AIS_ERR_UNAVAILABLE);
			rc = SA_AIS_ERR_UNAVAILABLE;
			m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
			goto clm_left;
		}

		if (!(lc_node->open_flags & SA_CKPT_CHECKPOINT_WRITE)) {
			rc = SA_AIS_ERR_ACCESS;
			TRACE_4("Cpa RetDurSet failed with return value:%d,ckptHandle:%llx", rc, checkpointHandle);
			m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
			goto fail1;
		}

	}
	/* Populate the event & send it to CPND */
	memset(&evt, 0, sizeof(CPSV_EVT));
	evt.type = CPSV_EVT_TYPE_CPND;
	evt.info.cpnd.type = CPND_EVT_A2ND_CKPT_RDSET;
	evt.info.cpnd.info.rdsetReq.ckpt_id = lc_node->gbl_ckpt_hdl;

	/* Convert the retentionDuration from saTimeT to millisecs */
/*   retentionDuration = m_CPSV_CONVERT_SATIME_TEN_MILLI_SEC(retentionDuration); */
	evt.info.cpnd.info.rdsetReq.reten_time = retentionDuration;

	m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);

	/* IF CPND IS DOWN  */
	if (false == cb->is_cpnd_up) {
		rc = SA_AIS_ERR_TRY_AGAIN;
		TRACE_4("Cpa RetDurSet:CPND_DOWN api failed with return value:%d,ckptHandle:%llx", rc, checkpointHandle);
		goto fail1;
	}
	proc_rc = cpa_mds_msg_sync_send(cb->cpa_mds_hdl, &(cb->cpnd_mds_dest), &evt, &out_evt, CPSV_WAIT_TIME);

	/* Generate rc from proc_rc */
	switch (proc_rc) {
	case NCSCC_RC_SUCCESS:
		break;
	case NCSCC_RC_REQ_TIMOUT:
		rc = SA_AIS_ERR_TIMEOUT;
		TRACE_4("cpa RetDurSet:MDS api failed with return value:%d,ckptHandle:%llx,cpnd_mds_dest:%"PRIu64,
                        proc_rc , checkpointHandle, cb->cpnd_mds_dest);
		goto fail1;
	default:
		rc = SA_AIS_ERR_TRY_AGAIN;
		TRACE_4("cpa RetDurSet:MDS api failed with return value:%d,ckptHandle:%llx,cpnd_mds_dest:%"PRIu64,
                        proc_rc, checkpointHandle, cb->cpnd_mds_dest);
		goto fail1;
	}

	if (out_evt) {
		rc = out_evt->info.cpa.info.rdsetRsp.error;
		m_MMGR_FREE_CPSV_EVT(out_evt, NCS_SERVICE_ID_CPA);
	} else {
		rc = SA_AIS_ERR_NO_RESOURCES;
		TRACE_4("Cpa RetDurSet Api failed with return value:%d,ckptHandle:%llx", rc, checkpointHandle);
	}

 fail1:
 lock_fail:
 clm_left:
	m_CPA_GIVEUP_CB;

 done:
	if (rc == SA_AIS_OK) {
		TRACE_1("Cpa RetDurSet Api Success with return value:%d,ckptHandle:%llx", rc, checkpointHandle);
	}
	TRACE_LEAVE2("API return code = %u", rc);
	return rc;
}

/****************************************************************************
  Name          :  saCkptActiveReplicaSet
 
  Description   :  This function can only be used for checkpoint that have
                   been created with the SA_CKPT_CHECKPOINT_COLLOCATED flag.
                   The local checkpoint replica will become the active replica
                   after invocation of this function.
 
  Arguments     :  checkpointHandle - handle that designates the checkpoint.
 
  Return Values :  Refer to SAI-AIS specification for various return values.
 
  Notes         :
******************************************************************************/
SaAisErrorT saCkptActiveReplicaSet(SaCkptCheckpointHandleT checkpointHandle)
{
	SaAisErrorT rc = SA_AIS_OK;
	CPSV_EVT evt;
	CPSV_EVT *out_evt = NULL;
	uint32_t proc_rc = NCSCC_RC_SUCCESS;
	CPA_LOCAL_CKPT_NODE *lc_node = NULL;
	CPA_GLOBAL_CKPT_NODE *gc_node = NULL;
	bool add_flag = false;
	CPA_CB *cb = NULL;
	CPA_CLIENT_NODE *cl_node = NULL;

	TRACE_ENTER2("SaCkptCheckpointHandleT passed is %llx",checkpointHandle);
	/* For Active Replica set CPA will not perform any operation 
	   other than passing the request to CPND */

	/* retrieve CPA CB */
	m_CPA_RETRIEVE_CB(cb);
	if (!cb) {
		rc = SA_AIS_ERR_BAD_HANDLE;
		TRACE_4("Cpa ActiveRepSet failed with return value:%d,ckptHandle:%llx", rc, checkpointHandle);
		goto done;
	}

	/* get the CB Lock */
	if (m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
		rc = SA_AIS_ERR_LIBRARY;
		TRACE_4("Cpa ActiveRepSet:LOCK failed with return value:%d,ckptHandle:%llx", rc, checkpointHandle);
		goto lock_fail;
	}

	proc_rc = cpa_lcl_ckpt_node_get(&cb->lcl_ckpt_tree, &checkpointHandle, &lc_node);
	if (!lc_node) {
		rc = SA_AIS_ERR_BAD_HANDLE;
		TRACE_4("Cpa ActiveRepSet:lcl_ckpt_node_get failed with return value:%d,ckptHandle:%llx", rc, checkpointHandle);
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
		goto fail1;
	}

	proc_rc = cpa_client_node_get(&cb->client_tree, &lc_node->cl_hdl, &cl_node);

	if (!cl_node) {
		TRACE_4("Cpa SelObjGet:client_node_get Api failed with return value:%d,cl_hdl:%llx", SA_AIS_ERR_BAD_HANDLE, lc_node->cl_hdl);
		rc = SA_AIS_ERR_BAD_HANDLE;
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
		goto fail1;
	}

	/* Checking Node availability */
	if (m_CPA_VER_IS_ABOVE_B_1_1(&cl_node->version)) {
		if ((cb->is_cpnd_joined_clm != true)  || ( cl_node->stale == true)) {
			TRACE_4("Cpa CLM Node left: with return value:%d",SA_AIS_ERR_UNAVAILABLE);
			rc = SA_AIS_ERR_UNAVAILABLE;
			m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
			goto clm_left;
		}
	}

	/* Don't allow Active replica Set, in case if SA_CKPT_CHECKPOINT_WRITE is 
	   not set */
	if (!(lc_node->open_flags & SA_CKPT_CHECKPOINT_WRITE)) {
		rc = SA_AIS_ERR_ACCESS;
		TRACE_4("Cpa ActiveRepSet failed with return value:%d,ckptHandle:%llx", rc, checkpointHandle);
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
		goto fail1;
	}

	/* Get the Global Ckpt node */
	proc_rc = cpa_gbl_ckpt_node_find_add(&cb->gbl_ckpt_tree, &lc_node->gbl_ckpt_hdl, &gc_node, &add_flag);
	if (!gc_node) {
		rc = SA_AIS_ERR_NO_MEMORY;
		TRACE_4("Cpa ActiveRepSet failed with return value:%d,ckptHandle:%llx", proc_rc, checkpointHandle);
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
		goto fail1;
	}

	/* Select the active replica */
	if (!(m_IS_SA_CKPT_CHECKPOINT_COLLOCATED(&gc_node->ckpt_creat_attri) &&
	      m_IS_ASYNC_UPDATE_OPTION(&gc_node->ckpt_creat_attri))) {
		rc = SA_AIS_ERR_BAD_OPERATION;
		TRACE_4("Cpa ActiveRepSet failed with return value:%d,ckptHandle:%llx", proc_rc, checkpointHandle);
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
		goto fail1;
	}

	gc_node->is_active_bcast_came = false;
	/* Populate the event & send it to CPND */
	memset(&evt, 0, sizeof(CPSV_EVT));
	evt.type = CPSV_EVT_TYPE_CPND;
	evt.info.cpnd.type = CPND_EVT_A2ND_CKPT_AREP_SET;
	evt.info.cpnd.info.arsetReq.ckpt_id = lc_node->gbl_ckpt_hdl;

	/* Unlock CB before MDS send */
	m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);

	/* IF CPND IS DOWN  */
	if (false == cb->is_cpnd_up) {
		rc = SA_AIS_ERR_TRY_AGAIN;
		TRACE_1("Cpa ActiveRepSet failed with return value:%d,ckptHandle:%llx", rc, checkpointHandle);
		goto fail1;
	}
	proc_rc = cpa_mds_msg_sync_send(cb->cpa_mds_hdl, &(cb->cpnd_mds_dest), &evt, &out_evt, CPSV_WAIT_TIME);

	/* Generate rc from proc_rc */
	switch (proc_rc) {
	case NCSCC_RC_SUCCESS:
		break;
	case NCSCC_RC_REQ_TIMOUT:
		rc = SA_AIS_ERR_TIMEOUT;
		TRACE_4("Cpa ActiveRepSet:MDS api failed with return value:%d,ckptHandle:%llx,cpnd_mds_dest:%"PRIu64,
                        proc_rc , checkpointHandle, cb->cpnd_mds_dest);
		goto fail1;
	default:
		rc = SA_AIS_ERR_TRY_AGAIN;
		TRACE_4("Cpa ActiveRepSet:MDS api failed with return value:%d,ckptHandle:%llx,cpnd_mds_dest:%"PRIu64,
                        proc_rc , checkpointHandle, cb->cpnd_mds_dest);
		goto fail1;
	}

	if (out_evt) {
		rc = out_evt->info.cpa.info.arsetRsp.error;
		m_MMGR_FREE_CPSV_EVT(out_evt, NCS_SERVICE_ID_CPA);
		if (rc == SA_AIS_OK) {
			cpa_sync_with_cpd_for_active_replica_set(gc_node);
			gc_node->is_active_exists = true;
			gc_node->active_mds_dest = cb->cpnd_mds_dest;
		}
	} else {
		rc = SA_AIS_ERR_NO_RESOURCES;
		TRACE_4("Cpa ActiveRepSet Api failed with return value:%d,ckptHandle:%llx", rc, checkpointHandle);
	}

 fail1:
 lock_fail:
 clm_left:
	m_CPA_GIVEUP_CB;

 done:

	if (rc == SA_AIS_OK) {
		TRACE_1("Cpa ActiveRepSet Api success with return value:%d,ckptHandle:%llx", rc, checkpointHandle);
	}

	TRACE_LEAVE2("API return code = %u", rc);
	return rc;
}

/****************************************************************************
  Name          :  saCkptCheckpointStatusGet
 
  Description   :  This function retrieves the checkpointStatus of the check-
                   point designated by the checkpointHandle.
 
  Arguments     :  checkpointHandle - Handle designates the checkpoint.
                   checkpointStatus - Status of the checkpoint.
 
  Return Values :  Refer to SAI-AIS specification for various return values.
 
  Notes         :
******************************************************************************/
SaAisErrorT saCkptCheckpointStatusGet(SaCkptCheckpointHandleT checkpointHandle,
				      SaCkptCheckpointDescriptorT *checkpointStatus)
{
	SaAisErrorT rc = SA_AIS_OK;
	CPSV_EVT evt;
	CPSV_EVT *out_evt = NULL;
	uint32_t proc_rc = NCSCC_RC_SUCCESS;
	CPA_LOCAL_CKPT_NODE *lc_node = NULL;
	CPA_CB *cb = NULL;
	CPA_GLOBAL_CKPT_NODE *gc_node = NULL;
	bool add_flag = false;
	bool is_local_get = false;
	CPA_CLIENT_NODE *cl_node = NULL;

	TRACE_ENTER2("SaCkptCheckpointHandleT passed is %llx",checkpointHandle);
	/* For Checkpoint status get CPA will not perform any operation 
	   other than passing the request to CPND */

	/* Check the input parameters */
	if (!checkpointStatus) {
		rc = SA_AIS_ERR_INVALID_PARAM;
		TRACE_4("Cpa StatusGet failed with return value:%d,ckptHandle:%llx", rc, checkpointHandle);
		goto done;
	}

	/* retrieve CPA CB */
	m_CPA_RETRIEVE_CB(cb);
	if (!cb) {
		rc = SA_AIS_ERR_BAD_HANDLE;
		TRACE_4("Cpa StatusGet:HDL_TAKE failed with return value:%d,ckptHandle:%llx", rc, checkpointHandle);
		goto done;
	}

	/* get the CB Lock */
	if (m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
		rc = SA_AIS_ERR_LIBRARY;
		TRACE_4("Cpa StatusGet:LOCK failed with return value:%d,ckptHandle:%llx", rc, checkpointHandle);
		goto lock_fail;
	}

	proc_rc = cpa_lcl_ckpt_node_get(&cb->lcl_ckpt_tree, &checkpointHandle, &lc_node);
	if (!lc_node) {
		rc = SA_AIS_ERR_BAD_HANDLE;
		TRACE_4("Cpa StatusGet failed with return value:%d,ckptHandle:%llx", rc, checkpointHandle);
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
		goto fail1;
	}
	proc_rc = cpa_client_node_get(&cb->client_tree, &lc_node->cl_hdl, &cl_node);

	if (!cl_node) {
		TRACE_4("Cpa SelObjGet:client_node_get failed with return value:%d,cl_hdl:%llx",SA_AIS_ERR_BAD_HANDLE, lc_node->cl_hdl);
		rc = SA_AIS_ERR_BAD_HANDLE;
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
		goto fail1;
	}

	/* Checking Node availability */
	if (m_CPA_VER_IS_ABOVE_B_1_1(&cl_node->version)) {
		if ((cb->is_cpnd_joined_clm != true)  || ( cl_node->stale == true)) {
			TRACE_4("Cpa CLM Node left, return value:%d",SA_AIS_ERR_UNAVAILABLE);
			rc = SA_AIS_ERR_UNAVAILABLE;
			m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
			goto clm_left;
		}
		if (!(lc_node->open_flags & SA_CKPT_CHECKPOINT_READ)) {
			rc = SA_AIS_ERR_ACCESS;
			TRACE_4("Cpa StatusGet Api failed with return value:%d,ckptHandle:%llx", rc, checkpointHandle);
			m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
			goto fail1;
		}
	}
	proc_rc = cpa_gbl_ckpt_node_find_add(&cb->gbl_ckpt_tree, &lc_node->gbl_ckpt_hdl, &gc_node, &add_flag);

	if (!gc_node) {
		TRACE_4("Cpa GBl ckpt find add failed for gbl_ckpt_hdl:%llx",lc_node->gbl_ckpt_hdl); 
                rc = SA_AIS_ERR_NO_RESOURCES;
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
		TRACE_4("Cpa StatusGet Api failed with return value:%d,ckptHandle:%llx", rc, checkpointHandle);
		goto fail1;
	}

	if (((m_CPA_IS_COLLOCATED_ATTR_SET(gc_node->ckpt_creat_attri.creationFlags) == true)
	     && (m_CPA_IS_ALL_REPLICA_ATTR_SET(gc_node->ckpt_creat_attri.creationFlags) == true))) {
		is_local_get = true;
	}

	if (is_local_get == false) {
		if (gc_node->is_restart) {
			rc = SA_AIS_ERR_TRY_AGAIN;
			m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
			TRACE_1("Cpa StatusGet failed with return value:%d,ckptHandle:%llx", rc, checkpointHandle);
			goto fail1;
		}

		if (!gc_node->is_active_exists) {
			rc = SA_AIS_ERR_NOT_EXIST;
			m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
			TRACE_4("Cpa StatusGet failed with return value:%d,ckptHandle:%llx", rc, checkpointHandle);
			goto fail1;
		}
	} else {
		if (false == cb->is_cpnd_up) {
			rc = SA_AIS_ERR_TRY_AGAIN;
			m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
			TRACE_1("Cpa StatusGet failed with return value:%d,ckptHandle:%llx", rc, checkpointHandle);
			goto fail1;
		}
	}

	/* Populate the event & send it to CPND */
	memset(&evt, 0, sizeof(CPSV_EVT));
	evt.type = CPSV_EVT_TYPE_CPND;
	evt.info.cpnd.type = CPND_EVT_A2ND_CKPT_STATUS_GET;
	evt.info.cpnd.info.statReq.ckpt_id = lc_node->gbl_ckpt_hdl;

	m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);

	if (false == is_local_get) {
		proc_rc = cpa_mds_msg_sync_send(cb->cpa_mds_hdl, &(gc_node->active_mds_dest),
						&evt, &out_evt, CPSV_WAIT_TIME);
	} else {
		proc_rc = cpa_mds_msg_sync_send(cb->cpa_mds_hdl, &(cb->cpnd_mds_dest), &evt, &out_evt, CPSV_WAIT_TIME);

	}

	/* Generate rc from proc_rc */
	switch (proc_rc) {
	case NCSCC_RC_SUCCESS:
		break;
	case NCSCC_RC_REQ_TIMOUT:
		TRACE_4("cpa StatusGet:MDS api failed with return value:%d,ckptHandle:%llx,cpnd_mds_dest:%"PRIu64,
                        proc_rc , checkpointHandle, cb->cpnd_mds_dest);
		rc = SA_AIS_ERR_TIMEOUT;
		goto fail1;
	default:
		rc = SA_AIS_ERR_TRY_AGAIN;
		TRACE_4("cpa StatusGet:MDS api failed with return value:%d,ckptHandle:%llx,cpnd_mds_dest:%"PRIu64,
                        proc_rc , checkpointHandle, cb->cpnd_mds_dest);
		goto fail1;
	}

	if (out_evt) {
		rc = out_evt->info.cpa.info.status.error;
		if (rc == SA_AIS_OK) {
			*checkpointStatus = out_evt->info.cpa.info.status.status;
			TRACE_1("Cpa StatusGet Api Success with return value:%d,ckptHandle:%llx",proc_rc, checkpointHandle);
		}

		m_MMGR_FREE_CPSV_EVT(out_evt, NCS_SERVICE_ID_CPA);
	} else {
		rc = SA_AIS_ERR_NO_RESOURCES;
		TRACE_4("Cpa StatusGet failed with return value:%d,ckptHandle:%llx", rc, checkpointHandle);
	}

 fail1:
 lock_fail:
 clm_left:
	m_CPA_GIVEUP_CB;

 done:
	TRACE_LEAVE2("API return code = %u", rc);
	return rc;
}

/****************************************************************************
  Name          :  saCkptSectionCreate
 
  Description   :  This function creates a new section. The section will be 
                   deleted by the checkpoint service when its expiration time
                   is reached.
 
  Arguments     :  checkpointHandle - Handles that designates the checkpoint
                   sectionCreationAttributes - Pointer to a
                                 SaCkptSectionCreationAttributesT structure.
                   initialData - A location in the address space of the 
                             invoking process that contains the initial data
                             of the section to be created.
                   initialDataSize - The size of the initial data of the 
                                     section to be created.
                   
  Return Values :  Refer to SAI-AIS specification for various return values.
 
  Notes         :
******************************************************************************/
SaAisErrorT saCkptSectionCreate(SaCkptCheckpointHandleT checkpointHandle,
				SaCkptSectionCreationAttributesT *sectionCreationAttributes,
				const void *initialData, SaSizeT initialDataSize)
{
	SaAisErrorT rc = SA_AIS_OK;
	CPSV_EVT evt;
	CPSV_EVT *out_evt = NULL;
	uint32_t proc_rc = NCSCC_RC_SUCCESS;
	CPA_LOCAL_CKPT_NODE *lc_node = NULL;
	CPA_GLOBAL_CKPT_NODE *gc_node = NULL;
	bool add_flag = false;
	CPA_CB *cb = NULL;
	bool gen_sec_flag = false;
	SaCkptSectionIdT app_ptr;
	CPA_CLIENT_NODE *cl_node = NULL;

	TRACE_ENTER2("SaCkptCheckpointHandleT passed is %llx",checkpointHandle);
	/* Validate the Input Parameters */
	if ((sectionCreationAttributes == NULL) || (sectionCreationAttributes->sectionId == NULL)) {
		rc = SA_AIS_ERR_INVALID_PARAM;
		TRACE_4("cpa SectCreate with return value:%d,ckptHandle:%llx", rc, checkpointHandle);
		TRACE_LEAVE2("API return code = %u", rc);
		return rc;
	}

	if ((initialData == NULL) && (initialDataSize > 0)) {
		rc = SA_AIS_ERR_INVALID_PARAM;
		TRACE_4("cpa SectCreate Api failed with return value:%d,ckptHandle:%llx", rc, checkpointHandle);
		goto done;
	}

	/* retrieve CPA CB */
	m_CPA_RETRIEVE_CB(cb);
	if (!cb) {
		rc = SA_AIS_ERR_BAD_HANDLE;
		TRACE_4("Cpa SectCreate:HDL_TAKE api failed with return value:%d,ckptHandle:%llx", rc, checkpointHandle);
		goto done;
	}

	/* get the CB Lock */
	if (m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
		rc = SA_AIS_ERR_LIBRARY;
		TRACE_4("Cpa SectCreate:LOCK failed with return value:%d,ckptHandle:%llx", rc, checkpointHandle);
		goto lock_fail;
	}


	if (initialDataSize > CPSV_MAX_DATA_SIZE) {
		rc = SA_AIS_ERR_NO_RESOURCES;
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
		goto fail1;
	}

	proc_rc = cpa_lcl_ckpt_node_get(&cb->lcl_ckpt_tree, &checkpointHandle, &lc_node);
	if (!lc_node) {
		rc = SA_AIS_ERR_BAD_HANDLE;
		TRACE_4("cpa SectCreate Api failed with return value:%d,ckptHandle:%llx", proc_rc, checkpointHandle);
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
		goto fail1;
	}

	proc_rc = cpa_client_node_get(&cb->client_tree, &lc_node->cl_hdl, &cl_node);

	if (!cl_node) {
		TRACE_4("Cpa SelObjGet:client_node_get Api failed with return value:%d,cl_hdl:%llx",SA_AIS_ERR_BAD_HANDLE, lc_node->cl_hdl);
		rc = SA_AIS_ERR_BAD_HANDLE;
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
		goto fail1;
	}

	/* Checking Node availability */
	if (m_CPA_VER_IS_ABOVE_B_1_1(&cl_node->version)) {
		if ((cb->is_cpnd_joined_clm != true)  || ( cl_node->stale == true)) {
			TRACE_4("Cpa CLM Node left with return value:%d",SA_AIS_ERR_UNAVAILABLE);
			rc = SA_AIS_ERR_UNAVAILABLE;
			m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
			goto clm_left;
		}
	}

	if (!(lc_node->open_flags & SA_CKPT_CHECKPOINT_WRITE)) {
		rc = SA_AIS_ERR_ACCESS;
		TRACE_4("cpa SectCreate Api failed with return value:%d,ckptHandle:%llx", rc, checkpointHandle);
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
		goto fail1;
	}

	/* Get the Global Ckpt node */
	proc_rc = cpa_gbl_ckpt_node_find_add(&cb->gbl_ckpt_tree, &lc_node->gbl_ckpt_hdl, &gc_node, &add_flag);
	if (!gc_node) {
		rc = SA_AIS_ERR_NO_MEMORY;
		TRACE_4("cpa SectCreate Api failed with return value:%d,ckptHandle:%llx", proc_rc, checkpointHandle);
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
		goto fail1;
	}
	if (sectionCreationAttributes->sectionId->id != NULL) {
		if ((sectionCreationAttributes->sectionId->idLen > gc_node->ckpt_creat_attri.maxSectionIdSize) ||
		    (sectionCreationAttributes->sectionId->idLen == 0)) {
			rc = SA_AIS_ERR_INVALID_PARAM;
			TRACE_4("cpa SectCreate Api failed with return value:%d,ckptHandle:%llx", rc, checkpointHandle);
			m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
			goto fail1;
		}
	} else {
		if (sectionCreationAttributes->sectionId->idLen != 0) {
			rc = SA_AIS_ERR_INVALID_PARAM;
			TRACE_4("cpa SectCreate Api failed with return value:%d,ckptHandle:%llx", rc, checkpointHandle);
			m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
			goto fail1;
		}
	}
	if (gc_node->is_restart) {
		rc = SA_AIS_ERR_TRY_AGAIN;
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
		TRACE_4("cpa SectCreate Api failed with return value:%d,ckptHandle:%llx", rc, checkpointHandle);
		goto fail1;
	}

	if (!gc_node->is_active_exists) {
		rc = SA_AIS_ERR_NOT_EXIST;
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
		TRACE_4("cpa SectCreate Api failed with return value:%d,ckptHandle:%llx", rc, checkpointHandle);
		goto fail1;
	}

	if ((sectionCreationAttributes->sectionId->idLen == 0 &&
	     sectionCreationAttributes->sectionId->id == NULL) && (gc_node->ckpt_creat_attri.maxSections > 1)) {
		gen_sec_flag = true;
	}

	memset(&app_ptr, 0, sizeof(SaCkptSectionIdT));
	/* Populate the event & send it to CPND */
	memset(&evt, 0, sizeof(CPSV_EVT));
	evt.type = CPSV_EVT_TYPE_CPND;
	evt.info.cpnd.type = CPND_EVT_A2ND_CKPT_SECT_CREATE;
	evt.info.cpnd.info.sec_creatReq.ckpt_id = lc_node->gbl_ckpt_hdl;
	evt.info.cpnd.info.sec_creatReq.lcl_ckpt_id = lc_node->lcl_ckpt_hdl;
	evt.info.cpnd.info.sec_creatReq.agent_mdest = cb->cpa_mds_dest;
	evt.info.cpnd.info.sec_creatReq.sec_attri = *sectionCreationAttributes;
	evt.info.cpnd.info.sec_creatReq.sec_attri.expirationTime = sectionCreationAttributes->expirationTime;
	evt.info.cpnd.info.sec_creatReq.init_data = (void *)initialData;
	evt.info.cpnd.info.sec_creatReq.init_size = initialDataSize;

	m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
	proc_rc = cpa_mds_msg_sync_send(cb->cpa_mds_hdl, &(gc_node->active_mds_dest),
					&evt, &out_evt, CPA_WAIT_TIME(initialDataSize));

	/* Generate rc from proc_rc */
	switch (proc_rc) {
	case NCSCC_RC_SUCCESS:
		break;
	case NCSCC_RC_REQ_TIMOUT:
		rc = SA_AIS_ERR_TIMEOUT;
		TRACE_4("cpa SectCreate:MDS api failed with return value:%d,ckptHandle:%llx,cpnd_mds_dest:%"PRIu64,
			 proc_rc, checkpointHandle, cb->cpnd_mds_dest);
		goto fail1;
	default:
		TRACE_4("cpa SectCreate:MDS api failed with return value:%d,ckptHandle:%llx,cpnd_mds_dest:%"PRIu64,
			 proc_rc, checkpointHandle, cb->cpnd_mds_dest);
		rc = SA_AIS_ERR_TRY_AGAIN;
		goto fail1;
	}

	if (out_evt) {
		/* Processing of Out Event */
		rc = out_evt->info.cpa.info.sec_creat_rsp.error;
		if (rc == SA_AIS_OK) {
			if (gen_sec_flag) {
				if (out_evt->info.cpa.info.sec_creat_rsp.sec_id.idLen) {
					if (m_CPA_VER_IS_ABOVE_B_1_1(&cl_node->version)) {
						app_ptr.id =
						    (SaUint8T *)m_MMGR_ALLOC_CPA_DEFAULT(out_evt->info.cpa.
											 info.sec_creat_rsp.sec_id.
											 idLen * sizeof(SaUint8T));
					} else {
						app_ptr.id =
						    (SaUint8T *)malloc(out_evt->info.cpa.info.sec_creat_rsp.
								       sec_id.idLen * sizeof(SaUint8T));
					}

					if (app_ptr.id == NULL) {
						TRACE_4("cpd daa buff memory allocation failed");
						if (out_evt->info.cpa.info.sec_creat_rsp.sec_id.id != NULL &&
						    out_evt->info.cpa.info.sec_creat_rsp.sec_id.idLen != 0)
							m_MMGR_FREE_CPSV_DEFAULT_VAL(out_evt->info.cpa.
										     info.sec_creat_rsp.sec_id.id,
										     NCS_SERVICE_ID_CPND);
						m_MMGR_FREE_CPSV_EVT(out_evt, NCS_SERVICE_ID_CPA);
						rc = SA_AIS_ERR_NO_MEMORY;
						TRACE_1("cpa api ckpt sect create success with return value:%d,ckptHandle:%llx",
								rc, checkpointHandle);
						goto fail1;
					}
					memset(app_ptr.id, 0,
					       out_evt->info.cpa.info.sec_creat_rsp.sec_id.idLen * sizeof(SaUint8T));
					app_ptr.idLen = out_evt->info.cpa.info.sec_creat_rsp.sec_id.idLen;
					memcpy(app_ptr.id, out_evt->info.cpa.info.sec_creat_rsp.sec_id.id,
					       app_ptr.idLen);

					*sectionCreationAttributes->sectionId = app_ptr;
				}
			}
			if (out_evt->info.cpa.info.sec_creat_rsp.sec_id.id != NULL
			    && out_evt->info.cpa.info.sec_creat_rsp.sec_id.idLen != 0)
				m_MMGR_FREE_CPSV_DEFAULT_VAL(out_evt->info.cpa.info.sec_creat_rsp.sec_id.id,
							     NCS_SERVICE_ID_CPND);

		} else {
			TRACE_4("cpa for sectCreate Api failed with return value:%d,ckptHandle:%llx,mds_dest:%"PRIu64,
					 rc, checkpointHandle,gc_node->active_mds_dest);
		}
		m_MMGR_FREE_CPSV_EVT(out_evt, NCS_SERVICE_ID_CPA);
	} else {
		rc = SA_AIS_ERR_NO_RESOURCES;
		TRACE_4("cpa SectCreate Api failed with return value:%d,ckptHandle:%llx", rc, checkpointHandle);
	}
 fail1:
 lock_fail:
 clm_left:
	m_CPA_GIVEUP_CB;

 done:
	if (rc == SA_AIS_OK)
		TRACE_1("cpa sectCreate api success with return value:%d,ckptHandle:%llx", rc, checkpointHandle);

	TRACE_LEAVE2("API return code = %u", rc);
	return rc;
}

/****************************************************************************
  Name          :  saCkptSectionIdFree
 
  Description   :  This function frees the memory to which id points. This memory was allocated by the
			   Checkpoint Service library in a previous call to the saCkptSectionCreate() function.
 
  Arguments     :  checkpointHandle - [in] The handle to the checkpoint. The handle
			          checkpointHandle must have been obtained by a previous invocation of
                   	          saCkptCheckpointOpen() or saCkptCheckpointOpenAsync(). 
                   	    
                        id - [in] A pointer to the section identifier that was allocated by the Checkpoint Service
				  library in the saCkptSectionCreate() function and is to be freed. 
				  
  Return Values :  Refer to SAI-AIS specification for various return values.
 

  Notes         :
******************************************************************************/

SaAisErrorT saCkptSectionIdFree(SaCkptCheckpointHandleT checkpointHandle, SaUint8T *id)
{
	SaAisErrorT rc = SA_AIS_OK;
	CPA_CB *cb = NULL;
	CPA_CLIENT_NODE *cl_node = NULL;
	CPA_LOCAL_CKPT_NODE *lc_node = NULL;

	TRACE_ENTER2("SaCkptCheckpointHandleT passed is %llx",checkpointHandle);
	/* retrieve CPA CB */
	m_CPA_RETRIEVE_CB(cb);

	if (!cb) {
		rc = SA_AIS_ERR_BAD_HANDLE;
		TRACE_4("cpa saCkptSectionIdFree:HDL_TAKE Api failed with return value:%d,ckptHandle:%llx", rc, checkpointHandle);
		TRACE_LEAVE2("API return code = %u", rc);
		return rc;
	}

	/* Take the CB Lock */
	if (m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
		TRACE_4("cpa CkptInit:LOCK"); 
		rc = SA_AIS_ERR_LIBRARY;
		goto lock_fail;
	}

	(void)cpa_lcl_ckpt_node_get(&cb->lcl_ckpt_tree, &checkpointHandle, &lc_node);
	if (!lc_node) {
		rc = SA_AIS_ERR_BAD_HANDLE;
		TRACE_4("cpa StatusGet Api failed with return value:%d,ckptHandle:%llx", rc, checkpointHandle);
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
		goto fail;
	}

	(void)cpa_client_node_get(&cb->client_tree, &lc_node->cl_hdl, &cl_node);

	if (!cl_node) {
		TRACE_4("cpa SelObjGet:client_node_get Api failed with return value:%d,cl_hdl:%llx ",SA_AIS_ERR_BAD_HANDLE, lc_node->cl_hdl);
		rc = SA_AIS_ERR_BAD_HANDLE;
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
		goto fail;
	}

	/* Checking Node availability */
	if (m_CPA_VER_IS_ABOVE_B_1_1(&cl_node->version)) {
		if ((cb->is_cpnd_joined_clm != true)  || ( cl_node->stale == true)) {
			TRACE_4("cpa CLM Node left with return value:%d",SA_AIS_ERR_UNAVAILABLE);
			rc = SA_AIS_ERR_UNAVAILABLE;
			m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
			goto clm_left;
		}
	}

	/* id is  Null , so return INVALID_PARAM */
	if (id == NULL) {
		rc = SA_AIS_ERR_INVALID_PARAM;
		TRACE_4("cpa saCkptSectionIdFree Api failed with return value:%d,ckptHandle:%llx", rc, checkpointHandle);
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
		goto fail;

	}

	m_MMGR_FREE_CPA_DEFAULT(id);
	/* Unlock CB  */
	m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);

 lock_fail:
 clm_left:
 fail:
	m_CPA_GIVEUP_CB;

	TRACE_LEAVE2("API return code = %u", rc);
	return rc;
}

/****************************************************************************
  Name          :  saCkptSectionDelete
 
  Description   :  This function deletes a section.
 
  Arguments     :  checkpointHandle - Handle that designates the checkpoint
                   sectionId - A pointer to the identifier of the section that
                               is to be deleted.
 
  Return Values :  Refer to SAI-AIS specification for various return values.
 
  Notes         :
******************************************************************************/
SaAisErrorT saCkptSectionDelete(SaCkptCheckpointHandleT checkpointHandle, const SaCkptSectionIdT *sectionId)
{
	SaAisErrorT rc = SA_AIS_OK;
	CPSV_EVT evt;
	CPSV_EVT *out_evt = NULL;
	uint32_t proc_rc = NCSCC_RC_SUCCESS;
	CPA_LOCAL_CKPT_NODE *lc_node = NULL;
	CPA_CB *cb = NULL;
	CPA_GLOBAL_CKPT_NODE *gc_node = NULL;
	bool add_flag = false;
	CPA_CLIENT_NODE *cl_node = NULL;

	TRACE_ENTER2("SaCkptCheckpointHandleT passed is %llx",checkpointHandle);
	/* Validate the Input Parameters */
	if ((sectionId == NULL) || ((sectionId->id == NULL) && (sectionId->idLen == 0))) {
		TRACE_4("cpa SectDelete Api failed with return value:%d,ckptHandle:%llx", SA_AIS_ERR_INVALID_PARAM, checkpointHandle);
		return SA_AIS_ERR_INVALID_PARAM;
	}

	/* For Section Delete CPA will not perform any operation 
	   other than passing the request to CPND */

	/* retrieve CPA CB */
	m_CPA_RETRIEVE_CB(cb);
	if (!cb) {
		rc = SA_AIS_ERR_BAD_HANDLE;
		TRACE_4("cpa SectDelete:HDL_TAKE Api failed with return value:%d,ckptHandle:%llx ", rc, checkpointHandle);
		goto done;
	}

	/* get the CB Lock */
	if (m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
		rc = SA_AIS_ERR_LIBRARY;
		TRACE_4("cpa SectDelete:LOCK Api failed with return value:%d,ckptHandle:%llx ", rc, checkpointHandle);

		goto lock_fail;
	}

	proc_rc = cpa_lcl_ckpt_node_get(&cb->lcl_ckpt_tree, &checkpointHandle, &lc_node);
	if (!lc_node) {
		rc = SA_AIS_ERR_BAD_HANDLE;
		TRACE_4("cpa SectDelete Api failed with return value:%d,ckptHandle:%llx", rc, checkpointHandle);
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
		goto fail1;
	}

	proc_rc = cpa_client_node_get(&cb->client_tree, &lc_node->cl_hdl, &cl_node);

	if (!cl_node) {
		TRACE_4("cpa SelObjGet:client_node_get Api failed with return value:%d,cl_hdl:%llx ", SA_AIS_ERR_BAD_HANDLE, lc_node->cl_hdl);
		rc = SA_AIS_ERR_BAD_HANDLE;
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
		goto fail1;
	}

	/* Checking Node availability */
	if (m_CPA_VER_IS_ABOVE_B_1_1(&cl_node->version)) {
		if ((cb->is_cpnd_joined_clm != true)  || ( cl_node->stale == true)) {
			TRACE_4("cpa CLM Node left with return value:%d",SA_AIS_ERR_UNAVAILABLE);
			rc = SA_AIS_ERR_UNAVAILABLE;
			m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
			goto clm_left;
		}
	}

	/* Don't allow the delete, in case if SA_CKPT_CHECKPOINT_WRITE is 
	   not set */
	if (!(lc_node->open_flags & SA_CKPT_CHECKPOINT_WRITE)) {
		rc = SA_AIS_ERR_ACCESS;
		TRACE_4("cpa SectDelete Api failed with return value:%d,ckptHandle:%llx", rc, checkpointHandle);
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
		goto fail1;
	}

	/* Get the Global Ckpt node */
	proc_rc = cpa_gbl_ckpt_node_find_add(&cb->gbl_ckpt_tree, &lc_node->gbl_ckpt_hdl, &gc_node, &add_flag);
	if (!gc_node) {
		rc = SA_AIS_ERR_NO_MEMORY;
		TRACE_4("cpa SectDelete Api failed with return value:%d,ckptHandle:%llx", proc_rc, checkpointHandle);
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
		goto fail1;
	}

	if (gc_node->is_restart) {
		rc = SA_AIS_ERR_TRY_AGAIN;
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
		TRACE_4("cpa SectDelete Api failed with return value:%d,ckptHandle:%llx", rc, checkpointHandle);
		goto fail1;
	}

	if (!gc_node->is_active_exists) {
		rc = SA_AIS_ERR_NOT_EXIST;
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
		TRACE_4("cpa SectDelete Api failed with return value:%d,ckptHandle:%llx", rc, checkpointHandle);
		goto fail1;
	}

	/* Populate the event & send it to CPND */
	memset(&evt, 0, sizeof(CPSV_EVT));
	evt.type = CPSV_EVT_TYPE_CPND;
	evt.info.cpnd.type = CPND_EVT_A2ND_CKPT_SECT_DELETE;
	evt.info.cpnd.info.sec_delReq.ckpt_id = lc_node->gbl_ckpt_hdl;
	evt.info.cpnd.info.sec_delReq.lcl_ckpt_id = lc_node->lcl_ckpt_hdl;
	evt.info.cpnd.info.sec_delReq.agent_mdest = cb->cpa_mds_dest;
	evt.info.cpnd.info.sec_delReq.sec_id = *sectionId;

	m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
	proc_rc = cpa_mds_msg_sync_send(cb->cpa_mds_hdl, &(gc_node->active_mds_dest), &evt, &out_evt, CPSV_WAIT_TIME);

	/* Generate rc from proc_rc */
	switch (proc_rc) {
	case NCSCC_RC_SUCCESS:
		break;
	case NCSCC_RC_REQ_TIMOUT:
		rc = SA_AIS_ERR_TIMEOUT;
		TRACE_4("cpa SectDelete:MDS api failed  with return value:%d,ckptHandle:%llx,cpnd_mds_dest:%"PRIu64,
                        proc_rc , checkpointHandle, cb->cpnd_mds_dest);
		goto fail1;
	default:
		rc = SA_AIS_ERR_TRY_AGAIN;
		TRACE_4("cpa SectDelete:MDS api failed with return value:%d,ckptHandle:%llx,cpnd_mds_dest:%"PRIu64,
                        proc_rc , checkpointHandle, cb->cpnd_mds_dest);
		goto fail1;
	}

	if (out_evt) {
		/* Processing of Out Event */
		rc = out_evt->info.cpa.info.sec_delete_rsp.error;
		m_MMGR_FREE_CPSV_EVT(out_evt, NCS_SERVICE_ID_CPA);
	} else {
		rc = SA_AIS_ERR_NO_RESOURCES;
		TRACE_4("cpa SectDelete Api failed with return value:%d,ckptHandle:%llx", rc, checkpointHandle);
	}
 fail1:
 lock_fail:
 clm_left:
	m_CPA_GIVEUP_CB;

 done:
	if (rc == SA_AIS_OK)
		TRACE_1("Cpa SectDelete Api Success with return value:%d,ckptHandle:%llx", rc, checkpointHandle);
	TRACE_LEAVE2("API return code = %u", rc);
	return rc;
}

/****************************************************************************
  Name          :  saCkptSectionExpirationTimeSet
 
  Description   :  This function sets the expiration time of the section.
 
  Arguments     :  checkpointHandle - Handle that designates the checkpoint
                   sectionId - A pointer to the identifier of the section for 
                         which the expiration time is to be set.
                   expirationTime - The expirationTime that is to be set for
                                    the section.
 
  Return Values :  Refer to SAI-AIS specification for various return values.
 
  Notes         :
******************************************************************************/
SaAisErrorT saCkptSectionExpirationTimeSet(SaCkptCheckpointHandleT checkpointHandle,
					   const SaCkptSectionIdT *sectionId, SaTimeT expirationTime)
{
	SaAisErrorT rc = SA_AIS_OK;
	CPSV_EVT evt;
	CPSV_EVT *out_evt = NULL;
	uint32_t proc_rc = NCSCC_RC_SUCCESS;
	CPA_LOCAL_CKPT_NODE *lc_node = NULL;
	CPA_CB *cb = NULL;
	SaTimeT now, duration;
	int64_t time_stamp, giga_sec, result;
	CPA_CLIENT_NODE *cl_node = NULL;

	TRACE_ENTER2("SaCkptCheckpointHandleT passed is %llx",checkpointHandle); /* Validate the Input Parameters */
	if (sectionId == NULL || (sectionId->id == NULL && sectionId->idLen == 0)) {
		rc = SA_AIS_ERR_INVALID_PARAM;
		TRACE_4("cpa SectExpTimeSet Api failed with return value:%d,ckptHandle:%llx", rc, checkpointHandle);
		TRACE_LEAVE2("API return code = %u", rc);
		return rc;
	}

	/* retrieve CPA CB */
	m_CPA_RETRIEVE_CB(cb);
	if (!cb) {
		rc = SA_AIS_ERR_BAD_HANDLE;
		TRACE_4("cpa SectExpTimeSet:HDL_TAKE Api failed with return value:%d,ckptHandle:%llx", rc, checkpointHandle);
		goto done;
	}

	/* get the CB Lock */
	if (m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
		rc = SA_AIS_ERR_LIBRARY;
		TRACE_4("cpa SectExpTimeSet:LOCK Api failed with return value:%d,ckptHandle:%llx", rc, checkpointHandle);
		goto lock_fail;
	}

	proc_rc = cpa_lcl_ckpt_node_get(&cb->lcl_ckpt_tree, &checkpointHandle, &lc_node);
	if (!lc_node) {
		rc = SA_AIS_ERR_BAD_HANDLE;
		TRACE_4("cpa SectExpTimeSet:lcl_ckpt_node_get Api failed with return value:%d,ckptHandle:%llx", rc, checkpointHandle);
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
		goto fail1;
	}

	proc_rc = cpa_client_node_get(&cb->client_tree, &lc_node->cl_hdl, &cl_node);

	if (!cl_node) {
		TRACE_4("cpa SelObjGet:client_node_get Api failed with return value:%d,ckptHandle:%llx",
				 SA_AIS_ERR_BAD_HANDLE, lc_node->cl_hdl);
		rc = SA_AIS_ERR_BAD_HANDLE;
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
		goto fail1;
	}

	/* Checking Node availability */
	if (m_CPA_VER_IS_ABOVE_B_1_1(&cl_node->version)) {
		if ((cb->is_cpnd_joined_clm != true)  || ( cl_node->stale == true)) {
			TRACE_4("cpa CLM Node left with return value:%d", SA_AIS_ERR_UNAVAILABLE);
			rc = SA_AIS_ERR_UNAVAILABLE;
			m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
			goto clm_left;
		}
	}

	/* Don't allow the exp time set, in case if SA_CKPT_CHECKPOINT_WRITE is 
	   not set */
	if (!(lc_node->open_flags & SA_CKPT_CHECKPOINT_WRITE)) {
		rc = SA_AIS_ERR_ACCESS;
		TRACE_4("cpa SectExpTimeSet Api failed with return value:%d,ckptHandle:%llx", rc, checkpointHandle);
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
		goto fail1;
	}

	/* Populate the event & send it to CPND */
	memset(&evt, 0, sizeof(CPSV_EVT));
	evt.type = CPSV_EVT_TYPE_CPND;
	evt.info.cpnd.type = CPND_EVT_A2ND_CKPT_SECT_EXP_SET;
	evt.info.cpnd.info.sec_expset.ckpt_id = lc_node->gbl_ckpt_hdl;
	evt.info.cpnd.info.sec_expset.sec_id = *sectionId;
	/* Convert the Time to millisecs */

	time_stamp = m_GET_TIME_STAMP(now);
	giga_sec = 1000000000;
	result = time_stamp * giga_sec;
	duration = (expirationTime - result);
	if (duration < 0) {
		rc = SA_AIS_ERR_INVALID_PARAM;
		TRACE_4("cpa SectExpTimeSet Api failed with return value:%d,ckptHandle:%llx,expirationTime:%llu ", 
			rc, checkpointHandle, expirationTime);
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
		goto fail1;
	}

	evt.info.cpnd.info.sec_expset.exp_time = duration;
	/* evt.info.cpnd.info.sec_expset.exp_time = expirationTime; */
	m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);

	/* IF CPND IS DOWN  */
	if (false == cb->is_cpnd_up) {
		rc = SA_AIS_ERR_TRY_AGAIN;
		TRACE_1("Cpa SectExpTimeSet:CPND_DOWN Api failed with return value:%d,ckptHandle:%llx", rc, checkpointHandle);
		goto fail1;
	}
	proc_rc = cpa_mds_msg_sync_send(cb->cpa_mds_hdl, &(cb->cpnd_mds_dest), &evt, &out_evt, CPSV_WAIT_TIME);

	/* Generate rc from proc_rc */
	switch (proc_rc) {
	case NCSCC_RC_SUCCESS:
		break;
	case NCSCC_RC_REQ_TIMOUT:
		rc = SA_AIS_ERR_TIMEOUT;
		TRACE_4("Cpa SectExpTimeSet:MDS api failed with return value:%d,ckptHandle:%llx,cpnd_mds_dest:%"PRIu64,
                        proc_rc , checkpointHandle, cb->cpnd_mds_dest);
		goto fail1;
	default:
		rc = SA_AIS_ERR_TRY_AGAIN;
		TRACE_4("cpa SectExpTimeSet:MDS api failed with return value:%d,ckptHandle:%llx,cpnd_mds_dest:%"PRIu64,
                        proc_rc , checkpointHandle, cb->cpnd_mds_dest);
		goto fail1;
	}

	if (out_evt) {
		/* Processing of Out Event */
		rc = out_evt->info.cpa.info.sec_exptmr_rsp.error;
		m_MMGR_FREE_CPSV_EVT(out_evt, NCS_SERVICE_ID_CPA);
		if (rc != SA_AIS_OK)
			TRACE_4("Cpa SectExpTimeSet failed with return value:%d,ckptHandle:%llx", rc, checkpointHandle);
		else
			TRACE_1("Cpa SectExpTimeSet Api Success with return value:%d,ckptHandle:%llx", rc, checkpointHandle);
	} else {
		rc = SA_AIS_ERR_NO_RESOURCES;
		TRACE_4("Cpa SectExpTimeSet failed with return value:%d,ckptHandle:%llx", rc, checkpointHandle);
	}
 fail1:
 lock_fail:
 clm_left:
	m_CPA_GIVEUP_CB;

 done:
	TRACE_LEAVE2("API return code = %u", rc);
	return rc;
}

/****************************************************************************
  Name          :  saCkptSectionIteratorInitialize
 
  Description   :  This API initializes the section iteration.
 
  Arguments     :  checkpointHandle - Handle that designates the checkpoint
                   sectionChosen - A pridicate, defined by the 
                                   SaCkptSectionsChosenT structure.
                   expirationTime - An absolute time used by sectionChosen.
                   sectionIterator - An iterator for stepping through the 
                                     sections in the checkpoint.
 
  Return Values :  Refer to SAI-AIS specification for various return values.
 
  Notes         :
******************************************************************************/
SaAisErrorT saCkptSectionIterationInitialize(SaCkptCheckpointHandleT checkpointHandle,
					     SaCkptSectionsChosenT sectionsChosen,
					     SaTimeT expirationTime,
					     SaCkptSectionIterationHandleT *sectionIterationHandle)
{
	SaAisErrorT rc = SA_AIS_OK;
	uint32_t proc_rc = NCSCC_RC_SUCCESS;
	CPA_LOCAL_CKPT_NODE *lc_node = NULL;
	CPA_SECT_ITER_NODE *sect_iter_node = NULL;
	CPA_CB *cb = NULL;
	CPA_GLOBAL_CKPT_NODE *gc_node = NULL;
	bool add_flag = false;
	bool is_local_get_next = false;
	CPA_CLIENT_NODE *cl_node = NULL;
	
	TRACE_ENTER2("SaCkptCheckpointHandleT passed is %llx",checkpointHandle);

	/* Validate the input parameters */
	if ((!sectionIterationHandle) ||
	    (sectionsChosen < SA_CKPT_SECTIONS_FOREVER) || (sectionsChosen > SA_CKPT_SECTIONS_ANY)) {
		rc = SA_AIS_ERR_INVALID_PARAM;
		TRACE_4("cpa SectIterInit Api failed with return value:%d,ckptHandle:%llx", rc, checkpointHandle);
		return rc;
	}

	/* retrieve CPA CB */
	m_CPA_RETRIEVE_CB(cb);
	if (!cb) {
		TRACE_4("cpa SectIterInit Api failed with return value:%d,ckptHandle:%llx", SA_AIS_ERR_BAD_HANDLE, checkpointHandle);
		return SA_AIS_ERR_BAD_HANDLE;
	}

	/* get the CB Lock */
	if (m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
		rc = SA_AIS_ERR_LIBRARY;
		TRACE_4("cpa SectIterInit Api failed with return value:%d,ckptHandle:%llx", rc, checkpointHandle);
		goto lock_fail;
	}

	proc_rc = cpa_lcl_ckpt_node_get(&cb->lcl_ckpt_tree, &checkpointHandle, &lc_node);
	if (!lc_node) {
		rc = SA_AIS_ERR_BAD_HANDLE;
		TRACE_4("cpa SectIterInit Api failed with return value:%d,ckptHandle:%llx", rc, checkpointHandle);
		goto ckpt_node_get_fail;
	}
	proc_rc = cpa_client_node_get(&cb->client_tree, &lc_node->cl_hdl, &cl_node);

	if (!cl_node) {
		TRACE_4("cpa SelObjGet:client_node_get Api failed with return value:%d,cl_hdl:%llx ",
				SA_AIS_ERR_BAD_HANDLE, lc_node->cl_hdl);
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto ckpt_node_get_fail;
	}

	/* Checking Node availability */
	if (m_CPA_VER_IS_ABOVE_B_1_1(&cl_node->version)) {
		if ((cb->is_cpnd_joined_clm != true)  || ( cl_node->stale == true)) {
			TRACE_4("cpa CLM Node left");
			rc = SA_AIS_ERR_UNAVAILABLE;
			m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
			goto clm_left;
		}

		if (!(lc_node->open_flags & SA_CKPT_CHECKPOINT_READ)) {
			rc = SA_AIS_ERR_ACCESS;
			TRACE_4("cpa SectIterInit Api failed with return value:%d,ckptHandle:%llx", rc, checkpointHandle);
			goto ckpt_node_get_fail;
		}
	}
	proc_rc = cpa_gbl_ckpt_node_find_add(&cb->gbl_ckpt_tree, &lc_node->gbl_ckpt_hdl, &gc_node, &add_flag);

	if (!gc_node) {
		TRACE_4("cpd gbl ckpt find add failed");
		rc = SA_AIS_ERR_NO_RESOURCES;
		TRACE_4("cpa SectIterInit Api failed with return value:%d,ckptHandle:%llx", rc, checkpointHandle);
		goto done;
	}

	if (((m_CPA_IS_COLLOCATED_ATTR_SET(gc_node->ckpt_creat_attri.creationFlags) == true)
	     && (m_CPA_IS_ALL_REPLICA_ATTR_SET(gc_node->ckpt_creat_attri.creationFlags) == true))) {
		is_local_get_next = true;
	}

	if (is_local_get_next == false) {

		if (gc_node->is_restart) {
			rc = SA_AIS_ERR_TRY_AGAIN;
			TRACE_1("cpa SectIterInit Api failed with return value:%d,ckptHandle:%llx", rc, checkpointHandle);
			goto done;
		}

		if (!gc_node->is_active_exists) {
			if (m_CPA_VER_IS_ABOVE_B_1_1(&cl_node->version)) {
				if (m_CPA_IS_ALL_REPLICA_ATTR_SET(gc_node->ckpt_creat_attri.creationFlags) == false) {
					rc = SA_AIS_ERR_NOT_EXIST;
					TRACE_4("cpa SectIterInit Api failed with return value:%d,ckptHandle:%llx", rc, checkpointHandle);
					goto done;
				}
			} else {
				rc = SA_AIS_ERR_NOT_EXIST;
				TRACE_4("cpa SectIterInit Api failed with return value:%d,ckptHandle:%llx", rc, checkpointHandle);
				goto done;
			}

		}
	} else {
		if (false == cb->is_cpnd_up) {
			rc = SA_AIS_ERR_TRY_AGAIN;
			TRACE_4("cpa SectIterInit Api failed with return value:%d,ckptHandle:%llx", rc, checkpointHandle);
			goto done;
		}
	}

	lc_node->sect_iter_cnt++;

	/* Allocate the section Iretation Node */
	sect_iter_node = (CPA_SECT_ITER_NODE *)m_MMGR_ALLOC_CPA_SECT_ITER_NODE;
	if (sect_iter_node == NULL) {
		rc = SA_AIS_ERR_NO_MEMORY;
		TRACE_4("cpa SectIterInit Api failed with return value:%d,ckptHandle:%llx", rc, checkpointHandle);
		goto iter_alloc_fail;
	}
	memset(sect_iter_node, 0, sizeof(CPA_SECT_ITER_NODE));

	sect_iter_node->iter_id = NCS_PTR_TO_UNS64_CAST(sect_iter_node);
	sect_iter_node->lcl_ckpt_hdl = lc_node->lcl_ckpt_hdl;
	sect_iter_node->gbl_ckpt_hdl = lc_node->gbl_ckpt_hdl;

	/* Convert the Time to millisecs */
	sect_iter_node->exp_time = expirationTime;
	sect_iter_node->filter = sectionsChosen;

	proc_rc = cpa_sect_iter_node_add(&cb->sect_iter_tree, sect_iter_node);
	if (proc_rc != NCSCC_RC_SUCCESS) {
		rc = SA_AIS_ERR_LIBRARY;
		TRACE_4("cpa SectIterInit Api failed with return value:%d,ckptHandle:%llx ", rc, checkpointHandle);
		goto iter_add_fail;
	} else {
		*sectionIterationHandle = sect_iter_node->iter_id;
		goto done;
	}

 iter_add_fail:
	m_MMGR_FREE_CPA_SECT_ITER_NODE(sect_iter_node);

 iter_alloc_fail:
 ckpt_node_get_fail:
 done:
	m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);

 lock_fail:
 clm_left:
	m_CPA_GIVEUP_CB;
	TRACE_LEAVE2("API return code = %u", rc);
	return rc;
}

/****************************************************************************
  Name          :  saCkptSectionIteratorNext
 
  Description   :  This function iterates over an internal table of sections.
                   
  Arguments     :  sectionIterator - A pointer to the iterator for stepping 
                         through the sections in the checkpoint.
                   sectionDescriptor - A pointer to the 
                                SaCkptSectionDescriptorT structure.
                                
  Return Values :  Refer to SAI-AIS specification for various return values.
 
  Notes         :
******************************************************************************/
SaAisErrorT saCkptSectionIterationNext(SaCkptSectionIterationHandleT sectionIterationHandle,
				       SaCkptSectionDescriptorT *sectionDescriptor)
{
	SaAisErrorT rc = SA_AIS_OK;
	uint32_t proc_rc = NCSCC_RC_SUCCESS;
	CPA_SECT_ITER_NODE *sect_iter_node = NULL;
	CPSV_EVT evt;
	CPSV_EVT *out_evt = NULL;
	CPA_CB *cb = NULL;
	CPA_GLOBAL_CKPT_NODE *gc_node = NULL;
	bool add_flag = false;
	bool is_local_get_next = false;
	CPA_CLIENT_NODE *cl_node = NULL;
	CPA_LOCAL_CKPT_NODE *lc_node = NULL;

	if (sectionDescriptor == NULL)
		return SA_AIS_ERR_INVALID_PARAM;

	TRACE_ENTER2("SaCkptSectionIterationHandleT passed is %llx",sectionIterationHandle);
	/* retrieve CPA CB */
	m_CPA_RETRIEVE_CB(cb);
	if (!cb) {
		rc = SA_AIS_ERR_BAD_HANDLE;
		TRACE_4("cpa SectIterNext Api failed with return value:%d,sectioniterationHandle:%llx", rc, sectionIterationHandle);
		goto done;
	}

	/* get the CB Lock */
	if (m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
		rc = SA_AIS_ERR_LIBRARY;
		TRACE_4("cpa SectIterNext:LOCK Api failed with return value:%d,sectionIterationHandle:%llx", rc, sectionIterationHandle);
		goto lock_fail;
	}

	proc_rc = cpa_sect_iter_node_get(&cb->sect_iter_tree, &sectionIterationHandle, &sect_iter_node);
	if (!sect_iter_node) {
		rc = SA_AIS_ERR_BAD_HANDLE;
		TRACE_4("cpa SectIterNext:sect_iter_node_get Api failed with return value:%d,sectionIterationHandle:%llx", rc, sectionIterationHandle);
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
		goto sect_iter_get_fail;
	}

	proc_rc = cpa_lcl_ckpt_node_get(&cb->lcl_ckpt_tree, &sect_iter_node->lcl_ckpt_hdl, &lc_node);
	if (!lc_node) {
		rc = SA_AIS_ERR_BAD_HANDLE;
		TRACE_4("cpa StatusGet Api failed with return value:%d,lcl_ckpt_hdl:%llx", rc, sect_iter_node->lcl_ckpt_hdl);
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
		goto fail1;
	}

	proc_rc = cpa_client_node_get(&cb->client_tree, &lc_node->cl_hdl, &cl_node);

	if (!cl_node) {
		TRACE_4("cpa SelObjGet:client_node_get cl_hdl:%llx ", lc_node->cl_hdl);
		rc = SA_AIS_ERR_BAD_HANDLE;
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
		goto fail1;
	}

	/* Checking Node availability */
	if (m_CPA_VER_IS_ABOVE_B_1_1(&cl_node->version)) {
		if ((cb->is_cpnd_joined_clm != true)  || ( cl_node->stale == true)) {
			TRACE_4("Cpa CLM Node left return value:%d",SA_AIS_ERR_UNAVAILABLE);
			rc = SA_AIS_ERR_UNAVAILABLE;
			m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
			goto clm_left;
		}
	}

	proc_rc = cpa_gbl_ckpt_node_find_add(&cb->gbl_ckpt_tree, &sect_iter_node->gbl_ckpt_hdl, &gc_node, &add_flag);

	if (!gc_node) {
		TRACE_4("cpa gbl ckpt find add failed");
		rc = SA_AIS_ERR_NO_RESOURCES;
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
		TRACE_4("Cpa SectIterNext Api failed with return value:%d,sectionInterationHandle:%llx" , rc, sectionIterationHandle);
		goto sect_iter_get_fail;
	}

	if (((m_CPA_IS_COLLOCATED_ATTR_SET(gc_node->ckpt_creat_attri.creationFlags) == true)
	     && (m_CPA_IS_ALL_REPLICA_ATTR_SET(gc_node->ckpt_creat_attri.creationFlags) == true))) {
		is_local_get_next = true;
	}

	if (is_local_get_next == false) {
		if (gc_node->is_restart) {
			rc = SA_AIS_ERR_TRY_AGAIN;
			m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
			TRACE_1("Cpa SectIterNext Api failed with return value:%d,sectionInterationHandle:%llx", rc, sectionIterationHandle);
			goto sect_iter_get_fail;
		}

		if (!gc_node->is_active_exists) {
			proc_rc = cpa_lcl_ckpt_node_get(&cb->lcl_ckpt_tree, &sect_iter_node->lcl_ckpt_hdl, &lc_node);
			if (!lc_node) {
				rc = SA_AIS_ERR_BAD_HANDLE;
				TRACE_4("Cpa StatusGet failed with with return value:%d,lcl_ckpt_hdl:%llx", rc, sect_iter_node->lcl_ckpt_hdl);
				m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
				goto fail1;
			}

			proc_rc = cpa_client_node_get(&cb->client_tree, &lc_node->cl_hdl, &cl_node);

			if (!cl_node) {
				TRACE_4("Cpa SelObjGet:client_node_get Api failed with return value:%d,cl_hdl:%llx", SA_AIS_ERR_BAD_HANDLE,
						lc_node->cl_hdl);
				rc = SA_AIS_ERR_BAD_HANDLE;
				m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
				goto fail1;
			}

			if (m_CPA_VER_IS_ABOVE_B_1_1(&cl_node->version)) {
				if (m_CPA_IS_ALL_REPLICA_ATTR_SET(gc_node->ckpt_creat_attri.creationFlags) == false) {
					rc = SA_AIS_ERR_NOT_EXIST;
					m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
					TRACE_4("Cpa SectIterNext failed with return value:%d,sectionInterationHandle:%llx", rc, sectionIterationHandle);
					goto sect_iter_get_fail;
				}
			} else {
				rc = SA_AIS_ERR_NOT_EXIST;
				m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
				TRACE_4("Cpa SectIterNext failed with return value:%d,sectionInterationHandle:%llx", rc, sectionIterationHandle);
				goto sect_iter_get_fail;
			}

		}

	} else {
		if (false == cb->is_cpnd_up) {
			rc = SA_AIS_ERR_TRY_AGAIN;
			m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
			TRACE_1("Cpa SectIterNext failed with return value:%d,sectionInterationHandle:%llx", rc, sectionIterationHandle);
			goto sect_iter_get_fail;
		}
	}

	/* Populate the event & send it to CPND */
	memset(&evt, 0, sizeof(CPSV_EVT));
	evt.type = CPSV_EVT_TYPE_CPND;
	evt.info.cpnd.type = CPND_EVT_A2ND_CKPT_ITER_GETNEXT;
	evt.info.cpnd.info.iter_getnext.ckpt_id = sect_iter_node->gbl_ckpt_hdl;
	evt.info.cpnd.info.iter_getnext.section_id = sect_iter_node->section_id;
	evt.info.cpnd.info.iter_getnext.iter_id = sect_iter_node->iter_id;
	evt.info.cpnd.info.iter_getnext.filter = sect_iter_node->filter;
	evt.info.cpnd.info.iter_getnext.n_secs_trav = sect_iter_node->n_secs_trav;
	evt.info.cpnd.info.iter_getnext.exp_tmr = sect_iter_node->exp_time;

	m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
	if (is_local_get_next == false) {
		proc_rc = cpa_mds_msg_sync_send(cb->cpa_mds_hdl, &(gc_node->active_mds_dest),
						&evt, &out_evt, CPSV_WAIT_TIME);
	} else {
		proc_rc = cpa_mds_msg_sync_send(cb->cpa_mds_hdl, &(cb->cpnd_mds_dest), &evt, &out_evt, CPSV_WAIT_TIME);
	}

	/* Generate rc from proc_rc */
	switch (proc_rc) {
	case NCSCC_RC_SUCCESS:
		break;
	case NCSCC_RC_REQ_TIMOUT:
		rc = SA_AIS_ERR_TIMEOUT;
		TRACE_4("Cpa SectIterNext:MDS api failed with return value:%d,sectionIterationHandle:%llx,cpnd_mds_dest:%"PRIu64,
                        proc_rc , sectionIterationHandle, cb->cpnd_mds_dest);
		goto fail1;
	default:
		rc = SA_AIS_ERR_TRY_AGAIN;

		TRACE_4("Cpa SectIterNext:MDS api failed with return value:%d,sectionIterationHandle:%llx,cpnd_mds_dest:%"PRIu64,
                        proc_rc , sectionIterationHandle, cb->cpnd_mds_dest);
		goto fail1;
	}

	if (out_evt) {
		rc = out_evt->info.cpa.info.iter_next_rsp.error;
		if (rc == SA_AIS_OK) {
			*sectionDescriptor = out_evt->info.cpa.info.iter_next_rsp.sect_desc;
			sect_iter_node->n_secs_trav = out_evt->info.cpa.info.iter_next_rsp.n_secs_trav;

			if (sect_iter_node->section_id.id) {
				/* EDU Library forced us to use NCS_SERVICE_ID_CPND for freeing section_id.id */
				m_MMGR_FREE_CPSV_DEFAULT_VAL(sect_iter_node->section_id.id, NCS_SERVICE_ID_CPND);
			}

			sect_iter_node->section_id = out_evt->info.cpa.info.iter_next_rsp.sect_desc.sectionId;
			if (sect_iter_node->out_evt != NULL) {
				m_MMGR_FREE_CPSV_EVT(sect_iter_node->out_evt, NCS_SERVICE_ID_CPA);
			}
			sect_iter_node->out_evt = out_evt;
			out_evt = NULL;

		}
		/* need to free memory of sec id */
		if (out_evt != NULL)
			m_MMGR_FREE_CPSV_EVT(out_evt, NCS_SERVICE_ID_CPA);

		TRACE_1("Cpa SectIterNext Api Success with return value:%d,sectionInterationHandle:%llx", rc, sectionIterationHandle);
	} else {
		rc = SA_AIS_ERR_NO_RESOURCES;
		TRACE_4("Cpa SectIterNext Api failed with return value:%d,sectionInterationHandle:%llx", rc, sectionIterationHandle);
	}
 sect_iter_get_fail:
 fail1:
 lock_fail:
 clm_left:
	m_CPA_GIVEUP_CB;

 done:
	TRACE_LEAVE2("API return code = %u", rc);
	return rc;
}

/****************************************************************************
  Name          :  saCkptSectionIterationFinalize
 
  Description   :  This function frees resources allocated for iteration.
 
  Arguments     :  sectionIterator - A pointer to a structure obtained from an 
                          invocation of the saCkptSectionIteratorInitialize()
                          function.
 
  Return Values :  Refer to SAI-AIS specification for various return values.
 
  Notes         :
******************************************************************************/
SaAisErrorT saCkptSectionIterationFinalize(SaCkptSectionIterationHandleT sectionIterationHandle)
{
	SaAisErrorT rc = SA_AIS_OK;
	CPA_SECT_ITER_NODE *sect_iter_node = NULL;
	CPA_CB *cb = NULL;
	CPA_GLOBAL_CKPT_NODE *gc_node = NULL;
	bool add_flag = false;
	bool is_local_get_next = false;
	CPA_CLIENT_NODE *cl_node = NULL;
	CPA_LOCAL_CKPT_NODE *lc_node = NULL;

	TRACE_ENTER2("SaCkptSectionIterationHandleT passed is %llx",sectionIterationHandle);
	/* retrieve CPA CB */
	m_CPA_RETRIEVE_CB(cb);
	if (!cb) {
		rc = SA_AIS_ERR_BAD_HANDLE;
		TRACE_4("Cpa SectIterFinalize:HDL_TAKE Api failed with return value:%d,sectionIterationHandle:%llx", rc, sectionIterationHandle);
		goto done;
	}

	/* get the CB Lock */
	if (m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
		rc = SA_AIS_ERR_LIBRARY;
		TRACE_4("Cpa SectIterFinalize:LOCK Api failed with return value:%d,sectionIterationHandle:%llx", rc, sectionIterationHandle);
		goto lock_fail;
	}
	(void)cpa_sect_iter_node_get(&cb->sect_iter_tree, &sectionIterationHandle, &sect_iter_node);
	if (!sect_iter_node) {
		rc = SA_AIS_ERR_BAD_HANDLE;
		TRACE_4("Cpa SectIterFinalize failed with return value:%d,sectionIterationHandle:%llx", rc, sectionIterationHandle);
		goto sect_iter_get_fail;
	}
	(void)cpa_lcl_ckpt_node_get(&cb->lcl_ckpt_tree, &sect_iter_node->lcl_ckpt_hdl, &lc_node);
	if (!lc_node) {
		rc = SA_AIS_ERR_BAD_HANDLE;
		TRACE_4("Cpa StatusGet Api failed with return value:%d,lcl_ckpt_hdl:%llx", rc, sect_iter_node->lcl_ckpt_hdl);
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
		goto fail1;
	}
	(void)cpa_client_node_get(&cb->client_tree, &lc_node->cl_hdl, &cl_node);
	if (!cl_node) {
		TRACE_4("Cpa SelObjGet:client_node_get failed with return value:%d,cl_hdl:%llx",SA_AIS_ERR_BAD_HANDLE, lc_node->cl_hdl);
		rc = SA_AIS_ERR_BAD_HANDLE;
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
		goto fail1;
	}

	if (m_CPA_VER_IS_ABOVE_B_1_1(&cl_node->version)) {
		if ((cb->is_cpnd_joined_clm != true)  || ( cl_node->stale == true)) {
			TRACE_4("Cpa CLM Node left ,return value:%d", SA_AIS_ERR_UNAVAILABLE);
			rc = SA_AIS_ERR_UNAVAILABLE;
			m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
			goto clm_left;
		}
	}

	(void)cpa_gbl_ckpt_node_find_add(&cb->gbl_ckpt_tree, &sect_iter_node->gbl_ckpt_hdl, &gc_node, &add_flag);

	if (!gc_node) {
		TRACE_4("cpa - Global Ckpt Find Add Failed");
		rc = SA_AIS_ERR_NO_RESOURCES;
		TRACE_4("Cpa SectIterFinalize failed with return value:%d,sectionIterationHandle:%llx", rc, sectionIterationHandle);
		goto sect_iter_get_fail;
	}

	if (((m_CPA_IS_COLLOCATED_ATTR_SET(gc_node->ckpt_creat_attri.creationFlags) == true)
	     && (m_CPA_IS_ALL_REPLICA_ATTR_SET(gc_node->ckpt_creat_attri.creationFlags) == true))) {
		is_local_get_next = true;
	}

	if (is_local_get_next == false) {

		if (gc_node->is_restart) {
			rc = SA_AIS_ERR_TRY_AGAIN;
			TRACE_1("Cpa SectIterFinalize failed with return value:%d,sectionIterationHandle:%llx", rc, sectionIterationHandle);
			goto sect_iter_get_fail;
		}

		if (!gc_node->is_active_exists) {
			rc = SA_AIS_ERR_NOT_EXIST;
			TRACE_4("Cpa SectIterFinalize failed with return value:%d,sectionIterationHandle:%llx", rc, sectionIterationHandle);
			goto sect_iter_get_fail;
		}
	} else {
		if (false == cb->is_cpnd_up) {
			rc = SA_AIS_ERR_TRY_AGAIN;
			TRACE_1("Cpa SectIterFinalize failed with return value:%d,sectionIterationHandle:%llx", rc, sectionIterationHandle);
			goto sect_iter_get_fail;
		}
	}

	/* Delete the section iteration Node */
	if (NCSCC_RC_SUCCESS != cpa_sect_iter_node_delete(cb, sect_iter_node)) {
		rc = SA_AIS_ERR_LIBRARY;
		TRACE_4("Cpa SectIterFinalize failed with return value:%d,sectionIterationHandle:%llx", rc, sectionIterationHandle);
		goto sect_iter_get_fail;
	}

 sect_iter_get_fail:
	m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);

 lock_fail:
 clm_left:
 fail1:
	m_CPA_GIVEUP_CB;

 done:
	TRACE_LEAVE2("API return code = %u", rc);
	return rc;
}

/****************************************************************************
  Name          :  saCkptCheckpointWrite
 
  Description   :  This function writes data from the memory regions specified
                   by ioVector into a checkpoint.
 
  Arguments     :  checkpointHandle - Handle that designates the checkpoint.
                   ioVector - A pointer to a vector.
                   numberOfElements - Size of the ioVector.
                   erroneousVectorIndex - A pointer to an index, stored in 
                   caller's address space.
 
  Return Values :  Refer to SAI-AIS specification for various return values.
 
  Notes         :
******************************************************************************/
SaAisErrorT saCkptCheckpointWrite(SaCkptCheckpointHandleT checkpointHandle,
				  const SaCkptIOVectorElementT *ioVector,
				  SaUint32T numberOfElements, SaUint32T *erroneousVectorIndex)
{
	SaAisErrorT rc = SA_AIS_OK;
	uint32_t proc_rc;
	CPA_LOCAL_CKPT_NODE *lc_node = NULL;
	CPA_CB *cb = NULL;
	CPSV_EVT evt, *out_evt = NULL;
	CPSV_CKPT_DATA *ckpt_data = NULL;
	uint32_t iter = 0;
	CPA_GLOBAL_CKPT_NODE *gc_node = NULL;
	bool add_flag = false;
	CPA_CLIENT_NODE *cl_node = NULL;
	SaSizeT all_ioVector_size = 0;
	uint32_t err_flag = 0;
	
	TRACE_ENTER2("SaCkptCheckpointHandleT passed is %llx",checkpointHandle);

	memset(&evt, '\0', sizeof(CPSV_EVT));

	if (ioVector == NULL) {
		rc = SA_AIS_ERR_INVALID_PARAM;
		TRACE_4("cpa CkptWrite Api failed with return value:%d,ckptHandle:%llx", rc, checkpointHandle);
		return rc;
	}

	if (erroneousVectorIndex != NULL)
		*erroneousVectorIndex = 0;


	/* retrieve CPA CB */
	m_CPA_RETRIEVE_CB(cb);
	if (!cb) {
		rc = SA_AIS_ERR_BAD_HANDLE;
		TRACE_4("cpa CkptWrite Api failed with return value:%d,ckptHandle:%llx", rc, checkpointHandle);
		return rc;
	}

	/* get the CB Lock */
	if (m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
		rc = SA_AIS_ERR_LIBRARY;
		TRACE_4("cpa CkptWrite Api failed with return value:%d,ckptHandle:%llx", rc, checkpointHandle);
		goto lock_fail;
	}

	proc_rc = cpa_lcl_ckpt_node_get(&cb->lcl_ckpt_tree, &checkpointHandle, &lc_node);
	if (!lc_node) {
		rc = SA_AIS_ERR_BAD_HANDLE;
		TRACE_4("cpa CkptWrite Api failed with return value:%d,ckptHandle:%llx", rc, checkpointHandle);
		goto ckpt_node_get_fail;
	}

	proc_rc = cpa_client_node_get(&cb->client_tree, &lc_node->cl_hdl, &cl_node);
	if (!cl_node) {
		TRACE_4("Cpa SelObjGet:client_node_get failed with return value:%d,cl_hdl:%llx", SA_AIS_ERR_BAD_HANDLE, lc_node->cl_hdl);
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto ckpt_node_get_fail;
	}

	if (m_CPA_VER_IS_ABOVE_B_1_1(&cl_node->version)) {
		if ((cb->is_cpnd_joined_clm != true)  || ( cl_node->stale == true)) {
			TRACE_4("Cpa  CLM Node left, return value:%d", SA_AIS_ERR_UNAVAILABLE);
			rc = SA_AIS_ERR_UNAVAILABLE;
			m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
			goto clm_left;
		}
	}
	proc_rc = cpa_proc_check_iovector(cb, lc_node, ioVector, numberOfElements, &err_flag);
	if (proc_rc != NCSCC_RC_SUCCESS) {
		if (erroneousVectorIndex != NULL)
			*erroneousVectorIndex = err_flag;
	
		rc = SA_AIS_ERR_INVALID_PARAM;
		TRACE_4("cpa CkptWrite Api failed with return value:%d,ckptHandle:%llx", rc, checkpointHandle);
		goto ckpt_node_get_fail;
	}

	for (iter = 0; iter < numberOfElements; iter++) {
		if (ioVector[iter].dataSize > CPSV_MAX_DATA_SIZE) {
			rc = SA_AIS_ERR_NO_RESOURCES;
			TRACE_4("cpa CkptWrite Api failed with return value:%d,ckptHandle:%llx", rc, checkpointHandle);
			goto ckpt_node_get_fail;
		}
	}

	/* If Write is not allowed return the error */
	if (!(lc_node->open_flags & SA_CKPT_CHECKPOINT_WRITE)) {
		rc = SA_AIS_ERR_ACCESS;
		TRACE_4("cpa CkptWrite Api failed with return value:%d,ckptHandle:%llx", rc, checkpointHandle);
		goto write_error;
	}

	proc_rc = cpa_gbl_ckpt_node_find_add(&cb->gbl_ckpt_tree, &lc_node->gbl_ckpt_hdl, &gc_node, &add_flag);

	if (!gc_node) {
		TRACE_4("cpa - Global Ckpt Find Add Failed");
		rc = SA_AIS_ERR_NO_RESOURCES;
		goto write_error;
	}

	if (gc_node->is_restart) {
		rc = SA_AIS_ERR_TRY_AGAIN;
		TRACE_4("cpa CkptWrite Api failed with return value:%d,ckptHandle:%llx", rc, checkpointHandle);
		goto write_error;
	}

	if (!gc_node->is_active_exists) {
		rc = SA_AIS_ERR_NOT_EXIST;
		TRACE_4("cpa CkptWrite Api failed with return value:%d,ckptHandle:%llx", rc, checkpointHandle);
		goto write_error;
	}
	/* Populate the Ckpt data structure,i.e  build CPSV_CKPT_DATA from ioVector */
	proc_rc = cpa_proc_build_data_access_evt(ioVector, numberOfElements,
						 CPSV_CKPT_ACCESS_WRITE, gc_node->ckpt_creat_attri.maxSectionSize,
						 erroneousVectorIndex, &ckpt_data);
	if (proc_rc == NCSCC_RC_FAILURE) {
		rc = SA_AIS_ERR_INVALID_PARAM;
		TRACE_4("cpa CkptWrite Api failed with return value:%d,ckptHandle:%llx", rc, checkpointHandle);
		goto done;
	}

	/* Populate the event & send it to CPND */

	evt.type = CPSV_EVT_TYPE_CPND;
	evt.info.cpnd.type = CPND_EVT_A2ND_CKPT_WRITE;
	evt.info.cpnd.info.ckpt_write.type = CPSV_CKPT_ACCESS_WRITE;
	evt.info.cpnd.info.ckpt_write.ckpt_id = lc_node->gbl_ckpt_hdl;
	evt.info.cpnd.info.ckpt_write.lcl_ckpt_id = lc_node->lcl_ckpt_hdl;
	evt.info.cpnd.info.ckpt_write.agent_mdest = cb->cpa_mds_dest;
	evt.info.cpnd.info.ckpt_write.num_of_elmts = numberOfElements;
	evt.info.cpnd.info.ckpt_write.data = ckpt_data;

	/* Unlock cpa_lock before calling mds api */
	m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
#if 1
	for (iter = 0; iter < numberOfElements; iter++)
		all_ioVector_size += ioVector[iter].dataSize;

	proc_rc = cpa_mds_msg_sync_send(cb->cpa_mds_hdl, &(gc_node->active_mds_dest),
					&evt, &out_evt, CPA_WAIT_TIME(all_ioVector_size));
#else
	proc_rc = cpa_mds_msg_sync_send(cb->cpa_mds_hdl, &(gc_node->active_mds_dest), &evt, &out_evt, CPSV_WAIT_TIME);
#endif
   /* Generate rc from proc_rc */
   switch (proc_rc)
   {
       case NCSCC_RC_SUCCESS: 
           break;
       case NCSCC_RC_REQ_TIMOUT:
           rc = SA_AIS_ERR_TIMEOUT;
		TRACE_4("cpa CkptWrite:MDS api failed with return value:%d,ckptHandle:%llx,cpnd_mds_dest:%"PRIu64, 
			proc_rc , checkpointHandle, cb->cpnd_mds_dest);
           goto fail1;
       default:
           rc = SA_AIS_ERR_TRY_AGAIN;
		TRACE_4("cpa CkptWrite:MDS api failed with return value:%d,ckptHandle:%llx,cpnd_mds_dest:%"PRIu64,
			proc_rc , checkpointHandle, cb->cpnd_mds_dest);
           goto fail1;
   }
   
   if(out_evt)
   {
	if (erroneousVectorIndex != NULL)
      		*erroneousVectorIndex = out_evt->info.cpa.info.sec_data_rsp.error_index;

      if (out_evt->info.cpa.info.sec_data_rsp.num_of_elmts == -1)
      {
         rc=out_evt->info.cpa.info.sec_data_rsp.error;
         TRACE_4("Cpa CkptWrite Api failed with return value:%d,ckptHandle:%llx", rc, checkpointHandle);
      }
      else {
	      if (out_evt->info.cpa.info.sec_data_rsp.info.write_err_index != NULL)
		      m_MMGR_FREE_CPSV_SaUint32T(out_evt->info.cpa.info.sec_data_rsp.info.write_err_index,NCS_SERVICE_ID_CPA);
      }

      m_MMGR_FREE_CPSV_EVT(out_evt,NCS_SERVICE_ID_CPA);
      TRACE_1("Cpa CkptWrite Api Success with return value:%d,ckptHandle:%llx", SA_AIS_OK, checkpointHandle);
   }
   else
   {
      rc = SA_AIS_ERR_NO_RESOURCES;
      TRACE_4("cpa CkptWrite Api failed with return value:%d,ckptHandle:%llx", rc, checkpointHandle);
   }
      

	goto end;

	/* Allocate the section Iretation Node */
 ckpt_node_get_fail:
 write_error:
 done:
	m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);

 fail1:
 lock_fail:
 clm_left:
 end:
	cpa_proc_free_cpsv_ckpt_data(ckpt_data);
	m_CPA_GIVEUP_CB;
	TRACE_LEAVE2("API return code = %u", rc);
	return rc;
}

/****************************************************************************
  Name          :  saCkptSectionOverwrite
 
  Description   :  This function is similar to saCkptCheckpointWrite() except 
                   that it over-writes only a single section.
 
  Arguments     :  checkpointHandle - Handle that designates the checkpoint.
                   sectionId - A pointer to an identifier for the section that
                               is to be overwritten.
                   dataBuffer - A pointer to a buffer that contains the data
                                to be written.
                   dataSize - The size in bytes of the data to be written.
 
  Return Values :  Refer to SAI-AIS specification for various return values.
 
  Notes         :
******************************************************************************/
SaAisErrorT saCkptSectionOverwrite(SaCkptCheckpointHandleT checkpointHandle,
				   const SaCkptSectionIdT *sectionId, const void *dataBuffer, SaSizeT dataSize)
{
	SaAisErrorT rc = SA_AIS_OK;
	uint32_t proc_rc;
	CPA_LOCAL_CKPT_NODE *lc_node = NULL;
	CPA_CB *cb = NULL;
	CPSV_EVT evt, *out_evt = NULL;
	CPSV_CKPT_DATA ckpt_data;
	CPA_GLOBAL_CKPT_NODE *gc_node = NULL;
	bool add_flag = false;
	CPA_CLIENT_NODE *cl_node = NULL;

	TRACE_ENTER2("SaCkptCheckpointHandleT passed is %llx",checkpointHandle);
	memset(&ckpt_data, '\0', sizeof(CPSV_CKPT_DATA));
	memset(&evt, '\0', sizeof(CPSV_EVT));
	

	if (sectionId == NULL || dataBuffer == NULL) {
		rc = SA_AIS_ERR_INVALID_PARAM;
		TRACE_4("cpa CkptOverWrite Api failed with return value:%d,ckptHandle:%llx", rc, checkpointHandle);
		return rc;
	}

	/* retrieve CPA CB */
	m_CPA_RETRIEVE_CB(cb);
	if (!cb) {
		rc = SA_AIS_ERR_BAD_HANDLE;
		TRACE_4("cpa CkptOverWrite Api failed with return value:%d,ckptHandle:%llx", rc, checkpointHandle);
		return rc;
	}

	/* get the CB Lock */
	if (m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
		rc = SA_AIS_ERR_LIBRARY;
		TRACE_4("cpa CkptOverWrite Api failed with return value:%d,ckptHandle:%llx", rc, checkpointHandle);
		goto lock_fail;
	}

	proc_rc = cpa_lcl_ckpt_node_get(&cb->lcl_ckpt_tree, &checkpointHandle, &lc_node);
	if (!lc_node) {
		rc = SA_AIS_ERR_BAD_HANDLE;
		TRACE_4("cpa CkptOverWrite Api failed with return value:%d,ckptHandle:%llx", rc, checkpointHandle);
		goto ckpt_node_get_fail;
	}
	proc_rc = cpa_client_node_get(&cb->client_tree, &lc_node->cl_hdl, &cl_node);
	if (!cl_node) {
		TRACE_4("Cpa SelObjGet:client_node_get failed with return value:%d,cl_hdl:%llx",SA_AIS_ERR_BAD_HANDLE, lc_node->cl_hdl);
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto ckpt_node_get_fail;
	}
	if (m_CPA_VER_IS_ABOVE_B_1_1(&cl_node->version)) {
		if ((cb->is_cpnd_joined_clm != true)  || ( cl_node->stale == true)) {
			TRACE_4("Cpa CLM Node left, with return value:%d",SA_AIS_ERR_UNAVAILABLE);
			rc = SA_AIS_ERR_UNAVAILABLE;
			m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
			goto clm_left;
		}
	}

	/* Don't allow the Overwrite, in case if SA_CKPT_CHECKPOINT_WRITE is 
	   not set */
	if (!(lc_node->open_flags & SA_CKPT_CHECKPOINT_WRITE)) {
		rc = SA_AIS_ERR_ACCESS;
		TRACE_4("cpa CkptOverWrite Api failed with return value:%d,ckptHandle:%llx", rc, checkpointHandle);
		goto no_access;
	}

	/* Get the Global Ckpt node */
	proc_rc = cpa_gbl_ckpt_node_find_add(&cb->gbl_ckpt_tree, &lc_node->gbl_ckpt_hdl, &gc_node, &add_flag);
	if (!gc_node) {
		rc = SA_AIS_ERR_NO_MEMORY;
		TRACE_4("cpa CkptOverWrite Api failed with return value:%d,ckptHandle:%llx", proc_rc, checkpointHandle);
		goto done;
	}

	if (gc_node->ckpt_creat_attri.maxSectionSize < dataSize) {
		rc = SA_AIS_ERR_INVALID_PARAM;
		TRACE_4("cpa CkptOverWrite Api failed with return value:%d,ckptHandle:%llx", rc, checkpointHandle);
		goto done;
	}

	if (dataSize > CPSV_MAX_DATA_SIZE) {
		rc = SA_AIS_ERR_NO_RESOURCES;
		TRACE_4("cpa CkptOverWrite Api failed with return value:%d,ckptHandle:%llx", rc, checkpointHandle);
		goto done;
	}

	if (gc_node->is_restart) {
		rc = SA_AIS_ERR_TRY_AGAIN;
		TRACE_4("cpa CkptOverWrite Api failed with return value:%d,ckptHandle:%llx", rc, checkpointHandle);
		goto done;
	}

	if (!gc_node->is_active_exists) {
		rc = SA_AIS_ERR_NOT_EXIST;
		TRACE_4("cpa CkptOverWrite Api failed with return value:%d,ckptHandle:%llx", rc, checkpointHandle);
		goto done;
	}

	ckpt_data.sec_id = *sectionId;
	ckpt_data.data = (void *)dataBuffer;
	ckpt_data.dataSize = dataSize;
	ckpt_data.next = NULL;

	/* Populate the event & send it to CPND */

	evt.type = CPSV_EVT_TYPE_CPND;
	evt.info.cpnd.type = CPND_EVT_A2ND_CKPT_WRITE;
	evt.info.cpnd.info.ckpt_write.type = CPSV_CKPT_ACCESS_OVWRITE;
	evt.info.cpnd.info.ckpt_write.ckpt_id = lc_node->gbl_ckpt_hdl;
	evt.info.cpnd.info.ckpt_write.lcl_ckpt_id = lc_node->lcl_ckpt_hdl;
	evt.info.cpnd.info.ckpt_write.agent_mdest = cb->cpa_mds_dest;
	evt.info.cpnd.info.ckpt_write.num_of_elmts = 1;
	evt.info.cpnd.info.ckpt_write.data = &ckpt_data;

	/* Unlock cpa_lock before calling mds api */
	m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);

	proc_rc = cpa_mds_msg_sync_send(cb->cpa_mds_hdl, &(gc_node->active_mds_dest),
					&evt, &out_evt, CPA_WAIT_TIME(dataSize));

	/* Generate rc from proc_rc */
	switch (proc_rc) {
	case NCSCC_RC_SUCCESS:
		break;
	case NCSCC_RC_REQ_TIMOUT:
		rc = SA_AIS_ERR_TIMEOUT;
		TRACE_4("Cpa CkptOverWrite:MDS api failed with return value:%d,ckptHandle:%llx,cpnd_mds_dest:%"PRIu64,
			proc_rc, checkpointHandle, cb->cpnd_mds_dest);
		goto fail1;
	default:
		rc = SA_AIS_ERR_TRY_AGAIN;
		TRACE_4("Cpa CkptOverWrite:MDS api failed with return value:%d,ckptHandle:%llx,cpnd_mds_dest:%"PRIu64, 
			proc_rc, checkpointHandle, cb->cpnd_mds_dest);
		goto fail1;
	}

	if (out_evt) {
		rc = out_evt->info.cpa.info.sec_data_rsp.info.ovwrite_error.error;
		m_MMGR_FREE_CPSV_EVT(out_evt, NCS_SERVICE_ID_CPA);
		if (rc == SA_AIS_OK) {
			TRACE_1("cpa  CkptOverWrite Api failed with return value:%d,ckptHandle:%llx", rc, checkpointHandle);
		} else {
			TRACE_4("cpa CkptOverWrite Api failed with return value:%d,ckptHandle:%llx", rc, checkpointHandle);
		}
	} else {
		rc = SA_AIS_ERR_NO_RESOURCES;
		TRACE_4("cpa CkptOverWrite Api failed with return value:%d,ckptHandle:%llx", rc, checkpointHandle);
	}

	goto end;

	/* Allocate the section Iretation Node */
 ckpt_node_get_fail:
 no_access:
 done:
	m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);

 fail1:
 lock_fail:
 clm_left:
 end:
	m_CPA_GIVEUP_CB;
	TRACE_LEAVE2("API return code = %u", rc);
	return rc;
}

/****************************************************************************
  Name          :  saCkptCheckpointRead
 
  Description   :  This function copies data from a checkpoint replica into
                   the vector specified by ioVector.
 
  Arguments     :  checkpointHandle - Handle that designates the checkpoint.
                   ioVector - A pointer to a vector.
                   numberOfElements - The size of the ioVector. 
                   erroneousVectorIndex - A pointer to an index, in the 
                       caller's address space, of the first vector element
                       that causes the invocation to fail.
 
  Return Values :  Refer to SAI-AIS specification for various return values.
 
  Notes         :
******************************************************************************/
SaAisErrorT saCkptCheckpointRead(SaCkptCheckpointHandleT checkpointHandle,
				 SaCkptIOVectorElementT *ioVector,
				 SaUint32T numberOfElements, SaUint32T *erroneousVectorIndex)
{
	SaAisErrorT rc = SA_AIS_OK;
	uint32_t proc_rc, counter = 0;
	CPA_LOCAL_CKPT_NODE *lc_node = NULL;
	CPA_CB *cb = NULL;
	CPSV_EVT evt, *out_evt = NULL;
	CPSV_CKPT_DATA *ckpt_data = NULL;
	CPA_GLOBAL_CKPT_NODE *gc_node = NULL;
	bool add_flag = false;
	bool is_local_read = false;
	CPA_CLIENT_NODE *cl_node = NULL;

	TRACE_ENTER2("SaCkptCheckpointHandleT passed is %llx",checkpointHandle);
	memset(&evt, '\0', sizeof(CPSV_EVT));

	/* retrieve CPA CB */
	if (ioVector == NULL) {
		rc = SA_AIS_ERR_INVALID_PARAM;
		TRACE_4("cpa CkptRead Api failed with return value:%d,ckptHandle:%llx", rc, checkpointHandle);
		return rc;
	}

	if (erroneousVectorIndex != NULL)
		*erroneousVectorIndex = 0;

	/* DataBuffer is not Null but the dataSize is 0 ,ie no data to read , so return INVALID_PARAM */
	while (counter < numberOfElements) {
		if ((ioVector[counter].dataBuffer != NULL) && (ioVector[counter].dataSize == 0))
			return SA_AIS_ERR_INVALID_PARAM;
		counter++;
	}
	m_CPA_RETRIEVE_CB(cb);
	if (!cb) {
		rc = SA_AIS_ERR_BAD_HANDLE;
		TRACE_4("cpa CkptRead:HDL_TAKE Api failed with return value:%d,ckptHandle:%llx", rc, checkpointHandle);
		return rc;
	}

	/* get the CB Lock */
	if (m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
		rc = SA_AIS_ERR_LIBRARY;
		TRACE_4("cpa CkptRead:LOCK Api failed with return value:%d,ckptHandle:%llx", rc, checkpointHandle);
		goto lock_fail;
	}

	proc_rc = cpa_lcl_ckpt_node_get(&cb->lcl_ckpt_tree, &checkpointHandle, &lc_node);
	if (!lc_node) {
		rc = SA_AIS_ERR_BAD_HANDLE;
		TRACE_4("cpa CkptRead Api failed with return value:%d,ckptHandle:%llx", rc, checkpointHandle);
		goto ckpt_node_get_fail;
	}
	proc_rc = cpa_client_node_get(&cb->client_tree, &lc_node->cl_hdl, &cl_node);
	if (!cl_node) {
		TRACE_4("cpa SelObjGet:client_node_get Api failed with return value:%d,cl_hdl:%llx",SA_AIS_ERR_BAD_HANDLE, lc_node->cl_hdl);
		rc = SA_AIS_ERR_BAD_HANDLE;
		goto ckpt_node_get_fail;
	}
	if (m_CPA_VER_IS_ABOVE_B_1_1(&cl_node->version)) {
		if ((cb->is_cpnd_joined_clm != true)  || ( cl_node->stale == true)) {
			TRACE_4("cpa CLM Node left return value:%d", SA_AIS_ERR_UNAVAILABLE);
			rc = SA_AIS_ERR_UNAVAILABLE;
			m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
			goto clm_left;
		}

	
	}

	/* If Read is not allowed return the error */
	if (!(lc_node->open_flags & SA_CKPT_CHECKPOINT_READ)) {
		rc = SA_AIS_ERR_ACCESS;
		TRACE_4("cpa CkptRead Api failed with return value:%d,ckptHandle:%llx", rc, checkpointHandle);
		goto read_error;
	}

	proc_rc = cpa_gbl_ckpt_node_find_add(&cb->gbl_ckpt_tree, &lc_node->gbl_ckpt_hdl, &gc_node, &add_flag);

	if (!gc_node) {
		TRACE_4("cpa gbl ckpt find add failed");
		rc = SA_AIS_ERR_NO_RESOURCES;
		goto read_error;
	}

	if (((m_CPA_IS_COLLOCATED_ATTR_SET(gc_node->ckpt_creat_attri.creationFlags) == true)
	     && (m_CPA_IS_ALL_REPLICA_ATTR_SET(gc_node->ckpt_creat_attri.creationFlags) == true))) {
		is_local_read = true;
	}

	if (is_local_read == false) {

		if (gc_node->is_restart) {
			rc = SA_AIS_ERR_TRY_AGAIN;
			TRACE_1("cpa  CkptRead Api failed with return value:%d,ckptHandle:%llx", rc, checkpointHandle);
			goto read_error;
		}
		if (!gc_node->is_active_exists) {
			rc = SA_AIS_ERR_NOT_EXIST;
			TRACE_4("cpa CkptRead Api failed with return value:%d,ckptHandle:%llx", rc, checkpointHandle);
			goto read_error;
		}
	} else {
		if (false == cb->is_cpnd_up) {
			rc = SA_AIS_ERR_TRY_AGAIN;
			TRACE_1("cpa CkptRead Api failed with return value:%d,ckptHandle:%llx", rc, checkpointHandle);
			goto read_error;
		}
	}

	/* Populate the Ckpt data  */
	proc_rc = cpa_proc_build_data_access_evt(ioVector, numberOfElements,
						 CPSV_CKPT_ACCESS_READ, gc_node->ckpt_creat_attri.maxSectionSize,
						 erroneousVectorIndex, &ckpt_data);
	if (proc_rc == NCSCC_RC_FAILURE) {
		rc = SA_AIS_ERR_INVALID_PARAM;
		TRACE_4("cpa CkptRead Api failed with return value:%d,ckptHandle:%llx", proc_rc, checkpointHandle);
		goto done;
	}

	/* Populate the event & send it to CPND */
	evt.type = CPSV_EVT_TYPE_CPND;
	evt.info.cpnd.type = CPND_EVT_A2ND_CKPT_READ;
	evt.info.cpnd.info.ckpt_read.type = CPSV_CKPT_ACCESS_READ;
	evt.info.cpnd.info.ckpt_read.ckpt_id = lc_node->gbl_ckpt_hdl;
	evt.info.cpnd.info.ckpt_read.agent_mdest = cb->cpa_mds_dest;
	evt.info.cpnd.info.ckpt_read.num_of_elmts = numberOfElements;
	evt.info.cpnd.info.ckpt_read.data = ckpt_data;

	/* Unlock cpa_lock before calling mds api */
	m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
	if (false == is_local_read) {
		proc_rc = cpa_mds_msg_sync_send(cb->cpa_mds_hdl, &(gc_node->active_mds_dest),
						&evt, &out_evt, CPSV_WAIT_TIME);
	} else {
		proc_rc = cpa_mds_msg_sync_send(cb->cpa_mds_hdl, &(cb->cpnd_mds_dest), &evt, &out_evt, CPSV_WAIT_TIME);

	}

   /* Generate rc from proc_rc */
   switch (proc_rc)
   {
      case NCSCC_RC_SUCCESS:
         break;
      case NCSCC_RC_REQ_TIMOUT:
         rc = SA_AIS_ERR_TIMEOUT;
         TRACE_4("cpa CkptRead:MDS Api failed with return value:%d,ckptHandle:%llx,cpnd_mds_dest:%"PRIu64,
		proc_rc , checkpointHandle, cb->cpnd_mds_dest);
         goto fail1;
      default:
         rc = SA_AIS_ERR_TRY_AGAIN;
         TRACE_4("cpa CkptRead:MDS Api failed with return value:%d,ckptHandle:%llx,cpnd_mds_dest:%"PRIu64,
		proc_rc , checkpointHandle, cb->cpnd_mds_dest);
         goto fail1;
   }
   
   if(out_evt)
   {
      if(out_evt->info.cpa.info.sec_data_rsp.num_of_elmts == -1)
      {
	if (erroneousVectorIndex != NULL)
         	*erroneousVectorIndex = out_evt->info.cpa.info.sec_data_rsp.error_index;

         rc=out_evt->info.cpa.info.sec_data_rsp.error;
      }
      else 
      {
                        /* IF the buffer = NULL then CPSv allocates memory and user has to free this memory */

			proc_rc = cpa_proc_rmt_replica_read(numberOfElements,
							    ioVector,
							    out_evt->info.cpa.info.sec_data_rsp.info.read_data,
							    &erroneousVectorIndex, &cl_node->version);
			if (proc_rc == NCSCC_RC_FAILURE) {
				rc = SA_AIS_ERR_NOT_EXIST;
				TRACE_4("cpa CkptRead Api failed with return value:%d,ckptHandle:%llx", rc, checkpointHandle);
				goto end;
			}

/*               break;*/
			rc = SA_AIS_OK;
			TRACE_4("Cpa CkptRead Api failed with return value:%d,ckptHandle:%llx",SA_AIS_OK, checkpointHandle);

		}
	} else {
		rc = SA_AIS_ERR_NO_RESOURCES;
		TRACE_4("cpa CkptRead Api failed with return value:%d,ckptHandle:%llx", rc, checkpointHandle);
	}
	goto end;

 done:
 ckpt_node_get_fail:
 read_error:
	m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
	goto fail1;

 end:
	/* Free the out_evt contents */
	if (out_evt && (out_evt->info.cpa.info.sec_data_rsp.type == CPSV_DATA_ACCESS_LCL_READ_RSP
			|| out_evt->info.cpa.info.sec_data_rsp.type == CPSV_DATA_ACCESS_RMT_READ_RSP)) {
		cpa_proc_free_read_data(&out_evt->info.cpa.info.sec_data_rsp);
		m_MMGR_FREE_CPSV_EVT(out_evt, NCS_SERVICE_ID_CPA);
		out_evt = NULL;
	}

 fail1:
 lock_fail:
 clm_left:
	m_CPA_GIVEUP_CB;
	cpa_proc_free_cpsv_ckpt_data(ckpt_data);
	TRACE_LEAVE2("API return code = %u", rc);
	return rc;
}

SaAisErrorT saCkptIOVectorElementDataFree(SaCkptCheckpointHandleT checkpointHandle, void *data)
{
	SaAisErrorT rc = SA_AIS_OK;
	CPA_CB *cb = NULL;
	CPA_CLIENT_NODE *cl_node = NULL;
	CPA_LOCAL_CKPT_NODE *lc_node = NULL;
	
	TRACE_ENTER2("SaCkptCheckpointHandleT passed is %llx",checkpointHandle);
	if (data == NULL) {
		rc = SA_AIS_ERR_INVALID_PARAM;
		TRACE_4("cpa saCkptIOVectorElementDataFree Api failed with return value:%d,ckptHandle:%llx", rc, checkpointHandle);
		return rc;
	}
	m_CPA_RETRIEVE_CB(cb);
	if (!cb) {
		rc = SA_AIS_ERR_BAD_HANDLE;
		TRACE_4("cpa saCkptIOVectorElementDataFree:HDL_TAKE Api failed with return value:%d,ckptHandle:%llx", rc, checkpointHandle);
		return rc;
	}
	if (m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
		TRACE_4("cpa CkptInit:LOCK");
		rc = SA_AIS_ERR_LIBRARY;
		goto lock_fail;
	}
	(void)cpa_lcl_ckpt_node_get(&cb->lcl_ckpt_tree, &checkpointHandle, &lc_node);
	if (!lc_node) {
		rc = SA_AIS_ERR_BAD_HANDLE;
		TRACE_4("cpa StatusGet Api failed with return value:%d,ckptHandle:%llx", rc, checkpointHandle);
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
		goto fail;
	}
	(void)cpa_client_node_get(&cb->client_tree, &lc_node->cl_hdl, &cl_node);
	if (!cl_node) {
		TRACE_4("cpa SelObjGet:client_node_get Api failed with return value:%d,ckptHandle:%llx", SA_AIS_ERR_BAD_HANDLE, lc_node->cl_hdl);
		rc = SA_AIS_ERR_BAD_HANDLE;
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
		goto fail;
	}
	if (m_CPA_VER_IS_ABOVE_B_1_1(&cl_node->version)) {
		if ((cb->is_cpnd_joined_clm != true)  || ( cl_node->stale == true)) {
			TRACE_4("cpa CLM Node left with return value:%d", SA_AIS_ERR_UNAVAILABLE);
			rc = SA_AIS_ERR_UNAVAILABLE;
			m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
			goto clm_left;
		}
	}

	m_MMGR_FREE_CPA_DEFAULT(data);
	m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
 lock_fail:
 clm_left:
 fail:
	m_CPA_GIVEUP_CB;
	TRACE_LEAVE2("API return code = %u", rc);
	return rc;
}

/****************************************************************************
  Name          :  saCkptCheckpointSynchronize
 
  Description   :  This function ensures that all previous operations applied 
                   on the active checkpoint replica are propagated to other
                   checkpoint replicas.
 
  Arguments     :  checkpointHandle - Handle that designates the checkpoint.
                   timeout - The Function will terminate if the time it takes 
                             exceeds timeout.
 
  Return Values :  Refer to SAI-AIS specification for various return values.
 
  Notes         :
******************************************************************************/
SaAisErrorT saCkptCheckpointSynchronize(SaCkptCheckpointHandleT checkpointHandle, SaTimeT timeout)
{
	SaAisErrorT rc = SA_AIS_OK;
	CPSV_EVT evt;
	CPSV_EVT *out_evt = NULL;
	uint32_t proc_rc = NCSCC_RC_SUCCESS;
	CPA_LOCAL_CKPT_NODE *lc_node = NULL;
	CPA_GLOBAL_CKPT_NODE *gc_node = NULL;
	CPA_CB *cb = NULL;
	bool add_flag = false;
	CPA_CLIENT_NODE *cl_node = NULL;

	TRACE_ENTER2("SaCkptCheckpointHandleT passed is %llx",checkpointHandle);
	/* For Ckpt Sync CPA will not perform any operation 
	   other than passing the request to CPND */

	/* retrieve CPA CB */
	m_CPA_RETRIEVE_CB(cb);
	if (!cb) {
		rc = SA_AIS_ERR_BAD_HANDLE;
		TRACE_4("cpa CkptSynchronize Api failed with return value:%d,ckptHandle:%llx", rc, checkpointHandle);
		goto done;
	}

	/* get the CB Lock */
	if (m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
		rc = SA_AIS_ERR_LIBRARY;
		TRACE_4("cpa CkptSynchronize Api failed with return value:%d,ckptHandle:%llx", rc, checkpointHandle);
		goto lock_fail;
	}

	proc_rc = cpa_lcl_ckpt_node_get(&cb->lcl_ckpt_tree, &checkpointHandle, &lc_node);
	if (!lc_node) {
		rc = SA_AIS_ERR_BAD_HANDLE;
		TRACE_4("cpa CkptSynchronize Api failed with return value:%d,ckptHandle:%llx", rc, checkpointHandle);
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
		goto fail1;
	}
	proc_rc = cpa_client_node_get(&cb->client_tree, &lc_node->cl_hdl, &cl_node);
	if (!cl_node) {
		TRACE_4("cpa SelObjGet:client_node_get Api failed with return value:%d,cl_hdl:%llx",SA_AIS_ERR_BAD_HANDLE, lc_node->cl_hdl);
		rc = SA_AIS_ERR_BAD_HANDLE;
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
		goto fail1;
	}
	if (m_CPA_VER_IS_ABOVE_B_1_1(&cl_node->version)) {
		if ((cb->is_cpnd_joined_clm != true)  || ( cl_node->stale == true)) {
			TRACE_4("cpa CLM Node left with return value:%d", SA_AIS_ERR_UNAVAILABLE);
			rc = SA_AIS_ERR_UNAVAILABLE;
			m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
			goto clm_left;
		}
	}

	/* Don't allow the sync, in case if SA_CKPT_CHECKPOINT_WRITE is 
	   not set */
	if (!(lc_node->open_flags & SA_CKPT_CHECKPOINT_WRITE)) {
		rc = SA_AIS_ERR_ACCESS;
		TRACE_4("cpa CkptSynchronize Api failed with return value:%d,ckptHandle:%llx", rc, checkpointHandle);
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
		goto fail1;
	}

	/* Get the Global Ckpt node */
	proc_rc = cpa_gbl_ckpt_node_find_add(&cb->gbl_ckpt_tree, &lc_node->gbl_ckpt_hdl, &gc_node, &add_flag);
	if (!gc_node) {
		rc = SA_AIS_ERR_NO_RESOURCES;
		TRACE_4("cpa mem alloc failed in CkptSynchronize with return value:%d,ckptHandle:%llx", proc_rc, checkpointHandle);
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
		goto fail1;
	}

	if (!(m_IS_ASYNC_UPDATE_OPTION(&gc_node->ckpt_creat_attri))) {
		rc = SA_AIS_ERR_BAD_OPERATION;
		TRACE_4("cpa CkptSynchronize Api failed with return value:%d,ckptHandle:%llx", rc, checkpointHandle);
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
		goto fail1;
	}
	if (gc_node->is_restart) {
		rc = SA_AIS_ERR_TRY_AGAIN;
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
		TRACE_4("cpa CkptSynchronize Api failed with return value:%d,ckptHandle:%llx ", rc, checkpointHandle);
		goto fail1;
	}

	if (!gc_node->is_active_exists) {
		rc = SA_AIS_ERR_NOT_EXIST;
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
		TRACE_4("cpa CkptSynchronize Api failed with return value:%d,ckptHandle:%llx", rc, checkpointHandle);
		goto fail1;
	}

	/* Populate the event & send it to CPND */
	memset(&evt, 0, sizeof(CPSV_EVT));
	evt.type = CPSV_EVT_TYPE_CPND;
	evt.info.cpnd.type = CPND_EVT_A2ND_CKPT_SYNC;
	evt.info.cpnd.info.ckpt_sync.ckpt_id = lc_node->gbl_ckpt_hdl;
	evt.info.cpnd.info.ckpt_sync.client_hdl = lc_node->cl_hdl;

	/* Convert the time from saTimeT to millisecs */
	timeout = m_CPSV_CONVERT_SATIME_TEN_MILLI_SEC(timeout);

	if (timeout < NCS_SAF_MIN_ACCEPT_TIME) {
		rc = SA_AIS_ERR_TIMEOUT;
		TRACE_4("cpa CkptSynchronize Api failed with return value:%d,ckptHandle:%llx ", rc, checkpointHandle);
		goto fail1;
	}

	m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);

	proc_rc = cpa_mds_msg_sync_send(cb->cpa_mds_hdl, &(gc_node->active_mds_dest), &evt, &out_evt, timeout);

	/* Generate rc from proc_rc */
	switch (proc_rc) {
	case NCSCC_RC_SUCCESS:
		break;
	case NCSCC_RC_REQ_TIMOUT:
		rc = SA_AIS_ERR_TIMEOUT;
		TRACE_4("cpa CkptSynchronize:MDS Api failed with return value:%d,ckptHandle:%llx,cpnd_mds_dest:%"PRIu64,
			 proc_rc, checkpointHandle, cb->cpnd_mds_dest);
		goto fail1;
	default:
		rc = SA_AIS_ERR_TRY_AGAIN;
		TRACE_4("cpa CkptSynchronize:MDS Api failed with return value:%d,ckptHandle:%llx,cpnd_mds_dest:%"PRIu64, 
		proc_rc, checkpointHandle,cb->cpnd_mds_dest);
		goto fail1;
	}

	if (out_evt) {
		rc = out_evt->info.cpa.info.sync_rsp.error;
		m_MMGR_FREE_CPSV_EVT(out_evt, NCS_SERVICE_ID_CPA);
	} else {
		rc = SA_AIS_ERR_NO_RESOURCES;
		TRACE_4("cpa CkptSynchronize Api failed with return value:%d,ckptHandle:%llx ", rc, checkpointHandle);
	}

 fail1:
 lock_fail:
 clm_left:
	m_CPA_GIVEUP_CB;

 done:
	if (rc == SA_AIS_OK) {
		TRACE_1("cpa CkptSynchronize Api success with return value:%d,ckptHandle:%llx ", rc, checkpointHandle);
	}
	TRACE_LEAVE2("API return code = %u", rc);
	return rc;
}

/****************************************************************************
  Name          :  saCkptCheckpointSynchronizeAsync
 
  Description   :  This function ensures that all previous operations applied 
                   on the active checkpoint replica are propagated to other
                   checkpoint replicas.
 
  Arguments     :  ckptHandle - Checkpoint handle.
                   invocation - Designates a particular invocation of the 
                                response callback.
                   checkpointHandle - Handle that designates the checkpoint
 
  Return Values :  Refer to SAI-AIS specification for various return values.
 
  Notes         :
******************************************************************************/
SaAisErrorT saCkptCheckpointSynchronizeAsync(SaCkptCheckpointHandleT checkpointHandle, SaInvocationT invocation)
{
	SaAisErrorT rc = SA_AIS_OK;
	CPSV_EVT evt;
	uint32_t proc_rc = NCSCC_RC_SUCCESS;
	CPA_LOCAL_CKPT_NODE *lc_node = NULL;
	CPA_CLIENT_NODE *cl_node = NULL;
	CPA_GLOBAL_CKPT_NODE *gc_node = NULL;
	CPA_CB *cb = NULL;
	bool add_flag = false;

	/* For Ckpt Sync CPA will not perform any operation 
	   other than passing the request to CPND */

	/* retrieve CPA CB */
	TRACE_ENTER2("SaCkptCheckpointHandleT passed is %llx",checkpointHandle);
	m_CPA_RETRIEVE_CB(cb);
	if (!cb) {
		rc = SA_AIS_ERR_BAD_HANDLE;
		TRACE_4("cpa CkptSynchronizeAsync:HDL_TAKE Api failed with return value:%d,ckptHandle:%llx", rc, checkpointHandle);
		goto done;
	}

	/* get the CB Lock */
	if (m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
		rc = SA_AIS_ERR_LIBRARY;
		TRACE_4("cpa CkptSynchronizeAsync:LOCK Api failed with return value:%d,ckptHandle:%llx", rc, checkpointHandle);
		goto lock_fail;
	}

	proc_rc = cpa_lcl_ckpt_node_get(&cb->lcl_ckpt_tree, &checkpointHandle, &lc_node);
	if (!lc_node) {
		rc = SA_AIS_ERR_BAD_HANDLE;
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
		TRACE_4("cpa CkptSynchronizeAsync Api failed with return value:%d,ckptHandle:%llx", rc, checkpointHandle);
		goto fail1;
	}
	proc_rc = cpa_client_node_get(&cb->client_tree, &lc_node->cl_hdl, &cl_node);
	if (!cl_node) {
		TRACE_4("cpa SelObjGet:client_node_get Api failed with return value:%d,cl_hdl:%llx ", SA_AIS_ERR_BAD_HANDLE, lc_node->cl_hdl);
		rc = SA_AIS_ERR_BAD_HANDLE;
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
		goto fail1;
	}
	if (m_CPA_VER_IS_ABOVE_B_1_1(&cl_node->version)) {
		if ((cb->is_cpnd_joined_clm != true)  || ( cl_node->stale == true)) {
			TRACE_4("cpa CLM Node left with return value:%d",SA_AIS_ERR_UNAVAILABLE);
			rc = SA_AIS_ERR_UNAVAILABLE;
			m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
			goto clm_left;
		}
	}

	/* Don't allow the sync, in case if SA_CKPT_CHECKPOINT_WRITE is 
	   not set */
	if (!(lc_node->open_flags & SA_CKPT_CHECKPOINT_WRITE)) {
		rc = SA_AIS_ERR_ACCESS;
		TRACE_4("cpa CkptSynchronizeAsync Api failed with return value:%d,ckptHandle:%llx", rc, checkpointHandle);
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
		goto fail1;
	}

	/* Get the Global Ckpt node */
	proc_rc = cpa_gbl_ckpt_node_find_add(&cb->gbl_ckpt_tree, &lc_node->gbl_ckpt_hdl, &gc_node, &add_flag);
	if (!gc_node) {
		rc = SA_AIS_ERR_NO_MEMORY;
		TRACE_4("cpa CkptSynchronizeAsync Api failed with return value:%d,ckptHandle:%llx", proc_rc, checkpointHandle);
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
		goto fail1;
	}

	if (!(m_IS_ASYNC_UPDATE_OPTION(&gc_node->ckpt_creat_attri))) {
		rc = SA_AIS_ERR_BAD_OPERATION;
		TRACE_4("cpa CkptSynchronizeAsync Api failed with return value:%d,ckptHandle:%llx", rc, checkpointHandle);
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
		goto fail1;
	}

	/* Get the Client info */
	rc = cpa_client_node_get(&cb->client_tree, &lc_node->cl_hdl, &cl_node);
	if (!cl_node) {
		rc = SA_AIS_ERR_BAD_HANDLE;
		TRACE_4("cpa CkptSynchronizeAsync:client_node_get Api failed with return value:%d,ckptHandle:%llx", rc, checkpointHandle);
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
		goto client_not_found;
	}

	if (cl_node->ckpt_callbk.saCkptCheckpointSynchronizeCallback == NULL) {
		rc = SA_AIS_ERR_INIT;
		TRACE_4("cpa CkptSynchronizeAsync Api failed with return value:%d,ckptHandle:%llx", rc, checkpointHandle);
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
		goto no_callback;
	}
	if (gc_node->is_restart) {
		rc = SA_AIS_ERR_TRY_AGAIN;
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
		TRACE_4("cpa CkptSynchronizeAsync Api failed with return value:%d,ckptHandle:%llx", rc, checkpointHandle);
		goto fail1;
	}

	if (!gc_node->is_active_exists) {
		rc = SA_AIS_ERR_NOT_EXIST;
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
		TRACE_4("cpa CkptSynchronizeAsync Api failed with return value:%d,ckptHandle:%llx", rc, checkpointHandle);
		goto fail1;
	}

	/* Populate the event & send it to CPND */
	memset(&evt, 0, sizeof(CPSV_EVT));
	evt.type = CPSV_EVT_TYPE_CPND;
	evt.info.cpnd.type = CPND_EVT_A2ND_CKPT_SYNC;
	evt.info.cpnd.info.ckpt_sync.ckpt_id = lc_node->gbl_ckpt_hdl;
	evt.info.cpnd.info.ckpt_sync.client_hdl = lc_node->cl_hdl;
	evt.info.cpnd.info.ckpt_sync.invocation = invocation;
	evt.info.cpnd.info.ckpt_sync.lcl_ckpt_hdl = lc_node->lcl_ckpt_hdl;

	/* Create the handle to protect the CPA_TMR structure (part of lcl_ckpt_node */
	if (lc_node->async_req_tmr.uarg) {
		/* create the association with hdl-mngr */
		if (!(lc_node->async_req_tmr.uarg = ncshm_create_hdl(cb->pool_id,
								     NCS_SERVICE_ID_CPA,
								     (NCSCONTEXT)&lc_node->async_req_tmr))) {
			rc = SA_AIS_ERR_LIBRARY;
			m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
			TRACE_4("cpa CkptSynchronizeAsync Api failed with return value:%d,ckptHandle:%llx", rc, checkpointHandle);
			goto hm_create_fail;
		}
	}

	/* Start the timer with value timeout */
	lc_node->async_req_tmr.type = CPA_TMR_TYPE_SYNC;
	lc_node->async_req_tmr.info.ckpt.invocation = invocation;
	lc_node->async_req_tmr.info.ckpt.client_hdl = lc_node->cl_hdl;
	lc_node->async_req_tmr.info.ckpt.lcl_ckpt_hdl = lc_node->lcl_ckpt_hdl;

	cpa_tmr_start(&lc_node->async_req_tmr, CPA_SYNC_ASYNC_WAIT_TIME);

	/* Unlock before MDS Send */
	m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
	proc_rc = cpa_mds_msg_send(cb->cpa_mds_hdl, &gc_node->active_mds_dest, &evt, NCSMDS_SVC_ID_CPND);

	/* Generate rc from proc_rc */
	switch (proc_rc) {
	case NCSCC_RC_SUCCESS:
		break;
	case NCSCC_RC_REQ_TIMOUT:
		rc = SA_AIS_ERR_TIMEOUT;
		TRACE_4("cpa CkptSynschronizeAsync:MDS Api failed with return value:%d,ckptHandle:%llx,cpnd_mds_dest:%"PRIu64, proc_rc, checkpointHandle,
				 cb->cpnd_mds_dest);
		goto fail1;
	default:
		rc = SA_AIS_ERR_NO_RESOURCES;
		TRACE_4("cpa CkptSynchronizeAsync:MDS Api failed with return value:%d,ckptHandle:%llx,cpnd_mds_dest:%"PRIu64, proc_rc, checkpointHandle,
				 cb->cpnd_mds_dest);
		goto fail1;
	}

 fail1:
 no_callback:
 hm_create_fail:
 client_not_found:
 lock_fail:
 clm_left:
	m_CPA_GIVEUP_CB;

 done:
	if (SA_AIS_OK == rc) {
		TRACE_1("cpa  CkptSynchronizeAsync Api success with return value:%d,ckptHandle:%llx", rc, checkpointHandle);
	}

	TRACE_LEAVE2("API return code = %u", rc);
	return rc;
}

/****************************************************************************
  Name          :  ncsCkptRegisterCkptArrivalCallback
 
  Description   :  This function registers the Ckpt arrival call back
 
  Arguments     :  ckptHandle - Checkpoint handle.
                   ckptArrivalCallback - Pointer to arrival callback function.
 
  Return Values :  Refer to SAI-AIS specification for various return values.

  Notes         :
******************************************************************************/
SaAisErrorT
ncsCkptRegisterCkptArrivalCallback(SaCkptHandleT ckptHandle, ncsCkptCkptArrivalCallbackT ckptArrivalCallback)
{
	SaAisErrorT rc = SA_AIS_OK;
	CPSV_EVT evt;
	uint32_t proc_rc = NCSCC_RC_SUCCESS;
	CPA_CLIENT_NODE *cl_node = NULL;
	CPA_CB *cb = NULL;
	bool is_locked = false;

	/* retrieve CPA CB */
	m_CPA_RETRIEVE_CB(cb);
	if (!cb) {
		rc = SA_AIS_ERR_BAD_HANDLE;
		TRACE_4("cpa CkptArrivalCallback Api failed with return value:%d,ckptHandle:%llx",rc, ckptHandle);
		goto done;
	}

	/* get the CB Lock */
	if (m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
		rc = SA_AIS_ERR_LIBRARY;
		TRACE_4("cpa CkptArrivalCallback:LOCK Api failed with return value:%d,ckptHandle:%llx", rc, ckptHandle);
		goto lock_fail;
	}

	is_locked = true;

	/* Get the Client info */
	rc = cpa_client_node_get(&cb->client_tree, &ckptHandle, &cl_node);

	if (!cl_node) {
		rc = SA_AIS_ERR_BAD_HANDLE;
		TRACE_4("cpa CkptArrivalCallback:client_node_get Api failed with return value:%d,ckptHandle:%llx", rc, ckptHandle);
		goto proc_fail;
	}

	if (ckptArrivalCallback) {
		cl_node->ckptArrivalCallback = ckptArrivalCallback;
		if (cl_node->callbk_mbx == 0)
			proc_rc = cpa_callback_ipc_init(cl_node);
		else
			proc_rc = NCSCC_RC_SUCCESS;
	} else {
		rc = SA_AIS_ERR_INVALID_PARAM;
		TRACE_4("cpa CkptArrivalCallback Api failed with return value:%d,ckptHandle:%llx", rc, ckptHandle);
		goto proc_fail;

	}

	if (proc_rc != NCSCC_RC_SUCCESS) {
		/* Error handling */
		rc = SA_AIS_ERR_LIBRARY;
		TRACE_4("cpa CkptArrivalCallback Api failed with return value:%d,ckptHandle:%llx", rc, ckptHandle);
		goto proc_fail;
	}

	/* Send it to CPND */
	memset(&evt, 0, sizeof(CPSV_EVT));
	evt.type = CPSV_EVT_TYPE_CPND;
	evt.info.cpnd.type = CPND_EVT_A2ND_ARRIVAL_CB_REG;
	evt.info.cpnd.info.arr_ntfy.client_hdl = ckptHandle;

	m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
	is_locked = false;

	/* IF CPND IS DOWN  */
	if (false == cb->is_cpnd_up) {
		rc = SA_AIS_ERR_TRY_AGAIN;
		TRACE_4("cpa CkptArrivalCallback Api failed with return value:%d,ckptHandle:%llx", rc, ckptHandle);
		goto lock_fail;
	}
	proc_rc = cpa_mds_msg_send(cb->cpa_mds_hdl, &cb->cpnd_mds_dest, &evt, NCSMDS_SVC_ID_CPND);

	/* Generate rc from proc_rc */
	switch (proc_rc) {
	case NCSCC_RC_SUCCESS:
		break;
	case NCSCC_RC_REQ_TIMOUT:
		rc = SA_AIS_ERR_TIMEOUT;
		TRACE_4("cpa CkptArrivalCallback:MDS Api failed with return value:%d,ckptHandle:%llx,cpnd_mds_dest:%"PRIu64, proc_rc, ckptHandle, cb->cpnd_mds_dest);
		goto proc_fail;
	default:
		rc = SA_AIS_ERR_NO_RESOURCES;
		TRACE_4("cpa CkptArrivalCallback:MDS Api failed with return value:%d,ckptHandle:%llx,cpnd_mds_dest:%"PRIu64, proc_rc, ckptHandle, cb->cpnd_mds_dest);
		goto proc_fail;
	}

 proc_fail:
	if (is_locked)
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
 lock_fail:
	m_CPA_GIVEUP_CB;

 done:
	TRACE_LEAVE2("API return code = %u", rc);
	return rc;
}
