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
  FILe NAME: cpd_evt.c

  DESCRIPTION: CPD Event handling routines

  FUNCTIONS INCLUDED in this module:
  cpd_process_evt .........CPD Event processing routine.
******************************************************************************/

#include "cpd.h"
#include "cpd_log.h"
#include "immutil.h"

uns32 cpd_evt_proc_cb_dump(CPD_CB *cb);
static uns32 cpd_evt_proc_ckpt_create(CPD_CB *cb, CPD_EVT *evt, CPSV_SEND_INFO *sinfo);
static uns32 cpd_evt_proc_ckpt_usr_info(CPD_CB *cb, CPD_EVT *evt, CPSV_SEND_INFO *sinfo);
/*static uns32 cpd_evt_proc_ckpt_sync_info(CPD_CB *cb,CPD_EVT *evt,CPSV_SEND_INFO *sinfo);*/
static uns32 cpd_evt_proc_ckpt_sec_info_upd(CPD_CB *cb, CPD_EVT *evt, CPSV_SEND_INFO *sinfo);
static uns32 cpd_evt_proc_ckpt_unlink(CPD_CB *cb, CPD_EVT *evt, CPSV_SEND_INFO *sinfo);
static uns32 cpd_evt_proc_ckpt_rdset(CPD_CB *cb, CPD_EVT *evt, CPSV_SEND_INFO *sinfo);

static uns32 cpd_evt_proc_active_set(CPD_CB *cb, CPD_EVT *evt, CPSV_SEND_INFO *sinfo);

static uns32 cpd_evt_proc_ckpt_destroy(CPD_CB *cb, CPD_EVT *evt, CPSV_SEND_INFO *sinfo);
static uns32 cpd_evt_proc_ckpt_destroy_byname(CPD_CB *cb, CPD_EVT *evt, CPSV_SEND_INFO *sinfo);
static uns32 cpd_evt_proc_timer_expiry(CPD_CB *cb, CPD_EVT *evt);
static uns32 cpd_evt_proc_mds_evt(CPD_CB *cb, CPD_EVT *evt);

static uns32 cpd_evt_mds_quiesced_ack_rsp(CPD_CB *cb, CPD_EVT *evt, CPSV_SEND_INFO *sinfo);

#if ( CPSV_DEBUG == 1)
static char *cpd_evt_str[] = {
	"CPD_EVT_BASE",
	"CPD_EVT_MDS_INFO",
	"CPD_EVT_ND2D_CKPT_CREATE",
	"CPD_EVT_ND2D_CKPT_UNLINK",
	"CPD_EVT_ND2D_CKPT_RDSET",
	"CPD_EVT_ND2D_ACTIVE_SET",
	"CPD_EVT_ND2D_CKPT_CLOSE",
	"CPD_EVT_ND2D_CKPT_DESTROY",
	"CPD_EVT_ND2D_CKPT_USR_INFO",
	"CPD_EVT_ND2D_CKPT_SYNC_INFO",
	"CPD_EVT_ND2D_CKPT_SEC_INFO_UPD",
	"CPD_EVT_ND2D_CKPT_MEM_USED",
	"CPD_EVT_CB_DUMP",
	"CPD_EVT_MDS_QUIESCED_ACK_RSP",
	"CPD_EVT_ND2D_CKPT_DESTROY_BYNAME",
	"CPD_EVT_ND2D_CKPT_CREATED_SECTIONS",
	"CPD_EVT_MAX"
};
#endif

/****************************************************************************
 * Name          : cpd_process_evt
 *
 * Description   : This is the top level function to process the events posted
 *                  to CPD.
 * Arguments     : 
 *   evt          : Pointer to CPSV_EVT
 * Return Values : None
 *
 * Notes         : None.
 *****************************************************************************/
void cpd_process_evt(CPSV_EVT *evt)
{
	CPD_CB *cb;
	uns32 cb_hdl = m_CPD_GET_CB_HDL;
	uns32 rc = NCSCC_RC_SUCCESS;

	if (evt->type != CPSV_EVT_TYPE_CPD) {
		/*TBD Log the error BAD EVENT */
		return;
	}

	/* Get the CB from the handle */
	cb = ncshm_take_hdl(NCS_SERVICE_ID_CPD, cb_hdl);

	if (cb == NULL) {
		m_LOG_CPD_HEADLINE(CPD_CB_HDL_TAKE_FAILED, NCSFL_SEV_ERROR);
		return;
	}
#if ( CPSV_DEBUG == 1)
	TRACE("%s", cpd_evt_str[evt->info.cpd.type]);
#endif

	switch (evt->info.cpd.type) {
	case CPD_EVT_MDS_INFO:
		rc = cpd_evt_proc_mds_evt(cb, &evt->info.cpd);
		break;
	case CPD_EVT_TIME_OUT:
		rc = cpd_evt_proc_timer_expiry(cb, &evt->info.cpd);
		break;
	case CPD_EVT_ND2D_CKPT_CREATE:
		rc = cpd_evt_proc_ckpt_create(cb, &evt->info.cpd, &evt->sinfo);
		break;
	case CPD_EVT_ND2D_CKPT_USR_INFO:
		rc = cpd_evt_proc_ckpt_usr_info(cb, &evt->info.cpd, &evt->sinfo);
		break;
	case CPD_EVT_ND2D_CKPT_SEC_INFO_UPD:
		rc = cpd_evt_proc_ckpt_sec_info_upd(cb, &evt->info.cpd, &evt->sinfo);
		break;
	case CPD_EVT_ND2D_CKPT_UNLINK:
		rc = cpd_evt_proc_ckpt_unlink(cb, &evt->info.cpd, &evt->sinfo);
		break;
	case CPD_EVT_ND2D_CKPT_RDSET:
		rc = cpd_evt_proc_ckpt_rdset(cb, &evt->info.cpd, &evt->sinfo);
		break;
	case CPD_EVT_ND2D_ACTIVE_SET:
		rc = cpd_evt_proc_active_set(cb, &evt->info.cpd, &evt->sinfo);
		break;
	case CPD_EVT_ND2D_CKPT_DESTROY:
		rc = cpd_evt_proc_ckpt_destroy(cb, &evt->info.cpd, &evt->sinfo);
		break;
	case CPD_EVT_ND2D_CKPT_DESTROY_BYNAME:
		rc = cpd_evt_proc_ckpt_destroy_byname(cb, &evt->info.cpd, &evt->sinfo);
		break;
	case CPD_EVT_MDS_QUIESCED_ACK_RSP:
		rc = cpd_evt_mds_quiesced_ack_rsp(cb, &evt->info.cpd, &evt->sinfo);
		break;

	case CPD_EVT_CB_DUMP:
		rc = cpd_evt_proc_cb_dump(cb);
		break;
	default:
		/* Log the error TBD */
		break;
	}

	/*  if(rc != NCSCC_RC_SUCCESS)
	   TBD Log the error */

	/* Return the Handle */
	ncshm_give_hdl(cb_hdl);

	/* Free the Event */
	m_MMGR_FREE_CPSV_EVT(evt, NCS_SERVICE_ID_CPD);
	return;
}

/****************************************************************************
 * Name          : cpd_evt_proc_ckpt_create
 *
 * Description   : Function to process the CPD_EVT_ND2D_CKPT_CREATE event 
 *                 from CPD.
 *
 * Arguments     : CPD_CB *cb - CPD CB pointer
 *                 CPSV_EVT *evt - Received Event structure
 *
 * Return Values : NCSCC_RC_SUCCESS/Error.
 *
 * Notes         : None.
 *****************************************************************************/
