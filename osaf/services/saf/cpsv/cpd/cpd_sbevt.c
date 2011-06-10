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
  FILE NAME: cpd_sbevt.c

  DESCRIPTION: CPD Standby Processing Routines

******************************************************************************/

#include "cpd.h"

     /* This is the function prototype for event handling */

typedef uns32 (*CPDS_EVT_HANDLER) (CPD_CB *cb, CPD_MBCSV_MSG *msg);
uns32 cpd_sb_proc_ckpt_create(CPD_CB *cb, CPD_MBCSV_MSG *msg);
static uns32 cpd_sb_proc_ckpt_unlink(CPD_CB *cb, CPD_MBCSV_MSG *msg);
static uns32 cpd_sb_proc_ckpt_arep_set(CPD_CB *cb, CPD_MBCSV_MSG *msg);
static uns32 cpd_sb_proc_ckpt_rd_set(CPD_CB *cb, CPD_MBCSV_MSG *msg);
static uns32 cpd_sb_proc_ckpt_dest_add(CPD_CB *cb, CPD_MBCSV_MSG *msg);
static uns32 cpd_sb_proc_ckpt_dest_del(CPD_CB *cb, CPD_MBCSV_MSG *msg);
static uns32 cpd_sb_proc_ckpt_usrinfo(CPD_CB *cb, CPD_MBCSV_MSG *msg);
static uns32 cpd_sb_proc_ckpt_dest_down(CPD_CB *cb, CPD_MBCSV_MSG *msg);

const CPDS_EVT_HANDLER cpds_evt_dispatch_tbl[CPD_A2S_MSG_MAX_EVT - CPD_A2S_MSG_BASE] = {
	cpd_sb_proc_ckpt_create,
	cpd_sb_proc_ckpt_dest_add,
	cpd_sb_proc_ckpt_dest_del,
	cpd_sb_proc_ckpt_rd_set,
	cpd_sb_proc_ckpt_arep_set,
	cpd_sb_proc_ckpt_unlink,
	cpd_sb_proc_ckpt_usrinfo,
	cpd_sb_proc_ckpt_dest_down,
};

/***************************************************************************
* Name  :  cpd_process_sb_msg
*
* Description  : To process the message at the Stand by
*
* Arguments : CPD_MBCSV_MSG - Mbcsv message to be sent to Standby

* Return Values :  Success / Error

* Notes : This function will be called from the MBCSv dispatch and also while receiving Async Updates
*********************************************************************************************/
uns32 cpd_process_sb_msg(CPD_CB *cb, CPD_MBCSV_MSG *msg)
{
	uns32 rc = NCSCC_RC_SUCCESS;
	if ((msg->type >= CPD_A2S_MSG_BASE) && (msg->type < CPD_A2S_MSG_MAX_EVT)) {
		rc = cpds_evt_dispatch_tbl[(msg->type) - 1] (cb, msg);
		if (rc != NCSCC_RC_SUCCESS) {
			m_LOG_CPD_CL(CPD_PROCESS_SB_MSG_FAILED, CPD_FC_MBCSV, NCSFL_SEV_ERROR, __FILE__, __LINE__);
		}
	} else {
		m_LOG_CPD_CL(CPD_PROCESS_WRONG_EVENT, CPD_FC_HDLN, NCSFL_SEV_ERROR, __FILE__, __LINE__);
		rc = NCSCC_RC_FAILURE;
	}
	return rc;
}

