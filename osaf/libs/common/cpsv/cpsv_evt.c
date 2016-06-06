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
  
  This file consists of CPSV routines used for event handling.

    
******************************************************************************
*/

#include "cpsv.h"
#include "cpa_tmr.h"

FUNC_DECLARATION(CPSV_CKPT_DATA);
static SaCkptSectionIdT *cpsv_evt_dec_sec_id(NCS_UBAID *i_ub, uint32_t svc_id);
static uint32_t cpsv_evt_enc_sec_id(NCS_UBAID *o_ub, SaCkptSectionIdT *sec_id);
static void cpsv_convert_sec_id_to_string(char *sec_id_str, SaCkptSectionIdT *section_id);

const char *cpa_evt_str[] = {
	"STRING_0",

	/* Locally generated events */
	"CPA_EVT_MDS_INFO",	/* CPND UP/DOWN Info */
	"CPA_EVT_TIME_OUT",	/* Time out events at CPA */

	/* Events from CPND */

	"CPA_EVT_ND2A_CKPT_INIT_RSP",
	"CPA_EVT_ND2A_CKPT_FINALIZE_RSP",
	"CPA_EVT_ND2A_CKPT_OPEN_RSP",
	"CPA_EVT_ND2A_CKPT_CLOSE_RSP",
	"CPA_EVT_ND2A_CKPT_UNLINK_RSP",
	"CPA_EVT_ND2A_CKPT_RDSET_RSP",
	"CPA_EVT_ND2A_CKPT_AREP_SET_RSP",
	"CPA_EVT_ND2A_CKPT_STATUS",

	"CPA_EVT_ND2A_SEC_CREATE_RSP",
	"CPA_EVT_ND2A_SEC_DELETE_RSP",
	"CPA_EVT_ND2A_SEC_EXPTIME_RSP",
	"CPA_EVT_ND2A_SEC_ITER_RSP",
	"CPA_EVT_ND2A_SEC_ITER_GETNEXT_RSP",

	"CPA_EVT_ND2A_CKPT_ARRIVAL_NTFY",

	"CPA_EVT_ND2A_CKPT_DATA_RSP",

	"CPA_EVT_ND2A_CKPT_SYNC_RSP",
	"CPA_EVT_D2A_ACT_CKPT_INFO_BCAST_SEND",
	"CPA_EVT_ND2A_CKPT_READ_ACK_RSP",
	"CPA_EVT_ND2A_CKPT_BCAST_SEND",
	"CPA_EVT_D2A_NDRESTART",
	"CPA_EVT_CB_DUMP",
	"CPA_EVT_ND2A_CKPT_CLM_NODE_LEFT",
	"CPA_EVT_ND2A_CKPT_CLM_NODE_JOINED",
	"CPA_EVT_ND2A_ACT_CKPT_INFO_BCAST_SEND",
};

const char* cpnd_evt_str[] = {
	/* events from CPA to CPND */
	"CPND_EVT_BASE = 1",

	/* Locally generated events */
	"CPND_EVT_MDS_INFO",	/* CPA/CPND/CPD UP/DOWN Info */
	"CPND_EVT_TIME_OUT",	/* Time out event */

	/* Events from CPA */

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
	"CPND_EVT_A2ND_CKPT_SECT_ITER_REQ",	/*Checkpoint Section iteration initialize */

	"CPND_EVT_A2ND_CKPT_ITER_GETNEXT",	/* Checkpoint Section Iternation Getnext Call */

	"CPND_EVT_A2ND_ARRIVAL_CB_REG",	/* Checkpoint Arrival Callback Register*/

	"CPND_EVT_A2ND_CKPT_WRITE",	/* Checkpoint Write And overwrite call */
	"CPND_EVT_A2ND_CKPT_READ",	/* Checkpoint Read Call  */
	"CPND_EVT_A2ND_CKPT_SYNC",	/* Checkpoint Synchronize call */

	"CPND_EVT_A2ND_CKPT_READ_ACK",	/* read ack */

	/* Events from other CPND */

/* ckpt status information from active */

	"CPND_EVT_ND2ND_ACTIVE_STATUS",	/* ckpt status info from active */
	"CPND_EVT_ND2ND_ACTIVE_STATUS_ACK",	/* ckpt status ack from active */

	"CPND_EVT_ND2ND_CKPT_SYNC_REQ",	/* rqst from ND to ND(A) to sync ckpt */
	"CPND_EVT_ND2ND_CKPT_ACTIVE_SYNC",	/* CPND(A) sync updts to All the Ckpts */
/* Section Create Stuff.... */

	"CPSV_EVT_ND2ND_CKPT_SECT_CREATE_REQ",
	"CPSV_EVT_ND2ND_CKPT_SECT_CREATE_RSP",
	"CPSV_EVT_ND2ND_CKPT_SECT_ACTIVE_CREATE_RSP",

	"CPSV_EVT_ND2ND_CKPT_SECT_DELETE_REQ",
	"CPSV_EVT_ND2ND_CKPT_SECT_DELETE_RSP",

	"CPSV_EVT_ND2ND_CKPT_SECT_EXPTMR_REQ",
	"CPSV_EVT_ND2ND_CKPT_SECT_EXPTMR_RSP",

	"CPSV_EVT_ND2ND_CKPT_SECT_DATA_ACCESS_REQ",	/* for write,read,overwrite */
	"CPSV_EVT_ND2ND_CKPT_SECT_ACTIVE_DATA_ACCESS_REQ",
	"CPSV_EVT_ND2ND_CKPT_SECT_ACTIVE_DATA_ACCESS_RSP",

	/* Events from CPD to CPND */

	"CPND_EVT_D2ND_CKPT_INFO",	/* Rsp to the ckpt open call */
	"CPND_EVT_D2ND_CKPT_SIZE",
	"CPND_EVT_D2ND_CKPT_REP_ADD",	/* ckpt open is propogated to other NDs */
	"CPND_EVT_D2ND_CKPT_REP_DEL",	/* ckpt close is propogated to other NDs */

	"CPSV_D2ND_RESTART",	/* for cpnd redundancy */
	"CPSV_D2ND_RESTART_DONE",	/* for cpnd redundancy */

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
   "CPND_EVT_A2ND_CKPT_REFCNTSET",        /* ref cont opener's set call */
   "CPND_EVT_A2ND_CKPT_LIST_UPDATE",	/* Checkpoint ckpt list update Call */
   "CPND_EVT_A2ND_ARRIVAL_CB_UNREG",	/* Checkpoint Arrival Callback Un-Register*/
};

const char* cpd_evt_str[] = {
	"CPD_EVT_BASE = 1",

	/* Locally generated Events */
	"CPD_EVT_MDS_INFO",

	/* Events from CPND */
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
	"CPD_EVT_TIME_OUT",
};

