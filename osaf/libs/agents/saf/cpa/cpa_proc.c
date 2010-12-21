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
  
  This file contains the CPA processing routines callback
  processing routines etc.
    
******************************************************************************
*/

#include "cpa.h"
static void cpa_process_callback_info(CPA_CB *cb, CPA_CLIENT_NODE *cl_node, CPA_CALLBACK_INFO *callback);

#define m_MMGR_FREE_CPSV_DEFAULT(p) m_NCS_MEM_FREE(p, \
           NCS_MEM_REGION_PERSISTENT, NCS_SERVICE_ID_OS_SVCS, \
           0)

/****************************************************************************
  Name          : cpa_version_validate
 
  Description   : This routine Validates the received version
 
  Arguments     : SaVersionT *version - Version Info
 
  Return Values : SA_AIS_OK/SA_AIS<ERROR>
 
  Notes         : None
******************************************************************************/
uns32 cpa_version_validate(SaVersionT *version)
{
	if (version->releaseCode == CPA_RELEASE_CODE &&
	    (version->majorVersion == CPA_MAJOR_VERSION || version->majorVersion == CPA_BASE_MAJOR_VERSION)) {
		version->releaseCode = CPA_RELEASE_CODE;
		version->majorVersion = CPA_MAJOR_VERSION;
		version->minorVersion = CPA_MINOR_VERSION;
		return SA_AIS_OK;
	} else {
		m_LOG_CPA_HEADLINE(CPA_VERSION_INCOMPATIBLE, NCSFL_SEV_ERROR);

		/* Implimentation is supporting the required release code */
		if (CPA_RELEASE_CODE > version->releaseCode) {
			version->releaseCode = CPA_RELEASE_CODE;
		} else if (CPA_RELEASE_CODE < version->releaseCode) {
			version->releaseCode = CPA_RELEASE_CODE;
		}
		version->majorVersion = CPA_MAJOR_VERSION;
		version->minorVersion = CPA_MINOR_VERSION;

		return SA_AIS_ERR_VERSION;
	}

	return SA_AIS_OK;
}

/****************************************************************************
  Name          : cpa_open_attr_validate
 
  Description   : This routine Validates the Received Version
 
  Arguments     : SaVersionT *version - Version Info
 
  Return Values : SA_AIS_OK/SA_AIS<ERROR>
 
  Notes         : None
******************************************************************************/
uns32 cpa_open_attr_validate(const SaCkptCheckpointCreationAttributesT
			     *checkpointCreationAttributes,
			     SaCkptCheckpointOpenFlagsT checkpointOpenFlags, const SaNameT *checkpointName)
{
	SaCkptCheckpointCreationFlagsT creationFlags = 0;
	/* Check the Open Flags is set, it should  */

	if (!(checkpointOpenFlags & (SA_CKPT_CHECKPOINT_READ | SA_CKPT_CHECKPOINT_WRITE | SA_CKPT_CHECKPOINT_CREATE))) {
		m_LOG_CPA_CCLL(CPA_PROC_FAILED, NCSFL_LC_CKPT_MGMT, NCSFL_SEV_ERROR,
			       "open_attr_validate", __FILE__, __LINE__, SA_AIS_ERR_BAD_FLAGS);
		return SA_AIS_ERR_BAD_FLAGS;
	}

	if (checkpointCreationAttributes) {
		creationFlags = checkpointCreationAttributes->creationFlags;

		if (creationFlags == 0 || creationFlags == SA_CKPT_CHECKPOINT_COLLOCATED
		    || (creationFlags > (SA_CKPT_CHECKPOINT_COLLOCATED | SA_CKPT_WR_ACTIVE_REPLICA_WEAK))) {
			m_LOG_CPA_CCLL(CPA_PROC_FAILED, NCSFL_LC_CKPT_MGMT, NCSFL_SEV_ERROR, "open_attr_validate",
				       __FILE__, __LINE__, SA_AIS_ERR_INVALID_PARAM);
			return SA_AIS_ERR_INVALID_PARAM;
		}

		/* Checking for mutual exclusive ness of flags SA_CKPT_WR_ALL_REPLICAS, 
		   SA_CKPT_WR_ACTIVE_REPLICA, SA_CKPT_WR_ACTIVE_REPLICA_WEAK */
		if (((creationFlags & SA_CKPT_WR_ALL_REPLICAS) && (creationFlags & SA_CKPT_WR_ACTIVE_REPLICA))
		    || ((creationFlags & SA_CKPT_WR_ALL_REPLICAS) && (creationFlags & SA_CKPT_WR_ACTIVE_REPLICA_WEAK))
		    || ((creationFlags & SA_CKPT_WR_ACTIVE_REPLICA) && (creationFlags & SA_CKPT_WR_ACTIVE_REPLICA_WEAK))
		    ) {
			m_LOG_CPA_CCLL(CPA_PROC_FAILED, NCSFL_LC_CKPT_MGMT, NCSFL_SEV_ERROR,
				       "open_attr_validate", __FILE__, __LINE__, SA_AIS_ERR_INVALID_PARAM);
			return SA_AIS_ERR_INVALID_PARAM;
		}
	}

	/* Validations on checkpointopenflags */
	if (checkpointOpenFlags & SA_CKPT_CHECKPOINT_CREATE) {
		if (checkpointCreationAttributes == NULL) {
			m_LOG_CPA_CCLL(CPA_PROC_FAILED, NCSFL_LC_CKPT_MGMT, NCSFL_SEV_ERROR,
				       "open_attr_validate", __FILE__, __LINE__, SA_AIS_ERR_INVALID_PARAM);
			return SA_AIS_ERR_INVALID_PARAM;
		} else if (checkpointCreationAttributes->checkpointSize >
			   (checkpointCreationAttributes->maxSections * checkpointCreationAttributes->maxSectionSize)) {
			m_LOG_CPA_CCLL(CPA_PROC_FAILED, NCSFL_LC_CKPT_MGMT, NCSFL_SEV_ERROR,
				       "open_attr_validate", __FILE__, __LINE__, SA_AIS_ERR_INVALID_PARAM);
			return SA_AIS_ERR_INVALID_PARAM;
		}
	} else {
		if (checkpointCreationAttributes != NULL) {
			m_LOG_CPA_CCLL(CPA_PROC_FAILED, NCSFL_LC_CKPT_MGMT, NCSFL_SEV_ERROR,
				       "open_attr_validate", __FILE__, __LINE__, SA_AIS_ERR_INVALID_PARAM);
			return SA_AIS_ERR_INVALID_PARAM;
		}
	}

	return SA_AIS_OK;
}

/****************************************************************************
  Name          : cpa_callback_ipc_init
  
  Description   : This routine is used to initialize the queue for the callbacks.
 
  Arguments     : client_info - pointer to the client info
                   
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None
******************************************************************************/
uns32 cpa_callback_ipc_init(CPA_CLIENT_NODE *client_info)
{
	uns32 rc = NCSCC_RC_SUCCESS;
	if ((rc = m_NCS_IPC_CREATE(&client_info->callbk_mbx)) == NCSCC_RC_SUCCESS) {
		if (m_NCS_IPC_ATTACH(&client_info->callbk_mbx) == NCSCC_RC_SUCCESS) {
			return NCSCC_RC_SUCCESS;
		}
		m_NCS_IPC_RELEASE(&client_info->callbk_mbx, NULL);
		m_LOG_CPA_CCLL(CPA_API_FAILED, NCSFL_LC_CKPT_MGMT, NCSFL_SEV_ERROR, "IPC_CREATE", __FILE__, __LINE__,
			       rc);
	}
	return rc;
}

