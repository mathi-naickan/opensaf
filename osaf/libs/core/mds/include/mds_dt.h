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

  DESCRIPTION:  MCM DT2C header

******************************************************************************
*/

#ifndef _MDS_DT_H
#define _MDS_DT_H

#include "mds_dt2c.h"
#include <ncsgl_defs.h>
#include "ncs_lib.h"
#include "ncssysf_tmr.h"
#include "ncs_main_papi.h"
#include "ncssysf_mem.h"
#include "ncspatricia.h"

#include <linux/tipc.h>

/* This file is private to the MDTM layer. */

/* Global ReAssembly TMR Value */
extern uns32 MDTM_REASSEMBLE_TMR_VAL;
extern uns32 MDTM_CACHED_EVENTS_TMR_VAL;

/* Locks */
extern NCS_LOCK gl_lock;
extern NCS_LOCK *mds_lock(void);

typedef struct mdtm_reassembly_key {

	uns32 frag_sequence_num;	/* Frag Sequence number of this message */
	struct tipc_portid tipc_id;

} MDTM_REASSEMBLY_KEY;

typedef struct mdtm_reassembly_queue {

	/* Indexing info */
	NCS_PATRICIA_NODE node;

	/* Explicit key for fast-access in the send and receive flow */
	MDTM_REASSEMBLY_KEY key;

	uns8 to_be_dropped;	/* Out of recd or anything more */
	uns8 tmr_flag;		/* If true represents the timer running else timer has stopped
				   donot reassemble any further data */
	uns16 next_frag_num;

	uns32 svc_sequence_num;	/* SVC Sequence number of this message */

	MDS_DATA_RECV recv;

	tmr_t tmr;

	MDS_TMR_REQ_INFO *tmr_info;

	uns32 tmr_hdl;

} MDTM_REASSEMBLY_QUEUE;

/* Defines regarding to the Send and receive buff sizes */
#define MDS_HDR_LEN         24	/* Mds_prot-4bit, Mds_version-2bit , Msg prior-2bit, Hdr_len-16bit, Seq_no-32bit, Enc_dec_type-2bit, Msg_snd_type-6bit,
				   Pwe_id-16bit, Sndr_vdest_id-16bit, Sndr_svc_id-16bit, Rcvr_vdest_id-16bit, Rcvr_svc_id-16bit, Exch_id-32bit, App_Vers-16bit */

/* Following defines the positions of each field in the mds hdr */
#define MDS_HEADER_PROT_VER_PRIOR_POSITION    0
#define MDS_HEADER_HDR_LEN_POSITION           1
#define MDS_HEADER_SEQ_NUM_POSITION           3
#define MDS_HEADER_MSG_TYPE_POSITION          7
#define MDS_HEADER_PWE_ID_POSITION            8
#define MDS_HEADER_SNDR_VDEST_ID_POSITION     10
#define MDS_HEADER_SNDR_SVC_ID_POSITION       12
#define MDS_HEADER_RCVR_VDEST_ID_POSITION     14
#define MDS_HEADER_RCVR_SVC_ID_POSITION       16
#define MDS_HEADER_EXCHG_ID_POSITION          18
#define MDS_HEADER_APP_VERSION_ID_POSITION    22

#define MDTM_FRAG_HDR_LEN    8	/* Msg Seq_no-32bit, More Frag-1bit, Frag_num-15bit, Frag_size-16bit */

/* Following defines the positions of each field in the frag hdr */
#define MDTM_FRAG_HDR_SEQ_NUM_POSITION        0
#define MDTM_FRAG_HDR_MF_FRAG_NUM_POSITION    4
#define MDTM_FRAG_HDR_FRAG_SIZE_POSITION      6

#ifdef MDS_CHECKSUM_ENABLE_FLAG
#define CHK_SUM_ENB_FLAG_LEN     1
#define CHK_SUM_BYTES_LEN        2

#define SUM_MDS_HDR_PLUS_MDTM_HDR_PLUS_LEN ((2+CHK_SUM_ENB_FLAG_LEN+CHK_SUM_BYTES_LEN+MDTM_FRAG_HDR_LEN+MDS_HDR_LEN))	/*2Len,1chksum_flg,2chksum,Fraghdr,MDS hdr */
#else
#define SUM_MDS_HDR_PLUS_MDTM_HDR_PLUS_LEN ((2+MDTM_FRAG_HDR_LEN+MDS_HDR_LEN))	/* 2 Len , Frag hdr, MDS hdr */
#endif

#define MDTM_MAX_SEGMENT_SIZE (MDS_DIRECT_BUF_MAXSIZE+SUM_MDS_HDR_PLUS_MDTM_HDR_PLUS_LEN)

#define MDTM_MAX_DIRECT_BUFF_SIZE  MDTM_MAX_SEGMENT_SIZE

#define MDTM_NORMAL_MSG_FRAG_SIZE   1400

