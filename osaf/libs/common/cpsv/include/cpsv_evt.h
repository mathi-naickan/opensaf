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

  This file contains the Enum and datastructure definations for events 
  exchanged between CPSv components 
******************************************************************************/

#ifndef CPSV_EVT_H
#define CPSV_EVT_H

#include <saCkpt.h>
#include "ncssysf_tmr.h"

/*****************************************************************************
 * Event Type of CPSV
 *****************************************************************************/
typedef enum cpsv_evt_type {
	CPSV_EVT_BASE = 1,
	CPSV_EVT_TYPE_CPA = CPSV_EVT_BASE,
	CPSV_EVT_TYPE_CPND,
	CPSV_EVT_TYPE_CPD,
	CPSV_EVT_TYPE_MAX
} CPSV_EVT_TYPE;

/*****************************************************************************
 * Event Type of CPA 
 *****************************************************************************/
typedef enum cpa_evt_type {
	CPA_EVT_BASE = 1,

	/* Locally generated events */
	CPA_EVT_MDS_INFO = CPA_EVT_BASE,	/* CPND UP/DOWN Info */
	CPA_EVT_TIME_OUT,	/* Time out events at CPA */

	/* Events from CPND */

	CPA_EVT_ND2A_CKPT_INIT_RSP,
	CPA_EVT_ND2A_CKPT_FINALIZE_RSP,
	CPA_EVT_ND2A_CKPT_OPEN_RSP,
	CPA_EVT_ND2A_CKPT_CLOSE_RSP,
	CPA_EVT_ND2A_CKPT_UNLINK_RSP,
	CPA_EVT_ND2A_CKPT_RDSET_RSP,
	CPA_EVT_ND2A_CKPT_AREP_SET_RSP,
	CPA_EVT_ND2A_CKPT_STATUS,

	CPA_EVT_ND2A_SEC_CREATE_RSP,
	CPA_EVT_ND2A_SEC_DELETE_RSP,
	CPA_EVT_ND2A_SEC_EXPTIME_RSP,
	CPA_EVT_ND2A_SEC_ITER_RSP,
	CPA_EVT_ND2A_SEC_ITER_GETNEXT_RSP,

	CPA_EVT_ND2A_CKPT_ARRIVAL_NTFY,

	CPA_EVT_ND2A_CKPT_DATA_RSP,

	CPA_EVT_ND2A_CKPT_SYNC_RSP,
	CPA_EVT_D2A_ACT_CKPT_INFO_BCAST_SEND,
	CPA_EVT_ND2A_CKPT_READ_ACK_RSP,
	CPA_EVT_ND2A_CKPT_BCAST_SEND,
	CPA_EVT_D2A_NDRESTART,
	CPA_EVT_CB_DUMP,
	CPA_EVT_ND2A_CKPT_CLM_NODE_LEFT,
	CPA_EVT_ND2A_CKPT_CLM_NODE_JOINED,
	CPA_EVT_ND2A_ACT_CKPT_INFO_BCAST_SEND,
	CPA_EVT_MAX
} CPA_EVT_TYPE;

/*****************************************************************************
 * Event Type of CPND 
 *****************************************************************************/
