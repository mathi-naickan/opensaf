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

  DESCRIPTION:  This file contains the API that are given by MDTM module to MCM module

******************************************************************************
*/

#ifndef _MDS_DT2C_H
#define _MDS_DT2C_H

#if __AFSINC_LINUX__
#include <linux/tipc.h>
#endif

#include <ncsgl_defs.h>
#include "mds_papi.h"
#include <linux/tipc.h>

/* Specification of the interface between MCM and MDTM */

typedef uns8 MDS_SVC_ARCHWORD_TYPE;	/*MDS  app-svc arch and word_size combination */

/* MDS_WORD_SIZE_TYPE and MDS_ARCH_TYPE are compile-time macros */

#ifndef MDS_ARCH_TYPE
#define MDS_ARCH_TYPE 0		/* Stands for unspecified architecture type */
#elif (MDS_ARCH_TYPE > 7)
#error MDS_ARCH_TYPE should be in the range 0 to 7.
#endif

#define MDS_WORD_SIZE_TYPE ((sizeof(long)/4) - 1)	/* 0 for 32-bit, 1 for 64-bit */

#define MDS_SELF_ARCHWORD    ((MDS_SVC_ARCHWORD_TYPE) ((MDS_WORD_SIZE_TYPE<<3) | MDS_ARCH_TYPE))

typedef enum {
	MDS_VIEW_NORMAL,
	MDS_VIEW_RED
} MDS_VIEW;

typedef uns16 MDS_PWE_ID;

typedef uns64 MDS_SUBTN_REF_VAL;

typedef NCS_VDEST_TYPE MDS_POLICY;

typedef uns32 MDS_VDEST_HDL;	/* <0,vdestid> */
typedef uns64 MDS_SVC_HDL;	/* <pweid,vdestid,svcid> */
typedef uns32 MDS_PWE_HDL;	/* <pweid,vdestid> */
/* typedef uns32 MDS_SUBTN_HDL; */
typedef uns64 MDS_TX_HDL;
typedef enum {
	MDS_SUBTN_IMPLICIT,
	MDS_SUBTN_EXPLICIT
} MDS_SUBTN_TYPE;

typedef enum {
	MDS_ENC_TYPE_CPY,
	MDS_ENC_TYPE_FLAT,
	MDS_ENC_TYPE_FULL,
	MDS_ENC_TYPE_DIRECT_BUFF,
} MDS_ENC_TYPES;

typedef struct mds_direct_buff_info {
	uns16 len;
	MDS_DIRECT_BUFF buff;
} MDS_DIRECT_BUFF_INFO;

typedef struct mds_encoded_msg {
	MDS_ENC_TYPES encoding;
	union {
		NCSCONTEXT cpy_msg;

		NCS_UBAID flat_uba;
		NCS_UBAID fullenc_uba;
		MDS_DIRECT_BUFF_INFO buff_info;
	} data;
} MDS_ENCODED_MSG;

/* Structure format given to MCM from MDTM when the data is recd */

typedef struct mds_data_recv {

	MDS_ENCODED_MSG msg;
	uns16 src_svc_id;
	uns16 src_pwe_id;
	uns16 src_vdest;
	uns32 exchange_id;
	MDS_SVC_HDL dest_svc_hdl;	/* Got from upper layer by passing SVC,PWE and VDEST */
	MDS_DEST src_adest;
	MDS_SENDTYPES snd_type;
	uns8 ret_val;		/* Valid only for ACK messages */
	MDS_SEND_PRIORITY_TYPE pri;
	MDS_CLIENT_MSG_FORMAT_VER msg_fmt_ver;	/* message format version specification */
	MDS_SVC_PVT_SUB_PART_VER src_svc_sub_part_ver;
	MDS_SVC_ARCHWORD_TYPE msg_arch_word;

} MDS_DATA_RECV;

