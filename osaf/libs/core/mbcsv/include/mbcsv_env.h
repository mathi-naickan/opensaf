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

  
    
      
        REVISION HISTORY:
        
          
            DESCRIPTION:
            
              This module contains declarations and structures related to MBCSv.
              
*******************************************************************************/

/* 
   Further LOCK Notes: We should retain them as a note somewhere in code
                       
   A) Locks: (i) Global lock and (ii) service-level (i.e. per Initialize)
   B) Handles: (i) "MBCSV-HANDLE" = service-level-handle.
               (ii) "CKPT-HANDLE" = ckpt-level-handle.
               (iii) "peer-inst-handle" = peer-inst-handle.
   C) The locking code conventions need to avoid the following situations:
       (i ) Deadlock among application and MBCSv: We mandate that
            application should release its locks before invoking MDS APIs.

       (ii) Deadlock among MDS and MBCSv: MBCSv ensures that MBCSv never
            takes a lock in MDS callbacks. 

       (iii) Deadlock among MBCSv hdl-destroy and locks: When destroying
             an object (for example svc-reg structure), the handle should
             be destroyed first and then the object-lock should be taken
             (I claim taking of the object-lock is not required)

       (iv)  Deadlock possible due to a lock hierarchy: When there is 
             hierarchy of locks, deadlock should be avoided by using
             the following convention: Locks should always be taken
             in higher-to-lower direction. Therefore, while holding only
             a lower-level-lock, a higher-level-lock should not be taken.
             To do that, first the lower-level-lock should be relinquished
             first. 

   D) Race among handle-based object access and object destruction: We need 
      to ensure that an object (retrieved using a HM-handle) while in use 
      cannot be destroyed simultaneouly somewhere else. This is automatically 
      ensured by using HM-handles as follows:
        ACCESS:
           STEP 1: Take handle to get object
           STEP 2: (Optional) Take object-lock.
           STEP 3: Use the object for processing.
           STEP 4: (Depends on step 2) Release object-lock.
           STEP 5: Release object-handle.
        DESTROY:
           STEP 1: Call destroy-handle. By the time this function returns,
                   all "takes-have-been-given", and from then on nobody
                   can get access to the structure using the handle.
           STEP 2: Destroy object-lock and object as well.

    E) Race among pointer-based access and object destruction: We need to 
      ensure that an object (retrieved using a direct pointer) while in use
      cannot be destroyed simultaneouly somewhere else. This is trivially 
      ensured by using locks as follows:
       OPTION 1: ====
        ACCESS:
           STEP 1: *Take* the lock protecting the list of objects.
           STEP 2: Retrieve the pointer to that object.
           STEP 3: (Optional) Take any object-level-lock.
           STEP 4: Use the object for processing.
           STEP 5: (Depends on step 2) Release any object-level-lock.
           STEP 6: *Release* the lock protecting the list of objects.
        DESTROY:
           STEP 1: Take the lock protecting the list of objects.
           STEP 2: Destroy the object as in (D) above. 
           STEP 3: Release the lock protecting the list of objects.

       OPTION 2: ====
        ACCESS:
           STEP 1: *Take* the lock protecting the list of objects.
           STEP 2: Retrieve the pointer to that object.
           STEP 3: Save the object-handle
           STEP 4: *Release* the lock protecting the list of objects.
           STEP 5: Use the object-handle to access the object (as in (D) 
                   above)
        DESTROY:
           STEP 1: *Take* the lock protecting the list of objects.
           STEP 2: Detach object from list of objects.
           STEP 3: *Release* the lock protecting the list of objects.
           =>Now the access to object using a pointer has been cut off.
           STEP 4: Destroy the object as in (D) above. 
       ==============

    F) Need to know (and clear mark) what function is called in what 
       different threads.

    E) List of flows that pass-through MBCSV.
                 MBCSV-Papi = 
                     o  Dispatch: Uses MBCSv handle.
                     o  Other life-cycle APIs: MBCSv handle
                     o  Other ckpt-APIs: CKPT handle 
                 Timer = Peer-instance pointer is accessed.
                 MDS = mailbox structure is accessed
*/

/*
* Module Inclusion Control...
*/
#ifndef MBCSV_ENV_H
#define MBCSV_ENV_H