static uns32 cpd_evt_proc_ckpt_create(CPD_CB *cb, CPD_EVT *evt, CPSV_SEND_INFO *sinfo)
{
	CPSV_EVT send_evt;
	SaAisErrorT rc = SA_AIS_OK;
	uns32 proc_rc = NCSCC_RC_SUCCESS;
	CPD_CKPT_INFO_NODE *ckpt_node = 0;
	CPD_CKPT_MAP_INFO *map_info = NULL;
	CPSV_ND2D_CKPT_CREATE *ckpt_create = &evt->info.ckpt_create;
	NCS_BOOL is_first_rep = FALSE, is_new_noncol = FALSE;

	cpd_ckpt_map_node_get(&cb->ckpt_map_tree, &ckpt_create->ckpt_name, &map_info);
	if (map_info) {

		/*   ckpt_create->ckpt_name.length = m_NCS_OS_NTOHS(ckpt_create->ckpt_name.length);  */
		if (m_CPA_VER_IS_ABOVE_B_1_1(&ckpt_create->client_version)) {
			if ((ckpt_create->ckpt_flags & SA_CKPT_CHECKPOINT_CREATE) &&
			    (!m_COMPARE_CREATE_ATTR(&ckpt_create->attributes, &map_info->attributes))) {
				m_LOG_CPD_CFCL(CPD_CKPT_CREATE_FAILURE, CPD_FC_HDLN, NCSFL_SEV_NOTICE,
					       ckpt_create->ckpt_name.value, sinfo->dest, __FILE__, __LINE__);
				rc = SA_AIS_ERR_EXIST;
				goto send_rsp;
			}
		} else {
			if ((ckpt_create->ckpt_flags & SA_CKPT_CHECKPOINT_CREATE) &&
			    (!m_COMPARE_CREATE_ATTR_B_1_1(&ckpt_create->attributes, &map_info->attributes))) {
				m_LOG_CPD_CFCL(CPD_CKPT_CREATE_FAILURE, CPD_FC_HDLN, NCSFL_SEV_NOTICE,
					       ckpt_create->ckpt_name.value, sinfo->dest, __FILE__, __LINE__);
				rc = SA_AIS_ERR_EXIST;
				goto send_rsp;
			}
		}
	} else {
		SaCkptCheckpointCreationAttributesT ckpt_local_attrib;
		memset(&ckpt_local_attrib, 0, sizeof(SaCkptCheckpointCreationAttributesT));
		/* ckpt_create->ckpt_name.length = m_NCS_OS_NTOHS(ckpt_create->ckpt_name.length); */
		is_first_rep = TRUE;
		if (m_CPA_VER_IS_ABOVE_B_1_1(&ckpt_create->client_version)) {
			if (!(ckpt_create->ckpt_flags & SA_CKPT_CHECKPOINT_CREATE) &&
			    (m_COMPARE_CREATE_ATTR(&ckpt_create->attributes, &ckpt_local_attrib))) {
				m_LOG_CPD_CFCL(CPD_CKPT_CREATE_FAILURE, CPD_FC_HDLN, NCSFL_SEV_ERROR,
					       ckpt_create->ckpt_name.value, sinfo->dest, __FILE__, __LINE__);
				rc = SA_AIS_ERR_NOT_EXIST;
				goto send_rsp;
			}
		} else {
			if (!(ckpt_create->ckpt_flags & SA_CKPT_CHECKPOINT_CREATE) &&
			    (m_COMPARE_CREATE_ATTR_B_1_1(&ckpt_create->attributes, &ckpt_local_attrib))) {
				m_LOG_CPD_CFCL(CPD_CKPT_CREATE_FAILURE, CPD_FC_HDLN, NCSFL_SEV_ERROR,
					       ckpt_create->ckpt_name.value, sinfo->dest, __FILE__, __LINE__);
				rc = SA_AIS_ERR_NOT_EXIST;
				goto send_rsp;
			}
		}
	}

	/* Add/Update the entries in ckpt DB, ckpt_map DB, ckpt_node DB */
	proc_rc = cpd_ckpt_db_entry_update(cb, &sinfo->dest, ckpt_create, &ckpt_node, &map_info);
	if (proc_rc == NCSCC_RC_OUT_OF_MEM) {
		/*m_LOG_CPD_CFCL(CPD_DB_ADD_FAILED,CPD_FC_DB,NCSFL_SEV_ERROR,ckpt_create->ckpt_name.value,sinfo->dest,__FILE__,__LINE__); */
		m_LOG_CPD_CFCL(CPD_CKPT_CREATE_FAILURE, CPD_FC_HDLN, NCSFL_SEV_ERROR, ckpt_create->ckpt_name.value,
			       sinfo->dest, __FILE__, __LINE__);
		rc = SA_AIS_ERR_NO_MEMORY;
		goto send_rsp;
	} else if (proc_rc != NCSCC_RC_SUCCESS) {
		m_LOG_CPD_CFCL(CPD_CKPT_CREATE_FAILURE, CPD_FC_HDLN, NCSFL_SEV_ERROR, ckpt_create->ckpt_name.value,
			       sinfo->dest, __FILE__, __LINE__);
		rc = SA_AIS_ERR_LIBRARY;
		goto send_rsp;
	}

	if (ckpt_create->ckpt_flags & SA_CKPT_CHECKPOINT_READ)
		ckpt_node->num_readers++;
	if (ckpt_create->ckpt_flags & SA_CKPT_CHECKPOINT_WRITE)
		ckpt_node->num_writers++;

	ckpt_node->num_users++;

	cb->is_db_upd = TRUE;

	/* Redundancy A2S   This is for async update   */
	/* Only for 1st  replica we send the entire info , for later openings we send dest_add */
	if (is_first_rep) {
		cpd_a2s_ckpt_create(cb, ckpt_node);
		cpd_a2s_ckpt_usr_info(cb, ckpt_node);
	}

	/* Send the dest info to the Standby SCXB This is for async update */
	if (is_first_rep == FALSE) {
		cpd_a2s_ckpt_dest_add(cb, ckpt_node, &sinfo->dest);
		cpd_a2s_ckpt_usr_info(cb, ckpt_node);
	}
	/* Non-colocated processing */
	if ((is_first_rep == TRUE) && (!(map_info->attributes.creationFlags & SA_CKPT_CHECKPOINT_COLLOCATED))) {
		/* Policy is to create the replic on both active & standby SCXB's CPND 
		   Right now replica is created only on the active (local) SCXB */
		/* if(cb->is_loc_cpnd_up && (cpd_get_slot_sub_id_from_mds_dest(sinfo->dest) != ckpt_node->ckpt_on_scxb1)) */
		if (cb->is_loc_cpnd_up
		    && (!m_CPND_IS_ON_SCXB(cb->cpd_self_id, cpd_get_slot_sub_id_from_mds_dest(sinfo->dest)))) {
			proc_rc = cpd_noncolloc_ckpt_rep_create(cb, &cb->loc_cpnd_dest, ckpt_node, map_info);
			if (proc_rc == NCSCC_RC_SUCCESS)
				m_LOG_CPD_CFCL(CPD_NON_COLOC_CKPT_CREATE_SUCCESS, CPD_FC_HDLN, NCSFL_SEV_INFO,
					       ckpt_create->ckpt_name.value, cb->loc_cpnd_dest, __FILE__, __LINE__);
			else
				m_LOG_CPD_CFCL(CPD_NON_COLOC_CKPT_CREATE_FAILURE, CPD_FC_HDLN, NCSFL_SEV_NOTICE,
					       ckpt_create->ckpt_name.value, cb->loc_cpnd_dest, __FILE__, __LINE__);

		}
		/*  if(cb->is_rem_cpnd_up && (cpd_get_slot_sub_id_from_mds_dest(sinfo->dest) != ckpt_node->ckpt_on_scxb2)) */
		if (cb->is_rem_cpnd_up
		    && (!m_CPND_IS_ON_SCXB(cb->cpd_remote_id, cpd_get_slot_sub_id_from_mds_dest(sinfo->dest)))) {
			proc_rc = cpd_noncolloc_ckpt_rep_create(cb, &cb->rem_cpnd_dest, ckpt_node, map_info);
			if (proc_rc == NCSCC_RC_SUCCESS)
				m_LOG_CPD_CFCL(CPD_NON_COLOC_CKPT_CREATE_SUCCESS, CPD_FC_HDLN, NCSFL_SEV_INFO,
					       ckpt_create->ckpt_name.value, cb->rem_cpnd_dest, __FILE__, __LINE__);
			else
				m_LOG_CPD_CFCL(CPD_NON_COLOC_CKPT_CREATE_FAILURE, CPD_FC_HDLN, NCSFL_SEV_NOTICE,
					       ckpt_create->ckpt_name.value, cb->rem_cpnd_dest, __FILE__, __LINE__);
		}
		/* ND on SCXB has created the same checkpoint, so is_new_noncol must be made to true */
		is_new_noncol = TRUE;

	}

 send_rsp:
	/* Send the response to the creater of this ckpt */
	/* Populate & Send the Open Event to CPND */
	memset(&send_evt, 0, sizeof(CPSV_EVT));
	send_evt.type = CPSV_EVT_TYPE_CPND;
	send_evt.info.cpnd.type = CPND_EVT_D2ND_CKPT_INFO;
	send_evt.info.cpnd.info.ckpt_info.error = rc;
	if (rc == SA_AIS_OK) {
		send_evt.info.cpnd.info.ckpt_info.ckpt_id = ckpt_node->ckpt_id;
		send_evt.info.cpnd.info.ckpt_info.is_active_exists = ckpt_node->is_active_exists;
		send_evt.info.cpnd.info.ckpt_info.attributes = map_info->attributes;
		if (send_evt.info.cpnd.info.ckpt_info.is_active_exists)
			send_evt.info.cpnd.info.ckpt_info.active_dest = ckpt_node->active_dest;

		if (map_info->attributes.creationFlags & SA_CKPT_CHECKPOINT_COLLOCATED)
			send_evt.info.cpnd.info.ckpt_info.ckpt_rep_create = TRUE;
		else {
			if (is_first_rep)
				send_evt.info.cpnd.info.ckpt_info.ckpt_rep_create = TRUE;
			else
				send_evt.info.cpnd.info.ckpt_info.ckpt_rep_create = FALSE;
		}

		if (ckpt_node->dest_cnt) {
			CPD_NODE_REF_INFO *node_list = ckpt_node->node_list;
			uns32 i = 0;

			send_evt.info.cpnd.info.ckpt_info.dest_cnt = ckpt_node->dest_cnt;
			send_evt.info.cpnd.info.ckpt_info.dest_list =
			    m_MMGR_ALLOC_CPSV_CPND_DEST_INFO(ckpt_node->dest_cnt);
			if (send_evt.info.cpnd.info.ckpt_info.dest_list == NULL) {
				send_evt.info.cpnd.info.ckpt_info.error = SA_AIS_ERR_NO_MEMORY;
				rc = SA_AIS_ERR_NO_MEMORY;
				m_LOG_CPD_CL(CPD_CPND_DEST_INFO_ALLOC_FAILED, CPD_FC_MEMFAIL, NCSFL_SEV_ERROR, __FILE__,
					     __LINE__);
				proc_rc = NCSCC_RC_OUT_OF_MEM;
			} else {
				memset(send_evt.info.cpnd.info.ckpt_info.dest_list, 0,
				       (sizeof(CPSV_CPND_DEST_INFO) * ckpt_node->dest_cnt));

				for (i = 0; i < ckpt_node->dest_cnt; i++) {
					send_evt.info.cpnd.info.ckpt_info.dest_list[i].dest = node_list->dest;
					node_list = node_list->next;
				}
			}

		}
	}

	proc_rc = cpd_mds_send_rsp(cb, sinfo, &send_evt);

	if (send_evt.info.cpnd.info.ckpt_info.dest_list)
		m_MMGR_FREE_CPSV_CPND_DEST_INFO(send_evt.info.cpnd.info.ckpt_info.dest_list);

	if (proc_rc != NCSCC_RC_SUCCESS)
		m_LOG_CPD_CFCL(CPD_CKPT_CREATE_FAILURE, CPD_FC_HDLN, NCSFL_SEV_ERROR, ckpt_create->ckpt_name.value,
			       sinfo->dest, __FILE__, __LINE__);

	if ((proc_rc != NCSCC_RC_SUCCESS) || (rc != SA_AIS_OK))
		return proc_rc;

	/* Ckpt info successfully updated at CPD, send it to all CPNDs */
	/* Broadcast the ckpt add info to all the CPNDs, only the relevent CPNDs 
	   will process this message */
	if ((is_first_rep == FALSE) || (is_new_noncol == TRUE)) {
		memset(&send_evt, 0, sizeof(CPSV_EVT));
		send_evt.type = CPSV_EVT_TYPE_CPND;
		send_evt.info.cpnd.type = CPND_EVT_D2ND_CKPT_REP_ADD;
		send_evt.info.cpnd.info.ckpt_add.ckpt_id = ckpt_node->ckpt_id;
		send_evt.info.cpnd.info.ckpt_add.mds_dest = sinfo->dest;
		send_evt.info.cpnd.info.ckpt_add.is_cpnd_restart = FALSE;

		proc_rc = cpd_mds_bcast_send(cb, &send_evt, NCSMDS_SVC_ID_CPND);
		m_LOG_CPD_FFCL(CPD_REP_ADD_SUCCESS, CPD_FC_DB, NCSFL_SEV_INFO, map_info->ckpt_id, sinfo->dest, __FILE__,
			       __LINE__);
	}
	if (is_first_rep)
		m_LOG_CPD_CFFCL(CPD_CKPT_CREATE_SUCCESS, CPD_FC_HDLN, NCSFL_SEV_INFO, map_info->ckpt_name.value,
				sinfo->dest, map_info->ckpt_id, __FILE__, __LINE__);
	else
		m_LOG_CPD_FFCL(CPD_CKPT_CREATE_SUCCESS, CPD_FC_HDLN, NCSFL_SEV_INFO,
			       sinfo->dest, map_info->ckpt_id, __FILE__, __LINE__);

	return proc_rc;
}

