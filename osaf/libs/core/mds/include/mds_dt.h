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


/* This file is private to the MDTM layer. */

/* Global ReAssembly TMR Value */
extern uint32_t MDTM_REASSEMBLE_TMR_VAL;
extern uint32_t MDTM_CACHED_EVENTS_TMR_VAL;

typedef struct mdtm_reassembly_key {

	uint32_t frag_sequence_num;	/* Frag Sequence number of this message */
	MDS_DEST id;

} MDTM_REASSEMBLY_KEY;

typedef struct mdtm_reassembly_queue {

	/* Indexing info */
	NCS_PATRICIA_NODE node;

	/* Explicit key for fast-access in the send and receive flow */
	MDTM_REASSEMBLY_KEY key;

	uint8_t to_be_dropped;	/* Out of recd or anything more */
	uint8_t tmr_flag;		/* If true represents the timer running else timer has stopped
				   donot reassemble any further data */
	uint16_t next_frag_num;

	uint32_t svc_sequence_num;	/* SVC Sequence number of this message */

	MDS_DATA_RECV recv;

	tmr_t tmr;

	MDS_TMR_REQ_INFO *tmr_info;

	uint32_t tmr_hdl;

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

/* Common to TCP and TIPC */
#define MAX_SUBSCRIPTIONS   200
#define MAX_SUBSCRIPTIONS_RETURN_ERROR   500
#define MDS_EVENT_SHIFT_FOR_PWE   10	/* As SVCID is 8-MDS Prefix, 8- SVC Inst, 6- PWE, 10 -SVCid */
#define MDS_EVENT_MASK_FOR_PWE  0x3f
#define MDS_EVENT_MASK_FOR_SVCID  0x3ff

#define LEN_4_BYTES         32
#define MDS_PWE_BITS_LEN     6
#define MDS_VER_BITS_LEN       8
#define MDS_ARCHWORD_BITS_LEN  4
#define VDEST_POLICY_LEN     1
#define ACT_STBY_LEN         1
#define MDS_SCOPE_LEN        2

#define MDS_VER_MASK      0x0ff00000
#define MDS_POLICY_MASK   0x00080000
#define MDS_ARCHWORD_MASK 0xf0000000
#define MDS_ROLE_MASK     0x00040000
#define MDS_SCOPE_MASK    0x00030000

#define MORE_FRAG_BIT  0x8000
#define NO_FRAG_BIT    0x0000

uint32_t mdtm_add_to_ref_tbl(MDS_SVC_HDL svc_hdl, MDS_SUBTN_REF_VAL ref);
uint32_t mdtm_del_from_ref_tbl(MDS_SUBTN_REF_VAL ref);
uint32_t mds_tmr_mailbox_processing(void);
uint32_t mdtm_get_from_ref_tbl(MDS_SUBTN_REF_VAL ref, MDS_SVC_HDL *svc_hdl);
uint32_t mdtm_add_frag_hdr(uint8_t *buf_ptr, uint16_t len, uint32_t seq_num, uint16_t frag_byte);
uint32_t mdtm_free_reassem_msg_mem(MDS_ENCODED_MSG *msg);
uint32_t mdtm_process_recv_data(uint8_t *buf, uint16_t len, uint64_t tipc_id, uint32_t *buff_dump);

typedef enum {
	MDTM_TX_TYPE_TIPC = 1,
	MDTM_TX_TYPE_TCP = 2,
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

typedef struct mdtm_ref_hdl_list {
	struct mdtm_ref_hdl_list *next;
	MDS_SUBTN_REF_VAL ref_val;
	MDS_SVC_HDL svc_hdl;
} MDTM_REF_HDL_LIST;

MDTM_REF_HDL_LIST *mdtm_ref_hdl_list_hdr;
uint32_t mdtm_attach_mbx(SYSF_MBX mbx);
void mds_buff_dump(uint8_t *buff, uint32_t len, uint32_t max);
NCS_PATRICIA_TREE mdtm_reassembly_list;

uint32_t mdtm_set_transport(MDTM_TX_TYPE transport);
bool mdtm_mailbox_mbx_cleanup(NCSCONTEXT arg, NCSCONTEXT msg);

#define MDTM_PKT_TYPE_OFFSET            4	/* Fragmented or normal */

#define MDTM_CHECK_MORE_FRAG            0x8000
#define MDTM_NORMAL_PKT                 0
#define MDTM_FIRST_FRAG_NUM             0x8001
#define MDS_MSG_SND_TYPE_OFFSET         10
#define MDS_MSG_TYPE_OFFSET             2
#define MDS_MSG_ENC_TYPE_OFFSET         5

#define MDTM_DIRECT     0
#define MDTM_REASSEMBLE 1

#define m_MDS_MCM_GET_NODE_ID    m_MDS_GET_NODE_ID_FROM_ADEST(tipc_cb.adest)

#define MDS_MCM_GET_PROCESS_ID  m_MDS_GET_PROCESS_ID_FROM_ADEST(tipc_cb.adest)

#define MDS_PROT        0xA0
#define MDS_VERSION         0x08
#define MDS_PROT_VER_MASK  (MDS_PROT | MDS_VERSION)
#define MDTM_PRI_MASK 0x3

/* Added for the subscription changes */
#define MDS_NCS_CHASSIS_ID       (m_NCS_GET_NODE_ID&0x00ff0000)
#define MDS_TIPC_COMMON_ID       0x01001000

/*
 * In the default addressing scheme TIPC addresses will be 1.1.31, 1.1.47.
 * The slot ID is shifted 4 bits up and subslot ID is added in the 4 LSB.
 * When use of subslot ID is disabled (set MDS_USE_SUBSLOT_ID=0 in CFLAGS), the
 * TIPC addresses will be 1.1.1, 1.1.2, etc.
 */
#ifndef MDS_USE_SUBSLOT_ID
#define MDS_USE_SUBSLOT_ID 1
#endif

#if (MDS_USE_SUBSLOT_ID == 0)
#define MDS_TIPC_NODE_ID_MIN     0x01001001
#define MDS_TIPC_NODE_ID_MAX     0x010010ff
#define MDS_NCS_NODE_ID_MIN      (MDS_NCS_CHASSIS_ID|0x0000010f)
#define MDS_NCS_NODE_ID_MAX      (MDS_NCS_CHASSIS_ID|0x0000ff0f)
#define m_MDS_GET_NCS_NODE_ID_FROM_TIPC_NODE_ID(node) \
        (NODE_ID)( MDS_NCS_CHASSIS_ID | (((node)&0xff)<<8) | (0xf))
#define m_MDS_GET_TIPC_NODE_ID_FROM_NCS_NODE_ID(node) \
        (NODE_ID)( MDS_TIPC_COMMON_ID | (((node)&0xff00)>>8) )
#else
#define MDS_TIPC_NODE_ID_MIN     0x01001001
#define MDS_TIPC_NODE_ID_MAX     0x0100110f
#define MDS_NCS_NODE_ID_MIN      (MDS_NCS_CHASSIS_ID|0x00000100)
#define MDS_NCS_NODE_ID_MAX      (MDS_NCS_CHASSIS_ID|0x0000100f)
#define m_MDS_GET_NCS_NODE_ID_FROM_TIPC_NODE_ID(node) \
        (NODE_ID)( MDS_NCS_CHASSIS_ID | ((node)&0xf) | (((node)&0xff0)<<4))
#define m_MDS_GET_TIPC_NODE_ID_FROM_NCS_NODE_ID(node) \
        (NODE_ID)( MDS_TIPC_COMMON_ID | (((node)&0xff00)>>4) | ((node)&0xf) )
#endif

#define m_MDS_CHECK_TIPC_NODE_ID_RANGE(node) (((((node)<MDS_TIPC_NODE_ID_MIN)||((node)>MDS_TIPC_NODE_ID_MAX))?NCSCC_RC_FAILURE:NCSCC_RC_SUCCESS))
#define m_MDS_CHECK_NCS_NODE_ID_RANGE(node) (((((node)<MDS_NCS_NODE_ID_MIN)||((node)>MDS_NCS_NODE_ID_MAX))?NCSCC_RC_FAILURE:NCSCC_RC_SUCCESS))

/* ******************************************** */
/* ******************************************** */
/*               TABLE Operations               */
/* ******************************************** */
/* ******************************************** */

/* ADEST INFO TABLE Operations */
/*
extern uint32_t mds_adest_info_tbl_add ();
extern uint32_t mds_adest_info_tbl_set ();
extern uint32_t mds_adest_info_tbl_get ();
extern uint32_t mds_adest_info_tbl_getnext ();
extern uint32_t mds_adest_info_tbl_query ();
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