#include "mbcsv.h"
#include "ncspatricia.h"

#define MBCSV_HA_ROLE_INIT                        0

typedef enum ncs_mbcsv_fsm_states {
/** States of the Generic Fault Tolerant State Machine (Init side)
**/
	NCS_MBCSV_INIT_STATE_IDLE = 1,	/* Idle, state machine not engaged */
	NCS_MBCSV_INIT_MAX_STATES = 1,	/* Max number of init states */

/** States of the Generic Fault Tolerant State Machine (Standby side)
**/
	NCS_MBCSV_STBY_STATE_IDLE = 1,	/* Idle, state machine not engaged */
	NCS_MBCSV_STBY_STATE_WAIT_TO_COLD_SYNC = 2,	/* Waiting for cold sync to finish */
	NCS_MBCSV_STBY_STATE_STEADY_IN_SYNC = 3,	/* Steady state */
	NCS_MBCSV_STBY_STATE_WAIT_TO_WARM_SYNC = 4,	/* Waiting for warm sync to finish */
	NCS_MBCSV_STBY_STATE_VERIFY_WARM_SYNC_DATA = 5,	/* Verifying data from warm sync */
	NCS_MBCSV_STBY_STATE_WAIT_FOR_DATA_RESP = 6,	/* Waiting for response for data request */
	NCS_MBCSV_STBY_MAX_STATES = 6,	/* Maximum number of standby states */

/** States of the Generic Fault Tolerant State Machine (Active side)
**/
	NCS_MBCSV_ACT_STATE_IDLE = 1,	/* Idle, state machine not engaged */
	NCS_MBCSV_ACT_STATE_WAIT_FOR_COLD_WARM_SYNC = 2,	/* Async updates will NOT be sent */
	NCS_MBCSV_ACT_STATE_KEEP_STBY_IN_SYNC = 3,	/* Async updates will be sent */
	NCS_MBCSV_ACT_STATE_MULTIPLE_ACTIVE = 4,	/* Async updates will be sent */
	NCS_MBCSV_ACT_MAX_STATES = 4,	/* Maximum number of primary states */

/** States of the Generic Fault Tolerant State Machine (Quiesced side)
**/
	NCS_MBCSV_QUIESCED_STATE = 1,	/* Quiesced state */
	NCS_MBCSV_QUIESCED_MAX_STATES = 1,	/* Max number of init states */

} NCS_MBCSV_FSM_STATES;

typedef uns32 (*MBCSV_PROCESS_REQ_FUNC_PTR) (struct ncs_mbcsv_arg *);

/*
* Subscription Services
*
* Internal structure used to house subscription services info
*/

typedef struct mbcsv_s_desc {
	struct mbcsv_s_desc *next;	/* Downstream ptr within chain */
	uns16 msg_id;
	NCS_MBCSV_SUB_CB cb_func;
	NCSCONTEXT usr_ctxt;
	NCS_MBCSV_DIR dir;
	tmr_t tmr_id;
	NCSCONTEXT subscriber_ckpt_hdl;	/* Checkpoint handle of the subscriber
					   who wants to subscribe my message */
} MBCSV_S_DESC;

/* This macro checks to see if a subscription might need servicing */

#define  m_MBCSV_CHK_SUBSCRIPTION(f,e,d)