/****************************************************************************
  Name          : cpa_client_cleanup_mbx
  
  Description   : This routine is used to destroy the queue for the callbacks.
 
  Arguments     : cl_node - pointer to the client info
                   
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None
******************************************************************************/
static NCS_BOOL cpa_client_cleanup_mbx(NCSCONTEXT arg, NCSCONTEXT msg)
{
	CPA_CALLBACK_INFO *callback, *pnext;

	pnext = callback = (CPA_CALLBACK_INFO *)msg;

	while (pnext) {
		pnext = callback->next;
		m_MMGR_FREE_CPA_CALLBACK_INFO(callback);
		callback = pnext;
	}

	return TRUE;

}

/****************************************************************************
  Name          : cpa_callback_ipc_destroy
  
  Description   : This routine is used to initialize the queue for the callbacks.
 
  Arguments     : client_info - pointer to the client info
                   
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None
******************************************************************************/
void cpa_callback_ipc_destroy(CPA_CLIENT_NODE *cl_node)
{
	/* detach the mail box */
	m_NCS_IPC_DETACH(&cl_node->callbk_mbx, cpa_client_cleanup_mbx, cl_node);

	/* delete the mailbox */
	m_NCS_IPC_RELEASE(&cl_node->callbk_mbx, NULL);
}

/****************************************************************************
  Name          : cpa_ckpt_finalize_proc
  
  Description   : This routine is used to process the finalize request at CPA.
 
  Arguments     : cb - CPA CB.
                  cl_node - pointer to the client info
                   
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None
******************************************************************************/
uns32 cpa_ckpt_finalize_proc(CPA_CB *cb, CPA_CLIENT_NODE *cl_node)
{
	SaCkptHandleT temp_hdl, *temp_ptr = NULL;
	CPA_LOCAL_CKPT_NODE *lc_node = NULL;
	CPA_GLOBAL_CKPT_NODE *gc_node = NULL;
	NCS_BOOL add_flag = FALSE;
	CPA_SECT_ITER_NODE *sect_iter_node;

	temp_ptr = 0;

	/* Scan the entire checkpoint DB and close the checkpoints opened by this client */
	while ((lc_node = (CPA_LOCAL_CKPT_NODE *)
		ncs_patricia_tree_getnext(&cb->lcl_ckpt_tree, (uns8 *)temp_ptr))) {
		temp_hdl = lc_node->lcl_ckpt_hdl;
		temp_ptr = &temp_hdl;

		if (lc_node->cl_hdl == cl_node->cl_hdl) {
			cpa_gbl_ckpt_node_find_add(&cb->gbl_ckpt_tree, &lc_node->gbl_ckpt_hdl, &gc_node, &add_flag);
			if (gc_node) {
				gc_node->ref_cnt--;
				if (gc_node->ref_cnt == 0) {
					cpa_gbl_ckpt_node_delete(cb, gc_node);
				}

				if (lc_node->sect_iter_cnt) {
					sect_iter_node = 0;
					cpa_sect_iter_node_getnext(&cb->sect_iter_tree, 0, &sect_iter_node);
					while (sect_iter_node != NULL) {
						if (lc_node->lcl_ckpt_hdl == sect_iter_node->lcl_ckpt_hdl) {
							cpa_sect_iter_node_delete(cb, sect_iter_node);
							cpa_sect_iter_node_getnext(&cb->sect_iter_tree, 0,
										   &sect_iter_node);
						} else
							cpa_sect_iter_node_getnext(&cb->sect_iter_tree,
										   &sect_iter_node->iter_id,
										   &sect_iter_node);
					}
				}

			}
			cpa_lcl_ckpt_node_delete(cb, lc_node);
		}
	}

	cpa_callback_ipc_destroy(cl_node);

	cpa_client_node_delete(cb, cl_node);

	return NCSCC_RC_SUCCESS;

}

/****************************************************************************
  Name          : cpa_proc_async_open_rsp
  Description   : This function will process the async response to Open req.
  Arguments     : cb - CPA CB.
                  evt - CPA_EVT.
  Return Values : None
  Notes         : None
******************************************************************************/
static void cpa_proc_async_open_rsp(CPA_CB *cb, CPA_EVT *evt)
{
	uns32 proc_rc = NCSCC_RC_SUCCESS;
	SaAisErrorT rc = SA_AIS_OK;
	NCS_BOOL add_flag = TRUE;
	CPA_CALLBACK_INFO *callback;
	CPA_LOCAL_CKPT_NODE *lc_node = NULL;
	CPA_GLOBAL_CKPT_NODE *gc_node = NULL;
	CPA_CLIENT_NODE *cl_node = NULL;


	/* get the CB Lock */
	if (m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
		m_LOG_CPA_CCLL(CPA_PROC_FAILED, NCSFL_LC_CKPT_MGMT, NCSFL_SEV_ERROR,
			       "async_open_rsp:LOCK", __FILE__, __LINE__, evt->info.openRsp.error);
		return;
	}

	/* Get the local Ckpt Node */
	cpa_lcl_ckpt_node_get(&cb->lcl_ckpt_tree, &evt->info.openRsp.lcl_ckpt_hdl, &lc_node);
	if (lc_node == NULL) {
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
		m_LOG_CPA_CCLLFF(CPA_PROC_FAILED, NCSFL_LC_CKPT_MGMT, NCSFL_SEV_ERROR,
				 "async_open_rsp", __FILE__, __LINE__, evt->info.openRsp.error,
				 evt->info.openRsp.lcl_ckpt_hdl, evt->info.openRsp.gbl_ckpt_hdl);
		return;
	}

	/* Stop the timer */
	cpa_tmr_stop(&lc_node->async_req_tmr);

	/* Get the Client info */
	rc = cpa_client_node_get(&cb->client_tree, &lc_node->cl_hdl, &cl_node);
	if (!cl_node) {
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
		m_LOG_CPA_CCLLFFF(CPA_PROC_FAILED, NCSFL_LC_CKPT_MGMT, NCSFL_SEV_ERROR,
				  "async_open_rsp", __FILE__, __LINE__, evt->info.openRsp.error,
				  lc_node->cl_hdl, evt->info.openRsp.lcl_ckpt_hdl,
				  evt->info.openRsp.gbl_ckpt_hdl);
		return;
	}

	if (evt->info.openRsp.error == SA_AIS_OK) {
		SaSizeT ckpt_size = 0;

		/* We got all the data that we want, update it */
		lc_node->gbl_ckpt_hdl = evt->info.openRsp.gbl_ckpt_hdl;

		/* Update the info in the gbl_ckpt_tree */
		proc_rc = cpa_gbl_ckpt_node_find_add(&cb->gbl_ckpt_tree, &lc_node->gbl_ckpt_hdl, &gc_node, &add_flag);

		if (proc_rc == NCSCC_RC_OUT_OF_MEM) {
			rc = SA_AIS_ERR_NO_MEMORY;
			m_LOG_CPA_CCLLFFF(CPA_PROC_FAILED, NCSFL_LC_CKPT_MGMT, NCSFL_SEV_ERROR,
					  "async_open_rsp", __FILE__, __LINE__, rc, lc_node->cl_hdl,
					  evt->info.openRsp.lcl_ckpt_hdl, lc_node->gbl_ckpt_hdl);
			goto send_cb_evt;

		} else if (proc_rc != NCSCC_RC_SUCCESS) {
			rc = SA_AIS_ERR_NO_RESOURCES;
			m_LOG_CPA_CCLLFFF(CPA_PROC_FAILED, NCSFL_LC_CKPT_MGMT, NCSFL_SEV_ERROR,
					  "async_open_rsp", __FILE__, __LINE__, rc, lc_node->cl_hdl,
					  evt->info.openRsp.lcl_ckpt_hdl, lc_node->gbl_ckpt_hdl);
			goto send_cb_evt;
		}

		if (add_flag == FALSE) {
			gc_node->ref_cnt++;
			gc_node->ckpt_creat_attri = evt->info.openRsp.creation_attr;
			/*To store the active MDS_DEST info of checkpoint */
			if (evt->info.openRsp.is_active_exists) {
				gc_node->is_active_exists = TRUE;
				gc_node->active_mds_dest = evt->info.openRsp.active_dest;
			}

			ckpt_size = sizeof(CPSV_CKPT_HDR) + (gc_node->ckpt_creat_attri.maxSections *
							     (sizeof(CPSV_SECT_HDR) +
							      gc_node->ckpt_creat_attri.maxSectionSize));
		}
	}

 send_cb_evt:
	/* Allocate the Callback info */
	callback = m_MMGR_ALLOC_CPA_CALLBACK_INFO;
	if (!callback) {
		proc_rc = NCSCC_RC_OUT_OF_MEM;
		m_LOG_CPA_CCLLFFF(CPA_PROC_FAILED, NCSFL_LC_CKPT_MGMT, NCSFL_SEV_ERROR,
				  "async_open_rsp", __FILE__, __LINE__, rc, lc_node->cl_hdl,
				  evt->info.openRsp.lcl_ckpt_hdl, lc_node->gbl_ckpt_hdl);
		goto done;
	}

	if (callback) {
		/* Fill the Call Back Info */
		memset(callback, 0, sizeof(CPA_CALLBACK_INFO));
		callback->type = CPA_CALLBACK_TYPE_OPEN;
		callback->lcl_ckpt_hdl = evt->info.openRsp.lcl_ckpt_hdl;
		callback->invocation = evt->info.openRsp.invocation;
		if (proc_rc != NCSCC_RC_SUCCESS)
			callback->sa_err = rc;
		else
			callback->sa_err = evt->info.openRsp.error;

		/* Send the event */
		proc_rc = m_NCS_IPC_SEND(&cl_node->callbk_mbx, callback, NCS_IPC_PRIORITY_NORMAL);

		m_LOG_CPA_CCLLFFF(CPA_PROC_SUCCESS, NCSFL_LC_CKPT_MGMT, NCSFL_SEV_INFO,
				  "async_open_rsp", __FILE__, __LINE__, proc_rc, lc_node->cl_hdl,
				  evt->info.openRsp.lcl_ckpt_hdl, lc_node->gbl_ckpt_hdl);
	}
 done:

	/* Release The Lock */
	m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);

	/* Close the checkpoint in case of failure */
	if (proc_rc != NCSCC_RC_SUCCESS)
		saCkptCheckpointClose(callback->lcl_ckpt_hdl);
}

