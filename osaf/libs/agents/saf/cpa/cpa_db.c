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
  
  This file contains the CPA processing routines (Routines to add, delete nodes
  from CPA data bases, callback processing routines etc.)
    
******************************************************************************
*/

#include "cpa.h"
/****************************************************************************
  Name          : cpa_client_tree_init
  Description   : This routine is used to initialize the client tree
  Arguments     : cb - pointer to the CPA Control Block
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
  Notes         : None
******************************************************************************/
uns32 cpa_client_tree_init(CPA_CB *cb)
{
	NCS_PATRICIA_PARAMS param;
	memset(&param, 0, sizeof(NCS_PATRICIA_PARAMS));
	param.key_size = sizeof(SaCkptHandleT);
	if (ncs_patricia_tree_init(&cb->client_tree, &param) != NCSCC_RC_SUCCESS) {
		return NCSCC_RC_FAILURE;
	}
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
  Name          : cpa_client_node_get
  Description   : This routine finds the client node.
  Arguments     : client_tree - Client Tree.
                  cl_hdl - Client Handle
  Return Values : cl_node - Client Node
                  NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
  Notes         : The caller takes the cb lock before calling this function                 
******************************************************************************/
uns32 cpa_client_node_get(NCS_PATRICIA_TREE *client_tree, SaCkptHandleT *cl_hdl, CPA_CLIENT_NODE **cl_node)
{
	*cl_node = (CPA_CLIENT_NODE *)
	    ncs_patricia_tree_get(client_tree, (uns8 *)cl_hdl);

	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
  Name          : cpa_client_node_add
  Description   : This routine adds the new client to the client tree.
  Arguments     :client_tree - Client Tree.
                 cl_node - Client Nod
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
  Notes         : The caller takes the cb lock before calling this function                  
******************************************************************************/
uns32 cpa_client_node_add(NCS_PATRICIA_TREE *client_tree, CPA_CLIENT_NODE *cl_node)
{
	uns32 rc = NCSCC_RC_FAILURE;
	/* Store the client_info pointer as msghandle. */
	cl_node->patnode.key_info = (uns8 *)&cl_node->cl_hdl;

	if ((rc = ncs_patricia_tree_add(client_tree, &cl_node->patnode)) != NCSCC_RC_SUCCESS) {
		return NCSCC_RC_FAILURE;
	}

	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
  Name          : cpa_client_node_delete
  Description   : This routine deletes the client from the client tree
  Arguments     : CPA_CB *cb - CPA Control Block.
                : CPA_CLIENT_NODE *cl_node - Client Node.
  Return Values : None
  Notes         : None
******************************************************************************/
uns32 cpa_client_node_delete(CPA_CB *cb, CPA_CLIENT_NODE *cl_node)
{
	uns32 rc = NCSCC_RC_SUCCESS;

	if (cl_node == NULL)
		return NCSCC_RC_FAILURE;

	/* Remove the Node from the client tree */
	if (ncs_patricia_tree_del(&cb->client_tree, &cl_node->patnode) != NCSCC_RC_SUCCESS) {
		rc = NCSCC_RC_FAILURE;
	}

	/* Free the Client Node */
	if (cl_node)
		m_MMGR_FREE_CPA_CLIENT_NODE(cl_node);

	return rc;

}

/****************************************************************************
  Name          : cpa_client_tree_destroy
  Description   : This routine destroys the CPA client tree.
  Arguments     : CPA_CB *cb - CPA Control Block.
  Return Values : None
  Notes         : None
******************************************************************************/
void cpa_client_tree_destroy(CPA_CB *cb)
{
	/* cleanup the client tree */
	cpa_client_tree_cleanup(cb);

	/* destroy the tree */
	ncs_patricia_tree_destroy(&cb->client_tree);

	return;
}

/****************************************************************************
  Name          : cpa_client_tree_cleanup
  Description   : This routine cleansup the CPA Client tree
  Arguments     : CPA_CB *cb - CPA Control Block.
  Return Values : None
  Notes         : None
******************************************************************************/
void cpa_client_tree_cleanup(CPA_CB *cb)
{
	CPA_CLIENT_NODE *clnode = NULL;
	SaCkptHandleT *temp_ptr = NULL;
	SaCkptHandleT temp_hdl = 0;

	/* scan the entire handle db & delete each record */
	while ((clnode = (CPA_CLIENT_NODE *)
		ncs_patricia_tree_getnext(&cb->client_tree, (uns8 *)temp_ptr))) {
		/* delete the client info */
		temp_hdl = clnode->cl_hdl;
		temp_ptr = &temp_hdl;

		/* Destroy the IPC attached to this client */
		cpa_callback_ipc_destroy(clnode);

		/* Delete the Client Node */
		cpa_client_node_delete(cb, clnode);
	}
	return;
}

/****************************************************************************
  Name          : cpa_lcl_ckpt_tree_init
  Description   : This routine is used to initialize the Local Checkpoint 
                  Tree.
  Arguments     : cb - pointer to the CPA Control Block
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
  Notes         : None
******************************************************************************/
uns32 cpa_lcl_ckpt_tree_init(CPA_CB *cb)
{
	NCS_PATRICIA_PARAMS param;
	memset(&param, 0, sizeof(NCS_PATRICIA_PARAMS));
	param.key_size = sizeof(SaCkptCheckpointHandleT);
	if (ncs_patricia_tree_init(&cb->lcl_ckpt_tree, &param) != NCSCC_RC_SUCCESS) {
		return NCSCC_RC_FAILURE;
	}
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
  Name          : cpa_lcl_ckpt_node_get
  Description   : This routine finds the Local checkpoint node.
  Arguments     : lcl_ckpt_tree - Local Ckpt Tree.
                  lc_hdl - Local checkpoint Handle
  Return Values : lc_node - Local checkpoint Node
                  NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
  Notes         : The caller takes the cb lock before calling this function                 
******************************************************************************/
uns32 cpa_lcl_ckpt_node_get(NCS_PATRICIA_TREE *lcl_ckpt_tree,
			    SaCkptCheckpointHandleT *lc_hdl, CPA_LOCAL_CKPT_NODE **lc_node)
{
	*lc_node = (CPA_LOCAL_CKPT_NODE *)
	    ncs_patricia_tree_get(lcl_ckpt_tree, (uns8 *)lc_hdl);

	return NCSCC_RC_SUCCESS;
}

/******************************************************************************
  Name          : cpa_lcl_ckpt_node_getnext
  Description   : This routine will find the next Local checkpoint node
  Arguments     : lcl_ckpt_tree - Local Ckpt Tree.
                  lc_hdl - Local checkpoint Handle
  Return Values :
 
  Notes
******************************************************************************/
void cpa_lcl_ckpt_node_getnext(CPA_CB *cb, SaCkptCheckpointHandleT *lc_hdl, CPA_LOCAL_CKPT_NODE **lc_node)
{
	if (lc_hdl)
		*lc_node = (CPA_LOCAL_CKPT_NODE *)ncs_patricia_tree_getnext(&cb->lcl_ckpt_tree, (uns8 *)lc_hdl);

	else
		*lc_node = (CPA_LOCAL_CKPT_NODE *)ncs_patricia_tree_getnext(&cb->lcl_ckpt_tree, (uns8 *)NULL);

	return;
}

/****************************************************************************
  Name          : cpa_lcl_ckpt_node_add
  Description   : This routine adds the new node to lcl_ckpt_tree.
  Arguments     : client_tree - Client Tree.
                  lc_node - Local checkpoint Node.
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
  Notes         : The caller takes the cb lock before calling this function                  
******************************************************************************/
uns32 cpa_lcl_ckpt_node_add(NCS_PATRICIA_TREE *lcl_ckpt_tree, CPA_LOCAL_CKPT_NODE *lc_node)
{
	/* Store the client_info pointer as msghandle. */
	lc_node->patnode.key_info = (uns8 *)&lc_node->lcl_ckpt_hdl;

	if (ncs_patricia_tree_add(lcl_ckpt_tree, &lc_node->patnode) != NCSCC_RC_SUCCESS) {
		/* Logging done by caller of this function */
		return NCSCC_RC_FAILURE;
	}

	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
  Name          : cpa_lcl_ckpt_node_delete
  Description   : This routine deletes the Local checkpoint node from tree
  Arguments     : CPA_CB *cb - CPA Control Block.
                : CPA_LOCAL_CKPT_NODE *lc_node - Local Ckeckpoint Node.
  Return Values : None
  Notes         : None
******************************************************************************/
uns32 cpa_lcl_ckpt_node_delete(CPA_CB *cb, CPA_LOCAL_CKPT_NODE *lc_node)
{
	uns32 rc = NCSCC_RC_SUCCESS;

	if (lc_node == NULL)
		return NCSCC_RC_FAILURE;

	/* Remove the Node from the client tree */
	if (ncs_patricia_tree_del(&cb->lcl_ckpt_tree, &lc_node->patnode) != NCSCC_RC_SUCCESS) {
		return NCSCC_RC_FAILURE;
	}

	/* Free the Client Node */
	if (lc_node) {
		/* Stop timer, if running */
		cpa_tmr_stop(&lc_node->async_req_tmr);

		if (lc_node->async_req_tmr.uarg) {
			ncshm_destroy_hdl(NCS_SERVICE_ID_CPA, lc_node->async_req_tmr.uarg);
		}

		m_MMGR_FREE_CPA_LOCAL_CKPT_NODE(lc_node);
	}

	return rc;

}

/****************************************************************************
  Name          : cpa_lcl_ckpt_tree_destroy
  Description   : This routine destroys the nodes in the CPA lcl ckpt tree.
  Arguments     : CPA_CB *cb - CPA Control Block.
  Return Values : None
  Notes         : None
******************************************************************************/
void cpa_lcl_ckpt_tree_cleanup(CPA_CB *cb)
{
	SaCkptCheckpointHandleT prev_ckpt_id = 0;
	CPA_LOCAL_CKPT_NODE *lc_node;

	/* Print the Lcl Checkpoint Details */
	if (cb->is_lcl_ckpt_tree_up) {
		/* Get the First Node */
		lc_node = (CPA_LOCAL_CKPT_NODE *)ncs_patricia_tree_getnext(&cb->lcl_ckpt_tree, (uns8 *)&prev_ckpt_id);
		while (lc_node) {
			prev_ckpt_id = lc_node->lcl_ckpt_hdl;

			cpa_lcl_ckpt_node_delete(cb, lc_node);

			lc_node = (CPA_LOCAL_CKPT_NODE *)ncs_patricia_tree_getnext(&cb->lcl_ckpt_tree,
										   (uns8 *)&prev_ckpt_id);
		}
	}
	return;
}

/****************************************************************************
  Name          : cpa_lcl_ckpt_tree_destroy
  Description   : This routine destroys the CPA lcl ckpt tree.
  Arguments     : CPA_CB *cb - CPA Control Block.
  Return Values : None
  Notes         : None
******************************************************************************/
void cpa_lcl_ckpt_tree_destroy(CPA_CB *cb)
{
	/* cleanup the client tree */
	cpa_lcl_ckpt_tree_cleanup(cb);

	/* destroy the tree */
	ncs_patricia_tree_destroy(&cb->lcl_ckpt_tree);

	return;
}

/****************************************************************************
  Name          : cpa_gbl_ckpt_tree_init
  
  Description   : This routine is used to initialize the Global Checkpoint 
                  Tree.
 
  Arguments     : cb - pointer to the CPA Control Block
                   
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None
******************************************************************************/
uns32 cpa_gbl_ckpt_tree_init(CPA_CB *cb)
{
	NCS_PATRICIA_PARAMS param;
	memset(&param, 0, sizeof(NCS_PATRICIA_PARAMS));
	param.key_size = sizeof(SaCkptCheckpointHandleT);
	if (ncs_patricia_tree_init(&cb->gbl_ckpt_tree, &param) != NCSCC_RC_SUCCESS) {
		return NCSCC_RC_FAILURE;
	}
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
  Name          : cpa_gbl_ckpt_node_find_add
  Description   : This routine finds the Global checkpoint node.
  Arguments     : gbl_ckpt_tree - Global Ckpt Tree.
                  gc_hdl - Local checkpoint Handle
  Return Values : gc_node - Local checkpoint Node
                  NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
  Notes         : The caller takes the cb lock before calling this function                 
******************************************************************************/
uns32 cpa_gbl_ckpt_node_find_add(NCS_PATRICIA_TREE *gbl_ckpt_tree,
				 SaCkptCheckpointHandleT *gc_hdl, CPA_GLOBAL_CKPT_NODE **gc_node, NCS_BOOL *add_flag)
{
	uns32 rc = NCSCC_RC_SUCCESS;

	*gc_node = (CPA_GLOBAL_CKPT_NODE *)
	    ncs_patricia_tree_get(gbl_ckpt_tree, (uns8 *)gc_hdl);

	if ((*gc_node == NULL) && (*add_flag == TRUE)) {
		/* Allocate the CPA_GLOBAL_CKPT_NODE & Populate gbl_ckpt_hdl */
		*gc_node = (CPA_GLOBAL_CKPT_NODE *)m_MMGR_ALLOC_CPA_GLOBAL_CKPT_NODE;
		if (*gc_node == NULL) {
			rc = NCSCC_RC_OUT_OF_MEM;
			/* Log not required, this will be logged by theh caller */
			return NCSCC_RC_FAILURE;
		}

		memset(*gc_node, 0, sizeof(CPA_GLOBAL_CKPT_NODE));
		(*gc_node)->gbl_ckpt_hdl = (SaCkptCheckpointHandleT)*gc_hdl;

		m_NCS_LOCK_INIT(&((*gc_node)->cpd_active_sync_lock));

		rc = cpa_gbl_ckpt_node_add(gbl_ckpt_tree, *gc_node);

		if (rc != NCSCC_RC_SUCCESS) {
			m_MMGR_FREE_CPA_GLOBAL_CKPT_NODE(*gc_node);
			*gc_node = NULL;
			/* Log not required, this will be logged by theh caller */
			return rc;
		}

		/* Toggle the add Flag */
		*add_flag = FALSE;
	}

	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
  Name          : cpa_gbl_ckpt_node_add
  Description   : This routine adds the new node to lcl_ckpt_tree.
  Arguments     : client_tree - Client Tree.
                  lc_node - Local checkpoint Node.
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
  Notes         : The caller takes the cb lock before calling this function                  
******************************************************************************/
uns32 cpa_gbl_ckpt_node_add(NCS_PATRICIA_TREE *gbl_ckpt_tree, CPA_GLOBAL_CKPT_NODE *gc_node)
{
	/* Store the client_info pointer as msghandle. */
	gc_node->patnode.key_info = (uns8 *)&gc_node->gbl_ckpt_hdl;

	if (ncs_patricia_tree_add(gbl_ckpt_tree, &gc_node->patnode) != NCSCC_RC_SUCCESS) {
		m_LOG_CPA_HEADLINE(CPA_GLOBAL_CKPT_NODE_ADD_FAILED, NCSFL_SEV_ERROR);
		return NCSCC_RC_FAILURE;
	}

	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
  Name          : cpa_sect_iter_tree_destroy
  Description   : This routine destroys the CPA Section Iteration tree.
  Arguments     : CPA_CB *cb - CPA Control Block.
  Return Values : None
  Notes         : None
******************************************************************************/
void cpa_sect_iter_tree_destroy(CPA_CB *cb)
{

	/* destroy the tree */
	ncs_patricia_tree_destroy(&cb->sect_iter_tree);

	return;
}

/****************************************************************************
  Name          : cpa_gbl_ckpt_node_delete
  Description   : This routine deletes the Local checkpoint node from tree
  Arguments     : CPA_CB *cb - CPA Control Block.
                : CPA_LOCAL_CKPT_NODE *gc_node - Global Ckeckpoint Node.
  Return Values : None
  Notes         : None
******************************************************************************/
uns32 cpa_gbl_ckpt_node_delete(CPA_CB *cb, CPA_GLOBAL_CKPT_NODE *gc_node)
{
	uns32 rc = NCSCC_RC_SUCCESS;

	if (gc_node == NULL)
		return NCSCC_RC_FAILURE;

	/* Remove the Node from the client tree */
	if (ncs_patricia_tree_del(&cb->gbl_ckpt_tree, &gc_node->patnode) != NCSCC_RC_SUCCESS) {
		rc = NCSCC_RC_FAILURE;
	}

	if (gc_node)
		m_MMGR_FREE_CPA_GLOBAL_CKPT_NODE(gc_node);

	return rc;

}

/****************************************************************************
  Name          : cpa_sect_iter_tree_init
  Description   : This routine is used to initialize the Section Iteration 
                  Tree.
  Arguments     : cb - pointer to the CPA Control Block
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
  Notes         : None
******************************************************************************/
uns32 cpa_sect_iter_tree_init(CPA_CB *cb)
{
	NCS_PATRICIA_PARAMS param;
	memset(&param, 0, sizeof(NCS_PATRICIA_PARAMS));
	param.key_size = sizeof(SaCkptSectionIterationHandleT);
	if (ncs_patricia_tree_init(&cb->sect_iter_tree, &param) != NCSCC_RC_SUCCESS) {
		return NCSCC_RC_FAILURE;
	}
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
  Name          : cpa_sect_iter_node_get
  Description   : This routine finds the Section Iteration Node.
  Arguments     : sect_iter_tree - Section Iteration Tree.
                  sect_iter_hdl - Section Iteration Handle
  Return Values : sect_iter_node - Section Iteration Node
                  NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
  Notes         : The caller takes the cb lock before calling this function                 
******************************************************************************/
uns32 cpa_sect_iter_node_get(NCS_PATRICIA_TREE *sect_iter_tree,
			     SaCkptSectionIterationHandleT *sect_iter_hdl, CPA_SECT_ITER_NODE **sect_iter_node)
{
	*sect_iter_node = (CPA_SECT_ITER_NODE *)
	    ncs_patricia_tree_get(sect_iter_tree, (uns8 *)sect_iter_hdl);

	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
  Name          : cpa_sect_iter_node_getnext
  Description   : This routine finds the Section Iteration Node.
  Arguments     : sect_iter_tree - Section Iteration Tree.
                  sect_iter_hdl - Section Iteration Handle
  Return Values : sect_iter_node - Section Iteration Node
                  NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
  Notes         : The caller takes the cb lock before calling this function                 
******************************************************************************/
void cpa_sect_iter_node_getnext(NCS_PATRICIA_TREE *sect_iter_tree,
				SaCkptSectionIterationHandleT *sect_iter_hdl, CPA_SECT_ITER_NODE **sect_iter_node)
{
	if (sect_iter_hdl)
		*sect_iter_node = (CPA_SECT_ITER_NODE *)
		    ncs_patricia_tree_getnext(sect_iter_tree, (uns8 *)sect_iter_hdl);
	else
		*sect_iter_node = (CPA_SECT_ITER_NODE *)
		    ncs_patricia_tree_getnext(sect_iter_tree, (uns8 *)NULL);
	return;
}

/****************************************************************************
  Name          : cpa_sect_iter_node_add
  Description   : This routine adds the new node to lcl_ckpt_tree.
  Arguments     : sect_iter_tree - Client Tree.
                  sect_iter_node - Section Iteration Node.
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
  Notes         : The caller takes the cb lock before calling this function                  
******************************************************************************/
uns32 cpa_sect_iter_node_add(NCS_PATRICIA_TREE *sect_iter_tree, CPA_SECT_ITER_NODE *sect_iter_node)
{
	/* Store the client_info pointer as msghandle. */
	sect_iter_node->patnode.key_info = (uns8 *)&sect_iter_node->iter_id;

	if (ncs_patricia_tree_add(sect_iter_tree, &sect_iter_node->patnode) != NCSCC_RC_SUCCESS) {
		return NCSCC_RC_FAILURE;
	}

	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
  Name          : cpa_sect_iter_node_delete
  Description   : This routine deletes the Section Iteration node from tree
  Arguments     : CPA_CB *cb - CPA Control Block.
                : CPA_SECT_ITER_NODE *lc_node - Section Iteration Node.
  Return Values : None
  Notes         : None
******************************************************************************/
uns32 cpa_sect_iter_node_delete(CPA_CB *cb, CPA_SECT_ITER_NODE *sect_iter_node)
{
	uns32 rc = NCSCC_RC_SUCCESS;

	if (sect_iter_node == NULL)
		return NCSCC_RC_FAILURE;

	/* Remove the Node from the client tree */
	if (ncs_patricia_tree_del(&cb->sect_iter_tree, &sect_iter_node->patnode) != NCSCC_RC_SUCCESS) {
		rc = NCSCC_RC_FAILURE;
	}

	/* Free the Client Node */
	if (sect_iter_node) {
		if (sect_iter_node->section_id.id) {
			m_MMGR_FREE_CPSV_DEFAULT_VAL(sect_iter_node->section_id.id, NCS_SERVICE_ID_CPND);
		}
		if (sect_iter_node->out_evt != NULL) {
			m_MMGR_FREE_CPSV_EVT(sect_iter_node->out_evt, NCS_SERVICE_ID_CPA);
		}
		m_MMGR_FREE_CPA_SECT_ITER_NODE(sect_iter_node);
		sect_iter_node = NULL;
	}
	return rc;

}

/****************************************************************************
  Name          : cpa_gbl_ckpt_tree_destroy 
  Description   : This routine destroys the CPA Section Iteration tree.
  Arguments     : CPA_CB *cb - CPA Control Block.
  Return Values : None
  Notes         : None
******************************************************************************/
void cpa_gbl_ckpt_tree_destroy(CPA_CB *cb)
{

	/* destroy the tree */
	ncs_patricia_tree_destroy(&cb->gbl_ckpt_tree);

	return;
}

/****************************************************************************
  Name          : cpa_db_init 
  Description   : This routine initializes the CPA CB database.
  Arguments     : CPA_CB *cb - CPA Control Block.
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_error
  Notes         : None
******************************************************************************/
uns32 cpa_db_init(CPA_CB *cb)
{
	uns32 rc;

	rc = cpa_client_tree_init(cb);
	if (rc != NCSCC_RC_SUCCESS) {
		m_LOG_CPA_CCL(CPA_PROC_FAILED, NCSFL_LC_CKPT_MGMT, NCSFL_SEV_ERROR, "db_init", __FILE__, __LINE__);
		return rc;
	}

	rc = cpa_lcl_ckpt_tree_init(cb);
	if (rc != NCSCC_RC_SUCCESS) {
		m_LOG_CPA_CCL(CPA_PROC_FAILED, NCSFL_LC_CKPT_MGMT, NCSFL_SEV_ERROR, "db_init", __FILE__, __LINE__);
		return rc;
	}

	rc = cpa_gbl_ckpt_tree_init(cb);
	if (rc != NCSCC_RC_SUCCESS) {
		m_LOG_CPA_CCL(CPA_PROC_FAILED, NCSFL_LC_CKPT_MGMT, NCSFL_SEV_ERROR, "db_init", __FILE__, __LINE__);
		return rc;
	}

	rc = cpa_sect_iter_tree_init(cb);
	if (rc != NCSCC_RC_SUCCESS) {
		m_LOG_CPA_CCL(CPA_PROC_FAILED, NCSFL_LC_CKPT_MGMT, NCSFL_SEV_ERROR, "db_init", __FILE__, __LINE__);
		return rc;
	}

	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
  Name          : cpa_db_destroy 
  Description   : This routine will destroy the CPA CB database.
  Arguments     : CPA_CB *cb - CPA Control Block.
  Return Values : NCSCC_RC_SUCCESS
  Notes         : None
******************************************************************************/
uns32 cpa_db_destroy(CPA_CB *cb)
{

	cpa_client_tree_destroy(cb);

	cpa_lcl_ckpt_tree_destroy(cb);

	cpa_gbl_ckpt_tree_destroy(cb);

	cpa_sect_iter_tree_destroy(cb);

	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
  Name          : cpa_client_tree_mark_stale
  Description   : This routine mark  the CPA Client tree as stale
  Arguments     : CPA_CB *cb - CPA Control Block.
  Return Values : None
  Notes         : None
******************************************************************************/
void cpa_client_tree_mark_stale(CPA_CB *cb)
{
	CPA_CLIENT_NODE *clnode = NULL;
	SaCkptHandleT *temp_ptr = NULL;
	SaCkptHandleT temp_hdl = 0;

	/* scan the entire handle db & mark each record as stale*/
	while ((clnode = (CPA_CLIENT_NODE *)
		ncs_patricia_tree_getnext(&cb->client_tree, (uns8 *)temp_ptr))) {
		clnode->stale = TRUE;
		/* mark the client info */
		temp_hdl = clnode->cl_hdl;
		temp_ptr = &temp_hdl;
			
	}
	return;
}
