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
  DESCRIPTION: This file consists all top level api & structs for test purpose.
*****************************************************************************/
#ifndef MBC_APITEST_MBCSV_API_H_
#define MBC_APITEST_MBCSV_API_H_

/* include files */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ncsgl_defs.h"
#include "ncs_osprm.h"
#include "base/ncssysf_def.h"
#include "base/ncssysf_tsk.h"
#include "ncssysf_ipc.h"
#include "ncssysf_lck.h"
#include "ncssysfpool.h"
#include "ncssysf_tmr.h"
#include "ncspatricia.h"
#include "mds_papi.h"
#include "ncs_mda_papi.h"

/* saf related include files */
#include "saAis.h"
#include "saAmf.h"
#include "saCkpt.h"

/* #incude "mbcsv.h"  */
#include "mbcsv_papi.h"
#include "mbcsv_purpose.h"


/* Macro definations */
/* service id macros */
#define MBCSTM_SVC_ID1     NCSMDS_SVC_ID_EXTERNAL_MIN 
#define MBCSTM_SVC_ID2     NCSMDS_SVC_ID_EXTERNAL_MIN + 1
#define MBCSTM_SVC_ID3     NCSMDS_SVC_ID_EXTERNAL_MIN + 2
#define MBCSTM_SVC_ID4     NCSMDS_SVC_ID_EXTERNAL_MIN + 3

/* Destination id marcos */
#define MBCSTM_SSN_DEST_MIN   NCSVDA_EXTERNAL_UNNAMED_MIN
#define MBCSTM_SSN_DEST1      MBCSTM_SSN_DEST_MIN + 1
#define MBCSTM_SSN_DEST2      MBCSTM_SSN_DEST_MIN + 2
#define MBCSTM_SSN_DEST3      MBCSTM_SSN_DEST_MIN + 3 
#define MBCSTM_SSN_DEST4      MBCSTM_SSN_DEST_MIN + 4 
#if 0
#define MBCSTM_SSN_ANC1       100
#define MBCSTM_SSN_ANC2       120
#define MBCSTM_SSN_ANC3       140
#define MBCSTM_SSN_ANC4       160
#endif
#define MBCSTM_WARM_DEF       100

#define MBCSTM_CSI_DATA_MAX   50
#define MBCSTM_SSN_MAX        4
#define MBCSTM_SSN_PEER_MAX   3
#define MBCSTM_PEER_COUNT     3
#define MBCSTM_SVC_MAX        4
#define MBCSTM_TP_MAX         100
#define MBCSTM_SYS_NAMES      4
#define MBCSTM_SYNC_TIMEOUT   30

/* timer values */
#define MBCSTM_TIMER_SEND_COLD_SYNC_PERIOD      10 /*  NCS_MBCSV_TMR_SEND_COLD_SYNC_PERIOD/1000 */
#define MBCSTM_TIMER_SEND_WARM_SYNC_PERIOD      20 /* NCS_MBCSV_TMR_SEND_WARM_SYNC_PERIOD/1000 */
#define MBCSTM_TIMER_COLD_SYNC_CMPLT_PERIOD     20 /* NCS_MBCSV_TMR_COLD_SYNC_CMPLT_PERIOD/1000 */
#define MBCSTM_TIMER_WARM_SYNC_CMPLT_PERIOD     20 /* NCS_MBCSV_TMR_WARM_SYNC_CMPLT_PERIOD/1000 */
#define MBCSTM_TIMER_DATA_RESP_CMPLT_PERIOD     10 /* NCS_MBCSV_TMR_DATA_RESP_CMPLT_PERIOD/1000 */

typedef enum mbcstm_sys {
  MBCSTM_SYS_MIN = 0,
  MBCSTM_SVC_INS1 = MBCSTM_SYS_MIN,
  MBCSTM_SVC_INS2,
  MBCSTM_SVC_INS3,
  MBCSTM_SVC_INS4,
  MBCSTM_SYS_MAX =  MBCSTM_SVC_INS4
}MBCSTM_SYS;

typedef enum mbcstm_sync {
  MBCSTM_SYNC_NO,
  MBCSTM_SYNC_PROCESS,
  MBCSTM_SYNC_COMPLETE,
  MBCSTM_DATA_REQUEST,
  MBCSTM_DATA_RSP_CMPL
} MBCSTM_SYNC;