/****************************************************************************
  Name          : cpa_proc_async_sync_rsp
  Description   : This function will process the async response to sync req.
  Arguments     : cb - CPA CB.
                  evt - CPA_EVT.
  Return Values : None
  Notes         : None
******************************************************************************/
static void cpa_proc_async_sync_rsp(CPA_CB *cb, CPA_EVT *evt)
{
	uns32 proc_rc = NCSCC_RC_SUCCESS;
	CPA_CALLBACK_INFO *callback = NULL;
	CPA_CLIENT_NODE *cl_node = NULL;
	CPA_LOCAL_CKPT_NODE *lc_node = NULL;

	/* get the CB Lock */
	if (m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
		m_LOG_CPA_HEADLINE(CPA_CB_LOCK_TAKE_FAILED, NCSFL_SEV_ERROR);
		return;
	}

	/* Get the Client info */
	proc_rc = cpa_client_node_get(&cb->client_tree, &evt->info.sync_rsp.client_hdl, &cl_node);
	if (!cl_node) {
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
		return;
	}

	/* Get the local Ckpt Node */
	cpa_lcl_ckpt_node_get(&cb->lcl_ckpt_tree, &evt->info.sync_rsp.lcl_ckpt_hdl, &lc_node);
	if (lc_node == NULL) {
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
		return;
	}

	/* Stop the timer */
	cpa_tmr_stop(&lc_node->async_req_tmr);

	/* Allocate the Callback info */
	callback = m_MMGR_ALLOC_CPA_CALLBACK_INFO;
	if (!callback) {
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
		return;
	}
	/* Fill the Call Back Info */
	memset(callback, 0, sizeof(CPA_CALLBACK_INFO));
	callback->type = CPA_CALLBACK_TYPE_SYNC;
	callback->invocation = evt->info.sync_rsp.invocation;
	callback->sa_err = evt->info.sync_rsp.error;
	callback->lcl_ckpt_hdl = evt->info.sync_rsp.lcl_ckpt_hdl;

	/* Send the event */
	m_NCS_IPC_SEND(&cl_node->callbk_mbx, callback, NCS_IPC_PRIORITY_NORMAL);

	/* Release The Lock */
	m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
	return;
}

/***************************************************************************************************
* Name         : cpa_proc_ckpt_arrival_ntfy
                                                                                                                             
* Description  :
                                                                                                                             
***************************************************************************************************/
static void cpa_proc_ckpt_arrival_ntfy(CPA_CB *cb, CPA_EVT *evt)
{
	uns32 proc_rc = NCSCC_RC_SUCCESS, i = 0;
	CPA_CALLBACK_INFO *callback = NULL;
	CPA_CLIENT_NODE *cl_node = NULL;
	CPA_LOCAL_CKPT_NODE *lc_node = NULL;
	CPSV_CKPT_DATA *ckpt_data = NULL;
	SaCkptHandleT prev_ckpt_hdl;

	/* get the CB Lock */

	if (m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
		m_LOG_CPA_HEADLINE(CPA_CB_LOCK_TAKE_FAILED, NCSFL_SEV_ERROR);
		return;
	}

	/* Get the Client info */
	proc_rc = cpa_client_node_get(&cb->client_tree, &evt->info.arr_msg.client_hdl, &cl_node);
	if (!cl_node) {
		goto free_mem;
	}

	cpa_lcl_ckpt_node_getnext(cb, 0, &lc_node);

	while (lc_node) {
		/* check if the ckpt is opened with Read Flag */
		if (lc_node->open_flags & SA_CKPT_CHECKPOINT_READ) {
			/* The local writer should not get the callback */
			if (!((evt->info.arr_msg.mdest == cb->cpa_mds_dest) &&
			      (evt->info.arr_msg.lcl_ckpt_hdl == lc_node->lcl_ckpt_hdl))) {
				/* To get to the correct client with the correct ckpt */
				if ((evt->info.arr_msg.ckpt_hdl == lc_node->gbl_ckpt_hdl) &&
				    (evt->info.arr_msg.client_hdl == lc_node->cl_hdl)) {

					/* Allocate the Callback info */
					callback = m_MMGR_ALLOC_CPA_CALLBACK_INFO;
					if (!callback) {
						/* Log TBD */
						goto free_mem;
					}
					memset(callback, 0, sizeof(CPA_CALLBACK_INFO));

					callback->type = CPA_CALLBACK_TYPE_ARRIVAL_NTFY;
					callback->lcl_ckpt_hdl = lc_node->lcl_ckpt_hdl;
					callback->num_of_elmts = evt->info.arr_msg.num_of_elmts;
					if (evt->info.arr_msg.num_of_elmts) {
						callback->ioVector =
						    m_MMGR_ALLOC_SaCkptIOVectorElementT(evt->info.arr_msg.num_of_elmts);
					}
					if (callback->ioVector == NULL) {
						/* Log TBD */
						/* Free Call Back */
						m_MMGR_FREE_CPA_CALLBACK_INFO(callback);
						goto free_mem;
					}
					ckpt_data = evt->info.arr_msg.ckpt_data;
					for (i = 0; i < evt->info.arr_msg.num_of_elmts; i++) {

						/* check for Default section */
						if ((ckpt_data->sec_id.id != NULL) && (ckpt_data->sec_id.idLen != 0)) {
							callback->ioVector[i].sectionId.id =
							    m_MMGR_ALLOC_CPSV_DEFAULT_VAL(ckpt_data->sec_id.idLen,
											  NCS_SERVICE_ID_CPA);
							memcpy(callback->ioVector[i].sectionId.id, ckpt_data->sec_id.id,
							       ckpt_data->sec_id.idLen);
							callback->ioVector[i].sectionId.idLen = ckpt_data->sec_id.idLen;
						} else
							callback->ioVector[i].sectionId = ckpt_data->sec_id;

						callback->ioVector[i].dataBuffer = NULL;
						/*   callback->ioVector[i].dataBuffer = ckpt_data->data; */
						callback->ioVector[i].dataSize = ckpt_data->dataSize;
						callback->ioVector[i].dataOffset = ckpt_data->dataOffset;
						ckpt_data = ckpt_data->next;

					}
					/* IPC Send */
					m_NCS_IPC_SEND(&cl_node->callbk_mbx, callback, NCS_IPC_PRIORITY_NORMAL);

				}
			}
		}

		prev_ckpt_hdl = lc_node->lcl_ckpt_hdl;
		cpa_lcl_ckpt_node_getnext(cb, &prev_ckpt_hdl, &lc_node);
	}			/* End of While */

 free_mem:
	m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);

	ckpt_data = evt->info.arr_msg.ckpt_data;
	/* Allocated by EDU should be freed */
	cpa_proc_free_arrival_ntfy_cpsv_ckpt_data(ckpt_data);

}

