/*      -*- OpenSAF  -*-
 *
 * (C) Copyright 2010 The OpenSAF Foundation
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
 * Author(s):  Emerson Network Power
 */

#ifndef CLM_CLMSV_MSG_H_
#define CLM_CLMSV_MSG_H_

/* CLMS->CLMA && CLMA->CLMS message types */
typedef enum clms_msg_type {
  CLMSV_CLMA_TO_CLMS_API_MSG = 0,
  CLMSV_CLMS_TO_CLMA_CBK_MSG,
  CLMSV_CLMS_TO_CLMA_API_RESP_MSG,
  CLMSV_CLMS_TO_CLMA_IS_MEMBER_MSG,
  CLMSV_CLMS_TO_CLMNA_REBOOT_MSG,
  CLMSV_MSG_MAX
} CLMSV_MSG_TYPE;

/* CLM API enums */
typedef enum clmsv_api_type {
  CLMSV_INITIALIZE_REQ = 0,
  CLMSV_FINALIZE_REQ,
  CLMSV_TRACK_START_REQ,
  CLMSV_TRACK_STOP_REQ,
  CLMSV_NODE_GET_REQ,
  CLMSV_NODE_GET_ASYNC_REQ,
  CLMSV_RESPONSE_REQ,
  CLMSV_CLUSTER_JOIN_REQ,
  CLMSV_API_MAX
} CLMSV_API_MSG_TYPE;

typedef enum clmsv_api_resp_type {
  CLMSV_INITIALIZE_RESP = 0,
  CLMSV_FINALIZE_RESP,
  CLMSV_TRACK_CURRENT_RESP,
  CLMSV_TRACK_STOP_RESP,
  CLMSV_NODE_GET_RESP,
  CLMSV_NODE_GET_ASYNC_RESP,
  CLMSV_RESPONSE_RESP,
  CLMSV_CLUSTER_JOIN_RESP,
  CLMSV_API_RESP_MAX
} CLMSV_API_RESP_MSG_TYPE;

/* CLM Callback API enums */
typedef enum clmsv_cbk_type {
  CLMSV_TRACK_CBK = 0,
  CLMSV_NODE_ASYNC_GET_CBK,
  CLMSV_CBK_MAX
} CLMSV_CBK_MSG_TYPE;

/* Structures to hold messages exchanged between CLMA && CLMS */
/* CLMA Initialize Request */
typedef struct {
  SaVersionT version;     /* Not needed.TBRemoved */
} clmsv_init_param_t;

/* CLMA Finalize Request */
typedef struct {
  SaUint32T client_id;
} clmsv_finalize_param_t;

/* CLMA Track Start Request */
typedef struct {
  SaUint32T client_id;
  SaUint8T flags;
  SaUint8T sync_resp;
} clmsv_track_start_param_t;

/* CLMA Track Stop Request */
typedef struct {
  SaUint32T client_id;
} clmsv_track_stop_param_t;

/* CLMA Node Get Request */
typedef struct {
  SaUint32T client_id;
  SaClmNodeIdT node_id;
} clmsv_node_get_param_t;

/* CLMA NodeGet Async Request */
typedef struct {
  SaUint32T client_id;
  SaInvocationT inv;
  SaClmNodeIdT node_id;
} clmsv_node_get_async_param_t;

/*ClmResponse from CLMA to CLMS*/
typedef struct {
  SaUint32T client_id;
  SaUint32T resp;
  SaInvocationT inv;
} clmsv_clm_response_param_t;

/* CLMS To CLMA track callback delivery */
typedef struct clm_track_cbk_param_t {
  SaClmClusterNotificationBufferT_4 buf_info;
  SaUint32T mem_num;
  SaInvocationT inv;
  SaNameT *root_cause_ent;
  SaNtfCorrelationIdsT *cor_ids;
  SaClmChangeStepT step;
  SaTimeT time_super;
  SaAisErrorT err;
} CLMSV_TRACK_CBK_INFO;

/* CLMS To CLMA node get async callback delivery */
typedef struct clma_node_get_async_cbk_param_t {
  SaAisErrorT err;
  SaInvocationT inv;
  SaClmClusterNodeT_4 info;
} CLMSV_NODE_GET_ASYNC_CBK_INFO;

/* Top most containment structure for CLMS to CLMA callback types */
typedef struct clma_cbk_info {
  SaUint32T client_id;
  CLMSV_CBK_MSG_TYPE type;
  union {
    CLMSV_TRACK_CBK_INFO track;
    CLMSV_NODE_GET_ASYNC_CBK_INFO node_get;
  } param;
} CLMSV_CBK_INFO;

/* clm track info */
typedef struct clmsv_track_info_t {
  SaClmClusterNotificationBufferT_4 *notify_info; /* Node Info */
  /*   SaUint32T  view_num; */    /* Current view number */
  SaUint16T num;          /* no of items */
} CLMSV_TRACK_INFO;

typedef struct {
  SaUint32T node_id;
  SaNameT node_name;
} clmsv_clms_node_up_info_t;

typedef struct clmsv_api_info_t {
  CLMSV_API_MSG_TYPE type;
  union {
    clmsv_init_param_t init;
    clmsv_finalize_param_t finalize;
    clmsv_track_start_param_t track_start;
    clmsv_track_stop_param_t track_stop;
    clmsv_node_get_param_t node_get;
    clmsv_node_get_async_param_t node_get_async;
    clmsv_clm_response_param_t clm_resp;
    clmsv_clms_node_up_info_t nodeup_info;
  } param;
} CLMSV_API_INFO;

typedef struct clmsv_api_resp_info_t {
  CLMSV_API_RESP_MSG_TYPE type;
  SaAisErrorT rc;
  union {
    SaUint32T client_id;
    SaClmClusterNodeT_4 node_get;
    SaInvocationT inv;
    CLMSV_TRACK_INFO track;
    SaNameT node_name;
  } param;
} CLMSV_API_RESP_INFO;

typedef struct clmsv_is_member_info_t {
  SaBoolT is_member;
  SaBoolT is_configured;
  SaUint32T client_id;
}CLMSV_IS_MEMBER_INFO;

typedef struct clmsv_reboot_info_t {
  SaClmNodeIdT node_id;
} CLMSV_REBOOT_INFO;

/* Top Level CLMSv MDS message structure for use between CLMS-> CLMA && CLMA -> CLMS */
typedef struct clmsv_msg_t {
  struct clmsv_msg_t *next;       /* Mailbox processing */
  CLMSV_MSG_TYPE evt_type;        /* Message type */
  union {
    CLMSV_API_INFO api_info;        /* Messages Between CLA to CLMS */
    CLMSV_CBK_INFO cbk_info;        /* Callback Messages from CLMS to CLA */
    CLMSV_API_RESP_INFO api_resp_info;      /* Response Messages from CLMS to CLA */
    CLMSV_IS_MEMBER_INFO is_member_info;    /*Is node member or not Message from CLMS to CLA*/
    CLMSV_REBOOT_INFO reboot_info;	/* Reboot request from CLMS to CLMNA */
  } info;
} CLMSV_MSG;

#endif  // CLM_CLMSV_MSG_H_