/***********************************************************************************************************
 * Name        : cpd_sb_proc_ckpt_create
 
 * Description : This is used to create one checkpoint at Standby to  update the 3 databases 

 * Arguments   : CPD_MBCSV_MSG - Mbcsv message 

 * Return Values : Success / Error

 * Notes   : This routine will update the 3 databases at the Standby, and is called at the time of Cold Sync and Async Update of Checkpoint Open API
*****************************************************************************************************************/
uns32 cpd_sb_proc_ckpt_create(CPD_CB *cb, CPD_MBCSV_MSG *msg)
{
	CPD_CPND_INFO_NODE *node_info = NULL;
	CPD_CKPT_INFO_NODE *ckpt_node = NULL;
	CPD_CKPT_MAP_INFO *map_info = NULL;
	CPD_NODE_REF_INFO *nref_info = NULL;
	CPD_CKPT_REF_INFO *cref_info = NULL;
	uns32 rc, proc_rc = NCSCC_RC_SUCCESS;
	NCS_BOOL add_flag = TRUE;
	uns32 count, dest_cnt;
	NODE_ID key;
	SaClmNodeIdT node_id;
	CPD_CKPT_REPLOC_INFO *reploc_info = NULL;
	SaClmClusterNodeT cluster_node;
	CPD_REP_KEY_INFO key_info;

	memset(&cluster_node, 0, sizeof(SaClmClusterNodeT));
	memset(&key_info, 0, sizeof(CPD_REP_KEY_INFO));

	/* 1. check if the checkpoint already exist  (this should not happen ) */
	cpd_ckpt_map_node_get(&cb->ckpt_map_tree, &msg->info.ckpt_create.ckpt_name, &map_info);
/*  msg->info.ckpt_create.ckpt_name.length = m_NCS_OS_NTOHS(msg->info.ckpt_create.ckpt_name.length);     */
	if (map_info == NULL) {	/* Checkpoint does not exist, so allocate memory */
		map_info = m_MMGR_ALLOC_CPD_CKPT_MAP_INFO;
		if (map_info == NULL) {
			m_LOG_CPD_CL(CPD_STANDBY_CREATE_EVT_FAILED, CPD_FC_MBCSV, NCSFL_SEV_ERROR, __FILE__, __LINE__);
			proc_rc = NCSCC_RC_FAILURE;
			goto end;
		}

		ckpt_node = m_MMGR_ALLOC_CPD_CKPT_INFO_NODE;
		if (ckpt_node == NULL) {
			m_LOG_CPD_CL(CPD_STANDBY_CREATE_EVT_FAILED, CPD_FC_MBCSV, NCSFL_SEV_ERROR, __FILE__, __LINE__);
			proc_rc = NCSCC_RC_FAILURE;
			goto ckpt_node_alloc_fail;
		}

	} else {		/* Checkpoint already exists (this should not happen ) */

		goto end;
	}

	/* Fill the Map info structure  */
	memset(map_info, 0, sizeof(CPD_CKPT_MAP_INFO));
	map_info->ckpt_name = msg->info.ckpt_create.ckpt_name;
	map_info->ckpt_id = msg->info.ckpt_create.ckpt_id;
	map_info->attributes = msg->info.ckpt_create.ckpt_attrib;
	cb->nxt_ckpt_id = map_info->ckpt_id + 1;

	/* Adding to the map_info database  */
	proc_rc = cpd_ckpt_map_node_add(&cb->ckpt_map_tree, map_info);
	if (proc_rc != NCSCC_RC_SUCCESS) {
		m_LOG_CPD_FCL(CPD_STANDBY_CREATE_EVT_FAILED, CPD_FC_MBCSV, NCSFL_SEV_ERROR, map_info->ckpt_id, __FILE__,
			      __LINE__);
		goto map_node_add_fail;
	}

	/*  Fill the CKPT_NODE structure  */
	memset(ckpt_node, 0, sizeof(CPD_CKPT_INFO_NODE));
	ckpt_node->ckpt_id = msg->info.ckpt_create.ckpt_id;
	ckpt_node->ckpt_name = msg->info.ckpt_create.ckpt_name;
	dest_cnt = msg->info.ckpt_create.dest_cnt;
	ckpt_node->is_unlink_set = msg->info.ckpt_create.is_unlink_set;
	ckpt_node->attributes = msg->info.ckpt_create.ckpt_attrib;
	ckpt_node->is_active_exists = msg->info.ckpt_create.is_active_exists;
	ckpt_node->active_dest = msg->info.ckpt_create.active_dest;
	ckpt_node->create_time = msg->info.ckpt_create.create_time;
	ckpt_node->num_users = msg->info.ckpt_create.num_users;
	ckpt_node->num_writers = msg->info.ckpt_create.num_writers;
	ckpt_node->num_readers = msg->info.ckpt_create.num_readers;
	ckpt_node->num_sections = msg->info.ckpt_create.num_sections;

	ckpt_node->ret_time = msg->info.ckpt_create.ckpt_attrib.retentionDuration;

	for (count = 0; count < dest_cnt; count++) {
		nref_info = m_MMGR_ALLOC_CPD_NODE_REF_INFO;
		if (nref_info == NULL) {
			m_LOG_CPD_CL(CPD_STANDBY_CREATE_EVT_FAILED, CPD_FC_MBCSV, NCSFL_SEV_ERROR, __FILE__, __LINE__);
			rc = SA_AIS_ERR_NO_MEMORY;
			proc_rc = NCSCC_RC_OUT_OF_MEM;
			goto nref_info_alloc_fail;
		}

		memset(nref_info, '\0', sizeof(CPD_NODE_REF_INFO));
		nref_info->dest = msg->info.ckpt_create.dest_list[count].dest;

		/*  cpd_cpnd database is updated  */
		proc_rc = cpd_cpnd_info_node_find_add(&cb->cpnd_tree, &msg->info.ckpt_create.dest_list[count].dest,
						      &node_info, &add_flag);
		if (!node_info) {
			m_LOG_CPD_FCL(CPD_STANDBY_CREATE_EVT_FAILED, CPD_FC_MBCSV, NCSFL_SEV_ERROR,
				      msg->info.ckpt_create.dest_list[count].dest, __FILE__, __LINE__);
			proc_rc = NCSCC_RC_OUT_OF_MEM;
			if (nref_info)
				m_MMGR_FREE_CPD_NODE_REF_INFO(nref_info);
			goto cpd_cpnd_node_find_fail;
		}
		add_flag = TRUE;
		key = m_NCS_NODE_ID_FROM_MDS_DEST(nref_info->dest);
		node_id = key;
		if (saClmClusterNodeGet(cb->clm_hdl, node_id, CPD_CLM_API_TIMEOUT, &cluster_node) != SA_AIS_OK) {
			proc_rc = NCSCC_RC_FAILURE;
			m_LOG_CPD_LCL(CPD_STANDBY_CREATE_EVT_FAILED, CPD_FC_MBCSV, NCSFL_SEV_ERROR, node_id, __FILE__,
				      __LINE__);
			goto cluster_node_get_fail;
		}

		node_info->node_name = cluster_node.nodeName;

		key_info.ckpt_name = msg->info.ckpt_create.ckpt_name;
		key_info.node_name = cluster_node.nodeName;
		cpd_ckpt_reploc_get(&cb->ckpt_reploc_tree, &key_info, &reploc_info);
		if (reploc_info == NULL) {
			reploc_info = m_MMGR_ALLOC_CPD_CKPT_REPLOC_INFO;

			memset(reploc_info, 0, sizeof(CPD_CKPT_REPLOC_INFO));

			reploc_info->rep_key.node_name = cluster_node.nodeName;
			reploc_info->rep_key.ckpt_name = msg->info.ckpt_create.ckpt_name;

			if (!m_IS_SA_CKPT_CHECKPOINT_COLLOCATED(&msg->info.ckpt_create.ckpt_attrib))
				reploc_info->rep_type = REP_NONCOLL;
			else {
				if ((msg->info.ckpt_create.ckpt_attrib.creationFlags & SA_CKPT_WR_ALL_REPLICAS) &&
				    (m_IS_SA_CKPT_CHECKPOINT_COLLOCATED(&msg->info.ckpt_create.ckpt_attrib)))
					reploc_info->rep_type = REP_SYNCUPD;

				if ((msg->info.ckpt_create.ckpt_attrib.creationFlags & SA_CKPT_WR_ACTIVE_REPLICA) &&
				    (m_IS_SA_CKPT_CHECKPOINT_COLLOCATED(&msg->info.ckpt_create.ckpt_attrib)))
					reploc_info->rep_type = REP_NOTACTIVE;

				if ((msg->info.ckpt_create.ckpt_attrib.creationFlags & SA_CKPT_WR_ACTIVE_REPLICA_WEAK)
				    && (m_IS_SA_CKPT_CHECKPOINT_COLLOCATED(&msg->info.ckpt_create.ckpt_attrib)))
					reploc_info->rep_type = REP_NOTACTIVE;
			}

			cpd_ckpt_reploc_node_add(&cb->ckpt_reploc_tree, reploc_info, cb->ha_state, cb->immOiHandle);

		}
		cref_info = m_MMGR_ALLOC_CPD_CKPT_REF_INFO;

		memset(cref_info, 0, sizeof(CPD_CKPT_REF_INFO));
		cref_info->ckpt_node = ckpt_node;
		cpd_ckpt_ref_info_add(node_info, cref_info);
		cpd_node_ref_info_add(ckpt_node, nref_info);
	}

	/* filling up the ckpt_node database */
	if (cpd_ckpt_node_add(&cb->ckpt_tree, ckpt_node, cb->ha_state, cb->immOiHandle) != NCSCC_RC_SUCCESS) {
		m_LOG_CPD_FCL(CPD_STANDBY_CREATE_EVT_FAILED, CPD_FC_MBCSV, NCSFL_SEV_ERROR, ckpt_node->ckpt_id,
			      __FILE__, __LINE__);
		goto cpd_ckpt_node_add_fail;
	}

	m_LOG_CPD_CFCL(CPD_STANDBY_CREATE_EVT, CPD_FC_MBCSV, NCSFL_SEV_INFO, map_info->ckpt_name.value,
		       map_info->ckpt_id, __FILE__, __LINE__);

	goto end;

	/* Failed some where in the for loop or in ckpt node add */
 cpd_ckpt_node_add_fail:
 cluster_node_get_fail:
 cpd_cpnd_node_find_fail:
 nref_info_alloc_fail:

	if (ckpt_node) {
		cpd_ckpt_node_and_ref_delete(cb, ckpt_node);
		ckpt_node = NULL;
	}

	/* cpd_ckpt_reflist_del(cb, ckpt_node, msg->info.ckpt_create.dest_list);  */
	cpd_ckpt_map_node_delete(cb, map_info);
	map_info = NULL;

 map_node_add_fail:
	if (ckpt_node)
		m_MMGR_FREE_CPD_CKPT_INFO_NODE(ckpt_node);
 ckpt_node_alloc_fail:
	if (map_info)
		m_MMGR_FREE_CPD_CKPT_MAP_INFO(map_info);

 end:
	return proc_rc;
}

