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
  
    
..............................................................................
      
DESCRIPTION:
        
This include file contains the message definitions for EDA and EDSV
communication.
          
******************************************************************************
*/

/*
* Module Inclusion Control...
*/
#ifndef EDSV_MSG_H
#define EDSV_MSG_H

#include <saEvt.h>

/* AMF API enums */
typedef enum edsv_msg_type {
	EDSV_BASE_MSG,
	EDSV_EDA_API_MSG = EDSV_BASE_MSG,
	EDSV_EDA_MISC_MSG,
	EDSV_EDS_MISC_MSG,
	EDSV_EDS_CBK_MSG,
	EDSV_EDA_API_RESP_MSG,
	EDSV_MSG_MAX
} EDSV_MSG_TYPE;

/* EDSV API enums */
typedef enum edsv_api_msg_type {
	EDSV_API_BASE_MSG,
	EDSV_EDA_INITIALIZE = EDSV_API_BASE_MSG,
	EDSV_EDA_SEL_OBJ_GET,
	EDSV_EDA_DISPATCH,
	EDSV_EDA_FINALIZE,
	EDSV_EDA_CHAN_OPEN_SYNC,
	EDSV_EDA_CHAN_OPEN_ASYNC,
	EDSV_EDA_CHAN_CLOSE,
	EDSV_EDA_CHAN_UNLINK,
	EDSV_EDA_EVT_ALLOC,
	EDSV_EDA_EVT_FREE,
	EDSV_EDA_ATTRIB_SET,
	EDSV_EDA_ATTRIB_GET,
	EDSV_EDA_DATA_GET,
	EDSV_EDA_PUBLISH,
	EDSV_EDA_SUBSCRIBE,
	EDSV_EDA_UNSUBSCRIBE,
	EDSV_EDA_RETENTION_TIME_CLR,
	EDSV_EDA_LIMIT_GET,
	EDSV_API_MAX
} EDSV_API_TYPE;

/* EDSV Callback enums */

typedef enum edsv_cbk_msg_type {
	EDSV_CBK_BASE_MSG,
	EDSV_EDS_CHAN_OPEN = EDSV_CBK_BASE_MSG,
	EDSV_EDS_DELIVER_EVENT,
	EDSV_EDS_CLMNODE_STATUS,
	EDSV_EDS_CBK_MAX
} EDSV_CBK_TYPE;

typedef enum edsv_api_resp_msg_type {
	EDSV_EDA_INITIALIZE_RSP_MSG = 1,
	EDSV_EDA_CHAN_OPEN_SYNC_RSP_MSG,
	EDSV_EDA_CHAN_UNLINK_SYNC_RSP_MSG,
	EDSV_EDA_CHAN_RETENTION_TIME_CLEAR_SYNC_RSP_MSG,
	EDSV_EDA_LIMIT_GET_RSP_MSG,
	EDSV_EDA_API_RSP_MAX
} EDSV_API_RSP_TYPE;

/* 
* API & callback API parameter definitions.
* These definitions are used for 
* 1) encoding & decoding messages between EDA & EDS.
* 2) storing the callback parameters in the pending callback 
*    list.
*/

/*** API Parameter definitions ***/

/* initialize - not reqd */
typedef struct edsv_eda_init_param_tag {
	SaVersionT version;
} EDSV_EDA_INIT_PARAM;

/* selection object get - not reqd */
typedef struct edsv_eda_sel_obj_get_param_tag {
	void *dummy;
} EDSV_EDA_SEL_OBJ_GET_PARAM;

/* dispatch  - not reqd */
typedef struct edsv_eda_dispatch_param_tag {
	void *dummy;
} EDSV_EDA_DISPATCH_PARAM;

typedef struct edsv_eda_finalize_param_tag {
	uns32 reg_id;
} EDSV_EDA_FINALIZE_PARAM;

typedef struct edsv_eda_chan_open_sync_param_tag {
	uns32 reg_id;
	uns8 chan_open_flags;
	SaNameT chan_name;
} EDSV_EDA_CHAN_OPEN_SYNC_PARAM;

typedef struct edsv_eda_chan_unlink_sync_rsp_tag {
	SaNameT chan_name;
} EDSV_EDA_CHAN_UNLINK_SYNC_RSP;