/***************************************************************************
  Name          : cpd_evt_proc_ckpt_usr_info
 *
 * Description   : Function to process the Open flags to determine the number of readers and writers
 * Arguments     :
 *
 * Return Values :
***************************************************************************/
static uns32 cpd_evt_proc_ckpt_usr_info(CPD_CB *cb, CPD_EVT *evt, CPSV_SEND_INFO *sinfo)
{
	CPD_CKPT_INFO_NODE *ckpt_node;
	CPSV_EVT send_evt;
	uns32 rc = NCSCC_RC_SUCCESS;

	rc = cpd_ckpt_node_get(&cb->ckpt_tree, &evt->info.ckpt_usr_info.ckpt_id, &ckpt_node);
	if (ckpt_node == 0) {
		m_LOG_CPD_FFCL(CPD_CKPT_INFO_NODE_GET_FAILED, CPD_FC_HDLN, NCSFL_SEV_ERROR,
			       evt->info.ckpt_usr_info.ckpt_id, sinfo->dest, __FILE__, __LINE__);
		rc = NCSCC_RC_FAILURE;
		return rc;
	}
	
	if (evt->info.ckpt_usr_info.info_type == CPSV_USR_INFO_CKPT_OPEN_FIRST) {
		if (!(m_IS_SA_CKPT_CHECKPOINT_COLLOCATED(&ckpt_node->attributes))) {

			TRACE("  non-collocated CPSV_CKPT_RDSET_STOP ckpt_node->ckpt_id %llu ckpt_node->num_users %d ", 
				       (SaUint64T)ckpt_node->ckpt_id,ckpt_node->num_users);
			/* Clients for  non-collocated Ckpt , Stop ret timer ,  broadcast to all CPNDs */
			memset(&send_evt, 0, sizeof(CPSV_EVT));
			send_evt.type = CPSV_EVT_TYPE_CPND;
			send_evt.info.cpnd.type = CPND_EVT_D2ND_CKPT_RDSET;
			send_evt.info.cpnd.info.rdset.ckpt_id = ckpt_node->ckpt_id;
			send_evt.info.cpnd.info.rdset.type = CPSV_CKPT_RDSET_STOP;
			rc = cpd_mds_bcast_send(cb, &send_evt, NCSMDS_SVC_ID_CPND);
			m_LOG_CPD_FFCL(CPD_CKPT_RDSET_SUCCESS, CPD_FC_HDLN, NCSFL_SEV_INFO, ckpt_node->ckpt_id,
				       ckpt_node->active_dest, __FILE__, __LINE__);

			if (m_CPND_IS_ON_SCXB(cb->cpd_self_id, cpd_get_slot_sub_id_from_mds_dest(sinfo->dest))) {

				if (evt->info.ckpt_usr_info.info_type == CPSV_USR_INFO_CKPT_OPEN_FIRST) {
					if (!ckpt_node->ckpt_on_scxb1)
						ckpt_node->ckpt_on_scxb1 =
						    (uns32)cpd_get_slot_sub_id_from_mds_dest(sinfo->dest);
					else
						ckpt_node->ckpt_on_scxb2 =
						    (uns32)cpd_get_slot_sub_id_from_mds_dest(sinfo->dest);

				}
			}
			if (m_CPND_IS_ON_SCXB(cb->cpd_remote_id, cpd_get_slot_sub_id_from_mds_dest(sinfo->dest))) {
				if (evt->info.ckpt_usr_info.info_type == CPSV_USR_INFO_CKPT_OPEN_FIRST) {
					if (!ckpt_node->ckpt_on_scxb1)
						ckpt_node->ckpt_on_scxb1 =
						    (uns32)cpd_get_slot_sub_id_from_mds_dest(sinfo->dest);
					else
						ckpt_node->ckpt_on_scxb2 =
						    (uns32)cpd_get_slot_sub_id_from_mds_dest(sinfo->dest);
				}
			}
		}

		ckpt_node->num_users++;

		if (evt->info.ckpt_usr_info.ckpt_flags & SA_CKPT_CHECKPOINT_READ)
			ckpt_node->num_readers++;
		if (evt->info.ckpt_usr_info.ckpt_flags & SA_CKPT_CHECKPOINT_WRITE)
			ckpt_node->num_writers++;
	}
	if (evt->info.ckpt_usr_info.info_type == CPSV_USR_INFO_CKPT_CLOSE_LAST) {

		if (!(m_IS_SA_CKPT_CHECKPOINT_COLLOCATED(&ckpt_node->attributes))) {
			TRACE("  non-collocated  CPSV_CKPT_RDSET_START ckpt_node->ckpt_id %llu ckpt_node->num_users %d ", 
				       (SaUint64T)ckpt_node->ckpt_id,ckpt_node->num_users);
			/* Clients for  non-collocated Ckpt , Stop ret timer ,  broadcast to all CPNDs */
			memset(&send_evt, 0, sizeof(CPSV_EVT));
			send_evt.type = CPSV_EVT_TYPE_CPND;
			send_evt.info.cpnd.type = CPND_EVT_D2ND_CKPT_RDSET;
			send_evt.info.cpnd.info.rdset.ckpt_id = ckpt_node->ckpt_id;
			send_evt.info.cpnd.info.rdset.type = CPSV_CKPT_RDSET_START;
			rc = cpd_mds_bcast_send(cb, &send_evt, NCSMDS_SVC_ID_CPND);
			m_LOG_CPD_FFCL(CPD_CKPT_RDSET_SUCCESS, CPD_FC_HDLN, NCSFL_SEV_INFO, ckpt_node->ckpt_id,
				       ckpt_node->active_dest, __FILE__, __LINE__);
			if (m_CPND_IS_ON_SCXB(ckpt_node->ckpt_on_scxb1, cpd_get_slot_sub_id_from_mds_dest(sinfo->dest))) {
				if (evt->info.ckpt_usr_info.info_type == CPSV_USR_INFO_CKPT_CLOSE_LAST) {
					ckpt_node->ckpt_on_scxb1 = 0;
				}

			}
			if (m_CPND_IS_ON_SCXB(ckpt_node->ckpt_on_scxb2, cpd_get_slot_sub_id_from_mds_dest(sinfo->dest))) {
				if (evt->info.ckpt_usr_info.info_type == CPSV_USR_INFO_CKPT_CLOSE_LAST) {
					ckpt_node->ckpt_on_scxb2 = 0;
				}
			}

		}

		ckpt_node->num_users--;

		if (evt->info.ckpt_usr_info.ckpt_flags & SA_CKPT_CHECKPOINT_READ)
			ckpt_node->num_readers--;
		if (evt->info.ckpt_usr_info.ckpt_flags & SA_CKPT_CHECKPOINT_WRITE)
			ckpt_node->num_writers--;
	}

	cpd_a2s_ckpt_usr_info(cb, ckpt_node);

	return rc;
}