/******************************************************************************
* Name        : cpd_sb_proc_ckpt_dest_del
*
* Description : To delete the destination at the Standby 

* Arguments  : CPD_MBCSV_MSG - Mbcsv message

* Return Values : Success / Error

* Notes  : 2 cases : remove node from ckpt_node , destroy the entire node , 
*          ND has deleted the ckpt, -> if it is the only ckpt on the node then delete the ckpt & node from cpd_cpnd
*          --> delete the node ref info from ckpt_node  
*********************************************************************************/
uns32 cpd_sb_proc_ckpt_dest_del(CPD_CB *cb, CPD_MBCSV_MSG *msg)
{
	CPD_CPND_INFO_NODE *node_info = NULL;
	CPD_CKPT_INFO_NODE *ckpt_node = NULL;
	CPD_CKPT_REF_INFO *cref_info = NULL;
	CPD_NODE_REF_INFO *nref_info = NULL;
	CPD_CKPT_MAP_INFO *map_info = NULL;
	uns32 proc_rc = NCSCC_RC_SUCCESS;
	SaNameT ckpt_name, node_name;
	CPD_REP_KEY_INFO key_info;
	CPD_CKPT_REPLOC_INFO *rep_info = NULL;

	memset(&key_info, 0, sizeof(CPD_REP_KEY_INFO));
	memset(&ckpt_name, 0, sizeof(SaNameT));
	memset(&node_name, 0, sizeof(SaNameT));

	/* IF CPND IS DOWN THEN CKPT_ID = 0 , DELETE THAT NODE INFO */
	if (msg->info.dest_del.ckpt_id == 0) {
		cpd_process_cpnd_del(cb, &msg->info.dest_del.mds_dest);
		return proc_rc;
	}

	cpd_ckpt_node_get(&cb->ckpt_tree, &msg->info.dest_del.ckpt_id, &ckpt_node);
	if (ckpt_node == NULL) {
		m_LOG_CPD_FCL(CPD_STANDBY_DESTDEL_EVT_FAILED, CPD_FC_MBCSV, NCSFL_SEV_ERROR, msg->info.dest_del.ckpt_id,
			      __FILE__, __LINE__);
		proc_rc = NCSCC_RC_FAILURE;
		goto fail;
	}

	if (ckpt_node->is_unlink_set != TRUE) {
		cpd_ckpt_map_node_get(&cb->ckpt_map_tree, &ckpt_node->ckpt_name, &map_info);
		/*    ckpt_node->ckpt_name.length = m_NCS_OS_NTOHS(ckpt_node->ckpt_name.length); */
		if (map_info == NULL) {
			m_LOG_CPD_CL(CPD_STANDBY_DESTDEL_EVT_FAILED, CPD_FC_MBCSV, NCSFL_SEV_ERROR, __FILE__, __LINE__);
			return NCSCC_RC_FAILURE;
		}
	}
	cpd_cpnd_info_node_get(&cb->cpnd_tree, &msg->info.dest_del.mds_dest, &node_info);
	if (node_info) {
		for (cref_info = node_info->ckpt_ref_list; cref_info != NULL; cref_info = cref_info->next) {
			if (cref_info->ckpt_node == ckpt_node) {
				cpd_ckpt_ref_info_del(node_info, cref_info);
				break;
			}
		}

		ckpt_name = ckpt_node->ckpt_name;
		key_info.ckpt_name = ckpt_name;
		node_name = node_info->node_name;

		/* No check point ref in this node */
		if (node_info->ckpt_cnt == 0) {
			cpd_cpnd_info_node_delete(cb, node_info);
		}

		key_info.node_name = node_name;
		/*  key_info.node_name.length = m_NCS_OS_NTOHS(node_name.length); */
		cpd_ckpt_reploc_get(&cb->ckpt_reploc_tree, &key_info, &rep_info);
		if (rep_info) {
			cpd_ckpt_reploc_node_delete(cb, rep_info,ckpt_node->is_unlink_set);
		}
	} else {
		m_LOG_CPD_FCL(CPD_STANDBY_DESTDEL_EVT_FAILED, CPD_FC_MBCSV, NCSFL_SEV_ERROR,
			      msg->info.dest_del.mds_dest, __FILE__, __LINE__);
		proc_rc = NCSCC_RC_FAILURE;
		goto fail;
	}
	if (ckpt_node) {
		for (nref_info = ckpt_node->node_list; nref_info != NULL; nref_info = nref_info->next) {
			if (m_NCS_MDS_DEST_EQUAL(&nref_info->dest, &msg->info.dest_del.mds_dest)) {
				if (m_NCS_MDS_DEST_EQUAL(&msg->info.dest_del.mds_dest, &ckpt_node->active_dest)) {
					ckpt_node->is_active_exists = FALSE;
				}
				cpd_node_ref_info_del(ckpt_node, nref_info);
				break;
			}
		}
		if (ckpt_node->dest_cnt == 0) {
			cpd_ckpt_node_delete(cb, ckpt_node);
			if (map_info) {
				cpd_ckpt_map_node_delete(cb, map_info);
			}
		}
	}

 fail:
	return proc_rc;
}