typedef struct edsv_eda_chan_open_async_param_tag {
	SaInvocationT inv;
	uns32 reg_id;
	uns8 chan_open_flags;
	SaNameT chan_name;
} EDSV_EDA_CHAN_OPEN_ASYNC_PARAM;

typedef struct edsv_eda_chan_close_param_tag {
	uns32 reg_id;
	uns32 chan_id;
	uns32 chan_open_id;
} EDSV_EDA_CHAN_CLOSE_PARAM;

typedef struct edsv_eda_chan_unlink_param_tag {
	uns32 reg_id;
	SaNameT chan_name;
} EDSV_EDA_CHAN_UNLINK_PARAM;

typedef struct edsv_eda_evt_alloc_param_tag {
	void *dummy;
} EDSV_EDA_EVT_ALLOC_PARAM;

typedef struct edsv_eda_evt_free_param_tag {
	void *dummy;
} EDSV_EDA_EVT_FREE_PARAM;

typedef struct edsv_eda_attrib_set_param_tag {
	void *dummy;
} EDSV_EDA_ATTRIB_SET_PARAM;

typedef struct edsv_eda_attrib_get_param_tag {
	void *dummy;
} EDSV_EDA_ATTRIB_GET_PARAM;

typedef struct edsv_eda_data_get_param_tag {
	void *dummy;
} EDSV_EDA_DATA_GET_PARAM;

typedef struct edsv_eda_publish_param_tag {
	uns32 reg_id;
	uns32 chan_id;
	uns32 chan_open_id;
	SaEvtEventPatternArrayT *pattern_array;
	uns8 priority;
	SaTimeT retention_time;
	SaNameT publisher_name;
	uns32 event_id;
	SaSizeT data_len;
	uns8 *data;
} EDSV_EDA_PUBLISH_PARAM;

typedef struct edsv_eda_subscribe_param_tag {
	uns32 reg_id;
	uns32 chan_id;
	uns32 chan_open_id;
	uns32 sub_id;
	SaEvtEventFilterArrayT *filter_array;
} EDSV_EDA_SUBSCRIBE_PARAM;

typedef struct edsv_eda_unsubscribe_param_tag {
	uns32 reg_id;
	uns32 chan_id;
	uns32 chan_open_id;
	uns32 sub_id;
} EDSV_EDA_UNSUBSCRIBE_PARAM;

typedef struct edsv_eda_retention_time_clr_param_tag {
	uns32 reg_id;
	uns32 chan_id;
	uns32 chan_open_id;
	uns32 event_id;
} EDSV_EDA_RETENTION_TIME_CLR_PARAM;

/* API param definition */
typedef struct edsv_api_info_tag {
	EDSV_API_TYPE type;	/* api type */
	union {
		EDSV_EDA_INIT_PARAM init;
		EDSV_EDA_FINALIZE_PARAM finalize;
		EDSV_EDA_CHAN_OPEN_SYNC_PARAM chan_open_sync;
		EDSV_EDA_CHAN_OPEN_ASYNC_PARAM chan_open_async;
		EDSV_EDA_CHAN_CLOSE_PARAM chan_close;
		EDSV_EDA_CHAN_UNLINK_PARAM chan_unlink;
		EDSV_EDA_PUBLISH_PARAM publish;
		EDSV_EDA_SUBSCRIBE_PARAM subscribe;
		EDSV_EDA_UNSUBSCRIBE_PARAM unsubscribe;
		EDSV_EDA_RETENTION_TIME_CLR_PARAM rettimeclr;
	} param;
} EDSV_API_INFO;

/*** Callback Parameter definitions ***/

typedef struct edsv_eda_chan_open_cb_param_tag {
	SaNameT chan_name;
	uns32 chan_id;
	uns32 chan_open_id;
	uns8 chan_open_flags;
	uns32 eda_chan_hdl;	/* filled in at the EDA with channelHandle, use 0 at EDS */
	SaAisErrorT error;
} EDSV_EDA_CHAN_OPEN_CBK_PARAM;

