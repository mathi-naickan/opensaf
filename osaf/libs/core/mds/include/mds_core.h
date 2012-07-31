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

  DESCRIPTION:  MCM APIs

******************************************************************************
*/

#ifndef _MDS_CORE_H
#define _MDS_CORE_H

#include <ncsgl_defs.h>
#include "mds_papi.h"
#include "mds_dt2c.h"
#include "mds_adm.h"
#include "ncssysf_tmr.h"
#include "ncssysf_mem.h"
#include "ncspatricia.h"

/* Declarations private to MDS Core module - Vishal */
typedef uint32_t MDS_SYNC_TXN_ID;

/**************************************\
    MDS general internal structures
\**************************************/
typedef struct mds_sync_snd_ctxt_info {
	/* N O T E :  The amount of informatio in this structure
	   influences the value of MDS_SYNC_SND_CTXT_LEN_MAX used
	   in "mds_papi.h"
	 */

	/* All of the following information pertains to the original sender.
	   We put explicit check to prevent a response going to the wrong
	   destination.

	   N O T E:  We however do not check whether it is the original receiver
	   that is responding or not. Not doing that check, allows a 3-hop
	   send-response which is used by some services like the MQSv
	 */
	MDS_SYNC_TXN_ID txn_id;	/* Adds 4 bytes to MDS_SYNC_SND_CTXT_LEN_MAX */
	MDS_DEST adest;		/* Adds 8 bytes to MDS_SYNC_SND_CTXT_LEN_MAX */

} MDS_SYNC_SND_CTXT_INFO;

/* Data structures common for both MCM and MDTM */
struct mds_pwe_info;

typedef struct mds_subscription_results_key {
	MDS_SVC_HDL svc_hdl;
	MDS_SVC_ID sub_svc_id;	/* Subscribed service-id */
	MDS_VDEST_ID vdest_id;	/* The destination(s) on which it is installed */
	MDS_DEST adest;		/* Absolute qualifier ! */
} MDS_SUBSCRIPTION_RESULTS_KEY;

typedef struct mds_await_disc_queue {
	MDS_SENDTYPES send_type;
	MDS_VDEST_ID vdest;
	MDS_DEST adest;		/* For redundant sends and responses */
	NCS_SEL_OBJ sel_obj;	/* Sender waits on this object */

	struct mds_await_disc_queue *next_msg;
} MDS_AWAIT_DISC_QUEUE;

typedef struct mds_mcm_sync_send_queue {
	uint8_t msg_snd_type;	/* Type of send if this is just ack, no data is searched on */
	MDS_SYNC_TXN_ID txn_id;	/* A Key : Looked up when response received */
	NCS_SEL_OBJ sel_obj;	/* Raised when a response is received */
	uint32_t status;		/* Result sent by remote if any */
	MDS_ENCODED_MSG recvd_msg;
	NCSCONTEXT orig_msg;	/* To supply to enc, enc-flat callback to allow
				   a user to supply the response inlined in the
				   original message.
				 */
	NCSCONTEXT sent_msg;	/* FIXME: Change name to recvd_msg_ready */

	union {
		MDS_DEST adest;
	} dest_sndrack_adest;	/*  Filled, when the sndrack and redrack is being sent to originator and checked when ack
				   is recd for the sent sndrack or redrack */

	struct mds_mcm_sync_send_queue *next_send;
	MDS_CLIENT_MSG_FORMAT_VER msg_fmt_ver;

} MDS_MCM_SYNC_SEND_QUEUE;

typedef struct mds_active_result_info {
	/* Information to determine next active recipient */
	struct mds_subscription_results_info *next_active_in_turn;	/* Who's the next active */
	bool dest_is_n_way;	/* If yes, we need to rotate act_adest */
	uint32_t act_send_count;	/* Updated when act_dest used to send */

	/* Info to maintain await-active queue, etc. */
	MDS_TMR_REQ_INFO *tmr_req_info;
	uint32_t tmr_req_info_hdl;
	bool tmr_running;	/* true if timer running */
	tmr_t await_active_tmr;	/* Valid if await_active = true */
	MDS_AWAIT_ACTIVE_QUEUE *await_active_queue;
	MDS_SVC_PVT_SUB_PART_VER last_active_svc_sub_part_ver;

} MDS_ACTIVE_RESULT_INFO;