typedef enum cpnd_evt_type {
	/* events from CPA to CPND */
	CPND_EVT_BASE = 1,

	/* Locally generated events */
	CPND_EVT_MDS_INFO = CPND_EVT_BASE,	/* CPA/CPND/CPD UP/DOWN Info */
	CPND_EVT_TIME_OUT,	/* Time out event */

	/* Events from CPA */

	CPND_EVT_A2ND_CKPT_INIT,	/* Checkpoint Initialization */
	CPND_EVT_A2ND_CKPT_FINALIZE,	/* Checkpoint finalization */
	CPND_EVT_A2ND_CKPT_OPEN,	/* Checkpoint Open Request */
	CPND_EVT_A2ND_CKPT_CLOSE,	/* Checkpoint Close Call */
	CPND_EVT_A2ND_CKPT_UNLINK,	/* Checkpoint Unlink Call */
	CPND_EVT_A2ND_CKPT_RDSET,	/* Checkpoint Retention duration set call */
	CPND_EVT_A2ND_CKPT_AREP_SET,	/* Checkpoint Active Replica Set Call */
	CPND_EVT_A2ND_CKPT_STATUS_GET,	/* Checkpoint Status Get Call */

	CPND_EVT_A2ND_CKPT_SECT_CREATE,	/* Checkpoint Section Create Call */
	CPND_EVT_A2ND_CKPT_SECT_DELETE,	/* Checkpoint Section Delete Call */
	CPND_EVT_A2ND_CKPT_SECT_EXP_SET,	/* Checkpoint Section Expiry Time Set Call */
	CPND_EVT_A2ND_CKPT_SECT_ITER_REQ,	/*Checkpoint Section iteration initialize */

	CPND_EVT_A2ND_CKPT_ITER_GETNEXT,	/* Checkpoint Section Iternation Getnext Call */

	CPND_EVT_A2ND_ARRIVAL_CB_REG,	/* Checkpoint Arrival Callback */

	CPND_EVT_A2ND_CKPT_WRITE,	/* Checkpoint Write And overwrite call */
	CPND_EVT_A2ND_CKPT_READ,	/* Checkpoint Read Call  */
	CPND_EVT_A2ND_CKPT_SYNC,	/* Checkpoint Synchronize call */

	CPND_EVT_A2ND_CKPT_READ_ACK,	/* read ack */

	/* Events from other CPND */

/* ckpt status information from active */

	CPND_EVT_ND2ND_ACTIVE_STATUS,	/* ckpt status info from active */
	CPND_EVT_ND2ND_ACTIVE_STATUS_ACK,	/* ckpt status ack from active */

	CPND_EVT_ND2ND_CKPT_SYNC_REQ,	/* rqst from ND to ND(A) to sync ckpt */
	CPND_EVT_ND2ND_CKPT_ACTIVE_SYNC,	/* CPND(A) sync updts to All the Ckpts */
/* Section Create Stuff.... */

	CPSV_EVT_ND2ND_CKPT_SECT_CREATE_REQ,
	CPSV_EVT_ND2ND_CKPT_SECT_CREATE_RSP,
	CPSV_EVT_ND2ND_CKPT_SECT_ACTIVE_CREATE_RSP,

	CPSV_EVT_ND2ND_CKPT_SECT_DELETE_REQ,
	CPSV_EVT_ND2ND_CKPT_SECT_DELETE_RSP,

	CPSV_EVT_ND2ND_CKPT_SECT_EXPTMR_REQ,
	CPSV_EVT_ND2ND_CKPT_SECT_EXPTMR_RSP,

	CPSV_EVT_ND2ND_CKPT_SECT_DATA_ACCESS_REQ,	/* for write,read,overwrite */
	CPSV_EVT_ND2ND_CKPT_SECT_ACTIVE_DATA_ACCESS_REQ,
	CPSV_EVT_ND2ND_CKPT_SECT_ACTIVE_DATA_ACCESS_RSP,

	/* Events from CPD to CPND */

	CPND_EVT_D2ND_CKPT_INFO,	/* Rsp to the ckpt open call */
	CPND_EVT_D2ND_CKPT_SIZE,
	CPND_EVT_D2ND_CKPT_REP_ADD,	/* ckpt open is propogated to other NDs */
	CPND_EVT_D2ND_CKPT_REP_DEL,	/* ckpt close is propogated to other NDs */

	CPSV_D2ND_RESTART,	/* for cpnd redundancy */
	CPSV_D2ND_RESTART_DONE,	/* for cpnd redundancy */

	CPND_EVT_D2ND_CKPT_CREATE,	/* ckpt create evt for Non-collocated */
	CPND_EVT_D2ND_CKPT_DESTROY,	/* The ckpt destroy evt for Non-colloc */
	CPND_EVT_D2ND_CKPT_DESTROY_ACK,
	CPND_EVT_D2ND_CKPT_CLOSE_ACK,	/* Rsps to ckpt close call */
	CPND_EVT_D2ND_CKPT_UNLINK,	/* Unlink info */
	CPND_EVT_D2ND_CKPT_UNLINK_ACK,	/* Rsps to ckpt unlink call */
	CPND_EVT_D2ND_CKPT_RDSET,	/* Retention duration to set */
	CPND_EVT_D2ND_CKPT_RDSET_ACK,	/* Retention duration Ack */
	CPND_EVT_D2ND_CKPT_ACTIVE_SET,	/* for colloc ckpts,mark the Active */
	CPND_EVT_D2ND_CKPT_ACTIVE_SET_ACK,	/* Ack for active replica set rqst */

	CPND_EVT_ND2ND_CKPT_ITER_NEXT_REQ,
	CPND_EVT_ND2ND_CKPT_ACTIVE_ITERNEXT,

	CPND_EVT_CB_DUMP,

   CPND_EVT_D2ND_CKPT_NUM_SECTIONS,
   CPND_EVT_A2ND_CKPT_REFCNTSET,        /* ref cont opener's set call */
   CPND_EVT_A2ND_CKPT_LIST_UPDATE,	/* Checkpoint ckpt list update Call */
   CPND_EVT_MAX

}CPND_EVT_TYPE;

/*****************************************************************************
 * Event Types of CPD 
 *****************************************************************************/
typedef enum cpd_evt_type {
	CPD_EVT_BASE = 1,

	/* Locally generated Events */
	CPD_EVT_MDS_INFO = CPD_EVT_BASE,

	/* Events from CPND */
	CPD_EVT_ND2D_CKPT_CREATE,
	CPD_EVT_ND2D_CKPT_UNLINK,
	CPD_EVT_ND2D_CKPT_RDSET,
	CPD_EVT_ND2D_ACTIVE_SET,
	CPD_EVT_ND2D_CKPT_CLOSE,
	CPD_EVT_ND2D_CKPT_DESTROY,
	CPD_EVT_ND2D_CKPT_USR_INFO,
	CPD_EVT_ND2D_CKPT_SYNC_INFO,
	CPD_EVT_ND2D_CKPT_SEC_INFO_UPD,
	CPD_EVT_ND2D_CKPT_MEM_USED,
	CPD_EVT_CB_DUMP,
	CPD_EVT_MDS_QUIESCED_ACK_RSP,
	CPD_EVT_ND2D_CKPT_DESTROY_BYNAME,
	CPD_EVT_ND2D_CKPT_CREATED_SECTIONS,
	CPD_EVT_TIME_OUT,
	CPD_EVT_MAX
} CPD_EVT_TYPE;