/****************************************************************************\
 PROCEDURE NAME : cpsv_evt_str

 DESCRIPTION    : This routine will return the event string from the event

 ARGUMENTS      : evt : event .

 RETURNS        : None

 NOTES          : 
\*****************************************************************************/
char* cpsv_evt_str(CPSV_EVT *evt, char *o_evt_str, size_t len)
{
	if (evt == NULL)
		return "Invalid evt";

	if (o_evt_str == NULL)
		return "Invalid o_evt_str";

	switch (evt->type) {
	case CPSV_EVT_TYPE_CPND:
		switch (evt->info.cpnd.type) {
		case CPND_EVT_MDS_INFO:
		{
			CPSV_MDS_INFO *info = &evt->info.cpnd.info.mds_info;
			snprintf(o_evt_str, len, "CPND_EVT_MDS_INFO(dest=0x%llX, svc=%s(%u) - %s(%u))", 
				(unsigned long long)info->dest,
				info->svc_id == NCSMDS_SVC_ID_CPA ? "CPA" :
				info->svc_id == NCSMDS_SVC_ID_CPND ? "CPND" :
				info->svc_id == NCSMDS_SVC_ID_CPD ? "CPD" : "OTHER",
			       	info->svc_id,	
			       	info->change == NCSMDS_NO_ACTIVE ? "NO_ACT" : 
				info->change == NCSMDS_NEW_ACTIVE ? "NEW_ACT" :
				info->change == NCSMDS_UP ? "UP" :
				info->change == NCSMDS_DOWN ? "DOWN" :
			        info->change == NCSMDS_RED_UP ? "RED_UP" :
				info->change == NCSMDS_RED_DOWN ? "RED_DOWN" :
				info->change == NCSMDS_CHG_ROLE ? "CHG_ROLE" : "OTHER",
				info->change);	
			break;
		}
		case CPND_EVT_TIME_OUT:
		{
			CPND_TMR_INFO *info = &evt->info.cpnd.info.tmr_info;
			snprintf(o_evt_str, len, "[%llu] CPND_EVT_TIME_OUT(type=%s(%u))", info->ckpt_id,
				info->type == CPND_TMR_TYPE_RETENTION ? "RETENTION" :
				info->type == CPND_TMR_TYPE_SEC_EXPI ? "SEC_EXPI" :
				info->type == CPND_ALL_REPL_RSP_EXPI ? "REPL_RSP_EXPI" :
				info->type == CPND_TMR_OPEN_ACTIVE_SYNC ? "OPEN_ACTIVE_SYNC" :
				info->type == CPND_TMR_TYPE_NON_COLLOC_RETENTION ? "NON_COL_RETENTION" : "INVALID",
				info->type);
			break;
		}
		case CPND_EVT_A2ND_CKPT_INIT:
			snprintf(o_evt_str, len, "CPND_EVT_A2ND_CKPT_INIT");
			break;
		case CPND_EVT_A2ND_CKPT_FINALIZE:
		{
			snprintf(o_evt_str, len, "CPND_EVT_A2ND_CKPT_FINALIZE(hdl=%llu)", evt->info.cpnd.info.finReq.client_hdl);
			break;
		}
		case CPND_EVT_A2ND_CKPT_OPEN:
		{
			CPSV_A2ND_OPEN_REQ *info = &evt->info.cpnd.info.openReq;
			snprintf(o_evt_str, len, "CPND_EVT_A2ND_CKPT_OPEN(hdl=%llu, %s)",
				info->client_hdl, info->ckpt_name.value);
			break;
		}
		case CPND_EVT_A2ND_CKPT_CLOSE:
		{
			CPSV_A2ND_CKPT_CLOSE *info = &evt->info.cpnd.info.closeReq;
			snprintf(o_evt_str, len, "[%llu] CPND_EVT_A2ND_CKPT_CLOSE(hdl=%llu, flags=0x%X)",
				info->ckpt_id, info->client_hdl, info->ckpt_flags);
			break;
		}
		case CPND_EVT_A2ND_CKPT_UNLINK:
		{
			CPSV_A2ND_CKPT_UNLINK *info = &evt->info.cpnd.info.ulinkReq;
			snprintf(o_evt_str, len, "CPND_EVT_A2ND_CKPT_UNLINK(%s)", info->ckpt_name.value);
			break;
		}
		case CPND_EVT_A2ND_CKPT_RDSET:
		{
			CPSV_A2ND_RDSET *info = &evt->info.cpnd.info.rdsetReq;
			snprintf(o_evt_str, len, "[%llu] CPND_EVT_A2ND_CKPT_RDSET(reten_time=%llu)", info->ckpt_id, info->reten_time);
			break;
		}
		case CPND_EVT_A2ND_CKPT_AREP_SET:
			snprintf(o_evt_str, len, "[%llu] CPND_EVT_A2ND_CKPT_AREP_SET", evt->info.cpnd.info.arsetReq.ckpt_id);
			break;
		case CPND_EVT_A2ND_CKPT_STATUS_GET:
			snprintf(o_evt_str, len, "[%llu] CPND_EVT_A2ND_CKPT_STATUS_GET", evt->info.cpnd.info.statReq.ckpt_id);
			break;
		case CPND_EVT_A2ND_CKPT_SECT_CREATE:
		{
			CPSV_CKPT_SECT_CREATE *info = &evt->info.cpnd.info.sec_creatReq;
			char sec_id[MAX_SEC_ID_LEN] = {0};
			cpsv_convert_sec_id_to_string(sec_id, info->sec_attri.sectionId);

			snprintf(o_evt_str, len, "[%llu] CPND_EVT_A2ND_CKPT_SECT_CREATE(sec_id=%s, mdest=0x%llX)", 
				info->ckpt_id, sec_id, (unsigned long long)info->agent_mdest);
			break;
		}
		case CPND_EVT_A2ND_CKPT_SECT_DELETE:
		{
			CPSV_A2ND_SECT_DELETE *info = &evt->info.cpnd.info.sec_delReq;
			char sec_id[MAX_SEC_ID_LEN] = {0};
			cpsv_convert_sec_id_to_string(sec_id, &info->sec_id);

			snprintf(o_evt_str, len, "[%llu] CPND_EVT_A2ND_CKPT_SECT_DELETE(sec_id=%s)", 
				info->ckpt_id, sec_id);
			break;
		}
		case CPND_EVT_A2ND_CKPT_SECT_EXP_SET:
		{
			CPSV_A2ND_SECT_EXP_TIME *info = &evt->info.cpnd.info.sec_expset;
			char sec_id[MAX_SEC_ID_LEN] = {0};
			cpsv_convert_sec_id_to_string(sec_id, &info->sec_id);

			snprintf(o_evt_str, len, "[%llu] CPND_EVT_A2ND_CKPT_SECT_EXP_SET(sec_id=%s, exp_time=%llu)", 
				info->ckpt_id, sec_id, info->exp_time);
			break;
		}
		case CPND_EVT_A2ND_CKPT_SECT_ITER_REQ:
		{
			CPSV_CKPT_STATUS_GET *info = &evt->info.cpnd.info.sec_iter_req;
			snprintf(o_evt_str, len, "[%llu] CPND_EVT_A2ND_CKPT_SECT_ITER_REQ", info->ckpt_id );
			break;
		}
		case CPND_EVT_A2ND_CKPT_ITER_GETNEXT:
		{
			CPSV_A2ND_SECT_ITER_GETNEXT *info = &evt->info.cpnd.info.iter_getnext;
			char sec_id[MAX_SEC_ID_LEN] = {0};
			cpsv_convert_sec_id_to_string(sec_id, &info->section_id);

			snprintf(o_evt_str, len, "[%llu] CPND_EVT_A2ND_CKPT_ITER_GETNEXT(sec_id=%s, filter=%s, n_secs_trav=%u, exp_tmr=%llu)", 
				info->ckpt_id, sec_id,
				info->filter == SA_CKPT_SECTIONS_FOREVER ? "FOREVER" :
				info->filter == SA_CKPT_SECTIONS_LEQ_EXPIRATION_TIME ? "LEQ_EX_TIME" :
				info->filter == SA_CKPT_SECTIONS_GEQ_EXPIRATION_TIME ? "GEQ_EX_TIME" :
				info->filter == SA_CKPT_SECTIONS_CORRUPTED ? "CORRUPTED" :
				info->filter == SA_CKPT_SECTIONS_ANY ? "ANY" : "INVALID",
				info->n_secs_trav, info->exp_tmr);
			break;
		}
		case CPND_EVT_A2ND_ARRIVAL_CB_REG:
			snprintf(o_evt_str, len, "CPND_EVT_A2ND_ARRIVAL_CB_REG(hdl=%llu)", evt->info.cpnd.info.arr_ntfy.client_hdl);
			break;
		case CPND_EVT_A2ND_CKPT_WRITE:
		{
			CPSV_CKPT_ACCESS *info = &evt->info.cpnd.info.ckpt_write;
			snprintf(o_evt_str, len, "[%llu] CPND_EVT_A2ND_CKPT_WRITE(type=%s(%d))", info->ckpt_id, 
				info->type == CPSV_CKPT_ACCESS_WRITE ? "WRITE" :
				info->type == CPSV_CKPT_ACCESS_OVWRITE ? "OVWRITE" : "INVALID",
				info->type); 
			break;
		}
		case CPND_EVT_A2ND_CKPT_READ:
		{
			CPSV_CKPT_ACCESS *info = &evt->info.cpnd.info.ckpt_read;
			snprintf(o_evt_str, len, "[%llu] CPND_EVT_A2ND_CKPT_READ()", info->ckpt_id);
			break;
		}
		case CPND_EVT_A2ND_CKPT_SYNC:
		{
			CPSV_A2ND_CKPT_SYNC *info = &evt->info.cpnd.info.ckpt_sync;
			snprintf(o_evt_str, len, "[%llu] CPND_EVT_A2ND_CKPT_SYNC(hdl=%llu, open=%s)",
				info->ckpt_id, info->client_hdl, info->is_ckpt_open ? "true" : "false");
			break;
		}
		case CPND_EVT_A2ND_CKPT_READ_ACK:
			/* This message is not used */
			snprintf(o_evt_str, len, "CPND_EVT_A2ND_CKPT_READ_ACK");
			break;
		case CPND_EVT_ND2ND_ACTIVE_STATUS:
			snprintf(o_evt_str, len, "[%llu] CPND_EVT_ND2ND_ACTIVE_STATUS", evt->info.cpnd.info.stat_get.ckpt_id);
			break;
		case CPND_EVT_ND2ND_ACTIVE_STATUS_ACK:
		{
			CPSV_CKPT_STATUS *info = &evt->info.cpnd.info.status;
			snprintf(o_evt_str, len, "[%llu] CPND_EVT_ND2ND_ACTIVE_STATUS_ACK(err=%u, mem_used=%llu, "
				"n_secs=%u, ckpt_size=%llu, reten=%llu, max_sec=%u)", info->ckpt_id, info->error, 
				info->status.memoryUsed, info->status.numberOfSections, 
				info->status.checkpointCreationAttributes.checkpointSize,
				info->status.checkpointCreationAttributes.retentionDuration,
				info->status.checkpointCreationAttributes.maxSections);
			break;
		}
		case CPND_EVT_ND2ND_CKPT_SYNC_REQ:
		{
			CPSV_A2ND_CKPT_SYNC *info = &evt->info.cpnd.info.sync_req;
			snprintf(o_evt_str, len, "[%llu] CPND_EVT_ND2ND_CKPT_SYNC_REQ(hdl=%llu, open=%s)",
				info->ckpt_id, info->client_hdl, info->is_ckpt_open ? "true" : "false");
			break;
		}
		case CPND_EVT_ND2ND_CKPT_ACTIVE_SYNC:
		{
			CPSV_CKPT_ACCESS *info = &evt->info.cpnd.info.ckpt_nd2nd_sync;
			snprintf(o_evt_str, len, "[%llu] CPND_EVT_ND2ND_CKPT_ACTIVE_SYNC(seq_no=%u, last_seq=%u, is_open=%s)", 
				info->ckpt_id, info->seqno, info->last_seq, info->ckpt_sync.is_ckpt_open == true ? "true" : "false");
			break;
		}
		case CPSV_EVT_ND2ND_CKPT_SECT_CREATE_REQ:
		{
			CPSV_CKPT_SECT_CREATE *info = &evt->info.cpnd.info.active_sec_creat;
			char sec_id[MAX_SEC_ID_LEN] = {0};
			cpsv_convert_sec_id_to_string(sec_id, info->sec_attri.sectionId);

			snprintf(o_evt_str, len, "[%llu] CPSV_EVT_ND2ND_CKPT_SECT_CREATE_REQ(sec_id=%s)", 
				info->ckpt_id, sec_id);
			break;
		}
		case CPSV_EVT_ND2ND_CKPT_SECT_CREATE_RSP:
		{
			CPSV_CKPT_SECT_INFO *info = &evt->info.cpnd.info.active_sec_creat_rsp;
			char sec_id[MAX_SEC_ID_LEN] = {0};
			cpsv_convert_sec_id_to_string(sec_id, &info->sec_id);

			snprintf(o_evt_str, len, "[%llu] CPSV_EVT_ND2ND_CKPT_SECT_CREATE_RSP(err=%u, sec_id=%s)",
				info->ckpt_id, info->error, sec_id);
			break;
		}
		case CPSV_EVT_ND2ND_CKPT_SECT_ACTIVE_CREATE_RSP:
		{
			CPSV_CKPT_SECT_INFO *info = &evt->info.cpnd.info.active_sec_creat_rsp;
			snprintf(o_evt_str, len, "[%llu] CPSV_EVT_ND2ND_CKPT_SECT_ACTIVE_CREATE_RSP(err=%u)",
				info->ckpt_id, info->error);
			break;
		}
		case CPSV_EVT_ND2ND_CKPT_SECT_DELETE_REQ:
		{
			CPSV_CKPT_SECT_INFO *info = &evt->info.cpnd.info.sec_delete_req;
			char sec_id[MAX_SEC_ID_LEN] = {0};
			cpsv_convert_sec_id_to_string(sec_id, &info->sec_id);

			snprintf(o_evt_str, len, "[%llu] CPSV_EVT_ND2ND_CKPT_SECT_DELETE_REQ(sec_id=%s)", 
				info->ckpt_id, sec_id);
			break;
		}
		case CPSV_EVT_ND2ND_CKPT_SECT_DELETE_RSP:
			snprintf(o_evt_str, len, "CPSV_EVT_ND2ND_CKPT_SECT_DELETE_RSP(err=%u)", evt->info.cpnd.info.sec_delete_rsp.error);
			break;
		case CPSV_EVT_ND2ND_CKPT_SECT_EXPTMR_REQ:
		{
			CPSV_A2ND_SECT_EXP_TIME *info = &evt->info.cpnd.info.sec_exp_set;
			char sec_id[MAX_SEC_ID_LEN] = {0};
			cpsv_convert_sec_id_to_string(sec_id, &info->sec_id);

			snprintf(o_evt_str, len, "[%llu] CPSV_EVT_ND2ND_CKPT_SECT_EXPTMR_REQ(sec_id=%s, exp_tmr=%llu)", 
				info->ckpt_id, sec_id, info->exp_time);
			break;
		}
		case CPSV_EVT_ND2ND_CKPT_SECT_EXPTMR_RSP:
			snprintf(o_evt_str, len, "CPSV_EVT_ND2ND_CKPT_SECT_EXPTMR_RSP(err=%u)", 
				evt->info.cpnd.info.sec_exp_rsp.error);
			break;
		case CPSV_EVT_ND2ND_CKPT_SECT_DATA_ACCESS_REQ:
			/* This message is not used */
			snprintf(o_evt_str, len, "CPSV_EVT_ND2ND_CKPT_SECT_DATA_ACCESS_REQ");
			break;
		case CPSV_EVT_ND2ND_CKPT_SECT_ACTIVE_DATA_ACCESS_REQ:
		{
			CPSV_CKPT_ACCESS *info = &evt->info.cpnd.info.ckpt_nd2nd_data;
			snprintf(o_evt_str, len, "[%llu] CPSV_EVT_ND2ND_CKPT_SECT_ACTIVE_DATA_ACCESS_REQ(type=%s(%d), seqno=%u, last_seq=%u)",
				info->ckpt_id, 
				info->type == CPSV_CKPT_ACCESS_WRITE ? "WRITE" :
				info->type == CPSV_CKPT_ACCESS_OVWRITE ? "OVWRITE" : "INVALID",
				info->type, info->seqno, info->last_seq);
			break;
		}
		case CPSV_EVT_ND2ND_CKPT_SECT_ACTIVE_DATA_ACCESS_RSP:
		{
			CPSV_ND2A_DATA_ACCESS_RSP *info = &evt->info.cpnd.info.ckpt_nd2nd_data_rsp;
			snprintf(o_evt_str, len, "[%llu] CPSV_EVT_ND2ND_CKPT_SECT_ACTIVE_DATA_ACCESS_RSP(err=%u, type=%s(%u))",
				info->ckpt_id, 
				info->type == CPSV_DATA_ACCESS_WRITE_RSP ? info->error : info->info.ovwrite_error.error,
				info->type == CPSV_DATA_ACCESS_WRITE_RSP? "WRITE_RSP" : "OVWRITE_RSP", info->type);
			break;
		}
		case CPND_EVT_D2ND_CKPT_INFO:
		{
			CPSV_D2ND_CKPT_INFO *info = &evt->info.cpnd.info.ckpt_info;
			snprintf(o_evt_str, len, "[%llu] CPND_EVT_D2ND_CKPT_INFO(err=%u, active=0x%X, create_rep=%s)",
				info->ckpt_id, info->error, m_NCS_NODE_ID_FROM_MDS_DEST(info->active_dest), 
				info->ckpt_rep_create == true ? "true" : "false");
			break;
		}
		case CPND_EVT_D2ND_CKPT_SIZE:
		{
			CPSV_CKPT_USED_SIZE *info = &evt->info.cpnd.info.ckpt_mem_size;
			snprintf(o_evt_str, len, "[%llu] CPND_EVT_D2ND_CKPT_SIZE(err=%u, used_mem=%u)", 
				info->ckpt_id, info->error, info->ckpt_used_size);
			break;
		}
		case CPND_EVT_D2ND_CKPT_REP_ADD:
		{
			CPSV_CKPT_DESTLIST_INFO *info = &evt->info.cpnd.info.ckpt_add;
			snprintf(o_evt_str, len, "[%llu] CPND_EVT_D2ND_CKPT_REP_ADD(dest=0x%X, active=0x%X, restart=%s, dest_cnt=%u)",
				info->ckpt_id, m_NCS_NODE_ID_FROM_MDS_DEST(info->mds_dest),
				m_NCS_NODE_ID_FROM_MDS_DEST(info->active_dest), 
				info->is_cpnd_restart == true ? "true" : "false", info->dest_cnt);
			break;
		}
		case CPND_EVT_D2ND_CKPT_REP_DEL:
		{
			CPSV_CKPT_DEST_INFO *info = &evt->info.cpnd.info.ckpt_del;
			snprintf(o_evt_str, len, "[%llu] CPND_EVT_D2ND_CKPT_REP_DEL(node_id=0x%X)",
				info->ckpt_id, m_NCS_NODE_ID_FROM_MDS_DEST(info->mds_dest));
			break;
		}
		case CPSV_D2ND_RESTART:
			snprintf(o_evt_str, len, "[%llu] CPSV_D2ND_RESTART", evt->info.cpnd.info.cpnd_restart.ckpt_id);
			break;
		case CPSV_D2ND_RESTART_DONE:
		{
			CPSV_CKPT_DESTLIST_INFO *info = &evt->info.cpnd.info.cpnd_restart_done;
			snprintf(o_evt_str, len, "[%llu] CPSV_D2ND_RESTART_DONE(active=0x%X, dest_cnt=%u)", info->ckpt_id,
				m_NCS_NODE_ID_FROM_MDS_DEST(info->active_dest), info->dest_cnt);
			break;
		}
		case CPND_EVT_D2ND_CKPT_CREATE:
		{
			CPSV_D2ND_CKPT_CREATE *info = &evt->info.cpnd.info.ckpt_create;
			snprintf(o_evt_str, len, "[%llu] CPND_EVT_D2ND_CKPT_CREATE(%s, create_rep=%s, active=0x%X)",
				info->ckpt_info.ckpt_id, info->ckpt_name.value,
			       	info->ckpt_info.ckpt_rep_create ? "true" : "false",
				m_NCS_NODE_ID_FROM_MDS_DEST(info->ckpt_info.active_dest));
			break;
		}
		case CPND_EVT_D2ND_CKPT_DESTROY:
		{
			snprintf(o_evt_str, len, "[%llu] CPND_EVT_D2ND_CKPT_DESTROY", evt->info.cpnd.info.ckpt_destroy.ckpt_id);
			break;
		}
		case CPND_EVT_D2ND_CKPT_DESTROY_ACK:
		{
			snprintf(o_evt_str, len, "CPND_EVT_D2ND_CKPT_DESTROY_ACK(err=%u)", evt->info.cpnd.info.destroy_ack.error);
			break;
		}
		case CPND_EVT_D2ND_CKPT_CLOSE_ACK:
			/* This message is not used */
			snprintf(o_evt_str, len, "CPND_EVT_D2ND_CKPT_CLOSE_ACK");
			break;
		case CPND_EVT_D2ND_CKPT_UNLINK:
		{
			snprintf(o_evt_str, len, "[%llu] CPND_EVT_D2ND_CKPT_UNLINK", evt->info.cpnd.info.ckpt_ulink.ckpt_id);
			break;
		}
		case CPND_EVT_D2ND_CKPT_UNLINK_ACK:
		{
			CPSV_SAERR_INFO *info = &evt->info.cpnd.info.ulink_ack;
			snprintf(o_evt_str, len, "CPND_EVT_D2ND_CKPT_UNLINK_ACK(err=%u)", info->error);
			break;
		}
		case CPND_EVT_D2ND_CKPT_RDSET:
		{
			CPSV_CKPT_RDSET *info = &evt->info.cpnd.info.rdset;
			snprintf(o_evt_str, len, "[%llu] CPND_EVT_D2ND_CKPT_RDSET(%s(%u), time=%llu)",
				info->ckpt_id,
			       	info->type == CPSV_CKPT_RDSET_INFO ? "INFO" :
				info->type == CPSV_CKPT_RDSET_START ? "START" :
				info->type == CPSV_CKPT_RDSET_STOP ? "STOP" : "OTHER",
				info->type, info->reten_time);
			break;
		}
		case CPND_EVT_D2ND_CKPT_RDSET_ACK:
			snprintf(o_evt_str, len, "CPND_EVT_D2ND_CKPT_RDSET_ACK(err=%u)", evt->info.cpnd.info.rdset_ack.error);
			break;
		case CPND_EVT_D2ND_CKPT_ACTIVE_SET:
		{
			CPSV_CKPT_DEST_INFO *info = &evt->info.cpnd.info.active_set;
			snprintf(o_evt_str, len, "[%llu] CPND_EVT_D2ND_CKPT_ACTIVE_SET(active_node = 0x%X)",
				info->ckpt_id, m_NCS_NODE_ID_FROM_MDS_DEST(info->mds_dest));
			break;
		}
		case CPND_EVT_D2ND_CKPT_ACTIVE_SET_ACK:
			/* This message is not used */
			snprintf(o_evt_str, len, "CPND_EVT_D2ND_CKPT_ACTIVE_SET_ACK(err=%u)",
				evt->info.cpnd.info.arep_ack.error);
			break;
		case CPND_EVT_ND2ND_CKPT_ITER_NEXT_REQ:
		{
			CPSV_A2ND_SECT_ITER_GETNEXT *info = &evt->info.cpnd.info.iter_getnext;
			char sec_id[MAX_SEC_ID_LEN] = {0};
			cpsv_convert_sec_id_to_string(sec_id, &info->section_id);

			snprintf(o_evt_str, len, "[%llu] CPND_EVT_ND2ND_CKPT_ITER_NEXT_REQ(sec_id=%s, filter=%s, n_secs_trav=%u, exp_tmr=%llu)", 
				info->ckpt_id, sec_id,
				info->filter == SA_CKPT_SECTIONS_FOREVER ? "FOREVER" :
				info->filter == SA_CKPT_SECTIONS_LEQ_EXPIRATION_TIME ? "LEQ_EX_TIME" :
				info->filter == SA_CKPT_SECTIONS_GEQ_EXPIRATION_TIME ? "GEQ_EX_TIME" :
				info->filter == SA_CKPT_SECTIONS_CORRUPTED ? "CORRUPTED" :
				info->filter == SA_CKPT_SECTIONS_ANY ? "ANY" : "INVALID",
				info->n_secs_trav, info->exp_tmr);

			break;
		}
		case CPND_EVT_ND2ND_CKPT_ACTIVE_ITERNEXT:
			snprintf(o_evt_str, len, "CPND_EVT_ND2ND_CKPT_ACTIVE_ITERNEXT");
			break;
		case CPND_EVT_CB_DUMP:
			snprintf(o_evt_str, len, "CPND_EVT_CB_DUMP");
			break;
		case CPND_EVT_D2ND_CKPT_NUM_SECTIONS:
		{
			CPSV_CKPT_NUM_SECTIONS *info = &evt->info.cpnd.info.ckpt_sections;
			snprintf(o_evt_str, len, "[%llu] CPND_EVT_D2ND_CKPT_NUM_SECTIONS", info->ckpt_id);
			break;
		}
		case CPND_EVT_A2ND_CKPT_REFCNTSET:
		{
			CPSV_A2ND_REFCNTSET *info = &evt->info.cpnd.info.refCntsetReq;
			snprintf(o_evt_str, len, " CPND_EVT_A2ND_CKPT_REFCNTSET(no_of_nodes=%u)", info->no_of_nodes);
			break;
		}
		case CPND_EVT_A2ND_CKPT_LIST_UPDATE:
		{
			CPSV_A2ND_CKPT_LIST_UPDATE *info = &evt->info.cpnd.info.ckptListUpdate;
			snprintf(o_evt_str, len, "CPND_EVT_A2ND_CKPT_LIST_UPDATE(hdl=%llu, %s)", 
				info->client_hdl, info->ckpt_name.value);
			break;
		}
		case CPND_EVT_A2ND_ARRIVAL_CB_UNREG:
			snprintf(o_evt_str, len, "CPND_EVT_A2ND_ARRIVAL_CB_UNREG(hdl=%llu)", 
				evt->info.cpnd.info.arr_ntfy.client_hdl);
			break;
		case CPND_EVT_D2ND_CKPT_INFO_UPDATE_ACK:
			snprintf(o_evt_str, len, "CPND_EVT_D2ND_CKPT_INFO_UPDATE_ACK(err=%u)",
				evt->info.cpnd.info.ckpt_info_update_ack.error);
			break;
		default:
			snprintf(o_evt_str, len, "INVALID_CPND_TYPE(type = %d)", evt->info.cpnd.type);
			break;
		}
		break;

	case CPSV_EVT_TYPE_CPA:
		switch (evt->info.cpa.type) {
		case CPA_EVT_MDS_INFO:
			/* This message type is not used */
			snprintf(o_evt_str, len, "CPA_EVT_MDS_INFO");
			break;
		case CPA_EVT_TIME_OUT:
		{
			CPA_TMR_INFO *info = &evt->info.cpa.info.tmr_info;
			snprintf(o_evt_str, len, "CPA_EVT_TIME_OUT(hdl=%llu, type=%s(%u))",
				info->client_hdl, 
				info->type == CPA_TMR_TYPE_CPND_RETENTION ? "RETENTION" :
				info->type == CPA_TMR_TYPE_OPEN ? "OPEN" :
				info->type == CPA_TMR_TYPE_SYNC ? "SYNC" : "INVALID", 
				info->type);
			break;
		}
		case CPA_EVT_ND2A_CKPT_INIT_RSP:
		{
			CPSV_ND2A_INIT_RSP *info = &evt->info.cpa.info.initRsp;
			snprintf(o_evt_str, len, "CPA_EVT_ND2A_CKPT_INIT_RSP(hdl=%llu, err=%u)", info->ckptHandle, info->error);
			break;
		}
		case CPA_EVT_ND2A_CKPT_FINALIZE_RSP:
			snprintf(o_evt_str, len, "CPA_EVT_ND2A_CKPT_FINALIZE_RSP(err=%u)", evt->info.cpa.info.finRsp.error);
			break;
		case CPA_EVT_ND2A_CKPT_OPEN_RSP:
		{
			CPSV_ND2A_OPEN_RSP *info = &evt->info.cpa.info.openRsp;
			snprintf(o_evt_str, len, "CPA_EVT_ND2A_CKPT_OPEN_RSP(err=%u, active=0x%X, gbl_hdl=%llu, lcl_hdl=0x%llX)",
				info->error, m_NCS_NODE_ID_FROM_MDS_DEST(info->active_dest), info->gbl_ckpt_hdl, info->lcl_ckpt_hdl);
			break;
		}
		case CPA_EVT_ND2A_CKPT_CLOSE_RSP:
		{
			snprintf(o_evt_str, len, "CPA_EVT_ND2A_CKPT_CLOSE_RSP(err=%u)", evt->info.cpa.info.closeRsp.error);
			break;
		}
		case CPA_EVT_ND2A_CKPT_UNLINK_RSP:
		{
			snprintf(o_evt_str, len, "CPA_EVT_ND2A_CKPT_UNLINK_RSP(err=%u)", evt->info.cpa.info.ulinkRsp.error);
			break;
		}
		case CPA_EVT_ND2A_CKPT_RDSET_RSP:
			snprintf(o_evt_str, len, "CPA_EVT_ND2A_CKPT_RDSET_RSP(err=%u)", evt->info.cpa.info.rdsetRsp.error);
			break;
		case CPA_EVT_ND2A_CKPT_AREP_SET_RSP:
			snprintf(o_evt_str, len, "CPA_EVT_ND2A_CKPT_AREP_SET_RSP(err=%u)", evt->info.cpa.info.arsetRsp.error);
			break;
		case CPA_EVT_ND2A_CKPT_STATUS:
		{
			CPSV_CKPT_STATUS *info = &evt->info.cpa.info.status;
			snprintf(o_evt_str, len, "[%llu] CPA_EVT_ND2A_CKPT_STATUS(err=%u, mem_used=%llu, "
				"n_secs=%u, ckpt_size=%llu, reten=%llu, max_sec=%u)", info->ckpt_id, info->error, 
				info->status.memoryUsed, info->status.numberOfSections, 
				info->status.checkpointCreationAttributes.checkpointSize,
				info->status.checkpointCreationAttributes.retentionDuration,
				info->status.checkpointCreationAttributes.maxSections);

			break;
		}
		case CPA_EVT_ND2A_SEC_CREATE_RSP:
		{
			CPSV_CKPT_SECT_INFO *info = &evt->info.cpa.info.sec_creat_rsp;
			char sec_id[MAX_SEC_ID_LEN] = {0};
			cpsv_convert_sec_id_to_string(sec_id, &info->sec_id);

			snprintf(o_evt_str, len, "[%llu] CPA_EVT_ND2A_SEC_CREATE_RSP(err=%u, sec_id=%s,  mdest=%llX)", 
				info->ckpt_id, info->error, sec_id, (unsigned long long)info->agent_mdest);
			break;
		}
		case CPA_EVT_ND2A_SEC_DELETE_RSP:
			snprintf(o_evt_str, len, "CPA_EVT_ND2A_SEC_DELETE_RSP(err=%u)", evt->info.cpa.info.sec_delete_rsp.error);
			break;
		case CPA_EVT_ND2A_SEC_EXPTIME_RSP:
			snprintf(o_evt_str, len, "CPA_EVT_ND2A_SEC_EXPTIME_RSP(err=%u)", evt->info.cpa.info.sec_exptmr_rsp.error);
			break;
		case CPA_EVT_ND2A_SEC_ITER_RSP:
			snprintf(o_evt_str, len, "CPA_EVT_ND2A_SEC_ITER_RSP(err=%u)", evt->info.cpa.info.sec_iter_rsp.error);
			break;
		case CPA_EVT_ND2A_SEC_ITER_GETNEXT_RSP:
		{
			CPSV_ND2A_SECT_ITER_GETNEXT_RSP *info = &evt->info.cpa.info.iter_next_rsp;
			char sec_id[MAX_SEC_ID_LEN] = {0};
			cpsv_convert_sec_id_to_string(sec_id, &info->sect_desc.sectionId);

			snprintf(o_evt_str, len, "[%llu] CPA_EVT_ND2A_SEC_ITER_GETNEXT_RSP(err=%u, iter_id=%llu, sec_id=%s, n_secs_trav=%u)", 
				info->ckpt_id, info->error, info->iter_id, sec_id, info->n_secs_trav);
			break;
		}
		case CPA_EVT_ND2A_CKPT_ARRIVAL_NTFY:
		{
			CPSV_ND2A_ARRIVAL_MSG *info = &evt->info.cpa.info.arr_msg;
			snprintf(o_evt_str, len, "CPA_EVT_ND2A_CKPT_ARRIVAL_NTFY(hdl=%llu)", info->client_hdl);
			break;
		}
		case CPA_EVT_ND2A_CKPT_DATA_RSP:
		{
			CPSV_ND2A_DATA_ACCESS_RSP *info = &evt->info.cpa.info.sec_data_rsp;
			snprintf(o_evt_str, len, "CPA_EVT_ND2A_CKPT_DATA_RSP(err=%u, type=%s(%u))", 
				info->type == CPSV_DATA_ACCESS_OVWRITE_RSP ? info->info.ovwrite_error.error : info->error,
				info->type == CPSV_DATA_ACCESS_LCL_READ_RSP ? "LCL_READ" :
				info->type == CPSV_DATA_ACCESS_RMT_READ_RSP ? "RMT_READ" :
				info->type == CPSV_DATA_ACCESS_WRITE_RSP ? "WRITE" :
				info->type == CPSV_DATA_ACCESS_OVWRITE_RSP ? "OVWRITE" : "INVALID", info->type);
			break;
		}
		case CPA_EVT_ND2A_CKPT_SYNC_RSP:
			snprintf(o_evt_str, len, "CPA_EVT_ND2A_CKPT_SYNC_RSP(err=%u)",
				evt->info.cpa.info.sync_rsp.error);
			break;
		case CPA_EVT_D2A_ACT_CKPT_INFO_BCAST_SEND:
		{
			CPSV_CKPT_DEST_INFO *info = &evt->info.cpa.info.ackpt_info;
			snprintf(o_evt_str, len, "[%llu] CPA_EVT_D2A_ACT_CKPT_INFO_BCAST_SEND(active_node=0x%X)",
				info->ckpt_id, m_NCS_NODE_ID_FROM_MDS_DEST(info->mds_dest));
			break;
		}
		case CPA_EVT_ND2A_CKPT_READ_ACK_RSP:
			/* This message type is not used */
			snprintf(o_evt_str, len, "CPA_EVT_ND2A_CKPT_READ_ACK_RSP");
			break;
		case CPA_EVT_ND2A_CKPT_BCAST_SEND:
		{
			snprintf(o_evt_str, len, "CPA_EVT_ND2A_CKPT_BCAST_SEND");
			break;
		}
		case CPA_EVT_D2A_NDRESTART:
		{
			CPSV_CKPT_DEST_INFO *info = &evt->info.cpa.info.ackpt_info;
			snprintf(o_evt_str, len, "[%llu] CPA_EVT_D2A_NDRESTART(node=0x%X)", 
				info->ckpt_id, m_NCS_NODE_ID_FROM_MDS_DEST(info->mds_dest));
			break;
		}
		case CPA_EVT_CB_DUMP:
			snprintf(o_evt_str, len, "CPA_EVT_CB_DUMP");
			break;
		case CPA_EVT_ND2A_CKPT_CLM_NODE_LEFT:
			snprintf(o_evt_str, len, "CPA_EVT_ND2A_CKPT_CLM_NODE_LEFT");
			break;
		case CPA_EVT_ND2A_CKPT_CLM_NODE_JOINED:
			snprintf(o_evt_str, len, "CPA_EVT_ND2A_CKPT_CLM_NODE_JOINED");
			break;
		case CPA_EVT_ND2A_ACT_CKPT_INFO_BCAST_SEND:
		{
			CPSV_CKPT_DEST_INFO *info = &evt->info.cpa.info.ackpt_info;
			snprintf(o_evt_str, len, "[%llu] CPA_EVT_ND2A_ACT_CKPT_INFO_BCAST_SEND(active_node=0x%X)",
				info->ckpt_id, m_NCS_NODE_ID_FROM_MDS_DEST(info->mds_dest));
			break;
		}
		case CPA_EVT_ND2A_CKPT_DESTROY:
		{
			CPSV_CKPT_ID_INFO *info = &evt->info.cpa.info.ckpt_destroy;
			snprintf(o_evt_str, len, "[%llu] CPA_EVT_ND2A_CKPT_DESTROY", info->ckpt_id);
			break;
		}
		default:
			snprintf(o_evt_str, len, "INVALID_CPA_TYPE(type = %d)", evt->info.cpa.type);
			break;
		}			
		break;

	case CPSV_EVT_TYPE_CPD:
		switch (evt->info.cpd.type) {
		case CPD_EVT_MDS_INFO:
		{
			CPSV_MDS_INFO *info = &evt->info.cpd.info.mds_info;
			snprintf(o_evt_str, len, "CPD_EVT_MDS_INFO(dest=0x%llX, svc=%s(%u) - %s(%u))", 
				(unsigned long long)info->dest,
				info->svc_id == NCSMDS_SVC_ID_CPA ? "CPA" :
				info->svc_id == NCSMDS_SVC_ID_CPND ? "CPND" :
				info->svc_id == NCSMDS_SVC_ID_CPD ? "CPD" : "OTHER",
			       	info->svc_id,	
			       	info->change == NCSMDS_NO_ACTIVE ? "NO_ACT" : 
				info->change == NCSMDS_NEW_ACTIVE ? "NEW_ACT" :
				info->change == NCSMDS_UP ? "UP" :
				info->change == NCSMDS_DOWN ? "DOWN" :
			        info->change == NCSMDS_RED_UP ? "RED_UP" :
				info->change == NCSMDS_RED_DOWN ? "RED_DOWN" :
				info->change == NCSMDS_CHG_ROLE ? "CHG_ROLE" : "OTHERS",
				info->change);	
			break;

		}
		case CPD_EVT_ND2D_CKPT_CREATE:
		{
			CPSV_ND2D_CKPT_CREATE *info = &evt->info.cpd.info.ckpt_create;
			snprintf(o_evt_str, len, "CPD_EVT_ND2D_CKPT_CREATE(%s, creationFlags=0x%X)",
				info->ckpt_name.value, info->attributes.creationFlags);
			break;
		}
		case CPD_EVT_ND2D_CKPT_UNLINK:
		{
			snprintf(o_evt_str, len, "CPD_EVT_ND2D_CKPT_UNLINK(%s)", evt->info.cpd.info.ckpt_ulink.ckpt_name.value);
			break;
		}
		case CPD_EVT_ND2D_CKPT_RDSET:
		{
			CPSV_CKPT_RDSET *info = &evt->info.cpd.info.rd_set;
			snprintf(o_evt_str, len, "[%llu] CPD_EVT_ND2D_CKPT_RDSET(reten_time=%llu)", info->ckpt_id, info->reten_time);
			break;
		}
		case CPD_EVT_ND2D_ACTIVE_SET:
		{
			CPSV_CKPT_DEST_INFO *info = &evt->info.cpd.info.arep_set;
			snprintf(o_evt_str, len, "[%llu] CPD_EVT_ND2D_ACTIVE_SET(node=0x%X)", info->ckpt_id,
				m_NCS_NODE_ID_FROM_MDS_DEST(info->mds_dest));
			break;
		}
		case CPD_EVT_ND2D_CKPT_CLOSE:
		{
			CPSV_CKPT_ID_INFO *info = &evt->info.cpd.info.ckpt_close;
			snprintf(o_evt_str, len, "[%llu] CPD_EVT_ND2D_CKPT_CLOSE", info->ckpt_id);
			break;
		}
		case CPD_EVT_ND2D_CKPT_DESTROY:
		{
			CPSV_CKPT_ID_INFO *info = &evt->info.cpd.info.ckpt_destroy;
			snprintf(o_evt_str, len, "[%llu] CPD_EVT_ND2D_CKPT_DESTROY", info->ckpt_id);
			break;
		}
		case CPD_EVT_ND2D_CKPT_USR_INFO:
		{
			CPSV_ND2D_USR_INFO *info = &evt->info.cpd.info.ckpt_usr_info;
			snprintf(o_evt_str, len, "[%llu] CPD_EVT_ND2D_CKPT_USR_INFO(open_flags=0x%X, %s(%u), ckpt_flags=%u)",
				info->ckpt_id, info->ckpt_flags, 
				info->info_type == CPSV_USR_INFO_CKPT_OPEN ? "OPEN" :
				info->info_type == CPSV_USR_INFO_CKPT_CLOSE ? "CLOSE" :
				info->info_type == CPSV_USR_INFO_CKPT_OPEN_FIRST ? "OPEN_FIRST" :
				info->info_type == CPSV_USR_INFO_CKPT_CLOSE_LAST ? "CLOSE_LAST" : "OTHER",
				info->info_type, info->ckpt_flags);
			break;
		}
		case CPD_EVT_ND2D_CKPT_SYNC_INFO:
			/* This message type is not used */
			snprintf(o_evt_str, len, "CPD_EVT_ND2D_CKPT_SYNC_INFO");
			break;
		case CPD_EVT_ND2D_CKPT_SEC_INFO_UPD:
		{
			CPSV_CKPT_SEC_INFO_UPD *info = &evt->info.cpd.info.ckpt_sec_info;
			snprintf(o_evt_str, len, "[%llu] CPD_EVT_ND2D_CKPT_SEC_INFO_UPD(type=%s(%u))", info->ckpt_id,
				info->info_type == CPSV_CKPT_SEC_INFO_CREATE ? "CREATE" :
				info->info_type == CPSV_CKPT_SEC_INFO_DELETE ? "DELETE" : "OTHER",
				info->info_type);
			break;
		}
		case CPD_EVT_ND2D_CKPT_MEM_USED:
		{
			CPSV_CKPT_USED_SIZE *info = &evt->info.cpd.info.ckpt_mem_used;
			snprintf(o_evt_str, len, "[%llu] CPD_EVT_ND2D_CKPT_MEM_USED(err=%u, used_size=%u)", 
				info->ckpt_id, info->error, info->ckpt_used_size);
			break;
		}
		case CPD_EVT_CB_DUMP:
			snprintf(o_evt_str, len, "CPD_EVT_CB_DUMP");
			break;
		case CPD_EVT_MDS_QUIESCED_ACK_RSP:
			snprintf(o_evt_str, len, "CPD_EVT_MDS_QUIESCED_ACK_RSP");
			break;
		case CPD_EVT_ND2D_CKPT_DESTROY_BYNAME:
		{
			CPSV_CKPT_NAME_INFO *info = &evt->info.cpd.info.ckpt_destroy_byname;
			snprintf(o_evt_str, len, "CPD_EVT_ND2D_CKPT_DESTROY_BYNAME(ckpt=%s)", info->ckpt_name.value);
			break;
		}
		case CPD_EVT_ND2D_CKPT_CREATED_SECTIONS:
		{
			CPSV_CKPT_NUM_SECTIONS *info = &evt->info.cpd.info.ckpt_created_sections;
			snprintf(o_evt_str, len, "[%llu] CPD_EVT_ND2D_CKPT_CREATED_SECTIONS(err=%u, num_secs=%u)", 
				info->ckpt_id, info->error, info->ckpt_num_sections);
			break;
		}
		case CPD_EVT_TIME_OUT:
		{
			CPD_TMR_INFO *info = &evt->info.cpd.info.tmr_info;
			snprintf(o_evt_str, len, "CPD_EVT_TIME_OUT(type=%u, node=0x%X)", info->type, 
				m_NCS_NODE_ID_FROM_MDS_DEST(info->info.cpnd_dest));
			break;
		}
		case CPD_EVT_ND2D_CKPT_INFO_UPDATE:
		{
			CPSV_ND2D_CKPT_INFO_UPD *info = &evt->info.cpd.info.ckpt_info;
			snprintf(o_evt_str, len, "[%llu] CPD_EVT_ND2D_CKPT_INFO_UPDATE(%s, creationFlags=0x%X, "
				"openFlags=0x%X, numbers[U/W/R]=[%u/%u/%u], active=%s, last=%s)", info->ckpt_id,
				info->ckpt_name.value, info->attributes.creationFlags, info->ckpt_flags, info->num_users,
				info->num_writers, info->num_readers, info->is_active ? "true" : "false",
				info->is_last ? "true" : "false");
			break;
		}
		default:
			snprintf(o_evt_str, len, "INVALID_CPD_TYPE(type = %d)", evt->info.cpd.type);
			break;
		}
		break;
	default:
		snprintf(o_evt_str, len, "INVALID_EVENT_TYPE(type = %d)", evt->type);
	}
	return o_evt_str;
}