/***************************************************************************
 * Name         : cpd_evt_proc_ckpt_sec_info_upd
 *
 * Description  : To get the number of sections 
 **************************************************************************/
uns32 cpd_evt_proc_ckpt_sec_info_upd(CPD_CB *cb, CPD_EVT *evt, CPSV_SEND_INFO *sinfo)
{
	CPD_CKPT_INFO_NODE *ckpt_node;
	uns32 rc = NCSCC_RC_SUCCESS;

	cpd_ckpt_node_get(&cb->ckpt_tree, &evt->info.ckpt_sec_info.ckpt_id, &ckpt_node);
	if (ckpt_node == 0) {
		return NCSCC_RC_FAILURE;
	}

	if (evt->info.ckpt_sec_info.info_type == CPSV_CKPT_SEC_INFO_CREATE) {
		ckpt_node->num_sections++;
	} else if (evt->info.ckpt_sec_info.info_type == CPSV_CKPT_SEC_INFO_DELETE) {
		ckpt_node->num_sections--;
	}

	cpd_a2s_ckpt_usr_info(cb, ckpt_node);

	return rc;
}

/****************************************************************************
 * Name          : cpd_evt_proc_ckpt_unlink
 *
 * Description   : Function to process the Unlink request received from CPND.
 *
 * Arguments     : CPD_CB *cb - CPND CB pointer
 *                 CPSV_EVT *evt - Received Event structure
 *
 * Return Values : NCSCC_RC_SUCCESS/Error.
 *
 * Notes         : None.
 *****************************************************************************/
static uns32 cpd_evt_proc_ckpt_unlink(CPD_CB *cb, CPD_EVT *evt, CPSV_SEND_INFO *sinfo)
{
	CPD_CKPT_INFO_NODE *ckpt_node = NULL;
	CPD_CKPT_MAP_INFO *map_info = NULL;
	SaNameT *ckpt_name = &evt->info.ckpt_ulink.ckpt_name;
	SaAisErrorT rc = SA_AIS_OK;
	SaAisErrorT proc_rc = SA_AIS_OK;
	CPSV_EVT send_evt;

	rc = cpd_proc_unlink_set(cb, &ckpt_node, map_info, ckpt_name);
	if (rc != SA_AIS_OK)
		goto send_rsp;

	/* Redundancy  A2S */
	cpd_a2s_ckpt_unlink_set(cb, ckpt_node);

 send_rsp:
	memset(&send_evt, 0, sizeof(CPSV_EVT));
	send_evt.type = CPSV_EVT_TYPE_CPND;
	send_evt.info.cpnd.type = CPND_EVT_D2ND_CKPT_UNLINK_ACK;
	send_evt.info.cpnd.info.ulink_ack.error = rc;
	proc_rc = cpd_mds_send_rsp(cb, sinfo, &send_evt);

	if (rc == SA_AIS_OK) {
		/* Broadcast the Unlink info to all CPNDs */
		memset(&send_evt, 0, sizeof(CPSV_EVT));
		send_evt.type = CPSV_EVT_TYPE_CPND;
		send_evt.info.cpnd.type = CPND_EVT_D2ND_CKPT_UNLINK;
		send_evt.info.cpnd.info.ckpt_ulink.ckpt_id = ckpt_node->ckpt_id;

		proc_rc = cpd_mds_bcast_send(cb, &send_evt, NCSMDS_SVC_ID_CPND);
		m_LOG_CPD_CFCL(CPD_EVT_UNLINK_SUCCESS, CPD_FC_HDLN, NCSFL_SEV_INFO,
			       evt->info.ckpt_ulink.ckpt_name.value, sinfo->dest, __FILE__, __LINE__);

		/* delete imm ckpt runtime object */
		if (cb->ha_state == SA_AMF_HA_ACTIVE) {
			if (immutil_saImmOiRtObjectDelete(cb->immOiHandle, &ckpt_node->ckpt_name) != SA_AIS_OK) {
				cpd_log(NCSFL_SEV_ERROR, "Deleting run time object %s FAILED",
					ckpt_node->ckpt_name.value);
				/* Free the Client Node */
			}
		}
	}

	return proc_rc;
}

/****************************************************************************
 * Name          : cpd_evt_proc_ckpt_rdset
 *
 * Description   : Function to process the retention duration set.
 *
 * Arguments     : CPND_CB *cb - CPND CB pointer
 *                 CPSV_EVT *evt - Received Event structure
 *
 * Return Values : NCSCC_RC_SUCCESS/Error.
 *
 * Notes         : None.
 *****************************************************************************/
static uns32 cpd_evt_proc_ckpt_rdset(CPD_CB *cb, CPD_EVT *evt, CPSV_SEND_INFO *sinfo)
{
	CPD_CKPT_INFO_NODE *ckpt_node = NULL;
	SaAisErrorT rc = SA_AIS_OK;
	CPSV_EVT send_evt;
	uns32 proc_rc = NCSCC_RC_SUCCESS;

	rc = cpd_proc_retention_set(cb, evt->info.rd_set.ckpt_id, evt->info.rd_set.reten_time, &ckpt_node);
	if (rc != SA_AIS_OK)
		goto send_rsp;

	/*  REDUNDANCY  A2S  */
	cpd_a2s_ckpt_rdset(cb, ckpt_node);

	m_LOG_CPD_FCL(CPD_CKPT_RDSET_SUCCESS, CPD_FC_HDLN, NCSFL_SEV_INFO, evt->info.rd_set.ckpt_id,
		      __FILE__, __LINE__);

 send_rsp:
	memset(&send_evt, 0, sizeof(CPSV_EVT));
	send_evt.type = CPSV_EVT_TYPE_CPND;
	send_evt.info.cpnd.type = CPND_EVT_D2ND_CKPT_RDSET_ACK;
	send_evt.info.cpnd.info.rdset_ack.error = rc;
	proc_rc = cpd_mds_send_rsp(cb, sinfo, &send_evt);

	if (rc == SA_AIS_OK) {
		/* Broadcast the Retention Duration info to all CPNDs */
		memset(&send_evt, 0, sizeof(CPSV_EVT));
		send_evt.type = CPSV_EVT_TYPE_CPND;
		send_evt.info.cpnd.type = CPND_EVT_D2ND_CKPT_RDSET;
		send_evt.info.cpnd.info.rdset.ckpt_id = evt->info.rd_set.ckpt_id;
		send_evt.info.cpnd.info.rdset.reten_time = evt->info.rd_set.reten_time;
		send_evt.info.cpnd.info.rdset.type = CPSV_CKPT_RDSET_INFO;
		proc_rc = cpd_mds_bcast_send(cb, &send_evt, NCSMDS_SVC_ID_CPND);
	}

	return proc_rc;
}

/****************************************************************************
 * Name          : cpd_evt_proc_active_set
 *
 * Description   : Function to process the Timer expiry events at CPND.
 *
 * Arguments     : CPND_CB *cb - CPND CB pointer
 *                 CPSV_EVT *evt - Received Event structure
 *
 * Return Values : NCSCC_RC_SUCCESS/Error.
 *
 * Notes         : None.
 *****************************************************************************/