/* Sending data/messages to the destination */
typedef struct mdtm_send_req {

#define DESTINATION_SAME_PROCESS  1
#define DESTINATION_ON_NODE       2
#define DESTINATION_OFF_NODE      3

	uns8 to;		/* Destination same node, process or off node */
	NCS_BOOL consume_buf;	/* Can be FALSE for broadcast.
				   Avoids ditto step.
				   Is there a better way?
				 */
	MDS_SVC_ID src_svc_id;
	PW_ENV_ID src_pwe_id;
	MDS_VDEST_ID src_vdest_id;
	MDS_DEST src_adest;
	MDS_SENDTYPES snd_type;
	uns32 xch_id;
	uns32 svc_seq_num;

	MDS_SVC_ID dest_svc_id;
	PW_ENV_ID dest_pwe_id;
	MDS_VDEST_ID dest_vdest_id;

	MDS_ENCODED_MSG msg;	/* Mem released after send if
				   consume_buf = TRUE- SOme
				   such scheme
				 */
	MDS_DEST adest;		/* MDTM to do local/remote routing, destination adest */
	MDS_SEND_PRIORITY_TYPE pri;
	MDS_CLIENT_MSG_FORMAT_VER msg_fmt_ver;	/* message format version specification */
	MDS_SVC_PVT_SUB_PART_VER src_svc_sub_part_ver;
	MDS_SVC_ARCHWORD_TYPE msg_arch_word;
} MDTM_SEND_REQ;

typedef struct mds_await_active_queue {

	MDTM_SEND_REQ req;

	struct mds_await_active_queue *next_msg;
} MDS_AWAIT_ACTIVE_QUEUE;

typedef struct mds_mdtm_query_adest_info {
	MDS_DEST i_adest;
	NCS_BOOL o_is_reachable;	/* Putting JFK. Do not know use */
	NCS_BOOL o_is_in_mypcon;
	NCS_BOOL o_is_in_mynode;
	NCS_BOOL o_is_in_mychassis;
	MDS_ENC_TYPES o_enc_type;	/* enc/enc-flat/cpy */
} MDS_MDTM_QUERY_ADEST_INFO;

typedef struct mdtm_svc_inst_info {
	PW_ENV_ID pwe_id;
	MDS_SVC_ID svc_id;
	NCSMDS_SCOPE_TYPE install_scope;
	V_DEST_RL role;
	MDS_VDEST_ID vdest_id;
	NCS_VDEST_TYPE vdest_policy;
	MDS_SVC_PVT_SUB_PART_VER svc_sub_part_ver;
} MDTM_SVC_INST_INFO;

typedef MDTM_SVC_INST_INFO MDTM_SVC_UNINST_INFO;

typedef struct mcm_svc_up_info {
	PW_ENV_ID pwe_id;
	MDS_SVC_ID svc_id;
	V_DEST_RL role;
	NCSMDS_SCOPE_TYPE scope;
	MDS_VDEST_ID vdest;
	NCS_VDEST_TYPE vdest_policy;
	MDS_DEST adest;
	NCS_BOOL my_pcon;
	MDS_SVC_HDL local_svc_hdl;
	MDS_SUBTN_REF_VAL subtn_ref_val;
	MDS_SVC_PVT_SUB_PART_VER sub_part_ver;
	MDS_SVC_ARCHWORD_TYPE arch_word;
} MCM_SVC_UP_INFO;

typedef MCM_SVC_UP_INFO MCM_SVC_DOWN_INFO;

uns32 mds_dt2c_query_adest(MDS_MDTM_QUERY_ADEST_INFO * req);

/* ******************************************** */
/* ******************************************** */
/*                  SUB_SVC_ID                  */
/* ******************************************** */
/* ******************************************** */

typedef enum {
	MDS_MEM_MCM_CB = 1,
	MDS_MEM_ACTIVE_RESULT_INFO,
	MDS_MEM_VDEST_INFO,
	MDS_MEM_PWE_INFO,
	MDS_MEM_SVC_INFO,
	MDS_MEM_SUBTN_INFO,
	MDS_MEM_SUBTN_RESULT_INFO,
	MDS_MEM_SUBTN_ACTIVE_RESULT_INFO,
	MDS_MEM_AWAIT_ACTIVE_QUEUE_INFO,
	MDS_MEM_CALLBACK_INFO,
	MDS_DIRECT_BUFF_AL,
	MDS_MEM_TMR_INFO,
	MDS_MEM_MBX_EVT_INFO,

	MDS_MEM_DISC_QUEUE,
	MDS_MEM_SYNC_SEND_QUEUE,
	MDS_MEM_DIRECT_BUFF,
	MDS_MEM_AWAIT_ACTIVE,
	MDS_MEM_MSGELEM,
	MDS_MEM_ADEST_LIST,
	MDS_MEM_REASSEM_QUEUE,
	MDS_MEM_HDL_LIST,
	MDS_MEM_CACHED_EVENTS_LIST,
	MDS_MEM_BCAST_BUFF_LIST,
} MDS_MEM_SUB_ID;