/******************************************************************************
*  Name :  cpd_sb_proc_ckpt_unlink
* 
*  Description : This routine will set the unlink flag of the checkpoint  
*
*  Arguments : CPD_MBCSV_MSG - mbcsv message 

*  Return Values : Success / Error

*  Notes : None
******************************************************************************/
uns32 cpd_sb_proc_ckpt_unlink(CPD_CB *cb, CPD_MBCSV_MSG *msg)
{
	CPD_CKPT_INFO_NODE *ckpt_node = NULL;
	CPD_CKPT_MAP_INFO *map_info = NULL;
	SaNameT *ckpt_name = &msg->info.ckpt_ulink.ckpt_name;
	uns32 proc_rc = SA_AIS_OK;
	uns32 rc = NCSCC_RC_SUCCESS;

	proc_rc = cpd_proc_unlink_set(cb, &ckpt_node, map_info, ckpt_name);
	if (proc_rc != SA_AIS_OK) {
		m_LOG_CPD_CL(CPD_STANDBY_UNLINK_EVT_FAILED, CPD_FC_MBCSV, NCSFL_SEV_ERROR, __FILE__, __LINE__);
		rc = NCSCC_RC_FAILURE;
	}
	m_LOG_CPD_CCL(CPD_EVT_UNLINK_SUCCESS, CPD_FC_HDLN, NCSFL_SEV_INFO, msg->info.ckpt_ulink.ckpt_name.value,
		      __FILE__, __LINE__);
	return rc;
}

