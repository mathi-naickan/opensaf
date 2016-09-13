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
#ifndef NTFSV_MSG_H
#define NTFSV_MSG_H

#include "saNtf.h"
#include "ncsgl_defs.h"
#include <mds_papi.h>
#ifdef  __cplusplus
extern "C" {
#endif

#define NTFS_MAX_STRING_LEN 64

/* Common notification limits TODO: specifify */
/*MAX length of addtionaltext conforms to MAX value of logMaxLogrecsize
  as mentioned in LOGSV PR doc Section 3.5.2.1*/
#define MAX_ADDITIONAL_TEXT_LENGTH 65535

/* Message type enums */
typedef enum {
  NTFSV_NTFA_API_MSG = 1,
  NTFSV_NTFS_CBK_MSG = 2,
  NTFSV_NTFA_API_RESP_MSG = 3,
  NTFSV_MSG_MAX
} ntfsv_msg_type_t;

/* NTFSV API enums */
typedef enum {
  NTFSV_INITIALIZE_REQ = 1,
  NTFSV_FINALIZE_REQ = 2,
  NTFSV_SUBSCRIBE_REQ = 3,
  NTFSV_UNSUBSCRIBE_REQ = 4,
  NTFSV_SEND_NOT_REQ = 5,
  NTFSV_READER_INITIALIZE_REQ = 6,
  NTFSV_READER_FINALIZE_REQ = 7,
  NTFSV_READ_NEXT_REQ = 8,
  NTFSV_READER_INITIALIZE_REQ_2 = 9,
  NTFSV_API_MAX
} ntfsv_api_msg_type_t;

/* NTFSV Callback enums */
typedef enum {
  NTFSV_DUMMY_CALLBACK = 0,
  NTFSV_NOTIFICATION_CALLBACK = 1,
  NTFSV_DISCARDED_CALLBACK = 2,
  NTFSV_CLM_NODE_STATUS_CALLBACK = 3,
  NTFSV_NTFS_CBK_MAX = 3
} ntfsv_cbk_msg_type_t;

typedef enum {
  NTFSV_INITIALIZE_RSP = 1,
  NTFSV_SUBSCRIBE_RSP = 2,
  NTFSV_SEND_NOT_RSP = 3,
  NTFSV_UNSUBSCRIBE_RSP = 4,
  NTFSV_READER_INITIALIZE_RSP = 5,
  NTFSV_READER_FINALIZE_RSP = 6,
  NTFSV_READ_NEXT_RSP = 7,
  NTFSV_FINALIZE_RSP = 8,
  NTFSV_NTFA_API_RSP_MAX
} ntfsv_api_resp_msg_type;

/*
 * API & callback API parameter definitions.
 * These definitions are used for
 * 1) encoding & decoding messages between NTFA & NTFS.
 * 2) storing the callback parameters in the pending callback
 *    list.
 */

/*** API Parameter definitions ***/

/* Notification message structs */

typedef struct ntfsv_filter_ptrs {
  SaNtfObjectCreateDeleteNotificationFilterT *obj_cr_del_filter;
  SaNtfAttributeChangeNotificationFilterT *att_ch_filter;
  SaNtfStateChangeNotificationFilterT *sta_ch_filter;
  SaNtfAlarmNotificationFilterT *alarm_filter;
  SaNtfSecurityAlarmNotificationFilterT *sec_al_filter;
} ntfsv_filter_ptrs_t;

typedef struct variable_data {
  void *p_base;
  size_t size;
  SaUint32T max_data_size;
} v_data;

struct ntfsv_send_not_req {
  SaNtfNotificationTypeT notificationType;

  /* Notification type specific attributes */
  union {
    SaNtfAlarmNotificationT alarm;
    SaNtfSecurityAlarmNotificationT securityAlarm;
    SaNtfStateChangeNotificationT stateChange;
    SaNtfAttributeChangeNotificationT attributeChange;
    SaNtfObjectCreateDeleteNotificationT objectCreateDelete;
  } notification;

  /* only used for server --> lib */
  SaNtfSubscriptionIdT subscriptionId;

  uint32_t client_id;
  v_data variable_data;
};
typedef struct ntfsv_send_not_req ntfsv_send_not_req_t;

typedef struct {
  SaVersionT version;
} ntfsv_initialize_req_t;

typedef struct {
  uint32_t client_id;
} ntfsv_finalize_req_t;

typedef struct {
  uint32_t client_id;
  SaNtfSearchCriteriaT searchCriteria;
} ntfsv_reader_init_req_t;

typedef struct {
  ntfsv_reader_init_req_t head;
  ntfsv_filter_ptrs_t f_rec;
} ntfsv_reader_init_req_2_t;

typedef struct {
  uint32_t client_id;
  uint32_t readerId;
} ntfsv_reader_finalize_req_t;

typedef struct {
  uint32_t client_id;
  SaNtfSearchDirectionT searchDirection;
  uint32_t readerId;
} ntfsv_read_next_req_t;

/*** Callback Parameter definitions ***/
typedef struct {
  SaNtfNotificationTypeT notificationType;
  SaUint32T numberDiscarded;
  SaNtfIdentifierT *discardedNotificationIdentifiers;
} ntfsv_discarded_info_t;

typedef struct {
  uint32_t client_id;
  SaNtfSubscriptionIdT subscriptionId;
  ntfsv_filter_ptrs_t f_rec;
  ntfsv_discarded_info_t d_info;
} ntfsv_subscribe_req_t;

typedef struct {
  uint32_t client_id;
  SaNtfSubscriptionIdT subscriptionId;
} ntfsv_unsubscribe_req_t;

/* API param definition */
typedef struct {
  ntfsv_api_msg_type_t type;      /* api type */
  union {
    ntfsv_initialize_req_t init;
    ntfsv_finalize_req_t finalize;
    ntfsv_subscribe_req_t subscribe;
    ntfsv_unsubscribe_req_t unsubscribe;
    ntfsv_send_not_req_t *send_notification;
    ntfsv_reader_init_req_t reader_init;
    ntfsv_reader_finalize_req_t reader_finalize;
    ntfsv_read_next_req_t read_next;
    ntfsv_reader_init_req_2_t reader_init_2;
  } param;
} ntfsv_api_info_t;

/*CLM node status callback structure for NTFA*/
typedef struct ntfsv_ntfa_clm_status_param_tag {
  uint32_t clm_node_status;
} ntfsv_ntfa_clm_status_cbk_t;

/* wrapper structure for all the callbacks */
typedef struct {
  ntfsv_cbk_msg_type_t type;      /* callback type */
  uint32_t ntfs_client_id;        /* ntfs client_id */
  SaNtfSubscriptionIdT subscriptionId;
  MDS_SEND_PRIORITY_TYPE mds_send_priority;
  union {
    ntfsv_send_not_req_t *notification_cbk;
    ntfsv_discarded_info_t discarded_cbk;
    ntfsv_ntfa_clm_status_cbk_t clm_node_status_cbk;
  } param;
} ntfsv_cbk_info_t;

/* API Response parameter definitions */
typedef struct {
  uint32_t client_id;
} ntfsv_initialize_rsp_t;

typedef struct {
  uint32_t readerId;
} ntfsv_reader_init_rsp_t;

typedef struct {
  uint32_t reader_id;
} ntfsv_reader_finalize_rsp_t;

typedef struct {
  ntfsv_send_not_req_t *readNotification;
} ntfsv_read_next_rsp_t;

typedef struct {
  SaNtfSubscriptionIdT subscriptionId;
} ntfsv_subscribe_rsp_t;

typedef struct {
  SaNtfSubscriptionIdT subscriptionId;
} ntfsv_unsubscribe_rsp_t;

typedef struct {
  SaNtfIdentifierT notificationId;
} ntfsv_send_not_rsp_t;

/* wrapper structure for all API responses
 */
typedef struct {
  ntfsv_api_resp_msg_type type;   /* response type */
  SaAisErrorT rc; /* return code */
  union {
    ntfsv_initialize_rsp_t init_rsp;
    ntfsv_subscribe_rsp_t subscribe_rsp;
    ntfsv_unsubscribe_rsp_t unsubscribe_rsp;
    ntfsv_send_not_rsp_t send_not_rsp;
    ntfsv_reader_init_rsp_t reader_init_rsp;
    ntfsv_reader_finalize_rsp_t reader_finalize_rsp;
    ntfsv_read_next_rsp_t read_next_rsp;
  } param;
} ntfsv_api_rsp_info_t;

/* message used for NTFA-NTFS interaction */
typedef struct ntfsv_msg {
  struct ntfsv_msg *next; /* for mailbox processing */
  ntfsv_msg_type_t type;  /* message type */
  union {
    /* elements encoded by NTFA (& decoded by NTFS) */
    ntfsv_api_info_t api_info;      /* api info */

    /* elements encoded by NTFS (& decoded by NTFA) */
    ntfsv_cbk_info_t cbk_info;      /* callbk info */
    ntfsv_api_rsp_info_t api_resp_info;     /* api response info */
  } info;
} ntfsv_msg_t;

struct NtfGlobals {
  SaNtfIdentifierT notificationId;
  SaUint64T clientIdCounter;
};

#ifdef  __cplusplus
}
#endif

#endif   /* !NTFSV_MSG_H */