/* ******************************************** */
/* ******************************************** */
/*                 Timer message                */
/* ******************************************** */
/* ******************************************** */

typedef enum {
	MDS_QUIESCED_TMR = 1,
	MDS_SUBTN_TMR,
	MDS_AWAIT_ACTIVE_TMR,
	MDS_REASSEMBLY_TMR,
	MDS_CACHED_EVENTS_TMR	/* No more used, after deleting the PCON related stuff */
} TMR_TYPE;

typedef struct mds_tmr_req_info {
	TMR_TYPE type;
	union {
		struct {
			MDS_VDEST_ID vdest_id;
		} quiesced_tmr_info;

		struct {
			MDS_SVC_HDL svc_hdl;
			MDS_SVC_ID sub_svc_id;
		} subtn_tmr_info;

		struct {
			MDS_SVC_HDL svc_hdl;
			MDS_SVC_ID sub_svc_id;
			MDS_VDEST_ID vdest_id;
		} await_active_tmr_info;

		struct {
			uns32 seq_no;
			struct tipc_portid id;
		} reassembly_tmr_info;

		struct {
			NCSCONTEXT adest_list_entry_ptr;
		} cached_events_tmr_info;

	} info;

} MDS_TMR_REQ_INFO;

typedef enum {
	MDS_MBX_EVT_TIMER_EXPIRY,
	MDS_MBX_EVT_DESTROY,
} MDS_MBX_EVT_TYPE;

typedef struct mds_mbx_evt_info {
	void *next;

	MDS_MBX_EVT_TYPE type;

	union {
		uns32 tmr_info_hdl;	/* For use with MDS_MBX_EVT_TIMER_EXPIRY */
		NCS_SEL_OBJ destroy_ack_obj;	/* For use with MDS_MBX_EVT_DESTROY      */
	} info;
} MDS_MBX_EVT_INFO;

#define m_MMGR_ALLOC_TMR_INFO (MDS_TMR_REQ_INFO *)m_NCS_MEM_ALLOC \
                                        (sizeof(MDS_TMR_REQ_INFO), \
                                        NCS_MEM_REGION_TRANSIENT, \
                                        NCS_SERVICE_ID_MDS, MDS_MEM_TMR_INFO)

#define m_MMGR_FREE_TMR_INFO(p)   m_NCS_MEM_FREE(p, NCS_MEM_REGION_TRANSIENT, \
                                    NCS_SERVICE_ID_MDS, MDS_MEM_TMR_INFO)

#define m_MMGR_ALLOC_MBX_EVT_INFO (MDS_MBX_EVT_INFO *)m_NCS_MEM_ALLOC \
                                        (sizeof(MDS_MBX_EVT_INFO), \
                                        NCS_MEM_REGION_TRANSIENT, \
                                        NCS_SERVICE_ID_MDS, MDS_MEM_MBX_EVT_INFO)

#define m_MMGR_FREE_MBX_EVT_INFO(p)   m_NCS_MEM_FREE(p, NCS_MEM_REGION_TRANSIENT, \
                                    NCS_SERVICE_ID_MDS, MDS_MEM_MBX_EVT_INFO)

/* ******************************************** */
/* ******************************************** */
/*        Default Timer Values Macros           */
/* ******************************************** */
/* ******************************************** */

/* Initialize Logging */
extern uns32 mds_log_init(char *log_file_name, char *line_prefix);

/* extern gl_mds_checksum */
extern uns32 gl_mds_checksum;

/* ******************************************** */
/* ******************************************** */
/*                MCM to MDTM                   */
/* ******************************************** */
/* ******************************************** */

/* Initialization of MDTM Module */
extern uns32 mds_mdtm_init(NODE_ID node_id, uns32 *mds_tipc_ref);

/* Destroying the MDTM Module*/
extern uns32 mds_mdtm_destroy(void);

extern uns32 mds_mdtm_send(MDTM_SEND_REQ *req);

/* SVC Install */
extern uns32 mds_mdtm_svc_install(PW_ENV_ID pwe_id, MDS_SVC_ID svc_id, NCSMDS_SCOPE_TYPE install_scope,
				  V_DEST_RL role, MDS_VDEST_ID vdest_id, NCS_VDEST_TYPE vdest_policy,
				  MDS_SVC_PVT_SUB_PART_VER mds_svc_pvt_ver);