/**********************************************************************************
*  Name :  cpd_sb_proc_arep_set
*
*  Description : This routine will set the active replica flag of the checkpoint
*
*  Arguments   : CPD_MBCSV_MSG - mbcsv message 

*  Return Values : Success / Error

*  Notes : None

**********************************************************************************/
uns32 cpd_sb_proc_ckpt_arep_set(CPD_CB *cb, CPD_MBCSV_MSG *msg)
{
	CPD_CKPT_INFO_NODE *ckpt_node = NULL;
	uns32 proc_rc = NCSCC_RC_SUCCESS;
	uns32 rc = SA_AIS_OK;

	rc = cpd_proc_active_set(cb, msg->info.arep_set.ckpt_id, msg->info.arep_set.mds_dest, &ckpt_node);
	if (rc != SA_AIS_OK) {
		proc_rc = NCSCC_RC_FAILURE;
		m_LOG_CPD_CL(CPD_SB_PROC_ACTIVE_SET_FAILED, CPD_FC_MBCSV, NCSFL_SEV_ERROR, __FILE__, __LINE__);
	}
	m_LOG_CPD_FCL(CPD_CKPT_ACTIVE_SET_SUCCESS, CPD_FC_HDLN, NCSFL_SEV_INFO, msg->info.arep_set.ckpt_id,
		      __FILE__, __LINE__);
	return proc_rc;
}