/*****************************************************************************
 Common Structs used in events 
 *****************************************************************************/

/* Struct used for passing only error in sync resp */
typedef struct cpsv_saerr_info {
	SaAisErrorT error;
} CPSV_SAERR_INFO;

/* Struct used to store MDS dest of replicas of a perticula ckpt*/
typedef struct cpsv_cpnd_dest_info {
	MDS_DEST dest;
	struct cpsv_cpnd_dest_info *next;
} CPSV_CPND_DEST_INFO;

/* Struct used to store MDS dest and flag to keep track of responsed dests*/
typedef struct cpsv_cpnd_write_dest {
	MDS_DEST dest;
	bool write_rsp_flag;
	struct cpsv_cpnd_write_dest *next;
} CPSV_CPND_UPDATE_DEST;

/* Structure for passing MDS info to components */
typedef struct cpsv_mds_info {
	NCSMDS_CHG change;	/* GONE, UP, DOWN, CHG ROLE  */
	MDS_DEST dest;
	MDS_SVC_ID svc_id;
	NODE_ID node_id;
	V_DEST_RL role;
} CPSV_MDS_INFO;

/* Struct used for convaying MDS dest info of a ckpt */
typedef struct cpsv_ckpt_dest_info {
	SaCkptCheckpointHandleT ckpt_id;
	MDS_DEST mds_dest;
} CPSV_CKPT_DEST_INFO;

typedef struct cpsv_send_info {
	MDS_SVC_ID to_svc;	/* The service at the destination */
	MDS_DEST dest;		/* Who to send */
	MDS_SENDTYPES stype;	/* Send type */
	MDS_SYNC_SND_CTXT ctxt;	/* MDS Opaque context */
} CPSV_SEND_INFO;

typedef enum cpndq_tmr_type {
	CPND_TMR_TYPE_RETENTION = 1,
	CPND_TMR_TYPE_SEC_EXPI,
	CPND_ALL_REPL_RSP_EXPI,
	CPND_TMR_OPEN_ACTIVE_SYNC,
	CPND_TMR_TYPE_NON_COLLOC_RETENTION,
	CPND_TMR_TYPE_MAX = CPND_TMR_TYPE_NON_COLLOC_RETENTION,
} CPND_TMR_TYPE;
typedef struct cpnd_tmr {
	CPND_TMR_TYPE type;
	tmr_t tmr_id;
	SaCkptCheckpointHandleT ckpt_id;
	MDS_DEST agent_dest;
	uint32_t lcl_sec_id;
	uint32_t uarg;
	bool is_active;
	SaUint32T write_type;
	CPSV_SEND_INFO sinfo;
	SaInvocationT invocation;
	SaCkptCheckpointHandleT lcl_ckpt_hdl;
	bool is_active_sync_err;
} CPND_TMR;
/* Struct used for convaying MDS active dest and other dest list info of a ckpt 
   This is used in CPND_EVT_D2ND_CKPT_REP_ADD & CPSV_D2ND_RESTART_DONE */
typedef struct cpsv_ckpt_destlist_info {
	SaCkptCheckpointHandleT ckpt_id;
	MDS_DEST mds_dest;
	MDS_DEST active_dest;
	SaCkptCheckpointCreationAttributesT attributes;
	SaCkptCheckpointOpenFlagsT ckpt_flags;
	bool is_cpnd_restart;
	uint32_t dest_cnt;
	CPSV_CPND_DEST_INFO *dest_list;

} CPSV_CKPT_DESTLIST_INFO;

/****************************************************************************
 Requests CPA --> CPND 
 ****************************************************************************/
typedef struct cpsv_init_req {
	SaVersionT version;
} CPSV_INIT_REQ;

typedef struct cpsv_finalize_req {
	SaCkptHandleT client_hdl;
} CPSV_FINALIZE_REQ;

typedef struct cpsv_a2nd_open_req {
	SaCkptHandleT client_hdl;
	SaCkptCheckpointHandleT lcl_ckpt_hdl;
	SaNameT ckpt_name;
	SaCkptCheckpointCreationAttributesT ckpt_attrib;
	SaCkptCheckpointOpenFlagsT ckpt_flags;
	SaInvocationT invocation;
	SaTimeT timeout;
} CPSV_A2ND_OPEN_REQ;

typedef struct cpsv_a2nd_ckpt_close {
	SaCkptHandleT client_hdl;
	SaCkptCheckpointHandleT ckpt_id;
	SaCkptCheckpointOpenFlagsT ckpt_flags;
} CPSV_A2ND_CKPT_CLOSE;

