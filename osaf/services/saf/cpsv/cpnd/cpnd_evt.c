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
  cpnd_process_evt .........CPND Event processing routine.
******************************************************************************/

#include "cpnd.h"

extern uns32 cpnd_ckpt_non_collocated_rplica_close(CPND_CB *cb, CPND_CKPT_NODE *cp_node, SaAisErrorT *error);
extern uns32 cpnd_proc_non_colloc_rt_expiry(CPND_CB *cb, SaCkptCheckpointHandleT ckpt_id);

static uns32 cpnd_evt_proc_cb_dump(CPND_CB *cb);
static uns32 cpnd_evt_proc_ckpt_init(CPND_CB *cb, CPND_EVT *evt, CPSV_SEND_INFO *sinfo);
static uns32 cpnd_evt_proc_ckpt_finalize(CPND_CB *cb, CPND_EVT *evt, CPSV_SEND_INFO *sinfo);
static uns32 cpnd_evt_proc_ckpt_open(CPND_CB *cb, CPND_EVT *evt, CPSV_SEND_INFO *sinfo);
static uns32 cpnd_evt_proc_ckpt_close(CPND_CB *cb, CPND_EVT *evt, CPSV_SEND_INFO *sinfo);
static uns32 cpnd_evt_proc_ckpt_unlink(CPND_CB *cb, CPND_EVT *evt, CPSV_SEND_INFO *sinfo);
static uns32 cpnd_evt_proc_ckpt_unlink_info(CPND_CB *cb, CPND_EVT *evt, CPSV_SEND_INFO *sinfo);
/*  FOR CPND REDUNDANCY */
static uns32 cpsv_cpnd_restart(CPND_CB *cb, CPND_EVT *evt, CPSV_SEND_INFO *sinfo);
static uns32 cpsv_cpnd_restart_done(CPND_CB *cb, CPND_EVT *evt, CPSV_SEND_INFO *sinfo);

static uns32 cpnd_evt_proc_ckpt_rdset(CPND_CB *cb, CPND_EVT *evt, CPSV_SEND_INFO *sinfo);
static uns32 cpnd_evt_proc_ckpt_arep_set(CPND_CB *cb, CPND_EVT *evt, CPSV_SEND_INFO *sinfo);
static uns32 cpnd_evt_proc_ckpt_status_get(CPND_CB *cb, CPND_EVT *evt, CPSV_SEND_INFO *sinfo);
static uns32 cpnd_evt_proc_ckpt_active_set(CPND_CB *cb, CPND_EVT *evt, CPSV_SEND_INFO *sinfo);
static uns32 cpnd_evt_proc_ckpt_rdset_info(CPND_CB *cb, CPND_EVT *evt, CPSV_SEND_INFO *sinfo);
static uns32 cpnd_evt_proc_ckpt_rep_del(CPND_CB *cb, CPND_EVT *evt, CPSV_SEND_INFO *sinfo);
static uns32 cpnd_evt_proc_ckpt_rep_add(CPND_CB *cb, CPND_EVT *evt, CPSV_SEND_INFO *sinfo);
static uns32 cpnd_evt_proc_ckpt_sect_exp_set(CPND_CB *cb, CPND_EVT *evt, CPSV_SEND_INFO *sinfo);
static uns32 cpnd_evt_proc_ckpt_sect_create(CPND_CB *cb, CPND_EVT *evt, CPSV_SEND_INFO *sinfo);
static uns32 cpnd_evt_proc_ckpt_sect_delete(CPND_CB *cb, CPND_EVT *evt, CPSV_SEND_INFO *sinfo);
static uns32 cpnd_evt_proc_nd2nd_ckpt_sect_create(CPND_CB *cb, CPND_EVT *evt, CPSV_SEND_INFO *sinfo);
static uns32 cpnd_evt_proc_nd2nd_ckpt_sect_delete(CPND_CB *cb, CPND_EVT *evt, CPSV_SEND_INFO *sinfo);
static uns32 cpnd_evt_proc_nd2nd_ckpt_sect_exptmr_req(CPND_CB *cb, CPND_EVT *evt, CPSV_SEND_INFO *sinfo);
static uns32 cpnd_evt_proc_nd2nd_ckpt_status(CPND_CB *cb, CPND_EVT *evt, CPSV_SEND_INFO *sinfo);
static uns32 cpnd_evt_proc_ckpt_write(CPND_CB *cb, CPND_EVT *evt, CPSV_SEND_INFO *sinfo);
static uns32 cpnd_evt_proc_nd2nd_ckpt_active_data_access_req(CPND_CB *cb, CPND_EVT *evt, CPSV_SEND_INFO *sinfo);
static uns32 cpnd_evt_proc_nd2nd_ckpt_active_data_access_rsp(CPND_CB *cb, CPND_EVT *evt, CPSV_SEND_INFO *sinfo);
static uns32 cpnd_evt_proc_ckpt_sync(CPND_CB *cb, CPND_EVT *evt, CPSV_SEND_INFO *sinfo);
static uns32 cpnd_evt_proc_arrival_cbreg(CPND_CB *cb, CPND_EVT *evt, CPSV_SEND_INFO *sinfo);
static uns32 cpnd_evt_proc_nd2nd_ckpt_sync_req(CPND_CB *cb, CPND_EVT *evt, CPSV_SEND_INFO *sinfo);
static uns32 cpnd_evt_proc_nd2nd_ckpt_active_sync(CPND_CB *cb, CPND_EVT *evt, CPSV_SEND_INFO *sinfo);
static uns32 cpnd_evt_proc_ckpt_read(CPND_CB *cb, CPND_EVT *evt, CPSV_SEND_INFO *sinfo);
static uns32 cpnd_evt_proc_timer_expiry(CPND_CB *cb, CPND_EVT *evt);
static uns32 cpnd_evt_proc_mds_evt(CPND_CB *cb, CPND_EVT *evt);
static uns32 cpnd_evt_proc_ckpt_destroy(CPND_CB *cb, CPND_EVT *evt, CPSV_SEND_INFO *sinfo);
static uns32 cpnd_evt_proc_ckpt_create(CPND_CB *cb, CPND_EVT *evt, CPSV_SEND_INFO *sinfo);

static uns32 cpnd_evt_proc_ckpt_sect_iter_req(CPND_CB *cb, CPND_EVT *evt, CPSV_SEND_INFO *sinfo);
static uns32 cpnd_evt_proc_ckpt_iter_getnext(CPND_CB *cb, CPND_EVT *evt, CPSV_SEND_INFO *sinfo);
static uns32 cpnd_evt_proc_ckpt_iter_getnext(CPND_CB *cb, CPND_EVT *evt, CPSV_SEND_INFO *sinfo);
static uns32 cpnd_evt_proc_ckpt_iter_next_req(CPND_CB *cb, CPND_EVT *evt, CPSV_SEND_INFO *sinfo);
static uns32 cpnd_evt_proc_ckpt_mem_size(CPND_CB *cb, CPND_EVT *evt, CPSV_SEND_INFO *sinfo);
static uns32 cpnd_evt_proc_ckpt_num_sections(CPND_CB *cb, CPND_EVT *evt, CPSV_SEND_INFO *sinfo);
static uns32 cpsv_cpnd_active_replace(CPND_CKPT_NODE *cp_node, CPND_EVT *evt);
static uns32 cpnd_evt_proc_ckpt_refcntset(CPND_CB *cb,CPND_EVT *evt);
static uns32 cpnd_proc_cpd_new_active(CPND_CB *cb);

static uns32 cpnd_is_cpd_up(CPND_CB *cb);

static uns32 cpnd_transfer_replica(CPND_CB *cb, CPND_CKPT_NODE *cp_node, SaCkptCheckpointHandleT ckpt_id,
				   CPSV_CPND_DEST_INFO *dest_list, CPSV_A2ND_CKPT_SYNC sync);

#if (CPSV_DEBUG == 1)
static char *cpnd_evt_str[] = {
	"CPND_EVT_BASE",
	"CPND_EVT_MDS_INFO",	/* CPA/CPND/CPD UP/DOWN Info */
	"CPND_EVT_TIME_OUT",	/* Time out event */
	"CPND_EVT_A2ND_CKPT_INIT",	/* Checkpoint Initialization */
	"CPND_EVT_A2ND_CKPT_FINALIZE",	/* Checkpoint finalization */
	"CPND_EVT_A2ND_CKPT_OPEN",	/* Checkpoint Open Request */
	"CPND_EVT_A2ND_CKPT_CLOSE",	/* Checkpoint Close Call */
	"CPND_EVT_A2ND_CKPT_UNLINK",	/* Checkpoint Unlink Call */
	"CPND_EVT_A2ND_CKPT_RDSET",	/* Checkpoint Retention duration set call */
	"CPND_EVT_A2ND_CKPT_AREP_SET",	/* Checkpoint Active Replica Set Call */
	"CPND_EVT_A2ND_CKPT_STATUS_GET",	/* Checkpoint Status Get Call */
	"CPND_EVT_A2ND_CKPT_SECT_CREATE",	/* Checkpoint Section Create Call */
	"CPND_EVT_A2ND_CKPT_SECT_DELETE",	/* Checkpoint Section Delete Call */
	"CPND_EVT_A2ND_CKPT_SECT_EXP_SET",	/* Checkpoint Section Expiry Time Set Call */
	"CPND_EVT_A2ND_CKPT_SECT_ITER_REQ",	/*Checkpoint Section Iteration Initialize */
	"CPND_EVT_A2ND_CKPT_ITER_GETNEXT",	/* Checkpoint Section Iternation Getnext Call */
	"CPND_EVT_A2ND_CKPT_WRITE",	/* Checkpoint Write And overwrite call */
	"CPND_EVT_A2ND_CKPT_READ",	/* Checkpoint Read Call  */
	"CPND_EVT_A2ND_CKPT_SYNC",	/* Checkpoint Synchronize call */
	"CPND_EVT_A2ND_CKPT_READ_ACK",	/* read ack */
	"CPND_EVT_A2ND_ARRIVAL_CB_REG",	/* Arrival Callback Register */

	"CPND_EVT_ND2ND_ACTIVE_STATUS",	/* ckpt status info from active */	/* Not used Anywhere from 3.0.2 */
	"CPND_EVT_ND2ND_ACTIVE_STATUS_ACK",	/* ckpt status ack from active */	/*Not used Anywhere from 3.0.2 */
	"CPND_EVT_ND2ND_CKPT_SYNC_REQ",	/* rqst from ND to ND(A) to sync ckpt */	/* Sync request to Active */
	"CPND_EVT_ND2ND_CKPT_ACTIVE_SYNC",	/* CPND(A) sync updts to All the Ckpts */	/* Sync response from Active */

	"CPSV_EVT_ND2ND_CKPT_SECT_CREATE_REQ",	/* Sect_Create info sent from Active - Non Active */
	"CPSV_EVT_ND2ND_CKPT_SECT_CREATE_RSP",	/* Not used anywhere */
	"CPSV_EVT_ND2ND_CKPT_SECT_ACTIVE_CREATE_RSP",	/* Response for Sect_Create frm Non Active- Active */
	"CPSV_EVT_ND2ND_CKPT_SECT_DELETE_REQ",	/* Sect_Delete info sent from Active - Non Active */
	"CPSV_EVT_ND2ND_CKPT_SECT_DELETE_RSP",	/*  Response for Sect_Delete frm Non Active- Active */
	"CPSV_EVT_ND2ND_CKPT_SECT_EXPTMR_REQ",
	"CPSV_EVT_ND2ND_CKPT_SECT_EXPTMR_RSP",
	"CPSV_EVT_ND2ND_CKPT_SECT_DATA_ACCESS_REQ",	/* for write,read,overwrite  Not used Now */
	"CPSV_EVT_ND2ND_CKPT_SECT_ACTIVE_DATA_ACCESS_REQ",	/* Write info from Active- Non Active */
	"CPSV_EVT_ND2ND_CKPT_SECT_ACTIVE_DATA_ACCESS_RSP",	/* Response  Write info from Non Active- Active */

	/* FOR CPND REDUNDANCY */
	"CPSV_D2ND_RESTART",
	"CPSV_D2ND_RESTART_DONE",

	"CPND_EVT_D2ND_CKPT_INFO",	/* Rsp to the ckpt open call */
	"CPND_EVT_D2ND_CKPT_SIZE",
	"CPND_EVT_D2ND_CKPT_REP_ADD",	/* ckpt open is propogated to other NDs */
	"CPND_EVT_D2ND_CKPT_REP_DEL",	/* ckpt close is propogated to other NDs */

	"CPND_EVT_D2ND_CKPT_CREATE",	/* ckpt create evt for Non-collocated */
	"CPND_EVT_D2ND_CKPT_DESTROY",	/* The ckpt destroy evt for Non-colloc */
	"CPND_EVT_D2ND_CKPT_DESTROY_ACK",
	"CPND_EVT_D2ND_CKPT_CLOSE_ACK",	/* Rsps to ckpt close call */
	"CPND_EVT_D2ND_CKPT_UNLINK",	/* Unlink info */
	"CPND_EVT_D2ND_CKPT_UNLINK_ACK",	/* Rsps to ckpt unlink call */
	"CPND_EVT_D2ND_CKPT_RDSET",	/* Retention duration to set */
	"CPND_EVT_D2ND_CKPT_RDSET_ACK",	/* Retention duration Ack */
	"CPND_EVT_D2ND_CKPT_ACTIVE_SET",	/* for colloc ckpts,mark the Active */
	"CPND_EVT_D2ND_CKPT_ACTIVE_SET_ACK",	/* Ack for active replica set rqst */
	"CPND_EVT_ND2ND_CKPT_ITER_NEXT_REQ",
	"CPND_EVT_ND2ND_CKPT_ACTIVE_ITERNEXT",
	"CPND_EVT_CB_DUMP",
	"CPND_EVT_D2ND_CKPT_NUM_SECTIONS",
   "CPND_EVT_A2ND_CKPT_REFCNTSET", 
	"CPND_EVT_MAX"
};
#endif

#define m_ASSIGN_CPSV_HANDLE_ID(handle_id)       handle_id
uns32 gl_read_lck = 0;

void cpnd_process_evt(CPSV_EVT *evt)
{
	CPND_CB *cb = NULL;
	uns32 cb_hdl = m_CPND_GET_CB_HDL;
	uns32 rc = NCSCC_RC_SUCCESS;
	CPND_SYNC_SEND_NODE *node = NULL;

   if(evt->type != CPSV_EVT_TYPE_CPND)
   {
      m_LOG_CPND_CL(CPND_EVT_UNKNOWN, CPND_FC_EVT,NCSFL_SEV_ERROR,__FILE__,__LINE__);
      cpnd_evt_destroy(evt);
      return;
   }

	/* Get the CB from the handle */
	cb = m_CPND_TAKE_CPND_CB;
	if (cb == NULL) {
		m_LOG_CPND_CL(CPND_CB_HDL_TAKE_FAILED, CPND_FC_HDLN, NCSFL_SEV_ERROR, __FILE__, __LINE__);
		cpnd_evt_destroy(evt);
		return;
	}
#if (CPSV_DEBUG == 1)
	TRACE("%s", cpnd_evt_str[evt->info.cpnd.type]);
#endif

	switch (evt->info.cpnd.type) {
	case CPND_EVT_MDS_INFO:
		rc = cpnd_evt_proc_mds_evt(cb, &evt->info.cpnd);
		break;

	case CPND_EVT_TIME_OUT:
		rc = cpnd_evt_proc_timer_expiry(cb, &evt->info.cpnd);
		break;

		/* Agent to ND */
	case CPND_EVT_A2ND_CKPT_INIT:
		rc = cpnd_evt_proc_ckpt_init(cb, &evt->info.cpnd, &evt->sinfo);
		break;

	case CPND_EVT_A2ND_CKPT_FINALIZE:
		rc = cpnd_evt_proc_ckpt_finalize(cb, &evt->info.cpnd, &evt->sinfo);
		break;

	case CPND_EVT_A2ND_CKPT_OPEN:
		rc = cpnd_evt_proc_ckpt_open(cb, &evt->info.cpnd, &evt->sinfo);
		break;

	case CPND_EVT_A2ND_CKPT_CLOSE:
		rc = cpnd_evt_proc_ckpt_close(cb, &evt->info.cpnd, &evt->sinfo);
		break;

	case CPND_EVT_A2ND_CKPT_UNLINK:
		rc = cpnd_evt_proc_ckpt_unlink(cb, &evt->info.cpnd, &evt->sinfo);
		break;

	case CPND_EVT_A2ND_CKPT_RDSET:
		rc = cpnd_evt_proc_ckpt_rdset(cb, &evt->info.cpnd, &evt->sinfo);
		break;

	case CPND_EVT_A2ND_CKPT_AREP_SET:
		rc = cpnd_evt_proc_ckpt_arep_set(cb, &evt->info.cpnd, &evt->sinfo);
		break;

	case CPND_EVT_A2ND_CKPT_STATUS_GET:
		rc = cpnd_evt_proc_ckpt_status_get(cb, &evt->info.cpnd, &evt->sinfo);
		break;

	case CPND_EVT_A2ND_CKPT_SECT_CREATE:
		rc = cpnd_evt_proc_ckpt_sect_create(cb, &evt->info.cpnd, &evt->sinfo);
		break;

	case CPND_EVT_A2ND_CKPT_SECT_DELETE:
		rc = cpnd_evt_proc_ckpt_sect_delete(cb, &evt->info.cpnd, &evt->sinfo);
		break;

	case CPND_EVT_A2ND_CKPT_SECT_EXP_SET:
		rc = cpnd_evt_proc_ckpt_sect_exp_set(cb, &evt->info.cpnd, &evt->sinfo);
		break;

	case CPND_EVT_A2ND_CKPT_SECT_ITER_REQ:
		rc = cpnd_evt_proc_ckpt_sect_iter_req(cb, &evt->info.cpnd, &evt->sinfo);
		break;

	case CPND_EVT_A2ND_CKPT_ITER_GETNEXT:
		rc = cpnd_evt_proc_ckpt_iter_getnext(cb, &evt->info.cpnd, &evt->sinfo);
		break;

	case CPND_EVT_ND2ND_CKPT_ITER_NEXT_REQ:
		rc = cpnd_evt_proc_ckpt_iter_next_req(cb, &evt->info.cpnd, &evt->sinfo);
		break;

	case CPND_EVT_A2ND_CKPT_WRITE:
		rc = cpnd_evt_proc_ckpt_write(cb, &evt->info.cpnd, &evt->sinfo);
		break;

	case CPND_EVT_A2ND_CKPT_READ:
		rc = cpnd_evt_proc_ckpt_read(cb, &evt->info.cpnd, &evt->sinfo);
		break;
/*
   case CPND_EVT_A2ND_CKPT_READ_ACK:
      rc = cpnd_evt_proc_ckpt_read_ack(cb, &evt->info.cpnd, &evt->sinfo);
      break;
*/
	case CPND_EVT_A2ND_CKPT_SYNC:
		rc = cpnd_evt_proc_ckpt_sync(cb, &evt->info.cpnd, &evt->sinfo);
		break;

	case CPND_EVT_A2ND_ARRIVAL_CB_REG:
		rc = cpnd_evt_proc_arrival_cbreg(cb, &evt->info.cpnd, &evt->sinfo);
		break;
	case CPND_EVT_D2ND_CKPT_ACTIVE_SET:	/* broadcast message */
		rc = cpnd_evt_proc_ckpt_active_set(cb, &evt->info.cpnd, &evt->sinfo);
		break;

	case CPND_EVT_D2ND_CKPT_RDSET:
		rc = cpnd_evt_proc_ckpt_rdset_info(cb, &evt->info.cpnd, &evt->sinfo);
		break;

	case CPND_EVT_D2ND_CKPT_SIZE:
		rc = cpnd_evt_proc_ckpt_mem_size(cb, &evt->info.cpnd, &evt->sinfo);
		break;

	case CPND_EVT_D2ND_CKPT_NUM_SECTIONS:
		rc = cpnd_evt_proc_ckpt_num_sections(cb, &evt->info.cpnd, &evt->sinfo);
		break;

	case CPND_EVT_D2ND_CKPT_REP_ADD:
		rc = cpnd_evt_proc_ckpt_rep_add(cb, &evt->info.cpnd, &evt->sinfo);
		break;

	case CPND_EVT_D2ND_CKPT_REP_DEL:
		rc = cpnd_evt_proc_ckpt_rep_del(cb, &evt->info.cpnd, &evt->sinfo);
		break;

	case CPND_EVT_D2ND_CKPT_UNLINK:
		rc = cpnd_evt_proc_ckpt_unlink_info(cb, &evt->info.cpnd, &evt->sinfo);
		break;
/* FOR CPND REDUNDANCY */
	case CPSV_D2ND_RESTART:
		rc = cpsv_cpnd_restart(cb, &evt->info.cpnd, &evt->sinfo);
		break;

	case CPSV_D2ND_RESTART_DONE:
		rc = cpsv_cpnd_restart_done(cb, &evt->info.cpnd, &evt->sinfo);
		break;

		/* shashi TBD :need to code for the following */

	case CPND_EVT_D2ND_CKPT_DESTROY:
		rc = cpnd_evt_proc_ckpt_destroy(cb, &evt->info.cpnd, &evt->sinfo);
		break;

	case CPND_EVT_D2ND_CKPT_CREATE:	/* happens for non-collocated and received
						   by CPNDs on SCXB */
		rc = cpnd_evt_proc_ckpt_create(cb, &evt->info.cpnd, &evt->sinfo);
		break;

		/* end */

/* ND-ND */

	case CPND_EVT_ND2ND_ACTIVE_STATUS:
		rc = cpnd_evt_proc_nd2nd_ckpt_status(cb, &evt->info.cpnd, &evt->sinfo);
		break;

	case CPND_EVT_ND2ND_CKPT_SYNC_REQ:
		rc = cpnd_evt_proc_nd2nd_ckpt_sync_req(cb, &evt->info.cpnd, &evt->sinfo);
		break;

	case CPND_EVT_ND2ND_CKPT_ACTIVE_SYNC:
		rc = cpnd_evt_proc_nd2nd_ckpt_active_sync(cb, &evt->info.cpnd, &evt->sinfo);
		break;

	case CPSV_EVT_ND2ND_CKPT_SECT_CREATE_REQ:
		rc = cpnd_evt_proc_nd2nd_ckpt_sect_create(cb, &evt->info.cpnd, &evt->sinfo);
		break;

	case CPSV_EVT_ND2ND_CKPT_SECT_DELETE_REQ:
		rc = cpnd_evt_proc_nd2nd_ckpt_sect_delete(cb, &evt->info.cpnd, &evt->sinfo);
		break;

	case CPSV_EVT_ND2ND_CKPT_SECT_EXPTMR_REQ:
		rc = cpnd_evt_proc_nd2nd_ckpt_sect_exptmr_req(cb, &evt->info.cpnd, &evt->sinfo);
		break;

	case CPSV_EVT_ND2ND_CKPT_SECT_ACTIVE_DATA_ACCESS_REQ:
		rc = cpnd_evt_proc_nd2nd_ckpt_active_data_access_req(cb, &evt->info.cpnd, &evt->sinfo);
		break;

	case CPSV_EVT_ND2ND_CKPT_SECT_ACTIVE_DATA_ACCESS_RSP:
		rc = cpnd_evt_proc_nd2nd_ckpt_active_data_access_rsp(cb, &evt->info.cpnd, &evt->sinfo);
		break;

	case CPND_EVT_CB_DUMP:
		rc = cpnd_evt_proc_cb_dump(cb);
		break;
  case CPND_EVT_A2ND_CKPT_REFCNTSET:
     rc = cpnd_evt_proc_ckpt_refcntset(cb, &evt->info.cpnd);
     break;

	default:
		rc = NCSCC_RC_FAILURE;
		break;
	}

	/* Remove item from deadlock prevention sync send list */
	m_NCS_LOCK(&cb->cpnd_sync_send_lock, NCS_LOCK_WRITE);

	node = ncs_remove_item(&cb->cpnd_sync_send_list, (void *)evt, cpnd_match_evt);

	if (node)
		m_MMGR_FREE_CPND_SYNC_SEND_NODE(node);

	m_NCS_UNLOCK(&cb->cpnd_sync_send_lock, NCS_LOCK_WRITE);

	/* Free the Event */
	cpnd_evt_destroy(evt);

	/* Return the Handle */
	ncshm_give_hdl(cb_hdl);

	return;
}

/****************************************************************************
 * Name          : cpnd_evt_proc_ckpt_init
 *
 * Description   : Function to process the SaCkptInitialize from clients
 *
 * Arguments     : CPND_CB *cb - CPND CB pointer
 *                 CPND_EVT *evt - Received Event structure
 *
 * Return Values : NCSCC_RC_SUCCESS/Error.
 *
 * Notes         : None.
 *****************************************************************************/
static uns32 cpnd_evt_proc_ckpt_init(CPND_CB *cb, CPND_EVT *evt, CPSV_SEND_INFO *sinfo)
{
	CPSV_EVT send_evt;
	uns32 rc = NCSCC_RC_SUCCESS;
	CPND_CKPT_CLIENT_NODE *cl_node = NULL;
	int32 cl_offset;

	memset(&send_evt, '\0', sizeof(CPSV_EVT));

	/* Allocate the CPND_CKPT_CLIENT_NODE Struct
	   & populate ckpt_app_hdl, agent_mds_dest and version
	   Add it to patricia cb->cpnd_client_info */
	if (m_CPA_VER_IS_ABOVE_B_1_1(&evt->info.initReq.version))
		if (cb->is_joined_cl == FALSE) {
			m_LOG_CPND_CL(CPND_CLM_NODE_GET_FAILED, CPND_FC_HDLN, NCSFL_SEV_ERROR, __FILE__, __LINE__);
			rc = SA_AIS_ERR_UNAVAILABLE;
			goto agent_rsp;
		}

	cl_node = m_MMGR_ALLOC_CPND_CKPT_CLIENT_NODE;
	if (cl_node == NULL) {
		m_LOG_CPND_CL(CPND_CLIENT_ALLOC_FAILED, CPND_FC_MEMFAIL, NCSFL_SEV_ERROR, __FILE__, __LINE__);
		rc = SA_AIS_ERR_NO_MEMORY;
		goto agent_rsp;
	}

	memset(cl_node, '\0', sizeof(CPND_CKPT_CLIENT_NODE));

	/* cl_node->ckpt_app_hdl= m_ASSIGN_CPSV_HANDLE_ID((uns32)cl_node); */	/* hdl is saf tdef */
	cl_node->ckpt_app_hdl = cb->cli_id_gen;
	cl_node->agent_mds_dest = sinfo->dest;
	cl_node->version = evt->info.initReq.version;
	cl_node->ckpt_list = NULL;

	if (cpnd_client_node_add(cb, cl_node) != NCSCC_RC_SUCCESS) {
		m_LOG_CPND_FCL(CPND_CLIENT_TREE_ADD_FAILED, CPND_FC_API, NCSFL_SEV_ERROR,
			       cl_node->ckpt_app_hdl, __FILE__, __LINE__);
		m_MMGR_FREE_CPND_CKPT_CLIENT_NODE(cl_node);
		rc = SA_AIS_ERR_NO_MEMORY;
		goto agent_rsp;
	}

	/* CPND RESTART UPDATE THE SHARED MEMORY WITH CLIENT INFO  */
	cl_offset = cpnd_restart_shm_client_update(cb, cl_node);
	/* -1 shared memory is full &&& -2 - shared memory read failed */
	if (cl_offset == -1 || cl_offset == -2) {
		m_LOG_CPND_FCL(CPND_CLIENT_INF0_UPDATE_FAILED, CPND_FC_RESTART, NCSFL_SEV_ERROR,
			       cl_offset, __FILE__, __LINE__);
	}
	send_evt.info.cpa.info.initRsp.ckptHandle = cl_node->ckpt_app_hdl;
	rc = SA_AIS_OK;

	cb->cli_id_gen++;

	if (rc == SA_AIS_OK)
		m_LOG_CPND_FCL(CPND_CLIENT_INF0_UPDATE_SUCCESS, CPND_FC_RESTART, NCSFL_SEV_INFO,
			       cl_node->ckpt_app_hdl, __FILE__, __LINE__);

 agent_rsp:
	send_evt.type = CPSV_EVT_TYPE_CPA;
	send_evt.info.cpa.type = CPA_EVT_ND2A_CKPT_INIT_RSP;
	send_evt.info.cpa.info.initRsp.error = rc;

	rc = cpnd_mds_send_rsp(cb, sinfo, &send_evt);

	return rc;

}