static uns32 cpd_evt_proc_active_set(CPD_CB *cb, CPD_EVT *evt, CPSV_SEND_INFO *sinfo)
{
	CPD_CKPT_INFO_NODE *ckpt_node = NULL;
	SaAisErrorT rc = SA_AIS_OK;
	CPSV_EVT send_evt;
	uns32 proc_rc = NCSCC_RC_SUCCESS;

	rc = cpd_proc_active_set(cb, evt->info.arep_set.ckpt_id, evt->info.arep_set.mds_dest, &ckpt_node);
	if (rc != SA_AIS_OK)
		goto send_rsp;

	/* REDUNDANCY  A2S  */
	cpd_a2s_ckpt_arep_set(cb, ckpt_node);

 send_rsp:
	memset(&send_evt, 0, sizeof(CPSV_EVT));
	send_evt.type = CPSV_EVT_TYPE_CPND;
	send_evt.info.cpnd.type = CPND_EVT_D2ND_CKPT_ACTIVE_SET_ACK;
	send_evt.info.cpnd.info.arep_ack.error = rc;
	proc_rc = cpd_mds_send_rsp(cb, sinfo, &send_evt);

	if (rc == SA_AIS_OK) {
		/* Broadcast the Active Replica info to all CPNDs */
		memset(&send_evt, 0, sizeof(CPSV_EVT));
		send_evt.type = CPSV_EVT_TYPE_CPND;
		send_evt.info.cpnd.type = CPND_EVT_D2ND_CKPT_ACTIVE_SET;
		send_evt.info.cpnd.info.active_set.ckpt_id = evt->info.arep_set.ckpt_id;
		send_evt.info.cpnd.info.active_set.mds_dest = evt->info.arep_set.mds_dest;
		proc_rc = cpd_mds_bcast_send(cb, &send_evt, NCSMDS_SVC_ID_CPND);
	}

	/*Broadcast the active MDS_DEST info of ckpt to all CPA's */
	if (rc == SA_AIS_OK) {
		memset(&send_evt, 0, sizeof(CPSV_EVT));
		send_evt.type = CPSV_EVT_TYPE_CPA;
		send_evt.info.cpa.type = CPA_EVT_D2A_ACT_CKPT_INFO_BCAST_SEND;
		send_evt.info.cpa.info.ackpt_info.ckpt_id = evt->info.arep_set.ckpt_id;
		send_evt.info.cpa.info.ackpt_info.mds_dest = evt->info.arep_set.mds_dest;
		proc_rc = cpd_mds_bcast_send(cb, &send_evt, NCSMDS_SVC_ID_CPA);
		m_LOG_CPD_FFCL(CPD_CKPT_ACTIVE_SET_SUCCESS, CPD_FC_HDLN, NCSFL_SEV_INFO, evt->info.arep_set.ckpt_id,
			       evt->info.arep_set.mds_dest, __FILE__, __LINE__);
	}

	return proc_rc;
}

/****************************************************************************
 * Name          : cpd_evt_proc_ckpt_destroy
 *
 * Description   : Function to process the Timer expiry events at CPND.
 *
 * Arguments     : CPND_CB *cb - CPND CB pointer
 *                 CPSV_EVT *evt - Received Event structure
 *
 * Return Values : NCSCC_RC_SUCCESS/Error.
 *
 * Notes         : None.
 *****************************************************************************/
static uns32 cpd_evt_proc_ckpt_destroy(CPD_CB *cb, CPD_EVT *evt, CPSV_SEND_INFO *sinfo)
{
	CPD_CKPT_INFO_NODE *ckpt_node = NULL;
	uns32 proc_rc = NCSCC_RC_SUCCESS;
	CPSV_EVT send_evt;
	NCS_BOOL o_ckpt_node_deleted = FALSE;
	NCS_BOOL o_is_active_changed = FALSE;
	NCS_BOOL ckptid_flag = FALSE;

	memset(&send_evt, 0, sizeof(CPSV_EVT));

	cpd_ckpt_node_get(&cb->ckpt_tree, &evt->info.ckpt_destroy.ckpt_id, &ckpt_node);

	if (ckpt_node == 0) {
		send_evt.info.cpnd.info.destroy_ack.error = SA_AIS_ERR_NOT_EXIST;
		m_LOG_CPD_FFCL(CPD_REP_DEL_FAILED, CPD_FC_DB, NCSFL_SEV_ERROR, evt->info.ckpt_destroy.ckpt_id,
			       sinfo->dest, __FILE__, __LINE__);
		goto send_rsp;
	}

	proc_rc = cpd_process_ckpt_delete(cb, ckpt_node, sinfo, &o_ckpt_node_deleted, &o_is_active_changed);
	if (proc_rc != NCSCC_RC_SUCCESS) {
		m_LOG_CPD_FFCL(CPD_REP_DEL_FAILED, CPD_FC_DB, NCSFL_SEV_ERROR, evt->info.ckpt_destroy.ckpt_id,
			       sinfo->dest, __FILE__, __LINE__);
		send_evt.info.cpnd.info.destroy_ack.error = SA_AIS_ERR_LIBRARY;
		goto send_rsp;
	}

	/* If ckpt_node is deleted, No need to send ckpt destroy to CPNDs, as 
	   none of the CPNDs are having replicas of this ckpt */
	if (o_ckpt_node_deleted) {
		/* Broadcast the Active Replica info to all CPNDs */
		memset(&send_evt, 0, sizeof(CPSV_EVT));
		send_evt.type = CPSV_EVT_TYPE_CPND;
		send_evt.info.cpnd.type = CPND_EVT_D2ND_CKPT_REP_DEL;
		send_evt.info.cpnd.info.ckpt_del.ckpt_id = ckpt_node->ckpt_id;
		send_evt.info.cpnd.info.ckpt_del.mds_dest = sinfo->dest;

		/* Broadcast to all the ND's */
		proc_rc = cpd_mds_bcast_send(cb, &send_evt, NCSMDS_SVC_ID_CPND);
		m_LOG_CPD_FFCL(CPD_REP_DEL_SUCCESS, CPD_FC_DB, NCSFL_SEV_INFO, evt->info.ckpt_destroy.ckpt_id,
			       sinfo->dest, __FILE__, __LINE__);
	}

	/* Send the New Active in case if the Active replica got changed */
	if (o_is_active_changed) {
		memset(&send_evt, 0, sizeof(CPSV_EVT));
		send_evt.type = CPSV_EVT_TYPE_CPND;
		send_evt.info.cpnd.type = CPND_EVT_D2ND_CKPT_ACTIVE_SET;
		send_evt.info.cpnd.info.active_set.ckpt_id = ckpt_node->ckpt_id;
		send_evt.info.cpnd.info.active_set.mds_dest = ckpt_node->active_dest;
		/* Send the new active set to the Stand by */
		cpd_a2s_ckpt_arep_set(cb, ckpt_node);
		/* Send it to all ND's  */
		proc_rc = cpd_mds_bcast_send(cb, &send_evt, NCSMDS_SVC_ID_CPND);

		/*To broadcast the active MDS_DEST info of ckpt to all CPA's */
		memset(&send_evt, 0, sizeof(CPSV_EVT));
		send_evt.type = CPSV_EVT_TYPE_CPA;
		send_evt.info.cpa.type = CPA_EVT_D2A_ACT_CKPT_INFO_BCAST_SEND;
		send_evt.info.cpa.info.ackpt_info.ckpt_id = ckpt_node->ckpt_id;
		send_evt.info.cpa.info.ackpt_info.mds_dest = ckpt_node->active_dest;
		proc_rc = cpd_mds_bcast_send(cb, &send_evt, NCSMDS_SVC_ID_CPA);
		m_LOG_CPD_FFCL(CPD_CKPT_ACTIVE_CHANGE_SUCCESS, CPD_FC_HDLN, NCSFL_SEV_INFO,
			       evt->info.ckpt_destroy.ckpt_id, ckpt_node->active_dest, __FILE__, __LINE__);
	}

	/* Send this info to Standby */
	cpd_a2s_ckpt_dest_del(cb, evt->info.ckpt_destroy.ckpt_id, &sinfo->dest, ckptid_flag);

	memset(&send_evt, 0, sizeof(CPSV_EVT));
	send_evt.info.cpnd.info.destroy_ack.error = SA_AIS_OK;

 send_rsp:
	send_evt.type = CPSV_EVT_TYPE_CPND;
	send_evt.info.cpnd.type = CPND_EVT_D2ND_CKPT_DESTROY_ACK;
	proc_rc = cpd_mds_send_rsp(cb, sinfo, &send_evt);

	return proc_rc;
}

/****************************************************************************
 * Name          : cpd_evt_proc_ckpt_destroy_byname
 *
 * Description   : Function to destroy checkpoint by name
 *
 * Arguments     : CPND_CB *cb - CPND CB pointer
 *                 CPSV_EVT *evt - Received Event structure
 *
 * Return Values : NCSCC_RC_SUCCESS/Error.
 *
 * Notes         : None.
 *****************************************************************************/

static uns32 cpd_evt_proc_ckpt_destroy_byname(CPD_CB *cb, CPD_EVT *evt, CPSV_SEND_INFO *sinfo)
{
	CPD_CKPT_MAP_INFO *map_info = NULL;
	SaNameT *ckpt_name = &evt->info.ckpt_destroy_byname.ckpt_name;
	CPD_EVT destroy_evt;
	CPSV_EVT send_evt;
	uns32 proc_rc = NCSCC_RC_SUCCESS;

	cpd_ckpt_map_node_get(&cb->ckpt_map_tree, ckpt_name, &map_info);

	if (map_info) {

		memset(&destroy_evt, '\0', sizeof(CPD_EVT));

		destroy_evt.type = CPD_EVT_ND2D_CKPT_DESTROY;
		destroy_evt.info.ckpt_destroy.ckpt_id = map_info->ckpt_id;

		proc_rc = cpd_evt_proc_ckpt_destroy(cb, &destroy_evt, sinfo);
	} else {
		memset(&send_evt, 0, sizeof(CPSV_EVT));

		send_evt.type = CPSV_EVT_TYPE_CPND;
		send_evt.info.cpnd.type = CPND_EVT_D2ND_CKPT_DESTROY_ACK;
		send_evt.info.cpnd.info.destroy_ack.error = SA_AIS_ERR_NOT_EXIST;

		proc_rc = cpd_mds_send_rsp(cb, sinfo, &send_evt);
	}

	return proc_rc;
}