/* SVC Uninstall */
extern uns32 mds_mdtm_svc_uninstall(PW_ENV_ID pwe_id, MDS_SVC_ID svc_id, NCSMDS_SCOPE_TYPE install_scope,
				    V_DEST_RL role, MDS_VDEST_ID vdest_id, NCS_VDEST_TYPE vdest_policy,
				    MDS_SVC_PVT_SUB_PART_VER mds_svc_pvt_ver);

/* SVC Subscribe */
extern uns32 mds_mdtm_svc_subscribe(PW_ENV_ID pwe_id, MDS_SVC_ID svc_id, NCSMDS_SCOPE_TYPE subscribe_scope,
				    MDS_SVC_HDL local_svc_hdl, MDS_SUBTN_REF_VAL *subtn_ref_val);

/*  added svc_hdl */
/* SVC Unsubscribe */
extern uns32 mds_mdtm_svc_unsubscribe(PW_ENV_ID pwe_id, MDS_SVC_ID svc_id, 
			NCSMDS_SCOPE_TYPE subscribe_scope, MDS_SUBTN_REF_VAL subtn_ref_val);

/* VDEST Install */
extern uns32 mds_mdtm_vdest_install(MDS_VDEST_ID vdest_id);

/* VDEST Uninstall */
extern uns32 mds_mdtm_vdest_uninstall(MDS_VDEST_ID vdest_id);

/* VDEST Subscribe */
extern uns32 mds_mdtm_vdest_subscribe(MDS_VDEST_ID vdest_id, MDS_SUBTN_REF_VAL *subtn_ref_val);

/* VDEST Unsubscribe */
extern uns32 mds_mdtm_vdest_unsubscribe(MDS_VDEST_ID vdest_id, MDS_SUBTN_REF_VAL subtn_ref_val);

/* Tx Register (For incrementing the use count) */
extern uns32 mds_mdtm_tx_hdl_register(MDS_DEST adest);

/* Tx Unregister (For decrementing the use count) */
extern uns32 mds_mdtm_tx_hdl_unregister(MDS_DEST adest);

/* Node subscription */
extern uns32 mds_mdtm_node_subscribe(MDS_SVC_HDL svc_hdl, MDS_SUBTN_REF_VAL *subtn_ref_val);

/* Node unsubscription */
extern uns32 mds_mdtm_node_unsubscribe(MDS_SUBTN_REF_VAL subtn_ref_val);

/* ******************************************** */
/* ******************************************** */
/*                MDTM to MCM                   */
/* ******************************************** */
/* ******************************************** */

/* Data receive from mdtm layer*/
extern uns32 mds_mcm_ll_data_rcv(MDS_DATA_RECV *recv);

/* SVC UP */
extern uns32 mds_mcm_svc_up(PW_ENV_ID pwe_id, MDS_SVC_ID svc_id, V_DEST_RL role,
			    NCSMDS_SCOPE_TYPE scope, MDS_VDEST_ID vdest,
			    NCS_VDEST_TYPE vdest_policy, MDS_DEST adest, NCS_BOOL my_pcon,
			    MDS_SVC_HDL local_svc_hdl, MDS_SUBTN_REF_VAL subtn_ref_val,
			    MDS_SVC_PVT_SUB_PART_VER svc_sub_part_ver, MDS_SVC_ARCHWORD_TYPE archword_type);

/* SVC DOWN */
extern uns32 mds_mcm_svc_down(PW_ENV_ID pwe_id, MDS_SVC_ID svc_id, V_DEST_RL role,
			      NCSMDS_SCOPE_TYPE scope, MDS_VDEST_ID vdest,
			      NCS_VDEST_TYPE vdest_policy, MDS_DEST adest, NCS_BOOL my_pcon,
			      MDS_SVC_HDL local_svc_hdl, MDS_SUBTN_REF_VAL subtn_ref_val,
			      MDS_SVC_PVT_SUB_PART_VER svc_sub_part_ver, MDS_SVC_ARCHWORD_TYPE archword_type);

/* NODE UP */
extern uns32 mds_mcm_node_up(MDS_SVC_HDL local_svc_hdl, NODE_ID node_id);

/* NODE DOWN */
extern uns32 mds_mcm_node_down(MDS_SVC_HDL local_svc_hdl, NODE_ID node_id);