/****************************************************************************
 * Name          : cpnd_evt_proc_ckpt_finalize
 *
 * Description   : Function to process the SaCkptFinalize from Applications
 *
 * Arguments     : CPND_CB *cb - CPND CB pointer
 *                 CPSV_EVT *evt - Received Event structure
 *
 * Return Values : NCSCC_RC_SUCCESS/Error.
 *
 * Notes         : None.
 *****************************************************************************/
static uns32 cpnd_evt_proc_ckpt_finalize(CPND_CB *cb, CPND_EVT *evt, CPSV_SEND_INFO *sinfo)
{
	CPSV_EVT send_evt;
	uns32 rc = NCSCC_RC_SUCCESS;
	CPND_CKPT_CLIENT_NODE *cl_node = NULL;
	CPND_CKPT_NODE *cp_node = NULL;
	SaAisErrorT error;

	memset(&send_evt, '\0', sizeof(CPSV_EVT));

	if (!cpnd_is_cpd_up(cb)) {
		send_evt.info.cpa.info.finRsp.error = SA_AIS_ERR_TRY_AGAIN;
		goto agent_rsp;
	}

	cpnd_client_node_get(cb, evt->info.finReq.client_hdl, &cl_node);
	if (cl_node == NULL) {
		m_LOG_CPND_FCL(CPND_CLIENT_NODE_GET_FAILED, CPND_FC_API, NCSFL_SEV_ERROR,
			       evt->info.finReq.client_hdl, __FILE__, __LINE__);
		send_evt.info.cpa.info.finRsp.error = SA_AIS_ERR_BAD_HANDLE;
		goto agent_rsp;

	}

	while (cl_node->ckpt_list != NULL) {

		cp_node = cl_node->ckpt_list->cnode;

		cpnd_ckpt_client_del(cp_node, cl_node);
		cpnd_client_ckpt_info_del(cl_node, cp_node);

		if (cp_node->ckpt_lcl_ref_cnt)
			cp_node->ckpt_lcl_ref_cnt--;

		/* reset the client info in shared memory */
		cpnd_restart_client_reset(cb, cp_node, cl_node);

		/* Sending the Num Users, Num Writers , Num readers & Local Ref Count Update to CPD */
		rc = cpnd_send_ckpt_usr_info_to_cpd(cb, cp_node, cp_node->open_flags, CPSV_USR_INFO_CKPT_CLOSE);
		if (rc != NCSCC_RC_SUCCESS) {
			m_LOG_CPND_CL(CPND_MDS_SEND_FAIL, CPND_FC_HDLN, NCSFL_SEV_ERROR, __FILE__, __LINE__);
		}
		m_LOG_CPND_FFLCL(CPND_CLIENT_CKPT_CLOSE_SUCCESS, CPND_FC_API, NCSFL_SEV_INFO,
				 cl_node->ckpt_app_hdl, cp_node->ckpt_id, cp_node->ckpt_lcl_ref_cnt, __FILE__,
				 __LINE__);

		rc = cpnd_ckpt_replica_close(cb, cp_node, &error);
		if (rc == NCSCC_RC_FAILURE) {
			m_LOG_CPND_FCL(CPND_CKPT_REPLICA_CLOSE_FAILED, CPND_FC_GENERIC, NCSFL_SEV_ERROR,
				       cp_node->ckpt_id, __FILE__, __LINE__);
			send_evt.info.cpa.info.finRsp.error = error;
			goto agent_rsp;
		}
	}

	/* CPND RESTART - FREE THE GLOBAL SHARED MEMORY */
	m_LOG_CPND_FCL(CPND_CLIENT_FINALIZE_SUCCESS, CPND_FC_API, NCSFL_SEV_INFO,
		       cl_node->ckpt_app_hdl, __FILE__, __LINE__);
	cpnd_restart_client_node_del(cb, cl_node);

	rc = cpnd_client_node_del(cb, cl_node);
	if (rc == NCSCC_RC_FAILURE) {
		m_LOG_CPND_CL(CPND_CLIENT_TREE_DEL_FAILED, CPND_FC_API, NCSFL_SEV_ERROR, __FILE__, __LINE__);
	}

	m_MMGR_FREE_CPND_CKPT_CLIENT_NODE(cl_node);

	send_evt.info.cpa.info.finRsp.error = SA_AIS_OK;

 agent_rsp:
	send_evt.type = CPSV_EVT_TYPE_CPA;
	send_evt.info.cpa.type = CPA_EVT_ND2A_CKPT_FINALIZE_RSP;
	rc = cpnd_mds_send_rsp(cb, sinfo, &send_evt);
	return rc;
}

/****************************************************************************
 * Name          : cpnd_evt_proc_ckpt_open
 *
 * Description   : Function to process the SaCkptCkeckpointOpen & 
 *                 SaCkptCkeckpointOpenAsync from Applications
 *                 
 *
 * Arguments     : CPND_CB *cb - CPND CB pointer
 *                 CPSV_EVT *evt - Received Event structure
 *
 * Return Values : NCSCC_RC_SUCCESS/Error.
 *
 * Notes         : None.
 *****************************************************************************/
static uns32 cpnd_evt_proc_ckpt_open(CPND_CB *cb, CPND_EVT *evt, CPSV_SEND_INFO *sinfo)
{
	CPSV_EVT send_evt, *out_evt = NULL;
	SaNameT ckpt_name;
	uns32 rc = NCSCC_RC_SUCCESS;
	CPND_CPD_DEFERRED_REQ_NODE *node = NULL;
	CPND_CKPT_CLIENT_NODE *cl_node = NULL;
	CPND_CKPT_NODE *cp_node = NULL;
	SaCkptHandleT client_hdl;
	SaAisErrorT error;
	SaTimeT timeout;

	memset(&send_evt, '\0', sizeof(CPSV_EVT));

	if (!cpnd_is_cpd_up(cb)) {
		send_evt.info.cpa.info.openRsp.error = SA_AIS_ERR_TRY_AGAIN;
		goto agent_rsp;
	}

	ckpt_name = evt->info.openReq.ckpt_name;
	client_hdl = evt->info.openReq.client_hdl;

	cpnd_client_node_get(cb, client_hdl, &cl_node);
	if (cl_node == NULL) {
		m_LOG_CPND_FCL(CPND_CLIENT_HDL_GET_FAILED, CPND_FC_HDLN, NCSFL_SEV_ERROR,
			       client_hdl, __FILE__, __LINE__);
		return rc;
	}

	if (((cp_node = cpnd_ckpt_node_find_by_name(cb, ckpt_name)) != NULL) && cp_node->is_unlink == FALSE) {
		if (m_CPA_VER_IS_ABOVE_B_1_1(&cl_node->version)) {
			if (m_CPND_IS_CREAT_ATTRIBUTE_EQUAL(cp_node->create_attrib, evt->info.openReq.ckpt_attrib) !=
			    TRUE) {

				if (m_CPND_IS_CHECKPOINT_CREATE_SET(evt->info.openReq.ckpt_flags) == TRUE) {
					send_evt.info.cpa.info.openRsp.error = SA_AIS_ERR_EXIST;
					goto agent_rsp;
				}
			}
		} else {
			if (m_CPND_IS_CREAT_ATTRIBUTE_EQUAL_B_1_1(cp_node->create_attrib, evt->info.openReq.ckpt_attrib)
			    != TRUE) {
				if (m_CPND_IS_CHECKPOINT_CREATE_SET(evt->info.openReq.ckpt_flags) == TRUE) {
					send_evt.info.cpa.info.openRsp.error = SA_AIS_ERR_EXIST;
					goto agent_rsp;
				}
			}
		}

		/* found locally,incr ckpt_locl ref count and add client list info */
		if (TRUE == cp_node->is_restart) {
			send_evt.info.cpa.info.openRsp.error = SA_AIS_ERR_TRY_AGAIN;
			goto agent_rsp;
		}

		if (cp_node->is_close == TRUE) {
			cpnd_tmr_stop(&cp_node->ret_tmr);
			cp_node->is_close = FALSE;
			cpnd_restart_reset_close_flag(cb, cp_node);
		}

		cp_node->ckpt_lcl_ref_cnt++;
		cp_node->is_cpa_created_ckpt_replica = TRUE;

		cpnd_ckpt_client_add(cp_node, cl_node);
		cpnd_client_ckpt_info_add(cl_node, cp_node);

/* 2 initializes and the 2nd initialize opens same ckpt then just update his client hdl to ckpt */
		cpnd_restart_shm_ckpt_update(cb, cp_node, client_hdl);

		rc = cpnd_send_ckpt_usr_info_to_cpd(cb, cp_node, evt->info.openReq.ckpt_flags, CPSV_USR_INFO_CKPT_OPEN);
		if (rc != NCSCC_RC_SUCCESS) {
			m_LOG_CPND_FCL(CPND_MDS_SEND_FAIL, CPND_FC_HDLN, NCSFL_SEV_ERROR, cp_node->ckpt_id,
				       __FILE__, __LINE__);
		}

		send_evt.info.cpa.info.openRsp.gbl_ckpt_hdl = cp_node->ckpt_id;
		send_evt.info.cpa.info.openRsp.addr = cp_node->replica_info.open.info.open.o_addr;
		send_evt.info.cpa.info.openRsp.creation_attr = cp_node->create_attrib;
		send_evt.info.cpa.info.openRsp.error = SA_AIS_OK;
		if (cp_node->is_active_exist) {
			send_evt.info.cpa.info.openRsp.is_active_exists = TRUE;
			send_evt.info.cpa.info.openRsp.active_dest = cp_node->active_mds_dest;
		}
		m_LOG_CPND_FFFLCL(CPND_CKPT_OPEN_SUCCESS, CPND_FC_API, NCSFL_SEV_INFO, client_hdl, cp_node->ckpt_id,
				  cp_node->active_mds_dest, cp_node->ckpt_lcl_ref_cnt, __FILE__, __LINE__);

		goto agent_rsp;
	}

	/* ckpt is not present locally */
	/* if not present send to cpd,get the details... 
	   Send the CPD_EVT_ND2D_CKPT_CREATE evt to CPD to get the ckpt details 
	   Fill send_evt.info.cpd.info.ckpt_create */
	if (sinfo->stype == MDS_SENDTYPE_SNDRSP) {
		timeout = m_CPSV_CONVERT_SATIME_TEN_MILLI_SEC(evt->info.openReq.timeout);
		if ((timeout > CPSV_WAIT_TIME) || (timeout == 0))
			timeout = CPSV_WAIT_TIME;
	} else
		timeout = CPSV_WAIT_TIME;

	send_evt.type = CPSV_EVT_TYPE_CPD;
	send_evt.info.cpd.type = CPD_EVT_ND2D_CKPT_CREATE;

	send_evt.info.cpd.info.ckpt_create.ckpt_name = ckpt_name;
	send_evt.info.cpd.info.ckpt_create.attributes = evt->info.openReq.ckpt_attrib;
	send_evt.info.cpd.info.ckpt_create.ckpt_flags = evt->info.openReq.ckpt_flags;
	send_evt.info.cpd.info.ckpt_create.client_version = cl_node->version;

	/* send the request to the CPD */
	rc = cpnd_mds_msg_sync_send(cb, NCSMDS_SVC_ID_CPD, cb->cpd_mdest_id, &send_evt, &out_evt, CPSV_WAIT_TIME);

	if (rc != NCSCC_RC_SUCCESS) {
		m_LOG_CPND_CFCL(CPND_CKPT_OPEN_FAILURE, CPND_FC_API, NCSFL_SEV_ERROR, ckpt_name.value,
				client_hdl, __FILE__, __LINE__);

		if (rc == NCSCC_RC_REQ_TIMOUT) {

			node = (CPND_CPD_DEFERRED_REQ_NODE *)m_MMGR_ALLOC_CPND_CPD_DEFERRED_REQ_NODE;
			if (!node) {
				m_LOG_CPND_CFCL(CPND_CPD_DEFERRED_REQ_NODE_ALLOC_FAILED, CPND_FC_MEMFAIL,
						NCSFL_SEV_ERROR, ckpt_name.value, client_hdl, __FILE__, __LINE__);
				send_evt.info.cpa.info.openRsp.error = SA_AIS_ERR_NO_MEMORY;
				goto agent_rsp;
			}

			memset(node, '\0', sizeof(CPND_CPD_DEFERRED_REQ_NODE));
			node->evt.type = CPSV_EVT_TYPE_CPD;
			node->evt.info.cpd.type = CPD_EVT_ND2D_CKPT_DESTROY_BYNAME;
			node->evt.info.cpd.info.ckpt_destroy_byname.ckpt_name = ckpt_name;

			ncs_enqueue(&cb->cpnd_cpd_deferred_reqs_list, (void *)node);
		}

		send_evt.info.cpa.info.openRsp.error = SA_AIS_ERR_TRY_AGAIN;
		goto agent_rsp;
	}

	if (out_evt == NULL) {
		rc = NCSCC_RC_FAILURE;
		m_LOG_CPND_CL(CPND_CKPT_ALLOC_FAILED, CPND_FC_MEMFAIL, NCSFL_SEV_ERROR, __FILE__, __LINE__);
		send_evt.info.cpa.info.openRsp.error = SA_AIS_ERR_NO_MEMORY;
		goto agent_rsp;
	}
	if (out_evt && out_evt->info.cpnd.info.ckpt_info.error != SA_AIS_OK) {
		send_evt.info.cpa.info.openRsp.error = out_evt->info.cpnd.info.ckpt_info.error;
		goto agent_rsp;
	}

	switch (out_evt->info.cpnd.type) {

	case CPND_EVT_D2ND_CKPT_INFO:

		cp_node = m_MMGR_ALLOC_CPND_CKPT_NODE;
		if (cp_node == NULL) {
			m_LOG_CPND_CL(CPND_CKPT_ALLOC_FAILED, CPND_FC_MEMFAIL, NCSFL_SEV_ERROR, __FILE__, __LINE__);
			send_evt.info.cpa.info.openRsp.error = SA_AIS_ERR_NO_MEMORY;
			goto agent_rsp;
		}

		memset(cp_node, '\0', sizeof(CPND_CKPT_NODE));

		cp_node->clist = NULL;
		cp_node->cpnd_dest_list = NULL;
		cp_node->ckpt_name = ckpt_name;
		cp_node->create_attrib = out_evt->info.cpnd.info.ckpt_info.attributes;
		cp_node->open_flags = evt->info.openReq.ckpt_flags;
		cp_node->ckpt_id = out_evt->info.cpnd.info.ckpt_info.ckpt_id;

		if (out_evt->info.cpnd.info.ckpt_info.is_active_exists == TRUE) {
			cp_node->active_mds_dest = out_evt->info.cpnd.info.ckpt_info.active_dest;
			cp_node->is_active_exist = TRUE;
			send_evt.info.cpa.info.openRsp.is_active_exists = TRUE;
			send_evt.info.cpa.info.openRsp.active_dest = cp_node->active_mds_dest;
		}

		cp_node->offset = SHM_INIT;
		cp_node->is_close = FALSE;
		cp_node->is_unlink = FALSE;
		cp_node->is_restart = FALSE;
		cp_node->evt_bckup_q = NULL;
		cpnd_ckpt_client_add(cp_node, cl_node);
		cpnd_client_ckpt_info_add(cl_node, cp_node);

		cp_node->ckpt_lcl_ref_cnt++;

		{
			CPSV_CPND_DEST_INFO *tmp = NULL;
			uns32 dst_cnt = 0;

			/*  dst_cnt=out_evt->info.cpnd.info.ckpt_info.dest_cnt; */
			tmp = out_evt->info.cpnd.info.ckpt_info.dest_list;

			while (dst_cnt < out_evt->info.cpnd.info.ckpt_info.dest_cnt) {
				if (m_CPND_IS_LOCAL_NODE(&tmp[dst_cnt].dest, &cb->cpnd_mdest_id) != 0) {
					cpnd_ckpt_remote_cpnd_add(cp_node, tmp[dst_cnt].dest);
				}
				dst_cnt++;
			}

		}
		cp_node->cpnd_rep_create = out_evt->info.cpnd.info.ckpt_info.ckpt_rep_create;

		/* UPDATE THE GLOBAL SHARED MEMORY CKPT INFO */
		if (out_evt->info.cpnd.info.ckpt_info.ckpt_rep_create == TRUE) {

			rc = cpnd_ckpt_replica_create(cb, cp_node);
			if (rc == NCSCC_RC_FAILURE) {
				m_LOG_CPND_CFCL(CPND_CKPT_REP_CREATE_FAILED, CPND_FC_GENERIC, NCSFL_SEV_ERROR,
						ckpt_name.value, client_hdl, __FILE__, __LINE__);
				send_evt.info.cpa.info.openRsp.error = SA_AIS_ERR_NO_RESOURCES;
				goto ckpt_node_free_error;
			}

			/* UPDATE THE CHECKPOINT HEADER */
			rc = cpnd_ckpt_hdr_update(cp_node);
			if (rc == NCSCC_RC_FAILURE) {
				m_LOG_CPND_CFCL(CPND_CKPT_HDR_UPDATE_FAILED, CPND_FC_HDLN, NCSFL_SEV_ERROR,
						ckpt_name.value, client_hdl, __FILE__, __LINE__);
			}
		}

		rc = cpnd_restart_shm_ckpt_update(cb, cp_node, client_hdl);
		if (rc == NCSCC_RC_FAILURE) {
			m_LOG_CPND_CFCL(CPND_RESTART_SHM_CKPT_UPDATE_FAILED, CPND_FC_RESTART, NCSFL_SEV_ERROR,
					ckpt_name.value, client_hdl, __FILE__, __LINE__);
			send_evt.info.cpa.info.openRsp.error = SA_AIS_ERR_NO_RESOURCES;
			goto ckpt_shm_node_free_error;
		}

		/* add it to to tree db */
		if (cpnd_ckpt_node_add(cb, cp_node) == NCSCC_RC_FAILURE) {
			m_LOG_CPND_FFCL(CPND_CKPT_NODE_ADDITION_FAILED, CPND_FC_CKPTINFO, NCSFL_SEV_ERROR,
					cp_node->ckpt_id, client_hdl, __FILE__, __LINE__);
			send_evt.info.cpa.info.openRsp.error = SA_AIS_ERR_NO_RESOURCES;
			goto ckpt_shm_node_free_error;
		}

		if (out_evt->info.cpnd.info.ckpt_info.ckpt_rep_create == TRUE &&
		    cp_node->create_attrib.maxSections == 1) {

			SaCkptSectionIdT sec_id = SA_CKPT_DEFAULT_SECTION_ID;
			cpnd_ckpt_sec_add(cp_node, &sec_id, 0, 0);
		}
		if (out_evt) {
			cpnd_evt_destroy(out_evt);
			out_evt = NULL;
		}

		/* if not the first collocated replica, response will come from REP_ADD processing */

		if ((cp_node->create_attrib.creationFlags & SA_CKPT_CHECKPOINT_COLLOCATED) && cp_node->is_active_exist
		    && !m_NCS_MDS_DEST_EQUAL(&cp_node->active_mds_dest, &cb->cpnd_mdest_id)) {

			memset(&send_evt, '\0', sizeof(CPSV_EVT));

			send_evt.type = CPSV_EVT_TYPE_CPND;
			send_evt.info.cpnd.type = CPND_EVT_ND2ND_CKPT_SYNC_REQ;
			send_evt.info.cpnd.info.sync_req.ckpt_id = cp_node->ckpt_id;
			send_evt.info.cpnd.info.sync_req.lcl_ckpt_hdl = evt->info.openReq.lcl_ckpt_hdl;
			send_evt.info.cpnd.info.sync_req.cpa_sinfo = *sinfo;
			send_evt.info.cpnd.info.sync_req.is_ckpt_open = TRUE;
			if (sinfo->stype != MDS_SENDTYPE_SNDRSP)
				send_evt.info.cpnd.info.sync_req.invocation = evt->info.openReq.invocation;

			rc = cpnd_mds_msg_sync_send(cb, NCSMDS_SVC_ID_CPND, cp_node->active_mds_dest, &send_evt,
						    &out_evt, CPSV_WAIT_TIME);
			if (rc != NCSCC_RC_SUCCESS) {
				if (rc == NCSCC_RC_REQ_TIMOUT) {
					send_evt.info.cpa.info.openRsp.error = SA_AIS_ERR_TIMEOUT;
					m_LOG_CPND_CFFFCL(CPND_REMOTE_TO_ACTIVE_MDS_SEND_FAIL, CPND_FC_MDSFAIL,
							  NCSFL_SEV_ERROR, ckpt_name.value, cb->cpnd_mdest_id,
							  cp_node->active_mds_dest, cp_node->ckpt_id, __FILE__,
							  __LINE__);

				} else {
					send_evt.info.cpa.info.openRsp.error = SA_AIS_ERR_TRY_AGAIN;
					m_LOG_CPND_CFFFCL(CPND_REMOTE_TO_ACTIVE_MDS_SEND_FAIL, CPND_FC_MDSFAIL,
							  NCSFL_SEV_ERROR, ckpt_name.value, cb->cpnd_mdest_id,
							  cp_node->active_mds_dest, cp_node->ckpt_id, __FILE__,
							  __LINE__);

				}
				goto agent_rsp;
			} else {
				if ((out_evt) && (out_evt->info.cpnd.error != SA_AIS_OK)) {
					send_evt.info.cpa.info.openRsp.error = out_evt->info.cpnd.error;
					goto agent_rsp;
				} else if ((out_evt) && (out_evt->info.cpnd.error == SA_AIS_OK) &&
					   (!out_evt->info.cpnd.info.ckpt_nd2nd_sync.num_of_elmts)) {
					goto agent_rsp2;
				}
			}
			cp_node->open_active_sync_tmr.type = CPND_TMR_OPEN_ACTIVE_SYNC;
			cp_node->open_active_sync_tmr.uarg = cb->cpnd_cb_hdl_id;
			cp_node->open_active_sync_tmr.ckpt_id = cp_node->ckpt_id;
			cp_node->open_active_sync_tmr.sinfo = *sinfo;
			cp_node->open_active_sync_tmr.invocation = evt->info.openReq.invocation;
			cp_node->open_active_sync_tmr.lcl_ckpt_hdl = evt->info.openReq.lcl_ckpt_hdl;
			cp_node->open_active_sync_tmr.is_active_sync_err = FALSE;
			if (cpnd_tmr_start
			    (&cp_node->open_active_sync_tmr,
			     CPND_WAIT_TIME(cp_node->create_attrib.checkpointSize)) == NCSCC_RC_SUCCESS)
				m_LOG_CPND_FCL(CPND_OPEN_ACTIVE_SYNC_START_TMR_SUCCESS, CPND_FC_CKPTINFO,
					       NCSFL_SEV_NOTICE, cp_node->ckpt_id, __FILE__, __LINE__);
			if (out_evt) {
				cpnd_evt_destroy(out_evt);
				out_evt = NULL;
			}
	            goto agent_rsp2;	
              }

 agent_rsp2:
		send_evt.info.cpa.info.openRsp.gbl_ckpt_hdl = cp_node->ckpt_id;
		send_evt.info.cpa.info.openRsp.addr = cp_node->replica_info.open.info.open.o_addr;
		send_evt.info.cpa.info.openRsp.creation_attr = cp_node->create_attrib;
		send_evt.info.cpa.info.openRsp.error = SA_AIS_OK;
		if (cp_node->is_active_exist) {
			send_evt.info.cpa.info.openRsp.is_active_exists = TRUE;
			send_evt.info.cpa.info.openRsp.active_dest = cp_node->active_mds_dest;
		}
		if (send_evt.info.cpa.info.openRsp.error == SA_AIS_OK) {
			m_LOG_CPND_CFFFCL(CPND_CKPT_OPEN_SUCCESS, CPND_FC_API, NCSFL_SEV_INFO, ckpt_name.value,
					  client_hdl, cp_node->ckpt_id, cp_node->active_mds_dest, __FILE__, __LINE__);
		}
		goto agent_rsp;

	default:
		m_LOG_CPND_LCL(CPND_EVT_UNKNOWN, CPND_FC_EVT, NCSFL_SEV_ERROR, out_evt->info.cpnd.type, __FILE__,
			       __LINE__);
		break;
	}

	send_evt.info.cpa.info.openRsp.error = SA_AIS_ERR_LIBRARY;
	m_LOG_CPND_CFCL(CPND_CKPT_OPEN_FAILURE, CPND_FC_EVT, NCSFL_SEV_ERROR, ckpt_name.value,
			client_hdl, __FILE__, __LINE__);
	goto agent_rsp;

 ckpt_shm_node_free_error:
	cpnd_ckpt_replica_destroy(cb, cp_node, &error);

 ckpt_node_free_error:
	cpnd_ckpt_client_del(cp_node, cl_node);
	cpnd_client_ckpt_info_del(cl_node, cp_node);

	if (cp_node->ckpt_lcl_ref_cnt)
		cp_node->ckpt_lcl_ref_cnt--;

	{
		CPSV_CPND_DEST_INFO *tmp, *free_tmp;
		tmp = cp_node->cpnd_dest_list;
		while (tmp != NULL) {
			free_tmp = tmp;
			tmp = tmp->next;
			m_MMGR_FREE_CPND_DEST_INFO(free_tmp);
		}
	}
	m_MMGR_FREE_CPND_CKPT_NODE(cp_node);

 agent_rsp:
	send_evt.type = CPSV_EVT_TYPE_CPA;
	send_evt.info.cpa.type = CPA_EVT_ND2A_CKPT_OPEN_RSP;
	send_evt.info.cpa.info.openRsp.lcl_ckpt_hdl = evt->info.openReq.lcl_ckpt_hdl;
	send_evt.info.cpa.info.openRsp.sync_async = evt->info.openReq.sync_async;


        if(!send_evt.info.cpa.info.openRsp.sync_async)
           send_evt.info.cpa.info.openRsp.invocation = evt->info.openReq.invocation;
	
	rc = cpnd_mds_msg_send(cb, sinfo->to_svc, sinfo->dest, &send_evt);

	if (out_evt)
		cpnd_evt_destroy(out_evt);

	return rc;
}

/****************************************************************************
 * Name          : cpnd_evt_proc_ckpt_close
 *
 * Description   : Function to process the saCkptCheckpointClose from 
 *                 Applications. 
 *
 * Arguments     : CPND_CB *cb - CPND CB pointer
 *                 CPSV_EVT *evt - Received Event structure
 *                 CPSV_SEND_INFO *sinfo - Sender MDS information.
 *
 * Return Values : NCSCC_RC_SUCCESS/Error.
 *
 * Notes         : None.
 *****************************************************************************/
