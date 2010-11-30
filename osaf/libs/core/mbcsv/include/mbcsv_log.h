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

  DESCRIPTION:

  This module contains logging/tracing functions.

******************************************************************************
*/

/*
 * Module Inclusion Control...
 */
#ifndef MBCSV_LOG_H
#define MBCSV_LOG_H

/*******************************************************************************************/

/******************************************************************************
 Logging offset indexes for Headline logging
 ******************************************************************************/

typedef enum mbcsv_hdln_flex {
	MBCSV_HDLN_MBCSV_CREATE_FAILED,
	MBCSV_HDLN_MBCSV_CREATE_SUCCESS,
	MBCSV_HDLN_MBCSV_DESTROY_FAILED,
	MBCSV_HDLN_MBCSV_DESTROY_SUCCESS,
	MBCSV_HDLN_NULL_INST,

} MBCSV_HDLN_FLEX;

/******************************************************************************
 Logging offset indexes for Service Provider (MDS) logging
 ******************************************************************************/

typedef enum mbcsv_svc_prvdr_flex {
	MBCSV_SP_MDS_INSTALL_FAILED,
	MBCSV_SP_MDS_INSTALL_SUCCESS,
	MBCSV_SP_MDS_UNINSTALL_FAILED,
	MBCSV_SP_MDS_UNINSTALL_SUCCESS,
	MBCSV_SP_MDS_SUBSCR_FAILED,
	MBCSV_SP_MDS_SUBSCR_SUCCESS,
	MBCSV_SP_MDS_SND_MSG_SUCCESS,
	MBCSV_SP_MDS_SND_MSG_FAILED,
	MBCSV_SP_MDS_RCV_MSG,
	MBCSV_SP_MDS_RCV_EVT_UP,
	MBCSV_SP_MDS_RCV_EVT_DN,

} MBCSV_SVC_PRVDR_FLEX;

/******************************************************************************
 Logging offset indexes for Lock logging 
 ******************************************************************************/

typedef enum mbcsv_locks_flex {
	MBCSV_LK_LOCKED,
	MBCSV_LK_UNLOCKED,
	MBCSV_LK_CREATED,
	MBCSV_LK_DELETED,

} MBCSV_LOCKS_FLEX;

/******************************************************************************
 Logging offset indexes for Memory Fail logging 
 ******************************************************************************/

typedef enum mbcsv_memfail_flex {
	MBCSV_MF_MBCSVMSG_CREATE,
	MBCSV_MF_MBCSVTBL_CREATE,
	MBCSV_MF_MBCSV_REGTBL_CREATE,
	MBCSV_MF_MBCSV_VCD_TBL_CREATE
} MBCSV_MEMFAIL_FLEX;

/******************************************************************************
 Logging offset indexes for API logging 
 ******************************************************************************/

typedef enum mbcsv_api_flex {
	MBCSV_API_IDLE,
	MBCSV_API_ACTIVE,
	MBCSV_API_STANDBY,
	MBCSV_API_QUIESCED,
	MBCSV_API_NONE,

	MBCSV_API_OP_INITIALIZE,
	MBCSV_API_OP_FINALIZE,
	MBCSV_API_OP_SELECT,
	MBCSV_API_OP_DISPATCH,
	MBCSV_API_OP_OPEN,
	MBCSV_API_OP_CLOSE,
	MBCSV_API_OP_CHG_ROLE,
	MBCSV_API_OP_SEND_CKPT,
	MBCSV_API_OP_SEND_NTFY,
	MBCSV_API_OP_SUB_ONESH,
	MBCSV_API_OP_SUB_ONEPE,
	MBCSV_API_OP_SUB_CANCEL,
	MBCSV_API_OP_GET,
	MBCSV_API_OP_SET,

} MBCSV_API_FLEX;

/******************************************************************************
 Logging offset indexes for peer event
 ******************************************************************************/

typedef enum mbcsv_peer_evt_flex {
	MBCSV_PEER_EV_DBG_IDLE,
	MBCSV_PEER_EV_DBG_ACTIVE,
	MBCSV_PEER_EV_DBG_STANDBY,
	MBCSV_PEER_EV_DBG_QUIESCED,
	MBCSV_PEER_EV_DBG_NONE,

	MBCSV_PEER_EV_PEER_UP,
	MBCSV_PEER_EV_PEER_DOWN,
	MBCSV_PEER_EV_PEER_INFO,
	MBCSV_PEER_EV_MBCSV_INFO_RSP,
	MBCSV_PEER_EV_CHG_ROLE,
	MBCSV_PEER_EV_MBCSV_UP,
	MBCSV_PEER_EV_MBCSV_DOWN,
	MBCSV_PEER_EV_ILLIGAL_EVT
} MBCSV_PEER_EVT_FLEX;
/******************************************************************************
 Logging offset indexes for Event logging 
 ******************************************************************************/