typedef struct mds_subscription_results_info {

	/* Indexing info */
	NCS_PATRICIA_NODE node;

	/* Explicit key for fast-access in the send and receive flow */
	MDS_SUBSCRIPTION_RESULTS_KEY key;

	/* Used only when deleting all subtn res info on getting ADEST down */
	NCSMDS_SCOPE_TYPE install_scope;
	union {
		/* Valid when vdest_id = m_VDEST_ID_FOR_ADEST_ENTRY */
		struct {
			uint32_t dummy;
		} adest;

		/* Valid when vdest_id != m_VDEST_ID_FOR_ADEST_ENTRY, ADEST != 0 */
		struct {
			V_DEST_RL role;
			NCS_VDEST_TYPE policy;
		} vdest_inst;

		/* Valid when vdest_id != m_VDEST_ID_FOR_ADEST_ENTRY, ADEST = 0 */
		struct {
			MDS_ACTIVE_RESULT_INFO *active_route_info;
		} active_vdest;
	} info;
	MDS_SVC_PVT_SUB_PART_VER rem_svc_sub_part_ver;
	MDS_SVC_ARCHWORD_TYPE rem_svc_arch_word;
	uint32_t  msg_snd_cnt;			/* Message send count to this destination */
	uint32_t  msg_rcv_cnt;			/* Message rcv count from this destination */

} MDS_SUBSCRIPTION_RESULTS_INFO;

/**************************************\
    MDS subscription info
\**************************************/

typedef struct mds_subscription_info {
	struct mds_subscription_info *next;

	/* Information on the subscrtipion */
	MDS_SVC_ID sub_svc_id;	/* "Uniquifier" for subscriptions */
	NCSMDS_SCOPE_TYPE scope;
	MDS_VIEW view;		/* Normal/Redundant */
	MDS_SUBTN_TYPE subtn_type;	/* Implicit subscription by MDS */

	/* Handle returned by MDTM. Required for subscription cancellation */
	MDS_SUBTN_REF_VAL subscr_req_hdl;

	/* Messages queued on the subscription being completed */
	MDS_TMR_REQ_INFO *tmr_req_info;
	uint32_t tmr_req_info_hdl;
	bool tmr_flag;	/* Flag = Y/N */
	tmr_t discovery_tmr;	/* Timer Cb */
	MDS_AWAIT_DISC_QUEUE *await_disc_queue;	/* Msg + Svc_hdl */

} MDS_SUBSCRIPTION_INFO;

/*********************************************\
    MDS CLIENT SERVICE related declarations
\*********************************************/
#define m_GET_HDL_FROM_MDS_SVC_INFO(info)                   \
                ((info->svc-id)<<32 ||                      \
                 (info->parent_pwe->pwe_id)<<16 ||          \
                 (info->parent_pwe->parent_vdest->vdest-id)))

#define m_GET_HDL_FROM_MDS_SVC_PWE_VDEST(s,p,v) \
                ((s)<<32 ||                     \
                 (p)<<16 ||                     \
                 (v)))

/**************************************\
    MDS PWE related declarations
\**************************************/
/* typedef uint32_t MDS_PWE_HDL; */
#define m_GET_HDL_FROM_MDS_PWE_INFO(info) (info->pwe_id)
typedef struct mds_pwe_info {
	/* Indexing info */
	struct mds_pwe_info *next_pwe;

	/* Information */
	PW_ENV_ID pwe_id;

	/* Back pointers */
	struct mds_vdest_info *parent_vdest;
} MDS_PWE_INFO;

/**************************************\
    MDS VDEST related declarations
\**************************************/

#define m_GET_HDL_FROM_MDS_VDEST_FRM_SVC_INFO(info) (info->parant_pwe->parent_vdest)
typedef struct mds_vdest_info {

	/* Indexing info */
	NCS_PATRICIA_NODE node;

	/* Vdest info */
	MDS_VDEST_ID vdest_id;	/* Serves as VDEST hdl. Key for Patricia node */
	MDS_SUBTN_REF_VAL subtn_ref_val;
	NCS_VDEST_TYPE policy;
	V_DEST_RL role;
	MDS_TMR_REQ_INFO *tmr_req_info;
	uint32_t tmr_req_info_hdl;
	bool tmr_running;
	tmr_t quiesced_cbk_tmr;
	/* PWE and service list */
	MDS_PWE_INFO *pwe_list;

} MDS_VDEST_INFO;

typedef struct mds_svc_info {

	/* Indexing info */
	NCS_PATRICIA_NODE svc_list_node;

	/* Explicit-key to this struct for fast access in the receive flow */
	MDS_SVC_HDL svc_hdl;

	/* Information */
	uint16_t svc_id;		/* Client service id */
	NCSMDS_SCOPE_TYPE install_scope;
	NCSMDS_CALLBACK_API cback_ptr;	/* Client's callback pointer */
	MDS_CLIENT_HDL yr_svc_hdl;	/* Client's context handle */
	bool q_ownership;	/* true implies MDS owned queue */
	SYSF_MBX q_mbx;
	uint32_t seq_no;
	MDS_VDEST_INFO *parent_vdest_info;
	/* List of subscriptions made by this service */
	MDS_SUBSCRIPTION_INFO *subtn_info;

	/* List of senders blocked on synchronous send operation. Multiple
	   senders can be blocked because MDS does not prohibit multiple threads
	   to simultaneously send using the same service instance (<pwe_hdl, svc-id>
	   combination.
	 */
	MDS_MCM_SYNC_SEND_QUEUE *sync_send_queue;
	uint8_t sync_count;
	MDS_SVC_PVT_SUB_PART_VER svc_sub_part_ver;
	bool i_fail_no_active_sends;	/* Default messages will be buufered in MDS when destination is
						   in No-Active state, else dropped */
	bool i_node_subscr;	/* suscription to node */
	MDS_SUBTN_REF_VAL node_subtn_ref_val;  
	bool i_msg_loss_indication;	/* message loss indication if set to 1 */
} MDS_SVC_INFO;