static uns32 cpnd_evt_proc_ckpt_close(CPND_CB *cb, CPND_EVT *evt, CPSV_SEND_INFO *sinfo)
{
	CPSV_EVT send_evt;
	uns32 rc = NCSCC_RC_SUCCESS;
	CPND_CKPT_CLIENT_NODE *cl_node = NULL;
	CPND_CKPT_NODE *cp_node = NULL;
	SaAisErrorT error;

	memset(&send_evt, '\0', sizeof(CPSV_EVT));

	if (!cpnd_is_cpd_up(cb)) {
		send_evt.info.cpa.info.closeRsp.error = SA_AIS_ERR_TRY_AGAIN;
		goto agent_rsp;
	}

	cpnd_client_node_get(cb, evt->info.closeReq.client_hdl, &cl_node);
	if (cl_node == NULL) {
		m_LOG_CPND_FFCL(CPND_CLIENT_NODE_GET_FAILED, CPND_FC_API, NCSFL_SEV_ERROR,
				evt->info.closeReq.client_hdl, evt->info.closeReq.ckpt_id, __FILE__, __LINE__);
		send_evt.info.cpa.info.closeRsp.error = SA_AIS_ERR_LIBRARY;
		rc = NCSCC_RC_FAILURE;
		goto agent_rsp;
	}

	cpnd_ckpt_node_get(cb, evt->info.closeReq.ckpt_id, &cp_node);
	if (cp_node == NULL) {
		m_LOG_CPND_FFCL(CPND_CKPT_NODE_GET_FAILED, CPND_FC_API, NCSFL_SEV_ERROR,
				evt->info.closeReq.client_hdl, evt->info.closeReq.ckpt_id, __FILE__, __LINE__);
		send_evt.info.cpa.info.closeRsp.error = SA_AIS_ERR_NOT_EXIST;
		rc = NCSCC_RC_FAILURE;
		goto agent_rsp;
	}

	/* Read the out_evt.info.cpnd.info.cl_ack */
	cpnd_ckpt_client_del(cp_node, cl_node);
	cpnd_client_ckpt_info_del(cl_node, cp_node);

	if (cp_node->ckpt_lcl_ref_cnt)
		cp_node->ckpt_lcl_ref_cnt--;

	/* reset the client info */
	cpnd_restart_client_reset(cb, cp_node, cl_node);

	rc = cpnd_send_ckpt_usr_info_to_cpd(cb, cp_node, cp_node->open_flags, CPSV_USR_INFO_CKPT_CLOSE);
	if (rc != NCSCC_RC_SUCCESS) {
		m_LOG_CPND_FFCL(CPND_MDS_SEND_FAIL, CPND_FC_HDLN, NCSFL_SEV_ERROR,
				evt->info.closeReq.client_hdl, evt->info.closeReq.ckpt_id, __FILE__, __LINE__);
		/* For now go ahread and close the queue, we need to think of a better way to handle this */
	}
	m_LOG_CPND_FFLCL(CPND_CLIENT_CKPT_CLOSE_SUCCESS, CPND_FC_API, NCSFL_SEV_INFO,
			 cl_node->ckpt_app_hdl, cp_node->ckpt_id, cp_node->ckpt_lcl_ref_cnt, __FILE__, __LINE__);

	rc = cpnd_ckpt_replica_close(cb, cp_node, &error);
	if (rc == NCSCC_RC_FAILURE) {
		m_LOG_CPND_FFCL(CPND_CKPT_REPLICA_CLOSE_FAILED, CPND_FC_GENERIC, NCSFL_SEV_ERROR,
				evt->info.closeReq.client_hdl, cp_node->ckpt_id, __FILE__, __LINE__);

		send_evt.info.cpa.info.closeRsp.error = error;
		goto agent_rsp;
	}

	send_evt.info.cpa.info.closeRsp.error = SA_AIS_OK;

 agent_rsp:
	send_evt.type = CPSV_EVT_TYPE_CPA;
	send_evt.info.cpa.type = CPA_EVT_ND2A_CKPT_CLOSE_RSP;
	rc = cpnd_mds_send_rsp(cb, sinfo, &send_evt);

	return rc;
}

/****************************************************************************
 * Name          : cpnd_evt_proc_ckpt_unlink
 *
 * Description   : Function to process the saCkptCheckpointUnlink from 
 *                 Applications. 
 *
 * Arguments     : CPND_CB *cb - CPND CB pointer
 *                 CPSV_EVT *evt - Received Event structure
 *                 CPSV_SEND_INFO *sinfo - Sender MDS information.
 *
 * Return Values : NCSCC_RC_SUCCESS/Error.
 *
 * Notes         : None.
 *****************************************************************************/
static uns32 cpnd_evt_proc_ckpt_unlink(CPND_CB *cb, CPND_EVT *evt, CPSV_SEND_INFO *sinfo)
{
	CPSV_EVT send_evt, *out_evt = NULL;
	CPND_CKPT_NODE *cp_node = NULL;
	uns32 rc = NCSCC_RC_SUCCESS;
	CPND_CPD_DEFERRED_REQ_NODE *node = NULL;

	memset(&send_evt, '\0', sizeof(CPSV_EVT));

	if (!cpnd_is_cpd_up(cb)) {
		send_evt.info.cpa.info.ulinkRsp.error = SA_AIS_ERR_TRY_AGAIN;
		goto agent_rsp;
	}

	send_evt.type = CPSV_EVT_TYPE_CPD;
	send_evt.info.cpd.type = CPD_EVT_ND2D_CKPT_UNLINK;
	send_evt.info.cpd.info.ckpt_ulink.ckpt_name = evt->info.ulinkReq.ckpt_name;

	rc = cpnd_mds_msg_sync_send(cb, NCSMDS_SVC_ID_CPD, cb->cpd_mdest_id, &send_evt, &out_evt, CPSV_WAIT_TIME);

	if (rc != NCSCC_RC_SUCCESS) {
		m_LOG_CPND_LCL(CPND_MDS_SEND_FAIL, CPND_FC_HDLN, NCSFL_SEV_ERROR, rc, __FILE__, __LINE__);

		if (rc == NCSCC_RC_REQ_TIMOUT) {

			node = (CPND_CPD_DEFERRED_REQ_NODE *)m_MMGR_ALLOC_CPND_CPD_DEFERRED_REQ_NODE;
			if (!node) {
				m_LOG_CPND_CL(CPND_CPD_DEFERRED_REQ_NODE_ALLOC_FAILED, CPND_FC_MEMFAIL, NCSFL_SEV_ERROR,
					      __FILE__, __LINE__);
				send_evt.info.cpa.info.ulinkRsp.error = SA_AIS_ERR_NO_MEMORY;
				goto agent_rsp;
			}

			memset(node, '\0', sizeof(CPND_CPD_DEFERRED_REQ_NODE));
			node->evt = send_evt;

			ncs_enqueue(&cb->cpnd_cpd_deferred_reqs_list, (void *)node);
		} else {
			send_evt.info.cpa.info.ulinkRsp.error = SA_AIS_ERR_TRY_AGAIN;
			goto agent_rsp;
		}
	} else {
		if (out_evt && out_evt->info.cpnd.info.ulink_ack.error != SA_AIS_OK) {
			send_evt.info.cpa.info.ulinkRsp.error = out_evt->info.cpnd.info.ulink_ack.error;
			goto agent_rsp;
		}
	}

	cp_node = cpnd_ckpt_node_find_by_name(cb, evt->info.ulinkReq.ckpt_name);
	if (cp_node == NULL) {
		if (out_evt != NULL)
			send_evt.info.cpa.info.ulinkRsp.error = out_evt->info.cpnd.info.ulink_ack.error;
		else
			send_evt.info.cpa.info.ulinkRsp.error = SA_AIS_ERR_TIMEOUT;

		m_LOG_CPND_CCL(CPND_PROC_CKPT_UNLINK_SUCCESS, CPND_FC_API, NCSFL_SEV_INFO,
			       evt->info.ulinkReq.ckpt_name.value, __FILE__, __LINE__);
		goto agent_rsp;
	}
	cp_node->cpa_sinfo = *(sinfo);
	cp_node->cpa_sinfo_flag = TRUE;

	if (out_evt)
		cpnd_evt_destroy(out_evt);

	return rc;

 agent_rsp:
	send_evt.type = CPSV_EVT_TYPE_CPA;
	send_evt.info.cpa.type = CPA_EVT_ND2A_CKPT_UNLINK_RSP;
	rc = cpnd_mds_send_rsp(cb, sinfo, &send_evt);
	if (out_evt)
		cpnd_evt_destroy(out_evt);
	return rc;
}

/****************************************************************************
 * Name          : cpnd_evt_proc_ckpt_unlink_info
 *
 * Description   : Function to process check point unlink 
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
static uns32 cpnd_evt_proc_ckpt_unlink_info(CPND_CB *cb, CPND_EVT *evt, CPSV_SEND_INFO *sinfo)
{
	uns32 rc = NCSCC_RC_SUCCESS;
	CPND_CKPT_NODE *cp_node = NULL;
	SaAisErrorT error;
	CPSV_SEND_INFO sinfo_cpa;
	CPSV_EVT send_evt;
	NCS_BOOL sinfo_cpa_flag = FALSE;

	memset(&send_evt, '\0', sizeof(CPSV_EVT));
	cpnd_ckpt_node_get(cb, evt->info.ckpt_ulink.ckpt_id, &cp_node);
	if (cp_node == NULL) {
		m_LOG_CPND_FCL(CPND_CKPT_NODE_GET_FAILED, CPND_FC_API, NCSFL_SEV_INFO, evt->info.ckpt_ulink.ckpt_id,
			       __FILE__, __LINE__);
		rc = NCSCC_RC_FAILURE;
		send_evt.info.cpa.info.ulinkRsp.error = SA_AIS_ERR_NOT_EXIST;
		goto agent_rsp;
	}

	sinfo_cpa = cp_node->cpa_sinfo;
	sinfo_cpa_flag = cp_node->cpa_sinfo_flag;
	if (cp_node->is_close == TRUE) {
		send_evt.info.cpa.info.ulinkRsp.error = SA_AIS_OK;
		/* check timer is present,if yes...stop the timer and destroy shm_info and the node */
		cpnd_tmr_stop(&cp_node->ret_tmr);

		if (!m_CPND_IS_COLLOCATED_ATTR_SET(cp_node->create_attrib.creationFlags)) {
			if (cpnd_is_noncollocated_replica_present_on_payload(cb, cp_node)) {
				rc = NCSCC_RC_SUCCESS;
				goto agent_rsp;
			}
		}

		rc = cpnd_ckpt_replica_destroy(cb, cp_node, &error);
		if (rc == NCSCC_RC_FAILURE) {

			m_LOG_CPND_FLCL(CPND_CKPT_REPLICA_DESTROY_FAILED, CPND_FC_GENERIC, NCSFL_SEV_ERROR,
					cp_node->ckpt_id, error, __FILE__, __LINE__);
			goto agent_rsp;
		}
		m_LOG_CPND_FCL(CPND_CKPT_REPLICA_DESTROY_SUCCESS, CPND_FC_GENERIC, NCSFL_SEV_INFO, cp_node->ckpt_id,
			       __FILE__, __LINE__);

		cpnd_restart_shm_ckpt_free(cb, cp_node);
		cpnd_ckpt_node_destroy(cb, cp_node);
	} else {

		cpnd_restart_ckpt_name_length_reset(cb, cp_node);

		cp_node->is_unlink = TRUE;
		cp_node->ckpt_name.length = 0;

		if (cp_node->cpnd_rep_create) {
			rc = cpnd_ckpt_hdr_update(cp_node);

		}
		m_LOG_CPND_FCL(CPND_PROC_CKPT_UNLINK_SET, CPND_FC_CKPTINFO, NCSFL_SEV_INFO,
			       cp_node->ckpt_id, __FILE__, __LINE__);

		send_evt.info.cpa.info.ulinkRsp.error = SA_AIS_OK;
	}

 agent_rsp:

	if (sinfo_cpa_flag == 1) {
		send_evt.type = CPSV_EVT_TYPE_CPA;
		send_evt.info.cpa.type = CPA_EVT_ND2A_CKPT_UNLINK_RSP;
		rc = cpnd_mds_send_rsp(cb, &sinfo_cpa, &send_evt);

	}

	return rc;
}

/******************************************************************************
 * Name          : cpsv_cpnd_restart
 *
 * Description   : Set the is_restart flag to true
 *
 * Arguments   : CPSV_SEND_INFO - send info 
 *
 * Return Values : Success / Error
 *****************************************************************************/
static uns32 cpsv_cpnd_restart(CPND_CB *cb, CPND_EVT *evt, CPSV_SEND_INFO *sinfo)
{
	uns32 rc = NCSCC_RC_SUCCESS;
	CPND_CKPT_NODE *cp_node = NULL;

	cpnd_ckpt_node_get(cb, evt->info.cpnd_restart.ckpt_id, &cp_node);

	if (cp_node == NULL) {
		m_LOG_CPND_FCL(CPND_CKPT_NODE_GET_FAILED, CPND_FC_API, NCSFL_SEV_INFO, evt->info.cpnd_restart.ckpt_id,
			       __FILE__, __LINE__);
		return NCSCC_RC_FAILURE;
	}
	cp_node->is_restart = TRUE;
	return rc;
}

/************************************************************************************
 * Name        : cpsv_cpnd_active_replace
 *
 * Description : Active ND went down, and came up , now it will have diff. dest id 
                 this is has to be replaced in the dest_list of the other ND
 *
 * Return Values : Success / Error

************************************************************************************/
static uns32 cpsv_cpnd_active_replace(CPND_CKPT_NODE *cp_node, CPND_EVT *evt)
{
	uns32 rc = NCSCC_RC_SUCCESS;
	CPSV_CPND_DEST_INFO *cpnd_mdest_trav = cp_node->cpnd_dest_list;

	while (cpnd_mdest_trav) {
		if (m_CPND_IS_LOCAL_NODE(&cpnd_mdest_trav->dest, &cp_node->active_mds_dest) == 0) {
			cpnd_mdest_trav->dest = evt->info.cpnd_restart_done.mds_dest;
			break;
		} else
			cpnd_mdest_trav = cpnd_mdest_trav->next;
	}
	return rc;
}

/************************************************************************************
 * Name          : cpsv_cpnd_restart_done
 *
 * Description   : Reset the is_restart flag and set the active mds_dest
 *
 * Arguments     : CPND_EVT - cpnd evt , cb , send_info
 *
 * Return Values : Success / Error
 ***********************************************************************************/
static uns32 cpsv_cpnd_restart_done(CPND_CB *cb, CPND_EVT *evt, CPSV_SEND_INFO *sinfo)
{
	uns32 rc = NCSCC_RC_SUCCESS;
	CPND_CKPT_NODE *cp_node = NULL;
	CPSV_EVT send_evt;
	CPSV_CPND_DEST_INFO *tmp = NULL;
	uns32 dst_cnt = 0;

	memset(&send_evt, '\0', sizeof(CPSV_EVT));
	cpnd_ckpt_node_get(cb, evt->info.cpnd_restart_done.ckpt_id, &cp_node);
	if (cp_node == NULL) {
		m_LOG_CPND_FCL(CPND_CKPT_NODE_GET_FAILED, CPND_FC_API, NCSFL_SEV_INFO,
			       evt->info.cpnd_restart_done.ckpt_id, __FILE__, __LINE__);
		return NCSCC_RC_FAILURE;
	}
	cp_node->is_restart = FALSE;

	/* If active restarted ,bcast came to him : then update its dest list */
	if (m_CPND_IS_LOCAL_NODE(&evt->info.cpnd_restart_done.mds_dest, &cb->cpnd_mdest_id) == 0) {
		if (evt->info.cpnd_restart_done.dest_list) {
			tmp = evt->info.cpnd_restart_done.dest_list;

			while (dst_cnt < evt->info.cpnd_restart_done.dest_cnt) {
				if (m_CPND_IS_LOCAL_NODE(&tmp[dst_cnt].dest, &cb->cpnd_mdest_id) != 0) {
					cpnd_ckpt_remote_cpnd_add(cp_node, tmp[dst_cnt].dest);
				}
				dst_cnt++;
			}
		}

		cp_node->active_mds_dest = evt->info.cpnd_restart_done.active_dest;

	}
	/* Active restarted , bcast came to non-active : then update with new dest */
	else {
		cpsv_cpnd_active_replace(cp_node, evt);
	}

	cp_node->active_mds_dest = evt->info.cpnd_restart_done.mds_dest;
	return rc;
}

/****************************************************************************
 * Name          : cpnd_evt_proc_ckpt_rdset
 *
 * Description   : Function to process the saCkptCheckpointRetentionDurationSet  
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
static uns32 cpnd_evt_proc_ckpt_rdset(CPND_CB *cb, CPND_EVT *evt, CPSV_SEND_INFO *sinfo)
{
	CPSV_EVT send_evt, *out_evt = NULL;
	uns32 rc = NCSCC_RC_SUCCESS;
	CPND_CKPT_NODE *cp_node = NULL;
	CPND_CPD_DEFERRED_REQ_NODE *node = NULL;

	memset(&send_evt, '\0', sizeof(CPSV_EVT));

	if (!cpnd_is_cpd_up(cb)) {
		send_evt.info.cpa.info.rdsetRsp.error = SA_AIS_ERR_TRY_AGAIN;
		goto agent_rsp;
	}

	cpnd_ckpt_node_get(cb, evt->info.rdsetReq.ckpt_id, &cp_node);
	if (cp_node == NULL) {
		m_LOG_CPND_FCL(CPND_CKPT_NODE_GET_FAILED, CPND_FC_API, NCSFL_SEV_ERROR,
			       evt->info.rdsetReq.ckpt_id, __FILE__, __LINE__);
		send_evt.info.cpa.info.rdsetRsp.error = SA_AIS_ERR_NOT_EXIST;
		goto agent_rsp;
	}
	if (cp_node->is_restart == TRUE) {
		send_evt.type = CPSV_EVT_TYPE_CPA;
		send_evt.info.cpa.type = CPA_EVT_ND2A_CKPT_RDSET_RSP;
		send_evt.info.cpa.info.rdsetRsp.error = SA_AIS_ERR_TRY_AGAIN;
		goto agent_rsp;
	}
	if (cp_node->is_unlink == TRUE) {
		send_evt.type = CPSV_EVT_TYPE_CPA;
		send_evt.info.cpa.type = CPA_EVT_ND2A_CKPT_RDSET_RSP;
		send_evt.info.cpa.info.rdsetRsp.error = SA_AIS_ERR_BAD_OPERATION;
		goto agent_rsp;
	}

	send_evt.type = CPSV_EVT_TYPE_CPD;
	send_evt.info.cpd.type = CPD_EVT_ND2D_CKPT_RDSET;
	send_evt.info.cpd.info.rd_set.ckpt_id = evt->info.rdsetReq.ckpt_id;
	send_evt.info.cpd.info.rd_set.reten_time = evt->info.rdsetReq.reten_time;

	rc = cpnd_mds_msg_sync_send(cb, NCSMDS_SVC_ID_CPD, cb->cpd_mdest_id, &send_evt, &out_evt, CPSV_WAIT_TIME);

	if (rc != NCSCC_RC_SUCCESS) {
		m_LOG_CPND_LCL(CPND_MDS_SEND_FAIL, CPND_FC_HDLN, NCSFL_SEV_ERROR, rc, __FILE__, __LINE__);
		if (rc == NCSCC_RC_REQ_TIMOUT) {

			node = (CPND_CPD_DEFERRED_REQ_NODE *)m_MMGR_ALLOC_CPND_CPD_DEFERRED_REQ_NODE;
			if (!node) {
				m_LOG_CPND_CL(CPND_CPD_DEFERRED_REQ_NODE_ALLOC_FAILED, CPND_FC_MEMFAIL, NCSFL_SEV_ERROR,
					      __FILE__, __LINE__);
				send_evt.info.cpa.info.rdsetRsp.error = SA_AIS_ERR_NO_MEMORY;
				goto out_evt_free;
			}

			memset(node, '\0', sizeof(CPND_CPD_DEFERRED_REQ_NODE));
			node->evt = send_evt;

			ncs_enqueue(&cb->cpnd_cpd_deferred_reqs_list, (void *)node);
		} else {
			send_evt.info.cpa.info.rdsetRsp.error = SA_AIS_ERR_TRY_AGAIN;
			goto agent_rsp;
		}
	} else {
		if (out_evt && out_evt->info.cpnd.info.rdset_ack.error != SA_AIS_OK) {
			send_evt.info.cpa.info.rdsetRsp.error = out_evt->info.cpnd.info.rdset_ack.error;
			goto out_evt_free;
		}
	}

	cp_node->create_attrib.retentionDuration = evt->info.rdsetReq.reten_time;
	cp_node->is_rdset = TRUE;
	/*Non-collocated which does not have replica */
	if (cp_node->cpnd_rep_create)
		cpnd_ckpt_hdr_update(cp_node);

	send_evt.info.cpa.info.rdsetRsp.error = SA_AIS_OK;

 out_evt_free:
	send_evt.type = CPSV_EVT_TYPE_CPA;
	send_evt.info.cpa.type = CPA_EVT_ND2A_CKPT_RDSET_RSP;

/* need to free out_evt */
 agent_rsp:
	send_evt.type = CPSV_EVT_TYPE_CPA;
	send_evt.info.cpa.type = CPA_EVT_ND2A_CKPT_RDSET_RSP;
	rc = cpnd_mds_send_rsp(cb, sinfo, &send_evt);

	if (out_evt)
		cpnd_evt_destroy(out_evt);

	return rc;
}

/****************************************************************************
 * Name          : cpnd_evt_proc_ckpt_arep_set
 *
 * Description   : Function to process the saCkptActiveReplicaSet  
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
static uns32 cpnd_evt_proc_ckpt_arep_set(CPND_CB *cb, CPND_EVT *evt, CPSV_SEND_INFO *sinfo)
{
	CPSV_EVT send_evt, *out_evt = NULL;
	CPND_CKPT_NODE *cp_node = NULL;
	uns32 rc = NCSCC_RC_SUCCESS;
	CPND_CPD_DEFERRED_REQ_NODE *node = NULL;

	memset(&send_evt, '\0', sizeof(CPSV_EVT));

	if (!cpnd_is_cpd_up(cb)) {
		send_evt.info.cpa.info.arsetRsp.error = SA_AIS_ERR_TRY_AGAIN;
		goto agent_rsp;
	}

	cpnd_ckpt_node_get(cb, evt->info.arsetReq.ckpt_id, &cp_node);
	if (cp_node == NULL) {
		m_LOG_CPND_FCL(CPND_CKPT_NODE_GET_FAILED, CPND_FC_API, NCSFL_SEV_ERROR,
			       evt->info.arsetReq.ckpt_id, __FILE__, __LINE__);
		send_evt.info.cpa.info.arsetRsp.error = SA_AIS_ERR_NOT_EXIST;
		goto agent_rsp;
	}

	if (cp_node->is_restart) {
		send_evt.info.cpa.info.arsetRsp.error = SA_AIS_ERR_TRY_AGAIN;
		goto agent_rsp;
	}

	cp_node->active_mds_dest = cb->cpnd_mdest_id;
	cp_node->is_active_exist = TRUE;

	send_evt.type = CPSV_EVT_TYPE_CPD;
	send_evt.info.cpd.type = CPD_EVT_ND2D_ACTIVE_SET;
	send_evt.info.cpd.info.arep_set.ckpt_id = evt->info.arsetReq.ckpt_id;
	send_evt.info.cpd.info.arep_set.mds_dest = cb->cpnd_mdest_id;

	/* ###TBD sync up is missing with old active if now this fellow is becoming active. */
	rc = cpnd_mds_msg_sync_send(cb, NCSMDS_SVC_ID_CPD, cb->cpd_mdest_id, &send_evt, &out_evt, CPSV_WAIT_TIME);

	if (rc != NCSCC_RC_SUCCESS) {
		m_LOG_CPND_LCL(CPND_MDS_SEND_FAIL, CPND_FC_HDLN, NCSFL_SEV_ERROR, rc, __FILE__, __LINE__);
		if (rc == NCSCC_RC_REQ_TIMOUT) {

			node = (CPND_CPD_DEFERRED_REQ_NODE *)m_MMGR_ALLOC_CPND_CPD_DEFERRED_REQ_NODE;
			if (!node) {
				m_LOG_CPND_CL(CPND_CPD_DEFERRED_REQ_NODE_ALLOC_FAILED, CPND_FC_MEMFAIL, NCSFL_SEV_ERROR,
					      __FILE__, __LINE__);
				send_evt.info.cpa.info.rdsetRsp.error = SA_AIS_ERR_NO_MEMORY;
				goto agent_rsp;
			}

			memset(node, '\0', sizeof(CPND_CPD_DEFERRED_REQ_NODE));
			node->evt = send_evt;

			ncs_enqueue(&cb->cpnd_cpd_deferred_reqs_list, (void *)node);
		} else {
			send_evt.info.cpa.info.arsetRsp.error = SA_AIS_ERR_TRY_AGAIN;
			goto agent_rsp;
		}
	} else {
		if (out_evt && out_evt->info.cpnd.info.arep_ack.error != SA_AIS_OK) {
			send_evt.info.cpa.info.arsetRsp.error = out_evt->info.cpnd.info.arep_ack.error;
			goto agent_rsp;
		}
	}

	rc = cpnd_ckpt_hdr_update(cp_node);

	send_evt.info.cpa.info.arsetRsp.error = SA_AIS_OK;
	goto agent_rsp;

 agent_rsp:
	send_evt.type = CPSV_EVT_TYPE_CPA;
	send_evt.info.cpa.type = CPA_EVT_ND2A_CKPT_AREP_SET_RSP;
	rc = cpnd_mds_send_rsp(cb, sinfo, &send_evt);
	if (out_evt)
		cpnd_evt_destroy(out_evt);

	return rc;

}

/****************************************************************************
 * Name          : cpnd_evt_proc_ckpt_status_get
 *
 * Description   : Function to process the saCkptCheckpointStatusGet  
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
static uns32 cpnd_evt_proc_ckpt_status_get(CPND_CB *cb, CPND_EVT *evt, CPSV_SEND_INFO *sinfo)
{
	CPSV_EVT send_evt;
	CPND_CKPT_NODE *cp_node = NULL;
	uns32 rc = NCSCC_RC_SUCCESS;

	memset(&send_evt, '\0', sizeof(CPSV_EVT));

	/* get cp_node from ckpt_info_db */

	cpnd_ckpt_node_get(cb, evt->info.statReq.ckpt_id, &cp_node);
	if (cp_node == NULL) {
		m_LOG_CPND_FCL(CPND_CKPT_NODE_GET_FAILED, CPND_FC_API, NCSFL_SEV_ERROR,
			       evt->info.statReq.ckpt_id, __FILE__, __LINE__);
		send_evt.info.cpa.info.status.error = SA_AIS_ERR_NOT_EXIST;
		goto agent_rsp;
	}

	/* CPND REDUNDANCY */
	if (TRUE == cp_node->is_restart) {
		send_evt.info.cpa.info.status.error = SA_AIS_ERR_TRY_AGAIN;
		send_evt.info.cpa.info.status.ckpt_id = cp_node->ckpt_id;
		goto agent_rsp;
	}

	/* check for active replica presence */
	if (cp_node->is_active_exist != TRUE) {
		send_evt.info.cpa.info.status.error = SA_AIS_ERR_NOT_EXIST;
		goto agent_rsp;
	}

	if ((m_CPND_IS_LOCAL_NODE(&cp_node->active_mds_dest, &cb->cpnd_mdest_id) == 0) ||
	    ((m_CPND_IS_ALL_REPLICA_ATTR_SET(cp_node->create_attrib.creationFlags) == TRUE)
	     && (m_CPND_IS_COLLOCATED_ATTR_SET(cp_node->create_attrib.creationFlags) == TRUE))) {

		send_evt.info.cpa.info.status.error = SA_AIS_OK;
		send_evt.info.cpa.info.status.ckpt_id = cp_node->ckpt_id;
		send_evt.info.cpa.info.status.status.checkpointCreationAttributes = cp_node->create_attrib;
		send_evt.info.cpa.info.status.status.memoryUsed = cp_node->replica_info.mem_used;
		send_evt.info.cpa.info.status.status.numberOfSections = cp_node->replica_info.n_secs;
		goto agent_rsp;

	} else {
		send_evt.info.cpa.info.status.error = SA_AIS_ERR_TRY_AGAIN;
		send_evt.info.cpa.info.status.ckpt_id = cp_node->ckpt_id;
		goto agent_rsp;
	}

 agent_rsp:

	send_evt.type = CPSV_EVT_TYPE_CPA;
	send_evt.info.cpa.type = CPA_EVT_ND2A_CKPT_STATUS;
	rc = cpnd_mds_send_rsp(cb, sinfo, &send_evt);
	return rc;
}

/****************************************************************************
 * Name          : cpnd_evt_proc_ckpt_active_set
 *
 * Description   : Function to process active change from Director
 *
 * Arguments     : CPND_CB *cb - CPND CB pointer
 *                 CPSV_EVT *evt - Received Event structure
 *                 CPSV_SEND_INFO *sinfo - Sender MDS information.
 *
 * Return Values : NCSCC_RC_SUCCESS/Error.
 *
 * Notes         : None.
 *****************************************************************************/