/****************************************************************************************
*  Name  :   cpd_sb_proc_ckpt_rd_set

*  Description : This routine will set the retention time of the checkpoint

*  Arguments : CPD_MBCSV_MSG -  Mbcsv message

*  Return Values : Success / Error
 
*  Notes  : None
****************************************************************************************/
uns32 cpd_sb_proc_ckpt_rd_set(CPD_CB *cb, CPD_MBCSV_MSG *msg)
{
	CPD_CKPT_INFO_NODE *ckpt_node = NULL;
	uns32 proc_rc = SA_AIS_OK;
	uns32 rc = NCSCC_RC_SUCCESS;

	proc_rc = cpd_proc_retention_set(cb, msg->info.rd_set.ckpt_id, msg->info.rd_set.reten_time, &ckpt_node);
	if (proc_rc != SA_AIS_OK) {
		rc = NCSCC_RC_FAILURE;
		m_LOG_CPD_CL(CPD_SB_PROC_RETENTION_SET_FAILED, CPD_FC_MBCSV, NCSFL_SEV_ERROR, __FILE__, __LINE__);
	}

	m_LOG_CPD_FCL(CPD_CKPT_RDSET_SUCCESS, CPD_FC_HDLN, NCSFL_SEV_INFO, msg->info.rd_set.ckpt_id,
		      __FILE__, __LINE__);

	return rc;
}

/*********************************************************************************
*  Name  :   cpd_sb_proc_ckpt_dest_add
*
*  Description : A new checkpoint opened by ND and this information has to be updated here in Standby
*  
*  Arguments : CPD_MBCSV_MSG -  Mbcsv message
*
*  Return Values : Success / Error

*  Notes :  1. add the checkpoint to the Cpd_cpnd database 
*           2. add the node info to the cpd_ckpt database 
*********************************************************************************/