MDS_SVC_INFO *mds_get_svc_info_by_hdl(MDS_SVC_HDL hdl);

typedef struct mds_mcm_cb {
	MDS_DEST adest;

	/* List of all subscription results MDS_SUBSCRIPTION_RESULTS_INFO */
	NCS_PATRICIA_TREE subtn_results;
	NCS_PATRICIA_TREE svc_list;	/* Tree of MDS_SVC_INFO information */
	NCS_PATRICIA_TREE vdest_list;	/* Tree of MDS_VDEST_INFO information */
} MDS_MCM_CB;

/* Global MDSCB */
extern MDS_MCM_CB *gl_mds_mcm_cb;

/* Global TMR Values */
extern uint32_t MDS_QUIESCED_TMR_VAL;
extern uint32_t MDS_AWAIT_ACTIVE_TMR_VAL;
extern uint32_t MDS_SUBSCRIPTION_TMR_VAL;

/* Global gl_mds_checksum */
extern uint32_t gl_mds_checksum;

/* ******************************************** */
/* ******************************************** */
/*                MCM Private                   */
/* ******************************************** */
/* ******************************************** */
extern uint32_t mds_mcm_init(void);

extern uint32_t mds_mcm_destroy(void);

extern uint32_t mds_mcm_cleanup(void);

extern uint32_t mds_mcm_vdest_create(NCSMDS_ADMOP_INFO *info);

extern uint32_t mds_mcm_vdest_destroy(NCSMDS_ADMOP_INFO *info);

extern uint32_t mds_mcm_vdest_query(NCSMDS_ADMOP_INFO *info);

extern uint32_t mds_mcm_vdest_chg_role(NCSMDS_ADMOP_INFO *info);

extern uint32_t mds_mcm_pwe_create(NCSMDS_ADMOP_INFO *info);

extern uint32_t mds_mcm_pwe_destroy(NCSMDS_ADMOP_INFO *info);

extern uint32_t mds_mcm_adm_pwe_query(NCSMDS_ADMOP_INFO *info);

extern uint32_t mds_mcm_svc_install(NCSMDS_INFO *info);

extern uint32_t mds_mcm_svc_uninstall(NCSMDS_INFO *info);

extern uint32_t mds_mcm_svc_subscribe(NCSMDS_INFO *info);

extern uint32_t mds_mcm_svc_unsubscribe(NCSMDS_INFO *info);

extern uint32_t mds_send(NCSMDS_INFO *info);

extern uint32_t mds_retrieve(NCSMDS_INFO *info);

extern uint32_t mds_mcm_dest_query(NCSMDS_INFO *info);

extern uint32_t mds_mcm_pwe_query(NCSMDS_INFO *info);

extern uint32_t mds_mcm_node_subscribe(NCSMDS_INFO *info);

extern uint32_t mds_mcm_node_unsubscribe(NCSMDS_INFO *info);


/* Note in case of the DOWN, RED-DOWN and NO-ACTIVE callbacks to the user, archword provided will be unspecified
     and will be as follows */
#define  MDS_SVC_ARCHWORD_TYPE_UNSPECIFIED   0xff
/* User event callback */
extern uint32_t mds_mcm_user_event_callback(MDS_SVC_HDL local_svc_hdl, PW_ENV_ID pwe_id, MDS_SVC_ID svc_id,
					 V_DEST_RL role, MDS_VDEST_ID vdest_id, MDS_DEST adest,
					 NCSMDS_CHG event_type,
					 MDS_SVC_PVT_SUB_PART_VER svc_sub_part_ver,
					 MDS_SVC_ARCHWORD_TYPE archword_type);

extern uint32_t mds_validate_pwe_hdl(MDS_PWE_HDL pwe_hdl);

/* ******************************************** */
/* ******************************************** */
/*               TABLE Operations               */
/* ******************************************** */
/* ******************************************** */

/* VDEST TABLE Operations */