static uns32 cpnd_evt_proc_ckpt_active_set(CPND_CB *cb, CPND_EVT *evt, CPSV_SEND_INFO *sinfo)
{

	CPND_CKPT_NODE *cp_node = NULL;
	MDS_DEST mds_dest;
	uns32 rc = NCSCC_RC_SUCCESS;
	CPSV_EVT send_evt;

	memset(&mds_dest, '\0', sizeof(MDS_DEST));
	m_LOG_CPND_FFCL(CPND_ACTIVE_REP_SET_SUCCESS, CPND_FC_HDLN, NCSFL_SEV_INFO, evt->info.active_set.ckpt_id,
			evt->info.active_set.mds_dest, __FILE__, __LINE__);

	/* get cp_node from ckpt_info_db */
	cpnd_ckpt_node_get(cb, evt->info.active_set.ckpt_id, &cp_node);
	if (cp_node == NULL) {
		m_LOG_CPND_FCL(CPND_CKPT_NODE_GET_FAILED, CPND_FC_API, NCSFL_SEV_INFO,
			       evt->info.active_set.ckpt_id, __FILE__, __LINE__);
		return NCSCC_RC_FAILURE;

	}
	if (m_CPND_IS_LOCAL_NODE(&evt->info.active_set.mds_dest, &mds_dest) == 0) {
		cp_node->is_active_exist = FALSE;
	} else {
		cp_node->is_active_exist = TRUE;
		cp_node->active_mds_dest = evt->info.active_set.mds_dest;
		memset(&send_evt, 0, sizeof(CPSV_EVT));
		send_evt.type = CPSV_EVT_TYPE_CPA;
		send_evt.info.cpa.type = CPA_EVT_ND2A_ACT_CKPT_INFO_BCAST_SEND;
		send_evt.info.cpa.info.ackpt_info.ckpt_id = cp_node->ckpt_id;
		send_evt.info.cpa.info.ackpt_info.mds_dest = cp_node->active_mds_dest;
		rc = cpnd_mds_bcast_send(cb, &send_evt, NCSMDS_SVC_ID_CPA);
		m_LOG_CPND_FFCL(CPND_ACTIVE_REP_SET_SUCCESS, CPND_FC_HDLN, NCSFL_SEV_NOTICE, cp_node->ckpt_id,
				cp_node->active_mds_dest, __FILE__, __LINE__);
	}
	cp_node->is_restart = FALSE;
	return rc;
}

/****************************************************************************
 * Name          : cpnd_evt_proc_ckpt_rdset_info
 *
 * Description   : Function to process retention duration request
 *
 * Arguments     : CPND_CB *cb - CPND CB pointer
 *                 CPSV_EVT *evt - Received Event structure
 *                 CPSV_SEND_INFO *sinfo - Sender MDS information.
 *
 * Return Values : NCSCC_RC_SUCCESS/Error.
 *
 * Notes         : None.
 *****************************************************************************/
static uns32 cpnd_evt_proc_ckpt_rdset_info(CPND_CB *cb, CPND_EVT *evt, CPSV_SEND_INFO *sinfo)
{
	CPND_CKPT_NODE *cp_node = NULL;
	SaAisErrorT error = SA_AIS_OK;

	/* get cp_node from ckpt_info_db */
	cpnd_ckpt_node_get(cb, evt->info.rdset.ckpt_id, &cp_node);
	if (cp_node == NULL) {
		m_LOG_CPND_FCL(CPND_CKPT_NODE_GET_FAILED, CPND_FC_API, NCSFL_SEV_INFO, evt->info.rdset.ckpt_id,
			       __FILE__, __LINE__);
		return NCSCC_RC_FAILURE;
	}

	if (evt->info.rdset.type == CPSV_CKPT_RDSET_STOP) {
		if (!m_CPND_IS_COLLOCATED_ATTR_SET(cp_node->create_attrib.creationFlags)) {

			if (cp_node->ret_tmr.is_active)
				cpnd_tmr_stop(&cp_node->ret_tmr);

		}
		return NCSCC_RC_SUCCESS;
	}

	if (evt->info.rdset.type == CPSV_CKPT_RDSET_START) {
		if (!m_CPND_IS_COLLOCATED_ATTR_SET(cp_node->create_attrib.creationFlags)) {
			if (cpnd_ckpt_non_collocated_rplica_close(cb, cp_node, &error) == NCSCC_RC_FAILURE) {
				m_LOG_CPND_FFCL(CPND_CKPT_REPLICA_CLOSE_FAILED, CPND_FC_GENERIC, NCSFL_SEV_ERROR,
						evt->info.closeReq.client_hdl, cp_node->ckpt_id, __FILE__, __LINE__);

			}
			return NCSCC_RC_SUCCESS;
		}
	}

	/* if timer already started on one of the node then what to do!!! 
	   not doing any thing just updating the value */

	if (cp_node->is_close == TRUE)
		return NCSCC_RC_SUCCESS;

	if (!cp_node->is_rdset)
		cp_node->create_attrib.retentionDuration = evt->info.rdset.reten_time;

	return NCSCC_RC_SUCCESS;

}

/**********************************************************
 * Name          : cpnd_evt_proc_ckpt_mem_size
 *
 * Description   : Function to process the info
 *
 * Arguments     : CPND_CB *cb - CPND CB pointer
 *                 CPSV_EVT *evt - Received Event structure
 *                 CPSV_SEND_INFO *sinfo - Sender MDS information.
**************************************************************/
static uns32 cpnd_evt_proc_ckpt_mem_size(CPND_CB *cb, CPND_EVT *evt, CPSV_SEND_INFO *sinfo)
{

	CPND_CKPT_NODE *cp_node = NULL;
	uns32 rc = SA_AIS_OK;
	CPSV_EVT send_evt;

	memset(&send_evt, 0, sizeof(CPSV_EVT));
	cpnd_ckpt_node_get(cb, evt->info.ckpt_mem_size.ckpt_id, &cp_node);
	if (cp_node == NULL) {
		m_LOG_CPND_FCL(CPND_CKPT_NODE_GET_FAILED, CPND_FC_API, NCSFL_SEV_ERROR,
			       evt->info.ckpt_mem_size.ckpt_id, __FILE__, __LINE__);
		send_evt.type = CPSV_EVT_TYPE_CPD;
		send_evt.info.cpd.type = CPD_EVT_ND2D_CKPT_MEM_USED;
		send_evt.info.cpd.info.ckpt_mem_used.error = SA_AIS_ERR_NOT_EXIST;
		goto end;
	}

	send_evt.type = CPSV_EVT_TYPE_CPD;
	send_evt.info.cpd.type = CPD_EVT_ND2D_CKPT_MEM_USED;
	send_evt.info.cpd.info.ckpt_mem_used.ckpt_id = cp_node->ckpt_id;
	send_evt.info.cpd.info.ckpt_mem_used.ckpt_used_size = cp_node->replica_info.mem_used;
	send_evt.info.cpd.info.ckpt_mem_used.error = rc;

 end:
	rc = cpnd_mds_send_rsp(cb, sinfo, &send_evt);
	return rc;
}

/**********************************************************
 * Name          : cpnd_evt_proc_ckpt_num_sections
 *
 * Description   : Function to process the info
 *
 * Arguments     : CPND_CB *cb - CPND CB pointer
 *                 CPSV_EVT *evt - Received Event structure
 *                 CPSV_SEND_INFO *sinfo - Sender MDS information.
**************************************************************/
static uns32 cpnd_evt_proc_ckpt_num_sections(CPND_CB *cb, CPND_EVT *evt, CPSV_SEND_INFO *sinfo)
{

	CPND_CKPT_NODE *cp_node = NULL;
	uns32 rc = SA_AIS_OK;
	CPSV_EVT send_evt;

	memset(&send_evt, 0, sizeof(CPSV_EVT));
	cpnd_ckpt_node_get(cb, evt->info.ckpt_sections.ckpt_id, &cp_node);
	if (cp_node == NULL) {
		m_LOG_CPND_FCL(CPND_CKPT_NODE_GET_FAILED, CPND_FC_API, NCSFL_SEV_ERROR,
			       evt->info.ckpt_sections.ckpt_id, __FILE__, __LINE__);
		send_evt.type = CPSV_EVT_TYPE_CPD;
		send_evt.info.cpd.type = CPD_EVT_ND2D_CKPT_CREATED_SECTIONS;
		send_evt.info.cpd.info.ckpt_created_sections.error = SA_AIS_ERR_NOT_EXIST;
		send_evt.info.cpd.info.ckpt_created_sections.ckpt_num_sections = 0;
		goto end;
	}

	send_evt.type = CPSV_EVT_TYPE_CPD;
	send_evt.info.cpd.type = CPD_EVT_ND2D_CKPT_CREATED_SECTIONS;
	send_evt.info.cpd.info.ckpt_created_sections.ckpt_id = cp_node->ckpt_id;
	send_evt.info.cpd.info.ckpt_created_sections.ckpt_num_sections = cp_node->replica_info.n_secs;
	send_evt.info.cpd.info.ckpt_created_sections.error = rc;

 end:
	rc = cpnd_mds_send_rsp(cb, sinfo, &send_evt);
	return rc;
}

/****************************************************************************
 * Name          : cpnd_evt_proc_ckpt_rep_del
 *
 * Description   : Function to process addition of new replica node 
 *                 for a checkpoint. 
 *
 * Arguments     : CPND_CB *cb - CPND CB pointer
 *                 CPSV_EVT *evt - Received Event structure
 *                 CPSV_SEND_INFO *sinfo - Sender MDS information.
 *
 * Return Values : NCSCC_RC_SUCCESS/Error.
 *
 * Notes         : None.
 ***************************************************************/
static uns32 cpnd_evt_proc_ckpt_rep_del(CPND_CB *cb, CPND_EVT *evt, CPSV_SEND_INFO *sinfo)
{
	CPND_CKPT_NODE *cp_node = NULL;
	uns32 rc = NCSCC_RC_SUCCESS;

	/* get cp_node from ckpt_info_db */
	cpnd_ckpt_node_get(cb, evt->info.ckpt_del.ckpt_id, &cp_node);
	if (cp_node == NULL) {
		m_LOG_CPND_FCL(CPND_CKPT_NODE_GET_FAILED, CPND_FC_API, NCSFL_SEV_INFO, evt->info.ckpt_del.ckpt_id,
			       __FILE__, __LINE__);
		return NCSCC_RC_FAILURE;
	}

	rc = cpnd_ckpt_remote_cpnd_del(cp_node, evt->info.ckpt_del.mds_dest);
	if (rc != NCSCC_RC_SUCCESS) {
		m_LOG_CPND_FCL(CPND_CKPT_NODE_GET_FAILED, CPND_FC_API, NCSFL_SEV_INFO, evt->info.ckpt_del.ckpt_id,
			       __FILE__, __LINE__);
		return NCSCC_RC_FAILURE;
	}
	m_LOG_CPND_FFCL(CPND_REP_DEL_SUCCESS, CPND_FC_API, NCSFL_SEV_INFO,
			cp_node->ckpt_id, evt->info.ckpt_del.mds_dest, __FILE__, __LINE__);

	return NCSCC_RC_SUCCESS;

}

/****************************************************************************
 * Name          : cpnd_evt_proc_ckpt_rep_add
 *
 * Description   : Function to process deletion of replica node   
 *                 for a checkpoint
 *
 * Arguments     : CPND_CB *cb - CPND CB pointer
 *                 CPSV_EVT *evt - Received Event structure
 *                 CPSV_SEND_INFO *sinfo - Sender MDS information.
 *
 * Return Values : NCSCC_RC_SUCCESS/Error.
 *
 * Notes         : None.
 *****************************************************************************/
static uns32 cpnd_evt_proc_ckpt_rep_add(CPND_CB *cb, CPND_EVT *evt, CPSV_SEND_INFO *sinfo)
{
	CPND_CKPT_NODE *cp_node = NULL;
	uns32 rc = NCSCC_RC_SUCCESS;
	CPSV_CPND_DEST_INFO *tmp = NULL;
	uns32 dst_cnt = 0;

	cpnd_ckpt_node_get(cb, evt->info.ckpt_add.ckpt_id, &cp_node);
	if (cp_node == NULL) {
		m_LOG_CPND_FCL(CPND_CKPT_NODE_GET_FAILED, CPND_FC_API, NCSFL_SEV_INFO,
			       evt->info.ckpt_add.ckpt_id, __FILE__, __LINE__);
		return NCSCC_RC_FAILURE;
	}

	/* Non-active Restarted , bcast came to him : update the dest list */
	if (m_CPND_IS_LOCAL_NODE(&evt->info.ckpt_add.mds_dest, &cb->cpnd_mdest_id) == 0) {

		if (evt->info.ckpt_add.dest_list) {

			tmp = evt->info.ckpt_add.dest_list;

			while (dst_cnt < evt->info.ckpt_add.dest_cnt) {

				if (m_CPND_IS_LOCAL_NODE(&tmp[dst_cnt].dest, &cb->cpnd_mdest_id) != 0) {
					cpnd_ckpt_remote_cpnd_add(cp_node, tmp[dst_cnt].dest);
				}

				dst_cnt++;
			}
		}

		/* REP_ADD in case of RESTART scenario */
		if (evt->info.ckpt_add.is_cpnd_restart) {
			cp_node->create_attrib = evt->info.ckpt_add.attributes;
			cp_node->open_flags = evt->info.ckpt_add.ckpt_flags;
			cp_node->active_mds_dest = evt->info.ckpt_add.active_dest;
			if (evt->info.ckpt_add.active_dest) {
				cp_node->is_active_exist = TRUE;
			} else {
				cp_node->is_active_exist = FALSE;
			}
		}

		return NCSCC_RC_SUCCESS;
	}

	/* Add the new replica's MDS_DEST to the dest liot of this replica */
	rc = cpnd_ckpt_remote_cpnd_add(cp_node, evt->info.ckpt_add.mds_dest);

	return rc;
}

/****************************************************************************
 * Name          : cpnd_evt_proc_ckpt_sect_exp_set
 *
 * Description   : Function to porcess section expiration time set request
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
static uns32 cpnd_evt_proc_ckpt_sect_exp_set(CPND_CB *cb, CPND_EVT *evt, CPSV_SEND_INFO *sinfo)
{
	uns32 rc = NCSCC_RC_SUCCESS;
	CPSV_EVT send_evt, *out_evt = NULL;
	CPND_CKPT_NODE *cp_node = NULL;
	CPND_CKPT_SECTION_INFO *sec_info = NULL;

	memset(&send_evt, '\0', sizeof(CPSV_EVT));

	cpnd_ckpt_node_get(cb, evt->info.sec_expset.ckpt_id, &cp_node);

	if (cp_node == NULL) {
		m_LOG_CPND_FCL(CPND_CKPT_NODE_GET_FAILED, CPND_FC_API, NCSFL_SEV_INFO,
			       evt->info.sec_expset.ckpt_id, __FILE__, __LINE__);
		send_evt.info.cpa.info.sec_exptmr_rsp.error = SA_AIS_ERR_NOT_EXIST;
		goto agent_rsp;
	}

	if (cp_node->is_active_exist != TRUE) {
		send_evt.info.cpa.info.sec_exptmr_rsp.error = SA_AIS_ERR_NOT_EXIST;
		goto agent_rsp;
	}

	/*  CPND REDUNDANCY */
	if (cp_node->is_restart) {
		send_evt.info.cpa.info.sec_exptmr_rsp.error = SA_AIS_ERR_TRY_AGAIN;
		goto agent_rsp;
	}

	if (cp_node->create_attrib.maxSections == 1) {
		send_evt.info.cpa.info.sec_exptmr_rsp.error = SA_AIS_ERR_INVALID_PARAM;
		goto agent_rsp;
	}

	if (cp_node->cpnd_rep_create) {
		sec_info = cpnd_ckpt_sec_get(cp_node, &evt->info.sec_expset.sec_id);
		if (sec_info == NULL) {
			m_LOG_CPND_CFCL(CPND_CKPT_SECT_GET_FAILED, CPND_FC_API, NCSFL_SEV_ERROR,
					evt->info.sec_expset.sec_id.id, evt->info.sec_expset.ckpt_id,
					__FILE__, __LINE__);
			send_evt.info.cpa.info.sec_exptmr_rsp.error = SA_AIS_ERR_NOT_EXIST;
			goto agent_rsp;
		}
	}

	if ((m_CPND_IS_LOCAL_NODE(&cp_node->active_mds_dest, &cb->cpnd_mdest_id) == 0)) {
		if (sec_info) {
			sec_info->exp_tmr = evt->info.sec_expset.exp_time;
			sec_info->ckpt_sec_exptmr.type = CPND_TMR_TYPE_SEC_EXPI;
			sec_info->ckpt_sec_exptmr.ckpt_id = cp_node->ckpt_id;
			sec_info->ckpt_sec_exptmr.uarg = cb->cpnd_cb_hdl_id;
			sec_info->ckpt_sec_exptmr.lcl_sec_id = sec_info->lcl_sec_id;

			cpnd_sec_hdr_update(sec_info, cp_node);

			cpnd_tmr_start(&sec_info->ckpt_sec_exptmr,
				       m_CPSV_CONVERT_SATIME_TEN_MILLI_SEC(sec_info->exp_tmr));
		}
	} else {
		send_evt.type = CPSV_EVT_TYPE_CPND;
		send_evt.info.cpnd.type = CPSV_EVT_ND2ND_CKPT_SECT_EXPTMR_REQ;
		send_evt.info.cpnd.info.sec_exp_set = evt->info.sec_expset;
		rc = cpnd_mds_msg_sync_send(cb, NCSMDS_SVC_ID_CPND, cp_node->active_mds_dest,
					    &send_evt, &out_evt, CPSV_WAIT_TIME);
		if (rc != NCSCC_RC_SUCCESS) {
			m_LOG_CPND_FFFLCL(CPND_REMOTE_TO_ACTIVE_MDS_SEND_FAIL, CPND_FC_MDSFAIL, NCSFL_SEV_ERROR,
					  cb->cpnd_mdest_id, cp_node->active_mds_dest, cp_node->ckpt_id, rc, __FILE__,
					  __LINE__);
			if (rc == NCSCC_RC_REQ_TIMOUT) {
				send_evt.info.cpa.info.sec_exptmr_rsp.error = SA_AIS_ERR_TIMEOUT;
			} else
				send_evt.info.cpa.info.sec_exptmr_rsp.error = SA_AIS_ERR_TRY_AGAIN;
			goto agent_rsp;
		}
		if (out_evt && out_evt->info.cpnd.info.sec_exp_rsp.error != SA_AIS_OK) {
			send_evt.info.cpa.info.sec_exptmr_rsp.error = out_evt->info.cpnd.info.sec_exp_rsp.error;
			goto agent_rsp;
		}
		if (cp_node->cpnd_rep_create) {
			sec_info->exp_tmr = evt->info.sec_expset.exp_time;
			cpnd_sec_hdr_update(sec_info, cp_node);
		}
	}

	send_evt.info.cpa.info.sec_exptmr_rsp.error = SA_AIS_OK;

 agent_rsp:
	send_evt.type = CPSV_EVT_TYPE_CPA;
	send_evt.info.cpa.type = CPA_EVT_ND2A_SEC_EXPTIME_RSP;
	rc = cpnd_mds_send_rsp(cb, sinfo, &send_evt);

	if (out_evt)
		cpnd_evt_destroy(out_evt);

	return rc;
}

/****************************************************************************
 * Name          : cpnd_evt_proc_ckpt_sect_create
 *
 * Description   : Function to process creation of new section request  
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
static uns32 cpnd_evt_proc_ckpt_sect_create(CPND_CB *cb, CPND_EVT *evt, CPSV_SEND_INFO *sinfo)
{
	CPND_CKPT_NODE *cp_node = NULL;
	uns32 gen_sec_id = 0;
	CPSV_EVT send_evt, *out_evt = NULL;
	CPND_CKPT_SECTION_INFO *sec_info = NULL;
	uns32 rc = NCSCC_RC_SUCCESS;
	CPSV_CKPT_ACCESS ckpt_access;
	CPSV_CKPT_DATA *ckpt_data = NULL;
	SaTimeT now, duration;
	int64 time_stamp, giga_sec, result;

	memset(&send_evt, '\0', sizeof(CPSV_EVT));

	cpnd_ckpt_node_get(cb, evt->info.sec_creatReq.ckpt_id, &cp_node);
	if (cp_node == NULL) {
		m_LOG_CPND_FLCL(CPND_CKPT_NODE_GET_FAILED, CPND_FC_API, NCSFL_SEV_ERROR,
				evt->info.sec_creatReq.ckpt_id, SA_AIS_ERR_NOT_EXIST, __FILE__, __LINE__);
		send_evt.type = CPSV_EVT_TYPE_CPA;
		send_evt.info.cpa.type = CPA_EVT_ND2A_SEC_CREATE_RSP;
		send_evt.info.cpa.info.sec_creat_rsp.error = SA_AIS_ERR_NOT_EXIST;
		goto agent_rsp;
	}

	/* CPND REDUNDANCY */
	if ((TRUE == cp_node->is_restart) || (m_CPND_IS_LOCAL_NODE(&cp_node->active_mds_dest, &cb->cpnd_mdest_id) != 0)) {
		send_evt.type = CPSV_EVT_TYPE_CPA;
		send_evt.info.cpa.type = CPA_EVT_ND2A_SEC_CREATE_RSP;
		send_evt.info.cpa.info.sec_creat_rsp.error = SA_AIS_ERR_TRY_AGAIN;
		m_LOG_CPND_FLCL(CPND_CKPT_SECT_CREATE_FAILED, CPND_FC_HDLN, NCSFL_SEV_ERROR,
				evt->info.sec_creatReq.ckpt_id, SA_AIS_ERR_TRY_AGAIN, __FILE__, __LINE__);
		goto agent_rsp;
	}

	/* check for active replica presence */
	if (cp_node->is_active_exist != TRUE) {
		send_evt.type = CPSV_EVT_TYPE_CPA;
		send_evt.info.cpa.type = CPA_EVT_ND2A_SEC_CREATE_RSP;
		send_evt.info.cpa.info.sec_creat_rsp.error = SA_AIS_ERR_NOT_EXIST;
		m_LOG_CPND_FLCL(CPND_CKPT_SECT_CREATE_FAILED, CPND_FC_HDLN, NCSFL_SEV_ERROR,
				evt->info.sec_creatReq.ckpt_id, SA_AIS_ERR_NOT_EXIST, __FILE__, __LINE__);
		goto agent_rsp;
	}

	if (evt->info.sec_creatReq.init_size > cp_node->create_attrib.maxSectionSize) {
		send_evt.type = CPSV_EVT_TYPE_CPA;
		send_evt.info.cpa.type = CPA_EVT_ND2A_SEC_CREATE_RSP;
		send_evt.info.cpa.info.sec_creat_rsp.error = SA_AIS_ERR_INVALID_PARAM;
		m_LOG_CPND_FLCL(CPND_CKPT_SECT_CREATE_FAILED, CPND_FC_HDLN, NCSFL_SEV_ERROR,
				evt->info.sec_creatReq.ckpt_id, SA_AIS_ERR_INVALID_PARAM, __FILE__, __LINE__);
		goto agent_rsp;
	}
	if (evt->info.sec_creatReq.sec_attri.sectionId->id == NULL &&
	    evt->info.sec_creatReq.sec_attri.sectionId->idLen == 0) {
		if (cp_node->create_attrib.maxSections > 1) {
			gen_sec_id = 1;
		} else {
			send_evt.type = CPSV_EVT_TYPE_CPA;
			send_evt.info.cpa.type = CPA_EVT_ND2A_SEC_CREATE_RSP;
			send_evt.info.cpa.info.sec_creat_rsp.error = SA_AIS_ERR_EXIST;
			m_LOG_CPND_FLCL(CPND_CKPT_SECT_CREATE_FAILED, CPND_FC_HDLN, NCSFL_SEV_ERROR,
					evt->info.sec_creatReq.ckpt_id, SA_AIS_ERR_EXIST, __FILE__, __LINE__);
			goto agent_rsp;
		}
	} else {
		if (cp_node->create_attrib.maxSections == 1) {

			send_evt.type = CPSV_EVT_TYPE_CPA;
			send_evt.info.cpa.type = CPA_EVT_ND2A_SEC_CREATE_RSP;
			send_evt.info.cpa.info.sec_creat_rsp.error = SA_AIS_ERR_EXIST;
			m_LOG_CPND_FLCL(CPND_CKPT_SECT_CREATE_FAILED, CPND_FC_HDLN, NCSFL_SEV_ERROR,
					evt->info.sec_creatReq.ckpt_id, SA_AIS_ERR_EXIST, __FILE__, __LINE__);
			goto agent_rsp;

		}
	}

	if (gen_sec_id == 0) {
		rc = cpnd_ckpt_sec_find(cp_node, evt->info.sec_creatReq.sec_attri.sectionId);
		if (rc == NCSCC_RC_SUCCESS) {
			send_evt.type = CPSV_EVT_TYPE_CPA;
			send_evt.info.cpa.type = CPA_EVT_ND2A_SEC_CREATE_RSP;
			send_evt.info.cpa.info.sec_creat_rsp.error = SA_AIS_ERR_EXIST;
			m_LOG_CPND_FCL(CPND_CKPT_SECT_CREATE_FAILED, CPND_FC_HDLN, NCSFL_SEV_ERROR,
				       evt->info.sec_creatReq.ckpt_id, __FILE__, __LINE__);
			goto agent_rsp;
		}
	}

	if (cp_node->cpnd_rep_create) {
		if (gen_sec_id)
			sec_info = cpnd_ckpt_sec_add(cp_node, evt->info.sec_creatReq.sec_attri.sectionId,
						     evt->info.sec_creatReq.sec_attri.expirationTime, 1);

		else
			sec_info = cpnd_ckpt_sec_add(cp_node, evt->info.sec_creatReq.sec_attri.sectionId,
						     evt->info.sec_creatReq.sec_attri.expirationTime, 0);

		if (sec_info == NULL) {
			m_LOG_CPND_CFCL(CPND_CKPT_SECT_ADD_FAILED, CPND_FC_API, NCSFL_SEV_ERROR,
					evt->info.sec_creatReq.sec_attri.sectionId->id, cp_node->ckpt_id,
					__FILE__, __LINE__);
			send_evt.type = CPSV_EVT_TYPE_CPA;
			send_evt.info.cpa.type = CPA_EVT_ND2A_SEC_CREATE_RSP;
			send_evt.info.cpa.info.sec_creat_rsp.error = SA_AIS_ERR_NO_SPACE;
			goto agent_rsp;
		}

		if ((evt->info.sec_creatReq.init_data != NULL) && (evt->info.sec_creatReq.init_size != 0)) {

			rc = cpnd_ckpt_sec_write(cp_node, sec_info, evt->info.sec_creatReq.init_data,
						 evt->info.sec_creatReq.init_size, 0, 1);
			if (rc == NCSCC_RC_FAILURE) {
				m_LOG_CPND_CFCL(CPND_CKPT_SECT_WRITE_FAILED, CPND_FC_HDLN, NCSFL_SEV_ERROR,
						sec_info->sec_id.id, cp_node->ckpt_id, __FILE__, __LINE__);
				send_evt.type = CPSV_EVT_TYPE_CPA;
				send_evt.info.cpa.type = CPA_EVT_ND2A_SEC_CREATE_RSP;
				send_evt.info.cpa.info.sec_creat_rsp.error = SA_AIS_ERR_NO_RESOURCES;
				goto agent_rsp;

			}
			/* Section Create Register Arrival Callback */
			else {
				ckpt_data = m_MMGR_ALLOC_CPSV_CKPT_DATA;
				if (ckpt_data == NULL) {
					rc = NCSCC_RC_FAILURE;
					m_LOG_CPND_CL(CPND_CKPT_DATA_ALLOC_FAILED, CPND_FC_MEMFAIL, NCSFL_SEV_ERROR,
						      __FILE__, __LINE__);
					send_evt.type = CPSV_EVT_TYPE_CPA;
					send_evt.info.cpa.type = CPA_EVT_ND2A_SEC_CREATE_RSP;
					send_evt.info.cpa.info.sec_creat_rsp.error = SA_AIS_ERR_NO_SPACE;
					goto agent_rsp;
				}

				memset(ckpt_data, '\0', sizeof(CPSV_CKPT_DATA));

				ckpt_data->sec_id = sec_info->sec_id;
				ckpt_data->data = evt->info.sec_creatReq.init_data;
				ckpt_data->dataSize = sec_info->sec_size;
				ckpt_data->dataOffset = 0;

				memset(&ckpt_access, '\0', sizeof(CPSV_CKPT_ACCESS));
				ckpt_access.ckpt_id = cp_node->ckpt_id;
				ckpt_access.lcl_ckpt_id = evt->info.sec_creatReq.lcl_ckpt_id;
				ckpt_access.agent_mdest = evt->info.sec_creatReq.agent_mdest;
				ckpt_access.num_of_elmts = 1;
				ckpt_access.data = ckpt_data;

				cpnd_proc_ckpt_arrival_info_ntfy(cb, cp_node, &ckpt_access, sinfo);
				m_MMGR_FREE_CPSV_CKPT_DATA(ckpt_data);
			}
		}

		if (cp_node->cpnd_dest_list != NULL) {
			/* yes ,go trough cp_node->cpnd_dest_list(sync send) */
			{
				CPSV_CPND_DEST_INFO *tmp = NULL;
				tmp = cp_node->cpnd_dest_list;
				while (tmp != NULL) {
					if (m_CPND_IS_LOCAL_NODE(&tmp->dest, &cb->cpnd_mdest_id) == 0) {
						tmp = tmp->next;
					} else {
						send_evt.type = CPSV_EVT_TYPE_CPND;
						send_evt.info.cpnd.type = CPSV_EVT_ND2ND_CKPT_SECT_CREATE_REQ;
						send_evt.info.cpnd.info.active_sec_creat.ckpt_id =
						    evt->info.sec_creatReq.ckpt_id;
						send_evt.info.cpnd.info.active_sec_creat.lcl_ckpt_id =
						    evt->info.sec_creatReq.lcl_ckpt_id;
						send_evt.info.cpnd.info.active_sec_creat.agent_mdest =
						    evt->info.sec_creatReq.agent_mdest;
						send_evt.info.cpnd.info.active_sec_creat.sec_attri =
						    evt->info.sec_creatReq.sec_attri;

						if (gen_sec_id) {
							send_evt.info.cpnd.info.active_sec_creat.sec_attri.sectionId =
							    &sec_info->sec_id;
						}
						send_evt.info.cpnd.info.active_sec_creat.init_data =
						    evt->info.sec_creatReq.init_data;
						send_evt.info.cpnd.info.active_sec_creat.init_size =
						    evt->info.sec_creatReq.init_size;
						rc = cpnd_mds_msg_sync_send(cb, NCSMDS_SVC_ID_CPND, tmp->dest,
									    &send_evt, &out_evt,
									    CPND_WAIT_TIME(evt->info.
											   sec_creatReq.init_size));
						if (rc != NCSCC_RC_SUCCESS) {
							if (rc == NCSCC_RC_REQ_TIMOUT) {
								m_LOG_CPND_FFFLCL(CPND_ACTIVE_TO_REMOTE_MDS_SEND_FAIL,
										  CPND_FC_MDSFAIL, NCSFL_SEV_ERROR,
										  cb->cpnd_mdest_id, tmp->dest,
										  cp_node->ckpt_id, rc, __FILE__,
										  __LINE__);
							}
						}

						if (out_evt
						    && out_evt->info.cpnd.info.active_sec_creat_rsp.error !=
						    SA_AIS_OK) {
							CPND_CKPT_SECTION_INFO *tmp_sec_info = NULL;
							memset(&send_evt, '\0', sizeof(CPSV_EVT));
							send_evt.type = CPSV_EVT_TYPE_CPA;
							send_evt.info.cpa.type = CPA_EVT_ND2A_SEC_CREATE_RSP;
							/* TBD: Don't know what to do, revisit this. Ideally send the request to 
							   other CPNDs to delete */
							/*  Section Create fails with SA_AIS_NOT_VALID */
							send_evt.info.cpa.info.sec_creat_rsp.error =
							    out_evt->info.cpnd.info.active_sec_creat_rsp.error;
							m_LOG_CPND_FLCL(CPND_CKPT_SECT_CREATE_FAILED, CPND_FC_HDLN,
									NCSFL_SEV_ERROR, cp_node->ckpt_id,
									send_evt.info.cpa.info.sec_creat_rsp.error,
									__FILE__, __LINE__);

							/* delete the section */
							if (gen_sec_id)
								tmp_sec_info =
								    cpnd_ckpt_sec_del(cp_node, &sec_info->sec_id);
							else
								tmp_sec_info =
								    cpnd_ckpt_sec_del(cp_node,
										      evt->info.sec_creatReq.
										      sec_attri.sectionId);

							if (tmp_sec_info == sec_info) {
								cp_node->replica_info.
								    shm_sec_mapping[sec_info->lcl_sec_id] = 1;
								m_CPND_FREE_CKPT_SECTION(sec_info);
							} else {
								m_LOG_CPND_CL(CPND_CKPT_SECT_DEL_FAILED, CPND_FC_API,
									      NCSFL_SEV_ERROR, __FILE__, __LINE__);
							}
							cpnd_evt_destroy(out_evt);
							out_evt = NULL;
							goto agent_rsp;
						}
						tmp = tmp->next;
						if (out_evt) {
							cpnd_evt_destroy(out_evt);
							out_evt = NULL;
						}
					}
				}
			}
		}

		time_stamp = m_GET_TIME_STAMP(now);
		giga_sec = 1000000000;
		result = time_stamp * giga_sec;
		if (sec_info->exp_tmr != SA_TIME_END) {
			duration = (sec_info->exp_tmr - result);
			duration = duration / 10000000;

			sec_info->ckpt_sec_exptmr.type = CPND_TMR_TYPE_SEC_EXPI;
			sec_info->ckpt_sec_exptmr.uarg = cb->cpnd_cb_hdl_id;
			sec_info->ckpt_sec_exptmr.lcl_sec_id = sec_info->lcl_sec_id;
			sec_info->ckpt_sec_exptmr.ckpt_id = cp_node->ckpt_id;

			/*      cpnd_tmr_start(&sec_info->ckpt_sec_exptmr, \
			   m_CPSV_CONVERT_SATIME_TEN_MILLI_SEC(sec_info->exp_tmr));  */
			cpnd_tmr_start(&sec_info->ckpt_sec_exptmr, duration);
		}

	}

	memset(&send_evt, '\0', sizeof(CPSV_EVT));

	send_evt.type = CPSV_EVT_TYPE_CPA;
	send_evt.info.cpa.type = CPA_EVT_ND2A_SEC_CREATE_RSP;
	send_evt.info.cpa.info.sec_creat_rsp.error = SA_AIS_OK;
	/* Bug no. 58263 (sol)-  section id is being sent back to CPA */
	if (cp_node->cpnd_rep_create)
		send_evt.info.cpa.info.sec_creat_rsp.sec_id = sec_info->sec_id;
	else
		send_evt.info.cpa.info.sec_creat_rsp.sec_id.id = NULL;
 agent_rsp:
	rc = cpnd_mds_send_rsp(cb, sinfo, &send_evt);
	if (out_evt)
		cpnd_evt_destroy(out_evt);
	return rc;

}