/****************************************************************************\
 PROCEDURE NAME : cpsv_evt_cpy

 DESCRIPTION    : This routine will allocate the memory for CPSV_EVT and its
                  internal pointers and copy the contents.

 ARGUMENTS      : cb : CPA control Block.
                  cpy  : copy info.

 RETURNS        : None

 NOTES          : 
\*****************************************************************************/
uint32_t cpsv_evt_cpy(CPSV_EVT *src, CPSV_EVT *dest, uint32_t svc_id)
{
	uint32_t rc = NCSCC_RC_SUCCESS;

	dest = m_MMGR_ALLOC_CPSV_EVT(svc_id);

	if (dest) {
		memcpy(dest, src, sizeof(CPSV_EVT));

		/* Copy the internal pointers TBD */
	} else {
		rc = NCSCC_RC_OUT_OF_MEM;
	}

	return rc;

}
uint32_t cpsv_ref_cnt_encode(NCS_UBAID *i_ub, CPSV_A2ND_REFCNTSET *data)
{
   uint32_t rc= NCSCC_RC_SUCCESS;
   uint8_t *pStream = NULL; 
   uint32_t counter =0,array_cnt=0;

   pStream = ncs_enc_reserve_space(i_ub, 4);
   ncs_encode_32bit(&pStream, data->no_of_nodes);
   ncs_enc_claim_space(i_ub, 4);


   counter = data->no_of_nodes;
   while(counter!=0)
   {
     pStream = ncs_enc_reserve_space(i_ub,12 );
     ncs_encode_64bit(&pStream , data->ref_cnt_array[array_cnt].ckpt_id); 
     ncs_encode_32bit(&pStream , data->ref_cnt_array[array_cnt].ckpt_ref_cnt); 
     counter--;
     array_cnt++;
     ncs_enc_claim_space(i_ub, 12);
   }
   
   return rc;
}