static uns32 cpa_proc_ckpt_clm_status_changed(CPA_CB *cb, CPSV_EVT *evt)
{
	NCS_BOOL locked = TRUE;
	if (m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
		m_LOG_CPA_CCL(CPA_API_FAILED, NCSFL_LC_CKPT_MGMT, NCSFL_SEV_ERROR, "Clm_status_changed:LOCK", __FILE__,
			      __LINE__);
		return NCSCC_RC_FAILURE;
	}
	if (evt->info.cpa.type == CPA_EVT_ND2A_CKPT_CLM_NODE_LEFT) {
		cb->is_cpnd_joined_clm = FALSE;
		cpa_client_tree_mark_stale(cb);
	} else if (evt->info.cpa.type == CPA_EVT_ND2A_CKPT_CLM_NODE_JOINED) {
		cb->is_cpnd_joined_clm = TRUE;
	}
	if (locked)
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
  Name          : cpa_proc_tmr_expiry
  Description   : This function will process the Timer expiry info.
  Arguments     : cb - CPA CB.
                  evt - CPA_EVT.
  Return Values : None
  Notes         : None
******************************************************************************/
static void cpa_proc_tmr_expiry(CPA_CB *cb, CPA_EVT *evt)
{
	uns32 proc_rc = NCSCC_RC_SUCCESS;
	CPA_TMR_INFO *tmr_info = &evt->info.tmr_info;
	CPA_CALLBACK_INFO *callback = NULL;
	CPA_CLIENT_NODE *cl_node = NULL;
	CPA_LOCAL_CKPT_NODE *lc_node = NULL;

	/* get the CB Lock */
	if (m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
		m_LOG_CPA_HEADLINE(CPA_CB_LOCK_TAKE_FAILED, NCSFL_SEV_ERROR);
		return;
	}

	if ((evt->info.tmr_info.type == CPA_TMR_TYPE_OPEN) || (evt->info.tmr_info.type == CPA_TMR_TYPE_SYNC)) {
		/* Get the local Ckpt Node */
		cpa_lcl_ckpt_node_get(&cb->lcl_ckpt_tree, &tmr_info->lcl_ckpt_hdl, &lc_node);
		if (lc_node == NULL)
			goto free_lock;

		/* Stop the timer */
		cpa_tmr_stop(&lc_node->async_req_tmr);

		/* Get the Client info */
		proc_rc = cpa_client_node_get(&cb->client_tree, &tmr_info->client_hdl, &cl_node);
		if (!cl_node)
			goto free_lock;

		/* Allocate the Callback info */
		callback = m_MMGR_ALLOC_CPA_CALLBACK_INFO;
		if (!callback) {
			goto free_lock;
		}
		/* Fill the Call Back Info */
		memset(callback, 0, sizeof(CPA_CALLBACK_INFO));
		if (evt->info.tmr_info.type == CPA_TMR_TYPE_OPEN)
			callback->type = CPA_CALLBACK_TYPE_OPEN;
		else if (evt->info.tmr_info.type == CPA_TMR_TYPE_SYNC)
			callback->type = CPA_CALLBACK_TYPE_SYNC;

		callback->invocation = tmr_info->invocation;
		callback->lcl_ckpt_hdl = tmr_info->lcl_ckpt_hdl;
		callback->sa_err = SA_AIS_ERR_TIMEOUT;

		/* Send the event */
		m_NCS_IPC_SEND(&cl_node->callbk_mbx, callback, NCS_IPC_PRIORITY_NORMAL);
	}

 free_lock:
	/* Release The Lock */
	m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
	return;

}

/****************************************************************************
  Name          : cpa_proc_active_ckpt_info_bcast
  Description   : This function will process the bcast from  cpd.
  Arguments     : cb - CPA CB.
                  evt - CPA_EVT.
  Return Values : None
  Notes         : None
******************************************************************************/
static void cpa_proc_active_ckpt_info_bcast(CPA_CB *cb, CPA_EVT *evt)
{
	CPA_GLOBAL_CKPT_NODE *gc_node = NULL;
	NCS_BOOL add_flag = FALSE;
	uns32 proc_rc = NCSCC_RC_SUCCESS;

	/* Get the Global Ckpt node */
	proc_rc = cpa_gbl_ckpt_node_find_add(&cb->gbl_ckpt_tree, &evt->info.ackpt_info.ckpt_id, &gc_node, &add_flag);

	if (proc_rc != NCSCC_RC_SUCCESS) {
		m_LOG_CPA_CCLLFF(CPA_API_FAILED, NCSFL_LC_CKPT_MGMT, NCSFL_SEV_ERROR,
				 "active_ckpt_info_bcast", __FILE__, __LINE__, proc_rc, evt->info.ackpt_info.ckpt_id,
				 0);
	}

	if (gc_node) {
		m_NCS_LOCK(&gc_node->cpd_active_sync_lock, NCS_LOCK_WRITE);
		gc_node->active_mds_dest = evt->info.ackpt_info.mds_dest;
		if (evt->info.ackpt_info.mds_dest) {
			gc_node->is_active_exists = TRUE;
			m_LOG_CPA_CCLLFF(CPA_API_SUCCESS, NCSFL_LC_CKPT_MGMT, NCSFL_SEV_NOTICE,
					 "active_ckpt_info_bcast", __FILE__, __LINE__, gc_node->is_active_exists,
					 evt->info.ackpt_info.ckpt_id, gc_node->active_mds_dest);
		} else {
			gc_node->is_active_exists = FALSE;
			m_LOG_CPA_CCLLFF(CPA_API_FAILED, NCSFL_LC_CKPT_MGMT, NCSFL_SEV_NOTICE,
					 "active_ckpt_info_bcast", __FILE__, __LINE__, gc_node->is_active_exists,
					 evt->info.ackpt_info.ckpt_id, gc_node->active_mds_dest);
		}
		gc_node->is_restart = FALSE;
		/*Indicate the Waiting CPA API therad & wake up */
		gc_node->is_active_bcast_came = TRUE;
		if (gc_node->cpd_active_sync_awaited) {
			m_NCS_SEL_OBJ_IND(gc_node->cpd_active_sync_sel);
		}
		m_NCS_UNLOCK(&gc_node->cpd_active_sync_lock, NCS_LOCK_WRITE);
	}
}

/****************************************************************************
  Name          : cpa_proc_active_nd_down_bcast 
  Description   : This function will process the bcast from  cpd.
  Arguments     : cb - CPA CB.
                  evt - CPA_EVT.
  Return Values : None
  Notes         : None
******************************************************************************/
static void cpa_proc_active_nd_down_bcast(CPA_CB *cb, CPA_EVT *evt)
{
	CPA_GLOBAL_CKPT_NODE *gc_node = NULL;
	NCS_BOOL add_flag = FALSE;

	/* Get the Global Ckpt node */
	cpa_gbl_ckpt_node_find_add(&cb->gbl_ckpt_tree, &evt->info.ackpt_info.ckpt_id, &gc_node, &add_flag);

	if (gc_node)
		gc_node->is_restart = TRUE;
}

/****************************************************************************
  Name          : cpa_process_evt
  Description   : This routine will process the callback event received from
                  CPND.
  Arguments     : cb - CPA CB.
                  evt - CPSV_EVT.
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
  Notes         : None
******************************************************************************/
uns32 cpa_process_evt(CPA_CB *cb, CPSV_EVT *evt)
{
	uns32 rc = NCSCC_RC_SUCCESS;
	switch (evt->info.cpa.type) {
	case CPA_EVT_ND2A_CKPT_OPEN_RSP:
		cpa_proc_async_open_rsp(cb, &evt->info.cpa);
		break;

	case CPA_EVT_ND2A_CKPT_SYNC_RSP:
		cpa_proc_async_sync_rsp(cb, &evt->info.cpa);
		break;

	case CPA_EVT_ND2A_CKPT_ARRIVAL_NTFY:

		cpa_proc_ckpt_arrival_ntfy(cb, &evt->info.cpa);
		break;

	case CPA_EVT_TIME_OUT:
		cpa_proc_tmr_expiry(cb, &evt->info.cpa);
		break;

	case CPA_EVT_ND2A_ACT_CKPT_INFO_BCAST_SEND:
	case CPA_EVT_D2A_ACT_CKPT_INFO_BCAST_SEND:
		cpa_proc_active_ckpt_info_bcast(cb, &evt->info.cpa);
		break;

	case CPA_EVT_D2A_NDRESTART:
		cpa_proc_active_nd_down_bcast(cb, &evt->info.cpa);
		break;
	case CPA_EVT_ND2A_CKPT_CLM_NODE_LEFT:
	case CPA_EVT_ND2A_CKPT_CLM_NODE_JOINED:
		if (cpa_proc_ckpt_clm_status_changed(cb, evt) != NCSCC_RC_SUCCESS)
			rc = NCSCC_RC_FAILURE;
		break;

	default:
		break;
	}
	return rc;
}

/****************************************************************************
  Name          : cpa_callback_ipc_rcv
 
  Description   : This routine is used Receive the message posted to callback
                  MBX.
 
  Return Values : pointer to the callback
 
  Notes         : None
******************************************************************************/
CPA_CALLBACK_INFO *cpa_callback_ipc_rcv(CPA_CLIENT_NODE *cl_node)
{
	CPA_CALLBACK_INFO *cb_info = NULL;

	/* remove it to the queue */
	cb_info = (CPA_CALLBACK_INFO *)m_NCS_IPC_NON_BLK_RECEIVE(&cl_node->callbk_mbx, NULL);

	return cb_info;
}

/****************************************************************************
  Name          : cpa_hdl_callbk_dispatch_one
 
  Description   : This routine dispatches one pending callback.
 
  Arguments     : cb      - ptr to the CPA control block
                  ckptHandle - cpsv handle
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None
******************************************************************************/
uns32 cpa_hdl_callbk_dispatch_one(CPA_CB *cb, SaCkptHandleT ckptHandle)
{
	CPA_CALLBACK_INFO *callback = NULL;
	uns32 rc = SA_AIS_OK;
	CPA_LOCAL_CKPT_NODE *lc_node = NULL;
	CPA_CLIENT_NODE *cl_node = NULL;

	if (m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
		m_LOG_CPA_HEADLINE(CPA_CB_LOCK_TAKE_FAILED, NCSFL_SEV_ERROR);
		return SA_AIS_ERR_LIBRARY;
	}

	cpa_client_node_get(&cb->client_tree, &ckptHandle, &cl_node);

	if (cl_node == NULL) {
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
		return SA_AIS_ERR_BAD_HANDLE;
	}

	/* get it from the queue */
	while ((callback = cpa_callback_ipc_rcv(cl_node))) {
		cpa_lcl_ckpt_node_get(&cb->lcl_ckpt_tree, &callback->lcl_ckpt_hdl, &lc_node);
		if (lc_node) {
			m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
			cpa_process_callback_info(cb, cl_node, callback);
			return SA_AIS_OK;
		} else {
			if (m_CPA_VER_IS_ABOVE_B_1_1(&cl_node->version)) {
				if ((callback->type == CPA_CALLBACK_TYPE_OPEN) ||
				    (callback->type == CPA_CALLBACK_TYPE_SYNC)) {
					callback->sa_err = SA_AIS_ERR_BAD_HANDLE;
					m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
					cpa_process_callback_info(cb, cl_node, callback);
					return SA_AIS_OK;
				}
			}
			m_MMGR_FREE_CPA_CALLBACK_INFO(callback);
			continue;
		}
	}

	m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);

	return rc;
}