/***********************************************************************************@
*
*                        Peer Instance
*
*    List of Peers. New peer instance is created when MBCSv discovers 
*    its peer entity.
***********************************************************************************/
typedef struct peer_inst {
	struct peer_inst *next;	/* Downstream ptr within chain */
	uns32 hdl;		/* Handle manager handle for timers */
	struct ckpt_inst *my_ckpt_inst;	/*Holds Back pointer to checkpoint data structure */
	uns32 peer_hdl;		/* Peer data structure  handle got through peer discovery mechanism */

	/* Timers */
	NCS_MBCSV_TMR tmr[NCS_MBCSV_MAX_TMRS];

	NCS_MBCSV_FSM_STATES state;	/* State Machines Current State */
	MBCSV_ANCHOR peer_anchor;
	MDS_DEST peer_adest;
	SaAmfHAStateT peer_role;
	uns16 version;		/* peer version info as per SAF */

	/* We need to store data request context and pass it
	   back to user in data resp encode */
	uns64 req_context;

	/* Call again parameters to be preserved */
	uns32 call_again_reo_type;
	MBCSV_REO_HDL call_again_reo_hdl;
	NCS_MBCSV_ACT_TYPE call_again_action;
	uns8 call_again_event;

	/* Some runtime boolean variables */
	uns32 incompatible:1;
	uns32 warm_sync_sent:1;	/* Set to TRUE if warm sync is sent */
	uns32 peer_disc:1;
	uns32 ckpt_msg_sent:1;
	uns32 notify_msg_sent:1;
	uns32 okay_to_async_updt:1;
	uns32 okay_to_send_ntfy:1;
	uns32 ack_rcvd:1;
	uns32 cold_sync_done:1;
	uns32 data_resp_process:1;
	uns32 c_syn_resp_process:1;
	uns32 w_syn_resp_process:1;
	uns32 cold_sync_dec_fail:1;	/* Flag Indicate that cold sync is dec failed */
	uns32 warm_sync_dec_fail:1;
	uns32 data_rsp_dec_fail:1;
	uns32 new_msg_seq:1;	/* Flag Indicates that this in a new message
				 * of the sequence */

} PEER_INST;

typedef void (*NCS_MBCSV_STATE_ACTION_FUNC_PTR) (struct peer_inst *, struct mbcsv_evt *);

/***********************************************************************************@
*
*                        Checkpoint Instance
*
*    A new Checkpoint instance is created when MBCSv receives 
*    OPEN request from MBCSv client.
***********************************************************************************/
typedef struct ckpt_inst {
	NCS_PATRICIA_NODE pat_node;	/* Downstream ptr within chain */
	uns32 pwe_hdl;
	NCS_MBCSV_CLIENT_HDL client_hdl;	/* Handle supplied by client */
	MBCSV_ANCHOR my_anchor;
	MDS_DEST my_vdest;
	SaAmfHAStateT my_role;
	NCS_MBCSV_STATE_ACTION_FUNC_PTR *fsm;	/* MBCSv FSM */
	struct mbcsv_reg *my_mbcsv_inst;	/*Holds Back pointer to MBCSv data structure */
	PEER_INST *active_peer;	/* Pointer to the Active Peer; Applicable only 
				   if my role is not Active. In case of ACTIVE RE this
				   is set to NULL. */
	uns32 ckpt_hdl;		/* Handle to this data structure; to be passed back to 
				   the application on OPEN request */

	/* MBCSv objects to be set and get */
	NCS_BOOL warm_sync_on;
	uns32 warm_sync_time;

	/* Subscription Services */
	MBCSV_S_DESC *msg_slots[NCSMBCSV_MAX_SUBSCRIBE_EVT + 1];

	PEER_INST *peer_list;	/* Base pointer to the list of peers discovered 
				   using peer discovery mechanism */

	/* Some runtime variables */
	uns32 in_quiescing:1;
	uns32 peer_up_sent:1;
	uns32 ftm_role_set:1;
	uns32 role_set:1;
	uns32 data_req_sent:1;
} CKPT_INST;

/***********************************************************************************@
*
*                        MBC registration list
*
*    This is the list of services currently registered with MBC. With 
*    each new Initialize call, MBCSv will add one entry in this list.
***********************************************************************************/
typedef struct mbcsv_reg {
	NCS_PATRICIA_NODE pat_node;	/* Downstream ptr within chain */
	SS_SVC_ID svc_id;	/* Service ID of the service registered with MBCSv */
	NCS_LOCK svc_lock;	/* MBCSv service lock */
	uns16 version;		/* client version info as per SAF */
	SYSF_MBX mbx;
	NCS_MBCSV_CB mbcsv_cb_func;
	uns32 mbcsv_hdl;	/* Handle to this data structure; to be passed back to 
				   the application on INITIALIZE request */
	NCS_PATRICIA_TREE ckpt_ssn_list;	/* Base pointer to the ckpt instance list */
} MBCSV_REG;