typedef enum mbcstm_fsm_states {
  /** States of the Generic Fault Tolerant State Machine (Init side) **/ 
  MBCSTM_INIT_MAX_STATES                 = 1, /* Max number of init states*/
  
  /** States of the Generic Fault Tolerant State Machine (Standby side)  **/
  MBCSTM_STBY_STATE_IDLE                 = 1, /* Idle, state machine not engaged */
  MBCSTM_STBY_STATE_WAIT_TO_COLD_SYNC    = 2, /* Waiting for cold sync to finish */
  MBCSTM_STBY_STATE_STEADY_IN_SYNC       = 3, /* Steady state */
  MBCSTM_STBY_STATE_WAIT_TO_WARM_SYNC    = 4, /* Waiting for warm sync to finish */
  MBCSTM_STBY_STATE_VERIFY_WARM_SYNC_DATA= 5, /* Verifying data from warm sync */
  MBCSTM_STBY_STATE_WAIT_FOR_DATA_RESP   = 6, /* Waiting for response for data request */
  MBCSTM_STBY_MAX_STATES                 = 6, /* Maximum number of standby states */
  
  /** States of the Generic Fault Tolerant State Machine (Active side) **/
  MBCSTM_ACT_STATE_IDLE                    = 1, /* Idle, state machine not engaged */
  MBCSTM_ACT_STATE_WAIT_FOR_COLD_WARM_SYNC = 2, /* Async updates will NOT be sent */
  MBCSTM_ACT_STATE_KEEP_STBY_IN_SYNC       = 3, /* Async updates will be sent */
  MBCSTM_ACT_STATE_MULTIPLE_ACTIVE         = 4, /* Async updates will be sent */
  MBCSTM_ACT_MAX_STATES                    = 4, /* Maximum number of primary states */
  
  /** States of the Generic Fault Tolerant State Machine (Quiesced side) **/
  MBCSTM_QUIESCED_STATE                    = 1, /* Quiesced state */
  MBCSTM_QUIESCED_MAX_STATES               = 1, /* Max number of init states*/
  
}MBCSTM_FSM_STATES;

/* macro routines  */ 

/* for real SPO3
   #define MBCSTM_SET_VDEST_ID_IN_MDS_DEST(mds_dest,value)  m_NCS_SET_VDEST_ID_IN_MDS_DEST(mds_dest,value)
   #define MBCSTM_MDS_DEST_EQUAL(dest1,dest2)  m_NCS_MDS_DEST_EQUAL(dest1,dest2)
   #define MBCSTM_NODE_ID_FROM_MDS_DEST(mds_dest)  m_NCS_NODE_ID_FROM_MDS_DEST(mds_dest)
   #define MBCSTM_MDS_DEST_IS_AN_ADEST(mds_dest)  m_NCS_MDS_DEST_IS_AN_ADEST(mds_dest)
   
*/ 
/* for SPO2 patch */
#define MBCSTM_SET_VDEST_ID_IN_MDS_DEST(mds_dest,value)  m_NCSVDA_SET_VDEST(mds_dest,value)

/* structure definations */

typedef struct mbcstm_csi_data {
  uint32_t   data_id;
  uint32_t   sec;
  uint32_t   usec;    
} MBCSTM_CSI_DATA;

typedef enum  mbcstm_csi_data_type {
  NORMAL_DATA = 1,
  CONTROL_DATA,
  PERFORMANCE_DATA
} MBCSTM_CSI_DATA_TYPE;

typedef struct mbcstm_peer {    
  NCSCONTEXT  pwe_hdl;
  
} MBCSTM_PEER;

typedef struct mbcstm_ssn {
  uint32_t           svc_index;
  MDS_DEST        dest_id;
  V_DEST_QA       anchor;
  V_DEST_RL       dest_role;
  uint32_t           dest_status;
  SaAmfHAStateT   csi_role;    
  MDS_HDL      pwe_hdl;
  uint32_t           ckpt_hdl;
  uint32_t           ssn_index;
  uint32_t           ws_flag;
  uint32_t           ws_timer;
  /* cold or work sync related varibles */
  uint32_t           data_count;
  uint32_t           sync_count;
  MBCSTM_SYNC     cold_flag;
  MBCSTM_SYNC     warm_flag;
  uint32_t           data_req;
  uint32_t           data_req_count;
  MBCSTM_CSI_DATA data[MBCSTM_CSI_DATA_MAX];
  /* call back routine cases related */
  MBCSTM_CB_TEST  cb_test;
  uint32_t           cb_test_result;
  uint32_t           cb_flag;
  
  /* performance time */
  uint32_t           perf_flag;
  double          perf_init_time;
  double          perf_final_time;
  uint32_t           perf_data_size;
  uint32_t           perf_data_sent;
  uint32_t           perf_msg_size;
  uint32_t           perf_msg_inc;
  
  /* peer data */
  uint32_t           peer_count;
  MBCSTM_PEER     peers[MBCSTM_SSN_PEER_MAX];
} MBCSTM_SSN;