/* VDEST UP */
extern uns32 mds_mcm_vdest_up(MDS_VDEST_ID vdest_id, MDS_DEST adest);

		  /* VDEST DOWN */	/* Presently Discarded */
extern uns32 mds_mcm_vdest_down(MDS_VDEST_ID vdest_id, MDS_DEST adest);

/* Timer expiry functions of MCM */

/* Quiesced timer expiry */
extern uns32 mds_mcm_quiesced_tmr_expiry(MDS_VDEST_ID vdest_id);

/* Subscription timer expiry */
extern uns32 mds_mcm_subscription_tmr_expiry(MDS_SVC_HDL svc_hdl, MDS_SVC_ID sub_svc_id);

/* Await Active timer expiry */
extern uns32 mds_mcm_await_active_tmr_expiry(MDS_SVC_HDL svc_hdl, MDS_SVC_ID sub_svc_id, MDS_VDEST_ID vdest_id);

/* MDS Destroy event */
extern uns32 mds_destroy_event(NCS_SEL_OBJ destroy_ack_obj);
/* Timer callback event */
extern uns32 mds_tmr_callback(NCSCONTEXT hdl);

/* for pcon check for bcast send*/
extern uns32 mdtm_check_pcon(MDS_DEST adest);

/* SVC TABLE Operations */
extern uns32 mds_svc_tbl_get(MDS_PWE_HDL pwe_hdl, MDS_SVC_ID svc_id, NCSCONTEXT *svc_cb);
extern uns32 mds_svc_tbl_get_role(MDS_SVC_HDL svc_hdl);	/*  returns 0 or 1 */
extern uns32 mds_svc_tbl_get_svc_hdl(MDS_PWE_HDL pwe_hdl, MDS_SVC_ID svc_id, MDS_SVC_HDL *svc_hdl);
/* AWAIT ACTIVE TABLE Operations */
extern uns32 mds_await_active_tbl_send(MDS_AWAIT_ACTIVE_QUEUE *queue, MDS_DEST adest);
extern uns32 mds_await_active_tbl_del(MDS_AWAIT_ACTIVE_QUEUE *queue);

/* Adding Subscription */
extern uns32 mds_mcm_subtn_add(MDS_SVC_HDL svc_hdl, MDS_SVC_ID subscr_svc_id, NCSMDS_SCOPE_TYPE scope,
			       MDS_VIEW view, MDS_SUBTN_TYPE subtn_type);

#define m_MDS_GET_PWE_HDL_FROM_VDEST_HDL_AND_PWE_ID(vdest_hdl,pwe_id) ((uns32)pwe_id << 16 | (uns32) vdest_hdl)
#define m_MDS_GET_SVC_HDL_FROM_PWE_ID_VDEST_ID_AND_SVC_ID(pwe_id,vdest_id,svc_id) (((uns64)pwe_id << 48) | ((uns64)vdest_id << 32) | ((uns64)svc_id))

/* Lock Macros */
#define m_MDS_UNLOCK(p,q) m_NCS_UNLOCK(p,q)
#define m_MDS_LOCK(p,q) m_NCS_LOCK(p,q)

/* Get VDEST id from MDSDEST INTERNAL */
extern MDS_VDEST_ID ncs_get_internal_vdest_id_from_mds_dest(MDS_DEST mdsdest);
#define m_MDS_GET_INTERNAL_VDEST_ID_FROM_MDS_DEST(mdsdest) (ncs_get_internal_vdest_id_from_mds_dest(mdsdest))

/* Macros to Get NODE_ID and PROCESS_ID from ADEST */
#define m_MDS_GET_NODE_ID_FROM_ADEST(adest) (NODE_ID) ((uns64)adest >> 32)
#define m_MDS_GET_PROCESS_ID_FROM_ADEST(adest) (uns32) ((uns64)adest & 0x00000000ffffffff)

/* Macros to get SVC_ID from SVC_HDL */
#define m_MDS_GET_SVC_ID_FROM_SVC_HDL(svc_hdl) (MDS_SVC_ID)(((uns64)svc_hdl & 0x00000000ffffffff))

/* for defining the MDS internal return values */
typedef enum {
	MDS_INT_RC_MIN = 400,
	MDS_INT_RC_DIRECT_SEND_FAIL,
	/* Add some more here , Fix me */
} MDS_INT_RETURN_TYPES;

#endif