/***********************************************************************************@
*
*                        MBCSv control block
*
*    This is the main data structure of the MBCSv which will be initialized
*    with MBCSv layer management CREATE request.
***********************************************************************************/
typedef struct mbcsv_cb {
	NCS_BOOL created;
	uns8 hmpool_id;
	NCS_PATRICIA_TREE reg_list;	/* Base pointer of the MBC registration list; 
					   contains link list of the services registered with the MBCSv */
	NCS_LOCK global_lock;	/* MBCSv global control block lock */

	/* Base pointer of the MBCSv's registration list on MDS */
	NCS_PATRICIA_TREE mbx_list;
	NCS_LOCK mbx_list_lock;

	/* Base pointer of the peers list */
	NCS_PATRICIA_TREE peer_list;
	NCS_LOCK peer_list_lock;

} MBCSV_CB;

extern MBCSV_CB mbcsv_cb;

extern const NCS_MBCSV_STATE_ACTION_FUNC_PTR
 ncsmbcsv_init_state_tbl[NCS_MBCSV_INIT_MAX_STATES][NCSMBCSV_NUM_EVENTS - 1];

extern const NCS_MBCSV_STATE_ACTION_FUNC_PTR
 ncsmbcsv_standby_state_tbl[NCS_MBCSV_STBY_MAX_STATES][NCSMBCSV_NUM_EVENTS - 1];

extern const NCS_MBCSV_STATE_ACTION_FUNC_PTR
 ncsmbcsv_active_state_tbl[NCS_MBCSV_ACT_MAX_STATES][NCSMBCSV_NUM_EVENTS - 1];

extern const NCS_MBCSV_STATE_ACTION_FUNC_PTR
 ncsmbcsv_quiesced_state_tbl[NCS_MBCSV_QUIESCED_MAX_STATES][NCSMBCSV_NUM_EVENTS - 1];

/***********************************************************************************

  INTERNALLY USED MACROS
  
***********************************************************************************/

#define m_NCS_MBCSV_FSM_DISPATCH(peer,event,mbcsv_msg) \
((peer)->my_ckpt_inst)->fsm[(((peer)->state-1)*(NCSMBCSV_NUM_EVENTS-1))+((event)-1)]((peer),(mbcsv_msg))

#define m_SET_NCS_MBCSV_STATE(peer, st)   (peer)->state = st

#define m_MBCSV_INDICATE_ERROR(peer, h, c, e, v, arg, rc)           \
{                                                                   \
   NCS_MBCSV_CB_ARG    parg;                                        \
   memset(&parg, '\0', sizeof(NCS_MBCSV_CB_ARG));             \
   parg.i_op                 = NCS_MBCSV_CBOP_ERR_IND;              \
   parg.i_client_hdl         = h;                                   \
   parg.i_ckpt_hdl           = peer->my_ckpt_inst->ckpt_hdl;        \
   parg.info.error.i_code    = c;                                   \
   parg.info.error.i_err     = e;                                   \
   parg.info.error.i_arg     = (NCSCONTEXT)(long)arg;               \
   parg.info.error.i_peer_version = v;                              \
   rc = peer->my_ckpt_inst->my_mbcsv_inst->mbcsv_cb_func(&parg);    \
}

#define m_MBCSV_PEER_INFO(peer, h, s, v, ic)                        \
{                                                                   \
   NCS_MBCSV_CB_ARG    parg;                                        \
   memset(&parg, '\0', sizeof(NCS_MBCSV_CB_ARG));             \
   parg.i_op                 = NCS_MBCSV_CBOP_PEER;                 \
   parg.i_client_hdl         = h;                                   \
   parg.i_ckpt_hdl           = peer->my_ckpt_inst->ckpt_hdl;        \
   parg.info.peer.i_service  = s;                                   \
   parg.info.peer.i_peer_version = v;                               \
   if (NCSCC_RC_SUCCESS != peer->my_ckpt_inst->my_mbcsv_inst->mbcsv_cb_func(&parg))  \
   { \
       ic = TRUE;  \
   }  \
   else  \
   {  \
       ic = FALSE;  \
   }  \
}

/***********************************************************************************

  INTERNALLY USED HANDLE MANAGER MACROS
  
***********************************************************************************/
#define m_MBCSV_CREATE_HANDLE(ptr) \
ncshm_create_hdl(NCS_HM_POOL_ID_NCS, NCS_SERVICE_ID_MBCSV, (NCSCONTEXT)ptr)