typedef struct cpsv_a2nd_ckpt_unlink {
/* Delete   SaCkptHandleT              client_hdl; 
   SaCkptCheckpointHandleT             ckpt_id; */
	SaNameT ckpt_name;
} CPSV_A2ND_CKPT_UNLINK;

typedef struct cpsv_a2nd_rdset {
/* Delete SaCkptHandleT                client_hdl; */
	SaCkptCheckpointHandleT ckpt_id;
	SaTimeT reten_time;
} CPSV_A2ND_RDSET;

typedef struct cpsv_a2nd_active_rep_set {
/* Delete SaCkptHandleT                client_hdl; */
	SaCkptCheckpointHandleT ckpt_id;
} CPSV_A2ND_ACTIVE_REP_SET;

typedef struct cpsv_ckpt_status_get {
	SaCkptCheckpointHandleT ckpt_id;
} CPSV_CKPT_STATUS_GET;

/* shashi */

typedef struct cpsv_a2nd_sect_create {
	SaCkptCheckpointHandleT ckpt_id;
	SaCkptSectionCreationAttributesT sec_attri;
	void *init_data;
	SaSizeT init_size;

} CPSV_A2ND_SECT_CREATE;

/* shashi common on 17*/
typedef struct cpsv_ckpt_sect_create {
	SaCkptCheckpointHandleT ckpt_id;
	SaCkptCheckpointHandleT lcl_ckpt_id;
	MDS_DEST agent_mdest;
	SaCkptSectionCreationAttributesT sec_attri;
	void *init_data;
	SaSizeT init_size;

} CPSV_CKPT_SECT_CREATE;

typedef struct cpsv_ckpt_sect_delete {
	SaCkptCheckpointHandleT ckpt_id;
	SaCkptSectionIdT sec_id;
	SaAisErrorT error;
	SaCkptCheckpointHandleT lcl_ckpt_id;
	MDS_DEST agent_mdest;
} CPSV_CKPT_SECT_INFO;
/* end */

typedef struct cpsv_a2nd_sect_delete {
	SaCkptCheckpointHandleT ckpt_id;
	SaCkptSectionIdT sec_id;
	SaCkptCheckpointHandleT lcl_ckpt_id;
	MDS_DEST agent_mdest;

} CPSV_A2ND_SECT_DELETE;

typedef struct cpsv_a2nd_sect_exp_time {
	SaCkptCheckpointHandleT ckpt_id;
	SaCkptSectionIdT sec_id;
	SaTimeT exp_time;
} CPSV_A2ND_SECT_EXP_TIME;

typedef struct cpsv_a2nd_sect_iter_getnext {
	SaCkptCheckpointHandleT ckpt_id;
	SaCkptSectionIdT section_id;
	SaCkptSectionIterationHandleT iter_id;	/* CPA expects this to pass this in resp */
	SaCkptSectionsChosenT filter;
	uint32_t n_secs_trav;
	SaTimeT exp_tmr;
} CPSV_A2ND_SECT_ITER_GETNEXT;

typedef struct cpsv_a2nd_arrival_reg {
	SaCkptCheckpointHandleT client_hdl;
} CPSV_A2ND_ARRIVAL_REG;

typedef struct cpsv_a2nd_sync_ckpt {
	SaCkptCheckpointHandleT ckpt_id;
	SaCkptCheckpointHandleT lcl_ckpt_hdl;
	SaCkptHandleT client_hdl;
	SaInvocationT invocation;
	CPSV_SEND_INFO cpa_sinfo;
	bool is_ckpt_open;
} CPSV_A2ND_CKPT_SYNC;

typedef struct cpsv_ckpt_data {
	SaCkptSectionIdT sec_id;
	SaTimeT expirationTime;
	void *data;
	SaSizeT dataSize;	/* basically write_data size,means already data is present */
	SaSizeT readSize;
	SaOffsetT dataOffset;
	struct cpsv_ckpt_data *next;
} CPSV_CKPT_DATA;

typedef struct cpsv_a2nd_ckpt_list_update {
	SaCkptHandleT client_hdl; 
	SaNameT ckpt_name;
} CPSV_A2ND_CKPT_LIST_UPDATE;

#define CPSV_CKPT_ACCESS_WRITE           0x0
#define CPSV_CKPT_ACCESS_OVWRITE         0x1
#define CPSV_CKPT_ACCESS_READ            0x2
#define CPSV_CKPT_ACCESS_SYNC            0x3

#define MAX_SYNC_TRANSFER_SIZE           (30 * 1024 * 1024)

typedef struct cpsv_ckpt_access {
	SaUint32T type;		/* --- 0-write/1-overwrite/2-read/3-sync ----- */
	SaCkptCheckpointHandleT ckpt_id;
	SaCkptCheckpointHandleT lcl_ckpt_id;
	MDS_DEST agent_mdest;
	SaUint32T num_of_elmts;
	bool all_repl_evt_flag;
	CPSV_CKPT_DATA *data;
	SaUint32T seqno;	/* sequence number of the imessage */
	SaUint8T last_seq;	/* Last sequence true/false */
	CPSV_A2ND_CKPT_SYNC ckpt_sync;

} CPSV_CKPT_ACCESS;

