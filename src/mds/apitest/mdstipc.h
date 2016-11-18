/*          OpenSAF
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
 * Author(s): Ericsson AB
 *
 */

#ifndef MDS_APITEST_MDSTIPC_H_
#define MDS_APITEST_MDSTIPC_H_

#include "base/ncssysf_tsk.h"
#include "base/ncssysf_def.h"

typedef struct tet_task{
  NCS_OS_CB      entry;
  void *         arg;
  char *         name;
  int            prio;
  unsigned int   stack_size;
  NCSCONTEXT     t_handle;     
}TET_TASK;

typedef struct tet_subscr{
  NCSMDS_SCOPE_TYPE  scope;
  uint8_t               num_svcs;
  MDS_SVC_ID         *svc_ids;
  EVT_FLTR           evt_map;
}TET_SUBSCR;
typedef struct tet_eventinfo{
  MDS_SVC_ID         ur_svc_id;
  NCSMDS_CHG         event;
  MDS_DEST           dest;
  V_DEST_QA          anc;
  V_DEST_RL          role;
  NODE_ID            node_id;
  PW_ENV_ID          pwe_id;
  MDS_SVC_ID         svc_id;
  MDS_SVC_PVT_SUB_PART_VER         rem_svc_pvt_ver; 
  MDS_HDL                          svc_pwe_hdl;
}TET_EVENT_INFO;
typedef struct tet_svc{
  NCSMDS_CALLBACK_API svc_cb;
  MDS_CLIENT_HDL      yr_svc_hdl;/*the decider*/
  NCSMDS_SCOPE_TYPE   install_scope;
  bool            mds_q_ownership;
  /* return values */
  MDS_DEST            dest;    /*can be Absolute or Virtual*/
  V_DEST_QA           anc;    /*can b on multiple Vdests*/
  NCS_SEL_OBJ         sel_obj;
  /*added later*/
  TET_SUBSCR          subscr; 
  TET_EVENT_INFO      svcevt[100];/*holds 100 subscribed services event info*/
  int                 subscr_count;
  MDS_SVC_ID          svc_id;
  MDS_SVC_PVT_SUB_PART_VER   mds_svc_pvt_ver;
  TET_TASK            task; /* FIX THIS can b multiple tasks */
  bool            got_msg;/* when will u fill this*/
}TET_SVC;

typedef struct tet_pwe{
  MDS_HDL     mds_dest_hdl; /* can be mds_adest_hdl or mds_vdest_hdl */
  PW_ENV_ID   pwe_id;
  /* to b passed to the Service API's*/
  MDS_HDL  mds_pwe_hdl;
}TET_PWE;

typedef struct tet_adest{
  
  /*return values*/
  MDS_DEST    adest;
  /*to be passed to Service API's*/
  MDS_HDL  mds_pwe1_hdl;
  MDS_HDL  mds_adest_hdl;
  /*added by me: should support multiple*/
  TET_SVC     svc[600];     /*change the array size*/
  int         svc_count;   /* how many services r installed in this ADESt */
  TET_PWE     pwe[2000];
  int         pwe_count;   /* how many pwes r installed in this ADEST */
}TET_ADEST;

typedef struct tet_vdest{
  bool       persistent;
  NCS_VDEST_TYPE policy;
  SaNameT        name;
  /*return values*/
  MDS_DEST       vdest;
  /*to be passed to Service API's*/
  MDS_HDL     mds_pwe1_hdl;
  MDS_HDL     mds_vdest_hdl;
  /*added by me: should support multiple */
  TET_SVC        svc[600];  /*Stress : change it to 2500 from 1000*/
  int            svc_count;  /* how many services r installed in this VDEST */
  TET_PWE        pwe[2000];
  int            pwe_count;  /* how many pwes r installed in this VDEST */
}TET_VDEST;