#define m_MBCSV_DESTROY_HANDLE(hdl) \
ncshm_destroy_hdl(NCS_SERVICE_ID_MBCSV, hdl)

#define m_MBCSV_TAKE_HANDLE(hdl) \
ncshm_take_hdl(NCS_SERVICE_ID_MBCSV, hdl)

#define m_MBCSV_GIVE_HANDLE(hdl)   ncshm_give_hdl(hdl)

/*
 * Timer function prototypes.
 */
EXTERN_C void ncs_mbcsv_start_timer(PEER_INST *peer, uns8 timer_type);

EXTERN_C void ncs_mbcsv_tmr_expiry(void *uarg);

/*
 * FSM Function prototypes.
 */

EXTERN_C void ncs_mbcsv_null_func(PEER_INST *peer, MBCSV_EVT *evt);
EXTERN_C void ncs_mbcsv_send_async_update(PEER_INST *peer, MBCSV_EVT *evt);
EXTERN_C void ncs_mbcsv_send_cold_sync_resp_cmplt(PEER_INST *peer, MBCSV_EVT *evt);
EXTERN_C void ncs_mbcsv_send_warm_sync_resp_cmplt(PEER_INST *peer, MBCSV_EVT *evt);
EXTERN_C void ncs_mbcsv_send_warm_sync_resp(PEER_INST *peer, MBCSV_EVT *evt);
EXTERN_C void ncs_mbcsv_send_data_resp(PEER_INST *peer, MBCSV_EVT *evt);
EXTERN_C void ncs_mbcsv_send_data_resp_cmplt(PEER_INST *peer, MBCSV_EVT *evt);
EXTERN_C void ncs_mbcsv_send_data_req(PEER_INST *peer, MBCSV_EVT *evt);
EXTERN_C void ncs_mbcsv_send_cold_sync(PEER_INST *peer, MBCSV_EVT *evt);
EXTERN_C void ncs_mbcsv_send_cold_sync_resp(PEER_INST *peer, MBCSV_EVT *evt);
EXTERN_C void ncs_mbcsv_send_warm_sync(PEER_INST *peer, MBCSV_EVT *evt);
EXTERN_C uns32 ncs_mbscv_rcv_decode(PEER_INST *peer, MBCSV_EVT *evt);

EXTERN_C void ncs_mbcsv_rcv_async_update(PEER_INST *peer, MBCSV_EVT *evt);
EXTERN_C void ncs_mbcsv_rcv_cold_sync_resp(PEER_INST *peer, MBCSV_EVT *evt);
EXTERN_C void ncs_mbcsv_rcv_cold_sync_resp_cmplt(PEER_INST *peer, MBCSV_EVT *evt);
EXTERN_C void ncs_mbcsv_rcv_warm_sync_resp(PEER_INST *peer, MBCSV_EVT *evt);
EXTERN_C void ncs_mbcsv_rcv_warm_sync_resp_cmplt(PEER_INST *peer, MBCSV_EVT *evt);
EXTERN_C void ncs_mbcsv_rcv_entity_in_sync(PEER_INST *peer, MBCSV_EVT *evt);
EXTERN_C void ncs_mbcsv_rcv_data_resp(PEER_INST *peer, MBCSV_EVT *evt);
EXTERN_C void ncs_mbcsv_rcv_data_resp_cmplt(PEER_INST *peer, MBCSV_EVT *evt);
EXTERN_C void ncs_mbcsv_rcv_cold_sync(PEER_INST *peer, MBCSV_EVT *evt);
EXTERN_C void ncs_mbcsv_rcv_warm_sync(PEER_INST *peer, MBCSV_EVT *evt);
EXTERN_C void ncs_mbcsv_rcv_data_req(PEER_INST *peer, MBCSV_EVT *evt);
EXTERN_C void ncs_mbcsv_send_notify(PEER_INST *peer, MBCSV_EVT *evt);
EXTERN_C void ncs_mbcsv_rcv_notify(PEER_INST *peer, MBCSV_EVT *evt);
EXTERN_C void ncs_mbcsv_state_to_mul_act(PEER_INST *peer, MBCSV_EVT *evt);
EXTERN_C void ncs_mbcsv_state_to_wfcs(PEER_INST *peer, MBCSV_EVT *evt);
EXTERN_C void ncs_mbcsv_state_to_kstby_sync(PEER_INST *peer, MBCSV_EVT *evt);