typedef enum mbcsv_fsm_event_flex {
	MBCSV_FSM_EV_DBG_IDLE,
	MBCSV_FSM_EV_DBG_ACTIVE,
	MBCSV_FSM_EV_DBG_STANDBY,
	MBCSV_FSM_EV_DBG_QUIESCED,
	MBCSV_FSM_EV_DBG_NONE,

	MBCSV_FSM_EV_EVENT_ASYNC_UPDATE,
	MBCSV_FSM_EV_EVENT_COLD_SYNC_REQ,
	MBCSV_FSM_EV_EVENT_COLD_SYNC_RESP,
	MBCSV_FSM_EV_EVENT_COLD_SYNC_RESP_CPLT,
	MBCSV_FSM_EV_EVENT_WARM_SYNC_REQ,
	MBCSV_FSM_EV_EVENT_WARM_SYNC_RESP,
	MBCSV_FSM_EV_EVENT_WARM_SYNC_RESP_CPLT,
	MBCSV_FSM_EV_EVENT_DATA_REQ,
	MBCSV_FSM_EV_EVENT_DATA_RESP,
	MBCSV_FSM_EV_EVENT_DATA_RESP_CPLT,
	MBCSV_FSM_EV_EVENT_NOTIFY,
	MBCSV_FSM_EV_SEND_COLD_SYNC_REQ,
	MBCSV_FSM_EV_SEND_WARM_SYNC_REQ,
	MBCSV_FSM_EV_SEND_ASYNC_UPDATE,
	MBCSV_FSM_EV_SEND_DATA_REQ,
	MBCSV_FSM_EV_SEND_COLD_SYNC_RESP,
	MBCSV_FSM_EV_SEND_COLD_SYNC_RESP_CPLT,
	MBCSV_FSM_EV_SEND_WARM_SYNC_RESP,
	MBCSV_FSM_EV_SEND_WARM_SYNC_RESP_CPLT,
	MBCSV_FSM_EV_SEND_DATA_RESP,
	MBCSV_FSM_EV_SEND_DATA_RESP_CPLT,
	MBCSV_FSM_EV_ENTITY_IN_SYNC,
	MBCSV_FSM_EV_SEND_NOTIFY,
	MBCSV_FSM_EV_TMR_SEND_COLD_SYNC,
	MBCSV_FSM_EV_TMR_SEND_WARM_SYNC,
	MBCSV_FSM_EV_TMR_COLD_SYNC_CPLT,
	MBCSV_FSM_EV_TMR_WARM_SYNC_CPLT,
	MBCSV_FSM_EV_TMR_DATA_RESP_CPLT,
	MBCSV_FSM_EV_TMR_TRANSMIT,
	MBCSV_FSM_EV_MULTIPLE_ACTIVE,
	MBCSV_FSM_EV_MULTIPLE_ACTIVE_CLEAR,

	/* FSM Active State */
	MBCSV_FSM_STATE_ACTIVE_IDLE,
	MBCSV_FSM_STATE_ACTIVE_WFCS,
	MBCSV_FSM_STATE_ACTIVE_KBIS,
	MBCSV_FSM_STATE_ACTIVE_MUAC,

	/* FSM Standby State */
	MBCSV_FSM_STATE_STBY_IDLE,
	MBCSV_FSM_STATE_STBY_WTCS,
	MBCSV_FSM_STATE_STBY_SIS,
	MBCSV_FSM_STATE_STBY_WTWS,
	MBCSV_FSM_STATE_STBY_VWSD,
	MBCSV_FSM_STATE_STBY_WFDR,

	MBCSV_FSM_STATE_QUIESCED,
	MBCSV_FSM_STATE_NONE,

} MBCSV_FSM_EVENT_FLEX;

/******************************************************************************
 Logging offset indexes for timer events
 ******************************************************************************/

typedef enum mbcsv_tmr_flex {
	MBCSV_TMR_IDLE,
	MBCSV_TMR_ACTIVE,
	MBCSV_TMR_STANDBY,
	MBCSV_TMR_QUIESCED,
	MBCSV_TMR_NONE,

	MBCSV_TMR_SND_COLD_SYNC,
	MBCSV_TMR_SND_WARM_SYNC,
	MBCSV_TMR_COLD_SYN_CMP,
	MBCSV_TMR_WARM_SYN_CMP,
	MBCSV_TMR_DATA_RSP_CMP,
	MBCSV_TMR_TRANSMIT,

	MBCSV_TMR_START,
	MBCSV_TMR_STOP,
	MBCSV_TMR_EXP
} MBCSV_TMR_FLEX;