typedef struct mds_tet_msg{
#define TET_MSG_SIZE_MAX  45875200
#define TET_MSG_SIZE_MIN  131472 
  uint32_t send_len;
  uint32_t recvd_len;
  /*char *send_data; this is best approach*/
  /*char *recvd_data;this is best approach*/
  char  send_data[TET_MSG_SIZE_MIN];
  char  recvd_data[TET_MSG_SIZE_MIN];
#if 0
  char  send_data[TET_MSG_SIZE_MAX];
  char  recvd_data[TET_MSG_SIZE_MAX];
#endif
}TET_MDS_MSG;

/*this is for callback receive*/

typedef struct tet_mds_recvd_msg_info{
  
  MDS_SVC_ID             yr_svc_id; /*the service id , the MDS is talking to: 
                                      should b identical to the one specified
                                      by the user at the time of service 
                                      installation*/
  MDS_CLIENT_HDL         yr_svc_hdl; /*its service handle*/
  NCSCONTEXT             msg;       /* message to be delivered*/
  bool               rsp_reqd;  /* true if send is awaiting a response */
  MDS_SYNC_SND_CTXT      msg_ctxt;  /* Valid only if "i_rsp_expected == true"*/
  MDS_DEST               fr_dest;   /* MDS destination on which sending 
                                       service resides   */
  V_DEST_QA              fr_anc;    /* From address anchor in case of VDEST */
  MDS_SVC_ID             fr_svc_id; /* Sender service id   */            
  MDS_DEST               to_dest;   /* MDS destination on which Receiving 
                                       service resides */
  MDS_SVC_ID             to_svc_id; /*Receiver service id*/
  MDS_SEND_PRIORITY_TYPE priority; 
  NODE_ID                node_id;   /*nodeid on which the sender service 
                                      resides */
  MDS_CLIENT_MSG_FORMAT_VER msg_fmt_ver;
  MDS_DIRECT_BUFF        buff;
  uint16_t                  len;
}TET_MDS_RECVD_MSG_INFO;

/********************* GLOBAL variables ********************/
TET_ADEST              gl_tet_adest;
TET_VDEST              gl_tet_vdest[4];/*change it to 6 to run VDS Redundancy: 101 for Stress*/
NCSADA_INFO            ada_info;
NCSVDA_INFO            vda_info;
NCSMDS_INFO            svc_to_mds_info;
TET_EVENT_INFO         gl_event_data;
TET_SVC                gl_tet_svc;
TET_MDS_RECVD_MSG_INFO gl_rcvdmsginfo,gl_direct_rcvmsginfo;
int                    gl_vdest_indx;
MDS_DIRECT_BUFF        direct_buff;

/*Callback Failure*/
int                    gl_COPY_CB_FAIL;
int                    gl_SYS_EVENT_CB_FAIL;
int                    gl_ENC_CB_FAIL;
int                    gl_DEC_CB_FAIL;
int                    gl_ENC_FLAT_CB_FAIL;
int                    gl_DEC_FLAT_CB_FAIL;
int                    gl_RECEIVE_CB_FAIL;
int                    gl_COPY_CB_FAIL;


uint32_t ncs_encode_16bit(uint8_t **,uint32_t);
uint16_t ncs_decode_16bit(uint8_t **);

uint32_t tet_mds_svc_callback(NCSMDS_CALLBACK_INFO *);
/******************MDS call back routines *********************************/
uint32_t tet_mds_cb_cpy         (NCSMDS_CALLBACK_INFO *);
uint32_t tet_mds_cb_enc         (NCSMDS_CALLBACK_INFO *);
uint32_t tet_mds_cb_dec         (NCSMDS_CALLBACK_INFO *);
uint32_t tet_mds_cb_enc_flat    (NCSMDS_CALLBACK_INFO *);
uint32_t tet_mds_cb_dec_flat    (NCSMDS_CALLBACK_INFO *);
uint32_t tet_mds_cb_rcv         (NCSMDS_CALLBACK_INFO *);
uint32_t tet_mds_svc_event      (NCSMDS_CALLBACK_INFO *);
uint32_t tet_mds_sys_event      (NCSMDS_CALLBACK_INFO *);
uint32_t tet_mds_cb_quiesced_ack(NCSMDS_CALLBACK_INFO *);
uint32_t tet_mds_cb_direct_rcv  (NCSMDS_CALLBACK_INFO *);
/*uint32_t tet_mds_svc_max   (NCSMDS_CALLBACK_INFO *mds_to_svc_info);*/
/**********************************************************************/
uint32_t tet_initialize_setup(bool);
uint32_t tet_cleanup_setup(void);