/* 
 * Timer Prototypes
 */
EXTERN_C void ncs_mbcsv_start_timer(PEER_INST *peer, uns8 timer_type);
EXTERN_C void ncs_mbcsv_stop_timer(PEER_INST *peer, uns32 timer_type);
EXTERN_C void ncs_mbcsv_stop_all_timers(PEER_INST *peer);
EXTERN_C void ncs_mbcsv_send_cold_sync_tmr(PEER_INST *peer, MBCSV_EVT *evt);
EXTERN_C void ncs_mbcsv_send_warm_sync_tmr(PEER_INST *peer, MBCSV_EVT *evt);
EXTERN_C void ncs_mbcsv_send_data_req_tmr(PEER_INST *peer, MBCSV_EVT *evt);
EXTERN_C void ncs_mbcsv_cold_sync_cmplt_tmr(PEER_INST *peer, MBCSV_EVT *evt);
EXTERN_C void ncs_mbcsv_warm_sync_cmplt_tmr(PEER_INST *peer, MBCSV_EVT *evt);
EXTERN_C void ncs_mbcsv_transmit_tmr(PEER_INST *peer, MBCSV_EVT *evt);

/*
 * API processing function prototypes.
 */
EXTERN_C uns32 mbcsv_process_initialize_request(NCS_MBCSV_ARG *arg);
EXTERN_C uns32 mbcsv_process_finalize_request(NCS_MBCSV_ARG *arg);
EXTERN_C uns32 mbcsv_process_get_sel_obj_request(NCS_MBCSV_ARG *arg);
EXTERN_C uns32 mbcsv_process_dispatch_request(NCS_MBCSV_ARG *arg);
EXTERN_C uns32 mbcsv_process_open_request(NCS_MBCSV_ARG *arg);
EXTERN_C uns32 mbcsv_process_close_request(NCS_MBCSV_ARG *arg);
EXTERN_C uns32 mbcsv_process_chg_role_request(NCS_MBCSV_ARG *arg);
EXTERN_C uns32 mbcsv_process_snd_ckpt_request(NCS_MBCSV_ARG *arg);
EXTERN_C uns32 mbcsv_process_snd_ntfy_request(NCS_MBCSV_ARG *arg);
EXTERN_C uns32 mbcsv_process_snd_data_req(NCS_MBCSV_ARG *arg);
EXTERN_C uns32 mbcsv_process_sub_osh_request(NCS_MBCSV_ARG *arg);
EXTERN_C uns32 mbcsv_process_sub_per_request(NCS_MBCSV_ARG *arg);
EXTERN_C uns32 mbcsv_process_sub_cancel_request(NCS_MBCSV_ARG *arg);
EXTERN_C uns32 mbcsv_process_get_request(NCS_MBCSV_ARG *arg);
EXTERN_C uns32 mbcsv_process_set_request(NCS_MBCSV_ARG *arg);

/*
 * Utility function prototypes.
 */
EXTERN_C uns32 mbcsv_rmv_reg_inst(MBCSV_REG *reg_list, MBCSV_REG *mbc_reg);
EXTERN_C uns32 mbcsv_remove_ckpt_inst(CKPT_INST *ckpt);
EXTERN_C uns32 mbcsv_process_chg_role(MBCSV_EVT *rcvd_evt, MBCSV_REG *mbc_inst);
EXTERN_C uns32 mbcsv_send_ckpt_data_to_all_peers(NCS_MBCSV_SEND_CKPT *msg_to_send,
						 CKPT_INST *ckpt_inst, MBCSV_REG *mbc_inst);