typedef struct mbcstm_svc {
  NCS_MBCSV_CLIENT_SVCID svc_id;    /* service id of client               */
  NCS_MBCSV_HDL          mbcsv_hdl; /* MBCSv returns handle for calls     */
 /* SaVersionT             version;    client version info as per SAF     */
  uint16_t                  version;
  SaSelectionObjectT     sel_obj;   /* slection object of queue           */
  SaDispatchFlagsT       disp_flags;/* one of ONE, ALL or BLOCKING        */
  uint32_t                  task_flag;
  uint32_t                  ssn_count;
  MBCSTM_SSN             ssns[MBCSTM_SSN_MAX];
  
} MBCSTM_SVC;

typedef struct mbcstm_vdests {
  MDS_DEST    dest_id;
  V_DEST_QA   anchor;
  uint32_t       status;
} MBCSTM_VDESTS;

typedef struct mbcstm_cb {
  MBCSTM_SYS    sys;
  uint32_t         flag;
  uint32_t         log_file_fd;
  NCS_LOCK      mbcstm_lock;
  uint32_t         vdest_count;
  MBCSTM_VDESTS vdests[MBCSTM_SSN_MAX];
  uint32_t         svc_count;
  MBCSTM_SVC    svces[MBCSTM_SVC_MAX]; 
} MBCSTM_CB;

/* NEW MDS: added on 21st november */
typedef struct mbcstm_peer_inst_data {
    
  MBCSTM_FSM_STATES    state;  /* State Machines Current State */
  V_DEST_QA            peer_anchor;
  MDS_DEST             peer_adest;
  uint32_t                peer_role;
  SaVersionT           version;  /* peer version info as per SAF*/
}MBCSTM_PEER_INST;

typedef struct mbcstm_peers {
  uint32_t            peer_count;
  MBCSTM_PEER_INST peers[MBCSTM_SSN_PEER_MAX]; /* NEW MDS: changed on 21st november */
} MBCSTM_PEERS_DATA;

/*    Global declarations */
MBCSTM_CB    mbcstm_cb;
/*Rajesh*/
V_DEST_QA    MBCSTM_SSN_ANC1,MBCSTM_SSN_ANC2,MBCSTM_SSN_ANC3,MBCSTM_SSN_ANC4;

/*Rajesh*/
uint32_t ncs_encode_32bit(uint8_t **,uint32_t);
uint32_t ncs_decode_32bit(uint8_t **);
/*     API declarations   */
/* initializtion and configuration related api */
uint32_t  mbcstm_input(void);
uint32_t  mbcstm_config(void);

/* start up and mds related API */
uint32_t   mbcstm_system_startup(void);
uint32_t   mbcstm_system_close(void);
uint32_t   mbcstm_dest_start(void);
uint32_t   mbcstm_specific_dest_start(uint32_t ssn_index);
uint32_t   mbcstm_specific_dest_close(uint32_t ssn_index);
uint32_t   mbcstm_dest_close(void);
uint32_t   mbcstm_dest_change_role(uint32_t svc_index, uint32_t ssn_index);  
uint32_t   mbcstm_start_process_thread(uint32_t svc_index);

/* syncronization related API */
uint32_t   mbcstm_sync_point(void);

/* svc installation related API */
uint32_t  mbcstm_svc_start(uint32_t svc_index);
uint32_t  mbcstm_svc_close(uint32_t svc_index);