/****************************************************************************
 * Name          : cpnd_evt_proc_ckpt_sect_delete
 *
 * Description   : Function to process deletion of a section request
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
static uns32 cpnd_evt_proc_ckpt_sect_delete(CPND_CB *cb, CPND_EVT *evt, CPSV_SEND_INFO *sinfo)
{
	uns32 rc = NCSCC_RC_SUCCESS;
	CPND_CKPT_NODE *cp_node = NULL;
	CPSV_EVT send_evt, *out_evt = NULL;
	CPND_CKPT_SECTION_INFO *sec_info = NULL;
	CPSV_CKPT_DATA ckpt_data;
	CPSV_CKPT_ACCESS ckpt_access;

	memset(&send_evt, '\0', sizeof(CPSV_EVT));

	cpnd_ckpt_node_get(cb, evt->info.sec_delReq.ckpt_id, &cp_node);

	if (cp_node == NULL) {
		send_evt.type = CPSV_EVT_TYPE_CPA;
		send_evt.info.cpa.type = CPA_EVT_ND2A_SEC_DELETE_RSP;
		send_evt.info.cpa.info.sec_delete_rsp.error = SA_AIS_ERR_NOT_EXIST;
		m_LOG_CPND_FCL(CPND_CKPT_NODE_GET_FAILED, CPND_FC_API, NCSFL_SEV_ERROR,
			       evt->info.sec_delReq.ckpt_id, __FILE__, __LINE__);
		goto agent_rsp;
	}

	if (cp_node->is_active_exist != TRUE) {
		send_evt.type = CPSV_EVT_TYPE_CPA;
		send_evt.info.cpa.type = CPA_EVT_ND2A_SEC_DELETE_RSP;
		send_evt.info.cpa.info.sec_delete_rsp.error = SA_AIS_ERR_NOT_EXIST;
		goto agent_rsp;
	}
	/* CPND REDUNDANCY */
	if ((TRUE == cp_node->is_restart) || (m_CPND_IS_LOCAL_NODE(&cp_node->active_mds_dest, &cb->cpnd_mdest_id) != 0)) {
		send_evt.type = CPSV_EVT_TYPE_CPA;
		send_evt.info.cpa.type = CPA_EVT_ND2A_SEC_DELETE_RSP;
		send_evt.info.cpa.info.sec_delete_rsp.error = SA_AIS_ERR_TRY_AGAIN;
		goto agent_rsp;
	}
	if (cp_node->create_attrib.maxSections == 1) {
		send_evt.type = CPSV_EVT_TYPE_CPA;
		send_evt.info.cpa.type = CPA_EVT_ND2A_SEC_DELETE_RSP;
		send_evt.info.cpa.info.sec_delete_rsp.error = SA_AIS_ERR_INVALID_PARAM;
		goto agent_rsp;
	}

	rc = cpnd_ckpt_sec_find(cp_node, &evt->info.sec_delReq.sec_id);
	if (rc == NCSCC_RC_SUCCESS) {

		sec_info = cpnd_ckpt_sec_del(cp_node, &evt->info.sec_delReq.sec_id);
		/* resetting lcl_sec_id mapping */
		if (sec_info == NULL) {
			rc = NCSCC_RC_FAILURE;
			m_LOG_CPND_CFCL(CPND_CKPT_SECT_DEL_FAILED, CPND_FC_API, NCSFL_SEV_ERROR,
					evt->info.sec_delete_req.sec_id.id, evt->info.sec_delete_req.ckpt_id,
					__FILE__, __LINE__);
			send_evt.type = CPSV_EVT_TYPE_CPA;
			send_evt.info.cpa.type = CPA_EVT_ND2A_SEC_DELETE_RSP;
			send_evt.info.cpa.info.sec_delete_rsp.error = SA_AIS_ERR_INVALID_PARAM;
			goto agent_rsp;
		}
		cp_node->replica_info.shm_sec_mapping[sec_info->lcl_sec_id] = 1;

		/* Send the arrival callback */
		memset(&ckpt_data, '\0', sizeof(CPSV_CKPT_DATA));
		if (sec_info) {
			ckpt_data.sec_id = sec_info->sec_id;
			ckpt_data.data = NULL;
			ckpt_data.dataSize = sec_info->sec_size;
			ckpt_data.dataOffset = 0;

			memset(&ckpt_access, '\0', sizeof(CPSV_CKPT_ACCESS));
			ckpt_access.ckpt_id = cp_node->ckpt_id;
			ckpt_access.lcl_ckpt_id = evt->info.sec_delReq.lcl_ckpt_id;
			ckpt_access.agent_mdest = evt->info.sec_delReq.agent_mdest;
			ckpt_access.num_of_elmts = 1;
			ckpt_access.data = &ckpt_data;
			cpnd_proc_ckpt_arrival_info_ntfy(cb, cp_node, &ckpt_access, sinfo);
		}

		if (cp_node->cpnd_dest_list != NULL) {
			/* yes ,go trough cp_node->cpnd_dest_list(sync send) */
			/* send with sec_info->section_offset to other cpnd's */

			CPSV_CPND_DEST_INFO *tmp = NULL;
			tmp = cp_node->cpnd_dest_list;
			while (tmp != NULL) {
				send_evt.type = CPSV_EVT_TYPE_CPND;
				send_evt.info.cpnd.type = CPSV_EVT_ND2ND_CKPT_SECT_DELETE_REQ;
				send_evt.info.cpnd.info.sec_delete_req.ckpt_id = evt->info.sec_delReq.ckpt_id;
				send_evt.info.cpnd.info.sec_delete_req.lcl_ckpt_id = evt->info.sec_delReq.lcl_ckpt_id;
				send_evt.info.cpnd.info.sec_delete_req.agent_mdest = evt->info.sec_delReq.agent_mdest;
				send_evt.info.cpnd.info.sec_delete_req.sec_id = evt->info.sec_delReq.sec_id;

				rc = cpnd_mds_msg_sync_send(cb, NCSMDS_SVC_ID_CPND, tmp->dest,
							    &send_evt, &out_evt, CPSV_WAIT_TIME);
				if (rc != NCSCC_RC_SUCCESS) {
					if (rc == NCSCC_RC_REQ_TIMOUT) {
						m_LOG_CPND_FFFLCL(CPND_ACTIVE_TO_REMOTE_MDS_SEND_FAIL, CPND_FC_MDSFAIL,
								  NCSFL_SEV_ERROR, cb->cpnd_mdest_id, tmp->dest,
								  cp_node->ckpt_id, rc, __FILE__, __LINE__);
					}
					/*  Don't handle this error, go ahead and delete the rest 
					   send_evt.type=CPSV_EVT_TYPE_CPA;
					   send_evt.info.cpa.type=CPA_EVT_ND2A_SEC_DELETE_RSP;
					   send_evt.info.cpa.info.sec_delete_rsp.error=SA_AIS_ERR_LIBRARY;
					   goto agent_rsp;
					 */

				}
				if (out_evt && out_evt->info.cpnd.info.sec_delete_rsp.error != SA_AIS_OK) {
					/*  Don't handle this error, go ahead and delete the rest 
					   send_evt.type=CPSV_EVT_TYPE_CPA;
					   send_evt.info.cpa.type=CPA_EVT_ND2A_SEC_DELETE_RSP;
					   send_evt.info.cpa.info.sec_delete_rsp.error= \
					   out_evt->info.cpnd.info.sec_delete_rsp.error;
					   cpnd_evt_destroy(out_evt);
					   out_evt = NULL;
					   goto agent_rsp; */
				}
				tmp = tmp->next;
				if (out_evt) {
					cpnd_evt_destroy(out_evt);
					out_evt = NULL;
				}
			}
		}

		/* stop the timer and delete */
		cpnd_tmr_stop(&sec_info->ckpt_sec_exptmr);
		m_CPND_FREE_CKPT_SECTION(sec_info);
	} else {
		m_LOG_CPND_CFCL(CPND_CKPT_SECT_DEL_FAILED, CPND_FC_API, NCSFL_SEV_ERROR,
				evt->info.sec_delReq.sec_id.id, cp_node->ckpt_id, __FILE__, __LINE__);
		send_evt.type = CPSV_EVT_TYPE_CPA;
		send_evt.info.cpa.type = CPA_EVT_ND2A_SEC_DELETE_RSP;
		send_evt.info.cpa.info.sec_delete_rsp.error = SA_AIS_ERR_NOT_EXIST;
		goto agent_rsp;

	}

	send_evt.type = CPSV_EVT_TYPE_CPA;
	send_evt.info.cpa.type = CPA_EVT_ND2A_SEC_DELETE_RSP;
	send_evt.info.cpa.info.sec_delete_rsp.error = SA_AIS_OK;

 agent_rsp:
	rc = cpnd_mds_send_rsp(cb, sinfo, &send_evt);

	if (out_evt)
		cpnd_evt_destroy(out_evt);
	return rc;

}

/****************************************************************************
 * Name          : cpnd_evt_proc_nd2nd_ckpt_sect_create
 *
 * Description   : Function to process section creation request from other   
 *                 Node director for a checkpoint.
 *
 * Arguments     : CPND_CB *cb - CPND CB pointer
 *                 CPSV_EVT *evt - Received Event structure
 *                 CPSV_SEND_INFO *sinfo - Sender MDS information.
 *
 * Return Values : NCSCC_RC_SUCCESS/Error.
 *
 * Notes         : None.
 *****************************************************************************/
static uns32 cpnd_evt_proc_nd2nd_ckpt_sect_create(CPND_CB *cb, CPND_EVT *evt, CPSV_SEND_INFO *sinfo)
{
	uns32 rc = NCSCC_RC_SUCCESS;
	CPND_CKPT_NODE *cp_node = NULL;
	CPSV_EVT send_evt;
	CPND_CKPT_SECTION_INFO *sec_info = NULL;
	CPSV_CKPT_ACCESS ckpt_access;
	CPSV_CKPT_DATA *ckpt_data = NULL;
	SaAisErrorT error = SA_AIS_OK;

	memset(&send_evt, '\0', sizeof(CPSV_EVT));

	cpnd_ckpt_node_get(cb, evt->info.active_sec_creat.ckpt_id, &cp_node);
	if (cp_node == NULL) {
		error = SA_AIS_ERR_TRY_AGAIN;
		m_LOG_CPND_FCL(CPND_CKPT_NODE_GET_FAILED, CPND_FC_API, NCSFL_SEV_ERROR,
			       evt->info.active_sec_creat.ckpt_id, __FILE__, __LINE__);
		goto nd_rsp;
	}

	rc = cpnd_ckpt_sec_find(cp_node, evt->info.active_sec_creat.sec_attri.sectionId);
	if (rc == NCSCC_RC_SUCCESS) {
		error = SA_AIS_ERR_EXIST;
		m_LOG_CPND_FCL(CPND_CKPT_SECT_CREATE_FAILED, CPND_FC_HDLN, NCSFL_SEV_ERROR,
			       evt->info.sec_creatReq.ckpt_id, __FILE__, __LINE__);
		goto nd_rsp;
	}

	if (cp_node->cpnd_rep_create) {
		sec_info = cpnd_ckpt_sec_add(cp_node, evt->info.active_sec_creat.sec_attri.sectionId,
					     evt->info.active_sec_creat.sec_attri.expirationTime, 0);
		if (sec_info == NULL) {
			m_LOG_CPND_CFCL(CPND_CKPT_SECT_ADD_FAILED, CPND_FC_API, NCSFL_SEV_ERROR,
					evt->info.active_sec_creat.sec_attri.sectionId->id, cp_node->ckpt_id,
					__FILE__, __LINE__);
			error = SA_AIS_ERR_NO_SPACE;
			goto nd_rsp;
		}

		if (evt->info.active_sec_creat.init_data != NULL) {
			rc = cpnd_ckpt_sec_write(cp_node, sec_info, evt->info.active_sec_creat.init_data,
						 evt->info.active_sec_creat.init_size, 0, 1);
			if (rc == NCSCC_RC_FAILURE) {
				m_LOG_CPND_CL(CPND_CKPT_SECT_WRITE_FAILED, CPND_FC_HDLN, NCSFL_SEV_ERROR,
					      __FILE__, __LINE__);
				error = SA_AIS_ERR_NO_RESOURCES;
				goto nd_rsp;
			}

			/* Section Create Register Arrival Callback */
			else {
				ckpt_data = m_MMGR_ALLOC_CPSV_CKPT_DATA;
				if (ckpt_data == NULL) {
					rc = NCSCC_RC_FAILURE;
					m_LOG_CPND_CL(CPND_CKPT_DATA_ALLOC_FAILED, CPND_FC_MEMFAIL, NCSFL_SEV_ERROR,
						      __FILE__, __LINE__);
					error = SA_AIS_ERR_NO_SPACE;
					goto nd_rsp;
				}

				memset(ckpt_data, '\0', sizeof(CPSV_CKPT_DATA));

				ckpt_data->sec_id = sec_info->sec_id;
				ckpt_data->data = evt->info.active_sec_creat.init_data;
				ckpt_data->dataSize = sec_info->sec_size;
				ckpt_data->dataOffset = 0;

				memset(&ckpt_access, '\0', sizeof(CPSV_CKPT_ACCESS));
				ckpt_access.ckpt_id = cp_node->ckpt_id;
				ckpt_access.lcl_ckpt_id = evt->info.active_sec_creat.lcl_ckpt_id;
				ckpt_access.agent_mdest = evt->info.active_sec_creat.agent_mdest;
				ckpt_access.num_of_elmts = 1;
				ckpt_access.data = ckpt_data;

				cpnd_proc_ckpt_arrival_info_ntfy(cb, cp_node, &ckpt_access, sinfo);
				m_MMGR_FREE_CPSV_CKPT_DATA(ckpt_data);

				send_evt.info.cpnd.info.active_sec_creat_rsp.sec_id = sec_info->sec_id;
				send_evt.info.cpnd.info.active_sec_creat_rsp.ckpt_id =
				    evt->info.active_sec_creat.ckpt_id;
			}
		}
	}

 nd_rsp:
	send_evt.type = CPSV_EVT_TYPE_CPND;
	send_evt.info.cpnd.type = CPSV_EVT_ND2ND_CKPT_SECT_ACTIVE_CREATE_RSP;
	send_evt.info.cpnd.info.active_sec_creat_rsp.error = error;

	return cpnd_mds_send_rsp(cb, sinfo, &send_evt);
}

/****************************************************************************
 * Name          : cpnd_evt_proc_nd2nd_ckpt_sect_delete
 *
 * Description   : Function to process section deletion request from other
 *                 Node director for a checkpoint.
 *
 * Arguments     : CPND_CB *cb - CPND CB pointer
 *                 CPSV_EVT *evt - Received Event structure
 *                 CPSV_SEND_INFO *sinfo - Sender MDS information.
 *
 * Return Values : NCSCC_RC_SUCCESS/Error.
 *
 * Notes         : None.
 *****************************************************************************/
static uns32 cpnd_evt_proc_nd2nd_ckpt_sect_delete(CPND_CB *cb, CPND_EVT *evt, CPSV_SEND_INFO *sinfo)
{
	uns32 rc = NCSCC_RC_SUCCESS;
	CPND_CKPT_NODE *cp_node = NULL;
	CPSV_EVT send_evt;
	CPND_CKPT_SECTION_INFO *sec_info = NULL;
	CPSV_CKPT_DATA ckpt_data;
	CPSV_CKPT_ACCESS ckpt_access;

	memset(&send_evt, '\0', sizeof(CPSV_EVT));

	cpnd_ckpt_node_get(cb, evt->info.sec_delete_req.ckpt_id, &cp_node);
	if (cp_node == NULL) {
		m_LOG_CPND_FCL(CPND_CKPT_NODE_GET_FAILED, CPND_FC_API, NCSFL_SEV_ERROR,
			       evt->info.sec_delete_req.ckpt_id, __FILE__, __LINE__);
		send_evt.type = CPSV_EVT_TYPE_CPND;
		send_evt.info.cpnd.type = CPSV_EVT_ND2ND_CKPT_SECT_DELETE_RSP;
		send_evt.info.cpnd.info.sec_delete_rsp.error = SA_AIS_ERR_TRY_AGAIN;
		goto nd_rsp;
	}
	sec_info = cpnd_ckpt_sec_del(cp_node, &evt->info.sec_delete_req.sec_id);
	if (sec_info == NULL) {
		m_LOG_CPND_CFCL(CPND_CKPT_SECT_DEL_FAILED, CPND_FC_API, NCSFL_SEV_ERROR,
				evt->info.sec_delete_req.sec_id.id, evt->info.sec_delete_req.ckpt_id,
				__FILE__, __LINE__);
		send_evt.type = CPSV_EVT_TYPE_CPND;
		send_evt.info.cpnd.type = CPSV_EVT_ND2ND_CKPT_SECT_DELETE_RSP;
		send_evt.info.cpnd.info.sec_delete_rsp.error = SA_AIS_ERR_NOT_EXIST;
		goto nd_rsp;

	}
	/* resetting lcl_sec_id mapping */
	cp_node->replica_info.shm_sec_mapping[sec_info->lcl_sec_id] = 1;

	/* Send the arrival callback */
	memset(&ckpt_data, '\0', sizeof(CPSV_CKPT_DATA));
	if (sec_info) {
		ckpt_data.sec_id = sec_info->sec_id;
		ckpt_data.data = NULL;
		ckpt_data.dataSize = sec_info->sec_size;
		ckpt_data.dataOffset = 0;

		memset(&ckpt_access, '\0', sizeof(CPSV_CKPT_ACCESS));
		ckpt_access.ckpt_id = cp_node->ckpt_id;
		ckpt_access.lcl_ckpt_id = evt->info.sec_delete_req.lcl_ckpt_id;
		ckpt_access.agent_mdest = evt->info.sec_delete_req.agent_mdest;
		ckpt_access.num_of_elmts = 1;
		ckpt_access.data = &ckpt_data;
		cpnd_proc_ckpt_arrival_info_ntfy(cb, cp_node, &ckpt_access, sinfo);
	}

	m_CPND_FREE_CKPT_SECTION(sec_info);
	send_evt.type = CPSV_EVT_TYPE_CPND;
	send_evt.info.cpnd.type = CPSV_EVT_ND2ND_CKPT_SECT_DELETE_RSP;
	send_evt.info.cpnd.info.sec_delete_rsp.error = SA_AIS_OK;

 nd_rsp:
	rc = cpnd_mds_send_rsp(cb, sinfo, &send_evt);

	return rc;

}

/****************************************************************************
 * Name          : cpnd_evt_proc_nd2nd_ckpt_sect_exptmr_req
 *
 * Description   : Function to set section expiration time request    
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
static uns32 cpnd_evt_proc_nd2nd_ckpt_sect_exptmr_req(CPND_CB *cb, CPND_EVT *evt, CPSV_SEND_INFO *sinfo)
{
	uns32 rc = NCSCC_RC_SUCCESS;
	CPSV_EVT send_evt;
	CPND_CKPT_NODE *cp_node = NULL;
	CPND_CKPT_SECTION_INFO *sec_info = NULL;

	memset(&send_evt, '\0', sizeof(CPSV_EVT));

	cpnd_ckpt_node_get(cb, evt->info.sec_exp_set.ckpt_id, &cp_node);
	if (cp_node == NULL) {
		m_LOG_CPND_FCL(CPND_CKPT_NODE_GET_FAILED, CPND_FC_API, NCSFL_SEV_ERROR,
			       evt->info.sec_exp_set.ckpt_id, __FILE__, __LINE__);
		send_evt.type = CPSV_EVT_TYPE_CPND;
		send_evt.info.cpnd.type = CPSV_EVT_ND2ND_CKPT_SECT_EXPTMR_RSP;
		send_evt.info.cpnd.info.sec_exp_rsp.error = SA_AIS_ERR_TRY_AGAIN;
		goto nd_rsp;
	}

	if ((m_CPND_IS_LOCAL_NODE(&cp_node->active_mds_dest, &cb->cpnd_mdest_id) == 0)) {
		sec_info = cpnd_ckpt_sec_get(cp_node, &evt->info.sec_exp_set.sec_id);
		if (sec_info == NULL) {
			m_LOG_CPND_CFCL(CPND_CKPT_SECT_GET_FAILED, CPND_FC_API, NCSFL_SEV_ERROR,
					evt->info.sec_exp_set.sec_id.id, evt->info.sec_exp_set.ckpt_id,
					__FILE__, __LINE__);
			send_evt.type = CPSV_EVT_TYPE_CPND;
			send_evt.info.cpnd.type = CPSV_EVT_ND2ND_CKPT_SECT_EXPTMR_RSP;
			send_evt.info.cpnd.info.sec_exp_rsp.error = SA_AIS_ERR_NOT_EXIST;
			goto nd_rsp;
		} else {
			sec_info->exp_tmr = evt->info.sec_exp_set.exp_time;
			sec_info->ckpt_sec_exptmr.type = CPND_TMR_TYPE_SEC_EXPI;
			sec_info->ckpt_sec_exptmr.uarg = cb->cpnd_cb_hdl_id;
			sec_info->ckpt_sec_exptmr.lcl_sec_id = sec_info->lcl_sec_id;
			sec_info->ckpt_sec_exptmr.ckpt_id = cp_node->ckpt_id;

			if (cp_node->cpnd_rep_create) {
				cpnd_sec_hdr_update(sec_info, cp_node);
			}
			cpnd_tmr_start(&sec_info->ckpt_sec_exptmr, sec_info->exp_tmr);
		}
	}

	send_evt.type = CPSV_EVT_TYPE_CPND;
	send_evt.info.cpnd.type = CPSV_EVT_ND2ND_CKPT_SECT_EXPTMR_RSP;
	send_evt.info.cpnd.info.sec_exp_rsp.error = SA_AIS_OK;

 nd_rsp:
	rc = cpnd_mds_send_rsp(cb, sinfo, &send_evt);
	return rc;

}

/****************************************************************************
 * Name          : cpnd_evt_proc_nd2nd_ckpt_status
 *
 * Description   : Function to process checkpoint status from   
 *                 Node Director.
 *
 * Arguments     : CPND_CB *cb - CPND CB pointer
 *                 CPSV_EVT *evt - Received Event structure
 *                 CPSV_SEND_INFO *sinfo - Sender MDS information.
 *
 * Return Values : NCSCC_RC_SUCCESS/Error.
 *
 * Notes         : None.
 *****************************************************************************/