/***************  USER DEFINED WRAPPERS FOR MDS DEST APIs     *************/

uint32_t adest_get_handle(void);
uint32_t create_pwe_on_adest(MDS_HDL,PW_ENV_ID);
uint32_t destroy_pwe_on_adest(MDS_HDL);
uint32_t create_vdest(NCS_VDEST_TYPE,MDS_DEST);
uint32_t create_named_vdest(bool,NCS_VDEST_TYPE,char *);
MDS_DEST vdest_lookup(char *);
uint32_t vdest_change_role(MDS_DEST ,V_DEST_RL);
uint32_t destroy_vdest(MDS_DEST);
uint32_t destroy_named_vdest(bool,MDS_DEST,char *);
uint32_t create_pwe_on_vdest(MDS_HDL,PW_ENV_ID);
uint32_t destroy_pwe_on_vdest(MDS_HDL);


/**************     USER DEFINED WRAPPERS FOR MDS SERVICE APIs **************/

uint32_t tet_create_task(NCS_OS_CB ,NCSCONTEXT );
uint32_t tet_release_task(void *task_handle);
int is_adest_sel_obj_found(int );
int is_sel_obj_found(int);
int is_vdest_sel_obj_found(int ,int );
void tet_mds_free_msg(NCSCONTEXT msg_to_be_freed);

uint32_t mds_service_install(MDS_HDL ,MDS_SVC_ID ,MDS_SVC_PVT_SUB_PART_VER ,NCSMDS_SCOPE_TYPE,bool,bool);
uint32_t mds_service_subscribe(MDS_HDL,MDS_SVC_ID ,NCSMDS_SCOPE_TYPE ,uint8_t ,MDS_SVC_ID *);
uint32_t mds_service_redundant_subscribe(MDS_HDL ,MDS_SVC_ID , NCSMDS_SCOPE_TYPE ,uint8_t ,MDS_SVC_ID *);
uint32_t mds_service_system_subscribe(MDS_HDL ,MDS_SVC_ID ,EVT_FLTR );
uint32_t mds_service_cancel_subscription(MDS_HDL ,MDS_SVC_ID ,uint8_t , MDS_SVC_ID *);
uint32_t mds_service_uninstall(MDS_HDL ,MDS_SVC_ID);

uint32_t mds_send_message(MDS_HDL ,MDS_SVC_ID ,NCSCONTEXT ,MDS_SVC_ID ,
                       MDS_SEND_PRIORITY_TYPE ,MDS_SENDTYPES,MDS_DEST ,uint32_t ,
                       MDS_DIRECT_BUFF ,uint16_t ,MDS_SYNC_SND_CTXT ,
                       NCSMDS_SCOPE_TYPE ,V_DEST_QA );

/* Non Redundant */
uint32_t mds_just_send(MDS_HDL ,MDS_SVC_ID ,MDS_SVC_ID ,MDS_DEST,
                    MDS_SEND_PRIORITY_TYPE,TET_MDS_MSG * );
uint32_t mds_send_get_ack(MDS_HDL ,MDS_SVC_ID ,MDS_SVC_ID ,MDS_DEST ,int64_t ,
                       MDS_SEND_PRIORITY_TYPE,TET_MDS_MSG *);