/****************************************************************************
  Name          : cpa_hdl_callbk_dispatch_all
 
  Description   : This routine dispatches all pending callback.
 
  Arguments     : cb      - ptr to the CPA control block
                  ckptHandle - cpsv handle
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None
******************************************************************************/
uns32 cpa_hdl_callbk_dispatch_all(CPA_CB *cb, SaCkptHandleT ckptHandle)
{
	CPA_CALLBACK_INFO *callback = NULL;
	CPA_LOCAL_CKPT_NODE *lc_node = NULL;
	CPA_CLIENT_NODE *cl_node = NULL;

	if (m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
		m_LOG_CPA_HEADLINE(CPA_CB_LOCK_TAKE_FAILED, NCSFL_SEV_ERROR);
		return SA_AIS_ERR_LIBRARY;
	}

	cpa_client_node_get(&cb->client_tree, &ckptHandle, &cl_node);

	if (cl_node == NULL) {
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
		return SA_AIS_ERR_BAD_HANDLE;
	}

	while ((callback = cpa_callback_ipc_rcv(cl_node))) {
		cpa_lcl_ckpt_node_get(&cb->lcl_ckpt_tree, &callback->lcl_ckpt_hdl, &lc_node);
		if (lc_node) {
			m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
			cpa_process_callback_info(cb, cl_node, callback);
		} else {
			m_MMGR_FREE_CPA_CALLBACK_INFO(callback);
			break;
		}

		if (m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
			m_LOG_CPA_HEADLINE(CPA_CB_LOCK_TAKE_FAILED, NCSFL_SEV_ERROR);
			return SA_AIS_ERR_LIBRARY;
		}

		cpa_client_node_get(&cb->client_tree, &ckptHandle, &cl_node);

		if (cl_node == NULL) {
			m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
			return SA_AIS_ERR_BAD_HANDLE;
		}

	}

	m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);

	return SA_AIS_OK;
}

