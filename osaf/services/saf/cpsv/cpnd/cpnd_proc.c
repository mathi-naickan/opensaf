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
  FILE NAME: cpcn_evt.c

  DESCRIPTION: CPND Event handling routines

  FUNCTIONS INCLUDED in this module:
  cpnd_ckpt_client_add............Add the new client info to ckpt info.
  cpnd_proc_colloc_ckpt_open .....Processing for collocated ckpt open.
  cpnd_proc_noncolloc_ckpt_open...Processing for non-collocated ckpt open.

******************************************************************************/

#include "cpnd.h"
extern uns32 gl_read_lck;
static void cpnd_dump_ckpt_info(CPND_CKPT_NODE *ckpt_node);
static void cpnd_dump_client_info(CPND_CKPT_CLIENT_NODE *cl_node);
static void cpnd_dump_replica_info(CPND_CKPT_REPLICA_INFO *ckpt_replica_node);
static void cpnd_dump_section_info(CPND_CKPT_SECTION_INFO *sec_info);
static void cpnd_dump_shm_info(NCS_OS_POSIX_SHM_REQ_INFO *open);
static void cpnd_dump_ckpt_attri(CPND_CKPT_NODE *cp_node);

/****************************************************************************
 * Name          : cpnd_ckpt_client_add
 *
 * Description   : This routine will add the client details to the existing
 *                 ckpt info
 *
 * Return Values : NCSCC_RC_SUCCESS/Error.
 *
 * Notes         : None.
 *****************************************************************************/
uns32 cpnd_ckpt_client_add(CPND_CKPT_NODE *cp_node, CPND_CKPT_CLIENT_NODE *cl_node)
{
	uns32 rc = NCSCC_RC_SUCCESS;
	CPND_CKPT_CLLIST_NODE *clist_node;
	CPND_CKPT_CLLIST_NODE *ptr_cl_node = cp_node->clist;

	while (ptr_cl_node) {
		if (ptr_cl_node->cnode->ckpt_app_hdl == cl_node->ckpt_app_hdl) {
			ptr_cl_node->cl_ref_cnt++;
			cl_node->upd_shm = FALSE;
			return rc;
		} else {
			if (ptr_cl_node->next == NULL)
				break;
			else
				ptr_cl_node = ptr_cl_node->next;
		}
	}

	clist_node = m_MMGR_ALLOC_CPND_CKPT_CLIST_NODE;
	memset(clist_node, '\0', sizeof(CPND_CKPT_CLLIST_NODE));

	clist_node->next = ptr_cl_node;
	clist_node->cnode = cl_node;
	clist_node->cl_ref_cnt = 1;
	cp_node->clist = clist_node;
	cl_node->upd_shm = TRUE;

	return rc;

}

/****************************************************************************
 * Name          : cpnd_ckpt_client_find
 *
 * Description   : This routine will find the client details to the existing
 *                 ckpt info
 *
 * Return Values : NCSCC_RC_SUCCESS/Error.
 *
 * Notes         : None.
 *****************************************************************************/
uns32 cpnd_ckpt_client_find(CPND_CKPT_NODE *cp_node, CPND_CKPT_CLIENT_NODE *cl_node)
{
	CPND_CKPT_CLLIST_NODE *ptr_cl_node = cp_node->clist;
	uns32 rc = NCSCC_RC_SUCCESS;

	while (ptr_cl_node != NULL) {
		if (ptr_cl_node->cnode == cl_node)
			return rc;
	}

	return NCSCC_RC_FAILURE;
}

/****************************************************************************
 * Name          : cpnd_ckpt_client_del
 *
 * Description   : This routine will del the client details to the existing
 *                 ckpt info
 *
 * Return Values : NCSCC_RC_SUCCESS/Error.
 *
 * Notes         : None.
 *****************************************************************************/
