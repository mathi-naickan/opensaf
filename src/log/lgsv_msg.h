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
 * Author(s): Ericsson AB
 *
 */

/*
 * Module Inclusion Control...
 */
#ifndef LOG_LGSV_MSG_H_
#define LOG_LGSV_MSG_H_

#include <limits.h>
#include "log/saf/saLog.h"

/* Message type enums */
typedef enum {
  LGSV_LGA_API_MSG = 0,
  LGSV_LGS_CBK_MSG = 1,
  LGSV_LGA_API_RESP_MSG = 2,
  LGSV_MSG_MAX
} lgsv_msg_type_t;

/* LGSV API enums */
typedef enum {
  LGSV_INITIALIZE_REQ = 0,
  LGSV_FINALIZE_REQ = 1,
  LGSV_STREAM_OPEN_REQ = 2,
  LGSV_STREAM_CLOSE_REQ = 3,
  LGSV_WRITE_LOG_ASYNC_REQ = 4,
  LGSV_API_MAX
} lgsv_api_msg_type_t;

/* LGSV Callback enums */
typedef enum {
  LGSV_WRITE_LOG_CALLBACK_IND = 0,
  LGSV_CLM_NODE_STATUS_CALLBACK = 1,
  LGSV_LGS_CBK_MAX
} lgsv_cbk_msg_type_t;

typedef enum {
  LGSV_INITIALIZE_RSP = 0,
  LGSV_FINALIZE_RSP = 1,
  LGSV_STREAM_OPEN_RSP = 2,
  LGSV_STREAM_CLOSE_RSP = 3,
  LGSV_LGA_API_RSP_MAX
} lgsv_api_resp_msg_type;

/*
 * API & callback API parameter definitions.
 * These definitions are used for
 * 1) encoding & decoding messages between LGA & LGS.
 * 2) storing the callback parameters in the pending callback
 *    list.
 */

/*** API Parameter definitions ***/
typedef struct {
  SaVersionT version;
} lgsv_initialize_req_t;

typedef struct {
  uint32_t client_id;
} lgsv_finalize_req_t;

typedef struct {
  uint32_t client_id;
  SaNameT lstr_name;
  char *logFileName;
  char *logFilePathName;
  SaUint64T maxLogFileSize;
  SaUint32T maxLogRecordSize;
  SaBoolT haProperty;
  SaLogFileFullActionT logFileFullAction;
  SaUint16T maxFilesRotated;
  SaUint16T logFileFmtLength;
  SaStringT logFileFmt;
  SaUint8T lstr_open_flags;
} lgsv_stream_open_req_t;

typedef struct {
  uint32_t client_id;
  uint32_t lstr_id;
} lgsv_stream_close_req_t;

typedef struct {
  SaInvocationT invocation;
  uint32_t ack_flags;
  uint32_t client_id;
  uint32_t lstr_id;
  SaLogRecordT *logRecord;
  SaNameT *logSvcUsrName;
  SaTimeT *logTimeStamp;
} lgsv_write_log_async_req_t;

/* API param definition */
typedef struct {
  lgsv_api_msg_type_t type;       /* api type */
  union {
    lgsv_initialize_req_t init;
    lgsv_finalize_req_t finalize;
    lgsv_stream_open_req_t lstr_open_sync;
    lgsv_stream_close_req_t lstr_close;
    lgsv_write_log_async_req_t write_log_async;
  } param;
} lgsv_api_info_t;

/*** Callback Parameter definitions ***/

typedef struct {
  SaAisErrorT error;
} lgsv_write_log_callback_ind_t;

/*CLM node status callback structure for LGS*/
typedef struct logsv_loga_clm_status_param_tag {
  uint32_t clm_node_status;
} logsv_lga_clm_status_cbk_t;

/* wrapper structure for all the callbacks */
typedef struct {
  lgsv_cbk_msg_type_t type;       /* callback type */
  uint32_t lgs_client_id; /* lgs client_id */
  SaInvocationT inv;      /* invocation value */
  /*      union {*/
  lgsv_write_log_callback_ind_t write_cbk;
  logsv_lga_clm_status_cbk_t clm_node_status_cbk;
  /*      } param; */
} lgsv_cbk_info_t;

/* API Response parameter definitions */
typedef struct {
  uint32_t client_id;
} lgsv_initialize_rsp_t;

typedef struct {
  uint32_t client_id;
  uint32_t lstr_id;
} lgsv_stream_open_rsp_t;

typedef struct {
  uint32_t client_id;
} lgsv_finalize_rsp_t;

typedef struct {
  uint32_t client_id;
  uint32_t lstr_id;
} lgsv_stream_close_rsp_t;

/* wrapper structure for all API responses
 */
typedef struct {
  lgsv_api_resp_msg_type type;    /* callback type */
  SaAisErrorT rc;         /* return code */
  union {
    lgsv_initialize_rsp_t init_rsp;
    lgsv_stream_open_rsp_t lstr_open_rsp;
    lgsv_finalize_rsp_t finalize_rsp;
    lgsv_stream_close_rsp_t close_rsp;
  } param;
} lgsv_api_rsp_info_t;

/* message used for LGA-LGS interaction */
typedef struct lgsv_msg {
  struct lgsv_msg *next;  /* for mailbox processing */
  lgsv_msg_type_t type;   /* message type */
  union {
    /* elements encoded by LGA (& decoded by LGS) */
    lgsv_api_info_t api_info;       /* api info */

    /* elements encoded by LGS (& decoded by LGA) */
    lgsv_cbk_info_t cbk_info;       /* callbk info */
    lgsv_api_rsp_info_t api_resp_info;      /* api response info */
  } info;
} lgsv_msg_t;

#endif  // LOG_LGSV_MSG_H_