/****************************************************************************
  Name          : cpa_hdl_callbk_dispatch_block
 
  Description   : This routine dispatches all pending callback.
 
  Arguments     : cb      - ptr to the CPA control block
                  ckptHandle - cpsv handle
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None
******************************************************************************/
uns32 cpa_hdl_callbk_dispatch_block(CPA_CB *cb, SaCkptHandleT ckptHandle)
{
	CPA_CALLBACK_INFO *callback = NULL;
	SYSF_MBX *callbk_mbx = NULL;
	CPA_CLIENT_NODE *client_info = NULL;
	CPA_LOCAL_CKPT_NODE *lc_node = NULL;

	if (m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
		m_LOG_CPA_HEADLINE(CPA_CB_LOCK_TAKE_FAILED, NCSFL_SEV_ERROR);
		return SA_AIS_ERR_LIBRARY;
	}

	cpa_client_node_get(&cb->client_tree, &ckptHandle, &client_info);

	if (client_info == NULL) {
		/* Another thread called Finalize */
		m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
		return SA_AIS_ERR_BAD_HANDLE;
	}

	callbk_mbx = &(client_info->callbk_mbx);

	m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);

	callback = (CPA_CALLBACK_INFO *)m_NCS_IPC_RECEIVE(callbk_mbx, NULL);

	while (1) {
		/* Take the CB Lock */
		if (m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
			m_LOG_CPA_HEADLINE(CPA_CB_LOCK_TAKE_FAILED, NCSFL_SEV_ERROR);
			return SA_AIS_ERR_LIBRARY;
		}

		cpa_client_node_get(&cb->client_tree, &ckptHandle, &client_info);

		if (client_info == NULL) {
			/* Another thread called Finalize */
			m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
			return SA_AIS_OK;
		}

		if (callback) {
			cpa_lcl_ckpt_node_get(&cb->lcl_ckpt_tree, &callback->lcl_ckpt_hdl, &lc_node);
			if (lc_node) {
				m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
				cpa_process_callback_info(cb, client_info, callback);
			} else {
				m_MMGR_FREE_CPA_CALLBACK_INFO(callback);
				m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
				return SA_AIS_OK;

			}
		} else {
			m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
			return SA_AIS_ERR_LIBRARY;
		}

		callback = (CPA_CALLBACK_INFO *)m_NCS_IPC_RECEIVE(callbk_mbx, NULL);
		if (!callback)
			return SA_AIS_OK;

	}
}

/****************************************************************************
  Name          : cpa_process_callback_info
 
  Description   : This routine invokes the registered callback routine.
 
  Arguments     : cb  - ptr to the CPA control block
                  cl_node - Client Node
                  cb_info - ptr to the registered callbacks
 
  Return Values : None
 
  Notes         : None
******************************************************************************/
static void cpa_process_callback_info(CPA_CB *cb, CPA_CLIENT_NODE *cl_node, CPA_CALLBACK_INFO *callback)
{

	/* invoke the corresponding callback */
	switch (callback->type) {
	case CPA_CALLBACK_TYPE_OPEN:
		if (cl_node->ckpt_callbk.saCkptCheckpointOpenCallback) {
			cl_node->ckpt_callbk.saCkptCheckpointOpenCallback(callback->invocation,
									  callback->lcl_ckpt_hdl, callback->sa_err);
		}
		break;

	case CPA_CALLBACK_TYPE_SYNC:
		if (cl_node->ckpt_callbk.saCkptCheckpointSynchronizeCallback) {
			cl_node->ckpt_callbk.saCkptCheckpointSynchronizeCallback(callback->invocation,
										 callback->sa_err);
		}
		break;
	case CPA_CALLBACK_TYPE_ARRIVAL_NTFY:
		if (cl_node->ckptArrivalCallback) {
			uns32 i = 0;
			cl_node->ckptArrivalCallback(callback->lcl_ckpt_hdl, callback->ioVector,
						     callback->num_of_elmts);
			for (i = 0; i < callback->num_of_elmts; i++) {
				if (callback->ioVector[i].sectionId.id != NULL
				    && callback->ioVector[i].sectionId.idLen != 0)
					m_MMGR_FREE_CPSV_DEFAULT_VAL(callback->ioVector[i].sectionId.id,
								     NCS_SERVICE_ID_CPA);
			}
			m_MMGR_FREE_SaCkptIOVectorElementT(callback->ioVector);

		}
		break;

	default:
		break;
	}

	/* free the callback info. This will be allocated by MDS EDU functions */
	m_MMGR_FREE_CPA_CALLBACK_INFO(callback);
}

/****************************************************************************
  Name          : cpa_proc_build_data_access_evt
 
  Description   : build CPSV_CKPT_DATA from ioVector to send it to CPND
  Arguments     : ioVector -- pointer to ioVector 
                  numberOfElements -- Number of ioVectors
                  ckpt_data        --- Pointer to CPSV_CKPT_DATA
                  data_access_type ---- Either Write/Read Access 
  Return Values : NCSCC_RC_FAILURE/NCSCC_RC_SUCCESS
  Notes         : None
******************************************************************************/
uns32 cpa_proc_build_data_access_evt(const SaCkptIOVectorElementT *ioVector,
				     uns32 numberOfElements, uns32 data_access_type,
				     SaSizeT maxSectionSize, SaUint32T *errflag, CPSV_CKPT_DATA **ckpt_data)
{
	CPSV_CKPT_DATA *tmp_ckpt_data = NULL;
	if (numberOfElements > 0) {
		while (numberOfElements > 0) {
			tmp_ckpt_data = m_MMGR_ALLOC_CPSV_CKPT_DATA;
			if (tmp_ckpt_data == NULL)
				return NCSCC_RC_FAILURE;
			memset(tmp_ckpt_data, '\0', sizeof(CPSV_CKPT_DATA));

			switch (data_access_type) {
			case CPSV_CKPT_ACCESS_WRITE:
				tmp_ckpt_data->sec_id = ioVector[numberOfElements - 1].sectionId;
				tmp_ckpt_data->data = ioVector[numberOfElements - 1].dataBuffer;
				tmp_ckpt_data->dataSize = ioVector[numberOfElements - 1].dataSize;
				tmp_ckpt_data->dataOffset = ioVector[numberOfElements - 1].dataOffset;
				tmp_ckpt_data->next = NULL;
				break;

			case CPSV_CKPT_ACCESS_READ:
				tmp_ckpt_data->sec_id = ioVector[numberOfElements - 1].sectionId;
				if (ioVector[numberOfElements - 1].dataBuffer == NULL) {
					tmp_ckpt_data->dataSize = 0;
				} else {
					if ((ioVector[numberOfElements - 1].dataSize > maxSectionSize)
					    || (ioVector[numberOfElements - 1].dataOffset +
						ioVector[numberOfElements - 1].dataSize) > maxSectionSize) {
						if (errflag != NULL)
							*errflag = (numberOfElements - 1);
						return NCSCC_RC_FAILURE;
					} else
					tmp_ckpt_data->dataSize = ioVector[numberOfElements - 1].dataSize;
				}
				tmp_ckpt_data->dataOffset = ioVector[numberOfElements - 1].dataOffset;
				tmp_ckpt_data->next = NULL;
				break;

			default:
				return NCSCC_RC_FAILURE;
			}

			if (*ckpt_data == NULL)
				*ckpt_data = tmp_ckpt_data;
			else {
				tmp_ckpt_data->next = (*ckpt_data);
				(*ckpt_data) = tmp_ckpt_data;
			}
			numberOfElements--;
		}
		return NCSCC_RC_SUCCESS;
	} else {
		return NCSCC_RC_FAILURE;
	}
}

/****************************************************************************
  Name          : cpa_proc_free_arrival_ntfy_cpsv_ckpt_data
  Description   : frees memory for  CPSV_CKPT_DATA 
  Arguments     : ckpt_data -- pointer to CPSV_CKPT_DATA
  Return Values : NCSCC_RC_FAILURE/NCSCC_RC_SUCCESS
  Notes         : None
******************************************************************************/
void cpa_proc_free_arrival_ntfy_cpsv_ckpt_data(CPSV_CKPT_DATA *ckpt_data)
{
	CPSV_CKPT_DATA *tmp_data, *next_data;
	tmp_data = ckpt_data;
	do {
		if (tmp_data->sec_id.id != NULL && tmp_data->sec_id.idLen != 0)
			m_MMGR_FREE_CPSV_DEFAULT_VAL(tmp_data->sec_id.id, NCS_SERVICE_ID_CPND);

		if (tmp_data->data != NULL)
			m_MMGR_FREE_CPSV_SYS_MEMORY(tmp_data->data);
		next_data = tmp_data->next;
		m_MMGR_FREE_CPSV_CKPT_DATA(tmp_data);
		tmp_data = next_data;
	} while (tmp_data != NULL);

}