extern uint32_t mds_vdest_tbl_add(MDS_VDEST_ID vdest_id, NCS_VDEST_TYPE policy, MDS_VDEST_HDL *vdest_hdl);
extern uint32_t mds_vdest_tbl_del(MDS_VDEST_ID vdest_id);
extern uint32_t mds_vdest_tbl_update_role(MDS_VDEST_ID vdest_id, V_DEST_RL role, bool del_tmr_info);
extern uint32_t mds_vdest_tbl_update_ref_val(MDS_VDEST_ID vdest_id, MDS_SUBTN_REF_VAL subtn_ref_val);
extern uint32_t mds_vdest_tbl_query(MDS_VDEST_ID vdest_id);
extern uint32_t mds_vdest_tbl_get_role(MDS_VDEST_ID vdest_id, V_DEST_RL *role);
extern uint32_t mds_vdest_tbl_get_policy(MDS_VDEST_ID vdest_id, NCS_VDEST_TYPE *policy);
extern uint32_t mds_vdest_tbl_get_first(MDS_VDEST_ID vdest_id, MDS_PWE_HDL *first_pwe_hdl);
extern uint32_t mds_vdest_tbl_get_vdest_hdl(MDS_VDEST_ID vdest_id, MDS_VDEST_HDL *vdest_hdl);
extern uint32_t mds_vdest_tbl_get_subtn_ref_val(MDS_VDEST_ID vdest_id, MDS_SUBTN_REF_VAL *subtn_ref_ptr);
extern uint32_t mds_vdest_tbl_get_vdest_info_cb(MDS_VDEST_ID vdest_id, MDS_VDEST_INFO **vdest_info);
extern uint32_t mds_vdest_tbl_cleanup(void);

/* PWE TABLE Operations */

extern uint32_t mds_pwe_tbl_add(MDS_VDEST_HDL vdest_hdl, PW_ENV_ID pwe_id, MDS_PWE_HDL *pwe_hdl);
extern uint32_t mds_pwe_tbl_del(MDS_PWE_HDL pwe_hdl);	/* mds_hdl is stored as back ptr in PWE_INFO */
extern uint32_t mds_pwe_tbl_query(MDS_VDEST_HDL vdest_hdl, PW_ENV_ID pwe_id);

/* SVC TABLE Operations */
extern uint32_t mds_svc_tbl_add(NCSMDS_INFO *info);
extern uint32_t mds_svc_tbl_del(MDS_PWE_HDL pwe_hdl, MDS_SVC_ID svc_id, MDS_Q_MSG_FREE_CB msg_free_cb);
extern uint32_t mds_svc_tbl_query(MDS_PWE_HDL pwe_hdl, MDS_SVC_ID svc_id);
extern uint32_t mds_svc_tbl_get(MDS_PWE_HDL pwe_hdl, MDS_SVC_ID svc_id, NCSCONTEXT *svc_cb);
extern uint32_t mds_svc_tbl_get_role(MDS_SVC_HDL svc_hdl);	/*  returns 0 or 1 */
extern uint32_t mds_svc_tbl_get_install_scope(MDS_PWE_HDL pwe_hdl, MDS_SVC_ID svc_id, NCSMDS_SCOPE_TYPE *install_scope);
extern uint32_t mds_svc_tbl_get_svc_hdl(MDS_PWE_HDL pwe_hdl, MDS_SVC_ID svc_id, MDS_SVC_HDL *svc_hdl);
extern uint32_t mds_svc_tbl_get_first_subscription(MDS_SVC_HDL svc_hdl, MDS_SUBSCRIPTION_INFO **first_subscription);

extern uint32_t mds_svc_tbl_getnext_on_vdest(MDS_VDEST_ID vdest_id, MDS_SVC_HDL current_svc_hdl, MDS_SVC_INFO **svc_info);
extern uint32_t mds_svc_tbl_cleanup(void);

/* SUBTN TABLE Operations */
extern uint32_t mds_subtn_tbl_add(MDS_SVC_HDL svc_hdl, MDS_SVC_ID subscr_svc_id, NCSMDS_SCOPE_TYPE scope,
			       MDS_VIEW view, MDS_SUBTN_TYPE subtn_type);
extern uint32_t mds_subtn_tbl_del(MDS_SVC_HDL svc_hdl, uint32_t subscr_svc_id);
extern uint32_t mds_subtn_tbl_change_explicit(MDS_SVC_HDL svc_hdl, MDS_SVC_ID subscr_svc_id, MDS_VIEW subtn_view_type);
extern uint32_t mds_subtn_tbl_query(MDS_SVC_HDL svc_hdl, MDS_SVC_ID subscr_svc_id);
extern uint32_t mds_subtn_tbl_update_ref_hdl(MDS_SVC_HDL svc_hdl, MDS_SVC_ID subscr_svc_id,
					  MDS_SUBTN_REF_VAL subscr_req_hdl);

extern uint32_t mds_subtn_tbl_get(MDS_SVC_HDL svc_hdl, MDS_SVC_ID subscr_svc_id, MDS_SUBSCRIPTION_INFO **result);

extern uint32_t mds_subtn_tbl_get_details(MDS_SVC_HDL svc_hdl, MDS_SVC_ID subscr_svc_id,
				       NCSMDS_SCOPE_TYPE *scope, MDS_VIEW *view);