uns32 cpd_sb_proc_ckpt_dest_add(CPD_CB *cb, CPD_MBCSV_MSG *msg)
{
	CPD_CKPT_INFO_NODE *ckpt_node = NULL;
	uns32 proc_rc = NCSCC_RC_SUCCESS;
	CPD_CKPT_REF_INFO *cref_info = NULL;
	CPD_CPND_INFO_NODE *node_info = NULL;
	CPD_NODE_REF_INFO *nref_info = NULL;
	NCS_BOOL add_flag = TRUE;
	NODE_ID key;
	SaClmNodeIdT node_id;
	CPD_CKPT_REPLOC_INFO *reploc_info = NULL;
	SaClmClusterNodeT cluster_node;
	CPD_REP_KEY_INFO key_info;

	memset(&cluster_node, 0, sizeof(SaClmClusterNodeT));
	memset(&key_info, 0, sizeof(CPD_REP_KEY_INFO));

	nref_info = m_MMGR_ALLOC_CPD_NODE_REF_INFO;
	cref_info = m_MMGR_ALLOC_CPD_CKPT_REF_INFO;

	memset(nref_info, 0, sizeof(CPD_NODE_REF_INFO));
	memset(cref_info, 0, sizeof(CPD_CKPT_REF_INFO));

	nref_info->dest = msg->info.dest_add.mds_dest;
	cpd_ckpt_node_get(&cb->ckpt_tree, &msg->info.dest_add.ckpt_id, &ckpt_node);
	if (ckpt_node)
		cpd_node_ref_info_add(ckpt_node, nref_info);
	else {
		m_LOG_CPD_FCL(CPD_STANDBY_DESTADD_EVT_FAILED, CPD_FC_MBCSV, NCSFL_SEV_ERROR, msg->info.dest_add.ckpt_id,
			      __FILE__, __LINE__);
		return NCSCC_RC_OUT_OF_MEM;
	}

	cpd_cpnd_info_node_find_add(&cb->cpnd_tree, &msg->info.dest_add.mds_dest, &node_info, &add_flag);
	if (!node_info) {
		m_LOG_CPD_FCL(CPD_STANDBY_DESTADD_EVT_FAILED, CPD_FC_MBCSV, NCSFL_SEV_ERROR,
			      msg->info.dest_add.mds_dest, __FILE__, __LINE__);
		proc_rc = NCSCC_RC_OUT_OF_MEM;
		goto free_mem;
	}

	key = m_NCS_NODE_ID_FROM_MDS_DEST(msg->info.dest_add.mds_dest);
	node_id = key;
	if (saClmClusterNodeGet(cb->clm_hdl, node_id, CPD_CLM_API_TIMEOUT, &cluster_node) != SA_AIS_OK) {
		proc_rc = NCSCC_RC_FAILURE;
		m_LOG_CPD_LCL(CPD_STANDBY_DESTADD_EVT_FAILED, CPD_FC_MBCSV, NCSFL_SEV_ERROR, node_id, __FILE__,
			      __LINE__);
		goto free_mem;
	}

	node_info->node_name = cluster_node.nodeName;

	key_info.node_name = cluster_node.nodeName;
	key_info.ckpt_name = ckpt_node->ckpt_name;
	cpd_ckpt_reploc_get(&cb->ckpt_reploc_tree, &key_info, &reploc_info);
	if (reploc_info == NULL) {
		reploc_info = m_MMGR_ALLOC_CPD_CKPT_REPLOC_INFO;
		if (reploc_info == NULL)
			goto free_mem;

		memset(reploc_info, 0, sizeof(CPD_CKPT_REPLOC_INFO));

		reploc_info->rep_key.node_name = cluster_node.nodeName;
		reploc_info->rep_key.ckpt_name = ckpt_node->ckpt_name;
		if (!m_IS_SA_CKPT_CHECKPOINT_COLLOCATED(&ckpt_node->attributes))
			reploc_info->rep_type = REP_NONCOLL;
		else {
			if ((ckpt_node->attributes.creationFlags & SA_CKPT_WR_ALL_REPLICAS) &&
			    (m_IS_SA_CKPT_CHECKPOINT_COLLOCATED(&ckpt_node->attributes)))
				reploc_info->rep_type = REP_SYNCUPD;

			if ((ckpt_node->attributes.creationFlags & SA_CKPT_WR_ACTIVE_REPLICA) &&
			    (m_IS_SA_CKPT_CHECKPOINT_COLLOCATED(&ckpt_node->attributes)))
				reploc_info->rep_type = REP_NOTACTIVE;

			if ((ckpt_node->attributes.creationFlags & SA_CKPT_WR_ACTIVE_REPLICA_WEAK) &&
			    (m_IS_SA_CKPT_CHECKPOINT_COLLOCATED(&ckpt_node->attributes)))
				reploc_info->rep_type = REP_NOTACTIVE;
		}

		proc_rc = cpd_ckpt_reploc_node_add(&cb->ckpt_reploc_tree, reploc_info, cb->ha_state, cb->immOiHandle);
		if (proc_rc != NCSCC_RC_SUCCESS) {
			m_LOG_CPD_CL(CPD_STANDBY_DESTADD_EVT_FAILED, CPD_FC_MBCSV, NCSFL_SEV_INFO, __FILE__, __LINE__);
			/*  goto free_mem; */
		}
	}

	cref_info->ckpt_node = ckpt_node;
	cpd_ckpt_ref_info_add(node_info, cref_info);

	m_LOG_CPD_FFCL(CPD_STANDBY_DESTADD_EVT_SUCCESS, CPD_FC_MBCSV, NCSFL_SEV_INFO, msg->info.dest_add.ckpt_id,
		       msg->info.dest_add.mds_dest, __FILE__, __LINE__);

 free_mem:
	if (node_info == NULL) {
		cpd_node_ref_info_del(ckpt_node, nref_info);
		m_MMGR_FREE_CPD_NODE_REF_INFO(nref_info);
		m_MMGR_FREE_CPD_CKPT_REF_INFO(cref_info);
	}
	return proc_rc;
}