/****************************************************************************
  Name          : cpa_proc_free_cpsv_ckpt_data
  Description   : frees memory for  CPSV_CKPT_DATA 
  Arguments     : ckpt_data -- pointer to CPSV_CKPT_DATA
  Return Values : NCSCC_RC_FAILURE/NCSCC_RC_SUCCESS
  Notes         : None
******************************************************************************/
void cpa_proc_free_cpsv_ckpt_data(CPSV_CKPT_DATA *ckpt_data)
{

	CPSV_CKPT_DATA *tmp_ckpt_data = NULL;
	while (ckpt_data != NULL) {

		tmp_ckpt_data = ckpt_data;
		ckpt_data = ckpt_data->next;
		m_MMGR_FREE_CPSV_CKPT_DATA(tmp_ckpt_data);
	}
}

void cpa_proc_free_read_data(CPSV_ND2A_DATA_ACCESS_RSP *rmt_read_rsp)
{
	uns32 iter = 0;
	if (rmt_read_rsp != NULL && rmt_read_rsp->info.read_data != NULL) {
		for (; iter < rmt_read_rsp->size; iter++) {
			if (rmt_read_rsp->info.read_data[iter].data != NULL) {
				m_MMGR_FREE_CPSV_DEFAULT_VAL(rmt_read_rsp->info.read_data[iter].data,
							     NCS_SERVICE_ID_CPA);
			}
		}
		m_MMGR_FREE_CPSV_ND2A_READ_DATA(rmt_read_rsp->info.read_data, NCS_SERVICE_ID_CPA);
	}
}

/****************************************************************************
  Name          : cpa_proc_replica_read
  Description   : reads replica from (shared memory )
  Arguments     :  cb         ---   Control block of CPA
                   ioVector   ---   ioVector of Data
                   read_map   ---   read_map information received from ND
                   erroneousVectorIndex ---   Vector Index
                   gbl_ckpt_hdl  ----  Global Checkpoint Hdl
                   numbeofOfElements --- Number of Elements
  Return Values : NCSCC_RC_FAILURE/NCSCC_RC_SUCCESS
  Notes         : None
******************************************************************************/
uns32 cpa_proc_replica_read(CPA_CB *cb, SaUint32T numberOfElements,
			    SaCkptCheckpointHandleT gbl_ckpt_hdl,
			    SaCkptIOVectorElementT **ioVector, CPSV_ND2A_READ_MAP *read_map,
			    SaUint32T **erroneousVectorIndex)
{
	uns32 iter = 0, rc = NCSCC_RC_SUCCESS;
	NCS_OS_POSIX_SHM_REQ_INFO shm_info;
	CPA_GLOBAL_CKPT_NODE *gc_node;
	NCS_BOOL add_flag = FALSE;

	memset(&shm_info, '\0', sizeof(NCS_OS_POSIX_SHM_REQ_INFO));

	cpa_gbl_ckpt_node_find_add(&cb->gbl_ckpt_tree, &gbl_ckpt_hdl, &gc_node, &add_flag);

	for (; iter < numberOfElements; iter++) {
		if (read_map[iter].offset_index == -1) {
			if (*erroneousVectorIndex != NULL) 
				(*erroneousVectorIndex)[0] = iter;
			return NCSCC_RC_FAILURE;
		}

		(*ioVector)[iter].readSize = read_map[iter].read_size;

		if (read_map[iter].read_size == 0) {
			continue;
		} else {
			if ((*ioVector)[iter].dataBuffer == NULL) {
				(*ioVector)[iter].dataBuffer = m_MMGR_ALLOC_CPA_DEFAULT(read_map[iter].read_size);
				if ((*ioVector)[iter].dataBuffer == NULL) {
					m_LOG_CPA_MEMFAIL(CPA_DATA_BUFF_ALLOC_FAILED);
					return NCSCC_RC_FAILURE;
				}
				memset((*ioVector)[iter].dataBuffer, '\0', read_map[iter].read_size);
			}
			shm_info.type = NCS_OS_POSIX_SHM_REQ_READ;
			shm_info.info.read.i_addr =
			    (void *)((char *)gc_node->open.info.open.o_addr + sizeof(CPSV_CKPT_HDR) +
				     ((read_map[iter].offset_index + 1) * sizeof(CPSV_SECT_HDR)) +
				     (read_map[iter].offset_index * gc_node->ckpt_creat_attri.maxSectionSize));
			shm_info.info.read.i_to_buff = (*ioVector)[iter].dataBuffer;
			shm_info.info.read.i_read_size = read_map[iter].read_size;
			shm_info.info.read.i_offset = (*ioVector)[iter].dataOffset;

			ncs_os_posix_shm(&shm_info);
		}

	}
	return rc;

}

/****************************************************************************
  Name          : cpa_proc_rmt_replica_read
  Description   : reads replica from ND Message 
  Arguments     :  ioVector   ---   ioVector of Data
                   read_data  ---   read_data information received from ND
                   erroneousVectorIndex ---   Vector Index
                   numbeofOfElements --- Number of Elements
  Return Values : NCSCC_RC_FAILURE/NCSCC_RC_SUCCESS
  Notes         : None
******************************************************************************/
uns32 cpa_proc_rmt_replica_read(SaUint32T numberOfElements,
				SaCkptIOVectorElementT *ioVector, CPSV_ND2A_READ_DATA *read_data,
				SaUint32T **erroneousVectorIndex, SaVersionT *version)
{
	uns32 iter = 0, rc = NCSCC_RC_SUCCESS;

	for (iter = 0; iter < numberOfElements; iter++) {
		if (read_data[iter].err == 1) {
			if (*erroneousVectorIndex != NULL) 
				(**erroneousVectorIndex) = (uns32)iter;
			return NCSCC_RC_FAILURE;
		} else {
			if (read_data[iter].read_size == 0)
				continue;
			if (ioVector[iter].dataBuffer == NULL) {
				if (m_CPA_VER_IS_ABOVE_B_1_1(version))
					ioVector[iter].dataBuffer = m_MMGR_ALLOC_CPA_DEFAULT(read_data[iter].read_size);
				else
					ioVector[iter].dataBuffer = (uns8 *)malloc(read_data[iter].read_size);
				if (ioVector[iter].dataBuffer == NULL) {
					m_LOG_CPA_MEMFAIL(CPA_DATA_BUFF_ALLOC_FAILED);
					return NCSCC_RC_FAILURE;
				}
				memset(ioVector[iter].dataBuffer, '\0', read_data[iter].read_size);
			}
			memcpy(ioVector[iter].dataBuffer, read_data[iter].data, read_data[iter].read_size);
			ioVector[iter].readSize = read_data[iter].read_size;
		}
	}
	return rc;
}

/****************************************************************************
  Name          : cpa_proc_check_iovector
  Description   : procedure to check IoVector data size with ckpt max sizes
  Arguments     : 
  Return Values : NCSCC_RC_FAILURE/NCSCC_RC_SUCCESS
  Notes         : None
******************************************************************************/
uns32 cpa_proc_check_iovector(CPA_CB *cb, CPA_LOCAL_CKPT_NODE *lc_node, const SaCkptIOVectorElementT *iovector,
			      uns32 num_of_elmts, uns32 *errflag)
{
	uns32 iter, rc = NCSCC_RC_SUCCESS;
	uns32 proc_rc = NCSCC_RC_SUCCESS;
	NCS_BOOL add_flag = FALSE;
	CPA_GLOBAL_CKPT_NODE *gc_node = NULL;

	proc_rc = cpa_gbl_ckpt_node_find_add(&cb->gbl_ckpt_tree, &lc_node->gbl_ckpt_hdl, &gc_node, &add_flag);
	if (gc_node == NULL) {
		return NCSCC_RC_FAILURE;

	}

	for (iter = 0; iter < num_of_elmts; iter++)
		if (gc_node->ckpt_creat_attri.maxSectionSize < iovector[iter].dataSize) {
			*errflag = iter;
			return NCSCC_RC_FAILURE;
		}

	return rc;
}