/******************************************************************************
 Logging offset indexes for Debug String logging 
 ******************************************************************************/

typedef enum mbcsv_dbg_flex {
	MBCSV_DBG_IDLE,
	MBCSV_DBG_ACTIVE,
	MBCSV_DBG_STANDBY,
	MBCSV_DBG_QUIESCED,
	MBCSV_DBG_NONE
} MBCSV_DBG_FLEX;
/******************************************************************************/

/******************************************************************************
 Logging offset indexes for canned constant strings for the ASCII SPEC
 ******************************************************************************/

typedef enum mbcsv_flex_sets {
	MBCSV_FC_HDLN,
	MBCSV_FC_SVC_PRVDR_FLEX,
	MBCSV_FC_LOCKS,
	MBCSV_FC_MEMFAIL,
	MBCSV_FC_API,
	MBCSV_FC_PEER_EVT,
	MBCSV_FC_FSM_EVT,
	MBCSV_FC_TMR,
	MBCSV_FC_STR
} MBCSV_FLEX_SETS;

typedef enum mbcsv_log_ids {
	MBCSV_LID_HDLN,
	MBCSV_LID_SVC_PRVDR_FLEX,
	MBCSV_LID_GL_LOCKS,
	MBCSV_LID_SVC_LOCKS,
	MBCSV_LID_MEMFAIL,
	MBCSV_LID_API,
	MBCSV_LID_PEER_EVT,
	MBCSV_LID_FSM_EVT,
	MBCSV_LID_TMR,
	MBCSV_LID_STR,
	MBCSV_LID_DBG_SNK,
	MBCSV_LID_DBG_SNK_SVC
} MBCSV_LOG_IDS;

/*@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

                          MBCSV Logging Control

@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@*/
/* DTSv version support */
#define MBCSV_LOG_VERSION 2

#if (MBCSV_LOG == 1)
EXTERN_C uns32 mbcsv_log_str_lib_req(NCS_LIB_REQ_INFO *req_info);
EXTERN_C uns32 mbcsv_reg_strings(void);
EXTERN_C uns32 mbcsv_dereg_strings(void);
EXTERN_C uns32 mbcsv_log_bind(void);
EXTERN_C uns32 mbcsv_log_unbind(void);

#define m_LOG_MBCSV_HEADLINE(hdln_id) \
ncs_logmsg(NCS_SERVICE_ID_MBCSV, MBCSV_LID_HDLN, MBCSV_FC_HDLN, \
           NCSFL_LC_HEADLINE, NCSFL_SEV_INFO, "TI", hdln_id)

#define m_LOG_MBCSV_SVC_PRVDR(pwe, anch, sp_id)  \
{ \
   char log_str[32]; \
   snprintf(log_str,sizeof(log_str),"0x%llX",(uns64)anch); \
   ncs_logmsg(NCS_SERVICE_ID_MBCSV, MBCSV_LID_SVC_PRVDR_FLEX, MBCSV_FC_SVC_PRVDR_FLEX, \
           NCSFL_LC_SVC_PRVDR, NCSFL_SEV_INFO, "TLCI", pwe, log_str, sp_id); \
}

#define m_LOG_MBCSV_GL_LOCK(lck_id, lck)   \
ncs_logmsg(NCS_SERVICE_ID_MBCSV, MBCSV_LID_GL_LOCKS, MBCSV_FC_LOCKS, NCSFL_LC_LOCKS, \
         NCSFL_SEV_DEBUG, "TIL", lck_id, (long)lck)

  /* 
     In the following definition (long)lck is cast to uns32.
     In 64 bit machine this may truncate the address
   */
#define m_LOG_MBCSV_SVC_LOCK(lck_id, svc, lck)  \
ncs_logmsg(NCS_SERVICE_ID_MBCSV, MBCSV_LID_SVC_LOCKS, MBCSV_FC_LOCKS, NCSFL_LC_LOCKS, \
         NCSFL_SEV_DEBUG, "TILL", lck_id, svc, (uns32)(long)lck)

#define m_LOG_MBCSV_MEMFAIL(ha_state, mf_id)  \
ncs_logmsg(NCS_SERVICE_ID_MBCSV, MBCSV_LID_MEMFAIL, MBCSV_FC_MEMFAIL, NCSFL_LC_MEMORY, \
         NCSFL_SEV_CRITICAL, "TII", ha_state, mf_id)