#define MDTM_RECV_BUFFER_SIZE ((MDS_DIRECT_BUF_MAXSIZE>MDTM_NORMAL_MSG_FRAG_SIZE)? \
                      (MDS_DIRECT_BUF_MAXSIZE+SUM_MDS_HDR_PLUS_MDTM_HDR_PLUS_LEN):(MDTM_NORMAL_MSG_FRAG_SIZE+SUM_MDS_HDR_PLUS_MDTM_HDR_PLUS_LEN))

/* Prefixes and defines regarding to the MDS_TIPC*/

#define MDS_TIPC_PREFIX         0x56000000

typedef enum {
	MDS_SVC_INST_TYPE = 0x00010000,
	MDS_VDEST_INST_TYPE = 0x00020000,
	MDS_PCON_INST_TYPE = 0x00030000,
	MDS_NODE_INST_TYPE = 0x00000000,
	MDS_PROCESS_INST_TYPE = 0x00050000,
} MDS_MDTM_INST_TYPES;

typedef enum {
	MDTM_TX_TYPE_TIPC = 1,
	/* MDTM_TX_TYPE_BLAH = 2, If new transport is there */
} MDTM_TX_TYPE;

typedef enum {
	MDTM_CACHED_SVC_EVT = 1,
	MDTM_CACHED_VDEST_EVT
} MDTM_CACHED_EVENT_TYPE;

typedef struct mdtm_cached_event_list {
	MDTM_CACHED_EVENT_TYPE event_type;
	union {
		struct {
			MDS_VDEST_ID vdest_id;
		} vdest_info;

		struct {
			MDS_SVC_ID svc_id;
			MDS_VDEST_ID vdest_id;
			PW_ENV_ID pwe_id;
			NCS_VDEST_TYPE policy;
			V_DEST_RL role;
			NCSMDS_SCOPE_TYPE scope;
			MDS_SUBTN_REF_VAL subtn_ref_val;
			MDS_SVC_HDL svc_hdl;
			MDS_SVC_PVT_SUB_PART_VER svc_sub_part_ver;
			MDS_SVC_ARCHWORD_TYPE archword_type;
		} svc_info;
	} info;
	struct mdtm_cached_event_list *next;

} MDTM_CACHED_EVENT_LIST;

/* This structure element can be passed from MDTM to MCM when receiving message */
typedef struct mds_subscription_ll_hdl {
	NCSCONTEXT ll_hdl;
	NCSCONTEXT ul_hdl;
} MDS_SUBSCRIPTION_LL_HDL;

/* ******************************************** */
/* ******************************************** */
/*               TABLE Operations               */
/* ******************************************** */
/* ******************************************** */

/* ADEST INFO TABLE Operations */
/*
extern uns32 mds_adest_info_tbl_add ();
extern uns32 mds_adest_info_tbl_set ();
extern uns32 mds_adest_info_tbl_get ();
extern uns32 mds_adest_info_tbl_getnext ();
extern uns32 mds_adest_info_tbl_query ();
*/

/* DEFINE THESE THINGS*/

#define NCS_MDTM_TASKNAME       "MDTM"
#define NCS_MDTM_PRIORITY       NCS_TASK_PRIORITY_4
#define NCS_MDTM_STACKSIZE      NCS_STACKSIZE_HUGE

#define MDTM_TIPC_POLL_TIMEOUT 20000

#define m_MMGR_ALLOC_REASSEM_QUEUE m_NCS_MEM_ALLOC(sizeof(MDTM_REASSEMBLY_QUEUE),\
                                                  NCS_MEM_REGION_PERSISTENT,\
                                                  NCS_SERVICE_ID_MDS,\
                                                  MDS_MEM_REASSEM_QUEUE)

#define m_MMGR_FREE_REASSEM_QUEUE(p)  m_NCS_MEM_FREE(p,\
                                     NCS_MEM_REGION_PERSISTENT,\
                                     NCS_SERVICE_ID_MDS,\
                                     MDS_MEM_REASSEM_QUEUE)

#define m_MMGR_ALLOC_HDL_LIST  m_NCS_MEM_ALLOC(sizeof(MDTM_REF_HDL_LIST), \
                                                  NCS_MEM_REGION_PERSISTENT,\
                                                  NCS_SERVICE_ID_MDS,\
                                                  MDS_MEM_HDL_LIST)

#define m_MMGR_FREE_HDL_LIST(p)  m_NCS_MEM_FREE(p, \
                                                  NCS_MEM_REGION_PERSISTENT,\
                                                  NCS_SERVICE_ID_MDS,\
                                                  MDS_MEM_HDL_LIST)

#define m_MMGR_ALLOC_CACHED_EVENTS_LIST  m_NCS_MEM_ALLOC(sizeof(MDTM_CACHED_EVENT_LIST), \
                                                  NCS_MEM_REGION_PERSISTENT,\
                                                  NCS_SERVICE_ID_MDS,\
                                                  MDS_MEM_CACHED_EVENTS_LIST)

#define m_MMGR_FREE_CACHED_EVENTS_LIST(p)  m_NCS_MEM_FREE(p, \
                                                  NCS_MEM_REGION_PERSISTENT,\
                                                  NCS_SERVICE_ID_MDS,\
                                                  MDS_MEM_CACHED_EVENTS_LIST)

#endif