uint32_t cpsv_refcnt_ckptid_decode(CPSV_A2ND_REFCNTSET *pdata , NCS_UBAID *io_uba)
{
   uint8_t *pstream = NULL;
   uint8_t local_data[50];
   uint32_t counter=0,array_cnt=0;
 

   pstream = ncs_dec_flatten_space(io_uba,local_data, 4);
   pdata->no_of_nodes = ncs_decode_32bit(&pstream);
   ncs_dec_skip_space(io_uba, 4);

   counter =  pdata->no_of_nodes; 
   while (counter != 0)
   {
    pstream = ncs_dec_flatten_space(io_uba,local_data, 12);
    pdata->ref_cnt_array[array_cnt].ckpt_id = ncs_decode_64bit(&pstream);
    pdata->ref_cnt_array[array_cnt].ckpt_ref_cnt = ncs_decode_32bit(&pstream);
    ncs_dec_skip_space(io_uba, 12);
    counter--;
    array_cnt++;
   }
   return NCSCC_RC_SUCCESS;
}

/****************************************************************************\
 PROCEDURE NAME : cpsv_ckpt_data_encode

 DESCRIPTION    : This routine will encode the contents of CPSV_EVT into user buf

 ARGUMENTS      : *data - CPSV_CKPT_DATA
                  *i_ub  - User Buff.

 RETURNS        : None

 NOTES          : 
\*****************************************************************************/

uint32_t cpsv_ckpt_data_encode(NCS_UBAID *i_ub, CPSV_CKPT_DATA *data)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	uint8_t *header = NULL;
	CPSV_CKPT_DATA *pdata = data;
	uint16_t num_of_nodes = 0;

	header = ncs_enc_reserve_space(i_ub, 2);
	if (!header)
		return m_CPSV_DBG_SINK(NCSCC_RC_FAILURE, "Memory alloc failed in cpsv_ckpt_data_encode \n");

	ncs_enc_claim_space(i_ub, 2);

	while (pdata != NULL) {
		cpsv_ckpt_node_encode(i_ub, pdata);
		num_of_nodes++;
		pdata = pdata->next;
	}
	ncs_encode_16bit(&header, num_of_nodes);

	return rc;

}