extern uint32_t mds_subtn_tbl_get_ref_hdl(MDS_SVC_HDL svc_hdl, MDS_SVC_ID subscr_svc_id,
				       MDS_SUBTN_REF_VAL *subscr_ref_hdl, NCSMDS_SCOPE_TYPE *scope);

/* BLOCK SEND REQ TABLE Operations */

extern uint32_t mds_block_snd_req_tbl_add(MDS_SVC_HDL svc_hdl, MDS_SYNC_TXN_ID txn_id, MDS_MCM_SYNC_SEND_QUEUE *result);
extern uint32_t mds_block_snd_req_tbl_del(MDS_SVC_HDL svc_hdl, MDS_SYNC_TXN_ID txn_id);
extern uint32_t mds_block_snd_req_tbl_query(MDS_SVC_HDL svc_hdl, MDS_SYNC_TXN_ID txn_id);
extern uint32_t mds_block_snd_req_tbl_get(MDS_SVC_HDL svc_hdl, MDS_SYNC_TXN_ID txn_id, MDS_MCM_SYNC_SEND_QUEUE *result);
extern uint32_t mds_block_snd_req_tbl_getnext();

/* SUBTN RESULT TABLE Operations */

extern uint32_t mds_subtn_res_tbl_add(MDS_SVC_HDL svc_hdl, MDS_SVC_ID subscr_svc_id,
				   MDS_VDEST_ID vdest_id, MDS_DEST adest, V_DEST_RL role,
				   NCSMDS_SCOPE_TYPE scope,
				   NCS_VDEST_TYPE local_vdest_policy,
				   MDS_SVC_PVT_SUB_PART_VER svc_sub_part_ver, MDS_SVC_ARCHWORD_TYPE archword_type);
extern uint32_t mds_subtn_res_tbl_del(MDS_SVC_HDL svc_hdl, MDS_SVC_ID svc_id,
				   MDS_VDEST_ID vdest_id, MDS_DEST adest,
				   NCS_VDEST_TYPE vdest_policy,
				   MDS_SVC_PVT_SUB_PART_VER svc_sub_part_ver, MDS_SVC_ARCHWORD_TYPE archword_type);
extern uint32_t mds_subtn_res_tbl_query(MDS_SVC_HDL svc_hdl, MDS_SVC_ID subscr_svc_id, MDS_VDEST_ID vdest_id);
extern uint32_t mds_subtn_res_tbl_query_by_adest(MDS_SVC_HDL svc_hdl, MDS_SVC_ID subscr_svc_id,
			      MDS_VDEST_ID vdest_id, MDS_DEST adest);
extern uint32_t mds_get_subtn_res_tbl_by_adest(MDS_SVC_HDL svc_hdl, MDS_SVC_ID subscr_svc_id,
		       MDS_VDEST_ID vdest_id, MDS_DEST adest, MDS_SUBSCRIPTION_RESULTS_INFO **result);
extern uint32_t mds_subtn_res_tbl_change_active(MDS_SVC_HDL svc_hdl, MDS_SVC_ID subscr_svc_id,
					     MDS_VDEST_ID vdest_id, MDS_SUBSCRIPTION_RESULTS_INFO *active_result,
					     MDS_SVC_PVT_SUB_PART_VER svc_sub_part_ver,
					     MDS_SVC_ARCHWORD_TYPE archword_type);
extern uint32_t mds_subtn_res_tbl_remove_active(MDS_SVC_HDL svc_hdl, MDS_SVC_ID subscr_svc_id, MDS_VDEST_ID vdest_id);
extern uint32_t mds_subtn_res_tbl_add_active(MDS_SVC_HDL svc_hdl, MDS_SVC_ID subscr_svc_id,
					  MDS_VDEST_ID vdest_id, NCS_VDEST_TYPE vdest_policy,
					  MDS_SUBSCRIPTION_RESULTS_INFO *active_result,
					  MDS_SVC_PVT_SUB_PART_VER svc_sub_part_ver,
					  MDS_SVC_ARCHWORD_TYPE archword_type);

extern uint32_t mds_subtn_res_tbl_change_role(MDS_SVC_HDL svc_hdl, MDS_SVC_ID subscr_svc_id,
					   MDS_VDEST_ID vdest_id, MDS_DEST adest, V_DEST_RL role);

extern uint32_t mds_subtn_res_tbl_get(MDS_SVC_HDL svc_hdl, MDS_SVC_ID subscr_svc_id,
				   MDS_VDEST_ID vdest_id, MDS_DEST *adest, bool *tmr_running,
				   MDS_SUBSCRIPTION_RESULTS_INFO **result,
				   bool call_ref_flag /* True for internal call False otherwise */ );

extern uint32_t mds_subtn_res_tbl_get_by_adest(MDS_SVC_HDL svc_hdl, MDS_SVC_ID subscr_svc_id, MDS_VDEST_ID vdest_id, MDS_DEST adest, V_DEST_RL *o_role, MDS_SUBSCRIPTION_RESULTS_INFO **result);	/* Use for send */