static uns32 cpnd_evt_proc_nd2nd_ckpt_status(CPND_CB *cb, CPND_EVT *evt, CPSV_SEND_INFO *sinfo)
{
	CPSV_EVT send_evt;
	CPND_CKPT_NODE *cp_node = NULL;
	uns32 rc = NCSCC_RC_SUCCESS;

	memset(&send_evt, '\0', sizeof(CPSV_EVT));

	/* get cp_node from ckpt_info_db */
	cpnd_ckpt_node_get(cb, evt->info.stat_get.ckpt_id, &cp_node);
	if (cp_node == NULL) {
		m_LOG_CPND_FCL(CPND_CKPT_NODE_GET_FAILED, CPND_FC_API, NCSFL_SEV_ERROR,
			       evt->info.stat_get.ckpt_id, __FILE__, __LINE__);
		send_evt.type = CPSV_EVT_TYPE_CPND;
		send_evt.info.cpnd.type = CPND_EVT_ND2ND_ACTIVE_STATUS_ACK;
		send_evt.info.cpnd.info.status.error = SA_AIS_ERR_TRY_AGAIN;
		goto nd_rsp;

	}

	send_evt.type = CPSV_EVT_TYPE_CPND;
	send_evt.info.cpnd.type = CPND_EVT_ND2ND_ACTIVE_STATUS_ACK;
	send_evt.info.cpnd.info.status.error = SA_AIS_OK;
	send_evt.info.cpnd.info.status.ckpt_id = cp_node->ckpt_id;
	send_evt.info.cpnd.info.status.status.checkpointCreationAttributes = cp_node->create_attrib;
	send_evt.info.cpnd.info.status.status.memoryUsed = cp_node->replica_info.mem_used;
	send_evt.info.cpnd.info.status.status.numberOfSections = cp_node->replica_info.n_secs;

 nd_rsp:
	rc = cpnd_mds_send_rsp(cb, sinfo, &send_evt);
	return rc;
}

/****************************************************************************
 * Name          : cpnd_evt_proc_ckpt_write
 *
 * Description   : Function to process SaCkptCheckpointWrite  
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
static uns32 cpnd_evt_proc_ckpt_write(CPND_CB *cb, CPND_EVT *evt, CPSV_SEND_INFO *sinfo)
{
	uns32 rc = NCSCC_RC_SUCCESS;
	CPND_CKPT_NODE *cp_node = NULL;
	CPSV_EVT send_evt;
	uns32 err_flag = 0;
	uns32 errflag = 0;
	memset(&send_evt, '\0', sizeof(CPSV_EVT));

	cpnd_ckpt_node_get(cb, evt->info.ckpt_write.ckpt_id, &cp_node);

	if (cp_node == NULL || cp_node->is_active_exist != TRUE) {
		send_evt.type = CPSV_EVT_TYPE_CPA;
		send_evt.info.cpa.type = CPA_EVT_ND2A_CKPT_DATA_RSP;
		switch (evt->info.ckpt_write.type) {
		case CPSV_CKPT_ACCESS_WRITE:
			send_evt.info.cpa.info.sec_data_rsp.type = CPSV_DATA_ACCESS_WRITE_RSP;
			send_evt.info.cpa.info.sec_data_rsp.num_of_elmts = -1;
			send_evt.info.cpa.info.sec_data_rsp.error = SA_AIS_ERR_NOT_EXIST;
			break;

		case CPSV_CKPT_ACCESS_OVWRITE:
			send_evt.info.cpa.info.sec_data_rsp.type = CPSV_DATA_ACCESS_OVWRITE_RSP;
			send_evt.info.cpa.info.sec_data_rsp.info.ovwrite_error.error = SA_AIS_ERR_NOT_EXIST;
			break;

		default:
			m_LOG_CPND_LCL(CPND_EVT_UNKNOWN, CPND_FC_EVT, NCSFL_SEV_ERROR, evt->info.ckpt_write.type,
				       __FILE__, __LINE__);
			break;

		}
		m_LOG_CPND_FLCL(CPND_CKPT_SECT_WRITE_FAILED, CPND_FC_HDLN, NCSFL_SEV_ERROR,
				evt->info.ckpt_write.ckpt_id, SA_AIS_ERR_NOT_EXIST, __FILE__, __LINE__);
		goto agent_rsp;
	}
	if ((TRUE == cp_node->is_restart) || (m_CPND_IS_LOCAL_NODE(&cp_node->active_mds_dest, &cb->cpnd_mdest_id) != 0)) {
		send_evt.type = CPSV_EVT_TYPE_CPA;
		send_evt.info.cpa.type = CPA_EVT_ND2A_CKPT_DATA_RSP;
		switch (evt->info.ckpt_write.type) {
		case CPSV_CKPT_ACCESS_WRITE:
			send_evt.info.cpa.info.sec_data_rsp.type = CPSV_DATA_ACCESS_WRITE_RSP;
			send_evt.info.cpa.info.sec_data_rsp.num_of_elmts = -1;
			send_evt.info.cpa.info.sec_data_rsp.error = SA_AIS_ERR_TRY_AGAIN;
			break;

		case CPSV_CKPT_ACCESS_OVWRITE:
			send_evt.info.cpa.info.sec_data_rsp.type = CPSV_DATA_ACCESS_OVWRITE_RSP;
			send_evt.info.cpa.info.sec_data_rsp.info.ovwrite_error.error = SA_AIS_ERR_TRY_AGAIN;
			break;

		default:
			m_LOG_CPND_LCL(CPND_EVT_UNKNOWN, CPND_FC_EVT, NCSFL_SEV_ERROR, evt->info.ckpt_write.type,
				       __FILE__, __LINE__);
			break;

		}
		m_LOG_CPND_FLCL(CPND_CKPT_SECT_WRITE_FAILED, CPND_FC_HDLN, NCSFL_SEV_ERROR,
				evt->info.ckpt_write.ckpt_id, cp_node->is_restart, __FILE__, __LINE__);

		/*send_evt.info.cpa.info.sec_data_rsp.num_of_elmts=-1;
		   send_evt.info.cpa.info.sec_data_rsp.error=SA_AIS_ERR_TRY_AGAIN; */
		goto agent_rsp;
	}

         rc=cpnd_ckpt_update_replica(cb,cp_node,&evt->info.ckpt_write,\
                 evt->info.ckpt_write.type,&err_flag,&errflag);
         if (rc == NCSCC_RC_FAILURE)
         {
            send_evt.type=CPSV_EVT_TYPE_CPA;
            send_evt.info.cpa.type=CPA_EVT_ND2A_CKPT_DATA_RSP;
            switch(evt->info.ckpt_write.type)
            {
               case CPSV_CKPT_ACCESS_WRITE:
                  send_evt.info.cpa.info.sec_data_rsp.error_index = errflag; 
                  send_evt.info.cpa.info.sec_data_rsp.type= \
                        CPSV_DATA_ACCESS_WRITE_RSP;
                  send_evt.info.cpa.info.sec_data_rsp.num_of_elmts=-1;
                  if ( err_flag ==  CKPT_UPDATE_REPLICA_RES_ERR )
                     send_evt.info.cpa.info.sec_data_rsp.error=SA_AIS_ERR_NO_RESOURCES;
                  else
                     send_evt.info.cpa.info.sec_data_rsp.error=SA_AIS_ERR_NOT_EXIST;
                  break;

               case CPSV_CKPT_ACCESS_OVWRITE:
                  send_evt.info.cpa.info.sec_data_rsp.error_index = errflag; 
                  send_evt.info.cpa.info.sec_data_rsp.type= \
                      CPSV_DATA_ACCESS_OVWRITE_RSP;
                  send_evt.info.cpa.info.sec_data_rsp.info.ovwrite_error.error= \
                             SA_AIS_ERR_NOT_EXIST;
                  break;

		default:
			m_LOG_CPND_LCL(CPND_EVT_UNKNOWN, CPND_FC_EVT, NCSFL_SEV_ERROR, evt->info.ckpt_write.type,
				       __FILE__, __LINE__);
			break;
		}
		m_LOG_CPND_FLCL(CPND_CKPT_SECT_WRITE_FAILED, CPND_FC_HDLN, NCSFL_SEV_ERROR,
				cp_node->ckpt_id, err_flag, __FILE__, __LINE__);
		goto agent_rsp;
	} else {
		cpnd_proc_ckpt_arrival_info_ntfy(cb, cp_node, &evt->info.ckpt_write, sinfo);
	}

	/* srikanth --- TBD need to check for active async and weak active and
	   return back  */

	rc = cpnd_proc_update_remote(cb, cp_node, evt, &send_evt, sinfo);
	if (rc == NCSCC_RC_FAILURE) {
		goto agent_rsp;
	}
	if (m_CPND_IS_ALL_REPLICA_ATTR_SET(cp_node->create_attrib.creationFlags) == TRUE
	    && cp_node->cpnd_dest_list != NULL)
		return rc;
	else
		goto agent_rsp;

 agent_rsp:
	rc = cpnd_mds_send_rsp(cb, sinfo, &send_evt);

	return rc;
}

/****************************************************************************
 * Name          : cpnd_evt_proc_nd2nd_ckpt_active_data_access_req
 *
 * Description   : Function to process write/read/overwrite   
 *                 from other ND to Active ND.
 *
 * Arguments     : CPND_CB *cb - CPND CB pointer
 *                 CPSV_EVT *evt - Received Event structure
 *                 CPSV_SEND_INFO *sinfo - Sender MDS information.
 *
 * Return Values : NCSCC_RC_SUCCESS/Error.
 *
 * Notes         : None.
 *****************************************************************************/
static uns32 cpnd_evt_proc_nd2nd_ckpt_active_data_access_req(CPND_CB *cb, CPND_EVT *evt, CPSV_SEND_INFO *sinfo)
{
	uns32 rc = NCSCC_RC_SUCCESS;
	CPND_CKPT_NODE *cp_node = NULL;
	CPSV_EVT send_evt;
	uns32 err_flag = 0;
	uns32 errflag = 0;
	memset(&send_evt, '\0', sizeof(CPSV_EVT));

	send_evt.type = CPSV_EVT_TYPE_CPND;
	send_evt.info.cpnd.type = CPSV_EVT_ND2ND_CKPT_SECT_ACTIVE_DATA_ACCESS_RSP;

	cpnd_ckpt_node_get(cb, evt->info.ckpt_nd2nd_data.ckpt_id, &cp_node);
	if (cp_node == NULL) {
		m_LOG_CPND_FCL(CPND_CKPT_NODE_GET_FAILED, CPND_FC_API, NCSFL_SEV_ERROR,
			       evt->info.ckpt_nd2nd_data.ckpt_id, __FILE__, __LINE__);
		switch (evt->info.ckpt_nd2nd_data.type) {

		case CPSV_CKPT_ACCESS_WRITE:
			send_evt.info.cpnd.info.ckpt_nd2nd_data_rsp.type = CPSV_DATA_ACCESS_WRITE_RSP;
			send_evt.info.cpnd.info.ckpt_nd2nd_data_rsp.num_of_elmts = -1;
			send_evt.info.cpnd.info.ckpt_nd2nd_data_rsp.error = SA_AIS_ERR_TRY_AGAIN;
			send_evt.info.cpnd.info.ckpt_nd2nd_data_rsp.size = 0;
			send_evt.info.cpnd.info.ckpt_nd2nd_data_rsp.info.write_err_index = NULL;
			break;

		case CPSV_CKPT_ACCESS_OVWRITE:
			send_evt.info.cpnd.info.ckpt_nd2nd_data_rsp.type = CPSV_DATA_ACCESS_OVWRITE_RSP;
			send_evt.info.cpnd.info.ckpt_nd2nd_data_rsp.info.ovwrite_error.error = SA_AIS_ERR_TRY_AGAIN;
			break;
		}

		send_evt.info.cpnd.info.ckpt_nd2nd_data_rsp.ckpt_id = evt->info.ckpt_nd2nd_data.ckpt_id;
		send_evt.info.cpnd.info.ckpt_nd2nd_data_rsp.from_svc = evt->info.ckpt_nd2nd_data.agent_mdest;
		goto nd_rsp;
	}
	if (cp_node->cpnd_rep_create) {
		rc = cpnd_ckpt_update_replica(cb, cp_node, &evt->info.ckpt_nd2nd_data,
					      evt->info.ckpt_nd2nd_data.type, &err_flag, &errflag);
		if (rc == NCSCC_RC_FAILURE) {
			/* crash the sec_id */
			m_LOG_CPND_FLCL(CPND_CKPT_SECT_WRITE_FAILED, CPND_FC_HDLN, NCSFL_SEV_ERROR,
					cp_node->ckpt_id, err_flag, __FILE__, __LINE__);
		}
	}
	if (rc == NCSCC_RC_SUCCESS) {
		cpnd_proc_ckpt_arrival_info_ntfy(cb, cp_node, &evt->info.ckpt_nd2nd_data, sinfo);
	}
	switch (evt->info.ckpt_nd2nd_data.type) {
	case CPSV_CKPT_ACCESS_WRITE:
		send_evt.info.cpnd.info.ckpt_nd2nd_data_rsp.type = CPSV_DATA_ACCESS_WRITE_RSP;
		send_evt.info.cpnd.info.ckpt_nd2nd_data_rsp.num_of_elmts = evt->info.ckpt_nd2nd_data.num_of_elmts;
		send_evt.info.cpnd.info.ckpt_nd2nd_data_rsp.size = 0;
		send_evt.info.cpnd.info.ckpt_nd2nd_data_rsp.info.write_err_index = NULL;
		break;

	case CPSV_CKPT_ACCESS_OVWRITE:
		send_evt.info.cpnd.info.ckpt_nd2nd_data_rsp.type = CPSV_DATA_ACCESS_OVWRITE_RSP;
		send_evt.info.cpnd.info.ckpt_nd2nd_data_rsp.info.ovwrite_error.error = SA_AIS_OK;
		break;
	}

	send_evt.info.cpnd.info.ckpt_nd2nd_data_rsp.ckpt_id = cp_node->ckpt_id;
	send_evt.info.cpnd.info.ckpt_nd2nd_data_rsp.from_svc = evt->info.ckpt_nd2nd_data.agent_mdest;
 nd_rsp:
	if (evt->info.ckpt_nd2nd_data.all_repl_evt_flag)
		rc = cpnd_mds_msg_send(cb, sinfo->to_svc, sinfo->dest, &send_evt);
	return rc;
}

/****************************************************************************
 * Name          : cpnd_evt_proc_nd2nd_ckpt_active_data_access_rsp
 *
 * Description   : Function to process write/overwrite response
 *                 from other ND to Active ND.
 *
 * Arguments     : CPND_CB *cb - CPND CB pointer
 *                 CPSV_EVT *evt - Received Event structure
 *                 CPSV_SEND_INFO *sinfo - Sender MDS information.
 *
 * Return Values : NCSCC_RC_SUCCESS/Error.
 *
 * Notes         : None.
 *****************************************************************************/
static uns32 cpnd_evt_proc_nd2nd_ckpt_active_data_access_rsp(CPND_CB *cb, CPND_EVT *evt, CPSV_SEND_INFO *sinfo)
{
	uns32 rc = NCSCC_RC_SUCCESS;
	CPND_CKPT_NODE *cp_node = NULL;
	CPSV_CPND_ALL_REPL_EVT_NODE *evt_node = NULL;
	CPSV_EVT rsp_evt;
	SaAisErrorT error = SA_AIS_OK;

	cpnd_ckpt_node_get(cb, evt->info.ckpt_nd2nd_data_rsp.ckpt_id, &cp_node);
	cpnd_evt_node_get(cb, evt->info.ckpt_nd2nd_data_rsp.from_svc, &evt_node);

	memset(&rsp_evt, '\0', sizeof(CPSV_EVT));

	rsp_evt.type = CPSV_EVT_TYPE_CPA;
	rsp_evt.info.cpa.type = CPA_EVT_ND2A_CKPT_DATA_RSP;

	if (cp_node && evt_node) {
		if (cp_node->ckpt_id == evt_node->ckpt_id) {
			if (evt_node->write_rsp_cnt > 0) {
				/*Set the response flag of the dest in dest list from where the response has come */
				CPSV_CPND_UPDATE_DEST *cpnd_mdest_trav = evt_node->cpnd_update_dest_list;

				while (cpnd_mdest_trav) {
					if (m_NCS_NODE_ID_FROM_MDS_DEST(cpnd_mdest_trav->dest) ==
					    m_NCS_NODE_ID_FROM_MDS_DEST(sinfo->dest))
						break;
					else
						cpnd_mdest_trav = cpnd_mdest_trav->next;
				}

				if (cpnd_mdest_trav == NULL) {
					rc = NCSCC_RC_FAILURE;
					m_LOG_CPND_CL(CPND_CKPT_ALLOC_FAILED, CPND_FC_MEMFAIL, NCSFL_SEV_ERROR,
						      __FILE__, __LINE__);
					goto error;
				}

				cpnd_mdest_trav->write_rsp_flag = TRUE;
				evt_node->write_rsp_cnt--;
			}

			if (evt_node->write_rsp_cnt == 0) {

				cpnd_tmr_stop(&evt_node->write_rsp_tmr);
				/*Send OK response to CPA */
				switch (evt->info.ckpt_write.type) {
				case CPSV_DATA_ACCESS_WRITE_RSP:
					rsp_evt.info.cpa.info.sec_data_rsp.type = CPSV_DATA_ACCESS_WRITE_RSP;
					rsp_evt.info.cpa.info.sec_data_rsp.error = SA_AIS_OK;
					break;

				case CPSV_DATA_ACCESS_OVWRITE_RSP:
					rsp_evt.info.cpa.info.sec_data_rsp.type = CPSV_DATA_ACCESS_OVWRITE_RSP;
					rsp_evt.info.cpa.info.sec_data_rsp.info.ovwrite_error.error = SA_AIS_OK;
					break;
				}
				rc = cpnd_mds_send_rsp(cb, &evt_node->sinfo, &rsp_evt);

				/*Remove the all repl event node */
				rc = cpnd_evt_node_del(cb, evt_node);
				/*Free the memory */
				if (evt_node)
					cpnd_allrepl_write_evt_node_free(evt_node);
			}

		} else {
			m_LOG_CPND_FCL(CPND_CKPT_NODE_GET_FAILED, CPND_FC_API, NCSFL_SEV_ERROR,
				       evt->info.ckpt_nd2nd_data.ckpt_id, __FILE__, __LINE__);
			/*Send the try again response to CPA */
			rc = NCSCC_RC_FAILURE;
			error = SA_AIS_ERR_TRY_AGAIN;
			goto error;
		}
	} else {
		m_LOG_CPND_FCL(CPND_CKPT_NODE_GET_FAILED, CPND_FC_API, NCSFL_SEV_ERROR,
			       evt->info.ckpt_nd2nd_data.ckpt_id, __FILE__, __LINE__);
		/*Ckpt ids are not matching */
		rc = NCSCC_RC_FAILURE;
		error = SA_AIS_ERR_NOT_EXIST;
		goto error;
	}
	return rc;

 error:
	if (evt_node != NULL) {
		/*Send Error response to CPA */
		switch (evt->info.ckpt_write.type) {
		case CPSV_DATA_ACCESS_WRITE_RSP:
			rsp_evt.info.cpa.info.sec_data_rsp.type = CPSV_DATA_ACCESS_WRITE_RSP;
			rsp_evt.info.cpa.info.sec_data_rsp.error = error;
			break;

		case CPSV_DATA_ACCESS_OVWRITE_RSP:
			rsp_evt.info.cpa.info.sec_data_rsp.type = CPSV_DATA_ACCESS_OVWRITE_RSP;
			rsp_evt.info.cpa.info.sec_data_rsp.info.ovwrite_error.error = error;
			break;
		}
		rc = cpnd_mds_send_rsp(cb, &evt_node->sinfo, &rsp_evt);

		/*Remove the all repl event node */
		rc = cpnd_evt_node_del(cb, evt_node);
		/*Free the memory */
		cpnd_allrepl_write_evt_node_free(evt_node);
	}

	return rc;
}

/****************************************************************************
 * Name          :  cpnd_evt_proc_ckpt_sync
 *
 * Description   : Function to proces synchronisation request
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
static uns32 cpnd_evt_proc_ckpt_sync(CPND_CB *cb, CPND_EVT *evt, CPSV_SEND_INFO *sinfo)
{

	uns32 rc = NCSCC_RC_SUCCESS;
	CPND_CKPT_NODE *cp_node = NULL;
	CPSV_EVT send_evt;
	memset(&send_evt, '\0', sizeof(CPSV_EVT));

	cpnd_ckpt_node_get(cb, evt->info.ckpt_sync.ckpt_id, &cp_node);

	if (cp_node == NULL) {
		m_LOG_CPND_FCL(CPND_CKPT_NODE_GET_FAILED, CPND_FC_API, NCSFL_SEV_ERROR,
			       evt->info.ckpt_sync.ckpt_id, __FILE__, __LINE__);
		send_evt.info.cpa.info.sync_rsp.error = SA_AIS_ERR_NOT_EXIST;
		goto agent_rsp;
	}

	if (cp_node->is_active_exist != TRUE) {
		send_evt.info.cpa.info.sync_rsp.error = SA_AIS_ERR_NOT_EXIST;
		goto agent_rsp;
	}

	/* CPND REDUNDANCY  */
	if ((TRUE == cp_node->is_restart) || (m_CPND_IS_LOCAL_NODE(&cp_node->active_mds_dest, &cb->cpnd_mdest_id) != 0)) {
		send_evt.info.cpa.info.sync_rsp.error = SA_AIS_ERR_TRY_AGAIN;
		goto agent_rsp;
	}

	if (cp_node->replica_info.n_secs == 0) {
		send_evt.info.cpa.info.sync_rsp.error = SA_AIS_OK;
		goto agent_rsp;
	}

	if (cp_node->replica_info.section_info == NULL) {
		send_evt.info.cpa.info.sync_rsp.error = SA_AIS_ERR_NOT_EXIST;
		goto agent_rsp;
	}

	memset(&send_evt, '\0', sizeof(CPSV_EVT));

	cpnd_transfer_replica(cb, cp_node, evt->info.ckpt_sync.ckpt_id, cp_node->cpnd_dest_list, evt->info.ckpt_sync);

	send_evt.info.cpa.info.sync_rsp.error = SA_AIS_OK;

 agent_rsp:
	send_evt.type = CPSV_EVT_TYPE_CPA;
	send_evt.info.cpa.type = CPA_EVT_ND2A_CKPT_SYNC_RSP;

	if (sinfo->stype == MDS_SENDTYPE_SNDRSP)
		rc = cpnd_mds_send_rsp(cb, sinfo, &send_evt);
	else {
		send_evt.info.cpa.info.sync_rsp.invocation = evt->info.ckpt_sync.invocation;
		send_evt.info.cpa.info.sync_rsp.client_hdl = evt->info.ckpt_sync.client_hdl;
		send_evt.info.cpa.info.sync_rsp.lcl_ckpt_hdl = evt->info.ckpt_sync.lcl_ckpt_hdl;
		rc = cpnd_mds_msg_send(cb, sinfo->to_svc, sinfo->dest, &send_evt);
	}

	return rc;
}

/******************************************************************************
 * Name          : cpnd_evt_proc_arrival_cbreg
 *
 * Description   : Function to process the arrival callback registration 
 *
 * Arguments     : CPND_CB *cb - CPND CB pointer
 *                 CPSV_EVT *evt - Received Event structure
 *                 CPSV_SEND_INFO *sinfo - Sender MDS information.
 *
 * Return Values : NCSCC_RC_SUCCESS/Error
 *
 *****************************************************************************/

static uns32 cpnd_evt_proc_arrival_cbreg(CPND_CB *cb, CPND_EVT *evt, CPSV_SEND_INFO *sinfo)
{
	CPND_CKPT_CLIENT_NODE *cl_node = NULL;
	uns32 rc = NCSCC_RC_SUCCESS;

	cpnd_client_node_get(cb, evt->info.arr_ntfy.client_hdl, &cl_node);
	if (cl_node == NULL) {
		m_LOG_CPND_FCL(CPND_CLIENT_HDL_GET_FAILED, CPND_FC_HDLN, NCSFL_SEV_ERROR,
			       evt->info.arr_ntfy.client_hdl, __FILE__, __LINE__);
		rc = NCSCC_RC_FAILURE;
		return rc;
	}
	cl_node->arrival_cb_flag = TRUE;

	cpnd_restart_set_arrcb(cb, cl_node);

	return rc;
}

/****************************************************************************
 * Name          : cpnd_evt_proc_nd2nd_ckpt_sync_req
 *
 * Description   : Function to process check point sync request  
 *                 from Node directors.
 *
 * Arguments     : CPND_CB *cb - CPND CB pointer
 *                 CPSV_EVT *evt - Received Event structure
 *                 CPSV_SEND_INFO *sinfo - Sender MDS information.
 *
 * Return Values : NCSCC_RC_SUCCESS/Error.
 *
 * Notes         : None.
 *****************************************************************************/
static uns32 cpnd_evt_proc_nd2nd_ckpt_sync_req(CPND_CB *cb, CPND_EVT *evt, CPSV_SEND_INFO *sinfo)
{
	uns32 rc = NCSCC_RC_SUCCESS;
	CPND_CKPT_NODE *cp_node = NULL;
	CPSV_EVT send_evt;
	CPSV_CPND_DEST_INFO dest_list;

	memset(&send_evt, '\0', sizeof(CPSV_EVT));

	send_evt.type = CPSV_EVT_TYPE_CPND;
	send_evt.info.cpnd.type = CPND_EVT_ND2ND_CKPT_ACTIVE_SYNC;
	send_evt.info.cpnd.error = SA_AIS_OK;

	cpnd_ckpt_node_get(cb, evt->info.sync_req.ckpt_id, &cp_node);

	if (cp_node == NULL) {
		m_LOG_CPND_FCL(CPND_CKPT_NODE_GET_FAILED, CPND_FC_API, NCSFL_SEV_ERROR,
			       evt->info.sync_req.ckpt_id, __FILE__, __LINE__);
		send_evt.info.cpnd.error = SA_AIS_ERR_TRY_AGAIN;
		rc = cpnd_mds_send_rsp(cb, sinfo, &send_evt);
		return rc;
	}

	if (cp_node->create_attrib.creationFlags & SA_CKPT_CHECKPOINT_COLLOCATED) {
	send_evt.info.cpnd.info.ckpt_nd2nd_sync.num_of_elmts = cp_node->replica_info.n_secs;
	rc = cpnd_mds_send_rsp(cb, sinfo, &send_evt);
	}

	if ((cp_node->replica_info.n_secs > 0) && (cp_node->replica_info.section_info)) {

		if (evt->info.sync_req.is_ckpt_open) {
			dest_list.dest = sinfo->dest;
			dest_list.next = NULL;
			cpnd_transfer_replica(cb, cp_node, evt->info.sync_req.ckpt_id, &dest_list, evt->info.sync_req);
		} else
			cpnd_transfer_replica(cb, cp_node, evt->info.sync_req.ckpt_id, cp_node->cpnd_dest_list,
					      evt->info.sync_req);
	}

	if (evt->info.sync_req.is_ckpt_open) {
		/* Add the new replica's MDS_DEST to the dest liot of this replica */
		rc = cpnd_ckpt_remote_cpnd_add(cp_node, sinfo->dest);
	}

	return rc;
}