/****************************************************************************\
 PROCEDURE NAME : cpsv_ckpt_node_encode

 DESCRIPTION    : This routine will encode the contents from CPSV_CKPT_DATA* node pdata 
                  into the io buffer NCS_UBAID *i_ub.

 ARGUMENTS      : i_ub  - NCS_UBAID pointer.
                  pdata - CPSV_CKPT_DATA pointer.

 RETURNS        : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

 NOTES          :
\*****************************************************************************/

uint32_t cpsv_ckpt_node_encode(NCS_UBAID *i_ub, CPSV_CKPT_DATA *pdata)
{
	uint8_t *pStream = NULL;
	uint32_t rc = NCSCC_RC_SUCCESS, size;

	pStream = ncs_enc_reserve_space(i_ub, 2);
	if (!pStream)
		return m_CPSV_DBG_SINK(NCSCC_RC_FAILURE, "Memory alloc failed in cpsv_ckpt_node_enoode \n");

/* Section id */
	ncs_encode_16bit(&pStream, pdata->sec_id.idLen);
	ncs_enc_claim_space(i_ub, 2);
	if (pdata->sec_id.idLen)
		ncs_encode_n_octets_in_uba(i_ub, (uint8_t *)pdata->sec_id.id, (uint32_t)pdata->sec_id.idLen);

	size = 8 + 8 + 8;
	pStream = ncs_enc_reserve_space(i_ub, size);
	if (!pStream)
		return m_CPSV_DBG_SINK(NCSCC_RC_FAILURE, "Memory alloc failed in cpsv_ckpt_node_encode \n");

	ncs_encode_64bit(&pStream, pdata->expirationTime);
	ncs_encode_64bit(&pStream, pdata->dataSize);
	ncs_encode_64bit(&pStream, pdata->readSize);
	ncs_enc_claim_space(i_ub, size);

	if (pdata->dataSize)
		ncs_encode_n_octets_in_uba(i_ub, pdata->data, (uint32_t)pdata->dataSize);

	pStream = ncs_enc_reserve_space(i_ub, 8);
	if (!pStream)
		return m_CPSV_DBG_SINK(NCSCC_RC_FAILURE, "Memory alloc failed in cpsv_ckpt_node_encode\n");

	ncs_encode_64bit(&pStream, pdata->dataOffset);
	ncs_enc_claim_space(i_ub, 8);
	return rc;
}

/****************************************************************************\
 PROCEDURE NAME : cpsv_ckpt_node_encode

 DESCRIPTION    : This routine will encode the contents from CPSV_CKPT_DATA* node pdata 
                  into the io buffer NCS_UBAID *i_ub.

 ARGUMENTS      : io_ub  - NCS_UBAID pointer.
                  pdata - CPSV_CKPT_DATA pointer.

 RETURNS        : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

 NOTES          :
\*****************************************************************************/

uint32_t cpsv_ckpt_access_encode(CPSV_CKPT_ACCESS *ckpt_data, NCS_UBAID *io_uba)
{

	uint32_t rc = NCSCC_RC_SUCCESS;
	uint8_t *pstream = NULL;
	uint32_t space, all_repl_evt_flag = 0, is_ckpt_open = 0;

	space = 4 + 8 + 8 + 8 + 4 + 4;
	pstream = ncs_enc_reserve_space(io_uba, space);
	if (!pstream)
		return m_CPSV_DBG_SINK(NCSCC_RC_FAILURE, "Memory alloc failed in cpsv_ckpt_access_encode\n");

	ncs_encode_32bit(&pstream, ckpt_data->type);
	ncs_encode_64bit(&pstream, ckpt_data->ckpt_id);
	ncs_encode_64bit(&pstream, ckpt_data->lcl_ckpt_id);
	ncs_encode_64bit(&pstream, ckpt_data->agent_mdest);
	ncs_encode_32bit(&pstream, ckpt_data->num_of_elmts);
	all_repl_evt_flag = ckpt_data->all_repl_evt_flag;
	ncs_encode_32bit(&pstream, all_repl_evt_flag);

	ncs_enc_claim_space(io_uba, space);

	/* CKPT_DATA LINKED LIST OF EDU */
	cpsv_ckpt_data_encode(io_uba, ckpt_data->data);

	/* Following are Not Required But for Write/Read (Used in checkpoint sync evt) compatiblity with 3.0.2 */
	space = 4 + 1 + 8 + 8 + 8 + 8 + 4 + 8 + 4 + 4 + 1 /*+ MDS_SYNC_SND_CTXT_LEN_MAX */ ;
	pstream = ncs_enc_reserve_space(io_uba, space);
	if (!pstream)
		return m_CPSV_DBG_SINK(NCSCC_RC_FAILURE, "Memory alloc failed in cpsv_ckpt_access_encode\n");

	ncs_encode_32bit(&pstream, ckpt_data->seqno);
	ncs_encode_8bit(&pstream, ckpt_data->last_seq);
	ncs_encode_64bit(&pstream, ckpt_data->ckpt_sync.ckpt_id);
	ncs_encode_64bit(&pstream, ckpt_data->ckpt_sync.invocation);
	ncs_encode_64bit(&pstream, ckpt_data->ckpt_sync.lcl_ckpt_hdl);
	ncs_encode_64bit(&pstream, ckpt_data->ckpt_sync.client_hdl);
	is_ckpt_open = ckpt_data->ckpt_sync.is_ckpt_open;
	ncs_encode_32bit(&pstream, is_ckpt_open);
	ncs_encode_64bit(&pstream, ckpt_data->ckpt_sync.cpa_sinfo.dest);
	ncs_encode_32bit(&pstream, ckpt_data->ckpt_sync.cpa_sinfo.stype);
	ncs_encode_32bit(&pstream, ckpt_data->ckpt_sync.cpa_sinfo.to_svc);
	ncs_encode_8bit(&pstream, ckpt_data->ckpt_sync.cpa_sinfo.ctxt.length);
	ncs_enc_claim_space(io_uba, space);
	ncs_encode_n_octets_in_uba(io_uba, ckpt_data->ckpt_sync.cpa_sinfo.ctxt.data, (uint32_t)MDS_SYNC_SND_CTXT_LEN_MAX);
	return rc;
}

/****************************************************************************
  Name          : cpsv_nd2a_read_data_encode
 
  Description   : This function encodes the CPSV_ND2A_READ_DATA.
 
  Arguments     : CPSV_ND2A_READ_DATA *read_data
                  NCS_UBAID *io_uba
                  
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/

uint32_t cpsv_nd2a_read_data_encode(CPSV_ND2A_READ_DATA *read_data, NCS_UBAID *io_uba)
{
	uint8_t *pstream = NULL;

	pstream = ncs_enc_reserve_space(io_uba, 4);
	if (!pstream)
		return m_CPSV_DBG_SINK(NCSCC_RC_FAILURE, "Memory alloc failed in cpsv_nd2a_read_data_encode\n");

	ncs_encode_32bit(&pstream, read_data->read_size);	/* Type */
	ncs_enc_claim_space(io_uba, 4);

	if (read_data->read_size) {
		ncs_encode_n_octets_in_uba(io_uba, read_data->data, (uint32_t)read_data->read_size);
	}

	pstream = ncs_enc_reserve_space(io_uba, 4);
	if (!pstream)
		return m_CPSV_DBG_SINK(NCSCC_RC_FAILURE, "Memory alloc failed in cpsv_nd2a_read_data_encode\n");
	ncs_encode_32bit(&pstream, read_data->err);	/* Type */
	ncs_enc_claim_space(io_uba, 4);

	return NCSCC_RC_SUCCESS;

}

/****************************************************************************\
 PROCEDURE NAME : cpsv_data_access_rsp_encode

 DESCRIPTION    : This routine will encode the contents of CPSV_ND2A_DATA_ACCESS_RSP* into user buf

 ARGUMENTS      : *CPSV_ND2A_DATA_ACCESS_RSP *data_rsp .
                  *io_ub  - User Buff.
				  MDS_CLIENT_MSG_FORMAT_VER.

 RETURNS        : None

 NOTES          : 
\*****************************************************************************/
uint32_t cpsv_data_access_rsp_encode(CPSV_ND2A_DATA_ACCESS_RSP *data_rsp, NCS_UBAID *io_uba, MDS_CLIENT_MSG_FORMAT_VER o_msg_fmt_ver)
{

	uint8_t *pstream = NULL;
	uint32_t size, i;
	uint32_t ver_compare = 4; /* CPND_MDS_PVT_SUBPART_VERSION  and CPA_MDS_PVT_SUBPART_VERSION*/
	if (o_msg_fmt_ver  >= ver_compare)
		size = 4 + 4 + 4 + 4 + 8 + 4 + 8 + 8;   
	else
		size = 4 + 4 + 4 + 4 + 8 + 4 + 8;
   pstream = ncs_enc_reserve_space(io_uba, size);
   if(!pstream)
            return m_CPSV_DBG_SINK(NCSCC_RC_FAILURE,"Memory alloc failed in cpsv_data_access_rsp_encode\n");
   ncs_encode_32bit(&pstream , data_rsp->type);
   ncs_encode_32bit(&pstream , data_rsp->num_of_elmts);
   ncs_encode_32bit(&pstream , data_rsp->error);
   ncs_encode_32bit(&pstream , data_rsp->size);
   ncs_encode_64bit(&pstream , data_rsp->ckpt_id);
   ncs_encode_32bit(&pstream , data_rsp->error_index);
   ncs_encode_64bit(&pstream , data_rsp->from_svc);
   if (o_msg_fmt_ver  >= ver_compare) 
	   ncs_encode_64bit(&pstream , data_rsp->lcl_ckpt_id);
   ncs_enc_claim_space(io_uba, size);

	if (data_rsp->type == CPSV_DATA_ACCESS_WRITE_RSP) {

		SaUint32T *write_err_index = data_rsp->info.write_err_index;
		size = data_rsp->size * sizeof(SaUint32T);
		if (size) {
			pstream = ncs_enc_reserve_space(io_uba, size);
			if (!pstream)
				return m_CPSV_DBG_SINK(NCSCC_RC_FAILURE,
						       "Memory alloc failed in cpsv_data_access_rsp_encode\n");
			for (i = 0; i < data_rsp->size; i++) {
				/* Encode Write Error Index */
				ncs_encode_32bit(&pstream, write_err_index[i]);
			}
			ncs_enc_claim_space(io_uba, size);
		}

	} else if ((data_rsp->type == CPSV_DATA_ACCESS_LCL_READ_RSP)
		   || (data_rsp->type == CPSV_DATA_ACCESS_RMT_READ_RSP)) {

		for (i = 0; i < data_rsp->size; i++) {
			cpsv_nd2a_read_data_encode(&data_rsp->info.read_data[i], io_uba);
		}

	} else if (data_rsp->type == CPSV_DATA_ACCESS_OVWRITE_RSP) {

		pstream = ncs_enc_reserve_space(io_uba, 4);
		if (!pstream)
			return m_CPSV_DBG_SINK(NCSCC_RC_FAILURE,
					       "Memory alloc failed in cpsv_data_access_rsp_encode\n");
		ncs_encode_32bit(&pstream, data_rsp->info.ovwrite_error.error);
		ncs_enc_claim_space(io_uba, 4);
	}
	return NCSCC_RC_SUCCESS;

}