/****************************************************************************
 Resp to Requests CPND --> CPA
 ****************************************************************************/

/* Init Response */
typedef struct cpsv_nd2a_init_rsp {
	SaCkptHandleT ckptHandle;
	SaAisErrorT error;
} CPSV_ND2A_INIT_RSP;

/* Open Response */
typedef struct cpsv_nd2a_open_rsp {
	SaCkptCheckpointHandleT lcl_ckpt_hdl;
	SaCkptCheckpointHandleT gbl_ckpt_hdl;
	SaCkptCheckpointCreationAttributesT creation_attr;
	bool is_active_exists;	/* true/false */
	MDS_DEST active_dest;

/* Not required
   CPSV_REQ_TYPE              req_type;*/
	SaInvocationT invocation;
	SaAisErrorT error;
	/* TBD Shared memory details, Name, SHM start address */
	void *addr;
} CPSV_ND2A_OPEN_RSP;

typedef struct cpsv_nd2a_sync_rsp {
	SaInvocationT invocation;
	SaCkptCheckpointHandleT lcl_ckpt_hdl;
	SaCkptHandleT client_hdl;
	SaAisErrorT error;
} CPSV_ND2A_SYNC_RSP;

typedef struct cpsv_nd2a_sect_iter_getnext_rsp {
	SaCkptCheckpointHandleT ckpt_id;
	SaCkptSectionIterationHandleT iter_id;	/* Passed by CPA */
	SaAisErrorT error;
	SaCkptSectionDescriptorT sect_desc;	/* Section Description */
	uint32_t n_secs_trav;

} CPSV_ND2A_SECT_ITER_GETNEXT_RSP;

typedef struct cpsv_nd2a_arrival_msg {
	SaCkptHandleT client_hdl;
	SaCkptCheckpointHandleT ckpt_hdl;
	SaCkptCheckpointHandleT lcl_ckpt_hdl;
	MDS_DEST mdest;
	SaUint32T num_of_elmts;
	CPSV_CKPT_DATA *ckpt_data;
} CPSV_ND2A_ARRIVAL_MSG;

typedef struct cpsv_nd2a_read_map {

/* 
   SaCkptSectionIdT sec_id; order is important in the array */
	SaInt32T offset_index;
	SaUint32T read_size;

} CPSV_ND2A_READ_MAP;

typedef struct cpsv_nd2a_read_data {
	void *data;
	SaUint32T read_size;
	SaUint32T err;
} CPSV_ND2A_READ_DATA;

#define CPSV_DATA_ACCESS_LCL_READ_RSP    0x1
#define CPSV_DATA_ACCESS_RMT_READ_RSP    0x2	/*Unused */
#define CPSV_DATA_ACCESS_WRITE_RSP       0x3
#define CPSV_DATA_ACCESS_OVWRITE_RSP    0x4



typedef struct cpsv_nd2a_read_rsp
{
   SaUint32T type; /* 1-read_lcl,2-read_rmt,3-write and 4-ovwrite rsps */
   SaUint32T num_of_elmts;
   SaUint32T size;
   SaAisErrorT error;
   SaCkptCheckpointHandleT   ckpt_id;   /* index for identifying the checkpoint */
   SaUint32T error_index;
   MDS_DEST from_svc;
   union {
     SaUint32T *write_err_index;
     CPSV_ND2A_READ_MAP *read_mapping;
     CPSV_ND2A_READ_DATA *read_data;
     CPSV_SAERR_INFO     ovwrite_error;

	} info;
   SaCkptCheckpointHandleT lcl_ckpt_id;
} CPSV_ND2A_DATA_ACCESS_RSP;

typedef struct cpsv_ckpt_status {
	SaAisErrorT error;
	SaCkptCheckpointHandleT ckpt_id;
	SaCkptCheckpointDescriptorT status;
} CPSV_CKPT_STATUS;

/****************************************************************************
 CPD --> CPND
 ****************************************************************************/
typedef struct cpsv_d2nd_ckpt_info {
	SaAisErrorT error;
	SaCkptCheckpointHandleT ckpt_id;
	bool is_active_exists;	/* true/false */
	MDS_DEST active_dest;
	uint32_t dest_cnt;
	CPSV_CPND_DEST_INFO *dest_list;
	SaCkptCheckpointCreationAttributesT attributes;
	bool ckpt_rep_create;
} CPSV_D2ND_CKPT_INFO;

typedef struct cpsv_d2nd_ckpt_create {
	SaNameT ckpt_name;
	CPSV_D2ND_CKPT_INFO ckpt_info;
} CPSV_D2ND_CKPT_CREATE;

typedef struct cpsv_ckpt_used_size {
	SaCkptCheckpointHandleT ckpt_id;
	SaUint32T ckpt_used_size;
	SaAisErrorT error;
} CPSV_CKPT_USED_SIZE;

typedef struct cpsv_ckpt_num_sections {
	SaCkptCheckpointHandleT ckpt_id;
	SaUint32T ckpt_num_sections;
	SaAisErrorT error;
} CPSV_CKPT_NUM_SECTIONS;