uint32_t mds_send_get_response(MDS_HDL ,MDS_SVC_ID ,MDS_SVC_ID ,MDS_DEST ,
                            int64_t ,MDS_SEND_PRIORITY_TYPE,TET_MDS_MSG *);
uint32_t mds_send_response(MDS_HDL ,MDS_SVC_ID ,TET_MDS_MSG *);
uint32_t mds_sendrsp_getack(MDS_HDL,MDS_SVC_ID ,int64_t,TET_MDS_MSG *);
uint32_t mds_broadcast_to_svc(MDS_HDL ,MDS_SVC_ID ,MDS_SVC_ID ,
                           NCSMDS_SCOPE_TYPE ,MDS_SEND_PRIORITY_TYPE,
                           TET_MDS_MSG *);


/*Direct Send*/
uint32_t mds_direct_send_message(MDS_HDL,MDS_SVC_ID,MDS_SVC_ID,
                              MDS_CLIENT_MSG_FORMAT_VER,MDS_SENDTYPES,
                              MDS_DEST,int64_t,MDS_SEND_PRIORITY_TYPE,char *);
uint32_t mds_direct_broadcast_message(MDS_HDL ,MDS_SVC_ID ,MDS_SVC_ID ,
                                   MDS_CLIENT_MSG_FORMAT_VER,
                                   MDS_SENDTYPES ,NCSMDS_SCOPE_TYPE,
                                   MDS_SEND_PRIORITY_TYPE);
uint32_t mds_direct_response(MDS_HDL ,MDS_SVC_ID, MDS_CLIENT_MSG_FORMAT_VER, MDS_SENDTYPES ,int64_t );

/*General Purpose*/
uint32_t mds_service_retrieve(MDS_HDL ,MDS_SVC_ID ,SaDispatchFlagsT );
uint32_t mds_query_vdest_for_anchor(MDS_HDL ,MDS_SVC_ID ,MDS_DEST ,MDS_SVC_ID ,
                                 V_DEST_RL );
uint32_t mds_query_vdest_for_role(MDS_HDL ,MDS_SVC_ID ,MDS_DEST ,MDS_SVC_ID ,
                               V_DEST_QA );
uint32_t mds_service_query_for_pwe(MDS_HDL ,MDS_SVC_ID );
uint32_t is_service_on_adest(MDS_HDL ,MDS_SVC_ID );
uint32_t change_role(MDS_HDL ,MDS_SVC_ID ,V_DEST_RL );