/****************************************************************************
 * Name          : cpnd_evt_proc_nd2nd_ckpt_active_sync
 *
 * Description   : Function to process checkpoint synchronisation   
 *                 request by active Node Director. 
 *
 * Arguments     : CPND_CB *cb - CPND CB pointer
 *                 CPSV_EVT *evt - Received Event structure
 *                 CPSV_SEND_INFO *sinfo - Sender MDS information.
 *
 * Return Values : NCSCC_RC_SUCCESS/Error.
 *
 * Notes         : None.
 *****************************************************************************/
static uns32 cpnd_evt_proc_nd2nd_ckpt_active_sync(CPND_CB *cb, CPND_EVT *evt, CPSV_SEND_INFO *sinfo)
{
	uns32 rc = NCSCC_RC_SUCCESS;
	CPND_CKPT_NODE *cp_node = NULL;
	CPSV_EVT send_evt;
	uns32 err_flag = 0;
	CPSV_EVT des_evt, *out_evt = NULL;
	uns32 errflag = 0;

	memset(&send_evt, '\0', sizeof(CPSV_EVT));

	cpnd_ckpt_node_get(cb, evt->info.ckpt_nd2nd_sync.ckpt_id, &cp_node);
	if (cp_node == NULL) {
		m_LOG_CPND_FCL(CPND_CKPT_NODE_GET_FAILED, CPND_FC_API, NCSFL_SEV_ERROR,
			       evt->info.ckpt_nd2nd_sync.ckpt_id, __FILE__, __LINE__);
		return NCSCC_RC_FAILURE;
	}
	if (cp_node->cpnd_rep_create) {
		if (cpnd_ckpt_update_replica
		    (cb, cp_node, &evt->info.ckpt_nd2nd_sync, evt->info.ckpt_nd2nd_sync.type, &err_flag,
		     &errflag) != NCSCC_RC_SUCCESS)
			cp_node->open_active_sync_tmr.is_active_sync_err = TRUE;

		if (evt->info.ckpt_nd2nd_sync.last_seq == TRUE) {

			if (evt->info.ckpt_nd2nd_sync.ckpt_sync.is_ckpt_open == FALSE) {

				if (m_NCS_NODE_ID_FROM_MDS_DEST(evt->info.ckpt_nd2nd_sync.ckpt_sync.cpa_sinfo.dest) ==
				    m_NCS_NODE_ID_FROM_MDS_DEST(cb->cpnd_mdest_id)) {

					send_evt.type = CPSV_EVT_TYPE_CPA;
					send_evt.info.cpa.type = CPA_EVT_ND2A_CKPT_SYNC_RSP;
					send_evt.info.cpa.info.sync_rsp.error = SA_AIS_OK;

					if (evt->info.ckpt_nd2nd_sync.ckpt_sync.cpa_sinfo.stype == MDS_SENDTYPE_SNDRSP) {
						rc = cpnd_mds_send_rsp(cb,
								       &evt->info.ckpt_nd2nd_sync.ckpt_sync.cpa_sinfo,
								       &send_evt);
					} else {
						send_evt.info.cpa.info.sync_rsp.invocation =
						    evt->info.ckpt_nd2nd_sync.ckpt_sync.invocation;
						send_evt.info.cpa.info.sync_rsp.client_hdl =
						    evt->info.ckpt_nd2nd_sync.ckpt_sync.client_hdl;
						send_evt.info.cpa.info.sync_rsp.lcl_ckpt_hdl =
						    evt->info.ckpt_nd2nd_sync.ckpt_sync.lcl_ckpt_hdl;
						rc = cpnd_mds_msg_send(cb, NCSMDS_SVC_ID_CPA,
								       evt->info.ckpt_nd2nd_sync.ckpt_sync.
								       cpa_sinfo.dest, &send_evt);
					}
				}
			} else if (evt->info.ckpt_nd2nd_sync.ckpt_sync.is_ckpt_open == TRUE) {

				if (m_NCS_NODE_ID_FROM_MDS_DEST(evt->info.ckpt_nd2nd_sync.ckpt_sync.cpa_sinfo.dest) ==
				    m_NCS_NODE_ID_FROM_MDS_DEST(cb->cpnd_mdest_id)) {
					send_evt.type = CPSV_EVT_TYPE_CPA;
					send_evt.info.cpa.type = CPA_EVT_ND2A_CKPT_OPEN_RSP;
					send_evt.info.cpa.info.openRsp.lcl_ckpt_hdl =
					    evt->info.ckpt_nd2nd_sync.ckpt_sync.lcl_ckpt_hdl;
					if (cp_node->open_active_sync_tmr.is_active_sync_err == FALSE) {
						send_evt.info.cpa.info.openRsp.error = SA_AIS_OK;
						send_evt.info.cpa.info.openRsp.gbl_ckpt_hdl = cp_node->ckpt_id;
						send_evt.info.cpa.info.openRsp.addr =
						    cp_node->replica_info.open.info.open.o_addr;
						send_evt.info.cpa.info.openRsp.creation_attr = cp_node->create_attrib;
						if (cp_node->is_active_exist) {
							send_evt.info.cpa.info.openRsp.is_active_exists = TRUE;
							send_evt.info.cpa.info.openRsp.active_dest =
							    cp_node->active_mds_dest;
						}
						m_LOG_CPND_CFFFCL(CPND_CKPT_OPEN_SUCCESS, CPND_FC_API, NCSFL_SEV_INFO,
								  cp_node->ckpt_name.value,
								  evt->info.ckpt_nd2nd_sync.ckpt_sync.client_hdl,
								  cp_node->ckpt_id, cp_node->active_mds_dest, __FILE__,
								  __LINE__);
					} else {
						memset(&des_evt, '\0', sizeof(CPSV_EVT));
						des_evt.type = CPSV_EVT_TYPE_CPD;
						des_evt.info.cpd.type = CPD_EVT_ND2D_CKPT_DESTROY;
						des_evt.info.cpd.info.ckpt_destroy.ckpt_id = cp_node->ckpt_id;
						rc = cpnd_mds_msg_sync_send(cb, NCSMDS_SVC_ID_CPD, cb->cpd_mdest_id,
									    &des_evt, &out_evt, CPSV_WAIT_TIME);
						if (out_evt && out_evt->info.cpnd.info.destroy_ack.error != SA_AIS_OK) {
							m_LOG_CPND_LCL(CPND_CPD_NEW_ACTIVE_DESTROY_FAILED, CPND_FC_EVT,
								       NCSFL_SEV_ERROR,
								       out_evt->info.cpnd.info.destroy_ack.error,
								       __FILE__, __LINE__);
						}
						send_evt.info.cpa.info.openRsp.error = SA_AIS_ERR_TRY_AGAIN;
						m_LOG_CPND_CFCL(CPND_CKPT_OPEN_FAILURE, CPND_FC_API, NCSFL_SEV_ERROR,
								"ckpt_name", des_evt.info.cpd.info.ckpt_destroy.ckpt_id,
								__FILE__, __LINE__);
						if (out_evt)
							cpnd_evt_destroy(out_evt);
					}
					cpnd_tmr_stop(&cp_node->open_active_sync_tmr);
					m_LOG_CPND_FCL(CPND_OPEN_ACTIVE_SYNC_STOP_TMR_SUCCESS, CPND_FC_CKPTINFO,
						       NCSFL_SEV_NOTICE, cp_node->ckpt_id, __FILE__, __LINE__);

					if (evt->info.ckpt_nd2nd_sync.ckpt_sync.cpa_sinfo.stype == MDS_SENDTYPE_SNDRSP) {
						rc = cpnd_mds_send_rsp(cb,
								       &evt->info.ckpt_nd2nd_sync.ckpt_sync.cpa_sinfo,
								       &send_evt);
					} else {
						send_evt.info.cpa.info.openRsp.invocation =
						    evt->info.ckpt_nd2nd_sync.ckpt_sync.invocation;
						rc = cpnd_mds_msg_send(cb, NCSMDS_SVC_ID_CPA,
								       evt->info.ckpt_nd2nd_sync.ckpt_sync.
								       cpa_sinfo.dest, &send_evt);
					}
				}
			}
		}
	}

	return rc;

}

/****************************************************************************
 * Name          :  cpnd_evt_proc_ckpt_read
 *
 * Description   : Function to process saCkptCheckpointRead  
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
static uns32 cpnd_evt_proc_ckpt_read(CPND_CB *cb, CPND_EVT *evt, CPSV_SEND_INFO *sinfo)
{
	uns32 rc = NCSCC_RC_SUCCESS;
	CPND_CKPT_NODE *cp_node = NULL;
	CPSV_EVT send_evt;

	memset(&send_evt, '\0', sizeof(CPSV_EVT));

	cpnd_ckpt_node_get(cb, evt->info.ckpt_read.ckpt_id, &cp_node);

	send_evt.type = CPSV_EVT_TYPE_CPA;
	send_evt.info.cpa.type = CPA_EVT_ND2A_CKPT_DATA_RSP;

	if ((m_NCS_GET_NODE_ID == m_NCS_NODE_ID_FROM_MDS_DEST(evt->info.ckpt_read.agent_mdest)))
		send_evt.info.cpa.info.sec_data_rsp.type = CPSV_DATA_ACCESS_LCL_READ_RSP;
	else
		send_evt.info.cpa.info.sec_data_rsp.type = CPSV_DATA_ACCESS_RMT_READ_RSP;

	if (cp_node == NULL || !(cp_node->is_active_exist)) {
		send_evt.info.cpa.info.sec_data_rsp.num_of_elmts = -1;
		send_evt.info.cpa.info.sec_data_rsp.error = SA_AIS_ERR_NOT_EXIST;
		m_LOG_CPND_FLCL(CPND_CKPT_SECT_READ_FAILED, CPND_FC_HDLN, NCSFL_SEV_ERROR,
				evt->info.ckpt_read.ckpt_id, SA_AIS_ERR_NOT_EXIST, __FILE__, __LINE__);
		goto agent_rsp;
	}

	if (cp_node->is_restart) {
		send_evt.info.cpa.info.sec_data_rsp.num_of_elmts = -1;
		send_evt.info.cpa.info.sec_data_rsp.error = SA_AIS_ERR_TRY_AGAIN;
		m_LOG_CPND_FLCL(CPND_CKPT_SECT_READ_FAILED, CPND_FC_HDLN, NCSFL_SEV_ERROR,
				evt->info.ckpt_read.ckpt_id, SA_AIS_ERR_TRY_AGAIN, __FILE__, __LINE__);
		goto agent_rsp;
	}

	if ((m_CPND_IS_LOCAL_NODE(&cp_node->active_mds_dest, &cb->cpnd_mdest_id) == 0) ||
	    ((m_CPND_IS_COLLOCATED_ATTR_SET(cp_node->create_attrib.creationFlags) == TRUE)
	     && (m_CPND_IS_ALL_REPLICA_ATTR_SET(cp_node->create_attrib.creationFlags) == TRUE)))

		cpnd_ckpt_read_replica(cb, cp_node, &evt->info.ckpt_read, &send_evt);
	else {
		/*Send try again response to agent becoz the active have just changed */
		send_evt.info.cpa.info.sec_data_rsp.num_of_elmts = -1;
		send_evt.info.cpa.info.sec_data_rsp.error = SA_AIS_ERR_TRY_AGAIN;
		send_evt.info.cpa.info.sec_data_rsp.size = 0;
		m_LOG_CPND_FLCL(CPND_CKPT_SECT_READ_FAILED, CPND_FC_HDLN, NCSFL_SEV_ERROR,
				evt->info.ckpt_read.ckpt_id, SA_AIS_ERR_TRY_AGAIN, __FILE__, __LINE__);
	}

 agent_rsp:
	rc = cpnd_mds_send_rsp(cb, sinfo, &send_evt);
	cpnd_proc_free_read_data(&send_evt);
	return rc;

}

/****************************************************************************
 * Name          : cpnd_evt_proc_timer_expiry
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
static uns32 cpnd_evt_proc_timer_expiry(CPND_CB *cb, CPND_EVT *evt)
{
	uns32 rc = NCSCC_RC_SUCCESS;

	switch (evt->info.tmr_info.type) {
	case CPND_TMR_TYPE_RETENTION:
		rc = cpnd_proc_rt_expiry(cb, evt->info.tmr_info.ckpt_id);
		if (rc != NCSCC_RC_SUCCESS) {
			m_LOG_CPND_LCL(CPND_PROC_RT_EXPIRY_FAILED, CPND_FC_API, NCSFL_SEV_ERROR,
				       rc, __FILE__, __LINE__);
		}
		break;
	case CPND_TMR_TYPE_NON_COLLOC_RETENTION:
		rc = cpnd_proc_non_colloc_rt_expiry(cb, evt->info.tmr_info.ckpt_id);
		if (rc != NCSCC_RC_SUCCESS) {
			m_LOG_CPND_LCL(CPND_PROC_RT_EXPIRY_FAILED, CPND_FC_API, NCSFL_SEV_ERROR,
				       rc, __FILE__, __LINE__);
		}
		break;
	case CPND_TMR_TYPE_SEC_EXPI:
		rc = cpnd_proc_sec_expiry(cb, &evt->info.tmr_info);
		if (rc != NCSCC_RC_SUCCESS) {
			m_LOG_CPND_LCL(CPND_PROC_SEC_EXPIRY_FAILED, CPND_FC_API, NCSFL_SEV_ERROR,
				       rc, __FILE__, __LINE__);
		}

		break;

	case CPND_ALL_REPL_RSP_EXPI:
		rc = cpnd_all_repl_rsp_expiry(cb, &evt->info.tmr_info);
		break;
	case CPND_TMR_OPEN_ACTIVE_SYNC:
		rc = cpnd_open_active_sync_expiry(cb, &evt->info.tmr_info);
		if (rc != NCSCC_RC_SUCCESS) {
			m_LOG_CPND_LCL(CPND_OPEN_ACTIVE_SYNC_EXPIRY_FAILED, CPND_FC_API, NCSFL_SEV_ERROR,
				       rc, __FILE__, __LINE__);
		}
		break;

	}
	return rc;
}

/****************************************************************************
 * Name          : cpnd_evt_proc_mds_evt
 *
 * Description   : Function to process the Events received from MDS
 *
 * Arguments     : CPND_CB *cb - CPND CB pointer
 *                 CPSV_EVT *evt - Received Event structure
 *
 * Return Values : NCSCC_RC_SUCCESS/Error.
 *
 * Notes         : None.
 *****************************************************************************/
static uns32 cpnd_evt_proc_mds_evt(CPND_CB *cb, CPND_EVT *evt)
{
	uns32 rc = NCSCC_RC_SUCCESS;

	if ((evt->info.mds_info.change == NCSMDS_DOWN) && evt->info.mds_info.svc_id == NCSMDS_SVC_ID_CPA) {
		cpnd_proc_cpa_down(cb, evt->info.mds_info.dest);
	} else if ((evt->info.mds_info.change == NCSMDS_DOWN) && evt->info.mds_info.svc_id == NCSMDS_SVC_ID_CPD) {
		cpnd_proc_cpd_down(cb);
	} else if ((evt->info.mds_info.change == NCSMDS_UP) && evt->info.mds_info.svc_id == NCSMDS_SVC_ID_CPA) {
		cpnd_proc_cpa_up(cb, evt->info.mds_info.dest);
	} else if ((evt->info.mds_info.change == NCSMDS_RED_UP) &&
		   (evt->info.mds_info.role == V_DEST_RL_ACTIVE) && (evt->info.mds_info.svc_id == NCSMDS_SVC_ID_CPD)) {
		cpnd_proc_cpd_new_active(cb);
	} else if ((evt->info.mds_info.change == NCSMDS_CHG_ROLE) &&
		   (evt->info.mds_info.role == V_DEST_RL_ACTIVE) && (evt->info.mds_info.svc_id == NCSMDS_SVC_ID_CPD)) {
		cpnd_proc_cpd_new_active(cb);
	}

	return rc;
}

/****************************************************************************
 * Name          : cpnd_proc_cpd_new_active
 *
 * Description   : Function to send all deferred i.e. timed out CPD requests
 *                 on reception of NEW_ACTIVE
 *
 * Arguments     : CPND_CB *cb - CPND CB pointer
 *
 * Return Values : NCSCC_RC_SUCCESS/Error.
 *
 * Notes         : None.
 *****************************************************************************/
static uns32 cpnd_proc_cpd_new_active(CPND_CB *cb)
{
	CPND_CPD_DEFERRED_REQ_NODE *node = NULL;
	uns32 rc = NCSCC_RC_SUCCESS;
	CPSV_EVT *out_evt = NULL;

	node = (CPND_CPD_DEFERRED_REQ_NODE *)ncs_dequeue(&cb->cpnd_cpd_deferred_reqs_list);

	while (node) {

		out_evt = NULL;

		rc = cpnd_mds_msg_sync_send(cb, NCSMDS_SVC_ID_CPD, cb->cpd_mdest_id, &node->evt, &out_evt,
					    CPSV_WAIT_TIME);

		if (rc != NCSCC_RC_SUCCESS) {
			/* put back the event into the deferred requests queue */
			m_LOG_CPND_LCL(CPND_SYNC_SEND_TO_CPD_FAILED, CPND_FC_MDSFAIL, NCSFL_SEV_ERROR,
				       rc, __FILE__, __LINE__);
			ncs_enqueue(&cb->cpnd_cpd_deferred_reqs_list, (void *)node);
			return rc;
		}

		switch (node->evt.info.cpd.type) {

		case CPD_EVT_ND2D_CKPT_DESTROY:
			if (out_evt && out_evt->info.cpnd.info.destroy_ack.error != SA_AIS_OK) {
				m_LOG_CPND_LCL(CPND_CPD_NEW_ACTIVE_DESTROY_FAILED, CPND_FC_EVT, NCSFL_SEV_ERROR,
					       out_evt->info.cpnd.info.destroy_ack.error, __FILE__, __LINE__);
			}
			break;
		case CPD_EVT_ND2D_CKPT_DESTROY_BYNAME:
			if (out_evt && out_evt->info.cpnd.info.destroy_ack.error != SA_AIS_OK) {
				m_LOG_CPND_LCL(CPND_CPD_NEW_ACTIVE_DESTROY_BYNAME_FAILED, CPND_FC_EVT, NCSFL_SEV_ERROR,
					       out_evt->info.cpnd.info.destroy_ack.error, __FILE__, __LINE__);
			}
			break;

		case CPD_EVT_ND2D_CKPT_UNLINK:
			if (out_evt && out_evt->info.cpnd.info.ulink_ack.error != SA_AIS_OK) {
				m_LOG_CPND_LCL(CPND_CPD_NEW_ACTIVE_UNLINK_FAILED, CPND_FC_EVT, NCSFL_SEV_ERROR,
					       out_evt->info.cpnd.info.ulink_ack.error, __FILE__, __LINE__);
			}

			break;

		case CPD_EVT_ND2D_CKPT_RDSET:
			if (out_evt && out_evt->info.cpnd.info.rdset_ack.error != SA_AIS_OK) {
				m_LOG_CPND_LCL(CPND_CPD_NEW_ACTIVE_RDSET_FAILED, CPND_FC_EVT, NCSFL_SEV_ERROR,
					       out_evt->info.cpnd.info.rdset_ack.error, __FILE__, __LINE__);

			}
			break;

		case CPD_EVT_ND2D_ACTIVE_SET:
			if (out_evt && out_evt->info.cpnd.info.arep_ack.error != SA_AIS_OK) {
				m_LOG_CPND_LCL(CPND_CPD_NEW_ACTIVE_AREP_SET_FAILED, CPND_FC_EVT, NCSFL_SEV_ERROR,
					       out_evt->info.cpnd.info.arep_ack.error, __FILE__, __LINE__);
			}
			break;

		default:
			break;
		}

		if (node)
			m_MMGR_FREE_CPND_CPD_DEFERRED_REQ_NODE(node);

		node = NULL;

		node = (CPND_CPD_DEFERRED_REQ_NODE *)ncs_dequeue(&cb->cpnd_cpd_deferred_reqs_list);
	}

	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
 * Name          : cpnd_evt_proc_ckpt_destroy
 *
 * Description   : Function to process check point destroy   
 *
 * Arguments     : CPND_CB *cb - CPND CB pointer
 *                 CPSV_EVT *evt - Received Event structure
 *                 CPSV_SEND_INFO *sinfo - Sender MDS information.
 *
 * Return Values : NCSCC_RC_SUCCESS/Error.
 *
 * Notes         : None.
 *****************************************************************************/
static pthread_mutex_t ckpt_destroy_lock = PTHREAD_MUTEX_INITIALIZER;
static uns32 cpnd_evt_proc_ckpt_destroy(CPND_CB *cb, CPND_EVT *evt, CPSV_SEND_INFO *sinfo)
{
	uns32 rc = NCSCC_RC_SUCCESS;
	CPND_CKPT_NODE *cp_node = NULL;

	(void)pthread_mutex_lock(&ckpt_destroy_lock);
	cpnd_ckpt_node_get(cb, evt->info.ckpt_destroy.ckpt_id, &cp_node);
	if (cp_node == NULL) {
		m_LOG_CPND_FCL(CPND_CKPT_NODE_GET_FAILED, CPND_FC_API, NCSFL_SEV_ERROR,
			       evt->info.ckpt_destroy.ckpt_id, __FILE__, __LINE__);
		(void)pthread_mutex_unlock(&ckpt_destroy_lock);
		return NCSCC_RC_SUCCESS;
	}

	if (cp_node->cpnd_rep_create) {
		/* Free back pointers from client list and ckpt_list */
		cpnd_ckpt_delete_all_sect(cp_node);

		/* need to destroy only the shm info,no need to send to director */
		{
			NCS_OS_POSIX_SHM_REQ_INFO shm_info;
			uns32 rc = NCSCC_RC_SUCCESS;

			/* size of chkpt */
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
			}

			/* unlink the name */
			shm_info.type = NCS_OS_POSIX_SHM_REQ_UNLINK;
			shm_info.info.unlink.i_name = cp_node->replica_info.open.info.open.i_name;

			rc = ncs_os_posix_shm(&shm_info);
			if (rc == NCSCC_RC_FAILURE) {

				m_LOG_CPND_FCL(CPND_CKPT_UNLINK_FAILED, CPND_FC_SYS_CALL, NCSFL_SEV_ERROR,
					       cp_node->ckpt_id, __FILE__, __LINE__);
			}

			if (cb->num_rep) {
				cb->num_rep--;
			}

			m_MMGR_FREE_CPND_DEFAULT(cp_node->replica_info.open.info.open.i_name);
			/* freeing the sec_mapping memory */
			m_MMGR_FREE_CPND_DEFAULT(cp_node->replica_info.shm_sec_mapping);

		}

		m_LOG_CPND_FCL(CPND_CKPT_REPLICA_DESTROY_SUCCESS, CPND_FC_GENERIC, NCSFL_SEV_INFO,
			       cp_node->ckpt_id, __FILE__, __LINE__);
		cpnd_restart_shm_ckpt_free(cb, cp_node);
		cpnd_ckpt_node_destroy(cb, cp_node);
	}
	(void)pthread_mutex_unlock(&ckpt_destroy_lock);
	return rc;
}

/****************************************************************************
 * Name          : cpnd_evt_proc_ckpt_create
 *
 * Description   : Function to process checkpoint create(for non-collocated)
 *
 * Arguments     : CPND_CB *cb - CPND CB pointer
 *                 CPSV_EVT *evt - Received Event structure
 *                 CPSV_SEND_INFO *sinfo - Sender MDS information.
 *
 * Return Values : NCSCC_RC_SUCCESS/Error.
 *
 * Notes         : None.
 *****************************************************************************/
static uns32 cpnd_evt_proc_ckpt_create(CPND_CB *cb, CPND_EVT *evt, CPSV_SEND_INFO *sinfo)
{
	uns32 rc = NCSCC_RC_SUCCESS;
	CPND_CKPT_NODE *cp_node = NULL;
	SaCkptHandleT client_hdl = 0;
	CPSV_EVT send_evt;

	/* Check if this ckpt is created by someone on this node */
	cpnd_ckpt_node_get(cb, evt->info.ckpt_create.ckpt_info.ckpt_id, &cp_node);
	if (cp_node != NULL) {
		/* D2ND EVENT will have rep_create as TRUE */
		cp_node->cpnd_rep_create = TRUE;
	} else {
		cp_node = m_MMGR_ALLOC_CPND_CKPT_NODE;
		if (cp_node == NULL) {
			m_LOG_CPND_FCL(CPND_CKPT_ALLOC_FAILED, CPND_FC_MEMFAIL, NCSFL_SEV_ERROR,
				       evt->info.ckpt_create.ckpt_info.ckpt_id, __FILE__, __LINE__);
			rc = NCSCC_RC_FAILURE;
			return rc;
		}

		memset(cp_node, '\0', sizeof(CPND_CKPT_NODE));

		cp_node->clist = NULL;
		cp_node->cpnd_dest_list = NULL;
		cp_node->ckpt_name = evt->info.ckpt_create.ckpt_name;
		cp_node->create_attrib = evt->info.ckpt_create.ckpt_info.attributes;
		cp_node->ckpt_id = evt->info.ckpt_create.ckpt_info.ckpt_id;

		if (evt->info.ckpt_create.ckpt_info.is_active_exists == TRUE) {
			cp_node->active_mds_dest = evt->info.ckpt_create.ckpt_info.active_dest;
			cp_node->is_active_exist = TRUE;
		}

		/* ref count and client info is needed for non-collocated cases */
		cp_node->offset = SHM_INIT;
		cp_node->is_close = FALSE;
		cp_node->is_unlink = FALSE;
		cp_node->cpnd_rep_create = evt->info.ckpt_create.ckpt_info.ckpt_rep_create;
		cp_node->is_ckpt_onscxb = TRUE;
		/*   cp_node->ckpt_lcl_ref_cnt++; */

		cpnd_restart_shm_ckpt_update(cb, cp_node, client_hdl);

		if (cpnd_ckpt_node_add(cb, cp_node) == NCSCC_RC_FAILURE) {
			/* THE BELOW LOG HAS TO BE CHANGED */
			m_LOG_CPND_FCL(CPND_CKPT_NODE_ADDITION_FAILED, CPND_FC_CKPTINFO, NCSFL_SEV_ERROR,
				       cp_node->ckpt_id, __FILE__, __LINE__);
			rc = NCSCC_RC_FAILURE;
			m_MMGR_FREE_CPND_CKPT_NODE(cp_node);
			return rc;
		}
		{
			CPSV_CPND_DEST_INFO *tmp = NULL;
			uns32 dst_cnt = 0;
			tmp = evt->info.ckpt_create.ckpt_info.dest_list;

			while (dst_cnt < evt->info.ckpt_create.ckpt_info.dest_cnt) {
				if (m_CPND_IS_LOCAL_NODE(&tmp[dst_cnt].dest, &cb->cpnd_mdest_id) != 0) {
					cpnd_ckpt_remote_cpnd_add(cp_node, tmp[dst_cnt].dest);
				}
				dst_cnt++;
			}

		}

		if (evt->info.ckpt_create.ckpt_info.ckpt_rep_create == TRUE) {
			rc = cpnd_ckpt_replica_create(cb, cp_node);
			if (rc == NCSCC_RC_FAILURE) {
				m_LOG_CPND_FLCL(CPND_CKPT_REP_CREATE_FAILED, CPND_FC_GENERIC, NCSFL_SEV_ERROR,
						cp_node->ckpt_id, rc, __FILE__, __LINE__);
				goto ckpt_replica_create_failed;
			}
			rc = cpnd_ckpt_hdr_update(cp_node);
			if (rc == NCSCC_RC_FAILURE) {
				m_LOG_CPND_FLCL(CPND_CKPT_HDR_UPDATE_FAILED, CPND_FC_HDLN, NCSFL_SEV_ERROR,
						cp_node->ckpt_id, rc, __FILE__, __LINE__);
			}

		}
		if (evt->info.ckpt_create.ckpt_info.ckpt_rep_create == TRUE && cp_node->create_attrib.maxSections == 1) {
			SaCkptSectionIdT sec_id = SA_CKPT_DEFAULT_SECTION_ID;
			cpnd_ckpt_sec_add(cp_node, &sec_id, 0, 0);
		}
		goto end;
	}

	if (evt->info.ckpt_create.ckpt_info.ckpt_rep_create == TRUE) {
		rc = cpnd_ckpt_replica_create(cb, cp_node);
		if (rc == NCSCC_RC_FAILURE) {
			m_LOG_CPND_LCL(CPND_CKPT_REP_CREATE_FAILED, CPND_FC_GENERIC, NCSFL_SEV_ERROR,
				       rc, __FILE__, __LINE__);
			goto ckpt_replica_create_failed;
		}
		rc = cpnd_ckpt_hdr_update(cp_node);
		if (rc == NCSCC_RC_FAILURE) {
			m_LOG_CPND_LCL(CPND_CKPT_HDR_UPDATE_FAILED, CPND_FC_HDLN, NCSFL_SEV_ERROR,
				       rc, __FILE__, __LINE__);
		}
	}

 end:

	memset(&send_evt, '\0', sizeof(CPSV_EVT));

	send_evt.type = CPSV_EVT_TYPE_CPND;
	send_evt.info.cpnd.type = CPND_EVT_ND2ND_CKPT_SYNC_REQ;
	send_evt.info.cpnd.info.sync_req.ckpt_id = cp_node->ckpt_id;


	rc = cpnd_mds_msg_send(cb, NCSMDS_SVC_ID_CPND, cp_node->active_mds_dest, &send_evt);

	if (rc != NCSCC_RC_SUCCESS) {
		m_LOG_CPND_FFFLCL(CPND_REMOTE_TO_ACTIVE_MDS_SEND_FAIL, CPND_FC_MDSFAIL, NCSFL_SEV_ERROR,
				  cb->cpnd_mdest_id, cp_node->active_mds_dest, cp_node->ckpt_id, rc, __FILE__,
				  __LINE__);

		goto ckpt_replica_create_failed;
	}

	m_LOG_CPND_FCL(CPND_NON_COLLOC_CKPT_REPLICA_CREATE_SUCCESS, CPND_FC_CKPTINFO, NCSFL_SEV_INFO,
		       cp_node->ckpt_id, __FILE__, __LINE__);
	return rc;

 ckpt_replica_create_failed:
	cpnd_ckpt_node_destroy(cb, cp_node);

	return rc;

}