/****************************************************************************
 CPND --> CPD
 ****************************************************************************/
typedef enum cpsv_usr_info_type {
	CPSV_USR_INFO_CKPT_BASE,
	CPSV_USR_INFO_CKPT_OPEN = CPSV_USR_INFO_CKPT_BASE,
	CPSV_USR_INFO_CKPT_CLOSE,
	CPSV_USR_INFO_CKPT_OPEN_FIRST,
	CPSV_USR_INFO_CKPT_CLOSE_LAST,
	CPSV_USR_INFO_CKPT_MAX
} CPSV_USR_INFO_CKPT_TYPE;

typedef enum cpsv_ckpt_sec_info_type {
	CPSV_CKPT_SEC_INFO_BASE,
	CPSV_CKPT_SEC_INFO_CREATE = CPSV_CKPT_SEC_INFO_BASE,
	CPSV_CKPT_SEC_INFO_DELETE,
	CPSV_CKPT_SEC_INFO_CORRUPT,
	CPSV_CKPT_SEC_INFO_MAX
} CPSV_CKPT_SEC_INFO_TYPE;

typedef enum cpsv_ckpt_rdset_type {
	CPSV_CKPT_RDSET_INFO = 1,
	CPSV_CKPT_RDSET_START = 2,
	CPSV_CKPT_RDSET_STOP = 3,
	CPSV_CKPT_RDSET_MAX
} CPSV_CKPT_RDSET_TYPE;

typedef struct cpsv_nd2d_ckpt_create {
	SaNameT ckpt_name;
	SaCkptCheckpointCreationAttributesT attributes;
	SaCkptCheckpointOpenFlagsT ckpt_flags;
	SaVersionT client_version;

} CPSV_ND2D_CKPT_CREATE;

typedef struct cpsv_nd2d_usr_info {
	SaCkptCheckpointHandleT ckpt_id;
	CPSV_USR_INFO_CKPT_TYPE info_type;
	SaCkptCheckpointOpenFlagsT ckpt_flags;
} CPSV_ND2D_USR_INFO;

typedef struct cpsv_ckpt_sec_info_upd {
	SaCkptCheckpointHandleT ckpt_id;
	CPSV_CKPT_SEC_INFO_TYPE info_type;
} CPSV_CKPT_SEC_INFO_UPD;

typedef struct cpsv_ckpt_id_info {
	SaCkptCheckpointHandleT ckpt_id;
} CPSV_CKPT_ID_INFO;

typedef struct cpsv_nd2d_ckpt_unlink {
	SaNameT ckpt_name;
/* Deleted    SaCkptCheckpointHandleT   ckpt_id;   */
} CPSV_ND2D_CKPT_UNLINK;

typedef struct cpsv_ref_cnt 
{
   SaCkptCheckpointHandleT  ckpt_id;
   uint32_t  ckpt_ref_cnt;
}CPSV_REF_CNT;

typedef struct cpsv_a2nd_refcntset
{
   uint32_t           		      no_of_nodes;      
   CPSV_REF_CNT                       ref_cnt_array[100];
}CPSV_A2ND_REFCNTSET;
typedef struct cpsv_ckpt_rdset {
	SaCkptCheckpointHandleT ckpt_id;
	SaTimeT reten_time;
	uint32_t type;
} CPSV_CKPT_RDSET;

typedef struct cpsv_nd2d_ckpt_name {
	SaNameT ckpt_name;
} CPSV_CKPT_NAME_INFO;

/* CPA Local Events */
typedef struct cpa_tmr_info {
	uint32_t type;
	SaCkptCheckpointHandleT lcl_ckpt_hdl;
	SaCkptHandleT client_hdl;
	SaInvocationT invocation;
} CPA_TMR_INFO;

/******************************************************************************
 CPA Event Data Struct 
 ******************************************************************************/
typedef struct cpa_evt {
	CPA_EVT_TYPE type;
	union {
		CPSV_ND2A_INIT_RSP initRsp;
		CPSV_SAERR_INFO finRsp;
		CPSV_ND2A_OPEN_RSP openRsp;
		CPSV_SAERR_INFO closeRsp;
		CPSV_SAERR_INFO ulinkRsp;
		CPSV_SAERR_INFO rdsetRsp;
		CPSV_SAERR_INFO arsetRsp;
		CPSV_CKPT_STATUS status;

		CPSV_CKPT_SECT_INFO sec_creat_rsp;
		CPSV_SAERR_INFO sec_delete_rsp;
		CPSV_SAERR_INFO sec_exptmr_rsp;
		CPSV_ND2A_SECT_ITER_GETNEXT_RSP iter_next_rsp;

		CPSV_SAERR_INFO sec_iter_rsp;

		CPSV_ND2A_ARRIVAL_MSG arr_msg;

		CPSV_ND2A_DATA_ACCESS_RSP sec_data_rsp;

		CPSV_ND2A_SYNC_RSP sync_rsp;
		CPSV_SAERR_INFO readackRsp;
		CPA_TMR_INFO tmr_info;
		CPSV_CKPT_DEST_INFO ackpt_info;
	} info;

} CPA_EVT;