/**********************************************************************************
*  Name :  cpd_sb_proc_ckpt_usrinfo
*
*  Description : This routine will set the active replica flag of the checkpoint
*
*  Arguments   : CPD_MBCSV_MSG - mbcsv message
                                                                                                                             
*  Return Values : Success / Error
                                                                                                                             
*  Notes : None
                                                                                                                             
**********************************************************************************/
uns32 cpd_sb_proc_ckpt_usrinfo(CPD_CB *cb, CPD_MBCSV_MSG *msg)
{
	CPD_CKPT_INFO_NODE *ckpt_node = NULL;

	cpd_ckpt_node_get(&cb->ckpt_tree, &msg->info.dest_add.ckpt_id, &ckpt_node);
	if (ckpt_node == NULL) {
		m_LOG_CPD_FCL(CPD_CKPT_INFO_NODE_GET_FAILED, CPD_FC_HDLN, NCSFL_SEV_ERROR, msg->info.dest_add.ckpt_id,
			      __FILE__, __LINE__);
		return NCSCC_RC_FAILURE;
	}

	ckpt_node->num_users = msg->info.usr_info.num_user;
	ckpt_node->num_writers = msg->info.usr_info.num_writer;
	ckpt_node->num_readers = msg->info.usr_info.num_reader;
	ckpt_node->num_sections = msg->info.usr_info.num_sections;
	ckpt_node->ckpt_on_scxb1 = msg->info.usr_info.ckpt_on_scxb1;
	ckpt_node->ckpt_on_scxb2 = msg->info.usr_info.ckpt_on_scxb2;

	return NCSCC_RC_SUCCESS;
}

/**********************************************************************************
*  Name :  cpd_sb_proc_ckpt_dest_down
*
*  Description : This routine will start the timer
*
*  Arguments   : CPD_MBCSV_MSG - mbcsv message
                                                                                                                             
*  Return Values : Success / Error
                                                                                                                             
*  Notes : None
                                                                                                                             
**********************************************************************************/

uns32 cpd_sb_proc_ckpt_dest_down(CPD_CB *cb, CPD_MBCSV_MSG *msg)
{
	CPD_CPND_INFO_NODE *cpnd_info = NULL;
	TRACE("THIS IS IN DEST DOWN OF NODE 1");
	cpd_cpnd_info_node_get(&cb->cpnd_tree, &msg->info.dest_down.mds_dest, &cpnd_info);
	if (cpnd_info) {
		cpnd_info->cpnd_ret_timer.type = CPD_TMR_TYPE_CPND_RETENTION;
		cpnd_info->cpnd_ret_timer.info.cpnd_dest = msg->info.dest_down.mds_dest;
		cpd_tmr_start(&cpnd_info->cpnd_ret_timer, CPD_CPND_DOWN_RETENTION_TIME);
		return NCSCC_RC_SUCCESS;
	} else
		return NCSCC_RC_FAILURE;
}
