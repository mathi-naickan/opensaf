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
#include "immutil.h"

uint32_t cpd_evt_proc_cb_dump(CPD_CB *cb);
static uint32_t cpd_evt_proc_ckpt_create(CPD_CB *cb, CPD_EVT *evt, CPSV_SEND_INFO *sinfo);
static uint32_t cpd_evt_proc_ckpt_usr_info(CPD_CB *cb, CPD_EVT *evt, CPSV_SEND_INFO *sinfo);
/*static uint32_t cpd_evt_proc_ckpt_sync_info(CPD_CB *cb,CPD_EVT *evt,CPSV_SEND_INFO *sinfo);*/
static uint32_t cpd_evt_proc_ckpt_sec_info_upd(CPD_CB *cb, CPD_EVT *evt, CPSV_SEND_INFO *sinfo);
static uint32_t cpd_evt_proc_ckpt_unlink(CPD_CB *cb, CPD_EVT *evt, CPSV_SEND_INFO *sinfo);
static uint32_t cpd_evt_proc_ckpt_rdset(CPD_CB *cb, CPD_EVT *evt, CPSV_SEND_INFO *sinfo);

static uint32_t cpd_evt_proc_active_set(CPD_CB *cb, CPD_EVT *evt, CPSV_SEND_INFO *sinfo);

static uint32_t cpd_evt_proc_ckpt_destroy(CPD_CB *cb, CPD_EVT *evt, CPSV_SEND_INFO *sinfo);
static uint32_t cpd_evt_proc_ckpt_destroy_byname(CPD_CB *cb, CPD_EVT *evt, CPSV_SEND_INFO *sinfo);
static uint32_t cpd_evt_proc_timer_expiry(CPD_CB *cb, CPD_EVT *evt);
static uint32_t cpd_evt_proc_mds_evt(CPD_CB *cb, CPD_EVT *evt);

static uint32_t cpd_evt_mds_quiesced_ack_rsp(CPD_CB *cb, CPD_EVT *evt, CPSV_SEND_INFO *sinfo);

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
	uint32_t cb_hdl = m_CPD_GET_CB_HDL;

	TRACE_ENTER();
	if (evt->type != CPSV_EVT_TYPE_CPD) {
		/*TBD Log the error BAD EVENT */
		return;
	}

	/* Get the CB from the handle */
	cb = ncshm_take_hdl(NCS_SERVICE_ID_CPD, cb_hdl);

	if (cb == NULL) {
		LOG_ER("cpd cb take hdl failed ");
		TRACE_LEAVE();
		return;
	}
#if ( CPSV_DEBUG == 1)
	TRACE("%s", cpd_evt_str[evt->info.cpd.type]);