/****************************************************************************\
 PROCEDURE NAME : cpsv_evt_enc_flat

 DESCRIPTION    : This routine will encode the contents of CPSV_EVT into user buf

 ARGUMENTS      : *i_evt - Event Struct.
                  *o_ub  - User Buff.

 RETURNS        : None

 NOTES          : 
\*****************************************************************************/
uint32_t cpsv_evt_enc_flat(EDU_HDL *edu_hdl, CPSV_EVT *i_evt, NCS_UBAID *o_ub)
{
	uint32_t size;
	uint32_t rc = NCSCC_RC_SUCCESS;
	size = sizeof(CPSV_EVT);

	/* Encode the Top level evt envolop */
	ncs_encode_n_octets_in_uba(o_ub, (uint8_t *)i_evt, size);

	/* Encode the internal Pointers */
	if (i_evt->type == CPSV_EVT_TYPE_CPA) {
		if (i_evt->info.cpa.type == CPA_EVT_ND2A_CKPT_DATA_RSP) {
			CPSV_ND2A_DATA_ACCESS_RSP *data_rsp = &i_evt->info.cpa.info.sec_data_rsp;
			if ((data_rsp->type == CPSV_DATA_ACCESS_LCL_READ_RSP)
			    || (data_rsp->type == CPSV_DATA_ACCESS_RMT_READ_RSP)) {
				if (data_rsp->num_of_elmts == -1) {
					size = 0;
				} else {
					if (data_rsp->size > 0) {
						uint32_t iter = 0;
						for (; iter < data_rsp->size; iter++) {
							size = sizeof(CPSV_ND2A_READ_DATA);
							ncs_encode_n_octets_in_uba(o_ub,
										   (uint8_t *)&data_rsp->
										   info.read_data[iter]
										   , size);
							if (data_rsp->info.read_data[iter].read_size > 0) {
								size = data_rsp->info.read_data[iter].read_size;
								ncs_encode_n_octets_in_uba(o_ub,
											   (uint8_t *)data_rsp->info.
											   read_data[iter].data, size);
							}
						}
					}
				}
			} else if (data_rsp->type == CPSV_DATA_ACCESS_WRITE_RSP) {
				if (data_rsp->num_of_elmts == -1)
					size = 0;
				else
					size = data_rsp->num_of_elmts * sizeof(SaUint32T);
				if (size)
					ncs_encode_n_octets_in_uba(o_ub, (uint8_t *)data_rsp->info.write_err_index, size);
			}
		} else if (i_evt->info.cpa.type == CPA_EVT_ND2A_CKPT_CLM_NODE_LEFT) {
			/* Do nothing */
		} else if (i_evt->info.cpa.type == CPA_EVT_ND2A_CKPT_CLM_NODE_JOINED) {
			/* Do nothing */
		} else if (i_evt->info.cpa.type == CPA_EVT_ND2A_CKPT_ARRIVAL_NTFY) {
			CPSV_CKPT_DATA *data = i_evt->info.cpa.info.arr_msg.ckpt_data;
			EDU_ERR ederror = 0;
			rc = m_NCS_EDU_EXEC(edu_hdl, FUNC_NAME(CPSV_CKPT_DATA), o_ub, EDP_OP_TYPE_ENC, data, &ederror);
			if (rc != NCSCC_RC_SUCCESS) {
				return rc;
			}
		}

		else if (i_evt->info.cpa.type == CPA_EVT_ND2A_SEC_CREATE_RSP) {
			SaCkptSectionIdT *sec_id = &i_evt->info.cpa.info.sec_creat_rsp.sec_id;
			if (sec_id->idLen) {
				ncs_encode_n_octets_in_uba(o_ub, (uint8_t *)sec_id->id, sec_id->idLen);
			}
		}

		else if (i_evt->info.cpa.type == CPA_EVT_ND2A_SEC_ITER_GETNEXT_RSP) {
			SaCkptSectionIdT *sec_id = &i_evt->info.cpa.info.iter_next_rsp.sect_desc.sectionId;
			if (sec_id->idLen) {
				ncs_encode_n_octets_in_uba(o_ub, (uint8_t *)sec_id->id, sec_id->idLen);
			}
		}

	} else if (i_evt->type == CPSV_EVT_TYPE_CPND) {
		if (i_evt->info.cpnd.type == CPND_EVT_A2ND_CKPT_WRITE) {
			/* CKPT_DATA LINKED LIST OF EDU */
			cpsv_ckpt_data_encode(o_ub, i_evt->info.cpnd.info.ckpt_write.data);

		} else if (i_evt->info.cpnd.type == CPND_EVT_A2ND_CKPT_READ) {
			cpsv_ckpt_data_encode(o_ub, i_evt->info.cpnd.info.ckpt_read.data);
		}
		/* Added for 3.0.B , these events encoding is missing in 3.0.2 */
		else if (i_evt->info.cpnd.type == CPND_EVT_ND2ND_CKPT_ACTIVE_SYNC) {
			cpsv_ckpt_data_encode(o_ub, i_evt->info.cpnd.info.ckpt_nd2nd_sync.data);
		} else if (i_evt->info.cpnd.type == CPSV_EVT_ND2ND_CKPT_SECT_ACTIVE_DATA_ACCESS_REQ) {
			cpsv_ckpt_data_encode(o_ub, i_evt->info.cpnd.info.ckpt_nd2nd_data.data);
		} else if (i_evt->info.cpnd.type == CPSV_EVT_ND2ND_CKPT_SECT_ACTIVE_DATA_ACCESS_RSP) {
			CPSV_ND2A_DATA_ACCESS_RSP *data_rsp = &i_evt->info.cpnd.info.ckpt_nd2nd_data_rsp;
			if (data_rsp->type == CPSV_DATA_ACCESS_WRITE_RSP) {
				if (data_rsp->num_of_elmts == -1)
					size = 0;
				else
					size = data_rsp->num_of_elmts * sizeof(SaUint32T);
				if (size)
					ncs_encode_n_octets_in_uba(o_ub, (uint8_t *)data_rsp->info.write_err_index, size);
			}

		} else if (i_evt->info.cpnd.type == CPSV_EVT_ND2ND_CKPT_SECT_CREATE_REQ) {
			CPSV_CKPT_SECT_CREATE *create = &i_evt->info.cpnd.info.active_sec_creat;

			if (create->sec_attri.sectionId) {
				cpsv_evt_enc_sec_id(o_ub, create->sec_attri.sectionId);
			}
			if (create->init_size)
				ncs_encode_n_octets_in_uba(o_ub, (uint8_t *)create->init_data, create->init_size);
		} else if (i_evt->info.cpnd.type == CPSV_EVT_ND2ND_CKPT_SECT_ACTIVE_CREATE_RSP) {
			SaCkptSectionIdT *sec_id = &i_evt->info.cpnd.info.active_sec_creat_rsp.sec_id;
			if (sec_id->idLen) {
				ncs_encode_n_octets_in_uba(o_ub, (uint8_t *)sec_id->id, sec_id->idLen);

			}
		} else if (i_evt->info.cpnd.type == CPSV_EVT_ND2ND_CKPT_SECT_DELETE_REQ) {
			SaCkptSectionIdT *sec_id = &i_evt->info.cpnd.info.sec_delete_req.sec_id;
			if (sec_id->idLen) {
				ncs_encode_n_octets_in_uba(o_ub, (uint8_t *)sec_id->id, sec_id->idLen);

			}
		}
		/* End of Added for 3.0.B , these events encoding is missing in 3.0.2 */
		else if (i_evt->info.cpnd.type == CPND_EVT_A2ND_CKPT_SECT_CREATE) {
			uint32_t size;
			CPSV_CKPT_SECT_CREATE *create = &i_evt->info.cpnd.info.sec_creatReq;

			if (create->sec_attri.sectionId) {
				cpsv_evt_enc_sec_id(o_ub, create->sec_attri.sectionId);
			}

			size = create->init_size;
			if (size != 0)
				ncs_encode_n_octets_in_uba(o_ub, (uint8_t *)create->init_data, size);
		} else if (i_evt->info.cpnd.type == CPND_EVT_A2ND_CKPT_ITER_GETNEXT) {
			SaCkptSectionIdT *sec_id = &i_evt->info.cpnd.info.iter_getnext.section_id;
			size = sec_id->idLen;
			if (size)
				ncs_encode_n_octets_in_uba(o_ub, (uint8_t *)sec_id->id, size);
		} else if (i_evt->info.cpnd.type == CPND_EVT_A2ND_CKPT_SECT_DELETE) {
			SaCkptSectionIdT *sec_id = &i_evt->info.cpnd.info.sec_delReq.sec_id;
			if (sec_id->idLen) {
				size = sec_id->idLen;
				if (size)
					ncs_encode_n_octets_in_uba(o_ub, (uint8_t *)sec_id->id, size);
			}
		} else if (i_evt->info.cpnd.type == CPND_EVT_A2ND_CKPT_SECT_EXP_SET) {
			SaCkptSectionIdT *sec_id = &i_evt->info.cpnd.info.sec_expset.sec_id;
			if (sec_id->idLen) {
				size = sec_id->idLen;
				if (size)
					ncs_encode_n_octets_in_uba(o_ub, (uint8_t *)sec_id->id, size);
			}
		} else if (i_evt->info.cpnd.type == CPSV_EVT_ND2ND_CKPT_SECT_EXPTMR_REQ) {
			SaCkptSectionIdT *sec_id = &i_evt->info.cpnd.info.sec_exp_set.sec_id;
			if (sec_id->idLen) {
				ncs_encode_n_octets_in_uba(o_ub, (uint8_t *)sec_id->id, sec_id->idLen);
			}
		} else if (i_evt->info.cpnd.type == CPND_EVT_D2ND_CKPT_INFO) {
			CPSV_CPND_DEST_INFO *dest_list = i_evt->info.cpnd.info.ckpt_info.dest_list;
			if (i_evt->info.cpnd.info.ckpt_info.dest_cnt) {
				size = i_evt->info.cpnd.info.ckpt_info.dest_cnt * sizeof(CPSV_CPND_DEST_INFO);
				ncs_encode_n_octets_in_uba(o_ub, (uint8_t *)dest_list, size);
			}
		} else if (i_evt->info.cpnd.type == CPND_EVT_D2ND_CKPT_CREATE) {
			CPSV_CPND_DEST_INFO *dest_list = i_evt->info.cpnd.info.ckpt_create.ckpt_info.dest_list;
			if (i_evt->info.cpnd.info.ckpt_create.ckpt_info.dest_cnt) {
				size =
				    i_evt->info.cpnd.info.ckpt_create.ckpt_info.dest_cnt * sizeof(CPSV_CPND_DEST_INFO);
				ncs_encode_n_octets_in_uba(o_ub, (uint8_t *)dest_list, size);
			}
		} else if (i_evt->info.cpnd.type == CPSV_D2ND_RESTART_DONE) {
			CPSV_CPND_DEST_INFO *dest_list = i_evt->info.cpnd.info.cpnd_restart_done.dest_list;
			if (i_evt->info.cpnd.info.cpnd_restart_done.dest_cnt) {
				size = i_evt->info.cpnd.info.cpnd_restart_done.dest_cnt * sizeof(CPSV_CPND_DEST_INFO);
				ncs_encode_n_octets_in_uba(o_ub, (uint8_t *)dest_list, size);
			}
		} else if (i_evt->info.cpnd.type == CPND_EVT_D2ND_CKPT_REP_ADD) {
			CPSV_CPND_DEST_INFO *dest_list = i_evt->info.cpnd.info.ckpt_add.dest_list;
			if (i_evt->info.cpnd.info.ckpt_add.dest_cnt) {
				size = i_evt->info.cpnd.info.ckpt_add.dest_cnt * sizeof(CPSV_CPND_DEST_INFO);
				ncs_encode_n_octets_in_uba(o_ub, (uint8_t *)dest_list, size);
			}
		}
      else if(i_evt->info.cpnd.type == CPND_EVT_A2ND_CKPT_REFCNTSET)
      {
       if(i_evt->info.cpnd.info.refCntsetReq.no_of_nodes)
           cpsv_ref_cnt_encode(o_ub, &i_evt->info.cpnd.info.refCntsetReq);
      }
    }
	return NCSCC_RC_SUCCESS;
}

static uint32_t cpsv_evt_enc_sec_id(NCS_UBAID *o_ub, SaCkptSectionIdT *sec_id)
{
	uint32_t size;

	size = sizeof(SaCkptSectionIdT);
	ncs_encode_n_octets_in_uba(o_ub, (uint8_t *)sec_id, size);
	if (sec_id->idLen) {
		size = sec_id->idLen;
		ncs_encode_n_octets_in_uba(o_ub, (uint8_t *)sec_id->id, size);
	}
	return NCSCC_RC_SUCCESS;
}

static SaCkptSectionIdT *cpsv_evt_dec_sec_id(NCS_UBAID *i_ub, uint32_t svc_id)
{
	uint32_t size;
	SaCkptSectionIdT *sec_id = NULL;

	size = sizeof(SaCkptSectionIdT);
	sec_id = (SaCkptSectionIdT *)m_MMGR_ALLOC_CPSV_SaCkptSectionIdT(svc_id);

	if (sec_id == NULL) {
		return NULL;
	}

	if (sec_id)
		ncs_decode_n_octets_from_uba(i_ub, (uint8_t *)sec_id, size);

	if (sec_id->idLen) {
		size = sec_id->idLen;
		sec_id->id = m_MMGR_ALLOC_CPSV_DEFAULT_VAL(size + 1, svc_id);
		if (sec_id->id) {
			ncs_decode_n_octets_from_uba(i_ub, (uint8_t *)sec_id->id, size);
			sec_id->id[size] = 0;
		}
	}
	return sec_id;
}

/****************************************************************************\
 PROCEDURE NAME : cpsv_ckpt_data_decode

 DESCRIPTION    : This routine will decode the contents of CPSV_EVT into user buf

 ARGUMENTS      :  CPSV_CKPT_DATA **data.
                  *io_ub  - User Buff.

 RETURNS        : None

 NOTES          : 
\*****************************************************************************/

uint32_t cpsv_ckpt_data_decode(CPSV_CKPT_DATA **data, NCS_UBAID *io_uba)
{
	uint8_t *pstream = NULL;
	uint8_t local_data[5];
	uint16_t num_of_nodes;
	uint32_t rc;
	CPSV_CKPT_DATA *pdata;

	pstream = ncs_dec_flatten_space(io_uba, local_data, 2);
	num_of_nodes = ncs_decode_16bit(&pstream);
	ncs_dec_skip_space(io_uba, 2);

	/* For The First Node */
	if (num_of_nodes) {
		*data = m_MMGR_ALLOC_CPSV_CKPT_DATA;
		if (!(*data))
			return NCSCC_RC_FAILURE;
	}

	pdata = *data;
	while (num_of_nodes) {
		memset(pdata, 0, sizeof(CPSV_CKPT_DATA));
		rc = cpsv_ckpt_node_decode(pdata, io_uba);
		if (rc != NCSCC_RC_SUCCESS)
			return NCSCC_RC_FAILURE;
		num_of_nodes--;
		if (num_of_nodes) {	/* For rest of the Nodes of the Linked List */
			pdata->next = m_MMGR_ALLOC_CPSV_CKPT_DATA;
			if (!pdata->next) {
				return NCSCC_RC_FAILURE;
			}
			pdata = pdata->next;
		}
	}

	return NCSCC_RC_SUCCESS;

}

/****************************************************************************\
 PROCEDURE NAME : cpsv_ckpt_node_decode

 DESCRIPTION    : This routine will decode the contents of CPSV_EVT into user buf

 ARGUMENTS      :  CPSV_CKPT_DATA *pdata.
                  *io_ub  - User Buff.

 RETURNS        : None

 NOTES          : 
\*****************************************************************************/