/* CPND Local Events */
typedef struct cpnd_tmr_info {
	uint32_t type;
	SaCkptCheckpointHandleT ckpt_id;
	uint32_t lcl_sec_id;
	MDS_DEST agent_dest;
	SaUint32T write_type;
	CPSV_SEND_INFO sinfo;
	SaInvocationT invocation;
	SaCkptCheckpointHandleT lcl_ckpt_hdl;
	CPND_TMR *cpnd_tmr;
} CPND_TMR_INFO;

/******************************************************************************
 CPND Event Data Structures
 ******************************************************************************/
typedef struct cpnd_evt
{
   bool       dont_free_me;
   SaAisErrorT    error; 
   CPND_EVT_TYPE   type;
   union
   {
      /* CPA --> CPND */
      CPSV_INIT_REQ             initReq;
      CPSV_FINALIZE_REQ         finReq;
      CPSV_A2ND_OPEN_REQ        openReq;
      CPSV_A2ND_CKPT_CLOSE      closeReq;
      CPSV_A2ND_CKPT_UNLINK     ulinkReq;
      CPSV_A2ND_RDSET           rdsetReq;
      CPSV_A2ND_ACTIVE_REP_SET  arsetReq;
      CPSV_CKPT_STATUS_GET      statReq;
      CPSV_A2ND_REFCNTSET       refCntsetReq;

		CPSV_CKPT_SECT_CREATE sec_creatReq;
		CPSV_A2ND_SECT_DELETE sec_delReq;
		CPSV_A2ND_SECT_EXP_TIME sec_expset;
		CPSV_A2ND_SECT_ITER_GETNEXT iter_getnext;

		CPSV_A2ND_ARRIVAL_REG arr_ntfy;

		CPSV_CKPT_ACCESS ckpt_write;
		CPSV_CKPT_ACCESS ckpt_read;
		CPSV_A2ND_CKPT_SYNC ckpt_sync;
		CPSV_CKPT_DEST_INFO ckpt_read_ack;

		/* CPD --> CPND */
		CPSV_D2ND_CKPT_INFO ckpt_info;
		CPSV_CKPT_USED_SIZE ckpt_mem_size;
		CPSV_CKPT_NUM_SECTIONS ckpt_sections;
		CPSV_CKPT_DESTLIST_INFO ckpt_add;
		CPSV_CKPT_DEST_INFO ckpt_del;
		CPSV_D2ND_CKPT_CREATE ckpt_create;

		CPSV_CKPT_ID_INFO ckpt_destroy;
		CPSV_CKPT_ID_INFO ckpt_ulink;

		CPSV_CKPT_RDSET rdset;
		CPSV_CKPT_DEST_INFO active_set;

		CPSV_SAERR_INFO cl_ack;	/* How about get rid of this ACK */
		CPSV_SAERR_INFO ulink_ack;	/* How about get rid of this ACK */
		CPSV_SAERR_INFO rdset_ack;	/* How about get rid of this ACK */
		CPSV_SAERR_INFO crset_ack;	/* How about get rid of this ACK */
		CPSV_SAERR_INFO arep_ack;
		CPSV_SAERR_INFO destroy_ack;

		CPSV_CKPT_ID_INFO cpnd_restart;
		CPSV_CKPT_DESTLIST_INFO cpnd_restart_done;

		/* CPND --> CPND */

		CPSV_CKPT_STATUS_GET stat_get;
		CPSV_CKPT_STATUS status;

		CPSV_CKPT_SECT_CREATE active_sec_creat;
		CPSV_SAERR_INFO sec_creat_rsp;
		CPSV_CKPT_SECT_INFO active_sec_creat_rsp;

		CPSV_CKPT_SECT_INFO sec_delete_req;
		CPSV_SAERR_INFO sec_delete_rsp;

		/* Iteration initialize */
		CPSV_CKPT_STATUS_GET sec_iter_req;
		/* need to change to common */

		CPSV_A2ND_SECT_EXP_TIME sec_exp_set;
		CPSV_SAERR_INFO sec_exp_rsp;

		CPSV_A2ND_CKPT_SYNC sync_req;
		CPSV_CKPT_ACCESS ckpt_nd2nd_sync;
		CPSV_SAERR_INFO active_sync_rsp;

		CPSV_CKPT_ACCESS ckpt_nd2nd_data;
		CPSV_ND2A_DATA_ACCESS_RSP ckpt_nd2nd_data_rsp;
		CPSV_A2ND_SECT_ITER_GETNEXT getnext_req;
		CPSV_ND2A_SECT_ITER_GETNEXT_RSP ckpt_nd2nd_getnext_rsp;

		/* mapping for local and sync */

		/* Locally generated events */
		CPSV_MDS_INFO mds_info;
		CPND_TMR_INFO tmr_info;
		CPSV_A2ND_CKPT_LIST_UPDATE  ckptListUpdate;

	} info;
} CPND_EVT;