#endif

	switch (evt->info.cpd.type) {
	case CPD_EVT_MDS_INFO:
		(void)cpd_evt_proc_mds_evt(cb, &evt->info.cpd);
		break;
	case CPD_EVT_TIME_OUT:
		(void)cpd_evt_proc_timer_expiry(cb, &evt->info.cpd);
		break;
	case CPD_EVT_ND2D_CKPT_CREATE:
		(void)cpd_evt_proc_ckpt_create(cb, &evt->info.cpd, &evt->sinfo);
		break;
	case CPD_EVT_ND2D_CKPT_USR_INFO:
		(void)cpd_evt_proc_ckpt_usr_info(cb, &evt->info.cpd, &evt->sinfo);
		break;
	case CPD_EVT_ND2D_CKPT_SEC_INFO_UPD:
		(void)cpd_evt_proc_ckpt_sec_info_upd(cb, &evt->info.cpd, &evt->sinfo);
		break;
	case CPD_EVT_ND2D_CKPT_UNLINK:
		(void)cpd_evt_proc_ckpt_unlink(cb, &evt->info.cpd, &evt->sinfo);
		break;
	case CPD_EVT_ND2D_CKPT_RDSET:
		(void)cpd_evt_proc_ckpt_rdset(cb, &evt->info.cpd, &evt->sinfo);
		break;
	case CPD_EVT_ND2D_ACTIVE_SET:
		(void)cpd_evt_proc_active_set(cb, &evt->info.cpd, &evt->sinfo);
		break;
	case CPD_EVT_ND2D_CKPT_DESTROY:
		(void)cpd_evt_proc_ckpt_destroy(cb, &evt->info.cpd, &evt->sinfo);
		break;
	case CPD_EVT_ND2D_CKPT_DESTROY_BYNAME:
		(void)cpd_evt_proc_ckpt_destroy_byname(cb, &evt->info.cpd, &evt->sinfo);
		break;
	case CPD_EVT_MDS_QUIESCED_ACK_RSP:
		(void)cpd_evt_mds_quiesced_ack_rsp(cb, &evt->info.cpd, &evt->sinfo);
		break;

	case CPD_EVT_CB_DUMP:
		(void)cpd_evt_proc_cb_dump(cb);
		break;
	default:
		/* Log the error TBD */
		break;
	}

	/* Return the Handle */
	ncshm_give_hdl(cb_hdl);

	/* Free the Event */
	m_MMGR_FREE_CPSV_EVT(evt, NCS_SERVICE_ID_CPD);
	TRACE_LEAVE();
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
static uint32_t cpd_evt_proc_ckpt_create(CPD_CB *cb, CPD_EVT *evt, CPSV_SEND_INFO *sinfo)
{
	CPSV_EVT send_evt;
	SaAisErrorT rc = SA_AIS_OK;
	uint32_t proc_rc = NCSCC_RC_SUCCESS;
	CPD_CKPT_INFO_NODE *ckpt_node = 0;
	CPD_CKPT_MAP_INFO *map_info = NULL;
	CPSV_ND2D_CKPT_CREATE *ckpt_create = &evt->info.ckpt_create;
	bool is_first_rep = false, is_new_noncol = false;
	
	TRACE_ENTER();
	cpd_ckpt_map_node_get(&cb->ckpt_map_tree, &ckpt_create->ckpt_name, &map_info);
	if (map_info) {

		/*   ckpt_create->ckpt_name.length = m_NCS_OS_NTOHS(ckpt_create->ckpt_name.length);  */
		if (m_CPA_VER_IS_ABOVE_B_1_1(&ckpt_create->client_version)) {
			if ((ckpt_create->ckpt_flags & SA_CKPT_CHECKPOINT_CREATE) &&
			    (!m_COMPARE_CREATE_ATTR(&ckpt_create->attributes, &map_info->attributes))) {
				TRACE_4("cpd ckpt create failure ckpt name,dest :%s,%"PRIu64,ckpt_create->ckpt_name.value, sinfo->dest);
				rc = SA_AIS_ERR_EXIST;
				goto send_rsp;
			}
		} else {
			if ((ckpt_create->ckpt_flags & SA_CKPT_CHECKPOINT_CREATE) &&
			    (!m_COMPARE_CREATE_ATTR_B_1_1(&ckpt_create->attributes, &map_info->attributes))) {
				TRACE_4("cpd ckpt create failure ckpt name,dest :  %s, %"PRIu64,ckpt_create->ckpt_name.value, sinfo->dest);
				rc = SA_AIS_ERR_EXIST;
				goto send_rsp;
			}
		}
	} else {
		SaCkptCheckpointCreationAttributesT ckpt_local_attrib;
		memset(&ckpt_local_attrib, 0, sizeof(SaCkptCheckpointCreationAttributesT));
		/* ckpt_create->ckpt_name.length = m_NCS_OS_NTOHS(ckpt_create->ckpt_name.length); */
		is_first_rep = true;
		if (m_CPA_VER_IS_ABOVE_B_1_1(&ckpt_create->client_version)) {
			if (!(ckpt_create->ckpt_flags & SA_CKPT_CHECKPOINT_CREATE) &&
			    (m_COMPARE_CREATE_ATTR(&ckpt_create->attributes, &ckpt_local_attrib))) {

				TRACE_4("cpd ckpt create failure ckpt name,dest :  %s, %"PRIu64,ckpt_create->ckpt_name.value, sinfo->dest);
				rc = SA_AIS_ERR_NOT_EXIST;
				goto send_rsp;
			}
		} else {
			if (!(ckpt_create->ckpt_flags & SA_CKPT_CHECKPOINT_CREATE) &&
			    (m_COMPARE_CREATE_ATTR_B_1_1(&ckpt_create->attributes, &ckpt_local_attrib))) {

				TRACE_4("cpd ckpt create failure ckpt name,dest :  %s, %"PRIu64,ckpt_create->ckpt_name.value, sinfo->dest);
				rc = SA_AIS_ERR_NOT_EXIST;
				goto send_rsp;
			}
		}
	}

	/* Add/Update the entries in ckpt DB, ckpt_map DB, ckpt_node DB */
	proc_rc = cpd_ckpt_db_entry_update(cb, &sinfo->dest, ckpt_create, &ckpt_node, &map_info);
	if (proc_rc == NCSCC_RC_OUT_OF_MEM) {
		TRACE_4("cpd ckpt create failure ckpt name,dest :  %s, %"PRIu64,ckpt_create->ckpt_name.value, sinfo->dest);
		rc = SA_AIS_ERR_NO_MEMORY;
		goto send_rsp;
	} else if (proc_rc != NCSCC_RC_SUCCESS) {

		TRACE_4("cpd ckpt create failure ckpt name,dest :  %s, %"PRIu64,ckpt_create->ckpt_name.value, sinfo->dest);
		rc = SA_AIS_ERR_LIBRARY;
		goto send_rsp;
	}

	if (ckpt_create->ckpt_flags & SA_CKPT_CHECKPOINT_READ)
		ckpt_node->num_readers++;
	if (ckpt_create->ckpt_flags & SA_CKPT_CHECKPOINT_WRITE)
		ckpt_node->num_writers++;

	ckpt_node->num_users++;

	cb->is_db_upd = true;

	/* Redundancy A2S   This is for async update   */
	/* Only for 1st  replica we send the entire info , for later openings we send dest_add */
	if (is_first_rep) {
		cpd_a2s_ckpt_create(cb, ckpt_node);
		cpd_a2s_ckpt_usr_info(cb, ckpt_node);
	}

	/* Send the dest info to the Standby SCXB This is for async update */
	if (is_first_rep == false) {
		cpd_a2s_ckpt_dest_add(cb, ckpt_node, &sinfo->dest);
		cpd_a2s_ckpt_usr_info(cb, ckpt_node);
	}
	/* Non-colocated processing */
	if ((is_first_rep == true) && (!(map_info->attributes.creationFlags & SA_CKPT_CHECKPOINT_COLLOCATED))) {
		/* Policy is to create the replic on both active & standby SCXB's CPND 
		   Right now replica is created only on the active (local) SCXB */
		/* if(cb->is_loc_cpnd_up && (cpd_get_slot_sub_id_from_mds_dest(sinfo->dest) != ckpt_node->ckpt_on_scxb1)) */
		if (cb->is_loc_cpnd_up
		    && (!m_CPND_IS_ON_SCXB(cb->cpd_self_id, cpd_get_slot_sub_id_from_mds_dest(sinfo->dest)))) {
			proc_rc = cpd_noncolloc_ckpt_rep_create(cb, &cb->loc_cpnd_dest, ckpt_node, map_info);
			if (proc_rc == NCSCC_RC_SUCCESS)
				TRACE_1("cpd non coloc ckpt create success ckpt name %s, loc_cpnd_dest:%"PRIu64,ckpt_create->ckpt_name.value, cb->loc_cpnd_dest);
			else
				TRACE_2("cpd non coloc ckpt create failure ckpt name %s, loc_cpnd_dest:%"PRIu64,ckpt_create->ckpt_name.value, cb->loc_cpnd_dest);

		}
		/*  if(cb->is_rem_cpnd_up && (cpd_get_slot_sub_id_from_mds_dest(sinfo->dest) != ckpt_node->ckpt_on_scxb2)) */
		if (cb->is_rem_cpnd_up
		    && (!m_CPND_IS_ON_SCXB(cb->cpd_remote_id, cpd_get_slot_sub_id_from_mds_dest(sinfo->dest)))) {
			proc_rc = cpd_noncolloc_ckpt_rep_create(cb, &cb->rem_cpnd_dest, ckpt_node, map_info);
			if (proc_rc == NCSCC_RC_SUCCESS)
				TRACE_1("cpd non coloc ckpt create success ckpt_name %s and rem_cpnd %"PRIu64,ckpt_create->ckpt_name.value,cb->rem_cpnd_dest);
			else
				TRACE_4("cpd non coloc ckpt create failure ckpt_name %s and rem_cpnd %"PRIu64,ckpt_create->ckpt_name.value,cb->rem_cpnd_dest);
		}
		/* ND on SCXB has created the same checkpoint, so is_new_noncol must be made to true */
		is_new_noncol = true;

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
			send_evt.info.cpnd.info.ckpt_info.ckpt_rep_create = true;
		else {
			if (is_first_rep)
				send_evt.info.cpnd.info.ckpt_info.ckpt_rep_create = true;
			else
				send_evt.info.cpnd.info.ckpt_info.ckpt_rep_create = false;
		}

		if (ckpt_node->dest_cnt) {
			CPD_NODE_REF_INFO *node_list = ckpt_node->node_list;
			uint32_t i = 0;

			send_evt.info.cpnd.info.ckpt_info.dest_cnt = ckpt_node->dest_cnt;
			send_evt.info.cpnd.info.ckpt_info.dest_list =
			    m_MMGR_ALLOC_CPSV_CPND_DEST_INFO(ckpt_node->dest_cnt);
			if (send_evt.info.cpnd.info.ckpt_info.dest_list == NULL) {
				send_evt.info.cpnd.info.ckpt_info.error = SA_AIS_ERR_NO_MEMORY;
				rc = SA_AIS_ERR_NO_MEMORY;
				LOG_CR("cpd cpnd dest info memory alloc failed");
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
		TRACE_4("cpd ckpt create failure for ckpt_name :%s,dest :%"PRIu64,ckpt_create->ckpt_name.value,sinfo->dest);

	if ((proc_rc != NCSCC_RC_SUCCESS) || (rc != SA_AIS_OK))
		return proc_rc;

	/* Ckpt info successfully updated at CPD, send it to all CPNDs */
	/* Broadcast the ckpt add info to all the CPNDs, only the relevent CPNDs 
	   will process this message */
	if ((is_first_rep == false) || (is_new_noncol == true)) {
		memset(&send_evt, 0, sizeof(CPSV_EVT));
		send_evt.type = CPSV_EVT_TYPE_CPND;
		send_evt.info.cpnd.type = CPND_EVT_D2ND_CKPT_REP_ADD;
		send_evt.info.cpnd.info.ckpt_add.ckpt_id = ckpt_node->ckpt_id;
		send_evt.info.cpnd.info.ckpt_add.mds_dest = sinfo->dest;
		send_evt.info.cpnd.info.ckpt_add.is_cpnd_restart = false;

		proc_rc = cpd_mds_bcast_send(cb, &send_evt, NCSMDS_SVC_ID_CPND);
		TRACE_2("cpd rep add success for ckpt_id:%llx,dest :%"PRIu64,map_info->ckpt_id,sinfo->dest);
	}
	if (is_first_rep)
		TRACE_2("cpd ckpt create success for first replica ckpt_id:%llx,dest :%"PRIu64,map_info->ckpt_id,sinfo->dest);
	else
		TRACE_2("cpd ckpt create success ckpt_id:%llx,dest :%"PRIu64,map_info->ckpt_id,sinfo->dest);

	TRACE_LEAVE();
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
static uint32_t cpd_evt_proc_ckpt_usr_info(CPD_CB *cb, CPD_EVT *evt, CPSV_SEND_INFO *sinfo)
{
	CPD_CKPT_INFO_NODE *ckpt_node;
	CPSV_EVT send_evt;
	uint32_t rc = NCSCC_RC_SUCCESS;

	TRACE_ENTER();
	rc = cpd_ckpt_node_get(&cb->ckpt_tree, &evt->info.ckpt_usr_info.ckpt_id, &ckpt_node);
	if (ckpt_node == 0) {
		TRACE_4("cpd ckpt info node get failed for ckpt_id:%llx,dest:%"PRIu64,evt->info.ckpt_usr_info.ckpt_id, sinfo->dest);
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
			TRACE_2("cpd ckpt rdset success for ckpt_id:%llx,active_dest:%"PRIu64,ckpt_node->ckpt_id,ckpt_node->active_dest);
			if (m_CPND_IS_ON_SCXB(cb->cpd_self_id, cpd_get_slot_sub_id_from_mds_dest(sinfo->dest))) {

				if (evt->info.ckpt_usr_info.info_type == CPSV_USR_INFO_CKPT_OPEN_FIRST) {
					if (!ckpt_node->ckpt_on_scxb1)
						ckpt_node->ckpt_on_scxb1 =
						    (uint32_t)cpd_get_slot_sub_id_from_mds_dest(sinfo->dest);
					else
						ckpt_node->ckpt_on_scxb2 =
						    (uint32_t)cpd_get_slot_sub_id_from_mds_dest(sinfo->dest);

				}
			}
			if (m_CPND_IS_ON_SCXB(cb->cpd_remote_id, cpd_get_slot_sub_id_from_mds_dest(sinfo->dest))) {
				if (evt->info.ckpt_usr_info.info_type == CPSV_USR_INFO_CKPT_OPEN_FIRST) {
					if (!ckpt_node->ckpt_on_scxb1)
						ckpt_node->ckpt_on_scxb1 =
						    (uint32_t)cpd_get_slot_sub_id_from_mds_dest(sinfo->dest);
					else
						ckpt_node->ckpt_on_scxb2 =
						    (uint32_t)cpd_get_slot_sub_id_from_mds_dest(sinfo->dest);
				}
			}
		}

		ckpt_node->num_users++;

		if (evt->info.ckpt_usr_info.ckpt_flags & SA_CKPT_CHECKPOINT_READ)
			ckpt_node->num_readers++;
		if (evt->info.ckpt_usr_info.ckpt_flags & SA_CKPT_CHECKPOINT_WRITE)
			ckpt_node->num_writers++;
	}
	else if (evt->info.ckpt_usr_info.info_type == CPSV_USR_INFO_CKPT_OPEN) {

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
			TRACE_2("cpd ckpt rdset success for ckpt_id:%llx,active_dest:%"PRIu64,ckpt_node->ckpt_id,ckpt_node->active_dest);
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
	else if (evt->info.ckpt_usr_info.info_type == CPSV_USR_INFO_CKPT_CLOSE) {

		ckpt_node->num_users--;

		if (evt->info.ckpt_usr_info.ckpt_flags & SA_CKPT_CHECKPOINT_READ)
			ckpt_node->num_readers--;
		if (evt->info.ckpt_usr_info.ckpt_flags & SA_CKPT_CHECKPOINT_WRITE)
			ckpt_node->num_writers--;

	}

	cpd_a2s_ckpt_usr_info(cb, ckpt_node);
	TRACE_LEAVE();
	return rc;
}

/***************************************************************************
 * Name         : cpd_evt_proc_ckpt_sec_info_upd
 *
 * Description  : To get the number of sections 
 **************************************************************************/
uint32_t cpd_evt_proc_ckpt_sec_info_upd(CPD_CB *cb, CPD_EVT *evt, CPSV_SEND_INFO *sinfo)
{
	CPD_CKPT_INFO_NODE *ckpt_node;
	uint32_t rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER();
	cpd_ckpt_node_get(&cb->ckpt_tree, &evt->info.ckpt_sec_info.ckpt_id, &ckpt_node);
	if (ckpt_node == 0) {
		TRACE_LEAVE();
		return NCSCC_RC_FAILURE;
	}

	if (evt->info.ckpt_sec_info.info_type == CPSV_CKPT_SEC_INFO_CREATE) {
		ckpt_node->num_sections++;
	} else if (evt->info.ckpt_sec_info.info_type == CPSV_CKPT_SEC_INFO_DELETE) {
		ckpt_node->num_sections--;
	}

	cpd_a2s_ckpt_usr_info(cb, ckpt_node);

	TRACE_LEAVE();
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
static uint32_t cpd_evt_proc_ckpt_unlink(CPD_CB *cb, CPD_EVT *evt, CPSV_SEND_INFO *sinfo)
{
	CPD_CKPT_INFO_NODE *ckpt_node = NULL;
	CPD_CKPT_MAP_INFO *map_info = NULL;
	SaNameT *ckpt_name = &evt->info.ckpt_ulink.ckpt_name;
	SaAisErrorT rc = SA_AIS_OK;
	SaAisErrorT proc_rc = SA_AIS_OK;
	CPSV_EVT send_evt;

	TRACE_ENTER();
	rc = cpd_proc_unlink_set(cb, &ckpt_node, map_info, ckpt_name);
	if (rc != SA_AIS_OK)
		goto send_rsp;

	/* Redundancy  A2S */
	cpd_a2s_ckpt_unlink_set(cb, ckpt_node);

 send_rsp:

	if (rc == SA_AIS_OK) {
		/* Broadcast the Unlink info to all CPNDs */
		memset(&send_evt, 0, sizeof(CPSV_EVT));
		send_evt.type = CPSV_EVT_TYPE_CPND;
		send_evt.info.cpnd.type = CPND_EVT_D2ND_CKPT_UNLINK;
		send_evt.info.cpnd.info.ckpt_ulink.ckpt_id = ckpt_node->ckpt_id;

		proc_rc = cpd_mds_bcast_send(cb, &send_evt, NCSMDS_SVC_ID_CPND);
		TRACE_2("cpd evt unlink success for ckpt_name:%s,dest :%"PRIu64,evt->info.ckpt_ulink.ckpt_name.value, sinfo->dest);

		/* delete imm ckpt runtime object */
		if (cb->ha_state == SA_AMF_HA_ACTIVE) {
			if (immutil_saImmOiRtObjectDelete(cb->immOiHandle, &ckpt_node->ckpt_name) != SA_AIS_OK) {
				TRACE_4("Deleting run time object %s failed",ckpt_node->ckpt_name.value);
				/* Free the Client Node */
			}
		}
	}
	   
	memset(&send_evt, 0, sizeof(CPSV_EVT));
	send_evt.type = CPSV_EVT_TYPE_CPND;
	send_evt.info.cpnd.type = CPND_EVT_D2ND_CKPT_UNLINK_ACK;
	send_evt.info.cpnd.info.ulink_ack.error = rc;
	proc_rc = cpd_mds_send_rsp(cb, sinfo, &send_evt);
	
	TRACE_LEAVE2("Ret val %d",proc_rc);
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
static uint32_t cpd_evt_proc_ckpt_rdset(CPD_CB *cb, CPD_EVT *evt, CPSV_SEND_INFO *sinfo)
{
	CPD_CKPT_INFO_NODE *ckpt_node = NULL;
	SaAisErrorT rc = SA_AIS_OK;
	CPSV_EVT send_evt;
	uint32_t proc_rc = NCSCC_RC_SUCCESS;

	TRACE_ENTER();
	rc = cpd_proc_retention_set(cb, evt->info.rd_set.ckpt_id, evt->info.rd_set.reten_time, &ckpt_node);
	if (rc != SA_AIS_OK)
		goto send_rsp;

	/*  REDUNDANCY  A2S  */
	cpd_a2s_ckpt_rdset(cb, ckpt_node);

	TRACE_2("cpd ckpt rdset success for ckpt_id:%llx",evt->info.rd_set.ckpt_id);

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
	TRACE_LEAVE2("Ret Val %d",proc_rc);
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
static uint32_t cpd_evt_proc_active_set(CPD_CB *cb, CPD_EVT *evt, CPSV_SEND_INFO *sinfo)
{
	CPD_CKPT_INFO_NODE *ckpt_node = NULL;
	SaAisErrorT rc = SA_AIS_OK;
	CPSV_EVT send_evt;
	uint32_t proc_rc = NCSCC_RC_SUCCESS;

	TRACE_ENTER();
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
		TRACE_2("cpd ckpt active set success for ckpt_id:%llx,mds_dest:%"PRIu64,evt->info.arep_set.ckpt_id,
                               evt->info.arep_set.mds_dest);
	}

	TRACE_LEAVE2("Ret val %d",proc_rc);
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
static uint32_t cpd_evt_proc_ckpt_destroy(CPD_CB *cb, CPD_EVT *evt, CPSV_SEND_INFO *sinfo)
{
	CPD_CKPT_INFO_NODE *ckpt_node = NULL;
	uint32_t proc_rc = NCSCC_RC_SUCCESS;
	CPSV_EVT send_evt;
	bool o_ckpt_node_deleted = false;
	bool o_is_active_changed = false;
	bool ckptid_flag = false;

	TRACE_ENTER();
	memset(&send_evt, 0, sizeof(CPSV_EVT));

	cpd_ckpt_node_get(&cb->ckpt_tree, &evt->info.ckpt_destroy.ckpt_id, &ckpt_node);

	if (ckpt_node == 0) {
		send_evt.info.cpnd.info.destroy_ack.error = SA_AIS_ERR_NOT_EXIST;
		TRACE_4("cpd rep del failed for ckpt_id:%llx,dest:%"PRIu64,evt->info.ckpt_destroy.ckpt_id,sinfo->dest);
		goto send_rsp;
	}

	proc_rc = cpd_process_ckpt_delete(cb, ckpt_node, sinfo, &o_ckpt_node_deleted, &o_is_active_changed);
	if (proc_rc != NCSCC_RC_SUCCESS) {
		TRACE_4("cpd rep del failed for ckpt_id:%llx,dest:%"PRIu64,evt->info.ckpt_destroy.ckpt_id,
                               sinfo->dest);
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
		TRACE_2("cpd rep del success for ckpt_id:%llx,dest:%"PRIu64, evt->info.ckpt_destroy.ckpt_id,sinfo->dest);
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
		TRACE_2("cpd ckpt active change success ckpt_id:%llx, active_dest:%"PRIu64,evt->info.ckpt_destroy.ckpt_id, ckpt_node->active_dest);
	}

	/* Send this info to Standby */
	cpd_a2s_ckpt_dest_del(cb, evt->info.ckpt_destroy.ckpt_id, &sinfo->dest, ckptid_flag);

	memset(&send_evt, 0, sizeof(CPSV_EVT));
	send_evt.info.cpnd.info.destroy_ack.error = SA_AIS_OK;

 send_rsp:
	send_evt.type = CPSV_EVT_TYPE_CPND;
	send_evt.info.cpnd.type = CPND_EVT_D2ND_CKPT_DESTROY_ACK;
	proc_rc = cpd_mds_send_rsp(cb, sinfo, &send_evt);

	TRACE_LEAVE2("Ret val %d",proc_rc);
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

static uint32_t cpd_evt_proc_ckpt_destroy_byname(CPD_CB *cb, CPD_EVT *evt, CPSV_SEND_INFO *sinfo)
{
	CPD_CKPT_MAP_INFO *map_info = NULL;
	SaNameT *ckpt_name = &evt->info.ckpt_destroy_byname.ckpt_name;
	CPD_EVT destroy_evt;
	CPSV_EVT send_evt;
	uint32_t proc_rc = NCSCC_RC_SUCCESS;

	TRACE_ENTER();
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

	TRACE_LEAVE2("Ret val %d",proc_rc);
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
uint32_t cpd_evt_proc_cb_dump(CPD_CB *cb)
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
static uint32_t cpd_evt_proc_timer_expiry(CPD_CB *cb, CPD_EVT *evt)
{
	CPD_CPND_INFO_NODE *node_info = NULL;

	CPD_TMR_INFO *tmr_info = &evt->info.tmr_info;

	switch (tmr_info->type) {
	case CPD_TMR_TYPE_CPND_RETENTION:
		if (cb->ha_state == SA_AMF_HA_ACTIVE) {
			cpd_cpnd_info_node_get(&cb->cpnd_tree, &tmr_info->info.cpnd_dest, &node_info);
			if (node_info) {
				(void)cpd_process_cpnd_down(cb, &tmr_info->info.cpnd_dest);
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
static uint32_t cpnd_down_process(CPD_CB *cb, CPSV_MDS_INFO *mds_info, CPD_CPND_INFO_NODE *cpnd_info)
{
	CPD_CKPT_REF_INFO *cref_info;
	CPSV_EVT send_evt;
	uint32_t proc_rc = NCSCC_RC_SUCCESS;

	TRACE_ENTER();
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
			TRACE_2("cpd rep del success for ckpt_id:%llx,cpnd_dest:%"PRIu64,cref_info->ckpt_node->ckpt_id,cpnd_info->cpnd_ret_timer.info.cpnd_dest);
		}
		cref_info = cref_info->next;
	}
	TRACE_LEAVE2("Ret val %d",proc_rc);
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
static uint32_t cpd_cpnd_dest_replace(CPD_CB *cb, CPD_CKPT_REF_INFO *cref_info, CPSV_MDS_INFO *mds_info)
{
	CPD_CKPT_INFO_NODE *cp_node = NULL;
	CPD_NODE_REF_INFO *nref_info = NULL;
	uint32_t rc = NCSCC_RC_SUCCESS;
	
	TRACE_ENTER();
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
	TRACE_LEAVE2("Ret val %d",rc);
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
static uint32_t cpnd_up_process(CPD_CB *cb, CPSV_MDS_INFO *mds_info, CPD_CPND_INFO_NODE *cpnd_info)
{
	CPD_CKPT_REF_INFO *cref_info = NULL;
	CPSV_EVT send_evt;
	uint32_t proc_rc = NCSCC_RC_SUCCESS;
	uint32_t i = 0;

	TRACE_ENTER();
	cpnd_info->cpnd_ret_timer.type = CPD_TMR_TYPE_CPND_RETENTION;
	cpnd_info->cpnd_ret_timer.info.cpnd_dest = mds_info->dest;
	cpd_tmr_stop(&cpnd_info->cpnd_ret_timer);

	cref_info = cpnd_info->ckpt_ref_list;

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
					LOG_CR("cpd cpnd_dest info memory allocation failed");
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
			send_evt.info.cpnd.info.ckpt_add.is_cpnd_restart = true;
			send_evt.info.cpnd.info.ckpt_add.ckpt_flags = cref_info->ckpt_node->ckpt_flags;

			if (send_evt.info.cpnd.info.ckpt_add.dest_cnt) {
				CPD_NODE_REF_INFO *node_list = cref_info->ckpt_node->node_list;

				send_evt.info.cpnd.info.ckpt_add.dest_list =
				    m_MMGR_ALLOC_CPSV_CPND_DEST_INFO(cref_info->ckpt_node->dest_cnt);

				if (!send_evt.info.cpnd.info.ckpt_add.dest_list) {
					/* No memory, don't know what to do, setting dest_cnt to zero & continue */
					LOG_CR("cpd cpnd_dest info memory allocation failed ");
					send_evt.info.cpnd.info.ckpt_add.dest_cnt = 0;
				}

				for (i = 0; i < send_evt.info.cpnd.info.ckpt_add.dest_cnt; i++) {
					send_evt.info.cpnd.info.ckpt_add.dest_list[i].dest = node_list->dest;
					node_list = node_list->next;
				}
			}

			proc_rc = cpd_mds_bcast_send(cb, &send_evt, NCSMDS_SVC_ID_CPND);
			TRACE_2("cpd rep add success ckpt_id:%llx,cpnd_dest:%"PRIu64,cref_info->ckpt_node->ckpt_id,cpnd_info->cpnd_ret_timer.info.cpnd_dest);

			if (send_evt.info.cpnd.info.ckpt_add.dest_list) {
				m_MMGR_FREE_CPSV_CPND_DEST_INFO(send_evt.info.cpnd.info.ckpt_add.dest_list);
				send_evt.info.cpnd.info.ckpt_add.dest_list = NULL;
				send_evt.info.cpnd.info.ckpt_add.dest_cnt = 0;
			}
		}
		cref_info = cref_info->next;
	}
	TRACE_LEAVE2("Ret val %d",proc_rc);
	return proc_rc;
}

static uint32_t cpd_evt_mds_quiesced_ack_rsp(CPD_CB *cb, CPD_EVT *evt, CPSV_SEND_INFO *sinfo)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	SaAisErrorT saErr = SA_AIS_OK;
	TRACE_ENTER();
	cb->ha_state = SA_AMF_HA_QUIESCED;	/* Set the HA State */
       /* Give up our IMM OI implementer role */
	saErr = immutil_saImmOiImplementerClear(cb->immOiHandle);
	if (saErr != SA_AIS_OK) {
		LOG_ER("saImmOiImplementerClear failed with err:%d",saErr);
	}
	
	cpd_mbcsv_chgrole(cb);
	saAmfResponse(cb->amf_hdl, cb->amf_invocation, saErr);
	
	TRACE_LEAVE2("Ret val %d",rc);
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
static uint32_t cpd_evt_proc_mds_evt(CPD_CB *cb, CPD_EVT *evt)
{
	CPSV_MDS_INFO *mds_info;
	CPD_CPND_INFO_NODE *node_info = NULL;
	CPD_CKPT_INFO_NODE *ckpt_node = NULL;
	CPD_NODE_REF_INFO *nref_info;
	CPD_CKPT_REF_INFO *cref_info = NULL;
	CPD_CKPT_MAP_INFO *map_info = NULL;
	SaCkptCheckpointHandleT prev_ckpt_hdl;
	bool flag = false;
	SaNameT ckpt_name;
	uint32_t phy_slot_sub_slot;
	bool add_flag = true;

	TRACE_ENTER();
	memset(&ckpt_name, 0, sizeof(SaNameT));
	mds_info = &evt->info.mds_info;

	memset(&phy_slot_sub_slot, 0, sizeof(uint32_t));

	switch (mds_info->change) {

	case NCSMDS_RED_UP:
		/* get the peer mds_red_up */
		/* get the mds_dest of remote CPND */
		TRACE_1("Recived mds red up ");
		if (cb->node_id != mds_info->node_id) {
			MDS_DEST tmpDest = 0LL;
			phy_slot_sub_slot = cpd_get_slot_sub_slot_id_from_node_id(mds_info->node_id);
			cb->cpd_remote_id = phy_slot_sub_slot;

			if (cb->is_rem_cpnd_up == false) {
				cpd_cpnd_info_node_getnext(&cb->cpnd_tree, &tmpDest, &node_info);
				while (node_info) {	/* while-1 */
					if (mds_info->node_id == node_info->cpnd_key) {
						cb->is_rem_cpnd_up = true;
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
												flag = true;
											}
											nref_info = nref_info->next;
										}
										if (flag == false) {
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
		TRACE_1("Recived mds up for %d",mds_info->svc_id);
		if (mds_info->svc_id == NCSMDS_SVC_ID_CPD) {
			if (mds_info->change == NCSMDS_UP)
				TRACE_LEAVE();
				return NCSCC_RC_SUCCESS;
		}

		if (mds_info->svc_id == NCSMDS_SVC_ID_CPND) {
			/* Save MDS address for this CPND */
			phy_slot_sub_slot = cpd_get_slot_sub_id_from_mds_dest(mds_info->dest);
			if (cb->cpd_remote_id == phy_slot_sub_slot) {
				cb->rem_cpnd_dest = mds_info->dest;
			}

			if (m_CPND_IS_ON_SCXB(cb->cpd_self_id, cpd_get_slot_sub_id_from_mds_dest(mds_info->dest))) {
				cb->is_loc_cpnd_up = true;
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
									flag = true;
								}
								nref_info = nref_info->next;
							}
							if (flag == false) {
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
				cb->is_rem_cpnd_up = true;
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
									flag = true;
								}
								nref_info = nref_info->next;
							}
							if (flag == false) {
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
					TRACE_LEAVE();
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
					TRACE_LEAVE();
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

		TRACE_1("Recived mds down for %d",mds_info->svc_id);
		if (mds_info->svc_id == NCSMDS_SVC_ID_CPD) {
			if (mds_info->change == NCSMDS_DOWN)
				TRACE_LEAVE();
				return NCSCC_RC_SUCCESS;
		}

		if (mds_info->svc_id == NCSMDS_SVC_ID_CPND) {
			if (m_CPND_IS_ON_SCXB(cb->cpd_self_id, cpd_get_slot_sub_id_from_mds_dest(mds_info->dest))) {
				cb->is_loc_cpnd_up = false;
			}

			if (m_CPND_IS_ON_SCXB(cb->cpd_remote_id, cpd_get_slot_sub_id_from_mds_dest(mds_info->dest))) {
				cb->is_rem_cpnd_up = false;
			}

			/* MDS address for this CPND is no longer valid */
			phy_slot_sub_slot = cpd_get_slot_sub_id_from_mds_dest(mds_info->dest);

			if (cb->ha_state == SA_AMF_HA_ACTIVE) {
				cpd_cpnd_info_node_get(&cb->cpnd_tree, &mds_info->dest, &node_info);
				if (!node_info) {
					/* No Checkpoints on this node, */
					TRACE_LEAVE();
					return NCSCC_RC_SUCCESS;
				} else if (node_info && (node_info->ckpt_ref_list == NULL)) {
					cpd_cpnd_info_node_delete(cb, node_info);
					TRACE_LEAVE();
					return NCSCC_RC_SUCCESS;
				} else
					cpnd_down_process(cb, mds_info, node_info);
			}
			if (cb->ha_state == SA_AMF_HA_STANDBY) {
				cpd_cpnd_info_node_get(&cb->cpnd_tree, &mds_info->dest, &node_info);
				if (!node_info) {
					TRACE_LEAVE();
					/* No Checkpoints on this node, */
					return NCSCC_RC_SUCCESS;
				}
				else if (node_info && (node_info->ckpt_ref_list == NULL)) {
					cpd_cpnd_info_node_delete(cb, node_info);
					TRACE_LEAVE();
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
	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}