uint32_t cpsv_ckpt_node_decode(CPSV_CKPT_DATA *pdata, NCS_UBAID *io_uba)
{
	uint8_t *pstream = NULL;
	uint8_t local_data[50];
	uint32_t size;

	pstream = ncs_dec_flatten_space(io_uba, local_data, 2);
	pdata->sec_id.idLen = ncs_decode_16bit(&pstream);
	ncs_dec_skip_space(io_uba, 2);

	/* Allocate Memory for sec_id.id */
	if (pdata->sec_id.idLen) {
		pdata->sec_id.id = m_MMGR_ALLOC_CPSV_DEFAULT_VAL(pdata->sec_id.idLen + 1, NCS_SERVICE_ID_CPND);
		if (!pdata->sec_id.id) {
			m_MMGR_FREE_CPSV_CKPT_DATA(pdata);
			return NCSCC_RC_FAILURE;
		}
		memset(pdata->sec_id.id, 0, pdata->sec_id.idLen + 1);
		ncs_decode_n_octets_from_uba(io_uba, pdata->sec_id.id, (uint32_t)pdata->sec_id.idLen);
	}
	size = 8 + 8 + 8;
	pstream = ncs_dec_flatten_space(io_uba, local_data, size);
	pdata->expirationTime = ncs_decode_64bit(&pstream);
	pdata->dataSize = ncs_decode_64bit(&pstream);
	pdata->readSize = ncs_decode_64bit(&pstream);
	ncs_dec_skip_space(io_uba, size);

	/* Allocate Memory for the data */
	if (pdata->dataSize) {
		pdata->data = m_MMGR_ALLOC_CPSV_SYS_MEMORY(pdata->dataSize);
		if (!pdata->data) {
			m_MMGR_FREE_CPSV_DEFAULT_VAL(pdata->sec_id.id, NCS_SERVICE_ID_CPND);
			m_MMGR_FREE_CPSV_CKPT_DATA(pdata);
			return NCSCC_RC_FAILURE;
		}
		memset(pdata->data, 0, pdata->dataSize);

		ncs_decode_n_octets_from_uba(io_uba, pdata->data, (uint32_t)pdata->dataSize);
	}
	pstream = ncs_dec_flatten_space(io_uba, local_data, 8);
	pdata->dataOffset = ncs_decode_64bit(&pstream);
	ncs_dec_skip_space(io_uba, 8);

	return NCSCC_RC_SUCCESS;

}

/****************************************************************************
  Name          : cpsv_nd2a_read_data_decode

  Description   : This function decodes an events sent to CPA.

  Arguments     : CPSV_ND2A_READ_DATA *read_data
                  NCS_UBAID *io_uba

  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

  Notes         : This is done as Replacement for EDU decode for Read Data.
******************************************************************************/

uint32_t cpsv_nd2a_read_data_decode(CPSV_ND2A_READ_DATA *read_data, NCS_UBAID *io_uba)
{

	uint8_t *pstream = NULL;
	uint8_t local_data[10];

	pstream = ncs_dec_flatten_space(io_uba, local_data, 4);
	read_data->read_size = ncs_decode_32bit(&pstream);
	ncs_dec_skip_space(io_uba, 4);

	if (read_data->read_size) {
		/* Allocate Memory For read_data->data */
		read_data->data = m_MMGR_ALLOC_CPSV_DEFAULT_VAL(read_data->read_size, NCS_SERVICE_ID_CPA);
		if (!read_data->data) {
			/* Free the Above Memory and Return */
			m_MMGR_FREE_CPSV_ND2A_READ_DATA(read_data, NCS_SERVICE_ID_CPA);
			return m_CPSV_DBG_SINK(NCSCC_RC_FAILURE,
					       "Memory alloc failed in cpsv_nd2a_read_data_decode \n");
		}
		memset(read_data->data, 0, read_data->read_size);
		ncs_decode_n_octets_from_uba(io_uba, read_data->data, (uint32_t)read_data->read_size);

	}
	pstream = ncs_dec_flatten_space(io_uba, local_data, 4);
	read_data->err = ncs_decode_32bit(&pstream);
	ncs_dec_skip_space(io_uba, 4);

	return NCSCC_RC_SUCCESS;

}

/****************************************************************************\
 PROCEDURE NAME : cpsv_data_access_rsp_decode

 DESCRIPTION    : This routine will decode the contents of CPSV_EVT into user buf

 ARGUMENTS      :  CPSV_ND2A_DATA_ACCESS_RSP *data_rsp .
                  *io_uba  - User Buff.
				  MDS_CLIENT_MSG_FORMAT_VER.

 RETURNS        : None

 NOTES          : 
\*****************************************************************************/
uint32_t cpsv_data_access_rsp_decode(CPSV_ND2A_DATA_ACCESS_RSP *data_rsp, NCS_UBAID *io_uba , MDS_CLIENT_MSG_FORMAT_VER i_msg_fmt_ver)
{
  
   uint8_t local_data[1024];
   uint8_t *pstream;
   uint32_t i,size , rc =NCSCC_RC_SUCCESS;
   uint32_t ver_compare = 4; /* CPND_MDS_PVT_SUBPART_VERSION  and CPA_MDS_PVT_SUBPART_VERSION*/  
   
   if ( i_msg_fmt_ver >= ver_compare)
	   size = 4 + 4 + 4 + 4 + 8 + 4 + 8 + 8;
   else
	   size = 4 + 4 + 4 + 4 + 8 + 4 + 8;
   pstream = ncs_dec_flatten_space(io_uba, local_data , size);
   data_rsp->type      =     ncs_decode_32bit(&pstream);
   data_rsp->num_of_elmts =  ncs_decode_32bit(&pstream);
   data_rsp->error     =     ncs_decode_32bit(&pstream);
   data_rsp->size      =     ncs_decode_32bit(&pstream);
   data_rsp->ckpt_id   =     ncs_decode_64bit(&pstream);
   data_rsp->error_index      =     ncs_decode_32bit(&pstream);
   data_rsp->from_svc  =     ncs_decode_64bit(&pstream);
   if ( i_msg_fmt_ver >= ver_compare)  
	   data_rsp->lcl_ckpt_id   = ncs_decode_64bit(&pstream);
   ncs_dec_skip_space(io_uba, size);
   if(data_rsp->type == CPSV_DATA_ACCESS_WRITE_RSP)
   {
       size = data_rsp->size * sizeof(SaUint32T);
       SaUint32T* write_err_index =NULL;
        /* Allocate Memory for data_rsp->info.write_err_index */
       if(size)
       {
          data_rsp->info.write_err_index = m_MMGR_ALLOC_CPSV_SaUint32T(data_rsp->size, NCS_SERVICE_ID_CPA);
          if(!data_rsp->info.write_err_index)
          {
              return NCSCC_RC_FAILURE;
	      }
           write_err_index = data_rsp->info.write_err_index;
           memset(write_err_index , 0 , size);
           pstream = ncs_dec_flatten_space(io_uba, local_data , size);
           for(i = 0 ; i< data_rsp->size ; i++)
           {
               /* Encode Write Error Index */
               write_err_index[i] = ncs_decode_32bit(&pstream);
           }
           ncs_dec_skip_space(io_uba , size);
         }

	} else if ((data_rsp->type == CPSV_DATA_ACCESS_LCL_READ_RSP)
		   || (data_rsp->type == CPSV_DATA_ACCESS_RMT_READ_RSP)) {
		if (data_rsp->size) {
			data_rsp->info.read_data = m_MMGR_ALLOC_CPSV_ND2A_READ_DATA(data_rsp->size, NCS_SERVICE_ID_CPA);
			if (!data_rsp->info.read_data) {
				return NCSCC_RC_FAILURE;

			}
			memset(data_rsp->info.read_data, 0, data_rsp->size * sizeof(CPSV_ND2A_READ_DATA));

			for (i = 0; i < data_rsp->size; i++)
				rc = cpsv_nd2a_read_data_decode(&data_rsp->info.read_data[i], io_uba);
		}
	} else if (data_rsp->type == CPSV_DATA_ACCESS_OVWRITE_RSP) {
		pstream = ncs_dec_flatten_space(io_uba, local_data, 4);
		data_rsp->info.ovwrite_error.error = ncs_decode_32bit(&pstream);
		ncs_dec_skip_space(io_uba, 4);
	} else {
		rc = NCSCC_RC_FAILURE;
	}

	return rc;

}