/* CPD Local Events */
typedef struct cpd_tmr_info {
	uint32_t type;
	union {
		MDS_DEST cpnd_dest;
	} info;
} CPD_TMR_INFO;

/******************************************************************************
 CPD Event Data Structures
 ******************************************************************************/
typedef struct cpd_evt {
	CPD_EVT_TYPE type;
	union {
		/* CPND --> CPD */
		CPSV_ND2D_CKPT_CREATE ckpt_create;
		CPSV_ND2D_USR_INFO ckpt_usr_info;
		CPSV_CKPT_SEC_INFO_UPD ckpt_sec_info;
		CPSV_CKPT_USED_SIZE ckpt_mem_used;
		CPSV_CKPT_NUM_SECTIONS ckpt_created_sections;
/* Deleted       CPSV_ND2D_CKPT_OPEN    ckpt_open; */
		CPSV_CKPT_ID_INFO ckpt_close;
		CPSV_CKPT_ID_INFO ckpt_destroy;
		CPSV_CKPT_NAME_INFO ckpt_destroy_byname;
		CPSV_ND2D_CKPT_UNLINK ckpt_ulink;
		CPSV_CKPT_RDSET rd_set;
		CPSV_CKPT_DEST_INFO arep_set;
		CPD_TMR_INFO tmr_info;
		CPSV_MDS_INFO mds_info;

	} info;
} CPD_EVT;

/******************************************************************************
 CPSV Event Data Struct 
 ******************************************************************************/
typedef struct cpsv_evt {
	struct cpsv_evt *next;
	CPSV_EVT_TYPE type;
	union {
		CPA_EVT cpa;
		CPND_EVT cpnd;
		CPD_EVT cpd;
	} info;
	CPSV_SEND_INFO sinfo;	/* MDS Sender information */

} CPSV_EVT;

/* Event Declerations */
uint32_t cpsv_evt_cpy(CPSV_EVT *src, CPSV_EVT *dest, uint32_t svc_id);
uint32_t cpsv_evt_enc_flat(EDU_HDL *edu_hdl, CPSV_EVT *i_evt, NCS_UBAID *o_ub);
uint32_t cpsv_evt_dec_flat(EDU_HDL *edu_hdl, NCS_UBAID *i_ub, CPSV_EVT *o_evt);
uint32_t cpsv_ckpt_data_encode(NCS_UBAID *i_ub, CPSV_CKPT_DATA *data);
uint32_t cpsv_ref_cnt_encode(NCS_UBAID *i_ub, CPSV_A2ND_REFCNTSET *data);
uint32_t cpsv_refcnt_ckptid_decode(CPSV_A2ND_REFCNTSET *data, NCS_UBAID *io_uba);
uint32_t cpsv_ckpt_node_encode(NCS_UBAID *i_ub, CPSV_CKPT_DATA *pdata);
uint32_t cpsv_ckpt_data_decode(CPSV_CKPT_DATA **data, NCS_UBAID *io_uba);
uint32_t cpsv_ckpt_node_decode(CPSV_CKPT_DATA *pdata, NCS_UBAID *io_uba);
uint32_t cpsv_ckpt_access_encode(CPSV_CKPT_ACCESS *ckpt_data, NCS_UBAID *io_uba);
uint32_t cpsv_nd2a_read_data_encode(CPSV_ND2A_READ_DATA *read_data, NCS_UBAID *io_uba);
uint32_t cpsv_data_access_rsp_decode(CPSV_ND2A_DATA_ACCESS_RSP *data_rsp, NCS_UBAID *io_uba, MDS_CLIENT_MSG_FORMAT_VER i_msg_fmt_ver);
uint32_t cpsv_nd2a_read_data_decode(CPSV_ND2A_READ_DATA *read_data, NCS_UBAID *io_uba);
uint32_t cpsv_data_access_rsp_encode(CPSV_ND2A_DATA_ACCESS_RSP *data_rsp, NCS_UBAID *io_uba, MDS_CLIENT_MSG_FORMAT_VER o_msg_fmt_ver);

/*
 * m_CPSV_DBG_SINK
 *
 * If cpa/cpnd/cpd fails an if-conditional or other test that we would not expect
 * under normal conditions, it will call this macro.
 *
 * If NCS_CPD == 1, fall into the sink function. A customer can put
 * a breakpoint there or leave the CONSOLE print statement, etc.
 *
 * If NCS_DEBUG == 0, just echo the value passed. It is typically a
 * return code or a NULL.
 *
 * NCS_CPD & NCS_CPND can be enabled / disabled in Makefile.am
 */

#if ((NCS_CPND == 1) || (NCS_CPD == 1))

uint32_t cpsv_dbg_sink(uint32_t, char *, uint32_t, char *);
#define m_CPSV_DBG_SINK(r, s)  cpsv_dbg_sink(__LINE__,__FILE__,(uint32_t)r, (char*)s)
#define m_CPSV_DBG_VOID     cpsv_dbg_sink(__LINE__,__FILE__,1)
#else

#define m_CPSV_DBG_SINK(r, s)  r
#define m_CPSV_DBG_VOID
#endif

#endif
