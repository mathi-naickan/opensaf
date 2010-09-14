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
  
  This file contains the CPD Database access Routines
    
******************************************************************************
*/

#include "cpd.h"
#include "cpd_imm.h"
#include "immutil.h"
#include "cpd_log.h"

/****************************************************************************
  Name          : cpd_ckpt_tree_init
  Description   : This routine is used to initialize the CPD Checkpoint 
                  Tree.
  Arguments     : cb - pointer to the CPD Control Block
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
  Notes         : None
******************************************************************************/
uns32 cpd_ckpt_tree_init(CPD_CB *cb)
{
	NCS_PATRICIA_PARAMS param;
	memset(&param, 0, sizeof(NCS_PATRICIA_PARAMS));
	param.key_size = sizeof(SaCkptCheckpointHandleT);
	if (ncs_patricia_tree_init(&cb->ckpt_tree, &param) != NCSCC_RC_SUCCESS) {
		return NCSCC_RC_FAILURE;
	}
	cb->is_ckpt_tree_up = TRUE;
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
  Description   : This routine finds the checkpoint node.
  Arguments     : ckpt_tree - Ckpt Tree.
                  ckpt_hdl - Checkpoint Handle
  Return Values : ckpt_node - Checkpoint Node
                  NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
  Notes         : The caller takes the cb lock before calling this function                 
******************************************************************************/
uns32 cpd_ckpt_node_get(NCS_PATRICIA_TREE *ckpt_tree, SaCkptCheckpointHandleT *ckpt_hdl, CPD_CKPT_INFO_NODE **ckpt_node)
{
	*ckpt_node = (CPD_CKPT_INFO_NODE *)
	    ncs_patricia_tree_get(ckpt_tree, (uns8 *)ckpt_hdl);

	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
  Description   : This routine finds the checkpoint node.
  Arguments     : ckpt_tree - Ckpt Tree.
                  ckpt_hdl - Checkpoint Handle
  Return Values : ckpt_node - Checkpoint Node
                  NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
  Notes         : The caller takes the cb lock before calling this function                 
******************************************************************************/
void cpd_ckpt_node_getnext(NCS_PATRICIA_TREE *ckpt_tree,
			   SaCkptCheckpointHandleT *ckpt_hdl, CPD_CKPT_INFO_NODE **ckpt_node)
{
	if (ckpt_hdl) {
		*ckpt_node = (CPD_CKPT_INFO_NODE *)ncs_patricia_tree_getnext(ckpt_tree, (uns8 *)ckpt_hdl);
	} else {
		*ckpt_node = (CPD_CKPT_INFO_NODE *)ncs_patricia_tree_getnext(ckpt_tree, (uns8 *)NULL);
	}
	return;
}

/****************************************************************************
  Name          : cpd_ckpt_node_add
  Description   : This routine adds the new node to ckpt_tree.
  Arguments     : ckpt_tree - Checkpoint Tree.
                  ckpt_node -  checkpoint Node.
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
  Notes         : The caller takes the cb lock before calling this function                  
******************************************************************************/
uns32 cpd_ckpt_node_add(NCS_PATRICIA_TREE *ckpt_tree, CPD_CKPT_INFO_NODE *ckpt_node, SaAmfHAStateT ha_state,
			SaImmOiHandleT immOiHandle)
{
	SaAisErrorT err = SA_AIS_OK;
	/* Store the client_info pointer as msghandle. */
	ckpt_node->patnode.key_info = (uns8 *)&ckpt_node->ckpt_id;

	/*create the imm runtime object */
	if (ha_state == SA_AMF_HA_ACTIVE) {
		err = create_runtime_ckpt_object(ckpt_node, immOiHandle);
		if (err != SA_AIS_OK) {
			cpd_log(NCSFL_SEV_ERROR, "create_runtime_ckpt_object failed %u\n", err);
			return NCSCC_RC_FAILURE;
		}
	}

	if (ncs_patricia_tree_add(ckpt_tree, &ckpt_node->patnode) != NCSCC_RC_SUCCESS) {
		m_LOG_CPD_FCL(CPD_CKPT_INFO_NODE_ADD_FAILED, CPD_FC_HDLN, NCSFL_SEV_ERROR, ckpt_node->ckpt_id, __FILE__,
			      __LINE__);
		/* delete imm ckpt runtime object */
		if (ha_state == SA_AMF_HA_ACTIVE) {
			if (immutil_saImmOiRtObjectDelete(immOiHandle, &ckpt_node->ckpt_name) != SA_AIS_OK) {
				cpd_log(NCSFL_SEV_ERROR, "Deleting run time object %s FAILED",
					ckpt_node->ckpt_name.value);
				return NCSCC_RC_FAILURE;
			}
		}
		return NCSCC_RC_FAILURE;
	}

	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
  Name          : cpd_ckpt_node_delete
  Description   : This routine deletes the Checkpoint node from tree
  Arguments     : CPD_CB *cb - CPD Control Block.
                : CPD_CKPT_INFO_NODE *lc_node - Local Ckeckpoint Node.
  Return Values : None
  Notes         : None
******************************************************************************/
uns32 cpd_ckpt_node_delete(CPD_CB *cb, CPD_CKPT_INFO_NODE *ckpt_node)
{
	uns32 rc = NCSCC_RC_SUCCESS;
	CPD_NODE_REF_INFO *nref_info, *next_info;

	/* In case if the internal pointers present, delete them 
	   The while loop is executed in case of cleanup, at the time of destroy */
	if (ckpt_node == NULL) {
		rc = NCSCC_RC_FAILURE;
		return rc;
	}
	nref_info = ckpt_node->node_list;
	while (nref_info) {
		next_info = nref_info->next;
		m_MMGR_FREE_CPD_NODE_REF_INFO(nref_info);
		nref_info = next_info;
	}

	/* delete imm ckpt runtime object */
	if ((cb->ha_state == SA_AMF_HA_ACTIVE) && (ckpt_node->is_unlink_set != TRUE)) {
		if (immutil_saImmOiRtObjectDelete(cb->immOiHandle, &ckpt_node->ckpt_name) != SA_AIS_OK) {
			cpd_log(NCSFL_SEV_ERROR, "Deleting run time object %s FAILED", ckpt_node->ckpt_name.value);
			/* Free the Client Node */
			if (ckpt_node)
				m_MMGR_FREE_CPD_CKPT_INFO_NODE(ckpt_node);
			return NCSCC_RC_FAILURE;
		}
	}

	if (ncs_patricia_tree_del(&cb->ckpt_tree, &ckpt_node->patnode) != NCSCC_RC_SUCCESS) {
		rc = NCSCC_RC_FAILURE;
	}

	/* Free the Client Node */
	if (ckpt_node)
		m_MMGR_FREE_CPD_CKPT_INFO_NODE(ckpt_node);

	return rc;

}

/****************************************************************************
  Name          : cpd_ckpt_node_and_ref_delete
  Description   : This routine deletes the Checkpoint node and its reference 
                  with CPND node info 
  Arguments     : CPD_CB *cb - CPD Control Block.
                : CPD_CKPT_INFO_NODE *lc_node - Local Ckeckpoint Node.
  Return Values : None
  Notes         : None
******************************************************************************/
void cpd_ckpt_node_and_ref_delete(CPD_CB *cb, CPD_CKPT_INFO_NODE *ckpt_node)
{
	CPD_NODE_REF_INFO *nref = ckpt_node->node_list, *nref_next;
	CPD_CPND_INFO_NODE *del_node_info = NULL;
	ckpt_node->node_list = NULL;
	ckpt_node->dest_cnt = 0;

	while (nref) {
		nref_next = nref->next;
		del_node_info = NULL;
		cpd_cpnd_info_node_get(&cb->cpnd_tree, &nref->dest, &del_node_info);

		if (del_node_info) {
			CPD_CKPT_REF_INFO *cref_info = NULL;

			/* Remove the checkpoint reference from the node info */
			for (cref_info = del_node_info->ckpt_ref_list; cref_info != NULL; cref_info = cref_info->next) {
				if (cref_info->ckpt_node == ckpt_node) {
					cpd_ckpt_ref_info_del(del_node_info, cref_info);
					break;
				}
			}

			if (del_node_info->ckpt_cnt == 0)
				cpd_cpnd_info_node_delete(cb, del_node_info);
		}

		m_MMGR_FREE_CPD_NODE_REF_INFO(nref);
		nref = nref_next;
	}

	m_MMGR_FREE_CPD_CKPT_INFO_NODE(ckpt_node);

}

/****************************************************************************
  Name          : cpd_ckpt_tree_cleanup
  Description   : This routine Removes & Frees all the internal nodes.
  Arguments     : CPD_CB *cb - CPD Control Block.
  Return Values : None
  Notes         : None
******************************************************************************/
void cpd_ckpt_tree_cleanup(CPD_CB *cb)
{
	CPD_CKPT_INFO_NODE *ckpt_node;
	SaCkptCheckpointHandleT prev_ckpt_id = 0;

	/* Get the First Node */
	ckpt_node = (CPD_CKPT_INFO_NODE *)ncs_patricia_tree_getnext(&cb->ckpt_tree, (uns8 *)&prev_ckpt_id);
	while (ckpt_node) {
		prev_ckpt_id = ckpt_node->ckpt_id;

		cpd_ckpt_node_delete(cb, ckpt_node);

		ckpt_node = (CPD_CKPT_INFO_NODE *)ncs_patricia_tree_getnext(&cb->ckpt_tree, (uns8 *)&prev_ckpt_id);
	}

	return;
}

/****************************************************************************
  Name          : cpd_ckpt_tree_destroy
  Description   : This routine destroys the CPD lcl ckpt tree.
  Arguments     : CPD_CB *cb - CPD Control Block.
  Return Values : None
  Notes         : None
******************************************************************************/
void cpd_ckpt_tree_destroy(CPD_CB *cb)
{
	if (!cb->is_ckpt_tree_up)
		return;

	/* cleanup the client tree */
	cpd_ckpt_tree_cleanup(cb);

	/* destroy the tree */
	ncs_patricia_tree_destroy(&cb->ckpt_tree);

	return;
}

/****************************************************************************
  Name          : cpd_ckpt_tree_node_destroy
  Description   : This cleans the nodes of trees in CB.
  Arguments     : CPD_CB *cb - CPD Control Block.
  Return Values : None
  Notes         : None
******************************************************************************/

void cpd_ckpt_tree_node_destroy(CPD_CB *cb)
{
	if (!cb->is_ckpt_tree_up)
		return;
	cpd_ckpt_tree_cleanup(cb);

	if (!cb->is_ckpt_map_up)
		return;
	cpd_ckpt_map_tree_cleanup(cb);

	if (!cb->is_cpnd_tree_up)
		return;
	cpd_cpnd_info_tree_cleanup(cb);

	if (!cb->is_ckpt_reploc_up)
		return;
	cpd_ckpt_reploc_cleanup(cb);

	return;
}

/****************************************************************************
  Name          : cpd_ckpt_reploc_tree_init
  Description   : This routine is used for replica location table 
  Arguments     : cb - pointer to the CPD Control Block
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
  Notes         : None
******************************************************************************/
uns32 cpd_ckpt_reploc_tree_init(CPD_CB *cb)
{
	NCS_PATRICIA_PARAMS param;
	memset(&param, 0, sizeof(NCS_PATRICIA_PARAMS));
	/*  param.key_size = 2 *sizeof(SaNameT); */
	param.key_size = sizeof(CPD_REP_KEY_INFO);
	if (ncs_patricia_tree_init(&cb->ckpt_reploc_tree, &param) != NCSCC_RC_SUCCESS) {
		TRACE("CPD_CKPT_REPLOC_TREE_INIT FAILED");
		return NCSCC_RC_FAILURE;
	}
	cb->is_ckpt_reploc_up = TRUE;
	TRACE("CPD_CKPT_REPLOC_TREE_INIT SUCCESS");
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
  Name          : cpd_ckpt_reploc_get
  Description   : This routine finds the checkpoint node.
  Arguments     : ckpt_reploc_tree - Ckpt Tree.
                  ckpt_name - Checkpoint Name
  Return Values : ckpt_reploc_node - Checkpoint Node
                  NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
  Notes         : The caller takes the cb lock before calling this function                 
******************************************************************************/
uns32 cpd_ckpt_reploc_get(NCS_PATRICIA_TREE *ckpt_reploc_tree,
			  CPD_REP_KEY_INFO *key_info, CPD_CKPT_REPLOC_INFO **ckpt_reploc_node)
{
	key_info->ckpt_name.length = m_NCS_OS_HTONS(key_info->ckpt_name.length);
	key_info->node_name.length = m_NCS_OS_HTONS(key_info->node_name.length);

	*ckpt_reploc_node = (CPD_CKPT_REPLOC_INFO *)ncs_patricia_tree_get(ckpt_reploc_tree, (uns8 *)key_info);
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
  Name          : cpd_ckpt_reploc_getnext
  Description   : This routine finds the checkpoint node.
  Arguments     : ckpt_reploc_tree - Ckpt Tree.
                  ckpt_name - Checkpoint Name 
  Return Values : ckpt_reploc_node - Checkpoint Node
                  NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
  Notes         : The caller takes the cb lock before calling this function                 
******************************************************************************/
void cpd_ckpt_reploc_getnext(NCS_PATRICIA_TREE *ckpt_reploc_tree,
			     CPD_REP_KEY_INFO *key_info, CPD_CKPT_REPLOC_INFO **ckpt_reploc_node)
{
	if (key_info) {
		key_info->ckpt_name.length = m_NCS_OS_HTONS(key_info->ckpt_name.length);
		key_info->node_name.length = m_NCS_OS_HTONS(key_info->node_name.length);
		*ckpt_reploc_node =
		    (CPD_CKPT_REPLOC_INFO *)ncs_patricia_tree_getnext(ckpt_reploc_tree, (uns8 *)key_info);
	} else {
		*ckpt_reploc_node = (CPD_CKPT_REPLOC_INFO *)ncs_patricia_tree_getnext(ckpt_reploc_tree, (uns8 *)NULL);
	}
	return;
}

/****************************************************************************
  Name          : cpd_ckpt_reploc_node_add
  Description   : This routine adds the new node to ckpt_tree.
  Arguments     : ckpt_tree - Checkpoint Tree.
                  ckpt_node -  checkpoint Node.
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
  Notes         : The caller takes the cb lock before calling this function                  
******************************************************************************/
uns32 cpd_ckpt_reploc_node_add(NCS_PATRICIA_TREE *ckpt_reploc_tree, CPD_CKPT_REPLOC_INFO *ckpt_reploc_node,
			       SaAmfHAStateT ha_state, SaImmOiHandleT immOiHandle)
{
	SaAisErrorT err = SA_AIS_OK;
	SaNameT replica_dn;
	memset(&replica_dn, 0, sizeof(SaNameT));

	/* Add the imm runtime object */
	if (ha_state == SA_AMF_HA_ACTIVE) {
		err = create_runtime_replica_object(ckpt_reploc_node, immOiHandle);
		if (err != SA_AIS_OK) {
			cpd_log(NCSFL_SEV_ERROR, "create_runtime_replica_object failed %u\n", err);
			return NCSCC_RC_FAILURE;
		}
	}
	ckpt_reploc_node->rep_key.ckpt_name.length = m_NCS_OS_HTONS(ckpt_reploc_node->rep_key.ckpt_name.length);
	/* node name is obtained from cluster info which always returns in network order, so no need of the conversion for the node_name length */
	ckpt_reploc_node->rep_key.node_name.length = m_NCS_OS_HTONS(ckpt_reploc_node->rep_key.node_name.length);

	ckpt_reploc_node->patnode.key_info = (uns8 *)&ckpt_reploc_node->rep_key;
	if (ncs_patricia_tree_add(ckpt_reploc_tree, &ckpt_reploc_node->patnode) != NCSCC_RC_SUCCESS) {
		/* m_LOG_CPD_HEADLINE(CPD_CKPT_REPLOC_INFO_ADD_FAILED, NCSFL_SEV_ERROR); */
		/* delete reploc imm runtime object */
		if (ha_state == SA_AMF_HA_ACTIVE) {
			cpd_create_association_class_dn(&ckpt_reploc_node->rep_key.node_name,
							&ckpt_reploc_node->rep_key.ckpt_name, "safReplica", &replica_dn);

			if (immutil_saImmOiRtObjectDelete(immOiHandle, &replica_dn) != SA_AIS_OK) {
				cpd_log(NCSFL_SEV_ERROR, "Deleting run time object %s FAILED", replica_dn.value);
				return NCSCC_RC_FAILURE;
			}
			return NCSCC_RC_FAILURE;
		}
	}

	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
  Name          : cpd_ckpt_reploc_node_delete
  Description   : This routine deletes the Checkpoint node from tree
  Arguments     : CPD_CB *cb - CPD Control Block.
                : CPD_CKPT_REPLOC_INFO  *ckpt_reploc_node - Local Ckeckpoint Node.
  Return Values : None
  Notes         : None
******************************************************************************/
uns32 cpd_ckpt_reploc_node_delete(CPD_CB *cb, CPD_CKPT_REPLOC_INFO *ckpt_reploc_node, NCS_BOOL is_unlink_set)
{
	uns32 rc = NCSCC_RC_SUCCESS;

	if (cb->ha_state == SA_AMF_HA_ACTIVE) {

		rc = cpd_ckpt_reploc_imm_object_delete(cb, ckpt_reploc_node, is_unlink_set);
		if (rc != NCSCC_RC_SUCCESS) {

			/* goto reploc_node_add_fail; */
			m_LOG_CPD_CL(CPD_DB_ADD_FAILED, CPD_FC_DB, NCSFL_SEV_ERROR, __FILE__, __LINE__);

			rc = NCSCC_RC_FAILURE;

		}
	}

	/* Remove the Node from the client tree */
	if (ncs_patricia_tree_del(&cb->ckpt_reploc_tree, &ckpt_reploc_node->patnode) != NCSCC_RC_SUCCESS) {
		rc = cpd_ckpt_reploc_node_add(&cb->ckpt_reploc_tree, ckpt_reploc_node, cb->ha_state, cb->immOiHandle);
		if (rc != NCSCC_RC_SUCCESS) {
			/* goto reploc_node_add_fail; */
			m_LOG_CPD_CL(CPD_DB_ADD_FAILED, CPD_FC_DB, NCSFL_SEV_ERROR, __FILE__, __LINE__);
		}
		rc = NCSCC_RC_FAILURE;
	}

	/* Free the Client Node */
	if (ckpt_reploc_node)
		m_MMGR_FREE_CPD_CKPT_REPLOC_INFO(ckpt_reploc_node);

	return rc;
}

/****************************************************************************
  Name          : cpd_ckpt_reploc_cleanup
  Description   : This routine cleans up the Checkpoint node from tree
  Arguments     : CPD_CB *cb - CPD Control Block.
                : CPD_CKPT_REPLOC_INFO  *ckpt_reploc_node - Local Ckeckpoint Node.
  Return Values : None
  Notes         : None
******************************************************************************/
void cpd_ckpt_reploc_cleanup(CPD_CB *cb)
{
	CPD_CKPT_REPLOC_INFO *ckpt_reploc_node;
	CPD_REP_KEY_INFO key_info;

	memset(&key_info, 0, sizeof(CPD_REP_KEY_INFO));

	/*  Get the 1st Node */
	ckpt_reploc_node = (CPD_CKPT_REPLOC_INFO *)ncs_patricia_tree_getnext(&cb->ckpt_reploc_tree, (uns8 *)&key_info);
	while (ckpt_reploc_node) {
		key_info = ckpt_reploc_node->rep_key;

		cpd_ckpt_reploc_node_delete(cb, ckpt_reploc_node, FALSE);

		ckpt_reploc_node =
		    (CPD_CKPT_REPLOC_INFO *)ncs_patricia_tree_getnext(&cb->ckpt_reploc_tree, (uns8 *)&key_info);

	}
	return;
}

/****************************************************************************
  Name          : cpd_ckpt_reploc_destroy
  Description   : This routine cleans up the Checkpoint node from tree
  Arguments     : CPD_CB *cb - CPD Control Block.
                : CPD_CKPT_REPLOC_INFO  *ckpt_reploc_node - Local Ckeckpoint Node.
  Return Values : None
  Notes         : None
******************************************************************************/
void cpd_ckpt_reploc_tree_destroy(CPD_CB *cb)
{
	if (!cb->is_ckpt_reploc_up)
		return;

	cpd_ckpt_reploc_cleanup(cb);

	ncs_patricia_tree_destroy(&cb->ckpt_reploc_tree);

	return;
}

/****************************************************************************
  Name          : cpd_ckpt_map_tree_init
  Description   : This routine is used to initialize the CPD Checkpoint MAP 
                  Tree.
  Arguments     : cb - pointer to the CPD Control Block
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
  Notes         : None
******************************************************************************/
uns32 cpd_ckpt_map_tree_init(CPD_CB *cb)
{
	NCS_PATRICIA_PARAMS param;
	memset(&param, 0, sizeof(NCS_PATRICIA_PARAMS));
	param.key_size = sizeof(SaNameT);
	if (ncs_patricia_tree_init(&cb->ckpt_map_tree, &param) != NCSCC_RC_SUCCESS) {
		return NCSCC_RC_FAILURE;
	}
	cb->is_ckpt_map_up = TRUE;
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
  Name          : cpd_ckpt_map_node_get
  Description   : This routine finds the checkpoint node.
  Arguments     : ckpt_map_tree - Ckpt Tree.
                  ckpt_name - Checkpoint Name
  Return Values : ckpt_map_node - Checkpoint Node
                  NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
  Notes         : The caller takes the cb lock before calling this function                 
******************************************************************************/
uns32 cpd_ckpt_map_node_get(NCS_PATRICIA_TREE *ckpt_map_tree, SaNameT *ckpt_name, CPD_CKPT_MAP_INFO **ckpt_map_node)
{

	/* stuffing the key with NULL as patricia tries to match the max keylen */
	ckpt_name->length = m_NCS_OS_HTONS(ckpt_name->length);

/*   ckpt_name->length = m_NCS_OS_HTONS(ckpt_name->length); */
	*ckpt_map_node = (CPD_CKPT_MAP_INFO *)
	    ncs_patricia_tree_get(ckpt_map_tree, (uns8 *)ckpt_name);

	ckpt_name->length = m_NCS_OS_NTOHS(ckpt_name->length);

	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
  Name          : cpd_ckpt_map_node_getnext
  Description   : This routine finds the checkpoint node.
  Arguments     : ckpt_map_tree - Ckpt Tree.
                  ckpt_name - Checkpoint Name 
  Return Values : ckpt_map_node - Checkpoint Node
                  NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
  Notes         : The caller takes the cb lock before calling this function                 
******************************************************************************/
void cpd_ckpt_map_node_getnext(NCS_PATRICIA_TREE *ckpt_map_tree, SaNameT *ckpt_name, CPD_CKPT_MAP_INFO **ckpt_map_node)
{

	if (ckpt_name) {
		ckpt_name->length = m_NCS_OS_HTONS(ckpt_name->length);
		*ckpt_map_node = (CPD_CKPT_MAP_INFO *)ncs_patricia_tree_getnext(ckpt_map_tree, (uns8 *)ckpt_name);
		ckpt_name->length = m_NCS_OS_NTOHS(ckpt_name->length);
	}

	else {
		*ckpt_map_node = (CPD_CKPT_MAP_INFO *)ncs_patricia_tree_getnext(ckpt_map_tree, (uns8 *)NULL);
	}

	return;
}

/****************************************************************************
  Name          : cpd_ckpt_map_node_add
  Description   : This routine adds the new node to ckpt_tree.
  Arguments     : ckpt_tree - Checkpoint Tree.
                  ckpt_node -  checkpoint Node.
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
  Notes         : The caller takes the cb lock before calling this function                  
******************************************************************************/
uns32 cpd_ckpt_map_node_add(NCS_PATRICIA_TREE *ckpt_map_tree, CPD_CKPT_MAP_INFO *ckpt_map_node)
{
	ckpt_map_node->ckpt_name.length = m_NCS_OS_HTONS(ckpt_map_node->ckpt_name.length);
	ckpt_map_node->patnode.key_info = (uns8 *)&ckpt_map_node->ckpt_name;

	if (ncs_patricia_tree_add(ckpt_map_tree, &ckpt_map_node->patnode) != NCSCC_RC_SUCCESS) {

		m_LOG_CPD_CCL(CPD_CKPT_MAP_INFO_ADD_FAILED, CPD_FC_HDLN, NCSFL_SEV_ERROR,
			      ckpt_map_node->ckpt_name.value, __FILE__, __LINE__);

		return NCSCC_RC_FAILURE;
	}
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
  Name          : cpd_ckpt_map_node_delete
  Description   : This routine deletes the Checkpoint node from tree
  Arguments     : CPD_CB *cb - CPD Control Block.
                : CPD_CKPT_INFO_NODE *lc_node - Local Ckeckpoint Node.
  Return Values : None
  Notes         : None
******************************************************************************/
uns32 cpd_ckpt_map_node_delete(CPD_CB *cb, CPD_CKPT_MAP_INFO *ckpt_map_node)
{
	uns32 rc = NCSCC_RC_SUCCESS;

	/* Remove the Node from the client tree */
	if (ncs_patricia_tree_del(&cb->ckpt_map_tree, &ckpt_map_node->patnode) != NCSCC_RC_SUCCESS) {
		m_LOG_CPD_CCL(CPD_MAP_NODE_DELETE_FAILED, CPD_FC_HDLN, NCSFL_SEV_ERROR, ckpt_map_node->ckpt_name.value,
			      __FILE__, __LINE__);
		rc = NCSCC_RC_FAILURE;
	}

	/* Free the Client Node */
	if (ckpt_map_node)
		m_MMGR_FREE_CPD_CKPT_MAP_INFO(ckpt_map_node);

	return rc;

}

/****************************************************************************
  Name          : cpd_ckpt_map_tree_cleanup
  Description   : This routine Frees all the nodes in the map_tree.
  Arguments     : CPD_CB *cb - CPD Control Block.
  Return Values : None
  Notes         : None
******************************************************************************/
void cpd_ckpt_map_tree_cleanup(CPD_CB *cb)
{
	CPD_CKPT_MAP_INFO *ckpt_map_node;
	SaNameT name;

	memset(&name, 0, sizeof(SaNameT));

	/* Get the First Node */
	ckpt_map_node = (CPD_CKPT_MAP_INFO *)ncs_patricia_tree_getnext(&cb->ckpt_map_tree, (uns8 *)&name);
	while (ckpt_map_node) {
		name = ckpt_map_node->ckpt_name;

		cpd_ckpt_map_node_delete(cb, ckpt_map_node);

		ckpt_map_node = (CPD_CKPT_MAP_INFO *)ncs_patricia_tree_getnext(&cb->ckpt_map_tree, (uns8 *)&name);
	}

	return;
}

/****************************************************************************
  Name          : cpd_ckpt_map_tree_destroy
  Description   : This routine destroys the CPD lcl ckpt tree.
  Arguments     : CPD_CB *cb - CPD Control Block.
  Return Values : None
  Notes         : None
******************************************************************************/
void cpd_ckpt_map_tree_destroy(CPD_CB *cb)
{
	if (!cb->is_ckpt_map_up)
		return;

	/* cleanup the client tree */
	cpd_ckpt_map_tree_cleanup(cb);

	/* destroy the tree */
	ncs_patricia_tree_destroy(&cb->ckpt_map_tree);

	return;
}

/****************************************************************************
  Name          : cpd_cpnd_info_tree_init
  Description   : This routine is used to initialize the CPND info Tree
  Arguments     : cb - pointer to the CPD Control Block
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
  Notes         : None
******************************************************************************/
uns32 cpd_cpnd_info_tree_init(CPD_CB *cb)
{
	NCS_PATRICIA_PARAMS param;
	memset(&param, 0, sizeof(NCS_PATRICIA_PARAMS));

	param.key_size = sizeof(NODE_ID);
	if (ncs_patricia_tree_init(&cb->cpnd_tree, &param) != NCSCC_RC_SUCCESS) {
		return NCSCC_RC_FAILURE;
	}
	cb->is_cpnd_tree_up = TRUE;
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
  Name          : cpd_cpnd_info_node_get
  Description   : This routine finds the CPND Info node.
  Arguments     : ckpt_map_tree - Ckpt Tree.
                  ckpt_name - Checkpoint Name
  Return Values : ckpt_map_node - Checkpoint Node
                  NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
  Notes         : The caller takes the cb lock before calling this function                 
******************************************************************************/
uns32 cpd_cpnd_info_node_get(NCS_PATRICIA_TREE *cpnd_tree, MDS_DEST *dest, CPD_CPND_INFO_NODE **cpnd_info_node)
{
	NODE_ID key;

	memset(&key, 0, sizeof(NODE_ID));
	/* Fill the Key */
	key = m_NCS_NODE_ID_FROM_MDS_DEST((*dest));

	*cpnd_info_node = (CPD_CPND_INFO_NODE *)
	    ncs_patricia_tree_get(cpnd_tree, (uns8 *)&key);

	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
  Name          : cpd_cpnd_info_node_getnext
  Description   : This routine finds the CPND Info node.
  Arguments     : cpnd_tree - Ckpt Tree.
                  dest - MDS_DEST
  Return Values : cpnd_info_node - Checkpoint Node
                  NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
  Notes         : The caller takes the cb lock before calling this function                 
******************************************************************************/
void cpd_cpnd_info_node_getnext(NCS_PATRICIA_TREE *cpnd_tree, MDS_DEST *dest, CPD_CPND_INFO_NODE **cpnd_info_node)
{
	NODE_ID key;
	memset(&key, 0, sizeof(NODE_ID));
	/* Fill the Key */

	if (dest) {
		key = m_NCS_NODE_ID_FROM_MDS_DEST((*dest));

		*cpnd_info_node = (CPD_CPND_INFO_NODE *)
		    ncs_patricia_tree_getnext(cpnd_tree, (uns8 *)&key);
	} else
		*cpnd_info_node = (CPD_CPND_INFO_NODE *)
		    ncs_patricia_tree_getnext(cpnd_tree, (uns8 *)NULL);

	return;

}

/****************************************************************************
  Name          : cpd_cpnd_info_node_add
  Description   : This routine adds the new node to ckpt_tree.
  Arguments     : ckpt_tree - Checkpoint Tree.
                  ckpt_node -  checkpoint Node.
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
  Notes         : The caller takes the cb lock before calling this function                  
******************************************************************************/
uns32 cpd_cpnd_info_node_add(NCS_PATRICIA_TREE *cpnd_tree, CPD_CPND_INFO_NODE *cpnd_info_node)
{
	/* Store the client_info pointer as msghandle. */
	NODE_ID key;

	key = m_NCS_NODE_ID_FROM_MDS_DEST(cpnd_info_node->cpnd_dest);

	/*  cpnd_info_node->patnode.key_info = (uns8*)(m_NCS_NODE_ID_FROM_MDS_DEST(cpnd_info_node->cpnd_dest));  */
	cpnd_info_node->patnode.key_info = (uns8 *)&key;

	if (ncs_patricia_tree_add(cpnd_tree, &cpnd_info_node->patnode) != NCSCC_RC_SUCCESS) {
		m_LOG_CPD_CL(CPD_CPND_INFO_NODE_FAILED, CPD_FC_HDLN, NCSFL_SEV_ERROR, __FILE__, __LINE__);

		return NCSCC_RC_FAILURE;
	}

	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
  Name          : cpd_cpnd_info_node_add
  Description   : This routine adds the new node to ckpt_tree.
  Arguments     : ckpt_tree - Checkpoint Tree.
                  ckpt_node -  checkpoint Node.
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
  Notes         : The caller takes the cb lock before calling this function                  
******************************************************************************/
uns32 cpd_cpnd_info_node_find_add(NCS_PATRICIA_TREE *cpnd_tree, MDS_DEST *dest,
				  CPD_CPND_INFO_NODE **cpnd_info_node, NCS_BOOL *add_flag)
{
	/*MDS_DEST key; */
	NODE_ID key;

	memset(&key, 0, sizeof(NODE_ID));
	/* Fill the Key */
	key = m_NCS_NODE_ID_FROM_MDS_DEST((*dest));

	*cpnd_info_node = (CPD_CPND_INFO_NODE *)
	    ncs_patricia_tree_get(cpnd_tree, (uns8 *)&key);
	if ((*cpnd_info_node == NULL) && (*add_flag == TRUE)) {
		*cpnd_info_node = m_MMGR_ALLOC_CPD_CPND_INFO_NODE;
		if (*cpnd_info_node == NULL) {
			m_LOG_CPD_CL(CPD_CPND_INFO_ALLOC_FAILED, CPD_FC_MEMFAIL, NCSFL_SEV_ERROR, __FILE__, __LINE__);
			return NCSCC_RC_FAILURE;
		}
		memset((*cpnd_info_node), '\0', sizeof(CPD_CPND_INFO_NODE));

		/* Store the client_info pointer as msghandle. */
		(*cpnd_info_node)->cpnd_dest = *dest;
		(*cpnd_info_node)->cpnd_key = m_NCS_NODE_ID_FROM_MDS_DEST((*dest));
		/*  (*cpnd_info_node)->patnode.key_info = (uns8*)&(m_NCS_NODE_ID_FROM_MDS_DEST((*cpnd_info_node)->cpnd_dest)); */
		(*cpnd_info_node)->patnode.key_info = (uns8 *)&((*cpnd_info_node)->cpnd_key);

		if (ncs_patricia_tree_add(cpnd_tree, &(*cpnd_info_node)->patnode) != NCSCC_RC_SUCCESS) {
			m_LOG_CPD_FCL(CPD_CPND_INFO_NODE_FAILED, CPD_FC_HDLN, NCSFL_SEV_ERROR, *dest, __FILE__,
				      __LINE__);
			return NCSCC_RC_FAILURE;
		}
		*add_flag = FALSE;
	}

	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
  Name          : cpd_cpnd_info_node_delete
  Description   : This routine deletes the cpnd_info node from tree
  Arguments     : CPD_CB *cb - CPD Control Block.
                : CPD_CPND_INFO_NODE *cpnd_info - CPND Info Node.
  Return Values : None
  Notes         : None
******************************************************************************/
uns32 cpd_cpnd_info_node_delete(CPD_CB *cb, CPD_CPND_INFO_NODE *cpnd_info_node)
{
	uns32 rc = NCSCC_RC_SUCCESS;
	CPD_CKPT_REF_INFO *cref_info = NULL, *next_ref;

	/* Remove the internal linked list, if exists, The while loop will be executed
	   only in case of clean up */
	if (cpnd_info_node == NULL) {
		m_LOG_CPD_CL(CPD_CPND_INFO_NODE_DELETE_FAILED, CPD_FC_HDLN, NCSFL_SEV_ERROR, __FILE__, __LINE__);
		return NCSCC_RC_FAILURE;
	}
	cref_info = cpnd_info_node->ckpt_ref_list;
	while (cref_info) {
		next_ref = cref_info->next;
		m_MMGR_FREE_CPD_CKPT_REF_INFO(cref_info);
		cref_info = next_ref;
	}

	/* Remove the Node from the client tree */
	if (ncs_patricia_tree_del(&cb->cpnd_tree, &cpnd_info_node->patnode) != NCSCC_RC_SUCCESS) {
		m_LOG_CPD_CL(CPD_CPND_INFO_NODE_DELETE_FAILED, CPD_FC_HDLN, NCSFL_SEV_ERROR, __FILE__, __LINE__);
		rc = NCSCC_RC_FAILURE;
	}

	/* Free the Client Node */
	if (cpnd_info_node) {
		if (cpnd_info_node->cpnd_ret_timer.uarg)
			ncshm_destroy_hdl(NCS_SERVICE_ID_CPD, cpnd_info_node->cpnd_ret_timer.uarg);

		m_MMGR_FREE_CPD_CPND_INFO_NODE(cpnd_info_node);
	}

	return rc;

}

/****************************************************************************
  Name          : cpd_cpnd_info_tree_cleanup
  Description   : This routine Free all the nodes in cpnd_tree.
  Arguments     : CPD_CB *cb - CPD Control Block.
  Return Values : None
  Notes         : None
******************************************************************************/
void cpd_cpnd_info_tree_cleanup(CPD_CB *cb)
{
	CPD_CPND_INFO_NODE *cpnd_info_node;
	NODE_ID key;

	memset(&key, 0, sizeof(NODE_ID));

	/* Get the First Node */
	cpnd_info_node = (CPD_CPND_INFO_NODE *)ncs_patricia_tree_getnext(&cb->cpnd_tree, (uns8 *)&key);
	while (cpnd_info_node) {
		key = m_NCS_NODE_ID_FROM_MDS_DEST(cpnd_info_node->cpnd_dest);

		cpd_cpnd_info_node_delete(cb, cpnd_info_node);

		cpnd_info_node = (CPD_CPND_INFO_NODE *)ncs_patricia_tree_getnext(&cb->cpnd_tree, (uns8 *)&key);
	}

	return;
}

/****************************************************************************
  Name          : cpd_cpnd_info_tree_destroy
  Description   : This routine destroys the CPD lcl ckpt tree.
  Arguments     : CPD_CB *cb - CPD Control Block.
  Return Values : None
  Notes         : None
******************************************************************************/
void cpd_cpnd_info_tree_destroy(CPD_CB *cb)
{
	if (!cb->is_cpnd_tree_up)
		return;

	/* cleanup the client tree */
	cpd_cpnd_info_tree_cleanup(cb);

	/* destroy the tree */
	ncs_patricia_tree_destroy(&cb->cpnd_tree);

	return;
}

/****************************************************************************
 * Name          : cpd_cb_db_init
 *
 * Description   : This is the function which initializes all the data 
 *                 structures and locks used belongs to CPD.
 *
 * Arguments     : cb  - CPD control block pointer.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * Notes         : None.
 *****************************************************************************/
uns32 cpd_cb_db_init(CPD_CB *cb)
{
	uns32 rc = NCSCC_RC_SUCCESS;

	cb->nxt_ckpt_id = 1;

	rc = cpd_ckpt_tree_init(cb);
	if (rc != NCSCC_RC_SUCCESS) {
		m_LOG_CPD_CL(CPD_CKPT_TREE_INIT_FAILED, CPD_FC_HDLN, NCSFL_SEV_ERROR, __FILE__, __LINE__);
		return rc;
	}

	rc = cpd_ckpt_map_tree_init(cb);
	if (rc != NCSCC_RC_SUCCESS) {
		m_LOG_CPD_CL(CPD_MAP_TREE_INIT_FAILED, CPD_FC_HDLN, NCSFL_SEV_ERROR, __FILE__, __LINE__);
		return rc;
	}

	rc = cpd_cpnd_info_tree_init(cb);
	if (rc != NCSCC_RC_SUCCESS) {
		m_LOG_CPD_CL(CPD_CPND_INFO_TREE_INIT_FAILED, CPD_FC_HDLN, NCSFL_SEV_ERROR, __FILE__, __LINE__);
		return rc;
	}

	rc = cpd_ckpt_reploc_tree_init(cb);
	if (rc != NCSCC_RC_SUCCESS) {
		m_LOG_CPD_CL(CPD_CKPT_REPLOC_TREE_INIT_FAILED, CPD_FC_HDLN, NCSFL_SEV_ERROR, __FILE__, __LINE__);
		return rc;
	}

	return (NCSCC_RC_SUCCESS);
}

/****************************************************************************
 * Name          : cpd_cb_db_destroy
 *
 * Description   : Destoroy the databases in CB
 *
 * Arguments     : cb  - Ifsv control block pointer.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * Notes         : None.
 *****************************************************************************/
uns32 cpd_cb_db_destroy(CPD_CB *cb)
{

	cpd_ckpt_tree_destroy(cb);
	cpd_ckpt_map_tree_destroy(cb);
	cpd_cpnd_info_tree_destroy(cb);
	cpd_ckpt_reploc_tree_destroy(cb);

	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
 * Name          : cpd_ckpt_ref_info_add
 *
 * Description   : CPD_CKPT_REF_INFO Linked list manipulation function
 *
 *****************************************************************************/
void cpd_ckpt_ref_info_add(CPD_CPND_INFO_NODE *node_info, CPD_CKPT_REF_INFO *cref_info)
{

	CPD_CKPT_REF_INFO *cref = NULL;

	cref = node_info->ckpt_ref_list;
	while (cref) {
		if (cref->ckpt_node->ckpt_id == cref_info->ckpt_node->ckpt_id)
			return;

		cref = cref->next;
	}

	/* Add the node at the begin of the linked list */
	cref_info->next = node_info->ckpt_ref_list;
	node_info->ckpt_ref_list = cref_info;
	node_info->ckpt_cnt++;
/*   m_NCS_CONST_PRINTF("cpnd_node ckpt_reference %d \n",node_info->ckpt_cnt); */

	return;
}

/****************************************************************************
 * Name          : cpd_ckpt_ref_info_del
 *
 * Description   : CPD_CKPT_REF_INFO Linked list manipulation function
 *
 *****************************************************************************/
void cpd_ckpt_ref_info_del(CPD_CPND_INFO_NODE *node_info, CPD_CKPT_REF_INFO *cref_info)
{
	CPD_CKPT_REF_INFO *cref, *cref_prev = 0;
	NCS_BOOL found = FALSE;

	cref = node_info->ckpt_ref_list;
	while (cref) {
		if (cref == cref_info) {
			found = TRUE;
			break;
		}
		cref_prev = cref;
		cref = cref->next;
	}

	if (found) {
		if (cref_prev == NULL) {
			/* First node */
			node_info->ckpt_ref_list = cref->next;
		} else {
			cref_prev->next = cref->next;
		}

		node_info->ckpt_cnt--;
		m_MMGR_FREE_CPD_CKPT_REF_INFO(cref);
	}

	return;
}

/****************************************************************************
 * Name          : cpd_node_ref_info_add
 *
 * Description   : CPD_NODE_REF_INFO Linked list manipulation function
 *
 *****************************************************************************/
void cpd_node_ref_info_add(CPD_CKPT_INFO_NODE *ckpt_node, CPD_NODE_REF_INFO *nref_info)
{
	CPD_NODE_REF_INFO *nref = NULL;

	nref = ckpt_node->node_list;
	while (nref) {
		if (nref->dest == nref_info->dest)
			return;

		nref = nref->next;
	}

	/* Add the node at the begin of the linked list */
	nref_info->next = ckpt_node->node_list;
	ckpt_node->node_list = nref_info;
	ckpt_node->dest_cnt++;

	return;
}

/****************************************************************************
 * Name          : cpd_node_ref_info_del
 *
 * Description   : CPD_NODE_REF_INFO Linked list manipulation function
 *
 *****************************************************************************/
void cpd_node_ref_info_del(CPD_CKPT_INFO_NODE *ckpt_node, CPD_NODE_REF_INFO *nref_info)
{
	CPD_NODE_REF_INFO *nref, *nref_prev = 0;
	NCS_BOOL found = FALSE;

	nref = ckpt_node->node_list;
	while (nref) {
		if (nref == nref_info) {
			found = TRUE;
			break;
		}
		nref_prev = nref;
		nref = nref->next;
	}

	if (found) {
		if (nref_prev == NULL) {
			/* First node */
			ckpt_node->node_list = nref->next;
		} else {
			nref_prev->next = nref->next;
		}
		ckpt_node->dest_cnt--;
		m_MMGR_FREE_CPD_NODE_REF_INFO(nref);
	}

	return;
}

/******************************************************************************************************
* Name  : cpd_process_cpnd_del
*
* Description : To delete the info of the CPND when CPND goes down
*
* Arguments  : MDS_DEST - mds dest
*
* 
*******************************************************************************************************/
uns32 cpd_process_cpnd_del(CPD_CB *cb, MDS_DEST *cpnd_dest)
{
	CPD_CPND_INFO_NODE *cpnd_info = NULL;
	CPD_CKPT_REF_INFO *cref_info, *temp;
	CPD_NODE_REF_INFO *nref_info = NULL;
	CPD_CKPT_MAP_INFO *map_info = NULL;
	uns32 rc = NCSCC_RC_SUCCESS;
	CPD_REP_KEY_INFO key_info;
	CPD_CKPT_REPLOC_INFO *rep_info = NULL;

	memset(&key_info, 0, sizeof(CPD_REP_KEY_INFO));

	cpd_cpnd_info_node_get(&cb->cpnd_tree, cpnd_dest, &cpnd_info);
	if (cpnd_info == NULL) {
		rc = NCSCC_RC_FAILURE;
		return rc;
	}
	cref_info = cpnd_info->ckpt_ref_list;

	while (cref_info) {
		CPD_CKPT_INFO_NODE *ckpt_node = NULL;

		ckpt_node = cref_info->ckpt_node;
		if (ckpt_node) {
			key_info.ckpt_name = ckpt_node->ckpt_name;
			key_info.node_name = cpnd_info->node_name;

			for (nref_info = ckpt_node->node_list; nref_info != NULL; nref_info = nref_info->next) {
				if (m_NCS_MDS_DEST_EQUAL(&nref_info->dest, cpnd_dest)) {
					if (m_NCS_MDS_DEST_EQUAL(cpnd_dest, &ckpt_node->active_dest)) {
						ckpt_node->is_active_exists = FALSE;
					}
					cpd_node_ref_info_del(ckpt_node, nref_info);
					break;
				}
			}

			cpd_ckpt_reploc_get(&cb->ckpt_reploc_tree, &key_info, &rep_info);
			if (rep_info) {
				cpd_ckpt_reploc_node_delete(cb, rep_info, ckpt_node->is_unlink_set);
			}

			if (ckpt_node->dest_cnt == 0) {
				cpd_ckpt_map_node_get(&cb->ckpt_map_tree, &ckpt_node->ckpt_name, &map_info);
				if (map_info) {
					cpd_ckpt_map_node_delete(cb, map_info);
				}
				cpd_ckpt_node_delete(cb, ckpt_node);
			}

		}

		temp = cref_info;
		cref_info = cref_info->next;

		m_MMGR_FREE_CPD_CKPT_REF_INFO(temp);
	}
	cpnd_info->ckpt_ref_list = NULL;

	/* get each  ckpt_nodes and */

	cpd_cpnd_info_node_delete(cb, cpnd_info);

	return rc;
}

/********************************************************************************
 Name    :  cpd_get_slot_sub_id_from_mds_dest

 Description :  To get the physical slot id from the mds dest

 Arguments   :

*************************************************************************************/

uns32 cpd_get_slot_sub_id_from_mds_dest(MDS_DEST dest)
{
	NCS_PHY_SLOT_ID phy_slot;
	NCS_SUB_SLOT_ID sub_slot;

	m_NCS_GET_PHYINFO_FROM_NODE_ID(m_NCS_NODE_ID_FROM_MDS_DEST(dest), NULL, &phy_slot, &sub_slot);

	return ((sub_slot * NCS_SUB_SLOT_MAX) + (phy_slot));
}

/********************************************************************************
 Name    :  cpd_get_slot_sub_id_from_mds_dest

 Description :  To get the physical slot id from the node id

 Arguments   :

*************************************************************************************/

uns32 cpd_get_slot_sub_slot_id_from_node_id(NCS_NODE_ID i_node_id)
{
	NCS_PHY_SLOT_ID phy_slot;
	NCS_SUB_SLOT_ID sub_slot;

	m_NCS_GET_PHYINFO_FROM_NODE_ID(i_node_id, NULL, &phy_slot, &sub_slot);

	return ((sub_slot * NCS_SUB_SLOT_MAX) + (phy_slot));

}

/*******************************************************************************************
 *
 *  Name  :  cpd_clm_cluster_track_cb
 *
 *  Description : We will get the callback from CLM for the cluster track
 *
 ******************************************************************************************/

void cpd_clm_cluster_track_cb(const SaClmClusterNotificationBufferT *notificationBuffer,
			      SaUint32T numberOfMembers, SaAisErrorT error)
{
	CPD_CB *cb;
	SaClmNodeIdT node_id;
	CPD_CPND_INFO_NODE *cpnd_info_node = NULL;
	NODE_ID key;
	uns32 counter = 0;

	/* 1. Get the CPD_CB */
	m_CPD_RETRIEVE_CB(cb);
	if (cb == NULL) {
		return;
	} else {
		/* 2. Check the HA_STATE */
		for (counter = 0; counter < notificationBuffer->numberOfItems; counter++) {
			if (notificationBuffer->notification[counter].clusterChange == SA_CLM_NODE_LEFT) {
				if (cb->ha_state == SA_AMF_HA_ACTIVE) {
					node_id = notificationBuffer->notification[counter].clusterNode.nodeId;
					key = node_id;
					cpnd_info_node = (CPD_CPND_INFO_NODE *)
					    ncs_patricia_tree_get(&cb->cpnd_tree, (uns8 *)&key);
					if (cpnd_info_node) {
						cpd_process_cpnd_down(cb, &cpnd_info_node->cpnd_dest);
					}
				} else if (cb->ha_state == SA_AMF_HA_STANDBY) {
					node_id = notificationBuffer->notification[counter].clusterNode.nodeId;
					key = node_id;
					cpnd_info_node = (CPD_CPND_INFO_NODE *)
					    ncs_patricia_tree_get(&cb->cpnd_tree, (uns8 *)&key);
					if (cpnd_info_node) {
						cpnd_info_node->timer_state = 2;
					}
				}
			}
		}
	}
	m_CPD_GIVEUP_CB;
	return;
}