#define m_LOG_MBCSV_API(ha_state, svc, ckpt, api, sev)  \
ncs_logmsg(NCS_SERVICE_ID_MBCSV, MBCSV_LID_API, MBCSV_FC_API, NCSFL_LC_API, \
         sev, "TILLI", ha_state, svc, ckpt, api)

#define m_LOG_MBCSV_PEER_EVT(ha_state, svc, ckpt, anch, evt)  \
{ \
   char log_str[32]; \
   snprintf(log_str,sizeof(log_str),"0x%llX",(uns64)anch); \
   ncs_logmsg(NCS_SERVICE_ID_MBCSV, MBCSV_LID_PEER_EVT, MBCSV_FC_PEER_EVT, NCSFL_LC_EVENT, \
         NCSFL_SEV_NOTICE, "TILLCI", ha_state, svc, ckpt, log_str, evt); \
}

#define m_LOG_MBCSV_FSM_EVT(ha_state, svc, ckpt, p_anch, f_state, evt)   \
{ \
   char log_str[32]; \
   snprintf(log_str,sizeof(log_str),"0x%llX",(uns64)p_anch); \
   ncs_logmsg(NCS_SERVICE_ID_MBCSV, MBCSV_LID_FSM_EVT, MBCSV_FC_FSM_EVT, NCSFL_LC_STATE,  \
           NCSFL_SEV_INFO, "TILLCII", ha_state, svc, ckpt, log_str, \
          (f_state + ((ha_state==MBCSV_HA_ROLE_INIT)?MBCSV_FSM_STATE_NONE: \
          ((ha_state==SA_AMF_HA_ACTIVE)?(MBCSV_FSM_STATE_ACTIVE_IDLE-1): \
          ((ha_state==SA_AMF_HA_STANDBY)?(MBCSV_FSM_STATE_STBY_IDLE-1):  \
            MBCSV_FSM_STATE_QUIESCED)))), evt); \
}

#define m_LOG_MBCSV_TMR(ha_state, svc, ckpt, p_anch, type, state)  \
{ \
   char log_str[32]; \
   snprintf(log_str,sizeof(log_str),"0x%llX",(uns64)p_anch); \
   ncs_logmsg(NCS_SERVICE_ID_MBCSV, MBCSV_LID_TMR, MBCSV_FC_TMR, NCSFL_LC_TIMER,  \
           NCSFL_SEV_INFO, "TILLCII", ha_state, svc, ckpt, log_str, type, state); \
}

#define m_LOG_MBCSV_DBGSTR(ha_state, svc, ckpt, str)   \
ncs_logmsg(NCS_SERVICE_ID_MBCSV, MBCSV_LID_STR, MBCSV_FC_STR, NCSFL_LC_MISC,  \
           NCSFL_SEV_DEBUG, "TILLC", ha_state, svc, ckpt, str);

#define m_LOG_MBCSV_DBG_SNK(str, file, line)   \
ncs_logmsg(NCS_SERVICE_ID_MBCSV, MBCSV_LID_DBG_SNK, MBCSV_FC_STR, NCSFL_LC_MISC,  \
           NCSFL_SEV_WARNING, "TCCL", str, file, line);

#define m_LOG_MBCSV_DBG_SNK_SVC(str, svc, file, line)   \
ncs_logmsg(NCS_SERVICE_ID_MBCSV, MBCSV_LID_DBG_SNK_SVC, MBCSV_FC_STR, NCSFL_LC_MISC, \
           NCSFL_SEV_WARNING, "TCLCL", str, svc, file, line);
#else

#define m_LOG_MBCSV_HEADLINE(hdln_id)
#define m_LOG_MBCSV_SVC_PRVDR(pwe, anch, sp_id)
#define m_LOG_MBCSV_GL_LOCK(id, lck)
#define m_LOG_MBCSV_SVC_LOCK(lck_id, svc, lck)
#define m_LOG_MBCSV_MEMFAIL(ha_state, mf_id)
#define m_LOG_MBCSV_API(ha_state, svc, ckpt, api, sev)
#define m_LOG_MBCSV_PEER_EVT(ha_state, svc, ckpt, anch, evt)
#define m_LOG_MBCSV_FSM_EVT(ha_state, svc, ckpt, p_anch, f_state, evt)
#define m_LOG_MBCSV_TMR(ha_state, svc, ckpt, p_anch, type, state)
#define m_LOG_MBCSV_DBGSTR(ha_state, svc, ckpt, str)
#define m_LOG_MBCSV_DBG_SNK(str, file, line)
#define m_LOG_MBCSV_DBG_SNK_SVC(str, file, line, svc)
#endif   /* MBCSV_LOG == 1 */

#endif   /* MBCSV_LOG_H */