extern uint32_t mds_subtn_res_tbl_getnext_by_adest(MDS_DEST adest, MDS_SUBSCRIPTION_RESULTS_KEY *key,
						MDS_SUBSCRIPTION_RESULTS_INFO **ret_result);
				/* used while deleting results when ADEST goes down */

extern uint32_t mds_subtn_res_tbl_getnext_active(MDS_SVC_HDL svc_hdl, MDS_SVC_ID subscr_svc_id, MDS_SUBSCRIPTION_RESULTS_INFO **result);	/* use for bcast */

extern uint32_t mds_subtn_res_tbl_getnext_any(MDS_SVC_HDL svc_hdl, MDS_SVC_ID subscr_svc_id, MDS_SUBSCRIPTION_RESULTS_INFO **result);	/* use for red bcast */

extern uint32_t mds_subtn_res_tbl_query_next_active(MDS_SVC_HDL svc_hdl, MDS_SVC_ID sub_svc_id,
						 MDS_VDEST_ID vdest_id,
						 MDS_SUBSCRIPTION_RESULTS_INFO *current_active_result,
						 MDS_SUBSCRIPTION_RESULTS_INFO **next_active_result);
				    /* called only when vdest in N-Way */

extern uint32_t mds_subtn_res_tbl_del_all(MDS_SVC_HDL svc_hdl, MDS_SVC_ID sub_svc_id);

extern uint32_t mds_subtn_res_tbl_cleanup(void);

/* For scope validation while getting svc up/down */
extern uint32_t mds_mcm_validate_scope(NCSMDS_SCOPE_TYPE local_scope, NCSMDS_SCOPE_TYPE remote_scope,
				    MDS_DEST remote_adest, MDS_SVC_ID remote_svc_id, bool my_pcon);

extern uint32_t mds_mcm_free_msg_uba_start(MDS_ENCODED_MSG msg);

/* ******************************************** */
/* ******************************************** */
/*                 MMGR Macros                  */
/* ******************************************** */
/* ******************************************** */

/* SUB_SVC_IDs are defined in an enum in mds_dt2c.h */

#define m_MMGR_ALLOC_MCM_CB        (MDS_MCM_CB *)m_NCS_MEM_ALLOC(sizeof(MDS_MCM_CB), \
                                    NCS_MEM_REGION_TRANSIENT, \
                                    NCS_SERVICE_ID_MDS, MDS_MEM_MCM_CB)

#define m_MMGR_FREE_MCM_CB(p)        m_NCS_MEM_FREE(p, NCS_MEM_REGION_TRANSIENT, \
                                    NCS_SERVICE_ID_MDS, MDS_MEM_MCM_CB)

#define m_MMGR_ALLOC_MDS_ACTIVE_RESULT_INFO   \
    (MDS_ACTIVE_RESULT_INFO *)m_NCS_MEM_ALLOC(sizeof(MDS_ACTIVE_RESULT_INFO), \
                                    NCS_MEM_REGION_TRANSIENT, \
                                    NCS_SERVICE_ID_MDS, MDS_MEM_ACTIVE_RESULT_INFO)

#define m_MMGR_FREE_MDS_ACTIVE_RESULT_INFO      \
    m_NCS_MEM_FREE(p, NCS_MEM_REGION_TRANSIENT, \
                                    NCS_SERVICE_ID_MDS, MDS_MEM_ACTIVE_RESULT_INFO)

#define m_MMGR_ALLOC_VDEST_INFO     (MDS_VDEST_INFO *)m_NCS_MEM_ALLOC(sizeof(MDS_VDEST_INFO), \
                                    NCS_MEM_REGION_TRANSIENT, \
                                    NCS_SERVICE_ID_MDS, MDS_MEM_VDEST_INFO)

#define m_MMGR_FREE_VDEST_INFO(p)   m_NCS_MEM_FREE(p, NCS_MEM_REGION_TRANSIENT, \
                                    NCS_SERVICE_ID_MDS, MDS_MEM_VDEST_INFO)

#define m_MMGR_ALLOC_PWE_INFO     (MDS_PWE_INFO *)m_NCS_MEM_ALLOC(sizeof(MDS_PWE_INFO), \
                                    NCS_MEM_REGION_TRANSIENT, \
                                    NCS_SERVICE_ID_MDS, MDS_MEM_PWE_INFO)

#define m_MMGR_FREE_PWE_INFO(p)   m_NCS_MEM_FREE(p, NCS_MEM_REGION_TRANSIENT, \
                                    NCS_SERVICE_ID_MDS, MDS_MEM_PWE_INFO)