/************** TEST CASE FUNCTION DECLARATIONS ************/
void  tet_svc_install_tp_01(void);
void  tet_svc_install_tp_02(void);
void  tet_svc_install_tp_03(void);
void  tet_svc_install_tp_05(void);
void  tet_svc_install_tp_06(void);
void  tet_svc_install_tp_07(void);
void  tet_svc_install_tp_08(void);
void  tet_svc_install_tp_09(void);
void  tet_svc_install_tp_10(void);
void  tet_svc_install_tp_11(void);
void  tet_svc_install_tp_12(void);
void  tet_svc_install_tp_13(void);
void  tet_svc_install_tp_14(void);
void  tet_svc_install_tp_15(void);
void  tet_svc_install_tp_16(void);
void  tet_svc_install_tp_17(void);
void tet_svc_unstall_tp_1(void);
void tet_svc_unstall_tp_3(void);
void tet_svc_unstall_tp_4(void);
void tet_svc_unstall_tp_5(void);
void tet_svc_subscr_VDEST_1(void);
void tet_svc_subscr_VDEST_2(void);
void tet_svc_subscr_VDEST_3(void);
void tet_svc_subscr_VDEST_4(void);
void tet_svc_subscr_VDEST_5(void);
void tet_svc_subscr_VDEST_6(void);
void tet_svc_subscr_VDEST_8(void);
void tet_svc_subscr_VDEST_9(void);
void tet_svc_subscr_VDEST_7(void);
void tet_svc_subscr_VDEST_10(void);
void tet_svc_subscr_VDEST_11(void);
void tet_svc_subscr_VDEST_12(void);
void cleanup_ADEST_srv(void);
void tet_svc_subscr_ADEST_1(void);
void tet_svc_subscr_ADEST_2(void);
void tet_svc_subscr_ADEST_3(void);
void tet_svc_subscr_ADEST_4(void);
void tet_svc_subscr_ADEST_5(void);
void tet_svc_subscr_ADEST_6(void);
void tet_svc_subscr_ADEST_7(void);
void tet_svc_subscr_ADEST_8(void);
void tet_svc_subscr_ADEST_9(void);
void tet_svc_subscr_ADEST_10(void);
void tet_svc_subscr_ADEST_11(void);
void tet_just_send_tp_1(void);
void tet_just_send_tp_2(void);
void tet_just_send_tp_3(void);
void tet_just_send_tp_4(void);
void tet_just_send_tp_5(void);
void tet_just_send_tp_6(void);
void tet_just_send_tp_7(void);
void tet_just_send_tp_8(void);
void tet_just_send_tp_9(void);
void tet_just_send_tp_10(void);
void tet_just_send_tp_11(void);
void tet_just_send_tp_12(void);
void tet_just_send_tp_13(void);
void tet_just_send_tp_14(void);
void tet_send_ack_tp_1(void);
void tet_send_ack_tp_2(void);
void tet_send_ack_tp_3(void);
void tet_send_ack_tp_4(void);
void tet_send_ack_tp_5(void);
void tet_send_ack_tp_6(void);
void tet_send_ack_tp_7(void);
void tet_send_ack_tp_8(void);
void tet_send_ack_tp_9(void);
void tet_send_ack_tp_10(void);
void tet_send_ack_tp_11(void);
void tet_send_ack_tp_12(void);
void tet_send_ack_tp_13(void);
void tet_query_pwe_tp_1(void);
void tet_query_pwe_tp_2(void);
void tet_query_pwe_tp_3(void);
void tet_send_response_tp_1(void);
void tet_send_response_tp_2(void);
void tet_send_response_tp_3(void);
void tet_send_response_tp_4(void);
void tet_send_response_tp_5(void);
void tet_send_response_tp_6(void);
void tet_send_response_tp_7(void);
void tet_send_response_tp_8(void);
void tet_send_response_tp_9(void);
void tet_send_response_tp_10(void);
void tet_send_response_tp_11(void);
void tet_send_response_tp_12(void);