uns32 cpnd_ckpt_client_del(CPND_CKPT_NODE *cp_node, CPND_CKPT_CLIENT_NODE *cl_node)
{

	CPND_CKPT_CLLIST_NODE *ptr_cl_node = cp_node->clist;
	CPND_CKPT_CLLIST_NODE *prev_ptr_cl_node = NULL;

	while (ptr_cl_node != NULL) {
		if (ptr_cl_node->cnode == cl_node)
			break;
		prev_ptr_cl_node = ptr_cl_node;
		ptr_cl_node = ptr_cl_node->next;

	}
	if (ptr_cl_node == NULL) {
		return NCSCC_RC_FAILURE;
	} else {
		ptr_cl_node->cl_ref_cnt--;

		if (ptr_cl_node->cl_ref_cnt) {
			return NCSCC_RC_SUCCESS;
		}

		if (prev_ptr_cl_node == NULL) {
			cp_node->clist = ptr_cl_node->next;
		} else {
			prev_ptr_cl_node->next = ptr_cl_node->next;
		}
	}

	m_MMGR_FREE_CPND_CKPT_CLIST_NODE(ptr_cl_node);
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
 * Name          : cpnd_client_ckpt_info_add;
 *
 * Description   : This routine will add the ckpt referenced by this 
 *                 client.
 *
 * Return Values : NCSCC_RC_SUCCESS/Error.
 *
 * Notes         : None.
 *****************************************************************************/
uns32 cpnd_client_ckpt_info_add(CPND_CKPT_CLIENT_NODE *cl_node, CPND_CKPT_NODE *cp_node)
{
	uns32 rc = NCSCC_RC_SUCCESS;

	CPND_CKPT_CKPT_LIST_NODE *cplist = NULL;
	CPND_CKPT_CKPT_LIST_NODE *ptr_cp_node = cl_node->ckpt_list;

	cplist = m_MMGR_ALLOC_CPND_CKPT_LIST_NODE;

	/* need to check */
	memset(cplist, '\0', sizeof(CPND_CKPT_CKPT_LIST_NODE));

	cplist->next = ptr_cp_node;
	cplist->cnode = cp_node;

	cl_node->ckpt_list = cplist;

	return rc;

}

/****************************************************************************
 * Name          : cpnd_client_ckpt_info_del.
 *
 * Description   : This routine will del the ckpt refrence info
 *                 from this client.
 *
 * Return Values : NCSCC_RC_SUCCESS/Error.
 *
 * Notes         : None.
 *****************************************************************************/
uns32 cpnd_client_ckpt_info_del(CPND_CKPT_CLIENT_NODE *cl_node, CPND_CKPT_NODE *cp_node)
{

	CPND_CKPT_CKPT_LIST_NODE *ptr_cp_node = cl_node->ckpt_list;
	CPND_CKPT_CKPT_LIST_NODE *prev_ptr_cp_node = NULL;

	while (ptr_cp_node != NULL) {
		if (ptr_cp_node->cnode == cp_node)
			break;
		prev_ptr_cp_node = ptr_cp_node;
		ptr_cp_node = ptr_cp_node->next;
	}
	if (ptr_cp_node == NULL) {
		return NCSCC_RC_FAILURE;
	} else {
		if (prev_ptr_cp_node == NULL)
			cl_node->ckpt_list = ptr_cp_node->next;
		else
			prev_ptr_cp_node->next = ptr_cp_node->next;
	}
	m_MMGR_FREE_CPND_CKPT_LIST_NODE(ptr_cp_node);
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
 * Name          : cpnd_client_ckpt_info_delete.
 *
 * Description   : This routine will delete all the ckpt refrence info
 *                 from this client.
 *
 * Return Values : NCSCC_RC_SUCCESS/Error.
 *
 * Notes         : None.
 *****************************************************************************/
uns32 cpnd_client_ckpt_info_delete(CPND_CKPT_CLIENT_NODE *cl_node, CPND_CKPT_NODE *cp_node)
{

	CPND_CKPT_CKPT_LIST_NODE *ptr_cp_node = cl_node->ckpt_list;
	CPND_CKPT_CKPT_LIST_NODE *prev_ptr_cp_node = NULL, *free_cp_node = NULL;;

	while (ptr_cp_node != NULL) {

		if (ptr_cp_node->cnode == cp_node) {
			free_cp_node = ptr_cp_node;
			ptr_cp_node = ptr_cp_node->next;

			if (prev_ptr_cp_node == NULL)
				cl_node->ckpt_list = free_cp_node->next;
			else
				prev_ptr_cp_node->next = free_cp_node->next;

			m_MMGR_FREE_CPND_CKPT_LIST_NODE(free_cp_node);
		} else {
			prev_ptr_cp_node = ptr_cp_node;
			ptr_cp_node = ptr_cp_node->next;
		}
	}
	return NCSCC_RC_SUCCESS;
}

/**************************************************************************
 * Name         : cpnd_proc_ckpt_arrival_info_ntfy
 *
 * Description  :
 *
 *************************************************************************/
uns32 cpnd_proc_ckpt_arrival_info_ntfy(CPND_CB *cb, CPND_CKPT_NODE *cp_node, CPSV_CKPT_ACCESS *in_evt,
				       CPSV_SEND_INFO *sinfo)
{
	CPND_CKPT_CLLIST_NODE *ptr_cl_node = cp_node->clist;
	CPSV_EVT send_evt;
	memset(&send_evt, '\0', sizeof(CPSV_EVT));
	uns32 rc = NCSCC_RC_SUCCESS;

	while (ptr_cl_node != NULL) {
		if (ptr_cl_node->cnode->arrival_cb_flag == TRUE) {
			send_evt.type = CPSV_EVT_TYPE_CPA;
			send_evt.info.cpa.type = CPA_EVT_ND2A_CKPT_ARRIVAL_NTFY;
			send_evt.info.cpa.info.arr_msg.client_hdl = ptr_cl_node->cnode->ckpt_app_hdl;
			send_evt.info.cpa.info.arr_msg.ckpt_hdl = cp_node->ckpt_id;
			send_evt.info.cpa.info.arr_msg.lcl_ckpt_hdl = in_evt->lcl_ckpt_id;
			send_evt.info.cpa.info.arr_msg.mdest = in_evt->agent_mdest;
			send_evt.info.cpa.info.arr_msg.num_of_elmts = in_evt->num_of_elmts;
			send_evt.info.cpa.info.arr_msg.ckpt_data = in_evt->data;

			rc = cpnd_mds_msg_send(cb, NCSMDS_SVC_ID_CPA, ptr_cl_node->cnode->agent_mds_dest, &send_evt);
		}
		ptr_cl_node = ptr_cl_node->next;
	}
	return rc;
}

uns32 cpnd_proc_ckpt_clm_node_left(CPND_CB *cb)
{
	CPSV_EVT send_evt;
	memset(&send_evt, '\0', sizeof(CPSV_EVT));
	uns32 rc = NCSCC_RC_SUCCESS;
	cb->is_joined_cl = FALSE;
	send_evt.type = CPSV_EVT_TYPE_CPA;
	send_evt.info.cpa.type = CPA_EVT_ND2A_CKPT_CLM_NODE_LEFT;
	rc = cpnd_mds_bcast_send(cb, &send_evt, NCSMDS_SVC_ID_CPA);
	return rc;
}

uns32 cpnd_proc_ckpt_clm_node_joined(CPND_CB *cb)
{
	CPSV_EVT send_evt;
	memset(&send_evt, '\0', sizeof(CPSV_EVT));
	uns32 rc = NCSCC_RC_SUCCESS;
	cb->is_joined_cl = TRUE;
	send_evt.type = CPSV_EVT_TYPE_CPA;
	send_evt.info.cpa.type = CPA_EVT_ND2A_CKPT_CLM_NODE_JOINED;
	rc = cpnd_mds_bcast_send(cb, &send_evt, NCSMDS_SVC_ID_CPA);
	return rc;
}

/****************************************************************************
 * Name          : cpnd_ckpt_replica_destroy
 *
 * Description   : This routine will create the replica for the non collocated 
 *                 ckpt, and send the details to CPD. Then sync the local 
 *                 replica with the active replica.
 *
 * Return Values : NCSCC_RC_SUCCESS/Error.
 *
 * Notes         : None.
 *****************************************************************************/
uns32 cpnd_ckpt_replica_destroy(CPND_CB *cb, CPND_CKPT_NODE *cp_node, SaAisErrorT *error)
{
	NCS_OS_POSIX_SHM_REQ_INFO shm_info;
	CPSV_EVT evt, *out_evt = NULL;
	uns32 rc = NCSCC_RC_SUCCESS;
	CPND_CPD_DEFERRED_REQ_NODE *node = NULL;

	if (cp_node->cpnd_rep_create) {

		/* First delete all sections in the heckpoint about to be deleted */
		cpnd_ckpt_delete_all_sect(cp_node);

		memset(&shm_info, '\0', sizeof(shm_info));

		shm_info.type = NCS_OS_POSIX_SHM_REQ_CLOSE;
		shm_info.info.close.i_addr = cp_node->replica_info.open.info.open.o_addr;
		shm_info.info.close.i_fd = cp_node->replica_info.open.info.open.o_fd;
		shm_info.info.close.i_hdl = cp_node->replica_info.open.info.open.o_hdl;
		shm_info.info.close.i_size = cp_node->replica_info.open.info.open.i_size;

		rc = ncs_os_posix_shm(&shm_info);

		if (rc == NCSCC_RC_FAILURE) {
			m_LOG_CPND_FCL(CPND_CKPT_CLOSE_FAILED, CPND_FC_SYS_CALL, NCSFL_SEV_ERROR,
				       cp_node->ckpt_id, __FILE__, __LINE__);
			*error = SA_AIS_ERR_LIBRARY;
			return rc;
		}

		/* unlink the name */
		shm_info.type = NCS_OS_POSIX_SHM_REQ_UNLINK;
		shm_info.info.unlink.i_name = cp_node->replica_info.open.info.open.i_name;

		rc = ncs_os_posix_shm(&shm_info);

		if (rc == NCSCC_RC_FAILURE) {
			m_LOG_CPND_FCL(CPND_CKPT_UNLINK_FAILED, CPND_FC_SYS_CALL, NCSFL_SEV_ERROR,
				       cp_node->ckpt_id, __FILE__, __LINE__);
			*error = SA_AIS_ERR_LIBRARY;
			return rc;
		}

		if (cb->num_rep)
			cb->num_rep--;

		m_MMGR_FREE_CPND_DEFAULT(cp_node->replica_info.open.info.open.i_name);

		/* freeing the sec_mapping memory */
		if (cp_node->replica_info.shm_sec_mapping)
			m_MMGR_FREE_CPND_DEFAULT(cp_node->replica_info.shm_sec_mapping);
	}

	/* send destroy evt to CPD */
	memset(&evt, '\0', sizeof(CPSV_EVT));

	evt.type = CPSV_EVT_TYPE_CPD;
	evt.info.cpd.type = CPD_EVT_ND2D_CKPT_DESTROY;
	evt.info.cpd.info.ckpt_destroy.ckpt_id = cp_node->ckpt_id;

	rc = cpnd_mds_msg_sync_send(cb, NCSMDS_SVC_ID_CPD, cb->cpd_mdest_id, &evt, &out_evt, CPSV_WAIT_TIME);

	if (rc != NCSCC_RC_SUCCESS) {
		m_LOG_CPND_LCL(CPND_MDS_SEND_FAIL, CPND_FC_HDLN, NCSFL_SEV_ERROR, rc, __FILE__, __LINE__);
		m_NCS_LOCK(&cb->cpnd_cpd_up_lock, NCS_LOCK_WRITE);

		if ((rc == NCSCC_RC_REQ_TIMOUT) || ((rc == NCSCC_RC_FAILURE) && (!cb->is_cpd_up))) {

			node = (CPND_CPD_DEFERRED_REQ_NODE *)m_MMGR_ALLOC_CPND_CPD_DEFERRED_REQ_NODE;
			if (!node) {
				m_LOG_CPND_CL(CPND_CPD_DEFERRED_REQ_NODE_ALLOC_FAILED, CPND_FC_MEMFAIL, NCSFL_SEV_ERROR,
					      __FILE__, __LINE__);
				m_NCS_UNLOCK(&cb->cpnd_cpd_up_lock, NCS_LOCK_WRITE);
				*error = SA_AIS_ERR_NO_MEMORY;
				return NCSCC_RC_FAILURE;
			}

			memset(node, '\0', sizeof(CPND_CPD_DEFERRED_REQ_NODE));
			node->evt = evt;

			ncs_enqueue(&cb->cpnd_cpd_deferred_reqs_list, (void *)node);
		} else {
			m_NCS_UNLOCK(&cb->cpnd_cpd_up_lock, NCS_LOCK_WRITE);
			*error = SA_AIS_ERR_TRY_AGAIN;
			return NCSCC_RC_FAILURE;
		}
		m_NCS_UNLOCK(&cb->cpnd_cpd_up_lock, NCS_LOCK_WRITE);
	}

	cpnd_evt_destroy(out_evt);

	*error = SA_AIS_OK;
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
 * Name          : cpnd_ckpt_replica_create
 *
 * Description   : Function to process the   
 *                 from Applications. 
 *
 * Arguments     : CPND_CB *cb - CPND CB pointer
 *                 CPND_CKPT_NODE - ckpt node pointer 
 *
 * Return Values : NCSCC_RC_SUCCESS/Error.
 *
 * Notes         : None.
 *****************************************************************************/
uns32 cpnd_ckpt_replica_create(CPND_CB *cb, CPND_CKPT_NODE *cp_node)
{

	uns32 rc = NCSCC_RC_SUCCESS;
	char *buf;
	uns8 size = 0, total_length;
	int32 sec_cnt = 0;

	/* Return Error no resource */
	if (!(cb->num_rep < CPND_MAX_REPLICAS)) {
		return NCSCC_RC_FAILURE;
	}

	size = cp_node->ckpt_name.length;
	total_length = size + sizeof(cp_node->ckpt_id) + sizeof(NODE_ID) + 5;

	buf = m_MMGR_ALLOC_CPND_DEFAULT(total_length);
	memset(buf, '\0', total_length);
	strncpy(buf, (char *)cp_node->ckpt_name.value, size);

	sprintf(buf + size - 1, "_%d_%llu", (uns32)m_NCS_NODE_ID_FROM_MDS_DEST(cb->cpnd_mdest_id), cp_node->ckpt_id);
	/* size of chkpt */
	memset(&cp_node->replica_info.open, '\0', sizeof(cp_node->replica_info.open));

	cp_node->replica_info.open.type = NCS_OS_POSIX_SHM_REQ_OPEN;
	cp_node->replica_info.open.info.open.i_size =
	    sizeof(CPSV_CKPT_HDR) + cp_node->create_attrib.maxSections * (sizeof(CPSV_SECT_HDR) +
									  cp_node->create_attrib.maxSectionSize);
	cp_node->replica_info.open.info.open.i_offset = 0;
	cp_node->replica_info.open.info.open.i_name = buf;
	cp_node->replica_info.open.info.open.i_map_flags = MAP_SHARED;
	cp_node->replica_info.open.info.open.o_addr = NULL;
	cp_node->replica_info.open.info.open.i_flags = O_RDWR;

 again:
	rc = ncs_os_posix_shm(&cp_node->replica_info.open);

	if (rc == NCSCC_RC_FAILURE) {
		if ((cp_node->replica_info.open.info.open.i_flags & O_RDWR) &&
		    (cp_node->replica_info.open.info.open.i_flags & O_CREAT)) {
			m_LOG_CPND_LCL(CPND_CKPT_REP_CREATE_FAILED, CPND_FC_GENERIC, NCSFL_SEV_ERROR,
				       rc, __FILE__, __LINE__);
			return NCSCC_RC_FAILURE;
		} else {
			/* the replica does not exist, we need to create it */
			cp_node->replica_info.open.info.open.i_flags = O_RDWR | O_CREAT;
			goto again;
		}
	}

	if (cp_node->replica_info.open.info.open.i_flags & O_CREAT)
		cb->num_rep++;

	cp_node->replica_info.shm_sec_mapping =
	    (uns32 *)m_MMGR_ALLOC_CPND_DEFAULT(sizeof(uns32) * cp_node->create_attrib.maxSections);

	for (; sec_cnt < cp_node->create_attrib.maxSections; sec_cnt++)
		cp_node->replica_info.shm_sec_mapping[sec_cnt] = 1;

	return rc;

}

/****************************************************************************
 * Name          : cpnd_ckpt_remote_cpnd_add
 *
 * Description   : This routine will add the remote cpnd mdest to the existing
 *                 ckpt info
 *
 * Return Values : NCSCC_RC_SUCCESS/Error.
 *
 * Notes         : None.
 *****************************************************************************/
uns32 cpnd_ckpt_remote_cpnd_add(CPND_CKPT_NODE *cp_node, MDS_DEST mds_info)
{

	uns32 rc = NCSCC_RC_SUCCESS;
	CPSV_CPND_DEST_INFO *cpnd_mdest;
	CPSV_CPND_DEST_INFO *ptr_cpnd_mdest = cp_node->cpnd_dest_list;
	CPSV_CPND_DEST_INFO *cpnd_mdest_trav = cp_node->cpnd_dest_list;

	while (cpnd_mdest_trav) {
		if (m_CPND_IS_LOCAL_NODE(&cpnd_mdest_trav->dest, &mds_info) == 0)
			return rc;
		else
			cpnd_mdest_trav = cpnd_mdest_trav->next;
	}

	cpnd_mdest = m_MMGR_ALLOC_CPND_DEST_INFO;
	memset(cpnd_mdest, '\0', sizeof(CPSV_CPND_DEST_INFO));

	cpnd_mdest->next = ptr_cpnd_mdest;
	cpnd_mdest->dest = mds_info;

	cp_node->cpnd_dest_list = cpnd_mdest;
	m_LOG_CPND_FFCL(CPND_REP_ADD_SUCCESS, CPND_FC_API, NCSFL_SEV_INFO,
			cp_node->ckpt_id, mds_info, __FILE__, __LINE__);

	return rc;

}

/****************************************************************************
 * Name          : cpnd_ckpt_remote_cpnd_del
 *
 * Description   : This routine will delete the remote cpnd details to the existing
 *                 ckpt info
 *
 * Return Values : NCSCC_RC_SUCCESS/Error.
 *
 * Notes         : None.
 *****************************************************************************/
uns32 cpnd_ckpt_remote_cpnd_del(CPND_CKPT_NODE *cp_node, MDS_DEST mds_info)
{

	CPSV_CPND_DEST_INFO *ptr_cpnd_mdest = cp_node->cpnd_dest_list;
	CPSV_CPND_DEST_INFO *prev_ptr_cpnd_mdest = NULL;

	while (ptr_cpnd_mdest != NULL) {

		if (m_CPND_IS_LOCAL_NODE(&ptr_cpnd_mdest->dest, &mds_info) == 0)
			break;

		prev_ptr_cpnd_mdest = ptr_cpnd_mdest;
		ptr_cpnd_mdest = ptr_cpnd_mdest->next;

	}
	if (ptr_cpnd_mdest == NULL) {
		m_LOG_CPND_FCL(CPND_RMT_CPND_ENTRY_DOES_NOT_EXIST, CPND_FC_API, NCSFL_SEV_ERROR, mds_info,
			       __FILE__, __LINE__);
		return NCSCC_RC_FAILURE;
	} else {
		if (prev_ptr_cpnd_mdest == NULL)
			cp_node->cpnd_dest_list = ptr_cpnd_mdest->next;
		else
			prev_ptr_cpnd_mdest->next = ptr_cpnd_mdest->next;
	}

	m_MMGR_FREE_CPND_DEST_INFO(ptr_cpnd_mdest);
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
 * Name          :  cpnd_ckpt_get_lck_sec_id
 *
 * Description   : Function to process the   
 *                 from Applications. 
 *
 * Arguments     : CPND_CKPT_NODE *cp_node  - CPND CKPT pointer
 *
 * Return Values : NCSCC_RC_SUCCESS/Error.
 *
 * Notes         : None.
 *****************************************************************************/
int32 cpnd_ckpt_get_lck_sec_id(CPND_CKPT_NODE *cp_node)
{
	uns32 i = 0;

	for (; i < cp_node->create_attrib.maxSections; i++)
		if (cp_node->replica_info.shm_sec_mapping[i] == 1) {
			cp_node->replica_info.shm_sec_mapping[i] = 0;
			break;
		}
	if (i == cp_node->create_attrib.maxSections) {
		return -1;
	}

	return i;
}

/****************************************************************************
 * Name          : cpnd_ckpt_sec_write
 *
 * Description   : Function to process the   
 *                 from Applications. 
 *
 * Arguments     : CPND_CB *cb - CPND CB pointer
 *
 * Return Values : NCSCC_RC_SUCCESS/Error.
 *
 * Notes         : None.
 *****************************************************************************/
uns32 cpnd_ckpt_sec_write(CPND_CKPT_NODE *cp_node, CPND_CKPT_SECTION_INFO
			  *sec_info, const void *data, uns32 size, uns32 offset, uns32 type)
{				/* for sync type=2 */
	uns32 rc = NCSCC_RC_SUCCESS;
	NCS_OS_POSIX_SHM_REQ_INFO write_req;

	/* checking to write,whether it is possible to write or not */

	if (type == 0) {
		if (offset + size > cp_node->create_attrib.maxSectionSize) {
			return NCSCC_RC_FAILURE;
		}
	} else if (type == 1) {	/* Over write Case */
		if (size > cp_node->create_attrib.maxSectionSize) {
			return NCSCC_RC_FAILURE;
		}
	}

	write_req.type = NCS_OS_POSIX_SHM_REQ_WRITE;
	write_req.info.write.i_addr = (void *)((char *)cp_node->replica_info.open.info.open.o_addr +
					       sizeof(CPSV_CKPT_HDR) +
					       ((sec_info->lcl_sec_id + 1) * sizeof(CPSV_SECT_HDR)) +
					       (sec_info->lcl_sec_id * cp_node->create_attrib.maxSectionSize));
	write_req.info.write.i_from_buff = (uns8 *)data;

	/* if ( type == 0) Needs to be cleaned up later TBD 
	   write_req.info.write.i_offset=sec_info->size + offset;
	   else */
	write_req.info.write.i_offset = offset;

	write_req.info.write.i_write_size = size;

	ncs_os_posix_shm(&write_req);

	m_GET_TIME_STAMP(sec_info->lastUpdate);

	if (type == 0) {
		if (sec_info->sec_size < offset + size) {
			cp_node->replica_info.mem_used =
			    cp_node->replica_info.mem_used + ((offset + size) - sec_info->sec_size);
			sec_info->sec_size = offset + size;
		}

		/* SECTION HEADER UPDATE */
		cpnd_sec_hdr_update(sec_info, cp_node);

	} else if ((type == 1) || (type == 3)) {
		cp_node->replica_info.mem_used -= sec_info->sec_size;
		sec_info->sec_size = size;
		cp_node->replica_info.mem_used += size;
		cpnd_sec_hdr_update(sec_info, cp_node);
	}
	return rc;

}

/****************************************************************************
 * Name          : cpnd_ckpt_sec_read
 *
 * Description   : Function to process the   
 *                 from Applications. 
 *
 * Arguments     : CPND_CB *cb - CPND CB pointer
 *                 CPSV_EVT *evt - Received Event structure
 *                 CPSV_SEND_INFO *sinfo - Sender MDS information.
 *
 * Return Values : NCSCC_RC_SUCCESS/Error.
 *
 * Notes         : None.
 *****************************************************************************/
uns32 cpnd_ckpt_sec_read(CPND_CKPT_NODE *cp_node, CPND_CKPT_SECTION_INFO
			 *sec_info, void *data, uns32 size, uns32 offset)
{

	uns32 rc = NCSCC_RC_SUCCESS;
	NCS_OS_POSIX_SHM_REQ_INFO read_req;

	read_req.type = NCS_OS_POSIX_SHM_REQ_READ;
	read_req.info.read.i_addr = (void *)((char *)cp_node->replica_info.open.info.open.o_addr +
					     sizeof(CPSV_CKPT_HDR) +
					     ((sec_info->lcl_sec_id + 1) * sizeof(CPSV_SECT_HDR)) +
					     (sec_info->lcl_sec_id * cp_node->create_attrib.maxSectionSize));
	read_req.info.read.i_to_buff = data;
	read_req.info.read.i_read_size = size;
	read_req.info.read.i_offset = offset;

	rc = ncs_os_posix_shm(&read_req);

	return rc;

}

/****************************************************************************
 * Name          : cpnd_proc_cpd_down 
 *
 * Description   : Function to handle Director going down
 *
 * Arguments     : CPND_CB *cb - CPND CB pointer
 *
 * Return Values : NCSCC_RC_SUCCESS/Error.
 * Notes         : Policy used for handling cpd down is to blindly cleanup cpnd_cb 
 *****************************************************************************/
void cpnd_proc_cpd_down(CPND_CB *cb)
{
	/* cleanup ckpt node tree */
	cpnd_ckpt_node_tree_cleanup(cb);

	/* cleanup client node tree */
	cpnd_client_node_tree_cleanup(cb);
}

/****************************************************************************
 * Name          : cpnd_proc_cpa_down 
 *
 * Description   : Function to process agent going down    
 *                 from Applications. 
 *
 * Arguments     : CPND_CB *cb - CPND CB pointer
 *                 MDS_DEST dest - Agent MDS_DEST
 *
 * Return Values : NCSCC_RC_SUCCESS/Error.
 *
 * Notes         : None.
 *****************************************************************************/
void cpnd_proc_cpa_down(CPND_CB *cb, MDS_DEST dest)
{
	CPND_CKPT_CLIENT_NODE *cl_node = NULL;
	CPND_CKPT_NODE *cp_node = NULL;
	SaCkptHandleT prev_ckpt_hdl;
	SaAisErrorT error;
	uns32 rc = NCSCC_RC_SUCCESS;

	/* go through the client tree ,need to check tmr list ,for close stuff */
	cpnd_client_node_getnext(cb, 0, &cl_node);

	while (cl_node) {
		prev_ckpt_hdl = cl_node->ckpt_app_hdl;

		if (memcmp(&dest, &cl_node->agent_mds_dest, sizeof(MDS_DEST)) == 0) {
			while (cl_node->ckpt_list != NULL) {
				cp_node = cl_node->ckpt_list->cnode;

				/* Process the pending write events for this checkpoint if any */
				cpnd_proc_pending_writes(cb, cp_node, dest);

				cpnd_ckpt_client_del(cp_node, cl_node);
				cpnd_client_ckpt_info_del(cl_node, cp_node);

				if (cp_node->ckpt_lcl_ref_cnt)
					cp_node->ckpt_lcl_ref_cnt--;

				cpnd_restart_client_reset(cb, cp_node, cl_node);

				rc = cpnd_send_ckpt_usr_info_to_cpd(cb, cp_node, cp_node->open_flags,
								    CPSV_USR_INFO_CKPT_CLOSE);
				if (rc != NCSCC_RC_SUCCESS) {
					m_LOG_CPND_CL(CPND_MDS_SEND_FAIL, CPND_FC_HDLN, NCSFL_SEV_ERROR, __FILE__,
						      __LINE__);
				}
				m_LOG_CPND_FFLCL(CPND_CLIENT_CKPT_CLOSE_SUCCESS, CPND_FC_API, NCSFL_SEV_INFO,
						 cl_node->ckpt_app_hdl, cp_node->ckpt_id, cp_node->ckpt_lcl_ref_cnt,
						 __FILE__, __LINE__);

				if (m_CPND_IS_COLLOCATED_ATTR_SET(cp_node->create_attrib.creationFlags)) {
					rc = cpnd_ckpt_replica_close(cb, cp_node, &error);
					if (rc != NCSCC_RC_SUCCESS) {

						m_LOG_CPND_FCL(CPND_CKPT_REPLICA_CLOSE_FAILED, CPND_FC_GENERIC, NCSFL_SEV_ERROR,
								cp_node->ckpt_id, __FILE__, __LINE__);
					}
				}
			}

			/* CPND RESTART - FREE THE GLOBAL SHARED MEMORY */
			m_LOG_CPND_FCL(CPND_CKPT_CLIENT_DEL_SUCCESS, CPND_FC_CKPTINFO, NCSFL_SEV_INFO,
				       cl_node->ckpt_app_hdl, __FILE__, __LINE__);
			cpnd_restart_client_node_del(cb, cl_node);
			cpnd_client_node_del(cb, cl_node);
			m_MMGR_FREE_CPND_CKPT_CLIENT_NODE(cl_node);
		}

		cpnd_client_node_getnext(cb, prev_ckpt_hdl, &cl_node);
	}
}

/**************************************************************************
 * Name     :  cpnd_proc_cpa_up
 *
 * Description   : Function to process agent going down
 *                 from Applications.
 *
 * Arguments     : CPND_CB *cb - CPND CB pointer
 *                 MDS_DEST dest - Agent MDS_DEST
 *
 * Return Values : NCSCC_RC_SUCCESS/Error.
 *
 * Notes         : None.
 *****************************************************************************/
void cpnd_proc_cpa_up(CPND_CB *cb, MDS_DEST dest)
{
	CPND_CKPT_CLIENT_NODE *cl_node = NULL;
	SaCkptHandleT prev_ckpt_hdl;

	cpnd_client_node_getnext(cb, 0, &cl_node);

	while (cl_node) {
		prev_ckpt_hdl = cl_node->ckpt_app_hdl;
		if (memcmp(&dest, &cl_node->agent_mds_dest, sizeof(MDS_DEST)) == 0) {
			cl_node->app_status = TRUE;
		}
		cpnd_client_node_getnext(cb, prev_ckpt_hdl, &cl_node);
	}
}

/**************************************************************************
 * Name     :  cpnd_proc_app_status
 *
 * Description   : Function to process agent going down
 *                 from Applications.
 *
 * Arguments     : CPND_CB *cb - CPND CB pointer
 *
 * Return Values : NCSCC_RC_SUCCESS/Error.
 *
 * Notes         : None.
 *****************************************************************************/
void cpnd_proc_app_status(CPND_CB *cb)
{
	CPND_CKPT_CLIENT_NODE *cl_node = NULL;
	SaCkptHandleT prev_ckpt_hdl;
	CPSV_EVT send_evt, *evt = NULL;
	uns32 rc = NCSCC_RC_SUCCESS;

	cpnd_client_node_getnext(cb, 0, &cl_node);

	while (cl_node) {
		prev_ckpt_hdl = cl_node->ckpt_app_hdl;
		memset(&send_evt, 0, sizeof(CPSV_EVT));
		send_evt.type = CPSV_EVT_TYPE_CPA;
		send_evt.info.cpa.type = CPA_EVT_ND2A_CKPT_BCAST_SEND;
		rc = cpnd_mds_msg_send(cb, NCSMDS_SVC_ID_CPA, cl_node->agent_mds_dest, &send_evt);

		if (rc == NCSCC_RC_FAILURE) {
			/* Post the CPA_DOWN event to CPND mailbox, this is in Main thread */
			evt = m_MMGR_ALLOC_CPSV_EVT(NCS_SERVICE_ID_CPND);
			memset(evt, 0, sizeof(CPSV_EVT));
			evt->type = CPSV_EVT_TYPE_CPND;
			evt->info.cpnd.type = CPND_EVT_MDS_INFO;
			evt->info.cpnd.info.mds_info.change = NCSMDS_DOWN;
			evt->info.cpnd.info.mds_info.svc_id = NCSMDS_SVC_ID_CPA;
			evt->info.cpnd.info.mds_info.dest = cl_node->agent_mds_dest;

			rc = m_NCS_IPC_SEND(&cb->cpnd_mbx, (NCSCONTEXT)evt, NCS_IPC_PRIORITY_VERY_HIGH);
		}

		cpnd_client_node_getnext(cb, prev_ckpt_hdl, &cl_node);
	}
}

/****************************************************************************
 * Name          : cpnd_ckpt_update_replica
 *
 * Description   : Function to process 
 *                 from Applications. 
 *
 * Arguments     : CPND_CB *cb - CPND CB pointer
 *
 * Return Values : NCSCC_RC_SUCCESS/Error.
 *
 * Notes         : None.
 *****************************************************************************/
uns32 cpnd_ckpt_update_replica(CPND_CB *cb, CPND_CKPT_NODE *cp_node,
			       CPSV_CKPT_ACCESS *write_data, uns32 type, uns32 *err_type, uns32 *errflag)
{
	uns32 rc = NCSCC_RC_SUCCESS;
	uns32 i = 0;
	CPSV_CKPT_DATA *data;
	CPND_CKPT_SECTION_INFO *sec_info = NULL;

	data = write_data->data;
	for (; i < write_data->num_of_elmts; i++) {
		sec_info = cpnd_ckpt_sec_get_create(cp_node, &data->sec_id);
		if (sec_info == NULL) {
			if (type == CPSV_CKPT_ACCESS_SYNC) {
				sec_info = cpnd_ckpt_sec_add(cp_node, &data->sec_id, data->expirationTime, 0);

				if (sec_info == NULL) {
					m_LOG_CPND_CFCL(CPND_CKPT_SECT_ADD_FAILED, CPND_FC_API, NCSFL_SEV_ERROR,
							data->sec_id.id, cp_node->ckpt_id, __FILE__, __LINE__);
					return NCSCC_RC_FAILURE;

				}
			} else {
				m_LOG_CPND_LCL(CPND_REPLICA_HAS_NO_SECTIONS, CPND_FC_GENERIC, NCSFL_SEV_ERROR, type,
					       __FILE__, __LINE__);
				*errflag = i;
				*err_type = CKPT_UPDATE_REPLICA_NO_SECTION;
				return NCSCC_RC_FAILURE;
			}
		}

		rc = cpnd_ckpt_sec_write(cp_node, sec_info, data->data, data->dataSize,
					 data->dataOffset, write_data->type);
		if (rc == NCSCC_RC_FAILURE) {
			m_LOG_CPND_CFCL(CPND_CKPT_SECT_WRITE_FAILED, CPND_FC_HDLN, NCSFL_SEV_ERROR,
					data->sec_id.id, cp_node->ckpt_id, __FILE__, __LINE__);
			*errflag = i;
			*err_type = CKPT_UPDATE_REPLICA_RES_ERR;
			return rc;
		}

		data = data->next;
	}
	return rc;
}

/****************************************************************************
 * Name          : cpnd_ckpt_read_replica
 *
 * Description   : Function to process 
 *                 from Applications. 
 *
 * Arguments     : CPND_CB *cb - CPND CB pointer
 *
 * Return Values : NCSCC_RC_SUCCESS/Error.
 *
 * Notes         : None.
 *****************************************************************************/
uns32 cpnd_ckpt_read_replica(CPND_CB *cb, CPND_CKPT_NODE *cp_node, CPSV_CKPT_ACCESS *read_data, CPSV_EVT *evt)
{

	uns32 rc = NCSCC_RC_SUCCESS;
	uns32 i = 0, read_size = 0, j = 0;
	CPSV_CKPT_DATA *data;
	CPND_CKPT_SECTION_INFO *sec_info = NULL;
	CPSV_ND2A_READ_DATA *ptr_read_data;

	evt->info.cpa.info.sec_data_rsp.num_of_elmts = read_data->num_of_elmts;
	evt->info.cpa.info.sec_data_rsp.size = read_data->num_of_elmts;

	evt->info.cpa.info.sec_data_rsp.info.read_data =
	    (CPSV_ND2A_READ_DATA *)m_MMGR_ALLOC_CPND_DEFAULT(sizeof(CPSV_ND2A_READ_DATA) * read_data->num_of_elmts);
	if (evt->info.cpa.info.sec_data_rsp.info.read_data == NULL) {
		m_LOG_CPND_CL(CPND_CPSV_ND2A_READ_DATA_ALLOC_FAIL, CPND_FC_MEMFAIL, NCSFL_SEV_ERROR, __FILE__,
			      __LINE__);
		evt->info.cpa.info.sec_data_rsp.num_of_elmts = -1;
		evt->info.cpa.info.sec_data_rsp.error = SA_AIS_ERR_NO_MEMORY;
		evt->info.cpa.info.sec_data_rsp.size = 0;
		return NCSCC_RC_FAILURE;
	}

	memset(evt->info.cpa.info.sec_data_rsp.info.read_data, '\0',
	       sizeof(CPSV_ND2A_READ_DATA) * read_data->num_of_elmts);

	ptr_read_data = evt->info.cpa.info.sec_data_rsp.info.read_data;
	data = read_data->data;

	for (; i < read_data->num_of_elmts; i++, data = data->next) {
		sec_info = cpnd_ckpt_sec_get(cp_node, &data->sec_id);
		if (sec_info == NULL) {
			m_LOG_CPND_CFCL(CPND_CKPT_SECT_GET_FAILED, CPND_FC_API, NCSFL_SEV_ERROR, data->sec_id.id,
					cp_node->ckpt_id, __FILE__, __LINE__);
			ptr_read_data[i].data = NULL;
			ptr_read_data[i].err = 1;
			continue;
		}


      if(data->dataOffset > sec_info->sec_size)
      {
         m_LOG_CPND_CL(CPND_SECTION_BOUNDARY_VIOLATION,CPND_FC_GENERIC,NCSFL_SEV_ERROR,__FILE__,__LINE__);
         evt->info.cpa.info.sec_data_rsp.num_of_elmts=-1;
         evt->info.cpa.info.sec_data_rsp.error_index = i; /** Error index updated **/
         evt->info.cpa.info.sec_data_rsp.error = SA_AIS_ERR_INVALID_PARAM;
         for(j=0;j<i;j++)
         {
            if(ptr_read_data[j].data != NULL)   
            { 
               m_MMGR_FREE_CPND_DEFAULT(ptr_read_data[j].data);
               ptr_read_data[j].data=NULL; 
               ptr_read_data[j].read_size=0; 
            }
          }
          rc = NCSCC_RC_FAILURE;
          break;
       }
         
      /* If dataSize == 0 then it means that dataBuffer is NULL */
       if(data->dataSize == 0)
       {
         read_size = sec_info->sec_size - data->dataOffset;
       }
       else
       {
         if ( (data->dataOffset + data->dataSize) >= sec_info->sec_size )
           read_size=sec_info->sec_size - data->dataOffset;
         else 
           read_size=data->dataSize;
       }
   
       if(!read_size) 
        { 
         if(ptr_read_data[i].data != NULL)
           m_MMGR_FREE_CPND_DEFAULT(ptr_read_data[i].data);
         ptr_read_data[i].data=NULL;
         ptr_read_data[i].read_size=0;
         continue;
        }
  
       ptr_read_data[i].data =m_MMGR_ALLOC_CPND_DEFAULT(read_size);
       if (ptr_read_data[i].data == NULL)
       {
          ptr_read_data[i].err=1;
          continue;
       }
       rc=cpnd_ckpt_sec_read(cp_node,sec_info,ptr_read_data[i].data,read_size,\
                        data->dataOffset);
       if ( rc == NCSCC_RC_FAILURE) 
       {
         ptr_read_data[i].err=1;
         m_MMGR_FREE_CPND_DEFAULT(ptr_read_data[i].data);
         ptr_read_data[i].data=NULL;
         continue;
       }
       ptr_read_data[i].read_size=read_size;
   }
   return rc;
}

/****************************************************************************
 * Name          : cpnd_ckpt_generate_cpsv_ckpt_access_evt
 *
 * Description   : Function to process the   
 *                 from Applications. 
 *
 * Arguments     : CPND_CB *cb - CPND CB pointer
 *                 CPSV_EVT *evt - Received Event structure
 *                 CPSV_SEND_INFO *sinfo - Sender MDS information.
 *
 * Return Values : NCSCC_RC_SUCCESS/Error.
 *
 * Notes         : None.
 *****************************************************************************/
CPSV_CKPT_DATA *cpnd_ckpt_generate_cpsv_ckpt_access_evt(CPND_CKPT_NODE *cp_node)
{

	CPSV_CKPT_DATA *tmp_sec_data = NULL, *sec_data = NULL;
	CPND_CKPT_SECTION_INFO *tmp_sec_info = NULL;
	uns32 i;

	tmp_sec_info = cp_node->replica_info.section_info;

	for (i = 0; i < cp_node->replica_info.n_secs; i++) {
		tmp_sec_data = m_MMGR_ALLOC_CPSV_CKPT_DATA;
		memset(tmp_sec_data, '\0', sizeof(CPSV_CKPT_DATA));

		tmp_sec_data->sec_id = tmp_sec_info->sec_id;
		tmp_sec_data->dataSize = tmp_sec_info->sec_size;

		tmp_sec_data->data = m_MMGR_ALLOC_CPND_DEFAULT(tmp_sec_data->dataSize);

		cpnd_ckpt_sec_read(cp_node, tmp_sec_info, tmp_sec_data->data, tmp_sec_data->dataSize, 0);

		tmp_sec_data->next = sec_data;
		sec_data = tmp_sec_data;

		tmp_sec_info = tmp_sec_info->next;
	}

	return sec_data;
}

/****************************************************************************
 * Name          : cpnd_proc_free_cpsv_ckpt_data 
 *
 * Description   : Function to free temporary ckpt data
 *
 * Arguments     : CPSV_CKPT_DATA *data - Pointer to temporary Ckpt Data.
 *
 * Return Values : NCSCC_RC_SUCCESS/Error.
 *
 * Notes         : None.
 *****************************************************************************/
void cpnd_proc_free_cpsv_ckpt_data(CPSV_CKPT_DATA *data)
{
	CPSV_CKPT_DATA *tmp_data = NULL;

	if (data == NULL) {
		return;
	}

	while (data != NULL) {

		tmp_data = data;
		data = data->next;
		if (tmp_data->data)
			m_MMGR_FREE_CPND_DEFAULT(tmp_data->data);
		m_MMGR_FREE_CPSV_CKPT_DATA(tmp_data);
	}
}

/****************************************************************************
 * Name          : cpnd_allrepl_write_evt_node_free 
 *
 * Description   : Function to free ALLREPL write evt memory
 *
 * Arguments     : Pointer to ALLREPL event node.
 *
 * Return Values : NCSCC_RC_SUCCESS/Error.
 *
 * Notes         : None.
 *****************************************************************************/
uns32 cpnd_allrepl_write_evt_node_free(CPSV_CPND_ALL_REPL_EVT_NODE *evt_node)
{
	CPSV_CPND_UPDATE_DEST *free_tmp = evt_node->cpnd_update_dest_list, *tmp = NULL;

	while (free_tmp) {
		tmp = free_tmp->next;
		m_MMGR_FREE_CPND_UPDATE_DEST_INFO(free_tmp);
		free_tmp = tmp;
	}

	m_MMGR_FREE_CPND_ALL_REPL_EVT_NODE(evt_node);

	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
 * Name          : 
 *
 * Description   : Function to process the   
 *                 from Applications. 
 *
 * Arguments     : CPND_CB *cb - CPND CB pointer
 *                 CPSV_EVT *evt - Received Event structure
 *                 CPSV_SEND_INFO *sinfo - Sender MDS information.
 *
 * Return Values : NCSCC_RC_SUCCESS/Error.
 *
 * Notes         : None.
 *****************************************************************************/
void cpnd_proc_gen_mapping(CPND_CKPT_NODE *cp_node, CPSV_CKPT_ACCESS *ckpt_read, CPSV_EVT *evt)
{

	CPND_CKPT_SECTION_INFO *sec_info = NULL;
	CPSV_CKPT_DATA *sec_data = NULL;
	int iter;
	uns32 read_size = 0;

	evt->type = CPSV_EVT_TYPE_CPA;
	evt->info.cpa.type = CPA_EVT_ND2A_CKPT_DATA_RSP;
	evt->info.cpa.info.sec_data_rsp.type = CPSV_DATA_ACCESS_LCL_READ_RSP;
	evt->info.cpa.info.sec_data_rsp.num_of_elmts = ckpt_read->num_of_elmts;

	evt->info.cpa.info.sec_data_rsp.info.read_mapping = (CPSV_ND2A_READ_MAP *)
	    m_MMGR_ALLOC_CPND_DEFAULT(sizeof(CPSV_ND2A_READ_MAP) * ckpt_read->num_of_elmts);
	if (evt->info.cpa.info.sec_data_rsp.info.read_mapping == NULL) {
		m_LOG_CPND_CL(CPND_CPSV_ND2A_READ_MAP_ALLOC_FAIL, CPND_FC_MEMFAIL, NCSFL_SEV_ERROR, __FILE__, __LINE__);
		evt->info.cpa.info.sec_data_rsp.num_of_elmts = -1;
		evt->info.cpa.info.sec_data_rsp.error = SA_AIS_ERR_NO_MEMORY;
		return;
	}

	memset(evt->info.cpa.info.sec_data_rsp.info.read_mapping, '\0',
	       sizeof(CPSV_ND2A_READ_MAP) * ckpt_read->num_of_elmts);

	sec_data = ckpt_read->data;
	for (iter = 0; iter < ckpt_read->num_of_elmts; iter++, sec_data = sec_data->next) {
		sec_info = cpnd_ckpt_sec_get(cp_node, &sec_data->sec_id);
		if (sec_info == NULL) {
			m_LOG_CPND_CFCL(CPND_CKPT_SECT_GET_FAILED, CPND_FC_API, NCSFL_SEV_ERROR, sec_data->sec_id.id,
					cp_node->ckpt_id, __FILE__, __LINE__);
			evt->info.cpa.info.sec_data_rsp.info.read_mapping[iter].offset_index = -1;
			continue;
		}

		if (((sec_data->dataOffset + sec_data->dataSize) > (cp_node->create_attrib.maxSectionSize))
		    || (sec_data->dataOffset > sec_info->sec_size)) {
			m_LOG_CPND_CL(CPND_SECTION_BOUNDARY_VIOLATION, CPND_FC_GENERIC, NCSFL_SEV_ERROR, __FILE__,
				      __LINE__);
			evt->info.cpa.info.sec_data_rsp.num_of_elmts = -1;
			evt->info.cpa.info.sec_data_rsp.error = SA_AIS_ERR_INVALID_PARAM;
			return;
		}

		/* If dataSize == 0 then that means dataBuffer is NULL */
		if (sec_data->dataSize == 0) {
			read_size = sec_info->sec_size - sec_data->dataOffset;
		} else {
			if ((sec_data->dataOffset + sec_data->dataSize) >= sec_info->sec_size)
				read_size = sec_info->sec_size - sec_data->dataOffset;
			else
				read_size = sec_data->dataSize;
		}

		evt->info.cpa.info.sec_data_rsp.info.read_mapping[iter].offset_index = sec_info->lcl_sec_id;
		evt->info.cpa.info.sec_data_rsp.info.read_mapping[iter].read_size = read_size;
	}
}

/****************************************************************************
 * Name          : 
 *
 * Description   : Function to process the   
 *                 from Applications. 
 *
 * Arguments     : CPND_CB *cb - CPND CB pointer
 *                 CPSV_EVT *evt - Received Event structure
 *                 CPSV_SEND_INFO *sinfo - Sender MDS information.
 *
 * Return Values : NCSCC_RC_SUCCESS/Error.
 *
 * Notes         : None.
 *****************************************************************************/
uns32
cpnd_proc_update_remote(CPND_CB *cb, CPND_CKPT_NODE *cp_node, CPND_EVT *in_evt,
			CPSV_EVT *out_evt, CPSV_SEND_INFO *sinfo)
{
	CPSV_EVT send_evt;
	CPSV_CPND_ALL_REPL_EVT_NODE *all_repl_evt = NULL;
	CPSV_CPND_UPDATE_DEST *new = NULL, *head = NULL;
	uns32 rc = NCSCC_RC_SUCCESS;
	uns32 timeout = 0;
	SaSizeT datasize = 0;

	memset(&send_evt, '\0', sizeof(CPSV_EVT));
	if (m_CPND_IS_ALL_REPLICA_ATTR_SET(cp_node->create_attrib.creationFlags) == TRUE) {
		if (cp_node->cpnd_dest_list != NULL) {
			CPSV_CPND_DEST_INFO *tmp = NULL;

			tmp = cp_node->cpnd_dest_list;
			send_evt.type = CPSV_EVT_TYPE_CPND;
			send_evt.info.cpnd.type = CPSV_EVT_ND2ND_CKPT_SECT_ACTIVE_DATA_ACCESS_REQ;

			{
				send_evt.info.cpnd.info.ckpt_nd2nd_data = in_evt->info.ckpt_write;
				datasize = in_evt->info.ckpt_write.num_of_elmts * cp_node->create_attrib.maxSectionSize;
				timeout = CPND_WAIT_TIME(datasize);
			}
			/*Flag set to distinguish ALL_REPL case on response side */
			send_evt.info.cpnd.info.ckpt_nd2nd_data.all_repl_evt_flag = TRUE;
			send_evt.info.cpnd.info.ckpt_nd2nd_data.agent_mdest = sinfo->dest;
			/*Allocate memory to store the ALL REPL event node */
			all_repl_evt = m_MMGR_ALLOC_CPND_ALL_REPL_EVT_NODE;
			if (all_repl_evt) {

				memset(all_repl_evt, '\0', sizeof(CPSV_CPND_ALL_REPL_EVT_NODE));

				/*Populate the Event node */
				all_repl_evt->ckpt_id = cp_node->ckpt_id;
				all_repl_evt->sinfo = *sinfo;

				/*Copy the entire dest_list info of ckpt node to  all_repl_evt dest_list */
				while (tmp != NULL) {
					new = m_MMGR_ALLOC_CPND_UPDATE_DEST_INFO;
					if (!new) {
						rc = NCSCC_RC_FAILURE;
						goto mem_fail;
					}
					memset(new, 0, sizeof(CPSV_CPND_UPDATE_DEST));
					new->dest = tmp->dest;
					if (head == NULL) {
						head = new;
					} else {
						new->next = head;
						head = new;
					}
					tmp = tmp->next;
				}
				all_repl_evt->cpnd_update_dest_list = head;

				if (cpnd_evt_node_add(cb, all_repl_evt) == NCSCC_RC_FAILURE) {
					/*log this error */
					/*free the memory allocated */
				}

				/*Start the timer before send the async req to remote node */
				all_repl_evt->write_rsp_tmr.type = CPND_ALL_REPL_RSP_EXPI;
				all_repl_evt->write_rsp_tmr.uarg = cb->cpnd_cb_hdl_id;
				all_repl_evt->write_rsp_tmr.ckpt_id = cp_node->ckpt_id;
				all_repl_evt->write_rsp_tmr.agent_dest = sinfo->dest;
				all_repl_evt->write_rsp_tmr.write_type = in_evt->info.ckpt_write.type;
				rc = cpnd_tmr_start(&all_repl_evt->write_rsp_tmr,
						    m_CPSV_CONVERT_SATIME_TEN_MILLI_SEC(timeout));

				while (head != NULL) {
					rc = cpnd_mds_msg_send(cb, NCSMDS_SVC_ID_CPND, head->dest, &send_evt);
					if (rc != NCSCC_RC_SUCCESS) {
						m_LOG_CPND_FFFLCL(CPND_ACTIVE_TO_REMOTE_MDS_SEND_FAIL, CPND_FC_MDSFAIL,
								  NCSFL_SEV_ERROR, cb->cpnd_mdest_id, head->dest,
								  cp_node->ckpt_id, rc, __FILE__, __LINE__);
					} else {
						all_repl_evt->write_rsp_cnt++;
					}

					head = head->next;
				}
			} else {
				/*Log memory allocation failure */
			}

		}
	}

	else if ((m_CPND_IS_ACTIVE_REPLICA_ATTR_SET(cp_node->create_attrib.creationFlags) == TRUE) ||
		 (m_CPND_IS_ACTIVE_REPLICA_WEAK_ATTR_SET(cp_node->create_attrib.creationFlags) == TRUE)) {
		/* send rsp to agent */
		/* send to all other cpnd's using mds send */
		if (cp_node->cpnd_dest_list != NULL) {
			CPSV_CPND_DEST_INFO *tmp = NULL;
			tmp = cp_node->cpnd_dest_list;
			send_evt.type = CPSV_EVT_TYPE_CPND;
			send_evt.info.cpnd.type = CPSV_EVT_ND2ND_CKPT_SECT_ACTIVE_DATA_ACCESS_REQ;
			send_evt.info.cpnd.info.ckpt_nd2nd_data = in_evt->info.ckpt_write;
			while (tmp != NULL) {
				if (sinfo && m_CPND_IS_LOCAL_NODE(tmp, &sinfo->dest) == 0) {
					tmp = tmp->next;
					continue;
				}
				rc = cpnd_mds_msg_send(cb, NCSMDS_SVC_ID_CPND, tmp->dest, &send_evt);
				if (rc == NCSCC_RC_FAILURE) {
					if (rc == NCSCC_RC_REQ_TIMOUT) {
						m_LOG_CPND_FFFLCL(CPND_ACTIVE_TO_REMOTE_MDS_SEND_FAIL, CPND_FC_MDSFAIL,
								  NCSFL_SEV_ERROR, cb->cpnd_mdest_id, tmp->dest,
								  cp_node->ckpt_id, rc, __FILE__, __LINE__);
					}

					/*  mds_failure=TRUE;
					   goto mds_failure; */
				}
				tmp = tmp->next;
			}
		}
	}

	/* TBD IF received from Agent, fill the response to Agent */

	/* if ( (tmp_out_evt == NULL) || tmp_out_evt->type == CPSV_EVT_ND2ND_CKPT_SECT_DATA_ACCESS_REQ) */
	{
		out_evt->type = CPSV_EVT_TYPE_CPA;
		out_evt->info.cpa.type = CPA_EVT_ND2A_CKPT_DATA_RSP;

		switch (in_evt->info.ckpt_write.type) {
		case CPSV_CKPT_ACCESS_WRITE:
			out_evt->info.cpa.info.sec_data_rsp.type = CPSV_DATA_ACCESS_WRITE_RSP;
			out_evt->info.cpa.info.sec_data_rsp.error = SA_AIS_OK;
			break;

		case CPSV_CKPT_ACCESS_OVWRITE:
			out_evt->info.cpa.info.sec_data_rsp.type = CPSV_DATA_ACCESS_OVWRITE_RSP;
			out_evt->info.cpa.info.sec_data_rsp.info.ovwrite_error.error = SA_AIS_OK;
			break;
		}
	}

 mem_fail:
	return rc;
}

/****************************************************************************
 * Name          : cpnd_proc_rt_expiry
 * Description   : Function to process ckpt retentionDuration timer
 * Arguments     : CPND_CB *cb - CPND CB pointer
 *                 SaCkptCheckpointHandleT -- Ckpt handle
 * Return Values : NCSCC_RC_SUCCESS/Error.
 * Notes         : None.
 *****************************************************************************/
uns32 cpnd_proc_rt_expiry(CPND_CB *cb, SaCkptCheckpointHandleT ckpt_id)
{
	uns32 rc = NCSCC_RC_SUCCESS;
	CPND_CKPT_NODE *cp_node = NULL;
	SaAisErrorT error;

	cpnd_ckpt_node_get(cb, ckpt_id, &cp_node);

	if (cp_node == NULL) {
		m_LOG_CPND_FCL(CPND_CKPT_NODE_GET_FAILED, CPND_FC_API, NCSFL_SEV_ERROR, ckpt_id, __FILE__, __LINE__);
		return NCSCC_RC_FAILURE;
	}

	if (!m_CPND_IS_COLLOCATED_ATTR_SET(cp_node->create_attrib.creationFlags)) {

		if (cpnd_is_noncollocated_replica_present_on_payload(cb, cp_node))
			return NCSCC_RC_SUCCESS;
	}
	rc = cpnd_ckpt_replica_destroy(cb, cp_node, &error);
	if (rc == NCSCC_RC_FAILURE) {
		m_LOG_CPND_FCL(CPND_CKPT_REPLICA_DESTROY_FAILED, CPND_FC_GENERIC, NCSFL_SEV_ERROR, cp_node->ckpt_id,
			       __FILE__, __LINE__);
		return NCSCC_RC_FAILURE;
	}

	cpnd_restart_shm_ckpt_free(cb, cp_node);
	cpnd_ckpt_node_destroy(cb, cp_node);

	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
 * Name          : cpnd_proc_non_colloc_rt_expiry
 * Description   : Function to process ckpt retentionDuration timer for non-colloc
 * Arguments     : CPND_CB *cb - CPND CB pointer
 *                 SaCkptCheckpointHandleT -- Ckpt handle
 * Return Values : NCSCC_RC_SUCCESS/Error.
 * Notes         : None.
 *****************************************************************************/
uns32 cpnd_proc_non_colloc_rt_expiry(CPND_CB *cb, SaCkptCheckpointHandleT ckpt_id)
{
	uns32 rc = NCSCC_RC_SUCCESS;
	CPND_CKPT_NODE *cp_node = NULL;
	SaAisErrorT error;

	cpnd_ckpt_node_get(cb, ckpt_id, &cp_node);

	if (cp_node == NULL) {
		m_LOG_CPND_FCL(CPND_CKPT_NODE_GET_FAILED, CPND_FC_API, NCSFL_SEV_ERROR, ckpt_id, __FILE__, __LINE__);
		return NCSCC_RC_FAILURE;
	}
	if (!m_CPND_IS_COLLOCATED_ATTR_SET(cp_node->create_attrib.creationFlags)) {

		if (cpnd_is_noncollocated_replica_present_on_payload(cb, cp_node))
			return NCSCC_RC_SUCCESS;
	}

	rc = cpnd_ckpt_replica_destroy(cb, cp_node, &error);
	if (rc == NCSCC_RC_FAILURE) {
		m_LOG_CPND_FCL(CPND_CKPT_REPLICA_DESTROY_FAILED, CPND_FC_GENERIC, NCSFL_SEV_ERROR, cp_node->ckpt_id,
			       __FILE__, __LINE__);
		return NCSCC_RC_FAILURE;
	}

	cpnd_restart_shm_ckpt_free(cb, cp_node);
	cpnd_ckpt_node_destroy(cb, cp_node);

	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
 * Name          : cpnd_proc_sec_expiry
 *
 * Description   : Function to process section expriry timer
 *
 * Arguments     : CPND_CB *cb - CPND CB pointer
 *                 CPND_TMR_INFO - Sec Tmr block 
 *
 * Return Values : NCSCC_RC_SUCCESS/Error.
 *
 * Notes         : None.
 *****************************************************************************/
uns32 cpnd_proc_sec_expiry(CPND_CB *cb, CPND_TMR_INFO *tmr_info)
{

	CPND_CKPT_NODE *cp_node = NULL;
	CPND_CKPT_SECTION_INFO *pSec_info = NULL;
	CPSV_EVT send_evt, *out_evt = NULL;
	uns32 rc = NCSCC_RC_SUCCESS;

	memset(&send_evt, '\0', sizeof(CPSV_EVT));

	cpnd_ckpt_node_get(cb, tmr_info->ckpt_id, &cp_node);
	if (cp_node == NULL) {
		m_LOG_CPND_FCL(CPND_CKPT_NODE_GET_FAILED, CPND_FC_API, NCSFL_SEV_ERROR,
			       tmr_info->ckpt_id, __FILE__, __LINE__);
		return NCSCC_RC_FAILURE;
	}

	pSec_info = cpnd_get_sect_with_id(cp_node, tmr_info->lcl_sec_id);
	if (pSec_info == NULL) {
		m_LOG_CPND_LCL(CPND_CKPT_SECT_FIND_FAILED, CPND_FC_API, NCSFL_SEV_ERROR,
			       tmr_info->lcl_sec_id, __FILE__, __LINE__);
		return NCSCC_RC_FAILURE;
	}

	cpnd_ckpt_sec_del(cp_node, &pSec_info->sec_id);
	cp_node->replica_info.shm_sec_mapping[pSec_info->lcl_sec_id] = 1;

	/* send out destory to all cpnd's maintaining this ckpt */
	if (cp_node->cpnd_dest_list != NULL) {

		CPSV_CPND_DEST_INFO *tmp = NULL;
		tmp = cp_node->cpnd_dest_list;
		while (tmp != NULL) {
			send_evt.type = CPSV_EVT_TYPE_CPND;
			send_evt.info.cpnd.type = CPSV_EVT_ND2ND_CKPT_SECT_DELETE_REQ;
			send_evt.info.cpnd.info.sec_delete_req.ckpt_id = tmr_info->ckpt_id;
			send_evt.info.cpnd.info.sec_delete_req.sec_id = pSec_info->sec_id;

			rc = cpnd_mds_msg_sync_send(cb, NCSMDS_SVC_ID_CPND, tmp->dest,
						    &send_evt, &out_evt, CPSV_WAIT_TIME);
			if (rc != NCSCC_RC_SUCCESS) {
				if (rc == NCSCC_RC_REQ_TIMOUT) {
					m_LOG_CPND_FFFLCL(CPND_ACTIVE_TO_REMOTE_MDS_SEND_FAIL, CPND_FC_MDSFAIL,
							  NCSFL_SEV_ERROR, cb->cpnd_mdest_id, tmp->dest,
							  cp_node->ckpt_id, rc, __FILE__, __LINE__);
				}
			}

			if (out_evt != NULL)
				cpnd_evt_destroy(out_evt);

			tmp = tmp->next;
			out_evt = NULL;
		}
	}
	m_CPND_FREE_CKPT_SECTION(pSec_info);
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
 * Name          : cpnd_all_repl_rsp_expiry 
 *
 * Description   : Function to process ALLREPL write event expiry
 *
 * Arguments     : CPND_CB *cb - CPND CB pointer
 *                 CPND_TMR_INFO - Sec Tmr block
 *
 * Return Values : NCSCC_RC_SUCCESS/Error.
 *
 * Notes         : None.
 *****************************************************************************/
uns32 cpnd_open_active_sync_expiry(CPND_CB *cb, CPND_TMR_INFO *tmr_info)
{
	uns32 rc = NCSCC_RC_SUCCESS;
	CPSV_EVT des_evt, *out_evt = NULL;
	CPSV_EVT send_evt;
	memset(&des_evt, '\0', sizeof(CPSV_EVT));
	memset(&send_evt, '\0', sizeof(CPSV_EVT));
	des_evt.type = CPSV_EVT_TYPE_CPD;
	des_evt.info.cpd.type = CPD_EVT_ND2D_CKPT_DESTROY;
	des_evt.info.cpd.info.ckpt_destroy.ckpt_id = tmr_info->ckpt_id;
	rc = cpnd_mds_msg_sync_send(cb, NCSMDS_SVC_ID_CPD, cb->cpd_mdest_id, &des_evt, &out_evt, CPSV_WAIT_TIME);
	if (out_evt && out_evt->info.cpnd.info.destroy_ack.error != SA_AIS_OK) {
		m_LOG_CPND_LCL(CPND_CPD_NEW_ACTIVE_DESTROY_FAILED, CPND_FC_EVT, NCSFL_SEV_ERROR,
			       out_evt->info.cpnd.info.destroy_ack.error, __FILE__, __LINE__);
	}
	if (out_evt)
		cpnd_evt_destroy(out_evt);
	send_evt.info.cpa.info.openRsp.error = SA_AIS_ERR_TIMEOUT;
	send_evt.type = CPSV_EVT_TYPE_CPA;
	send_evt.info.cpa.type = CPA_EVT_ND2A_CKPT_OPEN_RSP;
	send_evt.info.cpa.info.openRsp.lcl_ckpt_hdl = tmr_info->lcl_ckpt_hdl;
	if (tmr_info->sinfo.stype == MDS_SENDTYPE_SNDRSP) {
		rc = cpnd_mds_send_rsp(cb, &tmr_info->sinfo, &send_evt);
	} else {
		send_evt.info.cpa.info.openRsp.invocation = tmr_info->invocation;
		rc = cpnd_mds_msg_send(cb, tmr_info->sinfo.to_svc, tmr_info->sinfo.dest, &send_evt);
	}
	return NCSCC_RC_SUCCESS;
}

uns32 cpnd_all_repl_rsp_expiry(CPND_CB *cb, CPND_TMR_INFO *tmr_info)
{
	uns32 rc = NCSCC_RC_SUCCESS;
	CPND_CKPT_NODE *cp_node = NULL;
	CPSV_CPND_ALL_REPL_EVT_NODE *evt_node = NULL;
	CPSV_EVT rsp_evt;

	cpnd_ckpt_node_get(cb, tmr_info->ckpt_id, &cp_node);
	cpnd_evt_node_get(cb, tmr_info->ckpt_id, &evt_node);

	memset(&rsp_evt, 0, sizeof(CPSV_EVT));

	rsp_evt.type = CPSV_EVT_TYPE_CPA;
	rsp_evt.info.cpa.type = CPA_EVT_ND2A_CKPT_DATA_RSP;

	if (cp_node && evt_node) {
		if (cp_node->ckpt_id == evt_node->ckpt_id) {
			switch (tmr_info->write_type) {
			case CPSV_CKPT_ACCESS_WRITE:
				rsp_evt.info.cpa.info.sec_data_rsp.type = CPSV_DATA_ACCESS_WRITE_RSP;
				rsp_evt.info.cpa.info.sec_data_rsp.error = SA_AIS_ERR_TIMEOUT;
				break;

			case CPSV_CKPT_ACCESS_OVWRITE:
				rsp_evt.info.cpa.info.sec_data_rsp.type = CPSV_DATA_ACCESS_OVWRITE_RSP;
				rsp_evt.info.cpa.info.sec_data_rsp.info.ovwrite_error.error = SA_AIS_ERR_TIMEOUT;
				break;
			}

			rc = cpnd_mds_send_rsp(cb, &evt_node->sinfo, &rsp_evt);

		}
	}

	if (evt_node) {
		/*Remove the all repl event node */
		cpnd_evt_node_del(cb, evt_node);

		/*Free the memory */
		cpnd_allrepl_write_evt_node_free(evt_node);
	}

	return rc;
}

uns32 cpnd_proc_fill_sec_desc(CPND_CKPT_SECTION_INFO *pTmpSecPtr, SaCkptSectionDescriptorT *sec_des)
{

	sec_des->sectionId = pTmpSecPtr->sec_id;
	sec_des->expirationTime = pTmpSecPtr->exp_tmr;
	sec_des->sectionSize = pTmpSecPtr->sec_size;
	sec_des->sectionState = pTmpSecPtr->sec_state;
	sec_des->lastUpdate = pTmpSecPtr->lastUpdate;	/* need to 
							   update the section */
	return NCSCC_RC_SUCCESS;

}

uns32
cpnd_proc_getnext_section(CPND_CKPT_NODE *cp_node,
			  CPSV_A2ND_SECT_ITER_GETNEXT *get_next, SaCkptSectionDescriptorT *sec_des, uns32 *n_secs_trav)
{

	CPND_CKPT_SECTION_INFO *pSecPtr = NULL, *pTmpSecPtr = NULL;
	pSecPtr = cp_node->replica_info.section_info;

	if (cp_node->replica_info.n_secs == 0 || (cp_node->replica_info.n_secs <= get_next->n_secs_trav)) {
		m_LOG_CPND_LCL(CPND_REPLICA_HAS_NO_SECTIONS, CPND_FC_GENERIC, NCSFL_SEV_ERROR,
			       cp_node->replica_info.n_secs, __FILE__, __LINE__);
		return NCSCC_RC_FAILURE;
	}

	if (get_next->n_secs_trav < cp_node->replica_info.n_secs) {
		if (get_next->section_id.idLen == 0 && cp_node->create_attrib.maxSections == 1) {
			if (get_next->filter == SA_CKPT_SECTIONS_ANY ||
			    get_next->filter == SA_CKPT_SECTIONS_FOREVER ||
			    get_next->filter == SA_CKPT_SECTIONS_GEQ_EXPIRATION_TIME) {
				cpnd_proc_fill_sec_desc(pSecPtr, sec_des);
				*n_secs_trav = get_next->n_secs_trav + 1;
				return NCSCC_RC_SUCCESS;
			} else	/* for default section rem options results in no section *
				 * need to think about corruption */
				return NCSCC_RC_FAILURE;
		}

		/* search the existing section id */
		*n_secs_trav = get_next->n_secs_trav;
		while (pSecPtr != NULL && *n_secs_trav != 0) {
			if ((pSecPtr->sec_id.idLen == get_next->section_id.idLen) &&
			    (memcmp(pSecPtr->sec_id.id, get_next->section_id.id, get_next->section_id.idLen) == 0)) {
				break;
			}
			pSecPtr = pSecPtr->next;
		}
		/* if next is NULL then return no more sections */
		if (pSecPtr == NULL) {
			m_LOG_CPND_CL(CPND_REPLICA_HAS_NO_SECTIONS, CPND_FC_GENERIC, NCSFL_SEV_ERROR,
				      __FILE__, __LINE__);
			return NCSCC_RC_FAILURE;
		}

		/* get section descriptor with given filter */
		if (*n_secs_trav == 0)
			pTmpSecPtr = pSecPtr;
		else
			pTmpSecPtr = pSecPtr->next;

		switch (get_next->filter) {
		case SA_CKPT_SECTIONS_ANY:
			(*n_secs_trav)++;
			break;

		case SA_CKPT_SECTIONS_CORRUPTED:
			return NCSCC_RC_FAILURE;
			break;

		case SA_CKPT_SECTIONS_FOREVER:
			while (pTmpSecPtr != NULL) {
				if (pTmpSecPtr->exp_tmr == SA_TIME_END) {
					(*n_secs_trav)++;
					break;
				} else
					pTmpSecPtr = pTmpSecPtr->next;
			}
			if (pTmpSecPtr == NULL) {
				m_LOG_CPND_CL(CPND_REPLICA_HAS_NO_SECTIONS, CPND_FC_GENERIC, NCSFL_SEV_ERROR,
					      __FILE__, __LINE__);
				return NCSCC_RC_FAILURE;
			}
			break;

		case SA_CKPT_SECTIONS_LEQ_EXPIRATION_TIME:
			while (pTmpSecPtr != NULL) {
				if (pTmpSecPtr->exp_tmr <= get_next->exp_tmr) {
					(*n_secs_trav)++;
					break;
				} else
					pTmpSecPtr = pTmpSecPtr->next;
			}
			if (pTmpSecPtr == NULL) {
				m_LOG_CPND_CL(CPND_REPLICA_HAS_NO_SECTIONS, CPND_FC_GENERIC, NCSFL_SEV_ERROR,
					      __FILE__, __LINE__);
				return NCSCC_RC_FAILURE;
			}
			break;

		case SA_CKPT_SECTIONS_GEQ_EXPIRATION_TIME:
			while (pTmpSecPtr != NULL) {
				if (pTmpSecPtr->exp_tmr >= get_next->exp_tmr) {
					(*n_secs_trav)++;
					break;
				} else
					pTmpSecPtr = pTmpSecPtr->next;
			}
			if (pTmpSecPtr == NULL) {
				m_LOG_CPND_CL(CPND_REPLICA_HAS_NO_SECTIONS, CPND_FC_GENERIC, NCSFL_SEV_ERROR,
					      __FILE__, __LINE__);
				return NCSCC_RC_FAILURE;
			}
			break;

		default:
			return NCSCC_RC_FAILURE;
		}

		cpnd_proc_fill_sec_desc(pTmpSecPtr, sec_des);
		return NCSCC_RC_SUCCESS;
	}
	return NCSCC_RC_FAILURE;
}

/***************************************************************************
 * Name          : cpnd_ckpt_hdr_update
 *
 * Description   : To update the checkpoint header information
 *
 * Arguments     : CPND_CKPT_NODE - ckpt node
 *
 * Return Values : Success / Error
****************************************************************************/

uns32 cpnd_ckpt_hdr_update(CPND_CKPT_NODE *cp_node)
{
	CPSV_CKPT_HDR ckpt_hdr;
	uns32 rc = NCSCC_RC_SUCCESS;
	NCS_OS_POSIX_SHM_REQ_INFO write_req;

	memset(&write_req, '\0', sizeof(write_req));
	memset(&ckpt_hdr, '\0', sizeof(CPSV_CKPT_HDR));
	ckpt_hdr.ckpt_id = cp_node->ckpt_id;
	ckpt_hdr.ckpt_name = cp_node->ckpt_name;
	ckpt_hdr.create_attrib = cp_node->create_attrib;
	ckpt_hdr.open_flags = cp_node->open_flags;
	ckpt_hdr.is_unlink = cp_node->is_unlink;
	ckpt_hdr.is_close = cp_node->is_close;
	ckpt_hdr.n_secs = cp_node->replica_info.n_secs;
	ckpt_hdr.mem_used = cp_node->replica_info.mem_used;
	ckpt_hdr.cpnd_rep_create = cp_node->cpnd_rep_create;
	ckpt_hdr.is_active_exist = cp_node->is_active_exist;
	ckpt_hdr.active_mds_dest = cp_node->active_mds_dest;
	ckpt_hdr.cpnd_lcl_wr = cp_node->cur_state;
	ckpt_hdr.cpnd_oth_state = cp_node->oth_state;

	write_req.type = NCS_OS_POSIX_SHM_REQ_WRITE;
	write_req.info.write.i_addr = cp_node->replica_info.open.info.open.o_addr;
	write_req.info.write.i_from_buff = (CPSV_CKPT_HDR *)&ckpt_hdr;
	write_req.info.write.i_offset = 0;
	write_req.info.write.i_write_size = sizeof(CPSV_CKPT_HDR);
	rc = ncs_os_posix_shm(&write_req);

	return rc;

}

/**********************************************************************************
 * Name          :  cpnd_sec_hdr_update
 
 * Description   : To update the section header information
  
 * Arguments    : CPND_CKPT_SECTION_INFO - section info , CPND_CKPT_NODE - ckpt node
 
 * Return Values : Success / Error
***********************************************************************************/

uns32 cpnd_sec_hdr_update(CPND_CKPT_SECTION_INFO *sec_info, CPND_CKPT_NODE *cp_node)
{
	CPSV_SECT_HDR sec_hdr;
	uns32 rc = NCSCC_RC_SUCCESS;
	NCS_OS_POSIX_SHM_REQ_INFO write_req;
	memset(&write_req, '\0', sizeof(write_req));
	memset(&sec_hdr, '\0', sizeof(CPSV_SECT_HDR));
	sec_hdr.lcl_sec_id = sec_info->lcl_sec_id;
	sec_hdr.idLen = sec_info->sec_id.idLen;
	memcpy(sec_hdr.id, sec_info->sec_id.id, sec_info->sec_id.idLen);
	sec_hdr.sec_state = sec_info->sec_state;
	sec_hdr.sec_size = sec_info->sec_size;
	sec_hdr.exp_tmr = sec_info->exp_tmr;
	sec_hdr.lastUpdate = sec_info->lastUpdate;

	write_req.type = NCS_OS_POSIX_SHM_REQ_WRITE;
	write_req.info.write.i_addr =
	    (void *)((char *)cp_node->replica_info.open.info.open.o_addr + sizeof(CPSV_CKPT_HDR));
	write_req.info.write.i_from_buff = (CPSV_SECT_HDR *)&sec_hdr;
	write_req.info.write.i_offset =
	    sec_info->lcl_sec_id * (sizeof(CPSV_SECT_HDR) + cp_node->create_attrib.maxSectionSize);
	write_req.info.write.i_write_size = sizeof(CPSV_SECT_HDR);
	rc = ncs_os_posix_shm(&write_req);

	return rc;
}

/****************************************************************************
 * Name          : cpnd_cb_dump
 *
 * Description   : This is the function dumps the contents of the control block. *
 * Arguments     : cpnd_cb     -  Pointer to the control block
 *
 * Return Values : TRUE/FALSE
 *
 * Notes         : None.
 *****************************************************************************/
void cpnd_cb_dump(void)
{
	CPND_CB *cb = NULL;

	uns32 cb_hdl = m_CPND_GET_CB_HDL;

	/* Get the CB from the handle */
	cb = ncshm_take_hdl(NCS_SERVICE_ID_CPND, cb_hdl);

	if (!cb) {
		m_LOG_CPND_CL(CPND_CB_HDL_TAKE_FAILED, CPND_FC_HDLN, NCSFL_SEV_ERROR, __FILE__, __LINE__);
		return;
	}

	CPND_CKPT_NODE *ckpt_node = NULL;
	CPND_CKPT_CLIENT_NODE *cl_node = NULL;

	TRACE("************ CPND CB Details *************** ");

	TRACE("**** Global Cb Details ***************");
	TRACE("CPND MDS dest - %d", m_NCS_NODE_ID_FROM_MDS_DEST(cb->cpnd_mdest_id));
	if (cb->is_cpd_up) {
		TRACE("CPD is UP  ");
		TRACE("CPD MDS dest - %d", m_NCS_NODE_ID_FROM_MDS_DEST(cb->cpd_mdest_id));
	} else
		TRACE("CPD is DOWN ");

	TRACE("*************************************************");
	TRACE("Number of Shared memory segments %d\n", cb->num_rep);

	TRACE("***** Start of Ckpt Details ***************");
	cpnd_ckpt_node_getnext(cb, 0, &ckpt_node);
	while (ckpt_node != NULL) {
		SaCkptCheckpointHandleT prev_ckpt_id;
		CPND_CKPT_CLLIST_NODE *ckpt_client_list = NULL;

		cpnd_dump_ckpt_info(ckpt_node);
		cpnd_dump_ckpt_attri(ckpt_node);
		cpnd_dump_replica_info(&ckpt_node->replica_info);

		ckpt_client_list = ckpt_node->clist;
		while (ckpt_client_list != NULL) {
			cpnd_dump_client_info(ckpt_client_list->cnode);
			ckpt_client_list = ckpt_client_list->next;
		}

		prev_ckpt_id = ckpt_node->ckpt_id;
		cpnd_ckpt_node_getnext(cb, prev_ckpt_id, &ckpt_node);
	}
	TRACE("***** End of Ckpt Details ***************");

	TRACE("***** Start of Client Details ***************");
	cpnd_client_node_getnext(cb, 0, &cl_node);
	while (cl_node != NULL) {
		SaCkptHandleT prev_cl_hdl;
		CPND_CKPT_CKPT_LIST_NODE *ckpt_list = NULL;

		cpnd_dump_client_info(cl_node);
		ckpt_list = cl_node->ckpt_list;
		while (ckpt_list != NULL) {
			cpnd_dump_ckpt_info(ckpt_list->cnode);
			ckpt_list = ckpt_list->next;
		}
		prev_cl_hdl = cl_node->ckpt_app_hdl;
		cpnd_client_node_getnext(cb, prev_cl_hdl, &cl_node);
	}
	TRACE("***** End of Client Details ***************");

}

/****************************************************************************
 * Name          : cpnd_dump_ckpt_attri
 *
 * Description   : This is the function dumps the ckpt attri
 * Arguments     : cp_node    -  Pointer to the Ckpt Node
 *
 * Return Values : TRUE/FALSE
 *
 * Notes         : None.
 *****************************************************************************/
static
void cpnd_dump_ckpt_attri(CPND_CKPT_NODE *cp_node)
{

	TRACE("++++++++++++++++++++ATTRI+++++++++++++++++++++++++++");

	TRACE(" Creation Flags : ");
	switch (cp_node->create_attrib.creationFlags) {
	case SA_CKPT_WR_ALL_REPLICAS:
		TRACE("SA_CKPT_WR_ALL_REPLICAS");
		break;

	case SA_CKPT_WR_ACTIVE_REPLICA:
		TRACE("SA_CKPT_WR_ACTIVE_REPLICA");
		break;

	case SA_CKPT_WR_ACTIVE_REPLICA_WEAK:
		TRACE("SA_CKPT_WR_ACTIVE_REPLICA_WEAK");
		break;

	case SA_CKPT_CHECKPOINT_COLLOCATED:
		TRACE("SA_CKPT_CHECKPOINT_COLLOCATED");
		break;

	default:
		TRACE("\n Unkown creationFlags ..");
		break;
	}

	TRACE("Ckpt Size %d  retnetion time %d(scale of 100ms) ", (uns32)(cp_node->create_attrib.checkpointSize),
	       (uns32)(cp_node->create_attrib.retentionDuration));
	TRACE("Ckpt Max Sections %d Ckpt Max Section Size %d", (uns32)(cp_node->create_attrib.maxSections),
	       (uns32)(cp_node->create_attrib.maxSectionSize));
	TRACE("Ckpt Section Max Size %d ", (uns32)(cp_node->create_attrib.maxSectionIdSize));
	TRACE("++++++++++++++++++++++++++++++++++++++++++++++++++");

}

/****************************************************************************
 * Name          : cpnd_dump_shm_info
 *
 * Description   : This is the function dumps the shared Memory Info 
 * Arguments     : open    -  Pointer to the NCS_OS_POSIX_SHM_REQ_INFO
 *
 * Return Values : TRUE/FALSE
 *
 * Notes         : None.
 *****************************************************************************/
static
void cpnd_dump_shm_info(NCS_OS_POSIX_SHM_REQ_INFO *open)
{

	TRACE("+++++++++++++++++SHM++++++++++++++++++++++++++++++");

	TRACE("Shm File Name %s ", open->info.open.i_name);
	TRACE("Shm Starting Address %p", open->info.open.o_addr);
	TRACE("Shm File Descriptor %d ", (uns32)open->info.open.o_fd);

	TRACE("++++++++++++++++++++++++++++++++++++++++++++++++++");
}

/****************************************************************************
 * Name          : cpnd_dump_section_info
 *
 * Description   : This is the function dumps the Ckpt Section info
 * Arguments     : sec_info    -  Pointer to the CPND_CKPT_SECTION_INFO
 *
 * Return Values : TRUE/FALSE
 *
 * Notes         : None.
 *****************************************************************************/
static
void cpnd_dump_section_info(CPND_CKPT_SECTION_INFO *sec_info)
{
	int i;
	SaUint8T *buf;

	TRACE("++++++++++++++++++++SEC+++++++++++++++++++++++++++");
	if (sec_info != NULL) {
		TRACE("Shm Offset %d ", sec_info->lcl_sec_id);
		TRACE("Section Id ");
		i = sec_info->sec_id.idLen;
		buf = sec_info->sec_id.id;

		if (buf != NULL) {
			while (i >= 0) {
				TRACE("%c", buf[sec_info->sec_id.idLen - i]);
				i--;
			}
		}
		TRACE("Section Size %d  - Section state %d ",
		       (uns32)sec_info->sec_size, (uns32)sec_info->sec_state);
		TRACE("Expiration Time %d ", (uns32)sec_info->exp_tmr);
		TRACE("++++++++++++++++++++++++++++++++++++++++++++++++++");

	}

}

/****************************************************************************
 * Name          : cpnd_dump_replica_info
 *
 * Description   : This is the function dumps the Ckpt Replica Info
 * Arguments     : ckp_replica_node   -  Pointer to the CPND_CKPT_REPLICA_INFO
 *
 * Return Values : TRUE/FALSE
 *
 * Notes         : None.
 *****************************************************************************/
static
void cpnd_dump_replica_info(CPND_CKPT_REPLICA_INFO *ckpt_replica_node)
{
	cpnd_dump_shm_info(&ckpt_replica_node->open);

	if (ckpt_replica_node->n_secs) {
		CPND_CKPT_SECTION_INFO *sec_info = NULL;
		sec_info = ckpt_replica_node->section_info;
		while (sec_info != NULL) {
			cpnd_dump_section_info(sec_info);
			sec_info = sec_info->next;
		}
	}
}

/****************************************************************************
 * Name          : cpnd_dump_client_info
 *
 * Description   : This is the function dumps the Ckpt Client info
 * Arguments     : cl_node  -  Pointer to the CPND_CKPT_CLIENT_NODE
 *
 * Return Values : TRUE/FALSE
 *
 * Notes         : None.
 *****************************************************************************/
static
void cpnd_dump_client_info(CPND_CKPT_CLIENT_NODE *cl_node)
{
	TRACE("++++++++++++++++++++++++++++++++++++++++++++++++++\n");

	TRACE("Client_hdl  %d MDS DEST %d ",
	       (uns32)(cl_node->ckpt_app_hdl), (uns32)m_NCS_NODE_ID_FROM_MDS_DEST(cl_node->agent_mds_dest));
	TRACE("++++++++++++++++++++++++++++++++++++++++++++++++++");
}

static
void cpnd_dump_ckpt_info(CPND_CKPT_NODE *ckpt_node)
{

	CPSV_CPND_DEST_INFO *cpnd_dest_list = NULL;

	TRACE("++++++++++++++++++++++++++++++++++++++++++++++++++");
	TRACE("Ckpt_id - %d  Ckpt Name - %.10s ", (uns32)ckpt_node->ckpt_id, ckpt_node->ckpt_name.value);
	if (ckpt_node->is_unlink)
		TRACE("Ckpt Unlinked - ");
	if (ckpt_node->is_close)
		TRACE("Ckpt Closed ");
	TRACE("ref count - %d", ckpt_node->ckpt_lcl_ref_cnt);
	TRACE("ACTIVE CKPT NODE MDS dest - %d", m_NCS_NODE_ID_FROM_MDS_DEST(ckpt_node->active_mds_dest));

	TRACE("CPND List ......");

	cpnd_dest_list = ckpt_node->cpnd_dest_list;
	while (cpnd_dest_list != NULL) {
		TRACE("--%d-> ", m_NCS_NODE_ID_FROM_MDS_DEST(cpnd_dest_list->dest));
		cpnd_dest_list = cpnd_dest_list->next;
	}
	TRACE("++++++++++++++++++++++++++++++++++++++++++++++++++");
}

/****************************************************************************
 * Name          : cpnd_match_dest
 *
 * Description   : This is the function dumps the Ckpt Replica Info
 * Arguments     : key    - mds_dest of cpnd which has done a sync send to this cpnd
 *                 qelem  - item in the cb->cpnd_sync_send_list
 * Return Values : TRUE/FALSE
 *
 * Notes         : None.
 *****************************************************************************/
NCS_BOOL cpnd_match_dest(void *key, void *qelem)
{
	CPND_SYNC_SEND_NODE *node = (CPND_SYNC_SEND_NODE *)qelem;
	MDS_DEST *dest = (MDS_DEST *)key;

	if (!qelem)
		return FALSE;

	if (*dest == node->dest)
		return TRUE;

	return FALSE;
}

/****************************************************************************
 * Name          : cpnd_match_evt
 *
 * Description   : This is the function dumps the Ckpt Replica Info
 * Arguments     : key    - CPSV_EVT* sync send request to this cpnd
 *                 qelem  - item in the cb->cpnd_sync_send_list
 * Return Values : TRUE/FALSE
 *
 * Notes         : None.
 *****************************************************************************/
NCS_BOOL cpnd_match_evt(void *key, void *qelem)
{
	CPND_SYNC_SEND_NODE *node = (CPND_SYNC_SEND_NODE *)qelem;
	CPSV_EVT *evt = (CPSV_EVT *)key;

	if (!qelem)
		return FALSE;

	if (evt == node->evt)
		return TRUE;

	return FALSE;
}

/****************************************************************************
 * Name          : cpnd_is_noncollocated_replica_present_on_payload
 *
 * Description   : This is the function dumps the Ckpt Replica Info
 * Arguments     : cb       - CPND Control Block pointer
 *                 cp_node  - pointer to checkpoint node
 *
 * Return Values : TRUE/FALSE
 *****************************************************************************/
NCS_BOOL cpnd_is_noncollocated_replica_present_on_payload(CPND_CB *cb, CPND_CKPT_NODE *cp_node)
{
	CPSV_CPND_DEST_INFO *dest_list = NULL;

	/* Check if the CPND is on Active or Standby SCXB */
	if (m_CPND_IS_ON_SCXB(cb->cpnd_active_id, cpnd_get_slot_sub_slot_id_from_mds_dest(cb->cpnd_mdest_id)) ||
	    m_CPND_IS_ON_SCXB(cb->cpnd_standby_id, cpnd_get_slot_sub_slot_id_from_mds_dest(cb->cpnd_mdest_id))) {

		dest_list = cp_node->cpnd_dest_list;

		while (dest_list) {
			/* Check if a replica is present on one of the payload blades */
			if ((!m_CPND_IS_ON_SCXB
			     (cb->cpnd_active_id, cpnd_get_slot_sub_slot_id_from_mds_dest(dest_list->dest)))
			    &&
			    (!m_CPND_IS_ON_SCXB
			     (cb->cpnd_standby_id, cpnd_get_slot_sub_slot_id_from_mds_dest(dest_list->dest)))) {
				return TRUE;
			}

			dest_list = dest_list->next;
		}
	}

	return FALSE;
}

/****************************************************************************************
 * Name          : cpnd_send_ckpt_usr_info_to_cpd
 *
 * Description   : This is the function dumps the Ckpt Replica Info
 * Arguments     : cb       - CPND Control Block pointer
 *                 cp_node  - pointer to checkpoint node
 *                 ckpt_open_flags - checkpoint open flags
 *                 usrinfoType - CPSV_USR_INFO_CKPT_OPEN / CPSV_USR_INFO_CKPT_CLOSE
 * 
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *****************************************************************************************/
uns32 cpnd_send_ckpt_usr_info_to_cpd(CPND_CB *cb, CPND_CKPT_NODE *cp_node,
				     SaCkptCheckpointOpenFlagsT ckpt_open_flags, CPSV_USR_INFO_CKPT_TYPE usrinfoType)
{
	CPSV_EVT send_evt;
	uns32 rc;

	memset(&send_evt, '\0', sizeof(CPSV_EVT));

	send_evt.type = CPSV_EVT_TYPE_CPD;
	send_evt.info.cpd.type = CPD_EVT_ND2D_CKPT_USR_INFO;
	send_evt.info.cpd.info.ckpt_usr_info.ckpt_id = cp_node->ckpt_id;
	if (usrinfoType == CPSV_USR_INFO_CKPT_OPEN) {
		if (cp_node->ckpt_lcl_ref_cnt == 1) {
			usrinfoType = CPSV_USR_INFO_CKPT_OPEN_FIRST;
		}
	}
	if (usrinfoType == CPSV_USR_INFO_CKPT_CLOSE) {
		if (cp_node->ckpt_lcl_ref_cnt == 0) {
			usrinfoType = CPSV_USR_INFO_CKPT_CLOSE_LAST;
		}
	}
	send_evt.info.cpd.info.ckpt_usr_info.info_type = usrinfoType;
	send_evt.info.cpd.info.ckpt_usr_info.ckpt_flags = ckpt_open_flags;
	rc = cpnd_mds_msg_send(cb, NCSMDS_SVC_ID_CPD, cb->cpd_mdest_id, &send_evt);

	if (rc != NCSCC_RC_SUCCESS) {
		return NCSCC_RC_FAILURE;
	}

	return NCSCC_RC_SUCCESS;
}

/****************************************************************************************
 * Name          : cpnd_ckpt_replica_close
 *
 * Description   : This is the function dumps the Ckpt Replica Info
 * Arguments     : cb       - CPND Control Block pointer
 *                 cp_node  - pointer to checkpoint node
 * 
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *****************************************************************************************/

uns32 cpnd_ckpt_replica_close(CPND_CB *cb, CPND_CKPT_NODE *cp_node, SaAisErrorT *error)
{
	SaTimeT presentTime;
	uns32 rc = NCSCC_RC_SUCCESS;

	if (cp_node->ckpt_lcl_ref_cnt == 0) {

		cp_node->is_close = TRUE;
		cpnd_restart_set_close_flag(cb, cp_node);

		if (cp_node->is_unlink != TRUE &&
		    (m_CPSV_CONVERT_SATIME_TEN_MILLI_SEC(cp_node->create_attrib.retentionDuration) != 0)) {
			m_GET_TIME_STAMP(presentTime);
			cpnd_restart_update_timer(cb, cp_node, presentTime);

			cp_node->ret_tmr.type = CPND_TMR_TYPE_RETENTION;
			cp_node->ret_tmr.uarg = cb->cpnd_cb_hdl_id;
			cp_node->ret_tmr.ckpt_id = cp_node->ckpt_id;
			cpnd_tmr_start(&cp_node->ret_tmr,
				       m_CPSV_CONVERT_SATIME_TEN_MILLI_SEC(cp_node->create_attrib.retentionDuration));

			m_LOG_CPND_FCL(CPND_CKPT_RET_TMR_SUCCESS, CPND_FC_CKPTINFO, NCSFL_SEV_INFO,
				       cp_node->ckpt_id, __FILE__, __LINE__);
		} else {
			/* Check for Non-Collocated Replica */
			if (!m_CPND_IS_COLLOCATED_ATTR_SET(cp_node->create_attrib.creationFlags)) {
				if (cpnd_is_noncollocated_replica_present_on_payload(cb, cp_node)) {
					return NCSCC_RC_SUCCESS;
				}
			}
			rc = cpnd_ckpt_replica_destroy(cb, cp_node, error);
			if (rc == NCSCC_RC_FAILURE) {
				m_LOG_CPND_FCL(CPND_CKPT_REPLICA_DESTROY_FAILED, CPND_FC_GENERIC, NCSFL_SEV_ERROR,
					       cp_node->ckpt_id, __FILE__, __LINE__);
				return NCSCC_RC_FAILURE;
			}
			m_LOG_CPND_FCL(CPND_CKPT_REPLICA_DESTROY_SUCCESS, CPND_FC_GENERIC, NCSFL_SEV_INFO,
				       cp_node->ckpt_id, __FILE__, __LINE__);

			cpnd_restart_shm_ckpt_free(cb, cp_node);
			cpnd_ckpt_node_destroy(cb, cp_node);
		}
	}

	return NCSCC_RC_SUCCESS;
}

/****************************************************************************************
 * Name          : cpnd_ckpt_non_collocated_rplica_close
 *
 * Description   : This is the function close the non_collocated Ckpt Replica
 * Arguments     : cb       - CPND Control Block pointer
 *                 cp_node  - pointer to checkpoint node
 * 
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *****************************************************************************************/

uns32 cpnd_ckpt_non_collocated_rplica_close(CPND_CB *cb, CPND_CKPT_NODE *cp_node, SaAisErrorT *error)
{
	SaTimeT presentTime;
	uns32 rc = NCSCC_RC_SUCCESS;

	if (cp_node->ckpt_lcl_ref_cnt == 0) {

		cp_node->is_close = TRUE;
		cpnd_restart_set_close_flag(cb, cp_node);

		if (cp_node->is_unlink != TRUE &&
		    (m_CPSV_CONVERT_SATIME_TEN_MILLI_SEC(cp_node->create_attrib.retentionDuration) != 0)) {
			m_GET_TIME_STAMP(presentTime);
			cpnd_restart_update_timer(cb, cp_node, presentTime);

			cp_node->ret_tmr.type = CPND_TMR_TYPE_NON_COLLOC_RETENTION;
			cp_node->ret_tmr.uarg = cb->cpnd_cb_hdl_id;
			cp_node->ret_tmr.ckpt_id = cp_node->ckpt_id;
			cpnd_tmr_start(&cp_node->ret_tmr,
				       m_CPSV_CONVERT_SATIME_TEN_MILLI_SEC(cp_node->create_attrib.retentionDuration));

			m_LOG_CPND_FCL(CPND_CKPT_RET_TMR_SUCCESS, CPND_FC_CKPTINFO, NCSFL_SEV_INFO,
				       cp_node->ckpt_id, __FILE__, __LINE__);
		} else {
			/* Check for Non-Collocated Replica */
			if (cpnd_is_noncollocated_replica_present_on_payload(cb, cp_node)) {
				return NCSCC_RC_SUCCESS;
			}
			rc = cpnd_ckpt_replica_destroy(cb, cp_node, error);
			if (rc == NCSCC_RC_FAILURE) {
				m_LOG_CPND_FCL(CPND_CKPT_REPLICA_DESTROY_FAILED, CPND_FC_GENERIC, NCSFL_SEV_ERROR,
					       cp_node->ckpt_id, __FILE__, __LINE__);
				return NCSCC_RC_FAILURE;
			}
			m_LOG_CPND_FCL(CPND_CKPT_REPLICA_DESTROY_SUCCESS, CPND_FC_GENERIC, NCSFL_SEV_INFO,
				       cp_node->ckpt_id, __FILE__, __LINE__);

			cpnd_restart_shm_ckpt_free(cb, cp_node);
			cpnd_ckpt_node_destroy(cb, cp_node);
		}
	}

	return NCSCC_RC_SUCCESS;
}

void cpnd_proc_free_read_data(CPSV_EVT *evt)
{
	uns32 iter = 0;
	CPSV_ND2A_READ_DATA *ptr_read_data;
	ptr_read_data = evt->info.cpa.info.sec_data_rsp.info.read_data;

	if (evt->info.cpa.info.sec_data_rsp.info.read_data) {
		for (; iter < evt->info.cpa.info.sec_data_rsp.size; iter++) {
			if (ptr_read_data[iter].data != NULL) {
				m_MMGR_FREE_CPND_DEFAULT(ptr_read_data[iter].data);
			}
		}
		m_MMGR_FREE_CPND_DEFAULT(evt->info.cpa.info.sec_data_rsp.info.read_data);
	}
}