#define m_MMGR_ALLOC_SVC_INFO     (MDS_SVC_INFO *)m_NCS_MEM_ALLOC(sizeof(MDS_SVC_INFO), \
                                    NCS_MEM_REGION_TRANSIENT, \
                                    NCS_SERVICE_ID_MDS, MDS_MEM_SVC_INFO)

#define m_MMGR_FREE_SVC_INFO(p)   m_NCS_MEM_FREE(p, NCS_MEM_REGION_TRANSIENT, \
                                    NCS_SERVICE_ID_MDS, MDS_MEM_SVC_INFO)

#define m_MMGR_ALLOC_SUBTN_INFO     (MDS_SUBSCRIPTION_INFO *)m_NCS_MEM_ALLOC \
                                        (sizeof(MDS_SUBSCRIPTION_INFO), \
                                        NCS_MEM_REGION_TRANSIENT, \
                                        NCS_SERVICE_ID_MDS, MDS_MEM_SUBTN_INFO)

#define m_MMGR_FREE_SUBTN_INFO(p)   m_NCS_MEM_FREE(p, NCS_MEM_REGION_TRANSIENT, \
                                    NCS_SERVICE_ID_MDS, MDS_MEM_SUBTN_INFO)

#define m_MMGR_ALLOC_SUBTN_RESULT_INFO     (MDS_SUBSCRIPTION_RESULTS_INFO *)m_NCS_MEM_ALLOC \
                                        (sizeof(MDS_SUBSCRIPTION_RESULTS_INFO), \
                                        NCS_MEM_REGION_TRANSIENT, \
                                        NCS_SERVICE_ID_MDS, MDS_MEM_SUBTN_RESULT_INFO)

#define m_MMGR_FREE_SUBTN_RESULT_INFO(p)   m_NCS_MEM_FREE(p, NCS_MEM_REGION_TRANSIENT, \
                                    NCS_SERVICE_ID_MDS, MDS_MEM_SUBTN_RESULT_INFO)

#define m_MMGR_ALLOC_SUBTN_ACTIVE_RESULT_INFO (MDS_ACTIVE_RESULT_INFO *)m_NCS_MEM_ALLOC \
                                        (sizeof(MDS_ACTIVE_RESULT_INFO), \
                                        NCS_MEM_REGION_TRANSIENT, \
                                        NCS_SERVICE_ID_MDS, MDS_MEM_SUBTN_ACTIVE_RESULT_INFO)

#define m_MMGR_FREE_SUBTN_ACTIVE_RESULT_INFO(p)   m_NCS_MEM_FREE(p, NCS_MEM_REGION_TRANSIENT, \
                                    NCS_SERVICE_ID_MDS, MDS_MEM_SUBTN_ACTIVE_RESULT_INFO)

#define m_MMGR_ALLOC_CALLBACK_INFO (NCSMDS_CALLBACK_INFO *)m_NCS_MEM_ALLOC \
                                        (sizeof(NCSMDS_CALLBACK_INFO), \
                                        NCS_MEM_REGION_TRANSIENT, \
                                        NCS_SERVICE_ID_MDS, MDS_MEM_CALLBACK_INFO)

#define m_MMGR_FREE_CALLBACK_INFO(p)   m_NCS_MEM_FREE(p, NCS_MEM_REGION_TRANSIENT, \
                                    NCS_SERVICE_ID_MDS, MDS_MEM_CALLBACK_INFO)

/* ******************************************** */
/* ******************************************** */
/*              MBX MSG Macros                  */
/* ******************************************** */
/* ******************************************** */

typedef enum {
	MDS_DATA_TYPE = 1,
	MDS_EVENT_TYPE,
} MBX_POST_TYPES;

typedef struct mds_mcm_msg_elem {
	void *next;
	MBX_POST_TYPES type;
	MDS_SEND_PRIORITY_TYPE pri;
	union {
		struct {
			NCS_IPC_MSG msg;
			MDS_ENCODED_MSG enc_msg;
			MDS_SVC_ID fr_svc_id;
			PW_ENV_ID fr_pwe_id;
			MDS_VDEST_ID fr_vdest_id;
			MDS_DEST adest;
			MDS_SENDTYPES snd_type;
			uint32_t xch_id;
			MDS_CLIENT_MSG_FORMAT_VER msg_fmt_ver;
			MDS_SVC_PVT_SUB_PART_VER src_svc_sub_part_ver;
			MDS_SVC_ARCHWORD_TYPE arch_word;
		} data;
		struct {
			NCSMDS_CALLBACK_INFO cbinfo;
		} event;
	} info;

} MDS_MCM_MSG_ELEM;

/* ******************************************** */
/* ******************************************** */
/*           MMGR Macros for Snd Rcv            */
/* ******************************************** */
/* ******************************************** */

#define m_MMGR_ALLOC_DISC_QUEUE m_NCS_MEM_ALLOC(sizeof(MDS_AWAIT_DISC_QUEUE),\
                                                           NCS_MEM_REGION_PERSISTENT,\
                                                           NCS_SERVICE_ID_MDS,\
                                                           MDS_MEM_DISC_QUEUE)

