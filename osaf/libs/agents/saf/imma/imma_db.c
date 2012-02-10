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
  
  This file contains the IMMA processing routines (Routines to add, 
  delete nodes from IMM Agent CB repository, callback processing routines etc.)
*****************************************************************************/

#include "imma.h"
/****************************************************************************
  Name          : imma_client_tree_init
  Description   : This routine is used to initialize the client tree
  Arguments     : cb - pointer to the IMMA Control Block
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
  Notes         : None
******************************************************************************/
uint32_t imma_client_tree_init(IMMA_CB *cb)
{
	NCS_PATRICIA_PARAMS param;
	memset(&param, 0, sizeof(NCS_PATRICIA_PARAMS));
	param.key_size = sizeof(SaImmHandleT);
	if (ncs_patricia_tree_init(&cb->client_tree, &param) != NCSCC_RC_SUCCESS) {
		return NCSCC_RC_FAILURE;
	}
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
  Name          : imma_client_node_get
  Description   : This routine finds the client node.
  Arguments     : client_tree - Client Tree.
                  cl_hdl - Client Handle
  Notes         : The caller takes the cb lock before calling this function                 
******************************************************************************/
void imma_client_node_get(NCS_PATRICIA_TREE *client_tree, SaImmHandleT *cl_hdl, IMMA_CLIENT_NODE **cl_node)
{
	*cl_node = (IMMA_CLIENT_NODE *)
	    ncs_patricia_tree_get(client_tree, (uint8_t *)cl_hdl);
}

/****************************************************************************
  Name          : imma_client_node_add
  Description   : This routine adds the new client to the client tree.
  Arguments     :client_tree - Client Tree.
                 cl_node - Client Node
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
  Notes         : The caller takes the cb lock before calling this function                  
******************************************************************************/
uint32_t imma_client_node_add(NCS_PATRICIA_TREE *client_tree, IMMA_CLIENT_NODE *cl_node)
{
	uint32_t rc = NCSCC_RC_FAILURE;
	/* Store the client_info pointer as msghandle. */
	cl_node->patnode.key_info = (uint8_t *)&cl_node->handle;

	if ((rc = ncs_patricia_tree_add(client_tree, &cl_node->patnode)) != NCSCC_RC_SUCCESS) {
		return NCSCC_RC_FAILURE;
	}

	return NCSCC_RC_SUCCESS;
}


int imma_oi_ccb_record_delete(IMMA_CLIENT_NODE *cl_node, SaImmOiCcbIdT ccbId);

/****************************************************************************
  Name          : imma_client_node_delete
  Description   : This routine deletes the client from the client tree
  Arguments     : IMMA_CB *cb - IMMA Control Block.
                : IMMA_CLIENT_NODE *cl_node - Client Node.
  Return Values : None
  Notes         : None
******************************************************************************/
uint32_t imma_client_node_delete(IMMA_CB *cb, IMMA_CLIENT_NODE *cl_node)
{
	uint32_t rc = NCSCC_RC_SUCCESS;

	if (cl_node == NULL)
		return NCSCC_RC_FAILURE;

	/* Remove the Node from the client tree */
	if (ncs_patricia_tree_del(&cb->client_tree, &cl_node->patnode) != NCSCC_RC_SUCCESS) {
		rc = NCSCC_RC_FAILURE;
	}

	/* Remove any implementer name associated with handle */
	if (cl_node->mImplementerName) {
		free(cl_node->mImplementerName);
		cl_node->mImplementerName = NULL;
		cl_node->mImplementerId = 0;
	}

	/* Remove any ccb_records. */

	while(cl_node->activeOiCcbs) {
		imma_oi_ccb_record_delete(cl_node, cl_node->activeOiCcbs->ccbId);
	}

	free(cl_node);

	return rc;
}

/****************************************************************************
  Name          : imma_client_tree_destroy
  Description   : This routine destroys the IMMA client tree.
  Arguments     : IMMA_CB *cb - IMMA Control Block.
  Return Values : None
  Notes         : None
******************************************************************************/
void imma_client_tree_destroy(IMMA_CB *cb)
{
	TRACE_ENTER();
	/* cleanup the client tree */
	imma_client_tree_cleanup(cb);

	/* destroy the tree */
	ncs_patricia_tree_destroy(&cb->client_tree);
	TRACE_LEAVE();
	return;
}

/****************************************************************************
  Name          : imma_client_tree_cleanup
  Description   : This routine cleansup the IMMA Client tree
  Arguments     : IMMA_CB *cb - IMMA Control Block.
  Return Values : None
  Notes         : None
******************************************************************************/
void imma_client_tree_cleanup(IMMA_CB *cb)
{
	IMMA_CLIENT_NODE *clnode;
	SaImmHandleT *temp_ptr = 0;
	SaImmHandleT temp_hdl = 0;

	TRACE_ENTER();

	/* scan the entire handle db & delete each record */
	while ((clnode = (IMMA_CLIENT_NODE *)
		ncs_patricia_tree_getnext(&cb->client_tree, (uint8_t *)temp_ptr))) {
		/* delete the client info */
		temp_hdl = clnode->handle;
		temp_ptr = &temp_hdl;

		/* Destroy the IPC attached to this client */
		imma_callback_ipc_destroy(clnode);

		/* Delete the Client Node */
		imma_client_node_delete(cb, clnode);
	}
	TRACE_LEAVE();
	return;
}

/****************************************************************************
  Name          : imma_oi_ccb_record_find/add/terminate/delete
  Description   : When OI receives a ccb related upcall, we add a record
                  to the client structure keeping track of ccbId and if the ccb
                  has reached critical state. This simplifies recovery processing
                  in the case of IMMND crash. Non critical CCBs can then be aborted 
                  autonomously by the IMMA because it knows the CCB had to be aborted.
                  For critical ccbs, it has to recover ccb-outcomes from the
                  restarted IMMND. 

  Arguments     : IMMA_CLIENT_NODE *cl_node - client node.
                : SaUint32T ccbId - the ccbId.

******************************************************************************/
struct imma_oi_ccb_record * imma_oi_ccb_record_find(IMMA_CLIENT_NODE *cl_node, SaImmOiCcbIdT ccbId)
{
	TRACE_ENTER();
	struct imma_oi_ccb_record *tmp = cl_node->activeOiCcbs;
	while (tmp && (tmp->ccbId != ccbId)) {
		tmp = tmp->next;
	}

	if(tmp) TRACE("Record for ccbid:0x%llx handle:%llx client:%p found", 
		ccbId, cl_node->handle, cl_node);
	else    TRACE("Record for ccbid:0x%llx handle:%llx client:%p NOT found", 
		ccbId, cl_node->handle, cl_node);

	TRACE_LEAVE();
	return tmp;
}

int imma_oi_ccb_record_exists(IMMA_CLIENT_NODE *cl_node, SaImmOiCcbIdT ccbId)
{
	return (imma_oi_ccb_record_find(cl_node, ccbId) != NULL);
}

void imma_oi_ccb_record_add(IMMA_CLIENT_NODE *cl_node, SaImmOiCcbIdT ccbId, SaUint32T inv)
{
	TRACE_ENTER();
	struct imma_oi_ccb_record *new_ccb = imma_oi_ccb_record_find(cl_node, ccbId);
	if(new_ccb) { /* actually old/existing ccb if we found it. */
		if(new_ccb->isAborted) {
			TRACE("CcbOiRecord add for ccbid:0x%llx handle:%llx received AFTER aborted", 
				ccbId, cl_node->handle);
			return;
		}

		if(!inv) {
			new_ccb->opCount++;
			TRACE_2("Zero inv => PBE Incremented opcount to %u", new_ccb->opCount);
			if(!(cl_node->isPbe || cl_node->isApplier)) {
				LOG_ER("imma_oi_ccb_record_add inv==0 yet both isPbe and isApplier are false");
				osafassert(cl_node->isPbe);
			}
		}

		if(!(cl_node->isApplier)) {new_ccb->isCcbErrOk = true;}

		if (new_ccb->mCcbErrorString) { /* remove any old string. */
			free(new_ccb->mCcbErrorString);
			new_ccb->mCcbErrorString = NULL;
		}
		return;
	}

	new_ccb = calloc(1, sizeof(struct imma_oi_ccb_record));
	new_ccb->ccbId = ccbId;

	if(!(cl_node->isApplier)) {new_ccb->isCcbErrOk = true;}

	if(!inv) {/* zero inv =>PBE or Applier => count ops. */
		new_ccb->opCount = 1; 
		TRACE_2("Zero inv => PBE/Applier initialized opcount to 1");
		if(!(cl_node->isPbe || cl_node->isApplier)) {
			LOG_ER("imma_oi_ccb_record_add inv==0 yet cl_node->isPbe is false!");
			osafassert(cl_node->isPbe);
		}
	}
	new_ccb->next = cl_node->activeOiCcbs;
	cl_node->activeOiCcbs = new_ccb;
	TRACE("Record for ccbid:0x%llx handle:%llx client:%p opCount:%d added", 
		ccbId, cl_node->handle, cl_node, new_ccb->opCount);
	TRACE_LEAVE();
}

int imma_oi_ccb_record_delete(IMMA_CLIENT_NODE *cl_node, SaImmOiCcbIdT ccbId)
{
	TRACE_ENTER();
   	struct imma_oi_ccb_record **tmpp = &(cl_node->activeOiCcbs);
	while ((*tmpp) && ((*tmpp)->ccbId != ccbId)) {
		tmpp = &((*tmpp)->next);
	}

	if(*tmpp) {
		struct imma_oi_ccb_record *to_delete = (*tmpp);
		osafassert(to_delete->ccbId == ccbId);
		if(to_delete->isCritical) {
			TRACE_3("Removing imma_oi_ccb_record ccb:0x%llx handle:%llx client:%p in CRITICAL state", 
				ccbId, cl_node->handle, cl_node);
		} else {
			TRACE_2("Removing imma_oi_ccb_record ccb:0x%llx handle:%llx client:%p in non-critical state", 
				ccbId, cl_node->handle, cl_node);
		}
		(*tmpp) = to_delete->next;
		to_delete->next = NULL;
		to_delete->ccbId = 0LL;

		/* Remove any ccbErrorString associated with ccb_record */
		if (to_delete->mCcbErrorString) {
			free(to_delete->mCcbErrorString);
			to_delete->mCcbErrorString = NULL;
		}

		free(to_delete);
		TRACE_LEAVE();
		return 1;
	}

	TRACE_LEAVE();
	return 0;
}

int imma_oi_ccb_record_terminate(IMMA_CLIENT_NODE *cl_node, SaImmOiCcbIdT ccbId)
{
	TRACE_ENTER();
	int rs = 0;
	struct imma_oi_ccb_record *tmp = imma_oi_ccb_record_find(cl_node, ccbId);

	if(tmp) {
		tmp->isCritical = false;
		rs = imma_oi_ccb_record_delete(cl_node, ccbId);
	}

	TRACE_LEAVE();
	return rs;
}

int imma_oi_ccb_record_abort(IMMA_CLIENT_NODE *cl_node, SaImmOiCcbIdT ccbId)
{
	TRACE_ENTER();
	int rs = 0;
	struct imma_oi_ccb_record *tmp = imma_oi_ccb_record_find(cl_node, ccbId);

	if(tmp) {
		tmp->isCritical = false;
		tmp->isAborted = true;
		tmp->ccbCallback = NULL;
		rs = 1;
	}

	TRACE_LEAVE();
	return rs;
}

int imma_oi_ccb_record_ok_for_critical(IMMA_CLIENT_NODE *cl_node, SaImmOiCcbIdT ccbId, SaUint32T inv)
{
	TRACE_ENTER();
	int rs = 0;
	struct imma_oi_ccb_record *tmp = imma_oi_ccb_record_find(cl_node, ccbId);

	if(tmp && !(tmp->isAborted)) {
		osafassert(!tmp->isCritical);
		rs = 1;
		if(tmp->opCount) {
			if(!(cl_node->isPbe || cl_node->isApplier)) {
				LOG_ER("imma_oi_ccb_record_ok_for_critical opCount!=0 yet cl_node->isPbe "
					"is false! ccb:0x%llx", ccbId);
				rs = 0;
			}

			if(tmp->opCount != inv) {
				if(cl_node->isApplier) {
					TRACE_5("Mismatch in applier op-count %u should be %u for Ccbid:0x%llx"
						" - opcount not reliable for appliers. => Need a fix for #1795",  
					    tmp->opCount, inv, ccbId);
				} else {
					LOG_ER("Mismatch in PBE op-count %u should be %u for Ccbid:0x%llx",  
					    tmp->opCount, inv, ccbId);
					rs = 0;
				}
			} else {
				TRACE_5("op-count matches with inv:%u", inv);
			}
		}
		if(!(cl_node->isApplier)) {tmp->isCcbErrOk = true;}
		tmp->isCcbAugOk = false; /* not allowed to augment ccb in completed UC */
		tmp->ccbCallback = NULL;
		if (tmp->mCcbErrorString) { /* remove any old string. */
			free(tmp->mCcbErrorString);
			tmp->mCcbErrorString = NULL;
		}
	} else {
		LOG_NO("Record for ccb 0x%llx not found or found aborted in ok_for_critical", ccbId);
	}

	TRACE_LEAVE();
	return rs;
}

int imma_oi_ccb_record_set_critical(IMMA_CLIENT_NODE *cl_node, SaImmOiCcbIdT ccbId, SaUint32T inv)
{
	TRACE_ENTER();
	int rs = 0;
	struct imma_oi_ccb_record *tmp = imma_oi_ccb_record_find(cl_node, ccbId);

	if(tmp && !(tmp->isAborted)) {
		osafassert(!tmp->isCritical);
		tmp->isCritical = true;
		rs = 1;
		tmp->isCcbErrOk = false;
		tmp->isCcbAugOk = false;
		tmp->ccbCallback = NULL;
		if(tmp->opCount) {
			if(!(cl_node->isPbe || cl_node->isApplier)) {
				LOG_ER("imma_oi_ccb_record_set_critical opCount!=0 yet cl_node->isPbe is false! "
					"ccbId:0x%llx", ccbId);
				osafassert(cl_node->isPbe);
			}

			if(tmp->opCount != inv) {
				if(cl_node->isApplier) {
					TRACE_5("Mismatch in applier op-count %u should be %u for Ccbid:0x%llx"
						" - opcount not reliable for appliers. => Need a fix for #1795",  
						tmp->opCount, inv, ccbId);
				} else {
					LOG_ER("Mismatch in PBE op-count %u should be %u for CCBid:0x%llx (isPbe:%u)", 
						tmp->opCount, inv, ccbId, cl_node->isPbe);
					rs = 0;
				}
			} else {
				TRACE_5("op-count matches with inv:%u", inv);
			}
		}
		TRACE("Record for ccbid:0x%llx %llx %p PBE-opcount:%u set to critical", ccbId, cl_node->handle, cl_node, tmp->opCount);
	} else {
		LOG_NO("Record for ccb 0x%llx not found or found aborted in set_critical", ccbId);
	}

	TRACE_LEAVE();
	return rs;
}

int imma_oi_ccb_record_set_error(IMMA_CLIENT_NODE *cl_node, SaImmOiCcbIdT ccbId, SaStringT errorString)
{
	TRACE_ENTER();
	int rs = 0;
	struct imma_oi_ccb_record *tmp = imma_oi_ccb_record_find(cl_node, ccbId);

	if(tmp && tmp->isCcbErrOk) {
		osafassert(!(tmp->isCritical));
		rs = 1;
		if (tmp->mCcbErrorString) {
			free(tmp->mCcbErrorString);
			tmp->mCcbErrorString = NULL;
		}

		tmp->mCcbErrorString = strdup(errorString);
	}

	TRACE_LEAVE();
	return rs;
}

SaStringT imma_oi_ccb_record_get_error(IMMA_CLIENT_NODE *cl_node, SaImmOiCcbIdT ccbId)
{
	struct imma_oi_ccb_record *tmp = imma_oi_ccb_record_find(cl_node, ccbId);

	if(tmp && tmp->isCcbErrOk) {
		osafassert(!(tmp->isCritical));
		tmp->isCcbErrOk = 0x0; 
		return tmp->mCcbErrorString;
	}
	return NULL;
}

struct imma_callback_info * 
imma_oi_ccb_record_ok_augment(IMMA_CLIENT_NODE *cl_node, SaImmOiCcbIdT ccbId, SaImmHandleT *privateOmHandle,
	SaImmAdminOwnerHandleT *privateAoHandle)
{
	TRACE_ENTER();
	struct imma_callback_info * cbi = NULL;
	struct imma_oi_ccb_record *tmp = imma_oi_ccb_record_find(cl_node, ccbId);

	if(tmp) {
		if(tmp->isAborted) {
			TRACE("oi_ccb_record for %llu found as aborted in ccb_record_ok_augment",
				ccbId);
		} else if(tmp->isCcbAugOk) {
			osafassert(!(tmp->isCritical));
			osafassert(!(cl_node->isApplier));
			osafassert(tmp->ccbCallback);
			cbi = tmp->ccbCallback;
			if(privateOmHandle) {
				*privateOmHandle = tmp->privateAugOmHandle;
			}
			if(privateAoHandle) {
				*privateAoHandle = tmp->privateAoHandle;
			}			
		}
	}

	TRACE_LEAVE();
	return cbi;
}

void imma_oi_ccb_record_augment(IMMA_CLIENT_NODE *cl_node, SaImmOiCcbIdT ccbId, SaImmHandleT privateOmHandle,
	SaImmAdminOwnerHandleT privateAoHandle)
{
	TRACE_ENTER();
	struct imma_oi_ccb_record *tmp = imma_oi_ccb_record_find(cl_node, ccbId);

	osafassert(tmp && tmp->isCcbAugOk);

	osafassert(!(tmp->isAborted));

	osafassert(!(tmp->isCritical));

	osafassert(!(cl_node->isApplier));
	if(privateOmHandle) {
		if(tmp->privateAugOmHandle) {
			osafassert(tmp->privateAugOmHandle == privateOmHandle);
		} else {
			tmp->privateAugOmHandle = privateOmHandle;
		}
	}
	if(privateAoHandle) {
		if(tmp->privateAoHandle) {
			osafassert(tmp->privateAoHandle == privateAoHandle);
		} else {
			tmp->privateAoHandle = privateAoHandle;
		}
	}
	TRACE_LEAVE();
}

int imma_oi_ccb_record_close_augment(IMMA_CLIENT_NODE *cl_node, SaImmOiCcbIdT ccbId, SaImmHandleT *privateOmHandle, bool ccbDone)
{
	int rs = 0;
	struct imma_oi_ccb_record *tmp = imma_oi_ccb_record_find(cl_node, ccbId);

	if(tmp) {
		tmp->isCcbAugOk = false;
		if(privateOmHandle) {
			*privateOmHandle = tmp->privateAugOmHandle;
			if(ccbDone) {
				/* should be apply-uc or abort-uc which will close the om-handle */
				tmp->privateAugOmHandle = 0LL;
				tmp->privateAoHandle = 0LL;
			}
			//tmp->isCritical = false;
		}

		//osafassert(!(tmp->isCritical));
		rs = 1;
	}

	return rs;
}

int imma_oi_ccb_record_note_callback(IMMA_CLIENT_NODE *cl_node, SaImmOiCcbIdT ccbId, 
	IMMA_CALLBACK_INFO *callback)
{
	/* Stores pointer to callback record in oi_ccb_record, but should only
	   be done for ccb create/delete/modify upcalls and only for main OI 
	   (not appliers), since otherwise the ccb can not be augmented.
	   The pointer is needed for the augmented ccb dowcalls. These 
	   downcalls are in essence incremental pre-replies on an on-going
	   oi-upcall (create/delete/modify). The augmenting "sub ccb" must be
	   terminated before returning from the oi-upcall. 
	 */

	int rs = 0;
	struct imma_oi_ccb_record *tmp = imma_oi_ccb_record_find(cl_node, ccbId);

	if(tmp && !(tmp->isAborted)) {
		tmp->ccbCallback = callback;
		if(callback) {
			tmp->isCcbAugOk = true;
			osafassert(!(tmp->isCritical));
			osafassert(!(cl_node->isApplier));
			rs = 1;
		}
	}

	return rs;
}

/****************************************************************************
  Name          : imma_mark_clients_stale
  Description   : If we have lost contact with IMMND over MDS, then mark all
                  handles stale. In addition, if handle is exposed
                  (i.e. not possible to resurrect) AND has a selection object,
                  then dispatch on the selection object NOW generating
                  BAD_HANDLE.
  Arguments     : IMMA_CB *cb - IMMA Control Block.
                : mark_exposed - true => not only stale mark, but expose-mark also
                  This is used when imma detects MDS message loss. 
                  Since imma can not determine which handle the lost message 
                  was destined for, it has to expose-mark all existing handles.
******************************************************************************/
void imma_mark_clients_stale(IMMA_CB *cb, bool mark_exposed)
{
	/* We are LOCKED already */
	IMMA_CLIENT_NODE *clnode = NULL;
	SaImmHandleT temp_hdl = 0LL;
	SaImmHandleT *temp_ptr = NULL;

	IMMA_CCB_NODE *ccb_node = NULL;
	SaImmCcbHandleT ccb_temp_hdl = 0LL;
	SaImmCcbHandleT *ccb_temp_ptr = NULL;

	struct imma_oi_ccb_record *oiCcb = NULL;

	IMMA_SEARCH_NODE *search_node = NULL;
	SaImmSearchHandleT search_tmp_hdl = 0LL;
	SaImmSearchHandleT *search_tmp_ptr = NULL;

	TRACE_ENTER();

	/* scan the entire handle db & mark each record */
	while ((clnode = (IMMA_CLIENT_NODE *)
			   ncs_patricia_tree_getnext(&cb->client_tree, (uint8_t *)temp_ptr))) {
		temp_hdl = clnode->handle;
		temp_ptr = &temp_hdl;

		/* Process OM side CCBs */
		while ((ccb_node = (IMMA_CCB_NODE *)
				   ncs_patricia_tree_getnext(&cb->ccb_tree, (uint8_t *)ccb_temp_ptr))) {
			ccb_temp_hdl = ccb_node->ccb_hdl;
			ccb_temp_ptr = &ccb_temp_hdl;

			if ((ccb_node->mImmHandle == clnode->handle) &&
				!(ccb_node->mApplying) && !(ccb_node->mApplied) &&
				!(ccb_node->mAborted)) {
				TRACE("CCb:%u for handle 0x%llx aborted in non critical state", 
					ccb_node->mCcbId, clnode->handle);
				ccb_node->mAborted = true;
			}
		}

		/* Process OI side CCBs */
		oiCcb = clnode->activeOiCcbs;
		while (oiCcb) {
			oiCcb->isStale = true;
			oiCcb = oiCcb->next;
		}

		/* Invalidate search handles */
		while ((search_node = (IMMA_SEARCH_NODE *)
				   ncs_patricia_tree_getnext(&cb->search_tree, (uint8_t *)search_tmp_ptr))) {
			search_tmp_hdl = search_node->search_hdl;
			search_tmp_ptr = &search_tmp_hdl;
			if ((search_node->mImmHandle == clnode->handle) &&
				search_node->mSearchId) {
				TRACE("Search id %u for handle %llx closed for stale imm-handle",
					search_node->mSearchId,  clnode->handle);
				search_node->mSearchId = 0;
			}
		}
		
		if (clnode->exposed) {continue;} /* No need to stale dispatched on this
						    handle when already exposed. */
		if(mark_exposed) {
			clnode->exposed = true;
			LOG_WA("marking handle as exposed");
		}

		clnode->stale = true;
		TRACE("Stale marked client cl:%u node:%x",
			m_IMMSV_UNPACK_HANDLE_HIGH(clnode->handle),
			m_IMMSV_UNPACK_HANDLE_LOW(clnode->handle));
		if (isExposed(cb, clnode)  && clnode->selObjUsable) {
			/* If we have a selection object, then we assume someone is
			   dispatching on it. If the connection is already exposed, then
			   inform the user (dispatch on it) now.
			   This will cause a BAD_HANDLE return from the dispatch.
			   If the user is not dispatching on the selection object,
			   then the client will remain stale marked, until the user
			   uses the handle in some way.
			*/
			imma_proc_stale_dispatch(cb, clnode);
		}
	}


    TRACE_LEAVE();
}

/****************************************************************************
  Name          : imma_process_stale_clients
  Description   : We have lost contact (IMMND DOWN earlier) with IMMND, 
                  then all handles are stale.  Now we have regained contact
                  (IMMND UP). For handles that are NOT exposed
                  (i.e. possible to resurrect) and that have a selection
                  object, dispatch on the selection object. The dispatch tries
                  to resurrect the client handle. If resurrection fails then we
                  fall back on the BAD_HANDLE solution, exposing to the client
                  by exiting their dispatch.
  Arguments     : IMMA_CB *cb - IMMA Control Block.
******************************************************************************/
void imma_process_stale_clients(IMMA_CB *cb)
{
	/* We are LOCKED already */
	IMMA_CLIENT_NODE  * clnode;
	SaImmHandleT *temp_ptr=0;
	SaImmHandleT temp_hdl=0;

	TRACE_ENTER();

	/* scan the entire handle db & check each record */
	while ((clnode = (IMMA_CLIENT_NODE *)
		       ncs_patricia_tree_getnext(&cb->client_tree, (uint8_t *)temp_ptr)))
	{
		temp_hdl = clnode->handle;
		temp_ptr = &temp_hdl;
		if (!clnode->stale) {continue;}
		TRACE("Stale client to process cl:%u node:%x exposed:%u",
			m_IMMSV_UNPACK_HANDLE_HIGH(clnode->handle),
			m_IMMSV_UNPACK_HANDLE_LOW(clnode->handle),
			clnode->exposed);

		if (clnode->selObjUsable) {
			/* If we have a selection object, then we assume someone is
			   dispatching on it. Inform the user (dispatch on it) now. 
			   This will cause either a resurrection or a BAD_HANDLE 
			   return from the dispatch. If the user is not dispatching
			   on the selection object, then the client will remain stale
			   marked, until the user uses the handle in some way. 
			*/
			imma_proc_stale_dispatch(cb, clnode);
		}
	}

	TRACE_LEAVE();
}
        
/****************************************************************************
  Name          : isExposed
  Description   : Determine if a client is already, or SHOULD be exposed.
                  If the client has already seen the handle as bad, then
                  we can not resurrect it (actively or reactively).
  Arguments     : IMMA_CB *cb - IMMA Control Block.
                  IMMA_CLIENT_NODE  *clnode a client node.
******************************************************************************/
int isExposed(IMMA_CB *cb, IMMA_CLIENT_NODE  *clnode)
{
	/* Must be LOCKED. */
	TRACE_ENTER();
	if (!clnode->exposed) {	/*If we are already exposed then skip checks*/
		/* Check if we are currently blocked => exposed. 
		   In the future we could for some cases avoid declaring exposure,
		   despite being blocked. 

		   A trivial example is handle finalize calls, which should not reply
		   with BAD_HANDLE just because the handle turned stale during the call.
		   On the other hand you dont want to resurrect such handles, which are being
		   finalized!.

		   Another example would be CcbApply. if we implement ccb recovery then
		   the imm-handle could in some cases be resurrected.
		   
		*/
	
		/* Check of replyPending moved forward to imma_proc_resurrect_client (imma_proc.c)
		   That is, we only get exposed if some thread tries to resurrect during the 
		   blocked call. 
		   if (clnode->replyPending) {
		       clnode->exposed = true;
		   }	
		*/

		if (clnode->isOm) {	
			/* Check for associated admin owners. If releaseOnFinalize is set
			   for anyone, then this handle is doomed, i.e. exposed. */
			IMMA_ADMIN_OWNER_NODE *adm_node = NULL;
			SaImmAdminOwnerHandleT temp_hdl = 0LL;
			SaImmAdminOwnerHandleT *temp_ptr = NULL;
			TRACE("OM CLIENT");

			while ((adm_node = (IMMA_ADMIN_OWNER_NODE *)
				       ncs_patricia_tree_getnext(&cb->admin_owner_tree, (uint8_t *)temp_ptr))) {
				temp_hdl = adm_node->admin_owner_hdl;
				temp_ptr = &temp_hdl;

				if ((adm_node->mImmHandle == clnode->handle) && 
					(adm_node->mReleaseOnFinalize)) {
					TRACE_3("Client gets exposed because releaseOnFinalize set for admin owner");
					clnode->exposed = true;
				}
			}
			if(clnode->isAug) {
				TRACE_3("Will not resurrect Om handle that is internal to OI augmented CCB");
				/* Prevent internal OI CCB augment OM handle from being resurrected.
				   If the OI has lost contact with IMMND then the CCB is in any case
				   terminated. See http://devel.opensaf.org/ticket/1963
				*/
				clnode->exposed = true;
			}
		} else { /* OI client. */
			TRACE("OI CLIENT");
			if(clnode->isApplier) {
				/* Prevent resurrect of applier handles.
				   See http://devel.opensaf.org/ticket/1933
				 */
				clnode->exposed = true;
			}
		}
	}
	TRACE("isExposed Returning Exposed:%u", clnode->exposed);
	TRACE_LEAVE();
	return clnode->exposed;
}

/****************************************************************************
  Name          : imma_admin_owner_tree_init
  Description   : This routine is used to initialize the Admin Owner Tree.
  Arguments     : cb - pointer to the IMMA Control Block
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
  Notes         : None
******************************************************************************/
uint32_t imma_admin_owner_tree_init(IMMA_CB *cb)
{
	NCS_PATRICIA_PARAMS param;
	memset(&param, 0, sizeof(NCS_PATRICIA_PARAMS));
	param.key_size = sizeof(SaImmAdminOwnerHandleT);
	if (ncs_patricia_tree_init(&cb->admin_owner_tree, &param) != NCSCC_RC_SUCCESS) {
		return NCSCC_RC_FAILURE;
	}
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
  Name          : imma_admin_owner_node_get
  Description   : This routine finds the Admin Owner node.
  Arguments     : admin_owner_tree - Admin Owner Tree.
                  adm_hdl - Admin Owner Handle
  Return Values : adm_node - Admin Owner Node
                  NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
  Notes         : The caller takes the cb lock before calling this function                 
******************************************************************************/
void imma_admin_owner_node_get(NCS_PATRICIA_TREE *admin_owner_tree,
	              SaImmAdminOwnerHandleT *adm_hdl, IMMA_ADMIN_OWNER_NODE **adm_node)
{
	*adm_node = (IMMA_ADMIN_OWNER_NODE *)
	    ncs_patricia_tree_get(admin_owner_tree, (uint8_t *)adm_hdl);
}

/******************************************************************************
  Name          : imma_admin_owner_node_getnext
  Description   : This routine will find the next Admin Owner Node
  Arguments     : admin_owner_tree - Admin Owner Tree.
                  adm_hdl - Admin Owner Handle
  Return Values :
 
  Notes
******************************************************************************/
void imma_admin_owner_node_getnext(IMMA_CB *cb, SaImmAdminOwnerHandleT *adm_hdl, IMMA_ADMIN_OWNER_NODE **adm_node)
{
	if (adm_hdl) {
		*adm_node = (IMMA_ADMIN_OWNER_NODE *)
		    ncs_patricia_tree_getnext(&cb->admin_owner_tree, (uint8_t *)adm_hdl);
	} else {
		*adm_node = (IMMA_ADMIN_OWNER_NODE *)
		    ncs_patricia_tree_getnext(&cb->admin_owner_tree, (uint8_t *)NULL);
	}

	return;
}

/****************************************************************************
  Name          : imma_admin_owner_node_add
  Description   : This routine adds the new node to admin_owner_tree.
  Arguments     : admin_owner_tree - Admin Owner Tree.
                  adm_node - Admin Owner Node.
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
  Notes         : The caller takes the cb lock before calling this function                  
******************************************************************************/
uint32_t imma_admin_owner_node_add(NCS_PATRICIA_TREE *admin_owner_tree, IMMA_ADMIN_OWNER_NODE *adm_node)
{
	adm_node->patnode.key_info = (uint8_t *)&adm_node->admin_owner_hdl;

	if (ncs_patricia_tree_add(admin_owner_tree, &adm_node->patnode) != NCSCC_RC_SUCCESS) {
		return NCSCC_RC_FAILURE;
	}

	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
  Name          : imma_admin_owner_node_delete
  Description   : This routine deletes the Admin Owner node from tree
  Arguments     : IMMA_CB *cb - IMMA Control Block.
                : IMMA_ADMIN_OWNERNODE *adm_node - Admin Owner Node.
  Notes         : None
******************************************************************************/
void imma_admin_owner_node_delete(IMMA_CB *cb, IMMA_ADMIN_OWNER_NODE *adm_node)
{
	SaImmCcbHandleT ccb_temp_hdl, *ccb_temp_ptr = NULL;
	IMMA_CCB_NODE *ccb_node = NULL;

	osafassert(adm_node);

	/* Remove any ccb nodes opened by the client using this admin-owner */
	while ((ccb_node = (IMMA_CCB_NODE *)
		ncs_patricia_tree_getnext(&cb->ccb_tree, (uint8_t *)ccb_temp_ptr))) {
		ccb_temp_hdl = ccb_node->ccb_hdl;
		ccb_temp_ptr = &ccb_temp_hdl;

		if (ccb_node->mAdminOwnerHdl == adm_node->admin_owner_hdl) {
			if (ccb_node->mExclusive) {
				TRACE("imma_admin_owner_node_delete: associated ccb (%u) in exclusive mode"
					  " - ccb is orphaned.", ccb_node->mCcbId);
			} else  {
				TRACE("Deleting ccb node");
				osafassert(imma_ccb_node_delete(cb, ccb_node) == NCSCC_RC_SUCCESS);
				ccb_temp_ptr = NULL;	/*Redo iteration from start after delete. */
			}
		}
	}

	/* Remove the Node from the tree */
	osafassert(ncs_patricia_tree_del(&cb->admin_owner_tree, &adm_node->patnode) == NCSCC_RC_SUCCESS);

	if (adm_node->mAdminOwnerName) {
		free(adm_node->mAdminOwnerName);
		adm_node->mAdminOwnerName = NULL;
		adm_node->mAdminOwnerId = 0;
	}

	free(adm_node);
}

/****************************************************************************
  Name          : imma_admin_owner_tree_cleanup
  Description   : This routine destroys the nodes in the IMMA admin owner tree.
  Arguments     : IMMA_CB *cb - IMMA Control Block.
  Return Values : None
  Notes         : None
******************************************************************************/
void imma_admin_owner_tree_cleanup(IMMA_CB *cb)
{
	SaImmAdminOwnerHandleT prev_adm_owner_id = 0;
	IMMA_ADMIN_OWNER_NODE *adm_node;

	/* Get the First Node */
	adm_node = (IMMA_ADMIN_OWNER_NODE *)
	    ncs_patricia_tree_getnext(&cb->admin_owner_tree, (uint8_t *)&prev_adm_owner_id);
	while (adm_node) {
		prev_adm_owner_id = adm_node->admin_owner_hdl;
		imma_admin_owner_node_delete(cb, adm_node);
		adm_node = (IMMA_ADMIN_OWNER_NODE *)
		    ncs_patricia_tree_getnext(&cb->admin_owner_tree, (uint8_t *)&prev_adm_owner_id);
	}
	return;
}

/****************************************************************************
  Name          : imma_admin_owner_tree_destroy
  Description   : This routine destroys the IMMA admin owner tree.
  Arguments     : IMMA_CB *cb - IMMA Control Block.
  Return Values : None
  Notes         : None
******************************************************************************/
void imma_admin_owner_tree_destroy(IMMA_CB *cb)
{
	/* cleanup the admin owner tree */
	imma_admin_owner_tree_cleanup(cb);

	/* destroy the tree */
	ncs_patricia_tree_destroy(&cb->admin_owner_tree);

	return;
}

/****************************************************************************
  Name          : imma_ccb_tree_init
  Description   : This routine is used to initialize the Ccb Tree.
  Arguments     : cb - pointer to the IMMA Control Block
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
  Notes         : None
******************************************************************************/
uint32_t imma_ccb_tree_init(IMMA_CB *cb)
{
	NCS_PATRICIA_PARAMS param;
	memset(&param, 0, sizeof(NCS_PATRICIA_PARAMS));
	param.key_size = sizeof(SaImmCcbHandleT);
	if (ncs_patricia_tree_init(&cb->ccb_tree, &param) != NCSCC_RC_SUCCESS) {
		return NCSCC_RC_FAILURE;
	}
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
  Name          : imma_ccb_node_get
  Description   : This routine finds the Ccb node.
  Arguments     : ccb_tree - Ccb Tree.
                  adm_hdl - Ccb Handle
  Return Values : ccb_node - Ccb Node
  Notes         : The caller takes the cb lock before calling this function                 
******************************************************************************/
void imma_ccb_node_get(NCS_PATRICIA_TREE *ccb_tree, SaImmCcbHandleT *ccb_hdl, IMMA_CCB_NODE **ccb_node)
{
	*ccb_node = (IMMA_CCB_NODE *)
		ncs_patricia_tree_get(ccb_tree, (uint8_t *)ccb_hdl);
}

/******************************************************************************
  Name          : imma_ccb_node_getnext
  Description   : This routine will find the next Ccb Node
  Arguments     : ccb_tree - Ccb Tree.
                  ccb_hdl - Ccb Handle
  Return Values :
 
  Notes
******************************************************************************/
void imma_ccb_node_getnext(IMMA_CB *cb, SaImmCcbHandleT *ccb_hdl, IMMA_CCB_NODE **ccb_node)
{
	if (ccb_hdl) {
		*ccb_node = (IMMA_CCB_NODE *)
		    ncs_patricia_tree_getnext(&cb->ccb_tree, (uint8_t *)ccb_hdl);
	} else {
		*ccb_node = (IMMA_CCB_NODE *)
		    ncs_patricia_tree_getnext(&cb->ccb_tree, (uint8_t *)NULL);
	}

	return;
}

/****************************************************************************
  Name          : imma_ccb_node_add
  Description   : This routine adds the new node to ccb_tree.
  Arguments     : ccb_tree - Ccb Tree.
                  ccb_node - Ccb Node.
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
  Notes         : The caller takes the cb lock before calling this function
******************************************************************************/
uint32_t imma_ccb_node_add(NCS_PATRICIA_TREE *ccb_tree, IMMA_CCB_NODE *ccb_node)
{
	ccb_node->patnode.key_info = (uint8_t *)&ccb_node->ccb_hdl;

	if (ncs_patricia_tree_add(ccb_tree, &ccb_node->patnode) != NCSCC_RC_SUCCESS) {
		return NCSCC_RC_FAILURE;
	}

	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
  Name          : imma_ccb_node_delete
  Description   : This routine deletes the CCb node from tree
  Arguments     : IMMA_CB *cb - IMMA Control Block.
                : IMMA_CC_BNODE *ccb_node - Ccb Node.
  Notes         : None
******************************************************************************/
uint32_t imma_ccb_node_delete(IMMA_CB *cb, IMMA_CCB_NODE *ccb_node)
{
	uint32_t rc = NCSCC_RC_SUCCESS;

	if (ccb_node == NULL)
		return NCSCC_RC_FAILURE;

	/* Remove the Node from the tree */
	if (ncs_patricia_tree_del(&cb->ccb_tree, &ccb_node->patnode) != NCSCC_RC_SUCCESS) {
		TRACE("Failure in deleting ccb_node! handle %llx ccbid %u",
			ccb_node->ccb_hdl, ccb_node->mCcbId);
		rc = NCSCC_RC_FAILURE;
	}

	/* Remove error-strings array */
	imma_free_errorStrings(ccb_node->mErrorStrings);
	ccb_node->mErrorStrings = NULL;

	TRACE("Freeing ccb_node handle %llx ccbid %u", ccb_node->ccb_hdl, ccb_node->mCcbId);
	free(ccb_node);

	return rc;
}

/****************************************************************************
  Name          : imma_ccb_tree_cleanup
  Description   : This routine destroys the nodes in the IMMA ccb tree.
  Arguments     : IMMA_CB *cb - IMMA Control Block.
  Return Values : None
  Notes         : None
******************************************************************************/
void imma_ccb_tree_cleanup(IMMA_CB *cb)
{
	SaImmCcbHandleT prev_ccb_id = 0;
	IMMA_CCB_NODE *ccb_node;

	/* Get the First Node */
	ccb_node = (IMMA_CCB_NODE *)
	    ncs_patricia_tree_getnext(&cb->ccb_tree, (uint8_t *)&prev_ccb_id);
	while (ccb_node) {
		prev_ccb_id = ccb_node->ccb_hdl;
		imma_ccb_node_delete(cb, ccb_node);
		ccb_node = (IMMA_CCB_NODE *)
		    ncs_patricia_tree_getnext(&cb->ccb_tree, (uint8_t *)&prev_ccb_id);
	}
	return;
}

/****************************************************************************
  Name          : imma_ccb_tree_destroy
  Description   : This routine destroys the IMMA ccb tree.
  Arguments     : IMMA_CB *cb - IMMA Control Block.
  Return Values : None
  Notes         : None
******************************************************************************/
void imma_ccb_tree_destroy(IMMA_CB *cb)
{
	/* cleanup the ccb tree */
	imma_ccb_tree_cleanup(cb);

	/* destroy the tree */
	ncs_patricia_tree_destroy(&cb->ccb_tree);

	return;
}

/****************************************************************************
  Name          : imma_search_tree_init
  Description   : This routine is used to initialize the Search Tree.
  Arguments     : cb - pointer to the IMMA Control Block
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
  Notes         : None
******************************************************************************/
uint32_t imma_search_tree_init(IMMA_CB *cb)
{
	NCS_PATRICIA_PARAMS param;
	memset(&param, 0, sizeof(NCS_PATRICIA_PARAMS));
	param.key_size = sizeof(SaImmSearchHandleT);
	if (ncs_patricia_tree_init(&cb->search_tree, &param) != NCSCC_RC_SUCCESS) {
		return NCSCC_RC_FAILURE;
	}
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
  Name          : imma_search_node_get
  Description   : This routine finds the Search node.
  Arguments     : search_tree - Search Tree.
                  search_hdl - Search Handle
  Return Values : search_node - Search Node
                  NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
  Notes         : The caller takes the cb lock before calling this function 
******************************************************************************/
void imma_search_node_get(NCS_PATRICIA_TREE *search_tree,
			   SaImmSearchHandleT *search_hdl, IMMA_SEARCH_NODE **search_node)
{
	*search_node = (IMMA_SEARCH_NODE *)
	    ncs_patricia_tree_get(search_tree, (uint8_t *)search_hdl);
}

/******************************************************************************
  Name          : imma_search_node_getnext
  Description   : This routine will find the next Search Node
  Arguments     : search_tree - Search Tree.
                  search_hdl - Search Handle
  Return Values :
 
  Notes
******************************************************************************/
void imma_search_node_getnext(IMMA_CB *cb, SaImmSearchHandleT *search_hdl, IMMA_SEARCH_NODE **search_node)
{
	if (search_hdl) {
		*search_node = (IMMA_SEARCH_NODE *)
		    ncs_patricia_tree_getnext(&cb->search_tree, (uint8_t *)search_hdl);
	} else {
		*search_node = (IMMA_SEARCH_NODE *)
		    ncs_patricia_tree_getnext(&cb->search_tree, (uint8_t *)NULL);
	}

	return;
}

/****************************************************************************
  Name          : imma_search_node_add
  Description   : This routine adds the new node to search_tree.
  Arguments     : search_tree - Search Tree.
                  search_node - Search Node.
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
  Notes         : The caller takes the cb lock before calling this function
******************************************************************************/
uint32_t imma_search_node_add(NCS_PATRICIA_TREE *search_tree, IMMA_SEARCH_NODE *search_node)
{
	search_node->patnode.key_info = (uint8_t *)&search_node->search_hdl;

	if (ncs_patricia_tree_add(search_tree, &search_node->patnode) != NCSCC_RC_SUCCESS) {
		return NCSCC_RC_FAILURE;
	}

	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
  Name          : imma_search_node_delete
  Description   : This routine deletes the Search node from tree
  Arguments     : IMMA_CB *cb - IMMA Control Block.
                : IMMA_SEARCH_NODE *search_node - Search Node.
  Notes         : None
******************************************************************************/
uint32_t imma_search_node_delete(IMMA_CB *cb, IMMA_SEARCH_NODE *search_node)
{
	uint32_t rc = NCSCC_RC_SUCCESS;

	if (search_node == NULL)
		return NCSCC_RC_FAILURE;

	/* Remove the Node from the tree */
	if (ncs_patricia_tree_del(&cb->search_tree, &search_node->patnode) != NCSCC_RC_SUCCESS) {
		rc = NCSCC_RC_FAILURE;
	}

	/* Free the Node */
	if (search_node) {
		free(search_node);
	}

	return rc;
}

/****************************************************************************
  Name          : imma_search_tree_cleanup
  Description   : This routine destroys the nodes in the IMMA search tree.
  Arguments     : IMMA_CB *cb - IMMA Control Block.
  Return Values : None
  Notes         : None
******************************************************************************/
void imma_search_tree_cleanup(IMMA_CB *cb)
{
	SaImmSearchHandleT prev_search_id = 0;
	IMMA_SEARCH_NODE *search_node;

	/* Get the First Node */
	search_node = (IMMA_SEARCH_NODE *)
	    ncs_patricia_tree_getnext(&cb->search_tree, (uint8_t *)&prev_search_id);

	while (search_node) {
		prev_search_id = search_node->search_hdl;
		imma_search_node_delete(cb, search_node);
		search_node = (IMMA_SEARCH_NODE *)
		    ncs_patricia_tree_getnext(&cb->search_tree, (uint8_t *)&prev_search_id);
	}
	return;
}

/****************************************************************************
  Name          : imma_search_tree_destroy
  Description   : This routine destroys the IMMA search tree.
  Arguments     : IMMA_CB *cb - IMMA Control Block.
  Return Values : None
  Notes         : None
******************************************************************************/
void imma_search_tree_destroy(IMMA_CB *cb)
{
	/* cleanup the search tree */
	imma_search_tree_cleanup(cb);

	/* destroy the tree */
	ncs_patricia_tree_destroy(&cb->search_tree);

	return;
}

/****************************************************************************
  Name          : imma_db_init 
  Description   : This routine initializes the IMMA CB database.
  Arguments     : IMMA_CB *cb - IMMA Control Block.
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_error
  Notes         : None
******************************************************************************/
uint32_t imma_db_init(IMMA_CB *cb)
{
	uint32_t rc;

	rc = imma_client_tree_init(cb);
	if (rc != NCSCC_RC_SUCCESS) {
		TRACE_3("imma_client_tree_init failed");
		return rc;
	}

	rc = imma_admin_owner_tree_init(cb);
	if (rc != NCSCC_RC_SUCCESS) {
		TRACE_3("imma_admin_owner_tree_init failed");
		return rc;
	}

	rc = imma_ccb_tree_init(cb);
	if (rc != NCSCC_RC_SUCCESS) {
		TRACE_3("imma_ccb_tree_init failed");
		return rc;
	}

	rc = imma_search_tree_init(cb);
	if (rc != NCSCC_RC_SUCCESS) {
		TRACE_3("imma_search_tree_init failed");
		return rc;
	}

	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
  Name          : imma_db_destroy 
  Description   : This routine will destroy the IMMA CB database.
  Arguments     : IMMA_CB *cb - IMMA Control Block.
  Return Values : NCSCC_RC_SUCCESS
  Notes         : None
******************************************************************************/
uint32_t imma_db_destroy(IMMA_CB *cb)
{
	TRACE_ENTER();

	imma_client_tree_destroy(cb);

	imma_admin_owner_tree_destroy(cb);

	imma_ccb_tree_destroy(cb);

	imma_search_tree_destroy(cb);
	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

void imma_free_errorStrings(SaStringT* errorStrings)
{
	if(errorStrings) {
		unsigned int ix = 0;
		for(;errorStrings[ix];++ix) {
			free(errorStrings[ix]);
			errorStrings[ix] = NULL;
		}
		free(errorStrings);
	}
	
}

SaStringT* imma_getErrorStrings(IMMSV_SAERR_INFO* errRsp)
{
	unsigned int listSize = 0;
	SaStringT* errStringArr=NULL;
	if(errRsp->errStrings == NULL) {goto done;}

	IMMSV_ATTR_NAME_LIST* errStrs = errRsp->errStrings;

	while(errStrs) {
		++listSize;
		errStrs = errStrs->next;
	}

	errStringArr = calloc(listSize + 1, sizeof(SaStringT));

	errStrs = errRsp->errStrings;
        listSize = 0;
	while(errStrs) {
		errStringArr[listSize] = (SaStringT) errStrs->name.buf;
		errStrs->name.buf = NULL;
		errStrs->name.size = 0;
		errStrs = errStrs->next;
		++listSize;
	}

        errStringArr[listSize] = NULL;

	immsv_evt_free_attrNames(errRsp->errStrings);
	errRsp->errStrings = NULL;
	
 done:
	return errStringArr;
}