/****************************************************************************
 * Name          : cpd_evt_proc_cb_dump
 *
 * Description   : Function to dump the CPD Control Block
 *
 * Arguments     : CPND_CB *cb - CPND CB pointer
 *
 * Return Values : NCSCC_RC_SUCCESS/Error.
 *
 * Notes         : None.
 *****************************************************************************/
uns32 cpd_evt_proc_cb_dump(CPD_CB *cb)
{

	cpd_cb_dump();
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
 * Name          : cpd_evt_proc_timer_expiry
 *
 * Description   : Function to process the Timer expiry events at CPD.
 *
 * Arguments     : CPD_CB *cb - CPD CB pointer
 *                 CPSV_EVT *evt - Received Event structure
 *
 * Return Values : NCSCC_RC_SUCCESS/Error.
 *
 * Notes         : None.
 *****************************************************************************/
static uns32 cpd_evt_proc_timer_expiry(CPD_CB *cb, CPD_EVT *evt)
{
	uns32 rc;
	CPD_CPND_INFO_NODE *node_info = NULL;

	CPD_TMR_INFO *tmr_info = &evt->info.tmr_info;

	switch (tmr_info->type) {
	case CPD_TMR_TYPE_CPND_RETENTION:
		if (cb->ha_state == SA_AMF_HA_ACTIVE) {
			cpd_cpnd_info_node_get(&cb->cpnd_tree, &tmr_info->info.cpnd_dest, &node_info);
			if (node_info) {
				rc = cpd_process_cpnd_down(cb, &tmr_info->info.cpnd_dest);
			}
		}
		if (cb->ha_state == SA_AMF_HA_STANDBY) {
			cpd_cpnd_info_node_get(&cb->cpnd_tree, &tmr_info->info.cpnd_dest, &node_info);
			if (node_info) {
				node_info->timer_state = 2;
			}
		}
		break;
	default:
		break;
	}

	return NCSCC_RC_SUCCESS;

}

/****************************************************************************
 * Name           : cpnd_down_process
 *
 * Description    : This function is invoked when cpnd is down
 *
 * Arguments      : CPSV_MDS_INFO - mds_info , CPD_CPND_INFO_NODE - cpnd info
 *
 * Return Values  : Success / Error
 *
 * Notes:  1. Start the Retention timer
 *         2. If that ND contains Active Replica , then send RESTART event to other NDs
 *         3. If that ND does not contain Active Replica, send REP_DEL event to other NDs                
 *
 ***************************************************************************/
static uns32 cpnd_down_process(CPD_CB *cb, CPSV_MDS_INFO *mds_info, CPD_CPND_INFO_NODE *cpnd_info)
{
	CPD_CKPT_REF_INFO *cref_info;
	CPSV_EVT send_evt;
	uns32 proc_rc = NCSCC_RC_SUCCESS;

	cpnd_info->cpnd_ret_timer.type = CPD_TMR_TYPE_CPND_RETENTION;
	cpnd_info->cpnd_ret_timer.info.cpnd_dest = mds_info->dest;
	cpd_tmr_start(&cpnd_info->cpnd_ret_timer, CPD_CPND_DOWN_RETENTION_TIME);

	cref_info = cpnd_info->ckpt_ref_list;

	while (cref_info) {
		if (m_CPD_IS_LOCAL_NODE
		    (m_NCS_NODE_ID_FROM_MDS_DEST(cref_info->ckpt_node->active_dest),
		     m_NCS_NODE_ID_FROM_MDS_DEST(cpnd_info->cpnd_ret_timer.info.cpnd_dest))) {
			memset(&send_evt, 0, sizeof(CPSV_EVT));
			send_evt.type = CPSV_EVT_TYPE_CPND;
			send_evt.info.cpnd.type = CPSV_D2ND_RESTART;
			send_evt.info.cpnd.info.cpnd_restart.ckpt_id = cref_info->ckpt_node->ckpt_id;
			proc_rc = cpd_mds_bcast_send(cb, &send_evt, NCSMDS_SVC_ID_CPND);

			/* send this event to Standby also */
			/* cpd_a2s_ckpt_dest_down(cb,cref_info->ckpt_node,&mds_info->dest); */
			memset(&send_evt, 0, sizeof(CPSV_EVT));
			send_evt.type = CPSV_EVT_TYPE_CPA;
			send_evt.info.cpa.type = CPA_EVT_D2A_NDRESTART;
			send_evt.info.cpa.info.ackpt_info.ckpt_id = cref_info->ckpt_node->ckpt_id;
			proc_rc = cpd_mds_bcast_send(cb, &send_evt, NCSMDS_SVC_ID_CPA);
		} else {
			memset(&send_evt, 0, sizeof(CPSV_EVT));
			send_evt.type = CPSV_EVT_TYPE_CPND;
			send_evt.info.cpnd.type = CPND_EVT_D2ND_CKPT_REP_DEL;
			send_evt.info.cpnd.info.ckpt_del.ckpt_id = cref_info->ckpt_node->ckpt_id;
			send_evt.info.cpnd.info.ckpt_del.mds_dest = cpnd_info->cpnd_ret_timer.info.cpnd_dest;
			proc_rc = cpd_mds_bcast_send(cb, &send_evt, NCSMDS_SVC_ID_CPND);
			m_LOG_CPD_FFCL(CPD_REP_DEL_SUCCESS, CPD_FC_DB, NCSFL_SEV_INFO, cref_info->ckpt_node->ckpt_id,
				       cpnd_info->cpnd_ret_timer.info.cpnd_dest, __FILE__, __LINE__);
		}
		cref_info = cref_info->next;
	}
	return proc_rc;
}

/********************************************************************
 * Name         : cpd_cpnd_dest_replace
 *
 * Description  : To replace the old Dest info with the new Dest info when ND goes down
 *
 * Arguments    : CPD_CKPT_REF_INFO - ckpt reference info , CPSV_MDS_INFO - mds info
 *
 * Return Values: Success / Error
 *
********************************************************************************/
static uns32 cpd_cpnd_dest_replace(CPD_CB *cb, CPD_CKPT_REF_INFO *cref_info, CPSV_MDS_INFO *mds_info)
{
	CPD_CKPT_INFO_NODE *cp_node = NULL;
	CPD_NODE_REF_INFO *nref_info = NULL;
	uns32 rc = NCSCC_RC_SUCCESS;

	while (cref_info) {
		cp_node = cref_info->ckpt_node;
		if (m_CPND_NODE_ID_CMP
		    (m_NCS_NODE_ID_FROM_MDS_DEST(cp_node->active_dest), m_NCS_NODE_ID_FROM_MDS_DEST(mds_info->dest))) {
			cp_node->active_dest = mds_info->dest;
		}

		nref_info = cp_node->node_list;
		while (nref_info) {
			if (m_CPND_NODE_ID_CMP
			    (m_NCS_NODE_ID_FROM_MDS_DEST(nref_info->dest),
			     m_NCS_NODE_ID_FROM_MDS_DEST(mds_info->dest))) {
				nref_info->dest = mds_info->dest;
			}
			nref_info = nref_info->next;
		}
		cref_info = cref_info->next;
	}
	return rc;
}

/*****************************************************************************
 * Name          : cpnd_up_process
 *
 * Description   : This function is invoked when the ND is up
 * 
 * Arguments     : CPSV_MDS_INFO - mds info , CPD_CPND_INFO_NODE - cpnd info
 *
 * Return Values : Success / Error
 *
 * Notes :  1. Send the Restart Done event to other ND's if ND having Active replica is up
            2. Send a REP_ADD event if ND not having Active Replica is up
 *****************************************************************************/
static uns32 cpnd_up_process(CPD_CB *cb, CPSV_MDS_INFO *mds_info, CPD_CPND_INFO_NODE *cpnd_info)
{
	CPD_CKPT_REF_INFO *cref_info = NULL, *cpd_ckpt_info = NULL;
	CPSV_EVT send_evt;
	uns32 proc_rc = NCSCC_RC_SUCCESS;
	uns32 i = 0;

	cpnd_info->cpnd_ret_timer.type = CPD_TMR_TYPE_CPND_RETENTION;
	cpnd_info->cpnd_ret_timer.info.cpnd_dest = mds_info->dest;
	cpd_tmr_stop(&cpnd_info->cpnd_ret_timer);

	cref_info = cpnd_info->ckpt_ref_list;
	cpd_ckpt_info = cpnd_info->ckpt_ref_list;

	if (cref_info)
		cpd_cpnd_dest_replace(cb, cref_info, mds_info);

	while (cref_info) {
		memset(&send_evt, 0, sizeof(CPSV_EVT));
		/* If the Node contains Active Replica, then send RESTART_DONE event */
		if (m_CPD_IS_LOCAL_NODE
		    (m_NCS_NODE_ID_FROM_MDS_DEST(cref_info->ckpt_node->active_dest),
		     m_NCS_NODE_ID_FROM_MDS_DEST(cpnd_info->cpnd_ret_timer.info.cpnd_dest))) {
			send_evt.type = CPSV_EVT_TYPE_CPND;
			send_evt.info.cpnd.type = CPSV_D2ND_RESTART_DONE;
			send_evt.info.cpnd.info.cpnd_restart_done.ckpt_id = cref_info->ckpt_node->ckpt_id;
			send_evt.info.cpnd.info.cpnd_restart_done.mds_dest = cpnd_info->cpnd_ret_timer.info.cpnd_dest;
			send_evt.info.cpnd.info.cpnd_restart_done.dest_cnt = cref_info->ckpt_node->dest_cnt;
			send_evt.info.cpnd.info.cpnd_restart_done.active_dest = cref_info->ckpt_node->active_dest;
			send_evt.info.cpnd.info.cpnd_restart_done.attributes = cref_info->ckpt_node->attributes;
			send_evt.info.cpnd.info.cpnd_restart_done.ckpt_flags = cref_info->ckpt_node->ckpt_flags;

			if (send_evt.info.cpnd.info.cpnd_restart_done.dest_cnt) {
				CPD_NODE_REF_INFO *node_list = cref_info->ckpt_node->node_list;

				send_evt.info.cpnd.info.cpnd_restart_done.dest_list =
				    m_MMGR_ALLOC_CPSV_CPND_DEST_INFO(cref_info->ckpt_node->dest_cnt);

				if (!send_evt.info.cpnd.info.cpnd_restart_done.dest_list) {
					/* No memory, don't know what to do, setting dest_cnt to zero & continue */
					m_LOG_CPD_CL(CPD_CPND_DEST_INFO_ALLOC_FAILED, CPD_FC_MEMFAIL, NCSFL_SEV_ERROR,
						     __FILE__, __LINE__);
					send_evt.info.cpnd.info.cpnd_restart_done.dest_cnt = 0;
				}

				for (i = 0; i < send_evt.info.cpnd.info.cpnd_restart_done.dest_cnt; i++) {
					send_evt.info.cpnd.info.cpnd_restart_done.dest_list[i].dest = node_list->dest;
					node_list = node_list->next;
				}
			}

			proc_rc = cpd_mds_bcast_send(cb, &send_evt, NCSMDS_SVC_ID_CPND);

			if (send_evt.info.cpnd.info.cpnd_restart_done.dest_list) {
				m_MMGR_FREE_CPSV_CPND_DEST_INFO(send_evt.info.cpnd.info.cpnd_restart_done.dest_list);
				send_evt.info.cpnd.info.cpnd_restart_done.dest_list = NULL;
				send_evt.info.cpnd.info.cpnd_restart_done.dest_cnt = 0;
			}

			/*To broadcast the active MDS_DEST info of ckpt to all CPA's */
			memset(&send_evt, 0, sizeof(CPSV_EVT));
			send_evt.type = CPSV_EVT_TYPE_CPA;
			send_evt.info.cpa.type = CPA_EVT_D2A_ACT_CKPT_INFO_BCAST_SEND;
			send_evt.info.cpa.info.ackpt_info.ckpt_id = cref_info->ckpt_node->ckpt_id;
			send_evt.info.cpa.info.ackpt_info.mds_dest = cref_info->ckpt_node->active_dest;
			proc_rc = cpd_mds_bcast_send(cb, &send_evt, NCSMDS_SVC_ID_CPA);

		} else {
			/* else just send the ND which has the active replica */
			send_evt.type = CPSV_EVT_TYPE_CPND;
			send_evt.info.cpnd.type = CPND_EVT_D2ND_CKPT_REP_ADD;
			send_evt.info.cpnd.info.ckpt_add.ckpt_id = cref_info->ckpt_node->ckpt_id;
			send_evt.info.cpnd.info.ckpt_add.mds_dest = cpnd_info->cpnd_ret_timer.info.cpnd_dest;
			send_evt.info.cpnd.info.ckpt_add.dest_cnt = cref_info->ckpt_node->dest_cnt;
			send_evt.info.cpnd.info.ckpt_add.active_dest = cref_info->ckpt_node->active_dest;
			send_evt.info.cpnd.info.ckpt_add.attributes = cref_info->ckpt_node->attributes;
			send_evt.info.cpnd.info.ckpt_add.is_cpnd_restart = TRUE;
			send_evt.info.cpnd.info.ckpt_add.ckpt_flags = cref_info->ckpt_node->ckpt_flags;

			if (send_evt.info.cpnd.info.ckpt_add.dest_cnt) {
				CPD_NODE_REF_INFO *node_list = cref_info->ckpt_node->node_list;

				send_evt.info.cpnd.info.ckpt_add.dest_list =
				    m_MMGR_ALLOC_CPSV_CPND_DEST_INFO(cref_info->ckpt_node->dest_cnt);

				if (!send_evt.info.cpnd.info.ckpt_add.dest_list) {
					/* No memory, don't know what to do, setting dest_cnt to zero & continue */
					m_LOG_CPD_CL(CPD_CPND_DEST_INFO_ALLOC_FAILED, CPD_FC_MEMFAIL, NCSFL_SEV_ERROR,
						     __FILE__, __LINE__);
					send_evt.info.cpnd.info.ckpt_add.dest_cnt = 0;
				}

				for (i = 0; i < send_evt.info.cpnd.info.ckpt_add.dest_cnt; i++) {
					send_evt.info.cpnd.info.ckpt_add.dest_list[i].dest = node_list->dest;
					node_list = node_list->next;
				}
			}

			proc_rc = cpd_mds_bcast_send(cb, &send_evt, NCSMDS_SVC_ID_CPND);
			m_LOG_CPD_FFCL(CPD_REP_ADD_SUCCESS, CPD_FC_DB, NCSFL_SEV_INFO, cref_info->ckpt_node->ckpt_id,
				       cpnd_info->cpnd_ret_timer.info.cpnd_dest, __FILE__, __LINE__);

			if (send_evt.info.cpnd.info.ckpt_add.dest_list) {
				m_MMGR_FREE_CPSV_CPND_DEST_INFO(send_evt.info.cpnd.info.ckpt_add.dest_list);
				send_evt.info.cpnd.info.ckpt_add.dest_list = NULL;
				send_evt.info.cpnd.info.ckpt_add.dest_cnt = 0;
			}
		}
		cref_info = cref_info->next;
	}

	return proc_rc;
}

static uns32 cpd_evt_mds_quiesced_ack_rsp(CPD_CB *cb, CPD_EVT *evt, CPSV_SEND_INFO *sinfo)
{
	uns32 rc = NCSCC_RC_SUCCESS;
	SaAisErrorT saErr = SA_AIS_OK;
	cpd_mbcsv_chgrole(cb);
	saAmfResponse(cb->amf_hdl, cb->amf_invocation, saErr);
	return rc;
}

/****************************************************************************
 * Name          : cpd_evt_proc_mds_evt
 *
 * Description   : Function to process the Events received from MDS
 *
 * Arguments     : CPD_CB *cb - CPD CB pointer
 *                 CPSV_EVT *evt - Received Event structure
 *
 * Return Values : NCSCC_RC_SUCCESS/Error.
 *
 * Notes         : None.
 *****************************************************************************/
static uns32 cpd_evt_proc_mds_evt(CPD_CB *cb, CPD_EVT *evt)
{
	CPSV_MDS_INFO *mds_info;
	CPD_CPND_INFO_NODE *node_info = NULL;
	CPD_CKPT_INFO_NODE *ckpt_node = NULL;
	CPD_NODE_REF_INFO *nref_info;
	CPD_CKPT_REF_INFO *cref_info = NULL;
	CPD_CKPT_MAP_INFO *map_info = NULL;
	SaCkptCheckpointHandleT prev_ckpt_hdl;
	NCS_BOOL flag = FALSE;
	SaNameT ckpt_name;
	uns32 phy_slot_sub_slot;
	NCS_BOOL add_flag = TRUE;

	memset(&ckpt_name, 0, sizeof(SaNameT));
	mds_info = &evt->info.mds_info;

	memset(&phy_slot_sub_slot, 0, sizeof(uns32));

	switch (mds_info->change) {

	case NCSMDS_RED_UP:
		/* get the peer mds_red_up */
		/* get the mds_dest of remote CPND */
		if (cb->node_id != mds_info->node_id) {
			MDS_DEST tmpDest = 0LL;
			phy_slot_sub_slot = cpd_get_slot_sub_slot_id_from_node_id(mds_info->node_id);
			cb->cpd_remote_id = phy_slot_sub_slot;

			if (cb->is_rem_cpnd_up == FALSE) {
				cpd_cpnd_info_node_getnext(&cb->cpnd_tree, &tmpDest, &node_info);
				while (node_info) {	/* while-1 */
					if (mds_info->node_id == node_info->cpnd_key) {
						cb->is_rem_cpnd_up = TRUE;
						cb->rem_cpnd_dest = node_info->cpnd_dest;
						/* Break out of while-1. We found */
							if (cb->ha_state == SA_AMF_HA_ACTIVE) {
								cpd_ckpt_node_getnext(&cb->ckpt_tree, NULL, &ckpt_node);
								while (ckpt_node) {
									prev_ckpt_hdl = ckpt_node->ckpt_id;
									if (!m_IS_SA_CKPT_CHECKPOINT_COLLOCATED(&ckpt_node->attributes)) {
										nref_info = ckpt_node->node_list;
										while (nref_info) {
											if (m_CPD_IS_LOCAL_NODE
													(cb->cpd_remote_id,
													 cpd_get_slot_sub_id_from_mds_dest
													 (nref_info->dest))) {
												flag = TRUE;
											}
											nref_info = nref_info->next;
										}
										if (flag == FALSE) {
											cpd_ckpt_map_node_get(&cb->ckpt_map_tree,
													&ckpt_node->ckpt_name, &map_info);
											if (map_info) {
												cpd_noncolloc_ckpt_rep_create(cb,
														&cb->
														rem_cpnd_dest,
														ckpt_node,
														map_info);
											}
										}
									}
									cpd_ckpt_node_getnext(&cb->ckpt_tree, &prev_ckpt_hdl, &ckpt_node);
								}
							}
						break;
					}
					tmpDest = node_info->cpnd_dest;
					cpd_cpnd_info_node_getnext(&cb->cpnd_tree, &tmpDest, &node_info);
				}
			}
		} else {
			/* Don't handle RED_UP from our own node */
			break;
		}

		break;

	case NCSMDS_UP:

		if (mds_info->svc_id == NCSMDS_SVC_ID_CPD) {
			if (mds_info->change == NCSMDS_UP)
				return NCSCC_RC_SUCCESS;
		}

		if (mds_info->svc_id == NCSMDS_SVC_ID_CPND) {
			/* Save MDS address for this CPND */
			phy_slot_sub_slot = cpd_get_slot_sub_id_from_mds_dest(mds_info->dest);
			if (cb->cpd_remote_id == phy_slot_sub_slot) {
				cb->rem_cpnd_dest = mds_info->dest;
			}

			if (m_CPND_IS_ON_SCXB(cb->cpd_self_id, cpd_get_slot_sub_id_from_mds_dest(mds_info->dest))) {
				cb->is_loc_cpnd_up = TRUE;
				cb->loc_cpnd_dest = mds_info->dest;

				if (cb->ha_state == SA_AMF_HA_ACTIVE) {
					cpd_ckpt_node_getnext(&cb->ckpt_tree, NULL, &ckpt_node);
					while (ckpt_node) {
						prev_ckpt_hdl = ckpt_node->ckpt_id;
						if (!m_IS_SA_CKPT_CHECKPOINT_COLLOCATED(&ckpt_node->attributes)) {
							nref_info = ckpt_node->node_list;
							while (nref_info) {
								if (m_CPD_IS_LOCAL_NODE
								    (cb->cpd_self_id,
								     cpd_get_slot_sub_id_from_mds_dest
								     (nref_info->dest))) {
									flag = TRUE;
								}
								nref_info = nref_info->next;
							}
							if (flag == FALSE) {
								cpd_ckpt_map_node_get(&cb->ckpt_map_tree,
										      &ckpt_node->ckpt_name, &map_info);
								if (map_info) {
									cpd_noncolloc_ckpt_rep_create(cb,
												      &cb->
												      loc_cpnd_dest,
												      ckpt_node,
												      map_info);
								}
							}
						}

						cpd_ckpt_node_getnext(&cb->ckpt_tree, &prev_ckpt_hdl, &ckpt_node);
					}
				}
			}
			/* When CPND ON STANDBY COMES UP */
			if (m_CPND_IS_ON_SCXB(cb->cpd_remote_id, cpd_get_slot_sub_id_from_mds_dest(mds_info->dest))) {
				cb->is_rem_cpnd_up = TRUE;
				cb->rem_cpnd_dest = mds_info->dest;

				if (cb->ha_state == SA_AMF_HA_ACTIVE) {
					cpd_ckpt_node_getnext(&cb->ckpt_tree, NULL, &ckpt_node);
					while (ckpt_node) {
						prev_ckpt_hdl = ckpt_node->ckpt_id;
						if (!m_IS_SA_CKPT_CHECKPOINT_COLLOCATED(&ckpt_node->attributes)) {
							nref_info = ckpt_node->node_list;
							while (nref_info) {
								if (m_CPD_IS_LOCAL_NODE
								    (cb->cpd_remote_id,
								     cpd_get_slot_sub_id_from_mds_dest
								     (nref_info->dest))) {
									flag = TRUE;
								}
								nref_info = nref_info->next;
							}
							if (flag == FALSE) {
								cpd_ckpt_map_node_get(&cb->ckpt_map_tree,
										      &ckpt_node->ckpt_name, &map_info);
								if (map_info) {
									cpd_noncolloc_ckpt_rep_create(cb,
												      &cb->
												      rem_cpnd_dest,
												      ckpt_node,
												      map_info);

								}
							}
						}

						cpd_ckpt_node_getnext(&cb->ckpt_tree, &prev_ckpt_hdl, &ckpt_node);
					}
				}
			}

			if (cb->ha_state == SA_AMF_HA_ACTIVE) {
				cpd_cpnd_info_node_get(&cb->cpnd_tree, &mds_info->dest, &node_info);
				if (!node_info) {
					cpd_cpnd_info_node_find_add(&cb->cpnd_tree, &mds_info->dest, &node_info, &add_flag);
					/* No Checkpoints on this node, */
					return NCSCC_RC_SUCCESS;
				} else {
					cpnd_up_process(cb, mds_info, node_info);
				}
			}

			if (cb->ha_state == SA_AMF_HA_STANDBY) {
				cpd_cpnd_info_node_get(&cb->cpnd_tree, &mds_info->dest, &node_info);
				if (!node_info) {
					cpd_cpnd_info_node_find_add(&cb->cpnd_tree, &mds_info->dest, &node_info, &add_flag);
					/* No Checkpoints on this node, */
					return NCSCC_RC_SUCCESS;
				} else {
					node_info->cpnd_ret_timer.type = CPD_TMR_TYPE_CPND_RETENTION;
					node_info->cpnd_ret_timer.info.cpnd_dest = mds_info->dest;
					cpd_tmr_stop(&node_info->cpnd_ret_timer);
					node_info->timer_state = 1;

					cref_info = node_info->ckpt_ref_list;

					if (cref_info)
						cpd_cpnd_dest_replace(cb, cref_info, mds_info);
				}
			}
		}
		break;
	case NCSMDS_DOWN:

		if (mds_info->svc_id == NCSMDS_SVC_ID_CPD) {
			if (mds_info->change == NCSMDS_DOWN)
				return NCSCC_RC_SUCCESS;
		}

		if (mds_info->svc_id == NCSMDS_SVC_ID_CPND) {
			if (m_CPND_IS_ON_SCXB(cb->cpd_self_id, cpd_get_slot_sub_id_from_mds_dest(mds_info->dest))) {
				cb->is_loc_cpnd_up = FALSE;
			}

			if (m_CPND_IS_ON_SCXB(cb->cpd_remote_id, cpd_get_slot_sub_id_from_mds_dest(mds_info->dest))) {
				cb->is_rem_cpnd_up = FALSE;
			}

			/* MDS address for this CPND is no longer valid */
			phy_slot_sub_slot = cpd_get_slot_sub_id_from_mds_dest(mds_info->dest);

			if (cb->ha_state == SA_AMF_HA_ACTIVE) {
				cpd_cpnd_info_node_get(&cb->cpnd_tree, &mds_info->dest, &node_info);
				if (!node_info) {
					/* No Checkpoints on this node, */
					return NCSCC_RC_SUCCESS;
				} else if (node_info && (node_info->ckpt_ref_list == NULL)) {
					cpd_cpnd_info_node_delete(cb, node_info);
					return NCSCC_RC_SUCCESS;
				} else
					cpnd_down_process(cb, mds_info, node_info);
			}
			if (cb->ha_state == SA_AMF_HA_STANDBY) {
				cpd_cpnd_info_node_get(&cb->cpnd_tree, &mds_info->dest, &node_info);
				if (!node_info) {
					/* No Checkpoints on this node, */
					return NCSCC_RC_SUCCESS;
				}
				else if (node_info && (node_info->ckpt_ref_list == NULL)) {
					cpd_cpnd_info_node_delete(cb, node_info);
					return NCSCC_RC_SUCCESS;
				} else {
					node_info->timer_state = 1;
					node_info->cpnd_ret_timer.type = CPD_TMR_TYPE_CPND_RETENTION;
					node_info->cpnd_ret_timer.info.cpnd_dest = mds_info->dest;
					cpd_tmr_start(&node_info->cpnd_ret_timer, CPD_CPND_DOWN_RETENTION_TIME);
				}
			}
		}
		break;
	default:

		/* RSR:TBD Log */
		break;
	}

	return NCSCC_RC_SUCCESS;
}