#define m_MMGR_FREE_DISC_QUEUE(p) m_NCS_MEM_FREE(p,\
                                                  NCS_MEM_REGION_PERSISTENT,\
                                                  NCS_SERVICE_ID_MDS,\
                                                  MDS_MEM_DISC_QUEUE)

#define m_MMGR_ALLOC_SYNC_SEND_QUEUE m_NCS_MEM_ALLOC(sizeof(MDS_MCM_SYNC_SEND_QUEUE),\
                                                           NCS_MEM_REGION_PERSISTENT,\
                                                           NCS_SERVICE_ID_MDS,\
                                                           MDS_MEM_SYNC_SEND_QUEUE)

#define m_MMGR_FREE_SYNC_SEND_QUEUE(p) m_NCS_MEM_FREE(p,\
                                                       NCS_MEM_REGION_PERSISTENT,\
                                                       NCS_SERVICE_ID_MDS,\
                                                       MDS_MEM_SYNC_SEND_QUEUE)
#define m_MMGR_ALLOC_MSGELEM m_NCS_MEM_ALLOC(sizeof(MDS_MCM_MSG_ELEM),\
                                                  NCS_MEM_REGION_PERSISTENT,\
                                                  NCS_SERVICE_ID_MDS,\
                                                  MDS_MEM_MSGELEM)

#define m_MMGR_FREE_MSGELEM(p) m_NCS_MEM_FREE(p,\
                                                  NCS_MEM_REGION_PERSISTENT,\
                                                  NCS_SERVICE_ID_MDS,\
                                                  MDS_MEM_MSGELEM)

#define m_MMGR_ALLOC_AWAIT_ACTIVE m_NCS_MEM_ALLOC(sizeof(MDS_AWAIT_ACTIVE_QUEUE),\
                                                  NCS_MEM_REGION_PERSISTENT,\
                                                  NCS_SERVICE_ID_MDS,\
                                                  MDS_MEM_AWAIT_ACTIVE)

#define m_MMGR_FREE_AWAIT_ACTIVE(p) m_NCS_MEM_FREE(p,\
                                                  NCS_MEM_REGION_PERSISTENT,\
                                                  NCS_SERVICE_ID_MDS,\
                                                  MDS_MEM_AWAIT_ACTIVE)

/* ******************************************** */
/* ******************************************** */
/*                  Macros                      */
/* ******************************************** */
/* ******************************************** */

#define m_MDS_GET_VDEST_ID_FROM_VDEST_HDL(hdl) (MDS_VDEST_ID)((uint32_t)hdl & 0x0000ffff)
#define m_MDS_GET_VDEST_ID_FROM_PWE_HDL(hdl) (MDS_VDEST_ID)((uint32_t)hdl & 0x0000ffff)
#define m_MDS_GET_VDEST_ID_FROM_SVC_HDL(svc_hdl) (MDS_VDEST_ID)(((uint64_t)svc_hdl >> 32) & 0x0000ffff)

#define m_MDS_GET_PWE_ID_FROM_PWE_HDL(hdl) (MDS_PWE_ID)((uint32_t)hdl>>16)
#define m_MDS_GET_PWE_ID_FROM_SVC_HDL(svc_hdl) (MDS_PWE_ID)(((uint64_t)svc_hdl >> 48))

#define m_MDS_GET_PWE_HDL_FROM_PWE_ID_AND_VDEST_ID(pwe_id,vdest_id) (((uint32_t)pwe_id << 16) | (uint32_t)vdest_id)
#define m_MDS_GET_PWE_HDL_FROM_SVC_HDL(svc_hdl) (MDS_PWE_HDL)(((uint64_t)svc_hdl >> 32))

#define m_MDS_GET_SVC_HDL_FROM_PWE_HDL_AND_SVC_ID(pwe_hdl,svc_id) (MDS_SVC_HDL)(((uint64_t)pwe_hdl << 32) | (uint64_t)svc_id)

#define m_MDS_GET_VDEST_HDL_FROM_VDEST_ID(vdest_id) ((MDS_VDEST_HDL)vdest_id)
#define m_MDS_GET_VDEST_HDL_FROM_PWE_HDL(pwe_hdl) (MDS_VDEST_HDL)((uint32_t)pwe_hdl & 0x0000ffff)

#define m_ADEST_HDL 0x0000ffff
#define m_VDEST_ID_FOR_ADEST_ENTRY 0xffff
#define m_MDS_GET_ADEST (gl_mds_mcm_cb->adest)
#define m_MDS_GET_PCON_ID (gl_mds_mcm_cb->pcon_id)

#define m_MDS_GET_ADEST_FROM_NODE_ID_AND_PROCESS_ID(node_id, process_id) (MDS_DEST)(((uint64_t)node_id << 32) | (uint64_t)process_id)

#endif