EXTERN_C uns32 mbcsv_send_notify_msg(uns32 msg_dest, CKPT_INST *ckpt_inst, MBCSV_REG *mbc_inst, NCSCONTEXT i_msg);
EXTERN_C uns32 mbcsv_send_data_req(NCS_UBAID *uba, CKPT_INST *ckpt_inst, MBCSV_REG *mbc_inst);
EXTERN_C uns32 mbcsv_send_client_msg(PEER_INST *peer, uns8 evt, uns32 action);
EXTERN_C uns32 ncs_mbcsv_encode_message(PEER_INST *peer, MBCSV_EVT *evt_msg, uns8 *event, NCS_UBAID *uba);
EXTERN_C uns32 mbcsv_send_msg(PEER_INST *peer, MBCSV_EVT *evt_msg, uns8 event);
EXTERN_C uns32 mbcsv_subscribe_oneshot(NCS_MBCSV_FLTR *fltr, uns16 time_10ms);
EXTERN_C uns32 mbcsv_subscribe_persist(NCS_MBCSV_FLTR *fltr);
EXTERN_C uns32 mbcsv_subscribe_cancel(uns32 sub_hdl);

/*
 * MBCSv queue prototypes.
 */
EXTERN_C NCS_BOOL mbcsv_client_cleanup_mbx(NCSCONTEXT arg, NCSCONTEXT msg);
EXTERN_C uns32 mbcsv_client_queue_init(MBCSV_REG *mbc_reg);
EXTERN_C void mbcsv_client_queue_destroy(MBCSV_REG *mbc_reg);

/*
 * Process events prototypes.
 */
EXTERN_C uns32 mbcsv_process_events(MBCSV_EVT *rcvd_evt, uns32 mbcsv_hdl);
EXTERN_C uns32 mbcsv_hdl_dispatch_one(uns32 mbcsv_hdl, SYSF_MBX mbx);
EXTERN_C uns32 mbcsv_hdl_dispatch_all(uns32 mbcsv_hdl, SYSF_MBX mbx);
EXTERN_C uns32 mbcsv_hdl_dispatch_block(uns32 mbcsv_hdl, SYSF_MBX mbx);

/*
 * Peer discovery function prototypes.
 */
EXTERN_C PEER_INST *mbcsv_search_and_return_peer(PEER_INST *peer_list, MBCSV_ANCHOR anchor);
EXTERN_C PEER_INST *mbcsv_add_new_peer(CKPT_INST *ckpt, MBCSV_ANCHOR anchor);
EXTERN_C uns32 mbcsv_shutdown_peer(PEER_INST *peer_ptr);
EXTERN_C uns32 mbcsv_rmv_peer(CKPT_INST *ckpt, MBCSV_ANCHOR anchor);
EXTERN_C uns32 mbcsv_empty_peers_list(CKPT_INST *ckpt);
EXTERN_C uns32 mbcsv_process_peer_discovery_message(MBCSV_EVT *msg, MBCSV_REG *mbc_reg);
EXTERN_C PEER_INST *mbcsv_my_active_peer(CKPT_INST *ckpt);
EXTERN_C void mbcsv_clear_multiple_active_state(CKPT_INST *ckpt);
EXTERN_C void mbcsv_close_old_session(PEER_INST *active_peer);
EXTERN_C void mbcsv_set_up_new_session(CKPT_INST *ckpt, PEER_INST *new_act_peer);
EXTERN_C void mbcsv_set_peer_state(CKPT_INST *ckpt, PEER_INST *peer, NCS_BOOL peer_up);
EXTERN_C uns32 mbcsv_process_peer_up_info(MBCSV_EVT *msg, CKPT_INST *ckpt, uns8 peer_up);
EXTERN_C void mbcsv_update_peer_info(MBCSV_EVT *msg, CKPT_INST *ckpt, PEER_INST *peer);
EXTERN_C uns32 mbcsv_process_peer_down(MBCSV_EVT *msg, CKPT_INST *ckpt);
EXTERN_C uns32 mbcsv_process_peer_info_rsp(MBCSV_EVT *msg, CKPT_INST *ckpt);
EXTERN_C uns32 mbcsv_process_peer_chg_role(MBCSV_EVT *msg, CKPT_INST *ckpt);
EXTERN_C uns32 mbcsv_send_peer_disc_msg(uns32 type, MBCSV_REG *mbc, CKPT_INST *ckpt,
					PEER_INST *peer, uns32 mds_send_type, MBCSV_ANCHOR anchor);

/*
 * Library create and destroy functions.
 */
EXTERN_C uns32 mbcsv_lib_init(NCS_LIB_REQ_INFO *req_info);
EXTERN_C uns32 mbcsv_lib_destroy(void);

#endif