void tet_send_all_tp_1(void);
void tet_send_all_tp_2(void);
void tet_send_response_ack_tp_1(void);
void tet_send_response_ack_tp_2(void);
void tet_send_response_ack_tp_3(void);
void tet_send_response_ack_tp_4(void);
void tet_send_response_ack_tp_5(void);
void tet_send_response_ack_tp_6(void);
void tet_send_response_ack_tp_7(void);
void tet_send_response_ack_tp_8(void);
void tet_broadcast_to_svc_tp_1(void);
void tet_broadcast_to_svc_tp_2(void);
void tet_broadcast_to_svc_tp_3(void);
void tet_broadcast_to_svc_tp_4(void);
void tet_broadcast_to_svc_tp_5(void);
void tet_broadcast_to_svc_tp_6(void);
void tet_direct_just_send_tp_1(void);
void tet_direct_just_send_tp_2(void);
void tet_direct_just_send_tp_3(void);
void tet_direct_just_send_tp_4(void);
void tet_direct_just_send_tp_5(void);
void tet_direct_just_send_tp_6(void);
void tet_direct_just_send_tp_7(void);
void tet_direct_just_send_tp_8(void);
void tet_direct_just_send_tp_9(void);
void tet_direct_just_send_tp_10(void);
void tet_direct_just_send_tp_11(void);
void tet_direct_just_send_tp_12(void);
void tet_direct_just_send_tp_13(void);
void tet_direct_just_send_tp_14(void);
void tet_direct_just_send_tp_15(void);
void tet_direct_send_all_tp_1(void);
void tet_direct_send_all_tp_2(void);
void tet_direct_send_all_tp_3(void);
void tet_direct_send_all_tp_4(void);
void tet_direct_send_all_tp_5(void);
void tet_direct_send_all_tp_6(void);
void tet_Dadest_all_chgrole_rcvr_thread(void);
void tet_direct_send_ack_tp_1(void);
void tet_direct_send_ack_tp_2(void);
void tet_direct_send_ack_tp_3(void);
void tet_direct_send_ack_tp_4(void);
void tet_direct_send_ack_tp_5(void);
void tet_direct_send_ack_tp_6(void);
void tet_direct_send_ack_tp_7(void);
void tet_direct_send_ack_tp_8(void);
void tet_direct_send_ack_tp_9(void);
void tet_direct_send_ack_tp_10(void);
void tet_direct_send_ack_tp_11(void);
void tet_direct_send_ack_tp_12(void);
void tet_direct_send_ack_tp_13(void);
void tet_direct_send_response_tp_1(void);
void tet_direct_send_response_tp_2(void);
void tet_direct_send_response_tp_3(void);
void tet_direct_send_response_tp_4(void);
void tet_direct_send_response_tp_5(void);
void tet_direct_send_response_ack_tp_1(void);
void tet_direct_send_response_ack_tp_2(void);
void tet_direct_send_response_ack_tp_3(void);
void tet_direct_send_response_ack_tp_4(void);
void tet_direct_send_response_ack_tp_5(void);
void tet_direct_broadcast_to_svc_tp_1(void);
void tet_direct_broadcast_to_svc_tp_2(void);
void tet_direct_broadcast_to_svc_tp_3(void);
void tet_direct_broadcast_to_svc_tp_4(void);
void tet_direct_broadcast_to_svc_tp_5(void);
void tet_direct_broadcast_to_svc_tp_6(void);
void tet_direct_broadcast_to_svc_tp_7(void);
void tet_direct_broadcast_to_svc_tp_8(void);
void tet_create_MxN_VDEST_1(void);
void tet_create_MxN_VDEST_2(void);
void tet_create_MxN_VDEST_3(void);
void tet_create_MxN_VDEST_4(void);
void tet_create_Nway_VDEST_1(void);
void tet_create_Nway_VDEST_2(void);
void tet_create_Nway_VDEST_3(void);
void tet_create_Nway_VDEST_4(void);
void tet_create_OAC_VDEST_tp_1(void);
void tet_create_OAC_VDEST_tp_2(void);
void tet_destroy_VDEST_twice_tp_1(void);
void tet_destroy_VDEST_twice_tp_2(void);
void tet_destroy_ACTIVE_MxN_VDEST_tp_1(void);
void tet_destroy_ACTIVE_MxN_VDEST_tp_2(void);
void tet_chg_standby_to_queisced_tp_1(void);
void tet_chg_standby_to_queisced_tp_2(void);
void tet_chg_standby_to_queisced_tp_3(void);
void tet_chg_standby_to_queisced_tp_4(void);
void tet_create_named_VDEST_1(void);
void tet_create_named_VDEST_2(void);
void tet_create_named_VDEST_3(void);
void tet_create_named_VDEST_4(void);
void tet_create_named_VDEST_5(void);
void tet_create_named_VDEST_6(void);
void tet_create_named_VDEST_7(void);
void tet_create_named_VDEST_8(void);
void tet_create_named_VDEST_9(void);
void tet_test_PWE_VDEST_tp_1(void);
void tet_test_PWE_VDEST_tp_2(void);
void tet_test_PWE_VDEST_tp_3(void);
void tet_test_PWE_VDEST_tp_4(void);
void tet_test_PWE_VDEST_tp_5(void);
void tet_test_PWE_VDEST_tp_6(void);