/**********************************************************************
 * Name          : cpnd_evt_proc_ckpt_sect_iter_req
 *
 * Description   :
 *
 * Arguments     :
 *
**********************************************************************/
/* 3 */
static uns32 cpnd_evt_proc_ckpt_sect_iter_req(CPND_CB *cb, CPND_EVT *evt, CPSV_SEND_INFO *sinfo)
{
	uns32 rc = SA_AIS_OK;
	CPND_CKPT_NODE *cp_node = NULL;
	CPSV_EVT send_evt;

	memset(&send_evt, '\0', sizeof(CPSV_EVT));

	cpnd_ckpt_node_get(cb, evt->info.sec_iter_req.ckpt_id, &cp_node);
	if (cp_node == NULL) {
		m_LOG_CPND_FCL(CPND_CKPT_NODE_GET_FAILED, CPND_FC_API, NCSFL_SEV_ERROR,
			       evt->info.sec_iter_req.ckpt_id, __FILE__, __LINE__);
		rc = SA_AIS_ERR_NOT_EXIST;
		goto agent_rsp;
	}

	if (TRUE == cp_node->is_restart) {
		rc = SA_AIS_ERR_TRY_AGAIN;
		goto agent_rsp;
	}
	if (cp_node->is_active_exist != TRUE) {
		rc = SA_AIS_ERR_NOT_EXIST;
	}

 agent_rsp:
	send_evt.type = CPSV_EVT_TYPE_CPA;
	send_evt.info.cpa.type = CPA_EVT_ND2A_SEC_ITER_RSP;
	send_evt.info.cpa.info.sec_iter_rsp.error = rc;
	rc = cpnd_mds_send_rsp(cb, sinfo, &send_evt);
	return rc;
}

/****************************************************************************
 * Name          : cpnd_evt_proc_ckpt_iter_getnext
 *
 * Description   : Function to process saCkptSectionIterationNext  
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
static uns32 cpnd_evt_proc_ckpt_iter_getnext(CPND_CB *cb, CPND_EVT *evt, CPSV_SEND_INFO *sinfo)
{

	uns32 rc = NCSCC_RC_SUCCESS;
	CPND_CKPT_NODE *cp_node = NULL;
	CPSV_EVT send_evt;
	SaCkptSectionDescriptorT sect_desc;
	uns32 num_secs_trav = 0;

/*  evt contain filter iter_id section_id ckpt_id */
	memset(&send_evt, '\0', sizeof(CPSV_EVT));
	memset(&sect_desc, '\0', sizeof(SaCkptSectionDescriptorT));

	cpnd_ckpt_node_get(cb, evt->info.iter_getnext.ckpt_id, &cp_node);
	if (cp_node == NULL) {
		m_LOG_CPND_FCL(CPND_CKPT_NODE_GET_FAILED, CPND_FC_API, NCSFL_SEV_ERROR,
			       evt->info.iter_getnext.ckpt_id, __FILE__, __LINE__);
		rc = SA_AIS_ERR_NOT_EXIST;
		goto agent_rsp;
	}

	if (TRUE == cp_node->is_restart) {
		rc = SA_AIS_ERR_TRY_AGAIN;
		goto agent_rsp;
	}

	if (cp_node->is_active_exist != TRUE) {
		rc = SA_AIS_ERR_NOT_EXIST;
		goto agent_rsp;
	}

	if ((m_CPND_IS_LOCAL_NODE(&cp_node->active_mds_dest, &cb->cpnd_mdest_id) == 0) ||
	    ((m_CPND_IS_COLLOCATED_ATTR_SET(cp_node->create_attrib.creationFlags) == TRUE)
	     && (m_CPND_IS_ALL_REPLICA_ATTR_SET(cp_node->create_attrib.creationFlags) == TRUE))) {
		if (cpnd_proc_getnext_section(cp_node, &evt->info.iter_getnext, &sect_desc, &num_secs_trav)
		    != NCSCC_RC_SUCCESS) {
			rc = SA_AIS_ERR_NO_SECTIONS;
			goto agent_rsp;
		}

		memcpy(&send_evt.info.cpa.info.iter_next_rsp.sect_desc, &sect_desc, sizeof(SaCkptSectionDescriptorT));
		/* need to cpy sec_id */
		rc = SA_AIS_OK;
	} else {
		rc = SA_AIS_ERR_TRY_AGAIN;
		goto agent_rsp;
	}
 agent_rsp:

	send_evt.type = CPSV_EVT_TYPE_CPA;
	send_evt.info.cpa.type = CPA_EVT_ND2A_SEC_ITER_GETNEXT_RSP;
	send_evt.info.cpa.info.iter_next_rsp.ckpt_id = evt->info.iter_getnext.ckpt_id;
	send_evt.info.cpa.info.iter_next_rsp.error = rc;
	send_evt.info.cpa.info.iter_next_rsp.iter_id = evt->info.iter_getnext.iter_id;
	send_evt.info.cpa.info.iter_next_rsp.n_secs_trav = num_secs_trav;

	rc = cpnd_mds_send_rsp(cb, sinfo, &send_evt);

	return rc;

}

/*************************************************************
 * Name : cpnd_evt_proc_ckpt_iter_next_req
 *
 * output : send the section descriptor 
 ************************************************************/
static uns32 cpnd_evt_proc_ckpt_iter_next_req(CPND_CB *cb, CPND_EVT *evt, CPSV_SEND_INFO *sinfo)
{
	SaAisErrorT rc = SA_AIS_OK;
	CPND_CKPT_NODE *cp_node = NULL;
	CPSV_EVT send_evt;
	SaCkptSectionDescriptorT sect_desc;
	uns32 num_secs_trav = 0;

/*  evt contain filter iter_id section_id ckpt_id */
	memset(&send_evt, '\0', sizeof(CPSV_EVT));
	memset(&sect_desc, '\0', sizeof(SaCkptSectionDescriptorT));

	cpnd_ckpt_node_get(cb, evt->info.iter_getnext.ckpt_id, &cp_node);
	if (cp_node == NULL) {
		m_LOG_CPND_FCL(CPND_CKPT_NODE_GET_FAILED, CPND_FC_API, NCSFL_SEV_ERROR,
			       evt->info.iter_getnext.ckpt_id, __FILE__, __LINE__);
		rc = SA_AIS_ERR_TRY_AGAIN;
		goto end;
	}

	if (cpnd_proc_getnext_section(cp_node, &evt->info.iter_getnext, &sect_desc, &num_secs_trav)
	    != NCSCC_RC_SUCCESS) {
		rc = SA_AIS_ERR_NO_SECTIONS;
	}

 end:
	send_evt.type = CPSV_EVT_TYPE_CPND;
	send_evt.info.cpnd.type = CPND_EVT_ND2ND_CKPT_ACTIVE_ITERNEXT;
	if (rc == SA_AIS_OK) {
		send_evt.info.cpnd.info.ckpt_nd2nd_getnext_rsp.sect_desc = sect_desc;
		send_evt.info.cpnd.info.ckpt_nd2nd_getnext_rsp.error = rc;
		send_evt.info.cpnd.info.ckpt_nd2nd_getnext_rsp.n_secs_trav = num_secs_trav;
	} else
		send_evt.info.cpnd.info.ckpt_nd2nd_getnext_rsp.error = rc;

	rc = cpnd_mds_send_rsp(cb, sinfo, &send_evt);
	return rc;
}

static uns32 cpnd_evt_proc_ckpt_refcntset(CPND_CB *cb,CPND_EVT *evt)
{

   uns32                   rc = NCSCC_RC_SUCCESS,i=0;
   CPND_CKPT_NODE          *cp_node=NULL;

 for(i=0;i<evt->info.refCntsetReq.no_of_nodes;i++)
  {
   cpnd_ckpt_node_get(cb,evt->info.refCntsetReq.ref_cnt_array[i].ckpt_id,&cp_node);

   if (cp_node == NULL)
   {
      m_LOG_CPND_FCL(CPND_CKPT_NODE_GET_FAILED,CPND_FC_API,NCSFL_SEV_ERROR,\
        evt->info.refCntsetReq.ref_cnt_array[i].ckpt_id,__FILE__,__LINE__);
      rc = NCSCC_RC_FAILURE;
   }
   else
   {
       cp_node->ckpt_lcl_ref_cnt = (cp_node->ckpt_lcl_ref_cnt + evt->info.refCntsetReq.ref_cnt_array[i].ckpt_ref_cnt);
   }
  }
   return rc;

}

/****************************************************************************
 * Name          : cpnd_evt_proc_cb_dump
 * Description   : Function to dump the Control Block
 * Arguments     : CPND_CB *cb - CPND CB pointer
 * Return Values : NCSCC_RC_SUCCESS/Error.
 * Notes         : None.
 *****************************************************************************/
static uns32 cpnd_evt_proc_cb_dump(CPND_CB *cb)
{
	cpnd_cb_dump();

	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
 * Name          : cpnd_evt_destroy
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
#define m_MMGR_FREE_CPSV_SYS_MEMORY(p)  m_NCS_MEM_FREE(p, \
            NCS_MEM_REGION_PERSISTENT, NCS_SERVICE_ID_OS_SVCS, 0)

#define m_MMGR_FREE_CPSV_DEFAULT(p) m_NCS_MEM_FREE(p, \
           NCS_MEM_REGION_PERSISTENT, NCS_SERVICE_ID_OS_SVCS, \
           0)

uns32 cpnd_evt_destroy(CPSV_EVT *evt)
{
	uns32 rc = NCSCC_RC_SUCCESS;

	if (evt == NULL) {
		/* LOG */
		return NCSCC_RC_SUCCESS;
	}

	if (evt->info.cpnd.dont_free_me == TRUE)
		return NCSCC_RC_SUCCESS;

	if (evt->type != CPSV_EVT_TYPE_CPND)
		return NCSCC_RC_SUCCESS;

	if (evt->info.cpnd.type == CPND_EVT_D2ND_CKPT_INFO) {
		if (evt->info.cpnd.info.ckpt_info.dest_list != NULL)
			m_MMGR_FREE_CPSV_SYS_MEMORY(evt->info.cpnd.info.ckpt_info.dest_list);
	} else if (evt->info.cpnd.type == CPND_EVT_D2ND_CKPT_CREATE) {
		if (evt->info.cpnd.info.ckpt_create.ckpt_info.dest_list != NULL)
			m_MMGR_FREE_CPSV_SYS_MEMORY(evt->info.cpnd.info.ckpt_create.ckpt_info.dest_list);
	} else if (evt->info.cpnd.type == CPSV_D2ND_RESTART_DONE) {
		if (evt->info.cpnd.info.cpnd_restart_done.dest_list != NULL)
			m_MMGR_FREE_CPSV_SYS_MEMORY(evt->info.cpnd.info.cpnd_restart_done.dest_list);
	} else if (evt->info.cpnd.type == CPND_EVT_D2ND_CKPT_REP_ADD) {
		if (evt->info.cpnd.info.ckpt_add.dest_list != NULL)
			m_MMGR_FREE_CPSV_SYS_MEMORY(evt->info.cpnd.info.ckpt_add.dest_list);
	} else if (evt->info.cpnd.type == CPND_EVT_A2ND_CKPT_SECT_CREATE) {
		if (evt->info.cpnd.info.sec_creatReq.init_data != NULL
		    && evt->info.cpnd.info.sec_creatReq.init_size != 0) {
			m_MMGR_FREE_CPSV_DEFAULT_VAL(evt->info.cpnd.info.sec_creatReq.init_data, NCS_SERVICE_ID_CPND);
		}

		/*Bug 58195 (sol)- " sectionid free " macro added for SectionCreate  which was not there earlier */
		if (evt->info.cpnd.info.sec_creatReq.sec_attri.sectionId) {
			if (evt->info.cpnd.info.sec_creatReq.sec_attri.sectionId->id != NULL &&
			    evt->info.cpnd.info.sec_creatReq.sec_attri.sectionId->idLen != 0) {
				m_MMGR_FREE_CPSV_DEFAULT_VAL(evt->info.cpnd.info.sec_creatReq.sec_attri.sectionId->id,
							     NCS_SERVICE_ID_CPND);
			}
			m_MMGR_FREE_CPSV_SaCkptSectionIdT(evt->info.cpnd.info.sec_creatReq.sec_attri.sectionId,
							  NCS_SERVICE_ID_CPND);
		}
	} else if (evt->info.cpnd.type == CPND_EVT_A2ND_CKPT_SECT_DELETE) {
		if (evt->info.cpnd.info.sec_delReq.sec_id.id != NULL
		    && evt->info.cpnd.info.sec_delReq.sec_id.idLen != 0)
			m_MMGR_FREE_CPSV_DEFAULT_VAL(evt->info.cpnd.info.sec_delReq.sec_id.id, NCS_SERVICE_ID_CPND);
	} else if (evt->info.cpnd.type == CPND_EVT_A2ND_CKPT_ITER_GETNEXT) {
		if (evt->info.cpnd.info.iter_getnext.section_id.id != NULL
		    && evt->info.cpnd.info.iter_getnext.section_id.idLen != 0)
			m_MMGR_FREE_CPSV_DEFAULT_VAL(evt->info.cpnd.info.iter_getnext.section_id.id,
						     NCS_SERVICE_ID_CPND);
	} else if (evt->info.cpnd.type == CPND_EVT_A2ND_CKPT_SECT_EXP_SET) {
		if (evt->info.cpnd.info.sec_expset.sec_id.id != NULL
		    && evt->info.cpnd.info.sec_expset.sec_id.idLen != 0)
			m_MMGR_FREE_CPSV_DEFAULT_VAL(evt->info.cpnd.info.sec_expset.sec_id.id, NCS_SERVICE_ID_CPND);
	} else if (evt->info.cpnd.type == CPSV_EVT_ND2ND_CKPT_SECT_EXPTMR_REQ) {
		if (evt->info.cpnd.info.sec_exp_set.sec_id.id != NULL
		    && evt->info.cpnd.info.sec_exp_set.sec_id.idLen != 0)
			m_MMGR_FREE_CPSV_DEFAULT_VAL(evt->info.cpnd.info.sec_exp_set.sec_id.id, NCS_SERVICE_ID_CPND);
	} else if (evt->info.cpnd.type == CPSV_EVT_ND2ND_CKPT_SECT_CREATE_REQ) {
		if (evt->info.cpnd.info.active_sec_creat.init_data != NULL
		    && evt->info.cpnd.info.active_sec_creat.init_size != 0)
			m_MMGR_FREE_CPSV_DEFAULT_VAL(evt->info.cpnd.info.active_sec_creat.init_data,
						     NCS_SERVICE_ID_CPND);
		/*free the sectionId */
		if (evt->info.cpnd.info.active_sec_creat.sec_attri.sectionId) {
			if (evt->info.cpnd.info.active_sec_creat.sec_attri.sectionId->id != NULL
			    && evt->info.cpnd.info.active_sec_creat.sec_attri.sectionId->idLen != 0)
				m_MMGR_FREE_CPSV_DEFAULT_VAL(evt->info.cpnd.info.active_sec_creat.sec_attri.
							     sectionId->id, NCS_SERVICE_ID_CPND);

			m_MMGR_FREE_CPSV_SaCkptSectionIdT(evt->info.cpnd.info.sec_creatReq.sec_attri.sectionId,
							  NCS_SERVICE_ID_CPND);
		}
	} else if (evt->info.cpnd.type == CPSV_EVT_ND2ND_CKPT_SECT_ACTIVE_CREATE_RSP) {
		if (evt->info.cpnd.info.active_sec_creat_rsp.sec_id.id != NULL
		    && evt->info.cpnd.info.active_sec_creat_rsp.sec_id.idLen != 0) {
			m_MMGR_FREE_CPSV_DEFAULT_VAL(evt->info.cpnd.info.active_sec_creat_rsp.sec_id.id,
						     NCS_SERVICE_ID_CPND);
		}
	} else if (evt->info.cpnd.type == CPSV_EVT_ND2ND_CKPT_SECT_DELETE_REQ) {
		if (evt->info.cpnd.info.sec_delete_req.sec_id.id != NULL
		    && evt->info.cpnd.info.sec_delete_req.sec_id.idLen != 0)
			m_MMGR_FREE_CPSV_DEFAULT_VAL(evt->info.cpnd.info.sec_delete_req.sec_id.id, NCS_SERVICE_ID_CPND);
	}

	else if (evt->info.cpnd.type == CPND_EVT_A2ND_CKPT_WRITE) {
		if (evt->info.cpnd.info.ckpt_write.data != NULL) {
			CPSV_CKPT_DATA *tmp_data, *next_data;
			tmp_data = evt->info.cpnd.info.ckpt_write.data;
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
	} else if (evt->info.cpnd.type == CPND_EVT_A2ND_CKPT_READ) {
		if (evt->info.cpnd.info.ckpt_read.data != NULL) {
			CPSV_CKPT_DATA *tmp_data, *next_data;
			tmp_data = evt->info.cpnd.info.ckpt_read.data;
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
	} else if (evt->info.cpnd.type == CPND_EVT_ND2ND_CKPT_ACTIVE_SYNC) {
		if (evt->info.cpnd.info.ckpt_nd2nd_sync.data != NULL) {
			CPSV_CKPT_DATA *tmp_data, *next_data;
			tmp_data = evt->info.cpnd.info.ckpt_nd2nd_sync.data;
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
	}
	/*else if ( evt->info.cpnd.type == CPSV_EVT_ND2ND_CKPT_SECT_DATA_ACCESS_REQ  || \ */
	else if (evt->info.cpnd.type == CPSV_EVT_ND2ND_CKPT_SECT_ACTIVE_DATA_ACCESS_REQ) {
		if (evt->info.cpnd.info.ckpt_nd2nd_data.data != NULL) {
			CPSV_CKPT_DATA *tmp_data, *next_data;
			tmp_data = evt->info.cpnd.info.ckpt_nd2nd_data.data;
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
	} else if (evt->info.cpnd.type == CPSV_EVT_ND2ND_CKPT_SECT_ACTIVE_DATA_ACCESS_RSP) {
		switch (evt->info.cpnd.info.ckpt_nd2nd_data_rsp.type) {
		case CPSV_DATA_ACCESS_WRITE_RSP:
			if (evt->info.cpnd.info.ckpt_nd2nd_data_rsp.info.write_err_index != NULL)
				m_MMGR_FREE_CPSV_SaUint32T(evt->info.cpnd.info.ckpt_nd2nd_data_rsp.info.write_err_index,
							   NCS_SERVICE_ID_CPA);
			break;
		case CPSV_DATA_ACCESS_LCL_READ_RSP:
		case CPSV_DATA_ACCESS_RMT_READ_RSP:
			{
				int i = 0;
				if (evt->info.cpnd.info.ckpt_nd2nd_data_rsp.num_of_elmts != -1) {
					for (; i < evt->info.cpnd.info.ckpt_nd2nd_data_rsp.num_of_elmts; i++) {
						if ((evt->info.cpnd.info.ckpt_nd2nd_data_rsp.info.read_data[i].data !=
						     NULL)) {
							m_MMGR_FREE_CPND_DEFAULT(evt->info.cpnd.
										 info.ckpt_nd2nd_data_rsp.info.
										 read_data[i].data);
							evt->info.cpnd.info.ckpt_nd2nd_data_rsp.info.read_data[i].data =
							    NULL;
						}
					}
				}
				if (evt->info.cpnd.info.ckpt_nd2nd_data_rsp.info.read_data)
					m_MMGR_FREE_CPND_DEFAULT(evt->info.cpnd.info.ckpt_nd2nd_data_rsp.
								 info.read_data);
				evt->info.cpnd.info.ckpt_nd2nd_data_rsp.info.read_data = NULL;
			}
			break;
		}
	}

	m_MMGR_FREE_CPSV_EVT(evt, NCS_SERVICE_ID_CPND);
	return rc;
}

/****************************************************************************************
 * Name          : cpnd_is_cpd_up
 *
 * Description   : This routine tests whether CPD is up or down
 * Arguments     : cb       - CPND Control Block pointer
 *
 * Return Values : TRUE/FALSE
 *****************************************************************************************/
static uns32 cpnd_is_cpd_up(CPND_CB *cb)
{
	uns32 is_cpd_up;

	m_NCS_LOCK(&cb->cpnd_cpd_up_lock, NCS_LOCK_WRITE);

	is_cpd_up = cb->is_cpd_up;

	m_NCS_UNLOCK(&cb->cpnd_cpd_up_lock, NCS_LOCK_WRITE);

	return is_cpd_up;
}

/****************************************************************************************
 * Name          : cpnd_transfer_replica
 *
 * Description   : This routine transfers replica contents to another CPND
 * Arguments     : cb - CPND Control Block pointer
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *****************************************************************************************/
static uns32 cpnd_transfer_replica(CPND_CB *cb, CPND_CKPT_NODE *cp_node, SaCkptCheckpointHandleT ckpt_id,
				   CPSV_CPND_DEST_INFO *dest_list, CPSV_A2ND_CKPT_SYNC sync)
{
	CPSV_EVT send_evt;
	CPSV_CKPT_DATA *tmp_sec_data = NULL, *sec_data = NULL;
	CPND_CKPT_SECTION_INFO *tmp_sec_info = NULL;
	CPSV_CPND_DEST_INFO *tmp = NULL;
	uns32 rc = NCSCC_RC_SUCCESS, seqno = 1, num = 0, total_num = 0;
	SaSizeT size = 0;

	memset(&send_evt, '\0', sizeof(CPSV_EVT));

	send_evt.type = CPSV_EVT_TYPE_CPND;
	send_evt.info.cpnd.type = CPND_EVT_ND2ND_CKPT_ACTIVE_SYNC;
	send_evt.info.cpnd.info.ckpt_nd2nd_sync.type = CPSV_CKPT_ACCESS_SYNC;
	send_evt.info.cpnd.info.ckpt_nd2nd_sync.ckpt_id = ckpt_id;
	send_evt.info.cpnd.info.ckpt_nd2nd_sync.ckpt_sync = sync;

	tmp_sec_info = cp_node->replica_info.section_info;
	if (tmp_sec_info == NULL) {
		rc = NCSCC_RC_FAILURE;
		m_LOG_CPND_CL(CPND_CKPT_ALLOC_FAILED, CPND_FC_MEMFAIL, NCSFL_SEV_ERROR, __FILE__, __LINE__);
		send_evt.info.cpnd.info.ckpt_nd2nd_sync.data = sec_data;
		cpnd_proc_free_cpsv_ckpt_data(send_evt.info.cpnd.info.ckpt_nd2nd_sync.data);
		return rc;
	}

	while (1) {

		if (((tmp_sec_info) && ((size + tmp_sec_info->sec_size) > MAX_SYNC_TRANSFER_SIZE))
		    || (total_num == cp_node->replica_info.n_secs)) {

			send_evt.info.cpnd.info.ckpt_nd2nd_sync.num_of_elmts = num;
			send_evt.info.cpnd.info.ckpt_nd2nd_sync.data = sec_data;
			send_evt.info.cpnd.info.ckpt_nd2nd_sync.seqno = seqno;

			if (total_num == cp_node->replica_info.n_secs)
				send_evt.info.cpnd.info.ckpt_nd2nd_sync.last_seq = TRUE;
			else
				send_evt.info.cpnd.info.ckpt_nd2nd_sync.last_seq = FALSE;

			for (tmp = dest_list; tmp; tmp = tmp->next) {

				if (m_CPND_IS_LOCAL_NODE(&tmp->dest, &cb->cpnd_mdest_id) == 0)
					continue;

				rc = cpnd_mds_msg_send(cb, NCSMDS_SVC_ID_CPND, tmp->dest, &send_evt);

				if (rc == NCSCC_RC_FAILURE) {
					m_LOG_CPND_CL(CPND_MDS_SEND_FAIL, CPND_FC_HDLN, NCSFL_SEV_ERROR,
						      __FILE__, __LINE__);
					cpnd_proc_free_cpsv_ckpt_data(send_evt.info.cpnd.info.ckpt_nd2nd_sync.data);
					return rc;
				}
			}
			/* free ckpt_nd2nd_sync stuff */
			cpnd_proc_free_cpsv_ckpt_data(send_evt.info.cpnd.info.ckpt_nd2nd_sync.data);

			if (total_num == cp_node->replica_info.n_secs) {
				return rc;
			}

			send_evt.info.cpnd.info.ckpt_nd2nd_sync.data = NULL;
			num = 0;
			size = 0;
			sec_data = NULL;
			seqno++;
		}

		tmp_sec_data = m_MMGR_ALLOC_CPSV_CKPT_DATA;
		memset(tmp_sec_data, '\0', sizeof(CPSV_CKPT_DATA));

		tmp_sec_data->sec_id = tmp_sec_info->sec_id;
		tmp_sec_data->expirationTime = tmp_sec_info->exp_tmr;
		tmp_sec_data->dataSize = tmp_sec_info->sec_size;

		tmp_sec_data->data = m_MMGR_ALLOC_CPND_DEFAULT(tmp_sec_data->dataSize);

		cpnd_ckpt_sec_read(cp_node, tmp_sec_info, tmp_sec_data->data, tmp_sec_data->dataSize, 0);

		tmp_sec_data->next = sec_data;
		sec_data = tmp_sec_data;

		size += tmp_sec_info->sec_size;
		num++;
		total_num++;

		tmp_sec_info = tmp_sec_info->next;
	}

	return NCSCC_RC_FAILURE;
}