/* svc, mbcsv operations related API */
uint32_t   mbcstm_svc_registration(uint32_t svc_index);
uint32_t   mbcstm_svc_finalize (uint32_t svc_index);
uint32_t   mbcstm_ssn_open(uint32_t svc_index, uint32_t ssn_index);
uint32_t   mbcstm_ssn_close(uint32_t svc_index, uint32_t ssn_index);
uint32_t   mbcstm_ssn_set_role (uint32_t svc_index, uint32_t ssn_index);
uint32_t   mbcstm_ssn_get_select(uint32_t svc_index);
uint32_t   mbcstm_svc_dispatch (uint32_t  svc_index);
uint32_t   mbcstm_svc_obj(uint32_t svc_index, uint32_t ssn_index, uint32_t action,NCS_MBCSV_OBJ obj);
uint32_t   mbcstm_svc_cp_send(uint32_t svc_index, uint32_t ssn_index, NCS_MBCSV_ACT_TYPE action, uint32_t reo_type, long reo_hanle, NCS_MBCSV_MSG_TYPE send_type);
uint32_t   mbcstm_svc_data_request(uint32_t svc_index, uint32_t ssn_index);
uint32_t   mbcstm_svc_notify(uint32_t svc_index, uint32_t ssn_index); 
uint32_t   mbcsv_svc_err_ind(NCS_MBCSV_CB_ARG *arg);

/* CALL BACK ROUTINES */   
uint32_t   mbcstm_svc_cb(NCS_MBCSV_CB_ARG *arg);
uint32_t   mbcstm_svc_encode_cb(NCS_MBCSV_CB_ARG *arg);
uint32_t   mbcstm_svc_decode_cb(NCS_MBCSV_CB_ARG *arg);
uint32_t   mbcstm_svc_peer_cb(NCS_MBCSV_CB_ARG *arg);
uint32_t   mbcstm_svc_notify_cb(NCS_MBCSV_CB_ARG *arg);
uint32_t   mbcstm_svc_enc_notify_cb(NCS_MBCSV_CB_ARG *arg);

/* util apis */
uint32_t mbcstm_print(char *file, char *fun_name, uint32_t line_num, char *call, uint32_t status); 
void  mbcsv_print_data(void);
uint32_t mbcstm_config_print(void);
uint32_t mbcstm_crc(char *str, uint32_t len);
uint32_t    mbcstm_system_startup(void);
uint32_t     mbcstm_system_close(void);
uint32_t     mbcstm_dest_start(void);
uint32_t mbcstm_dest_close(void);
uint32_t    mbcstm_ssn_open_all(uint32_t svc_index);
uint32_t   mbcstm_svc_send_notify(uint32_t svc_index, uint32_t ssn_index,
                               NCS_MBCSV_NTFY_MSG_DEST msg_dest, 
                               char *str, uint32_t len);
uint32_t  mbcstm_svc_err_ind(NCS_MBCSV_CB_ARG *arg);
uint32_t mbcstm_config_print(void);
uint32_t   mbcstm_sync_point(void);
MDS_DEST get_vdest_anchor(void);
uint32_t tet_mbcsv_config(void);
void tet_mbcsv_test(void);
void tet_mbcsv_send_checkpoint(int choice);
void tet_mbcsv_data_request(int choice);
void tet_get_set_warm_sync(int choice);
void tet_mbcsv_warm_sync(int choice);
void tet_mbcsv_cold_sync(int choice);
void tet_mbcsv_Notify(int choice);
void tet_mbcsv_change_role(int choice);
void tet_mbcsv_open_close(int choice);
void tet_mbcsv_initialize(int choice);
void tet_mbcsv_op(int choice);
uint32_t mbcstm_csync_perf_purposes(uint32_t svc_index, uint32_t ssn_index, uint32_t asys);
uint32_t mbcstm_disc_perf_purposes(uint32_t svc_index, uint32_t ssn_index, uint32_t asys, uint32_t nrepeat,MBCSTM_CB_TEST sync);
uint32_t mbcstm_msg_check_purposes(uint32_t svc_index, uint32_t ssn_index,NCS_MBCSV_MSG_TYPE send_type);
uint32_t mbcstm_verify_sync_msg(SSN_PERF_DATA *data, MBCSTM_SSN *ssn);
uint32_t mbcstm_perf_sync_msg(uint32_t svc_index, uint32_t ssn_index,uint32_t size,uint32_t send_type);
uint32_t mbcstm_check_inv(MBCSTM_CHECK check, uint32_t svc_index, uint32_t ssn_index, 
                       void *data);

#endif  // MBC_APITEST_MBCSV_API_H_