/*Multi Process/Node Test Case Functions*/
/* Non imlemented */
void tet_just_send_both(int);
void tet_send_ack_both(int);
void tet_send_response_both(int);
void tet_send_resp_ack_both(int);

void tet_direct_just_send_both(int);
void tet_direct_send_ack_both(int);
void tet_direct_send_resp_both(int);
void tet_direct_send_resp_ack_both(int);

void tet_red_just_send_both(int);
void tet_red_send_ack_both(int);
void tet_red_send_resp_both(int);
void tet_red_sendresp_ack_both(int);

void tet_dispatch_blocking_both(int choice);
void msg_priority_conformance(int);
void sequential_delivery(int);
void MXN_redundancy(void);
void all_events(void);
void cb_fail_all_events(void);
void red_all_events(void);
void tet_stress(void);
void tet_flood_both(int choice);
void tet_just_send_MAXSIZE_both(int choice);
void tet_VDS_both(void);
void tet_dummy_both(void);
void tet_message_from_unknown_adest(void);
void tet_svc_subscribe_MAX(void);
void ncs_agents_shut_start(int choice);
 

/* Implemented */
uint32_t tet_initialise_setup(bool fail_no_active_sends); 
void Print_return_status(uint32_t rs); 
void tet_vdest_install_thread(void);
void tet_svc_install_upto_MAX(void);
void tet_vdest_uninstall_thread(void);
void tet_adest_cancel_thread(void);
void tet_adest_retrieve_thread(void);
void tet_adest_rcvr_thread(void);
void tet_adest_rcvr_svc_thread(void);
void tet_vdest_rcvr_resp_thread(void);
void tet_vdest_rcvr_thread(void);
void tet_Dadest_all_rcvr_thread(void);
void tet_adest_all_chgrole_rcvr_thread(void);
void tet_vdest_all_rcvr_thread(void);
void tet_adest_all_rcvrack_thread(void);
void tet_adest_all_rcvrack_chgrole_thread(void);
void tet_Dadest_all_rcvrack_chgrole_thread(void);
void tet_change_role_thread(void);
void tet_adest_all_rcvr_thread(void);
void tet_vdest_Srcvr_thread(void);
void tet_vdest_Arcvr_thread(void);
void tet_Dadest_rcvr_thread(void);
void tet_Dvdest_rcvr_thread(void);
void tet_Dvdest_rcvr_all_rack_thread(void);
void tet_Dvdest_rcvr_all_thread(void);
void tet_Dvdest_rcvr_all_chg_role_thread(void);
void tet_Dvdest_Srcvr_thread(void);
void tet_Dvdest_Srcvr_all_thread(void);
void tet_Dvdest_Arcvr_thread(void);
void tet_Dvdest_Arcvr_all_thread(void);
void tet_destroy_PWE_ADEST_twice_tp(void);
void tet_create_PWE_ADEST_twice_tp(void);
void tet_create_default_PWE_ADEST_tp(void);
void tet_create_PWE_upto_MAX_tp(void);
void tet_create_PWE_upto_MAX_VDEST(void);
void tet_create_default_PWE_VDEST_tp(void);
uint32_t mds_send_get_redack(MDS_HDL mds_hdl,
                             MDS_SVC_ID svc_id,
                             MDS_SVC_ID to_svc,
                             MDS_DEST to_vdest,
                             V_DEST_QA to_anc,
                             int64_t time_to_wait,
                             MDS_SEND_PRIORITY_TYPE priority,
                             TET_MDS_MSG *message);
uint32_t mds_send_redrsp_getack(MDS_HDL mds_hdl,
                                MDS_SVC_ID svc_id,
                                int64_t time_to_wait,
                                TET_MDS_MSG *response);
uint32_t   tet_sync_point(void);

#endif  // MDS_APITEST_MDSTIPC_H_