/********************************************************************
 Name    : cpa_sync_with_cpd_for_active_replica_set 
   
 Description :  
   
**********************************************************************/
void cpa_sync_with_cpd_for_active_replica_set(CPA_GLOBAL_CKPT_NODE *gc_node)
{
	NCS_SEL_OBJ_SET set;
	uns32 timeout = 5000;

	m_NCS_LOCK(&gc_node->cpd_active_sync_lock, NCS_LOCK_WRITE);
	if (gc_node->is_active_bcast_came == TRUE) {
		m_NCS_UNLOCK(&gc_node->cpd_active_sync_lock, NCS_LOCK_WRITE);
		return;
	}
	/* Creat the sync Lock and wait for the active set */
	gc_node->cpd_active_sync_awaited = TRUE;
	m_NCS_SEL_OBJ_CREATE(&gc_node->cpd_active_sync_sel);
	m_NCS_UNLOCK(&gc_node->cpd_active_sync_lock, NCS_LOCK_WRITE);

	m_NCS_SEL_OBJ_ZERO(&set);
	m_NCS_SEL_OBJ_SET(gc_node->cpd_active_sync_sel, &set);
	m_NCS_SEL_OBJ_SELECT(gc_node->cpd_active_sync_sel, &set, 0, 0, &timeout);

	/* Destroy the sync - object */
	m_NCS_LOCK(&gc_node->cpd_active_sync_lock, NCS_LOCK_WRITE);
	gc_node->cpd_active_sync_awaited = FALSE;
	m_NCS_SEL_OBJ_DESTROY(gc_node->cpd_active_sync_sel);
	m_NCS_UNLOCK(&gc_node->cpd_active_sync_lock, NCS_LOCK_WRITE);
	return;
}

/****************************************************************************
 * Name          : cpa_cb_dump
 *
 * Description   : This routine is used for debugging.
 *
 * Return Values : NCSCC_RC_SUCCESS/Error.
 *
 * Notes         : None.
 *****************************************************************************/
void cpa_cb_dump(void)
{
	CPA_CB *cb = NULL;

	/* retrieve CPA CB */
	m_CPA_RETRIEVE_CB(cb);
	if (cb == NULL) {
		return;
	}

	/* Take the CB lock */
	if (m_NCS_LOCK(&cb->cb_lock, NCS_LOCK_WRITE) != NCSCC_RC_SUCCESS) {
		m_CPA_GIVEUP_CB;
		return;
	}

	TRACE("*****************Printing CPA CB Dump******************");
	TRACE(" MDS Handle:             %x", cb->cpa_mds_hdl);
	TRACE(" Handle Manager Pool ID: %d", cb->pool_id);
	TRACE(" Handle Manager Handle:  %d", cb->agent_handle_id);
	if (cb->is_cpnd_up) {
		TRACE(" CPND UP, Node ID = %d", m_NCS_NODE_ID_FROM_MDS_DEST(cb->cpnd_mds_dest));
	} else
		TRACE(" CPND DOWN");

	if (cb->is_client_tree_up) {
		TRACE("+++++++++++++Client Tree is UP+++++++++++++++++++++++++++");
		TRACE("Number of nodes in ClientTree:  %d", cb->client_tree.n_nodes);

		/* Print the Client tree Details */
		{
			CPA_CLIENT_NODE *clnode = NULL;
			SaCkptHandleT *temp_ptr = 0;
			SaCkptHandleT temp_hdl = 0;

			temp_ptr = &temp_hdl;

			/* scan the entire handle db & delete each record */
			while ((clnode = (CPA_CLIENT_NODE *)
				ncs_patricia_tree_getnext(&cb->client_tree, (uns8 *)temp_ptr))) {
				/* delete the client info */
				temp_hdl = clnode->cl_hdl;

				TRACE("------------------------------------------------------");
				TRACE(" CLient Handle   = %d", (uns32)clnode->cl_hdl);
			}
			TRACE(" End of Info for this client");
		}
		TRACE(" End of Client info nodes ");
	}

	/* Print the Lcl Checkpoint Details */
	if (cb->is_lcl_ckpt_tree_up) {
		TRACE("+++++++++++++Lcl CKPT Tree is UP+++++++++++++++++++++++++++");
		TRACE("Number of nodes in Lcl CKPT Tree:  %d", cb->lcl_ckpt_tree.n_nodes);

		/* Print the Lcl CKPT Details */
		{
			SaCkptCheckpointHandleT prev_ckpt_id = 0;
			CPA_LOCAL_CKPT_NODE *lc_node;

			/* Get the First Node */
			lc_node = (CPA_LOCAL_CKPT_NODE *)ncs_patricia_tree_getnext(&cb->lcl_ckpt_tree,
										   (uns8 *)&prev_ckpt_id);

			while (lc_node) {
				prev_ckpt_id = lc_node->lcl_ckpt_hdl;

				TRACE("------------------------------------------------------");
				TRACE(" Lcl CKPT Hdl:  = %d", (uns32)lc_node->lcl_ckpt_hdl);
				TRACE(" Client CKPT Hdl:  = %d", (uns32)lc_node->cl_hdl);
				TRACE(" Global CKPT Hdl:  = %d", (uns32)lc_node->gbl_ckpt_hdl);
				TRACE(" Open Flags:  = %d", (uns32)lc_node->open_flags);
				if (lc_node->async_req_tmr.is_active)
					TRACE("Timer Type %d is active", lc_node->async_req_tmr.type);
				else
					TRACE(" Timer is not active");

				TRACE(" End of Local CKPT Info");

				lc_node = (CPA_LOCAL_CKPT_NODE *)ncs_patricia_tree_getnext(&cb->lcl_ckpt_tree,
											   (uns8 *)&prev_ckpt_id);
			}
			TRACE(" End of Local CKPT nodes information ");
		}

	}

	/* Print the Global Checkpoint Details */
	if (cb->is_gbl_ckpt_tree_up) {
		TRACE("+++++++++++++Global CKPT Tree is UP+++++++++++++++++++++++++++");
		TRACE("Number of nodes in Global CKPT Tree:  %d", cb->gbl_ckpt_tree.n_nodes);

		/* Print the Gbl CKPT Details */
		{
			SaCkptCheckpointHandleT prev_ckpt_id = 0;
			CPA_GLOBAL_CKPT_NODE *gc_node;

			/* Get the First Node */
			gc_node = (CPA_GLOBAL_CKPT_NODE *)ncs_patricia_tree_getnext(&cb->gbl_ckpt_tree,
										    (uns8 *)&prev_ckpt_id);

			while (gc_node) {
				prev_ckpt_id = gc_node->gbl_ckpt_hdl;

				TRACE("------------------------------------------------------");
				TRACE(" Lcl CKPT Hdl:  = %d", (uns32)gc_node->gbl_ckpt_hdl);
				TRACE(" No of Clients = %d", gc_node->ref_cnt);

				TRACE(" End of Local CKPT Info");

				gc_node = (CPA_GLOBAL_CKPT_NODE *)ncs_patricia_tree_getnext(&cb->gbl_ckpt_tree,
											    (uns8 *)&prev_ckpt_id);
			}

			TRACE(" End of Local CKPT nodes information ");
		}
	}
	TRACE("*****************End of CPD CB Dump******************");

	m_NCS_UNLOCK(&cb->cb_lock, NCS_LOCK_WRITE);
	m_CPA_GIVEUP_CB;
	return;
}