/****************************************************************************\
 PROCEDURE NAME : cpsv_evt_dec_flat

 DESCRIPTION    : This routine will decode the contents of CPSV_EVT into user buf

 ARGUMENTS      : *i_evt - Event Struct.
                  *o_ub  - User Buff.

 RETURNS        : None

 NOTES          : 
\*****************************************************************************/
uint32_t cpsv_evt_dec_flat(EDU_HDL *edu_hdl, NCS_UBAID *i_ub, CPSV_EVT *o_evt)
{
	uint32_t size = 0;
	uint32_t rc = NCSCC_RC_SUCCESS;
	size = sizeof(CPSV_EVT);

	/* Decode the Top level evt envolop */
	ncs_decode_n_octets_from_uba(i_ub, (uint8_t *)o_evt, size);

	/* Decode the internal Pointers */
	if (o_evt->type == CPSV_EVT_TYPE_CPA) {
		switch (o_evt->info.cpa.type) {
		case CPA_EVT_ND2A_CKPT_DATA_RSP:
			{
				CPSV_ND2A_DATA_ACCESS_RSP *data_rsp = &o_evt->info.cpa.info.sec_data_rsp;
				if ((data_rsp->type == CPSV_DATA_ACCESS_LCL_READ_RSP)
				    || (data_rsp->type == CPSV_DATA_ACCESS_RMT_READ_RSP)) {
					if (data_rsp->num_of_elmts == -1) {
						data_rsp->info.read_data = NULL;
					} else {
						data_rsp->info.read_data =
						    m_MMGR_ALLOC_CPSV_ND2A_READ_DATA(data_rsp->num_of_elmts,
										     NCS_SERVICE_ID_CPA);
						if (data_rsp->info.read_data) {
							uint32_t iter = 0;
							for (; iter < data_rsp->size; iter++) {
								size = sizeof(CPSV_ND2A_READ_DATA);
								ncs_decode_n_octets_from_uba(i_ub,
											     (uint8_t *)&data_rsp->
											     info.read_data[iter],
											     size);
								if (data_rsp->info.read_data[iter].read_size > 0) {
									size = data_rsp->info.read_data[iter].read_size;
									data_rsp->info.read_data[iter].data =
									    m_MMGR_ALLOC_CPSV_DEFAULT_VAL(size,
													  NCS_SERVICE_ID_CPA);
									if (data_rsp->info.read_data[iter].data) {
										ncs_decode_n_octets_from_uba(i_ub,
													     data_rsp->info.read_data
													     [iter].data,
													     size);
									}
								}
							}
						}
					}
				} else if (data_rsp->type == CPSV_DATA_ACCESS_WRITE_RSP) {
					if (data_rsp->num_of_elmts == -1) {
						data_rsp->info.write_err_index = NULL;
					} else {
						size = data_rsp->num_of_elmts * sizeof(SaUint32T);
						data_rsp->info.write_err_index =
						    m_MMGR_ALLOC_CPSV_SaUint32T(data_rsp->num_of_elmts,
										NCS_SERVICE_ID_CPA);
						if (data_rsp->info.write_err_index)
							ncs_decode_n_octets_from_uba(i_ub,
										     (uint8_t *)data_rsp->
										     info.write_err_index, size);
					}
				}
				break;
			}
		case CPA_EVT_ND2A_CKPT_CLM_NODE_LEFT:
			{
				/* do nothing */

				break;
			}
		case CPA_EVT_ND2A_CKPT_CLM_NODE_JOINED:
			{
				/*do nothing */

				break;
			}
		case CPA_EVT_ND2A_CKPT_ARRIVAL_NTFY:
			{
				CPSV_CKPT_DATA *data;
				EDU_ERR ederror = 0;

				data = NULL;	/* Explicity set it to NULL */

				rc = m_NCS_EDU_EXEC(edu_hdl, FUNC_NAME(CPSV_CKPT_DATA),
						    i_ub, EDP_OP_TYPE_DEC, &data, &ederror);
				if (rc != NCSCC_RC_SUCCESS) {
					return rc;
				}
				o_evt->info.cpa.info.arr_msg.ckpt_data = data;
				break;
			}

		case CPA_EVT_ND2A_SEC_CREATE_RSP:
			{
				SaCkptSectionIdT *sec_id = &o_evt->info.cpa.info.sec_creat_rsp.sec_id;
				if (sec_id) {
					size = sec_id->idLen;
					if (size) {
						sec_id->id =
						    m_MMGR_ALLOC_CPSV_DEFAULT_VAL(size + 1, NCS_SERVICE_ID_CPND);
						if (sec_id->id) {
							ncs_decode_n_octets_from_uba(i_ub, (uint8_t *)sec_id->id, size);
							sec_id->id[size] = 0;
						}
					}
				}
				break;
			}
		case CPA_EVT_ND2A_SEC_ITER_GETNEXT_RSP:
			{
				SaCkptSectionIdT *sec_id = &o_evt->info.cpa.info.iter_next_rsp.sect_desc.sectionId;
				if (sec_id) {
					size = sec_id->idLen;
					if (size) {
						sec_id->id =
						    m_MMGR_ALLOC_CPSV_DEFAULT_VAL(size + 1, NCS_SERVICE_ID_CPND);
						if (sec_id->id) {
							ncs_decode_n_octets_from_uba(i_ub, (uint8_t *)sec_id->id, size);
							sec_id->id[size] = 0;
						}
					}
				}
				break;
			}
		default:
			break;
		}
	} else if (o_evt->type == CPSV_EVT_TYPE_CPND) {
		switch (o_evt->info.cpnd.type) {
		case CPND_EVT_A2ND_CKPT_WRITE:
			cpsv_ckpt_data_decode(&o_evt->info.cpnd.info.ckpt_write.data, i_ub);

			break;

		case CPND_EVT_A2ND_CKPT_READ:
			cpsv_ckpt_data_decode(&o_evt->info.cpnd.info.ckpt_write.data, i_ub);
			break;
			/* Added for 3.0.B , these events decoding is missing in 3.0.2 */
		case CPND_EVT_ND2ND_CKPT_ACTIVE_SYNC:

			cpsv_ckpt_data_decode(&o_evt->info.cpnd.info.ckpt_nd2nd_sync.data, i_ub);
			break;

		case CPSV_EVT_ND2ND_CKPT_SECT_ACTIVE_DATA_ACCESS_REQ:

			cpsv_ckpt_data_decode(&o_evt->info.cpnd.info.ckpt_nd2nd_data.data, i_ub);
			break;

		case CPSV_EVT_ND2ND_CKPT_SECT_ACTIVE_DATA_ACCESS_RSP:
			{
				CPSV_ND2A_DATA_ACCESS_RSP *data_rsp = &o_evt->info.cpnd.info.ckpt_nd2nd_data_rsp;

				if (data_rsp->type == CPSV_DATA_ACCESS_WRITE_RSP) {
					if (data_rsp->num_of_elmts == -1) {
						data_rsp->info.write_err_index = NULL;
					} else {
						size = data_rsp->num_of_elmts * sizeof(SaUint32T);
						if (size) {
							data_rsp->info.write_err_index =
							    m_MMGR_ALLOC_CPSV_SaUint32T(data_rsp->num_of_elmts,
											NCS_SERVICE_ID_CPA);
							if (data_rsp->info.write_err_index)
								ncs_decode_n_octets_from_uba(i_ub,
											     (uint8_t *)data_rsp->
											     info.write_err_index,
											     size);
						} else
							data_rsp->info.write_err_index = NULL;
					}
				}
				break;
			}
		case CPSV_EVT_ND2ND_CKPT_SECT_CREATE_REQ:
			{
				CPSV_CKPT_SECT_CREATE *create = &o_evt->info.cpnd.info.active_sec_creat;

				if (create->sec_attri.sectionId) {
					create->sec_attri.sectionId = cpsv_evt_dec_sec_id(i_ub, NCS_SERVICE_ID_CPND);
				}
				if (create->sec_attri.sectionId == NULL) {
					return NCSCC_RC_FAILURE;
				}

				if (create->init_size) {
					create->init_data =
					    (void *)m_MMGR_ALLOC_CPSV_DEFAULT_VAL(create->init_size,
										  NCS_SERVICE_ID_CPND);
					ncs_decode_n_octets_from_uba(i_ub, (uint8_t *)create->init_data,
								     create->init_size);
				} else
					create->init_data = NULL;
				break;
			}
		case CPSV_EVT_ND2ND_CKPT_SECT_ACTIVE_CREATE_RSP:
			{
				SaCkptSectionIdT *sec_id = &o_evt->info.cpnd.info.active_sec_creat_rsp.sec_id;
				if (sec_id) {
					size = sec_id->idLen;
					if (size) {
						sec_id->id =
						    m_MMGR_ALLOC_CPSV_DEFAULT_VAL(size + 1, NCS_SERVICE_ID_CPND);
						if (sec_id->id) {
							ncs_decode_n_octets_from_uba(i_ub, (uint8_t *)sec_id->id, size);
							sec_id->id[size] = 0;
						}
					} else
						sec_id->id = NULL;
				}
				break;
			}
		case CPSV_EVT_ND2ND_CKPT_SECT_DELETE_REQ:
			{
				SaCkptSectionIdT *sec_id = &o_evt->info.cpnd.info.sec_delete_req.sec_id;
				if (sec_id) {
					size = sec_id->idLen;
					if (size) {
						sec_id->id =
						    m_MMGR_ALLOC_CPSV_DEFAULT_VAL(size + 1, NCS_SERVICE_ID_CPND);
						if (sec_id->id) {
							ncs_decode_n_octets_from_uba(i_ub, (uint8_t *)sec_id->id, size);
							sec_id->id[size] = 0;
						}
					} else
						sec_id->id = NULL;
				}
				break;
			}
			/* End of - Added for 3.0.B , these events decoding is missing in 3.0.2 */
		case CPND_EVT_A2ND_CKPT_SECT_CREATE:
			{
				uint32_t size;
				CPSV_CKPT_SECT_CREATE *create = &o_evt->info.cpnd.info.sec_creatReq;

				if (create->sec_attri.sectionId) {
					create->sec_attri.sectionId = cpsv_evt_dec_sec_id(i_ub, NCS_SERVICE_ID_CPND);
				}

				size = create->init_size;
				if (size != 0) {
					create->init_data =
					    (void *)m_MMGR_ALLOC_CPSV_DEFAULT_VAL(size, NCS_SERVICE_ID_CPND);
					ncs_decode_n_octets_from_uba(i_ub, (uint8_t *)create->init_data, size);
				}
				break;
			}
		case CPND_EVT_A2ND_CKPT_ITER_GETNEXT:
			{
				uint32_t size = 0;
				SaCkptSectionIdT *sec_id = &o_evt->info.cpnd.info.iter_getnext.section_id;
				if (sec_id) {
					size = sec_id->idLen;
					if (size) {
						sec_id->id =
						    m_MMGR_ALLOC_CPSV_DEFAULT_VAL(size + 1, NCS_SERVICE_ID_CPND);
						if (sec_id->id) {
							ncs_decode_n_octets_from_uba(i_ub, (uint8_t *)sec_id->id, size);
							sec_id->id[size] = 0;
					}
					} else
						sec_id->id = NULL;
				}
				break;
			}

		case CPND_EVT_A2ND_CKPT_SECT_DELETE:

			if (o_evt->info.cpnd.info.sec_delReq.sec_id.idLen) {
				SaCkptSectionIdT *sec_id = &o_evt->info.cpnd.info.sec_delReq.sec_id;
				uint32_t size = o_evt->info.cpnd.info.sec_delReq.sec_id.idLen;
				if (size) {
					sec_id->id = m_MMGR_ALLOC_CPSV_DEFAULT_VAL(size + 1, NCS_SERVICE_ID_CPND);
					if (sec_id->id) {
					ncs_decode_n_octets_from_uba(i_ub, (uint8_t *)sec_id->id, size);
						sec_id->id[size] = 0;
					}
				} else
					sec_id->id = NULL;
			}
			break;

		case CPND_EVT_A2ND_CKPT_SECT_EXP_SET:

			if (o_evt->info.cpnd.info.sec_expset.sec_id.idLen) {
				SaCkptSectionIdT *sec_id = &o_evt->info.cpnd.info.sec_expset.sec_id;
				uint32_t size = o_evt->info.cpnd.info.sec_expset.sec_id.idLen;
				if (size) {
					sec_id->id = m_MMGR_ALLOC_CPSV_DEFAULT_VAL(size + 1, NCS_SERVICE_ID_CPND);
					if (sec_id->id) {
					ncs_decode_n_octets_from_uba(i_ub, (uint8_t *)sec_id->id, size);
						sec_id->id[size] = 0;
					}
				} else
					sec_id->id = NULL;
			}
			break;

		case CPSV_EVT_ND2ND_CKPT_SECT_EXPTMR_REQ:

			if (o_evt->info.cpnd.info.sec_exp_set.sec_id.idLen) {
				SaCkptSectionIdT *sec_id = &o_evt->info.cpnd.info.sec_exp_set.sec_id;
				uint32_t size = o_evt->info.cpnd.info.sec_exp_set.sec_id.idLen;
				if (size) {
					sec_id->id = m_MMGR_ALLOC_CPSV_DEFAULT_VAL(size + 1, NCS_SERVICE_ID_CPND);
					if (sec_id->id) {
						ncs_decode_n_octets_from_uba(i_ub, (uint8_t *)sec_id->id, size);
						sec_id->id[size] = 0;
					}
				} else
					sec_id->id = NULL;

			}
			break;

		case CPND_EVT_D2ND_CKPT_INFO:

			if (o_evt->info.cpnd.info.ckpt_info.dest_cnt) {
				CPSV_CPND_DEST_INFO *dest_list = 0;
				uint32_t size = sizeof(CPSV_CPND_DEST_INFO) * o_evt->info.cpnd.info.ckpt_info.dest_cnt;
				if (size)
					dest_list = m_MMGR_ALLOC_CPSV_SYS_MEMORY(size);
				if (dest_list && size)
					ncs_decode_n_octets_from_uba(i_ub, (uint8_t *)dest_list, size);
				o_evt->info.cpnd.info.ckpt_info.dest_list = dest_list;
			}
			break;

		case CPND_EVT_D2ND_CKPT_CREATE:

			if (o_evt->info.cpnd.info.ckpt_create.ckpt_info.dest_cnt) {
				CPSV_CPND_DEST_INFO *dest_list = 0;
				uint32_t size = sizeof(CPSV_CPND_DEST_INFO) *
				    o_evt->info.cpnd.info.ckpt_create.ckpt_info.dest_cnt;
				if (size)
					dest_list = m_MMGR_ALLOC_CPSV_SYS_MEMORY(size);
				if (dest_list && size)
					ncs_decode_n_octets_from_uba(i_ub, (uint8_t *)dest_list, size);
				o_evt->info.cpnd.info.ckpt_create.ckpt_info.dest_list = dest_list;
			}
			break;

		case CPSV_D2ND_RESTART_DONE:

			if (o_evt->info.cpnd.info.cpnd_restart_done.dest_cnt) {
				CPSV_CPND_DEST_INFO *dest_list = 0;
				uint32_t size = sizeof(CPSV_CPND_DEST_INFO) *
				    o_evt->info.cpnd.info.cpnd_restart_done.dest_cnt;
				if (size)
					dest_list = m_MMGR_ALLOC_CPSV_SYS_MEMORY(size);
				if (dest_list && size)
					ncs_decode_n_octets_from_uba(i_ub, (uint8_t *)dest_list, size);
				o_evt->info.cpnd.info.cpnd_restart_done.dest_list = dest_list;
			}
			break;

		case CPND_EVT_D2ND_CKPT_REP_ADD:

			if (o_evt->info.cpnd.info.ckpt_add.dest_cnt) {
				CPSV_CPND_DEST_INFO *dest_list = 0;
				uint32_t size = sizeof(CPSV_CPND_DEST_INFO) * o_evt->info.cpnd.info.ckpt_add.dest_cnt;
				if (size)
					dest_list = m_MMGR_ALLOC_CPSV_SYS_MEMORY(size);
				if (dest_list && size)
					ncs_decode_n_octets_from_uba(i_ub, (uint8_t *)dest_list, size);
				o_evt->info.cpnd.info.ckpt_add.dest_list = dest_list;
			}
			break;
      case CPND_EVT_A2ND_CKPT_REFCNTSET:
         if(o_evt->info.cpnd.info.refCntsetReq.no_of_nodes)
           cpsv_refcnt_ckptid_decode(&o_evt->info.cpnd.info.refCntsetReq,i_ub ); 
     break;
		default:
			break;
		}
	}

	return NCSCC_RC_SUCCESS;
}

/*****************************************************************************

  PROCEDURE NAME:    cpsv_dbg_sink

  DESCRIPTION:

   cpsv is instrumented to fall into this debug sink function
   if it hits any if-conditional that fails, but should not fail. This
   is the default implementation of that SINK macro.

  ARGUMENTS:

  uint32_t   l             line # in file
  char*   f             file name where macro invoked
  code    code          Error code value.. Usually FAILURE

  RETURNS:

  code    - just echo'ed back

*****************************************************************************/

#if ((NCS_CPND == 1) || (NCS_CPD == 1))

uint32_t cpsv_dbg_sink(uint32_t l, char *f, uint32_t code, char *str)
{
	TRACE("In file %s at line %d ", f, l);

	if (NULL != str)
		TRACE("Reason : %s ", str);

	return code;
}

#endif

/*****************************************************************************

  PROCEDURE NAME:    cpsv_evt_trace

  DESCRIPTION: This function log the event message.

  ARGUMENTS:
  char *svc_name : service name
  CPSV_EVT_REQUEST request : event request (send, receive, or broadcast)
  CPSV_EVT *evt : pointer to event content
  MDS_DEST mds_dest : MDS destination

  RETURNS:

*****************************************************************************/
void cpsv_evt_trace(char *svc_name, CPSV_EVT_REQUEST request, CPSV_EVT *evt, MDS_DEST mds_dest)
{
	char evt_str[MAX_EVT_STR_LEN] = {0};
	int node_id = m_NCS_NODE_ID_FROM_MDS_DEST(mds_dest);

	cpsv_evt_str(evt, evt_str, MAX_EVT_STR_LEN);

	if ((evt->type == CPSV_EVT_TYPE_CPA && evt->info.cpa.type == CPA_EVT_MDS_INFO) ||
		(evt->type == CPSV_EVT_TYPE_CPND && evt->info.cpnd.type == CPND_EVT_MDS_INFO) || 
		(evt->type == CPSV_EVT_TYPE_CPD && evt->info.cpd.type == CPD_EVT_MDS_INFO)) {
		switch (request)
		{
		case CPSV_EVT_SEND:
			TRACE("%s ==>> %s", svc_name, evt_str);
			break;
		case CPSV_EVT_RECEIVE:
			TRACE("%s <<== %s", svc_name, evt_str);
			break;
		default:
			TRACE("Invalid CPSV_EVT_REQUEST");
			break;
		}
	} else {
		switch (request)
		{
		case CPSV_EVT_SEND:
			if (node_id != 0)
				TRACE("%s ==>> %s to node 0x%X", svc_name, evt_str, node_id);
			else
				TRACE("%s ==>> %s to CPD", svc_name, evt_str);
			break;
		case CPSV_EVT_RECEIVE:
			if (node_id != 0)
				TRACE("%s <<== %s from node 0x%X", svc_name, evt_str, node_id);
			else
				TRACE("%s <<== %s from CPD", svc_name, evt_str);
			break;
		case CPSV_EVT_BROADCAST:
			TRACE("%s ==>> %s (broadcast)", svc_name, evt_str);
			break;
		default:
			TRACE("Invalid CPSV_EVT_REQUEST");
			break;
		}

	}
}

/*****************************************************************************

  PROCEDURE NAME:    cpsv_convert_sec_id_to_string

  DESCRIPTION: This function convert section_id to string

  ARGUMENTS:
  char *sec_id_str : output section id string
  SaCkptSectionIdT *section_id : pointer to section id

  RETURNS:

*****************************************************************************/
void cpsv_convert_sec_id_to_string(char *sec_id_str, SaCkptSectionIdT *section_id)
{
	int16_t remaining_bytes = MAX_SEC_ID_LEN - 1;

	if (section_id != NULL && section_id->id != NULL && section_id->idLen != 0) {
		strncpy(sec_id_str, "0x", remaining_bytes);
		remaining_bytes -= 2;

		for(int i = 0; i < section_id->idLen; i++) {
			char element_id[3] = {0};
			sprintf(element_id, "%02X", *(section_id->id + i));
			strncat(sec_id_str, element_id, remaining_bytes);
			remaining_bytes -= 2;

			if (remaining_bytes < 2)
				break;
		}
	} else {
		strncpy(sec_id_str, "(NULL)", MAX_SEC_ID_LEN);
	}
}