typedef struct edsv_eda_evt_deliver_cb_param_tag {
	SaEvtSubscriptionIdT sub_id;
	uns32 chan_id;
	uns32 chan_open_id;
	SaEvtEventPatternArrayT *pattern_array;
	uns8 priority;
	SaNameT publisher_name;
	SaTimeT publish_time;
	SaTimeT retention_time;
	uns32 eda_event_id;
	uns32 event_hdl;	/* filled in at the EDA with eventHandle use 0 at EDS */
	uns32 ret_evt_ch_oid;
	SaSizeT data_len;
	uns8 *data;
} EDSV_EDA_EVT_DELIVER_CBK_PARAM;

typedef struct edsv_eda_clm_status_param_tag {
	uns16 node_status;
} EDSV_EDA_CLM_STATUS_CBK_PARAM;

/* wrapper structure for all the callbacks */
typedef struct edsv_cbk_info {
	EDSV_CBK_TYPE type;	/* callback type */
	uns32 eds_reg_id;	/* eds reg_id */
	SaInvocationT inv;	/* invocation value */

	union {
		EDSV_EDA_CHAN_OPEN_CBK_PARAM chan_open_cbk;
		EDSV_EDA_EVT_DELIVER_CBK_PARAM evt_deliver_cbk;
		EDSV_EDA_CLM_STATUS_CBK_PARAM clm_status_cbk;
	} param;
} EDSV_CBK_INFO;

/* API Response parameter definitions */
typedef struct edsv_eda_initialize_rsp_tag {
	uns32 reg_id;		/* Map to evtHandle */
} EDSV_EDA_INITIALIZE_RSP;

typedef struct edsv_eda_limit_get_rsp_tag {
	/* Event Service Limits */
	SaUint64T max_chan;
	SaUint64T max_evt_size;
	SaUint64T max_ptrn_size;
	SaUint64T max_num_ptrns;
	SaTimeT max_ret_time;
} EDSV_EDA_LIMIT_GET_RSP;

typedef struct edsv_eda_chan_open_sync_rsp_tag {
	uns32 chan_id;
	uns32 chan_open_id;
} EDSV_EDA_CHAN_OPEN_SYNC_RSP;

typedef struct edsv_eda_publish_rsp_tag {
	void *dummy;
} EDSV_EDA_PUBLISH_RSP;

/* wrapper structure for all API responses 
 */
typedef struct edsv_api_rsp_info_tag {
	union {
		uns32 raw;
		EDSV_API_RSP_TYPE api_rsp;
		EDSV_CBK_TYPE cbk;
	} type;
	SaAisErrorT rc;		/* return code */
	union {
		EDSV_EDA_INITIALIZE_RSP init_rsp;
		EDSV_EDA_CHAN_OPEN_SYNC_RSP chan_open_rsp;
		EDSV_EDA_CHAN_UNLINK_SYNC_RSP chan_unlink_rsp;
		EDSV_EDA_LIMIT_GET_RSP limit_get_rsp;
	} param;
} EDSV_API_RSP_INFO;

/* wrapper structure for EDA-EDS misc messages 
 */
typedef struct edsv_eda_misc_info_tag {
	void *dummy;
} EDSV_EDA_MISC_MSG_INFO;

/* wrapper structure for EDA-EDS misc messages 
 */
typedef struct edsv_eds_misc_info_tag {
	void *dummy;
} EDSV_EDS_MISC_MSG_INFO;

/* message used for EDA-EDS interaction */
typedef struct edsv_msg {
	struct edsv_msg *next;	/* for mailbox processing */
	EDSV_MSG_TYPE type;	/* message type */
	union {
		/* elements encoded by EDA (& decoded by EDS) */
		EDSV_API_INFO api_info;	/* api info */
		EDSV_EDA_MISC_MSG_INFO edsv_eda_misc_info;

		/* elements encoded by EDS (& decoded by EDA) */
		EDSV_CBK_INFO cbk_info;	/* callbk info */
		EDSV_API_RSP_INFO api_resp_info;	/* api response info */
		EDSV_EDS_MISC_MSG_INFO edsv_eds_misc_info;
	} info;
} EDSV_MSG;

#define EDSV_WAIT_TIME 500

#endif   /* !EDSV_MSG_H */
